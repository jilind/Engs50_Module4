/*
 * testlhash_singlet.c - Tests for the locked hash table (single-threaded)
 *
 * Authors: Timur Tazeev, James Lind
 * Created: 11-10-2026
 *
 * Description: Tests of the methods of a
 * reusable locked hash table
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lhash.h"

typedef struct {
    char key[16];
    int value;
} entry_t;

static int visited;
static int sum;

static bool comp(void *element, const void *key) {
    const entry_t *entry = element;
    return strcmp(entry->key, key) == 0;
}

static void sum_up(void *element) {
    const entry_t *entry = element;
    visited++;
    sum += entry->value;
}

int main(void) {
    lhashtable_t *table = lhopen(1);
    assert(table != NULL);

    entry_t entries[] = {{"first", 1}, {"second", 2}, {"third", 3}};
    char absent[] = "absent";
    assert(lhsearch(table, comp, absent, (int)strlen(absent)) == NULL);

    for (size_t i = 0; i < sizeof(entries) / sizeof(entries[0]); i++) {
        entry_t *entry = &entries[i];
        int keylen = (int)strlen(entry->key);
        int result = lhput(table, entry, entry->key, keylen);
        assert(result == 0);
        assert(lhsearch(table, comp, entry->key, keylen) == entry);
    }

    lhapply(table, sum_up);
    assert(visited == 3);
    assert(sum == 6);

    void *removed = lhremove(table, comp, entries[1].key, (int)strlen(entries[1].key));
    assert(removed == &entries[1]);
    assert(lhsearch(table, comp, entries[1].key, (int)strlen(entries[1].key)) == NULL);
    assert(lhremove(table, comp, entries[1].key, (int)strlen(entries[1].key)) == NULL);
    assert(lhsearch(table, comp, entries[0].key, (int)strlen(entries[0].key)) == &entries[0]);
    assert(lhsearch(table, comp, entries[2].key, (int)strlen(entries[2].key)) == &entries[2]);

    lhclose(table);
    puts("PASSED");
    return EXIT_SUCCESS;
}
