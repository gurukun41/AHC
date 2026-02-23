#pragma GCC optimize("O3")
#pragma GCC optimize("unroll-loops")
#pragma GCC target("avx2,bmi,bmi2,lzcnt,popcnt")

#include <iostream>
#include <vector>
#include <chrono>
#include <algorithm>
#include <unordered_set>

using namespace std;
using ll = long long;
using pl = pair<ll, ll>;
using vpl = vector<pl>;
using vvl = vector<vector<ll>>;
using Clock = chrono::high_resolution_clock;
using TimePoint = chrono::time_point<Clock>;

// 制約
constexpr int MAX_N = 10;
constexpr int MAX_M = 8;

// グローバル変数
ll N, M, T, U;
int V[MAX_N][MAX_N];
vpl FP;
constexpr int dir_x[] = {1, 0, -1, 0};
constexpr int dir_y[] = {0, 1, 0, -1};

// --- 軽量乱数生成 (XorShift64) ---
uint64_t rng64() {
    static uint64_t x = 88172645463325252ULL;
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    return x;
}

// ハッシュ用テーブル (サイズを最小化してキャッシュ効率を上げる)
uint64_t z_owner[MAX_N][MAX_N][MAX_M + 1]; // owner: -1〜7 -> index 0〜8
uint64_t z_level[MAX_N][MAX_N][128];      // level: 0〜127
uint64_t z_pos[MAX_M][MAX_N][MAX_N];

void initZobrist() {
    for(int i=0; i<MAX_N; ++i) for(int j=0; j<MAX_N; ++j) {
        for(int k=0; k<=MAX_M; ++k) z_owner[i][j][k] = rng64();
        for(int k=0; k<128; ++k) z_level[i][j][k] = rng64();
        for(int m=0; m<MAX_M; ++m) z_pos[m][i][j] = rng64();
    }
}

// AIパラメータ
struct AIParams {
    double wa = 0.6, wb = 0.6, wc = 0.6, wd = 0.6;
};
AIParams aiParams[MAX_M];
int currentTurn = 0;
TimePoint gameStartTime;

const ll TOTAL_TIME_LIMIT_MS = 1900;

int getAdaptiveBeamWidth() {
    if (M <= 2) return 300;
    if (M == 3) return 250;
    if (M == 4) return 200;
    if (M == 5) return 150;
    if (M == 6) return 120;
    if (M == 7) return 100;
    return 80;
}

// 高速化用バッファ
int visited[MAX_N][MAX_N];
int visited_token = 0;

// ゲーム状態
struct State {
    int8_t owner[MAX_N][MAX_N];
    int8_t level[MAX_N][MAX_N];
    int current_scores[MAX_M];  // 差分更新用スコアキャッシュ
    uint64_t hash;
    double score;
    int8_t pos_x[MAX_M];
    int8_t pos_y[MAX_M];
    int8_t tileCounts[MAX_M];
    int8_t first_move_x;
    int8_t first_move_y;

    State() {
        first_move_x = -1; first_move_y = -1;
        score = 0.0; hash = 0;
        for(int i = 0; i < MAX_M; ++i) {
            pos_x[i] = 0; pos_y[i] = 0;
            tileCounts[i] = 0; current_scores[i] = 0;
        }
        for(int i = 0; i < MAX_N; ++i) {
            for(int j = 0; j < MAX_N; ++j) {
                owner[i][j] = -1; level[i][j] = 0;
            }
        }
    }

    void computeHash() {
        hash = 0;
        for (int i = 0; i < N; ++i) {
            for (int j = 0; j < N; ++j) {
                hash ^= z_owner[i][j][owner[i][j] + 1];
                hash ^= z_level[i][j][level[i][j]];
            }
        }
        for (int i = 0; i < M; ++i) {
            hash ^= z_pos[i][pos_x[i]][pos_y[i]];
        }
    }

    pl getFirstMove() const { return {first_move_x, first_move_y}; }
    void setFirstMove(pl move) { first_move_x = move.first; first_move_y = move.second; }

    int countConnectedComponents(int player) const {
        visited_token++;
        int components = 0;
        static int8_t q[128];
        for(int i=0; i<N; ++i) {
            for(int j=0; j<N; ++j) {
                if (owner[i][j] == player && visited[i][j] != visited_token) {
                    components++;
                    int head = 0, tail = 0;
                    q[tail++] = i * 10 + j;
                    visited[i][j] = visited_token;
                    while(head < tail) {
                        int pos = q[head++];
                        int cx = pos / 10, cy = pos % 10;
                        for (int d=0; d<4; ++d) {
                            int nx = cx + dir_x[d], ny = cy + dir_y[d];
                            if (nx >= 0 && nx < N && ny >= 0 && ny < N) {
                                if (owner[nx][ny] == player && visited[nx][ny] != visited_token) {
                                    visited[nx][ny] = visited_token;
                                    q[tail++] = nx * 10 + ny;
                                }
                            }
                        }
                    }
                }
            }
        }
        return components;
    }

    double evaluateBase() const {
        if (M >= 4 && currentTurn < T / 2) {
            return (double)tileCounts[0];
        } else {
            int maxAI = 0;
            for(int i = 1; i < M; ++i) if(current_scores[i] > maxAI) maxAI = current_scores[i];
            if (maxAI == 0) return 1000.0;
            return (double)current_scores[0] / maxAI;
        }
    }

    void applyHeavyEvaluation() {
        int myComponents = countConnectedComponents(0);
        double connectivityBonus = 0.0;
        if (myComponents > 0) {
            connectivityBonus = max(0.0, 0.05 * (4.0 - myComponents) / 3.0);
        }
        score = score * (1.0 + connectivityBonus);
    }

    // 実際の盤面情報から状態を同期
    void syncFromActual(const vvl& O, const vvl& L, const vpl& EP) {
        for(int i=0; i<M; ++i) {
            pos_x[i] = EP[i].first; pos_y[i] = EP[i].second;
            tileCounts[i] = 0; current_scores[i] = 0;
        }
        for(int i=0; i<N; ++i) {
            for(int j=0; j<N; ++j) {
                owner[i][j] = O[i][j]; level[i][j] = L[i][j];
                if(owner[i][j] >= 0) {
                    tileCounts[owner[i][j]]++;
                    current_scores[owner[i][j]] += V[i][j] * level[i][j];
                }
            }
        }
        computeHash();
    }
};

// 候補手用構造体
struct MyCand { double sc; int8_t x, y; };

// 候補手を列挙（メモリアロケーションなし、パスを分けて正しくBFSする）
int getCandidates(const State& state, int player, MyCand out_cands[]) {
    visited_token++;
    int8_t q[128];
    int head = 0, tail = 0;
    
    int8_t px = state.pos_x[player], py = state.pos_y[player];
    q[tail++] = px * 10 + py;
    visited[px][py] = visited_token;
    
    // パス1: 到達可能な自領土をBFSで全探索
    while(head < tail) {
        int pos = q[head++];
        int cx = pos / 10, cy = pos % 10;
        
        for(int d=0; d<4; ++d) {
            int nx = cx + dir_x[d], ny = cy + dir_y[d];
            if(nx>=0 && nx<N && ny>=0 && ny<N) {
                if (state.owner[nx][ny] == player && visited[nx][ny] != visited_token) {
                    visited[nx][ny] = visited_token;
                    q[tail++] = nx * 10 + ny;
                }
            }
        }
    }
    
    // パス2: 到達可能な自領土とその隣接マスを候補にする
    int cand_token = ++visited_token;
    int8_t cands[256];
    int cands_sz = 0;
    
    for(int i = 0; i < tail; ++i) {
        int pos = q[i];
        int cx = pos / 10, cy = pos % 10;
        
        if (visited[cx][cy] != cand_token) {
            cands[cands_sz++] = pos;
            visited[cx][cy] = cand_token;
        }
        
        for(int d=0; d<4; ++d) {
            int nx = cx + dir_x[d], ny = cy + dir_y[d];
            if(nx>=0 && nx<N && ny>=0 && ny<N) {
                if (visited[nx][ny] != cand_token) {
                    cands[cands_sz++] = nx * 10 + ny;
                    visited[nx][ny] = cand_token;
                }
            }
        }
    }
    
    bool has_other[MAX_N][MAX_N] = {false};
    for(int i=0; i<M; ++i) if (i != player) has_other[state.pos_x[i]][state.pos_y[i]] = true;
    
    int strongestEnemy = -1, maxEnemyScore = -1;
    if (player == 0) {
        for(int i=1; i<M; ++i) {
            if (state.current_scores[i] > maxEnemyScore) {
                maxEnemyScore = state.current_scores[i];
                strongestEnemy = i;
            }
        }
    }
    
    bool isExpansionPhase = (currentTurn < T / 2);
    int out_sz = 0;
    
    for(int i=0; i<cands_sz; ++i) {
        int cx = cands[i] / 10, cy = cands[i] % 10;
        if (has_other[cx][cy]) continue;
        if (state.owner[cx][cy] == player && state.level[cx][cy] >= U) continue;
        
        double score = V[cx][cy];
        int o = state.owner[cx][cy];
        
        if (player == 0) {
            if (o == -1) score *= isExpansionPhase ? 10.0 : 1.5;
            else if (o == 0) score *= isExpansionPhase ? 0.01 : 0.8;
            else {
                score *= 2.0;
                if (o == strongestEnemy) score *= 300.0;
            }
        }
        
        out_cands[out_sz++] = {score, (int8_t)cx, (int8_t)cy};
    }
    
    if (out_sz == 0) {
        out_cands[out_sz++] = {0.0, px, py};
    } else if (player == 0) {
        // ソートもインプレイスで行う（自分のみ）
        sort(out_cands, out_cands + out_sz, [](const MyCand& a, const MyCand& b) { return a.sc > b.sc; });
    }
    return out_sz;
}

// AIの行動予測（共通関数を利用して軽量に）
pl predictAIMove(const State& state, int player) {
    MyCand cands[256];
    int cands_sz = getCandidates(state, player, cands);
    
    const AIParams& params = aiParams[player - 1];
    double bestVal = -1e18;
    pl bestMove = {state.pos_x[player], state.pos_y[player]};
    
    for(int i=0; i<cands_sz; ++i) {
        int cx = cands[i].x, cy = cands[i].y;
        double val = 0.0;
        int o = state.owner[cx][cy];
        
        if (o == -1) val = V[cx][cy] * params.wa;
        else if (o == player) {
            if (state.level[cx][cy] < U) val = V[cx][cy] * params.wb;
        } else {
            val = V[cx][cy] * (state.level[cx][cy] == 1 ? params.wc : params.wd);
        }
        
        if (val > bestVal) { bestVal = val; bestMove = {cx, cy}; }
    }
    return bestMove;
}

// 状態遷移（ポインタで受け取りインプレイス更新）
void simulate_into(const State& state, const pl moves[], State& nextState) {
    nextState = state; // 高速な memcpy 相当
    bool collected[MAX_M] = {false};
    
    for (int i = 0; i < M; ++i) {
        int cx = moves[i].first, cy = moves[i].second;
        int players_on_cell = 0;
        int conflict_players[MAX_M];
        bool ownerPresent = false;
        int cellOwner = state.owner[cx][cy];
        
        for (int j = 0; j < M; ++j) {
            if (moves[j].first == cx && moves[j].second == cy) {
                conflict_players[players_on_cell++] = j;
                if (j == cellOwner) ownerPresent = true;
            }
        }
        
        if (players_on_cell >= 2) {
            if (ownerPresent) {
                for (int k = 0; k < players_on_cell; ++k) {
                    int p = conflict_players[k];
                    if (p != cellOwner) collected[p] = true;
                }
            } else {
                for (int k = 0; k < players_on_cell; ++k) collected[conflict_players[k]] = true;
            }
        }
    }
    
    for (int i = 0; i < M; ++i) {
        if (collected[i]) continue;
        int x = moves[i].first, y = moves[i].second;
        int oldOwner = state.owner[x][y];
        int oldLevel = state.level[x][y];
        
        nextState.hash ^= z_owner[x][y][oldOwner + 1];
        nextState.hash ^= z_level[x][y][oldLevel];
        
        if (oldOwner == -1) {
            nextState.owner[x][y] = i; nextState.level[x][y] = 1;
            nextState.tileCounts[i]++;
            nextState.current_scores[i] += V[x][y];
        } else if (oldOwner == i) {
            if (nextState.level[x][y] < U) {
                nextState.level[x][y]++;
                nextState.current_scores[i] += V[x][y];
            }
        } else {
            nextState.level[x][y]--;
            nextState.current_scores[oldOwner] -= V[x][y];
            if (nextState.level[x][y] == 0) {
                nextState.owner[x][y] = i; nextState.level[x][y] = 1;
                nextState.tileCounts[oldOwner]--; nextState.tileCounts[i]++;
                nextState.current_scores[i] += V[x][y];
            } else collected[i] = true;
        }
        
        nextState.hash ^= z_owner[x][y][nextState.owner[x][y] + 1];
        nextState.hash ^= z_level[x][y][nextState.level[x][y]];
    }
    
    for (int i = 0; i < M; ++i) {
        nextState.hash ^= z_pos[i][state.pos_x[i]][state.pos_y[i]];
        if (!collected[i]) {
            nextState.pos_x[i] = moves[i].first;
            nextState.pos_y[i] = moves[i].second;
        }
        nextState.hash ^= z_pos[i][nextState.pos_x[i]][nextState.pos_y[i]];
    }
}

// グローバルメモリプール（vectorの代わり）
const int MAX_BEAM_NODES = 50000;
State beamPool[MAX_BEAM_NODES];
State nextBeamPool[MAX_BEAM_NODES];

pl beamSearch(const State& initialState) {
    beamPool[0] = initialState;
    int beamSize = 1;
    pl best_move = {initialState.pos_x[0], initialState.pos_y[0]};
    
    double elapsed_ms = chrono::duration_cast<chrono::milliseconds>(Clock::now() - gameStartTime).count();
    double turn_time_limit = (TOTAL_TIME_LIMIT_MS - elapsed_ms) / (T - currentTurn);
    auto turn_start = Clock::now();
    
    int depth = 0;
    const int LIMIT_DEPTH = min(50, (int)(T - currentTurn));
    
    while (depth < LIMIT_DEPTH) {
        int BEAM_WIDTH = getAdaptiveBeamWidth();
        int nextBeamSize = 0;
        unordered_set<uint64_t> seenHashes;
        seenHashes.reserve(BEAM_WIDTH * 50); // メモリ再確保防止
        bool timeout = false;
        int time_check_counter = 0;

        for (int i = 0; i < beamSize; ++i) {
            // 時間計測を間引いて高速化（16回に1回）
            if ((time_check_counter++ & 15) == 0) {
                if (chrono::duration_cast<chrono::milliseconds>(Clock::now() - turn_start).count() > turn_time_limit) {
                    timeout = true; break;
                }
            }
            
            const State& state = beamPool[i];
            pl allMovesArr[MAX_M];
            for(int j=1; j<M; ++j) allMovesArr[j] = predictAIMove(state, j);
            
            MyCand myCandidates[256];
            int myCandSize = getCandidates(state, 0, myCandidates);
            int numCands = min(myCandSize, 500);
            
            for (int j = 0; j < numCands; ++j) {
                if (nextBeamSize >= MAX_BEAM_NODES) break;
                
                allMovesArr[0] = {myCandidates[j].x, myCandidates[j].y};
                simulate_into(state, allMovesArr, nextBeamPool[nextBeamSize]);
                
                uint64_t h = nextBeamPool[nextBeamSize].hash;
                if (seenHashes.find(h) != seenHashes.end()) continue;
                seenHashes.insert(h);

                nextBeamPool[nextBeamSize].setFirstMove((depth == 0) ? allMovesArr[0] : state.getFirstMove());
                nextBeamPool[nextBeamSize].score = nextBeamPool[nextBeamSize].evaluateBase();
                nextBeamSize++;
            }
            if (timeout) break;
        }
        
        if (timeout || nextBeamSize == 0) break;
        
        int pre_beam_size = min(nextBeamSize, BEAM_WIDTH * 3);
        if (nextBeamSize > pre_beam_size) {
            nth_element(nextBeamPool, nextBeamPool + pre_beam_size, nextBeamPool + nextBeamSize, 
                        [](const State& a, const State& b){ return a.score > b.score; });
            nextBeamSize = pre_beam_size;
        }
        
        for (int i = 0; i < nextBeamSize; ++i) nextBeamPool[i].applyHeavyEvaluation();
        
        sort(nextBeamPool, nextBeamPool + nextBeamSize, [](const State& a, const State& b){ return a.score > b.score; });
        if (nextBeamSize > BEAM_WIDTH) nextBeamSize = BEAM_WIDTH;
        
        for(int i = 0; i < nextBeamSize; ++i) beamPool[i] = nextBeamPool[i];
        beamSize = nextBeamSize;
        
        best_move = beamPool[0].getFirstMove();
        depth++;
    }
    return best_move;
}

void solve() {
    initZobrist();
    cin >> N >> M >> T >> U;
    for(int i=0; i<N; ++i) for(int j=0; j<N; ++j) cin >> V[i][j];
    FP.resize(M);
    for(int i=0; i<M; ++i) cin >> FP[i].first >> FP[i].second;

    gameStartTime = Clock::now();
    State currentState;
    for(int i=0; i<M; ++i) {
        auto [x, y] = FP[i];
        currentState.owner[x][y] = i;
        currentState.level[x][y] = 1;
        currentState.pos_x[i] = x;
        currentState.pos_y[i] = y;
        currentState.tileCounts[i] = 1;
        currentState.current_scores[i] = V[x][y];
    }
    currentState.computeHash();

    for(int turn=0; turn<T; ++turn) {
        currentTurn = turn;
        State prevState = currentState;
        
        pl move = beamSearch(currentState);
        cout << move.first << " " << move.second << endl;
        
        vpl TP(M), EP(M);
        vvl O(N, vector<ll>(N)), L(N, vector<ll>(N));
        for(int i=0; i<M; ++i) cin >> TP[i].first >> TP[i].second;
        for(int i=0; i<M; ++i) cin >> EP[i].first >> EP[i].second;
        for(int i=0; i<N; ++i) for(int j=0; j<N; ++j) cin >> O[i][j];
        for(int i=0; i<N; ++i) for(int j=0; j<N; ++j) cin >> L[i][j];

        // AIの学習ロジック（そのまま）
        if (turn > 0) {
            for(int i=1; i<M; ++i) {
                MyCand cand[256];
                int c_sz = getCandidates(prevState, i, cand);
                if (c_sz < 2) continue;
                
                auto [mx, my] = TP[i];
                int cO = prevState.owner[mx][my], cL = prevState.level[mx][my];
                AIParams& p = aiParams[i-1];
                
                double selectedValue = V[mx][my], maxValueInCategory = 0.0;
                for (int c_idx=0; c_idx<c_sz; ++c_idx) {
                    int cx = cand[c_idx].x, cy = cand[c_idx].y;
                    int tO = prevState.owner[cx][cy], tL = prevState.level[cx][cy];
                    bool sameCategory = false;
                    
                    if (cO == -1 && tO == -1) sameCategory = true;
                    else if (cO == i && tO == i && cL < U && tL < U) sameCategory = true;
                    else if (cO != -1 && cO != i && tO != -1 && tO != i) {
                        if ((cL == 1 && tL == 1) || (cL > 1 && tL > 1)) sameCategory = true;
                    }
                    if (sameCategory) maxValueInCategory = max(maxValueInCategory, (double)V[cx][cy]);
                }
                
                if (selectedValue < maxValueInCategory - 1e-9) continue;
                const double lr = 0.02;
                if (cO == -1) p.wa += lr;
                else if (cO == i) { if (cL < U) p.wb += lr; }
                else if (cL == 1) p.wc += lr;
                else p.wd += lr;
                
                double sum = p.wa + p.wb + p.wc + p.wd;
                if (sum > 0) {
                    p.wa = p.wa / sum * 2.4; p.wb = p.wb / sum * 2.4;
                    p.wc = p.wc / sum * 2.4; p.wd = p.wd / sum * 2.4;
                }
            }
        }
        
        // 実際の盤面情報から状態を完璧に同期
        currentState.syncFromActual(O, L, EP);
    }
}

// 限界高速化
int main() {
    cin.tie(0); ios::sync_with_stdio(0);
    solve();
    return 0;
}