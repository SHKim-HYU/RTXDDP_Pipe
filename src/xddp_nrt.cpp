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

#define XDDP_PORT 0	/* [0..CONFIG-XENO_OPT_PIPE_NRDEV - 1] */

static void fail(const char *reason)
{
	perror(reason);
	exit(EXIT_FAILURE);
}

static void *regular_thread(void *arg)
{
	char buf[128], *devname;
	int fd, ret;

	if (asprintf(&devname, "/dev/rtp%d", XDDP_PORT) < 0)
		fail("asprintf");

	fd = open(devname, O_RDWR);
	free(devname);
	if (fd < 0)
		fail("open");

	for (;;) {
		/* Get the next message from realtime_thread. */
		ret = read(fd, buf, sizeof(buf));
		if (ret <= 0)
			fail("read");

		/* Echo the message back to realtime_thread. */
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
