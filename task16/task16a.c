
#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <pthread.h>
#include <limits.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>


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
        if(intsf_sem_wait(context->my_sem) == -1){
            perror("sem wait fail");
            exit(1);
        }
        printf("%s\n", context->message);
        if(sem_post(context->neighbor_sem)==-1){
            perror("sem post fail!");
            exit(1);
        }
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
        sem_unlink("/sem1");
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
        if(wait(NULL)==-1){
            perror("wait fail");
            exit(1);
        }
    }else{
        perror("fork fail");
    }

    if(sem_unlink("/sem1")==-1){
        perror("sem1 unlink fail");
        exit(1);
    }
    if(sem_unlink("/sem2")==-1){
        perror("sem2 unlink fail");
        exit(1);
    }

    return 0;
}