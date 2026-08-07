# wide spatial placement v25（結果記録）

## 仮説

admission、placement、repackingを分離し、現行admissionがAcceptedとした到着だけについて、広い候補集合をcell×time空間DLPで順位付けすれば将来配置を改善できると考えた。

## baselineと変更範囲

`AHC069_ENABLE_WIDE_SPATIAL_PLACEMENT`時だけ`SpatialDlpModel`を使い、baseline候補を必ず残した。Fullは周長`+8`、全anchor、connected-growth、grow-and-trim、baselineより低料金の候補まで比較し、SameFeeは同料金候補だけに絞った。因果Controlを含む全armでroot actionを無効化した。

## 実行前に固定した仕様・停止条件

詳細な実行前仕様は[履歴08](../history/08-spatial-placement.md)にある。

## source / binary / config / input / oracle

結果JSON:

- default: `pahcer/json/result_20260803_182700.json`
- causal Control: `pahcer/json/result_20260803_182919.json`
- SameFee: `pahcer/json/result_20260803_183041.json`
- Full: `pahcer/json/result_20260803_183320.json`

defaultの100 seed oracleは`pahcer/json/result_20260803_003818.json`、合計`6,515,194,836`。baseline復元基準の`main.cpp` SHA-256は`086cb6cc2e24848c77a4b766ab8dde433dbf16469095cca557f848bdff091c6a`。

## 静的検証

ASan/UBSan、Clang Static Analyzer、警告付きC++17/C++20 build、保存則、合法性、決定性は全て正常だった。

## 実行結果

| arm | 合計score | causal Control比 | 勝/分/負 |
|---|---:|---:|---:|
| default互換 | `6,515,194,836` | 参考値 | oracleと100/100完全一致 |
| causal Control | `6,491,323,827` | — | — |
| SameFee | `6,197,493,979` | `-4.5265%` | `6/0/94` |
| Full | `6,232,940,594` | `-3.9804%` | `9/0/91` |

内部CPUはcausal Control平均`594.9ms`、SameFee平均`1,359.5ms`、Full平均`1,514.6ms`。Fullはp95 `1,921.7ms`、最大`2,114.6ms`でruntime gateも不合格だった。

## paired比較と損失分解

FullはSameFeeより`+35,446,615`、`+0.5720%`、63勝37敗だったため、低料金候補まで広げる部分はSameFeeより有効だった。しかし予測上のFull改善`+507,990,969`に対し実差は`-258,383,233`で符号が逆転した。restricted future columns、6反復、96 sample、16 windowによるprice holeと予測の未較正が主な疑いである。

## 採否と残る問い

Full/SameFeeとも棄却し、無フラグbaselineを提出候補として維持した。ユーザーが`main.cpp`をbaselineへ戻したため、v25実験コードは現ソースに残っていない。
