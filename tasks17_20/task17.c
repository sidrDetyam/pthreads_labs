
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
    char *str;
};
typedef struct ListNode list_node_t;

struct List {
    list_node_t term;
    size_t count;
    pthread_mutex_t mutex;
};
typedef struct List list_t;


static void
list_init(list_t *l) {
    CE(pthread_mutex_init(&l->mutex, NULL));
    l->term.next = NULL;
    l->count = 0;
}

static void
list_print(list_t *l) {
    CE(pthread_mutex_lock(&l->mutex));

    printf("===================================\n");
    for (list_node_t *it = l->term.next; it != NULL; it = it->next) {
        printf("%s\n", it->str);
    }
    printf("===================================\n");

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

    for (size_t i = 0; i < l->count; ++i) {
        for (list_node_t **it = &l->term.next; *it != NULL && (*it)->next != NULL; it = &((*it)->next->next)) {
            if (str_comp((*it)->str, (*it)->next->str) == GREATER) {
                list_node_t *tmp = *it;
                *it = (*it)->next;
                ((*it)->next->next) = tmp;

//                char *tmp = it->str;
//                it->str = it->next->str;
//                it->next->str = tmp;
            }
        }
    }

    CE(pthread_mutex_unlock(&l->mutex));
}

static void
list_push_front(list_t *l, char *str) {
    CE(pthread_mutex_lock(&l->mutex));

    list_node_t *new_node = malloc(sizeof(list_node_t));
    assert(new_node != NULL);
    new_node->next = l->term.next;
    new_node->str = str;
    l->term.next = new_node;
    ++l->count;

    CE(pthread_mutex_unlock(&l->mutex));
}


enum CONF {
    SLEEP_DURATION = 5
};


_Noreturn static void *
subroutine(void *arg) {
    list_t *list = (list_t *) arg;

    while (1) {
        sort_list(list);
        sleep(SLEEP_DURATION);
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

    pthread_t sort_thread;
    CE(pthread_create(&sort_thread, NULL, subroutine, &list));

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
