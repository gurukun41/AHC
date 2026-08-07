# connected near-template polish + 限定root expert v31

更新日: 2026-08-07 JST

## 状態

- 直前の実行正本は`result_20260807_220201.json`。seed 0〜99を100/100 AC、合計`6,621,941,047`、平均`66,219,410.47`。
- この記録は、その結果を受けたユーザーの新しい明示指示に基づく次候補を、解答初回実行前に固定したものである。
- v31は`main.cpp`へ実装済み。source SHA-256は`7133ceb3f6b4ea84154c4d01520fbca10f7f5a07902dc2f86f3c3fd935db9a5f`、行数は6,431行で、最終smokeおよびユーザー実行の100 seed後も不変である。
- エージェントは全実装・記録・静的監査後に警告付きcompileとseed 0 smokeを一回だけ実行し、その後はソースを変更しなかった。
- ユーザーの`test`実行は`result_20260807_232956.json`。seed 0〜99を100/100 AC、合計`6,634,405,584`、平均`66,344,055.84`、WA 0で新最高となった。
- 実行後の本追記は、ユーザーの新しい「確認と記録」指示だけに基づく。解答source、方針、定数は変更せず、エージェントは解答プログラムを追加実行していない。

## 残差

保存済み100 seedの旧配置軌跡を再生すると、acceptedの`ConnectedGrowth` / `GrowAndTrim`はshape lossの中心である。

- growth + grow-and-trim採用: 19,063件
- 旧shape loss: `1,043,940,000`付近
- `P>=50`だけで旧shape loss `978,022,000`付近、全体の93.69%
- expert別growth loss: e0 約532M、e1 約189M、e2 約111M、e3 約211.6M

この残差は、受入価格全体を再調整するより「既に経済的に受け入れたconnected形状を同面積のまま短周長化する」変更に適する。

## 比較した方針

### A. connected Reject risk gateの再導入

v26 Gateは開発100 seedで`+0.1515%`だったがbootstrap区間が0をまたぎ、solver CPU平均を約212ms、24.25%増やした。現在baselineもp95 `1747.041ms`、max `2054.115ms`で時間余裕が小さい。さらにRejectはideal feeを失う非単調変更であるため、今回は重ねない。

### B. 最小周長Ferrers/polyominoの固定列挙

最小周長polyominoの理論に沿い64形程度を全配置する案を、保存済み全accepted growthへ厳密再生した。しかし改善可能だったのは268件、即時料金上限は`2,812,500`に留まった。障害物に合わせて変形できない固定形だけでは残差被覆が小さいため棄却した。

### C. near-template dense box deformation

`P..P+16`の近正方形boxを盤面全体へ置き、box内の空き連結成分を関節点回避で`P`セルへ削る。固定形ではなく現障害物へ適応し、old connectedだけを置換候補にする。

厳密な存在診断は次のとおり。

```text
P >= 50
P <= h*w <= P+16
2(h+w) <= min(Lold-2, Lmin+4)
box内free誘導グラフの最大連結成分 >= P
```

- 4,608 / 15,331件で成立
- 該当旧shape-loss上限: `159,873,768`
- P50〜99: 3,521件 / `110,249,367`
- P100〜150: 1,087件 / `49,624,401`
- slack 8以下だけでは2,261件・約57.1Mであり、9〜16の追加が2,347件・約102.8Mを覆うため`+16`を採用

現実装相当のglobal 12 + 象限補完、最大16 anchor、2種類trimを近似再生すると、2,203件、総周長`-9,726`、旧料金比`+26,958,022`だった。inc/absの最終tieとfuture-fit/root rollback前なので性能予測ではなく候補生成上限だが、固定Ferrersより一桁広い。

### D. repeated one-cell perimeter descent

旧領域の非関節境界セルを1個外し、外側frontierの凹部セルを1個加える。同じselected集合に対し、削除セルのselected近傍数を`k_r`、削除後の追加セル近傍数を`k_a`とすると、

```text
delta perimeter = -2 (k_a - k_r)
```

である。`k_a>k_r`だけを最大8 step反復するため、面積と連結性を維持し、各stepで周長を2以上短縮する。保存軌跡の`P>=50`では811件、総周長`-2,052`、丸め後料金`+4,166,619`を確定した。15,330試行のstep分布は全体mean 0.0669 / p95 1 / max 6、改善811件ではmean 1.265 / p95 3 / max 6で、8を超える例は0だった。旧候補を失わずworst caseを抑えるため32ではなく8を採用し、denseが作れない局所凹凸を拾う補助とする。

### E. 限定root weight expert

保存済みv29 cacheで、rootの比較だけを

```text
1000 * scenario_count * direct_gain
  + root_future_weight_milli * sum(future_delta)
```

とし、`.70<=E/G<.80 && R<.060`だけ未来重みを1000から100へ下げる静的expertを再合成した。

- search 400: `+9,537,366 (+0.035084%)`
- validation 300: `+6,228,032 (+0.029803%)`
- 全700: `+15,765,398 (+0.032788%)`
- 11勝686分3敗
- 対象枝内: search `+1.3654%`、validation `+1.2042%`

`E/G>=.80`へ広げるとvalidationが`-0.0295%`だったため広げない。DLP・placementはその領域の旧baselineを保ち、root比較の全screen/confirmation経路だけ同じruntime整数helperへ通す。

## 採用した合成方針

### 1. 第五の静的expert

従来4領域を保ち、次の部分領域だけexpert 4へ分ける。

```text
.70 <= E/G < .80 && r_milli < 60
DLP scale = 1.00
placement = baseline 3/6/16/8/8/0.25
root future weight = 0.10
```

それ以外はroot future weight 1.00で従来どおり。初期盤面と入力Rだけでケース開始時に固定し、到着列や途中scoreを見て切り替えない。

### 2. 実ターンのold connectedだけをpolish

通常placementを従来どおり最後まで選んだ後、次を全て満たすときだけ発火する。

```text
実ターンである
P >= 50
旧sourceがConnectedGrowthまたはGrowAndTrim
旧丸め後料金 > scaled opportunity cost
Lold > Lmin
```

synthetic rolloutでは候補生成を行わず従来の軽量方策を保つ。rootの数百policy stepへ全盤面box走査が再帰的に掛かることを防ぐためである。

### 3. dense候補生成

全eligibleへ無制限に実行するとcase mean 151.87 / p95 310 / max 385回になり、2秒制約へ危険だった。そこで理論上の最大即時改善

```text
U = fee(V,P,Lmin) - old_fee
```

をexactな`round_payment()`で先に求め、`U>=10,000`かつケース中の実dense走査が24回未満のときだけ次を実行する。予算はUを通過して実走査した時だけ消費し、syntheticは0回である。

1. blocked prefixと50-bit free rowを作る。
2. `P<=hw<=P+16`かつ`2(h+w)<=min(Lold-2,Lmin+4)`の全box anchorを走査する。
3. freeセル数とfree内部共有辺から`4q-2e`でfree集合周長を求める。
4. free周長、box周長、free数、縦横差、temporal cost、固定列挙順でglobal上位12を残す。
5. 四象限最良を補完し、合計最大16 anchorにする。
6. box内free成分がちょうどPならそのまま候補化する。
7. Pより大きければTarjan関節点を毎回再計算し、incremental/absolute costの順を入れ替えた2種類でPまでtrimする。
8. 面積、連結性、池、占有、旧周長よりstrict短いことを再検証し、重複を除く。

保存replayでは、Uなしのdense+swap unionが`+29,649,627`。denseだけを`U>=10,000`かつ先着24回、swapを全eligibleにするとdense callは平均23.66 / max 24へ84%減り、union概算`+13,121,000`、無制限の44.26%を残した。先着だけ、P、周長超過によるfilterよりgain/callが良いためこの組合せを選んだ。

### 4. 局所swap候補

まずselected境界の最小近傍数とfrontierの最大selected近傍数をO(P+frontier)で求める。remove後にadd近傍数は増えないため、`max_add<=min_remove`なら正のswapは不可能であり、Tarjanと全組合せを方策厳密同値に省く。必要条件を通ったときだけ旧selected領域の関節点をTarjan DFSで求め、合法frontierとの全組合せから、周長gain最大、incremental差最小、absolute差最小、座標順で1 swapを選ぶ。strict周長降下がなくなるか8 stepに達するまで反復し、全中間形を周長tierへ残す。swapはdenseの24回予算を消費しない。

### 5. 共通の採用保護

denseとswapを周長tier別の同一shortlistへ入れる。

1. 丸め後料金が旧料金をstrictに上回る候補だけ残す。
2. 周長tierを短い順に見る。
3. 各tier内は従来のincremental / absolute / 象限shortlistで圧縮する。
4. 旧案と同じ3 snapshotでfuture-fitを比較する。
5. tier内最良のfuture-fitが旧案以上なら採用し、未満なら次の周長tierへ進む。
6. 全tierが失敗すれば旧案へ完全rollbackする。
7. polish採用時も旧connected案をrootの第1runner-upへ残す。
8. polishで周長が`Lmin+4`以下へ下がっても、旧connected周長をCompact rescueと通常rootの発火参照に使う。rescue移動先rankingも旧cellsを使う一方、direct gainは改善後polish料金と比較する。

したがって、この局所置換だけでRejectをAcceptへ変えず、現在料金を落とさず、既存future-fit proxyも落とさない。長期scoreの真の単調性を証明するものではないが、過去のhard geometry gateより保護境界が明確である。

## 文献との対応

- [Kurz, Counting polyominoes with minimum perimeter](https://arxiv.org/abs/math/0506428): 最小周長と近正方形envelope、角削除の構造。box面積・周長gateの根拠。
- [Seo and Kim, L-shaped submesh allocation](https://doi.org/10.1016/S0164-1212(02)00086-9): 矩形優先と、失敗時だけ柔軟形へ広げるfragmentation-aware allocation。template失敗後のconnected限定gateと整合。
- [Tarjan, Depth-First Search and Linear Graph Algorithms](https://doi.org/10.1137/0201010): 関節点を線形時間で求め、連結性を壊す削除を除外する基礎。
- [Zhang et al., Space Defragmentation Heuristic](https://www.ijcai.org/Proceedings/11/Papers/123.pdf): 断片化した空間へtarget-specificな局所変形を行う考え方。
- [Re-solving Heuristics with Uniformly Bounded Loss](https://arxiv.org/abs/1802.06192): 高頻度な大域再最適化ではなく、保護付き限定更新を選ぶ根拠。
- [Fully-Dynamic Bin Packing with Limited Repacking](https://arxiv.org/abs/1711.02078): 動的packingで変更量をbounded recourseへ制限する考え方。今回の局所swapと旧案rollbackに対応。

## 診断

stderrへ次を追加する。

- expert 4とruntime root weight
- dense eligible、全anchor、feasible、shortlist、component failure
- connected polish eligible、dense U filter、24回budget skip、実dense callのpartition
- trim attempt / cell / failure / duplicate / candidate
- fee非改善、future-fit reject、採用周長改善、最終source
- perimeter descent attempt / step / candidate / fee非改善 / future-fit reject / 採用周長改善
- accepted dense / descentの件数、ideal fee、initial fee、perimeter excess
- connected polish source整合性error

既存のscore再構成、status、source、DLP、grow-and-trim、Push-out保存則も維持する。

## 静的監査結果

最終sourceをSHA-256 `7133ceb3f6b4ea84154c4d01520fbca10f7f5a07902dc2f86f3c3fd935db9a5f`、6,431行で凍結した。解答compile・実行なしの独立監査と主担当監査でblocking issueは0だった。

- root weight 1000は旧式全体の1000倍で、符号、順位、tie、診断金額が厳密同値。confirmation、rescue、normal-rootの全call-siteが共通helperを使う。
- denseとswapの面積、空きセル、4近傍連結、関節点、周長式、重複除去を監査した。
- old Accepted、strict丸め料金増、future-fit非悪化、次tier / old rollbackを確認した。
- polishが旧周長gateを下回る相互作用を修正し、Compact rescue gate、preferred destination cells、normal-root発火を旧rollback viewで保存した。直接利益は改善後baseline基準のままである。
- denseのU filter / 24回budget / synthetic 0回、swapの8 step / sound prefilterを確認した。
- source追加・root上書き後の診断、dense gate partition、実attempt countの保存則を確認した。
- C++上の参照寿命、範囲、50-bit shift、128-bit root/料金計算にblocking issueなし。
- `git diff --check`はexit 0。placeholder、旧singular descent名、stale `dense_choice`は残っていない。
- tester SHA-256は`3702067f731de62b30a395f99eee04a4a9e247a1f421e2c42556a9e45b62ec92`、smoke inputは`61857f9adeb56546a876f9f54eec3e95be617d4a1135d987ae790a2248304754`。

上記監査後、固定コマンドで警告なしcompileとseed 0 smokeを実行した。exit code 0、score `55,762,976`、solver CPU `1366.475ms`で、新旧の不変条件診断は0、score再構成値はtesterと一致した。その後、ユーザーの次の明示指示まで編集・追加実行を行わなかった。

## 開発100 seed実測

ユーザー実行の[result_20260807_232956.json](../../pahcer/json/result_20260807_232956.json)を正本とする。comment `test`、seed 0〜99、100/100 AC、`wa_seeds=[]`である。seed 0の`55,762,976`は最終smokeと一致し、現在sourceのSHA-256もfreeze値と一致する。JSON自体にbinary hashとtagはないため、実行artifactの同一性はその範囲で確認した。現在の`a.out` SHA-256は`5613b1087748d4589a0d45630d99d863c6afe1c81f166250e2b1274034ccab66`。

| 実装 | 合計score | 平均score | 直前差 |
|---|---:|---:|---:|
| 4-expert + `LegalAnchorIndex` | `6,621,941,047` | `66,219,410.47` | - |
| v31 connected polish + 限定root expert | `6,634,405,584` | `66,344,055.84` | `+12,464,537 (+0.188231%)` |

- paired勝/分/敗は`62 / 6 / 32`。最大改善はseed 32の`+4,106,810`、最大悪化はseed 42の`-2,452,512`。seed ratioのp05 / worstは`0.967241 / 0.961565`。
- 100,000回paired bootstrap（固定seed 69031）の合計比差95%区間は`[-0.120014%, +0.498061%]`で0をまたぐ。非同点94件のexact sign testは`p=0.00258731`。勝率の信号とscore量の不確実性を分けて扱う。
- expert分布は`70 / 13 / 1 / 11 / 5`。expert 4以外でpolishが動いた91 seedは`+12,804,588`・`60/2/29`、expert 4かつpolishが動いた5 seedは`-340,051`・`2/0/3`、expert 4以外かつpolishなしの4 seedは4/4完全同点だった。expert 4の5 seedは全てpolishも動いたため、限定rootとpolishの個別寄与はこのA/Bだけでは分離できない。
- connected polishはeligible `15,815`回。denseは価値上界filter `4,100`、budget skip `9,347`、実走査`2,368`でpartitionが一致し、dense最終choice `158`、descent最終choice `504`。合計周長改善counterは`714 + 1,234`、placement上のpolish changeは`690`回だった。
- 内部solver CPUはmean `1200.498ms`、p95 `1629.677ms`、max `1916.647ms`、2秒超0/100。直前の`1240.126 / 1747.041 / 2054.115ms`、2秒超1/100から悪化しなかった。Pahcer wallはmean `1.883583s`、p95 `2.841406s`、max `3.417648s`だが、`threads=0`の並列tester・I/O待ちを含むためCPUの因果評価に使わない。
- 100 logの53種類の`error` / `mismatch`診断は全件0。全100 seedでscore再構成値はJSON scoreと一致した。
- 保存済み100/100 ACの139 run中でraw合計1位。ただし同じ開発100 seedを繰り返し参照しており、bootstrap区間も0をまたぐため、fresh汎化の証明ではない。

## 実行前に固定した停止条件

- root weight 1000で旧式の符号・順位・診断金額が厳密同値であること。
- root screen、rescue、normal alternative、confirmationの全比較がruntime helperを使うこと。
- dense/swapが実ターンのold Accepted connectedだけで発火すること。
- 候補の面積、合法性、連結性、strict周長短縮、strict丸め後料金増を確認すること。
- future-fit不合格時に次tierまたは旧案へ戻れること。
- selected sourceの追加・削除がroot上書き後も診断と一致すること。
- stale code、未使用定数、古いコメントを残さないこと。
- `git diff --check`、警告付きC++20 compileを通すこと。
- 最後の一回は次の固定コマンドだけとし、その後は編集・追加実行をしないこと。

```bash
g++ -std=c++20 -O2 -DNDEBUG -Wall -Wextra -Wshadow -Wpedantic main.cpp \
  -o /private/tmp/ahc069-connected-polish-v31 && \
./tester /private/tmp/ahc069-connected-polish-v31 < tools/in/0000.txt
```

## 残る不確実性

- denseの`+26.96M`、swapの`+4.17M`は保存軌跡上の候補生成上限であり、future-fitとroot rollback後の最終score予測ではない。
- 同じ開発100 seed由来の残差を設計に使っており、freshな汎化保証ではない。
- future-fitは長期価値の完全な教師ではないが、未較正な新proxyを追加せず既存保護を流用した。
- 実ターン限定・dense最大24回・swap必要条件filterでも追加計算はある。意味保存anchor indexで得たCPU余裕を使うが、baseline最大は既に2秒をわずかに超えており、時間安全性は単一smokeでは証明できない。
- 限定root expertは保存cacheのpost-hoc合成で、今回100 seedでは全5対象seedでpolishも動いた。合成方策は実測したが、root個別の効果は独立評価できていない。重み1.0のケースは旧root式を保つ。
