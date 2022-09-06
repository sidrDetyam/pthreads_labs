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
};
typedef struct ConsumerContext consumer_context_t;


void*
producer(void* arg){
    producer_context_t* context = (producer_context_t*) arg;

    for(int32_t i=0; i < context->count; ++i){
        bq_charptr_t_put(context->bq, &(context->text));

        pthread_mutex_lock(context->mutex);
        printf("[%s] put to queue\n", context->text);
        pthread_mutex_unlock(context->mutex);
    }

    return NULL;
}


void*
consumer(void* arg){
    consumer_context_t* context = (consumer_context_t*) arg;

    while(1){
        charptr_t ptr;
        bq_charptr_t_take(context->bq, &ptr);

        pthread_mutex_lock(context->mutex);
        printf("[%s] get from queue\n", ptr);
        pthread_mutex_unlock(context->mutex);
        sleep(1);
    }

    //return NULL;
}


int 
main(){

    pthread_t producers_threads[2];
    pthread_t consumers[2];
    pthread_attr_t thread_attr;
    pthread_attr_init(&thread_attr);
    pthread_attr_setstacksize(&thread_attr, PTHREAD_STACK_MIN);

    bq_charptr_t_t bq;
    if(bq_charptr_t_init(&bq, 10)==-1){
        perror("init fail");
    }

    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    producer_context_t prod_context1 = {&bq, "--------", 1000, &mutex};
    producer_context_t prod_context2 = {&bq, "++++++++", 1000, &mutex};
    consumer_context_t cons_context = {&bq, &mutex};


    if(pthread_create(&producers_threads[0], &thread_attr, producer, (void*) &prod_context1)==-1 ||
        pthread_create(&producers_threads[1], &thread_attr, producer, (void*) &prod_context2)==-1 ||
        pthread_create(&consumers[0], &thread_attr, consumer, (void*) &cons_context)==-1 ){

        perror("Creating threads fail");
        return 0;
    }

    pthread_join(producers_threads[0], NULL);
    pthread_join(producers_threads[1], NULL); 
    //pthread_join(thr);
    bq_charptr_t_destroy(&bq);

	return 0;
}
