/*
 * indexer.c --- An indexer for the TSE
 *
 * Author: James I. Lind, Timur Tazeev
 * Created: 10-27-2026
 * Version: 1.0
 *
 * Description: Reads a crawled webpage and creates
 * a hash table with normalized words and their counts
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <limits.h>
#include <errno.h>
#include <sys/stat.h>
#include "webpage.h"
#include "pageio.h"
#include "indexio.h"

static size_t total_words = 0;

bool NormalizeWord(char *word) {
    if (strlen(word) < 3) {
        return false;
    }
    for (int i = 0; word[i] != '\0'; i++) {
        if (!isalpha((unsigned char)word[i])) {
            return false;
        }
        word[i] = (char)tolower((unsigned char)word[i]);
    }
    return true;
}

static bool same_word(void *element, const void *key) {
    wordentry_t *entry = element;
    return strcmp(entry->word, key) == 0;
}

static bool same_document(void *element, const void *key) {
    document_t *doc = element;
    return doc->id == *(const int *)key;
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

static bool add_word(hashtable_t *index, const char *word, int doc_id) {
    int len = (int)strlen(word);
    wordentry_t *entry = hsearch(index, same_word, word, len);
    bool is_new = (entry == NULL);

    if (is_new) {
        entry = malloc(sizeof(*entry));
        if (entry == NULL) {
            return false;
        }
        entry->word = malloc((size_t)len + 1);
        if (entry->word == NULL) {
            free(entry);
            return false;
        }
        strcpy(entry->word, word);
        entry->documents = qopen();
        if (entry->documents == NULL) {
            free(entry->word);
            free(entry);
            return false;
        }
    } else {
        document_t *doc = qsearch(entry->documents, same_document, &doc_id);
        if (doc != NULL) {
            doc->count++;
            return true;
        }
    }

    document_t *doc = malloc(sizeof(*doc));
    if (doc == NULL) {
        if (is_new) {
            free_word(entry);
        }
        return false;
    }
    doc->id = doc_id;
    doc->count = 1;

    if (qput(entry->documents, doc) != 0) {
        free(doc);
        if (is_new) {
            free_word(entry);
        }
        return false;
    }

    if (is_new && hput(index, entry, entry->word, len) != 0) {
        free_word(entry);
        return false;
    }
    return true;
}

static void sum_document(void *element) {
    document_t *doc = element;
    total_words += doc->count;
}

static void sum_word(void *element) {
    wordentry_t *entry = element;
    qapply(entry->documents, sum_document);
}

static size_t sumwords(hashtable_t *index) {
    total_words = 0;
    happly(index, sum_word);
    return total_words;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: indexer <pagedir> <indexnm>\n");
        return EXIT_FAILURE;
    }

    char *pagedir = argv[1];
    char *indexnm = argv[2];
    struct stat st;

    if (stat(pagedir, &st) != 0 || !S_ISDIR(st.st_mode)) {
        fprintf(stderr, "Invalid page directory: %s\n", pagedir);
        return EXIT_FAILURE;
    }

    hashtable_t *index = hopen(1000);
    if (index == NULL) {
        return EXIT_FAILURE;
    }

    bool success = true;
    int pages = 0;

    for (int doc_id = 1; doc_id < INT_MAX; doc_id++) {
        char filename[4096];
        int n = snprintf(filename, sizeof(filename), "%s/%d", pagedir, doc_id);

        if (n < 0 || (size_t)n >= sizeof(filename)) {
            success = false;
            break;
        }
        if (stat(filename, &st) != 0) {
            if (errno == ENOENT && doc_id > 1) {
                break;
            }
            success = false;
            break;
        }
        if (!S_ISREG(st.st_mode)) {
            success = false;
            break;
        }

        webpage_t *page = pageload(doc_id, pagedir);
        if (page == NULL) {
            fprintf(stderr, "Unable to load page %d\n", doc_id);
            success = false;
            break;
        }

        int pos = 0;
        char *word = NULL;

        while ((pos = webpage_getNextWord(page, pos, &word)) > 0) {
            if (NormalizeWord(word) && !add_word(index, word, doc_id)) {
                success = false;
                free(word);
                word = NULL;
                break;
            }
            free(word);
            word = NULL;
        }

        free(word);
        webpage_delete(page);

        if (!success) {
            break;
        }
        pages++;
    }

    if (pages == 0) {
        success = false;
    }
    if (success && indexsave(index, indexnm) != 0) {
        success = false;
    }

    if (success) {
        printf("Indexed %d pages. Total words: %zu\n", pages, sumwords(index));
    } else {
        fprintf(stderr, "Unable to build or save index\n");
    }

    happly(index, free_word);
    hclose(index);

    return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
