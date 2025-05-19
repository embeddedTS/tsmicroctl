#include <errno.h>
#include <error.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <assert.h>
#include <getopt.h>

#include "micro.h"
#include "ts7100.h"
#include "ts7180.h"
#include "ts7800v2.h"

/* Currently, all supported platforms, luckily, have the microcontroller on
 * I2C bus 0, and are at chip address 0x54. Because of that, we can still
 * get limited communication to the microcontroller when the exact model of the
 * current platform cannot be determined. This allows us, for example, to issue
 * a sleep command even if /sys is not mounted.
 */
board_t generic_board = {
	.compatible = "",
	.i2c_bus = 0,
	.i2c_chip = 0x54,
	.info_function = NULL,
	.has_silo = 0,
	.power_fail_active = 0,
	.power_fail_bank = "",
	.power_fail_io = 0,
	.max_current = 0,
	.min_current = 0,
};

void usage(char **argv, board_t *board)
{
	fprintf(stderr,
		"Usage: %s [OPTION] ...\n"
		"embeddedTS microcontroller utility\n"
		"\n"
		"  -e, --enable             Enable charging\n"
		"  -d, --disable            Disables any further charging\n"
		"  -w, --wait-pct <percent> Enable charging and block until charged to a set percent\n"
		"  -b, --daemon <percent>   Monitor power_fail# and issue \"reboot\" if the supercaps fall below percent\n"
		"  -i, --info               Print current information about supercaps\n"
		"  -c, --current <mA>       Permanently set max charging mA (default: 100, min: %d, max: %d)\n"
		"  -s, --sleep <seconds>    Turns off power to everything for a specified number of seconds\n"
		"  -h, --help               This message\n"
		"\n",
		argv[0],
		board->min_current,
		board->max_current);
}

board_t boards[] = {
	ts7100_board,
	ts7180_board,
	ts7800v2_board,
};

board_t *get_board()
{
	FILE *file;
	char comp[256];

	file = fopen("/sys/firmware/devicetree/base/compatible", "r");
	if (!file) {
		perror("Unable to open /sys/firmware/devicetree/base/compatible");
		/* NOTE WELL!
		 * When attempting to issue a sleep command at the last possible
		 * moment on a booted system, it is completely valid to have /sys
		 * NOT be mounted when we're called. Because of this, this will
		 * result in errno being ENOENT. In this case, we're going to just
		 * guess that we still want to run.
		 */
		if (errno == ENOENT) {
			fprintf(stderr, "\nPlatform could not be detected!\n\n" \
					"This is likely because /sys is not properly " \
					"mounted. The only command that will be " \
					"accepted in this situation is -s/--sleep." \
					"\n\n");
			return &generic_board;
		} else {
			return NULL;
		}
	}

	if (fgets(comp, sizeof(comp), file) == NULL) {
		perror("Failed to read compatible string");
		fclose(file);
		return NULL;
	}
	fclose(file);

	for (int i = 0; i < sizeof(boards) / sizeof(boards[0]); i++) {
		if (strstr(comp, boards[i].compatible) != NULL) {
			return &boards[i];
		}
	}
	return NULL;
}

int main(int argc, char *argv[])
{
	board_t *board;
	int option_index = 0;
	int c;
	int i2cfd;

	int opt_enable = 0;
	int opt_disable = 0;
	int opt_wait_pct = -1;
	int opt_daemon_pct = -1;
	int opt_info = 0;
	int opt_current = -1;
	int opt_sleep = -1;
	int opt_nonsleep_opt = 0;

	board = get_board();
	if (board == NULL) {
		printf("Unsupported platform\n");
		return 1;
	}

	if (argc < 2) {
		usage(argv, board);
		return 1;
	}

	static struct option long_options[] = { { "enable", no_argument, NULL, 'e' },
						{ "disable", no_argument, NULL, 'd' },
						{ "wait-pct", required_argument, NULL, 'w' },
						{ "daemon", required_argument, NULL, 'b' },
						{ "info", no_argument, NULL, 'i' },
						{ "current", required_argument, NULL, 'c' },
						{ "sleep", required_argument, NULL, 's' },
						{ "help", no_argument, NULL, 'h' },
						{ 0, 0, 0, 0 } };

	while ((c = getopt_long(argc, argv, "edw:b:ic:s:h", long_options, &option_index)) != -1) {
		switch (c) {
		case 'e':
			opt_enable = 1;
			opt_nonsleep_opt = 1;
			break;
		case 'd':
			opt_disable = 1;
			opt_nonsleep_opt = 1;
			break;
		case 'w':
			opt_wait_pct = atoi(optarg);
			opt_nonsleep_opt = 1;
			break;
		case 'b':
			opt_daemon_pct = atoi(optarg);
			opt_nonsleep_opt = 1;
			break;
		case 'i':
			opt_info = 1;
			opt_nonsleep_opt = 1;
			break;
		case 'c':
			opt_current = atoi(optarg);
			opt_nonsleep_opt = 1;
			break;
		case 's':
			opt_sleep = atoi(optarg);
			break;
		case 'h':
			usage(argv, board);
			return 0;
		case '?':
		default:
			fprintf(stderr, "Unexpected argument \"%s\"\n", optarg);
			usage(argv, board);
			return 1;
		}
	}

	/* If we had to fall back to the generic_board struct, we need to only
	 * allow opt_sleep to be processed. Any other flags/options are not
	 * guaranteed to correctly run in this case.
	 */
	if (board == &generic_board && opt_nonsleep_opt) {
		fprintf(stderr, "Only -s/--sleep is allowed to be issued when " \
				"the platform is not able to correctly be recognized " \
				"due to /sys not being available or not able to be " \
				"opened.\n");
		return 1;
	}

	i2cfd = micro_init(board->i2c_bus, board->i2c_chip);

	if (opt_enable) {
		micro_scaps_en(i2cfd, board, 1);
	}
	if (opt_disable) {
		micro_scaps_en(i2cfd, board, 0);
	}
	if (opt_wait_pct != -1) {
		micro_scaps_block_pct(i2cfd, board, opt_wait_pct);
	}
	if (opt_daemon_pct != -1) {
		micro_scaps_monitor_daemon(i2cfd, board, opt_daemon_pct);
	}
	if (opt_info) {
		board->info_function(i2cfd, board);
	}
	if (opt_current != -1) {
		if (opt_current < board->min_current || opt_current > board->max_current) {
			fprintf(stderr, "Current must be between %d mA and %d mA\n",
				board->min_current,
				board->max_current);
			return 1;
		}

		// Set max charging mA
		micro_set_charge_current(i2cfd, board, (uint16_t)opt_current);
	}
	if (opt_sleep != -1) {
		micro_sleep(i2cfd, board, opt_sleep);
	}

	return 0;
}
