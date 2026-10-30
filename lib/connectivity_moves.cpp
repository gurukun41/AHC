#include <bits/stdc++.h>
using namespace std;

// selected の頂点だけからなる部分グラフの関節点。
vector<char> articulation_points(const vector<vector<int>>& graph, const vector<char>& selected) {
    int n = (int)graph.size(), timer = 0;
    vector<int> discovery(n, -1), low(n), parent(n, -1);
    vector<char> cut(n, false);
    auto dfs = [&](auto&& self, int v) -> void {
        discovery[v] = low[v] = timer++;
        int children = 0;
        for (int u : graph[v]) {
            if (!selected[u]) continue;
            if (discovery[u] == -1) {
                parent[u] = v;
                ++children;
                self(self, u);
                low[v] = min(low[v], low[u]);
                if (parent[v] != -1 && low[u] >= discovery[v]) cut[v] = true;
            } else if (u != parent[v]) low[v] = min(low[v], discovery[u]);
        }
        if (parent[v] == -1 && children > 1) cut[v] = true;
    };
    for (int v = 0; v < n; v++) if (selected[v] && discovery[v] == -1) dfs(dfs, v);
    return cut;
}

// 初めの領域は非空かつ連結。cut はその領域で計算したもの。
bool can_swap_safely(const vector<vector<int>>& graph, const vector<char>& selected,
                     const vector<char>& cut, int size, int removed, int added) {
    if (!selected[removed] || selected[added]) return false;
    if (size == 1) return true; // 入れ替え後も 1 頂点
    if (cut[removed]) return false;
    for (int u : graph[added]) if (u != removed && selected[u]) return true;
    return false;
}

int main() {
    vector<vector<int>> graph = {{1, 3}, {0, 2, 4}, {1, 5}, {0, 4}, {1, 3, 5}, {2, 4}};
    vector<char> selected = {true, true, true, false, false, false}; // 2×3 盤面の上の行
    auto cut = articulation_points(graph, selected);
    assert(cut[1] && !cut[0] && !cut[2]);
    assert(can_swap_safely(graph, selected, cut, 3, 0, 4));
    assert(!can_swap_safely(graph, selected, cut, 3, 0, 3));
}

/*
連結性を保ちながら領域のマスを入れ替える
- 関節点を取り除くと領域が分断される。除去後に残る領域へ追加マスを接続する。
- added が removed にしか隣接していない場合は接続できないので除外する。
- この条件は十分条件。関節点を消してから追加マスで再接続する有効な交換も除外する。
  全候補を正確に判定したければ、仮に交換した領域で BFS などを行う。
- 関節点の計算は O(V+E)。領域を変更したら再計算する。cut と size は現在の領域に合わせる。
- 例は単純な無向グラフ。大きな領域では再帰 DFS のスタック量にも注意する。
*/
