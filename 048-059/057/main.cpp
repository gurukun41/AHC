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

    inline void init(uint32_t seed) {
        X2 = seed;
        X3 = 0xcafef00d ^ (seed << 17); // 適当に混ぜる
        C_X1 = 0xd15ea5e5ULL << 32 | (23456 ^ seed);
        // ウォームアップ（最初の数回を捨てる）
        for(int i=0; i<8; ++i) next();
    }
}

struct Input {
    ll N;
    ll T;
    ll M;
    ll K;
    ll L;
    vpd ps;
    vpd vs;
    Input() {}
    void read() {
        cin >> N >> T >> M >> K >> L;
        ps.resize(N);
        vs.resize(N);
        rep(i, 0, N) {
            cin >> ps[i].x >> ps[i].y >> vs[i].x >> vs[i].y;
        }
    }
};

struct Output {
    vector<tuple<ll, ll, ll>> merges;
    Input &input;
    Output(Input &input) : input(input) {
        merges.resize(0);
    }

    bool check_size() {
        return merges.size() == (input.N - input.M);
    }

    void push(ll t, ll a, ll b) {
        merges.push_back(make_tuple(t, a, b));
    }

    void print() {
        cerr << "Total merges: " << merges.size() << "\n";
        cerr << "Expected merges: " << (input.N - input.M) << "\n";
        assert(check_size());
        for (auto [t, a, b] : merges) {
            cout << t << " " << a << " " << b << "\n";
        }
    }

};

struct Atom {
    ll id;
    pd p;
    pd v;
    // デフォルトコンストラクタを追加
    Atom() : id(0), p({0, 0}), v({0, 0}) {}
    Atom(ll id, pd p, pd v) : id(id), p(p), v(v) {}

    void move(ll t, Input &input) {
        p.x = fmod(p.x + v.x * t, input.L);
        if(p.x < 0) p.x += input.L;
        p.y = fmod(p.y + v.y * t, input.L);
        if(p.y < 0) p.y += input.L;
    }

    void change_velocity(pd new_v) {
        v = new_v;
    }
};


struct Cluster {
    vector<Atom> atoms;
    ll size;
    Input &input;
    bool alive;
    pd v;
    pd gp;  // 重心位置

    Cluster(Atom atom, Input &input) : input(input) {
        atoms.resize(0);
        atoms.push_back(atom);
        size = 1;
        alive = true;
        v = atom.v;
        gp = atom.p;
    }

    bool can_merge(const Cluster &other) const {
        if (!alive || !other.alive) return false;
        return (size + other.size) <= input.K;
    }

    double gdistance(const Cluster &other) const {
        if(!can_merge(other)) return 1e18;
        double dx = fabs(gp.x - other.gp.x);
        dx = min(dx, input.L - dx);
        double dy = fabs(gp.y - other.gp.y);
        dy = min(dy, input.L - dy);
        return sqrt(dx * dx + dy * dy);
    }

    double distance(const Cluster &other, ll &id1, ll &id2) const {
        if(!can_merge(other)) return 1e18;
        double min_dist = 1e18;
        for (const auto& atom1 : atoms) {
            for (const auto& atom2 : other.atoms) {
                double dx = fabs(atom1.p.x - atom2.p.x);
                dx = min(dx, input.L - dx);
                double dy = fabs(atom1.p.y - atom2.p.y);
                dy = min(dy, input.L - dy);
                double dist = sqrt(dx * dx + dy * dy);
                if (dist < min_dist) {
                    min_dist = dist;
                    id1 = atom1.id;
                    id2 = atom2.id;
                }
            }
        }
        return min_dist;
    }

    bool merge(Cluster &other) {
        if (!can_merge(other)) return false;
        for (auto atom : other.atoms) {
            atoms.push_back(atom);
        }

        double new_vx = (size * v.x + other.size * other.v.x) / (size + other.size);
        double new_vy = (size * v.y + other.size * other.v.y) / (size + other.size);
        for (auto &atom : atoms) {
            atom.change_velocity({new_vx, new_vy});
        }
        v = {new_vx, new_vy};

        double new_gpx = (size * gp.x + other.size * other.gp.x) / (size + other.size);
        new_gpx = fmod(new_gpx, input.L);
        if(new_gpx < 0) new_gpx += input.L;
        double new_gpy = (size * gp.y + other.size * other.gp.y) / (size + other.size);
        new_gpy = fmod(new_gpy, input.L);
        if(new_gpy < 0) new_gpy += input.L;
        gp = {new_gpx, new_gpy};

        size += other.size;
        other.alive = false;
        return true;
    }

    void move(ll t = 1) {
        if (!alive) return;
        for (auto &atom : atoms) {
            atom.move(t, input);
        }
        gp.x = fmod(gp.x + v.x * t, input.L);
        if(gp.x < 0) gp.x += input.L;
        gp.y = fmod(gp.y + v.y * t, input.L);
        if(gp.y < 0) gp.y += input.L;
    }
};

struct ClusterGroup {
    vector<ll> cluster_indices;
    ll target_size;
    ll current_size;
    Input &input;
    
    ClusterGroup(Input &input) : input(input), target_size(input.K), current_size(0) {
        cluster_indices.resize(0);
    }
    
    bool can_add(ll size) const {
        return (current_size + size) <= target_size;
    }
    
    void add_cluster(ll idx, ll size) {
        cluster_indices.push_back(idx);
        current_size += size;
    }
    
    bool is_complete() const {
        return current_size == target_size;
    }
};

struct State {
    vector<Cluster> clusters;
    ll current_time;
    Input &input;
    Output &output;

    State(Input &input, Output &output) : input(input), output(output), current_time(0) {
        clusters.reserve(input.N);
        for (int i = 0; i < input.N; i++) {
            clusters.emplace_back(Atom(i, input.ps[i], input.vs[i]), input);
        }
    }

    // 生きているクラスタの数を取得
    ll count_alive_clusters() const {
        ll cnt = 0;
        for (const auto& cluster : clusters) {
            if (cluster.alive) cnt++;
        }
        return cnt;
    }

    // 目標に到達しているかチェック
    bool is_goal() const {
        return count_alive_clusters() == input.M;
    }

    // 2つのクラスタを結合（時刻tで）
    bool merge_clusters(ll idx1, ll idx2, ll t) {
        if (idx1 == idx2) return false;
        if (!clusters[idx1].alive || !clusters[idx2].alive) return false;
        if (t < current_time) return false;
        
        // 時刻tまで移動
        ll dt = t - current_time;
        if (dt > 0) {
            move_all(dt);
        }
        
        // 結合する原子のIDを取得
        ll atom_id1, atom_id2;
        clusters[idx1].distance(clusters[idx2], atom_id1, atom_id2);
        
        // 結合実行
        if (clusters[idx1].merge(clusters[idx2])) {
            output.push(t, atom_id1, atom_id2);
            return true;
        }
        return false;
    }

    // 全クラスタを時間分移動
    void move_all(ll dt = 1) {
        for (auto &cluster : clusters) {
            cluster.move(dt);
        }
        current_time += dt;
    }

    // 時刻tまで進める
    void advance_to(ll t) {
        if (t > current_time) {
            move_all(t - current_time);
        }
    }

    // 最も近い2つのクラスタのペアを取得（結合可能なもの）
    pair<ll, ll> get_nearest_pair() const {
        double min_dist = 1e18;
        ll best_i = -1, best_j = -1;
        
        for (ll i = 0; i < clusters.size(); i++) {
            if (!clusters[i].alive) continue;
            for (ll j = i + 1; j < clusters.size(); j++) {
                if (!clusters[j].alive) continue;
                if (!clusters[i].can_merge(clusters[j])) continue;
                
                double dist = clusters[i].gdistance(clusters[j]);
                if (dist < min_dist) {
                    min_dist = dist;
                    best_i = i;
                    best_j = j;
                }
            }
        }
        return {best_i, best_j};
    }

    // スコア評価用：総結合コストの概算（既に出力されたもの）
    ll calculate_current_cost() const {
        // output.mergesから計算するか、別途管理
        return 0; // TODO: 実装
    }

    // デバッグ用
    void print_status() const {
        cerr << "Time: " << current_time << ", Alive clusters: " << count_alive_clusters() << endl;
    }
};


// トーラス上の重心を計算する関数
pd calc_torus_centroid(const vector<int>& members, const vector<pd>& ps, double L) {
    if (members.empty()) return {0, 0};
    
    // 最初の点を基準にする（トーラスの境界またぎ対策）
    pd ref = ps[members[0]];
    double sum_dx = 0;
    double sum_dy = 0;
    
    for (int idx : members) {
        double dx = ps[idx].x - ref.x;
        double dy = ps[idx].y - ref.y;
        
        // 最短経路での変位に補正
        if (dx > L / 2) dx -= L;
        if (dx < -L / 2) dx += L;
        if (dy > L / 2) dy -= L;
        if (dy < -L / 2) dy += L;
        
        sum_dx += dx;
        sum_dy += dy;
    }
    
    double avg_x = ref.x + sum_dx / members.size();
    double avg_y = ref.y + sum_dy / members.size();
    
    // 0~Lの範囲に収める
    avg_x = fmod(avg_x, L);
    if(avg_x < 0) avg_x += L;
    avg_y = fmod(avg_y, L);
    if(avg_y < 0) avg_y += L;
    
    return {avg_x, avg_y};
}

// 2点間のトーラス距離の2乗を返す（K-meansは2乗距離の和を最小化するため）
long long dist_sq_torus(pd p1, pd p2, double L) {
    double dx = fabs(p1.x - p2.x);
    dx = min(dx, L - dx);
    double dy = fabs(p1.y - p2.y);
    dy = min(dy, L - dy);
    // MCFのコストは整数である必要があるため、適当にスケーリングしてlong longにする
    return (long long)((dx * dx + dy * dy) * 100); 
}


// 指定した時刻 t における全原子の位置を計算して返す
vector<pd> get_positions_at_time(const Input &input, ll t) {
    vector<pd> current_ps(input.N);
    for(int i = 0; i < input.N; ++i) {
        double px = input.ps[i].x + input.vs[i].x * t;
        double py = input.ps[i].y + input.vs[i].y * t;
        
        // トーラス処理
        px = fmod(px, input.L);
        if(px < 0) px += input.L;
        py = fmod(py, input.L);
        if(py < 0) py += input.L;
        
        current_ps[i] = {px, py};
    }
    return current_ps;
}

// 戻り値: {最小コスト, 割り当て配列(index -> group_id)}
pair<long long, vector<int>> run_kmeans_on_positions(const Input &input, const vector<pd> &current_ps) {
    int N = input.N;
    int M = input.M;
    int K = input.K;
    double L = input.L;
    
    // 1. K-means++ 初期化
    vector<pd> centroids;
    centroids.push_back(current_ps[rnd::get(N)]);
    
    while(centroids.size() < M) {
        vector<double> min_dists(N, 1e18);
        double sum_sq_dist = 0;
        for(int i=0; i<N; ++i) {
            for(const auto& c : centroids) {
                double dx = fabs(current_ps[i].x - c.x);
                dx = min(dx, L - dx);
                double dy = fabs(current_ps[i].y - c.y);
                dy = min(dy, L - dy);
                min_dists[i] = min(min_dists[i], dx*dx + dy*dy);
            }
            sum_sq_dist += min_dists[i];
        }
        
        size_t pre_size = centroids.size();
        double r = rnd::nextf() * sum_sq_dist;
        for(int i=0; i<N; ++i) {
            r -= min_dists[i];
            if(r <= 0) {
                centroids.push_back(current_ps[i]);
                break;
            }
        }
        if(centroids.size() == pre_size) centroids.push_back(current_ps[rnd::get(N)]);
    }

    // 2. K-means 反復
    int max_iter = 10; // 探索回数を増やすため反復は少し減らしてもOK
    vector<int> assignment(N);
    long long final_cost = -1;
    
    for(int iter=0; iter<max_iter; ++iter) {
        int S = 0, T = N + M + 1;
        atcoder::mcf_graph<long long, long long> g(T + 1);
        
        for(int i=0; i<N; ++i) g.add_edge(S, i + 1, 1, 0);
        
        for(int i=0; i<N; ++i) {
            for(int j=0; j<M; ++j) {
                long long cost = dist_sq_torus(current_ps[i], centroids[j], L);
                g.add_edge(i + 1, N + 1 + j, 1, cost);
            }
        }
        
        for(int j=0; j<M; ++j) g.add_edge(N + 1 + j, T, K, 0);
        
        auto result = g.flow(S, T, N);
        final_cost = result.second; // 最小費用流のコストがそのままクラスタリングの良さになる
        
        vector<vector<int>> new_clusters(M);
        auto edges = g.edges();
        for(const auto& e : edges) {
            if(e.from >= 1 && e.from <= N && e.to >= N + 1 && e.to <= N + M && e.flow > 0) {
                int atom_idx = e.from - 1;
                int group_idx = e.to - (N + 1);
                new_clusters[group_idx].push_back(atom_idx);
                assignment[atom_idx] = group_idx;
            }
        }
        
        bool changed = false;
        for(int j=0; j<M; ++j) {
            if(new_clusters[j].empty()) continue;
            pd new_c = calc_torus_centroid(new_clusters[j], current_ps, L);
            if(abs(new_c.x - centroids[j].x) > 1e-3 || abs(new_c.y - centroids[j].y) > 1e-3) changed = true;
            centroids[j] = new_c;
        }
        if(!changed) break;
    }
    
    return {final_cost, assignment};
}


// --- 追加: 軌道予測に基づく結合計画 ---

struct SimCluster {
    int id;
    pd p, v;
    int size;
    int available_time; // このクラスタが結合可能になる時刻（前の結合が終わった時刻）

    // 指定時刻の位置予測
    pd pos_at(int t, double L) const {
        double dt = t - available_time; // 基準時刻からの経過時間ではなく、位置pが更新された時刻からの差分が必要
        // 今回の実装では、merge時に位置をその時刻のものに更新する方針をとるため、
        // 単純に (t - update_time) で計算する
        // 簡易実装のため、メンバ変数 p は "available_time" における位置とする
        
        double px = p.x + v.x * dt;
        double py = p.y + v.y * dt;
        px = fmod(px, L); if(px < 0) px += L;
        py = fmod(py, L); if(py < 0) py += L;
        return {px, py};
    }
};

// 2つのSimClusterの将来の最小距離と、その時刻を探す
tuple<double, int> find_best_merge_time(const SimCluster &c1, const SimCluster &c2, int T, double L) {
    int start_t = max(c1.available_time, c2.available_time);
    double min_dist = 1e18;
    int best_t = start_t;

    // Tまでの時間を走査 (刻み幅を小さくすれば精度向上)
    // ここでは処理速度優先で 粗く探索 -> 周辺を細かく探索 とする
    
    auto calc_dist = [&](int t) -> double {
        pd p1 = c1.pos_at(t, L);
        pd p2 = c2.pos_at(t, L);
        double dx = fabs(p1.x - p2.x); dx = min(dx, L - dx);
        double dy = fabs(p1.y - p2.y); dy = min(dy, L - dy);
        return sqrt(dx*dx + dy*dy);
    };

    // 1. 粗い探索 (step 10)
    for(int t = start_t; t < T; t += 10) {
        double d = calc_dist(t);
        if(d < min_dist) {
            min_dist = d;
            best_t = t;
        }
    }
    // 最後もチェック
    if (T > start_t) {
        double d = calc_dist(T-1);
        if(d < min_dist) {
            min_dist = d;
            best_t = T-1;
        }
    }

    // 2. 精密探索 (best_t の前後)
    int search_start = max(start_t, best_t - 15);
    int search_end = min(T, best_t + 15);
    for(int t = search_start; t < search_end; ++t) {
        double d = calc_dist(t);
        if(d < min_dist) {
            min_dist = d;
            best_t = t;
        }
    }

    return {min_dist, best_t};
}

// 1つのグループに対する結合計画を作成
void plan_merges_greedy(const Input &input, const vector<int> &atom_indices, vector<tuple<ll, ll, ll>> &all_merges) {
    int K = atom_indices.size();
    vector<SimCluster> clusters;
    
    // 初期化
    for(int idx : atom_indices) {
        clusters.push_back({
            (int)input.vs[idx].x, // input.vs[idx].x は速度だが、idを入れる場所間違えた？ -> いや、Atom生成時にあわせる
            // SimCluster定義: id, p, v, size, time
            (int)idx, 
            input.ps[idx], 
            input.vs[idx], 
            1, 
            0
        });
        // 最初のメンバ変数が id なので修正
        clusters.back().id = idx; // 念のため上書き
    }

    // クラスタが1つになるまで繰り返す
    while(clusters.size() > 1) {
        double global_min_dist = 1e18;
        int best_t = -1;
        int best_i = -1, best_j = -1;

        // 全ペア探索
        for(int i=0; i<(int)clusters.size(); ++i) {
            for(int j=i+1; j<(int)clusters.size(); ++j) {
                // サイズ制限チェック
                if(clusters[i].size + clusters[j].size > input.K) continue;

                auto [dist, t] = find_best_merge_time(clusters[i], clusters[j], input.T, input.L);
                
                // コストが小さいもの、同じなら時間が早いものを優先
                if(dist < global_min_dist) {
                    global_min_dist = dist;
                    best_t = t;
                    best_i = i;
                    best_j = j;
                }
            }
        }

        if(best_i == -1) break; // 結合候補なし

        // 結合採用
        SimCluster &c1 = clusters[best_i];
        SimCluster &c2 = clusters[best_j];
        
        // 出力用リストに追加 (時刻, id1, id2)
        all_merges.emplace_back(best_t, c1.id, c2.id);

        // 新しいクラスタの状態作成
        SimCluster new_c;
        new_c.id = min(c1.id, c2.id); // IDは小さい方を継承（出力仕様に合わせて適当に）
        new_c.size = c1.size + c2.size;
        new_c.available_time = best_t;

        // 速度更新 (運動量保存)
        new_c.v.x = (c1.size * c1.v.x + c2.size * c2.v.x) / new_c.size;
        new_c.v.y = (c1.size * c1.v.y + c2.size * c2.v.y) / new_c.size;

        // 位置更新 (重心)
        pd p1 = c1.pos_at(best_t, input.L);
        pd p2 = c2.pos_at(best_t, input.L);
        
        // トーラス上の重心計算
        double dx = p2.x - p1.x;
        if (dx > input.L / 2) dx -= input.L;
        if (dx < -input.L / 2) dx += input.L;
        double dy = p2.y - p1.y;
        if (dy > input.L / 2) dy -= input.L;
        if (dy < -input.L / 2) dy += input.L;

        new_c.p.x = p1.x + dx * c2.size / new_c.size;
        new_c.p.y = p1.y + dy * c2.size / new_c.size;
        
        // 正規化
        new_c.p.x = fmod(new_c.p.x, input.L); if(new_c.p.x < 0) new_c.p.x += input.L;
        new_c.p.y = fmod(new_c.p.y, input.L); if(new_c.p.y < 0) new_c.p.y += input.L;

        // 配列更新 (後ろから消すとindexズレが少ない)
        clusters.erase(clusters.begin() + best_j);
        clusters[best_i] = new_c;
    }
}


void solve() {
    Input input;
    input.read();
    Output output(input);

    // 1. K-means Init (ここまでは元のコードと同じ)
    long long best_cost = -1; 
    vector<int> best_assignment;
    int best_time = 0;
    int time_step = 50; 
    for(int t = 0; t < input.T; t += time_step) {
        vector<pd> ps_at_t = get_positions_at_time(input, t);
        pair<long long, vector<int>> result = run_kmeans_on_positions(input, ps_at_t);
        if (best_cost == -1 || result.first < best_cost) {
            best_cost = result.first;
            best_assignment = result.second;
            best_time = t;
        }
    }
    cerr << "Best clustering found at time: " << best_time << " with cost: " << best_cost << endl;

    // グループ作成
    vector<vector<int>> groups(input.M);
    for(int i=0; i<input.N; ++i) {
        groups[best_assignment[i]].push_back(i);
    }

    // 2. 実行フェーズ (変更点: 一括シミュレーションではなく、グループごとの計画作成)
    vector<tuple<ll, ll, ll>> all_merges;
    
    for(int i=0; i<input.M; ++i) {
        plan_merges_greedy(input, groups[i], all_merges);
    }

    // 3. 出力 (時刻順にソート)
    sort(all_merges.begin(), all_merges.end());
    for(auto [t, a, b] : all_merges) {
        output.push(t, a, b);
    }
    
    // Output構造体のprintを呼ぶ (assertチェックなどが入っているため)
    output.print();
}


int main(){
    rnd::init(42); // 乱数初期化
    solve();
}