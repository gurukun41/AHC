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

ll N, M, T, U;  // 盤面のサイズ、プレイヤーの数、ターン数、レベル上限
vvl V;          // 各マスの価値
vpl FP;         // 各プレイヤーの初期位置

vpl dir = {{1, 0}, {0, 1}, {-1, 0}, {0, -1}}; // 移動方向

struct Get {
    vpl TP;     // 移動先として選んだ点
    vpl EP;     // ターン終了後の実際の点
    vvl O;      // 各マスの所有者
    vvl L;      // 各マスのレベル
    Get(){
        TP = vpl(M);
        EP = vpl(M);
        O = vvl(N, vl(N));
        L = vvl(N, vl(N));

    }

    // ターンごとの入力を取得
    void Getinit(){
        rep(i,0,M) {
            ll tx, ty;
            cin >> tx >> ty;
            TP[i] = {tx, ty};
        }
        rep(i,0,M) {
           ll ex, ey;
           cin >> ex >> ey;
           EP[i] = {ex, ey};
        }
        rep(i,0,N) {
            rep(j,0,N) {
                cin >> O[i][j];
            }
        }
        rep(i,0,N) {
            rep(j,0,N) {
                cin >> L[i][j];
            }
        }
    }
};

// 到達可能な領土をBFSで探索
set<pl> getReachable(vpl& EP, vvl& O) {
    set<pl> reachable;
    queue<pl> q;
    pl start = EP[0];
    reachable.insert(start);
    q.push(start);
    
    while (!q.empty()) {
        auto [x, y] = q.front();
        q.pop();
        
        for (auto [dx, dy] : dir) {
            ll nx = x + dx;
            ll ny = y + dy;
            if (nx >= 0 && nx < N && ny >= 0 && ny < N) {
                if (O[nx][ny] == 0 && reachable.find({nx, ny}) == reachable.end()) {
                    reachable.insert({nx, ny});
                    q.push({nx, ny});
                }
            }
        }
    }
    
    return reachable;
}

// 移動可能な候補を取得
set<pl> getCandidates(vpl& EP, vvl& O) {
    set<pl> reachable = getReachable(EP, O);
    set<pl> candidates = reachable;
    
    // 到達可能領土に隣接するマスも候補に追加
    for (auto [x, y] : reachable) {
        for (auto [dx, dy] : dir) {
            ll nx = x + dx;
            ll ny = y + dy;
            if (nx >= 0 && nx < N && ny >= 0 && ny < N) {
                candidates.insert({nx, ny});
            }
        }
    }
    
    // 他のプレイヤーがいる場所は除外
    rep(i, 1, M) {
        candidates.erase(EP[i]);
    }
    
    return candidates;
}

// 最も価値が高い非自陣地マスを選択
pl selectBestMove(set<pl>& candidates, vvl& O) {
    ll bestValue = -1;
    pl bestMove = {-1, -1};
    
    for (auto [x, y] : candidates) {
        // 自分の陣地ではない場所のみ考慮
        if (O[x][y] != 0) {
            if (V[x][y] > bestValue) {
                bestValue = V[x][y];
                bestMove = {x, y};
            }
        }
    }
    
    // 非自陣地が見つからない場合は、候補の中から最も価値が高い場所
    if (bestMove.first == -1) {
        for (auto [x, y] : candidates) {
            if (V[x][y] > bestValue) {
                bestValue = V[x][y];
                bestMove = {x, y};
            }
        }
    }
    
    return bestMove;
}

void GetFirstInput(){
    cin >> N >> M >> T >> U;
    V = vvl(N, vl(N));
    rep(i,0,N) {
        rep(j,0,N) {
            cin >> V[i][j];
        }
    }
    FP = vpl(M);
    rep(i,0,M) {
        ll fx, fy;
        cin >> fx >> fy;
        FP[i] = {fx, fy};
    }
}

void solve(){
    GetFirstInput();
    
    Get g;
    // 初期状態の所有者とレベルを設定
    rep(i, 0, N) {
        rep(j, 0, N) {
            g.O[i][j] = -1;
            g.L[i][j] = 0;
        }
    }
    rep(i, 0, M) {
        auto [x, y] = FP[i];
        g.O[x][y] = i;
        g.L[x][y] = 1;
    }
    g.EP = FP;
    
    rep(turn, 0, T) {
        // 移動可能な候補を取得
        set<pl> candidates = getCandidates(g.EP, g.O);
        
        // 最善の移動先を選択
        pl move = selectBestMove(candidates, g.O);
        
        // 移動先を出力
        cout << move.first << " " << move.second << endl;
        
        // ターン結果を取得
        g.Getinit();
    }
}

// 到達可能な場所の中で最も価値の高いマスを選ぶ戦略
int main(){
    solve();
    return 0;
}