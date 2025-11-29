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


void solve() {
    Input input;
    input.read();
    Output output(input);

    State state(input, output);
    
    vector<bool> used(input.N, false);
    vector<ClusterGroup> groups(input.M, ClusterGroup(input));
    
    // --- 1. 時間探索パート（変更なし） ---
    long long best_cost = -1; 
    vector<int> best_assignment;
    int best_time = 0;

    // 粗い刻み幅で探索（例: 50刻み）
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

    // --- 2. グループ割り当ての適用 ---
    // (ここで groups を構築)
    fill(used.begin(), used.end(), true);
    for(int i=0; i<input.N; ++i) {
        int g_idx = best_assignment[i];
        groups[g_idx].add_cluster(i, 1);
    }

    // --- 3. 実行フェーズ（ここを大幅修正） ---

    // 重要: 計算された「ベストな時刻」まで一気に進める
    // ギリギリだと結合順序などで溢れる可能性があるので、念のため少し余裕を見るなら -10 くらいしても良いが、
    // K-meansはその瞬間をターゲットにしているので、ジャストでOK。
    if (best_time > 0) {
        state.advance_to(best_time);
    }

    // メインループ
    // 基本的に best_time でほとんどの結合が終わるはずですが、
    // わずかに届かない場合などのために、Tまで少しずつ進める処理は残します
    ll dt = 1; // 結合フェーズに入ったら時間は細かく進める

    while(!state.is_goal()) {
        
        // 全グループに対して結合を試行
        for(ll g = 0; g < input.M; g++) {
            ClusterGroup &group = groups[g];
            
            // 可能な限り結合を繰り返す
            while(true) {
                double min_dist = 1e18;
                ll best_i = -1, best_j = -1;

                // グループ内で結合可能なペアを探索
                for (ll i_idx : group.cluster_indices) {
                    if (!state.clusters[i_idx].alive) continue;
                    
                    for (ll j_idx : group.cluster_indices) {
                        if (i_idx >= j_idx) continue;
                        if (!state.clusters[j_idx].alive) continue;

                        if (!state.clusters[i_idx].can_merge(state.clusters[j_idx])) continue;

                        // ここで「距離制限」を設けても良い
                        // 例: if (dist > 5000) continue; 
                        // 今回はベスト時刻に来ているはずなので、無条件で近い順に繋ぐ
                        
                        double dist = state.clusters[i_idx].gdistance(state.clusters[j_idx]);
                        if (dist < min_dist) {
                            min_dist = dist;
                            best_i = i_idx;
                            best_j = j_idx;
                        }
                    }
                }

                if (best_i != -1 && best_j != -1) {
                    // 結合実行（時刻は現在の state.current_time）
                    state.merge_clusters(best_i, best_j, state.current_time);
                } else {
                    // このグループではもう結合できるペアがない
                    break;
                }
            }
        }
        
        // 時間切れチェック
        if (state.current_time + dt >= input.T) break;
        
        // 時間を少し進める
        state.advance_to(state.current_time + dt);
    }

    output.print();
}


int main(){
    rnd::init(42); // 乱数初期化
    solve();
}