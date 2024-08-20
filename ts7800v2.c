#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>

#include "micro.h"

void ts7800v2_info(int i2cfd, board_t *board)
{
	uint16_t mv;

	micro_generic_info(i2cfd, board);
}
