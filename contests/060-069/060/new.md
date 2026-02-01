AHC改善仕様書：訪問順序（Waypoints）最適化モデル

1. 概要と目的

現状の問題点

現在の「1ステップごとの移動（Action）をランダムに変更する方針」では、パスの途中で変更を加えると、それ以降のパスが全て無効（ランダム再生成）になってしまう「破壊的変更」が発生している。これにより、探索が収束せず、ランダムウォークに近い挙動となっている。

改善方針

「どの施設をどの順番で巡るか（Waypoints）」を最適化する方針へ移行する。
経路は常に「施設間の最短経路」で自動補完することで、順序を入れ替えてもパス全体が破綻せず、効率的な探索（TSPライクな最適化）を可能にする。

2. データ構造の再定義

探索空間（遺伝子）を具体的な移動経路ではなく、抽象的な「タスクリスト」として定義する。

struct Solution {
    // 訪問するターゲットの順序リスト
    // 値は頂点ID (0 ~ K-1: ショップ, K ~ N-1: 木)
    // 例: {TreeA, TreeB, Shop0, TreeC, TreeA, Shop1 ...}
    vector<int> waypoints;

    // 各木を「赤(Strawberry)」にするかどうかの計画
    // サイズN (0~K-1は未使用、K~N-1が有効)
    // false: バニラのまま維持, true: 訪問時に赤に変更
    vector<bool> tree_is_red;

    // 現在の評価値（キャッシュ用）
    ll score;
};


3. 事前計算（Pre-computation）

評価関数内で毎回BFSを行うと計算コストが高すぎるため、移動経路を $O(1)$ で取得できるテーブルを作成する。

dist[N][N]: 全点対最短距離。

next_node[N][N]: 経路復元用テーブル。

意味: 頂点 $u$ から $v$ へ最短経路で向かう際、次に踏むべき隣接頂点。

作成方法: ワーシャル・フロイド法の副産物、または全頂点始点のBFSで構築する。

4. 評価関数（デコーダ）の仕様

Solution (設計図) から実際の actions (行動) を生成し、スコアを計算するプロセス。

処理フロー

初期化:

現在地 curr = 0 (初期位置)。

手持ちコーン cone = ""。

現在の木の味状態 current_tree_flavor[N] (全てFalse)。

経過ステップ steps = 0。

ショップ在庫 shops[K] (set型)。

ループ実行: waypoints の各 target について順次処理。

移動フェーズ (Step-by-Step):

curr から target まで next_node[curr][target] を辿って1歩ずつ移動。

1移動ごとに steps++。

終了条件: 最大ターン $T$ を超えたら、即座に終了してスコア計算へ。

(注): 最短経路を通るため、「直前の頂点に戻らない」制約は自然に満たされる（閉路がない限り）。

アクション実行フェーズ (Target到達時):

ターゲットが木 ($target \ge K$) の場合:

味変 (Action 2):

条件: tree_is_red[target] == true (計画が赤) かつ current_tree_flavor[target] == false (現場が白)。

処理: Action 2 を発行し、steps++。current_tree_flavor[target] = true に更新。

収穫:

現在の味 (current_tree_flavor 参照) に応じた文字 ('W' or 'R') を cone に追加。

高速化制約: cone の長さが閾値（例: 10文字）を超えている場合、追加しない（またはペナルティとみなす）。

ターゲットがショップ ($target < K$) の場合:

納品: cone をそのショップの shops[target] に追加。

リセット: cone を空文字列 "" に戻す。

スコア算出:

全ショップの shops (set) のサイズ合計を返す。

5. 近傍操作（Neighborhood Operations）

状態 waypoints に対して、巡回セールスマン問題 (TSP) のような近傍操作を適用する。

基本操作

Insert (挿入)

ランダムな「木」を選び、waypoints のランダムな位置に挿入する。

Delete (削除)

waypoints からランダムに1つの要素を削除する。

Swap (交換)

waypoints 内の2つの要素の位置を入れ替える。

Change Shop (納品先変更)

waypoints 内の「ショップ」である要素を、別のショップ（現在地から近い場所など）に変更する。

特殊操作

Flavor Flip (味変更)

低確率 (例: 5-10%) で実行。

tree_is_red のランダムな1ビットを反転させる。

6. 実装ロードマップ

Step 1: 経路復元の実装

calc_all_pairs_shortest_path を拡張し、next_node[u][v] テーブルを作成できるようにする。

// next_node[u][v]: uからvへ向かうための次の頂点
// BFSの際、prev[next] = current を記録する要領で逆算、またはWarshall-Floydで構築


Step 2: 初期解生成の改良

ランダムではなく、貪欲法で初期 Solution を作る。

戦略例: 「現在地から最も近く」かつ「現在のコーンの状態にとって有益な（まだ持っていない文字が取れる）」木を選んで waypoints に追加していく。

Step 3: 山登り法のメインループ書き換え

evaluate_actions を上記「4. 評価関数」のロジックに完全書き換え。

近傍操作を Action 単位ではなく Solution 単位のものに差し替える。

Step 4: パラメータ調整

waypoints の長さの上限管理（長すぎると後半が評価されない）。

文字列長の制限値（多様性を生むには短く回数をこなす方が有利）。