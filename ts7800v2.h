#pragma once

void ts7800v2_info(int i2cfd, board_t *board);

const board_t ts7800v2_board = {
	.compatible = "technologic,ts7800v2",
	.i2c_bus = 0,
	.i2c_chip = 0x54,
	.info_function = ts7800v2_info,
	.has_silo = 0,
};
