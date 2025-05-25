#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <signal.h>
#include <time.h>

#include <hidapi/hidapi.h>

int running = 1;
int last_sent = 0;

enum {
	VENDOR_ID = 0x03f0,
	PRODUCT_ID = 0x098d,
};

enum {
	STATUS_REQUEST = 0x21bb0b00,
	PING = 0x21bb0c00,
	GET_INDENTIFIER = 0x21bb0d00,
	CONNECTION_STATE = 0x21bb0300,
	MICROPHONE_STATE = 0x21bb0500,
	VOICE_STATE = 0x21bb0900,
	VOICE_PROMPTS = 0x21bb1301,
	VOICE_PROMPTS_OFF = 0x21bb1300,
	MIC_MONITOR_STATE = 0x21bb0a00,
	MICROPHONE_MONITOR = 0x21bb1001,
	MICROPHONE_MONITOR_OFF = 0x21bb1000,
	SLEEP_STATE = 0x21bb0700,
	SLEEP_TIMER_10 = 0x21bb120a,
	SLEEP_TIMER_20 = 0x21bb1214,
	SLEEP_TIMER_30 = 0x21bb121e,
};

int read_bytes(hid_device *device, unsigned char *bytes) {
	return hid_read_timeout(device, bytes, 31, 60000) != 31;
}

int read_headset(hid_device *device) {
	unsigned char bytes[32] = { 0 };
	if (read_bytes(device, bytes))
		return 1;
	if (bytes[0] == 0x21 && bytes[1] == 0xbb) {
		switch (bytes[2]) {
		/* read connection state */
		case 0x3:
			if (bytes[3] == 0x1) {
				printf("{\"text\": \"....\", \"alt\": \"offline\"}\n");
				return 1;
			}
			break;
		/* ??? */
		case 0x5:
		/* read sleep state */
		case 0x7:
		/* read voice prompts */
		case 0x9:
		/* ??? */
		case 0xa:
			break;
		/* read battery */
		case 0xb:
			printf("{\"text\": \"%3d%%\", \"alt\": \"online\"}\n", bytes[3]);
			return 1;
		/* ping? */
		case 0xc:

		case 0xd:
		/* read sleep timer */
		case 0x12:
		/* read voice prompts */
		case 0x13:
			break;
		/* power */
		case 0x24:
			last_sent = 0;
			break;
		default:
			return 2;
		}
	}
	return 0;
}

int send_headset(hid_device *device, uint32_t command) {
	unsigned char bytes[4];
	bytes[0] = command >> 24;
	bytes[1] = (command >> 16) & 0xff;
	bytes[2] = (command >> 8) & 0xff;
	bytes[3] = command & 0xff;

	return hid_write(device, bytes, 4) != 4;
}

void sigint_handler(int signum) {
	(void)signum;
	running = 0;
	signal(signum, &sigint_handler);
}

int main(void) {
	int error = 0;
	hid_device *device = NULL;

	signal(SIGINT, &sigint_handler);

	if (hid_init() < 0) {
		printf("error: hid_init\n");
		return 1;
	}

	if (!(device = hid_open(VENDOR_ID, PRODUCT_ID, NULL))) {
		printf("error: hid_open\n");
		error = 3;
		goto exitearly;
	}



	while (running) {
		if (time(NULL) - last_sent >= 58) {
			last_sent = time(NULL);
			if (send_headset(device, CONNECTION_STATE)) {
				printf("error: send_headset\n");
				error = 4;
				goto exitearly;
			}
			if (send_headset(device, STATUS_REQUEST)) {
				printf("error: send_headset\n");
				error = 5;
				goto exitearly;
			}
		}
		if (read_headset(device) == 2) {
			goto exitearly;
		}
		fflush(stdout);
	}

exitearly:;
	if (device)
		hid_close(device);
	if (hid_exit() < 0) {
		printf("error: hid_exit\n");
		return 2;
	}

	return error;
}
