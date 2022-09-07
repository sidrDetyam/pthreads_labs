#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdint.h>
#include <limits.h>


struct Monitor{
    pthread_mutex_t mutex;
    pthread_cond_t cond;
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
    pthread_cond_init(&(monitor->cond), NULL);

	static pthread_mutexattr_t mutex_attr;
    pthread_mutexattr_init(&mutex_attr);
    //зачем нам тут еррор-мьютекс?
    pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_ERRORCHECK);
    pthread_mutex_init(&(monitor->mutex), &mutex_attr);
}


static const int32_t COUNT_OF_LOOPS = 10;


static void*
subroutine(void* context_){
    context_t* context = (context_t*) context_;
    pthread_mutex_t* my_mutex = &(context->my_monitor->mutex);
    pthread_cond_t* my_cond = &(context->my_monitor->cond);

    pthread_mutex_lock(my_mutex);
    pthread_cond_signal(my_cond);
      
    for(int32_t i=0; i<COUNT_OF_LOOPS; ++i) {
        pthread_cond_wait(my_cond, my_mutex);
        printf("%s\n", context->message);
        usleep(1000 * 500);
        pthread_cond_signal(&(context->neighbor_monitor->cond));
    }
    pthread_mutex_unlock(my_mutex);

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
    pthread_attr_init(&thread_attr);
    pthread_attr_setstacksize(&thread_attr, PTHREAD_STACK_MIN);

    pthread_mutex_lock(&(monitor1.mutex));
    pthread_mutex_lock(&(monitor2.mutex));

    if(pthread_create(&thread1, &thread_attr, subroutine, (void*) &context1)==-1 ||
        pthread_create(&thread2, &thread_attr, subroutine, (void*) &context2)==-1){

        perror("Creating threads fail");
        return 0;
    }

    pthread_cond_wait(&(monitor1.cond), &(monitor1.mutex));
    pthread_cond_wait(&(monitor2.cond), &(monitor2.mutex));
    pthread_mutex_unlock(&(monitor1.mutex));
    pthread_mutex_unlock(&(monitor2.mutex));
    pthread_cond_signal(&(monitor1.cond));

    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL); 
	return 0;
}
