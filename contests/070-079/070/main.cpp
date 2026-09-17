#pragma GCC target("avx2")
#pragma GCC optimize("O3")
#pragma GCC optimize("unroll-loops")
#include <bits/stdc++.h>
#include <atcoder/all>
#include <iostream>
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

double get_time() {
    double time;

#ifdef LOCAL
    struct timespec ts;
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts);
    time = ts.tv_sec + ts.tv_nsec * 1e-9;
#else
    using namespace std::chrono;
    auto now = system_clock::now();
    time = duration_cast<nanoseconds>(now.time_since_epoch()).count() * 1e-9;
#endif

    static double START = -1.0;
    if (START == -1.0) {
        START = time;
    }

#ifdef LOCAL
    return (time - START) * 1.0;
#else
    return time - START;
#endif
}

namespace rnd {
    static uint32_t X2 = 12345;
    static uint32_t X3 = 0xcafef00d;
    static uint64_t C_X1 = 0xd15ea5e5ULL << 32 | 23456;

    inline uint32_t next() {
        uint64_t work = (uint64_t)X3 * 3487286589ULL;
        uint32_t ret = (X3 ^ X2) + ((uint32_t)C_X1 ^ (uint32_t)(work >> 32));
        X3 = X2;
        X2 = (uint32_t)C_X1;
        C_X1 = work + (C_X1 >> 32);
        return ret;
    }

    inline uint64_t next64() {
        return (uint64_t)next() << 32 | (uint64_t)next();
    }

    inline double nextf() {
        uint64_t v = 0x3ff0000000000000ULL | ((uint64_t)next() << 20);
        double d;
        memcpy(&d, &v, sizeof(double));
        return d - 1.0;
    }

    inline size_t get(size_t n) {
        assert(0 < n && n <= UINT32_MAX);
        return (size_t)((uint64_t)next() * n >> 32);
    }

    inline size_t range(size_t a, size_t b) {
        assert(a < b);
        return get(b - a) + a;
    }

    inline size_t range_skip(size_t a, size_t b, size_t skip) {
        assert(a <= skip && skip < b);
        size_t n = range(a, b - 1);
        return n + (skip <= n);
    }

    inline ll rangei(ll a, ll b) {
        assert(a < b);
        return (ll)get((size_t)(b - a)) + a;
    }

    template<typename T>
    void shuffle(vector<T>& a) {
        for (size_t i = a.size() - 1; i > 0; --i) {
            swap(a[i], a[get(i + 1)]);
        }
    }
    
    template<typename T, size_t N>
    void shuffle(T (&a)[N]) {
        for (size_t i = N - 1; i > 0; --i) {
            swap(a[i], a[get(i + 1)]);
        }
    }
}

ll N;   // マスの縦横のサイズ(100固定)
ll M;   // 神力の通り道の数(3固定)
vpl spt;  // 各ターンに怪異が発生するマスの座標

vpl ans_path;  // 神力の通り道
vl ans_use;   // ターンごとに使う神力の通り道の番号

ll best_score = 0;  // 最良スコア

vvl dir = {{1,0},{-1,0},{0,1},{0,-1}};

struct Cand {
    vector<vector<pair<pl,ll>>> mins;// 各点におけるその時点までに置かれた札の中で最も近い札までの距離とその札の座標
    vpl path;
    vl use;
    ll score;
    pl now;
    ll den;
    ll t;
    set<pl> ohuda;
    void init(const vpl& p, const vl& u){
        path = p;
        use = u;
        now = {0,0};
        den = 0;
        t=0;
        ohuda.clear();
        mins = vector<vector<pair<pl,ll>>>(N,vector<pair<pl,ll>>(N,{{-1,-1},-1}));
    }
    ll calc_next(ll idx, ll new_use) {
        t++;
        now = {(now.first + path[new_use].first)%N, (now.second + path[new_use].second)%N};
        // 札が新しい場合はminsを更新する
        if(ohuda.count(now)==0){
            // 幅優先探索を行い、各点の距離が最小となる限り幅優先探索を行う
            queue<pair<pl,ll>> q;
            q.push({now,0});
            while(!q.empty()){
                auto [p,d] = q.front(); q.pop();
                if(mins[p.first][p.second].second!=-1 && mins[p.first][p.second].second<=d) continue;
                mins[p.first][p.second] = {now,d};
                rep(i,0,4){
                    if(inside(p.first+dir[i][0],p.second+dir[i][1],N,N)==false) continue;
                    pl np = {(p.first+dir[i][0]),(p.second+dir[i][1])};
                    q.push({np,d+1});
                }
            }
            ohuda.insert(now);
        }
        ll min_dist = mins[spt[idx].first][spt[idx].second].second;
        den += (ll)floor((ld)(min_dist)*sqrt((ld)(1+idx)));
        return den;
    }
    ll calc_score() {
        now = {0,0};
        den = 0;
        ohuda.clear();
        rep(i,0,N*N){
            calc_next(i,use[i]);
        }
        ld dn = (ld)N;
        ld up = (ld)(1e6*dn*dn*dn);
        score = (ll)round(up/((ld)den+1));
        return score;
    }
};

// annealではない
void anneal(double tl) {
    double start = get_time();
    tl = tl - start;
    assert(0.0 < tl);

    ll valid = 0;
    ll iter = 0;
    double heat = 0.0;
    
    static double log_table[65536];
    for (int i = 0; i < 65536; ++i) {
        log_table[i] = log((i + 0.5) / 65536.0);
    }
    rnd::shuffle(log_table);

    while (true) {
        if (true) {
            double time = (get_time() - start) / tl;
            if (time >= tl) {
                break;
            }
            // 問題に合わせて調整する
            const double T0 = tl;
            const double T1 = 0.1;
            heat = T0 * pow(T1 / T0, time);
        }
        iter++;

        //double add = heat * log_table[iter % 65536]; // 最大化
        // double add = -heat * log_table[iter % 65536]; // 最小化
        // double add = 0.0; // 山登り法になる

        // TODO: スコア計算の実装
        // double old_score = ...;
        // double new_score = ...;

        // if (new_score - old_score >= add) { // 最大化
        // if (new_score - old_score <= add) { // 最小化
            //valid += 1;
            // TODO: 状態の更新
        // }
        // 一旦ランダム
        vpl path(M);
        vl use(N*N);
        // ランダムに選ぶ
        rep(i,0,M){
            path[i] = {rnd::range(0,N),rnd::range(0,N)};
        }
            
        // ターンごとにランダムにパスを選ぶ
        rep(i,0,N*N){
            use[i] = rnd::range(0,M);
        }
        Cand c;
        c.init(path,use);
        c.calc_score();
        if(chmax(best_score,c.score)){
            cerr << "best_score = " << best_score << "\n";
            ans_path = path;
            ans_use = use;
        }
    }

    cerr << "iter = " << iter << "\n";
}

void solve() {
    // ランダムに選ぶ
    rep(i,0,M){
        ans_path[i] = {rnd::range(0,N),rnd::range(0,N)};
    }
        
    // ターンごとにランダムにパスを選ぶ
    rep(i,0,N*N){
        ans_use[i] = rnd::range(0,M);
    }
    Cand c;
    c.init(ans_path,ans_use);
    c.calc_score();
    best_score = c.score;
    cerr << "best_score = " << best_score << "\n";
    // 制限時間までランダムに探索する
    double tl = 1.4;
    anneal(tl);
}

int main(){

    scan(N,M);
    spt.resize(N*N);
    ans_path.resize(M);
    ans_use.resize(N*N);
    scan(spt);
    solve();
    emit(ans_path);
    emit(ans_use);
}