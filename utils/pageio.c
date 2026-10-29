/* 
 * pageio.c --- 
 * 
 * Authors: James I. Lind and Timur Tazeev
 * Created: 10-24-2026
 * Version: 1.0
 * 
 * Description: Save a page to a file that contains url, depth, html length, and the html
 * 
 */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "pageio.h"
#include "webpage.h"

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

webpage_t *pageload(int id, char *dirnm) {
	char filename[4096];
	char url[4096];
	int depth;
	size_t html_len;

	if (snprintf(filename, sizeof(filename), "%s/%d", dirnm, id) >= sizeof(filename)) {
		return NULL;
	}

	FILE *fp = fopen(filename, "rb");

	if (fp == NULL) {
		return NULL;
	}

	if (fscanf(fp, "%4095s", url) != 1 || fscanf(fp, "%d", &depth) != 1 || fscanf(fp, "%zu", &html_len) != 1) {
		fclose(fp);
		return NULL;
	}
	
	if (fgetc(fp) != '\n') {
		fclose(fp);
		return NULL;
	}

	char *html = malloc(html_len + 1);
	if (html == NULL) {
		fclose(fp);
		return NULL;
	}
	
	if (fread(html, 1, html_len, fp) != html_len) {
		free(html);
		fclose(fp);
		return NULL;
	}

	html[html_len] = '\0';
	fclose(fp);
	webpage_t *page = webpage_new(url, depth, html);

	if (page == NULL) {
		free(html);
	}

	return page;
}


