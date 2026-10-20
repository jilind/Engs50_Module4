/* 
 * crawler.c --- crawler for TSE 
 * 
 * Author: James I. Lind and Timur Tazeev
 * Created: 10-19-2026
 * Version: 1.0
 * 
 * Description: Crawler code for TSE
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "webpage.h"
#include "queue.h"
#include "hash.h"

int32_t pagesave(webpage_t *pagep, int id, char *dirname) {                                                        
  if (pagep == NULL || dirname == NULL || webpage_getHTML(pagep) == NULL || id < 1) {                              
    fprintf(stderr, "Error saving file!\n");                                                                        
    return -1;                                                                                
  }                                                                                                                
  char filename[256];                                                                                              
  sprintf(filename, "%s/%d", dirname, id);                                                                         
  FILE *f = fopen(filename, "w");

  if (f == NULL) {                                                                                                 
    fprintf(stderr, "Error opening file!\n");                                                                       
    return -1;                                                                                                     
  }

  
  int write1 = fprintf(f, "%s\n", webpage_getURL(pagep));                                                                       
  int write2 = fprintf(f, "%d\n", webpage_getDepth(pagep));                                                                     
  int write3 = fprintf(f, "%d\n", webpage_getHTMLlen(pagep));                                                                   
  int write4 = fprintf(f, "%s", webpage_getHTML(pagep));                                                                        
  int closure = fclose(f);

  if (write1 < 0 || write2 < 0 || write3 < 0 || write4 < 0 || closure != 0) {
    fprintf(stderr, "Error writing into file!\n");
    return -1;
  }
  return 0;                                                                                                        
}    

bool hash_compare(void *page, const void *url) {
    return strcmp(webpage_getURL(page), url) == 0;
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: crawler <seedurl> <pagedir> <maxdepth>\n");
        return EXIT_FAILURE;
    }

    char *seedurl = argv[1];
    char *pagedir = argv[2];
    int maxdepth;
    char extra;

    if (sscanf(argv[3], "%d%c", &maxdepth, &extra) != 1 || maxdepth < 0) {
        fprintf(stderr, "Invalid depth!\n");
        return EXIT_FAILURE;
    }

    webpage_t *page = webpage_new(seedurl, 0, NULL);
    if (page == NULL) {
        return EXIT_FAILURE;
    }

    if (!IsInternalURL(webpage_getURL(page))) {
        fprintf(stderr, "Invalid seed URL\n");
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
    if (hput(hash, page, root, strlen(root)) != 0 || qput(queue, page) != 0) {
        webpage_delete(page);
        qclose(queue);
        hclose(hash);
        return EXIT_FAILURE;
    }

    int id = 1;
    int check = 0;
    webpage_t *cur;

    while ((cur = qget(queue)) != NULL) {
        if (!webpage_fetch(cur)) {
            fprintf(stderr, "Failed to fetch: %s\n", webpage_getURL(cur));
            continue;
        }

        if (pagesave(cur, id, pagedir) != 0) {
            check = 1;
            break;
        }

        printf("Page %d @ %s\n", id, webpage_getURL(cur));
        id++;

        if (webpage_getDepth(cur) < maxdepth) {
            int pos = 0;
            int next;
            char *link = NULL;

            while ((next = webpage_getNextURL(cur, pos, &link)) > 0) {
                pos = next;

                if (IsInternalURL(link)) {
                    if (hsearch(hash, hash_compare, link, strlen(link)) == NULL) {
                        webpage_t *child = webpage_new(link, webpage_getDepth(cur) + 1, NULL);

                        if (child == NULL) {
                            check = 1;
                        } else if (hput(hash, child, link, strlen(link)) != 0) {
                            webpage_delete(child);
                            check = 1;
                        } else if (qput(queue, child) != 0) {
                            check = 1;
                        }
                    }
                }

                free(link);
                link = NULL;

                if (check == 1) {
                    break;
                }
            }

            free(link);

            if (next == -2) {
                fprintf(stderr, "Error extracting URL\n");
                check = 1;
            }

            if (check == 1) {
                break;
            }
        }
    }

    qclose(queue);
    happly(hash, webpage_delete);
    hclose(hash);

    if (check == 1) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
