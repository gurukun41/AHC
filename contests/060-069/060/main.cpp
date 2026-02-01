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

// 行動配列を評価する関数
// 制限違反がある場合は-1を返す
// 制限を満たす場合はスコアを返す
ll evaluate_actions(const vector<Action>& acts) {
    // シミュレーション用の状態変数
    vb tree_type(N, false); // false: W(バニラ), true: R(ストロベリー)
    vector<set<string>> shop_inventory(K);
    ll current_pos = 0;
    ll prev_move_from = -1;
    string cone = "";
    
    for (ll step = 0; step < (ll)acts.size(); step++) {
        const Action& act = acts[step];
        
        if (act.type == 1) {
            // 行動1: 移動
            ll next = act.target;
            
            // 制限チェック1: 隣接頂点か
            bool is_adjacent = false;
            for (ll adj : graph[current_pos]) {
                if (adj == next) {
                    is_adjacent = true;
                    break;
                }
            }
            if (!is_adjacent) return -1;
            
            // 制限チェック2: 直前の移動元に戻っていないか
            if (prev_move_from != -1 && next == prev_move_from) {
                return -1;
            }
            
            // 移動実行
            prev_move_from = current_pos;
            current_pos = next;
            
            // 移動先での処理
            if (current_pos >= K) {
                // アイスクリームの木: 収穫
                cone += (tree_type[current_pos] ? "R" : "W");
            } else {
                // ショップ: 納品
                shop_inventory[current_pos].insert(cone);
                cone = "";
            }
            
        } else if (act.type == 2) {
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
    rep(i, 0, K) {
        score += shop_inventory[i].size();
    }
    
    return score;
}

// 近傍操作1: ランダムな位置の行動を変更
void neighbor_operation_1(vector<Action>& acts, ll pos) {
    if (acts[pos].type == 1) {
        // 行動1: 移動先を変更し、それ以降を再構築
        // まず現在のpos-1までをシミュレート
        ll current_pos = 0;
        ll prev_move_from = -1;
        
        for (ll i = 0; i < pos; i++) {
            if (acts[i].type == 1) {
                prev_move_from = current_pos;
                current_pos = acts[i].target;
            }
            // 行動2は位置に影響しない
        }
        
        // pos番目の移動先を変更
        vl candidates;
        for (ll adj : graph[current_pos]) {
            if (prev_move_from == -1 || adj != prev_move_from) {
                candidates.push_back(adj);
            }
        }
        
        if (!candidates.empty()) {
            ll new_target = candidates[rand() % candidates.size()];
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
                        ll next = cand[rand() % cand.size()];
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
        acts.erase(acts.begin() + pos);
        
        // 最後に新しい行動を追加
        // pos-1までをシミュレートして状態を復元
        ll current_pos = 0;
        ll prev_move_from = -1;
        
        for (ll i = 0; i < (ll)acts.size(); i++) {
            if (acts[i].type == 1) {
                prev_move_from = current_pos;
                current_pos = acts[i].target;
            }
        }
        
        // 新しい行動1を追加
        vl candidates;
        for (ll adj : graph[current_pos]) {
            if (prev_move_from == -1 || adj != prev_move_from) {
                candidates.push_back(adj);
            }
        }
        
        if (!candidates.empty()) {
            ll next = candidates[rand() % candidates.size()];
            acts.push_back(Action(1, next));
        } else {
            acts.push_back(Action(1, graph[current_pos][0]));
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
        ll next = candidates[rand() % candidates.size()];
        
        actions[i] = Action(1, next);
        
        // 状態更新
        prev_move_from = current_pos;
        current_pos = next;
    }
    
    // 初期スコアの計算
    ll current_score = evaluate_actions(actions);
    cerr << "Initial Score: " << current_score << endl;
    
    // 山登り法のメインループ
    ll iteration = 0;
    const ll max_iteration = 3000;
    
    while (iteration < max_iteration) {
        iteration++;
        
        // ランダムに位置を選択
        ll pos = rand() % T;
        
        // ランダムに操作を選択（操作1または操作2）
        ll operation = rand() % 2;
        
        // 現在の状態を保存
        vector<Action> old_actions = actions;
        ll old_score = current_score;
        
        // 近傍操作を適用
        if (operation == 0) {
            neighbor_operation_1(actions, pos);
        } else {
            neighbor_operation_2(actions, pos);
        }
        
        // 新しいスコアを計算
        ll new_score = evaluate_actions(actions);
        
        // スコアが改善したか、または制限違反がない場合に採用
        if (new_score > old_score) {
            // 改善した場合は採用
            current_score = new_score;
            if (iteration % 10000 == 0) {
                cerr << "Iteration: " << iteration << ", Score: " << current_score << endl;
            }
        } else {
            // 改善しなかった場合は元に戻す
            actions = old_actions;
        }
    }
    
    cerr << "Finished iteration: " << iteration << endl;
    
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