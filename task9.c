
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <linux/membarrier.h>
#include <sys/syscall.h>
#include <unistd.h>

#include "common.h"


enum{
    COUNT_OF_THREADS = 4,
    STOP_POINTS = 1000000
};


struct Context{
    long double local_res;
    int rank;
    int* is_end;
    int64_t *mx_iters;
    pthread_barrier_t *barrier;
};
typedef struct Context context_t;

#define get(i__) (1.0 / (1.0 + (i__)*2.0) * ((i__) & 1? -1.0 : 1.0))
#define MAX(X, Y) (((X) > (Y)) ? (X) : (Y))

static void*
subroutine(void *arg) {
    context_t* context = (context_t*) arg;

    long double local_res = 0;
    int64_t i;
    int is_end = 0;
    for(i=0; !is_end; ++i) {
        local_res += get(i*COUNT_OF_THREADS + context->rank);

        if(i % STOP_POINTS){

            is_end = *context->is_end;
            CE(pthread_rwlock_unlock(context->rwlock));
        }
    }
    CE(pthread_rwlock_wrlock(context->rwlock));
    *context->mx_iters = MAX(*context->mx_iters, i);
    CE(pthread_rwlock_unlock(context->rwlock));

    CE(pthread_barrier_wait(context->barrier));
    int64_t mx = *context->mx_iters;
    for(; i < mx; ++i){
        local_res += get(i*COUNT_OF_THREADS + context->rank);
    }

    context->local_res = local_res;
    return NULL;
}


static int *is_end_ptr;
static pthread_rwlock_t *rwlock_ptr;

static void
int_handler(int sig){
    CE(pthread_rwlock_rdlock(rwlock_ptr));
    *is_end_ptr = 1;
    CE(pthread_rwlock_unlock(rwlock_ptr));
}

int
main() {
    int is_end = 0;


    context_t context[COUNT_OF_THREADS];
    pthread_t workers[COUNT_OF_THREADS];
    for(int i=0; i<COUNT_OF_THREADS; ++i){
        context[i].rank = i;
        context[i].
        CE(pthread_create(workers+i, NULL, subroutine, context+i));
    }

    double res = 0;
    for(int i=0; i<COUNT_OF_THREADS; ++i){
        CE(pthread_join(workers[i], NULL));
        res += context[i].res;
    }
    res *= 4.0;

    printf("%.6f\n", res);

    return 0;
}
