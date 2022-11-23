
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <signal.h>

#include "../common.h"


struct ListNode {
    struct ListNode *next;
    pthread_mutex_t mutex;
    char *str;
};
typedef struct ListNode list_node_t;

enum ListStatus{
    FREE, SORTING, UPDATING
};

struct List {
    list_node_t term;
    size_t count;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int sorting_count;
    enum ListStatus status;
};
typedef struct List list_t;


static void
list_init(list_t *l) {
    CE(pthread_mutex_init(&l->mutex, NULL));
    CE(pthread_cond_init(&l->cond, NULL));
    CE(pthread_mutex_init(&l->term.mutex, NULL));
    l->term.next = NULL;
    l->count = 0;
    l->sorting_count = 0;
    l->status = FREE;
}

static void
list_print(list_t *l) {

    printf("===================================\n");
    CE(pthread_mutex_lock(&l->term.mutex));
    list_node_t *it = &l->term->ne;



    while(1){
        if(it->next != NULL){
            printf("%s\n", it->next->str);
            it = it->next;
            CE(pthread_mutex_lock(&it->next->mutex));
            CE(pthread_mutex_unlock(&it->mutex));
            continue;
        }
        break;
    };
    CE(pthread_mutex_unlock(&l->term.mutex));

    printf("===================================\n");

    CE(pthread_mutex_lock(&l->mutex));
    l->status = FREE;
    CE(pthread_cond_broadcast(&l->cond));
    CE(pthread_mutex_unlock(&l->mutex));
}


enum COMP_RES {
    LESS = -1,
    EQ,
    GREATER
};


enum COMP_RES
str_comp(const char *s1_, const char *s2_) {
    const unsigned char* s1 = (const unsigned char*) s1_;
    const unsigned char* s2 = (const unsigned char*) s2_;

    size_t i;
    for (i = 0; s1[i] != '\0' && s2[i] != '\0'; i++) {
        if (s1[i] > s2[i]) {
            return GREATER;
        }
        if (s2[i] > s1[i]) {
            return LESS;
        }
    }

    if (s1[i] == '\0' && s2[i] == '\0') {
        return EQ;
    }

    if (s1[i] != '\0') {
        return GREATER;
    }

    return LESS;
}


static void
sort_list(list_t *l) {
    CE(pthread_mutex_lock(&l->mutex));
    // Статический анализатор силайна умер?
    //while(l->status != FREE || l->status != SORTING){
    while(l->status == UPDATING){
        CE(pthread_cond_wait(&l->cond, &l->mutex));
    }
    //printf("reach!!!!\n");
    l->status = SORTING;
    ++l->sorting_count;
    CE(pthread_mutex_unlock(&l->mutex));

    for (size_t i = 0; i < l->count - 1; ++i) {
        CE(pthread_mutex_lock(&l->term.mutex));
        list_node_t *base = &l->term;
        CE(pthread_mutex_lock(&base->next->mutex));

        int is_end = 0;
        do{
            CE(pthread_mutex_lock(&base->next->next->mutex));
            list_node_t *first = base->next;
            list_node_t *second = first->next;

            if (str_comp(first->str, second->str) == GREATER) {
                list_node_t *tmp = second->next;
                base->next = second;
                second->next = first;
                first->next = tmp;
            }

            is_end = base->next->next == NULL;
            list_node_t *tmp = base->next;
            CE(pthread_mutex_unlock(&base->mutex));
            base = tmp;
        }while(is_end);

        CE(pthread_mutex_unlock(&base->next->mutex));
        CE(pthread_mutex_unlock(&base->mutex));
    }

    CE(pthread_mutex_lock(&l->mutex));
    --l->sorting_count;
    if(l->sorting_count == 0) {
        l->status = FREE;
    }
    CE(pthread_cond_broadcast(&l->cond));
    CE(pthread_mutex_unlock(&l->mutex));
}

static void
list_push_front(list_t *l, char *str) {
    CE(pthread_mutex_lock(&l->mutex));
    while(l->status != FREE){
        CE(pthread_cond_wait(&l->cond, &l->mutex));
    }
    l->status = UPDATING;
    CE(pthread_mutex_unlock(&l->mutex));

    list_node_t *new_node = malloc(sizeof(list_node_t));
    assert(new_node != NULL);
    new_node->next = l->term.next;
    new_node->str = str;
    CE(pthread_mutex_init(&new_node->mutex, NULL));
    l->term.next = new_node;
    ++l->count;

    CE(pthread_mutex_lock(&l->mutex));
    l->status = FREE;
    CE(pthread_cond_broadcast(&l->cond));
    CE(pthread_mutex_unlock(&l->mutex));
}


enum CONF {
    COUNT_OF_SORTING_THREADS = 3
};


struct Context{
    list_t *list;
    int sleep_duration;
};
typedef struct Context context_t;

_Noreturn static void *
subroutine(void *arg) {
    context_t* context = (context_t*) arg;

    while (1) {
        sort_list(context->list);
        printf("--- 1 %d\n", context->sleep_duration);
        sleep(context->sleep_duration);
        printf("--- 2 %d\n", context->sleep_duration);
    }
}

static void sigint_handler(int sig) {
    assert(SIGINT == sig);
    _exit(0);
}

int
main() {
    list_t list;
    list_init(&list);

    pthread_t sort_thread[COUNT_OF_SORTING_THREADS];
    context_t contexts[COUNT_OF_SORTING_THREADS];
    for(size_t i=0; i<COUNT_OF_SORTING_THREADS; ++i) {
        contexts[i].list = &list;
        contexts[i].sleep_duration = 2 * (int)i;
        CE(pthread_create(sort_thread+i, NULL, subroutine, contexts+i));
    }

    struct sigaction int_action;
    int_action.sa_handler = sigint_handler;
    CE(sigemptyset(&int_action.sa_mask));
    int_action.sa_flags = 0;
    CE(sigaction(SIGINT, &int_action, NULL));

    for (int i = 0; i < 10000; ++i) {
        char *buff = NULL;
        size_t cnt;
        assert(getline(&buff, &cnt, stdin) != -1);

        if (strlen(buff) == 1) {
            list_print(&list);
        } else {
            list_push_front(&list, buff);
        }
    }

    return 0;
}
