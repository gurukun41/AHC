#include <bits/stdc++.h>
#include <atcoder/all>
using namespace std;
using ll = long long;
using ld = long double;
using mint = atcoder::modint998244353;
using vl = vector<ll>;                                   // long long型の一次元
using vvl = vector<vl>;                                  // long long型の二次元配列
using vvvl = vector<vvl>;                                // long long型の三次元配列
using vi = vector<int>;                                  // int型の一次元
using vvi = vector<vi>;                                  // int型の二次元配列
using vvvi = vector<vvi>;                                // int型の三次元配列
using vb = vector<bool>;                                 // bool型の一次元
using vvb = vector<vb>;                                  // bool型の二次元配列
using vvvb = vector<vvb>;                                // bool型の三次元配列
using vs = vector<string>;                               // string型の一次元
using vvs = vector<vs>;                                  // string型の二次元配列
using pl = pair<ll, ll>;                                 // long long型のペア
using vpl = vector<pl>;                                  // long long型のペアの一次元配列
#define rep(i, a, b) for (ll i = (a); i < (ll)(b); i++)  // for文の短縮
#define all(v) v.begin(), v.end()                        // all(v)でvの始まりと終わりのイテレーター
#define x first
#define y second


ll N;           // 盤面の大きさ
ll K;           // 目的地の個数
ll T;           // ステップ数の上限
vs v;           // 壁(文字列のj文字目はマス (i,j) とマス (i,j+1) の間に壁があるか)
vs h;           // 壁(文字列のj文字目はマス (i,j) とマス (i+1,j) の間に壁があるか)
vpl targets;    // 目的地の座標リスト

inline ll cell_id(ll i, ll j) {
    return i * N + j;
}

inline pl id_to_cell(ll id) {
    return pl{id / N, id % N};
}

void Input() {
    cin >> N >> K >> T;
    v.resize(N);
    rep(i, 0, N) {
        cin >> v[i];
    }
    h.resize(N - 1);
    rep(i, 0, N - 1) {
        cin >> h[i];
    }
    targets.resize(K);
    rep(i, 0, K) {
        cin >> targets[i].x >> targets[i].y;
    }
}

// 時間取得関数
double get_time() {
    using namespace std::chrono;
    return duration_cast<duration<double>>(steady_clock::now().time_since_epoch()).count();
}

// 乱数生成器
struct Random {
    std::mt19937 rng;
    Random(uint32_t seed = 42) : rng(seed) {}
    
    ll next_int(ll n) {
        return std::uniform_int_distribution<ll>(0, n - 1)(rng);
    }
    
    double next_double() {
        return std::uniform_real_distribution<double>(0.0, 1.0)(rng);
    }
};

Random rnd(42);


struct Route {
    vector<vpl> paths; // 移動経路(ターゲットからターゲットへの経路)
    map<pl, ll> usedCells; // これまでに使用したマス -> 使用回数

    Route() {}

    // 経路内に重複があるかチェック
    bool hasLoopInPath(const vpl& path) {
        set<pl> visited;
        for (const auto& cell : path) {
            if (visited.count(cell)) {
                return true; // 重複発見
            }
            visited.insert(cell);
        }
        return false;
    }

    // 2つの経路を連結しても自己交差しないかチェック
    bool canMergePaths(const vpl& path1, const vpl& path2) {
        set<pl> visited;
        // path1の全セルを追加
        for (const auto& cell : path1) {
            visited.insert(cell);
        }
        // path2で重複があるかチェック（始点は共通なのでスキップ）
        for (size_t i = 1; i < path2.size(); i++) {
            if (visited.count(path2[i])) {
                return false; // 重複発見
            }
            visited.insert(path2[i]);
        }
        return true;
    }

    // 連続する経路を可能な限り連結
    void mergePaths() {
        vector<vpl> merged;
        vpl current;
        
        rep(i, 0, (ll)paths.size()) {
            if (current.empty()) {
                current = paths[i];
            } else {
                // 現在の経路と次の経路を連結できるか確認
                if (canMergePaths(current, paths[i])) {
                    // 連結可能：次の経路を追加（始点は重複するので除く）
                    current.insert(current.end(), paths[i].begin() + 1, paths[i].end());
                } else {
                    // 連結不可：現在の経路を保存し、新しい経路を開始
                    merged.push_back(current);
                    current = paths[i];
                }
            }
        }
        
        // 最後の経路を追加
        if (!current.empty()) {
            merged.push_back(current);
        }
        
        ll before = paths.size();
        paths = merged;
        
        cerr << "Merged paths: " << before << " -> " << paths.size() << " segments" << endl;
        
        // usedCellsを再計算
        usedCells.clear();
        for (const auto& path : paths) {
            for (const auto& cell : path) {
                usedCells[cell]++;
            }
        }
    }

    void DecideRoute() {
        rep(i,0,K-1) {
            // targets[i] から targets[i+1] への経路を決定し、pathsに追加
            vpl path;
            ll si = targets[i].x;
            ll sj = targets[i].y;
            ll gi = targets[i+1].x;
            ll gj = targets[i+1].y;

            // A*アルゴリズム(タイブレーカー付き)
            auto heuristic = [&](ll pi, ll pj) -> ll {
                return abs(pi - gi) + abs(pj - gj);
            };
            
            // タイブレーカー: 使用済みマスを優先（重複を促進）
            auto tie_breaker = [&](ll pi, ll pj) -> ld {
                pl cell = {pi, pj};
                // 使用済みマスには負のペナルティ（優先度を上げる）
                if (usedCells.count(cell)) {
                    return -1 * usedCells[cell] * 0.5; // 使用回数が多いほど優先
                }
                return 0.0;
            };

            priority_queue<pair<ld, pair<ll, pl>>, vector<pair<ld, pair<ll, pl>>>, greater<>> pq;
            ll NN = N * N;
            vector<ll> parent(NN, -1);
            vector<ll> gScore(NN, (ll)4e18);
            
            pl start = {si, sj};
            ld f_start = 0 + heuristic(si, sj) + tie_breaker(si, sj);
            ll start_id = cell_id(si, sj);
            ll goal_id = cell_id(gi, gj);
            pq.push({f_start, {0, start}});
            parent[start_id] = start_id;
            gScore[start_id] = 0;
            bool found = false;
            
            while (!pq.empty() && !found) {
                auto [f, gAndCurr] = pq.top();
                pq.pop();
                ll g = gAndCurr.first;
                pl curr = gAndCurr.second;
                ll ci = curr.x;
                ll cj = curr.y;
                ll curr_id = cell_id(ci, cj);
                
                if (curr_id == goal_id) {
                    found = true;
                    break;
                }
                
                if (g > gScore[curr_id]) continue;
                
                vector<pl> directions = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
                for (const auto& dir : directions) {
                    ll ni = ci + dir.x;
                    ll nj = cj + dir.y;
                    if (ni >= 0 && ni < N && nj >= 0 && nj < N) {
                        if (dir.x == -1 && ci > 0 && h[ci - 1][cj] == '1') continue;
                        if (dir.x == 1 && ci < N - 1 && h[ci][cj] == '1') continue;
                        if (dir.y == -1 && cj > 0 && v[ci][cj - 1] == '1') continue;
                        if (dir.y == 1 && cj < N - 1 && v[ci][cj] == '1') continue;

                        pl next = {ni, nj};
                        ll next_id = cell_id(ni, nj);
                        ll tentative_g = g + 1;
                        
                        if (tentative_g < gScore[next_id]) {
                            gScore[next_id] = tentative_g;
                            ld f_next = tentative_g + heuristic(ni, nj) + tie_breaker(ni, nj);
                            parent[next_id] = curr_id;
                            pq.push({f_next, {tentative_g, next}});
                        }
                    }
                }
            }
            
            ll step_id = goal_id;
            if (parent[step_id] == -1) step_id = start_id; // fallback to start if unreachable
            while (true) {
                path.push_back(id_to_cell(step_id));
                if (step_id == parent[step_id]) break;
                step_id = parent[step_id];
            }
            reverse(all(path));
            paths.push_back(path);
            
            for (const auto& cell : path) {
                usedCells[cell]++;
            }
        }
    }

    // 経路全体を焼きなまして共有部分を増やす
    void optimizeGlobalRoute(double tl) {
        double start = get_time();
        double deadline = tl;
        if (deadline <= start) return;
        double duration = max(1e-9, deadline - start);
        
        // 現在の経路の総距離とユニークセル数
        auto calcMetrics = [&]() -> pair<ll, ll> {
            ll totalDist = 0;
            set<pl> uniqueCells;
            for (const auto& path : paths) {
                totalDist += (ll)path.size() - 1;
                for (const auto& cell : path) {
                    uniqueCells.insert(cell);
                }
            }
            return {totalDist, uniqueCells.size()};
        };
        
        auto [currentDist, currentUnique] = calcMetrics();
        ll best_score = currentUnique * 1000 + currentDist;
        vector<vpl> best_paths = paths;
        
        ll iter = 0;
        while (get_time() < deadline) {
            double now = get_time();
            double time = min(1.0, (now - start) / duration);
            double temp = 5000.0 * (1.0 - time);
            iter++;
            
            // ランダムに2つの連続する経路を選ぶ
            if (K < 3) break;
            ll seg_idx = rnd.next_int(K - 2);
            
            // targets[seg_idx] -> targets[seg_idx+1] -> targets[seg_idx+2]
            // の経路を一度に最適化
            pl s = targets[seg_idx];
            pl m = targets[seg_idx + 1];
            pl e = targets[seg_idx + 2];
            
            vpl old_path1 = paths[seg_idx];
            vpl old_path2 = paths[seg_idx + 1];
            
            // ランダムな中間点を通る経路を生成
            pl via;
            if (rnd.next_double() < 0.5 && !usedCells.empty()) {
                // 既存の使用マスを中間点に
                auto it = usedCells.begin();
                advance(it, rnd.next_int(usedCells.size()));
                via = it->first;
            } else {
                // ランダムな点
                via = {rnd.next_int(N), rnd.next_int(N)};
            }
            
            // BFSで3区間の経路を計算
            auto bfs_path = [&](pl start, pl goal) -> vpl {
                ll NN = N * N;
                vector<int> parent(NN, -1);
                queue<ll> q;
                ll s_id = cell_id(start.first, start.second);
                ll g_id = cell_id(goal.first, goal.second);
                q.push(s_id);
                parent[s_id] = s_id;
                
                while (!q.empty() && parent[g_id] == -1) {
                    ll curr_id = q.front();
                    q.pop();
                    pl curr = id_to_cell(curr_id);
                    ll ci = curr.first, cj = curr.second;
                    vector<pl> dirs = {{-1,0}, {1,0}, {0,-1}, {0,1}};
                    
                    for (auto [di, dj] : dirs) {
                        ll ni = ci + di, nj = cj + dj;
                        if (ni < 0 || ni >= N || nj < 0 || nj >= N) continue;
                        
                        // 壁チェック
                        if (di == -1 && ci > 0 && h[ci-1][cj] == '1') continue;
                        if (di == 1 && ci < N-1 && h[ci][cj] == '1') continue;
                        if (dj == -1 && cj > 0 && v[ci][cj-1] == '1') continue;
                        if (dj == 1 && cj < N-1 && v[ci][cj] == '1') continue;
                        
                        ll next_id = cell_id(ni, nj);
                        if (parent[next_id] != -1) continue;
                        parent[next_id] = curr_id;
                        q.push(next_id);
                    }
                }
                
                if (parent[g_id] == -1) return {};
                vpl path;
                ll step = g_id;
                while (true) {
                    path.push_back(id_to_cell(step));
                    if (step == parent[step]) break;
                    step = parent[step];
                }
                reverse(all(path));
                return path;
            };
            
            vpl path_sm = bfs_path(s, m);
            vpl path_me = bfs_path(m, e);
            
            if (path_sm.empty() || path_me.empty()) continue;
            
            // 経路を更新
            paths[seg_idx] = path_sm;
            paths[seg_idx + 1] = path_me;
            
            // T回制約チェック
            ll total_moves = 0;
            for (const auto& path : paths) {
                total_moves += max(0LL, (ll)path.size() - 1);
            }
            
            if (total_moves > T) {
                // 制約違反なので元に戻す
                paths[seg_idx] = old_path1;
                paths[seg_idx + 1] = old_path2;
                continue;
            }
            
            // usedCellsを更新
            usedCells.clear();
            for (const auto& path : paths) {
                for (const auto& cell : path) {
                    usedCells[cell]++;
                }
            }
            
            auto [newDist, newUnique] = calcMetrics();
            ll new_score = newUnique * 1000 + newDist;
            
            if (new_score < best_score || rnd.next_double() < exp((best_score - new_score) / temp)) {
                best_score = new_score;
                best_paths = paths;
                
                if (iter % 1000 == 0) {
                    cerr << "Iter " << iter << ": unique=" << newUnique 
                         << ", dist=" << newDist << ", score=" << new_score << endl;
                }
            } else {
                // 元に戻す
                paths[seg_idx] = old_path1;
                paths[seg_idx + 1] = old_path2;
                usedCells.clear();
                for (const auto& path : paths) {
                    for (const auto& cell : path) {
                        usedCells[cell]++;
                    }
                }
            }
        }
        
        paths = best_paths;
        usedCells.clear();
        for (const auto& path : paths) {
            for (const auto& cell : path) {
                usedCells[cell]++;
            }
        }
        
        cerr << "Global route optimization: " << iter << " iterations" << endl;
        cerr << "Best score: " << best_score << endl;
    }
};

struct Board {
    ll colorCount = 0;  // 塗られた色種類の数
    vvl colors;         // 盤面の色情報

    Board() {
        colors.resize(N, vl(N, 0));
    }

    // 指定したマスを塗り、方向と長さに従って連続するマスも塗る(長さの指定がない場合はそのマスだけ塗る)
    void paint(ll i, ll j, ll color, char direction = 'U', ll length = 1) {
        colors[i][j] = color;
        if(color > colorCount) colorCount = color;
        if (length ==  1) return;
        ll di = 0, dj = 0;
        if (direction == 'U') di = -1;
        else if (direction == 'D') di = 1;
        else if (direction == 'L') dj = -1;
        else if (direction == 'R') dj = 1;

        rep(step, 1, length) {
            i += di;
            j += dj;
            colors[i][j] = color;
        }
    }

    ll getColorCount() {
        return colorCount;
    }
    

    // 盤面の色を出力
    void print() {
        rep(i, 0, N) {
            rep(j, 0, N) {
                cout << colors[i][j] << " ";
            }
            cout << "\n";
        }
    }
};

struct Rule {
    ll C;               // 使う色の数
    ll Q;               // 内部状態の数
    ll M;               // 遷移規則の数
    set<pl> states;     // 遷移規則で使う状態の集合
    map<pl, ll> A;      // 新しく塗る色
    map<pl, ll> S;      // 内部状態
    map<pl, char> D;    // 移動方向 U, D, L, R, S(停止)
    Rule() {
        C = 1;
        Q = 1;
        M = 0;
    }


    void printInfo() {
        M = states.size();   // 遷移規則の数
        for (const auto& state : states) {
            C = max(C, A[state] + 1);
            Q = max(Q, S[state] + 1);
        }
        cout << C << " " << Q << " " << M << "\n";
    }

    void printStates() {
        for (const auto& state : states) {
            cout << state.first << " " << state.second << " " << A[state] << " " << S[state] << " " << D[state] << "\n";
        }
    }

    // 状態を追加
    void addState(ll color, ll internalState, ll a, ll s, char d) {
        pl state = {color, internalState};
        M++;
        C = max(C, a + 1);
        Q = max(Q, s + 1);
        states.insert(state);
        A[state] = a;
        S[state] = s;
        D[state] = d;
    }


    
    // 色の再割り当てで色数を削減（状態は固定）
    void optimizeColorOnly(double tl, Board& board) {
        double start = get_time();
        double deadline = tl;
        if (deadline <= start) return;
        double duration = max(1e-9, deadline - start);
        
        // 現在使われている全ての色を収集
        set<ll> used_colors;
        for (const auto& [state, _] : A) {
            used_colors.insert(state.first); // 入力色
            used_colors.insert(A[state]);    // 出力色
        }
        
        vector<ll> color_list(used_colors.begin(), used_colors.end());
        ll current_C = color_list.size();
        ll best_C = current_C;
        
        // 初期マッピング（恒等写像）
        map<ll, ll> color_map;
        for (ll c : color_list) {
            color_map[c] = c;
        }
        map<ll, ll> best_color_map = color_map;
        
        // バックアップ
        auto backup_A = A;
        auto backup_S = S;
        auto backup_D = D;
        auto backup_states = states;
        auto backup_colors = board.colors;
        
        ll iter = 0;
        ll accepted = 0;
        
        while (get_time() < deadline) {
            double now = get_time();
            double time = min(1.0, (now - start) / duration);
            double temp = 20.0 * (1.0 - time);
            iter++;
            
            // 2つの色を選んでマージを試みる
            if (color_list.size() < 2) break;
            
            ll c1_idx = rnd.next_int(color_list.size());
            ll c2_idx = rnd.next_int(color_list.size());
            if (c1_idx == c2_idx) continue;
            
            ll c1 = color_list[c1_idx];
            ll c2 = color_list[c2_idx];
            
            // 一時的なマッピング：c2をc1に統合
            map<ll, ll> temp_map = color_map;
            for (auto& [orig_c, mapped_c] : temp_map) {
                if (mapped_c == c2) {
                    temp_map[orig_c] = c1;
                }
            }
            
            // 新しいルールを構築して衝突チェック
            map<pair<ll, ll>, tuple<ll, ll, char>> new_rules;
            bool conflict = false;
            
            for (const auto& [state, _] : backup_A) {
                ll orig_col = state.first;
                ll state_num = state.second;
                ll new_col = temp_map[orig_col];
                
                ll orig_a = backup_A[state];
                ll new_a = temp_map[orig_a];
                ll s = backup_S[state];
                char d = backup_D[state];
                
                auto key = make_pair(new_col, state_num);
                
                if (new_rules.count(key)) {
                    auto [existing_a, existing_s, existing_d] = new_rules[key];
                    // 同じ入力(色, 状態)に対して異なる出力があれば衝突
                    if (existing_a != new_a || existing_s != s || existing_d != d) {
                        conflict = true;
                        break;
                    }
                } else {
                    new_rules[key] = {new_a, s, d};
                }
            }
            
            if (conflict) continue;
            
            // 新しい色数を計算
            set<ll> new_unique_colors;
            for (const auto& [orig_c, mapped_c] : temp_map) {
                new_unique_colors.insert(mapped_c);
            }
            ll new_C = new_unique_colors.size();
            
            // 受理判定
            bool accept = false;
            if (new_C < current_C) {
                accept = true;
            } else if (new_C == current_C && rnd.next_double() < exp((current_C - new_C) / temp)) {
                accept = true;
            }
            
            if (accept) {
                color_map = temp_map;
                current_C = new_C;
                accepted++;
                
                if (new_C < best_C) {
                    best_C = new_C;
                    best_color_map = temp_map;
                    cerr << "Color improved: C=" << new_C << " at iter " << iter << endl;
                }
            }
        }
        
        cerr << "Color optimization: " << iter << " iterations, " << accepted << " accepted" << endl;
        cerr << "C: " << C << " -> " << best_C << endl;
        
        // 最良のマッピングを適用
        color_map = best_color_map;
        
        // ルールを完全に再構築
        map<pl, ll> new_A;
        map<pl, ll> new_S;
        map<pl, char> new_D;
        set<pl> new_states;
        map<pair<ll, ll>, bool> added;
        
        for (const auto& [state, _] : backup_A) {
            ll orig_col = state.first;
            ll state_num = state.second;
            ll new_col = color_map[orig_col];
            
            ll orig_a = backup_A[state];
            ll new_a = color_map[orig_a];
            ll s = backup_S[state];
            char d = backup_D[state];
            
            pl new_state = {new_col, state_num};
            auto key = make_pair(new_col, state_num);
            
            if (!added[key]) {
                new_states.insert(new_state);
                new_A[new_state] = new_a;
                new_S[new_state] = s;
                new_D[new_state] = d;
                added[key] = true;
            }
        }
        
        states = new_states;
        A = new_A;
        S = new_S;
        D = new_D;
        C = best_C;
        M = states.size();
        
        // 盤面の色も更新
        rep(i, 0, N) {
            rep(j, 0, N) {
                if (color_map.count(backup_colors[i][j])) {
                    board.colors[i][j] = color_map[backup_colors[i][j]];
                }
            }
        }
        
        // 色数を再計算
        ll max_c = 0;
        for (const auto& [state, _] : A) {
            max_c = max(max_c, state.first);
            max_c = max(max_c, A[state]);
        }
        C = max_c + 1;
        
        cerr << "Final: C=" << C << ", Q=" << Q << ", M=" << M << endl;
    }
};







// 経路に基づいてルールと盤面を生成
void MakeRuleAndBoardByPath(Rule &rule, Board &board, Route &route) {
    auto dir_from = [&](const pl& a, const pl& b) -> char {
        if (b.first == a.first - 1) return 'U';
        if (b.first == a.first + 1) return 'D';
        if (b.second == a.second - 1) return 'L';
        return 'R';
    };
    auto dir2col = [&](char d) -> ll {
        if (d == 'U') return 0;
        if (d == 'D') return 1;
        if (d == 'L') return 2;
        return 3; // 'R'
    };

    ll numPaths = route.paths.size();

    // 各pathの各マスでの「次に進む方向」を収集
    map<pl, map<ll, char>> cellDirs;
    vector<vector<pair<pl,char>>> pathSteps(numPaths);
    rep(p, 0, numPaths) {
        const auto &path = route.paths[p];
        if (path.size() >= 2) {
            rep(i, 0, (ll)path.size()-1) {
                pl c = path[i];
                char d = dir_from(path[i], path[i+1]);
                pathSteps[p].push_back({c, d});
                cellDirs[c][p] = d;
            }
        }
    }
    
    // 交差点: 同一セルで複数pathが異なる方向を要求
    set<pl> crossCells;
    for (auto &kv : cellDirs) {
        set<char> uniq;
        for (auto &pd : kv.second) uniq.insert(pd.second);
        if ((ll)uniq.size() >= 2) crossCells.insert(kv.first);
    }

    // 経路間の干渉チェック
    auto pathsInterfere = [&](ll p1, ll p2) -> bool {
        set<pl> cells1;
        for (const auto& cell : route.paths[p1]) {
            cells1.insert(cell);
        }
        for (const auto& cell : route.paths[p2]) {
            if (cells1.count(cell)) return true;
        }
        return false;
    };

    // グラフ彩色で経路に状態IDを割り当て
    vector<ll> pathColor(numPaths, -1);
    ll maxColor = 0;
    
    rep(p, 0, numPaths) {
        set<ll> usedColors;
        rep(q, 0, p) {
            if (pathsInterfere(p, q)) {
                usedColors.insert(pathColor[q]);
            }
        }
        
        ll color = 0;
        while (usedColors.count(color)) {
            color++;
        }
        pathColor[p] = color;
        maxColor = max(maxColor, color);
    }
    
    cerr << "Path coloring: " << numPaths << " paths -> " << (maxColor + 1) << " colors" << endl;

    // 状態割当
    vector<ll> stateId(numPaths);
    rep(p, 0, numPaths) {
        stateId[p] = pathColor[p] + 1;
    }

    vector<vector<int>> cellToPaths(N * N);
    rep(p, 0, numPaths) {
        for (const auto& cell : route.paths[p]) {
            cellToPaths[cell_id(cell.first, cell.second)].push_back(p);
        }
    }
    rep(idx, 0, N * N) {
        auto &vec = cellToPaths[idx];
        sort(all(vec));
        vec.erase(unique(all(vec)), vec.end());
    }

    vector<vector<ll>> cellToPathColors(N * N);
    rep(idx, 0, N * N) {
        auto &cols = cellToPathColors[idx];
        for (int p : cellToPaths[idx]) {
            cols.push_back(pathColor[p]);
        }
        sort(all(cols));
        cols.erase(unique(all(cols)), cols.end());
    }

    // 特別色の最適化：グラフ彩色を使って最小色数で割り当て
    
    // 1. 特別色が必要なセルを収集
    set<pl> specialCells;
    
    // 交差点
    for (auto &c : crossCells) {
        specialCells.insert(c);
    }
    
    // 各経路の開始マス（状態切替のため）
    rep(p, 0, numPaths) {
        if (!route.paths[p].empty()) {
            specialCells.insert(route.paths[p][0]);
        }
    }
    
    // 2. 特別色セル間の干渉グラフを構築
    // 2つの特別色セルが「同時に使われる可能性がある」場合、異なる色が必要
    auto hasCommon = [&](const auto& a, const auto& b) -> bool {
        size_t i = 0, j = 0;
        while (i < a.size() && j < b.size()) {
            if (a[i] == b[j]) return true;
            if (a[i] < b[j]) i++;
            else j++;
        }
        return false;
    };

    auto specialCellsInterfere = [&](const pl& c1, const pl& c2) -> bool {
        ll id1 = cell_id(c1.first, c1.second);
        ll id2 = cell_id(c2.first, c2.second);
        const auto& paths1 = cellToPaths[id1];
        const auto& paths2 = cellToPaths[id2];
        if (!paths1.empty() && !paths2.empty() && hasCommon(paths1, paths2)) return true;
        const auto& colors1 = cellToPathColors[id1];
        const auto& colors2 = cellToPathColors[id2];
        if (!colors1.empty() && !colors2.empty() && hasCommon(colors1, colors2)) return true;
        return false;
    };
    
    // 3. グラフ彩色で特別色を割り当て
    map<pl, ll> specialColor;
    ll nextColor = 4; // 0..3 は U/D/L/R
    
    vector<pl> specialCellList(specialCells.begin(), specialCells.end());
    vector<ll> cellColor(specialCellList.size(), -1);
    
    rep(i, 0, (ll)specialCellList.size()) {
        set<ll> usedColors;
        
        // 干渉する既割当セルの色を収集
        rep(j, 0, i) {
            if (specialCellsInterfere(specialCellList[i], specialCellList[j])) {
                usedColors.insert(cellColor[j]);
            }
        }
        
        // 使われていない最小の色を割り当て
        ll color = 0;
        while (usedColors.count(color)) {
            color++;
        }
        cellColor[i] = color;
        specialColor[specialCellList[i]] = nextColor + color;
    }
    
    // 実際に使用された特別色の最大値
    ll maxSpecialColor = 0;
    for (auto &kv : specialColor) {
        maxSpecialColor = max(maxSpecialColor, kv.second);
    }
    
    cerr << "Special color optimization: " << specialCellList.size() 
         << " cells -> " << (maxSpecialColor - 3) << " special colors" << endl;

    // 盤面を事前塗り
    rep(p, 0, numPaths) {
        for (auto &st : pathSteps[p]) {
            pl c = st.first;
            char d = st.second;
            if (specialColor.count(c)) {
                board.paint(c.first, c.second, specialColor[c]);
            } else {
                board.paint(c.first, c.second, dir2col(d));
            }
        }
        
        if (route.paths[p].size() <= 1 && !route.paths[p].empty()) {
            pl s = route.paths[p][0];
            if (specialColor.count(s)) {
                board.paint(s.first, s.second, specialColor[s]);
            }
        }
    }

    // 通し経路（実際に使う遷移のみ生成）
    struct Step { pl c; char d; ll p; bool isStart; };
    vector<Step> seq;
    seq.reserve(1024);
    rep(p, 0, numPaths) {
        auto &ps = pathSteps[p];
        rep(i, 0, (ll)ps.size()) {
            seq.push_back(Step{ps[i].first, ps[i].second, p, i==0});
        }
    }

    ll useSteps = min<ll>(T, (ll)seq.size());

    map<pair<ll,ll>, tuple<ll,ll,char>> ruleDef;
    auto add_rule_once = [&](ll col, ll sIn, ll a, ll sOut, char d) {
        auto key = make_pair(col, sIn);
        auto it = ruleDef.find(key);
        if (it == ruleDef.end()) {
            ruleDef[key] = make_tuple(a, sOut, d);
            rule.addState(col, sIn, a, sOut, d);
        }
    };

    ll curState = 0;
    rep(k, 0, useSteps) {
        auto st = seq[k];
        ll col = specialColor.count(st.c) ? specialColor[st.c] : dir2col(st.d);
        ll sIn = curState;
        ll sOut = curState;

        if (st.isStart) {
            sOut = stateId[st.p];
        }

        add_rule_once(col, sIn, col, sOut, st.d);
        curState = sOut;
    }
    
    cerr << "Generated rules: C=" << rule.C << ", Q=" << rule.Q << ", M=" << rule.M << endl;
}




struct Estimator {
    Board board;
    Rule rule;

    pl currentPosition;
    ll currentState;

    ll score;

    Estimator(Board &b, Rule &r) : board(b), rule(r) {
        currentPosition = targets[0];
        currentState = 0;
        calculateScore();

    }

    void calculateScore() {
        ll V = estimate();
        score = scoreDecider(V);
    }

    ll getScore() {
        return score;
    }

    // スコア計算(Vは到達した目的地の数)
    ll scoreDecider(ll V) {
        if(V == K) return rule.C + rule.Q;
        return N*N * (2*N*N + K - V);
    }

    // ルールに則って盤面を進め、目的地に到達した数を返す
    ll estimate() {
        Board backupBoard = board;
        pl backupPos = currentPosition;
        ll backupState = currentState;
        ll V = 0;
        rep(t, 0, T) {
            // 目的地に到達したか確認
            if(currentPosition == targets[V]) {
                V++;
                if(V == K) break;
            }

            ll i = currentPosition.first;
            ll j = currentPosition.second;
            pl state = {board.colors[i][j], currentState};

            if (rule.states.find(state) == rule.states.end()) {
                break; // 現在の状態に対応するルールがない場合、終了
            }

            // ルールに従って行動
            ll newColor = rule.A[state];
            ll newState = rule.S[state];
            char direction = rule.D[state];
            board.paint(i, j, newColor);
            currentState = newState;
            // 位置の更新
            if (direction == 'U' && i > 0 && h[i - 1][j] == '0') {
                currentPosition.first--;
            } else if (direction == 'D' && i < N - 1 && h[i][j] == '0') {
                currentPosition.first++;
            } else if (direction == 'L' && j > 0 && v[i][j - 1] == '0') {
                currentPosition.second--;
            } else if (direction == 'R' && j < N - 1 && v[i][j] == '0') {
                currentPosition.second++;
            }
        }
        board = backupBoard;
        currentPosition = backupPos;
        currentState = backupState;
        return V;
    }
};


void solve(int seed = -1) {
    if (seed != -1) {
        std::ostringstream oss;
        oss << std::setw(4) << std::setfill('0') << seed;
        string input_filename = "in/" + oss.str() + ".txt";
        string src_filename = __FILE__;
        size_t last_slash = src_filename.find_last_of("/\\");
        string base = (last_slash == string::npos) ? src_filename : src_filename.substr(last_slash + 1);
        size_t under = base.find('_');
        string X = (under == string::npos) ? base : base.substr(0, under);
        string output_filename = "out/" + X + "_" + oss.str() + ".txt";
        freopen(input_filename.c_str(), "r", stdin);
        freopen(output_filename.c_str(), "w", stdout);
    }

    Input();
    Route route;
    route.DecideRoute();

    // 1. 経路の焼きなまし（0.4秒）
    double time_limit = get_time() + 0.9;
    route.optimizeGlobalRoute(time_limit);
    route.mergePaths();
    
    Rule rule;
    Board board;
    MakeRuleAndBoardByPath(rule, board, route);
    
    cerr << "Initial: C=" << rule.C << ", Q=" << rule.Q << ", score=" << (rule.C + rule.Q) << endl;

    // 2. 色の焼きなまし（0.4秒）
    double time2 = get_time() + 0.4;
    rule.optimizeColorOnly(time2, board);
    
    cerr << "Final: C=" << rule.C << ", Q=" << rule.Q << ", score=" << (rule.C + rule.Q) << endl;
    
    rule.printInfo();
    board.print();
    rule.printStates();
}

// union2から変更
int main(){
    solve();
}