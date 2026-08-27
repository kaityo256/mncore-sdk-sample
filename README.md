# MN-Core SDKを使うためのサンプル

## リポジトリのクローン

このリポジトリを--recursiveにクローンする。

```sh
git clone --recursive https://
cd mncore-sdk-sample
```

## Docker Imageのビルド

まず、mncore-sdk-full:0.7のイメージを作成

```sh
docker build -t mncore-sdk-minimal:0.7 -f mncore/sdk/0.7/mncore-sdk-minimal.Dockerfile .
docker build -t mncore-sdk-full:0.7 -f mncore/sdk/0.7/mncore-sdk-full.Dockerfile --build-arg minimal_image_ref=mncore-sdk-minimal:0.7 .
```

公式サイトでは`create_dev_ctr.sh`を使う例が載っていますが、これは利用するMN-Coreデバイスを探して、もし実機があればそれを使うための補助スクリプトです。しかし、

- スクリプト内でbashの機能である`readarray`を使っているのですが、Mac標準のBashである`/bin/bash`がまだ3.2系で、`readarray`をサポートしていない
- デバイスの排他制御の`/opt/mncore_shared_semaphore`を作ろうとして失敗する

という問題があるので、直接イメージを起動しましょう。

```sh
docker compose -f docker/docker-compose.yml run --rm mncore-sdk
```

```sh
mnclc add.c -e add -o add.bin
c++ -std=c++23 main.cc -o add_host -lmncl
./add_host
```
