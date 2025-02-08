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

void usage(char **argv)
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
		"  -c, --current <mA>       Permanently set max charging mA (default 100mA, max 1000)\n"
		"  -s, --sleep <seconds>    Turns off power to everything for a specified number of seconds\n"
		"  -h, --help               This message\n"
		"\n",
		argv[0]);
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
		return NULL;
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

	if (argc < 2) {
		usage(argv);
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
			break;
		case 'd':
			opt_disable = 1;
			break;
		case 'w':
			opt_wait_pct = atoi(optarg);
			break;
		case 'b':
			opt_daemon_pct = atoi(optarg);
			break;
		case 'i':
			opt_info = 1;
			break;
		case 'c':
			opt_current = atoi(optarg);
			break;
		case 's':
			opt_sleep = atoi(optarg);
			break;
		case 'h':
			usage(argv);
			return 0;
		case '?':
		default:
			fprintf(stderr, "Unexpected argument \"%s\"\n", optarg);
			usage(argv);
			return 1;
		}
	}

	board = get_board();
	if (!board) {
		printf("Unsupported board\n");
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
		if (opt_current < 50 || opt_current > 950) {
			fprintf(stderr, "Current must be between 50mA and 950mA\n");
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
