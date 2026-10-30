#include <bits/stdc++.h>
using namespace std;
using ull = uint64_t;

struct State { int position; ull visited; long long score; };
struct Node { State state; int first; }; // first: 最初に移動する頂点

// 未訪問頂点を移動し、点を得る例。先読みの経路のうち最初の移動先だけ返す。
optional<int> choose_next(const State& start, const vector<vector<int>>& graph,
                          const vector<int>& reward, int lookahead, size_t width) {
    assert(lookahead >= 1 && width >= 1 && graph.size() <= 64);
    vector<Node> beam = {{start, -1}};
    optional<int> best_first;
    long long best_score = LLONG_MIN;
    for (int depth = 0; depth < lookahead; depth++) {
        vector<Node> next;
        for (const auto& node : beam) for (int to : graph[node.state.position]) {
            if ((node.state.visited >> to) & 1) continue;
            State s = {to, node.state.visited | (ull(1) << to), node.state.score + reward[to]};
            int first = depth == 0 ? to : node.first;
            next.push_back({s, first});
            if (!best_first || s.score > best_score) {
                best_score = s.score;
                best_first = first;
            }
        }
        if (next.empty()) break;
        if (next.size() > width) {
            nth_element(next.begin(), next.begin() + width, next.end(),
                        [](const Node& a, const Node& b) { return a.state.score > b.state.score; });
            next.resize(width);
        }
        beam = std::move(next);
    }
    return best_first; // 合法手がなければ nullopt
}

int main() {
    vector<vector<int>> graph = {{1, 2}, {}, {3}, {}};
    vector<int> reward = {0, 5, 1, 10};
    State current{0, 1, 0};
    int remaining = 2;
    while (remaining > 0) {
        auto to = choose_next(current, graph, reward, min(2, remaining), 10);
        if (!to) break;
        // 先読みで作った終端状態ではなく、実際の状態を 1 手だけ進める。
        current = {*to, current.visited | (ull(1) << *to), current.score + reward[*to]};
        cout << *to << " "; // 2 3
        --remaining;
    }
    cout << "\n";
}

/*
先読み -> 最初の 1 手だけ実行 -> 再探索
- 先読み中は実際の状態を変更しない。オンライン問題では入力された観測で状態を更新する。
- 親の first を子へ引き継げば、最初の 1 手を選ぶだけなら全経路の履歴は不要。
- 例は 1〜lookahead 手先の得点で比較する。固定手数や終端評価がある場合は比較条件を変える。
- 次のターンでも探索し直すため、その分の時間を残す。残り手数より先を読まない。
- 例は重複除去なし。頂点番号・シフト範囲・reward の長さは呼び出し側で保証する。
*/
