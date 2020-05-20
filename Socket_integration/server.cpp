#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/sysinfo.h>
#include <sys/time.h>
#include <sys/resource.h>

#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>

#include <pthread.h>
#include <signal.h>

#define PORT_SERVER 55555
#define PORT_CLIENT 55556
#define protocol_start "protocol_start_1"
#define protocol_server_responce "protocol_responce_server_1"

#define BUF_SIZE 50

#define cpu_amount_filename "/sys/devices/system/cpu/online"
#define cpu_0_affinity "/sys/devices/system/cpu/cpu0/topology/thread_siblings_list"

#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_GREEN "\x1b[32m"
#define ANSI_COLOR_YELLOW "\x1b[33m"
#define ANSI_COLOR_BLUE "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN "\x1b[36m"
#define ANSI_COLOR_RESET "\x1b[0m"

#define DIFFICULTY 50000000 // set to 500000000 for normal time

#define MAX_WAIT_SEC 1000

#define A -5000
#define B 5000

struct Calc_node
{
    int node_number;
    long double (*function)(double, void *);
    int a, b;
    long double result_p;
    int theads;
    char trash[4028 - sizeof(void *) * 2 - sizeof(int) * 3];
};

void *CalcFunc(struct Calc_node *node)
{
    long double a_ = (*node).a;
    long double b_ = (*node).b;
    long double step = (b_ - a_) / (DIFFICULTY / node->theads);
    long double result = 0;
    for (; a_ < b_; a_ += step)
    {
        result += (*node).function(a_ + step / 2, NULL) * step;
    }
    (*node).result_p = result;
    return 0;
}

long double f(double x, void *params)
{
    return expl(-pow(x, 2));
}

#ifdef TRASH_ALLOWED
void *TrashFunc(void *number)
{
    while (1)
    {
        pow(1000, 3);
    }
}
#endif

int main(int argc, char **argv)
{

    printf("size of node is %li", sizeof(Calc_node));

    // ---reading threads amount from first arg---
    argc--;
    argv++;

    int threads = get_nprocs();
    int thread_amount = 0;
    if (argc != 1)
    {
        printf("use with arg = num_of_threa\n");
        printf("executing with all cores\n");
        thread_amount = threads;
    }
    else
    {
        if (sscanf(argv[0], "%i", &thread_amount) == 0)
        {
            printf("can't read number");
            return -1;
        }
        if (thread_amount <= 0)
        {
            printf("threads amount is less then 1");
            return -1;
        }
    }
    printf("executing with thread amount of %i\n", thread_amount);
    int coreMax = threads;
    printf("system have %d cpus\n", threads);
    int cpus_per_core = 0;
    char waste;

    int thread_sibling[8];

    FILE *file_cpus;
    file_cpus = fopen(cpu_0_affinity, "r");
    if (file_cpus != NULL)
    {
        //cpus_per_core = fscanf(file_cpus, "%i%c%i%c%i%c%i%c%i%c%i%c%i%c%i", &core0, &waste, &core0, &waste, &core0, &waste, &core0, &waste, &core0, &waste, &core0, &waste, &core0, &waste, &core0) / 2 + 1;
        while (!feof(file_cpus))
        {
            if (fscanf(file_cpus, "%i%c", thread_sibling + cpus_per_core, &waste) > 0)
            {
                cpus_per_core += 1;
            }
        }

        printf("cpu SMT is %i\n", cpus_per_core);

        fclose(file_cpus);
    }
    else
    {
        perror("can't read thread topology\n");
        return EXIT_FAILURE;
    }

    if (thread_amount > coreMax)
        thread_amount = coreMax;
    coreMax /= cpus_per_core;

    // creating udp waiting for client socket

    struct sockaddr_in addr_server;
    bzero(&addr_server, sizeof(addr_server));
    addr_server.sin_family = AF_INET;
    addr_server.sin_port = htons(PORT_SERVER);
    addr_server.sin_addr.s_addr = INADDR_ANY;

    int udp = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp == -1)
    {
        perror("socket ");
        return EXIT_FAILURE;
    }

    int broadcast = 1;
    if (setsockopt(udp, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast)))
    {
        perror("can't set broadcast");
        return EXIT_FAILURE;
    }
    if (bind(udp, (struct sockaddr *)&addr_server, sizeof(addr_server)))
    {
        perror("bind ");
        return EXIT_FAILURE;
    }

    socklen_t addrlen = sizeof(struct sockaddr_in);
    char buf[BUF_SIZE] = "";

    struct sockaddr_in addr_client = {};
    bzero(&addr_client, sizeof(addr_client));
    printf("waiting for client\n");

    ssize_t received = recvfrom(udp, buf, BUF_SIZE, 0, (struct sockaddr *)&addr_client, &addrlen);
    if (received == -1)
    {
        perror("recvfrom ");
        return EXIT_FAILURE;
    }
    if (strncmp(buf, protocol_start, received) != 0 || received < 1)
    {
        printf("wrong client request\n");
        return EXIT_FAILURE;
    }

    printf("broadcast received\n");

    // replying to client

    if (sendto(udp, protocol_server_responce, strlen(protocol_server_responce), 0, (const struct sockaddr *)&addr_client, sizeof(addr_client)) == -1)
    {
        perror("sendto");
        return EXIT_FAILURE;
    }

    close(udp);

    printf("creating tcp\n");

    int tcp = socket(AF_INET, SOCK_STREAM, 0);

    int option = 1;
    setsockopt(tcp, SOL_SOCKET, SO_REUSEPORT, &option, sizeof(option));
    option = 1;
    setsockopt(tcp, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

    if (bind(tcp, (struct sockaddr *)&addr_server, sizeof(addr_server)))
    {
        perror("bind tpc");
        return EXIT_FAILURE;
    }

    if (listen(tcp, 1024) == -1)
    {
        perror("listen");
        return EXIT_FAILURE;
    }

    fd_set rfds;
    struct timeval tv;
    int retval;

    tv.tv_sec = 1;
    tv.tv_usec = 0;

    FD_ZERO(&rfds);
    FD_SET(tcp, &rfds);

    retval = select(tcp + 1, &rfds, NULL, NULL, &tv);
    if (retval == -1)
    {
        perror("select");
        return EXIT_FAILURE;
    }
    else if (retval == 0)
    {
        printf("wait for client tcp connection timeout\n");
        return EXIT_FAILURE;
    }

    printf("select done\n");

    int connect_local = accept(tcp, (struct sockaddr *)&addr_client, &addrlen);
    if (connect_local < 0)
    {
        perror("accept");
        return EXIT_FAILURE;
    }

    close(tcp);

    option = 1;
    setsockopt(connect_local, SOL_SOCKET, SO_KEEPALIVE, &option, sizeof(option));
    perror("keepalive");
    option = 1;
    setsockopt(connect_local, IPPROTO_TCP, TCP_KEEPCNT, &option, sizeof(option));
    option = 1;
    setsockopt(connect_local, IPPROTO_TCP, TCP_KEEPIDLE, &option, sizeof(option));
    option = 1;
    setsockopt(connect_local, IPPROTO_TCP, TCP_KEEPINTVL, &option, sizeof(option));

    if (send(connect_local, &thread_amount, sizeof(threads), 0) > 0)
    {
        printf("connection established\n");
    }
    else
    {
        perror("can't send tcp to client");
        return EXIT_FAILURE;
    }

    // waiting for task from client
    pthread_t *tids_w = (pthread_t *)calloc(thread_amount, sizeof(pthread_t));
    pthread_attr_t attr_w;
    pthread_attr_init(&attr_w);

    struct Calc_node *nodes = (struct Calc_node *)calloc(thread_amount, sizeof(struct Calc_node));

    for (int i = 0; i < thread_amount; i++)
    {
        tv.tv_sec = 5;
        tv.tv_usec = 0;

        FD_ZERO(&rfds);
        FD_SET(connect_local, &rfds);

        retval = select(connect_local + 1, &rfds, NULL, NULL, &tv);
        if (retval == -1)
        {
            perror("error in select");
            return EXIT_FAILURE;
        }
        else if (retval == 0)
        {
            perror("no task from clien or not enought");
            return EXIT_FAILURE;
        }

        struct Calc_node node;

        if (recv(connect_local, &node, sizeof(Calc_node), MSG_DONTWAIT) < 1)
        {
            perror("broken task form client");
            return EXIT_FAILURE;
        }

        node.function = &f;

        nodes[i] = node;

        printf("node %i get with number %d thread is %i\n", i, node.node_number, node.theads);
    }

    printf("\nstarting threads\n");

    long double result_f_global = 0;
    for (int i = 0; i < thread_amount; i++)
    {

        int status = pthread_create(&(tids_w[i]), &attr_w, (void *(*)(void *))CalcFunc, &(nodes[i]));
        if (status != 0)
        {
            printf("#### Error in thread creating happend\n");
            return -1;
        }
    }

    pid_t pid = -1;

    struct rlimit limit;
    if (getrlimit(RLIMIT_NICE, &limit))
    {
        perror("error in getrlimit\n");
        return EXIT_FAILURE;
    }

#ifdef TRASH_ALLOWED

    pthread_t *tids; // [coreMax - thread_amount + 1];

    // load default attrebuts in attr of thread
    pthread_attr_t attr;

    if (coreMax >= thread_amount)
    {
        pid = fork();
        if (pid == -1)
        {
            perror("error while fork\n");
            return EXIT_FAILURE;
        }
        if (pid != 0)
        {
            goto thread_waiting;
        }

        if (setpriority(PRIO_PROCESS, getpid(), 19))
        {
            perror("error in child with setpriority\n");
            return EXIT_FAILURE;
        }
        else
        {
            printf(ANSI_COLOR_YELLOW "child is running with priority of 19\n" ANSI_COLOR_RESET);
        }

        // creating "trash threads"
        printf(ANSI_COLOR_MAGENTA "creating %i TrashThread\n" ANSI_COLOR_RESET, coreMax - thread_amount + 1);

        tids = (pthread_t *)calloc(coreMax - thread_amount + 1, sizeof(pthread_t));

        pthread_attr_init(&attr);

        for (int i = 0; i <= coreMax - thread_amount; i++)
        {
            int status = pthread_create(&(tids[i]), &attr, TrashFunc, NULL);
            if (status != 0)
            {
                printf("#### Error in thread creating happend\n");
                return -1;
            }
        }

        for (int i = 0; i <= coreMax - thread_amount; i++)
        {
            pthread_join(tids[i], NULL);
        }

        return 0;
    }

thread_waiting:
#endif

    if (limit.rlim_max > 0)
    {
        printf("hard limit is %ld , soft if %ld\n", limit.rlim_max, limit.rlim_cur);
        if (setpriority(PRIO_PROCESS, getpid(), limit.rlim_max))
        {
            perror("error in parent with setpriority\n");
            return EXIT_FAILURE;
        }
        printf("priority set to %ld\n", limit.rlim_max);
    }
    else
    {
        printf(ANSI_COLOR_GREEN "main process is running with default priority\n" ANSI_COLOR_RESET);
    }

    printf("waiting for threads\n");

    for (int i = 0; i < thread_amount; i++)
    {
        pthread_join(tids_w[i], NULL);

        tv.tv_sec = 5;
        tv.tv_usec = 0;

        FD_ZERO(&rfds);
        FD_SET(connect_local, &rfds);

        retval = select(connect_local + 1, NULL, &rfds, NULL, &tv);
        if (retval == -1)
        {
            perror("error in select");
            return EXIT_FAILURE;
        }
        else if (retval == 0)
        {
            perror("client don't wait for aswer");
            return EXIT_FAILURE;
        }

        if (send(connect_local, &nodes[i].result_p, sizeof(nodes[i].result_p), 0) < (int)sizeof(nodes[i].result_p))
        {
            perror("can't send result to client");
            return EXIT_FAILURE;
        }

        result_f_global += nodes[i].result_p;
    }

    if (pid != -1)
    {
        kill(pid, SIGKILL);
    }

    printf(ANSI_COLOR_RESET "calculations send\n");

    sleep(1);

    shutdown(connect_local, SHUT_RD);

    return EXIT_SUCCESS;
}