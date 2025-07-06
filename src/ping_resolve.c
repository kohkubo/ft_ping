#define _GNU_SOURCE
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 199309L
#endif

#include "ping.h"
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>

// ping_resolve.c: ホスト名をIPアドレスに解決する処理を担当するファイル
// IPアドレス文字列かどうか判定し、ホスト名の場合はDNS解決を行う
int resolve_hostname(PingContext *ctx, const char *hostname) {
  // 入力パラメータの検証
  if (!ctx || !hostname || strlen(hostname) == 0) {
    return -1;
  }

  // ホスト名の長さチェック（バッファオーバーフロー防止）
  if (strlen(hostname) >= sizeof(ctx->dest_hostname)) {
    printf("ping: hostname too long\n");
    return -1;
  }

  // dest_addrをstruct sockaddr_inとして適切にキャスト
  struct sockaddr_in *sin = (struct sockaddr_in *)&ctx->dest_addr;

  // sockaddr_in構造体を初期化
  memset(sin, 0, sizeof(struct sockaddr_in));
  sin->sin_family = AF_INET;

  // ホスト名がIPアドレス形式かどうか判定
  if (inet_pton(AF_INET, hostname, &sin->sin_addr) == 1) {
    // IPアドレス形式の場合
    // 安全な文字列コピー（snprintf使用）
    int ret = snprintf(ctx->dest_ip, sizeof(ctx->dest_ip), "%s", hostname);
    if ((size_t)ret >= sizeof(ctx->dest_ip) || ret < 0) {
      printf("ping: IP address string too long\n");
      return -1;
    }

    ret = snprintf(ctx->dest_hostname, sizeof(ctx->dest_hostname), "%s",
                   hostname);
    if ((size_t)ret >= sizeof(ctx->dest_hostname) || ret < 0) {
      printf("ping: hostname string too long\n");
      return -1;
    }

    return 0;
  }

  // ホスト名をDNSで解決
  struct addrinfo hints, *res = NULL;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;    // IPv4のみ
  hints.ai_socktype = SOCK_RAW; // ping用途
  hints.ai_protocol = IPPROTO_ICMP;

  int err = getaddrinfo(hostname, NULL, &hints, &res);
  if (err != 0) {
    // gai_strerror()関数を用いて、エラーコードを人間に可読な文字列に変換
    printf("ping: cannot resolve %s: %s\n", hostname, gai_strerror(err));
    return -1;
  }

  // resがNULLでないことを確認（getaddrinfoが成功してもresがNULLの場合がある）
  if (res == NULL) {
    printf("ping: cannot resolve %s: No address returned\n", hostname);
    return -1;
  }

  // 結果をstruct sockaddr_inにコピー（型安全性を確保）
  if (res->ai_addr->sa_family != AF_INET ||
      res->ai_addrlen != sizeof(struct sockaddr_in)) {
    printf("ping: unexpected address family or size\n");
    freeaddrinfo(res);
    return -1;
  }

  // 安全なメモリコピー
  memcpy(sin, res->ai_addr, sizeof(struct sockaddr_in));

  // IPアドレス文字列を安全に作成
  if (inet_ntop(AF_INET, &sin->sin_addr, ctx->dest_ip, sizeof(ctx->dest_ip)) ==
      NULL) {
    printf("ping: inet_ntop failed\n");
    freeaddrinfo(res);
    return -1;
  }

  // ホスト名を安全にコピー
  int ret =
      snprintf(ctx->dest_hostname, sizeof(ctx->dest_hostname), "%s", hostname);
  if ((size_t)ret >= sizeof(ctx->dest_hostname) || ret < 0) {
    printf("ping: hostname string too long\n");
    freeaddrinfo(res);
    return -1;
  }

  // メモリリーク防止のため必ずfreeaddrinfoを呼び出す
  freeaddrinfo(res);

  return 0;
}
