# smooth限定v31 polish復元 v33

更新日: 2026-08-08 JST

## 状態

- 直前の実行正本はv32の`pahcer/json/result_20260808_003555.json`。seed 0〜99を100/100 AC、合計`6,634,181,482`、平均`66,341,814.82`、WA 0。
- v33は、その結果を受けたユーザーの新しい明示指示に基づく推奨修正版である。
- 実装・静的監査後の`main.cpp`は6,534行、SHA-256 `3709a9de4a71111ff1a116ecfde7c4fd349a459b1bb179275d1e5ccf22d6461d`。
- エージェントは全source・記録を凍結した後、警告付きcompileとseed 0 smokeを一回だけ実行した。exit 0、Score`55,762,976`、solver CPU`1281.788ms`、全identity/error 0で、その後sourceを変更しなかった。
- ユーザーの`test`実行は`pahcer/json/result_20260808_012614.json`。seed 0〜99を100/100 AC、合計`6,641,661,858`、平均`66,416,618.58`、WA 0である。現在sourceは上記freeze SHAから不変。
- 本結果追記はユーザーの新しい「確認と記録」指示だけに基づく。解答source・方針・定数は変更せず、エージェントは解答プログラムを追加実行していない。
- 現系列の区切りとして、現行solver・分割済みメモ・実験記録・再現用artifactをsubject `AHC069: checkpoint v33 smooth-gated solver`でコミットする。ignore済みinput / JSON / binaryと他コンテストは対象外。

## v32実測の分解

v31は合計`6,634,405,584`、平均`66,344,055.84`。v32はv31比`-224,102 (-0.003378%)`、39勝18分43敗だった。ただし地形別の符号は逆である。

| 初期`E/G` | seed数 | v32−v31 | 勝/分/敗 |
|---|---:|---:|---:|
| `<0.55` | 70 | `-7,480,376` | `25/17/28` |
| `[0.55,0.625)` | 8 | `-526,864` | `2/1/5` |
| `[0.625,0.70)` | 5 | `+1,711,877` | `2/0/3` |
| `[0.70,0.80)` | 6 | `+1,432,349` | `3/0/3` |
| `>=0.80` | 11 | `+4,638,912` | `7/0/4` |

非smooth 30 seedでは、v32がv31を合計`+7,256,274`上回った。一方smooth 70 seedでは、v32で同時導入したdense閾値変更、descent Pareto、zero plateau、component guardの合成差が`-7,480,376`だった。個別寄与は分離できないため、正だった静的地形gateだけを残し、smooth側は一括して実測済みv31方策へ戻す。

## 比較した修正案

### A. v31へ全面復帰

v31の平均はv32よりわずかに高いが、非smooth 30 seedで観測した`+7.256M`の静的保護を失う。高外周率側の大きな負の裾を再導入するため不採用。

### B. v32を維持

全体はv31とほぼ同点だが、smooth側の合成差が明確に負であり、追加探索・追加profile評価のCPUも必要になる。不採用。

### C. `E/G<0.625`だけv31 polish

同じ100 seedのcasewise反実仮想では合計`6,642,188,722`、平均`66,421,887.22`となる。しかし`0.625`は今回の結果を見て新設するpost-hoc境界であり、既存の静的expert境界ではない。8 seedだけを根拠に方策境界を増やすため不採用。

### D. 既存smooth expertだけv31 polishへ戻す

`case_static_expert==0`、すなわち既存境界`E/G<0.55`だけでv31 polishを使い、それ以外はv32と同じくpolishを止める。新しい閾値を作らず、正だった静的保護と実測済みv31の候補選択を合成できるため採用。

## 採用実装

実ターンで旧方策が経済的にAcceptした`P>=50`の`ConnectedGrowth` / `GrowAndTrim`だけを対象にし、次を行う。

1. 初期盤面から固定した`case_static_expert==0`の場合だけpolishを有効にする。
2. dense全盤面走査は、最小周長までの厳密な料金改善上限`U>=10,000`かつcase先着24回だけ実行する。
3. strict 1-cell perimeter descentはdense予算と分離し、最大8 step、全中間改善形を候補に残す。
4. denseとdescentを周長tier別の共通shortlistへ入れ、丸め後料金が旧料金をstrictに上回る候補だけ残す。
5. 旧案と同じ3 snapshotのscalar future-fitでtier内最良を選び、旧案以上なら採用する。不合格なら次tier、最後までなければ旧案へ戻る。
6. polish採用時も旧connected案をrootの第1runner-upに残し、Compact rescueとnormal rootの旧発火機会を維持する。
7. synthetic rolloutではpolish候補を生成しない。

v32固有のzero-gain plateau、24要素square Pareto、component-capacity guardは、定数・helper・候補統合・source enum・rollback・損失分解・stderr診断を含めて完全撤去した。future-fitはside`{2,3,4,5,6,8,10,12}`の加重scalarへ戻し、通常expertの最悪snapshot重み25%、高外周率placement expertの50%は従来どおり維持する。

## 同一100 seed上のcasewise反実仮想

v33は`E/G<0.55`の70 seedでv31、それ以外の30 seedでv32と同じ方策を選ぶ設計である。同じ入力・保存結果をcasewiseに合成すると、次になる。

```text
v31 total                              6,634,405,584
+ v32の非smooth 30 seedでの改善          7,256,274
= casewise hybrid total                6,641,661,858
average                                  66,416,618.58
v31比                                  +0.109373%
v32比                                  +0.112755%
```

これは実行前の時点では新規100 seed実測ではなく、v31/v32が各地形で対応方策と同じ挙動をすることに基づく同じ開発100 seed上の反実仮想だった。後述のユーザー実測で完全一致を確認したが、fresh seed性能や赤パフォーマンスを保証しない。

## ユーザー100 seed実測

[result_20260808_012614.json](../../pahcer/json/result_20260808_012614.json)を正本とする。comment `test`、start time `2026-08-08T01:26:14.473042+09:00`、seed 0〜99、100/100 AC、`wa_seeds=[]`、各caseの`error_message`は全て空である。

| 実装 | 合計score | 平均score | v33−比較対象 | v33の勝/分/敗 |
|---|---:|---:|---:|---:|
| 4-expert + `LegalAnchorIndex` | `6,621,941,047` | `66,219,410.47` | `+19,720,811 (+0.297810%)` | `48/34/18` |
| v31 全地形polish | `6,634,405,584` | `66,344,055.84` | `+7,256,274 (+0.109373%)` | `14/71/15` |
| v32 smooth plateau/Pareto | `6,634,181,482` | `66,341,814.82` | `+7,480,376 (+0.112755%)` | `28/47/25` |
| v33 smooth限定v31 polish | `6,641,661,858` | `66,416,618.58` | — | — |

- 実測合計・平均は実行前に固定したcasewise反実仮想と完全一致し、予測差は`0`。
- 入力から`E/G`を再計算し、smooth 70 seedではv31 score、非smooth 30 seedではv32 scoreとseedごとに比較した。100/100 seedが期待側と一致し、mismatchは`0`。したがって静的gateとsmooth側復元はscore軌跡まで想定どおり動いた。
- v31比の正差合計は`+13,749,520`、負差合計は`-6,493,246`。v32比は正差`+33,467,262`、負差`-25,986,886`。
- 保存済み100/100 AC runは141件となり、v33はraw合計1位。v31が2位、v32が3位である。ただし同じ開発100 seedを繰り返し参照した結果で、fresh汎化や赤パフォーマンスの保証ではない。
- Pahcer `execution_time`はmean`2.2383s`、p95約`4.3210s`、max`6.0598s`。並列tester・I/O・プロトコル待機を含むwall値であり、内部solver CPUの100 seed logはJSONにないため、CPU tailの因果比較には使わない。
- JSONにbinary/source hashやtagはないが、結果確認時の`main.cpp`は6,534行、freeze SHA-256と一致した。

## 静的監査

- `plateau` / Pareto profile / component-capacity関連識別子は`main.cpp`から0件。
- denseは`U>=10,000`、case最大24実走査。strict descentは別予算。
- old future-fitは既に通常候補比較で評価済みなら保存scalar値を再利用し、未評価時だけ3 snapshotを評価する。
- 周長tier内の全候補はscalar future-fitだけで比較し、旧値以上のtierだけ採用する。
- smooth gateは`candidate = static_filtered + eligible`、dense gateは`eligible = value_filtered + budget_skip + actual_scan`の保存則診断を維持する。
- source、root上書き、old rollback、accepted loss分解はdense / descentの2種だけに同期した。
- strict swap helperの分割は残るが、v31と同じ候補集合、gain、tie順を保つ防御的refactorである。
- read-only独立監査でblocking issueは0。`git diff --check`も出力0。

## 実行後の固定状態

エージェントの最終smokeには次の固定コマンドだけを使用した。

```bash
g++ -std=c++20 -O2 -DNDEBUG -Wall -Wextra -Wshadow -Wpedantic main.cpp \
  -o /private/tmp/ahc069-static-polish-gate-v33 && \
./tester /private/tmp/ahc069-static-polish-gate-v33 < tools/in/0000.txt
```

大規模100 seed比較はユーザーが実行した。比較正本はv31、v32、v33を別々に保持する。本追記後はユーザーの次の明示指示まで解答source・方針・定数を変更せず、追加実行もしない。
