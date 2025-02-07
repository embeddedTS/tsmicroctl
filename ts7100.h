#pragma once

void ts7100_info(int i2cfd, board_t *board);

const board_t ts7100_board = {
	.compatible = "technologic,ts7100",
	.i2c_bus = 0,
	.i2c_chip = 0x54,
	.info_function = ts7100_info,
	.has_silo = 1,
	.power_fail_active = 1,
	.power_fail_bank = "20ac000.gpio",
	.power_fail_io = 0,
	.max_current = 950,
};
