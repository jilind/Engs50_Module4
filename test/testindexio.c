/* 
 * testindexio.c --- test for indexio.c
 * 
 * Author: Timur Tazeev, James I. Lind
 * Created: 10-28-2026
 * Version: 1.0
 * 
 * Description: Test for indexio.c
 * 
 */
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "indexio.h"

static bool same_word(void *element, const void *key) {
    wordentry_t *entry = element;
    return strcmp(entry->word, key) == 0;
}

static bool same_document(void *element, const void *key) {
    document_t *doc = element;
    return doc->id == *(const int *)key;
}

static void add_count(hashtable_t *index, const char *word, int id, size_t count) {
    wordentry_t *entry = hsearch(index, same_word, word, (int)strlen(word));
    if (entry == NULL) {
        entry = malloc(sizeof(*entry));
        assert(entry != NULL);
        entry->word = malloc(strlen(word) + 1);
        assert(entry->word != NULL);
        strcpy(entry->word, word);
        entry->documents = qopen();
        assert(entry->documents != NULL);
        assert(hput(index, entry, entry->word, (int)strlen(word)) == 0);
    }
    document_t *doc = qsearch(entry->documents, same_document, &id);
    if (doc != NULL) {
        doc->count += count;
    } else {
        doc = malloc(sizeof(*doc));
        assert(doc != NULL);
        doc->id = id;
        doc->count = count;
        assert(qput(entry->documents, doc) == 0);
    }
}

static size_t get_count(hashtable_t *index, const char *word, int id) {
    wordentry_t *entry = hsearch(index, same_word, word, (int)strlen(word));
    if (entry == NULL) {
        return 0;
    }
    document_t *doc = qsearch(entry->documents, same_document, &id);
    return doc == NULL ? 0 : doc->count;
}

static void free_document(void *element) {
    free(element);
}

static void free_word(void *element) {
    wordentry_t *entry = element;
    qapply(entry->documents, free_document);
    qclose(entry->documents);
    free(entry->word);
    free(entry);
}

static void delete_index(hashtable_t *index) {
    happly(index, free_word);
    hclose(index);
}

int main(void) {
    hashtable_t *index = hopen(1000);
    assert(index != NULL);

    add_count(index, "hello", 1, 3);
    add_count(index, "hello", 2, 2);
    add_count(index, "world", 1, 4);
    add_count(index, "hello", 1, 1);

    assert(indexsave(index, "test.index") == 0);

    hashtable_t *loaded = indexload("test.index");
    assert(loaded != NULL);

    assert(get_count(loaded, "hello", 1) == 4);
    assert(get_count(loaded, "hello", 2) == 2);
    assert(get_count(loaded, "world", 1) == 4);
    assert(get_count(loaded, "world", 2) == 0);
    assert(get_count(loaded, "hello", 1) +
           get_count(loaded, "hello", 2) +
           get_count(loaded, "world", 1) == 10);

    delete_index(loaded);
    delete_index(index);

    printf("indexio test passed\n");
    return EXIT_SUCCESS;
}
