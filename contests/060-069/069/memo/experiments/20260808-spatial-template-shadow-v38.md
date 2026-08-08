# compact-template configuration shadow v38

更新日: 2026-08-08 JST

## 状態

実測incumbentはv35である。ユーザーが同じ3000入力で実行したv35は
`pahcer/json/result_20260808_180959.json`、3000/3000 AC、合計
`204,023,485,135`、平均`68,007,828.3783`だった。v37は
`result_20260808_193707.json`、合計`204,014,063,376`、平均
`68,004,687.7920`で、v35差`-9,421,759 (-0.004618%)`だった。
ユーザーの指示どおりv35を維持し、v37の`Rq>=1` hard gateはsourceから撤去した。

v38は実ケースの分類や差分を方策へ入れず、公式生成式と料金式から通常placementの
空間機会費用を作る。実装・静的検証・release freeze後、ユーザーが100 caseと3000 caseを
実行した。いずれも全ACだったが、v35比はそれぞれ`-0.196676%`、`-0.119609%`であり、
v38はnon-incumbentと判定する。実測incumbentはv35のままである。

結果確認後、ユーザーの明示指示によりv38固有差分を全撤去した。現在の未コミット
`main.cpp`はv35正本へbyte単位で復元済みで、6,654行、SHA-256
`1a5f652b17ca8de08b34920ea35f1928cfea7008dc98a4c7138b933e22d3db60`である。
復元後に解答プログラムは実行していない。

## 問題構造からの導出

滞在時間を`D=T-S`、`Z~N(0,0.8^2)`とすると、丸めと上限を除いて

~~~text
V = P D^0.9 2^Z
fee = compactness * V
fee / (P D) = compactness * 2^Z D^-0.1
~~~

となる。面積`P`はcell-time当たり価値から消え、主に連結配置の困難さと
compactnessへ効く。したがって、総芝面積を一つの流体容量に潰すtemporal DLPは
admissionの基礎として残し、placementでは「未来の連結template列がどのセルを
必要とするか」を別の空間shadowとして評価する。

残り組数を`R`、今回の退去までに未来1組が始まる確率を`q`とすると、未来価値は
`Rq`へ連続的に比例する。目的関数に`Rq=1`の不連続点はないため、v37型gateは使わない。
`R=0`では空間shadowを厳密に停止する。

## 採用した近似configuration価格

1. 公式`P=round(U^2), U~Uniform[2,sqrt(150)]`を等確率16層へ分け、各層の中点を
   決定的な面積probeにする。同じ`P`へ丸められた層は確率質量を合併する。
2. 今回の滞在中に始まる未来開始分布の`1/6, 3/6, 5/6`分位を3 snapshotにする。
3. snapshotまでに`T<S_snapshot`となる既存組を解放した盤面を作る。
4. 各面積probeについて、`Lmin, Lmin+2, Lmin+4`の順に最初に1本以上置けるtierを選び、
   そのtierの全合法compact-template anchorをconfiguration列とみなす。
5. 同じ面積classでは未来1組を全合法列へ一様に流すfractional解を作る。各列の
   期待料金を使用セルへ均等配分し、セルごとのmarginal bid priceを得る。
6. `E[2^Z]`は対数正規平均として解析的に積分する。開始時刻で条件付けた
   `E[D^0.9]`も既存48点length求積から計算するため、価値乱数は標本化しない。
7. 現在候補の空間costは、候補セルのbid priceを3 snapshotで合計し、`Rq/3`を掛ける。
   同一周長shortlist内でcost最小を選び、数値同点だけ既存incremental costへ戻す。

列`r`の面積を`p`、料金期待値を`F`、合法列数を`A`とすると、列の各セルへ
`F/(A p)`を配る。ある面積classについて全セル価格を合計すると`F`へ戻るため、
面積やorientation数の違いで総価値を水増ししない。

## 因果境界

- 変更するのは実到着の通常shortlist内だけで、候補は全て同じ周長・同じ丸め後料金。
- admission、temporal DLP、候補生成、周長ladder、repacking、移動費、root発火を変えない。
- connected polishは異周長を比較するため、v35の3 snapshot square-fit非悪化guardを維持する。
- synthetic rolloutはv35の軽量square-fitを維持する。重い列価格を再帰適用せず、実状態の
  one-step policy improvementとしてCPU経路を分離する。
- 空間shadowが非有限なら、その到着だけv35 square-fitへ戻す。
- 新しい地形閾値、seed分類、score由来係数、受理Rejectは追加しない。

## 診断

stderrへ次を追加する。

- 空間shadowの実評価turn、選択変更turn、候補評価数、snapshot数
- 面積probe数、shape-row照会数、合法configuration列数
- 最良tierも置けなかったprobe数
- 非有限error数（0必須）

## 静的検証と実行前freeze

- `main.cpp`: 6,993行、source SHA-256
  `8edfb91ea48efdab43e75ea39eede0ed2b0bb966368b064e106afa73f08a7959`。
- release binary: `/private/tmp/ahc069-spatial-template-shadow-v38`、SHA-256
  `7e1f764e4b567095be9bfc8a0660bf05442afcbe4ad865f9b08943b5bcd2a171`。
- Apple Clang 17、C++17/C++20、`-O2 -DNDEBUG -Wall -Wextra -Wshadow -Wpedantic -fsyntax-only`:
  pass、警告0。C++20 static analyzerは指摘0、`git diff --check`もpass。
- 実行前の解答プログラム実行: 0回。
- 全設計、コメント、静的監査、release compile、source freeze後にseed 0を一度だけsmokeした。
  その後、ユーザーがPahcer Studio 100 caseと`tools/in_big` 3000 caseを実行した。
- 実行時のfreeze v38 `main.cpp`は6,993行、上記source SHA-256と一致した。100 / 3000 caseの
  間にsource更新はなく、3000 case結果確定まではsource・方針・定数を変更していない。

## freeze後の最小smoke

エージェントがfreeze後にseed 0を1回だけ実行した。AC、Score `55,032,673`で、
v35 seed 0の`55,762,976`比`-730,303 (-1.310%)`だった。solver CPUは
`1829.376ms`、空間shadowは725 turnを評価して419 turnで選択を変更し、
`spatial_shadow_nonfinite_errors=0`だった。この結果からユーザーの3000 case完了までは
sourceを変更していない。

## ユーザー実行: 100 case

- 正本: [`result_20260808_204340.json`](../../pahcer/json/result_20260808_204340.json)
- comment `v38`、seed 0〜99、100/100 AC、WA 0
- 合計`6,630,798,632`、平均`66,307,986.32`
- v35正本`result_20260808_030825.json`比:
  `-13,066,875 (-0.196676%)`、46勝0分54敗
- 最大絶対改善はseed 59の`+5,704,137`、最大絶対悪化はseed 83の`-3,957,691`

100 caseの全scoreは、後述する3000 case正本のseed 0〜99と100/100で一致した。

## ユーザー実行: 3000 case

- 正本: [`result_20260808_225925.json`](../../pahcer/json/result_20260808_225925.json)
- start `2026-08-08T22:59:25.229116+09:00`、comment `test`
- seed 0〜2999、3000/3000 AC、WA 0、全`error_message`空
- 合計`203,779,454,424`、平均`67,926,484.8080`
- total log10 `23453.364164679595`
- JSON SHA-256:
  `43a9d6b459ccd4e155c848b10e54e2801d16b92f86bdb504935c814fdd4cc500`

同一seedのv35正本`result_20260808_180959.json`とのpaired比較は次のとおり。

| 指標 | v38 - v35 |
|---|---:|
| 合計差 | `-244,030,711` |
| 平均差 | `-81,343.5703` |
| 合計比 | `-0.119609%` |
| 勝 / 分 / 敗 | `1,433 / 0 / 1,567` |
| positive gross | `+1,538,842,442` |
| negative gross | `-1,782,873,153` |
| casewise ratio p05 / median / p95 | `0.964334 / 0.999052 / 1.033044` |

paired差の標本標準偏差は`1,513,916.689`、平均差の通常近似95%区間は
`[-135,518.38, -27,168.76]`だった。seed 0〜99だけでなく、seed 100〜2999でも
合計`-230,963,836`、平均`-79,642.70`、1387勝0分1513敗であり、負差は先頭100 caseだけに
集中していない。

- 最大絶対悪化: seed 1952、`-7,445,643`
- 最大絶対改善: seed 1339、`+6,923,558`
- 最悪ratio: seed 1542、`0.890200`、`-6,270,002`
- 最良ratio: seed 196、`1.084441`、`+5,484,221`

参考として、v37比は`-234,608,952 (-0.114996%)`、一つ前に貼られたコード比は
`+195,514,959 (+0.096037%)`である。採否基準は実測incumbent v35との比較に置く。

## 3000 case診断

`tools/err`の3000 stderrを集計した。

- 空間shadow評価`1,811,703` turn、選択変更`1,045,957` turn
  (`57.734%`)、候補評価`7,944,844`
- snapshot `5,435,109`、面積probe `86,961,744`
- shape-row照会`110,137,735,251`、合法configuration列`317,086,832,356`
- 最良tierにも置けないprobe `13,260,929`
- `spatial_shadow_nonfinite_errors=0`
- `error`または`validation_failure`を名前に持つ診断tokenは全caseで0
- solver CPU mean / median / p95 / max:
  `2083.124 / 1985.509 / 3030.481 / 5075.625ms`
- Pahcer execution time mean / median / p95 / max:
  `2.8885 / 2.7537 / 4.2433 / 7.2528s`

v35のPahcer execution time平均は同じ3000 caseで`1.8319s`であり、v38は約57.7%長い。
wall値は背景負荷を含むためjudge CPUそのものではないが、configuration列全列挙が重いことは
solver CPU診断とも整合する。

## 判定と解釈

v38は全不変条件を守って完走し、実装事故や非有限値は観測されなかった。一方、100 caseと
その外側2900 caseの双方で負、3000 caseの中央値も負であり、v35から昇格させる根拠はない。
したがってv38を棄却し、実測incumbentはv35を維持する。

公式分布から価格を導出したこと自体は問題構造に沿っているが、各未来組を同周長tierの全合法列へ
一様にfractional配分したセル価格は、逐次配置後の競合、将来のpolicy、configuration間の代替関係を
十分に表していない可能性がある。また、評価turnの57.7%で既存選択を変えており、同一料金内だけの
変更でも保守的なtie-breakではなく大規模なpolicy置換になった。これは診断と実測に整合する解釈で
あって、個別seedからの因果断定ではない。

この結果を見て係数、地形gate、seed分類を後付けしなかった。ユーザーの明示指示を受け、
v38のcell price、selector、診断、コメントを全撤去してv35正本へbyte一致で復元した。
復元後の解答実行は行っていない。次案の実装は別の明示指示を待つ。
