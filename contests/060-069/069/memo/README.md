# AHC069 メモ分類索引

このディレクトリは、旧`memo.md`の約397KB・5,357行を毎回会話へ読み込まずに済むよう、現状要約・知見・公式提出・正確な過去ログへ分けたものです。

## 新しい会話で読む順番

1. リポジトリ規則の`../AGENTS.md`
2. 軽量な入口の`../memo.md`
3. 現在だけをまとめた[current-state.md](current-state.md)
4. 必要なら[key-lessons.md](key-lessons.md)または[submissions.md](submissions.md)
5. 過去の厳密な数値・実装仕様が必要なときだけ、下記`history/`の該当ファイル

`history/`を最初から全件読まないこと。通常の引き継ぎには1〜3だけで十分です。

## 分類

| ファイル | 分類 | 主な内容 |
|---|---|---|
| [current-state.md](current-state.md) | 現在 | 現行コード、baseline方策、作業ツリー、直近v31〜v38、次の判断 |
| [../implementation-overview.md](../implementation-overview.md) | 現行full構造の設計資料 | commit `5145bc7`のfull全体フローを基準に、現行の静的5-expert、connected polish、合法anchor索引、各判断層、改善接点を同期 |
| [key-lessons.md](key-lessons.md) | 知見 | 成功・失敗から再利用できる設計原則、採用・棄却一覧 |
| [submissions.md](submissions.md) | 公式提出 | AtCoder提出の時系列、スコア、実行時間、比較上の注意 |
| [handoff-prompt.md](handoff-prompt.md) | 引き継ぎ | 新しいCodex会話へ貼るためのプロンプト |
| [experiments/README.md](experiments/README.md) | 今後の記録 | 新しい実験を巨大な単一memoへ戻さないための記録様式 |

## 新規実験

| 実験記録 | 状態 | 主な比較 |
|---|---|---|
| [20260803-wide-spatial-v25.md](experiments/20260803-wide-spatial-v25.md) | 棄却・コード撤去済み | 空間DLP Control / SameFee / Full |
| [20260803-fallback-phase-v26.md](experiments/20260803-fallback-phase-v26.md) | 棄却・無フラグbaseline維持 | 通常template `+4/+8` × connected risk比較 on/off |
| [20260804-fallback-risk-attribution-v27.md](experiments/20260804-fallback-risk-attribution-v27.md) | 完了・mixed/inconclusive・通常baseline維持 | connected RejectのEarlyMid/Late・source別因果分解 |
| [20260804-offline-value-model-v28.md](experiments/20260804-offline-value-model-v28.md) | 完了・線形model不採用・通常baseline維持 | 新規1200 seedの実未来教師によるadmission / placement価値モデル |
| [20260805-optuna-final-v29.md](experiments/20260805-optuna-final-v29.md) | 完了・全block不採用・baseline維持 | fresh 1400 seedで既存8定数をadmission / placement / root別に探索・独立validation |
| [20260806-direct-admission-comparison.md](experiments/20260806-direct-admission-comparison.md) | 完了・不採用 | sampled未来に対するaccept/reject二容量の未来獲得額直接比較 |
| [20260807-protected-all-candidates.md](experiments/20260807-protected-all-candidates.md) | 完了・直前誤実装より改善 | 従来候補を保護した全テンプレート走査と同一最短周長内future-fit |
| [20260807-expected-future-fee-all-candidates.md](experiments/20260807-expected-future-fee-all-candidates.md) | 完了・不採用 | 全候補を共通12標本の独立未来料金で比較 |
| [20260807-continuous-acceptance-rate.md](experiments/20260807-continuous-acceptance-rate.md) | 完了・不採用・コード撤去済み | `log(duration/theta)`上の連続受入率で未来需要量を補正 |
| [20260807-lightweight-hybrid.md](experiments/20260807-lightweight-hybrid.md) | 完了・開発100 seedで最高・時間リスク未解消・source凍結 | protected軽量版 + 外周適応DLP + grow-and-trim + criticality champion |
| [20260807-full-static-dlp-hybrid.md](experiments/20260807-full-static-dlp-hybrid.md) | 完了・開発100 seedで新最高・source凍結・時間リスク残存 | 旧fullのrepacking / root + 独立validation済み静的外周適応DLP |
| [20260807-static-portfolio-anchor-index.md](experiments/20260807-static-portfolio-anchor-index.md) | 100 seed完了・新最高・source凍結・bootstrap区間は0をまたぐ | 4領域DLP/placement expert + 意味保存bitset合法anchor列挙 |
| [20260807-connected-polish-root-v31.md](experiments/20260807-connected-polish-root-v31.md) | 100 seed完了・新最高・source凍結・bootstrap区間は0をまたぐ | old Accepted connected限定のdense/swap周長改善 + `.70<=E/G<.80 && R<.060`限定root expert |
| [20260807-connected-polish-plateau-v32.md](experiments/20260807-connected-polish-plateau-v32.md) | 100 seed完了・raw合計2位・v31比`-0.003378%`・source凍結 | smooth限定polish + 高価値dense予算 + locked zero-gain plateau escape + 将来形状Pareto guard |
| [20260808-static-polish-gate-v33.md](experiments/20260808-static-polish-gate-v33.md) | 100 seed完了・新最高・反実仮想と完全一致・source凍結 | 非smooth停止を維持し、smoothだけv31のdense `U>=10,000` + strict descent + scalar future-fitへ復元 |
| [20260808-strict-tie-multistart-v34.md](experiments/20260808-strict-tie-multistart-v34.md) | 100 seed完了・v33比`-0.010752%`で棄却・コード撤去済み | v33 strict descentを保持し、同一最大gainの次点だけを固定8/8予算でlimited-discrepancy探索 |
| [20260808-small-group-strict-descent-v35.md](experiments/20260808-small-group-strict-descent-v35.md) | 100 seed完了・現行incumbent・v33比`+0.033179%`・19/81/0・source完全復元 | denseの`P>=50`を維持し、既存strict descentだけをsmooth小規模connectedへ一般化 |
| [20260808-free-space-backbone-small-descent-v36.md](experiments/20260808-free-space-backbone-small-descent-v36.md) | 100 seed完了・v35比`-0.046348%`・8/88/4・棄却・コード撤去済み | v35 smooth経路を完全保護したが、非smoothの空き骨格保護descentで4件の後続tailが小改善を上回った |
| [20260808-expected-overlap-future-fit-v37.md](experiments/20260808-expected-overlap-future-fit-v37.md) | 3000 caseでv35比`-0.004618%`・棄却・コード撤去済み | `残り未来組数 × 今回の退去までの到着確率 >= 1`だけ通常placementのfuture-fitを使うhard gate |
| [20260808-spatial-template-shadow-v38.md](experiments/20260808-spatial-template-shadow-v38.md) | 100 / 3000 case完了・v35比`-0.196676% / -0.119609%`・棄却・コード撤去済み | 公式P分布と最良周長の連結template列からセル別configuration shadowを作り、同一料金候補だけを順位付け |
| [20260808-v35-maintenance-audit.md](experiments/20260808-v35-maintenance-audit.md) | v35方策維持・correctness監査完了・保守整理 | tester照合、数値境界、状態遷移を監査し、候補順とログ互換を保つ共通化・命名整理だけを実施 |

## 旧memoの正確な分割履歴

旧`memo.md`は次の8ファイルへ、内容を変更せず行境界で分割しました。連結後のSHA-256は分割前と同じ`e08ec31b37625a25524bfefd5702ac16b083f973b6437be564dd91c5d21dcf74`です。

| 元の行 | 履歴ファイル | 主な系列 |
|---:|---|---|
| 1–902 | [01-early-baseline-and-admission.md](history/01-early-baseline-and-admission.md) | init、future、component、admission-v1〜v4、初期3提出 |
| 903–1333 | [02-shadow-and-placement.md](history/02-shadow-and-placement.md) | shadow-price-v1、placement-fit-v1、4回目・黄パフォ提出 |
| 1334–2647 | [03-relocation-and-causal-rollout.md](history/03-relocation-and-causal-rollout.md) | relocation LNS、proactive cleanup v1〜v14、rollout幅・深さ・予算 |
| 2648–3211 | [04-rescue-and-root-rollout.md](history/04-rescue-and-root-rollout.md) | compact rescue、expanded/cross-fitted root v1〜v7 |
| 3212–3885 | [05-loss-diagnostics-and-shadow-experiments.md](history/05-loss-diagnostics-and-shadow-experiments.md) | 損失分解、component shadow、NoRegion Push-out、期限レイヤー、grow-and-trim、sampled DLP |
| 3886–4145 | [06-pushout-helper.md](history/06-pushout-helper.md) | one-helper Push-out v15/v16と廃止 |
| 4146–4976 | [07-predictive-dp-and-static-review.md](history/07-predictive-dp-and-static-review.md) | predictive DP v17〜v19、静的レビュー4案、v20〜v23 |
| 4977–5357 | [08-spatial-placement.md](history/08-spatial-placement.md) | v24隙間侵入・障壁回避、v25広域配置＋cell×time DLPの実行前仕様 |

v25の実行後結果は、旧memo凍結後に分析したため履歴08には含まれません。[v25結果記録](experiments/20260803-wide-spatial-v25.md)を参照してください。

## テーマ別の読み先

- 受入判断: 履歴01、02、05
- 通常配置: 履歴01、02、05、07、08
- 再配置・root action: 履歴03、04、05、06、07
- 予測、rollout、DP: 履歴03、04、05、07、08
- 公式提出: [submissions.md](submissions.md)。根拠の原文は履歴01〜04
- 最新の作業再開: [current-state.md](current-state.md)と[handoff-prompt.md](handoff-prompt.md)

## 今後の更新方針

- 1実験を`experiments/YYYYMMDD-short-name.md`の1ファイルにする。
- `current-state.md`は現状だけに保ち、古い現状説明を蓄積しない。
- 再利用できる結論だけ`key-lessons.md`へ短く反映する。
- 公式提出時だけ`submissions.md`を更新する。
- ルート`memo.md`を長大な時系列ログへ戻さない。
- 実験プログラムを一度実行した後の編集制限は、必ず`AGENTS.md`に従う。
