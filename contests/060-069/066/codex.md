# AHC066 Codex Notes

## 基本情報

- 問題文: `problem.md`
- 通常コード: `main.cpp`
- 3000 ケース検証用コード: `large/main.cpp`
- 通常テストケース: `tools/in`
- 3000 ケーステストケース: `large/tools/in`

このディレクトリは `AHC` リポジトリ配下にある。作業前は必ず以下を確認する。

```sh
cd /Users/user37/kyopro/AHC
git status --short
git diff -- contests/060-069/066/main.cpp
```

ユーザーの未コミット変更を勝手に戻さないこと。

## 実行方法

実行には `pahcer` を使う。

通常 100 ケース:

```sh
cd /Users/user37/kyopro/AHC/contests/060-069/066
pahcer run -c <comment>
```

3000 ケース:

```sh
cd /Users/user37/kyopro/AHC/contests/060-069/066/large
pahcer run -c <comment>
```

コメント名は後で比較しやすい名前にする。

## 結果ファイル

通常:

- JSON: `pahcer/json/result_*.json`
- stderr: `tools/err/0000.txt` など
- stdout: `tools/out/0000.txt` など

large:

- JSON: `large/pahcer/json/result_*.json`
- stderr: `large/tools/err/0000.txt` など
- stdout: `large/tools/out/0000.txt` など

## 比較で見るもの

平均スコアだけで判断しない。最低限以下を見る。

- `Average Score`
- `Average Score (log10)`
- `Max Execution Time`
- 2 秒超えケース数
- 改善 / 同値 / 悪化ケース数
- 大悪化ケースの有無

方針比較では、できるだけ実行時間を揃える。時間が長い版と短い版をそのまま比較しない。

## 高速化を見るとき

高速化の効果はスコアだけでなく stderr の `iter` を見る。

- 同じ時間で `iter` が増えているか
- `iter` が増えた結果、スコアが安定しているか
- キャッシュや枝刈りで探索回数が増えているか

stderr は例えば以下で確認する。

```sh
sed -n '1,20p' tools/err/0000.txt
```

large 側なら:

```sh
sed -n '1,20p' large/tools/err/0000.txt
```

## 時間上限について

時間上限を伸ばした実験は有用だが、提出用の評価と混同しない。

- 提出想定では 2 秒以内を意識する
- ただし発想検証中は 2 秒超えでもよい
- 比較するときは、同じ時間条件で比べる
- 最大実行時間が 2 秒を超える場合は、スコアが良くても提出向きとは限らない

## 作業方針

ユーザーは実験フェーズではスコア悪化を許容することがある。そういう場合は、既存実装の微改善に囚われすぎず、大きめの方針変更も試す。

ただし、実装後は必ず `pahcer` で確認し、結果を具体的な数値で報告する。
