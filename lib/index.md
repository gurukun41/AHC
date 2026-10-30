# AHC の実装メモ

各ファイルは必要な部分をコピーして使う。`main()` は使用例。
考え方・前提・失敗しやすい点は、コード内のコメントを参照。

## 探索を組み立てる

- [beam_search.cpp](beam_search.cpp)：ビーム探索の全体例。重複除去・候補選択・経路復元。
- [path_restore.cpp](path_restore.cpp)：親番号と最後の操作から操作列を復元する。
- [candidate_selection.cpp](candidate_selection.cpp)：軽い候補だけを選別し、採用候補の状態だけコピーする。
- [top_n.cpp](top_n.cpp)：配列から上位 N 個を選ぶ。nth_element / partial_sort。
- [bounded_top_k.cpp](bounded_top_k.cpp)：候補を生成しながら上位 K 個だけ保持する。
- [beam_hash.cpp](beam_hash.cpp)：複数項目を含む状態のハッシュと差分更新。
- [two_stage_selection.cpp](two_stage_selection.cpp)：軽い評価で絞り、残した候補だけ重い評価をする。
- [beam_diversity.cpp](beam_diversity.cpp)：スコアと進捗など、複数の基準でビーム枠を分ける。
- [receding_horizon.cpp](receding_horizon.cpp)：先読みした経路の最初の 1 手だけ実行して再探索する。
- [recent_states.cpp](recent_states.cpp)：直近の状態の出現回数を管理し、再訪を避ける判断に使う。
- [search_portfolio.cpp](search_portfolio.cpp)：残り時間を複数の探索設定へ配り、共通の最良解を残す。
- [past_modify_greedy.cpp](past_modify_greedy.cpp)：過去の操作列へ新しい操作を挿入し、後ろの合法性と評価を再計算する。

## 状態を変更・評価する

- [apply_undo.cpp](apply_undo.cpp)：操作の適用と取り消し。変更前の値を保存する。
- [score_diff.cpp](score_diff.cpp)：影響する辺だけでスコアを更新し、重複計算を避ける。
- [permutation_moves.cpp](permutation_moves.cpp)：順列の交換・挿入・区間反転と取り消し。
- [two_opt.cpp](two_opt.cpp)：対称な巡回路の 2-opt。境界の辺だけで反転差分を計算。
- [anneal_step.cpp](anneal_step.cpp)：焼きなましの採否・温度・現在解と最良解の保存。
- [restart_search.cpp](restart_search.cpp)：複数の初期解から探索し、停滞した試行を打ち切る。
- [branch_bound.cpp](branch_bound.cpp)：楽観的な上界による枝刈りと、探索予算での終了。
- [prefix_simulation.cpp](prefix_simulation.cpp)：途中状態を保存し、変更箇所以降だけシミュレーションし直す。
- [evaluation_cache.cpp](evaluation_cache.cpp)：条件をキーに重い計算結果を再利用し、保存量も制限する。
- [shared_rollout.cpp](shared_rollout.cpp)：同じ未来サンプルを使って候補を比較する。
- [candidate_repair.cpp](candidate_repair.cpp)：壊れた候補を修復してから評価する。順列の例。
- [connectivity_moves.cpp](connectivity_moves.cpp)：関節点を使って、連結性を保つ領域の入れ替えを選ぶ。
- [validate_plan.cpp](validate_plan.cpp)：操作の合法性を確認し、完成した操作列を初期状態から再生検証する。

## 探索を支える

- [timer.cpp](timer.cpp)：経過時間・残り時間・進捗と打ち切り。
- [random_choice.cpp](random_choice.cpp)：範囲、異なる 2 点、シャッフル、重み付き選択。
- [flatten_grid.cpp](flatten_grid.cpp)：2 次元・3 次元の一次元化と、安全な隣接マスの列挙。
- [bit_state.cpp](bit_state.cpp)：64 bit や bitset で訪問済み・所持品の集合を扱う。
- [random_set.cpp](random_set.cpp)：追加・削除しながら集合からランダムに選ぶ。
- [visit_stamp.cpp](visit_stamp.cpp)：繰り返す探索で訪問済み配列の初期化を省く。
- [precompute_transition.cpp](precompute_transition.cpp)：固定盤面で位置・向き・操作の遷移先を事前計算する。
- [torus_grid.cpp](torus_grid.cpp)：端がつながる座標の正規化、距離、符号付きの最短差。

## 既存のテンプレート

- [main.cpp](main.cpp)：共通の型、時間計測、乱数。
- [aneal.cpp](aneal.cpp)：時間制の焼きなましテンプレート。

BFS・Dijkstra・累積和・Union-Find などの基本部品は [ABC/library](../../ABC/library) を参照。
各例の評価式・制約は仮の問題用。実際の状態・操作・採点に合わせて変更する。
