# connected fallback risk v26

## 仮説

最小周長・拡張templateを置けないときの三つのconnected系候補生成器は、空き連結成分が`P`セル以上なら現在組を必ず配置できる。その完全性は終盤には細切れ空間を収益へ変えるが、今回組の滞在中にも未来到着が見込まれる序盤・中盤では、現在料金と引き換えに後続のcompact配置を壊している可能性がある。

そこで、現在時刻`S`より後に残る1組が時刻`T`までに始まる条件付き確率を`F(T)`として、時間riskを

```text
expected_overlap = remaining_groups * F(T)
```

とする。同じfuture-fit 3 snapshotで、今回組を置かない盤面を`U0`、connected候補を置く盤面を`U1`として、空間damageと総合riskを

```text
damage = 1 - exp(U1-U0)
risk = expected_overlap * damage
```

とする。`U`は各正方形サイズの`log(1+合法anchor数)`の重み付き平均なので、`damage`は配置選択肢の相対減少、`risk`は失う未来到着相当数と解釈できる。`risk < 1`は従来どおり受け入れ、`risk >= 1`だけAccept/Rejectを同じ未来で比較する。`expected_overlap < 1`なら`damage <= 1`より必ずbypassされるため、終盤・短期滞在では細切れ空間を従来どおり使う。

さらに通常templateの探索幅を`Lmin+4`から`Lmin+8`へ広げ、connectedへ落ちる頻度とrisk gateの相互作用を分けて測る。

## baselineと変更範囲

無フラグControlはcommit `5145bc7`の採用済みbaselineと同じ方策を保つ。v25の棄却済み空間DLPコードはユーザーが実験前に`main.cpp`から戻している。

変更は次の二軸だけである。

1. 通常配置用template上限を`Lmin+4`または`Lmin+8`にする。
2. raw BFS、multi-start connected growth、grow-and-trimから選ばれた経済的受入のうち`risk >= 1`だけ、Accept対Reject比較を追加する。

admission、placement、repackingを混同しないため、次を固定する。

- placementは三つのconnected生成器を従来どおり列挙・順位付けし、選択source、`expected_overlap`、`damage`、`risk`を返す。
- upper-bound feeと実周長feeによる既存admissionを先に通す。料金判定で落ちる候補はrisk比較しない。
- 既存の通常次点rootとCompact rescueを先に実行する。最小周長templateへ直せた場合やtemplate次点へ変わった場合はrisk比較しない。
- 最終案もconnected系なら、Acceptを保護案、Rejectを挑戦案としてQ2/H4の共通未来screenへ載せる。
- `screen margin = -current_fee + future_fee(Reject) - future_fee(Accept)`が正の場合だけ、既存の独立Q8/H12 holdoutで確認する。holdoutも厳密に正の場合だけ`FallbackPolicyRejected`へ変える。
- screenは進行4区間ごとに最大2回、合計最大8回。risk confirmationはケース最大4回の専用予算を持ち、既存rootの最大4回を消費しない。
- v6/v7ではRejectが共有confirmation枠を消費し、Rejectを採用しないケースでも後続runner-upの判断を変える既知の交絡があった。今回は既存root予算を完全に保護し、追加CPUは`timing_solver_cpu_ms`停止条件で判定する。
- 同点、scenario生成失敗、screen予算・confirmation予算切れではAcceptを保護する。
- `FallbackPolicyRejected`は`NoRegion`と分け、NoRegion Push-outへ流さない。本当に連結`P`セルがない`NoRegion`の扱いは変更しない。
- Compact rescueの発火境界は全armで従来の`Lmin+4`に固定する。
- risk比較は実ターンだけで行い、仮想未来内では再帰しない。候補順位、shadow price、未来方策は変更しない。
- 既知のv24幾何hard reject失敗を繰り返さないため、`damage`や`risk`だけではRejectしない。既知のv6 generic Reject失敗に対しては、対象をconnectedかつ`risk >= 1`へ限定し、Accept protected・固定screen予算・独立holdoutを必須にする。

## 実行前に固定した仕様・停止条件

### 比較arm

| arm | compile flag | 通常template | connected risk比較 |
|---|---|---:|---:|
| Control | なし | `Lmin+4` | off |
| Wide | `AHC069_WIDE_TEMPLATE_LADDER` | `Lmin+8` | off |
| Gate | `AHC069_ENABLE_FALLBACK_RISK_GATE` | `Lmin+4` | on |
| WideGate | 上記2 flag | `Lmin+8` | on |

同一seed 0..99を各arm 1回だけ実行する。主比較は`WideGate-Control`、主効果は`Wide-Control`、`Gate-Control`、`WideGate-Gate`、`WideGate-Wide`で分ける。scoreだけでなく勝/分/負、paired差の中央値・分位点・95% bootstrap CI、受入/拒否、source別形状損、後続のNoRegion/fragmentation、Pahcer wall、コード内`timing_solver_cpu_ms`の平均・p95・最大・1800ms超件数を比較する。

`fallback_risk_cpu_ms`はscreen・holdout部分だけで、placement内の`U0/U1`計算を含まない。性能判断は常に全体の`timing_solver_cpu_ms`を正本とする。

因果隔離のhard checkとして、Gateで`fallback_risk_final_rejections=0`のseedはControlとstdout byte一致、WideGateで0のseedはWideとstdout byte一致することを要求する。

次のいずれかなら該当armを採用しない。

- ACが100/100でない、決定性・合法性・保存則に違反する。
- Controlが既存oracleとscoreまたはstdoutで1 seedでも不一致になる。
- Control比の合計scoreが非正となる。
- paired bootstrap 95% CIの上端が0以下となる。
- solver CPUがControl比で平均またはp95 `+15%`を超える、最大`1900ms`以上、または`1800ms`超が増える。

`WideGate`を通常提出候補へ昇格する強い条件は、Control比の合計scoreが正、95% CI下端も正、上記安全性・CPU条件を全て通ることとする。そこまで届かない正の結果は追加検証候補に留める。結果を見た後、この実験内で閾値、候補集合、定数、コードを調整しない。

## source / binary / config / input / oracle

2026-08-03 JST、最初の解答実行前に次を固定した。コミットは作成していない。

- HEAD: `5145bc7e6799f312889239696d66902abf63d008`、branch `069`
- baseline Git object: `086cb6cc2e24848c77a4b766ab8dde433dbf16469095cca557f848bdff091c6a`
- v26 `main.cpp`: `7ee9165b5b7075bb4cef5980631320fbf4efea92e850e0af89a1cc93fb04fb9a`
- Control binary: `59a2da4c22e8a6a6d3f86d795def7ef713f220c7cceb902f3359ba378322eec5`
- Wide binary: `21e298bf1a162e2bb929f18101272e8c9fdb47a657a535989337ece33789dfb0`
- Gate binary: `977a055e55ecf83f4e84628d0c238be976c5b18814d43d0b541ba5ed8c969a26`
- WideGate binary: `6439be5fe787f39549cea0351d8ef7bf010a91f746638b8f921eb29ed65bed9e`
- ASan / UBSan WideGate binary: `742c5315d297801068cb942339bbae286c05319eaf36b5bd65e6fa8161961104`
- Control / Wide / Gate / WideGate config: `9fb6d40d4fbc0cbcddc217e6e4231f71172236e5e53326b6e182dae273c883ae` / `dafb7c39cee32c448e7c4f2166021b5061b9c094900066f31c71d1d647f15a6e` / `8f2a7a624a2a2eae73054346ff184791835e225ea6655d35ac7820a46c0d6049` / `ee318f3cfb9bbcec93a7c09ea0cbe43ab978438fa010ce4e4bd00e24847e7069`
- seed 0..99入力の順序付きhash一覧digest: `c12354a1545d49df17358688f8269b57186f9ef22317039127753f6343641773`
- tester: `3702067f731de62b30a395f99eee04a4a9e247a1f421e2c42556a9e45b62ec92`
- `pahcer/best_scores.json`: `9f84e3234da06c37b672ff6890e2d04d538dbf2e846ffb617a2c1f608d2556b9`
- oracle JSON: `9b064a2c0670a2df7dc2ea153ab50fa323a178e05a4e578318773e74c645c0f3`
- oracle stdout 100件の順序付きhash一覧digest: `9ecf5c097f3509de2d8a157467d7846f9688bedaedfa002a86daf7f83e96018f`
- seed 0..99入力のpath非依存content-hash digest: `49bf804d96c713381cc60955fa7b46e45674b0e456e3a25ba238ca7bbb3cefb8`
- oracle stdoutのpath非依存content-hash digest: `959a9fefc8ab44c03a143345f3b830ab052f9b9968b9e2e878d328ff23be61ad`

固定binary pathはControl / Wide / Gate / WideGateの順に、`/private/tmp/ahc069_fallback_phase_v26_{control,wide,gate,wide_gate}`。configは`pahcer/bench_fallback_phase_v26_{control,wide,gate,wide_gate}.toml`、入力は`tools/in/0000.txt`から`0099.txt`である。既存oracleは`pahcer/json/result_20260803_003818.json`、合計`6,515,194,836`、stdout比較先は`tools/out-wide-stp-v25-default/`である。

## 静的検証

releaseはHomebrew Clang `22.1.8`、C++20、`-O2 -DNDEBUG -Wall -Wextra -Wshadow -Wpedantic`、`-I/usr/local/include`で4 armを作成し、全て警告0。C++17でも4 armのsyntax検査を行い、全て警告0。WideGateのASan / UBSan build、Clang Static Analyzer、`git diff --check`もpassした。独立したGNU GCCはこのホストにない。

4 TOMLはPahcer `0.3.1`でparseでき、全てseed `[0,100)`、`threads=1`、`measure_time=true`、`./tester`、上記固定binary path、arm別stdout/stderr directoryを指すことを確認した。入力・oracle stdoutは各100件である。ここまで解答binaryは一度も実行していない。

## 実行結果

4 armともseed 0..99を1回ずつ実行し、100/100 AC。全ての保存則・funnel・source partition・DLP call count・非有限値・合法性診断は0であった。Controlは既存oracleとscore 100/100、stdout byte 100/100で完全一致した。

| arm | total score | Control差 | Control比 | 勝/分/負 | Pahcer wall 平均 / p95 / 最大 ms | solver CPU 平均 / p95 / 最大 ms | solver CPU >1800ms |
|---|---:|---:|---:|---:|---:|---:|---:|
| Control | 6,515,194,836 | 0 | 0 | — | 1055.729 / 1387.033 / 1723.485 | 955.673 / 1236.692 / 1543.508 | 0 |
| Wide | 6,507,545,417 | -7,649,419 | -0.117409% | 27/48/25 | 1089.272 / 1390.229 / 1565.337 | 994.990 / 1261.250 / 1413.844 | 0 |
| Gate | 6,525,065,419 | +9,870,583 | +0.151501% | 34/38/28 | 1284.176 / 1738.774 / 1890.121 | 1187.381 / 1582.743 / 1760.921 | 0 |
| WideGate | 6,519,121,065 | +3,926,229 | +0.060263% | 39/23/38 | 1338.800 / 1847.028 / 1960.898 | 1243.556 / 1683.613 / 1873.860 | 2 |

Pahcer wallの1800ms超はControl / Wide / Gate / WideGateで0 / 0 / 4 / 6件。内部solver CPUのControl比は、Wideが平均`+4.114%`・p95`+1.986%`、Gateが`+24.246%`・`+27.982%`、WideGateが`+30.124%`・`+36.138%`であった。Gate系は事前停止条件の`+15%`を超える。

保存result JSONとSHA-256は次の通り。

- Control: `pahcer/json/result_20260803_234657.json`、`2e1e381c7e2080c97b9ee5e621e47fc9cc5acaa86faae7231935bec382e4497a`
- Wide: `pahcer/json/result_20260803_234847.json`、`f8fe2715b8407c39bb6acb0716bd41f8e045d64b5041a2a9a356792725727b9d`
- Gate: `pahcer/json/result_20260803_235042.json`、`ddd508be0752add0a799241d9c470b614d9f6c527307fc7850b1a29ddabada55`
- WideGate: `pahcer/json/result_20260803_235257.json`、`684ac61ad946c45cfdd49710081c15205d0188f4ba040e79f6a7534f11c99d14`

Control stdoutのpath非依存content-hash digestもoracleと同じ`959a9fefc8ab44c03a143345f3b830ab052f9b9968b9e2e878d328ff23be61ad`。Wide / Gate / WideGateは`ee890edeecabe65b54b15a5dd8637e1f88b7a5e8547ae56590b772ce5eaab94e` / `13c1bbbcceecef6cb90d65ccac1662adc704230fe2d01c29e26fd38d0c0213dd` / `cf3ce26d946e18997fa4adfacb5e4c38b3266f68e22604550d88f79a1726a702`である。

## paired比較と損失分解

paired raw score差を100 seed単位で復元抽出するpercentile bootstrapを100,000回、乱数seed `260803`で行った。全比較で差の中央値は0である。

| 比較 | total差 | 勝/分/負 | bootstrap 95% CI |
|---|---:|---:|---:|
| Wide - Control | -7,649,419 | 27/48/25 | [-20,386,917, +4,576,431] |
| Gate - Control | +9,870,583 | 34/38/28 | [-6,569,626, +26,615,522] |
| WideGate - Control | +3,926,229 | 39/23/38 | [-12,968,227, +21,062,509] |
| WideGate - Gate | -5,944,354 | 31/46/23 | [-17,173,935, +4,850,272] |
| WideGate - Wide | +11,575,648 | 32/39/29 | [-2,705,111, +26,036,005] |

### 因果隔離

- Gateで方策Rejectが0回だった38 seedは、38/38でControl stdoutとbyte一致した。残る62 seedは全てRejectが1回以上あり、Controlと異なるstdoutも正確に62 seedであった。
- WideGateでRejectが0回だった39 seedは、39/39でWide stdoutとbyte一致した。残る61 seedは全てRejectが1回以上あり、異なるstdoutも正確に61 seedであった。
- Reject不採用時に既存root confirmation予算を消費する交絡、risk計算だけで後続方策が変わる交絡は観測されなかった。

### connected risk funnel

| 指標 | Gate | WideGate |
|---|---:|---:|
| baselineの経済的connected | 20,086 | 19,966 |
| baselineの`risk>=1` | 9,661 | 9,545 |
| repacking後も残るconnected | 19,442 | 19,332 |
| raw BFS / multi-start / GrowAndTrim | 599 / 8,085 / 10,758 | 608 / 8,058 / 10,666 |
| `risk<1` bypass | 10,266 | 10,261 |
| eligible | 9,176 | 9,071 |
| screen budget skip / attempt | 8,385 / 791 | 8,280 / 791 |
| screenでReject勝ち / Accept保護 | 147 / 644 | 148 / 643 |
| holdout承認 / 棄却 / budget skip | 99 / 48 / 0 | 97 / 50 / 1 |
| 最終方策Reject | 99 | 97 |
| 放棄した現在料金 | 6,071,345 | 6,071,956 |

Gateのeligibleは進行4区間で`2,057 / 2,269 / 2,486 / 2,364`。`expected_overlap<1`なら必ずbypassする設計は機能しているが、長期滞在なら第4区間にもeligibleは残る。最終Rejectの区間別・source別内訳は今回の固定診断にはないため、「序盤・中盤だけRejectすべき」までの直接結論は出せない。

Gateの最終Rejectは、repacking後connected 19,442件のうち99件、`0.509%`だけである。eligible 9,176件に対しても`1.079%`であり、実測は「三生成器を一律に拒否」ではなく、ごく一部だけを拒否した方がよい可能性を示す。screenは固定予算により791件しか評価していないので、この比率を全eligibleの真の最適Reject率とは解釈しない。

Gateのrisk処理CPUは合計`21,217.064ms`、1 case平均`212.171ms`、case最大`573.188ms`、単一turn最大`224.264ms`。Control比のsolver CPU増`231.708ms/case`の約91.6%をrisk処理が占める。WideGateは平均`224.010ms/case`、単一turn最大`227.853ms`であった。

### score損失分解

各値はControlとの差。shape lossとfragmentation ideal feeは負ほど改善、movement costは正ほど悪化である。

| arm | accepted ideal fee | initial shape loss | accepted final fee | movement cost | NoRegion ideal fee | fragmentation ideal fee | 最終score |
|---|---:|---:|---:|---:|---:|---:|---:|
| Wide | -18,865,631 | -11,202,205 | -7,663,426 | -14,007 | +22,606,377 | +14,971,464 | -7,649,419 |
| Gate | -11,078,482 | -21,145,410 | +10,066,928 | +196,345 | +574,699 | -14,421,898 | +9,870,583 |
| WideGate | -15,952,220 | -19,956,610 | +4,004,390 | +78,161 | +10,954,830 | -7,200,720 | +3,926,229 |

WideはControlよりconnected placement successを22,354から22,194へ160件、`0.716%`しか減らさなかった。形状損は11.20M改善した一方、後続を含むaccepted ideal feeを18.87M失い、最終料金が7.66M減った。単純に`Lmin+8`まで列挙してconnectedへ落ちる頻度を下げる案は、目的に対して効果が小さく、盤面連鎖を悪化させた。

Gateはaccepted数が23件減ったが、fragmentation起因の拒否は101件、fragmentation ideal feeは14.42M減った。accepted ideal feeを11.08M失う一方、shape lossを21.15M改善し、accepted final feeは10.07M増えた。99回の選択的Rejectが空間を残し、後続の量を単純に増やすより、後続をよりcompactに置く側へ効いたという分解である。ただしseed差のCIは0をまたぐ。

## 採否と残る問い

- **Wideは棄却。** Control比totalが負という事前条件に違反し、WideGateもGate比で`-5,944,354`。通常templateを一律`Lmin+8`へ広げない。
- **Gateは通常提出へ採用しない。** totalは`+0.151501%`だがbootstrap下端が負で、solver CPU平均`+24.246%`・p95`+27.982%`が事前上限`+15%`を超えた。
- **WideGateも採用しない。** totalは`+0.060263%`に留まり、bootstrap下端が負、solver CPU平均`+30.124%`・p95`+36.138%`、1800ms超2件で停止条件に違反した。
- 通常提出候補は引き続き無フラグbaseline、合計`6,515,194,836`。

仮説自体は一律棄却しない。Gateの小幅正、厳密なzero-Reject byte一致、shape lossとfragmentation lossの改善は、「序盤・中盤の高risk connected受入の一部が後続compact配置を壊す」という方向と整合する。一方で全connectedの約半数が`risk>=1`なのに、固定予算rolloutが最終Rejectしたのは0.51%だけであり、三生成器全てをhard rejectする根拠にはならない。

次に検討するなら、新しい明示指示を受けた別実験として、同じ因果隔離を保ちながら、(1) 791回のscreenに使ったCPUを減らす、(2) 最終Rejectの時期・source・risk帯を診断する、(3) 高価なrolloutの前に予測精度を落とさないcheap filterを作る、の順で原因を分ける。今回の実行結果を見た同一実験内ではコード・閾値・予算を変更しない。
