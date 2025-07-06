#ifndef PING_ARGS_H
#define PING_ARGS_H

// argc, argvからホスト名やヘルプフラグ、verboseフラグを抽出する
// 戻り値: 0=正常, -1=エラー
int parse_ping_args(int argc, char **argv, char *hostname_out, int *show_help, int *verbose_mode);

#endif // PING_ARGS_H
