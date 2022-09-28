
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


struct Node{
    struct Node* next_node;
    char* text;
}
typedef struct Node node_t;


struct LinkedList{
    
}





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
        intsf_sem_wait(context->my_sem);
        printf("%s\n", context->message);
        sem_post(context->neighbor_sem);
    }

    return NULL;
}


int 
main() {

    sem_t *sem1 = sem_open("/sem1", O_CREAT, 0644, 1);
    if(sem1 == SEM_FAILED){
        perror("semaphore 1 creation fail");
        exit(0);
    }

    sem_t *sem2 = sem_open("/sem2", O_CREAT, 0644, 0);
    if(sem2 == SEM_FAILED){
        perror("semaphore 1 creation fail");
        sem_unlink("/sem1");
        exit(0);
    }

    context_t context1 = {sem1, sem2, "+++++"};
    context_t context2 = {sem2, sem1, "-----"};

    pid_t pid = fork();
    if(pid==0){
        subroutine(&context2);
        sem_unlink("/sem1");
        sem_unlink("/sem2");
        exit(0);
    }

    if(pid!=-1){
        subroutine(&context1);
        wait(NULL);
    }else{
        perror("fork fail");
    }

    sem_unlink("/sem1");
    sem_unlink("/sem2");

    return 0;
}
