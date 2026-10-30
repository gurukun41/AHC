//https://github.com/gurukun41/ABC
//https://github.com/gurukun41/AHC
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

struct Emitter {
    static constexpr int floating_precision = 15;

    static constexpr bool is_separator(string_view s) {
        return s.empty() || s == " " || s == "\n";
    }

    template <typename T>
    static constexpr false_type container_tag(const T &) {
        return {};
    }

    template <typename T, typename U>
    static constexpr true_type container_tag(const pair<T, U> &) {
        return {};
    }

    template <typename T, size_t N>
    static constexpr true_type container_tag(const array<T, N> &) {
        return {};
    }

    template <typename T, typename Alloc>
    static constexpr true_type container_tag(const vector<T, Alloc> &) {
        return {};
    }

    template <typename T>
    auto write_value(const T &x, string_view sep, bool &first) const
        -> decltype((cout << x, void())) {
        if (!first) cout << sep;
        first = false;
        if constexpr (is_floating_point_v<decay_t<T>>) {
            auto flags = cout.flags();
            auto precision = cout.precision();
            cout << fixed << setprecision(floating_precision) << x;
            cout.flags(flags);
            cout.precision(precision);
        } else {
            cout << x;
        }
    }

    template <typename T, typename U>
    void write_value(const pair<T, U> &p, string_view sep, bool &first) const {
        write_value(p.first, sep, first);
        write_value(p.second, sep, first);
    }

    template <typename T, size_t N>
    void write_value(const array<T, N> &a, string_view sep, bool &first) const {
        for (const T &x : a) write_value(x, sep, first);
    }

    template <typename T, typename Alloc>
    void write_value(const vector<T, Alloc> &v, string_view sep, bool &first) const {
        for (const auto &x : v) write_value(x, sep, first);
    }

    template <typename Tuple, size_t... Is>
    void write_values(const Tuple &xs, string_view sep, string_view end, index_sequence<Is...>) const {
        bool first = true;
        (write_value(get<Is>(xs), sep, first), ...);
        cout << end;
    }

    template <typename T, typename... Ts>
    void operator()(const T &x, const Ts &...xs) const {
        auto values = tie(x, xs...);
        constexpr size_t count = sizeof...(Ts) + 1;
        constexpr bool first_is_container = decltype(container_tag(declval<const T &>()))::value;

        if constexpr (first_is_container && count == 2) {
            using Sep = decltype(get<1>(values));
            if constexpr (is_convertible_v<Sep, string_view>) {
                write_values(values, get<1>(values), "\n", make_index_sequence<1>{});
                return;
            }
        }

        if constexpr (first_is_container && count == 3) {
            using Sep = decltype(get<1>(values));
            using End = decltype(get<2>(values));
            if constexpr (is_convertible_v<Sep, string_view> && is_convertible_v<End, string_view>) {
                write_values(values, get<1>(values), get<2>(values), make_index_sequence<1>{});
                return;
            }
        }

        if constexpr (count >= 3) {
            using Last = decltype(get<count - 1>(values));
            if constexpr (is_convertible_v<Last, string_view>) {
                string_view last = get<count - 1>(values);
                if (is_separator(last)) {
                    write_values(values, last, "\n", make_index_sequence<count - 1>{});
                    return;
                }
            }
        }

        write_values(values, " ", "\n", make_index_sequence<count>{});
    }
};

inline constexpr Scanner scan{};
inline constexpr Emitter emit{};

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

template <typename Range>
void yns(const Range &xs) {
    for (const auto &x : xs) cout << (x ? "Yes\n" : "No\n");
}

void yn(bool a) {
    cout << (a ? "Yes\n" : "No\n");
}

bool inside(int x, int y, int h, int w) {
    return 0 <= x && x < h && 0 <= y && y < w;
}

namespace delivery {

using Clock = chrono::steady_clock;

enum Face { T, B, N, S, W, E };
constexpr array<char, 4> directions = {'U', 'D', 'L', 'R'};
constexpr array<int, 4> dr = {-1, 1, 0, 0};
constexpr array<int, 4> dc = {0, 0, -1, 1};
constexpr array<array<int, 6>, 4> rotation = {{
    {S, N, T, B, W, E},
    {N, S, B, T, W, E},
    {E, W, N, S, T, B},
    {W, E, N, S, B, T},
}};

struct State {
    int n;
    int row = 0;
    int column = 0;
    // N=16。探索中の状態コピーでマスごとの動的確保を行わない。
    array<uint8_t, 16 * 16> floor{};
    array<int, 6> orientation = {0, 1, 2, 3, 4, 5};
    array<int, 6> cargo{};

    State(int size, const vector<int> &initial_floor) : n(size) {
        assert(initial_floor.size() <= floor.size());
        copy(initial_floor.begin(), initial_floor.end(), floor.begin());
    }

    int position() const {
        return row * n + column;
    }

    bool roll(int direction) {
        int next_row = row + dr[direction];
        int next_column = column + dc[direction];
        if (!inside(next_row, next_column, n, n)) return false;
        auto previous = orientation;
        for (int face = 0; face < 6; ++face) {
            orientation[face] = previous[rotation[direction][face]];
        }
        row = next_row;
        column = next_column;
        int bottom = orientation[B];
        int cell = position();
        int on_floor = floor[cell];
        floor[cell] = uint8_t(cargo[bottom]);
        cargo[bottom] = on_floor;
        return true;
    }

    bool operator==(const State &other) const {
        return n == other.n && row == other.row && column == other.column &&
               orientation == other.orientation && cargo == other.cargo &&
               floor == other.floor;
    }
};

size_t state_hash(const State &state) {
    string_view bytes(reinterpret_cast<const char *>(state.floor.data()),
                      state.n * state.n);
    size_t value = hash<string_view>{}(bytes);
    auto combine = [&](int component) {
        value ^= size_t(component) + size_t(0x9e3779b97f4a7c15ULL)
                 + (value << 6) + (value >> 2);
    };
    combine(state.n);
    combine(state.row);
    combine(state.column);
    for (int face : state.orientation) combine(face);
    for (int parcel : state.cargo) combine(parcel);
    return value;
}

using LocalDistance = array<array<int, 17>, 17>;
// 単独の荷物を拾った直後から配送するBFSの結果。行・列の差は-8〜8。
// 中心の評価距離は指定により-1。
constexpr LocalDistance local_distance = {{
    {16, 15, 14, 13, 12, 11, 10, 11, 10, 11, 10, 11, 12, 13, 14, 15, 16},
    {15, 14, 13, 12, 11, 10, 9, 10, 9, 10, 9, 10, 11, 12, 13, 14, 15},
    {14, 13, 12, 11, 10, 9, 8, 9, 8, 9, 8, 9, 10, 11, 12, 13, 14},
    {13, 12, 11, 10, 9, 8, 7, 8, 7, 8, 7, 8, 9, 10, 11, 12, 13},
    {12, 11, 10, 9, 8, 7, 6, 5, 4, 5, 6, 7, 8, 9, 10, 11, 12},
    {11, 10, 9, 8, 7, 6, 5, 6, 5, 6, 5, 6, 7, 8, 9, 10, 11},
    {10, 9, 8, 7, 6, 5, 4, 5, 4, 5, 4, 5, 6, 7, 8, 9, 10},
    {11, 10, 9, 8, 5, 6, 5, 6, 3, 6, 5, 6, 5, 8, 9, 10, 11},
    {10, 9, 8, 7, 4, 5, 4, 3, -1, 3, 4, 5, 4, 7, 8, 9, 10},
    {11, 10, 9, 8, 5, 6, 5, 6, 3, 6, 5, 6, 5, 8, 9, 10, 11},
    {10, 9, 8, 7, 6, 5, 4, 5, 4, 5, 4, 5, 6, 7, 8, 9, 10},
    {11, 10, 9, 8, 7, 6, 5, 6, 5, 6, 5, 6, 7, 8, 9, 10, 11},
    {12, 11, 10, 9, 8, 7, 6, 5, 4, 5, 6, 7, 8, 9, 10, 11, 12},
    {13, 12, 11, 10, 9, 8, 7, 8, 7, 8, 7, 8, 9, 10, 11, 12, 13},
    {14, 13, 12, 11, 10, 9, 8, 9, 8, 9, 8, 9, 10, 11, 12, 13, 14},
    {15, 14, 13, 12, 11, 10, 9, 10, 9, 10, 9, 10, 11, 12, 13, 14, 15},
    {16, 15, 14, 13, 12, 11, 10, 11, 10, 11, 10, 11, 12, 13, 14, 15, 16},
}};

struct Evaluation {
    vector<vector<int>> distance;
    vector<vector<int>> floor_distance;
    vector<int> minimum_distance;
    vector<int> target;

    Evaluation(int n, int m, const vector<int> &goal)
        : distance(m + 1, vector<int>(n * n)),
          floor_distance(m + 1, vector<int>(n * n)),
          minimum_distance(m + 1, numeric_limits<int>::max()),
          target(goal) {
        for (int destination = 0; destination < n * n; ++destination) {
            int parcel = goal[destination];
            if (parcel == 0) continue;
            for (int cell = 0; cell < n * n; ++cell) {
                int row_delta = destination / n - cell / n;
                int column_delta = destination % n - cell % n;
                int manhattan = abs(row_delta) + abs(column_delta);
                distance[parcel][cell] = manhattan;
                floor_distance[parcel][cell] =
                    abs(row_delta) <= 8 && abs(column_delta) <= 8
                    ? local_distance[row_delta + 8][column_delta + 8] : manhattan;
            }
        }
    }

    int parcel_value(int parcel, int cell, bool carried) {
        int current = carried ? distance[parcel][cell] : floor_distance[parcel][cell];
        // 荷物ごとの過去最小距離は、探索全体を通して保持する。
        minimum_distance[parcel] = min(minimum_distance[parcel], current);
        return current + 2 * (current - minimum_distance[parcel]);
    }

    int operator()(const State &state) {
        int value = 0;
        for (int cell = 0; cell < state.n * state.n; ++cell) {
            int parcel = state.floor[cell];
            if (parcel != 0) value += parcel_value(parcel, cell, false);
        }
        constexpr array<int, 6> face_penalty = {2, 0, 1, 1, 1, 1};
        for (int face = 0; face < 6; ++face) {
            int parcel = state.cargo[state.orientation[face]];
            if (parcel != 0) {
                value += parcel_value(parcel, state.position(), true) + face_penalty[face];
            }
        }
        int delivered = 0;
        // 右下から行をさかのぼり、各行では右から左へ確認する。
        for (int cell = state.n * state.n - 1; cell >= 0; --cell) {
            if (state.floor[cell] != target[cell]) break;
            // 両方空のマスは、一致数に含めず読み飛ばす。
            if (target[cell] != 0) ++delivered;
        }
        // 左上からも、上の行を優先し、各行では左から右へ確認する。
        for (int cell = 0; cell < state.n * state.n; ++cell) {
            if (state.floor[cell] != target[cell]) break;
            if (target[cell] != 0) ++delivered;
        }
        // 右の列を優先し、各列では下から上へ確認する。
        for (int index = 0; index < state.n * state.n; ++index) {
            int column = state.n - 1 - index / state.n;
            int row = state.n - 1 - index % state.n;
            int cell = row * state.n + column;
            if (state.floor[cell] != target[cell]) break;
            if (target[cell] != 0) ++delivered;
        }
        // 左の列を優先し、各列では上から下へ確認する。
        for (int index = 0; index < state.n * state.n; ++index) {
            int column = index / state.n;
            int row = index % state.n;
            int cell = row * state.n + column;
            if (state.floor[cell] != target[cell]) break;
            if (target[cell] != 0) ++delivered;
        }
        return value - 100 * delivered;
    }
};

struct StateHash {
    size_t operator()(const State &state) const {
        return state_hash(state);
    }
};

using History = unordered_set<State, StateHash>;

bool finished(const State &state, const vector<int> &goal) {
    for (int cell = 0; cell < state.n * state.n; ++cell) {
        if (goal[cell] != 0 && state.floor[cell] != goal[cell]) return false;
    }
    return true;
}

struct BeamNode {
    State state;
    int value;
    int path_index;
};

struct PathStep {
    int parent;
    char direction;
};

vector<char> restore_path(const vector<PathStep> &paths, int index) {
    vector<char> operations;
    while (index != -1) {
        operations.push_back(paths[index].direction);
        index = paths[index].parent;
    }
    reverse(operations.begin(), operations.end());
    return operations;
}

vector<char> beam_search(int n, int m, const vector<int> &initial,
                          const vector<int> &goal, Clock::time_point deadline) {
    State state(n, initial);
    if (finished(state, goal)) return {};
    Evaluation evaluate(n, m, goal);
    History history;
    history.insert(state);
    vector<PathStep> paths;
    constexpr int beam_width = 32;
    constexpr int max_operations = 10000;
    vector<BeamNode> beam;
    beam.push_back({state, evaluate(state), -1});
    int best_value = beam.front().value;
    int best_path = -1;
    // 深さ10以降も同じビームを伸ばし、探索全体で最小の評価を保存する。
    for (int depth = 0; depth < max_operations; ++depth) {
        vector<BeamNode> candidates;
        candidates.reserve(beam_width * 4);
        for (const BeamNode &parent : beam) {
            for (int direction = 0; direction < 4; ++direction) {
                if (Clock::now() >= deadline) {
                    return restore_path(paths, best_path);
                }
                State next = parent.state;
                // 配送済みのマスへの移動も含め、盤面内の全方向を生成する。
                if (!next.roll(direction)) continue;
                // 全生成状態を枝刈り前に登録し、探索中は履歴を保持する。
                if (!history.insert(next).second) continue;
                int value = evaluate(next);
                int path_index = int(paths.size());
                paths.push_back({parent.path_index, directions[direction]});
                if (value < best_value) {
                    best_value = value;
                    best_path = path_index;
                }
                if (finished(next, goal)) return restore_path(paths, path_index);
                candidates.push_back({std::move(next), value, path_index});
            }
        }
        if (candidates.empty()) {
            return restore_path(paths, best_path);
        }
        // 同評価の場合は、生成された順序を維持する。
        stable_sort(candidates.begin(), candidates.end(),
                    [](const BeamNode &a, const BeamNode &b) {
                        return a.value < b.value;
                    });
        if (candidates.size() > beam_width) {
            candidates.erase(candidates.begin() + beam_width, candidates.end());
        }
        beam = std::move(candidates);
    }
    return restore_path(paths, best_path);
}

}  // namespace delivery

int main() {
    const auto deadline = delivery::Clock::now() + chrono::milliseconds(1900);
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int N, M;
    scan(N, M);
    vector<int> initial(N * N), goal(N * N);
    scan(initial, goal);

    auto operations = delivery::beam_search(N, M, initial, goal, deadline);
    for (char direction : operations) emit(direction);
    return 0;
}
