# smooth connected polish + locked plateau escape v32

更新日: 2026-08-08 JST

## 状態と比較基準

- 直前v31の実行正本は`pahcer/json/result_20260807_232956.json`。seed 0〜99を100/100 AC、合計`6,634,405,584`、平均`66,344,055.84`、WA 0。
- v32はこの結果を受けたユーザーの新しい明示指示で実装した。
- 最終実行前に凍結した`main.cpp`は7,162行、SHA-256 `b3399f41a7ebb719147cf5909f37cecf2355b549e73c6e33e90952621b898928`。
- 残差、文献対応、全コード経路の独立read-only監査はいずれもblocking 0。`git diff --check`も出力0だった。
- エージェントは全実装・記録・静的監査後に警告付きcompileとseed 0 smokeを一回だけ実行した。Score `56,759,306`、solver CPU `1263.353ms`、全identity/error 0で、その後sourceを変更しなかった。
- ユーザーの`test`実行は`pahcer/json/result_20260808_003555.json`。seed 0〜99を100/100 AC、合計`6,634,181,482`、平均`66,341,814.82`、WA 0。実行時sourceは上記freeze SHAから不変である。
- 本追記はユーザーの新しい「確認と記録」指示に基づく。解答source・方針・定数は変更せず、エージェントは解答プログラムを追加実行していない。

## 100 seed結果

| 比較 | total | average | 差 | 勝/分/敗 |
|---|---:|---:|---:|---:|
| v31 | `6,634,405,584` | `66,344,055.84` | — | — |
| v32 | `6,634,181,482` | `66,341,814.82` | `-224,102 (-0.003378%)` | `39/18/43` |

- 正差合計`+39,736,406`、負差合計`-39,960,508`で、ほぼ相殺してわずかにv31を下回った。
- seed ratioのworst / p05 / median / bestは`0.948383 / 0.969832 / 1.000000 / 1.061131`。
- worstはseed 7の`-3,188,115`、bestはseed 71の`+4,036,837`。
- 直前4-expert版`result_20260807_220201.json`比では`+12,240,435 (+0.184847%)`、44勝37分19敗。v31をわずかに下回るが、保存済み100/100 AC 140 run中ではraw合計2位である。
- Pahcer側execution timeはmean `1.8409s`、p95 `2.9076s`、max `3.5636s`。並列負荷とプロトコル待機を含むため、内部solver CPUとの代用比較には使わない。

初期外周率別のv32−v31差は次のとおり。

| 初期`E/G` | seed数 | 差 | 勝/分/敗 |
|---|---:|---:|---:|
| `<0.55` | 70 | `-7,480,376` | `25/17/28` |
| `[0.55,0.625)` | 8 | `-526,864` | `2/1/5` |
| `[0.625,0.70)` | 5 | `+1,711,877` | `2/0/3` |
| `[0.70,0.80)` | 6 | `+1,432,349` | `3/0/3` |
| `>=0.80` | 11 | `+4,638,912` | `7/0/4` |

非smooth 30 seedの合計`+7,256,274`が、smooth 70 seedの`-7,480,376`をほぼ相殺した。したがって静的な高外周率保護は想定どおり正に働いた一方、smooth側へ同時導入したdense予算変更、descent Pareto、plateauの合成差はこの100 seedで負だった。ただし同時導入なので各要素の個別寄与は分離できず、繰り返し参照した開発100 seed上の結果である。結果記録だけを行い、この指示では採否変更や実装修正をしない。

## v31の残差

v31と直前版のcase差を初期外周率`E/G`で分けると、polishを地形全体へ適用した平均改善の裏に大きな負の裾が残っていた。

| 初期`E/G` | seed数 | v31差 |
|---|---:|---:|
| `<0.55` | 70 | `+18,587,452` |
| `[0.55,0.625)` | 8 | `+526,864` |
| `[0.625,0.70)` | 5 | `-1,711,877` |
| `[0.70,0.80)` | 6 | `-298,990` |
| `>=0.80` | 11 | `-4,638,912` |

`E/G<0.55`だけでpolishを有効にし、それ以外を直前配置へ戻す同一100 seed上の反実仮想は、現行v31より約`+5.824M`だった。これは同じ開発集合を再利用したpost-hoc値でありfresh性能推定ではないが、負のtailを初期盤面だけで分離する実装根拠にはなる。

候補source別では、denseだけが選ばれた6 caseは全て`E/G<0.55`で合計`+5,714,440`。descentだけの21 caseは17勝でも合計`-1,339,173`となり、少数の大損が多数の小勝ちを上回った。一方、denseとdescentの両方が動いた67 caseは`+8,089,270`なので、descentを全面停止するのではなく将来形状のtail guardを強める。

v31のdense実走査はcase先着24回で、試行が到着列前半へ偏った。理論最大料金改善

```text
U = fee(V,P,Lmin) - fee(V,P,Lold)
```

のgateを`10,000`から`50,000`へ上げる保存log上の再集計では、呼出しを約6.9%減らしながら、実走査へ渡る`U`合計が約57.9%増えた。先着順自体は維持しつつ、希少な全盤面走査を高価値到着へ移す。

## 周長局所最適の定式化

面積`P`が固定された4近傍領域`S`の周長は、内部隣接辺数を`E(S)`として

```text
L(S) = 4P - 2E(S)
```

である。非関節セル`r`を外し、削除後も領域へ接するfrontierセル`a`を加えると、削除前のselected近傍数を`k_r`、削除後の追加セル近傍数を`k_a`として

```text
L(new) - L(old) = 2(k_r - k_a)
```

となる。v31は`k_a>k_r`だけを反復するstrict 1-exchange descentなので、停止形は1交換局所最適である。局所最適から同じ周長の`k_a=k_r`交換を経由し、その先で再びstrict descentすれば、面積・連結・途中周長を悪化させず別のbasinへ移れる。

この構造は、非正gainを含む交換列とpass中の最良prefixを使うKernighan–Lin / Fiduccia–Mattheysesの考え方を、AHC069の連結polyominoへ制約付きで写したものになる。今回は一時的な負gainを許さず、zero-gainだけの保守的なVariable Neighborhood Searchとして実装する。

## 比較した方針

### A. v31を維持し、静的対象領域だけ修正

最も安全だが、strict 1-swap局所最適に残る形状損へ新しく届かない。`E/G<0.55`限定は採用するが、これだけでは飛躍候補にならない。

### B. strict descentのstep数・候補幅を増やす

保存15,330試行でstrict stepの最大は6、現上限は8なので、深さを増やしても候補空間はほぼ変わらない。局所最適の外へ出ないため棄却する。

### C. zero-gain locked plateau escape

strict descentが上限到達ではなく本当に停止した形だけを起点にする。周長同値swapの両端をlockし、depth 2・beam 4で別の近傍へ移る。各plateau stateからstrict descentを再開し、元のstrict終端より短い形だけを候補化する。zero途中形は採用候補にしない。

面積、4連結、途中周長非悪化を構造的に保ち、既存v31候補も残せるため採用する。

### D. 負gainを許す本来のFM/KL pass

`gain=-1`相当まで許せばより深い谷を越えられるが、途中周長悪化と探索幅が増える。今回の一回実行ではCと同時導入せず、Cの発火率・出口率・tailを見てから独立実験にする。

### E. qセルdestroy/repair VNS・ALNS

境界を2〜4セル壊し、frontier beamで同面積へ修復すれば、交換では届かない変形を作れる。ただし連結制約下の状態数と候補選択差が大きく、Cより因果境界が広い。次段候補として保留する。

### F. seeded graph cut / max-flow

固定面積の周長目的はcutと近いが、exact cardinalityと4連結を同時に課す必要がある。反復max-flowを通常到着へ入れるには重く、現行near-template局所探索より実装・CPU riskが大きいため棄却する。

### G. admission、deadline arena、指数価格、複数組repackingの同時変更

過去のActualFeeRejected救済、deadline layer、空間DLP、複数assignmentの失敗と因果境界が重なる。今回はaccepted connectedの形状改善だけに閉じ、受入価格とrepackingは変更しない。

## 採用したv32合成方針

### 1. polishをsmooth expertへ静的限定

- `case_static_expert==0`、すなわち初期`E/G<0.55`だけ有効。
- 判定は到着前に一回だけ固定し、到着列や途中scoreを見て変更しない。
- それ以外のexpertではv31のpolish候補を作らず、従来placementを保つ。
- `.70<=E/G<.80 && R<.060`の限定root expert自体は維持する。

### 2. dense探索予算を高価値到着へ移す

- `U>=50,000`だけ全盤面dense box走査を行う。
- case最大24回、box slack、anchor shortlist、Tarjan trimはv31と同じ。
- strict descentはdenseの24回予算と分離する。

### 3. locked plateau escape

- v31 strict descentが8-step capではなく真に尽きた終端だけを起点にする。
- zero-gain swapだけを許し、remove/add両セルを同一path中でlockする。
- depth 2、beam 4。
- incremental delta、absolute delta、固定座標両端から最大4個の多様なzero swapを作る。
- 各depthで累積cost上位beam 4だけを残し、その各stateで最大8 stepのstrict descentを再開する。
- lockはzero-swap列だけに適用し、stateからのstrict descentへは引き継がない。
- strict終端より周長が短い各中間形だけを候補へ出す。
- plateau後に残る理論最大料金改善が`50,000`未満なら探索しない。
- root展開とzero子状態を合わせてcase最大32状態。到着進行4区間へ累積`8/16/24/32`状態ずつ解放し、序盤だけで使い切らない。
- zero swapが存在しないroot展開も状態予算へ数え、失敗局所最適の反復Tarjan走査を防ぐ。

### 4. 将来断面Pareto guard

旧connected形とdescent / plateau候補を、同じ未来到着分布の1/6・3/6・5/6分位snapshotで比較する。

descentとplateauは、各snapshotの各side

```text
side = {2,3,4,5,6,8,10,12}
```

について、置ける空き正方形数が旧形を一つも下回らないことを必須にする。従来の加重scalar future-fitだけでは、大きいsideの改善で小さいsideの悪化を相殺できたためである。

plateauにはさらに、各snapshotで空き4連結成分の次のprofileが旧形以上であることを要求する。

- `min(largest_component,150)`
- `q={25,50,100,150}`ごとの`sum floor(component_size/q)`

後者は厳密packing数ではなくcomponent capacity proxyなので、最大成分と併用する。denseはv31挙動を保つため従来scalar future-fit guardのままとする。

### 5. v31候補と後段探索の保護

- 既存dense/descent builderとplateau builderを分離する。
- profile通過時だけ同じ周長tierへunionし、完全tieと重複では既存v31候補を優先する。
- old connectedは常にnormal alternatives先頭へ残す。
- Compact rescueとnormal rootの発火はold周長、rescue destination rankingはold cellsを参照する。
- direct gainは改善後料金を基準にする。
- RejectをAcceptへ変えず、synthetic rolloutではpolish探索をしない。

## 静的不変条件と診断

- plateauの各swap後に面積、池・占有、4連結を再検証する。
- strict候補は丸め後料金がoldよりstrictに高いものだけ残す。
- profile guardでtierが落ちても、次の長いtierとold候補を試せる。
- `connected_polish_candidate = static_filtered + eligible`を診断する。
- `dense eligible = value_filtered + budget_skip + actual_scan`を診断する。
- `plateau eligible = value_filtered + budget_skip + attempt`を診断する。
- `plateau states = attempts + zero_swaps`、global state counterとの一致、32超過0を診断する。
- descent / plateauのprofile rejection、plateau component rejection、source採用、rollback後のsource置換を個別計数する。
- 損失分解へplateau採用数、ideal fee、initial fee、周長超過を独立追加する。

## 文献

- Kernighan, Lin, “An Efficient Heuristic Procedure for Partitioning Graphs”: https://doi.org/10.1002/j.1538-7305.1970.tb01770.x
- Fiduccia, Mattheyses, “A Linear-Time Heuristic for Improving Network Partitions”: https://limsk.ece.gatech.edu/book/papers/fm.pdf
- Mladenović, Hansen, “Variable Neighborhood Search”: https://doi.org/10.1016/S0305-0548(97)00031-2
- Lourenço, Martin, Stützle, “Iterated Local Search”: https://arxiv.org/abs/math/0102188
- Ropke, Pisinger, “An Adaptive Large Neighborhood Search Heuristic”: https://doi.org/10.1287/trsc.1050.0135
- graph-cut比較: https://doi.org/10.1109/CVPR.2008.4587440

KL/FMの一般論をそのまま使うのではなく、AHC069ではexact面積、池・占有、4連結、丸め料金、将来空き形状を追加制約にする。v32は負gain passではなくzero plateauだけを採用した限定版である。

## 判定と残る不確実性

seed 0 smokeは構文・AC・診断identity・概算時間の行動確認にだけ使い、score判断には使わなかった。その後の同一seed 0〜99 paired比較は100/100 ACで、v31比`-0.003378%`と実質同水準ながら点推定は負だった。JSONには内部solver CPUとsource funnelの100 seed logが保存されていないため、計算量tailとplateau採用寄与はこの記録から確定できない。同じ開発100 seedを繰り返し参照しており、赤パフォーマンスやfresh汎化性能は保証しない。本指示は確認・記録だけなので、v31/v32の採否変更は行わない。
