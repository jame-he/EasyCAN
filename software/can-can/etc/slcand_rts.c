/*
 * slcand.c - userspace daemon for serial line CAN interface driver SLCAN
 *
 * Copyright (c) 2009 Robert Haddon <robert.haddon@verari.com>
 * Copyright (c) 2009 Verari Systems Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Send feedback to <linux-can@vger.kernel.org>
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <syslog.h>
#include <errno.h>
#include <pwd.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <termios.h>

/* default slcan line discipline since Kernel 2.6.25 */
#define LDISC_N_SLCAN 17

/* Change this to whatever your daemon is called */
#define DAEMON_NAME "slcand"

/* Change this to the user under which to run */
#define RUN_AS_USER "root"

/* The length of ttypath buffer */
#define TTYPATH_LENGTH	64

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

/* UART flow control types */
#define FLOW_NONE 0
#define FLOW_HW 1
#define FLOW_SW 2

void print_usage(char *prg)
{
	fprintf(stderr, "\nUsage: %s [options] <tty> [canif-name]\n\n", prg);
	fprintf(stderr, "Options: -o         (send open command 'O\\r')\n");
	fprintf(stderr, "         -c         (send close command 'C\\r')\n");
	fprintf(stderr, "         -f         (read status flags with 'F\\r' to reset error states)\n");
	fprintf(stderr, "         -s <speed> (set CAN speed 0..8)\n");
	fprintf(stderr, "         -S <speed> (set UART speed in baud)\n");
	fprintf(stderr, "         -t <type>  (set UART flow control type 'hw' or 'sw')\n");
	fprintf(stderr, "         -b <btr>   (set bit time register value)\n");
	fprintf(stderr, "         -F         (stay in foreground; no daemonize)\n");
	fprintf(stderr, "         -h         (show this help page)\n");
	fprintf(stderr, "\nExamples:\n");
	fprintf(stderr, "slcand -o -c -f -s6 ttyslcan0\n");
	fprintf(stderr, "slcand -o -c -f -s6 ttyslcan0 can0\n");
	fprintf(stderr, "\n");
	exit(EXIT_FAILURE);
}

static int slcand_running;
static int exit_code;
static char ttypath[TTYPATH_LENGTH];

static void child_handler(int signum)
{
	switch (signum) {

	case SIGUSR1:
		/* exit parent */
		exit(EXIT_SUCCESS);
		break;
	case SIGALRM:
	case SIGCHLD:
		syslog(LOG_NOTICE, "received signal %i on %s", signum, ttypath);
		exit_code = EXIT_FAILURE;
		slcand_running = 0;
		break;
	case SIGINT:
	case SIGTERM:
		syslog(LOG_NOTICE, "received signal %i on %s", signum, ttypath);
		exit_code = EXIT_SUCCESS;
		slcand_running = 0;
		break;
	}
}

static int look_up_uart_speed(long int s)
{
	switch (s) {

	case 9600:
		return B9600;
	case 19200:
		return B19200;
	case 38400:
		return B38400;
	case 57600:
		return B57600;
	case 115200:
		return B115200;
	case 230400:
		return B230400;
	case 460800:
		return B460800;
	case 500000:
		return B500000;
	case 576000:
		return B576000;
	case 921600:
		return B921600;
	case 1000000:
		return B1000000;
	case 1152000:
		return B1152000;
	case 1500000:
		return B1500000;
	case 2000000:
		return B2000000;
#ifdef B2500000
	case 2500000:
		return B2500000;
#endif
#ifdef B3000000
	case 3000000:
		return B3000000;
#endif
#ifdef B3500000
	case 3500000:
		return B3500000;
#endif
#ifdef B3710000
	case 3710000
		return B3710000;
#endif
#ifdef B4000000
	case 4000000:
		return B4000000;
#endif
	default:
		return -1;
	}
}

int main(int argc, char *argv[])
{
	char *tty = NULL;
	char const *devprefix = "/dev/";
	char *name = NULL;
	char buf[IFNAMSIZ+1];
	struct termios tios;
	speed_t old_ispeed;
	speed_t old_ospeed;

	int opt;
	int send_open = 0;
	int send_close = 0;
	int send_read_status_flags = 0;
	char *speed = NULL;
	char *uart_speed_str = NULL;
	long int uart_speed = 0;
	int flow_type = FLOW_NONE;
	char *btr = NULL;
	int run_as_daemon = 1;
	char *pch;
	int ldisc = LDISC_N_SLCAN;
	int fd;
	int status, arg;

	ttypath[0] = '\0';

	while ((opt = getopt(argc, argv, "ocfs:S:t:b:?hF")) != -1) {
		switch (opt) {
		case 'o':
			send_open = 1;
			break;
		case 'c':
			send_close = 1;
			break;
		case 'f':
			send_read_status_flags = 1;
			break;
		case 's':
			speed = optarg;
			if (strlen(speed) > 1)
				print_usage(argv[0]);
			break;
		case 'S':
			uart_speed_str = optarg;
			errno = 0;
			uart_speed = strtol(uart_speed_str, NULL, 10);
			if (errno)
				print_usage(argv[0]);
			if (look_up_uart_speed(uart_speed) == -1) {
				fprintf(stderr, "Unsupported UART speed (%lu)\n", uart_speed);
				exit(EXIT_FAILURE);
			}
			break;
		case 't':
			if (!strcmp(optarg, "hw")) {
				flow_type = FLOW_HW;
			} else if (!strcmp(optarg, "sw")) {
				flow_type = FLOW_SW;
			} else {
				fprintf(stderr, "Unsupported flow type (%s)\n", optarg);
				exit(EXIT_FAILURE);
			}
			break;
		case 'b':
			btr = optarg;
			if (strlen(btr) > 6)
				print_usage(argv[0]);
			break;
		case 'F':
			run_as_daemon = 0;
			break;
		case 'h':
		case '?':
		default:
			print_usage(argv[0]);
			break;
		}
	}

	/* Initialize the logging interface */
	openlog(DAEMON_NAME, LOG_PID, LOG_LOCAL5);

	/* Parse serial device name and optional can interface name */
	tty = argv[optind];
	if (NULL == tty)
		print_usage(argv[0]);

	name = argv[optind + 1];

	/* Prepare the tty device name string */
	pch = strstr(tty, devprefix);
	if (pch != tty)
		snprintf(ttypath, TTYPATH_LENGTH, "%s%s", devprefix, tty);
	else
		snprintf(ttypath, TTYPATH_LENGTH, "%s", tty);

	syslog(LOG_INFO, "starting on TTY device %s", ttypath);

	/* Daemonize */
	if (run_as_daemon) {
		if (daemon(0, 0)) {
			syslog(LOG_ERR, "failed to daemonize");
			exit(EXIT_FAILURE);
		}
	}
	else {
		/* Trap signals that we expect to receive */
		signal(SIGINT, child_handler);
		signal(SIGTERM, child_handler);
	}

	/* */
	slcand_running = 1;

	/* Now we are a daemon -- do the work for which we were paid */
	fd = open(ttypath, O_RDWR | O_NONBLOCK | O_NOCTTY);
	if (fd < 0) {
		syslog(LOG_NOTICE, "failed to open TTY device %s\n", ttypath);
		perror(ttypath);
		exit(EXIT_FAILURE);
	}

	/* Configure baud rate */
	memset(&tios, 0, sizeof(struct termios));
	if (tcgetattr(fd, &tios) < 0) {
		syslog(LOG_NOTICE, "failed to get attributes for TTY device %s: %s\n", ttypath, strerror(errno));
		exit(EXIT_FAILURE);
	}

	/* Get old values for later restore */
	old_ispeed = cfgetispeed(&tios);
	old_ospeed = cfgetospeed(&tios);

	/* Reset UART settings */
	cfmakeraw(&tios);
	tios.c_iflag &= ~IXOFF;
	tios.c_cflag &= ~CRTSCTS;

	/* Baud Rate */
	cfsetispeed(&tios, look_up_uart_speed(uart_speed));
	cfsetospeed(&tios, look_up_uart_speed(uart_speed));

	/* Flow control */
	if (flow_type == FLOW_HW)
		tios.c_cflag |= CRTSCTS;
	else if (flow_type == FLOW_SW)
		tios.c_iflag |= (IXON | IXOFF);

	/* apply changes */
	if (tcsetattr(fd, TCSADRAIN, &tios) < 0)
		syslog(LOG_NOTICE, "Cannot set attributes for device \"%s\": %s!\n", ttypath, strerror(errno));

	if (speed) {
		sprintf(buf, "C\rS%s\r", speed);
		write(fd, buf, strlen(buf));
	}

	if (btr) {
		sprintf(buf, "C\rs%s\r", btr);
		write(fd, buf, strlen(buf));
	}

	if (send_read_status_flags) {
		sprintf(buf, "F\r");
		write(fd, buf, strlen(buf));
	}

	if (send_open) {
		sprintf(buf, "O\r");
		write(fd, buf, strlen(buf));
	}

	/* set slcan like discipline on given tty */
	if (ioctl(fd, TIOCSETD, &ldisc) < 0) {
		perror("ioctl TIOCSETD");
		exit(1);
	}
	
	/* retrieve the name of the created CAN netdevice */
	if (ioctl(fd, SIOCGIFNAME, buf) < 0) {
		perror("ioctl SIOCGIFNAME");
		exit(1);
	}

	status = ioctl(fd, TIOCMGET, &arg);
	arg &= ~TIOCM_RTS;
	ioctl(fd, TIOCMSET, &arg);

	printf("waiting for bootloader ...\n");
	sleep(1);

	syslog(LOG_NOTICE, "attached TTY %s to netdevice %s\n", ttypath, buf);
	
	/* try to rename the created netdevice */
	if (name) {
		struct ifreq ifr;
		int s = socket(PF_INET, SOCK_DGRAM, 0);

		if (s < 0)
			perror("socket for interface rename");
		else {
			strncpy(ifr.ifr_name, buf, IFNAMSIZ);
			strncpy(ifr.ifr_newname, name, IFNAMSIZ);

			if (ioctl(s, SIOCSIFNAME, &ifr) < 0) {
				syslog(LOG_NOTICE, "netdevice %s rename to %s failed\n", buf, name);
				perror("ioctl SIOCSIFNAME rename");
				exit(1);
			} else
				syslog(LOG_NOTICE, "netdevice %s renamed to %s\n", buf, name);

			close(s);
		}	
	}

	/* The Big Loop */
	while (slcand_running)
		sleep(1); /* wait 1 second */

	/* Reset line discipline */
	syslog(LOG_INFO, "stopping on TTY device %s", ttypath);
	ldisc = N_TTY;
	if (ioctl(fd, TIOCSETD, &ldisc) < 0) {
		perror("ioctl TIOCSETD");
		exit(EXIT_FAILURE);
	}

	if (send_close) {
		sprintf(buf, "C\r");
		write(fd, buf, strlen(buf));
	}

	/* Reset old rates */
	cfsetispeed(&tios, old_ispeed);
	cfsetospeed(&tios, old_ospeed);

	/* apply changes */
	if (tcsetattr(fd, TCSADRAIN, &tios) < 0)
		syslog(LOG_NOTICE, "Cannot set attributes for device \"%s\": %s!\n", ttypath, strerror(errno));

	/* Finish up */
	syslog(LOG_NOTICE, "terminated on %s", ttypath);
	closelog();
	return exit_code;
}