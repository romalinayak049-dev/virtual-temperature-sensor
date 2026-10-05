/*
 * monitor.c - User-space client for the virtual temperature sensor.
 * Talks to /dev/vtempsensor using open(), read(), write(), close().
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#ifndef DEV_PATH
#define DEV_PATH "/dev/vtempsensor"
#endif
#define STEP 5   /* degrees C per increase/decrease */

struct sensor {
	int temp;
	int threshold;
	char state[16];
};

/* Read and parse the sensor. Returns 0 on success, -1 on failure. */
static int read_sensor(struct sensor *s)
{
	char buf[128];
	ssize_t n;
	int fd = open(DEV_PATH, O_RDONLY);

	if (fd < 0)
		return -1;
	n = read(fd, buf, sizeof(buf) - 1);
	close(fd);
	if (n <= 0)
		return -1;
	buf[n] = '\0';

	if (sscanf(buf, "temperature=%d C threshold=%d C state=%15s",
		   &s->temp, &s->threshold, s->state) != 3)
		return -1;
	return 0;
}

/* Send a command string to the driver. Returns 0 on success, -1 on failure. */
static int write_sensor(const char *cmd)
{
	int fd = open(DEV_PATH, O_WRONLY);
	ssize_t n;

	if (fd < 0)
		return -1;
	n = write(fd, cmd, strlen(cmd));
	close(fd);
	return (n == (ssize_t)strlen(cmd)) ? 0 : -1;
}

static void set_value(const char *prefix, int value)
{
	char cmd[32];

	snprintf(cmd, sizeof(cmd), "%s%d", prefix, value);
	if (write_sensor(cmd) < 0)
		printf("Error: driver rejected the value (valid range -50..150 C).\n");
}

static int ask_int(const char *prompt, int *out)
{
	char line[32];
	char *end;
	long v;

	printf("%s", prompt);
	if (!fgets(line, sizeof(line), stdin))
		return -1;
	v = strtol(line, &end, 10);
	if (end == line || (*end != '\n' && *end != '\0'))
		return -1;
	*out = (int)v;
	return 0;
}

static void show_screen(const struct sensor *s)
{
	printf("\n========================================\n");
	printf("       VIRTUAL TEMPERATURE SENSOR\n");
	printf("========================================\n\n");
	printf("Device     : %s\n", DEV_PATH);
	printf("Driver     : vtempsensor\n");
	printf("Status     : ACTIVE\n");
	printf("Temperature: %d C\n", s->temp);
	printf("Threshold  : %d C\n", s->threshold);
	printf("State      : %s\n\n", s->state);
	printf("----------------------------------------\n");
	printf("1. Read Temperature\n2. Increase Temperature\n");
	printf("3. Decrease Temperature\n4. Set Threshold\n");
	printf("5. Show Status\n6. Exit\n");
	printf("----------------------------------------\n");
}

int main(void)
{
	struct sensor s;
	int choice, v;

	for (;;) {
		if (read_sensor(&s) < 0) {
			fprintf(stderr, "Error: Unable to read %s (%s)\n"
				"Make sure the kernel module is loaded.\n",
				DEV_PATH, strerror(errno));
			return 1;
		}
		show_screen(&s);
		if (ask_int("Enter choice: ", &choice) < 0) {
			if (feof(stdin))
				break;
			printf("Invalid choice.\n");
			continue;
		}

		switch (choice) {
		case 1:
			printf("Temperature: %d C\n", s.temp);
			break;
		case 2:
			set_value("", s.temp + STEP);
			break;
		case 3:
			set_value("", s.temp - STEP);
			break;
		case 4:
			if (ask_int("New threshold (C): ", &v) < 0)
				printf("Invalid number.\n");
			else
				set_value("threshold=", v);
			break;
		case 5:
			printf("Temperature: %d C | Threshold: %d C | State: %s\n",
			       s.temp, s.threshold, s.state);
			break;
		case 6:
			return 0;
		default:
			printf("Invalid choice.\n");
		}
	}
	return 0;
}
