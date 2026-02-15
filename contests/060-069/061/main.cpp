#pragma GCC optimize("O3")
#pragma GCC optimize("unroll-loops")

#include <iostream>
#include <vector>
#include <queue>
#include <random>
#include <chrono>
#include <algorithm>
#include <cstring>
#include <map>
#include <set>

using namespace std;

// --- 定数 ---
const int N = 10;
const int DX[4] = {-1, 1, 0, 0};
const int DY[4] = {0, 0, -1, 1};

// 相手AIのパラメータ推定値（Python版と同じ）
struct AIParams {
    double wa = 0.6;
    double wb = 0.6;
    double wc = 0.6;
    double wd = 0.6;
    double eps = 0.3;
};

// プレイヤーごとのパラメータ（学習用）
vector<AIParams> aiParams;

// グローバル定数（入力で決定）
int M_GLOBAL; // プレイヤー数
int T_GLOBAL; // ターン数
int U_GLOBAL; // レベル上限
int V_GLOBAL[N][N]; // 価値

// 乱数生成器
mt19937 rng(12345);

// --- 状態クラス ---
struct GameState {
    int owner[N][N];
    int level[N][N];
    int px[10]; // 最大10人とする
    int py[10];
    int turn;

    GameState() {
        memset(owner, -1, sizeof(owner));
        memset(level, 0, sizeof(level));
        turn = 0;
    }

    // スコア計算（比率）
    double get_score_ratio() const {
        double scores[10] = {0};
        for(int r=0; r<N; ++r){
            for(int c=0; c<N; ++c){
                int o = owner[r][c];
                if(o != -1){
                    scores[o] += V_GLOBAL[r][c] * level[r][c];
                }
            }
        }

        double my_score = scores[0];
        double max_enemy_score = 0;
        for(int i=1; i<M_GLOBAL; ++i){
            if(scores[i] > max_enemy_score) max_enemy_score = scores[i];
        }

        if(max_enemy_score == 0) max_enemy_score = 1.0;
        return my_score / max_enemy_score;
    }

    // 有効な移動先を取得
    // pair<int, int> の vector を返す
    vector<pair<int, int>> get_valid_moves(int p_id) const {
        bool visited[N][N];
        memset(visited, 0, sizeof(visited));
        
        vector<pair<int, int>> reachable;
        reachable.reserve(N*N);
        
        // BFS用
        // pairを使うと遅いので簡易キュー配列
        int qx[N*N], qy[N*N];
        int head = 0, tail = 0;

        int cx = px[p_id];
        int cy = py[p_id];

        // 現在地
        qx[tail] = cx; qy[tail] = cy; tail++;
        visited[cx][cy] = true;
        reachable.push_back({cx, cy});

        while(head < tail){
            int x = qx[head];
            int y = qy[head];
            head++;

            for(int d=0; d<4; ++d){
                int nx = x + DX[d];
                int ny = y + DY[d];
                if(nx >= 0 && nx < N && ny >= 0 && ny < N){
                    if(owner[nx][ny] == p_id && !visited[nx][ny]){
                        visited[nx][ny] = true;
                        reachable.push_back({nx, ny});
                        qx[tail] = nx; qy[tail] = ny; tail++;
                    }
                }
            }
        }

        // 隣接マスの追加（setを使って重複排除する代わりに、bool配列を使う）
        vector<pair<int, int>> candidates;
        candidates.reserve(reachable.size() * 4);
        
        bool is_candidate[N][N];
        memset(is_candidate, 0, sizeof(is_candidate));

        // Reachable nodes are candidates
        for(auto& p : reachable){
            if(!is_candidate[p.first][p.second]){
                is_candidate[p.first][p.second] = true;
                candidates.push_back(p);
            }
            // Adjacent
            for(int d=0; d<4; ++d){
                int nx = p.first + DX[d];
                int ny = p.second + DY[d];
                if(nx >= 0 && nx < N && ny >= 0 && ny < N){
                    if(!is_candidate[nx][ny]){
                        is_candidate[nx][ny] = true;
                        candidates.push_back({nx, ny});
                    }
                }
            }
        }

        // 他のプレイヤーがいる場所を除外
        // erase-remove idiom
        candidates.erase(remove_if(candidates.begin(), candidates.end(), [&](const pair<int, int>& pos){
            for(int i=0; i<M_GLOBAL; ++i){
                if(i != p_id && px[i] == pos.first && py[i] == pos.second) return true;
            }
            return false;
        }), candidates.end());

        return candidates;
    }

    // 相手の行動予測
    pair<int, int> predict_opponent_move(int p_id) {
        vector<pair<int, int>> candidates = get_valid_moves(p_id);
        if(candidates.empty()) return {px[p_id], py[p_id]};

        // プレイヤーごとのパラメーターを使用（インデックスは p_id - 1）
        AIParams& params = aiParams[p_id - 1];

        // ランダム行動
        uniform_real_distribution<double> dist(0.0, 1.0);
        if(dist(rng) < params.eps){
            uniform_int_distribution<int> idx_dist(0, (int)candidates.size() - 1);
            return candidates[idx_dist(rng)];
        }

        // 貪欲行動
        double max_eval = -1e18;
        vector<pair<int, int>> best_moves;
        best_moves.reserve(candidates.size());

        for(auto& pos : candidates){
            int r = pos.first;
            int c = pos.second;
            double val = 0;
            int v_ij = V_GLOBAL[r][c];

            if(owner[r][c] == -1){ // 中立
                val = v_ij * params.wa;
            } else if(owner[r][c] == p_id){ // 自分
                if(level[r][c] < U_GLOBAL) val = v_ij * params.wb;
                else val = 0;
            } else { // 他人
                if(level[r][c] == 1) val = v_ij * params.wc;
                else val = v_ij * params.wd;
            }

            if(val > max_eval){
                max_eval = val;
                best_moves.clear();
                best_moves.push_back(pos);
            } else if(abs(val - max_eval) < 1e-9){
                best_moves.push_back(pos);
            }
        }

        if(best_moves.empty()) return candidates[0];
        uniform_int_distribution<int> idx_dist(0, (int)best_moves.size() - 1);
        return best_moves[idx_dist(rng)];
    }

    // 1ステップ進める
    void step(pair<int, int> my_move) {
        pair<int, int> moves[10];
        moves[0] = my_move;
        
        for(int p=1; p<M_GLOBAL; ++p){
            moves[p] = predict_opponent_move(p);
        }

        // 競合解決
        // pos -> count
        // 10x10 なので直接配列でカウント
        int conflict_map[N][N];
        memset(conflict_map, 0, sizeof(conflict_map));

        for(int p=0; p<M_GLOBAL; ++p){
            conflict_map[moves[p].first][moves[p].second]++;
        }

        bool survived[10];
        for(int p=0; p<M_GLOBAL; ++p){
            int r = moves[p].first;
            int c = moves[p].second;
            if(conflict_map[r][c] > 1){
                // 競合
                if(owner[r][c] == p) survived[p] = true;
                else survived[p] = false;
            } else {
                survived[p] = true;
            }
        }

        // 更新
        int next_px[10], next_py[10];
        for(int p=0; p<M_GLOBAL; ++p) {
            next_px[p] = px[p];
            next_py[p] = py[p];
        }

        // 生き残ったプレイヤーの処理
        for(int p=0; p<M_GLOBAL; ++p){
            if(survived[p]){
                int tx = moves[p].first;
                int ty = moves[p].second;
                
                // 移動成功の仮定
                next_px[p] = tx;
                next_py[p] = ty;

                int current_owner = owner[tx][ty];
                if(current_owner == -1){
                    // 占領
                    owner[tx][ty] = p;
                    level[tx][ty] = 1;
                } else if(current_owner == p){
                    // 強化
                    if(level[tx][ty] < U_GLOBAL){
                        level[tx][ty]++;
                    }
                } else {
                    // 攻撃
                    level[tx][ty]--;
                    if(level[tx][ty] == 0){
                        owner[tx][ty] = p;
                        level[tx][ty] = 1;
                    } else {
                        // 倒しきれなかった -> 駒回収（元の位置に戻る）
                        next_px[p] = px[p];
                        next_py[p] = py[p];
                    }
                }
            } else {
                // 競合負け -> 元の位置
                next_px[p] = px[p];
                next_py[p] = py[p];
            }
        }

        for(int p=0; p<M_GLOBAL; ++p){
            px[p] = next_px[p];
            py[p] = next_py[p];
        }
        turn++;
    }
};

// --- MCTS (Flat Monte Carlo) ---
// 時間いっぱいまでプレイアウトを行う
pair<int, int> run_mcts(const GameState& root_state, double time_limit_sec) {
    auto start_time = chrono::high_resolution_clock::now();
    
    vector<pair<int, int>> candidates = root_state.get_valid_moves(0);
    if(candidates.empty()) return {root_state.px[0], root_state.py[0]};
    if(candidates.size() == 1) return candidates[0];

    // 統計情報
    map<pair<int, int>, double> total_scores;
    map<pair<int, int>, int> counts;
    for(auto& m : candidates){
        total_scores[m] = 0;
        counts[m] = 0;
    }

    int SIMULATION_DEPTH = 10; // C++ならもっと深くてもいいが、精度重視で10-15程度
    int loop_cnt = 0;

    while(true){
        // 時間チェック (100回に1回)
        if(loop_cnt % 100 == 0){
            auto now = chrono::high_resolution_clock::now();
            double elapsed = chrono::duration_cast<chrono::duration<double>>(now - start_time).count();
            if(elapsed > time_limit_sec) break;
        }

        // 候補手を順番に選ぶ (Round Robin)
        pair<int, int> first_move = candidates[loop_cnt % candidates.size()];
        
        GameState sim_state = root_state; // コピー
        sim_state.step(first_move);

        // プレイアウト
        int depth_limit = min(sim_state.turn + SIMULATION_DEPTH, T_GLOBAL);
        while(sim_state.turn < depth_limit){
            // 自分も敵と同じロジック（予測関数）で動くと仮定して高速化
            // 本来は自分の手をランダムか何かで選ぶべきだが、greedyに近いほうが評価精度が良い場合が多い
            pair<int, int> my_next = sim_state.predict_opponent_move(0);
            sim_state.step(my_next);
        }

        double score = sim_state.get_score_ratio();
        total_scores[first_move] += score;
        counts[first_move]++;
        
        loop_cnt++;
    }

    // ベストな手を選択
    pair<int, int> best_move = candidates[0];
    double best_avg = -1.0;

    for(auto& m : candidates){
        if(counts[m] > 0){
            double avg = total_scores[m] / counts[m];
            if(avg > best_avg){
                best_avg = avg;
                best_move = m;
            }
        }
    }

    // デバッグ出力（必要ならstderrへ）
    // cerr << "Loops: " << loop_cnt << " Best Score: " << best_avg << endl;

    return best_move;
}

// --- 入出力処理 ---

void read_initial_input() {
    int N_DUMMY; // Already const N=10
    cin >> N_DUMMY >> M_GLOBAL >> T_GLOBAL >> U_GLOBAL; // N is constant 10

    for(int i=0; i<N; ++i){
        for(int j=0; j<N; ++j){
            cin >> V_GLOBAL[i][j];
        }
    }
}

void read_turn_result(GameState& state, int attempted_tx[], int attempted_ty[]) {
    // 全プレイヤーの移動しようとした先 (tx, ty) -> 学習用に保存
    for(int i=0; i<M_GLOBAL; ++i) {
        cin >> attempted_tx[i] >> attempted_ty[i];
    }

    // 確定位置
    for(int i=0; i<M_GLOBAL; ++i){
        cin >> state.px[i] >> state.py[i];
    }

    // 所有者
    for(int i=0; i<N; ++i){
        for(int j=0; j<N; ++j){
            cin >> state.owner[i][j];
        }
    }

    // レベル
    for(int i=0; i<N; ++i){
        for(int j=0; j<N; ++j){
            cin >> state.level[i][j];
        }
    }
}

// AIパラメーターの学習
void learn_ai_params(const GameState& prev_state, int attempted_tx[], int attempted_ty[]) {
    const double lr = 0.02; // 学習率
    
    for(int p = 1; p < M_GLOBAL; ++p) {
        // このAIプレイヤーが選択可能だったマスが2つ以上ある場合のみ学習
        vector<pair<int, int>> candidates = prev_state.get_valid_moves(p);
        if(candidates.size() < 2) continue;
        
        // AIが実際に選んだマス
        int mx = attempted_tx[p];
        int my = attempted_ty[p];
        
        // 前の状態でのマスの情報
        int cell_owner = prev_state.owner[mx][my];
        int cell_level = prev_state.level[mx][my];
        
        AIParams& params = aiParams[p - 1];
        
        // AIが選んだマスの種類に応じて対応する重みを増加
        if(cell_owner == -1) {
            // 中立マスを選んだ
            params.wa += lr;
        } else if(cell_owner == p) {
            // 自分の領土を選んだ
            if(cell_level < U_GLOBAL) {
                params.wb += lr;
            }
            // 敵の領土を選んだ
            if(cell_level == 1) {
                params.wc += lr;
            } else {
                params.wd += lr;
            }
        }
        
        // 正規化（合計を一定に保つ）
        double sum = params.wa + params.wb + params.wc + params.wd;
        if(sum > 0) {
            double avg = sum / 4.0;
            params.wa = params.wa / sum * (avg * 4.0);
            params.wb = params.wb / sum * (avg * 4.0);
            params.wc = params.wc / sum * (avg * 4.0);
            params.wd = params.wd / sum * (avg * 4.0);
        }
    }
}

int main() {
    // 高速化
    ios_base::sync_with_stdio(false);
    cin.tie(NULL);

    // 時間計測開始
    auto global_start = chrono::high_resolution_clock::now();
    double TIME_LIMIT_TOTAL = 1.95; // 2.0s制限に対して少し余裕を持つ

    // 初期入力
    // N M T U
    // V...
    // sx sy ...
    int n_in;
    if(!(cin >> n_in)) return 0;
    M_GLOBAL = 0; // Initialize later
    // The first line handling needs to match input format exactly
    // Format: N M T U
    int m_in, t_in, u_in;
    cin >> m_in >> t_in >> u_in;
    M_GLOBAL = m_in;
    T_GLOBAL = t_in;
    U_GLOBAL = u_in;

    for(int i=0; i<N; ++i){
        for(int j=0; j<N; ++j){
            cin >> V_GLOBAL[i][j];
        }
    }

    // AIパラメータの初期化（M-1人分）
    aiParams.resize(M_GLOBAL - 1);
    
    GameState state;
    state.turn = 0;
    
    // 初期位置
    for(int p=0; p<M_GLOBAL; ++p){
        cin >> state.px[p] >> state.py[p];
        // 初期所有権
        state.owner[state.px[p]][state.py[p]] = p;
        state.level[state.px[p]][state.py[p]] = 1;
    }

    // ターンループ
    GameState prev_state; // 学習用に前の状態を保存
    int attempted_tx[10], attempted_ty[10]; // 移動先の記録
    
    for(int t=0; t<T_GLOBAL; ++t){
        state.turn = t;

        // 時間管理
        auto now = chrono::high_resolution_clock::now();
        double elapsed = chrono::duration_cast<chrono::duration<double>>(now - global_start).count();
        double remaining_time = TIME_LIMIT_TOTAL - elapsed;
        double remaining_turns = T_GLOBAL - t;
        
        // 1ターンあたりの時間割り当て（動的）
        // 終盤に時間を残しすぎないようにしつつ、最低限の時間は確保
        double time_for_turn = max(0.005, remaining_time / (remaining_turns + 2.0));

        // MCTS実行
        pair<int, int> best_move = run_mcts(state, time_for_turn);

        // 出力
        cout << best_move.first << " " << best_move.second << endl; // endl flushes

        // 学習用に現在の状態を保存
        if(t > 0) {
            // 前のターンの情報を使って学習
            learn_ai_params(prev_state, attempted_tx, attempted_ty);
        }
        prev_state = state;

        // 結果読み込み
        read_turn_result(state, attempted_tx, attempted_ty);
    }

    return 0;
}