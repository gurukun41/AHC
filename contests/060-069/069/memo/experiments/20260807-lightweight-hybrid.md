# 外周適応付き軽量hybrid

## 仮説

2秒制限の余裕を優先して再配置とroot rolloutを戻さず、直前の軽量配置へ次の3要素を安全な追加候補として統合する。

- 保存済みtrain / validationで信号が強かったDLP倍率`1.30`を、静的外周率が低い盤面だけで使う。
- 過去に単独で`+0.6975%`だった`grow-and-trim`を戻す。
- 外周の多い制約盤面だけ、将来のcompact slotを塞ぎにくいcriticality championを最大1件追加する。

既存shortlistを先に固定し、追加候補で押し出さず、最短周長以外をfuture-fit前に除外すれば、直前のprotected lightweightを大きく改善しつつ旧full solverにも届くと予想した。

## baselineと変更範囲

直接のbaselineは従来候補保護付き軽量版である。100 seed scoreが完全一致する結果が2件ある。

- [result_20260807_000654.json](../../pahcer/json/result_20260807_000654.json): 合計`6,439,164,489`
- [result_20260807_021319.json](../../pahcer/json/result_20260807_021319.json): 合計`6,439,164,489`、comment「シンプル候補増し」

Treatmentは次のpackageであり、この100 seed実行だけから各要素の個別寄与は分離できない。

1. 初期芝セル数を`G`、芝から池または盤外へ出る4近傍辺数を`E`とする。
2. `100E < 55G`のsmooth盤面ではDLP倍率`1.30`、それ以外では`1.00`にケース開始時から固定する。
3. 通常connected-growthの先頭8 seedを`P+8`まで育て、関節点でない境界セルを削る`grow-and-trim`を作る。その最良1件だけを追加する。
4. `E/G >= 0.55`の制約盤面では、未来3 snapshotの合法compact anchor数から作るcriticality最小候補を最大1件追加する。
5. protected最大6件、expanded最大6件、trim最大1件、criticality最大1件を既存順で保護する。重複除去後、同じ最短周長の最大14候補だけを従来の厳密future-fitで比較する。
6. Compact rescue、NoRegion Push-out、root rollout、既存組の再配置は含めない。

## 実行前に固定した仕様・停止条件

- エージェントは警告付きcompileとseed 0の最小smokeだけを行い、100 seed実行はユーザーが行う。
- score確認後は、ユーザーの新しい明示指示なしにsource、方式、定数を変更しない。
- 最終候補は100 seedのraw paired score、勝敗、下位裾と時間リスクを分けて評価する。

同一sourceの事前smokeはseed 0でscore `57,920,874`、process wall `1.419s`、exit code 0だった。その後の100 seed結果を受けた本ファイルの追記は、ユーザーの「分析して記録してください」という新しい明示指示に基づく。解答sourceと方策は変更していない。

## source / binary / config / input / oracle

- source: `main.cpp`、2,226行
- source SHA-256: `f76b3e40e404b9f2646da820e320fd8bb7014e1f8c9d1550ee69527b76c2d25e`
- Pahcer build binary: `a.out`
- binary SHA-256: `89c64fc395d9ac6889877a305c235703dd478432d1c68aa497b7619ac2b5c835`
- config: `pahcer_config.toml`
- config SHA-256: `4d0af5fcfefce5fe98346f86f1f1f3a95d3a0a846239417b162e767245e013a3`
- compile: `g++ -std=c++20 -O2 main.cpp`
- input: `tools/in/0000.txt`〜`0099.txt`
- Pahcer並列設定: `threads = 0`
- Treatment正本: [result_20260807_095758.json](../../pahcer/json/result_20260807_095758.json)
- Treatment JSON SHA-256: `7b1993c4de798bd4bf954f72258ee2cfed66b381f9dd62062b2a4084c7623f9d`
- protected正本 SHA-256: `3b0793711d1755d619665de86d37981e91e4a0b1ff80b14dc10a55a01ec750c1`
- 旧full oracle: [result_20260803_003818.json](../../pahcer/json/result_20260803_003818.json)
- 旧full oracle SHA-256: `9b064a2c0670a2df7dc2ea153ab50fa323a178e05a4e578318773e74c645c0f3`

`main.cpp`のmtimeは2026-08-07 03:23:46 JST、`a.out`は09:57:57、result開始は09:57:58であり、時刻上も上記sourceからPahcerが直前buildしたbinaryに対応する。ただしhashは実行後に確認したもので、独立した実行前manifestではない。

## 静的検証

- `clang++ -std=c++20 -O2 -Wall -Wextra -pedantic`で警告なしcompile成功。
- 候補上限、重複除去、最短周長filter、criticalityの有効層、DLP倍率の固定条件を独立に監査し、blocking issue 0。
- 実行後の分析では解答のcompile・実行を追加していない。

## 実行結果

- 開始: 2026-08-07 09:57:58 JST
- comment: `test`
- seed: 0〜99、100ケース
- AC: 100/100
- WA seed / case error: 0 / 0
- 合計score: `6,603,615,029`
- 平均score: `66,036,150.29`
- 中央値: `63,302,819.5`
- p05 / p95: `38,472,189.6 / 91,415,105.05`
- worst: seed 67、`27,199,826`
- best: seed 38、`103,080,328`

保存されているseed 0〜99・100/100 ACの134 runの中では、このraw合計が最高である。直前までの最高は[result_20260806_032505.json](../../pahcer/json/result_20260806_032505.json)の`6,534,395,462`だった。

## paired比較と損失分解

同じseed 0〜99のraw scoreをseed番号で対応させた。bootstrap区間は100 seedを200,000回復元抽出したaggregate score比のpercentile 95%区間である。この100 seedは開発中に繰り返し参照した集合であり、区間はこの集合上の安定性を表すだけで、fresh holdoutの汎化保証ではない。

| 比較対象 | 対象合計 | Treatment差 | 勝 / 分 / 負 | paired bootstrap 95% |
|---|---:|---:|---:|---:|
| protected lightweight | 6,439,164,489 | `+164,450,540 (+2.553911%)` | 84 / 0 / 16 | `[+1.959%, +3.169%]` |
| 直前までの保存済み最高 | 6,534,395,462 | `+69,219,567 (+1.059311%)` | 61 / 0 / 39 | `[+0.470%, +1.662%]` |
| 旧full oracle | 6,515,194,836 | `+88,420,193 (+1.357138%)` | 65 / 0 / 35 | `[+0.776%, +1.962%]` |

protected比ではper-seed比の幾何平均が`+2.394904%`、中央値が`+2.538338%`、p05が`-1.354440%`、worstがseed 13の`-6.692791%`だった。正のgross差`+179,244,387`に対して負のgross差は`-14,793,847`で、正負比は約`12.12:1`。baseline絶対score四分位の全てで合計差が正で、最大gainのseed 38を除いても`+2.3891%`なので、少数の高score seedだけに依存した改善ではない。

protected比の主な正寄与はseed 38 `+10,611,605`、98 `+8,820,351`、76 `+6,379,919`、80 `+6,042,772`、82 `+5,601,931`。主な負寄与はseed 13 `-4,087,108`、69 `-2,720,714`、31 `-1,653,295`、48 `-1,306,769`、41 `-1,171,964`だった。

### 静的外周率による層別

各保存inputから実装と同じ式で`E/G`を再計算した。

| 層 | seed数 | protected比 | 勝 / 負 | 旧full比 | 勝 / 負 |
|---|---:|---:|---:|---:|---:|
| smooth `E/G < 0.55` | 70 | `+145,467,903 (+2.894175%)` | 60 / 10 | `+100,084,336 (+1.973422%)` | 52 / 18 |
| constrained `E/G >= 0.55` | 30 | `+18,982,637 (+1.343491%)` | 24 / 6 | `-11,664,143 (-0.808001%)` | 13 / 17 |

全protected比改善の`88.46%`はsmooth 70 seedから生じた。両層とも直接baselineには勝っている一方、旧full超えはsmooth層だけが作り、constrained層では旧fullのrepacking / rescue / rootを含むpackageにまだ届かない。さらに閾値直上の`0.55 <= E/G < 0.65`は9 seed合計でprotected比`+0.0131%`に留まった。これは事後的な小分けの診断であり、閾値や方策を変更する直接根拠にはしない。

今回のrunは外周適応DLP、grow-and-trim、criticality championの同時変更である。smooth層の強い改善は保存済みDLP validationの`+2.018%`、全体の改善は過去grow-and-trim単独の`+0.6975%`と方向整合するが、各要素の因果効果や相互作用はこの比較だけでは確定しない。

### 実行時間

| run | mean | p95 | max | 2秒超 |
|---|---:|---:|---:|---:|
| Treatment | 2.248052s | 3.056409s | 3.545901s | 62/100 |
| protected 00:06 run | 1.665800s | 2.432061s | 2.696873s | 18/100 |
| protected 02:13 duplicate | 1.711112s | 2.259703s | 2.662257s | 15/100 |

Treatmentはprotectedの2 runよりmeanで約31〜35%遅い。smooth層のTreatment meanは`2.0728s`、constrained層は`2.6570s`で、protectedに対するpaired幾何時間比はそれぞれ`1.2262 / 1.6686`。高外周でだけ作るcriticality mapの負荷と整合するが、入力難度と並列競合も含むため単独原因とは断定しない。

`pahcer_config.toml`の`threads = 0`は100ケースを並列実行し、ここでの`execution_time`はtester・対話待ち・CPU競合を含むwallである。コード内solver CPU記録はなく、同一sourceの単seed smoke `1.419s`に対してbatch内seed 0が`2.984s`だった。このwallだけから公式TLEとは断定できないが、2秒安全性も証明できておらず、提出判断上の重大な残存リスクである。

## 採否と残る問い

- 開発100 seedでは、直接baseline、旧full、従来の保存済み最高を全て明確に上回ったため、現行の軽量提出候補として保持する。
- 本run後、解答source、方式、定数は変更していない。ユーザーの新しい明示指示に基づき、結果分析とメモ同期だけを行った。
- 同じ開発100 seedを多数回参照しているため、未使用seedでの再現性は未確認。
- 3変更の個別寄与は未分離。特にconstrained層は旧full比で負であり、package全体の平均だけを各部品の成功と解釈しない。
- 1 threadまたはコード内solver CPUによる2秒余裕は未確認。現在のPahcer wallを公式実行時間として扱わない。
- 公式提出結果ではないため、`submissions.md`は更新しない。
