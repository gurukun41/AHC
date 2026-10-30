#include <bits/stdc++.h>
using namespace std;
using ull = uint64_t;

// 例: 重み付きグラフを移動し、未訪問の頂点の点を得る。最大 depth 手。
struct Key {
    ull visited; // 頂点数は 64 以下
    int position;
    bool operator==(const Key& other) const {
        return visited == other.visited && position == other.position;
    }
};
struct KeyHash {
    size_t operator()(const Key& k) const {
        return hash<ull>{}(k.visited) ^ (hash<int>{}(k.position) * 0x9e3779b9ULL);
    }
};
struct State { Key key; long long score; int node; };
struct Candidate { Key key; long long score; int parent; int action; };
struct Node { int parent; int action; };

vector<int> beam_search(const vector<vector<int>>& graph, const vector<int>& reward,
                        int start, int depth, size_t width) {
    assert(!graph.empty() && graph.size() <= 64 && width > 0);
    vector<Node> nodes = {{-1, start}};
    vector<State> beam = {{{ull(1) << start, start}, reward[start], 0}};
    long long best_score = reward[start];
    int best_node = 0;

    for (int turn = 0; turn < depth; turn++) {
        vector<Candidate> candidates;
        unordered_map<Key, size_t, KeyHash> index; // このターンだけの重複除去
        for (const State& s : beam) {
            for (int to : graph[s.key.position]) {
                if ((s.key.visited >> to) & 1) continue;
                Candidate c = {{s.key.visited | (ull(1) << to), to},
                               s.score + reward[to], s.node, to};
                auto [it, inserted] = index.emplace(c.key, candidates.size());
                if (inserted) candidates.push_back(c);
                else if (c.score > candidates[it->second].score) candidates[it->second] = c;
            }
        }
        if (candidates.empty()) break; // 行き止まりなら、それまでの最良解を返す
        auto better = [](const Candidate& a, const Candidate& b) { return a.score > b.score; };
        if (candidates.size() > width) {
            nth_element(candidates.begin(), candidates.begin() + width, candidates.end(), better);
            candidates.resize(width);
        }

        beam.clear();
        for (const Candidate& c : candidates) {
            int node = (int)nodes.size();
            nodes.push_back({c.parent, c.action}); // 絞り込んだ候補の履歴だけ記録
            beam.push_back({c.key, c.score, node});
            if (c.score > best_score) {
                best_score = c.score;
                best_node = node;
            }
        }
    }
    vector<int> path;
    for (int id = best_node; id != -1; id = nodes[id].parent) path.push_back(nodes[id].action);
    reverse(path.begin(), path.end());
    return path; // 開始頂点を含む
}

int main() {
    vector<vector<int>> graph = {{1, 2}, {0, 3}, {0, 3}, {1, 2}};
    vector<int> reward = {1, 2, 5, 4};
    auto path = beam_search(graph, reward, 0, 3, 10);
    for (int p : path) cout << p << " ";
    cout << "\n";
}

/*
ビームサーチの骨組み
- 候補生成 -> 同じ深さで重複除去 -> 上位を選択 -> 履歴保存、の順で進める。
- Key は将来を決める情報。別問題では位置・資源・履歴など必要な項目に置き換える。
- unordered_map は hash が同じでも Key の == で区別する。hash だけをキーにしない例。
- 今回は最大 depth 手なので途中の最良解も保存する。丁度 depth 手なら終端の比較を変える。
- 候補を並べる評価値と実際の得点が異なる問題では、それぞれ別の変数で管理する。
- 頂点・start は範囲内、reward は頂点数分あること。時間制ならループに終了判定を加える。
- 重い盤面を候補ごとに持つ場合は candidate_selection.cpp の方法を使える。
*/
