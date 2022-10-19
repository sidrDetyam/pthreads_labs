#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdint.h>
#include <limits.h>


#define CHECK_ERROR(message, code) \
do{\
    if(code != 0){ \
        perror(message); \
        exit(1); \
    } \
}while(0)


struct Monitor{
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int flag;
};
typedef struct Monitor monitor_t;


struct Context{
    monitor_t* my_monitor;
    monitor_t* neighbor_monitor;
    const char* message;
};
typedef struct Context context_t;


static void 
monitor_init(monitor_t* monitor){
    if(pthread_cond_init(&(monitor->cond), NULL)==-1){
        perror("pthread_cond_init");
        exit(1);
    }

	static pthread_mutexattr_t mutex_attr;
    if(pthread_mutexattr_init(&mutex_attr)==-1){
        exit(1);
    }
    //зачем нам тут еррор-мьютекс?
    if(pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_ERRORCHECK)==-1){
        exit(1);
    }
    
    if(pthread_mutex_init(&(monitor->mutex), &mutex_attr)==-1){
        exit(1);
    }

    monitor->flag = 0;
}


static const int32_t COUNT_OF_LOOPS = 10;


static void
monitor_wait(monitor_t* monitor){
    do{
        CHECK_ERROR("", pthread_cond_wait(&(monitor->cond), &(monitor->mutex)));
    }while(!monitor->flag);
    monitor->flag = 0;
}


static void
monitor_signal(monitor_t* monitor){
    monitor->flag = 1;
    CHECK_ERROR("", pthread_cond_signal(&(monitor->cond)));
}


static void
monitor_lock(monitor_t* monitor){
    CHECK_ERROR("", pthread_mutex_lock(&(monitor->mutex)));
}


static void
monitor_unlock(monitor_t* monitor){
    CHECK_ERROR("", pthread_mutex_unlock(&(monitor->mutex)));
}


static void*
subroutine(void* context_){
    context_t* context = (context_t*) context_;
    monitor_t* my = context->my_monitor;
    monitor_t* other = context->neighbor_monitor;

    monitor_lock(my);
    monitor_signal(my);

    for(int32_t i=0; i<COUNT_OF_LOOPS; ++i) {
        monitor_wait(my);
        printf("%s\n", context->message);

        monitor_lock(other);
        monitor_signal(other);
        monitor_unlock(other);
    }
    monitor_unlock(my);

    return NULL;
}


int 
main(){
    monitor_t monitor1, monitor2;
    monitor_init(&monitor1);
    monitor_init(&monitor2);
    context_t context1 = {&monitor1, &monitor2, "+++++"};
    context_t context2 = {&monitor2, &monitor1, "-----"};

	pthread_t thread1, thread2;
    pthread_attr_t thread_attr;
    CHECK_ERROR("attr_init", pthread_attr_init(&thread_attr));
    CHECK_ERROR("setstacksize", pthread_attr_setstacksize(&thread_attr, PTHREAD_STACK_MIN));

    monitor_lock(&monitor1);
    monitor_lock(&monitor2);

    if(pthread_create(&thread1, &thread_attr, subroutine, (void*) &context1)==-1 ||
        pthread_create(&thread2, &thread_attr, subroutine, (void*) &context2)==-1){

        perror("Creating threads fail");
        return 0;
    }

    monitor_wait(&monitor1);
    monitor_wait(&monitor2);

    monitor_signal(&monitor1);
    monitor_unlock(&monitor2);
    monitor_unlock(&monitor1);

    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL); 

	return 0;
}
