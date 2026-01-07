#include "priority_queue.h"
#include <stdlib.h>

/* aging: +1 prioritate la fiecare 5 secunde de asteptare */
int effective_priority(RequestNode *r) {
    int wait = (int)(time(NULL) - r->enqueue_time);
    return r->base_priority + wait / 5;
}

void pq_init(PriorityQueue *q) {
    q->head = NULL;
    pthread_mutex_init(&q->mutex, NULL);
    pthread_cond_init(&q->cond, NULL);
}

void pq_push(PriorityQueue *q, RequestNode *node) {
    pthread_mutex_lock(&q->mutex);

    if (!q->head ||
        effective_priority(node) > effective_priority(q->head)) {

        node->next = q->head;
        q->head = node;

    } else {
        RequestNode *cur = q->head;
        while (cur->next &&
               effective_priority(cur->next) >= effective_priority(node)) {
            cur = cur->next;
        }
        node->next = cur->next;
        cur->next = node;
    }

    pthread_cond_signal(&q->cond);
    pthread_mutex_unlock(&q->mutex);
}

RequestNode *pq_pop(PriorityQueue *q) {
    pthread_mutex_lock(&q->mutex);

    while (!q->head)
        pthread_cond_wait(&q->cond, &q->mutex);

    RequestNode *node = q->head;
    q->head = node->next;

    pthread_mutex_unlock(&q->mutex);
    return node;
}
