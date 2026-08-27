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

SDK付属の`create_dev_ctr.sh`は、MN-Coreの実機をコンテナに接続するための補助スクリプトです。

しかし、macOSで`create_dev_ctr.sh`を使用すると、次の問題が発生します。

- スクリプトが使用する`readarray`は、macOS標準のBash 3.2では利用できない
- デバイスの排他制御に使う`/opt/mncore_shared_semaphore`のマウントに失敗する

したがって、macOSでエミュレータを試す場合は上記のDocker Composeによる実行が楽だと思います。

## ライセンス

H. Watanabeが作成したこのリポジトリのファイルは、[MIT License](LICENSE)で
公開しています。

Gitサブモジュールとして取得される`mncore/`はMIT Licenseの対象外です。
`mncore/`にはApache License 2.0が適用されます。詳細は
[`THIRD_PARTY_NOTICES.md`](THIRD_PARTY_NOTICES.md)および
[`mncore/LICENSE`](mncore/LICENSE)を参照してください。
