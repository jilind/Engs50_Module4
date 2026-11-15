/*
 * testlhash_doublet.c - Tests for the locked hash table (double-threaded)
 *
 * Authors: Timur Tazeev, James Lind
 * Created: 11-10-2026
 *
 * Description: Tests of the methods of a
 * reusable locked hash table for 2 threads
 */

#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lhash.h"

#define THREADS 2
#define ITEMS_PER_THREAD 1000

typedef struct {
    char key[24];
    int keylen;
} entry_t;

typedef struct {
    lhashtable_t *table;
    int first_id;
    entry_t entries[ITEMS_PER_THREAD];
} worker_t;

static size_t visited;

static bool same_key(void *element, const void *key) {
    const entry_t *entry = element;
    return strcmp(entry->key, key) == 0;
}

static void count_entry(void *element) {
    (void)element;
    visited++;
}

static void *run_worker(void *argument) {
    worker_t *worker = argument;
    for (int i = 0; i < ITEMS_PER_THREAD; i++) {
        entry_t *entry = &worker->entries[i];
        entry->keylen = snprintf(entry->key, sizeof(entry->key), "key%d", worker->first_id + i);
        if (lhput(worker->table, entry, entry->key, entry->keylen) != 0) {
            return worker;
        }
    }
    for (int i = 0; i < ITEMS_PER_THREAD; i++) {
        entry_t *entry = &worker->entries[i];
        if (lhsearch(worker->table, same_key, entry->key, entry->keylen) != entry) {
            return worker;
        }
        if (i % 2 == 0 &&
            lhremove(worker->table, same_key, entry->key, entry->keylen) != entry) {
            return worker;
        }
    }
    return NULL;
}

int main(void) {
    lhashtable_t *table = lhopen(1);
    assert(table != NULL);

    pthread_t threads[THREADS];
    worker_t workers[THREADS];
    for (int i = 0; i < THREADS; i++) {
        workers[i].table = table;
        workers[i].first_id = i * ITEMS_PER_THREAD;
        int result = pthread_create(&threads[i], NULL, run_worker, &workers[i]);
        assert(result == 0);
    }
    for (int i = 0; i < THREADS; i++) {
        void *worker_result = NULL;
        int result = pthread_join(threads[i], &worker_result);
        assert(result == 0);
        assert(worker_result == NULL);
    }

    for (int i = 0; i < THREADS; i++) {
        for (int j = 0; j < ITEMS_PER_THREAD; j++) {
            entry_t *entry = &workers[i].entries[j];
            void *expected = j % 2 == 0 ? NULL : entry;
            assert(lhsearch(table, same_key, entry->key, entry->keylen) == expected);
        }
    }
    lhapply(table, count_entry);
    assert(visited == THREADS * (ITEMS_PER_THREAD / 2));

    lhclose(table);
    puts("PASSED");
    return EXIT_SUCCESS;
}
