#ifndef PING_SIGNAL_H
#define PING_SIGNAL_H

#define _GNU_SOURCE
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 199309L
#endif

#include <signal.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "ping.h"


void signal_handler(int sig, siginfo_t *info, void *ucontext);
int get_exit_flag(void);
void print_statistics(PingContext *ctx);


#endif // PING_SIGNAL_H
