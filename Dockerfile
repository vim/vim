FROM ubuntu:24.04

ENV TZ=Asia/Tokyo
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && \
    echo $TZ > /etc/timezone
RUN apt update && apt upgrade -y

# カレントディレクトリ
WORKDIR /artifact

# 必要なAPTパッケージを適当にインストール
RUN apt update && apt install -y git build-essential libncurses5-dev libncursesw5-dev

# Gitリポジトリを展開しても良い
RUN git clone https://github.com/oss-experiment-uec/2024-a2110013-vim2.git

#リポジトリに移動
WORKDIR /artifact/2024-a2110013-vim

RUN make -j
RUN make install
# Dockerfileを実行する場所からファイルをコピーする場合
# COPY . /artifact
