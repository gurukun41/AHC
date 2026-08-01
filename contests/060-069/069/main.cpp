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
constexpr int ROOT_SCREEN_SCENARIO_COUNT = 2;
constexpr int ROOT_SCREEN_ROLLOUT_LENGTH = 4;
constexpr int ROOT_CONFIRM_SCENARIO_COUNT = 8;
constexpr int ROOT_CONFIRM_ROLLOUT_LENGTH = 12;
constexpr int ROOT_CONFIRMATION_TURN_LIMIT = 4;
constexpr int ROOT_ROLLOUT_NORMAL_ALTERNATIVE_LIMIT = 2;
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

enum class PlacementSource {
    MinimumTemplate,
    ExtendedTemplate,
    ConnectedGrowth,
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
    if (choice.source == PlacementSource::ConnectedGrowth) {
        diagnostics.fallback_successes++;
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

void remove_selected_placement_success(TemporalPlacementDiagnostics &diagnostics) {
    if (diagnostics.fallback_successes > 0) {
        diagnostics.fallback_successes--;
        return;
    }
    if (diagnostics.compact_successes > 0) diagnostics.compact_successes--;
    if (diagnostics.extended_template_successes > 0) diagnostics.extended_template_successes--;
}

void replace_selected_placement_success(TemporalPlacementDiagnostics &diagnostics,
                                        PlacementSource source) {
    remove_selected_placement_success(diagnostics);
    if (source == PlacementSource::ConnectedGrowth) {
        diagnostics.fallback_successes++;
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
    int perimeter = 0;
    ll movement_cost = 0;
    ll immediate_improvement = 0;
    long long order = 0;
};

vector<RescueTarget> make_rescue_targets(const vs &park, const vvi &owner,
                                         const vector<GroupState> &groups, int arrival_id, int r_milli,
                                         const ArrivalDecision &baseline,
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

    vector<RescueTargetSeed> seeds;
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
                seeds.push_back({shape_index, base_x, base_y, occupied, fractional, order++});
            }
        }
    }

    vector<int> shortlisted;
    auto add_shortlist = [&](auto less) {
        vector<int> indices(seeds.size());
        iota(indices.begin(), indices.end(), 0);
        int keep = min(RESCUE_TARGET_SHORTLIST_PER_METRIC, (int)indices.size());
        partial_sort(indices.begin(), indices.begin() + keep, indices.end(),
                     [&](int lhs, int rhs) { return less(seeds[lhs], seeds[rhs]); });
        indices.resize(keep);
        shortlisted.insert(shortlisted.end(), indices.begin(), indices.end());
    };
    add_shortlist([](const RescueTargetSeed &lhs, const RescueTargetSeed &rhs) {
        return tie(lhs.occupied_cells, lhs.fractional_move_cost, lhs.order) <
               tie(rhs.occupied_cells, rhs.fractional_move_cost, rhs.order);
    });
    add_shortlist([](const RescueTargetSeed &lhs, const RescueTargetSeed &rhs) {
        return tie(lhs.fractional_move_cost, lhs.occupied_cells, lhs.order) <
               tie(rhs.fractional_move_cost, rhs.occupied_cells, rhs.order);
    });
    sort(shortlisted.begin(), shortlisted.end());
    shortlisted.erase(unique(shortlisted.begin(), shortlisted.end()), shortlisted.end());
    diagnostics.target_shortlisted += shortlisted.size();

    vector<RescueTarget> result;
    vector<int> seen(groups.size(), -1);
    int stamp = 0;
    for (int seed_index : shortlisted) {
        const RescueTargetSeed &seed = seeds[seed_index];
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
        for (int id : blockers) movement_cost_sum += move_cost(groups[id].v, r_milli);
        diagnostics.exact_targets++;
        ll improvement = compact_fee - baseline.fee - movement_cost_sum;
        if (improvement <= 0) continue;
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
        result.push_back({std::move(cells), region_hash, std::move(blockers), minimum_perimeter,
                          movement_cost_sum, improvement, seed.order});
    }

    sort(result.begin(), result.end(), [](const RescueTarget &lhs, const RescueTarget &rhs) {
        if (lhs.immediate_improvement != rhs.immediate_improvement) {
            return lhs.immediate_improvement > rhs.immediate_improvement;
        }
        if (lhs.blockers.size() != rhs.blockers.size()) return lhs.blockers.size() < rhs.blockers.size();
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
    int quadrant = 0;
    long double temporal_cost = 0.0L;
    long long order = 0;
};

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
    RescueDiagnostics &diagnostics) {
    int n = park.size();
    const GroupState &group = groups[mover_id];
    vector<vi> blocked_prefix = make_blocked_prefix(park, base_owner);
    vector<char> fallback_mask(n * n, false);
    for (auto [x, y] : baseline_cells) fallback_mask[x * n + y] = true;
    vector<vi> fallback_prefix = make_flag_prefix(fallback_mask, n);
    vector<vi> cleared_prefix = make_flag_prefix(cleared_mask, n);

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
    while (remaining_destination_anchors > 0 && sampled_anchors < RESCUE_DESTINATION_ANCHOR_LIMIT &&
           (int)legal.size() < RESCUE_DESTINATION_LEGAL_LIMIT) {
        bool progressed = false;
        for (int index = 0; index < (int)eligible_shapes.size(); index++) {
            if (remaining_destination_anchors == 0 ||
                sampled_anchors >= RESCUE_DESTINATION_ANCHOR_LIMIT ||
                (int)legal.size() >= RESCUE_DESTINATION_LEGAL_LIMIT) {
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
            if (rectangle_sum(blocked_prefix, base_x + a.x, base_y + a.y, a.h, a.w) != 0 ||
                rectangle_sum(blocked_prefix, base_x + b.x, base_y + b.y, b.h, b.w) != 0) {
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
            int lower_half = 2 * base_x + shape.h >= n;
            int right_half = 2 * base_y + shape.w >= n;
            BoardMask mask = make_board_mask(cells, n);
            long double temporal_cost = rescue_destination_temporal_cost(
                cells, mask, park, base_owner, groups, mover_id, arrival_id, current_s, theta);
            legal.push_back({std::move(cells), mask, shape.perimeter, fallback_overlap, cleared_overlap,
                             2 * lower_half + right_half, temporal_cost, local_order++});
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
    sort(legal.begin(), legal.end(), better);

    vector<RescueDestination> result;
    auto add = [&](const RescueDestination &candidate) {
        for (const RescueDestination &existing : result) {
            if (same_region(existing.cells, candidate.cells)) return;
        }
        result.push_back(candidate);
    };
    for (int index = 0; index < min(4, (int)legal.size()); index++) add(legal[index]);
    for (int quadrant = 0; quadrant < 4; quadrant++) {
        auto it = find_if(legal.begin(), legal.end(),
                          [&](const RescueDestination &candidate) { return candidate.quadrant == quadrant; });
        if (it != legal.end()) add(*it);
    }
    for (const RescueDestination &candidate : legal) {
        if ((int)result.size() == RESCUE_DESTINATION_LIMIT) break;
        add(candidate);
    }
    if ((int)result.size() > RESCUE_DESTINATION_LIMIT) result.resize(RESCUE_DESTINATION_LIMIT);
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
                    child.rank += 1000.0L * candidate.fallback_overlap + 10.0L * candidate.cleared_overlap -
                                  candidate.temporal_cost - 0.01L * candidate.perimeter;
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
    const DensityModel &density_model, const vector<vector<Shape>> &compact_shapes) {
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

        ShadowEvaluation shadow = evaluate_shadow_cost(state.groups, spec.s, spec.t, spec.p,
                                                       spec.remaining_after, grass_cells, spec.theta,
                                                       density_model);
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
    const DensityModel &density_model, const vector<vector<Shape>> &compact_shapes,
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
            scenario, grass_cells, density_model, compact_shapes);
        RescueRolloutOutcome challenger_outcome = evaluate_rescue_rollout_branch(
            park, *challenger_branch.final_owner, groups, arrival_id, *challenger_branch.plan,
            scenario, grass_cells, density_model, compact_shapes);
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

struct PreparedRescueCandidate {
    TurnPlan plan;
    vvi final_owner;
    vector<int> blockers;
    ll compact_fee = 0;
    ll direct_gain = 0;
    ll movement_cost = 0;
};

optional<RootActionResult> choose_root_action_with_rescue(
    const vs &park, const vvi &owner, const vector<GroupState> &groups, int arrival_id, ll current_s,
    int remaining_groups, int r_milli, long double theta, const ThetaEstimator &theta_estimator,
    const DensityModel &density_model, int grass_cells, const ArrivalDecision &baseline,
    const vector<NormalPlacementChoice> &normal_alternatives,
    const vector<vector<Shape>> &compact_shapes, const vector<vector<Shape>> &all_shapes,
    int &confirmations_used, bool &root_screen_evaluated, RescueDiagnostics &diagnostics) {
    root_screen_evaluated = false;
    const GroupState &arrival = groups[arrival_id];
    int minimum_perimeter = compact_shapes[arrival.p].front().perimeter;
    if (baseline.status != ArrivalStatus::Accepted || !baseline.cells ||
        baseline.perimeter <= minimum_perimeter + COMPACT_PERIMETER_MARGIN) {
        return nullopt;
    }
    diagnostics.eligible_fallbacks++;
    vector<RescueTarget> targets = make_rescue_targets(park, owner, groups, arrival_id, r_milli, baseline,
                                                       compact_shapes, diagnostics);
    if (targets.empty()) {
        diagnostics.no_economic_target++;
        return nullopt;
    }

    int remaining_nodes = RESCUE_REPAIR_NODE_LIMIT;
    int remaining_destination_anchors = RESCUE_DESTINATION_ANCHOR_GLOBAL_LIMIT;
    int attempted_targets = 0;
    vector<PreparedRescueCandidate> candidates;
    RescueRolloutScenarios rollout_scenarios;
    bool rollout_ready = false;
    bool stop_after_primary = false;
    for (const RescueTarget &target : targets) {
        if (attempted_targets == RESCUE_TARGET_REPAIR_LIMIT || remaining_nodes == 0 ||
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

        vector<vector<RescueDestination>> pools;
        pools.reserve(target.blockers.size());
        bool missing_destination = false;
        for (int id : target.blockers) {
            vector<RescueDestination> pool = make_rescue_destinations(
                park, base_owner, groups, id, arrival_id, current_s, theta, *baseline.cells, cleared_mask,
                all_shapes, remaining_destination_anchors, diagnostics);
            if (pool.empty()) {
                missing_destination = true;
                break;
            }
            pools.push_back(std::move(pool));
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
            plan.immediate_gain - baseline.fee <= 0) {
            diagnostics.validation_failures++;
            continue;
        }

        ll direct_gain = plan.immediate_gain - baseline.fee;
        int blocker_bucket = min((int)target.blockers.size(), 4) - 1;
        diagnostics.feasible_plans++;
        diagnostics.feasible_by_blocker_count[blocker_bucket]++;
        diagnostics.feasible_direct_gain += direct_gain;
        candidates.push_back({std::move(plan), std::move(final_owner), target.blockers, compact_fee,
                              direct_gain, target.movement_cost});

        if (candidates.size() == 1) {
            diagnostics.feasible_turns++;
            if (remaining_groups == 0) {
                diagnostics.rollout_skipped_no_future++;
                stop_after_primary = true;
                break;
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
                // Scenario construction is only a filter.  Preserve the legal,
                // positive-immediate v1 action if that filter cannot be built.
                diagnostics.rollout_generation_failures++;
                stop_after_primary = true;
                break;
            }
            rollout_ready = true;
        }
    }

    if (candidates.empty()) {
        if (attempted_targets == RESCUE_TARGET_REPAIR_LIMIT && (int)targets.size() > attempted_targets) {
            diagnostics.target_limit_exhausted++;
        }
        if (remaining_nodes == 0) diagnostics.node_limit_exhausted++;
        diagnostics.no_repair++;
        return nullopt;
    }
    if (!stop_after_primary && (int)candidates.size() < RESCUE_ROLLOUT_CANDIDATE_LIMIT &&
        attempted_targets == RESCUE_TARGET_REPAIR_LIMIT && (int)targets.size() > attempted_targets) {
        diagnostics.target_limit_exhausted++;
    }
    if (remaining_nodes == 0) diagnostics.node_limit_exhausted++;

    RootActionKind selected_kind = RootActionKind::Rescue;
    int selected_candidate = 0;
    int selected_alternative = -1;
    if (rollout_ready) {
        root_screen_evaluated = true;
        diagnostics.rollout_turns++;
        diagnostics.rollout_candidates_compared += candidates.size();
        vector<int> available_alternatives;
        if constexpr (!ROOT_PROTECTED_ONLY) {
            for (int index = 0; index < (int)normal_alternatives.size(); index++) {
                if (!same_region(normal_alternatives[index].cells, *baseline.cells)) {
                    available_alternatives.push_back(index);
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
        place_cells(baseline_final_owner, *baseline.cells, arrival_id);
        TurnPlan baseline_plan = make_arrival_plan(baseline);
        array<RescueRolloutOutcome, RESCUE_ROLLOUT_SCENARIO_COUNT> baseline_outcomes;
        for (int scenario = 0; scenario < RESCUE_ROLLOUT_SCENARIO_COUNT; scenario++) {
            baseline_outcomes[scenario] = evaluate_rescue_rollout_branch(
                park, baseline_final_owner, groups, arrival_id, baseline_plan,
                rollout_scenarios.arrivals[scenario], grass_cells, density_model, compact_shapes);
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
                    rollout_scenarios.arrivals[scenario], grass_cells, density_model, compact_shapes);
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
        bool first_accepts = best_candidate.direct_gain + best_evaluation.future_delta[0] > 0;
        bool second_accepts = best_candidate.direct_gain + best_evaluation.future_delta[1] > 0;
        if (first_accepts != second_accepts) diagnostics.rollout_scenario_disagreements++;

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
                        rollout_scenarios.arrivals[scenario], grass_cells, density_model, compact_shapes);
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
                    grass_cells, density_model, compact_shapes, protected_branch, challenger_branch,
                    confirmations_used, diagnostics);
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
                return nullopt;
            }
            assert(selected_kind == RootActionKind::NormalAlternative);
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

    PreparedRescueCandidate &chosen = candidates[selected_candidate];
    int blocker_bucket = min((int)chosen.blockers.size(), 4) - 1;
    ArrivalDecision selected = baseline;
    selected.cells = *chosen.plan.arrival;
    selected.perimeter = chosen.plan.arrival_perimeter;
    selected.fee = chosen.compact_fee;
    // The counterfactual used connected growth, but the selected arrival is
    // now a minimum-perimeter template.  Keep diagnostics factual.
    replace_selected_placement_success(selected.diagnostics, PlacementSource::MinimumTemplate);
    diagnostics.successes++;
    diagnostics.successes_by_blocker_count[blocker_bucket]++;
    diagnostics.moved_groups += chosen.blockers.size();
    diagnostics.arrival_fee_gain += chosen.compact_fee - baseline.fee;
    diagnostics.movement_cost += chosen.movement_cost;
    diagnostics.immediate_gain += chosen.direct_gain;
    return RootActionResult{std::move(chosen.plan), std::move(selected)};
}

optional<RootActionResult> choose_normal_root_action(
    const vs &park, const vvi &owner, const vector<GroupState> &groups, int arrival_id,
    ll current_s, int remaining_groups, long double theta, const ThetaEstimator &theta_estimator,
    const DensityModel &density_model, int grass_cells, const ArrivalDecision &baseline,
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
            scenarios.arrivals[scenario], grass_cells, density_model, compact_shapes);
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
                grass_cells, density_model, compact_shapes);
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
        grass_cells, density_model, compact_shapes, baseline_branch, challenger_branch,
        confirmations_used, diagnostics);
    if (!confirmed) {
        diagnostics.normal_root_selected_primary++;
        return nullopt;
    }

    diagnostics.normal_root_selected_alternative++;
    diagnostics.normal_root_selected_alternative_rank[selected_alternative]++;
    return RootActionResult{std::move(alternative_plans[selected_alternative]),
                            std::move(alternative_decisions[selected_alternative])};
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
    Timer timer;

    int N, M;
    ld R;
    cin >> N >> M >> R;
    int r_milli = (int)llroundl(R * 1000.0L);
    vs park(N);
    for (string &row : park) cin >> row;

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
    int root_confirmations_used = 0;
    array<bool, 4> normal_root_window_used{};

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

        // A departure at exactly S is still present by the problem's ordering;
        // only t < S is removed before this arrival is handled.
        while (!departures.empty() && departures.top().first < S) {
            int j = departures.top().second;
            departures.pop();
            if (!groups[j].active) continue;
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

        // The ordinary primary remains the protected counterfactual.  Rescue
        // roots are screened first; a small deterministic set of non-compact
        // normal turns may use the same root machinery without relocation.
        vector<NormalPlacementChoice> baseline_alternatives;
        ArrivalDecision baseline_arrival = evaluate_arrival_decision(
            park, owner, groups, i, S, remaining_groups, theta, shadow.opportunity_cost, compact_shapes,
            &baseline_alternatives);
        bool rescue_root_screen_evaluated = false;
        optional<RootActionResult> expanded_action = choose_root_action_with_rescue(
            park, owner, groups, i, S, remaining_groups, r_milli, theta, theta_estimator, density_model,
            grass_cells, baseline_arrival, baseline_alternatives, compact_shapes, all_shapes,
            root_confirmations_used, rescue_root_screen_evaluated, rescue_diagnostics);

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
                park, owner, groups, i, S, remaining_groups, theta, theta_estimator, density_model,
                grass_cells, baseline_arrival, baseline_alternatives, compact_shapes,
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
         << " model_expected_p=" << density_model.expected_group_size
         << " elapsed=" << setprecision(3) << timer.elapsed() << '\n';

    return 0;
}
