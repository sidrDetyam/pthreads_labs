
int
CONCAT(BLOCKING_QUEUE_PREFIX, _take)(BLOCKING_QUEUE_TYPE* bq, ELEMENT_TYPE* item){

    if(pthread_mutex_lock(&(bq->mutex))==-1){
        return -1;
    }

    while(bq->count == 0){
        if(pthread_cond_wait(&(bq->not_empty), &(bq->mutex))==-1){
            return -1;
        }
        if(bq->is_destroyed){
            if(pthread_mutex_unlock(&(bq->mutex))==-1){
                return -1;
            }
            return 0;
        }
    }
    memcpy(item, bq->items + bq->take_index, sizeof(ELEMENT_TYPE));
    if(++bq->take_index == bq->capacity){
        bq->take_index = 0;
    }
    --bq->count;
    if(pthread_cond_signal(&(bq->not_full))==-1){
        return -1;
    }

    if(pthread_mutex_unlock(&(bq->mutex))==-1){
        return -1;
    }

    return 1;
}


int
CONCAT(BLOCKING_QUEUE_PREFIX, _put)(BLOCKING_QUEUE_TYPE* bq, ELEMENT_TYPE* item){
    
    if(pthread_mutex_lock(&(bq->mutex))==-1){
        return -1;
    }

    while(bq->count == bq->capacity){
        if(pthread_cond_wait(&(bq->not_full), &(bq->mutex))==-1){
            return -1;
        }

        if(bq->is_destroyed){
            if(pthread_mutex_unlock(&(bq->mutex))==-1){
                return -1;
            }
            return 0;
        }
    }
    memcpy(bq->items + bq->put_index, item, sizeof(ELEMENT_TYPE));
    if(++bq->put_index == bq->capacity){
        bq->put_index = 0;
    }
    ++bq->count;
    if(pthread_cond_signal(&(bq->not_empty))==-1){
        return -1;
    }

    if(pthread_mutex_unlock(&(bq->mutex))==-1){
        return -1;
    }

    return 1;
}


int
CONCAT(BLOCKING_QUEUE_PREFIX, _init)(BLOCKING_QUEUE_TYPE* bq, size_t capacity){
    
    bq->items = malloc(sizeof(ELEMENT_TYPE) * capacity);
    if(bq->items==NULL){
        return -1;
    }

    bq->is_destroyed = 0;
    bq->count = 0;
    bq->capacity = capacity;
    bq->take_index = 0;
    bq->put_index = 0;

    if(pthread_cond_init(&(bq->not_empty), NULL)==-1 ||
        pthread_cond_init(&(bq->not_full), NULL)==-1 ||
        pthread_mutex_init(&(bq->mutex), NULL)==-1){
        free(bq->items);
        return -1;
    }
    return 0;
}


int
CONCAT(BLOCKING_QUEUE_PREFIX, _destroy)(BLOCKING_QUEUE_TYPE* bq){
    if(bq->is_destroyed){
        return -1;
    }
    bq->is_destroyed = 1;

    if(pthread_cond_broadcast(&(bq->not_empty))==-1 || 
        pthread_cond_broadcast(&(bq->not_full))==-1){
        return -1;
    }
    free(bq->items);

    if(pthread_mutex_lock(&(bq->mutex))==-1 || 
        pthread_cond_destroy(&(bq->not_empty))==-1 ||
        pthread_cond_destroy(&(bq->not_full))==-1 ||
        pthread_mutex_unlock(&(bq->mutex))==-1 ||
        pthread_mutex_destroy(&(bq->mutex))==-1){
        return -1;
    }

    return 0;
}


#undef ELEMENT_TYPE
