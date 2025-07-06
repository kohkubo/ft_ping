# ft_ping

🏓 A custom implementation of the classic `ping` command written in C

## 概要

ft_pingは、ICMPエコーリクエストを送信してネットワーク接続をテストするpingコマンドの実装です。

## 機能

- **ICMPエコーリクエスト送信**: 指定されたホストにpingパケットを送信
- **RTT測定**: ラウンドトリップタイムの測定と統計情報の表示
- **パケット統計**: 送信・受信・ロスト・重複パケットの統計
- **ホスト名解決**: ドメイン名からIPアドレスへの自動変換
- **シグナルハンドリング**: SIGINT/SIGTERMでの適切な終了処理
- **Verboseモード**: 詳細な出力オプション

## 必要な環境

- Linux/macOS
- GCC コンパイラ
- root権限（RAWソケット使用のため）
- Docker（開発環境用）

## インストール

### ローカル環境

```bash
git clone https://github.com/kohkubo/ft_ping.git
cd ft_ping
make ft_ping
```

### Docker環境

```bash
# Docker環境をビルド
make build

# コンテナを起動
make up

# コンテナに入る
make exec

# コンテナ内でビルド
make ping
```

## 使用方法

### 基本的な使用法

```bash
# 基本的なping
./ft_ping google.com

# Verboseモード
./ft_ping -v google.com

# ヘルプ表示
./ft_ping --help
```

### オプション

- `-v` : Verboseモード - 詳細な出力を表示
- `--help` : ヘルプメッセージを表示
- `--usage` : 使用法を表示

## 出力例

```
PING google.com (172.217.175.14): 56 data bytes
64 bytes from 172.217.175.14: icmp_seq=0 ttl=115 time=12.345 ms
64 bytes from 172.217.175.14: icmp_seq=1 ttl=115 time=11.234 ms
64 bytes from 172.217.175.14: icmp_seq=2 ttl=115 time=13.456 ms
^C
--- google.com ping statistics ---
3 packets transmitted, 3 packets received, 0.0% packet loss
round-trip min/avg/max/stddev = 11.234/12.345/13.456/0.912 ms
```

## プロジェクト構造

```
ft_ping/
├── src/                    # ソースコード
│   ├── main.c             # メイン関数
│   ├── ping_args.c        # 引数解析
│   ├── ping_packet.c      # パケット送受信
│   ├── ping_resolve.c     # ホスト名解決
│   └── ping_signal.c      # シグナル処理
├── include/               # ヘッダファイル
│   ├── ping.h            # 共通定義
│   ├── ping_args.h       # 引数解析
│   ├── ping_packet.h     # パケット処理
│   ├── ping_resolve.h    # ホスト名解決
│   └── ping_signal.h     # シグナル処理
├── tests/                 # テストファイル
│   └── ping_error_test.sh # エラーテスト
├── docs/                  # ドキュメント
│   └── test.md           # テスト設定
├── docker/                # Docker関連
│   ├── Dockerfile        # Docker設定
│   └── docker-compose.yml # Docker Compose設定
├── obj/                   # オブジェクトファイル（自動生成）
├── Makefile              # ビルド設定
└── README.md             # このファイル
```

## 開発

### ビルド

```bash
# 通常ビルド
make ft_ping

# クリーンビルド
make clean
make ft_ping
```

### テスト

```bash
# 基本テスト
make ping-run

# エラーテスト
./tests/ping_error_test.sh

# Docker環境でのテスト
make exec
# コンテナ内で
make ping-run
```

### デバッグ

```bash
# デバッグビルド
CFLAGS="-g -O0" make ft_ping

# Valgrindでのメモリチェック
valgrind --leak-check=full ./ft_ping google.com
```

## 技術的な詳細

### ICMPプロトコル

- ICMP Echo Request (Type 8) を送信
- ICMP Echo Reply (Type 0) を受信
- 各パケットにシーケンス番号とタイムスタンプを埋め込み

### RTT計算

- 高精度タイマー（`clock_gettime`）を使用
- マイクロ秒単位での時間測定
- 統計値（min/avg/max/stddev）の計算

### メモリ管理

- 適切なリソース管理
- シグナル受信時の安全な終了処理
- メモリリークの防止

## 制限事項

- IPv4のみサポート（IPv6は未対応）
- Raw socketの使用により root権限が必要
- 最大1024個のpingに制限
- 固定のping間隔（1秒）

## 参考資料

- [Debian Manpages](https://manpages.debian.org/)
- [RFC 792 - Internet Control Message Protocol](https://tools.ietf.org/html/rfc792)
