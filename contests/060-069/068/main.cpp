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

struct Region {
    int r;
    int c;
    int h;
    int w;
    vi cells;
};

ostream& operator<<(ostream& os, const Operation& op) {
    return os << op.direction << ' ' << op.r << ' ' << op.c << ' '
              << op.h << ' ' << op.w;
}

class Solver {
    static constexpr int OUTPUT_OPERATION_LIMIT = 100000;
    static constexpr int CANDIDATE_OPERATION_LIMIT = 200000;
    static constexpr int OVERLAP_AFFINITY_BONUS = 4;

    struct RegionLink {
        int to;
        int here;
        int there;
    };

    const Input& input;
    int n;
    int vertex_count;

    vvi graph;
    vi initial_board;
    vi board;  // board[cell] = card
    vi pos;    // pos[card] = cell
    vi goal_cell;  // goal_cell[card] = 現在の段階での目標マス
    vector<char> active;
    vector<Operation> answer;

    vector<Region> regions;
    vi region_id;
    vvi rectangle_affinity;

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

    bool area_has_no_walls(int r, int c, int h, int w) const {
        const int vertical_walls = rectangle_sum(
            vertical_wall_prefix, r, c, h, w - 1
        );
        const int horizontal_walls = rectangle_sum(
            horizontal_wall_prefix, r, c, h - 1, w
        );
        return vertical_walls == 0 && horizontal_walls == 0;
    }

    bool rectangle_has_no_walls(const Operation& op) const {
        return area_has_no_walls(op.r, op.c, op.h, op.w);
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

    // 未割当部分から最大面積の壁なし長方形を繰り返し選び、
    // 盤面を互いに素な長方形領域へ分割する。
    void build_regions() {
        regions.clear();
        region_id.assign(vertex_count, -1);
        vector<char> assigned(vertex_count, false);
        int remaining = vertex_count;

        while (remaining > 0) {
            vvi assigned_values(n, vi(n, 0));
            for (int r = 0; r < n; ++r) {
                for (int c = 0; c < n; ++c) {
                    assigned_values[r][c] = assigned[id(r, c)];
                }
            }
            const vvi assigned_prefix = make_prefix_sum(assigned_values);

            Region best{};
            int best_area = 0;
            int best_short_side = -1;

            for (int top = 0; top < n; ++top) {
                for (int left = 0; left < n; ++left) {
                    for (int bottom = top + 1; bottom <= n; ++bottom) {
                        for (int right = left + 1; right <= n; ++right) {
                            const int h = bottom - top;
                            const int w = right - left;
                            const int area = h * w;
                            const int short_side = min(h, w);

                            if (area < best_area) continue;
                            if (area == best_area && short_side <= best_short_side) continue;
                            if (rectangle_sum(assigned_prefix, top, left, h, w) != 0) {
                                continue;
                            }
                            if (!area_has_no_walls(top, left, h, w)) continue;

                            best = {top, left, h, w, {}};
                            best_area = area;
                            best_short_side = short_side;
                        }
                    }
                }
            }

            if (best_area == 0) {
                throw runtime_error("Failed to partition the board into rectangles.");
            }

            const int rid = regions.size();
            for (int r = best.r; r < best.r + best.h; ++r) {
                for (int c = best.c; c < best.c + best.w; ++c) {
                    const int v = id(r, c);
                    assigned[v] = true;
                    region_id[v] = rid;
                    best.cells.push_back(v);
                }
            }
            remaining -= best_area;
            regions.push_back(std::move(best));
        }
    }

    // 重なりを許して、これ以上上下左右へ拡張できない壁なし長方形を列挙する。
    // affinity[u][v] は u と v が共通して属する最大長方形の個数。
    void build_overlapping_rectangle_affinity() {
        vector<Region> overlapping_rectangles;

        for (int top = 0; top < n; ++top) {
            for (int left = 0; left < n; ++left) {
                for (int bottom = top + 1; bottom <= n; ++bottom) {
                    for (int right = left + 1; right <= n; ++right) {
                        const int h = bottom - top;
                        const int w = right - left;
                        if (!area_has_no_walls(top, left, h, w)) continue;

                        bool maximal = true;
                        if (top > 0 && area_has_no_walls(top - 1, left, h + 1, w)) {
                            maximal = false;
                        }
                        if (bottom < n && area_has_no_walls(top, left, h + 1, w)) {
                            maximal = false;
                        }
                        if (left > 0 && area_has_no_walls(top, left - 1, h, w + 1)) {
                            maximal = false;
                        }
                        if (right < n && area_has_no_walls(top, left, h, w + 1)) {
                            maximal = false;
                        }
                        if (!maximal) continue;

                        Region region{top, left, h, w, {}};
                        for (int r = top; r < bottom; ++r) {
                            for (int c = left; c < right; ++c) {
                                region.cells.push_back(id(r, c));
                            }
                        }
                        overlapping_rectangles.push_back(std::move(region));
                    }
                }
            }
        }

        if (overlapping_rectangles.empty()) {
            throw runtime_error("No maximal wall-free rectangle was found.");
        }

        const int rectangle_count = overlapping_rectangles.size();
        const int word_count = (rectangle_count + 63) / 64;
        vector<vector<uint64_t>> membership(
            vertex_count, vector<uint64_t>(word_count, 0)
        );

        for (int rid = 0; rid < rectangle_count; ++rid) {
            for (const int v : overlapping_rectangles[rid].cells) {
                membership[v][rid / 64] |= 1ULL << (rid % 64);
            }
        }

        rectangle_affinity.assign(vertex_count, vi(vertex_count, 0));
        for (int u = 0; u < vertex_count; ++u) {
            for (int v = u; v < vertex_count; ++v) {
                int common = 0;
                for (int word = 0; word < word_count; ++word) {
                    common += __builtin_popcountll(
                        membership[u][word] & membership[v][word]
                    );
                }
                rectangle_affinity[u][v] = rectangle_affinity[v][u] = common;
            }
        }
    }

    // 領域グラフの全域木と、各領域内部のBFS木を接続したセル順序を作る。
    // 返す順序では、木の親は必ず子より前に現れる。
    vi make_hierarchical_order() const {
        const int region_count = regions.size();
        vector<vector<RegionLink>> region_graph(region_count);
        vvi linked(region_count, vi(region_count, 0));

        for (int u = 0; u < vertex_count; ++u) {
            for (const int v : graph[u]) {
                const int ru = region_id[u];
                const int rv = region_id[v];
                if (ru == rv || linked[ru][rv]) continue;

                linked[ru][rv] = linked[rv][ru] = 1;
                region_graph[ru].push_back({rv, u, v});
                region_graph[rv].push_back({ru, v, u});
            }
        }

        // 階層木全体も、root領域の角にあたる盤面左上から始める。
        const int root_cell = id(0, 0);
        const int root_region = region_id[root_cell];
        vi parent_region(region_count, -1);
        vi entry_cell(region_count, -1);
        vi parent_gateway(region_count, -1);
        vi region_order;
        queue<int> region_queue;

        parent_region[root_region] = root_region;
        entry_cell[root_region] = root_cell;
        region_queue.push(root_region);

        while (!region_queue.empty()) {
            const int rid = region_queue.front();
            region_queue.pop();
            region_order.push_back(rid);

            for (const RegionLink& link : region_graph[rid]) {
                if (parent_region[link.to] != -1) continue;
                parent_region[link.to] = rid;
                entry_cell[link.to] = link.there;
                parent_gateway[link.to] = link.here;
                region_queue.push(link.to);
            }
        }

        if (static_cast<int>(region_order.size()) != region_count) {
            throw runtime_error("The region graph is disconnected.");
        }

        vi order;
        vb added(vertex_count, false);
        for (const int rid : region_order) {
            const int entry = entry_cell[rid];
            if (rid != root_region && !added[parent_gateway[rid]]) {
                throw runtime_error("A parent gateway was not added first.");
            }

            queue<int> que;
            added[entry] = true;
            order.push_back(entry);
            que.push(entry);

            while (!que.empty()) {
                const int v = que.front();
                que.pop();

                for (const int to : graph[v]) {
                    if (region_id[to] != rid || added[to]) continue;
                    added[to] = true;
                    order.push_back(to);
                    que.push(to);
                }
            }
        }

        if (static_cast<int>(order.size()) != vertex_count) {
            throw runtime_error("A region is not internally connected.");
        }
        return order;
    }

    vi make_region_order(int rid) const {
        const Region& region = regions[rid];
        // active領域を角へ向かって残すため、長方形の左上角をrootにする。
        const int root = id(region.r, region.c);
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
                if (region_id[to] != rid || visited[to]) continue;
                visited[to] = true;
                que.push(to);
            }
        }

        if (order.size() != region.cells.size()) {
            throw runtime_error("A rectangular region is not connected.");
        }
        return order;
    }

    void assign_cards_min_cost(const vi& cards, const vi& cells, vi& desired) const {
        const int size = cards.size();
        if (size != static_cast<int>(cells.size())) {
            throw runtime_error("Region supply and demand do not match.");
        }
        if (size == 0) return;

        // Hungarian algorithm: cards[i] を cells[j] に割り当てる。
        vl row_potential(size + 1, 0);
        vl col_potential(size + 1, 0);
        vi matching(size + 1, 0);
        vi way(size + 1, 0);

        for (int i = 1; i <= size; ++i) {
            matching[0] = i;
            int column = 0;
            vl min_value(size + 1, numeric_limits<ll>::max());
            vb used(size + 1, false);

            do {
                used[column] = true;
                const int row = matching[column];
                ll delta = numeric_limits<ll>::max();
                int next_column = 0;

                for (int j = 1; j <= size; ++j) {
                    if (used[j]) continue;
                    const ll cost = cell_distance[pos[cards[row - 1]]][cells[j - 1]];
                    const ll value = cost - row_potential[row] - col_potential[j];
                    if (value < min_value[j]) {
                        min_value[j] = value;
                        way[j] = column;
                    }
                    if (min_value[j] < delta) {
                        delta = min_value[j];
                        next_column = j;
                    }
                }

                for (int j = 0; j <= size; ++j) {
                    if (used[j]) {
                        row_potential[matching[j]] += delta;
                        col_potential[j] -= delta;
                    } else {
                        min_value[j] -= delta;
                    }
                }
                column = next_column;
            } while (matching[column] != 0);

            do {
                const int previous_column = way[column];
                matching[column] = matching[previous_column];
                column = previous_column;
            } while (column != 0);
        }

        for (int j = 1; j <= size; ++j) {
            desired[cells[j - 1]] = cards[matching[j] - 1];
        }
    }

    // 各領域が、最終的にその領域へ属するカード集合を持つ一時目標盤面。
    vi make_region_goal() const {
        vi desired(vertex_count, -1);
        vb assigned_card(vertex_count, false);

        // 既に正しい領域内にいるカードは、その場に残す。
        for (int v = 0; v < vertex_count; ++v) {
            const int card = board[v];
            if (region_id[v] == region_id[card]) {
                desired[v] = card;
                assigned_card[card] = true;
            }
        }

        for (int rid = 0; rid < static_cast<int>(regions.size()); ++rid) {
            vi cells;
            vi cards;

            for (const int v : regions[rid].cells) {
                if (desired[v] == -1) cells.push_back(v);
            }
            for (int card = 0; card < vertex_count; ++card) {
                if (!assigned_card[card] && region_id[card] == rid) {
                    cards.push_back(card);
                }
            }

            assign_cards_min_cost(cards, cells, desired);
            for (const int card : cards) assigned_card[card] = true;
        }

        for (const int card : desired) {
            if (card == -1) throw runtime_error("The region goal is incomplete.");
        }
        return desired;
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
        const ll before = cell_distance[u][goal_cell[card_u]]
            + cell_distance[v][goal_cell[card_v]];
        const ll after = cell_distance[v][goal_cell[card_u]]
            + cell_distance[u][goal_cell[card_v]];

        const int affinity_before =
            rectangle_affinity[u][goal_cell[card_u]]
            + rectangle_affinity[v][goal_cell[card_v]];
        const int affinity_after =
            rectangle_affinity[v][goal_cell[card_u]]
            + rectangle_affinity[u][goal_cell[card_v]];

        return before - after
            + OVERLAP_AFFINITY_BONUS * (affinity_after - affinity_before);
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
        if (static_cast<int>(answer.size()) > CANDIDATE_OPERATION_LIMIT) {
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
        if (static_cast<int>(answer.size()) > CANDIDATE_OPERATION_LIMIT) {
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

    void set_goal_cells(const vi& desired) {
        if (static_cast<int>(desired.size()) != vertex_count) {
            throw runtime_error("Invalid goal board size.");
        }

        goal_cell.assign(vertex_count, -1);
        for (int v = 0; v < vertex_count; ++v) {
            const int card = desired[v];
            if (card < 0 || card >= vertex_count || goal_cell[card] != -1) {
                throw runtime_error("The goal board is not a permutation.");
            }
            goal_cell[card] = v;
        }
    }

    // order が表す木を葉から処理し、desired の配置へ揃える。
    void route_to_goal(
        const vi& desired,
        const vi& order,
        const vector<char>& initial_active
    ) {
        active = initial_active;
        set_goal_cells(desired);

        if (order.empty()) return;
        for (int index = static_cast<int>(order.size()) - 1; index >= 1; --index) {
            const int target_cell = order[index];
            const int target_card = desired[target_cell];
            const int source = pos[target_card];

            if (!active[target_cell] || !active[source]) {
                throw runtime_error("A phase target is outside the active region.");
            }

            const vvi inactive_prefix = make_inactive_prefix();
            const vi path = find_rectangle_path(source, target_cell, inactive_prefix);
            for (int i = 0; i + 1 < static_cast<int>(path.size()); ++i) {
                if (pos[target_card] != path[i]) {
                    throw runtime_error("The routed card is at an unexpected cell.");
                }

                const Operation op = choose_best_operation(
                    path[i], path[i + 1], inactive_prefix
                );
                apply_operation(op);

                if (pos[target_card] != path[i + 1]) {
                    throw runtime_error("A rectangle did not realize its transition.");
                }
            }

            if (board[target_cell] != target_card) {
                throw runtime_error("Failed to place a phase target card.");
            }
            active[target_cell] = false;
        }

        const int root = order.front();
        if (board[root] != desired[root]) {
            throw runtime_error("The phase root card is incorrect.");
        }
    }

    vi make_identity_goal() const {
        vi desired(vertex_count);
        iota(all(desired), 0);
        return desired;
    }

    void restore_initial_state() {
        board = initial_board;
        pos.assign(vertex_count, -1);
        for (int v = 0; v < vertex_count; ++v) pos[board[v]] = v;
        active.assign(vertex_count, true);
        answer.clear();
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
        goal_cell.resize(vertex_count);
        active.assign(vertex_count, true);
        answer.reserve(CANDIDATE_OPERATION_LIMIT);

        for (int r = 0; r < n; ++r) {
            for (int c = 0; c < n; ++c) {
                const int v = id(r, c);
                board[v] = input.initial_board[r][c];
                pos[board[v]] = v;
            }
        }
        initial_board = board;
    }

    vector<Operation> solve() {
        build_overlapping_rectangle_affinity();
        const vi identity_goal = make_identity_goal();

        // 領域は評価値にだけ使い、固定境界を設けず全盤面を一度で完成させる。
        const int root = id(n / 2, n / 2);
        route_to_goal(
            identity_goal,
            make_bfs_order(root),
            vector<char>(vertex_count, true)
        );

        if (static_cast<int>(answer.size()) > OUTPUT_OPERATION_LIMIT) {
            throw runtime_error("The final answer exceeds the operation limit.");
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
