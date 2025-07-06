#ifndef PING_PACKET_H
#define PING_PACKET_H

#include "ping.h"
#include <time.h>


int send_ping(PingContext *ctx, int print_header, const struct timespec *timestamp);
int receive_ping(PingContext *ctx);


#endif // PING_PACKET_H