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
constexpr bool ENABLE_RELOCATION = false;
constexpr bool ENABLE_ADMISSION_CONTROL = true;
constexpr long double ADMISSION_MAX_HISTORY_QUANTILE = 0.25L;
constexpr ll ARRIVAL_TIME_HORIZON = 100000;
constexpr array<int, 8> FUTURE_SLOT_SIZES = {4, 9, 16, 25, 36, 64, 100, 150};
constexpr long double FUTURE_SHAPE_WEIGHT = 0.08L;

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
    FutureSlotWeights future_slot_weights =
        make_future_slot_weights(compact_shapes);

    vvi owner(N, vi(N, -1));
    vector<GroupState> groups(M);
    priority_queue<pair<ll, int>, vector<pair<ll, int>>, greater<pair<ll, int>>> departures;
    int r_milli = (int)llroundl(R * 1000.0L);
    int accepted_count = 0;
    int rejected_count = 0;
    int relocation_count = 0;
    bool future_disabled = false;
    FutureDiagnostics future_diagnostics;
    AdmissionDiagnostics admission_diagnostics;
    vector<long double> observed_potential_densities;
    observed_potential_densities.reserve(M);
    ll estimated_relocation_gain = 0;

    for (int turn = 0; turn < M; turn++) {
        int i, P;
        ll S, T, V;
        scan(i, S, T, P, V);

        groups[i].t = T;
        groups[i].v = V;
        groups[i].p = P;

        ll potential_fee = round_payment(
            V, P, compact_shapes[P].front().perimeter);
        ll potential_cell_time = (ll)P * (T - S);
        observed_potential_densities.push_back(
            (long double)potential_fee / (long double)potential_cell_time);

        while (!departures.empty() && departures.top().first < S) {
            int j = departures.top().second;
            departures.pop();
            if (!groups[j].active) continue;
            clear_cells(owner, groups[j].cells);
            groups[j].cells.clear();
            groups[j].active = false;
        }

        TurnPlan plan;
        optional<NormalPlacementChoice> normal_choice;
        if (!future_disabled && timer.elapsed() >= FUTURE_EMERGENCY_TIME_LIMIT) {
            future_disabled = true;
            future_diagnostics.emergency_stops++;
        }
        if (!future_disabled) {
            normal_choice = choose_demand_aware_region(
                park, owner, groups, i, S, T, P, V, compact_shapes[P],
                future_slot_weights, timer, future_disabled,
                future_diagnostics);
        } else if (auto region = find_region(park, owner, P, compact_shapes[P])) {
            normal_choice = NormalPlacementChoice{
                *region, calc_perimeter(*region, N)};
        }

        if (normal_choice) {
            bool admission_rejected = reject_by_admission_control(
                i, M, S, T, P, V, normal_choice->perimeter,
                observed_potential_densities,
                admission_diagnostics);
            if (!admission_rejected) {
                plan.arrival = normal_choice->cells;
                plan.arrival_perimeter = normal_choice->perimeter;
            }
        } else if (ENABLE_RELOCATION) {
            if (auto relocation = try_single_relocation(
                    park, owner, groups, P, V, r_milli, compact_shapes, timer)) {
                plan = std::move(*relocation);
            }
        }

        apply_plan(i, plan, owner, groups);
        if (plan.arrival) {
            departures.emplace(T, i);
            accepted_count++;
        } else {
            rejected_count++;
        }
        if (!plan.moves.empty()) {
            relocation_count++;
            estimated_relocation_gain += plan.immediate_gain;
        }

        emit_plan(plan);
    }

    long double final_reference_density = empirical_quantile(
        observed_potential_densities, ADMISSION_MAX_HISTORY_QUANTILE);
    long double mean_rejected_density = admission_diagnostics.rejected == 0
        ? 0.0L
        : admission_diagnostics.rejected_density_sum /
              admission_diagnostics.rejected;
    long double mean_rejected_threshold = admission_diagnostics.rejected == 0
        ? 0.0L
        : admission_diagnostics.threshold_sum / admission_diagnostics.rejected;
    long double mean_dynamic_quantile =
        admission_diagnostics.density_comparisons == 0
        ? 0.0L
        : admission_diagnostics.dynamic_quantile_sum /
              admission_diagnostics.density_comparisons;
    long double mean_expected_arrivals =
        admission_diagnostics.density_comparisons == 0
        ? 0.0L
        : admission_diagnostics.expected_arrivals_sum /
              admission_diagnostics.density_comparisons;
    cerr << "accepted=" << accepted_count
         << " rejected=" << rejected_count
         << " relocations=" << relocation_count
         << " future_attempts=" << future_diagnostics.attempts
         << " future_turns=" << future_diagnostics.evaluated_turns
         << " future_candidates=" << future_diagnostics.candidates
         << " future_emergency_stops=" << future_diagnostics.emergency_stops
         << " future_last_turn=" << future_diagnostics.last_turn
         << " changed_placements=" << future_diagnostics.changed_placements
         << " admission_considered=" << admission_diagnostics.considered
         << " admission_density_comparisons="
         << admission_diagnostics.density_comparisons
         << " admission_rejected=" << admission_diagnostics.rejected
         << " admission_other_rejected="
         << rejected_count - admission_diagnostics.rejected
         << " admission_rejected_fee=" << admission_diagnostics.rejected_fee
         << " admission_rejected_cell_time="
         << admission_diagnostics.rejected_cell_time
         << " admission_reference_density=" << fixed << setprecision(6)
         << final_reference_density
         << " admission_mean_rejected_density=" << mean_rejected_density
         << " admission_mean_rejected_threshold=" << mean_rejected_threshold
         << " admission_mean_dynamic_quantile=" << mean_dynamic_quantile
         << " admission_mean_expected_arrivals=" << mean_expected_arrivals
         << " estimated_relocation_gain=" << estimated_relocation_gain
         << " elapsed=" << setprecision(3) << timer.elapsed() << '\n';

    return 0;
}
