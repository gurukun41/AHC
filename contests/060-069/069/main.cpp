#include <bits/stdc++.h>
using namespace std;
using ll = long long;
using ld = long double;
using vi = vector<int>;
using vvi = vector<vi>;
using vb = vector<bool>;
using vvb = vector<vb>;
using vs = vector<string>;

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

bool inside(int x, int y, int h, int w) { return 0 <= x && x < h && 0 <= y && y < w; }

using Cell = pair<int, int>;

struct RuntimeDiagnostics {
    using WallClock = chrono::steady_clock;

    WallClock::time_point protocol_start = WallClock::now();
    clock_t process_cpu_start = clock();
    WallClock::duration solver_wall{};
    WallClock::duration diagnostic_wall{};
    WallClock::duration input_wall{};
    WallClock::duration output_wall{};
    WallClock::duration preprocess_wall{};
    WallClock::duration maximum_solver_turn_wall{};
    double solver_cpu_seconds = 0.0;
    double diagnostic_cpu_seconds = 0.0;

    static double cpu_seconds(clock_t begin, clock_t end) {
        if (begin == (clock_t)-1 || end == (clock_t)-1) return -1.0;
        return (double)(end - begin) / CLOCKS_PER_SEC;
    }

    void add_preprocess(WallClock::time_point wall_begin, clock_t cpu_begin) {
        WallClock::duration elapsed_wall = WallClock::now() - wall_begin;
        solver_wall += elapsed_wall;
        preprocess_wall += elapsed_wall;
        double elapsed_cpu = cpu_seconds(cpu_begin, clock());
        if (elapsed_cpu >= 0.0) solver_cpu_seconds += elapsed_cpu;
    }

    void add_turn(WallClock::time_point wall_begin, clock_t cpu_begin) {
        WallClock::duration elapsed_wall = WallClock::now() - wall_begin;
        solver_wall += elapsed_wall;
        chmax(maximum_solver_turn_wall, elapsed_wall);
        double elapsed_cpu = cpu_seconds(cpu_begin, clock());
        if (elapsed_cpu >= 0.0) solver_cpu_seconds += elapsed_cpu;
    }

    void add_diagnostic(WallClock::time_point wall_begin, clock_t cpu_begin) {
        diagnostic_wall += WallClock::now() - wall_begin;
        double elapsed_cpu = cpu_seconds(cpu_begin, clock());
        if (elapsed_cpu >= 0.0) diagnostic_cpu_seconds += elapsed_cpu;
    }

    void add_input(WallClock::time_point wall_begin) {
        input_wall += WallClock::now() - wall_begin;
    }

    void add_output(WallClock::time_point wall_begin) {
        output_wall += WallClock::now() - wall_begin;
    }
};

struct RuntimeSnapshot {
    double process_cpu_ms = 0.0;
    double solver_cpu_ms = 0.0;
    double diagnostic_cpu_ms = 0.0;
    double solver_wall_ms = 0.0;
    double diagnostic_wall_ms = 0.0;
    double input_wall_ms = 0.0;
    double output_wall_ms = 0.0;
    double protocol_wall_ms = 0.0;
    double unaccounted_wall_ms = 0.0;
    double preprocess_wall_ms = 0.0;
    double maximum_solver_turn_wall_ms = 0.0;
};

RuntimeSnapshot snapshot_runtime(const RuntimeDiagnostics &diagnostics) {
    RuntimeSnapshot result;
    auto protocol_wall = RuntimeDiagnostics::WallClock::now() - diagnostics.protocol_start;
    double process_cpu = RuntimeDiagnostics::cpu_seconds(diagnostics.process_cpu_start, clock());
    result.process_cpu_ms = process_cpu < 0.0 ? -1.0 : 1000.0 * process_cpu;
    result.solver_cpu_ms = 1000.0 * diagnostics.solver_cpu_seconds;
    result.diagnostic_cpu_ms = 1000.0 * diagnostics.diagnostic_cpu_seconds;
    result.solver_wall_ms = chrono::duration<double, milli>(diagnostics.solver_wall).count();
    result.diagnostic_wall_ms = chrono::duration<double, milli>(diagnostics.diagnostic_wall).count();
    result.input_wall_ms = chrono::duration<double, milli>(diagnostics.input_wall).count();
    result.output_wall_ms = chrono::duration<double, milli>(diagnostics.output_wall).count();
    result.protocol_wall_ms = chrono::duration<double, milli>(protocol_wall).count();
    result.unaccounted_wall_ms = result.protocol_wall_ms - result.solver_wall_ms -
                                 result.diagnostic_wall_ms - result.input_wall_ms -
                                 result.output_wall_ms;
    result.preprocess_wall_ms = chrono::duration<double, milli>(diagnostics.preprocess_wall).count();
    result.maximum_solver_turn_wall_ms =
        chrono::duration<double, milli>(diagnostics.maximum_solver_turn_wall).count();
    return result;
}

constexpr ll ARRIVAL_TIME_HORIZON = 100000;
constexpr int TIME_BUCKET_COUNT = 64;
constexpr int SAMPLED_DLP_BUCKET_COUNT = 16;
constexpr int SAMPLED_DLP_REQUEST_COUNT = 256;
constexpr int SAMPLED_DLP_COORDINATE_SWEEPS = 8;
constexpr long double SAMPLED_DLP_PRICE_QUANTIZATION = 1000000000.0L;
constexpr int THETA_MIN = 2000;
constexpr int THETA_MAX = 8000;
constexpr int THETA_STEP = 100;
constexpr int THETA_QUADRATURE_STEPS = 48;
constexpr int COMPACT_PERIMETER_MARGIN = 4;
constexpr int PLACEMENT_GLOBAL_SHORTLIST = 3;
constexpr int PLACEMENT_SHORTLIST_LIMIT = 6;
constexpr int CONNECTED_GROWTH_SEED_LIMIT = 16;
constexpr int GROW_AND_TRIM_EXTRA_CELLS = 8;
constexpr int GROW_AND_TRIM_CANDIDATE_LIMIT = 8;
constexpr int FUTURE_FIT_SNAPSHOT_COUNT = 3;
constexpr array<int, 8> FUTURE_FIT_SIDES = {2, 3, 4, 5, 6, 8, 10, 12};
// Every minimum-perimeter target is cheap-scanned.  Exact blocker sets are
// recovered only for the union of two shortlists, then repaired in economic
// upper-bound order.  These are work limits, not limits on blocker count.
constexpr int RESCUE_TARGET_SHORTLIST_PER_METRIC = 160;
constexpr int RESCUE_TARGET_REPAIR_LIMIT = 8;
constexpr int RESCUE_DESTINATION_ANCHOR_LIMIT = 4096;
constexpr int RESCUE_DESTINATION_ANCHOR_GLOBAL_LIMIT = 50000;
constexpr int RESCUE_DESTINATION_LEGAL_LIMIT = 64;
constexpr int RESCUE_DESTINATION_LIMIT = 10;
constexpr int RESCUE_BEAM_WIDTH = 32;
constexpr int RESCUE_REPAIR_NODE_LIMIT = 2048;
constexpr int RESCUE_ROLLOUT_CANDIDATE_LIMIT = 2;
// NoRegion is far more frequent than Accepted compact rescue.  Use the same
// unrestricted-blocker search with smaller deterministic work caps so the
// added repair path remains viable under the contest time limit.
constexpr int PUSHOUT_TARGET_SHORTLIST_PER_METRIC = 96;
constexpr int PUSHOUT_TARGET_REPAIR_LIMIT = 4;
constexpr int PUSHOUT_DESTINATION_ANCHOR_LIMIT = 2048;
constexpr int PUSHOUT_DESTINATION_ANCHOR_GLOBAL_LIMIT = 16000;
constexpr int PUSHOUT_DESTINATION_LEGAL_LIMIT = 40;
constexpr int PUSHOUT_DESTINATION_LIMIT = 8;
constexpr int PUSHOUT_REPAIR_NODE_LIMIT = 1024;
// A helper is an active non-blocker whose removal opens destination regions
// for the true blockers.  Surveying reuses the legacy destination probes; the
// second-stage repair has its own deterministic caps and never consumes work
// reserved for the protected legacy candidates.
constexpr int PUSHOUT_HELPER_OBSTRUCTION_PROBE_LIMIT = 1024;
constexpr int PUSHOUT_HELPER_MAX_BLOCKERS = 3;
#ifdef AHC069_DISABLE_WIDE_PUSHOUT_HELPER
constexpr int PUSHOUT_HELPER_CHOICE_LIMIT_PER_TARGET = 1;
constexpr int PUSHOUT_HELPER_REPAIR_LIMIT = 2;
constexpr int PUSHOUT_HELPER_DESTINATION_ANCHOR_LIMIT = 1024;
constexpr int PUSHOUT_HELPER_DESTINATION_ANCHOR_GLOBAL_LIMIT = 4096;
constexpr int PUSHOUT_HELPER_DESTINATION_LEGAL_LIMIT = 24;
constexpr int PUSHOUT_HELPER_DESTINATION_LIMIT = 6;
constexpr int PUSHOUT_HELPER_REPAIR_NODE_LIMIT = 256;
constexpr int PUSHOUT_HELPER_FEASIBLE_LIMIT = 1;
#else
constexpr int PUSHOUT_HELPER_CHOICE_LIMIT_PER_TARGET = 3;
constexpr int PUSHOUT_HELPER_REPAIR_LIMIT = 6;
constexpr int PUSHOUT_HELPER_DESTINATION_ANCHOR_LIMIT = 2048;
constexpr int PUSHOUT_HELPER_DESTINATION_ANCHOR_GLOBAL_LIMIT = 16384;
constexpr int PUSHOUT_HELPER_DESTINATION_LEGAL_LIMIT = 48;
constexpr int PUSHOUT_HELPER_DESTINATION_LIMIT = 12;
constexpr int PUSHOUT_HELPER_REPAIR_NODE_LIMIT = 1024;
constexpr int PUSHOUT_HELPER_FEASIBLE_LIMIT = 2;
#endif
static_assert(PUSHOUT_HELPER_FEASIBLE_LIMIT <= RESCUE_ROLLOUT_CANDIDATE_LIMIT);
// Deadline-layer reconstruction has no semantic limit on the number of moved
// groups.  Every limit below counts deterministic work instead; an arbitrarily
// deep closure remains eligible when its required work fits these budgets.
constexpr int DEADLINE_GRAPH_BUILD_CASE_LIMIT = 8;
constexpr int DEADLINE_WINDOW_ATTEMPT_LIMIT = 1;
constexpr int DEADLINE_CLOSURE_EXPANSION_TURN_LIMIT = 64;
constexpr int DEADLINE_CLOSURE_EXPANSION_CASE_LIMIT = 256;
constexpr int DEADLINE_CLOSURE_KEEP_LIMIT = 2;
constexpr int DEADLINE_CORE_ROOT_LIMIT = 2;
constexpr int DEADLINE_LAYOUT_BEAM_WIDTH = 6;
constexpr int DEADLINE_REGION_CANDIDATE_LIMIT = 3;
constexpr int DEADLINE_LAYOUT_NODE_WORKSPACE_LIMIT = 384;
constexpr int DEADLINE_LAYOUT_NODE_TURN_LIMIT = 768;
constexpr int DEADLINE_LAYOUT_NODE_CASE_LIMIT = 3072;
constexpr int DEADLINE_TEMPLATE_PROBE_TURN_LIMIT = 4096;
constexpr int DEADLINE_TEMPLATE_PROBE_CASE_LIMIT = 16384;
constexpr int DEADLINE_GROWTH_STEP_TURN_LIMIT = 1024;
constexpr int DEADLINE_GROWTH_STEP_CASE_LIMIT = 4096;
constexpr int DEADLINE_CONNECTIVITY_CALL_TURN_LIMIT = 512;
constexpr int DEADLINE_CONNECTIVITY_CALL_CASE_LIMIT = 2048;
constexpr int DEADLINE_CONNECTIVITY_VISIT_TURN_LIMIT = 750000;
constexpr int DEADLINE_CONNECTIVITY_VISIT_CASE_LIMIT = 3000000;
constexpr int DEADLINE_COMPLETE_PLAN_TURN_LIMIT = 2;
constexpr int DEADLINE_COMPLETE_PLAN_CASE_LIMIT = 16;
constexpr int DEADLINE_ROLLOUT_CANDIDATE_LIMIT = 2;
constexpr int DEADLINE_CONFIRMATION_CASE_LIMIT = 1;
constexpr int ROOT_SCREEN_SCENARIO_COUNT = 2;
constexpr int ROOT_SCREEN_ROLLOUT_LENGTH = 4;
constexpr int ROOT_CONFIRM_SCENARIO_COUNT = 8;
constexpr int ROOT_CONFIRM_ROLLOUT_LENGTH = 12;
constexpr int ROOT_CONFIRMATION_TURN_LIMIT = 4;
constexpr int ROOT_ROLLOUT_NORMAL_ALTERNATIVE_LIMIT = 2;
#ifdef AHC069_DISABLE_DEADLINE_LAYER
constexpr bool ENABLE_DEADLINE_LAYER = false;
#else
constexpr bool ENABLE_DEADLINE_LAYER = true;
#endif
#ifdef AHC069_DISABLE_NO_REGION_PUSHOUT
constexpr bool ENABLE_NO_REGION_PUSHOUT = false;
#else
constexpr bool ENABLE_NO_REGION_PUSHOUT = true;
#endif
#ifdef AHC069_DISABLE_PUSHOUT_HELPER
constexpr bool ENABLE_PUSHOUT_HELPER = false;
#else
constexpr bool ENABLE_PUSHOUT_HELPER = true;
#endif
#if defined(AHC069_DISABLE_PUSHOUT_HELPER) || defined(AHC069_DISABLE_WIDE_PUSHOUT_HELPER)
constexpr bool ENABLE_WIDE_PUSHOUT_HELPER = false;
#else
constexpr bool ENABLE_WIDE_PUSHOUT_HELPER = true;
#endif
#ifdef AHC069_DISABLE_GROW_AND_TRIM
constexpr bool ENABLE_GROW_AND_TRIM = false;
#else
constexpr bool ENABLE_GROW_AND_TRIM = true;
#endif
#ifdef AHC069_DISABLE_SAMPLED_DLP
constexpr bool ENABLE_SAMPLED_DLP = false;
#else
constexpr bool ENABLE_SAMPLED_DLP = true;
#endif
#ifdef AHC069_PROTECTED_ONLY
constexpr bool ROOT_PROTECTED_ONLY = true;
#else
constexpr bool ROOT_PROTECTED_ONLY = false;
#endif
constexpr int ROOT_SCREEN_MAX_ACTIONS =
    1 + RESCUE_ROLLOUT_CANDIDATE_LIMIT + ROOT_ROLLOUT_NORMAL_ALTERNATIVE_LIMIT;
// Compatibility aliases keep the existing screen implementation intact while
// the root driver is split into screen and confirmation stages.
constexpr int RESCUE_ROLLOUT_SCENARIO_COUNT = ROOT_SCREEN_SCENARIO_COUNT;
constexpr int RESCUE_ROLLOUT_LENGTH = ROOT_SCREEN_ROLLOUT_LENGTH;
constexpr int ROOT_ROLLOUT_MAX_ACTIONS = ROOT_SCREEN_MAX_ACTIONS;
constexpr uint64_t ROOT_ROLLOUT_SEQUENCE_BLOCK_SIZE = 1000003ULL;
constexpr int ROOT_ROLLOUT_SEQUENCE_BLOCKS_PER_BATCH = ROOT_CONFIRM_SCENARIO_COUNT / 2;
constexpr int BOARD_MASK_WORDS = (50 * 50 + 63) / 64;

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
    ll s = 0;
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
// The full ladder is used when an already-admitted group is repaired: any
// template preserving its confirmed fee is a valid destination.
vector<Shape> make_template_shapes(int p, int n) {
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
            add_shape({0, 0, full, width}, {0, 0, 0, 0}, full, width, 2 * (full + width));
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
    sort(shapes.begin(), shapes.end(), [&](const Shape &lhs, const Shape &rhs) { return key(lhs) < key(rhs); });
    shapes.erase(
        unique(shapes.begin(), shapes.end(), [&](const Shape &lhs, const Shape &rhs) { return key(lhs) == key(rhs); }),
        shapes.end());

    return shapes;
}

vector<Cell> materialize_shape(const Shape &shape, int base_x, int base_y, int p) {
    vector<Cell> region;
    region.reserve(p);
    auto append_rectangle = [&](const Rect &rect) {
        for (int dx = 0; dx < rect.h; dx++) {
            for (int dy = 0; dy < rect.w; dy++) {
                region.emplace_back(base_x + rect.x + dx, base_y + rect.y + dy);
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

ll rectangle_sum(const vector<vector<ll>> &prefix, int x, int y, int h, int w) {
    if (h == 0 || w == 0) return 0;
    return prefix[x + h][y + w] - prefix[x][y + w] - prefix[x + h][y] + prefix[x][y];
}

long double rectangle_sum(const vector<vector<long double>> &prefix, int x, int y, int h, int w) {
    if (h == 0 || w == 0) return 0.0L;
    return prefix[x + h][y + w] - prefix[x][y + w] - prefix[x + h][y] + prefix[x][y];
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
            if (park[start_x][start_y] == '#' || owner[start_x][start_y] != -1 || visited[start_x][start_y]) {
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

bool same_region(vector<Cell> lhs, vector<Cell> rhs) {
    if (lhs.size() != rhs.size()) return false;
    sort(lhs.begin(), lhs.end());
    sort(rhs.begin(), rhs.end());
    return lhs == rhs;
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

    const long double approximate = (long double)v * 4.0L * sqrtl((long double)p) / (long double)perimeter;
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

// Read-only geometry observation used only after the turn action is fixed.
// Keeping it out of the placement path makes the loss diagnosis independent
// of the candidate generator that it is intended to audit.
__attribute__((noinline)) int largest_free_component(const vs &park, const vvi &owner) {
    int n = park.size();
    vector<char> visited(n * n, false);
    constexpr int DX[4] = {-1, 1, 0, 0};
    constexpr int DY[4] = {0, 0, -1, 1};
    int largest = 0;

    for (int start_x = 0; start_x < n; start_x++) {
        for (int start_y = 0; start_y < n; start_y++) {
            int start = start_x * n + start_y;
            if (visited[start] || park[start_x][start_y] == '#' || owner[start_x][start_y] != -1) {
                continue;
            }
            visited[start] = true;
            queue<int> que;
            que.push(start);
            int component_size = 0;
            while (!que.empty()) {
                int cell = que.front();
                que.pop();
                component_size++;
                int x = cell / n;
                int y = cell % n;
                for (int dir = 0; dir < 4; dir++) {
                    int nx = x + DX[dir];
                    int ny = y + DY[dir];
                    if (!inside(nx, ny, n, n)) continue;
                    int next = nx * n + ny;
                    if (visited[next] || park[nx][ny] == '#' || owner[nx][ny] != -1) continue;
                    visited[next] = true;
                    que.push(next);
                }
            }
            chmax(largest, component_size);
        }
    }
    return largest;
}

enum class PlacementSource {
    MinimumTemplate,
    ExtendedTemplate,
    ConnectedGrowth,
    GrowAndTrim,
};

struct NormalPlacementChoice {
    vector<Cell> cells;
    int perimeter;
    PlacementSource source;
};

// The hidden duration scale is shared by all groups in one test case.  Besides
// observed durations, the likelihood below uses the fact that every unseen
// group must start after the current order statistic S.
struct ThetaEstimator {
    static constexpr int PARTICLE_COUNT = (THETA_MAX - THETA_MIN) / THETA_STEP + 1;

    struct PosteriorParticles {
        array<long double, PARTICLE_COUNT> weights{};
        long double weight_sum = 0.0L;
    };

    int observed_count = 0;
    int rounded_zero_count = 0;
    long double exponential_sample_sum = 0.0L;

    void observe(ll duration) {
        observed_count++;
        if (duration == 1) rounded_zero_count++;
        exponential_sample_sum += duration - 1;
    }

    long double start_survival(ll current_s, long double theta) const {
        if (current_s >= ARRIVAL_TIME_HORIZON - 1) return 0.0L;

        const long double horizon = ARRIVAL_TIME_HORIZON;
        const long double last_start_without_duration = horizon - 1.0L;
        const long double upper = (last_start_without_duration - current_s) / theta;
        // y = 1-exp(-x) absorbs the exponential measure.  Uniform steps in x
        // are inaccurate near S=0 because the integration range can be about
        // 50 while almost all probability mass is near x=0.
        const long double y_upper = -expm1l(-upper);
        const long double dy = y_upper / THETA_QUADRATURE_STEPS;
        long double integral = 0.0L;
        for (int k = 0; k < THETA_QUADRATURE_STEPS; k++) {
            long double y = (k + 0.5L) * dy;
            long double x = -log1pl(-y);
            long double numerator = last_start_without_duration - current_s - theta * x;
            long double denominator = horizon - theta * x;
            integral += numerator / denominator;
        }
        integral *= dy;
        long double normalizer = -expm1l(-horizon / theta);
        return clamp(integral / normalizer, 1e-300L, 1.0L);
    }

    PosteriorParticles make_posterior(ll current_s, int remaining_groups) const {
        array<long double, PARTICLE_COUNT> log_weights{};
        long double max_log_weight = -numeric_limits<long double>::infinity();

        for (int k = 0; k < PARTICLE_COUNT; k++) {
            long double theta = THETA_MIN + THETA_STEP * k;
            long double normalizer = -expm1l(-(long double)ARRIVAL_TIME_HORIZON / theta);
            long double log_weight =
                -observed_count * logl(theta) - exponential_sample_sum / theta - observed_count * logl(normalizer);
            if (remaining_groups > 0) {
                log_weight += remaining_groups * logl(start_survival(current_s, theta));
            }
            log_weights[k] = log_weight;
            chmax(max_log_weight, log_weight);
        }

        PosteriorParticles posterior;
        for (int k = 0; k < PARTICLE_COUNT; k++) {
            long double weight = expl(log_weights[k] - max_log_weight);
            posterior.weights[k] = weight;
            posterior.weight_sum += weight;
        }
        return posterior;
    }

    long double estimate(ll current_s, int remaining_groups) const {
        // Keep the legacy v3/v4 accumulation path byte-compatible.  Computing
        // the particles in make_posterior() and reducing them in a second
        // function changed the generated floating-point reduction (including
        // FMA use), and tiny theta differences could change placement ties.
        array<long double, PARTICLE_COUNT> log_weights{};
        long double max_log_weight = -numeric_limits<long double>::infinity();

        for (int k = 0; k < PARTICLE_COUNT; k++) {
            long double theta = THETA_MIN + THETA_STEP * k;
            long double normalizer = -expm1l(-(long double)ARRIVAL_TIME_HORIZON / theta);
            long double log_weight =
                -observed_count * logl(theta) - exponential_sample_sum / theta - observed_count * logl(normalizer);
            if (remaining_groups > 0) {
                log_weight += remaining_groups * logl(start_survival(current_s, theta));
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

    long double posterior_quantile(ll current_s, int remaining_groups, long double probability) const {
        PosteriorParticles posterior = make_posterior(current_s, remaining_groups);
        long double target = clamp(probability, 0.0L, 1.0L) * posterior.weight_sum;
        long double cumulative = 0.0L;
        // The posterior itself is the fixed 61-point theta grid, so use its
        // left-continuous inverse CDF rather than interpolating new particles.
        for (int k = 0; k < PARTICLE_COUNT; k++) {
            cumulative += posterior.weights[k];
            if (cumulative >= target) return THETA_MIN + THETA_STEP * k;
        }
        return THETA_MAX;
    }
};

long double inverse_standard_normal(long double probability) {
    // Peter J. Acklam's rational approximation.
    static constexpr array<long double, 6> A = {
        -3.969683028665376e+01L, 2.209460984245205e+02L,  -2.759285104469687e+02L,
        1.383577518672690e+02L,  -3.066479806614716e+01L, 2.506628277459239e+00L,
    };
    static constexpr array<long double, 5> B = {
        -5.447609879822406e+01L, 1.615858368580409e+02L,  -1.556989798598866e+02L,
        6.680131188771972e+01L,  -1.328068155288572e+01L,
    };
    static constexpr array<long double, 6> C = {
        -7.784894002430293e-03L, -3.223964580411365e-01L, -2.400758277161838e+00L,
        -2.549732539343734e+00L, 4.374664141464968e+00L,  2.938163982698783e+00L,
    };
    static constexpr array<long double, 4> D = {
        7.784695709041462e-03L,
        3.224671290700398e-01L,
        2.445134137142996e+00L,
        3.754408661907416e+00L,
    };
    constexpr long double LOW = 0.02425L;
    constexpr long double HIGH = 1.0L - LOW;

    probability = clamp(probability, 1e-15L, 1.0L - 1e-15L);
    if (probability < LOW) {
        long double q = sqrtl(-2.0L * logl(probability));
        return (((((C[0] * q + C[1]) * q + C[2]) * q + C[3]) * q + C[4]) * q + C[5]) /
               ((((D[0] * q + D[1]) * q + D[2]) * q + D[3]) * q + 1.0L);
    }
    if (probability > HIGH) {
        long double q = sqrtl(-2.0L * logl(1.0L - probability));
        return -(((((C[0] * q + C[1]) * q + C[2]) * q + C[3]) * q + C[4]) * q + C[5]) /
               ((((D[0] * q + D[1]) * q + D[2]) * q + D[3]) * q + 1.0L);
    }

    long double q = probability - 0.5L;
    long double r = q * q;
    return (((((A[0] * r + A[1]) * r + A[2]) * r + A[3]) * r + A[4]) * r + A[5]) * q /
           (((((B[0] * r + B[1]) * r + B[2]) * r + B[3]) * r + B[4]) * r + 1.0L);
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
            long double compactness = 4.0L * sqrtl((long double)p) / perimeter;
            long double log_compactness = log2l(compactness);
            long double size_weight = probability * p;
            size_weight_sum += size_weight;
            weighted_log_sum += size_weight * log_compactness;
            weighted_log_square_sum += size_weight * log_compactness * log_compactness;
        }

        mean_log2_compactness = weighted_log_sum / size_weight_sum;
        variance_log2_compactness =
            weighted_log_square_sum / size_weight_sum - mean_log2_compactness * mean_log2_compactness;

        base_log_density_variance = 0.8L * 0.8L + variance_log2_compactness;
    }

    long double shadow_price(long double mean_log_duration, long double variance_log_duration,
                             long double rejected_fraction) const {
        if (rejected_fraction <= 0.0L) return 0.0L;
        const long double ln2 = logl(2.0L);
        long double mean_log_density = mean_log2_compactness - 0.1L * mean_log_duration / ln2;
        long double log_density_variance = base_log_density_variance + 0.01L * variance_log_duration / (ln2 * ln2);
        long double quantile = min(rejected_fraction, 1.0L - 1e-9L);
        return exp2l(mean_log_density + sqrtl(max(log_density_variance, 0.0L)) * inverse_standard_normal(quantile));
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

    ConditionalFutureDemand(ll current_s_, long double theta) : current_s(current_s_) {
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
            remaining_start_measure += joint_weight * (last_start - current_s);
        }
    }

    long double future_start_cdf(long double time) const {
        if (remaining_start_measure <= 0.0L || time <= current_s) {
            return 0.0L;
        }
        long double measure = 0.0L;
        for (const Node &node : nodes) {
            long double available_length = max(0.0L, node.last_start - (long double)current_s);
            long double prefix_length = clamp(time - (long double)current_s, 0.0L, available_length);
            measure += node.joint_weight * prefix_length;
        }
        return clamp(measure / remaining_start_measure, 0.0L, 1.0L);
    }

    FutureBucketDemand in_bucket(long double a, long double c, int remaining_groups,
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
                long double at_first = max(0.0L, boundary - current_s);
                long double at_last = max(0.0L, boundary - node.last_start);
                return 0.5L * (at_first * at_first - at_last * at_last);
            };
            long double integrated_overlap = integrated_positive_part(c) - integrated_positive_part(a) -
                                             integrated_positive_part(c - node.stay_duration) +
                                             integrated_positive_part(a - node.stay_duration);
            integrated_overlap = max(0.0L, integrated_overlap);

            long double weight = node.joint_weight * integrated_overlap;
            weight_sum += weight;
            weighted_log_sum += weight * node.log_duration;
            weighted_log_square_sum += weight * node.log_duration * node.log_duration;
        }
        if (weight_sum <= 0.0L) return result;

        result.cell_time = remaining_groups * expected_group_size * weight_sum / remaining_start_measure;
        result.mean_log_duration = weighted_log_sum / weight_sum;
        result.variance_log_duration =
            max(0.0L, weighted_log_square_sum / weight_sum - result.mean_log_duration * result.mean_log_duration);
        return result;
    }
};

struct ShadowEvaluation {
    long double opportunity_cost = 0.0L;
    long double duration_weighted_rejected_fraction = 0.0L;
    long double maximum_rejected_fraction = 0.0L;
    int priced_buckets = 0;
};

struct SampledDlpDiagnostics {
    int rebuilds = 0;
    int initial_rebuilds = 0;
    int scheduled_rebuilds = 0;
    int boundary_rebuilds = 0;
    int zero_future_calls = 0;
    int real_price_calls = 0;
    int rollout_price_calls = 0;
    int invalid_model_errors = 0;
    int nonfinite_errors = 0;
    long long generated_requests = 0;
    long long coordinate_updates = 0;
    long long positive_price_buckets = 0;
    long double dual_objective_sum = 0.0L;
    long double capacity_sum = 0.0L;
    long double offered_load_sum = 0.0L;
    long double opportunity_cost_sum = 0.0L;
    long double maximum_price = 0.0L;
    double rebuild_cpu_ms = 0.0;
    double maximum_rebuild_cpu_ms = 0.0;
    uint64_t sample_hash = 1469598103934665603ULL;
};

long double sampled_dlp_radical_inverse(uint64_t index, int base) {
    long double inverse_base = 1.0L / base;
    long double place = inverse_base;
    long double result = 0.0L;
    while (index > 0) {
        result += (index % base) * place;
        index /= base;
        place *= inverse_base;
    }
    return clamp(result, 1e-12L, 1.0L - 1e-12L);
}

// A periodically re-solved deterministic linear program replaces the
// independent 64-bucket tail approximation when sampled DLP is enabled.  It
// prices pooled cell-time only; geometry remains the responsibility of the
// unchanged placement and Push-out layers.
struct SampledDlpShadowModel {
    struct Request {
        ll s = 0;
        ll t = 0;
        int p = 0;
        ll ideal_fee = 0;
        array<long double, SAMPLED_DLP_BUCKET_COUNT> load{};
    };

    enum class RebuildTrigger {
        None,
        Initial,
        Scheduled,
        Boundary,
    };

    array<int, 151> minimum_perimeter{};
    vector<float> exact_future_survival;
    array<ll, SAMPLED_DLP_BUCKET_COUNT + 1> boundaries{};
    array<long double, SAMPLED_DLP_BUCKET_COUNT> prices{};
    int bucket_count = 0;
    bool ready = false;
    SampledDlpDiagnostics diagnostics;

    void initialize(const vector<vector<Shape>> &compact_shapes) {
        for (int p = 4; p <= 150; p++) {
            minimum_perimeter[p] = compact_shapes[p].front().perimeter;
        }

        // Q_theta(s)=Pr(S_future>s | theta) for the official rounded
        // duration distribution.  The backward recurrence is exact up to
        // floating point and needs one 61 x 100000 float table (about 24 MB).
        exact_future_survival.assign(
            (size_t)ThetaEstimator::PARTICLE_COUNT * ARRIVAL_TIME_HORIZON,
            0.0F);
        for (int k = 0; k < ThetaEstimator::PARTICLE_COUNT; k++) {
            long double theta = THETA_MIN + THETA_STEP * k;
            long double inverse_theta = 1.0L / theta;
            long double ratio = expl(-inverse_theta);
            long double left_tail = expl(-0.5L * inverse_theta);
            long double normalizer =
                -expm1l(-(ARRIVAL_TIME_HORIZON - 0.5L) * inverse_theta);
            long double reciprocal_start_mass = 0.0L;
            long double survival = 0.0L;
            for (int l = 0; l <= ARRIVAL_TIME_HORIZON - 2; l++) {
                long double unnormalized_mass =
                    l == 0 ? -expm1l(-0.5L * inverse_theta)
                           : left_tail * (1.0L - ratio);
                long double probability = unnormalized_mass / normalizer;
                reciprocal_start_mass += probability / (ARRIVAL_TIME_HORIZON - l);
                survival += reciprocal_start_mass;
                int s = ARRIVAL_TIME_HORIZON - l - 2;
                exact_future_survival[(size_t)k * ARRIVAL_TIME_HORIZON + s] =
                    (float)survival;
                if (l >= 1) left_tail *= ratio;
            }
        }
    }

    array<int, 5> exact_posterior_quantiles(const ThetaEstimator &theta_estimator,
                                            ll current_s, int remaining_groups) const {
        static constexpr array<long double, 5> PROBABILITIES = {
            0.10L, 0.30L, 0.50L, 0.70L, 0.90L,
        };
        array<long double, ThetaEstimator::PARTICLE_COUNT> log_weights{};
        long double maximum_log_weight = -numeric_limits<long double>::infinity();
        int positive_count = theta_estimator.observed_count - theta_estimator.rounded_zero_count;
        for (int k = 0; k < ThetaEstimator::PARTICLE_COUNT; k++) {
            long double theta = THETA_MIN + THETA_STEP * k;
            long double inverse_theta = 1.0L / theta;
            long double normalizer =
                -expm1l(-(ARRIVAL_TIME_HORIZON - 0.5L) * inverse_theta);
            long double log_weight =
                theta_estimator.rounded_zero_count * logl(-expm1l(-0.5L * inverse_theta)) +
                positive_count *
                    (logl(-expm1l(-inverse_theta)) + 0.5L * inverse_theta) -
                theta_estimator.exponential_sample_sum * inverse_theta -
                theta_estimator.observed_count * logl(normalizer);
            if (remaining_groups > 0) {
                log_weight +=
                    remaining_groups * logl((long double)exact_future_survival[
                                           (size_t)k * ARRIVAL_TIME_HORIZON + current_s]);
            }
            log_weights[k] = log_weight;
            chmax(maximum_log_weight, log_weight);
        }

        array<long double, ThetaEstimator::PARTICLE_COUNT> weights{};
        long double weight_sum = 0.0L;
        for (int k = 0; k < ThetaEstimator::PARTICLE_COUNT; k++) {
            weights[k] = expl(log_weights[k] - maximum_log_weight);
            weight_sum += weights[k];
        }

        array<int, 5> result{};
        for (int quantile = 0; quantile < 5; quantile++) {
            long double target = PROBABILITIES[quantile] * weight_sum;
            long double cumulative = 0.0L;
            result[quantile] = THETA_MAX;
            for (int k = 0; k < ThetaEstimator::PARTICLE_COUNT; k++) {
                cumulative += weights[k];
                if (cumulative >= target) {
                    result[quantile] = THETA_MIN + THETA_STEP * k;
                    break;
                }
            }
        }
        return result;
    }

    RebuildTrigger rebuild_trigger(int turn, ll current_s) const {
        if (!ready) return RebuildTrigger::Initial;
        if (turn == 4 || turn == 8 || turn == 16 || (turn > 16 && turn % 16 == 0)) {
            return RebuildTrigger::Scheduled;
        }
        int crossed = 0;
        for (int b = 1; b < bucket_count; b++) {
            if (boundaries[b] <= current_s) crossed++;
        }
        if (crossed >= 2) return RebuildTrigger::Boundary;
        return RebuildTrigger::None;
    }

    static uint64_t mix_hash(uint64_t hash, uint64_t value) {
        value += 0x9e3779b97f4a7c15ULL;
        value = (value ^ (value >> 30)) * 0xbf58476d1ce4e5b9ULL;
        value = (value ^ (value >> 27)) * 0x94d049bb133111ebULL;
        value ^= value >> 31;
        hash ^= value;
        hash *= 1099511628211ULL;
        return hash;
    }

    static vector<long double> make_conditional_duration_cdf(ll current_s, int theta) {
        int maximum_l = (int)(ARRIVAL_TIME_HORIZON - current_s - 2);
        if (maximum_l < 0) return {};

        vector<long double> cdf(maximum_l + 1);
        long double inverse_theta = 1.0L / theta;
        long double ratio = expl(-inverse_theta);
        long double left_tail = expl(-0.5L * inverse_theta);
        long double cumulative = 0.0L;
        for (int l = 0; l <= maximum_l; l++) {
            long double duration_mass =
                l == 0 ? -expm1l(-0.5L * inverse_theta) : left_tail * (1.0L - ratio);
            long double future_start_count = ARRIVAL_TIME_HORIZON - l - 1 - current_s;
            long double all_start_count = ARRIVAL_TIME_HORIZON - l;
            cumulative += duration_mass * future_start_count / all_start_count;
            cdf[l] = cumulative;
            if (l >= 1) left_tail *= ratio;
        }
        if (!(cumulative > 0.0L) || !isfinite(cumulative)) return {};
        for (long double &value : cdf) value /= cumulative;
        cdf.back() = 1.0L;
        return cdf;
    }

    void build_buckets(ll current_s) {
        ll remaining_time = ARRIVAL_TIME_HORIZON - current_s;
        bucket_count = (int)min<ll>(SAMPLED_DLP_BUCKET_COUNT, max(1LL, remaining_time));
        for (int b = 0; b <= bucket_count; b++) {
            boundaries[b] = current_s + remaining_time * b / bucket_count;
        }
        prices.fill(0.0L);
    }

    vector<Request> build_requests(ll current_s, int remaining_groups,
                                   const ThetaEstimator &theta_estimator) {
        array<int, 5> theta_values =
            exact_posterior_quantiles(theta_estimator, current_s, remaining_groups);

        vector<int> unique_theta;
        vector<vector<long double>> duration_cdfs;
        array<int, 5> cdf_index{};
        for (int k = 0; k < 5; k++) {
            auto found = find(unique_theta.begin(), unique_theta.end(), theta_values[k]);
            if (found == unique_theta.end()) {
                cdf_index[k] = unique_theta.size();
                unique_theta.push_back(theta_values[k]);
                duration_cdfs.push_back(make_conditional_duration_cdf(current_s, theta_values[k]));
            } else {
                cdf_index[k] = found - unique_theta.begin();
            }
        }

        vector<Request> requests;
        requests.reserve(SAMPLED_DLP_REQUEST_COUNT);
        const long double size_width = sqrtl(150.0L) - 2.0L;
        for (int sample = 0; sample < SAMPLED_DLP_REQUEST_COUNT; sample++) {
            uint64_t index = sample + 1;
            // The centered first coordinate gives the five posterior strata
            // counts 51, 51, 52, 51, 51 without correlating theta with P's
            // base-5 radical inverse.
            long double theta_quantile = (sample + 0.5L) / SAMPLED_DLP_REQUEST_COUNT;
            int theta_slot = min(4, (int)floorl(5.0L * theta_quantile));
            const vector<long double> &cdf = duration_cdfs[cdf_index[theta_slot]];
            if (cdf.empty()) continue;

            long double duration_quantile = sampled_dlp_radical_inverse(index, 2);
            int l = lower_bound(cdf.begin(), cdf.end(), duration_quantile) - cdf.begin();
            ll duration = l + 1;
            ll future_start_count = ARRIVAL_TIME_HORIZON - duration - current_s;
            if (future_start_count <= 0) continue;
            long double start_quantile = sampled_dlp_radical_inverse(index, 3);
            ll start = current_s + 1 +
                       min(future_start_count - 1,
                           (ll)floorl(start_quantile * future_start_count));

            long double size_quantile = sampled_dlp_radical_inverse(index, 5);
            long double root_size = 2.0L + size_width * size_quantile;
            int p = clamp((int)llroundl(root_size * root_size), 4, 150);
            long double value_quantile = sampled_dlp_radical_inverse(index, 7);
            long double noise = 0.8L * inverse_standard_normal(value_quantile);
            long double raw_v = p * powl((long double)duration, 0.9L) * exp2l(noise);
            ll v = clamp((ll)llroundl(raw_v), 1LL, 100000000LL);

            Request request;
            request.s = start;
            request.t = start + duration;
            request.p = p;
            request.ideal_fee = round_payment(v, p, minimum_perimeter[p]);
            for (int b = 0; b < bucket_count; b++) {
                ll overlap = max(0LL, min(request.t, boundaries[b + 1]) -
                                          max(request.s, boundaries[b]));
                request.load[b] = (long double)p * overlap;
            }
            requests.push_back(std::move(request));
        }
        return requests;
    }

    void solve_dual(const vector<Request> &requests, int remaining_groups,
                    const vector<GroupState> &groups, int grass_cells) {
        array<long double, SAMPLED_DLP_BUCKET_COUNT> capacity{};
        array<long double, SAMPLED_DLP_BUCKET_COUNT> offered_load{};
        long double sample_weight = (long double)remaining_groups / SAMPLED_DLP_REQUEST_COUNT;
        for (int b = 0; b < bucket_count; b++) {
            capacity[b] = (long double)grass_cells * (boundaries[b + 1] - boundaries[b]);
            for (const GroupState &group : groups) {
                if (!group.active) continue;
                ll overlap = max(0LL, min(group.t, boundaries[b + 1]) -
                                          max(group.s, boundaries[b]));
                capacity[b] -= (long double)group.p * overlap;
            }
            capacity[b] = max(0.0L, capacity[b]);
            for (const Request &request : requests) {
                offered_load[b] += sample_weight * request.load[b];
            }
            diagnostics.capacity_sum += capacity[b];
            diagnostics.offered_load_sum += offered_load[b];
        }

        struct Breakpoint {
            long double value;
            long double load;
            int request_index;
        };
        vector<Breakpoint> breakpoints;
        breakpoints.reserve(requests.size());
        for (int sweep = 0; sweep < SAMPLED_DLP_COORDINATE_SWEEPS; sweep++) {
            for (int b = 0; b < bucket_count; b++) {
                breakpoints.clear();
                long double active_load = 0.0L;
                for (int request_index = 0; request_index < (int)requests.size(); request_index++) {
                    const Request &request = requests[request_index];
                    long double a = request.load[b];
                    if (a <= 0.0L) continue;
                    long double residual = request.ideal_fee;
                    for (int c = 0; c < bucket_count; c++) {
                        if (c != b) residual -= prices[c] * request.load[c];
                    }
                    if (residual <= 0.0L) continue;
                    long double weighted_load = sample_weight * a;
                    breakpoints.push_back({residual / a, weighted_load, request_index});
                    active_load += weighted_load;
                }

                long double next_price = 0.0L;
                if (active_load > capacity[b]) {
                    sort(breakpoints.begin(), breakpoints.end(), [](const Breakpoint &lhs,
                                                                   const Breakpoint &rhs) {
                        if (lhs.value != rhs.value) return lhs.value < rhs.value;
                        return lhs.request_index < rhs.request_index;
                    });
                    long double remaining_load = active_load;
                    for (const Breakpoint &point : breakpoints) {
                        remaining_load -= point.load;
                        if (remaining_load <= capacity[b]) {
                            next_price = point.value;
                            break;
                        }
                    }
                }
                prices[b] = max(0.0L, next_price);
                diagnostics.coordinate_updates++;
            }
        }

        // Quantize only the completed solve.  Quantizing every coordinate
        // would perturb the later Gauss-Seidel breakpoints within each sweep.
        for (int b = 0; b < bucket_count; b++) {
            prices[b] = roundl(prices[b] * SAMPLED_DLP_PRICE_QUANTIZATION) /
                        SAMPLED_DLP_PRICE_QUANTIZATION;
        }

        long double dual_objective = 0.0L;
        for (int b = 0; b < bucket_count; b++) {
            dual_objective += prices[b] * capacity[b];
            if (prices[b] > 0.0L) diagnostics.positive_price_buckets++;
            chmax(diagnostics.maximum_price, prices[b]);
        }
        for (const Request &request : requests) {
            long double priced_load = 0.0L;
            for (int b = 0; b < bucket_count; b++) {
                priced_load += prices[b] * request.load[b];
            }
            dual_objective += sample_weight * max(0.0L, (long double)request.ideal_fee - priced_load);
        }
        if (!isfinite(dual_objective) || dual_objective < 0.0L) {
            diagnostics.nonfinite_errors++;
            prices.fill(0.0L);
            return;
        }
        diagnostics.dual_objective_sum += dual_objective;
    }

    void rebuild(int turn, ll current_s, int remaining_groups,
                 const vector<GroupState> &groups, int grass_cells,
                 const ThetaEstimator &theta_estimator, RebuildTrigger trigger) {
        clock_t cpu_begin = clock();
        build_buckets(current_s);
        vector<Request> requests = build_requests(current_s, remaining_groups, theta_estimator);
        diagnostics.rebuilds++;
        if (trigger == RebuildTrigger::Initial) diagnostics.initial_rebuilds++;
        if (trigger == RebuildTrigger::Scheduled) diagnostics.scheduled_rebuilds++;
        if (trigger == RebuildTrigger::Boundary) diagnostics.boundary_rebuilds++;
        diagnostics.generated_requests += requests.size();

        uint64_t rebuild_hash = mix_hash(1469598103934665603ULL, turn);
        rebuild_hash = mix_hash(rebuild_hash, current_s);
        rebuild_hash = mix_hash(rebuild_hash, remaining_groups);
        for (const Request &request : requests) {
            rebuild_hash = mix_hash(rebuild_hash, request.s);
            rebuild_hash = mix_hash(rebuild_hash, request.t);
            rebuild_hash = mix_hash(rebuild_hash, request.p);
            rebuild_hash = mix_hash(rebuild_hash, request.ideal_fee);
        }
        diagnostics.sample_hash = mix_hash(diagnostics.sample_hash, rebuild_hash);

        if ((int)requests.size() != SAMPLED_DLP_REQUEST_COUNT) {
            diagnostics.invalid_model_errors++;
            prices.fill(0.0L);
        } else {
            solve_dual(requests, remaining_groups, groups, grass_cells);
        }
        ready = true;
        double cpu_ms = 1000.0 * (double)(clock() - cpu_begin) / CLOCKS_PER_SEC;
        diagnostics.rebuild_cpu_ms += cpu_ms;
        chmax(diagnostics.maximum_rebuild_cpu_ms, cpu_ms);
    }

    ShadowEvaluation evaluate_cached(ll current_s, ll arrival_t, int p, bool rollout,
                                     int remaining_groups = -1) {
        ShadowEvaluation result;
        if (rollout) {
            diagnostics.rollout_price_calls++;
        } else {
            diagnostics.real_price_calls++;
        }
        if (remaining_groups == 0) {
            diagnostics.zero_future_calls++;
            return result;
        }
        if (!ready) {
            diagnostics.invalid_model_errors++;
            return result;
        }
        for (int b = 0; b < bucket_count; b++) {
            ll overlap = max(0LL, min(arrival_t, boundaries[b + 1]) -
                                      max(current_s, boundaries[b]));
            if (overlap <= 0) continue;
            result.opportunity_cost += (long double)p * overlap * prices[b];
            if (prices[b] > 0.0L) result.priced_buckets++;
        }
        if (!isfinite(result.opportunity_cost) || result.opportunity_cost < 0.0L) {
            diagnostics.nonfinite_errors++;
            result = ShadowEvaluation{};
        }
        diagnostics.opportunity_cost_sum += result.opportunity_cost;
        return result;
    }

    ShadowEvaluation evaluate_real_turn(int turn, ll current_s, ll arrival_t, int p,
                                        int remaining_groups, const vector<GroupState> &groups,
                                        int grass_cells, const ThetaEstimator &theta_estimator) {
        if (remaining_groups <= 0) {
            diagnostics.zero_future_calls++;
            diagnostics.real_price_calls++;
            return {};
        }
        RebuildTrigger trigger = rebuild_trigger(turn, current_s);
        if (trigger != RebuildTrigger::None) {
            rebuild(turn, current_s, remaining_groups, groups, grass_cells, theta_estimator, trigger);
        }
        return evaluate_cached(current_s, arrival_t, p, false);
    }
};

// Price the candidate's occupied cell-time by the fee density of future groups
// that would be crowded out.  Compact rescue is considered only after this
// ordinary admission decision has accepted the fallback placement.
ShadowEvaluation evaluate_shadow_cost(const vector<GroupState> &groups, ll current_s, ll arrival_t, int p,
                                      int remaining_groups, int grass_cells, long double theta,
                                      const DensityModel &density_model) {
    ShadowEvaluation result;
    if (remaining_groups <= 0) return result;

    const long double horizon = ARRIVAL_TIME_HORIZON;
    long double total_candidate_duration = arrival_t - current_s;
    ConditionalFutureDemand future_demand(current_s, theta);

    for (int bucket = 0; bucket < TIME_BUCKET_COUNT; bucket++) {
        long double bucket_begin = horizon * bucket / TIME_BUCKET_COUNT;
        long double bucket_end = horizon * (bucket + 1) / TIME_BUCKET_COUNT;
        long double a = max((long double)current_s, bucket_begin);
        long double c = bucket_end;
        long double candidate_end = min((long double)arrival_t, c);
        long double candidate_overlap = max(0.0L, candidate_end - a);
        if (candidate_overlap <= 0.0L) continue;

        long double committed_cell_time = 0.0L;
        for (const GroupState &group : groups) {
            if (!group.active) continue;
            long double overlap = max(0.0L, min((long double)group.t, c) - a);
            committed_cell_time += group.p * overlap;
        }

        long double interval_length = c - a;
        long double capacity = grass_cells * interval_length;
        long double available_capacity = max(0.0L, capacity - committed_cell_time);
        FutureBucketDemand bucket_demand =
            future_demand.in_bucket(a, c, remaining_groups, density_model.expected_group_size);
        long double future_cell_time = bucket_demand.cell_time;

        long double rejected_fraction = 0.0L;
        if (future_cell_time > available_capacity) {
            rejected_fraction = clamp(1.0L - available_capacity / future_cell_time, 0.0L, 1.0L);
        }
        long double price = density_model.shadow_price(bucket_demand.mean_log_duration,
                                                       bucket_demand.variance_log_duration, rejected_fraction);
        result.opportunity_cost += p * candidate_overlap * price;
        result.duration_weighted_rejected_fraction += candidate_overlap * rejected_fraction / total_candidate_duration;
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
    int actual_rejected_candidate_perimeter = 0;
    ll actual_rejected_candidate_fee = 0;
    int future_fit_evaluated_turns = 0;
    int future_fit_changed_placements = 0;
    int incremental_changed_from_absolute = 0;
    int final_changed_from_absolute = 0;
    long long anchors_checked = 0;
    long long legal_compact_candidates = 0;
    long long connected_growth_candidates = 0;
    long long grow_and_trim_base_candidates = 0;
    long long grow_and_trim_growth_failures = 0;
    long long grow_and_trim_full_growths = 0;
    long long grow_and_trim_trim_failures = 0;
    long long grow_and_trim_duplicate_candidates = 0;
    long long grow_and_trim_candidates = 0;
    long long grow_and_trim_grown_cells = 0;
    long long grow_and_trim_trimmed_cells = 0;
    long long grow_and_trim_perimeter_improvement = 0;
    long long grow_and_trim_perimeter_improved_candidates = 0;
    long long grow_and_trim_perimeter_equal_candidates = 0;
    long long grow_and_trim_perimeter_worsened_candidates = 0;
    long long grow_and_trim_shortlisted_candidates = 0;
    int grow_and_trim_successes = 0;
    long long shortlisted_candidates = 0;
    long long future_fit_snapshots = 0;
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
        uint64_t value = (uint64_t)(x * 64 + y + 1) + 0x9e3779b97f4a7c15ULL;
        value = (value ^ (value >> 30)) * 0xbf58476d1ce4e5b9ULL;
        value = (value ^ (value >> 27)) * 0x94d049bb133111ebULL;
        hash ^= value ^ (value >> 31);
    }
    return hash;
}

bool placement_increment_less(const PlacementCandidate &lhs, const PlacementCandidate &rhs) {
    if (lhs.incremental_cost != rhs.incremental_cost) {
        return lhs.incremental_cost < rhs.incremental_cost;
    }
    if (lhs.absolute_cost != rhs.absolute_cost) {
        return lhs.absolute_cost < rhs.absolute_cost;
    }
    return lhs.enumeration_order < rhs.enumeration_order;
}

bool placement_absolute_less(const PlacementCandidate &lhs, const PlacementCandidate &rhs) {
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
    void consider(int perimeter, long double incremental_cost, long double absolute_cost, long long enumeration_order,
                  int quadrant, PlacementSource source, Maker &&maker) {
        if (perimeter < best_perimeter) reset(perimeter);
        if (perimeter > best_perimeter) return;

        optional<PlacementCandidate> cache;
        auto get_candidate = [&]() -> const PlacementCandidate & {
            if (!cache) {
                vector<Cell> cells = maker();
                uint64_t region_hash = placement_region_hash(cells);
                cache = PlacementCandidate{std::move(cells), region_hash,       perimeter, incremental_cost,
                                           absolute_cost,    enumeration_order, quadrant,  source};
            }
            return *cache;
        };

        if (!first_candidate) first_candidate = get_candidate();

        PlacementCandidate key_candidate{{},       0,     perimeter, incremental_cost, absolute_cost, enumeration_order,
                                         quadrant, source};
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
                sort(global_best.begin(), global_best.end(), placement_increment_less);
                if ((int)global_best.size() > PLACEMENT_GLOBAL_SHORTLIST) {
                    global_best.pop_back();
                }
            }
        }

        if (!absolute_best || placement_absolute_less(key_candidate, *absolute_best)) {
            absolute_best = get_candidate();
        }
        if (!quadrant_best[quadrant] || placement_increment_less(key_candidate, *quadrant_best[quadrant])) {
            quadrant_best[quadrant] = get_candidate();
        }
    }

    vector<PlacementCandidate> finalize() const {
        vector<PlacementCandidate> result;
        auto add = [&](const optional<PlacementCandidate> &candidate) {
            if (!candidate) return;
            for (const PlacementCandidate &existing : result) {
                if (existing.region_hash == candidate->region_hash && same_region(existing.cells, candidate->cells)) {
                    return;
                }
            }
            result.push_back(*candidate);
        };
        auto add_value = [&](const PlacementCandidate &candidate) { add(optional<PlacementCandidate>(candidate)); };

        for (const PlacementCandidate &candidate : global_best) {
            add_value(candidate);
        }
        add(absolute_best);
        add(first_candidate);

        int primary_quadrant = global_best.empty() ? -1 : global_best.front().quadrant;
        optional<PlacementCandidate> diverse;
        for (int quadrant = 0; quadrant < 4; quadrant++) {
            if (quadrant == primary_quadrant || !quadrant_best[quadrant]) {
                continue;
            }
            if (!diverse || placement_increment_less(*quadrant_best[quadrant], *diverse)) {
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

bool validate_connected_region(const vector<Cell> &cells, int n);

// A greedy connected prefix can end with an avoidable spike merely because it
// stops at exactly P cells.  Continue the same growth for eight cells, then
// peel non-articulation boundary cells.  This is only a candidate generator:
// admission, shadow, future-fit, relocation, and every downstream evaluator
// remain unchanged.
optional<vector<Cell>> trim_grown_connected_region(const vs &park, const vvi &owner,
                                                   const vector<Cell> &grown, int p,
                                                   TemporalPlacementDiagnostics &diagnostics) {
    int n = park.size();
    vector<char> selected(n * n, false);
    vector<int> growth_order(n * n, -1);
    for (int index = 0; index < (int)grown.size(); index++) {
        auto [x, y] = grown[index];
        int cell = x * n + y;
        selected[cell] = true;
        growth_order[cell] = index;
    }

    constexpr int DX[4] = {-1, 1, 0, 0};
    constexpr int DY[4] = {0, 0, -1, 1};
    int remaining = grown.size();
    vector<int> discovery(n * n);
    vector<int> low(n * n);
    vector<int> parent(n * n);
    vector<char> articulation(n * n);
    while (remaining > p) {
        for (const Cell &position : grown) {
            int cell = position.first * n + position.second;
            if (!selected[cell]) continue;
            discovery[cell] = -1;
            low[cell] = -1;
            parent[cell] = -1;
            articulation[cell] = false;
        }
        int timer = 0;

        function<void(int)> dfs = [&](int cell) {
            discovery[cell] = low[cell] = timer++;
            int children = 0;
            int x = cell / n;
            int y = cell % n;
            for (int dir = 0; dir < 4; dir++) {
                int nx = x + DX[dir];
                int ny = y + DY[dir];
                if (!inside(nx, ny, n, n)) continue;
                int next = nx * n + ny;
                if (!selected[next]) continue;
                if (discovery[next] == -1) {
                    parent[next] = cell;
                    children++;
                    dfs(next);
                    chmin(low[cell], low[next]);
                    if (parent[cell] == -1 && children > 1) articulation[cell] = true;
                    if (parent[cell] != -1 && low[next] >= discovery[cell]) articulation[cell] = true;
                } else if (next != parent[cell]) {
                    chmin(low[cell], discovery[next]);
                }
            }
        };

        for (const Cell &position : grown) {
            int cell = position.first * n + position.second;
            if (selected[cell] && discovery[cell] == -1) dfs(cell);
        }

        int removed = -1;
        int best_perimeter_change = numeric_limits<int>::max();
        int best_growth_order = -1;
        for (const Cell &position : grown) {
            int cell = position.first * n + position.second;
            if (!selected[cell] || articulation[cell]) continue;
            int x = cell / n;
            int y = cell % n;
            int selected_neighbors = 0;
            for (int dir = 0; dir < 4; dir++) {
                int nx = x + DX[dir];
                int ny = y + DY[dir];
                if (inside(nx, ny, n, n) && selected[nx * n + ny]) selected_neighbors++;
            }
            if (selected_neighbors == 4) continue;
            int perimeter_change = 2 * selected_neighbors - 4;
            if (perimeter_change < best_perimeter_change ||
                (perimeter_change == best_perimeter_change && growth_order[cell] > best_growth_order) ||
                (perimeter_change == best_perimeter_change && growth_order[cell] == best_growth_order &&
                 (removed == -1 || cell < removed))) {
                removed = cell;
                best_perimeter_change = perimeter_change;
                best_growth_order = growth_order[cell];
            }
        }
        if (removed == -1) return nullopt;
        selected[removed] = false;
        remaining--;
        diagnostics.grow_and_trim_trimmed_cells++;
    }

    vector<Cell> result;
    result.reserve(p);
    for (const Cell &cell : grown) {
        if (selected[cell.first * n + cell.second]) result.push_back(cell);
    }
    if ((int)result.size() != p || !validate_connected_region(result, n)) return nullopt;
    for (auto [x, y] : result) {
        if (park[x][y] != '.' || owner[x][y] != -1) return nullopt;
    }
    return result;
}

vector<vector<Cell>> make_connected_growth_candidates(
    const vs &park, const vvi &owner, int p,
    vector<vector<Cell>> &grow_and_trim_candidates,
    TemporalPlacementDiagnostics &diagnostics) {
    int n = park.size();
    constexpr int DX[4] = {-1, 1, 0, 0};
    constexpr int DY[4] = {0, 0, -1, 1};
    vector<vector<Cell>> candidates;
    vector<vector<Cell>> completed_grow_and_trim;
    grow_and_trim_candidates.clear();

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
            if (visited[start_x][start_y] || park[start_x][start_y] == '#' || owner[start_x][start_y] != -1) {
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
         [&](int lhs, int rhs) { return components[lhs].size() > components[rhs].size(); });

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
            case 0:
                return pair<int, int>{x, y};
            case 1:
                return pair<int, int>{-x, y};
            case 2:
                return pair<int, int>{y, x};
            case 3:
                return pair<int, int>{-y, x};
            case 4:
                return pair<int, int>{x + y, x};
            case 5:
                return pair<int, int>{-(x + y), x};
            case 6:
                return pair<int, int>{x - y, x};
            case 7:
                return pair<int, int>{-(x - y), x};
            case 8:
                return pair<int, int>{obstacle_distance[x][y], x * n + y};
            default:
                return pair<int, int>{-obstacle_distance[x][y], x * n + y};
        }
    };
    for (int component_id = 0; component_id < (int)components.size(); component_id++) {
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
    for (int feature = 0; feature < SEED_FEATURE_COUNT && (int)seeds.size() < CONNECTED_GROWTH_SEED_LIMIT; feature++) {
        for (int order_index = 0;
             order_index < (int)component_order.size() && (int)seeds.size() < CONNECTED_GROWTH_SEED_LIMIT;
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
        return tuple(lhs.selected_neighbors, -lhs.distance, -lhs.bias_key, -lhs.cell) <
               tuple(rhs.selected_neighbors, -rhs.distance, -rhs.bias_key, -rhs.cell);
    };

    int grow_and_trim_attempts = 0;
    for (const Seed &seed_info : seeds) {
        int seed_x = seed_info.cell.first;
        int seed_y = seed_info.cell.second;
        vector<char> selected(n * n, false);
        vector<Cell> region;
        region.reserve(p + GROW_AND_TRIM_EXTRA_CELLS);
        priority_queue<GrowthEntry, vector<GrowthEntry>, decltype(entry_worse)> frontier(entry_worse);

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
            if (!inside(x, y, n, n) || park[x][y] == '#' || owner[x][y] != -1 || selected[x * n + y]) {
                return;
            }
            int cell = x * n + y;
            frontier.push({cell, count_selected_neighbors(cell), abs(x - seed_x) + abs(y - seed_y), bias_key(x, y)});
        };
        auto select_cell = [&](int x, int y) {
            selected[x * n + y] = true;
            region.emplace_back(x, y);
            for (int dir = 0; dir < 4; dir++) {
                push_frontier(x + DX[dir], y + DY[dir]);
            }
        };

        auto grow_until = [&](int target_size) {
            while ((int)region.size() < target_size && !frontier.empty()) {
                GrowthEntry entry = frontier.top();
                frontier.pop();
                if (selected[entry.cell]) continue;
                int current_neighbors = count_selected_neighbors(entry.cell);
                if (current_neighbors != entry.selected_neighbors) {
                    int x = entry.cell / n;
                    int y = entry.cell % n;
                    frontier.push(
                        {entry.cell, current_neighbors, abs(x - seed_x) + abs(y - seed_y), bias_key(x, y)});
                    continue;
                }
                select_cell(entry.cell / n, entry.cell % n);
            }
        };

        select_cell(seed_x, seed_y);
        grow_until(p);
        if ((int)region.size() == p) {
            if constexpr (ENABLE_GROW_AND_TRIM) {
                // Preserve the legacy P-cell candidate and its enumeration
                // order.  The refined candidate is an addition, never a
                // replacement.
                add_candidate(optional<vector<Cell>>(region));
                if (grow_and_trim_attempts < GROW_AND_TRIM_CANDIDATE_LIMIT) {
                    grow_and_trim_attempts++;
                    diagnostics.grow_and_trim_base_candidates++;
                    int base_perimeter = calc_perimeter(region, n);
                    grow_until(p + GROW_AND_TRIM_EXTRA_CELLS);
                    diagnostics.grow_and_trim_grown_cells += region.size() - p;
                    if ((int)region.size() != p + GROW_AND_TRIM_EXTRA_CELLS) {
                        diagnostics.grow_and_trim_growth_failures++;
                    } else {
                        diagnostics.grow_and_trim_full_growths++;
                        optional<vector<Cell>> trimmed =
                            trim_grown_connected_region(park, owner, region, p, diagnostics);
                        if (!trimmed) {
                            diagnostics.grow_and_trim_trim_failures++;
                        } else {
                            int improvement = base_perimeter - calc_perimeter(*trimmed, n);
                            diagnostics.grow_and_trim_perimeter_improvement += improvement;
                            if (improvement > 0) {
                                diagnostics.grow_and_trim_perimeter_improved_candidates++;
                            } else if (improvement == 0) {
                                diagnostics.grow_and_trim_perimeter_equal_candidates++;
                            } else {
                                diagnostics.grow_and_trim_perimeter_worsened_candidates++;
                            }
                            completed_grow_and_trim.push_back(std::move(*trimmed));
                        }
                    }
                }
            } else {
                add_candidate(optional<vector<Cell>>(std::move(region)));
            }
        }
    }

    if constexpr (ENABLE_GROW_AND_TRIM) {
        for (vector<Cell> &candidate : completed_grow_and_trim) {
            bool duplicate = false;
            for (const vector<Cell> &existing : candidates) {
                if (same_region(existing, candidate)) {
                    duplicate = true;
                    break;
                }
            }
            if (!duplicate) {
                for (const vector<Cell> &existing : grow_and_trim_candidates) {
                    if (same_region(existing, candidate)) {
                        duplicate = true;
                        break;
                    }
                }
            }
            if (duplicate) {
                diagnostics.grow_and_trim_duplicate_candidates++;
            } else {
                grow_and_trim_candidates.push_back(std::move(candidate));
            }
        }
        diagnostics.grow_and_trim_candidates += grow_and_trim_candidates.size();
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

long double compact_fit_utility(const vs &park, const vvi &owner, const vector<GroupState> &groups,
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
            bool is_free = park[x][y] == '#'
                               ? false
                               : !in_candidate[cell] && (occupied_by == -1 || groups[occupied_by].t < snapshot_time);
            if (!is_free) continue;
            current[y + 1] = 1 + min({previous[y + 1], current[y], previous[y]});
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

array<ll, FUTURE_FIT_SNAPSHOT_COUNT> make_future_fit_snapshots(const ConditionalFutureDemand &future_demand,
                                                               ll current_s, ll arrival_t) {
    array<ll, FUTURE_FIT_SNAPSHOT_COUNT> snapshots{};
    long double total_mass = future_demand.future_start_cdf(arrival_t);
    for (int index = 0; index < FUTURE_FIT_SNAPSHOT_COUNT; index++) {
        long double fraction = (2.0L * index + 1.0L) / (2.0L * FUTURE_FIT_SNAPSHOT_COUNT);
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

long double evaluate_compact_fit(const vs &park, const vvi &owner, const vector<GroupState> &groups,
                                 const vector<Cell> &candidate, const array<ll, FUTURE_FIT_SNAPSHOT_COUNT> &snapshots) {
    int n = park.size();
    vector<char> in_candidate(n * n, false);
    for (auto [x, y] : candidate) in_candidate[x * n + y] = true;

    long double sum = 0.0L;
    long double minimum = numeric_limits<long double>::infinity();
    for (ll snapshot : snapshots) {
        long double utility = compact_fit_utility(park, owner, groups, in_candidate, snapshot);
        sum += utility;
        chmin(minimum, utility);
    }
    long double average = sum / FUTURE_FIT_SNAPSHOT_COUNT;
    return 0.75L * average + 0.25L * minimum;
}

optional<NormalPlacementChoice> choose_temporally_coherent_region(const vs &park, const vvi &owner,
                                                                  const vector<GroupState> &groups, ll current_s,
                                                                  ll arrival_t, int p, long double theta,
                                                                  int remaining_groups, const vector<Shape> &shapes,
                                                                  TemporalPlacementDiagnostics &diagnostics,
                                                                  vector<NormalPlacementChoice> *root_alternatives = nullptr) {
    // Prefer boundaries next to groups with similar release timing.  Only a
    // small spatial shortlist proceeds to the more expensive future-fit test.
    if (root_alternatives) root_alternatives->clear();
    diagnostics.attempts++;
    int n = park.size();
    vector<vi> blocked_prefix = make_blocked_prefix(park, owner);

    ConditionalFutureDemand future_demand(current_s, theta);
    long double candidate_arrival_level = future_demand.future_start_cdf(arrival_t);

    auto release_level = [&](ll release_time) {
        long double remaining = max(0LL, release_time - current_s);
        return -expm1l(-remaining / theta);
    };
    long double candidate_release_level = release_level(arrival_t);
    vector<long double> group_arrival_level(groups.size(), -1.0L);
    vector<long double> group_release_level(groups.size(), -1.0L);

    vector<vector<long double>> incremental_cell(n, vector<long double>(n));
    vector<vector<long double>> absolute_cell(n, vector<long double>(n));
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
                        group_arrival_level[adjacent_owner] = future_demand.future_start_cdf(groups[adjacent_owner].t);
                        group_release_level[adjacent_owner] = release_level(groups[adjacent_owner].t);
                    }
                    adjacent_arrival_level = group_arrival_level[adjacent_owner];
                    adjacent_release_level = group_release_level[adjacent_owner];
                }
                incremental_cell[x][y] +=
                    fabsl(candidate_arrival_level - adjacent_arrival_level) - adjacent_arrival_level;
                absolute_cell[x][y] += fabsl(candidate_release_level - adjacent_release_level);
            }
        }
    }

    auto make_prefix = [&](const vector<vector<long double>> &values) {
        vector<vector<long double>> prefix(n + 1, vector<long double>(n + 1));
        for (int x = 0; x < n; x++) {
            for (int y = 0; y < n; y++) {
                prefix[x + 1][y + 1] = values[x][y] + prefix[x][y + 1] + prefix[x + 1][y] - prefix[x][y];
            }
        }
        return prefix;
    };
    vector<vector<long double>> incremental_prefix = make_prefix(incremental_cell);
    vector<vector<long double>> absolute_prefix = make_prefix(absolute_cell);

    PlacementShortlistBuilder shortlist_builder;
    long long enumeration_order = 0;
    int minimum_perimeter = shapes.front().perimeter;

    auto scan_shape = [&](const Shape &shape, PlacementSource source) {
        bool found_legal = false;
        auto relative_coordinate_sum = [](const Rect &rect, bool x_axis) {
            long long coordinate = x_axis ? rect.x : rect.y;
            long long length = x_axis ? rect.h : rect.w;
            long long copies = x_axis ? rect.w : rect.h;
            return copies * (length * coordinate + length * (length - 1) / 2);
        };
        long long relative_sum_x =
            relative_coordinate_sum(shape.main_rect, true) + relative_coordinate_sum(shape.extra_rect, true);
        long long relative_sum_y =
            relative_coordinate_sum(shape.main_rect, false) + relative_coordinate_sum(shape.extra_rect, false);
        int max_x = n - shape.h;
        int max_y = n - shape.w;
        for (int base_x = 0; base_x <= max_x; base_x++) {
            for (int base_y = 0; base_y <= max_y; base_y++) {
                diagnostics.anchors_checked++;
                const Rect &main_rect = shape.main_rect;
                const Rect &extra_rect = shape.extra_rect;
                if (rectangle_sum(blocked_prefix, base_x + main_rect.x, base_y + main_rect.y, main_rect.h,
                                  main_rect.w) != 0) {
                    continue;
                }
                if (rectangle_sum(blocked_prefix, base_x + extra_rect.x, base_y + extra_rect.y, extra_rect.h,
                                  extra_rect.w) != 0) {
                    continue;
                }
                diagnostics.legal_compact_candidates++;
                found_legal = true;

                long double incremental_cost = rectangle_sum(incremental_prefix, base_x + main_rect.x,
                                                             base_y + main_rect.y, main_rect.h, main_rect.w) +
                                               rectangle_sum(incremental_prefix, base_x + extra_rect.x,
                                                             base_y + extra_rect.y, extra_rect.h, extra_rect.w) -
                                               (4 * p - shape.perimeter) * candidate_arrival_level;
                long double absolute_cost = rectangle_sum(absolute_prefix, base_x + main_rect.x, base_y + main_rect.y,
                                                          main_rect.h, main_rect.w) +
                                            rectangle_sum(absolute_prefix, base_x + extra_rect.x, base_y + extra_rect.y,
                                                          extra_rect.h, extra_rect.w) -
                                            (4 * p - shape.perimeter) * candidate_release_level;
                long long sum_x = (long long)p * base_x + relative_sum_x;
                long long sum_y = (long long)p * base_y + relative_sum_y;
                int lower_half = 2 * sum_x >= (long long)p * n;
                int right_half = 2 * sum_y >= (long long)p * n;
                int quadrant = 2 * lower_half + right_half;
                long long order = enumeration_order++;
                shortlist_builder.consider(shape.perimeter, incremental_cost, absolute_cost, order, quadrant, source,
                                           [&] { return materialize_shape(shape, base_x, base_y, p); });
            }
        }
        return found_legal;
    };

    bool found_minimum_template = false;
    for (const Shape &shape : shapes) {
        if (shape.perimeter != minimum_perimeter) continue;
        found_minimum_template |= scan_shape(shape, PlacementSource::MinimumTemplate);
    }

    if (!found_minimum_template) {
        // Shapes are sorted by perimeter.  Once one extended tier has a
        // legal placement, every later tier is strictly worse in immediate
        // perimeter, so it cannot survive the minimum-perimeter collector.
        for (size_t first = 0; first < shapes.size();) {
            size_t last = first + 1;
            while (last < shapes.size() && shapes[last].perimeter == shapes[first].perimeter) {
                last++;
            }
            if (shapes[first].perimeter > minimum_perimeter) {
                bool found_in_tier = false;
                for (size_t index = first; index < last; index++) {
                    found_in_tier |= scan_shape(shapes[index], PlacementSource::ExtendedTemplate);
                }
                if (found_in_tier) break;
            }
            first = last;
        }

        vector<vector<Cell>> grow_and_trim_candidates;
        vector<vector<Cell>> growth_candidates =
            make_connected_growth_candidates(park, owner, p, grow_and_trim_candidates, diagnostics);
        diagnostics.connected_growth_candidates += growth_candidates.size();
        for (vector<Cell> &region : growth_candidates) {
            int perimeter = calc_perimeter(region, n);
            long double incremental_cost = 0.0L;
            long double absolute_cost = 0.0L;
            for (auto [x, y] : region) {
                incremental_cost += incremental_cell[x][y];
                absolute_cost += absolute_cell[x][y];
            }
            incremental_cost -= (4 * p - perimeter) * candidate_arrival_level;
            absolute_cost -= (4 * p - perimeter) * candidate_release_level;
            int quadrant = placement_quadrant(region, n);
            long long order = enumeration_order++;
            shortlist_builder.consider(perimeter, incremental_cost, absolute_cost, order, quadrant,
                                       PlacementSource::ConnectedGrowth, [&] { return region; });
        }
        for (vector<Cell> &region : grow_and_trim_candidates) {
            int perimeter = calc_perimeter(region, n);
            long double incremental_cost = 0.0L;
            long double absolute_cost = 0.0L;
            for (auto [x, y] : region) {
                incremental_cost += incremental_cell[x][y];
                absolute_cost += absolute_cell[x][y];
            }
            incremental_cost -= (4 * p - perimeter) * candidate_arrival_level;
            absolute_cost -= (4 * p - perimeter) * candidate_release_level;
            int quadrant = placement_quadrant(region, n);
            long long order = enumeration_order++;
            shortlist_builder.consider(perimeter, incremental_cost, absolute_cost, order, quadrant,
                                       PlacementSource::GrowAndTrim, [&] { return region; });
        }
    }

    vector<PlacementCandidate> candidates = shortlist_builder.finalize();
    if (candidates.empty()) return nullopt;
    diagnostics.shortlisted_candidates += candidates.size();
    diagnostics.grow_and_trim_shortlisted_candidates +=
        count_if(candidates.begin(), candidates.end(), [](const PlacementCandidate &candidate) {
            return candidate.source == PlacementSource::GrowAndTrim;
        });

    int incremental_best = 0;
    int absolute_best = 0;
    for (int index = 1; index < (int)candidates.size(); index++) {
        if (placement_increment_less(candidates[index], candidates[incremental_best])) {
            incremental_best = index;
        }
        if (placement_absolute_less(candidates[index], candidates[absolute_best])) {
            absolute_best = index;
        }
    }
    if (!same_region(candidates[incremental_best].cells, candidates[absolute_best].cells)) {
        diagnostics.incremental_changed_from_absolute++;
    }
    int best_index = incremental_best;

    long double future_mass = future_demand.future_start_cdf(arrival_t);
    vector<long double> future_fit_values;
    bool used_future_fit = false;
    if ((int)candidates.size() >= 2 && remaining_groups > 0 && arrival_t - current_s > 1 && future_mass > 1e-12L) {
        used_future_fit = true;
        if (root_alternatives) future_fit_values.resize(candidates.size());
        array<ll, FUTURE_FIT_SNAPSHOT_COUNT> snapshots = make_future_fit_snapshots(future_demand, current_s, arrival_t);
        long double best_fit = -numeric_limits<long double>::infinity();
        for (int index = 0; index < (int)candidates.size(); index++) {
            long double fit = evaluate_compact_fit(park, owner, groups, candidates[index].cells, snapshots);
            if (root_alternatives) future_fit_values[index] = fit;
            diagnostics.future_fit_snapshots += FUTURE_FIT_SNAPSHOT_COUNT;
            if (fit > best_fit + 1e-15L || (fabsl(fit - best_fit) <= 1e-15L &&
                                            placement_increment_less(candidates[index], candidates[best_index]))) {
                best_fit = fit;
                best_index = index;
            }
        }
        diagnostics.future_fit_evaluated_turns++;
        if (!same_region(candidates[incremental_best].cells, candidates[best_index].cells)) {
            diagnostics.future_fit_changed_placements++;
        }
    }

    if (!same_region(candidates[best_index].cells, candidates[absolute_best].cells)) {
        diagnostics.final_changed_from_absolute++;
    }
    // The real-arrival caller may compare a few ordinary runner-ups at the
    // root.  Reuse the evaluated shortlist and apply the exact same ranking;
    // synthetic rollout arrivals do not request this list.
    if (root_alternatives && candidates.size() >= 2) {
        vector<char> chosen(candidates.size(), false);
        chosen[best_index] = true;
        while ((int)root_alternatives->size() < ROOT_ROLLOUT_NORMAL_ALTERNATIVE_LIMIT) {
            int alternative_index = -1;
            for (int index = 0; index < (int)candidates.size(); index++) {
                if (chosen[index]) continue;
                bool better = alternative_index == -1;
                if (!better && used_future_fit) {
                    better = future_fit_values[index] > future_fit_values[alternative_index] + 1e-15L ||
                             (fabsl(future_fit_values[index] - future_fit_values[alternative_index]) <= 1e-15L &&
                              placement_increment_less(candidates[index], candidates[alternative_index]));
                } else if (!better) {
                    better = placement_increment_less(candidates[index], candidates[alternative_index]);
                }
                if (better) alternative_index = index;
            }
            if (alternative_index == -1) break;
            chosen[alternative_index] = true;
            root_alternatives->push_back(
                NormalPlacementChoice{candidates[alternative_index].cells, candidates[alternative_index].perimeter,
                                      candidates[alternative_index].source});
        }
    }
    PlacementCandidate choice = std::move(candidates[best_index]);
    if (choice.source == PlacementSource::ConnectedGrowth || choice.source == PlacementSource::GrowAndTrim) {
        diagnostics.fallback_successes++;
        if (choice.source == PlacementSource::GrowAndTrim) diagnostics.grow_and_trim_successes++;
    } else {
        diagnostics.compact_successes++;
        if (choice.source == PlacementSource::ExtendedTemplate) {
            diagnostics.extended_template_successes++;
        }
    }
    return NormalPlacementChoice{std::move(choice.cells), choice.perimeter, choice.source};
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

bool validate_connected_region(const vector<Cell> &cells, int n);

enum class ArrivalStatus {
    UpperBoundRejected,
    NoRegion,
    ActualFeeRejected,
    Accepted,
};

struct ArrivalDecision {
    optional<vector<Cell>> cells;
    int perimeter = 0;
    ll fee = 0;
    ArrivalStatus status = ArrivalStatus::NoRegion;
    TemporalPlacementDiagnostics diagnostics;
};

void accumulate_placement_diagnostics(TemporalPlacementDiagnostics &total,
                                      const TemporalPlacementDiagnostics &part) {
    total.attempts += part.attempts;
    total.compact_successes += part.compact_successes;
    total.extended_template_successes += part.extended_template_successes;
    total.fallback_successes += part.fallback_successes;
    total.actual_rejected_candidate_perimeter += part.actual_rejected_candidate_perimeter;
    total.actual_rejected_candidate_fee += part.actual_rejected_candidate_fee;
    total.future_fit_evaluated_turns += part.future_fit_evaluated_turns;
    total.future_fit_changed_placements += part.future_fit_changed_placements;
    total.incremental_changed_from_absolute += part.incremental_changed_from_absolute;
    total.final_changed_from_absolute += part.final_changed_from_absolute;
    total.anchors_checked += part.anchors_checked;
    total.legal_compact_candidates += part.legal_compact_candidates;
    total.connected_growth_candidates += part.connected_growth_candidates;
    total.grow_and_trim_base_candidates += part.grow_and_trim_base_candidates;
    total.grow_and_trim_growth_failures += part.grow_and_trim_growth_failures;
    total.grow_and_trim_full_growths += part.grow_and_trim_full_growths;
    total.grow_and_trim_trim_failures += part.grow_and_trim_trim_failures;
    total.grow_and_trim_duplicate_candidates += part.grow_and_trim_duplicate_candidates;
    total.grow_and_trim_candidates += part.grow_and_trim_candidates;
    total.grow_and_trim_grown_cells += part.grow_and_trim_grown_cells;
    total.grow_and_trim_trimmed_cells += part.grow_and_trim_trimmed_cells;
    total.grow_and_trim_perimeter_improvement += part.grow_and_trim_perimeter_improvement;
    total.grow_and_trim_perimeter_improved_candidates +=
        part.grow_and_trim_perimeter_improved_candidates;
    total.grow_and_trim_perimeter_equal_candidates +=
        part.grow_and_trim_perimeter_equal_candidates;
    total.grow_and_trim_perimeter_worsened_candidates +=
        part.grow_and_trim_perimeter_worsened_candidates;
    total.grow_and_trim_shortlisted_candidates += part.grow_and_trim_shortlisted_candidates;
    total.grow_and_trim_successes += part.grow_and_trim_successes;
    total.shortlisted_candidates += part.shortlisted_candidates;
    total.future_fit_snapshots += part.future_fit_snapshots;
}

void remove_selected_placement_success(TemporalPlacementDiagnostics &diagnostics) {
    if (diagnostics.fallback_successes > 0) {
        diagnostics.fallback_successes--;
        if (diagnostics.grow_and_trim_successes > 0) diagnostics.grow_and_trim_successes--;
        return;
    }
    if (diagnostics.compact_successes > 0) diagnostics.compact_successes--;
    if (diagnostics.extended_template_successes > 0) diagnostics.extended_template_successes--;
}

void replace_selected_placement_success(TemporalPlacementDiagnostics &diagnostics,
                                        PlacementSource source) {
    remove_selected_placement_success(diagnostics);
    if (source == PlacementSource::ConnectedGrowth || source == PlacementSource::GrowAndTrim) {
        diagnostics.fallback_successes++;
        if (source == PlacementSource::GrowAndTrim) diagnostics.grow_and_trim_successes++;
    } else {
        diagnostics.compact_successes++;
        if (source == PlacementSource::ExtendedTemplate) diagnostics.extended_template_successes++;
    }
}

ArrivalDecision evaluate_arrival_decision(const vs &park, const vvi &decision_owner,
                                          const vector<GroupState> &groups, int arrival_id, ll current_s,
                                          int remaining_groups, long double theta, long double opportunity_cost,
                                          const vector<vector<Shape>> &compact_shapes,
                                          vector<NormalPlacementChoice> *root_alternatives = nullptr) {
    ArrivalDecision result;
    if (root_alternatives) root_alternatives->clear();
    const GroupState &arrival = groups[arrival_id];
    int minimum_perimeter = compact_shapes[arrival.p].front().perimeter;
    ll upper_bound_fee = round_payment(arrival.v, arrival.p, minimum_perimeter);
    if ((long double)upper_bound_fee <= opportunity_cost) {
        result.status = ArrivalStatus::UpperBoundRejected;
        return result;
    }

    optional<NormalPlacementChoice> placement =
        choose_temporally_coherent_region(park, decision_owner, groups, current_s, arrival.t, arrival.p, theta,
                                          remaining_groups, compact_shapes[arrival.p], result.diagnostics,
                                          root_alternatives);
    if (!placement) {
        result.status = ArrivalStatus::NoRegion;
        return result;
    }

    ll actual_fee = round_payment(arrival.v, arrival.p, placement->perimeter);
    if ((long double)actual_fee <= opportunity_cost) {
        if (root_alternatives) root_alternatives->clear();
        result.diagnostics.actual_rejected_candidate_perimeter = placement->perimeter;
        result.diagnostics.actual_rejected_candidate_fee = actual_fee;
        result.status = ArrivalStatus::ActualFeeRejected;
        return result;
    }
    if (root_alternatives) {
        root_alternatives->erase(
            remove_if(root_alternatives->begin(), root_alternatives->end(), [&](const NormalPlacementChoice &choice) {
                ll alternative_fee = round_payment(arrival.v, arrival.p, choice.perimeter);
                return (long double)alternative_fee <= opportunity_cost;
            }),
            root_alternatives->end());
    }
    result.cells = std::move(placement->cells);
    result.perimeter = placement->perimeter;
    result.fee = actual_fee;
    result.status = ArrivalStatus::Accepted;
    return result;
}

TurnPlan make_arrival_plan(const ArrivalDecision &decision) {
    TurnPlan plan;
    if (decision.cells) {
        plan.arrival = *decision.cells;
        plan.arrival_perimeter = decision.perimeter;
    }
    plan.immediate_gain = decision.fee;
    return plan;
}

// This is a deliberately infeasible upper bound: every offered group is
// assumed to be accepted at its minimum possible perimeter, with no capacity
// conflict.  Its gap from the realized money is nevertheless an exact way to
// separate geometry loss, rejected value, and movement cost.
struct LossDiagnostics {
    int observed = 0;
    int accepted = 0;
    int upper_rejected = 0;
    int actual_rejected = 0;
    int no_region_rejected = 0;
    int rejected_status_mismatch = 0;
    int rejected_feasible = 0;
    int rejected_unplaceable = 0;
    int upper_rejected_feasible = 0;
    int upper_rejected_unplaceable = 0;
    int unplaceable_static = 0;
    int unplaceable_capacity = 0;
    int unplaceable_fragmentation = 0;
    int feasibility_mismatches = 0;
    int accepted_status_mismatches = 0;
    int accepted_plan_mismatches = 0;
    int accepted_source_mismatches = 0;
    int rejected_move_plans = 0;
    int finalized_accepted = 0;
    // minimum template, extended template, connected growth, unclassified
    array<int, 4> accepted_by_source{};
    int accepted_grow_and_trim = 0;

    ll offered_ideal_fee = 0;
    ll offered_cell_time = 0;
    ll accepted_ideal_fee = 0;
    ll accepted_initial_fee = 0;
    ll accepted_initial_shape_loss = 0;
    ll accepted_relocation_fee_loss = 0;
    ll accepted_final_fee = 0;
    ll accepted_cell_time = 0;
    ll movement_cost_paid = 0;

    ll rejected_ideal_fee = 0;
    ll rejected_cell_time = 0;
    ll upper_rejected_ideal_fee = 0;
    ll upper_rejected_cell_time = 0;
    ll actual_rejected_ideal_fee = 0;
    ll actual_rejected_cell_time = 0;
    ll actual_rejected_candidate_fee = 0;
    ll actual_rejected_geometry_loss = 0;
    ll no_region_ideal_fee = 0;
    ll no_region_cell_time = 0;
    ll rejected_status_mismatch_ideal_fee = 0;
    ll rejected_status_mismatch_cell_time = 0;

    ll rejected_feasible_ideal_fee = 0;
    ll rejected_feasible_cell_time = 0;
    ll rejected_unplaceable_ideal_fee = 0;
    ll rejected_unplaceable_cell_time = 0;
    ll unplaceable_static_ideal_fee = 0;
    ll unplaceable_static_cell_time = 0;
    ll unplaceable_capacity_ideal_fee = 0;
    ll unplaceable_capacity_cell_time = 0;
    ll unplaceable_fragmentation_ideal_fee = 0;
    ll unplaceable_fragmentation_cell_time = 0;

    ll accepted_perimeter_excess = 0;
    ll accepted_decision_fee_error = 0;
    ll accepted_decision_perimeter_error = 0;
    array<ll, 4> accepted_source_ideal_fee{};
    array<ll, 4> accepted_source_initial_fee{};
    array<ll, 4> accepted_source_perimeter_excess{};
    ll accepted_grow_and_trim_ideal_fee = 0;
    ll accepted_grow_and_trim_initial_fee = 0;
    ll accepted_grow_and_trim_perimeter_excess = 0;
    ll accepted_free_cells_sum = 0;
    ll rejected_feasible_free_cells_sum = 0;
    ll rejected_unplaceable_free_cells_sum = 0;

    long double accepted_opportunity_cost = 0.0L;
    long double upper_rejected_opportunity_cost = 0.0L;
    long double actual_rejected_opportunity_cost = 0.0L;
    long double no_region_opportunity_cost = 0.0L;
};

__attribute__((noinline)) void observe_loss(
    LossDiagnostics &diagnostics, const ArrivalDecision &decision, const TurnPlan &plan,
    const GroupState &arrival, int minimum_perimeter, int free_cells_before,
    int static_largest_component, int reject_largest_component,
    ll turn_movement_cost, ll turn_relocation_fee_loss, long double opportunity_cost) {
    ll ideal_fee = round_payment(arrival.v, arrival.p, minimum_perimeter);
    ll cell_time = (ll)arrival.p * (arrival.t - arrival.s);
    diagnostics.observed++;
    diagnostics.offered_ideal_fee += ideal_fee;
    diagnostics.offered_cell_time += cell_time;
    diagnostics.movement_cost_paid += turn_movement_cost;
    diagnostics.accepted_relocation_fee_loss += turn_relocation_fee_loss;

    if (plan.arrival) {
        ll initial_fee = round_payment(arrival.v, arrival.p, plan.arrival_perimeter);
        diagnostics.accepted++;
        diagnostics.accepted_ideal_fee += ideal_fee;
        diagnostics.accepted_initial_fee += initial_fee;
        diagnostics.accepted_initial_shape_loss += ideal_fee - initial_fee;
        diagnostics.accepted_cell_time += cell_time;
        diagnostics.accepted_perimeter_excess += plan.arrival_perimeter - minimum_perimeter;
        diagnostics.accepted_free_cells_sum += free_cells_before;
        diagnostics.accepted_decision_fee_error += decision.fee - initial_fee;
        diagnostics.accepted_decision_perimeter_error +=
            decision.perimeter - plan.arrival_perimeter;
        if (decision.status != ArrivalStatus::Accepted) diagnostics.accepted_status_mismatches++;
        if (decision.fee != initial_fee || decision.perimeter != plan.arrival_perimeter) {
            diagnostics.accepted_plan_mismatches++;
        }

        int source = 3;
        if (decision.diagnostics.compact_successes == 1 &&
            decision.diagnostics.extended_template_successes == 0 &&
            decision.diagnostics.fallback_successes == 0) {
            source = 0;
        } else if (decision.diagnostics.compact_successes == 1 &&
                   decision.diagnostics.extended_template_successes == 1 &&
                   decision.diagnostics.fallback_successes == 0) {
            source = 1;
        } else if (decision.diagnostics.compact_successes == 0 &&
                   decision.diagnostics.extended_template_successes == 0 &&
                   decision.diagnostics.fallback_successes == 1 &&
                   (decision.diagnostics.grow_and_trim_successes == 0 ||
                    decision.diagnostics.grow_and_trim_successes == 1)) {
            source = 2;
        } else {
            diagnostics.accepted_source_mismatches++;
        }
        diagnostics.accepted_by_source[source]++;
        diagnostics.accepted_source_ideal_fee[source] += ideal_fee;
        diagnostics.accepted_source_initial_fee[source] += initial_fee;
        diagnostics.accepted_source_perimeter_excess[source] +=
            plan.arrival_perimeter - minimum_perimeter;
        if (decision.diagnostics.grow_and_trim_successes == 1) {
            diagnostics.accepted_grow_and_trim++;
            diagnostics.accepted_grow_and_trim_ideal_fee += ideal_fee;
            diagnostics.accepted_grow_and_trim_initial_fee += initial_fee;
            diagnostics.accepted_grow_and_trim_perimeter_excess +=
                plan.arrival_perimeter - minimum_perimeter;
        }
        diagnostics.accepted_opportunity_cost += opportunity_cost;
        return;
    }

    diagnostics.rejected_ideal_fee += ideal_fee;
    diagnostics.rejected_cell_time += cell_time;
    if (!plan.moves.empty()) diagnostics.rejected_move_plans++;
    bool feasible = reject_largest_component >= arrival.p;
    if (reject_largest_component < 0) diagnostics.feasibility_mismatches++;
    switch (decision.status) {
        case ArrivalStatus::UpperBoundRejected:
            diagnostics.upper_rejected++;
            diagnostics.upper_rejected_ideal_fee += ideal_fee;
            diagnostics.upper_rejected_cell_time += cell_time;
            diagnostics.upper_rejected_opportunity_cost += opportunity_cost;
            if (feasible) {
                diagnostics.upper_rejected_feasible++;
            } else {
                diagnostics.upper_rejected_unplaceable++;
            }
            break;
        case ArrivalStatus::ActualFeeRejected:
            diagnostics.actual_rejected++;
            diagnostics.actual_rejected_ideal_fee += ideal_fee;
            diagnostics.actual_rejected_cell_time += cell_time;
            diagnostics.actual_rejected_candidate_fee +=
                decision.diagnostics.actual_rejected_candidate_fee;
            diagnostics.actual_rejected_geometry_loss +=
                ideal_fee - decision.diagnostics.actual_rejected_candidate_fee;
            diagnostics.actual_rejected_opportunity_cost += opportunity_cost;
            if (!feasible) diagnostics.feasibility_mismatches++;
            break;
        case ArrivalStatus::NoRegion:
            diagnostics.no_region_rejected++;
            diagnostics.no_region_ideal_fee += ideal_fee;
            diagnostics.no_region_cell_time += cell_time;
            diagnostics.no_region_opportunity_cost += opportunity_cost;
            if (feasible) diagnostics.feasibility_mismatches++;
            break;
        case ArrivalStatus::Accepted:
            diagnostics.rejected_status_mismatch++;
            diagnostics.rejected_status_mismatch_ideal_fee += ideal_fee;
            diagnostics.rejected_status_mismatch_cell_time += cell_time;
            break;
    }

    if (feasible) {
        diagnostics.rejected_feasible++;
        diagnostics.rejected_feasible_ideal_fee += ideal_fee;
        diagnostics.rejected_feasible_cell_time += cell_time;
        diagnostics.rejected_feasible_free_cells_sum += free_cells_before;
        return;
    }

    diagnostics.rejected_unplaceable++;
    diagnostics.rejected_unplaceable_ideal_fee += ideal_fee;
    diagnostics.rejected_unplaceable_cell_time += cell_time;
    diagnostics.rejected_unplaceable_free_cells_sum += free_cells_before;

    if (static_largest_component < arrival.p) {
        diagnostics.unplaceable_static++;
        diagnostics.unplaceable_static_ideal_fee += ideal_fee;
        diagnostics.unplaceable_static_cell_time += cell_time;
    } else if (free_cells_before < arrival.p) {
        diagnostics.unplaceable_capacity++;
        diagnostics.unplaceable_capacity_ideal_fee += ideal_fee;
        diagnostics.unplaceable_capacity_cell_time += cell_time;
    } else {
        diagnostics.unplaceable_fragmentation++;
        diagnostics.unplaceable_fragmentation_ideal_fee += ideal_fee;
        diagnostics.unplaceable_fragmentation_cell_time += cell_time;
    }
}

__attribute__((noinline)) void finalize_loss_diagnostics(
    LossDiagnostics &diagnostics, const vector<GroupState> &groups) {
    for (const GroupState &group : groups) {
        if (group.max_perimeter <= 0) continue;
        diagnostics.finalized_accepted++;
        diagnostics.accepted_final_fee +=
            round_payment(group.v, group.p, group.max_perimeter);
    }
}

using BoardMask = array<uint64_t, BOARD_MASK_WORDS>;

BoardMask make_board_mask(const vector<Cell> &cells, int n) {
    BoardMask mask{};
    for (auto [x, y] : cells) {
        int index = x * n + y;
        mask[index >> 6] |= 1ULL << (index & 63);
    }
    return mask;
}

BoardMask make_occupied_mask(const vvi &owner) {
    int n = owner.size();
    BoardMask mask{};
    for (int x = 0; x < n; x++) {
        for (int y = 0; y < n; y++) {
            if (owner[x][y] == -1) continue;
            int index = x * n + y;
            mask[index >> 6] |= 1ULL << (index & 63);
        }
    }
    return mask;
}

bool masks_overlap(const BoardMask &lhs, const BoardMask &rhs) {
    for (int word = 0; word < BOARD_MASK_WORDS; word++) {
        if (lhs[word] & rhs[word]) return true;
    }
    return false;
}

void merge_mask(BoardMask &destination, const BoardMask &source) {
    for (int word = 0; word < BOARD_MASK_WORDS; word++) destination[word] |= source[word];
}

vector<vi> make_flag_prefix(const vector<char> &flag, int n) {
    vector<vi> prefix(n + 1, vi(n + 1));
    for (int x = 0; x < n; x++) {
        for (int y = 0; y < n; y++) {
            prefix[x + 1][y + 1] =
                flag[x * n + y] + prefix[x][y + 1] + prefix[x + 1][y] - prefix[x][y];
        }
    }
    return prefix;
}

struct RescueDiagnostics {
    int eligible_fallbacks = 0;
    int feasible_turns = 0;
    int feasible_plans = 0;
    int successes = 0;
    int rollout_turns = 0;
    int rollout_generation_failures = 0;
    int rollout_adopted = 0;
    int rollout_rescue_not_selected = 0;
    int rollout_scenario_disagreements = 0;
    int rollout_skipped_no_future = 0;
    int rollout_one_candidate_turns = 0;
    int rollout_two_candidate_turns = 0;
    int rollout_selected_candidate_0 = 0;
    int rollout_selected_candidate_1 = 0;
    int rollout_candidate_0_disagreements = 0;
    int rollout_candidate_1_disagreements = 0;
    int rollout_same_blocker_sets = 0;
    int root_alternative_available_turns = 0;
    int root_selected_primary = 0;
    int root_selected_alternative = 0;
    int root_alternative_disagreements = 0;
    int root_screen_v3_overrides = 0;
    int root_screen_selected_alternative = 0;
    int root_v3_winner_overridden = 0;
    array<int, ROOT_ROLLOUT_NORMAL_ALTERNATIVE_LIMIT> root_selected_alternative_rank{};
    array<int, ROOT_ROLLOUT_MAX_ACTIONS + 1> root_turns_by_action_count{};
    int normal_root_gate_turns = 0;
    int normal_root_rollout_turns = 0;
    int normal_root_generation_failures = 0;
    int normal_root_screen_overrides = 0;
    int normal_root_screen_selected_alternative = 0;
    int normal_root_selected_primary = 0;
    int normal_root_selected_alternative = 0;
    array<int, ROOT_ROLLOUT_NORMAL_ALTERNATIVE_LIMIT> normal_root_selected_alternative_rank{};
    array<int, 1 + ROOT_ROLLOUT_NORMAL_ALTERNATIVE_LIMIT + 1> normal_root_turns_by_action_count{};
    int root_confirmation_attempts = 0;
    int root_confirmation_approved = 0;
    int root_confirmation_rejected = 0;
    int root_confirmation_generation_failures = 0;
    int root_confirmation_budget_skips = 0;
    int root_confirmation_full_horizon = 0;
    int root_confirmation_short_horizon = 0;
    int root_confirmation_pair_disagreements = 0;
    int no_economic_target = 0;
    int no_repair = 0;
    int target_limit_exhausted = 0;
    int destination_limit_exhausted = 0;
    int node_limit_exhausted = 0;
    int validation_failures = 0;
    int maximum_blockers = 0;
    array<int, 4> feasible_by_blocker_count{};
    array<int, 4> successes_by_blocker_count{};
    long long target_anchors = 0;
    long long target_shortlisted = 0;
    long long exact_targets = 0;
    long long economic_targets = 0;
    long long repair_attempts = 0;
    long long destination_anchors = 0;
    long long destination_candidates = 0;
    long long beam_nodes = 0;
    long long rollout_policy_steps = 0;
    long long rollout_candidates_compared = 0;
    long long rollout_positive_candidates = 0;
    long long rollout_nonpositive_candidates = 0;
    long long rollout_unselected_positive_candidates = 0;
    long long rollout_candidate_overlap_cells = 0;
    long long rollout_baseline_acceptances = 0;
    long long rollout_rescue_acceptances = 0;
    long long root_actions_compared = 0;
    long long root_alternatives_compared = 0;
    long long root_alternative_acceptances = 0;
    long long normal_root_actions_compared = 0;
    long long normal_root_alternatives_compared = 0;
    long long normal_root_policy_steps = 0;
    long long root_confirmation_policy_steps = 0;
    long long root_confirmation_scenarios = 0;
    long long root_confirmation_positive_scenarios = 0;
    long long moved_groups = 0;
    ll feasible_direct_gain = 0;
    ll arrival_fee_gain = 0;
    ll movement_cost = 0;
    ll immediate_gain = 0;
    ll rollout_scenario_0_future_delta = 0;
    ll rollout_scenario_1_future_delta = 0;
    ll root_alternative_scenario_0_future_delta = 0;
    ll root_alternative_scenario_1_future_delta = 0;
    array<ll, RESCUE_ROLLOUT_CANDIDATE_LIMIT> rollout_slot_scenario_0_future_delta{};
    array<ll, RESCUE_ROLLOUT_CANDIDATE_LIMIT> rollout_slot_scenario_1_future_delta{};
    long double rollout_adopted_direct_gain = 0.0L;
    long double rollout_adopted_future_mean = 0.0L;
    long double rollout_adopted_margin = 0.0L;
    long double rollout_not_selected_direct_gain = 0.0L;
    long double rollout_not_selected_future_mean = 0.0L;
    long double rollout_not_selected_margin = 0.0L;
    array<long double, RESCUE_ROLLOUT_CANDIDATE_LIMIT> rollout_slot_margin{};
    long double rollout_width_predicted_gain = 0.0L;
    long double root_alternative_direct_gain = 0.0L;
    long double root_alternative_future_mean = 0.0L;
    long double root_alternative_margin = 0.0L;
    long double root_expanded_predicted_gain = 0.0L;
    long double root_confirmation_screen_gain = 0.0L;
    long double root_confirmation_holdout_margin = 0.0L;
    long double root_confirmation_approved_margin = 0.0L;
    long double root_confirmation_rejected_margin = 0.0L;

    // NoRegion Push-out is a Reject-vs-relocation decision, unlike the
    // Accepted-vs-compact rescue above.  Keep its funnel separate so the two
    // mechanisms can be judged independently even though they share search.
    int pushout_eligible = 0;
    int pushout_area_insufficient = 0;
    int pushout_no_economic_target = 0;
    int pushout_feasible_turns = 0;
    int pushout_feasible_plans = 0;
    int pushout_no_repair = 0;
    int pushout_rollout_generation_failures = 0;
    int pushout_rollout_turns = 0;
    int pushout_screen_rejected = 0;
    int pushout_adopted = 0;
    int pushout_target_limit_exhausted = 0;
    int pushout_destination_limit_exhausted = 0;
    int pushout_node_limit_exhausted = 0;
    int pushout_maximum_blockers = 0;
    array<int, 4> pushout_feasible_by_blocker_count{};
    array<int, 4> pushout_adopted_by_blocker_count{};
    long long pushout_shadow_filtered_targets = 0;
    long long pushout_target_anchors = 0;
    long long pushout_target_shortlisted = 0;
    long long pushout_exact_targets = 0;
    long long pushout_economic_targets = 0;
    long long pushout_repair_attempts = 0;
    long long pushout_destination_anchors = 0;
    long long pushout_destination_candidates = 0;
    long long pushout_beam_nodes = 0;
    long long pushout_rollout_policy_steps = 0;
    long long pushout_moved_groups = 0;
    long long pushout_moved_cells = 0;
    ll pushout_arrival_fee = 0;
    ll pushout_movement_cost = 0;
    ll pushout_relocation_fee_loss = 0;
    ll pushout_direct_gain = 0;
    ll pushout_scenario_0_future_delta = 0;
    ll pushout_scenario_1_future_delta = 0;
    long double pushout_screen_margin = 0.0L;
    double pushout_cpu_seconds = 0.0;
    double pushout_maximum_turn_cpu_seconds = 0.0;

    // One-helper Push-out is an additive second search phase.  The legacy
    // blockers-only candidates remain protected and these counters expose the
    // added funnel and work independently.
    int pushout_helper_considered_turns = 0;
    int pushout_helper_no_eligible_target_turns = 0;
    int pushout_helper_no_evidence_turns = 0;
    int pushout_helper_economic_rejected_turns = 0;
    int pushout_helper_seeded_turns = 0;
    int pushout_helper_attempts = 0;
    int pushout_helper_missing_destination = 0;
    int pushout_helper_missing_blocker_destination = 0;
    int pushout_helper_missing_helper_destination = 0;
    int pushout_helper_repair_failures = 0;
    int pushout_helper_validation_failures = 0;
    int pushout_helper_duplicate_plans = 0;
    int pushout_helper_feasible_plans = 0;
    int pushout_helper_screen_rejected = 0;
    int pushout_helper_adopted = 0;
    int pushout_helper_surveyed_turns = 0;
    int pushout_helper_surveyed_targets = 0;
    int pushout_helper_large_blocker_targets = 0;
    int pushout_helper_probe_limit_exhausted = 0;
    int pushout_helper_feasible_limit_exhausted = 0;
    int pushout_helper_two_feasible_turns = 0;
    int pushout_helper_maximum_movers = 0;
    array<int, PUSHOUT_HELPER_MAX_BLOCKERS> pushout_helper_feasible_by_blocker_count{};
    array<int, PUSHOUT_HELPER_MAX_BLOCKERS> pushout_helper_adopted_by_blocker_count{};
    long long pushout_helper_obstruction_probes = 0;
    long long pushout_helper_single_owner_regions = 0;
    long long pushout_helper_overlap_cells = 0;
    long long pushout_helper_evidenced_groups = 0;
    long long pushout_helper_recorded_witnesses = 0;
    long long pushout_helper_shortlisted_choices = 0;
    long long pushout_helper_destination_anchors = 0;
    long long pushout_helper_destination_candidates = 0;
    long long pushout_helper_foreign_destination_candidates = 0;
    long long pushout_helper_retained_foreign_destinations = 0;
    long long pushout_helper_forced_witness_destinations = 0;
    long long pushout_helper_beam_nodes = 0;
    long long pushout_helper_selected_covered_blockers = 0;
    long long pushout_helper_selected_unlocked_regions = 0;
    long long pushout_helper_selected_overlap_cells = 0;
    ll pushout_helper_selected_movement_cost = 0;
    ll pushout_helper_selected_departure_distance = 0;
    ll pushout_helper_selected_adjusted_gain = 0;
    int pushout_helper_feasible_blocker_uses_helper = 0;
    int pushout_helper_feasible_helper_uses_blocker = 0;
    int pushout_helper_feasible_bidirectional_cross_use = 0;
    int pushout_helper_adopted_blocker_uses_helper = 0;
    int pushout_helper_adopted_helper_uses_blocker = 0;
    int pushout_helper_adopted_bidirectional_cross_use = 0;
    long long pushout_helper_adopted_moved_groups = 0;
    long long pushout_helper_adopted_moved_cells = 0;
    ll pushout_helper_adopted_arrival_fee = 0;
    ll pushout_helper_adopted_movement_cost = 0;
    ll pushout_helper_adopted_direct_gain = 0;
    ll pushout_helper_scenario_0_future_delta = 0;
    ll pushout_helper_scenario_1_future_delta = 0;
    long double pushout_helper_screen_margin = 0.0L;
    double pushout_helper_phase_cpu_seconds = 0.0;
};

struct DeadlineLayerDiagnostics {
    int eligible = 0;
    int no_region_eligible = 0;
    int noncompact_eligible = 0;
    int area_insufficient = 0;
    int economic_upper_bound_rejected = 0;
    int case_budget_skips = 0;
    int window_budget_skips = 0;
    int attempts = 0;
    array<int, 4> attempts_by_window{};
    array<int, 4> no_region_attempts_by_window{};
    array<int, 4> noncompact_attempts_by_window{};
    int graph_builds = 0;
    int graph_failures = 0;
    int closure_failures = 0;
    int workspaces_searched = 0;
    int layout_failures = 0;
    int validation_failures = 0;
    int feasible_turns = 0;
    int feasible_plans = 0;
    int complete_plan_attempts = 0;
    int zero_move_candidates_filtered = 0;
    int direct_gate_rejected = 0;
    int rollout_generation_failures = 0;
    int rollout_turns = 0;
    int screen_rejected = 0;
    int confirmation_rejected = 0;
    int confirmation_attempts = 0;
    int adopted = 0;
    int adopted_with_move = 0;
    int adopted_from_no_region = 0;
    int closure_limit_exhausted = 0;
    int layout_limit_exhausted = 0;
    int template_limit_exhausted = 0;
    int growth_limit_exhausted = 0;
    int connectivity_limit_exhausted = 0;
    int complete_plan_limit_exhausted = 0;
    int partition_errors = 0;
    int prefix_connectivity_errors = 0;
    int direct_identity_errors = 0;
    long long closure_expansions = 0;
    long long closure_states = 0;
    long long completed_closures = 0;
    long long global_closures = 0;
    long long layout_nodes = 0;
    long long region_candidates = 0;
    long long template_probes = 0;
    long long growth_steps = 0;
    long long connectivity_visits = 0;
    long long connectivity_calls = 0;
    long long rollout_policy_steps = 0;
    long long moved_groups = 0;
    long long moved_cells = 0;
    ll arrival_fee = 0;
    ll movement_cost = 0;
    ll relocation_fee_loss = 0;
    ll direct_gain = 0;
    ll scenario_0_future_delta = 0;
    ll scenario_1_future_delta = 0;
    long double screen_margin = 0.0L;
    double cpu_seconds = 0.0;
    double maximum_turn_cpu_seconds = 0.0;
};

struct DeadlineLayerCpuScope {
    DeadlineLayerDiagnostics &diagnostics;
    clock_t begin;

    explicit DeadlineLayerCpuScope(DeadlineLayerDiagnostics &diagnostics)
        : diagnostics(diagnostics), begin(clock()) {}

    ~DeadlineLayerCpuScope() {
        clock_t end = clock();
        if (begin == (clock_t)-1 || end == (clock_t)-1) return;
        double seconds = (double)(end - begin) / CLOCKS_PER_SEC;
        diagnostics.cpu_seconds += seconds;
        chmax(diagnostics.maximum_turn_cpu_seconds, seconds);
    }
};

struct PushOutDiagnosticScope {
    RescueDiagnostics &diagnostics;
    bool active;
    clock_t cpu_begin;
    long long target_anchors;
    long long target_shortlisted;
    long long exact_targets;
    long long economic_targets;
    long long repair_attempts;
    long long destination_anchors;
    long long destination_candidates;
    long long beam_nodes;
    long long rollout_policy_steps;
    int feasible_plans;
    int rollout_turns;
    int target_limit_exhausted;
    int destination_limit_exhausted;
    int node_limit_exhausted;

    PushOutDiagnosticScope(RescueDiagnostics &diagnostics, bool active)
        : diagnostics(diagnostics),
          active(active),
          cpu_begin(active ? clock() : (clock_t)-1),
          target_anchors(diagnostics.target_anchors),
          target_shortlisted(diagnostics.target_shortlisted),
          exact_targets(diagnostics.exact_targets),
          economic_targets(diagnostics.economic_targets),
          repair_attempts(diagnostics.repair_attempts),
          destination_anchors(diagnostics.destination_anchors),
          destination_candidates(diagnostics.destination_candidates),
          beam_nodes(diagnostics.beam_nodes),
          rollout_policy_steps(diagnostics.rollout_policy_steps),
          feasible_plans(diagnostics.feasible_plans),
          rollout_turns(diagnostics.rollout_turns),
          target_limit_exhausted(diagnostics.target_limit_exhausted),
          destination_limit_exhausted(diagnostics.destination_limit_exhausted),
          node_limit_exhausted(diagnostics.node_limit_exhausted) {}

    ~PushOutDiagnosticScope() {
        if (!active) return;
        diagnostics.pushout_target_anchors += diagnostics.target_anchors - target_anchors;
        diagnostics.pushout_target_shortlisted += diagnostics.target_shortlisted - target_shortlisted;
        diagnostics.pushout_exact_targets += diagnostics.exact_targets - exact_targets;
        diagnostics.pushout_economic_targets += diagnostics.economic_targets - economic_targets;
        diagnostics.pushout_repair_attempts += diagnostics.repair_attempts - repair_attempts;
        diagnostics.pushout_destination_anchors += diagnostics.destination_anchors - destination_anchors;
        diagnostics.pushout_destination_candidates +=
            diagnostics.destination_candidates - destination_candidates;
        diagnostics.pushout_beam_nodes += diagnostics.beam_nodes - beam_nodes;
        diagnostics.pushout_rollout_policy_steps +=
            diagnostics.rollout_policy_steps - rollout_policy_steps;
        diagnostics.pushout_feasible_plans += diagnostics.feasible_plans - feasible_plans;
        diagnostics.pushout_rollout_turns += diagnostics.rollout_turns - rollout_turns;
        diagnostics.pushout_target_limit_exhausted +=
            diagnostics.target_limit_exhausted - target_limit_exhausted;
        diagnostics.pushout_destination_limit_exhausted +=
            diagnostics.destination_limit_exhausted - destination_limit_exhausted;
        diagnostics.pushout_node_limit_exhausted +=
            diagnostics.node_limit_exhausted - node_limit_exhausted;
        clock_t cpu_end = clock();
        if (cpu_begin != (clock_t)-1 && cpu_end != (clock_t)-1) {
            double seconds = (double)(cpu_end - cpu_begin) / CLOCKS_PER_SEC;
            diagnostics.pushout_cpu_seconds += seconds;
            chmax(diagnostics.pushout_maximum_turn_cpu_seconds, seconds);
        }
    }
};

struct RescueTargetSeed {
    int shape_index = -1;
    int base_x = 0;
    int base_y = 0;
    int occupied_cells = 0;
    long double fractional_move_cost = 0.0L;
    long long order = 0;
};

struct RescueTarget {
    vector<Cell> cells;
    uint64_t region_hash = 0;
    vector<int> blockers;
    int blocker_cells = 0;
    int perimeter = 0;
    ll movement_cost = 0;
    ll immediate_improvement = 0;
    long long order = 0;
};

vector<RescueTarget> make_rescue_targets(const vs &park, const vvi &owner,
                                         const vector<GroupState> &groups, int arrival_id, int r_milli,
                                         ll baseline_score, long double direct_gain_threshold,
                                         bool no_region_pushout, int shortlist_per_metric,
                                         const vector<vector<Shape>> &compact_shapes,
                                         RescueDiagnostics &diagnostics) {
    int n = park.size();
    const GroupState &arrival = groups[arrival_id];
    const vector<Shape> &shapes = compact_shapes[arrival.p];
    int minimum_perimeter = shapes.front().perimeter;
    ll compact_fee = round_payment(arrival.v, arrival.p, minimum_perimeter);

    vector<char> pond(n * n, false);
    vector<vi> occupied_prefix(n + 1, vi(n + 1));
    vector<vector<long double>> fractional_prefix(n + 1, vector<long double>(n + 1));
    for (int x = 0; x < n; x++) {
        for (int y = 0; y < n; y++) {
            pond[x * n + y] = park[x][y] == '#';
            int id = owner[x][y];
            long double fractional =
                id == -1 ? 0.0L : (long double)move_cost(groups[id].v, r_milli) / groups[id].p;
            occupied_prefix[x + 1][y + 1] =
                (id != -1) + occupied_prefix[x][y + 1] + occupied_prefix[x + 1][y] - occupied_prefix[x][y];
            fractional_prefix[x + 1][y + 1] =
                fractional + fractional_prefix[x][y + 1] + fractional_prefix[x + 1][y] -
                fractional_prefix[x][y];
        }
    }
    vector<vi> pond_prefix = make_flag_prefix(pond, n);

    auto occupied_better = [](const RescueTargetSeed &lhs, const RescueTargetSeed &rhs) {
        return tie(lhs.occupied_cells, lhs.fractional_move_cost, lhs.order) <
               tie(rhs.occupied_cells, rhs.fractional_move_cost, rhs.order);
    };
    auto fractional_better = [](const RescueTargetSeed &lhs, const RescueTargetSeed &rhs) {
        return tie(lhs.fractional_move_cost, lhs.occupied_cells, lhs.order) <
               tie(rhs.fractional_move_cost, rhs.occupied_cells, rhs.order);
    };
    auto retain_shortlist = [&](vector<RescueTargetSeed> &heap, const RescueTargetSeed &seed, auto better) {
        if ((int)heap.size() < shortlist_per_metric) {
            heap.push_back(seed);
            push_heap(heap.begin(), heap.end(), better);
        } else if (better(seed, heap.front())) {
            pop_heap(heap.begin(), heap.end(), better);
            heap.back() = seed;
            push_heap(heap.begin(), heap.end(), better);
        }
    };
    vector<RescueTargetSeed> occupied_shortlist;
    vector<RescueTargetSeed> fractional_shortlist;
    occupied_shortlist.reserve(shortlist_per_metric);
    fractional_shortlist.reserve(shortlist_per_metric);
    long long order = 0;
    for (int shape_index = 0; shape_index < (int)shapes.size(); shape_index++) {
        const Shape &shape = shapes[shape_index];
        if (shape.perimeter != minimum_perimeter) break;
        for (int base_x = 0; base_x + shape.h <= n; base_x++) {
            for (int base_y = 0; base_y + shape.w <= n; base_y++) {
                diagnostics.target_anchors++;
                const Rect &a = shape.main_rect;
                const Rect &b = shape.extra_rect;
                if (rectangle_sum(pond_prefix, base_x + a.x, base_y + a.y, a.h, a.w) != 0 ||
                    rectangle_sum(pond_prefix, base_x + b.x, base_y + b.y, b.h, b.w) != 0) {
                    continue;
                }
                int occupied = rectangle_sum(occupied_prefix, base_x + a.x, base_y + a.y, a.h, a.w) +
                               rectangle_sum(occupied_prefix, base_x + b.x, base_y + b.y, b.h, b.w);
                long double fractional =
                    rectangle_sum(fractional_prefix, base_x + a.x, base_y + a.y, a.h, a.w) +
                    rectangle_sum(fractional_prefix, base_x + b.x, base_y + b.y, b.h, b.w);
                RescueTargetSeed seed{shape_index, base_x, base_y, occupied, fractional, order++};
                retain_shortlist(occupied_shortlist, seed, occupied_better);
                retain_shortlist(fractional_shortlist, seed, fractional_better);
            }
        }
    }

    // Keep only the two bounded top-k heaps instead of materializing every
    // pond-legal anchor and two full index arrays.  The unique enumeration
    // order is the final key, so this retains exactly the old partial_sort set.
    vector<RescueTargetSeed> shortlisted;
    shortlisted.reserve(occupied_shortlist.size() + fractional_shortlist.size());
    shortlisted.insert(shortlisted.end(), occupied_shortlist.begin(), occupied_shortlist.end());
    shortlisted.insert(shortlisted.end(), fractional_shortlist.begin(), fractional_shortlist.end());
    sort(shortlisted.begin(), shortlisted.end(), [](const RescueTargetSeed &lhs, const RescueTargetSeed &rhs) {
        return lhs.order < rhs.order;
    });
    shortlisted.erase(
        unique(shortlisted.begin(), shortlisted.end(), [](const RescueTargetSeed &lhs, const RescueTargetSeed &rhs) {
            return lhs.order == rhs.order;
        }),
        shortlisted.end());
    diagnostics.target_shortlisted += shortlisted.size();

    vector<RescueTarget> result;
    vector<int> seen(groups.size(), -1);
    int stamp = 0;
    for (const RescueTargetSeed &seed : shortlisted) {
        vector<Cell> cells =
            materialize_shape(shapes[seed.shape_index], seed.base_x, seed.base_y, arrival.p);
        vector<int> blockers;
        stamp++;
        for (auto [x, y] : cells) {
            int id = owner[x][y];
            if (id != -1 && seen[id] != stamp) {
                seen[id] = stamp;
                blockers.push_back(id);
            }
        }
        // A no-blocker minimum template would have been found by the normal
        // compact enumeration, so it is not a relocation action.
        if (blockers.empty()) continue;
        sort(blockers.begin(), blockers.end());
        ll movement_cost_sum = 0;
        int blocker_cells = 0;
        for (int id : blockers) {
            movement_cost_sum += move_cost(groups[id].v, r_milli);
            blocker_cells += groups[id].p;
        }
        diagnostics.exact_targets++;
        ll improvement = compact_fee - baseline_score - movement_cost_sum;
        if (improvement <= 0) continue;
        if ((long double)improvement <= direct_gain_threshold) {
            if (no_region_pushout) diagnostics.pushout_shadow_filtered_targets++;
            continue;
        }
        diagnostics.economic_targets++;

        uint64_t region_hash = placement_region_hash(cells);
        bool duplicate = false;
        for (const RescueTarget &existing : result) {
            if (existing.region_hash == region_hash && same_region(existing.cells, cells)) {
                duplicate = true;
                break;
            }
        }
        if (duplicate) continue;
        result.push_back({std::move(cells), region_hash, std::move(blockers), blocker_cells,
                          minimum_perimeter, movement_cost_sum, improvement, seed.order});
    }

    sort(result.begin(), result.end(), [&](const RescueTarget &lhs, const RescueTarget &rhs) {
        if (lhs.immediate_improvement != rhs.immediate_improvement) {
            return lhs.immediate_improvement > rhs.immediate_improvement;
        }
        if (lhs.blockers.size() != rhs.blockers.size()) return lhs.blockers.size() < rhs.blockers.size();
        if (no_region_pushout && lhs.blocker_cells != rhs.blocker_cells) {
            return lhs.blocker_cells < rhs.blocker_cells;
        }
        return lhs.order < rhs.order;
    });
    return result;
}

struct RescueDestination {
    vector<Cell> cells;
    BoardMask mask{};
    int perimeter = 0;
    int fallback_overlap = 0;
    int cleared_overlap = 0;
    int foreign_cleared_overlap = 0;
    int quadrant = 0;
    int sector = 0;
    long double temporal_cost = 0.0L;
    long long order = 0;
};

struct PushOutHelperWitness {
    int blocker_id = -1;
    vector<Cell> cells;
    int perimeter = 0;
};

struct PushOutHelperSurvey {
    int remaining_probes = PUSHOUT_HELPER_OBSTRUCTION_PROBE_LIMIT;
    bool probe_limit_reported = false;
    vector<vi> owner_count_prefix;
    vector<vector<ll>> owner_sum_prefix;
    vector<vector<ll>> owner_square_sum_prefix;
    vector<int> unlocked_regions;
    vector<long long> overlap_cells;
    vector<int> covered_blockers;
    vector<int> last_blocker;
    vector<vector<PushOutHelperWitness>> witnesses;

    PushOutHelperSurvey(int group_count, const vvi &base_owner)
        : owner_count_prefix(base_owner.size() + 1,
                             vi(base_owner.size() + 1)),
          owner_sum_prefix(base_owner.size() + 1,
                           vector<ll>(base_owner.size() + 1)),
          owner_square_sum_prefix(base_owner.size() + 1,
                                  vector<ll>(base_owner.size() + 1)),
          unlocked_regions(group_count),
          overlap_cells(group_count),
          covered_blockers(group_count),
          last_blocker(group_count, -1),
          witnesses(group_count) {
        int n = base_owner.size();
        for (int x = 0; x < n; x++) {
            for (int y = 0; y < n; y++) {
                ll owner_value = base_owner[x][y] == -1 ? 0 : base_owner[x][y] + 1;
                owner_count_prefix[x + 1][y + 1] =
                    (owner_value != 0) + owner_count_prefix[x][y + 1] +
                    owner_count_prefix[x + 1][y] - owner_count_prefix[x][y];
                owner_sum_prefix[x + 1][y + 1] =
                    owner_value + owner_sum_prefix[x][y + 1] +
                    owner_sum_prefix[x + 1][y] - owner_sum_prefix[x][y];
                owner_square_sum_prefix[x + 1][y + 1] =
                    owner_value * owner_value +
                    owner_square_sum_prefix[x][y + 1] +
                    owner_square_sum_prefix[x + 1][y] -
                    owner_square_sum_prefix[x][y];
            }
        }
    }
};

// Record an almost-legal blocker destination only when clearing exactly one
// additional active owner would make every cell available.  This is the
// causal shortlist for a helper; arbitrary active groups are never searched.
// Count/sum/square-sum prefixes prove in O(1) that every occupied cell has the
// same owner, avoiding allocation and O(P) materialization per blocked anchor.
void observe_pushout_helper_obstruction(PushOutHelperSurvey &survey,
                                        const Shape &shape, int base_x, int base_y,
                                        int blocked_cells,
                                        const vector<GroupState> &groups,
                                        int blocker_id, int arrival_id,
                                        RescueDiagnostics &diagnostics) {
    if (survey.remaining_probes == 0) {
        if (!survey.probe_limit_reported) {
            survey.probe_limit_reported = true;
            diagnostics.pushout_helper_probe_limit_exhausted++;
        }
        return;
    }
    survey.remaining_probes--;
    diagnostics.pushout_helper_obstruction_probes++;

    const Rect &a = shape.main_rect;
    const Rect &b = shape.extra_rect;
    auto region_sum = [&](const auto &prefix) {
        return rectangle_sum(prefix, base_x + a.x, base_y + a.y, a.h, a.w) +
               rectangle_sum(prefix, base_x + b.x, base_y + b.y, b.h, b.w);
    };
    int occupied_cells = region_sum(survey.owner_count_prefix);
    // blocked_cells also counts pond cells.  Equality proves the shape is
    // pond-free, so clearing its one owner is sufficient for legality.
    if (occupied_cells == 0 || occupied_cells != blocked_cells) return;
    ll owner_sum = region_sum(survey.owner_sum_prefix);
    ll owner_square_sum = region_sum(survey.owner_square_sum_prefix);
    if (owner_sum % occupied_cells != 0 ||
        owner_square_sum * occupied_cells != owner_sum * owner_sum) {
        return;
    }
    int helper_id = (int)(owner_sum / occupied_cells) - 1;
    if (helper_id < 0 || helper_id >= (int)groups.size() ||
        helper_id == arrival_id || !groups[helper_id].active) {
        return;
    }

    diagnostics.pushout_helper_single_owner_regions++;
    diagnostics.pushout_helper_overlap_cells += occupied_cells;
    survey.unlocked_regions[helper_id]++;
    survey.overlap_cells[helper_id] += occupied_cells;
    if (survey.last_blocker[helper_id] != blocker_id) {
        survey.last_blocker[helper_id] = blocker_id;
        survey.covered_blockers[helper_id]++;
    }
    if constexpr (ENABLE_WIDE_PUSHOUT_HELPER) {
        vector<PushOutHelperWitness> &helper_witnesses =
            survey.witnesses[helper_id];
        bool already_recorded = any_of(
            helper_witnesses.begin(), helper_witnesses.end(),
            [&](const PushOutHelperWitness &witness) {
                return witness.blocker_id == blocker_id;
            });
        if (!already_recorded) {
            helper_witnesses.push_back({
                blocker_id,
                materialize_shape(shape, base_x, base_y,
                                  groups[blocker_id].p),
                shape.perimeter,
            });
            diagnostics.pushout_helper_recorded_witnesses++;
        }
    }
}

struct PushOutHelperChoice {
    int id = -1;
    int covered_blockers = 0;
    int unlocked_regions = 0;
    long long overlap_cells = 0;
    ll movement_cost = 0;
    ll departure_distance = 0;
    vector<PushOutHelperWitness> witnesses;
};

vector<PushOutHelperChoice> choose_pushout_helpers(
    const PushOutHelperSurvey &survey, const RescueTarget &target,
    const vector<GroupState> &groups, int arrival_id, int r_milli,
    long double direct_gain_threshold, int choice_limit,
    int &evidenced_groups) {
    evidenced_groups = 0;
    vector<ll> blocker_departures;
    blocker_departures.reserve(target.blockers.size());
    for (int id : target.blockers) blocker_departures.push_back(groups[id].t);
    sort(blocker_departures.begin(), blocker_departures.end());
    ll median_departure = blocker_departures[(blocker_departures.size() - 1) / 2];

    vector<PushOutHelperChoice> choices;
    for (int id = 0; id < (int)groups.size(); id++) {
        if (survey.unlocked_regions[id] == 0) continue;
        evidenced_groups++;
        if (id == arrival_id || !groups[id].active ||
            binary_search(target.blockers.begin(), target.blockers.end(), id)) {
            continue;
        }
        ll added_movement_cost = move_cost(groups[id].v, r_milli);
        if ((long double)(target.immediate_improvement - added_movement_cost) <=
            direct_gain_threshold) {
            continue;
        }
        choices.push_back({
            id,
            survey.covered_blockers[id],
            survey.unlocked_regions[id],
            survey.overlap_cells[id],
            added_movement_cost,
            llabs(groups[id].t - median_departure),
            survey.witnesses[id],
        });
    }
    auto key = [](const PushOutHelperChoice &choice) {
        return tuple{-choice.covered_blockers, -choice.unlocked_regions,
                     choice.movement_cost, choice.departure_distance, choice.id};
    };
    sort(choices.begin(), choices.end(), [&](const PushOutHelperChoice &lhs,
                                             const PushOutHelperChoice &rhs) {
        return key(lhs) < key(rhs);
    });
    if ((int)choices.size() > choice_limit) choices.resize(choice_limit);
    return choices;
}

long double rescue_destination_temporal_cost(const vector<Cell> &cells, const BoardMask &cell_mask,
                                             const vs &park, const vvi &base_owner,
                                             const vector<GroupState> &groups, int mover_id, int arrival_id,
                                             ll current_s, long double theta) {
    int n = park.size();
    auto level = [&](int id) {
        long double remaining = max(0LL, groups[id].t - current_s);
        return -expm1l(-remaining / theta);
    };
    long double mover_level = level(mover_id);
    long double result = 0.0L;
    constexpr int DX[4] = {-1, 1, 0, 0};
    constexpr int DY[4] = {0, 0, -1, 1};
    for (auto [x, y] : cells) {
        for (int dir = 0; dir < 4; dir++) {
            int nx = x + DX[dir];
            int ny = y + DY[dir];
            if (!inside(nx, ny, n, n) || park[nx][ny] == '#') continue;
            int index = nx * n + ny;
            if ((cell_mask[index >> 6] >> (index & 63)) & 1ULL) continue;
            int neighbor = base_owner[nx][ny];
            long double neighbor_level = neighbor == -1 ? 0.0L : level(neighbor == arrival_id ? arrival_id : neighbor);
            result += fabsl(mover_level - neighbor_level);
        }
    }
    return result;
}

vector<RescueDestination> make_rescue_destinations(
    const vs &park, const vvi &base_owner, const vector<GroupState> &groups, int mover_id, int arrival_id,
    ll current_s, long double theta, const vector<Cell> &baseline_cells, const vector<char> &cleared_mask,
    const vector<vector<Shape>> &all_shapes, int &remaining_destination_anchors,
    int anchor_limit, int legal_limit, int destination_limit,
    RescueDiagnostics &diagnostics, PushOutHelperSurvey *helper_survey = nullptr,
    bool prefer_joint_exchange = false) {
    int n = park.size();
    const GroupState &group = groups[mover_id];
    vector<vi> blocked_prefix = make_blocked_prefix(park, base_owner);
    vector<char> fallback_mask(n * n, false);
    for (auto [x, y] : baseline_cells) fallback_mask[x * n + y] = true;
    vector<vi> fallback_prefix = make_flag_prefix(fallback_mask, n);
    vector<vi> cleared_prefix = make_flag_prefix(cleared_mask, n);
    vector<vi> foreign_cleared_prefix;
    if (prefer_joint_exchange) {
        vector<char> foreign_cleared_mask = cleared_mask;
        for (auto [x, y] : group.cells) foreign_cleared_mask[x * n + y] = false;
        foreign_cleared_prefix = make_flag_prefix(foreign_cleared_mask, n);
    }

    const vector<Shape> &shapes = all_shapes[group.p];
    ll previous_fee = round_payment(group.v, group.p, group.max_perimeter);
    vector<int> eligible_shapes;
    for (int shape_index = 0; shape_index < (int)shapes.size(); shape_index++) {
        ll next_fee = round_payment(group.v, group.p, max(group.max_perimeter, shapes[shape_index].perimeter));
        if (next_fee == previous_fee) eligible_shapes.push_back(shape_index);
    }
    if (eligible_shapes.empty()) return {};

    vector<int> samples(eligible_shapes.size());
    vector<int> anchor_counts(eligible_shapes.size());
    vector<int> starts(eligible_shapes.size());
    vector<int> strides(eligible_shapes.size());
    for (int index = 0; index < (int)eligible_shapes.size(); index++) {
        const Shape &shape = shapes[eligible_shapes[index]];
        int count = (n - shape.h + 1) * (n - shape.w + 1);
        anchor_counts[index] = count;
        starts[index] = (int)(((long long)(arrival_id + 1) * 1009 + (long long)(mover_id + 1) * 9176 +
                               (long long)(eligible_shapes[index] + 1) * 6113) %
                              count);
        int stride = max(1, count / 2 + 1 + index % 11);
        while (gcd(stride, count) != 1) stride++;
        strides[index] = stride;
    }

    vector<RescueDestination> legal;
    long long local_order = 0;
    int sampled_anchors = 0;
    while (remaining_destination_anchors > 0 && sampled_anchors < anchor_limit &&
           (int)legal.size() < legal_limit) {
        bool progressed = false;
        for (int index = 0; index < (int)eligible_shapes.size(); index++) {
            if (remaining_destination_anchors == 0 ||
                sampled_anchors >= anchor_limit || (int)legal.size() >= legal_limit) {
                break;
            }
            if (samples[index] >= anchor_counts[index]) continue;
            progressed = true;
            const Shape &shape = shapes[eligible_shapes[index]];
            int columns = n - shape.w + 1;
            int flat = (starts[index] + (long long)samples[index] * strides[index]) % anchor_counts[index];
            samples[index]++;
            sampled_anchors++;
            remaining_destination_anchors--;
            if (remaining_destination_anchors == 0) diagnostics.destination_limit_exhausted++;
            diagnostics.destination_anchors++;
            int base_x = flat / columns;
            int base_y = flat % columns;
            const Rect &a = shape.main_rect;
            const Rect &b = shape.extra_rect;
            int blocked_cells =
                rectangle_sum(blocked_prefix, base_x + a.x, base_y + a.y, a.h, a.w) +
                rectangle_sum(blocked_prefix, base_x + b.x, base_y + b.y, b.h, b.w);
            if (blocked_cells != 0) {
                if (helper_survey != nullptr && helper_survey->remaining_probes > 0) {
                    observe_pushout_helper_obstruction(
                        *helper_survey, shape, base_x, base_y, blocked_cells,
                        groups, mover_id, arrival_id, diagnostics);
                } else if (helper_survey != nullptr &&
                           !helper_survey->probe_limit_reported) {
                    helper_survey->probe_limit_reported = true;
                    diagnostics.pushout_helper_probe_limit_exhausted++;
                }
                continue;
            }

            vector<Cell> cells = materialize_shape(shape, base_x, base_y, group.p);
            if (same_region(cells, group.cells)) continue;
            uint64_t hash = placement_region_hash(cells);
            bool duplicate = false;
            for (const RescueDestination &existing : legal) {
                if (placement_region_hash(existing.cells) == hash && same_region(existing.cells, cells)) {
                    duplicate = true;
                    break;
                }
            }
            if (duplicate) continue;

            int fallback_overlap = rectangle_sum(fallback_prefix, base_x + a.x, base_y + a.y, a.h, a.w) +
                                   rectangle_sum(fallback_prefix, base_x + b.x, base_y + b.y, b.h, b.w);
            int cleared_overlap = rectangle_sum(cleared_prefix, base_x + a.x, base_y + a.y, a.h, a.w) +
                                  rectangle_sum(cleared_prefix, base_x + b.x, base_y + b.y, b.h, b.w);
            int foreign_cleared_overlap = 0;
            if (prefer_joint_exchange) {
                foreign_cleared_overlap =
                    rectangle_sum(foreign_cleared_prefix, base_x + a.x, base_y + a.y, a.h, a.w) +
                    rectangle_sum(foreign_cleared_prefix, base_x + b.x, base_y + b.y, b.h, b.w);
                diagnostics.pushout_helper_foreign_destination_candidates +=
                    foreign_cleared_overlap > 0;
            }
            int lower_half = 2 * base_x + shape.h >= n;
            int right_half = 2 * base_y + shape.w >= n;
            int sector_x = min(2, 3 * (2 * base_x + shape.h) / (2 * n));
            int sector_y = min(2, 3 * (2 * base_y + shape.w) / (2 * n));
            BoardMask mask = make_board_mask(cells, n);
            long double temporal_cost = rescue_destination_temporal_cost(
                cells, mask, park, base_owner, groups, mover_id, arrival_id, current_s, theta);
            legal.push_back({std::move(cells), mask, shape.perimeter, fallback_overlap,
                             cleared_overlap, foreign_cleared_overlap,
                             2 * lower_half + right_half, 3 * sector_x + sector_y,
                             temporal_cost, local_order++});
            diagnostics.destination_candidates++;
        }
        if (!progressed) break;
    }

    auto better = [](const RescueDestination &lhs, const RescueDestination &rhs) {
        if (lhs.fallback_overlap != rhs.fallback_overlap) return lhs.fallback_overlap > rhs.fallback_overlap;
        if (lhs.cleared_overlap != rhs.cleared_overlap) return lhs.cleared_overlap > rhs.cleared_overlap;
        if (lhs.temporal_cost != rhs.temporal_cost) return lhs.temporal_cost < rhs.temporal_cost;
        if (lhs.perimeter != rhs.perimeter) return lhs.perimeter < rhs.perimeter;
        return lhs.order < rhs.order;
    };
    vector<RescueDestination> result;
    auto add = [&](const RescueDestination &candidate) {
        for (const RescueDestination &existing : result) {
            if (same_region(existing.cells, candidate.cells)) return;
        }
        result.push_back(candidate);
    };
    if (!prefer_joint_exchange) {
        sort(legal.begin(), legal.end(), better);
        for (int index = 0; index < min(4, (int)legal.size()); index++) add(legal[index]);
        for (int quadrant = 0; quadrant < 4; quadrant++) {
            auto it = find_if(legal.begin(), legal.end(),
                              [&](const RescueDestination &candidate) {
                                  return candidate.quadrant == quadrant;
                              });
            if (it != legal.end()) add(*it);
        }
        for (const RescueDestination &candidate : legal) {
            if ((int)result.size() == destination_limit) break;
            add(candidate);
        }
    } else {
        // The old order made every mover chase the same pre-existing free
        // cells.  Preserve several such choices, but explicitly retain moves
        // into another mover's old region and spatially diverse alternatives.
        auto joint_better = [](const RescueDestination &lhs,
                               const RescueDestination &rhs) {
            if (lhs.foreign_cleared_overlap != rhs.foreign_cleared_overlap) {
                return lhs.foreign_cleared_overlap > rhs.foreign_cleared_overlap;
            }
            if (lhs.cleared_overlap != rhs.cleared_overlap) {
                return lhs.cleared_overlap > rhs.cleared_overlap;
            }
            if (lhs.fallback_overlap != rhs.fallback_overlap) {
                return lhs.fallback_overlap > rhs.fallback_overlap;
            }
            if (lhs.temporal_cost != rhs.temporal_cost) {
                return lhs.temporal_cost < rhs.temporal_cost;
            }
            if (lhs.perimeter != rhs.perimeter) return lhs.perimeter < rhs.perimeter;
            return lhs.order < rhs.order;
        };
        auto add_best = [&](auto comparator, int count) {
            vector<int> order(legal.size());
            iota(order.begin(), order.end(), 0);
            sort(order.begin(), order.end(), [&](int lhs, int rhs) {
                return comparator(legal[lhs], legal[rhs]);
            });
            for (int index : order) {
                if (count == 0 || (int)result.size() == destination_limit) break;
                int before = result.size();
                add(legal[index]);
                count -= (int)result.size() != before;
            }
        };
        add_best(joint_better, 4);
        add_best(better, 4);
        for (int sector = 0; sector < 9 && (int)result.size() < destination_limit;
             sector++) {
            const RescueDestination *best_in_sector = nullptr;
            for (const RescueDestination &candidate : legal) {
                if (candidate.sector != sector) continue;
                if (best_in_sector == nullptr || joint_better(candidate, *best_in_sector)) {
                    best_in_sector = &candidate;
                }
            }
            if (best_in_sector != nullptr) add(*best_in_sector);
        }
        add_best(joint_better, destination_limit);
    }
    if ((int)result.size() > destination_limit) result.resize(destination_limit);
    if (prefer_joint_exchange) {
        for (const RescueDestination &candidate : result) {
            diagnostics.pushout_helper_retained_foreign_destinations +=
                candidate.foreign_cleared_overlap > 0;
        }
    }
    return result;
}

struct RescueBeamState {
    BoardMask occupied{};
    vector<int> choice;
    long double rank = 0.0L;
    long long order = 0;
};

optional<vector<int>> repair_rescue_blockers(const vvi &base_owner, const vector<GroupState> &groups,
                                             const vector<int> &blockers,
                                             const vector<vector<RescueDestination>> &pools,
                                             int &remaining_nodes, RescueDiagnostics &diagnostics) {
    int blocker_count = blockers.size();
    vector<vector<int>> orders;
    auto add_order = [&](vector<int> order) {
        for (const vector<int> &existing : orders) {
            if (existing == order) return;
        }
        orders.push_back(std::move(order));
    };
    vector<int> indices(blocker_count);
    iota(indices.begin(), indices.end(), 0);
    sort(indices.begin(), indices.end(), [&](int lhs, int rhs) {
        if (pools[lhs].size() != pools[rhs].size()) return pools[lhs].size() < pools[rhs].size();
        if (groups[blockers[lhs]].p != groups[blockers[rhs]].p) {
            return groups[blockers[lhs]].p > groups[blockers[rhs]].p;
        }
        return blockers[lhs] < blockers[rhs];
    });
    add_order(indices);
    sort(indices.begin(), indices.end(), [&](int lhs, int rhs) {
        if (groups[blockers[lhs]].p != groups[blockers[rhs]].p) {
            return groups[blockers[lhs]].p > groups[blockers[rhs]].p;
        }
        return blockers[lhs] < blockers[rhs];
    });
    add_order(indices);
    sort(indices.begin(), indices.end(),
         [&](int lhs, int rhs) { return tie(groups[blockers[lhs]].t, blockers[lhs]) <
                                        tie(groups[blockers[rhs]].t, blockers[rhs]); });
    add_order(indices);
    reverse(indices.begin(), indices.end());
    add_order(indices);

    BoardMask base_mask = make_occupied_mask(base_owner);
    // Try every deterministic spine before spending any beam nodes.  Thus an
    // exhausted first beam cannot hide an easy arbitrary-depth solution under
    // a different insertion order.
    for (const vector<int> &insertion_order : orders) {
        BoardMask greedy_mask = base_mask;
        vector<int> greedy_choice(blocker_count, -1);
        bool greedy_complete = true;
        for (int pool_index : insertion_order) {
            bool placed = false;
            for (int candidate_index = 0; candidate_index < (int)pools[pool_index].size(); candidate_index++) {
                const RescueDestination &candidate = pools[pool_index][candidate_index];
                if (masks_overlap(greedy_mask, candidate.mask)) continue;
                merge_mask(greedy_mask, candidate.mask);
                greedy_choice[pool_index] = candidate_index;
                placed = true;
                break;
            }
            if (!placed) {
                greedy_complete = false;
                break;
            }
        }
        if (greedy_complete) return greedy_choice;
    }

    // The bounded beam only repairs greedy collisions; it is not the only path
    // capable of reaching the full blocker depth.
    long long state_order = 0;
    for (const vector<int> &insertion_order : orders) {
        vector<RescueBeamState> beam(1);
        beam.front().occupied = base_mask;
        beam.front().choice.assign(blocker_count, -1);
        bool exhausted = false;
        for (int position = 0; position < blocker_count; position++) {
            int pool_index = insertion_order[position];
            vector<RescueBeamState> next;
            for (const RescueBeamState &state : beam) {
                for (int candidate_index = 0; candidate_index < (int)pools[pool_index].size(); candidate_index++) {
                    const RescueDestination &candidate = pools[pool_index][candidate_index];
                    if (masks_overlap(state.occupied, candidate.mask)) continue;
                    if (remaining_nodes == 0) {
                        exhausted = true;
                        break;
                    }
                    remaining_nodes--;
                    diagnostics.beam_nodes++;
                    RescueBeamState child = state;
                    merge_mask(child.occupied, candidate.mask);
                    child.choice[pool_index] = candidate_index;
                    child.rank +=
                        10000.0L * candidate.foreign_cleared_overlap +
                        1000.0L * candidate.fallback_overlap +
                        10.0L * candidate.cleared_overlap - candidate.temporal_cost -
                        0.01L * candidate.perimeter;
                    child.order = state_order++;
                    next.push_back(std::move(child));
                }
                if (exhausted) break;
            }
            if (next.empty()) {
                beam.clear();
                break;
            }
            sort(next.begin(), next.end(), [](const RescueBeamState &lhs, const RescueBeamState &rhs) {
                if (lhs.rank != rhs.rank) return lhs.rank > rhs.rank;
                return lhs.order < rhs.order;
            });
            if ((int)next.size() > RESCUE_BEAM_WIDTH) next.resize(RESCUE_BEAM_WIDTH);
            beam = std::move(next);
            if (exhausted) break;
        }
        if (!beam.empty() && count(beam.front().choice.begin(), beam.front().choice.end(), -1) == 0) {
            return beam.front().choice;
        }
        if (exhausted) break;
    }
    return nullopt;
}

bool validate_and_build_rescue_owner(const TurnPlan &plan, const vs &park, const vvi &owner,
                                     const vector<GroupState> &groups, int arrival_id, int r_milli,
                                     vvi &final_owner, ll &fee_loss, ll &movement_cost_sum) {
    if (plan.moves.empty() || !plan.arrival || groups[arrival_id].active) return false;
    int n = park.size();
    vector<char> moved(groups.size(), false);
    final_owner = owner;
    for (const MovePlan &move : plan.moves) {
        if (move.id < 0 || move.id >= (int)groups.size() || moved[move.id] || !groups[move.id].active) {
            return false;
        }
        moved[move.id] = true;
        const GroupState &group = groups[move.id];
        if ((int)group.cells.size() != group.p) return false;
        for (auto [x, y] : group.cells) {
            if (!inside(x, y, n, n) || final_owner[x][y] != move.id) return false;
        }
    }
    for (const MovePlan &move : plan.moves) clear_cells(final_owner, groups[move.id].cells);

    auto region_is_legal = [&](const vector<Cell> &cells, int expected_size) {
        if ((int)cells.size() != expected_size || !validate_connected_region(cells, n)) return false;
        for (auto [x, y] : cells) {
            if (!inside(x, y, n, n) || park[x][y] != '.' || final_owner[x][y] != -1) return false;
        }
        return true;
    };

    fee_loss = 0;
    movement_cost_sum = 0;
    for (const MovePlan &move : plan.moves) {
        const GroupState &group = groups[move.id];
        if (!region_is_legal(move.cells, group.p) || same_region(move.cells, group.cells)) return false;
        int perimeter = calc_perimeter(move.cells, n);
        if (perimeter != move.perimeter) return false;
        ll previous_fee = round_payment(group.v, group.p, group.max_perimeter);
        ll next_fee = round_payment(group.v, group.p, max(group.max_perimeter, perimeter));
        fee_loss += previous_fee - next_fee;
        movement_cost_sum += move_cost(group.v, r_milli);
        place_cells(final_owner, move.cells, move.id);
    }

    const GroupState &arrival = groups[arrival_id];
    if (!region_is_legal(*plan.arrival, arrival.p) ||
        calc_perimeter(*plan.arrival, n) != plan.arrival_perimeter) {
        return false;
    }
    ll expected_gain = round_payment(arrival.v, arrival.p, plan.arrival_perimeter) - movement_cost_sum;
    place_cells(final_owner, *plan.arrival, arrival_id);
    return expected_gain == plan.immediate_gain;
}

struct RescueSyntheticArrival {
    ll s = 0;
    ll t = 0;
    int p = 0;
    ll v = 0;
    long double theta = 0.0L;
    int remaining_after = 0;
};

long double rescue_radical_inverse(uint64_t index, int base) {
    long double inverse_base = 1.0L / base;
    long double place = inverse_base;
    long double result = 0.0L;
    while (index > 0) {
        result += (index % base) * place;
        index /= base;
        place *= inverse_base;
    }
    return clamp(result, 1e-9L, 1.0L - 1e-9L);
}

uint64_t rescue_sequence_offset(const vector<GroupState> &groups, int arrival_id, ll current_s) {
    uint64_t value = (uint64_t)(arrival_id + 1) * 0x9e3779b97f4a7c15ULL;
    value ^= (uint64_t)current_s + 0xbf58476d1ce4e5b9ULL;
    value ^= (uint64_t)groups[arrival_id].v * 0x94d049bb133111ebULL;
    value = (value ^ (value >> 30)) * 0xbf58476d1ce4e5b9ULL;
    value = (value ^ (value >> 27)) * 0x94d049bb133111ebULL;
    return (value ^ (value >> 31)) % ROOT_ROLLOUT_SEQUENCE_BLOCK_SIZE;
}

struct RescueRolloutScenarios {
    vector<vector<RescueSyntheticArrival>> arrivals;
    bool complete = true;
};

RescueRolloutScenarios make_rescue_rollout_scenarios(
    const vector<GroupState> &groups, int arrival_id, ll current_s, int remaining_groups, long double theta,
    const ThetaEstimator &theta_estimator, int scenario_count = ROOT_SCREEN_SCENARIO_COUNT,
    int rollout_horizon = ROOT_SCREEN_ROLLOUT_LENGTH, bool posterior_predictive = false, int batch = 0) {
    RescueRolloutScenarios result;
    if (scenario_count <= 0 || scenario_count % 2 != 0 || rollout_horizon < 0 || batch < 0) {
        result.complete = false;
        return result;
    }
    result.arrivals.resize(scenario_count);
    if (remaining_groups <= 0 || rollout_horizon == 0) return result;

    struct RawArrival {
        ll s;
        ll t;
        int p;
        ll v;
        long long order;
    };

    uint64_t sequence_offset = rescue_sequence_offset(groups, arrival_id, current_s);
    const long double size_width = sqrtl(150.0L) - 2.0L;

    for (int scenario = 0; scenario < scenario_count; scenario++) {
        int pair_index = scenario / 2;
        bool antithetic = scenario % 2 == 1;
        // A sampled latent theta controls only future-input generation.  The
        // online policy's theta stored in each spec is still re-estimated below.
        long double generation_theta = theta;
        if (posterior_predictive) {
            long double pair_quantile = (2.0L * pair_index + 1.0L) / scenario_count;
            generation_theta =
                theta_estimator.posterior_quantile(current_s, remaining_groups, pair_quantile);
        }
        ConditionalFutureDemand future_demand(current_s, generation_theta);
        // Batch 0 / pair 0 is exactly the legacy screen sequence.  Confirmation
        // uses batch 1 and four disjoint blocks, one per antithetic pair.
        uint64_t sequence_block =
            (uint64_t)batch * ROOT_ROLLOUT_SEQUENCE_BLOCKS_PER_BATCH + pair_index;
        uint64_t sequence_block_offset = sequence_block * ROOT_ROLLOUT_SEQUENCE_BLOCK_SIZE;

        set<ll> used_times;
        for (int id = 0; id <= arrival_id; id++) {
            used_times.insert(groups[id].s);
            used_times.insert(groups[id].t);
        }

        vector<RawArrival> generated;
        generated.reserve(remaining_groups);
        int maximum_attempts = 8 * remaining_groups + 64;
        for (int attempt = 0; (int)generated.size() < remaining_groups && attempt < maximum_attempts; attempt++) {
            uint64_t index = sequence_offset + sequence_block_offset + attempt + 1;
            auto quantile = [&](int base) {
                long double value = rescue_radical_inverse(index, base);
                return antithetic ? 1.0L - value : value;
            };
            long double duration_quantile = quantile(2);
            long double start_quantile = quantile(3);
            long double size_quantile = quantile(5);
            long double value_quantile = quantile(7);

            long double total_weight = 0.0L;
            array<long double, THETA_QUADRATURE_STEPS> node_weight{};
            for (int node_index = 0; node_index < THETA_QUADRATURE_STEPS; node_index++) {
                const auto &node = future_demand.nodes[node_index];
                ll duration = max(1LL, (ll)llroundl(node.stay_duration - 1.0L) + 1);
                ll maximum_start = ARRIVAL_TIME_HORIZON - duration;
                long double available_starts = max(0LL, maximum_start - current_s);
                node_weight[node_index] = node.joint_weight * available_starts;
                total_weight += node_weight[node_index];
            }
            if (total_weight <= 0.0L) continue;

            long double target = duration_quantile * total_weight;
            int chosen_node = THETA_QUADRATURE_STEPS - 1;
            long double prefix = 0.0L;
            for (int node_index = 0; node_index < THETA_QUADRATURE_STEPS; node_index++) {
                prefix += node_weight[node_index];
                if (prefix >= target) {
                    chosen_node = node_index;
                    break;
                }
            }
            ll duration = max(1LL, (ll)llroundl(future_demand.nodes[chosen_node].stay_duration - 1.0L) + 1);
            ll maximum_start = ARRIVAL_TIME_HORIZON - duration;
            ll start_count = maximum_start - current_s;
            if (start_count <= 0) continue;
            ll start = current_s + 1 + min(start_count - 1, (ll)floorl(start_quantile * start_count));
            ll end = start + duration;

            long double root_size = 2.0L + size_width * size_quantile;
            int p = clamp((int)llroundl(root_size * root_size), 4, 150);
            long double noise = 0.8L * inverse_standard_normal(value_quantile);
            long double raw_v = p * powl((long double)duration, 0.9L) * exp2l(noise);
            ll v = clamp((ll)llroundl(raw_v), 1LL, 100000000LL);
            if (used_times.count(start) || used_times.count(end)) continue;
            used_times.insert(start);
            used_times.insert(end);
            generated.push_back({start, end, p, v, attempt});
        }
        // The rollout needs the chronological prefix among all remaining
        // groups.  A prefix of a partial sample would be biased toward later S.
        if ((int)generated.size() != remaining_groups) {
            result.complete = false;
            return result;
        }

        sort(generated.begin(), generated.end(), [](const RawArrival &lhs, const RawArrival &rhs) {
            if (lhs.s != rhs.s) return lhs.s < rhs.s;
            if (lhs.t != rhs.t) return lhs.t < rhs.t;
            return lhs.order < rhs.order;
        });

        ThetaEstimator rollout_theta_estimator = theta_estimator;
        int rollout_length = min(rollout_horizon, remaining_groups);
        for (const RawArrival &raw : generated) {
            int remaining_after = remaining_groups - (int)result.arrivals[scenario].size() - 1;
            rollout_theta_estimator.observe(raw.t - raw.s);
            long double rollout_theta = rollout_theta_estimator.estimate(raw.s, remaining_after);
            result.arrivals[scenario].push_back({raw.s, raw.t, raw.p, raw.v, rollout_theta, remaining_after});
            if ((int)result.arrivals[scenario].size() == rollout_length) break;
        }
    }
    return result;
}

struct RescueRolloutState {
    vvi owner;
    vector<GroupState> groups;
    priority_queue<pair<ll, int>, vector<pair<ll, int>>, greater<pair<ll, int>>> departures;
};

RescueRolloutState make_rescue_rollout_state(const vvi &final_owner, const vector<GroupState> &groups,
                                             int arrival_id, const TurnPlan &plan, int synthetic_count) {
    RescueRolloutState state;
    state.owner = final_owner;
    state.groups = groups;
    state.groups.reserve(state.groups.size() + synthetic_count);
    for (const MovePlan &move : plan.moves) {
        GroupState &moved = state.groups[move.id];
        moved.cells = move.cells;
        chmax(moved.max_perimeter, move.perimeter);
    }
    if (plan.arrival) {
        GroupState &arrival = state.groups[arrival_id];
        arrival.active = true;
        arrival.cells = *plan.arrival;
        arrival.max_perimeter = plan.arrival_perimeter;
    }
    for (int id = 0; id < (int)state.groups.size(); id++) {
        if (state.groups[id].active) state.departures.emplace(state.groups[id].t, id);
    }
    return state;
}

struct RescueRolloutOutcome {
    ll fee = 0;
    int acceptances = 0;
};

RescueRolloutOutcome evaluate_rescue_rollout_branch(
    const vs &park, const vvi &final_owner, const vector<GroupState> &groups, int arrival_id,
    const TurnPlan &plan, const vector<RescueSyntheticArrival> &scenario, int grass_cells,
    const DensityModel &density_model, SampledDlpShadowModel &sampled_dlp_model,
    const vector<vector<Shape>> &compact_shapes) {
    // Future arrivals use the ordinary online policy and never recurse into
    // rescue.  Only their realized fees are compared between the two roots.
    RescueRolloutState state =
        make_rescue_rollout_state(final_owner, groups, arrival_id, plan, scenario.size());
    RescueRolloutOutcome result;
    for (const RescueSyntheticArrival &spec : scenario) {
        while (!state.departures.empty() && state.departures.top().first < spec.s) {
            int id = state.departures.top().second;
            state.departures.pop();
            if (!state.groups[id].active) continue;
            clear_cells(state.owner, state.groups[id].cells);
            state.groups[id].cells.clear();
            state.groups[id].active = false;
        }

        int synthetic_id = state.groups.size();
        state.groups.push_back(GroupState{});
        GroupState &synthetic = state.groups.back();
        synthetic.s = spec.s;
        synthetic.t = spec.t;
        synthetic.v = spec.v;
        synthetic.p = spec.p;

        ShadowEvaluation shadow;
        if constexpr (ENABLE_SAMPLED_DLP) {
            // Every root branch shares the real turn's frozen bid-price
            // snapshot.  Re-solving from a sampled branch would give its
            // synthetic future privileged information and would no longer be
            // the isolated DLP-for-shadow replacement tested here.
            shadow = sampled_dlp_model.evaluate_cached(
                spec.s, spec.t, spec.p, true, spec.remaining_after);
        } else {
            shadow = evaluate_shadow_cost(state.groups, spec.s, spec.t, spec.p,
                                          spec.remaining_after, grass_cells, spec.theta,
                                          density_model);
        }
        ArrivalDecision decision = evaluate_arrival_decision(
            park, state.owner, state.groups, synthetic_id, spec.s, spec.remaining_after, spec.theta,
            shadow.opportunity_cost, compact_shapes);
        if (!decision.cells) continue;

        place_cells(state.owner, *decision.cells, synthetic_id);
        synthetic.active = true;
        synthetic.cells = *decision.cells;
        synthetic.max_perimeter = decision.perimeter;
        state.departures.emplace(spec.t, synthetic_id);
        result.fee += decision.fee;
        result.acceptances++;
    }
    return result;
}

struct RootBranchView {
    const TurnPlan *plan = nullptr;
    const vvi *final_owner = nullptr;
    ll direct_vs_baseline = 0;
};

bool confirm_root_override(
    const vs &park, const vector<GroupState> &groups, int arrival_id, ll current_s, int remaining_groups,
    long double theta, const ThetaEstimator &theta_estimator, int grass_cells,
    const DensityModel &density_model, SampledDlpShadowModel &sampled_dlp_model,
    const vector<vector<Shape>> &compact_shapes,
    const RootBranchView &protected_branch, const RootBranchView &challenger_branch,
    int &confirmations_used, RescueDiagnostics &diagnostics) {
    assert(protected_branch.plan != nullptr && protected_branch.final_owner != nullptr);
    assert(challenger_branch.plan != nullptr && challenger_branch.final_owner != nullptr);
    if (confirmations_used >= ROOT_CONFIRMATION_TURN_LIMIT) {
        diagnostics.root_confirmation_budget_skips++;
        return false;
    }
    confirmations_used++;
    diagnostics.root_confirmation_attempts++;

    RescueRolloutScenarios scenarios = make_rescue_rollout_scenarios(
        groups, arrival_id, current_s, remaining_groups, theta, theta_estimator,
        ROOT_CONFIRM_SCENARIO_COUNT, ROOT_CONFIRM_ROLLOUT_LENGTH, true, 1);
    int expected_length = min(ROOT_CONFIRM_ROLLOUT_LENGTH, remaining_groups);
    bool generation_ok = scenarios.complete &&
                         (int)scenarios.arrivals.size() == ROOT_CONFIRM_SCENARIO_COUNT;
    if (generation_ok) {
        for (const auto &scenario : scenarios.arrivals) {
            if ((int)scenario.size() != expected_length) {
                generation_ok = false;
                break;
            }
        }
    }
    if (!generation_ok) {
        diagnostics.root_confirmation_generation_failures++;
        return false;
    }
    if (expected_length == ROOT_CONFIRM_ROLLOUT_LENGTH) {
        diagnostics.root_confirmation_full_horizon++;
    } else {
        diagnostics.root_confirmation_short_horizon++;
    }

    i128 future_delta_sum = 0;
    array<bool, ROOT_CONFIRM_SCENARIO_COUNT> positive_scenario{};
    ll direct_delta =
        challenger_branch.direct_vs_baseline - protected_branch.direct_vs_baseline;
    for (int scenario_index = 0; scenario_index < ROOT_CONFIRM_SCENARIO_COUNT; scenario_index++) {
        const auto &scenario = scenarios.arrivals[scenario_index];
        RescueRolloutOutcome protected_outcome = evaluate_rescue_rollout_branch(
            park, *protected_branch.final_owner, groups, arrival_id, *protected_branch.plan,
            scenario, grass_cells, density_model, sampled_dlp_model, compact_shapes);
        RescueRolloutOutcome challenger_outcome = evaluate_rescue_rollout_branch(
            park, *challenger_branch.final_owner, groups, arrival_id, *challenger_branch.plan,
            scenario, grass_cells, density_model, sampled_dlp_model, compact_shapes);
        diagnostics.root_confirmation_policy_steps += 2LL * scenario.size();
        ll future_delta = challenger_outcome.fee - protected_outcome.fee;
        future_delta_sum += future_delta;
        positive_scenario[scenario_index] = (i128)direct_delta + future_delta > 0;
        diagnostics.root_confirmation_positive_scenarios += positive_scenario[scenario_index];
    }
    diagnostics.root_confirmation_scenarios += ROOT_CONFIRM_SCENARIO_COUNT;
    for (int pair = 0; pair < ROOT_CONFIRM_SCENARIO_COUNT / 2; pair++) {
        diagnostics.root_confirmation_pair_disagreements +=
            positive_scenario[2 * pair] != positive_scenario[2 * pair + 1];
    }

    i128 margin_times_scenarios =
        (i128)ROOT_CONFIRM_SCENARIO_COUNT * direct_delta +
        future_delta_sum;
    long double holdout_margin =
        (long double)margin_times_scenarios / ROOT_CONFIRM_SCENARIO_COUNT;
    diagnostics.root_confirmation_holdout_margin += holdout_margin;
    if (margin_times_scenarios > 0) {
        diagnostics.root_confirmation_approved++;
        diagnostics.root_confirmation_approved_margin += holdout_margin;
        return true;
    }
    diagnostics.root_confirmation_rejected++;
    diagnostics.root_confirmation_rejected_margin += holdout_margin;
    return false;
}

struct RootActionResult {
    TurnPlan plan;
    ArrivalDecision arrival_decision;
};

enum class RootActionKind {
    Baseline,
    Rescue,
    NormalAlternative,
};

enum class RescueMode {
    CompactAccepted,
    NoRegionPushOut,
};

struct PreparedRescueCandidate {
    TurnPlan plan;
    vvi final_owner;
    // blockers occupy the arrival target; movers additionally contains the
    // optional helper.  Keep them separate so blocker histograms retain their
    // original meaning while costs and emitted moves cover every mover.
    vector<int> blockers;
    vector<int> movers;
    int helper_id = -1;
    ll compact_fee = 0;
    ll direct_gain = 0;
    ll movement_cost = 0;
    bool blocker_uses_helper_region = false;
    bool helper_uses_blocker_region = false;
};

struct PushOutHelperSeed {
    int target_index = -1;
    PushOutHelperChoice helper;
    ll adjusted_direct_gain = 0;
};

pair<bool, bool> classify_pushout_helper_exchange(
    const TurnPlan &plan, const vector<GroupState> &groups,
    const vector<int> &blockers, int helper_id, int n) {
    vector<char> helper_old(n * n, false);
    vector<char> blocker_old(n * n, false);
    for (auto [x, y] : groups[helper_id].cells) helper_old[x * n + y] = true;
    for (int id : blockers) {
        for (auto [x, y] : groups[id].cells) blocker_old[x * n + y] = true;
    }
    bool blocker_uses_helper = false;
    bool helper_uses_blocker = false;
    for (const MovePlan &move : plan.moves) {
        if (move.id == helper_id) {
            for (auto [x, y] : move.cells) helper_uses_blocker |= blocker_old[x * n + y];
        } else if (binary_search(blockers.begin(), blockers.end(), move.id)) {
            for (auto [x, y] : move.cells) blocker_uses_helper |= helper_old[x * n + y];
        }
    }
    return {blocker_uses_helper, helper_uses_blocker};
}

struct PushOutHelperPhaseScope {
    RescueDiagnostics &diagnostics;
    clock_t cpu_begin;
    long long destination_anchors;
    long long destination_candidates;
    long long beam_nodes;

    explicit PushOutHelperPhaseScope(RescueDiagnostics &diagnostics)
        : diagnostics(diagnostics),
          cpu_begin(clock()),
          destination_anchors(diagnostics.destination_anchors),
          destination_candidates(diagnostics.destination_candidates),
          beam_nodes(diagnostics.beam_nodes) {}

    ~PushOutHelperPhaseScope() {
        diagnostics.pushout_helper_destination_anchors +=
            diagnostics.destination_anchors - destination_anchors;
        diagnostics.pushout_helper_destination_candidates +=
            diagnostics.destination_candidates - destination_candidates;
        diagnostics.pushout_helper_beam_nodes += diagnostics.beam_nodes - beam_nodes;
        clock_t cpu_end = clock();
        if (cpu_begin != (clock_t)-1 && cpu_end != (clock_t)-1) {
            diagnostics.pushout_helper_phase_cpu_seconds +=
                (double)(cpu_end - cpu_begin) / CLOCKS_PER_SEC;
        }
    }
};

optional<RootActionResult> choose_root_action_with_rescue(
    const vs &park, const vvi &owner, const vector<GroupState> &groups, int arrival_id, ll current_s,
    int remaining_groups, int r_milli, long double theta, const ThetaEstimator &theta_estimator,
    const DensityModel &density_model, SampledDlpShadowModel &sampled_dlp_model,
    int grass_cells, long double opportunity_cost,
    const ArrivalDecision &baseline,
    const vector<NormalPlacementChoice> &normal_alternatives,
    const vector<vector<Shape>> &compact_shapes, const vector<vector<Shape>> &all_shapes,
    int &confirmations_used, bool &root_screen_evaluated, RescueDiagnostics &diagnostics) {
    root_screen_evaluated = false;
    const GroupState &arrival = groups[arrival_id];
    int minimum_perimeter = compact_shapes[arrival.p].front().perimeter;
    bool compact_rescue = baseline.status == ArrivalStatus::Accepted && baseline.cells &&
                          baseline.perimeter > minimum_perimeter + COMPACT_PERIMETER_MARGIN;
    bool no_region_pushout = ENABLE_NO_REGION_PUSHOUT && baseline.status == ArrivalStatus::NoRegion &&
                             !baseline.cells;
    if (!compact_rescue && !no_region_pushout) {
        return nullopt;
    }
    RescueMode mode = no_region_pushout ? RescueMode::NoRegionPushOut : RescueMode::CompactAccepted;
    PushOutDiagnosticScope pushout_scope(diagnostics, no_region_pushout);
    vector<Cell> preexisting_free_cells;
    if (no_region_pushout) {
        preexisting_free_cells.reserve(park.size() * park.size());
        diagnostics.pushout_eligible++;
        for (int x = 0; x < (int)park.size(); x++) {
            for (int y = 0; y < (int)park.size(); y++) {
                if (park[x][y] == '.' && owner[x][y] == -1) {
                    preexisting_free_cells.emplace_back(x, y);
                }
            }
        }
        // Relocation preserves occupied area, so it cannot repair a capacity
        // shortage.  Restrict Push-out to genuinely fragmented NoRegion turns.
        if ((int)preexisting_free_cells.size() < arrival.p) {
            diagnostics.pushout_area_insufficient++;
            return nullopt;
        }
    } else {
        diagnostics.eligible_fallbacks++;
    }

    ll baseline_score = compact_rescue ? baseline.fee : 0;
    if (no_region_pushout) {
        ll minimum_move_cost = numeric_limits<ll>::max();
        for (const GroupState &group : groups) {
            if (group.active) chmin(minimum_move_cost, move_cost(group.v, r_milli));
        }
        ll compact_fee = round_payment(arrival.v, arrival.p, minimum_perimeter);
        // Every Push-out target has at least one blocker.  If even the
        // cheapest possible single move fails the admission shadow, no exact
        // target scan can produce an eligible action.
        if (minimum_move_cost == numeric_limits<ll>::max() ||
            (long double)(compact_fee - minimum_move_cost) <= opportunity_cost) {
            diagnostics.no_economic_target++;
            diagnostics.pushout_no_economic_target++;
            return nullopt;
        }
    }
    // Push-out must first satisfy the same full-horizon admission shadow as an
    // ordinary arrival.  The short common-random-number rollout below is an
    // additional veto for near-future geometry, not another score deduction.
    long double direct_gain_threshold = no_region_pushout ? opportunity_cost : 0.0L;
    vector<RescueTarget> targets = make_rescue_targets(
        park, owner, groups, arrival_id, r_milli, baseline_score, direct_gain_threshold,
        no_region_pushout,
        no_region_pushout ? PUSHOUT_TARGET_SHORTLIST_PER_METRIC : RESCUE_TARGET_SHORTLIST_PER_METRIC,
        compact_shapes, diagnostics);
    if (targets.empty()) {
        diagnostics.no_economic_target++;
        if (no_region_pushout) diagnostics.pushout_no_economic_target++;
        return nullopt;
    }

    int target_repair_limit =
        no_region_pushout ? PUSHOUT_TARGET_REPAIR_LIMIT : RESCUE_TARGET_REPAIR_LIMIT;
    int destination_anchor_limit =
        no_region_pushout ? PUSHOUT_DESTINATION_ANCHOR_LIMIT : RESCUE_DESTINATION_ANCHOR_LIMIT;
    int destination_legal_limit =
        no_region_pushout ? PUSHOUT_DESTINATION_LEGAL_LIMIT : RESCUE_DESTINATION_LEGAL_LIMIT;
    int destination_limit =
        no_region_pushout ? PUSHOUT_DESTINATION_LIMIT : RESCUE_DESTINATION_LIMIT;
    int remaining_nodes =
        no_region_pushout ? PUSHOUT_REPAIR_NODE_LIMIT : RESCUE_REPAIR_NODE_LIMIT;
    int remaining_destination_anchors =
        no_region_pushout ? PUSHOUT_DESTINATION_ANCHOR_GLOBAL_LIMIT
                          : RESCUE_DESTINATION_ANCHOR_GLOBAL_LIMIT;
    int attempted_targets = 0;
    vector<PreparedRescueCandidate> candidates;
    RescueRolloutScenarios rollout_scenarios;
    bool rollout_ready = false;
    bool stop_after_primary = false;
    vector<PushOutHelperSeed> helper_seeds;
    bool helper_raw_evidence = false;
    bool helper_surveyed_turn = false;
    const vector<Cell> &preferred_destination_cells =
        mode == RescueMode::NoRegionPushOut ? preexisting_free_cells : *baseline.cells;

    auto initialize_rollout_for_first_candidate = [&]() {
        diagnostics.feasible_turns++;
        if (no_region_pushout) diagnostics.pushout_feasible_turns++;
        if (remaining_groups == 0) {
            diagnostics.rollout_skipped_no_future++;
            stop_after_primary = true;
            return true;
        }

        rollout_scenarios = make_rescue_rollout_scenarios(
            groups, arrival_id, current_s, remaining_groups, theta, theta_estimator);
        int expected_length = min(RESCUE_ROLLOUT_LENGTH, remaining_groups);
        bool generation_ok = rollout_scenarios.complete;
        for (const auto &scenario : rollout_scenarios.arrivals) {
            if ((int)scenario.size() != expected_length) {
                generation_ok = false;
                break;
            }
        }
        if (!generation_ok) {
            diagnostics.rollout_generation_failures++;
            if (no_region_pushout) {
                // Reject-to-Accept is a larger intervention.  Without the
                // common-random-number comparison, retain Reject.
                diagnostics.pushout_rollout_generation_failures++;
                diagnostics.pushout_screen_rejected++;
                if (candidates.size() == 1 && candidates.front().helper_id != -1) {
                    diagnostics.pushout_helper_screen_rejected++;
                }
                return false;
            }
            // Scenario construction is only a filter for the existing
            // Accepted rescue.  Preserve its legal positive-direct action.
            stop_after_primary = true;
            return true;
        }
        rollout_ready = true;
        return true;
    };

    for (int target_index = 0; target_index < (int)targets.size(); target_index++) {
        const RescueTarget &target = targets[target_index];
        if (attempted_targets == target_repair_limit || remaining_nodes == 0 ||
            (int)candidates.size() == RESCUE_ROLLOUT_CANDIDATE_LIMIT) {
            break;
        }
        attempted_targets++;
        diagnostics.repair_attempts++;
        chmax(diagnostics.maximum_blockers, (int)target.blockers.size());

        vvi base_owner = owner;
        vector<char> cleared_mask(park.size() * park.size(), false);
        for (int id : target.blockers) {
            for (auto [x, y] : groups[id].cells) cleared_mask[x * park.size() + y] = true;
            clear_cells(base_owner, groups[id].cells);
        }
        bool target_legal = true;
        for (auto [x, y] : target.cells) {
            if (park[x][y] != '.' || base_owner[x][y] != -1) {
                target_legal = false;
                break;
            }
            base_owner[x][y] = arrival_id;
        }
        if (!target_legal) {
            diagnostics.validation_failures++;
            continue;
        }

        optional<PushOutHelperSurvey> helper_survey;
        if constexpr (ENABLE_PUSHOUT_HELPER) {
            if (no_region_pushout &&
                (int)target.blockers.size() <= PUSHOUT_HELPER_MAX_BLOCKERS) {
                helper_survey.emplace(groups.size(), base_owner);
                diagnostics.pushout_helper_surveyed_targets++;
                if (!helper_surveyed_turn) {
                    helper_surveyed_turn = true;
                    diagnostics.pushout_helper_surveyed_turns++;
                }
            } else if (no_region_pushout) {
                diagnostics.pushout_helper_large_blocker_targets++;
            }
        }
        vector<vector<RescueDestination>> pools;
        pools.reserve(target.blockers.size());
        bool missing_destination = false;
        for (int id : target.blockers) {
            vector<RescueDestination> pool = make_rescue_destinations(
                park, base_owner, groups, id, arrival_id, current_s, theta,
                preferred_destination_cells, cleared_mask,
                all_shapes, remaining_destination_anchors, destination_anchor_limit,
                destination_legal_limit, destination_limit, diagnostics,
                helper_survey ? &*helper_survey : nullptr);
            if (pool.empty()) {
                missing_destination = true;
                break;
            }
            pools.push_back(std::move(pool));
        }
        if constexpr (ENABLE_PUSHOUT_HELPER) {
            if (helper_survey) {
                int evidenced_groups = 0;
                vector<PushOutHelperChoice> helpers = choose_pushout_helpers(
                    *helper_survey, target, groups, arrival_id, r_milli,
                    direct_gain_threshold,
                    PUSHOUT_HELPER_CHOICE_LIMIT_PER_TARGET,
                    evidenced_groups);
                diagnostics.pushout_helper_evidenced_groups += evidenced_groups;
                helper_raw_evidence |= evidenced_groups > 0;
                diagnostics.pushout_helper_shortlisted_choices += helpers.size();
                for (PushOutHelperChoice &helper : helpers) {
                    ll adjusted_direct_gain =
                        target.immediate_improvement - helper.movement_cost;
                    helper_seeds.push_back({
                        target_index,
                        std::move(helper),
                        adjusted_direct_gain,
                    });
                }
            }
        }
        if (missing_destination) continue;

        optional<vector<int>> choices = repair_rescue_blockers(base_owner, groups, target.blockers, pools,
                                                                remaining_nodes, diagnostics);
        if (!choices) continue;

        TurnPlan plan;
        for (int index = 0; index < (int)target.blockers.size(); index++) {
            const RescueDestination &destination = pools[index][(*choices)[index]];
            plan.moves.push_back({target.blockers[index], destination.cells, destination.perimeter});
        }
        plan.arrival = target.cells;
        plan.arrival_perimeter = target.perimeter;
        ll compact_fee = round_payment(arrival.v, arrival.p, target.perimeter);
        plan.immediate_gain = compact_fee - target.movement_cost;

        vvi final_owner;
        ll fee_loss = 0;
        ll checked_movement_cost = 0;
        if (!validate_and_build_rescue_owner(plan, park, owner, groups, arrival_id, r_milli, final_owner,
                                             fee_loss, checked_movement_cost) ||
            fee_loss != 0 || checked_movement_cost != target.movement_cost ||
            plan.immediate_gain - baseline_score <= 0) {
            diagnostics.validation_failures++;
            continue;
        }

        ll direct_gain = plan.immediate_gain - baseline_score;
        int blocker_bucket = min((int)target.blockers.size(), 4) - 1;
        diagnostics.feasible_plans++;
        diagnostics.feasible_by_blocker_count[blocker_bucket]++;
        diagnostics.feasible_direct_gain += direct_gain;
        if (no_region_pushout) {
            diagnostics.pushout_feasible_by_blocker_count[blocker_bucket]++;
            chmax(diagnostics.pushout_maximum_blockers, (int)target.blockers.size());
        }
        candidates.push_back({std::move(plan), std::move(final_owner), target.blockers,
                              target.blockers, -1, compact_fee, direct_gain,
                              target.movement_cost});

        if (candidates.size() == 1) {
            if (!initialize_rollout_for_first_candidate()) return nullopt;
            if (stop_after_primary) break;
        }
    }

    // Preserve the blockers-only path as the protected first phase.  The
    // helper is tried only when that phase produced no complete plan, so it
    // can rescue a missing action but cannot displace an existing candidate.
    if constexpr (ENABLE_PUSHOUT_HELPER) {
        if (no_region_pushout && candidates.empty() && !stop_after_primary) {
            diagnostics.pushout_helper_considered_turns++;
            if (!helper_surveyed_turn) {
                diagnostics.pushout_helper_no_eligible_target_turns++;
            } else if (helper_seeds.empty()) {
                if (helper_raw_evidence) {
                    diagnostics.pushout_helper_economic_rejected_turns++;
                } else {
                    diagnostics.pushout_helper_no_evidence_turns++;
                }
            } else {
                diagnostics.pushout_helper_seeded_turns++;
                sort(helper_seeds.begin(), helper_seeds.end(),
                     [](const PushOutHelperSeed &lhs, const PushOutHelperSeed &rhs) {
                         if (lhs.adjusted_direct_gain != rhs.adjusted_direct_gain) {
                             return lhs.adjusted_direct_gain > rhs.adjusted_direct_gain;
                         }
                         if (lhs.helper.covered_blockers != rhs.helper.covered_blockers) {
                             return lhs.helper.covered_blockers > rhs.helper.covered_blockers;
                         }
                         if (lhs.helper.unlocked_regions != rhs.helper.unlocked_regions) {
                             return lhs.helper.unlocked_regions > rhs.helper.unlocked_regions;
                         }
                         if (lhs.target_index != rhs.target_index) {
                             return lhs.target_index < rhs.target_index;
                         }
                         return lhs.helper.id < rhs.helper.id;
                     });

                PushOutHelperPhaseScope helper_scope(diagnostics);
                int helper_remaining_nodes = PUSHOUT_HELPER_REPAIR_NODE_LIMIT;
                int helper_remaining_destination_anchors =
                    PUSHOUT_HELPER_DESTINATION_ANCHOR_GLOBAL_LIMIT;
                int helper_attempts = 0;
                for (const PushOutHelperSeed &seed : helper_seeds) {
                    if (helper_attempts == PUSHOUT_HELPER_REPAIR_LIMIT ||
                        helper_remaining_nodes == 0 ||
                        helper_remaining_destination_anchors == 0 ||
                        (int)candidates.size() == PUSHOUT_HELPER_FEASIBLE_LIMIT ||
                        stop_after_primary) {
                        break;
                    }
                    helper_attempts++;
                    diagnostics.repair_attempts++;
                    diagnostics.pushout_helper_attempts++;
                    if (seed.target_index < 0 ||
                        seed.target_index >= (int)targets.size() ||
                        seed.helper.id < 0 || seed.helper.id >= (int)groups.size()) {
                        diagnostics.validation_failures++;
                        diagnostics.pushout_helper_validation_failures++;
                        continue;
                    }
                    const RescueTarget &target = targets[seed.target_index];
                    if (target.blockers.empty() ||
                        (int)target.blockers.size() > PUSHOUT_HELPER_MAX_BLOCKERS) {
                        diagnostics.validation_failures++;
                        diagnostics.pushout_helper_validation_failures++;
                        continue;
                    }

                    vector<int> movers = target.blockers;
                    movers.push_back(seed.helper.id);
                    sort(movers.begin(), movers.end());
                    bool valid_movers =
                        adjacent_find(movers.begin(), movers.end()) == movers.end() &&
                        seed.helper.id != arrival_id && groups[seed.helper.id].active;
                    if (!valid_movers) {
                        diagnostics.validation_failures++;
                        diagnostics.pushout_helper_validation_failures++;
                        continue;
                    }
                    chmax(diagnostics.pushout_helper_maximum_movers,
                          (int)movers.size());
                    diagnostics.pushout_helper_selected_covered_blockers +=
                        seed.helper.covered_blockers;
                    diagnostics.pushout_helper_selected_unlocked_regions +=
                        seed.helper.unlocked_regions;
                    diagnostics.pushout_helper_selected_overlap_cells +=
                        seed.helper.overlap_cells;
                    diagnostics.pushout_helper_selected_movement_cost +=
                        seed.helper.movement_cost;
                    diagnostics.pushout_helper_selected_departure_distance +=
                        seed.helper.departure_distance;
                    diagnostics.pushout_helper_selected_adjusted_gain +=
                        seed.adjusted_direct_gain;

                    ll full_movement_cost = 0;
                    for (int id : movers) full_movement_cost += move_cost(groups[id].v, r_milli);
                    ll compact_fee = round_payment(arrival.v, arrival.p, target.perimeter);
                    ll direct_gain = compact_fee - full_movement_cost - baseline_score;
                    if ((long double)direct_gain <= direct_gain_threshold) {
                        // The same strict economic gate was already used by
                        // the shortlist.  Rechecking the exact mover set makes
                        // any future refactor fail closed.
                        diagnostics.validation_failures++;
                        diagnostics.pushout_helper_validation_failures++;
                        continue;
                    }

                    vvi base_owner = owner;
                    vector<char> cleared_mask(park.size() * park.size(), false);
                    for (int id : movers) {
                        for (auto [x, y] : groups[id].cells) {
                            cleared_mask[x * park.size() + y] = true;
                        }
                        clear_cells(base_owner, groups[id].cells);
                    }
                    bool target_legal = true;
                    for (auto [x, y] : target.cells) {
                        if (park[x][y] != '.' || base_owner[x][y] != -1) {
                            target_legal = false;
                            break;
                        }
                        base_owner[x][y] = arrival_id;
                    }
                    if (!target_legal) {
                        diagnostics.validation_failures++;
                        diagnostics.pushout_helper_validation_failures++;
                        continue;
                    }

                    vector<char> preferred_mask;
                    if constexpr (ENABLE_WIDE_PUSHOUT_HELPER) {
                        preferred_mask.assign(park.size() * park.size(), false);
                        for (auto [x, y] : preferred_destination_cells) {
                            preferred_mask[x * park.size() + y] = true;
                        }
                    }
                    auto force_causal_witness =
                        [&](int mover_id, vector<RescueDestination> &pool) {
                            if constexpr (!ENABLE_WIDE_PUSHOUT_HELPER) return;
                            if (!binary_search(target.blockers.begin(),
                                               target.blockers.end(), mover_id)) {
                                return;
                            }
                            auto witness_it = find_if(
                                seed.helper.witnesses.begin(),
                                seed.helper.witnesses.end(),
                                [&](const PushOutHelperWitness &witness) {
                                    return witness.blocker_id == mover_id;
                                });
                            if (witness_it == seed.helper.witnesses.end()) return;
                            const PushOutHelperWitness &witness = *witness_it;
                            if ((int)witness.cells.size() != groups[mover_id].p ||
                                same_region(witness.cells, groups[mover_id].cells)) {
                                return;
                            }
                            for (auto [x, y] : witness.cells) {
                                if (park[x][y] != '.' || base_owner[x][y] != -1) {
                                    return;
                                }
                            }
                            ll old_fee = round_payment(
                                groups[mover_id].v, groups[mover_id].p,
                                groups[mover_id].max_perimeter);
                            ll new_fee = round_payment(
                                groups[mover_id].v, groups[mover_id].p,
                                max(groups[mover_id].max_perimeter,
                                    witness.perimeter));
                            if (old_fee != new_fee) return;
                            for (int index = 0; index < (int)pool.size(); index++) {
                                if (!same_region(pool[index].cells, witness.cells)) continue;
                                rotate(pool.begin(), pool.begin() + index,
                                       pool.begin() + index + 1);
                                diagnostics.pushout_helper_forced_witness_destinations++;
                                return;
                            }

                            vector<char> own_old(park.size() * park.size(), false);
                            for (auto [x, y] : groups[mover_id].cells) {
                                own_old[x * park.size() + y] = true;
                            }
                            int fallback_overlap = 0;
                            int cleared_overlap = 0;
                            int foreign_cleared_overlap = 0;
                            int min_x = park.size(), min_y = park.size();
                            int max_x = -1, max_y = -1;
                            for (auto [x, y] : witness.cells) {
                                int cell = x * park.size() + y;
                                fallback_overlap += preferred_mask[cell];
                                cleared_overlap += cleared_mask[cell];
                                foreign_cleared_overlap +=
                                    cleared_mask[cell] && !own_old[cell];
                                chmin(min_x, x);
                                chmin(min_y, y);
                                chmax(max_x, x);
                                chmax(max_y, y);
                            }
                            int height = max_x - min_x + 1;
                            int width = max_y - min_y + 1;
                            int lower_half = 2 * min_x + height >= (int)park.size();
                            int right_half = 2 * min_y + width >= (int)park.size();
                            int sector_x = min(
                                2, 3 * (2 * min_x + height) /
                                       (2 * (int)park.size()));
                            int sector_y = min(
                                2, 3 * (2 * min_y + width) /
                                       (2 * (int)park.size()));
                            BoardMask mask = make_board_mask(witness.cells, park.size());
                            long double temporal_cost =
                                rescue_destination_temporal_cost(
                                    witness.cells, mask, park, base_owner, groups,
                                    mover_id, arrival_id, current_s, theta);
                            RescueDestination destination{
                                witness.cells,
                                mask,
                                witness.perimeter,
                                fallback_overlap,
                                cleared_overlap,
                                foreign_cleared_overlap,
                                2 * lower_half + right_half,
                                3 * sector_x + sector_y,
                                temporal_cost,
                                -1,
                            };
                            pool.insert(pool.begin(), std::move(destination));
                            if ((int)pool.size() > PUSHOUT_HELPER_DESTINATION_LIMIT) {
                                pool.pop_back();
                            }
                            diagnostics.destination_candidates++;
                            diagnostics.pushout_helper_forced_witness_destinations++;
                            diagnostics.pushout_helper_foreign_destination_candidates +=
                                foreign_cleared_overlap > 0;
                            diagnostics.pushout_helper_retained_foreign_destinations +=
                                foreign_cleared_overlap > 0;
                        };

                    vector<vector<RescueDestination>> pools;
                    pools.reserve(movers.size());
                    bool missing_destination = false;
                    for (int id : movers) {
                        vector<RescueDestination> pool = make_rescue_destinations(
                            park, base_owner, groups, id, arrival_id, current_s, theta,
                            preferred_destination_cells, cleared_mask, all_shapes,
                            helper_remaining_destination_anchors,
                            PUSHOUT_HELPER_DESTINATION_ANCHOR_LIMIT,
                            PUSHOUT_HELPER_DESTINATION_LEGAL_LIMIT,
                            PUSHOUT_HELPER_DESTINATION_LIMIT, diagnostics,
                            nullptr, ENABLE_WIDE_PUSHOUT_HELPER);
                        force_causal_witness(id, pool);
                        if (pool.empty()) {
                            missing_destination = true;
                            if (id == seed.helper.id) {
                                diagnostics.pushout_helper_missing_helper_destination++;
                            } else {
                                diagnostics.pushout_helper_missing_blocker_destination++;
                            }
                            break;
                        }
                        pools.push_back(std::move(pool));
                    }
                    if (missing_destination) {
                        diagnostics.pushout_helper_missing_destination++;
                        continue;
                    }

                    optional<vector<int>> choices = repair_rescue_blockers(
                        base_owner, groups, movers, pools, helper_remaining_nodes,
                        diagnostics);
                    if (!choices) {
                        diagnostics.pushout_helper_repair_failures++;
                        continue;
                    }

                    TurnPlan plan;
                    for (int index = 0; index < (int)movers.size(); index++) {
                        const RescueDestination &destination =
                            pools[index][(*choices)[index]];
                        plan.moves.push_back(
                            {movers[index], destination.cells, destination.perimeter});
                    }
                    plan.arrival = target.cells;
                    plan.arrival_perimeter = target.perimeter;
                    plan.immediate_gain = compact_fee - full_movement_cost;

                    vvi final_owner;
                    ll fee_loss = 0;
                    ll checked_movement_cost = 0;
                    if (!validate_and_build_rescue_owner(
                            plan, park, owner, groups, arrival_id, r_milli,
                            final_owner, fee_loss, checked_movement_cost) ||
                        fee_loss != 0 || checked_movement_cost != full_movement_cost ||
                        plan.immediate_gain - baseline_score != direct_gain ||
                        (long double)direct_gain <= direct_gain_threshold) {
                        diagnostics.validation_failures++;
                        diagnostics.pushout_helper_validation_failures++;
                        continue;
                    }

                    bool duplicate = false;
                    for (const PreparedRescueCandidate &candidate : candidates) {
                        if (candidate.final_owner == final_owner) {
                            duplicate = true;
                            break;
                        }
                    }
                    if (duplicate) {
                        diagnostics.pushout_helper_duplicate_plans++;
                        continue;
                    }

                    int blocker_bucket = min((int)target.blockers.size(), 4) - 1;
                    auto [blocker_uses_helper, helper_uses_blocker] =
                        classify_pushout_helper_exchange(
                            plan, groups, target.blockers, seed.helper.id,
                            park.size());
                    diagnostics.feasible_plans++;
                    diagnostics.feasible_by_blocker_count[blocker_bucket]++;
                    diagnostics.feasible_direct_gain += direct_gain;
                    diagnostics.pushout_feasible_by_blocker_count[blocker_bucket]++;
                    chmax(diagnostics.pushout_maximum_blockers,
                          (int)target.blockers.size());
                    diagnostics.pushout_helper_feasible_plans++;
                    diagnostics.pushout_helper_feasible_by_blocker_count[
                        target.blockers.size() - 1]++;
                    diagnostics.pushout_helper_feasible_blocker_uses_helper +=
                        blocker_uses_helper;
                    diagnostics.pushout_helper_feasible_helper_uses_blocker +=
                        helper_uses_blocker;
                    diagnostics.pushout_helper_feasible_bidirectional_cross_use +=
                        blocker_uses_helper && helper_uses_blocker;
                    candidates.push_back({
                        std::move(plan), std::move(final_owner), target.blockers,
                        std::move(movers), seed.helper.id, compact_fee, direct_gain,
                        full_movement_cost, blocker_uses_helper,
                        helper_uses_blocker,
                    });

                    if (candidates.size() == 1 &&
                        !initialize_rollout_for_first_candidate()) {
                        return nullopt;
                    }
                }
                if ((int)candidates.size() == PUSHOUT_HELPER_FEASIBLE_LIMIT &&
                    helper_attempts < (int)helper_seeds.size()) {
                    diagnostics.pushout_helper_feasible_limit_exhausted++;
                }
                diagnostics.pushout_helper_two_feasible_turns +=
                    candidates.size() >= 2;
                if (helper_remaining_nodes == 0) diagnostics.node_limit_exhausted++;
            }
        }
    }

    if (candidates.empty()) {
        if (attempted_targets == target_repair_limit && (int)targets.size() > attempted_targets) {
            diagnostics.target_limit_exhausted++;
        }
        if (remaining_nodes == 0) diagnostics.node_limit_exhausted++;
        diagnostics.no_repair++;
        if (no_region_pushout) diagnostics.pushout_no_repair++;
        return nullopt;
    }
    if (!stop_after_primary && (int)candidates.size() < RESCUE_ROLLOUT_CANDIDATE_LIMIT &&
        attempted_targets == target_repair_limit && (int)targets.size() > attempted_targets) {
        diagnostics.target_limit_exhausted++;
    }
    if (remaining_nodes == 0) diagnostics.node_limit_exhausted++;

    int helper_candidate_count = count_if(
        candidates.begin(), candidates.end(),
        [](const PreparedRescueCandidate &candidate) {
            return candidate.helper_id != -1;
        });
    RootActionKind selected_kind = RootActionKind::Rescue;
    int selected_candidate = 0;
    int selected_alternative = -1;
    if (rollout_ready) {
        root_screen_evaluated = true;
        diagnostics.rollout_turns++;
        diagnostics.rollout_candidates_compared += candidates.size();
        vector<int> available_alternatives;
        if constexpr (!ROOT_PROTECTED_ONLY) {
            if (mode == RescueMode::CompactAccepted) {
                for (int index = 0; index < (int)normal_alternatives.size(); index++) {
                    if (!same_region(normal_alternatives[index].cells, *baseline.cells)) {
                        available_alternatives.push_back(index);
                    }
                }
            }
        }
        if (!available_alternatives.empty()) diagnostics.root_alternative_available_turns++;
        diagnostics.root_alternatives_compared += available_alternatives.size();
        int root_action_count = 1 + (int)candidates.size() + (int)available_alternatives.size();
        diagnostics.root_actions_compared += root_action_count;
        diagnostics.root_turns_by_action_count[root_action_count]++;
        if (candidates.size() == 1) {
            diagnostics.rollout_one_candidate_turns++;
        } else {
            diagnostics.rollout_two_candidate_turns++;
            if (candidates[0].blockers == candidates[1].blockers) diagnostics.rollout_same_blocker_sets++;
            vector<char> first_target(park.size() * park.size(), false);
            for (auto [x, y] : *candidates[0].plan.arrival) first_target[x * park.size() + y] = true;
            for (auto [x, y] : *candidates[1].plan.arrival) {
                diagnostics.rollout_candidate_overlap_cells += first_target[x * park.size() + y];
            }
        }

        vvi baseline_final_owner = owner;
        if (baseline.cells) place_cells(baseline_final_owner, *baseline.cells, arrival_id);
        TurnPlan baseline_plan = make_arrival_plan(baseline);
        array<RescueRolloutOutcome, RESCUE_ROLLOUT_SCENARIO_COUNT> baseline_outcomes;
        for (int scenario = 0; scenario < RESCUE_ROLLOUT_SCENARIO_COUNT; scenario++) {
            baseline_outcomes[scenario] = evaluate_rescue_rollout_branch(
                park, baseline_final_owner, groups, arrival_id, baseline_plan,
                rollout_scenarios.arrivals[scenario], grass_cells, density_model,
                sampled_dlp_model, compact_shapes);
            diagnostics.rollout_policy_steps += rollout_scenarios.arrivals[scenario].size();
            diagnostics.rollout_baseline_acceptances += baseline_outcomes[scenario].acceptances;
        }

        struct CandidateRolloutEvaluation {
            array<ll, RESCUE_ROLLOUT_SCENARIO_COUNT> future_delta{};
            ll margin_twice = 0;
        };
        vector<CandidateRolloutEvaluation> evaluations(candidates.size());
        for (int candidate_index = 0; candidate_index < (int)candidates.size(); candidate_index++) {
            const PreparedRescueCandidate &candidate = candidates[candidate_index];
            CandidateRolloutEvaluation &evaluation = evaluations[candidate_index];
            for (int scenario = 0; scenario < RESCUE_ROLLOUT_SCENARIO_COUNT; scenario++) {
                RescueRolloutOutcome rescue_outcome = evaluate_rescue_rollout_branch(
                    park, candidate.final_owner, groups, arrival_id, candidate.plan,
                    rollout_scenarios.arrivals[scenario], grass_cells, density_model,
                    sampled_dlp_model, compact_shapes);
                diagnostics.rollout_policy_steps += rollout_scenarios.arrivals[scenario].size();
                diagnostics.rollout_rescue_acceptances += rescue_outcome.acceptances;
                evaluation.future_delta[scenario] = rescue_outcome.fee - baseline_outcomes[scenario].fee;
            }
            evaluation.margin_twice = 2 * candidate.direct_gain + evaluation.future_delta[0] +
                                      evaluation.future_delta[1];
            diagnostics.rollout_slot_scenario_0_future_delta[candidate_index] +=
                evaluation.future_delta[0];
            diagnostics.rollout_slot_scenario_1_future_delta[candidate_index] +=
                evaluation.future_delta[1];
            diagnostics.rollout_slot_margin[candidate_index] += 0.5L * evaluation.margin_twice;
            bool first_accepts = candidate.direct_gain + evaluation.future_delta[0] > 0;
            bool second_accepts = candidate.direct_gain + evaluation.future_delta[1] > 0;
            if (first_accepts != second_accepts) {
                if (candidate_index == 0) {
                    diagnostics.rollout_candidate_0_disagreements++;
                } else {
                    diagnostics.rollout_candidate_1_disagreements++;
                }
            }
            if (evaluation.margin_twice > 0) {
                diagnostics.rollout_positive_candidates++;
            } else {
                diagnostics.rollout_nonpositive_candidates++;
            }
        }

        int best_rescue = 0;
        for (int candidate_index = 1; candidate_index < (int)candidates.size(); candidate_index++) {
            if (evaluations[candidate_index].margin_twice > evaluations[best_rescue].margin_twice) {
                best_rescue = candidate_index;
            }
        }
        const PreparedRescueCandidate &best_candidate = candidates[best_rescue];
        const CandidateRolloutEvaluation &best_evaluation = evaluations[best_rescue];
        diagnostics.rollout_scenario_0_future_delta += best_evaluation.future_delta[0];
        diagnostics.rollout_scenario_1_future_delta += best_evaluation.future_delta[1];
        long double future_mean =
            0.5L * (best_evaluation.future_delta[0] + best_evaluation.future_delta[1]);
        long double rollout_margin = 0.5L * best_evaluation.margin_twice;
        if (best_candidate.helper_id != -1) {
            diagnostics.pushout_helper_scenario_0_future_delta +=
                best_evaluation.future_delta[0];
            diagnostics.pushout_helper_scenario_1_future_delta +=
                best_evaluation.future_delta[1];
            diagnostics.pushout_helper_screen_margin += rollout_margin;
        }
        bool first_accepts = best_candidate.direct_gain + best_evaluation.future_delta[0] > 0;
        bool second_accepts = best_candidate.direct_gain + best_evaluation.future_delta[1] > 0;
        if (first_accepts != second_accepts) diagnostics.rollout_scenario_disagreements++;
        if (no_region_pushout) {
            diagnostics.pushout_scenario_0_future_delta += best_evaluation.future_delta[0];
            diagnostics.pushout_scenario_1_future_delta += best_evaluation.future_delta[1];
            diagnostics.pushout_screen_margin += rollout_margin;
        }

        if (candidates.size() == 2) {
            ll width_one_margin = max(0LL, evaluations[0].margin_twice);
            ll width_two_margin = max(width_one_margin, evaluations[1].margin_twice);
            diagnostics.rollout_width_predicted_gain += 0.5L * (width_two_margin - width_one_margin);
        }

        ll best_root_margin_twice = 0;
        selected_kind = RootActionKind::Baseline;
        // Establish the exact v3 winner first.  Every new action must beat
        // this protected branch both on the cheap screen and on an independent
        // posterior-predictive holdout.
        for (int candidate_index = 0; candidate_index < (int)candidates.size(); candidate_index++) {
            if (evaluations[candidate_index].margin_twice > best_root_margin_twice) {
                best_root_margin_twice = evaluations[candidate_index].margin_twice;
                selected_kind = RootActionKind::Rescue;
                selected_candidate = candidate_index;
            }
        }
        RootActionKind protected_kind = selected_kind;
        int protected_candidate = selected_candidate;
        ll protected_margin_twice = best_root_margin_twice;

        vector<TurnPlan> alternative_plans;
        vector<ArrivalDecision> alternative_decisions;
        vector<vvi> alternative_owners;
        alternative_plans.reserve(available_alternatives.size());
        alternative_decisions.reserve(available_alternatives.size());
        alternative_owners.reserve(available_alternatives.size());
        if constexpr (!ROOT_PROTECTED_ONLY) {
            for (int source_index : available_alternatives) {
                const NormalPlacementChoice &choice = normal_alternatives[source_index];
                ArrivalDecision alternative_decision = baseline;
                alternative_decision.cells = choice.cells;
                alternative_decision.perimeter = choice.perimeter;
                alternative_decision.fee = round_payment(arrival.v, arrival.p, choice.perimeter);
                replace_selected_placement_success(alternative_decision.diagnostics, choice.source);
                TurnPlan alternative_plan = make_arrival_plan(alternative_decision);
                vvi alternative_owner = owner;
                place_cells(alternative_owner, *alternative_decision.cells, arrival_id);

                CandidateRolloutEvaluation alternative_evaluation;
                ll alternative_direct_gain = alternative_decision.fee - baseline.fee;
                for (int scenario = 0; scenario < RESCUE_ROLLOUT_SCENARIO_COUNT; scenario++) {
                    RescueRolloutOutcome alternative_outcome = evaluate_rescue_rollout_branch(
                        park, alternative_owner, groups, arrival_id, alternative_plan,
                        rollout_scenarios.arrivals[scenario], grass_cells, density_model,
                        sampled_dlp_model, compact_shapes);
                    diagnostics.rollout_policy_steps += rollout_scenarios.arrivals[scenario].size();
                    diagnostics.root_alternative_acceptances += alternative_outcome.acceptances;
                    alternative_evaluation.future_delta[scenario] =
                        alternative_outcome.fee - baseline_outcomes[scenario].fee;
                }
                alternative_evaluation.margin_twice =
                    2 * alternative_direct_gain + alternative_evaluation.future_delta[0] +
                    alternative_evaluation.future_delta[1];
                diagnostics.root_alternative_direct_gain += alternative_direct_gain;
                diagnostics.root_alternative_scenario_0_future_delta +=
                    alternative_evaluation.future_delta[0];
                diagnostics.root_alternative_scenario_1_future_delta +=
                    alternative_evaluation.future_delta[1];
                long double alternative_future_mean =
                    0.5L * (alternative_evaluation.future_delta[0] + alternative_evaluation.future_delta[1]);
                diagnostics.root_alternative_future_mean += alternative_future_mean;
                diagnostics.root_alternative_margin += 0.5L * alternative_evaluation.margin_twice;
                bool alternative_first_accepts =
                    alternative_direct_gain + alternative_evaluation.future_delta[0] > 0;
                bool alternative_second_accepts =
                    alternative_direct_gain + alternative_evaluation.future_delta[1] > 0;
                if (alternative_first_accepts != alternative_second_accepts) {
                    diagnostics.root_alternative_disagreements++;
                }

                int alternative_index = alternative_plans.size();
                alternative_plans.push_back(std::move(alternative_plan));
                alternative_decisions.push_back(std::move(alternative_decision));
                alternative_owners.push_back(std::move(alternative_owner));
                if (alternative_evaluation.margin_twice > best_root_margin_twice) {
                    best_root_margin_twice = alternative_evaluation.margin_twice;
                    selected_kind = RootActionKind::NormalAlternative;
                    selected_alternative = alternative_index;
                }
            }

            long double screen_gain =
                0.5L * (best_root_margin_twice - protected_margin_twice);
            diagnostics.root_expanded_predicted_gain += screen_gain;
            if (selected_kind == RootActionKind::NormalAlternative) {
                diagnostics.root_screen_v3_overrides++;
                diagnostics.root_screen_selected_alternative++;
                diagnostics.root_confirmation_screen_gain += screen_gain;
                RootBranchView protected_branch;
                if (protected_kind == RootActionKind::Baseline) {
                    protected_branch = RootBranchView{&baseline_plan, &baseline_final_owner, 0};
                } else {
                    const PreparedRescueCandidate &candidate = candidates[protected_candidate];
                    protected_branch = RootBranchView{&candidate.plan, &candidate.final_owner,
                                                      candidate.direct_gain};
                }
                RootBranchView challenger_branch{
                    &alternative_plans[selected_alternative], &alternative_owners[selected_alternative],
                    alternative_decisions[selected_alternative].fee - baseline.fee};
                bool confirmed = confirm_root_override(
                    park, groups, arrival_id, current_s, remaining_groups, theta, theta_estimator,
                    grass_cells, density_model, sampled_dlp_model, compact_shapes,
                    protected_branch, challenger_branch, confirmations_used, diagnostics);
                if (!confirmed) {
                    selected_kind = protected_kind;
                    selected_candidate = protected_candidate;
                    selected_alternative = -1;
                } else {
                    diagnostics.root_v3_winner_overridden++;
                }
            }
        }

        int positive_count = 0;
        for (const CandidateRolloutEvaluation &evaluation : evaluations) {
            positive_count += evaluation.margin_twice > 0;
        }
        diagnostics.rollout_unselected_positive_candidates +=
            positive_count - (selected_kind == RootActionKind::Rescue);

        if (selected_kind != RootActionKind::Rescue) {
            diagnostics.rollout_rescue_not_selected++;
            diagnostics.rollout_not_selected_direct_gain += best_candidate.direct_gain;
            diagnostics.rollout_not_selected_future_mean += future_mean;
            diagnostics.rollout_not_selected_margin += rollout_margin;
            if (selected_kind == RootActionKind::Baseline) {
                diagnostics.root_selected_primary++;
                if (no_region_pushout) {
                    diagnostics.pushout_screen_rejected++;
                    diagnostics.pushout_helper_screen_rejected +=
                        helper_candidate_count;
                }
                return nullopt;
            }
            assert(selected_kind == RootActionKind::NormalAlternative);
            diagnostics.pushout_helper_screen_rejected += helper_candidate_count;
            diagnostics.root_selected_alternative++;
            diagnostics.root_selected_alternative_rank[selected_alternative]++;
            return RootActionResult{std::move(alternative_plans[selected_alternative]),
                                    std::move(alternative_decisions[selected_alternative])};
        }
        diagnostics.rollout_adopted++;
        if (selected_candidate == 0) {
            diagnostics.rollout_selected_candidate_0++;
        } else {
            diagnostics.rollout_selected_candidate_1++;
        }
        diagnostics.rollout_adopted_direct_gain += best_candidate.direct_gain;
        diagnostics.rollout_adopted_future_mean += future_mean;
        diagnostics.rollout_adopted_margin += rollout_margin;
    }

    if (no_region_pushout) {
        diagnostics.pushout_helper_screen_rejected +=
            helper_candidate_count -
            (candidates[selected_candidate].helper_id != -1);
    }
    PreparedRescueCandidate &chosen = candidates[selected_candidate];
    int blocker_bucket = min((int)chosen.blockers.size(), 4) - 1;
    ArrivalDecision selected = baseline;
    selected.status = ArrivalStatus::Accepted;
    selected.cells = *chosen.plan.arrival;
    selected.perimeter = chosen.plan.arrival_perimeter;
    selected.fee = chosen.compact_fee;
    // Whether the protected action was connected growth or Reject, the
    // selected arrival is now a minimum-perimeter template.
    replace_selected_placement_success(selected.diagnostics, PlacementSource::MinimumTemplate);
    diagnostics.successes++;
    diagnostics.successes_by_blocker_count[blocker_bucket]++;
    diagnostics.moved_groups += chosen.movers.size();
    diagnostics.arrival_fee_gain += chosen.compact_fee - baseline_score;
    diagnostics.movement_cost += chosen.movement_cost;
    diagnostics.immediate_gain += chosen.direct_gain;
    if (no_region_pushout) {
        diagnostics.pushout_adopted++;
        diagnostics.pushout_adopted_by_blocker_count[blocker_bucket]++;
        diagnostics.pushout_moved_groups += chosen.movers.size();
        for (int id : chosen.movers) diagnostics.pushout_moved_cells += groups[id].p;
        diagnostics.pushout_arrival_fee += chosen.compact_fee;
        diagnostics.pushout_movement_cost += chosen.movement_cost;
        diagnostics.pushout_direct_gain += chosen.direct_gain;
        if (chosen.helper_id != -1) {
            diagnostics.pushout_helper_adopted++;
            diagnostics.pushout_helper_adopted_by_blocker_count[
                chosen.blockers.size() - 1]++;
            diagnostics.pushout_helper_adopted_blocker_uses_helper +=
                chosen.blocker_uses_helper_region;
            diagnostics.pushout_helper_adopted_helper_uses_blocker +=
                chosen.helper_uses_blocker_region;
            diagnostics.pushout_helper_adopted_bidirectional_cross_use +=
                chosen.blocker_uses_helper_region &&
                chosen.helper_uses_blocker_region;
            diagnostics.pushout_helper_adopted_moved_groups += chosen.movers.size();
            for (int id : chosen.movers) {
                diagnostics.pushout_helper_adopted_moved_cells += groups[id].p;
            }
            diagnostics.pushout_helper_adopted_arrival_fee += chosen.compact_fee;
            diagnostics.pushout_helper_adopted_movement_cost += chosen.movement_cost;
            diagnostics.pushout_helper_adopted_direct_gain += chosen.direct_gain;
        }
    }
    return RootActionResult{std::move(chosen.plan), std::move(selected)};
}

optional<RootActionResult> choose_normal_root_action(
    const vs &park, const vvi &owner, const vector<GroupState> &groups, int arrival_id,
    ll current_s, int remaining_groups, long double theta, const ThetaEstimator &theta_estimator,
    const DensityModel &density_model, SampledDlpShadowModel &sampled_dlp_model,
    int grass_cells, const ArrivalDecision &baseline,
    const vector<NormalPlacementChoice> &normal_alternatives,
    const vector<vector<Shape>> &compact_shapes, int &confirmations_used,
    RescueDiagnostics &diagnostics) {
    if (baseline.status != ArrivalStatus::Accepted || !baseline.cells ||
        normal_alternatives.empty() || remaining_groups <= 0) {
        return nullopt;
    }

    RescueRolloutScenarios scenarios = make_rescue_rollout_scenarios(
        groups, arrival_id, current_s, remaining_groups, theta, theta_estimator);
    int expected_length = min(ROOT_SCREEN_ROLLOUT_LENGTH, remaining_groups);
    bool generation_ok = scenarios.complete &&
                         (int)scenarios.arrivals.size() == ROOT_SCREEN_SCENARIO_COUNT;
    if (generation_ok) {
        for (const auto &scenario : scenarios.arrivals) {
            if ((int)scenario.size() != expected_length) {
                generation_ok = false;
                break;
            }
        }
    }
    if (!generation_ok) {
        diagnostics.normal_root_generation_failures++;
        return nullopt;
    }

    const GroupState &arrival = groups[arrival_id];
    vector<int> available_alternatives;
    for (int index = 0; index < (int)normal_alternatives.size(); index++) {
        if (!same_region(normal_alternatives[index].cells, *baseline.cells)) {
            available_alternatives.push_back(index);
        }
    }
    if (available_alternatives.empty()) return nullopt;

    diagnostics.normal_root_rollout_turns++;
    diagnostics.normal_root_alternatives_compared += available_alternatives.size();
    int action_count = 1 + (int)available_alternatives.size();
    diagnostics.normal_root_actions_compared += action_count;
    diagnostics.normal_root_turns_by_action_count[action_count]++;

    vvi baseline_owner = owner;
    place_cells(baseline_owner, *baseline.cells, arrival_id);
    TurnPlan baseline_plan = make_arrival_plan(baseline);
    array<RescueRolloutOutcome, ROOT_SCREEN_SCENARIO_COUNT> baseline_outcomes;
    for (int scenario = 0; scenario < ROOT_SCREEN_SCENARIO_COUNT; scenario++) {
        baseline_outcomes[scenario] = evaluate_rescue_rollout_branch(
            park, baseline_owner, groups, arrival_id, baseline_plan,
            scenarios.arrivals[scenario], grass_cells, density_model,
            sampled_dlp_model, compact_shapes);
        diagnostics.normal_root_policy_steps += scenarios.arrivals[scenario].size();
    }

    struct NormalEvaluation {
        array<ll, ROOT_SCREEN_SCENARIO_COUNT> future_delta{};
        ll margin_twice = 0;
    };
    vector<TurnPlan> alternative_plans;
    vector<ArrivalDecision> alternative_decisions;
    vector<vvi> alternative_owners;
    alternative_plans.reserve(available_alternatives.size());
    alternative_decisions.reserve(available_alternatives.size());
    alternative_owners.reserve(available_alternatives.size());

    RootActionKind selected_kind = RootActionKind::Baseline;
    int selected_alternative = -1;
    ll best_margin_twice = 0;
    for (int source_index : available_alternatives) {
        const NormalPlacementChoice &choice = normal_alternatives[source_index];
        ArrivalDecision decision = baseline;
        decision.cells = choice.cells;
        decision.perimeter = choice.perimeter;
        decision.fee = round_payment(arrival.v, arrival.p, choice.perimeter);
        replace_selected_placement_success(decision.diagnostics, choice.source);
        TurnPlan plan = make_arrival_plan(decision);
        vvi final_owner = owner;
        place_cells(final_owner, *decision.cells, arrival_id);

        NormalEvaluation evaluation;
        ll direct_gain = decision.fee - baseline.fee;
        for (int scenario = 0; scenario < ROOT_SCREEN_SCENARIO_COUNT; scenario++) {
            RescueRolloutOutcome outcome = evaluate_rescue_rollout_branch(
                park, final_owner, groups, arrival_id, plan, scenarios.arrivals[scenario],
                grass_cells, density_model, sampled_dlp_model, compact_shapes);
            diagnostics.normal_root_policy_steps += scenarios.arrivals[scenario].size();
            evaluation.future_delta[scenario] = outcome.fee - baseline_outcomes[scenario].fee;
        }
        evaluation.margin_twice =
            ROOT_SCREEN_SCENARIO_COUNT * direct_gain +
            accumulate(evaluation.future_delta.begin(), evaluation.future_delta.end(), 0LL);

        int alternative_index = alternative_plans.size();
        alternative_plans.push_back(std::move(plan));
        alternative_decisions.push_back(std::move(decision));
        alternative_owners.push_back(std::move(final_owner));
        if (evaluation.margin_twice > best_margin_twice) {
            best_margin_twice = evaluation.margin_twice;
            selected_kind = RootActionKind::NormalAlternative;
            selected_alternative = alternative_index;
        }
    }

    if (selected_kind == RootActionKind::Baseline) {
        diagnostics.normal_root_selected_primary++;
        return nullopt;
    }
    assert(selected_kind == RootActionKind::NormalAlternative);
    diagnostics.normal_root_screen_overrides++;
    diagnostics.normal_root_screen_selected_alternative++;
    long double screen_gain =
        (long double)best_margin_twice / ROOT_SCREEN_SCENARIO_COUNT;
    diagnostics.root_confirmation_screen_gain += screen_gain;
    RootBranchView baseline_branch{&baseline_plan, &baseline_owner, 0};
    RootBranchView challenger_branch{
        &alternative_plans[selected_alternative], &alternative_owners[selected_alternative],
        alternative_decisions[selected_alternative].fee - baseline.fee};
    bool confirmed = confirm_root_override(
        park, groups, arrival_id, current_s, remaining_groups, theta, theta_estimator,
        grass_cells, density_model, sampled_dlp_model, compact_shapes,
        baseline_branch, challenger_branch, confirmations_used, diagnostics);
    if (!confirmed) {
        diagnostics.normal_root_selected_primary++;
        return nullopt;
    }

    diagnostics.normal_root_selected_alternative++;
    diagnostics.normal_root_selected_alternative_rank[selected_alternative]++;
    return RootActionResult{std::move(alternative_plans[selected_alternative]),
                            std::move(alternative_decisions[selected_alternative])};
}

// ---------------------------------------------------------------------------
// Deadline-layer canonical rebuilding
// ---------------------------------------------------------------------------

bool deadline_mask_has(const BoardMask &mask, int index) {
    return (mask[index >> 6] >> (index & 63)) & 1ULL;
}

void deadline_mask_set(BoardMask &mask, int index) {
    mask[index >> 6] |= 1ULL << (index & 63);
}

void deadline_mask_reset(BoardMask &mask, int index) {
    mask[index >> 6] &= ~(1ULL << (index & 63));
}

int deadline_mask_count(const BoardMask &mask) {
    int result = 0;
    for (uint64_t word : mask) result += __builtin_popcountll(word);
    return result;
}

bool deadline_mask_subset(const BoardMask &part, const BoardMask &whole) {
    for (int word = 0; word < BOARD_MASK_WORDS; word++) {
        if (part[word] & ~whole[word]) return false;
    }
    return true;
}

BoardMask deadline_mask_difference(const BoardMask &whole, const BoardMask &part) {
    BoardMask result{};
    for (int word = 0; word < BOARD_MASK_WORDS; word++) {
        result[word] = whole[word] & ~part[word];
    }
    return result;
}

BoardMask deadline_mask_union(const BoardMask &lhs, const BoardMask &rhs) {
    BoardMask result = lhs;
    merge_mask(result, rhs);
    return result;
}

uint64_t deadline_mask_hash(const BoardMask &mask) {
    uint64_t result = 0x9e3779b97f4a7c15ULL;
    for (uint64_t word : mask) {
        word += 0x9e3779b97f4a7c15ULL;
        word = (word ^ (word >> 30)) * 0xbf58476d1ce4e5b9ULL;
        word = (word ^ (word >> 27)) * 0x94d049bb133111ebULL;
        word ^= word >> 31;
        result ^= word + 0x9e3779b97f4a7c15ULL + (result << 6) + (result >> 2);
    }
    return result;
}

vector<Cell> deadline_mask_cells(const BoardMask &mask, int n) {
    vector<Cell> cells;
    cells.reserve(deadline_mask_count(mask));
    for (int index = 0; index < n * n; index++) {
        if (deadline_mask_has(mask, index)) cells.emplace_back(index / n, index % n);
    }
    return cells;
}

struct DeadlineAtom {
    bool is_free = false;
    int group_id = -1;
    vector<Cell> cells;
    vector<int> adjacent;
};

struct DeadlineQuotientGraph {
    int n = 0;
    vector<DeadlineAtom> atoms;
    vector<int> cell_atom;
    vector<int> free_atoms;
};

optional<DeadlineQuotientGraph> build_deadline_quotient_graph(
    const vs &park, const vvi &owner, const vector<GroupState> &groups) {
    int n = park.size();
    DeadlineQuotientGraph graph;
    graph.n = n;
    graph.cell_atom.assign(n * n, -1);
    graph.atoms.reserve(n * n);

    // Active groups are owner-closed atoms: selecting one always selects its
    // complete old region, which is essential for simultaneous swaps/cycles.
    for (int id = 0; id < (int)groups.size(); id++) {
        if (!groups[id].active) continue;
        int atom_id = graph.atoms.size();
        DeadlineAtom atom;
        atom.group_id = id;
        atom.cells = groups[id].cells;
        if ((int)atom.cells.size() != groups[id].p) return nullopt;
        for (auto [x, y] : atom.cells) {
            if (!inside(x, y, n, n) || park[x][y] != '.' || owner[x][y] != id) return nullopt;
            int cell = x * n + y;
            if (graph.cell_atom[cell] != -1) return nullopt;
            graph.cell_atom[cell] = atom_id;
        }
        graph.atoms.push_back(std::move(atom));
    }

    constexpr int DX[4] = {-1, 1, 0, 0};
    constexpr int DY[4] = {0, 0, -1, 1};
    array<int, 2500> que{};
    for (int start = 0; start < n * n; start++) {
        int sx = start / n;
        int sy = start % n;
        if (park[sx][sy] != '.' || owner[sx][sy] != -1 || graph.cell_atom[start] != -1) continue;
        int atom_id = graph.atoms.size();
        graph.atoms.push_back(DeadlineAtom{});
        DeadlineAtom &atom = graph.atoms.back();
        atom.is_free = true;
        graph.free_atoms.push_back(atom_id);
        int head = 0;
        int tail = 0;
        que[tail++] = start;
        graph.cell_atom[start] = atom_id;
        while (head < tail) {
            int cell = que[head++];
            int x = cell / n;
            int y = cell % n;
            atom.cells.emplace_back(x, y);
            for (int dir = 0; dir < 4; dir++) {
                int nx = x + DX[dir];
                int ny = y + DY[dir];
                if (!inside(nx, ny, n, n)) continue;
                int next = nx * n + ny;
                if (park[nx][ny] != '.' || owner[nx][ny] != -1 || graph.cell_atom[next] != -1) continue;
                graph.cell_atom[next] = atom_id;
                que[tail++] = next;
            }
        }
    }

    vector<pair<int, int>> edges;
    edges.reserve(2 * n * n);
    for (int x = 0; x < n; x++) {
        for (int y = 0; y < n; y++) {
            if (park[x][y] != '.') continue;
            int from = graph.cell_atom[x * n + y];
            if (from < 0) return nullopt;
            if (x + 1 < n && park[x + 1][y] == '.') {
                int to = graph.cell_atom[(x + 1) * n + y];
                if (from != to) edges.emplace_back(min(from, to), max(from, to));
            }
            if (y + 1 < n && park[x][y + 1] == '.') {
                int to = graph.cell_atom[x * n + y + 1];
                if (from != to) edges.emplace_back(min(from, to), max(from, to));
            }
        }
    }
    sort(edges.begin(), edges.end());
    edges.erase(unique(edges.begin(), edges.end()), edges.end());
    for (auto [from, to] : edges) {
        graph.atoms[from].adjacent.push_back(to);
        graph.atoms[to].adjacent.push_back(from);
    }
    return graph;
}

struct DeadlineClosureState {
    BoardMask atoms{};
    int free_area = 0;
    int total_area = 0;
    ll proxy_move_cost = 0;
    bool expanded_after_feasible = false;
    uint64_t order = 0;
};

struct DeadlineClosureOrder {
    int required_free = 0;

    bool operator()(const DeadlineClosureState &lhs, const DeadlineClosureState &rhs) const {
        auto key = [&](const DeadlineClosureState &state) {
            int deficit = max(0, required_free - state.free_area);
            return tuple<int, ll, int, int, uint64_t>{
                deficit, state.proxy_move_cost, state.total_area, -state.free_area, state.order};
        };
        return key(lhs) > key(rhs);
    }
};

DeadlineClosureState make_deadline_seed_closure(const DeadlineQuotientGraph &graph, int free_atom,
                                                uint64_t order) {
    DeadlineClosureState state;
    deadline_mask_set(state.atoms, free_atom);
    state.free_area = graph.atoms[free_atom].cells.size();
    state.total_area = state.free_area;
    state.order = order;
    return state;
}

DeadlineClosureState extend_deadline_closure(
    const DeadlineClosureState &parent, int group_atom, const DeadlineQuotientGraph &graph,
    const vector<GroupState> &groups, int r_milli, uint64_t order) {
    DeadlineClosureState child = parent;
    child.order = order;
    child.expanded_after_feasible = parent.free_area > 0 && parent.expanded_after_feasible;
    if (!deadline_mask_has(child.atoms, group_atom)) {
        deadline_mask_set(child.atoms, group_atom);
        const DeadlineAtom &atom = graph.atoms[group_atom];
        child.total_area += atom.cells.size();
        ll candidate_cost = move_cost(groups[atom.group_id].v, r_milli);
        child.proxy_move_cost = child.proxy_move_cost == 0
                                    ? candidate_cost
                                    : min(child.proxy_move_cost, candidate_cost);
    }
    // FreeClose: once a group is selected, take every adjacent current free
    // component.  This prevents a workspace from arbitrarily cutting away the
    // free reservoir exposed around that group.
    for (int next : graph.atoms[group_atom].adjacent) {
        const DeadlineAtom &atom = graph.atoms[next];
        if (!atom.is_free || deadline_mask_has(child.atoms, next)) continue;
        deadline_mask_set(child.atoms, next);
        child.free_area += atom.cells.size();
        child.total_area += atom.cells.size();
    }
    return child;
}

vector<DeadlineClosureState> enumerate_deadline_closures(
    const DeadlineQuotientGraph &graph, const vector<GroupState> &groups, int required_free,
    int r_milli, int &turn_expansions, DeadlineLayerDiagnostics &diagnostics) {
    priority_queue<DeadlineClosureState, vector<DeadlineClosureState>, DeadlineClosureOrder> queue(
        DeadlineClosureOrder{required_free});
    set<BoardMask> seen;
    uint64_t order = 0;
    for (int atom : graph.free_atoms) {
        DeadlineClosureState state = make_deadline_seed_closure(graph, atom, order++);
        if (seen.insert(state.atoms).second) queue.push(std::move(state));
    }

    vector<DeadlineClosureState> complete;
    while (!queue.empty()) {
        DeadlineClosureState state = queue.top();
        queue.pop();
        diagnostics.closure_states++;
        bool feasible = state.free_area >= required_free;
        if (feasible) {
            complete.push_back(state);
            diagnostics.completed_closures++;
            if ((int)complete.size() >= 2 * DEADLINE_CLOSURE_KEEP_LIMIT || state.expanded_after_feasible) {
                continue;
            }
        }

        vector<int> frontier;
        for (int atom = 0; atom < (int)graph.atoms.size(); atom++) {
            if (!deadline_mask_has(state.atoms, atom)) continue;
            for (int next : graph.atoms[atom].adjacent) {
                if (deadline_mask_has(state.atoms, next) || graph.atoms[next].is_free) continue;
                frontier.push_back(next);
            }
        }
        sort(frontier.begin(), frontier.end(), [&](int lhs, int rhs) {
            auto key = [&](int atom_id) {
                int free_gain = 0;
                for (int next : graph.atoms[atom_id].adjacent) {
                    if (graph.atoms[next].is_free && !deadline_mask_has(state.atoms, next)) {
                        free_gain += graph.atoms[next].cells.size();
                    }
                }
                int deficit = max(0, required_free - state.free_area - free_gain);
                ll cost = move_cost(groups[graph.atoms[atom_id].group_id].v, r_milli);
                return tuple<int, ll, int, int>{deficit, cost, -free_gain, atom_id};
            };
            return key(lhs) < key(rhs);
        });
        frontier.erase(unique(frontier.begin(), frontier.end()), frontier.end());
        if (feasible && (int)frontier.size() > 4) frontier.resize(4);
        for (int group_atom : frontier) {
            if (turn_expansions >= DEADLINE_CLOSURE_EXPANSION_TURN_LIMIT ||
                diagnostics.closure_expansions >= DEADLINE_CLOSURE_EXPANSION_CASE_LIMIT) {
                diagnostics.closure_limit_exhausted++;
                break;
            }
            turn_expansions++;
            diagnostics.closure_expansions++;
            DeadlineClosureState child = extend_deadline_closure(
                state, group_atom, graph, groups, r_milli, order++);
            if (feasible) child.expanded_after_feasible = true;
            if (seen.insert(child.atoms).second) queue.push(std::move(child));
        }
        if (turn_expansions >= DEADLINE_CLOSURE_EXPANSION_TURN_LIMIT ||
            diagnostics.closure_expansions >= DEADLINE_CLOSURE_EXPANSION_CASE_LIMIT) {
            break;
        }
    }

    auto closure_key = [](const DeadlineClosureState &state) {
        return tuple<ll, int, int, uint64_t>{
            state.proxy_move_cost, state.total_area, -state.free_area, state.order};
    };
    sort(complete.begin(), complete.end(), [&](const DeadlineClosureState &lhs,
                                               const DeadlineClosureState &rhs) {
        return closure_key(lhs) < closure_key(rhs);
    });
    vector<DeadlineClosureState> retained;
    for (const DeadlineClosureState &state : complete) {
        if ((int)retained.size() == DEADLINE_CLOSURE_KEEP_LIMIT - 1) break;
        retained.push_back(state);
    }

    // Also expose one true global candidate: every atom in one quotient-graph
    // component.  It is admitted only when even one node per item fits the
    // remaining deterministic layout budget; there is no group-count rule.
    vector<char> graph_seen(graph.atoms.size(), false);
    optional<DeadlineClosureState> global;
    for (int seed : graph.free_atoms) {
        if (graph_seen[seed]) continue;
        DeadlineClosureState state;
        vector<int> stack{seed};
        graph_seen[seed] = true;
        int item_count = 0;
        while (!stack.empty()) {
            int atom_id = stack.back();
            stack.pop_back();
            const DeadlineAtom &atom = graph.atoms[atom_id];
            deadline_mask_set(state.atoms, atom_id);
            state.total_area += atom.cells.size();
            if (atom.is_free) {
                state.free_area += atom.cells.size();
            } else {
                item_count++;
                ll candidate_cost = move_cost(groups[atom.group_id].v, r_milli);
                state.proxy_move_cost = state.proxy_move_cost == 0
                                            ? candidate_cost
                                            : min(state.proxy_move_cost, candidate_cost);
            }
            for (int next : atom.adjacent) {
                if (graph_seen[next]) continue;
                graph_seen[next] = true;
                stack.push_back(next);
            }
        }
        long long object_count = item_count + 1LL;
        long long minimum_layout_work = object_count * (object_count + 1) / 2;
        if (state.free_area < required_free ||
            minimum_layout_work > DEADLINE_LAYOUT_NODE_WORKSPACE_LIMIT ||
            minimum_layout_work > DEADLINE_LAYOUT_NODE_CASE_LIMIT - diagnostics.layout_nodes) {
            continue;
        }
        state.order = order++;
        if (!global || closure_key(state) < closure_key(*global)) global = std::move(state);
    }
    if (global) {
        bool duplicate = false;
        for (const auto &state : retained) duplicate |= state.atoms == global->atoms;
        if (!duplicate) {
            retained.insert(retained.begin(), std::move(*global));
            diagnostics.global_closures++;
        }
    }
    if ((int)retained.size() > DEADLINE_CLOSURE_KEEP_LIMIT) {
        retained.resize(DEADLINE_CLOSURE_KEEP_LIMIT);
    }
    return retained;
}

struct DeadlineTurnWork {
    int closure_expansions = 0;
    int layout_nodes = 0;
    int template_probes = 0;
    int growth_steps = 0;
    int complete_plans = 0;
    long long connectivity_visits = 0;
    int connectivity_calls = 0;
};

bool deadline_connectivity_budget_exhausted(const DeadlineTurnWork &work,
                                            const DeadlineLayerDiagnostics &diagnostics) {
    return work.connectivity_calls >= DEADLINE_CONNECTIVITY_CALL_TURN_LIMIT ||
           diagnostics.connectivity_calls >= DEADLINE_CONNECTIVITY_CALL_CASE_LIMIT ||
           work.connectivity_visits >= DEADLINE_CONNECTIVITY_VISIT_TURN_LIMIT ||
           diagnostics.connectivity_visits >= DEADLINE_CONNECTIVITY_VISIT_CASE_LIMIT;
}

bool deadline_begin_connectivity_call(DeadlineTurnWork &work,
                                      DeadlineLayerDiagnostics &diagnostics) {
    if (deadline_connectivity_budget_exhausted(work, diagnostics)) {
        diagnostics.connectivity_limit_exhausted++;
        return false;
    }
    work.connectivity_calls++;
    diagnostics.connectivity_calls++;
    return true;
}

bool deadline_consume_connectivity_visit(DeadlineTurnWork &work,
                                         DeadlineLayerDiagnostics &diagnostics) {
    if (work.connectivity_visits >= DEADLINE_CONNECTIVITY_VISIT_TURN_LIMIT ||
        diagnostics.connectivity_visits >= DEADLINE_CONNECTIVITY_VISIT_CASE_LIMIT) {
        diagnostics.connectivity_limit_exhausted++;
        return false;
    }
    work.connectivity_visits++;
    diagnostics.connectivity_visits++;
    return true;
}

bool deadline_mask_connected(const BoardMask &mask, int expected_count, int n,
                             DeadlineTurnWork &work, DeadlineLayerDiagnostics &diagnostics) {
    if (expected_count <= 0 || deadline_mask_count(mask) != expected_count) return false;
    if (!deadline_begin_connectivity_call(work, diagnostics)) return false;
    int first = -1;
    for (int word = 0; word < BOARD_MASK_WORDS && first < 0; word++) {
        if (mask[word]) first = 64 * word + __builtin_ctzll(mask[word]);
    }
    if (first < 0 || first >= n * n) return false;
    array<int, 2500> que{};
    array<unsigned char, 2500> visited{};
    int head = 0;
    int tail = 0;
    que[tail++] = first;
    visited[first] = true;
    constexpr int DX[4] = {-1, 1, 0, 0};
    constexpr int DY[4] = {0, 0, -1, 1};
    while (head < tail) {
        if (!deadline_consume_connectivity_visit(work, diagnostics)) return false;
        int cell = que[head++];
        int x = cell / n;
        int y = cell % n;
        for (int dir = 0; dir < 4; dir++) {
            int nx = x + DX[dir];
            int ny = y + DY[dir];
            if (!inside(nx, ny, n, n)) continue;
            int next = nx * n + ny;
            if (visited[next] || !deadline_mask_has(mask, next)) continue;
            visited[next] = true;
            que[tail++] = next;
        }
    }
    return tail == expected_count;
}

optional<vector<int>> deadline_mask_distances(const BoardMask &mask, int root, int n,
                                              DeadlineTurnWork &work,
                                              DeadlineLayerDiagnostics &diagnostics) {
    if (!deadline_mask_has(mask, root)) return nullopt;
    if (!deadline_begin_connectivity_call(work, diagnostics)) return nullopt;
    vector<int> distance(n * n, -1);
    array<int, 2500> que{};
    int head = 0;
    int tail = 0;
    que[tail++] = root;
    distance[root] = 0;
    constexpr int DX[4] = {-1, 1, 0, 0};
    constexpr int DY[4] = {0, 0, -1, 1};
    while (head < tail) {
        if (!deadline_consume_connectivity_visit(work, diagnostics)) return nullopt;
        int cell = que[head++];
        int x = cell / n;
        int y = cell % n;
        for (int dir = 0; dir < 4; dir++) {
            int nx = x + DX[dir];
            int ny = y + DY[dir];
            if (!inside(nx, ny, n, n)) continue;
            int next = nx * n + ny;
            if (distance[next] != -1 || !deadline_mask_has(mask, next)) continue;
            distance[next] = distance[cell] + 1;
            que[tail++] = next;
        }
    }
    return distance;
}

optional<vector<int>> deadline_boundary_clearance(const BoardMask &mask, int n,
                                                  DeadlineTurnWork &work,
                                                  DeadlineLayerDiagnostics &diagnostics) {
    if (!deadline_begin_connectivity_call(work, diagnostics)) return nullopt;
    vector<int> clearance(n * n, -1);
    array<int, 2500> que{};
    int head = 0;
    int tail = 0;
    constexpr int DX[4] = {-1, 1, 0, 0};
    constexpr int DY[4] = {0, 0, -1, 1};
    for (int cell = 0; cell < n * n; cell++) {
        if (!deadline_mask_has(mask, cell)) continue;
        int x = cell / n;
        int y = cell % n;
        bool boundary = false;
        for (int dir = 0; dir < 4; dir++) {
            int nx = x + DX[dir];
            int ny = y + DY[dir];
            if (!inside(nx, ny, n, n) || !deadline_mask_has(mask, nx * n + ny)) {
                boundary = true;
            }
        }
        if (boundary) {
            clearance[cell] = 0;
            que[tail++] = cell;
        }
    }
    while (head < tail) {
        if (!deadline_consume_connectivity_visit(work, diagnostics)) return nullopt;
        int cell = que[head++];
        int x = cell / n;
        int y = cell % n;
        for (int dir = 0; dir < 4; dir++) {
            int nx = x + DX[dir];
            int ny = y + DY[dir];
            if (!inside(nx, ny, n, n)) continue;
            int next = nx * n + ny;
            if (!deadline_mask_has(mask, next) || clearance[next] != -1) continue;
            clearance[next] = clearance[cell] + 1;
            que[tail++] = next;
        }
    }
    return clearance;
}

int deadline_mask_perimeter(const BoardMask &mask, int n) {
    int perimeter = 0;
    constexpr int DX[4] = {-1, 1, 0, 0};
    constexpr int DY[4] = {0, 0, -1, 1};
    for (int cell = 0; cell < n * n; cell++) {
        if (!deadline_mask_has(mask, cell)) continue;
        int x = cell / n;
        int y = cell % n;
        for (int dir = 0; dir < 4; dir++) {
            int nx = x + DX[dir];
            int ny = y + DY[dir];
            if (!inside(nx, ny, n, n) || !deadline_mask_has(mask, nx * n + ny)) perimeter++;
        }
    }
    return perimeter;
}

struct DeadlineWorkspace {
    BoardMask mask{};
    BoardMask current_free{};
    vector<int> group_ids;
    int area = 0;
    int free_area = 0;
};

optional<DeadlineWorkspace> materialize_deadline_workspace(
    const DeadlineClosureState &closure, const DeadlineQuotientGraph &graph, int arrival_p,
    DeadlineTurnWork &work, DeadlineLayerDiagnostics &diagnostics) {
    DeadlineWorkspace workspace;
    for (int atom_id = 0; atom_id < (int)graph.atoms.size(); atom_id++) {
        if (!deadline_mask_has(closure.atoms, atom_id)) continue;
        const DeadlineAtom &atom = graph.atoms[atom_id];
        for (auto [x, y] : atom.cells) {
            int cell = x * graph.n + y;
            deadline_mask_set(workspace.mask, cell);
            if (atom.is_free) deadline_mask_set(workspace.current_free, cell);
        }
        workspace.area += atom.cells.size();
        if (atom.is_free) {
            workspace.free_area += atom.cells.size();
        } else {
            workspace.group_ids.push_back(atom.group_id);
        }
    }
    sort(workspace.group_ids.begin(), workspace.group_ids.end());
    if (workspace.free_area < arrival_p + 1 || workspace.area != deadline_mask_count(workspace.mask) ||
        workspace.free_area != deadline_mask_count(workspace.current_free)) {
        diagnostics.partition_errors++;
        return nullopt;
    }
    if (!deadline_mask_connected(workspace.mask, workspace.area, graph.n, work, diagnostics)) {
        return nullopt;
    }
    return workspace;
}

vector<int> make_deadline_core_roots(const DeadlineWorkspace &workspace, int n,
                                     DeadlineTurnWork &work,
                                     DeadlineLayerDiagnostics &diagnostics) {
    vector<int> free_cells;
    free_cells.reserve(workspace.free_area);
    for (int cell = 0; cell < n * n; cell++) {
        if (deadline_mask_has(workspace.current_free, cell)) free_cells.push_back(cell);
    }
    if (free_cells.empty()) return {};

    vector<int> roots;
    optional<vector<int>> clearance = deadline_boundary_clearance(
        workspace.mask, n, work, diagnostics);
    if (!clearance) return {};
    int first_root = free_cells.front();
    for (int cell : free_cells) {
        if ((*clearance)[cell] > (*clearance)[first_root] ||
            ((*clearance)[cell] == (*clearance)[first_root] && cell < first_root)) {
            first_root = cell;
        }
    }
    roots.push_back(first_root);
    if (DEADLINE_CORE_ROOT_LIMIT == 1) return roots;

    optional<vector<int>> second_distance = deadline_mask_distances(
        workspace.mask, first_root, n, work, diagnostics);
    if (!second_distance) return roots;
    int second_root = first_root;
    for (int cell : free_cells) {
        if ((*second_distance)[cell] > (*second_distance)[second_root] ||
            ((*second_distance)[cell] == (*second_distance)[second_root] && cell < second_root)) {
            second_root = cell;
        }
    }
    if (second_root != first_root) roots.push_back(second_root);
    return roots;
}

struct DeadlineLayerItem {
    bool is_arrival = false;
    int id = -1;
    ll t = 0;
    ll v = 0;
    int p = 0;
    int old_max_perimeter = 0;
    BoardMask old_region{};
};

vector<DeadlineLayerItem> make_deadline_layer_items(
    const DeadlineWorkspace &workspace, const vector<GroupState> &groups, int arrival_id,
    int n) {
    vector<DeadlineLayerItem> items;
    items.reserve(workspace.group_ids.size() + 1);
    for (int id : workspace.group_ids) {
        const GroupState &group = groups[id];
        items.push_back({false, id, group.t, group.v, group.p, group.max_perimeter,
                         make_board_mask(group.cells, n)});
    }
    const GroupState &arrival = groups[arrival_id];
    items.push_back({true, arrival_id, arrival.t, arrival.v, arrival.p, 0, BoardMask{}});
    sort(items.begin(), items.end(), [](const DeadlineLayerItem &lhs,
                                       const DeadlineLayerItem &rhs) {
        if (lhs.t != rhs.t) return lhs.t > rhs.t;
        if (lhs.p != rhs.p) return lhs.p > rhs.p;
        if (lhs.is_arrival != rhs.is_arrival) return lhs.is_arrival < rhs.is_arrival;
        return lhs.id < rhs.id;
    });
    return items;
}

vector<int> deadline_boundary_seeds(const BoardMask &remaining, int root,
                                    const vector<int> &distance, int n, int limit) {
    struct RankedCell {
        int distance;
        int outside_edges;
        int cell;
    };
    vector<RankedCell> ranked;
    constexpr int DX[4] = {-1, 1, 0, 0};
    constexpr int DY[4] = {0, 0, -1, 1};
    for (int cell = 0; cell < n * n; cell++) {
        if (cell == root || !deadline_mask_has(remaining, cell)) continue;
        int x = cell / n;
        int y = cell % n;
        int outside_edges = 0;
        for (int dir = 0; dir < 4; dir++) {
            int nx = x + DX[dir];
            int ny = y + DY[dir];
            if (!inside(nx, ny, n, n) || !deadline_mask_has(remaining, nx * n + ny)) {
                outside_edges++;
            }
        }
        if (outside_edges > 0) ranked.push_back({distance[cell], outside_edges, cell});
    }
    sort(ranked.begin(), ranked.end(), [](const RankedCell &lhs, const RankedCell &rhs) {
        if (lhs.distance != rhs.distance) return lhs.distance > rhs.distance;
        if (lhs.outside_edges != rhs.outside_edges) return lhs.outside_edges > rhs.outside_edges;
        return lhs.cell < rhs.cell;
    });
    vector<int> result;
    for (const RankedCell &entry : ranked) {
        result.push_back(entry.cell);
        if ((int)result.size() == limit) break;
    }
    return result;
}

struct DeadlineRegionCandidate {
    BoardMask mask{};
    int perimeter = 0;
    ll local_direct = 0;
    ll movement_cost = 0;
    ll fee_loss = 0;
    bool moved = false;
    uint64_t order = 0;
};

DeadlineRegionCandidate score_deadline_region(
    const BoardMask &region, int perimeter, const DeadlineLayerItem &item,
    const vector<GroupState> &groups, int r_milli, uint64_t order) {
    DeadlineRegionCandidate result;
    result.mask = region;
    result.perimeter = perimeter;
    result.order = order;
    if (item.is_arrival) {
        result.local_direct = round_payment(item.v, item.p, perimeter);
        return result;
    }
    result.moved = region != item.old_region;
    if (!result.moved) return result;
    const GroupState &group = groups[item.id];
    result.movement_cost = move_cost(group.v, r_milli);
    int next_max_perimeter = max(group.max_perimeter, perimeter);
    result.fee_loss = round_payment(group.v, group.p, group.max_perimeter) -
                      round_payment(group.v, group.p, next_max_perimeter);
    result.local_direct = -result.movement_cost - result.fee_loss;
    return result;
}

optional<DeadlineRegionCandidate> make_deadline_ear_growth(
    const BoardMask &initial_remaining, int remaining_count, int root, int seed,
    const vector<int> &distance, const DeadlineLayerItem &item,
    const vector<GroupState> &groups, int r_milli, int n, uint64_t order,
    bool preserve_every_step, DeadlineTurnWork &work,
    DeadlineLayerDiagnostics &diagnostics) {
    if (seed == root || !deadline_mask_has(initial_remaining, seed) || item.p >= remaining_count) {
        return nullopt;
    }
    BoardMask remaining = initial_remaining;
    BoardMask region{};
    BoardMask frontier{};
    deadline_mask_set(frontier, seed);
    constexpr int DX[4] = {-1, 1, 0, 0};
    constexpr int DY[4] = {0, 0, -1, 1};

    for (int placed = 0; placed < item.p; placed++) {
        if (work.growth_steps >= DEADLINE_GROWTH_STEP_TURN_LIMIT ||
            diagnostics.growth_steps >= DEADLINE_GROWTH_STEP_CASE_LIMIT) {
            diagnostics.growth_limit_exhausted++;
            return nullopt;
        }
        work.growth_steps++;
        diagnostics.growth_steps++;
        struct Choice {
            int shared_edges;
            int distance;
            int boundary_edges;
            int cell;
        };
        vector<Choice> choices;
        for (int word = 0; word < BOARD_MASK_WORDS; word++) {
            uint64_t bits = frontier[word] & remaining[word];
            while (bits) {
                int bit = __builtin_ctzll(bits);
                bits &= bits - 1;
                int cell = 64 * word + bit;
                if (cell >= n * n || cell == root) continue;
            int x = cell / n;
            int y = cell % n;
            int shared_edges = 0;
            int boundary_edges = 0;
            for (int dir = 0; dir < 4; dir++) {
                int nx = x + DX[dir];
                int ny = y + DY[dir];
                if (!inside(nx, ny, n, n)) {
                    boundary_edges++;
                    continue;
                }
                int next = nx * n + ny;
                shared_edges += deadline_mask_has(region, next);
                boundary_edges += !deadline_mask_has(initial_remaining, next);
            }
            choices.push_back({shared_edges, distance[cell], boundary_edges, cell});
            }
        }
        sort(choices.begin(), choices.end(), [](const Choice &lhs, const Choice &rhs) {
            if (lhs.shared_edges != rhs.shared_edges) return lhs.shared_edges > rhs.shared_edges;
            if (lhs.distance != rhs.distance) return lhs.distance > rhs.distance;
            if (lhs.boundary_edges != rhs.boundary_edges) return lhs.boundary_edges > rhs.boundary_edges;
            return lhs.cell < rhs.cell;
        });

        int selected = -1;
        int tries = 0;
        for (const Choice &choice : choices) {
            if (!preserve_every_step) {
                selected = choice.cell;
                break;
            }
            if (tries++ == 6) break;
            BoardMask next_remaining = remaining;
            deadline_mask_reset(next_remaining, choice.cell);
            int next_count = remaining_count - placed - 1;
            if (deadline_mask_connected(next_remaining, next_count, n, work, diagnostics)) {
                selected = choice.cell;
                break;
            }
        }
        if (selected < 0) return nullopt;
        deadline_mask_reset(remaining, selected);
        deadline_mask_reset(frontier, selected);
        deadline_mask_set(region, selected);
        int x = selected / n;
        int y = selected % n;
        for (int dir = 0; dir < 4; dir++) {
            int nx = x + DX[dir];
            int ny = y + DY[dir];
            if (!inside(nx, ny, n, n)) continue;
            int next = nx * n + ny;
            if (deadline_mask_has(remaining, next)) deadline_mask_set(frontier, next);
        }
    }
    if (deadline_mask_count(region) != item.p) return nullopt;
    if (!preserve_every_step &&
        !deadline_mask_connected(remaining, remaining_count - item.p, n, work, diagnostics)) {
        return nullopt;
    }
    int perimeter = deadline_mask_perimeter(region, n);
    return score_deadline_region(region, perimeter, item, groups, r_milli, order);
}

vector<DeadlineRegionCandidate> make_deadline_region_candidates(
    const BoardMask &remaining, int remaining_count, int root,
    const DeadlineLayerItem &item, const vector<GroupState> &groups, int r_milli,
    int n, const vector<vector<Shape>> &compact_shapes, DeadlineTurnWork &work,
    DeadlineLayerDiagnostics &diagnostics, uint64_t &order) {
    vector<DeadlineRegionCandidate> candidates;
    set<BoardMask> seen;
    auto try_region = [&](const BoardMask &region, int perimeter) {
        if ((int)candidates.size() >= 2 * DEADLINE_REGION_CANDIDATE_LIMIT ||
            deadline_mask_has(region, root) || deadline_mask_count(region) != item.p ||
            !deadline_mask_subset(region, remaining) || !seen.insert(region).second) {
            return;
        }
        BoardMask next_remaining = deadline_mask_difference(remaining, region);
        if (!deadline_mask_connected(next_remaining, remaining_count - item.p, n, work, diagnostics)) {
            return;
        }
        candidates.push_back(score_deadline_region(
            region, perimeter, item, groups, r_milli, order++));
        diagnostics.region_candidates++;
    };

    if (!item.is_arrival && deadline_mask_subset(item.old_region, remaining)) {
        try_region(item.old_region, deadline_mask_perimeter(item.old_region, n));
    }

    optional<vector<int>> distance = deadline_mask_distances(
        remaining, root, n, work, diagnostics);
    if (!distance) return candidates;
    vector<int> boundary = deadline_boundary_seeds(remaining, root, *distance, n, 8);
    vector<int> shape_indices;
    set<pair<int, int>> seen_dimensions;
    auto add_shape_index = [&](int index) {
        if (index < 0 || index >= (int)compact_shapes[item.p].size() ||
            (int)shape_indices.size() == 4) {
            return;
        }
        const Shape &shape = compact_shapes[item.p][index];
        if (seen_dimensions.insert({shape.h, shape.w}).second) shape_indices.push_back(index);
    };
    add_shape_index(0);
    for (int relation : {1, -1, 0}) {
        for (int index = 0; index < (int)compact_shapes[item.p].size(); index++) {
            const Shape &shape = compact_shapes[item.p][index];
            int shape_relation = (shape.h > shape.w) - (shape.h < shape.w);
            if (shape_relation == relation) {
                add_shape_index(index);
                break;
            }
        }
    }
    for (int index = 0; index < (int)compact_shapes[item.p].size(); index++) add_shape_index(index);
    for (int seed : boundary) {
        for (int shape_index : shape_indices) {
            const Shape &shape = compact_shapes[item.p][shape_index];
            array<pair<int, int>, 4> offsets = {
                pair<int, int>{0, 0}, {shape.h - 1, 0},
                {0, shape.w - 1}, {shape.h - 1, shape.w - 1}};
            for (auto [dx, dy] : offsets) {
                if (deadline_connectivity_budget_exhausted(work, diagnostics)) break;
                if (work.template_probes >= DEADLINE_TEMPLATE_PROBE_TURN_LIMIT ||
                    diagnostics.template_probes >= DEADLINE_TEMPLATE_PROBE_CASE_LIMIT) {
                    diagnostics.template_limit_exhausted++;
                    break;
                }
                work.template_probes++;
                diagnostics.template_probes++;
                int base_x = seed / n - dx;
                int base_y = seed % n - dy;
                if (base_x < 0 || base_y < 0 || base_x + shape.h > n || base_y + shape.w > n) {
                    continue;
                }
                vector<Cell> cells = materialize_shape(shape, base_x, base_y, item.p);
                BoardMask region{};
                bool legal = true;
                for (auto [x, y] : cells) {
                    int cell = x * n + y;
                    if (!deadline_mask_has(remaining, cell) || cell == root ||
                        deadline_mask_has(region, cell)) {
                        legal = false;
                        break;
                    }
                    deadline_mask_set(region, cell);
                }
                if (legal) try_region(region, shape.perimeter);
            }
            if (work.template_probes >= DEADLINE_TEMPLATE_PROBE_TURN_LIMIT ||
                diagnostics.template_probes >= DEADLINE_TEMPLATE_PROBE_CASE_LIMIT ||
                deadline_connectivity_budget_exhausted(work, diagnostics)) {
                break;
            }
        }
        if (work.template_probes >= DEADLINE_TEMPLATE_PROBE_TURN_LIMIT ||
            diagnostics.template_probes >= DEADLINE_TEMPLATE_PROBE_CASE_LIMIT ||
            deadline_connectivity_budget_exhausted(work, diagnostics)) {
            break;
        }
    }

    vector<pair<int, bool>> growth_modes;
    if (!boundary.empty()) {
        growth_modes.push_back({0, true});
        growth_modes.push_back({0, false});
    }
    if ((int)boundary.size() >= 2) growth_modes.push_back({1, false});
    for (auto [index, preserve_every_step] : growth_modes) {
        if (deadline_connectivity_budget_exhausted(work, diagnostics)) break;
        optional<DeadlineRegionCandidate> growth = make_deadline_ear_growth(
            remaining, remaining_count, root, boundary[index], *distance, item, groups,
            r_milli, n, order++, preserve_every_step, work, diagnostics);
        if (growth && seen.insert(growth->mask).second) {
            candidates.push_back(std::move(*growth));
            diagnostics.region_candidates++;
        }
    }

    sort(candidates.begin(), candidates.end(), [](const DeadlineRegionCandidate &lhs,
                                                   const DeadlineRegionCandidate &rhs) {
        if (lhs.local_direct != rhs.local_direct) return lhs.local_direct > rhs.local_direct;
        if (lhs.moved != rhs.moved) return lhs.moved < rhs.moved;
        if (lhs.perimeter != rhs.perimeter) return lhs.perimeter < rhs.perimeter;
        return lhs.order < rhs.order;
    });
    if ((int)candidates.size() > DEADLINE_REGION_CANDIDATE_LIMIT) {
        candidates.resize(DEADLINE_REGION_CANDIDATE_LIMIT);
    }
    return candidates;
}

struct DeadlinePeelState {
    BoardMask remaining{};
    int remaining_count = 0;
    vector<BoardMask> assignments;
    ll direct_score = 0;
    ll movement_cost = 0;
    ll fee_loss = 0;
    int moved_groups = 0;
    int perimeter_sum = 0;
    uint64_t order = 0;
};

struct DeadlineLayoutResult {
    DeadlineWorkspace workspace;
    vector<DeadlineLayerItem> items;
    vector<BoardMask> assignments;
    BoardMask core{};
    ll direct_score = 0;
    ll movement_cost = 0;
    ll fee_loss = 0;
    int moved_groups = 0;
    uint64_t order = 0;
};

vector<DeadlineLayoutResult> search_deadline_layout(
    const DeadlineWorkspace &workspace, const vector<GroupState> &groups, int arrival_id,
    int r_milli, int n, const vector<vector<Shape>> &compact_shapes,
    DeadlineTurnWork &work, DeadlineLayerDiagnostics &diagnostics) {
    vector<DeadlineLayoutResult> results;
    vector<DeadlineLayerItem> items = make_deadline_layer_items(workspace, groups, arrival_id, n);
    vector<int> roots = make_deadline_core_roots(workspace, n, work, diagnostics);
    uint64_t order = 0;
    int workspace_layout_begin = work.layout_nodes;
    for (int root : roots) {
        vector<DeadlinePeelState> beam(1);
        beam.front().remaining = workspace.mask;
        beam.front().remaining_count = workspace.area;
        bool exhausted = false;
        for (int item_index = 0; item_index < (int)items.size(); item_index++) {
            vector<DeadlinePeelState> next;
            for (const DeadlinePeelState &state : beam) {
                if (work.layout_nodes - workspace_layout_begin >= DEADLINE_LAYOUT_NODE_WORKSPACE_LIMIT ||
                    work.layout_nodes >= DEADLINE_LAYOUT_NODE_TURN_LIMIT ||
                    diagnostics.layout_nodes >= DEADLINE_LAYOUT_NODE_CASE_LIMIT) {
                    diagnostics.layout_limit_exhausted++;
                    exhausted = true;
                    break;
                }
                vector<DeadlineRegionCandidate> regions = make_deadline_region_candidates(
                    state.remaining, state.remaining_count, root, items[item_index], groups,
                    r_milli, n, compact_shapes, work, diagnostics, order);
                for (const DeadlineRegionCandidate &region : regions) {
                    int node_work = (int)state.assignments.size() + 1;
                    if (work.layout_nodes - workspace_layout_begin + node_work >
                            DEADLINE_LAYOUT_NODE_WORKSPACE_LIMIT ||
                        work.layout_nodes + node_work > DEADLINE_LAYOUT_NODE_TURN_LIMIT ||
                        diagnostics.layout_nodes + node_work > DEADLINE_LAYOUT_NODE_CASE_LIMIT) {
                        diagnostics.layout_limit_exhausted++;
                        exhausted = true;
                        break;
                    }
                    work.layout_nodes += node_work;
                    diagnostics.layout_nodes += node_work;
                    DeadlinePeelState child = state;
                    child.remaining = deadline_mask_difference(state.remaining, region.mask);
                    child.remaining_count -= items[item_index].p;
                    child.assignments.push_back(region.mask);
                    child.direct_score += region.local_direct;
                    child.movement_cost += region.movement_cost;
                    child.fee_loss += region.fee_loss;
                    child.moved_groups += region.moved;
                    child.perimeter_sum += region.perimeter;
                    child.order = order++;
                    next.push_back(std::move(child));
                }
                if (exhausted) break;
            }
            if (next.empty()) {
                beam.clear();
                break;
            }
            sort(next.begin(), next.end(), [](const DeadlinePeelState &lhs,
                                              const DeadlinePeelState &rhs) {
                if (lhs.direct_score != rhs.direct_score) return lhs.direct_score > rhs.direct_score;
                if (lhs.moved_groups != rhs.moved_groups) return lhs.moved_groups < rhs.moved_groups;
                if (lhs.perimeter_sum != rhs.perimeter_sum) return lhs.perimeter_sum < rhs.perimeter_sum;
                return lhs.order < rhs.order;
            });
            if ((int)next.size() > DEADLINE_LAYOUT_BEAM_WIDTH) next.resize(DEADLINE_LAYOUT_BEAM_WIDTH);
            beam = std::move(next);
            if (exhausted) break;
        }
        if (exhausted) break;
        for (const DeadlinePeelState &state : beam) {
            if ((int)state.assignments.size() != (int)items.size()) continue;
            if (!deadline_mask_connected(state.remaining, state.remaining_count, n, work, diagnostics)) {
                continue;
            }
            results.push_back({workspace, items, state.assignments, state.remaining,
                               state.direct_score, state.movement_cost, state.fee_loss,
                               state.moved_groups, state.order});
        }
    }
    sort(results.begin(), results.end(), [](const DeadlineLayoutResult &lhs,
                                            const DeadlineLayoutResult &rhs) {
        if (lhs.direct_score != rhs.direct_score) return lhs.direct_score > rhs.direct_score;
        if (lhs.moved_groups != rhs.moved_groups) return lhs.moved_groups < rhs.moved_groups;
        return lhs.order < rhs.order;
    });
    return results;
}

bool validate_and_build_deadline_owner(
    const TurnPlan &plan, const vs &park, const vvi &owner,
    const vector<GroupState> &groups, int arrival_id, int r_milli,
    vvi &final_owner, ll &fee_loss, ll &movement_cost_sum) {
    if (!plan.arrival || groups[arrival_id].active) return false;
    int n = park.size();
    vector<char> moved(groups.size(), false);
    final_owner = owner;
    for (const MovePlan &move : plan.moves) {
        if (move.id < 0 || move.id >= (int)groups.size() || move.id == arrival_id ||
            moved[move.id] || !groups[move.id].active) {
            return false;
        }
        moved[move.id] = true;
        const GroupState &group = groups[move.id];
        if ((int)group.cells.size() != group.p) return false;
        for (auto [x, y] : group.cells) {
            if (!inside(x, y, n, n) || final_owner[x][y] != move.id) return false;
        }
    }
    for (const MovePlan &move : plan.moves) clear_cells(final_owner, groups[move.id].cells);

    auto region_is_legal = [&](const vector<Cell> &cells, int expected_size) {
        if ((int)cells.size() != expected_size || !validate_connected_region(cells, n)) return false;
        for (auto [x, y] : cells) {
            if (!inside(x, y, n, n) || park[x][y] != '.' || final_owner[x][y] != -1) return false;
        }
        return true;
    };

    fee_loss = 0;
    movement_cost_sum = 0;
    for (const MovePlan &move : plan.moves) {
        const GroupState &group = groups[move.id];
        if (!region_is_legal(move.cells, group.p) || same_region(move.cells, group.cells)) return false;
        int perimeter = calc_perimeter(move.cells, n);
        if (perimeter != move.perimeter) return false;
        ll previous_fee = round_payment(group.v, group.p, group.max_perimeter);
        ll next_fee = round_payment(group.v, group.p, max(group.max_perimeter, perimeter));
        fee_loss += previous_fee - next_fee;
        movement_cost_sum += move_cost(group.v, r_milli);
        place_cells(final_owner, move.cells, move.id);
    }

    const GroupState &arrival = groups[arrival_id];
    if (!region_is_legal(*plan.arrival, arrival.p) ||
        calc_perimeter(*plan.arrival, n) != plan.arrival_perimeter) {
        return false;
    }
    ll arrival_fee = round_payment(arrival.v, arrival.p, plan.arrival_perimeter);
    if (plan.immediate_gain != arrival_fee - movement_cost_sum) return false;
    place_cells(final_owner, *plan.arrival, arrival_id);
    return true;
}

bool validate_deadline_layout_invariant(const DeadlineLayoutResult &layout, int n,
                                        DeadlineLayerDiagnostics &diagnostics) {
    int arrival_p = 0;
    for (const DeadlineLayerItem &item : layout.items) {
        if (item.is_arrival) arrival_p = item.p;
    }
    if (layout.assignments.size() != layout.items.size() || arrival_p == 0 ||
        deadline_mask_count(layout.core) != layout.workspace.free_area - arrival_p) {
        diagnostics.partition_errors++;
        return false;
    }

    BoardMask reconstructed = layout.core;
    if (!validate_connected_region(deadline_mask_cells(reconstructed, n), n)) {
        diagnostics.prefix_connectivity_errors++;
        return false;
    }
    for (int item_index = (int)layout.items.size() - 1; item_index >= 0; item_index--) {
        const BoardMask &region = layout.assignments[item_index];
        if (deadline_mask_count(region) != layout.items[item_index].p ||
            masks_overlap(reconstructed, region) ||
            !validate_connected_region(deadline_mask_cells(region, n), n)) {
            diagnostics.partition_errors++;
            return false;
        }
        merge_mask(reconstructed, region);
        if (!validate_connected_region(deadline_mask_cells(reconstructed, n), n)) {
            diagnostics.prefix_connectivity_errors++;
            return false;
        }
    }
    if (reconstructed != layout.workspace.mask) {
        diagnostics.partition_errors++;
        return false;
    }
    return true;
}

struct PreparedDeadlineCandidate {
    TurnPlan plan;
    vvi final_owner;
    ll arrival_fee = 0;
    ll movement_cost = 0;
    ll fee_loss = 0;
    ll direct_score = 0;
    int moved_groups = 0;
    int moved_cells = 0;
    uint64_t order = 0;
};

optional<PreparedDeadlineCandidate> prepare_deadline_candidate(
    const DeadlineLayoutResult &layout, const vs &park, const vvi &owner,
    const vector<GroupState> &groups, int arrival_id, int r_milli,
    DeadlineLayerDiagnostics &diagnostics) {
    int n = park.size();
    if (!validate_deadline_layout_invariant(layout, n, diagnostics)) return nullopt;

    PreparedDeadlineCandidate candidate;
    candidate.order = layout.order;
    for (int item_index = 0; item_index < (int)layout.items.size(); item_index++) {
        const DeadlineLayerItem &item = layout.items[item_index];
        vector<Cell> cells = deadline_mask_cells(layout.assignments[item_index], n);
        int perimeter = calc_perimeter(cells, n);
        if (item.is_arrival) {
            candidate.plan.arrival = std::move(cells);
            candidate.plan.arrival_perimeter = perimeter;
            candidate.arrival_fee = round_payment(item.v, item.p, perimeter);
        } else if (layout.assignments[item_index] != item.old_region) {
            candidate.moved_groups++;
            candidate.moved_cells += item.p;
            candidate.plan.moves.push_back({item.id, std::move(cells), perimeter});
        }
    }
    sort(candidate.plan.moves.begin(), candidate.plan.moves.end(),
         [](const MovePlan &lhs, const MovePlan &rhs) { return lhs.id < rhs.id; });
    candidate.plan.immediate_gain = candidate.arrival_fee - layout.movement_cost;

    ll validated_fee_loss = 0;
    ll validated_movement_cost = 0;
    if (!validate_and_build_deadline_owner(
            candidate.plan, park, owner, groups, arrival_id, r_milli,
            candidate.final_owner, validated_fee_loss, validated_movement_cost)) {
        diagnostics.validation_failures++;
        return nullopt;
    }
    candidate.movement_cost = validated_movement_cost;
    candidate.fee_loss = validated_fee_loss;
    candidate.direct_score = candidate.arrival_fee - candidate.movement_cost - candidate.fee_loss;
    if (candidate.moved_groups != (int)candidate.plan.moves.size() ||
        candidate.moved_groups != layout.moved_groups ||
        candidate.movement_cost != layout.movement_cost ||
        candidate.fee_loss != layout.fee_loss ||
        candidate.direct_score != layout.direct_score) {
        diagnostics.direct_identity_errors++;
        return nullopt;
    }
    return candidate;
}

optional<RootActionResult> choose_deadline_layer_root(
    const vs &park, const vvi &owner, const vector<GroupState> &groups,
    int arrival_id, int turn, int total_groups, ll current_s, int remaining_groups,
    int free_cells_before, int r_milli, long double theta,
    const ThetaEstimator &theta_estimator, const DensityModel &density_model,
    SampledDlpShadowModel &sampled_dlp_model,
    int grass_cells, long double opportunity_cost, const ArrivalDecision &baseline,
    const vector<vector<Shape>> &compact_shapes, int &deadline_confirmations_used,
    DeadlineLayerDiagnostics &diagnostics) {
    if constexpr (!ENABLE_DEADLINE_LAYER) return nullopt;

    const GroupState &arrival = groups[arrival_id];
    int minimum_perimeter = compact_shapes[arrival.p].front().perimeter;
    bool no_region = baseline.status == ArrivalStatus::NoRegion && !baseline.cells;
    bool noncompact = baseline.status == ArrivalStatus::Accepted && baseline.cells &&
                      baseline.perimeter > minimum_perimeter;
    if (!no_region && !noncompact) return nullopt;

    DeadlineLayerCpuScope cpu_scope(diagnostics);
    diagnostics.eligible++;
    diagnostics.no_region_eligible += no_region;
    diagnostics.noncompact_eligible += noncompact;
    if (free_cells_before < arrival.p + 1) {
        diagnostics.area_insufficient++;
        return nullopt;
    }

    if (no_region) {
        ll minimum_move_cost = numeric_limits<ll>::max();
        for (const GroupState &group : groups) {
            if (group.active) chmin(minimum_move_cost, move_cost(group.v, r_milli));
        }
        ll compact_fee = round_payment(arrival.v, arrival.p, minimum_perimeter);
        if (minimum_move_cost == numeric_limits<ll>::max() ||
            (long double)(compact_fee - minimum_move_cost) <= opportunity_cost) {
            diagnostics.economic_upper_bound_rejected++;
            return nullopt;
        }
    }

    if (diagnostics.graph_builds >= DEADLINE_GRAPH_BUILD_CASE_LIMIT ||
        diagnostics.closure_expansions >= DEADLINE_CLOSURE_EXPANSION_CASE_LIMIT ||
        diagnostics.layout_nodes >= DEADLINE_LAYOUT_NODE_CASE_LIMIT ||
        diagnostics.template_probes >= DEADLINE_TEMPLATE_PROBE_CASE_LIMIT ||
        diagnostics.growth_steps >= DEADLINE_GROWTH_STEP_CASE_LIMIT ||
        diagnostics.connectivity_calls >= DEADLINE_CONNECTIVITY_CALL_CASE_LIMIT ||
        diagnostics.connectivity_visits >= DEADLINE_CONNECTIVITY_VISIT_CASE_LIMIT ||
        diagnostics.complete_plan_attempts >= DEADLINE_COMPLETE_PLAN_CASE_LIMIT) {
        diagnostics.case_budget_skips++;
        return nullopt;
    }
    int window = min(3, turn * 4 / total_groups);
    array<int, 4> &mode_attempts =
        no_region ? diagnostics.no_region_attempts_by_window
                  : diagnostics.noncompact_attempts_by_window;
    if (mode_attempts[window] >= DEADLINE_WINDOW_ATTEMPT_LIMIT) {
        diagnostics.window_budget_skips++;
        return nullopt;
    }
    diagnostics.attempts++;
    diagnostics.attempts_by_window[window]++;
    mode_attempts[window]++;
    diagnostics.graph_builds++;

    optional<DeadlineQuotientGraph> graph = build_deadline_quotient_graph(park, owner, groups);
    if (!graph) {
        diagnostics.graph_failures++;
        return nullopt;
    }
    DeadlineTurnWork work;
    vector<DeadlineClosureState> closures = enumerate_deadline_closures(
        *graph, groups, arrival.p + 1, r_milli, work.closure_expansions, diagnostics);
    if (closures.empty()) {
        diagnostics.closure_failures++;
        return nullopt;
    }

    vector<PreparedDeadlineCandidate> candidates;
    for (const DeadlineClosureState &closure : closures) {
        optional<DeadlineWorkspace> workspace = materialize_deadline_workspace(
            closure, *graph, arrival.p, work, diagnostics);
        if (!workspace) continue;
        diagnostics.workspaces_searched++;
        vector<DeadlineLayoutResult> layouts = search_deadline_layout(
            *workspace, groups, arrival_id, r_milli, park.size(), compact_shapes,
            work, diagnostics);
        for (const DeadlineLayoutResult &layout : layouts) {
            // Do not let arrival-only layouts consume the limited number of
            // complete-plan validations reserved for real reconstructions.
            if (layout.moved_groups == 0) {
                diagnostics.zero_move_candidates_filtered++;
                continue;
            }
            if (work.complete_plans >= DEADLINE_COMPLETE_PLAN_TURN_LIMIT ||
                diagnostics.complete_plan_attempts >= DEADLINE_COMPLETE_PLAN_CASE_LIMIT) {
                diagnostics.complete_plan_limit_exhausted++;
                break;
            }
            work.complete_plans++;
            diagnostics.complete_plan_attempts++;
            optional<PreparedDeadlineCandidate> candidate = prepare_deadline_candidate(
                layout, park, owner, groups, arrival_id, r_milli, diagnostics);
            if (!candidate) break;
            // The prepared output is validated independently from the layout.
            if (candidate->moved_groups == 0) {
                diagnostics.direct_identity_errors++;
                continue;
            }
            if (no_region && (long double)candidate->direct_score <= opportunity_cost) {
                diagnostics.direct_gate_rejected++;
                break;
            }
            diagnostics.feasible_plans++;
            candidates.push_back(std::move(*candidate));
            break;
        }
        if (work.complete_plans >= DEADLINE_COMPLETE_PLAN_TURN_LIMIT ||
            diagnostics.complete_plan_attempts >= DEADLINE_COMPLETE_PLAN_CASE_LIMIT) {
            break;
        }
    }
    if (candidates.empty()) {
        diagnostics.layout_failures++;
        return nullopt;
    }
    diagnostics.feasible_turns++;
    sort(candidates.begin(), candidates.end(), [](const PreparedDeadlineCandidate &lhs,
                                                   const PreparedDeadlineCandidate &rhs) {
        if (lhs.direct_score != rhs.direct_score) return lhs.direct_score > rhs.direct_score;
        if (lhs.moved_groups != rhs.moved_groups) return lhs.moved_groups < rhs.moved_groups;
        return lhs.order < rhs.order;
    });
    if ((int)candidates.size() > DEADLINE_ROLLOUT_CANDIDATE_LIMIT) {
        candidates.resize(DEADLINE_ROLLOUT_CANDIDATE_LIMIT);
    }

    TurnPlan baseline_plan = make_arrival_plan(baseline);
    vvi baseline_owner = owner;
    if (baseline_plan.arrival) place_cells(baseline_owner, *baseline_plan.arrival, arrival_id);
    ll baseline_fee = baseline.status == ArrivalStatus::Accepted ? baseline.fee : 0;
    int selected = -1;
    i128 best_margin_twice = 0;
    array<ll, ROOT_SCREEN_SCENARIO_COUNT> selected_future{};

    if (remaining_groups == 0) {
        for (int index = 0; index < (int)candidates.size(); index++) {
            ll direct_delta = candidates[index].direct_score - baseline_fee;
            if ((i128)2 * direct_delta > best_margin_twice) {
                best_margin_twice = (i128)2 * direct_delta;
                selected = index;
            }
        }
    } else {
        RescueRolloutScenarios scenarios = make_rescue_rollout_scenarios(
            groups, arrival_id, current_s, remaining_groups, theta, theta_estimator);
        int expected_length = min(ROOT_SCREEN_ROLLOUT_LENGTH, remaining_groups);
        bool generation_ok = scenarios.complete &&
                             (int)scenarios.arrivals.size() == ROOT_SCREEN_SCENARIO_COUNT;
        if (generation_ok) {
            for (const auto &scenario : scenarios.arrivals) {
                if ((int)scenario.size() != expected_length) generation_ok = false;
            }
        }
        if (!generation_ok) {
            diagnostics.rollout_generation_failures++;
            return nullopt;
        }
        diagnostics.rollout_turns++;
        array<RescueRolloutOutcome, ROOT_SCREEN_SCENARIO_COUNT> baseline_outcomes;
        for (int scenario = 0; scenario < ROOT_SCREEN_SCENARIO_COUNT; scenario++) {
            baseline_outcomes[scenario] = evaluate_rescue_rollout_branch(
                park, baseline_owner, groups, arrival_id, baseline_plan,
                scenarios.arrivals[scenario], grass_cells, density_model,
                sampled_dlp_model, compact_shapes);
            diagnostics.rollout_policy_steps += scenarios.arrivals[scenario].size();
        }
        for (int index = 0; index < (int)candidates.size(); index++) {
            array<ll, ROOT_SCREEN_SCENARIO_COUNT> future_delta{};
            for (int scenario = 0; scenario < ROOT_SCREEN_SCENARIO_COUNT; scenario++) {
                RescueRolloutOutcome outcome = evaluate_rescue_rollout_branch(
                    park, candidates[index].final_owner, groups, arrival_id,
                    candidates[index].plan, scenarios.arrivals[scenario], grass_cells,
                    density_model, sampled_dlp_model, compact_shapes);
                diagnostics.rollout_policy_steps += scenarios.arrivals[scenario].size();
                future_delta[scenario] = outcome.fee - baseline_outcomes[scenario].fee;
            }
            ll direct_delta = candidates[index].direct_score - baseline_fee;
            i128 margin_twice = (i128)ROOT_SCREEN_SCENARIO_COUNT * direct_delta +
                                future_delta[0] + future_delta[1];
            if (margin_twice > best_margin_twice) {
                best_margin_twice = margin_twice;
                selected = index;
                selected_future = future_delta;
            }
        }
    }
    if (selected < 0) {
        diagnostics.screen_rejected++;
        return nullopt;
    }

    PreparedDeadlineCandidate &chosen = candidates[selected];
    if (remaining_groups > 0) {
        if (deadline_confirmations_used >= DEADLINE_CONFIRMATION_CASE_LIMIT) {
            diagnostics.confirmation_rejected++;
            return nullopt;
        }
        diagnostics.confirmation_attempts++;
        RootBranchView protected_branch{&baseline_plan, &baseline_owner, 0};
        RootBranchView challenger_branch{
            &chosen.plan, &chosen.final_owner, chosen.direct_score - baseline_fee};
        RescueDiagnostics confirmation_diagnostics;
        if (!confirm_root_override(
                park, groups, arrival_id, current_s, remaining_groups, theta,
                theta_estimator, grass_cells, density_model, sampled_dlp_model, compact_shapes,
                protected_branch, challenger_branch, deadline_confirmations_used,
                confirmation_diagnostics)) {
            diagnostics.rollout_policy_steps +=
                confirmation_diagnostics.root_confirmation_policy_steps;
            diagnostics.confirmation_rejected++;
            return nullopt;
        }
        diagnostics.rollout_policy_steps +=
            confirmation_diagnostics.root_confirmation_policy_steps;
    }

    ArrivalDecision selected_arrival = baseline;
    selected_arrival.status = ArrivalStatus::Accepted;
    selected_arrival.cells = *chosen.plan.arrival;
    selected_arrival.perimeter = chosen.plan.arrival_perimeter;
    selected_arrival.fee = chosen.arrival_fee;
    replace_selected_placement_success(
        selected_arrival.diagnostics,
        chosen.plan.arrival_perimeter == minimum_perimeter
            ? PlacementSource::MinimumTemplate
            : PlacementSource::ConnectedGrowth);

    diagnostics.adopted++;
    diagnostics.adopted_with_move += chosen.moved_groups > 0;
    diagnostics.adopted_from_no_region += no_region;
    diagnostics.moved_groups += chosen.moved_groups;
    diagnostics.moved_cells += chosen.moved_cells;
    diagnostics.arrival_fee += chosen.arrival_fee;
    diagnostics.movement_cost += chosen.movement_cost;
    diagnostics.relocation_fee_loss += chosen.fee_loss;
    diagnostics.direct_gain += chosen.direct_score;
    diagnostics.scenario_0_future_delta += selected_future[0];
    diagnostics.scenario_1_future_delta += selected_future[1];
    diagnostics.screen_margin += (long double)best_margin_twice / ROOT_SCREEN_SCENARIO_COUNT;
    if (chosen.arrival_fee - chosen.movement_cost - chosen.fee_loss != chosen.direct_score) {
        diagnostics.direct_identity_errors++;
    }
    return RootActionResult{std::move(chosen.plan), std::move(selected_arrival)};
}

bool validate_connected_region(const vector<Cell> &cells, int n) {
    if (cells.empty()) return false;
    vector<char> in_region(n * n, false);
    for (auto [x, y] : cells) {
        if (!inside(x, y, n, n)) return false;
        int cell = x * n + y;
        if (in_region[cell]) return false;
        in_region[cell] = true;
    }
    vector<char> visited(n * n, false);
    queue<Cell> que;
    que.push(cells.front());
    visited[cells.front().first * n + cells.front().second] = true;
    int reached = 0;
    constexpr int DX[4] = {-1, 1, 0, 0};
    constexpr int DY[4] = {0, 0, -1, 1};
    while (!que.empty()) {
        auto [x, y] = que.front();
        que.pop();
        reached++;
        for (int dir = 0; dir < 4; dir++) {
            int nx = x + DX[dir];
            int ny = y + DY[dir];
            if (!inside(nx, ny, n, n)) continue;
            int next = nx * n + ny;
            if (!in_region[next] || visited[next]) continue;
            visited[next] = true;
            que.emplace(nx, ny);
        }
    }
    return reached == (int)cells.size();
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
    cout << plan.moves.size() << '\n';
    for (const MovePlan &move : plan.moves) {
        cout << move.id << '\n';
        for (auto [x, y] : move.cells) {
            cout << x << ' ' << y << '\n';
        }
    }

    if (plan.arrival) {
        cout << "Yes\n";
        for (auto [x, y] : *plan.arrival) {
            cout << x << ' ' << y << '\n';
        }
    } else {
        cout << "No\n";
    }
    cout.flush();
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    RuntimeDiagnostics runtime_diagnostics;

    auto initial_input_wall_begin = RuntimeDiagnostics::WallClock::now();
    int N, M;
    ld R;
    cin >> N >> M >> R;
    int r_milli = (int)llroundl(R * 1000.0L);
    vs park(N);
    for (string &row : park) cin >> row;
    runtime_diagnostics.add_input(initial_input_wall_begin);

    auto preprocess_wall_begin = RuntimeDiagnostics::WallClock::now();
    clock_t preprocess_cpu_begin = clock();

    vector<vector<Shape>> compact_shapes(151);
    vector<vector<Shape>> all_shapes(151);
    for (int p = 4; p <= 150; p++) {
        all_shapes[p] = make_template_shapes(p, N);
        compact_shapes[p] = all_shapes[p];
        int minimum_perimeter = compact_shapes[p].front().perimeter;
        compact_shapes[p].erase(
            remove_if(compact_shapes[p].begin(), compact_shapes[p].end(), [&](const Shape &shape) {
                return shape.perimeter > minimum_perimeter + COMPACT_PERIMETER_MARGIN;
            }),
            compact_shapes[p].end());
    }
    DensityModel density_model(compact_shapes);
    SampledDlpShadowModel sampled_dlp_model;
    if constexpr (ENABLE_SAMPLED_DLP) sampled_dlp_model.initialize(compact_shapes);
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
    RescueDiagnostics rescue_diagnostics;
    DeadlineLayerDiagnostics deadline_diagnostics;
    LossDiagnostics loss_diagnostics;
    int root_confirmations_used = 0;
    int deadline_confirmations_used = 0;
    array<bool, 4> normal_root_window_used{};
    int occupied_cells = 0;
    runtime_diagnostics.add_preprocess(preprocess_wall_begin, preprocess_cpu_begin);

    auto static_geometry_wall_begin = RuntimeDiagnostics::WallClock::now();
    clock_t static_geometry_cpu_begin = clock();
    int static_largest_component = largest_free_component(park, owner);
    runtime_diagnostics.add_diagnostic(static_geometry_wall_begin, static_geometry_cpu_begin);

    for (int turn = 0; turn < M; turn++) {
        auto input_wall_begin = RuntimeDiagnostics::WallClock::now();
        int i, P;
        ll S, T, V;
        cin >> i >> S >> T >> P >> V;
        runtime_diagnostics.add_input(input_wall_begin);

        auto turn_wall_begin = RuntimeDiagnostics::WallClock::now();
        clock_t turn_cpu_begin = clock();

        groups[i].s = S;
        groups[i].t = T;
        groups[i].v = V;
        groups[i].p = P;
        theta_estimator.observe(T - S);
        int remaining_groups = M - i - 1;
        long double theta = theta_estimator.estimate(S, remaining_groups);

        // A departure at exactly S is still present by the problem's ordering;
        // only t < S is removed before this arrival is handled.
        while (!departures.empty() && departures.top().first < S) {
            int j = departures.top().second;
            departures.pop();
            if (!groups[j].active) continue;
            clear_cells(owner, groups[j].cells);
            occupied_cells -= groups[j].p;
            groups[j].cells.clear();
            groups[j].active = false;
        }
        int free_cells_before = grass_cells - occupied_cells;

        ShadowEvaluation shadow;
        if constexpr (ENABLE_SAMPLED_DLP) {
            shadow = sampled_dlp_model.evaluate_real_turn(
                turn, S, T, P, remaining_groups, groups, grass_cells, theta_estimator);
        } else {
            shadow = evaluate_shadow_cost(groups, S, T, P, remaining_groups,
                                          grass_cells, theta, density_model);
        }
        shadow_diagnostics.considered++;
        shadow_diagnostics.theta_sum += theta;
        shadow_diagnostics.opportunity_cost_sum += shadow.opportunity_cost;
        shadow_diagnostics.rejected_fraction_sum += shadow.duration_weighted_rejected_fraction;
        chmax(shadow_diagnostics.maximum_rejected_fraction, shadow.maximum_rejected_fraction);
        shadow_diagnostics.priced_buckets += shadow.priced_buckets;

        // The ordinary primary remains the protected counterfactual.  Rescue
        // roots are screened first; a small deterministic set of non-compact
        // normal turns may use the same root machinery without relocation.
        vector<NormalPlacementChoice> baseline_alternatives;
        ArrivalDecision baseline_arrival = evaluate_arrival_decision(
            park, owner, groups, i, S, remaining_groups, theta, shadow.opportunity_cost,
            compact_shapes, &baseline_alternatives);
        bool rescue_root_screen_evaluated = false;
        optional<RootActionResult> expanded_action = choose_deadline_layer_root(
            park, owner, groups, i, turn, M, S, remaining_groups, free_cells_before,
            r_milli, theta, theta_estimator, density_model, sampled_dlp_model, grass_cells,
            shadow.opportunity_cost, baseline_arrival, compact_shapes,
            deadline_confirmations_used, deadline_diagnostics);
        if (!expanded_action) {
            expanded_action = choose_root_action_with_rescue(
                park, owner, groups, i, S, remaining_groups, r_milli, theta,
                theta_estimator, density_model, sampled_dlp_model,
                grass_cells, shadow.opportunity_cost,
                baseline_arrival, baseline_alternatives, compact_shapes, all_shapes,
                root_confirmations_used, rescue_root_screen_evaluated, rescue_diagnostics);
        }

        int minimum_perimeter = compact_shapes[P].front().perimeter;
        int normal_root_window = min(3, turn * 4 / M);
        bool normal_root_gate =
            !ROOT_PROTECTED_ONLY && !expanded_action && !rescue_root_screen_evaluated && remaining_groups > 0 &&
            baseline_arrival.status == ArrivalStatus::Accepted && baseline_arrival.cells &&
            baseline_arrival.perimeter > minimum_perimeter && !baseline_alternatives.empty() &&
            !normal_root_window_used[normal_root_window];
        if (normal_root_gate) {
            normal_root_window_used[normal_root_window] = true;
            rescue_diagnostics.normal_root_gate_turns++;
            expanded_action = choose_normal_root_action(
                park, owner, groups, i, S, remaining_groups, theta, theta_estimator,
                density_model, sampled_dlp_model, grass_cells, baseline_arrival,
                baseline_alternatives, compact_shapes,
                root_confirmations_used, rescue_diagnostics);
        }

        TurnPlan plan;
        ArrivalDecision selected_arrival;
        if (expanded_action) {
            plan = std::move(expanded_action->plan);
            selected_arrival = std::move(expanded_action->arrival_decision);
        } else {
            plan = make_arrival_plan(baseline_arrival);
            selected_arrival = std::move(baseline_arrival);
        }
        accumulate_placement_diagnostics(placement_diagnostics, selected_arrival.diagnostics);
        switch (selected_arrival.status) {
            case ArrivalStatus::UpperBoundRejected:
                shadow_diagnostics.upper_bound_rejected++;
                break;
            case ArrivalStatus::NoRegion:
                shadow_diagnostics.no_region_rejected++;
                break;
            case ArrivalStatus::ActualFeeRejected:
                shadow_diagnostics.actual_fee_rejected++;
                break;
            case ArrivalStatus::Accepted:
                shadow_diagnostics.accepted++;
                break;
        }

        ll turn_movement_cost = 0;
        ll turn_relocation_fee_loss = 0;
        for (const MovePlan &move : plan.moves) {
            const GroupState &group = groups[move.id];
            int next_max_perimeter = max(group.max_perimeter, move.perimeter);
            turn_movement_cost += move_cost(group.v, r_milli);
            turn_relocation_fee_loss +=
                round_payment(group.v, group.p, group.max_perimeter) -
                round_payment(group.v, group.p, next_max_perimeter);
        }

        apply_plan(i, plan, owner, groups);
        if (plan.arrival) {
            departures.emplace(T, i);
            accepted_count++;
            occupied_cells += P;
        } else {
            rejected_count++;
        }
        runtime_diagnostics.add_turn(turn_wall_begin, turn_cpu_begin);

        auto loss_wall_begin = RuntimeDiagnostics::WallClock::now();
        clock_t loss_cpu_begin = clock();
        int reject_largest_component = -1;
        if (!plan.arrival) {
            reject_largest_component = largest_free_component(park, owner);
        }
        observe_loss(loss_diagnostics, selected_arrival, plan, groups[i], minimum_perimeter,
                     free_cells_before, static_largest_component, reject_largest_component,
                     turn_movement_cost, turn_relocation_fee_loss, shadow.opportunity_cost);
        runtime_diagnostics.add_diagnostic(loss_wall_begin, loss_cpu_begin);

        auto output_wall_begin = RuntimeDiagnostics::WallClock::now();
        emit_plan(plan);
        runtime_diagnostics.add_output(output_wall_begin);
    }

    auto final_loss_wall_begin = RuntimeDiagnostics::WallClock::now();
    clock_t final_loss_cpu_begin = clock();
    finalize_loss_diagnostics(loss_diagnostics, groups);

    long double mean_theta =
        shadow_diagnostics.considered == 0 ? 0.0L : shadow_diagnostics.theta_sum / shadow_diagnostics.considered;
    long double mean_opportunity_cost = shadow_diagnostics.considered == 0
                                            ? 0.0L
                                            : shadow_diagnostics.opportunity_cost_sum / shadow_diagnostics.considered;
    long double mean_rejected_fraction = shadow_diagnostics.considered == 0
                                             ? 0.0L
                                             : shadow_diagnostics.rejected_fraction_sum / shadow_diagnostics.considered;
    ll reconstructed_raw_score =
        loss_diagnostics.accepted_final_fee - loss_diagnostics.movement_cost_paid;
    ll reconstructed_absolute_score = max(0LL, reconstructed_raw_score);
    ll accepted_initial_identity_error =
        loss_diagnostics.accepted_ideal_fee - loss_diagnostics.accepted_initial_fee -
        loss_diagnostics.accepted_initial_shape_loss;
    ll accepted_final_identity_error =
        loss_diagnostics.accepted_initial_fee - loss_diagnostics.accepted_final_fee -
        loss_diagnostics.accepted_relocation_fee_loss;
    ll offered_identity_error =
        loss_diagnostics.offered_ideal_fee - loss_diagnostics.accepted_ideal_fee -
        loss_diagnostics.rejected_ideal_fee;
    ll cell_time_identity_error =
        loss_diagnostics.offered_cell_time - loss_diagnostics.accepted_cell_time -
        loss_diagnostics.rejected_cell_time;
    ll gap_identity_error =
        loss_diagnostics.offered_ideal_fee - reconstructed_raw_score -
        loss_diagnostics.rejected_ideal_fee - loss_diagnostics.accepted_initial_shape_loss -
        loss_diagnostics.accepted_relocation_fee_loss - loss_diagnostics.movement_cost_paid;
    int observed_count_error = loss_diagnostics.observed - M;
    int total_count_partition_error =
        loss_diagnostics.observed - loss_diagnostics.accepted -
        loss_diagnostics.rejected_feasible - loss_diagnostics.rejected_unplaceable;
    int accepted_count_error = accepted_count - loss_diagnostics.accepted;
    int finalized_count_error =
        loss_diagnostics.accepted - loss_diagnostics.finalized_accepted;
    int rejected_count_error =
        rejected_count - loss_diagnostics.rejected_feasible -
        loss_diagnostics.rejected_unplaceable;
    int rejected_status_count_error =
        rejected_count - loss_diagnostics.upper_rejected - loss_diagnostics.actual_rejected -
        loss_diagnostics.no_region_rejected - loss_diagnostics.rejected_status_mismatch;
    int upper_count_partition_error =
        loss_diagnostics.upper_rejected - loss_diagnostics.upper_rejected_feasible -
        loss_diagnostics.upper_rejected_unplaceable;
    int unplaceable_count_partition_error =
        loss_diagnostics.rejected_unplaceable - loss_diagnostics.unplaceable_static -
        loss_diagnostics.unplaceable_capacity - loss_diagnostics.unplaceable_fragmentation;
    int accepted_source_count_error =
        loss_diagnostics.accepted -
        accumulate(loss_diagnostics.accepted_by_source.begin(),
                   loss_diagnostics.accepted_by_source.end(), 0);
    ll rejected_fee_partition_error =
        loss_diagnostics.rejected_ideal_fee - loss_diagnostics.rejected_feasible_ideal_fee -
        loss_diagnostics.rejected_unplaceable_ideal_fee;
    ll rejected_cell_time_partition_error =
        loss_diagnostics.rejected_cell_time - loss_diagnostics.rejected_feasible_cell_time -
        loss_diagnostics.rejected_unplaceable_cell_time;
    ll rejected_status_fee_error =
        loss_diagnostics.rejected_ideal_fee - loss_diagnostics.upper_rejected_ideal_fee -
        loss_diagnostics.actual_rejected_ideal_fee - loss_diagnostics.no_region_ideal_fee -
        loss_diagnostics.rejected_status_mismatch_ideal_fee;
    ll rejected_status_cell_time_error =
        loss_diagnostics.rejected_cell_time - loss_diagnostics.upper_rejected_cell_time -
        loss_diagnostics.actual_rejected_cell_time - loss_diagnostics.no_region_cell_time -
        loss_diagnostics.rejected_status_mismatch_cell_time;
    ll unplaceable_fee_partition_error =
        loss_diagnostics.rejected_unplaceable_ideal_fee -
        loss_diagnostics.unplaceable_static_ideal_fee -
        loss_diagnostics.unplaceable_capacity_ideal_fee -
        loss_diagnostics.unplaceable_fragmentation_ideal_fee;
    ll unplaceable_cell_time_partition_error =
        loss_diagnostics.rejected_unplaceable_cell_time -
        loss_diagnostics.unplaceable_static_cell_time -
        loss_diagnostics.unplaceable_capacity_cell_time -
        loss_diagnostics.unplaceable_fragmentation_cell_time;
    ll accepted_source_ideal_fee_error =
        loss_diagnostics.accepted_ideal_fee -
        accumulate(loss_diagnostics.accepted_source_ideal_fee.begin(),
                   loss_diagnostics.accepted_source_ideal_fee.end(), 0LL);
    ll accepted_source_initial_fee_error =
        loss_diagnostics.accepted_initial_fee -
        accumulate(loss_diagnostics.accepted_source_initial_fee.begin(),
                   loss_diagnostics.accepted_source_initial_fee.end(), 0LL);
    ll accepted_source_perimeter_error =
        loss_diagnostics.accepted_perimeter_excess -
        accumulate(loss_diagnostics.accepted_source_perimeter_excess.begin(),
                   loss_diagnostics.accepted_source_perimeter_excess.end(), 0LL);
    ll actual_candidate_fee_identity_error =
        placement_diagnostics.actual_rejected_candidate_fee -
        loss_diagnostics.actual_rejected_candidate_fee;
    long long grow_and_trim_growth_funnel_error =
        placement_diagnostics.grow_and_trim_base_candidates -
        placement_diagnostics.grow_and_trim_growth_failures -
        placement_diagnostics.grow_and_trim_full_growths;
    long long grow_and_trim_completion_funnel_error =
        placement_diagnostics.grow_and_trim_full_growths -
        placement_diagnostics.grow_and_trim_trim_failures -
        placement_diagnostics.grow_and_trim_duplicate_candidates -
        placement_diagnostics.grow_and_trim_candidates;
    long long grow_and_trim_perimeter_partition_error =
        placement_diagnostics.grow_and_trim_full_growths -
        placement_diagnostics.grow_and_trim_trim_failures -
        placement_diagnostics.grow_and_trim_perimeter_improved_candidates -
        placement_diagnostics.grow_and_trim_perimeter_equal_candidates -
        placement_diagnostics.grow_and_trim_perimeter_worsened_candidates;
    int grow_and_trim_source_error =
        max(0, loss_diagnostics.accepted_grow_and_trim -
                   loss_diagnostics.accepted_by_source[2]);
    int pushout_status_identity_error =
        ENABLE_NO_REGION_PUSHOUT
            ? rescue_diagnostics.pushout_eligible - shadow_diagnostics.no_region_rejected -
                  rescue_diagnostics.pushout_adopted
            : 0;
    int pushout_feasible_histogram_error =
        rescue_diagnostics.pushout_feasible_plans -
        accumulate(rescue_diagnostics.pushout_feasible_by_blocker_count.begin(),
                   rescue_diagnostics.pushout_feasible_by_blocker_count.end(), 0);
    int pushout_adopted_histogram_error =
        rescue_diagnostics.pushout_adopted -
        accumulate(rescue_diagnostics.pushout_adopted_by_blocker_count.begin(),
                   rescue_diagnostics.pushout_adopted_by_blocker_count.end(), 0);
    int pushout_funnel_identity_error =
        rescue_diagnostics.pushout_eligible - rescue_diagnostics.pushout_area_insufficient -
        rescue_diagnostics.pushout_no_economic_target - rescue_diagnostics.pushout_no_repair -
        rescue_diagnostics.pushout_screen_rejected - rescue_diagnostics.pushout_adopted;
    ll pushout_direct_identity_error =
        rescue_diagnostics.pushout_arrival_fee - rescue_diagnostics.pushout_movement_cost -
        rescue_diagnostics.pushout_relocation_fee_loss - rescue_diagnostics.pushout_direct_gain;
    int pushout_helper_turn_funnel_error =
        rescue_diagnostics.pushout_helper_considered_turns -
        rescue_diagnostics.pushout_helper_no_eligible_target_turns -
        rescue_diagnostics.pushout_helper_no_evidence_turns -
        rescue_diagnostics.pushout_helper_economic_rejected_turns -
        rescue_diagnostics.pushout_helper_seeded_turns;
    int pushout_helper_attempt_funnel_error =
        rescue_diagnostics.pushout_helper_attempts -
        rescue_diagnostics.pushout_helper_missing_destination -
        rescue_diagnostics.pushout_helper_repair_failures -
        rescue_diagnostics.pushout_helper_validation_failures -
        rescue_diagnostics.pushout_helper_duplicate_plans -
        rescue_diagnostics.pushout_helper_feasible_plans;
    int pushout_helper_missing_partition_error =
        rescue_diagnostics.pushout_helper_missing_destination -
        rescue_diagnostics.pushout_helper_missing_blocker_destination -
        rescue_diagnostics.pushout_helper_missing_helper_destination;
    int pushout_helper_feasible_funnel_error =
        rescue_diagnostics.pushout_helper_feasible_plans -
        rescue_diagnostics.pushout_helper_screen_rejected -
        rescue_diagnostics.pushout_helper_adopted;
    int pushout_helper_feasible_histogram_error =
        rescue_diagnostics.pushout_helper_feasible_plans -
        accumulate(rescue_diagnostics.pushout_helper_feasible_by_blocker_count.begin(),
                   rescue_diagnostics.pushout_helper_feasible_by_blocker_count.end(), 0);
    int pushout_helper_adopted_histogram_error =
        rescue_diagnostics.pushout_helper_adopted -
        accumulate(rescue_diagnostics.pushout_helper_adopted_by_blocker_count.begin(),
                   rescue_diagnostics.pushout_helper_adopted_by_blocker_count.end(), 0);
    ll pushout_helper_direct_identity_error =
        rescue_diagnostics.pushout_helper_adopted_arrival_fee -
        rescue_diagnostics.pushout_helper_adopted_movement_cost -
        rescue_diagnostics.pushout_helper_adopted_direct_gain;
    int pushout_helper_work_cap_error =
        (rescue_diagnostics.pushout_helper_attempts >
         PUSHOUT_HELPER_REPAIR_LIMIT *
             rescue_diagnostics.pushout_helper_seeded_turns) +
        (rescue_diagnostics.pushout_helper_surveyed_targets >
         PUSHOUT_TARGET_REPAIR_LIMIT *
             rescue_diagnostics.pushout_helper_surveyed_turns) +
        (rescue_diagnostics.pushout_helper_obstruction_probes >
         (long long)PUSHOUT_HELPER_OBSTRUCTION_PROBE_LIMIT *
             rescue_diagnostics.pushout_helper_surveyed_targets) +
        (rescue_diagnostics.pushout_helper_destination_anchors >
         (long long)PUSHOUT_HELPER_DESTINATION_ANCHOR_GLOBAL_LIMIT *
             rescue_diagnostics.pushout_helper_seeded_turns) +
        (rescue_diagnostics.pushout_helper_beam_nodes >
         (long long)PUSHOUT_HELPER_REPAIR_NODE_LIMIT *
             rescue_diagnostics.pushout_helper_seeded_turns) +
        (rescue_diagnostics.pushout_helper_feasible_plans >
         (long long)PUSHOUT_HELPER_FEASIBLE_LIMIT *
             rescue_diagnostics.pushout_helper_seeded_turns) +
        (rescue_diagnostics.pushout_helper_shortlisted_choices >
         (long long)PUSHOUT_HELPER_CHOICE_LIMIT_PER_TARGET *
             rescue_diagnostics.pushout_helper_surveyed_targets) +
        (rescue_diagnostics.pushout_helper_two_feasible_turns >
         rescue_diagnostics.pushout_helper_seeded_turns) +
        (rescue_diagnostics.pushout_helper_maximum_movers >
         PUSHOUT_HELPER_MAX_BLOCKERS + 1);
#ifdef AHC069_DISABLE_PUSHOUT_HELPER
    bool any_pushout_helper_diagnostic =
        rescue_diagnostics.pushout_helper_considered_turns != 0 ||
        rescue_diagnostics.pushout_helper_no_eligible_target_turns != 0 ||
        rescue_diagnostics.pushout_helper_no_evidence_turns != 0 ||
        rescue_diagnostics.pushout_helper_economic_rejected_turns != 0 ||
        rescue_diagnostics.pushout_helper_seeded_turns != 0 ||
        rescue_diagnostics.pushout_helper_attempts != 0 ||
        rescue_diagnostics.pushout_helper_missing_blocker_destination != 0 ||
        rescue_diagnostics.pushout_helper_missing_helper_destination != 0 ||
        rescue_diagnostics.pushout_helper_feasible_plans != 0 ||
        rescue_diagnostics.pushout_helper_screen_rejected != 0 ||
        rescue_diagnostics.pushout_helper_adopted != 0 ||
        rescue_diagnostics.pushout_helper_surveyed_targets != 0 ||
        rescue_diagnostics.pushout_helper_large_blocker_targets != 0 ||
        rescue_diagnostics.pushout_helper_feasible_limit_exhausted != 0 ||
        rescue_diagnostics.pushout_helper_two_feasible_turns != 0 ||
        rescue_diagnostics.pushout_helper_obstruction_probes != 0 ||
        rescue_diagnostics.pushout_helper_single_owner_regions != 0 ||
        rescue_diagnostics.pushout_helper_evidenced_groups != 0 ||
        rescue_diagnostics.pushout_helper_recorded_witnesses != 0 ||
        rescue_diagnostics.pushout_helper_shortlisted_choices != 0 ||
        rescue_diagnostics.pushout_helper_destination_anchors != 0 ||
        rescue_diagnostics.pushout_helper_destination_candidates != 0 ||
        rescue_diagnostics.pushout_helper_foreign_destination_candidates != 0 ||
        rescue_diagnostics.pushout_helper_retained_foreign_destinations != 0 ||
        rescue_diagnostics.pushout_helper_forced_witness_destinations != 0 ||
        rescue_diagnostics.pushout_helper_beam_nodes != 0 ||
        rescue_diagnostics.pushout_helper_adopted_arrival_fee != 0 ||
        rescue_diagnostics.pushout_helper_adopted_movement_cost != 0 ||
        rescue_diagnostics.pushout_helper_adopted_direct_gain != 0 ||
        rescue_diagnostics.pushout_helper_phase_cpu_seconds != 0.0;
    int pushout_helper_disabled_error = any_pushout_helper_diagnostic;
#else
    int pushout_helper_disabled_error = 0;
#endif
    ll deadline_direct_identity_error =
        deadline_diagnostics.arrival_fee - deadline_diagnostics.movement_cost -
        deadline_diagnostics.relocation_fee_loss - deadline_diagnostics.direct_gain;
    int deadline_funnel_identity_error =
        deadline_diagnostics.eligible - deadline_diagnostics.area_insufficient -
        deadline_diagnostics.economic_upper_bound_rejected -
        deadline_diagnostics.case_budget_skips - deadline_diagnostics.window_budget_skips -
        deadline_diagnostics.graph_failures - deadline_diagnostics.closure_failures -
        deadline_diagnostics.layout_failures -
        deadline_diagnostics.rollout_generation_failures - deadline_diagnostics.screen_rejected -
        deadline_diagnostics.confirmation_rejected - deadline_diagnostics.adopted;
    int deadline_no_region_status_identity_error =
        ENABLE_DEADLINE_LAYER
            ? deadline_diagnostics.no_region_eligible -
                  deadline_diagnostics.adopted_from_no_region -
                  rescue_diagnostics.pushout_adopted - shadow_diagnostics.no_region_rejected
            : 0;
    int deadline_adopted_move_identity_error =
        deadline_diagnostics.adopted - deadline_diagnostics.adopted_with_move;
    int deadline_work_cap_error =
        (deadline_diagnostics.graph_builds > DEADLINE_GRAPH_BUILD_CASE_LIMIT) +
        (deadline_diagnostics.closure_expansions > DEADLINE_CLOSURE_EXPANSION_CASE_LIMIT) +
        (deadline_diagnostics.layout_nodes > DEADLINE_LAYOUT_NODE_CASE_LIMIT) +
        (deadline_diagnostics.template_probes > DEADLINE_TEMPLATE_PROBE_CASE_LIMIT) +
        (deadline_diagnostics.growth_steps > DEADLINE_GROWTH_STEP_CASE_LIMIT) +
        (deadline_diagnostics.connectivity_calls > DEADLINE_CONNECTIVITY_CALL_CASE_LIMIT) +
        (deadline_diagnostics.connectivity_visits > DEADLINE_CONNECTIVITY_VISIT_CASE_LIMIT) +
        (deadline_diagnostics.complete_plan_attempts > DEADLINE_COMPLETE_PLAN_CASE_LIMIT);
    const SampledDlpDiagnostics &dlp_diagnostics = sampled_dlp_model.diagnostics;
    long long sampled_dlp_request_count_error =
        ENABLE_SAMPLED_DLP
            ? dlp_diagnostics.generated_requests -
                  (long long)dlp_diagnostics.rebuilds * SAMPLED_DLP_REQUEST_COUNT
            : dlp_diagnostics.generated_requests;
    int sampled_dlp_trigger_partition_error =
        dlp_diagnostics.rebuilds - dlp_diagnostics.initial_rebuilds -
        dlp_diagnostics.scheduled_rebuilds - dlp_diagnostics.boundary_rebuilds;
    int sampled_dlp_real_call_error =
        ENABLE_SAMPLED_DLP ? dlp_diagnostics.real_price_calls - M
                           : dlp_diagnostics.real_price_calls;
    long long sampled_dlp_expected_rollout_calls =
        rescue_diagnostics.rollout_policy_steps +
        rescue_diagnostics.normal_root_policy_steps +
        rescue_diagnostics.root_confirmation_policy_steps +
        deadline_diagnostics.rollout_policy_steps;
    long long sampled_dlp_rollout_call_error =
        ENABLE_SAMPLED_DLP
            ? dlp_diagnostics.rollout_price_calls - sampled_dlp_expected_rollout_calls
            : dlp_diagnostics.rollout_price_calls;
    runtime_diagnostics.add_diagnostic(final_loss_wall_begin, final_loss_cpu_begin);
    RuntimeSnapshot runtime = snapshot_runtime(runtime_diagnostics);
    cerr << "accepted=" << accepted_count << " rejected=" << rejected_count
         << " deadline_enabled=" << ENABLE_DEADLINE_LAYER
         << " deadline_eligible=" << deadline_diagnostics.eligible
         << " deadline_no_region_eligible=" << deadline_diagnostics.no_region_eligible
         << " deadline_noncompact_eligible=" << deadline_diagnostics.noncompact_eligible
         << " deadline_area_insufficient=" << deadline_diagnostics.area_insufficient
         << " deadline_economic_ub_rejected="
         << deadline_diagnostics.economic_upper_bound_rejected
         << " deadline_case_budget_skips=" << deadline_diagnostics.case_budget_skips
         << " deadline_window_budget_skips=" << deadline_diagnostics.window_budget_skips
         << " deadline_attempts=" << deadline_diagnostics.attempts
         << " deadline_graph_builds=" << deadline_diagnostics.graph_builds
         << " deadline_graph_failures=" << deadline_diagnostics.graph_failures
         << " deadline_closure_failures=" << deadline_diagnostics.closure_failures
         << " deadline_workspaces=" << deadline_diagnostics.workspaces_searched
         << " deadline_layout_failures=" << deadline_diagnostics.layout_failures
         << " deadline_validation_failures=" << deadline_diagnostics.validation_failures
         << " deadline_feasible_turns=" << deadline_diagnostics.feasible_turns
         << " deadline_feasible_plans=" << deadline_diagnostics.feasible_plans
         << " deadline_complete_plan_attempts="
         << deadline_diagnostics.complete_plan_attempts
         << " deadline_zero_move_candidates_filtered="
         << deadline_diagnostics.zero_move_candidates_filtered
         << " deadline_direct_gate_rejected=" << deadline_diagnostics.direct_gate_rejected
         << " deadline_rollout_generation_failures="
         << deadline_diagnostics.rollout_generation_failures
         << " deadline_rollout_turns=" << deadline_diagnostics.rollout_turns
         << " deadline_screen_rejected=" << deadline_diagnostics.screen_rejected
         << " deadline_confirmation_rejected=" << deadline_diagnostics.confirmation_rejected
         << " deadline_confirmation_attempts=" << deadline_diagnostics.confirmation_attempts
         << " deadline_confirmation_used=" << deadline_confirmations_used
         << " deadline_adopted=" << deadline_diagnostics.adopted
         << " deadline_adopted_with_move=" << deadline_diagnostics.adopted_with_move
         << " deadline_adopted_from_no_region=" << deadline_diagnostics.adopted_from_no_region
         << " deadline_closure_limit_exhausted="
         << deadline_diagnostics.closure_limit_exhausted
         << " deadline_layout_limit_exhausted="
         << deadline_diagnostics.layout_limit_exhausted
         << " deadline_template_limit_exhausted="
         << deadline_diagnostics.template_limit_exhausted
         << " deadline_growth_limit_exhausted="
         << deadline_diagnostics.growth_limit_exhausted
         << " deadline_connectivity_limit_exhausted="
         << deadline_diagnostics.connectivity_limit_exhausted
         << " deadline_complete_plan_limit_exhausted="
         << deadline_diagnostics.complete_plan_limit_exhausted
         << " deadline_closure_expansions=" << deadline_diagnostics.closure_expansions
         << " deadline_closure_states=" << deadline_diagnostics.closure_states
         << " deadline_completed_closures=" << deadline_diagnostics.completed_closures
         << " deadline_global_closures=" << deadline_diagnostics.global_closures
         << " deadline_layout_nodes=" << deadline_diagnostics.layout_nodes
         << " deadline_region_candidates=" << deadline_diagnostics.region_candidates
         << " deadline_template_probes=" << deadline_diagnostics.template_probes
         << " deadline_growth_steps=" << deadline_diagnostics.growth_steps
         << " deadline_connectivity_visits=" << deadline_diagnostics.connectivity_visits
         << " deadline_connectivity_calls=" << deadline_diagnostics.connectivity_calls
         << " deadline_rollout_policy_steps=" << deadline_diagnostics.rollout_policy_steps
         << " deadline_moved_groups=" << deadline_diagnostics.moved_groups
         << " deadline_moved_cells=" << deadline_diagnostics.moved_cells
         << " deadline_arrival_fee=" << deadline_diagnostics.arrival_fee
         << " deadline_movement_cost=" << deadline_diagnostics.movement_cost
         << " deadline_relocation_fee_loss=" << deadline_diagnostics.relocation_fee_loss
         << " deadline_direct_gain=" << deadline_diagnostics.direct_gain
         << " deadline_scenario_0_future_delta="
         << deadline_diagnostics.scenario_0_future_delta
         << " deadline_scenario_1_future_delta="
         << deadline_diagnostics.scenario_1_future_delta
         << " deadline_screen_margin=" << fixed << setprecision(6)
         << (double)deadline_diagnostics.screen_margin
         << " deadline_cpu_ms=" << 1000.0 * deadline_diagnostics.cpu_seconds
         << " deadline_maximum_turn_cpu_ms="
         << 1000.0 * deadline_diagnostics.maximum_turn_cpu_seconds
         << " deadline_partition_errors=" << deadline_diagnostics.partition_errors
         << " deadline_prefix_connectivity_errors="
         << deadline_diagnostics.prefix_connectivity_errors
         << " deadline_direct_identity_errors=" << deadline_diagnostics.direct_identity_errors
         << " deadline_direct_identity_error=" << deadline_direct_identity_error
         << " deadline_funnel_identity_error=" << deadline_funnel_identity_error
         << " deadline_no_region_status_identity_error="
         << deadline_no_region_status_identity_error
         << " deadline_adopted_move_identity_error="
         << deadline_adopted_move_identity_error
         << " deadline_work_cap_error=" << deadline_work_cap_error
         << " pushout_enabled=" << ENABLE_NO_REGION_PUSHOUT
         << " pushout_eligible=" << rescue_diagnostics.pushout_eligible
         << " pushout_area_insufficient=" << rescue_diagnostics.pushout_area_insufficient
         << " pushout_shadow_filtered_targets="
         << rescue_diagnostics.pushout_shadow_filtered_targets
         << " pushout_no_economic_target=" << rescue_diagnostics.pushout_no_economic_target
         << " pushout_feasible_turns=" << rescue_diagnostics.pushout_feasible_turns
         << " pushout_feasible_plans=" << rescue_diagnostics.pushout_feasible_plans
         << " pushout_no_repair=" << rescue_diagnostics.pushout_no_repair
         << " pushout_rollout_generation_failures="
         << rescue_diagnostics.pushout_rollout_generation_failures
         << " pushout_rollout_turns=" << rescue_diagnostics.pushout_rollout_turns
         << " pushout_screen_rejected=" << rescue_diagnostics.pushout_screen_rejected
         << " pushout_adopted=" << rescue_diagnostics.pushout_adopted
         << " pushout_target_limit_exhausted="
         << rescue_diagnostics.pushout_target_limit_exhausted
         << " pushout_destination_limit_exhausted="
         << rescue_diagnostics.pushout_destination_limit_exhausted
         << " pushout_node_limit_exhausted="
         << rescue_diagnostics.pushout_node_limit_exhausted
         << " pushout_maximum_blockers=" << rescue_diagnostics.pushout_maximum_blockers
         << " pushout_feasible_1_blocker="
         << rescue_diagnostics.pushout_feasible_by_blocker_count[0]
         << " pushout_feasible_2_blockers="
         << rescue_diagnostics.pushout_feasible_by_blocker_count[1]
         << " pushout_feasible_3_blockers="
         << rescue_diagnostics.pushout_feasible_by_blocker_count[2]
         << " pushout_feasible_4plus_blockers="
         << rescue_diagnostics.pushout_feasible_by_blocker_count[3]
         << " pushout_adopted_1_blocker="
         << rescue_diagnostics.pushout_adopted_by_blocker_count[0]
         << " pushout_adopted_2_blockers="
         << rescue_diagnostics.pushout_adopted_by_blocker_count[1]
         << " pushout_adopted_3_blockers="
         << rescue_diagnostics.pushout_adopted_by_blocker_count[2]
         << " pushout_adopted_4plus_blockers="
         << rescue_diagnostics.pushout_adopted_by_blocker_count[3]
         << " pushout_target_anchors=" << rescue_diagnostics.pushout_target_anchors
         << " pushout_target_shortlisted=" << rescue_diagnostics.pushout_target_shortlisted
         << " pushout_exact_targets=" << rescue_diagnostics.pushout_exact_targets
         << " pushout_economic_targets=" << rescue_diagnostics.pushout_economic_targets
         << " pushout_repair_attempts=" << rescue_diagnostics.pushout_repair_attempts
         << " pushout_destination_anchors=" << rescue_diagnostics.pushout_destination_anchors
         << " pushout_destination_candidates="
         << rescue_diagnostics.pushout_destination_candidates
         << " pushout_beam_nodes=" << rescue_diagnostics.pushout_beam_nodes
         << " pushout_rollout_policy_steps=" << rescue_diagnostics.pushout_rollout_policy_steps
         << " pushout_moved_groups=" << rescue_diagnostics.pushout_moved_groups
         << " pushout_moved_cells=" << rescue_diagnostics.pushout_moved_cells
         << " pushout_arrival_fee=" << rescue_diagnostics.pushout_arrival_fee
         << " pushout_movement_cost=" << rescue_diagnostics.pushout_movement_cost
         << " pushout_relocation_fee_loss=" << rescue_diagnostics.pushout_relocation_fee_loss
         << " pushout_direct_gain=" << rescue_diagnostics.pushout_direct_gain
         << " pushout_scenario_0_future_delta="
         << rescue_diagnostics.pushout_scenario_0_future_delta
         << " pushout_scenario_1_future_delta="
         << rescue_diagnostics.pushout_scenario_1_future_delta
         << " pushout_screen_margin=" << fixed << setprecision(6)
         << (double)rescue_diagnostics.pushout_screen_margin
         << " pushout_cpu_ms=" << 1000.0 * rescue_diagnostics.pushout_cpu_seconds
         << " pushout_maximum_turn_cpu_ms="
         << 1000.0 * rescue_diagnostics.pushout_maximum_turn_cpu_seconds
         << " pushout_helper_enabled=" << ENABLE_PUSHOUT_HELPER
         << " pushout_helper_wide_enabled=" << ENABLE_WIDE_PUSHOUT_HELPER
         << " pushout_helper_considered_turns="
         << rescue_diagnostics.pushout_helper_considered_turns
         << " pushout_helper_no_eligible_target_turns="
         << rescue_diagnostics.pushout_helper_no_eligible_target_turns
         << " pushout_helper_no_evidence_turns="
         << rescue_diagnostics.pushout_helper_no_evidence_turns
         << " pushout_helper_economic_rejected_turns="
         << rescue_diagnostics.pushout_helper_economic_rejected_turns
         << " pushout_helper_seeded_turns="
         << rescue_diagnostics.pushout_helper_seeded_turns
         << " pushout_helper_attempts=" << rescue_diagnostics.pushout_helper_attempts
         << " pushout_helper_missing_destination="
         << rescue_diagnostics.pushout_helper_missing_destination
         << " pushout_helper_missing_blocker_destination="
         << rescue_diagnostics.pushout_helper_missing_blocker_destination
         << " pushout_helper_missing_helper_destination="
         << rescue_diagnostics.pushout_helper_missing_helper_destination
         << " pushout_helper_repair_failures="
         << rescue_diagnostics.pushout_helper_repair_failures
         << " pushout_helper_validation_failures="
         << rescue_diagnostics.pushout_helper_validation_failures
         << " pushout_helper_duplicate_plans="
         << rescue_diagnostics.pushout_helper_duplicate_plans
         << " pushout_helper_feasible_plans="
         << rescue_diagnostics.pushout_helper_feasible_plans
         << " pushout_helper_screen_rejected="
         << rescue_diagnostics.pushout_helper_screen_rejected
         << " pushout_helper_adopted=" << rescue_diagnostics.pushout_helper_adopted
         << " pushout_helper_surveyed_turns="
         << rescue_diagnostics.pushout_helper_surveyed_turns
         << " pushout_helper_surveyed_targets="
         << rescue_diagnostics.pushout_helper_surveyed_targets
         << " pushout_helper_large_blocker_targets="
         << rescue_diagnostics.pushout_helper_large_blocker_targets
         << " pushout_helper_probe_limit_exhausted="
         << rescue_diagnostics.pushout_helper_probe_limit_exhausted
         << " pushout_helper_feasible_limit_exhausted="
         << rescue_diagnostics.pushout_helper_feasible_limit_exhausted
         << " pushout_helper_two_feasible_turns="
         << rescue_diagnostics.pushout_helper_two_feasible_turns
         << " pushout_helper_maximum_movers="
         << rescue_diagnostics.pushout_helper_maximum_movers
         << " pushout_helper_feasible_1_blocker="
         << rescue_diagnostics.pushout_helper_feasible_by_blocker_count[0]
         << " pushout_helper_feasible_2_blockers="
         << rescue_diagnostics.pushout_helper_feasible_by_blocker_count[1]
         << " pushout_helper_feasible_3_blockers="
         << rescue_diagnostics.pushout_helper_feasible_by_blocker_count[2]
         << " pushout_helper_adopted_1_blocker="
         << rescue_diagnostics.pushout_helper_adopted_by_blocker_count[0]
         << " pushout_helper_adopted_2_blockers="
         << rescue_diagnostics.pushout_helper_adopted_by_blocker_count[1]
         << " pushout_helper_adopted_3_blockers="
         << rescue_diagnostics.pushout_helper_adopted_by_blocker_count[2]
         << " pushout_helper_obstruction_probes="
         << rescue_diagnostics.pushout_helper_obstruction_probes
         << " pushout_helper_single_owner_regions="
         << rescue_diagnostics.pushout_helper_single_owner_regions
         << " pushout_helper_overlap_cells="
         << rescue_diagnostics.pushout_helper_overlap_cells
         << " pushout_helper_evidenced_groups="
         << rescue_diagnostics.pushout_helper_evidenced_groups
         << " pushout_helper_recorded_witnesses="
         << rescue_diagnostics.pushout_helper_recorded_witnesses
         << " pushout_helper_shortlisted_choices="
         << rescue_diagnostics.pushout_helper_shortlisted_choices
         << " pushout_helper_destination_anchors="
         << rescue_diagnostics.pushout_helper_destination_anchors
         << " pushout_helper_destination_candidates="
         << rescue_diagnostics.pushout_helper_destination_candidates
         << " pushout_helper_foreign_destination_candidates="
         << rescue_diagnostics.pushout_helper_foreign_destination_candidates
         << " pushout_helper_retained_foreign_destinations="
         << rescue_diagnostics.pushout_helper_retained_foreign_destinations
         << " pushout_helper_forced_witness_destinations="
         << rescue_diagnostics.pushout_helper_forced_witness_destinations
         << " pushout_helper_beam_nodes="
         << rescue_diagnostics.pushout_helper_beam_nodes
         << " pushout_helper_selected_covered_blockers="
         << rescue_diagnostics.pushout_helper_selected_covered_blockers
         << " pushout_helper_selected_unlocked_regions="
         << rescue_diagnostics.pushout_helper_selected_unlocked_regions
         << " pushout_helper_selected_overlap_cells="
         << rescue_diagnostics.pushout_helper_selected_overlap_cells
         << " pushout_helper_selected_movement_cost="
         << rescue_diagnostics.pushout_helper_selected_movement_cost
         << " pushout_helper_selected_departure_distance="
         << rescue_diagnostics.pushout_helper_selected_departure_distance
         << " pushout_helper_selected_adjusted_gain="
         << rescue_diagnostics.pushout_helper_selected_adjusted_gain
         << " pushout_helper_feasible_blocker_uses_helper="
         << rescue_diagnostics.pushout_helper_feasible_blocker_uses_helper
         << " pushout_helper_feasible_helper_uses_blocker="
         << rescue_diagnostics.pushout_helper_feasible_helper_uses_blocker
         << " pushout_helper_feasible_bidirectional_cross_use="
         << rescue_diagnostics.pushout_helper_feasible_bidirectional_cross_use
         << " pushout_helper_adopted_blocker_uses_helper="
         << rescue_diagnostics.pushout_helper_adopted_blocker_uses_helper
         << " pushout_helper_adopted_helper_uses_blocker="
         << rescue_diagnostics.pushout_helper_adopted_helper_uses_blocker
         << " pushout_helper_adopted_bidirectional_cross_use="
         << rescue_diagnostics.pushout_helper_adopted_bidirectional_cross_use
         << " pushout_helper_adopted_moved_groups="
         << rescue_diagnostics.pushout_helper_adopted_moved_groups
         << " pushout_helper_adopted_moved_cells="
         << rescue_diagnostics.pushout_helper_adopted_moved_cells
         << " pushout_helper_adopted_arrival_fee="
         << rescue_diagnostics.pushout_helper_adopted_arrival_fee
         << " pushout_helper_adopted_movement_cost="
         << rescue_diagnostics.pushout_helper_adopted_movement_cost
         << " pushout_helper_adopted_direct_gain="
         << rescue_diagnostics.pushout_helper_adopted_direct_gain
         << " pushout_helper_scenario_0_future_delta="
         << rescue_diagnostics.pushout_helper_scenario_0_future_delta
         << " pushout_helper_scenario_1_future_delta="
         << rescue_diagnostics.pushout_helper_scenario_1_future_delta
         << " pushout_helper_screen_margin=" << fixed << setprecision(6)
         << (double)rescue_diagnostics.pushout_helper_screen_margin
         << " pushout_helper_phase_cpu_ms="
         << 1000.0 * rescue_diagnostics.pushout_helper_phase_cpu_seconds
         << " pushout_helper_turn_funnel_error=" << pushout_helper_turn_funnel_error
         << " pushout_helper_attempt_funnel_error="
         << pushout_helper_attempt_funnel_error
         << " pushout_helper_missing_partition_error="
         << pushout_helper_missing_partition_error
         << " pushout_helper_feasible_funnel_error="
         << pushout_helper_feasible_funnel_error
         << " pushout_helper_feasible_histogram_error="
         << pushout_helper_feasible_histogram_error
         << " pushout_helper_adopted_histogram_error="
         << pushout_helper_adopted_histogram_error
         << " pushout_helper_direct_identity_error="
         << pushout_helper_direct_identity_error
         << " pushout_helper_work_cap_error=" << pushout_helper_work_cap_error
         << " pushout_helper_disabled_error=" << pushout_helper_disabled_error
         << " pushout_status_identity_error=" << pushout_status_identity_error
         << " pushout_feasible_histogram_error=" << pushout_feasible_histogram_error
         << " pushout_adopted_histogram_error=" << pushout_adopted_histogram_error
         << " pushout_funnel_identity_error=" << pushout_funnel_identity_error
         << " pushout_direct_identity_error=" << pushout_direct_identity_error
         << " compact_rescue_feasible_turns="
         << rescue_diagnostics.feasible_turns - rescue_diagnostics.pushout_feasible_turns
         << " compact_rescue_feasible_plans="
         << rescue_diagnostics.feasible_plans - rescue_diagnostics.pushout_feasible_plans
         << " compact_rescue_successes="
         << rescue_diagnostics.successes - rescue_diagnostics.pushout_adopted
         << " compact_rescue_target_anchors="
         << rescue_diagnostics.target_anchors - rescue_diagnostics.pushout_target_anchors
         << " compact_rescue_destination_anchors="
         << rescue_diagnostics.destination_anchors - rescue_diagnostics.pushout_destination_anchors
         << " compact_rescue_rollout_turns="
         << rescue_diagnostics.rollout_turns - rescue_diagnostics.pushout_rollout_turns
         << " compact_rescue_moved_groups="
         << rescue_diagnostics.moved_groups - rescue_diagnostics.pushout_moved_groups
         << " compact_rescue_movement_cost="
         << rescue_diagnostics.movement_cost - rescue_diagnostics.pushout_movement_cost
         << " compact_rescue_immediate_gain="
         << rescue_diagnostics.immediate_gain - rescue_diagnostics.pushout_direct_gain
         << " rescue_eligible_fallbacks=" << rescue_diagnostics.eligible_fallbacks
         << " rescue_feasible_turns=" << rescue_diagnostics.feasible_turns
         << " rescue_feasible_plans=" << rescue_diagnostics.feasible_plans
         << " rescue_successes=" << rescue_diagnostics.successes
         << " rescue_feasible_1_blocker=" << rescue_diagnostics.feasible_by_blocker_count[0]
         << " rescue_feasible_2_blockers=" << rescue_diagnostics.feasible_by_blocker_count[1]
         << " rescue_feasible_3_blockers=" << rescue_diagnostics.feasible_by_blocker_count[2]
         << " rescue_feasible_4plus_blockers=" << rescue_diagnostics.feasible_by_blocker_count[3]
         << " rescue_successes_1_blocker=" << rescue_diagnostics.successes_by_blocker_count[0]
         << " rescue_successes_2_blockers=" << rescue_diagnostics.successes_by_blocker_count[1]
         << " rescue_successes_3_blockers=" << rescue_diagnostics.successes_by_blocker_count[2]
         << " rescue_successes_4plus_blockers=" << rescue_diagnostics.successes_by_blocker_count[3]
         << " rescue_no_economic_target=" << rescue_diagnostics.no_economic_target
         << " rescue_no_repair=" << rescue_diagnostics.no_repair
         << " rescue_target_limit_exhausted=" << rescue_diagnostics.target_limit_exhausted
         << " rescue_destination_limit_exhausted=" << rescue_diagnostics.destination_limit_exhausted
         << " rescue_node_limit_exhausted=" << rescue_diagnostics.node_limit_exhausted
         << " rescue_validation_failures=" << rescue_diagnostics.validation_failures
         << " rescue_maximum_blockers=" << rescue_diagnostics.maximum_blockers
         << " rescue_target_anchors=" << rescue_diagnostics.target_anchors
         << " rescue_target_shortlisted=" << rescue_diagnostics.target_shortlisted
         << " rescue_exact_targets=" << rescue_diagnostics.exact_targets
         << " rescue_economic_targets=" << rescue_diagnostics.economic_targets
         << " rescue_repair_attempts=" << rescue_diagnostics.repair_attempts
         << " rescue_destination_anchors=" << rescue_diagnostics.destination_anchors
         << " rescue_destination_candidates=" << rescue_diagnostics.destination_candidates
         << " rescue_beam_nodes=" << rescue_diagnostics.beam_nodes
         << " rescue_rollout_turns=" << rescue_diagnostics.rollout_turns
         << " rescue_rollout_generation_failures=" << rescue_diagnostics.rollout_generation_failures
         << " rescue_rollout_skipped_no_future=" << rescue_diagnostics.rollout_skipped_no_future
         << " rescue_rollout_adopted=" << rescue_diagnostics.rollout_adopted
         << " rescue_rollout_not_selected=" << rescue_diagnostics.rollout_rescue_not_selected
         << " rescue_rollout_one_candidate_turns=" << rescue_diagnostics.rollout_one_candidate_turns
         << " rescue_rollout_two_candidate_turns=" << rescue_diagnostics.rollout_two_candidate_turns
         << " rescue_rollout_selected_candidate_0=" << rescue_diagnostics.rollout_selected_candidate_0
         << " rescue_rollout_selected_candidate_1=" << rescue_diagnostics.rollout_selected_candidate_1
         << " rescue_rollout_scenario_disagreements=" << rescue_diagnostics.rollout_scenario_disagreements
         << " rescue_rollout_candidate_0_disagreements="
         << rescue_diagnostics.rollout_candidate_0_disagreements
         << " rescue_rollout_candidate_1_disagreements="
         << rescue_diagnostics.rollout_candidate_1_disagreements
         << " rescue_rollout_same_blocker_sets=" << rescue_diagnostics.rollout_same_blocker_sets
         << " rescue_rollout_policy_steps=" << rescue_diagnostics.rollout_policy_steps
         << " rescue_rollout_candidates_compared=" << rescue_diagnostics.rollout_candidates_compared
         << " rescue_rollout_positive_candidates=" << rescue_diagnostics.rollout_positive_candidates
         << " rescue_rollout_nonpositive_candidates=" << rescue_diagnostics.rollout_nonpositive_candidates
         << " rescue_rollout_unselected_positive_candidates="
         << rescue_diagnostics.rollout_unselected_positive_candidates
         << " rescue_rollout_candidate_overlap_cells=" << rescue_diagnostics.rollout_candidate_overlap_cells
         << " rescue_rollout_baseline_acceptances=" << rescue_diagnostics.rollout_baseline_acceptances
         << " rescue_rollout_rescue_acceptances=" << rescue_diagnostics.rollout_rescue_acceptances
         << " root_alternative_available_turns=" << rescue_diagnostics.root_alternative_available_turns
         << " root_selected_primary=" << rescue_diagnostics.root_selected_primary
         << " root_selected_alternative=" << rescue_diagnostics.root_selected_alternative
         << " root_alternative_disagreements=" << rescue_diagnostics.root_alternative_disagreements
         << " root_screen_v3_overrides=" << rescue_diagnostics.root_screen_v3_overrides
         << " root_screen_selected_alternative="
         << rescue_diagnostics.root_screen_selected_alternative
         << " root_v3_winner_overridden=" << rescue_diagnostics.root_v3_winner_overridden
         << " root_selected_alternative_rank_0="
         << rescue_diagnostics.root_selected_alternative_rank[0]
         << " root_selected_alternative_rank_1="
         << rescue_diagnostics.root_selected_alternative_rank[1]
         << " root_2_action_turns=" << rescue_diagnostics.root_turns_by_action_count[2]
         << " root_3_action_turns=" << rescue_diagnostics.root_turns_by_action_count[3]
         << " root_4_action_turns=" << rescue_diagnostics.root_turns_by_action_count[4]
         << " root_5_action_turns=" << rescue_diagnostics.root_turns_by_action_count[5]
         << " root_actions_compared=" << rescue_diagnostics.root_actions_compared
         << " root_alternatives_compared=" << rescue_diagnostics.root_alternatives_compared
         << " root_alternative_acceptances=" << rescue_diagnostics.root_alternative_acceptances
         << " normal_root_gate_turns=" << rescue_diagnostics.normal_root_gate_turns
         << " normal_root_rollout_turns=" << rescue_diagnostics.normal_root_rollout_turns
         << " normal_root_generation_failures=" << rescue_diagnostics.normal_root_generation_failures
         << " normal_root_screen_overrides=" << rescue_diagnostics.normal_root_screen_overrides
         << " normal_root_screen_selected_alternative="
         << rescue_diagnostics.normal_root_screen_selected_alternative
         << " normal_root_selected_primary=" << rescue_diagnostics.normal_root_selected_primary
         << " normal_root_selected_alternative=" << rescue_diagnostics.normal_root_selected_alternative
         << " normal_root_selected_alternative_rank_0="
         << rescue_diagnostics.normal_root_selected_alternative_rank[0]
         << " normal_root_selected_alternative_rank_1="
         << rescue_diagnostics.normal_root_selected_alternative_rank[1]
         << " normal_root_2_action_turns=" << rescue_diagnostics.normal_root_turns_by_action_count[2]
         << " normal_root_3_action_turns=" << rescue_diagnostics.normal_root_turns_by_action_count[3]
         << " normal_root_actions_compared=" << rescue_diagnostics.normal_root_actions_compared
         << " normal_root_alternatives_compared=" << rescue_diagnostics.normal_root_alternatives_compared
         << " normal_root_policy_steps=" << rescue_diagnostics.normal_root_policy_steps
         << " root_confirmation_used=" << root_confirmations_used
         << " root_confirmation_attempts=" << rescue_diagnostics.root_confirmation_attempts
         << " root_confirmation_approved=" << rescue_diagnostics.root_confirmation_approved
         << " root_confirmation_rejected=" << rescue_diagnostics.root_confirmation_rejected
         << " root_confirmation_generation_failures="
         << rescue_diagnostics.root_confirmation_generation_failures
         << " root_confirmation_budget_skips=" << rescue_diagnostics.root_confirmation_budget_skips
         << " root_confirmation_full_horizon=" << rescue_diagnostics.root_confirmation_full_horizon
         << " root_confirmation_short_horizon=" << rescue_diagnostics.root_confirmation_short_horizon
         << " root_confirmation_pair_disagreements="
         << rescue_diagnostics.root_confirmation_pair_disagreements
         << " root_confirmation_policy_steps=" << rescue_diagnostics.root_confirmation_policy_steps
         << " root_confirmation_scenarios=" << rescue_diagnostics.root_confirmation_scenarios
         << " root_confirmation_positive_scenarios="
         << rescue_diagnostics.root_confirmation_positive_scenarios
         << " rescue_moved_groups=" << rescue_diagnostics.moved_groups
         << " rescue_feasible_direct_gain=" << rescue_diagnostics.feasible_direct_gain
         << " rescue_arrival_fee_gain=" << rescue_diagnostics.arrival_fee_gain
         << " rescue_movement_cost=" << rescue_diagnostics.movement_cost
         << " rescue_immediate_gain=" << rescue_diagnostics.immediate_gain
         << " rescue_rollout_scenario_0_future_delta=" << rescue_diagnostics.rollout_scenario_0_future_delta
         << " rescue_rollout_scenario_1_future_delta=" << rescue_diagnostics.rollout_scenario_1_future_delta
         << " rescue_rollout_slot_0_scenario_0_future_delta="
         << rescue_diagnostics.rollout_slot_scenario_0_future_delta[0]
         << " rescue_rollout_slot_0_scenario_1_future_delta="
         << rescue_diagnostics.rollout_slot_scenario_1_future_delta[0]
         << " rescue_rollout_slot_1_scenario_0_future_delta="
         << rescue_diagnostics.rollout_slot_scenario_0_future_delta[1]
         << " rescue_rollout_slot_1_scenario_1_future_delta="
         << rescue_diagnostics.rollout_slot_scenario_1_future_delta[1]
         << " root_alternative_scenario_0_future_delta="
         << rescue_diagnostics.root_alternative_scenario_0_future_delta
         << " root_alternative_scenario_1_future_delta="
         << rescue_diagnostics.root_alternative_scenario_1_future_delta
         << " sampled_dlp_enabled=" << ENABLE_SAMPLED_DLP
         << " sampled_dlp_rebuilds=" << dlp_diagnostics.rebuilds
         << " sampled_dlp_initial_rebuilds=" << dlp_diagnostics.initial_rebuilds
         << " sampled_dlp_scheduled_rebuilds=" << dlp_diagnostics.scheduled_rebuilds
         << " sampled_dlp_boundary_rebuilds=" << dlp_diagnostics.boundary_rebuilds
         << " sampled_dlp_real_price_calls=" << dlp_diagnostics.real_price_calls
         << " sampled_dlp_rollout_price_calls=" << dlp_diagnostics.rollout_price_calls
         << " sampled_dlp_zero_future_calls=" << dlp_diagnostics.zero_future_calls
         << " sampled_dlp_generated_requests=" << dlp_diagnostics.generated_requests
         << " sampled_dlp_coordinate_updates=" << dlp_diagnostics.coordinate_updates
         << " sampled_dlp_positive_price_buckets=" << dlp_diagnostics.positive_price_buckets
         << " sampled_dlp_sample_hash="
         << (dlp_diagnostics.rebuilds == 0 ? 0ULL : dlp_diagnostics.sample_hash)
         << " sampled_dlp_invalid_model_errors=" << dlp_diagnostics.invalid_model_errors
         << " sampled_dlp_nonfinite_errors=" << dlp_diagnostics.nonfinite_errors
         << " sampled_dlp_request_count_error=" << sampled_dlp_request_count_error
         << " sampled_dlp_trigger_partition_error=" << sampled_dlp_trigger_partition_error
         << " sampled_dlp_real_call_error=" << sampled_dlp_real_call_error
         << " sampled_dlp_rollout_call_error=" << sampled_dlp_rollout_call_error
         << " shadow_considered=" << shadow_diagnostics.considered
         << " shadow_upper_rejected=" << shadow_diagnostics.upper_bound_rejected
         << " shadow_actual_rejected=" << shadow_diagnostics.actual_fee_rejected
         << " shadow_no_region_rejected=" << shadow_diagnostics.no_region_rejected
         << " shadow_accepted=" << shadow_diagnostics.accepted
         << " placement_attempts=" << placement_diagnostics.attempts
         << " placement_compact_successes=" << placement_diagnostics.compact_successes
         << " placement_extended_template_successes=" << placement_diagnostics.extended_template_successes
         << " placement_fallback_successes=" << placement_diagnostics.fallback_successes
         << " placement_actual_rejected_candidate_perimeter_sum="
         << placement_diagnostics.actual_rejected_candidate_perimeter
         << " placement_actual_rejected_candidate_fee_sum="
         << placement_diagnostics.actual_rejected_candidate_fee
         << " placement_future_fit_turns=" << placement_diagnostics.future_fit_evaluated_turns
         << " placement_future_fit_changes=" << placement_diagnostics.future_fit_changed_placements
         << " placement_incremental_changes_from_absolute=" << placement_diagnostics.incremental_changed_from_absolute
         << " placement_final_changes_from_absolute=" << placement_diagnostics.final_changed_from_absolute
         << " placement_anchors_checked=" << placement_diagnostics.anchors_checked
         << " placement_legal_compact_candidates=" << placement_diagnostics.legal_compact_candidates
         << " placement_growth_candidates=" << placement_diagnostics.connected_growth_candidates
         << " grow_and_trim_enabled=" << ENABLE_GROW_AND_TRIM
         << " grow_and_trim_base_candidates="
         << placement_diagnostics.grow_and_trim_base_candidates
         << " grow_and_trim_growth_failures="
         << placement_diagnostics.grow_and_trim_growth_failures
         << " grow_and_trim_full_growths=" << placement_diagnostics.grow_and_trim_full_growths
         << " grow_and_trim_trim_failures=" << placement_diagnostics.grow_and_trim_trim_failures
         << " grow_and_trim_duplicate_candidates="
         << placement_diagnostics.grow_and_trim_duplicate_candidates
         << " grow_and_trim_candidates=" << placement_diagnostics.grow_and_trim_candidates
         << " grow_and_trim_grown_cells=" << placement_diagnostics.grow_and_trim_grown_cells
         << " grow_and_trim_trimmed_cells=" << placement_diagnostics.grow_and_trim_trimmed_cells
         << " grow_and_trim_perimeter_improvement="
         << placement_diagnostics.grow_and_trim_perimeter_improvement
         << " grow_and_trim_perimeter_improved_candidates="
         << placement_diagnostics.grow_and_trim_perimeter_improved_candidates
         << " grow_and_trim_perimeter_equal_candidates="
         << placement_diagnostics.grow_and_trim_perimeter_equal_candidates
         << " grow_and_trim_perimeter_worsened_candidates="
         << placement_diagnostics.grow_and_trim_perimeter_worsened_candidates
         << " grow_and_trim_shortlisted_candidates="
         << placement_diagnostics.grow_and_trim_shortlisted_candidates
         << " grow_and_trim_choices=" << placement_diagnostics.grow_and_trim_successes
         << " placement_shortlisted_candidates=" << placement_diagnostics.shortlisted_candidates
         << " placement_future_fit_snapshots=" << placement_diagnostics.future_fit_snapshots
         << " decomp_static_largest_component=" << static_largest_component
         << " decomp_observed=" << loss_diagnostics.observed
         << " decomp_accepted=" << loss_diagnostics.accepted
         << " decomp_finalized_accepted=" << loss_diagnostics.finalized_accepted
         << " decomp_upper_rejected=" << loss_diagnostics.upper_rejected
         << " decomp_actual_rejected=" << loss_diagnostics.actual_rejected
         << " decomp_no_region_rejected=" << loss_diagnostics.no_region_rejected
         << " decomp_rejected_status_mismatch=" << loss_diagnostics.rejected_status_mismatch
         << " decomp_rejected_feasible=" << loss_diagnostics.rejected_feasible
         << " decomp_rejected_unplaceable=" << loss_diagnostics.rejected_unplaceable
         << " decomp_upper_rejected_feasible=" << loss_diagnostics.upper_rejected_feasible
         << " decomp_upper_rejected_unplaceable=" << loss_diagnostics.upper_rejected_unplaceable
         << " decomp_unplaceable_static=" << loss_diagnostics.unplaceable_static
         << " decomp_unplaceable_capacity=" << loss_diagnostics.unplaceable_capacity
         << " decomp_unplaceable_fragmentation=" << loss_diagnostics.unplaceable_fragmentation
         << " decomp_feasibility_mismatches=" << loss_diagnostics.feasibility_mismatches
         << " decomp_accepted_status_mismatches=" << loss_diagnostics.accepted_status_mismatches
         << " decomp_accepted_plan_mismatches=" << loss_diagnostics.accepted_plan_mismatches
         << " decomp_accepted_source_mismatches=" << loss_diagnostics.accepted_source_mismatches
         << " decomp_rejected_move_plans=" << loss_diagnostics.rejected_move_plans
         << " decomp_offered_ideal_fee=" << loss_diagnostics.offered_ideal_fee
         << " decomp_offered_cell_time=" << loss_diagnostics.offered_cell_time
         << " decomp_accepted_ideal_fee=" << loss_diagnostics.accepted_ideal_fee
         << " decomp_accepted_initial_fee=" << loss_diagnostics.accepted_initial_fee
         << " decomp_accepted_final_fee=" << loss_diagnostics.accepted_final_fee
         << " decomp_accepted_initial_shape_loss=" << loss_diagnostics.accepted_initial_shape_loss
         << " decomp_accepted_relocation_fee_loss=" << loss_diagnostics.accepted_relocation_fee_loss
         << " decomp_accepted_cell_time=" << loss_diagnostics.accepted_cell_time
         << " decomp_movement_cost=" << loss_diagnostics.movement_cost_paid
         << " decomp_reconstructed_raw_score=" << reconstructed_raw_score
         << " decomp_reconstructed_absolute_score=" << reconstructed_absolute_score
         << " decomp_rejected_ideal_fee=" << loss_diagnostics.rejected_ideal_fee
         << " decomp_rejected_cell_time=" << loss_diagnostics.rejected_cell_time
         << " decomp_upper_rejected_ideal_fee=" << loss_diagnostics.upper_rejected_ideal_fee
         << " decomp_upper_rejected_cell_time=" << loss_diagnostics.upper_rejected_cell_time
         << " decomp_actual_rejected_ideal_fee=" << loss_diagnostics.actual_rejected_ideal_fee
         << " decomp_actual_rejected_cell_time=" << loss_diagnostics.actual_rejected_cell_time
         << " decomp_actual_rejected_candidate_fee=" << loss_diagnostics.actual_rejected_candidate_fee
         << " decomp_actual_rejected_geometry_loss=" << loss_diagnostics.actual_rejected_geometry_loss
         << " decomp_no_region_ideal_fee=" << loss_diagnostics.no_region_ideal_fee
         << " decomp_no_region_cell_time=" << loss_diagnostics.no_region_cell_time
         << " decomp_rejected_status_mismatch_ideal_fee="
         << loss_diagnostics.rejected_status_mismatch_ideal_fee
         << " decomp_rejected_status_mismatch_cell_time="
         << loss_diagnostics.rejected_status_mismatch_cell_time
         << " decomp_rejected_feasible_ideal_fee=" << loss_diagnostics.rejected_feasible_ideal_fee
         << " decomp_rejected_feasible_cell_time=" << loss_diagnostics.rejected_feasible_cell_time
         << " decomp_rejected_unplaceable_ideal_fee=" << loss_diagnostics.rejected_unplaceable_ideal_fee
         << " decomp_rejected_unplaceable_cell_time=" << loss_diagnostics.rejected_unplaceable_cell_time
         << " decomp_unplaceable_static_ideal_fee=" << loss_diagnostics.unplaceable_static_ideal_fee
         << " decomp_unplaceable_static_cell_time=" << loss_diagnostics.unplaceable_static_cell_time
         << " decomp_unplaceable_capacity_ideal_fee=" << loss_diagnostics.unplaceable_capacity_ideal_fee
         << " decomp_unplaceable_capacity_cell_time=" << loss_diagnostics.unplaceable_capacity_cell_time
         << " decomp_unplaceable_fragmentation_ideal_fee="
         << loss_diagnostics.unplaceable_fragmentation_ideal_fee
         << " decomp_unplaceable_fragmentation_cell_time="
         << loss_diagnostics.unplaceable_fragmentation_cell_time
         << " decomp_accepted_minimum_count=" << loss_diagnostics.accepted_by_source[0]
         << " decomp_accepted_extended_count=" << loss_diagnostics.accepted_by_source[1]
         << " decomp_accepted_growth_count=" << loss_diagnostics.accepted_by_source[2]
         << " decomp_accepted_grow_and_trim_count=" << loss_diagnostics.accepted_grow_and_trim
         << " decomp_accepted_unclassified_count=" << loss_diagnostics.accepted_by_source[3]
         << " decomp_accepted_minimum_ideal_fee=" << loss_diagnostics.accepted_source_ideal_fee[0]
         << " decomp_accepted_extended_ideal_fee=" << loss_diagnostics.accepted_source_ideal_fee[1]
         << " decomp_accepted_growth_ideal_fee=" << loss_diagnostics.accepted_source_ideal_fee[2]
         << " decomp_accepted_grow_and_trim_ideal_fee="
         << loss_diagnostics.accepted_grow_and_trim_ideal_fee
         << " decomp_accepted_unclassified_ideal_fee="
         << loss_diagnostics.accepted_source_ideal_fee[3]
         << " decomp_accepted_minimum_initial_fee=" << loss_diagnostics.accepted_source_initial_fee[0]
         << " decomp_accepted_extended_initial_fee=" << loss_diagnostics.accepted_source_initial_fee[1]
         << " decomp_accepted_growth_initial_fee=" << loss_diagnostics.accepted_source_initial_fee[2]
         << " decomp_accepted_grow_and_trim_initial_fee="
         << loss_diagnostics.accepted_grow_and_trim_initial_fee
         << " decomp_accepted_unclassified_initial_fee="
         << loss_diagnostics.accepted_source_initial_fee[3]
         << " decomp_accepted_perimeter_excess=" << loss_diagnostics.accepted_perimeter_excess
         << " decomp_accepted_decision_fee_error="
         << loss_diagnostics.accepted_decision_fee_error
         << " decomp_accepted_decision_perimeter_error="
         << loss_diagnostics.accepted_decision_perimeter_error
         << " decomp_accepted_minimum_perimeter_excess="
         << loss_diagnostics.accepted_source_perimeter_excess[0]
         << " decomp_accepted_extended_perimeter_excess="
         << loss_diagnostics.accepted_source_perimeter_excess[1]
         << " decomp_accepted_growth_perimeter_excess="
         << loss_diagnostics.accepted_source_perimeter_excess[2]
         << " decomp_accepted_grow_and_trim_perimeter_excess="
         << loss_diagnostics.accepted_grow_and_trim_perimeter_excess
         << " decomp_accepted_unclassified_perimeter_excess="
         << loss_diagnostics.accepted_source_perimeter_excess[3]
         << " decomp_accepted_free_cells_sum=" << loss_diagnostics.accepted_free_cells_sum
         << " decomp_rejected_feasible_free_cells_sum="
         << loss_diagnostics.rejected_feasible_free_cells_sum
         << " decomp_rejected_unplaceable_free_cells_sum="
         << loss_diagnostics.rejected_unplaceable_free_cells_sum
         << " decomp_offered_identity_error=" << offered_identity_error
         << " decomp_cell_time_identity_error=" << cell_time_identity_error
         << " decomp_accepted_initial_identity_error=" << accepted_initial_identity_error
         << " decomp_accepted_final_identity_error=" << accepted_final_identity_error
         << " decomp_gap_identity_error=" << gap_identity_error
         << " decomp_observed_count_error=" << observed_count_error
         << " decomp_total_count_partition_error=" << total_count_partition_error
         << " decomp_accepted_count_error=" << accepted_count_error
         << " decomp_finalized_count_error=" << finalized_count_error
         << " decomp_rejected_count_error=" << rejected_count_error
         << " decomp_rejected_status_count_error=" << rejected_status_count_error
         << " decomp_upper_count_partition_error=" << upper_count_partition_error
         << " decomp_unplaceable_count_partition_error=" << unplaceable_count_partition_error
         << " decomp_accepted_source_count_error=" << accepted_source_count_error
         << " decomp_rejected_fee_partition_error=" << rejected_fee_partition_error
         << " decomp_rejected_cell_time_partition_error="
         << rejected_cell_time_partition_error
         << " decomp_rejected_status_fee_error=" << rejected_status_fee_error
         << " decomp_rejected_status_cell_time_error=" << rejected_status_cell_time_error
         << " decomp_unplaceable_fee_partition_error=" << unplaceable_fee_partition_error
         << " decomp_unplaceable_cell_time_partition_error="
         << unplaceable_cell_time_partition_error
         << " decomp_accepted_source_ideal_fee_error=" << accepted_source_ideal_fee_error
         << " decomp_accepted_source_initial_fee_error=" << accepted_source_initial_fee_error
         << " decomp_accepted_source_perimeter_error=" << accepted_source_perimeter_error
         << " decomp_actual_candidate_fee_identity_error="
         << actual_candidate_fee_identity_error
         << " grow_and_trim_growth_funnel_error=" << grow_and_trim_growth_funnel_error
         << " grow_and_trim_completion_funnel_error=" << grow_and_trim_completion_funnel_error
         << " grow_and_trim_perimeter_partition_error=" << grow_and_trim_perimeter_partition_error
         << " grow_and_trim_source_error=" << grow_and_trim_source_error
         << fixed << setprecision(6)
         << " decomp_accepted_opportunity=" << loss_diagnostics.accepted_opportunity_cost
         << " decomp_upper_rejected_opportunity=" << loss_diagnostics.upper_rejected_opportunity_cost
         << " decomp_actual_rejected_opportunity=" << loss_diagnostics.actual_rejected_opportunity_cost
         << " decomp_no_region_opportunity=" << loss_diagnostics.no_region_opportunity_cost
         << " rescue_rollout_adopted_direct_gain=" << rescue_diagnostics.rollout_adopted_direct_gain
         << " rescue_rollout_adopted_future_mean=" << rescue_diagnostics.rollout_adopted_future_mean
         << " rescue_rollout_adopted_margin=" << rescue_diagnostics.rollout_adopted_margin
         << " rescue_rollout_not_selected_direct_gain="
         << rescue_diagnostics.rollout_not_selected_direct_gain
         << " rescue_rollout_not_selected_future_mean="
         << rescue_diagnostics.rollout_not_selected_future_mean
         << " rescue_rollout_not_selected_margin=" << rescue_diagnostics.rollout_not_selected_margin
         << " rescue_rollout_slot_0_margin=" << rescue_diagnostics.rollout_slot_margin[0]
         << " rescue_rollout_slot_1_margin=" << rescue_diagnostics.rollout_slot_margin[1]
         << " rescue_rollout_width_predicted_gain=" << rescue_diagnostics.rollout_width_predicted_gain
         << " root_alternative_direct_gain=" << rescue_diagnostics.root_alternative_direct_gain
         << " root_alternative_future_mean=" << rescue_diagnostics.root_alternative_future_mean
         << " root_alternative_margin=" << rescue_diagnostics.root_alternative_margin
         << " root_expanded_predicted_gain=" << rescue_diagnostics.root_expanded_predicted_gain
         << " root_confirmation_screen_gain=" << rescue_diagnostics.root_confirmation_screen_gain
         << " root_confirmation_holdout_margin="
         << rescue_diagnostics.root_confirmation_holdout_margin
         << " root_confirmation_approved_margin="
         << rescue_diagnostics.root_confirmation_approved_margin
         << " root_confirmation_rejected_margin="
         << rescue_diagnostics.root_confirmation_rejected_margin
         << " theta_mean=" << mean_theta << " shadow_mean_opportunity=" << mean_opportunity_cost
         << " shadow_mean_rejected_fraction=" << mean_rejected_fraction
         << " shadow_max_rejected_fraction=" << shadow_diagnostics.maximum_rejected_fraction
         << " shadow_priced_buckets=" << shadow_diagnostics.priced_buckets
         << " sampled_dlp_dual_objective_sum=" << dlp_diagnostics.dual_objective_sum
         << " sampled_dlp_capacity_sum=" << dlp_diagnostics.capacity_sum
         << " sampled_dlp_offered_load_sum=" << dlp_diagnostics.offered_load_sum
         << " sampled_dlp_opportunity_sum=" << dlp_diagnostics.opportunity_cost_sum
         << " sampled_dlp_maximum_price=" << dlp_diagnostics.maximum_price
         << " sampled_dlp_rebuild_cpu_ms=" << dlp_diagnostics.rebuild_cpu_ms
         << " sampled_dlp_maximum_rebuild_cpu_ms="
         << dlp_diagnostics.maximum_rebuild_cpu_ms
         << " model_expected_p=" << density_model.expected_group_size
         << " timing_process_cpu_ms=" << runtime.process_cpu_ms
         << " timing_solver_cpu_ms=" << runtime.solver_cpu_ms
         << " timing_diagnostic_cpu_ms=" << runtime.diagnostic_cpu_ms
         << " timing_solver_wall_ms=" << runtime.solver_wall_ms
         << " timing_diagnostic_wall_ms=" << runtime.diagnostic_wall_ms
         << " timing_input_wall_ms=" << runtime.input_wall_ms
         << " timing_output_wall_ms=" << runtime.output_wall_ms
         << " timing_protocol_wall_ms=" << runtime.protocol_wall_ms
         << " timing_unaccounted_wall_ms=" << runtime.unaccounted_wall_ms
         << " timing_preprocess_wall_ms=" << runtime.preprocess_wall_ms
         << " timing_max_solver_turn_wall_ms=" << runtime.maximum_solver_turn_wall_ms
         << " elapsed=" << setprecision(3) << runtime.protocol_wall_ms / 1000.0 << '\n';

    return 0;
}
