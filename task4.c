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
    const char* message;
};
typedef struct Context context_t;


static void*
subroutine(void* context_){
    context_t* context = (context_t*) context_;
      
    for(;;) {
        printf("%s\n", context->message);
        usleep(100);
    }
}


int 
main(){
    context_t context = {"child pthead"};

	pthread_t thread;
    pthread_attr_t thread_attr;
    pthread_attr_init(&thread_attr);
    pthread_attr_setstacksize(&thread_attr, PTHREAD_STACK_MIN*2);

    if(pthread_create(&thread, &thread_attr, subroutine, (void*) &context)==-1){
        perror("Creating threads fail");
        return 0;
    }
    sleep(2);
    pthread_cancel(thread);
    pthread_join(thread, NULL);

	return 0;
}
