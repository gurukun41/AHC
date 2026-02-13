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

const int MAX_DEPTH = 3;  // 探索深さ
const int MY_CANDIDATES = 10;  // 自分の候補手数
const int AI_CANDIDATES = 3;   // AIの候補手数

// 敵の数に応じてビーム幅を決定
int getBeamWidth() {
    // M=2: 200, M=3: 150, M=4: 100, M=5: 80, M=6: 60, M=7: 50, M=8: 40
    if (M <= 2) return 200;
    if (M == 3) return 150;
    if (M == 4) return 100;
    if (M == 5) return 80;
    if (M == 6) return 60;
    if (M == 7) return 50;
    return 40;  // M=8
}

// 盤面状態を表す構造体
struct State {
    vpl positions;  // 各プレイヤーの位置
    vvl owner;      // 各マスの所有者
    vvl level;      // 各マスのレベル
    pl myFirstMove; // 自分の最初の手
    ld score;       // 評価値
    
    State() {
        positions = vpl(M);
        owner = vvl(N, vl(N, -1));
        level = vvl(N, vl(N, 0));
        myFirstMove = {-1, -1};
        score = 0.0;
    }
    
    State(const vpl& pos, const vvl& o, const vvl& l) 
        : positions(pos), owner(o), level(l), myFirstMove({-1, -1}), score(0.0) {}
    
    // 各プレイヤーのスコアを計算
    vl calcScores() const {
        vl scores(M, 0);
        rep(i, 0, N) {
            rep(j, 0, N) {
                if (owner[i][j] >= 0) {
                    scores[owner[i][j]] += V[i][j] * level[i][j];
                }
            }
        }
        return scores;
    }
    
    // 評価値（プレイヤー0のスコア比率）
    ld evaluate() const {
        vl scores = calcScores();
        ll maxAI = 0;
        rep(i, 1, M) {
            chmax(maxAI, scores[i]);
        }
        if (maxAI == 0) return 1000.0;  // AIスコアが0の場合は高評価
        return (ld)scores[0] / maxAI;
    }
    
    bool operator<(const State& other) const {
        return score < other.score;
    }
    
    bool operator>(const State& other) const {
        return score > other.score;
    }
};

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
set<pl> getReachable(const pl& pos, const vvl& O, int player) {
    set<pl> reachable;
    queue<pl> q;
    reachable.insert(pos);
    q.push(pos);
    
    while (!q.empty()) {
        auto [x, y] = q.front();
        q.pop();
        
        for (auto [dx, dy] : dir) {
            ll nx = x + dx;
            ll ny = y + dy;
            if (nx >= 0 && nx < N && ny >= 0 && ny < N) {
                if (O[nx][ny] == player && reachable.find({nx, ny}) == reachable.end()) {
                    reachable.insert({nx, ny});
                    q.push({nx, ny});
                }
            }
        }
    }
    
    return reachable;
}

// 移動可能な候補を取得
vector<pl> getCandidatesForPlayer(const State& state, int player) {
    set<pl> reachable = getReachable(state.positions[player], state.owner, player);
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
    rep(i, 0, M) {
        if (i != player) {
            candidates.erase(state.positions[i]);
        }
    }
    
    // 評価値でソート
    vector<pair<ld, pl>> scored;
    for (auto [x, y] : candidates) {
        ld score = V[x][y];
        // 所有状況によって重み付け
        if (state.owner[x][y] == -1) {
            score *= 2.0;  // 未占領は高評価
        } else if (state.owner[x][y] == player) {
            if (state.level[x][y] < U) {
                score *= 0.8;  // 自陣強化は中評価
            } else {
                score *= 0.1;  // レベル上限は低評価
            }
        } else {
            score *= 1.5;  // 敵陣攻撃は高評価
        }
        scored.push_back({score, {x, y}});
    }
    
    sort(all(scored), greater<pair<ld, pl>>());
    
    vector<pl> result;
    for (auto [sc, pos] : scored) {
        result.push_back(pos);
    }
    
    return result;
}

// 状態遷移をシミュレート
State simulate(const State& state, const vpl& moves) {
    State nextState = state;
    
    // 1. 全員移動
    vpl targetPos = moves;
    
    // 2. 競合解決
    map<pl, vector<int>> conflicts;  // 位置 -> プレイヤーリスト
    rep(i, 0, M) {
        conflicts[targetPos[i]].push_back(i);
    }
    
    vb collected(M, false);  // 駒が回収されたか
    
    for (auto& [pos, players] : conflicts) {
        if (players.size() >= 2) {
            auto [x, y] = pos;
            int cellOwner = state.owner[x][y];
            
            // 所有者の駒がいる場合、それ以外を回収
            bool ownerPresent = false;
            if (cellOwner >= 0) {
                for (int p : players) {
                    if (p == cellOwner) {
                        ownerPresent = true;
                        break;
                    }
                }
            }
            
            if (ownerPresent) {
                for (int p : players) {
                    if (p != cellOwner) {
                        collected[p] = true;
                    }
                }
            } else {
                // 全員回収
                for (int p : players) {
                    collected[p] = true;
                }
            }
        }
    }
    
    // 3. 領土の更新
    rep(i, 0, M) {
        if (collected[i]) continue;
        
        auto [x, y] = targetPos[i];
        int cellOwner = state.owner[x][y];
        
        if (cellOwner == -1) {
            // 占領
            nextState.owner[x][y] = i;
            nextState.level[x][y] = 1;
        } else if (cellOwner == i) {
            // 強化
            if (nextState.level[x][y] < U) {
                nextState.level[x][y]++;
            }
        } else {
            // 攻撃
            nextState.level[x][y]--;
            if (nextState.level[x][y] == 0) {
                nextState.owner[x][y] = i;
                nextState.level[x][y] = 1;
            } else {
                collected[i] = true;
            }
        }
    }
    
    // 4. 駒の復帰
    rep(i, 0, M) {
        if (collected[i]) {
            nextState.positions[i] = state.positions[i];
        } else {
            nextState.positions[i] = targetPos[i];
        }
    }
    
    return nextState;
}

// AIプレイヤーの手を予測（簡易版：評価値が高い候補から選ぶ）
pl predictAIMove(const State& state, int player) {
    vector<pl> candidates = getCandidatesForPlayer(state, player);
    
    if (candidates.empty()) {
        return state.positions[player];
    }
    
    // 評価値が最も高い候補を選ぶ（問題文のアルゴリズムの簡易実装）
    return candidates[0];
}

// 全プレイヤーの手を決定してシミュレート
State simulateOneTurn(const State& state, pl myMove) {
    vpl allMoves(M);
    allMoves[0] = myMove;
    
    // AIプレイヤーの手を予測
    rep(i, 1, M) {
        allMoves[i] = predictAIMove(state, i);
    }
    
    return simulate(state, allMoves);
}

// ビームサーチで最善手を探索
pl beamSearch(const State& initialState) {
    int BEAM_WIDTH = getBeamWidth();  // 敵の数に応じたビーム幅
    
    vector<State> beam;
    beam.push_back(initialState);
    
    rep(depth, 0, MAX_DEPTH) {
        vector<State> nextBeam;
        
        for (const State& state : beam) {
            // 自分の候補手を取得
            vector<pl> myCandidates = getCandidatesForPlayer(state, 0);
            
            int limit = MY_CANDIDATES;
            if (myCandidates.size() > limit) {
                myCandidates.resize(limit);
            }
            
            if (myCandidates.empty()) {
                myCandidates.push_back(state.positions[0]);
            }
            
            // 各候補手をシミュレート
            for (pl move : myCandidates) {
                State nextState = simulateOneTurn(state, move);
                
                // 最初の手を記録
                if (depth == 0) {
                    nextState.myFirstMove = move;
                } else {
                    nextState.myFirstMove = state.myFirstMove;
                }
                
                // 評価値を計算
                nextState.score = nextState.evaluate();
                
                nextBeam.push_back(nextState);
            }
        }
        
        // 評価値でソートして上位BEAM_WIDTH個を残す
        sort(all(nextBeam), greater<State>());
        
        if (nextBeam.size() > BEAM_WIDTH) {
            nextBeam.resize(BEAM_WIDTH);
        }
        
        beam = nextBeam;
    }
    
    // 最も評価値が高い状態の最初の手を返す
    if (beam.empty()) {
        return initialState.positions[0];
    }
    
    return beam[0].myFirstMove;
}

// 最善手を探索
pl searchBestMove(const State& state) {
    return beamSearch(state);
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
    
    State currentState;
    
    // 初期状態の所有者とレベルを設定
    rep(i, 0, M) {
        auto [x, y] = FP[i];
        currentState.owner[x][y] = i;
        currentState.level[x][y] = 1;
    }
    currentState.positions = FP;
    
    rep(turn, 0, T) {
        // ゲーム木探索で最善手を決定
        pl move = searchBestMove(currentState);
        
        // 移動先を出力
        cout << move.first << " " << move.second << endl;
        
        // ターン結果を取得
        Get g;
        g.Getinit();
        
        // 状態を更新
        currentState.positions = g.EP;
        currentState.owner = g.O;
        currentState.level = g.L;
    }
}

// ビームサーチに変更
int main(){
    solve();
    return 0;
}