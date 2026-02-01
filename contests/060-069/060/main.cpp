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
}

// 全点対最短距離を求める関数
// 計算量: O(N(N+M)) = O(N^2 + NM)
// BFSをN回実行するため、各BFSがO(N+M)でそれをN回行う
vvl calc_all_pairs_shortest_path(ll n, const vvl& g) {
    vvl dist(n, vl(n, 1e18)); // 初期値を無限大に設定
    
    // 各頂点を始点としてBFS
    rep(start, 0, n) {
        dist[start][start] = 0;
        queue<ll> q;
        q.push(start);
        
        while (!q.empty()) {
            ll v = q.front();
            q.pop();
            
            for (ll nv : g[v]) {
                if (dist[start][nv] == 1e18) {
                    dist[start][nv] = dist[start][v] + 1;
                    q.push(nv);
                }
            }
        }
    }
    
    return dist;
}

// グローバル変数
ll N, M, K, T;
vvl graph;
vl X, Y;
vvl dist; // 全点対最短距離

// 行動を表す構造体
struct Action {
    ll type; // 1: 行動1(移動), 2: 行動2(木の変更)
    ll target; // 行動1の場合は移動先頂点番号、行動2の場合は-1
    
    Action() : type(1), target(-1) {}
    Action(ll t, ll tg) : type(t), target(tg) {}
};

// 行動配列
vector<Action> actions;

// 行動配列を評価する関数（高速化版）
// 制限違反がある場合は-1を返す
// 制限を満たす場合はスコアを返す
inline ll evaluate_actions(const vector<Action>& acts) {
    // シミュレーション用の状態変数
    static vb tree_type(100, false);
    static vector<set<string>> shop_inventory(10);
    
    // 初期化
    fill(tree_type.begin(), tree_type.begin() + N, false);
    rep(i, 0, K) shop_inventory[i].clear();
    
    ll current_pos = 0;
    ll prev_move_from = -1;
    static string cone;
    cone.clear();
    
    ll acts_size = acts.size();
    for (ll step = 0; step < acts_size; step++) {
        const Action& act = acts[step];
        
        if (act.type == 1) {
            // 行動1: 移動
            ll next = act.target;
            
            // 制限チェック2: 直前の移動元に戻っていないか（高速化のため先にチェック）
            if (prev_move_from != -1 && next == prev_move_from) {
                return -1;
            }
            
            // 制限チェック1: 隣接頂点か
            bool is_adjacent = false;
            const vl& adj_list = graph[current_pos];
            ll adj_size = adj_list.size();
            for (ll i = 0; i < adj_size; i++) {
                if (adj_list[i] == next) {
                    is_adjacent = true;
                    break;
                }
            }
            if (!is_adjacent) return -1;
            
            // 移動実行
            prev_move_from = current_pos;
            current_pos = next;
            
            // 移動先での処理
            if (current_pos >= K) {
                // アイスクリームの木: 収穫
                cone += (tree_type[current_pos] ? 'R' : 'W');
            } else {
                // ショップ: 納品
                shop_inventory[current_pos].insert(cone);
                cone.clear();
            }
            
        } else {
            // 行動2: 木の変更
            // 制限チェック: 現在位置がバニラ味のアイスクリームの木か
            if (current_pos < K || tree_type[current_pos]) {
                return -1;
            }
            
            // 木の変更実行
            tree_type[current_pos] = true;
        }
    }
    
    // スコア計算
    ll score = 0;
    for (ll i = 0; i < K; i++) {
        score += shop_inventory[i].size();
    }
    
    return score;
}

// 近傍操作1: ランダムな位置の行動を変更
void neighbor_operation_1(vector<Action>& acts, ll pos) {
    if (acts[pos].type == 1) {
        // 行動1: 移動先を変更し、それ以降を再構築
        // pos-1番目とpos-2番目の行動1の位置を取得
        ll current_pos = 0;
        ll prev_move_from = -1;
        
        // pos直前の行動1を探す
        for (ll i = pos - 1; i >= 0; i--) {
            if (acts[i].type == 1) {
                current_pos = acts[i].target;
                // さらにその前の行動1を探す
                for (ll j = i - 1; j >= 0; j--) {
                    if (acts[j].type == 1) {
                        prev_move_from = acts[j].target;
                        break;
                    }
                }
                break;
            }
        }
        
        // pos番目の移動先を変更
        ll old_target = acts[pos].target;
        
        vl candidates;
        for (ll adj : graph[current_pos]) {
            if (prev_move_from == -1 || adj != prev_move_from) {
                candidates.push_back(adj);
            }
        }
        
        if (!candidates.empty()) {
            ll new_target = candidates[rnd::get(candidates.size())];
            
            // 移動先が同じならスキップ（何も変更しない）
            if (new_target == old_target) {
                return;
            }
            
            acts[pos] = Action(1, new_target);
            
            // pos+1以降を再構築
            prev_move_from = current_pos;
            current_pos = new_target;
            
            for (ll i = pos + 1; i < (ll)acts.size(); i++) {
                if (acts[i].type == 1) {
                    vl cand;
                    for (ll adj : graph[current_pos]) {
                        if (prev_move_from == -1 || adj != prev_move_from) {
                            cand.push_back(adj);
                        }
                    }
                    if (!cand.empty()) {
                        ll next = cand[rnd::get(cand.size())];
                        acts[i] = Action(1, next);
                        prev_move_from = current_pos;
                        current_pos = next;
                    }
                }
                // 行動2の場合は変更しない
            }
        }
    } else {
        // 行動2: 削除して以降を前に詰める
        // 最後の行動1の位置情報を先に取得
        ll last_pos = 0;
        ll last_prev = -1;
        
        // 最後の行動1を見つける
        for (ll i = (ll)acts.size() - 1; i >= 0; i--) {
            if (acts[i].type == 1) {
                last_pos = acts[i].target;
                // その1つ前の行動1を探す
                for (ll j = i - 1; j >= 0; j--) {
                    if (acts[j].type == 1) {
                        last_prev = acts[j].target;
                        break;
                    }
                }
                break;
            }
        }
        
        // 行動2を削除
        acts.erase(acts.begin() + pos);
        
        // 新しい行動1を追加
        vl candidates;
        for (ll adj : graph[last_pos]) {
            if (last_prev == -1 || adj != last_prev) {
                candidates.push_back(adj);
            }
        }
        
        if (!candidates.empty()) {
            ll next = candidates[rnd::get(candidates.size())];
            acts.push_back(Action(1, next));
        } else {
            acts.push_back(Action(1, graph[last_pos][0]));
        }
    }
}

// 近傍操作2: ランダムな位置の前に行動2を挿入
void neighbor_operation_2(vector<Action>& acts, ll pos) {
    if (acts[pos].type == 1) {
        // pos番目の1つ前に行動2を挿入
        acts.insert(acts.begin() + pos, Action(2, -1));
        // 最後の行動を削除
        acts.pop_back();
    }
    // 行動2の場合は何もしない
}

int main(){
    // 入力
    cin >> N >> M >> K >> T;
    
    // グラフの構築
    graph.resize(N);
    rep(i, 0, M) {
        ll a, b;
        cin >> a >> b;
        graph[a].push_back(b);
        graph[b].push_back(a);
    }
    
    // 座標情報(必要に応じて使用)
    X.resize(N);
    Y.resize(N);
    rep(i, 0, N) {
        cin >> X[i] >> Y[i];
    }
    
    // 全点対最短距離を計算
    dist = calc_all_pairs_shortest_path(N, graph);
    
    // 行動配列の初期化
    actions.resize(T);
    
    // 初期解の生成（グラフに沿った有効な行動列）
    ll current_pos = 0;
    ll prev_move_from = -1;
    
    rep(i, 0, T) {
        // 現在位置から隣接頂点を選択
        vl candidates;
        for (ll adj : graph[current_pos]) {
            // 直前の移動元でない頂点を候補に追加
            if (prev_move_from == -1 || adj != prev_move_from) {
                candidates.push_back(adj);
            }
        }
        
        // 候補からランダムに選択
        ll next = candidates[rnd::get(candidates.size())];
        
        actions[i] = Action(1, next);
        
        // 状態更新
        prev_move_from = current_pos;
        current_pos = next;
    }
    
    // 初期スコアの計算
    ll current_score = evaluate_actions(actions);
    cerr << "Initial Score: " << current_score << endl;
    
    // 焼きなまし法のメインループ
    double start_time = get_time();
    double time_limit = 1.8; // 制限時間
    
    ll iteration = 0;
    ll valid = 0;
    
    // 対数テーブルの準備
    static double log_table[65536];
    for (int i = 0; i < 65536; ++i) {
        log_table[i] = log((i + 0.5) / 65536.0);
    }

    
    while (true) {
        if (iteration % 200 == 0) {
            double elapsed = get_time() - start_time;
            if (elapsed >= time_limit) {
                break;
            }
            
            // 温度パラメータの計算
            double time_ratio = elapsed / time_limit;
            const double T0 = 1000.0;  // 初期温度
            const double T1 = 1.0;    // 最終温度
            double heat = T0 * pow(T1 / T0, time_ratio);
            
            if (iteration % 100 == 0) {
                cerr << "Time: " << fixed << setprecision(3) << elapsed 
                     << "s, Iter: " << iteration 
                     << ", Score: " << current_score 
                     << ", Heat: " << heat << endl;
            }
        }
        iteration++;
        
        // ランダムに位置を選択
        ll pos = rnd::get(T);
        
        // ランダムに操作を選択（操作1または操作2）
        ll operation = rnd::get(2);
        
        // 現在の状態を保存
        vector<Action> old_actions = actions;
        ll old_score = current_score;
        
        // 近傍操作を適用
        if (operation == 0) {
            neighbor_operation_1(actions, pos);
        } else {
            neighbor_operation_2(actions, pos);
        }
        
        // 変更がない場合はスキップ
        if (actions[pos].target == old_actions[pos].target && 
            actions[pos].type == old_actions[pos].type) {
            continue;
        }
        
        // 新しいスコアを計算
        ll new_score = evaluate_actions(actions);
        
        // 焼きなまし法による受理判定
        double elapsed = get_time() - start_time;
        double time_ratio = elapsed / time_limit;
        const double T0 = 100.0;
        const double T1 = 1.0;
        double heat = T0 * pow(T1 / T0, time_ratio);
        
        double add = heat * log_table[iteration % 65536]; // 最大化
        
        if (new_score >= 0 && (double)(new_score - old_score) >= add) {
            // 改善した場合、または確率的に受理
            current_score = new_score;
            valid++;
        } else {
            // 受理しない場合は元に戻す
            actions = old_actions;
        }
    }
    
    cerr << "Finished iteration: " << iteration << endl;
    cerr << "Accept ratio: " << fixed << setprecision(3) 
         << ((double)valid / iteration) << endl;
    
    // 最終解の出力
    rep(i, 0, T) {
        if (actions[i].type == 1) {
            cout << actions[i].target << endl;
        } else {
            cout << -1 << endl;
        }
    }
    
    // 最終スコア
    cerr << "Final Score: " << current_score << endl;
    
    return 0;
}