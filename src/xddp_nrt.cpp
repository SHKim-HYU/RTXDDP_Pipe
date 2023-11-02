#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <malloc.h>
#include <pthread.h>
#include <fcntl.h>
#include <errno.h>

pthread_t nrt;

#define XDDP_PORT_LABEL  "xddp"

static const char *msg[] = {
	"Surfing With The Alien",
	"Lords of Karma",
	"Banana Mango",
	"Psycho Monkey",
	"Luminous Flesh Giants",
	"Moroccan Sunset",
	"Satch Boogie",
	"Flying In A Blue Dream",
	"Ride",
	"Summer Song",
	"Speed Of Light",
	"Crystal Planet",
	"Raspberry Jam Delta-V",
	"Champagne?",
	"Clouds Race Across The Sky",
	"Engines Of Creation"
};

static void fail(const char *reason)
{
	perror(reason);
	exit(EXIT_FAILURE);
}

static void *regular_thread(void *arg)
{
	char buf[128], *devname;
	int fd, ret;

	if (asprintf(&devname,
		     "/proc/xenomai/registry/rtipc/xddp/%s",
		     XDDP_PORT_LABEL) < 0)
		fail("asprintf");

	fd = open(devname, O_RDWR);
	free(devname);
	if (fd < 0)
		fail("open");

	for (;;) {
		/* Get the next message from realtime_thread2. */
		ret = read(fd, buf, sizeof(buf));
		if (ret <= 0)
			fail("read");

		/* Relay the message to realtime_thread1. */
		ret = write(fd, buf, ret);
		if (ret <= 0)
			fail("write");
	}

	return NULL;
}

int main(int argc, char **argv)
{
	pthread_attr_t regattr;
	sigset_t set;
	int sig;

	sigemptyset(&set);
	sigaddset(&set, SIGINT);
	sigaddset(&set, SIGTERM);
	sigaddset(&set, SIGHUP);
	pthread_sigmask(SIG_BLOCK, &set, NULL);

	pthread_attr_init(&regattr);
	pthread_attr_setdetachstate(&regattr, PTHREAD_CREATE_JOINABLE);
	pthread_attr_setinheritsched(&regattr, PTHREAD_EXPLICIT_SCHED);
	pthread_attr_setschedpolicy(&regattr, SCHED_OTHER);

	errno = pthread_create(&nrt, &regattr, &regular_thread, NULL);
	if (errno)
		fail("pthread_create");

	sigwait(&set, &sig);
	pthread_cancel(nrt);
	pthread_join(nrt, NULL);

	return 0;
}
