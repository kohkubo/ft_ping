#ifndef PING_H
#define PING_H

#define _GNU_SOURCE
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 199309L
#endif

#include <netinet/in.h>
#include <time.h>
#include <sys/time.h>

// ping全体で共通利用する定数や型定義
#define ICMP_HDRLEN 8 // ICMPヘッダ長（バイト数、通常8バイト）
#define PACKET_SIZE (ICMP_HDRLEN + ICMP_DATA_SIZE) // ICMPパケット全体サイズ
#define ICMP_DATA_SIZE 56   // ICMPデータ部サイズ
#define PING_INTERVAL 1     // ping送信間隔(秒)
#define MAX_PINGS 1024      // RTT記録最大数

// 受信済みシーケンス番号のビットマップ（MAX_PINGS/32で十分だが、MAX_PINGSが32の倍数でない場合の安全性も考慮し、(MAX_PINGS+31)/32とする）
#define RECEIVED_SEQ_SIZE ((MAX_PINGS + 31) / 32)

// pingの統計情報や状態をまとめた構造体
typedef struct {
    double rtt_times[MAX_PINGS]; // RTT記録配列
    int rtt_count;               // RTT記録数
    double rtt_min, rtt_max, rtt_sum, rtt_sum2; // RTT統計
    int packets_sent;            // 送信パケット数
    int packets_received;        // 受信パケット数
    int packets_duplicate;        // 重複受信パケット数
    int received_seq[RECEIVED_SEQ_SIZE]; // 受信済みシーケンス番号のビットマップ
    int ping_running;            // pingループ継続フラグ
    int sock_fd;                 // ソケットディスクリプタ
    struct sockaddr dest_addr;    // 宛先アドレス (互換性のためstruct sockaddrを使用)
    char dest_ip[INET_ADDRSTRLEN]; // 宛先IP文字列 (IPv4のみ利用)
    char dest_hostname[256];     // 宛先ホスト名
    struct timespec sent_times[MAX_PINGS]; // シーケンス番号ごとの送信時刻記録
    int verbose_mode;            // verboseモードフラグ
} PingContext;

#endif // PING_H
