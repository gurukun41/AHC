## 2026-08-01: target-first bounded LNS 再配置 (`relocation-lns-v2`)

`placement-fit-v1` 提出後のユーザー指示により、過去AHCの局所破壊・再構築、共同repair、安い上界から厳密評価へ進む方式を調査し、再配置を新しく実装した。開催中のAHC069参加者の解法は調査対象にしていない。

旧1組再配置は、安い移動組を先に選び、その跡地へ到着組を置き、移動組をfirst-fitで戻していた。また再配置経由の受け入れがshadow-priceを通らず、100ケースで局所見積利益が正でも`admission-v4`比`-0.7103%`だった。今回は旧関数の有効化ではなく、到着領域を先に決める別探索へ置き換えた。

### 発動条件と損益

- 理論最小周長料金がshadow機会費用を上回った後、通常配置が存在しない場合だけ呼ぶ。
- 現在の総空き面積が`P_i`未満なら、再配置しても面積は増えないため即終了する。
- 最安移動費を引いた理論料金でもshadow機会費用以下なら、target列挙前に終了する。
- 完成計画は

```text
arrival payment - sum(move cost) > shadow opportunity cost
```

  を必須とする。
- 移動組について、移動前後の丸め後利用料金が完全に同じ候補だけを許可する。過去最大周長は回復しないため、良形へ戻すことによる料金増加は利益に数えない。
- 通常配置可能時、実料金によるadmission拒否時、未来改善だけを目的とする予防的再配置は今回の対象外とする。

### target-first候補

- 到着組の`Lmin`から`Lmin+4`までのcompact templateを、既存占有との重なりを許して配置する。
- target内で重なる相異なるactive組をblockerとし、1組または2組だけの候補を残す。
- blocker合計人数は到着人数の2倍以下に制限する。
- target料金から全blockerの移動費を引いた楽観値がshadow機会費用以下ならrepairしない。
- 同じblocker集合から最大4候補、全体で最大12候補を保持し、実repairは上位最大4候補に固定する。
- 全anchor無制限走査は行わず、1回最大4096 anchor・12万cellを、shape間でround-robinに分配した決定的samplingで調べる。

### 最大2組のjoint repair

1. blocker全組の旧領域をscratch盤面から同時に消す。
2. 到着targetを実際の到着IDで予約する。
3. 各blockerの移動先poolを同じbase盤面から独立に作る。
4. 2組の場合はpoolの直積を調べ、重ならない組み合わせを完成計画にする。直接pairがない場合のみ両配置順を1候補ずつ試すconditional repairを行う。

移動先は四隅first-fitではなく、料金低下0のcompact templateを1探索最大8000 anchor調べる。退去時刻levelの境界コストを2次元prefixでO(1)評価し、全体上位3件、各四象限の最良、旧領域再利用最大を合わせて最大8候補残す。compact候補がない場合だけmulti-start connected-growthを使う。

2 blockerが隣接する場合は、独立評価で生じる

```text
level(a) + level(b) - abs(level(a)-level(b))
```

の余分な共有辺コストを差し引き、完成配置の時間境界値へ補正する。

### 完成計画の選択と安全策

- 金銭価値`arrival payment - move costs`を最優先し、同額の完成計画間だけ既存の3断面future compact-fitを使う。
- future-fitには、全移動先と到着targetを置いた完成scratch盤面を渡す。無次元utilityで金銭的に劣る計画を逆転させない。
- validatorが全移動元clear、移動先place、到着placeをcheckerと同じ順序で再現する。
- active/ID重複、セル数、範囲、芝生、重複、4連結、周長、既存料金不悪化を再検査し、完全に合法な計画だけを実盤面へcommitする。
- 再配置では既存組の退去heapを変更しない。`apply_plan`は全移動元を消してから全移動先を置き、`max_perimeter`だけを単調更新する。
- 壁時計によって後続の通常配置評価経路を変えない。ケース全体でtarget探索64回、destination探索192回、connected-growth探索32回を上限とし、予算切れは移動なしへfail-closedする。

診断には、再配置試行・成功、1/2 blocker別成功、target/destination走査量、pair数、future tie変更、validator失敗、移動費、shadow控除後保証利益を追加した。

この節の記録時点では解答を一度も実行していない。実行前にコンパイラ警告、static analyzer、独立仕様監査を完了させ、最後に固定したコードを測定する。

実行前検証結果:

- `main.cpp` SHA-256: `0490ae358da92a2305a3a5c387b935dd1f228a96041c41d9564191c27b9ba2c1`
- Clang C++17 release binary SHA-256: `bcfdb9791d7e4989bc3c9a96a7dd3f22e46df585b3b8dd241dc012fa0e8714f3`
- ASan/UBSan binary SHA-256: `2a8a266005d40d32479c9be0ea370ae1cd8bf4b08cf1b81b6af9dc92aaf51751`
- Clang C++17 release build: pass、`-Wall -Wextra -Wshadow -Wpedantic`警告0件
- GCC C++20 syntax build: pass、警告0件
- Clang static analyzer: pass、指摘0件
- `git diff --check`: pass
- 独立静的監査3系統: 公式同時移動仕様、owner/groups/heap、丸め料金、shadow控除、target/repair候補、共同境界補正、完成盤面future-fit、固定計算量上限を確認し、blocking issueなし

ここまで解答プログラムおよびsanitizer binaryは未実行である。

### `relocation-lns-v2` の100ケース結果

上記のsource/binaryを固定したままseed 0〜99を1回実行した。

- AC: `100/100`
- 合計score: `6,352,312,849`
- 比較元`placement-fit-v1`: `6,351,225,419`
- 差: `+1,087,430`、`+0.0171216%`
- seed別: 改善17、悪化13、同点70
- 最大改善: seed 5、`+6,106,398`（`+7.104%`）
- 最大悪化: seed 24、`-3,888,624`（`-4.633%`）
- 時間: 中央値`1.532 sec`、95 percentile `2.000 sec`、最大`2.257 sec`、2秒超6ケース
- 再配置: 30ケース43turn、移動72組。1 blocker成功14、2 blocker成功29
- validator失敗: 0

再配置が起きた30ケースだけが非同点で、局所的なshadow控除後保証利益は合計約`7.78M`だった一方、最終score差は約`1.09M`に留まった。2 blockerだけ成功した20ケースは11勝9敗、差`+775,752`だった。したがって再配置自体は小幅に有効だが、局所利益のかなりの部分を後続盤面で失っている。またtarget・destination・growthの固定予算が多くのケースで上限に達し、計算時間にも余裕が小さい。

この結果を確認した時点では、規約どおりコード・方針を変更せず停止した。以降の変更は、ユーザーから「1〜2組だけに絞ったのが良くないかもしれない」「やってみてください」という新しい明示指示を受けて開始した。

## 2026-08-01: 最大3組の段階的再配置 (`relocation-lns-v3`)

`relocation-lns-v2`の1〜2 blocker探索を先に保ち、それで完成計画を1件も作れなかった場合だけ3 blockerを試す。3組を単純に全anchorで最後まで読むと、旧版が3組目を見た時点で打ち切っていた密集anchorのcell消費が増え、1〜2組候補の探索量を削る。そのためtarget探索を二段階にした。

### triple seedと完全検証

- 通常target走査は旧版と同じく、第3の異なるownerを見つけたそのセルで打ち切る。
- 破棄する代わりに`shape/anchor/3 ID/楽観cash`をcheap seedとして保存する。
- seed発見用の列挙順は1〜2 blockerの列挙順と分離し、完全同値時のtie順にも影響させない。
- 同一blocker集合は最大4件、全体は最大32件だけ保持する。
- 1〜2 blockerのrepairが全滅した時だけ最大32 seedを全セル再走査し、池を含まず、ownerが厳密に3組で、保存したID集合と一致するものだけをtarget化する。
- 完全検証後にblocker集合ごと上位4件、全体上位8件へ絞る。未検証上位seedが4組目や池で無効でも、下位の有効seedへ補充できる順序にする。
- seed完全検証はケース全体最大8回、追加確認cellは最大`8 * 32 * 150 = 38,400`に固定する。

1〜2 blockerのtarget走査・shortlist・repairを先に行い、成功した場合は3 blockerを比較対象にしない。これは既存挙動を優先するための段階的拡張であり、1〜2組案より高収益な3組案を比較しない場合がある。

### 3組の採算条件

既存の`blocker合計人数 <= 2 * arrival P`は変更せず、組数上限だけの効果を分離する。全移動組は移動後も丸め後利用料金が完全に同じ候補だけを許可する。

3 blockerには通常の厳密採算条件より強い安全marginとして、

```text
cash_after_moves = arrival_fee - total_move_cost
cash_after_moves > shadow opportunity_cost + total_move_cost
```

を要求する。すなわち`arrival_fee > opportunity_cost + 2 * total_move_cost`であり、shadow控除後利益がさらに移動費1回分を上回るtargetだけを扱う。

### 3組joint repair

1. 3 blockerの旧領域をscratch盤面からすべて消す。
2. arrival targetを予約する。
3. 同じbase盤面から各blockerの料金低下0の移動先poolを作り、時間境界値順の上位6件を残す。
4. 3 poolの各組合せを最大`6^3 = 216`通り完全列挙し、3組の全ペアが非重複な組合せだけを完成計画にする。

候補数6では全直積216通りの方が、複数順序のbeamより計算量が小さく候補落ちもないため、幅制限beamから完全列挙へ確定した。時間境界値は各移動先の単独値を足し、3つの非順序pairすべてについて共有辺の余分な値

```text
level(a) + level(b) - abs(level(a) - level(b))
```

を引く。3体固有の補正は不要である。完成組合せは上位12件を正確に再評価し、既存のplan上限4件とfuture-fit同点比較へ渡す。

3 blocker repairは1到着最大2 target、ケース全体最大8 targetなので、追加destination探索は最大24回、組合せ確認は最大1,728通りである。3組ではconditionalな移動先再生成を行わない。target、destination、growthの既存ケース全体予算も維持し、予算不足時は移動なしへfail-closedする。

plan生成時にmove数とsort済みID集合がtargetの3 blockerと完全一致することを確認する。最終validatorは最大move数3を明示し、全移動元clear、全移動先、arrivalの順に、active/ID重複/セル数/範囲/芝生/占有/4連結/周長/既存料金/全移動費/immediate gainを再検査する。`apply_plan`と出力は元から可変move数なので、同じ全clear→全place順で3組の交換・循環移動も扱える。

### 実行前検証

- `main.cpp` SHA-256: `54f9da884ac0e4c10f18041cbc8b838f47b0d6d339fe25145b185816d54282e0`
- release binary SHA-256: `187bcc624f9ec115267714c1b6baa89e0c278042e56992cf2c51e549bf758965`
- ASan/UBSan binary SHA-256: `4252236466c10c37aec2e58eadb32286e2ec467c464836e262bb600848035a43`
- Clang C++17 release build: pass、`-Wall -Wextra -Wshadow -Wpedantic`警告0件
- GCC C++20 syntax build: pass、警告0件
- ASan/UBSan build: pass（解答は未実行）
- Clang static analyzer: pass、指摘0件
- `git diff --check`: pass
- 独立静的監査3系統: triple seed早期打切りと完全検証、fixed-base key、3 pool完全直積、全pair共有辺補正、move ID完全性、validator/apply/output、固定計算量上限を確認し、blocking issueなし

ここまで解答プログラムおよびsanitizer binaryは未実行である。次にこのsource/binaryを変更せず、seed 0〜99を1回だけ測定する。

### `relocation-lns-v3` の100ケース結果

上記のsource/binaryを固定したままseed 0〜99を1回実行した。結果ファイルは`pahcer/json/result_20260801_143908.json`。

- AC: `100/100`
- 合計score: `6,351,096,024`
- 比較元`relocation-lns-v2`: `6,352,312,849`
- 差: `-1,216,825`、`-0.01916%`
- seed別: 改善2、悪化2、同点96
- 3 blocker成功: 1回のみ。seed 17で発動し、このseedの最終score差は`-406,892`

3組対応はほぼ発動せず、発動した1件も改善へ結び付かなかった。到着組を置けるようにするtarget-first再配置をさらに広げるより、既に受け入れている組を整理して将来の空間を改善する方針へ切り替える。

この結果を確認した時点では規約どおりコードを変更せず停止した。以降の変更は、ユーザーから既存組の整理を目的にし、空間改善・周長履歴の悪化・移動費を同時に評価するという新しい明示指示を受けて開始した。

## 2026-08-01: 退去跡を使う予防的整理 (`proactive-cleanup-v1`)

### 方針変更

到着組が置けないときに1〜3組を動かす`try_target_first_relocation`はmainの実行経路から外した。通常配置が存在しない到着は、そのまま`No`にする。

新しい再配置は、直前に退去した組の跡へ利用中の1組を移し、その組の旧領域を将来の到着に使いやすい空間へ変換する「departure-hole exchange」とする。現在到着の受入可否と配置領域は通常方策で先に固定し、整理候補間で変更しない。したがって、再配置が現在到着を救済した効果を空間改善へ混ぜない。通常配置が`Yes`なら`move + Yes`、拒否なら`move + No`のどちらも扱う。

コンパクトな場所へ移して現在組の周長が短くなっても、過去の最大周長は回復せず利用料は増えない。この架空の料金改善は利益へ数えない。

### 退去跡と移動候補

- 各到着の退去処理で、`T_j<S_i`の組のセルを解放前に`fresh_mask`へ保存する。
- 移動先は人数別のcompact template（理論最小周長から`+4`まで）に限定する。
- 移動先の少なくとも`ceil(3P_j/4)`セルが、そのターンの`fresh_mask`に含まれることを要求する。これにより既存の大きな空地へ単に移す候補を除く。
- 同一領域への移動を禁止し、移動先は現在到着の固定領域とも重ならない。
- 1ターンで移動するのは1組、同じ組の予防的移動はケース中1回まで。
- 移動元は、旧領域を解放したときに隣接空き成分を統合する量を`merged_gain`とし、

```text
merged_gain * (T_j - S_i) / move_cost_j
```

  が大きい順に最大8組を調べる。形状の理論最小周長からの超過、解放後成分サイズ、単位人数当たり移動費を後段のtie-breakに使う。
- 各移動元では移動費・料金低下が小さい候補、fresh重複が大きい候補、異なる象限の候補を残し、完成盤面を最大3件評価する。

### 整理後スペースの金銭評価

移動組`j`が退去すれば整理前後の盤面差は消えるため、未知の未来到着が`S_i`から`T_j`までに始まる確率質量を、既存の条件付き未来需要モデルから求める。その質量の`1/6, 1/2, 5/6`分位に対応する3時点で、現在到着の固定判断まで反映したbaseline盤面と移動後盤面を対称に比較する。

各断面では辺長

```text
s in {2,3,4,5,6,8,10,12}
```

について、次を計算する。

- 空き`s*s`正方形anchor数`C_s`
- 空き連結成分ごとの大きさから得る、代表人数`s^2`を収容できるslot数`min(2, sum floor(component_size/s^2))`

サイズ別の空間availabilityは

```text
0.8 * log(1+C_s) / log(1+(N-s+1)^2)
+ 0.2 * component_slots / 2
```

とする。正方形anchorでコンパクトな置き場所の選択肢を主に見つつ、歪でも十分大きい連結成分を20%だけ評価する。候補の増加だけでなく減少も符号付きで差し引く。

この無次元差を、公開生成分布から次のように料金単位へ直す。

- 公式の`P`分布を最も近い代表辺長bucketへ集約する。
- bucketの1組当たり料金規模を`E[P*Cmax(P)]`とする。
- `S_i`より後かつ`T_j`までに到着するという条件付きの`E[D^0.9]`を48点quadratureで求める。
- 価格ノイズには`E[2^Z] = exp((0.8 ln 2)^2/2) = 1.166193428...`を使う。
- bucketの期待到着数を`lambda`として、そのサイズの改善を使う機会を保守的に`1-exp(-lambda)`、最大1回分へ飽和させる。

各断面の料金差を`delta_k`として、空間改善の生推定額を

```text
raw_space_gain = 0.75 * average(delta_k) + 0.25 * min(delta_k)
estimated_space_gain = 0.25 * raw_space_gain
```

とする。最後の0.25は、静的な空間proxyが実際の逐次配置で回収される割合への安全係数である。

### 周長悪化と移動費

移動組の現在の確定料金と移動後の確定料金を、それぞれ

```text
old_fee = round_payment(V_j, P_j, old_max_perimeter)
new_fee = round_payment(V_j, P_j,
                        max(old_max_perimeter, new_perimeter))
fee_loss = old_fee - new_fee
```

で厳密に求める。移動費は公式と同じ`max(round(V_j*R),1)`である。採用条件は

```text
空間availabilityが少なくとも1項で改善
raw_space_gain > 0
estimated_space_gain > move_cost + fee_loss
```

の全てとする。したがって料金低下を許す場合も、その低下と移動費を合わせた額の4倍を超える生の将来空間価値が必要になる。

### 合法性と固定計算量

専用validatorが、移動元の現owner一致、人数、範囲、芝生、4連結、非同一領域、実周長、到着領域との非重複、移動費を再検証する。fee lossはvalidatorでも再計算する。commitは従来どおり全移動元clear、移動先place、現在到着placeの順で、移動組の`max_perimeter`だけを単調更新する。退去時刻は変わらないため既存組のheapは更新しない。

ケース全体の上限は次のとおり。

```text
cleanup試行                 24 turn
実移動                       12回
移動元shortlist               8組/試行
destination anchor          4096/移動元
destination anchor総数    160000/case
完成候補                       3件/移動元
candidate未来評価            192件/case
```

candidateのprofileは`192*3=576`断面、移動元ごとのbaseline profileは最大`24*8*3=576`断面なので、全space profileは最大1,152断面、約288万セルで固定される。

### 実行前検証

- `main.cpp` SHA-256: `ea43ccb95e643cf23c7b1542838db0ea5c7786dcf3547ded8b1fb52cbc5a100e`
- GCC C++20 release binary SHA-256: `f0f2b02b227bb910b84e840c55f9272604aedf8e095ddbc9cd4ac4933f2ac21c`
- Clang C++17 release binary SHA-256: `f4105f80a949af89e8ee547e0827424e75dd7826c34fe5049fe822f8c8862b0e`
- ASan/UBSan binary SHA-256: `d558ad7032baeab2f3745ff85c28ecd940c38d766de4862b0c0de358c79a01bd`
- GCC C++20 release build: pass、`-Wall -Wextra -Wshadow -Wpedantic`警告0件
- Clang C++17 release build: pass、同じ警告設定で警告0件
- ASan/UBSan build: pass（解答は未実行）
- Clang static analyzer: 指摘0件
- `git diff --check`: pass
- 独立静的監査2系統: move+Yes/No、退去境界、owner/groups/heap、最大周長、fee loss、移動費、旧救済経路の無効化、baseline/candidate対称性、生成分布、符号、安全係数、固定上限を確認し、blocking issueなし

ここまで解答プログラムおよびsanitizer binaryは一度も実行していない。以降はこのsource/binaryを変更せず、seed 0〜99の100ケースを1回だけ測定する。

### `proactive-cleanup-v1` の100ケース結果

固定したsource/binaryの基準結果は`pahcer/json/result_20260801_152335.json`。

- AC: `100/100`
- 合計score: `6,342,871,815`
- 比較元`placement-fit-v1`: `6,351,225,419`
- 差: `-8,353,604`、`-0.131527%`
- seed別: 改善14、悪化15、同点71
- cleanup: 29ケース42回。`move + Yes`が33回、`move + No`が9回
- 移動費合計: `50,881`
- 既存組の料金低下合計: `9,167`
- 時間: 中央値`1.390 sec`、95 percentile `1.860 sec`、最大`2.121 sec`、2秒超2ケース

同一binaryの再確認結果`pahcer/json/result_20260801_152617.json`でも全seedのscoreは完全一致し、最大時間だけ`1.968 sec`だった。

予防的整理は実際に発動したが、将来空間proxyだけで採用した結果、no-relocation版より合計scoreが低下した。特に、コード上は現在到着の配置を先に固定してからcleanupを評価しており、公式の意思決定順である「再配置を出力してから今回到着のYes/Noと領域を出力する」と内部評価順が逆だった。

この結果確認後はコードを変更せず停止した。その後、ユーザーから、再配置は到着を受け入れるか決める前の盤面で評価すべきであり、その自然な順序へ変更するという新しい明示指示を受けて次版を開始した。

## 2026-08-01: 到着前整理と枝別到着判断 (`proactive-cleanup-v2`)

### 意思決定順の修正

各turnを次の論理順序で扱う。

1. `T_j < S_i`の退去を処理し、そのセルを`fresh_mask`へ記録する。
2. 到着`i`をまだ置いていないowner上で、移動なし盤面と予防的整理盤面を比較する。
3. 整理候補では既存組をscratch owner上で実際に移した後、同じshadow price・同じ通常配置器を使って、到着`i`の`Yes/No`と領域を改めて決める。
4. 移動なし枝と整理枝の完成盤面を比較し、採用した枝だけをactual ownerへ一度commitする。
5. 出力は公式順どおり`move`を先に、その後に`Yes/No`と到着領域を出す。

実装上、移動なし枝の到着判断はcounterfactualとして先に計算して再利用するが、その時点ではownerやgroup stateを変更しない。したがって、実際に選ぶplanの状態遷移は上記順序と一致する。

### 到着前の現在空間評価

時刻`S_i`の到着未配置盤面で、v1と同じ8サイズbucketについて空間availabilityを計算する。公式`P`分布のbucket確率`q_b`とcompact時の期待料金係数`F_b`を用い、

```text
pre_score = sum_b q_b * F_b * availability_b
            / sum_b q_b * F_b
```

とする。整理後の`pre_score`が整理前より小さい候補は破棄する。

この値は「改善した空間が必ず今回の売上になる」とは限らないため、最終利益へ直接加算しない。あくまで、再配置時点で現在の空間を悪化させないための構造的gateと、高価な枝シミュレーションへ送る候補の順位付けにだけ使う。これにより、現在空間価値と今回到着料金の二重計上を避ける。

### 二段階候補評価

退去跡へのdestination生成、移動元shortlist、fresh 75%以上、1組1回、1turn 1組というv1の制約は維持する。

cheap段階ではmove-only planを元ownerから専用validatorで再構築し、次を確認する。

- 移動先の合法性、4連結、人数、周長
- 到着前`pre_score`の非悪化
- 既存組の不可逆な最大周長から求める料金低下
- 移動組の退去時刻までのproxy future

shortlist用の順位は概ね

```text
upper_bound_arrival_fee * max(0, pre_score差)
+ proxyのestimated future差
- move_cost
- fee_loss
```

とする。この値自体では移動を採用せず、全候補から上位4件だけをfinalistにする。浮動小数点keyはepsilon比較をsort comparatorへ使わず完全順序で比較し、strict weak orderingを保つ。

### finalistの枝別完全比較

各finalistについて、移動後scratch owner上で通常の到着判断を最初から実行する。

- 理論最小周長でもshadow price以下なら`No`
- placementが存在しなければ`No`
- 実周長料金がshadow price以下なら`No`
- それ以外は選ばれた領域へ`Yes`

移動なし枝にも同じ処理で得た結果を使う。両枝について、実際に`Yes`なら到着領域を置いた完成owner、`No`なら置かない完成ownerを作る。現在差は推定値ではなく、

```text
current_gain = candidate枝の実到着料金
             - baseline枝の実到着料金
```

とする。拒否枝の料金は0である。

未来需要数から現在到着を除き、両完成ownerのspace profileを対称に比較する。到着領域の有無または領域自体が枝間で異なる場合は`max(T_i, T_mover)`まで、同じ場合は盤面差が消える`T_mover`までをhorizonとする。v1と同じraw future差へ回収率0.25を一度だけ掛ける。

最終採用条件は

```text
pre_arrival_score(candidate) >= pre_arrival_score(baseline)

margin = current_gain
       + 0.25 * raw_future_gain_on_completed_boards
       - move_cost
       - fee_loss

margin > 0
```

である。pre-arrival値は金銭加算せず、今回料金は実現した枝の差だけ、futureは今回到着を反映した完成盤面の差だけなので、相互の二重計上はない。

この仕組みにより、整理の副作用としてbaselineで置けなかった今回到着が置ける場合はある。ただし、移動先は直前の退去跡75%以上で、到着前空間を非悪化にしなければならず、到着targetを先に置いてblockerを動かす旧`target-first relocation`は実行しない。目的はあくまで退去跡を用いた既存盤面の整理である。

### 状態更新・診断・固定計算量

candidate上の通常配置器とshadow評価はownerおよび既存組の退去時刻だけを見るため、scratch owner上で移動組の`groups[id].cells`が旧位置のままでも評価に影響しない。旧`evaluate_future_placement`は`groups[id].cells`を参照するため、この経路では呼ばない。

最終validatorはscratchではなく元ownerと旧`groups.cells`から、移動元clear、移動先place、到着領域検査/placeの順で完成盤面を再構築する。actual ownerへのcommitも選択後に一度だけ同順で行う。移動組の退去時刻は不変なのでheapを更新せず、`max_perimeter`だけを単調更新する。

ケース全体の上限は次のとおり。

```text
cleanup試行                     24 turn
実移動                           12回
cheap candidate評価             192件
finalist                         4件/試行
到着配置の完全再評価             64件/case
finalist完成盤面future profile  最大384断面/case
```

placement diagnosticsには探索した全counterfactualではなく、最終選択枝の到着判断だけを加算する。cleanup側ではcheap candidate数とfinalist数を別に出力し、実探索量を確認できる。

### 実行前検証

- `main.cpp` SHA-256: `6c2601f77c1ad0293e6205525bfc55a9a2777d36259adaabf1e836fdf88807e1`
- Clang C++17 release binary SHA-256: `5907a9e784df539ba7789801ae675a89fb57dc37dffe1bf0814f0016ad0334b5`
- Clang C++20 release binary SHA-256: `3e2a407620fd96d9087bc535c5c098bf2b33974d84adf07fe8f714e339418e42`
- ASan/UBSan binary SHA-256: `cb43b96edbaf3d1b5d8c3424b67fb9ed1ab78f87ab2ae713ddb7b4e5f2bac10b`
- C++17/C++20 release build: pass、`-Wall -Wextra -Wshadow -Wpedantic`警告0件
- ASan/UBSan build: pass（解答は未実行）
- Clang static analyzer: 指摘0件
- `git diff --check`: pass
- 独立静的監査3系統: scratch/actual owner、arrival拒否時fee、validator/apply/emit、heapと最大周長、sort比較、枝別料金、pre-arrival gate、完成盤面future、二重計上、horizon、固定計算量を確認し、blocking issueなし

ここまで新しい解答プログラムは一度も実行していない。次にsourceとbinaryを固定し、seed 0〜99を1回だけ測定する。

### `proactive-cleanup-v2` の100ケース結果

固定したsource/binaryの結果は`pahcer/json/result_20260801_155131.json`。

- AC: `100/100`
- 合計score: `6,350,676,900`
- 比較元`placement-fit-v1`: `6,351,225,419`
- 差: `-548,519`、`-0.008637%`
- seed別: 改善10、悪化15、同点75
- cleanup: 25ケース28回
- 時間: 中央値`1.329 sec`、95 percentile `1.913 sec`、最大`2.452 sec`

v1の大幅悪化は解消したものの、移動なし版を僅かに下回った。悪化seedをturn単位で分解すると、移動はすべてturn 12〜50であり、終盤特有の判断ミスではなかった。悪化15 seedでは、移動したturn自身の整理効果と料金差の合計は`+88,995`だった一方、その後の別グループの受入・配置の連鎖差が`-11,336,649`になった。また、現在到着料金が増えた7ケースは全て最終的に悪化した。

静的な完成盤面profileは「その断面で空間が広いか」は測れるが、その配置差によって次の組の置き場所と受入可否が変わり、さらに次の盤面へ伝播する効果を表現できていない。現在到着の枝だけを正確にしたv2でも、この逐次的な方策差が残ったことが主因と判断した。

この結果確認後はコードを変更せず停止した。その後、ユーザーから再配置にも拒否判断と同じshadow等を利用して評価を整合させるという新しい明示指示を受け、次版を開始した。

## 2026-08-01: shadow方策rolloutによる整理評価 (`proactive-cleanup-v3`)

### 最終評価の変更

v2の静的な完成盤面future金額推定を、候補shortlist・最終marginの両方から削除した。代わりに、移動なし枝と整理枝を同じ仮想未来入力へ進め、実際の逐次方策が得る料金差を比較する。

各仮想到着では実mainと同じ順に、次を行う。

1. `T_j < S`の利用中グループを退去させ、その実セルを解放する。
2. その枝のactive group群から`evaluate_shadow_cost`を再計算する。
3. 同じ`evaluate_cleanup_arrival_decision`で、理論最大料金gate、通常配置探索、実周長料金gateを順に適用する。
4. `Yes`ならscratch owner/groups/departure heapへ実際に追加し、実周長から求めた料金をrollout収益へ加える。
5. rollout内では再配置を再帰的に呼ばない。

shadow priceは入場方策のgateとしてだけ使う。受け入れた組の料金からshadow priceをさらに引くと機会費用を二重に数えるため、rollout収益は受け入れた仮想組の実料金合計だけとする。

### 共通の仮想未来

現在までのduration観測から得たposterior mean `theta`と`ConditionalFutureDemand`を用い、残り全組の仮想`S,T,P,V`を生成して`S`順に並べ、その先頭48到着を使う。

- 低食い違い列にはbase 2, 3, 5, 7のradical inverseを使う。
- 2本目は各quantileを`1-u`にしたantithetic scenarioとする。
- `P = round((2 + u*(sqrt(150)-2))^2)`。
- `V = round(P * D^0.9 * 2^(0.8*Phi^-1(u)))`を`[1, 10^8]`へ制限する。
- duration nodeは、現在時刻より後に整数startを取れる数を含めた条件付き重みで選ぶ。
- 公開generatorと同様、既知の過去・現在の`S/T`および同scenario内で生成済みのendpointと衝突した組は再生成する。試行は`8*remaining+64`回で打ち切り、残り組数を生成できなければfail-closedで整理しない。
- 仮想到着を処理する直前に、そのdurationをコピーした`ThetaEstimator`へobserveし、実mainと同じ順で次の`theta`を推定する。

scenarioは候補に依存させず一度だけ生成し、baseline/candidateへ完全に同じ列を渡す。したがって未来乱数差ではなく、整理による盤面・shadow・入場方策の差だけを測るcommon-random-number比較になる。未観測の実入力は参照しない。

### 枝状態と採用式

baselineは移動なしで今回到着の`Yes/No`を反映した完成owner、candidateは専用validatorで移動と今回到着を反映した完成ownerから始める。各scratch stateでは次も同期する。

- 移動組の`cells`と不可逆な`max_perimeter`
- 今回到着組の`active/cells/max_perimeter`
- 全active groupのdeparture heap
- 元の`M`より後ろへ追加する仮想group ID

2 scenarioそれぞれについて、

```text
scenario_margin
  = candidate今回料金 - baseline今回料金
  + candidate仮想48組料金 - baseline仮想48組料金
  - move_cost
  - 移動組の確定料金低下
```

を求める。最終採用条件は

```text
min(scenario_margin[0], scenario_margin[1]) > 0
```

とする。48組より後ろのterminal valueは0であり、旧space proxyを追加しない。これにより、現在料金、未来料金、移動費、最大周長悪化をそれぞれ一度だけ数える。

### cheap候補と計算量

到着前space profileの非悪化gateは維持するが、これは金額へ加算しない。rolloutへ送る完成候補は1件だけとし、候補順位はまず

```text
pre_arrival_space_gain / (move_cost + fee_loss)
```

を最大化し、同率ならspace gainが大きく、費用が小さいものを選ぶ。これにより、空間改善が僅かに大きいだけの高コスト候補が唯一のrollout枠を奪うのを抑える。

1回の完全比較は

```text
2 scenario * 48 arrival * (baseline + candidate) = 192 policy steps
```

である。2秒制限とv2の実行時間を考慮し、case全体の上限も192 step、すなわち通常は最初に完成候補が得られた1 turnだけを評価する。一度棄却した後の整理機会を逃す弱点は意図的に受け入れ、まず方策整合の効果を限定した計算量で測る。残り件数が48未満なら実際の残件数までとする。

### 実行前検証

- `main.cpp` SHA-256: `7ad658abb0f67c79ca9bd22c896855b12d0a0f5358a36b3c21a22d77e16fc133`
- Clang C++17 release binary SHA-256: `8b0c1946012b367e28dffee2f71fe4bbd17ef0be60ad6d05160d5497f4e0c116`
- Clang C++20 release binary SHA-256: `8a11f27159876603fb8318521e47823ddf852ad84394543b2bf9023435e02878`
- ASan/UBSan binary SHA-256: `84b1fabb23604789fe784d2fe8be4393add98212fa27cecae679dc6aeb53cb85`
- C++17/C++20 release build: pass、`-Wall -Wextra -Wshadow -Wpedantic`警告0件
- ASan/UBSan build: pass（解答は未実行）
- Clang static analyzer: 指摘0件
- `git diff --check`: pass
- 独立静的監査2系統: baseline/candidate状態同期、退去境界、仮想IDと参照寿命、未来情報リーク、endpoint再生成、shadow/admission同一性、実料金と費用の一重計上、robust最小値、候補費用効率、192 step上限を再確認し、blocking issueなし

ここまで固定した解答プログラムおよびsanitizer binaryは一度も実行していない。以降はこのsource/binaryを変更せず、seed 0〜99の100ケースを1回だけ測定する。

### `proactive-cleanup-v3` の100ケース結果

固定したsource/binaryの結果は`pahcer/json/result_20260801_163153.json`。

- AC: `100/100`
- 合計score: `6,353,144,924`
- 比較元`placement-fit-v1`: `6,351,225,419`
- 差: `+1,919,505`、`+0.03022%`
- v2比: `+2,468,024`
- それまでのローカル最高`relocation-lns-v2`比: `+832,075`
- 移動なし版とのseed別: 改善8、悪化10、同点82
- rollout実施91ケース、整理成功18ケース、scenario生成失敗0件
- 成功分の移動費`93,689`、既存組料金低下`29,497`、現在料金差`+13,200`
- worst-scenario未来料金差合計`+1,975,573`、予測margin合計`+1,865,587`
- 時間: 中央値`1.621 sec`、95 percentile `2.371 sec`、最大`2.678 sec`
- 最初の8 seedを除くと中央値`1.575 sec`、95 percentile `2.031 sec`、最大`2.265 sec`

shadow方策rolloutによって再配置は移動なし版を初めて上回り、100ケース合計も当時のローカル最高になった。一方、成功18件中、実スコアでは改善8・悪化10であり、純増は少数の大きな改善による。さらに91ケースでは最初の完成候補に192 policy stepを全て使い、その後の退去機会を評価できなかった。

固定実行後はコード・方針・memoを変更せず停止した。その後、ユーザーから再配置をさらに活用する改善案の提示と、そのうち推奨した事前shadow gate・rollout末尾評価を実装する明示指示を受け、次版を開始した。

## 2026-08-01: rollout予算gateとshadow terminal (`proactive-cleanup-v4`)

### 変更範囲

1組を直前の退去跡へ移すdeparture-hole exchange、到着前space非悪化gate、2 scenario×48到着の共通rollout、通常のshadow入場方策は維持する。今回は次の2点だけを変更する。

1. 弱い候補が最初の192 stepを消費しないための、rollout前の楽観的な金額gate。
2. 48組終了時に残っている占有が、さらに後の未知組へ与える影響を同じshadow分布で評価するterminal continuation value。

### rollout前の候補gate

cheap候補を1件へ絞った後、scenarioを生成する前にcandidate盤面で今回到着の通常判断を正確に行い、元ownerから専用validatorも通す。ここで

```text
current_gain = candidate今回料金 - baseline今回料金
economic_cost = move_cost + fee_loss
```

を確定する。

移動元を除くことで最大空き成分へ新たに連結される`merged_gain`セルを、移動組の退去時刻まで完全に使えるという楽観条件にする。同じ`evaluate_shadow_cost`へ

```text
p = merged_gain
arrival_t = T_mover
```

を与え、その機会費用を`shadow_upper`とする。candidate/baselineで今回到着の有無または領域が異なる場合は、今回到着組自身のshadow opportunity costも加え、配置枝差を過小評価しない。

容量shadowだけでは、総面積に余裕があるが断片化している候補を過剰に落とす。そのため、到着前space改善から別の楽観上限も作る。

```text
H = max(T_mover, T_current)
mu = remaining_groups * Pr(S_future <= H | S_future > current_S)
K_hi = min(48, remaining_groups, ceil(mu + 2*sqrt(mu)))
fee_scale = max(今回到着の理論最大料金,
                ceil(今回のshadow opportunity cost))
structural_upper
    = fee_scale * K_hi * clamp(pre_arrival_gain, 0, 1)
```

最終的な事前値は

```text
gate_value = current_gain
           + max(shadow_upper, structural_upper)
```

とし、`gate_value <= economic_cost`ならrolloutせずbaselineを採用する。両future値は候補を通しやすい楽観値であり、最終marginへは加算しない。gate棄却時はactual owner/groups、`proactively_moved`、rollout policy stepを変更しないため、次の退去機会で再び候補を評価できる。

残り組数0なら未来不確実性がないので、gate通過後に`current_gain-economic_cost>0`を満たす場合だけ、rolloutなしで厳密に採用する。

### shadow-consistent terminal value

scenarioの48組目を処理した直後をcutoffとする。

```text
cutoff_s         = scenario.back().s
tail_remaining   = scenario.back().remaining_after
cutoff_theta     = scenario.back().theta
```

`cutoff_theta`は48組目のdurationをobserveした後、実mainと同じ`estimate(cutoff_s, tail_remaining)`で得た値である。

cutoffより後の各time bucket`[a,c]`について、branchのactive組が使うcell-timeを引いた容量を

```text
A_b = max(0,
    grass_cells*(c-a)
    - sum_active P_j*max(0,min(T_j,c)-a))
```

とする。`ConditionalFutureDemand(cutoff_s, cutoff_theta)`から未知tail組の期待需要cell-time`F_b`と、overlap重み付き`log D`の平均・分散を得る。容量だけを見た低密度側の拒否率は

```text
q_b = clamp(1 - A_b/F_b, 0, 1)
```

である。

既存`DensityModel`が仮定するcell-time当たり料金密度を

```text
X = log2(fee/(P*D)) ~ Normal(m_b, s_b^2)
```

とすると、上位`1-q_b`だけを受け入れた料金密度の一次モーメントは、`z_b=Phi^-1(q_b)`として

```text
E[2^X * 1(X is accepted)]
  = exp(ln(2)*m_b + 0.5*ln(2)^2*s_b^2)
    * Phi(ln(2)*s_b - z_b)
```

になる。これへ`F_b`を掛けて全bucketで合計したものを、そのbranchのterminal continuation valueとする。`q=0`は全料金密度の期待値、`q=1`は0へ明示分岐する。

terminal active組の料金は、current料金またはrollout48組の実料金として既に数えているため再加算しない。active組はtail容量を減らすcommitmentとしてだけ使う。terminalで料金を数えるのは、まだ生成していない`tail_remaining`組だけである。

scenarioごとの最終値は

```text
future_gain
  = candidate_rollout_realized_fee
    - baseline_rollout_realized_fee
    + candidate_terminal_value
    - baseline_terminal_value

margin = current_gain + future_gain
         - move_cost - fee_loss
```

であり、従来どおり2 scenarioの`margin`最小値が正の場合だけ整理する。事前gateの楽観値、各到着のshadow opportunity cost、旧static profile金額は最終値へ加えない。

terminalは配置形状を直接見ず、active組の`P,T`による容量圧迫だけを見る。bucketごとに密度順でfractional admissionするfluid relaxationでもある。これは48組後のgeometryを正確に解く値ではなく、有限rollout末尾の未回収占有コストを現行shadowと同じ分布で補うための近似である。

### 計算量と診断

gate棄却はrollout stepを消費しないが、通常のcleanup attempt、candidate評価、candidate到着配置1回は消費する。既存上限`24 attempt / 192 candidate / 64 finalist / 160000 destination anchors`内で後続候補を探す。

full rolloutの上限は従来どおり192 policy step。terminalは最大64 bucketについてactive組のcell-timeを合計し、2 branch×2 scenarioでもplacement探索より十分軽い。

新しい診断には、gateのconsidered/rejected、gate value/cost/current gain、structural/shadow上限、機会数、terminal評価数、実現未来料金差、terminal差を追加した。

### 実行前検証

- `main.cpp` SHA-256: `b278da65ad9355ea58b2c6254ff064de03f3d05413b7d70ebaba581345d229ed`
- Clang C++17 release binary SHA-256: `b1da2628f4cb827578fd577bef44e4c38903dc917062dad945c4d2ccf84e10a1`
- Clang C++20 release binary SHA-256: `ef8da88d230fd74c73dfae8d3786803ab71ac9c8f54b76629c1e165f6e659a54`
- ASan/UBSan binary SHA-256: `f90b3d5e50aa4281ef8b5c4a0e3d0e60dfdaff642d72df7513434fd21a4ad6b7`
- C++17/C++20 release build: pass、`-Wall -Wextra -Wshadow -Wpedantic`警告0件
- ASan/UBSan build: pass（解答は未実行）
- Clang static analyzer: 指摘0件
- `git diff --check`: pass
- 独立静的監査3系統: gateの符号・順序・skip時状態、未来情報リーク、cutoff時刻、remaining/theta、active commitment、lognormal打切り一次モーメント、bucket積分、料金と費用の一重計上、残り0件の厳密分岐を確認し、blocking issueなし

既知の近似は、terminalが配置形状を直接見ないことと、bucket別のfractional admissionであること。いずれも現行shadowとの整合を優先した仕様である。

ここまで固定した解答プログラムおよびsanitizer binaryは一度も実行していない。以降はこのsource/binaryを変更せず、seed 0〜99の100ケースを1回だけ測定する。

### `proactive-cleanup-v4` の100ケース結果

固定したsource/binaryの結果は`pahcer/json/result_20260801_171109.json`。

- AC: `100/100`
- 合計score: `6,348,369,571`
- v3比: `-4,775,353`
- 再配置なし`placement-fit-v1`比: `-2,855,848`
- v3とのseed別: 改善4、悪化6、同点90
- 再配置なし版とのseed別: 改善5、悪化10、同点85
- gate検討114件、gate棄却28件、full rollout 86件、整理成功15件
- rollout policy step合計`16,512`、terminal評価344回
- 時間: 中央値`1.617 sec`、95 percentile `2.135 sec`、最大`2.231 sec`

v4はv3の悪い移動をseed 40、44で止めた一方、seed 23、38、63、64の大きな改善も止めた。さらにseed 72では新しい悪化移動を採用した。特にseed 23、38、40、44は事前gateを通過した後のterminal込みrolloutで棄却されており、良い移動と悪い移動を分離できなかった。合計scoreもv3を明確に下回ったため、ユーザー指示によりv4の事前gateとterminal continuationを取り下げ、v3の実現料金差だけの評価へ戻す。

## 2026-08-01: 共通rolloutによる2候補比較 (`proactive-cleanup-v5`)

### v3評価への復帰

v4で追加した次の要素をすべて削除した。

- rollout前のshadow/structural楽観gate
- 48組後のshadow terminal continuation value
- 打切りlognormal一次モーメント
- gate、terminal、実現料金分解用の診断
- 残り0件を事前gate後に特別採用する分岐

保存済みv3 binary `/private/tmp/ahc069_shadow_rollout_v1` のsymbol、diagnostic string、逆アセンブルも参照し、rollout branchを仮想未来組の実料金合計`ll`へ戻した。最終評価はv3と同じくscenarioごとに

```text
margin_k
  = candidate今回料金 - baseline今回料金
  + candidate仮想未来料金 - baseline仮想未来料金
  - move_cost - fee_loss
```

を求め、各候補の`min_k margin_k`だけを採用値とする。static space proxy、shadow opportunity cost、terminal値は最終marginへ加えない。到着前space profileの非悪化条件は、候補の構造的gateとして引き続き使う。

### 複数候補の公平な比較

cheap段階では従来と同じ

```text
pre_arrival_space_gain / (move_cost + fee_loss)
```

の順で全候補から上位2件を保持する。各候補は異なる移動組または異なる移動先を表す。候補ごとに移動後owner上で今回到着の通常shadow入場判断をやり直し、元ownerから専用validatorで完成盤面を再構築する。

仮想未来scenarioはターンごとに一度だけ生成する。baselineは各scenarioを一度だけrolloutして料金をcacheし、2候補とも全く同じ到着列・同じbaseline料金との差を取る。候補の番号や評価順はscenario生成へ入らないため、候補間の差は盤面と逐次方策の差だけになる。

正のworst-scenario marginを持つ候補のうちmargin最大を採用する。同値なら、移動費と料金低下の合計が小さい候補、worst-scenario未来料金差が大きい候補、元の列挙順が早い候補の順に決める。1位候補が負でも2位候補の評価を続け、2位の方が良ければ2位だけをactual stateへcommitする。

### 計算量

2候補を従来と同じ48到着で見るとpolicy stepが288へ増える。v3は100ケースで中央値`1.621 sec`、95 percentile `2.371 sec`だったため、時間上限を維持する方を選んだ。

```text
v3: baseline 1 + candidate 1, 2 scenario, 48 arrival
    2 * 2 * 48 = 192 policy steps

v5: baseline 1 + candidate 2, 2 scenario, 32 arrival
    3 * 2 * 32 = 192 policy steps
```

残り組数が32未満なら全残件を使う。case全体のpolicy step上限192、cleanup試行24、cheap candidate 192、finalist 64、destination anchor 160000は維持する。full長の比較では最初の整理機会でpolicy予算を使い切る点もv3と同じである。

診断にはfull rolloutした候補数、2候補を比較できたturn数、cheap順位2位が勝った回数を追加した。

### 実行前検証

- `main.cpp` SHA-256: `83d18820cf9f5a369ef7fba836bb662d948ba20410175d00beb4f18b47f0d183`
- Clang C++17 release binary SHA-256: `3d1b68bd1e059f31b7b7ec07f7e50a7053981b875b3d38295230413112d08257`
- Clang C++20 release binary SHA-256: `56c206fb8fea02bc49747a2f482e76a9b47eacdf5e3cfbd3cf6ef5eb832c789a`
- ASan/UBSan binary SHA-256: `aedcacc313a32ccfc414571f27333f1d07dab115687895812cf89facaa15f943`
- C++17/C++20 release build: pass、`-Wall -Wextra -Wshadow -Wpedantic`警告0件
- ASan/UBSan build: pass（解答は未実行）
- Clang static analyzer: 指摘0件
- `git diff --check`: pass
- 独立静的監査3系統: v4固有評価の完全除去、common random numbers、baseline共有、候補状態独立、validator、worst-scenario margin、tie-break、費用一重計上、予算境界、2位採用時のcommitを確認し、blocking issueなし

ここまで固定した解答プログラムおよびsanitizer binaryは一度も実行していない。以降はこのsource/binaryを変更せず、seed 0〜99の100ケースを1回だけ測定する。

### `proactive-cleanup-v5` の100ケース結果

固定したsource/binaryの結果は`pahcer/json/result_20260801_173229.json`。

- AC: `100/100`
- 合計score: `6,353,801,913`
- v3比: `+656,989`
- v4比: `+5,432,342`
- 再配置なし`placement-fit-v1`比: `+2,576,494`
- v3とのseed別: 改善11、悪化7、同点82
- 再配置なし版とのseed別: 改善8、悪化6、同点86
- 現在の`best_scores.json`基準の相対平均: v5 `99.60250`、v3 `99.60280`。生score合計はv5が上だが、相対指標ではほぼ同点でv3が僅かに上
- rolloutを行った整理機会91回、full評価候補131件、2候補比較40回、cheap順位2位の勝利1回（seed 91）
- 整理成功14回（到着あり13、到着なし1）
- 移動費`52,883`、既存組料金低下`36,082`、現在料金差`+32,123`
- worst-scenario未来料金差合計`+964,052`、予測margin合計`+907,210`
- rollout policy step合計`14,208`、scenario生成失敗0件
- 時間: 中央値`1.533 sec`、95番目`1.963 sec`、最大`2.250 sec`

v3で動いた18 seedのうち7件を同じ結果のまま維持し、その実gain合計は`+2,655,846`。v3の移動を取りやめた11件は、v3での実gain合計が`-736,341`だったため、取りやめによって同額改善した。一方、v5で新しく動いた7件の実gain合計は`-79,352`だった。したがってv3比`+656,989`は、主に旧版のnetで悪い移動群を止めた効果である。

2位候補が実際に採用されたのはseed 91の1件で、この版の再配置なし比は`+363,919`だった。複数候補比較は有効例を1件作ったが、48件から32件へのhorizon短縮による採否変更の方が全体への影響は大きかった。

## 2026-08-01: 2候補を48件で比較 (`proactive-cleanup-v6`)

ユーザー指示により、32件へ短縮せずv3と同じ48件の未来を2候補とも評価する版を確認する。v5から変更するのは次の2定数だけであり、その他のsourceはbyte-for-byte同一である。

```text
CLEANUP_ROLLOUT_LENGTH:              32 -> 48
CLEANUP_ROLLOUT_POLICY_STEP_LIMIT:  192 -> 288
```

scenario生成は候補に依存せず、同じraw到着列を生成・sortして先頭から使用する。したがってv5で見た先頭32件は変えず、その後ろへ同じscenarioの16件を追加する。

```text
baseline 1 + candidate 2
2 scenario
48 arrival

3 * 2 * 48 = 288 policy steps
```

2候補が揃う通常turnではcase全体の288 stepを使い切る。候補1件なら`2*2*48=192`である。v5比ではrollout主項が最大1.5倍になるが、通常の実到着1000回の配置判断を含む全体では増加率はそれより小さい。固定上限は維持される一方、v5でもローカル2秒超が4ケースあったため、時間超過ケースが増える可能性を測定する。

### 実行前検証

- `main.cpp` SHA-256: `debce5a5e8f287879ce1da81a8591f2e3b4a1c1c0cab6e6e307111c42c14fc34`
- Clang C++17 release binary SHA-256: `484f866602e62ea1b697f9a09c74311f8a99bedb71e3e64953b412f37e7bd288`
- Clang C++20 release binary SHA-256: `fa5c7729ff4621496ed6fe16660d1bf7c979d82248fe7ed39cbf1c2ec99e9827`
- ASan/UBSan binary SHA-256: `57f0179f26143a619ad2beeac5dfb8a7ed16e48c1aa9c398d4271f331441d58e`
- C++17/C++20 release build: pass、`-Wall -Wextra -Wshadow -Wpedantic`警告0件
- ASan/UBSan build: pass（解答は未実行）
- Clang static analyzer: 指摘0件
- `git diff --check`: pass
- 独立静的監査2系統: v5との差が定数2点だけであること、先頭32件のscenario不変、288 step予算、baseline共有、common random numbers、候補状態独立、worst-scenario marginを確認し、blocking issueなし

ここまで固定した解答プログラムおよびsanitizer binaryは一度も実行していない。以降はこのsource/binaryを変更せず、seed 0〜99の100ケースを1回だけ測定する。

### `proactive-cleanup-v6` の100ケース結果

固定したsource/binaryの結果は`pahcer/json/result_20260801_174559.json`。

- AC: `100/100`
- 合計score: `6,357,018,471`
- v5（幅2・深さ32）比: `+3,216,558`
- v3（幅1・深さ48）比: `+3,873,547`
- 再配置なし`placement-fit-v1`比: `+5,793,052`
- v3とのseed別: 改善5、悪化2、同点93
- 再配置なし版とのseed別: 改善11、悪化9、同点80
- 現在の`best_scores.json`基準の相対平均: v6 `99.63093`、v5 `99.57727`、v3 `99.57761`
- rolloutを行った整理機会91回、full評価候補131件、2候補比較40回、cheap順位2位の勝利7回
- 整理成功20回（到着あり18、到着なし2）
- 移動費`138,605`、既存組料金低下`31,663`、現在料金差`+15,365`
- worst-scenario未来料金差合計`+2,427,566`、予測margin合計`+2,272,663`
- rollout policy step合計`21,312`、scenario生成失敗0件
- 時間: 中央値`1.725 sec`、95番目`4.976 sec`、最大`6.299 sec`
- 2秒超28ケース、3秒超9ケース、4秒超8ケース

v3と結果が変わったseedは、cheap順位2位が勝った次の7件と完全に一致した。

```text
seed 24:  -576,728
seed 28:  +321,651
seed 39: +1,617,444
seed 40: +1,915,305
seed 66: +1,898,114
seed 69: -1,666,158
seed 91:  +363,919
合計:    +3,873,547
```

幅1・深さ48のv3と未来列・評価式を揃えた比較なので、この差は2件目の候補を比較した効果である。生score合計だけでなく相対指標も明確に改善し、候補幅2の価値は確認できた。

一方、48件版は時間が大幅に悪化した。遅いケースは最初のseedだけではなく、解答自身のelapsedでもseed 91が`6.287 sec`だった。後半16件の追加は、空きの多い盤面で高価な配置探索へ進む場合があり、policy stepの1.5倍より大きな実時間差になり得る。スコア面では最良だが、そのままの提出にはTLEリスクが高い。

## 2026-08-01: rollout候補幅3 (`proactive-cleanup-v7`)

ユーザー指示により、今度は未来horizonではなく比較する候補幅を広げる。v5と同じ32件の未来に戻し、cheap評価上位3件を共通scenarioで比較する。

```text
CLEANUP_FINALIST_LIMIT:              2 -> 3
CLEANUP_ROLLOUT_LENGTH:                  32
CLEANUP_ROLLOUT_POLICY_STEP_LIMIT:  192 -> 256

(baseline 1 + candidate 3) * 2 scenario * 32 arrival
= 256 policy steps
```

v5との評価差は候補幅2から3と、それに必要なpolicy上限だけであり、深さ32、scenario、shadow入場判断、validator、baseline共有、worst-scenario margin、tie-break、rollout本体は同一である。したがってv5との測定差は幅を広げた効果として比較できる。

3候補が揃わないturnでは実際の候補数に応じて必要stepを計算する。上位候補が棄却されても後続候補を最後まで評価し、正のworst-scenario marginが最大の候補だけをcommitする。診断には3候補を実際に比較できたturn数と、cheap順位3位の勝利回数を追加した。

### 実行前検証

- `main.cpp` SHA-256: `d4dd8a503c0aa89c73bffc2147973d84bb5fda57be5f63a21b51044020d5b05f`
- Clang C++17 release binary SHA-256: `2f5703c5ef425a604ac8f395a68cb92ef2523a656fc032333dc6df814b08d577`
- Clang C++20 release binary SHA-256: `e5015e7ba79db28e0d8e6b9044008c9decb5100d96e9803fbe06795732c7c5f7`
- ASan/UBSan binary SHA-256: `64b00fefd7d60972d7860fe57886945a92ee1fe7ad1599c13ccb7b640371d48c`
- C++17/C++20 release build: pass、`-Wall -Wextra -Wshadow -Wpedantic`警告0件
- ASan/UBSan build: pass（解答は未実行）
- Clang static analyzer: 指摘0件
- `git diff --check`: pass
- 独立静的監査2系統: 256 step予算、上位3候補loop、common random numbers、baseline共有、候補状態独立、worst-scenario margin、3候補/3位勝利診断、v5主要関数との一致を確認し、blocking issueなし

ここまで固定した解答プログラムおよびsanitizer binaryは一度も実行していない。以降はこのsource/binaryを変更せず、seed 0〜99の100ケースを1回だけ測定する。

### `proactive-cleanup-v7` の100ケース結果

固定したsource/binaryの結果は`pahcer/json/result_20260801_175324.json`。

- AC: `100/100`
- 合計score: `6,353,205,399`
- v5（幅2・深さ32）比: `-596,514`
- v6（幅2・深さ48）比: `-3,813,072`
- v3（幅1・深さ48）比: `+60,475`
- 再配置なし`placement-fit-v1`比: `+1,979,980`
- v5とのseed別: 改善1、悪化3、同点96
- 現在の`best_scores.json`基準の相対平均: v7 `99.54289`、v5 `99.55555`、v6 `99.60921`
- rollout候補170件、rollout turn 113回
- 1候補turn 73回、2候補turn 23回、3候補turn 17回
- cheap順位3位の勝利0回、順位2位の勝利1回（seed 91）
- 整理成功18回（到着あり17、到着なし1）
- rollout policy step合計`18,112`、scenario生成失敗0件
- 時間: 中央値`1.574 sec`、95番目`2.001 sec`、最大`2.260 sec`、2秒超6ケース

3候補を比較できた17 turnでは、3位候補は一度も勝たなかった。v5から変わったのはseed 4、42、90、95の4件であり、いずれも3候補比較ではない。各caseで1候補ずつのrolloutを2 turn行い、policy stepを`128+128=256`使っていた。v5の上限192では2回目を行えなかったため、今回の差は候補幅3の効果ではなく、上限拡大により後続の整理機会まで評価した効果である。その実score差合計は`-596,514`だった。

したがって、深さ32におけるcheap順位3位までの拡張には、この100 seedでは直接の利益が確認できなかった。一方、幅を広げる発想自体を検証するには、さらに浅いrolloutにして多数のroot候補を同じ予算内で比較する余地がある。

## 2026-08-01: 浅いrolloutで候補幅16 (`proactive-cleanup-v8`)

ユーザー指示により、未来horizonを8件まで浅くし、cheap評価上位16候補を同じ2 scenarioで比較する。

```text
CLEANUP_FINALIST_LIMIT:               3 -> 16
CLEANUP_ROLLOUT_LENGTH:              32 -> 8
CLEANUP_ROLLOUT_POLICY_STEP_LIMIT:  256 -> 272

(baseline 1 + candidate 16) * 2 scenario * 8 arrival
= 272 policy steps
```

最大policy stepはv6の288に近いが、各scenarioの後半40件を評価しない。その代わり、今回到着の通常shadow判断・完成盤面validator・branch初期化は最大16候補へ増える。

cheap shortlist、到着前space非悪化gate、候補ごとの正確な今回到着判断、共通scenario、baseline共有、候補状態独立、worst-scenario margin、tie-breakは従来と同じである。scenarioは同じraw未来列の先頭8件だけを全候補へ渡す。

診断には、実際に16候補を比較できたturn数、勝者のcheap順位合計・最大、4位以降および8位以降の勝利回数を追加した。これにより、広げた候補枠が実際の採用へ届いたかを確認する。

### 実行前検証

- `main.cpp` SHA-256: `c74bb7b779e46ba0e913cbf6e4031d1c8f9723b1d41551010f56335e98fc3f14`
- Clang C++17 release binary SHA-256: `266ec55929f5c768035cc44bb3e948c7fc9f7bd96da26e2b5ae279a2d3b0e1f2`
- Clang C++20 release binary SHA-256: `c4588d4ef1b53a2924e8469034bb1f92743fdd1dca6cadba94accc15595c30cc`
- ASan/UBSan binary SHA-256: `c828c6d00576fdbcdb1e419244ce7e9db1bee5ff5fb940f73ce694afb1978730`
- C++17/C++20 release build: pass、`-Wall -Wextra -Wshadow -Wpedantic`警告0件
- ASan/UBSan build: pass（解答は未実行）
- Clang static analyzer: 指摘0件
- `git diff --check`: pass
- 独立静的監査2系統: 上位16候補保持、272 step予算、common scenarios、baseline共有、branch状態独立、winner rank診断、配列・整数境界、v7との差分を確認し、blocking issueなし

ここまで固定した解答プログラムおよびsanitizer binaryは一度も実行していない。以降はこのsource/binaryを変更せず、seed 0〜99の100ケースを1回だけ測定する。

### `proactive-cleanup-v8` の100ケース結果

固定したsource/binaryの結果は`pahcer/json/result_20260801_180143.json`。

- AC: `100/100`
- 合計score: `6,346,587,751`
- v7（幅3・深さ32）比: `-6,617,648`（改善10、悪化14、同点76）
- v5（幅2・深さ32）比: `-7,214,162`（改善8、悪化13、同点79）
- v6（幅2・深さ48）比: `-10,430,720`
- v3（幅1・深さ48）比: `-6,557,173`
- 再配置なし`placement-fit-v1`比: `-4,637,668`（改善4、悪化9、同点87）
- 現在の`best_scores.json`基準の相対平均: v8 `99.41110`、v5 `99.52182`、v6 `99.57548`
- 整理成功14回、rollout候補393件、rollout policy step合計`9,728`
- rolloutを行ったturnは215回。1候補112回、2候補57回、3候補以上46回、16候補turnは0回
- cheap順位1位の勝利12回、順位2位の勝利2回、3位以降の勝利0回
- 時間: 中央値`1.741 sec`、95番目`2.482 sec`、最大`2.654 sec`、2秒超27ケース

幅16の枠まで実際に候補が揃ったturnはなく、採用も2位までに留まった。一方、深さ8で1回の比較が安くなったため、同じcase内で後続の整理機会へ余ったpolicy stepを繰り越し、rollout turnが215回まで増えた。未来を浅くしたことと、別turnの前処理・scenario生成を繰り返したことが同時に変わっており、合計scoreと時間はいずれも悪化した。

## 2026-08-01: rollout予算を後続turnへ繰り越さない (`proactive-cleanup-v9`)

ユーザー指示により、v8の幅16・深さ8・最大272 policy stepは維持し、最初に実際のrollout比較へ進んだ1 turnだけでcase全体のrollout予算を使用済みとする。

- cheap候補が空、未来長0、scenario生成失敗、必要step超過なら、まだrolloutを始めていないため使用済みにしない。
- 比較可能な候補とscenarioが揃い、必要stepが上限内だと確認できた時点で使用済みにする。
- 候補数が16未満で実stepが272未満でも、余りを後続turnへ持ち越さない。
- rolloutの結果が不採用でも、実際に比較したため使用済みのままとする。
- `cleanup_rollout_policy_steps`は水増しせず実際の計算量を保持し、新設した`cleanup_rollout_turns`で各case最大1 turnであることを確認する。

候補生成、cheap順位、到着前space非悪化gate、共通scenario、baseline共有、worst-scenario margin、到着判断、validator、幅16、深さ8はv8から変更しない。したがって、v8との差は余った予算を後続の再配置機会へ回すかどうかである。

### 実行前検証

- `main.cpp` SHA-256: `71e3d5e8f686581f62d5b03c76ee6ef4db3ab4203a1e4de2231c9696a27ba499`
- Clang C++17 release binary SHA-256: `7a9779a022d56d650c65e4941bf058ad31b0c8aab6135b2af06de9ac4bfa08dc`
- Clang C++20 release binary SHA-256: `eafcb5291290c334ffcdf60ec112eaf6150a667f3a5261635b2268067ee15c71`
- ASan/UBSan binary SHA-256: `d316e61166b99ff3bf55d307bc4fa073001ea732aafa885e3e2db3200d3ef69c`
- C++17/C++20 release build: pass
- C++17 `-fsyntax-only -Wall -Wextra -Wshadow -Wpedantic`: 警告0件
- ASan/UBSan build: pass（解答は未実行）
- Clang static analyzer: 指摘0件
- `git diff --check`: pass
- 独立静的監査: 最初の実rollout turnのみ許可、未使用step非繰越、生成前失敗では非消費、実policy step診断保持を確認し、blocking issueなし

Pahcer設定は`pahcer/bench_proactive_cleanup_v9.toml`、固定binaryは`/private/tmp/ahc069_width16_depth8_single_rollout_v1`。ここまで固定した解答プログラムおよびsanitizer binaryは一度も実行していない。以降はsource/binaryを変更せず、seed 0〜99の100ケースを1回だけ測定する。

### `proactive-cleanup-v9` の100ケース結果

固定したsource/binaryの結果は`pahcer/json/result_20260801_181514.json`。

- AC: `100/100`
- 合計score: `6,352,330,426`
- v8（幅16・深さ8・余り繰越あり）比: `+5,742,675`（改善4、悪化0、同点96）
- 再配置なし`placement-fit-v1`比: `+1,105,007`（改善4、悪化6、同点90）
- v5（幅2・深さ32）比: `-1,471,487`
- v6（幅2・深さ48）比: `-4,688,045`
- 現在の`best_scores.json`基準の相対平均: v9 `99.52690`、v5 `99.52182`、v6 `99.57548`
- rollout turn: 91回（91 caseで1回、9 caseで0回、最大1回）
- rollout候補156件、policy step合計`3,952`、scenario生成失敗0件
- 整理成功10回。cheap順位1位9回、順位2位1回、3位以降0回
- 1候補case 51、2候補23、3候補10、4候補6、5候補1。6候補以上および16候補caseは0
- Pahcer時間: 中央値`2.602 sec`、95番目`4.738 sec`、最大`5.266 sec`

v8との差が出たseed 7、48、58、92は全て改善した。seed 48、58、92はv9が再配置なし版と一致し、seed 7は最初の有益な整理だけを残してv8より`+1,249,893`だった。余った予算で後続turnを評価・採用した4件は、この100 seedでは全て実scoreを悪化させていた。

policy stepはv8の`9,728`から`3,952`へ59.4%減った一方、同時実行時のPahcer計測時間は逆に長かった。前半seed群全体が一様に遅い測定だったため、この1回の時間値からone-shot化の計算量を判断しない。

## 2026-08-01: 候補幅2・rollout深さ45 (`proactive-cleanup-v10`)

ユーザー指示により、v9のone-shot予算管理を維持したまま、候補幅を16から2へ絞り、その分だけ未来horizonを8件から45件へ深くする。

```text
CLEANUP_FINALIST_LIMIT:   16 -> 2
CLEANUP_ROLLOUT_LENGTH:    8 -> 45

(baseline 1 + candidate 2) * 2 scenario * 45 arrival
= 270 policy steps <= case上限272
```

深さ46では276 stepとなり上限を超えるため、272 stepを変えずに取れる最大深さは45である。候補が1件なら実際には`2 * 2 * 45 = 180` stepだが、余り92 stepは後続turnへ繰り越さない。

候補生成、cheap順位、到着前space非悪化gate、共通scenario、baseline共有、worst-scenario margin、到着判断、validatorはv9から変更しない。診断上の`cleanup_rollout_full_width_turns`は、今回から実際に2候補を比較したturn数を表す。

### 実行前検証

- `main.cpp` SHA-256: `141ebe0c4d4cecf04dc61d121d2d6fb3fe37c33cb6d8218bf9e7dcf9b59c9612`
- Clang C++17 release binary SHA-256: `e8929c99e03210c2baf35c9ffc690abfb4439991ed0a38ad6fde5f1f6030e880`
- Clang C++20 release binary SHA-256: `a4d4dcc7a26cdc0977ecf41138c251795d82ce9a7dc9dbd47f28e4160443b739`
- ASan/UBSan binary SHA-256: `834e387cfc491c2f284ce16186b79c8944ba906a44a00a229ef813134ee5cb71`
- C++17/C++20 release build: pass
- C++17 `-fsyntax-only -Wall -Wextra -Wshadow -Wpedantic`: 警告0件
- ASan/UBSan build: pass（解答は未実行）
- Clang static analyzer: 指摘0件
- `git diff --check`: pass

Pahcer設定は`pahcer/bench_proactive_cleanup_v10.toml`、固定binaryは`/private/tmp/ahc069_width2_depth45_single_rollout_v1`。ここまで固定した解答プログラムおよびsanitizer binaryは一度も実行していない。以降はsource/binaryを変更せず、seed 0〜99の100ケースを1回だけ測定する。

### `proactive-cleanup-v10` の100ケース結果

固定したsource/binaryの結果は`pahcer/json/result_20260801_182654.json`。

- AC: `100/100`
- 合計score: `6,353,543,922`
- v9（幅16・深さ8・one-shot）比: `+1,213,496`（改善11、悪化12、同点77）
- v5（幅2・深さ32）比: `-257,991`
- v6（幅2・深さ48）比: `-3,474,549`（改善2、悪化4、同点94）
- v3（幅1・深さ48）比: `+398,998`
- 再配置なし`placement-fit-v1`比: `+2,318,503`（改善8、悪化10、同点82）
- 現在の`best_scores.json`基準の相対平均: v10 `99.53193`、v5 `99.52182`、v6 `99.57548`
- rollout turn 91回、候補131件。1候補51回、2候補40回、各case最大1回
- 整理成功18回（到着あり17、なし1）。cheap順位1位13回、順位2位5回
- rollout policy step合計`19,980`、scenario生成失敗0件
- Pahcer時間: 中央値`1.673 sec`、95番目`2.206 sec`、最大`2.389 sec`

同じ幅2で深さ48のv6とrollout turn・候補数・2候補turn数は一致した。深さを3件削るとpolicy stepは`21,312`から`19,980`へ6.25%減り、合計scoreは`-3,474,549`だった。一方、Pahcerの95番目は`4.976`から`2.206 sec`、最大は`6.299`から`2.389 sec`へ短縮した。

### 深さ52・固定予算272の公式提出

ユーザーが`CLEANUP_ROLLOUT_LENGTH=52`へ変更して公式提出した結果は、score `3,406,163,032`、実行時間`988 ms`。過去の公式score `3,407,384,159`比では`-1,221,127`（約`-0.036%`）だった。

ただし固定予算272を残した状態では、1候補の`(1+1)*2*52=208` stepは通る一方、2候補の`(1+2)*2*52=312` stepは予算gateでturn全体が棄却される。この提出は、幅2・深さ52を全て評価する版ではなかった。公式時間がローカルPahcerより十分短いことに加え、この2候補棄却も計算量を下げていた可能性がある。

## 2026-08-01: rollout数値予算を削除 (`proactive-cleanup-v11`)

ユーザー指示により、rolloutの固定policy-step上限を完全に削除する。

- `CLEANUP_ROLLOUT_POLICY_STEP_LIMIT`を削除。
- 必要stepの事前計算と上限超過によるturn棄却を削除。
- 幅2・深さ52・2 scenarioでは、2候補なら312 step、1候補なら208 stepをそのまま実行する。
- caseごとに最初のrollout比較だけを許す`rollout_used`は維持し、後続turnへは進まない。
- 旧`rollout_budget_*`診断を削除し、one-shotで2回目を止めた回数を`cleanup_rollout_reuse_blocked`として記録する。
- `cleanup_rollout_policy_steps`は上限ではなく実計算量の観測値なので維持する。

候補生成側のsearch/move/evaluation上限は今回の対象外であり維持する。候補生成、cheap順位、到着前space非悪化gate、共通scenario、baseline共有、worst-scenario margin、到着判断、validatorも変更しない。

### 実行前検証

- `main.cpp` SHA-256: `7617290e4b833a1f92e38e500eaef230549e49168bdd4ee35c10be6a379ff965`
- Clang C++17 release binary SHA-256: `0750b0b1fd36b6e06b3eae90d69c3ee50f66fbf640008691d8ffcfef8d533ff3`
- Clang C++20 release binary SHA-256: `8ad3dac8e30697697a6f726ae0cd3d27434323be6d5e226128e7f21447975696`
- ASan/UBSan binary SHA-256: `dfc2b93d71577f96747103650e5fad5f3f8c29f4fd5d901e35431372667460b3`
- C++17/C++20 release build: pass
- C++17 `-fsyntax-only -Wall -Wextra -Wshadow -Wpedantic`: 警告0件
- ASan/UBSan build: pass（解答は未実行）
- Clang static analyzer: 指摘0件
- `git diff --check`: pass
- 独立静的監査: 数値上限・必要step計算・予算guard・旧診断の完全削除、one-shot維持、2候補312 step到達を確認し、blocking issueなし

Pahcer設定は`pahcer/bench_proactive_cleanup_v11.toml`、固定binaryは`/private/tmp/ahc069_width2_depth52_no_rollout_budget_v1`。ここまで固定した解答プログラムおよびsanitizer binaryは一度も実行していない。以降はsource/binaryを変更せず、seed 0〜99の100ケースを1回だけ測定する。

### `proactive-cleanup-v11` の100ケース結果

固定したsource/binaryの結果は`pahcer/json/result_20260801_184150.json`。

- AC: `100/100`
- 合計score: `6,354,793,770`
- v10（幅2・深さ45）比: `+1,249,848`（改善7、悪化5、同点88）
- v6（幅2・深さ48・旧予算あり）比: `-2,224,701`（改善4、悪化4、同点92）
- 現在の`best_scores.json`基準の相対平均: `99.52221`
- rollout turn 91回、候補131件。1候補51回、2候補40回、各case最大1回
- 整理成功25回（到着あり22、なし3）。cheap順位1位19回、順位2位6回
- rollout policy step合計`23,088`、scenario生成失敗0件
- Pahcer時間: 中央値`1.668 sec`、95番目`2.097 sec`、最大`2.356 sec`

数値予算を外したため、深さ52でも2候補を棄却せず312 policy stepまで評価できた。公式提出時の`988 ms`から、少なくとも実行時間上はこの規模のrolloutが十分現実的だと判断した。

## 2026-08-01: 提出前cleanup・候補幅2・rollout深さ48 (`proactive-cleanup-v12`)

ユーザーが提出候補を幅2・深さ48に決定したため、アルゴリズムを変更せず、現行経路から到達しない実装を削除してコメントを整理した。

削除したものは次の通り。

- 旧通常配置探索 (`find_compact_region*`, `find_region*`)
- 旧future評価・履歴型admission control一式
- 現行cleanupから呼ばれない旧target-first relocation / repair / LNS一式
- case内one-shot化によって到達不能になったmove数・finalist総数gateと移動済み集合
- 候補幅2では到達不能な3位以降のrollout診断
- 読み出されていなかったcomponent・destination・space-profile field
- 未使用の`atcoder/all`、型alias、汎用Scanner/Emitter

`find_connected_region`、`FreeComponents`、`validate_connected_region`など、通常配置またはcleanupの現行経路で共有される部品は残した。直接`cin`/`cout`する形に単純化したが、対話出力の順序とturnごとのflushは維持している。

コメントは、shadow price、時間整合配置、退去直後の空きへの75%以上の重なり、cheapな候補選抜と共通scenario rolloutの役割分担、worst-scenario margin、厳密な退去条件`t < S`を中心に追加した。行数はcleanup前の`4,425`行からコメント追加後`2,662`行になった。

提出設定は以下。

```text
CLEANUP_FINALIST_LIMIT:         2
CLEANUP_ROLLOUT_LENGTH:        48
CLEANUP_ROLLOUT_SCENARIO_COUNT: 2
数値rollout予算:               なし
rollout機会:                   case内で最初の1回のみ
```

### 実行前検証

- `main.cpp` SHA-256: `426c9dbbabc04e749a9f6a6e4dbbca7edc015e6e3ced0459270f231fd0f99e70`
- Clang C++17 release binary SHA-256: `61ba0d5a33540e9d2a4f9609be32e1e041eeff21b5da9af7fde1b55673fa102a`
- Clang C++20 release binary SHA-256: `722f96d348f6b076c7da0c33309a864323021b23d1a0776bc34ca290e5c73bde`
- ASan/UBSan binary SHA-256: `14275270789c7b8cd3188db1ec54bbeb9e310a6b5f6431f55f023a1de8e3e247`
- C++17/C++20 release build: pass
- C++17 `-fsyntax-only -Wall -Wextra -Wshadow -Wpedantic`: 警告0件
- ASan/UBSan build: pass（解答は未実行）
- Clang static analyzer: 指摘0件
- `git diff --check`: pass
- 独立した到達性監査: `main`から到達しないトップレベル関数・定数は0件

Pahcer設定は`pahcer/bench_proactive_cleanup_v12.toml`、固定binaryは`/private/tmp/ahc069_width2_depth48_clean_v1`。ここまで固定した解答プログラムおよびsanitizer binaryは一度も実行していない。以降はsource/binaryを変更せず、seed 0〜99の100ケースを1回だけ測定する。

### `proactive-cleanup-v12` の100ケース結果

固定したsource/binaryの結果は`pahcer/json/result_20260801_190352.json`。

- AC: `100/100`
- 合計score: `6,357,018,471`
- cleanup前のv6（幅2・深さ48）および直前の手動測定と、全100 seedで同一score
- 再配置なし`placement-fit-v1`比: `+5,793,052`（改善11、悪化9、同点80）
- v11（幅2・深さ52）比: `+2,224,701`（改善4、悪化4、同点92）
- rollout turn 91回、候補131件、整理成功20回、policy step合計`21,312`
- Pahcer時間: 中央値`1.651 sec`、95番目`2.102 sec`、最大`2.306 sec`

不要実装の削除と直接I/O化の前後で100 seedすべてのscoreが一致し、cleanupによる挙動変化は確認されなかった。

### AtCoder提出結果

`proactive-cleanup-v12`を提出した。

- 提出結果: `3,401,351,097`
- 実行時間: `891 ms`
- `placement-fit-v1`比: `-6,033,062`（約`-0.177%`）
- 深さ52提出比: `-4,811,935`（約`-0.141%`）

公式暫定テストはローカル100ケースよりケース数が少ないため、解法選択にはローカルのpaired比較を優先し、公式結果は主に実行時間の確認に使う。現時点ではローカル合計で優る幅2・深さ48を基準版として維持する。

## 2026-08-01: 退去時刻で打ち切るcausal rollout (`proactive-cleanup-v13`)

再配置判断を見直すため、v12と再配置なし`placement-fit-v1`の既存100ケースを、採用された再配置イベント単位で比較した。

- 再配置20件は改善11・悪化9、純増`+5,793,052`
- 予測marginと実差のPearson相関は`+0.335`、Spearman順位相関は`-0.023`
- 上位2勝を除く98 seed合計は`-131,631`で、改善が少数の大勝ちへ集中
- 到着前space gainと実差のSpearman順位相関は`-0.564`
- 移動組の退去までの期待到着数が48未満だった11件は4勝7敗、合計`-3,307,556`

料金、移動費、不可逆な最大周長による料金低下の計上には二重計上や符号誤りはなかった。主な不整合は、移動組が数件後に退去する場合も固定48件のrolloutを続け、移動の直接原因が消えた後の高分散な配置連鎖まで利益とみなしていたことである。

### 変更した最終判断

候補生成、候補幅2、shadow admission、通常配置、case内one-shotはv12から変更しない。

候補`c`が移動する組の退去時刻を`T_mover`とし、各仮想未来では

```text
S_future < T_mover
```

の到着だけを処理する。最大到着数は従来どおり48件で、退去後の二次的な配置連鎖は評価しない。

scenarioは1組のantithetic pairから2組、合計4本へ増やす。各scenarioのbaseline/candidate未来料金差を`g0, g1, g2, g3`とし、pair平均を

```text
a0 = (g0 + g1) / 2
a1 = (g2 + g3) / 2
mu = (a0 + a1) / 2
SE = abs(a0 - a1) / 2
```

とする。最終marginは

```text
current_fee_gain - move_cost - permanent_fee_loss + mu - SE
```

で、正の候補だけを採用する。これは2個のantithetic pair平均を独立な推定単位とした「平均－1標準誤差」であり、4 scenarioでは小さい方のpair平均を未来利益として使うことと等しい。

baselineはscenarioごとに候補中の最大`T_mover`まで一度だけ進め、到着ごとの累積料金を保存する。候補は`lower_bound`で得た`S<T_mover`の件数に対応するbaseline prefixと比較する。最悪policy stepは旧288から576へ増えるが、実際には既知退去時刻で早期終了する。数値step予算による候補棄却は再導入していない。

診断には採用候補の`mu`、`SE`、`mu-SE`をそれぞれ追加した。

### 実行前検証

- `main.cpp` SHA-256: `b204d7157a6a823b4c39b4a0fe4bd38db46cbadf8e8a747a6cdf627dbcb0209b`
- Clang C++17 release binary SHA-256: `03bba15faf8d9668d5f17b296bb0dffcc3584e397108f71ed8591f2daabdf6ae`
- Clang C++20 release binary SHA-256: `7187831cd0baed7b210f2b61547752f1585042f3aaf317f288bd75a20c5a6d98`
- ASan/UBSan binary SHA-256: `1bc7c1cbb3e12ad766e4ce1528d2e1ee084bfee16eb2f2101889d182a8497bc1`
- C++17/C++20 release build: pass
- C++17 `-fsyntax-only -Wall -Wextra -Wshadow -Wpedantic`: 警告0件
- ASan/UBSan build: pass（解答は未実行）
- Clang static analyzer: 指摘0件
- `git diff --check`: pass
- 独立静的監査3系統: causal prefix、baseline共有、antithetic生成、pair平均SE、費用の一重計上、枝状態、退去境界を確認し、blocking issueなし

Pahcer設定は`pahcer/bench_proactive_cleanup_v13.toml`、固定binaryは`/private/tmp/ahc069_causal_rollout_v13`。ここまで固定した解答プログラムおよびsanitizer binaryは一度も実行していない。以降はsource/binaryを変更せず、seed 0〜99の100ケースを1回だけ測定する。

### `proactive-cleanup-v13` の100ケース結果

固定したsource/binaryの結果は`pahcer/json/result_20260801_193723.json`。

- AC: `100/100`
- 合計score: `6,353,380,768`
- v12比: `-3,637,703`（改善14、悪化13、同点73）
- 再配置なし`placement-fit-v1`比: `+2,155,349`（改善10、悪化6、同点84）
- 再配置16件。改善合計`+8,978,116`、悪化合計`-6,822,767`
- 採用候補の予測marginと実差のPearson相関`-0.018`、Spearman順位相関`-0.029`
- rollout policy step合計`29,369`（v12は`21,312`）
- Pahcer時間: 中央値`1.392 sec`、95番目`1.996 sec`、最大`2.131 sec`、2秒超3ケース

誤採用はv12の9件から6件へ減ったが、v12で大きく改善したseed 38 (`+3,294,282`) とseed 52 (`+2,630,401`) も棄却した。両seedは移動組が48件後も残るため、これらを落とした原因はcausal cutoffではなく、追加した2組目のscenarioと`mu-SE`判定である。

## 2026-08-01: causal rolloutの保守控除を削除 (`proactive-cleanup-v14`)

ユーザー指示により、v13の4 scenarioと`S_future<T_mover`のcausal cutoffを維持し、最終判断から標準誤差控除だけを外す。

変更前:

```text
margin = current_fee_gain - move_cost - permanent_fee_loss + mu - SE
```

変更後:

```text
margin = current_fee_gain - move_cost - permanent_fee_loss + mu
```

採否、候補の主比較、同margin時のtie-breakはすべて`mu`を使う。SEは採用候補の不確実性を観測する診断値としてのみ残し、判断には一切使わない。ログの意味を明確にするため、v13で`mu-SE`を表した`cleanup_rollout_future_gain`は廃止し、v14では`cleanup_rollout_future_mean`と`cleanup_rollout_future_se`を出力する。

候補生成、pre-arrival gate、候補幅2、4 scenarioの生成、pair平均、causal deadline、baseline prefix共有、shadow admission、通常配置、one-shotはv13から変更しない。

### 実行前検証

- `main.cpp` SHA-256: `28c9b77e43157840460013b338247e7e17157fb955ea01fb66421817e911727c`
- Clang C++17 release binary SHA-256: `c81607a146014e0ef1751eeb1724c31ee18842276a2fea31439d6720a76b3eca`
- Clang C++20 release binary SHA-256: `f90323d22861e808b0680dbc560ae11e6fec53b7a207ce4b21eb59646bbc220e`
- ASan/UBSan binary SHA-256: `b025d020aa14acd62e696035b7ed63674a6c6fd874eaeee4943f69861dcc9e6a`
- C++17/C++20 release build: pass
- C++17 `-fsyntax-only -Wall -Wextra -Wshadow -Wpedantic`: 警告0件
- ASan/UBSan build: pass（解答は未実行）
- Clang static analyzer: 指摘0件
- `git diff --check`: pass
- 独立静的監査2系統: 採否・順位・tie-breakが全てrisk-neutral meanを使い、SEは診断経路だけであることを確認し、blocking issueなし

Pahcer設定は`pahcer/bench_proactive_cleanup_v14.toml`、固定binaryは`/private/tmp/ahc069_causal_mean_v14`。ここまで固定した解答プログラムおよびsanitizer binaryは一度も実行していない。以降はsource/binaryを変更せず、seed 0〜99の100ケースを1回だけ測定する。

### `proactive-cleanup-v14` の100ケース結果

固定したsource/binaryの結果は`pahcer/json/result_20260801_195757.json`。

- AC: `100/100`
- 合計score: `6,358,193,055`
- v12比: `+1,174,584`
- 再配置なし`placement-fit-v1`比: `+6,967,636`

