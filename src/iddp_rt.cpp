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

RT_TASK server_task;
RT_TASK client_task;

#define IDDP_SVPORT 12
#define IDDP_CLPORT 13
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

void rt_client_run(void *arg) {
    rt_task_set_periodic(NULL, TM_NOW, 100*cycle_ns); // 100ms
    struct sockaddr_ipc svsaddr, clsaddr;
    int ret, s, n = 0, len;
    struct timespec ts;
    s = __cobalt_socket(AF_RTIPC, SOCK_DGRAM, IPCPROTO_IDDP);
    if (s < 0)
            fail("socket");
    clsaddr.sipc_family = AF_RTIPC;
    clsaddr.sipc_port = IDDP_CLPORT;
    ret = __cobalt_bind(s, (struct sockaddr *)&clsaddr, sizeof(clsaddr));
    if (ret)
            fail("bind");
    svsaddr.sipc_family = AF_RTIPC;
    svsaddr.sipc_port = IDDP_SVPORT;    

    if (ret)
            fail("bind");    
    while(1) 
    {
        rt_task_wait_period(NULL); //wait for next cycle
        len = strlen(msg[n]);
        ret = __cobalt_sendto(s, msg[n], len, 0,
                        (struct sockaddr *)&svsaddr, sizeof(svsaddr));
        if (ret < 0) {
                close(s);
                fail("sendto");
        }
        printf("%s: sent %d bytes, \"%.*s\"\n",
                __FUNCTION__, ret, ret, msg[n]);
        n = (n + 1) % (sizeof(msg) / sizeof(msg[0]));

	}
	return;
}


void rt_server_run(void *arg) {
    rt_task_set_periodic(NULL, TM_NOW, 100*cycle_ns); // 100ms
    struct sockaddr_ipc saddr, claddr;
    socklen_t addrlen;
    char buf[128];
    size_t poolsz;
    int ret, s;
    s = __cobalt_socket(AF_RTIPC, SOCK_DGRAM, IPCPROTO_IDDP);
    if (s < 0)
            fail("socket");
    /*
        * Set a local 32k pool for the server endpoint. Memory needed
        * to convey datagrams will be pulled from this pool, instead
        * of Xenomai's system pool.
        */
    poolsz = 32768; /* bytes */
    ret = __cobalt_setsockopt(s, SOL_IDDP, IDDP_POOLSZ,
                        &poolsz, sizeof(poolsz));
    if (ret)
            fail("setsockopt");
    saddr.sipc_family = AF_RTIPC;
    saddr.sipc_port = IDDP_SVPORT;
    ret = __cobalt_bind(s, (struct sockaddr *)&saddr, sizeof(saddr));    
    while(1) 
    {
        rt_task_wait_period(NULL); //wait for next cycle
        addrlen = sizeof(saddr);
        ret = __cobalt_recvfrom(s, buf, sizeof(buf), 0,
                    (struct sockaddr *)&claddr, &addrlen);
        if (ret < 0) {
            close(s);
            fail("recvfrom");
        }
        printf("%s: received %d bytes, \"%.*s\" from port %d\n",
            __FUNCTION__, ret, ret, buf, claddr.sipc_port);

	}
	return;
}
int main(int argc, char* argv[]) {
    mlockall(MCL_CURRENT|MCL_FUTURE);

    rt_task_create(&server_task, "server_task", 0, 99, 0);
    rt_task_start(&server_task, &rt_server_run, NULL);
    rt_task_create(&client_task, "client_task", 0, 98, 0);
    rt_task_start(&client_task, &rt_client_run, NULL);
    pause();  // Wait for Ctrl+C

    rt_task_delete(&server_task);
    rt_task_delete(&client_task);

    return 0;
}

