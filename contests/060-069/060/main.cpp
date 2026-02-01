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

void yn(bool a) {
    if (a)
        cout << "Yes\n";
    else
        cout << "No\n";
}

// 時間計測
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

// 乱数生成器
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

    inline size_t get(size_t n) {
        if (n == 0) return 0;
        return (size_t)((uint64_t)next() * n >> 32);
    }

    inline size_t range(size_t a, size_t b) {
        if (a >= b) return a;
        return get(b - a) + a;
    }
}

// グローバル変数
ll N, M, K, T;
vvl graph;
vl X, Y;
vvl dist_matrix; // 全点対最短距離

// 全点対最短距離の計算
void calc_all_pairs_shortest_path(ll n, const vvl& g) {
    dist_matrix.assign(n, vl(n, 1e18));
    rep(start, 0, n) {
        dist_matrix[start][start] = 0;
        queue<ll> q;
        q.push(start);
        while (!q.empty()) {
            ll v = q.front();
            q.pop();
            for (ll nv : g[v]) {
                if (dist_matrix[start][nv] == 1e18) {
                    dist_matrix[start][nv] = dist_matrix[start][v] + 1;
                    q.push(nv);
                }
            }
        }
    }
}

struct Solution {
    // 0 ~ N-1: 通常訪問 (移動のみ/収穫/納品)
    // N ~ 2N-1: 味変訪問 (移動 -> 味変 -> 収穫) ※木の場合のみ
    vector<int> waypoints;
    ll score;

    Solution() : score(0) {}
};

// 評価関数: 通過点での納品を考慮
ll evaluate_solution(Solution& sol) {
    ll curr = 0;
    ll steps = 0;
    ll prev_move_from = -1;
    
    string cone = "";
    const int MAX_CONE_LEN = 100; 
    
    vector<bool> current_tree_flavor(N, false); // false: W, true: R
    vector<set<string>> shops(K);
    
    for (int raw_target : sol.waypoints) {
        if (steps >= T) break;

        // ターゲット情報のデコード
        int target = raw_target % N;
        bool try_change_flavor = (raw_target >= N);

        // --- 移動フェーズ ---
        while (curr != target) {
            if (steps >= T) break;
            
            // 次の移動先を決定 (prev_move_from以外で最短方向)
            int next = -1;
            ll min_d = 1e18;
            
            for (int adj : graph[curr]) {
                if (adj == prev_move_from) continue;
                
                ll d = dist_matrix[adj][target];
                if (d < min_d) {
                    min_d = d;
                    next = adj;
                }
            }
            
            if (next == -1) { sol.score = 0; return -1; } // Error
            
            prev_move_from = curr;
            curr = next;
            steps++;
            
            // --- 到着した頂点での処理 ---
            if (curr >= K) { 
                // 木の場合：収穫
                char flavor = current_tree_flavor[curr] ? 'R' : 'W';
                if (cone.size() < MAX_CONE_LEN) {
                    cone += flavor;
                }
            } else { 
                // ショップの場合：納品
                shops[curr].insert(cone);
                cone = ""; 
            }
        }
        
        if (steps >= T) break;
        
        // --- Target到達時のアクション (味変) ---
        // ターゲットに指定されたタイミングでのみ味変を試みる
        if (try_change_flavor) {
            // 木であり、かつ現在バニラ(白)であれば赤にする
            if (target >= K && !current_tree_flavor[target]) {
                if (steps < T) {
                    current_tree_flavor[target] = true;
                    steps++;
                    // ※味変後に再度収穫するかは戦略次第だが、
                    // 現状の仕様では「移動時」に収穫しているので、
                    // ここでは味変のみでターン消費とする。
                    // もし味変直後の赤も欲しいなら、ここに追加しても良い。
                    // 今回はシンプルに「次回以降の通過・訪問から赤になる」とする。
                }
            }
        }
    }
    
    // スコア計算
    ll total_score = 0;
    rep(i, 0, K) total_score += shops[i].size();
    
    sol.score = total_score;
    return total_score;
}

// 初期解生成: 貪欲法
Solution generate_initial_solution() {
    Solution sol;
    
    int curr = 0;
    int loop_cnt = 0;
    
    while (sol.waypoints.size() < 300) { 
        loop_cnt++;
        if (loop_cnt > 2000) break; 
        
        bool aim_shop = (rnd::get(10) < 4); // 40%くらいで店
        
        int best_target = -1;
        ll min_dist = 1e18;
        
        if (aim_shop) {
            // 近くのショップ
            rep(s, 0, K) {
                if (s == curr) continue;
                if (dist_matrix[curr][s] < min_dist) {
                    min_dist = dist_matrix[curr][s];
                    best_target = s;
                }
            }
        } else {
            // 近くの木
            rep(t, K, N) {
                if (t == curr) continue;
                if (dist_matrix[curr][t] < min_dist) {
                    min_dist = dist_matrix[curr][t];
                    best_target = t;
                }
            }
        }
        
        if (best_target != -1) {
            // 木の場合、50%の確率で「味変ターゲット」として追加してみる
            if (best_target >= K && rnd::get(2) == 0) {
                sol.waypoints.push_back(best_target + N);
            } else {
                sol.waypoints.push_back(best_target);
            }
            curr = best_target; 
        } else {
            break;
        }
    }
    
    evaluate_solution(sol);
    return sol;
}

// 近傍操作
void mutate(Solution& sol) {
    int type = rnd::get(100);
    
    if (type < 40) { // Insert
        if (sol.waypoints.size() >= 1000) return;
        
        // 木を挿入する場合、味変するかどうかもランダム
        int tree = rnd::range(K, N);
        if (rnd::get(2) == 0) tree += N; // 味変版
        
        int pos = rnd::get(sol.waypoints.size() + 1);
        sol.waypoints.insert(sol.waypoints.begin() + pos, tree);
    } 
    else if (type < 60) { // Delete
        if (sol.waypoints.empty()) return;
        int pos = rnd::get(sol.waypoints.size());
        sol.waypoints.erase(sol.waypoints.begin() + pos);
    }
    else if (type < 80) { // Swap
        if (sol.waypoints.size() < 2) return;
        int p1 = rnd::get(sol.waypoints.size());
        int p2 = rnd::get(sol.waypoints.size());
        swap(sol.waypoints[p1], sol.waypoints[p2]);
    }
    else if (type < 90) { // Change Target
        if (sol.waypoints.empty()) return;
        int pos = rnd::get(sol.waypoints.size());
        int current_val = sol.waypoints[pos];
        int current_id = current_val % N;
        
        if (current_id < K) {
            // 今がショップなら別のショップへ
            sol.waypoints[pos] = rnd::get(K);
        } else {
            // 今が木なら別の木へ（味変有無もランダム）
            int next_tree = rnd::range(K, N);
            if (rnd::get(2) == 0) next_tree += N;
            sol.waypoints[pos] = next_tree;
        }
    }
    else { // Flip Flavor (Local)
        // あるターゲットの味変フラグを反転させる
        if (sol.waypoints.empty()) return;
        int pos = rnd::get(sol.waypoints.size());
        int val = sol.waypoints[pos];
        int id = val % N;
        
        // 木の場合のみ反転可能
        if (id >= K) {
            if (val >= N) {
                sol.waypoints[pos] -= N; // 赤 -> 白
            } else {
                sol.waypoints[pos] += N; // 白 -> 赤
            }
        }
    }
}

int main() {
    cin >> N >> M >> K >> T;
    
    graph.resize(N);
    rep(i, 0, M) {
        ll a, b;
        cin >> a >> b;
        graph[a].push_back(b);
        graph[b].push_back(a);
    }
    
    X.resize(N); Y.resize(N);
    rep(i, 0, N) cin >> X[i] >> Y[i];
    
    calc_all_pairs_shortest_path(N, graph);
    
    Solution best_sol = generate_initial_solution();
    Solution current_sol = best_sol;
    
    double time_limit = 1.95; 
    ll iteration = 0;
    
    while (get_time() < time_limit) {
        iteration++;
        Solution next_sol = current_sol;
        mutate(next_sol);
        
        ll score = evaluate_solution(next_sol);
        
        if (score >= current_sol.score) {
            current_sol = next_sol;
            if (current_sol.score > best_sol.score) {
                best_sol = current_sol;
            }
        }
    }
    
    // --- 最終出力生成 ---
    ll curr = 0;
    ll prev_move_from = -1;
    ll steps = 0;
    vector<bool> current_tree_flavor(N, false);
    string cone = ""; 
    const int MAX_CONE_LEN = 100;
    vector<set<string>> shops(K);

    for (int raw_target : best_sol.waypoints) {
        if (steps >= T) break;
        
        int target = raw_target % N;
        bool try_change_flavor = (raw_target >= N);

        // 移動
        while (curr != target) {
            if (steps >= T) break;
            
            int next = -1;
            ll min_d = 1e18;
            for (int adj : graph[curr]) {
                if (adj == prev_move_from) continue;
                ll d = dist_matrix[adj][target];
                if (d < min_d) {
                    min_d = d;
                    next = adj;
                }
            }
            
            if (next == -1) break;
            
            cout << next << endl; // 行動1出力
            
            prev_move_from = curr;
            curr = next;
            steps++;

            // 移動先での処理
            if (curr >= K) { // 木
                char flavor = current_tree_flavor[curr] ? 'R' : 'W';
                if (cone.size() < MAX_CONE_LEN) cone += flavor;
            } else { // ショップ
                shops[curr].insert(cone);
                cone = "";
            }
        }
        
        if (steps >= T) break;
        
        // 味変
        if (try_change_flavor) {
            if (target >= K && !current_tree_flavor[target]) {
                if (steps < T) {
                    cout << "-1" << endl; // 行動2出力
                    current_tree_flavor[target] = true;
                    steps++;
                }
            }
        }
    }
    
    ll final_score = 0;
    rep(i, 0, K) final_score += shops[i].size();
    cerr << "Final Simulated Score: " << final_score << endl;
    
    return 0;
}