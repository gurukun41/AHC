#include <bits/stdc++.h>
#include <atcoder/all>
#include <chrono>
#include <random>
#include <unordered_set>
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

// 盤面サイズと最大プレイヤー数は制約から固定可能
constexpr int MAX_N = 10;
constexpr int MAX_M = 8;

// グローバル変数：盤面サイズ、プレイヤー数、ターン数、レベル上限
ll N, M, T, U;
vvl V;  // 各マスの価値
vpl FP;  // 各プレイヤーの初期位置
vpl dir = {{1, 0}, {0, 1}, {-1, 0}, {0, -1}};  // 4方向

// --- Zobrist Hashing 用 ---
// 64bit乱数生成
uint64_t rng64() {
    static mt19937_64 mt(12345);
    return mt();
}

// ハッシュ用テーブル (サイズは適宜余裕を持つ)
// owner: -1(未所属)〜M-1 まで。indexは owner+1 でアクセス
uint64_t z_owner[55][55][20]; 
// level: 0〜U。最大レベル+α
uint64_t z_level[55][55][305];
// pos: プレイヤー0〜M-1 の位置
uint64_t z_pos[20][55][55];

void initZobrist() {
    rep(i, 0, 55) rep(j, 0, 55) {
        rep(k, 0, 20) z_owner[i][j][k] = rng64();
        rep(k, 0, 305) z_level[i][j][k] = rng64();
    }
    rep(i, 0, 20) rep(x, 0, 55) rep(y, 0, 55) {
        z_pos[i][x][y] = rng64();
    }
}
// ------------------------

// ビームサーチのパラメータ
const int MAX_DEPTH = 30;  // 先読みする深さ
const int MY_CANDIDATES = 100;  // 自分の手の候補数上限

// AIの評価パラメータ（学習用）
struct AIParams {
    double wa, wb, wc, wd;  // 各状況での重み
    AIParams() : wa(0.6), wb(0.6), wc(0.6), wd(0.6) {}
};
vector<AIParams> aiParams;
int currentTurn = 0;
TimePoint gameStartTime;

const ll TOTAL_TIME_LIMIT_MS = 1900;
const ll TIME_BUFFER_MS = 500;

// プレイヤー数に応じてビーム幅を調整（敵が多いほど計算量増加のため幅を削減）
int getBeamWidth() {
    if (M <= 2) return 300;
    if (M == 3) return 250;
    if (M == 4) return 200;
    if (M == 5) return 150;
    if (M == 6) return 120;
    if (M == 7) return 100;
    return 80; // M = 8
}

int getAdaptiveBeamWidth() {
    return getBeamWidth();
}

// 高速化用バッファ（BFSで使用）
int visited[55][55];
int visited_token = 0;

// ゲームの状態を表す構造体
struct State {
    // 1. 動的配列(vector)を全廃止し、固定長配列に変更
    // 2. 扱う値の範囲に合わせて型を最小化 (int8_t は -128〜127)
    
    int8_t pos_x[MAX_M];        // 各プレイヤーのX座標 (0〜9)
    int8_t pos_y[MAX_M];        // 各プレイヤーのY座標 (0〜9)
    
    int8_t owner[MAX_N][MAX_N]; // 所有者 (-1〜7)
    int8_t level[MAX_N][MAX_N]; // レベル (0〜5)
    
    int8_t first_move_x;        // 最初の手のX座標
    int8_t first_move_y;        // 最初の手のY座標
    
    int8_t tileCounts[MAX_M];   // 各プレイヤーのタイル数 (最大でも100なので int8_t でOK)
    
    double score;               // 評価値 (※long doubleは処理が重いためdoubleに変更)
    uint64_t hash;              // 盤面ハッシュ
    
    // デフォルトコンストラクタ
    State() {
        first_move_x = -1;
        first_move_y = -1;
        score = 0.0;
        hash = 0;
        
        // C++の固定長配列は自動でゼロクリアされないため、手動で初期化
        // （ビームサーチ中で状態をコピーして作る場合はコピーコンストラクタが呼ばれるため高速）
        for(int i = 0; i < MAX_M; ++i) {
            pos_x[i] = 0; pos_y[i] = 0;
            tileCounts[i] = 0;
        }
        for(int i = 0; i < MAX_N; ++i) {
            for(int j = 0; j < MAX_N; ++j) {
                owner[i][j] = -1;
                level[i][j] = 0;
            }
        }
    }
    
    // vplとvvlから初期化するコンストラクタ（既存コードとの互換性のため）
    State(const vpl& pos, const vvl& o, const vvl& l) 
        : first_move_x(-1), first_move_y(-1), score(0.0), hash(0) {
        for(int i = 0; i < MAX_M; ++i) {
            tileCounts[i] = 0;
        }
        for(int i = 0; i < M; ++i) {
            pos_x[i] = pos[i].first;
            pos_y[i] = pos[i].second;
        }
        for(int i = 0; i < N; ++i) {
            for(int j = 0; j < N; ++j) {
                owner[i][j] = o[i][j];
                level[i][j] = l[i][j];
                if (owner[i][j] >= 0) tileCounts[owner[i][j]]++;
            }
        }
        computeHash();
    }
    
    // ハッシュ計算（配列へのアクセスが連続的になり、キャッシュに乗りやすくなります）
    void computeHash() {
        hash = 0;
        for (int i = 0; i < N; ++i) {
            for (int j = 0; j < N; ++j) {
                if (owner[i][j] + 1 < 20) {
                    hash ^= z_owner[i][j][owner[i][j] + 1];
                }
                if (level[i][j] < 305) {
                    hash ^= z_level[i][j][level[i][j]];
                }
            }
        }
        for (int i = 0; i < M; ++i) {
            hash ^= z_pos[i][pos_x[i]][pos_y[i]];
        }
    }

    // 各プレイヤーのスコアを計算
    // ※戻り値も vector<ll> ではなく std::array や 引数参照 にするとさらに速くなります
    std::array<long long, MAX_M> calcScores() const {
        std::array<long long, MAX_M> scores = {0};
        for (int i = 0; i < N; ++i) {
            for (int j = 0; j < N; ++j) {
                if (owner[i][j] >= 0) {
                    scores[owner[i][j]] += V[i][j] * level[i][j];
                }
            }
        }
        return scores;
    }
    
    
    // 互換性のためのヘルパー関数: positions配列として扱う
    pl getPosition(int player) const {
        return {pos_x[player], pos_y[player]};
    }
    
    void setPosition(int player, pl pos) {
        pos_x[player] = pos.first;
        pos_y[player] = pos.second;
    }
    
    // myFirstMoveのヘルパー関数
    pl getFirstMove() const {
        return {first_move_x, first_move_y};
    }
    
    void setFirstMove(pl move) {
        first_move_x = move.first;
        first_move_y = move.second;
    }
    
    // 指定プレイヤーの連結成分数をカウント（領土の分断度合いを評価）
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
    
    // 状態の評価値を計算（スコア比率 + 連結性ボーナス）
    double evaluate() const {
        bool isManyEnemies = (M >= 4);
        bool isExpansionPhase = (currentTurn < (T / 2) );
        double baseScore;
        
        // 敵が多く序盤の場合：タイル数で評価（早期拡張を優先）
        if (isManyEnemies && isExpansionPhase) {
            /*ll maxAITiles = 0;
            rep(i, 1, M) chmax(maxAITiles, tileCounts[i]);
            if (maxAITiles == 0) return 1000.0;*/
            baseScore = (double)tileCounts[0] ; // / maxAITiles;
        } else {
            // それ以外：最強AI対比のスコア比率で評価
            auto scores = calcScores();            
            ll maxAI = 0;
            rep(i, 1, M) chmax(maxAI, scores[i]);
            if (maxAI == 0) return 1000.0;
            baseScore = (double)scores[0] / maxAI;
        }
        
        // 連結性ボーナス：領土が分断されていると減点（1連結が理想）
        int myComponents = countConnectedComponents(0);
        double connectivityBonus = 0.0;
        if (myComponents > 0) {
            connectivityBonus = max(0.0, 0.05 * (4.0 - myComponents) / 3.0);
        }
        return baseScore * (1.0 + connectivityBonus);
    }
    
    bool operator>(const State& other) const { return score > other.score; }
};

// ターン終了時の情報を受け取る構造体
struct Get {
    vpl TP, EP;  // 移動先と確定位置
    vvl O, L;    // 所有者とレベル
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

// 指定プレイヤーが移動可能なマスをスコア順に取得
vector<pl> getCandidatesForPlayer(const State& state, int player) {
    // BFSで到達可能領土を探索（自分の領土を辿って到達できる範囲）
    visited_token++;
    vpl reachable;
    static pl q[2500];
    int head = 0, tail = 0;
    pl playerPos = state.getPosition(player);
    q[tail++] = playerPos;
    visited[playerPos.first][playerPos.second] = visited_token;
    
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
    
    // 到達可能領土とその隣接マスが移動候補
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
        auto p = state.getPosition(i);
        rep(j, 0, candidates.size()) {
            if (candidates[j] == p) {
                candidates.erase(candidates.begin() + j);
                break;
            }
        }
    }

    // 最もスコアが高い敵を特定（プレイヤー0の視点で）
    int strongestEnemy = -1;
    ll maxEnemyScore = 0;
    if (player == 0) {
        auto scores = state.calcScores();
        rep(i, 1, M) {
            if (scores[i] > maxEnemyScore) {
                maxEnemyScore = scores[i];
                strongestEnemy = i;
            }
        }
    }
    
    // 候補マスをスコアリング（序盤は拡張優先、終盤は強化優先）
    vector<pair<double, pl>> scored;
    bool isExpansionPhase = (currentTurn < T / 2);
    for (auto [x, y] : candidates) {
        if (state.owner[x][y] == player && state.level[x][y] >= U) continue;  // 上限到達は除外
        double score = V[x][y];
        // 未占領：序盤は高評価、終盤はやや低評価
        if (state.owner[x][y] == -1) score *= isExpansionPhase ? 10.0 : 1.5;
        // 自領土：序盤は低評価、終盤は標準
        else if (state.owner[x][y] == player) score *= isExpansionPhase ? 0.01 : 0.8;
        // 敵領土：攻撃は常に高評価、最強の敵ならさらにボーナス
        else {
            score *= 2.0;
            if (player == 0 && state.owner[x][y] == strongestEnemy) {
                score *= 300.0;  // 最強の敵を攻撃する場合は300倍のボーナス
            }
        }
        scored.push_back({score, {x, y}});
    }
    sort(all(scored), greater<pair<double, pl>>());
    vpl result;
    for (auto [sc, pos] : scored) result.push_back(pos);
    if (result.empty()) result.push_back(state.getPosition(player));
    return result;
}

// 全プレイヤーの手を適用してシミュレーション
State simulate(const State& state, const vpl& moves) {
    State nextState = state;
    
    // std::mapを使わずに、単純な配列と二重ループで衝突を判定する
    bool collected[MAX_M] = {false};
    
    for (int i = 0; i < M; ++i) {
        int cx = moves[i].first;
        int cy = moves[i].second;
        
        // このマス (cx, cy) に移動してくるプレイヤーを集計
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
        
        // 2人以上が同じマスに移動した場合の競合解決
        if (players_on_cell >= 2) {
            if (ownerPresent) {
                // 所有者がいる場合、所有者以外を回収
                for (int k = 0; k < players_on_cell; ++k) {
                    int p = conflict_players[k];
                    if (p != cellOwner) collected[p] = true;
                }
            } else {
                // 所有者がいない場合、全員回収
                for (int k = 0; k < players_on_cell; ++k) {
                    collected[conflict_players[k]] = true;
                }
            }
        }
    }
    
    // 各プレイヤーの移動先での領土更新とハッシュの差分更新
    for (int i = 0; i < M; ++i) {
        if (collected[i]) continue;  // 競合で既に回収済みの場合は盤面変化なし
        
        int x = moves[i].first;
        int y = moves[i].second;
        int oldOwner = state.owner[x][y];
        int oldLevel = state.level[x][y];
        
        // 1. 変化前の盤面情報をハッシュからXORで消去
        nextState.hash ^= z_owner[x][y][oldOwner + 1];
        nextState.hash ^= z_level[x][y][oldLevel];
        
        if (oldOwner == -1) {
            // 占領：未占領マスを自領土に
            nextState.owner[x][y] = i;
            nextState.level[x][y] = 1;
            nextState.tileCounts[i]++;
        } else if (oldOwner == i) {
            // 強化：自領土のレベルアップ（上限Uまで）
            if (nextState.level[x][y] < U) nextState.level[x][y]++;
        } else {
            // 攻撃：敵領土のレベルダウン
            nextState.level[x][y]--;
            if (nextState.level[x][y] == 0) {
                // レベル0になったら奪取成功
                nextState.owner[x][y] = i;
                nextState.level[x][y] = 1;
                nextState.tileCounts[oldOwner]--;
                nextState.tileCounts[i]++;
            } else {
                // レベルが残っていたら攻撃失敗で駒回収
                collected[i] = true;
                // ※レベルダウン自体は成功しているため、マスの状態変更は有効です
            }
        }
        
        // 2. 変化後の盤面情報をハッシュにXORで追加
        nextState.hash ^= z_owner[x][y][nextState.owner[x][y] + 1];
        nextState.hash ^= z_level[x][y][nextState.level[x][y]];
    }
    
    // 3. プレイヤー位置の更新とハッシュの差分更新
    for (int i = 0; i < M; ++i) {
        // 古い位置を消去
        nextState.hash ^= z_pos[i][state.pos_x[i]][state.pos_y[i]];
        // 新しい位置を設定
        nextState.setPosition(i, collected[i] ? state.getPosition(i) : moves[i]);
        // 新しい位置を追加
        nextState.hash ^= z_pos[i][nextState.pos_x[i]][nextState.pos_y[i]];
    }
    return nextState;
}

// AIプレイヤーの次の手を予測（貪欲法）
pl predictAIMove(const State& state, int player) {
    vpl candidates = getCandidatesForPlayer(state, player);
    AIParams& params = aiParams[player - 1];
    double bestVal = -1e18;
    pl bestMove = state.getPosition(player);
    for (auto [x, y] : candidates) {
        double val = 0.0;
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

// ターンに割り当てる時間を計算して反復深化を行うビームサーチ
pl beamSearch(const State& initialState) {
    vector<State> beam;
    beam.push_back(initialState);
    
    // 万が一どこにも移動できない場合の初期手
    pl best_move = initialState.getPosition(0);
    
    // 1ターンあたりの割り当て時間を計算
    auto now = Clock::now();
    double elapsed_ms = chrono::duration_cast<chrono::milliseconds>(now - gameStartTime).count();
    
    // 残り時間を残りターン数で割る（TOTAL_TIME_LIMIT_MS は 1900）
    double turn_time_limit = (TOTAL_TIME_LIMIT_MS - elapsed_ms) / (T - currentTurn);
    auto turn_start = Clock::now();
    
    int depth = 0;
    const int MAX_LIMIT_DEPTH = 15; // 行き過ぎ防止（15手読めれば十分すぎるため）
    
    // 時間が許す限り深く探索を続ける（反復深化）
    while (depth < MAX_LIMIT_DEPTH) {
        int BEAM_WIDTH = getAdaptiveBeamWidth();
        vector<State> nextBeam;
        unordered_set<uint64_t> seenHashes;
        
        bool timeout = false;

        for (const State& state : beam) {
            // 状態を展開する前に時間チェック
            auto current_time = Clock::now();
            if (chrono::duration_cast<chrono::milliseconds>(current_time - turn_start).count() > turn_time_limit) {
                timeout = true;
                break;
            }
            
            // 各AIプレイヤーの行動を予測（貪欲法）
            vpl allMoves(M);
            rep(i, 1, M) allMoves[i] = predictAIMove(state, i);
            
            // 自分の手の候補を取得
            vpl myCandidates = getCandidatesForPlayer(state, 0);
            if (myCandidates.size() > MY_CANDIDATES) myCandidates.resize(MY_CANDIDATES);
            
            // 各候補手について次状態を生成
            for (pl move : myCandidates) {
                allMoves[0] = move;
                State nextState = simulate(state, allMoves);
                
                // ハッシュ計算と重複チェック
                if (seenHashes.count(nextState.hash)) continue;
                seenHashes.insert(nextState.hash);

                nextState.setFirstMove((depth == 0) ? move : state.getFirstMove());
                nextState.score = nextState.evaluate();
                nextBeam.push_back(nextState);
            }
        }
        
        // 途中で時間切れになった場合、中途半端な探索結果 (nextBeam) は捨てて終了
        if (timeout) {
            break; 
        }
        
        // 有効な手が一つもなければ終了
        if (nextBeam.empty()) break;
        
        // スコア順にソートして上位 BEAM_WIDTH 個を保持
        sort(all(nextBeam), greater<State>());
        if (nextBeam.size() > BEAM_WIDTH) nextBeam.resize(BEAM_WIDTH);
        
        // ビームを更新し、この「完了した深さ」での最善手を記録
        beam = nextBeam;
        best_move = beam[0].getFirstMove();
        
        depth++;
    }
    
    // cout << "Reached Depth: " << depth << endl; // ローカルテスト用（提出時は消す）
    
    return best_move;
}

// メイン処理
void solve() {
    // ★Zobrist初期化
    initZobrist();

    // 初期入力
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
        currentState.setPosition(i, FP[i]);
    }
    currentState.computeHash(); // 初期ハッシュ計算

    // ターンループ
    rep(turn, 0, T) {
        currentTurn = turn;
        State prevState = currentState;
        // ビームサーチで最善手を決定
        pl move = beamSearch(currentState);
        cout << move.first << " " << move.second << endl;
        // ターン終了時の状態を取得
        Get g;
        g.Getinit();
        // AIパラメータの学習（観測した行動から重みを更新）
        if (turn > 0) rep(i, 1, M) {
            vector<pl> cand = getCandidatesForPlayer(prevState, i);
            if (cand.size() < 2) continue;  // 選択肢が1つしかない場合は学習なし
            auto [mx, my] = g.TP[i];
            int cO = prevState.owner[mx][my], cL = prevState.level[mx][my];
            const double lr = 0.02;  // 学習率
            AIParams& p = aiParams[i-1];
            
            // AIが選んだマスの種類に応じて対応する重みを増加
            if (cO == -1) p.wa += lr;  // 未占領を選んだ
            else if (cO == i) { if (cL < U) p.wb += lr; }  // 自領土強化を選んだ
            else if (cL == 1) p.wc += lr;  // レベル1の敵領土攻撃
            else p.wd += lr;  // レベル2+の敵領土攻撃
            
            // 正規化（合計を一定に保つ：4パラメータで平均20.6）
            double sum = p.wa + p.wb + p.wc + p.wd;
            if (sum > 0) {
                p.wa = p.wa / sum * 2.4;
                p.wb = p.wb / sum * 2.4;
                p.wc = p.wc / sum * 2.4;
                p.wd = p.wd / sum * 2.4;
            }
        }
        // 受け取った情報で状態を更新
        rep(i, 0, M) currentState.setPosition(i, g.EP[i]);
        rep(i, 0, N) rep(j, 0, N) {
            currentState.owner[i][j] = g.O[i][j];
            currentState.level[i][j] = g.L[i][j];
        }
        // タイル数を再計算
        for(int i = 0; i < MAX_M; ++i) currentState.tileCounts[i] = 0;
        rep(i, 0, N) rep(j, 0, N) if (currentState.owner[i][j] >= 0) currentState.tileCounts[currentState.owner[i][j]]++;
        // ターン更新時にもハッシュ再計算
        currentState.computeHash();
    }
}
// 超攻撃深さ変更
int main() {
    cin.tie(0); ios::sync_with_stdio(0);
    solve();
    return 0;
}