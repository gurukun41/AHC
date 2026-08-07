# 新しい実験記録の運用

巨大な単一memoへ戻さないため、今後は1実験につき1ファイルを作る。

## ファイル名

`YYYYMMDD-short-name.md`

例: `20260804-column-generation-v26.md`

## 最小テンプレート

```markdown
# 実験名

## 仮説

## baselineと変更範囲

## 実行前に固定した仕様・停止条件

## source / binary / config / input / oracle

## 静的検証

## 実行結果

## paired比較と損失分解

## 採否と残る問い
```

## 規則

- 解答プログラムを初めて実行する前に、仮説、比較arm、hash、停止条件まで書く。
- 実行後は`AGENTS.md`に従い、ユーザーの新しい明示指示なしにコード・方式・定数・記録を結果へ合わせて変更しない。
- ControlとTreatmentの差を一つに絞り、既存oracleとの互換性を確認する。
- 結果後はこのファイルを正確な実験記録として残す。
- `current-state.md`には採否と現在地だけ、`key-lessons.md`には再利用できる結論だけを反映する。
- 新しい履歴ファイルを追加したら`memo/README.md`の索引も更新する。
