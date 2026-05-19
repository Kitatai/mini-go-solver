# Mini-Go

1 次元版 Mini-Go の勝敗探索プログラムです。

長さ `N <= 32` の盤面について、先手が左から `i` 番目に初手を打った後、その初手が先手必勝かどうかを計算します。

現在のルールは [docs/rules.md](docs/rules.md)、実装メモは [docs/implementation.md](docs/implementation.md) にまとめています。

## ビルド

```bash
make
```

次の実行ファイルを生成します。

- `bin/solve_memo`: メモ化つきの高速ソルバー
- `bin/solve_simple`: 小さい盤面用の単純な参照ソルバー

## 実行

単一の `N` を解く場合:

```bash
bin/solve_memo 32 --sparse --learn
```

範囲指定で解く場合:

```bash
bin/solve_memo --from 2 --to 32 --sparse --learn
```

疎メモテーブルを大きめに確保する場合:

```bash
bin/solve_memo 32 --sparse --learn --sparse-gib 30
```

## 検証

小さい盤面について、高速ソルバーと単純ソルバーの結果が一致することを確認します。

```bash
make test
```

## 結果

現在のルールでの `N=2..32` の結果です。

- [results/updated_rules/results_new_rules_n2_32.md](results/updated_rules/results_new_rules_n2_32.md)
- [results/updated_rules/results_new_rules_n2_32.svg](results/updated_rules/results_new_rules_n2_32.svg)
- [results/updated_rules/results_new_rules_n2_32.png](results/updated_rules/results_new_rules_n2_32.png)

SVG を再生成する場合:

```bash
make plot
```

SVG と PNG を再生成する場合:

```bash
make png
```

## ディレクトリ構成

- `src/`: C++ ソルバー
- `scripts/`: 図の生成・検証用スクリプト
- `docs/`: ルールと実装メモ
- `results/`: 結果表と図
