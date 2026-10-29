/*
 * indexio.c
 *
 * Author: James I. Lind, Timur Tazeev
 * Created: 10-28-2026
 * Version: 1.0
 *
 * Description: indexio file
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "indexio.h"

static FILE *output;
static int write_error;

static void write_document(void *element) {
    document_t *doc = element;
    if (fprintf(output, " %d %zu", doc->id, doc->count) < 0) {
        write_error = 1;
    }
}

static void write_word(void *element) {
    wordentry_t *entry = element;
    if (fprintf(output, "%s", entry->word) < 0) {
        write_error = 1;
    }
    qapply(entry->documents, write_document);
    if (fputc('\n', output) == EOF) {
        write_error = 1;
    }
}

int indexsave(hashtable_t *index, const char *filename) {
    if (index == NULL || filename == NULL) {
        return -1;
    }
    output = fopen(filename, "w");
    if (output == NULL) {
        return -1;
    }
    write_error = 0;
    happly(index, write_word);
    if (fclose(output) != 0) {
        write_error = 1;
    }
    output = NULL;
    return write_error ? -1 : 0;
}

static void free_document(void *element) {
    free(element);
}

static void free_word(void *element) {
    wordentry_t *entry = element;
    if (entry->documents != NULL) {
        qapply(entry->documents, free_document);
        qclose(entry->documents);
    }
    free(entry->word);
    free(entry);
}

hashtable_t *indexload(const char *filename) {
    if (filename == NULL) {
        return NULL;
    }
    FILE *fp = fopen(filename, "r");
    if (fp == NULL) {
        return NULL;
    }
    hashtable_t *index = hopen(1000);
    if (index == NULL) {
        fclose(fp);
        return NULL;
    }

    char line[8192];
    bool success = true;

    while (success && fgets(line, sizeof(line), fp) != NULL) {
        if (strchr(line, '\n') == NULL && !feof(fp)) {
            success = false;
            break;
        }

        char *word = strtok(line, " \t\r\n");
        if (word == NULL) {
            success = false;
            break;
        }

        wordentry_t *entry = calloc(1, sizeof(*entry));
        if (entry == NULL) {
            success = false;
            break;
        }

        entry->word = malloc(strlen(word) + 1);
        entry->documents = qopen();
        if (entry->word == NULL || entry->documents == NULL) {
            free_word(entry);
            success = false;
            break;
        }
        strcpy(entry->word, word);

        bool found = false;
        char *id_text;

        while ((id_text = strtok(NULL, " \t\r\n")) != NULL) {
            char *count_text = strtok(NULL, " \t\r\n");
            if (count_text == NULL) {
                success = false;
                break;
            }

            document_t *doc = malloc(sizeof(*doc));
            if (doc == NULL) {
                success = false;
                break;
            }

            if (sscanf(id_text, "%d", &doc->id) != 1 ||
                sscanf(count_text, "%zu", &doc->count) != 1 ||
                doc->id < 1 || doc->count == 0 ||
                qput(entry->documents, doc) != 0) {
                free(doc);
                success = false;
                break;
            }
            found = true;
        }

        if (!found) {
            success = false;
        }

        if (success && hput(index, entry, entry->word, (int)strlen(entry->word)) != 0) {
            success = false;
        }

        if (!success) {
            free_word(entry);
            break;
        }
    }

    if (ferror(fp)) {
        success = false;
    }
    if (fclose(fp) != 0) {
        success = false;
    }

    if (!success) {
        happly(index, free_word);
        hclose(index);
        return NULL;
    }
    return index;
}
