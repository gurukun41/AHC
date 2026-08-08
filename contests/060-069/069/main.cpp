#include <bits/stdc++.h>

/*
 * AHC069 解法の全体像
 *
 * 1. これまでに観測した滞在時間から、テストケース固有の分布パラメータ theta を推定する。
 * 2. sampled DLP で未来256組を決定的に生成し、16個の時間帯ごとのセル価格を求める。
 * 3. 到着組を置いたときに失う未来価値（機会損失）と、今回得る料金を比較して入場を判定する。
 * 4. 配置候補は矩形テンプレート、連結成長、grow-and-trimから作り、
 *    退去時刻の近さと未来に残る空き形状で順位付けする。
 * 5. 通常配置が悪い、または断片化で置けない場合だけ、既存組の再配置を検討する。
 * 6. 再配置候補は短い共通乱数rolloutで通常案と比べ、改善が確認できた場合だけ採用する。
 *
 * コメント中の「root action」は、現在ターンで比較する行動候補
 * （通常案、再配置案、通常配置の次点案）を指す。
 */

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

// 全経路で4近傍の列挙順を統一する。候補生成のtie-breakにも影響するため、
// 順序は従来どおり「上、下、左、右」から変えない。
constexpr array<int, 4> ORTHOGONAL_DX = {-1, 1, 0, 0};
constexpr array<int, 4> ORTHOGONAL_DY = {0, 0, -1, 1};

// 問題制約N=50をbit maskと固定長anchor indexで共有する。
constexpr int BOARD_SIDE_LIMIT = 50;
static_assert(BOARD_SIDE_LIMIT < 64, "1行をuint64_tへ格納できる必要がある");

// 対話入出力の待機時間を除いて、解法本体に使った時間を測る。
// スコア計算には一切使わず、最後のstderr診断にのみ出力する。
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

// ---------- 未来分布・shadow price ----------
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

// sampled DLPは総芝面積を流体容量として扱うため、初期盤面の外周率に応じて
// 滑らかな盤面は1.30倍、中程度の盤面は1.25倍、厳しい盤面は1.00倍とする。
// ケース中は倍率を固定し、再配置・rolloutを含む全admissionで同じ尺度を使う。
constexpr int DLP_SCALE_DENOMINATOR = 1000;
constexpr int DLP_SMOOTH_SCALE_MILLI = 1300;
constexpr int DLP_MODERATE_SCALE_MILLI = 1250;
constexpr int DLP_CONSTRAINED_SCALE_MILLI = 1000;
constexpr int DLP_SMOOTH_BOUNDARY_THRESHOLD_PERCENT = 55;
constexpr int DLP_MODERATE_BOUNDARY_THRESHOLD_PERCENT = 70;
constexpr int HARD_PLACEMENT_BOUNDARY_THRESHOLD_PERCENT = 80;
int case_dlp_scale_milli = DLP_CONSTRAINED_SCALE_MILLI;

// root rolloutの未来料金は通常1.0倍で現在差へ足す。
// 保存済みv29 cacheで再現した限定expertだけ0.1倍とし、短いrolloutの分散より
// 現在ターンの確定利益を優先する。比較は1000倍整数のまま行う。
constexpr int ROOT_WEIGHT_SCALE = 1000;
constexpr int ROOT_DEFAULT_FUTURE_WEIGHT_MILLI = 1000;
constexpr int ROOT_DIRECT_FUTURE_WEIGHT_MILLI = 100;
int case_root_future_weight_milli = ROOT_DEFAULT_FUTURE_WEIGHT_MILLI;

// ---------- 通常配置 ----------
constexpr int COMPACT_PERIMETER_MARGIN = 4;
constexpr int FUTURE_FIT_SNAPSHOT_COUNT = 3;
constexpr array<int, 8> FUTURE_FIT_SIDES = {2, 3, 4, 5, 6, 8, 10, 12};

// connected系の旧Acceptedだけを磨くnear-template deformation。
// 保存100 seedでtailが出た高外周率盤面は対象外とし、E/G<0.55のみで
// v31と同じdense boxとstrict周長降下を候補として追加する。
// 全盤面を走査するdense boxだけはshape lossの中心であるP>=50へ限定する。
// 安いstrict descentは面積によらず同じ合法性・料金・future-fit保護を通す。
// 旧Reject、template成功、synthetic rolloutの経路は変更しない。
constexpr int DENSE_BOX_MIN_GROUP_SIZE = 50;
constexpr int DENSE_BOX_EXTRA_CELLS = 16;
constexpr int DENSE_BOX_PERIMETER_MARGIN = 4;
constexpr int DENSE_BOX_GLOBAL_ANCHOR_LIMIT = 12;
constexpr int DENSE_BOX_TOTAL_ANCHOR_LIMIT = 16;
// 最小周長まで改善したときの料金上界が1万以上の先着24回だけ、
// 高価な全盤面走査を行う。strict descentはこの予算から分離する。
constexpr ll DENSE_BOX_MIN_MAXIMUM_FEE_GAIN = 10000;
constexpr int DENSE_BOX_ATTEMPT_LIMIT_PER_CASE = 24;
constexpr int PERIMETER_DESCENT_MAX_STEPS = 8;
int case_dense_box_attempts = 0;
bool case_connected_polish_enabled = false;

// 初期盤面だけで選ぶ排他的なplacement expert。
// 通常値は旧fullと同一で、E/G>=0.80の難しい盤面だけ、保存済みv29 candidate p2の
// 候補幅とfuture-fit比率をそのまま使う。到着列を見て途中でexpertを変えない。
struct CasePlacementConfig {
    int global_shortlist = 3;
    int shortlist_limit = 6;
    int connected_growth_seed_limit = 16;
    int grow_and_trim_extra_cells = 8;
    int grow_and_trim_candidate_limit = 8;
    int future_fit_min_weight_milli = 250;
};

CasePlacementConfig case_placement_config;

// 初期盤面から一度だけ選ぶexpertの全設定をまとめる。
// 各値を別々の分岐で更新して設定が混ざることを防ぎ、ケース中は不変にする。
struct CaseStaticPolicy {
    int expert = 0;
    int dlp_scale_milli = DLP_CONSTRAINED_SCALE_MILLI;
    CasePlacementConfig placement;
    int root_future_weight_milli = ROOT_DEFAULT_FUTURE_WEIGHT_MILLI;
    bool connected_polish_enabled = false;
};

CaseStaticPolicy select_case_static_policy(
    int exposed_boundary_edges, int grass_cells, int r_milli) {
    int scaled_boundary = 100 * exposed_boundary_edges;
    if (scaled_boundary < DLP_SMOOTH_BOUNDARY_THRESHOLD_PERCENT * grass_cells) {
        return CaseStaticPolicy{
            0, DLP_SMOOTH_SCALE_MILLI, CasePlacementConfig{},
            ROOT_DEFAULT_FUTURE_WEIGHT_MILLI, true};
    }
    if (scaled_boundary < DLP_MODERATE_BOUNDARY_THRESHOLD_PERCENT * grass_cells) {
        return CaseStaticPolicy{
            1, DLP_MODERATE_SCALE_MILLI, CasePlacementConfig{},
            ROOT_DEFAULT_FUTURE_WEIGHT_MILLI, false};
    }
    if (scaled_boundary < HARD_PLACEMENT_BOUNDARY_THRESHOLD_PERCENT * grass_cells) {
        if (r_milli < 60) {
            return CaseStaticPolicy{
                4, DLP_CONSTRAINED_SCALE_MILLI, CasePlacementConfig{},
                ROOT_DIRECT_FUTURE_WEIGHT_MILLI, false};
        }
        return CaseStaticPolicy{
            2, DLP_CONSTRAINED_SCALE_MILLI, CasePlacementConfig{},
            ROOT_DEFAULT_FUTURE_WEIGHT_MILLI, false};
    }
    return CaseStaticPolicy{
        3, DLP_CONSTRAINED_SCALE_MILLI,
        CasePlacementConfig{5, 8, 24, 12, 8, 500},
        ROOT_DEFAULT_FUTURE_WEIGHT_MILLI, false};
}

// ---------- 受入済み配置を整えるCompact rescue ----------
// 最小周長テンプレートの全アンカーを安価に走査し、「衝突セル数」と
// 「概算移動費」の2基準で上位だけを残す。その後に正確な衝突組を復元する。
// 以下は探索量の上限であり、同時に動かせる組数の上限ではない。
constexpr int RESCUE_TARGET_SHORTLIST_PER_METRIC = 160;
constexpr int RESCUE_TARGET_REPAIR_LIMIT = 8;
constexpr int RESCUE_DESTINATION_ANCHOR_LIMIT = 4096;
constexpr int RESCUE_DESTINATION_ANCHOR_GLOBAL_LIMIT = 50000;
constexpr int RESCUE_DESTINATION_LEGAL_LIMIT = 64;
constexpr int RESCUE_DESTINATION_LIMIT = 10;
constexpr int RESCUE_BEAM_WIDTH = 32;
constexpr int RESCUE_REPAIR_NODE_LIMIT = 2048;
constexpr int RESCUE_ROLLOUT_CANDIDATE_LIMIT = 2;

// ---------- 断片化で置けない到着を救うNoRegion Push-out ----------
// NoRegionはCompact rescueより発生しやすいため、同じ探索を小さい上限で回す。
// 面積不足は再配置では直せないので、この探索の対象にはしない。
constexpr int PUSHOUT_TARGET_SHORTLIST_PER_METRIC = 96;
constexpr int PUSHOUT_TARGET_REPAIR_LIMIT = 4;
constexpr int PUSHOUT_DESTINATION_ANCHOR_LIMIT = 2048;
constexpr int PUSHOUT_DESTINATION_ANCHOR_GLOBAL_LIMIT = 16000;
constexpr int PUSHOUT_DESTINATION_LEGAL_LIMIT = 40;
constexpr int PUSHOUT_DESTINATION_LIMIT = 8;
constexpr int PUSHOUT_REPAIR_NODE_LIMIT = 1024;

// ---------- root actionを比較する未来rollout ----------
// screenは2シナリオ×4到着、confirmationは独立な8シナリオ×12到着を使う。
constexpr int ROOT_SCREEN_SCENARIO_COUNT = 2;
constexpr int ROOT_SCREEN_ROLLOUT_LENGTH = 4;
constexpr int ROOT_CONFIRM_SCENARIO_COUNT = 8;
constexpr int ROOT_CONFIRM_ROLLOUT_LENGTH = 12;
constexpr int ROOT_CONFIRMATION_TURN_LIMIT = 4;
constexpr int ROOT_ROLLOUT_NORMAL_ALTERNATIVE_LIMIT = 2;

// 比較実験用のコンパイルスイッチ。通常提出では全機能を有効にする。
#ifdef AHC069_DISABLE_NO_REGION_PUSHOUT
constexpr bool ENABLE_NO_REGION_PUSHOUT = false;
#else
constexpr bool ENABLE_NO_REGION_PUSHOUT = true;
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
// screen用の既存関数ではRescueという名前を使うため、同じ値を別名で参照する。
constexpr int RESCUE_ROLLOUT_SCENARIO_COUNT = ROOT_SCREEN_SCENARIO_COUNT;
constexpr int RESCUE_ROLLOUT_LENGTH = ROOT_SCREEN_ROLLOUT_LENGTH;
constexpr int ROOT_ROLLOUT_MAX_ACTIONS = ROOT_SCREEN_MAX_ACTIONS;
constexpr uint64_t ROOT_ROLLOUT_SEQUENCE_BLOCK_SIZE = 1000003ULL;
constexpr int ROOT_ROLLOUT_SEQUENCE_BLOCKS_PER_BATCH = ROOT_CONFIRM_SCENARIO_COUNT / 2;
constexpr int BOARD_MASK_WORDS =
    (BOARD_SIDE_LIMIT * BOARD_SIDE_LIMIT + 63) / 64;

// 下記の診断配列とscreen集計は2本の未来・最大2候補を明示的に参照する。
// 定数だけを変更して添字と集計式がずれる事故をcompile時に防ぐ。
static_assert(ROOT_SCREEN_SCENARIO_COUNT == 2);
static_assert(RESCUE_ROLLOUT_CANDIDATE_LIMIT == 2);
static_assert(ROOT_ROLLOUT_NORMAL_ALTERNATIVE_LIMIT == 2);
static_assert(ROOT_CONFIRM_SCENARIO_COUNT > 0 && ROOT_CONFIRM_SCENARIO_COUNT % 2 == 0);

// テンプレートは「主矩形」と、面積の端数を埋める「追加の1行または1列」で表す。
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

// 受入済みの1組の状態。
// max_perimeterは、その組がこれまで経験した最大周長である。
// 料金は最大周長で決まり、一度悪化した料金は後で整形しても戻らないため履歴を保持する。
struct GroupState {
    bool active = false;
    ll s = 0;
    ll t = 0;
    ll v = 0;
    int p = 0;
    int max_perimeter = 0;
    vector<Cell> cells;
};

// 既存組1つの移動先。全MovePlanの旧領域を消してから、新領域をまとめて配置する。
struct MovePlan {
    int id;
    vector<Cell> cells;
    int perimeter;
};

// 1ターンに出力する行動。movesは既存組の再配置、arrivalは新規組の配置である。
// immediate_gainは再配置候補の「到着料金 - 移動費 - 既存料金の悪化」を保持する。
struct TurnPlan {
    vector<MovePlan> moves;
    optional<vector<Cell>> arrival;
    int arrival_perimeter = 0;
    ll immediate_gain = numeric_limits<ll>::min();
};

// 面積pを「矩形＋端数の1行/1列」で表せる全テンプレートを列挙する。
// 既存組の移動先では、確定済み料金を悪化させない周長までの全形状を利用する。
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

        // full×widthの主矩形に、端数行を上または下から付ける。
        for (int below = 0; below < 2; below++) {
            for (int right = 0; right < 2; right++) {
                Rect main_rect{below ? 0 : 1, 0, full, width};
                Rect extra_rect{below ? full : 0, right ? width - rem : 0, 1, rem};
                add_shape(main_rect, extra_rect, full + 1, width, perimeter);
            }
        }

        // 上の転置形。端数列を左または右から付ける。
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

// テンプレートを左上(base_x, base_y)へ置き、実際のセル列へ展開する。
vector<Cell> materialize_shape(const Shape &shape, int base_x, int base_y) {
    vector<Cell> region;
    region.reserve(shape.main_rect.h * shape.main_rect.w +
                   shape.extra_rect.h * shape.extra_rect.w);
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
// テンプレートの合法性を矩形2個の和としてO(1)で調べるために使う。
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

// N=50を1行1ワードに収め、templateの合法anchorだけをbase_y昇順で列挙する。
// invalid_start[w][k][x]のbit yは、xから2^k行のどこかで
// [y,y+w)に池または占有セルがあることを表す。行方向は冪長sparse tableなので、
// 任意の高さを重なる2区間のORで厳密に照会できる。
// 累積和版と合法集合・base_x/base_y順を変えず、不合法anchorの個別照会だけを省く。
struct LegalAnchorIndex {
    static constexpr int MAX_N = BOARD_SIDE_LIMIT;
    static constexpr int LOG_N = 6;

    int n = 0;
    array<array<array<uint64_t, MAX_N>, LOG_N>, MAX_N + 1> invalid_start{};

    LegalAnchorIndex(const vs &park, const vvi *owner) : n(park.size()) {
        array<uint64_t, MAX_N> blocked_rows{};
        for (int x = 0; x < n; x++) {
            for (int y = 0; y < n; y++) {
                if (park[x][y] == '#' || (owner != nullptr && (*owner)[x][y] != -1)) {
                    blocked_rows[x] |= 1ULL << y;
                }
            }
        }

        // bit yを「この行の幅w窓[y,y+w)が塞がる」にする。
        for (int x = 0; x < n; x++) {
            uint64_t blocked_windows = 0;
            for (int w = 1; w <= n; w++) {
                blocked_windows |= blocked_rows[x] >> (w - 1);
                invalid_start[w][0][x] = blocked_windows;
            }
        }
        for (int w = 1; w <= n; w++) {
            for (int level = 1; level < LOG_N; level++) {
                int half = 1 << (level - 1);
                int length = 2 * half;
                for (int x = 0; x + length <= n; x++) {
                    invalid_start[w][level][x] =
                        invalid_start[w][level - 1][x] |
                        invalid_start[w][level - 1][x + half];
                }
            }
        }
    }

    uint64_t rectangle_invalid_base_y(const Rect &rect, int base_x) const {
        if (rect.h == 0 || rect.w == 0) return 0;
        int level = 31 - __builtin_clz(rect.h);
        int length = 1 << level;
        int x = base_x + rect.x;
        uint64_t invalid = invalid_start[rect.w][level][x] |
                           invalid_start[rect.w][level][x + rect.h - length];
        // invalidのbit qは矩形自身の開始列qを表す。q=base_y+rect.yを
        // base_yのbit位置へ揃える。
        return invalid >> rect.y;
    }

    uint64_t legal_base_y_mask(const Shape &shape, int base_x) const {
        uint64_t invalid = rectangle_invalid_base_y(shape.main_rect, base_x) |
                           rectangle_invalid_base_y(shape.extra_rect, base_x);
        int base_y_count = n - shape.w + 1;
        uint64_t range_mask = (1ULL << base_y_count) - 1;
        return ~invalid & range_mask;
    }
};

// テンプレートが1つも置けない場合の完全なフォールバック。
// 各空き連結成分をBFSし、先頭pセルを取る。BFSのprefixは常に連結なので、
// pセル以上の空き連結成分が存在すれば必ず合法領域を返せる。
optional<vector<Cell>> find_connected_region(const vs &park, const vvi &owner, int p) {
    int n = park.size();
    vvb visited(n, vb(n));

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
                    int nx = x + ORTHOGONAL_DX[dir];
                    int ny = y + ORTHOGONAL_DY[dir];
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

    int perimeter = 0;
    for (auto [x, y] : cells) {
        for (int dir = 0; dir < 4; dir++) {
            int nx = x + ORTHOGONAL_DX[dir];
            int ny = y + ORTHOGONAL_DY[dir];
            if (!inside(nx, ny, n, n) || !in_region[nx * n + ny]) {
                perimeter++;
            }
        }
    }
    return perimeter;
}

using i128 = __int128_t;

// root比較の「現在差 + 重み付き未来差」を整数で合成する。
// weight=1000では従来式と符号・順位が厳密に一致する。
i128 root_margin_scaled(int scenario_count, ll direct_gain, i128 future_delta_sum) {
    return (i128)ROOT_WEIGHT_SCALE * scenario_count * direct_gain +
           (i128)case_root_future_weight_milli * future_delta_sum;
}

ll root_margin_scaled_ll(int scenario_count, ll direct_gain, ll future_delta_sum) {
    i128 value = root_margin_scaled(scenario_count, direct_gain, future_delta_sum);
    assert(numeric_limits<ll>::min() <= value && value <= numeric_limits<ll>::max());
    return (ll)value;
}

bool root_scenario_positive(ll direct_gain, ll future_delta) {
    return (i128)ROOT_WEIGHT_SCALE * direct_gain +
               (i128)case_root_future_weight_milli * future_delta >
           0;
}

long double root_margin_fee(i128 scaled_margin, int scenario_count) {
    return (long double)scaled_margin / (ROOT_WEIGHT_SCALE * scenario_count);
}

// 公式の料金 round(4*V*sqrt(P)/周長) を整数演算で確定する。
// sqrtによる近似値を初期値にし、丸め境界を128bit整数で前後補正することで
// 浮動小数点誤差による1円のずれを防ぐ。
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

// 1回の再配置で支払う費用 round(R*V)。Rは入力時に1000倍の整数へ変換済み。
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

// 行動確定後にだけ空き最大連結成分を測る診断関数。
// 配置候補の生成には使わないため、診断対象のアルゴリズムへ影響しない。
__attribute__((noinline)) int largest_free_component(const vs &park, const vvi &owner) {
    int n = park.size();
    vector<char> visited(n * n, false);
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
                    int nx = x + ORTHOGONAL_DX[dir];
                    int ny = y + ORTHOGONAL_DY[dir];
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
    DenseBoxTrim,
    PerimeterDescent,
};

// 通常配置がどの候補生成器から選ばれたかを、診断用に保持する。
struct NormalPlacementChoice {
    vector<Cell> cells;
    int perimeter;
    PlacementSource source;
};

// 1ケース内で共有される未知の滞在時間スケールthetaをベイズ推定する。
// 観測済みの滞在時間だけでなく、「未到着の全組は現在時刻Sより後に開始する」
// という打ち切り情報も尤度へ入れる。thetaは2000..8000の61点で離散化する。
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
        // y=1-exp(-x)へ変数変換し、指数分布の重みを積分幅へ吸収する。
        // xを等間隔にするとSが小さいときだけ積分区間が極端に長くなり、
        // 確率質量が集中するx=0付近の精度が落ちるため、この形を使う。
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
        // make_posterior()と式が重複しているが、意図的に統合しない。
        // 関数を分けると加算順やFMAの有無が変わり、thetaの微小差が配置の同点判定を
        // 変えたことがあるため、提出挙動を固定できるこの計算順を保つ。
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
        // 事後分布そのものが61点の離散分布なので、点間補間はせず左連続の逆CDFを使う。
        for (int k = 0; k < PARTICLE_COUNT; k++) {
            cumulative += posterior.weights[k];
            if (cumulative >= target) return THETA_MIN + THETA_STEP * k;
        }
        return THETA_MAX;
    }
};

long double inverse_standard_normal(long double probability) {
    // Peter J. Acklamの有理関数近似による標準正規分布の逆CDF。
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

// sampled DLPを無効化した比較ビルドで使う旧shadow-priceモデル。
// 通常提出ではSampledDlpShadowModelを使用するが、A/B比較用に残している。
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

// 「未観測の1組は現在時刻Sより後に始まる」という条件付き未来分布。
// 指数分布からlを取り滞在時間をD=l+1とし、開始時刻はH-l個の整数から一様に選ばれる。
// y=1-exp(-l/theta)への変数変換で指数密度を吸収し、正規化定数は分子・分母で相殺する。
// 旧64時間帯shadowと、通常配置の将来snapshot生成の両方で使う。
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
    // 今回の組がセル時間を占有することで、将来受け入れられなくなる料金の推定生値。
    // case expert倍率はevaluate_arrival_decision / Push-out gateで各1回だけ適用する。
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
    // expert倍率適用前のsampled DLP生値。倍率はadmission側で一度だけ掛ける。
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

// sampled DLP（deterministic linear programming）による機会損失モデル。
// 現在から時刻上限までを最大16区間に分け、各区間の「1セル・1時間」の価格を求める。
// 未来256組を低食い違い列から決定的に生成し、空き容量と未来需要が釣り合うよう
// 双対価格を座標降下で更新する。ここでは総セル時間だけを価格付けし、
// 盤面の連結性や形状は後段の配置・Push-outが扱う。
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

    // 61個のtheta候補それぞれについて、公式の丸め後分布から
    // Q_theta(s)=Pr(未来の開始時刻>S)を全時刻分前計算する。
    void initialize(const vector<vector<Shape>> &compact_shapes) {
        for (int p = 4; p <= 150; p++) {
            minimum_perimeter[p] = compact_shapes[p].front().perimeter;
        }

        // 後ろ向き漸化式なので浮動小数点誤差を除けば厳密。
        // 61×100000個のfloatを使い、メモリは約24MB。
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
        // 初回、4/8/16ターン、その後16ターンごとに再計算する。
        // さらに、前回の時間区間境界を2本以上通過した場合も価格を更新する。
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
        // thetaの10/30/50/70/90%点を均等に使い、base 2/3/5/7のradical inverseで
        // 滞在時間・開始時刻・面積・価値を生成する。乱数を使わないため毎回再現可能。
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
            // 中点を使って5つのtheta層へ51,51,52,51,51件を割り当てる。
            // 面積生成のbase-5列とは独立にし、thetaと面積の人工的な相関を避ける。
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
        // 各時間帯の容量から、現在盤面の既知の占有セル時間を引く。
        // 未来sampleには「残り組数/256」の重みを掛け、実際の総需要へ換算する。
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
        // 1座標ずつ価格を更新するGauss-Seidel型の座標降下。
        // 他時間帯の価格を引いた残余価値/当該負荷が、そのrequestの離脱価格になる。
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

        // 量子化は全sweep終了後だけ行う。途中で丸めると、後続座標の離脱価格が変わる。
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
        // bucket境界、未来sample、双対価格を一括して更新する。
        // sample hashは決定性確認用で、意思決定には使わない。
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
        // 凍結した価格に「pセル×各時間帯との重なり」を掛け、今回の機会損失を求める。
        // rollout中も価格を再学習せず、実ターンと同じ情報だけで全branchを比較する。
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

// sampled DLPを無効化した比較ビルド用の旧機会損失計算。
// 各64時間帯で、今回占有するセル時間に、容量不足で押し出される未来組の
// 料金密度を掛ける。通常提出ではこの関数を通らない。
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
    int connected_polish_changed_placements = 0;
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
    int connected_polish_candidate_turns = 0;
    int connected_polish_static_filtered_turns = 0;
    int connected_polish_eligible_turns = 0;
    int dense_box_size_filtered_turns = 0;
    int dense_box_eligible_turns = 0;
    int dense_box_value_filtered_turns = 0;
    int dense_box_budget_skips = 0;
    long long dense_box_anchors_checked = 0;
    long long dense_box_feasible_anchors = 0;
    long long dense_box_shortlisted_anchors = 0;
    long long dense_box_component_failures = 0;
    long long dense_box_trim_attempts = 0;
    long long dense_box_trimmed_cells = 0;
    long long dense_box_trim_failures = 0;
    long long dense_box_nonimproving_candidates = 0;
    long long dense_box_duplicate_candidates = 0;
    long long dense_box_candidates = 0;
    long long dense_box_future_fit_rejections = 0;
    long long dense_box_perimeter_improvement = 0;
    int dense_box_successes = 0;
    long long perimeter_descent_attempts = 0;
    long long perimeter_descent_prefilter_rejections = 0;
    long long perimeter_descent_remove_prefilter_rejections = 0;
    long long perimeter_descent_steps = 0;
    long long perimeter_descent_candidates = 0;
    long long small_group_perimeter_descent_attempts = 0;
    long long small_group_perimeter_descent_steps = 0;
    long long small_group_perimeter_descent_candidates = 0;
    long long small_group_perimeter_descent_future_fit_rejections = 0;
    long long small_group_perimeter_descent_perimeter_improvement = 0;
    ll small_group_perimeter_descent_fee_gain = 0;
    int small_group_perimeter_descent_successes = 0;
    long long perimeter_descent_nonimproving_candidates = 0;
    long long perimeter_descent_future_fit_rejections = 0;
    long long perimeter_descent_perimeter_improvement = 0;
    int perimeter_descent_successes = 0;
    long long shortlisted_candidates = 0;
    long long future_fit_snapshots = 0;
};

struct PlacementCandidate {
    vector<Cell> cells;
    uint64_t region_hash = 0;
    int perimeter = 0;
    // incremental_cost: 現在盤面へ候補を足したことで増える退去時刻境界コスト。
    // absolute_cost: 候補を足した後の境界コストそのもの。
    // どちらも、退去時刻が近い組同士が接するほど小さくなる。
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

// 全合法候補を高価なfuture-fitへ渡さず、性質の異なる最大6候補へ圧縮する。
// 高外周率placement expertだけはglobal枠を増やして最大8候補とする。
// 内訳はincremental上位3または5、absolute最良、列挙順の最初、別象限の最良。
// 同一領域はhashで安価に絞った後、セル集合を比較して重複除去する。
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
        if ((int)global_best.size() < case_placement_config.global_shortlist ||
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
                if ((int)global_best.size() > case_placement_config.global_shortlist) {
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
        if ((int)result.size() > case_placement_config.shortlist_limit) {
            result.resize(case_placement_config.shortlist_limit);
        }
        return result;
    }
};

bool validate_connected_region(const vector<Cell> &cells, int n);

// grow-and-trimの「trim」部分。
// 貪欲成長をちょうどpセルで止めると、最後の数セルだけが突起になることがある。
// そこで一度p+8（高外周率expertではp+12）セルまで育て、
// 連結性を壊す関節点を避けながら境界セルを削る。
// 削除後の周長変化が良いセルを優先し、pセルへ戻した領域を追加候補として返す。
optional<vector<Cell>> trim_connected_region(
    const vs &park, const vvi &owner, const vector<Cell> &grown, int p,
    long long &trimmed_cells,
    const vector<vector<long double>> *primary_removal_cost = nullptr,
    const vector<vector<long double>> *secondary_removal_cost = nullptr) {
    int n = park.size();
    vector<char> selected(n * n, false);
    vector<int> growth_order(n * n, -1);
    for (int index = 0; index < (int)grown.size(); index++) {
        auto [x, y] = grown[index];
        int cell = x * n + y;
        selected[cell] = true;
        growth_order[cell] = index;
    }

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
                int nx = x + ORTHOGONAL_DX[dir];
                int ny = y + ORTHOGONAL_DY[dir];
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
        long double best_primary_cost = -numeric_limits<long double>::infinity();
        long double best_secondary_cost = -numeric_limits<long double>::infinity();
        for (const Cell &position : grown) {
            int cell = position.first * n + position.second;
            if (!selected[cell] || articulation[cell]) continue;
            int x = cell / n;
            int y = cell % n;
            int selected_neighbors = 0;
            for (int dir = 0; dir < 4; dir++) {
                int nx = x + ORTHOGONAL_DX[dir];
                int ny = y + ORTHOGONAL_DY[dir];
                if (inside(nx, ny, n, n) && selected[nx * n + ny]) selected_neighbors++;
            }
            if (selected_neighbors == 4) continue;
            int perimeter_change = 2 * selected_neighbors - 4;
            long double primary_cost = primary_removal_cost
                                           ? (*primary_removal_cost)[x][y]
                                           : 0.0L;
            long double secondary_cost = secondary_removal_cost
                                             ? (*secondary_removal_cost)[x][y]
                                             : 0.0L;
            bool better = perimeter_change < best_perimeter_change;
            if (!better && perimeter_change == best_perimeter_change) {
                if (primary_removal_cost) {
                    better = primary_cost > best_primary_cost ||
                             (primary_cost == best_primary_cost &&
                              secondary_cost > best_secondary_cost) ||
                             (primary_cost == best_primary_cost &&
                              secondary_cost == best_secondary_cost &&
                              growth_order[cell] > best_growth_order) ||
                             (primary_cost == best_primary_cost &&
                              secondary_cost == best_secondary_cost &&
                              growth_order[cell] == best_growth_order &&
                              (removed == -1 || cell < removed));
                } else {
                    // 既存grow-and-trimは従来の列挙順を完全に保つ。
                    better = growth_order[cell] > best_growth_order ||
                             (growth_order[cell] == best_growth_order &&
                              (removed == -1 || cell < removed));
                }
            }
            if (better) {
                removed = cell;
                best_perimeter_change = perimeter_change;
                best_growth_order = growth_order[cell];
                best_primary_cost = primary_cost;
                best_secondary_cost = secondary_cost;
            }
        }
        if (removed == -1) return nullopt;
        selected[removed] = false;
        remaining--;
        trimmed_cells++;
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

// 旧connected配置を、近最小周長box内の高密度な空き成分から作り直す。
// 全anchorはfree集合の周長までbitsetで安価に採点し、Tarjan trimは上位16件だけに限定する。
// ここでは候補を返すだけで、旧Acceptedの保護・料金増・future-fit非悪化は呼び出し側で確認する。
vector<vector<Cell>> make_dense_box_trim_candidates(
    const vs &park, const vvi &owner, int p, int minimum_perimeter,
    int old_perimeter, const vector<vector<long double>> &incremental_cell,
    const vector<vector<long double>> &absolute_cell,
    const vector<vector<long double>> &incremental_prefix,
    const vector<vector<long double>> &absolute_prefix,
    TemporalPlacementDiagnostics &diagnostics) {
    struct DenseBoxAnchor {
        int base_x = 0;
        int base_y = 0;
        int h = 0;
        int w = 0;
        int free_cells = 0;
        int free_perimeter = 0;
        int box_perimeter = 0;
        int quadrant = 0;
        long double incremental_sum = 0.0L;
        long double absolute_sum = 0.0L;
        long long order = 0;
    };

    auto anchor_less = [](const DenseBoxAnchor &lhs, const DenseBoxAnchor &rhs) {
        return tuple(lhs.free_perimeter, lhs.box_perimeter, lhs.free_cells,
                     abs(lhs.h - lhs.w), lhs.incremental_sum, lhs.absolute_sum,
                     lhs.order) <
               tuple(rhs.free_perimeter, rhs.box_perimeter, rhs.free_cells,
                     abs(rhs.h - rhs.w), rhs.incremental_sum, rhs.absolute_sum,
                     rhs.order);
    };

    int n = park.size();
    int extra_cells = DENSE_BOX_EXTRA_CELLS;
    int maximum_box_perimeter =
        min(old_perimeter - 2, minimum_perimeter + DENSE_BOX_PERIMETER_MARGIN);
    if (maximum_box_perimeter < minimum_perimeter) return {};

    vector<vi> blocked_prefix = make_blocked_prefix(park, owner);
    array<uint64_t, 50> free_rows{};
    for (int x = 0; x < n; x++) {
        for (int y = 0; y < n; y++) {
            if (park[x][y] == '.' && owner[x][y] == -1) {
                free_rows[x] |= 1ULL << y;
            }
        }
    }

    vector<DenseBoxAnchor> global_best;
    array<optional<DenseBoxAnchor>, 4> quadrant_best;
    long long anchor_order = 0;
    for (int h = 1; h <= n; h++) {
        for (int w = 1; w <= n; w++) {
            int area = h * w;
            int box_perimeter = 2 * (h + w);
            if (area < p || area > p + extra_cells ||
                box_perimeter > maximum_box_perimeter) {
                continue;
            }
            uint64_t width_mask = (1ULL << w) - 1;
            for (int base_x = 0; base_x + h <= n; base_x++) {
                for (int base_y = 0; base_y + w <= n; base_y++) {
                    diagnostics.dense_box_anchors_checked++;
                    int blocked = rectangle_sum(blocked_prefix, base_x, base_y, h, w);
                    int free_count = area - blocked;
                    if (free_count < p) continue;
                    diagnostics.dense_box_feasible_anchors++;

                    int shared_edges = 0;
                    uint64_t previous = 0;
                    for (int dx = 0; dx < h; dx++) {
                        uint64_t row = (free_rows[base_x + dx] >> base_y) & width_mask;
                        shared_edges += __builtin_popcountll(row & (row >> 1));
                        if (dx > 0) shared_edges += __builtin_popcountll(row & previous);
                        previous = row;
                    }
                    int free_perimeter = 4 * free_count - 2 * shared_edges;
                    long double incremental_sum = rectangle_sum(
                        incremental_prefix, base_x, base_y, h, w);
                    long double absolute_sum = rectangle_sum(
                        absolute_prefix, base_x, base_y, h, w);
                    int lower_half = 2 * base_x + h >= n;
                    int right_half = 2 * base_y + w >= n;
                    DenseBoxAnchor anchor{base_x, base_y, h, w, free_count,
                                          free_perimeter, box_perimeter,
                                          2 * lower_half + right_half,
                                          incremental_sum, absolute_sum,
                                          anchor_order++};

                    global_best.push_back(anchor);
                    sort(global_best.begin(), global_best.end(), anchor_less);
                    if ((int)global_best.size() > DENSE_BOX_GLOBAL_ANCHOR_LIMIT) {
                        global_best.pop_back();
                    }
                    if (!quadrant_best[anchor.quadrant] ||
                        anchor_less(anchor, *quadrant_best[anchor.quadrant])) {
                        quadrant_best[anchor.quadrant] = anchor;
                    }
                }
            }
        }
    }

    vector<DenseBoxAnchor> anchors = global_best;
    auto same_anchor = [](const DenseBoxAnchor &lhs, const DenseBoxAnchor &rhs) {
        return lhs.base_x == rhs.base_x && lhs.base_y == rhs.base_y &&
               lhs.h == rhs.h && lhs.w == rhs.w;
    };
    for (const auto &candidate : quadrant_best) {
        if (!candidate) continue;
        bool duplicate = any_of(anchors.begin(), anchors.end(),
                                [&](const DenseBoxAnchor &anchor) {
                                    return same_anchor(anchor, *candidate);
                                });
        if (!duplicate && (int)anchors.size() < DENSE_BOX_TOTAL_ANCHOR_LIMIT) {
            anchors.push_back(*candidate);
        }
    }
    diagnostics.dense_box_shortlisted_anchors += anchors.size();

    vector<vector<Cell>> result;
    auto add_result = [&](optional<vector<Cell>> candidate) {
        if (!candidate) {
            diagnostics.dense_box_trim_failures++;
            return;
        }
        int perimeter = calc_perimeter(*candidate, n);
        if (perimeter >= old_perimeter) {
            diagnostics.dense_box_nonimproving_candidates++;
            return;
        }
        if ((int)candidate->size() != p ||
            !validate_connected_region(*candidate, n)) {
            diagnostics.dense_box_trim_failures++;
            return;
        }
        for (auto [x, y] : *candidate) {
            if (park[x][y] != '.' || owner[x][y] != -1) {
                diagnostics.dense_box_trim_failures++;
                return;
            }
        }
        for (const vector<Cell> &existing : result) {
            if (same_region(existing, *candidate)) {
                diagnostics.dense_box_duplicate_candidates++;
                return;
            }
        }
        diagnostics.dense_box_candidates++;
        result.push_back(std::move(*candidate));
    };

    for (const DenseBoxAnchor &anchor : anchors) {
        vector<char> visited(anchor.h * anchor.w, false);
        bool found_component = false;
        for (int start_x = 0; start_x < anchor.h && !found_component; start_x++) {
            for (int start_y = 0; start_y < anchor.w && !found_component; start_y++) {
                int start = start_x * anchor.w + start_y;
                int global_x = anchor.base_x + start_x;
                int global_y = anchor.base_y + start_y;
                if (visited[start] || park[global_x][global_y] != '.' ||
                    owner[global_x][global_y] != -1) {
                    continue;
                }
                vector<Cell> component;
                queue<pair<int, int>> que;
                visited[start] = true;
                que.emplace(start_x, start_y);
                while (!que.empty()) {
                    auto [local_x, local_y] = que.front();
                    que.pop();
                    component.emplace_back(anchor.base_x + local_x,
                                           anchor.base_y + local_y);
                    for (int dir = 0; dir < 4; dir++) {
                        int next_x = local_x + ORTHOGONAL_DX[dir];
                        int next_y = local_y + ORTHOGONAL_DY[dir];
                        if (!inside(next_x, next_y, anchor.h, anchor.w)) continue;
                        int next = next_x * anchor.w + next_y;
                        int next_global_x = anchor.base_x + next_x;
                        int next_global_y = anchor.base_y + next_y;
                        if (visited[next] || park[next_global_x][next_global_y] != '.' ||
                            owner[next_global_x][next_global_y] != -1) {
                            continue;
                        }
                        visited[next] = true;
                        que.emplace(next_x, next_y);
                    }
                }
                if ((int)component.size() < p) continue;
                found_component = true;
                if ((int)component.size() == p) {
                    add_result(optional<vector<Cell>>(std::move(component)));
                    continue;
                }

                diagnostics.dense_box_trim_attempts += 2;
                add_result(trim_connected_region(
                    park, owner, component, p, diagnostics.dense_box_trimmed_cells,
                    &incremental_cell, &absolute_cell));
                add_result(trim_connected_region(
                    park, owner, component, p, diagnostics.dense_box_trimmed_cells,
                    &absolute_cell, &incremental_cell));
            }
        }
        if (!found_component) diagnostics.dense_box_component_failures++;
    }
    return result;
}

struct RegionSwap {
    int removed = -1;
    int added = -1;
    // gain=k_add_after-k_remove。周長は2*gainだけ短くなる。
    int gain = 0;
    long double incremental_delta = 0.0L;
    long double absolute_delta = 0.0L;
};

struct RegionSwapNeighborhood {
    vector<int> selected_neighbors;
    vector<int> frontier_neighbors;
    vector<int> frontier;
    int minimum_removed_neighbors = 5;
    int maximum_added_neighbors = 0;
};

RegionSwapNeighborhood make_region_swap_neighborhood(
    const vs &park, const vvi &owner, const vector<char> &selected) {
    int n = park.size();
    RegionSwapNeighborhood result;
    result.selected_neighbors.assign(n * n, 0);
    result.frontier_neighbors.assign(n * n, 0);
    vector<char> in_frontier(n * n, false);
    for (int cell = 0; cell < n * n; cell++) {
        if (!selected[cell]) continue;
        int x = cell / n;
        int y = cell % n;
        for (int dir = 0; dir < 4; dir++) {
            int nx = x + ORTHOGONAL_DX[dir];
            int ny = y + ORTHOGONAL_DY[dir];
            if (!inside(nx, ny, n, n)) continue;
            int next = nx * n + ny;
            if (selected[next]) {
                result.selected_neighbors[cell]++;
                continue;
            }
            if (park[nx][ny] != '.' || owner[nx][ny] != -1 ||
                in_frontier[next]) {
                continue;
            }
            in_frontier[next] = true;
            result.frontier.push_back(next);
        }
        chmin(result.minimum_removed_neighbors,
              result.selected_neighbors[cell]);
    }
    for (int added : result.frontier) {
        int x = added / n;
        int y = added % n;
        for (int dir = 0; dir < 4; dir++) {
            int nx = x + ORTHOGONAL_DX[dir];
            int ny = y + ORTHOGONAL_DY[dir];
            if (inside(nx, ny, n, n) && selected[nx * n + ny]) {
                result.frontier_neighbors[added]++;
            }
        }
        chmax(result.maximum_added_neighbors,
              result.frontier_neighbors[added]);
    }
    return result;
}

vector<char> find_region_articulation(const vector<char> &selected, int n) {
    vector<int> discovery(n * n, -1), low(n * n), parent(n * n, -1);
    vector<char> articulation(n * n, false);
    int timer = 0;
    function<void(int)> dfs = [&](int cell) {
        discovery[cell] = low[cell] = timer++;
        int children = 0;
        int x = cell / n;
        int y = cell % n;
        for (int dir = 0; dir < 4; dir++) {
            int nx = x + ORTHOGONAL_DX[dir];
            int ny = y + ORTHOGONAL_DY[dir];
            if (!inside(nx, ny, n, n)) continue;
            int next = nx * n + ny;
            if (!selected[next]) continue;
            if (discovery[next] == -1) {
                parent[next] = cell;
                children++;
                dfs(next);
                chmin(low[cell], low[next]);
                if (parent[cell] == -1 && children > 1) {
                    articulation[cell] = true;
                }
                if (parent[cell] != -1 && low[next] >= discovery[cell]) {
                    articulation[cell] = true;
                }
            } else if (next != parent[cell]) {
                chmin(low[cell], discovery[next]);
            }
        }
    };
    for (int cell = 0; cell < n * n; cell++) {
        if (selected[cell] && discovery[cell] == -1) dfs(cell);
    }
    return articulation;
}

optional<RegionSwap> find_best_strict_region_swap(
    const vs &park, const vvi &owner, const vector<char> &selected,
    const vector<vector<long double>> &incremental_cell,
    const vector<vector<long double>> &absolute_cell,
    bool *prefilter_rejected = nullptr,
    long long *remove_prefilter_rejections = nullptr) {
    if (prefilter_rejected) *prefilter_rejected = false;
    int n = park.size();
    RegionSwapNeighborhood neighborhood =
        make_region_swap_neighborhood(park, owner, selected);
    // add側近傍数はremoveで増えない。この条件なら
    // articulationを求めるまでもなくstrict降下は不可能である。
    if (neighborhood.maximum_added_neighbors <=
        neighborhood.minimum_removed_neighbors) {
        if (prefilter_rejected) *prefilter_rejected = true;
        return nullopt;
    }
    vector<char> articulation = find_region_articulation(selected, n);
    optional<RegionSwap> best;
    for (int removed = 0; removed < n * n; removed++) {
        if (!selected[removed] || articulation[removed]) continue;
        int rx = removed / n;
        int ry = removed % n;
        int removed_neighbors = neighborhood.selected_neighbors[removed];
        // removeに隣接するfrontierはswap後に近傍数が1減り、それ以外も増えない。
        // 現frontierの最大次数がremove次数以下なら正gainは不可能なので、
        // tie順や選択結果を変えずにこのremoveの全pairを省く。
        if (neighborhood.maximum_added_neighbors <= removed_neighbors) {
            if (remove_prefilter_rejections) {
                (*remove_prefilter_rejections)++;
            }
            continue;
        }
        for (int added : neighborhood.frontier) {
            int ax = added / n;
            int ay = added % n;
            int adjacent_to_removed = abs(ax - rx) + abs(ay - ry) == 1;
            int added_neighbors =
                neighborhood.frontier_neighbors[added] - adjacent_to_removed;
            int gain = added_neighbors - removed_neighbors;
            if (gain <= 0) continue;
            RegionSwap candidate{
                removed, added, gain,
                incremental_cell[ax][ay] - incremental_cell[rx][ry],
                absolute_cell[ax][ay] - absolute_cell[rx][ry]};
            if (!best || candidate.gain > best->gain ||
                (candidate.gain == best->gain &&
                 candidate.incremental_delta < best->incremental_delta) ||
                (candidate.gain == best->gain &&
                 candidate.incremental_delta == best->incremental_delta &&
                 candidate.absolute_delta < best->absolute_delta) ||
                (candidate.gain == best->gain &&
                 candidate.incremental_delta == best->incremental_delta &&
                 candidate.absolute_delta == best->absolute_delta &&
                 pair(candidate.removed, candidate.added) <
                     pair(best->removed, best->added))) {
                best = candidate;
            }
        }
    }
    return best;
}

vector<Cell> materialize_selected_region(const vector<char> &selected, int n) {
    vector<Cell> result;
    result.reserve(count(selected.begin(), selected.end(), true));
    for (int cell = 0; cell < n * n; cell++) {
        if (selected[cell]) result.emplace_back(cell / n, cell % n);
    }
    return result;
}

// swap構築側でも合法frontierだけを加えるが、各実体化後に面積以外の
// 池・占有・重複・盤外・4連結をまとめて再検証し、探索バグを局所化する。
bool validate_free_connected_region(const vs &park, const vvi &owner,
                                    const vector<Cell> &cells) {
    int n = park.size();
    if (!validate_connected_region(cells, n)) return false;
    for (auto [x, y] : cells) {
        if (park[x][y] != '.' || owner[x][y] != -1) return false;
    }
    return true;
}

// selected connected領域の突起1セルと外側の凹部1セルを交換する局所降下。
// 非関節セルだけを外し、k_add_after>k_removeのときだけ反映する。
// 終端形がfuture-fitで落ちても浅い改善を失わないよう全中間形を返す。
vector<vector<Cell>> make_perimeter_descent_candidates(
    const vs &park, const vvi &owner, const vector<Cell> &initial,
    const vector<vector<long double>> &incremental_cell,
    const vector<vector<long double>> &absolute_cell,
    TemporalPlacementDiagnostics &diagnostics) {
    diagnostics.perimeter_descent_attempts++;
    bool small_group =
        (int)initial.size() < DENSE_BOX_MIN_GROUP_SIZE;
    if (small_group) {
        diagnostics.small_group_perimeter_descent_attempts++;
    }
    int n = park.size();
    vector<char> selected(n * n, false);
    for (auto [x, y] : initial) selected[x * n + y] = true;
    int steps = 0;
    vector<vector<Cell>> result;
    while (steps < PERIMETER_DESCENT_MAX_STEPS) {
        bool prefilter_rejected = false;
        optional<RegionSwap> best = find_best_strict_region_swap(
            park, owner, selected, incremental_cell, absolute_cell,
            &prefilter_rejected,
            &diagnostics.perimeter_descent_remove_prefilter_rejections);
        if (!best) {
            if (prefilter_rejected) {
                diagnostics.perimeter_descent_prefilter_rejections++;
            }
            break;
        }
        selected[best->removed] = false;
        selected[best->added] = true;
        vector<Cell> candidate = materialize_selected_region(selected, n);
        if (candidate.size() != initial.size() ||
            !validate_free_connected_region(park, owner, candidate)) {
            break;
        }
        steps++;
        result.push_back(std::move(candidate));
    }
    diagnostics.perimeter_descent_steps += steps;
    diagnostics.perimeter_descent_candidates += result.size();
    if (small_group) {
        diagnostics.small_group_perimeter_descent_steps += steps;
        diagnostics.small_group_perimeter_descent_candidates += result.size();
    }
    return result;
}

vector<vector<Cell>> make_connected_growth_candidates(
    const vs &park, const vvi &owner, int p,
    vector<vector<Cell>> &grow_and_trim_candidates,
    TemporalPlacementDiagnostics &diagnostics) {
    // 処理順:
    // 1. 空き連結成分を抽出する。
    // 2. 障害物・盤面端からの距離を求める。
    // 3. 各成分の端・対角・障害物距離から最大16または24個のseedを選ぶ。
    // 4. 選択済み隣接数が多いセルを優先し、連結なままpセルへ成長させる。
    // 5. 一部のseedではさらに8または12セル育て、trimした候補も作る。
    int n = park.size();
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

    // pセル以上を含む空き連結成分だけを候補にする。
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
                    int nx = x + ORTHOGONAL_DX[dir];
                    int ny = y + ORTHOGONAL_DY[dir];
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

    // 障害物や盤面端に近いseed・遠いseedの両方を作るための多点BFS。
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
            int nx = x + ORTHOGONAL_DX[dir];
            int ny = y + ORTHOGONAL_DY[dir];
            if (!inside(nx, ny, n, n)) continue;
            if (obstacle_distance[nx][ny] <= obstacle_distance[x][y] + 1) {
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
    for (int feature = 0;
         feature < SEED_FEATURE_COUNT &&
         (int)seeds.size() < case_placement_config.connected_growth_seed_limit;
         feature++) {
        for (int order_index = 0;
             order_index < (int)component_order.size() &&
             (int)seeds.size() < case_placement_config.connected_growth_seed_limit;
             order_index++) {
            int component_id = component_order[order_index];
            Cell seed = feature_seeds[component_id][feature];
            if (used_seeds.insert(seed).second) {
                seeds.push_back({seed, feature % 4});
            }
        }
    }

    // 隣接済みセルが多いほど優先し、同点では障害物距離、seedからの向き、座標で固定する。
    // frontierへ同じセルが複数回入るため、pop時に隣接数を再計算する。
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
        region.reserve(p + case_placement_config.grow_and_trim_extra_cells);
        priority_queue<GrowthEntry, vector<GrowthEntry>, decltype(entry_worse)> frontier(entry_worse);

        auto count_selected_neighbors = [&](int cell) {
            int x = cell / n;
            int y = cell % n;
            int count = 0;
            for (int dir = 0; dir < 4; dir++) {
                int nx = x + ORTHOGONAL_DX[dir];
                int ny = y + ORTHOGONAL_DY[dir];
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
                push_frontier(x + ORTHOGONAL_DX[dir], y + ORTHOGONAL_DY[dir]);
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
                // 元のpセル候補も必ず残し、trim版は置換ではなく追加候補にする。
                // これによりgrow-and-trimが悪い場合は従来形状へ戻れる。
                add_candidate(optional<vector<Cell>>(region));
                if (grow_and_trim_attempts < case_placement_config.grow_and_trim_candidate_limit) {
                    grow_and_trim_attempts++;
                    diagnostics.grow_and_trim_base_candidates++;
                    int base_perimeter = calc_perimeter(region, n);
                    grow_until(p + case_placement_config.grow_and_trim_extra_cells);
                    diagnostics.grow_and_trim_grown_cells += region.size() - p;
                    if ((int)region.size() != p + case_placement_config.grow_and_trim_extra_cells) {
                        diagnostics.grow_and_trim_growth_failures++;
                    } else {
                        diagnostics.grow_and_trim_full_growths++;
                        optional<vector<Cell>> trimmed = trim_connected_region(
                            park, owner, region, p,
                            diagnostics.grow_and_trim_trimmed_cells);
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

long double compact_fit_utility(const vs &park, const vvi &owner,
                                const vector<GroupState> &groups,
                                const vector<char> &in_candidate,
                                ll snapshot_time) {
    // DPの各値は「そのセルを右下端とする最大空き正方形の一辺」。
    // 2,3,4,...,12四方を置ける位置数を数え、大きい正方形ほどside^2で重く評価する。
    // snapshot_timeまでに退去する既存組は空きとみなし、今回の候補領域は占有扱いにする。
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
    // 今回の滞在中に始まる未来組の確率質量を、1/6・3/6・5/6分位で3時点に切る。
    // 等間隔の時刻ではなく到着分布の分位点を使い、未来到着が多い期間を細かく見る。
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
    // 通常expertは最悪snapshotを25%、高外周率placement expertは50%混ぜる。
    // いずれも平均だけで途中の空き形状崩壊を見落とさないための固定比率である。
    long double average = sum / FUTURE_FIT_SNAPSHOT_COUNT;
    long double minimum_weight =
        (long double)case_placement_config.future_fit_min_weight_milli /
        DLP_SCALE_DENOMINATOR;
    return (1.0L - minimum_weight) * average + minimum_weight * minimum;
}

optional<NormalPlacementChoice> choose_temporally_coherent_region(const vs &park, const vvi &owner,
                                                                  const vector<GroupState> &groups, ll current_s,
                                                                  ll arrival_t, int p, ll arrival_v,
                                                                  long double opportunity_cost, long double theta,
                                                                  int remaining_groups, const vector<Shape> &shapes,
                                                                  TemporalPlacementDiagnostics &diagnostics,
                                                                  vector<NormalPlacementChoice> *root_alternatives = nullptr) {
    // 通常配置の選択手順:
    // 1. 置ける最小周長テンプレートを全走査する。
    // 2. なければ次の周長tier、connected growth、grow-and-trimを追加する。
    // 3. 退去時刻の近い領域が接するように、境界コストで最大6または8候補へ絞る。
    // 4. 未来3時点に残る空き正方形を評価して最終候補を選ぶ。
    // 5. 実ターンで旧方策がconnected系かつ経済的Acceptなら、near-template deformationを追加する。
    //    synthetic rolloutでは再帰的な探索量増加を避け、従来の軽量方策を保つ。
    // 6. 実ターンではrollout比較用に通常配置の次点も最大2件返す。
    if (root_alternatives) root_alternatives->clear();
    diagnostics.attempts++;
    int n = park.size();
    LegalAnchorIndex legal_anchor_index(park, &owner);

    ConditionalFutureDemand future_demand(current_s, theta);
    long double candidate_arrival_level = future_demand.future_start_cdf(arrival_t);

    auto release_level = [&](ll release_time) {
        long double remaining = max(0LL, release_time - current_s);
        return -expm1l(-remaining / theta);
    };
    long double candidate_release_level = release_level(arrival_t);
    vector<long double> group_arrival_level(groups.size(), -1.0L);
    vector<long double> group_release_level(groups.size(), -1.0L);

    // incremental_cellは候補追加による境界コストの増分、absolute_cellは
    // 配置後に残る境界の不整合そのものを表す。後者もshortlistへ1件必ず残す。
    vector<vector<long double>> incremental_cell(n, vector<long double>(n));
    vector<vector<long double>> absolute_cell(n, vector<long double>(n));
    for (int x = 0; x < n; x++) {
        for (int y = 0; y < n; y++) {
            if (park[x][y] != '.' || owner[x][y] != -1) continue;
            for (int dir = 0; dir < 4; dir++) {
                int nx = x + ORTHOGONAL_DX[dir];
                int ny = y + ORTHOGONAL_DY[dir];
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
            // 診断上のanchor数は旧累積和版と同じ論理全数を数える。
            diagnostics.anchors_checked += max_y + 1;
            uint64_t legal_base_y = legal_anchor_index.legal_base_y_mask(shape, base_x);
            while (legal_base_y != 0) {
                int base_y = __builtin_ctzll(legal_base_y);
                legal_base_y &= legal_base_y - 1;
                const Rect &main_rect = shape.main_rect;
                const Rect &extra_rect = shape.extra_rect;
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
                                           [&] { return materialize_shape(shape, base_x, base_y); });
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
        // shapesは周長順。置ける周長tierが見つかった後のtierは必ず料金が悪いため調べない。
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
    bool future_fit_available =
        remaining_groups > 0 && arrival_t - current_s > 1 && future_mass > 1e-12L;
    optional<array<ll, FUTURE_FIT_SNAPSHOT_COUNT>> common_snapshots;
    vector<long double> future_fit_values(candidates.size());
    bool used_future_fit = false;
    if ((int)candidates.size() >= 2 && future_fit_available) {
        used_future_fit = true;
        common_snapshots = make_future_fit_snapshots(future_demand, current_s, arrival_t);
        long double best_fit = -numeric_limits<long double>::infinity();
        for (int index = 0; index < (int)candidates.size(); index++) {
            long double fit = evaluate_compact_fit(
                park, owner, groups, candidates[index].cells, *common_snapshots);
            future_fit_values[index] = fit;
            diagnostics.future_fit_snapshots += FUTURE_FIT_SNAPSHOT_COUNT;
            if (fit > best_fit + 1e-15L ||
                (fabsl(fit - best_fit) <= 1e-15L &&
                 placement_increment_less(candidates[index], candidates[best_index]))) {
                best_fit = fit;
                best_index = index;
            }
        }
        diagnostics.future_fit_evaluated_turns++;
    }

    // 旧connected配置が既に料金判定を通る場合だけ候補を追加する。
    // 開発100 seedでエラーの尾が出た高外周率盤面は初期地形で静的に除外する。
    // Reject→Acceptは起こさず、周長・丸め後料金・future-fitの保護を全て維持する。
    int old_best_index = best_index;
    optional<PlacementCandidate> polished_choice;
    bool old_connected = candidates[old_best_index].source == PlacementSource::ConnectedGrowth ||
                         candidates[old_best_index].source == PlacementSource::GrowAndTrim;
    ll old_fee = round_payment(arrival_v, p, candidates[old_best_index].perimeter);
    bool connected_polish_candidate =
        root_alternatives != nullptr && old_connected &&
        old_fee > opportunity_cost &&
        candidates[old_best_index].perimeter > minimum_perimeter;
    if (connected_polish_candidate) {
        diagnostics.connected_polish_candidate_turns++;
    }
    if (connected_polish_candidate && !case_connected_polish_enabled) {
        diagnostics.connected_polish_static_filtered_turns++;
    }
    if (connected_polish_candidate && case_connected_polish_enabled) {
        diagnostics.connected_polish_eligible_turns++;
        vector<vector<Cell>> dense_regions;
        // dense全盤面走査はP>=50かつ改善上限Uが1万以上の先着24回へ限定する。
        // 安いdescentは面積gateとこの予算から分離し、全eligibleで実行する。
        ll maximum_fee_gain =
            round_payment(arrival_v, p, minimum_perimeter) - old_fee;
        if (p < DENSE_BOX_MIN_GROUP_SIZE) {
            diagnostics.dense_box_size_filtered_turns++;
        } else if (maximum_fee_gain < DENSE_BOX_MIN_MAXIMUM_FEE_GAIN) {
            diagnostics.dense_box_value_filtered_turns++;
        } else if (case_dense_box_attempts >= DENSE_BOX_ATTEMPT_LIMIT_PER_CASE) {
            diagnostics.dense_box_budget_skips++;
        } else {
            case_dense_box_attempts++;
            diagnostics.dense_box_eligible_turns++;
            dense_regions = make_dense_box_trim_candidates(
                park, owner, p, minimum_perimeter,
                candidates[old_best_index].perimeter,
                incremental_cell, absolute_cell, incremental_prefix,
                absolute_prefix, diagnostics);
        }
        // dense/descentを周長tier別に圧縮し、短いtierから保護判定する。
        map<int, PlacementShortlistBuilder> polish_builders;
        auto consider_polished_region = [&](vector<Cell> &region, PlacementSource source) {
            int perimeter = calc_perimeter(region, n);
            ll fee = round_payment(arrival_v, p, perimeter);
            if (fee <= old_fee) {
                if (source == PlacementSource::DenseBoxTrim) {
                    diagnostics.dense_box_nonimproving_candidates++;
                } else if (source == PlacementSource::PerimeterDescent) {
                    diagnostics.perimeter_descent_nonimproving_candidates++;
                }
                return;
            }
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
            polish_builders[perimeter].consider(
                perimeter, incremental_cost, absolute_cost, order, quadrant,
                source, [&] { return region; });
        };

        // 旧領域を最小限だけ変形する候補を先に入れる。同周長・同costなら、
        // 盤面を大きく飛び移るdense案より局所swapを固定tie-breakで優先する。
        vector<vector<Cell>> descent_regions = make_perimeter_descent_candidates(
            park, owner, candidates[old_best_index].cells,
            incremental_cell, absolute_cell, diagnostics);
        for (vector<Cell> &region : descent_regions) {
            consider_polished_region(region, PlacementSource::PerimeterDescent);
        }
        for (vector<Cell> &region : dense_regions) {
            consider_polished_region(region, PlacementSource::DenseBoxTrim);
        }

        long double old_fit = -numeric_limits<long double>::infinity();
        if (future_fit_available && !polish_builders.empty()) {
            if (!common_snapshots) {
                common_snapshots =
                    make_future_fit_snapshots(future_demand, current_s, arrival_t);
            }
            if (used_future_fit) {
                old_fit = future_fit_values[old_best_index];
            } else {
                old_fit = evaluate_compact_fit(
                    park, owner, groups, candidates[old_best_index].cells,
                    *common_snapshots);
                diagnostics.future_fit_snapshots += FUTURE_FIT_SNAPSHOT_COUNT;
                diagnostics.future_fit_evaluated_turns++;
            }
        }

        // 最短tierが保護条件で落ちても、次tierの安全な改善を試す。
        for (const auto &[perimeter, builder] : polish_builders) {
            vector<PlacementCandidate> tier_candidates = builder.finalize();
            diagnostics.shortlisted_candidates += tier_candidates.size();
            if (tier_candidates.empty()) continue;
            int tier_best = -1;
            long double tier_best_fit = -numeric_limits<long double>::infinity();
            if (future_fit_available) {
                for (int index = 0; index < (int)tier_candidates.size(); index++) {
                    long double fit = evaluate_compact_fit(
                        park, owner, groups, tier_candidates[index].cells,
                        *common_snapshots);
                    diagnostics.future_fit_snapshots += FUTURE_FIT_SNAPSHOT_COUNT;
                    if (tier_best == -1 || fit > tier_best_fit + 1e-15L ||
                        (fabsl(fit - tier_best_fit) <= 1e-15L &&
                         placement_increment_less(tier_candidates[index],
                                                  tier_candidates[tier_best]))) {
                        tier_best_fit = fit;
                        tier_best = index;
                    }
                }
            } else {
                for (int index = 0; index < (int)tier_candidates.size(); index++) {
                    if (tier_best == -1 ||
                        placement_increment_less(tier_candidates[index],
                                                 tier_candidates[tier_best])) {
                        tier_best = index;
                    }
                }
            }
            if (tier_best == -1) continue;
            PlacementSource polished_source = tier_candidates[tier_best].source;
            if (!future_fit_available || tier_best_fit + 1e-15L >= old_fit) {
                int perimeter_improvement =
                    candidates[old_best_index].perimeter - perimeter;
                if (polished_source == PlacementSource::DenseBoxTrim) {
                    diagnostics.dense_box_perimeter_improvement += perimeter_improvement;
                } else if (polished_source == PlacementSource::PerimeterDescent) {
                    diagnostics.perimeter_descent_perimeter_improvement +=
                        perimeter_improvement;
                    if (p < DENSE_BOX_MIN_GROUP_SIZE) {
                        diagnostics
                            .small_group_perimeter_descent_perimeter_improvement +=
                            perimeter_improvement;
                        diagnostics.small_group_perimeter_descent_fee_gain +=
                            round_payment(arrival_v, p, perimeter) - old_fee;
                    }
                }
                polished_choice = std::move(tier_candidates[tier_best]);
                break;
            }
            if (polished_source == PlacementSource::DenseBoxTrim) {
                diagnostics.dense_box_future_fit_rejections++;
            } else if (polished_source == PlacementSource::PerimeterDescent) {
                diagnostics.perimeter_descent_future_fit_rejections++;
                if (p < DENSE_BOX_MIN_GROUP_SIZE) {
                    diagnostics
                        .small_group_perimeter_descent_future_fit_rejections++;
                }
            }
        }
    }

    if (!same_region(candidates[incremental_best].cells,
                     candidates[old_best_index].cells)) {
        diagnostics.future_fit_changed_placements++;
    }
    if (polished_choice &&
        !same_region(candidates[old_best_index].cells, polished_choice->cells)) {
        diagnostics.connected_polish_changed_placements++;
    }
    const PlacementCandidate &final_view =
        polished_choice ? *polished_choice : candidates[old_best_index];
    if (!same_region(final_view.cells, candidates[absolute_best].cells)) {
        diagnostics.final_changed_from_absolute++;
    }

    // 実ターンだけはrollout比較用に旧primaryを含む次点を最大2件返す。
    // polish案へ替えた場合も旧connected配置を第1rollback候補として必ず保持する。
    if (root_alternatives) {
        vector<char> chosen(candidates.size(), false);
        chosen[old_best_index] = true;
        if (polished_choice) {
            root_alternatives->push_back(
                NormalPlacementChoice{candidates[old_best_index].cells,
                                      candidates[old_best_index].perimeter,
                                      candidates[old_best_index].source});
        }
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
                NormalPlacementChoice{candidates[alternative_index].cells,
                                      candidates[alternative_index].perimeter,
                                      candidates[alternative_index].source});
        }
    }
    PlacementCandidate choice = polished_choice ? std::move(*polished_choice)
                                                : std::move(candidates[old_best_index]);
    if (choice.source == PlacementSource::ConnectedGrowth ||
        choice.source == PlacementSource::GrowAndTrim ||
        choice.source == PlacementSource::DenseBoxTrim ||
        choice.source == PlacementSource::PerimeterDescent) {
        diagnostics.fallback_successes++;
        if (choice.source == PlacementSource::GrowAndTrim) diagnostics.grow_and_trim_successes++;
        if (choice.source == PlacementSource::DenseBoxTrim) diagnostics.dense_box_successes++;
        if (choice.source == PlacementSource::PerimeterDescent) {
            diagnostics.perimeter_descent_successes++;
            if (p < DENSE_BOX_MIN_GROUP_SIZE) {
                diagnostics.small_group_perimeter_descent_successes++;
            }
        }
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
    // expert倍率適用前のShadowEvaluationを集計する。
    long double opportunity_cost_sum = 0.0L;
    long double rejected_fraction_sum = 0.0L;
    long double maximum_rejected_fraction = 0.0L;
    long long priced_buckets = 0;
};

enum class ArrivalStatus {
    UpperBoundRejected,  // 最小周長でも料金が機会損失以下
    NoRegion,            // 空き総面積とは別に、連結なpセルを確保できない
    ActualFeeRejected,   // 配置は可能だが、実際の周長での料金が機会損失以下
    Accepted,            // 料金比較と配置可能性の両方を通過
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
    total.connected_polish_changed_placements +=
        part.connected_polish_changed_placements;
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
    total.connected_polish_candidate_turns +=
        part.connected_polish_candidate_turns;
    total.connected_polish_static_filtered_turns +=
        part.connected_polish_static_filtered_turns;
    total.connected_polish_eligible_turns +=
        part.connected_polish_eligible_turns;
    total.dense_box_size_filtered_turns +=
        part.dense_box_size_filtered_turns;
    total.dense_box_eligible_turns += part.dense_box_eligible_turns;
    total.dense_box_value_filtered_turns += part.dense_box_value_filtered_turns;
    total.dense_box_budget_skips += part.dense_box_budget_skips;
    total.dense_box_anchors_checked += part.dense_box_anchors_checked;
    total.dense_box_feasible_anchors += part.dense_box_feasible_anchors;
    total.dense_box_shortlisted_anchors += part.dense_box_shortlisted_anchors;
    total.dense_box_component_failures += part.dense_box_component_failures;
    total.dense_box_trim_attempts += part.dense_box_trim_attempts;
    total.dense_box_trimmed_cells += part.dense_box_trimmed_cells;
    total.dense_box_trim_failures += part.dense_box_trim_failures;
    total.dense_box_nonimproving_candidates += part.dense_box_nonimproving_candidates;
    total.dense_box_duplicate_candidates += part.dense_box_duplicate_candidates;
    total.dense_box_candidates += part.dense_box_candidates;
    total.dense_box_future_fit_rejections += part.dense_box_future_fit_rejections;
    total.dense_box_perimeter_improvement += part.dense_box_perimeter_improvement;
    total.dense_box_successes += part.dense_box_successes;
    total.perimeter_descent_attempts += part.perimeter_descent_attempts;
    total.perimeter_descent_prefilter_rejections +=
        part.perimeter_descent_prefilter_rejections;
    total.perimeter_descent_remove_prefilter_rejections +=
        part.perimeter_descent_remove_prefilter_rejections;
    total.perimeter_descent_steps += part.perimeter_descent_steps;
    total.perimeter_descent_candidates += part.perimeter_descent_candidates;
    total.small_group_perimeter_descent_attempts +=
        part.small_group_perimeter_descent_attempts;
    total.small_group_perimeter_descent_steps +=
        part.small_group_perimeter_descent_steps;
    total.small_group_perimeter_descent_candidates +=
        part.small_group_perimeter_descent_candidates;
    total.small_group_perimeter_descent_future_fit_rejections +=
        part.small_group_perimeter_descent_future_fit_rejections;
    total.small_group_perimeter_descent_perimeter_improvement +=
        part.small_group_perimeter_descent_perimeter_improvement;
    total.small_group_perimeter_descent_fee_gain +=
        part.small_group_perimeter_descent_fee_gain;
    total.small_group_perimeter_descent_successes +=
        part.small_group_perimeter_descent_successes;
    total.perimeter_descent_nonimproving_candidates +=
        part.perimeter_descent_nonimproving_candidates;
    total.perimeter_descent_future_fit_rejections +=
        part.perimeter_descent_future_fit_rejections;
    total.perimeter_descent_perimeter_improvement +=
        part.perimeter_descent_perimeter_improvement;
    total.perimeter_descent_successes += part.perimeter_descent_successes;
    total.shortlisted_candidates += part.shortlisted_candidates;
    total.future_fit_snapshots += part.future_fit_snapshots;
}

void remove_selected_placement_success(TemporalPlacementDiagnostics &diagnostics) {
    if (diagnostics.fallback_successes > 0) {
        diagnostics.fallback_successes--;
        if (diagnostics.dense_box_successes > 0) {
            diagnostics.dense_box_successes--;
        } else if (diagnostics.perimeter_descent_successes > 0) {
            diagnostics.perimeter_descent_successes--;
            if (diagnostics.small_group_perimeter_descent_successes > 0) {
                diagnostics.small_group_perimeter_descent_successes--;
                diagnostics
                    .small_group_perimeter_descent_perimeter_improvement = 0;
                diagnostics.small_group_perimeter_descent_fee_gain = 0;
            }
        } else if (diagnostics.grow_and_trim_successes > 0) {
            diagnostics.grow_and_trim_successes--;
        }
        return;
    }
    if (diagnostics.compact_successes > 0) diagnostics.compact_successes--;
    if (diagnostics.extended_template_successes > 0) diagnostics.extended_template_successes--;
}

void replace_selected_placement_success(TemporalPlacementDiagnostics &diagnostics,
                                        PlacementSource source) {
    remove_selected_placement_success(diagnostics);
    if (source == PlacementSource::ConnectedGrowth || source == PlacementSource::GrowAndTrim ||
        source == PlacementSource::DenseBoxTrim || source == PlacementSource::PerimeterDescent) {
        diagnostics.fallback_successes++;
        if (source == PlacementSource::GrowAndTrim) diagnostics.grow_and_trim_successes++;
        if (source == PlacementSource::DenseBoxTrim) diagnostics.dense_box_successes++;
        if (source == PlacementSource::PerimeterDescent) diagnostics.perimeter_descent_successes++;
    } else {
        diagnostics.compact_successes++;
        if (source == PlacementSource::ExtendedTemplate) diagnostics.extended_template_successes++;
    }
}

// polish採用でbaseline周長が短くなっても、旧connectedが持っていたroot探索機会を
// 消さないためのrollback view。polish時は旧primaryをalternatives先頭へ必ず入れる。
// 探索発火と移動先rankingだけ旧形状を参照し、direct gainは改善後baselineと比較する。
const NormalPlacementChoice *connected_polish_rollback(
    const ArrivalDecision &decision,
    const vector<NormalPlacementChoice> &normal_alternatives) {
    bool polished = decision.diagnostics.dense_box_successes == 1 ||
                    decision.diagnostics.perimeter_descent_successes == 1;
    if (!polished || normal_alternatives.empty()) return nullptr;
    const NormalPlacementChoice &rollback = normal_alternatives.front();
    if (rollback.source != PlacementSource::ConnectedGrowth &&
        rollback.source != PlacementSource::GrowAndTrim) {
        return nullptr;
    }
    return &rollback;
}

// 到着組の通常入場判定。高価な配置探索の前後で料金と機会損失を比較する。
// 1. 最小周長でも料金<=機会損失なら、どう置いても得しないため即拒否。
// 2. 連結なpセルを作れなければNoRegion。
// 3. 実際に選んだ周長での料金<=機会損失なら拒否。
// 3段階を全て通過した場合だけAcceptedとする。
ArrivalDecision evaluate_arrival_decision(const vs &park, const vvi &decision_owner,
                                          const vector<GroupState> &groups, int arrival_id, ll current_s,
                                          int remaining_groups, long double theta, long double opportunity_cost,
                                          const vector<vector<Shape>> &compact_shapes,
                                          vector<NormalPlacementChoice> *root_alternatives = nullptr) {
    ArrivalDecision result;
    // 実到着と仮想未来で同じ倍率を掛け、rollout内だけ価格尺度がずれることを防ぐ。
    opportunity_cost *=
        (long double)case_dlp_scale_milli / DLP_SCALE_DENOMINATOR;
    if (root_alternatives) root_alternatives->clear();
    const GroupState &arrival = groups[arrival_id];
    int minimum_perimeter = compact_shapes[arrival.p].front().perimeter;
    ll upper_bound_fee = round_payment(arrival.v, arrival.p, minimum_perimeter);
    if ((long double)upper_bound_fee <= opportunity_cost) {
        result.status = ArrivalStatus::UpperBoundRejected;
        return result;
    }

    optional<NormalPlacementChoice> placement =
        choose_temporally_coherent_region(
            park, decision_owner, groups, current_s, arrival.t,
            arrival.p, arrival.v,
            opportunity_cost, theta, remaining_groups, compact_shapes[arrival.p],
            result.diagnostics, root_alternatives);
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

// スコア悪化の原因を分解するための診断値。
// 「全到着を衝突なし・最小周長で受け入れる」という実現不能な上界を基準にすると、
// 実スコアとの差を、拒否した価値・形状損・再配置後の料金損・移動費へ厳密に分解できる。
// 意思決定には使わず、最終stderrへ出すだけである。
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
    // 最小テンプレート、拡張テンプレート、連結成長、分類不能の順。
    array<int, 4> accepted_by_source{};
    int accepted_grow_and_trim = 0;
    int accepted_dense_box_trim = 0;
    int accepted_perimeter_descent = 0;

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
    ll accepted_dense_box_trim_ideal_fee = 0;
    ll accepted_dense_box_trim_initial_fee = 0;
    ll accepted_dense_box_trim_perimeter_excess = 0;
    ll accepted_perimeter_descent_ideal_fee = 0;
    ll accepted_perimeter_descent_initial_fee = 0;
    ll accepted_perimeter_descent_perimeter_excess = 0;
    ll accepted_free_cells_sum = 0;
    ll rejected_feasible_free_cells_sum = 0;
    ll rejected_unplaceable_free_cells_sum = 0;

    // 以下4項目もexpert倍率適用前の生値で、case間のDLP出力比較に使う。
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

        int fallback_detail_sources =
            decision.diagnostics.grow_and_trim_successes +
            decision.diagnostics.dense_box_successes +
            decision.diagnostics.perimeter_descent_successes;
        int source = 3;
        if (decision.diagnostics.compact_successes == 1 &&
            decision.diagnostics.extended_template_successes == 0 &&
            decision.diagnostics.fallback_successes == 0 &&
            fallback_detail_sources == 0) {
            source = 0;
        } else if (decision.diagnostics.compact_successes == 1 &&
                   decision.diagnostics.extended_template_successes == 1 &&
                   decision.diagnostics.fallback_successes == 0 &&
                   fallback_detail_sources == 0) {
            source = 1;
        } else if (decision.diagnostics.compact_successes == 0 &&
                   decision.diagnostics.extended_template_successes == 0 &&
                   decision.diagnostics.fallback_successes == 1 &&
                   fallback_detail_sources <= 1) {
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
        if (decision.diagnostics.dense_box_successes == 1) {
            diagnostics.accepted_dense_box_trim++;
            diagnostics.accepted_dense_box_trim_ideal_fee += ideal_fee;
            diagnostics.accepted_dense_box_trim_initial_fee += initial_fee;
            diagnostics.accepted_dense_box_trim_perimeter_excess +=
                plan.arrival_perimeter - minimum_perimeter;
        }
        if (decision.diagnostics.perimeter_descent_successes == 1) {
            diagnostics.accepted_perimeter_descent++;
            diagnostics.accepted_perimeter_descent_ideal_fee += ideal_fee;
            diagnostics.accepted_perimeter_descent_initial_fee += initial_fee;
            diagnostics.accepted_perimeter_descent_perimeter_excess +=
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

// 再配置探索中の領域を最大50×50bitで表す。重複判定をword単位で行える。
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

// ---------- 再配置探索の診断値 ----------
// Compact rescueとNoRegion Push-outは探索本体を共有するため、共通値と個別値をまとめて持つ。
// いずれも最終stderr用で、候補の採否条件には使わない。
struct RescueDiagnostics {
    // rescueを検討・構築・採用できたターン数。
    int eligible_fallbacks = 0;
    int feasible_turns = 0;
    int feasible_plans = 0;
    int successes = 0;

    // 2シナリオscreenの結果と、最大2候補の比較状況。
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

    // rescueのscreenへ通常配置次点も同席させた拡張root比較。
    int root_alternative_available_turns = 0;
    int root_selected_primary = 0;
    int root_selected_alternative = 0;
    int root_alternative_disagreements = 0;
    int root_screen_v3_overrides = 0;
    int root_screen_selected_alternative = 0;
    int root_v3_winner_overridden = 0;
    array<int, ROOT_ROLLOUT_NORMAL_ALTERNATIVE_LIMIT> root_selected_alternative_rank{};
    array<int, ROOT_ROLLOUT_MAX_ACTIONS + 1> root_turns_by_action_count{};

    // rescueが発生しないターンで、通常配置次点だけをbaselineと比較した結果。
    int normal_root_gate_turns = 0;
    int normal_root_rollout_turns = 0;
    int normal_root_generation_failures = 0;
    int normal_root_screen_overrides = 0;
    int normal_root_screen_selected_alternative = 0;
    int normal_root_selected_primary = 0;
    int normal_root_selected_alternative = 0;
    array<int, ROOT_ROLLOUT_NORMAL_ALTERNATIVE_LIMIT> normal_root_selected_alternative_rank{};
    array<int, 1 + ROOT_ROLLOUT_NORMAL_ALTERNATIVE_LIMIT + 1> normal_root_turns_by_action_count{};

    // screen勝者を独立8シナリオで再確認するholdout。
    int root_confirmation_attempts = 0;
    int root_confirmation_approved = 0;
    int root_confirmation_rejected = 0;
    int root_confirmation_generation_failures = 0;
    int root_confirmation_budget_skips = 0;
    int root_confirmation_full_horizon = 0;
    int root_confirmation_short_horizon = 0;
    int root_confirmation_pair_disagreements = 0;

    // 候補生成・移動先修復・最終検証の失敗理由。
    int no_economic_target = 0;
    int no_repair = 0;
    int target_limit_exhausted = 0;
    int destination_limit_exhausted = 0;
    int node_limit_exhausted = 0;
    int validation_failures = 0;
    int maximum_blockers = 0;
    array<int, 4> feasible_by_blocker_count{};
    array<int, 4> successes_by_blocker_count{};

    // 探索量。速度低下が候補走査・移動先列挙・beam・rolloutのどこ由来かを分解する。
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

    // 現在ターンの料金・移動費と、2本のscreen未来で生じた料金差。
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

    // 採用/不採用別の予測marginと、screenからholdoutまでの改善量。
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

    // Compact rescueは「受入可能な通常案 vs 再配置案」、Push-outは「拒否 vs 再配置案」
    // の比較で意味が異なるため、同じ探索を使っても集計は分離する。
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

};

// 共通rescue探索が増やしたカウンタ差分を、Push-out専用カウンタへ転記するRAII。
// early returnが多い関数でも、scopeを抜ければ必ず正しい差分とCPU時間を記録できる。
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

// 全アンカーを累積和だけで安価に順位付けするための仮候補。
// ここでは衝突セル数と按分移動費だけを持ち、shortlist後に正確なblocker集合へ展開する。
struct RescueTargetSeed {
    int shape_index = -1;
    int base_x = 0;
    int base_y = 0;
    int occupied_cells = 0;
    long double fractional_move_cost = 0.0L;
    long long order = 0;
};

// 到着組を最小周長で置く目標領域と、そのために退かす必要がある既存組。
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
    // 全最小周長アンカーの池合法性を行bit maskでまとめて判定し、
    // 合法anchorだけをO(1)の累積和で評価する。
    // 「衝突セル数が少ない」「セル按分した概算移動費が小さい」の2基準でtop-kを取り、
    // その和集合についてだけ正確なblocker集合・移動費・直接利益を計算する。
    int n = park.size();
    const GroupState &arrival = groups[arrival_id];
    const vector<Shape> &shapes = compact_shapes[arrival.p];
    int minimum_perimeter = shapes.front().perimeter;
    ll compact_fee = round_payment(arrival.v, arrival.p, minimum_perimeter);

    vector<vi> occupied_prefix(n + 1, vi(n + 1));
    vector<vector<long double>> fractional_prefix(n + 1, vector<long double>(n + 1));
    for (int x = 0; x < n; x++) {
        for (int y = 0; y < n; y++) {
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
    LegalAnchorIndex pond_anchor_index(park, nullptr);

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
            int base_y_count = n - shape.w + 1;
            diagnostics.target_anchors += base_y_count;
            uint64_t legal_base_y = pond_anchor_index.legal_base_y_mask(shape, base_x);
            while (legal_base_y != 0) {
                int base_y = __builtin_ctzll(legal_base_y);
                legal_base_y &= legal_base_y - 1;
                const Rect &a = shape.main_rect;
                const Rect &b = shape.extra_rect;
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

    // 全アンカーを保存せず、2本の固定長heapだけを持ってメモリを抑える。
    // 最後のtie-breakは一意な列挙順なので、2基準のtop-k和集合を正確に再現できる。
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
            materialize_shape(shapes[seed.shape_index], seed.base_x, seed.base_y);
        vector<int> blockers;
        stamp++;
        for (auto [x, y] : cells) {
            int id = owner[x][y];
            if (id != -1 && seen[id] != stamp) {
                seen[id] = stamp;
                blockers.push_back(id);
            }
        }
        // blocker 0の最小周長領域は通常配置ですでに発見されるため、再配置候補ではない。
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

// 既存組1つの移動先候補。
// maskは他の移動先との重複判定に使う。fallback_overlapはCompact rescueなら元の到着領域、
// Push-outなら再配置前から空いていた領域との重なりで、cleared_overlapは今回blockerを
// 撤去して初めて空いた領域の再利用量、temporal_costは退去時刻境界の悪さを表す。
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

// 移動先領域の外周について、隣接組との「現在から退去まで残る確率」の差を合計する。
// 退去時刻が近い組を隣接させると同時期に大きな空きが生まれやすいため、この値を小さくする。
// cell_maskで候補内部の辺は除き、空きセルの残存確率は0として扱う。
long double rescue_destination_temporal_cost(const vector<Cell> &cells, const BoardMask &cell_mask,
                                             const vs &park, const vvi &base_owner,
                                             const vector<GroupState> &groups, int mover_id,
                                             ll current_s, long double theta) {
    int n = park.size();
    auto level = [&](int id) {
        long double remaining = max(0LL, groups[id].t - current_s);
        return -expm1l(-remaining / theta);
    };
    long double mover_level = level(mover_id);
    long double result = 0.0L;
    for (auto [x, y] : cells) {
        for (int dir = 0; dir < 4; dir++) {
            int nx = x + ORTHOGONAL_DX[dir];
            int ny = y + ORTHOGONAL_DY[dir];
            if (!inside(nx, ny, n, n) || park[nx][ny] == '#') continue;
            int index = nx * n + ny;
            if ((cell_mask[index >> 6] >> (index & 63)) & 1ULL) continue;
            int neighbor = base_owner[nx][ny];
            long double neighbor_level = neighbor == -1 ? 0.0L : level(neighbor);
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
    RescueDiagnostics &diagnostics) {
    // 1つのblockerについて移動先候補を作る。
    // 「現在までに確定した料金」を悪化させない周長の形だけを許し、
    // 到着組の通常配置との重なり、今回空ける領域の再利用、退去時刻境界で順位付けする。
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

    // 各shapeのアンカーを互いに素なstrideで巡回し、特定shapeだけで予算を使い切らない。
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
            if (rectangle_sum(blocked_prefix, base_x + a.x, base_y + a.y, a.h, a.w) != 0 ||
                rectangle_sum(blocked_prefix, base_x + b.x, base_y + b.y, b.h, b.w) != 0) {
                continue;
            }

            vector<Cell> cells = materialize_shape(shape, base_x, base_y);
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
                cells, mask, park, base_owner, groups, mover_id, current_s, theta);
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
                          [&](const RescueDestination &candidate) {
                              return candidate.quadrant == quadrant;
                          });
        if (it != legal.end()) add(*it);
    }
    for (const RescueDestination &candidate : legal) {
        if ((int)result.size() == destination_limit) break;
        add(candidate);
    }
    if ((int)result.size() > destination_limit) result.resize(destination_limit);
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
    // 複数blockerの移動先を、互いに重ならないよう同時に選ぶ。
    // 候補数・面積・退去時刻で作った複数の挿入順についてgreedyを先に試し、
    // greedyが衝突した場合だけbeam searchで組合せを修復する。
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
    // beam予算を使う前に、全ての決定的な挿入順でgreedyを試す。
    // ある順序のbeamが予算切れでも、別順序で簡単に置ける解を見逃さないためである。
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

    // beamはgreedyの衝突を修復する補助であり、深いblocker数を最初から制限しない。
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
                    child.rank += 1000.0L * candidate.fallback_overlap +
                                  10.0L * candidate.cleared_overlap -
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

// rescue探索が作った計画を、探索時の近似値に依存せず最初から検証する。
// 移動元を全て消してから、各移動先と到着領域について面積・連結性・池・重複・周長を確認し、
// 既存組の料金損と実際の移動費も再計算する。ここを通った計画だけをrollout候補にする。
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

// rollout内だけで使う仮想到着。実入力のGroupStateとは分け、残り組数と再推定thetaも保持する。
struct RescueSyntheticArrival {
    ll s = 0;
    ll t = 0;
    int p = 0;
    ll v = 0;
    long double theta = 0.0L;
    int remaining_after = 0;
};

// 低食い違い列の1次元成分。base 2/3/5/7を滞在時間・開始時刻・面積・価値へ割り当てる。
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

// 同じ実ターンなら常に同じシナリオを作り、異なるターンでは列の開始位置をずらすためのhash。
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

// 現在までの観測だけから、root action間で共有する未来到着列を決定的に作る。
// screenではthetaの点推定を使う反対変数2シナリオ、confirmationではthetaの事後分位点を使う
// 反対変数8シナリオを生成する。batchを分けることで両者のサンプル列は重ならない。
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
        // ここで選ぶ潜在thetaは未来入力の生成だけに使う。
        // 仮想オンライン方策が見るthetaは、生成した到着を1件ずつ観測した体で下で再推定する。
        long double generation_theta = theta;
        if (posterior_predictive) {
            long double pair_quantile = (2.0L * pair_index + 1.0L) / scenario_count;
            generation_theta =
                theta_estimator.posterior_quantile(current_s, remaining_groups, pair_quantile);
        }
        ConditionalFutureDemand future_demand(current_s, generation_theta);
        // screenの第1ペアはsequence block 0を使う。
        // confirmationはbatch 1の互いに素な4区間を、反対変数の各組へ1区間ずつ割り当てる。
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
        // 必要なのは「残り全組を開始時刻で並べた先頭」である。
        // 最初からrollout長だけ生成すると、遅い開始時刻の組を先頭と誤認して偏るため、
        // いったん残り全組を生成・整列してから必要なprefixだけを取り出す。
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

// 1つのroot actionを適用した直後から始まる、仮想未来専用の盤面状態。
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

// 1本の仮想到着列を、指定されたroot action後の盤面から通常方策だけで進める。
// 未来ターンでは再配置探索を再帰的に呼ばず、受け入れた組の料金合計だけをbranch間で比較する。
RescueRolloutOutcome evaluate_rescue_rollout_branch(
    const vs &park, const vvi &final_owner, const vector<GroupState> &groups, int arrival_id,
    const TurnPlan &plan, const vector<RescueSyntheticArrival> &scenario, int grass_cells,
    const DensityModel &density_model, SampledDlpShadowModel &sampled_dlp_model,
    const vector<vector<Shape>> &compact_shapes) {
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
            // 全branchで実ターン時点のDLP価格を凍結して共有する。
            // 仮想入力を見てDLPを解き直すと、実方策にはない未来情報をbranchへ与えてしまう。
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

// confirmationへ渡す行動branchの軽量view。盤面はすでにその行動を適用済みで、
// direct_vs_baselineには現在ターンだけのbaseline比を入れる。
struct RootBranchView {
    const TurnPlan *plan = nullptr;
    const vvi *final_owner = nullptr;
    ll direct_vs_baseline = 0;
};

// 2シナリオの安いscreenで従来案を上回った挑戦案を、独立な8シナリオで再確認する。
// 「現在ターンの直接差 + 未来12到着の料金差」の平均が正のときだけ上書きを許す。
// ケース全体で最大4回に制限し、偶然screenへ適合した配置を採用しにくくする。
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
        positive_scenario[scenario_index] = root_scenario_positive(direct_delta, future_delta);
        diagnostics.root_confirmation_positive_scenarios += positive_scenario[scenario_index];
    }
    diagnostics.root_confirmation_scenarios += ROOT_CONFIRM_SCENARIO_COUNT;
    for (int pair = 0; pair < ROOT_CONFIRM_SCENARIO_COUNT / 2; pair++) {
        diagnostics.root_confirmation_pair_disagreements +=
            positive_scenario[2 * pair] != positive_scenario[2 * pair + 1];
    }

    i128 margin_times_scenarios =
        root_margin_scaled(ROOT_CONFIRM_SCENARIO_COUNT, direct_delta, future_delta_sum);
    long double holdout_margin =
        root_margin_fee(margin_times_scenarios, ROOT_CONFIRM_SCENARIO_COUNT);
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

// 拡張root探索が採用した、出力計画とその到着判定の組。
struct RootActionResult {
    TurnPlan plan;
    ArrivalDecision arrival_decision;
};

// 現在ターンで比較する行動の種類。
// Baselineは通常方策、Rescueは既存組を動かす案、NormalAlternativeは通常配置の次点案。
enum class RootActionKind {
    Baseline,
    Rescue,
    NormalAlternative,
};

enum class RescueMode {
    // 通常案でも受入可能だが周長が悪い到着を、再配置により最小周長へ整える。
    CompactAccepted,
    // 空き面積は足りるのに断片化で置けない到着を、既存組を押し出して受け入れる。
    NoRegionPushOut,
};

// 合法性を再検証済みのrescue案。direct_gainは通常案との差で、未来価値はまだ含まない。
struct PreparedRescueCandidate {
    TurnPlan plan;
    vvi final_owner;
    vector<int> blockers;
    ll compact_fee = 0;
    ll direct_gain = 0;
    ll movement_cost = 0;
};

// Compact rescue / NoRegion Push-outを共通の探索器で作り、通常案と比較する中心関数。
// 1. 最小周長テンプレートのうち、退かす既存組が少なく経済的な目標領域を絞る。
// 2. blockerを一度全て消し、互いに重ならない移動先をgreedy + beamで組み合わせる。
// 3. 完成計画を独立に合法性検証し、現在ターンだけでも通常案より得なものを最大2案残す。
// 4. 共通の仮想到着列で通常案・rescue案・通常配置次点案をscreenする。
// 5. baselineとrescueだけのscreen勝者を通常配置次点が上回った場合だけ、独立holdoutで再確認する。
// 採用できる案がなければnulloptを返し、呼び出し元は通常案をそのまま使う。
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
    // Push-outの即時利益gateにも通常admissionと同じケース固定倍率を適用する。
    opportunity_cost *=
        (long double)case_dlp_scale_milli / DLP_SCALE_DENOMINATOR;
    const GroupState &arrival = groups[arrival_id];
    int minimum_perimeter = compact_shapes[arrival.p].front().perimeter;
    const NormalPlacementChoice *polish_rollback =
        connected_polish_rollback(baseline, normal_alternatives);
    int compact_rescue_trigger_perimeter = baseline.perimeter;
    if (polish_rollback) {
        chmax(compact_rescue_trigger_perimeter, polish_rollback->perimeter);
    }
    bool compact_rescue = baseline.status == ArrivalStatus::Accepted && baseline.cells &&
                          compact_rescue_trigger_perimeter >
                              minimum_perimeter + COMPACT_PERIMETER_MARGIN;
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
        // 再配置では占有セル総数が変わらないため、純粋な面積不足は直せない。
        // Push-outは「総空き面積は足りるが連結領域がない」断片化だけを対象にする。
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
        // Push-outでは必ず1組以上を動かす。最安の1回分の移動費を引いただけで
        // 入場shadowを下回るなら、正確な目標領域を走査しても採用可能な案は存在しない。
        if (minimum_move_cost == numeric_limits<ll>::max() ||
            (long double)(compact_fee - minimum_move_cost) <= opportunity_cost) {
            diagnostics.no_economic_target++;
            diagnostics.pushout_no_economic_target++;
            return nullopt;
        }
    }
    // Push-outにも通常入場と同じ全期間shadowを課す。
    // 後段の短い共通乱数rolloutは直近の盤面形状を見る追加の拒否判定であり、
    // shadowをもう一度料金から差し引くものではない。
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
    const vector<Cell> &preferred_destination_cells =
        mode == RescueMode::NoRegionPushOut
            ? preexisting_free_cells
            : (polish_rollback ? polish_rollback->cells : *baseline.cells);

    // shortlist順に目標領域を修復する。探索予算か候補2案のどちらかを使い切れば止める。
    for (const RescueTarget &target : targets) {
        if (attempted_targets == target_repair_limit || remaining_nodes == 0 ||
            (int)candidates.size() == RESCUE_ROLLOUT_CANDIDATE_LIMIT) {
            break;
        }
        attempted_targets++;
        diagnostics.repair_attempts++;
        chmax(diagnostics.maximum_blockers, (int)target.blockers.size());

        // 到着領域に衝突するblockerを全て一時撤去し、到着組を先に固定した盤面を作る。
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

        // blockerごとの移動先候補poolを作り、その直積から重ならない組合せを探す。
        vector<vector<RescueDestination>> pools;
        pools.reserve(target.blockers.size());
        bool missing_destination = false;
        for (int id : target.blockers) {
            vector<RescueDestination> pool = make_rescue_destinations(
                park, base_owner, groups, id, arrival_id, current_s, theta,
                preferred_destination_cells, cleared_mask,
                all_shapes, remaining_destination_anchors, destination_anchor_limit,
                destination_legal_limit, destination_limit, diagnostics);
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

        // 探索中の差分更新を信用せず、完成計画を元盤面から再構築して検算する。
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
                              compact_fee, direct_gain, target.movement_cost});

        if (candidates.size() == 1) {
            diagnostics.feasible_turns++;
            if (no_region_pushout) diagnostics.pushout_feasible_turns++;
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
                diagnostics.rollout_generation_failures++;
                if (no_region_pushout) {
                    // RejectからAcceptへ変えるPush-outは影響が大きい。
                    // 共通乱数比較を作れない場合は安全側に倒し、元のRejectを維持する。
                    diagnostics.pushout_rollout_generation_failures++;
                    diagnostics.pushout_screen_rejected++;
                    return nullopt;
                }
                // Compact rescueでは元々Acceptであり、rolloutは合法かつ直接得な再配置への追加filterである。
                // シナリオ生成だけが失敗した場合は、現在ターンで得な第1候補を残す。
                stop_after_primary = true;
                break;
            }
            rollout_ready = true;
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

        // 全行動を同じ2本の未来で評価するため、まず通常案の未来料金を基準値として計算する。
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
            ll scaled_margin = 0;
        };
        // scaled_marginは、1000倍した現在差と重み付き未来差の和。
        // 除算せず整数のまま比較し、同点順序と丸め誤差を固定する。
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
            evaluation.scaled_margin = root_margin_scaled_ll(
                RESCUE_ROLLOUT_SCENARIO_COUNT, candidate.direct_gain,
                evaluation.future_delta[0] + evaluation.future_delta[1]);
            diagnostics.rollout_slot_scenario_0_future_delta[candidate_index] +=
                evaluation.future_delta[0];
            diagnostics.rollout_slot_scenario_1_future_delta[candidate_index] +=
                evaluation.future_delta[1];
            diagnostics.rollout_slot_margin[candidate_index] +=
                root_margin_fee(evaluation.scaled_margin, RESCUE_ROLLOUT_SCENARIO_COUNT);
            bool first_accepts =
                root_scenario_positive(candidate.direct_gain, evaluation.future_delta[0]);
            bool second_accepts =
                root_scenario_positive(candidate.direct_gain, evaluation.future_delta[1]);
            if (first_accepts != second_accepts) {
                if (candidate_index == 0) {
                    diagnostics.rollout_candidate_0_disagreements++;
                } else {
                    diagnostics.rollout_candidate_1_disagreements++;
                }
            }
            if (evaluation.scaled_margin > 0) {
                diagnostics.rollout_positive_candidates++;
            } else {
                diagnostics.rollout_nonpositive_candidates++;
            }
        }

        int best_rescue = 0;
        for (int candidate_index = 1; candidate_index < (int)candidates.size(); candidate_index++) {
            if (evaluations[candidate_index].scaled_margin >
                evaluations[best_rescue].scaled_margin) {
                best_rescue = candidate_index;
            }
        }
        const PreparedRescueCandidate &best_candidate = candidates[best_rescue];
        const CandidateRolloutEvaluation &best_evaluation = evaluations[best_rescue];
        diagnostics.rollout_scenario_0_future_delta += best_evaluation.future_delta[0];
        diagnostics.rollout_scenario_1_future_delta += best_evaluation.future_delta[1];
        long double future_mean =
            0.5L * (best_evaluation.future_delta[0] + best_evaluation.future_delta[1]);
        long double rollout_margin =
            root_margin_fee(best_evaluation.scaled_margin, RESCUE_ROLLOUT_SCENARIO_COUNT);
        bool first_accepts =
            root_scenario_positive(best_candidate.direct_gain, best_evaluation.future_delta[0]);
        bool second_accepts =
            root_scenario_positive(best_candidate.direct_gain, best_evaluation.future_delta[1]);
        if (first_accepts != second_accepts) diagnostics.rollout_scenario_disagreements++;
        if (no_region_pushout) {
            diagnostics.pushout_scenario_0_future_delta += best_evaluation.future_delta[0];
            diagnostics.pushout_scenario_1_future_delta += best_evaluation.future_delta[1];
            diagnostics.pushout_screen_margin += rollout_margin;
        }

        if (candidates.size() == 2) {
            ll width_one_scaled_margin = max(0LL, evaluations[0].scaled_margin);
            ll width_two_scaled_margin =
                max(width_one_scaled_margin, evaluations[1].scaled_margin);
            diagnostics.rollout_width_predicted_gain += root_margin_fee(
                width_two_scaled_margin - width_one_scaled_margin,
                RESCUE_ROLLOUT_SCENARIO_COUNT);
        }

        ll best_root_scaled_margin = 0;
        selected_kind = RootActionKind::Baseline;
        // まずbaselineとrescue案だけでscreen勝者を決め、これを保護対象として確定する。
        // 新しく追加した通常配置次点は、安いscreenと独立な事後予測holdoutの両方で
        // この保護対象を上回った場合に限って採用する。
        for (int candidate_index = 0; candidate_index < (int)candidates.size(); candidate_index++) {
            if (evaluations[candidate_index].scaled_margin > best_root_scaled_margin) {
                best_root_scaled_margin = evaluations[candidate_index].scaled_margin;
                selected_kind = RootActionKind::Rescue;
                selected_candidate = candidate_index;
            }
        }
        RootActionKind protected_kind = selected_kind;
        int protected_candidate = selected_candidate;
        ll protected_scaled_margin = best_root_scaled_margin;

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
                alternative_evaluation.scaled_margin = root_margin_scaled_ll(
                    RESCUE_ROLLOUT_SCENARIO_COUNT, alternative_direct_gain,
                    alternative_evaluation.future_delta[0] + alternative_evaluation.future_delta[1]);
                diagnostics.root_alternative_direct_gain += alternative_direct_gain;
                diagnostics.root_alternative_scenario_0_future_delta +=
                    alternative_evaluation.future_delta[0];
                diagnostics.root_alternative_scenario_1_future_delta +=
                    alternative_evaluation.future_delta[1];
                long double alternative_future_mean =
                    0.5L * (alternative_evaluation.future_delta[0] + alternative_evaluation.future_delta[1]);
                diagnostics.root_alternative_future_mean += alternative_future_mean;
                diagnostics.root_alternative_margin += root_margin_fee(
                    alternative_evaluation.scaled_margin, RESCUE_ROLLOUT_SCENARIO_COUNT);
                bool alternative_first_accepts = root_scenario_positive(
                    alternative_direct_gain, alternative_evaluation.future_delta[0]);
                bool alternative_second_accepts = root_scenario_positive(
                    alternative_direct_gain, alternative_evaluation.future_delta[1]);
                if (alternative_first_accepts != alternative_second_accepts) {
                    diagnostics.root_alternative_disagreements++;
                }

                int alternative_index = alternative_plans.size();
                alternative_plans.push_back(std::move(alternative_plan));
                alternative_decisions.push_back(std::move(alternative_decision));
                alternative_owners.push_back(std::move(alternative_owner));
                if (alternative_evaluation.scaled_margin > best_root_scaled_margin) {
                    best_root_scaled_margin = alternative_evaluation.scaled_margin;
                    selected_kind = RootActionKind::NormalAlternative;
                    selected_alternative = alternative_index;
                }
            }

            long double screen_gain = root_margin_fee(
                best_root_scaled_margin - protected_scaled_margin,
                RESCUE_ROLLOUT_SCENARIO_COUNT);
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
            positive_count += evaluation.scaled_margin > 0;
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
                }
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
    selected.status = ArrivalStatus::Accepted;
    selected.cells = *chosen.plan.arrival;
    selected.perimeter = chosen.plan.arrival_perimeter;
    selected.fee = chosen.compact_fee;
    // 元の通常案が連結成長でもRejectでも、採用したrescue案の到着領域は最小周長テンプレートである。
    replace_selected_placement_success(selected.diagnostics, PlacementSource::MinimumTemplate);
    diagnostics.successes++;
    diagnostics.successes_by_blocker_count[blocker_bucket]++;
    diagnostics.moved_groups += chosen.blockers.size();
    diagnostics.arrival_fee_gain += chosen.compact_fee - baseline_score;
    diagnostics.movement_cost += chosen.movement_cost;
    diagnostics.immediate_gain += chosen.direct_gain;
    if (no_region_pushout) {
        diagnostics.pushout_adopted++;
        diagnostics.pushout_adopted_by_blocker_count[blocker_bucket]++;
        diagnostics.pushout_moved_groups += chosen.blockers.size();
        for (int id : chosen.blockers) diagnostics.pushout_moved_cells += groups[id].p;
        diagnostics.pushout_arrival_fee += chosen.compact_fee;
        diagnostics.pushout_movement_cost += chosen.movement_cost;
        diagnostics.pushout_direct_gain += chosen.direct_gain;
    }
    return RootActionResult{std::move(chosen.plan), std::move(selected)};
}

// 再配置を伴わない通常配置の次点案を、baselineと同じroot比較へ載せる。
// 高価なので、呼び出し側が「各進行度区間で最大1回」に絞ったターンだけ実行する。
// 2シナリオscreenで勝った後、独立holdoutでも勝った場合にだけ次点案を返す。
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

    // baselineの未来料金を先に計算し、全次点案で同じ基準・同じシナリオを共有する。
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
        ll scaled_margin = 0;
    };
    vector<TurnPlan> alternative_plans;
    vector<ArrivalDecision> alternative_decisions;
    vector<vvi> alternative_owners;
    alternative_plans.reserve(available_alternatives.size());
    alternative_decisions.reserve(available_alternatives.size());
    alternative_owners.reserve(available_alternatives.size());

    // 現在料金の差と未来料金の差を足し、平均が正になる最良の次点案をscreen勝者とする。
    RootActionKind selected_kind = RootActionKind::Baseline;
    int selected_alternative = -1;
    ll best_scaled_margin = 0;
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
        evaluation.scaled_margin = root_margin_scaled_ll(
            ROOT_SCREEN_SCENARIO_COUNT, direct_gain,
            accumulate(evaluation.future_delta.begin(), evaluation.future_delta.end(), 0LL));

        int alternative_index = alternative_plans.size();
        alternative_plans.push_back(std::move(plan));
        alternative_decisions.push_back(std::move(decision));
        alternative_owners.push_back(std::move(final_owner));
        if (evaluation.scaled_margin > best_scaled_margin) {
            best_scaled_margin = evaluation.scaled_margin;
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
    long double screen_gain = root_margin_fee(best_scaled_margin, ROOT_SCREEN_SCENARIO_COUNT);
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


// セルの重複・盤外を拒否し、4近傍BFSで全セルへ到達できるかを厳密に確認する。
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
    while (!que.empty()) {
        auto [x, y] = que.front();
        que.pop();
        reached++;
        for (int dir = 0; dir < 4; dir++) {
            int nx = x + ORTHOGONAL_DX[dir];
            int ny = y + ORTHOGONAL_DY[dir];
            if (!inside(nx, ny, n, n)) continue;
            int next = nx * n + ny;
            if (!in_region[next] || visited[next]) continue;
            visited[next] = true;
            que.emplace(nx, ny);
        }
    }
    return reached == (int)cells.size();
}

// 採用済み計画を実盤面へ反映する。
// 複数組の場所交換も許すため、移動元を全て消してから移動先を一括配置する。
// max_perimeterは料金履歴なので、再配置後の周長との最大値を保存する。
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

// 対話プロトコルどおりに「再配置数と各領域」「今回の受入可否と領域」を出力する。
// flushまでを出力待機時間として別計測し、解法本体の実行時間から除外する。
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

    // ---------- 初期入力と前計算 ----------
    auto initial_input_wall_begin = RuntimeDiagnostics::WallClock::now();
    int N, M;
    ld R;
    cin >> N >> M >> R;
    assert(0 < N && N <= BOARD_SIDE_LIMIT);
    int r_milli = (int)llroundl(R * 1000.0L);
    vs park(N);
    for (string &row : park) cin >> row;
    runtime_diagnostics.add_input(initial_input_wall_begin);

    auto preprocess_wall_begin = RuntimeDiagnostics::WallClock::now();
    clock_t preprocess_cpu_begin = clock();

    // 面積ごとに全テンプレートと、通常配置で優先する最小周長+4以内のテンプレートを作る。
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

    // Eは芝セルから池または盤外へ出る4近傍辺数、Gは芝セル数である。
    // 保存済みv29のcase別結果から作った排他的portfolioを初期盤面だけで選ぶ。
    // 0.55未満はDLP 1.30、0.55以上0.70未満は1.25、0.70以上0.80未満は
    // 旧full、0.80以上はDLPを変えず候補生成だけを広げたplacement p2とする。
    // さらに0.70以上0.80未満かつR<0.060だけ、保存cacheのsearch/validationで
    // ともに改善したroot未来重み0.1の第5expertへ置き換える。
    // 到着列や途中scoreを見てexpertを変更しない。
    int exposed_boundary_edges = 0;
    for (int x = 0; x < N; x++) {
        for (int y = 0; y < N; y++) {
            if (park[x][y] != '.') continue;
            for (int dir = 0; dir < 4; dir++) {
                int nx = x + ORTHOGONAL_DX[dir];
                int ny = y + ORTHOGONAL_DY[dir];
                if (!inside(nx, ny, N, N) || park[nx][ny] == '#') {
                    exposed_boundary_edges++;
                }
            }
        }
    }
    const CaseStaticPolicy case_policy =
        select_case_static_policy(exposed_boundary_edges, grass_cells, r_milli);
    const int case_static_expert = case_policy.expert;
    case_dlp_scale_milli = case_policy.dlp_scale_milli;
    case_placement_config = case_policy.placement;
    case_root_future_weight_milli = case_policy.root_future_weight_milli;
    // 保存100 seedでpolishの大損tailがなかったsmooth expertだけがtrueになる。
    case_connected_polish_enabled = case_policy.connected_polish_enabled;

    // owner[x][y]は池・空きセルを-1、占有セルを組IDで表す。
    // 退去queueは終了時刻の小さい順に、現在盤面から消す組を管理する。
    vvi owner(N, vi(N, -1));
    vector<GroupState> groups(M);
    priority_queue<pair<ll, int>, vector<pair<ll, int>>, greater<pair<ll, int>>> departures;
    int accepted_count = 0;
    int rejected_count = 0;
    ShadowDiagnostics shadow_diagnostics;
    TemporalPlacementDiagnostics placement_diagnostics;
    RescueDiagnostics rescue_diagnostics;
    LossDiagnostics loss_diagnostics;
    int root_confirmations_used = 0;
    array<bool, 4> normal_root_window_used{};
    int occupied_cells = 0;
    runtime_diagnostics.add_preprocess(preprocess_wall_begin, preprocess_cpu_begin);

    auto static_geometry_wall_begin = RuntimeDiagnostics::WallClock::now();
    clock_t static_geometry_cpu_begin = clock();
    int static_largest_component = largest_free_component(park, owner);
    runtime_diagnostics.add_diagnostic(static_geometry_wall_begin, static_geometry_cpu_begin);

    // ---------- 各到着ターン ----------
    for (int turn = 0; turn < M; turn++) {
        auto input_wall_begin = RuntimeDiagnostics::WallClock::now();
        int i, P;
        ll S, T, V;
        cin >> i >> S >> T >> P >> V;
        runtime_diagnostics.add_input(input_wall_begin);
        // 公式protocolの到着順と面積制約を、添字へ使う前にdebug buildで確認する。
        assert(i == turn);
        assert(4 <= P && P < (int)compact_shapes.size());

        auto turn_wall_begin = RuntimeDiagnostics::WallClock::now();
        clock_t turn_cpu_begin = clock();

        // 観測した滞在時間をtheta推定へ追加し、この時点での未来分布を更新する。
        groups[i].s = S;
        groups[i].t = T;
        groups[i].v = V;
        groups[i].p = P;
        theta_estimator.observe(T - S);
        int remaining_groups = M - i - 1;
        long double theta = theta_estimator.estimate(S, remaining_groups);

        // 問題のイベント順では、終了時刻がちょうどSの組も今回の到着処理中はまだ存在する。
        // したがって、この時点で消すのは t < S の組だけである。
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

        // 今回Pセルを[S,T]で使うことによる未来価値の損失を計算する。
        // 通常提出ではsampled DLP、比較ビルドでは旧64時間帯モデルを使う。
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

        // まず通常方策だけでbaselineを作る。これは必ず残す比較基準である。
        // 条件を満たす場合はrescue案を同じ未来でscreenし、さらに少数の非最小周長ターンでは
        // 再配置なしの通常配置次点も同じroot比較へ追加する。
        vector<NormalPlacementChoice> baseline_alternatives;
        ArrivalDecision baseline_arrival = evaluate_arrival_decision(
            park, owner, groups, i, S, remaining_groups, theta, shadow.opportunity_cost,
            compact_shapes, &baseline_alternatives);
        bool rescue_root_screen_evaluated = false;
        optional<RootActionResult> expanded_action = choose_root_action_with_rescue(
            park, owner, groups, i, S, remaining_groups, r_milli, theta,
            theta_estimator, density_model, sampled_dlp_model,
            grass_cells, shadow.opportunity_cost,
            baseline_arrival, baseline_alternatives, compact_shapes, all_shapes,
            root_confirmations_used, rescue_root_screen_evaluated, rescue_diagnostics);

        // rescueが候補を出さずscreenもしていない場合だけ、通常配置次点の単独比較を行う。
        // コンテスト進行度を4区間に分け、各区間で高々1回に制限する。
        int minimum_perimeter = compact_shapes[P].front().perimeter;
        const NormalPlacementChoice *polish_rollback =
            connected_polish_rollback(baseline_arrival, baseline_alternatives);
        int normal_root_trigger_perimeter = baseline_arrival.perimeter;
        if (polish_rollback) {
            chmax(normal_root_trigger_perimeter, polish_rollback->perimeter);
        }
        int normal_root_window = min(3, turn * 4 / M);
        bool normal_root_gate =
            !ROOT_PROTECTED_ONLY && !expanded_action && !rescue_root_screen_evaluated && remaining_groups > 0 &&
            baseline_arrival.status == ArrivalStatus::Accepted && baseline_arrival.cells &&
            normal_root_trigger_perimeter > minimum_perimeter &&
            !baseline_alternatives.empty() &&
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

        // 拡張探索が採用案を返さなければ、baselineをそのまま最終行動にする。
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

        // 行動反映前の状態から、このターンの移動費と不可逆な料金悪化を診断用に再計算する。
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

        // 計画を実状態へ反映し、受入組だけを退去queueへ登録する。
        apply_plan(i, plan, owner, groups);
        if (plan.arrival) {
            departures.emplace(T, i);
            accepted_count++;
            occupied_cells += P;
        } else {
            rejected_count++;
        }
        runtime_diagnostics.add_turn(turn_wall_begin, turn_cpu_begin);

        // スコア分解は意思決定後に行い、その時間を解法本体と分けて測る。
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

    // ---------- 最終診断 ----------
    // 各受入組の最終料金を確定し、stderrへ出すスコア分解と整合性検査値をまとめる。
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
    int dense_box_source_error =
        max(0, loss_diagnostics.accepted_dense_box_trim -
                   loss_diagnostics.accepted_by_source[2]);
    int perimeter_descent_source_error =
        max(0, loss_diagnostics.accepted_perimeter_descent -
                   loss_diagnostics.accepted_by_source[2]);
    int connected_polish_choice_error =
        max(0, placement_diagnostics.dense_box_successes +
                   placement_diagnostics.perimeter_descent_successes -
                   placement_diagnostics.fallback_successes);
    int connected_polish_static_partition_error =
        placement_diagnostics.connected_polish_candidate_turns -
        placement_diagnostics.connected_polish_static_filtered_turns -
        placement_diagnostics.connected_polish_eligible_turns;
    int dense_box_gate_partition_error =
        placement_diagnostics.connected_polish_eligible_turns -
        placement_diagnostics.dense_box_size_filtered_turns -
        placement_diagnostics.dense_box_eligible_turns -
        placement_diagnostics.dense_box_value_filtered_turns -
        placement_diagnostics.dense_box_budget_skips;
    int dense_box_attempt_count_error =
        case_dense_box_attempts - placement_diagnostics.dense_box_eligible_turns;
    long long small_group_perimeter_descent_subset_error =
        max(0LL, placement_diagnostics.small_group_perimeter_descent_attempts -
                       placement_diagnostics.perimeter_descent_attempts) +
        max(0LL, placement_diagnostics.small_group_perimeter_descent_steps -
                       placement_diagnostics.perimeter_descent_steps) +
        max(0LL, placement_diagnostics.small_group_perimeter_descent_candidates -
                       placement_diagnostics.perimeter_descent_candidates) +
        max(0LL,
            placement_diagnostics.small_group_perimeter_descent_future_fit_rejections -
                placement_diagnostics.perimeter_descent_future_fit_rejections) +
        max(0LL,
            placement_diagnostics.small_group_perimeter_descent_perimeter_improvement -
                placement_diagnostics.perimeter_descent_perimeter_improvement) +
        max(0, placement_diagnostics.small_group_perimeter_descent_successes -
                   placement_diagnostics.perimeter_descent_successes);
    long long small_group_perimeter_descent_attempt_error =
        placement_diagnostics.small_group_perimeter_descent_attempts -
        placement_diagnostics.dense_box_size_filtered_turns;
    int small_group_perimeter_descent_choice_value_error =
        ((placement_diagnostics.small_group_perimeter_descent_successes == 0) !=
         (placement_diagnostics.small_group_perimeter_descent_fee_gain == 0)) +
        ((placement_diagnostics.small_group_perimeter_descent_successes == 0) !=
         (placement_diagnostics
              .small_group_perimeter_descent_perimeter_improvement == 0));
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
        rescue_diagnostics.root_confirmation_policy_steps;
    long long sampled_dlp_rollout_call_error =
        ENABLE_SAMPLED_DLP
            ? dlp_diagnostics.rollout_price_calls - sampled_dlp_expected_rollout_calls
            : dlp_diagnostics.rollout_price_calls;
    runtime_diagnostics.add_diagnostic(final_loss_wall_begin, final_loss_cpu_begin);
    RuntimeSnapshot runtime = snapshot_runtime(runtime_diagnostics);
    cerr << "accepted=" << accepted_count << " rejected=" << rejected_count
         << " case_static_expert=" << case_static_expert
         << " case_dlp_scale_milli=" << case_dlp_scale_milli
         << " case_placement_global_shortlist=" << case_placement_config.global_shortlist
         << " case_placement_shortlist_limit=" << case_placement_config.shortlist_limit
         << " case_connected_growth_seed_limit="
         << case_placement_config.connected_growth_seed_limit
         << " case_grow_and_trim_extra_cells="
         << case_placement_config.grow_and_trim_extra_cells
         << " case_grow_and_trim_candidate_limit="
         << case_placement_config.grow_and_trim_candidate_limit
         << " case_future_fit_min_weight_milli="
         << case_placement_config.future_fit_min_weight_milli
         << " case_root_future_weight_milli=" << case_root_future_weight_milli
         << " case_connected_polish_enabled="
         << case_connected_polish_enabled
         << " dense_box_min_group_size=" << DENSE_BOX_MIN_GROUP_SIZE
         << " dense_box_extra_cells=" << DENSE_BOX_EXTRA_CELLS
         << " dense_box_perimeter_margin=" << DENSE_BOX_PERIMETER_MARGIN
         << " dense_box_global_anchor_limit=" << DENSE_BOX_GLOBAL_ANCHOR_LIMIT
         << " dense_box_total_anchor_limit=" << DENSE_BOX_TOTAL_ANCHOR_LIMIT
         << " dense_box_min_maximum_fee_gain="
         << DENSE_BOX_MIN_MAXIMUM_FEE_GAIN
         << " dense_box_attempt_limit_per_case="
         << DENSE_BOX_ATTEMPT_LIMIT_PER_CASE
         << " perimeter_descent_max_steps=" << PERIMETER_DESCENT_MAX_STEPS
         << " static_exposed_boundary_edges=" << exposed_boundary_edges
         << " static_grass_cells=" << grass_cells
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
         << " placement_connected_polish_changes="
         << placement_diagnostics.connected_polish_changed_placements
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
         << " connected_polish_candidate_turns="
         << placement_diagnostics.connected_polish_candidate_turns
         << " connected_polish_static_filtered_turns="
         << placement_diagnostics.connected_polish_static_filtered_turns
         << " connected_polish_eligible_turns="
         << placement_diagnostics.connected_polish_eligible_turns
         << " dense_box_size_filtered_turns="
         << placement_diagnostics.dense_box_size_filtered_turns
         << " dense_box_eligible_turns=" << placement_diagnostics.dense_box_eligible_turns
         << " dense_box_value_filtered_turns="
         << placement_diagnostics.dense_box_value_filtered_turns
         << " dense_box_budget_skips=" << placement_diagnostics.dense_box_budget_skips
         << " dense_box_anchors_checked=" << placement_diagnostics.dense_box_anchors_checked
         << " dense_box_feasible_anchors=" << placement_diagnostics.dense_box_feasible_anchors
         << " dense_box_shortlisted_anchors="
         << placement_diagnostics.dense_box_shortlisted_anchors
         << " dense_box_component_failures="
         << placement_diagnostics.dense_box_component_failures
         << " dense_box_trim_attempts=" << placement_diagnostics.dense_box_trim_attempts
         << " dense_box_trimmed_cells=" << placement_diagnostics.dense_box_trimmed_cells
         << " dense_box_trim_failures=" << placement_diagnostics.dense_box_trim_failures
         << " dense_box_nonimproving_candidates="
         << placement_diagnostics.dense_box_nonimproving_candidates
         << " dense_box_duplicate_candidates="
         << placement_diagnostics.dense_box_duplicate_candidates
         << " dense_box_candidates=" << placement_diagnostics.dense_box_candidates
         << " dense_box_future_fit_rejections="
         << placement_diagnostics.dense_box_future_fit_rejections
         << " dense_box_perimeter_improvement="
         << placement_diagnostics.dense_box_perimeter_improvement
         << " dense_box_choices=" << placement_diagnostics.dense_box_successes
         << " perimeter_descent_attempts="
         << placement_diagnostics.perimeter_descent_attempts
         << " perimeter_descent_prefilter_rejections="
         << placement_diagnostics.perimeter_descent_prefilter_rejections
         << " perimeter_descent_remove_prefilter_rejections="
         << placement_diagnostics.perimeter_descent_remove_prefilter_rejections
         << " perimeter_descent_steps=" << placement_diagnostics.perimeter_descent_steps
         << " perimeter_descent_candidates="
         << placement_diagnostics.perimeter_descent_candidates
         << " small_group_perimeter_descent_attempts="
         << placement_diagnostics.small_group_perimeter_descent_attempts
         << " small_group_perimeter_descent_steps="
         << placement_diagnostics.small_group_perimeter_descent_steps
         << " small_group_perimeter_descent_candidates="
         << placement_diagnostics.small_group_perimeter_descent_candidates
         << " small_group_perimeter_descent_future_fit_rejections="
         << placement_diagnostics.small_group_perimeter_descent_future_fit_rejections
         << " small_group_perimeter_descent_perimeter_improvement="
         << placement_diagnostics.small_group_perimeter_descent_perimeter_improvement
         << " small_group_perimeter_descent_fee_gain="
         << placement_diagnostics.small_group_perimeter_descent_fee_gain
         << " small_group_perimeter_descent_choices="
         << placement_diagnostics.small_group_perimeter_descent_successes
         << " perimeter_descent_nonimproving_candidates="
         << placement_diagnostics.perimeter_descent_nonimproving_candidates
         << " perimeter_descent_future_fit_rejections="
         << placement_diagnostics.perimeter_descent_future_fit_rejections
         << " perimeter_descent_perimeter_improvement="
         << placement_diagnostics.perimeter_descent_perimeter_improvement
         << " perimeter_descent_choices="
         << placement_diagnostics.perimeter_descent_successes
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
         << " decomp_accepted_dense_box_trim_count="
         << loss_diagnostics.accepted_dense_box_trim
         << " decomp_accepted_perimeter_descent_count="
         << loss_diagnostics.accepted_perimeter_descent
         << " decomp_accepted_unclassified_count=" << loss_diagnostics.accepted_by_source[3]
         << " decomp_accepted_minimum_ideal_fee=" << loss_diagnostics.accepted_source_ideal_fee[0]
         << " decomp_accepted_extended_ideal_fee=" << loss_diagnostics.accepted_source_ideal_fee[1]
         << " decomp_accepted_growth_ideal_fee=" << loss_diagnostics.accepted_source_ideal_fee[2]
         << " decomp_accepted_grow_and_trim_ideal_fee="
         << loss_diagnostics.accepted_grow_and_trim_ideal_fee
         << " decomp_accepted_dense_box_trim_ideal_fee="
         << loss_diagnostics.accepted_dense_box_trim_ideal_fee
         << " decomp_accepted_perimeter_descent_ideal_fee="
         << loss_diagnostics.accepted_perimeter_descent_ideal_fee
         << " decomp_accepted_unclassified_ideal_fee="
         << loss_diagnostics.accepted_source_ideal_fee[3]
         << " decomp_accepted_minimum_initial_fee=" << loss_diagnostics.accepted_source_initial_fee[0]
         << " decomp_accepted_extended_initial_fee=" << loss_diagnostics.accepted_source_initial_fee[1]
         << " decomp_accepted_growth_initial_fee=" << loss_diagnostics.accepted_source_initial_fee[2]
         << " decomp_accepted_grow_and_trim_initial_fee="
         << loss_diagnostics.accepted_grow_and_trim_initial_fee
         << " decomp_accepted_dense_box_trim_initial_fee="
         << loss_diagnostics.accepted_dense_box_trim_initial_fee
         << " decomp_accepted_perimeter_descent_initial_fee="
         << loss_diagnostics.accepted_perimeter_descent_initial_fee
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
         << " decomp_accepted_dense_box_trim_perimeter_excess="
         << loss_diagnostics.accepted_dense_box_trim_perimeter_excess
         << " decomp_accepted_perimeter_descent_perimeter_excess="
         << loss_diagnostics.accepted_perimeter_descent_perimeter_excess
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
         << " dense_box_source_error=" << dense_box_source_error
         << " perimeter_descent_source_error=" << perimeter_descent_source_error
         << " connected_polish_choice_error=" << connected_polish_choice_error
         << " connected_polish_static_partition_error="
         << connected_polish_static_partition_error
         << " dense_box_gate_partition_error=" << dense_box_gate_partition_error
         << " dense_box_attempt_count_error=" << dense_box_attempt_count_error
         << " small_group_perimeter_descent_subset_error="
         << small_group_perimeter_descent_subset_error
         << " small_group_perimeter_descent_attempt_error="
         << small_group_perimeter_descent_attempt_error
         << " small_group_perimeter_descent_choice_value_error="
         << small_group_perimeter_descent_choice_value_error
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
