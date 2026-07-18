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

    // active なマスだけを通る最短路を返す。
    vi find_active_path(int start, int goal) const {
        vi previous(vertex_count, -1);
        queue<int> que;

        previous[start] = start;
        que.push(start);

        while (!que.empty() && previous[goal] == -1) {
            const int v = que.front();
            que.pop();

            for (const int to : graph[v]) {
                if (!active[to] || previous[to] != -1) continue;
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

    // 壁のない隣接2マスの交換を、最小の長方形操作として追加する。
    void add_adjacent_swap(int u, int v) {
        const auto [r1, c1] = cell(u);
        const auto [r2, c2] = cell(v);

        if (r1 == r2 && abs(c1 - c2) == 1) {
            const int c = min(c1, c2);
            if (input.vertical_walls[r1][c] != '0') {
                throw runtime_error("Attempted to cross a vertical wall.");
            }
            answer.push_back({'H', r1, c, 1, 2});
        } else if (c1 == c2 && abs(r1 - r2) == 1) {
            const int r = min(r1, r2);
            if (input.horizontal_walls[r][c1] != '0') {
                throw runtime_error("Attempted to cross a horizontal wall.");
            }
            answer.push_back({'V', r, c1, 2, 1});
        } else {
            throw runtime_error("Attempted to swap non-adjacent cells.");
        }

        if (static_cast<int>(answer.size()) > MAX_OPERATIONS) {
            throw runtime_error("Too many operations.");
        }

        const int card_u = board[u];
        const int card_v = board[v];
        swap(board[u], board[v]);
        pos[card_u] = v;
        pos[card_v] = u;
    }

public:
    explicit Solver(const Input& input)
        : input(input), n(input.n), vertex_count(n * n) {
        build_graph();

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

            const vi path = find_active_path(source, target);
            for (int i = 0; i + 1 < static_cast<int>(path.size()); ++i) {
                add_adjacent_swap(path[i], path[i + 1]);
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
