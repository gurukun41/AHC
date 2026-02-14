#include <bits/stdc++.h>
#include <atcoder/all>
#include <chrono>
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
using Clock = chrono::high_resolution_clock;
using TimePoint = chrono::time_point<Clock>;
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

const int MAX_DEPTH = 2;  // 探索深さ
const int MY_CANDIDATES = 100;  // 自分の候補手数
const int AI_CANDIDATES = 100;   // AIの候補手数

// AIプレイヤーのパラメータ（学習用）
struct AIParams {
    ld wa, wb, wc, wd;  // 評価値の重み
    AIParams() : wa(0.6), wb(0.6), wc(0.6), wd(0.6) {}
};
vector<AIParams> aiParams;  // 各AIプレイヤーのパラメータ
int currentTurn = 0;  // 現在のターン数
TimePoint gameStartTime;  // ゲーム開始時刻
TimePoint turnStartTime;  // 現在の10ターン周期の開始時刻
const ll TOTAL_TIME_LIMIT_MS = 2000;  // 全体の制限時間(ミリ秒) = 2秒
const ll TIME_BUFFER_MS = 100;  // 安全バッファ(100ms)
int cachedBeamWidth = 0;  // キャッシュされたビーム幅（10ターンごとに更新）

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

// ゲーム開始からの経過時間を取得(ミリ秒)
ll getElapsedTimeMs() {
    auto now = Clock::now();
    return chrono::duration_cast<chrono::milliseconds>(now - gameStartTime).count();
}

// 残りの時間バジェットを取得(ミリ秒)
ll getRemainingBudgetMs() {
    return TOTAL_TIME_LIMIT_MS - TIME_BUFFER_MS - getElapsedTimeMs();
}

// 現在のターンで使える目標時間を計算(ミリ秒)
ll getTargetTimePerTurn() {
    ll remainingBudget = getRemainingBudgetMs();
    ll remainingTurns = T - currentTurn;
    if (remainingTurns <= 0) return 100;  // 最低保証
    
    // 残りターンで均等配分（ただし最大2000ms、最小100ms）
    ll targetTime = remainingBudget / remainingTurns;
    return max(100LL, min(2000LL, targetTime));
}

// 10ターン周期の開始からの経過時間を取得(ミリ秒)
ll getElapsedInCycleMs() {
    auto now = Clock::now();
    return chrono::duration_cast<chrono::milliseconds>(now - turnStartTime).count();
}

// 残り時間バジェットに応じてビーム幅を計算して更新（10ターンごとに呼ばれる）
void updateBeamWidth() {
    ll remainingBudget = getRemainingBudgetMs();
    int baseWidth = getBeamWidth();
    
    // 余裕がなくなった時に幅を小さくする
    if (remainingBudget > TOTAL_TIME_LIMIT_MS * 0.2) {
        // 十分な時間がある（20%以上残っている）：通常の幅
        cachedBeamWidth = baseWidth;
    } else if (remainingBudget > TOTAL_TIME_LIMIT_MS * 0.1) {
        // 時間が少なくなってきた（10-20%残っている）：半分に減らす
        cachedBeamWidth = baseWidth / 2;
    } else {
        // 時間切迫（10%未満）：大幅に減らす
        cachedBeamWidth = max(10, baseWidth / 4);
    }
}

// キャッシュされたビーム幅を取得
int getAdaptiveBeamWidth() {
    return cachedBeamWidth;
}

// 盤面状態を表す構造体
struct State {
    vpl positions;  // 各プレイヤーの位置
    vvl owner;      // 各マスの所有者
    vvl level;      // 各マスのレベル
    pl myFirstMove; // 自分の最初の手
    ld score;       // 評価値
    vl tileCounts;  // 各プレイヤーの所有マス数（差分更新用）
    
    State() {
        positions = vpl(M);
        owner = vvl(N, vl(N, -1));
        level = vvl(N, vl(N, 0));
        myFirstMove = {-1, -1};
        score = 0.0;
        tileCounts = vl(M, 0);
    }
    
    State(const vpl& pos, const vvl& o, const vvl& l) 
        : positions(pos), owner(o), level(l), myFirstMove({-1, -1}), score(0.0) {
        // マス数を計算
        tileCounts = vl(M, 0);
        rep(i, 0, N) {
            rep(j, 0, N) {
                if (owner[i][j] >= 0) {
                    tileCounts[owner[i][j]]++;
                }
            }
        }
    }
    
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
    
    // プレイヤーの領土の連結成分数を計算
    int countConnectedComponents(int player) const {
        vvb visited(N, vb(N, false));
        int components = 0;
        
        rep(i, 0, N) {
            rep(j, 0, N) {
                if (owner[i][j] == player && !visited[i][j]) {
                    components++;
                    // BFSで連結成分を探索
                    queue<pl> q;
                    q.push({i, j});
                    visited[i][j] = true;
                    
                    while (!q.empty()) {
                        auto [x, y] = q.front();
                        q.pop();
                        
                        for (auto [dx, dy] : dir) {
                            ll nx = x + dx;
                            ll ny = y + dy;
                            if (nx >= 0 && nx < N && ny >= 0 && ny < N) {
                                if (owner[nx][ny] == player && !visited[nx][ny]) {
                                    visited[nx][ny] = true;
                                    q.push({nx, ny});
                                }
                            }
                        }
                    }
                }
            }
        }
        
        return components;
    }
    
    // 評価値（敵が多い場合は前半マス数、少ない場合はスコア + 連結性ボーナス）
    ld evaluate() const {
        bool isManyEnemies = (M >= 4);  // 敵が多い場合
        bool isExpansionPhase = (currentTurn < T / 2);
        ld baseScore;
        
        if (isManyEnemies && isExpansionPhase) {
            // 敵が多く前半：マス目の数で評価
            ll maxAITiles = 0;
            rep(i, 1, M) {
                chmax(maxAITiles, tileCounts[i]);
            }
            if (maxAITiles == 0) return 1000.0;
            baseScore = (ld)tileCounts[0] / maxAITiles;
        } else {
            // それ以外：スコアで評価
            vl scores = calcScores();
            ll maxAI = 0;
            rep(i, 1, M) {
                chmax(maxAI, scores[i]);
            }
            if (maxAI == 0) return 1000.0;
            baseScore = (ld)scores[0] / maxAI;
        }
        
        // 連結性ボーナス：連結成分が少ないほど良い
        int myComponents = countConnectedComponents(0);
        ld connectivityBonus = 0.0;
        if (myComponents > 0) {
            // 連結成分が1つなら+5%、2つなら+2.5%、3つ以上は0%
            connectivityBonus = max(0.0, 0.05 * (4.0 - myComponents) / 3.0);
        }
        
        return baseScore * (1.0 + connectivityBonus);
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
    
    // 前半は陣地拡大を最優先
    bool isExpansionPhase = (currentTurn < T / 2);
    
    for (auto [x, y] : candidates) {
        // レベル上限の自陣マスは候補から完全除外
        if (state.owner[x][y] == player && state.level[x][y] >= U) {
            continue;
        }
        ld score = V[x][y];
        
        // 所有状況によって重み付け
        if (state.owner[x][y] == -1) {
            score *= isExpansionPhase ? 10.0 : 1.5;  // 未占領は前半で超高評価
        } else if (state.owner[x][y] == player) {
            if (state.level[x][y] < U) {
                score *= isExpansionPhase ? 0.01 : 0.8;  // 自陣強化は前半でほぼ除外
            } else {
                score *= isExpansionPhase ? 0.01 : 0.8; // レベル上限は低評価
            }
        } else {
            score *= 2.0;  // 敵陣攻撃は高評価
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
            nextState.tileCounts[i]++;  // マス数を更新
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
                nextState.tileCounts[cellOwner]--;  // 元の所有者のマス数を減らす
                nextState.tileCounts[i]++;          // 新しい所有者のマス数を増やす
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

// AIプレイヤーの手を予測（学習したパラメータを使用）
pl predictAIMove(const State& state, int player) {
    vector<pl> candidates = getCandidatesForPlayer(state, player);
    
    if (candidates.empty()) {
        return state.positions[player];
    }
    
    // 学習済みパラメータを使用
    AIParams& params = aiParams[player - 1];  // player 1-indexed, aiParams 0-indexed
    
    vector<pair<ld, pl>> scored;
    
    for (auto [x, y] : candidates) {
        ld evalValue = 0.0;
        
        if (state.owner[x][y] == -1) {
            // 誰の領土でもない場合
            evalValue = V[x][y] * params.wa;
        } else if (state.owner[x][y] == player) {
            // 自分の領土の場合
            if (state.level[x][y] < U) {
                evalValue = V[x][y] * params.wb;
            } else {
                evalValue = 0.0;  // レベル上限
            }
        } else {
            // 他のプレイヤーの領土の場合
            if (state.level[x][y] == 1) {
                evalValue = V[x][y] * params.wc;
            } else {
                evalValue = V[x][y] * params.wd;
            }
        }
        
        scored.push_back({evalValue, {x, y}});
    }
    
    // 評価値でソート（降順）
    sort(all(scored), greater<pair<ld, pl>>());
    
    // 貪欲行動を想定（確率1-εで最大評価値を選ぶ）
    // εは小さいと仮定して、常に最大評価値を選ぶ
    return scored[0].second;
}

// AIの実際の行動からパラメータを学習
void learnAIParams(const State& prevState, const State& currentState, int player, pl actualMove) {
    vector<pl> candidates = getCandidatesForPlayer(prevState, player);
    
    if (candidates.empty() || candidates.size() < 2) return;
    
    AIParams& params = aiParams[player - 1];
    
    // 実際に選ばれた手のマスの状態を確認
    auto [mx, my] = actualMove;
    int cellOwner = prevState.owner[mx][my];
    int cellLevel = prevState.level[mx][my];
    
    // 各候補の評価値を計算
    vector<pair<ld, pl>> scored;
    for (auto [x, y] : candidates) {
        ld evalValue = 0.0;
        
        if (prevState.owner[x][y] == -1) {
            evalValue = V[x][y] * params.wa;
        } else if (prevState.owner[x][y] == player) {
            if (prevState.level[x][y] < U) {
                evalValue = V[x][y] * params.wb;
            }
        } else {
            if (prevState.level[x][y] == 1) {
                evalValue = V[x][y] * params.wc;
            } else {
                evalValue = V[x][y] * params.wd;
            }
        }
        
        scored.push_back({evalValue, {x, y}});
    }
    
    sort(all(scored), greater<pair<ld, pl>>());
    
    // 実際の手が最高評価でない場合、パラメータを調整
    if (scored[0].second != actualMove) {
        // 学習率を軽く（εでランダム行動もあるため）
        const ld learningRate = 0.02;
        
        // 選ばれた手のタイプに応じてパラメータを増やす
        if (cellOwner == -1) {
            params.wa += learningRate;
        } else if (cellOwner == player) {
            if (cellLevel < U) {
                params.wb += learningRate;
            }
        } else {
            if (cellLevel == 1) {
                params.wc += learningRate;
            } else {
                params.wd += learningRate;
            }
        }
        
        // パラメータの正規化（合計を一定に保つ）
        ld sum = params.wa + params.wb + params.wc + params.wd;
        if (sum > 0) {
            params.wa = params.wa / sum * 5.0;
            params.wb = params.wb / sum * 5.0;
            params.wc = params.wc / sum * 5.0;
            params.wd = params.wd / sum * 5.0;
        }
    }
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
    vector<State> beam;
    beam.push_back(initialState);
    
    rep(depth, 0, MAX_DEPTH) {
        // 残り時間に応じてビーム幅を動的に調整
        int BEAM_WIDTH = getAdaptiveBeamWidth();
        
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
    
    gameStartTime = Clock::now();  // ゲーム開始時刻を記録
    
    // AIパラメータの初期化
    aiParams.resize(M - 1);  // AIプレイヤーの数
    
    State currentState;
    
    // 初期状態の所有者とレベルを設定
    rep(i, 0, M) {
        auto [x, y] = FP[i];
        currentState.owner[x][y] = i;
        currentState.level[x][y] = 1;
    }
    currentState.positions = FP;
    
    rep(turn, 0, T) {
        currentTurn = turn;  // グローバル変数を更新
        
        // 10ターンごとに時間を測定してビーム幅を更新
        if (turn % 10 == 0) {
            turnStartTime = Clock::now();  // ターン開始時刻を記録
            updateBeamWidth();  // ビーム幅を計算・キャッシュ
        }
        
        // 前の状態を保存（学習用）
        State prevState = currentState;
        
        // ゲーム木探索で最善手を決定
        pl move = searchBestMove(currentState);
        
        // 移動先を出力
        cout << move.first << " " << move.second << endl;
        
        // ターン結果を取得
        Get g;
        g.Getinit();
        
        // AIの実際の行動から学習（ターン2以降）
        if (turn > 0) {
            rep(i, 1, M) {
                learnAIParams(prevState, currentState, i, g.TP[i]);
            }
        }
        
        // 状態を更新
        currentState.positions = g.EP;
        currentState.owner = g.O;
        currentState.level = g.L;
    }
}

// ビームサーチ + AIパラメータ学習（フィードバックあり）
int main(){
    solve();
    return 0;
}