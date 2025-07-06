#include "ping_args.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_HOSTNAME_LEN 255

int parse_ping_args(int argc, char **argv, char *hostname_out, int *show_help,
                    int *verbose_mode) {
  if (!argv || !show_help || !verbose_mode) {
    return -1;
  }

  *show_help = 0;
  *verbose_mode = 0;

  int hostname_index = -1;

  for (int i = 1; i < argc; i++) {
    if (!argv[i]) {
      return -1;
    }

    if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "--usage") == 0 ||
        strcmp(argv[i], "-?") == 0) {
      *show_help = 1;
      return 0;
    }

    if (strcmp(argv[i], "-v") == 0) {
      *verbose_mode = 1;
      continue;
    }

    if (argv[i][0] == '-') {
      return -1;
    }

    if (hostname_index == -1) {
      hostname_index = i;
    } else {
      return -1;
    }
  }

  if (hostname_index == -1) {
    return -1;
  }

  if (hostname_out) {
    size_t len = strlen(argv[hostname_index]);
    if (len == 0 || len > MAX_HOSTNAME_LEN) {
      return -1;
    }
    strncpy(hostname_out, argv[hostname_index], MAX_HOSTNAME_LEN);
    hostname_out[MAX_HOSTNAME_LEN - 1] = '\0';
  }

  return 0;
}
