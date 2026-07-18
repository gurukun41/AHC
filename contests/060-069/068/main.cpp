#pragma GCC target("avx2,fma")
#pragma GCC optimize("O3")
#pragma GCC optimize("unroll-loops")

#include <bits/stdc++.h>
#include <atcoder/all>
using namespace std;
using ll = long long;
using ld = long double;
using mint = atcoder::modint998244353;
using vl = vector<ll>;
using vvl = vector<vl>;
using vvvl = vector<vvl>;
using vi = vector<int>;
using vvi = vector<vi>;
using vvvi = vector<vvi>;
using vb = vector<bool>;
using vvb = vector<vb>;
using vvvb = vector<vvb>;
using vs = vector<string>;
using vvs = vector<vs>;
using pl = pair<ll, ll>;
using vpl = vector<pl>;
#define rep(i, a, b) for (ll i = (a); i < (ll)(b); i++)
#define all(v) v.begin(), v.end()

template <typename T>
inline bool chmax(T &a, const T &b) {
    if (a < b) {
        a = b;
        return true;
    }
    return false;
}

template <typename T>
inline bool chmin(T &a, const T &b) {
    if (a > b) {
        a = b;
        return true;
    }
    return false;
}

void yn(bool a) {
    if (a)
        cout << "Yes\n";
    else
        cout << "No\n";
}

template <typename Range>
void yns(const Range &xs) {
    for (const auto &x : xs) yn(x);
}

struct Scanner {
    template <typename T>
    void read(T &x) const {
        cin >> x;
    }

    template <typename T, typename U>
    void read(pair<T, U> &p) const {
        read(p.first);
        read(p.second);
    }

    template <typename T, size_t N>
    void read(array<T, N> &a) const {
        for (T &x : a) read(x);
    }

    template <typename T>
    void read(vector<T> &v) const {
        for (T &x : v) read(x);
    }

    void read(vector<bool> &v) const {
        for (size_t i = 0; i < v.size(); i++) {
            bool x;
            cin >> x;
            v[i] = x;
        }
    }

    template <typename... Ts>
    void operator()(Ts &...xs) const {
        (read(xs), ...);
    }
};

inline constexpr Scanner scan{};

struct Printer {
    template <typename T, typename U>
    void operator()(const pair<T, U> &p, string_view sep = " ", string_view end = "\n") const {
        cout << p.first << sep << p.second << end;
    }

    template <typename Range>
    void operator()(const Range &xs, string_view sep = " ", string_view end = "\n") const {
        bool first = true;
        for (const auto &x : xs) {
            if (!first) cout << sep;
            first = false;
            cout << x;
        }
        cout << end;
    }
};

inline constexpr Printer print{};

template <typename Range>
void prints(const Range &xs) {
    for (const auto &x : xs) cout << x << '\n';
}

bool inside(int x, int y, int h, int w) {
    return 0 <= x && x < h && 0 <= y && y < w;
}

struct Input {
    int n;
    vvi initial_board;

    // vertical_walls[r][c]: (r, c) と (r, c + 1) の間の壁
    vs vertical_walls;
    // horizontal_walls[r][c]: (r, c) と (r + 1, c) の間の壁
    vs horizontal_walls;

    void read() {
        scan(n);

        initial_board.assign(n, vi(n));
        for (auto& row : initial_board) {
            scan(row);
        }

        vertical_walls.resize(n);
        scan(vertical_walls);

        horizontal_walls.resize(n - 1);
        scan(horizontal_walls);
    }
};

struct Operation {
    char direction;
    int r;
    int c;
    int h;
    int w;
};

ostream& operator<<(ostream& os, const Operation& op) {
    return os << op.direction << ' ' << op.r << ' ' << op.c << ' '
              << op.h << ' ' << op.w;
}

class Solver {
    static constexpr int MAX_OPERATIONS = 100000;
    static constexpr int MAX_BEAM_ROUNDS = 10000;
    static constexpr int BEAM_WIDTH = 8;
    static constexpr int BEAM_BRANCHING = 3;
    static constexpr int BEAM_ELITE_COUNT = 1;
    static constexpr int BEAM_WORSENING_SLACK = 20;
    static constexpr double BEAM_TIME_LIMIT = 1.85;

    struct RouteStep {
        int from;
        int to;
        Operation operation;
    };

    struct SearchSolution {
        // stages[index]: BFS order[index] のカードを固定する操作列。
        vector<vector<RouteStep>> stages;
        // 各 stage を開始する直前の盤面。
        vector<vi> board_before_stage;
        // 全操作列における各 stage の開始位置。
        vi stage_begin;
        vector<Operation> operations;
    };

    const Input& input;
    int n;
    int vertex_count;

    vvi graph;
    vi initial_board;
    vi board;  // board[cell] = card
    vi pos;    // pos[card] = cell
    vector<char> active;
    vector<Operation> answer;
    mt19937 random_engine;
    chrono::steady_clock::time_point start_time = chrono::steady_clock::now();

    vvi vertical_wall_prefix;
    vvi horizontal_wall_prefix;

    // thin_operations[u * vertex_count + v]:
    // カードを u から v へ移す、高さ1または幅1の合法操作。
    vector<vector<Operation>> thin_operations;
    vvi move_graph;

    // 壁なし隣接グラフ上での全点対最短距離。
    vvi cell_distance;
    // 1回の合法長方形操作を辺とした、全点対の最短操作回数。
    vvi rectangle_operation_distance;
    // 固定BFS順の各stageにおける、targetからの長方形遷移距離。
    // 空なら未計算。active集合はstageだけで決まるためビーム探索中に再利用できる。
    vvi stage_target_distance_cache;

    int id(int r, int c) const {
        return r * n + c;
    }

    pair<int, int> cell(int v) const {
        return {v / n, v % n};
    }

    bool same_operation(const Operation& lhs, const Operation& rhs) const {
        return lhs.direction == rhs.direction
            && lhs.r == rhs.r
            && lhs.c == rhs.c
            && lhs.h == rhs.h
            && lhs.w == rhs.w;
    }

    double elapsed_seconds() const {
        return chrono::duration<double>(
            chrono::steady_clock::now() - start_time
        ).count();
    }

    uint64_t operation_signature(const vector<Operation>& operations) const {
        uint64_t hash = 1469598103934665603ULL;
        auto mix = [&](uint64_t value) {
            hash ^= value + 0x9e3779b97f4a7c15ULL + (hash << 6) + (hash >> 2);
            hash *= 1099511628211ULL;
        };

        mix(operations.size());
        for (const Operation& op : operations) {
            mix(static_cast<unsigned char>(op.direction));
            mix(static_cast<uint64_t>(op.r));
            mix(static_cast<uint64_t>(op.c));
            mix(static_cast<uint64_t>(op.h));
            mix(static_cast<uint64_t>(op.w));
        }
        return hash;
    }

    void add_edge(int u, int v) {
        graph[u].push_back(v);
        graph[v].push_back(u);
    }

    void build_graph() {
        graph.assign(vertex_count, {});

        for (int r = 0; r < n; ++r) {
            for (int c = 0; c + 1 < n; ++c) {
                if (input.vertical_walls[r][c] == '0') {
                    add_edge(id(r, c), id(r, c + 1));
                }
            }
        }

        for (int r = 0; r + 1 < n; ++r) {
            for (int c = 0; c < n; ++c) {
                if (input.horizontal_walls[r][c] == '0') {
                    add_edge(id(r, c), id(r + 1, c));
                }
            }
        }
    }

    vvi make_prefix_sum(const vvi& values) const {
        vvi prefix(n + 1, vi(n + 1, 0));
        for (int r = 0; r < n; ++r) {
            for (int c = 0; c < n; ++c) {
                prefix[r + 1][c + 1] = values[r][c]
                    + prefix[r][c + 1]
                    + prefix[r + 1][c]
                    - prefix[r][c];
            }
        }
        return prefix;
    }

    int rectangle_sum(const vvi& prefix, int r, int c, int h, int w) const {
        if (h <= 0 || w <= 0) return 0;
        return prefix[r + h][c + w]
            - prefix[r][c + w]
            - prefix[r + h][c]
            + prefix[r][c];
    }

    void build_wall_prefix() {
        vvi vertical(n, vi(n, 0));
        vvi horizontal(n, vi(n, 0));

        for (int r = 0; r < n; ++r) {
            for (int c = 0; c + 1 < n; ++c) {
                vertical[r][c] = input.vertical_walls[r][c] == '1';
            }
        }
        for (int r = 0; r + 1 < n; ++r) {
            for (int c = 0; c < n; ++c) {
                horizontal[r][c] = input.horizontal_walls[r][c] == '1';
            }
        }

        vertical_wall_prefix = make_prefix_sum(vertical);
        horizontal_wall_prefix = make_prefix_sum(horizontal);
    }

    bool rectangle_has_no_walls(const Operation& op) const {
        const int vertical_walls = rectangle_sum(
            vertical_wall_prefix, op.r, op.c, op.h, op.w - 1
        );
        const int horizontal_walls = rectangle_sum(
            horizontal_wall_prefix, op.r, op.c, op.h - 1, op.w
        );
        return vertical_walls == 0 && horizontal_walls == 0;
    }

    void add_thin_transition(int from, int to, const Operation& op) {
        auto& candidates = thin_operations[from * vertex_count + to];
        if (candidates.empty()) move_graph[from].push_back(to);
        candidates.push_back(op);
    }

    // 単一カードが1回の操作で移動可能な遷移を列挙する。
    // 横移動は高さ1、縦移動は幅1だけを保存すれば、遷移の存在判定には十分。
    void build_thin_transitions() {
        thin_operations.assign(vertex_count * vertex_count, {});
        move_graph.assign(vertex_count, {});

        for (int r = 0; r < n; ++r) {
            for (int half = 1; 2 * half <= n; ++half) {
                for (int c = 0; c + 2 * half <= n; ++c) {
                    const Operation op{'H', r, c, 1, 2 * half};
                    if (!rectangle_has_no_walls(op)) continue;

                    for (int offset = 0; offset < half; ++offset) {
                        const int left = id(r, c + offset);
                        const int right = id(r, c + half + offset);
                        add_thin_transition(left, right, op);
                        add_thin_transition(right, left, op);
                    }
                }
            }
        }

        for (int c = 0; c < n; ++c) {
            for (int half = 1; 2 * half <= n; ++half) {
                for (int r = 0; r + 2 * half <= n; ++r) {
                    const Operation op{'V', r, c, 2 * half, 1};
                    if (!rectangle_has_no_walls(op)) continue;

                    for (int offset = 0; offset < half; ++offset) {
                        const int top = id(r + offset, c);
                        const int bottom = id(r + half + offset, c);
                        add_thin_transition(top, bottom, op);
                        add_thin_transition(bottom, top, op);
                    }
                }
            }
        }
    }

    void build_cell_distances() {
        cell_distance.assign(vertex_count, vi(vertex_count, -1));

        for (int start = 0; start < vertex_count; ++start) {
            queue<int> que;
            cell_distance[start][start] = 0;
            que.push(start);

            while (!que.empty()) {
                const int v = que.front();
                que.pop();

                for (const int to : graph[v]) {
                    if (cell_distance[start][to] != -1) continue;
                    cell_distance[start][to] = cell_distance[start][v] + 1;
                    que.push(to);
                }
            }
        }
    }

    void build_rectangle_operation_distances() {
        rectangle_operation_distance.assign(
            vertex_count, vi(vertex_count, -1)
        );

        for (int start = 0; start < vertex_count; ++start) {
            queue<int> que;
            rectangle_operation_distance[start][start] = 0;
            que.push(start);

            while (!que.empty()) {
                const int v = que.front();
                que.pop();

                for (const int to : move_graph[v]) {
                    if (rectangle_operation_distance[start][to] != -1) continue;
                    rectangle_operation_distance[start][to]
                        = rectangle_operation_distance[start][v] + 1;
                    que.push(to);
                }
            }
        }
    }

    int choose_open_root() const {
        // 各合法長方形の寄与を、それを含む全マスへ長方形加算する。
        // 大きい操作を強く評価するため、面積の2乗を重みとする。
        vvl openness_difference(n + 1, vl(n + 1, 0));

        for (int r = 0; r < n; ++r) {
            for (int c = 0; c < n; ++c) {
                for (int h = 1; r + h <= n; ++h) {
                    for (int w = 1; c + w <= n; ++w) {
                        const int direction_count = (h % 2 == 0) + (w % 2 == 0);
                        if (direction_count == 0) continue;

                        const Operation rectangle{'H', r, c, h, w};
                        if (!rectangle_has_no_walls(rectangle)) continue;

                        const ll area = h * w;
                        const ll weight = direction_count * area * area;
                        openness_difference[r][c] += weight;
                        openness_difference[r + h][c] -= weight;
                        openness_difference[r][c + w] -= weight;
                        openness_difference[r + h][c + w] += weight;
                    }
                }
            }
        }

        vvl openness(n, vl(n, 0));
        for (int r = 0; r < n; ++r) {
            for (int c = 0; c < n; ++c) {
                openness[r][c] = openness_difference[r][c];
                if (r > 0) openness[r][c] += openness[r - 1][c];
                if (c > 0) openness[r][c] += openness[r][c - 1];
                if (r > 0 && c > 0) openness[r][c] -= openness[r - 1][c - 1];
            }
        }

        int best_root = -1;
        ll best_numerator = 0;
        ll best_denominator = 1;
        ll best_openness = -1;
        ll best_center_distance = numeric_limits<ll>::max();

        for (int v = 0; v < vertex_count; ++v) {
            const auto [r, c] = cell(v);
            ll distance_sum = 0;
            for (const int distance : cell_distance[v]) {
                distance_sum += distance;
            }

            const ll mobility = move_graph[v].size();
            const ll numerator = openness[r][c]
                * (vertex_count + 2 * mobility);
            const ll denominator = vertex_count + distance_sum;
            const ll center_r = 2 * r - (n - 1);
            const ll center_c = 2 * c - (n - 1);
            const ll center_distance = center_r * center_r + center_c * center_c;

            const bool better_ratio = best_root == -1
                || static_cast<__int128>(numerator) * best_denominator
                    > static_cast<__int128>(best_numerator) * denominator;
            const bool same_ratio = best_root != -1
                && static_cast<__int128>(numerator) * best_denominator
                    == static_cast<__int128>(best_numerator) * denominator;

            if (better_ratio
                || (same_ratio && openness[r][c] > best_openness)
                || (same_ratio && openness[r][c] == best_openness
                    && center_distance < best_center_distance)) {
                best_root = v;
                best_numerator = numerator;
                best_denominator = denominator;
                best_openness = openness[r][c];
                best_center_distance = center_distance;
            }
        }

        return best_root;
    }

    // root から BFS 木を作る。逆順に頂点を除くと、常に木の葉から除ける。
    vi make_bfs_order(int root) const {
        vi order;
        vb visited(vertex_count, false);
        queue<int> que;

        visited[root] = true;
        que.push(root);

        while (!que.empty()) {
            const int v = que.front();
            que.pop();
            order.push_back(v);

            for (const int to : graph[v]) {
                if (visited[to]) continue;
                visited[to] = true;
                que.push(to);
            }
        }

        if (static_cast<int>(order.size()) != vertex_count) {
            throw runtime_error("The open-cell graph is disconnected.");
        }
        return order;
    }

    vvi make_inactive_prefix() const {
        vvi inactive(n, vi(n, 0));
        for (int r = 0; r < n; ++r) {
            for (int c = 0; c < n; ++c) {
                inactive[r][c] = !active[id(r, c)];
            }
        }
        return make_prefix_sum(inactive);
    }

    bool rectangle_is_active(const Operation& op, const vvi& inactive_prefix) const {
        return rectangle_sum(inactive_prefix, op.r, op.c, op.h, op.w) == 0;
    }

    bool has_active_transition(int from, int to, const vvi& inactive_prefix) const {
        const auto& candidates = thin_operations[from * vertex_count + to];
        for (const Operation& op : candidates) {
            if (rectangle_is_active(op, inactive_prefix)) return true;
        }
        return false;
    }

    // 1回の合法長方形操作を辺とした、単一カード用の最短路を返す。
    vi find_rectangle_path(int start, int goal, const vvi& inactive_prefix) const {
        vi previous(vertex_count, -1);
        queue<int> que;

        previous[start] = start;
        que.push(start);

        while (!que.empty() && previous[goal] == -1) {
            const int v = que.front();
            que.pop();

            for (const int to : move_graph[v]) {
                if (previous[to] != -1) continue;
                if (!has_active_transition(v, to, inactive_prefix)) continue;
                previous[to] = v;
                que.push(to);
            }
        }

        if (previous[goal] == -1) {
            throw runtime_error("No path between active cells.");
        }

        vi path;
        for (int v = goal;; v = previous[v]) {
            path.push_back(v);
            if (v == start) break;
        }
        reverse(all(path));
        return path;
    }

    vi make_rectangle_distances(int goal, const vvi& inactive_prefix) const {
        vi distance(vertex_count, -1);
        queue<int> que;

        distance[goal] = 0;
        que.push(goal);
        while (!que.empty()) {
            const int v = que.front();
            que.pop();

            // 各長方形遷移は両方向へ登録されているため、goalから辿れる。
            for (const int to : move_graph[v]) {
                if (distance[to] != -1) continue;
                if (!has_active_transition(v, to, inactive_prefix)) continue;
                distance[to] = distance[v] + 1;
                que.push(to);
            }
        }
        return distance;
    }

    bool choose_alternative_next_cell(
        int from,
        int current_to,
        int target,
        int stage,
        const vvi& inactive_prefix,
        int& alternative_to
    ) {
        if (stage_target_distance_cache[stage].empty()) {
            stage_target_distance_cache[stage]
                = make_rectangle_distances(target, inactive_prefix);
        }
        const vi& distance = stage_target_distance_cache[stage];
        if (distance[from] <= 0
            || distance[current_to] == -1
            || distance[current_to] + 1 != distance[from]
            || !has_active_transition(from, current_to, inactive_prefix)) {
            throw runtime_error("A recorded route is not on a shortest-path DAG.");
        }

        vi candidates;
        for (const int to : move_graph[from]) {
            if (to == current_to) continue;
            if (distance[to] == -1 || distance[to] + 1 != distance[from]) continue;
            if (!has_active_transition(from, to, inactive_prefix)) continue;
            candidates.push_back(to);
        }

        if (candidates.empty()) return false;
        uniform_int_distribution<int> distribution(
            0, static_cast<int>(candidates.size()) - 1
        );
        alternative_to = candidates[distribution(random_engine)];
        return true;
    }

    ll swapped_pair_gain(int u, int v) const {
        const int card_u = board[u];
        const int card_v = board[v];
        const ll before = rectangle_operation_distance[u][card_u]
            + rectangle_operation_distance[v][card_v];
        const ll after = rectangle_operation_distance[v][card_u]
            + rectangle_operation_distance[u][card_v];
        return before - after;
    }

    Operation choose_best_operation(
        int from,
        int to,
        const vvi& inactive_prefix
    ) const {
        const auto& bases = thin_operations[from * vertex_count + to];
        bool found = false;
        ll best_gain = numeric_limits<ll>::lowest();
        int best_area = -1;
        Operation best{};

        auto consider = [&](const Operation& op, ll gain) {
            if (!rectangle_has_no_walls(op)) return;
            if (!rectangle_is_active(op, inactive_prefix)) return;

            const int area = op.h * op.w;
            if (!found || gain > best_gain || (gain == best_gain && area > best_area)) {
                found = true;
                best_gain = gain;
                best_area = area;
                best = op;
            }
        };

        for (const Operation& base : bases) {
            // base が active でなければ、それを含む拡張長方形も active ではない。
            if (!rectangle_is_active(base, inactive_prefix)) continue;

            if (base.direction == 'H') {
                const int token_row = cell(from).first;
                vl row_gain(n, 0);
                for (int r = 0; r < n; ++r) {
                    for (int offset = 0; offset < base.w / 2; ++offset) {
                        const int left = id(r, base.c + offset);
                        const int right = id(r, base.c + base.w / 2 + offset);
                        row_gain[r] += swapped_pair_gain(left, right);
                    }
                }

                vl gain_prefix(n + 1, 0);
                for (int r = 0; r < n; ++r) {
                    gain_prefix[r + 1] = gain_prefix[r] + row_gain[r];
                }

                for (int top = 0; top <= token_row; ++top) {
                    for (int bottom = token_row + 1; bottom <= n; ++bottom) {
                        const Operation op{
                            'H', top, base.c, bottom - top, base.w
                        };
                        consider(op, gain_prefix[bottom] - gain_prefix[top]);
                    }
                }
            } else {
                const int token_col = cell(from).second;
                vl col_gain(n, 0);
                for (int c = 0; c < n; ++c) {
                    for (int offset = 0; offset < base.h / 2; ++offset) {
                        const int top = id(base.r + offset, c);
                        const int bottom = id(base.r + base.h / 2 + offset, c);
                        col_gain[c] += swapped_pair_gain(top, bottom);
                    }
                }

                vl gain_prefix(n + 1, 0);
                for (int c = 0; c < n; ++c) {
                    gain_prefix[c + 1] = gain_prefix[c] + col_gain[c];
                }

                for (int left = 0; left <= token_col; ++left) {
                    for (int right = token_col + 1; right <= n; ++right) {
                        const Operation op{
                            'V', base.r, left, base.h, right - left
                        };
                        consider(op, gain_prefix[right] - gain_prefix[left]);
                    }
                }
            }
        }

        if (!found) {
            throw runtime_error("No active rectangle realizes the transition.");
        }
        return best;
    }

    bool choose_random_alternative(
        int from,
        int to,
        const Operation& current_operation,
        const vvi& inactive_prefix,
        Operation& alternative
    ) {
        const auto& bases = thin_operations[from * vertex_count + to];
        vector<Operation> candidates;

        auto consider = [&](const Operation& op) {
            if (same_operation(op, current_operation)) return;
            if (!rectangle_has_no_walls(op)) return;
            if (!rectangle_is_active(op, inactive_prefix)) return;
            candidates.push_back(op);
        };

        for (const Operation& base : bases) {
            if (!rectangle_is_active(base, inactive_prefix)) continue;

            if (base.direction == 'H') {
                const int token_row = cell(from).first;
                for (int top = 0; top <= token_row; ++top) {
                    for (int bottom = token_row + 1; bottom <= n; ++bottom) {
                        consider({'H', top, base.c, bottom - top, base.w});
                    }
                }
            } else {
                const int token_col = cell(from).second;
                for (int left = 0; left <= token_col; ++left) {
                    for (int right = token_col + 1; right <= n; ++right) {
                        consider({'V', base.r, left, base.h, right - left});
                    }
                }
            }
        }

        sort(candidates.begin(), candidates.end(), [](const Operation& lhs, const Operation& rhs) {
            return tie(lhs.direction, lhs.r, lhs.c, lhs.h, lhs.w)
                < tie(rhs.direction, rhs.r, rhs.c, rhs.h, rhs.w);
        });
        candidates.erase(
            unique(candidates.begin(), candidates.end(), [&](const Operation& lhs, const Operation& rhs) {
                return same_operation(lhs, rhs);
            }),
            candidates.end()
        );

        if (candidates.empty()) return false;
        uniform_int_distribution<int> distribution(
            0, static_cast<int>(candidates.size()) - 1
        );
        alternative = candidates[distribution(random_engine)];
        return true;
    }

    void swap_board_cells(int u, int v) {
        if (static_cast<int>(answer.size()) > MAX_OPERATIONS) {
            throw runtime_error("Too many operations.");
        }

        const int card_u = board[u];
        const int card_v = board[v];
        swap(board[u], board[v]);
        pos[card_u] = v;
        pos[card_v] = u;
    }

    void apply_operation(const Operation& op) {
        const bool in_board = 0 <= op.r && 0 <= op.c
            && op.r + op.h <= n && op.c + op.w <= n;
        const bool valid_direction =
            (op.direction == 'H' && op.w % 2 == 0)
            || (op.direction == 'V' && op.h % 2 == 0);
        if (!in_board || op.h <= 0 || op.w <= 0 || !valid_direction) {
            throw runtime_error("Invalid rectangle operation.");
        }
        if (!rectangle_has_no_walls(op)) {
            throw runtime_error("A rectangle operation crosses a wall.");
        }

        for (int r = op.r; r < op.r + op.h; ++r) {
            for (int c = op.c; c < op.c + op.w; ++c) {
                if (!active[id(r, c)]) {
                    throw runtime_error("A rectangle operation touches an inactive cell.");
                }
            }
        }

        answer.push_back(op);
        if (static_cast<int>(answer.size()) > MAX_OPERATIONS) {
            throw runtime_error("Too many operations.");
        }

        if (op.direction == 'H') {
            for (int r = 0; r < op.h; ++r) {
                for (int c = 0; c < op.w / 2; ++c) {
                    const int left = id(op.r + r, op.c + c);
                    const int right = id(op.r + r, op.c + op.w / 2 + c);
                    swap_board_cells(left, right);
                }
            }
        } else {
            for (int r = 0; r < op.h / 2; ++r) {
                for (int c = 0; c < op.w; ++c) {
                    const int top = id(op.r + r, op.c + c);
                    const int bottom = id(op.r + op.h / 2 + r, op.c + c);
                    swap_board_cells(top, bottom);
                }
            }
        }
    }

    void rebuild_positions() {
        pos.assign(vertex_count, -1);
        for (int v = 0; v < vertex_count; ++v) {
            pos[board[v]] = v;
        }
    }

    void restore_initial_state() {
        board = initial_board;
        rebuild_positions();
        active.assign(vertex_count, true);
        answer.clear();
    }

    void restore_stage_state(
        const SearchSolution& solution,
        const vi& order,
        int stage
    ) {
        board = solution.board_before_stage[stage];
        rebuild_positions();

        active.assign(vertex_count, true);
        for (int index = vertex_count - 1; index > stage; --index) {
            active[order[index]] = false;
        }

        const int prefix_size = solution.stage_begin[stage];
        answer.assign(
            solution.operations.begin(),
            solution.operations.begin() + prefix_size
        );
    }

    bool finish_target(
        const vi& order,
        int stage,
        vector<RouteStep>& steps,
        size_t operation_limit,
        bool observe_time_limit
    ) {
        if (observe_time_limit && elapsed_seconds() >= BEAM_TIME_LIMIT) return false;

        const int target = order[stage];
        const int source = pos[target];
        if (!active[source]) {
            throw runtime_error("A target card is in an inactive cell.");
        }

        const vvi inactive_prefix = make_inactive_prefix();
        const vi path = find_rectangle_path(source, target, inactive_prefix);
        for (int i = 0; i + 1 < static_cast<int>(path.size()); ++i) {
            if (observe_time_limit && elapsed_seconds() >= BEAM_TIME_LIMIT) {
                return false;
            }
            if (answer.size() >= operation_limit) return false;
            if (pos[target] != path[i]) {
                throw runtime_error("The routed card is at an unexpected cell.");
            }

            const Operation op = choose_best_operation(
                path[i], path[i + 1], inactive_prefix
            );
            apply_operation(op);
            steps.push_back({path[i], path[i + 1], op});

            if (pos[target] != path[i + 1]) {
                throw runtime_error("A rectangle did not realize its transition.");
            }
        }

        if (board[target] != target) {
            throw runtime_error("Failed to place a target card.");
        }
        active[target] = false;
        return true;
    }

    SearchSolution make_initial_solution(const vi& order) {
        restore_initial_state();

        SearchSolution solution;
        solution.stages.resize(vertex_count);
        solution.board_before_stage.resize(vertex_count);
        solution.stage_begin.assign(vertex_count, 0);

        for (int stage = vertex_count - 1; stage >= 1; --stage) {
            solution.stage_begin[stage] = answer.size();
            solution.board_before_stage[stage] = board;
            if (!finish_target(
                order,
                stage,
                solution.stages[stage],
                MAX_OPERATIONS,
                false
            )) {
                throw runtime_error("The initial solution exceeds the operation limit.");
            }
        }

        const int root = order.front();
        if (board[root] != root) {
            throw runtime_error("The last card is incorrect.");
        }
        solution.stage_begin[0] = answer.size();
        solution.board_before_stage[0] = board;
        solution.operations = answer;
        return solution;
    }

    bool make_neighbor(
        const SearchSolution& current,
        const vi& order,
        size_t operation_limit,
        SearchSolution& candidate
    ) {
        if (current.operations.empty()) return false;
        if (elapsed_seconds() >= BEAM_TIME_LIMIT) return false;

        // 半分は全体、半分は後半から選び、長い近傍と安い近傍を両方試す。
        int first_step = 0;
        if ((random_engine() & 1U) != 0) {
            first_step = static_cast<int>(current.operations.size() / 2);
        }
        uniform_int_distribution<int> step_distribution(
            first_step, static_cast<int>(current.operations.size()) - 1
        );
        const int global_step = step_distribution(random_engine);

        int selected_stage = -1;
        int selected_index = -1;
        for (int stage = vertex_count - 1; stage >= 1; --stage) {
            const int begin = current.stage_begin[stage];
            const int end = begin + current.stages[stage].size();
            if (begin <= global_step && global_step < end) {
                selected_stage = stage;
                selected_index = global_step - begin;
                break;
            }
        }
        if (selected_stage == -1) {
            throw runtime_error("The operation trace is inconsistent.");
        }

        restore_stage_state(current, order, selected_stage);
        const int target = order[selected_stage];
        const auto& selected_steps = current.stages[selected_stage];

        // completed stage の操作は answer にだけコピー済みで、盤面は checkpoint から復元済み。
        // 現在 stage 内の変更位置より前だけを盤面へ再適用する。
        for (int index = 0; index < selected_index; ++index) {
            if (elapsed_seconds() >= BEAM_TIME_LIMIT) return false;
            const RouteStep& step = selected_steps[index];
            if (pos[target] != step.from) {
                throw runtime_error("A replayed route starts at an unexpected cell.");
            }
            apply_operation(step.operation);
            if (pos[target] != step.to) {
                throw runtime_error("A replayed route ends at an unexpected cell.");
            }
        }

        const RouteStep& replaced_step = selected_steps[selected_index];
        if (pos[target] != replaced_step.from) {
            throw runtime_error("A mutated route starts at an unexpected cell.");
        }

        const vvi inactive_prefix = make_inactive_prefix();
        Operation alternative{};
        int alternative_to = replaced_step.to;

        auto try_path_change = [&]() {
            int next_cell = -1;
            if (!choose_alternative_next_cell(
                replaced_step.from,
                replaced_step.to,
                target,
                selected_stage,
                inactive_prefix,
                next_cell
            )) {
                return false;
            }
            alternative_to = next_cell;
            alternative = choose_best_operation(
                replaced_step.from, alternative_to, inactive_prefix
            );
            return true;
        };

        auto try_rectangle_change = [&]() {
            alternative_to = replaced_step.to;
            return choose_random_alternative(
                replaced_step.from,
                replaced_step.to,
                replaced_step.operation,
                inactive_prefix,
                alternative
            );
        };

        // 経路変更と長方形だけの変更を半々で試し、失敗時は他方へfallbackする。
        const bool prefer_path_change = (random_engine() & 1U) != 0;
        const bool found_alternative = prefer_path_change
            ? (try_path_change() || try_rectangle_change())
            : (try_rectangle_change() || try_path_change());
        if (!found_alternative) {
            return false;
        }
        if (elapsed_seconds() >= BEAM_TIME_LIMIT) return false;
        if (answer.size() >= operation_limit) return false;

        candidate.stages.resize(vertex_count);
        candidate.board_before_stage.resize(vertex_count);
        candidate.stage_begin.assign(vertex_count, 0);

        candidate.board_before_stage[selected_stage]
            = current.board_before_stage[selected_stage];
        candidate.stage_begin[selected_stage]
            = current.stage_begin[selected_stage];
        candidate.stages[selected_stage].assign(
            selected_steps.begin(),
            selected_steps.begin() + selected_index
        );

        apply_operation(alternative);
        candidate.stages[selected_stage].push_back({
            replaced_step.from, alternative_to, alternative
        });
        if (pos[target] != alternative_to) {
            throw runtime_error("A mutated route did not realize its transition.");
        }

        // 変更した操作以降は、変化後の盤面から貪欲に作り直す。
        if (!finish_target(
            order,
            selected_stage,
            candidate.stages[selected_stage],
            operation_limit,
            true
        )) {
            return false;
        }

        for (int stage = selected_stage - 1; stage >= 1; --stage) {
            if (elapsed_seconds() >= BEAM_TIME_LIMIT) return false;
            candidate.stage_begin[stage] = answer.size();
            candidate.board_before_stage[stage] = board;
            if (!finish_target(
                order,
                stage,
                candidate.stages[stage],
                operation_limit,
                true
            )) {
                return false;
            }
        }

        const int root = order.front();
        if (board[root] != root) {
            throw runtime_error("The last card of a candidate is incorrect.");
        }

        // 完成候補だけ、変更位置より前の不変なmetadataをコピーする。
        for (int stage = vertex_count - 1; stage > selected_stage; --stage) {
            candidate.stages[stage] = current.stages[stage];
            candidate.board_before_stage[stage] = current.board_before_stage[stage];
            candidate.stage_begin[stage] = current.stage_begin[stage];
        }
        candidate.stage_begin[0] = answer.size();
        candidate.board_before_stage[0] = board;
        candidate.operations = answer;
        return true;
    }

    bool reduce_adjacent_operations(
        const Operation& first,
        const Operation& second,
        vector<Operation>& replacement
    ) const {
        replacement.clear();
        if (first.direction != second.direction) return false;

        vector<char> affected(n, false);
        Operation merged{};
        if (first.direction == 'H') {
            if (first.c != second.c || first.w != second.w) return false;
            for (int r = first.r; r < first.r + first.h; ++r) {
                affected[r] ^= true;
            }
            for (int r = second.r; r < second.r + second.h; ++r) {
                affected[r] ^= true;
            }
            merged = {'H', 0, first.c, 0, first.w};
        } else {
            if (first.r != second.r || first.h != second.h) return false;
            for (int c = first.c; c < first.c + first.w; ++c) {
                affected[c] ^= true;
            }
            for (int c = second.c; c < second.c + second.w; ++c) {
                affected[c] ^= true;
            }
            merged = {'V', first.r, 0, first.h, 0};
        }

        int open_begin = -1;
        int segment_begin = -1;
        int segment_end = -1;
        int segment_count = 0;
        for (int index = 0; index <= n; ++index) {
            const bool current = index < n && affected[index];
            if (current && open_begin == -1) {
                open_begin = index;
            }
            if (!current && open_begin != -1) {
                ++segment_count;
                if (segment_count >= 2) return false;
                segment_begin = open_begin;
                segment_end = index;
                open_begin = -1;
            }
        }

        // 対称差が空なら同一作用が相殺される。
        if (segment_count == 0) return true;

        if (first.direction == 'H') {
            merged.r = segment_begin;
            merged.h = segment_end - segment_begin;
        } else {
            merged.c = segment_begin;
            merged.w = segment_end - segment_begin;
        }
        if (!rectangle_has_no_walls(merged)) return false;
        replacement.push_back(merged);
        return true;
    }

    vector<Operation> postprocess_operations(
        const vector<Operation>& operations
    ) const {
        vector<Operation> simplified;
        simplified.reserve(operations.size());
        vector<Operation> replacement;

        for (const Operation& op : operations) {
            simplified.push_back(op);
            while (simplified.size() >= 2) {
                const Operation second = simplified.back();
                const Operation first = simplified[simplified.size() - 2];
                if (!reduce_adjacent_operations(
                    first, second, replacement
                )) {
                    break;
                }

                simplified.pop_back();
                simplified.pop_back();
                for (const Operation& reduced : replacement) {
                    simplified.push_back(reduced);
                }
            }
        }
        return simplified;
    }

public:
    explicit Solver(const Input& input)
        : input(input), n(input.n), vertex_count(n * n) {
        build_graph();
        build_wall_prefix();
        build_thin_transitions();
        build_cell_distances();
        build_rectangle_operation_distances();

        board.resize(vertex_count);
        pos.resize(vertex_count);
        active.assign(vertex_count, true);
        answer.reserve(MAX_OPERATIONS);

        for (int r = 0; r < n; ++r) {
            for (int c = 0; c < n; ++c) {
                const int v = id(r, c);
                board[v] = input.initial_board[r][c];
                pos[board[v]] = v;
            }
        }
        initial_board = board;

        // 入力ごとに固定seedを使い、探索結果を再現可能にする。
        uint32_t seed = 0xA0682026u;
        for (const int card : initial_board) {
            seed = seed * 1664525u + static_cast<uint32_t>(card) + 1013904223u;
        }
        random_engine.seed(seed);
    }

    vector<Operation> solve() {
        // 大きな合法長方形を使いやすく、かつ全体から遠すぎないマスを残す。
        const int root = choose_open_root();
        const vi order = make_bfs_order(root);
        stage_target_distance_cache.assign(vertex_count, {});

        SearchSolution initial = make_initial_solution(order);
        vector<Operation> best_answer = initial.operations;
        vector<SearchSolution> beam;
        beam.push_back(std::move(initial));

        struct BeamCandidate {
            SearchSolution solution;
            uint64_t signature;
        };

        for (int round = 0; round < MAX_BEAM_ROUNDS; ++round) {
            if (elapsed_seconds() >= BEAM_TIME_LIMIT) break;

            vector<BeamCandidate> candidates;
            candidates.reserve(
                BEAM_ELITE_COUNT + static_cast<int>(beam.size()) * BEAM_BRANCHING
            );

            // 最良候補を残しつつ、残りの枠でその周辺を並列に探索する。
            const int elite_count = min<int>(BEAM_ELITE_COUNT, beam.size());
            for (int index = 0; index < elite_count; ++index) {
                candidates.push_back({
                    beam[index],
                    operation_signature(beam[index].operations)
                });
            }

            bool generated_child = false;
            for (const SearchSolution& parent : beam) {
                for (int branch = 0; branch < BEAM_BRANCHING; ++branch) {
                    if (elapsed_seconds() >= BEAM_TIME_LIMIT) break;

                    // 一時的な悪化を許し、貪欲法では越えられない谷も探索する。
                    const size_t relaxed_best_limit = min<size_t>(
                        MAX_OPERATIONS,
                        best_answer.size() + BEAM_WORSENING_SLACK
                    );
                    const size_t operation_limit = max(
                        parent.operations.size(),
                        relaxed_best_limit
                    );

                    SearchSolution child;
                    if (!make_neighbor(
                        parent,
                        order,
                        operation_limit,
                        child
                    )) {
                        continue;
                    }

                    generated_child = true;
                    if (child.operations.size() < best_answer.size()) {
                        best_answer = child.operations;
                    }
                    const uint64_t signature = operation_signature(child.operations);
                    candidates.push_back({std::move(child), signature});
                }
                if (elapsed_seconds() >= BEAM_TIME_LIMIT) break;
            }

            if (!generated_child) continue;

            sort(candidates.begin(), candidates.end(),
                [](const BeamCandidate& lhs, const BeamCandidate& rhs) {
                    if (lhs.solution.operations.size()
                        != rhs.solution.operations.size()) {
                        return lhs.solution.operations.size()
                            < rhs.solution.operations.size();
                    }
                    return lhs.signature < rhs.signature;
                }
            );

            vector<SearchSolution> next_beam;
            next_beam.reserve(BEAM_WIDTH);
            unordered_set<uint64_t> used_signatures;
            used_signatures.reserve(candidates.size() * 2 + 1);

            for (BeamCandidate& candidate : candidates) {
                if (!used_signatures.insert(candidate.signature).second) continue;
                next_beam.push_back(std::move(candidate.solution));
                if (static_cast<int>(next_beam.size()) >= BEAM_WIDTH) break;
            }

            if (next_beam.empty()) break;
            beam = std::move(next_beam);
        }

        return postprocess_operations(best_answer);
    }
};

int main(){
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    Input input;
    input.read();

    Solver solver(input);
    const vector<Operation> answer = solver.solve();

    // 操作数の先頭行は不要。操作を EOF まで1行ずつ出力する。
    prints(answer);
}
