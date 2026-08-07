## 2026-08-01: connected-growthを救済する同時compact再配置 (`compact-rescue-v1`)

旧cleanupの実増分は再配置なし比で約`7m`に留まる一方、既存100ケースでは通常方策が受け入れた高周長fallback `17,497件`に、最小周長との差が合計約`1.13b`残っていた。そこで「将来の空きを予防的に整える」旧cleanupを全て置き換え、今回の到着を最小周長にできる場合だけ、重なる既存組を同時に退避するconstructive rescueへ変更した。

### 発火条件と損益

通常のshadow admissionと通常配置を先に実行し、次を全て満たす場合だけrescueを探索する。

- 通常方策が到着を受け入れた
- 通常配置の周長が`Lmin+4`より大きい（compact templateでなくconnected-growth fallback）
- 最小周長targetと重なる全既存組を移動しても即時利益が正

target `Q`のcheap scanでは、池に重ならない全ての最小周長anchorを列挙する。占有マス数が少ない上位160件と、target上の各マスへ按分した移動費が小さい上位160件の和集合について、重なるownerを全件回収して正確なblocker集合と移動費を計算する。blocker数そのものには上限を設けない。

正確な即時改善は

```text
compact arrival fee - ordinary fallback fee - sum(blocker move cost)
```

で、正の場合だけ残す。改善が大きい順に最大8 targetをrepairする。

### 同時repair

各targetについて、blocker全組の旧領域を最初に同時clearし、target `Q`を到着組で予約してから再挿入先を探す。各blockerの移動先は、rectangle+partial-row/columnの全templateのうち、履歴`max_perimeter`を更新した後も**丸め後の既存料金が変わらない**shapeだけを使う。

- blockerごとのanchor調査: 最大`4,096`
- 1回のrescue判断で全target・全blockerが共有するanchor上限: `50,000`
- blockerごとの合法候補を最大64件集め、時間境界・fallback領域との重なり・解放領域の再利用・四象限の多様性から10件を保持
- 候補数、面積、退去時刻の4種類の挿入順について、予算不要のgreedy spineを全て先に試す
- greedyが失敗した場合だけ、幅32・生成node最大2,048のbitset beamで候補衝突を解く

50×50盤面は40 wordのbitsetで衝突判定する。探索のwork limitはあるが、blocker数によるsemanticな打ち切りはない。

最後に専用validatorが、全移動IDと旧領域を確認し、全移動元clear、各移動先、到着targetの順に、マス数・芝生・重複・4近傍連結・周長を再計算する。さらに全既存組の料金低下が0、全移動費が候補生成時と一致、通常fallback比の即時利益が正であることを再確認してから採用する。

初版は因果を分離するため、rollout、terminal value、将来space score、pre-arrival非悪化gateを一切使わず、即時利益だけで判断する。旧proactive cleanup、旧rollout、fresh-mask、未使用のspace評価は削除した。通常のshadow admissionと時間整合配置は維持する。

### 実行前検証

- `main.cpp` SHA-256: `39d61918180dcae180ba8dea89ad9836ecf78ca99f63a4c85d631dd8f3d62c81`
- Clang C++17 release binary SHA-256: `0f69fc4e2d7893182e758a58a072aa52e5409bc17ea138c7193e1bc97490a9f0`
- Clang C++20 release binary SHA-256: `05cf72b1e503450ec841812760534a7d2dcdcbab93beb1af063ef9599fe69ca8`
- ASan/UBSan binary SHA-256: `2dcd9d73d72b9c58274a810cdb329de27747d956ec1dacad268cfffabc897a8a`
- Clang C++17/C++20 release build: pass、警告0件
- GCC C++17 syntax/警告検査: pass、警告0件
- ASan/UBSan build: pass（解答は未実行）
- Clang static analyzer: 指摘0件
- `git diff --check`: pass
- 独立静的監査2系統: 同時clear、target予約、任意blocker数、fee-neutral条件、移動費、validator、main統合、共有計算量上限を確認し、blocking issueなし

Pahcer設定は`pahcer/bench_compact_rescue_v1.toml`、固定binaryは`/private/tmp/ahc069_compact_rescue_v1`。比較対象はv14 `6,358,193,055`、再配置なし`6,351,225,419`。実行コマンドは次の通り。

```text
pahcer run --setting-file pahcer/bench_compact_rescue_v1.toml --no-compile --freeze-best-scores --comment compact-rescue-v1-immediate-only
```

ここまで固定したrelease/sanitizer binaryは一度も実行していない。以降はsource/binaryを変更せず、seed 0〜99の100ケースを1回だけ測定し、AGENTS.mdに従って結果確認後はユーザーの次の明示指示まで解法・コード・メモを変更しない。

## 2026-08-02: theta互換修正の結果 (`cross-fitted-root-v6`)

固定した通常binaryの100ケース結果は`pahcer/json/result_20260802_014639.json`。

- AC: `100/100`
- 合計score: `6,374,431,352`
- v3比: `-9,347,715`
- v4比: `-12,225,588`
- v5比: `-11,247,540`
- Pahcer時間: 平均`2.944 sec`、中央値`2.968 sec`、95 percentile約`3.764 sec`、最大`4.643 sec`

同時にprotected-only binaryと旧v3固定binaryをseed 0〜99で比較し、全100ケースのstdoutがbyte単位で一致した。両者の合計scoreも`6,383,779,067`で一致したため、v5で見つかった共通経路のtheta driftは解消され、追加root枝を通らない出力は旧v3へ戻った。

v6とv3の差を最終actionで分けると、通常runner-upを含む20 seedは合計`+1,285,532`、Rejectだけを含む61 seedは合計`-10,633,247`、追加actionなしの19 seedは完全同点だった。RejectのQ2/H4 screenおよびQ8/H12 confirmationが見積もった承認marginは合計`+13,582,478`だったのに、実差は大きく負である。したがって、今回の短期生成分布では「現在の受け入れを捨てて空間を温存する」価値を安定して評価できていない。一方、通常runner-upは小幅ながら正であり、Rejectと切り分けて残す価値がある。

## 2026-08-02: Reject枝を除いたcross-fitted root (`cross-fitted-root-v7`)

v6の因果分解に基づき、rescue-rootとnormal-rootの両方からRejectをaction集合ごと削除する。通常shadow admissionの`UpperBoundRejected`、`ActualFeeRejected`、`NoRegion`は本来の受け入れ判断なので変更しない。

### action集合と選択

rescue-rootは次の最大5枝を同じQ2/H4 scenarioで比較する。

```text
ordinary primary
rescue candidate 0
rescue candidate 1（存在する場合）
ordinary runner-up 0（distinctなら）
ordinary runner-up 1（distinctなら）
```

まず`primary / rescue 0 / rescue 1`から旧v3と同じstrict tie規則で保護方策を確定する。通常runner-upがscreenでそれを厳密に上回る場合だけ、独立したQ8/H12 holdoutで2枝を再比較する。normal-rootは`primary / runner-up最大2`だけを比較し、同様にprimaryを保護する。holdoutが非正、生成失敗、1ケース4回のconfirmation上限到達なら保護方策へ戻す。

Rejectを消すと、v6でRejectがscreen首位になっていたturnの次点runner-upが新たにconfirmationへ進む場合がある。またRejectが消費していたconfirmation枠も後続runner-upへ回る。このためv7は単なるv6のReject出力だけをv3へ戻す版ではなく、Rejectを除いた候補集合を一貫して再比較する版である。

不要になった`RootActionKind::Reject`、`ArrivalStatus::RolloutRejected`、Reject枝のplan・owner・rollout、専用診断fieldとstderr keyを削除した。reserve/pushされるだけだった両rootの`alternative_evaluations`も削除した。action数はrescue-rootが2〜5、normal-rootが2〜3となるため、診断配列とhistogramもこの範囲へ合わせた。`AHC069_PROTECTED_ONLY`時は通常runner-upの収集自体をcompile-timeで除外し、診断上も評価していない枝を数えない。

新しい主要保存則は次の通り。

```text
root_actions_compared
  = rescue_rollout_turns
  + rescue_rollout_candidates_compared
  + root_alternatives_compared

normal_root_actions_compared
  = normal_root_rollout_turns
  + normal_root_alternatives_compared

root_confirmation_approved
  = root_selected_alternative
  + normal_root_selected_alternative

shadow_accepted = accepted
rejected
  = shadow_upper_rejected
  + shadow_actual_rejected
  + shadow_no_region_rejected
```

### 実行前検証

- `main.cpp` SHA-256: `b3f51f6ca44f49f0c002a992cab93f8919b87bfb89cb214e7dc4f47d3c9098af`
- Clang C++17 release binary SHA-256: `f5df08da8b4bdb9730e42675d32423e35f28b4cbf937136cff54f1382518c6bf`
- protected-only binary SHA-256: `11208c511bc89878a7e69f452c965d76aab79c5b3a4a75ab1f08e727a935b90a`
- Clang C++20 release binary SHA-256: `85f17e651b52d0af6ed8476259331be26eddcbb710c183cd8e4f262cc318cfa3`
- ASan/UBSan binary SHA-256: `f3aee1027a2b9782767d3898ada0f64b80ce9733dd220288a6207b5fa460d8b3`
- `ThetaEstimator::estimate`のBL相対addressを正規化したopcode列は旧v3/v6と同じSHA-256 `abb4e5640a4fd952141e898c3d94513e7467957ec268ab1fb5fd543359ad238b`
- Clang C++17通常/protected-only、Clang C++20 release build: pass、`-Wall -Wextra -Wshadow -Wpedantic`警告0件
- ASan/UBSan build: pass（解答は未実行）
- Clang static analyzer: 指摘0件
- `git diff --check`: pass
- 独立静的監査3系統: Reject参照の完全削除、protected復元、strict tie、runner-up順位、confirmation、状態独立、action-count境界、診断保存則を確認し、blocking issueなし

固定binaryは`/private/tmp/ahc069_cross_fitted_root_v7`、protected-onlyは`/private/tmp/ahc069_cross_fitted_root_v7_protected_only`、Pahcer設定は`pahcer/bench_cross_fitted_root_v7.toml`。最後にprotected-onlyと旧v3をseed 0〜99で比較し、通常v7も固定100ケースで一度だけ測定する。ここまでv7の解答プログラムは一度も実行していない。以降はsource/binaryを変更せず、結果確認後は`AGENTS.md`に従ってユーザーの次の明示指示まで解法・コード・memoを変更しない。

### `cross-fitted-root-v7` の100ケース結果と提出

protected-only binaryは旧v3固定binaryとseed 0〜99のstdoutが全てbyte単位で一致し、両者の合計scoreも`6,383,779,067`で一致した。Reject削除と診断整理の後も保護経路にドリフトはない。

通常binaryの固定100ケース結果は`pahcer/json/result_20260802_020343.json`。

- AC: `100/100`
- 合計score: `6,386,024,428`
- v3比: `+2,245,361`（15勝10敗75分、`+0.0352%`）
- v4比: `-632,512`（35勝40敗25分、`-0.0099%`）
- v5比: `+345,536`（44勝49敗7分）
- v6比: `+11,593,076`（40勝34敗26分）
- Pahcer時間: 平均`2.319 sec`、中央値`2.273 sec`、95 percentile約`3.130 sec`、最大`3.591 sec`

通常runner-upは合計27回、25 seedで最終採用された。rescue-rootだけで採用した2 seedは1勝1敗、合計`+690,475`、normal-rootだけで採用した23 seedは14勝9敗、合計`+1,554,886`だった。採用なしの75 seedは全て旧v3と完全同点である。confirmationは56回試行し27回承認、29回棄却、生成失敗と上限skipは0だった。ケースごとの新保存則24種、合計2,400項目を検査して違反0。

公式へ提出した結果は次の通り。

- 提出score: `3,409,424,714`
- 実行時間: `1,736 ms`
- 直前の黄パフォ提出`3,407,384,159`比: `+2,040,555`（約`+0.060%`）
- 順位、相対score、予測performanceは記録時点で未確認

ローカルではv4に僅かに届かないが、Rejectを含むv6の大幅悪化を回収し、runner-upだけの変更は旧v3比で正となった。公式scoreもこれまで報告された提出の中では最高だった。測定・提出後は解答コードを変更せず、ユーザーから記録とcommitの明示指示を受けて本節だけを追記した。

## 2026-08-02: cross-fitted rootの結果と保護経路ドリフト

`cross-fitted-root-v5`の固定binaryを100ケースで測定した結果は`pahcer/json/result_20260802_011403.json`。

- AC: `100/100`
- 合計score: `6,385,678,892`
- v4比: `-978,048`（`-0.0153%`、52勝44敗4分、差の中央値`+28,878`）
- v3比: `+1,899,825`（`+0.0298%`、52勝44敗4分）
- Pahcer時間: 平均`3.033 sec`、中央値`3.039 sec`、95 percentile約`3.983 sec`、最大`4.277 sec`
- confirmation試行254回、承認170回、棄却84回、上限skip 32回、生成失敗0
- 全254回がQ8/H12を完走し、scenario 2,032件、policy step 48,768

v4比の最大悪化はseed 12の`-5,539,675`で、これを除く合計は`+4,561,627`だった。ただしケースを除外した値で採用判断はせず、100 seedではv4と横ばいの微減と判断する。confirmation承認marginとv3比の実差はPearson相関`+0.087`、Spearman相関`+0.093`で、独立holdoutを使っても予測値の校正は弱かった。

結果分析で、confirmation承認0の16 seed中12 seedがv3と非同点で、合計差`-920,368`だった。seed 35、36、41、44、63、73、81はv4ではv3と完全同点なのにv5だけ変化している。特にseed 36はrescue rollout 0で、追加root判断を通らなくてもv3比`+653,520`だった。このため追加枝の効果とは別に、v4からv5で変更した共通経路に意図しないドリフトがあると判断した。

## 2026-08-02: theta集計を復元する互換修正 (`cross-fitted-root-v6`)

### 原因の切り分け

v3、v4、v5の固定Clang C++17 binaryを逆アセンブルして`ThetaEstimator::estimate`を比較した。v3とv4は粒子ごとに次の順で逐次集計する同一命令列だった。

```text
weight_sum += weight         // fadd
theta_sum += weight * theta  // fmadd
```

v5ではposterior分位を得るためweight計算を`make_posterior`へ共通化し、`estimate`は返された配列からtheta平均だけを計算していた。この小さい関数がcall siteへinlineされ、Clangは2要素の`fmul`と個別`fadd`へ変換した。数学的には同じでもFMA一丸めとはbit単位で一致せず、僅かなtheta差が通常配置のtie付近を変え、その後の盤面を連鎖的に分岐させ得る。

配置shortlistはprimary確定後にrunner-upをコピーするだけで、v4/v5 binaryもprimary確定までの浮動小数点命令列が一致した。screenのbatch 0はQ2/H4、pair 0、同じsequence indexとantithetic順で旧生成器と意味上同一である。枝評価はowner、groups、departure heapのコピー上で行われ、不採用枝が実状態を変更する経路もなかった。独立した静的監査3系統でも、Theta集計以外のblocking issueは見つからなかった。

### 修正

- `ThetaEstimator::estimate`だけをv3/v4の独立log-weight計算と単一逐次集計loopへ復元した
- posterior分位にだけ`make_posterior`を使い、Q8/H12 confirmationの仕様は維持した
- `AHC069_PROTECTED_ONLY`を定義した検証buildでは、rescue rootの通常runner-up/Reject比較をcompile-timeで除外し、normal-only rootも無効化する
- protected-onlyではQ2/H4で決めたv3 winnerだけを出力するため、旧v3固定binaryとのstdout完全一致を直接検査できる

旧v3 binaryと修正版の`estimate`を逆アセンブルし、外部関数への`BL`相対addressだけを正規化したopcode列はSHA-256 `abb4e5640a4fd952141e898c3d94513e7467957ec268ab1fb5fd543359ad238b`で完全一致した。

### 実行前検証

- `main.cpp` SHA-256: `8fc201ca525501a16f6ca47ace50099b8f96a77a00946c645dd9fe23ad048017`
- Clang C++17 release binary SHA-256: `0fbb7c9aacc29dc0c25dec95623dafecfc6c646e619c56ade894a2a83ba32b99`
- protected-only binary SHA-256: `801df855ffbdca2550d0c2067361aa6ea00c57896424b56bff1e2b6a88f20a81`
- Clang C++20 release binary SHA-256: `29a20134383a14653ae8099abad7476156f5cfc0d0ff484d590ef4f3d511b880`
- GCC C++17 release binary SHA-256: `cbb2a0d77614dab2891135ec17ee1ce0d028eacef178b1275c757c87f4f4b5c2`
- ASan/UBSan binary SHA-256: `4a8b8829e2d7e2c64fd8235c2411db59dfb8dd215cb3cf43c5c97251137ca7ab`
- Clang C++17通常/protected-only、Clang C++20、GCC C++17 release build: pass、`-Wall -Wextra -Wshadow -Wpedantic`警告0件
- ASan/UBSan build: pass（解答は未実行）
- Clang static analyzer: 指摘0件
- `git diff --check`: pass
- 独立静的監査3系統: 旧theta命令列、placement primary、batch 0、v3 strict tie、protected fallback、枝状態、heap、RNG非存在、normal gate停止を確認し、blocking issueなし

固定binaryは`/private/tmp/ahc069_cross_fitted_root_v6`、protected-onlyは`/private/tmp/ahc069_cross_fitted_root_v6_protected_only`、Pahcer設定は`pahcer/bench_cross_fitted_root_v6.toml`。最後に、protected-onlyと旧v3をseed 0〜99で同じtesterへ渡してstdoutを比較し、同時に通常v6を100ケース測定する。ここまで修正版の解答プログラムは一度も実行していない。以降はsource/binaryを変更せず、結果確認後は`AGENTS.md`に従ってユーザーの次の明示指示まで解法・コード・memoを変更しない。

### `expanded-root-v4` の100ケース結果

固定したsource/binaryの結果は`pahcer/json/result_20260802_003216.json`。

- AC: `100/100`
- 合計score: `6,386,656,940`
- v3比: `+2,877,873`（`+0.0451%`、37勝30敗33分）
- Pahcer時間: 平均`2.126 sec`、中央値`2.058 sec`、95 percentile約`2.806 sec`、最大`3.430 sec`
- rollout対象985回。Rescue 645回、v3 primary 187回、通常runner-up 7回、Reject 146回を選択
- v3勝者を追加枝が上書きしたのは153回、67 seed。上書きなしの33 seedは全てv3と完全同点
- 通常runner-upは373回利用可能で7回採用、Rejectは985回中146回採用
- root actionは合計4,134枝、3/4/5枝turnは116/559/310回
- scenario生成失敗、未来なしskip、validator失敗、beam node上限到達はいずれも0
- ケースごとの診断保存則63種、合計6,233項目を検査して違反0

追加枝がQ2/H4で予測した改善は合計`14,309,137`だったが、実改善は`2,877,873`で約20%だった。上書き67 seedにおける予測改善と実差のPearson相関は`-0.218`、Spearman相関は`-0.169`であり、予測値の大きさは実改善を順位付けできていない。通常runner-up単独採用は1 seedだけで改善、Rejectだけを使った61 seedは33勝28敗、合計`+3,038,391`だった。方向は小幅に正だが、Q2/H4で選んだ同じ標本をそのまま採用判定にも使うselection biasと短いhorizonが残る。

実行結果を確認した後は、ユーザーの次の明示指示まで解法・コード・memoを変更しなかった。

## 2026-08-02: 独立holdoutで確認する最終flat root (`cross-fitted-root-v5`)

ユーザーから残り段階を最後まで進める指示を受け、v4までの段階と最初のflat rollout構想との差を再確認した。元の最終像は通常配置3〜4件とReject、posterior由来8 scenario、約12到着、terminal評価だった。途中測定を挟むと`AGENTS.md`上そこで停止する必要があるため、残要素を一つの最終仕様へ統合し、全実装と静的検証後に100ケースを一度だけ測定する。

### 二段階root比較

Q2/H4の既存rolloutは安いscreenとして残す。rescueがあるturnでは、v3と同じ`primary / rescue 0 / rescue 1`の勝者を**保護方策**とする。新しい通常候補またはRejectがscreenで保護方策を厳密に上回った場合だけ、独立holdoutでその2枝を再比較する。rescueがない通常rootではprimaryを保護方策とする。

holdoutは次の仕様である。

- scenario数`8`、horizonは直後最大`12`到着
- 4組のantithetic pairを使い、screenとは異なるlow-discrepancy sequence blockを使う
- 各pairのlatent thetaは、現在の61粒子posteriorの`1/8, 3/8, 5/8, 7/8`分位から層別に選ぶ
- latent thetaは未来生成だけに使い、rollout方策には見せない。方策側は実mainと同じくsynthetic durationをobserveしてposterior meanを逐次更新する
- 保護方策とchallengerは同一scenarioを共有し、現在料金差・移動費は1回、未来12件の実現料金差だけをscenario平均する
- holdout平均差が厳密に正の場合だけchallengerを採用する。同点、生成失敗、confirmation上限到達時は保護方策へ戻す

これにより、screenに使った同じ2標本で候補選択と最終採用を兼ねるselection biasを避ける。Q8/H12を全root枝へ掛けず2枝だけへ使うため、最終像のscenario幅と深さを保ちながら計算量を抑える。

### root枝と通常turnへの限定拡張

通常配置は既存shortlistを再利用し、primaryに加えて同じ順位規則によるrunner-upを最大2件保持する。再列挙せず、同じセル集合は除く。rescue turnのscreen枝は最大

```text
primary + rescue最大2 + ordinary runner-up最大2 + Reject
```

の6件である。tie優先順は`primary > rescue 0 > rescue 1 > runner-up 0 > runner-up 1 > Reject`とする。

rescueが完成しなかった通常turnにもroot比較を限定的に広げる。発火条件は、通常shadow方策が受け入れ、primaryが理論最小周長より大きく、distinctなrunner-upが存在することとする。現在情報だけで判定し、到着順を4等分した各区間で最初の対象1回、1ケース最大4回だけscreenする。枝は`primary / runner-up最大2 / Reject`で、保護方策はprimaryである。壁時計や未来の実入力はgateに使わない。

confirmationはrescue/通常rootを合わせて1ケース最大4回とする。上限後にscreen challengerが出ても保護方策へ戻す。最大policy stepは、rescue screenが`2*4*6=48`、通常screenが`2*4*4=32`、confirmationが`8*12*2=192`である。

### terminalを判断へ入れない理由

旧`proactive-cleanup-v4`のshadow terminalは100ケースで良い移動と悪い移動を分離できず撤回済みである。容量だけのfluid valueはgeometryを見ず、既存compact-fitは無次元なので、恣意的係数なしに料金へ加えることもできない。巨大なtail近似を同時に入れると、独立holdout・posterior・H12の効果へ結果を帰属できなくなる。そのため最終段階ではterminalを判断値へ入れず、有限horizonの弱点は独立H12へ延長することで扱う。SE控除、worst scenario、rollout内rescueも追加しない。

### 失敗時挙動と診断

- screen scenario生成失敗または残り0件では、rescue turnは従来どおり合法・即時利益正のcandidate 0、通常rootはprimaryを使う
- holdout生成失敗、同点、confirmation上限では保護方策を使う
- root seedとposterior theta層はactionに依存させず、全枝でCRNを守る
- screen上書き、confirmation試行・承認・棄却・上限skip、通常root発火、runner-up順位、scenario別差、policy stepを別診断にする
- 最終出力に対するrescue、通常runner-up、Reject、shadow acceptedの保存則をケースごとに検証可能にする

ここから先は仕様を変えずに実装と静的検証を行い、最後に固定binaryを100 seedで一度だけ実行する。

### 実装

- `ThetaEstimator`の61粒子weight計算を共通化し、同じ離散posteriorの逆CDFからtheta分位を取得できるようにした。通常の`estimate`の尤度、加算順、posterior meanは維持した。
- scenario生成器を動的Q/Hへ一般化した。batch 0・pair 0はv4と同じindex列と`u,1-u`の順を維持し、confirmationのbatch 1はpairごとに独立なsequence blockを使う。
- placement shortlistからprimaryと同じ順位規則でrunner-upを最大2件取得する。primaryの選択順とsynthetic rollout内の配置は変更しない。
- rescue rootではQ2/H4で厳密なv3勝者を先に確定し、その後にrunner-up 0、runner-up 1、Rejectをstrict順で比較する。追加枝が勝った場合だけ`confirm_root_override`へ進む。
- `confirm_root_override`はprotected/challengerの`plan + final_owner + baseline比direct`を受け取り、独立posterior Q8/H12の2枝だけをCRNで評価する。`8*direct差 + future差合計 > 0`のみ承認する。
- rescueが完成しなかったnormal turn用に`choose_normal_root_action`を追加した。非最小周長かつrunner-upありの各時間四分位最初の1回だけ、primary/runner-up最大2/Rejectをscreenする。
- confirmationはrescue/normal共通で1ケース最大4回。生成失敗を含む試行は枠を消費し、上限後はprotectedへ戻す。
- placement診断は最終選択のsourceへ置換し、Reject時は通常配置成功を除く。shadowの`rollout_rejected`は通常shadow acceptedの部分集合として維持する。
- screenと最終上書き、枝種別、confirmation試行/承認/棄却/失敗/上限、full/short H、8 scenario正判定数、pair不一致、policy stepを分離して出力する。

### 実行前検証

- `main.cpp` SHA-256: `ab32f5b84dd9abb3e96c542917205437fc6c3f61266638a9e69e0d8aa98a34bd`
- Clang C++17 release binary SHA-256: `02a4d0a48896b41b55703fb5fbfed5555a100c07756b7381085737e6b8edb020`
- Clang C++20 release binary SHA-256: `ad8c99821a12080256033d37367304854d02b45752436e9bcbc1c170b12b9929`
- GCC C++17 release binary SHA-256: `414650bc146db13fec16855e0df769cca89668f3d004584783bbf33740d73b33`
- ASan/UBSan binary SHA-256: `b2ddd086be434d6f1b35e0e961af7a39a02ed87e36dcec54a40d31b2c5bd63fe`
- Clang C++17/C++20、GCC C++17 release build: pass、`-Wall -Wextra -Wshadow -Wpedantic`警告0件
- ASan/UBSan build: pass（解答は未実行）
- Clang static analyzer: 指摘0件
- `git diff --check`: pass
- 独立静的監査3系統: v3 protected、6枝tie、normal gate、posterior分位、screen互換、sequence独立、全件生成、latent theta非漏洩、CRN、枝状態、料金・移動費の一重計上、confirmation fallback、配列境界、shadow/placement診断と保存則を確認し、blocking issueなし

Pahcer設定は`pahcer/bench_cross_fitted_root_v5.toml`、固定binaryは`/private/tmp/ahc069_cross_fitted_root_v5`。主比較対象はv4 `6,386,656,940`。実行コマンドは次の通り。

```text
pahcer run --setting-file pahcer/bench_cross_fitted_root_v5.toml --no-compile --freeze-best-scores --comment cross-fitted-root-v5-screen-q2h4-confirm-q8h12
```

ここまで固定したrelease/sanitizer binaryは一度も実行していない。以降はsource/binaryを変更せず、seed 0〜99の100ケースを1回だけ測定し、結果確認後は`AGENTS.md`に従ってユーザーの次の明示指示まで解法・コード・memoを変更しない。

### `compact-rescue-v1` の100ケース結果

固定したsource/binaryの結果は`pahcer/json/result_20260801_233026.json`。

- AC: `100/100`
- 合計score: `6,374,028,401`
- v14比: `+15,835,346`（改善55、悪化39、同点6）
- 再配置なし`placement-fit-v1`比: `+22,802,982`（改善53、悪化41、同点6）
- rescue成功904回、移動組1,219組
- 到着料金改善`48,668,113`、移動費`4,164,261`、即時利益`44,503,852`
- 1 blocker 642回、2 blocker 216回、3 blocker 40回、4 blocker以上6回
- validator失敗0、beam node上限到達0
- Pahcer時間: 中央値`1.894 sec`、95番目`2.427 sec`、最大`2.757 sec`

再配置なし比の最終増分`22,802,982`は即時利益`44,503,852`より`21,700,870`小さい。即時には必ず得をするconstructive rescueでも、その後の配置連鎖で約半分が失われているため、次段では候補生成を変えず、短い共通未来で現行v1候補を採否する。

## 2026-08-01: 直後4到着のflat rollout filter (`compact-rescue-rollout-v2`)

次段で追加する要素をfuture filter一つに限定した。root actionは次の2本だけである。

1. 通常shadow方策が選んだordinary connected-growth fallback
2. `compact-rescue-v1`が最初にrepair・validatorを通したrescue 1件

Reject、2件目のrescue、target/destination候補範囲、fee-neutral条件、repair、validatorは変更しない。rolloutで最初のrescueを棄却した場合も後続targetは探さず、ordinary fallbackへ戻る。これによりv1との差はfuture filterの有無だけになる。

### 共通scenario

- scenario数: `2`（1組のantithetic pair）
- horizon: 時刻によらず直後の最大`4`到着
- latent theta: 新たにsampleせず、現時点のposterior meanを使用
- future policy: 通常のshadow admissionと時間整合配置
- rollout内rescue: なし
- terminal value、worst-scenario、SE控除、pre-arrival gate: なし

各scenarioは現在時刻より後という条件付き分布から残り全組を生成し、`S`でsortして先頭4件を使う。最初から4件だけ生成すると「残り全組中の次の4件」にならないため、全残り組を試行上限内で生成できたことを必須とする。未完なら部分標本を使わず、future filterを省略して合法・即時利益正のv1 rescueを維持する。

scenario生成seedは現在観測可能なarrival ID、`S`、`V`だけから決まり、root actionを含めない。同じscenario vectorをfallback/rescue両枝へ渡す。scenario内では仮想到着のdurationをobserveしてから`theta`を再推定する。

### rollout状態と評価

各root後の状態は`owner`、`groups`、departure heapを複製する。rescue枝では全moverの`cells`と`max_perimeter`、現在到着組の`active`、`cells`、`max_perimeter`をplanと同期する。仮想組は元のサイズ`M`より後ろへappendし、退去は実方策と同じく`t < S`だけを処理する。

rolloutが返すのは後続4組から新たに得た料金だけで、既存active組や現在到着組の料金を再加算しない。scenario `q`について

```text
future_delta[q]
  = rescue future fee[q] - fallback future fee[q]

direct_gain
  = compact arrival fee - fallback arrival fee - sum(move cost)

margin
  = direct_gain + (future_delta[0] + future_delta[1]) / 2
```

とする。既存組の料金低下はv1同様0必須である。`margin > 0`ならrescue、`margin <= 0`なら同点を含めfallbackを選ぶ。shadow opportunity costや移動費をrollout側で再控除しない。残り組数0またはscenario生成失敗なら即時利益版へ戻る。

診断は完成planと実採用を分け、feasible数、rollout採用・棄却、scenario不一致、policy step、両枝の仮想受入数、scenario別future差、採用・棄却別のdirect/future/marginを出力する。

### 実行前検証

- `main.cpp` SHA-256: `5302fecce2c7588d829f5dd22a87b63e30d4255c85c54560884b8623345646e6`
- Clang C++17 release binary SHA-256: `26c8fb09ca0a18da8f94a3c9249eda5afcb8f93321d6b658dbdc1a468be4aef8`
- Clang C++20 release binary SHA-256: `3c48061491da2613cd9578a9fb708d2912f43b24c90cbc5566d5c50c98cd20a2`
- ASan/UBSan binary SHA-256: `77359e2f74df84306dabf2472d4d154f3f9b051aa35769c7e2e857e215a3e3fa`
- Clang C++17/C++20 release build: pass、警告0件
- GCC C++17 syntax/警告検査: pass、警告0件
- ASan/UBSan build: pass（解答は未実行）
- Clang static analyzer: 指摘0件
- `git diff --check`: pass
- 独立静的監査3系統: CRN、scenario全件生成、状態同期、synthetic ID・参照寿命、退去境界、終盤、料金の一重計上、fallbackを確認し、blocking issueなし

Pahcer設定は`pahcer/bench_compact_rescue_rollout_v2.toml`、固定binaryは`/private/tmp/ahc069_compact_rescue_rollout_v2`。主比較対象はv1 `6,374,028,401`。実行コマンドは次の通り。

```text
pahcer run --setting-file pahcer/bench_compact_rescue_rollout_v2.toml --no-compile --freeze-best-scores --comment compact-rescue-rollout-v2-q2-h4-mean
```

ここまで固定したrelease/sanitizer binaryは一度も実行していない。以降はsource/binaryを変更せず、seed 0〜99の100ケースを1回だけ測定し、AGENTS.mdに従って結果確認後はユーザーの次の明示指示まで解法・コード・メモを変更しない。

### `compact-rescue-rollout-v2` の100ケース結果

固定したsource/binaryの結果は`pahcer/json/result_20260801_234816.json`。

- AC: `100/100`
- 合計score: `6,378,221,834`
- `compact-rescue-v1`比: `+4,193,433`（改善36、悪化46、同点18）
- v14比: `+20,028,779`
- 再配置なし`placement-fit-v1`比: `+26,996,415`
- rollout対象925回、採用653回、棄却272回、2 scenarioの判断不一致241回
- 実採用したrescueの即時利益: `37,384,126`
- Pahcer時間: 中央値`1.976 sec`、95番目`2.520 sec`、最大`2.900 sec`

v1に比べて即時利益を`7,119,726`手放した一方、その後の配置連鎖による損失を約`11.313m`回収し、最終的に`+4,193,433`となった。短い共通未来で採否する方向自体には効果がある。

ただし、filterが予測した改善は合計約`19.648m`で、実改善`4.193m`よりかなり大きい。ケース単位の予測差と実差のPearson相関も`-0.089`であり、予測値の校正は弱い。次段ではQ=2、H=4、future policyを変えず、rootで比較するrescue候補の幅だけを1から最大2へ広げる。

## 2026-08-01: rescue root候補を最大2件へ拡張 (`compact-rescue-rollout-width2-v3`)

v2との差を「同じ短期未来で比較するrescue候補数」だけに限定する。

### 候補生成

- candidate 0は、v2が既存のtarget順・repair・validatorで最初に得るplanそのもの
- candidate 1は、その続きから同じ順序で次にvalidatorを通る正の即時利益plan
- 1 targetにつきrepairが返すplanは従来どおり1件
- target最大8件、beam node 2,048、全target共有anchor 50,000は変更も再初期化もしない
- target領域は`make_rescue_targets`で完全重複除去済みなので、candidate 0と1の到着領域は異なる
- blocker集合、target間の重なり、多様性bonusによる選別は行わない

candidate 0を得た時点で残り組数が0なら、candidate 1を探さずcandidate 0を採用する。共通scenarioを完全生成できなかった場合も同様である。これによりfuture filterを使えない場合はv1/v2のfallback挙動を維持する。

### 共通rolloutと選択

scenario生成に成功した場合だけ、残ったwork limitでcandidate 1を探す。通常fallbackのroot状態はscenarioごとに一度だけ評価し、全candidateが同じ2 scenarioを共有する。各candidate枝は、保存した`plan + final_owner`と元の`groups`から独立に構築する。

candidate `i`について

```text
delta[i][q] = candidate_i future fee[q] - fallback future fee[q]
margin2[i]  = 2 * direct_gain[i] + delta[i][0] + delta[i][1]
```

とする。`margin2`が最大かつ正のcandidateを採用し、全candidateが0以下なら通常fallbackへ戻す。candidate同士の同点は先に見つけたcandidate 0、0点でfallbackと同点ならfallbackを選ぶ。

Q=2、H=4、antithetic pair、posterior mean、通常shadow future policy、rollout内rescueなし、terminal valueなしはv2から変更しない。候補数を`K`、実horizonを`H'`とすると、baseline共有後のpolicy stepは

```text
2 * H' * (1 + K)
```

で、最大はK=1なら16、K=2なら24となる。Reject、SE控除、worst scenario、多様性加点、theta samplingは追加しない。

診断ではturn単位のfeasible/successとcandidate単位のfeasible planを分離し、1候補/2候補turn数、candidate 0/1の選択数、比較candidate総数、slot別future差、幅2による予測上の追加利益、同一blocker集合数、target重複マス数を記録する。

### 実行前検証

- `main.cpp` SHA-256: `404365aaf672332ef0de353714817329fbbd98c86a2587707db40936dfe53edd`
- Clang C++17 release binary SHA-256: `6e9edf6312defc5b103d656e40dd40a1336fc739333e28f6e26535649f6d067e`
- Clang C++20 release binary SHA-256: `ac9742caaa98dcccae1f8387417d4cae3fe58f675d02faa7fc52d608ccb84fa5`
- ASan/UBSan binary SHA-256: `a0ff74f44b49ed8689d27253dcdb391f86ea96a56371caf996df812dfcb49430`
- Clang C++17/C++20 release build: pass、警告0件
- GCC C++17 syntax/警告検査: pass、警告0件
- ASan/UBSan build: pass（解答は未実行）
- Clang static analyzer: 指摘0件
- `git diff --check`: pass
- 独立静的監査3系統: candidate 0のv2同一性、候補間の状態独立、CRN、baseline共有、料金の一重計上、同点規則、参照寿命、work limit、診断不変条件を確認し、blocking issueなし

Pahcer設定は`pahcer/bench_compact_rescue_rollout_width2_v3.toml`、固定binaryは`/private/tmp/ahc069_compact_rescue_rollout_width2_v3`。主比較対象はv2 `6,378,221,834`、副比較対象はv1 `6,374,028,401`。実行コマンドは次の通り。

```text
pahcer run --setting-file pahcer/bench_compact_rescue_rollout_width2_v3.toml --no-compile --freeze-best-scores --comment compact-rescue-rollout-width2-v3-q2-h4
```

ここまで固定したrelease/sanitizer binaryは一度も実行していない。以降はsource/binaryを変更せず、seed 0〜99の100ケースを1回だけ測定し、AGENTS.mdに従って結果確認後はユーザーの次の明示指示まで解法・コード・メモを変更しない。

### `compact-rescue-rollout-width2-v3` の100ケース結果

固定したsource/binaryの結果は`pahcer/json/result_20260802_000746.json`。

- AC: `100/100`
- 合計score: `6,383,779,067`
- v2比: `+5,557,233`（改善24、悪化16、同点60）
- v1比: `+9,750,666`
- 再配置なし`placement-fit-v1`比: `+32,553,648`
- rollout対象948回、rescue採用687回、通常fallback 261回
- 2 rescue候補を比較できたturn 767回、candidate 1選択67回
- 実採用したrescueの即時利益: `38,955,347`
- validator失敗、scenario生成失敗、beam node上限到達はいずれも0
- Pahcer時間: 中央値`2.006 sec`、95番目`2.411 sec`、最大`2.611 sec`

候補幅2が予測した追加利益は合計`1,161,747.5`だったが、v2からの実増分は`5,557,233`だった。幅追加で出力が変わった40 seedは24勝16敗である。一方、ケース単位の予測追加利益と実差のPearson相関は`-0.097`で、短期値の校正は引き続き弱い。

候補1件のfilterから候補2件のroot比較へ広げる方向には連続して改善が出た。次段ではrescue候補生成とQ=2/H=4を固定し、同じrollout turnに通常配置runner-upとRejectを追加する。

## 2026-08-02: 通常runner-upとRejectを含むexpanded root (`expanded-root-v4`)

v3で少なくとも1件のrescueがvalidatorを通り、共通scenarioを完全生成できたturnだけを対象とする。rescueがないturnまでrolloutを広げると発火範囲と計算量が同時に変わるため、今回は行わない。

### root action

比較する枝は最大5件である。

1. ordinary primary（v3のbaseline）
2. rescue candidate 0
3. rescue candidate 1（存在する場合）
4. ordinary runner-up 1件（存在する場合）
5. Reject

通常runner-upは、既存`PlacementShortlistBuilder`が一度の通常配置探索で作った最大6件のshortlistから取り出す。primaryを除外し、future-fitを使ったturnでは同じfit最大・`placement_increment_less` tie-break、使わないturnでは同じincremental順位を再適用した2位を選ぶ。再列挙、新しい多様性指標、追加の盤面評価は行わず、primaryの選択・浮動小数点比較順・配置診断を維持する。

### 共通評価

ordinary primaryを0とした相対値は次の通り。

```text
direct[primary]     = 0
direct[alternative] = alternative fee - primary fee
direct[rescue_i]    = compact fee - move cost - primary fee
direct[Reject]      = -primary fee

delta[action][q]
  = action future fee[q] - primary future fee[q]

score2[action]
  = 2 * direct[action] + delta[action][0] + delta[action][1]
```

Q=2、H=4、scenario、posterior mean、future policy、rescue candidate順とwork limitはv3から変更しない。primary未来はscenarioごとに1回だけ評価し、全actionが同じscenarioとtheta更新列を共有する。

Reject枝は現在料金0、元の盤面、移動なし、現在組inactive、departure追加なしとする。現在の`shadow.opportunity_cost`は残り未来全体の推定で、H=4の実現差と重複するためRejectへ加点しない。したがってH=4より後のReject利益を拾わない保守的な比較であり、terminal valueはまだ導入しない。

tie優先順は

```text
primary > rescue 0 > rescue 1 > ordinary runner-up > Reject
```

とし、追加actionがv3勝者を厳密に上回る場合だけ出力を変更する。残り組数0またはscenario生成失敗時はv3同様rescue 0を採用する。

rescue候補数を`K`、通常runner-up有無を`A`、実horizonを`H'`とすると、policy stepは

```text
2 * H' * (2 + K + A)
```

で、最大40となる。診断にはaction数別turn、各actionの選択数、runner-up/Rejectのdirect・future差・margin、v3勝者の上書き回数、expanded rootの予測追加利益を記録する。通常shadowの提案状態とrootによるReject上書きも別カウンタにする。

### 実行前検証

- `main.cpp` SHA-256: `455683a792ed6525b6d11fe237cbe3e20dd3b7bb321e81c30c7245337f1b6f89`
- Clang C++17 release binary SHA-256: `09c5359656a8d2ddf91220e63bdf0432135cead7e333f2da9bd27452f6c49452`
- Clang C++20 release binary SHA-256: `37cf3d9f0089e2ed6b7f1cd8e74cb86dd7c953b3462086a3e321c43520653a15`
- ASan/UBSan binary SHA-256: `1d8bbfc908286d4793d5ea71257783035f233219305ffbef29c64655182fa893`
- Clang C++17/C++20 release build: pass、警告0件
- GCC C++17 syntax/警告検査: pass、警告0件
- ASan/UBSan build: pass（解答は未実行）
- Clang static analyzer: 指摘0件
- `git diff --check`: pass
- 独立静的監査3系統: primary/runner-up順位、v3互換性、全root枝の状態・heap、CRN、料金の一重計上、Reject出力、tie、参照寿命、shadow/placement診断、policy stepと保存則を確認し、blocking issueなし

Pahcer設定は`pahcer/bench_expanded_root_v4.toml`、固定binaryは`/private/tmp/ahc069_expanded_root_v4`。主比較対象はv3 `6,383,779,067`。実行コマンドは次の通り。

```text
pahcer run --setting-file pahcer/bench_expanded_root_v4.toml --no-compile --freeze-best-scores --comment expanded-root-v4-alt1-reject-q2-h4
```

ここまで固定したrelease/sanitizer binaryは一度も実行していない。以降はsource/binaryを変更せず、seed 0〜99の100ケースを1回だけ測定し、AGENTS.mdに従って結果確認後はユーザーの次の明示指示まで解法・コード・メモを変更しない。

