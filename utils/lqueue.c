/*
 * lqueue.c - Implementation of a reusable locked queue
 *
 * Authors: Timur Tazeev, James Lind
 * Created: 11-10-2026
 *
 * Description: A program implementing the methods of a
 * reusable locked queue
 */

#include <pthread.h>
#include <stdlib.h>
#include "lqueue.h"
#include "queue.h"

struct locked_queue {
    queue_t *queue;
    pthread_mutex_t mutex;
};

lqueue_t *lqopen(void) {
    lqueue_t *lqp = malloc(sizeof(*lqp));

    if (lqp == NULL) {
        return NULL;
    }

    lqp->queue = qopen();
    if (lqp->queue == NULL) {
        free(lqp);
        return NULL;
    }

    if (pthread_mutex_init(&lqp->mutex, NULL) != 0) {
        qclose(lqp->queue);
        free(lqp);
        return NULL;
    }
    return lqp;
}

void lqclose(lqueue_t *lqp) {
    if (lqp == NULL) {
        return;
    }
    qclose(lqp->queue);
    pthread_mutex_destroy(&lqp->mutex);
    free(lqp);
}

int32_t lqput(lqueue_t *lqp, void *elementp) {
    if (lqp == NULL || pthread_mutex_lock(&lqp->mutex) != 0) {
        return -1;
    }

    int32_t res = qput(lqp->queue, elementp);
    pthread_mutex_unlock(&lqp->mutex);
    return res;
}

void *lqget(lqueue_t *lqp) {
    if (lqp == NULL || pthread_mutex_lock(&lqp->mutex) != 0) {
        return NULL;
    }
    void *res = qget(lqp->queue);
    pthread_mutex_unlock(&lqp->mutex);
    return res;
}

void lqapply(lqueue_t *lqp, void (*fn)(void *elementp)) {
    if (lqp == NULL || fn == NULL || pthread_mutex_lock(&lqp->mutex) != 0) {
        return;
    }
    qapply(lqp->queue, fn);
    pthread_mutex_unlock(&lqp->mutex);
}

void *lqsearch(lqueue_t *lqp, bool (*searchfn)(void *elementp, const void *keyp), const void *skeyp) {
    if (lqp == NULL || searchfn == NULL || skeyp == NULL || pthread_mutex_lock(&lqp->mutex) != 0) {
        return NULL;
    }
    void *res = qsearch(lqp->queue, searchfn, skeyp);
    pthread_mutex_unlock(&lqp->mutex);
    return res;
}
