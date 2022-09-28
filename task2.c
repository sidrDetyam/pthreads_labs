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
    const char * message;
};
typedef struct Context context_t;

static const int32_t COUNT_OF_LOOPS = 10;

static void*
subroutine(void* context_){
    context_t* context = (context_t*) context_;
      
    for(int32_t i=0; i<COUNT_OF_LOOPS; ++i) {
        printf("%s\n", context->message);
    }
    return NULL;
}


int 
main(){
    context_t context = {"+++++"};

	pthread_t thread;
    pthread_attr_t thread_attr;
    pthread_attr_init(&thread_attr);
    pthread_attr_setstacksize(&thread_attr, PTHREAD_STACK_MIN*2);

    if(pthread_create(&thread, &thread_attr, subroutine, (void*) &context)==-1){
        perror("Creating threads fail");
        return 0;
    }

    if(pthread_join(thread, NULL)==-1){
        perror("join fail");
        exit(1);
    }
    context.message = "-------";
    subroutine(&context);

	return 0;
}
