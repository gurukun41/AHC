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
vb tree_type; // false: W(バニラ), true: R(ストロベリー)
vector<set<string>> shop_inventory; // 各ショップの在庫集合
ll current_pos; // 現在位置
ll prev_move_from; // 前回の行動1での移動元(-1は未実行)
string cone; // 手元のコーン
vvl dist; // 全点対最短距離

// 現在位置からtargetまでの最短経路を求める（prev_move_fromを考慮）
vl find_path(ll start, ll target, ll forbidden) {
    if (start == target) return {};
    
    vl parent(N, -1);
    queue<ll> q;
    q.push(start);
    parent[start] = start;
    
    while (!q.empty()) {
        ll v = q.front();
        q.pop();
        
        if (v == target) break;
        
        for (ll nv : graph[v]) {
            // 直前の移動元には最初の1手では戻れない
            if (v == start && nv == forbidden) continue;
            
            if (parent[nv] == -1) {
                parent[nv] = v;
                q.push(nv);
            }
        }
    }
    
    // 経路の復元
    vl path;
    ll cur = target;
    while (cur != start) {
        path.push_back(cur);
        cur = parent[cur];
    }
    reverse(all(path));
    return path;
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
    
    // 状態管理の初期化
    tree_type.assign(N, false);
    shop_inventory.resize(K);
    current_pos = 0;
    prev_move_from = -1;
    cone = "";
    
    // 全点対最短距離を計算
    dist = calc_all_pairs_shortest_path(N, graph);
    
    // メインループ
    rep(step, 0, T) {
        if (cone.empty()) {
            // コーンが空の場合：新しいアイスを作って運ぶ
            // 各ショップについて、まだ持っていない最小距離を探す
            ll best_shop = -1;
            ll best_dist = 1e18;
            
            rep(shop, 0, K) {
                // 現在地と同じショップは除外
                if (shop == current_pos && current_pos < K) continue;
                
                // 現在地からの距離dのアイスを持っているか確認
                ll d = dist[current_pos][shop];
                string ice(d, 'W');
                
                if (shop_inventory[shop].find(ice) == shop_inventory[shop].end()) {
                    // まだ持っていない
                    if (d < best_dist) {
                        best_dist = d;
                        best_shop = shop;
                    }
                }
            }
            
            if (best_shop == -1) {
                // すべてのショップが距離dのアイスを持っている
                // とりあえず最も近いショップを選ぶ
                rep(shop, 0, K) {
                    // 現在地と同じショップは除外
                    if (shop == current_pos && current_pos < K) continue;
                    
                    ll d = dist[current_pos][shop];
                    if (d < best_dist) {
                        best_dist = d;
                        best_shop = shop;
                    }
                }
            }
            
            // best_shopへの経路を求める
            vl path = find_path(current_pos, best_shop, prev_move_from);
            
            if (path.empty()) {
                // 経路が見つからない場合は終了
                break;
            }
            
            // 経路に従って移動
            ll next = path[0];
            cout << next << endl;
            
            // 状態更新
            prev_move_from = current_pos;
            current_pos = next;
            
            // 移動先がアイスクリームの木なら収穫、ショップなら納品
            if (current_pos >= K) {
                // アイスクリームの木
                cone += (tree_type[current_pos] ? "R" : "W");
            } else {
                // ショップ
                shop_inventory[current_pos].insert(cone);
                cone = "";
            }
            
        } else {
            // コーンにアイスがある場合：納品先を探す
            ll cone_size = cone.size();
            ll best_shop = -1;
            ll best_dist = 1e18;
            
            rep(shop, 0, K) {
                if (shop_inventory[shop].find(cone) == shop_inventory[shop].end()) {
                    // まだこのアイスを持っていない
                    ll d = dist[current_pos][shop];
                    if (d < best_dist) {
                        best_dist = d;
                        best_shop = shop;
                    }
                }
            }
            
            if (best_shop == -1) {
                // すべてのショップがこのアイスを持っている
                // 最も近いショップを選ぶ
                rep(shop, 0, K) {
                    ll d = dist[current_pos][shop];
                    if (d < best_dist) {
                        best_dist = d;
                        best_shop = shop;
                    }
                }
            }
            
            // best_shopへの経路を求める
            vl path = find_path(current_pos, best_shop, prev_move_from);
            
            if (path.empty()) {
                // 既にショップにいる場合
                shop_inventory[current_pos].insert(cone);
                cone = "";
                prev_move_from = -1;
                continue;
            }
            
            ll next = path[0];
            cout << next << endl;
            
            // 状態更新
            prev_move_from = current_pos;
            current_pos = next;
            
            // 移動先がアイスクリームの木なら収穫、ショップなら納品
            if (current_pos >= K) {
                // アイスクリームの木
                cone += (tree_type[current_pos] ? "R" : "W");
            } else {
                // ショップ
                shop_inventory[current_pos].insert(cone);
                cone = "";
            }
        }
    }
    
    // デバッグ用: 最終スコア計算
    ll score = 0;
    rep(i, 0, K) {
        score += shop_inventory[i].size();
    }
    cerr << "Score: " << score << endl;
    
    return 0;
}