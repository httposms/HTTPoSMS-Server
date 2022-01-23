#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>
#include <string.h>
#include "common.h"
#include "log.h"

static int _serial_init(char *);
static int _set_handle_attributes(int, int);
/* 
 * reading and writing functions to be used on init 
 * before we setup the event loop
 */
static int _at_sync_flush(int);
static int _at_sync_write(int, char*);
static int _at_sync_read(int, char*);
static void at_loop();
static int modem_read();
static int at_add_job();

static int ingress_fd, egress_fd, at_fd;

typedef enum {
	INIT,
	READY,
	SENDING, /* We are in a sending state, sending a bulk response of sms */
	READING, /* We have recieved a request and are going to pull that from modem storage */
	WAITING,
} at_state;

static char *at_states[] = {
	[INIT] = "Initialising",
	[READY] = "Ready",
	[SENDING] = "Sending",
	[READING] = "Reading",
	[WAITING] = "Waiting"
};

struct sms_modem {
	at_state state;
};

int at_init(int response_fd, int request_fd, char *dev)
{
	/*
	 * Take things from the net component on ingress_fd 
	 * and send request urls to the egress_fd
	 */
	ingress_fd = response_fd;
	egress_fd = request_fd;
	at_fd = _serial_init(dev);
	at_loop();
	return 0;
}

static void at_loop()
{
	while(1) {
		int retval;
		int numfds = (at_fd > ingress_fd) ?  at_fd + 1 : ingress_fd + 1;
		fd_set readers;	
		FD_ZERO(&readers);
		FD_SET(ingress_fd, &readers);
		FD_SET(at_fd, &readers);

		retval = select(numfds, &readers, NULL, NULL, NULL);
		if(retval < 0) {
			LOG_WARN("Select returned < 0, will retry\n");
			continue;
		}

		/* Check requests on pipe or status from modem */
		if(FD_ISSET(at_fd, &readers)) {
			LOG_DBUG("Have data on modem handle\n");
			modem_read();
		}
		if(FD_ISSET(ingress_fd, &readers)) {
			LOG_DBUG("Have data on request handle\n");
			at_add_job();
		}

	
	
	}

}
static int modem_read() 
{
	/* Bad, but this works */
	char buff[4096];
	int retval;
	retval = read(at_fd, buff, sizeof(buff));
	for(int i = 0; i < retval; i++)
		printf("%c : %d\n", buff[i], buff[i]);
	return 0;
}

static int at_add_job()
{
	/* TODO: actually add the job from the modem */
	void *ptr;
	read(ingress_fd, &ptr, sizeof(ptr));
	return 0;
}

static int _serial_init(char *dev)
{
	int fd;

	/* Not a controlling tty, and synchronise writes so we can represent state*/
	fd = open(dev, O_RDWR | O_NOCTTY | O_SYNC );
	if(fd < 0){
		LOG_FAIL("Could not initialise modem\n");
		return -1;
	}
	LOG_DBUG("Created modem handle\n");
	LOG_DBUG("Attempt to set modem attribs\n");
	_set_handle_attributes(fd, B115200);
	LOG_DBUG("Attempt to flush modem to known state\n");
	_at_sync_flush(fd);
	_at_sync_write(fd, "AT\r");
	sleep(1);
	if(_at_sync_read(fd, "OK"))
		LOG_WARN("Missing OK in response\n");
	_at_sync_write(fd, "ATE0\r");
	sleep(1);
	if(_at_sync_read(fd, "OK"))
		LOG_WARN("Missing OK in response\n");


	LOG_DBUG("Flushed modem to known state\n");
	return fd;
}

static int _set_handle_attributes(int fd, int baud){
	struct termios attrs;
	if(tcgetattr(fd, &attrs) < 0) {
		LOG_FAIL("Could not get handle attributes\n");
		return 1;
	}
	cfsetispeed(&attrs, (speed_t)baud);
	cfsetospeed(&attrs, (speed_t)baud);

	attrs.c_cflag = (attrs.c_cflag & ~CSIZE) | CS8;
	attrs.c_lflag = 0;                      // no signaling chars, no echo,
	attrs.c_oflag = 0;                      // no remapping, no delays
	attrs.c_cc[VMIN]  = 0;                  // read doesn't block
	attrs.c_cc[VTIME] = 5;                  // 0.5 seconds read timeout
	attrs.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl
	attrs.c_cflag |= (CLOCAL | CREAD);      // ignore modem controls,
	attrs.c_cflag &= ~(PARENB | PARODD);    // shut off parity
	attrs.c_cflag &= ~CSTOPB;
	attrs.c_lflag &= ~ICANON;
	attrs.c_lflag &= ~ECHO;                 // Disable echo
	attrs.c_lflag &= ~ECHOE;                // Disable erasure
	attrs.c_lflag &= ~ECHONL;               // Disable new-line echo
	attrs.c_lflag &= ~ISIG;
	attrs.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL);
	attrs.c_oflag &= ~OPOST; // Prevent special interpretation of output bytes (e.g. newline chars)
	attrs.c_oflag &= ~ONLCR; // Prevent conversion of newline to carriage return/line feed

	if(tcsetattr(fd, TCSANOW, &attrs) < 0) {
		LOG_FAIL("Could not set handle attributes\n");
		return 1;
	}

	LOG_INFO("Set handle attributes\n");
	return 0;
}

static int _at_sync_flush(int fd)
{
	/* 
	 * Attempt to get device into some known state
	 * Check to see if the device was suspended during sending an SMS
	 */
	if(fd < 0)
		return 1;

	/* TODO: handle writes of less than this */
	const char *msg = "\r\n";
	char buff[1];
	write(fd, msg, strlen(msg));
	while(read(fd, buff, sizeof(buff)) > 0);
	return 0;
}

static int _at_sync_write(int fd, char* buffer)
{
	if(!buffer || fd < 0)
		return -1;

	/* TODO: handle < strlen writes */
	size_t len = write(fd, buffer, strlen(buffer));
	LOG_DBUG("Wrote buffer to device, len %d\n", len);
	return 0;
}
/* Read the at modem and check that the expected string is contained within */
static int _at_sync_read(int fd, char *expected)
{
	size_t len;
	if(!expected || fd < 0)
		return -1;

	len = strlen(expected);
	char buffer[128];
	/* TODO: handle < strlen reads */
	int sz = read(fd, buffer, sizeof(buffer));
	buffer[sz] = '\0';
	return strstr(buffer, expected) ? 0 : 1;
}
