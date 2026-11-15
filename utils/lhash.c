/*
 * lhash.c - Implementation of a reusable locked hash table
 *
 * Authors: Timur Tazeev, James Lind
 * Created: 11-10-2026
 *
 * Description: A program implementing the methods of a
 * reusable locked hash table
 */

#include <pthread.h>
#include <stdlib.h>
#include "lhash.h"
#include "hash.h"

struct locked_hash {
    hashtable_t *table;
    pthread_mutex_t mutex;
};

lhashtable_t *lhopen(uint32_t hsize) {
    lhashtable_t *lhtp = malloc(sizeof(*lhtp));

    if (lhtp == NULL) {
        return NULL;
    }

    lhtp->table = hopen(hsize);
    if (lhtp->table == NULL) {
        free(lhtp);
        return NULL;
    }

    if (pthread_mutex_init(&lhtp->mutex, NULL) != 0) {
        hclose(lhtp->table);
        free(lhtp);
        return NULL;
    }
    return lhtp;
}

int32_t lhput(lhashtable_t *lhtp, void *ep, const char *key, int keylen) {
    if (lhtp == NULL || ep == NULL || key == NULL || pthread_mutex_lock(&lhtp->mutex) != 0) {
        return -1;
    }
    int32_t res = hput(lhtp->table, ep, key, keylen);
    pthread_mutex_unlock(&lhtp->mutex);
    return res;
}

void lhapply(lhashtable_t *lhtp, void (*fn)(void *ep)) {
    if (lhtp == NULL || fn == NULL || pthread_mutex_lock(&lhtp->mutex) != 0) {
        return;
    }
    happly(lhtp->table, fn);
    pthread_mutex_unlock(&lhtp->mutex);
}

void *lhsearch(lhashtable_t *lhtp, bool (*searchfn)(void *ep, const void *key), const char *key, int32_t keylen) {
    if (lhtp == NULL || searchfn == NULL || key == NULL || pthread_mutex_lock(&lhtp->mutex) != 0) {
        return NULL;
    }
    void *elem = hsearch(lhtp->table, searchfn, key, keylen);
    pthread_mutex_unlock(&lhtp->mutex);
    return elem;
}

void *lhremove(lhashtable_t *lhtp, bool (*searchfn)(void *ep, const void *key), const char *key, int32_t keylen) {
    if (lhtp == NULL || searchfn == NULL || key == NULL || pthread_mutex_lock(&lhtp->mutex) != 0) {
        return NULL;
    }
    void *elem = hremove(lhtp->table, searchfn, key, keylen);
    pthread_mutex_unlock(&lhtp->mutex);
    return elem;
}

void lhclose(lhashtable_t *lhtp) {
    if (lhtp == NULL) {
        return;
    }
    hclose(lhtp->table);
    pthread_mutex_destroy(&lhtp->mutex);
    free(lhtp);
}
