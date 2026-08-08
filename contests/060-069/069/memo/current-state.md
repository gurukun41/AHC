# AHC069 現在状態

更新日: 2026-08-08 JST

## 最初に確認すること

- ブランチ: `069`
- v33区切りコミット: `AHC069: checkpoint v33 smooth-gated solver`（親HEADは`1fb776e`）
- HEADはv33区切りsourceである。実測incumbentはv35で、100 seed平均`66,438,655.07`、3000 case平均`68,007,828.3783`である。v37は3000 caseでv35比`-0.004618%`となり撤去済み。v38は100 / 3000 caseでv35比`-0.196676% / -0.119609%`となりnon-incumbentと判定した。ユーザーの明示指示によりv38固有差分を撤去してv35正本へ復元した後、correctness監査と挙動保存整理だけを加えた。現在の未コミット`main.cpp`はv35方策等価の6,677行、SHA-256 `0878466f475c52b4d6cbfa2f56aa8fa642cd6f040491c46adb49307883057652`である。詳細は[maintenance記録](experiments/20260808-v35-maintenance-audit.md)。
- 現在実装はtheta推定、sampled DLP、template / connected / grow-and-trim、future-fit、Compact rescue、NoRegion Push-out、root rolloutを全て持つ。初期芝セル数を`G`、芝から池・盤外へ出る4近傍辺数を`E`とし、DLPを`E/G<0.55`で1.30、`0.55<=E/G<0.70`で1.25、その他で1.00に固定する。`E/G>=0.80`だけplacementを保存p2設定`5/8/24/12/8/0.50`へ替え、`.70<=E/G<.80 && R<.060`だけ第五expertとしてroot未来差重みを1.00から0.10へ下げる。実到着・仮想到着・Push-out経済gateへ同じDLP倍率を各1回適用する。
- v33のplacement polishは、実ターンで従来方策が経済的にAcceptした`P>=50`のConnectedGrowth / GrowAndTrimのうち、初期`E/G<0.55`のsmooth expertだけを対象にした。v35ではdenseの理論最大料金改善`U>=10,000`・case最大24実走査・`P>=50`を維持し、strict 1-swap descentだけ面積gateから分離して全eligibleへ最大8 step適用する。
- dense / descent候補は丸め後料金strict増を必須にし、異周長polishにはv35の3 snapshot square-fit非悪化guardを維持する。通常shortlistもv35の境界costと3 snapshot square future-fitで順位付けする。admission、候補集合、料金、repacking、root発火もv35正本どおりである。
- v34の同一最大gain次点分岐はユーザーの100 seedで平均`66,409,477.31`、v33比`-714,127 (-0.010752%)`・3勝95分2敗だったため棄却し、sourceから完全撤去した。追加枝の即時fee gain`+30,216`に対して後続影響`-744,343`であり、負seedだけを除くpost-hoc gateは作らない。詳細は[v34記録](experiments/20260808-strict-tie-multistart-v34.md)。
- v35は新しい候補枝・source・expertを追加しない。`P<50`でも既存と同じstrict料金増、3 snapshot scalar future-fit非悪化、old/root rollback、synthetic無効を通す。各removeでfrontier最大近傍数がremove近傍数以下なら正gain不可能なので、全add scanを意味保存で省く。100 seedではv33比`+2,203,649 (+0.033179%)`・19勝81分0敗だった。改善の93.205%はseed 42に集中するため、負seedなしの安全信号と効果量の集中を分けて扱う。詳細は[v35実装・実測記録](experiments/20260808-small-group-strict-descent-v35.md)。
- v36は完了済みAHC上位解の「強い骨格を固定し、その中だけ局所改善する」原則を採用し、非smoothかつ`P<50`だけ空き骨格保護付きstrict descentへ開放した。100 seedではv35比`-3,079,288 (-0.046348%)`・8勝88分4敗。smooth 70 seedは完全同点だった一方、非smoothの正差`+41,707`を4敗の負差`-3,120,995`が上回ったため棄却した。seed別・`E/G`別のpost-hoc gateは追加せず、v36固有のBFS、非smooth枝、診断を全撤去して実行時v35へbyte単位で復元済み。詳細は[v36実装・実測・撤去記録](experiments/20260808-free-space-backbone-small-descent-v36.md)。
- v37は`Rq>=1`だけ通常future-fitを使うhard gateだった。3000 caseは合計`204,014,063,376`、v35の`204,023,485,135`比`-9,421,759 (-0.004618%)`。問題構造上も期待未来損失は`Rq`へ連続で、1に不連続点がないため撤去した。詳細は[v37記録](experiments/20260808-expected-overlap-future-fit-v37.md)。
- v38は`V/P=D^0.9 2^Z`を使い、`E[2^Z]`を解析積分し、開始時刻条件付き`E[D^0.9]`とcompactnessから各configuration列の期待料金を作った。同面積classの全合法列へfractionalに流すセル価格を試したが、3000 caseでv35比`-244,030,711 (-0.119609%)`、1433勝0分1567敗だった。評価turnの57.734%で既存選択を変更し、solver CPU平均は`2083.124ms`だったため、精度・計算量とも昇格根拠なしとし、固有コードを全撤去した。詳細は[v38実装・実測・撤去記録](experiments/20260808-spatial-template-shadow-v38.md)。
- 通常template placementとrescue targetは、N=50の行bitset + 行方向OR sparse tableから合法anchorだけを従来順に列挙する`LegalAnchorIndex`を使う。合法集合、tie-break、診断上の論理anchor数は旧累積和版と同じ。実測では内部solver CPU meanが`1644.464ms`から`1240.126ms`へ短縮した。
- 現行incumbentの100 seed正本はv35の[result_20260808_030825.json](../pahcer/json/result_20260808_030825.json)。comment `test`、seed 0〜99を100/100 AC、合計`6,643,865,507`、平均`66,438,655.07`、WA 0。v33の[result_20260808_012614.json](../pahcer/json/result_20260808_012614.json)比`+2,203,649 (+0.033179%)`・19勝81分0敗、negative gross 0、seed ratio p05 / worstはともに1.0である。
- v35の3000 case正本は[result_20260808_180959.json](../pahcer/json/result_20260808_180959.json)。3000/3000 AC、合計`204,023,485,135`、平均`68,007,828.3783`である。v37は[result_20260808_193707.json](../pahcer/json/result_20260808_193707.json)。
- v38の100 case正本は[result_20260808_204340.json](../pahcer/json/result_20260808_204340.json)。100/100 AC、合計`6,630,798,632`、平均`66,307,986.32`、v35比`-13,066,875 (-0.196676%)`・46勝0分54敗。3000 case正本は[result_20260808_225925.json](../pahcer/json/result_20260808_225925.json)。3000/3000 AC、合計`203,779,454,424`、平均`67,926,484.8080`、v35比`-244,030,711 (-0.119609%)`・1433勝0分1567敗である。
- v36の100 seed正本は[result_20260808_105054.json](../pahcer/json/result_20260808_105054.json)。comment `test`、100/100 AC、合計`6,640,786,219`、平均`66,407,862.19`、WA 0。v35比`-3,079,288 (-0.046348%)`・8勝88分4敗、ratio p05は1.0、worstは`0.966510`。変更12 seedのうち4敗（seed 25 / 44 / 87 / 91）が悪化を支配し、特にseed 44は`-1,569,199`だった。
- v35はv31比`+9,459,923 (+0.142589%)`、v32比`+9,684,025 (+0.145972%)`、直前4-expert比`+21,924,460 (+0.331088%)`である。なおv33自身のv31 / v32 / 4-expert比は`+7,256,274 / +7,480,376 / +19,720,811`だった。
- v31差の初期`E/G`別集計は`<.55: +18,587,452`、`.55-.625: +526,864`、`.625-.70: -1,711,877`、`.70-.80: -298,990`、`>=.80: -4,638,912`。同じ開発100 seed上のpost-hocなのでfresh推定ではないが、v32でpolishをsmoothだけへ限定する根拠とした。
- v32−v31の初期`E/G`別差は`<.55: -7,480,376`、`.55-.625: -526,864`、`.625-.70: +1,711,877`、`.70-.80: +1,432,349`、`>=.80: +4,638,912`。非smooth 30 seedの`+7,256,274`がsmooth 70 seedの`-7,480,376`をほぼ相殺した。静的保護は合計で正、smooth側の同時変更は合計で負だが、dense予算・Pareto・plateauの個別寄与は未分離である。
- v31のconnected polishはeligible `15,815`回、dense実走査`2,368`回、dense / descent最終choice `158 / 504`、placement polish change `690`回。内部solver CPUはmean `1200.498ms`、p95 `1629.677ms`、max `1916.647ms`、2秒超0/100だった。
- v31の100 logは53種類のerror/mismatchが全件0、score再構成は100/100でJSONと一致。実行結果は[connected polish + root v31記録](experiments/20260807-connected-polish-root-v31.md)、v32設計・実測は[plateau実装記録](experiments/20260807-connected-polish-plateau-v32.md)を正本とする。
- v33実測は、実行前に固定したcasewise反実仮想と合計・平均が完全一致した。入力の`E/G`を再計算するとsmooth 70 seedはv31、非smooth 30 seedはv32のscoreと100/100で一致し、mismatch 0。設計・実装・実測の正本は[v33記録](experiments/20260808-static-polish-gate-v33.md)。同じ開発100 seedの再利用なのでfresh保証ではない。
- `log(duration/theta)`上の連続受入率で未来標本のpopulation weightを補正した案は`60,460,863.02`で、protected lightweight `64,391,644.89`比`-6.104491%`。失敗として記録し、実装を撤去済み。詳細は[実験記録](experiments/20260807-continuous-acceptance-rate.md)。
- 異周長を含む全候補を共通12未来標本の期待料金で比較した過去案は`63,658,356.57`で、protected lightweight比`-1.138794%`。不採用であり、コードは撤去済み。詳細は[実験記録](experiments/20260807-expected-future-fee-all-candidates.md)。
- accept/reject二容量の未来獲得額を直接比較する受入判断Treatmentは、ユーザー報告スコア`61,785,497.09`で悪化し不採用。実装は撤去し、受入判断を従来のsampled DLPによる`時間帯価格 × 使用量`へ戻した。スコア以外の確認・原因分析は行っていない。詳細は[実験記録](experiments/20260806-direct-admission-comparison.md)。
- v29 Optuna最終調整はユーザーが全手順を実行済み。全blockがvalidation gateでbaselineへ戻り、`main-optuna-final.cpp`も当時のbaseline Git objectとbyte一致したため非baseline不採用で終了した。
- ユーザーの明示指示なしにコミットしない。
- `AGENTS.md`のコード保守規則と、解答実行後の変更制限を守る。

ユーザーの2026-08-04の明示指示により、v26のconnected Rejectを時期・最終source別に調べるv27を実行した。Control / GateAll / EarlyMid / Late / Bfs / Multi / GrowTrimを各100 seedで比較し、全arm 100/100 AC、1,188,482 hard checks error 0。source・binary・config・input・oracleは[v27実験記録](experiments/20260804-fallback-risk-attribution-v27.md)を正本とし、`main.cpp`のSHA-256は`d576c6cee56fe9d2a94d7165d413ec1fafcccc255b57666a0bf147f209702b0c`。v27の各arm実行開始から結果確定までは固定source・方策・config・binaryを変更していない。v28への変更は、その後のユーザーの新しい明示指示を受けて行った。

v27はEarlyMid `+0.1022%`、Late `-0.0160%`でユーザーの時期仮説と部分整合したが、GateAllがEarlyMidを`+0.0493%`上回り、全bootstrap CIが0をまたいだため固定判定は`mixed/inconclusive`。Bfs / Multi / GrowTrim限定は全てControl未満で、共有screen予算の希少性が暗黙の正則化として働く可能性が高い。全armはscore不確実性またはsolver CPU条件で不採用、通常提出候補は無フラグbaselineのまま。

ユーザーの次の明示指示でv28を実行した。v28は方策変更ではなく、実到着列を最大64組再生して通常Accept/Rejectと通常placement次点の長期差を収集し、池・空き成分・候補分断を含む軽量価値モデルをseed分離で評価する診断実験である。公式generatorをwrapper化し、既存0〜99はstdout互換sanityだけ、新規seed 100〜1299はtrain 800 / validation 200 / final holdout 200へ固定した。全1,300 caseがACで、sanityはbaselineと100/100 byte一致、全不変条件errorは0。source・binary・入力・runner・解析器を含む21 artifactは実行前に固定し、freeze manifest SHA-256は`408015175c7744ede7c28fd528e6e250454a53ccf252b0e321a3831d1e455b36`である。

finalでは、baseline Acceptedの29.84%が教師上Reject有利、経済Rejectの55.77%がAccept有利、placement rootの重み付き35.69%にprimaryより良い次点があり、方策を反転する余地自体は確認できた。しかしvalidationで選んだadmission `economic_ridge_10`はteacher gain `-521.0M`、95% CI `[-1,216.4M,+153.0M]`、placement `existing_ridge_0.01`は`-928.2M`、CI `[-2,308.7M,+408.3M]`で、どちらも昇格させず不採用とした。点推定は負だがCIは0をまたぐため、統計的な有害性自体はinconclusiveである。connected AcceptedはReject有利率39.82%でも平均`Reject-Accept=-9.9k`なので、一律Rejectの根拠にはならない。池/topologyも今回のweighted ridgeとsingle-validation選択では採用根拠を示せなかった。詳細は[v28実験記録](experiments/20260804-offline-value-model-v28.md)。通常提出候補は無フラグbaselineのままである。

v26の最終source/binary/config/input/oracle hashは実行前に固定済みであり、[v26実験記録](experiments/20260803-fallback-phase-v26.md)を正本とする。v26 `main.cpp`のSHA-256は`7ee9165b5b7075bb4cef5980631320fbf4efea92e850e0af89a1cc93fb04fb9a`である。

## 旧full比較基準、v31、v32、v33、棄却v34、incumbent v35、非incumbent v36

過去のfull baselineは、theta推定、sampled DLP、`Lmin..Lmin+4` template、raw BFS、multi-start connected growth、grow-and-trim、future-fit、Compact rescue、NoRegion Push-out、root rolloutを含む。100 seed oracleは`pahcer/json/result_20260803_003818.json`、合計`6,515,194,836`。復元sourceは`main-optuna-final.cpp`、SHA-256は`086cb6cc2e24848c77a4b766ab8dde433dbf16469095cca557f848bdff091c6a`である。

静的DLP版は平均`66,163,871.34`、そこへ4領域expertと`LegalAnchorIndex`を統合した版は`66,219,410.47`。v31は第五root expertとconnected polishを統合して`66,344,055.84`、v32はsmooth限定、価値予算、plateau、Pareto guardを追加して`66,341,814.82`。v33は非smooth停止だけをv32から残し、smoothをv31へ戻して`66,416,618.58`であり、実行前のcasewise反実仮想と実測が一致した。v34は`66,409,477.31`で棄却済み。v35は小規模strict descentを追加して`66,438,655.07`となり新incumbentである。v36は非smooth小規模へ空き骨格保護付きdescentを追加したが`66,407,862.19`で、v35比`-0.046348%`のため非incumbentである。さらに一つ前の軽量hybridは`66,036,150.29`。実測値は全て同じ開発100 seedでありfresh性能と混同しない。

## 実行済みのv29 Optuna最終調整

現行方策の8定数をadmission、placement、rootの3 blockへ分け、fresh seed 1,400件をsearch 400 / block validation 300 / combination validation 300 / final holdout 400へ固定した。既存100 seedのdefault互換sanityは100/100 stdout byte一致、score合計`6,515,194,836`だった。

block validationではadmission `dlp_scale=1300`が`+1.988659%`、placement bestが`+0.162910%`、root `weight=100`が`+0.056088%`だったが、いずれもp05または絶対CPU最大gateで不採用になった。root 900 / 1100は絶対CPU最大だけで落ちた一方、baseline自身も同じ300 seedで最大2714.630msだったため、2000ms絶対gateは今回の実行環境では全非baselineへの拒否条件として働いた。全block winnerがbaselineとなり、combination validationはbaselineだけ、final holdoutは未開封。出力`main-optuna-final.cpp`のSHA-256はbaselineと同じ`086cb6cc2e24848c77a4b766ab8dde433dbf16469095cca557f848bdff091c6a`である。

大規模実行はユーザーが行い、エージェントはその後の明示指示で記録だけを同期した。実行手順、全候補、停止条件、hashは[v29実験記録](experiments/20260805-optuna-final-v29.md)を正本とする。ユーザー判断によりOptunaは一旦終了し、通常提出候補はbaselineを維持する。

## 実行済みのv26実験

通常template幅とconnected受入riskを分ける2×2である。

| arm | 通常template | connected risk比較 |
|---|---:|---:|
| Control | `Lmin+4` | off |
| Wide | `Lmin+8` | off |
| Gate | `Lmin+4` | on |
| WideGate | `Lmin+8` | on |

Risk armでは既存placementと料金admissionを変えない。最終的にraw BFS、multi-start、GrowAndTrimのいずれかが選ばれ、`期待重複到着数 × future-fit相対damage >= 1`となる場合だけ、Accept protectedのQ2/H4 screenと独立Q8/H12 holdoutへRejectを追加する。Compact rescueや通常次点でtemplateへ直せた場合は対象外であり、確定Rejectは`NoRegion`と分離してPush-outへ戻さない。

Compact rescueの発火境界は全armで`Lmin+4`に固定する。screenは進行4区間ごとに最大2回、risk confirmationはケース最大4回の専用予算を持ち、既存rootの最大4回を消費しない。詳細な式、比較、停止条件は[v26実験記録](experiments/20260803-fallback-phase-v26.md)を参照。

100 seed結果は次の通り。全arm 100/100 AC、全不変条件は0、Controlは既存oracleと完全一致した。

| arm | total score | Control差 | solver CPU平均 / p95 | 採否 |
|---|---:|---:|---:|---|
| Control | 6,515,194,836 | 0 | 955.673 / 1236.692ms | 通常baseline |
| Wide | 6,507,545,417 | -7,649,419 (-0.117409%) | 994.990 / 1261.250ms | 棄却 |
| Gate | 6,525,065,419 | +9,870,583 (+0.151501%) | 1187.381 / 1582.743ms | CIが0をまたぎ、CPU+24%で不採用 |
| WideGate | 6,519,121,065 | +3,926,229 (+0.060263%) | 1243.556 / 1683.613ms | CIが0をまたぎ、CPU+30%で不採用 |

Gateはrepacking後のconnected 19,442件のうち99件だけを方策Rejectし、zero-Rejectの38 seedはControlとbyte一致した。accepted ideal feeを11.08M失う一方でshape lossを21.15M改善し、net `+9.87M`。高risk connectedのごく一部が後続compact配置を壊す仮説とは整合するが、全connectedを拒否する根拠ではなく、現在のscreen/holdoutは平均212ms/caseを要する。通常提出候補は引き続き無フラグbaselineである。

## 直前のv25

cell×time空間DLPのFullはcausal Control比`-3.9804%`、SameFeeは`-4.5265%`で棄却済み。コードはユーザーがbaselineへ戻した。数値・hash・CPUは[v25結果記録](experiments/20260803-wide-spatial-v25.md)へ移した。

## ユーザーの開発方針

- 目標は赤パフォーマンス。
- 安全な局所改善だけでなく、大きな案も原因分解できる形で試す。
- admission、placement、repackingを分離する。
- 大きな案が失敗したらbaselineへ戻し、原因を分ける。
- seed 0単独では判断しない。100 seed paired比較を最小の行動確認に使い、高分散な性能・model判断では公式generatorから独立seedを自動生成してtrain / validation / finalを分ける。
- Pahcer wallだけでなく、コード内`timing_solver_cpu_ms`を確認する。
- 日本語コメントを同期し、不要になった実験コードは削除する。
- 大規模seed生成・複数seed比較・Optuna探索はユーザーが実行する。v37 / v38の3000 caseもユーザーが実行し、いずれもv35維持と判定した。v38ではエージェントが全設計・静的監査後にseed 0を一度だけsmokeし、ユーザーが100 caseを`pahcer-studio`、3000 caseを`pahcer_config.toml`と`tools/in_big`で実行済みである。

## 再開時の注意

v26/v27/v28/v29は固定artifactで実行・分析済みで、当時のv28 source SHA-256は`21d4397e0a0dc3b86df4599d107cb942ca735df5b51bc24761cf0e92e3d945d3`。incumbentの100 seed正本はv35の`result_20260808_030825.json`、3000 case正本は`result_20260808_180959.json`、棄却v37は`result_20260808_193707.json`、non-incumbent v38は100 case `result_20260808_204340.json`と3000 case `result_20260808_225925.json`である。現在`main.cpp`はv35の候補・順序・判定を維持したmaintenance版で、行数・SHA-256は冒頭と[maintenance記録](experiments/20260808-v35-maintenance-audit.md)を正本とする。実測scoreの正本は引き続き[v35記録](experiments/20260808-small-group-strict-descent-v35.md)である。旧full baseline、直前lean、静的DLP、4-expert、v31〜v38を混同しない。新しい明示指示なしにsource・方針・定数を変更しない。

## v38 freeze・実行結果とv35復元

- `main.cpp`: 6,993行、source SHA-256 `8edfb91ea48efdab43e75ea39eede0ed2b0bb966368b064e106afa73f08a7959`。
- release binary: `/private/tmp/ahc069-spatial-template-shadow-v38`、SHA-256 `7e1f764e4b567095be9bfc8a0660bf05442afcbe4ad865f9b08943b5bcd2a171`。
- 静的検証: Apple Clang 17、C++17/C++20、`-O2 -DNDEBUG -Wall -Wextra -Wshadow -Wpedantic -fsyntax-only`を警告0でpass。C++20 static analyzerは指摘0、`git diff --check`もpass。
- freeze前の解答プログラム実行: 0回。freeze後にエージェントがseed 0を1回smokeし、ユーザーが100 / 3000 caseを実行した。
- 100 case: `6,630,798,632`、平均`66,307,986.32`、v35比`-0.196676%`。
- 3000 case: `203,779,454,424`、平均`67,926,484.8080`、v35比`-0.119609%`。
- 3000 case結果確定時の`main.cpp`は上記v38 SHA-256と一致していた。
- その後のユーザー明示指示によりv38固有差分を撤去し、v35の6,654行・SHA-256
  `1a5f652b17ca8de08b34920ea35f1928cfea7008dc98a4c7138b933e22d3db60`へ一度byte一致で復元した。
- 復元から下記maintenance開始までの解答プログラム実行: 0回。

## v35 correctness監査・maintenance整理

- 上記byte復元後、ユーザーの明示指示によりv35の全状態遷移と公式tester境界を監査した。Critical / High / Mediumの修正必須バグは0件。
- 4近傍配列、N=50制約、固定scenario数、Shape展開、5-expert設定を一元化し、root marginの古い名前と恒等処理を整理した。
- 候補集合・列挙順・tie-break・評価式・閾値・stderr key・stdout protocolは変更していない。
- 整理後`main.cpp`: 6,677行、SHA-256 `0878466f475c52b4d6cbfa2f56aa8fa642cd6f040491c46adb49307883057652`。
- C++17 / C++20警告付き構文検査、全A/B switch、Clang static analyzer、`git diff --check`は全て指摘0。
- 大規模case実行は行わない。source freeze後にseed 0を1 caseだけsmokeする場合も、その結果から追加変更しない。
