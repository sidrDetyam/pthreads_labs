#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <limits.h>
#include "common.h"


struct Context{
    pthread_mutex_t* mutex;
    const char ** message;
};
typedef struct Context context_t;


static void*
subroutine(void* context_){
    context_t* context = (context_t*) context_;
      
    for(const char ** ptr = context->message; *ptr != NULL; ++ptr) {
        CEM("lock", pthread_mutex_lock(context->mutex));
        printf("%s\n", *ptr);
        CEM("unlock", pthread_mutex_unlock(context->mutex));
    }

    return NULL;
}


int 
main(){
    pthread_mutex_t mutex;
	static pthread_mutexattr_t mutex_attr;
    CEM("", pthread_mutexattr_init(&mutex_attr));
    CEM("", pthread_mutex_init(&mutex, &mutex_attr));

    const char* msg1[] = {"foo", "foo", "foo", "foo", "foo", "foo", "foo", NULL};
    const char* msg2[] = {"bar", "bar", "bar", "bar", NULL};

    context_t context1 = {&mutex, msg1};
    context_t context2 = {&mutex, msg2};

	pthread_t thread1, thread2;
    pthread_attr_t thread_attr;
    pthread_attr_init(&thread_attr);
    pthread_attr_setstacksize(&thread_attr, PTHREAD_STACK_MIN*2);

    CEM("", pthread_create(&thread1, &thread_attr, subroutine, (void*) &context1));
    CEM("", pthread_create(&thread2, &thread_attr, subroutine, (void*) &context2));
    CEM("", pthread_join(thread1, NULL));
    CEM("", pthread_join(thread2, NULL));
    CEM("", pthread_mutex_destroy(&mutex));

	return 0;
}
