#ifndef UTILS_H
#define UTILS_H

#include <string.h>
#include <curl/curl.h>
#include <stdlib.h>

#include "macros.h"

char* str_concat(const char *s1, const char *s2);
char* alias_img(void);
int peer_print (int s, char *msg_buf);
int peer_read (int s, char *msg);

#endif