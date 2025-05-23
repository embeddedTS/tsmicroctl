#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <fcntl.h>
#include <gpiod.h>
#include <errno.h>
#include <syslog.h>
#include <string.h>
#include <sys/ioctl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>

#include "micro.h"

#define MIN_CHARGE_MV 3680
#define MAX_CHARGE_MV 4800

static int micro_chip_addr;

bool read_power_fail_status(struct gpiod_line *line, board_t *board);
struct gpiod_chip *init_power_fail_gpio(board_t *board, struct gpiod_line **line, const char *consumer_name);

int micro_init(int i2cbus, int i2caddr)
{
	static int fd = -1;
	char i2c_bus_path[20];

	if (fd != -1)
		return fd;

	snprintf(i2c_bus_path, sizeof(i2c_bus_path), "/dev/i2c-%d", i2cbus);
	fd = open(i2c_bus_path, O_RDWR);
	if (fd == -1) {
		perror("Couldn't open i2c device");
		exit(1);
	}

	/*
	 * We use force because there is typically a driver attached. This is
	 * safe because we are using only i2c_msgs and not read()/write() calls
	 */
	if (ioctl(fd, I2C_SLAVE_FORCE, i2caddr) < 0) {
		perror("Supervisor did not ACK\n");
		exit(1);
	}

	micro_chip_addr = i2caddr;

	return fd;
}

int micro_read(int i2cfd, uint16_t addr, void *data, size_t size)
{
	struct i2c_rdwr_ioctl_data packets;
	struct i2c_msg msgs[2];
	uint16_t swap_addr;

	swap_addr = addr >> 8;
	swap_addr |= (addr & 0xff) << 8;

	msgs[0].addr = micro_chip_addr;
	msgs[0].flags = 0;
	msgs[0].len = 2;
	msgs[0].buf = (uint8_t *)&swap_addr;

	msgs[1].addr = micro_chip_addr;
	msgs[1].flags = I2C_M_RD;
	msgs[1].len = size;
	msgs[1].buf = (uint8_t *)data;

	packets.msgs = msgs;
	packets.nmsgs = 2;

	if (ioctl(i2cfd, I2C_RDWR, &packets) < 0) {
		perror("Failed to read from supervisory micro");
		exit(1);
	}
	return 0;
}

int micro_write(int i2cfd, uint16_t addr, const void *data, size_t size)
{
	struct i2c_rdwr_ioctl_data packets;
	struct i2c_msg msg;
	uint8_t outdata[4096];

	/* The max size of 4k is not arbitrary, but may no longer be a limitation
	 * in the future. In older implementations, it was found that 4k was the
	 * max message size that could be sent in a single ioctl(). In theory,
	 * it should be 64k, as the kernel uses 16-bits for length. However, it has
	 * been found that some hardware, or drivers, or something was limited to 4k
	 * on some platforms. We stick to that, knowing that we need to limit data
	 * to 4094 bytes, plus 2 bytes of address for this interface.
	 */
	assert(size <= 4094);

	outdata[0] = ((addr >> 8) & 0xff);
	outdata[1] = (addr & 0xff);
	memcpy(&outdata[2], data, size);

	msg.addr = micro_chip_addr;
	msg.flags = 0;
	msg.len = 2 + size;
	msg.buf = outdata;

	packets.msgs = &msg;
	packets.nmsgs = 1;

	if (ioctl(i2cfd, I2C_RDWR, &packets) < 0) {
		perror("Failed to write to supervisory micro");
		exit(1);
	}
	return 0;
}

uint8_t micro_scaps_remaining_pct(int i2cfd, board_t *board)
{
	uint16_t current_voltage;
	uint32_t voltage_range, normalized_voltage;
	uint8_t remaining_percentage;

	// Read the current supercap voltage
	if (micro_read16_swap(i2cfd, MICRO_ADC_8, &current_voltage) < 0) {
		perror("Failed to read current supercap voltage");
		exit(EXIT_FAILURE);
	}

	// Calculate remaining percentage
	if (current_voltage <= MIN_CHARGE_MV) {
		remaining_percentage = 0;
	} else {
		normalized_voltage = current_voltage - MIN_CHARGE_MV;
		voltage_range = MAX_CHARGE_MV - MIN_CHARGE_MV;

		if (normalized_voltage >= voltage_range) {
			remaining_percentage = 100;
		} else {
			remaining_percentage = (normalized_voltage * 100 / voltage_range);
		}
	}

	// Ensure the remaining percentage does not exceed 100%
	if (remaining_percentage > 100) {
		remaining_percentage = 100;
	}

	return remaining_percentage;
}

uint16_t swap_endian16(uint16_t value)
{
	return (value << 8) | (value >> 8);
}

uint32_t swap_endian32(uint32_t value)
{
	return ((value >> 24) & 0x000000FF) | ((value >> 8) & 0x0000FF00) | ((value << 8) & 0x00FF0000) |
	       ((value << 24) & 0xFF000000);
}

int micro_read16_swap(int fd, int addr, uint16_t *data)
{
	int result = micro_read(fd, addr, (uint16_t *)data, sizeof(uint16_t));
	if (result >= 0)
		*data = swap_endian16(*data);
	return result;
}

int micro_write16_swap(int fd, int addr, uint16_t *data)
{
	uint16_t temp = swap_endian16(*data);
	return micro_write(fd, addr, &temp, sizeof(uint16_t));
}

// Read/Write 32-bit data with endianness swap
int micro_read32_swap(int fd, int addr, uint32_t *data)
{
	int result = micro_read(fd, addr, (uint32_t *)data, sizeof(uint32_t));
	if (result >= 0)
		*data = swap_endian32(*data);
	return result;
}

int micro_write32_swap(int fd, int addr, uint32_t *data)
{
	uint32_t temp = swap_endian32(*data);
	return micro_write(fd, addr, &temp, sizeof(uint32_t));
}

void micro_generic_info(int i2cfd, board_t *board)
{
	uint16_t temp, startup_temp;
	uint16_t charge_current;
	uint8_t status_flags;
	uint8_t revision;
	char build[80];

	if (micro_read8(i2cfd, MICRO_REVISION, &revision) < 0) {
		perror("Failed to read microcontroller version");
		exit(1);
	}
	printf("micro_revision=%d\n", revision);

	if (micro_read(i2cfd, MICRO_BUILD_STRING, build, sizeof(build)) < 0) {
		perror("Failed to read microcontroller build info");
		exit(1);
	}
	printf("micro_build=\"%s\"\n", build);

	if (micro_read16_swap(i2cfd, MICRO_ADC_4, &startup_temp) < 0) {
		perror("Failed to read startup temperature");
		exit(1);
	}
	printf("micro_startup_celcius=%d\n", startup_temp);

	if (micro_read16_swap(i2cfd, MICRO_ADC_10, &temp) < 0) {
		perror("Failed to read current temperature");
		exit(1);
	}
	printf("micro_celcius=%d\n", temp);

	if (micro_read8(i2cfd, MICRO_STATUS_FLAGS, &status_flags) < 0) {
		perror("Failed to read status flags");
		exit(1);
	}
	printf("usb_present=%d\n", !!(status_flags & MICRO_STATUS_FLAGS_USB_PRESENT));

	if (!board->has_silo)
		return;

	printf("power_fail=%d\n", !!(status_flags & MICRO_STATUS_FLAGS_POWER_FAIL));
	printf("scaps_enabled=%d\n", !!(status_flags & MICRO_STATUS_FLAGS_SCAPS_EN));
	printf("scaps_met_min=%d\n", !!(status_flags & MICRO_STATUS_FLAGS_SCAPS_MET_MIN));
	printf("scaps_charging=%d\n", !!(status_flags & MICRO_STATUS_FLAGS_SCAPS_CHARGING));
	printf("supercaps_remaining_pct=%d\n", micro_scaps_remaining_pct(i2cfd, board));

	if (micro_read16_swap(i2cfd, MICRO_CHARGE_CURRENT, &charge_current) < 0) {
		perror("Failed to read Supercaps charge current");
		exit(1);
	}
	printf("supercaps_charge_current_ma=%d\n", charge_current);

	if (micro_read16_swap(i2cfd, MICRO_CHARGE_CURRENT_DEFAULT, &charge_current) < 0) {
		perror("Failed to read Supercaps charge current default");
		exit(1);
	}
	printf("supercaps_charge_current_default_ma=%d\n", charge_current);
}

#define MAX_SLEEP_SECONDS (UINT32_MAX / 1000) 

void micro_sleep(int i2cfd, board_t *board, uint32_t seconds)
{
	uint8_t buf[5];
	uint32_t ms;

	// Check if the input seconds are within the valid range
	if (seconds > MAX_SLEEP_SECONDS) {
		fprintf(stderr, "Error: Invalid sleep duration. Please specify between 0 and %u seconds.\n",
			MAX_SLEEP_SECONDS);
		exit(EXIT_FAILURE);
	}

	ms = seconds * 1000;
	if (ms % 10)
		ms = (ms / 10) + 1;
	else
		ms = ms / 10;

	buf[0] = ms & 0xff;
	buf[1] = (ms >> 8) & 0xff;
	buf[2] = (ms >> 16) & 0xff;
	buf[3] = (ms >> 24) & 0xff;
	buf[4] = MICRO_CMD_SLEEP;

	if (micro_write(i2cfd, 1024, buf, sizeof(buf)) < 0) {
		perror("Failed to write sleep command to microcontroller");
		exit(EXIT_FAILURE);
	}
}

void micro_set_charge_current(int i2cfd, board_t *board, uint16_t ma)
{
	assert(ma >= board->min_current);
	assert(ma <= board->max_current);
	/* Write both the current and persistent charge rate */
	micro_write16_swap(i2cfd, MICRO_CHARGE_CURRENT_DEFAULT, &ma);
	micro_write16_swap(i2cfd, MICRO_CHARGE_CURRENT, &ma);
}

void micro_scaps_en(int i2cfd, board_t *board, int en)
{
	uint8_t value;

	micro_read8(i2cfd, MICRO_STATUS_FLAGS, &value);
	value &= ~6;
	if (en)
		value |= MICRO_STATUS_FLAGS_SCAPS_EN;
	micro_write8(i2cfd, MICRO_STATUS_FLAGS, &value);
}

struct gpiod_chip *init_power_fail_gpio(board_t *board, struct gpiod_line **line, const char *consumer_name)
{
	struct gpiod_chip *chip = gpiod_chip_open_by_label(board->power_fail_bank);
	if (!chip) {
		perror("Failed to open GPIO chip by label");
		exit(1);
	}

	*line = gpiod_chip_get_line(chip, board->power_fail_io);
	if (!*line) {
		perror("Failed to get GPIO line");
		gpiod_chip_close(chip);
		exit(1);
	}

	struct gpiod_line_request_config config = {
		.consumer = consumer_name,
		.request_type = GPIOD_LINE_REQUEST_DIRECTION_INPUT
	};

	if (gpiod_line_request(*line, &config, 0) < 0) {
		perror("Failed to request GPIO line as input");
		gpiod_chip_close(chip);
		exit(1);
	}

	return chip; // Return the opened GPIO chip for later cleanup
}

bool read_power_fail_status(struct gpiod_line *line, board_t *board)
{
	int value = gpiod_line_get_value(line);
	if (value < 0) {
		perror("Failed to read GPIO value");
		exit(1);
	}
	return (value == board->power_fail_active);
}

// Blocks until charge is above `block_pct` and power fail is cleared
void micro_scaps_block_pct(int i2cfd, board_t *board, int block_pct)
{
	uint8_t cur_pct;
	struct gpiod_chip *chip;
	struct gpiod_line *line;
	bool charge_ok, power_fail_clear;
	int counter = 0;

	assert(block_pct <= 100);

	micro_scaps_en(i2cfd, board, 1);
	chip = init_power_fail_gpio(board, &line, "micro_scaps_block_pct");

	while (true) {
		cur_pct = micro_scaps_remaining_pct(i2cfd, board);
		power_fail_clear = !read_power_fail_status(line, board);
		charge_ok = (cur_pct >= block_pct);

		// Print status once per second
		if (counter % 10 == 0) {
			printf("Supercap Charge: %d%% (Target: %d%%) | Power Fail: %s\n",
			       cur_pct, block_pct, power_fail_clear ? "No" : "YES");
			fflush(stdout);
		}

		if (charge_ok && power_fail_clear) {
			break;
		}

		usleep(1000 * 100);
		counter++;
	}

	gpiod_chip_close(chip);
}

// Monitors supercaps and triggers a reboot if charge is too low while power fails
void micro_scaps_monitor_daemon(int i2cfd, board_t *board, int reboot_pct)
{
	uint8_t cur_pct = 0;
	uint8_t status_flags;
	struct gpiod_chip *chip;
	struct gpiod_line *line;
	bool current_power_fail = false;
	bool power_fail_active = false;
	bool monitor_i2c = true;
	bool suppress_message = false;
	int counter = 0;
	int sleep_time = 100000; // Default sleep: 100ms
	int print_interval = 10; // Default print every 1s (10 x 100ms)

	openlog("tsmicroctl", LOG_PID | LOG_CONS, LOG_DAEMON);

	assert(reboot_pct <= 100);

	if (micro_read8(i2cfd, MICRO_STATUS_FLAGS, &status_flags) < 0) {
		syslog(LOG_ERR, "Failed to read status flags: %s", strerror(errno));
		exit(1);
	}

	if (board->has_silo == 0) {
		syslog(LOG_INFO, "Supercaps not present, exiting.");
		return;
	}

	if ((status_flags & MICRO_STATUS_FLAGS_SCAPS_EN) == 0) {
		syslog(LOG_INFO, "Supercaps not enabled, exiting and not monitoring charge");
		return;
	}

	chip = init_power_fail_gpio(board, &line, "micro_scaps_monitor_daemon");

	while (true) {
		current_power_fail = read_power_fail_status(line, board);

		if (current_power_fail && !power_fail_active) {
			monitor_i2c = true;
			power_fail_active = true;
			sleep_time = 100000; // Reset polling to 100ms
			print_interval = 10; // Print every 1s (10 x 100ms)
			suppress_message = false; // Allow prints again
			continue;
		}

		if (monitor_i2c) {
			cur_pct = micro_scaps_remaining_pct(i2cfd, board);
		}

		if ((power_fail_active || cur_pct < 100) && counter % print_interval == 0) {
			syslog(LOG_INFO, "Supercap Charge: %d%% (Reboot Threshold: %d%%) | Power Fail: %s",
			       cur_pct, reboot_pct, current_power_fail ? "YES" : "No");
		}

		if (power_fail_active && cur_pct < reboot_pct) {
			syslog(LOG_INFO, "Discharge percentage below threshold, rebooting...");
			system("/sbin/reboot");
		}

		if (!current_power_fail && power_fail_active) {
			syslog(LOG_INFO, "Power restored. Supercap Charge: %d%%", cur_pct);
			power_fail_active = false;

			if (cur_pct == 100) {
				monitor_i2c = false;
				sleep_time = 1000000; // Reduce polling to 1 second
				print_interval = 1;   // Print once per sleep cycle (1s)
				suppress_message = true;
			}
		}

		if (suppress_message && cur_pct == 100 && !power_fail_active) {
			usleep(sleep_time);
			continue;
		}

		usleep(sleep_time);
		counter++;
	}

	closelog();

	gpiod_chip_close(chip);
}
