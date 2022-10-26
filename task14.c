
#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <pthread.h>
#include <errno.h>

#include "common.h"


enum{
    COUNT_OF_LOOPS = 10
};


struct Context{
    sem_t* my_sem;
    sem_t* neighbor_sem;
    const char* message;
};
typedef struct Context context_t;


static int 
intsf_sem_wait(sem_t* sem){
    while(sem_wait(sem) == -1){
        if(errno != EINTR) {
            return -1;
        }
    }
    return 0;
}


static void *
subroutine(void *arg) {
    context_t* context = (context_t*) arg;

    for(int i=0; i<COUNT_OF_LOOPS; ++i) {
        intsf_sem_wait(context->my_sem);
        printf("%s\n", context->message);
        sem_post(context->neighbor_sem);
    }

    return NULL;
}


int 
main() {

    sem_t sem1, sem2;
    CE(sem_init(&sem1, 0, 1));
    CE(sem_init(&sem2, 0, 0));
    context_t context1 = {&sem1, &sem2, "+++++"};
    context_t context2 = {&sem2, &sem1, "-----"};

    pthread_t ping_thread, pong_thread;

    CE(pthread_create(&ping_thread, NULL, subroutine, &context1));
    CE(pthread_create(&pong_thread, NULL, subroutine, &context2));

    pthread_join(ping_thread, NULL);
    pthread_join(pong_thread, NULL);
    return 0;
}
