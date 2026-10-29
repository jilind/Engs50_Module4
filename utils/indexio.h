#pragma once

/*
 * indexio.h
 *
 * Authors: James I. Lind, Timur Tazeev
 * Created: 10-27-2026
 * Version: 1.0
 *
 * Description: Saves indexed files
 */

#include <stdint.h>
#include <stddef.h>
#include "hash.h"
#include "queue.h"

typedef struct {
    int id;
    size_t count;
} document_t;

typedef struct {
    char *word;
    queue_t *documents;
} wordentry_t;

int32_t indexsave(hashtable_t *index, const char *indexnm);
hashtable_t *indexload(const char *indexnm);
