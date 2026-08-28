# MN-Core 2 Life Game

MN-Core SDK 0.7 と MN-Core 2 Emulator で、32×32 の Conway's Game of
Lifeを10世代進めるサンプルです。セルはFP32の `0.0f`（dead）または
`1.0f`（alive）で保持し、周期境界条件を使います。

入力は32行×32文字のテキストです。`.` がdead、`*` がaliveです。

```sh
make
./lifegame sample.txt
# または
make run
```

Generation 0からGeneration 10までを表示します。各世代でhostが32×32盤面を
4×4 coreの64 blockへ分け、周期境界を含む1セル幅のhaloを加えた
`64×6×6` FP32入力を作ります。deviceは1 stepだけ計算して`64×4×4` FP32を
返すため、毎step host→device→hostの通信が発生します。

論理入力はblock-majorの`64×6×6`です。SDKの明示的scatter/gather APIへ渡す
物理入力では、hostがhaloから各セルの8近傍と現在値を取り出し、セルIDと同じ
PE recordへ配置します。1024セルを4回ずつ全4096 PEへ均等配置します。各物理
結果にもセルIDを付け、hostはIDを使って論理`64×4×4`へcompactします。
scatter/gather bufferはSDKのSoA順序（`local slot × NUM_PE + PE`）で保持します。

基本の論理配置は1 block = 1 L1B、1 core cell = 1 MABです。各入力recordは
自己完結したhaloから作るため、Life stencilの近傍取得にL1B間・MAB間通信は
使いません。
各MAB配下の4PEは同じセルを計算します。MAUのベクトル加算が8近傍和を
計算し、PEがhaloからの値の準備とLife ruleを処理します。

VSMは次のコマンドで出力できます。

```sh
make vsm
grep -E 'fvadd|l2bmi' lifegame.vsm
```

`fvadd` が近傍和のMAU命令です。Life近傍用のL1B間内部通信命令 `l2bmi` は
出力されない設計です。
