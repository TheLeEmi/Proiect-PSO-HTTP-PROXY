#ifndef PRIORITY_QUEUE_H
#define PRIORITY_QUEUE_H

#include <pthread.h>
#include <time.h>

typedef struct RequestNode {
    int client_sock;
    int server_sock;

    int base_priority;      // prioritatea initiala
    time_t enqueue_time;    // momentul intrarii in coada (pentru aging)

    char request[4096];
    struct RequestNode *next;
} RequestNode;

typedef struct {
    RequestNode *head;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} PriorityQueue;

void pq_init(PriorityQueue *q);
void pq_push(PriorityQueue *q, RequestNode *node);
RequestNode *pq_pop(PriorityQueue *q);

/* calcul prioritate cu aging */
int effective_priority(RequestNode *r);

#endif
