# v35 correctness audit・挙動保存整理

更新日: 2026-08-08 JST

## 結論

- 実測incumbent v35の方策、候補集合、候補順、tie-break、閾値、診断値、stdout protocolは維持した。
- 公式testerとの照合と独立read-only監査で、score・合法性に影響するCritical / High / Mediumの実バグは見つからなかった。
- 現在の`main.cpp`はv35とbyte一致ではないが、下記の保守整理だけを加えたv35方策等価版である。
- 整理前v35正本は6,654行、SHA-256 `1a5f652b17ca8de08b34920ea35f1928cfea7008dc98a4c7138b933e22d3db60`。
- 整理後sourceは6,677行、SHA-256 `0878466f475c52b4d6cbfa2f56aa8fa642cd6f040491c46adb49307883057652`。

## 監査した仕様境界

- 退去は公式testerと同じ厳密な`t < S`であり、`T == S`の組は今回到着turn中には残る。
- 複数組の再配置は、全移動元を先に消してから全移動先を置く公式の同時移動と一致する。
- `round(4*V*sqrt(P)/L)`は128 bit整数比較でhalf-up境界を確定し、移動費`max(round(V*R), 1)`もtesterと同式である。
- `max_perimeter`は履歴最大外周を保持し、最終料金総和から移動費総和を引いた値を0で下から切るscore再構成と一致する。
- 通常配置、grow-and-trim、strict descent、rescueの全候補は、盤内・芝・重複なし・面積・4連結・周長を必要箇所で再検証する。
- rescueは元盤面からmove ID、active状態、全移動元clear後の全領域、移動費、既存料金損、到着料金を採用直前に再計算する。
- rolloutは実状態のcopyだけを変更し、実`owner` / `GroupState`の変更は最終`apply_plan()`に限定される。
- N=50、P=4/150、R=0、未来0組、S=99999、最大Vでも、board mask、料金、cell-time、root marginは使用整数型の範囲内である。

## 行った整理

1. 全4近傍走査を`ORTHOGONAL_DX/DY`へ集約した。順序は従来どおり上・下・左・右で、探索順とtie-breakを変えていない。
2. 問題制約N=50を`BOARD_SIDE_LIMIT`へまとめ、row bit mask、`LegalAnchorIndex`、debug input assertionで共有した。
3. screen 2 scenario、rescue最大2候補、normal次点最大2候補、confirmationの正の偶数本という固定添字前提を`static_assert`で明示した。
4. `materialize_shape()`の冗長な面積引数を削除し、`Shape`自身の主矩形・追加矩形面積から従来と同じ容量をreserveするようにした。セル生成順は不変である。
5. 初期盤面の排他的5-expert設定を`CaseStaticPolicy`へまとめ、一度の選択でDLP倍率、placement設定、root未来重み、connected polish有効化を確定するようにした。全境界条件と値は従来と同じである。
6. root marginの古い`margin_twice`名を、実際の1000倍尺度を表す`scaled_margin`へ変更した。式、比較、丸め、stderr keyは変えていない。
7. rescueの恒等的な三項演算子を除き、不要になった引数を削除した。
8. `ShadowEvaluation`とloss診断のopportunity値はexpert倍率適用前のraw DLPであることをコメントへ明記した。履歴解析との互換性のためstderr keyと集計値は維持した。
9. 公式protocolの`N`上限、`i == turn`、P範囲をdebug assertionで明示した。

## 意図的に変更しなかったもの

- admission / placement / rescue / rootの定数と閾値
- expertの分類境界と設定値
- 候補の生成集合、列挙順、sort comparator、同点時の先着順
- normal-root windowを使用済みにする時点
- `decomp_*_opportunity`を含むstderr keyと診断計算
- A/B用の`AHC069_DISABLE_*`、`AHC069_PROTECTED_ONLY` compile-time switch
- 過去seedからの新しいgate、係数、expert

## 静的検証

- Apple ClangのC++17 / C++20、`-O2 -DNDEBUG -Wall -Wextra -Wshadow -Wpedantic -fsyntax-only`: 警告0。
- default、NoRegion Push-out無効、grow-and-trim無効、sampled DLP無効、protected-onlyの5構成: 全て警告0。
- C++20 Clang static analyzer: 指摘0。
- `git diff --check`: 指摘0。
- 未定義の旧`DX/DY`参照と旧`margin_twice`識別子: 0件。

大規模比較は行わない。最終的な解答実行を行う場合も、全source・記録を凍結した後のseed 0 one-case smokeだけとし、その結果から追加変更しない。
