## 2026-08-02: one-helper Push-out追加（実行前設計）

独立案の第3段階として、NoRegion Push-outのblocker再配置が完成しなかった局面だけに、追加のactive groupを1組同時移動するhelper探索を加える。grow-and-trim、sampled DLP、通常配置、future-fit、compact rescue、root rolloutは両側で固定する。Phiとcrisisは引き続き追加しない。

sampled DLP版100 seedではPush-out対象14,837 turnのうち、完成したのは118 turn・203 plan、採用90 turnで、10,069 turnが`no_repair`だった。beam node cap到達は0であるため、探索深さよりも「blockerの候補移動先が別の静止組に塞がれている」ことを今回の対象とする。

### 保護された第1段階

各NoRegion turnでは、従来どおり最小周長targetを列挙し、targetを直接占有するblocker集合`B`だけを全clearして既存repairを行う。候補順、最大4 target、destination anchor、greedy/beam、候補幅2を変えず、このblockers-only探索を先に完了する。1 planでも完成したturnではhelperを試さない。したがってhelperは既存候補を置換せず、従来`no_repair`だったturnにだけ新しい行動を作る。

### helperの因果的shortlist

helper対象は`1 <= |B| <= 3`のtargetだけとし、追加するhelperは1組、総mover数は最大4とする。既存Push-outの4組以上blocker経路は削らず、従来どおりblockers-onlyで扱う。

blockerのfee-neutral destination anchorが現在盤面では不合法なとき、その領域について次を厳密に判定する。

1. pondまたは到着targetとは重ならない。
2. blocker以外の占有ownerがちょうど1組`h`だけである。
3. したがって`h`をclearすれば、そのanchorはblockerの合法候補になる。

各targetのblockers-only workspaceについてownerの個数、`id+1`の和、二乗和の2次元prefixを一度構築する。候補領域の占有数を`c`、和を`u`、二乗和を`q`とすると、`q*c == u*u`かつ`u%c==0`は全占有セルのownerが同一であることの必要十分条件である。既存blocked prefixとの差からpond混入も除く。これによりblocked anchorごとのregion materializeやPセル走査をせずO(1)でnear-missを判定する。1 targetで最大1,024 probeとする。

target内ではhelperを次の辞書順で1組選ぶ。

1. helperを除くことで候補が開くblocker種類数が多い。
2. 開くdestination anchor数が多い。
3. helper移動費が小さい。
4. helperの退去時刻とblocker退去時刻中央値の差が小さい。
5. group IDが小さい。

ただし`target.immediate_improvement - move_cost(h) > sampled_DLP_OC`をstrictに満たすhelperだけを残す。複数targetのseedは、helper追加後direct gain、covered blocker数、unlock anchor数、従来target順、helper IDで並べる。

### helper付き同時repair

blockers-only候補が0件のときだけ、上位seedを最大2件試す。完成候補は最初の1件で止める。各seedで

    movers = B union {h}

をID順に正規化し、全moverの旧領域を同時にclearしてから到着targetを予約する。その同一workspaceでblockerとhelper全員のfee-neutral destination poolを再生成し、既存greedy spine＋beamで非重複配置を探す。helper専用work capは次で、既存Push-outのwork budgetを消費しない。

- repair attempt: 2/turn
- destination anchor: 1,024/mover、4,096/turn
- legal destination: 24/mover
- retained destination: 6/mover
- beam node: 256/turn

blocker数には既存経路を含む新しい制限を加えず、helper経路だけをD.7の最大4 moverに限定する。全moverの移動費を再計算し、既存validatorでactive・ID一意性・全clear・サイズ・4連結・pond・重複・周長・元領域との差・料金悪化0・移動費一致を検証する。到着料金から全mover移動費を引いたdirect gainがsampled DLP機会費用をstrictに超えることを再確認する。

完成したhelper planは特別ボーナスを持たず、既存Push-outと同じReject対Q2/H4 common-random-number rolloutを通す。未来が0件のときだけpositive direct gainでそのまま採用する。helperを含む`plan.moves`と`final_owner`をrolloutへ渡すため、移動後のownerと退去時刻も通常候補と同じ状態遷移になる。

### ablationと診断

`AHC069_DISABLE_PUSHOUT_HELPER`を付けるとnear-miss surveyとhelper第2段階をcompile-timeに無効化する。Controlでは従来target順、anchor/node budget、候補、rolloutを維持する。追加診断は次を含む。

- surveyed turn/target、probe、single-owner near-miss、overlap、evidence group
- considered/no-eligible-target/no-evidence/economic-rejected/seeded turn、およびblocker 4組以上でsurvey対象外だったtarget
- attempt、missing destination、repair/validation失敗、feasible、screen Reject、adopt
- 元blocker数1/2/3別feasible/adopt、最大mover数
- 選択helperのcovered blocker、unlock anchor、overlap、移動費、退去時刻差、adjusted gain
- blockerがhelper旧領域を使った件数、helperがblocker旧領域を使った件数、両方成立したbidirectional cross-use件数。これはblocker集合とhelperの相互利用であり、同一blockerとの厳密なcycleを意味しない
- helper destination anchor/candidate/beam node、helper repair phase CPU
- helper固有のfuture delta、screen margin、採用時arrival fee・移動費・direct gain

次を全caseで0とする。

    pushout_helper_turn_funnel_error
    pushout_helper_attempt_funnel_error
    pushout_helper_feasible_funnel_error
    pushout_helper_feasible_histogram_error
    pushout_helper_adopted_histogram_error
    pushout_helper_direct_identity_error
    pushout_helper_work_cap_error
    pushout_helper_disabled_error

既存Push-out、decomposition、grow-and-trim、sampled DLP、status/plan/source保存則も全て維持する。`pushout_helper_phase_cpu_ms`は追加repairだけを測り、prefix surveyを含むhelperの総時間差はTreatment/Controlの`timing_solver_cpu_ms`および`pushout_cpu_ms`のpaired差で判定する。

### A/B境界と事前判定基準

ControlとTreatmentはともにgrow-and-trim、sampled DLP、NoRegion Push-outを有効、Deadline Layerを無効とする。差はhelperだけである。

    Control:
      -DAHC069_DISABLE_DEADLINE_LAYER
      -DAHC069_DISABLE_PUSHOUT_HELPER

    Treatment:
      -DAHC069_DISABLE_DEADLINE_LAYER

Controlは前段Treatment `pahcer/json/result_20260802_174107.json`の合計6,515,194,836と全100 seedのscore/stdout一致を互換性gateとする。100/100 AC、全保存則0、決定性一致をhard gateにする。主判定は絶対score合計+0.20%以上、seed幾何平均比>1、paired bootstrap 95%下限>=0.998、p05>=0.99、worst>=0.95、高R層合計比>=0.99、solver内部CPUの平均/p95増加10%以内とする。helper-supported arrival feeが全移動費の1.5倍以上か、NoRegion棄却理想料金の減少がshape/movement悪化を上回るかも確認する。

設定名は`pahcer/bench_pushout_helper_v15.toml`と`pahcer/bench_pushout_helper_v15_disabled.toml`、seed 0〜99、threads=1とする。静的監査とbinary hashを固定してからsanitizer、Control互換、Treatment 100 seed、決定性probeを一つの最終batchとして行う。最初の解答実行後はAGENTS.mdに従い、結果にかかわらずsource・方針・診断・config・memoを変更しない。

### 実行前固定物

- `main.cpp` SHA-256: `9ccdefee591a70b6f707f5099ef6c5ec70e726feb8d21a1ac33a11e3fbb28959`
- Treatment binary: `/private/tmp/ahc069_pushout_helper_v15`
- Treatment binary SHA-256: `c3332efe17c6675378707bac536e93a8dadaec16d417e64de1303f838ac0c330`
- Control binary: `/private/tmp/ahc069_pushout_helper_v15_disabled`
- Control binary SHA-256: `191f5d17a82421bbe9e7bcea4c187690e5ebebf0871be5191affb8856bceaa0a`
- sanitizer binary: `/private/tmp/ahc069_pushout_helper_v15_san`
- sanitizer binary SHA-256: `eed5330993de32552749e5d5f12fd8a178716b3fd3f78337a0e0b8424b65b2c1`
- Treatment config SHA-256: `521d5d68c667187da9ab6035a03d49da5604f9a662d577033d768292f53e955c`
- Control config SHA-256: `8e30240d87619ef22119341d312cc218faa2613f1f673de4cd3bfb4ac9819b3a`
- tester SHA-256: `3702067f731de62b30a395f99eee04a4a9e247a1f421e2c42556a9e45b62ec92`
- `pahcer/best_scores.json` SHA-256: `f7b224dd97e8df62cdb73eadb5609eb37417ee56f9dbf35e893895f37cc3f5fa`
- seed 0 / 99 SHA-256: `61857f9adeb56546a876f9f54eec3e95be617d4a1135d987ae790a2248304754` / `cfb3c1678be95ff9ff760cafed7a36c99a769c4c75c679b4a0bdef7d8edf2ad8`

Clang C++17 Treatment/Control、Clang C++20、GCC C++17の警告付きsyntax検査、release/sanitizer build、Clang Static Analyzer、`git diff --check`はpassし、警告・指摘0件。独立した実装・計算量・A/B isolation監査でもblocking issueは0件だった。O(1)単一owner判定は分散0の必要十分条件、helper経路は`B<=3`かつmover最大4、repair用work capは既存Push-outと分離され、Controlではsurvey/prefix/repairがcompile-timeに除外されることを確認した。ここまで固定した3 binaryはまだ一度も実行していない。

以降はsanitizer、Control互換、Treatment 100 seed、決定性probeを一つの最終batchとして実行する。最初の解答実行後は結果にかかわらずsource・方針・診断・config・memoを変更せず、読み取りと結果報告だけを行う。

## 2026-08-02: one-helper Push-out v15の結果

前節で固定したbinaryをseed 0〜99、threads=1で実行した。Controlはhelper無効、Treatmentはone-helper有効であり、それ以外はgrow-and-trim、sampled DLP、NoRegion Push-outを有効、Deadline Layerを無効に揃えた。

| 指標 | Control | one-helper v15 |
| --- | ---: | ---: |
| 絶対スコア合計 | 6,515,194,836 | 6,517,375,675 |
| relative score合計 | 10,141.642297 | 10,145.170193 |
| 差 | - | +2,180,839 (+0.03347%) |
| 勝ち / 引き分け / 負け | - | 2 / 98 / 0 |
| WA | 0 | 0 |

結果ファイルはControlが`pahcer/json/result_20260802_181616.json`、Treatmentが`pahcer/json/result_20260802_181836.json`である。Controlは前段sampled DLP結果と100 seedのscoreおよびstdoutが完全一致した。Treatmentも全保存則0、seed 0の決定性一致、100/100 ACだった。

helperは10,064 turnで検討され、6,127 turnに候補seedがあった。11,592 attemptの内訳はmissing destination 10,047、repair failure 1,541、完成4で、完成後はscreen Reject 2、採用2だった。完成4件は全てblockerがhelper旧領域を利用した一方、helperがblocker旧領域を利用した件数は0だった。したがって因果的helperの選択自体は正しいが、全moverが同じ既存空き領域を優先するdestination poolがほぼ全ての同時repairを失敗させていた。

solver内部CPUは平均1,042.217ms、最大1,535.331msで、helper第2段階のCPUは平均11.726ms、最大30.102msだった。改善は非負だが+0.20%の事前採用基準には届かず、one-helperという考えを棄却するのではなく、候補幅と同時交換向けdestination生成を見直す。

## 2026-08-02: wide one-helper Push-out v16（実行前固定）

ユーザーの「AHCでは安全思考になりすぎず、失敗したら戻せばよいので少しリスクを取って広くする」という指示を受け、v15のblockers-only保護経路とone-helperの因果条件を維持したまま、helper第2段階を明確に広げた。変更はNoRegionでblockers-only候補が0件だった場合だけに限定し、通常配置、admission、sampled DLP、grow-and-trim、compact rescue、既存Push-outは変えていない。

### 探索幅

| work cap | v15 narrow | v16 wide |
| --- | ---: | ---: |
| helper候補 / target | 1 | 3 |
| repair attempt / turn | 2 | 6 |
| destination anchor / mover | 1,024 | 2,048 |
| destination anchor / turn | 4,096 | 16,384 |
| legal destination / mover | 24 | 48 |
| retained destination / mover | 6 | 12 |
| repair beam node / turn | 256 | 1,024 |
| 完成候補 / turn | 1 | 2 |

完成候補を最初の1件で止めず最大2件まで保持し、既存と同じReject対Q2/H4 common-random-number rolloutで比較する。one-helper、元blocker最大3組、総mover最大4組、全moverのfee-neutral、strict sampled-DLP gateは維持する。今回は探索幅の不足を検証する段階であり、移動費や料金悪化の許容までは同時に変更しない。

### 同時交換向けdestination pool

v15は`fallback overlap -> cleared overlap -> temporal cost`を主に使ったため、複数moverが同じ「以前から空いていた領域」を選びやすかった。v16のhelper repairだけは次を混ぜたpoolへ変更した。

1. 他moverの旧領域と重なる候補を最優先で保持する。自分自身の旧領域はforeign overlapから除く。
2. v15順位の上位候補も残し、交換を強制しすぎない。
3. 盤面を3x3 sectorに分け、各sectorの代表を保持して候補の位置を分散する。
4. beam rankもforeign overlapを最優先し、その後fallback overlap、cleared overlap、temporal cost、周長で比較する。

さらにnear-miss survey時に「このblocker候補はhelperだけをclearすれば合法になる」という最初の厳密な因果witnessをblockerごとに保存する。helper repairでは、サイズ、現在盤面での空き、非no-op、fee-neutralを再検証したwitnessをそのblockerのpool先頭へ強制保持する。最終的には従来validatorが連結性、pond、重複、周長、料金悪化0、移動費、direct gainを再検査するため、witnessは合法性を迂回しない。

`AHC069_DISABLE_WIDE_PUSHOUT_HELPER`を付けると上限と候補順位をv15へ戻す。同一sourceのnarrowを保存済みv15 Treatmentの全100 seed score/stdoutと比較し、wide以外の挙動が変わっていないことをhard gateにする。wideではmissing destinationをhelper側/blocker側へ分解し、recorded/forced witness、foreign候補、2完成候補turnも追加記録する。

実行前の最終監査で、旧案3の定数・強制ENABLEブロックと未使用診断7フィールドが外部混入していたことを検出した。凍結済み`/private/tmp/ahc069_main_before_deadline_only.cpp`と照合してv15のdeadline定数・compile-time feature flag・`DeadlineLayerDiagnostics`へ完全復元し、wide helper差分だけを残した。旧案3由来symbolは残っていない。

### 実行前固定物

- `main.cpp` SHA-256: `a46f239bdcdf4d48bfa49aebf6de9cfa7837909ffa47071fc62f189d89f24cc2`
- wide binary: `/private/tmp/ahc069_pushout_helper_wide_v16`
- wide binary SHA-256: `fd4a7fc3fbe38567204b26f4f93855e3f5b0142612ce883cda09915f5f86611c`
- narrow binary: `/private/tmp/ahc069_pushout_helper_wide_v16_narrow`
- narrow binary SHA-256: `fe763fa8a59f19f460867af9183c8189fa93ebc7fe40d565318a05868878dedf`
- sanitizer binary: `/private/tmp/ahc069_pushout_helper_wide_v16_san`
- sanitizer binary SHA-256: `fe294f5c4fdd6ed0a4ca348b386a03ac81d5e3034c041bd59da43b2711428d4f`
- wide config: `pahcer/bench_pushout_helper_wide_v16.toml`
- wide config SHA-256: `263404bfadd12f788e1cbf57235832896968cb403315ca5706571c89ae1781f8`
- narrow config: `pahcer/bench_pushout_helper_wide_v16_narrow.toml`
- narrow config SHA-256: `1b33119de4b4e6c1c3b4d3f1f28083a4ca5b33ab6957d8b603ebbfe5a2deca37`
- tester SHA-256: `3702067f731de62b30a395f99eee04a4a9e247a1f421e2c42556a9e45b62ec92`
- `pahcer/best_scores.json` SHA-256: `f7b224dd97e8df62cdb73eadb5609eb37417ee56f9dbf35e893895f37cc3f5fa`
- seed 0 / 99 SHA-256: `61857f9adeb56546a876f9f54eec3e95be617d4a1135d987ae790a2248304754` / `cfb3c1678be95ff9ff760cafed7a36c99a769c4c75c679b4a0bdef7d8edf2ad8`

Clang C++17 wide/narrow、Clang C++20、GCC C++17の警告付きsyntax検査、release/sanitizer build、Clang Static Analyzer、`git diff --check`はpassし、標準警告0件。独立した実装、合法性、保存則、A/B isolation監査でもblocking issue 0を確認した。現行定数では完成候補2件はrollout候補上限2件以下、root action配列境界内であることを`static_assert`でも固定した。

ここまで固定した3 binaryはまだ一度も実行していない。以降はsanitizer、narrowのv15完全互換、wide 100 seed、決定性probeを一つのbatchとして実行する。最初の解答実行後はsource、方針、診断、config、memo、binaryを変更せず、結果を読み取り報告してユーザーの次の明示指示を待つ。

## 2026-08-02: wide one-helper Push-out v16の結果とhelper段階の廃止

前節で固定したbinaryをseed 0〜99、threads=1で実行した。narrowは`AHC069_DISABLE_WIDE_PUSHOUT_HELPER`付きのv15相当、wideは拡張有効であり、それ以外はgrow-and-trim、sampled DLP、NoRegion Push-outを有効、Deadline Layerを無効に揃えた。

| 指標 | v15 narrow | v16 wide |
| --- | ---: | ---: |
| 絶対スコア合計 | 6,517,375,675 | 6,518,111,537 |
| 差 | - | +735,862 (+0.011291%) |
| 勝ち / 引き分け / 負け | - | 5 / 89 / 6 |
| helper attempt | 11,592 | 24,697 |
| helper feasible plan | 4 | 27 |
| helper adopted | 2 | 13 |
| solver内部CPU平均 | 1,043.636ms | 1,106.266ms |
| solver内部CPU p95 | 1,354.496ms | 1,538.934ms |

結果ファイルはnarrowが`pahcer/json/result_20260802_190202.json`、wideが`pahcer/json/result_20260802_190407.json`である。narrowは保存済みv15 Treatment `result_20260802_181836.json`と全100 seedのscore・stdout・主要helper funnelが完全一致したため、A/B差はwide部分だけに隔離できた。pahcer実行時に`best_scores.json`が更新されたため画面上のrelative score同士は直接比較せず、実行前の固定基準へ換算したwide relative scoreは10,146.345421、narrowは10,145.170193で、差は+1.175227だった。

seed比の幾何平均は100.010455%。paired bootstrapの合計比95%区間は99.9430%〜100.0874%、改善確率は約61.1%で、正方向だが統計的には誤差を抜けていない。最大改善はseed 66の+1,618,391、次点はseed 18の+966,431、最大悪化はseed 72の-1,227,803。seed 66を除くと全体差は-882,529になる。

### 探索拡張が機能した証拠

- causal witness記録147,595、foreign候補311,338、foreign保持158,797、witness強制挿入14,745。
- feasible 27件すべてでblockerがhelper旧領域を利用した。
- helperもblocker旧領域を利用する双方向交換はfeasible 7件、adopt 4件まで発生した。
- 13 turnで2候補まで完成し、幅2の経路が実際に使われた。
- turn、attempt、missing partition、feasible、histogram、direct identity、work capを含む全58種類のerror/identity fieldは両版・全100 caseで0。
- sanitizer seed 0/69は異常0。最大改善seed 66のrelease再実行は保存済みstdoutと完全一致した。

attempt funnelはnarrowが`11,592 = missing 10,047 + repair failure 1,541 + feasible 4`、wideが`24,697 = missing 20,344 + repair failure 4,326 + feasible 27`だった。wideでも全試行の82.38%がdestination不足で、helper destination不足だけで64.74%。全moverのdestinationが揃った4,353 attemptのうち4,326件、99.38%が同時repairに失敗した。node cap到達は0であり、単なる探索量ではなく候補間の非重複組合せを構成できないことが残るボトルネックである。

### スコア分解

wide - narrowは次の恒等式で+735,862になる。

    accepted ideal fee            -2,552,911
    initial shape lossの減少       +3,411,198
    movement costの増加             -122,425
    relocation fee loss                    0
    -----------------------------------------
    absolute score                  +735,862

追加採用されたhelper行動のdirect gainはnarrow比+4,178,575だったが、最終スコア差として残ったのは17.61%だけだった。wideは形状を改善する一方、その後に受け入れる組の集合を悪化させており、短いQ2/H4 rolloutの正marginだけでは長期影響を十分に識別できていない。

### 判断

探索幅不足という仮説は、feasible 4→27、adopt 2→13によって十分検証できた。しかしhelperなしv14から見たone-helper段階全体の改善は+2,916,701、+0.04477%に留まり、複雑な専用実装とCPU増加に見合わない。v15は2/100 seedだけ、v16追加分も11/100 seedだけに作用し、v16は5勝6敗だった。これ以上work capや候補数を調整する期待値は低い。

ユーザーとの合意により、one-helper v15/v16はmainlineから廃止し、次の基準をhelper導入前のv14 `grow-and-trim + sampled DLP + 既存NoRegion Push-out`へ戻す。今回の実験は失敗として消去せず、この節、結果JSON、出力、実装をGit履歴へ保存する。将来再配置へ再挑戦する場合は、blockerへhelperを1組足す拡張ではなく、複数moverの領域を最初から同時に割り当てる別設計として扱う。

## 2026-08-02: one-helper段階の削除とv14基準への復元（実行前固定）

one-helper v15/v16の実装と結果はcommit `8ae2217`（`AHC069: archive staged policy experiments`）へ保存した。その上で、mainlineからhelper専用の定数・feature flag・obstruction survey・causal witness・候補選択・joint destination pool・第2 repair phase・診断・保存則・stderr項をすべて削除した。`main.cpp`内の`helper`、`foreign_cleared_overlap`、`sector`、`movers`等の関連symbolは0件になった。

既存のblockers-only経路は、凍結済みhelper-free参照`/private/tmp/ahc069_main_before_deadline_only.cpp`と静的に照合した。RescueTargetとPushOutDiagnosticScopeは一致し、destination生成、beam repair、root rescueは整形と後から導入したsampled DLP引数・呼出し以外一致した。したがってtarget順、合法性、destination順位と4象限diversity、blocker repair、rollout候補比較、採用時の集計はhelper導入前へ戻っている。独立監査でも意図しない差は0件だった。

提出用source単体で構成が確定するよう、Deadline Layerはマクロではなく`ENABLE_DEADLINE_LAYER = false`へ固定した。NoRegion Push-out、grow-and-trim、sampled DLPは既定で有効である。Phiとcrisisは未導入のまま。

### 実行前固定物

- archive commit: `8ae2217`
- `main.cpp` SHA-256: `6f763500e0d5c0c851ce26971be88129f87666d9659d6463ded4f9f29280e997`
- Clang release binary: `/private/tmp/ahc069_v14_restored`
- Clang release binary SHA-256: `67bd21ad811c788b5f4bc88b4d36a4a83d35b6e02a8c151cb217dcb04c7cab2e`
- sanitizer binary: `/private/tmp/ahc069_v14_restored_san`
- sanitizer binary SHA-256: `0f8c12f1121185ec0ab12ee14bc8247c15887514388b0e1215e28861c5fad66b`
- GCC release binary: `/private/tmp/ahc069_v14_restored_gcc`
- GCC release binary SHA-256: `03c8e321515e92ad72959815bb32d5f7c487ed5dc5b10fac4a163b5c2e3d88a3`
- config: `pahcer/bench_v14_restored.toml`
- config SHA-256: `d2b26311bab5548a63d3a9df1a4bfb9fda2e2041b9f572e6107ba6330e1f1880`
- v14 oracle result: `pahcer/json/result_20260802_174107.json`
- v14 oracle absolute score: `6,515,194,836`（100/100 AC）
- v14 oracle stdout: `tools/out-sampled-dlp-v14/{0000..0099}.txt`

Clang C++17/C++20とGCC C++17の警告付きsyntax検査、3 binaryのbuild、Clang Static Analyzer、`git diff --check`はすべてpassし、標準警告・指摘0件。hard gateは全100 seedのscore一致、stdout byte一致、100/100 AC、共通するnon-timing診断値の一致、全error/identity field 0、ログ上`deadline=0 / pushout=1 / grow-and-trim=1 / sampled-DLP=1`とする。

ここまで固定した復元binaryはまだ一度も実行していない。以降はsanitizer probe、100 seed互換検証、seed 0決定性probeを行い、最初の解答実行後はユーザーの次の明示指示までsource・config・memoを変更しない。

