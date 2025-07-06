#include "ping.h"
#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// シグナルセーフなフラグ（volatile sig_atomic_t型を使用）
static volatile sig_atomic_t g_exit_flag = 0;

// ping_signal.c:
// シグナルハンドラ(SIGINT/SIGTERM)による統計出力と終了処理を担当するファイル
// pingの統計情報(送受信数、パケットロス、RTT統計)を表示し、ソケットを閉じて終了する

void signal_handler(int sig, siginfo_t *info, void *ucontext) {
    // シグナルハンドラ内では最小限の処理のみ実行
    // シグナルセーフな方法で終了フラグを設定
    (void)info;    // 未使用パラメータの警告抑制
    (void)ucontext; // 未使用パラメータの警告抑制
    (void)sig;     // 未使用パラメータの警告抑制

    g_exit_flag = 1;
}

int get_exit_flag(void) {
    return g_exit_flag;
}

void print_statistics(PingContext *ctx) {
    // 入力パラメータの検証
    if (!ctx) {
        return;
    }

    // ホスト名の有効性チェック
    if (ctx->dest_hostname[0] == '\0') {
        printf("\n--- ping statistics ---\n");
    } else {
        printf("\n--- %s ping statistics ---\n", ctx->dest_hostname);
    }

    // パケットロス率の計算（ゼロ除算防止）
    double loss_rate = 0.0;
    if (ctx->packets_sent > 0) {
        // 整数オーバーフロー防止のため double キャストを先に実行
        loss_rate = ((double)(ctx->packets_sent - ctx->packets_received) * 100.0) /
                    (double)ctx->packets_sent;

        // 負の値や異常値のチェック
        if (loss_rate < 0.0) {
            loss_rate = 0.0;
        } else if (loss_rate > 100.0) {
            loss_rate = 100.0;
        }
    }

    // 重複パケット情報の表示（負の値チェック追加）
    if (ctx->packets_duplicate > 0) {
        printf("%d packets transmitted, %d packets received, +%d duplicates, "
               "%.1f%% packet loss\n",
               ctx->packets_sent,
               ctx->packets_received >= 0 ? ctx->packets_received : 0,
               ctx->packets_duplicate >= 0 ? ctx->packets_duplicate : 0,
               loss_rate);
    } else {
        printf("%d packets transmitted, %d packets received, %.1f%% packet loss\n",
               ctx->packets_sent,
               ctx->packets_received >= 0 ? ctx->packets_received : 0,
               loss_rate);
    }

    // RTT統計の表示（データ有効性チェック強化）
    if (ctx->rtt_count > 0 && ctx->rtt_sum >= 0.0) {
        double avg = ctx->rtt_sum / (double)ctx->rtt_count;
        double mdev = 0.0;

        // 標準偏差計算の改善（数値安定性とゼロ除算防止）
        if (ctx->rtt_count > 1 && ctx->rtt_sum2 >= 0.0) {
            // 分散計算で負の値を防ぐ
            double variance = (ctx->rtt_sum2 / (double)ctx->rtt_count) - (avg * avg);

            // 浮動小数点計算の誤差により variance が負になる場合があるため
            if (variance > 0.0) {
                mdev = sqrt(variance);
            } else {
                mdev = 0.0;
            }
        }

        // RTT値の妥当性チェック（負の値や異常に大きい値の防止）
        double min_rtt = (ctx->rtt_min >= 0.0) ? ctx->rtt_min : 0.0;
        double max_rtt = (ctx->rtt_max >= 0.0) ? ctx->rtt_max : 0.0;

        // avg が負になることはないはずだが、安全のためチェック
        if (avg < 0.0) {
            avg = 0.0;
        }

        // mdev が負になることはないはずだが、安全のためチェック
        if (mdev < 0.0) {
            mdev = 0.0;
        }

        printf("round-trip min/avg/max/stddev = %.3f/%.3f/%.3f/%.3f ms\n",
               min_rtt, avg, max_rtt, mdev);
    }
}
