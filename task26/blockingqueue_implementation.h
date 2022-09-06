
void
CONCAT(BLOCKING_QUEUE_PREFIX, _take)(BLOCKING_QUEUE_TYPE* bq, ELEMENT_TYPE* item){

    pthread_mutex_lock(&(bq->mutex));

    while(bq->count == 0){
        pthread_cond_wait(&(bq->not_empty), &(bq->mutex));
    }
    memcpy(item, bq->items + bq->take_index, sizeof(ELEMENT_TYPE));
    if(++bq->take_index == bq->capacity){
        bq->take_index = 0;
    }
    --bq->count;
    pthread_cond_signal(&(bq->not_full));

    pthread_mutex_unlock(&(bq->mutex));
}


void
CONCAT(BLOCKING_QUEUE_PREFIX, _put)(BLOCKING_QUEUE_TYPE* bq, ELEMENT_TYPE* item){
    
    pthread_mutex_lock(&(bq->mutex));

    while(bq->count == bq->capacity){
        pthread_cond_wait(&(bq->not_full), &(bq->mutex));
    }
    memcpy(bq->items + bq->put_index, item, sizeof(ELEMENT_TYPE));
    if(++bq->put_index == bq->capacity){
        bq->put_index = 0;
    }
    ++bq->count;
    pthread_cond_signal(&(bq->not_empty));

    pthread_mutex_unlock(&(bq->mutex));
}


int
CONCAT(BLOCKING_QUEUE_PREFIX, _init)(BLOCKING_QUEUE_TYPE* bq, size_t capacity){
    
    bq->items = malloc(sizeof(ELEMENT_TYPE) * capacity);
    if(bq->items==NULL){
        return -1;
    }

    bq->count = 0;
    bq->capacity = capacity;
    bq->take_index = 0;
    bq->put_index = 0;

    pthread_cond_init(&(bq->not_empty), NULL);
    pthread_cond_init(&(bq->not_full), NULL);
    pthread_mutex_init(&(bq->mutex), NULL);  
}


void
CONCAT(BLOCKING_QUEUE_PREFIX, _destroy)(BLOCKING_QUEUE_TYPE* bq){
    free(bq->items);
}


#undef ELEMENT_TYPE
