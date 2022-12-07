//
// Created by argem on 07.12.2022.
//

#include "hash_map.h"
#include "common.h"

#define ELEMENT_TYPE pair_t;
#include "cvector_impl.h"

void
hash_map_init(hash_map_t* hm, size_t ks, size_t vs, int (*hash)(void*), int (*equals)(void*, void*)){
    hm->key_size = ks;
    hm->value_size = vs;
    hm->equals = equals;
    hm->hash = hash;
    hm->buckets = malloc(sizeof(void *) * BUCKETS_SIZE);
    ASSERT(hm->buckets != NULL);
    memset(hm->buckets, 0, sizeof(void *) * BUCKETS_SIZE);
}


void*
hash_map_put(hash_map_t* hm, void* key, void* value){
    int hash = hm->hash(key);
    size_t ind = ABS(hash) % BUCKETS_SIZE;
    if(hm->buckets[ind] == NULL){
        hm->buckets[ind] = malloc(sizeof(vpair_t));
        vpair_t_init(hm->buckets[ind]);
    }
    vpair_t* bucket = hm->buckets[ind];

    for(size_t i=0; i<bucket->cnt; ++i){
        pair_t* p = vpair_t_get(bucket, i);
        if(hm->equals(p->key, key)){
            void* ret = p->value;
            memcpy(p->key, key, hm->key_size);
            memcpy(p->value, value, hm->value_size);
            return ret;
        }
    }
    pair_t pair;
    pair.key = key;
    pair.value = value;
    vpair_t_push_back(bucket, &pair);
    return NULL;
}

void*
hash_map_get(hash_map_t* hm, void* key){
    int hash = hm->hash(key);
    size_t ind = ABS(hash) % BUCKETS_SIZE;
    if(hm->buckets[ind] == NULL){
        return NULL;
    }
    vpair_t* bucket = hm->buckets[ind];

    for(size_t i=0; i<bucket->cnt; ++i){
        pair_t* p = vpair_t_get(bucket, i);
        if(hm->equals(p->key, key)){
            return p->value;
        }
    }
    return NULL;
}
