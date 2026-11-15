/*
 * testlqueue_doublet.c - Tests for the locked queue (double-threaded)
 *
 * Authors: Timur Tazeev, James Lind
 * Created: 11-10-2026
 *
 * Description: Tests of the methods of a
 * reusable locked queue for 2 threads
 */

#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include "lqueue.h"

#define THREADS 2
#define ITEMS_PER_THREAD 100000

typedef struct {
    lqueue_t *queue;
    int first_value;
    int values[ITEMS_PER_THREAD];
    int removed[ITEMS_PER_THREAD];
} worker_t;

static void *init_worker(void *args) {
    worker_t *worker = args;
    for (int i = 0; i < ITEMS_PER_THREAD; i++) {
        worker->values[i] = worker->first_value + i;
        if (lqput(worker->queue, &worker->values[i]) != 0) {
            return worker;
        }
    }
    for (int i = 0; i < ITEMS_PER_THREAD; i++) {
        int *value = lqget(worker->queue);
        if (value == NULL) {
            return worker;
        }
        worker->removed[i] = *value;
    }
    return NULL;
}

int main(void) {
    lqueue_t *queue = lqopen();
    assert(queue != NULL);

    pthread_t threads[THREADS];
    worker_t workers[THREADS];
    for (int i = 0; i < THREADS; i++) {
        workers[i].queue = queue;
        workers[i].first_value = i * ITEMS_PER_THREAD;
        int result = pthread_create(&threads[i], NULL, init_worker, &workers[i]);
        assert(result == 0);
    }
    for (int i = 0; i < THREADS; i++) {
        void *worker_result = NULL;
        int res = pthread_join(threads[i], &worker_result);
        assert(res == 0);
        assert(worker_result == NULL);
    }

    bool seen[THREADS * ITEMS_PER_THREAD] = {false};
    for (int i = 0; i < THREADS; i++) {
        for (int j = 0; j < ITEMS_PER_THREAD; j++) {
            int value = workers[i].removed[j];
            assert(value >= 0 && value < THREADS * ITEMS_PER_THREAD);
            assert(!seen[value]);
            seen[value] = true;
        }
    }
    assert(lqget(queue) == NULL);
    lqclose(queue);

    puts("PASSED");
    return EXIT_SUCCESS;
}
