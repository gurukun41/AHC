# v28: 実未来offline teacherと軽量価値モデル

更新日: 2026-08-04 JST

状態: 完了・現行の線形価値modelは不採用・無フラグbaseline維持

## 目的

v27ではconnected配置をRejectする短い生成rolloutの順位性能が弱く、100 seedでは不確実性も大きかった。v28は方策を変更する実験ではなく、広い到着集合について次をオフライン測定する診断実験である。

1. 通常配置を固定したAcceptとRejectの長期差
2. Acceptを固定した通常primaryと配置次点の長期差
3. 池形状・現在の占有・候補による分断を分けて表す安価な特徴の追加価値

admissionとplacementは別ラベルにする。repackingは両branchのcontinuationで無効に固定し、v28の教師対象には含めない。Compact rescue / NoRegion Push-outを調べる場合は別のrepacking実験にする。

## 行動不変条件

- `AHC069_COLLECT_OFFLINE_VALUE`を付けた診断buildだけが収集する。
- 実ターンの通常admission、通常placement、root比較、repacking、出力は無フラグbaselineと同じである。
- 実ターンで選ばれたplanの出力後に、保存したroot状態だけを使って強制placementを再生する。
- 重い教師rolloutは1000到着を全て出力した後に行う。
- stdoutには診断を出さず、stderrの`offline_value_*`行だけを追加する。
- 既存seed 0〜99で`tools/out-wide-stp-v25-default/`と100/100 byte一致することを最初のhard gateとする。

## 追加入力とseed分割

公式generatorをwrapperから呼び、実generator seedをそのままファイル名にする。既存`tools/in/`を上書きしない。

| 用途 | 実generator seed | 件数 | 使用法 |
|---|---:|---:|---|
| 互換sanity | 0〜99 | 100 | 既存入力・既存stdoutとの一致確認だけ |
| train | 100〜899 | 800 | 係数推定 |
| validation | 900〜1099 | 200 | feature familyとridge係数の選択 |
| final holdout | 1100〜1299 | 200 | 選択済みmodelを1回だけ評価 |

生成物は`tools/in-offline-value-v28/`、manifestは`tools/input-manifest-offline-value-v28.json`。wrapperは既存directory/manifestがあれば上書きを拒否し、seed、各入力SHA-256、ordered digest、generator binary/source/Cargo.toml/Cargo.lock hashを記録する。

Pahcerのcase IDと既存`best_scores.json`を衝突させないため、Pahcer rootは`pahcer-v28/`へ分離する。収集は時間比較ではなくデータ生成なので8並列を使う。正式なsolver CPU判断は将来modelを方策へ統合した版を別途1 threadで測る。

## root sampling

進行を4 windowへ分け、各windowで次の4 strataを持つ。

1. baseline Accepted / MinimumTemplate
2. baseline Accepted / ExtendedTemplate
3. baseline Accepted / connected系（BFS / MultiStart / GrowAndTrim）
4. baselineがUpperBoundRejectedまたはActualFeeRejectedだが、料金gateを外すと通常配置可能

各`window × stratum`で、case hash・turn・stratumだけから作るpriority hashが最小の1 rootを保存する。したがって最大16 root/caseで、先頭到着へ偏らない。stratum 4は全経済Rejectを診断再生して配置可能母数を確定する。`stratum_population`は同stratumの配置可能root数であり、解析時のsampling weight専用とする。

NoRegionは通常placementを固定したadmission比較を作れないためv28対象外とし、将来のrepacking cohortへ分離する。

## 教師ラベル

保存rootより後に実際に入力された到着を、最大64組まで全branchへ同じ順序で与える。未来入力は教師ラベルだけに使用し、説明変数には含めない。

continuationは次へ固定する。

- root時点のsampled DLP価格と時間境界を凍結
- 未来の通常admissionと通常placementはbaseline関数を使用
- 未来内のCompact rescue、Push-out、runner-up root、Reject gateは再帰しない
- 移動を行わないため、既存組の料金と移動費はbranch間で同一

これはproduction全方策を未来情報で再実行した値ではなく、`actual input / frozen DLP / no repacking continuation`の因果差である。

### admission

同じ通常primary領域`C`について、

```text
reject_gain_H
  = future_fee(Reject, H)
    - candidate_fee
    - future_fee(Accept C, H)
```

をH=16、64で記録する。正なら教師上はReject、負ならAcceptが良い。shadow opportunity costは特徴であり、実現料金差へ再度引かない。

### placement

Acceptを固定し、primary `C0`と現行順位の次点最大2件`Ck`を比べる。

```text
alternative_gain_H(k)
  = fee(Ck) + future_fee(Ck, H)
    - fee(C0) - future_fee(C0, H)
```

正なら教師上は次点が良い。admissionとplacementを混ぜない。

## 特徴

全特徴はroot時点で観測できる値だけから作る。`case_hash`、priority、sampling後に確定するstratum population、実未来fee/count、教師gainは説明変数へ入れない。

### 経済・時期

- 進行度、滞在時間、P、V、theta
- ideal fee、candidate fee、opportunity cost、fee surplus
- free cell、active group、周長超過、placement source

### 既存空間値

- expected overlap
- future-fit U0/U1、damage、risk。risk一値だけへ潰さず各要素も残す

### 池とtopology

- 池セル数、池成分数、最大池成分、池―芝境界長、静的芝成分
- 配置前後のfree成分数、最大/第2成分、P以上成分数、小成分セル数
- 候補周長を池・盤外・占有・残存freeへの辺へ分解
- bbox充填率、最近傍池距離
- 候補を除いた残片数と、最大片以外へ分ける`split_excess_mass`
- `empty_owner`でも同じ候補を除き、池だけが作るpond-only splitと現在占有が追加したsplitを分離

周長辺分解、component cell和、配置前後free cell差、component数差をidentity checkし、error 0を必須とする。

## 解析の事前固定

解析器は標準Pythonだけを使うweighted ridgeで、seed範囲をまたいで行を混ぜない。目的値はH=64 gainを現在到着のideal/base feeで正規化する。sampling weightは配置可能stratum populationとする。

admissionのfeature family:

1. `economic`
2. `existing_spatial` = economic + 既存future-fit/risk
3. `pond_topology` = existing_spatial + 池・component・辺・split

placementのfeature family:

1. `existing`
2. `pond_topology`

各familyでridge `lambda ∈ {0.01, 0.1, 1, 10, 100}`を比較する。validationのweighted teacher regret rate最小を選び、同率なら特徴数が少ない方、その後name順とする。選択後はtrain+validationで係数を再推定し、model specを固定してからfinalを開く。

final前には第2段model lockを作り、model spec、selection JSON、train/validationの全stdout・stderr ordered digest、両Pahcer result JSON、初期freeze manifest hashを固定する。final runnerはこれらを全再照合し、専用tokenなしでは実行しない。

報告値:

- H16/H64の符号一致率
- weighted sign accuracyとnormalized MAE
- baseline方策またはprimaryに対するteacher gain
- teacher oracleに対するregret rate
- seed単位5000回bootstrapのteacher gain 95% CI

池/topologyが有用と判断するのは、pond familyがfinalでも既存familyより安定して選ばれ、かつ選択modelのteacher gain CI下端が0より大きい場合を基本とする。CIが0をまたぐ場合はinconclusive、負なら棄却する。これは実score昇格ではなく、次の方策A/Bを作る根拠に限る。

## 実行前hard checks

- collector無/有とsanitizer buildが成功
- 新規警告0（既存conversion警告のみ）
- generator seed範囲・ordinal rename・全hashを監査
- source、binary、tester、generator、全config、解析器、入力manifestをfreeze manifestへ固定
- collectorとv26/v27のbehavior-changing flagを同時に使わない

## 実行後の停止点

最初のcollector実行後はsource、解法、config、binaryを変更しない。sanity、train、validation、選択、固定済みfinalまで同じartifactで進め、結果と本記録だけを更新する。価値modelを`main.cpp`の判断へ統合するのは、結果報告後の新しいユーザー明示指示を待つ。

全splitは`tools/run_offline_value_v28.py`経由だけで実行し、TOMLを直接`pahcer run`へ渡さない。runnerは各stageの入力範囲、stdout/stderr、Pahcer resultをhashしたreceiptを発行する。trainはsanity成功receipt、validationはsanity/train成功receipt、finalは全receiptと第2段model lockが揃わない限り開始しない。これにより100 seedは性能判断ではなく、100/100 stdout一致を確認する最初のhard gateとしてのみ使う。

## Artifact hash

実行直前に21 artifactと入力corpusを`tools/offline-value-v28-freeze.json`へ固定した。freeze manifest自体のSHA-256は`408015175c7744ede7c28fd528e6e250454a53ccf252b0e321a3831d1e455b36`。

- `main.cpp`: `21d4397e0a0dc3b86df4599d107cb942ca735df5b51bc24761cf0e92e3d945d3`
- control binary: `4a35dace44c9ef79df19bfe80742bdf77da72deda81fa3882f679d55fccf7b2e`
- collector binary: `666a5c914d3f2a270a53497a562fc89d75d09ecc658ead45baff26c48bde3600`
- sanitizer binary: `e58f7715a3211e2d8d12390b35343710d81ed81e64a73e01c6edcf8fa606f529`
- 新規1200入力ordered digest: `5a9c358066517b09e2796b829c3c1e7d9d2d88ff38584925688c038ef3e8e365`

ここから先は凍結対象を変更せず、runnerが各stage開始時に全hashを再検査する。

## 実行の完全性

全stageをfreeze済みartifactのまま順に実行した。sanityは既存0〜99、性能推定用corpusは公式generator seed 100〜1299である。

| split | 用途 | AC | admission行 | placement root / 行 |
|---|---|---:|---:|---:|
| sanity 0〜99 | baseline互換だけ | 100/100 | 1,519 | 1,162 / 2,099 |
| train 100〜899 | 係数推定 | 800/800 | 12,191 | 9,300 / 16,974 |
| validation 900〜1099 | family・lambda選択 | 200/200 | 3,076 | 2,292 / 4,202 |
| final 1100〜1299 | 一回限りholdout | 200/200 | 3,052 | 2,332 / 4,243 |

新規1200 seedの合計はadmission 18,319行、placement 13,924 root / 25,419行、teacher continuation 4,844,094 policy step。全1,300 caseでWA、欠損、非finite、root重複、placement孤児は0だった。`accepted_replay_mismatches=0`、`geometry_identity_errors=0`、全`*_error(s)` / `*_mismatch(es)=0`。sanity stdoutはbaselineと100/100 byte一致した。

`forced_placement_failures`は経済Rejectを診断再生した際に通常配置不能だった件数であり、不変条件違反ではない。Accepted側の配置再生失敗は0。

### 実行時間

p95はnearest-rank。sanityは1 thread、他は8 threadなのでPahcer wallのsplit間比較には並列競合が入る。

| split | Pahcer wall mean / p95 / max | solver CPU mean / p95 / max | diagnostic CPU mean / p95 / max |
|---|---:|---:|---:|
| sanity | 3.122 / 4.340 / 4.767s | 1,012.690 / 1,373.931 / 1,620.098ms | 2,000.352 / 2,893.625 / 3,114.944ms |
| train | 7.640 / 10.130 / 12.378s | 1,672.368 / 2,164.276 / 2,938.689ms | 3,420.859 / 4,794.059 / 5,720.065ms |
| validation | 7.539 / 10.002 / 11.478s | 1,658.760 / 2,102.260 / 2,995.019ms | 3,408.201 / 4,796.576 / 5,210.901ms |
| final | 7.487 / 9.858 / 12.294s | 1,660.815 / 2,192.048 / 2,874.692ms | 3,388.592 / 4,767.688 / 5,536.843ms |

新規1200 seedのsolver CPU平均は1,668.175ms、p95 2,162.577ms。`timing_diagnostic_cpu_ms`はoffline replay・teacher rolloutだけでなく、静的幾何、通常の損失分解、最終集計も含み、collector単独counterは出力していない。これは収集build・異なるseed集合・8並列の値であり、提出方策のCPU A/Bではない。将来modelを実方策へ統合する場合は、collectorを外した1 thread paired比較を別に行う。

## validation選択

候補はtrainだけでfitし、validationのweighted teacher regret rate最小を選んだ。gainはstratum populationで母集団へ戻したteacher推定値であり、実contest score差ではない。

| 対象 | 選択model | no-change regret rate | 選択regret rate | teacher gain | seed bootstrap 95% CI |
|---|---|---:|---:|---:|---:|
| admission | `economic_ridge_10` | 0.683518 | 0.666159 | +316.0M | [-215.1M, +865.2M] |
| placement | `existing_ridge_0.01` | 0.741783 | 0.739240 | +28.1M | [-1,019.2M, +1,169.1M] |

どちらも点推定ではno-changeを上回ったがCIは0をまたいだ。admission family championは`economic < existing_spatial < pond_topology`のregret順、placementはexistingがpondの完全no-opを0.00254だけ上回った。

選択後のtrain+validation refitは、同じvalidationへ戻しても次まで縮退した。これは事前選択値ではなく、固定modelの事後診断である。

| 対象 | refit後validation gain | 95% CI | regret rate |
|---|---:|---:|---:|
| admission | +39.8M | [-576.2M, +650.0M] | 0.681333 |
| placement | -919.3M | [-2,182.7M, +249.0M] | 0.825007 |

placementは低正則化lambda 0.01の係数と0閾値がrefitで大きく動き、選択時の微差が再現しなかった。この時点でもmodelを変更せず、事前固定どおりlockしてfinalへ進んだ。

## final holdout結果

| 対象 | 固定model | no-change regret rate | model regret rate | teacher gain | seed bootstrap 95% CI | 判定 |
|---|---|---:|---:|---:|---:|---|
| admission | `economic_ridge_10` | 0.717943 | 0.744849 | -521.0M | [-1,216.4M, +153.0M] | 不採用・有害性はinconclusive |
| placement | `existing_ridge_0.01` | 0.809631 | 0.885615 | -928.2M | [-2,308.7M, +408.3M] | 不採用・有害性はinconclusive |

admissionは重み付き符号正解率68.66%でも、overrideの有益側が重み11,406・`+1.654B`・平均`+145k`に対し、有害側は重み7,515・`-2.175B`・平均`-289k`だった。seed単位73勝21同値106敗。placementは98勝4同値98敗でも損失側の裾が重く、点推定が大きく負になった。最悪5 seedの合計はadmission `-460.1M`、placement `-874.4M`で、総差の大部分を占める。

H16とH64の符号一致率はadmission 71.20%、placement 65.35%。短い未来の方向だけでも長期labelの約3〜3.5割が反転し、単一suffixのH64自体も大きな分散を持つ。

## どこに改善余地があり、なぜmodelが失敗したか

finalの母集団推定では、baseline Acceptedの29.84%はteacher上Reject有利、経済Rejectの55.77%はteacher上Accept有利だった。placementも重み付き35.69%のrootで少なくとも1次点がprimaryを上回った。したがって「baselineに反転余地がない」のではなく、「有益な反転を今回の安価な特徴を使ったridgeでは選別できていない」が結論である。

admissionをstratum別に見る。

| stratum | Reject有利率 | 平均 `Reject - Accept` | 読み方 |
|---|---:|---:|---|
| Minimum Accepted | 25.89% | -18.4k | 大半はAccept有利だがRejectすべき裾もある |
| Extended Accepted | 34.81% | -53.7k | 件数は少なくAccept優勢 |
| connected Accepted | 39.82% | -9.9k | MinimumよりReject有利率は高いがblanket Rejectは負 |
| economic Reject | 44.23% | +50.6k | Accept有利が過半数でも、誤Acceptの損失裾が重い |

選択modelのfinal悪化は経済RejectをAcceptへ戻す部分が中心だった。refit modelのvalidation→final寄与は、Accepted `-266.4M→-95.1M`、ActualFeeRejected `+47.6M→-132.3M`、UpperBoundRejected `+258.6M→-293.7M`。特に`UpperBoundRejected × Minimum`は平均labelがtrain `+19.5k`、validation `-32.0k`、final `+12.5k`とvalidationだけ反転し、modelがほぼ全件をAcceptへ戻したため`+434.0M→-159.1M`へ崩れた。

placementではfinal 2,332 rootのraw 34.56%、`stratum_population`重み付き35.69%に改善次点があった。一方、alternative行単位の重み付き正率は27.78%、平均H64 gainは`-8.2k`。rank 1の重み付きprimary超え率は30.09%でrank 0の25.73%より高いが、重み付き平均gainは`-13.3k`対`-3.7k`でdownsideが大きい。現行の短期順位は長期価値順ではなく、単純に次点へ寄せることもできない。

## 池・topologyの判定

「池形状が無価値」とは判定しないが、今回のweighted ridgeとsingle-validation選択には採用根拠がない。

- admission pond modelはfinal gain `-139.9M`、CI `[-1,151.7M, +990.0M]`。existing spatialとの差は`+437.2M`だがCI `[-389.7M, +1,455.0M]`。
- placement pond championはlambda 100で`selected_weight=0`の完全no-op。finalでexistingより`+928.2M`良く見えるのは、池特徴で良い場所を選んだためではなく、有害overrideを一度も行わなかったため。差のCIも`[-408.3M, +2,308.7M]`。
- bbox充填率差と第2成分差には弱い符号一貫性があるが、pond supportは相関が小さく、split系はsplit間で符号反転した。

## 固定結論

v28の`economic_ridge_10` admissionと`existing_ridge_0.01` placementは、正の昇格根拠がないためともに不採用とし、`main.cpp`の通常方策へ統合しない。点推定は負だが両CIは0をまたぐため、「有害だと統計的に確定した」という意味ではinconclusiveである。無フラグbaselineを提出候補のまま維持する。repackingは今回のteacherから除外しており、この結果をCompact rescue / Push-outへ外挿しない。

再利用する設計上の示唆は次である。

1. 100 seedはbehavior sanityに限定し、性能判断用seedはgenerator wrapperで独立に増やす。
2. raw validation最良ではなく、no-changeを明示候補にして正のCIまたは十分なmarginがない限りabstainする。
3. admissionはMinimum / Extended / connected Accepted / UpperBoundRejected / ActualFeeRejectedという排他的cohortとして扱う。双方向反転を1本の線形modelへ混ぜない。
4. 頻度より損失裾が支配するため、符号正解率や二乗誤差だけでなくtail regretを直接抑える。
5. placementはcandidateごとの独立scoreより、root内のpaired差と高信頼overrideを中心にする。
6. single validation 200でも選択が反転した。自動生成corpusを複数seed foldへ分け、refit後の方策もfinal前にcross-fitで検査する。
7. H16/H64不一致が大きい。より長い単一未来だけでなく、複数continuationでlabel分散を測る余地がある。

これらは次実験候補の整理であり、このturnでは新方策を実装・実行しない。

最終生成物のSHA-256:

- selection: `4fcf248b080a3c56e38da2437b6377497fd2b930615ac2a8bf8d8eb4dab6c520`
- model spec: `6cceacaa2bb2d1bb976720b7de808a4fb4082328819e6151fc5d596671177b0c`
- model lock: `4a7245f0c7ed4a9b4ca130f79a2d26c29ef3992144aa036443fb3fe8b3cc9d1c`
- final analysis: `10f18e42953bdf2f9e212d4b522381c09a278e10de96347e95de2df1a55370e9`
