#ifndef PING_RESOLVE_H
#define PING_RESOLVE_H

#include "ping.h"
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>


int resolve_hostname(PingContext *ctx, const char *hostname);


#endif // PING_RESOLVE_H
