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


ll N = 20;
vvl a(20, vl(20));


ll dist(pl p1, pl p2) {
    return abs(p1.first - p2.first) + abs(p1.second - p2.second);
}

struct Task {
    ll val;
    pl p[2];
};

void solve() {
    map<ll, vpl> m;
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            if (a[i][j] != -1) m[a[i][j]].push_back({(ll)i, (ll)j});
        }
    }

    vector<Task> tasks;
    for (auto const& [val, pos] : m) {
        if (pos.size() == 2) {
            tasks.push_back({val, {pos[0], pos[1]}});
        }
    }

    int M = tasks.size();
    if (M == 0) return;

    vl order;
    vl ori;
    vb used(M, false);
    pl cur = {0, 0};

    for (int i = 0; i < M; i++) {
        int best_idx = -1;
        ll min_d = 1e18;
        int best_o = 0;
        for (int j = 0; j < M; j++) {
            if (used[j]) continue;
            for (int o = 0; o < 2; o++) {
                ll d = dist(cur, tasks[j].p[o]);
                if (d < min_d) {
                    min_d = d;
                    best_idx = j;
                    best_o = o;
                }
            }
        }
        used[best_idx] = true;
        order.push_back(best_idx);
        ori.push_back(best_o);
        cur = tasks[best_idx].p[1 - best_o];
    }

    auto get_total_dist = [&]() {
        ll total = 0;
        pl p_prev = {0, 0};
        for (int i = 0; i < M; i++) {
            total += dist(p_prev, tasks[order[i]].p[ori[i]]);
            total += dist(tasks[order[i]].p[ori[i]], tasks[order[i]].p[1 - ori[i]]);
            p_prev = tasks[order[i]].p[1 - ori[i]];
        }
        return total;
    };

    ll current_dist = get_total_dist();
    mt19937 engine(42);
    auto start_time = chrono::steady_clock::now();

    while (true) {
        auto now_time = chrono::steady_clock::now();
        if (chrono::duration_cast<chrono::milliseconds>(now_time - start_time).count() > 1800) break;

        int type = engine() % 100;
        if (type < 30) {
            int i = engine() % M;
            int j = engine() % M;
            if (i == j) continue;
            swap(order[i], order[j]);
            ll next_dist = get_total_dist();
            if (next_dist < current_dist) current_dist = next_dist;
            else swap(order[i], order[j]);
        } else if (type < 60) {
            int i = engine() % M;
            ori[i] = 1 - ori[i];
            ll next_dist = get_total_dist();
            if (next_dist < current_dist) current_dist = next_dist;
            else ori[i] = 1 - ori[i];
        } else {
            int i = engine() % M;
            int j = engine() % M;
            if (i > j) swap(i, j);
            if (i == j) continue;
            reverse(order.begin() + i, order.begin() + j + 1);
            ll next_dist = get_total_dist();
            if (next_dist < current_dist) current_dist = next_dist;
            else reverse(order.begin() + i, order.begin() + j + 1);
        }
    }

    auto move_to = [&](pl target, pl& now, vector<char>& res) {
        while (now.first < target.first) { res.push_back('D'); now.first++; }
        while (now.first > target.first) { res.push_back('U'); now.first--; }
        while (now.second < target.second) { res.push_back('R'); now.second++; }
        while (now.second > target.second) { res.push_back('L'); now.second--; }
    };

    vector<char> moves;
    pl now_p = {0, 0};
    for (int i = 0; i < M; i++) {
        move_to(tasks[order[i]].p[ori[i]], now_p, moves);
        moves.push_back('Z');
        move_to(tasks[order[i]].p[1 - ori[i]], now_p, moves);
        moves.push_back('Z');
    }

    for (int i = 0; i < (int)moves.size() && i < 16000; i++) {
        cout << moves[i] << "\n";
    }
}

int main() {
    cin >> N;
    a.assign(N, vector<ll>(N));
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            cin >> a[i][j];
        }
    }
    solve();
    return 0;
}