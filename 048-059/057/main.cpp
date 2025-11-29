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
using pd = pair<double, double>;                           // double型のペア
using vpd = vector<pd>;                                  // double型のペアの一次元配
#define rep(i, a, b) for (ll i = (a); i < (ll)(b); i++)  // for文の短縮
#define all(v) v.begin(), v.end()                        // all(v)でvの始まりと終わりのイテレーター
#define x first
#define y second

// --- Utility Functions ---

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
    if (START == -1.0) START = time;
    return time - START;
}

namespace rnd {
    static uint32_t X2 = 12345;
    static uint32_t X3 = 0xcafef00d;
    static uint64_t C_X1 = 0xd15ea5e5ULL << 32 | 23456;

    inline uint32_t next() {
        uint64_t work = (uint64_t)X3 * 3487286589ULL;
        uint32_t ret = (X3 ^ X2) + ((uint32_t)C_X1 ^ (uint32_t)(work >> 32));
        X3 = X2; X2 = (uint32_t)C_X1; C_X1 = work + (C_X1 >> 32);
        return ret;
    }
    inline double nextf() {
        uint64_t v = 0x3ff0000000000000ULL | ((uint64_t)next() << 20);
        double d; memcpy(&d, &v, sizeof(double)); return d - 1.0;
    }
    inline size_t get(size_t n) { return (size_t)((uint64_t)next() * n >> 32); }
    inline size_t range(size_t a, size_t b) { return get(b - a) + a; }
    template<typename T> void shuffle(vector<T>& a) {
        for (size_t i = a.size() - 1; i > 0; --i) swap(a[i], a[get(i + 1)]);
    }
    inline void init(uint32_t seed) {
        X2 = seed;
        X3 = 0xcafef00d ^ (seed << 17);
        C_X1 = 0xd15ea5e5ULL << 32 | (23456 ^ seed);
        for(int i=0; i<8; ++i) next();
    }
}

// --- Problem Structures ---

struct Input {
    ll N, T, M, K, L;
    vector<pd> ps, vs;
    void read() {
        cin >> N >> T >> M >> K >> L;
        ps.resize(N); vs.resize(N);
        rep(i, 0, N) cin >> ps[i].x >> ps[i].y >> vs[i].x >> vs[i].y;
    }
};

struct Output {
    vector<tuple<ll, ll, ll>> merges;
    void push(ll t, ll a, ll b) { merges.emplace_back(t, a, b); }
    void print() {
        // 時刻順に出力する必要があるためソート
        sort(all(merges));
        for (auto [t, a, b] : merges) cout << t << " " << a << " " << b << "\n";
    }
};

// --- Physics Logic for Tree Simulation ---

// 物理状態のみを持つ軽量構造体
struct ClusterState {
    pd p; // 位置
    pd v; // 速度
    int size; // 原子数
    ll t_update; // 位置pが有効な時刻
    int id; // 代表ID（出力用）
};

// トーラス上の距離計算
double calc_dist(pd p1, pd p2, double L) {
    double dx = fabs(p1.x - p2.x);
    dx = min(dx, L - dx);
    double dy = fabs(p1.y - p2.y);
    dy = min(dy, L - dy);
    return sqrt(dx * dx + dy * dy);
}

// 時刻 t における位置を計算
pd predict_pos(const ClusterState &c, ll t, double L) {
    double dt = (double)(t - c.t_update);
    double px = c.p.x + c.v.x * dt;
    double py = c.p.y + c.v.y * dt;
    px = fmod(px, L); if(px < 0) px += L;
    py = fmod(py, L); if(py < 0) py += L;
    return {px, py};
}

// マージ後の状態を計算
ClusterState merge_states(const ClusterState &c1, const ClusterState &c2, ll t, double L) {
    pd p1 = predict_pos(c1, t, L);
    pd p2 = predict_pos(c2, t, L);
    
    double new_vx = (c1.size * c1.v.x + c2.size * c2.v.x) / (c1.size + c2.size);
    double new_vy = (c1.size * c1.v.y + c2.size * c2.v.y) / (c1.size + c2.size);
    
    // 重心位置（トーラス考慮）
    double dx = p2.x - p1.x;
    if (dx > L / 2) dx -= L; if (dx < -L / 2) dx += L;
    double dy = p2.y - p1.y;
    if (dy > L / 2) dy -= L; if (dy < -L / 2) dy += L;
    
    double gpx = p1.x + dx * c2.size / (c1.size + c2.size);
    double gpy = p1.y + dy * c2.size / (c1.size + c2.size);
    
    gpx = fmod(gpx, L); if(gpx < 0) gpx += L;
    gpy = fmod(gpy, L); if(gpy < 0) gpy += L;

    return {{gpx, gpy}, {new_vx, new_vy}, c1.size + c2.size, t, min(c1.id, c2.id)};
}

// 結合コスト計算
ll calc_merge_cost(const ClusterState &c1, const ClusterState &c2, ll t, double L) {
    pd p1 = predict_pos(c1, t, L);
    pd p2 = predict_pos(c2, t, L);
    double d = calc_dist(p1, p2, L);
    return (ll)round(d);
}

// --- Merge Tree Optimization ---

struct MergeNode {
    int left = -1;
    int right = -1;
    int parent = -1;
    int time = 0;
    bool is_leaf = false;
    int atom_idx = -1; // leafの場合の原子ID
};

struct GroupSolver {
    Input &input;
    vector<int> atom_indices; // このグループに属する原子IDのリスト
    vector<MergeNode> tree;
    int root;
    int K;

    GroupSolver(Input &inp, const vector<int>& atoms, int init_time) : input(inp), atom_indices(atoms) {
        K = atom_indices.size();
        tree.resize(2 * K - 1);
        
        // 完全二分木に近い形状で初期化
        // 葉ノード: 0 ~ K-1
        // 内部ノード: K ~ 2K-2
        rep(i, 0, K) {
            tree[i].is_leaf = true;
            tree[i].atom_idx = atom_indices[i];
        }
        
        // 単純なトーナメント形式で構築
        vector<int> current_layer(K);
        iota(all(current_layer), 0);
        int next_id = K;
        
        while(current_layer.size() > 1) {
            vector<int> next_layer;
            for(size_t i=0; i<current_layer.size(); i+=2) {
                if(i + 1 < current_layer.size()) {
                    int l = current_layer[i];
                    int r = current_layer[i+1];
                    int p = next_id++;
                    tree[p].left = l;
                    tree[p].right = r;
                    tree[p].time = init_time; // 初期時刻はK-meansで見つけたベストタイム
                    tree[p].is_leaf = false;
                    tree[l].parent = p;
                    tree[r].parent = p;
                    next_layer.push_back(p);
                } else {
                    next_layer.push_back(current_layer[i]);
                }
            }
            current_layer = next_layer;
        }
        root = current_layer[0];
    }

    // ツリー全体のコストを再帰計算
    // memo化なしでもNが小さいので高速
    pair<double, ClusterState> evaluate(int u) {
        if (tree[u].is_leaf) {
            int aid = tree[u].atom_idx;
            return {0.0, {input.ps[aid], input.vs[aid], 1, 0, aid}};
        }

        auto [cost_l, state_l] = evaluate(tree[u].left);
        auto [cost_r, state_r] = evaluate(tree[u].right);

        double my_cost = cost_l + cost_r;
        
        // コスト計算
        my_cost += calc_merge_cost(state_l, state_r, tree[u].time, input.L);
        
        // 新しい状態
        ClusterState my_state = merge_states(state_l, state_r, tree[u].time, input.L);
        
        return {my_cost, my_state};
    }

    double get_score() {
        return evaluate(root).first;
    }
    
    // Output生成用
    void collect_merges(int u, Output &out) {
        if (tree[u].is_leaf) return;
        
        collect_merges(tree[u].left, out);
        collect_merges(tree[u].right, out);
        
        // このノードでのマージを出力
        // IDを取得するために再計算が必要（あるいはキャッシュしておく）
        // ここでは簡易的にevaluateを部分的に呼ぶのと同等の処理を行う
        auto s_l = get_state(tree[u].left);
        auto s_r = get_state(tree[u].right);
        out.push(tree[u].time, s_l.id, s_r.id);
    }

    // 特定ノードの最終状態を取得（再帰）
    ClusterState get_state(int u) {
        if(tree[u].is_leaf) {
            int aid = tree[u].atom_idx;
            return {input.ps[aid], input.vs[aid], 1, 0, aid};
        }
        auto s_l = get_state(tree[u].left);
        auto s_r = get_state(tree[u].right);
        return merge_states(s_l, s_r, tree[u].time, input.L);
    }

    // 焼きなまし
    void anneal(double duration) {
        double start_time = get_time();
        double current_score = get_score();
        
        int internal_start = K;
        int internal_end = 2 * K - 1;
        
        ll valid = 0;
        ll iter = 0;
        
        // SAパラメータ
        double start_temp = 5000.0;
        double end_temp = 100.0;

        while(true) {
            iter++;
            if((iter & 255) == 0) {
                double now = get_time();
                if(now - start_time > duration) break;
            }

            double time_ratio = (get_time() - start_time) / duration;
            double temp = start_temp + (end_temp - start_temp) * time_ratio;

            int type = rnd::range(0, 2); // 0: time shift, 1: leaf swap

            if (type == 0) {
                // Time Shift
                int u = rnd::range(internal_start, internal_end);
                int old_t = tree[u].time;
                int delta = rnd::range(0, 2) ? rnd::range(1, 21) : -rnd::range(1, 21);
                int new_t = old_t + delta;
                
                // 制約チェック
                if (new_t < 0 || new_t >= input.T) continue;
                
                // 親より遅く、子より早くなければならない (同時刻はOK)
                int p = tree[u].parent;
                int l = tree[u].left;
                int r = tree[u].right;
                
                if (p != -1 && new_t > tree[p].time) continue; // 親の時刻を超えるのはNG
                int t_l = tree[l].is_leaf ? 0 : tree[l].time;
                int t_r = tree[r].is_leaf ? 0 : tree[r].time;
                if (new_t < t_l || new_t < t_r) continue; // 子の時刻より前なのはNG

                tree[u].time = new_t;
                double new_score = get_score(); // 部分更新すれば高速だがK=30なら全計算でも間に合う
                
                if (new_score < current_score || rnd::nextf() < exp((current_score - new_score) / temp)) {
                    current_score = new_score;
                    valid++;
                } else {
                    tree[u].time = old_t; // revert
                }
            } else {
                // Leaf Swap
                int u1 = rnd::range(0, K);
                int u2 = rnd::range(0, K);
                if (u1 == u2) continue;
                
                swap(tree[u1].atom_idx, tree[u2].atom_idx);
                double new_score = get_score();
                
                if (new_score < current_score || rnd::nextf() < exp((current_score - new_score) / temp)) {
                    current_score = new_score;
                    valid++;
                } else {
                    swap(tree[u1].atom_idx, tree[u2].atom_idx); // revert
                }
            }
        }
        cerr << "Iter: " << iter << " Valid: " << valid << " Score: " << current_score << endl;
    }

    // GroupSolver構造体の中に追加してください
    void set_atoms(const vector<int>& new_atoms, int init_time) {
        atom_indices = new_atoms;
        // ツリーの葉を更新
        rep(i, 0, K) {
            tree[i].is_leaf = true;
            tree[i].atom_idx = atom_indices[i];
        }
        
        // 内部ノードのリセット（トーナメント再構築のような処理）
        // 簡易的に全て init_time に戻す
        int next_id = K;
        vector<int> current_layer(K);
        iota(all(current_layer), 0);
        
        while(current_layer.size() > 1) {
            vector<int> next_layer;
            for(size_t i=0; i<current_layer.size(); i+=2) {
                if(i + 1 < current_layer.size()) {
                    int p = next_id++;
                    // 構造は変えず、時間だけリセットする
                    tree[p].time = init_time;
                    // 親子関係の再リンクは構造が変わらないなら不要だが念のため
                    tree[current_layer[i]].parent = p;
                    tree[current_layer[i+1]].parent = p;
                    next_layer.push_back(p);
                } else {
                    next_layer.push_back(current_layer[i]);
                }
            }
            current_layer = next_layer;
        }
        // ルートは変わらないはずだが念のため更新
        root = current_layer[0];
    }
};


// --- K-means Helpers (from previous discussion) ---

pd calc_torus_centroid(const vector<int>& members, const vector<pd>& ps, double L) {
    if (members.empty()) return {0, 0};
    pd ref = ps[members[0]];
    double sum_dx = 0, sum_dy = 0;
    for (int idx : members) {
        double dx = ps[idx].x - ref.x;
        double dy = ps[idx].y - ref.y;
        if (dx > L / 2) dx -= L; if (dx < -L / 2) dx += L;
        if (dy > L / 2) dy -= L; if (dy < -L / 2) dy += L;
        sum_dx += dx; sum_dy += dy;
    }
    double avg_x = ref.x + sum_dx / members.size();
    double avg_y = ref.y + sum_dy / members.size();
    avg_x = fmod(avg_x, L); if(avg_x < 0) avg_x += L;
    avg_y = fmod(avg_y, L); if(avg_y < 0) avg_y += L;
    return {avg_x, avg_y};
}

long long dist_sq_torus(pd p1, pd p2, double L) {
    double dx = fabs(p1.x - p2.x); dx = min(dx, L - dx);
    double dy = fabs(p1.y - p2.y); dy = min(dy, L - dy);
    return (long long)((dx * dx + dy * dy) * 100); 
}

vector<pd> get_positions_at_time(const Input &input, ll t) {
    vector<pd> current_ps(input.N);
    for(int i = 0; i < input.N; ++i) {
        double px = input.ps[i].x + input.vs[i].x * t;
        double py = input.ps[i].y + input.vs[i].y * t;
        px = fmod(px, input.L); if(px < 0) px += input.L;
        py = fmod(py, input.L); if(py < 0) py += input.L;
        current_ps[i] = {px, py};
    }
    return current_ps;
}

pair<long long, vector<int>> run_kmeans(const Input &input, const vector<pd> &current_ps) {
    int N = input.N; int M = input.M; int K = input.K; double L = input.L;
    vector<pd> centroids;
    centroids.push_back(current_ps[rnd::get(N)]);
    
    while(centroids.size() < M) {
        vector<double> min_dists(N, 1e18);
        double sum_sq_dist = 0;
        for(int i=0; i<N; ++i) {
            for(const auto& c : centroids) {
                double dx = fabs(current_ps[i].x - c.x); dx = min(dx, L - dx);
                double dy = fabs(current_ps[i].y - c.y); dy = min(dy, L - dy);
                min_dists[i] = min(min_dists[i], dx*dx + dy*dy);
            }
            sum_sq_dist += min_dists[i];
        }
        double r = rnd::nextf() * sum_sq_dist;
        size_t pre = centroids.size();
        for(int i=0; i<N; ++i) {
            r -= min_dists[i];
            if(r <= 0) { centroids.push_back(current_ps[i]); break; }
        }
        if(centroids.size() == pre) centroids.push_back(current_ps[rnd::get(N)]);
    }

    vector<int> assignment(N);
    long long final_cost = -1;
    for(int iter=0; iter<15; ++iter) { // 繰り返し回数
        atcoder::mcf_graph<long long, long long> g(N + M + 2);
        int S = 0, T = N + M + 1;
        for(int i=0; i<N; ++i) g.add_edge(S, i + 1, 1, 0);
        for(int i=0; i<N; ++i) {
            for(int j=0; j<M; ++j) {
                g.add_edge(i + 1, N + 1 + j, 1, dist_sq_torus(current_ps[i], centroids[j], L));
            }
        }
        for(int j=0; j<M; ++j) g.add_edge(N + 1 + j, T, K, 0);
        
        auto res = g.flow(S, T, N);
        final_cost = res.second;
        
        vector<vector<int>> new_cls(M);
        for(const auto& e : g.edges()) {
            if(e.from >= 1 && e.from <= N && e.to >= N + 1 && e.to <= N + M && e.flow > 0) {
                int aid = e.from - 1;
                int gid = e.to - (N + 1);
                new_cls[gid].push_back(aid);
                assignment[aid] = gid;
            }
        }
        bool chg = false;
        for(int j=0; j<M; ++j) {
            if(new_cls[j].empty()) continue;
            pd nc = calc_torus_centroid(new_cls[j], current_ps, L);
            if(abs(nc.x - centroids[j].x) > 1e-3 || abs(nc.y - centroids[j].y) > 1e-3) chg = true;
            centroids[j] = nc;
        }
        if(!chg) break;
    }
    return {final_cost, assignment};
}


// --- Main Logic ---

void solve() {
    Input input;
    input.read();
    Output output;

    // 1. K-means で最適な一括結合時刻とグループ分けを探す (Time Limit: 0.3s)
    long long best_cost = -1;
    vector<int> best_assignment;
    int best_time = 0;
    
    // 時間探索
    for(int t = 0; t < input.T; t += 20) {
        if(get_time() > 0.3) break; 
        vector<pd> ps = get_positions_at_time(input, t);
        auto res = run_kmeans(input, ps);
        if(best_cost == -1 || res.first < best_cost) {
            best_cost = res.first;
            best_assignment = res.second;
            best_time = t;
        }
    }
    cerr << "Init Time: " << best_time << " Cost: " << best_cost << endl;

    // グループごとに原子IDを分類
    vector<vector<int>> group_atoms(input.M);
    rep(i, 0, input.N) {
        group_atoms[best_assignment[i]].push_back(i);
    }

    // 2. 各グループごとに独立してマージ順序と時刻を最適化 (SA)
    // 残り時間を各グループに分配
    double remaining_time = 1.5 - get_time();
    double time_per_group = remaining_time / input.M;

    for(int i = 0; i < input.M; ++i) {
        // グループごとのSolver作成
        GroupSolver solver(input, group_atoms[i], best_time);
        
        // 焼きなまし実行
        solver.anneal(time_per_group);
        
        // 結果をOutputに登録
        solver.collect_merges(solver.root, output);
    }

    output.print();
}

int main() {
    // 時間をシードにする
    auto now = std::chrono::high_resolution_clock::now();
    uint32_t seed = (uint32_t)now.time_since_epoch().count();
    rnd::init(seed);
    
    solve();
    return 0;
}