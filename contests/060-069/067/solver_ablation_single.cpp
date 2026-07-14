// solver_ablation_single.cpp
//
// 高速化版 solver の 1 ファイル対照実験用実装です。
// 競技提出時は通常どおりこの 1 ファイルをコンパイルしてください。
// 比較したいときだけ、下の USE_* を 0/1 で切り替えるか、コンパイル時に -DUSE_XXX=0 を付けます。
//
// 例:
//   g++ -std=c++20 -O2 -pipe -DLOCAL solver_ablation_single.cpp -o fast_all
//   g++ -std=c++20 -O2 -pipe -DLOCAL -DUSE_FAST_STRUCTURAL_PATHS=0 solver_ablation_single.cpp -o no_structural
//   g++ -std=c++20 -O2 -pipe -DLOCAL -DUSE_FAST_EXACT_BFS=0 solver_ablation_single.cpp -o no_exact_bfs
//
// LOCAL を付けると stderr に以下が出ます:
//   config=...                         有効/無効な機能
//   bits=... exact=... candidates=...   探索量とスコア
//   treeinfo_ms/gadget_ms/assess_ms     時間内訳
//
// 機能一覧:
//   USE_FAST_STRUCTURAL_PATHS : 非木辺評価で edge_path DFS をやめ、tin/tout で構造辺だけ判定する
//   USE_FAST_FIRST_EDGE       : first_edge を DFS ではなく root 済み木 + binary lifting で求める
//   USE_FAST_EXACT_BFS        : exact_shortest_path の配列と queue を再利用する
//   USE_HASH_CACHE            : Candidate に tree hash を保持し、edge swap 時に差分更新する
//   USE_GADGET_PRUNE          : find_gadget で best.bits 未満の DP を省略する
//
// 注意:
//   USE_FAST_STRUCTURAL_PATHS と USE_FAST_FIRST_EDGE は TreeInfo を使うため、片方だけ OFF にしても
//   木前処理の時間 treeinfo_ms は残ります。評価すべき指標は主に generated/tree_iter/assess_ms/exact/time です。

#include <bits/stdc++.h>
using namespace std;

#ifndef USE_FAST_STRUCTURAL_PATHS
#define USE_FAST_STRUCTURAL_PATHS 1
#endif
#ifndef USE_FAST_FIRST_EDGE
#define USE_FAST_FIRST_EDGE 1
#endif
#ifndef USE_FAST_EXACT_BFS
#define USE_FAST_EXACT_BFS 1
#endif
#ifndef USE_HASH_CACHE
#define USE_HASH_CACHE 1
#endif
#ifndef USE_GADGET_PRUNE
#define USE_GADGET_PRUNE 1
#endif
#ifndef RNG_SEED
#define RNG_SEED 88172645463325252ULL
#endif
#ifndef INITIAL_SEARCH_LIMIT_SEC
#define INITIAL_SEARCH_LIMIT_SEC 0.68
#endif
#ifndef TREE_SEARCH_LIMIT_SEC
#define TREE_SEARCH_LIMIT_SEC 1.38
#endif
#ifndef TIME_LIMIT_SEC
#define TIME_LIMIT_SEC 1.75
#endif

struct Timer {
    chrono::steady_clock::time_point start = chrono::steady_clock::now();
    double elapsed() const {
        return chrono::duration<double>(chrono::steady_clock::now() - start).count();
    }
};

struct XorShift {
    uint64_t x;
    explicit XorShift(uint64_t seed = RNG_SEED) : x(seed) {}
    uint64_t next() {
        x ^= x << 7;
        x ^= x >> 9;
        return x;
    }
    int operator()(int n) { return int(next() % n); }
    double uniform() { return (next() >> 11) * (1.0 / (1ULL << 53)); }
};

struct Edge {
    int u, v;
    int d, i, j;
};

struct Door {
    int edge_id;
    int type;
};

struct Switch {
    int vertex;
    int type;
};

struct Gadget {
    long long estimate = -1;
    int bits = 0;
    int hub = -1;
    int tail = -1;
    vector<int> hubs;
    vector<int> side_switches;
};

struct Candidate {
    int exact = -1;
    uint64_t hash = 0;
    uint32_t archive_sources = 0;
    int critical_shortcuts = INT_MAX;
    int uncovered_shortcuts = INT_MAX;
    long long shortcut_risk = (1LL << 60);
    long long direct_score = -(1LL << 60);
    vector<char> in_tree;
    Gadget gadget;
};

enum ArchiveSource : uint32_t {
    SOURCE_REJECT_DIRECT = 1U << 0,
    SOURCE_ACCEPTED = 1U << 1,
};

struct SearchStats {
    long long initial_candidates = 0;
    long long generated_candidates = 0;
    long long accepted_candidates = 0;
    long long improved_candidates = 0;
    long long archive_inserted = 0;
    long long duplicate_candidates = 0;
    long long exact_evaluated = 0;
    double treeinfo_time = 0;
    double gadget_time = 0;
    double assess_time = 0;
};

struct TreeInfo {
    vector<vector<pair<int, int>>> tree;
    vector<int> parent;
    vector<int> parent_edge;
    vector<int> depth;
    vector<int> tin;
    vector<int> tout;
    vector<vector<int>> up;
    int timer = 0;

    bool is_ancestor(int a, int b) const {
        return tin[a] <= tin[b] && tin[b] <= tout[a];
    }

    bool edge_on_path_by_child(int child, int u, int v) const {
        return is_ancestor(child, u) != is_ancestor(child, v);
    }

    int jump_up(int v, int steps) const {
        for (int bit = 0; steps > 0; ++bit) {
            if (steps & 1) v = up[bit][v];
            steps >>= 1;
        }
        return v;
    }
};

class Solver {
    static constexpr double INITIAL_SEARCH_LIMIT = INITIAL_SEARCH_LIMIT_SEC;
    static constexpr double TREE_SEARCH_LIMIT = TREE_SEARCH_LIMIT_SEC;
    static constexpr double TIME_LIMIT = TIME_LIMIT_SEC;

    int N, M, K;
    vector<string> board;
    vector<vector<int>> id;
    vector<pair<int, int>> pos;
    vector<Edge> edges;
    vector<vector<pair<int, int>>> graph;
    int start_vertex, goal_vertex;

    Timer timer;
    XorShift rng{RNG_SEED};
    SearchStats stats;
    vector<uint64_t> edge_hash;

#if USE_FAST_EXACT_BFS
    mutable vector<int8_t> bfs_door_type;
    mutable vector<int8_t> bfs_switch_type;
    mutable vector<int> bfs_seen;
    mutable vector<int> bfs_dist;
    mutable vector<int> bfs_queue;
    mutable int bfs_stamp = 1;
#endif

    vector<char> random_dfs_tree() {
        const int V = (int)pos.size();
        vector<char> used(V, false), in_tree(edges.size(), false);
        int root = rng(3) == 0 ? rng(V) : start_vertex;

        auto dfs = [&](auto&& self, int v) -> void {
            used[v] = true;
            vector<pair<uint64_t, pair<int, int>>> candidates;
            candidates.reserve(graph[v].size());
            for (auto [to, eid] : graph[v]) {
                candidates.push_back({rng.next(), {to, eid}});
            }
            sort(candidates.begin(), candidates.end());
            for (auto [key, next] : candidates) {
                auto [to, eid] = next;
                if (used[to]) continue;
                in_tree[eid] = true;
                self(self, to);
            }
        };
        dfs(dfs, root);
        return in_tree;
    }

    vector<char> random_kruskal_tree() {
        const int V = (int)pos.size();
        vector<int> order(edges.size());
        iota(order.begin(), order.end(), 0);
        for (int i = (int)order.size() - 1; i > 0; --i) swap(order[i], order[rng(i + 1)]);

        vector<int> parent(V), size(V, 1);
        iota(parent.begin(), parent.end(), 0);
        auto root = [&](auto&& self, int v) -> int {
            return parent[v] == v ? v : parent[v] = self(self, parent[v]);
        };

        vector<char> in_tree(edges.size(), false);
        for (int eid : order) {
            int a = root(root, edges[eid].u);
            int b = root(root, edges[eid].v);
            if (a == b) continue;
            if (size[a] < size[b]) swap(a, b);
            parent[b] = a;
            size[a] += size[b];
            in_tree[eid] = true;
        }
        return in_tree;
    }

    vector<vector<pair<int, int>>> make_tree(const vector<char>& in_tree) const {
        vector<vector<pair<int, int>>> tree(pos.size());
        for (int eid = 0; eid < (int)edges.size(); ++eid) {
            if (!in_tree[eid]) continue;
            const auto& e = edges[eid];
            tree[e.u].push_back({e.v, eid});
            tree[e.v].push_back({e.u, eid});
        }
        return tree;
    }

    TreeInfo build_tree_info(const vector<char>& in_tree) const {
        const int V = (int)pos.size();
        TreeInfo info;
        info.tree = make_tree(in_tree);
        info.parent.assign(V, -1);
        info.parent_edge.assign(V, -1);
        info.depth.assign(V, 0);
        info.tin.assign(V, 0);
        info.tout.assign(V, 0);

        vector<int> iter(V, 0), stack;
        stack.reserve(V);
        stack.push_back(start_vertex);
        info.parent[start_vertex] = start_vertex;
        info.parent_edge[start_vertex] = -1;
        info.depth[start_vertex] = 0;
        info.timer = 0;

        while (!stack.empty()) {
            int v = stack.back();
            if (iter[v] == 0) info.tin[v] = info.timer++;
            if (iter[v] < (int)info.tree[v].size()) {
                auto [to, eid] = info.tree[v][iter[v]++];
                if (to == info.parent[v]) continue;
                info.parent[to] = v;
                info.parent_edge[to] = eid;
                info.depth[to] = info.depth[v] + 1;
                stack.push_back(to);
            } else {
                info.tout[v] = info.timer - 1;
                stack.pop_back();
            }
        }

        int LOG = 1;
        while ((1 << LOG) <= max(1, V)) ++LOG;
        info.up.assign(LOG, vector<int>(V));
        for (int v = 0; v < V; ++v) {
            info.up[0][v] = info.parent[v] == -1 ? v : info.parent[v];
        }
        for (int k = 1; k < LOG; ++k) {
            for (int v = 0; v < V; ++v) {
                info.up[k][v] = info.up[k - 1][info.up[k - 1][v]];
            }
        }
        return info;
    }

    vector<int> vertex_path(
        const vector<vector<pair<int, int>>>& tree,
        int source,
        int target
    ) const {
        vector<int> parent(pos.size(), -1);
        vector<int> stack = {source};
        parent[source] = source;
        while (!stack.empty()) {
            int v = stack.back();
            stack.pop_back();
            if (v == target) break;
            for (auto [to, eid] : tree[v]) {
                if (parent[to] != -1) continue;
                parent[to] = v;
                stack.push_back(to);
            }
        }
        vector<int> path;
        for (int v = target; v != source; v = parent[v]) {
            if (v == -1) return {};
            path.push_back(v);
        }
        path.push_back(source);
        reverse(path.begin(), path.end());
        return path;
    }

    int first_edge_slow(
        const vector<vector<pair<int, int>>>& tree,
        int source,
        int target
    ) const {
        vector<int> parent(pos.size(), -1), parent_edge(pos.size(), -1);
        vector<int> stack = {source};
        parent[source] = source;
        while (!stack.empty()) {
            int v = stack.back();
            stack.pop_back();
            if (v == target) break;
            for (auto [to, eid] : tree[v]) {
                if (parent[to] != -1) continue;
                parent[to] = v;
                parent_edge[to] = eid;
                stack.push_back(to);
            }
        }
        int v = target;
        while (parent[v] != source) v = parent[v];
        return parent_edge[v];
    }

    int first_edge_fast(const TreeInfo& info, int source, int target) const {
        if (source == target) return -1;
        if (info.is_ancestor(source, target)) {
            int steps = info.depth[target] - info.depth[source] - 1;
            int child = info.jump_up(target, steps);
            return info.parent_edge[child];
        }
        return info.parent_edge[source];
    }

    int first_edge_select(const TreeInfo& info, int source, int target) const {
#if USE_FAST_FIRST_EDGE
        return first_edge_fast(info, source, target);
#else
        return first_edge_slow(info.tree, source, target);
#endif
    }

    vector<int> edge_path(
        const vector<vector<pair<int, int>>>& tree,
        int source,
        int target
    ) const {
        vector<int> parent(pos.size(), -1), parent_edge(pos.size(), -1);
        vector<int> stack = {source};
        parent[source] = source;
        while (!stack.empty()) {
            int v = stack.back();
            stack.pop_back();
            if (v == target) break;
            for (auto [to, eid] : tree[v]) {
                if (parent[to] != -1) continue;
                parent[to] = v;
                parent_edge[to] = eid;
                stack.push_back(to);
            }
        }
        vector<int> path;
        for (int v = target; v != source; v = parent[v]) {
            if (v == -1) return {};
            path.push_back(parent_edge[v]);
        }
        return path;
    }

    int tree_edge_child(const TreeInfo& info, int eid) const {
        const auto& e = edges[eid];
        if (info.parent[e.u] == e.v) return e.u;
        if (info.parent[e.v] == e.u) return e.v;
        return -1;
    }

    Gadget find_gadget(const TreeInfo& info) const {
        const int V = (int)pos.size();
        const int max_bits = K;
        if (max_bits <= 0) return {};

        const auto& tree = info.tree;
        vector<int> main_path = vertex_path(tree, start_vertex, goal_vertex);
        vector<char> on_main(V, false);
        for (int v : main_path) on_main[v] = true;

        Gadget best;

        for (int main_index = 1; main_index + 1 < (int)main_path.size(); ++main_index) {
            int hub = main_path[main_index];
            for (auto [root, root_edge] : tree[hub]) {
                if (on_main[root]) continue;

                vector<int> parent(V, -1), depth(V, -1);
                vector<vector<int>> children(V);
                vector<int> order = {root};
                parent[root] = hub;
                depth[root] = 1;
                for (int qi = 0; qi < (int)order.size(); ++qi) {
                    int v = order[qi];
                    for (auto [to, eid] : tree[v]) {
                        if (to == parent[v]) continue;
                        parent[to] = v;
                        depth[to] = depth[v] + 1;
                        children[v].push_back(to);
                        order.push_back(to);
                    }
                }

                vector<int> leaves;
                for (int v : order) {
                    if (children[v].empty()) leaves.push_back(v);
                }

                for (int leaf : leaves) {
                    vector<int> branch_path;
                    for (int v = leaf; v != hub; v = parent[v]) branch_path.push_back(v);
                    reverse(branch_path.begin(), branch_path.end());
                    const int length = (int)branch_path.size();
                    if (length == 0) continue;

                    vector<int> side_depth(length, 0), side_end(length, -1);
                    for (int index = 0; index < length; ++index) {
                        int v = branch_path[index];
                        int next = index + 1 < length ? branch_path[index + 1] : -1;
                        for (int child : children[v]) {
                            if (child == next) continue;
                            vector<pair<int, int>> stack = {{child, 1}};
                            while (!stack.empty()) {
                                auto [u, d] = stack.back();
                                stack.pop_back();
                                if (d > side_depth[index]) {
                                    side_depth[index] = d;
                                    side_end[index] = u;
                                }
                                for (int to : children[u]) stack.push_back({to, d + 1});
                            }
                        }
                    }

                    for (int bits = max_bits; bits >= 1; --bits) {
#if USE_GADGET_PRUNE
                        if (bits < best.bits) break;
#endif
                        const int selected_count = bits - 1;
                        if (selected_count == 0) {
                            long long estimate = main_index + 2LL * length + 1;
                            if (bits > best.bits
                                || (bits == best.bits && estimate > best.estimate)) {
                                best.estimate = estimate;
                                best.bits = bits;
                                best.hub = hub;
                                best.tail = leaf;
                                best.hubs.clear();
                                best.side_switches.clear();
                            }
                            continue;
                        }
                        if (selected_count >= length) continue;

                        const long long NEG = -(1LL << 60);
                        vector<vector<long long>> dp(
                            selected_count + 1,
                            vector<long long>(length, NEG)
                        );
                        vector<vector<int>> previous(
                            selected_count + 1,
                            vector<int>(length, -1)
                        );

                        for (int index = 0; index + 1 < length; ++index) {
                            if (side_depth[index] == 0) continue;
                            dp[1][index] = (index + 1LL) + side_depth[index];
                        }

                        for (int count = 2; count <= selected_count; ++count) {
                            long long weight = 1LL << (count - 1);
                            for (int index = 0; index + 1 < length; ++index) {
                                if (side_depth[index] == 0) continue;
                                for (int prev = 0; prev < index; ++prev) {
                                    if (dp[count - 1][prev] == NEG) continue;
                                    long long value = dp[count - 1][prev]
                                        + weight * (index - prev + side_depth[index]);
                                    if (value > dp[count][index]) {
                                        dp[count][index] = value;
                                        previous[count][index] = prev;
                                    }
                                }
                            }
                        }

                        for (int last = 0; last + 1 < length; ++last) {
                            if (dp[selected_count][last] == NEG) continue;
                            long long estimate =
                                main_index
                                + dp[selected_count][last]
                                + (1LL << selected_count) * (length - 1 - last)
                                + (1LL << selected_count);
                            if (bits < best.bits) continue;
                            if (bits == best.bits && estimate <= best.estimate) continue;

                            vector<int> selected(selected_count);
                            int index = last;
                            for (int count = selected_count; count >= 1; --count) {
                                selected[count - 1] = index;
                                index = previous[count][index];
                            }

                            best.estimate = estimate;
                            best.bits = bits;
                            best.hub = hub;
                            best.tail = leaf;
                            best.hubs.clear();
                            best.side_switches.clear();
                            for (int path_index : selected) {
                                best.hubs.push_back(branch_path[path_index]);
                                best.side_switches.push_back(side_end[path_index]);
                            }
                        }
                    }
                }
            }
        }
        return best;
    }

    void assess_candidate(Candidate& candidate, const TreeInfo& info) const {
        if (candidate.gadget.bits == 0) return;

        vector<int> structural_type(edges.size(), -1);
        vector<char> critical_gate(edges.size(), false);
        const int top_bit = candidate.gadget.bits - 1;

        auto set_structural = [&](int eid, int type, bool critical) {
            if (eid < 0) return;
            structural_type[eid] = type;
            critical_gate[eid] = critical_gate[eid] || critical;
        };

        int start_gate = first_edge_select(info, candidate.gadget.hub, start_vertex);
        int goal_gate = first_edge_select(info, candidate.gadget.hub, goal_vertex);
        set_structural(start_gate, 2 * top_bit, true);
        set_structural(goal_gate, 2 * top_bit + 1, true);

        int previous_hub = candidate.gadget.hub;
        for (int index = 0; index < (int)candidate.gadget.hubs.size(); ++index) {
            int current_hub = candidate.gadget.hubs[index];
            int bit = candidate.gadget.bits - 2 - index;
            set_structural(first_edge_select(info, previous_hub, current_hub), 2 * bit, false);
            set_structural(
                first_edge_select(info, current_hub, candidate.gadget.side_switches[index]),
                2 * bit + 1,
                false
            );
            previous_hub = current_hub;
        }

        struct Shortcut {
            bool critical;
            long long risk;
        };
        vector<Shortcut> shortcuts;

#if USE_FAST_STRUCTURAL_PATHS
        struct StructuralEdge {
            int child;
            int type;
            bool critical;
            long long risk_weight;
        };
        vector<StructuralEdge> structural_edges;
        structural_edges.reserve(2 * candidate.gadget.bits);
        for (int eid = 0; eid < (int)edges.size(); ++eid) {
            if (structural_type[eid] == -1) continue;
            int child = tree_edge_child(info, eid);
            if (child == -1) continue;
            int bit = structural_type[eid] / 2;
            structural_edges.push_back({
                child,
                structural_type[eid],
                (bool)critical_gate[eid],
                1LL << max(0, K - 1 - bit)
            });
        }

        for (int eid = 0; eid < (int)edges.size(); ++eid) {
            if (candidate.in_tree[eid]) continue;
            const auto& edge = edges[eid];
            bool critical = false;
            long long risk = 0;
            for (const auto& se : structural_edges) {
                if (!info.edge_on_path_by_child(se.child, edge.u, edge.v)) continue;
                critical |= se.critical;
                risk += se.risk_weight;
            }
            if (critical || risk > 0) shortcuts.push_back({critical, risk});
        }
#else
        for (int eid = 0; eid < (int)edges.size(); ++eid) {
            if (candidate.in_tree[eid]) continue;
            const auto& edge = edges[eid];
            bool critical = false;
            long long risk = 0;
            for (int tree_eid : edge_path(info.tree, edge.u, edge.v)) {
                int type = structural_type[tree_eid];
                if (type == -1) continue;
                critical |= critical_gate[tree_eid];
                int bit = type / 2;
                risk += 1LL << max(0, K - 1 - bit);
            }
            if (critical || risk > 0) shortcuts.push_back({critical, risk});
        }
#endif

        sort(shortcuts.begin(), shortcuts.end(), [](const Shortcut& a, const Shortcut& b) {
            if (a.critical != b.critical) return a.critical > b.critical;
            return a.risk > b.risk;
        });

        const int structural_doors = 2 * candidate.gadget.bits;
        const int chord_budget = max(0, M - structural_doors);
        candidate.critical_shortcuts = 0;
        candidate.uncovered_shortcuts = 0;
        candidate.shortcut_risk = 0;
        for (int index = chord_budget; index < (int)shortcuts.size(); ++index) {
            candidate.critical_shortcuts += shortcuts[index].critical;
            candidate.uncovered_shortcuts += shortcuts[index].risk > 0;
            candidate.shortcut_risk += shortcuts[index].risk;
        }

        candidate.direct_score =
            candidate.gadget.estimate
            - 4 * candidate.shortcut_risk
            - (1LL << 40) * candidate.critical_shortcuts;
    }

    Candidate make_candidate(vector<char> in_tree, uint64_t hash) {
        Candidate candidate;
        candidate.in_tree = std::move(in_tree);
        candidate.hash = hash;

        auto t0 = chrono::steady_clock::now();
        TreeInfo info = build_tree_info(candidate.in_tree);
        auto t1 = chrono::steady_clock::now();
        candidate.gadget = find_gadget(info);
        auto t2 = chrono::steady_clock::now();
        assess_candidate(candidate, info);
        auto t3 = chrono::steady_clock::now();
        stats.treeinfo_time += chrono::duration<double>(t1 - t0).count();
        stats.gadget_time += chrono::duration<double>(t2 - t1).count();
        stats.assess_time += chrono::duration<double>(t3 - t2).count();
        return candidate;
    }

    static bool better_direct(const Candidate& a, const Candidate& b) {
        if (a.gadget.bits != b.gadget.bits) return a.gadget.bits > b.gadget.bits;
        if (a.critical_shortcuts != b.critical_shortcuts) {
            return a.critical_shortcuts < b.critical_shortcuts;
        }
        if (a.direct_score != b.direct_score) return a.direct_score > b.direct_score;
        if (a.uncovered_shortcuts != b.uncovered_shortcuts) {
            return a.uncovered_shortcuts < b.uncovered_shortcuts;
        }
        return a.gadget.estimate > b.gadget.estimate;
    }

    uint64_t tree_hash(const vector<char>& in_tree) const {
        uint64_t hash = 0;
        for (int eid = 0; eid < (int)in_tree.size(); ++eid) {
            if (in_tree[eid]) hash ^= edge_hash[eid];
        }
        return hash;
    }

    pair<vector<Door>, vector<Switch>> build_solution(
        const vector<char>& in_tree,
        const Gadget& gadget
    ) const {
        TreeInfo info = build_tree_info(in_tree);
        vector<Door> structural_doors;
        vector<Switch> switches;
        const int top_bit = gadget.bits - 1;
        const int blocker_bit = gadget.bits < K ? gadget.bits : top_bit;

        int start_gate = first_edge_select(info, gadget.hub, start_vertex);
        int goal_gate = first_edge_select(info, gadget.hub, goal_vertex);
        structural_doors.push_back({start_gate, 2 * top_bit});
        structural_doors.push_back({goal_gate, 2 * top_bit + 1});

        int previous_hub = gadget.hub;
        for (int index = 0; index < (int)gadget.hubs.size(); ++index) {
            int current_hub = gadget.hubs[index];
            int bit = gadget.bits - 2 - index;
            structural_doors.push_back({
                first_edge_select(info, previous_hub, current_hub),
                2 * bit
            });
            structural_doors.push_back({
                first_edge_select(info, current_hub, gadget.side_switches[index]),
                2 * bit + 1
            });
            switches.push_back({gadget.side_switches[index], bit + 1});
            previous_hub = current_hub;
        }
        switches.push_back({gadget.tail, 0});

        vector<int> structural_type(edges.size(), -1);
        vector<long long> structural_weight(edges.size(), 0);
        for (const auto& door : structural_doors) {
            structural_type[door.edge_id] = door.type;
            int bit = door.type / 2;
            structural_weight[door.edge_id] = 1LL << (K - 1 - min(bit, K - 1));
        }
        structural_weight[start_gate] += 1LL << 30;
        structural_weight[goal_gate] += 1LL << 30;

        struct Chord {
            long long importance;
            int crossings;
            int edge_id;
        };
        vector<Chord> chords;
        chords.reserve(edges.size());

#if USE_FAST_STRUCTURAL_PATHS
        struct StructuralEdge {
            int child;
            long long weight;
        };
        vector<StructuralEdge> structural_edges;
        structural_edges.reserve(structural_doors.size());
        for (int eid = 0; eid < (int)edges.size(); ++eid) {
            if (structural_type[eid] == -1) continue;
            int child = tree_edge_child(info, eid);
            if (child == -1) continue;
            structural_edges.push_back({child, structural_weight[eid]});
        }

        for (int eid = 0; eid < (int)edges.size(); ++eid) {
            if (in_tree[eid]) continue;
            const auto& edge = edges[eid];
            long long importance = 0;
            int crossings = 0;
            for (const auto& se : structural_edges) {
                if (!info.edge_on_path_by_child(se.child, edge.u, edge.v)) continue;
                importance += se.weight;
                ++crossings;
            }
            chords.push_back({importance, crossings, eid});
        }
#else
        for (int eid = 0; eid < (int)edges.size(); ++eid) {
            if (in_tree[eid]) continue;
            const auto& edge = edges[eid];
            long long importance = 0;
            int crossings = 0;
            for (int tree_eid : edge_path(info.tree, edge.u, edge.v)) {
                if (structural_type[tree_eid] == -1) continue;
                importance += structural_weight[tree_eid];
                ++crossings;
            }
            chords.push_back({importance, crossings, eid});
        }
#endif
        sort(chords.begin(), chords.end(), [](const Chord& a, const Chord& b) {
            if (a.importance != b.importance) return a.importance > b.importance;
            if (a.crossings != b.crossings) return a.crossings > b.crossings;
            return a.edge_id < b.edge_id;
        });

        const int chord_budget = max(0, M - (int)structural_doors.size());
        vector<Door> doors;
        for (int index = 0; index < min(chord_budget, (int)chords.size()); ++index) {
            doors.push_back({chords[index].edge_id, 2 * blocker_bit + 1});
        }
        doors.insert(doors.end(), structural_doors.begin(), structural_doors.end());
        return {doors, switches};
    }

    int optimize_non_tree_doors(
        vector<Door>& doors,
        const vector<Switch>& switches,
        int non_tree_count,
        int bits,
        int initial_score
    ) {
        if (bits < K || bits <= 1 || non_tree_count == 0) return initial_score;

        vector<Door> current = doors;
        vector<Door> best = doors;
        int current_score = initial_score;
        int best_score = initial_score;

        vector<int> candidate_types;
        for (int bit = 0; bit + 1 < bits; ++bit) {
            candidate_types.push_back(2 * bit + 1);
        }

        // First test coherent assignments. They often reveal a useful bit cheaply.
        for (int type : candidate_types) {
            if (timer.elapsed() >= TIME_LIMIT) break;
            vector<Door> candidate = doors;
            for (int index = 0; index < non_tree_count; ++index) {
                candidate[index].type = type;
            }
            int score = exact_shortest_path(candidate, switches);
            if (score > best_score) {
                best_score = score;
                best = std::move(candidate);
            }
        }
        current = best;
        current_score = best_score;

        vector<int> order(non_tree_count);
        iota(order.begin(), order.end(), 0);
        for (int i = non_tree_count - 1; i > 0; --i) swap(order[i], order[rng(i + 1)]);

        // Coordinate ascent with exact state-BFS evaluation.
        for (int door_index : order) {
            if (timer.elapsed() >= TIME_LIMIT) break;
            int chosen_type = current[door_index].type;
            int chosen_score = current_score;

            vector<int> types = candidate_types;
            for (int i = (int)types.size() - 1; i > 0; --i) swap(types[i], types[rng(i + 1)]);
            for (int type : types) {
                if (timer.elapsed() >= TIME_LIMIT) break;
                if (type == current[door_index].type) continue;
                current[door_index].type = type;
                int score = exact_shortest_path(current, switches);
                if (score >= chosen_score) {
                    chosen_score = score;
                    chosen_type = type;
                }
            }
            current[door_index].type = chosen_type;
            current_score = chosen_score;
            if (current_score > best_score) {
                best_score = current_score;
                best = current;
            }
        }

        doors = std::move(best);
        return best_score;
    }

    int exact_shortest_path(const vector<Door>& doors, const vector<Switch>& switches) const {
        const int V = (int)pos.size();
        const int MASKS = 1 << K;
#if USE_FAST_EXACT_BFS
        if ((int)bfs_door_type.size() != (int)edges.size()) {
            bfs_door_type.assign(edges.size(), -1);
        } else {
            fill(bfs_door_type.begin(), bfs_door_type.end(), -1);
        }
        if ((int)bfs_switch_type.size() != V) {
            bfs_switch_type.assign(V, -1);
        } else {
            fill(bfs_switch_type.begin(), bfs_switch_type.end(), -1);
        }
        for (const auto& door : doors) bfs_door_type[door.edge_id] = door.type;
        for (const auto& sw : switches) bfs_switch_type[sw.vertex] = sw.type;

        const int total_states = V * MASKS;
        if ((int)bfs_seen.size() != total_states) {
            bfs_seen.assign(total_states, 0);
            bfs_dist.assign(total_states, 0);
            bfs_queue.reserve(total_states);
            bfs_stamp = 1;
        }
        ++bfs_stamp;
        if (bfs_stamp == INT_MAX) {
            fill(bfs_seen.begin(), bfs_seen.end(), 0);
            bfs_stamp = 1;
        }
        bfs_queue.clear();
        int head = 0;
        int start_state = start_vertex * MASKS;
        bfs_seen[start_state] = bfs_stamp;
        bfs_dist[start_state] = 0;
        bfs_queue.push_back(start_state);

        while (head < (int)bfs_queue.size()) {
            int state = bfs_queue[head++];
            int v = state / MASKS;
            int mask = state & (MASKS - 1);
            int current = bfs_dist[state];
            if (v == goal_vertex) return current;

            for (auto [to, eid] : graph[v]) {
                int type = bfs_door_type[eid];
                if (type != -1 && ((mask >> (type / 2)) & 1) != (type & 1)) continue;
                int next_state = to * MASKS + mask;
                if (bfs_seen[next_state] == bfs_stamp) continue;
                bfs_seen[next_state] = bfs_stamp;
                bfs_dist[next_state] = current + 1;
                bfs_queue.push_back(next_state);
            }
            if (bfs_switch_type[v] != -1) {
                int next_mask = mask ^ (1 << bfs_switch_type[v]);
                int next_state = v * MASKS + next_mask;
                if (bfs_seen[next_state] != bfs_stamp) {
                    bfs_seen[next_state] = bfs_stamp;
                    bfs_dist[next_state] = current + 1;
                    bfs_queue.push_back(next_state);
                }
            }
        }
        return -1;
#else
        vector<int8_t> door_type(edges.size(), -1);
        vector<int8_t> switch_type(V, -1);
        for (const auto& door : doors) door_type[door.edge_id] = door.type;
        for (const auto& sw : switches) switch_type[sw.vertex] = sw.type;

        vector<int> dist(V * MASKS, -1);
        deque<pair<int, int>> que;
        dist[start_vertex * MASKS] = 0;
        que.push_back({start_vertex, 0});

        while (!que.empty()) {
            auto [v, mask] = que.front();
            que.pop_front();
            int current = dist[v * MASKS + mask];
            if (v == goal_vertex) return current;

            for (auto [to, eid] : graph[v]) {
                int type = door_type[eid];
                if (type != -1 && ((mask >> (type / 2)) & 1) != (type & 1)) continue;
                int index = to * MASKS + mask;
                if (dist[index] != -1) continue;
                dist[index] = current + 1;
                que.push_back({to, mask});
            }
            if (switch_type[v] != -1) {
                int next_mask = mask ^ (1 << switch_type[v]);
                int index = v * MASKS + next_mask;
                if (dist[index] == -1) {
                    dist[index] = current + 1;
                    que.push_back({v, next_mask});
                }
            }
        }
        return -1;
#endif
    }

    pair<vector<Door>, vector<Switch>> fallback() const {
        return {{}, {}};
    }

    void output(const vector<Door>& doors, const vector<Switch>& switches) const {
        cout << doors.size() << '\n';
        for (const auto& door : doors) {
            const auto& e = edges[door.edge_id];
            cout << e.d << ' ' << e.i << ' ' << e.j << ' ' << door.type << '\n';
        }
        cout << switches.size() << '\n';
        for (const auto& sw : switches) {
            auto [i, j] = pos[sw.vertex];
            cout << i << ' ' << j << ' ' << sw.type << '\n';
        }
    }

public:
    void solve() {
        cin >> N >> M >> K;
        board.resize(N);
        for (auto& row : board) cin >> row;

        id.assign(N, vector<int>(N, -1));
        for (int i = 0; i < N; ++i) {
            for (int j = 0; j < N; ++j) {
                if (board[i][j] == '.') {
                    id[i][j] = (int)pos.size();
                    pos.push_back({i, j});
                }
            }
        }
        start_vertex = id[0][0];
        goal_vertex = id[N - 1][N - 1];
        graph.assign(pos.size(), {});

        auto add_edge = [&](int i1, int j1, int i2, int j2, int d) {
            if (i2 >= N || j2 >= N || id[i2][j2] == -1) return;
            int eid = (int)edges.size();
            edges.push_back({id[i1][j1], id[i2][j2], d, i1, j1});
            graph[id[i1][j1]].push_back({id[i2][j2], eid});
            graph[id[i2][j2]].push_back({id[i1][j1], eid});
        };
        for (int i = 0; i < N; ++i) {
            for (int j = 0; j < N; ++j) {
                if (id[i][j] == -1) continue;
                add_edge(i, j, i + 1, j, 0);
                add_edge(i, j, i, j + 1, 1);
            }
        }

        edge_hash.resize(edges.size());
        auto splitmix64 = [](uint64_t x) {
            x += 0x9e3779b97f4a7c15ULL;
            x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
            x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
            return x ^ (x >> 31);
        };
        for (int eid = 0; eid < (int)edges.size(); ++eid) {
            edge_hash[eid] = splitmix64(eid + 1);
        }

        vector<Candidate> reject_archive;
        vector<Candidate> accepted_archive;
        unordered_set<uint64_t> generated_hashes;
        generated_hashes.reserve(4096);

        auto insert_archive = [&](vector<Candidate>& archive,
                                  const Candidate& candidate,
                                  int capacity,
                                  auto comparator) {
            archive.push_back(candidate);
            sort(archive.begin(), archive.end(), comparator);
            if ((int)archive.size() > capacity) archive.pop_back();
            return any_of(archive.begin(), archive.end(), [&](const Candidate& stored) {
#if USE_HASH_CACHE
                return stored.hash == candidate.hash;
#else
                return tree_hash(stored.in_tree) == tree_hash(candidate.in_tree);
#endif
            });
        };

        while (timer.elapsed() < INITIAL_SEARCH_LIMIT) {
            vector<char> tree = rng(3) == 0 ? random_dfs_tree() : random_kruskal_tree();
            uint64_t hash = tree_hash(tree);
            if (!generated_hashes.insert(hash).second) {
                ++stats.duplicate_candidates;
                continue;
            }
            ++stats.initial_candidates;
            Candidate candidate = make_candidate(std::move(tree), hash);
            stats.archive_inserted += insert_archive(
                accepted_archive,
                candidate,
                12,
                better_direct
            );
        }

        Candidate current = accepted_archive.empty() ? Candidate() : accepted_archive.front();
        int stale = 0;
        [[maybe_unused]] int tree_iterations = 0;
        while (!accepted_archive.empty() && timer.elapsed() < TREE_SEARCH_LIMIT) {
            int added_edge = rng((int)edges.size());
            if (current.in_tree[added_edge]) continue;

            TreeInfo current_info = build_tree_info(current.in_tree);
            const auto& edge = edges[added_edge];
            vector<int> cycle = edge_path(current_info.tree, edge.u, edge.v);
            if (cycle.empty()) continue;

            int removed_edge = cycle[rng((int)cycle.size())];
            vector<char> next_tree = current.in_tree;
            next_tree[added_edge] = true;
            next_tree[removed_edge] = false;
#if USE_HASH_CACHE
            uint64_t hash = current.hash ^ edge_hash[added_edge] ^ edge_hash[removed_edge];
#else
            uint64_t hash = tree_hash(next_tree);
#endif
            if (!generated_hashes.insert(hash).second) {
                ++stats.duplicate_candidates;
                continue;
            }
            ++stats.generated_candidates;
            Candidate next = make_candidate(std::move(next_tree), hash);
            if (next.gadget.bits == 0) continue;

            bool improve = better_direct(next, current);
            bool accept = improve;
            if (!accept
                && next.gadget.bits == current.gadget.bits
                && next.critical_shortcuts == current.critical_shortcuts) {
                double progress = timer.elapsed() / TREE_SEARCH_LIMIT;
                double temperature = 120.0 * pow(8.0 / 120.0, progress);
                long long delta = next.direct_score - current.direct_score;
                accept = rng.uniform() < exp((double)delta / temperature);
            }

            if (accept) {
                current = std::move(next);
                stats.archive_inserted += insert_archive(
                    accepted_archive,
                    current,
                    12,
                    better_direct
                );
                ++stats.accepted_candidates;
                stats.improved_candidates += improve;
                stale = improve ? 0 : stale + 1;
            } else {
                stats.archive_inserted += insert_archive(
                    reject_archive,
                    next,
                    4,
                    better_direct
                );
                ++stale;
            }
            if (stale >= 60) {
                current = accepted_archive[rng(min(4, (int)accepted_archive.size()))];
                stale = 0;
            }
            ++tree_iterations;
        }

        vector<Candidate> promising;
        unordered_map<uint64_t, int> exact_index;
        auto append_archive = [&](const vector<Candidate>& archive, uint32_t source) {
            for (const auto& candidate : archive) {
#if USE_HASH_CACHE
                uint64_t hash = candidate.hash;
#else
                uint64_t hash = tree_hash(candidate.in_tree);
#endif
                auto [it, inserted] = exact_index.emplace(hash, (int)promising.size());
                if (inserted) {
                    promising.push_back(candidate);
                    promising.back().archive_sources = source;
                } else {
                    promising[it->second].archive_sources |= source;
                }
            }
        };
        append_archive(reject_archive, SOURCE_REJECT_DIRECT);
        append_archive(accepted_archive, SOURCE_ACCEPTED);
        sort(promising.begin(), promising.end(), better_direct);

        Candidate best;
        array<int, 2> source_candidates{};
        array<int, 2> source_unique_candidates{};
        array<int, 2> source_best_exact{};
        array<int, 2> source_ablation_best{};
        const array<uint32_t, 2> source_bits = {
            SOURCE_REJECT_DIRECT,
            SOURCE_ACCEPTED
        };
        [[maybe_unused]] const array<const char*, 2> source_names = {
            "reject",
            "accepted"
        };
        for (auto& candidate : promising) {
            if (timer.elapsed() >= TIME_LIMIT) break;
            auto [doors, switches] = build_solution(candidate.in_tree, candidate.gadget);
            candidate.exact = exact_shortest_path(doors, switches);
            ++stats.exact_evaluated;
            int source_count = popcount(candidate.archive_sources);
            for (int index = 0; index < (int)source_bits.size(); ++index) {
                if (!(candidate.archive_sources & source_bits[index])) continue;
                ++source_candidates[index];
                source_unique_candidates[index] += source_count == 1;
                source_best_exact[index] = max(source_best_exact[index], candidate.exact);
                if (candidate.archive_sources != source_bits[index]) {
                    source_ablation_best[index] = max(
                        source_ablation_best[index],
                        candidate.exact
                    );
                }
            }
            for (int index = 0; index < (int)source_bits.size(); ++index) {
                if (candidate.archive_sources & source_bits[index]) continue;
                source_ablation_best[index] = max(
                    source_ablation_best[index],
                    candidate.exact
                );
            }
            if (candidate.exact > best.exact) best = std::move(candidate);
        }

        if (best.exact <= 0) {
            auto [doors, switches] = fallback();
            output(doors, switches);
            return;
        }

        auto [doors, switches] = build_solution(best.in_tree, best.gadget);
        int non_tree_count = 0;
        while (non_tree_count < (int)doors.size()
               && !best.in_tree[doors[non_tree_count].edge_id]) {
            ++non_tree_count;
        }
        [[maybe_unused]] int before_optimization = best.exact;
        best.exact = optimize_non_tree_doors(
            doors,
            switches,
            non_tree_count,
            best.gadget.bits,
            best.exact
        );
#ifdef LOCAL
        cerr << "config="
             << "fast_structural:" << USE_FAST_STRUCTURAL_PATHS
             << ",fast_first:" << USE_FAST_FIRST_EDGE
             << ",fast_exact:" << USE_FAST_EXACT_BFS
             << ",hash_cache:" << USE_HASH_CACHE
             << ",gadget_prune:" << USE_GADGET_PRUNE
             << ",seed:" << (uint64_t)RNG_SEED
             << '\n';
        cerr << "bits=" << best.gadget.bits
             << " estimate=" << best.gadget.estimate
             << " exact=" << before_optimization << "->" << best.exact
             << " risk=" << best.shortcut_risk
             << " uncovered=" << best.uncovered_shortcuts
             << " critical=" << best.critical_shortcuts
             << " doors=" << doors.size()
             << " switches=" << switches.size()
             << " candidates=" << promising.size()
             << " tree_iter=" << tree_iterations
             << " initial=" << stats.initial_candidates
             << " generated=" << stats.generated_candidates
             << " accepted=" << stats.accepted_candidates
             << " improved=" << stats.improved_candidates
             << " archived=" << stats.archive_inserted
             << " duplicate=" << stats.duplicate_candidates
             << " exact_eval=" << stats.exact_evaluated
             << " treeinfo_ms=" << stats.treeinfo_time * 1000
             << " gadget_ms=" << stats.gadget_time * 1000
             << " assess_ms=" << stats.assess_time * 1000
             << " time=" << timer.elapsed() << '\n';
        cerr << "archive_contribution winner_mask=" << best.archive_sources;
        for (int index = 0; index < (int)source_bits.size(); ++index) {
            cerr << ' ' << source_names[index]
                 << "={n:" << source_candidates[index]
                 << ",unique:" << source_unique_candidates[index]
                 << ",best:" << source_best_exact[index]
                 << ",without:" << source_ablation_best[index]
                 << ",loss:" << before_optimization - source_ablation_best[index]
                 << '}';
        }
        cerr << '\n';
#endif
        output(doors, switches);
    }
};

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    Solver().solve();
    return 0;
}
