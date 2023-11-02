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
#include <rtdm/ipc.h> 

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

    rt_task_set_periodic(NULL, TM_NOW, 1000*cycle_ns); // 100ms

	/*
	 * Get a datagram socket to bind to the RT endpoint. Each
	 * endpoint is represented by a port number within the XDDP
	 * protocol namespace.
	 */
	s = __cobalt_socket(AF_RTIPC, SOCK_DGRAM, IPCPROTO_XDDP);
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
	ret = __cobalt_setsockopt(s, SOL_XDDP, XDDP_POOLSZ,
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
	ret = __cobalt_bind(s, (struct sockaddr *)&saddr, sizeof(saddr));
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
		ret = __cobalt_sendto(s, msg[n], len, 0, NULL, 0);
		if (ret != len)
			fail("sendto");

		printf("%s: sent %d bytes, \"%.*s\"\n",
		       __FUNCTION__, ret, ret, msg[n]);

		/* Read back packets echoed by the regular thread */
		ret = __cobalt_recvfrom(s, buf, sizeof(buf), 0, NULL, 0);
		if (ret <= 0)
			fail("recvfrom");

		printf("   => \"%.*s\" echoed by peer\n", ret, buf);

		n = (n + 1) % (sizeof(msg) / sizeof(msg[0]));

	}

	return;
}

int main(int argc, char* argv[]) {
    mlockall(MCL_CURRENT|MCL_FUTURE);

    rt_task_create(&mytask, "mytask", 0, 99, 0);

    rt_task_start(&mytask, &rt_task, NULL);

    pause();  // Wait for Ctrl+C

    rt_task_delete(&mytask);

    return 0;
}
