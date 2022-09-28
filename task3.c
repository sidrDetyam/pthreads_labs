#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdint.h>
#include <limits.h>


struct Context{
    pthread_mutex_t* mutex;
    const char const** message;
};
typedef struct Context context_t;


static void*
subroutine(void* context_){
    context_t* context = (context_t*) context_;
      
    for(const char const** ptr = context->message; *ptr != NULL; ++ptr) {
        if(pthread_mutex_lock(context->mutex)==-1){
            perror("mutex lock error!");
            exit(1);
        }
        printf("%s\n", *ptr);
        if(pthread_mutex_unlock(context->mutex)==-1){
            perror("mutex unlock error!");
            exit(1);
        }
    }

    return NULL;
}


int 
main(){
    pthread_mutex_t mutex;
	static pthread_mutexattr_t mutex_attr;
    if(pthread_mutexattr_init(&mutex_attr)==-1){
        perror("attr init fail");
        exit(1);
    }
    if(pthread_mutex_init(&mutex, &mutex_attr)==-1){
        perror("mutex init fail");
        exit(1);
    }

    const char* msg1[] = {"foo", "foo", "foo", "foo", "foo", "foo", "foo", NULL};
    const char* msg2[] = {"bar", "bar", "bar", "bar", NULL};

    context_t context1 = {&mutex, msg1};
    context_t context2 = {&mutex, msg2};

	pthread_t thread1, thread2;
    pthread_attr_t thread_attr;
    pthread_attr_init(&thread_attr);
    pthread_attr_setstacksize(&thread_attr, PTHREAD_STACK_MIN*2);

    if(pthread_create(&thread1, &thread_attr, subroutine, (void*) &context1)==-1 ||
        pthread_create(&thread2, &thread_attr, subroutine, (void*) &context2)==-1){
        perror("Creating threads fail");
        return 0;
    }

    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL); 
	return 0;
}
