# free-space backbone preserving small descent v36

更新日: 2026-08-08 JST

## 状態

v35の100 seed正本は`pahcer/json/result_20260808_030825.json`で、100/100 AC、合計`6,643,865,507`、平均`66,438,655.07`である。v33比`+2,203,649 (+0.033179%)`、19勝81分0敗だが、改善の93.205%はseed 42へ集中する。

v36は、v35で未変更だった非smoothケースの小規模connected配置だけを対象にする。smoothのv35経路、`P>=50`、dense box、admission、Compact rescue、NoRegion Push-out、root rolloutを変更しない。非smoothかつ`P<50`では、既存strict 1-cell descentの各swapへ「空き連結成分数を増やさない」厳密条件を追加して開放する。

ユーザー実行の100 seedでは平均`66,407,862.19`、v35比`-3,079,288 (-0.046348%)`、8勝88分4敗となった。smooth 70 seedはv35と全件同一で設計どおり保護できたが、非smoothの小さな8改善を4件の大きな後続損失が上回った。v36は100 seed incumbentへ昇格させず、v35を維持する。本結果記録時点では`main.cpp`、方針、定数を変更しなかった。その後のユーザーの新しい明示指示によりv36固有コードを全撤去し、現在の`main.cpp`は実行時v35とbyte単位で一致する。

## 100 seed実測

- 正本: `pahcer/json/result_20260808_105054.json`
- 開始時刻: `2026-08-08T10:50:54.713786+09:00`
- comment: `test`
- seed: 0〜99、100/100 AC、WA 0
- 合計: `6,640,786,219`
- 平均: `66,407,862.19`
- v35差: `-3,079,288 (-0.046348%)`
- 勝分敗: `8 / 88 / 4`
- positive / negative gross: `+41,707 / -3,120,995`
- seed ratio p05 / worst: `1.000000 / 0.966510`
- v33差: `-875,639 (-0.013184%)`
- v34差: `-161,512 (-0.002432%)`
- v31差: `+6,380,635 (+0.096175%)`
- Pahcer wall mean / p95 / max: `1.666176 / 2.542958 / 3.194393`秒

scoreが変わったseedとv35差は、2 `+2,821`、13 `+1,163`、14 `+16,742`、25 `-881,399`、35 `+658`、44 `-1,569,199`、48 `+403`、55 `+18,500`、87 `-382,408`、91 `-287,989`、92 `+1,290`、94 `+130`である。最大悪化はseed 44のratio `0.966510`、次いでseed 25の`0.978876`だった。

初期`E/G`別のv36−v35差は次である。

| 初期`E/G` | seed数 | 差 | 勝/分/敗 |
|---|---:|---:|---:|
| `<0.55` | 70 | `0` | `0/70/0` |
| `[0.55,0.625)` | 8 | `-286,696` | `2/5/1` |
| `[0.625,0.70)` | 5 | `-382,408` | `0/4/1` |
| `[0.70,0.80)` | 6 | `-1,533,554` | `3/2/1` |
| `>=0.80` | 11 | `-876,630` | `3/7/1` |

したがって変更対象外のsmooth 70 seedは完全保護され、差は非smooth 30 seedの8勝18分4敗だけから生じた。空き連結成分数を増やさない保証は実装境界を正しく狭めたが、将来の形状価値や配置連鎖まで保証しない。8改善のgrossは`+41,707`に留まり、4悪化のgross `-3,120,995`が約74.8倍である。負seedや`E/G`帯を見て新しいpost-hoc gateを追加せず、この非smooth拡張全体を非incumbentとして記録する。

Pahcer wallはv35のmean / p95 / max `1.726351 / 2.677900 / 3.679066`秒より小さいが、実行環境差と対話I/Oを含むためsolver CPU改善とは主張しない。100 seed stderrはJSONへ保存されておらず、backbone choiceの総数・即時fee gain・内部solver CPUは本結果から再構成できない。

## 完了済みAHC上位解から得た方針

今回参照したのは終了済みコンテストの公開解説だけであり、開催中AHC069の他参加者・提出は調査していない。

- AHC031 1位は、全日程で維持できる短冊を先に作り、その骨格内だけを局所最適化した。難しい日を先に検査し、空き容量に応じて内部retryを厚くし、可逆変更とrollbackで厳密scoreを高速に扱った。
- AHC031 7位も短冊幅を固定して解空間を縮め、配置順だけを最適化した。逐次1日先読みより、最上位のように全期間へ局所改善を掛ける余地があったと分析している。
- AHC040上位解説は、箱が引っ掛からない配置制約へ最初から解空間を限定し、その安定な構成の中で段数・回転を最適化した。
- AHC065 1位は、全セルを距離1で覆う巡回路という強い骨格で大幅改善し、その後は開始位置・向き・配置方向という低次元差分だけを追加した。
- AHC059上位解説は、exactで安い再挿入を核にしたdestroy-and-repair LNSとrecord-to-record travelを用いた。ただしAHC069では既にCompact rescueと過去のproactive cleanup系列があり、広い再配置近傍を再導入すると既存の失敗と因果が重なる。

共通するのは「候補数を増やすこと」自体ではなく、価値の高い構造を固定し、その不変条件を壊さない低次元近傍へ計算を集中することである。v34の追加分岐は即時料金`+30,216`に対し後続影響`-744,343`だったため、今回は候補portfolioを広げず、空き空間の骨格を保証できる未適用領域だけへ既存近傍を移植する。

## 比較した実装方針

### A. 非smoothの小規模組へv35を無条件開放

実装は最小だが、v31の非smooth polishには大きい負の裾があった。小規模組は障害物間の1セル幅通路を塞ぎやすく、smoothで負seedがなかったことをそのまま外挿できないため棄却する。

### B. 空き連結骨格を保つ小規模strict descent

非smoothかつ`P<50`だけを開放し、各swapで追加セルが中間空きグラフの関節点でないことを厳密判定する。料金、周長、future-fit、root rollbackはv35を維持する。新しい係数・地形閾値・候補sourceを増やさず、非smoothの大規模経路も保護できるため採用する。

### C. 短冊・corridorを通常placement全体の固定骨格にする

上位解の構造的教訓へ最も近く、将来の大案として有望である。一方、AHC069の池盤面で永続corridorを予約すると有効面積と最小周長形を同時に失い得る。過去のDeadline Layer大域再構成はControlの82.69%、空間DLPは約-4%であり、現在のonline placement全体を一回で置換する根拠は不足するため今回は見送る。

### D. destroy-and-repair LNS / record-to-record travel

広いbasinを越えられるが、既存Compact rescue、Push-out、過去proactive cleanupと重複する。v34でも少数追加候補を既存proxyが誤選択したため、exactで安いrepairと独立な選択精度が揃う前に再導入しない。

### E. tight caseへのadaptive retry・multi-start

AHC031 1位の重要な成功要因であり、NoRegionやdense trimの将来候補である。ただし現行solverはonlineで各到着に即答し、v34のmulti-startは負だった。難しさのexact screenとretry先を先に分離せず探索数だけ増やすのは避ける。

## 空き骨格の厳密条件

現在の候補領域を`S`、現在の空きグラフを`F`とする。swapで`r in S`を外し、frontierの`a`を加える。

1. `r`が現在の空きグラフ`F`の少なくとも1セルへ隣接することを要求し、`F+ = F union {r}`を作る。この頂点追加は既存成分へ接続するため、連結成分数を増やさない。
2. `F+`で`a`を除いたまま、`a`の空き隣接セルが全て相互到達可能かBFSする。
3. 相互到達可能なら`a`は`F+`の関節点ではない。よって`F+ - {a}`の連結成分数は`F+`より増えない。

したがって各swap後の空き連結成分数はswap前以下である。全中間候補がこの条件を通るため、最終tierが浅い途中形を選んでも保証は保たれる。これは最大空き成分を目的関数へ加えるproxyではなく、現在の空き骨格を悪化させない二値の合法近傍制限である。料金やadmissionの順位には直接加えない。

## 採用実装

1. smooth (`case_static_expert==0`)はv35のdense/descentを完全維持する。
2. 非smoothは`P<50`かつ、旧方策がConnectedGrowth / GrowAndTrimを経済的にAcceptし、旧周長が最小周長より長い実ターンだけdescentを追加する。
3. removeは従来どおり領域の非関節セル、addは空きfrontier、`k_add_after>k_remove`のstrict周長改善だけである。
4. 非smooth枝では、removeが既存空きへ接することを確認し、最良safe候補を更新し得るswapだけにaddの空き関節点検査を行う。add除去後に確認すべき隣接空きセルが0または1なら即通過し、2以上でも全対象がつながった時点でBFSを打ち切る。
5. 毎step後に面積、池、占有、重複、盤外、領域4連結を再検証する。
6. 候補採用は丸め後料金のstrict増を必須にし、未来がある場合は旧形と共通の3 snapshot scalar future-fitが非悪化の最短周長tierだけを許す。
7. old connectedをroot alternatives先頭へ残し、旧周長によるrescue/root発火とrollbackを維持する。synthetic rollout内では探索しない。
8. backbone枝のattempt / connectivity test / reject / step / candidate / future-fit reject / final choice / 周長改善 / 即時fee gainを独立診断する。rootが別actionへ上書きした場合はfinal choiceと改善値を戻す。

## 過適合と既存経路の保護

- v35で勝ったsmooth全経路は、同じ候補列挙、比較式、tie-break、dense予算を使う。
- 非smoothの`P>=50`はv35と同じstatic filterで何もしない。
- `.55`以外の新しい地形境界、seed別例外、価値閾値、到着時刻gateを追加しない。
- 新しい候補sourceやmulti-startを作らず、v35のgreedy 1本だけを使う。
- 空き成分数は最大化しない。現状態から増加させない不変条件だけに使い、既存料金・future-fit・root比較を置換しない。
- v35の100 seedを繰り返し参照しているため、v36の性能判断はユーザー実行のpaired testとfresh holdoutを分ける。最後のsmokeには新規枝を通さないsmooth seed 0ではなく、最初に見つかった非smooth seed 2 (`E/G=0.999413`)を使う。v35の同seed scoreは`37,704,052`だが、smokeはAC・診断・CPUの確認にしか使わない。

## 文献・公開解説

- AHC031 1位解説: https://blog.oimo.io/2024/04/07/ahc031/
- AHC031 7位解説: https://zenn.dev/tishii2479/articles/7e9f891fb93e46
- AHC040上位公式解説: https://atcoder.jp/contests/ahc040/editorial/11761
- AHC059上位公式解説: https://atcoder.jp/contests/ahc059/editorial/15052
- AHC065 1位解説: https://qiita.com/tanaka-a/items/c286e1d03fd7d7b419ac
- Fiduccia, Mattheyses, *A Linear-Time Heuristic for Improving Network Partitions*: https://doi.org/10.1109/DAC.1982.1585498
- Ropke, Pisinger, *An Adaptive Large Neighborhood Search Heuristic*: https://doi.org/10.1287/trsc.1050.0135
- Balcan, Sandholm, Vitercik, *Generalization in portfolio-based algorithm selection*: https://arxiv.org/abs/2012.13315

上位解の具体構成をAHC069へそのまま移植したのではなく、「骨格を固定して低次元近傍だけを動かす」「難しい部分へ計算を集中する」「候補集合の自由度を抑える」という設計原則を使った。今回のBFSは一般的な関節点の定義を候補限定で直接検査するもので、FMの線形更新やALNSそのものではない。

## seed 2 smoke

実行前freeze後、エージェントは非smooth seed 2を一回だけ実行した。AC、Score `37,706,873`で、v35同seed比`+2,821`、内部solver CPU `1088.423ms`だった。backbone descentは128 attempt、20 connectivity test、18 reject、2 step / candidate、future-fit reject 1、final choice 1、周長改善2、即時fee gain`+2,821`だった。新規3 errorを含む全identity/errorは0で、再構成scoreも`37,706,873`と一致した。smoke後にsource・方針・定数を変更せず、上記100 seedはユーザーが実行した。

## hard diagnostics

次を0必須とする。

- `connected polish candidate = static filtered + eligible`
- `eligible = dense size filtered + dense value filtered + dense budget skip + dense実走査`
- small descent attempt = dense size filtered
- backbone attempt / step / candidate / future-fit reject / choice / 改善量がsmall descent集計以下
- backbone connectivity rejection <= test、step <= test、candidate = step、choice <= candidate
- backbone final choiceの有無と正の周長改善・正のfee gainの一致
- 既存の合法性、source、score再構成、DLP、root、Push-out全error

## 静的検証

- `git diff --check -- main.cpp`: pass
- Apple Clang 17、C++17/C++20、`-Wall -Wextra -Wshadow -Wpedantic -fsyntax-only`: pass、警告0
- Apple Clang 17、C++20、同warning指定のstatic analyzer: pass、出力0
- 実行前の解答プログラム実行: 0回

## 実行前freeze

全編集・静的監査・最終compile後のfreezeは次である。

- `main.cpp`: 6,872行
- source SHA-256: `8302f6b6fbf27180f7f9f5d541976141e26d8ae2bd81bbb1319056c29e31411b`
- release binary: `/private/tmp/ahc069-free-space-backbone-small-descent-v36`
- binary SHA-256: `45b2fbd1b933466400d6371f980b6d1bb58121778c294f5b1e68dccc23b3035f`
- compiler: Apple Clang 17.0.0
- compile: `-std=c++20 -O2 -DNDEBUG -Wall -Wextra -Wshadow -Wpedantic`
- freeze時点の解答プログラム実行: 0回

エージェントの解答実行は、このfreeze後の非smooth seed 2 smoke一回だけとし、実行後はsource・方針・定数を変更しない。

```bash
./tester /private/tmp/ahc069-free-space-backbone-small-descent-v36 < tools/in/0002.txt
```

ユーザーの100 seed paired testは上記正本として実行済みである。本結果の初回記録では`main.cpp`を変更せず、追加実行もしていない。

## 棄却後の完全復元

ユーザーの新しい明示指示を受け、seed別・`E/G`別のpost-hoc gateは追加せず、v36固有の空き連結BFS、非smooth有効化、backbone診断を全て撤去した。現在の`main.cpp`は6,654行、SHA-256 `1a5f652b17ca8de08b34920ea35f1928cfea7008dc98a4c7138b933e22d3db60`で、v35実行前freezeと完全一致する。C++17 / C++20の`-Wall -Wextra -Wshadow -Wpedantic -fsyntax-only`と`git diff --check`は全てpass。復元後に解答プログラムは実行していない。
