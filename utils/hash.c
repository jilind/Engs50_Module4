/* 
 * hash.c -- implements a generic hash table as an indexed set of queues.
 * Authors: Paul Hsieh, Stephen Taylor, Timur Tazeev, James Lind
 *
 */
#include "hash.h"
#include "queue.h"
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>

/* 
 * SuperFastHash() -- produces a number between 0 and the tablesize-1.
 * 
 * The following (rather complicated) code, has been taken from Paul
 * Hsieh's website under the terms of the BSD license. It's a hash
 * function used all over the place nowadays, including Google Sparse
 * Hash.
 */
#define get16bits(d) (*((const uint16_t *) (d)))

static uint32_t SuperFastHash (const char *data,int len,uint32_t tablesize) {
  uint32_t hash = len, tmp;
  int rem;
  
  if (len <= 0 || data == NULL)
		return 0;
  rem = len & 3;
  len >>= 2;
  /* Main loop */
  for (;len > 0; len--) {
    hash  += get16bits (data);
    tmp    = (get16bits (data+2) << 11) ^ hash;
    hash   = (hash << 16) ^ tmp;
    data  += 2*sizeof (uint16_t);
    hash  += hash >> 11;
  }
  /* Handle end cases */
  switch (rem) {
  case 3: hash += get16bits (data);
    hash ^= hash << 16;
    hash ^= data[sizeof (uint16_t)] << 18;
    hash += hash >> 11;
    break;
  case 2: hash += get16bits (data);
    hash ^= hash << 11;
    hash += hash >> 17;
    break;
  case 1: hash += *data;
    hash ^= hash << 10;
    hash += hash >> 1;
  }
  /* Force "avalanching" of final 127 bits */
  hash ^= hash << 3;
  hash += hash >> 5;
  hash ^= hash << 4;
  hash += hash >> 17;
  hash ^= hash << 25;
  hash += hash >> 6;
  return hash % tablesize;
}

typedef struct hash {
    uint32_t size;
    queue_t **bins;
} hash;

hashtable_t *hopen(uint32_t hsize) {
    if (hsize == 0) {
        return NULL;
    }

    hash *htable = malloc(sizeof(hash));

    if (htable == NULL) {
        return NULL;
    }

    htable->bins = malloc(hsize * sizeof(queue_t *));
    if (htable->bins == NULL) {
        free(htable);
        return NULL;
    }

    for (uint32_t i = 0; i < hsize; i++) {
        htable->bins[i] = NULL;
    }
    htable->size = hsize;
    return htable;
}

void hclose(hashtable_t *htp) {
    hash *htable = htp;

    if (htable == NULL) {
        return;
    }

    for (uint32_t i = 0; i < htable->size; i++) {
        if (htable->bins[i] != NULL) {
            qclose(htable->bins[i]);
        }
    }
    free(htable->bins);
    free(htable);
}

int32_t hput(hashtable_t *htp, void *ep, const char *key, int keylen) {
    hash *htable = htp;
    if (htable == NULL || ep == NULL || key == NULL || keylen < 0) {
        return -1;
    }

    uint32_t index = SuperFastHash(key, keylen, htable->size);
    if (htable->bins[index] == NULL) {
        htable->bins[index] = qopen();
        if (htable->bins[index] == NULL) {
            return -1;
        }
    }
    return qput(htable->bins[index], ep);
}

void happly(hashtable_t *htp, void (*fn)(void* ep)) {
    hash *htable = htp;
    if (htable == NULL || fn == NULL) {
        return;
    }

    for (uint32_t i = 0; i < htable->size; i++) {
        if (htable->bins[i] != NULL) {
            qapply(htable->bins[i], fn);
        }
    }
}

void *hsearch(hashtable_t *htp, bool (*searchfn)(void* elementp, const void* searchkeyp), const char *key, int32_t keylen) {
    hash *htable = htp;
    if (htable == NULL || searchfn == NULL || key == NULL || keylen < 0) {
        return NULL;
    }
    uint32_t index = SuperFastHash(key, keylen, htable->size);
    if (htable->bins[index] == NULL) {
        return NULL;
    }

    return qsearch(htable->bins[index], searchfn, key);
}

void *hremove(hashtable_t *htp, bool (*searchfn)(void* elementp, const void* searchkeyp), const char *key, int32_t keylen) {
    hash *htable = htp;
    if (htable == NULL || searchfn == NULL || key == NULL || keylen < 0) {
        return NULL;
    }
    uint32_t index = SuperFastHash(key, keylen, htable->size);
    if (htable->bins[index] == NULL) {
        return NULL;
    }
    return qremove(htable->bins[index], searchfn, key);
}
