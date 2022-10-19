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
    int value;
};
typedef struct Monitor monitor_t;


struct Context{
    monitor_t* mon;
    const char* message;
    int my_val;
    int other_val;
};
typedef struct Context context_t;


static void 
monitor_init(monitor_t* monitor)
{
    CHECK_ERROR("", pthread_cond_init(&(monitor->cond), NULL));

	static pthread_mutexattr_t mutex_attr;
    CHECK_ERROR("", pthread_mutexattr_init(&mutex_attr));
    CHECK_ERROR("", pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_ERRORCHECK));
    CHECK_ERROR("", pthread_mutex_init(&(monitor->mutex), &mutex_attr));

    monitor->value = -1;
}


static const int32_t COUNT_OF_LOOPS = 10;


static void
monitor_wait(monitor_t* monitor, int expect)
{
    do{
        CHECK_ERROR("", pthread_cond_wait(&monitor->cond, &monitor->mutex));
        //printf("v: %d e: %d\n", monitor->value, expect);
        if(monitor->value != expect){
            CHECK_ERROR("", pthread_cond_signal(&monitor->cond));
        }
        else{
            break;
        }
    }while(1);
    monitor->value = -1;
}


static void
monitor_signal(monitor_t* monitor, int expect)
{
    monitor->value = expect;
    CHECK_ERROR("", pthread_cond_signal(&monitor->cond));
}


static void
monitor_lock(monitor_t* monitor)
{
    CHECK_ERROR("", pthread_mutex_lock(&(monitor->mutex)));
}


static void
monitor_unlock(monitor_t* monitor)
{
    CHECK_ERROR("", pthread_mutex_unlock(&(monitor->mutex)));
}


static void*
subroutine(void* context_)
{
    context_t* context = (context_t*) context_;
    monitor_t* mon = context->mon;

    monitor_lock(mon);
    monitor_signal(mon, 0);

    for(int32_t i=0; i<COUNT_OF_LOOPS; ++i) {
        monitor_wait(mon, context->my_val);
        printf("%s\n", context->message);

        monitor_signal(mon, context->other_val);
    }
    monitor_unlock(mon);

    return NULL;
}


int 
main()
{
    monitor_t monitor_;
    monitor_t* monitor = &monitor_;
    monitor_init(monitor);
    context_t context1 = {monitor, "+++++", 1, 2};
    context_t context2 = {monitor, "-----", 2, 1};

	pthread_t thread1, thread2;
    pthread_attr_t thread_attr;
    CHECK_ERROR("attr_init", pthread_attr_init(&thread_attr));
    CHECK_ERROR("setstacksize", pthread_attr_setstacksize(&thread_attr, PTHREAD_STACK_MIN));

    monitor_lock(monitor);

    CHECK_ERROR("", pthread_create(&thread1, &thread_attr, subroutine, (void*) &context1));
    monitor_wait(monitor, 0);

    CHECK_ERROR("", pthread_create(&thread2, &thread_attr, subroutine, (void*) &context2));
    monitor_wait(monitor, 0);

    monitor_signal(monitor, 1);
    monitor_unlock(monitor);

    CHECK_ERROR("", pthread_join(thread1, NULL));
    CHECK_ERROR("", pthread_join(thread2, NULL)); 

	return 0;
}
