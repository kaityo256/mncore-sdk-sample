# MN-Core SDK を使うためのサンプル

MN-Core SDK 0.7 のDocker環境で、ベクトル加算のサンプルをコンパイルし、
MN-Coreエミュレータ上で実行します。MN-Coreの実機は必要ありません。

## 必要なもの

- Git
- Docker（Docker Composeを含む）

## リポジトリのクローン

SDKを含むサブモジュールも取得するため、`--recursive`を付けてクローンします。

```sh
git clone --recursive https://github.com/kaityo256/mncore-sdk-sample.git
cd mncore-sdk-sample
```

すでにクローン済みでサブモジュールが空の場合は、次のコマンドで取得できます。

```sh
git submodule update --init --recursive
```

## Dockerイメージのビルド

リポジトリのルートディレクトリで、SDKのminimalイメージと、それをベースにした
fullイメージを順にビルドします。

```sh
docker build -t mncore-sdk-minimal:0.7 -f mncore/sdk/0.7/mncore-sdk-minimal.Dockerfile .
docker build -t mncore-sdk-full:0.7 -f mncore/sdk/0.7/mncore-sdk-full.Dockerfile --build-arg minimal_image_ref=mncore-sdk-minimal:0.7 .
```

## サンプルの実行

fullイメージをベースにサンプル用イメージをビルドし、コンテナに入ります。
初回はイメージのビルドが行われるため、完了まで時間がかかります。

```sh
docker compose -f docker/docker-compose.yml run --rm --build mncore-sdk
```

コンテナ内で、デバイス用コードとホスト用コードをコンパイルして実行します。

```sh
mnclc add.c -e add -o add.bin
c++ -std=c++23 main.cc -o add_host -lmncl
./add_host
```

次のように表示されれば成功です。

```text
1 + 2 = 3
```

## 補足

SDK付属の`create_dev_ctr.sh`は、MN-Coreの実機をコンテナに接続するための
補助スクリプトです。このサンプルではエミュレータを使用するため、スクリプトを
使わずDocker Composeでコンテナを起動します。

また、macOSで`create_dev_ctr.sh`を使用すると、次の問題が発生します。

- スクリプトが使用する`readarray`は、macOS標準のBash 3.2では利用できない
- デバイスの排他制御に使う`/opt/mncore_shared_semaphore`のマウントに失敗する

この点からも、macOSでエミュレータを試す場合は上記のDocker Composeによる実行が
適しています。
