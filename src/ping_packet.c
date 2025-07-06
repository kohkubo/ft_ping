#define _POSIX_C_SOURCE 199309L
#include "ping.h"
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

// ICMPヘッダ構造体（RFC792に準拠、最低限のフィールドのみ）
// struct icmphdr {
//   uint8_t type;      // メッセージタイプ（8: Echo, 0: Echo Reply）
//   uint8_t code;      // コード（0固定）
//   uint16_t checksum; // チェックサム
//   union {
//     struct {
//       uint16_t id;       // 識別子（Identifier）
//       uint16_t sequence; // シーケンス番号
//     } echo;
//     uint32_t gateway;
//     struct {
//       uint16_t __unused;
//       uint16_t mtu;
//     } frag;
//   } un;
// };
// IPヘッダ構造体（最低限のフィールドのみ）
// struct iphdr {
//   uint8_t ihl : 4, version : 4;
//   uint8_t tos;
//   uint16_t tot_len;
//   uint16_t id;
//   uint16_t frag_off;
//   uint8_t ttl;
//   uint8_t protocol;
//   uint16_t check;
//   uint32_t saddr;
//   uint32_t daddr;
// };

// ping_packet.c: ICMPパケットの送信・受信・チェックサム計算を担当するファイル
// Echo Requestの送信とEcho
// Replyの受信、RTT計算、統計情報の更新、チェックサム計算を行う

// ICMP Echo/Echo Replyメッセージフォーマット
// 0                   1                   2                   3
// 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |     Type      |     Code      |          Checksum             |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |           Identifier          |        Sequence Number        |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |     Data ...
// +-+-+-+-+-
//
// Type: 8=Echo Request, 0=Echo Reply
// Code: 0固定
// Checksum: ICMPメッセージ全体の16ビット1の補数和の1の補数
// Identifier: 要求と応答の対応付け用ID（通常はプロセスID）
// Sequence Number: シーケンス番号
// Data: 任意のデータ（RTT計測などに利用）
//
// IPヘッダの送信元アドレスはEcho Requestの宛先、Echo Replyでは逆転する
//
// 詳細: RFC792参照

static unsigned short ping_checksum(void *b, int len) {
  // ICMPパケットのチェックサム計算
  // RFC792: The checksum is the 16-bit ones's complement of the one's
  // complement sum of the ICMP message starting with the ICMP Type.
  unsigned short *buf = b;
  unsigned int sum = 0;
  unsigned short result;

  // 16ビット単位で加算
  while (len > 1) {
    sum += *buf++;
    len -= 2;
  }

  // 奇数バイトの場合、最後の1バイトを処理
  if (len == 1) {
    sum += *(unsigned char *)buf << 8;
  }

  // キャリーを加算（16ビットを超えた分を下位16ビットに加算）
  while (sum >> 16) {
    sum = (sum & 0xFFFF) + (sum >> 16);
  }

  // 1の補数を取得
  result = ~sum;
  return result;
}

int send_ping(PingContext *ctx, int print_header,
              const struct timespec *timestamp) {
  if (!ctx || !timestamp) {
    return -1;
  }

  if (ctx->packets_sent >= MAX_PINGS) {
    fprintf(stderr, "Maximum ping count reached\n");
    return -1;
  }
  struct icmphdr icmp_hdr; // ICMPヘッダ（Type, Code, Checksum, Identifier,
                           // Sequence Number）
  char packet[PACKET_SIZE];
  char data[ICMP_DATA_SIZE]; // データ部バッファ（RTT計測や任意データ格納）

  // ICMPヘッダ初期化
  memset(&icmp_hdr, 0, sizeof(icmp_hdr));
  icmp_hdr.type = ICMP_ECHO; // Type: 8 (Echo Request)
  icmp_hdr.code = 0;         // Code: 0固定
  icmp_hdr.un.echo.id =
      htons(getpid() & 0xFFFF); // Identifier: プロセスID下位16bit
  icmp_hdr.un.echo.sequence =
      htons(ctx->packets_sent); // Sequence Number: 送信回数

  // データ部初期化（全体を0埋め）
  memset(data, 0, sizeof(data));
  // 送信時刻を保存（RTT計算用）
  ctx->sent_times[ctx->packets_sent] = *timestamp;
  memcpy(data, timestamp, sizeof(struct timespec));

  // パケットバッファ初期化・ヘッダ/データ部コピー
  memset(packet, 0, PACKET_SIZE);
  memcpy(packet, &icmp_hdr, ICMP_HDRLEN); // 8バイト固定: Type, Code, Checksum,
                                          // Identifier, Sequence Number
  memcpy(packet + ICMP_HDRLEN, data, ICMP_DATA_SIZE); // Data部

  // チェックサム計算
  // Checksum: ICMPメッセージ全体の16ビット1の補数和の1の補数
  ((struct icmphdr *)packet)->checksum = 0; // 計算前に0クリア
  ((struct icmphdr *)packet)->checksum =
      ping_checksum(packet, PACKET_SIZE); // 計算結果をセット

  // 最初の1回だけヘッダ出力
  if (print_header) {
    if (ctx->verbose_mode) {
      printf("PING %s (%s): %d data bytes, id 0x%04x = %d\n",
             ctx->dest_hostname, ctx->dest_ip, ICMP_DATA_SIZE,
             ntohs(icmp_hdr.un.echo.id), ntohs(icmp_hdr.un.echo.id));
    } else {
      printf("PING %s (%s): %d data bytes\n", ctx->dest_hostname, ctx->dest_ip,
             ICMP_DATA_SIZE);
    }
  }

  // ICMPパケット送信
  // IPヘッダの送信元アドレスはEcho Requestの宛先、Echo Replyでは逆転
  if (sendto(ctx->sock_fd, packet, PACKET_SIZE, 0, &ctx->dest_addr,
             sizeof(ctx->dest_addr)) < 0) {
    perror("sendto failed");
    return -1; // 送信失敗時は packets_sent をインクリメントしない
  } else {
    ctx->packets_sent++;
    return 0; // 送信成功
  }
}

int receive_ping(PingContext *ctx) {
  if (!ctx) {
    return -1;
  }

  char buffer[2048];
  struct sockaddr from;
  socklen_t fromlen = sizeof(from);
  struct timespec ts_recv;
  struct timespec ts_sent = {0};
  struct icmphdr *icmp_hdr;
  struct iphdr *ip_hdr;
  int bytes_received;
  double rtt;
  int ttl = 0;

  // ICMPパケット受信
  bytes_received =
      recvfrom(ctx->sock_fd, buffer, sizeof(buffer), 0, &from, &fromlen);

  // 受信タイムスタンプを即座にキャプチャ（RTT精度向上のため）
  if (clock_gettime(CLOCK_MONOTONIC, &ts_recv) != 0) {
    perror("clock_gettime failed in receive_ping");
    return -1;
  }

  if (bytes_received < 0) {
    perror("recvfrom failed");
    return -1;
  }

  if (bytes_received < (int)(sizeof(struct iphdr) + sizeof(struct icmphdr))) {
    // パケットサイズが不十分
    return -1;
  }

  // IPヘッダをスキップしICMPヘッダを取得
  ip_hdr = (struct iphdr *)buffer;

  // IPヘッダ長の妥当性チェック
  int ip_hdr_len = ip_hdr->ihl * 4;
  if (ip_hdr_len < (int)sizeof(struct iphdr) || ip_hdr_len > bytes_received) {
    // 無効なIPヘッダ長
    return -1;
  }

  // ICMPヘッダが含まれているかチェック
  if (bytes_received < ip_hdr_len + (int)sizeof(struct icmphdr)) {
    // ICMPヘッダが不完全
    return -1;
  }

  icmp_hdr =
      (struct icmphdr *)(buffer +
                         ip_hdr_len); // ICMPヘッダ（Type, Code, Checksum,
                                      // Identifier, Sequence Number）

  // ICMPチェックサム検証
  // Checksum: 受信時はフィールドを0にして再計算し、値が一致するか確認
  unsigned short received_checksum = icmp_hdr->checksum;
  icmp_hdr->checksum = 0; // チェックサム計算のため一時的に0に設定
  unsigned short calculated_checksum =
      ping_checksum(icmp_hdr, bytes_received - ip_hdr_len);
  icmp_hdr->checksum = received_checksum; // 元の値に戻す

  if (received_checksum != calculated_checksum) {
    // チェックサムが不一致の場合、パケットを破棄
    return -1;
  }

  // ICMP Echo Replyかつ自プロセスID宛か判定
  if (icmp_hdr->type == ICMP_ECHOREPLY &&
      ntohs(icmp_hdr->un.echo.id) == (getpid() & 0xFFFF)) {
    int seq = ntohs(icmp_hdr->un.echo.sequence); // Sequence Number
    if (seq < 0 || seq >= MAX_PINGS) {
      // 範囲外のシーケンス番号は無視
      return -1;
    }
    int idx = seq / 32;
    int bit = seq % 32;
    if (ctx->received_seq[idx] & (1 << bit)) {
      // 重複受信
      ctx->packets_duplicate++;
      ttl = ip_hdr->ttl;
      char addr_str[INET_ADDRSTRLEN];
      inet_ntop(AF_INET, &((struct sockaddr_in *)&from)->sin_addr, addr_str,
                sizeof(addr_str));

      // 重複パケットのRTT計算
      ts_sent = ctx->sent_times[seq];
      rtt = (ts_recv.tv_sec - ts_sent.tv_sec) * 1000.0 +
            (ts_recv.tv_nsec - ts_sent.tv_nsec) / 1000000.0;

      // ICMPペイロードサイズのみを表示（IPヘッダーを除く）
      int icmp_payload_size = bytes_received - ip_hdr_len;
      printf("%d bytes from %s: icmp_seq=%d ttl=%d time=%.3f ms (DUP!)\n",
             icmp_payload_size, addr_str, seq, ttl, rtt);
      return 0;
    }
    ctx->received_seq[idx] |= (1 << bit);
    ctx->packets_received++;

    // 送信時刻はPingContextのsent_timesから取得
    ts_sent = ctx->sent_times[seq];

    // RTT(往復遅延時間)を計算
    rtt = (ts_recv.tv_sec - ts_sent.tv_sec) * 1000.0 +
          (ts_recv.tv_nsec - ts_sent.tv_nsec) / 1000000.0;

    // RTT統計情報を更新
    if (ctx->rtt_count < MAX_PINGS) {
      ctx->rtt_times[ctx->rtt_count++] = rtt;
      ctx->rtt_sum += rtt;
      ctx->rtt_sum2 += rtt * rtt;
      if (ctx->rtt_count == 1 || rtt < ctx->rtt_min)
        ctx->rtt_min = rtt;
      if (ctx->rtt_count == 1 || rtt > ctx->rtt_max)
        ctx->rtt_max = rtt;
    }

    // TTL値を取得
    ttl = ip_hdr->ttl;

    // 受信結果を表示（icmp_seqは1始まりに合わせる）
    char addr_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &((struct sockaddr_in *)&from)->sin_addr, addr_str,
              sizeof(addr_str));

    // verbose出力とnomal出力の違いはない
    // ICMPペイロードサイズのみを表示（IPヘッダーを除く）
    int icmp_payload_size = bytes_received - ip_hdr_len;
    printf("%d bytes from %s: icmp_seq=%d ttl=%d time=%.3f ms\n",
           icmp_payload_size, addr_str, seq, ttl, rtt);
    return 0;
  }
  return 0;
}
