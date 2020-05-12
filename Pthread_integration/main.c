
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <sys/types.h>
#include <unistd.h>

#include <pthread.h>

#include <sys/time.h>
#include <sys/resource.h>

#include <signal.h>

#define cpu_amount_filename "/sys/devices/system/cpu/online"
#define cpu_0_affinity "/sys/devices/system/cpu/cpu0/topology/thread_siblings_list"

#define _GNU_SOURCE

#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_GREEN "\x1b[32m"
#define ANSI_COLOR_YELLOW "\x1b[33m"
#define ANSI_COLOR_BLUE "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN "\x1b[36m"
#define ANSI_COLOR_RESET "\x1b[0m"

#define DIFFICULTY 100000000 // set to 500000000 for normal time

#define A -800
#define B 800

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

struct Calc_node
{
    long double (*function)(double, void *);
    int a, b;
    double result_p;
    int theads;
    char trash[4028 - sizeof(void *) * 2 - sizeof(int) * 3];
};

void *CalcFunc(struct Calc_node *node)
{
    double a_ = (*node).a;
    double b_ = (*node).b;
    long double step = (b_ - a_) / (DIFFICULTY / node->theads);
    long double result = 0;
    for (; a_ < b_; a_ += step)
    {
        result += (*node).function(a_ + step / 2, NULL) * step;
    }
    (*node).result_p = result;
    return 0;
}

int main(int argc, char **argv)
{

    // ---reading threads amount from first arg---
    argc--;
    argv++;
    if (argc != 1)
    {
        printf("use with arg = num_of_thread");
        return -1;
    }
    int thread_amount = 0;
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

    printf("executing with thread amount of %i\n", thread_amount);

    FILE *file_cpus;
    file_cpus = fopen(cpu_amount_filename, "r");

    int core0 = 0, coreMax = 0;

    if (file_cpus != NULL)
    {
        fscanf(file_cpus, "%i-%i", &core0, &coreMax);
        fclose(file_cpus);
    }
    else
    {
        perror("can't read thread topology\n");
        return EXIT_FAILURE;
    }

    int cpus_per_core = 0;
    char waste;

    int thread_sibling[8];

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

    // ---calculating---

    pthread_t *tids_w = calloc(thread_amount, sizeof(pthread_t));
    pthread_attr_t attr_w;
    pthread_attr_init(&attr_w);

    struct Calc_node *nodes = calloc(thread_amount, sizeof(struct Calc_node));
    int a_b_every_body = (B - A) / thread_amount;
    int a_b_last = (B - A) % thread_amount;

    printf("\nstarting threads\n");

    long double result_f_global = 0;
    for (int i = 0; i < thread_amount; i++)
    {
        nodes[i].a = a_b_every_body * i + A;
        nodes[i].b = nodes[i].a + a_b_every_body;
        nodes[i].function = &f;
        nodes[i].theads = thread_amount;

        if (i >= thread_amount - 1)
        {
            nodes[i].b += a_b_last;
        }

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

        tids = calloc(coreMax - thread_amount + 1, sizeof(pthread_t));

        pthread_attr_init(&attr);

        for (size_t i = 0; i <= coreMax - thread_amount; i++)
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
        result_f_global += nodes[i].result_p;
    }

    if (pid != -1)
    {
        kill(pid, SIGKILL);
    }

    printf(ANSI_COLOR_RESET "fixed result    is %Lf\n", result_f_global);
    return 0;
}