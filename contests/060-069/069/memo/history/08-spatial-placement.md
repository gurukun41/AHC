## 2026-08-03: 隙間侵入・未来障壁回避 v24（実行前）

ユーザーの指示に従い、v23の異なる周長tier packageを混ぜず、最初に`main.cpp`だけをcommit `5145bc7`と同じSHA-256 `086cb6cc2e24848c77a4b766ab8dde433dbf16469095cca557f848bdff091c6a`へ限定復元した。その上で、「既存組と既存組の隙間へ入りながら別の場所まで伸び、周囲の組が退去した後も障壁として残る到着配置」を避ける独立方策を実装した。コミットは作成していない。

### 方針1: 池だけなら不要な長周長を全anchorで先に除く

池だけを障害物とした静的盤面を4近傍連結成分に分け、各成分・各面積`P=4..150`について、列挙済みの「矩形＋端数1行/1列」template族を全周長tierで短い順に調べる。その成分内に池だけで置ける最短template周長を`Lpond(component,P)`とする。

- 通常配置で物理的に合法なtemplate anchor、connected growth、grow-and-trimをshortlistへ入れる前に、代表セルの静的成分を引く。
- 候補周長`L > Lpond`なら、既存組に良形の場所を塞がれたために生じた不要な長周長として除く。
- `L <= Lpond`なら、池形状がその長さを必要とし得るため許可する。
- 成分面積は`P`以上でもtemplate族のどの形も置けない場合は`UNKNOWN`とし、誤ったhard rejectを避けるため全周長を許可する。
- component別にすることで、広い別成分に最小形が置けることを、細い池成分の候補へ誤って適用しない。
- 周長filterは既存の最大6候補へ圧縮する前、全ての生成済み合法候補へ掛ける。したがって、上位候補が落ちても同じtierの別anchorを拾える。
- ある拡張周長tierが物理的には置けても全anchorが周長filterで落ちた場合、そこで探索を止めず、方針を通る次tierまで進む。Controlでは全候補が通るので従来と同じ最初の物理合法tierで止まる。

静的catalogの理論上限は全`45,277` template、全anchor合計約`49,912,534`回のO(1)累積和判定である。各component/Pが解決した時点で打ち切り、解決済みcomponentのanchorは矩形照会前に飛ばす。専用CPU / wall、走査anchor数、解決pair数を記録する。

### 方針2: 現在と既知の退去直後に空きを横切る候補を除く

候補領域`C`を壁として塞ぎ、候補が属していた自由領域`F`の残り`F\\C`を候補境界からBFSする。16セル以上の成分が2個以上になれば、大きな空間を二分する障壁候補と判定する。4〜15セルの分断は置ける組が存在しても意図的に無視し、小片への過反応を避ける。

各候補は次の4断面で同じ判定を行い、1断面でも障壁ならhard filterする。

1. 現在時刻`S`
2. 候補境界に接する既存組の退去直後から最大3点
3. 退去点が3件未満なら、既存future-fitと同じ未来到着分布の1/6・3/6・5/6分位点のうち未使用時刻で補完

問題では時刻`x`に`T_j < x`の組が退去済みなので、既存組の退去時刻`T_j`はsnapshot `T_j+1`で初めてfreeにする。候補の滞在終了以上は候補自身も退去するため対象外とする。境界退去が4件以上なら、最初・中央・最後を使う。local時刻と需要分位が同一なら重複を除き、利用可能な異なる時刻が3個未満の場合だけ同一断面の重複を許す。BFS回数は常に現在1＋未来3で固定する。

池だけでも同じ候補が壁になることは「その配置が不可避」の証明ではなく、池のneckを塞ぐ有害配置でもあり得る。そのためbarrier側にはpond-only免除を置かない。池による長形の必要性は前述の周長catalogだけで扱う。

### 候補幅、拒否、再配置との接続

- 従来どおり退去時刻境界コストで最大6候補へ圧縮し、barrier判定する。
- 6件が全てbarrierなら、そのターンだけglobal上位を12件へ広げた予備shortlistから、重複を除く最大16件まで追加確認する。通常時のfuture-fit幅は変えない。
- 予備候補にも安全案がなければ方針上の全滅とする。ただしこれは全連結領域の完全探索ではなく、最初の許容周長tierで保持した有界候補集合の全滅である。
- 全滅候補のfilter前最小物理周長を保持し、Controlでも実周長料金がopportunity cost以下になる到着は`ActualFeeRejected`のままにする。今回の方針で初めて拒否される経済的候補だけを`NoRegion`として既存Push-outへ渡す。これにより負だったv22 ActualFeeRejected救済を混ぜない。
- Push-out / Compact rescueが作った完成計画は、blocker移動後かつ到着配置済みの`final_owner`で同じbarrier判定を再実行する。壁候補はrollout候補へ入れる前に捨て、通常候補を除いた直後に再配置経由で同種の壁を採用する抜け道を防ぐ。
- rescueの到着先は最小周長templateなので周長catalogを必ず満たす。完成案ではbarrierだけ再確認する。
- 方針全滅をPush-outで救えなかった拒否は物理的には配置可能である。従来の`NoRegion`探索不整合へ数えず、`gap_policy_rejected_feasible`へ独立計上する。

### 明示的な近似と限界

- barrierの未来断面は、候補境界組の退去時刻を最大3件へ圧縮する。境界外の組の退去、4件超の未選択中間状態は見逃し得る。
- 16セル未満の分断は意図的に許す。
- barrier全滅時も通常6件＋予備最大16件の有界探索であり、minimum templateが存在したターンのconnected growthや全周長tierを追加生成して安全領域を完全探索するわけではない。
- 静的`Lpond`は任意連結形状の真の最小周長ではなく、現在列挙しているtemplate族内の最小値である。template不能成分を`UNKNOWN`免除にするため、誤拒否より見逃し側へ倒す。
- synthetic rollout内でも同じ通常配置方策が使われる。実ターンの局所診断だけでは見えない方策差がroot評価へ入るため、実ターンの発火0だけをControl一致条件にはしない。

### A/B分離と診断

同じ`main.cpp`から次の4群を作り、100 seedを同じfrozen best scoreで比較する。

1. Control: `AHC069_DISABLE_GAP_AVOIDANCE`で両guard無効
2. Perimeter-only: `AHC069_DISABLE_FUTURE_BARRIER_GUARD`
3. Barrier-only: `AHC069_DISABLE_POND_PERIMETER_GUARD`
4. Combined: 両guard有効

Controlではcatalog構築、周長filter、barrier、予備幅、完成rescue再検査、counterfactual分岐が非発火し、commit `5145bc7`と候補、tie-break、future-fit、root action、stdout方策が等価である。診断用の軽微なオブジェクト構築だけは残る。Perimeter / Barrier / Combinedにより、各単独効果と`Combined - Perimeter - Barrier + Control`のinteractionを分ける。

主な新診断は次のとおり。

- 静的catalog: component、eligible/resolved/unknown/forced-long pair、anchor、CPU/wall
- 周長pre-filter: checked/allowed/filtered、UNKNOWN/forced-long免除、超過周長
- barrier: checked/safe/filtered、current/future split、backup候補/救済、detached mass
- local退去点: localあり評価数、local snapshot数
- 完成rescue: checked/filtered、current/future split、local退去点、visited cells
- synthetic込みglobal: barrier評価数、4断面数、visited cells、専用CPU
- 全滅: Actual/Economic分割、Push-out救済、最終拒否、拒否価値/cell-time

各caseで、新しい次のerror fieldを0とする。

```text
barrier checked = safe + filtered
perimeter checked = allowed + filtered
active turn = no-filter + partial-filter + no-candidate + all-filter
all-filter = counterfactual ActualFeeRejected + economic
economic all-filter = baseline policy rejection
baseline policy rejection = Push-out rescue + final rejection
final rejection = gap_policy_rejected_feasible
通常 / 完成rescue / globalのtopology snapshots = 4 * barrier evaluations
localあり評価数 <= local snapshot数 <= 3 * localあり評価数
static resolving anchors = resolved component/P pairs
```

既存のdecomposition、DLP、rescue、Push-out、grow-and-trim、root rollout / confirmationの全error / identity / validation fieldも0を要求する。

### 実行前検証と固定物

- Apple Clang C++20のCombined / Control / Perimeter-only / Barrier-only: `-O2 -DNDEBUG -Wall -Wextra -Wshadow`でpass、警告0
- Apple Clang C++17のCombined: pass、警告0
- `g++ -std=c++20` Combined: pass、警告0
- NoRegion Push-out無効版 / protected-only版: pass、警告0
- ASan / UBSan build: pass、警告0
- Clang Static Analyzer: diagnostics 0（plist 367 bytes）
- 4設定ファイルをPython `tomllib`でparseし、seed 0〜99、threads=1、固定binary pathを確認
- `git diff --check`: pass
- 合法性、call graph、退去event semantics、Control隔離、loss会計、診断保存則、時間上限を独立静的レビュー3系統で確認し、blocking issue 0

固定物は次のとおり。コミットは作成しない。

- baseline commit: `5145bc7`
- baseline `main.cpp` SHA-256: `086cb6cc2e24848c77a4b766ab8dde433dbf16469095cca557f848bdff091c6a`
- v24 `main.cpp` SHA-256: `a2a3b53a1da50af20541f85a63de833a98fe26f1d90ffca8376197c5843fcc5a`
- Combined release: `/private/tmp/ahc069_gap_v24_combined`
- Combined SHA-256: `6c08538db6f5347f803323c7334d42a4187bed1e167094c4c66280ac57e592fb`
- Control release: `/private/tmp/ahc069_gap_v24_control`
- Control SHA-256: `6bacece92b2e6b33a7b3fab62621287735d621140e3551107d8ee3071691af6d`
- Perimeter-only release: `/private/tmp/ahc069_gap_v24_perimeter`
- Perimeter-only SHA-256: `ff56c0d1dcbab2e4fd5cbc979ca40277470708da25c2734b67b1ccedcb1ae433`
- Barrier-only release: `/private/tmp/ahc069_gap_v24_barrier`
- Barrier-only SHA-256: `cb706a2b20a23ddd8d9762a496158e5141818621c52e2e03703d56b298abc23d`
- sanitizer: `/private/tmp/ahc069_gap_v24_san`
- sanitizer SHA-256: `55ed57c359ec2dfea9681678066ac46d74fba8572cb8c8bcef5389d14f682d53`
- Combined config: `pahcer/bench_gap_avoidance_v24.toml`
- Combined config SHA-256: `ad889c01c75bd191d2e908e8b1620a0d79e231928f34923dbceb66d5b5e82749`
- Control config: `pahcer/bench_gap_avoidance_v24_control.toml`
- Control config SHA-256: `6801a13c55977e8b11b5fea9279c6a868a464b0b90067abf0ffc6d328a6aba28`
- Perimeter-only config: `pahcer/bench_gap_avoidance_v24_perimeter.toml`
- Perimeter-only config SHA-256: `8d1bce7d0a599f63c0960e47fdf0977faf99c272015edc833407ae21ad2d75eb`
- Barrier-only config: `pahcer/bench_gap_avoidance_v24_barrier.toml`
- Barrier-only config SHA-256: `52e1aa7235d52eaa9baed36d04fee99b2eb8bc56425c85f72d21e5b040074869`
- Control oracle JSON: `pahcer/json/result_20260803_003818.json`（合計`6,515,194,836`）
- Control oracle stdout: `tools/out-multi-assignment-v20-control`
- tester SHA-256: `3702067f731de62b30a395f99eee04a4a9e247a1f421e2c42556a9e45b62ec92`
- frozen `best_scores.json` SHA-256: `b4c70ce5bb84ec557353d33d6c41f96e857a6d74d5f3213c685a1f602c5d1fa6`
- seed 0 / 99 SHA-256: `61857f9adeb56546a876f9f54eec3e95be617d4a1135d987ae790a2248304754` / `cfb3c1678be95ff9ff760cafed7a36c99a769c4c75c679b4a0bdef7d8edf2ad8`

### 最終実行バッチ

ユーザーの「実行まで進めてください」という明示指示に基づき、次を一つの最終バッチとして行う。

1. sanitizer Combinedをseed 0で単独実行し、ASan / UBSan、WA、全error / identity fieldを確認する。
2. same-source Controlをseed 0〜99、threads=1、frozen best scoreで実行する。
3. Controlの100 scoreを既存oracle JSON、stdoutを既存oracle directoryへ100 / 100照合する。
4. Perimeter-only、Barrier-only、Combinedを同じseed 0〜99、threads=1、同じfrozen best scoreで順に実行する。
5. Combined seed 0を同じrelease binaryで再実行し、stdout byte一致と時間項目以外の診断決定性を確認する。
6. absolute / frozen relative、勝・同点・負、seed比分位点、符号検定、paired bootstrap、内部CPU平均/p95/max/2秒超、policy funnel、拒否価値、既存スコア分解を読み取り専用で比較する。4群から周長単独、barrier単独、Combined、interactionを分ける。

sanitizer異常、WA、非0終了、score欠落、Control互換性破壊、error / identity / validation field非0の場合は、それ以降の必要な読み取り以外を行わず報告する。次の解答プログラム実行を境に、結果にかかわらずsource、方針、binary、config、memoを変更せず、読み取り、追加実行、分析、報告だけを行う。

### v24 最終実行結果

ユーザーの「実行まで進めてください」という明示指示に基づき、sanitizer seed 0、same-source Control、Perimeter-only、Barrier-only、Combinedの各100 seed、Combined seed 0の決定性再実行まで行った。macOSではLeakSanitizerが非対応のため、最初の`detect_leaks=1`指定は解答プログラムの開始前に拒否された。未対応指定を外して同じ固定sanitizer binaryを再実行し、ASan / UBSan異常なし、非0のerror / identity / validation fieldなしで完走した。これは環境上の指定訂正であり、sourceとbinaryは変更していない。

結果ファイルは次のとおり。

- Control: `pahcer/json/result_20260803_032631.json`
- Perimeter-only: `pahcer/json/result_20260803_032852.json`
- Barrier-only: `pahcer/json/result_20260803_033113.json`
- Combined: `pahcer/json/result_20260803_033438.json`

| 100 seed | Control | Perimeter-only | Barrier-only | Combined |
|---|---:|---:|---:|---:|
| absolute score合計 | 6,515,194,836 | 5,887,497,869 | 5,034,203,926 | 4,857,299,860 |
| Control差 | - | -627,696,967 (-9.6344%) | -1,480,990,910 (-22.7313%) | -1,657,894,976 (-25.4466%) |
| frozen relative合計 | 9,951.751023 | 8,908.871684 | 7,491.532705 | 7,199.452609 |
| 勝ち / 同点 / 負け | - | 0 / 0 / 100 | 0 / 0 / 100 | 0 / 0 / 100 |
| 受理数 | 71,417 | 65,110 | 61,797 | 60,129 |
| 配置形状による料金損失 | 1,098,180,013 | 494,410,625 | 270,149,201 | 158,855,547 |

seedごとのTreatment / Control比の算術平均 / 幾何平均は、Perimeter-onlyが`0.895332 / 0.892092`、Barrier-onlyが`0.753021 / 0.744997`、Combinedが`0.723653 / 0.714537`だった。p05 / median / p95は順に、Perimeter-onlyが`0.752725 / 0.910397 / 0.983609`、Barrier-onlyが`0.539492 / 0.770311 / 0.880584`、Combinedが`0.538653 / 0.739949 / 0.863680`である。3方式とも100 seed全敗で、同点除外の両側符号検定は全て`p=1.57772181044e-30`だった。

paired bootstrap 200,000回、固定乱数seed `20260803`による1 seedあたり平均score差の95%区間は、Perimeter-onlyが`[-7,067,801, -5,525,975]`、Barrier-onlyが`[-15,549,246, -14,068,059]`、Combinedが`[-17,348,535, -15,800,344]`で、全て0を大きく下回った。`Combined - Perimeter - Barrier + Control`のinteractionは合計`+450,792,901`である。これはCombinedに改善相乗効果があるのではなく、二つの拒否方策の害が重複・飽和し、単独悪化の単純和よりは悪化が小さいことを表す。CombinedもControlには全seedで負けている。

### v24 方策funnelと失敗原因

| 100 seed | Perimeter-only | Barrier-only | Combined |
|---|---:|---:|---:|
| 周長checked / filtered候補 | 117,012,487 / 471,509 | 0 / 0 | 154,245,056 / 448,893 |
| barrier checked / filtered候補 | 0 / 0 | 430,175 / 255,913 | 393,286 / 217,217 |
| backup候補 / safe rescue | 0 / 0 | 89,749 / 3,296 | 80,316 / 3,218 |
| 全候補filterターン | 22,847 | 34,312 | 36,718 |
| Controlでも実料金拒否だったターン | 1,855 | 2,142 | 1,901 |
| 今回初めて拒否対象になった経済的ターン | 20,992 | 32,170 | 34,817 |
| Push-out rescue / 最終拒否 | 1,720 / 19,272 | 2,141 / 30,029 | 2,453 / 32,364 |
| 完成rescue checked / filtered | 0 / 0 | 26,097 / 21,806 | 27,203 / 22,442 |

周長filterは全anchor比では少数しか落としていないが、良形templateが既存組に塞がれているchokepointでは候補経路をまとめて切り、その後の盤面を連鎖的に変えた。Barrier-onlyは確認した候補の約59.5%、Combinedは約55.2%をhard filterし、完成済みrescue案も大半を除外した。Combined seed 0ではbarrier filter 3,247件のうちcurrent splitが3,023件あり、悪化は退去snapshotの選び方だけでは説明できない。「現在二つの大領域へ分ける候補なら拒否」という二値規則自体が強すぎた。

score分解は、配置の幾何信号自体とhard rejectの害を明確に分けた。

| Controlとの差 | Perimeter-only | Barrier-only | Combined |
|---|---:|---:|---:|
| 受理数 | -6,307 | -9,620 | -11,288 |
| 受理集合の理想料金 | -1,226,690,185 | -2,303,223,735 | -2,590,463,795 |
| 受理cell-time | -1,855,739,130 | -3,664,764,613 | -4,168,523,883 |
| 形状損失の減少（改善） | +603,769,388 | +828,030,812 | +939,324,466 |
| 最終受理料金 | -622,920,797 | -1,475,192,923 | -1,651,139,329 |
| 移動費の増加 | +4,776,170 | +5,797,987 | +6,755,647 |
| 最終score差 | -627,696,967 | -1,480,990,910 | -1,657,894,976 |

つまり、周長・障壁信号は形状損失を減らす方向には働いた。Combinedは形状損失を約9.39億減らした一方、受理集合の理想料金を約25.90億失い、最終受理料金が約16.51億低下した。移動費増加は約676万にすぎず、再配置費や再配置後の周長悪化が主因ではない。既存shadowが経済的と判断した受入を、幾何だけのhard vetoで拒否して高価値・長cell-timeの組を失ったことが失敗原因である。

今回の実験から、「隙間へ侵入する形や大領域を分断する形を検出する信号」まで無価値とは断定できない。一方、その信号を候補集合全体のhard reject、入場拒否、NoRegion化、Push-out後完成案の拒否へ直結させる方式は明確に不採用とする。配置方策を今後検討するときは、admissionとplacementの因果を混ぜない評価が必要である。

### v24 正当性・実行時間・復元

4方式とも100 / 100 AC、`wa_seeds=[]`で、400 caseの全error / mismatch / validation fieldは0だった。same-source Controlは既存oracle `result_20260803_003818.json`と100 / 100 score一致し、旧Control stdoutとも100 / 100 byte一致した。Combined release seed 0の再実行はscore `39,772,206`とstdoutが一致し、CPU / wall / elapsed項目を除く427診断keyも全一致した。

| 内部CPU ms / case | Control | Perimeter-only | Barrier-only | Combined |
|---|---:|---:|---:|---:|
| 平均 | 1,010.909 | 1,269.376 | 1,884.124 | 1,810.635 |
| p95 | 1,334.713 | 1,647.735 | 2,257.610 | 2,192.478 |
| 最大 | 1,541.160 | 1,903.359 | 2,475.306 | 2,388.674 |
| 2,000 ms超 | 0 | 0 | 35 | 20 |

周長catalog専用CPUはPerimeter-only平均`26.768 ms`、Combined平均`26.905 ms`だった。global barrier専用CPUはBarrier-only平均`452.065 ms`、Combined平均`418.171 ms`であり、score以前にbarrierの計算量も大きい。Pahcerのwall最大はControl / Perimeter / Barrier / Combinedの順に`1.678 / 2.020 / 2.568 / 2.673秒`だったが、入出力待機を除いた上表の内部CPUを主な性能資料とする。

結果報告後、ユーザーが自分で`main.cpp`をcommit `5145bc7`相当へ復元した。復元後のSHA-256はbaselineと同じ`086cb6cc2e24848c77a4b766ab8dde433dbf16469095cca557f848bdff091c6a`であり、v24の失敗実装は現在の`main.cpp`には残っていない。v24の設計、固定artifact、結果、失敗原因は本節に保存した。エージェントはコミットを作成していない。

ユーザーは、配置にまだ大きな改善余地があると考え、問題文、復元済みbaseline `main.cpp`、本memoとともにChatGPTへ独立考察を依頼した。依頼では、現在進行中のAHC069の他参加者解法を調べず、admission、placement、repackingを分離し、現行コードの小調整に縛られない複数案と実装前の検証順を考えるよう求めた。

## 2026-08-03: v25 広域通常配置 + cell×time空間DLP（実行前）

### 背景と採用方針

v24では周長・分断の幾何信号そのものは形状損失を減らしたが、それをhard rejectへ昇格させた結果、高価値の受入を失って100 seed全敗した。ChatGPTの独立レビューも、admission、placement、repackingを分離し、幾何信号は通常配置候補の順位付けにだけ使うべきだと指摘した。

一方、コンテスト序盤で安全な局所変更だけを積み上げると別の山を探索できない。ユーザーは「焼きなましを必ず使う」という意味ではなく、大きく探索空間を広げてから失敗要因を切り分ける方針を希望した。このためv25では、同額候補だけの保守的な初版を主案にせず、料金低下を含む複数周長tierと非template連結形状を一度に比較する`Full`をprimary treatmentとする。同額限定版は原因分解用のablationとして残す。

### admission / placement / repackingの分離

1. 現行sampled DLPでtemporal opportunity costを計算する。
2. 現行`evaluate_arrival_decision`を先に一度だけ実行し、local baselineのAccepted / Rejectedを確定する。
3. local baselineがRejectedなら、新しい空間評価は何もせず同じRejectedを返す。
4. Acceptedならbaseline領域を必須候補として残し、広域候補の`現在料金 - 空間機会損失`がstrictに大きい場合だけ配置を替える。
5. treatment候補も`actual fee > temporal opportunity cost`を満たすものだけとし、元の受理根拠を壊さない。
6. 空間評価の失敗、非有限値、候補不正、同点は全てbaselineへ戻す。空間評価から`NoRegion`や入場拒否へ接続する経路は作らない。
7. v25の3 armではCompact rescue、NoRegion Push-out、通常root比較、confirmationを含む全root action / 再配置を無効にする。placementの因果効果を確認した後でのみ、既存再配置との統合を別実験にする。

この分離により、v24の失敗原因だった「配置候補の幾何評価を受理拒否へ昇格」は構造上発生しない。ただし配置差によって後続盤面が変わるため、後続ターンの現行admission判断がarm間で分岐すること自体は正常である。

### cell×time空間DLP

temporal sampled DLPは総セル時間だけを価格付けする。v25では別モデル`SpatialDlpModel`を追加し、未来要求がどのセルをどの時間層で使うかを流体緩和で価格付けする。

- 未来要求: 現行posteriorと同じ決定的low-discrepancy sample 256件から全域の中点indexを96件取り、theta層へ偏らず間引く。各sampleは`S,T,P,V`を保持し、残り組数/96の重みを持つ。
- 時間層: 最大8層。等間隔だけでなく、現在組の既知退去時刻と未来sampleの開始・終了時刻のevent分位点から境界を作る。
- 既存組: 未来要求の開始`S_r`で`T_j >= S_r`のセルを障害物とし、本体と同じ`T_j < S_r`退去規則を使う。
- future columns: 最小周長から`+8`までの各周長tierについて、列挙先頭とbounding boxが最も異なる代表2形状の全anchorを走査する。各tierから列挙両端と4象限最近を残し、全体min-hash 2列も加えて、1要求あたり最大32列を保持する。全shapeを走査しても32列へ入らない同型probeが大半だったため、現在候補の広さは変えず未来列生成だけを圧縮した。
- 双対反復: 6回。各要求は`料金 - cell×time価格`最大の列またはRejectを選び、sample重み付きloadと物理capacityの差で非負価格をprojected subgradient更新する。
- step: `0.70 * dual objective / (projected gradient norm^2 * sqrt(iteration+1))`。価格0かつunderloadの行は射影後も動かないためnormから除く。
- 安定化: 初期3反復を除いた後半3反復の平均価格を採り、solver終了後だけ`1e-9`単位へ量子化する。tieはrequest / iteration / column indexから決定的に回す。
- rebuild: 全1000ターンを16進行度windowへ分け、各window高々1回だけ配置前盤面から再構築する。成否やwall clockで再試行しない。

これは未来要求同士を完全に同時packingする整数計画ではない。少数の代表columnを持つDantzig-Wolfe型の流体緩和であり、得られるcell価格は有限候補を置いた正確な将来score差ではなく、その容量を微小に失う限界価値の近似である。また1つのsampleへ`remaining/96`を掛けるため、同じ代表列の複数copyが集中する近似も含む。今回はこの近似を小さく安全化する代わりに、32列、+8 tier、全Accepted turnでのlive利用まで広げ、本当の方向性を検証する。256要求×12反復×16再構築の約10億回規模の長倍精度処理は方針と無関係なTLE要因になるため、列幅・現在候補幅・16回の盤面更新は保ち、要求96件×6反復へ固定した。

### 現在の広域候補と金額評価

local baselineを`C0`、候補を`C`、空間価格による占有費を`OCspatial`とすると、比較量は次である。

```text
J(C) = fee(C) - OCspatial(C)

ΔJ(C) = fee(C) - fee(C0)
      - (OCspatial(C) - OCspatial(C0))
```

`ΔJ>0`の最良候補だけを採用し、同点は列挙順で動かさず`C0`を残す。旧temporal opportunity costは受理判断と候補の経済下限にだけ使い、配置間では一様な時間価格が相殺されるため二重に足さない。

現在候補は次を毎Accepted turnで独立生成する。

- baseline `C0`
- 最小周長から`+8`までの全template shape・全合法anchor
- minimum templateが置ける場合も含むconnected-growth候補
- minimum templateが置ける場合も含むgrow-and-trim候補

`+8`上限はtemplate族に対するものだけである。connected-growth / grow-and-trimは任意の連結形状なので周長が`+8`を超える場合も候補に残し、実料金と空間費でそのまま比較する。選択周長は最小周長比`+0/+2/+4/+6/+8/>+8`へ分けて診断する。

Fullでは料金がbaselineより高い、同じ、低い候補を全て許し、料金低下より空間機会損失の改善が大きければ採る。ただし候補の実料金が旧temporal opportunity cost以下なら除外する。SameFeeは全く同じsource・空間価格・候補生成を使い、`fee(C)==fee(C0)`だけに絞る。これにより、Fullが悪い場合に「空間ranking自体」と「周長を料金と交換する部分」を分けられる。

### 3 armとコンパイルスイッチ

1. Control: `AHC069_DISABLE_ALL_ROOT_ACTIONS`
2. SameFee: Control + `AHC069_ENABLE_WIDE_SPATIAL_PLACEMENT` + `AHC069_WIDE_SAME_FEE_ONLY`
3. Full: Control + `AHC069_ENABLE_WIDE_SPATIAL_PLACEMENT`

因果比較用の設定ファイルは次の3件で、全てseed 0〜99、threads=1、同じ入力・tester・frozen best scoreを使う。

- `pahcer/bench_wide_stp_v25_control.toml`
- `pahcer/bench_wide_stp_v25_same_fee.toml`
- `pahcer/bench_wide_stp_v25_full.toml`

これとは別に、無flag方策を復元済みoracleへ照合する互換性専用の
`pahcer/bench_wide_stp_v25_default.toml`も100 seedで実行する。

無flag buildはcommit `5145bc7`の提出方策と同じroot / sampled DLP / placementを通る。新コードはcompileされるがpolicy callは非発火する。因果Controlは新旧提出版との直接比較ではなく、3 arm全てから再配置を外した同一source比較である。

### 診断と実行前hard gate

新規診断では次を記録する。

- local Accepted / Rejected noop、baseline維持、valid challenger、treatment採用
- template anchors checked / legal、growth候補、scored候補
- baseline比のsame / lower / higher fee候補、temporal price除外、SameFee除外
- 選択領域でbaselineから入れ替わったセル数、source、料金差、空間費差、`ΔJ`
- spatial rebuild、request / column数、0-column要求、dual反復
- capacity / selected load / 反復中と最終量子化価格のdual objective / 最大overload
- 正価格cell-layer数、最大価格、request / column hash、専用CPU
- model失敗、非有限、候補合法性、admission invariant、各保存則error

counterの単位には意図的な違いがある。templateのtemporal / SameFee filterはanchor走査前のshape単位、`template_anchors_checked`は料金filter後のanchor単位、growthのfilterは完成候補単位である。これらを同じfunnelの保存則として足し合わせず、処理量と除外理由の記述統計として読む。

各caseで少なくとも次を0とする。

```text
wide considered = M
wide considered = local rejected noop + local accepted
wide local accepted = baseline kept + actual treatment selected
accepted count = wide local accepted
valid challenger = actual treatment selected
selected = fee class sum = perimeter tier sum = source sum
admission invariant errors = 0
invalid candidate fallback = 0
nonfinite errors = 0
spatial requests = rebuilds * 96
spatial rebuilds = 16
spatial priced turns = M
spatial zero-future turns = 1
spatial dual iterations = rebuilds * 6
root無効時のmovement cost = 0
root無効時のrelocation fee loss = 0
既存のscore decomposition / DLP / placement / loss保存則error = 0
```

初回解答実行前に、Apple Clangの無flag / Control / SameFee / Full、C++17、ASan / UBSan、static analyzer、`git diff --check`、TOML parse、binary / source / config hashを確認する。このホストの`g++`はGNU GCCではなくApple Clang 17へのdriverなので、独立したGNU GCC検証は利用不能として記録する。最初の解答プログラム実行を境に、結果にかかわらずsource、方式、定数、binary、config、memoを変更せず、読み取り、追加実行、分析、報告だけを行う。

### 既知の近似と結果解釈

- 8本のevent分位bucketはcell-time容量の緩和であり、同一bucket内の同時刻衝突を厳密には表さない。
- 空間価格は16 windowの先頭で固定するため、最大約62 turnの間に生じた受入、退去、remaining件数、posterior変化を再学習しない。終端turnは価格を無効化するが、終盤window内の過大評価は残り得る。
- `lambda * current load`は有限面積を一度に奪った正確な未来価値差ではなく、容量のmarginalな一次近似である。Fullはこの近似で現在料金低下も許すため、SameFeeより誤差の影響が大きい。
- future側は要求ごと最大32 restricted columns、current側は全anchorを調べる。この非対称性により、future列が通らず価格の付いていない`price hole`をcurrent探索が見つける可能性がある。
- 96 sampleへ序盤最大約10.4のfluid重みを掛け、双対反復も6回だけなので、整数packingの最適価格や収束を保証しない。

したがってFullが負けても「空間を価格付けして広域配置する仮説」全体を即座に棄却しない。量子化後のfinal overload / dual objective、価格coverage、選択source / tier、SameFeeとの差から、空間ranking、料金交換、価格陳腐化、restricted-column近似を分けて読む。

### 実行前の静的検証と固定artifact

コンパイラはApple Clang `17.0.0 (clang-1700.0.13.3)`、target `arm64-apple-darwin24.6.0`。releaseは次のコマンドで作成し、全て警告0で成功した。

```text
clang++ -std=c++20 -O2 -DNDEBUG -Wall -Wextra -Wshadow main.cpp -o /private/tmp/ahc069_wide_stp_v25_default
clang++ -std=c++20 -O2 -DNDEBUG -Wall -Wextra -Wshadow -DAHC069_DISABLE_ALL_ROOT_ACTIONS main.cpp -o /private/tmp/ahc069_wide_stp_v25_control
clang++ -std=c++20 -O2 -DNDEBUG -Wall -Wextra -Wshadow -DAHC069_DISABLE_ALL_ROOT_ACTIONS -DAHC069_ENABLE_WIDE_SPATIAL_PLACEMENT -DAHC069_WIDE_SAME_FEE_ONLY main.cpp -o /private/tmp/ahc069_wide_stp_v25_same_fee
clang++ -std=c++20 -O2 -DNDEBUG -Wall -Wextra -Wshadow -DAHC069_DISABLE_ALL_ROOT_ACTIONS -DAHC069_ENABLE_WIDE_SPATIAL_PLACEMENT main.cpp -o /private/tmp/ahc069_wide_stp_v25_full
```

FullのC++17 syntax、全4 modeのC++20 syntax、ASan / UBSan buildも警告0。Clang Static Analyzerはdiagnostics 0（plist 367 bytes）、`git diff --check`はpass。4 TOMLを`tomllib`でparseし、seed 0〜99、threads=1、固定binary pathを確認した。独立したGNU GCCはこのホストに無く、`g++ --version`も上記Apple Clangを返した。

固定hashは次のとおり。コミットは作成しない。

- baseline commit: `5145bc7`
- v25 `main.cpp`: `8391f1bb35ac9a137506bffd39175fd1660c0c3c2bf32b2f1b9e2448a1b659f6`
- default binary: `3f4c86b5398667895cb08645e86050f26a965d86dcdf43b9b6c6913750c41eb1`
- causal Control binary: `d073ad38c5281c60761119be3d87f2631b8e2c047d07207297fbc5422db6468e`
- SameFee binary: `066c9c5466890ae2cf6a56ca09ebefcfb7731ffd73ec40cc5fdab40a801903b0`
- Full binary: `db431447fb9e4df1d72a6154ca0252e59256076289b4c33b712e88ff9f098696`
- ASan / UBSan binary: `504122e9bb9ba8c8eba9aefaed66650b57be9ed69b407b8be7af06d4da80216a`
- analyzer plist: `7a1c7fc3acdf66f9c5b66ed5e2b241029a45bd3ee29c75bfdbb4e956b835f763`
- default / Control / SameFee / Full config: `ec59bd3c05f481a746dcfea84a5cf2a46a78dc5b918608c716e7cfd10545dad0` / `44466f2a86a34ea3621904a3a7933d07b7852633e12f75eb36ebf9623f4a81bf` / `c994da0e453ee8e7e633b80b33889fe10251c06eeee7816239adb29a810992ad` / `bac540e511086611c11ce66b2dac7600e9da58919dd10d84a5791eefaa4dbc45`
- tester: `3702067f731de62b30a395f99eee04a4a9e247a1f421e2c42556a9e45b62ec92`
- frozen `best_scores.json`: `b4c70ce5bb84ec557353d33d6c41f96e857a6d74d5f3213c685a1f602c5d1fa6`
- compatibility oracle JSON: `9b064a2c0670a2df7dc2ea153ab50fa323a178e05a4e578318773e74c645c0f3`
- compatibility stdout 100-file SHA-list manifest: `101ba85e8afbc58cae5dffc4641aef92242f7cda7da33a5839dcb3fbd308139d`
- input 100-file SHA-list manifest: `c12354a1545d49df17358688f8269b57186f9ef22317039127753f6343641773`
- seed 0 / 99 input: `61857f9adeb56546a876f9f54eec3e95be617d4a1135d987ae790a2248304754` / `cfb3c1678be95ff9ff760cafed7a36c99a769c4c75c679b4a0bdef7d8edf2ad8`

### 凍結する最終実行順・停止条件・runtime gate

1. Full sanitizerをseed 0で実行し、ASan / UBSan、WA、全error / identity fieldを確認する。
2. defaultを100 seed実行し、復元済みoracle JSONのscoreと旧stdoutへ100 / 100照合する。
3. causal Control、SameFee、Fullを順に各100 seed実行する。
4. 同じFull binaryでseed 0を再実行し、stdout byte一致と時間以外の診断一致を確認する。
5. absolute / frozen relative、勝・同点・負、seed比分位点、paired bootstrap、符号検定、score分解、fee / tier / source、final dual / overload、内部CPUを読み取り専用で比較する。

sanitizer異常、WA、非0終了、score欠落、default互換性不一致、error / identity / validation field非0なら、残りのscore runを止めて読み取り専用で原因を報告する。scoreが負であることだけでは途中停止しない。

runtimeのprimaryは対話入出力待機を除く`runtime_solver_cpu_ms`とし、Pahcer wallは参考値だけにする。Fullのp95が`1,800 ms`未満、最大が`2,000 ms`未満、2秒超0件を提出可能性のgateとする。`spatial_dlp_rebuild_cpu_ms`と`wide_scoring_cpu_ms`を分け、TLEなら方式を小さくした成功とは扱わず、この固定Fullをruntime不採用と判定する。
