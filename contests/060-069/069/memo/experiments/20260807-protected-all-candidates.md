# 従来候補保護付き全テンプレート走査

## 仮説

配置時に全テンプレート候補を一度確認しつつ、従来候補を保護し、同一最短周長内だけでfuture-fitを比較する。

## baselineと変更範囲

- `Lmin..Lmin+4`の全テンプレート・全合法anchorを走査する。
- 従来方式の最大6候補を保護する。
- 追加候補は別枠で最大6候補まで保持する。
- 追加候補による従来候補の押し出しを禁止する。
- future-fitの前に最短周長以外を除外し、異周長を比較しない。
- 受入判断は従来のsampled DLPによる`時間帯価格 × 使用量`のまま。

## 実行前に固定した仕様・停止条件

- エージェントは警告付きコンパイルまで行い、スコア実行はユーザーが行う。
- 結果後はユーザーの明示指示なしに実装を変更しない。

## source / binary / config / input / oracle

- source: 実行時点の`main.cpp`。source / binary hashは実行前に固定していない。
- config: `pahcer_config.toml`、seed 0〜99、`threads = 0`
- 正本: `pahcer/json/result_20260807_000654.json`
- 正本SHA-256: `3b0793711d1755d619665de86d37981e91e4a0b1ff80b14dc10a55a01ec750c1`
- 再実行: `pahcer/json/result_20260807_021319.json`、comment「シンプル候補増し」
- 再実行SHA-256: `0fb80a2bad7217c1eff9a3f8aaa2660aba6fff0954d30fc6584fa8346095bc73`
- 2 runのseed別raw scoreは100/100完全一致した。

## 静的検証

- `clang++ -std=c++20 -O2 -Wall -Wextra -pedantic`で警告なしコンパイル成功。

## 実行結果

- 100 seed合計: `6,439,164,489`
- 平均score: `64,391,644.89`
- 100/100 AC、WA seedなし。
- 正本Pahcer wall mean / p95 / max: `1.665800 / 2.432061 / 2.696873s`
- 再実行Pahcer wall mean / p95 / max: `1.711112 / 2.259703 / 2.662257s`
- ユーザー判断: 少し改善。
- 直前の、候補押し出しと異周長future-fit比較を含む誤実装は`60,903,541.55`だった。

## paired比較と損失分解

- 当時は実施していない。後続の軽量hybridとのpaired比較は[外周適応付き軽量hybrid](20260807-lightweight-hybrid.md)を正本とする。

## 採否と残る問い

- 修正版は直前の誤実装より改善。
- 当時の軽量baselineとして保持し、後続hybridの直接比較基準にした。
