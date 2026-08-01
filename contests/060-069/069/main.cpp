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

struct Timer {
    chrono::steady_clock::time_point start = chrono::steady_clock::now();

    double elapsed() const { return chrono::duration<double>(chrono::steady_clock::now() - start).count(); }
};

constexpr ll ARRIVAL_TIME_HORIZON = 100000;
constexpr int TIME_BUCKET_COUNT = 64;
constexpr int THETA_MIN = 2000;
constexpr int THETA_MAX = 8000;
constexpr int THETA_STEP = 100;
constexpr int THETA_QUADRATURE_STEPS = 48;
constexpr int COMPACT_PERIMETER_MARGIN = 4;
constexpr int PLACEMENT_GLOBAL_SHORTLIST = 3;
constexpr int PLACEMENT_SHORTLIST_LIMIT = 6;
constexpr int CONNECTED_GROWTH_SEED_LIMIT = 16;
constexpr int FUTURE_FIT_SNAPSHOT_COUNT = 3;
constexpr array<int, 8> FUTURE_FIT_SIDES = {2, 3, 4, 5, 6, 8, 10, 12};
constexpr int CLEANUP_SEARCH_LIMIT = 24;
constexpr int CLEANUP_MOVER_LIMIT = 8;
constexpr int CLEANUP_DESTINATION_ANCHOR_LIMIT = 4096;
constexpr long long CLEANUP_DESTINATION_ANCHOR_GLOBAL_LIMIT = 160000;
constexpr int CLEANUP_DESTINATION_LIMIT = 3;
constexpr int CLEANUP_CANDIDATE_EVALUATION_LIMIT = 192;
// Submission setting: compare at most two cleanup candidates by two mirrored
// synthetic futures, each looking 48 arrivals ahead.
constexpr int CLEANUP_FINALIST_LIMIT = 2;
constexpr int CLEANUP_ROLLOUT_LENGTH = 48;
constexpr int CLEANUP_ROLLOUT_SCENARIO_COUNT = 2;
constexpr int CLEANUP_FRESH_OVERLAP_NUMERATOR = 3;
constexpr int CLEANUP_FRESH_OVERLAP_DENOMINATOR = 4;

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

    int min_perimeter = numeric_limits<int>::max();
    for (const Shape &shape : shapes) {
        chmin(min_perimeter, shape.perimeter);
    }

    shapes.erase(
        remove_if(shapes.begin(), shapes.end(),
                  [&](const Shape &shape) { return shape.perimeter > min_perimeter + COMPACT_PERIMETER_MARGIN; }),
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

struct NormalPlacementChoice {
    vector<Cell> cells;
    int perimeter;
};

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

    long double estimate(ll current_s, int remaining_groups) const {
        constexpr int PARTICLE_COUNT = (THETA_MAX - THETA_MIN) / THETA_STEP + 1;
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

struct FutureSpaceValueModel {
    array<long double, FUTURE_FIT_SIDES.size()> size_probability{};
    array<long double, FUTURE_FIT_SIDES.size()> compact_fee_coefficient{};

    explicit FutureSpaceValueModel(const vector<vector<Shape>> &compact_shapes) {
        const long double sqrt_150 = sqrtl(150.0L);
        const long double denominator = sqrt_150 - 2.0L;
        for (int p = 4; p <= 150; p++) {
            long double lower = sqrtl(max(4.0L, (long double)p - 0.5L));
            long double upper = sqrtl(min(150.0L, (long double)p + 0.5L));
            long double probability = max(0.0L, upper - lower) / denominator;

            int bucket = 0;
            for (int index = 1; index < (int)FUTURE_FIT_SIDES.size(); index++) {
                if (fabsl(sqrtl((long double)p) - FUTURE_FIT_SIDES[index]) <
                    fabsl(sqrtl((long double)p) - FUTURE_FIT_SIDES[bucket])) {
                    bucket = index;
                }
            }

            int minimum_perimeter = compact_shapes[p].front().perimeter;
            long double compactness = 4.0L * sqrtl((long double)p) / minimum_perimeter;
            size_probability[bucket] += probability;
            compact_fee_coefficient[bucket] += probability * p * compactness;
        }

        for (int bucket = 0; bucket < (int)FUTURE_FIT_SIDES.size(); bucket++) {
            if (size_probability[bucket] > 0.0L) {
                compact_fee_coefficient[bucket] /= size_probability[bucket];
            }
        }
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

// Price the candidate's occupied cell-time by the fee density of future groups
// that would be crowded out.  The same price is used before and after cleanup,
// so relocation changes only the board and the realizable arrival fee.
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

vector<vector<Cell>> make_connected_growth_candidates(const vs &park, const vvi &owner, int p) {
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

    for (const Seed &seed_info : seeds) {
        int seed_x = seed_info.cell.first;
        int seed_y = seed_info.cell.second;
        vector<char> selected(n * n, false);
        vector<Cell> region;
        region.reserve(p);
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

        select_cell(seed_x, seed_y);
        while ((int)region.size() < p && !frontier.empty()) {
            GrowthEntry entry = frontier.top();
            frontier.pop();
            if (selected[entry.cell]) continue;
            int current_neighbors = count_selected_neighbors(entry.cell);
            if (current_neighbors != entry.selected_neighbors) {
                int x = entry.cell / n;
                int y = entry.cell % n;
                frontier.push({entry.cell, current_neighbors, abs(x - seed_x) + abs(y - seed_y), bias_key(x, y)});
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

struct CleanupSpaceProfile {
    array<int, FUTURE_FIT_SIDES.size()> square_anchors{};
    array<int, FUTURE_FIT_SIDES.size()> component_slots{};
};

// Measure the board at one snapshot by both square-placement flexibility and
// connected-component capacity.  A group with t < snapshot_time is treated as
// free because it will have departed before an arrival at that time.
CleanupSpaceProfile make_cleanup_space_profile(const vs &park, const vvi &owner, const vector<GroupState> &groups,
                                               ll snapshot_time) {
    int n = park.size();
    vector<char> free_cell(n * n, false);
    for (int x = 0; x < n; x++) {
        for (int y = 0; y < n; y++) {
            int occupied_by = owner[x][y];
            free_cell[x * n + y] = park[x][y] == '.' && (occupied_by == -1 || groups[occupied_by].t < snapshot_time);
        }
    }

    CleanupSpaceProfile result;
    vector<int> histogram(n + 1);
    vector<int> previous(n + 1), current(n + 1);
    for (int x = 0; x < n; x++) {
        fill(current.begin(), current.end(), 0);
        for (int y = 0; y < n; y++) {
            if (!free_cell[x * n + y]) continue;
            current[y + 1] = 1 + min({previous[y + 1], current[y], previous[y]});
            histogram[current[y + 1]]++;
        }
        swap(previous, current);
    }
    vector<int> at_least(n + 2);
    for (int side = n; side >= 1; side--) {
        at_least[side] = at_least[side + 1] + histogram[side];
    }
    for (int bucket = 0; bucket < (int)FUTURE_FIT_SIDES.size(); bucket++) {
        int side = FUTURE_FIT_SIDES[bucket];
        result.square_anchors[bucket] = at_least[side];
    }

    constexpr int DX[4] = {-1, 1, 0, 0};
    constexpr int DY[4] = {0, 0, -1, 1};
    vector<char> visited(n * n, false);
    vector<int> component_sizes;
    for (int start_x = 0; start_x < n; start_x++) {
        for (int start_y = 0; start_y < n; start_y++) {
            int start = start_x * n + start_y;
            if (!free_cell[start] || visited[start]) continue;
            int component_size = 0;
            queue<Cell> que;
            que.emplace(start_x, start_y);
            visited[start] = true;
            while (!que.empty()) {
                auto [x, y] = que.front();
                que.pop();
                component_size++;
                for (int dir = 0; dir < 4; dir++) {
                    int nx = x + DX[dir];
                    int ny = y + DY[dir];
                    if (!inside(nx, ny, n, n)) continue;
                    int next = nx * n + ny;
                    if (!free_cell[next] || visited[next]) continue;
                    visited[next] = true;
                    que.emplace(nx, ny);
                }
            }
            component_sizes.push_back(component_size);
        }
    }
    for (int bucket = 0; bucket < (int)FUTURE_FIT_SIDES.size(); bucket++) {
        int representative_size = FUTURE_FIT_SIDES[bucket] * FUTURE_FIT_SIDES[bucket];
        int slots = 0;
        for (int component_size : component_sizes) {
            slots += component_size / representative_size;
            if (slots >= 2) break;
        }
        result.component_slots[bucket] = min(slots, 2);
    }
    return result;
}

long double cleanup_space_availability(const CleanupSpaceProfile &profile, int bucket, int n) {
    int side = FUTURE_FIT_SIDES[bucket];
    int maximum_anchors = (n - side + 1) * (n - side + 1);
    long double square_flexibility = maximum_anchors <= 0 ? 0.0L
                                                          : log1pl((long double)profile.square_anchors[bucket]) /
                                                                log1pl((long double)maximum_anchors);
    long double component_capacity = 0.5L * profile.component_slots[bucket];
    return 0.8L * square_flexibility + 0.2L * component_capacity;
}

long double cleanup_pre_arrival_space_score(const CleanupSpaceProfile &profile,
                                            const FutureSpaceValueModel &space_value_model, int n) {
    long double weighted_score = 0.0L;
    long double weight_sum = 0.0L;
    for (int bucket = 0; bucket < (int)FUTURE_FIT_SIDES.size(); bucket++) {
        long double weight =
            space_value_model.size_probability[bucket] * space_value_model.compact_fee_coefficient[bucket];
        weighted_score += weight * cleanup_space_availability(profile, bucket, n);
        weight_sum += weight;
    }
    return weight_sum <= 0.0L ? 0.0L : weighted_score / weight_sum;
}

optional<NormalPlacementChoice> choose_temporally_coherent_region(const vs &park, const vvi &owner,
                                                                  const vector<GroupState> &groups, ll current_s,
                                                                  ll arrival_t, int p, long double theta,
                                                                  int remaining_groups, const vector<Shape> &shapes,
                                                                  TemporalPlacementDiagnostics &diagnostics) {
    // Prefer boundaries next to groups with similar release timing.  Only a
    // small spatial shortlist proceeds to the more expensive future-fit test.
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

        vector<vector<Cell>> growth_candidates = make_connected_growth_candidates(park, owner, p);
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
    }

    vector<PlacementCandidate> candidates = shortlist_builder.finalize();
    if (candidates.empty()) return nullopt;
    diagnostics.shortlisted_candidates += candidates.size();

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
    if ((int)candidates.size() >= 2 && remaining_groups > 0 && arrival_t - current_s > 1 && future_mass > 1e-12L) {
        array<ll, FUTURE_FIT_SNAPSHOT_COUNT> snapshots = make_future_fit_snapshots(future_demand, current_s, arrival_t);
        long double best_fit = -numeric_limits<long double>::infinity();
        for (int index = 0; index < (int)candidates.size(); index++) {
            long double fit = evaluate_compact_fit(park, owner, groups, candidates[index].cells, snapshots);
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
};

FreeComponents label_free_components(const vs &park, const vvi &owner) {
    int n = park.size();
    FreeComponents result{vvi(n, vi(n, -1)), {}};
    constexpr int DX[4] = {-1, 1, 0, 0};
    constexpr int DY[4] = {0, 0, -1, 1};

    for (int start_x = 0; start_x < n; start_x++) {
        for (int start_y = 0; start_y < n; start_y++) {
            if (park[start_x][start_y] == '#' || owner[start_x][start_y] != -1 || result.id[start_x][start_y] != -1) {
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
        }
    }
    return result;
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

struct CleanupDiagnostics {
    int attempts = 0;
    int successes = 0;
    int successes_with_arrival = 0;
    int successes_without_arrival = 0;
    int search_budget_exhausted = 0;
    int evaluation_budget_exhausted = 0;
    int rollout_reuse_blocked = 0;
    int rollout_generation_failures = 0;
    int no_mover = 0;
    int no_destination = 0;
    int pre_arrival_worsened = 0;
    int economic_rejections = 0;
    int validation_failures = 0;
    long long fresh_cells = 0;
    long long movers_considered = 0;
    long long destination_anchors = 0;
    long long destination_candidates = 0;
    long long candidate_evaluations = 0;
    long long finalist_evaluations = 0;
    int rollout_turns = 0;
    bool rollout_used = false;
    long long rollout_policy_steps = 0;
    long long rollout_acceptances = 0;
    long long rollout_candidates_compared = 0;
    int rollout_multi_candidate_turns = 0;
    int rollout_full_width_turns = 0;
    int rollout_nonprimary_wins = 0;
    long long rollout_winner_rank_sum = 0;
    int rollout_max_winner_rank = 0;
    ll move_cost_sum = 0;
    ll fee_loss_sum = 0;
    ll current_fee_gain_sum = 0;
    long double pre_arrival_score_gain_sum = 0.0L;
    long double rollout_future_gain_sum = 0.0L;
    long double rollout_margin_sum = 0.0L;
};

struct CleanupMoverCandidate {
    int id = -1;
    int merged_gain = 0;
    int unlocked_size = 0;
    int perimeter_excess = 0;
    ll movement_cost = 0;
    ll space_time_gain = 0;
};

struct CleanupDestinationSeed {
    int shape_index = -1;
    int base_x = 0;
    int base_y = 0;
    int perimeter = 0;
    int fresh_overlap = 0;
    int quadrant = 0;
    ll fee_loss = 0;
    long long order = 0;
};

struct CleanupDestination {
    vector<Cell> cells;
    int perimeter = 0;
    ll fee_loss = 0;
};

vector<vi> make_cleanup_mask_prefix(const vector<char> &mask, int n) {
    vector<vi> prefix(n + 1, vi(n + 1));
    for (int x = 0; x < n; x++) {
        for (int y = 0; y < n; y++) {
            prefix[x + 1][y + 1] = mask[x * n + y] + prefix[x][y + 1] + prefix[x + 1][y] - prefix[x][y];
        }
    }
    return prefix;
}

bool cleanup_destination_economic_less(const CleanupDestinationSeed &lhs, const CleanupDestinationSeed &rhs) {
    if (lhs.fee_loss != rhs.fee_loss) return lhs.fee_loss < rhs.fee_loss;
    if (lhs.perimeter != rhs.perimeter) {
        return lhs.perimeter < rhs.perimeter;
    }
    if (lhs.fresh_overlap != rhs.fresh_overlap) {
        return lhs.fresh_overlap > rhs.fresh_overlap;
    }
    return lhs.order < rhs.order;
}

bool cleanup_destination_fresh_less(const CleanupDestinationSeed &lhs, const CleanupDestinationSeed &rhs) {
    if (lhs.fresh_overlap != rhs.fresh_overlap) {
        return lhs.fresh_overlap > rhs.fresh_overlap;
    }
    return cleanup_destination_economic_less(lhs, rhs);
}

vector<CleanupDestination> make_cleanup_destinations(const vs &park, const vvi &baseline_owner,
                                                     const vector<GroupState> &groups, int mover_id,
                                                     const vector<char> &fresh_mask,
                                                     const vector<vector<Shape>> &compact_shapes,
                                                     CleanupDiagnostics &diagnostics) {
    // Cleanup means moving one existing group into space freed on this turn.
    // Requiring at least 75% fresh overlap prevents an unrelated board-wide
    // relocation search and focuses the move on consolidating the new hole.
    int n = park.size();
    const GroupState &group = groups[mover_id];
    vvi scratch_owner = baseline_owner;
    clear_cells(scratch_owner, group.cells);
    vector<vi> blocked_prefix = make_blocked_prefix(park, scratch_owner);
    vector<vi> fresh_prefix = make_cleanup_mask_prefix(fresh_mask, n);
    vector<char> old_mask(n * n, false);
    for (auto [x, y] : group.cells) old_mask[x * n + y] = true;
    vector<vi> old_prefix = make_cleanup_mask_prefix(old_mask, n);

    const vector<Shape> &shapes = compact_shapes[group.p];
    vector<int> samples(shapes.size());
    vector<int> anchor_counts(shapes.size());
    vector<int> starts(shapes.size());
    vector<int> strides(shapes.size());
    for (int shape_index = 0; shape_index < (int)shapes.size(); shape_index++) {
        const Shape &shape = shapes[shape_index];
        int count = (n - shape.h + 1) * (n - shape.w + 1);
        anchor_counts[shape_index] = count;
        starts[shape_index] = (int)(((long long)(mover_id + 1) * 1009 + (long long)(shape_index + 1) * 9176) % count);
        int stride = max(1, count / 2 + 1 + shape_index % 7);
        while (std::gcd(stride, count) != 1) stride++;
        strides[shape_index] = stride;
    }

    optional<CleanupDestinationSeed> economic_best;
    optional<CleanupDestinationSeed> fresh_best;
    optional<CleanupDestinationSeed> first_candidate;
    array<optional<CleanupDestinationSeed>, 4> quadrant_best;
    long long local_anchors = 0;
    long long order = 0;
    int minimum_fresh_overlap = (CLEANUP_FRESH_OVERLAP_NUMERATOR * group.p + CLEANUP_FRESH_OVERLAP_DENOMINATOR - 1) /
                                CLEANUP_FRESH_OVERLAP_DENOMINATOR;
    ll previous_fee = round_payment(group.v, group.p, group.max_perimeter);

    bool exhausted = false;
    while (!exhausted && local_anchors < CLEANUP_DESTINATION_ANCHOR_LIMIT) {
        bool progressed = false;
        for (int shape_index = 0; shape_index < (int)shapes.size(); shape_index++) {
            if (local_anchors >= CLEANUP_DESTINATION_ANCHOR_LIMIT ||
                diagnostics.destination_anchors >= CLEANUP_DESTINATION_ANCHOR_GLOBAL_LIMIT) {
                exhausted = true;
                break;
            }
            if (samples[shape_index] >= anchor_counts[shape_index]) {
                continue;
            }
            progressed = true;
            const Shape &shape = shapes[shape_index];
            int columns = n - shape.w + 1;
            int flat = (starts[shape_index] + (long long)samples[shape_index] * strides[shape_index]) %
                       anchor_counts[shape_index];
            samples[shape_index]++;
            local_anchors++;
            diagnostics.destination_anchors++;
            int base_x = flat / columns;
            int base_y = flat % columns;
            const Rect &a = shape.main_rect;
            const Rect &b = shape.extra_rect;
            if (rectangle_sum(blocked_prefix, base_x + a.x, base_y + a.y, a.h, a.w) != 0 ||
                rectangle_sum(blocked_prefix, base_x + b.x, base_y + b.y, b.h, b.w) != 0) {
                continue;
            }
            int old_overlap = rectangle_sum(old_prefix, base_x + a.x, base_y + a.y, a.h, a.w) +
                              rectangle_sum(old_prefix, base_x + b.x, base_y + b.y, b.h, b.w);
            if (old_overlap == group.p) continue;
            int fresh_overlap = rectangle_sum(fresh_prefix, base_x + a.x, base_y + a.y, a.h, a.w) +
                                rectangle_sum(fresh_prefix, base_x + b.x, base_y + b.y, b.h, b.w);
            if (fresh_overlap < minimum_fresh_overlap) continue;

            ll next_fee = round_payment(group.v, group.p, max(group.max_perimeter, shape.perimeter));
            int lower_half = 2 * base_x + shape.h >= n;
            int right_half = 2 * base_y + shape.w >= n;
            CleanupDestinationSeed seed{shape_index,
                                        base_x,
                                        base_y,
                                        shape.perimeter,
                                        fresh_overlap,
                                        2 * lower_half + right_half,
                                        previous_fee - next_fee,
                                        order++};
            diagnostics.destination_candidates++;
            if (!first_candidate) first_candidate = seed;
            if (!economic_best || cleanup_destination_economic_less(seed, *economic_best)) {
                economic_best = seed;
            }
            if (!fresh_best || cleanup_destination_fresh_less(seed, *fresh_best)) {
                fresh_best = seed;
            }
            if (!quadrant_best[seed.quadrant] ||
                cleanup_destination_economic_less(seed, *quadrant_best[seed.quadrant])) {
                quadrant_best[seed.quadrant] = seed;
            }
        }
        if (!progressed) break;
    }

    vector<CleanupDestinationSeed> selected_seeds;
    auto add_seed = [&](const optional<CleanupDestinationSeed> &seed) {
        if (!seed) return;
        for (const CleanupDestinationSeed &existing : selected_seeds) {
            if (existing.shape_index == seed->shape_index && existing.base_x == seed->base_x &&
                existing.base_y == seed->base_y) {
                return;
            }
        }
        selected_seeds.push_back(*seed);
    };
    add_seed(economic_best);
    add_seed(fresh_best);

    int primary_quadrant = economic_best ? economic_best->quadrant : -1;
    optional<CleanupDestinationSeed> diverse;
    for (int quadrant = 0; quadrant < 4; quadrant++) {
        if (quadrant == primary_quadrant || !quadrant_best[quadrant]) {
            continue;
        }
        if (!diverse || cleanup_destination_fresh_less(*quadrant_best[quadrant], *diverse)) {
            diverse = quadrant_best[quadrant];
        }
    }
    add_seed(diverse);
    add_seed(first_candidate);
    for (const auto &seed : quadrant_best) add_seed(seed);

    vector<CleanupDestination> result;
    for (const CleanupDestinationSeed &seed : selected_seeds) {
        vector<Cell> cells = materialize_shape(shapes[seed.shape_index], seed.base_x, seed.base_y, group.p);
        bool duplicate = false;
        for (const CleanupDestination &existing : result) {
            if (same_region(existing.cells, cells)) {
                duplicate = true;
                break;
            }
        }
        if (duplicate) continue;
        result.push_back({std::move(cells), seed.perimeter, seed.fee_loss});
        if ((int)result.size() == CLEANUP_DESTINATION_LIMIT) break;
    }
    return result;
}

bool validate_and_build_cleanup_owner(const TurnPlan &plan, const vs &park, const vvi &owner,
                                      const vector<GroupState> &groups, int arrival_id, int r_milli, vvi &final_owner,
                                      ll &fee_loss) {
    if (plan.moves.size() != 1 || groups[arrival_id].active) return false;
    int n = park.size();
    const MovePlan &move = plan.moves.front();
    if (move.id < 0 || move.id >= (int)groups.size() || !groups[move.id].active) {
        return false;
    }
    const GroupState &group = groups[move.id];
    if ((int)group.cells.size() != group.p) return false;

    final_owner = owner;
    for (auto [x, y] : group.cells) {
        if (!inside(x, y, n, n) || final_owner[x][y] != move.id) {
            return false;
        }
    }
    clear_cells(final_owner, group.cells);

    auto region_is_legal = [&](const vector<Cell> &cells, int expected_size) {
        if ((int)cells.size() != expected_size) return false;
        for (auto [x, y] : cells) {
            if (!inside(x, y, n, n) || park[x][y] != '.' || final_owner[x][y] != -1) {
                return false;
            }
        }
        return validate_connected_region(cells, n);
    };

    if (!region_is_legal(move.cells, group.p) || same_region(move.cells, group.cells)) {
        return false;
    }
    int move_perimeter = calc_perimeter(move.cells, n);
    if (move_perimeter != move.perimeter) return false;
    ll previous_fee = round_payment(group.v, group.p, group.max_perimeter);
    ll next_fee = round_payment(group.v, group.p, max(group.max_perimeter, move_perimeter));
    fee_loss = previous_fee - next_fee;
    place_cells(final_owner, move.cells, move.id);

    ll expected_gain = -move_cost(group.v, r_milli);
    if (plan.arrival) {
        if (!region_is_legal(*plan.arrival, groups[arrival_id].p)) {
            return false;
        }
        if (calc_perimeter(*plan.arrival, n) != plan.arrival_perimeter) {
            return false;
        }
        expected_gain += round_payment(groups[arrival_id].v, groups[arrival_id].p, plan.arrival_perimeter);
        place_cells(final_owner, *plan.arrival, arrival_id);
    } else if (plan.arrival_perimeter != 0) {
        return false;
    }
    return expected_gain == plan.immediate_gain;
}

enum class CleanupArrivalStatus {
    UpperBoundRejected,
    NoRegion,
    ActualFeeRejected,
    Accepted,
};

struct CleanupArrivalDecision {
    optional<vector<Cell>> cells;
    int perimeter = 0;
    ll fee = 0;
    CleanupArrivalStatus status = CleanupArrivalStatus::NoRegion;
    TemporalPlacementDiagnostics diagnostics;
};

void merge_temporal_placement_diagnostics(TemporalPlacementDiagnostics &total,
                                          const TemporalPlacementDiagnostics &part) {
    total.attempts += part.attempts;
    total.compact_successes += part.compact_successes;
    total.extended_template_successes += part.extended_template_successes;
    total.fallback_successes += part.fallback_successes;
    total.future_fit_evaluated_turns += part.future_fit_evaluated_turns;
    total.future_fit_changed_placements += part.future_fit_changed_placements;
    total.incremental_changed_from_absolute += part.incremental_changed_from_absolute;
    total.final_changed_from_absolute += part.final_changed_from_absolute;
    total.anchors_checked += part.anchors_checked;
    total.legal_compact_candidates += part.legal_compact_candidates;
    total.connected_growth_candidates += part.connected_growth_candidates;
    total.shortlisted_candidates += part.shortlisted_candidates;
    total.future_fit_snapshots += part.future_fit_snapshots;
}

CleanupArrivalDecision evaluate_cleanup_arrival_decision(const vs &park, const vvi &decision_owner,
                                                         const vector<GroupState> &groups, int arrival_id, ll current_s,
                                                         int remaining_groups, long double theta,
                                                         long double opportunity_cost,
                                                         const vector<vector<Shape>> &compact_shapes) {
    CleanupArrivalDecision result;
    const GroupState &arrival = groups[arrival_id];
    int minimum_perimeter = compact_shapes[arrival.p].front().perimeter;
    ll upper_bound_fee = round_payment(arrival.v, arrival.p, minimum_perimeter);
    if ((long double)upper_bound_fee <= opportunity_cost) {
        result.status = CleanupArrivalStatus::UpperBoundRejected;
        return result;
    }

    optional<NormalPlacementChoice> placement =
        choose_temporally_coherent_region(park, decision_owner, groups, current_s, arrival.t, arrival.p, theta,
                                          remaining_groups, compact_shapes[arrival.p], result.diagnostics);
    if (!placement) {
        result.status = CleanupArrivalStatus::NoRegion;
        return result;
    }

    ll actual_fee = round_payment(arrival.v, arrival.p, placement->perimeter);
    if ((long double)actual_fee <= opportunity_cost) {
        result.status = CleanupArrivalStatus::ActualFeeRejected;
        return result;
    }
    result.cells = std::move(placement->cells);
    result.perimeter = placement->perimeter;
    result.fee = actual_fee;
    result.status = CleanupArrivalStatus::Accepted;
    return result;
}

TurnPlan make_arrival_turn_plan(const CleanupArrivalDecision &decision) {
    TurnPlan plan;
    if (decision.cells) {
        plan.arrival = *decision.cells;
        plan.arrival_perimeter = decision.perimeter;
    }
    plan.immediate_gain = decision.fee;
    return plan;
}

struct CleanupSyntheticArrival {
    ll s = 0;
    ll t = 0;
    int p = 0;
    ll v = 0;
    long double theta = 0.0L;
    int remaining_after = 0;
};

long double cleanup_radical_inverse(uint64_t index, int base) {
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

uint64_t cleanup_sequence_offset(const vector<GroupState> &groups, int arrival_id, ll current_s) {
    uint64_t value = (uint64_t)(arrival_id + 1) * 0x9e3779b97f4a7c15ULL;
    value ^= (uint64_t)current_s + 0xbf58476d1ce4e5b9ULL;
    value ^= (uint64_t)groups[arrival_id].v * 0x94d049bb133111ebULL;
    value = (value ^ (value >> 30)) * 0xbf58476d1ce4e5b9ULL;
    value = (value ^ (value >> 27)) * 0x94d049bb133111ebULL;
    return (value ^ (value >> 31)) % 1000003ULL;
}

array<vector<CleanupSyntheticArrival>, CLEANUP_ROLLOUT_SCENARIO_COUNT> make_cleanup_rollout_scenarios(
    const vector<GroupState> &groups, int arrival_id, ll current_s, int remaining_groups, long double theta,
    const ThetaEstimator &theta_estimator) {
    // Generate every remaining sample first, sort by start time, then take the
    // first 48.  Stopping generation at 48 would bias the chronological prefix.
    array<vector<CleanupSyntheticArrival>, CLEANUP_ROLLOUT_SCENARIO_COUNT> scenarios;
    if (remaining_groups <= 0) return scenarios;

    struct RawArrival {
        ll s;
        ll t;
        int p;
        ll v;
        long long order;
    };

    ConditionalFutureDemand future_demand(current_s, theta);
    uint64_t sequence_offset = cleanup_sequence_offset(groups, arrival_id, current_s);
    const long double size_width = sqrtl(150.0L) - 2.0L;

    for (int scenario = 0; scenario < CLEANUP_ROLLOUT_SCENARIO_COUNT; scenario++) {
        set<ll> used_times;
        for (int id = 0; id <= arrival_id; id++) {
            used_times.insert(groups[id].s);
            used_times.insert(groups[id].t);
        }

        vector<RawArrival> generated;
        generated.reserve(remaining_groups);
        int maximum_attempts = 8 * remaining_groups + 64;
        for (int attempt = 0; (int)generated.size() < remaining_groups && attempt < maximum_attempts; attempt++) {
            uint64_t index = sequence_offset + attempt + 1;
            auto quantile = [&](int base) {
                long double value = cleanup_radical_inverse(index, base);
                return scenario == 0 ? value : 1.0L - value;
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
            if (used_times.count(start) || used_times.count(end)) {
                continue;
            }
            used_times.insert(start);
            used_times.insert(end);
            generated.push_back({start, end, p, v, attempt});
        }

        sort(generated.begin(), generated.end(), [](const RawArrival &lhs, const RawArrival &rhs) {
            if (lhs.s != rhs.s) return lhs.s < rhs.s;
            if (lhs.t != rhs.t) return lhs.t < rhs.t;
            return lhs.order < rhs.order;
        });

        ThetaEstimator rollout_theta_estimator = theta_estimator;
        int rollout_length = min(CLEANUP_ROLLOUT_LENGTH, remaining_groups);
        for (const RawArrival &raw : generated) {
            int remaining_after = remaining_groups - (int)scenarios[scenario].size() - 1;
            rollout_theta_estimator.observe(raw.t - raw.s);
            long double rollout_theta = rollout_theta_estimator.estimate(raw.s, remaining_after);
            scenarios[scenario].push_back({raw.s, raw.t, raw.p, raw.v, rollout_theta, remaining_after});
            if ((int)scenarios[scenario].size() == rollout_length) {
                break;
            }
        }
    }
    return scenarios;
}

struct CleanupRolloutState {
    vvi owner;
    vector<GroupState> groups;
    priority_queue<pair<ll, int>, vector<pair<ll, int>>, greater<pair<ll, int>>> departures;
};

CleanupRolloutState make_cleanup_rollout_state(const vvi &final_owner, const vector<GroupState> &groups, int arrival_id,
                                               const TurnPlan &plan, int synthetic_count) {
    CleanupRolloutState state;
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
        if (state.groups[id].active) {
            state.departures.emplace(state.groups[id].t, id);
        }
    }
    return state;
}

ll evaluate_cleanup_rollout_branch(const vs &park, const vvi &final_owner, const vector<GroupState> &groups,
                                   int arrival_id, const TurnPlan &plan,
                                   const vector<CleanupSyntheticArrival> &scenario, int grass_cells,
                                   const DensityModel &density_model, const vector<vector<Shape>> &compact_shapes,
                                   CleanupDiagnostics &diagnostics) {
    // Rollout arrivals follow the ordinary shadow-price admission and placement
    // policy.  They never trigger another relocation inside the simulation.
    CleanupRolloutState state = make_cleanup_rollout_state(final_owner, groups, arrival_id, plan, scenario.size());
    ll result = 0;
    for (const CleanupSyntheticArrival &spec : scenario) {
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

        ShadowEvaluation shadow = evaluate_shadow_cost(state.groups, spec.s, spec.t, spec.p, spec.remaining_after,
                                                       grass_cells, spec.theta, density_model);
        CleanupArrivalDecision decision = evaluate_cleanup_arrival_decision(
            park, state.owner, state.groups, synthetic_id, spec.s, spec.remaining_after, spec.theta,
            shadow.opportunity_cost, compact_shapes);
        diagnostics.rollout_policy_steps++;
        if (!decision.cells) continue;

        place_cells(state.owner, *decision.cells, synthetic_id);
        synthetic.active = true;
        synthetic.cells = *decision.cells;
        synthetic.max_perimeter = decision.perimeter;
        state.departures.emplace(spec.t, synthetic_id);
        result += decision.fee;
        diagnostics.rollout_acceptances++;
    }
    return result;
}

struct PreparedCleanupCandidate {
    MovePlan move;
    vvi moved_owner;
    ll movement_cost = 0;
    ll fee_loss = 0;
    long double pre_arrival_gain = 0.0L;
    long long order = 0;
};

struct ProactiveCleanupResult {
    TurnPlan plan;
    CleanupArrivalDecision arrival_decision;
};

optional<ProactiveCleanupResult> try_proactive_cleanup(
    const vs &park, const vvi &owner, const vector<GroupState> &groups, int arrival_id, ll current_s,
    int remaining_groups, int r_milli, long double theta, const ThetaEstimator &theta_estimator,
    const DensityModel &density_model, int grass_cells, long double opportunity_cost,
    const CleanupArrivalDecision &baseline_arrival, const vector<char> &fresh_mask,
    const vector<vector<Shape>> &compact_shapes, const FutureSpaceValueModel &space_value_model,
    CleanupDiagnostics &diagnostics) {
    // Phase 1 cheaply ranks legal moves by the current pre-arrival board.
    // Phase 2 compares the best two against the no-move branch using identical
    // synthetic futures; the cheap score itself is not part of the final gain.
    if (diagnostics.rollout_used) {
        diagnostics.rollout_reuse_blocked++;
        return nullopt;
    }
    if (diagnostics.attempts >= CLEANUP_SEARCH_LIMIT) {
        diagnostics.search_budget_exhausted++;
        return nullopt;
    }
    if (diagnostics.candidate_evaluations >= CLEANUP_CANDIDATE_EVALUATION_LIMIT) {
        diagnostics.evaluation_budget_exhausted++;
        return nullopt;
    }
    int n = park.size();
    int fresh_available = 0;
    for (int x = 0; x < n; x++) {
        for (int y = 0; y < n; y++) {
            if (fresh_mask[x * n + y] && owner[x][y] == -1) {
                fresh_available++;
            }
        }
    }
    if (fresh_available < 4) return nullopt;
    diagnostics.attempts++;

    FreeComponents components = label_free_components(park, owner);
    constexpr int DX[4] = {-1, 1, 0, 0};
    constexpr int DY[4] = {0, 0, -1, 1};
    vector<CleanupMoverCandidate> movers;
    for (int id = 0; id < arrival_id; id++) {
        const GroupState &group = groups[id];
        if (!group.active || group.t <= current_s ||
            fresh_available * CLEANUP_FRESH_OVERLAP_DENOMINATOR < group.p * CLEANUP_FRESH_OVERLAP_NUMERATOR) {
            continue;
        }
        vector<int> adjacent_components;
        for (auto [x, y] : group.cells) {
            for (int dir = 0; dir < 4; dir++) {
                int nx = x + DX[dir];
                int ny = y + DY[dir];
                if (!inside(nx, ny, n, n)) continue;
                int component = components.id[nx][ny];
                if (component == -1 || find(adjacent_components.begin(), adjacent_components.end(), component) !=
                                           adjacent_components.end()) {
                    continue;
                }
                adjacent_components.push_back(component);
            }
        }
        int neighbor_sum = 0;
        int largest_neighbor = 0;
        for (int component : adjacent_components) {
            neighbor_sum += components.size[component];
            chmax(largest_neighbor, components.size[component]);
        }
        int unlocked_size = group.p + neighbor_sum;
        int merged_gain = unlocked_size - largest_neighbor;
        int current_perimeter = calc_perimeter(group.cells, n);
        int minimum_perimeter = compact_shapes[group.p].front().perimeter;
        movers.push_back({id, merged_gain, unlocked_size, max(0, current_perimeter - minimum_perimeter),
                          move_cost(group.v, r_milli), (ll)merged_gain * (group.t - current_s)});
    }
    sort(movers.begin(), movers.end(), [&](const CleanupMoverCandidate &lhs, const CleanupMoverCandidate &rhs) {
        __int128 left_priority = (__int128)lhs.space_time_gain * rhs.movement_cost;
        __int128 right_priority = (__int128)rhs.space_time_gain * lhs.movement_cost;
        if (left_priority != right_priority) {
            return left_priority > right_priority;
        }
        if (lhs.merged_gain != rhs.merged_gain) {
            return lhs.merged_gain > rhs.merged_gain;
        }
        if (lhs.perimeter_excess != rhs.perimeter_excess) {
            return lhs.perimeter_excess > rhs.perimeter_excess;
        }
        if (lhs.unlocked_size != rhs.unlocked_size) {
            return lhs.unlocked_size > rhs.unlocked_size;
        }
        const GroupState &left_group = groups[lhs.id];
        const GroupState &right_group = groups[rhs.id];
        __int128 left_ratio = (__int128)lhs.movement_cost * right_group.p;
        __int128 right_ratio = (__int128)rhs.movement_cost * left_group.p;
        if (left_ratio != right_ratio) return left_ratio < right_ratio;
        if (left_group.t != right_group.t) {
            return left_group.t > right_group.t;
        }
        return lhs.id < rhs.id;
    });
    if ((int)movers.size() > CLEANUP_MOVER_LIMIT) {
        movers.resize(CLEANUP_MOVER_LIMIT);
    }
    if (movers.empty()) {
        diagnostics.no_mover++;
        return nullopt;
    }

    CleanupSpaceProfile baseline_pre_profile = make_cleanup_space_profile(park, owner, groups, current_s);
    long double baseline_pre_score = cleanup_pre_arrival_space_score(baseline_pre_profile, space_value_model, n);
    vector<PreparedCleanupCandidate> prepared;
    long long candidate_order = 0;

    auto prepared_less = [](const PreparedCleanupCandidate &lhs, const PreparedCleanupCandidate &rhs) {
        ll lhs_cost = lhs.movement_cost + lhs.fee_loss;
        ll rhs_cost = rhs.movement_cost + rhs.fee_loss;
        long double lhs_efficiency = lhs.pre_arrival_gain * rhs_cost;
        long double rhs_efficiency = rhs.pre_arrival_gain * lhs_cost;
        if (lhs_efficiency != rhs_efficiency) {
            return lhs_efficiency > rhs_efficiency;
        }
        if (lhs.pre_arrival_gain != rhs.pre_arrival_gain) {
            return lhs.pre_arrival_gain > rhs.pre_arrival_gain;
        }
        if (lhs_cost != rhs_cost) return lhs_cost < rhs_cost;
        return lhs.order < rhs.order;
    };

    for (const CleanupMoverCandidate &mover : movers) {
        if (diagnostics.candidate_evaluations >= CLEANUP_CANDIDATE_EVALUATION_LIMIT) {
            diagnostics.evaluation_budget_exhausted++;
            break;
        }
        diagnostics.movers_considered++;
        vector<CleanupDestination> destinations =
            make_cleanup_destinations(park, owner, groups, mover.id, fresh_mask, compact_shapes, diagnostics);
        if (destinations.empty()) continue;

        for (const CleanupDestination &destination : destinations) {
            if (diagnostics.candidate_evaluations >= CLEANUP_CANDIDATE_EVALUATION_LIMIT) {
                diagnostics.evaluation_budget_exhausted++;
                break;
            }
            long long this_candidate_order = candidate_order++;
            TurnPlan move_only;
            move_only.moves.push_back({mover.id, destination.cells, destination.perimeter});
            move_only.immediate_gain = -mover.movement_cost;

            vvi moved_owner;
            ll fee_loss = 0;
            if (!validate_and_build_cleanup_owner(move_only, park, owner, groups, arrival_id, r_milli, moved_owner,
                                                  fee_loss) ||
                fee_loss != destination.fee_loss) {
                diagnostics.validation_failures++;
                continue;
            }
            diagnostics.candidate_evaluations++;
            CleanupSpaceProfile candidate_pre_profile =
                make_cleanup_space_profile(park, moved_owner, groups, current_s);
            long double candidate_pre_score =
                cleanup_pre_arrival_space_score(candidate_pre_profile, space_value_model, n);
            long double pre_arrival_gain = candidate_pre_score - baseline_pre_score;
            if (pre_arrival_gain < -1e-15L) {
                diagnostics.pre_arrival_worsened++;
                continue;
            }

            prepared.push_back({move_only.moves.front(), std::move(moved_owner), mover.movement_cost, fee_loss,
                                pre_arrival_gain, this_candidate_order});
            sort(prepared.begin(), prepared.end(), prepared_less);
            if ((int)prepared.size() > CLEANUP_FINALIST_LIMIT) {
                prepared.resize(CLEANUP_FINALIST_LIMIT);
            }
        }
    }
    if (prepared.empty()) {
        diagnostics.no_destination++;
        return nullopt;
    }

    auto rollout_scenarios =
        make_cleanup_rollout_scenarios(groups, arrival_id, current_s, remaining_groups, theta, theta_estimator);
    int expected_rollout_length = min(CLEANUP_ROLLOUT_LENGTH, remaining_groups);
    if (expected_rollout_length == 0) return nullopt;
    for (const auto &scenario : rollout_scenarios) {
        if ((int)scenario.size() != expected_rollout_length) {
            diagnostics.rollout_generation_failures++;
            return nullopt;
        }
    }
    // A case gets exactly one rollout comparison opportunity, regardless of
    // how many finalists are available or how deep the rollout is.
    diagnostics.rollout_used = true;
    diagnostics.rollout_turns++;

    vvi baseline_final_owner = owner;
    if (baseline_arrival.cells) {
        place_cells(baseline_final_owner, *baseline_arrival.cells, arrival_id);
    }
    TurnPlan baseline_plan = make_arrival_turn_plan(baseline_arrival);
    array<ll, CLEANUP_ROLLOUT_SCENARIO_COUNT> baseline_rollout{};
    for (int scenario = 0; scenario < CLEANUP_ROLLOUT_SCENARIO_COUNT; scenario++) {
        baseline_rollout[scenario] = evaluate_cleanup_rollout_branch(
            park, baseline_final_owner, groups, arrival_id, baseline_plan, rollout_scenarios[scenario], grass_cells,
            density_model, compact_shapes, diagnostics);
    }

    optional<ProactiveCleanupResult> best_result;
    ll best_movement_cost = 0;
    ll best_fee_loss = 0;
    ll best_current_gain = 0;
    long double best_pre_arrival_gain = 0.0L;
    long double best_future_gain = 0.0L;
    long double best_margin = -numeric_limits<long double>::infinity();
    long long best_order = numeric_limits<long long>::max();
    int best_candidate_rank = -1;
    int compared_candidates = 0;

    for (int candidate_rank = 0; candidate_rank < (int)prepared.size(); candidate_rank++) {
        const PreparedCleanupCandidate &candidate = prepared[candidate_rank];
        diagnostics.finalist_evaluations++;

        CleanupArrivalDecision candidate_arrival =
            evaluate_cleanup_arrival_decision(park, candidate.moved_owner, groups, arrival_id, current_s,
                                              remaining_groups, theta, opportunity_cost, compact_shapes);
        TurnPlan candidate_plan = make_arrival_turn_plan(candidate_arrival);
        candidate_plan.moves.push_back(candidate.move);
        candidate_plan.immediate_gain = candidate_arrival.fee - candidate.movement_cost;

        vvi candidate_final_owner;
        ll fee_loss = 0;
        if (!validate_and_build_cleanup_owner(candidate_plan, park, owner, groups, arrival_id, r_milli,
                                              candidate_final_owner, fee_loss) ||
            fee_loss != candidate.fee_loss) {
            diagnostics.validation_failures++;
            continue;
        }

        ll current_gain = candidate_arrival.fee - baseline_arrival.fee;
        ll economic_cost = candidate.movement_cost + fee_loss;
        long double margin = numeric_limits<long double>::infinity();
        long double minimum_future_gain = numeric_limits<long double>::infinity();
        compared_candidates++;
        diagnostics.rollout_candidates_compared++;
        for (int scenario = 0; scenario < CLEANUP_ROLLOUT_SCENARIO_COUNT; scenario++) {
            ll candidate_rollout = evaluate_cleanup_rollout_branch(
                park, candidate_final_owner, groups, arrival_id, candidate_plan, rollout_scenarios[scenario],
                grass_cells, density_model, compact_shapes, diagnostics);
            long double future_gain = candidate_rollout - baseline_rollout[scenario];
            chmin(minimum_future_gain, future_gain);
            chmin(margin, current_gain + future_gain - economic_cost);
        }
        // Be conservative across the mirrored futures.  A move must repay its
        // movement cost and any permanent fee loss even in the worse scenario.
        if (margin <= 0.0L) {
            diagnostics.economic_rejections++;
            continue;
        }

        bool better = !best_result || margin > best_margin + 1e-12L;
        if (best_result && fabsl(margin - best_margin) <= 1e-12L) {
            ll best_economic_cost = best_movement_cost + best_fee_loss;
            if (economic_cost != best_economic_cost) {
                better = economic_cost < best_economic_cost;
            } else if (minimum_future_gain != best_future_gain) {
                better = minimum_future_gain > best_future_gain;
            } else {
                better = candidate.order < best_order;
            }
        }
        if (!better) continue;

        best_result = ProactiveCleanupResult{std::move(candidate_plan), std::move(candidate_arrival)};
        best_movement_cost = candidate.movement_cost;
        best_fee_loss = fee_loss;
        best_current_gain = current_gain;
        best_pre_arrival_gain = candidate.pre_arrival_gain;
        best_future_gain = minimum_future_gain;
        best_margin = margin;
        best_order = candidate.order;
        best_candidate_rank = candidate_rank;
    }

    if (compared_candidates >= 2) {
        diagnostics.rollout_multi_candidate_turns++;
    }
    if (compared_candidates == CLEANUP_FINALIST_LIMIT) {
        diagnostics.rollout_full_width_turns++;
    }
    if (!best_result) return nullopt;
    if (best_candidate_rank > 0) {
        diagnostics.rollout_nonprimary_wins++;
    }
    int winner_rank = best_candidate_rank + 1;
    diagnostics.rollout_winner_rank_sum += winner_rank;
    chmax(diagnostics.rollout_max_winner_rank, winner_rank);
    diagnostics.successes++;
    if (best_result->plan.arrival) {
        diagnostics.successes_with_arrival++;
    } else {
        diagnostics.successes_without_arrival++;
    }
    diagnostics.move_cost_sum += best_movement_cost;
    diagnostics.fee_loss_sum += best_fee_loss;
    diagnostics.current_fee_gain_sum += best_current_gain;
    diagnostics.pre_arrival_score_gain_sum += best_pre_arrival_gain;
    diagnostics.rollout_future_gain_sum += best_future_gain;
    diagnostics.rollout_margin_sum += best_margin;
    return best_result;
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
    Timer timer;

    int N, M;
    ld R;
    cin >> N >> M >> R;
    int r_milli = (int)llroundl(R * 1000.0L);
    vs park(N);
    for (string &row : park) cin >> row;

    vector<vector<Shape>> compact_shapes(151);
    for (int p = 4; p <= 150; p++) {
        compact_shapes[p] = make_compact_shapes(p, N);
    }
    DensityModel density_model(compact_shapes);
    FutureSpaceValueModel space_value_model(compact_shapes);
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
    CleanupDiagnostics cleanup_diagnostics;

    for (int turn = 0; turn < M; turn++) {
        int i, P;
        ll S, T, V;
        cin >> i >> S >> T >> P >> V;

        groups[i].s = S;
        groups[i].t = T;
        groups[i].v = V;
        groups[i].p = P;
        theta_estimator.observe(T - S);
        int remaining_groups = M - i - 1;
        long double theta = theta_estimator.estimate(S, remaining_groups);

        vector<char> fresh_mask(N * N, false);
        // A departure at exactly S is still present by the problem's ordering;
        // only t < S is removed.  The released cells seed this turn's cleanup.
        while (!departures.empty() && departures.top().first < S) {
            int j = departures.top().second;
            departures.pop();
            if (!groups[j].active) continue;
            for (auto [x, y] : groups[j].cells) {
                fresh_mask[x * N + y] = true;
                cleanup_diagnostics.fresh_cells++;
            }
            clear_cells(owner, groups[j].cells);
            groups[j].cells.clear();
            groups[j].active = false;
        }

        ShadowEvaluation shadow =
            evaluate_shadow_cost(groups, S, T, P, remaining_groups, grass_cells, theta, density_model);
        shadow_diagnostics.considered++;
        shadow_diagnostics.theta_sum += theta;
        shadow_diagnostics.opportunity_cost_sum += shadow.opportunity_cost;
        shadow_diagnostics.rejected_fraction_sum += shadow.duration_weighted_rejected_fraction;
        chmax(shadow_diagnostics.maximum_rejected_fraction, shadow.maximum_rejected_fraction);
        shadow_diagnostics.priced_buckets += shadow.priced_buckets;

        // First compute the no-move branch as a counterfactual.  Cleanup is
        // still judged on the pre-arrival board, and the chosen branch emits
        // its move before the arrival decision.
        CleanupArrivalDecision baseline_arrival = evaluate_cleanup_arrival_decision(
            park, owner, groups, i, S, remaining_groups, theta, shadow.opportunity_cost, compact_shapes);
        optional<ProactiveCleanupResult> cleanup =
            try_proactive_cleanup(park, owner, groups, i, S, remaining_groups, r_milli, theta, theta_estimator,
                                  density_model, grass_cells, shadow.opportunity_cost, baseline_arrival, fresh_mask,
                                  compact_shapes, space_value_model, cleanup_diagnostics);

        TurnPlan plan;
        CleanupArrivalDecision selected_arrival;
        if (cleanup) {
            plan = std::move(cleanup->plan);
            selected_arrival = std::move(cleanup->arrival_decision);
        } else {
            plan = make_arrival_turn_plan(baseline_arrival);
            selected_arrival = std::move(baseline_arrival);
        }
        merge_temporal_placement_diagnostics(placement_diagnostics, selected_arrival.diagnostics);
        switch (selected_arrival.status) {
            case CleanupArrivalStatus::UpperBoundRejected:
                shadow_diagnostics.upper_bound_rejected++;
                break;
            case CleanupArrivalStatus::NoRegion:
                shadow_diagnostics.no_region_rejected++;
                break;
            case CleanupArrivalStatus::ActualFeeRejected:
                shadow_diagnostics.actual_fee_rejected++;
                break;
            case CleanupArrivalStatus::Accepted:
                shadow_diagnostics.accepted++;
                break;
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

    long double mean_theta =
        shadow_diagnostics.considered == 0 ? 0.0L : shadow_diagnostics.theta_sum / shadow_diagnostics.considered;
    long double mean_opportunity_cost = shadow_diagnostics.considered == 0
                                            ? 0.0L
                                            : shadow_diagnostics.opportunity_cost_sum / shadow_diagnostics.considered;
    long double mean_rejected_fraction = shadow_diagnostics.considered == 0
                                             ? 0.0L
                                             : shadow_diagnostics.rejected_fraction_sum / shadow_diagnostics.considered;
    cerr << "accepted=" << accepted_count << " rejected=" << rejected_count
         << " cleanup_attempts=" << cleanup_diagnostics.attempts
         << " cleanup_successes=" << cleanup_diagnostics.successes
         << " cleanup_with_arrival=" << cleanup_diagnostics.successes_with_arrival
         << " cleanup_without_arrival=" << cleanup_diagnostics.successes_without_arrival
         << " cleanup_search_budget_exhausted=" << cleanup_diagnostics.search_budget_exhausted
         << " cleanup_evaluation_budget_exhausted=" << cleanup_diagnostics.evaluation_budget_exhausted
         << " cleanup_rollout_reuse_blocked=" << cleanup_diagnostics.rollout_reuse_blocked
         << " cleanup_rollout_generation_failures=" << cleanup_diagnostics.rollout_generation_failures
         << " cleanup_no_mover=" << cleanup_diagnostics.no_mover
         << " cleanup_no_destination=" << cleanup_diagnostics.no_destination
         << " cleanup_pre_arrival_worsened=" << cleanup_diagnostics.pre_arrival_worsened
         << " cleanup_economic_rejections=" << cleanup_diagnostics.economic_rejections
         << " cleanup_validation_failures=" << cleanup_diagnostics.validation_failures
         << " cleanup_fresh_cells=" << cleanup_diagnostics.fresh_cells
         << " cleanup_movers_considered=" << cleanup_diagnostics.movers_considered
         << " cleanup_destination_anchors=" << cleanup_diagnostics.destination_anchors
         << " cleanup_destination_candidates=" << cleanup_diagnostics.destination_candidates
         << " cleanup_candidate_evaluations=" << cleanup_diagnostics.candidate_evaluations
         << " cleanup_finalist_evaluations=" << cleanup_diagnostics.finalist_evaluations
         << " cleanup_rollout_policy_steps=" << cleanup_diagnostics.rollout_policy_steps
         << " cleanup_rollout_turns=" << cleanup_diagnostics.rollout_turns
         << " cleanup_rollout_acceptances=" << cleanup_diagnostics.rollout_acceptances
         << " cleanup_rollout_candidates_compared=" << cleanup_diagnostics.rollout_candidates_compared
         << " cleanup_rollout_multi_candidate_turns=" << cleanup_diagnostics.rollout_multi_candidate_turns
         << " cleanup_rollout_full_width_turns=" << cleanup_diagnostics.rollout_full_width_turns
         << " cleanup_rollout_nonprimary_wins=" << cleanup_diagnostics.rollout_nonprimary_wins
         << " cleanup_rollout_winner_rank_sum=" << cleanup_diagnostics.rollout_winner_rank_sum
         << " cleanup_rollout_max_winner_rank=" << cleanup_diagnostics.rollout_max_winner_rank
         << " cleanup_move_cost=" << cleanup_diagnostics.move_cost_sum
         << " cleanup_fee_loss=" << cleanup_diagnostics.fee_loss_sum
         << " cleanup_current_fee_gain=" << cleanup_diagnostics.current_fee_gain_sum
         << " shadow_considered=" << shadow_diagnostics.considered
         << " shadow_upper_rejected=" << shadow_diagnostics.upper_bound_rejected
         << " shadow_actual_rejected=" << shadow_diagnostics.actual_fee_rejected
         << " shadow_no_region_rejected=" << shadow_diagnostics.no_region_rejected
         << " shadow_accepted=" << shadow_diagnostics.accepted
         << " placement_attempts=" << placement_diagnostics.attempts
         << " placement_compact_successes=" << placement_diagnostics.compact_successes
         << " placement_extended_template_successes=" << placement_diagnostics.extended_template_successes
         << " placement_fallback_successes=" << placement_diagnostics.fallback_successes
         << " placement_future_fit_turns=" << placement_diagnostics.future_fit_evaluated_turns
         << " placement_future_fit_changes=" << placement_diagnostics.future_fit_changed_placements
         << " placement_incremental_changes_from_absolute=" << placement_diagnostics.incremental_changed_from_absolute
         << " placement_final_changes_from_absolute=" << placement_diagnostics.final_changed_from_absolute
         << " placement_anchors_checked=" << placement_diagnostics.anchors_checked
         << " placement_legal_compact_candidates=" << placement_diagnostics.legal_compact_candidates
         << " placement_growth_candidates=" << placement_diagnostics.connected_growth_candidates
         << " placement_shortlisted_candidates=" << placement_diagnostics.shortlisted_candidates
         << " placement_future_fit_snapshots=" << placement_diagnostics.future_fit_snapshots << fixed << setprecision(6)
         << " theta_mean=" << mean_theta << " shadow_mean_opportunity=" << mean_opportunity_cost
         << " shadow_mean_rejected_fraction=" << mean_rejected_fraction
         << " shadow_max_rejected_fraction=" << shadow_diagnostics.maximum_rejected_fraction
         << " shadow_priced_buckets=" << shadow_diagnostics.priced_buckets
         << " model_expected_p=" << density_model.expected_group_size
         << " cleanup_rollout_future_gain=" << cleanup_diagnostics.rollout_future_gain_sum
         << " cleanup_pre_arrival_score_gain=" << cleanup_diagnostics.pre_arrival_score_gain_sum
         << " cleanup_rollout_margin=" << cleanup_diagnostics.rollout_margin_sum << " elapsed=" << setprecision(3)
         << timer.elapsed() << '\n';

    return 0;
}
