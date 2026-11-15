/*
 * query.c --- Querier for TSE
 *
 * Authors: James I. Lind, Timur Tazeev
 * Created: 11-03-2026
 * Version: 3.0
 *
 * Description: Querier module for the tiny search engine
 */

#define _POSIX_C_SOURCE 200809L
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "indexio.h"

typedef struct {
    int id;
    size_t rank;
} match_t;

static bool valid_page_directory(const char *dir) {
    struct stat st;
    if (stat(dir, &st) != 0 || !S_ISDIR(st.st_mode) || access(dir, R_OK | X_OK) != 0) {
        return false;
    }
    char filename[4096];
    int n = snprintf(filename, sizeof(filename), "%s/1", dir);
    if (n < 0 || (size_t)n >= sizeof(filename)) {
        return false;
    }
    FILE *fp = fopen(filename, "r");
    if (fp == NULL) {
        return false;
    }
    char url[4096];
    int depth;
    size_t html_length;
    bool valid = fgets(url, sizeof(url), fp) != NULL &&
                 url[0] != '\n' && url[0] != '\r' &&
                 strchr(url, '\n') != NULL &&
                 fscanf(fp, "%d %zu", &depth, &html_length) == 2 && depth >= 0;
    fclose(fp);
    return valid;
}

static int split_query(char *line, char ***out, size_t *nwords) {
    *out = NULL;
    *nwords = 0;
    for (char *p = line; *p != '\0'; p++) {
        unsigned char ch = (unsigned char)*p;
        if (!isalpha(ch) && !isspace(ch)) {
            return 0;
        }
        *p = (char)tolower(ch);
    }
    size_t slots = strlen(line) / 2 + 1;
    char **words = calloc(slots, sizeof(*words));
    if (words == NULL) {
        return -1;
    }
    for (char *word = strtok(line, " \t\r\n\v\f"); word != NULL; word = strtok(NULL, " \t\r\n\v\f")) {
        words[(*nwords)++] = word;
    }
    *out = words;
    return 1;
}

static bool valid_syntax(char **words, size_t nwords) {
    if (nwords == 0) {
        return true;
    }
    bool need_word = true;
    for (size_t i = 0; i < nwords; i++) {
        bool op = strcmp(words[i], "and") == 0 || strcmp(words[i], "or") == 0;
        if (op) {
            if (need_word) {
                return false;
            }
            need_word = true;
        } else {
            need_word = false;
        }
    }
    return !need_word;
}

static bool same_word(void *element, const void *key) {
    wordentry_t *entry = element;
    return strcmp(entry->word, key) == 0;
}

static bool same_document(void *element, const void *key) {
    document_t *doc = element;
    return doc->id == *(const int *)key;
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

static bool same_match(void *element, const void *key) {
    match_t *match = element;
    return match->id == *(const int *)key;
}

static queue_t *candidate_queue;
static bool candidate_ok;

static void collect_candidate(void *element) {
    document_t *doc = element;
    if (!candidate_ok || qsearch(candidate_queue, same_match, &doc->id) != NULL) {
        return;
    }
    match_t *match = malloc(sizeof(*match));
    if (match == NULL) {
        candidate_ok = false;
        return;
    }
    match->id = doc->id;
    match->rank = 0;
    if (qput(candidate_queue, match) != 0) {
        free(match);
        candidate_ok = false;
    }
}

static size_t rank_document(hashtable_t *index, char **words, size_t nwords, int id) {
    size_t rank = 0;
    size_t group = (size_t)-1;
    bool has_word = false;
    for (size_t i = 0; i < nwords; i++) {
        if (strcmp(words[i], "or") == 0) {
            if (has_word) {
                rank += group;
            }
            group = (size_t)-1;
            has_word = false;
        } else if (strcmp(words[i], "and") != 0 && strlen(words[i]) >= 3) {
            size_t count = get_count(index, words[i], id);
            if (count < group) {
                group = count;
            }
            has_word = true;
        }
    }
    if (has_word) {
        rank += group;
    }
    return rank;
}

static int compare_matches(const void *a, const void *b) {
    const match_t *x = *(const match_t *const *)a;
    const match_t *y = *(const match_t *const *)b;
    if (x->rank != y->rank) {
        return x->rank > y->rank ? -1 : 1;
    }
    return (x->id > y->id) - (x->id < y->id);
}

static bool print_match(const match_t *match, const char *dir) {
    char filename[4096];
    int n = snprintf(filename, sizeof(filename), "%s/%d", dir, match->id);
    if (n < 0 || (size_t)n >= sizeof(filename)) {
        return false;
    }
    FILE *fp = fopen(filename, "r");
    if (fp == NULL) {
        return false;
    }
    char url[4096];
    bool ok = fgets(url, sizeof(url), fp) != NULL;
    fclose(fp);
    if (!ok) {
        return false;
    }
    url[strcspn(url, "\r\n")] = '\0';
    printf("rank: %zu: doc: %d : %s\n", match->rank, match->id, url);
    return true;
}

static bool search_documents(hashtable_t *index, char **words, size_t nwords, const char *dir) {
    queue_t *matches = qopen();
    if (matches == NULL) {
        return false;
    }
    candidate_queue = matches;
    candidate_ok = true;

    for (size_t i = 0; i < nwords && candidate_ok; i++) {
        if (strlen(words[i]) < 3 || strcmp(words[i], "and") == 0 || strcmp(words[i], "or") == 0) {
            continue;
        }
        wordentry_t *entry = hsearch(index, same_word, words[i], (int)strlen(words[i]));
        if (entry != NULL) {
            qapply(entry->documents, collect_candidate);
        }
    }

    candidate_queue = NULL;
    if (!candidate_ok) {
        qapply(matches, free);
        qclose(matches);
        return false;
    }

    match_t **sorted = NULL;
    size_t found = 0;
    size_t capacity = 0;
    bool success = true;
    match_t *match;

    while ((match = qget(matches)) != NULL) {
        match->rank = rank_document(index, words, nwords, match->id);
        if (match->rank == 0) {
            free(match);
            continue;
        }
        if (found == capacity) {
            size_t next = capacity == 0 ? 8 : capacity * 2;
            match_t **grown = realloc(sorted, next * sizeof(*sorted));
            if (grown == NULL) {
                free(match);
                success = false;
                break;
            }
            sorted = grown;
            capacity = next;
        }
        sorted[found++] = match;
    }

    if (!success) {
        qapply(matches, free);
    }
    qclose(matches);

    if (success && found > 1) {
        qsort(sorted, found, sizeof(*sorted), compare_matches);
    }
		if (success && found == 0) {
			printf("No documents match.\n");
		}
    if (success) {
        for (size_t i = 0; i < found; i++) {
            if (!print_match(sorted[i], dir)) {
                fprintf(stderr, "Skipping document %d: URL not readable\n", sorted[i]->id);
            }
        }
    }

    for (size_t i = 0; i < found; i++) {
        free(sorted[i]);
    }
    free(sorted);
    return success;
}

int main(int argc, char *argv[]) {
    if (argc != 3 && argc != 4) {
        fprintf(stderr, "usage: %s <pageDirectory> <indexFile> [-q]\n", argv[0]);
        return EXIT_FAILURE;
    }

    bool quiet = false;
    if (argc == 4) {
        if (strcmp(argv[3], "-q") != 0) {
            fprintf(stderr, "Unknown option: %s\n", argv[3]);
            return EXIT_FAILURE;
        }
        quiet = true;
    }

    const char *dir = argv[1];
    const char *index_file = argv[2];

    if (!valid_page_directory(dir)) {
        fprintf(stderr, "Invalid or uncrawled page directory: %s\n", dir);
        return EXIT_FAILURE;
    }

    hashtable_t *index = indexload(index_file);
    if (index == NULL) {
        fprintf(stderr, "Unable to load %s\n", index_file);
        return EXIT_FAILURE;
    }

    char *line = NULL;
    size_t capacity = 0;
    int status = EXIT_SUCCESS;

    for (;;) {
        if (!quiet) {
            printf("> ");
            fflush(stdout);
        }

        if (getline(&line, &capacity, stdin) == -1) {
            if (!feof(stdin)) {
                fprintf(stderr, "Error reading query\n");
                status = EXIT_FAILURE;
            }
            break;
        }

        char **words = NULL;
        size_t nwords = 0;
        int result = split_query(line, &words, &nwords);

        if (result < 0) {
            fprintf(stderr, "Unable to allocate query words\n");
            status = EXIT_FAILURE;
            break;
        }
        if (result == 0 || !valid_syntax(words, nwords)) {
            printf("[invalid query]\n");
            free(words);
            continue;
        }

        if (nwords > 0) {
            for (size_t i = 0; i < nwords; i++) {
                printf("%s%s", i == 0 ? "" : " ", words[i]);
            }
            putchar('\n');

            if (!search_documents(index, words, nwords, dir)) {
                fprintf(stderr, "Unable to process query\n");
                free(words);
                status = EXIT_FAILURE;
                break;
            }
        }
        free(words);
    }

    free(line);
    happly(index, free_word);
    hclose(index);
    return status;
}
