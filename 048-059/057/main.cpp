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

// 修正: used配列を参照で受け取るように変更
void decide_group(ClusterGroup &group, const vector<Cluster> &clusters, vector<bool> &used) {
    ll N = clusters.size();
    
    while (!group.is_complete()) {
        double min_dist = 1e18;
        ll best_idx = -1;
        
        // グループが空の場合、最初の1個はランダム（または未使の適当なもの）に決める
        if (group.cluster_indices.empty()) {
            // 単純に「まだ使われていない最初の原子」だと場所が偏るので、
            // 本来はランダムやK-means法が良いが、一旦バグ修正として「未使の先頭」で実装する
            // (改善するならここでランダムに未使のindexを選ぶ)
            for (ll i = 0; i < N; i++) {
                if (!used[i] && group.can_add(clusters[i].size)) {
                    best_idx = i;
                    break;
                }
            }
        } else {
            // 2個目以降は、重心に近いものを選ぶ
            for (ll i = 0; i < N; i++) {
                if (used[i]) continue;
                if (!group.can_add(clusters[i].size)) continue;
                
                // グループ内の全クラスタとの平均距離（重心距離のほうが計算が軽いが、現状維持）
                double total_dist = 0.0;
                for (ll idx : group.cluster_indices) {
                    total_dist += clusters[idx].gdistance(clusters[i]);
                }
                double dist = total_dist / group.cluster_indices.size();
                
                if (dist < min_dist) {
                    min_dist = dist;
                    best_idx = i;
                }
            }
        }
        
        if (best_idx != -1) {
            group.add_cluster(best_idx, clusters[best_idx].size);
            used[best_idx] = true;
        } else {
            break; 
        }
    }
}

void solve() {
    Input input;
    input.read();
    Output output(input);

    State state(input, output);
    
    // 修正: used配列をここで管理
    vector<bool> used(input.N, false);
    vector<ClusterGroup> groups(input.M, ClusterGroup(input));
    
    // グループ分けを実行
    for (ll i = 0; i < input.M; i++) {
        decide_group(groups[i], state.clusters, used);
    }

    // デバッグ: 全原子が割り振られたか確認
    int used_count = 0;
    for(bool u : used) if(u) used_count++;
    if(used_count != input.N) {
        cerr << "Warning: Not all atoms are assigned to groups!" << endl;
    }

    // 時間刻み（少し細かく見る）
    ll dt = 10; 

    // メインループ
    while(!state.is_goal()) {
        // 現在の時刻で、結合できるペアがある限り結合し続ける
        bool merged_any = false;
        
        for(ll g = 0; g < input.M; g++) {
            ClusterGroup &group = groups[g];
            
            // このグループ内で結合を実行
            // 1ステップで複数回結合してもよいのでループさせる
            while(true) {
                double min_dist = 1e18;
                ll best_i = -1, best_j = -1;

                // グループ内の総当たりで最小コストのペアを探す
                for (ll i_idx : group.cluster_indices) {
                    if (!state.clusters[i_idx].alive) continue; // 死んでいるクラスタは無視
                    
                    for (ll j_idx : group.cluster_indices) {
                        if (i_idx >= j_idx) continue;
                        if (!state.clusters[j_idx].alive) continue;

                        // マージ可能かチェック（サイズ超過など）
                        if (!state.clusters[i_idx].can_merge(state.clusters[j_idx])) continue;

                        double dist = state.clusters[i_idx].gdistance(state.clusters[j_idx]);
                        if (dist < min_dist) {
                            min_dist = dist;
                            best_i = i_idx;
                            best_j = j_idx;
                        }
                    }
                }

                // 見つかったペアがあれば結合
                // ここで閾値を設けて「遠すぎるなら今は結合しない」とするとスコアが上がるが、
                // まずは完成させるために無条件で結合する
                if (best_i != -1 && best_j != -1) {
                    state.merge_clusters(best_i, best_j, state.current_time);
                    merged_any = true;
                } else {
                    // このグループではもう結合できるペアがない
                    break;
                }
            }
        }
        
        // 時間切れチェック
        if (state.current_time + dt >= input.T) {
            // 時間が足りないので強制的に最後の処理をするか、諦める
            // 今回はループを抜けて終了
            break;
        }

        // 時間を進める
        state.advance_to(state.current_time + dt);
    }

    output.print();
}


int main(){
    solve();
}