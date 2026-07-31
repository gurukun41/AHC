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

using Cell = pair<int, int>;

struct Rect {
    int x;
    int y;
    int h;
    int w;
};

struct Shape {
    Rect main_rect;
    Rect extra_rect;
    int h;
    int w;
    int perimeter;
};

// P cells arranged as a rectangle plus, if necessary, one partial row/column.
// Only the minimum-perimeter shapes in this family are kept.
vector<Shape> make_compact_shapes(int p, int n) {
    vector<Shape> shapes;

    auto add_shape = [&](Rect main_rect, Rect extra_rect, int h, int w, int perimeter) {
        if (h <= n && w <= n) {
            shapes.push_back({main_rect, extra_rect, h, w, perimeter});
        }
    };

    for (int width = 1; width <= min(p, n); width++) {
        int full = p / width;
        int rem = p % width;

        if (rem == 0) {
            add_shape({0, 0, full, width}, {0, 0, 0, 0},
                      full, width, 2 * (full + width));
            continue;
        }

        int perimeter = 2 * (full + width) + 2;

        // full x width rectangle + a partial row above/below it.
        for (int below = 0; below < 2; below++) {
            for (int right = 0; right < 2; right++) {
                Rect main_rect{below ? 0 : 1, 0, full, width};
                Rect extra_rect{below ? full : 0, right ? width - rem : 0, 1, rem};
                add_shape(main_rect, extra_rect, full + 1, width, perimeter);
            }
        }

        // Transposes of the above: a partial column to the left/right.
        for (int right = 0; right < 2; right++) {
            for (int bottom = 0; bottom < 2; bottom++) {
                Rect main_rect{0, right ? 0 : 1, width, full};
                Rect extra_rect{bottom ? width - rem : 0, right ? full : 0, rem, 1};
                add_shape(main_rect, extra_rect, width, full + 1, perimeter);
            }
        }
    }

    int min_perimeter = numeric_limits<int>::max();
    for (const Shape &shape : shapes) {
        chmin(min_perimeter, shape.perimeter);
    }

    // BFS below remains a complete fallback, so retaining only the most compact
    // templates keeps the per-turn search small without affecting legality.
    constexpr int PERIMETER_MARGIN = 0;
    shapes.erase(remove_if(shapes.begin(), shapes.end(), [&](const Shape &shape) {
                     return shape.perimeter > min_perimeter + PERIMETER_MARGIN;
                 }),
                 shapes.end());

    auto key = [](const Shape &shape) {
        return array<int, 11>{
            shape.perimeter,
            shape.h,
            shape.w,
            shape.main_rect.x,
            shape.main_rect.y,
            shape.main_rect.h,
            shape.main_rect.w,
            shape.extra_rect.x,
            shape.extra_rect.y,
            shape.extra_rect.h,
            shape.extra_rect.w,
        };
    };
    sort(shapes.begin(), shapes.end(), [&](const Shape &lhs, const Shape &rhs) {
        return key(lhs) < key(rhs);
    });
    shapes.erase(unique(shapes.begin(), shapes.end(), [&](const Shape &lhs, const Shape &rhs) {
                     return key(lhs) == key(rhs);
                 }),
                 shapes.end());

    return shapes;
}

vector<vi> make_blocked_prefix(const vs &park, const vvi &owner) {
    int n = park.size();
    vector<vi> prefix(n + 1, vi(n + 1));
    for (int x = 0; x < n; x++) {
        for (int y = 0; y < n; y++) {
            int blocked = (park[x][y] == '#' || owner[x][y] != -1);
            prefix[x + 1][y + 1] = blocked + prefix[x][y + 1] + prefix[x + 1][y] - prefix[x][y];
        }
    }
    return prefix;
}

int rectangle_sum(const vector<vi> &prefix, int x, int y, int h, int w) {
    if (h == 0 || w == 0) return 0;
    return prefix[x + h][y + w] - prefix[x][y + w] - prefix[x + h][y] + prefix[x][y];
}

optional<vector<Cell>> find_compact_region(const vs &park, const vvi &owner, int p,
                                           const vector<Shape> &shapes) {
    int n = park.size();
    vector<vi> prefix = make_blocked_prefix(park, owner);

    for (const Shape &shape : shapes) {
        for (int base_x = 0; base_x + shape.h <= n; base_x++) {
            for (int base_y = 0; base_y + shape.w <= n; base_y++) {
                const Rect &a = shape.main_rect;
                const Rect &b = shape.extra_rect;
                if (rectangle_sum(prefix, base_x + a.x, base_y + a.y, a.h, a.w) != 0) continue;
                if (rectangle_sum(prefix, base_x + b.x, base_y + b.y, b.h, b.w) != 0) continue;

                vector<Cell> region;
                region.reserve(p);
                auto append_rectangle = [&](const Rect &rect) {
                    for (int dx = 0; dx < rect.h; dx++) {
                        for (int dy = 0; dy < rect.w; dy++) {
                            region.emplace_back(base_x + rect.x + dx, base_y + rect.y + dy);
                        }
                    }
                };
                append_rectangle(a);
                append_rectangle(b);
                return region;
            }
        }
    }
    return nullopt;
}

// If no compact template fits, inspect every free connected component.  The
// first p vertices popped by BFS are themselves connected, so this finds a
// legal region whenever a free component of size at least p exists.
optional<vector<Cell>> find_connected_region(const vs &park, const vvi &owner, int p) {
    int n = park.size();
    vvb visited(n, vb(n));
    constexpr int DX[4] = {-1, 1, 0, 0};
    constexpr int DY[4] = {0, 0, -1, 1};

    for (int start_x = 0; start_x < n; start_x++) {
        for (int start_y = 0; start_y < n; start_y++) {
            if (park[start_x][start_y] == '#' || owner[start_x][start_y] != -1 ||
                visited[start_x][start_y]) {
                continue;
            }

            queue<Cell> que;
            vector<Cell> region;
            visited[start_x][start_y] = true;
            que.emplace(start_x, start_y);

            while (!que.empty()) {
                auto [x, y] = que.front();
                que.pop();
                region.emplace_back(x, y);
                if ((int)region.size() == p) return region;

                for (int dir = 0; dir < 4; dir++) {
                    int nx = x + DX[dir];
                    int ny = y + DY[dir];
                    if (!inside(nx, ny, n, n) || visited[nx][ny]) continue;
                    if (park[nx][ny] == '#' || owner[nx][ny] != -1) continue;
                    visited[nx][ny] = true;
                    que.emplace(nx, ny);
                }
            }
        }
    }
    return nullopt;
}

optional<vector<Cell>> find_region(const vs &park, const vvi &owner, int p,
                                   const vector<Shape> &shapes) {
    optional<vector<Cell>> fallback = find_connected_region(park, owner, p);
    if (!fallback) return nullopt;
    if (auto region = find_compact_region(park, owner, p, shapes)) return region;
    return fallback;
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int N, M;
    ld R;
    scan(N, M, R);
    vs park(N);
    scan(park);

    vector<vector<Shape>> compact_shapes(151);
    for (int p = 4; p <= 150; p++) {
        compact_shapes[p] = make_compact_shapes(p, N);
    }

    vvi owner(N, vi(N, -1));
    vector<vector<Cell>> positions(M);
    vb active(M, false);
    priority_queue<pair<ll, int>, vector<pair<ll, int>>, greater<pair<ll, int>>> departures;

    for (int turn = 0; turn < M; turn++) {
        int i, P;
        ll S, T, V;
        scan(i, S, T, P, V);

        while (!departures.empty() && departures.top().first < S) {
            int j = departures.top().second;
            departures.pop();
            if (!active[j]) continue;
            for (auto [x, y] : positions[j]) {
                owner[x][y] = -1;
            }
            positions[j].clear();
            active[j] = false;
        }

        // Baseline policy: never move an active group.
        emit(0);

        optional<vector<Cell>> region = find_region(park, owner, P, compact_shapes[P]);
        if (!region) {
            emit("No");
            cout.flush();
            continue;
        }

        for (auto [x, y] : *region) {
            owner[x][y] = i;
        }
        positions[i] = *region;
        active[i] = true;
        departures.emplace(T, i);

        emit("Yes");
        for (const Cell &cell : *region) {
            emit(cell);
        }
        cout.flush();
    }

    return 0;
}
