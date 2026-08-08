# 新しいCodex会話への引き継ぎプロンプト

下のコードブロックを新しい会話へ貼り、末尾の「今回お願いすること」だけ書き換える。

```text
AHC069の開発をこの会話から引き継いでください。

作業ディレクトリ:
/Users/user37/kyopro/AHC/contests/060-069/069

最初に、必ず次の順で読んでください。
1. AGENTS.md
2. memo.md
3. memo/current-state.md
4. memo/README.md

旧memoの全文はmemo/history/に8分割されています。最初から全件を読まないでください。過去の厳密な実装・数値が必要になったときだけ、memo/README.mdの分類から該当ファイルを選んでください。主要知見はmemo/key-lessons.md、公式提出はmemo/submissions.mdにあります。

重要な現在状態:
- branchは069。v33区切りコミットのsubjectは`AHC069: checkpoint v33 smooth-gated solver`、親HEADは1fb776e。
- 実測incumbentはv35。100 seed平均66,438,655.07、3000 case平均68,007,828.3783。v38は100 / 3000 caseでv35比-0.196676% / -0.119609%となり棄却した。ユーザーの明示指示でv38固有差分を全撤去してv35へ復元後、方策・候補順・tie-breakを保つcorrectness監査とmaintenance整理だけを行った。現在の未コミットmain.cppは6,677行、SHA-256 `0878466f475c52b4d6cbfa2f56aa8fa642cd6f040491c46adb49307883057652`。詳細は`memo/experiments/20260808-v35-maintenance-audit.md`。
- 旧fullのtemplate / connected / grow-and-trim / future-fit / Compact rescue / NoRegion Push-out / root rolloutを保つ。DLPは初期`E/G`の`<.55/.55-.70/.70+`で`1.30/1.25/1.00`、`E/G>=.80`だけplacementを保存p2の`5/8/24/12/8/0.50`へ替える。さらに`.70<=E/G<.80 && R<.060`だけroot未来重みを0.10にする。分類は到着前に一度だけ固定する。
- 実ターンのold Accepted connectedのうち、初期E/G<.55だけv31のdense box trimとstrict descentを追加する。denseはP>=50、`U>=10,000`かつcase最大24走査、descentは面積gateと別予算で全P最大8 step。syntheticでは追加探索0回。
- dense/descentはstrict料金増を必須にし、異周長polishはv35 square-fit guardを保つ。通常shortlistもv35の境界costと3 snapshot square future-fitで選ぶ。admission、候補集合、repacking、root発火を含めv35正本どおり。
- v34の同一最大gain次点分岐は100 seedで平均66,409,477.31、v33比-714,127 (-0.010752%)・3/95/2だったため棄却し、ユーザーがmain.cppをv33へ戻した。追加枝の即時fee gain+30,216に対して後続影響-744,343。負seedだけを除くpost-hoc gateは作らず、branch/source/診断を全撤去済み。
- v35は既存strict descentだけをP<50へ一般化する。新しい候補枝・source・expert・閾値は追加しない。各removeでfrontier最大近傍数がremove近傍数以下なら正gain不可能として全add scanを省くが、候補順とP>=50の結果は意味保存する。
- v36は非smoothかつP<50だけ空き骨格保護付きstrict descentへ開放したが、100 seedでv35比-3,079,288 (-0.046348%)・8/88/4。smooth 70 seedは完全同点でも非smoothの4敗が悪化を支配したため棄却。seed別・E/G別gateは後付けせず、v36固有BFS・枝・診断を全撤去してv35へbyte単位で復元済み。
- v37の`Rq>=1` hard gateは3000 caseでv35比`-0.004618%`。期待未来損失は`Rq`へ連続で1に不連続点がないため撤去済み。v38のfractional cell priceは3000 caseでv35比`-244,030,711 (-0.119609%)`、1433/0/1567、評価turnの57.734%で選択変更、solver CPU平均2083.124msだったため棄却・撤去済み。個別seedから係数やgateを後付けしていない。
- 通常template placementとrescue targetは、N=50の行bitset + 行方向OR sparse tableで合法anchorだけを従来順に列挙する。合法集合、tie-break、論理anchor診断は旧累積和版と同値。独立read-only監査と100 seed実測はblocking issue 0。
- incumbent実行はpahcer/json/result_20260808_030825.json。seed 0〜99を100/100 AC、合計6,643,865,507、平均66,438,655.07、WA 0。v33比+2,203,649 (+0.033179%)、19/81/0、negative gross 0。改善の93.205%はseed 42の+2,053,920に集中し、seed 42を除いても+149,729・18/81/0。同じ開発100 seedなのでfresh保証はない。
- v36実行はpahcer/json/result_20260808_105054.json。comment test、100/100 AC、合計6,640,786,219、平均66,407,862.19、WA 0。v35比-3,079,288 (-0.046348%)、8/88/4、ratio p05 1.0、worst 0.966510。seed 25/44/87/91の4敗が悪化を支配した。
- v31比`+7,256,274 (+0.109373%)`、14/71/15。v32比`+7,480,376 (+0.112755%)`、28/47/25。直前4-expert比は`+19,720,811 (+0.297810%)`、48/34/18。
- v31のexpert分布は70/13/1/11/5。expert 4以外+polishあり91 seedは+12,804,588、expert 4+polishあり5 seedは-340,051、両方なし4 seedは完全同点。expert 4の全5 seedでpolishも動いたため個別寄与は未分離。
- v31 connected polishはeligible 15,815、dense実走査2,368、dense/descent最終choice 158/504。内部solver CPUはmean1200.498ms、p951629.677ms、max1916.647ms、2秒超0/100。53種類のerror/mismatch診断は全件0、score再構成は100/100一致。
- v31差は初期E/G別に`<.55:+18.59M / .55-.625:+0.53M / .625-.70:-1.71M / .70-.80:-0.30M / >=.80:-4.64M`。同じ開発100 seedのpost-hocなのでfresh推定ではないが、v32 smooth限定の根拠。
- v32−v31の初期E/G別差は`<.55:-7.48M / .55-.625:-0.53M / .625-.70:+1.71M / .70-.80:+1.43M / >=.80:+4.64M`。非smooth合計`+7.256M`がsmooth合計`-7.480M`をほぼ相殺。静的保護は合計正だが、smooth側のdense予算・Pareto・plateau個別寄与は未分離。
- v32は残差・文献・全コード経路の独立read-only監査でblocking 0、git diff checkも0。seed 0 smokeはScore56,759,306、solver CPU1263.353ms、全identity/error 0。100 seedの内部solver CPU logは未保存。本結果追記では解答source・方針・定数を変更せず、追加実行もしていない。
- v33実測は実行前のcasewise反実仮想と完全一致。入力E/Gを再計算するとsmooth 70 seedはv31、非smooth 30 seedはv32のscoreと100/100一致し、mismatch 0。新しい`.625`境界はpost-hocなので採用していない。同じ開発100 seedの再利用でfresh保証ではない。
- 外周適応DLPのfresh validation 300 seed cache合成は旧full比+2.018205%、178/80/42、p05 .989437、worst .971320。CPU mean 1651.397→1574.050ms、p95 2146.474→2101.960ms、max 2714.630ms。
- 現在4-expertの保存cache合成は一つ前の静的DLP版比+0.087686%、今回100 seed実測は+0.083942%で方向と効果量が近い。ただし閾値選択と評価に同じvalidation cacheを見ており、今回100 seedも開発集合なのでfresh保証ではない。
- 今回のseed ratio p05/worstは.987075/.966126、paired bootstrap 95%区間[-.0770%,+.2570%]で0をまたぐ。合計改善と裾・統計的不確実性を分けて扱う。
- incumbentの正本はmemo/experiments/20260808-small-group-strict-descent-v35.md、棄却v37はmemo/experiments/20260808-expected-overlap-future-fit-v37.md、棄却・撤去済みv38はmemo/experiments/20260808-spatial-template-shadow-v38.md。v38結果JSONは100 case `result_20260808_204340.json`、3000 case `result_20260808_225925.json`。
- implementation-overview.mdはfull構造と現在の5-expert / smooth限定connected polish / small-group strict descent / LegalAnchorIndex差分、および棄却v38の履歴を説明する。行番号表は旧full Git object基準。
- v26 WideはControl比-0.1174%で棄却。Gateは+0.1515%だがbootstrap区間が0をまたぎ、solver CPU平均+24.25%で不採用。WideGateも+0.0603%、CPU平均+30.12%で不採用。
- Gateはconnected 19,442件のうち99件だけをRejectし、shape lossを21.15M改善した。全connectedのblanket Rejectではなく、ごく一部が後続compact配置を壊す仮説は残る。
- v27はEarlyMid +0.1022%、Late -0.0160%で時期仮説と部分整合したが、全CIが0をまたぎ、source限定armも全てControl未満。通常baselineを維持した。
- v28は公式generator wrapperで新規seed 100〜1299を自動生成し、train 800 / validation 200 / final 200へ分離した。全1,300 case AC、sanity stdout 100/100一致、全不変条件error 0。
- v28 finalでは反転余地があったが、固定ridgeはadmission teacher gain -521.0M、placement -928.2Mで、両CIは0をまたいだ。統計的な有害性はinconclusiveだが昇格根拠がないため不採用。池/topologyも今回の方式では採用根拠なし。
- v29はmain.cppと別のmain-optuna-v29.cppで既存8定数をOptuna探索し、ユーザーがfresh 1400 seedの全手順を実行済み。default sanityは100/100 byte一致。block validationではadmission dlp=1300が+1.9887%、placement bestが+0.1629%、root weight=100が+0.0561%だったが、p05または固定CPU最大gateで全block不採用。combinationはbaselineだけ、final holdoutは未開封で、main-optuna-final.cppはbaselineと完全一致。Optunaは一旦終了し、正本はmemo/experiments/20260805-optuna-final-v29.md。
- 最新の公式絶対scoreはcross-fitted-root-v7の3,409,424,714、1,736ms。
- 作業ツリーにはv35方策等価のmaintenance版main.cpp、v34〜v38とmaintenanceの実験記録、current-state、implementation-overview、handoff更新が未コミットである。既存変更を失わないこと。

開発上の必須事項:
- ユーザーの明示指示なしにコミットしない。
- AGENTS.mdのAHC生成AI規則を厳守する。解答プログラムの実行結果を見た後は、ユーザーの新しい明示指示なしにその結果を利用したコード・方針変更をしない。
- 実装変更時は日本語コメントを同期し、不要になったコードや診断を削除する。
- seed 0単独で判断しない。100 seed pairedは最低限の行動確認とし、高分散な性能・model判断では公式generatorから独立seedを自動生成してtrain / validation / finalを分ける。
- Pahcer wallだけでなく、対話I/O待ちを除いたコード内solver CPUを見る。
- baseline互換buildのscore/stdout一致、保存則、合法性、決定性をhard gateにする。
- 新しい実験はmemo/experiments/YYYYMMDD-short-name.mdへ独立記録し、巨大なmemo.mdへ追記し続けない。
- 複数seed比較はユーザーが行う。v37 / v38の3000 caseもユーザー実行済み。v38はfreeze後のseed 0 smoke、Pahcer Studio 100 case、`tools/in_big` 3000 caseまで完了し、棄却・撤去してv35へ復元した。現在はその方策等価maintenance版であり、大規模実行はしていない。最後にseed 0を1 caseだけsmokeした場合も、その結果からsourceを変更しない。

ユーザーの方針:
- 目標は赤パフォーマンス。
- まだ序盤なので、安全な微調整だけでなく根本的で広い案も試したい。
- 「焼きなまし」は安全側だけに進まない姿勢の比喩で、必ず焼きなましを使えという意味ではない。
- 大きな案は広く実装して原因分解用ablationと比較し、失敗したらbaselineへ戻せばよい。
- admission、placement、repackingを混同せず、幾何信号を安易にhard rejectへ昇格させない。

着手前にgit status、現在のcompile-time switch、関連するメモだけを確認し、現状と今回の差分を短く説明してください。調査・実装・実行のどこまで求められているかをユーザーの依頼から厳密に判断してください。

今回お願いすること:
[ここに次の依頼を書く]
```
