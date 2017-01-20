#ifndef CONNECT_H
#define CONNECT_H

#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "macros.h"
#include "utils.h"

int run (int s, char *cmd) 
int parse (int s, char *msg)
int init (char *ip, int port);

#endif