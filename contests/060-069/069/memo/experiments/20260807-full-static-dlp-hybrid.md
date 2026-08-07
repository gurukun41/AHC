# 旧full solverと静的外周適応DLPのhybrid

## 状態

2026-08-07の新しい明示指示に基づき実装し、同日のユーザー実行で100 seedを確認した完了記録である。
直前の[軽量hybrid](20260807-lightweight-hybrid.md)は100/100 AC、合計`6,603,615,029`、平均`66,036,150.29`だった。本変更はその結果を分析・記録した後に届いた「更なる改善」の指示により開始し、実行後の本追記はユーザーの「確認と記録」指示だけに基づく。解答source・方針・定数は実行後に変更していない。

## 調べた方針

| 方針 | 根拠 | 利点 | 今回の判断 |
|---|---|---|---|
| 旧full + 静的外周適応DLP | fresh validation 300 seedの保存cacheで強い正信号 | fullのrepackingを保ちつつadmissionだけ改善 | **採用** |
| 現行lean + 需要重み付き最大空き矩形 | MaxRects・再利用資源の非対称価格付け | constrained配置のmodel misspecificationを直接修正 | 未較正proxyを一発実行へ入れる危険から保留 |
| `E/G`によるlean/full二solver portfolio | 開発100 seed合成で現行lean比`+0.176633%` | 各層の保存済みexpertを選べる | 二つの対話solver統合が大きく、期待差より事故リスクが高い |
| criticalityのfused imos + unified top-K | 現行criticalityの概算計算量を約1/3へ削減可能 | constrainedの時間を抑え、connectedにも幾何信号を渡せる | criticality単独のscore因果が未分離なので不採用 |
| Push-out-lite / selective rollout | dynamic reconfigurationとrollout文献に整合 | NoRegionや局所断片化を修復 | 旧fullが既により検証済みの実装を持つため新規版は不要 |

開催中のAHC069については解法共有禁止に従い、他参加者の現行解法は調べていない。一般理論として、再利用資源のonline allocationでは資源が滞在後に戻ることを明示した価格付けが必要であり、online LPでは双対価格を行動履歴へ接続する設計が中心になる。強いbase policyから少数枝だけを深く比較するrolloutと、動的2次元配置でfragmentationと再配置量を分ける考え方も、旧fullの構造を支持する。

- [Online Resource Allocation for Reusable Resources](https://arxiv.org/abs/2212.02855)
- [Online Linear Programming: Dual Convergence, New Algorithms, and Regret Bounds](https://arxiv.org/abs/1909.05499)
- [On-line Policy Improvement using Monte-Carlo Search](https://proceedings.neurips.cc/paper_files/paper/1996/file/996009f2374006606f4c0b0fda878af1-Paper.pdf)
- [An Efficient Data Structure for Dynamic Two-Dimensional Reconfiguration](https://arxiv.org/abs/1702.07696)

## 採用根拠

旧full正本は`main-optuna-final.cpp`、SHA-256 `086cb6cc2e24848c77a4b766ab8dde433dbf16469095cca557f848bdff091c6a`、開発100 seed合計`6,515,194,836`である。v29では固定DLP 1.30がfresh block validation 300 seedで平均を約`+1.99%`改善した一方、下位裾gateで不採用になった。その後、初期盤面だけでケースを分け、

```text
G = 初期芝セル数
E = 芝セルから池または盤外へ出る4近傍辺数
100E < 55G なら DLP 1.30
それ以外は DLP 1.00
```

とした保存cache合成では、同じfresh validation 300 seed上で次を得た。

- score差: `+413,049,127 (+2.018205%)`
- 勝 / 分 / 負: `178 / 80 / 42`
- seed別ratio p05 / worst: `0.989437 / 0.971320`
- solver CPU平均: `1651.397 -> 1574.050ms`
- solver CPU p95 / max: `2146.474 -> 2101.960ms / 2714.630ms`

これは固定1.30 binaryと1.00 binaryの保存結果を静的`E/G`で選んだ**cache合成**であり、今回の単一adaptive binaryの実測ではない。異なる開発100 seedへ`+2.018205%`をそのまま移送した値も予測にすぎない。p95と最大CPUは2秒を超えており、TLE安全性は証明されていない。

それでも、選択した変更は新規学習器や未検証の空間proxyではなく、独立集合で確認済みの二つの価格尺度をケース開始時に一度だけ選ぶものに限られる。constrained側は意味上1.00の旧full経路を保護し、最新leanが旧fullへ負けた層ではCompact rescue、NoRegion Push-out、root comparisonを取り戻す。

## 実装

`main.cpp`を旧full正本から復元し、差分を次の4点だけに限定した。

1. `DLP_SMOOTH_SCALE_MILLI=1300`、`DLP_CONSTRAINED_SCALE_MILLI=1000`を追加する。
2. 初期盤面から`E`と`G`を数え、strictな`100E < 55G`でケース中の倍率を固定する。
3. `evaluate_arrival_decision()`入口で値渡しの機会損失へ倍率を1回掛ける。実到着と仮想rollout到着の共通入口なので両方へ同じ尺度が届く。
4. `choose_root_action_with_rescue()`入口でもraw機会損失へ1回掛け、NoRegion Push-outの経済gateとtarget thresholdを通常admissionへ揃える。

placement、grow-and-trim、Compact rescue、NoRegion Push-out、root rollout、theta推定、sampled DLP本体、全定数には変更を加えていない。既存のloss診断はraw opportunity costを記録する箇所があり、末尾の`case_dlp_scale_milli`と併読する。これはv29の検証実装と同じで、意思決定には影響しない。

## 実行前監査と停止条件

- `main-optuna-final.cpp`との差分は上記42追加行だけである。
- 独立した2監査で、実到着・仮想到着・Push-outへの倍率は各1回、二重掛けなし、`E/G`式と変数scopeにblocking issue 0を確認した。
- 全実装・メモ・静的監査を凍結した後、エージェントは警告付きcompileとseed 0 smokeを一度だけ行った。exit code 0、score `56,279,386`、solver CPU `1675.600ms`だった。
- その後ユーザーが同じsourceをPahcerで100 seed実行した。以下は新しい明示指示による結果記録だけであり、source・方式・定数の変更や追加の解答実行は行わない。

## ユーザー実行結果

- 開始: 2026-08-07 21:08:35 JST
- comment: `test`
- seed: 0〜99、100ケース
- AC: 100/100
- WA seed / case error: 0 / 0
- 合計score: `6,616,387,134`
- 平均score: `66,163,871.34`
- 中央値: `63,646,409.5`
- p05 / p95: `37,493,314.4 / 91,158,778.6`
- worst / best: `27,048,829 / 102,451,870`

保存されているseed 0〜99・100/100 ACの137 run中でraw合計1位である。

### paired比較

| 比較対象 | 対象合計 | 今回の差 | 勝 / 分 / 負 |
|---|---:|---:|---:|
| 直前の軽量hybrid | 6,603,615,029 | `+12,772,105 (+0.193411%)` | 56 / 0 / 44 |
| 旧full | 6,515,194,836 | `+101,192,298 (+1.553174%)` | 50 / 37 / 13 |
| 直前までの保存済み最高 | 6,534,395,462 | `+81,991,672 (+1.254771%)` | 63 / 2 / 35 |
| protected lightweight | 6,439,164,489 | `+177,222,645 (+2.752261%)` | 87 / 0 / 13 |

直前軽量版比のseed別ratioは幾何平均`+0.233034%`、p05`-3.43247%`、worst`-7.51231%`。100,000回のseed bootstrapによるaggregate差の95%区間は`[-0.304%, +0.676%]`で0をまたぐ。旧full比の同区間は`[+1.029%, +2.070%]`で正だった。この100 seedは開発中に繰り返し参照済みなので、区間はこの集合内のばらつきでありfreshな汎化保証ではない。

### 静的外周率による層別

| 層 | seed数 | 直前lean比 | 勝 / 分 / 負 | 旧full比 |
|---|---:|---:|---:|---:|
| smooth `E/G<0.55` | 70 | `+1,107,962 (+0.021424%)` | 39 / 0 / 31 | `+101,192,298 (+1.995268%)` |
| constrained `E/G>=0.55` | 30 | `+11,664,143 (+0.814583%)` | 17 / 0 / 13 | `0`、30/30同点 |

直前比改善の`91.32%`はconstrained側のfull復元から生じた。constrained 30 seedが旧fullと全件完全一致したため、`1.00`枝が意味上baselineを保護する設計を実scoreでも確認できた。smooth側も直前leanを`+1,107,962`だけ上回り、事前の「lean smooth + full constrained」保存score合成`6,615,279,172`を同額だけ超えた。

constrainedをさらに分けると、直前lean比は`.55<=E/G<.65`で`+1.165333%`、`.65<=E/G<.80`で`+1.397696%`、`E/G>=.80`で`-0.299121%`だった。最後の細分は事後診断であり、閾値変更の根拠にはしない。

### 実行時間と不変条件

Pahcer wallはmean `2.154865s`、median `2.024779s`、p95 `3.096572s`、max `3.730568s`、2秒超53/100。直前leanのmean `2.248052s`より低い一方、p95 / maxは悪化した。`threads=0`の並列tester・I/O込みなので方策CPUの因果比較には使わない。

最新`tools/err`のコード内計時は次のとおり。

- solver CPU mean / median: `1644.464 / 1574.481ms`
- solver CPU p95 / max: `2314.446 / 2610.364ms`
- solver CPU 2秒超: 13/100
- process CPU mean / p95 / max: `1699.305 / 2369.023 / 2665.774ms`
- DLP scale: 1.30が70 seed、1.00が30 seed
- JSONとstderrのscore不一致: 0
- 全`*_error`非zero、rescue validation failure: 0

score改善と静的branchの復元は確認できたが、solver CPU p95 / maxは2秒を超え、TLE安全性は未解消である。

## artifact

- `main.cpp`: 5,476行
- SHA-256: `a9b4c499f037c671ecacfb2ef9bafab18fe09b47270ac1d842cc1a3213e222e5`
- base: `main-optuna-final.cpp`
- base SHA-256: `086cb6cc2e24848c77a4b766ab8dde433dbf16469095cca557f848bdff091c6a`
- tester SHA-256: `3702067f731de62b30a395f99eee04a4a9e247a1f421e2c42556a9e45b62ec92`
- smoke input: `tools/in/0000.txt`
- smoke input SHA-256: `61857f9adeb56546a876f9f54eec3e95be617d4a1135d987ae790a2248304754`
- compiler: Apple Clang 17.0.0（`g++` driver）
- 最終一回の固定コマンド: `g++ -std=c++20 -O2 -Wall -Wextra -pedantic main.cpp -o /tmp/ahc069-full-static-dlp && ./tester /tmp/ahc069-full-static-dlp < tools/in/0000.txt`
- Pahcer result: `pahcer/json/result_20260807_210835.json`
- result SHA-256: `778a335654bd0f7a59294180d9247354486ef7f04e182e8967cef75f984f34e1`
- Pahcer binary: `a.out`
- binary SHA-256: `af00957f3059ed7d18bcb80c98664f62c7f77ed5ecc764fdab824b31afd2d71a`
- config SHA-256: `4d0af5fcfefce5fe98346f86f1f1f3a95d3a0a846239417b162e767245e013a3`
- ordered stderr hash-list digest: `95d1318843e2ac93666d2d1fa2328af496da9bf381c7a2e254cf5badf2600c08`
