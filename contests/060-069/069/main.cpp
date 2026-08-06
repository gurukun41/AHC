#include <bits/stdc++.h>

/*
 * AHC069 core ablation
 *
 * 1. 観測済み滞在時間からケース固有の theta を推定する。
 * 2. sampled DLP で将来需要のセル時間価格を求める。
 * 3. 今回料金とセル時間の機会損失で受入可否を決める。
 * 4. 従来候補を保護し、同一最短周長内のfuture-fitで配置を選ぶ。
 *
 */

using namespace std;
using ll = long long;
using vi = vector<int>;
using vvi = vector<vi>;
using vb = vector<bool>;
using vvb = vector<vb>;
using vs = vector<string>;
using Cell = pair<int, int>;
using i128 = __int128_t;

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

bool inside(int x, int y, int h, int w) {
    return 0 <= x && x < h && 0 <= y && y < w;
}

// ---------- 未来分布・sampled DLP ----------
constexpr ll ARRIVAL_TIME_HORIZON = 100000;
constexpr int SAMPLED_DLP_BUCKET_COUNT = 16;
constexpr int SAMPLED_DLP_REQUEST_COUNT = 256;
constexpr int SAMPLED_DLP_COORDINATE_SWEEPS = 8;
constexpr long double SAMPLED_DLP_PRICE_QUANTIZATION = 1000000000.0L;
constexpr int THETA_MIN = 2000;
constexpr int THETA_MAX = 8000;
constexpr int THETA_STEP = 100;
constexpr int THETA_QUADRATURE_STEPS = 48;

// ---------- 通常配置 ----------
constexpr int COMPACT_PERIMETER_MARGIN = 4;
constexpr int CONNECTED_GROWTH_SEED_LIMIT = 16;
constexpr int PLACEMENT_GLOBAL_SHORTLIST = 3;
constexpr int PLACEMENT_SHORTLIST_LIMIT = 6;
constexpr int FUTURE_FIT_SNAPSHOT_COUNT = 3;
constexpr array<int, 8> FUTURE_FIT_SIDES = {2, 3, 4, 5, 6, 8, 10, 12};

// テンプレートは「主矩形」と、面積の端数を埋める1行または1列で表す。
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

// 受入済み1組の状態。比較版では再配置しないため、料金履歴は不要である。
struct GroupState {
    bool active = false;
    ll s = 0;
    ll t = 0;
    ll v = 0;
    int p = 0;
    vector<Cell> cells;
};

struct ArrivalDecision {
    optional<vector<Cell>> cells;
};

// 面積pを「矩形＋端数の1行/1列」で表せる全テンプレートを列挙する。
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
            add_shape({0, 0, full, width}, {0, 0, 0, 0}, full, width,
                      2 * (full + width));
            continue;
        }

        int perimeter = 2 * (full + width) + 2;

        // full×widthの主矩形に端数行を上または下から付ける。
        for (int below = 0; below < 2; below++) {
            for (int right = 0; right < 2; right++) {
                Rect main_rect{below ? 0 : 1, 0, full, width};
                Rect extra_rect{below ? full : 0, right ? width - rem : 0, 1, rem};
                add_shape(main_rect, extra_rect, full + 1, width, perimeter);
            }
        }

        // 転置形。端数列を左または右から付ける。
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
    sort(shapes.begin(), shapes.end(),
         [&](const Shape &lhs, const Shape &rhs) { return key(lhs) < key(rhs); });
    shapes.erase(unique(shapes.begin(), shapes.end(),
                        [&](const Shape &lhs, const Shape &rhs) {
                            return key(lhs) == key(rhs);
                        }),
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

// 池または既存組で使用中のセルを1とした二次元累積和。
vector<vi> make_blocked_prefix(const vs &park, const vvi &owner) {
    int n = static_cast<int>(park.size());
    vector<vi> prefix(n + 1, vi(n + 1));
    for (int x = 0; x < n; x++) {
        for (int y = 0; y < n; y++) {
            int blocked = park[x][y] == '#' || owner[x][y] != -1;
            prefix[x + 1][y + 1] =
                blocked + prefix[x][y + 1] + prefix[x + 1][y] - prefix[x][y];
        }
    }
    return prefix;
}

int rectangle_sum(const vector<vi> &prefix, int x, int y, int h, int w) {
    if (h == 0 || w == 0) return 0;
    return prefix[x + h][y + w] - prefix[x][y + w] -
           prefix[x + h][y] + prefix[x][y];
}

long double rectangle_sum(const vector<vector<long double>> &prefix,
                          int x, int y, int h, int w) {
    if (h == 0 || w == 0) return 0.0L;
    return prefix[x + h][y + w] - prefix[x][y + w] -
           prefix[x + h][y] + prefix[x][y];
}

// テンプレートが置けない場合の完全フォールバック。
// BFS prefixは連結なので、pセル以上の空き成分があれば必ず合法領域を返す。
optional<vector<Cell>> find_connected_region(const vs &park, const vvi &owner, int p) {
    int n = static_cast<int>(park.size());
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

bool same_region(vector<Cell> lhs, vector<Cell> rhs) {
    if (lhs.size() != rhs.size()) return false;
    sort(lhs.begin(), lhs.end());
    sort(rhs.begin(), rhs.end());
    return lhs == rhs;
}

int calc_perimeter(const vector<Cell> &cells, int n) {
    vector<char> in_region(n * n, false);
    for (auto [x, y] : cells) in_region[x * n + y] = true;

    constexpr int DX[4] = {-1, 1, 0, 0};
    constexpr int DY[4] = {0, 0, -1, 1};
    int perimeter = 0;
    for (auto [x, y] : cells) {
        for (int dir = 0; dir < 4; dir++) {
            int nx = x + DX[dir];
            int ny = y + DY[dir];
            if (!inside(nx, ny, n, n) || !in_region[nx * n + ny]) perimeter++;
        }
    }
    return perimeter;
}

// 公式料金 round(4*V*sqrt(P)/周長) を整数演算で確定する。
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

void clear_cells(vvi &owner, const vector<Cell> &cells) {
    for (auto [x, y] : cells) owner[x][y] = -1;
}

void place_cells(vvi &owner, const vector<Cell> &cells, int id) {
    for (auto [x, y] : cells) owner[x][y] = id;
}

// 1ケース内で共有される未知の滞在時間スケールthetaをベイズ推定する。
// 観測済み滞在時間に加え、「未到着組は現在時刻より後に始まる」打ち切り情報を使う。
struct ThetaEstimator {
    static constexpr int PARTICLE_COUNT =
        (THETA_MAX - THETA_MIN) / THETA_STEP + 1;

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
        const long double upper =
            (last_start_without_duration - current_s) / theta;
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
        array<long double, PARTICLE_COUNT> log_weights{};
        long double max_log_weight =
            -numeric_limits<long double>::infinity();

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
    // Peter J. Acklamの有理関数近似による標準正規分布の逆CDF。
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
        return (((((C[0] * q + C[1]) * q + C[2]) * q + C[3]) * q +
                 C[4]) *
                    q +
                C[5]) /
               ((((D[0] * q + D[1]) * q + D[2]) * q + D[3]) * q +
                1.0L);
    }
    if (probability > HIGH) {
        long double q = sqrtl(-2.0L * logl(1.0L - probability));
        return -(((((C[0] * q + C[1]) * q + C[2]) * q + C[3]) * q +
                  C[4]) *
                     q +
                 C[5]) /
               ((((D[0] * q + D[1]) * q + D[2]) * q + D[3]) * q +
                1.0L);
    }

    long double q = probability - 0.5L;
    long double r = q * q;
    return (((((A[0] * r + A[1]) * r + A[2]) * r + A[3]) * r +
             A[4]) *
                r +
            A[5]) *
           q /
           (((((B[0] * r + B[1]) * r + B[2]) * r + B[3]) * r +
             B[4]) *
                r +
            1.0L);
}

// 「未観測の1組は現在時刻Sより後に始まる」という条件付き未来分布。
// 通常配置の退去時刻整合とfuture-fitのsnapshot生成に使う。
struct ConditionalFutureDemand {
    struct Node {
        long double last_start;
        long double joint_weight;
    };

    ll current_s;
    array<Node, THETA_QUADRATURE_STEPS> nodes{};
    long double remaining_start_measure = 0.0L;

    ConditionalFutureDemand(ll current_s_, long double theta)
        : current_s(current_s_) {
        const long double horizon = ARRIVAL_TIME_HORIZON;
        long double sampled_length_upper = horizon - 1.0L - current_s;
        long double y_upper =
            -expm1l(-sampled_length_upper / theta);
        long double dy = y_upper / THETA_QUADRATURE_STEPS;

        for (int k = 0; k < THETA_QUADRATURE_STEPS; k++) {
            long double y = (k + 0.5L) * dy;
            long double sampled_length = -theta * log1pl(-y);
            long double last_start = horizon - 1.0L - sampled_length;
            long double joint_weight = dy / (horizon - sampled_length);
            nodes[k] = {last_start, joint_weight};
            remaining_start_measure +=
                joint_weight * (last_start - current_s);
        }
    }

    long double future_start_cdf(long double time) const {
        if (remaining_start_measure <= 0.0L || time <= current_s) return 0.0L;

        long double measure = 0.0L;
        for (const Node &node : nodes) {
            long double available_length =
                max(0.0L, node.last_start - (long double)current_s);
            long double prefix_length =
                clamp(time - (long double)current_s, 0.0L, available_length);
            measure += node.joint_weight * prefix_length;
        }
        return clamp(measure / remaining_start_measure, 0.0L, 1.0L);
    }
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

// sampled DLPによる機会損失モデル。
// 未来256組を決定的に生成し、最大16時間帯のセル時間価格を座標降下で求める。
struct SampledDlpShadowModel {
    struct Request {
        ll s = 0;
        ll t = 0;
        int p = 0;
        ll ideal_fee = 0;
        array<long double, SAMPLED_DLP_BUCKET_COUNT> load{};
    };

    array<int, 151> minimum_perimeter{};
    vector<float> exact_future_survival;
    array<ll, SAMPLED_DLP_BUCKET_COUNT + 1> boundaries{};
    array<long double, SAMPLED_DLP_BUCKET_COUNT> prices{};
    int bucket_count = 0;
    bool ready = false;

    // 61個のtheta候補について Q_theta(s)=Pr(未来開始時刻>S) を前計算する。
    void initialize(const vector<vector<Shape>> &compact_shapes) {
        for (int p = 4; p <= 150; p++) {
            minimum_perimeter[p] = compact_shapes[p].front().perimeter;
        }

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
                reciprocal_start_mass +=
                    probability / (ARRIVAL_TIME_HORIZON - l);
                survival += reciprocal_start_mass;
                int s = ARRIVAL_TIME_HORIZON - l - 2;
                exact_future_survival[
                    (size_t)k * ARRIVAL_TIME_HORIZON + s] =
                    (float)survival;
                if (l >= 1) left_tail *= ratio;
            }
        }
    }

    array<int, 5> exact_posterior_quantiles(
        const ThetaEstimator &theta_estimator,
        ll current_s,
        int remaining_groups) const {
        static constexpr array<long double, 5> PROBABILITIES = {
            0.10L, 0.30L, 0.50L, 0.70L, 0.90L,
        };
        array<long double, ThetaEstimator::PARTICLE_COUNT> log_weights{};
        long double maximum_log_weight =
            -numeric_limits<long double>::infinity();
        int positive_count =
            theta_estimator.observed_count - theta_estimator.rounded_zero_count;

        for (int k = 0; k < ThetaEstimator::PARTICLE_COUNT; k++) {
            long double theta = THETA_MIN + THETA_STEP * k;
            long double inverse_theta = 1.0L / theta;
            long double normalizer =
                -expm1l(-(ARRIVAL_TIME_HORIZON - 0.5L) * inverse_theta);
            long double log_weight =
                theta_estimator.rounded_zero_count *
                    logl(-expm1l(-0.5L * inverse_theta)) +
                positive_count *
                    (logl(-expm1l(-inverse_theta)) +
                     0.5L * inverse_theta) -
                theta_estimator.exponential_sample_sum * inverse_theta -
                theta_estimator.observed_count * logl(normalizer);
            if (remaining_groups > 0) {
                log_weight += remaining_groups *
                              logl((long double)exact_future_survival[
                                  (size_t)k * ARRIVAL_TIME_HORIZON +
                                  current_s]);
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

    bool should_rebuild(int turn, ll current_s) const {
        if (!ready) return true;
        if (turn == 4 || turn == 8 || turn == 16 ||
            (turn > 16 && turn % 16 == 0)) {
            return true;
        }

        int crossed = 0;
        for (int b = 1; b < bucket_count; b++) {
            if (boundaries[b] <= current_s) crossed++;
        }
        return crossed >= 2;
    }

    static vector<long double> make_conditional_duration_cdf(
        ll current_s, int theta) {
        int maximum_l =
            (int)(ARRIVAL_TIME_HORIZON - current_s - 2);
        if (maximum_l < 0) return {};

        vector<long double> cdf(maximum_l + 1);
        long double inverse_theta = 1.0L / theta;
        long double ratio = expl(-inverse_theta);
        long double left_tail = expl(-0.5L * inverse_theta);
        long double cumulative = 0.0L;
        for (int l = 0; l <= maximum_l; l++) {
            long double duration_mass =
                l == 0 ? -expm1l(-0.5L * inverse_theta)
                       : left_tail * (1.0L - ratio);
            long double future_start_count =
                ARRIVAL_TIME_HORIZON - l - 1 - current_s;
            long double all_start_count = ARRIVAL_TIME_HORIZON - l;
            cumulative += duration_mass * future_start_count /
                          all_start_count;
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
        bucket_count =
            (int)min<ll>(SAMPLED_DLP_BUCKET_COUNT,
                         max(1LL, remaining_time));
        for (int b = 0; b <= bucket_count; b++) {
            boundaries[b] =
                current_s + remaining_time * b / bucket_count;
        }
        prices.fill(0.0L);
    }

    vector<Request> build_requests(
        ll current_s,
        int remaining_groups,
        const ThetaEstimator &theta_estimator) {
        array<int, 5> theta_values = exact_posterior_quantiles(
            theta_estimator, current_s, remaining_groups);

        vector<int> unique_theta;
        vector<vector<long double>> duration_cdfs;
        array<int, 5> cdf_index{};
        for (int k = 0; k < 5; k++) {
            auto found =
                find(unique_theta.begin(), unique_theta.end(), theta_values[k]);
            if (found == unique_theta.end()) {
                cdf_index[k] = static_cast<int>(unique_theta.size());
                unique_theta.push_back(theta_values[k]);
                duration_cdfs.push_back(
                    make_conditional_duration_cdf(current_s,
                                                  theta_values[k]));
            } else {
                cdf_index[k] = static_cast<int>(found - unique_theta.begin());
            }
        }

        vector<Request> requests;
        requests.reserve(SAMPLED_DLP_REQUEST_COUNT);
        const long double size_width = sqrtl(150.0L) - 2.0L;
        for (int sample = 0; sample < SAMPLED_DLP_REQUEST_COUNT; sample++) {
            uint64_t index = sample + 1;
            long double theta_quantile =
                (sample + 0.5L) / SAMPLED_DLP_REQUEST_COUNT;
            int theta_slot =
                min(4, (int)floorl(5.0L * theta_quantile));
            const vector<long double> &cdf =
                duration_cdfs[cdf_index[theta_slot]];
            if (cdf.empty()) continue;

            long double duration_quantile =
                sampled_dlp_radical_inverse(index, 2);
            int l = static_cast<int>(
                lower_bound(cdf.begin(), cdf.end(), duration_quantile) -
                cdf.begin());
            ll duration = l + 1;
            ll future_start_count =
                ARRIVAL_TIME_HORIZON - duration - current_s;
            if (future_start_count <= 0) continue;
            long double start_quantile =
                sampled_dlp_radical_inverse(index, 3);
            ll start = current_s + 1 +
                       min(future_start_count - 1,
                           (ll)floorl(start_quantile *
                                      future_start_count));

            long double size_quantile =
                sampled_dlp_radical_inverse(index, 5);
            long double root_size =
                2.0L + size_width * size_quantile;
            int p = clamp((int)llroundl(root_size * root_size), 4, 150);
            long double value_quantile =
                sampled_dlp_radical_inverse(index, 7);
            long double noise =
                0.8L * inverse_standard_normal(value_quantile);
            long double raw_v =
                p * powl((long double)duration, 0.9L) * exp2l(noise);
            ll v = clamp((ll)llroundl(raw_v), 1LL, 100000000LL);

            Request request;
            request.s = start;
            request.t = start + duration;
            request.p = p;
            request.ideal_fee =
                round_payment(v, p, minimum_perimeter[p]);
            for (int b = 0; b < bucket_count; b++) {
                ll overlap =
                    max(0LL, min(request.t, boundaries[b + 1]) -
                                   max(request.s, boundaries[b]));
                request.load[b] = (long double)p * overlap;
            }
            requests.push_back(std::move(request));
        }
        return requests;
    }

    void solve_dual(const vector<Request> &requests,
                    int remaining_groups,
                    const vector<GroupState> &groups,
                    int grass_cells) {
        array<long double, SAMPLED_DLP_BUCKET_COUNT> capacity{};
        long double sample_weight =
            (long double)remaining_groups / SAMPLED_DLP_REQUEST_COUNT;
        for (int b = 0; b < bucket_count; b++) {
            capacity[b] =
                (long double)grass_cells *
                (boundaries[b + 1] - boundaries[b]);
            for (const GroupState &group : groups) {
                if (!group.active) continue;
                ll overlap =
                    max(0LL, min(group.t, boundaries[b + 1]) -
                                   max(group.s, boundaries[b]));
                capacity[b] -= (long double)group.p * overlap;
            }
            capacity[b] = max(0.0L, capacity[b]);
        }

        struct Breakpoint {
            long double value;
            long double load;
            int request_index;
        };
        vector<Breakpoint> breakpoints;
        breakpoints.reserve(requests.size());

        // 他時間帯価格を引いた残余価値から、1座標ずつ離脱価格を更新する。
        for (int sweep = 0; sweep < SAMPLED_DLP_COORDINATE_SWEEPS;
             sweep++) {
            for (int b = 0; b < bucket_count; b++) {
                breakpoints.clear();
                long double active_load = 0.0L;
                for (int request_index = 0;
                     request_index < (int)requests.size();
                     request_index++) {
                    const Request &request = requests[request_index];
                    long double a = request.load[b];
                    if (a <= 0.0L) continue;
                    long double residual = request.ideal_fee;
                    for (int c = 0; c < bucket_count; c++) {
                        if (c != b) residual -= prices[c] * request.load[c];
                    }
                    if (residual <= 0.0L) continue;
                    long double weighted_load = sample_weight * a;
                    breakpoints.push_back(
                        {residual / a, weighted_load, request_index});
                    active_load += weighted_load;
                }

                long double next_price = 0.0L;
                if (active_load > capacity[b]) {
                    sort(breakpoints.begin(), breakpoints.end(),
                         [](const Breakpoint &lhs, const Breakpoint &rhs) {
                             if (lhs.value != rhs.value) {
                                 return lhs.value < rhs.value;
                             }
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
            }
        }

        for (int b = 0; b < bucket_count; b++) {
            prices[b] =
                roundl(prices[b] * SAMPLED_DLP_PRICE_QUANTIZATION) /
                SAMPLED_DLP_PRICE_QUANTIZATION;
            if (!isfinite(prices[b]) || prices[b] < 0.0L) {
                prices.fill(0.0L);
                return;
            }
        }

        // 双対目的値まで有限であることを確認し、壊れた価格を意思決定へ渡さない。
        long double dual_objective = 0.0L;
        for (int b = 0; b < bucket_count; b++) {
            dual_objective += prices[b] * capacity[b];
        }
        for (const Request &request : requests) {
            long double priced_load = 0.0L;
            for (int b = 0; b < bucket_count; b++) {
                priced_load += prices[b] * request.load[b];
            }
            dual_objective +=
                sample_weight *
                max(0.0L, (long double)request.ideal_fee - priced_load);
        }
        if (!isfinite(dual_objective) || dual_objective < 0.0L) {
            prices.fill(0.0L);
        }
    }

    void rebuild(ll current_s,
                 int remaining_groups,
                 const vector<GroupState> &groups,
                 int grass_cells,
                 const ThetaEstimator &theta_estimator) {
        build_buckets(current_s);
        vector<Request> requests =
            build_requests(current_s, remaining_groups, theta_estimator);
        if ((int)requests.size() != SAMPLED_DLP_REQUEST_COUNT) {
            prices.fill(0.0L);
        } else {
            solve_dual(requests, remaining_groups, groups, grass_cells);
        }
        ready = true;
    }

    long double evaluate_real_turn(
        int turn,
        ll current_s,
        ll arrival_t,
        int p,
        int remaining_groups,
        const vector<GroupState> &groups,
        int grass_cells,
        const ThetaEstimator &theta_estimator) {
        if (remaining_groups <= 0) return 0.0L;

        if (should_rebuild(turn, current_s)) {
            rebuild(current_s, remaining_groups, groups, grass_cells,
                    theta_estimator);
        }
        if (!ready) return 0.0L;

        long double opportunity_cost = 0.0L;
        for (int b = 0; b < bucket_count; b++) {
            ll overlap =
                max(0LL, min(arrival_t, boundaries[b + 1]) -
                               max(current_s, boundaries[b]));
            if (overlap <= 0) continue;
            opportunity_cost += (long double)p * overlap * prices[b];
        }

        if (!isfinite(opportunity_cost) || opportunity_cost < 0.0L) {
            return 0.0L;
        }
        return opportunity_cost;
    }
};

struct PlacementChoice {
    vector<Cell> cells;
    int perimeter = 0;
};

struct PlacementCandidate {
    vector<Cell> cells;
    uint64_t region_hash = 0;
    int perimeter = 0;
    long double incremental_cost = 0.0L;
    long double absolute_cost = 0.0L;
    long long enumeration_order = 0;
    int quadrant = 0;
};

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

uint64_t placement_region_hash(const vector<Cell> &cells) {
    uint64_t hash = 0;
    for (auto [x, y] : cells) {
        uint64_t value =
            (uint64_t)(x * 64 + y + 1) + 0x9e3779b97f4a7c15ULL;
        value = (value ^ (value >> 30)) * 0xbf58476d1ce4e5b9ULL;
        value = (value ^ (value >> 27)) * 0x94d049bb133111ebULL;
        hash ^= value ^ (value >> 31);
    }
    return hash;
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

// 従来方式の最大6候補。最短周長を守ったまま、incremental上位3、
// absolute最良、列挙先頭、別象限の最良を重複なしで保持する。
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
    void consider(int perimeter,
                  long double incremental_cost,
                  long double absolute_cost,
                  long long enumeration_order,
                  int quadrant,
                  Maker &&maker) {
        if (perimeter < best_perimeter) reset(perimeter);
        if (perimeter > best_perimeter) return;

        optional<PlacementCandidate> cache;
        auto get_candidate = [&]() -> const PlacementCandidate & {
            if (!cache) {
                PlacementCandidate candidate;
                candidate.cells = maker();
                candidate.region_hash =
                    placement_region_hash(candidate.cells);
                candidate.perimeter = perimeter;
                candidate.incremental_cost = incremental_cost;
                candidate.absolute_cost = absolute_cost;
                candidate.enumeration_order = enumeration_order;
                candidate.quadrant = quadrant;
                cache = std::move(candidate);
            }
            return *cache;
        };

        if (!first_candidate) first_candidate = get_candidate();

        PlacementCandidate key_candidate;
        key_candidate.perimeter = perimeter;
        key_candidate.incremental_cost = incremental_cost;
        key_candidate.absolute_cost = absolute_cost;
        key_candidate.enumeration_order = enumeration_order;
        key_candidate.quadrant = quadrant;
        if ((int)global_best.size() < PLACEMENT_GLOBAL_SHORTLIST ||
            placement_increment_less(
                key_candidate, global_best.back())) {
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
                if ((int)global_best.size() >
                    PLACEMENT_GLOBAL_SHORTLIST) {
                    global_best.pop_back();
                }
            }
        }

        if (!absolute_best ||
            placement_absolute_less(key_candidate, *absolute_best)) {
            absolute_best = get_candidate();
        }
        if (!quadrant_best[quadrant] ||
            placement_increment_less(
                key_candidate, *quadrant_best[quadrant])) {
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
        for (const PlacementCandidate &candidate : global_best) {
            add(candidate);
        }
        add(absolute_best);
        add(first_candidate);

        int primary_quadrant =
            global_best.empty() ? -1 : global_best.front().quadrant;
        optional<PlacementCandidate> diverse;
        for (int quadrant = 0; quadrant < 4; quadrant++) {
            if (quadrant == primary_quadrant ||
                !quadrant_best[quadrant]) {
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

// テンプレートで置けないとき、空き連結成分の複数seedから貪欲成長候補を作る。
vector<vector<Cell>> make_connected_growth_candidates(
    const vs &park, const vvi &owner, int p) {
    int n = static_cast<int>(park.size());
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

    // pセル以上を含む空き連結成分だけを候補にする。
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

    // 障害物や盤面端に近いseed・遠いseedの両方を作る。
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
            if (obstacle_distance[nx][ny] <=
                obstacle_distance[x][y] + 1) {
                continue;
            }
            obstacle_distance[nx][ny] = obstacle_distance[x][y] + 1;
            distance_queue.emplace(nx, ny);
        }
    }

    // 上下左右・4対角・障害物最近/最遠という10種類の特徴点を取る。
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

    for (int component_id = 0;
         component_id < (int)components.size();
         component_id++) {
        for (int feature = 0; feature < SEED_FEATURE_COUNT; feature++) {
            Cell best = components[component_id].front();
            for (const Cell &cell : components[component_id]) {
                if (feature_key(feature, cell) <
                    feature_key(feature, best)) {
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
    for (int feature = 0;
         feature < SEED_FEATURE_COUNT &&
         (int)seeds.size() < CONNECTED_GROWTH_SEED_LIMIT;
         feature++) {
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
                       decltype(entry_worse)>
            frontier(entry_worse);

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

long double compact_fit_utility(const vs &park,
                                const vvi &owner,
                                const vector<GroupState> &groups,
                                const vector<char> &in_candidate,
                                ll snapshot_time) {
    // 各セルを右下端とする最大空き正方形をDPで求める。
    // 2～12四方の配置数を、大きい正方形ほど重く評価する。
    int n = static_cast<int>(park.size());
    constexpr int MAX_SIDE = FUTURE_FIT_SIDES.back();
    array<int, MAX_SIDE + 2> histogram{};
    vector<int> previous(n + 1), current(n + 1);
    for (int x = 0; x < n; x++) {
        fill(current.begin(), current.end(), 0);
        for (int y = 0; y < n; y++) {
            int cell = x * n + y;
            int occupied_by = owner[x][y];
            bool is_free =
                park[x][y] != '#' && !in_candidate[cell] &&
                (occupied_by == -1 ||
                 groups[occupied_by].t < snapshot_time);
            if (!is_free) continue;
            current[y + 1] =
                1 + min({previous[y + 1], current[y], previous[y]});
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
        weighted_utility +=
            weight * log1pl((long double)at_least[side]);
        weight_sum += weight;
    }
    return weighted_utility / weight_sum;
}

array<ll, FUTURE_FIT_SNAPSHOT_COUNT> make_future_fit_snapshots(
    const ConditionalFutureDemand &future_demand,
    ll current_s,
    ll arrival_t) {
    // 今回滞在中に始まる未来到着の確率質量を1/6・3/6・5/6分位で切る。
    array<ll, FUTURE_FIT_SNAPSHOT_COUNT> snapshots{};
    long double total_mass = future_demand.future_start_cdf(arrival_t);
    for (int index = 0; index < FUTURE_FIT_SNAPSHOT_COUNT; index++) {
        long double fraction =
            (2.0L * index + 1.0L) /
            (2.0L * FUTURE_FIT_SNAPSHOT_COUNT);
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
    const vs &park,
    const vvi &owner,
    const vector<GroupState> &groups,
    const vector<Cell> &candidate,
    const array<ll, FUTURE_FIT_SNAPSHOT_COUNT> &snapshots) {
    int n = static_cast<int>(park.size());
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

optional<PlacementChoice> choose_temporally_coherent_region(
    const vs &park,
    const vvi &owner,
    const vector<GroupState> &groups,
    ll current_s,
    ll arrival_t,
    int p,
    long double theta,
    int remaining_groups,
    const vector<vector<Shape>> &all_shapes) {
    // 1. 全周長tierの全テンプレートanchorを走査する。
    // 2. テンプレートの成否にかかわらず連結成長候補も作る。
    // 3. 最小周長候補だけを残し、従来6候補を保護して追加6候補を加える。
    // 4. 同一料金の候補内だけでfuture-fitを比較する。
    int n = static_cast<int>(park.size());
    const vector<Shape> &shapes = all_shapes[p];
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

    // incrementalは今回候補を足す差分、absoluteは配置後に残る境界不整合そのもの。
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
                prefix[x + 1][y + 1] =
                    values[x][y] + prefix[x][y + 1] +
                    prefix[x + 1][y] - prefix[x][y];
            }
        }
        return prefix;
    };
    vector<vector<long double>> incremental_prefix =
        make_prefix(incremental_cell);
    vector<vector<long double>> absolute_prefix =
        make_prefix(absolute_cell);

    PlacementShortlistBuilder template_shortlist_builder;
    long long enumeration_order = 0;
    int theoretical_minimum_perimeter = shapes.front().perimeter;

    auto scan_shape = [&](const Shape &shape) {
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

        for (int base_x = 0; base_x + shape.h <= n; base_x++) {
            for (int base_y = 0; base_y + shape.w <= n; base_y++) {
                const Rect &main_rect = shape.main_rect;
                const Rect &extra_rect = shape.extra_rect;
                if (rectangle_sum(blocked_prefix,
                                  base_x + main_rect.x,
                                  base_y + main_rect.y,
                                  main_rect.h,
                                  main_rect.w) != 0) {
                    continue;
                }
                if (rectangle_sum(blocked_prefix,
                                  base_x + extra_rect.x,
                                  base_y + extra_rect.y,
                                  extra_rect.h,
                                  extra_rect.w) != 0) {
                    continue;
                }
                found_legal = true;
                long double incremental_cost =
                    rectangle_sum(incremental_prefix,
                                  base_x + main_rect.x,
                                  base_y + main_rect.y,
                                  main_rect.h,
                                  main_rect.w) +
                    rectangle_sum(incremental_prefix,
                                  base_x + extra_rect.x,
                                  base_y + extra_rect.y,
                                  extra_rect.h,
                                  extra_rect.w) -
                    (4 * p - shape.perimeter) * candidate_arrival_level;
                long double absolute_cost =
                    rectangle_sum(absolute_prefix,
                                  base_x + main_rect.x,
                                  base_y + main_rect.y,
                                  main_rect.h,
                                  main_rect.w) +
                    rectangle_sum(absolute_prefix,
                                  base_x + extra_rect.x,
                                  base_y + extra_rect.y,
                                  extra_rect.h,
                                  extra_rect.w) -
                    (4 * p - shape.perimeter) * candidate_release_level;
                long long sum_x =
                    (long long)p * base_x + relative_sum_x;
                long long sum_y =
                    (long long)p * base_y + relative_sum_y;
                int lower_half = 2 * sum_x >= (long long)p * n;
                int right_half = 2 * sum_y >= (long long)p * n;
                int quadrant = 2 * lower_half + right_half;
                long long order = enumeration_order++;
                template_shortlist_builder.consider(
                    shape.perimeter,
                    incremental_cost,
                    absolute_cost,
                    order,
                    quadrant,
                    [&] {
                        return materialize_shape(
                            shape, base_x, base_y, p);
                    });
            }
        }
        return found_legal;
    };

    bool found_theoretical_minimum = false;
    for (const Shape &shape : shapes) {
        bool found_legal = scan_shape(shape);
        if (shape.perimeter == theoretical_minimum_perimeter) {
            found_theoretical_minimum |= found_legal;
        }
    }

    // protectedは従来template候補を固定し、expandedだけに連結候補を加える。
    // 理論最小周長templateが無い場合だけ、従来fallbackにも連結候補を渡す。
    PlacementShortlistBuilder protected_builder =
        template_shortlist_builder;
    PlacementShortlistBuilder expanded_builder =
        template_shortlist_builder;
    vector<vector<Cell>> growth_candidates =
        make_connected_growth_candidates(park, owner, p);
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
        if (!found_theoretical_minimum) {
            protected_builder.consider(
                perimeter,
                incremental_cost,
                absolute_cost,
                order,
                quadrant,
                [&] { return region; });
        }
        expanded_builder.consider(
            perimeter,
            incremental_cost,
            absolute_cost,
            order,
            quadrant,
            [&] { return region; });
    }

    vector<PlacementCandidate> protected_candidates =
        protected_builder.finalize();
    vector<PlacementCandidate> expanded_candidates =
        expanded_builder.finalize();
    vector<PlacementCandidate> candidates;
    auto add_candidate = [&](const PlacementCandidate &candidate) {
        for (const PlacementCandidate &existing : candidates) {
            if (existing.region_hash == candidate.region_hash &&
                same_region(existing.cells, candidate.cells)) {
                return;
            }
        }
        candidates.push_back(candidate);
    };
    for (const PlacementCandidate &candidate : protected_candidates) {
        add_candidate(candidate);
    }
    for (const PlacementCandidate &candidate : expanded_candidates) {
        add_candidate(candidate);
    }
    if (candidates.empty()) return nullopt;

    // future-fitへ渡す前に最短周長以外を除き、現在料金を変えない。
    int shortest_perimeter = numeric_limits<int>::max();
    for (const PlacementCandidate &candidate : candidates) {
        chmin(shortest_perimeter, candidate.perimeter);
    }
    candidates.erase(
        remove_if(candidates.begin(), candidates.end(),
                  [&](const PlacementCandidate &candidate) {
                      return candidate.perimeter != shortest_perimeter;
                  }),
        candidates.end());

    int best_index = 0;
    for (int index = 1; index < (int)candidates.size(); index++) {
        if (placement_increment_less(
                candidates[index], candidates[best_index])) {
            best_index = index;
        }
    }

    long double future_mass = future_demand.future_start_cdf(arrival_t);
    if ((int)candidates.size() >= 2 && remaining_groups > 0 &&
        arrival_t - current_s > 1 && future_mass > 1e-12L) {
        array<ll, FUTURE_FIT_SNAPSHOT_COUNT> snapshots =
            make_future_fit_snapshots(
                future_demand, current_s, arrival_t);
        long double best_fit =
            -numeric_limits<long double>::infinity();
        for (int index = 0; index < (int)candidates.size(); index++) {
            long double fit = evaluate_compact_fit(
                park, owner, groups, candidates[index].cells, snapshots);
            if (fit > best_fit + 1e-15L ||
                (fabsl(fit - best_fit) <= 1e-15L &&
                 placement_increment_less(
                     candidates[index], candidates[best_index]))) {
                best_fit = fit;
                best_index = index;
            }
        }
    }

    PlacementCandidate choice = std::move(candidates[best_index]);
    return PlacementChoice{std::move(choice.cells), choice.perimeter};
}

ArrivalDecision evaluate_arrival_decision(
    const vs &park,
    const vvi &owner,
    const vector<GroupState> &groups,
    int arrival_id,
    ll current_s,
    int remaining_groups,
    long double theta,
    long double opportunity_cost,
    const vector<vector<Shape>> &compact_shapes) {
    ArrivalDecision result;
    const GroupState &arrival = groups[arrival_id];
    int minimum_perimeter = compact_shapes[arrival.p].front().perimeter;

    // 最小周長でも採算が合わなければ、配置探索をせず拒否する。
    ll upper_bound_fee =
        round_payment(arrival.v, arrival.p, minimum_perimeter);
    if ((long double)upper_bound_fee <= opportunity_cost) return result;

    optional<PlacementChoice> placement =
        choose_temporally_coherent_region(
            park, owner, groups, current_s, arrival.t, arrival.p,
            theta, remaining_groups, compact_shapes);
    if (!placement) return result;

    ll actual_fee =
        round_payment(arrival.v, arrival.p, placement->perimeter);
    if ((long double)actual_fee <= opportunity_cost) return result;

    result.cells = std::move(placement->cells);
    return result;
}

// 比較版では再配置を完全に外しているため、毎ターン移動数は必ず0である。
void emit_decision(const ArrivalDecision &decision) {
    cout << 0 << '\n';
    if (decision.cells) {
        cout << "Yes\n";
        for (auto [x, y] : *decision.cells) {
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

    int N, M;
    long double R;
    cin >> N >> M >> R;
    (void)R;  // 再配置を外した比較版では移動費率を使わない。

    vs park(N);
    for (string &row : park) cin >> row;

    vector<vector<Shape>> compact_shapes(151);
    for (int p = 4; p <= 150; p++) {
        compact_shapes[p] = make_template_shapes(p, N);
        int minimum_perimeter = compact_shapes[p].front().perimeter;
        compact_shapes[p].erase(
            remove_if(compact_shapes[p].begin(), compact_shapes[p].end(),
                      [&](const Shape &shape) {
                          return shape.perimeter >
                                 minimum_perimeter +
                                     COMPACT_PERIMETER_MARGIN;
                      }),
            compact_shapes[p].end());
    }

    int grass_cells = 0;
    for (const string &row : park) {
        grass_cells += static_cast<int>(count(row.begin(), row.end(), '.'));
    }

    SampledDlpShadowModel sampled_dlp_model;
    sampled_dlp_model.initialize(compact_shapes);
    ThetaEstimator theta_estimator;

    vvi owner(N, vi(N, -1));
    vector<GroupState> groups(M);
    priority_queue<pair<ll, int>, vector<pair<ll, int>>,
                   greater<pair<ll, int>>>
        departures;

    for (int turn = 0; turn < M; turn++) {
        int i, P;
        ll S, T, V;
        cin >> i >> S >> T >> P >> V;

        GroupState &arrival = groups[i];
        arrival.s = S;
        arrival.t = T;
        arrival.v = V;
        arrival.p = P;

        // 今回観測も含めてthetaを更新してから、残り需要を評価する。
        theta_estimator.observe(T - S);
        int remaining_groups = M - i - 1;
        long double theta =
            theta_estimator.estimate(S, remaining_groups);

        // 終了時刻がちょうどSの組は今回処理中にはまだ存在する。
        while (!departures.empty() && departures.top().first < S) {
            int id = departures.top().second;
            departures.pop();
            if (!groups[id].active) continue;
            clear_cells(owner, groups[id].cells);
            groups[id].cells.clear();
            groups[id].active = false;
        }

        long double opportunity_cost =
            sampled_dlp_model.evaluate_real_turn(
                turn, S, T, P, remaining_groups, groups, grass_cells,
                theta_estimator);
        ArrivalDecision decision = evaluate_arrival_decision(
            park, owner, groups, i, S, remaining_groups, theta,
            opportunity_cost, compact_shapes);

        if (decision.cells) {
            place_cells(owner, *decision.cells, i);
            arrival.active = true;
            arrival.cells = *decision.cells;
            departures.emplace(T, i);
        }

        emit_decision(decision);
    }
    return 0;
}
