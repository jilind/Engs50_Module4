/*
 * queue.c --- Implementation of a reusable queue
 *
 * Authors: Timur Tazeev, James Lind
 * Created: 10-10-2026
 *
 * Description: A program implementing the methods of a reusable
 * queue containing pointers to elements of any type
 *
 */

#include "queue.h"
#include <stdlib.h>

typedef struct Node {
    void *val;
    struct Node *next;
} Node;

typedef struct queue {
    struct Node *head;
    struct Node *tail;
} queue;

queue_t* qopen(void) {
    queue *q = malloc(sizeof(queue));
    
    if (q == NULL) {
        return NULL;
    }

    q->head = NULL;
    q->tail = NULL;

    return q;
}

void qclose(queue_t *qp) {
    queue *q = qp;

    if (q == NULL) {
        return;
    }
    Node *cur = q->head;
    
    while (cur != NULL) {
        Node *next = cur->next;
        free(cur);
        cur = next;
    }

    free(q);
}

int32_t qput(queue_t *qp, void *elementp) {
    queue *q = qp;

    if (q == NULL || elementp == NULL) {
        return -1;
    }

    Node *node = malloc(sizeof(Node));
    
    if (node == NULL) {
        return -1;
    }

    node->val = elementp;
    node->next = NULL;

    if (q->tail== NULL) {
        q->head = node;
    } else {
        q->tail->next = node;
    }
    q->tail = node;
    
    return 0;
}

void* qget(queue_t *qp) {
    queue *q = qp;
    if (q == NULL) {
        return NULL;
    }

    if (q->head == NULL) {
        return NULL;
    }

    Node *head = q->head;

    void *element = head->val;
    q->head = q->head->next;

    if (q->head == NULL) {
        q->tail = NULL;
    }
    free(head);

    return element;
}

void qapply(queue_t *qp, void (*fn)(void* elementp)) {
    queue *q = qp;
    if (q == NULL || fn == NULL) {
        return;
    }
    Node *cur = q->head;
    while (cur != NULL) {
        fn(cur->val);
        cur = cur->next;
    }
}

void* qsearch(queue_t *qp, bool (*searchfn)(void* elementp,const void* keyp), const void* skeyp) {
    queue *q = qp;
    if (q == NULL || searchfn == NULL) {
        return NULL;
    }

    for (Node *cur = q->head; cur != NULL; cur = cur->next) {
        if (searchfn(cur->val, skeyp)) {
            return cur->val;
        }
    }

    return NULL;
}

void* qremove(queue_t *qp, bool (*searchfn)(void* elementp,const void* keyp), const void* skeyp) {
    queue *q = qp;
    if (q == NULL || searchfn == NULL) {
        return NULL;
    }

    Node *cur = q->head;
    Node *prev = NULL;

    while (cur != NULL) {
        if (searchfn(cur->val, skeyp)) {
            if (prev == NULL) {
                q->head = cur->next;
            } else {
                prev->next = cur->next;
            }
            if (cur == q->tail) {
                q->tail = prev;
            }
            void *res = cur->val;
            free(cur);
            return res;
        }
        prev = cur;
        cur = cur->next;
    }

    return NULL;
}

void qconcat(queue_t *q1p, queue_t *q2p) {
    queue *q1 = q1p;
    queue *q2 = q2p;
    if (q1 == NULL || q2 == NULL || q1 == q2) {
        return;
    }
    if (q2->head != NULL) {
        if (q1->head == NULL) {
            q1->head = q2->head;
        }
        else {
            q1->tail->next = q2->head;
        }
        q1->tail = q2->tail;
    }
    free(q2);
}
