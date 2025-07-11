#ifndef PING_PACKET_H
#define PING_PACKET_H

#include "ping.h"
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>


int send_ping(PingContext *ctx, int print_header, const struct timespec *timestamp);
int receive_ping(PingContext *ctx);


#endif // PING_PACKET_H
