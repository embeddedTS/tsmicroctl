#pragma once

#define MICRO_ADC_0 0
#define MICRO_ADC_1 2
#define MICRO_ADC_2 4
#define MICRO_ADC_3 6
#define MICRO_ADC_4 8
#define MICRO_ADC_5 10
#define MICRO_ADC_6 12
#define MICRO_ADC_7 14
#define MICRO_ADC_8 16
#define MICRO_ADC_9 18
#define MICRO_ADC_10 20
#define MICRO_STATUS_FLAGS 22
#define MICRO_STATUS_FLAGS_POWER_FAIL (1 << 0)
#define MICRO_STATUS_FLAGS_SCAPS_EN (1 << 1)
#define MICRO_STATUS_FLAGS_SCAPS_MET_MIN (1 << 2)
#define MICRO_STATUS_FLAGS_SCAPS_CHARGING (1 << 3)
#define MICRO_STATUS_FLAGS_USB_PRESENT (1 << 4)
#define MICRO_CHARGE_CURRENT_DEFAULT 24
#define MICRO_CHARGE_CURRENT 26
#define MICRO_CMD 1024
#define MICRO_CMD_SLEEP (1 << 1)
#define MICRO_REVISION 2048
#define MICRO_BUILD_STRING 4096

typedef struct board {
    const char *compatible;
    int i2c_bus;
    int i2c_chip;
    void (*info_function)(int i2cfd, struct board *board);
    const char *power_fail_bank;
    int power_fail_io;
    int power_fail_active;
    int has_silo;
    int max_current;
    int min_current;
} board_t;

int micro_init(int i2cbus, int i2caddr);
int micro_read(int i2cfd, uint16_t addr, void *data, size_t size);
int micro_write(int i2cfd, uint16_t addr, const void *data, size_t size);
uint16_t swap_endian16(uint16_t value);
uint32_t swap_endian32(uint32_t value);

#define micro_read8(fd, addr, data) micro_read(fd, addr, (uint8_t *)data, sizeof(uint8_t))
#define micro_write8(fd, addr, data) micro_write(fd, addr, (uint8_t *)data, sizeof(uint8_t))

#define micro_read16(fd, addr, data) micro_read(fd, addr, (uint16_t *)data, sizeof(uint16_t))
#define micro_write16(fd, addr, data) micro_write(fd, addr, (uint16_t *)data, sizeof(uint16_t))

#define micro_read32(fd, addr, data) micro_read(fd, addr, (uint32_t *)data, sizeof(uint32_t))
#define micro_write32(fd, addr, data) micro_write(fd, addr, (uint32_t *)data, sizeof(uint32_t))

int micro_read16_swap(int fd, int addr, uint16_t *data);
int micro_write16_swap(int fd, int addr, uint16_t *data);

int micro_read32_swap(int fd, int addr, uint32_t *data);
int micro_write32_swap(int fd, int addr, uint32_t *data);

uint8_t micro_scaps_remaining_pct(int i2cfd, board_t *board);
void micro_generic_info(int i2cfd, board_t *board);

void micro_sleep(int i2cfd, board_t *board, uint32_t ms);
void micro_set_charge_current(int i2cfd, board_t *board, uint16_t ma);
void micro_scaps_en(int i2cfd, board_t *board, int en);
void micro_scaps_block_pct(int i2cfd, board_t *board, int pct);
void micro_scaps_monitor_daemon(int i2cfd, board_t *board, int reboot_pct);
