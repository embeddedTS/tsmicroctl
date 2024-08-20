#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>

#include "micro.h"

void ts7100_info(int i2cfd, board_t *board)
{
	uint16_t mv;

	micro_generic_info(i2cfd, board);

	/* 5V_A */
	if (micro_read16_swap(i2cfd, MICRO_ADC_0, &mv) < 0) {
		perror("Failed to read microcontroller version");
		exit(1);
	}
	printf("adc_5v_a_mv=%d\n", mv);

	/* AN_SUP_CHRG */
	if (micro_read16_swap(i2cfd, MICRO_ADC_1, &mv) < 0) {
		perror("Failed to read microcontroller version");
		exit(1);
	}
	printf("an_sup_chrg=%d\n", mv);

	/* 3.3V */
	if (micro_read16_swap(i2cfd, MICRO_ADC_2, (uint16_t *)&mv) < 0) {
		perror("Failed to read microcontroller version");
		exit(1);
	}
	printf("adc_3p3v_mv=%d\n", mv);

	/* 8V_48V */
	if (micro_read16_swap(i2cfd, MICRO_ADC_3, (uint16_t *)&mv) < 0) {
		perror("Failed to read microcontroller version");
		exit(1);
	}
	printf("adc_8v_48v_mv=%d\n", mv);

	/* AN_SUP_CAP_1 */
	if (micro_read16_swap(i2cfd, MICRO_ADC_7, (uint16_t *)&mv) < 0) {
		perror("Failed to read microcontroller version");
		exit(1);
	}
	printf("adc_an_sup_cap_1_mv=%d\n", mv);

	/* AN_SUP_CAP_2 */
	if (micro_read16_swap(i2cfd, MICRO_ADC_8, (uint16_t *)&mv) < 0) {
		perror("Failed to read microcontroller version");
		exit(1);
	}
	printf("adc_an_sup_cap_2_mv=%d\n", mv);
}
