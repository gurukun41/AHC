#include <bits/stdc++.h>
using namespace std;

// 上下左右で隣接する 2 マスの値が同じなら 1 点。
struct State {
    int H, W;
    vector<int> board; // (r, c) は r * W + c
    int score;
};

// 全体から計算する。差分更新の確認にも使う。
int calc_score(const State& s) {
    int score = 0;
    for (int r = 0; r < s.H; r++) {
        for (int c = 0; c < s.W; c++) {
            int p = r * s.W + c;
            if (r + 1 < s.H) score += (s.board[p] == s.board[p + s.W]);
            if (c + 1 < s.W) score += (s.board[p] == s.board[p + 1]);
        }
    }
    return score;
}

// マス p に接する辺を追加する。辺は小さい id を先にして表す。
void add_edges(const State& s, int p, vector<pair<int, int>>& edges) {
    int r = p / s.W, c = p % s.W;
    int dr[] = {-1, 1, 0, 0};
    int dc[] = {0, 0, -1, 1};
    for (int d = 0; d < 4; d++) {
        int nr = r + dr[d], nc = c + dc[d];
        if (nr < 0 || nr >= s.H || nc < 0 || nc >= s.W) continue;
        int q = nr * s.W + nc;
        edges.emplace_back(min(p, q), max(p, q));
    }
}

int edge_score(const State& s, const vector<pair<int, int>>& edges) {
    int score = 0;
    for (auto [p, q] : edges) score += (s.board[p] == s.board[q]);
    return score;
}

// 2 マスを交換し、影響を受ける辺だけでスコアを更新する。
void swap_cells(State& s, int p, int q) {
    vector<pair<int, int>> edges;
    add_edges(s, p, edges);
    add_edges(s, q, edges);
    // p と q が隣接する場合、共通の辺が 2 回入るので 1 回にする。
    sort(edges.begin(), edges.end());
    edges.erase(unique(edges.begin(), edges.end()), edges.end());

    s.score -= edge_score(s, edges); // 変更前の寄与を引く
    swap(s.board[p], s.board[q]);
    s.score += edge_score(s, edges); // 変更後の寄与を足す
}

int main() {
    State s = {2, 3, {1, 1, 2, 2, 1, 2}, 0};
    s.score = calc_score(s);

    swap_cells(s, 1, 2); // 同じ行で隣接するマスを交換
    assert(s.score == calc_score(s)); // 全体からの再計算と照合
    cout << s.score << "\n";
}

/*
差分計算の考え方

1. 操作によって変わる評価項を列挙する。
2. それらの変更前の寄与を引き、状態を変更し、変更後の寄与を足す。
3. 開発中は全体からの再計算と比べ、更新漏れや二重計算を確認する。

この例では各マスに接する辺は最大 4 本なので、交換時に調べる辺は最大 8 本。
問題によっては影響範囲が広がるため、操作と評価式に合わせて列挙する。
操作を取り消す場合は、盤面だけでなく更新した score も元に戻す。
*/
