/*
 * indexer_pt.c --- A multi-threaded indexer for the TSE
 *
 * Author: James I. Lind, Timur Tazeev
 * Created: 11-11-2026
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
#include <pthread.h>
#include <sys/stat.h>
#include "webpage.h"
#include "pageio.h"
#include "indexio.h"

typedef struct {
    char *pagedir;
    hashtable_t *index;
    pthread_mutex_t mutex;
    int last_id;
    size_t thread_count;
} index_job_t;

typedef struct {
    index_job_t *job;
    size_t first_id;
    size_t pages;
} worker_t;

static size_t unique_words = 0;

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

static void count_entry(void *element) {
    (void)element;
    unique_words++;
}

static size_t count_unique_words(hashtable_t *index) {
    unique_words = 0;
    happly(index, count_entry);
    return unique_words;
}

static int last_page_id(const char *pagedir) {
    for (int id = 1; id < INT_MAX; id++) {
        char filename[4096];
        int length = snprintf(filename, sizeof(filename), "%s/%d", pagedir, id);
        if (length < 0 || (size_t)length >= sizeof(filename)) {
            return -1;
        }
        struct stat st;
        if (stat(filename, &st) != 0) {
            return errno == ENOENT ? id - 1 : -1;
        }
        if (!S_ISREG(st.st_mode)) {
            return -1;
        }
    }
    return INT_MAX - 1;
}

static bool scan_page(index_job_t *job, webpage_t *page, int id) {
    int pos = 0;
    char *word = NULL;
    while ((pos = webpage_getNextWord(page, pos, &word)) > 0) {
        bool success = true;
        if (NormalizeWord(word)) {
            if (pthread_mutex_lock(&job->mutex) != 0) {
                free(word);
                return false;
            }
            success = add_word(job->index, word, id);
            pthread_mutex_unlock(&job->mutex);
        }
        free(word);
        word = NULL;
        if (!success) {
            return false;
        }
    }
    free(word);
    return true;
}

static void *run_worker(void *argument) {
    worker_t *worker = argument;
    index_job_t *job = worker->job;
    for (size_t id = worker->first_id; id <= (size_t)job->last_id; id += job->thread_count) {
        webpage_t *page = pageload((int)id, job->pagedir);
        if (page == NULL) {
					worker->pages++;
					continue;
        }
        const char *html = webpage_getHTML(page);
        const char *last_open = strrchr(html, '<');
        const char *last_close = strrchr(html, '>');
        if (last_open != NULL && (last_close == NULL || last_open > last_close)) {
            fprintf(stderr, "Skipping malformed page %zu\n", id);
            webpage_delete(page);
            continue;
        }
        bool success = scan_page(job, page, (int)id);
        webpage_delete(page);
        if (!success) {
            fprintf(stderr, "Unable to index page %zu\n", id);
            return worker;
        }
        worker->pages++;
    }
    return NULL;
}

static bool run_workers(index_job_t *job, size_t *pages) {
    pthread_t *threads = calloc(job->thread_count, sizeof(*threads));
    worker_t *workers = calloc(job->thread_count, sizeof(*workers));
    if (threads == NULL || workers == NULL) {
        free(threads);
        free(workers);
        return false;
    }
    size_t started = 0;
    for (; started < job->thread_count; started++) {
        workers[started].job = job;
        workers[started].first_id = started + 1;
        int result = pthread_create(&threads[started], NULL, run_worker, &workers[started]);
        if (result != 0) {
            fprintf(stderr, "Unable to create thread: %s\n", strerror(result));
            break;
        }
    }
    bool success = started == job->thread_count;
    for (size_t i = 0; i < started; i++) {
        void *worker_result = NULL;
        int result = pthread_join(threads[i], &worker_result);
        if (result != 0) {
            fprintf(stderr, "Unable to join thread: %s\n", strerror(result));
            exit(EXIT_FAILURE);
        }
        if (worker_result != NULL) {
            success = false;
        }
        *pages += workers[i].pages;
    }
    free(threads);
    free(workers);
    return success;
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: indexer <pagedir> <indexnm> <threads>\n");
        return EXIT_FAILURE;
    }
    char *end;
    errno = 0;
    long thread_count = strtol(argv[3], &end, 10);
    if (errno != 0 || end == argv[3] || *end != '\0' ||
        thread_count < 1 || thread_count > INT_MAX) {
        fprintf(stderr, "Invalid thread count: %s\n", argv[3]);
        return EXIT_FAILURE;
    }
    char *pagedir = argv[1];
    const char *indexnm = argv[2];
    struct stat st;
    if (stat(pagedir, &st) != 0 || !S_ISDIR(st.st_mode)) {
        fprintf(stderr, "Invalid page directory: %s\n", pagedir);
        return EXIT_FAILURE;
    }
    int last_id = last_page_id(pagedir);
    if (last_id <= 0) {
        fprintf(stderr, "Unable to find crawler pages in %s\n", pagedir);
        return EXIT_FAILURE;
    }
    index_job_t job = {
        .pagedir = pagedir,
        .index = hopen(1000),
        .last_id = last_id,
        .thread_count = (size_t)thread_count
    };
    if (job.index == NULL) {
        fprintf(stderr, "Unable to create index\n");
        return EXIT_FAILURE;
    }
    if (pthread_mutex_init(&job.mutex, NULL) != 0) {
        fprintf(stderr, "Unable to initialize index mutex\n");
        hclose(job.index);
        return EXIT_FAILURE;
    }
    size_t pages = 0;
    bool success = run_workers(&job, &pages);
    pthread_mutex_destroy(&job.mutex);

    if (success && pages > 0 && indexsave(job.index, indexnm) == 0) {
        printf("Indexed %zu pages\n", pages);
        printf("Unique words: %zu\n", count_unique_words(job.index));
    } else {
        fprintf(stderr, "Unable to build or save index\n");
        success = false;
    }
    happly(job.index, free_word);
    hclose(job.index);
    return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
