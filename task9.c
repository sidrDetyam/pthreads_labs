
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <assert.h>

#include "common.h"


enum{
    COUNT_OF_THREADS = 4,
    STOP_POINTS = 1000
};


struct Context{
    long double local_res;
    int rank;
    int* is_end;
    int64_t *mx_iters;
    pthread_spinlock_t *spinlock;
    pthread_barrier_t *barrier;
};
typedef struct Context context_t;

#define get(i) (1.0 / (1.0 + (i)*2.0) * ((i) & 1? -1.0 : 1.0))
#define MAX(X, Y) (((X) > (Y)) ? (X) : (Y))

static void*
subroutine(void *arg) {
    context_t* context = (context_t*) arg;

    long double local_res = 0;
    int64_t i;
    for(i=0; ; ++i) {
        local_res += get(i*COUNT_OF_THREADS + context->rank);

        if(i % STOP_POINTS == 0){
            //asm volatile ("lfence" ::: "memory");
            //volatile int *is_end
            if(__sync_bool_compare_and_swap(context->is_end, 1, 1)){
                break;
            }
        }
    }

    CEM("spinlock", pthread_spin_lock(context->spinlock));
    *context->mx_iters = MAX(*context->mx_iters, i);
    CEM("spin unlock", pthread_spin_unlock(context->spinlock));
    printf("thread rank: %d, iters: %ld\n", context->rank, i);
    int bw = pthread_barrier_wait(context->barrier);
    assert(bw==0 || bw == PTHREAD_BARRIER_SERIAL_THREAD);

    int64_t mx = *context->mx_iters;
    for(; i < mx; ++i){
        local_res += get(i*COUNT_OF_THREADS + context->rank);
    }

    context->local_res = local_res;
    return arg;
}


static int is_end = 0;

static void
int_handler(int sig){
    assert(SIGINT == sig);
    assert(__sync_bool_compare_and_swap(&is_end, 0, 1));
}

int
main() {
    struct sigaction int_action;
    int_action.sa_handler = int_handler;
    CE(sigemptyset(&int_action.sa_mask));
    int_action.sa_flags = 0;
    CE(sigaction(SIGINT, &int_action, NULL));

    pthread_spinlock_t spinlock;
    pthread_barrier_t barrier;
    CE(pthread_spin_init(&spinlock, PTHREAD_PROCESS_PRIVATE));
    CE(pthread_barrier_init(&barrier, NULL, COUNT_OF_THREADS));
    int64_t mx_iters = 0;

    context_t context[COUNT_OF_THREADS];
    pthread_t workers[COUNT_OF_THREADS];
    for(int i=0; i<COUNT_OF_THREADS; ++i){
        context[i].rank = i;
        context[i].is_end = &is_end;
        context[i].spinlock = &spinlock;
        context[i].barrier = &barrier;
        context[i].mx_iters = &mx_iters;
        CE(pthread_create(workers+i, NULL, subroutine, context+i));
    }

    long double res = 0;
    for(int i=0; i<COUNT_OF_THREADS; ++i){
        pthread_join(workers[i], NULL);
        res += context[i].local_res;
    }
    res *= 4.0;
    printf("\n%.18Lf\n", res);

    CE(pthread_barrier_destroy(&barrier));
    CE(pthread_spin_destroy(&spinlock));

    return 0;
}
