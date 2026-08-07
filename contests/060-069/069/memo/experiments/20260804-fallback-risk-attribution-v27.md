# connected fallback attribution v27

## 仮説

v26 Gateはrepacking後connected 19,442件のうち99件だけを方策Rejectし、Control比`+0.151501%`、shape loss`-21.15M`、fragmentation ideal fee`-14.42M`だった。ただしbootstrap区間は0をまたぎ、Rejectの時期・source別内訳を固定診断へ残していなかった。

今回の主仮説は、細切れ空間を使うconnected受入の悪影響が最終四半期ではなく、後続到着が十分残る序盤・中盤に集中することである。4進行windowの0〜2だけGateを許可するEarlyMidと、window 3だけ許可するLateを分ける。

Control / GateAll / EarlyMid / Lateのseed別scoreを`C,A,E,L`とする。主比較は`E-C`。ユーザー仮説と整合する方向を、実行前に次で固定する。

```text
E-C > 0
L-C <= 0
E-L > 0
A-E < 0
```

4本のtotal符号が全て上記と一致した場合だけ、主仮説を`directionally supported`とする。`E-C<=0`なら`primary contradicted`、`E-C>0`でも残る整合条件が一つでも外れれば`mixed/inconclusive`とする。bootstrap CIは不確実性の表示に使うが、このラベルを結果後に緩めず、未見検証前の採用はどのラベルでも行わない。

補助的に時期非加法性`I_time = A-E-L+C`も同一seed内で測る。early/mid Rejectが後続盤面とlateの候補機会を変えるため、`A-E`や`A-L`を純粋な1イベント直接効果とは呼ばない。

また最終sourceをraw BFS、multi-start connected growth、GrowAndTrimへ分け、どの生成器だけに高価なscreen予算を集中する方策が有望かを探索する。source限定armはGateAllのsource別加法分解ではない。他sourceがscreen枠を使わないため、GateAllではbudget skipされた後続の同sourceを追加screenし得る独立方策である。

## baselineと変更範囲

v26 sourceを基準に、placement、admission、repacking、risk式、screen Q2/H4、holdout Q8/H12、各window最大2 screen、risk専用confirmation最大4は変更しない。

変更は次の三点だけである。

1. `risk>=1`かつfiniteなpost-repacking connected候補を、screen budget消費前にcompile-time study modeで時期または最終sourceにより除外できるようにする。
2. filterを通った候補について、`window(4) × 最終source(3) × risk帯(8)`ごとにeligible、screen budget skip、screen attempt、screen Reject勝利、最終方策Rejectを診断出力する。
3. 最終方策Reject最大4件について、turn、window、source、盤面・候補hash、料金、`U0/U1/lambda/damage/risk`、screen・holdout marginを診断出力する。

mode未指定のGateAllではfilterが常にtrueであり、v26 Gateとbudget消費、rollout、confirmation、出力行動が同じでなければならない。無フラグではrisk測定・比較を行わず、通常baselineを保つ。

限定armで除外した候補はscenarioを生成せず、`screens_used`もrisk confirmationも消費しない。filterはbaseline sourceではなく、既存root/repacking後の最終sourceへ適用する。`PlacementSource::ConnectedGrowth`を本文ではmulti-startと呼ぶ。

## 実行前に固定した仕様・停止条件

### 比較arm

| arm | compile flag | Rejectをscreenできる層 |
|---|---|---|
| Control | なし | なし |
| GateAll | `AHC069_ENABLE_FALLBACK_RISK_GATE` | 全window・全connected source |
| EarlyMid | Gate + `AHC069_FALLBACK_RISK_EARLY_MID_ONLY` | window 0〜2 |
| Late | Gate + `AHC069_FALLBACK_RISK_LATE_ONLY` | window 3 |
| Bfs | Gate + `AHC069_FALLBACK_RISK_BFS_ONLY` | raw BFS |
| Multi | Gate + `AHC069_FALLBACK_RISK_MULTI_START_ONLY` | multi-start |
| GrowTrim | Gate + `AHC069_FALLBACK_RISK_GROW_AND_TRIM_ONLY` | GrowAndTrim |

全armで同じseed 0..99を1回、`threads=1`で実行する。GateAllはv26 Gateの再現確認であり、同じ100 seed上の新しい統計的証拠とは数えない。5限定armはv26結果を見た後の探索なので、正になっても通常提出へ直接採用せず、別の未見集合または新しい明示指示による独立検証を必要とする。

### 比較と統計

- 主比較: `EarlyMid-Control`。
- 時期整合性: `Late-Control`、`EarlyMid-Late`、`GateAll-EarlyMid`、`I_time`。
- source副次探索: `Bfs-Control`、`Multi-Control`、`GrowTrim-Control`。
- 全比較でtotal、Control比、勝/分/負、changed seed、中央値、p05/p25/p75/p95、top gain/lossを出す。changed seedはpaired raw score差が0でないseedと定義する。
- seed indexを7 armまとめて共同再標本化するjoint paired bootstrapを100,000回、乱数seed`270804`で行い、各replicateのpaired total差と`I_time`からpercentile 95% CIを出す。
- 多数armからwinnerを選んだ未調整CIは採用根拠にしない。限定armは方向・効果量・損失分解・Reject母数を合わせた探索資料とする。

### risk帯とイベント帰属

risk帯は実行前に次の8半開区間へ固定する。

```text
[0,0.5), [0.5,1), [1,2), [2,4), [4,8), [8,16), [16,32), [32,+inf)
```

high-riskかつstudy filterを通った候補は、`window 0..3 × bfs/multi_start/grow_and_trim × risk band 0..7`の96 bucketへ一意に入れる。非空bucketごとに、`eligible / screen_budget_skips / screen_attempts / screen_reject_wins / final_rejections`を出す。これにより周辺分布だけでは失われる時期・source・Rの構成差と、screen対象母数に対するReject率を保持する。限定armのbucketは対象層だけを表し、filterで除外した層は含めない。

最終Rejectイベントには、turn/arrival id、window、screen/confirmation slot、最終source、remaining、`S,T,P,V`、最小/候補周長、free cells、region/owner-before hash、ideal/candidate fee、opportunity cost、`U0,U1,lambda,damage,risk`、2 screen future delta、screen/holdout marginを残す。stderr上の対応名は`U0=future_fit_without`、`U1=future_fit_with`、`lambda=expected_overlap`、`d=damage`、`R=risk`である。式の内部identityは丸め前の`long double`で検査し、イベント行には各値とrisk bandを併記する。

Reject action署名は`(turn, arrival_id, owner_before_hash, region_hash, source, candidate_perimeter, candidate_fee)`の7要素に固定する。screen/confirmation slot、risk、marginは照合情報であり署名へ含めない。

arm内Rejectがちょうど1回で、Reject turn直前までControlと出力prefixが一致し、Controlがそのturnに出した到着regionのhash・周長・料金もイベント候補と一致するseedに限り、そのseedのpaired score差を「その1回のRejectと全後続cascadeの総効果」と対応させる。これはReject回数というpost-treatment条件で選んだ記述統計であり、ATEや採用根拠にはしない。複数Reject seedのscore差は個別イベントへ加法配賦しない。

### hard check

次のいずれかに違反した実験は無効とする。

- 全arm 100/100 AC、合法性・保存則・finite・funnel errorが全て0。
- 新Controlが既存oracleおよびv26 Controlとscore/stdout 100/100 byte一致。
- GateAllがv26 Gateとscore/stdout 100/100 byte一致する。core funnelはv26に存在した`fallback_risk_baseline_*`、final source、below/eligible/window、screen outcome、confirmation outcome、final rejection、nonfinite、risk/fee/margin/policy-step集計、およびroot confirmationのusage/approval counterを指し、新規study/event counterとtimingだけを比較対象外とする。
- 各限定armで最終Reject 0回のseedはControl stdoutとbyte一致。
- 最初のstdout差分はそのarmの最初のReject turnにあり、それ以前のturn prefixはControlと一致。
- 同じseedでordered Reject署名列が同じ2 armはstdout全体も一致。
- EarlyMidはwindow 3、Lateはwindow 0〜2、source armは対象外sourceでscreen/confirmation/final Rejectを行わない。
- raw high-riskが`study_filtered + eligible`、eligibleが`screen budget skip + attempt`へ厳密に分割され、除外候補がscreen/confirmation予算を消費しない。raw high-riskはfiniteなpost-repacking最終connectedかつ`risk>=1`を指し、baseline high counterとは区別する。
- 96 bucketのeligible、budget skip、screen attempt、screen Reject、最終Rejectの各総和が対応する全体値と一致する。各bucketでも`eligible = budget skip + screen attempt`、`screen Reject <= screen attempt`、`final Reject <= screen Reject`を満たす。
- GateAllのfilter countは0、reject event overflowは0。実装済みの`fallback_risk_*partition_error`、window/source/band error、study mode violation、event count/fee/validity/formula errorが全て0。

EarlyMidは最大6 screen/case、Lateは最大2、source armは対象sourceに各window枠を専有させるため、raw totalとscreen・Reject件数、screen/confirmation policy steps、CPUを併記する。score差をscreen数やReject数へ割った値は因果効果・採否・arm比較に使わない。confirmation budget skipが1件でもあれば、そのarmは対象限定に加えてcap打切りを含む複合方策として解釈し、source固有性の根拠にしない。

性能はPahcer wallに加え、コード内`timing_solver_cpu_ms`の平均・p95・最大・1800ms超件数を正本にする。`fallback_risk_cpu_ms`は依然としてplacement内`U0/U1`計算を含まないので補助値とする。通常提出候補に昇格できるCPU条件はControl比平均/p95`+15%`以下、最大1900ms未満、1800ms超件数が増えないことだが、今回の限定armは未見検証前には昇格しない。

## source / binary / config / input / oracle

最初の解答実行前に、次を固定した。release buildはApple clang 17.0.0、C++20、`-O2 -DNDEBUG -Wall -Wextra -Wshadow -Wpedantic`で作成した。

| artifact | SHA-256 |
|---|---|
| `main.cpp` | `d576c6cee56fe9d2a94d7165d413ec1fafcccc255b57666a0bf147f209702b0c` |
| `tester` | `3702067f731de62b30a395f99eee04a4a9e247a1f421e2c42556a9e45b62ec92` |
| Control binary | `90b422d277bdf4f471e900bd7706510a63a80f0b829eff1eae579503d6e585e5` |
| GateAll binary | `d7073075b323b71a1db2904554dc2f61fd30a5b211ad2513bbec395e4d15697d` |
| EarlyMid binary | `14af7850ae56091764a2971d62d0969a1065ba4e351f12ba81f99c172a20c009` |
| Late binary | `59d5f9dfb878fa81f0ef4893e5f0b6efb65bbd321621f52391d2557b34c56512` |
| Bfs binary | `c8d235e8023a4f620178672071ca6ecfe3bf705980de2577c92fba67f1a704e5` |
| Multi binary | `7cd1c640015fe6bccd5010e869f61063d58468ff9817507c60e7dd62ac2bd496` |
| GrowTrim binary | `1c7f7547d85c9b4af75ef02a7613032defdfca67a4157901e114a4fd49e9a1ff` |

binary pathは順に`/private/tmp/ahc069_fallback_risk_v27_{control,all,early_mid,late,bfs,multi,grow_trim}`である。

| config | SHA-256 |
|---|---|
| `bench_fallback_risk_v27_control.toml` | `5cac74fa20071f9d97c27075590740497896d801dbe1426a894fd2b690959f57` |
| `bench_fallback_risk_v27_all.toml` | `11db1a7a4c6c4230bf744e685269d43f2b9fb32cf7163d3c83fd298e6dbc4b6c` |
| `bench_fallback_risk_v27_early_mid.toml` | `c1a393f550851c3eeef8cc38d3aaec6b3dcbbc017fd6429cf8bffc4c23541b10` |
| `bench_fallback_risk_v27_late.toml` | `a1aac177b96715b2b11926b070df14b2e7ff2683c3b81642c2d0f40792bccf59` |
| `bench_fallback_risk_v27_bfs.toml` | `68d825a30f568cdde95a195ef5c4350650cf01d352c02527f581c2bc7a641cb1` |
| `bench_fallback_risk_v27_multi.toml` | `816cf2130aac493e721bcb77a7b086aae0318842dc2a9a7c1daebd9abd3f94dc` |
| `bench_fallback_risk_v27_grow_trim.toml` | `1035bfd1d983a83b1b71353d5ac6408036aacb6659887019c7b40ea3465ed758` |

全configはseed 0..99、`threads=1`、上記固定binary、arm固有のstdout/stderr directoryを指す。Pahcerは0.3.1である。

入力は`tools/in/0000.txt`から`0099.txt`の100件。seed順の各file content SHA-256だけを連結して再hashしたdigestは`49bf804d96c713381cc60955fa7b46e45674b0e456e3a25ba238ca7bbb3cefb8`。

| oracle | JSON SHA-256 | stdout 100件のcombined SHA-256 |
|---|---|---|
| Control primary `result_20260803_003818.json` / `out-wide-stp-v25-default` | `9b064a2c0670a2df7dc2ea153ab50fa323a178e05a4e578318773e74c645c0f3` | `959a9fefc8ab44c03a143345f3b830ab052f9b9968b9e2e878d328ff23be61ad` |
| v26 Control `result_20260803_234657.json` / `out-fallback-phase-v26-control` | `2e1e381c7e2080c97b9ee5e621e47fc9cc5acaa86faae7231935bec382e4497a` | `959a9fefc8ab44c03a143345f3b830ab052f9b9968b9e2e878d328ff23be61ad` |
| v26 Gate `result_20260803_235042.json` / `out-fallback-phase-v26-gate` | `ddd508be0752add0a799241d9c470b614d9f6c527307fc7850b1a29ddabada55` | `13c1bbbcceecef6cb90d65ccac1662adc704230fe2d01c29e26fd38d0c0213dd` |

## 静的検証

2026-08-04、Control / GateAll / EarlyMid / Late / Bfs / Multi / GrowTrimの7 modeをApple Clang C++20、`-O2 -DNDEBUG -Wall -Wextra -Wshadow -Wpedantic -fsyntax-only`で検査し、全て警告0・終了code 0だった。同じ7 modeのC++17構文検査も警告0で通過した。EarlyMidのClang Static Analyzer、GateAllのASan/UBSan buildも通過し、sanitizer binaryは実行していない。Gateなしstudy modeと複数study modeは意図どおりcompile errorになった。

`git diff --check`、TOML parse、100入力、固定binary/config/oracle path、全artifact hashを確認した。配置方策、admission、repacking、risk式、screen/confirmation条件には変更を加えず、追加した96 bucketは既存scalarと同じ分岐上で診断counterだけを更新することを静的確認した。この時点で解答プログラムは未実行である。

## 実行結果

2026-08-04、固定した7 binaryをControl、GateAll、EarlyMid、Late、Bfs、Multi、GrowTrimの順に実行した。全arm 100/100 AC。解析器`tools/analyze_fallback_risk_v27.py`（SHA-256 `140e83172e500711f0ef3ea477a9424e8ac8ba2092f3bdb5704778378a33272a`）で1,188,482 checksを行い、score・stdout・可変長turn prefix・盤面/region hash・料金・Reject署名・全funnel/identityを含めてerror 0だった。

新Controlは既存2 Control oracleとscore・stdoutが100/100 byte一致した。GateAllもv26 Gateとscore・stdout・旧core診断が100/100一致した。限定armのzero-Reject seedは全てControlとbyte一致し、最初の差分turn、Reject署名、mode外screen禁止も全件通過した。

| arm | total score | Control差 | W/T/L | 最終Reject | solver CPU mean / p95 / max | CPU判定 |
|---|---:|---:|---:|---:|---:|---|
| Control | 6,515,194,836 | 0 | - | 0 | 1003.557 / 1299.115 / 1633.071ms | 基準 |
| GateAll | 6,525,065,419 | +9,870,583 (+0.151501%) | 34/38/28 | 99 | 1240.571 / 1649.682 / 1870.901ms | mean +23.62%、p95 +26.99%で不可 |
| EarlyMid | 6,521,854,185 | +6,659,349 (+0.102212%) | 27/49/24 | 69 | 1193.975 / 1653.355 / 1855.867ms | mean +18.97%、p95 +27.27%で不可 |
| Late | 6,514,149,285 | -1,045,551 (-0.016048%) | 15/68/17 | 36 | 1087.610 / 1503.707 / 1650.006ms | p95 +15.75%で不可 |
| Bfs | 6,512,359,878 | -2,834,958 (-0.043513%) | 11/74/15 | 32 | 1092.809 / 1503.307 / 1742.883ms | score負、p95 +15.72% |
| Multi | 6,508,958,732 | -6,236,104 (-0.095717%) | 29/36/35 | 103 | 1258.079 / 1635.447 / 2393.715ms | score負、CPU超過 |
| GrowTrim | 6,508,231,475 | -6,963,361 (-0.106879%) | 27/31/42 | 110 | 1246.593 / 1678.120 / 1806.042ms | score負、CPU超過 |

Pahcer wallでもGateAll / EarlyMid / Multi / GrowTrimは重い。特にMultiはwall最大3348.567ms、solver CPU最大2393.715msだった。どの限定armも事前固定したCPU昇格条件を満たさない。

### 結果artifact

combined digestはseed順の100ファイルについて各file content SHA-256だけを連結して再hashした値であり、directory名を含まない。このためControlと旧Control、GateAllと旧Gateのstdout digestが一致する。

| arm | result JSON / SHA-256 | stdout combined | stderr combined |
|---|---|---|---|
| Control | `result_20260804_011444.json` / `343640f1b2bd06538d3f0264c55c908ace01a62e31f191a9bb0fa7027c96882f` | `959a9fefc8ab44c03a143345f3b830ab052f9b9968b9e2e878d328ff23be61ad` | `f3f404e3b216cfcda33c557f3c04cbaa0706d02ac88d92077b6a0c1c91652b13` |
| GateAll | `result_20260804_011653.json` / `90fb9ca1884e5778e0bace182fa4992effac76a21d66be503c998f8d5b78218f` | `13c1bbbcceecef6cb90d65ccac1662adc704230fe2d01c29e26fd38d0c0213dd` | `bb0bf7230046b46cd26b80272b6a393f3df70689fa3ba0c4635328d41b02327a` |
| EarlyMid | `result_20260804_011927.json` / `8928a55192194c45bc696f122dcb031505aff014b6b05260249462f0460ab342` | `6d1c200efa5146db9847812b857547d13aca315e83afa1af735a13dbe153a1d3` | `52c463948bbbc1993a868be05c5d9225a4848daa7887780a0b419b89c58124ef` |
| Late | `result_20260804_012150.json` / `aa9bf653a8ca84794fef111d383961a9dd2f088931f0ab012da5ec42158357f2` | `f7b1b98f266d1a69693d4dc22b1386f7ed2518bbad6171a86fe499317299065d` | `d87b525d4886c82bd6fbab3be31e97d4802acafec5f6567cd9f5ca0ddc816bd6` |
| Bfs | `result_20260804_012400.json` / `012ea0e9ee3431934d48a760deeb261248280234f447c7808799f6f3656a3b88` | `c6025f042b7f6da9bfee3f3db24225dd6751f870be2c42979f02cf93ade5d852` | `22af58138e939e00c20a11a4cf62d1b53ccabd35968de3ac0c26ab5373e65277` |
| Multi | `result_20260804_012612.json` / `ca627939ee76693176c8f80cbf0160fd955b8e46df8473479efb325c8fe20d96` | `bd603a94d9d1e4a36f393ff6d828141e3eb3192d07a8d756581751ae040530bf` | `a5cb159fad8d5a3913ece12759462198fca3ac31f02ab0cf1b7f39467b8fe2c9` |
| GrowTrim | `result_20260804_012839.json` / `c72a1e095571488a1e33ddc7b021d39208fbd0c8c3c38e66c55bee6fc739000c` | `68a6f11f11ee01b831cc91f613649129b0241a2763950f5e845c48b318d4b663` | `4e690a2b8688cc3de3a3ea1ee60444a95d086d928d3e6d2f01b2ba4784743ec0` |

## paired比較と損失分解

同じseed indexを共同再標本化した100,000回のpaired bootstrap、seed `270804`の結果は次の通り。全区間が0をまたぐ。

| contrast | total差 | Control比 | W/T/L | changed | bootstrap 95% CI |
|---|---:|---:|---:|---:|---:|
| GateAll-Control | +9,870,583 | +0.151501% | 34/38/28 | 62 | [-6,689,381, +26,599,233] |
| EarlyMid-Control | +6,659,349 | +0.102212% | 27/49/24 | 51 | [-10,581,835, +24,041,347] |
| Late-Control | -1,045,551 | -0.016048% | 15/68/17 | 32 | [-7,968,061, +6,000,617] |
| EarlyMid-Late | +7,704,900 | +0.118260% | 35/38/27 | 62 | [-9,105,758, +24,620,724] |
| GateAll-EarlyMid | +3,211,234 | +0.049289% | 15/72/13 | 28 | [-2,067,899, +8,962,908] |
| GateAll-Late | +10,916,134 | +0.167549% | 30/49/21 | 51 | [-4,595,551, +26,433,386] |
| `I_time=A-E-L+C` | +4,256,785 | +0.065336% | 16/71/13 | 29 | [-3,321,834, +12,144,532] |
| Bfs-Control | -2,834,958 | -0.043513% | 11/74/15 | 26 | [-10,904,485, +4,440,138] |
| Multi-Control | -6,236,104 | -0.095717% | 29/36/35 | 64 | [-21,582,269, +9,150,698] |
| GrowTrim-Control | -6,963,361 | -0.106879% | 27/31/42 | 69 | [-26,089,187, +12,170,098] |

時期の事前固定4条件のうち、`E-C>0`、`L-C<=0`、`E-L>0`の3本は一致した。しかし`A-E<0`は外れ、実際にはGateAllがEarlyMidを3.21M上回った。したがって固定labelは`mixed/inconclusive`である。正の`I_time`も、序中盤で作られた盤面上では終盤Rejectの働きがLate単独と変わる可能性を示すが、CIは0をまたぐため確証ではない。

GateAll-EarlyMidを損失分解すると、終盤Gate追加後のcascadeはaccepted ideal feeを4.75M多く残す一方、shape lossは1.48M悪化し、accepted initial feeで+3.28M、movement cost増を引いてscore +3.21Mだった。したがってGateAllが上回った理由は単純な空き形状改善ではなく、序中盤介入後の終盤で受理集合の価値が変わったことにある。

| arm-Control | accepted ideal fee | shape loss | accepted initial fee | movement cost | fragmentation ideal fee | raw score |
|---|---:|---:|---:|---:|---:|---:|
| GateAll | -11,078,482 | -21,145,410 | +10,066,928 | +196,345 | -14,421,898 | +9,870,583 |
| EarlyMid | -15,829,737 | -22,620,758 | +6,791,021 | +131,672 | -2,803,718 | +6,659,349 |
| Late | -2,349,193 | -1,384,343 | -964,850 | +80,701 | -7,134 | -1,045,551 |
| Bfs | -11,615,873 | -8,853,206 | -2,762,667 | +72,291 | +17,999,758 | -2,834,958 |
| Multi | -22,524,988 | -16,418,469 | -6,106,519 | +129,585 | +28,552,053 | -6,236,104 |
| GrowTrim | -24,988,563 | -18,211,520 | -6,777,043 | +186,318 | -9,422,129 | -6,963,361 |

負のshape loss差は改善、正のmovement cost差は悪化を表す。EarlyMidはshape lossを22.62M改善したが、受理集合のideal feeを15.83M失い、netは6.66Mに留まった。Lateはshape改善1.38Mより受理価値損失2.35Mが大きい。source限定3本もshapeは改善したが、将来を守る利益以上に受理価値を失っており、現在のReject判定をそのsourceへ集中することはできない。

## 時期・source・risk attribution

### 時期

| policy/window | eligible | screen | screen Reject勝利 | 最終Reject |
|---|---:|---:|---:|---:|
| GateAll window 0 | 2,057 | 198 | 24 | 16 |
| GateAll window 1 | 2,269 | 198 | 33 | 22 |
| GateAll window 2 | 2,486 | 197 | 46 | 31 |
| GateAll window 3 | 2,364 | 198 | 44 | 30 |
| EarlyMid total | 6,812 | 593 | 103 | 69 |
| Late total | 2,338 | 198 | 48 | 36 |

GateAllとEarlyMidはwindow 0〜2のReject署名69件が完全一致し、window 3開始前のstdout prefixも一致する。その共通状態からGateAllだけが終盤に追加した30 Rejectを含む全cascadeが`+3.21M`だった。Late単独の負方向とEarlyMidの正方向はユーザー仮説と整合する一方、「終盤Rejectを常に止めればよい」とは言えない。early interventionの有無でlateの候補集合・効果が変わる非加法性がある。

単独RejectかつControlと候補まで一致した記述集合では、EarlyMidのscore差合計はwindow 0が6件で-1.67M、window 1が12件で-1.99M、window 2が19件で+8.99Mだった。GateAllでもwindow 2が14件で+9.20Mと最も強い。これはReject回数で選択したpost-treatment記述統計でありATEではないが、もし次に時期を再分割するなら「序盤全体」よりwindow 2単独を優先して調べる根拠になる。

### sourceと共有screen予算

| source | GateAll eligible / screen / final Reject | source限定 screen / final Reject | source限定のControl差 |
|---|---:|---:|---:|
| raw BFS | 263 / 24 / 0 | 215 / 32 | -2,834,958 |
| multi-start | 4,256 / 379 / 50 | 790 / 103 | -6,236,104 |
| GrowAndTrim | 4,657 / 388 / 49 | 783 / 110 | -6,963,361 |

source限定armはGateAllの加法分解ではない。GateAllでは3 sourceが各window最大2枠を共有するため、raw BFSは24回しかscreenされず最終Reject 0だった。Bfs限定では215回screenして32回Rejectし、scoreは負になった。Multi/GrowTrimも専用化でscreen回数がほぼ倍になり、Rejectも倍以上になって負となった。限定armのReject署名がGateAllにも存在したのはBFS 0/32、Multi 35/103、GrowTrim 30/110だけで、大半は専用化により新たに介入した候補である。

単独Rejectの記述集合でも、GateAll 35件のscore差合計は+8.29Mに対し、Bfs 20件は-3.20M、Multi 31件は-5.49M、GrowTrim 39件は-8.63Mだった。post-treatment選択なのでATEではないが、共有screen枠による競争と強い希少性が暗黙の正則化として働く説明とは整合する。GrowTrimはconfirmation budget skipが2件あり、純粋source限定に加えてcap打切りを含む複合方策である。

### risk帯

| GateAll risk帯 | screen | screen Reject勝利 | 最終Reject |
|---|---:|---:|---:|
| [1,2) | 197 | 50 | 32 |
| [2,4) | 198 | 36 | 24 |
| [4,8) | 145 | 30 | 20 |
| [8,16) | 115 | 19 | 15 |
| [16,32) | 83 | 11 | 8 |
| [32,+inf) | 53 | 1 | 0 |

`R`が大きいほどRejectへ進みやすい単調性は見られない。特に`R>=32`は53 screen中screen勝利1、最終Reject 0である。`R`は空間hazardであり、現在料金を放棄する費用を含むnet Reject価値ではない。候補母数を絞る入口としては使えても、現在の尺度のまま有害度やReject優先度とは解釈できない。また`R<1`をscreenする対照armはないため、閾値1そのものの有効性は今回の実験から判定できない。

## 採否と残る問い

主仮説は`mixed/inconclusive`。EarlyMidの正方向とLateの負方向は「後続が残る時期ほど細切れ空間の利用が将来へ響く」という考えと部分的に整合するが、全CIが0をまたぎ、GateAllはEarlyMidより高く、非加法性も大きい。時期だけでGateを採用・棄却するには証拠が足りない。

全限定armを棄却し、通常提出候補は無フラグbaselineのままとする。GateAllもv26どおりscore CIとCPUで不採用。探索的に正だったEarlyMidも同一0..99を見た後のarmで、CPU mean/p95が条件超過しているため採用しない。

次に検証価値がある問いは次の順である。ただし実行後制限により、この記録では実装しない。

1. window 2単独とwindow 0〜1を分け、利益が第三四半期へ集中するという仮説を未見seedで検証する。
2. source専用化ではなく、BFSをscreen対象外にしつつmulti/GrowTrimが共有する希少な2枠を保つ。
3. 「各windowで最初の2件」という到着順依存を、安価なpriorityで候補間比較してからscreenする。ただし`R`そのものは非単調なのでpriorityへ直結させない。
4. rollout前のrisk計算とscreen/confirmationを軽量化し、solver CPUをbaseline比+15%以内へ収める。

最初のv27解答実行後、solution、方策、定数、config、binaryは変更していない。コミットも行っていない。
