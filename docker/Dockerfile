# Debianベースのpingコマンド開発環境
FROM debian:stable

# 環境変数の設定
ENV DEBIAN_FRONTEND=noninteractive
ENV LANG=C.UTF-8

# システムのアップデートと開発ツールのインストール
RUN apt-get update && apt-get install -y \
    # C/C++開発環境
    build-essential \
    gcc \
    gdb \
    make \
    cmake \
    valgrind \
    # ネットワーク開発に必要なライブラリ
    libc6-dev \
    # ネットワーク関連ツール
    net-tools \
    iputils-ping \
    iproute2 \
    tcpdump \
    netcat-openbsd \
    # nslookupとかdigとか
    dnsutils \
    # テキストエディタ
    vim \
    # その他の便利ツール
    strace \
    git \
    curl \
    wget \
    && rm -rf /var/lib/apt/lists/*

# inetutils-2.0のダウンロードとインストール
RUN cd /tmp && \
    wget https://ftp.gnu.org/gnu/inetutils/inetutils-2.0.tar.xz && \
    tar -xf inetutils-2.0.tar.xz && \
    cd inetutils-2.0 && \
    ./configure --prefix=/usr/local \
                --enable-ping \
                --disable-servers \
                --disable-clients \
                --enable-ping6 && \
    make && \
    make install && \
    cd / && \
    rm -rf /tmp/inetutils-2.0*

# SetUIDビットの設定（ping実行のため）
RUN chmod +s /usr/local/bin/ping

# 作業ディレクトリの作成
WORKDIR /app

# デフォルトのシェルをbashに設定
SHELL ["/bin/bash", "-c"]

# コンテナ起動時のデフォルトコマンド
CMD ["/bin/bash"]
