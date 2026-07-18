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

    const Input& input;
    int n;
    int vertex_count;

    vvi graph;
    vi board;  // board[cell] = card
    vi pos;    // pos[card] = cell
    vector<char> active;
    vector<Operation> answer;

    vvi vertical_wall_prefix;
    vvi horizontal_wall_prefix;

    // thin_operations[u * vertex_count + v]:
    // カードを u から v へ移す、高さ1または幅1の合法操作。
    vector<vector<Operation>> thin_operations;
    vvi move_graph;

    // 壁なし隣接グラフ上での全点対最短距離。
    vvi cell_distance;

    int id(int r, int c) const {
        return r * n + c;
    }

    pair<int, int> cell(int v) const {
        return {v / n, v % n};
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

    ll swapped_pair_gain(int u, int v) const {
        const int card_u = board[u];
        const int card_v = board[v];
        const ll before = cell_distance[u][card_u] + cell_distance[v][card_v];
        const ll after = cell_distance[v][card_u] + cell_distance[u][card_v];
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

public:
    explicit Solver(const Input& input)
        : input(input), n(input.n), vertex_count(n * n) {
        build_graph();
        build_wall_prefix();
        build_thin_transitions();
        build_cell_distances();

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
    }

    vector<Operation> solve() {
        // 中央付近を最後まで残す根として固定する。
        const int root = id(n / 2, n / 2);
        const vi order = make_bfs_order(root);

        // BFS 順の逆順では target は残った全域木の葉になる。
        // target のカードを運んだ後にそのマスを固定し、以後触らない。
        for (int index = vertex_count - 1; index >= 1; --index) {
            const int target = order[index];
            const int source = pos[target];

            if (!active[source]) {
                throw runtime_error("A target card is in an inactive cell.");
            }

            const vvi inactive_prefix = make_inactive_prefix();
            const vi path = find_rectangle_path(source, target, inactive_prefix);
            for (int i = 0; i + 1 < static_cast<int>(path.size()); ++i) {
                if (pos[target] != path[i]) {
                    throw runtime_error("The routed card is at an unexpected cell.");
                }

                const Operation op = choose_best_operation(
                    path[i], path[i + 1], inactive_prefix
                );
                apply_operation(op);

                if (pos[target] != path[i + 1]) {
                    throw runtime_error("A rectangle did not realize its transition.");
                }
            }

            if (board[target] != target) {
                throw runtime_error("Failed to place a target card.");
            }
            active[target] = false;
        }

        if (board[root] != root) {
            throw runtime_error("The last card is incorrect.");
        }
        return answer;
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
