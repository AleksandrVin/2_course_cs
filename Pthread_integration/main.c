#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <sys/types.h>
#include <unistd.h>

#include <pthread.h>

#define cpu_amount_filename "/sys/devices/system/cpu/online"

#define DIFFICULTY 1000000000

#define A -1000
#define B 1000

long double f(double x, void *params)
{
    return expl(-pow(x, 2));
}

void *TrashFunc(void *number)
{
    while (1)
    {
        pow(1000, 3);
    }
}

struct Calc_node
{
    long double (*function)(double, void *);
    int a, b;
    double result_p;
    int theads;
    char trash[4028 - sizeof(void*)*2 - sizeof(int)*3];
};

void *CalcFunc(struct Calc_node *node)
{
    double a_ = (*node).a;
    double b_ = (*node).b;
    long double step = (b_ - a_) / ( DIFFICULTY / node->theads);
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

    int core0, coreMax;

    if (file_cpus != NULL)
    {
        fscanf(file_cpus, "%i-%i", &core0, &coreMax);
        fclose(file_cpus);
    }


#ifdef TRASH_ALLOWED

    pthread_t *tids; // [coreMax - thread_amount + 1];

    // load default attrebuts in attr of thread
    pthread_attr_t attr;

    if (coreMax >= thread_amount)
    {
        // creating "trash threads"
        printf("creating %i TrashThread\n", coreMax - thread_amount + 1);
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
    }
#endif

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

    printf("waiting for threads\n");

    for (int i = 0; i < thread_amount; i++)
    {
        int status = pthread_join(tids_w[i], NULL);
        if (status != 0)
        {
            printf("#### Error in thread waiting happend\n");
            return -1;
        }
        result_f_global += nodes[i].result_p;
    }

    /*     if (coreMax >= thread_amount)
    {
        // canseling "trash threads"
        for(size_t i = 0; i <= coreMax - thread_amount;i++)
        {
            pthread_cancel(tids[i]);
        }
    } */

    printf("fixed result    is %Lf\n", result_f_global);
    return 0;
}