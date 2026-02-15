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

ll N, M, T, U;
vvl V;
vpl FP;
vpl dir = {{1, 0}, {0, 1}, {-1, 0}, {0, -1}};

const int MAX_DEPTH = 2;
const int MY_CANDIDATES = 100;

struct AIParams {
    ld wa, wb, wc, wd;
    AIParams() : wa(0.6), wb(0.6), wc(0.6), wd(0.6) {}
};
vector<AIParams> aiParams;
int currentTurn = 0;
TimePoint gameStartTime;

const ll TOTAL_TIME_LIMIT_MS = 1900;
const ll TIME_BUFFER_MS = 100;

int getBeamWidth() {
    if (M <= 2) return 200;
    if (M == 3) return 150;
    if (M == 4) return 100;
    if (M == 5) return 80;
    if (M == 6) return 60;
    if (M == 7) return 50;
    return 40;
}

int getAdaptiveBeamWidth() {
    return getBeamWidth();
}

// 高速化用バッファ
int visited[55][55];
int visited_token = 0;

struct State {
    vpl positions;
    vvl owner;
    vvl level;
    pl myFirstMove;
    ld score;
    vl tileCounts;
    
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
        tileCounts = vl(M, 0);
        rep(i, 0, N) rep(j, 0, N) if (owner[i][j] >= 0) tileCounts[owner[i][j]]++;
    }
    
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
    
    int countConnectedComponents(int player) const {
        visited_token++;
        int components = 0;
        static pl q[2500];
        rep(i, 0, N) {
            rep(j, 0, N) {
                if (owner[i][j] == player && visited[i][j] != visited_token) {
                    components++;
                    int head = 0, tail = 0;
                    q[tail++] = {i, j};
                    visited[i][j] = visited_token;
                    while(head < tail) {
                        auto [cx, cy] = q[head++];
                        for (auto [dx, dy] : dir) {
                            ll nx = cx + dx, ny = cy + dy;
                            if (nx >= 0 && nx < N && ny >= 0 && ny < N) {
                                if (owner[nx][ny] == player && visited[nx][ny] != visited_token) {
                                    visited[nx][ny] = visited_token;
                                    q[tail++] = {nx, ny};
                                }
                            }
                        }
                    }
                }
            }
        }
        return components;
    }
    
    ld evaluate() const {
        bool isManyEnemies = (M >= 4);
        bool isExpansionPhase = (currentTurn < (T / 2) );
        ld baseScore;
        
        if (isManyEnemies && isExpansionPhase) {
            /*ll maxAITiles = 0;
            rep(i, 1, M) chmax(maxAITiles, tileCounts[i]);
            if (maxAITiles == 0) return 1000.0;*/
            baseScore = (ld)tileCounts[0] ; // / maxAITiles;
        } else {
            vl scores = calcScores();            
            ll maxAI = 0;
            rep(i, 1, M) chmax(maxAI, scores[i]);
            if (maxAI == 0) return 1000.0;
            baseScore = (ld)scores[0] / maxAI;
        }
        
        int myComponents = countConnectedComponents(0);
        ld connectivityBonus = 0.0;
        if (myComponents > 0) {
            connectivityBonus = max(0.0, 0.05 * (4.0 - myComponents) / 3.0);
        }
        return baseScore * (1.0 + connectivityBonus);
    }
    
    bool operator>(const State& other) const { return score > other.score; }
};

struct Get {
    vpl TP, EP;
    vvl O, L;
    Get(){
        TP = vpl(M); EP = vpl(M);
        O = vvl(N, vl(N)); L = vvl(N, vl(N));
    }
    void Getinit(){
        rep(i,0,M) cin >> TP[i].first >> TP[i].second;
        rep(i,0,M) cin >> EP[i].first >> EP[i].second;
        rep(i,0,N) rep(j,0,N) cin >> O[i][j];
        rep(i,0,N) rep(j,0,N) cin >> L[i][j];
    }
};

vector<pl> getCandidatesForPlayer(const State& state, int player) {
    visited_token++;
    vpl reachable;
    static pl q[2500];
    int head = 0, tail = 0;
    q[tail++] = state.positions[player];
    visited[state.positions[player].first][state.positions[player].second] = visited_token;
    
    while(head < tail) {
        auto [cx, cy] = q[head++];
        reachable.push_back({cx, cy});
        for (auto [dx, dy] : dir) {
            ll nx = cx + dx, ny = cy + dy;
            if (nx >= 0 && nx < N && ny >= 0 && ny < N) {
                if (state.owner[nx][ny] == player && visited[nx][ny] != visited_token) {
                    visited[nx][ny] = visited_token;
                    q[tail++] = {nx, ny};
                }
            }
        }
    }
    
    int candidate_token = ++visited_token;
    vpl candidates;
    for (auto [rx, ry] : reachable) {
        if (visited[rx][ry] != candidate_token) {
            candidates.push_back({rx, ry});
            visited[rx][ry] = candidate_token;
        }
        for (auto [dx, dy] : dir) {
            ll nx = rx + dx, ny = ry + dy;
            if (nx >= 0 && nx < N && ny >= 0 && ny < N) {
                if (visited[nx][ny] != candidate_token) {
                    candidates.push_back({nx, ny});
                    visited[nx][ny] = candidate_token;
                }
            }
        }
    }
    
    // 他のプレイヤーがいる場所を除外
    rep(i, 0, M) if (i != player) {
        auto p = state.positions[i];
        rep(j, 0, candidates.size()) {
            if (candidates[j] == p) {
                candidates.erase(candidates.begin() + j);
                break;
            }
        }
    }

    vector<pair<ld, pl>> scored;
    bool isExpansionPhase = (currentTurn < T / 2);
    for (auto [x, y] : candidates) {
        if (state.owner[x][y] == player && state.level[x][y] >= U) continue;
        ld score = V[x][y];
        if (state.owner[x][y] == -1) score *= isExpansionPhase ? 10.0 : 1.5;
        else if (state.owner[x][y] == player) score *= isExpansionPhase ? 0.01 : 0.8;
        else score *= 2.0;
        scored.push_back({score, {x, y}});
    }
    sort(all(scored), greater<pair<ld, pl>>());
    vpl result;
    for (auto [sc, pos] : scored) result.push_back(pos);
    if (result.empty()) result.push_back(state.positions[player]);
    return result;
}

State simulate(const State& state, const vpl& moves) {
    State nextState = state;
    vpl targetPos = moves;
    
    // 競合解決を元のロジック（map使用）から変更せず再現
    map<pl, vector<int>> conflicts;
    rep(i, 0, M) conflicts[targetPos[i]].push_back(i);
    
    vb collected(M, false);
    for (auto& [pos, players] : conflicts) {
        if (players.size() >= 2) {
            auto [x, y] = pos;
            int cellOwner = state.owner[x][y];
            bool ownerPresent = false;
            if (cellOwner >= 0) {
                for (int p : players) if (p == cellOwner) { ownerPresent = true; break; }
            }
            if (ownerPresent) {
                for (int p : players) if (p != cellOwner) collected[p] = true;
            } else {
                for (int p : players) collected[p] = true;
            }
        }
    }
    
    rep(i, 0, M) {
        if (collected[i]) continue;
        auto [x, y] = targetPos[i];
        int cellOwner = state.owner[x][y];
        if (cellOwner == -1) {
            nextState.owner[x][y] = i;
            nextState.level[x][y] = 1;
            nextState.tileCounts[i]++;
        } else if (cellOwner == i) {
            if (nextState.level[x][y] < U) nextState.level[x][y]++;
        } else {
            nextState.level[x][y]--;
            if (nextState.level[x][y] == 0) {
                nextState.owner[x][y] = i;
                nextState.level[x][y] = 1;
                nextState.tileCounts[cellOwner]--;
                nextState.tileCounts[i]++;
            } else {
                collected[i] = true;
            }
        }
    }
    rep(i, 0, M) nextState.positions[i] = collected[i] ? state.positions[i] : targetPos[i];
    return nextState;
}

pl predictAIMove(const State& state, int player) {
    vpl candidates = getCandidatesForPlayer(state, player);
    AIParams& params = aiParams[player - 1];
    ld bestVal = -1e18;
    pl bestMove = state.positions[player];
    for (auto [x, y] : candidates) {
        ld val = 0.0;
        if (state.owner[x][y] == -1) val = V[x][y] * params.wa;
        else if (state.owner[x][y] == player) {
            if (state.level[x][y] < U) val = V[x][y] * params.wb;
        } else {
            val = V[x][y] * (state.level[x][y] == 1 ? params.wc : params.wd);
        }
        if (val > bestVal) { bestVal = val; bestMove = {x, y}; }
    }
    return bestMove;
}

pl beamSearch(const State& initialState) {
    vector<State> beam;
    beam.push_back(initialState);
    rep(depth, 0, MAX_DEPTH) {
        int BEAM_WIDTH = getAdaptiveBeamWidth();
        vector<State> nextBeam;
        for (const State& state : beam) {
            // ここで敵の手を1回だけ予測
            vpl allMoves(M);
            rep(i, 1, M) allMoves[i] = predictAIMove(state, i);
            
            vpl myCandidates = getCandidatesForPlayer(state, 0);
            if (myCandidates.size() > MY_CANDIDATES) myCandidates.resize(MY_CANDIDATES);
            
            for (pl move : myCandidates) {
                allMoves[0] = move;
                State nextState = simulate(state, allMoves);
                nextState.myFirstMove = (depth == 0) ? move : state.myFirstMove;
                nextState.score = nextState.evaluate();
                nextBeam.push_back(nextState);
            }
        }
        sort(all(nextBeam), greater<State>());
        if (nextBeam.size() > BEAM_WIDTH) nextBeam.resize(BEAM_WIDTH);
        beam = nextBeam;
    }
    return beam.empty() ? initialState.positions[0] : beam[0].myFirstMove;
}

void solve() {
    cin >> N >> M >> T >> U;
    V.assign(N, vl(N));
    rep(i, 0, N) rep(j, 0, N) cin >> V[i][j];
    FP.resize(M);
    rep(i, 0, M) cin >> FP[i].first >> FP[i].second;

    gameStartTime = Clock::now();
    aiParams.resize(M - 1);
    State currentState;
    rep(i, 0, M) {
        auto [x, y] = FP[i];
        currentState.owner[x][y] = i;
        currentState.level[x][y] = 1;
    }
    currentState.positions = FP;

    rep(turn, 0, T) {
        currentTurn = turn;
        State prevState = currentState;
        pl move = beamSearch(currentState);
        cout << move.first << " " << move.second << endl;
        Get g;
        g.Getinit();
        if (turn > 0) rep(i, 1, M) {
            vector<pl> cand = getCandidatesForPlayer(prevState, i);
            if (cand.size() < 2) continue;
            auto [mx, my] = g.TP[i];
            int cO = prevState.owner[mx][my], cL = prevState.level[mx][my];
            const ld lr = 0.02;
            AIParams& p = aiParams[i-1];
            
            // パラメータを更新
            if (cO == -1) p.wa += lr;
            else if (cO == i) { if (cL < U) p.wb += lr; }
            else if (cL == 1) p.wc += lr;
            else p.wd += lr;
            
            // 正規化（合計を一定に保つ）
            ld sum = p.wa + p.wb + p.wc + p.wd;
            if (sum > 0) {
                p.wa = p.wa / sum * 2.4;
                p.wb = p.wb / sum * 2.4;
                p.wc = p.wc / sum * 2.4;
                p.wd = p.wd / sum * 2.4;
            }
        }
        currentState.positions = g.EP;
        currentState.owner = g.O;
        currentState.level = g.L;
        currentState.tileCounts.assign(M, 0);
        rep(i, 0, N) rep(j, 0, N) if (currentState.owner[i][j] >= 0) currentState.tileCounts[currentState.owner[i][j]]++;
    }
}

int main() {
    cin.tie(0); ios::sync_with_stdio(0);
    solve();
    return 0;
}