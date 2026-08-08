# strict same-gain limited-discrepancy polish v34

更新日: 2026-08-08 JST

## 状態

v33の100 seed正本は`pahcer/json/result_20260808_012614.json`で、100/100 AC、合計`6,641,661,858`、平均`66,416,618.58`である。v34はユーザーの新しい明示指示に基づき、追加自由度と過適合リスクを固定上限で抑えながらsmooth connected polishの未探索幅だけを広げた実装である。

ユーザー実行の100 seedではv33比`-714,127 (-0.010752%)`となったため、v34は棄却した。`main.cpp`はユーザーがv33へ戻しており、同一最大gainの次点分岐を現行方策へ残さない。本記録は実行前freeze、実測、棄却判断を固定する。

## 100 seed実測と棄却

- 正本: `pahcer/json/result_20260808_023641.json`
- comment: `test`
- seed: 0〜99、100/100 AC、WA 0
- 合計: `6,640,947,731`
- 平均: `66,409,477.31`
- v33差: `-714,127 (-0.010752%)`
- 勝分敗: `3 / 95 / 2`

scoreが変わったのは5 seedだけで、差はseed 1 `+1,648`、17 `+9,472`、41 `+3,363`、66 `-466,936`、75 `-261,674`だった。追加枝による当該ターンの即時fee gain合計は`+30,216`だが、最終score差から引いた後続影響は`-744,343`である。合法性・保存則errorではなく、追加候補を選んだ後の盤面連鎖が負差を作った。

5件だけを見てseed 66 / 75を除く新しい地形gateを作るのはpost-hoc過適合になる。したがって閾値を追加せず、同一最大gain multi-start全体を棄却してv33へ戻す。候補portfolioを増やすだけでは、既存3 snapshot future-fitと少数root rolloutの選択誤差を増やし得る、という負の結果として扱う。

## 残差と過適合監査

v33と4-expertの差はsmooth 70 seedで`+18,587,452`、47勝5分18敗だった。保存済みv31 replayではstrict descentを15,330回試し、811 turnで候補を生成した。成功時stepはmean 1.265、p95 3、max 6で、現上限8は一度も拘束していない。深さよりも、同じ最大周長gainのswapが複数ある状態では先頭以外を捨てることが構造的な未探索である。ただし保存済みv31診断はsame-gain tie頻度を直接数えておらず、その発生量はv34実行前には未測定である。

一方、smooth 70 seedのうち、当時のseed別stderrで`dense_box_choices==0 && perimeter_descent_choices>0`だったdescent-only 18 seedについて、v33 `result_20260808_012614.json`から4-expert `result_20260807_220201.json`を引くと合計`-413,820`、15勝3敗だった。seed 59の`-2,172,167`を除けば`+1,758,347`である。分類に使ったstderrは後続rerunで上書き済みで、現在のJSONだけからsubsetを再構成できないため参考残差とする。多数の小改善と少数tailが同居するため、候補幅だけを無制限に増やさず、v33 winnerの無干渉再構成、strict料金増、既存future-fit非悪化、固定CPU予算を同時に要求する。

初期盤面とRの単一stumpをseed 5-fold / LOOCVで調べても、常時smooth polishの`+18.587M`を上回る静的gateはなかった。`.625`境界、`col_cv`境界、到着統計gateはいずれも不安定または実行時利用不能であり、新しいcase分類は追加しない。

## 比較した方針

### A. strict same-gain limited-discrepancy multi-start

現行greedyの各状態で、周長gainが最大のswapを従来tie順に最大2件だけ保持する。先頭は旧greedyとしてそのまま進め、2番目だけを独立枝にして以降は旧greedyを続ける。全moveがstrict周長改善であり、zero/negative moveを許さない。既存候補、採用gate、rollbackを保てるため採用する。

### B. dense trim beam

`component_size-P<=16`の削除順をbeam化すれば候補生成上限は大きい。しかし各depthでTarjanを幅分だけ再計算し、v31のsolver CPU最大`1916.647ms`に対してtail riskが高い。旧候補保護を含む実装も複雑なため、最後一回条件では見送る。

### C. rescue screenへの追加holdout

2 scenario不一致だけ既存8×12 confirmationへ送る案。CRNによる誤選択低減は期待できるが、repacking採否を広く変え、既存rescue gainを削る可能性がある。placement polishと因果を混ぜないため見送る。

### D. concurrent-capacity future-fit / DLP online補正 / 新expert

いずれも通常placementまたはadmissionを広く変更する。v32 capacity guard、空間DLP、連続受入補正の悪化履歴があり、開発100 seedへ新しい閾値を足す自由度も高い。今回の過適合方針に反するため見送る。

## 採用実装

1. `find_best_strict_region_swaps()`は正gainの最大値だけを、従来の`gain / incremental / absolute / 座標`順で最大2件返す。limit 1の先頭は旧関数と同一である。
2. frontierの最大selected近傍数がremoveセルのselected近傍数以下なら、そのremoveから正gainは作れないため全add scanを意味保存で省く。
3. primary branchは従来greedyを最大8 stepまで完全再現し、全中間形を従来順で残す。
4. primary各状態に同じ最大gainの次点があれば、その1本だけを分岐する。追加枝から再分岐せず、総stepはinitialから最大8とする。
5. 追加枝はケース全体で最大8本、追加`find_best` stateも最大8回とする。synthetic rolloutとnon-smooth expertでは従来どおり0回である。
6. 旧primary descentとdenseは従来順でprotected builderへ入れ、v33の最終polish候補を先に旧ロジックだけで確定する。追加枝は別builderへ隔離し、固定6/8枠、旧tier選択、旧tie-breakを変えない。
7. 追加枝はcell集合をexact比較してprotected finalistとの重複を除く。追加枝がv33候補を上書きできるのは、丸め後料金がv33候補よりstrictに高く、かつ同じ3 snapshot scalar future-fitが数値許容幅内で非悪化の場合だけである。完全tieはprotectedを残す。
8. 追加枝が勝っても、old connectedをroot alternatives先頭、v33のprotected polishを次点へ残す。root幅は2のままとし、tail CPUと候補増によるoptimizer's curseを避ける。このため旧通常runner-up末尾まで含む集合の完全なsupersetではない。新しいsource enum、地形gate、admission変更は追加しない。

## 文献との対応

- AtCoder, AHC069問題文: https://atcoder.jp/contests/ahc069/tasks/ahc069_a
- AtCoder, Rules on generative AI in AtCoder Heuristic Contests: https://info.atcoder.jp/entry/ahc-llm-rules-en
- Fiduccia–Mattheyses, *A Linear-Time Heuristic for Improving Network Partitions*: https://limsk.ece.gatech.edu/book/papers/fm.pdf
- Harvey, Ginsberg, *Limited Discrepancy Search*: https://ijcai.org/Proceedings/95-1/Papers/080.pdf
- Lourenço, Martin, Stützle, *Iterated Local Search*: https://arxiv.org/abs/math/0102188
- Mladenović, Hansen, *Variable neighborhood search*: https://doi.org/10.1016/S0305-0548(97)00031-2
- Balcan, Sandholm, Vitercik, *Generalization in portfolio-based algorithm selection*: https://arxiv.org/abs/2012.13315
- Talluri, van Ryzin, *An Analysis of Bid-Price Controls for Network Revenue Management*: https://doi.org/10.1287/mnsc.44.11.1577

正確な位置づけは、VNS全体ではなく「greedyの次善分岐を先に調べるLDSに着想を得て、同一最大gainだけへ一段discrepancyを作る決定的low-width multi-start + strict descent」である。portfolio一般化の文献が扱う候補集合の複雑さと一般化のtrade-offは注意材料にとどめ、width 2、case 8/8は実測済みCPU tailを踏まえた工学的上限として固定する。文献から8/8が最適だとは主張せず、新しい学習selectorも作らない。

ILSは「一度perturbしてから局所探索を続ける」構造、FMはgain順局所改善と選択順依存の着想だけに対応する。本実装はFMのlock / pass / 非改善move / linear-time性を実装せず、近傍構造も切り替えないためVNSそのものでもない。Talluri–van Ryzinは変更しないbid-price admission / DLPの背景であり、今回のmulti-startの直接根拠にはしない。コンテスト中の他参加者の解法・提出コードは調査対象にしていない。

## 診断とhard gate

追加診断は、same-gain tie state、branch開始/予算skip、追加search state、state予算停止、追加step、unique/shortlisted/duplicate候補、validation failure、v33料金dominance reject、future-fit reject、最終choice、周長改善、即時fee gainを分離する。`state`は終端照会を含む追加枝の`find_best`試行、料金dominance rejectは料金非優越tierのshortlisted候補数、future-fit rejectはtier最良の棄却数である。choice・周長改善・fee gainのalternative専用値はrootで別actionへ上書きされた場合に戻す。

次を0必須とする。

- alternative branch/stateのglobal countと集計値の不一致
- branch 8 / state 8超過
- alternative choiceが最終perimeter descent choiceを超える不一致
- `steps<=branches+states`、`candidates<=steps`、`shortlisted<=candidates`、`choices<=shortlisted`のfunnel違反
- alternative candidateの面積・空き・連結性failure
- 既存のstatus/source/score再構成/DLP/root/Push-out全error

## 静的検証

- `git diff --check -- main.cpp`: pass
- Apple Clang 17、C++17/C++20、`-Wall -Wextra -Wshadow -Wpedantic -fsyntax-only`: pass、警告0
- Apple Clang 17、C++20、同warning指定のstatic analyzer: pass、出力0
- 独立read-only差分監査2系統: 最終SHAでblocking 0
  - v33 protected経路、same-gain列挙、move/copy寿命、root、診断、CPU上限
  - strict fee / weak fit dominance、不変条件funnel、optimizer's curse留保
- 実行前freeze時点の解答実行: 0回

## 実行前freeze

全監査と記録を完了した実行前freezeは次である。

- `main.cpp`: 6,976行
- source SHA-256: `4457478c8c0acd64f665b16dc1ea1ff86554feec1c6ff4722a1a6da994643501`
- release binary: `/private/tmp/ahc069-strict-tie-multistart-v34`
- binary SHA-256: `0f4c9cdfd7940a97309e1fd83296f7ecb8d2dcbeab9e9e5b0eb7a263f708d962`
- compile: Apple Clang 17、`-std=c++20 -O2 -DNDEBUG -Wall -Wextra -Wshadow -Wpedantic`

このfreeze後にエージェントは次のseed 0 smoke一回だけを行い、source・方針・定数を変更しなかった。その後の100 seed比較はユーザーが実行し、上記のとおりv34を棄却した。

```bash
./tester /private/tmp/ahc069-strict-tie-multistart-v34 < tools/in/0000.txt
```

実行時sourceとbinaryは上記hashで固定されている。現行`main.cpp`はこのsourceではなく、v33へ戻した後の別実験である。
