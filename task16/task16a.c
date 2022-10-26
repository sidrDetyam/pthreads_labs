
#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <fcntl.h>
#include <unistd.h>
#include "../common.h"
#include <sys/wait.h>


enum{
    COUNT_OF_LOOPS = 10
};


struct Context{
    sem_t* my_sem;
    sem_t* neighbor_sem;
    const char* message;
};
typedef struct Context context_t;


static void *
subroutine(void *arg) {
    context_t* context = (context_t*) arg;

    for(int i=0; i<COUNT_OF_LOOPS; ++i) {
        CE(sem_wait(context->my_sem));
        printf("%s\n", context->message);
        CE(sem_post(context->neighbor_sem));
    }

    return NULL;
}


int 
main() {

    sem_t *sem1 = sem_open("/sem1", O_CREAT, 0644, 1);
    if(sem1 == SEM_FAILED){
        perror("semaphore 1 creation fail");
        exit(1);
    }

    sem_t *sem2 = sem_open("/sem2", O_CREAT, 0644, 0);
    if(sem2 == SEM_FAILED){
        perror("semaphore 1 creation fail");
        CEM("sem unlink fail", sem_unlink("/sem1"));
        exit(1);
    }

    context_t context1 = {sem1, sem2, "+++++"};
    context_t context2 = {sem2, sem1, "-----"};

    pid_t pid = fork();
    if(pid==0){
        subroutine(&context2);
        sem_unlink("/sem1");
        sem_unlink("/sem2");
        exit(1);
    }

    if(pid != -1){
        subroutine(&context1);
        CE(wait(NULL));
    }else{
        perror("fork fail");
        sem_unlink("/sem1");
        sem_unlink("/sem2");
        exit(1);
    }

    CE(sem_close(sem1));
    CE(sem_close(sem2));

    return 0;
}
