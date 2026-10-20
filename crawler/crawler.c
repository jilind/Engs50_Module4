/* 
 * crawler.c --- 
 * 
 * Author: James I. Lind and Timur Tazeev
 * Created: 10-19-2026
 * Version: 1.0
 * 
 * Description: 
 */
#include <stdio.h>
#include <stdlib.h>
#include "webpage.h"
#include <stdint.h>
#include "queue.h"
#include "hash.h"
#include <string.h>

int32_t pagesave(webpage_t *pagep, int id, char *dirname) {
	if (pagep == NULL || dirname == NULL || id < 1 || webpage_getHTML(pagep) == NULL) {
		fprintf(stderr, "Error saving file\n");
		return -1; // Failure to save
	}
	char filename[256];
	sprintf(filename, "%s/%d", dirname, id);
	FILE *f = fopen(filename, "w");
	if (f == NULL) {
		fprintf(stderr, "Error opening file\n");
		return -1;
	}
	fprintf(f, "%s\n", webpage_getURL(pagep));
	fprintf(f, "%d\n", webpage_getDepth(pagep));
	fprintf(f, "%d\n", webpage_getHTMLlen(pagep));
	fprintf(f, "%s", webpage_getHTML(pagep));
	fclose(f);
	return 0;
}
	
bool hash_compare(void *page, const void *url) {
    return strcmp(webpage_getURL(page), url) == 0;
}

int main(void) {
    webpage_t *page = webpage_new("https://thayer.github.io/engs50/", 0, NULL);
    if (page == NULL) {
        return EXIT_FAILURE;
    }

    if (!webpage_fetch(page)) {
        webpage_delete(page);
        return EXIT_FAILURE;
    }

    queue_t *queue = qopen();
    hashtable_t *hash = hopen(100);
    if (queue == NULL || hash == NULL) {
        webpage_delete(page);
        qclose(queue);
        hclose(hash);
        return EXIT_FAILURE;
    }

    char *root = webpage_getURL(page);
    if (hput(hash, page, root, strlen(root)) != 0) {
        hclose(hash);
        qclose(queue);
        webpage_delete(page);
        return EXIT_FAILURE;
    }

    int pos = 0;
    char *link = NULL;
    webpage_t *cur;
    int check = 0;

    while ((pos = webpage_getNextURL(page, pos, &link)) > 0) {
        if (IsInternalURL(link)) {
            if (hsearch(hash, hash_compare, link, strlen(link)) == NULL) {
                cur = webpage_new(link, 1, NULL);
                if (cur == NULL || qput(queue, cur) != 0) {
                    webpage_delete(cur);
                    check = 1;
                } else if (hput(hash, cur, link, strlen(link)) != 0) {
                    check = 1;
                }
            }
        }
        else {
            printf("External Link: %s\n", link);
        }
        free(link);
        link = NULL;
        if (check == 1) {
            break;
        }
    }

    if (pos == -2) {
		printf("Allocation error, could not extract URL\n");
		check = 1;
    }

    hclose(hash);

    if (check == 1) {
        while ((cur = qget(queue)) != NULL) {
            webpage_delete(cur);
        }
        webpage_delete(page);
        qclose(queue);
        return EXIT_FAILURE;
    }

    printf("Internal pages in queue:\n");

    int count = 1;
    while ((cur = qget(queue)) != NULL) {
        printf("Page %d @ %s\n", count, webpage_getURL(cur));
        webpage_delete(cur);
        count++;
    }

    qclose(queue);
    webpage_delete(page);
    return EXIT_SUCCESS;
}
