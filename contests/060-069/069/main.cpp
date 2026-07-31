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

struct Timer {
    chrono::steady_clock::time_point start = chrono::steady_clock::now();

    double elapsed() const {
        return chrono::duration<double>(chrono::steady_clock::now() - start).count();
    }
};

constexpr double FUTURE_EMERGENCY_TIME_LIMIT = 1.20;
constexpr double RELOCATION_TIME_LIMIT = 1.60;
constexpr bool ENABLE_ADMISSION_CONTROL = true;
constexpr long double ADMISSION_MAX_HISTORY_QUANTILE = 0.25L;
constexpr ll ARRIVAL_TIME_HORIZON = 100000;
constexpr int TIME_BUCKET_COUNT = 64;
constexpr int THETA_MIN = 2000;
constexpr int THETA_MAX = 8000;
constexpr int THETA_STEP = 100;
constexpr int THETA_QUADRATURE_STEPS = 48;
constexpr array<int, 8> FUTURE_SLOT_SIZES = {4, 9, 16, 25, 36, 64, 100, 150};
constexpr long double FUTURE_SHAPE_WEIGHT = 0.08L;
constexpr int COMPACT_PERIMETER_MARGIN = 4;
constexpr int PLACEMENT_GLOBAL_SHORTLIST = 3;
constexpr int PLACEMENT_SHORTLIST_LIMIT = 6;
constexpr int CONNECTED_GROWTH_SEED_LIMIT = 16;
constexpr int FUTURE_FIT_SNAPSHOT_COUNT = 3;
constexpr array<int, 8> FUTURE_FIT_SIDES = {2, 3, 4, 5, 6, 8, 10, 12};

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

struct GroupState {
    bool active = false;
    ll t = 0;
    ll v = 0;
    int p = 0;
    int max_perimeter = 0;
    vector<Cell> cells;
};

struct MovePlan {
    int id;
    vector<Cell> cells;
    int perimeter;
};

struct TurnPlan {
    vector<MovePlan> moves;
    optional<vector<Cell>> arrival;
    int arrival_perimeter = 0;
    ll immediate_gain = numeric_limits<ll>::min();
};

// P cells arranged as a rectangle plus, if necessary, one partial row/column.
// A short perimeter ladder is retained so that near-compact templates can be
// tried before falling back to an arbitrary connected polyomino.
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

    shapes.erase(remove_if(shapes.begin(), shapes.end(), [&](const Shape &shape) {
                     return shape.perimeter >
                         min_perimeter + COMPACT_PERIMETER_MARGIN;
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

vector<Cell> materialize_shape(const Shape &shape, int base_x, int base_y,
                               int p) {
    vector<Cell> region;
    region.reserve(p);
    auto append_rectangle = [&](const Rect &rect) {
        for (int dx = 0; dx < rect.h; dx++) {
            for (int dy = 0; dy < rect.w; dy++) {
                region.emplace_back(base_x + rect.x + dx,
                                    base_y + rect.y + dy);
            }
        }
    };
    append_rectangle(shape.main_rect);
    append_rectangle(shape.extra_rect);
    return region;
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

long double rectangle_sum(const vector<vector<long double>> &prefix,
                          int x, int y, int h, int w) {
    if (h == 0 || w == 0) return 0.0L;
    return prefix[x + h][y + w] - prefix[x][y + w] -
           prefix[x + h][y] + prefix[x][y];
}

optional<vector<Cell>> find_compact_region_ordered(
    const vs &park, const vvi &owner, int p, const vector<Shape> &shapes,
    bool reverse_x, bool reverse_y) {
    int n = park.size();
    vector<vi> prefix = make_blocked_prefix(park, owner);

    for (const Shape &shape : shapes) {
        int max_x = n - shape.h;
        int max_y = n - shape.w;
        for (int index_x = 0; index_x <= max_x; index_x++) {
            int base_x = reverse_x ? max_x - index_x : index_x;
            for (int index_y = 0; index_y <= max_y; index_y++) {
                int base_y = reverse_y ? max_y - index_y : index_y;
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

optional<vector<Cell>> find_compact_region(const vs &park, const vvi &owner, int p,
                                           const vector<Shape> &shapes) {
    return find_compact_region_ordered(park, owner, p, shapes, false, false);
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

bool same_region(vector<Cell> lhs, vector<Cell> rhs) {
    if (lhs.size() != rhs.size()) return false;
    sort(lhs.begin(), lhs.end());
    sort(rhs.begin(), rhs.end());
    return lhs == rhs;
}

vector<vector<Cell>> find_region_variants(const vs &park, const vvi &owner, int p,
                                          const vector<Shape> &shapes) {
    vector<vector<Cell>> regions;

    auto add_region = [&](optional<vector<Cell>> region) {
        if (!region) return;
        for (const vector<Cell> &existing : regions) {
            if (same_region(existing, *region)) return;
        }
        regions.push_back(std::move(*region));
    };

    for (int reverse_x = 0; reverse_x < 2; reverse_x++) {
        for (int reverse_y = 0; reverse_y < 2; reverse_y++) {
            add_region(find_compact_region_ordered(
                park, owner, p, shapes, reverse_x, reverse_y));
        }
    }
    add_region(find_connected_region(park, owner, p));
    return regions;
}

int calc_perimeter(const vector<Cell> &cells, int n) {
    vector<char> in_region(n * n, false);
    for (auto [x, y] : cells) {
        in_region[x * n + y] = true;
    }

    constexpr int DX[4] = {-1, 1, 0, 0};
    constexpr int DY[4] = {0, 0, -1, 1};
    int perimeter = 0;
    for (auto [x, y] : cells) {
        for (int dir = 0; dir < 4; dir++) {
            int nx = x + DX[dir];
            int ny = y + DY[dir];
            if (!inside(nx, ny, n, n) || !in_region[nx * n + ny]) {
                perimeter++;
            }
        }
    }
    return perimeter;
}

using i128 = __int128_t;

ll round_payment(ll v, int p, int perimeter) {
    if (perimeter <= 0) return 0;

    const i128 squared = (i128)64 * v * v * p;
    auto lower_ok = [&](ll payment) {
        const i128 value = ((i128)2 * payment - 1) * perimeter;
        return value <= 0 || value * value <= squared;
    };
    auto upper_ok = [&](ll payment) {
        const i128 value = ((i128)2 * payment + 1) * perimeter;
        return squared < value * value;
    };

    const long double approximate =
        (long double)v * 4.0L * sqrtl((long double)p) / (long double)perimeter;
    ll payment = max(0LL, (ll)floorl(approximate + 0.5L));
    while (!lower_ok(payment)) payment--;
    while (!upper_ok(payment)) payment++;
    return payment;
}

ll move_cost(ll v, int r_milli) {
    const i128 numerator = (i128)2 * v * r_milli + 1000;
    return max((ll)(numerator / 2000), 1LL);
}

void clear_cells(vvi &owner, const vector<Cell> &cells) {
    for (auto [x, y] : cells) {
        owner[x][y] = -1;
    }
}

void place_cells(vvi &owner, const vector<Cell> &cells, int id) {
    for (auto [x, y] : cells) {
        owner[x][y] = id;
    }
}

using FutureSlotWeights = array<long double, FUTURE_SLOT_SIZES.size()>;

FutureSlotWeights make_future_slot_weights(
    const vector<vector<Shape>> &compact_shapes) {
    FutureSlotWeights weights{};

    // P is generated by rounding x^2 for uniform x in [2, sqrt(150)].
    // Aggregate each exact P into the nearest representative slot size and
    // weight it by its probability mass and expected fee scale (roughly P).
    for (int p = 4; p <= 150; p++) {
        long double lower = sqrtl(max(4.0L, (long double)p - 0.5L));
        long double upper = sqrtl(min(150.0L, (long double)p + 0.5L));
        long double probability_mass = max(0.0L, upper - lower);

        int bucket = 0;
        for (int b = 1; b < (int)FUTURE_SLOT_SIZES.size(); b++) {
            if (fabsl(sqrtl((long double)p) -
                      sqrtl((long double)FUTURE_SLOT_SIZES[b])) <
                fabsl(sqrtl((long double)p) -
                      sqrtl((long double)FUTURE_SLOT_SIZES[bucket]))) {
                bucket = b;
            }
        }

        int best_perimeter = compact_shapes[p].front().perimeter;
        long double best_compactness =
            4.0L * sqrtl((long double)p) / best_perimeter;
        weights[bucket] += probability_mass * p * best_compactness;
    }

    long double total = accumulate(weights.begin(), weights.end(), 0.0L);
    for (long double &weight : weights) weight /= total;
    return weights;
}

struct FutureDsu {
    vi parent;
    vi component_size;
    array<int, FUTURE_SLOT_SIZES.size()> slot_counts{};
    int free_cells = 0;
    int free_edges = 0;

    explicit FutureDsu(int n) : parent(n, -1), component_size(n, 0) {}

    bool is_active(int v) const {
        return parent[v] != -1;
    }

    int leader(int v) {
        if (parent[v] == v) return v;
        return parent[v] = leader(parent[v]);
    }

    void activate(int v) {
        parent[v] = v;
        component_size[v] = 1;
        free_cells++;
    }

    void add_free_edge() {
        free_edges++;
    }

    void unite(int a, int b) {
        a = leader(a);
        b = leader(b);
        if (a == b) return;
        if (component_size[a] < component_size[b]) swap(a, b);

        for (int k = 0; k < (int)FUTURE_SLOT_SIZES.size(); k++) {
            int p = FUTURE_SLOT_SIZES[k];
            slot_counts[k] -= component_size[a] / p;
            slot_counts[k] -= component_size[b] / p;
        }
        parent[b] = a;
        component_size[a] += component_size[b];
        for (int k = 0; k < (int)FUTURE_SLOT_SIZES.size(); k++) {
            slot_counts[k] += component_size[a] / FUTURE_SLOT_SIZES[k];
        }
    }

    long double space_utility(const FutureSlotWeights &weights) const {
        long double utility = 0.0L;
        for (int k = 0; k < (int)FUTURE_SLOT_SIZES.size(); k++) {
            int slots = slot_counts[k];
            if (slots == 0) continue;
            long double capped_slot_value =
                1.0L + 0.25L * min(slots - 1, 2);
            utility += weights[k] * capped_slot_value;
        }

        if (free_cells > 0) {
            long double adjacency_density =
                (long double)free_edges / (2.0L * free_cells);
            utility += FUTURE_SHAPE_WEIGHT * adjacency_density;
        }
        return utility;
    }
};

long double evaluate_future_placement(
    const vs &park, const vvi &owner, const vector<GroupState> &groups,
    const vector<int> &active_by_departure, const vector<Cell> &candidate,
    const FutureSlotWeights &slot_weights,
    int arrival_id, ll current_s, ll arrival_t) {
    int remaining_groups = (int)groups.size() - arrival_id - 1;
    if (remaining_groups <= 0) return 0.0L;

    int n = park.size();
    vector<char> in_candidate(n * n, false);
    for (auto [x, y] : candidate) {
        in_candidate[x * n + y] = true;
    }

    FutureDsu dsu(n * n);
    constexpr int DX[4] = {-1, 1, 0, 0};
    constexpr int DY[4] = {0, 0, -1, 1};

    auto activate = [&](int cell) {
        int x = cell / n;
        int y = cell % n;
        dsu.activate(cell);
        for (int dir = 0; dir < 4; dir++) {
            int nx = x + DX[dir];
            int ny = y + DY[dir];
            if (!inside(nx, ny, n, n)) continue;
            int neighbor = nx * n + ny;
            if (dsu.is_active(neighbor)) {
                dsu.add_free_edge();
                dsu.unite(cell, neighbor);
            }
        }
    };

    // Initially activate the currently free cells except for the candidate,
    // which remains occupied until arrival_t.
    for (int x = 0; x < n; x++) {
        for (int y = 0; y < n; y++) {
            if (park[x][y] == '#' || owner[x][y] != -1) continue;
            int cell = x * n + y;
            if (!in_candidate[cell]) activate(cell);
        }
    }

    // Integrate usable-space value for the whole stay.  Arrival starts are
    // approximately uniform in time, so interval length is proportional to
    // the expected number of future arrival opportunities in that state.
    long double score = 0.0L;
    ll total_weight = 0;
    ll previous_time = current_s;
    long double utility = dsu.space_utility(slot_weights);

    for (int id : active_by_departure) {
        ll release_time = groups[id].t;
        if (release_time >= arrival_t) break;

        ll interval_weight = max(0LL, release_time - previous_time - 1);
        score += interval_weight * utility;
        total_weight += interval_weight;

        for (auto [x, y] : groups[id].cells) {
            activate(x * n + y);
        }
        previous_time = release_time;
        utility = dsu.space_utility(slot_weights);
    }

    ll interval_weight = max(0LL, arrival_t - previous_time - 1);
    score += interval_weight * utility;
    total_weight += interval_weight;
    return total_weight == 0 ? 0.0L : score / total_weight;
}

struct NormalPlacementChoice {
    vector<Cell> cells;
    int perimeter;
};

struct FutureDiagnostics {
    int attempts = 0;
    int evaluated_turns = 0;
    int candidates = 0;
    int emergency_stops = 0;
    int last_turn = -1;
    int changed_placements = 0;
};

struct AdmissionDiagnostics {
    int considered = 0;
    int density_comparisons = 0;
    int rejected = 0;
    ll rejected_fee = 0;
    ll rejected_cell_time = 0;
    long double rejected_density_sum = 0.0L;
    long double threshold_sum = 0.0L;
    long double dynamic_quantile_sum = 0.0L;
    long double expected_arrivals_sum = 0.0L;
};

long double empirical_quantile(vector<long double> values, long double quantile) {
    size_t index = (size_t)floorl(quantile * (values.size() - 1));
    nth_element(values.begin(), values.begin() + index, values.end());
    return values[index];
}

bool reject_by_admission_control(
    int arrival_id, int group_count, ll current_s, ll arrival_t, int p, ll v,
    int perimeter,
    const vector<long double> &observed_potential_densities,
    AdmissionDiagnostics &diagnostics) {
    diagnostics.considered++;
    if (!ENABLE_ADMISSION_CONTROL) return false;

    int remaining_groups = group_count - arrival_id - 1;
    if (remaining_groups <= 0 || current_s >= ARRIVAL_TIME_HORIZON) return false;
    long double expected_future_arrivals =
        (long double)remaining_groups * (arrival_t - current_s) /
        (ARRIVAL_TIME_HORIZON - current_s);
    long double dynamic_quantile = ADMISSION_MAX_HISTORY_QUANTILE *
        (-expm1l(-expected_future_arrivals));

    diagnostics.density_comparisons++;
    diagnostics.dynamic_quantile_sum += dynamic_quantile;
    diagnostics.expected_arrivals_sum += expected_future_arrivals;
    long double threshold = empirical_quantile(
        observed_potential_densities, dynamic_quantile);
    ll estimated_fee = round_payment(v, p, perimeter);
    ll cell_time = (ll)p * (arrival_t - current_s);
    long double candidate_density =
        (long double)estimated_fee / (long double)cell_time;
    if (candidate_density >= threshold) return false;

    diagnostics.rejected++;
    diagnostics.rejected_fee += estimated_fee;
    diagnostics.rejected_cell_time += cell_time;
    diagnostics.rejected_density_sum += candidate_density;
    diagnostics.threshold_sum += threshold;
    return true;
}

optional<NormalPlacementChoice> choose_demand_aware_region(
    const vs &park, const vvi &owner, const vector<GroupState> &groups,
    int arrival_id, ll current_s, ll arrival_t, int p, ll v,
    const vector<Shape> &shapes, const FutureSlotWeights &slot_weights,
    const Timer &timer, bool &future_disabled,
    FutureDiagnostics &diagnostics) {
    diagnostics.attempts++;
    vector<vector<Cell>> regions = find_region_variants(park, owner, p, shapes);
    if (regions.empty()) return nullopt;

    vector<int> perimeters(regions.size());
    vector<ll> payments(regions.size());
    ll best_payment = -1;
    int best_perimeter = numeric_limits<int>::max();
    for (int index = 0; index < (int)regions.size(); index++) {
        perimeters[index] = calc_perimeter(regions[index], park.size());
        payments[index] = round_payment(v, p, perimeters[index]);
        if (payments[index] > best_payment ||
            (payments[index] == best_payment && perimeters[index] < best_perimeter)) {
            best_payment = payments[index];
            best_perimeter = perimeters[index];
        }
    }

    vector<int> tied_indices;
    for (int index = 0; index < (int)regions.size(); index++) {
        if (payments[index] == best_payment && perimeters[index] == best_perimeter) {
            tied_indices.push_back(index);
        }
    }
    int direct_choice = tied_indices.front();
    if (tied_indices.size() == 1 || arrival_id + 1 == (int)groups.size()) {
        return NormalPlacementChoice{regions[direct_choice], best_perimeter};
    }

    auto trigger_emergency_stop = [&] {
        if (!future_disabled) {
            future_disabled = true;
            diagnostics.emergency_stops++;
        }
    };
    if (timer.elapsed() >= FUTURE_EMERGENCY_TIME_LIMIT) {
        trigger_emergency_stop();
        return NormalPlacementChoice{regions[direct_choice], best_perimeter};
    }

    vector<int> active_by_departure;
    active_by_departure.reserve(arrival_id);
    for (int id = 0; id < arrival_id; id++) {
        if (groups[id].active) active_by_departure.push_back(id);
    }
    sort(active_by_departure.begin(), active_by_departure.end(),
         [&](int lhs, int rhs) { return groups[lhs].t < groups[rhs].t; });

    int best_index = direct_choice;
    long double best_future_score = -1.0L;
    bool completed_all = true;
    for (int index : tied_indices) {
        if (timer.elapsed() >= FUTURE_EMERGENCY_TIME_LIMIT) {
            completed_all = false;
            break;
        }

        long double future_score = evaluate_future_placement(
            park, owner, groups, active_by_departure, regions[index], slot_weights,
            arrival_id, current_s, arrival_t);
        diagnostics.candidates++;
        if (future_score > best_future_score + 1e-12L) {
            best_index = index;
            best_future_score = future_score;
        }
    }

    if (!completed_all) {
        trigger_emergency_stop();
        return NormalPlacementChoice{regions[direct_choice], best_perimeter};
    }

    diagnostics.evaluated_turns++;
    diagnostics.last_turn = arrival_id;
    if (!same_region(regions[direct_choice], regions[best_index])) {
        diagnostics.changed_placements++;
    }
    if (timer.elapsed() >= FUTURE_EMERGENCY_TIME_LIMIT) {
        trigger_emergency_stop();
    }
    return NormalPlacementChoice{regions[best_index], best_perimeter};
}

// The hidden duration scale is shared by all groups in one test case.  Besides
// observed durations, the likelihood below uses the fact that every unseen
// group must start after the current order statistic S.
struct ThetaEstimator {
    int observed_count = 0;
    long double exponential_sample_sum = 0.0L;

    void observe(ll duration) {
        observed_count++;
        exponential_sample_sum += duration - 1;
    }

    long double start_survival(ll current_s, long double theta) const {
        if (current_s >= ARRIVAL_TIME_HORIZON - 1) return 0.0L;

        const long double horizon = ARRIVAL_TIME_HORIZON;
        const long double last_start_without_duration = horizon - 1.0L;
        const long double upper =
            (last_start_without_duration - current_s) / theta;
        // y = 1-exp(-x) absorbs the exponential measure.  Uniform steps in x
        // are inaccurate near S=0 because the integration range can be about
        // 50 while almost all probability mass is near x=0.
        const long double y_upper = -expm1l(-upper);
        const long double dy = y_upper / THETA_QUADRATURE_STEPS;
        long double integral = 0.0L;
        for (int k = 0; k < THETA_QUADRATURE_STEPS; k++) {
            long double y = (k + 0.5L) * dy;
            long double x = -log1pl(-y);
            long double numerator =
                last_start_without_duration - current_s - theta * x;
            long double denominator = horizon - theta * x;
            integral += numerator / denominator;
        }
        integral *= dy;
        long double normalizer = -expm1l(-horizon / theta);
        return clamp(integral / normalizer, 1e-300L, 1.0L);
    }

    long double estimate(ll current_s, int remaining_groups) const {
        constexpr int PARTICLE_COUNT =
            (THETA_MAX - THETA_MIN) / THETA_STEP + 1;
        array<long double, PARTICLE_COUNT> log_weights{};
        long double max_log_weight = -numeric_limits<long double>::infinity();

        for (int k = 0; k < PARTICLE_COUNT; k++) {
            long double theta = THETA_MIN + THETA_STEP * k;
            long double normalizer =
                -expm1l(-(long double)ARRIVAL_TIME_HORIZON / theta);
            long double log_weight =
                -observed_count * logl(theta) -
                exponential_sample_sum / theta -
                observed_count * logl(normalizer);
            if (remaining_groups > 0) {
                log_weight += remaining_groups *
                    logl(start_survival(current_s, theta));
            }
            log_weights[k] = log_weight;
            chmax(max_log_weight, log_weight);
        }

        long double weight_sum = 0.0L;
        long double theta_sum = 0.0L;
        for (int k = 0; k < PARTICLE_COUNT; k++) {
            long double weight = expl(log_weights[k] - max_log_weight);
            weight_sum += weight;
            theta_sum += weight * (THETA_MIN + THETA_STEP * k);
        }
        return theta_sum / weight_sum;
    }
};

long double inverse_standard_normal(long double probability) {
    // Peter J. Acklam's rational approximation.
    static constexpr array<long double, 6> A = {
        -3.969683028665376e+01L, 2.209460984245205e+02L,
        -2.759285104469687e+02L, 1.383577518672690e+02L,
        -3.066479806614716e+01L, 2.506628277459239e+00L,
    };
    static constexpr array<long double, 5> B = {
        -5.447609879822406e+01L, 1.615858368580409e+02L,
        -1.556989798598866e+02L, 6.680131188771972e+01L,
        -1.328068155288572e+01L,
    };
    static constexpr array<long double, 6> C = {
        -7.784894002430293e-03L, -3.223964580411365e-01L,
        -2.400758277161838e+00L, -2.549732539343734e+00L,
        4.374664141464968e+00L, 2.938163982698783e+00L,
    };
    static constexpr array<long double, 4> D = {
        7.784695709041462e-03L, 3.224671290700398e-01L,
        2.445134137142996e+00L, 3.754408661907416e+00L,
    };
    constexpr long double LOW = 0.02425L;
    constexpr long double HIGH = 1.0L - LOW;

    probability = clamp(probability, 1e-15L, 1.0L - 1e-15L);
    if (probability < LOW) {
        long double q = sqrtl(-2.0L * logl(probability));
        return (((((C[0] * q + C[1]) * q + C[2]) * q + C[3]) * q +
                  C[4]) * q + C[5]) /
               ((((D[0] * q + D[1]) * q + D[2]) * q + D[3]) * q + 1.0L);
    }
    if (probability > HIGH) {
        long double q = sqrtl(-2.0L * logl(1.0L - probability));
        return -(((((C[0] * q + C[1]) * q + C[2]) * q + C[3]) * q +
                   C[4]) * q + C[5]) /
               ((((D[0] * q + D[1]) * q + D[2]) * q + D[3]) * q + 1.0L);
    }

    long double q = probability - 0.5L;
    long double r = q * q;
    return (((((A[0] * r + A[1]) * r + A[2]) * r + A[3]) * r +
              A[4]) * r + A[5]) * q /
           (((((B[0] * r + B[1]) * r + B[2]) * r + B[3]) * r +
              B[4]) * r + 1.0L);
}

struct DensityModel {
    long double expected_group_size = 0.0L;
    long double mean_log2_compactness = 0.0L;
    long double variance_log2_compactness = 0.0L;
    long double base_log_density_variance = 0.0L;

    explicit DensityModel(const vector<vector<Shape>> &compact_shapes) {
        const long double sqrt_150 = sqrtl(150.0L);
        const long double denominator = sqrt_150 - 2.0L;
        long double size_weight_sum = 0.0L;
        long double weighted_log_sum = 0.0L;
        long double weighted_log_square_sum = 0.0L;

        for (int p = 4; p <= 150; p++) {
            long double lower = sqrtl(max(4.0L, (long double)p - 0.5L));
            long double upper = sqrtl(min(150.0L, (long double)p + 0.5L));
            long double probability = max(0.0L, upper - lower) / denominator;
            expected_group_size += probability * p;

            int perimeter = compact_shapes[p].front().perimeter;
            long double compactness =
                4.0L * sqrtl((long double)p) / perimeter;
            long double log_compactness = log2l(compactness);
            long double size_weight = probability * p;
            size_weight_sum += size_weight;
            weighted_log_sum += size_weight * log_compactness;
            weighted_log_square_sum +=
                size_weight * log_compactness * log_compactness;
        }

        mean_log2_compactness = weighted_log_sum / size_weight_sum;
        variance_log2_compactness =
            weighted_log_square_sum / size_weight_sum -
            mean_log2_compactness * mean_log2_compactness;

        base_log_density_variance =
            0.8L * 0.8L + variance_log2_compactness;
    }

    long double shadow_price(long double mean_log_duration,
                             long double variance_log_duration,
                             long double rejected_fraction) const {
        if (rejected_fraction <= 0.0L) return 0.0L;
        const long double ln2 = logl(2.0L);
        long double mean_log_density = mean_log2_compactness -
            0.1L * mean_log_duration / ln2;
        long double log_density_variance = base_log_density_variance +
            0.01L * variance_log_duration / (ln2 * ln2);
        long double quantile = min(rejected_fraction, 1.0L - 1e-9L);
        return exp2l(mean_log_density +
                     sqrtl(max(log_density_variance, 0.0L)) *
                     inverse_standard_normal(quantile));
    }
};

struct FutureBucketDemand {
    long double cell_time = 0.0L;
    long double mean_log_duration = 0.0L;
    long double variance_log_duration = 0.0L;
};

// Distribution of one unseen group conditional on its start being after S.
// The generator samples l from the exponential distribution, then uses stay
// duration D=l+1 and H-l possible integer start times.  The exponential
// density is absorbed by y=1-exp(-l/theta); its truncation normalizer cancels
// between the overlap integral and Q(S, theta).
struct ConditionalFutureDemand {
    struct Node {
        long double stay_duration;
        long double last_start;
        long double joint_weight;
        long double log_duration;
    };

    ll current_s;
    array<Node, THETA_QUADRATURE_STEPS> nodes{};
    long double remaining_start_measure = 0.0L;

    ConditionalFutureDemand(ll current_s_, long double theta)
        : current_s(current_s_) {
        const long double horizon = ARRIVAL_TIME_HORIZON;
        long double sampled_length_upper = horizon - 1.0L - current_s;
        long double y_upper = -expm1l(-sampled_length_upper / theta);
        long double dy = y_upper / THETA_QUADRATURE_STEPS;

        for (int k = 0; k < THETA_QUADRATURE_STEPS; k++) {
            long double y = (k + 0.5L) * dy;
            long double sampled_length = -theta * log1pl(-y);
            long double stay_duration = sampled_length + 1.0L;
            long double last_start = horizon - 1.0L - sampled_length;
            long double joint_weight = dy / (horizon - sampled_length);
            nodes[k] = {
                stay_duration,
                last_start,
                joint_weight,
                logl(stay_duration),
            };
            remaining_start_measure +=
                joint_weight * (last_start - current_s);
        }
    }

    long double future_start_cdf(long double time) const {
        if (remaining_start_measure <= 0.0L || time <= current_s) {
            return 0.0L;
        }
        long double measure = 0.0L;
        for (const Node &node : nodes) {
            long double available_length = max(
                0.0L, node.last_start - (long double)current_s);
            long double prefix_length = clamp(
                time - (long double)current_s, 0.0L, available_length);
            measure += node.joint_weight * prefix_length;
        }
        return clamp(measure / remaining_start_measure, 0.0L, 1.0L);
    }

    FutureBucketDemand in_bucket(
        long double a, long double c, int remaining_groups,
        long double expected_group_size) const {
        FutureBucketDemand result;
        if (remaining_groups <= 0 || remaining_start_measure <= 0.0L) {
            return result;
        }

        long double weight_sum = 0.0L;
        long double weighted_log_sum = 0.0L;
        long double weighted_log_square_sum = 0.0L;
        for (const Node &node : nodes) {
            auto integrated_positive_part = [&](long double boundary) {
                long double at_first =
                    max(0.0L, boundary - current_s);
                long double at_last =
                    max(0.0L, boundary - node.last_start);
                return 0.5L *
                    (at_first * at_first - at_last * at_last);
            };
            long double integrated_overlap =
                integrated_positive_part(c) -
                integrated_positive_part(a) -
                integrated_positive_part(c - node.stay_duration) +
                integrated_positive_part(a - node.stay_duration);
            integrated_overlap = max(0.0L, integrated_overlap);

            long double weight = node.joint_weight * integrated_overlap;
            weight_sum += weight;
            weighted_log_sum += weight * node.log_duration;
            weighted_log_square_sum +=
                weight * node.log_duration * node.log_duration;
        }
        if (weight_sum <= 0.0L) return result;

        result.cell_time = remaining_groups * expected_group_size *
            weight_sum / remaining_start_measure;
        result.mean_log_duration = weighted_log_sum / weight_sum;
        result.variance_log_duration = max(
            0.0L,
            weighted_log_square_sum / weight_sum -
                result.mean_log_duration * result.mean_log_duration);
        return result;
    }
};

struct ShadowEvaluation {
    long double opportunity_cost = 0.0L;
    long double duration_weighted_rejected_fraction = 0.0L;
    long double maximum_rejected_fraction = 0.0L;
    int priced_buckets = 0;
};

ShadowEvaluation evaluate_shadow_cost(
    const vector<GroupState> &groups, ll current_s, ll arrival_t, int p,
    int remaining_groups, int grass_cells, long double theta,
    const DensityModel &density_model) {
    ShadowEvaluation result;
    if (remaining_groups <= 0) return result;

    const long double horizon = ARRIVAL_TIME_HORIZON;
    long double total_candidate_duration = arrival_t - current_s;
    ConditionalFutureDemand future_demand(current_s, theta);

    for (int bucket = 0; bucket < TIME_BUCKET_COUNT; bucket++) {
        long double bucket_begin =
            horizon * bucket / TIME_BUCKET_COUNT;
        long double bucket_end =
            horizon * (bucket + 1) / TIME_BUCKET_COUNT;
        long double a = max((long double)current_s, bucket_begin);
        long double c = bucket_end;
        long double candidate_end = min((long double)arrival_t, c);
        long double candidate_overlap = max(0.0L, candidate_end - a);
        if (candidate_overlap <= 0.0L) continue;

        long double committed_cell_time = 0.0L;
        for (const GroupState &group : groups) {
            if (!group.active) continue;
            long double overlap =
                max(0.0L, min((long double)group.t, c) - a);
            committed_cell_time += group.p * overlap;
        }

        long double interval_length = c - a;
        long double capacity = grass_cells * interval_length;
        long double available_capacity =
            max(0.0L, capacity - committed_cell_time);
        FutureBucketDemand bucket_demand = future_demand.in_bucket(
            a, c, remaining_groups, density_model.expected_group_size);
        long double future_cell_time = bucket_demand.cell_time;

        long double rejected_fraction = 0.0L;
        if (future_cell_time > available_capacity) {
            rejected_fraction = clamp(
                1.0L - available_capacity / future_cell_time, 0.0L, 1.0L);
        }
        long double price = density_model.shadow_price(
            bucket_demand.mean_log_duration,
            bucket_demand.variance_log_duration,
            rejected_fraction);
        result.opportunity_cost += p * candidate_overlap * price;
        result.duration_weighted_rejected_fraction +=
            candidate_overlap * rejected_fraction / total_candidate_duration;
        chmax(result.maximum_rejected_fraction, rejected_fraction);
        if (rejected_fraction > 0.0L) result.priced_buckets++;
    }
    return result;
}

struct TemporalPlacementDiagnostics {
    int attempts = 0;
    int compact_successes = 0;
    int extended_template_successes = 0;
    int fallback_successes = 0;
    int future_fit_evaluated_turns = 0;
    int future_fit_changed_placements = 0;
    int incremental_changed_from_absolute = 0;
    int final_changed_from_absolute = 0;
    long long anchors_checked = 0;
    long long legal_compact_candidates = 0;
    long long connected_growth_candidates = 0;
    long long shortlisted_candidates = 0;
    long long future_fit_snapshots = 0;
};

enum class PlacementSource {
    MinimumTemplate,
    ExtendedTemplate,
    ConnectedGrowth,
};

struct PlacementCandidate {
    vector<Cell> cells;
    uint64_t region_hash = 0;
    int perimeter = 0;
    long double incremental_cost = 0.0L;
    long double absolute_cost = 0.0L;
    long long enumeration_order = 0;
    int quadrant = 0;
    PlacementSource source = PlacementSource::MinimumTemplate;
};

uint64_t placement_region_hash(const vector<Cell> &cells) {
    uint64_t hash = 0;
    for (auto [x, y] : cells) {
        uint64_t value = (uint64_t)(x * 64 + y + 1) +
                         0x9e3779b97f4a7c15ULL;
        value = (value ^ (value >> 30)) * 0xbf58476d1ce4e5b9ULL;
        value = (value ^ (value >> 27)) * 0x94d049bb133111ebULL;
        hash ^= value ^ (value >> 31);
    }
    return hash;
}

bool placement_increment_less(const PlacementCandidate &lhs,
                              const PlacementCandidate &rhs) {
    if (lhs.incremental_cost != rhs.incremental_cost) {
        return lhs.incremental_cost < rhs.incremental_cost;
    }
    if (lhs.absolute_cost != rhs.absolute_cost) {
        return lhs.absolute_cost < rhs.absolute_cost;
    }
    return lhs.enumeration_order < rhs.enumeration_order;
}

bool placement_absolute_less(const PlacementCandidate &lhs,
                             const PlacementCandidate &rhs) {
    if (lhs.absolute_cost != rhs.absolute_cost) {
        return lhs.absolute_cost < rhs.absolute_cost;
    }
    return placement_increment_less(lhs, rhs);
}

struct PlacementShortlistBuilder {
    int best_perimeter = numeric_limits<int>::max();
    vector<PlacementCandidate> global_best;
    optional<PlacementCandidate> absolute_best;
    optional<PlacementCandidate> first_candidate;
    array<optional<PlacementCandidate>, 4> quadrant_best;

    void reset(int perimeter) {
        best_perimeter = perimeter;
        global_best.clear();
        absolute_best.reset();
        first_candidate.reset();
        for (auto &candidate : quadrant_best) candidate.reset();
    }

    template <class Maker>
    void consider(int perimeter, long double incremental_cost,
                  long double absolute_cost, long long enumeration_order,
                  int quadrant, PlacementSource source, Maker &&maker) {
        if (perimeter < best_perimeter) reset(perimeter);
        if (perimeter > best_perimeter) return;

        optional<PlacementCandidate> cache;
        auto get_candidate = [&]() -> const PlacementCandidate & {
            if (!cache) {
                vector<Cell> cells = maker();
                uint64_t region_hash = placement_region_hash(cells);
                cache = PlacementCandidate{
                    std::move(cells), region_hash, perimeter,
                    incremental_cost, absolute_cost, enumeration_order,
                    quadrant, source};
            }
            return *cache;
        };

        if (!first_candidate) first_candidate = get_candidate();

        PlacementCandidate key_candidate{
            {}, 0, perimeter, incremental_cost, absolute_cost,
            enumeration_order, quadrant, source};
        if ((int)global_best.size() < PLACEMENT_GLOBAL_SHORTLIST ||
            placement_increment_less(key_candidate, global_best.back())) {
            const PlacementCandidate &full_candidate = get_candidate();
            bool duplicate = false;
            for (const PlacementCandidate &existing : global_best) {
                if (existing.region_hash == full_candidate.region_hash &&
                    same_region(existing.cells, full_candidate.cells)) {
                    duplicate = true;
                    break;
                }
            }
            if (!duplicate) {
                global_best.push_back(full_candidate);
                sort(global_best.begin(), global_best.end(),
                     placement_increment_less);
                if ((int)global_best.size() > PLACEMENT_GLOBAL_SHORTLIST) {
                    global_best.pop_back();
                }
            }
        }

        if (!absolute_best ||
            placement_absolute_less(key_candidate, *absolute_best)) {
            absolute_best = get_candidate();
        }
        if (!quadrant_best[quadrant] ||
            placement_increment_less(key_candidate,
                                     *quadrant_best[quadrant])) {
            quadrant_best[quadrant] = get_candidate();
        }
    }

    vector<PlacementCandidate> finalize() const {
        vector<PlacementCandidate> result;
        auto add = [&](const optional<PlacementCandidate> &candidate) {
            if (!candidate) return;
            for (const PlacementCandidate &existing : result) {
                if (existing.region_hash == candidate->region_hash &&
                    same_region(existing.cells, candidate->cells)) {
                    return;
                }
            }
            result.push_back(*candidate);
        };
        auto add_value = [&](const PlacementCandidate &candidate) {
            add(optional<PlacementCandidate>(candidate));
        };

        for (const PlacementCandidate &candidate : global_best) {
            add_value(candidate);
        }
        add(absolute_best);
        add(first_candidate);

        int primary_quadrant = global_best.empty() ? -1
                                                    : global_best.front().quadrant;
        optional<PlacementCandidate> diverse;
        for (int quadrant = 0; quadrant < 4; quadrant++) {
            if (quadrant == primary_quadrant || !quadrant_best[quadrant]) {
                continue;
            }
            if (!diverse || placement_increment_less(
                                *quadrant_best[quadrant], *diverse)) {
                diverse = quadrant_best[quadrant];
            }
        }
        add(diverse);
        if ((int)result.size() > PLACEMENT_SHORTLIST_LIMIT) {
            result.resize(PLACEMENT_SHORTLIST_LIMIT);
        }
        return result;
    }
};

vector<vector<Cell>> make_connected_growth_candidates(
    const vs &park, const vvi &owner, int p) {
    int n = park.size();
    constexpr int DX[4] = {-1, 1, 0, 0};
    constexpr int DY[4] = {0, 0, -1, 1};
    vector<vector<Cell>> candidates;

    auto add_candidate = [&](optional<vector<Cell>> candidate) {
        if (!candidate) return;
        for (const vector<Cell> &existing : candidates) {
            if (same_region(existing, *candidate)) return;
        }
        candidates.push_back(std::move(*candidate));
    };
    add_candidate(find_connected_region(park, owner, p));

    vvb visited(n, vb(n));
    vector<vector<Cell>> components;
    for (int start_x = 0; start_x < n; start_x++) {
        for (int start_y = 0; start_y < n; start_y++) {
            if (visited[start_x][start_y] || park[start_x][start_y] == '#' ||
                owner[start_x][start_y] != -1) {
                continue;
            }
            vector<Cell> component;
            queue<Cell> que;
            visited[start_x][start_y] = true;
            que.emplace(start_x, start_y);
            while (!que.empty()) {
                auto [x, y] = que.front();
                que.pop();
                component.emplace_back(x, y);
                for (int dir = 0; dir < 4; dir++) {
                    int nx = x + DX[dir];
                    int ny = y + DY[dir];
                    if (!inside(nx, ny, n, n) || visited[nx][ny]) continue;
                    if (park[nx][ny] == '#' || owner[nx][ny] != -1) continue;
                    visited[nx][ny] = true;
                    que.emplace(nx, ny);
                }
            }
            if ((int)component.size() >= p) {
                components.push_back(std::move(component));
            }
        }
    }
    vi component_order(components.size());
    iota(component_order.begin(), component_order.end(), 0);
    sort(component_order.begin(), component_order.end(),
         [&](int lhs, int rhs) {
             return components[lhs].size() > components[rhs].size();
         });

    const int INF_DISTANCE = n * n + 1;
    vvi obstacle_distance(n, vi(n, INF_DISTANCE));
    queue<Cell> distance_queue;
    for (int x = 0; x < n; x++) {
        for (int y = 0; y < n; y++) {
            if (park[x][y] == '#' || owner[x][y] != -1) {
                obstacle_distance[x][y] = 0;
                distance_queue.emplace(x, y);
            } else if (x == 0 || y == 0 || x + 1 == n || y + 1 == n) {
                obstacle_distance[x][y] = 1;
                distance_queue.emplace(x, y);
            }
        }
    }
    while (!distance_queue.empty()) {
        auto [x, y] = distance_queue.front();
        distance_queue.pop();
        for (int dir = 0; dir < 4; dir++) {
            int nx = x + DX[dir];
            int ny = y + DY[dir];
            if (!inside(nx, ny, n, n)) continue;
            if (obstacle_distance[nx][ny] <= obstacle_distance[x][y] + 1) {
                continue;
            }
            obstacle_distance[nx][ny] = obstacle_distance[x][y] + 1;
            distance_queue.emplace(nx, ny);
        }
    }

    constexpr int SEED_FEATURE_COUNT = 10;
    vector<array<Cell, SEED_FEATURE_COUNT>> feature_seeds(components.size());
    auto feature_key = [&](int feature, const Cell &cell) {
        auto [x, y] = cell;
        switch (feature) {
            case 0: return pair<int, int>{x, y};
            case 1: return pair<int, int>{-x, y};
            case 2: return pair<int, int>{y, x};
            case 3: return pair<int, int>{-y, x};
            case 4: return pair<int, int>{x + y, x};
            case 5: return pair<int, int>{-(x + y), x};
            case 6: return pair<int, int>{x - y, x};
            case 7: return pair<int, int>{-(x - y), x};
            case 8: return pair<int, int>{obstacle_distance[x][y], x * n + y};
            default: return pair<int, int>{-obstacle_distance[x][y], x * n + y};
        }
    };
    for (int component_id = 0;
         component_id < (int)components.size(); component_id++) {
        for (int feature = 0; feature < SEED_FEATURE_COUNT; feature++) {
            Cell best = components[component_id].front();
            for (const Cell &cell : components[component_id]) {
                if (feature_key(feature, cell) < feature_key(feature, best)) {
                    best = cell;
                }
            }
            feature_seeds[component_id][feature] = best;
        }
    }

    struct Seed {
        Cell cell;
        int bias;
    };
    vector<Seed> seeds;
    set<Cell> used_seeds;
    for (int feature = 0; feature < SEED_FEATURE_COUNT &&
         (int)seeds.size() < CONNECTED_GROWTH_SEED_LIMIT; feature++) {
        for (int order_index = 0;
             order_index < (int)component_order.size() &&
             (int)seeds.size() < CONNECTED_GROWTH_SEED_LIMIT;
             order_index++) {
            int component_id = component_order[order_index];
            Cell seed = feature_seeds[component_id][feature];
            if (used_seeds.insert(seed).second) {
                seeds.push_back({seed, feature % 4});
            }
        }
    }

    struct GrowthEntry {
        int cell;
        int selected_neighbors;
        int distance;
        int bias_key;
    };
    auto entry_worse = [](const GrowthEntry &lhs, const GrowthEntry &rhs) {
        return tuple(lhs.selected_neighbors, -lhs.distance, -lhs.bias_key,
                     -lhs.cell) <
               tuple(rhs.selected_neighbors, -rhs.distance, -rhs.bias_key,
                     -rhs.cell);
    };

    for (const Seed &seed_info : seeds) {
        int seed_x = seed_info.cell.first;
        int seed_y = seed_info.cell.second;
        vector<char> selected(n * n, false);
        vector<Cell> region;
        region.reserve(p);
        priority_queue<GrowthEntry, vector<GrowthEntry>,
                       decltype(entry_worse)> frontier(entry_worse);

        auto count_selected_neighbors = [&](int cell) {
            int x = cell / n;
            int y = cell % n;
            int count = 0;
            for (int dir = 0; dir < 4; dir++) {
                int nx = x + DX[dir];
                int ny = y + DY[dir];
                if (inside(nx, ny, n, n) && selected[nx * n + ny]) count++;
            }
            return count;
        };
        auto bias_key = [&](int x, int y) {
            if (seed_info.bias == 0) return x * n + y;
            if (seed_info.bias == 1) return x * n + (n - 1 - y);
            if (seed_info.bias == 2) return (n - 1 - x) * n + y;
            return (n - 1 - x) * n + (n - 1 - y);
        };
        auto push_frontier = [&](int x, int y) {
            if (!inside(x, y, n, n) || park[x][y] == '#' ||
                owner[x][y] != -1 || selected[x * n + y]) {
                return;
            }
            int cell = x * n + y;
            frontier.push({cell, count_selected_neighbors(cell),
                           abs(x - seed_x) + abs(y - seed_y),
                           bias_key(x, y)});
        };
        auto select_cell = [&](int x, int y) {
            selected[x * n + y] = true;
            region.emplace_back(x, y);
            for (int dir = 0; dir < 4; dir++) {
                push_frontier(x + DX[dir], y + DY[dir]);
            }
        };

        select_cell(seed_x, seed_y);
        while ((int)region.size() < p && !frontier.empty()) {
            GrowthEntry entry = frontier.top();
            frontier.pop();
            if (selected[entry.cell]) continue;
            int current_neighbors = count_selected_neighbors(entry.cell);
            if (current_neighbors != entry.selected_neighbors) {
                int x = entry.cell / n;
                int y = entry.cell % n;
                frontier.push({entry.cell, current_neighbors,
                               abs(x - seed_x) + abs(y - seed_y),
                               bias_key(x, y)});
                continue;
            }
            select_cell(entry.cell / n, entry.cell % n);
        }
        if ((int)region.size() == p) {
            add_candidate(optional<vector<Cell>>(std::move(region)));
        }
    }
    return candidates;
}

int placement_quadrant(const vector<Cell> &cells, int n) {
    long long sum_x = 0;
    long long sum_y = 0;
    for (auto [x, y] : cells) {
        sum_x += x;
        sum_y += y;
    }
    int lower_half = 2 * sum_x >= (long long)cells.size() * n;
    int right_half = 2 * sum_y >= (long long)cells.size() * n;
    return 2 * lower_half + right_half;
}

long double compact_fit_utility(
    const vs &park, const vvi &owner, const vector<GroupState> &groups,
    const vector<char> &in_candidate, ll snapshot_time) {
    int n = park.size();
    constexpr int MAX_SIDE = FUTURE_FIT_SIDES.back();
    array<int, MAX_SIDE + 2> histogram{};
    vector<int> previous(n + 1), current(n + 1);

    for (int x = 0; x < n; x++) {
        fill(current.begin(), current.end(), 0);
        for (int y = 0; y < n; y++) {
            int cell = x * n + y;
            int occupied_by = owner[x][y];
            bool is_free = park[x][y] == '#' ? false
                : !in_candidate[cell] &&
                  (occupied_by == -1 || groups[occupied_by].t < snapshot_time);
            if (!is_free) continue;
            current[y + 1] = 1 + min({previous[y + 1], current[y],
                                      previous[y]});
            histogram[min(current[y + 1], MAX_SIDE)]++;
        }
        swap(previous, current);
    }

    array<int, MAX_SIDE + 2> at_least{};
    for (int side = MAX_SIDE; side >= 1; side--) {
        at_least[side] = at_least[side + 1] + histogram[side];
    }
    long double weighted_utility = 0.0L;
    long double weight_sum = 0.0L;
    for (int side : FUTURE_FIT_SIDES) {
        long double weight = (long double)side * side;
        weighted_utility += weight * log1pl((long double)at_least[side]);
        weight_sum += weight;
    }
    return weighted_utility / weight_sum;
}

array<ll, FUTURE_FIT_SNAPSHOT_COUNT> make_future_fit_snapshots(
    const ConditionalFutureDemand &future_demand, ll current_s,
    ll arrival_t) {
    array<ll, FUTURE_FIT_SNAPSHOT_COUNT> snapshots{};
    long double total_mass = future_demand.future_start_cdf(arrival_t);
    for (int index = 0; index < FUTURE_FIT_SNAPSHOT_COUNT; index++) {
        long double fraction =
            (2.0L * index + 1.0L) / (2.0L * FUTURE_FIT_SNAPSHOT_COUNT);
        long double target = total_mass * fraction;
        ll low = current_s;
        ll high = arrival_t;
        while (high - low > 1) {
            ll middle = (low + high) / 2;
            if (future_demand.future_start_cdf(middle) >= target) {
                high = middle;
            } else {
                low = middle;
            }
        }
        snapshots[index] = high;
    }
    return snapshots;
}

long double evaluate_compact_fit(
    const vs &park, const vvi &owner, const vector<GroupState> &groups,
    const vector<Cell> &candidate,
    const array<ll, FUTURE_FIT_SNAPSHOT_COUNT> &snapshots) {
    int n = park.size();
    vector<char> in_candidate(n * n, false);
    for (auto [x, y] : candidate) in_candidate[x * n + y] = true;

    long double sum = 0.0L;
    long double minimum = numeric_limits<long double>::infinity();
    for (ll snapshot : snapshots) {
        long double utility = compact_fit_utility(
            park, owner, groups, in_candidate, snapshot);
        sum += utility;
        chmin(minimum, utility);
    }
    long double average = sum / FUTURE_FIT_SNAPSHOT_COUNT;
    return 0.75L * average + 0.25L * minimum;
}

optional<NormalPlacementChoice> choose_temporally_coherent_region(
    const vs &park, const vvi &owner, const vector<GroupState> &groups,
    ll current_s, ll arrival_t, int p, long double theta,
    int remaining_groups, const vector<Shape> &shapes,
    TemporalPlacementDiagnostics &diagnostics) {
    diagnostics.attempts++;
    int n = park.size();
    vector<vi> blocked_prefix = make_blocked_prefix(park, owner);

    ConditionalFutureDemand future_demand(current_s, theta);
    long double candidate_arrival_level =
        future_demand.future_start_cdf(arrival_t);

    auto release_level = [&](ll release_time) {
        long double remaining = max(0LL, release_time - current_s);
        return -expm1l(-remaining / theta);
    };
    long double candidate_release_level = release_level(arrival_t);
    vector<long double> group_arrival_level(groups.size(), -1.0L);
    vector<long double> group_release_level(groups.size(), -1.0L);

    vector<vector<long double>> incremental_cell(
        n, vector<long double>(n));
    vector<vector<long double>> absolute_cell(
        n, vector<long double>(n));
    constexpr int DX[4] = {-1, 1, 0, 0};
    constexpr int DY[4] = {0, 0, -1, 1};
    for (int x = 0; x < n; x++) {
        for (int y = 0; y < n; y++) {
            if (park[x][y] != '.' || owner[x][y] != -1) continue;
            for (int dir = 0; dir < 4; dir++) {
                int nx = x + DX[dir];
                int ny = y + DY[dir];
                if (!inside(nx, ny, n, n) || park[nx][ny] == '#') continue;
                int adjacent_owner = owner[nx][ny];
                long double adjacent_arrival_level = 0.0L;
                long double adjacent_release_level = 0.0L;
                if (adjacent_owner != -1) {
                    if (group_arrival_level[adjacent_owner] < 0.0L) {
                        group_arrival_level[adjacent_owner] =
                            future_demand.future_start_cdf(
                                groups[adjacent_owner].t);
                        group_release_level[adjacent_owner] =
                            release_level(groups[adjacent_owner].t);
                    }
                    adjacent_arrival_level =
                        group_arrival_level[adjacent_owner];
                    adjacent_release_level =
                        group_release_level[adjacent_owner];
                }
                incremental_cell[x][y] +=
                    fabsl(candidate_arrival_level - adjacent_arrival_level) -
                    adjacent_arrival_level;
                absolute_cell[x][y] +=
                    fabsl(candidate_release_level - adjacent_release_level);
            }
        }
    }

    auto make_prefix = [&](const vector<vector<long double>> &values) {
        vector<vector<long double>> prefix(
            n + 1, vector<long double>(n + 1));
        for (int x = 0; x < n; x++) {
            for (int y = 0; y < n; y++) {
                prefix[x + 1][y + 1] = values[x][y] +
                    prefix[x][y + 1] + prefix[x + 1][y] - prefix[x][y];
            }
        }
        return prefix;
    };
    vector<vector<long double>> incremental_prefix =
        make_prefix(incremental_cell);
    vector<vector<long double>> absolute_prefix =
        make_prefix(absolute_cell);

    PlacementShortlistBuilder shortlist_builder;
    long long enumeration_order = 0;
    int minimum_perimeter = shapes.front().perimeter;

    auto scan_shape = [&](const Shape &shape, PlacementSource source) {
        bool found_legal = false;
        auto relative_coordinate_sum = [](const Rect &rect, bool x_axis) {
            long long coordinate = x_axis ? rect.x : rect.y;
            long long length = x_axis ? rect.h : rect.w;
            long long copies = x_axis ? rect.w : rect.h;
            return copies *
                (length * coordinate + length * (length - 1) / 2);
        };
        long long relative_sum_x =
            relative_coordinate_sum(shape.main_rect, true) +
            relative_coordinate_sum(shape.extra_rect, true);
        long long relative_sum_y =
            relative_coordinate_sum(shape.main_rect, false) +
            relative_coordinate_sum(shape.extra_rect, false);
        int max_x = n - shape.h;
        int max_y = n - shape.w;
        for (int base_x = 0; base_x <= max_x; base_x++) {
            for (int base_y = 0; base_y <= max_y; base_y++) {
                diagnostics.anchors_checked++;
                const Rect &main_rect = shape.main_rect;
                const Rect &extra_rect = shape.extra_rect;
                if (rectangle_sum(blocked_prefix,
                                  base_x + main_rect.x,
                                  base_y + main_rect.y,
                                  main_rect.h, main_rect.w) != 0) {
                    continue;
                }
                if (rectangle_sum(blocked_prefix,
                                  base_x + extra_rect.x,
                                  base_y + extra_rect.y,
                                  extra_rect.h, extra_rect.w) != 0) {
                    continue;
                }
                diagnostics.legal_compact_candidates++;
                found_legal = true;

                long double incremental_cost =
                    rectangle_sum(incremental_prefix,
                                  base_x + main_rect.x,
                                  base_y + main_rect.y,
                                  main_rect.h, main_rect.w) +
                    rectangle_sum(incremental_prefix,
                                  base_x + extra_rect.x,
                                  base_y + extra_rect.y,
                                  extra_rect.h, extra_rect.w) -
                    (4 * p - shape.perimeter) * candidate_arrival_level;
                long double absolute_cost =
                    rectangle_sum(absolute_prefix,
                                  base_x + main_rect.x,
                                  base_y + main_rect.y,
                                  main_rect.h, main_rect.w) +
                    rectangle_sum(absolute_prefix,
                                  base_x + extra_rect.x,
                                  base_y + extra_rect.y,
                                  extra_rect.h, extra_rect.w) -
                    (4 * p - shape.perimeter) * candidate_release_level;
                long long sum_x = (long long)p * base_x + relative_sum_x;
                long long sum_y = (long long)p * base_y + relative_sum_y;
                int lower_half = 2 * sum_x >= (long long)p * n;
                int right_half = 2 * sum_y >= (long long)p * n;
                int quadrant = 2 * lower_half + right_half;
                long long order = enumeration_order++;
                shortlist_builder.consider(
                    shape.perimeter, incremental_cost, absolute_cost, order,
                    quadrant, source,
                    [&] { return materialize_shape(shape, base_x, base_y, p); });
            }
        }
        return found_legal;
    };

    bool found_minimum_template = false;
    for (const Shape &shape : shapes) {
        if (shape.perimeter != minimum_perimeter) continue;
        found_minimum_template |=
            scan_shape(shape, PlacementSource::MinimumTemplate);
    }

    if (!found_minimum_template) {
        // Shapes are sorted by perimeter.  Once one extended tier has a
        // legal placement, every later tier is strictly worse in immediate
        // perimeter, so it cannot survive the minimum-perimeter collector.
        for (size_t first = 0; first < shapes.size();) {
            size_t last = first + 1;
            while (last < shapes.size() &&
                   shapes[last].perimeter == shapes[first].perimeter) {
                last++;
            }
            if (shapes[first].perimeter > minimum_perimeter) {
                bool found_in_tier = false;
                for (size_t index = first; index < last; index++) {
                    found_in_tier |= scan_shape(
                        shapes[index], PlacementSource::ExtendedTemplate);
                }
                if (found_in_tier) break;
            }
            first = last;
        }

        vector<vector<Cell>> growth_candidates =
            make_connected_growth_candidates(park, owner, p);
        diagnostics.connected_growth_candidates += growth_candidates.size();
        for (vector<Cell> &region : growth_candidates) {
            int perimeter = calc_perimeter(region, n);
            long double incremental_cost = 0.0L;
            long double absolute_cost = 0.0L;
            for (auto [x, y] : region) {
                incremental_cost += incremental_cell[x][y];
                absolute_cost += absolute_cell[x][y];
            }
            incremental_cost -=
                (4 * p - perimeter) * candidate_arrival_level;
            absolute_cost -=
                (4 * p - perimeter) * candidate_release_level;
            int quadrant = placement_quadrant(region, n);
            long long order = enumeration_order++;
            shortlist_builder.consider(
                perimeter, incremental_cost, absolute_cost, order, quadrant,
                PlacementSource::ConnectedGrowth,
                [&] { return region; });
        }
    }

    vector<PlacementCandidate> candidates = shortlist_builder.finalize();
    if (candidates.empty()) return nullopt;
    diagnostics.shortlisted_candidates += candidates.size();

    int incremental_best = 0;
    int absolute_best = 0;
    for (int index = 1; index < (int)candidates.size(); index++) {
        if (placement_increment_less(candidates[index],
                                     candidates[incremental_best])) {
            incremental_best = index;
        }
        if (placement_absolute_less(candidates[index],
                                    candidates[absolute_best])) {
            absolute_best = index;
        }
    }
    if (!same_region(candidates[incremental_best].cells,
                     candidates[absolute_best].cells)) {
        diagnostics.incremental_changed_from_absolute++;
    }
    int best_index = incremental_best;

    long double future_mass = future_demand.future_start_cdf(arrival_t);
    if ((int)candidates.size() >= 2 && remaining_groups > 0 &&
        arrival_t - current_s > 1 &&
        future_mass > 1e-12L) {
        array<ll, FUTURE_FIT_SNAPSHOT_COUNT> snapshots =
            make_future_fit_snapshots(future_demand, current_s, arrival_t);
        long double best_fit = -numeric_limits<long double>::infinity();
        for (int index = 0; index < (int)candidates.size(); index++) {
            long double fit = evaluate_compact_fit(
                park, owner, groups, candidates[index].cells, snapshots);
            diagnostics.future_fit_snapshots += FUTURE_FIT_SNAPSHOT_COUNT;
            if (fit > best_fit + 1e-15L ||
                (fabsl(fit - best_fit) <= 1e-15L &&
                 placement_increment_less(candidates[index],
                                          candidates[best_index]))) {
                best_fit = fit;
                best_index = index;
            }
        }
        diagnostics.future_fit_evaluated_turns++;
        if (!same_region(candidates[incremental_best].cells,
                         candidates[best_index].cells)) {
            diagnostics.future_fit_changed_placements++;
        }
    }

    if (!same_region(candidates[best_index].cells,
                     candidates[absolute_best].cells)) {
        diagnostics.final_changed_from_absolute++;
    }
    PlacementCandidate choice = std::move(candidates[best_index]);
    if (choice.source == PlacementSource::ConnectedGrowth) {
        diagnostics.fallback_successes++;
    } else {
        diagnostics.compact_successes++;
        if (choice.source == PlacementSource::ExtendedTemplate) {
            diagnostics.extended_template_successes++;
        }
    }
    return NormalPlacementChoice{std::move(choice.cells), choice.perimeter};
}

struct ShadowDiagnostics {
    int considered = 0;
    int upper_bound_rejected = 0;
    int actual_fee_rejected = 0;
    int no_region_rejected = 0;
    int accepted = 0;
    long double theta_sum = 0.0L;
    long double opportunity_cost_sum = 0.0L;
    long double rejected_fraction_sum = 0.0L;
    long double maximum_rejected_fraction = 0.0L;
    long long priced_buckets = 0;
};

struct FreeComponents {
    vvi id;
    vi size;
    int total_free = 0;
};

FreeComponents label_free_components(const vs &park, const vvi &owner) {
    int n = park.size();
    FreeComponents result{vvi(n, vi(n, -1)), {}, 0};
    constexpr int DX[4] = {-1, 1, 0, 0};
    constexpr int DY[4] = {0, 0, -1, 1};

    for (int start_x = 0; start_x < n; start_x++) {
        for (int start_y = 0; start_y < n; start_y++) {
            if (park[start_x][start_y] == '#' || owner[start_x][start_y] != -1 ||
                result.id[start_x][start_y] != -1) {
                continue;
            }

            int component_id = result.size.size();
            int component_size = 0;
            queue<Cell> que;
            result.id[start_x][start_y] = component_id;
            que.emplace(start_x, start_y);

            while (!que.empty()) {
                auto [x, y] = que.front();
                que.pop();
                component_size++;

                for (int dir = 0; dir < 4; dir++) {
                    int nx = x + DX[dir];
                    int ny = y + DY[dir];
                    if (!inside(nx, ny, n, n) || result.id[nx][ny] != -1) continue;
                    if (park[nx][ny] == '#' || owner[nx][ny] != -1) continue;
                    result.id[nx][ny] = component_id;
                    que.emplace(nx, ny);
                }
            }

            result.size.push_back(component_size);
            result.total_free += component_size;
        }
    }
    return result;
}

struct RelocationCandidate {
    int id;
    ll cost;
    int unlocked_size;
};

optional<TurnPlan> try_single_relocation(
    const vs &park, const vvi &owner, const vector<GroupState> &groups,
    int arrival_p, ll arrival_v, int r_milli,
    const vector<vector<Shape>> &compact_shapes, const Timer &timer) {
    if (timer.elapsed() >= RELOCATION_TIME_LIMIT) return nullopt;

    int n = park.size();
    FreeComponents components = label_free_components(park, owner);
    if (components.total_free < arrival_p) return nullopt;

    constexpr int DX[4] = {-1, 1, 0, 0};
    constexpr int DY[4] = {0, 0, -1, 1};
    vector<RelocationCandidate> candidates;

    for (int j = 0; j < (int)groups.size(); j++) {
        const GroupState &group = groups[j];
        if (!group.active) continue;

        vector<char> adjacent(components.size.size(), false);
        int unlocked_size = group.p;
        for (auto [x, y] : group.cells) {
            for (int dir = 0; dir < 4; dir++) {
                int nx = x + DX[dir];
                int ny = y + DY[dir];
                if (!inside(nx, ny, n, n)) continue;
                int component_id = components.id[nx][ny];
                if (component_id == -1 || adjacent[component_id]) continue;
                adjacent[component_id] = true;
                unlocked_size += components.size[component_id];
            }
        }
        if (unlocked_size < arrival_p) continue;

        ll cost = move_cost(group.v, r_milli);
        if (arrival_v <= cost) continue;
        candidates.push_back({j, cost, unlocked_size});
    }

    sort(candidates.begin(), candidates.end(), [](const RelocationCandidate &lhs,
                                                   const RelocationCandidate &rhs) {
        return tuple(lhs.cost, -lhs.unlocked_size, lhs.id) <
               tuple(rhs.cost, -rhs.unlocked_size, rhs.id);
    });

    constexpr int MAX_RELOCATION_TRIALS = 16;
    TurnPlan best_plan;
    int trials = 0;

    for (const RelocationCandidate &candidate : candidates) {
        if (timer.elapsed() >= RELOCATION_TIME_LIMIT) break;
        if (trials++ == MAX_RELOCATION_TRIALS) break;
        int j = candidate.id;
        const GroupState &group = groups[j];

        // Even a perfectly compact arrival and zero fee loss cannot beat this bound.
        if (arrival_v - candidate.cost <= best_plan.immediate_gain) break;

        vvi trial_owner = owner;
        clear_cells(trial_owner, group.cells);

        vector<vector<Cell>> arrivals = find_region_variants(
            park, trial_owner, arrival_p, compact_shapes[arrival_p]);
        for (const vector<Cell> &arrival : arrivals) {
            if (timer.elapsed() >= RELOCATION_TIME_LIMIT) break;
            bool uses_cleared_cell = false;
            for (auto [x, y] : arrival) {
                if (owner[x][y] == j) uses_cleared_cell = true;
            }
            if (!uses_cleared_cell) continue;

            vvi reserved_owner = trial_owner;
            constexpr int RESERVED_FOR_ARRIVAL = -2;
            place_cells(reserved_owner, arrival, RESERVED_FOR_ARRIVAL);
            optional<vector<Cell>> destination =
                find_region(park, reserved_owner, group.p, compact_shapes[group.p]);
            if (!destination) continue;

            int arrival_perimeter = calc_perimeter(arrival, n);
            int destination_perimeter = calc_perimeter(*destination, n);
            ll previous_fee = round_payment(group.v, group.p, group.max_perimeter);
            ll next_fee = round_payment(
                group.v, group.p, max(group.max_perimeter, destination_perimeter));
            ll fee_loss = previous_fee - next_fee;
            ll gain = round_payment(arrival_v, arrival_p, arrival_perimeter) -
                      candidate.cost - fee_loss;
            if (gain > 0 && gain > best_plan.immediate_gain) {
                best_plan.moves = {{j, *destination, destination_perimeter}};
                best_plan.arrival = arrival;
                best_plan.arrival_perimeter = arrival_perimeter;
                best_plan.immediate_gain = gain;
            }

            int minimum_arrival_perimeter = compact_shapes[arrival_p].front().perimeter;
            if (fee_loss == 0 && arrival_perimeter == minimum_arrival_perimeter) break;
        }
    }

    if (!best_plan.arrival) return nullopt;
    return best_plan;
}

void apply_plan(int arrival_id, TurnPlan &plan, vvi &owner, vector<GroupState> &groups) {
    for (const MovePlan &move : plan.moves) {
        clear_cells(owner, groups[move.id].cells);
    }
    for (const MovePlan &move : plan.moves) {
        place_cells(owner, move.cells, move.id);
        GroupState &group = groups[move.id];
        group.cells = move.cells;
        chmax(group.max_perimeter, move.perimeter);
    }

    if (plan.arrival) {
        place_cells(owner, *plan.arrival, arrival_id);
        GroupState &group = groups[arrival_id];
        group.active = true;
        group.cells = *plan.arrival;
        group.max_perimeter = plan.arrival_perimeter;
    }
}

void emit_plan(const TurnPlan &plan) {
    emit(plan.moves.size());
    for (const MovePlan &move : plan.moves) {
        emit(move.id);
        for (const Cell &cell : move.cells) {
            emit(cell);
        }
    }

    if (plan.arrival) {
        emit("Yes");
        for (const Cell &cell : *plan.arrival) {
            emit(cell);
        }
    } else {
        emit("No");
    }
    cout.flush();
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    Timer timer;

    int N, M;
    ld R;
    scan(N, M, R);
    vs park(N);
    scan(park);

    vector<vector<Shape>> compact_shapes(151);
    for (int p = 4; p <= 150; p++) {
        compact_shapes[p] = make_compact_shapes(p, N);
    }
    DensityModel density_model(compact_shapes);
    ThetaEstimator theta_estimator;
    int grass_cells = 0;
    for (const string &row : park) {
        grass_cells += count(row.begin(), row.end(), '.');
    }

    vvi owner(N, vi(N, -1));
    vector<GroupState> groups(M);
    priority_queue<pair<ll, int>, vector<pair<ll, int>>, greater<pair<ll, int>>> departures;
    int accepted_count = 0;
    int rejected_count = 0;
    ShadowDiagnostics shadow_diagnostics;
    TemporalPlacementDiagnostics placement_diagnostics;

    for (int turn = 0; turn < M; turn++) {
        int i, P;
        ll S, T, V;
        scan(i, S, T, P, V);

        groups[i].t = T;
        groups[i].v = V;
        groups[i].p = P;
        theta_estimator.observe(T - S);
        int remaining_groups = M - i - 1;
        long double theta =
            theta_estimator.estimate(S, remaining_groups);

        while (!departures.empty() && departures.top().first < S) {
            int j = departures.top().second;
            departures.pop();
            if (!groups[j].active) continue;
            clear_cells(owner, groups[j].cells);
            groups[j].cells.clear();
            groups[j].active = false;
        }

        TurnPlan plan;
        ShadowEvaluation shadow = evaluate_shadow_cost(
            groups, S, T, P, remaining_groups, grass_cells, theta,
            density_model);
        shadow_diagnostics.considered++;
        shadow_diagnostics.theta_sum += theta;
        shadow_diagnostics.opportunity_cost_sum += shadow.opportunity_cost;
        shadow_diagnostics.rejected_fraction_sum +=
            shadow.duration_weighted_rejected_fraction;
        chmax(shadow_diagnostics.maximum_rejected_fraction,
              shadow.maximum_rejected_fraction);
        shadow_diagnostics.priced_buckets += shadow.priced_buckets;

        int minimum_perimeter = compact_shapes[P].front().perimeter;
        ll upper_bound_fee = round_payment(V, P, minimum_perimeter);
        if ((long double)upper_bound_fee <= shadow.opportunity_cost) {
            shadow_diagnostics.upper_bound_rejected++;
        } else {
            optional<NormalPlacementChoice> placement =
                choose_temporally_coherent_region(
                    park, owner, groups, S, T, P, theta,
                    remaining_groups, compact_shapes[P],
                    placement_diagnostics);
            if (!placement) {
                shadow_diagnostics.no_region_rejected++;
            } else {
                ll actual_fee = round_payment(
                    V, P, placement->perimeter);
                if ((long double)actual_fee <= shadow.opportunity_cost) {
                    shadow_diagnostics.actual_fee_rejected++;
                } else {
                    plan.arrival = std::move(placement->cells);
                    plan.arrival_perimeter = placement->perimeter;
                    shadow_diagnostics.accepted++;
                }
            }
        }

        apply_plan(i, plan, owner, groups);
        if (plan.arrival) {
            departures.emplace(T, i);
            accepted_count++;
        } else {
            rejected_count++;
        }

        emit_plan(plan);
    }

    long double mean_theta = shadow_diagnostics.considered == 0
        ? 0.0L
        : shadow_diagnostics.theta_sum / shadow_diagnostics.considered;
    long double mean_opportunity_cost = shadow_diagnostics.considered == 0
        ? 0.0L
        : shadow_diagnostics.opportunity_cost_sum /
              shadow_diagnostics.considered;
    long double mean_rejected_fraction = shadow_diagnostics.considered == 0
        ? 0.0L
        : shadow_diagnostics.rejected_fraction_sum /
              shadow_diagnostics.considered;
    cerr << "accepted=" << accepted_count
         << " rejected=" << rejected_count
         << " relocations=0"
         << " shadow_considered=" << shadow_diagnostics.considered
         << " shadow_upper_rejected="
         << shadow_diagnostics.upper_bound_rejected
         << " shadow_actual_rejected="
         << shadow_diagnostics.actual_fee_rejected
         << " shadow_no_region_rejected="
         << shadow_diagnostics.no_region_rejected
         << " shadow_accepted=" << shadow_diagnostics.accepted
         << " placement_attempts=" << placement_diagnostics.attempts
         << " placement_compact_successes="
         << placement_diagnostics.compact_successes
         << " placement_extended_template_successes="
         << placement_diagnostics.extended_template_successes
         << " placement_fallback_successes="
         << placement_diagnostics.fallback_successes
         << " placement_future_fit_turns="
         << placement_diagnostics.future_fit_evaluated_turns
         << " placement_future_fit_changes="
         << placement_diagnostics.future_fit_changed_placements
         << " placement_incremental_changes_from_absolute="
         << placement_diagnostics.incremental_changed_from_absolute
         << " placement_final_changes_from_absolute="
         << placement_diagnostics.final_changed_from_absolute
         << " placement_anchors_checked="
         << placement_diagnostics.anchors_checked
         << " placement_legal_compact_candidates="
         << placement_diagnostics.legal_compact_candidates
         << " placement_growth_candidates="
         << placement_diagnostics.connected_growth_candidates
         << " placement_shortlisted_candidates="
         << placement_diagnostics.shortlisted_candidates
         << " placement_future_fit_snapshots="
         << placement_diagnostics.future_fit_snapshots
         << fixed << setprecision(6)
         << " theta_mean=" << mean_theta
         << " shadow_mean_opportunity=" << mean_opportunity_cost
         << " shadow_mean_rejected_fraction=" << mean_rejected_fraction
         << " shadow_max_rejected_fraction="
         << shadow_diagnostics.maximum_rejected_fraction
         << " shadow_priced_buckets="
         << shadow_diagnostics.priced_buckets
         << " model_expected_p=" << density_model.expected_group_size
         << " elapsed=" << setprecision(3) << timer.elapsed() << '\n';

    return 0;
}
