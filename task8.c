
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "common.h"


enum{
    COUNT_OF_THREADS = 4,
    COUNT_OF_LOOPS = 1000000
};


struct Context{
    double res;
    int rank;
    int size;
};
typedef struct Context context_t;

#define get(i__) (1.0 / (1.0 + (i__)*2.0) * ((i__) & 1? -1.0 : 1.0))

static void*
subroutine(void *arg) {
    context_t* context = (context_t*) arg;

    double local_res = 0;
    for(int i=0; i<COUNT_OF_LOOPS; ++i) {
        local_res += get(i*context->size + context->rank);
    }
    context->res = local_res;
    //printf("%.3f\n", local_res);

    return NULL;
}


int
main() {
    context_t context[COUNT_OF_THREADS];
    pthread_t workers[COUNT_OF_THREADS];
    for(int i=0; i<COUNT_OF_THREADS; ++i){
        context[i].size = COUNT_OF_THREADS;
        context[i].rank = i;
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
