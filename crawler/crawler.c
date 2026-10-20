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

int main(void) {
	webpage_t *page = webpage_new("https://thayer.github.io/engs50/", 0, NULL); // Create a new webpage

	if (page ==NULL) { // Confirm the webpage creation was successful
		fprintf(stderr, "Failure to create webpage\n");
		return EXIT_FAILURE;
	}

	if (!webpage_fetch(page)) { // Check to see if we can fetch the webpage
		fprintf(stderr, "Fetch webpage failed\n");
		webpage_delete(page); // delete bad webpage
		return EXIT_FAILURE;
	}
	// Define position and next to track urls we are checking
	int pos = 0;
	int next;
	char *url = NULL;

	while ((next = webpage_getNextURL(page, pos, &url)) > 0) { // Go through urls on a page labelling as external or internal
		pos = next;

		if (IsInternalURL(url)) {
			printf("Internal: %s\n", url);
		} else {
			printf("External: %s\n", url);
		}

		free(url); // Clear memory so we don't leak it
		url = NULL;
	}

	if (next == -2) { //Check if memory allocation has failed
		printf("Allocation error, could not extract URL\n");
		webpage_delete(page);
		return EXIT_FAILURE;
	}

	webpage_delete(page); //clear at the end

	return EXIT_SUCCESS;
}
