
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>

#include "common.h"


enum{
    COUNT_OF_PHIL = 5,
    DINNER_DELAY = 1000,
    THINK_DELAY = 2000
};


struct Context{
    int rank;
    pthread_mutex_t *m;
    pthread_cond_t *c;
    pthread_mutex_t* forks;
};
typedef struct Context context_t;


static void*
phil(void *arg) {
    context_t* context = (context_t*) arg;

    int l, r;
    if(context->rank == COUNT_OF_PHIL - 1){
        l = 0;
        r = COUNT_OF_PHIL - 1;
    }
    else{
        l = context->rank;
        r = context->rank + 1;
    }

    for(int i=0; i<2000000000; ++i) {
        usleep(THINK_DELAY);

        CE(pthread_mutex_lock(context->m));

        while(1) {
            int err;
            if ((err = pthread_mutex_trylock(context->forks + l)) != 0) {
                if (err != EBUSY) {
                    exit(1);
                }
                CE(pthread_cond_wait(context->c, context->m));
                continue;
            }

            if ((err = pthread_mutex_trylock(context->forks + r)) != 0) {
                if (err != EBUSY) {
                    exit(1);
                }
                CE(pthread_mutex_unlock(context->forks + l));
                CE(pthread_cond_wait(context->c, context->m));
                continue;
            }

            CE(pthread_mutex_unlock(context->m));
            break;
        }

        printf("Филосософ %d кушает\n", context->rank);
        usleep(DINNER_DELAY);

        CE(pthread_mutex_unlock(context->forks + r));
        CE(pthread_mutex_unlock(context->forks + l));

        CE(pthread_mutex_lock(context->m));
        CE(pthread_cond_broadcast(context->c));
        CE(pthread_mutex_unlock(context->m));
    }

    return NULL;
}

int
main() {

    pthread_mutex_t forks[COUNT_OF_PHIL];
    pthread_mutex_t m;
    pthread_cond_t cond;
    CE(pthread_mutex_init(&m, NULL));
    CE(pthread_cond_init(&cond, NULL));

    context_t contexts[COUNT_OF_PHIL];
    pthread_t phils[COUNT_OF_PHIL];

    for(size_t i = 0; i < COUNT_OF_PHIL; ++i){
        CE(pthread_mutex_init(forks+i, NULL));
        contexts[i].rank = (int) i;
        contexts[i].forks = forks;
        contexts[i].m = &m;
        contexts[i].c = &cond;
        CE(pthread_create(phils+i, NULL, phil, contexts+i));
    };

    for(int i=0; i<COUNT_OF_PHIL; ++i){
        CE(pthread_join(phils[i], NULL));
    }

    for(size_t i = 0; i < COUNT_OF_PHIL; ++i) {
        CE(pthread_mutex_destroy(forks + i));
    }
    return 0;
}
