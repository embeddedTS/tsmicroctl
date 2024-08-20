#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>

#include "micro.h"

void ts7180_info(int i2cfd, board_t *board)
{
	uint16_t mv;

	micro_generic_info(i2cfd, board);

	/* 5V_A */
	if (micro_read16_swap(i2cfd, MICRO_ADC_0, &mv) < 0) {
		perror("Failed to read microcontroller version");
		exit(1);
	}
	/* Simplified (2500/1023) * ((53600 + 42200)/42200) */
	mv = (uint16_t)((uint32_t)mv * 1197500 / 215853);
	printf("adc_5v_a_mv=%d\n", mv);

	/* AN_CHRG */
	if (micro_read16_swap(i2cfd, MICRO_ADC_1, &mv) < 0) {
		perror("Failed to read microcontroller version");
		exit(1);
	}
	/* Simplified (2500/1023) * ((20000 + 14700)/14700) */
	mv = (uint16_t)((uint32_t)mv * 867500 / 150381);
	printf("adc_an_chrg_mv=%d\n", mv);

	/* 3.3V */
	if (micro_read16_swap(i2cfd, MICRO_ADC_2, (uint16_t *)&mv) < 0) {
		perror("Failed to read microcontroller version");
		exit(1);
	}
	/* Simplified (2500/1023) * ((42200 + 42200)/42200) */
	mv = (uint16_t)((uint32_t)mv * 5000 / 1023);
	printf("adc_3p3v_mv=%d\n", mv);

	/* VIN */
	if (micro_read16_swap(i2cfd, MICRO_ADC_3, (uint16_t *)&mv) < 0) {
		perror("Failed to read microcontroller version");
		exit(1);
	}
	/* Simplified (2500/1023) * ((191000 + 10700)/10700) */
	mv = (uint16_t)((uint64_t)mv * 5042500 / 109461); // Needs 34 bits to multiply
	printf("adc_vin_mv=%d\n", mv);

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
