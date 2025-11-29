#include <bits/stdc++.h>
#include <atcoder/all>

using namespace std;
using ll = long long;
using ld = long double;
using mint = atcoder::modint998244353;
using vl = vector<ll>;                                   // long long型の一次元
using vvl = vector<vl>;                                  // long long型の二次元配列
using vvvl = vector<vvl>;                                // long long型の三次元配列
using vi = vector<int>;                                  // int型の一次元
using vvi = vector<vi>;                                  // int型の二次元配列
using vvvi = vector<vvi>;                                // int型の三次元配列
using vb = vector<bool>;                                 // bool型の一次元
using vvb = vector<vb>;                                  // bool型の二次元配列
using vvvb = vector<vvb>;                                // bool型の三次元配列
using vs = vector<string>;                               // string型の一次元
using vvs = vector<vs>;                                  // string型の二次元配列
using pl = pair<ll, ll>;                                 // long long型のペア
using vpl = vector<pl>;                                  // long long型のペアの一次元配列
#define rep(i, a, b) for (ll i = (a); i < (ll)(b); i++)  // for文の短縮
#define all(v) v.begin(), v.end()                        // all(v)でvの始まりと終わりのイテレーター

// get_time
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

// rnd

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

// anneal
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
        if (iter % 256 == 0) {
            double time = (get_time() - start) / tl;
            if (time >= 1.0) {
                break;
            }
            // 問題に合わせて調整する
            const double T0 = 1.0;
            const double T1 = 0.1;
            heat = T0 * pow(T1 / T0, time);
        }
        iter++;

        double add = heat * log_table[iter % 65536]; // 最大化
        // double add = -heat * log_table[iter % 65536]; // 最小化
        // double add = 0.0; // 山登り法になる

        // TODO: スコア計算の実装
        // double old_score = ...;
        // double new_score = ...;

        // if (new_score - old_score >= add) { // 最大化
        // if (new_score - old_score <= add) { // 最小化
            valid += 1;
            // TODO: 状態の更新
        // }
    }

    cerr << "iter = " << iter << "\n";
    cerr << "ratio = " << fixed << setprecision(3) 
              << ((double)valid / iter) << "\n";
}
