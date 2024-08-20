#pragma once

void ts7180_info(int i2cfd, board_t *board);

const board_t ts7180_board = {
	.compatible = "technologic,ts7180",
	.i2c_bus = 0,
	.i2c_chip = 0x54,
	.info_function = ts7180_info,
	.has_silo = 1,
	.power_fail_active = 1,
	.power_fail_bank = "20ac000.gpio",
	.power_fail_io = 0,
};
