/* 
 * testpageio.c --- 
 * 
 * Author: James I. Lind
 * Created: 10-28-2026
 * Version: 1.0
 * 
 * Description: Tests the pageio.c functions
 * 
 */

#include <stdio.h>
#include <string.h>
#include "pageio.h"
#include "webpage.h"
int main(void) {
	char from[] = "../pagedir";
	char to[] = "../testruns";

	webpage_t *first = pageload(1, from);

	if (first == NULL) {
		return 1;
	}

	if (pagesave(first, 1, to) != 0) {
		webpage_delete(first);

		return 1;
	}

	webpage_t *second = pageload(1, to);

	if (second == NULL) {
		webpage_delete(first);
		return 1;
	}

	int same = strcmp(webpage_getURL(first), webpage_getURL(second)) == 0 && webpage_getDepth(first) == webpage_getDepth(second)
		&& webpage_getHTMLlen(first) == webpage_getHTMLlen(second) && strcmp(webpage_getHTML(first), webpage_getHTML(second)) == 0;
	printf("%s\n", same ? "PASS" : "FAIL");

	webpage_delete(first);
	webpage_delete(second);

	return same ? 0 : 1;
}
