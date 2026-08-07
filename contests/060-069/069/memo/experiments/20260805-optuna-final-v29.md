# Optunaによる最終パラメータ調整 v29

## 状態

2026-08-06にユーザーが全手順を実行。全blockが独立validationでbaselineへ戻り、非baseline候補は不採用。Optuna v29はここで終了する。

この実験は、ユーザーが十分に考察できない期間にも、事前固定した規則だけで既存baselineの定数を探索し、根拠が弱ければ自動的にbaselineへ戻すための最終調整である。`main.cpp`は変更せず、調整用solverを`main-optuna-v29.cpp`へ分離した。

## 方針と変更範囲

新しい方策を後付けで増やすのではなく、既存実装内で意味が明確な8定数だけを対象にする。defaultは全て現行baselineと同じである。

| block | パラメータ | default | 探索範囲 | 意味 |
|---|---|---:|---:|---|
| admission | `dlp_scale_milli` | 1000 | 750..1300、25刻み | 実到着と仮想未来で共通のDLP機会費用倍率 |
| placement | `future_fit_min_weight_milli` | 250 | 0..500、50刻み | 3 snapshotの平均に混ぜる最悪値の重み |
| placement | `placement_global_shortlist` | 3 | 2..5 | incremental cost上位を残す数 |
| placement | `placement_shortlist_limit` | 6 | 4 / 6 / 8 | future-fitへ渡す総候補数 |
| placement | `connected_growth_seed_limit` | 16 | 8 / 12 / 16 / 24 | multi-start connected-growthのseed上限 |
| placement | `grow_and_trim_extra_cells` | 8 | 4 / 8 / 12 | trim前に余分に成長させるセル数 |
| placement | `grow_and_trim_candidate_limit` | 8 | 4 / 8 / 12 | grow-and-trim試行上限 |
| root | `root_future_weight_milli` | 1000 | 0..1500、100刻み | rescue・通常runner-upの共通未来差の重み |

root重みは2-scenario screen、8-scenario confirmation、NoRegion Push-out、通常runner-upの符号判定と順位へ一貫して適用する。整数を1000倍して比較するため、default 1000では元の符号と順位を保つ。DLP倍率も通常admissionだけでなく、仮想未来とPush-outの経済gateへ共通に適用する。

次は既に棄却または未確立なので探索対象へ戻さない。

- 通常templateを`Lmin+8`へ広げるv26 Wide
- connected候補をhard Rejectへ送るv26/v27 gate
- v28で不採用になったoffline ridge
- repackingの探索幅・深さ・候補機構そのもの

これにより、admission、placement、後段rootの原因を分けたまま調整する。repackingの新方式は作らず、root blockは既存repacking候補を未来差で採る重みだけを動かす。

## 事前固定したseedと探索

公式generatorからfresh seed 1300〜2699の1,400件を自動生成し、次の非重複区間へ固定する。

| 用途 | seed | 件数 | 使用回数 |
|---|---:|---:|---|
| Optuna search | 1300..1699 | 400 | 各trialを64→200→400件で段階評価 |
| block validation | 1700..1999 | 300 | 各blockのsearch上位3候補だけ |
| combination validation | 2000..2299 | 300 | block winnerの最大8組合せ |
| final holdout | 2300..2699 | 400 | 固定した最終候補1件だけ |

trial数はadmission 24、placement 72、root 24。各studyの最初にdefaultを1件投入する。64件時点では相対scoreが`-1%`未満の明白な悪化だけを除外し、200件から`SuccessiveHalvingPruner`の上位約1/3を400件へ進める。探索目的はpairedなraw scoreについて、

```text
relative_score = Σ(candidate_score - baseline_score) / Σbaseline_score
```

を最大化する。baseline結果は同じseed・binary hash単位で共有し、candidateとのseed集合不一致を許さない。

各blockは他blockをdefaultへ固定して探索する。search上位の異なる3候補をblock validationへ送り、gateを通る最良だけをblock winnerにする。次に非baseline winnerの全部分集合をcombination validationで比較し、最良の1候補を固定してからfinal holdoutを1回だけ開く。

## 採用gate

block validation、combination validation、finalで共通に次を要求する。

- 合計raw score差がstrictに正
- 幾何平均score ratioが1以上
- seed別ratioのp05が0.98以上、worstが0.90以上
- `timing_solver_cpu_ms`の平均比が1.10以下、p95比が1.15以下
- candidateのsolver CPU最大が2000ms未満

finalではさらに、seed bootstrap 5,000回による相対score 95%区間の下端が`-0.0005`以上であることを要求する。どの段階でも候補が残らなければbaselineを選ぶ。finalで落ちた場合も、出力する`main-optuna-final.cpp`は`main.cpp`とbyte-identicalなbaselineになる。

## 固定artifact

| artifact | SHA-256 |
|---|---|
| `main.cpp` | `086cb6cc2e24848c77a4b766ab8dde433dbf16469095cca557f848bdff091c6a` |
| `main-optuna-v29.cpp` | `37dd54dfd3f2182506fe8c57cfc8bf4e07aa355c868ce20c63c452f23b7088bb` |
| `tools/optuna_final_v29.py` | `a828ddba0d75c34848d0f73de2f4d8b07283a99f120ce3d498269c20bbaa4dd8` |
| `tools/optuna-final-v29-spec.json` | `abe005098b53c2e1978952653b07bbb9cf56924eed71b269ad6fb9538d41d86f` |
| `tools/requirements-optuna-v29.txt` | `4bd64bf3ab9d1b1907ff1b639eff2f46185b04642bbd4e5bd6e735c84f7fb92d` |
| `tester` | `3702067f731de62b30a395f99eee04a4a9e247a1f421e2c42556a9e45b62ec92` |
| `tools/generate_seed_corpus_v28.py` | `a37b024ecb58f47cb845a25ac59a78da635178394b41110cf65d7b880ce94962` |
| 既存sanity入力seed 0〜99のordered digest | `009ae2f6075cf596b0129c6378a085b63dabd2ea78b4fa1dcf5d12cab38e7e39` |

specはOptuna driver自身、baseline、tuning source、tester、入力生成driver、既存100入力のhashを実行時に再検査する。各buildはsource・compiler identity・flags・全macroのfingerprint、各caseは入力・tester・binaryのhashを持つ。studyにも同じ固定artifact identityを保存し、異なる条件でのresumeを拒否する。

Optunaはsamplerのversion差を避けるため`4.9.0`へ固定する。placementの総shortlist数を先にsampleし、global上位数の上限をそれ以下にする条件付き空間を`group=True`のmultivariate TPEで扱う。

## 実行前の静的検証

- `python3 -m py_compile tools/optuna_final_v29.py`: 成功
- specのJSON parseとparameter所属・default整合検査: 成功
- `main.cpp` raw baseline、`main-optuna-v29.cpp` default、代表的な非defaultの3 build: GNU++20、警告・出力なしで成功
- baselineが選ばれた場合の最終sourceが`main.cpp`とbyte-identicalになる静的検査: 成功
- 実装時点ではOptuna package未導入で、解答solver実行・seed生成・探索は未実施だった
- ユーザー実行時は固定したOptuna `4.9.0`を導入した

## ユーザーが実行した手順

以下は大規模実行を含むため、エージェントは実行しない。

```bash
cd /Users/user37/kyopro/AHC/contests/060-069/069
python3 -m pip install -r tools/requirements-optuna-v29.txt
python3 tools/optuna_final_v29.py check --compile
python3 tools/optuna_final_v29.py prepare-inputs
python3 tools/optuna_final_v29.py sanity --jobs 8 \
  --execution-token USER-RUNS-V29-1400-SEEDS
python3 tools/optuna_final_v29.py run --jobs 8 \
  --execution-token USER-RUNS-V29-1400-SEEDS
```

`sanity`はraw baselineとdefault tuning buildを既存seed 0〜99でpaired実行し、100/100のscore・solver stdout byte一致と、baseline合計`6,515,194,836`を確認してreceiptを作る。`run`はそのreceiptが現行source・tester・compiler・既存入力と一致しなければ開始しない。`--jobs`は1〜32で、マシンの実コア数や負荷に応じてユーザーが変更できる。

探索DB、case cache、各validation証跡、final lock/resultは`optuna-v29/`へ保存する。最終提出候補は`main-optuna-final.cpp`へ生成する。中断後は同じコマンドで成功済みcaseを再利用できるが、source、spec、tester、compiler identityが変わったstudyは混用しない。

## 実行後の扱い

`sanity`または`run`で解答プログラムを実行した後は、`AGENTS.md`に従う。エージェントは結果を報告して停止し、ユーザーの新しい明示指示なしにsource、探索空間、seed分割、gate、driver、実験記録を結果へ合わせて変更しない。

## 実行結果

### default互換sanity

既存seed 0〜99でraw baselineとdefault tuning buildをpaired実行した。100/100 seedでsolver stdoutがbyte一致し、score合計も固定oracle `6,515,194,836`と一致した。

| build | solver CPU平均 | solver CPU p95 |
|---|---:|---:|
| raw baseline | 1692.478ms | 2253.800ms |
| default tuning | 1686.033ms | 2258.170ms |

したがって、別solverへ調整値を導入したこと自体によるdefault行動差は観測されていない。

### Optuna search

固定trial数を全て処理し、FAILは0だった。

| block | COMPLETE / PRUNED | search 400 seedのbest | best parameter |
|---|---:|---:|---|
| admission | 13 / 11 | `+1.927351%` | `dlp_scale_milli=1300` |
| placement | 41 / 31 | `+0.358531%` | min weight 500、global 4、shortlist 8、growth seed 24、trim extra 12、trim limit 12 |
| root | 5 / 19 | `+0.054529%` | `root_future_weight_milli=100` |

これはsearch集合内の順位であり、採用結果ではない。同値parameterをTPEが複数回sampleしたtrialもある。

### block validation

searchと非重複のseed 1700〜1999で、各blockの上位3候補を比較した。下表は各blockで相対scoreが最良だった候補である。

| block / candidate | relative score | 勝/分/負 | p05 | CPU平均比 / p95比 / 最大 | 不合格gate |
|---|---:|---:|---:|---:|---|
| admission `dlp=1300` | `+1.988659%` | 207 / 17 / 76 | 0.977661 | 0.949 / 0.979 / 2733.264ms | p05、絶対CPU最大 |
| placement search best | `+0.162910%` | 157 / 0 / 143 | 0.967925 | 1.059 / 1.087 / 3293.954ms | p05、絶対CPU最大 |
| root `weight=100` | `+0.056088%` | 114 / 64 / 122 | 0.977916 | 0.984 / 0.963 / 2699.931ms | p05、絶対CPU最大 |

admission上位3件は全て合計`+1.67%`以上だったが、全件でp05 0.98と絶対CPU最大2000msを満たさなかった。placement上位3件も小幅正だったが、p05は0.965〜0.969で裾が悪く、最大CPUは3160〜3294msだった。

root `weight=1100`は`+0.007707%`、`weight=900`は`+0.012391%`で、score分布・CPU比gateは通ったが絶対CPU最大だけで落ちた。今回の同一300 seedでbaseline自身のCPU最大は2714.630ms、p95は2146.474msであり、固定した絶対2000ms gateは現実行環境では全非baseline候補への拒否条件として働いた。これは事前固定規則どおりの判定だが、平均・p95の相対CPUが悪化したという意味ではない。

全blockでeligibleな非baseline候補が0だったため、block winnerはadmission / placement / rootの全てでbaselineになった。

### combination validationとfinal

非baseline block winnerがなかったので、combination validationはseed 2000〜2299でbaseline 1件だけを確認した。final候補もbaselineのため、seed 2300〜2699のfinal holdoutは開いていない。このため`candidate_stats`とbootstrap CIは`null`である。

最終結果は次のとおり。

- `candidate_adopted=false`
- `selected_baseline=true`
- failure: `no non-baseline combination passed both validation stages`
- `main-optuna-final.cpp` SHA-256: `086cb6cc2e24848c77a4b766ab8dde433dbf16469095cca557f848bdff091c6a`
- 上記hashは`main.cpp`と完全一致

### 実行後artifact

| artifact | SHA-256 |
|---|---|
| `tools/input-manifest-optuna-v29.json` | `b04eac5e804bab52882b74fd8a8943eca086eaaa680fce4eeb356511c76c4f61` |
| `optuna-v29/sanity-receipt.json` | `990d126e90be37db500e9af67aa080e762aab58e1edb23909dfacce7fd972fc5` |
| `optuna-v29/block-validation.json` | `73aedc90f2fac6b04673aad4d329da2f43d07acb792ff2d0e202514c2779deab` |
| `optuna-v29/combination-validation.json` | `cca3b830b80d4c8bf73cfef597e1b4a9f4a32101852ac9b54b25ef453392ea39` |
| `optuna-v29/final-lock.json` | `2690da36415f1ed4af6adc9624ffe35afe6047df18b45a6d09094c676f9a93d6` |
| `optuna-v29/final-result.json` | `2fe4748c4f904ffa1383befaeedf2858d9bf29a7730cf756b6e2050bb650e0c7` |
| `main-optuna-final.cpp` | `086cb6cc2e24848c77a4b766ab8dde433dbf16469095cca557f848bdff091c6a` |

### 採否

固定protocolの結論は非baseline不採用、通常提出baseline維持である。特に`dlp_scale_milli=1300`は独立validationでも平均scoreに大きい正の信号を示したが、下位裾gateを満たさずfinal holdoutも未実施なので提出候補へ昇格させない。ユーザー判断によりOptuna調整は一旦終了し、この結果を根拠としたsolver・driver・gateの変更は行わない。
