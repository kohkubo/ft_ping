#include "ping.h"
#include "ping_args.h"
#include "ping_packet.h"
#include "ping_resolve.h"
#include "ping_signal.h"

static int initialize_context(PingContext *ctx);
static int setup_signal_handlers(void);
static int create_socket(PingContext *ctx);
static int run_ping_loop(PingContext *ctx);
static void cleanup_context(PingContext *ctx);

static int initialize_context(PingContext *ctx) {
  if (!ctx) {
    return -1;
  }

  memset(ctx, 0, sizeof(*ctx));
  ctx->packets_duplicate = 0;
  ctx->ping_running = 1;
  ctx->sock_fd = -1;
  ctx->verbose_mode = 0;
  
  // 初期容量を設定
  ctx->rtt_capacity = 64;
  ctx->sent_times_capacity = 64;
  ctx->received_seq_size = 8; // 256ビット分（64個のシーケンス番号まで対応）
  
  // 動的メモリ割り当て
  ctx->rtt_times = malloc(ctx->rtt_capacity * sizeof(double));
  ctx->sent_times = malloc(ctx->sent_times_capacity * sizeof(struct timespec));
  ctx->received_seq = calloc(ctx->received_seq_size, sizeof(int));
  
  if (!ctx->rtt_times || !ctx->sent_times || !ctx->received_seq) {
    free(ctx->rtt_times);
    free(ctx->sent_times);
    free(ctx->received_seq);
    return -1;
  }
  
  return 0;
}

static int setup_signal_handlers(void) {
  struct sigaction sa;
  memset(&sa, 0, sizeof(sa));
  sa.sa_sigaction = signal_handler;
  sa.sa_flags = SA_SIGINFO;
  sigemptyset(&sa.sa_mask);

  if (sigaction(SIGINT, &sa, NULL) < 0) {
    perror("sigaction SIGINT failed");
    return -1;
  }

  if (sigaction(SIGTERM, &sa, NULL) < 0) {
    perror("sigaction SIGTERM failed");
    return -1;
  }

  return 0;
}

static int create_socket(PingContext *ctx) {
  ctx->sock_fd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
  if (ctx->sock_fd < 0) {
    perror("socket creation failed");
    printf("Note: This program must be run as root\n");
    return -1;
  }
  return 0;
}

static int run_ping_loop(PingContext *ctx) {
  int first = 1;
  struct timespec last_send_time;

  if (clock_gettime(CLOCK_MONOTONIC, &last_send_time) != 0) {
    perror("clock_gettime failed");
    return -1;
  }

  while (ctx->ping_running && !get_exit_flag()) {
    fd_set read_fds;
    struct timeval timeout;

    FD_ZERO(&read_fds);
    FD_SET(ctx->sock_fd, &read_fds);
    timeout.tv_sec = 0;
    timeout.tv_usec = 100000;

    int sel_ret = select(ctx->sock_fd + 1, &read_fds, NULL, NULL, &timeout);
    if (sel_ret > 0 && FD_ISSET(ctx->sock_fd, &read_fds)) {
      if (receive_ping(ctx) < 0) {
        perror("receive_ping error");
      }
    } else if (sel_ret < 0) {
      perror("select error");
    }

    if (sel_ret == 0) {
      struct timespec current_time;
      if (clock_gettime(CLOCK_MONOTONIC, &current_time) != 0) {
        perror("clock_gettime failed");
        continue;
      }

      double elapsed =
          (current_time.tv_sec - last_send_time.tv_sec) +
          (current_time.tv_nsec - last_send_time.tv_nsec) / 1000000000.0;

      if (elapsed >= PING_INTERVAL) {
        if (send_ping(ctx, first, &current_time) == 0) {
          first = 0;
          last_send_time = current_time;
        }
      }
    }
  }

  return 0;
}

static void cleanup_context(PingContext *ctx) {
  if (ctx) {
    if (ctx->sock_fd >= 0) {
      close(ctx->sock_fd);
      ctx->sock_fd = -1;
    }
    
    // 動的メモリを解放
    free(ctx->rtt_times);
    free(ctx->sent_times);
    free(ctx->received_seq);
    
    ctx->rtt_times = NULL;
    ctx->sent_times = NULL;
    ctx->received_seq = NULL;
  }
}
int main(int argc, char *argv[]) {
  PingContext ctx;
  char hostname[256] = {0};
  int show_help = 0;
  int verbose_mode = 0;

  if (initialize_context(&ctx) < 0) {
    fprintf(stderr, "ft_ping: failed to initialize context\n");
    return EXIT_FAILURE;
  }
  if (parse_ping_args(argc, argv, hostname, &show_help, &verbose_mode) != 0) {
    fprintf(stderr, "ft_ping: missing host operand\nTry 'ft_ping --help' or 'ft_ping "
                    "--usage' for more information.\n");
    return EXIT_FAILURE;
  }
  ctx.verbose_mode = verbose_mode;
  if (show_help) {
    printf("Usage: ft_ping [-v] <destination>\n");
    printf("Send ICMP ECHO_REQUEST packets to network hosts.\n");
    printf("\nOptions:\n");
    printf("  -v         verbose output\n");
    printf("  -?         display this help and exit\n");
    printf("  --help     display this help and exit\n");
    printf("  --usage    display this help and exit\n");
    return EXIT_SUCCESS;
  }
  if (setup_signal_handlers() < 0) {
    cleanup_context(&ctx);
    return EXIT_FAILURE;
  }
  ctx.dest_addr.sa_family = AF_INET;
  if (resolve_hostname(&ctx, hostname) < 0) {
    cleanup_context(&ctx);
    return EXIT_FAILURE;
  }
  if (create_socket(&ctx) < 0) {
    cleanup_context(&ctx);
    return EXIT_FAILURE;
  }
  if (run_ping_loop(&ctx) < 0) {
    cleanup_context(&ctx);
    return EXIT_FAILURE;
  }
  if (get_exit_flag()) {
    print_statistics(&ctx);
  }
  cleanup_context(&ctx);
  return EXIT_SUCCESS;
}
