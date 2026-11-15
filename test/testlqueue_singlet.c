/*
 * testlqueue_singlet.c - Tests for the locked queue (single-threaded)
 *
 * Authors: Timur Tazeev, James Lind
 * Created: 11-10-2026
 *
 * Description: Tests of the methods of a
 * reusable locked queue
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "lqueue.h"

static int visited;
static int sum;

static bool comp(void *element, const void *key) {
    return *(const int *)element == *(const int *)key;
}

static void sum_up(void *element) {
    visited++;
    sum += *(const int *)element;
}

int main(void) {
    lqueue_t *queue = lqopen();
    assert(queue != NULL);
    assert(lqget(queue) == NULL);

    int values[] = {1, 2, 3, 4, 5};
    for (size_t i = 0; i < sizeof(values) / sizeof(values[0]); i++) {
        int result = lqput(queue, &values[i]);
        assert(result == 0);
    }

    int key = 2;
    assert(lqsearch(queue, comp, &key) == &values[1]);
    key = 999;
    assert(lqsearch(queue, comp, &key) == NULL);

    lqapply(queue, sum_up);
    assert(visited == 5);
    assert(sum == 15);

    assert(lqget(queue) == &values[0]);
    assert(lqget(queue) == &values[1]);
    assert(lqget(queue) == &values[2]);
    assert(lqget(queue) == &values[3]);
    assert(lqget(queue) == &values[4]);
    assert(lqget(queue) == NULL);
    lqclose(queue);

    puts("PASSED");
    return EXIT_SUCCESS;
}
