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
    
    // メインループ
    rep(step, 0, T) {
        // TODO: ここに戦略を実装
        // 行動1の例: 隣接頂点に移動
        // 行動2の例: 現在位置の木をストロベリーに変更
        
        // 仮の終了条件
        break;
    }
    
    // デバッグ用: 最終スコア計算
    ll score = 0;
    rep(i, 0, K) {
        score += shop_inventory[i].size();
    }
    cerr << "Score: " << score << endl;
    
    return 0;
}