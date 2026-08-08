# AHC069 作業メモ（軽量入口）

旧`memo.md`は5,357行・約397KBまで増えたため、2026-08-03に分類・分割した。このファイルには入口だけを置き、過去ログを再び追記し続けない。

## 新しい会話で最初に読むもの

1. `AGENTS.md` — AHC生成AI規則とコード保守規則
2. [memo/current-state.md](memo/current-state.md) — 現行コード、直近実験、未コミット状態
3. [memo/README.md](memo/README.md) — メモの分類と必要な履歴の選び方

通常はここまででよい。過去の主要結論は[memo/key-lessons.md](memo/key-lessons.md)、公式提出は[memo/submissions.md](memo/submissions.md)にある。

full solverの処理全体と現行の静的5-expert / smooth connected polish / small-group strict descent / `LegalAnchorIndex`差分は[implementation-overview.md](implementation-overview.md)に分離している。棄却・撤去済みv38のcompact-template spatial shadowも同資料と[実験記録](memo/experiments/20260808-spatial-template-shadow-v38.md)に履歴として残す。v35方策を維持したcorrectness監査・保守整理は[maintenance記録](memo/experiments/20260808-v35-maintenance-audit.md)、現行sourceと実行状態は[memo/current-state.md](memo/current-state.md)を正本にする。

## 全文履歴

旧memoの全文は[memo/history/](memo/history/)に8分割して保存した。連結時のSHA-256は分割前と一致しており、情報は削除していない。

使用量を抑えるため、`memo/history/`を一括で読まないこと。厳密な過去仕様・数値が必要なときだけ、[分類索引](memo/README.md)から該当ファイルを選ぶ。

## 今後の記録

- 新しい実験は1件1ファイルで`memo/experiments/`へ記録する。
- 記録様式は[memo/experiments/README.md](memo/experiments/README.md)を使う。
- `memo/current-state.md`は「現在」だけに更新する。
- 新しい会話への貼り付け文は[memo/handoff-prompt.md](memo/handoff-prompt.md)を使う。
- ユーザーの明示指示なしにコミットしない。
- 解答実行後の変更制限は必ず`AGENTS.md`に従う。
