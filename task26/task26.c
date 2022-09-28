#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdint.h>
#include <limits.h>


typedef char* charptr_t;
#define ELEMENT_TYPE charptr_t
#include "blockingqueue_definition.h"
#define ELEMENT_TYPE charptr_t
#include "blockingqueue_implementation.h"
typedef bq_charptr_t_t queue;


static void 
mymsginit(queue *q){
    if(bq_charptr_t_init(q, 10)==-1){
        perror("init fail");
        exit(1);
    }
}


static void 
mymsqdrop(queue *q){
    if(bq_charptr_t_destroy(q)==-1){
        perror("drop fail");
        exit(1);
    }
}


static void 
mymsgdestroy(queue *q){

}


static int 
mymsgput(queue *q, char *msg){
    return bq_charptr_t_put(q, &msg);
}


static int 
mymsgget(queue *q, char *buf, size_t bufsize){
    
}



struct ProducerContext{
    bq_charptr_t_t* bq;
    char* text;
    int32_t count;
    pthread_mutex_t* mutex;
};
typedef struct ProducerContext producer_context_t;


struct ConsumerContext{
    bq_charptr_t_t* bq;
    pthread_mutex_t* mutex;
    int consumer_id;
};
typedef struct ConsumerContext consumer_context_t;


static void*
producer(void* arg){
    producer_context_t* context = (producer_context_t*) arg;

    for(int32_t i=0; i < context->count; ++i){
        bq_charptr_t_put(context->bq, &(context->text));

        pthread_mutex_lock(context->mutex);
        printf(" ++++++ %s put to queue\n", context->text);
        pthread_mutex_unlock(context->mutex);
        usleep( 500000 + 1000 * 100 * rand()%20);
    }

    return NULL;
}


enum {
    COUNT_OF_PRODUCERS = 5,
    COUNT_OF_PRODUCTS = 5,
    COUNT_OF_CONSUMERS = 5
};


static void*
consumer(void* arg){
    consumer_context_t* context = (consumer_context_t*) arg;

    for(int i=0; i<COUNT_OF_CONSUMERS; ++i){
        charptr_t ptr;
        bq_charptr_t_take(context->bq, &ptr);

        pthread_mutex_lock(context->mutex);
        printf(" ------ %s get by consumer %d\n", ptr, context->consumer_id);
        pthread_mutex_unlock(context->mutex);
        usleep( 1000000 + 1000 * 100 * rand()%40);
    }

    return NULL;
}


int 
main(){

    pthread_t producers_threads[COUNT_OF_PRODUCERS];
    pthread_t consumers_threads[COUNT_OF_CONSUMERS];
    pthread_attr_t thread_attr;
    pthread_attr_init(&thread_attr);
    pthread_attr_setstacksize(&thread_attr, PTHREAD_STACK_MIN);

    bq_charptr_t_t bq;
    if(bq_charptr_t_init(&bq, 10)==-1){
        perror("init fail");
    }

    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

    producer_context_t producers_context[COUNT_OF_PRODUCERS];
    for(int i=0; i<COUNT_OF_PRODUCERS; ++i){
        producers_context[i].bq = &bq;
        producers_context[i].count = COUNT_OF_PRODUCTS;
        producers_context[i].mutex = &mutex;
        producers_context[i].text = malloc(10);
        sprintf(producers_context[i].text, "%d", i);

        if(pthread_create(&producers_threads[i], &thread_attr, producer, (void*) &producers_context[i])==-1){
            perror("creation producer thread fail");
            exit(0);
        }
    }

    consumer_context_t consumers_context[COUNT_OF_CONSUMERS];
    for(int i=0; i<COUNT_OF_CONSUMERS; ++i){
        consumers_context[i].bq = &bq;
        consumers_context[i].mutex = &mutex;
        consumers_context[i].consumer_id = i;

        if(pthread_create(&consumers_threads[i], &thread_attr, consumer, (void*) &consumers_context[i])==-1){
            perror("creation cunsumer thread fail");
            exit(0);
        }
    }

    for(int i=0; i<COUNT_OF_PRODUCERS; ++i){
        pthread_join(producers_threads[i], NULL);
    }

    for(int i=0; i<COUNT_OF_CONSUMERS; ++i){
        pthread_join(consumers_threads[i], NULL);
    }    

    for(int i=0; i<COUNT_OF_PRODUCERS; ++i){
        free(producers_context[i].text);
    }
    bq_charptr_t_destroy(&bq);

	return 0;
}
