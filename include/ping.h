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
// pingの統計情報や状態をまとめた構造体
typedef struct {
    double *rtt_times;           // RTT記録配列（動的割り当て）
    int rtt_count;               // RTT記録数
    int rtt_capacity;            // RTT記録配列の容量
    double rtt_min, rtt_max, rtt_sum, rtt_sum2; // RTT統計
    int packets_sent;            // 送信パケット数
    int packets_received;        // 受信パケット数
    int packets_duplicate;        // 重複受信パケット数
    int *received_seq;           // 受信済みシーケンス番号のビットマップ（動的割り当て）
    int received_seq_size;       // ビットマップサイズ
    int ping_running;            // pingループ継続フラグ
    int sock_fd;                 // ソケットディスクリプタ
    struct sockaddr dest_addr;    // 宛先アドレス (互換性のためstruct sockaddrを使用)
    char dest_ip[INET_ADDRSTRLEN]; // 宛先IP文字列 (IPv4のみ利用)
    char dest_hostname[256];     // 宛先ホスト名
    struct timespec *sent_times; // シーケンス番号ごとの送信時刻記録（動的割り当て）
    int sent_times_capacity;     // 送信時刻記録配列の容量
    int verbose_mode;            // verboseモードフラグ
} PingContext;

#endif // PING_H
