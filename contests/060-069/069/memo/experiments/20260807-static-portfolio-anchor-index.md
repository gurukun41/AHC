# 静的4-expert portfolio + LegalAnchorIndex

更新日: 2026-08-07 JST

## 状態

- `main.cpp`へ実装し、単一smoke後にユーザーがseed 0〜99を実行済み。source・方針は実行後も変更していない。
- source: 5,584行、SHA-256 `eebe4458c69abaa355e9b76df6687b8fd60d339f06a0889afc7349dfcab9a2a6`
- 実行binary `a.out` SHA-256: `55cb87b6a8c8ed5d39ad84b392370e4d0b0c79b5ace4023be2d092eabcfdd1f6`
- 最新実行正本: [result_20260807_220201.json](../../pahcer/json/result_20260807_220201.json)
- comment `test`、100/100 AC、WA 0、合計`6,621,941,047`、平均`66,219,410.47`。

## 出発点

一つ前のfull + 静的DLPはseed 0〜99を100/100 AC、合計`6,616,387,134`、平均`66,163,871.34`だった。直前lean比`+0.193411%`、旧full比`+1.553174%`で、保存済み100ケースrun中の最高だった。

残差診断では、主な改善余地は受入価格そのものより高外周率ケースの断片化とshape lossだった。

- 通常placementは論理anchor `6,233,655,626`件に対し合法anchorが`89,690,506`件、合法率`1.4388%`。
- placement anchor数とsolver CPUのseed相関は`r=0.709`。
- rescue targetでも論理anchor `1,379,657,044`件を走査していた。
- `E/G>=0.80`ではaccepted ideal feeに対するshape lossが大きく、未配置損失も高面積到着へ偏った。

したがって、未較正な新しいhard rejectを追加するより、保存済み方策から盤面静的expertを選び、意味を変えずに不合法anchor走査を削る方が今回の一回実行制約に適する。

## 比較した方針

### A. 滞在時間を第3軸にした3D adjacency

`(x,y,time)`の接触・空隙をplacement目的へ入れ、近い退去時刻の組を三次元的にまとめる案。時間を含む配置問題で近接関係を明示する考え方とは整合するが、現在の境界costとの二重計上、尺度較正、CPU増を新規に検証する必要がある。単一smokeしか許されない今回には不確実性が大きく、設計候補として保留した。

参考: [3D adjacency-based placement](https://www.cecs.uci.edu/~papers/aspdac06/pdf/p396_4B-5.pdf)

### B. 未来需要別future-fit

正方形の空き数だけでなく、未来`P`分布に対応する最小周長templateの可置数を重み付けする案。高外周率ケースのshape残差へ直接届く一方、過去の「独立未来料金」評価は逐次占有を表さず`-1.1388%`だった。候補比較のwinner's curseもあり、fresh検証なしでは昇格しない。

### C. 頻繁なre-solving / repair拡張

DLPやrootをより頻繁に解き直し、NoRegion以外でも限定repackingする案。re-solvingはオンライン資源配分で有力だが、選択的な発火と損失境界が重要である。現solverは既にCPU p95 `2314ms`であり、追加rolloutは時間リスクが大きい。今回の採用対象から外した。

参考: [A Re-solving Heuristic with Uniformly Bounded Loss](https://arxiv.org/abs/1802.06192)、[Online allocation of reusable resources](https://arxiv.org/abs/2212.02855)

### D. 静的expert portfolio + 意味保存anchor index

既に保存されたbinary/cacheだけで方策差を合成でき、新しい予測器やscore proxyを導入しない。既知のdurationを持つ動的packingでは、残存時間に応じた配置とfragmentation管理が本質的であり、ケース開始時に地形別方策を固定するのはturn別の不安定な切替より因果境界が明確である。今回はこちらを採用した。

参考: [A provably efficient algorithm for dynamic storage allocation](https://doi.org/10.1016/0022-0000(89)90031-7)、[Dynamic Vector Bin Packing with Known Item Departure Times](https://arxiv.org/abs/2304.08648)

## 採用した静的portfolio

初期芝セル数を`G`、芝から池または盤外へ出る4近傍辺数を`E`とする。到着列を見る前に一度だけ次を選び、ケース中は固定する。

| expert | 条件 | DLP scale | placement |
|---:|---|---:|---|
| 0 | `100E < 55G` | 1.30 | full baseline |
| 1 | `55G <= 100E < 70G` | 1.25 | full baseline |
| 2 | `70G <= 100E < 80G` | 1.00 | full baseline |
| 3 | `80G <= 100E` | 1.00 | 保存candidate p2 |

expert 3のp2は保存build key `6282b68372acdeb9f63a`と一致させた。

| 定数 | baseline | p2 |
|---|---:|---:|
| global incremental shortlist | 3 | 5 |
| 最終shortlist上限 | 6 | 8 |
| connected growth seed上限 | 16 | 24 |
| grow-and-trim追加セル | 8 | 12 |
| grow-and-trim試行上限 | 8 | 8 |
| future-fit minimum snapshot重み | 0.25 | 0.50 |

DLP倍率は実到着とsynthetic rolloutの`evaluate_arrival_decision()`で各1回だけ適用する。通常admissionを通らないPush-out経済gateもraw shadowへ入口で1回だけ掛け、二重適用しない。placement configは全実到着、synthetic rollout、root alternativeへ共通である。

## 保存cacheでの事前合成

v29 validation seed 1700〜1999の既存cacheから、上の排他的portfolioを合成した。比較対象は一つ前の1.30 / 1.00静的DLP版である。

| 指標 | 差・値 |
|---|---:|
| score差 | `+18,308,191 (+0.087686%)` |
| 勝/分/敗 | `32 / 251 / 17` |
| seed ratio p05 | `0.994343` |
| worst | `0.976691` |
| candidate CPU mean / p95 / max | `1580.244 / 2107.783 / 2721.582ms` |

枝別では`0.55<=E/G<0.70`のDLP 1.25が34 seedで`+11,731,543 (+0.637%)`、`E/G>=0.80`のp2が18 seedで`+6,576,648 (+0.8677%)`だった。DLP 1.25を`0.70<=E/G<0.80`へ広げる案は`+225,936`、5勝10敗と弱く、採用しなかった。探索上のplacement bestは候補幅12だったが、p2との差は300 seed合計`+132,212`に対してp95が約39ms重いため、p2を選んだ。

重要な制限として、閾値とp2の選択にも同じvalidation cacheを参照している。この`+0.087686%`はpost-hocな事前根拠であり、未開封holdoutや今回binaryのfresh性能保証ではない。

## 開発100 seed実測

ユーザーが現在sourceをPahcer 0.3.1、seed `[0,100)`、`threads=0`、`g++ -std=c++20 -O2 main.cpp`で実行した。開発中に繰り返し使ったseed 0〜99であり、fresh holdoutではない。

| 指標 | 今回 | 一つ前との差 |
|---|---:|---:|
| 合計score | `6,621,941,047` | `+5,553,913` |
| 平均score | `66,219,410.47` | `+55,539.13` |
| 改善率 | — | `+0.083941778%` |
| 勝/分/敗 | — | `17 / 76 / 7` |
| seed ratio p05 / worst | `0.987075 / 0.966126` | — |
| 100,000回paired bootstrap 95%区間 | `[-0.0770%, +0.2570%]` | 0をまたぐ |

保存cacheの事前合成`+0.087686%`に対し、実測は`+0.083942%`で方向・効果量が近かった。ただし、今回100 seedも開発集合であり独立再現ではない。裾は事前cacheのp05 `0.994343`、worst `0.976691`より悪く、合計改善だけで安全性が確定したとは扱わない。

### expert別寄与

| expert | seed数 | score差 | 枝内改善率 | 勝/分/敗 |
|---:|---:|---:|---:|---:|
| 0: `E/G<0.55` | 70 | `0` | `0%` | `0 / 70 / 0` |
| 1: `0.55<=E/G<0.70` | 13 | `+3,278,544` | `+0.456272%` | `8 / 0 / 5` |
| 2: `0.70<=E/G<0.80` | 6 | `0` | `0%` | `0 / 6 / 0` |
| 3: `E/G>=0.80` | 11 | `+2,275,369` | `+0.537166%` | `9 / 0 / 2` |

改善量の約59%がexpert 1、約41%がexpert 3から生じた。変更対象外のexpert 0・2は76/76 seedで一つ前のsourceと完全同点だった。これは静的portfolioの排他的分岐と、全expertで有効な`LegalAnchorIndex`がscore方策を変えないという設計の強い実測確認である。

最大gainはseed 49・expert 1の`+2,767,572 (+4.6898%)`、最大lossはseed 84・expert 1の`-2,056,178 (-3.3874%)`。expert 3の最大gainはseed 35の`+1,549,315`、最大lossはseed 11の`-1,581,277`だった。

### 過去基準との位置

| 基準 | score差 | 改善率 |
|---|---:|---:|
| 一つ前のfull + 静的DLP | `+5,553,913` | `+0.083942%` |
| 直前lean hybrid | `+18,326,018` | `+0.277515%` |
| 旧full | `+106,746,211` | `+1.638419%` |
| protected lightweight | `+182,776,558` | `+2.838514%` |

保存されている100/100 ACの138 run中でraw合計1位となり、直前の1位を更新した。

## 実行時間と整合性

内部solver CPUは一つ前の同じ100 seed記録から明確に短縮した。

| solver CPU | 一つ前 | 今回 | 変化 |
|---|---:|---:|---:|
| mean | `1644.464ms` | `1240.126ms` | `-24.59%` |
| p95 | `2314.446ms` | `1747.041ms` | `-24.52%` |
| max | `2610.364ms` | `2054.115ms` | `-21.31%` |
| 2秒超 | `13/100` | `1/100` | `-12 cases` |

Pahcer wallはmean `2.187174s`、p95 `3.687568s`、max `4.836715s`で、前runのmean `2.154865s`、p95 `3.096572s`より良化していない。`threads=0`の並列実行とI/O・待機の影響を含むため、計算量評価はコード内solver CPUを主資料とする。score変更も同時に入るので純粋なmicrobenchmarkではないが、約24.6%の短縮は`LegalAnchorIndex`の狙いと整合する。

内部CPUの2秒超は13件から1件へ減ったが、seed 67・expert 3が`2054.115ms`なので、制限時間安全性が完全に解消したとはしない。

100 log全件で次を確認した。

- 46種類の`error` / `mismatch`診断は全て0。
- expert分布は`70 / 13 / 6 / 11`、`E/G`閾値との不一致0。
- config tupleはexpertごとに設計値と完全一致。
- 各stderrに診断行と`Score`行が存在し、case error messageも全件空。

## LegalAnchorIndex

通常template placementとrescue targetの合法性を、全anchorへの矩形累積和照会から「合法base_yだけを列挙する」索引へ変更した。候補集合、`base_x/base_y`順、tie-break、診断上の論理anchor数は変えない。

1. N=50の各行を1個の`uint64_t` blocked maskにする。
2. 幅`w`について、bit `y`が窓`[y,y+w)`の衝突を表すmaskをshift ORで作る。
3. 行方向に冪等OR sparse tableを作り、任意高さの矩形を重なる2区間で照会する。
4. 主矩形と端数矩形のinvalid maskをORし、補集合の合法bitを`ctz`で昇順列挙する。

`extra_rect`が空のとき、最大shift、shape幅mask、配列境界を含めて静的監査した。通常placementでは池+owner、rescue targetでは池だけをblockedとし、後段の占有数・移動費rankingは従来どおり累積和で計算する。索引は1 call約122KBだが、100 seed実測ではsolver CPU meanが`1644.464ms`から`1240.126ms`へ短縮した。

## 監査と凍結条件

- 4 expertの不等号境界、p2の6定数、適用範囲を保存artifactと照合。
- DLP倍率が実到着、synthetic arrival、Push-outへ各1回だけ届くことをcall-site監査。
- `LegalAnchorIndex`の合法集合と列挙順が旧prefix版と一致することをbit式・境界ごとに監査。
- `git diff --check`通過。
- 実行後は100/100 AC、全診断error 0、全expert config一致。
- blocking issue 0。score上の主な不確実性は開発100 seed bootstrap区間が0をまたぐことと、post-hoc portfolioであること。

単一smokeとユーザーの100 seed実行後も、解答source・定数・方針は変更していない。以後の改善は新しい明示指示へ分離する。
