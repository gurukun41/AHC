# expected-overlap remaining-aware future-fit v37

更新日: 2026-08-08 JST

## 状態

v35の100 seed正本は`pahcer/json/result_20260808_030825.json`で、100/100 AC、合計`6,643,865,507`、平均`66,438,655.07`である。v36は平均`66,407,862.19`で棄却し、その固有差分を撤去して実行時v35へ完全復元した。

v37は、通常placementのfuture-fitを残り組数へ結び付ける単一変更である。ユーザーがv35、貼付旧版、v37を同じ3000入力で実行した。v35は`pahcer/json/result_20260808_180959.json`で合計`204,023,485,135`、平均`68,007,828.3783`、v37は`result_20260808_193707.json`で合計`204,014,063,376`、平均`68,004,687.7920`だった。v35差`-9,421,759 (-0.004618%)`のため非incumbentとし、ユーザーの新しい明示指示でhard gateをsourceから撤去した。

## 見つかった非対称

現在ターンより後の組数を`R`とする。変更前にも次は残り組数を使っていた。

- sampled DLPは`R=0`なら機会損失を0にし、未来sampleのpopulation weightを`R/256`とする。
- 旧64 bucket shadowも未来cell-timeを`R`倍する。
- root / rescue rolloutは`R=0`なら未来比較せず、horizonを`R`以下へ切る。

一方、通常placementの3 snapshot future-fitは`R>0`かだけを見ていた。1組でも100組でも同じ強さで、同一最短周長候補の最終順位をfuture-fitへ渡し、connected polishでは旧形以上というhard guardにも使っていた。最後の組では既に`R=0`で停止していたが、終盤の少数残りと短い滞在でもfuture-fitの影響は同じだった。

## 採用した量

`ConditionalFutureDemand::future_start_cdf(T)`を、未到着の1組が現在時刻`S`より後かつ今回の退去時刻`T`より前に始まる条件付き確率

~~~text
q = Pr(S_future < T | S_future > S)
~~~

とする。未到着組のうち今回の占有と時間的に重なる到着数の条件付き期待値は、期待値の線形性から

~~~text
E[K_overlap | current information] = R q
~~~

である。独立性を仮定しなくても周辺確率が同じならこの期待値は成立する。v37は

~~~text
R q >= 1
~~~

のときだけfuture-fitを使い、`R q < 1`なら既存のincremental / absolute退去時刻境界評価へ戻す。

この1.0は開発seedから探索した係数ではなく、「今回の配置中に影響を受ける未来到着が条件付き期待値で1組以上」という単位付きの発火境界である。現在を含めて残り1組、すなわち`R=0`なら必ずfuture-fit評価0となる。未来が1組だけでも、その組が今回の滞在中に必ず到着するなら`Rq=1`なのでfuture-fitを残す。

## 比較した方針

### A. 残り組数だけの固定gate

`R<=k`でfuture-fitを止める実装は簡単だが、長い滞在と短い滞在を区別しない。`k`を開発100 seedから選ぶとpost-hocな自由度も増えるため採用しない。

### B. `R`でfuture-fit utilityを連続scale

残り数に比例してutilityを弱めるには、future-fitと退去時刻境界costを同じ目的へ足す必要がある。しかし両者の単位とscaleは異なり、新しい重み係数の較正が必要になる。現在の「同一最短周長の中だけfuture-fitで選ぶ」という因果境界も広く変わるため見送る。

### C. 期待重複到着数による発火gate

`R`と現在組の滞在長を同時に反映し、既存future-fitを使う場合の順位・snapshot・tie-breakは完全に保てる。新しい候補や学習selectorを増やさず、変更経路を低期待需要だけへ限定できるため採用した。

### D. 残り全組の逐次placement rollout

問題の目的へ近いが、通常placement候補ごとに未来盤面を展開すると既存rootと役割が重なり、CPUとsample winner's curseが大きい。過去の全候補12標本未来料金比較も悪化しており、今回の終盤調整とは分離する。

## 実装

1. `FUTURE_FIT_MIN_EXPECTED_OVERLAPPING_GROUPS=1.0`を追加する。
2. `future_mass=q`と`expected_overlapping_future_groups=R*q`を計算する。
3. 旧発火条件`R>0 && T-S>1 && q>1e-12`を`future_fit_time_relevant`として保持する。
4. 旧条件に加えて`R*q>=1`のときだけ`future_fit_available=true`とする。
5. この共通booleanを、通常shortlistの最終選択とconnected polishのfuture-fit非悪化guardの両方へ使う。
6. DLP、admission料金、候補生成、周長、退去時刻境界cost、rescue、root rollout、case expert、synthetic polish無効化は変更しない。
7. 時間的に関連するturn数、期待重複到着数合計、`R*q<1`でfilterしたturn数と期待値合計をstderrへ出す。

## 過適合対策と判定

- seed番号、初期`E/G`、進行率、`P`、`V`による新しい分類を作らない。
- 閾値1.0を100 seed上で探索せず、期待される競合イベント1件という解釈を固定する。
- future-fitを有効にする領域では、v35と同じsnapshot、utility、候補順、tie-breakを使う。
- future-fitを無効にする領域でも、料金・周長・退去時刻境界・root rollbackを維持する。
- 同じ開発100 seedは行動確認に使えてもfresh性能保証ではない。まず100 seed pairedでAC、差の符号、勝分敗、worst、filter発火数、solver CPUを確認し、良ければ3000 caseで一般化とtailを確認する。

## 静的検証

- `git diff --check`: pass
- Apple Clang 17、C++17/C++20、`-O2 -DNDEBUG -Wall -Wextra -Wshadow -Wpedantic -fsyntax-only`: pass、警告0
- 解答プログラム実行: 0回

## 実行前freeze

- `main.cpp`: 6,697行
- source SHA-256: `37150252a0fd092f34f7d32af6c36cfce73446745a2991caca39e2f8d91442ae`
- 実測incumbent: v35、平均`66,438,655.07`
- v37の3000 score: `204,014,063,376`、v35差`-0.004618%`

## 撤去

実測差だけでなく、期待未来損失は`Rq`へ連続的に比例し、目的関数上`Rq=1`に不連続点がない。ユーザーから問題の性質に基づく再考察と新実装の明示指示を受け、定数、発火条件、専用診断を全撤去した。次案は[v38記録](20260808-spatial-template-shadow-v38.md)を参照する。

静的検証とfreeze後もエージェントは解答プログラムを実行しない。100 caseはユーザーが`pahcer-studio`、3000 caseはユーザーが`pahcer_config.toml`と`tools/in_big`で実行する。
