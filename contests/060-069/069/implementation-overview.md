# AHC069 full + 静的5-expert + smooth限定connected polish実装の全貌

> **現在との対応:** 本文はcommit `5145bc7`の5,434行full solverを構造基準にする。現在の6,534行`main.cpp`は、v32で正だった非smoothのpolish停止を維持し、smooth expertだけ実測済みv31のdense / strict descent / scalar future-fitへ戻したv33実行済みsourceである。追加差分は§3.1、§6.7、[4-expert実装記録](memo/experiments/20260807-static-portfolio-anchor-index.md)、[v31実行記録](memo/experiments/20260807-connected-polish-root-v31.md)、[v32実行記録](memo/experiments/20260807-connected-polish-plateau-v32.md)、[v33実行記録](memo/experiments/20260808-static-polish-gate-v33.md)を参照する。

この文書は、full solverの全判断層と改善接点を確認するための設計資料である。
実験の時系列を残す `memo/`、次の会話へ状態を渡す `current-state.md` とは目的が異なる。
過去の経緯を順番に説明するのではなく、**悪化していない通常提出baselineが何を入力に、どの候補を作り、何を基準に最終行動を決めているか**を層ごとに整理する。

改善箇所を探す目的なら、まず §1 の全体フロー、§11 の改善接点、§14 の要約を読むとよい。気になる層の詳細を §3〜§10 で確認し、実際の関数へ移るときは §12 を索引として使える。

## 0. この文書が表すスナップショット

- 現在対象: [main.cpp](main.cpp)
- 現在ソース SHA-256: `3709a9de4a71111ff1a116ecfde7c4fd349a459b1bb179275d1e5ccf22d6461d`
- 現在ソース行数: 6,534行
- 構造の基準Git object: `5145bc7:contests/060-069/069/main.cpp`
- ブランチ: `069`
- v33区切りコミット: `AHC069: checkpoint v33 smooth-gated solver`（親HEADは`1fb776e`）
- 基準ソース SHA-256: `086cb6cc2e24848c77a4b766ab8dde433dbf16469095cca557f848bdff091c6a`
- 基準ソース行数: 5,434行
- 作成日: 2026-08-03 JST
- 最終同期日: 2026-08-08 JST

本文の構造基準はcommit `5145bc7` に保存された上記Git objectである。現在は一度軽量solverへ移った後fullへ戻し、実行済みの静的DLP倍率版、静的4-expert、意味保存anchor索引、限定root expertとconnected near-template polish、v32のplateau実験を経て、v33で静的smooth gateとv31 polishを合成した。各過去実験のsource・結果は `memo/experiments/` と対応artifactに残る。

2026-08-05に、`main.cpp`を触らないOptuna調整用の`main-optuna-v29.cpp`とdriverを別途追加し、2026-08-06にユーザーが固定手順を実行した。全blockが独立validationでbaselineへ戻り、最終出力も`main.cpp`とbyte一致したため、この文書が説明する通常提出baseline自体には含まれない。仕様と結果は `memo/experiments/20260805-optuna-final-v29.md` にある。

このbaselineの固定100 seed oracleは [result_20260803_003818.json](pahcer/json/result_20260803_003818.json)、合計scoreは `6,515,194,836` である。比較結果の正本は [memo/current-state.md](memo/current-state.md) にある。

現在の4-expert版は[result_20260807_220201.json](pahcer/json/result_20260807_220201.json)で100/100 AC、合計`6,621,941,047`、平均`66,219,410.47`。一つ前の静的外周適応版比`+0.083942%`、17勝76分7敗である。変更対象外のexpert 0・2は76/76 seed完全同点、expert 1は`+0.456272%`、expert 3は`+0.537166%`だった。開発100 seedのbootstrap区間は0をまたぐため、合計改善と統計的不確実性を分ける。正本は[4-expert + anchor index記録](memo/experiments/20260807-static-portfolio-anchor-index.md)にある。

実行済みv31は[result_20260807_232956.json](pahcer/json/result_20260807_232956.json)で100/100 AC、合計`6,634,405,584`、平均`66,344,055.84`。直前4-expert比`+0.188231%`、62勝6分32敗で、保存済み100/100 AC run中の新最高である。一方、paired bootstrap 95%区間`[-0.120014%,+0.498061%]`は0をまたぎ、開発100 seedは繰り返し参照済みである。限定rootとpolishは同時導入なので個別寄与も未分離とする。

実行済みv32は[result_20260808_003555.json](pahcer/json/result_20260808_003555.json)で100/100 AC、合計`6,634,181,482`、平均`66,341,814.82`。v31比`-0.003378%`、39勝18分43敗で、保存済み100/100 AC 140 run中raw合計2位である。非smooth 30 seedの`+7.256M`がsmooth 70 seedの`-7.480M`をほぼ相殺した。残差・文献・全コード経路の独立read-only監査はblocking 0で、実行時sourceはSHA-256 `b3399f41a7ebb719147cf5909f37cecf2355b549e73c6e33e90952621b898928`に凍結されていた。

実行済みv33は[result_20260808_012614.json](pahcer/json/result_20260808_012614.json)で100/100 AC、合計`6,641,661,858`、平均`66,416,618.58`。v31比`+0.109373%`、v32比`+0.112755%`で、保存済み100/100 AC 141 run中raw合計1位である。非smooth 30 seedのv32方策とsmooth 70 seedのv31方策を既存`E/G<.55`境界で選び、100/100 seedでcasewise反実仮想と一致した。新しい`.625`境界はpost-hocになるため採用していない。同じ開発集合を再利用しておりfresh保証ではない。

## 1. 一枚で見る全体フロー

無フラグ build の1ターンは次の順で進む。

~~~text
到着 i,S,T,P,V を読む
        |
        v
T-S を theta 推定へ追加し、残り組の打ち切り情報も含めて再推定
        |
        v
t < S の退去済み組を owner から除去
        |
        v
temporal sampled DLP
今回が使う P × [S,T) の時間容量に機会損失を付ける
        |
        v
通常 admission + placement
  1. 最小周長料金による上界reject
  2. 通常配置候補の生成・選択
  3. 実周長料金によるreject
        |
        v
baseline ArrivalDecision（通常案）を作る
        |
        v
root action / repacking
  - Compact rescue
  - NoRegion Push-out
  - 通常配置runner-up
  を共通未来rolloutでbaselineと比較
        |
        v
最終 TurnPlan を apply_plan で一度だけ実盤面へ反映
        |
        v
意思決定に使わない損失分解・整合性・CPU診断
        |
        v
移動と Yes/No を出力して flush
~~~

### 四つの意思決定層

| 層 | 答える問い | baselineの中心評価 | 主な出力 |
|---|---|---|---|
| admission | この組を受け入れる価値があるか | 実料金と temporal sampled DLP の機会損失 | `ArrivalStatus` |
| placement | 受け入れるならどこへ置くか | 周長、退去時刻境界、future-fit | 到着領域と周長 |
| repacking | どの既存組をどこへ動かせるか | 移動費、既存料金維持、合法な同時移動 | rescue候補 `TurnPlan` |
| root selection | baseline、rescue、別配置のどれを採るか | 現在の直接差 + 共通未来rolloutの料金差 | 最終 `TurnPlan` |

コード上では実周長を知るため placement が admission の途中に呼ばれる。選択した周長と実料金は `ActualFeeRejected` へ接続する一方、同一周長内の位置評価である退去時刻境界とfuture-fitはhard rejectへ接続しない。repackingを通常判断の後段に置くことと合わせ、これがbaseline設計の重要な境界である。

## 2. 問題上の状態と実装の不変条件

問題サイズは `N=50`、`M=1000`、`4<=P<=150`、時刻上限100000で固定されている。baselineの配列長、bit mask、DLP前計算、各探索上限はこの固定制約を前提にしている。

### 2.1 盤面

`owner[x][y]` は占有中の組ID、空きセルは `-1` である。池も `owner=-1` なので、合法性判定では常に別の `park[x][y]` と組み合わせる。

受理済み組の退去は `departures` という `（退去時刻, 組ID）` のmin-heapで管理する。問題のイベント順に合わせ、現在の到着時刻が `S` のとき解放するのは `t < S` の組だけである。`t == S` の組は今回の配置中にはまだ盤面を塞ぐ。

### 2.2 グループ

`GroupState` は次を持つ。

| フィールド | 意味 |
|---|---|
| `active` | 現在利用中か |
| `s, t` | 到着・退去時刻 |
| `v, p` | 基本支払額・必要面積 |
| `cells` | 現在の占有領域 |
| `max_perimeter` | 過去に経験した最大周長 |

最終料金は滞在中の最悪コンパクト度で決まる。このため移動後に形を良くしても料金は回復せず、`max_perimeter = max(old, new)` を保存する。

### 2.3 行動

`MovePlan` は既存組1つの移動先、`TurnPlan` はそのターンの全移動と到着組の領域をまとめる。

複数移動では、すべての移動元を消してから、すべての移動先を置く。したがって組同士の位置交換も扱える。候補探索中はコピー盤面だけを変更し、実 `owner` を変更するのは最終的な `apply_plan()` 一箇所である。

### 2.4 金額

周長 `L` の領域へ置いた組の料金は、

~~~text
payment(V,P,L) = round(4 × V × sqrt(P) / L)
~~~

である。`round_payment()` は浮動小数点で初期値を求めた後、128 bit整数で丸め境界を前後補正し、1円の誤差を防ぐ。

1回の移動費は、

~~~text
move_cost(V) = max(round(R × V), 1)
~~~

である。入力 `R` は最初に1000倍整数 `r_milli` へ変換し、こちらも128 bit整数で確定する。

ケースのraw scoreは、

~~~text
raw score
  = accepted groups の最終料金合計
  - 全移動費
~~~

である。各層が主に動かす損失項は次のように対応する。

| 層 | 主に動かすもの |
|---|---|
| admission | どの理想料金をRejectとして失うか |
| placement | 受理組の初期shape loss |
| repacking | 既存組の恒久料金低下と移動費、到着組のshape改善 |
| root selection | 上の候補間で、短期差と予測未来差のどちらを採るか |

入力 `R` を意思決定に使うのはbaselineではrepackingだけで、通常admissionと通常placementは `R` を見ない。また通常placementは、同一最良周長の場所を選ぶ段階では `V` を見ない。`V` はadmission料金と、周長が変わる候補・移動を金額比較するときに効く。

## 3. 初期前計算と形状表現

### 3.1 rectangle + strip テンプレート

`Shape` は面積 `p` を「主矩形 + 端数を埋める1行または1列」で表す。
`make_template_shapes(p,n)` は幅、転置、端数を付ける上下左右を列挙し、周長順にsortして重複を除く。

各 `p=4..150` について次の二種類を前計算する。

| 配列 | 内容 | 主な用途 |
|---|---|---|
| `all_shapes[p]` | 盤面に入る全テンプレート | repackingでの既存組の移動先 |
| `compact_shapes[p]` | 最小周長 `Lmin` から `Lmin+4` まで | 通常 placement、admission、rollout |

現在の通常template placementとrescue targetは、池または占有セルをN=50の行bitsetへ詰めた`LegalAnchorIndex`を使う。幅ごとの衝突開始bitを作り、行方向のOR sparse tableで主矩形と端数矩形のinvalid maskをまとめて得る。補集合を`ctz`で走査するため、従来の二次元累積和と合法集合・`base_x/base_y`順・tie-breakを変えず、合法anchorだけを列挙できる。

二次元累積和自体は、rescue targetの占有数・概算移動費rankingとblocker移動先の合法性など、矩形内の個数が必要な処理に残る。

### 3.2 完全な連結フォールバック

テンプレートだけでなく、`find_connected_region()` が空き連結成分をBFSし、先頭 `p` セルを返す。
BFSのprefixは常に連結なので、`p` セル以上の空き連結成分が存在すれば、少なくとも1つは必ず合法領域が得られる。

したがってbaselineの `NoRegion` は「良いテンプレートがない」という意味ではない。現在盤面に `p` セル以上の空き連結成分が本当に存在しない状態である。

## 4. 未来分布と theta 推定

テストケースごとに未知の滞在時間スケール `theta` が1つあり、公式生成では2000から8000である。`ThetaEstimator` は、

~~~text
theta = 2000, 2100, ..., 8000
~~~

の61点事後分布を持つ。

### 4.1 観測

到着時には `T` も入力されるため、その組を受理する前に `D=T-S` を観測できる。毎ターン、

- 観測数
- `D==1` の数
- `sum(D-1)`

を更新する。

さらに「まだ到着していない残り全組の開始時刻は現在の `S` より後」という打ち切り情報を尤度へ入れる。通常の点推定は各 theta の事後重み付き平均で、placementの退去時刻評価やscreen rolloutの生成に使う。

### 4.2 二種類の事後計算

baselineコードには、用途が異なる二種類の theta 計算がある。

| 計算 | 特徴 | 使用先 |
|---|---|---|
| `estimate()` / `posterior_quantile()` | 48点求積による連続近似と打ち切り尤度 | placement、screen/confirmation rollout |
| sampled DLP の `exact_posterior_quantiles()` | 公式の整数丸めで `l=0` と `l>=1` を分ける | DLP未来要求の theta 10/30/50/70/90%点 |

`estimate()` 内には、計算順の微差で配置のtie-breakが変わらないよう、似た事後計算を意図的に別実装した箇所がある。

## 5. admission: temporal sampled DLP

無フラグ build の admission価格は `SampledDlpShadowModel` が作る。これは盤面上の場所を区別せず、「何セルを何時間使うか」だけを価格付けする。

### 5.1 未来256要求

各再構築時に、現在までの観測だけから未来要求を256件、決定的に生成する。

| 要素 | 生成方法 |
|---|---|
| theta | 事後10/30/50/70/90%点へ51/51/52/51/51件を割当て |
| 滞在時間 | base 2 radical inverseと、`start>S` で条件付けた公式丸め後CDF |
| 開始時刻 | base 3 radical inverse、一様 |
| 面積 | base 5、公式の `round(uniform(2,sqrt(150))^2)` |
| 価値 | base 7から標準正規 `Z` を作り、`V=clamp(round(P D^0.9 2^(0.8Z)),1,10^8)` |

乱数器は使わない。同じ状態なら同じ要求列になる。

各サンプルの価値 `F_j` は最小周長での料金、負荷 `a_jb` は時間bucket `b` との重なり時間に面積を掛けたものとする。サンプル重みは `remaining_groups / 256` である。

### 5.2 容量と双対

現在時刻から100000までを最大16等分し、各bucketの容量を、

~~~text
C_b
  = max(0,
        grass_cells × bucket_length
        - active groups が既に予約している cell-time)
~~~

とする。

実質的に解いている双対目的は、

~~~text
min λ>=0
    Σ_b λ_b C_b
  + weight × Σ_j max(0, F_j - Σ_b λ_b a_jb)
~~~

である。

8回のGauss-Seidel型座標sweepを行う。1つのbucketについて、他bucketの価格を引いた残余価値をそのbucket負荷で割り、未来要求が離脱するbreakpointを並べる。残る需要が容量以下になる価格を採用する。全sweep後だけ価格を `1e-9` 単位へ量子化する。

### 5.3 再構築タイミング

価格は毎ターン解き直さない。

- 初回
- `turn=4,8,16`（0-origin、5・9・17番目の到着）
- それ以降の `turn=32,48,...`（16ターンごと）
- 前回の時間bucket境界を2本以上通過したとき

に再構築し、それ以外はキャッシュを使う。

### 5.4 現在組の機会損失

現在の組が面積 `P`、滞在区間 `[S,T)` のとき、

~~~text
opportunity_cost
  = P × Σ_b λ_b × overlap([S,T), bucket_b)
~~~

とする。未来組が0なら0である。

### 5.5 初期外周率で固定するDLP expert

sampled DLPは芝セルを位置によらない流体容量として扱うため、初期盤面の外周率で価格水準を3段階に固定する。

~~~text
G = 初期芝セル数
E = 各芝セルから池または盤外へ出る4近傍辺の総数

100E < 55G               なら scale = 1.30
55G <= 100E < 70G        なら scale = 1.25
70G <= 100E              なら scale = 1.00
~~~

分類は到着列を見る前に一度だけ行い、ケース中は変えない。`evaluate_arrival_decision()`が値渡しで受け取った`opportunity_cost`へ入口で1回掛けるため、実到着とroot rollout内の仮想到着へ同じ倍率が届く。NoRegion Push-outは通常admissionを通らない経済gateも持つので、`choose_root_action_with_rescue()`入口のraw値へ別途1回掛ける。callerのshadow値自体は変更せず、二重掛けもしない。

最初の1.30 / 1.00境界は固定binaryのfresh validation 300 seed cacheから選ばれ、旧full比`+2.018205%`だった。追加の1.25領域を含む現在portfolioは同じ保存validation cacheで一つ前の静的DLP版比`+0.087686%`、開発100 seed実測は`+0.083942%`だった。ただし閾値選択にも同じcacheを見ており、今回100 seedも反復利用した開発集合なのでfresh保証ではない。根拠とCPUリスクは[現在実装の記録](memo/experiments/20260807-static-portfolio-anchor-index.md)に分離している。

### 5.6 admissionの厳密な三段階

`evaluate_arrival_decision()` は次の順で判定する。

| 段階 | 条件 | 結果 |
|---|---|---|
| 1. 上界料金 | `payment(V,P,Lmin) <= scale * opportunity_cost` | `UpperBoundRejected` |
| 2. 配置可能性 | 通常 placement が領域を返さない | `NoRegion` |
| 3. 実料金 | `payment(V,P,Lchosen) <= scale * opportunity_cost` | `ActualFeeRejected` |
| 通過 | 実料金がstrictに機会損失を上回る | `Accepted` |

同額は拒否する。通常配置runner-upも料金が機会損失以下なら後段へ渡さない。

### 5.7 旧64 bucketモデル

`AHC069_DISABLE_SAMPLED_DLP` buildでは、admission価格が `DensityModel`、`FutureBucketDemand`、`evaluate_shadow_cost()` を中心とする旧モデルへ戻る。これは64固定時間帯の将来cell-time超過率を計算し、価値密度の対数正規近似から価格を付ける。通常提出では通らず、A/B比較用に残されている。

`ConditionalFutureDemand` 自体は旧モデル専用ではない。無フラグbaselineでも通常placementの退去時刻境界・future-fitと、root未来生成の一部で共有する。

## 6. placement: 通常配置の候補と順位

中心は `choose_temporally_coherent_region()` である。選択は大きく、

~~~text
実現できる最小周長
    -> 同一周長内の退去時刻境界
    -> 同一周長内のfuture-fit
~~~

という辞書式になっている。

### 6.1 周長ladder

1. `Lmin` の全テンプレート・全アンカーを走査する。
2. 1件でも合法なら、extended templateもconnected-growthも作らない。
3. `Lmin` が全滅なら、`Lmin+2`、`Lmin+4` の順に、最初に合法templateが出たtierまで走査する。
4. 同時に connected-growth と grow-and-trim も生成する。
5. 全生成元を合わせ、見つかった最小周長だけを残す。

通常 placement に渡す `compact_shapes` は `Lmin+4` までなので、template ladderもそこまでである。連結成長はそれより悪い周長を作る場合がある。

### 6.2 connected-growth

`make_connected_growth_candidates()` は次の手順で、矩形テンプレート以外の連結領域を作る。

1. 空き連結成分を列挙する。
2. BFS先頭 `p` セルを完全フォールバック候補として入れる。
3. 池・占有セル・盤面端からの距離を多点BFSで求める。
4. 各成分の上下左右、4対角、障害物最近・最遠という10種類の特徴点を作る。
5. 重複を除き、通常expertは最大16、高外周率expertは最大24 seedを選ぶ。
6. 選択済み隣接数が多いfrontierセルを優先し、連結なまま `p` セルへ成長する。

seed、向き、座標によるtie-breakはすべて決定的である。

### 6.3 grow-and-trim

seedからの成長が `p` セルへ達した領域は、まずconnected-growth候補として残す。その後、成功したseed成長の先着最大8試行を、通常expertは`p+8`、高外周率expertは`p+12`セルまで追加成長し、`p` セルへ削る。領域が既存候補と重複していても試行枠は消費し、最初に追加するBFS完全フォールバック領域そのものはtrim対象ではない。

削除のたびにTarjan DFSで関節点を求める。関節点と内部セルを避け、選択済み近傍数を `k` としたときの周長変化、

~~~text
delta_perimeter = 2k - 4
~~~

が小さい境界セルから削る。最後に面積、連結性、池、占有を再検証する。trim版は元候補を置換せず、追加候補として扱う。

### 6.4 退去時刻境界コスト

記号を、

~~~text
A(u) = 現在より後に始まる未来組が時刻uまでに始まる条件付き確率
R(u) = 1 - exp(-(u-S)/theta)
~~~

とする。

候補の退去時刻を `T`、隣接既存組の退去時刻を `t_g` とし、候補外周にできる退去レベル差を評価する。実装はセルごとの値と二次元累積和を作り、候補内部辺を `4P-L` で補正する。

候補に入り得る空きセル `x` から、盤内かつ非池の4近傍だけを見て、近傍が空きなら `A(t_g)=R(t_g)=0` と置く。

~~~text
incremental_cell[x] += |A(T)-A(t_g)| - A(t_g)
absolute_cell[x]    += |R(T)-R(t_g)|

incremental_cost(C)
  = Σ[x in C] incremental_cell[x] - (4P-L)A(T)

absolute_cost(C)
  = Σ[x in C] absolute_cell[x] - (4P-L)R(T)
~~~

概念的には、

~~~text
incremental_cost:
    候補を足したことで増える |A(T)-A(t_g)| の境界不整合

absolute_cost:
    配置後に残る |R(T)-R(t_g)| の境界不整合
~~~

で、どちらも小さいほど、近い時期に空く領域がまとまりやすいとみなす。

### 6.5 最大6または8候補への圧縮

`PlacementShortlistBuilder` は、全候補を高価なfuture-fitへ渡さず、現在までに見た最小周長だけを保持する。より小さい周長が現れた時点で、それまでの候補はすべて捨てる。

最終shortlistは次の和集合で、通常expertは最大6件、高外周率expertは最大8件である。

- incremental cost 上位3件、高外周率expertでは上位5件
- absolute cost 最良
- その最良周長で列挙された最初の候補
- 主候補とは別象限のincremental最良

hashで絞った後にセル集合も比較し、同一領域を除く。shortlist内はすべて同じ周長なので、同じ `P,V` なら料金も同じである。

### 6.6 future-fit

候補が2件以上あり、未来組と十分な未来到着確率があるときだけ、今回の滞在中に始まる未来到着質量の、

~~~text
1/6, 3/6, 5/6
~~~

分位を3 snapshotにする。

各snapshotでは、

- 今回の候補セルは占有扱い
- 既存組は `t < snapshot` なら空き扱い
- 一辺 `2,3,4,5,6,8,10,12` の空き正方形を置ける位置数をDPで数える

とし、

~~~text
U(snapshot)
  = Σ_side side^2 × log(1 + count_side) / Σ_side side^2

future_fit
  = (1-w) × average(U)
  + w × min(U)

w = 0.25（通常expert）または0.50（E/G>=0.80 expert）
~~~

を最大化する。同点はincremental cost順である。

### 6.7 connected near-template polish

従来のtemplate / connected候補選択とfuture-fitを最後まで実行した後、実ターンのold primaryが次を全て満たす場合だけ追加探索する。

- 初期`E/G<0.55`のsmooth expert 0
- `P>=50`
- sourceが`ConnectedGrowth`または`GrowAndTrim`
- 旧丸め後料金がscaled opportunity costをstrictに上回る
- 旧周長が`Lmin`より大きい

初期盤面だけでsmoothかを固定し、到着列や途中scoreでは切り替えない。v31の100 seed差は`E/G<.55`で`+18.59M`、`.625-.70`で`-1.71M`、`>=.80`で`-4.64M`だったため、正の中心を残しつつ大損tailが出た地形を静的に保護する。これは繰り返し参照した同じ開発集合上のpost-hoc根拠であり、fresh性能推定ではない。

synthetic rolloutでは発火しない。root内の数百policy stepへ局所探索と全盤面box走査を掛けず、未来比較方策の計算量を従来どおりに保つためである。

主候補のdense deformationは、理論最大料金改善`U=fee(V,P,Lmin)-old_fee`が10,000以上で、ケース中の実dense走査が24回未満のときだけ発火する。`P<=h*w<=P+16`、`2(h+w)<=min(Lold-2,Lmin+4)`のboxを全anchor走査し、50-bit free rowからbox内free集合の周長`4q-2e`を計算する。global上位12と四象限補完を合わせた最大16 anchorだけを正確に調べる。box内にPセル以上のfree成分があれば、ちょうどPならそのまま、より大きければTarjan関節点を毎回再計算しながらPまで削る。incremental優先とabsolute優先の2種類を作り、面積、連結性、池、占有、旧周長strict改善を再検証する。Uを通過した実走査だけが24回予算を消費する。

補助候補のperimeter descentは、old selectedの非関節セルと合法frontierセルを交換する。先にfrontierの最大selected近傍数とselected境界の最小近傍数を求め、前者が後者以下なら正のswapは不可能なのでTarjanと全組合せを省く。必要条件を通った後、削除前近傍数`k_r`、削除後の追加近傍数`k_a`に対して`k_a>k_r`だけを選ぶので、各stepの周長差は`-2(k_a-k_r)<0`である。最大8 step反復し、終端だけでなく全strict降下中間形を候補にする。保存15,330試行の最大は6 stepであり、旧候補を失わずworst caseを抑えつつ、深い形のfuture-fitが落ちても浅い短縮を残せる。descentはdense予算と分離して全eligibleへ適用する。

denseとdescentは周長tier別の共通shortlistへ入れる。丸め後料金がoldよりstrictに高い候補だけを短い周長tierから評価し、各tierでは通常placementと同じ3 future snapshotのscalar future-fitを最大化する。tier最良がoldのscalar値以上なら採用し、不合格なら次tier、最後までなければoldへ戻る。既に通常候補比較でoldを評価済みならその値を再利用する。

polishを選んでもold connectedをroot runner-upの先頭に残し、旧周長をCompact rescueとnormal-rootの発火参照、旧cellsをrescue移動先rankingへ使う。rescueのdirect gain自体は改善後polish料金と比較する。これはglobal scoreの数学的単調性ではないが、RejectをAcceptへ変えず、即時料金、既存future-fit proxy、旧root探索機会を同時に保護する限定更新である。v32で試したzero-gain plateau、24要素Pareto、component-capacity guardはsmooth 70 seedの合成差が負だったため現行sourceから撤去した。

### 6.8 通常runner-up

実ターンでは、同じ評価済みshortlistから次点を最大2件返す。従来shortlist内のrunner-upは同一周長・同一料金である。polish採用時だけはold connectedを第1rollback候補にするため周長・料金が異なり得るが、old自身が経済gateを通ることを事前確認し、後段rootで現在差と未来盤面差を共通比較する。

仮想未来の placement ではrunner-upを返さず、root探索を再帰的に呼ばない。

## 7. repacking: Compact rescue と NoRegion Push-out

再配置は常時の盤面整理ではなく、現在の到着を契機にした限定的な救済である。

### 7.1 どの状態が後段で救済されるか

| baseline状態 | 後段の可能性 |
|---|---|
| `UpperBoundRejected` | 救済しない |
| `ActualFeeRejected` | 救済しない |
| `NoRegion` かつ総空き面積 `<P` | 救済しない |
| `NoRegion` かつ総空き面積 `>=P` | NoRegion Push-out候補 |
| `Accepted` かつ周長 `>Lmin+4` | Compact rescue候補 |
| `Accepted` かつ周長 `>Lmin` | 条件付きで通常runner-up比較 |
| `Accepted` かつ周長 `==Lmin` | 通常はそのまま |

NoRegion Push-outは、通常判断の `NoRegion -> Accepted` を変え得る唯一の経路である。
ただし `総空き面積>=P` は必要条件にすぎず、静的な池地形で置けないケースもこのgateを通り得る。その場合は後段のtarget生成で候補なしになる。

### 7.2 到着組の目標領域

`make_rescue_targets()` は、到着組を最小周長templateへ置く全アンカーを走査する。池との衝突は許さないが、既存組との衝突は一旦許す。

全アンカーを保存せず、累積和で、

- 衝突セル数が少ない
- 衝突セルへ按分した概算移動費が小さい

という二基準の上位を固定長heapへ残す。その和集合についてだけ、正確なblocker集合と移動費を復元する。

Compact rescueの直接差は、

~~~text
compact arrival fee
  - baseline arrival fee
  - blocker movement costs
~~~

Push-outの直接差は、

~~~text
compact arrival fee
  - blocker movement costs
~~~

である。Compactはstrictに正の候補だけ、Push-outはさらにこの値が temporal opportunity cost をstrictに上回る候補だけを残す。

Push-outでは全anchor走査の前にも、`compact fee - active組の最安1回移動費 <= opportunity cost` なら打ち切る安い上界filterがある。

正確なtargetは、直接差の降順、blocker数の昇順、Push-outではさらにblocker総面積の昇順、最後に列挙順でsortする。その先頭から実際に移動先を修復するのはCompact 8件、Push-out 4件までである。

### 7.3 blockerの移動先

目標到着領域を先に固定し、衝突するblockerをすべて一時撤去した盤面で、各blockerの移動先を作る。

移動後の料金が現在までに確定した料金と同じであることがhard制約である。

~~~text
payment(v,p,max(max_perimeter,new_perimeter))
==
payment(v,p,max_perimeter)
~~~

丸めのplateau内なら `new_perimeter > max_perimeter` もあり得るが、料金は低下しない。baselineのrescueは既存組の恒久料金低下を受け入れて別の利益と交換することはない。

移動先候補は次の順でsortする。

1. Compactではbaseline到着領域、Push-outでは元から空いていた領域との重なりが大きい
2. blocker撤去で空いたセルの再利用量が大きい
3. 外周の退去時刻境界costが小さい
4. 周長が小さい
5. 列挙順

その後、sort先頭4件、各4象限の最良、残りの順位順を重複除去しながら追加し、Compactは最大10件、Push-outは最大8件へ絞る。単純な上位切りではなく、盤面上の位置多様性を明示的に残す。

anchorは組ID・到着ID・shapeから決まる互いに素なstrideで巡回し、特定shapeだけが予算を使い切らないようにする。

到着targetもblockerの移動先もrectangle + stripテンプレート限定である。通常placementのconnected-growth / grow-and-trimはrepacking候補には使わない。

### 7.4 同時移動の組合せ

各blockerの候補poolから、互いに重ならない移動先を選ぶ。領域は最大50×50 bitの `BoardMask` で表し、重複をword単位で判定する。

まず、

- 候補poolの小さい順
- 面積の大きい順
- 退去の早い順
- 退去の遅い順

など複数の挿入順でgreedyを試す。greedyが失敗した場合だけbeam searchで修復する。

beam rankは、

~~~text
1000 × preferred-area overlap
+ 10 × cleared-area reuse
- temporal boundary cost
- 0.01 × perimeter
~~~

で、beam幅は32である。

### 7.5 探索上限

| 上限 | Compact rescue | NoRegion Push-out |
|---|---:|---:|
| target shortlist / 指標 | 160 | 96 |
| 修復するtarget | 8 | 4 |
| blockerごとのanchor | 4096 | 2048 |
| 1ターンの全destination anchor | 50000 | 16000 |
| blockerごとの合法destination | 64 | 40 |
| 最終destination | 10 | 8 |
| beam node | 2048 | 1024 |
| 完成root候補 | 2 | 2 |

### 7.6 独立した合法性検証

完成計画は `validate_and_build_rescue_owner()` が元盤面から作り直す。

- 移動組IDの重複
- 元領域と `owner` の一致
- 全移動元を消した後の面積、連結、池、重複
- 到着領域の面積、連結、周長
- 実際の移動費
- 既存組の料金低下
- `TurnPlan::immediate_gain`

を再計算する。探索中の近似差分だけで採用することはない。現在のCompact/Push-out候補は既存料金低下0も必須である。

`TurnPlan::immediate_gain` が保持するのは `到着料金 - 移動費` であり、baselineとの差ではない。既存料金低下0は別の検証条件として課す。Compactのbaseline比 `direct_gain` は、`immediate_gain` からbaseline到着料金を引いて別に計算する。Push-outではbaseline料金が0なので両者が一致する。

## 8. root action: 共通未来による上書き判定

### 8.1 screen未来の作り方

現在までの観測だけから、base 2/3/5/7の低食い違い列で未来全組を一度生成し、開始時刻順にsortして先頭だけを使う。

screenは、

- 2シナリオ
- 反対変数の1ペア
- 各4未来到着
- 潜在thetaは現在の点推定

である。すべてのroot branchへ同じ未来列を与える。

仮想未来では、

- 通常 admission / placementだけを実行
- 再配置やrunner-up rootを再帰呼び出ししない
- temporal DLP価格は実ターン時点のものを凍結
- syntheticな滞在時間を1件ずつtheta推定へ追加

する。

比較対象の未来価値は、仮想未来で受け入れた組の料金合計である。

### 8.2 rescueのscreen

rescue候補 `c` のbaseline比は、

~~~text
screen_margin(c)
  = current_direct_gain(c)
  + (future_delta_0(c) + future_delta_1(c)) / 2
~~~

である。実装は丸めを避けるため、

~~~text
2 × current_direct_gain + future_delta_0 + future_delta_1
~~~

を整数のまま比較し、strictに正の最良候補だけを採用する。

未来組が0なら、直接差が正の第1候補をそのまま使う。シナリオ生成に失敗した場合、元々AcceptedのCompact rescueは直接得な第1候補を残し得るが、RejectをAcceptへ変えるPush-outは元のRejectを維持する。

未来がありシナリオ生成にも成功した場合、rescueの最終filterはこの2シナリオscreenであり、後述の8シナリオconfirmationは行わない。

### 8.3 通常runner-upとprotected branch

Compact rescueのscreenには通常配置runner-upも最大2件参加できる。ただし、まず `baseline vs rescue` の勝者をprotected branchとして決める。

runner-upが2シナリオscreenでprotected branchを上回った場合だけ、独立holdoutへ進む。安いscreenに偶然合った通常配置が、既存のbaseline/rescueを直接上書きしない構造である。

通常runner-upはbaselineと同料金なので、baseline比の現在直接差は0である。protected branchがrescueなら、runner-upはそのrescueの直接益だけ不利な状態からscreenとholdoutで逆転する必要がある。

rescue候補もscreenも発生しなかったターンでは、`choose_normal_root_action()` が通常runner-upだけをbaselineと比較できる。計算量制御のため、コンテスト進行度を4区間に分け、各区間で高々1回だけ試す。

### 8.4 holdout confirmation

通常runner-upの上書き確認は、

- 8シナリオ
- 反対変数4ペア
- 各12未来到着
- theta事後分位点をpairごとに使用
- screenとは別の低食い違い列block

で行う。

~~~text
8 × current_direct_delta
+ Σ_{scenario=0..7} future_delta_scenario
> 0
~~~

の場合だけ上書きする。1ケースのconfirmation試行は最大4回である。生成失敗、予算超過、平均非正ならprotected branchへ戻る。

### 8.5 rootが現在扱わないもの

baselineのroot探索には次の制約がある。

- arrivalと無関係なproactive cleanupはしない
- `UpperBoundRejected` を復活させない
- `ActualFeeRejected` を復活させない
- 現在の直接差が非正のrepackingを、長期利益だけを理由に候補化しない
- 既存組の料金低下を許さない
- 仮想未来内では追加のrepackingをしない
- rescueの採用には8シナリオholdoutを使わない

これらは不具合ではなく、現在の候補空間そのものを定義している。

## 9. コンパイル時の構成

無フラグ時の値は次のとおり。

| 機能 | 無フラグ |
|---|---:|
| sampled DLP | 有効 |
| 静的DLP expert | `E/G<.55`は1.30、`.55<=E/G<.70`は1.25、その他1.00 |
| 静的placement expert | `E/G>=.80`だけ候補幅・growth・minimum future-fitをp2へ切替 |
| 静的root expert | `.70<=E/G<.80 && R<.060`だけ未来料金差重みを1.00から0.10へ切替 |
| connected polish | `E/G<.55`の実ターンでold Accepted connectedかつ`P>=50`だけ有効。dense `U>=10,000`・最大24実走査、strict descent最大8 step |
| grow-and-trim | 有効 |
| NoRegion Push-out | 有効 |
| root actions / rescue | 有効 |
| 通常runner-up拡張 | 有効 |

比較用マクロは次のとおり。

| マクロ | 効果 |
|---|---|
| `AHC069_DISABLE_SAMPLED_DLP` | temporal sampled DLPを旧64 bucketモデルへ戻す |
| `AHC069_DISABLE_GROW_AND_TRIM` | trim追加候補を無効化 |
| `AHC069_DISABLE_NO_REGION_PUSHOUT` | NoRegion救済を無効化 |
| `AHC069_PROTECTED_ONLY` | root内の通常runner-up拡張を無効化 |

baselineにはroot actions全体を止めるcompile-time switchはない。`AHC069_PROTECTED_ONLY` が止めるのは通常runner-up拡張だけで、Compact rescueとNoRegion Push-outは残る。棄却済みv26/v27の方策flagとv28 collectorは、現在の`main.cpp`から撤去済みである。

別ファイル`main-optuna-v29.cpp`は、baseline defaultを持つ8個の`AHC069_TUNE_*`マクロを持つ履歴artifactである。driverはDLP倍率をadmission、候補数・future-fit重みをplacement、未来差重みをrootとして分けて調整した。全blockは当時の固定gateで不採用だったが、保存cacheを地形別に再構成し、現在`main.cpp`はDLP 1.30 / 1.25 / 1.00と、`E/G>=.80`だけ保存p2 placementを排他的に選ぶ。さらにsearch/validation双方が正だった`.70<=E/G<.80 && R<.060`だけ、root未来差重み0.10をruntime整数helperで選ぶ。Optuna用compile-time macro自体は移植していない。

## 10. 診断、スコア分解、実行時間

### 10.1 意思決定と診断の分離

`largest_free_component()`、`observe_loss()`、最終集計は意思決定後または独立診断区間で呼ばれる。配置候補や受理判断には使わない。

拒否後の最大空き連結成分により、置けない理由を、

- 静的な池地形
- 総空き面積不足
- 断片化

へ分類する。

### 10.2 厳密な損失分解

「全到着を衝突なし・最小周長で受け入れる」という実現不能な上界を `offered_ideal_fee` とする。最終raw scoreとの差を、

~~~text
offered ideal fee - reconstructed raw score
  = rejected ideal fee
  + accepted initial shape loss
  + relocation fee loss
  + movement cost
~~~

へ分解する。

また、

~~~text
reconstructed raw score
  = accepted final fee - movement cost
~~~

を再構成し、受理数、拒否status、候補source、cell-time、Push-out funnel、DLP request数など多数の保存則を `*_error` として検査する。

### 10.3 placement診断

主に次を数える。

- template anchor数と合法候補数
- connected-growth / grow-and-trim の生成funnel
- trimによる周長改善・同値・悪化
- shortlist件数
- incremental最良とabsolute最良の差
- future-fitが選択を変えた回数
- connected polishのcandidate / static filter / eligible、dense anchor / trim funnel、perimeter descent step / tier
- strict料金増候補、future-fit rollback、polishがbaselineを変えた回数
- 最終採用source

### 10.4 root診断

rootでは、target、destination、beam、candidate幅、screenのfuture delta、予測margin、confirmationのscreen-to-holdout差を分けて記録する。

### 10.5 CPUとwall

`RuntimeDiagnostics` は次を分離する。

| 指標 | 含むもの |
|---|---|
| `solver_cpu_ms` | 前計算と各ターンの解法本体 |
| `diagnostic_cpu_ms` | 損失分解、最大連結成分、最終集計 |
| `solver_wall_ms` | 解法本体のwall |
| `input_wall_ms` | 対話入力待ち |
| `output_wall_ms` | 出力とflush |
| `protocol_wall_ms` | プロセス全体 |
| `maximum_solver_turn_wall_ms` | 最も重い1ターン |

Pahcer wallだけでは対話待ちや背景負荷を含み得るため、方策の計算量比較では `solver_cpu_ms` を主資料にする。

直前の4-expert + `LegalAnchorIndex`では、solver CPUがmean `1240.126ms`、p95 `1747.041ms`、max `2054.115ms`、2秒超1/100だった。v31実測はmean `1200.498ms`、p95 `1629.677ms`、max `1916.647ms`、2秒超0/100で、dense全anchor、Tarjan trim、local descent、追加future-fitを入れてもこの100 seedで内部CPU悪化は見られなかった。両runは並列Pahcer上の別時点測定であり、差全体を方針の因果効果とは扱わない。v32 / v33のseed 0 solver CPUはそれぞれ`1263.353ms` / `1281.788ms`。v33ユーザー100 seed JSONのPahcer execution timeはmean`2.2383s`、p95約`4.3210s`、max`6.0598s`だが、内部solver CPU logは未保存なのでCPU tailの因果比較には使わない。

baseline oracleと同時に保存された固定100 seedログ `tools/err-multi-assignment-v20-control/` では、`solver_cpu_ms` は平均1,007.7ms、最大1,599.4msだった。同じrunのPahcer側最大実行時間は1.697秒である。これは今回新たに実行した値ではなく、保存済みログを再集計した値である。

v28の新規1200 seed収集buildでは、solver CPUは平均1,668.175ms、p95 2,162.577ms、diagnostic CPUは平均3,413.371ms、p95 4,794.059msだった。ただし`timing_diagnostic_cpu_ms`はoffline replay・teacher rolloutだけでなく、静的幾何、通常の損失分解、最終集計も含み、collector単独counterはない。また8並列・異なるseed集合の収集値なので、通常提出baselineとのCPU A/Bには使わない。

計時値はstderr診断にしか使わない。wall clockに応じて探索を打ち切る処理はなく、baselineの計算量制御は候補数、node数、window数などの固定上限による。

## 11. baseline実装の改善接点

ここでは改善案そのものではなく、何を変えるとどの層へ影響するかを示す。

| 接点 | 現在固定しているもの | 主な変更箇所 | 分離時に保つべき対照 |
|---|---|---|---|
| theta / 未来生成 | 61点事後、256決定sample | `ThetaEstimator`、`SampledDlpShadowModel::build_requests` | placementとrootを固定 |
| admission価格 | 時間だけの16 bucket双対 | `solve_dual`、`evaluate_cached` | 配置候補を同じにする |
| placement候補 | min template成立時はgrowthを作らない | `choose_temporally_coherent_region`、growth生成 | admission statusを変えないarm |
| placement shortlist | 最良周長だけ、通常6件・高外周率8件 | `PlacementShortlistBuilder` | 候補生成と最終評価を別ablation |
| placement目的 | 境界cost + 空き正方形future-fit | `compact_fit_utility`、`evaluate_compact_fit` | 料金・周長を候補に残す |
| repacking対象 | poor AcceptedとNoRegionだけ | `choose_root_action_with_rescue` | admissionを固定 |
| repacking候補 | 既存料金損0、直接差正 | target/destination/validation | 条件を1つずつ外す |
| root予測 | 2×4 screen、通常次点だけ8×12確認 | rollout生成・branch評価・confirmation | 共通未来列を維持 |
| 計算量 | 固定候補・node・window上限 | 各定数とfunnel診断 | scoreとsolver CPUのpaired比較 |

### 11.1 コードから直接見えるモデル上の境界

- temporal DLPはセル位置、連結性、池、空き形状を見ない。
- 通常 placement は最小templateが1件でも置けると、connected-growthを比較しない。
- 通常shortlistは周長が違う候補を同時比較しない。
- future-fitは空き正方形数のproxyで、未来要求の個別 `P,V` や配置成功料金を直接rolloutしない。
- 退去時刻境界とfuture-fitは正規化された未来到着CDFを使う。future-fitは `remaining_groups>0` を発火条件に使うだけで、残り組数の大小をutilityへ反映しない。
- placementの境界評価はtheta点推定、admission DLPはtheta事後5分位sampleであり、予測表現が異なる。
- root未来は48点近似と開始・終了時刻の重複排除でjointな到着列を作る一方、sampled DLPは公式整数丸めCDFを別の256点低食い違い列でsampleし、時刻重複を排除しない。二つの未来生成器は同じ分布表現ではない。
- repackingは現在到着に結び付くときだけ動き、proactiveな整理をしない。
- Push-outは総空き面積不足を事前除外するが、静的地形由来の `NoRegion` は事前除外せず、target生成が失敗するまで探索対象に入り得る。
- ActualFeeRejectedを、再配置で最小周長に戻して救う経路はない。
- rootの仮想未来は再配置を再帰的に行わず、temporal価格を凍結する。
- rescueは現在の直接利益が正の候補だけを未来比較へ渡す。
- 実行時間は固定上限で制御し、ケースごとの残り時間へ適応しない。

これらは、改善余地の候補であると同時に、現在の結果を支えている因果境界でもある。複数を一度に変えると、score差の原因を分解しにくい。

### 11.2 新規1200 seedのoffline teacherで見えたもの

v28では公式generator seed 100〜1299を自動生成し、train 800 / validation 200 / final 200へseed非重複で固定した。これは実contest scoreのA/Bではなく、同じrootから一方の判断だけを強制して最大64実到着を流した教師差である。

| finalで観測した対象 | 長期的に反転有利な割合 | 平均差・補足 |
|---|---:|---|
| baseline AcceptedをReject | 29.84% | Accepted全体の平均はなおAccept有利 |
| Minimum AcceptedをReject | 25.89% | `Reject-Accept=-18.4k` |
| connected AcceptedをReject | 39.82% | `Reject-Accept=-9.9k`。Reject有利率は高いが一律Rejectは負 |
| economic RejectをAccept | 55.77% | 平均では`Reject-Accept=+50.6k`。誤Acceptの損失裾が重い |
| primaryより良いplacement次点があるroot | 重み付き35.69%（raw 34.56%） | 全次点の重み付き平均gainは`-8.2k` |

割合と平均差は、placementのraw 34.56%という補足値を除き、配置可能な同stratum母数`stratum_population`で重み付けした推定値である。

したがって、connected fallbackや経済Reject、placement順位のいずれにも改善余地はある。しかし頻度だけで反転すると、少数の大損が多数の小幅改善を打ち消す。H16/H64の符号一致もadmission 71.20%、placement 65.35%に留まり、短い未来だけでは長期判断が安定しない。

trainでfitしてvalidationでfamily・lambdaを選び、train+validationでrefitして固定したridgeを、未開封finalへ一度だけ適用した結果は次だった。

| 対象 | final teacher gain | seed bootstrap 95% CI | 結論 |
|---|---:|---:|---|
| admission `economic_ridge_10` | -521.0M | [-1,216.4M, +153.0M] | 不採用・統計的にはinconclusive |
| placement `existing_ridge_0.01` | -928.2M | [-2,308.7M, +408.3M] | 不採用・統計的にはinconclusive |

池・topologyについても「無価値」とは確定していないが、今回のweighted ridgeとsingle-validation選択では採用根拠を示せなかった。placementのpond championは完全no-opで、良い池配置を選んだのではなく有害なoverrideを避けただけである。

次にモデル化するなら、全cohortを一つの線形境界へ混ぜず、Minimum / Extended / connected Accepted / UpperBoundRejected / ActualFeeRejectedという排他的な層へ分けること、no-changeを常に候補へ置くこと、損失裾に耐える高信頼overrideだけを許すことが重要になる。placementは今回作った同一root内のpaired labelを維持し、学習lossとmodel選択もroot単位にする必要がある。詳しい母数・hash・失敗原因は[v28実験記録](memo/experiments/20260804-offline-value-model-v28.md)にある。

## 12. コードナビゲーション

下表の行番号はfull基準Git object（この文書冒頭の基準SHA-256）専用である。現在`main.cpp`は大きく増えているため後半の実行番号はずれるが、関数名と処理順は対応する。現在差分の入口は`LegalAnchorIndex`、`case_dlp_scale_milli`、`case_placement_config`、`case_root_future_weight_milli`、`case_connected_polish_enabled`、`make_dense_box_trim_candidates()`、`make_perimeter_descent_candidates()`、`evaluate_compact_fit()`、`evaluate_arrival_decision()`、`choose_root_action_with_rescue()`、`main()`内の`case_static_expert`で検索する。

| 範囲 | 中心要素 |
|---:|---|
| 50–135 | solver / diagnostic / I/O時間の分離 |
| 136–223 | 全定数と4個のcompile-time flag |
| 224–523 | shape、盤面基本操作、正確な料金、連結成分 |
| 524–540 | placement sourceと通常候補の返却型 |
| 541–692 | theta推定、正規分布逆CDF |
| 693–748 | sampled DLP無効時の旧density / bucket部品 |
| 749–831 | placementとも共有する条件付き未来需要CDF |
| 832–1301 | temporal sampled DLP |
| 1302–1348 | sampled DLP無効時の旧shadow評価 |
| 1349–1526 | placement診断・候補・shortlist |
| 1527–1936 | connected-growth、grow-and-trim |
| 1937–2293 | 退去境界、future-fit、通常配置選択 |
| 2294–2445 | `ArrivalStatus`、admission本体、`TurnPlan`化 |
| 2446–2667 | score/loss診断と最終料金確定 |
| 2668–2716 | 盤面bit mask補助 |
| 2717–2953 | rescue / Push-out / root診断 |
| 2954–3133 | rescue target生成 |
| 3134–3308 | blocker移動先生成 |
| 3309–3486 | greedy + beam修復、独立合法性検証 |
| 3487–3835 | 共通未来、branch rollout、holdout confirmation |
| 3836–4397 | Compact rescue / Push-out / 拡張root比較 |
| 4398–4537 | 通常runner-up単独root比較 |
| 4538–4615 | 連結性validator、`apply_plan`、`emit_plan` |
| 4616–5434 | main loopと最終stderr診断 |

特に入口になる関数は次のとおり。

| 関数 | 行 | 役割 |
|---|---:|---|
| `make_template_shapes` | 270 | 面積ごとのshape前計算 |
| `ThetaEstimator` | 541 | 滞在時間分布のオンライン推定 |
| `SampledDlpShadowModel` | 880 | admission価格 |
| `trim_grown_connected_region` | 1533 | grown領域を連結なまま削る |
| `make_connected_growth_candidates` | 1636 | 非template配置候補 |
| `choose_temporally_coherent_region` | 2018 | 通常placement |
| `evaluate_arrival_decision` | 2385 | 通常admission |
| `make_rescue_targets` | 2975 | 再配置する到着目標 |
| `make_rescue_destinations` | 3176 | blockerの移動先 |
| `repair_rescue_blockers` | 3316 | 同時移動のgreedy + beam |
| `make_rescue_rollout_scenarios` | 3527 | 共通未来列 |
| `evaluate_rescue_rollout_branch` | 3691 | 1 branchの仮想未来 |
| `confirm_root_override` | 3755 | 8×12 holdout |
| `choose_root_action_with_rescue` | 3873 | rescue中心のroot選択 |
| `choose_normal_root_action` | 4398 | 通常runner-up単独比較 |
| `apply_plan` | 4574 | 実盤面への唯一の反映点 |
| `main` | 4616 | 全体の接続 |

Optunaの全調整差分は`main-optuna-v29.cpp`に履歴として残す。現在の通常実装は、DLP倍率、保存p2 placement、限定root重みをケース固定変数として再利用し、Optuna用macroは含まない。調整実験を調べるときは[v29実験記録](memo/experiments/20260805-optuna-final-v29.md)、直前portfolioは[4-expert + anchor index記録](memo/experiments/20260807-static-portfolio-anchor-index.md)、v31は[connected polish + root記録](memo/experiments/20260807-connected-polish-root-v31.md)、v32は[plateau実行記録](memo/experiments/20260807-connected-polish-plateau-v32.md)、現在v33は[static polish gate記録](memo/experiments/20260808-static-polish-gate-v33.md)を参照する。

## 13. 実験するときの比較単位

この実装はオンラインで盤面分岐が連鎖するため、小さな浮動小数点差でも後続行動が変わる。100 seedは最低限のpaired gateであり、高分散な性能判断の十分な母数とはみなさない。改善判断では次を一組として見る。

- seed 0〜99のControl互換性、対象外経路のscore・stdout byte一致
- 公式generator wrapperで必要数を自動生成し、manifestへseed・各入力hash・ordered digestを固定
- 方策A/Bは同一の十分なseed集合でpaired raw scoreを比較
- model選択はseed非重複のtrain / validation / final holdout、必要なら複数fold
- 勝/分/負と合計差
- seed単位bootstrap CIと、最悪seedを含む損失裾
- Pahcer wallだけでなくコード内 `solver_cpu_ms`
- admission、placement、repackingのどこを変えたか
- 受理集合、shape loss、既存料金損、移動費への損失分解
- rolloutやDLPの予測値と、実際のpaired差の向き

入力生成の再利用可能な入口は`tools/generate_seed_corpus_v28.py`である。v28ではseed 100〜1299を一括生成し、v29はdriverから1300〜2699を生成する。既存directoryやmanifestへの上書きを拒否するため、さらに別のcorpusを作るなら未使用のseedと出力先を明示する。

~~~sh
python3 tools/generate_seed_corpus_v28.py --start-seed 2700 --count 2000 \
  --output-dir tools/in-offline-value-next --manifest tools/input-manifest-offline-value-next.json
~~~

自動化済みなのは公式入力の生成とmanifest固定である。方策A/Bの実行自体は各実験用のconfig / runnerを別途用意し、方策を統合した後はcollectorを外した1 threadのpaired比較として測る。

新しい実験は `memo/experiments/` に1実験1ファイルで記録する。解答実行後に結果を根拠としてコードを変更するときは、`AGENTS.md` のルールに従い、新しい明示指示を受けてから行う。

## 14. 最短の要約

この実装は、**時間容量の価値をsampled DLPで見積もり、初期外周率でDLP 1.30 / 1.25 / 1.00の静的expertを選び、高外周率ケースだけplacement探索を広げる。限定地形だけrootを直接利益寄りにし、smooth地形のold Accepted connectedだけをdense変形とstrict swapで短周長化する。polish候補は料金を上げ、既存の3断面scalar future-fitを落とさない。受理可能な最小周長を最優先し、その同一周長内で退去時刻のまとまりと未来の空き形状を残す場所を選ぶ**。

通常templateとrescue targetでは、行bitsetの`LegalAnchorIndex`が従来と同じ合法anchorを同じ順で列挙し、不合法anchorの個別照会を省く。方策portfolioと意味保存の計算量改善を分けた構成である。

通常配置がかなり崩れるか、必要な空き面積はあるのに置けない場合だけ、既存料金を落とさない移動先を探索する。直接差が正の候補だけを作り、原則として直接差と短い共通未来差の合計marginが正の案だけを採用する。通常配置runner-upの上書きには、さらに独立holdoutを要求する。

要するに、baselineは「受けるか」「どこへ置くか」「old connectedを安全に短周長化できるか」「既存組を動かすか」「どの完成planを選ぶか」を別の層に保ちつつ、最後だけ共通未来で比較する構成である。
