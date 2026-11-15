/*
 * lhash.h - Header of a reusable locked hash table
 *
 * Authors: Timur Tazeev, James Lind
 * Created: 11-10-2026
 *
 * Description: A description of the methods on a
 * reusable locked hash table
 */
#pragma once
#include <stdbool.h>
#include <stdint.h>

typedef struct locked_hash lhashtable_t;

/* Create a locked hash table*/
lhashtable_t *lhopen(uint32_t hsize);

/* Close and deallocate a locked hash table*/
void lhclose(lhashtable_t *lhtp);

/* Add an element to the locked hash table*/
int32_t lhput(lhashtable_t *lhtp, void *ep, const char *key, int keylen);

/* Apply function fn to all elements in the locked hash table*/
void lhapply(lhashtable_t *lhtp, void (*fn)(void *ep));

/* Search the locked hash table for element keyp using function searchfn */
void *lhsearch(lhashtable_t *lhtp, bool (*searchfn)(void *ep, const void *key), const char *key, int32_t keylen);

/* Remove an element from the locked hash table */
void *lhremove(lhashtable_t *lhtp, bool (*searchfn)(void *ep, const void *key), const char *key, int32_t keylen);
