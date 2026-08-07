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
- 現在の未コミットmain.cppは6,534行のfull + 静的5-expert + smooth限定v31 connected polish + LegalAnchorIndex。freeze SHA-256は3709a9de4a71111ff1a116ecfde7c4fd349a459b1bb179275d1e5ccf22d6461d。v33のseed 0 smokeとユーザー100 seed実行後も不変。
- 旧fullのtemplate / connected / grow-and-trim / future-fit / Compact rescue / NoRegion Push-out / root rolloutを保つ。DLPは初期`E/G`の`<.55/.55-.70/.70+`で`1.30/1.25/1.00`、`E/G>=.80`だけplacementを保存p2の`5/8/24/12/8/0.50`へ替える。さらに`.70<=E/G<.80 && R<.060`だけroot未来重みを0.10にする。分類は到着前に一度だけ固定する。
- 実ターンのold Accepted connectedかつP>=50のうち、初期E/G<.55だけv31のdense box trimとstrict descentを追加する。denseは`U>=10,000`かつcase最大24走査、descentは別予算で最大8 step。syntheticでは追加探索0回。
- dense/descentはstrict料金増を必須にし、未来3 snapshotのscalar future-fitがold以上の最短tierだけを採用する。old/root rollbackと旧rescue/root発火を保護する。v32のzero plateau、24要素Pareto、component capacityはsource・診断を含め撤去済み。
- 通常template placementとrescue targetは、N=50の行bitset + 行方向OR sparse tableで合法anchorだけを従来順に列挙する。合法集合、tie-break、論理anchor診断は旧累積和版と同値。独立read-only監査と100 seed実測はblocking issue 0。
- 最新実行はpahcer/json/result_20260808_012614.json。seed 0〜99を100/100 AC、合計6,641,661,858、平均66,416,618.58、WA 0で、保存済み141 run中のraw合計1位。
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
- 最新の100 seed実行・設計・実装正本はmemo/experiments/20260808-static-polish-gate-v33.md。直前v32はmemo/experiments/20260807-connected-polish-plateau-v32.md、v31はmemo/experiments/20260807-connected-polish-root-v31.md。
- implementation-overview.mdはfull構造と現在の5-expert / smooth限定v31 connected polish / LegalAnchorIndex差分を説明する。行番号表は旧full Git object基準。
- v26 WideはControl比-0.1174%で棄却。Gateは+0.1515%だがbootstrap区間が0をまたぎ、solver CPU平均+24.25%で不採用。WideGateも+0.0603%、CPU平均+30.12%で不採用。
- Gateはconnected 19,442件のうち99件だけをRejectし、shape lossを21.15M改善した。全connectedのblanket Rejectではなく、ごく一部が後続compact配置を壊す仮説は残る。
- v27はEarlyMid +0.1022%、Late -0.0160%で時期仮説と部分整合したが、全CIが0をまたぎ、source限定armも全てControl未満。通常baselineを維持した。
- v28は公式generator wrapperで新規seed 100〜1299を自動生成し、train 800 / validation 200 / final 200へ分離した。全1,300 case AC、sanity stdout 100/100一致、全不変条件error 0。
- v28 finalでは反転余地があったが、固定ridgeはadmission teacher gain -521.0M、placement -928.2Mで、両CIは0をまたいだ。統計的な有害性はinconclusiveだが昇格根拠がないため不採用。池/topologyも今回の方式では採用根拠なし。
- v29はmain.cppと別のmain-optuna-v29.cppで既存8定数をOptuna探索し、ユーザーがfresh 1400 seedの全手順を実行済み。default sanityは100/100 byte一致。block validationではadmission dlp=1300が+1.9887%、placement bestが+0.1629%、root weight=100が+0.0561%だったが、p05または固定CPU最大gateで全block不採用。combinationはbaselineだけ、final holdoutは未開封で、main-optuna-final.cppはbaselineと完全一致。Optunaは一旦終了し、正本はmemo/experiments/20260805-optuna-final-v29.md。
- 最新の公式絶対scoreはcross-fitted-root-v7の3,409,424,714、1,736ms。
- 作業ツリーには現行main.cpp、メモ再編、implementation-overview.md、v29の別solver・driver・実行artifactなど多くの未コミット変更がある。既存変更を失わないこと。

開発上の必須事項:
- ユーザーの明示指示なしにコミットしない。
- AGENTS.mdのAHC生成AI規則を厳守する。解答プログラムの実行結果を見た後は、ユーザーの新しい明示指示なしにその結果を利用したコード・方針変更をしない。
- 実装変更時は日本語コメントを同期し、不要になったコードや診断を削除する。
- seed 0単独で判断しない。100 seed pairedは最低限の行動確認とし、高分散な性能・model判断では公式generatorから独立seedを自動生成してtrain / validation / finalを分ける。
- Pahcer wallだけでなく、対話I/O待ちを除いたコード内solver CPUを見る。
- baseline互換buildのscore/stdout一致、保存則、合法性、決定性をhard gateにする。
- 新しい実験はmemo/experiments/YYYYMMDD-short-name.mdへ独立記録し、巨大なmemo.mdへ追記し続けない。
- 複数seed比較、入力corpus生成、Optuna探索などの大規模実行はユーザーが行う。v31・v32・v33は単一smokeと100 seedを実行済み。v33結果確認では記録だけを同期し、解答・方針・定数を変更せず追加実行もしていない。次の明示指示までこの状態を保つ。

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
