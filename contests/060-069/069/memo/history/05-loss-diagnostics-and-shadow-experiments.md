## 2026-08-02: 損失分解とコード内CPU計測 (`loss-diagnostics-v8`)

相対スコアから次の大きな変更先を決めるため、`cross-fitted-root-v7`の方策は変えず、行動確定後だけに診断を追加する。ローカル100ケースの比較対象はv7の`6,386,024,428`（`pahcer/json/result_20260802_020343.json`）。

### 損失の厳密分解

全到着を理論上の最小周長で受け入れられると仮定した、容量制約も池も無視する実現不能な上界を`offered ideal fee`とする。回収可能額ではなく、損失源の比率を見るための共通分母である。丸め後の料金を使うと、raw moneyについて次が厳密に成り立つ。

```text
offered ideal fee - realized raw money
  = rejected ideal fee
  + accepted initial shape loss
  + accepted relocation fee loss
  + movement cost

realized raw money
  = accepted final fee - movement cost
```

拒否価値はまず判断statusの`upper-bound / actual-fee / no-region`に分ける。それとは独立に、拒否全件について現在盤面の最大空き4連結成分を完全BFSし、`feasible / unplaceable`に分ける。配置不能は排他的に次の3種類へ分ける。

1. 池だけの盤面でも最大連結成分がP未満の`static geometry`
2. 空きセル総数がP未満の`capacity`
3. 空きセル総数は足りるが最大連結成分がP未満の`fragmentation`

受理側は`minimum template / extended template / connected growth / unclassified`、初期周長超過、初期料金、最終料金、再配置による最大周長悪化、移動費を記録する。各offerについて`P * (T - S)`のcell-timeも同じ分類で集計し、拒否価値だけでなく占有時間当たり価値を比較できるようにする。`ActualFeeRejected`の候補料金と理想料金との差は拒否価値の内訳であり、全体gapへ再加算しない。

行動、候補順位、tie-break、owner/groupsの既存更新、stdoutは変更しない。候補探索中に保存する値は候補料金のログだけで、追加BFSと損失集計は行動・状態更新後に行い、どの診断値も判断へ戻さない。現方策では拒否と同時の移動はないが、将来の不整合検出用に`decomp_rejected_move_plans`も出力する。

### 入出力待ちを除く時間計測

Pahcerのwall timeはinteractive testerとの待ちを含むため、コード内で次を別々に測る。

- `timing_solver_cpu_ms`: 初期入力後の前処理と、各turnの入力完了後から行動・状態更新までのprocess CPU。入力待ち、出力、後段の損失BFSを除くため、探索本体の主指標とする。
- `timing_process_cpu_ms`: 初回入力前から最終診断後・巨大なstderr出力前までのprocess CPU。blocking input/output待ちとdescheduleは数えず、入力parse、出力format、診断CPUは含む。
- `timing_solver_wall_ms`: 上と同じsolver区間のwall。明示的な入出力待ちは除くが、OSによるdescheduleは含む。
- `timing_input_wall_ms`: `cin`区間。純待ちだけでなくparseと初期確保の一部も含む。
- `timing_output_wall_ms`: stdout生成とflush。formatとpipe backpressureを含む。
- `timing_diagnostic_{cpu,wall}_ms`: 方策確定後の損失診断。
- `timing_protocol_wall_ms`: 初回入力前から最終診断後まで。入力待ち込みなので性能比較には使わない。

wallの区間は重複せず、`protocol = input + solver + diagnostic + output + unaccounted`。`preprocess_wall`はsolverの部分集合、`max_solver_turn_wall`は最大値なので合計には加えない。`std::clock()`を使うためAtCoder Linuxの単一thread実行ではprocess CPUになり、interactive待機を除外できる。`solver`には移動損失を観測する小さなloop、`process`には全診断を含む。snapshotは最終`cerr`前なので巨大な診断ログの出力時間は含まない。

### 実行後に確認する保存則

各ケースで以下の`*_error`と異常counterがすべて0、`decomp_observed=1000`、`decomp_reconstructed_absolute_score`がtester scoreと一致することを確認する。

```text
decomp_{offered,cell_time,accepted_initial,accepted_final,gap}_identity_error
decomp_observed_count_error
decomp_total_count_partition_error
decomp_{accepted,finalized,rejected}_count_error
decomp_rejected_status_count_error
decomp_upper_count_partition_error
decomp_unplaceable_count_partition_error
decomp_accepted_source_count_error
decomp_rejected_{fee,cell_time}_partition_error
decomp_rejected_status_{fee,cell_time}_error
decomp_unplaceable_{fee,cell_time}_partition_error
decomp_accepted_source_{ideal_fee,initial_fee,perimeter}_error
decomp_actual_candidate_fee_identity_error

decomp_feasibility_mismatches
decomp_accepted_status_mismatches
decomp_accepted_plan_mismatches
decomp_accepted_source_mismatches
decomp_rejected_move_plans
decomp_rejected_status_mismatch
decomp_accepted_unclassified_count
decomp_accepted_decision_fee_error
decomp_accepted_decision_perimeter_error
```

追加で`actual ideal = actual candidate fee + actual geometry loss`、`process CPU >= solver CPU + diagnostic CPU`、wall保存則、raw moneyが非負であることを確認する。保護経路版は旧v7の100 stdoutとbyte単位で比較する。

### 実行前検証

- `main.cpp` SHA-256: `8d72bd795371f41685872bbe903d59f817793c53604e3176ed7b2a23c8c13179`
- Clang C++17 release binary SHA-256: `0c44108c8a8e51d21b150b1de6f9b3a3e0284fbd0e479d31fc9ed68f3acec686`
- protected-only binary SHA-256: `a04e1d9cf2ddabc39b2e80b5295bd409062ce2e67526834704ee7e00415d8a13`
- Clang C++20 release binary SHA-256: `a0e2d4b505fb1af3e5fc2ffd2e33fbb45e3dd752c538c7f3a03eb1ca8bfe56ca`
- ASan/UBSan binary SHA-256: `ec009fcd7dbc29178bcad5eb187084674afdd5ac1c5a3797e76cb1f8585fd9ee`
- Clang C++17/C++20通常版、protected-only版、GCC C++17の警告付きbuild/syntax検査: pass、警告0件
- ASan/UBSan build: pass（解答は未実行）
- Clang static analyzer: 指摘0件
- `git diff --check`: pass
- stderr key 267件: 重複0件
- v7との`ThetaEstimator::estimate`の正規化disassembly比較: 完全一致
- 独立静的監査3系統: 損失保存則、全reject BFS、source pattern、時間区間、方策への非干渉、整数範囲・配列境界を確認。途中でextended sourceの重複counter解釈を発見して排他的patternへ修正後、blocking issueなし

通常版Pahcer設定は`pahcer/bench_loss_diagnostics_v8.toml`、固定binaryは`/private/tmp/ahc069_loss_diagnostics_v8`。protected-only比較は`pahcer/bench_loss_diagnostics_v8_protected.toml`と`/private/tmp/ahc069_loss_diagnostics_v8_protected_only`を使う。ここまで固定したbinaryは一度も実行していない。以降はprotected-only 100ケースと通常版seed 0〜99を最後の実行batchとして走らせ、結果確認後はAGENTS.mdに従ってユーザーの次の明示指示まで解法・コード・メモを変更しない。

## 2026-08-02: 需要重み付き空き成分評価 (`component-shadow-v9`)

`loss-diagnostics-v8`の100ケースでは、配置不能18,022件のうち18,021件が空き面積不足ではなくfragmentationであり、その理想料金は`4,761,266,036`だった。配置不能時にも平均約411.82セル空いていたため、従来の空き正方形anchor数ではなく「次の要求人数を受け入れられる最大空き連結成分」を通常配置候補の主評価にする。

過去の`component-size-v1`は`sum(c_k^2)/F^2`だけを全候補の主値にしてseed 0で悪化した。今回は次の点が異なる。

- 公開生成分布の要求人数と最小周長時の料金規模を使う需要閾値評価
- 現行のtemporal costで絞った最大6件の同一最良周長tierだけを比較
- 現在組の滞在中から選ぶ既存の3つの等確率質量snapshotを維持
- admission shadow、候補生成、root screen/confirmation、再配置の損益式は変更しない

### Component shadow

生成器は`U`を`[2,sqrt(150)]`から一様に取り、`P=round(U^2)`とする。したがって、各人数の確率を厳密に

```text
q_p = (sqrt(min(150,p+0.5)) - sqrt(max(4,p-0.5)))
      / (sqrt(150)-2)
```

とする。人数`p`の最小周長を`Lmin(p)`とし、要求人数が`p`である将来組のcompact料金に比例する重みを

```text
w_p = q_p * p * 4*sqrt(p)/Lmin(p)
```

とする。`D^0.9`と価格ノイズの期待倍率は`P`から独立な共通因子なので、配置順位からは消える。累積値を`p=150`で1に正規化し、最大空き4連結成分サイズが`L`の盤面の主値を

```text
G(L) = sum_{p=4..min(L,150)} w_p
```

とする。これは成分サイズの滑らかな二乗評価ではなく、実際の要求`P=4..150`を置ける閾値を越えたときだけ増える。

`L>=150`では主値が飽和するため、主値が同じ候補間だけ、全空き成分`c`について

```text
Pair = sum_c c*(c-1)/2
```

を比較し、一体の大きな空きreservoirを優先する。主値とPairを係数で混ぜず、`G -> Pair -> 既存incremental cost`の辞書順にする。

各候補について、条件付き未来到着分布の`1/6, 1/2, 5/6`分位に対応する3時点を使う。候補組は全時点で占有、既存組`j`は問題のイベント境界と同じく`T_j < snapshot`のときだけ解放する。各値は従来と同じ

```text
0.75 * average + 0.25 * minimum
```

で集約する。空き集合は時刻とともに単調に増えるため、実効的な時間重みは近い順に`0.50, 0.25, 0.25`となる。component値は配置候補順位だけに使い、現在料金、admission shadow、rootの金銭marginには加えない。

### 増分DSU

3時点を個別BFSすると旧largest-empty-square DPより定数倍が重くなるため、候補ごとにDSUを1個作り、snapshot順に新しく解放されたセルだけを追加する。

- セル追加時に既に有効な4近傍とunionする
- サイズ`a,b`の成分を結ぶと`Pair`へ`a*b`を加える
- 最大成分サイズもunion時に更新する
- 各セルと各辺は3断面合計で一度だけ追加処理される

盤面走査は`3*N^2`、連結処理は全断面合計`O(N^2 alpha(N^2))`で、heap確保や盤面copyはない。`N=50`固定なので2500要素のstack arrayを使う。

### 変更境界と診断

旧`FUTURE_FIT_*`、`compact_fit_utility`、`evaluate_compact_fit`と関連名を削除し、診断を次へ改名した。

```text
placement_component_shadow_turns
placement_component_shadow_changes
placement_component_shadow_snapshots
```

ordinary primaryとrunner-upは同じcomponent comparatorを使う。`choose_temporally_coherent_region`はsynthetic rolloutでも共用されるため、Q2/H4 screenとQ8/H12 confirmation内の未来方策も同じ評価へ変わる。一方、shortlistは同一最良周長tierに限定されているので、その到着の料金・周長・受理条件はcomponent評価だけでは悪化しない。`NoRegion`には候補自体がないため、この版は直接救済ではなく分断の予防である。

### 実行前検証

- `main.cpp` SHA-256: `8b55b91e662036d6961d2c6fc241f8b80176bb7e389716d0b61c3b3b6994d8b9`
- Clang C++17 release binary SHA-256: `e2046e7263df6eee6c9b33e055b5a16092f04dc11875d8ca66ab33011b813a68`
- protected-only binary SHA-256: `9350cc2f4de730eb762a95421f9c2a0e7b6e6404fcd5acc3a9422cd0d84a2951`
- Clang C++20 release binary SHA-256: `e2d8eaf2f9af7ac81602cf1eac102eee1112ea40854575a8399d26bafd4dea19`
- ASan/UBSan binary SHA-256: `db2f5f0dc0e4a335f6818c2a63be9ae3e5e65488248da29457d0ec454b542841`
- Clang C++17/C++20通常版、protected-only版、GCC C++17の警告付きsyntax検査: pass、警告0件
- release/protected/C++20/ASan+UBSan build: pass（解答は未実行）
- Clang static analyzer: 指摘0件
- `git diff --check`: pass
- `main.cpp`内の旧`FUTURE_FIT/future_fit/compact_fit`参照: 0件
- 独立静的監査3系統: 確率質量と正規化、strict退去境界、候補常時占有、増分DSUと個別BFSの同値性、union時のPair差分、配列境界・整数範囲、primary/runner-up、実到着/synthetic rollout、診断接続、既存損失保存則への非干渉を確認し、blocking issueなし

Pahcer設定は`pahcer/bench_component_shadow_v9.toml`、固定binaryは`/private/tmp/ahc069_component_shadow_v9`。ここまで固定したbinaryは一度も実行していない。以降はseed 0〜99の100ケースを最後の実行batchとして走らせ、既存の損失保存則、component診断、solver CPUを確認した後、AGENTS.mdに従ってユーザーの次の明示指示まで解法・コード・メモを変更しない。

### 100ケース結果と棄却

固定した`component-shadow-v9`をseed 0〜99で実行した結果は`pahcer/json/result_20260802_032016.json`。

```text
component-shadow-v9: 6,160,973,284
loss-diagnostics-v8: 6,386,024,428
差分:                 -225,051,144 (-3.524%)
seed勝分敗:           5勝0分95敗
```

100ケースすべてvalidで、損失分解、状態、料金、source、再構成scoreの既存保存則はすべて0だった。したがって実装不整合ではなく方策差である。component評価は57,643 turnで動作し、29,661 turn（51.456%）の配置を旧future-fitから変更した。

主要な変化は次の通り。

```text
accepted:                         74,488 -> 75,431  (+943)
NoRegion:                         16,468 -> 14,847  (-1,621)
fragmentation ideal loss:  4,761,266,036 -> 4,267,527,970  (-493,738,066)
rejected ideal loss:       6,232,328,943 -> 6,104,661,833  (-127,667,110)
initial shape loss:        1,161,067,554 -> 1,510,750,341  (+349,682,787)
movement cost:                 2,986,377 ->     6,021,844  (+3,035,467)
```

拒否価値の改善`+127,667,110`より、受理した組の形状悪化`-349,682,787`と移動費増`-3,035,467`が大きく、合計がscore差`-225,051,144`と厳密に一致した。connected-growth受理は20,533件から24,804件へ増え、そのshape lossも`1,143,177,236`から`1,492,428,471`へ増えた。最大成分を残す配置は将来の受理数を増やしたが、compact領域を早く消費して多数の到着を悪い周長で受け入れる方向へ寄りすぎたと解釈する。

solver CPUも悪化した。

```text
                       mean      p50      p95      max
loss-diagnostics-v8  1322.7   1296.8   1700.4   2018.9 ms
component-shadow-v9  1817.5   1837.9   2310.1   2423.5 ms
```

この結果によりcomponent-shadowは棄却し、次版の実装前に旧`future-fit`へ戻した。固定v8 binaryのsymbol/DWARF、HEADの旧関数、source差分を独立監査し、component固有参照が0件、関数signature・選択block・診断名がv8と一致することを確認した。復元時の一時的なSHA差は`evaluate_compact_fit`宣言の改行1行だけで、意味差ではなかった。

## 2026-08-02: NoRegion Push-out再配置 (`no-region-pushout-v10`)

component-shadowのように通常配置全体を変えるのではなく、旧future-fit方策が`NoRegion`を返したturnだけを直接救う。基準行動はRejectであり、既存のcompact rescue探索を次の2モードへ分けて共用する。

```text
CompactAccepted: 既存の非compact受理をcompact化
NoRegionPushOut: Rejectを基準に、blockerを押し出して到着組をcompact受理
```

### 発火条件と損益gate

NoRegionでも空きセル総数が`P`未満なら、再配置で占有面積は変わらないため即Rejectする。したがってPush-outの対象は空き面積は足りるfragmentationだけである。blocker数には1組、2組などの意味的上限を置かない。

到着組の最小周長料金を`F`、targetに重なる全blockerの移動費合計を`Cmove`、既存admissionのfull-horizon shadowを`Shadow`とすると、repair前に必ず

```text
F - Cmove > Shadow
```

を要求する。全targetはblockerを1組以上持つため、全active組の最小移動費すら引いた上界がshadow以下なら、anchor scan前に安全にRejectできる。shadowは通常受理と同じ長期admission gateであり、後述の短期rolloutでは再度scoreから引かない。

初版は既存組の丸め後料金が悪化しないdestinationだけを許す。各移動候補について

```text
fee(group, max(old max perimeter, destination perimeter))
  == fee(group, old max perimeter)
```

を要求し、完成planでもrelocation fee lossが0であることを再検証する。よって直接利益は厳密に`F - Cmove`。周長悪化を料金丸めが吸収する範囲は許すが、既存組の料金を実際に下げて到着料金で補う拡張はまだ行わない。

### TargetとPush-out先

池に重ならない全minimum-perimeter target anchorをprefix sumでcheap scanし、次の2尺度で上位を残す。

1. target内の占有セル数
2. 各占有セルへ組の移動費を人数按分した近似移動費

NoRegionは頻度が高いため各尺度96件とする。全anchorと2本のindex配列を保存して`partial_sort`する旧実装は、各尺度のworstを先頭に持つbounded heapへ置換した。最終keyは一意な列挙順なので、これは旧partial-sortと同じtop-k集合を保ちつつ、turnごとの大きなvector確保を除く。

正確なblocker集合を復元した後は、`F-Cmove`降順、blocker数昇順、blocker総面積昇順、列挙順で処理する。targetに重なるblockerをすべて旧領域から一旦外し、到着targetを予約してから各blockerのdestination poolを作る。

NoRegionではdestinationの第1基準を「Push-out前から空いていたセルとの重なり」とする。続いて「blockerを除いてできた空きとの重なり」、退去時刻のtemporal cost、周長を比較する。これにより、分断された既存空きへblockerを押し出し、blockerの旧領域側を到着組のまとまった空間に変換する。

複数blockerは既存のgreedy spineとbeam assignmentで同時に割り当てる。validatorは全moverの旧領域を先にclearしてから全destinationとarrivalを検査するため、他moverの旧領域利用、swap、3組以上の循環も問題仕様どおり扱える。最終領域の重複、池、盤外、非連結、セル数不一致、同一領域への見かけ上の移動は拒否する。

### Rejectとの共通乱数rollout

完成候補は最大2件とし、同じQ2/H4 synthetic arrivalsを次の枝へ流す。

```text
Reject branch:
  owner = 現在盤面
  moves = 0
  current arrival = inactive

Push-out branch:
  owner = blocker再配置 + current compact arrival
  moved max perimeterを更新
  current arrivalをTで退去heapへ追加
```

候補marginは

```text
(F - Cmove)
  + mean(future fee on Push-out branch - future fee on Reject branch)
```

で、strict positiveの候補だけ採用する。現在到着料金と移動費はdirectへ1回だけ入り、rollout outcomeは未来組の料金だけなので二重計上しない。NoRegionにはordinary normal alternativeを作らない。scenario生成に失敗し未来組が残る場合はRejectし、残り0組だけはshadowを通ったdirect利益で採用する。既存confirmation枠を消費して通常root方策を変えないため、初版のPush-outにはQ8/H12 confirmationを追加しない。

採用時は元のNoRegion decisionをコピーした後、`status=Accepted`、到着領域、周長、料金、sourceをminimum templateへ明示更新する。Reject branchでは`baseline.cells`を参照せず、arrivalなしplanとして扱う。

### 決定的work caps

NoRegionはv8で平均約165件/caseあり、既存Accepted rescueより頻繁なので、時刻依存のcutoffではなく再現可能な作業量だけを小さくする。

```text
target shortlist:             96 / metric
repair target:                 4
destination anchor/blocker: 2048
destination anchor/turn:    16000
legal destinations/blocker:   40
retained destinations:         8
beam nodes/turn:             1024
rollout candidates:             2
```

blocker数自体は無制限で、active組数、target面積、経済gate、共有anchor/node workだけが自然上限となる。Accepted compact rescueは従来の160/8/4096/50000/64/10/2048をそのまま使う。

### 診断と保存則

既存`rescue_*`総計には共用探索分としてPush-outも入るため、`pushout_*`を独立集計し、主要項目について`compact_rescue_* = rescue total - pushout`も出力する。Push-out専用には、対象turn、面積不足、shadow filter、target/destination/beam work、feasible plan、screen Reject、採用、blocker数、移動組・セル数、料金、移動費、future delta、screen margin、専用CPUを記録する。

各ケースで次を新たに確認する。

```text
pushout_eligible
  = pushout_area_insufficient
  + pushout_no_economic_target
  + pushout_no_repair
  + pushout_screen_rejected
  + pushout_adopted

original baseline NoRegion
  = final NoRegion rejected + pushout_adopted

pushout_direct_gain
  = pushout_arrival_fee
  - pushout_movement_cost
  - pushout_relocation_fee_loss

pushout_feasible_plans = sum(feasible blocker histogram)
pushout_adopted        = sum(adopted blocker histogram)
```

対応する`pushout_{funnel,status,direct,feasible_histogram,adopted_histogram}_identity_error`は0でなければならない。既存の全`decomp_*_error`、status/plan/source mismatch、tester scoreと再構成scoreの一致も維持する。`pushout_cpu_ms`はNoRegion入口から全探索・screenまでのprocess CPUで、interactive input待ちを含まず`timing_solver_cpu_ms`の部分集合になる。

### 実行前検証

- `main.cpp` SHA-256: `14dcafa85365e3382391898105afae058514c223b1960c61e028362aca1bbcc9`
- Clang C++17 release binary SHA-256: `b1754cc27b3d3451c3a403b9551409e9bb32ac333bf814cabddda99a65058e49`
- Push-out無効binary SHA-256: `db9438ef672d037460a17a9825bcf34208f41441d891e2567c62c7d17a324e6c`
- Clang C++20 release binary SHA-256: `a82bfa75e2a9e2e429a7e65066985b76ba76cf4f7414e497098ac68b99f39dd8`
- ASan/UBSan binary SHA-256: `5e65891850cc077763612d8eee2451f2a60c4e1634a5e73a55d1be43be1a2a5f`
- GCC C++17、Clang C++17/C++20、Push-out無効版の警告付きsyntax検査: pass、警告0件
- Clang static analyzer: 指摘0件
- release/C++20/Push-out無効/ASan+UBSan build: pass（解答は未実行）
- `git diff --check`: pass
- 独立静的監査: Reject baseline、null安全性、Accepted status/source更新、同時移動、料金・移動費計上、shadow gate、bounded heapの旧top-k同値性、全work cap接続、funnel/status/histogram/direct保存則、既存Accepted rescue定数維持を確認しblocking issueなし

Pahcer設定は`pahcer/bench_no_region_pushout_v10.toml`、固定binaryは`/private/tmp/ahc069_no_region_pushout_v10`。A/B用は`pahcer/bench_no_region_pushout_v10_disabled.toml`と`/private/tmp/ahc069_no_region_pushout_v10_disabled`。ここまで固定した解答binaryとsanitizer binaryは一度も実行していない。以降はsanitizer確認、Push-out無効版のv8互換確認、通常版seed 0〜99を同一固定sourceの最終実行batchとして行い、結果確認後はAGENTS.mdに従ってユーザーの次の明示指示まで解法・コード・メモを変更しない。

## 2026-08-02: 案3「期限レイヤー正規形への大域再構成」単独検証（棄却）

### 検証目的と実装境界

GPTによる独立設計案のうち案3を、現行解法への追加機能ではなく、それだけで完結する解法として作り直して検証した。旧実装のshadow、future-fit、Push-out、rescue、rollout、baseline/fallbackは一切利用していない。旧コードを残したまま経路だけ切る形にもせず、`main.cpp`を新しい`DeadlineLayerSolver`へ置換した。

案3単独版が選ぶ行動は次の3種類だけである。

1. Reject
2. 現在の空き領域へのDirect placement
3. 期限レイヤー正規形を作る複数組同時Rebuild

Direct placementも旧実装の配置候補や評価を流用せず、新しいatlas、growth、BFSから生成した。評価には独立案で提案されていた案2の共通評価部を案3の内部要素として用い、`J = ΔK - movement cost - OC_i + ΔPhi`でReject、Direct、Rebuildを比較した。

### 案3単独版の内容

- 未知パラメータ`theta`は61点gridのposteriorで推定し、未到着組については生存・順位選択の尤度も入れた。
- 条件付き未来代表requestを256件生成し、turn 0/4/8/16、その後16turnごと、時刻bucket境界、最後の16turnで更新した。
- 16個の時刻bucketごとに容量の影価格DLPを計算し、座標側は8 sweepで空間価格へ落とした。
- 4つの将来snapshotで退去後の空き連結成分を測り、`W(a)=h(total)-sum(h(component))`、`Phi=-sum(omega*W)`としてfragmentationを評価した。
- Rebuildではowner/freeの商グラフからworkspaceを作り、対象組を退去時刻`T`の降順に配置してから逆向きに監査し、各退去prefixで残りの空きが連結になる期限レイヤー正規形を要求した。
- active組が16以下なら全静的連結成分を対象にし、低`R`では条件付きで24組まで拡張した。それ以上はfree-closedな局所workspaceを使った。
- Rebuildは6 root、beam幅64、workspace node/probe上限4096、1 caseあたりprobe上限131072、各quartile 8回の計32 triggerとした。

### 固定物と実行前検証

- 案3単独版`main.cpp` SHA-256: `73b7e32f92feb240fb00f75f1ef6344f12ef14ac106dd96df92929215f658a1b`
- 固定release binary: `/private/tmp/ahc069_deadline_canonical_v1`
- release binary SHA-256: `9a6c106a8ceb8001d68a47fb4f0bdd13665fc085c2acba0b3e516e4e83e263fc`
- sanitizer binary: `/private/tmp/ahc069_deadline_canonical_v1_san`
- sanitizer binary SHA-256: `51f63b9e984312f359207a18cd245b1a0ea7e4d70704fb89d9554e465f8d2867`
- 置換前の解法退避: `/private/tmp/ahc069_main_before_deadline_only.cpp`
- 置換前ソース SHA-256: `533c207817aea26d55328606f5ace0730314003982a965bb3c3d0972e9434fc6`
- Clang C++17/C++20、GCC C++17のsyntax検査: pass
- Clang static analyzer: pass
- `git diff --check`: pass
- sanitizer seed 0: ASan/UBSan error 0、validation/canonical/action identity error 0
- sanitizer seed 0 score: `45,350,711`、Rebuild採用1回

### seed 0〜99の結果

設定は`pahcer/bench_deadline_canonical_v1.toml`、結果は`pahcer/json/result_20260802_162311.json`。比較対象は直前の現行解法`pahcer/json/result_20260802_144752.json`である。固定source・固定binaryをthreads=1で100 seed実行した。

| 指標 | 案3単独版 | 比較対象 |
| --- | ---: | ---: |
| 絶対スコア合計 | 5,284,956,858 | 6,391,210,376 |
| relative score合計 | 8,264.463950 | 9,928.721974 |
| 合計比 | 82.6910169918% | 100% |
| 勝ち / 引き分け / 負け | 0 / 0 / 100 | - |
| WA | 0 | - |

seed別の案3/比較対象比は、平均83.2472%、中央値83.0126%、p05 78.1747%、p25 81.2418%、p75 85.0171%、p95 91.1280%だった。最小はseed 23の74.8576%、最大でもseed 67の91.8387%であり、特定seedの外れではなく全seedで一貫して負けた。

### Rebuildの動作状況

- 受理75,307件、拒否24,693件
- Direct採用75,157件
- Rebuild採用150件、平均1.5件/case、最大7件
- 68/100 seedでRebuildを1回以上採用
- Rebuildによる移動178組、7,139セル
- 移動コスト645,484、再配置による料金低下1,662,501、合計損失2,307,985
- triggerは全caseで32回、各quartile 8回
- workspace node合計2,359,296、平均23,592.96/case、最大32,768
- layout node合計1,461,206、平均14,612.06/case、最大66,368
- connectivity probe合計3,379,765、平均33,797.65/case、最大112,300
- 完成layout合計29,546、平均295.46/case
- canonical failure、validation failure、action identity error、Reject/Rebuild move error、node cap errorはすべて0

Rebuildありseedの平均比は83.0038%、Rebuildなしseedでも83.7643%だった。Rebuildが壊れて動いていないわけではないが、採用頻度と改善量が小さく、Direct placementの損失を埋められていない。

### スコア損失の分解

- 全到着組を理想形で受理したときの料金合計: `13,782,407,302`
- 実際に受理した組の理想料金合計: `7,501,963,632`
- 実料金合計（score + movement）: `5,285,602,342`
- selection capture（受理組理想料金 / 全組理想料金）: 54.4314%
- shape retention（実料金 / 受理組理想料金）: 70.4563%
- 案3score / 全組理想料金: 38.3457%
- 比較対象score / 全組理想料金: 46.3722%
- 比較対象との差: `1,106,253,518`

移動コストと再配置料金低下の合計`2,307,985`は比較対象との差の0.2086%にすぎない。主な敗因はRebuildの移動費ではなく、独立版のDirect placementが初期配置の段階で形状を大きく損ねていること、およびRebuildがその損失を回収できるほど強くないことである。

`R`別に見ても、`R<=0.02`は20 seedで合計比82.2503%、`0.02<R<=0.05`は33 seedで83.1990%、`R>0.05`は47 seedで82.5442%だった。移動が安い低`R`で案3が逆転するという期待も成立しなかった。

### 実行時間

Pahcer wall timeは平均3.6894秒、中央値3.4985秒、p95 5.5438秒、最大7.8690秒で、100/100 caseが2秒を超えた。

interactive入出力待ちを除いたsolver内部CPU時間も平均2,672.85 ms、中央値2,451.15 ms、p90 3,743.04 ms、p95 4,450.33 ms、最大6,761.614 msだった。98/100 caseが1.4秒、85/100 caseが2.0秒を超えたため、Pahcer固有の入出力待ちだけが原因ではなく、探索自体も提出制限に対して重い。最大1 turn CPUは平均275.739 ms、p95 505.963 ms、最大706.527 msだった。

### 判断と復元

案3単独版は正しさの不具合なく意図どおりRebuildまで動作したが、100 seedすべてで比較対象に負け、合計スコアは82.69%、内部CPU時間も制限を大幅に超えた。低`R`を含む全層で優位性がなく、追加調整で埋めるには差が大きい。したがって案3「期限レイヤー正規形への大域再構成」は不採用とし、この系統の実装・調整を打ち切る。

ユーザーの明示指示を受け、案3導入直前に凍結していた`/private/tmp/ahc069_main_before_deadline_only.cpp`を`main.cpp`へ復元した。復元後の両ファイルは`cmp`で完全一致し、`main.cpp`のSHA-256は`533c207817aea26d55328606f5ace0730314003982a965bb3c3d0972e9434fc6`である。案3の結果を受けた新しい解法変更は加えていない。コミットはまだ行っていない。

復元後にClang C++17 release build、GCC C++17 syntax検査、`git diff --check`を行い、すべてpassした。解答の再実行は行っていない。

## 2026-08-02: grow-and-trim単独追加（実行前固定）

案3を棄却してmain.cppを直前解法へ復元した後、独立案の要素を現行解法へ一つずつ追加して比較する方針へ切り替えた。順序は次で固定する。

1. grow-and-trimだけを既存connected-growthへ追加
2. sampled DLPを現行shadowと単独比較
3. helper探索を既存Push-outへの候補追加として比較
4. Phiとcrisisは上記が終わるまで保留

この節では第1段階だけを実装した。sampled DLP、helper、Phi、crisisに関する状態・評価・候補・発火条件は追加または変更していない。現行のshadow、future-fit、rollout、compact rescue、NoRegion Push-out、Deadline Layerのコードも変更していない。

### 候補生成

既存connected-growthが各seedからちょうどPマスへ成長するまでの処理、seed、frontier priority、stale entry更新、従来候補をすべて維持する。その同じselectedとfrontierを使って、先頭8 seedだけ次を追加で行う。

1. 従来のPマス候補を先に保存する。
2. 同じfrontierをP+8まで継続する。
3. P+8へ到達できないseedはgrow-and-trim候補なしとする。
4. 現在領域の関節点をTarjan法で毎回再計算する。
5. 関節点でない境界セルのうち、削除後の周長変化 2*k-4 が最小のセルを削る。kは選択済み近傍数。
6. 同値なら成長時に後から加わったセル、さらにセル番号の順で決定する。
7. 8セル削除してPへ戻し、サイズ、4連結、芝生、非占有を再検証する。
8. 従来connected-growth候補およびgrow-and-trim候補同士の重複を除く。

grow-and-trimは1回の通常配置呼出しにつき最大8候補で、既存候補を置換せず後ろへ追加する。既存のPlacementShortlistBuilder、周長優先、incremental/absolute cost、future-fit、shadow admission、root比較へPlacementSource::GrowAndTrimとして流す。評価上は従来のConnectedGrowthと同じfallback区分であり、専用ボーナスは与えない。

今回の「grow-and-trimだけ」という切り分けを守るため、独立案にあった新しいfree-degree/expiry-affinity frontierへは変更せず、現行connected-growthのfrontierをそのまま延長した。したがってA/B差は、Pで停止せず8マスovershootして連結trimする候補を追加したことに帰属する。

同じ通常配置器を使う既存synthetic rollout内でもgrow-and-trimは有効になる。実方策とrollout内方策を一致させるためroot-onlyには制限していない。8候補capは1呼出し単位であり、case全体のglobal capではないため、増えたCPU時間もA/Bの評価対象とする。

### Compile-time ablationと診断

AHC069_DISABLE_GROW_AND_TRIMを付けると追加候補を完全に無効化する。無効版では従来候補の列挙順、tie-break、shadow、rollout、Push-outのwork capを維持し、grow-and-trim診断はすべて0になる。

新たに次を記録する。

- 対象base候補、P+8未到達、完全成長、trim失敗
- 従来候補との重複、完成した一意候補
- 成長・trimセル数
- 元候補に対する周長改善合計と改善/同値/悪化件数
- shortlist入り、最終choice
- 実際に受理されたgrow-and-trimの件数、理想料金、初期料金、周長超過

次の保存則を追加した。

    base_candidates
      = growth_failures + full_growths

    full_growths
      = trim_failures + duplicate_candidates + unique_candidates

    full_growths
      = trim_failures
      + perimeter_improved_candidates
      + perimeter_equal_candidates
      + perimeter_worsened_candidates

    accepted_grow_and_trim <= accepted_growth

対応するgrow_and_trim_{growth_funnel,completion_funnel,perimeter_partition,source}_errorは全caseで0でなければならない。既存の全decomp_*_error、Push-out保存則、status/plan/source mismatch、tester scoreと再構成scoreの一致も維持する。

### A/Bの切り分け

現行比較対象はpahcer/json/result_20260802_144752.jsonで、100 seed合計6,391,210,376、WA 0。この結果はdeadline_enabled=0、pushout_enabled=1なので、TreatmentとControlの両方でAHC069_DISABLE_DEADLINE_LAYERを指定し、NoRegion Push-outは有効のままにする。

    Control:
      -DAHC069_DISABLE_DEADLINE_LAYER
      -DAHC069_DISABLE_GROW_AND_TRIM

    Treatment:
      -DAHC069_DISABLE_DEADLINE_LAYER

Controlは既存結果と全100 seedのscoreおよびstdoutが一致することを互換性gateとする。その後、同一seedのTreatment/Controlをpaired比較する。主指標は絶対score合計と比、勝/同点/負、seed別ratio分位点、変更seed、worst/best seed。時間はPahcer wallでなくtiming_solver_cpu_msの平均、中央値、p95、最大、paired増分を主に見る。

設定は次の2本。

- pahcer/bench_grow_and_trim_v13.toml
- pahcer/bench_grow_and_trim_v13_disabled.toml

どちらもseed 0〜99、threads=1、固定best scoreを使う。sanitizerはgrow-and-trim経路が多い既存seed 67とsmoke用seed 0で確認してから100 seed A/Bを行う。

### 実行前固定物

- 復元元main.cpp SHA-256: 533c207817aea26d55328606f5ace0730314003982a965bb3c3d0972e9434fc6
- grow-and-trim追加後main.cpp SHA-256: 016a661a104cd351bedb75854672c640f5df101f60b9f281406ed4ffacad9786
- Treatment binary: /private/tmp/ahc069_grow_and_trim_v13
- Treatment binary SHA-256: bb16fbe8d90262cafc69b4dd41cfceb64daa16b7ffd13fc0d09d8a80eedbd747
- Control binary: /private/tmp/ahc069_grow_and_trim_v13_disabled
- Control binary SHA-256: 54a7cdcb19d9d14e30a9377416655c822605abab4acf1bf033591d1fb9691bdf
- sanitizer binary: /private/tmp/ahc069_grow_and_trim_v13_san
- sanitizer SHA-256: ef9217509df26bc7aabce4c40a3cde21ac4039b9b2b988d646e80815cda4a628
- Treatment config SHA-256: cbb1eed5c168ff31d48a5591c9aedb20bdc4f0b55ae712c938de86510008a989
- Control config SHA-256: 2f903233d5c3a5316cb501cfa05e9631f9f98e06260632325c11dd276a9b058b
- tester SHA-256: 3702067f731de62b30a395f99eee04a4a9e247a1f421e2c42556a9e45b62ec92
- pahcer/best_scores.json SHA-256: f7b224dd97e8df62cdb73eadb5609eb37417ee56f9dbf35e893895f37cc3f5fa
- seed 0 SHA-256: 61857f9adeb56546a876f9f54eec3e95be617d4a1135d987ae790a2248304754
- seed 99 SHA-256: cfb3c1678be95ff9ff760cafed7a36c99a769c4c75c679b4a0bdef7d8edf2ad8

Clang C++17/C++20、GCC C++17、Treatment/Control release build、sanitizer build、Clang static analyzer、git diff --checkはすべてpassした。独立静的監査ではTarjan、frontier継続、合法性、source伝播、disable時の挙動保持、3本のfunnel、他要素との分離を確認しblocking issueなし。

ここまでの固定Treatment/Control/sanitizer binaryはまだ一度も実行していない。以降はsource、方針、診断、設定、binaryを変更せずに最終batchを実行し、結果報告後はAGENTS.mdに従ってユーザーの次の明示指示を待つ。

## 2026-08-02: grow-and-trim単独追加の結果

前節で固定したbinaryをseed 0〜99、threads=1で実行した。Controlはgrow-and-trim無効、Treatmentはgrow-and-trim有効であり、それ以外は両方ともDeadline Layer無効、NoRegion Push-out有効、現行shadow有効で揃えた。

| 指標 | Control | grow-and-trim |
| --- | ---: | ---: |
| 絶対スコア合計 | 6,391,210,376 | 6,435,788,022 |
| relative score合計 | 9,928.721974 | 10,010.839344 |
| 差 | - | +44,577,646 (+0.69748%) |
| 勝ち / 引き分け / 負け | - | 67 / 1 / 32 |
| WA | 0 | 0 |

結果ファイルはControlが`pahcer/json/result_20260802_170905.json`、Treatmentが`pahcer/json/result_20260802_171100.json`である。seed別比は平均100.8371%、幾何平均100.8159%、中央値100.9768%、p05 97.6975%、p95 103.6204%。最悪はseed 66の94.617%、最良はseed 59の107.344%だった。paired bootstrapによる合計差の95%区間は約+16.28M〜+72.87Mで、100 seedでは正方向だった。

solver内部CPUはControlが平均853.37ms、中央値856.84ms、p95 1081.08ms、最大1150.58ms、Treatmentが平均993.81ms、中央値990.49ms、p95 1318.02ms、最大1467.26ms。paired平均増分は約140.44msだった。

Treatmentのgrow-and-trim診断合計は、base 183,160、P+8未到達32,976、完全成長150,184、trim失敗0、重複36,018、一意候補114,166、周長改善合計720,034、改善/同値/悪化122,656/27,528/0、shortlist 21,308、choice 12,418、実受理11,311。全funnel、decomposition、Push-out、status保存則は0だった。

スコア差の分解は、棄却理想料金が+35,511,315悪化、初期shape lossが-80,209,961改善、movement costが+121,000悪化し、合計+44,577,646となる。受理数は292件減ったが、grow-and-trimによる初期形状改善がそれ以上だった。Rの低・中・高の全層でも合計差はそれぞれ+0.193%、+0.842%、+0.822%だったため、第1段階は採用し、以後のControl/Treatment両方で有効に固定する。

## 2026-08-02: sampled DLPによる現行shadow単独置換（実行前固定）

独立案の第2段階として、grow-and-trimを両側で固定したまま、採否の機会費用だけを現行64-bucket shadowとsampled DLPでA/B比較する。helper探索、Phi、crisisは追加していない。future-fit、compact rescue、NoRegion Push-out、root候補、全tie-breakとwork capも変更していない。

### DLP専用posteriorと未来代表

未知thetaは`2000,2100,...,8000`の61点一様gridとする。現行shadow・既存配置が使う従来posteriorはControl互換のため変更せず、DLPだけが公式生成器の丸めを含む離散PMFを使う。

`l=D-1`、`H=100000`として、`l=0`の質量は`1-exp(-0.5/theta)`、`l>=1`は`exp(-(l-0.5)/theta)*(1-exp(-1/theta))`、打切り正規化は`1-exp(-(H-0.5)/theta)`である。観測尤度は観測数、`l=0`件数、`sum(l)`の十分統計量から計算する。未到着組の`S>current_s`尤度は、各thetaについて公式離散PMFからbackward recurrenceで作った`61*100000`のfuture-start survival表を使う。表はfloatで約23.27MiB、Treatmentだけ初期化し、Controlではcompile-timeに完全に除外する。

各rebuildでposteriorの10/30/50/70/90%分位点を取り、合計256件の決定的future representativeを作る。theta層の件数は51/51/52/51/51、duration/start/P/valueにはindex 1〜256のradical inverse base 2/3/5/7を使う。durationは`S_future>current_s`で条件付けた公式離散CDF、startは`current_s+1,...,H-D`から一様、PとVも公開生成分布どおりにinverse transformし、報酬は理論最小周長での丸め済み理想料金とする。endpoint衝突による再生成だけは依存が弱いため無視する。各標本の重みは`remaining_groups/256`で、重み合計は残り組数になる。

### 16時間bucket DLP

rebuild時刻`S`から`H`を最大16個の正幅な等時間半開区間`I_b`へ分ける。active組の既知退去を引いた残容量は

    C_b = grass_cells * |I_b|
          - sum(active j) P_j * |[S,T_j) intersect I_b|

である。現在到着した候補はまだactiveでないため差し引かない。future representative `r`の負荷を`a_rb=P_r*|[S_r,T_r) intersect I_b|`、理想料金を`b*_r`として

    max sum_r w_r b*_r x_r
    s.t. sum_r w_r a_rb x_r <= C_b, 0 <= x_r <= 1

のfluid LPを作る。双対は

    D(mu) = sum_b mu_b C_b
            + sum_r w_r [b*_r - sum_b mu_b a_rb]_+

である。毎rebuildで`mu=0`から始め、bucket 0から順にGauss-Seidel coordinate minimizationを8 sweep行う。各coordinateは正のbreakpoint`(b*_r-sum(c!=b)mu_c a_rc)/a_rb`を並べ、weighted active loadが容量を跨ぐ点を厳密に選ぶ。8 sweep後だけ`1e-9`単位へ量子化する。

現在組の機会費用は

    OC_i = P_i * sum_b mu_b * |[S_i,T_i) intersect I_b|

である。最小周長料金上界と実配置料金の両方を従来どおりこの単一scalarと比較し、等値はRejectする。既存active組へOCを再課金せず、fragmentation/component補正も足さない。

rebuild triggerは0-based turn 0,4,8,16、以後16到着ごと、または前回の内部bucket境界を現在時刻が2本以上通過したときで、hard capは設けない。rollout内では全root branchが実turnで凍結した同じDLP価格snapshotを使い、synthetic branch固有の未来情報で再solveしない。DLP有効時、旧shadowは実turn・rolloutともcompile-timeに除外される。

### A/Bと診断

`AHC069_DISABLE_SAMPLED_DLP`で旧shadow Controlへ戻せる。両側でgrow-and-trimとNoRegion Push-outを有効、Deadline Layerを無効、threads=1、seed 0〜99とする。

    Control:
      -DAHC069_DISABLE_DEADLINE_LAYER
      -DAHC069_DISABLE_SAMPLED_DLP

    Treatment:
      -DAHC069_DISABLE_DEADLINE_LAYER

Controlは前段Treatment `pahcer/json/result_20260802_171100.json`の合計6,435,788,022と全100 seedのscore/stdout一致を互換性gateとする。Treatmentはpaired score、勝敗、分位点、bootstrap区間、損失分解に加え、solver内部CPUとDLP rebuild CPUを比較する。

DLP診断にはrebuild trigger内訳、real/rollout price call、生成request、coordinate update、正価格bucket、sample hash、dual objective、容量、提示負荷、機会費用、最大価格、rebuild CPU、invalid/nonfinite件数を追加した。次を全caseで0とする。

    sampled_dlp_request_count_error
    sampled_dlp_trigger_partition_error
    sampled_dlp_real_call_error
    sampled_dlp_rollout_call_error
    sampled_dlp_invalid_model_errors
    sampled_dlp_nonfinite_errors

rollout call保存則は`rollout_price_calls = rescue rollout policy steps + normal-root policy steps + root-confirmation policy steps + deadline rollout policy steps`である。既存grow-and-trim、Push-out、decomposition、status/plan/source保存則も全て維持する。

### 実行前固定物

- main.cpp SHA-256: `d558332ff9932d90369638ffb8cdfbe83338dcf73e075ffd4ea7d87720d2550b`
- Treatment binary: `/private/tmp/ahc069_sampled_dlp_v14`
- Treatment binary SHA-256: `ce8b1d91bcdfec5fbe6d47f30da613f40561f32260cd9319d199b69b4f37447f`
- Control binary: `/private/tmp/ahc069_sampled_dlp_v14_legacy`
- Control binary SHA-256: `3974b3f7ac7d063ad20fd7f8b4b3a072ea98560b8ce4b6a2ad4d7a6877a3a3b1`
- sanitizer binary: `/private/tmp/ahc069_sampled_dlp_v14_san`
- sanitizer binary SHA-256: `ed3be8d83a44173919b5e933f02d4fc9705325a689176ae769b77d7c58558906`
- Treatment config: `pahcer/bench_sampled_dlp_v14.toml`
- Treatment config SHA-256: `b86a9d51a1db7bb8db1594c3d98c54f3c8f69c663b42251300a740b97e511123`
- Control config: `pahcer/bench_sampled_dlp_v14_legacy.toml`
- Control config SHA-256: `aaada4f7642c300da17723d0a44b4027522ac0e7c091ba3e9f433e94d1aba73f`
- tester SHA-256: `3702067f731de62b30a395f99eee04a4a9e247a1f421e2c42556a9e45b62ec92`
- best_scores.json SHA-256: `f7b224dd97e8df62cdb73eadb5609eb37417ee56f9dbf35e893895f37cc3f5fa`
- seed 0 / 99 SHA-256: `61857f9adeb56546a876f9f54eec3e95be617d4a1135d987ae790a2248304754` / `cfb3c1678be95ff9ff760cafed7a36c99a769c4c75c679b4a0bdef7d8edf2ad8`

Clang C++17 Treatment/Control、Clang C++20、GCC C++17の警告付き検査、release/sanitizer build、Clang Static Analyzer、`git diff --check`は全てpassし、警告・指摘0件。独立した数理、call graph、A/B isolationの3監査でもblocking issue 0を確認した。ここまで固定した3 binaryはまだ一度も実行していない。

以降はsanitizer、Control互換、Treatment 100 seed、決定性probeを一つの最終batchとして実行する。最初の解答実行後は結果にかかわらずsource・方針・診断・config・memoを変更せず、結果を報告してユーザーの次の明示指示を待つ。

## 2026-08-02: sampled DLP単独置換の結果

前節で固定したbinaryをseed 0〜99、threads=1で実行した。Controlはsampled DLP無効の旧shadow、Treatmentはsampled DLPであり、両側ともgrow-and-trimとNoRegion Push-outを有効、Deadline Layerを無効に揃えた。

| 指標 | Control | sampled DLP |
| --- | ---: | ---: |
| 絶対スコア合計 | 6,435,788,022 | 6,515,194,836 |
| relative score合計 | 10,010.839344 | 10,141.642297 |
| 差 | - | +79,406,814 (+1.233832%) |
| 勝ち / 引き分け / 負け | - | 68 / 8 / 24 |
| WA | 0 | 0 |

結果ファイルはControlが`pahcer/json/result_20260802_173910.json`、Treatmentが`pahcer/json/result_20260802_174107.json`である。Controlは前段grow-and-trim結果と全100 seedのscoreおよびstdoutが完全一致した。Treatmentのseed比は算術平均101.3244%、幾何平均101.2970%、中央値101.4816%、p05 97.9882%、p95 105.5686%。paired bootstrap 95%区間は合計比100.7081%〜101.7535%で、100 seedでは明確に正方向だった。

損失分解では受理数が74,051から71,417へ2,634件減った一方、受理組の理想料金が+87,180,415増えた。初期shape lossは+7,724,934、移動費は+48,667悪化し、差し引き+79,406,814となる。NoRegion拒否は17,118から14,747へ2,371件減った。価値密度の低い仕事を早く断り、後続の高価値仕事と連結空間を残す選別効果が主因である。

solver内部CPUはControl平均994.420ms、p95 1,290.015ms、最大1,483.225ms、Treatment平均1,044.785ms、p95 1,381.960ms、最大2,285.441ms。paired平均増分は50.365msだった。DLP rebuild自体は平均71.605ms/case、最大107.942msで、seed 7の2.285秒はDLP計算だけでなく、方策変更によりcompact-rescue rolloutが4 turnから12 turnへ増えたことが大きい。通常ケースには余裕があるが、worst-caseの2秒リスクは残る。

DLPは全100 caseでrebuild 7,494回、future representative 1,918,464件、coordinate update 959,232回。invalid/nonfinite/request/trigger/real-call/rollout-call error、既存の全保存則、WAは0。seed 0の再実行stdoutも保存済みTreatmentと一致した。以上から第2段階は採用し、以後のControl/Treatment両方でsampled DLPを有効に固定する。

