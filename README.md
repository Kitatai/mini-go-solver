# Mini-Go Solver

1 次元版 Mini-Go の勝敗探索プログラムです。

長さ `N <= 64` の直線状の盤面について、先手が左から `i` 番目に初手を打った後、その初手が先手必勝かどうかを計算します。

このゲームは [Kyo1r0](https://github.com/Kyo1r0) さんが考案した Mini-Go を基にしています。

- プレイページ: <https://kyo1r0.github.io/minigo_play_with_p/>
- 元リポジトリ: <https://github.com/Kyo1r0/minigo_test>

## ルール概要

盤面は `1 x N` の 1 次元盤面です。黒が先手、白が後手で、交互に合法手を着手します。

合法手は次の条件で定義します。

```text
合法手 = 捕獲手 ∪ 非自殺手
```

- 捕獲手: 空点に自分の石を置いた結果、相手の連の呼吸点が 0 になる手
- 自殺手: 空点に自分の石を置いた結果、その石を含む自分の連の呼吸点が 0 になる手

捕獲手を着手したプレイヤーは即勝利します。手番プレイヤーに合法手がない場合、そのプレイヤーは敗北します。

詳細は [docs/rules.md](docs/rules.md) を参照してください。

## 結果

現在のルールで `N=2..37` まで計算済みです。

- 表: [results/updated_rules/results_new_rules_n2_37.md](results/updated_rules/results_new_rules_n2_37.md)
- SVG: [results/updated_rules/results_new_rules_n2_37.svg](results/updated_rules/results_new_rules_n2_37.svg)

図では、緑が先手必勝、赤が先手必敗を表します。

## 必要環境

- C++20 対応コンパイラ
- `make`
- Python 3
- `rsvg-convert` PNG 生成を行う場合のみ

Ubuntu/Debian 系で PNG 生成まで行う場合:

```bash
sudo apt install build-essential python3 make librsvg2-bin
```

## ビルド

```bash
make
```

次の実行ファイルを生成します。

- `bin/solve_memo`: メモ化つきの高速ソルバー
- `bin/solve_memo64`: `N <= 64` 用の64bit盤面ソルバー
- `bin/solve_simple`: 小さい盤面用の単純な参照ソルバー

## 実行方法

単一の `N` を解く場合:

```bash
bin/solve_memo 32 --sparse --learn
```

範囲指定で解く場合:

```bash
bin/solve_memo --from 2 --to 32 --sparse --learn
```

出力例:

```text
N=10: L L W L W W L W L L
```

これは、左から `0..9` 番目の各初手について、先手必勝なら `W`、先手必敗なら `L` であることを表します。

## 主なオプション

- `--sparse`: 疎メモテーブルを使う
- `--learn`: 5 マス窓のオンライン評価をムーブオーダリングに使う
- `--from N --to M`: 複数の盤面サイズを連続して解く
- `--no-sym`: 左右反転対称性を使わない
- `--sparse-gib X`: 疎メモテーブルの初期容量をおおよそ `X` GiB 以下で大きめに確保する
- `--weights PATH`: 学習済みの5マス窓重みを読み書きする

通常は次の指定で十分です。

```bash
bin/solve_memo 32 --sparse --learn
```

`N > 32` を解く場合は64bit版を使います。64bit版は常に疎メモテーブルを使います。

```bash
bin/solve_memo64 33 --sparse --learn
```

## 検証

小さい盤面について、32bit版、64bit版、単純ソルバーの結果が一致することを確認します。

```bash
make test
```

## 図の再生成

SVG を再生成する場合:

```bash
make plot
```

SVG と PNG を再生成する場合:

```bash
make png
```

## 実装概要

高速ソルバーは、盤面を黒石・白石それぞれの bitboard で表現します。`solve_memo` は32bit bitboard、`solve_memo64` は64bit bitboard を使います。合法手生成と捕獲手判定は bit 演算で行います。

探索はメモ化つきの勝敗探索です。32bit版の疎メモでは、黒石と白石が隣接しない局面だけを対象にした rank key と勝敗値 2bit を合わせて、1 entry を 6 bytes に詰めています。64bit版では同じ考え方の rank key を 128bit 整数で扱い、1 entry を 11 bytes に詰めています。

詳細は [docs/implementation.md](docs/implementation.md) を参照してください。

## ディレクトリ構成

- `src/`: C++ ソルバー
- `scripts/`: 図の生成・検証用スクリプト
- `docs/`: ルールと実装メモ
- `results/`: 結果表と図
