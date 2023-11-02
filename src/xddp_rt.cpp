// #include <stdio.h>
// #include <stdlib.h>
// #include <unistd.h>
// #include <signal.h>
// #include <string.h>
// #include <malloc.h>
// #include <pthread.h>
// #include <fcntl.h>
// #include <errno.h>
// #include <rtdm/ipc.h>

// pthread_t rt1, rt2;

// #define XDDP_PORT_LABEL  "xddp"

// static const char *msg[] = {
// 	"Surfing With The Alien",
// 	"Lords of Karma",
// 	"Banana Mango",
// 	"Psycho Monkey",
// 	"Luminous Flesh Giants",
// 	"Moroccan Sunset",
// 	"Satch Boogie",
// 	"Flying In A Blue Dream",
// 	"Ride",
// 	"Summer Song",
// 	"Speed Of Light",
// 	"Crystal Planet",
// 	"Raspberry Jam Delta-V",
// 	"Champagne?",
// 	"Clouds Race Across The Sky",
// 	"Engines Of Creation"
// };

// static void fail(const char *reason)
// {
// 	perror(reason);
// 	exit(EXIT_FAILURE);
// }

// static void *realtime_thread1(void *arg)
// {
// 	struct rtipc_port_label plabel;
// 	struct sockaddr_ipc saddr;
// 	char buf[128];
// 	int ret, s;

// 	/*
// 	 * Get a datagram socket to bind to the RT endpoint. Each
// 	 * endpoint is represented by a port number within the XDDP
// 	 * protocol namespace.
// 	 */
// 	s = socket(AF_RTIPC, SOCK_DGRAM, IPCPROTO_XDDP);
// 	if (s < 0) {
// 		perror("socket");
// 		exit(EXIT_FAILURE);
// 	}

// 	/*
// 	 * Set a port label. This name will be registered when
// 	 * binding, in addition to the port number (if given).
// 	 */
// 	strcpy(plabel.label, XDDP_PORT_LABEL);
// 	ret = setsockopt(s, SOL_XDDP, XDDP_LABEL,
// 			 &plabel, sizeof(plabel));
// 	if (ret)
// 		fail("setsockopt");
// 	/*
// 	 * Bind the socket to the port, to setup a proxy to channel
// 	 * traffic to/from the Linux domain. Assign that port a label,
// 	 * so that peers may use a descriptive information to locate
// 	 * it. For instance, the pseudo-device matching our RT
// 	 * endpoint will appear as
// 	 * /proc/xenomai/registry/rtipc/xddp/<XDDP_PORT_LABEL> in the
// 	 * Linux domain, once the socket is bound.
// 	 *
// 	 * saddr.sipc_port specifies the port number to use. If -1 is
// 	 * passed, the XDDP driver will auto-select an idle port.
// 	 */
// 	memset(&saddr, 0, sizeof(saddr));
// 	saddr.sipc_family = AF_RTIPC;
// 	saddr.sipc_port = -1;
// 	ret = bind(s, (struct sockaddr *)&saddr, sizeof(saddr));
// 	if (ret)
// 		fail("bind");

// 	for (;;) {
// 		/* Get packets relayed by the regular thread */
// 		ret = recvfrom(s, buf, sizeof(buf), 0, NULL, 0);
// 		if (ret <= 0)
// 			fail("recvfrom");

// 		printf("%s: \"%.*s\" relayed by peer\n", __FUNCTION__, ret, buf);
// 	}

// 	return NULL;
// }

// static void *realtime_thread2(void *arg)
// {
// 	struct rtipc_port_label plabel;
// 	struct sockaddr_ipc saddr;
// 	int ret, s, n = 0, len;
// 	struct timespec ts;
// 	struct timeval tv;
// 	socklen_t addrlen;

// 	s = socket(AF_RTIPC, SOCK_DGRAM, IPCPROTO_XDDP);
// 	if (s < 0) {
// 		perror("socket");
// 		exit(EXIT_FAILURE);
// 	}

// 	/*
// 	 * Set the socket timeout; it will apply when attempting to
// 	 * connect to a labeled port, and to recvfrom() calls.  The
// 	 * following setup tells the XDDP driver to wait for at most
// 	 * one second until a socket is bound to a port using the same
// 	 * label, or return with a timeout error.
// 	 */
// 	tv.tv_sec = 1;
// 	tv.tv_usec = 0;
// 	ret = setsockopt(s, SOL_SOCKET, SO_RCVTIMEO,
// 			 &tv, sizeof(tv));
// 	if (ret)
// 		fail("setsockopt");

// 	/*
// 	 * Set a port label. This name will be used to find the peer
// 	 * when connecting, instead of the port number.
// 	 */
// 	strcpy(plabel.label, XDDP_PORT_LABEL);
// 	ret = setsockopt(s, SOL_XDDP, XDDP_LABEL,
// 			 &plabel, sizeof(plabel));
// 	if (ret)
// 		fail("setsockopt");

// 	memset(&saddr, 0, sizeof(saddr));
// 	saddr.sipc_family = AF_RTIPC;
// 	saddr.sipc_port = -1;	/* Tell XDDP to search by label. */
// 	ret = connect(s, (struct sockaddr *)&saddr, sizeof(saddr));
// 	if (ret)
// 		fail("connect");

// 	/*
// 	 * We succeeded in making the port our default destination
// 	 * address by using its label, but we don't know its actual
// 	 * port number yet. Use getpeername() to retrieve it.
// 	 */
// 	addrlen = sizeof(saddr);
// 	ret = getpeername(s, (struct sockaddr *)&saddr, &addrlen);
// 	if (ret || addrlen != sizeof(saddr))
// 		fail("getpeername");

// 	printf("%s: NRT peer is reading from /dev/rtp%d\n",
// 	       __FUNCTION__, saddr.sipc_port);

// 	for (;;) {
// 		len = strlen(msg[n]);
// 		/*
// 		 * Send a datagram to the NRT endpoint via the proxy.
// 		 * We may pass a NULL destination address, since the
// 		 * socket was successfully assigned the proper default
// 		 * address via connect(2).
// 		 */
// 		ret = sendto(s, msg[n], len, 0, NULL, 0);
// 		if (ret != len)
// 			fail("sendto");

// 		printf("%s: sent %d bytes, \"%.*s\"\n",
// 		       __FUNCTION__, ret, ret, msg[n]);

// 		n = (n + 1) % (sizeof(msg) / sizeof(msg[0]));
// 		/*
// 		 * We run in full real-time mode (i.e. primary mode),
// 		 * so we have to let the system breathe between two
// 		 * iterations.
// 		 */
// 		ts.tv_sec = 0;
// 		ts.tv_nsec = 500000000; /* 500 ms */
// 		clock_nanosleep(CLOCK_REALTIME, 0, &ts, NULL);
// 	}

// 	return NULL;
// }

// int main(int argc, char **argv)
// {
// 	struct sched_param rtparam = { .sched_priority = 42 };
// 	pthread_attr_t rtattr;
// 	sigset_t set;
// 	int sig;

// 	sigemptyset(&set);
// 	sigaddset(&set, SIGINT);
// 	sigaddset(&set, SIGTERM);
// 	sigaddset(&set, SIGHUP);
// 	pthread_sigmask(SIG_BLOCK, &set, NULL);

// 	pthread_attr_init(&rtattr);
// 	pthread_attr_setdetachstate(&rtattr, PTHREAD_CREATE_JOINABLE);
// 	pthread_attr_setinheritsched(&rtattr, PTHREAD_EXPLICIT_SCHED);
// 	pthread_attr_setschedpolicy(&rtattr, SCHED_FIFO);
// 	pthread_attr_setschedparam(&rtattr, &rtparam);

// 	/* Both real-time threads have the same attribute set. */

// 	errno = pthread_create(&rt1, &rtattr, &realtime_thread1, NULL);
// 	if (errno)
// 		fail("pthread_create");

// 	errno = pthread_create(&rt2, &rtattr, &realtime_thread2, NULL);
// 	if (errno)
// 		fail("pthread_create");

// 	__STD(sigwait(&set, &sig));
// 	pthread_cancel(rt1);
// 	pthread_cancel(rt2);
// 	pthread_join(rt1, NULL);
// 	pthread_join(rt2, NULL);

// 	return 0;
// }








#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <malloc.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/mman.h>
#include <alchemy/task.h>
#include <alchemy/timer.h>
#include <rtdm/ipc.h>  // RTDM IPC를 위한 헤더

RT_TASK mytask;

#define XDDP_PORT 0	/* [0..CONFIG-XENO_OPT_PIPE_NRDEV - 1] */

unsigned int cycle_ns = 1000000; // 1 ms

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

void rt_task(void *arg) {
    struct sockaddr_ipc saddr;
	int ret, s, n = 0, len;
	struct timespec ts;
	size_t poolsz;
	char buf[128];

    rt_task_set_periodic(NULL, TM_NOW, 100*cycle_ns); // 100ms

	/*
	 * Get a datagram socket to bind to the RT endpoint. Each
	 * endpoint is represented by a port number within the XDDP
	 * protocol namespace.
	 */
	s = socket(AF_RTIPC, SOCK_DGRAM, IPCPROTO_XDDP);
	if (s < 0) {
		perror("socket");
		exit(EXIT_FAILURE);
	}

	/*
	 * Set a local 16k pool for the RT endpoint. Memory needed to
	 * convey datagrams will be pulled from this pool, instead of
	 * Xenomai's system pool.
	 */
	poolsz = 16384; /* bytes */
	ret = setsockopt(s, SOL_XDDP, XDDP_POOLSZ,
			 &poolsz, sizeof(poolsz));
	if (ret)
		fail("setsockopt");

	/*
	 * Bind the socket to the port, to setup a proxy to channel
	 * traffic to/from the Linux domain.
	 *
	 * saddr.sipc_port specifies the port number to use.
	 */
	memset(&saddr, 0, sizeof(saddr));
	saddr.sipc_family = AF_RTIPC;
	saddr.sipc_port = XDDP_PORT;
	ret = bind(s, (struct sockaddr *)&saddr, sizeof(saddr));
	if (ret)
		fail("bind");

    while(1) 
    {
        rt_task_wait_period(NULL); //wait for next cycle

		len = strlen(msg[n]);
		/*
		 * Send a datagram to the NRT endpoint via the proxy.
		 * We may pass a NULL destination address, since a
		 * bound socket is assigned a default destination
		 * address matching the binding address (unless
		 * connect(2) was issued before bind(2), in which case
		 * the former would prevail).
		 */
		ret = sendto(s, msg[n], len, 0, NULL, 0);
		if (ret != len)
			fail("sendto");

		printf("%s: sent %d bytes, \"%.*s\"\n",
		       __FUNCTION__, ret, ret, msg[n]);

		/* Read back packets echoed by the regular thread */
		ret = recvfrom(s, buf, sizeof(buf), 0, NULL, 0);
		if (ret <= 0)
			fail("recvfrom");

		printf("   => \"%.*s\" echoed by peer\n", ret, buf);

		n = (n + 1) % (sizeof(msg) / sizeof(msg[0]));

	}

	return;
}

int main(int argc, char* argv[]) {
    mlockall(MCL_CURRENT|MCL_FUTURE);

    rt_task_create(&mytask, "mytask", 0, 50, 0);

    rt_task_start(&mytask, &rt_task, NULL);

    pause();  // Wait for Ctrl+C

    rt_task_delete(&mytask);

    return 0;
}
