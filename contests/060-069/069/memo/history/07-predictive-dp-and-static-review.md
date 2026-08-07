## 2026-08-02: predictive sparse DP v17（実行前固定）

sampled DLPが将来需要を連続量として価格付けするのに対し、将来の組を丸ごと受理・拒否する組合せを予測するDPへ将来価値評価を置換した。通常配置、grow-and-trim、admissionの比較式、future-fit、rollout、compact rescue、既存NoRegion Push-outには手を加えていない。比較は厳密には「16 bucketのfluid LP」と「同じ16境界を8帯へ集約し、量子化と幅制限を加えた0/1 sparse DP」のpackage A/Bであり、DP化だけの効果とは解釈しない。

### 共通の予測snapshot

posterior、256件の代表request、Halton型のsample座標、16本の元境界、rebuild trigger、request hashは旧sampled DLPと共通である。DPだけが16区間を次のindexで8帯へ集約する。

    {0, 1, 2, 3, 4, 6, 8, 12, 16}

終盤に元bucketが8以下なら1対1、それより多く16未満なら上のcutをstrict-increasingになるよう比例縮約する。近い将来を細かくし、遠方だけをまとめる構成である。

rebuild時の残り組数を`m`とし、256代表の共通重みを`w=m/256`とする。各帯の容量は、芝生セル数×帯時間から現在activeな全組の既知のcell-timeを引いた値である。未来代表requestは理想料金を報酬、全滞在区間が各帯で消費する`w`倍cell-timeを8次元負荷として、take/skipを同時に決める。これにより、長く滞在する1組が複数時刻の容量を同時に使う相関を残す。

容量は各帯のfull cell-timeを65,535段階へfloor、未来item負荷はnearest、現在候補のquery負荷はceilで量子化する。積と除算は`i128`、状態使用量は`uint16_t[8]`、item負荷は`uint32_t[8]`、報酬は重みを掛けない`long long`の理想料金和で保持する。

### sparse frontierと機会費用

frontier上限は512状態。各itemについてskipとfeasible takeを作り、exact usageが同じ状態は報酬最大だけを残す。512を超えた場合は各次元を同じbit数だけ右shiftするepsilon boxを作り、zero-usage状態と全box winnerが512以内になる最小shiftを選ぶ。box winnerは報酬降順、同値なら総使用量昇順、さらにexact usage辞書順で一意に決め、空き枠は同じ全順序の上位状態で埋める。同じbox内の低報酬・低使用量な非支配状態を落とし得るため、縮小容量で将来価値を過小、機会費用を過大評価する近似誤差は残る。これは初回幅512版の既知の制約として固定する。

初期実装の複数sortは静的計算量が大きかったため、解答実行前に次へ最適化した。

- exact usage順のskip列と、固定loadを加えたtake列を線形mergeする。
- 最大1,024 keyを2,048 slotのgeneration付きopen-address tableへ入れてbox数とwinnerを求める。
- 空き枠だけ`nth_element`で選び、最後は元のexact usage順の部分列としてfrontierを作る。

これにより、約75 rebuildならtake判定の上限は`75×256×512=9,830,400`回で、各itemに数回の最大1,024-state hash scanを加える程度になる。線形mergeの順序帰納、hash tableの空slot保証、collision処理、generation wrap、`nth_element`の全順序と決定性は独立監査済みである。

rebuild時の元容量に対する最大報酬を`V(C)`、現在候補の量子化負荷を引いた容量で同じfrontierを走査した最大報酬を`V(C-q)`とし、

    opportunity cost = m * (V(C) - V(C-q)) / 256

を既存の全admission・placement・Push-out・rollout評価へ渡す。rebuildしない実turnとsynthetic rolloutでも、rebuild時の`m`、容量、frontierを凍結して使う。候補ごとに未来を再推定せず、旧sampled DLPと同じ因果境界を守る。

### A/B境界と診断

既定のTreatmentは予測DPだけをsolve/queryする。`AHC069_DISABLE_PREDICTIVE_DP`付きControlは旧sampled DLPだけをsolveし、旧価格内積を使う。両方を同時に解く処理やblendはない。静的binary symbolでもTreatmentにはDP solve/queryだけ、Controlには旧dual solveだけが残ることを確認した。

DP診断にはsolve/query/item数、transition可否、prune入出力、exact duplicate、width prune、frontier、epsilon shift histogram、zero-load item、容量/item/query load、base/reduced value、frontier hash、rebuild/query CPUを記録する。次を全caseで0とする。

    predictive_dp_frontier_cap_errors
    predictive_dp_duplicate_errors
    predictive_dp_capacity_errors
    predictive_dp_query_order_errors
    predictive_dp_nonfinite_errors
    predictive_dp_zero_state_errors
    predictive_dp_frontier_order_errors
    predictive_dp_query_upper_bound_errors
    predictive_dp_solve_count_error
    predictive_dp_query_count_error
    predictive_dp_query_partition_error
    predictive_dp_transition_partition_error
    predictive_dp_item_count_error
    predictive_dp_generation_error
    predictive_dp_prune_accounting_error
    predictive_dp_shift_histogram_error
    predictive_dp_maximum_shift_error
    predictive_dp_internal_errors
    predictive_dp_disabled_activity_error
    predictive_dp_disabled_cpu_error
    predictive_dp_opportunity_identity_error

既存のsampled DLP、Push-out、grow-and-trim、score decomposition、status/plan/source等の全error/identity fieldも0を要求する。ControlとTreatmentで一致を要求するのはrebuild回数・trigger内訳・generated request数・sample hashである。採否後の盤面に依存する容量、rollout call数、zero-future call数、frontier hashは一致gateにしない。

### 実験順と事前判定基準

1. Treatment sanitizerをseed 0、7、37で実行する。
2. Controlをseed 0〜99、threads=1で実行する。
3. Controlが凍結v14と全score・stdoutで完全一致した場合だけTreatmentをseed 0〜99で実行する。
4. seed 0、7、37を再実行し、Treatmentの決定性を確認する。
5. 固定best scoreから相対値を再計算し、読み取り専用でpaired分析する。

sanitizer異常またはControl互換失敗なら、その時点でsourceを変更せず停止して報告する。hard gateは100/100 AC、全error/identity 0、Control total `6,515,194,836`、Control全stdout byte一致、Treatment/Controlの共通sample・rebuild因果一致である。

採用の強い基準は絶対score合計+0.20%以上、seed比の幾何平均>1、paired bootstrap 95%下限>=0.998、seed比p05>=0.98、worst>=0.94、低・中・高R層の各合計比>=0.99とする。solver内部CPUは同batch Control比で平均1.20倍以内、p95 1.25倍以内を要求する。0〜+0.20%は弱い正信号として即採用せず、負またはtail/runtime gate違反なら棄却寄りとする。

### 実行前固定物

- baseline commit: `9506343`
- baseline source SHA-256: `6f763500e0d5c0c851ce26971be88129f87666d9659d6463ded4f9f29280e997`
- baseline binary: `/private/tmp/ahc069_v14_restored`
- baseline result: `pahcer/json/result_20260802_192918.json`
- baseline absolute score: `6,515,194,836`（100/100 AC）
- baseline stdout: `tools/out-v14-restored/{0000..0099}.txt`
- `main.cpp` SHA-256: `ee024fb71356c494720882408a779269a47bb598b66550a9658a564ce39a36ff`
- Treatment binary: `/private/tmp/ahc069_predictive_dp_v17`
- Treatment binary SHA-256: `039bdd5926c69d30412389384fbfcf16568739efe423d5adfbb2fd33b5f3f5f7`
- Control binary: `/private/tmp/ahc069_predictive_dp_v17_control`
- Control binary SHA-256: `526245c8a6f660f0ad6e15382c2f339ddd0eb31ec14c0e6f2abfc9e837c194b9`
- sanitizer binary: `/private/tmp/ahc069_predictive_dp_v17_san`
- sanitizer binary SHA-256: `725d78e37a41b4025f7fac095404b3559f8a92f39aeecd6d1a691e7fc44d2179`
- Treatment config: `pahcer/bench_predictive_dp_v17.toml`
- Treatment config SHA-256: `c922c336a3da14a8cbe3790b96c617fb103c01639e6a47177325ba919f5e2c34`
- Control config: `pahcer/bench_predictive_dp_v17_control.toml`
- Control config SHA-256: `dbc4037943b95020a75cd0a0e9d8971fac9d53bce7b7bae0aa7335f403d5e076`
- tester SHA-256: `3702067f731de62b30a395f99eee04a4a9e247a1f421e2c42556a9e45b62ec92`
- `pahcer/best_scores.json` SHA-256: `b4c70ce5bb84ec557353d33d6c41f96e857a6d74d5f3213c685a1f602c5d1fa6`
- seed 0 / 99 SHA-256: `61857f9adeb56546a876f9f54eec3e95be617d4a1135d987ae790a2248304754` / `cfb3c1678be95ff9ff760cafed7a36c99a769c4c75c679b4a0bdef7d8edf2ad8`

Clang C++17 Treatment/Control、Clang C++20、GCC C++17のbuild・syntax検査、`-Wall -Wextra -Wpedantic`、ASan/UBSan build、Clang Static Analyzer、`git diff --check`はpassし、標準警告・解析指摘0件。数理、A/B isolation、hook、順序不変条件、hash安全性、決定性、計算量、診断保存則を3系統で独立監査し、blocking issue 0を確認した。

ここまで固定した3 binaryはまだ一度も実行していない。以降は上のsanitizer、Control互換、Treatment 100 seed、決定性probeを一つのbatchとして行う。最初の解答実行後は結果にかかわらずsource、方針、診断、config、memo、binaryを変更せず、読み取りと結果報告だけを行う。

## 2026-08-02: predictive sparse DP v17の結果

前節で固定したControlとTreatmentをseed 0〜99、threads=1で実行した。結果ファイルはControlが`pahcer/json/result_20260802_201954.json`、Treatmentが`pahcer/json/result_20260802_202209.json`である。

| 指標 | DLP Control | predictive DP v17 |
| --- | ---: | ---: |
| 絶対スコア合計 | 6,515,194,836 | 5,847,899,892 |
| Control比 | 100% | 89.7579% |
| 勝ち / 引き分け / 負け | - | 5 / 7 / 88 |
| solver内部CPU平均 | 1,030.85ms | 2,729.01ms |

両版とも100/100 AC、全error/identity field 0、sanitizer異常0だった。Controlは旧v14と全score・stdoutが一致し、Treatmentのsample/rebuild因果と決定性も確認できたため、差はDP packageへ隔離できている。

v17はControlより3,691組多く受け入れ、shape lossを217,849,295、movement costを1,185,118改善した一方、accepted ideal feeを886,329,357失い、最終的に667,294,944悪化した。未来の機会費用総量は旧DLPの約3.97倍で、価値ある到着を過剰に拒否していた。

主因は二つある。

1. 256代表の各itemへ`remaining_groups/256`相当の負荷を持たせたため、序盤は1 itemが約4組分の負荷を表し、`V(C)-V(C-q)`が大きな段差になった。
2. frontier上限512のepsilon-box winnerではpruneの47.1%がshift 13〜15となり、縮小容量で重要な低負荷状態を落とした。これにより`V(C-q)`を過小、機会費用を過大評価した。

したがってv17の係数調整は行わず、個別組atomとquery-safeな広いfrontierへ作り直す。

## 2026-08-02: quality-first predictive DP v18（実行前固定）

ユーザーの「一度計算時間を度外視した上でDPを試す」という指示に基づき、CPU時間を採否条件から外し、v17の二つの主要近似を除いた。通常配置、grow-and-trim、admission比較式、future-fit、root rollout、compact rescue、既存NoRegion Push-out、rebuild triggerは変えていない。今回もDP snapshotはrebuild間で凍結されるため、残り組数とactive組を厳密に反映するのはrebuild時点である。

### 個別atomと16時刻帯

旧DLP Controlは従来どおり256代表と`remaining_groups/256`の連続重みを用いる。v18 Treatmentはrebuild時の残り組数を`m`として、同じ決定的generatorからちょうど`m`件を生成し、1 requestを重み1の不可分な未来組として扱う。thetaはposteriorの10/30/50/70/90%層へ`m`件を可能な限り均等に配る。duration/start/P/valueはindex 1〜`m`のradical inverseを使う。

v17の8帯集約は廃止し、元の最大16時刻帯をそのまま16次元resourceとする。各次元のraw容量は

    grass cells × bucket width - active groupsの既知cell-time

であり、未来itemと現在queryのraw負荷は`P × overlap`である。

### 幅削減をしないadaptive-resolution DP

最初にresource levels `L=65,535`を試す。容量は

    floor(available cell-time × L / full cell-time)

未来itemとquery負荷は

    ceil(load cell-time × L / full cell-time)

へ安全側に量子化する。各itemについてskipとfeasible takeをusage辞書順で線形mergeし、exactly同じusageは最大rewardだけを残す。それ以外のPareto状態、低負荷状態、高報酬状態を恣意的に落とさない。

unique frontierが131,072状態を超えた試行は全て捨て、`L=ceil(L/2)`としてraw itemから量子化し直し、item 0・zero stateからDP全体を再計算する。最も細かく完走した解像度だけを採用する。`L=1`では各実現可能usage次元が0/1なので最大`2^16=65,536`状態となり、131,072上限内で必ず完走する。したがって選択された解像度内ではwidth pruningもepsilon-box winnerも存在しない。

元容量での最大rewardを`V(C)`、現在候補のceil負荷を引いた容量で同じfrontierを走査した値を`V(C-q)`として、

    opportunity cost = V(C) - V(C-q)

を既存評価へ渡す。1 atomが1組なのでv17の`m/256`係数は掛けない。

この方式は文字どおりの無限状態・raw cell-time完全DPではなく、「最大131,072状態に収まる最も細かい共通resource解像度で完走するquality-first DP」である。計算量を採否から外す一方、有限メモリと必ず完走する決定性は維持する。

### 診断とhard gate

resource-level attempt/restart、選択level histogram、最小選択level、cap超過候補の最大状態数、再始動で捨てた状態数、attemptを含む処理item数、最終完走item数を追加した。v17互換のshift/prune/width値は意図的に0のまま残し、実際の粗さはlevel診断だけで読む。

全caseで既存の全error/identity fieldが0に加え、次を要求する。

- `predictive_dp_frontier_cap_errors=0`
- duplicate/capacity/query-order/nonfinite/zero-state/frontier-order/query-upper-bound errorが全て0
- solve/query/transition/item/level/generation/merge accounting errorが全て0
- `generated_requests == expected_generated_requests == successful_items`
- `predictive_dp_width_pruned=0`、`prune_steps=0`、`maximum_shift=0`
- retained frontier最大131,072以下、candidate frontier最大262,144以下
- ASan/UBSan報告、signal、timeout、WAが0

CPU時間は記録するが採否には使わない。ただし未完走、OOM、異常終了はcorrectness failureとする。

### 固定した開発比較

最初は既知のseed 0〜4、threads=1だけを診断用に使う。この5 seedは最終採用の統計には使わない。比較anchorは次のとおり。

| seed | DLP Control | v17 DP |
| ---: | ---: | ---: |
| 0 | 55,844,966 | 57,946,859 |
| 1 | 75,366,251 | 77,125,140 |
| 2 | 36,717,780 | 32,231,413 |
| 3 | 40,085,569 | 34,462,075 |
| 4 | 75,488,342 | 71,424,802 |
| 合計 | 283,502,908 | 273,190,289 |

Control hard gateは5/5 AC、合計283,502,908、各seed scoreおよび`tools/out-predictive-dp-v17-control/{0000..0004}.txt`とのstdout byte一致である。新診断keyが増えるためstderr全体のbyte一致は要求しない。TreatmentとControlは生成request数が異なるので、`generated_requests`と`sample_hash`は一致条件にしない。rebuild総数・trigger内訳・real price call数だけを因果gateとする。

開発5 seedの判定は次で固定する。

- hard gate違反なら停止して報告する。
- 合計がv17以下ならHQ化でも救済できず棄却。
- v17超かつControl未満なら近似誤差は救済した可能性があるが、現行DLPには負けるため採用しない。
- Control以上、+0.20%未満なら広い未使用seedへ進める弱い候補。
- Control比+0.20%以上、seed比幾何平均>1、3/5勝以上、worst ratio>=0.94なら広い未使用seed A/Bへ昇格する。

実行順は、sanitizer seed 0、同一source Control seed 0〜4、Control互換確認、Treatment seed 0〜4、release seed 0決定性再実行で固定する。最初の解答実行後はsource、方針、診断、config、memo、binaryを変更しない。

### 実行前固定物

- baseline commit: `f3d75ae`
- v17 Control result: `pahcer/json/result_20260802_201954.json`、SHA-256 `d89ba96fa140225f8c0d511bfa2622c4890e966a1da00ee61315925a816d7894`
- v17 Treatment result: `pahcer/json/result_20260802_202209.json`、SHA-256 `cd5bf586c4f98920b470139c25c52a9b20dc8439a0530b2865f0e3304fd4c8c3`
- `main.cpp` SHA-256: `5bd0369ab88b6ec73e58fa762e17fb130e85bf347c8b878ac6caa62851a314b0`
- Treatment binary: `/private/tmp/ahc069_predictive_dp_hq_v18`、SHA-256 `4eb27f9406db7d0284b60b91249a1eab7a571f6d44b41262ae117843219142d6`
- Control binary: `/private/tmp/ahc069_predictive_dp_hq_v18_control`、SHA-256 `a6acbc72bb57009e461998053c179a578c6636b5f2c92fb175b6a3aad3da7269`
- sanitizer binary: `/private/tmp/ahc069_predictive_dp_hq_v18_san`、SHA-256 `f3008324c328ff2a12df203e278d8c30a1faabcb4e28504dfffd7b2bd873094b`
- Treatment config: `pahcer/bench_predictive_dp_hq_v18_dev.toml`、SHA-256 `75c2cda5f8b69094e9408a98c6ab48a6b0f64d9a3f7ac21118ff889d05eaf430`
- Control config: `pahcer/bench_predictive_dp_hq_v18_control_dev.toml`、SHA-256 `8f0b3c304ba12fcc0b429242d0d1d9cd396bf291cbebf3503b4b0b570d1d222c`
- tester SHA-256: `3702067f731de62b30a395f99eee04a4a9e247a1f421e2c42556a9e45b62ec92`
- `pahcer/best_scores.json` SHA-256: `b4c70ce5bb84ec557353d33d6c41f96e857a6d74d5f3213c685a1f602c5d1fa6`
- seed 0〜4 input SHA-256: `61857f9adeb56546a876f9f54eec3e95be617d4a1135d987ae790a2248304754`、`1c7eff30e432e30080eac0e05c96da923de8e96b437fbc8f0709d2136d629285`、`3e36e0dd25339bc6711f4571c435e3748e20e4252a77335b2fbb8cc483002722`、`185af1a03ba99df84515638dc59c7ab001beaee2bbbabf720d92dbb4aa9d0357`、`293341bc212d541234e4d4fc901ef8d8dac27617ed9e450a7a056f135ae7c638`

Clang C++17 Treatment/Control、GCC C++20、ASan/UBSan build、Clang Static Analyzer、`git diff --check`はpassし、標準警告・解析指摘0件。個別atom、16帯、丸め、解像度再始動、merge順序、L=1終了保証、overflow、機会費用係数、診断保存則を3系統で独立監査し、公式入力制約下のblocking issue 0を確認した。

## 2026-08-02: HQ v18 sanitizer完走と監視誤認の訂正

前節の実行後、長時間commandがsession IDを返して継続していたにもかかわらず、途中時点の出力ファイルを完了済みと誤認し、sanitizerがメモリでkillされたと報告してしまった。実際には同じseed 0のsanitizerを3本重複して開始しており、全て約6分後に完走した。メモリエラー、signal、ASan/UBSan報告はなかった。以降は返されたsession IDを1本ずつpollし、process終了を確認するまでretryを開始しない。

3試行は全て次で一致した。

- Score: `53,687,356`
- stdout SHA-256: `945dc4ab07a264653bf766472eb426301bb862f0361c678d5c51e837ced8c7a8`
- `sampled_dlp_sample_hash=6038475080775118892`
- `predictive_dp_frontier_hash=4634193102354827668`
- legacy非timing診断SHA-256: `5b152b70c6f0225b33580c4c34f96db11e5816b3c234c8e4efcc8448274949a6`
- 全error/identity field: 0

実行binaryの現物は`/private/tmp/ahc069_predictive_dp_hq_v18_san`、SHA-256 `67cf3a39275bac813e93d5a1fa3062cdf4880f27a741bdcaaf09bae1ca89e9e1`だった。前節に記録したsanitizer SHA `f300...`とは一致していないため、前節のsanitizer artifact固定は無効とする。3本は20:52:46〜21:01:19に重複して走っていたため、各protocol wall timeとRSSは性能資料に使用しない。

## 2026-08-02: memory-stable quality-first DP v19（実行前固定）

ユーザーの「極力DPを行いつつエラーが出ない程度に抑え、見積もりに十分時間をかける」という指示に基づき、DPの数学的内容とfrontier上限131,072を維持したままallocationだけを安定化した。個別atom、16時刻帯、capacity floor、item/query ceil、adaptive resource level、`V(C)-V(C-q)`、rebuild trigger、admission以降の全処理は変更していない。コミットは作成しない。

### 変更前のメモリ上界

`PredictiveDpState`は40 byte、RawItemは136 byte、Itemは72 byteである。旧実装は1 itemごとにcurrent、taken、mergedを別vectorとして作り、以前のsnapshotも同時に保持した。frontier上限を`F=131072`とすると、state vectorだけで最大35MiB、未来分布表23.27MiBとrequest/itemを含む通常live payloadはx86-64基準で約58.7MiBだった。

さらに状態数が大きいitemごとに最大約15MiBをfreeしていた。ASanは解放領域をquarantineへ保持するため、リークがなくても十数itemで既定級256MiBのquarantineを満たし得る。実際のv18 sanitizerは完走したが、これは不要なメモリ増幅とallocator負荷である。

### 永続2-bufferとlazy merge

次のbufferをcase全体で1回だけreserveし、`clear()`と`swap()`だけで再利用する。

- `predictive_dp_frontier`: `F+1`状態
- `predictive_dp_scratch`: `F+1`状態
- raw item: 999件
- quantized item: 999件

take列はvector化しない。current frontierを単調cursorで1回だけ走査し、次のfeasible childを1状態だけ保持する。skip列とlazy take列はともにusage辞書順かつ一意なので、旧実装と同じ2-way mergeができる。currentはtake生成完了まで参照されるためmoveせずscratchへcopyする。

論理candidateが`F+1`へ達した後はscratchへの格納だけを止めるが、merge自体は最後までcount-onlyで走査する。これによりfailed levelでもtransition、feasible/infeasible、exact duplicate、logical output、maximum candidate、discarded stateを旧v18と同じ値で記録する。完走levelでは全状態がscratchに収まるため、frontierの順序・usage・reward・resource level・hash・base/reduced value・OCは旧v18と完全一致する。

### 変更後のメモリ上界

通常のreserve capacityでは永続payloadは次のとおり。

| 領域 | byte |
| --- | ---: |
| future survival `61×100000×4` | 24,400,000 |
| raw item `999×136` | 135,864 |
| quantized item `999×72` | 71,928 |
| state buffer `2×131073×40` | 10,485,840 |
| 合計 | 35,093,632（33.47MiB） |

request生成中の最大5 CDFと999 requestを加えたDP関連最大live payloadは、`long double=16 byte`のx86-64で約41.37MiB、Apple arm64で約37.44MiBと見積もる。allocator metadata、ASan redzone/runtime、配置・Push-out用領域は別である。実capacityから`predictive_dp_persistent_payload_bytes`も出力する。

同時にsizeが正のstateは最大`F+(F+1)=262145`件、論理state payloadは10,485,800 byte、予約payloadは10,485,840 byteである。state/raw/itemのitem単位allocation churnは0になる。残る主なASan churnはrebuildごとの最大約7.63MiBのduration CDFと0.274MiBのrequestである。

### 診断と同値性gate

追加診断は次のとおり。

- overflowしたmerge数
- 物質化したnext状態の最大数
- 2 state bufferの最大live要素数
- buffer/item上限違反
- count-onlyで数えた論理出力数
- 永続payload byte
- `level_restarts - overflow_aborts`

全caseで次を要求する。

    maximum_materialized_next <= 131073
    maximum_live_states <= 262145
    buffer_limit_errors == 0
    overflow_aborts == level_restarts
    overflow_restart_error == 0

既存の全error/identity fieldも0とする。seed 0では旧v18 sanitizer oracleとScore、stdout、sample hash、frontier hash、および新memory fieldとtimingを除く全診断を一致させる。

### 実験順

1. frozen Controlをseed 0〜4、threads=1で実行し、旧v17 Controlのscore/stdoutと一致確認。
2. frozen sanitizerをseed 0だけ単独実行する。macOSはLSan非対応なので`detect_leaks=0`とし、ASan quarantineを64MiBへ制限する。
3. sanitizer seed 0を旧v18 oracleと同値確認。
4. frozen release Treatmentをseed 0〜4、threads=1で実行する。
5. 全caseのcorrectness、memory診断、RSS、scoreを読み取り専用で分析する。

`/usr/bin/time -l`はsandbox外preflightでRSS、peak footprint、swapを出力できることを確認した。testerのchildとして`/usr/bin/time -l -o <seed file> <solver>`を実行し、solver自身のRSSを記録する。全実行は逐次とし、返されたsession IDだけをpollする。時間は停止条件にしない。

停止条件はhash不一致、tester/solver非0終了、signal、WA、score欠落、sanitizer報告、旧oracle/Control不一致、error field非0、release RSS 1GiB以上、sanitizer RSS 1.5GiB以上、swap発生、または決定性不一致である。

### 実行前固定物

- baseline commit: `f3d75ae`（以降の変更は未コミット）
- `main.cpp` SHA-256: `c47b59aed52c27948c8dd7381496340cbe172a974fad5583f289f148b96a407a`
- Treatment release: `/private/tmp/ahc069_predictive_dp_hq_mem_v19`、SHA-256 `96c063b9b345da95858ebc0ec749f7894ff1015a5016ea0201e39c7944370357`
- Control release: `/private/tmp/ahc069_predictive_dp_hq_mem_v19_control`、SHA-256 `4b4af7d3f1e9904d1ef445f40a58c93f9ec59132fcf11dc98eeefd36f8094609`
- sanitizer: `/private/tmp/ahc069_predictive_dp_hq_mem_v19_san`、SHA-256 `e81bd7ba0d75fc130395b7b18a93c378f4b4939b3e4f268bee004e60eba8eed6`
- Treatment config: `pahcer/bench_predictive_dp_hq_mem_v19_dev.toml`、SHA-256 `eaa4803e4896789c73cec466e9d22532ea6b5bba82329f5295d91ba01c21c560`
- Control config: `pahcer/bench_predictive_dp_hq_mem_v19_control_dev.toml`、SHA-256 `3f2eef82fbdc3f4bca94ec68822a48b74a440703686a16ae124e450783f5209d`
- ASAN_OPTIONS: `quarantine_size_mb=64:detect_leaks=0:halt_on_error=1:abort_on_error=1`
- UBSAN_OPTIONS: `halt_on_error=1:print_stacktrace=1`
- old v18 sanitizer oracle binary: SHA-256 `67cf3a39275bac813e93d5a1fa3062cdf4880f27a741bdcaaf09bae1ca89e9e1`
- old v18 seed 0 stdout: SHA-256 `945dc4ab07a264653bf766472eb426301bb862f0361c678d5c51e837ced8c7a8`
- tester SHA-256: `3702067f731de62b30a395f99eee04a4a9e247a1f421e2c42556a9e45b62ec92`
- `pahcer/best_scores.json` SHA-256: `b4c70ce5bb84ec557353d33d6c41f96e857a6d74d5f3213c685a1f602c5d1fa6`
- seed 0〜4 input SHA-256: `61857f9adeb56546a876f9f54eec3e95be617d4a1135d987ae790a2248304754`、`1c7eff30e432e30080eac0e05c96da923de8e96b437fbc8f0709d2136d629285`、`3e36e0dd25339bc6711f4571c435e3748e20e4252a77335b2fbb8cc483002722`、`185af1a03ba99df84515638dc59c7ab001beaee2bbbabf720d92dbb4aa9d0357`、`293341bc212d541234e4d4fc901ef8d8dac27617ed9e450a7a056f135ae7c638`

Apple Clang 17.0.0のC++17 Treatment/Control、C++20 syntax、ASan/UBSan build、Clang Static Analyzer、`git diff --check`はpassし、警告・解析指摘0件。stream merge、count-only tail、buffer capacity、resource level同値性、overflow、L=1終了保証、全診断保存則を2系統で独立監査し、blocking issue 0を確認した。ここまでのsource、memo、binary、configは未コミットであり、一度もv19解答を実行していない。

## 2026-08-02: memory-stable quality-first DP v19の結果と棄却

前節で固定したControl、sanitizer、Treatmentを順番に実行した。Controlはseed 0〜4、sanitizerはseed 0、release Treatmentはseed 0〜4であり、全実行をthreads=1で逐次実行した。結果JSONはControlが`pahcer/json/result_20260802_212000.json`、Treatmentが`pahcer/json/result_20260802_212551.json`である。

### correctnessとメモリ

Controlは5/5 AC、合計`283,502,908`で、旧v17 Controlの全score・stdoutとbyte単位で一致した。Controlの最大RSSはseed 2の`59,015,168 byte`（56.28MiB）、全seedでswap 0だった。

v19 sanitizer seed 0も正常完走した。

- Score: `53,687,356`
- stdout SHA-256: `945dc4ab07a264653bf766472eb426301bb862f0361c678d5c51e837ced8c7a8`
- `sampled_dlp_sample_hash=6038475080775118892`
- `predictive_dp_frontier_hash=4634193102354827668`
- 最大RSS: `163,004,416 byte`（155.45MiB）
- peak memory footprint: `208,701,056 byte`（199.03MiB）
- swap、signal、ASan/UBSan報告: 0

Score、stdout、二つのhash、新memory fieldとtimingを除くlegacy非timing診断507 keyは旧v18 sanitizer oracleと完全一致した。

release Treatmentも5/5 ACで、5 seed×74個のerror/identity fieldは全て0だった。全seedで次の境界を満たした。

    maximum_frontier = 131072
    maximum_candidate_frontier = 262144
    maximum_materialized_next = 131073
    maximum_live_states = 262145
    persistent_payload_bytes = 35093632
    buffer_limit_errors = 0
    overflow_restart_error = 0
    swaps = 0

release最大RSSはseed 1の`61,980,672 byte`（59.11MiB）で、事前停止線1GiBを十分下回った。379回のrebuildを全てDPで処理し、level attemptは5,838回、overflow restartは5,459回、transitionは合計5,679,148,758回だった。379 solve中292回（77.0%）は最終的に最粗の`L=1`を選択した。

変更前v18 releaseとv19 releaseをseed 0で直接比較したところ、Score `52,336,498`、stdout、sample/frontier hash、新memory fieldとtimingを除くlegacy非timing診断505 keyが完全一致した。したがってmemory refactorはreleaseでも判断を変えていない。

| seed 0 | v18 | v19 | 変化 |
| --- | ---: | ---: | ---: |
| 最大RSS | 202,522,624 byte（193.14MiB） | 58,621,952 byte（55.91MiB） | -71.05% |
| page reclaim | 124,427 | 6,225 | -95.0% |
| system time | 7.49s | 0.49s | -93.5% |
| real time | 51.60s | 33.29s | -35.48% |
| solver CPU | 43,996.214ms | 31,636.178ms | -28.09% |

よって永続2-bufferとlazy mergeは、DPの意味を保ったままallocation churn、最大RSS、実行時間を大幅に削減する実装として成功した。

### スコアと採否

| seed | DLP Control | predictive DP v17 | quality-first DP v19 | v19 / Control |
| ---: | ---: | ---: | ---: | ---: |
| 0 | 55,844,966 | 57,946,859 | 52,336,498 | 93.7175% |
| 1 | 75,366,251 | 77,125,140 | 65,744,498 | 87.2333% |
| 2 | 36,717,780 | 32,231,413 | 11,803,027 | 32.1453% |
| 3 | 40,085,569 | 34,462,075 | 33,434,798 | 83.4086% |
| 4 | 75,488,342 | 71,424,802 | 66,713,417 | 88.3758% |
| 合計 | **283,502,908** | **273,190,289** | **230,032,238** | **81.1393%** |

v19はControl比`-53,470,670`、v17比`-43,158,051`で、両方に0勝5敗だった。特にseed 2のControl比32.15%が大きく悪化している。solver内部CPUは平均33.404秒、最大55.624秒で、その約96.8%をDP rebuildが占めた。入出力待機ではなくDP計算そのものが実行時間の原因である。

メモリ安定化には成功したものの、状態数を広げたquality-first DPは旧DLPより未来価値を正確にできず、事前基準の「v17以下なら棄却」に該当する。多数のsolveが`L=1`へ縮退しており、16次元の共通量子化と現在の価値モデルを単に大規模化する方向には見込みがない。今後DPを再検討する場合は、frontier幅ではなく状態表現または予測目的を変える必要がある。

この実験は棄却する。`f3d75ae`はv18/v19のGit上の直前状態だが、それ自体が100 seedでControl比89.7579%となり棄却したpredictive DP v17である。そのため、この記録後に`main.cpp`だけをpredictive DP導入前の採用状態であるcommit `9506343`へ復元する。復元後の期待SHA-256は`6f763500e0d5c0c851ce26971be88129f87666d9659d6463ded4f9f29280e997`である。`memo.md`にはv17〜v19の調査・実装・実測結果を残し、復元はコミットしない。

## 2026-08-02: 複数assignment比較（実行前固定）

ユーザーが指定した「推奨する実装順」の1番として、同じ再配置targetに対するblocker移動先の完成assignmentを複数比較する処理を追加した。greedy成功時の追加beamやdestination再列挙は行わず、既存探索が持つ候補だけを利用する。

### 完成assignmentの回収

- blockerが1組なら組合せ衝突がないため、`make_rescue_destinations`が作ったpool全件を返す。Compactは最大10件、NoRegion Push-outは最大8件であり、先頭候補だけでなく象限多様性のために追加された候補も静的評価へ届く。
- blockerが2組以上なら、最大4種類の決定的挿入順を全てgreedyで試し、完成した異なるchoice列を最大4件回収する。
- 全greedyが失敗した場合だけ従来どおりbeamを起動し、最初に完成した挿入順の既存最終beamからrank上位の異なる完成leafを最大4件回収する。
- greedy成功後にbeamは起動せず、beamの展開順、`remaining_nodes`、destination anchor予算は変更しない。最初のgreedy成功後も残り最大3順のmask照合を行うため、厳密には小さなCPU仕事量だけ増える。
- pool内の同一領域は生成時に除去済みなので、blocker順の候補index列をcanonical keyとして完全重複を除く。単なる最終占有maskでは、退去時刻が違う組同士の移動先交換を同一視してしまうため使わない。

### 完成盤面の検証と静的評価

各assignmentから独立した`TurnPlan`を作り、元のownerから`validate_and_build_rescue_owner`で再構築する。面積、連結、池、重複、周長、移動費を再検算し、既存組の`fee_loss == 0`と直接利益正を従来どおり要求する。

同じtargetとblocker集合なら現在料金、移動費、直接利益は共通なので、検証済み完成盤面を次の辞書順で評価する。

1. 到着分布の3 snapshotにおけるfuture-fit（大きい方）
2. 直後の最大空き連結成分（大きい方）
3. 全組を配置し終えた盤面の退去時刻境界コスト（小さい方）
4. 元の生成順

future-fitは到着組と移動後の既存組を含む`final_owner`へ空candidateを渡して計算する。評価関数が参照するのはowner IDと`groups[id].t`なので、移動前の`groups[id].cells`には依存しない。sortのstrict weak orderingを壊さないよう、浮動小数点値はepsilon比較ではなく完全な大小比較を使う。

### target多様性とroot比較

検証済みassignmentを同じ到着領域ごとのfamilyにまとめる。

- 修復できたtarget familyが2種類ある場合は、それぞれの静的評価1位を1案ずつ残す。
- 1種類しか修復できない場合だけ、そのfamilyの静的評価上位2assignmentを残す。
- root rolloutへ渡すrescue候補は従来どおり最大2件で、共通2シナリオ×4到着の料金差により最終比較する。
- `remaining_groups == 0`では静的sortを行わず、従来の第1assignmentをそのまま使用する。

これにより、同一targetの複数案だけでroot幅を先に使い切ることを避けつつ、他に修復可能なtargetがない場合は移動後形状の違いをrolloutで直接比較できる。

### 診断

単一blocker、greedy、beamそれぞれの完成数、重複数、返却数、独立検証通過数、複数案が得られたrepair数、family数、静的評価による先頭変更、同一targetの2案をrolloutした回数、第2案のscreen勝利・採用回数と予測改善量を追加した。

既存の`feasible_plans`、blocker数histogram、`feasible_direct_gain`は過去との意味を保つためfamily単位で数え、assignment単位の件数は`rescue_assignment_validated`へ分離した。`rescue_assignment_family_identity_error = assignment_families - feasible_plans`も出力し、0を要求する。

### 実行前検証

- Apple Clang C++17、`-Wall -Wextra -Wshadow`: pass、警告0
- NoRegion Push-out無効版: pass、警告0
- protected-only版: pass、警告0
- GCC C++20: pass、警告0
- ASan/UBSan build: pass、警告0
- Clang Static Analyzer: pass、指摘0
- `git diff --check`: pass
- 2系統の独立静的レビュー: blocking issue 0

AHC生成AI規約に従い、この時点では解答プログラムを一度も実行していない。したがってscore、実ケースでの各identity、greedy追加照合・最大20完成盤面静的評価のCPU時間は未測定である。実行結果を得た後は、新しい明示指示があるまでコードを変更しない。

- `main.cpp` SHA-256: `c70d84acfb34303c9bce3d723856de8d7cd09db1800eb16462b68c4f816e7932`
- release build: `/private/tmp/ahc069_multi_assignment_build`
- sanitizer build: `/private/tmp/ahc069_multi_assignment_san`
- コミット: 作成していない

### 実行バッチの固定

ユーザーの明示指示「実行してみて」を受け、次の順で固定バッチを実行する。

1. Treatment sanitizerをseed 0で単独実行する。
2. commit `5145bc7`の`main.cpp`を標準入力から再コンパイルしたControlをseed 0〜99、threads=1で実行する。
3. Treatmentを同じseed 0〜99、threads=1で実行する。
4. Treatment seed 0を同じbinaryで再実行し、stdoutの決定性を確認する。
5. absolute score、勝/同点/負、seed比分位点、実行前best score基準のrelative score、solver内部CPU、assignment診断、既存error/identity fieldを読み取り専用で分析する。

Controlは今回の変更直前のsourceそのものである。さらに既存v14 oracle `pahcer/json/result_20260802_192918.json`、合計`6,515,194,836`、`tools/out-v14-restored`との互換性も確認する。cleanup commitによるコメント・不要実装削除を含むためsource hashは旧v14と異なるが、score/stdoutが一致すれば同じ採用方策とみなせる。

Pahcerは`--freeze-best-scores`を付け、実行中にrelative score基準を更新しない。sanitizer異常、WA、非0終了、score欠落、Control/Treatmentのerror/identity field非0があれば、それ以降の必要な読み取り以外は行わず報告する。最初の解答実行後はAGENTS.mdに従い、ユーザーの次の明示指示までsource、方針、config、memo、binaryを変更しない。

- Treatment source SHA-256: `c70d84acfb34303c9bce3d723856de8d7cd09db1800eb16462b68c4f816e7932`
- Treatment release: `/private/tmp/ahc069_multi_assignment_build`
- Treatment release SHA-256: `820ca09c744bcb315e07c6c8b19a06793490982b9c2df0ec532d99b48317841b`
- Treatment sanitizer: `/private/tmp/ahc069_multi_assignment_san`
- Treatment sanitizer SHA-256: `d15fface3822cc7715690904e45273d96e62365d176cde5996f3d401780ff83b`
- Control release: `/private/tmp/ahc069_multi_assignment_control`
- Control release SHA-256: `19f7298e21b67c1b1bf4931111ea8d004b3ffa859a83caedf8beaefa41450d8f`
- Treatment config: `pahcer/bench_multi_assignment_v20.toml`
- Treatment config SHA-256: `fb9c397018aa5fec0868727204558bf2453bbc17e60864c6f1abb99fe546337c`
- Control config: `pahcer/bench_multi_assignment_v20_control.toml`
- Control config SHA-256: `614ad82510bcf80a912357f461d2cf4a98b16799ce3193a98aeeac7d3856ea97`
- tester SHA-256: `3702067f731de62b30a395f99eee04a4a9e247a1f421e2c42556a9e45b62ec92`
- frozen `best_scores.json` SHA-256: `b4c70ce5bb84ec557353d33d6c41f96e857a6d74d5f3213c685a1f602c5d1fa6`
- seed 0 / 99 SHA-256: `61857f9adeb56546a876f9f54eec3e95be617d4a1135d987ae790a2248304754` / `cfb3c1678be95ff9ff760cafed7a36c99a769c4c75c679b4a0bdef7d8edf2ad8`

## 2026-08-03: 複数assignment比較 v20の実測と棄却

前節で固定したControlとTreatmentをseed 0〜99、threads=1で実行した。Controlは100/100 AC、合計`6,515,194,836`で、既存v14 oracleのscoreと全stdoutに一致した。Treatmentも100/100 ACだったが、合計は`6,509,387,594`だった。

- absolute差: `-5,807,242`、Control比`-0.0891338%`
- 勝ち / 同点 / 負け: `35 / 19 / 46`
- seed比幾何平均: `0.9989760175`
- seed比中央値: `1.0000000000`
- p05 / p95: `0.9724604409 / 1.0252139391`
- frozen relative score合計差: `-9.5606657372`

実装が発火していないわけではない。100 seed合計で完成assignmentを`6,505`件返し、全件が独立合法性検証を通過した。target familyは`1,638`、複数assignmentを得たrepairは`1,031`、静的評価で先頭が変わったfamilyは`679`だった。同一targetの第2案をrolloutしたのは`109`回、screen勝利`18`回、最終採用`14`回である。assignment固有および既存の全error/identity fieldは0だった。

solver内部CPUはControl平均`1007.743ms`、Treatment平均`1015.998ms`で約`+0.82%`。差は小さいものの、absolute、seed比幾何平均、frozen relativeの全てが負であり、現在の複数assignment比較は採用しない。ユーザーの次の明示指示を受け、`main.cpp`を変更直前Controlであるcommit `5145bc7`と完全一致するSHA-256 `086cb6cc2e24848c77a4b766ab8dde433dbf16469095cca557f848bdff091c6a`へ戻した。assignment回収、family化、静的評価、専用診断だけを除去し、sampled DLP、grow-and-trim、Compact rescue、NoRegion Push-out、従来のgreedy+beam、root rolloutとconfirmationは維持した。

- Control JSON: `pahcer/json/result_20260803_003818.json`
- Treatment JSON: `pahcer/json/result_20260803_004019.json`
- コミット: 作成していない

## 2026-08-03: 固定移動費target shortlist v21（実行前）

ユーザーが指定した「推奨する実装順」の2番として、Compact rescue / NoRegion Push-out共通のtarget shortlistを正確なグループ固定移動費へ変更した。旧方式はownerの各セルへ`move_cost / P_owner`を配り、target内の重複セル分だけ加算していた。しかし実際の移動費は1セルでも重なればグループ全体へ一度だけ発生するため、shortlist前の順位が逆転し得る。旧fractional prefix、fractional shortlist、複数assignment関連コードは残していない。

### 全アンカーの正確な固定費sweep

単純に全アンカーで到着面積`P`セルを走査すると重いため、各最小周長shapeを盤面上で1マスずつ平行移動する。

- 各ownerの現在target内セル数を`overlap_count`へ保持する。
- セル追加で`0 -> 1`になったときだけ、そのownerの移動費全額とblocker 1組を加える。
- セル削除で`1 -> 0`になったときだけ、移動費全額とblocker 1組を引く。
- 横移動ではshapeの左境界を削除し、右境界の1マス先を追加する。縦移動では上境界と下境界を同様に扱う。
- shapeごとに、初期`P`セルと全遷移境界セルの更新回数を見積もり、横走査と縦走査の安い方を選ぶ。

したがって各アンカーで保持する`exact_move_cost`は常にdistinct blockerの固定移動費和となる。maskから上下左右の集合差を作るため、将来shapeが複数区間や穴を持っても平行移動差分として成立する。走査方向を変えても、`shape_order_base + base_x * y_positions + base_y`をtie-breakに使い、従来のshape、x、y列挙順を維持する。

### 二つのshortlistと後段の維持

各metricの上限はCompact `160`、Push-out `96`のままで、次の2 heapを全アンカーから厳密に作る。

1. `(exact_move_cost, blocker_count, occupied_cells, order)`
2. `(blocker_count, exact_move_cost, occupied_cells, order)`

これは推奨案どおり「固定移動費が小さいtarget」と「blocker数が少なく修復しやすいtarget」を分けて確保する変更である。両heapの和集合だけを従来の正確なblocker復元、経済性判定、destination列挙、greedy+beam、root rolloutへ渡す。target repair、destination、beam、node、root幅、rollout、confirmationの上限と処理は変更していない。

### 保存則と安全側停止

shortlist後はshapeを一からmaterializeし、正式なblocker集合から次を再計算してsweep cacheと照合する。

- 固定移動費
- distinct blocker数
- occupied cell数

両heapのorder集合からintersectionを独立に再計算し、heap内重複、上限、和集合も検査する。盤面ownerが範囲外またはinactiveなら、その再配置探索は候補を返さず安全側に中止する。境界更新でoverlap underflowを検出したsweep lineも以後の候補へ使わない。

追加診断は、owner-cell更新数、fixed/blocker各heap件数、intersection、固定費/blocker数/occupied数の再計算error、owner状態error、overlap count error、shortlist union errorである。いずれも採否には使わず、error系は0を要求する。

### 実行前検証と固定物

- Apple Clang C++17 release: pass、`-Wall -Wextra -Wshadow`警告0
- GCC C++20 release: pass、警告0
- NoRegion Push-out無効版: pass、警告0
- protected-only版: pass、警告0
- ASan/UBSan build: pass、警告0
- Clang Static Analyzer: pass、指摘0
- `git diff --check`: pass
- 3系統の独立静的レビュー: blocking issue 0

AHC生成AI規約に従い、v21解答プログラムはまだ一度も実行していない。したがってスコア、実ケースでの固定費保存則、owner-cell更新量、solver CPU時間は未測定である。

- baseline commit: `5145bc7`
- `main.cpp` SHA-256: `baae0712fb7f9e81945f317509169bf7a4eed1e0df40e00e08b75ab8c841a4cc`
- release build: `/private/tmp/ahc069_fixed_cost_shortlist_v21`
- release SHA-256: `8634428eb450f6aae5d3d386881df5655926e07086ef4f2536ed22ac2603f9f9`
- sanitizer build: `/private/tmp/ahc069_fixed_cost_shortlist_v21_san`
- sanitizer SHA-256: `0e5c0d79cb4082de041144a4d9bf30af4f9c101b62f18be395a618cd387da3e5`
- コミット: 作成していない

### v21実行バッチの固定

ユーザーの明示指示「実行してください」を受け、次の順で固定バッチを実行する。

1. sanitizer版をseed 0で単独実行し、ASan/UBSanと新旧error/identity fieldを確認する。
2. release版をseed 0〜99、threads=1で実行する。
3. 同じrelease版でseed 0を再実行し、stdoutとtiming以外の診断の決定性を確認する。
4. 直前Control `pahcer/json/result_20260803_003818.json`と、absolute score、勝/同点/負、seed比分位点、frozen relative score、solver内部CPUをpaired比較する。
5. fixed/blocker各shortlist、intersection、owner-cell更新量、再計算保存則、既存error/identityを読み取り専用で集計する。

Controlはcommit `5145bc7`そのものを同じ100 seedで直前に再実行した結果で、100/100 AC、合計`6,515,194,836`、既存v14 oracleと全stdoutが一致している。再実行による時間消費を避け、scoreと内部CPUの基準にはこの同一環境の直前batchを用いる。

最初の解答実行後は、結果にかかわらずsource、方針、診断、config、memo、binaryを変更せず、読み取りと報告だけを行ってユーザーの次の明示指示を待つ。

- Treatment config: `pahcer/bench_fixed_cost_shortlist_v21.toml`
- Treatment config SHA-256: `c49afda159ba49e441be40ecc6c57a9d1a5c7795dae2603d9a6ab82d15bf46fc`
- Control JSON: `pahcer/json/result_20260803_003818.json`
- Control release: `/private/tmp/ahc069_multi_assignment_control`
- Control release SHA-256: `19f7298e21b67c1b1bf4931111ea8d004b3ffa859a83caedf8beaefa41450d8f`
- tester SHA-256: `3702067f731de62b30a395f99eee04a4a9e247a1f421e2c42556a9e45b62ec92`
- frozen `best_scores.json` SHA-256: `b4c70ce5bb84ec557353d33d6c41f96e857a6d74d5f3213c685a1f602c5d1fa6`
- seed 0 / 99 SHA-256: `61857f9adeb56546a876f9f54eec3e95be617d4a1135d987ae790a2248304754` / `cfb3c1678be95ff9ff760cafed7a36c99a769c4c75c679b4a0bdef7d8edf2ad8`

## 2026-08-03: 固定移動費target shortlist v21の実測と棄却

前節で固定したsanitizerをseed 0、release Treatmentをseed 0〜99、threads=1で実行した。sanitizerはASan/UBSan報告なしで正常終了し、releaseは100/100 ACだった。Treatment結果は`pahcer/json/result_20260803_010946.json`である。

| 100 seed | Control | v21 Treatment | 差 |
| --- | ---: | ---: | ---: |
| absolute score合計 | 6,515,194,836 | 6,501,932,241 | -13,262,595 (-0.20356%) |
| frozen relative合計 | 9,951.751023 | 9,931.752408 | -19.998615 |
| solver CPU平均 | 1,007.743 ms | 2,834.743 ms | +1,827.000 ms (+181.30%) |
| solver CPU最大 | 1,599.408 ms | 4,685.128 ms | +3,085.720 ms |

- 勝ち / 同点 / 負けは`31 / 29 / 40`、Treatment / Controlの幾何平均は`0.9979406`、中央値は`1.0`だった。
- 非同点71件のexact sign testは`p=0.34247`で、100 seedだけでは小幅なスコア差を統計的に確定できない。
- 一方、Treatmentは全100 seedでControlより低速で、内部solver CPUが2秒以上のケースは86件だった。
- 固定費sweepは全体で29,640,682,830回、1ケース平均296,406,828回のowner-cell更新を行った。更新数とpaired CPU増分の相関は`r=0.9698`だった。

実装は意図どおり発火した。固定費heap、blocker数heapは各2,571,696件、共通部分1,840,431件、和集合3,302,961件であり、`fixed + blocker - intersection = union = exact_targets`が全ケースで成立した。shortlist内のeconomic target率は`68.64% -> 77.93%`、rescue採用数は`711 -> 777`へ増えた。しかしexact targetは`-21.78%`、最終scoreは小幅悪化し、計算量は採用不能級に増加した。追加6項目を含む全error/identity fieldは0だった。

同一release binaryでseed 0を再実行し、stdoutはbyte単位で一致、非時間診断も全項目一致した。したがって不具合や未発火ではなく、正確な固定費sweepの費用対効果そのものが悪いと判断する。v21は棄却し、次の明示指示に基づいて`main.cpp`だけをControl commit `5145bc7`相当へ復元する。memo、AGENTS.md、実行結果、設定は記録として残し、コミットは作成しない。

## 2026-08-03: ActualFeeRejected再配置救済 v22（実行前）

ユーザーが指定した新しい「推奨する実装順」の3番として、静的レビュー論点2の`ActualFeeRejected`救済を単独実装した。旧系列の第3段階one-helper Push-outではない。v20複数assignmentとv21固定移動費sweepはいずれも棄却済みなので、`main.cpp`をcommit `5145bc7`と同じSHA-256 `086cb6cc2e24848c77a4b766ab8dde433dbf16469095cca557f848bdff091c6a`へ限定復元してから変更した。

### 対象と不変な部分

通常判定は、最小周長料金ならsampled DLP opportunity costをstrictに上回るが、実際に見つかった通常配置の形状料金はそれ以下の場合を`ActualFeeRejected`とする。旧版ではこのstatusだけがCompact rescue / NoRegion Push-outの入口へ届かなかった。

v22は`ActualFeeRejected`だけを第3の`ActualFeeRecovery` modeとして既存rescue探索へ通す。それ以外はControlと同じである。

- baselineはRejectなので現在料金を0とする。
- 最小周長料金からdistinct blockerの正確な移動費を引いた値がopportunity costをstrictに上回るtargetだけを許す。
- 完成planでもvalidator後に同じstrict条件を再確認する。
- destination優先領域はNoRegionと同じ「再配置前から空いていた全セル」とする。通常判定で見つかった拒否候補領域は保存・特別利用せず、別ヒューリスティックを混ぜない。
- NoRegionと同じshortlist 96、target 4、destination anchor 2,048 / case内16,000、legal 40、destination 8、node 1,024を使う。
- 旧fractional-cost / occupied-cell shortlist、最初の単一greedy/beam assignment、fee loss 0、最小周長target、共通2 scenario x 4 arrival screenを維持する。
- scenario生成失敗はNoRegionと同じfail-closedとし、Rejectを維持する。未来が0件ならstrict shadow gateを通った直接利益だけで採用する。
- shadowとrolloutの直列gate、confirmation配分、case全体時間guard、通常配置tier、sampled DLP、future-fit、grow-and-trimには変更を加えない。

同一sourceに`AHC069_DISABLE_ACTUAL_FEE_RESCUE`を用意した。ControlではActual入口がcompile-timeで無効になり、owner、groups、乱数列、DLP、confirmation、出力計画は`5145bc7`と同じである。stderrへActual専用0診断が増えることと微小な診断読取り以外は変更しない。

### 採用時の診断整合性

通常判定が記録した`actual_rejected_candidate_fee/perimeter`を残したまま最終statusだけAcceptedへ変えると、既存placement / loss保存則が壊れる。そこでeligibility時に元値をActual専用ledgerへ転記し、採用時だけ最終`ArrivalDecision`の2値を0へ戻す。配置sourceは実際の到着領域に合わせてMinimumTemplateへ置換する。失敗時は元のActualFeeRejectedをそのまま返す。

Actual専用にeligible、context、area、shadow filter、no-economic、no-repair、validation、feasible、rollout、adopt、探索量、blocker histogram、料金、移動費、未来差、opportunity、CPUを記録する。RAII scopeが全early returnを覆い、共通rescue counterの差分をActual欄へ移す。Compact派生値は`total - NoRegion - Actual`へ変更した。

全caseで次を要求する。

```text
eligible = final ActualFeeRejected + adopted
eligible = context_error + area_insufficient + no_economic + no_repair + screen_rejected + adopted
feasible_turns = rollout_generation_failure + rollout_turns + skipped_no_future
feasible_plans = sum(feasible blocker histogram)
adopted = sum(adopted blocker histogram)
arrival_fee - movement_cost - relocation_fee_loss = direct_gain
eligible original candidate fee = final ActualRejected candidate fee + adopted original candidate fee
eligible original candidate perimeter = final ActualRejected candidate perimeter + adopted original candidate perimeter
area_insufficient = 0
economic_validation_errors = 0
```

既存DLP call、Push-out、grow-and-trim、decomposition、status/plan/source、料金分解の全保存則も0を要求する。Actual採用が0のseedはControlとstdout byte一致を要求する。

### 実行前検証と固定物

- Apple Clang C++17 Treatment / Control: `-O2 -Wall -Wextra -Wshadow`でpass、警告0
- GCC C++20 release: pass、警告0
- NoRegion Push-out無効版: pass、警告0
- protected-only版: pass、警告0
- ASan / UBSan build: pass、警告0
- Clang Static Analyzer: 指摘0
- `git diff --check`: pass
- status / 会計call graph、A/B隔離、診断保存則の独立静的レビュー3系統: blocking issue 0

固定物は次のとおりで、コミットは作成しない。

- baseline commit: `5145bc7`
- `main.cpp` SHA-256: `9ba03542acddf4d0f4018328417c211e294e0c92cc78a20b845ccb4d2ffdb5b8`
- Treatment release: `/private/tmp/ahc069_actual_fee_rescue_v22`
- Treatment release SHA-256: `5f26d900e47905b649cee7c17f208f0c608f11fad6e1099005adedf396d3b95d`
- same-source Control: `/private/tmp/ahc069_actual_fee_rescue_v22_control`
- Control SHA-256: `d32e32f5e768e65893f0e1527cab1e99b3f80ad99309141d5f9b6f74b9b73c14`
- sanitizer: `/private/tmp/ahc069_actual_fee_rescue_v22_san`
- sanitizer SHA-256: `9d07e8547f39d2cce721ecbf08ca15ddfd01e960f6a8599e95812237af2eb8bd`
- Treatment config: `pahcer/bench_actual_fee_rescue_v22.toml`
- Treatment config SHA-256: `a85a81fbb14b11d97d6b9e50d8293628db8b8ac353599fb01244e0a8c5155bac`
- Control config: `pahcer/bench_actual_fee_rescue_v22_control.toml`
- Control config SHA-256: `43f38d98482ade0c3ff823fe2e49a66042097876fae66111a596a7dbcc676289`
- 既存Control JSON: `pahcer/json/result_20260803_003818.json`
- tester SHA-256: `3702067f731de62b30a395f99eee04a4a9e247a1f421e2c42556a9e45b62ec92`
- frozen `best_scores.json` SHA-256: `b4c70ce5bb84ec557353d33d6c41f96e857a6d74d5f3213c685a1f602c5d1fa6`
- seed 0 / 99 SHA-256: `61857f9adeb56546a876f9f54eec3e95be617d4a1135d987ae790a2248304754` / `cfb3c1678be95ff9ff760cafed7a36c99a769c4c75c679b4a0bdef7d8edf2ad8`

### 最終実行バッチ

ユーザーの明示指示に基づき、次を一つの最終バッチとして行う。

1. sanitizerをseed 0で単独実行し、ASan/UBSanと全error/identity fieldを確認する。
2. same-source Controlをseed 0〜99、threads=1、frozen best scoreで実行する。
3. Controlの100 score / stdoutを既存`result_20260803_003818.json` / `tools/out-multi-assignment-v20-control`と照合する。
4. Treatmentを同じseed 0〜99、threads=1、同じfrozen best scoreで実行する。
5. Treatment seed 0を同じbinaryで再実行し、stdoutと非時間診断の決定性を確認する。
6. absolute、frozen relative、勝/同点/負、seed比分位点、符号検定、内部CPU、Actual funnel / 採用 / 予測margin / 分解、既存保存則を読み取り専用でpaired比較する。

sanitizer異常、WA、非0終了、score欠落、Control互換性破壊、error/identity field非0の場合は、それ以降の必要な読み取り以外を行わず報告する。最初の解答実行後は、結果にかかわらずsource、方針、binary、config、memoを変更せず、読み取りと報告だけを行う。

### v22 最終実行結果

ユーザーの明示指示に基づいて上記バッチを実行した。sanitizer seed 0はASan / UBSan異常なし、same-source Controlは100 / 100 ACで、既存Control `result_20260803_003818.json`と全score、旧Control stdoutと100 / 100 byte一致した。Treatmentも100 / 100 ACで、seed 0再実行はstdout byte一致、非時間診断416項目一致だった。全error / identity / validation fieldは0である。

結果ファイルはControlが`pahcer/json/result_20260803_014400.json`、Treatmentが`pahcer/json/result_20260803_014627.json`である。

| 指標 | Control | v22 Treatment | 差 |
|---|---:|---:|---:|
| score合計 | 6,515,194,836 | 6,501,318,193 | -13,876,643 (-0.21299%) |
| 平均score | 65,151,948.36 | 65,013,181.93 | -138,766.43 |
| frozen relative合計 | 9,951.751023 | 9,935.518649 | -16.232374 |
| solver内部CPU平均 | 1,036.478 ms | 1,041.398 ms | +4.921 ms |

seed別は12勝65同点23敗、Treatment / Control比の幾何平均は`0.9983141785`、p05 / median / p95は`0.977564 / 1.0 / 1.015972`だった。二項符号検定の両側p値は`0.08953`である。Actual救済を採用しなかった65 seedはControlとstdout byte一致し、採用した35 seedだけscoreが変化したため、差は対象機能へ隔離できている。

ActualFeeRejectedは2,977 turnがeligibleで、`1,488 no-economic + 1,418 no-repair + 16 screen-rejected + 55 adopted = 2,977`だった。feasible planは131、採用55件のblocker数は1 / 2 / 3 / 4+組が43 / 8 / 4 / 0件である。採用時の到着料金`16,375,184`から移動費`268,944`を引いた直接利益は`16,106,240`、adopted opportunity costは`13,676,235.8`で、shadow上は`+2,430,004`の余裕があった。

しかし最終paired差から直接利益を除いた下流差は`-29,982,883`だった。全100 seedの受入数は71,417から71,407へ10件減り、受理集合の理想料金は`-23,310,696`、初期shape lossは`-9,563,785`と改善、最終料金は`-13,746,911`、移動費は`+129,732`で、`-13,746,911 - 129,732 = -13,876,643`とscore差へ一致する。NoRegion拒否価値が`+29,452,873`、fragmentation配置不能が30件増えており、現在の救済が後続の高価値到着と連結領域を失わせたことが主因である。

Actual専用CPUは平均`19.353 ms / case`、p95約`52.1 ms`、最大`75.764 ms`で、solver内部CPUは両群とも2秒超0件だった。実装・合法性・会計ではなく、短期screenとsampled DLPが救済後の空間損失を過小評価した性能問題と判断する。v22は棄却し、次の明示指示に従って`main.cpp`をControl commit `5145bc7`相当へ復元する。コミットは作成しない。

## 2026-08-03: 異なる周長tierの独立比較 v23（実行前）

ユーザーの指定した「推奨する実装順」の4番として、静的レビュー論点7の「最初に置けた最小周長tierへ固定される問題」を独立した大変更として実装した。v22のActualFeeRejected救済は棄却済みなので、最初に`main.cpp`をcommit `5145bc7`と同じSHA-256 `086cb6cc2e24848c77a4b766ab8dde433dbf16469095cca557f848bdff091c6a`へ限定復元した。v20複数assignment、v21固定費sweep、v22 ActualFeeRejected救済は残していない。

### 比較対象と独立性

対象は「最小周長`Lmin`のテンプレートを合法に置け、通常のsampled DLP入場判定も通るturn」に限る。従来案を保護baselineとしてそのまま作り、別ledgerで次のdistinct tier `Lmin+2`と`Lmin+4`を調べる。

- 1000 turnを16進行度windowへ分け、各windowで最初に対象となった1 turnだけをprobeする。候補0件でもwindowを消費し、良いturnが出るまで繰り返すselection biasを避ける。
- 各tierは従来baselineと別の`PlacementShortlistBuilder`を使う。アンカー、列挙順、shortlist、future-fitを混ぜないため、Controlの通常配置は変わらない。
- 各tier内では既存と同じ退去時刻境界評価で最大6候補へ絞り、同じ未来3断面の空き正方形評価がbaselineをstrictに改善する代表1件だけを残す。
- 正確な丸め後料金がsampled DLP opportunity costをstrictに上回る候補だけを比較する。料金がbaselineより良くなる異常候補、非連結、盤外、池、占有、面積、周長、source不一致は採用直前にも拒否する。
- baselineと最大2候補を、共通乱数の2 scenario・未来4到着で比較する。評価単位は料金で、`2 * (候補の現在料金 - baseline現在料金) + scenario0未来料金差 + scenario1未来料金差 > 0`の候補だけを採用する。同点は必ずbaselineを残す。
- synthetic未来ではcross-tier比較を再帰させず従来方策だけで進める。最大追加policy stepは`16 probe * 3 actions * 2 scenarios * 4 arrivals = 384 / case`である。
- Compact rescue、NoRegion Push-out、通常次点root比較、confirmation、sampled DLP、grow-and-trim、future-fit本体は変更していない。`Lmin` baselineでは既存rescue/通常次点の対象条件と排他的なので、同じturnで競合しない。
- `AHC069_DISABLE_CROSS_TIER_PLACEMENT`を付けたsame-source Controlでは候補生成・rolloutをcompile-timeで無効にする。

今回検証するのは、無制限な全周長tier探索ではなく「16回・`+2/+4`・future-fit prefilter・Q2/H4」という一つの完結したcross-tier packageである。負の場合も周長tier一般を直ちに否定せず、このfilterと短期screenを含むpackageの結果として解釈する。`+4`は`+2`と同時に競合するため、Control対Treatmentはpackage全体の効果であり、`+4`単独の限界効果ではない。

### 診断と保存則

probe funnel、tier別anchor / legal / shortlist / future-fit改善 / admission filter、rollout候補0・1・2件、進行度四分位、`+2/+4`の比較・screen・採用、generation failure、policy step、受入数、専用CPUを記録する。

丸めplateauの配置変更と、現在料金を実際に犠牲にする変更を区別するため、compared / screened / adoptedを料金損失0と正へ分けた。さらにtier別料金損失、全screenのscenario未来差とmargin、採用案専用のscenario未来差とmarginも別ledgerへ記録する。これにより、採用seedについて次を分離できる。

```text
最終score差 = 現在料金差 + 下流差
screen予測margin = 現在料金差 + 2 scenario未来料金差の平均
```

全caseで次を要求する。

```text
probe = no-future + no-legal-tier + no-fit-improvement + no-fee-valid + eligible
eligible = rollout-generation-failure + rollout-turns
rollout-turns = selected-baseline + adopted
fit-eligible = admission-filtered + compared + generation-failed（tier合計）
rollout-turns = zero-candidate + one-candidate + two-candidate
compared = zero-fee-loss + positive-fee-loss
screened = zero-fee-loss + positive-fee-loss = tier histogram
adopted = zero-fee-loss + positive-fee-loss = tier histogram = progress histogram
baseline fee - challenger fee = fee loss（screen / adopted）
margin_twice + 2 * fee loss = scenario0 future delta + scenario1 future delta
future-fit snapshot数 = 3 * (未来ありprobe + tier別shortlist総数)
legal <= anchors、shortlist <= 6 * 未来ありprobe、probe <= 16
```

Controlは既存`result_20260803_003818.json`の全scoreと旧Control stdoutへ100 / 100一致すること、Treatmentでcross-tier採用0のseedはsame-source Controlとstdout byte一致することをhard gateとする。既存DLP call、decomposition、料金分解、status / plan / source、rescue、Push-out、grow-and-trimの全error / identityも0を要求する。

### 実行前検証と固定物

- Apple Clang C++17 Treatment / Control: `-O2 -DNDEBUG -Wall -Wextra -Wshadow`でpass、警告0
- GCC C++20 Treatment: pass、警告0
- NoRegion Push-out無効版: pass、警告0
- protected-only版: pass、警告0
- ASan / UBSan build: pass、警告0
- Clang Static Analyzer: 指摘0
- `git diff --check`: pass
- 合法性・会計・A/B隔離・最大計算量・診断保存則の独立静的レビュー3系統: blocking issue 0

固定物は次のとおりで、コミットは作成しない。

- baseline commit: `5145bc7`
- `main.cpp` SHA-256: `a042123c836467c3dd265e3042ec09911b8b086c033dcb5cd2782f8951fd4e06`
- Treatment release: `/private/tmp/ahc069_cross_tier_v23`
- Treatment release SHA-256: `b7ab0f7dfc86a514c440cc793c50a1adb0c1cb59828318781ee96d0aae351d84`
- same-source Control: `/private/tmp/ahc069_cross_tier_v23_control`
- Control SHA-256: `1fecf53325bc466b254b3e62f9124f899d1b2391de4b79835cdcc056516b11cc`
- sanitizer: `/private/tmp/ahc069_cross_tier_v23_san`
- sanitizer SHA-256: `6f4c5de60eb41ba575b8b399d739ef320694d19be7f9c4b7aaa098b06b6c519d`
- Treatment config: `pahcer/bench_cross_tier_v23.toml`
- Treatment config SHA-256: `590be51c11a52f20c8f329f7b38c0ebe96d21d468dc32b0c9aff0b8d42ac7ff8`
- Control config: `pahcer/bench_cross_tier_v23_control.toml`
- Control config SHA-256: `95dc73399ab075d9d53a3626e91aad90f5c477f685e3927b283953cef23a90a6`
- 既存Control JSON: `pahcer/json/result_20260803_003818.json`
- tester SHA-256: `3702067f731de62b30a395f99eee04a4a9e247a1f421e2c42556a9e45b62ec92`
- frozen `best_scores.json` SHA-256: `b4c70ce5bb84ec557353d33d6c41f96e857a6d74d5f3213c685a1f602c5d1fa6`
- seed 0 / 99 SHA-256: `61857f9adeb56546a876f9f54eec3e95be617d4a1135d987ae790a2248304754` / `cfb3c1678be95ff9ff760cafed7a36c99a769c4c75c679b4a0bdef7d8edf2ad8`

### 最終実行バッチ

ユーザーの明示指示に基づき、次を一つの最終バッチとして行う。

1. sanitizerをseed 0で単独実行し、ASan / UBSanと全error / identity fieldを確認する。
2. same-source Controlをseed 0〜99、threads=1、frozen best scoreで実行する。
3. Controlの100 score / stdoutを既存`result_20260803_003818.json` / `tools/out-multi-assignment-v20-control`と照合する。
4. Treatmentを同じseed 0〜99、threads=1、同じfrozen best scoreで実行する。
5. Treatment seed 0を同じbinaryで再実行し、stdoutとtiming以外の診断の決定性を確認する。
6. absolute score、frozen relative、勝 / 同点 / 負、seed比分位点、符号検定、内部CPU、cross-tier funnel / tier / fee class / 予測margin / 下流差 / 分解、既存保存則を読み取り専用でpaired比較する。

sanitizer異常、WA、非0終了、score欠落、Control互換性破壊、error / identity / validation field非0の場合は、それ以降の必要な読み取り以外を行わず報告する。最初の解答実行後は、結果にかかわらずsource、方針、binary、config、memoを変更せず、読み取りと報告だけを行う。

### v23 最終実行結果

ユーザーの明示指示に基づき、前節で固定したsanitizer seed 0、same-source Control 100 seed、Treatment 100 seed、Treatment seed 0再実行を順に行った。sanitizerはASan / UBSan異常なし、Control / Treatmentはいずれも100 / 100 ACだった。Controlは既存oracle `result_20260803_003818.json`と全score、旧Control stdoutと100 / 100 byte一致した。Treatment seed 0再実行もstdout byte一致、時間項目を除く診断449項目一致だった。全error / identity / validation / generation fieldは0である。

結果ファイルはControlが`pahcer/json/result_20260803_022343.json`、Treatmentが`pahcer/json/result_20260803_022605.json`である。

| 100 seed | Control | v23 Treatment | 差 |
|---|---:|---:|---:|
| absolute score合計 | 6,515,194,836 | 6,517,449,231 | +2,254,395 (+0.03460%) |
| frozen relative合計 | 9,951.751023 | 9,958.921477 | +7.170454 |
| solver内部CPU平均 | 1,004.860 ms | 1,174.424 ms | +169.564 ms (+17.67%) |
| solver内部CPU最大 | 1,618.894 ms | 1,661.803 ms | +42.909 ms |

勝ち / 同点 / 負けは`29 / 41 / 30`、seed比の幾何平均は`1.0006783`、p05 / median / p95は`0.983277 / 1.0 / 1.021396`だった。同点を除く符号検定は`p=1.0`、paired mean差のbootstrap 95%区間は約`[-123,150, +165,318]`で0をまたぐ。合計は正だが、100 seedだけでは統計的に確定できない小差である。

全100 seedで16回ずつ、計1,600回probeした。104回は合法な上位tierなし、552回はfuture-fit改善なし、37回はsampled DLP admissionで全候補が落ち、907回をQ2/H4比較した。比較候補数は1案471回、2案436回で、810回baseline、97回cross-tierを採用した。採用は59 seedに分布し、採用なし41 seedはControlとscore / stdoutが完全一致した。出力が変わったseedは全て採用seedである。

- compared: `+2`が773件、`+4`が570件
- adopted: `+2`が71件、`+4`が26件
- 料金損失0のcompared / screened / adopted: 全て0件
- 現在料金損失: `439,972`
- 採用案のscreen予測未来差: `+4,208,717`
- 採用案のscreen予測margin: `+3,768,745`
- 実score差から現在料金損失を戻した下流改善: `+2,694,367`
- 最終差: `-439,972 + 2,694,367 = +2,254,395`

したがって、短期screenは下流改善を約151万過大評価したが、合計の符号は正しく予測した。採用seed単位の予測marginと実score差の相関は`r=0.286`と弱い。`+2`だけを採用した35 seedは合計`+3,673,379`、`+4`だけの15 seedは`-1,236,709`、両方を含む9 seedは`-182,275`だった。これはseed内の他の採用と下流効果が混ざるためtier単独の因果ではないが、`+2`に見込みがあり`+4`が薄めている可能性を示す。

スコア分解では受入数が96件減る一方、受理集合の理想料金が`+4,698,541`、初期 / 最終料金が`+2,305,569`、移動費が`+51,174`となり、`2,305,569 - 51,174 = 2,254,395`で最終差へ一致した。単純な受入数ではなく、配置変更後に残る空間によって受理する価値の構成が変わったことが改善源である。

cross-tier専用CPUはgeneration平均`5.947 ms`、rollout平均`108.965 ms`、合計平均`114.912 ms`で、Treatment solver CPUの`9.78%`だった。Treatmentの内部CPU 2秒超は0件、Pahcer wall最大は`1,784.183 ms`である。実装・隔離・合法性・会計は成立し、小幅な正傾向は得たが、統計的不確実性と`+4`の悪化兆候があるため、このpackageを確定採用とはしない。

ユーザーの次の明示指示は、周長が長く既存組間の隙間へ入り込みながら外へ広がる配置を避け、池の歪みで長周長が不可避な場合だけ例外にする独立実験である。v23と混ぜず因果を分けるため、次節では`main.cpp`をcommit `5145bc7`相当へ復元してから実装する。コミットは作成していない。

