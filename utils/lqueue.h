/*
 * lqueue.h - Header of a reusable locked queue
 *
 * Authors: Timur Tazeev, James Lind
 * Created: 11-10-2026
 *
 * Description: A description of the methods on a
 * reusable locked queue
 */

#pragma once
#include <stdbool.h>
#include <stdint.h>

typedef struct locked_queue lqueue_t;

/* Create a locked queue */
lqueue_t *lqopen(void);

/* Add an element to the end of the locked queue*/
int32_t lqput(lqueue_t *lqp, void *elementp);

/* Get an element from the front of the locked queue*/
void *lqget(lqueue_t *lqp);

/* Apply function fn to all elements in the locked queue*/
void lqapply(lqueue_t *lqp, void (*fn)(void *elementp));

/* Search the locked queue for element keyp using function searchfn */
void *lqsearch(lqueue_t *lqp,
               bool (*searchfn)(void *elementp, const void *keyp),
               const void *keyp);

/* Close and deallocate the locked queue */               
void lqclose(lqueue_t *lqp);
