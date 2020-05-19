#include <stdio.h>
#include <math.h>
#include <gsl/gsl_integration.h>

#include <sys/types.h>
#include <unistd.h>

#include <sys/sysinfo.h>

#include <omp.h>

#include <pthread.h>

#define cpu_amount_filename "/sys/devices/system/cpu/online"

#define DIFFICULTY 1000

double f(double x, void *params)
{
    return exp(-pow(x, 2));
}

void *TrashFunc(void *number)
{
    while (1)
    {
        pow(1000, 3);
    }
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
    printf("system have %d cpus\n", get_nprocs());

    FILE *file_cpus = fopen(cpu_amount_filename, "r");
    int core0, coreMax;
    fscanf(file_cpus, "%i-%i", &core0, &coreMax);
    fclose(file_cpus);

    omp_set_num_threads(thread_amount);

    // ---calculating---
    double result_f_global = 0;
#pragma omp parallel for reduction(+ \
                                   : result_f_global)
    for (int i = -100; i <= 100; i++)
    {
        gsl_integration_fixed_workspace *w_f = gsl_integration_fixed_alloc(gsl_integration_fixed_legendre, DIFFICULTY, i, i + 1, 0, 0);
        double result_f;
        gsl_function F;
        F.function = &f;
        F.params = NULL;

        gsl_integration_fixed(&F, &result_f, w_f);
        result_f_global += result_f;

        gsl_integration_fixed_free(w_f);
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

    printf("fixed result    is %f\n", result_f_global);
    double pi = 3.14159265359;
    printf("expected result is %f\n", sqrt(pi));
    return 0;
}