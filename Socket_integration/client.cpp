#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <sys/socket.h>
#include <sys/types.h>

#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>

#include <arpa/inet.h>

#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>

#include <vector>

#define PORT_SERVER 55555
#define PORT_CLIENT 55556
#define protocol_start "protocol_start_1"
#define protocol_server_responce "protocol_responce_server_1"

#define BUF_SIZE 50

#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_GREEN "\x1b[32m"
#define ANSI_COLOR_YELLOW "\x1b[33m"
#define ANSI_COLOR_BLUE "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN "\x1b[36m"
#define ANSI_COLOR_RESET "\x1b[0m"

#define MAX_WAIT_SEC 1000

#define A -500
#define B 500

struct connection
{
    struct sockaddr_in addr;
    int tcp;
    int threads;
};

struct Calc_node
{
    int node_number;
    long double (*function)(double, void *);
    int a, b;
    long double result_p;
    int theads;
    char trash[4028 - sizeof(void *) * 2 - sizeof(int) * 3];
};

int thread_detected = 0;

int main()
{
    printf("seacting for nodes in local network\n");
    // udp broadcast
    int udp = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp == -1)
    {
        perror("can't create socket\n");
        return EXIT_FAILURE;
    }
    int broadcast = 1;
    if (setsockopt(udp, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast)))
    {
        perror("can't set broadcast socket\n");
        return EXIT_FAILURE;
    }

    struct sockaddr_in addr_broadcast;
    bzero(&addr_broadcast, sizeof(addr_broadcast));
    addr_broadcast.sin_family = AF_INET;
    addr_broadcast.sin_port = htons(PORT_SERVER);
    addr_broadcast.sin_addr.s_addr = INADDR_BROADCAST;

    // sending broadcast message
    char request[] = protocol_start;
    sendto(udp, request, strlen(request) + 1, 0, (struct sockaddr *)&addr_broadcast, sizeof(addr_broadcast));

    printf("waiting for servers responds\n");

    // waiting for responce

    std::vector<struct connection> connections;

    int retval;
    fd_set rfds;
    struct timeval tv;

    int max_fd = 0;

    do
    {
        tv.tv_sec = 1;
        tv.tv_usec = 0;

        FD_ZERO(&rfds);
        FD_SET(udp, &rfds);

        retval = select(udp + 1, &rfds, NULL, NULL, &tv);
        if (retval == -1)
        {
            perror("error in select\n");
            return EXIT_FAILURE;
        }
        if (retval)
        {
            struct connection connect_local = {};
            socklen_t addrlen = sizeof(struct sockaddr_in);
            char buf[BUF_SIZE] = "";
            ssize_t received = recvfrom(udp, buf, BUF_SIZE, MSG_DONTWAIT, (struct sockaddr *)&connect_local.addr, &addrlen);
            if (received == -1)
            {
                perror("error during recvfrom in nonblock after select\n");
                return EXIT_FAILURE;
            }
            if (strncmp(buf, protocol_server_responce, received) != 0 || received < 1)
            {
                printf("wrong responce accured\n");
                continue;
            }

            connect_local.tcp = socket(AF_INET, SOCK_STREAM, 0);
            if (connect_local.tcp == -1)
            {
                perror("socket");
                return EXIT_FAILURE;
            }

            int option = 1;
            setsockopt(connect_local.tcp, SOL_SOCKET, SO_KEEPALIVE, &option, sizeof(option));
            perror("keepalive");
            option = 1;
            setsockopt(connect_local.tcp, IPPROTO_TCP, TCP_KEEPCNT, &option, sizeof(option));
            perror("keepalive_2");
            option = 1;
            setsockopt(connect_local.tcp, IPPROTO_TCP, TCP_KEEPIDLE, &option, sizeof(option));
            perror("keepalive_3");
            option = 1;
            setsockopt(connect_local.tcp, IPPROTO_TCP, TCP_KEEPINTVL, &option, sizeof(option));
            perror("keepalive_4");

            if (connect(connect_local.tcp, (struct sockaddr *)&connect_local.addr, sizeof(connect_local.addr)))
            {
                perror("connect");
                return EXIT_FAILURE;
            }

            // setting keepalive time specs
            option = 1;
            setsockopt(connect_local.tcp, SOL_SOCKET, SO_KEEPALIVE, &option, sizeof(option));
            perror("keepalive_5");
            option = 1;
            setsockopt(connect_local.tcp, IPPROTO_TCP, TCP_KEEPCNT, &option, sizeof(option));
            option = 1;
            setsockopt(connect_local.tcp, IPPROTO_TCP, TCP_KEEPIDLE, &option, sizeof(option));
            option = 1;
            setsockopt(connect_local.tcp, IPPROTO_TCP, TCP_KEEPINTVL, &option, sizeof(option));

            if (recv(connect_local.tcp, &(connect_local.threads), sizeof(int), 0) > 0)
            {
                if (connect_local.threads > 0)
                {
                    connections.push_back(connect_local);
                    printf("connection established. Server have %i threads on ip: %s\n", connect_local.threads, inet_ntoa(connect_local.addr.sin_addr));
                    thread_detected += connect_local.threads;
                    if (max_fd < connect_local.tcp)
                        max_fd = connect_local.tcp;
                }
            }
            else
            {
                printf("no aswer from server");
            }
        }
    } while (retval > 0);

    if (connections.size() == 0)
    {
        printf("no servers found\n");
        return EXIT_FAILURE;
    }

    close(udp);

    printf("threads detected: %i", thread_detected);

    // prepating tasks for servers

    struct Calc_node *nodes = (struct Calc_node *)calloc(thread_detected, sizeof(struct Calc_node));
    int a_b_every_body = (B - A) / thread_detected;
    int a_b_last = (B - A) % thread_detected;

    printf("\nsending tasks\n");

    long double result_f_global = 0;
    for (int i = 0; i < thread_detected; i++)
    {
        nodes[i].a = a_b_every_body * i + A;
        nodes[i].b = nodes[i].a + a_b_every_body;
        nodes[i].theads = thread_detected;
        nodes[i].node_number = i;

        if (i >= thread_detected - 1)
        {
            nodes[i].b += a_b_last;
        }
    }

    int current_node = 0;
    for (auto a : connections)
    {
        for (int i = current_node; i < current_node + a.threads; i++)
        {
            tv.tv_sec = 1;
            tv.tv_usec = 0;

            FD_ZERO(&rfds);
            FD_SET(a.tcp, &rfds);

            retval = select(a.tcp + 1, NULL, &rfds, NULL, &tv);
            if (retval == -1)
            {
                perror("error in select");
                return EXIT_FAILURE;
            }
            else if (retval == 0)
            {
                perror("seems to server lost connection");
                return EXIT_FAILURE;
            }
            if (send(a.tcp, &(nodes[i]), sizeof(Calc_node), MSG_DONTWAIT) < (int)sizeof(Calc_node))
            {
                perror("can't send full node to server");
                return EXIT_FAILURE;
            }
        }
        current_node += a.threads;
    }

    printf("tasks send\n");

    FD_ZERO(&rfds);
    current_node = 0;
    while (current_node < thread_detected)
    {
        tv.tv_sec = MAX_WAIT_SEC;
        tv.tv_usec = 0;
        FD_ZERO(&rfds);
        for (auto b : connections)
        {
            FD_SET(b.tcp, &rfds);
        }

        //printf("waiting on select\n");

        retval = select(max_fd + 1, &rfds, NULL, NULL, &tv);
        if (retval == -1)
        {
            perror("error in select");
            return EXIT_FAILURE;
        }
        else if (retval == 0)
        {
            perror("seems to server lost connection");
            return EXIT_FAILURE;
        }

        int tcp_local;
        bool found =  false;

        for(auto a : connections)
        {
            if(FD_ISSET(a.tcp, &rfds))
            {
                tcp_local = a.tcp;
                found = true;
                break;
            }
        }
        if(!found)
        {
            printf("seems to disconnect of server");
            return EXIT_FAILURE;
        }

        struct Calc_node node;

        int recv_ret = recv(tcp_local, &(node.result_p), sizeof(node.result_p), MSG_DONTWAIT);
        if (recv_ret == -1)
        {
            perror("no result from server / seems to disconnect");
            return EXIT_FAILURE;
        }
        else if (recv_ret < (int)sizeof(node.result_p))
        {
            perror("not enoght result from server or disconnect of server");
            return EXIT_FAILURE;
        }
        result_f_global += node.result_p;
        current_node++;
    }

    printf(ANSI_COLOR_RESET "fixed result    is %Lf\n", result_f_global);

    // waiting for answers from servers

    return EXIT_SUCCESS;
}