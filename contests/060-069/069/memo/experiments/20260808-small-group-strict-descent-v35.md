# small-group strict perimeter descent v35

更新日: 2026-08-08 JST

## 状態

v33の100 seed正本は`pahcer/json/result_20260808_012614.json`で、100/100 AC、合計`6,641,661,858`、平均`66,416,618.58`である。v34のsame-gain multi-startは`result_20260808_023641.json`で平均`66,409,477.31`、v33比`-714,127 (-0.010752%)`だったため棄却し、ユーザーが`main.cpp`をv33へ戻した。

v35は、そのv33を基準に候補portfolio・case expert・admissionを増やさず、既存のstrict 1-cell perimeter descentを小規模connected配置へ一般化する。高価なdense boxの`P>=50`制限は維持する。ユーザーの100 seed実測で平均`66,438,655.07`、v33比`+2,203,649 (+0.033179%)`、19勝81分0敗となったため、現在の開発100 seed incumbentとする。

## 100 seed実測

- 正本: `pahcer/json/result_20260808_030825.json`
- 開始時刻: `2026-08-08T03:08:25.057793+09:00`
- comment: `test`
- seed: 0〜99、100/100 AC、WA 0
- 合計: `6,643,865,507`
- 平均: `66,438,655.07`
- v33差: `+2,203,649 (+0.033179%)`
- 勝分敗: `19 / 81 / 0`
- positive / negative gross: `+2,203,649 / 0`
- seed ratio p05 / worst: `1.000000 / 1.000000`
- Pahcer wall mean / p95 / max: `1.726351 / 2.677900 / 3.679066`秒

scoreが変わったseedとv33差は、7 `+48,448`、12 `+1,551`、18 `+1,662`、34 `+1,395`、38 `+210`、42 `+2,053,920`、46 `+73,809`、50 `+1,499`、52 `+5,265`、56 `+1,102`、58 `+1,301`、60 `+1,377`、62 `+1,164`、64 `+1,185`、68 `+3,866`、73 `+2,332`、78 `+961`、82 `+2,520`、89 `+82`である。seed 42を除いても18勝81分0敗、合計`+149,729`だが、全改善の93.205%はseed 42の`+2,053,920`が占める。

したがって「小規模strict descentはこの100 seed上で負seedなし」という安全信号と、「合計効果の大半は1 seedの盤面連鎖」という集中を分けて扱う。同じ開発100 seedを繰り返し参照しているためfresh一般化保証ではない。負seedがないことを根拠に追加gateや探索幅を後付けせず、この単一差分をincumbentとして維持する。

Pahcer wallはv33実行のmean `2.238280`、p95 `4.315725`、max `6.059802`秒より小さいが、実行環境差と対話I/Oを含むためCPU改善とは主張しない。今回の100 seed stderrはJSONへ保存されておらず、内部solver CPUとsmall-group診断の合計は未記録である。

## seed 0 smoke

実行前freeze後、エージェントは予定どおりseed 0を一回だけ実行した。AC、Score `55,762,976`でv33と同点、内部solver CPU `1296.656ms`だった。small-group descentは14 attempt、step / candidate / choice 0で、新規3 errorを含む全identity/errorは0だった。smoke後にsource・方針・定数・記録を変更せず、上記100 seedはユーザーが実行した。

## 残差

v31前の保存軌跡では、Accepted ConnectedGrowth / GrowAndTrimの旧shape lossは約`1,043,940,000`で、`P>=50`が約`978,022,000`、93.69%を占めた。このためv31は大規模組を優先したが、`P<50`にも約`65,918,000`、6.31%の残差がある。

既存strict descentは`P>=50`の15,330試行から811件を改善し、総周長`-2,052`、即時料金`+4,166,619`だった。面積に依存した合法性や目的関数ではないため、小規模組を除外していた理由は理論的不適用ではなく、当時のimpact集中とCPU管理だった。v34で探索幅の追加が失敗した後は、新しい候補選択器を足すより、実測済み近傍を未適用領域へ一般化する方が自由度を小さく保てる。

## 比較した方針

### A. 小規模組へ既存strict descentを開放

old Accepted connected、smooth expert、strict料金増、3 snapshot future-fit非悪化、old/root rollbackというv33の保護を全て維持し、descentだけ`P<50`へ開放する。新しい形状近傍、学習selector、閾値を追加せず、直接攻撃する損失項もaccepted shape lossのままである。採用する。

### B. 小規模組へdense boxも開放

全盤面anchor走査とTarjan trimは、小規模組でも盤面サイズ`N=50`に依存する。残差6.31%に対してCPU増が大きく、dense予算の先着順も大規模組から奪い得る。`P>=50`と`U>=10,000`の既存gateを維持して見送る。

### C. exact template availabilityをfuture-fitへ追加

将来需要の面積分布と各テンプレートの合法anchor数を評価すれば、正方形8サイズより問題へ近いproxyを作れる。一方、通常placement全体の順位を広く変え、過去の12未来料金比較やcapacity Pareto guardの失敗と同じく、小標本proxyの選択誤差を増やす。独立validationなしの一回実装では見送る。

### D. 2-swap、plateau、multi-start

zero plateauを含むv32はv31比`-0.003378%`、同一最大gain分岐のv34はv33比`-0.010752%`だった。v34は追加枝の即時差`+30,216`に対して後続影響`-744,343`であり、局所候補数を増やすだけでは既存future-fit/rootが真の未来を選び切れなかった。再導入しない。

### E. root rolloutのscenario追加・holdout化

common random numbersと独立holdoutは選択biasを減らせるが、通常rootとrescueを広く変えCPUも増やす。Kleywegt–Shapiro–Homem-de-MelloのSAAは候補解を別標本で評価する重要性を示す一方、現在の2 scenario screenを少し増やすだけで十分という根拠にはならない。今回の局所幾何差分と因果を分離するため見送る。

## 採用実装

1. polish候補判定から面積gateだけを外す。実ターン、旧sourceがConnectedGrowth / GrowAndTrim、旧料金が機会損失より高い、旧周長が最小周長より長い、初期`E/G<0.55`という条件は維持する。
2. dense boxは従来どおり`P>=50`、理論最大料金改善`U>=10,000`、case最大24実走査だけである。`P<50`は明示的なsize filterとして診断する。
3. strict descentは全eligible面積で最大8 step実行する。非関節セルだけを除去し、空きfrontierだけを追加し、毎候補を面積・池・占有・4連結で再検証する。
4. 候補採用には旧料金からの丸め後strict増を要求する。未来がある場合は、旧配置と共通の3 snapshot scalar future-fitが`1e-15`許容内で非悪化の最短周長tierだけを許す。
5. old connectedはroot runner-up先頭へ残す。Compact rescueとnormal-rootの発火参照も旧周長のままで、Reject→Acceptやsynthetic rollout内の追加探索は起こさない。
6. `P>=50`の候補・tie-breakを変えずCPUを相殺するため、各removeに対する意味保存prefilterを追加する。removeの選択近傍数を`k_r`、remove後のadd近傍数を`k_a`とすると、周長差は`L'-L=2(k_r-k_a)`である。remove前frontierの最大近傍数`K`が`K<=k_r`なら、remove後の近傍数は増えないので`k_a>k_r`は不可能であり、そのremoveの全add scanを省ける。
7. 小規模descentのattempt / step / candidate / future-fit reject / final choice / 周長改善 / 即時fee gainと、remove単位prefilterを分離して出力する。rootが別actionへ上書きした場合は、小規模final choice・改善量・fee gainを戻す。

## 過適合と意味保存

- 新しいcase分類、学習係数、到着時刻gate、価値閾値を追加しない。
- v34で負だったseedだけを除外するpost-hoc gateを作らない。
- 新しい候補sourceや近傍を追加せず、v33で使っていた同一greedy descentだけを再利用する。
- dense budgetを小規模組が消費しないため、`P>=50`のdense発火列は同一である。
- per-remove prefilterは正gain不可能なpairだけを除き、`P>=50`のdescent winner・列挙順を変えない。
- 小規模組でも直接fee増だけで採用せず、既存future-fitとroot rollbackを維持する。ただしproxyは真の将来scoreを保証しないため、100 seed結果はpaired total、tail、CPU、診断を分けて判定する。

## 文献との対応

- AtCoder, AHC069問題文: https://atcoder.jp/contests/ahc069/tasks/ahc069_a
- Sascha Kurz, *Counting polyominoes with minimum perimeter*: https://arxiv.org/abs/math/0506428
- Fiduccia, Mattheyses, *A Linear-Time Heuristic for Improving Network Partitions*: https://doi.org/10.1109/DAC.1982.1585498
- Kleywegt, Shapiro, Homem-de-Mello, *The Sample Average Approximation Method for Stochastic Discrete Optimization*: https://doi.org/10.1137/S1052623499363220
- Balcan, Sandholm, Vitercik, *Generalization in portfolio-based algorithm selection*: https://arxiv.org/abs/2012.13315

最小周長polyominoの結果は`Lmin`という厳密目標の背景、FMはセルgainを更新しながら局所改善する着想に対応する。本実装はFMのpartition、lock、非改善move、線形時間性を実装するものではなく、固定面積・4連結領域の1-remove/1-add strict descentである。SAAとportfolio一般化は、少数scenarioや候補増を無条件に信頼しないための注意材料であり、小規模gateの性能保証ではない。

## hard diagnostics

次を0必須とする。

- connected polish candidate = static filtered + eligible
- eligible = dense size filtered + dense value filtered + dense budget skip + dense実走査
- dense実走査数 = case budget消費数
- small descent attempt = dense size filtered
- small attempt / step / candidate / future-fit reject / choice / 周長改善が全descent集計を超えるsubset違反
- small final choiceの有無と、正の周長改善・正のfee gainの不一致
- 既存の合法性、source、score再構成、DLP、root、Push-out全error

## 静的検証

- `git diff --check -- main.cpp`: pass
- Apple Clang 17、C++17/C++20、`-Wall -Wextra -Wshadow -Wpedantic -fsyntax-only`: pass、警告0
- Apple Clang 17、C++20、同warning指定のstatic analyzer: pass、出力0
- 実行前の解答プログラム実行: 0回

## 実行前freeze

全編集・静的監査・最終compile後のfreezeは次である。

- `main.cpp`: 6,654行
- source SHA-256: `1a5f652b17ca8de08b34920ea35f1928cfea7008dc98a4c7138b933e22d3db60`
- release binary: `/private/tmp/ahc069-small-group-strict-descent-v35`
- binary SHA-256: `1f5ca3eede1fdbeec560001f6568e2c5c9fea4af3f941759567f4b6deed07e85`
- compiler: Apple Clang 17.0.0
- compile: `-std=c++20 -O2 -DNDEBUG -Wall -Wextra -Wshadow -Wpedantic`
- freeze時点の解答プログラム実行: 0回

エージェントの解答実行は、このfreeze後のseed 0 smoke一回だけで終了した。100 seed paired testはユーザーが上記正本として実行済みである。

```bash
./tester /private/tmp/ahc069-small-group-strict-descent-v35 < tools/in/0000.txt
```

本結果の記録では`main.cpp`を変更せず、追加実行もしていない。その後v36を試して棄却し、
ユーザーの新しい明示指示によりv36固有差分を全撤去した。ユーザーが同じv35で実行した
3000 case正本は`result_20260808_180959.json`、3000/3000 AC、合計
`204,023,485,135`、平均`68,007,828.3783`である。

さらにv37のremaining hard gateとv38のcompact-template configuration shadowを順に試したが、
3000 caseでv35比`-0.004618% / -0.119609%`となり、いずれも棄却した。v38結果確認後の
ユーザー明示指示によりv38固有差分も全撤去した。現在の`main.cpp`は上記6,654行・
SHA-256へ再び完全復元され、v35が現行sourceかつ100 / 3000 case incumbentである。
最終復元後に解答プログラムは実行していない。
