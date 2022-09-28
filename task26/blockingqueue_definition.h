
#include<stdlib.h>
#include<stdio.h>
#include<errno.h>
#include<string.h>
#include<assert.h>
#include<pthread.h>


#define CONCAT(a,b) CONCAT_V(a,b)
#define CONCAT_V(a,b) a##b

#define BLOCKING_QUEUE_PREFIX CONCAT(bq_, ELEMENT_TYPE)
#define BLOCKING_QUEUE_TYPE CONCAT(BLOCKING_QUEUE_PREFIX, _t)


struct BLOCKING_QUEUE_TYPE{
    ELEMENT_TYPE* items;
    
    int is_destroyed;
    size_t count;
    size_t capacity;
    size_t take_index;
    size_t put_index;

    pthread_mutex_t mutex;
    pthread_cond_t not_full;
    pthread_cond_t not_empty;
};
typedef struct BLOCKING_QUEUE_TYPE BLOCKING_QUEUE_TYPE;


int
CONCAT(BLOCKING_QUEUE_PREFIX, _take)(BLOCKING_QUEUE_TYPE* bq, ELEMENT_TYPE* item);

int
CONCAT(BLOCKING_QUEUE_PREFIX, _put)(BLOCKING_QUEUE_TYPE* bq, ELEMENT_TYPE* item);

int
CONCAT(BLOCKING_QUEUE_PREFIX, _init)(BLOCKING_QUEUE_TYPE* bq, size_t capacity);

int
CONCAT(BLOCKING_QUEUE_PREFIX, _destroy)(BLOCKING_QUEUE_TYPE* bq);

#undef ELEMENT_TYPE
