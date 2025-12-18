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
            map<pl, pl> parent;
            map<pl, ll> gScore;
            
            pl start = {si, sj};
            ld f_start = 0 + heuristic(si, sj) + tie_breaker(si, sj);
            pq.push({f_start, {0, start}});
            parent[start] = {-1, -1};
            gScore[start] = 0;
            bool found = false;
            
            while (!pq.empty() && !found) {
                auto [f, gAndCurr] = pq.top();
                pq.pop();
                ll g = gAndCurr.first;
                pl curr = gAndCurr.second;
                ll ci = curr.x;
                ll cj = curr.y;
                
                if (curr == pl{gi, gj}) {
                    found = true;
                    break;
                }
                
                if (gScore.count(curr) && g > gScore[curr]) continue;
                
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
                        ll tentative_g = g + 1;
                        
                        if (!gScore.count(next) || tentative_g < gScore[next]) {
                            gScore[next] = tentative_g;
                            ld f_next = tentative_g + heuristic(ni, nj) + tie_breaker(ni, nj);
                            parent[next] = curr;
                            pq.push({f_next, {tentative_g, next}});
                        }
                    }
                }
            }
            
            pl step = {gi, gj};
            while (step != pl{-1, -1}) {
                path.push_back(step);
                step = parent[step];
            }
            reverse(all(path));
            paths.push_back(path);
            
            for (const auto& cell : path) {
                usedCells[cell]++;
            }
        }
    }

    // 焼きなまし法で経路を最適化（重複を促進、ただし同一経路内の重複は禁止）
    void optimizeByAnnealing(double tl) {
        double start = get_time();
        tl = tl - start;
        assert(0. < tl);

        ll valid = 0;
        ll iter = 0;
        double heat = 0.;
        
        // 対数テーブルを事前計算
        vector<double> log_table(65536);
        rep(i, 0, 65536) {
            log_table[i] = std::log((i + 0.5) / 65536.0);
        }
        std::shuffle(all(log_table), rnd.rng);

        // スコア計算：使用マス数が少ないほど良い（重複を促進）
        auto calculateScore = [&]() -> ll {
            map<pl, ll> cellCount;
            for (const auto& path : paths) {
                for (const auto& cell : path) {
                    cellCount[cell]++;
                }
            }
            
            // 使用されているマスの総数（少ない方が良い）
            ll uniqueCells = cellCount.size();
            
            // 経路の合計長
            ll totalLength = 0;
            for (const auto& path : paths) {
                totalLength += max(0LL, (ll)path.size() - 1);
            }
            
            // スコア = 使用マス数 * 10000 + 合計経路長
            // 使用マス数を減らすことを優先
            return uniqueCells * 10000 + totalLength;
        };

        // 合計経路長を計算
        auto calculateTotalMoves = [&]() -> ll {
            ll total = 0;
            for (const auto& path : paths) {
                total += max(0LL, (ll)path.size() - 1);
            }
            return total;
        };

        ll current_score = calculateScore();
        ll best_score = current_score;
        vector<vpl> best_paths = paths;
        
        ll current_total_moves = calculateTotalMoves();

        cerr << "Initial unique cells score: " << current_score << endl;
        cerr << "Initial total moves: " << current_total_moves << " (T=" << T << ")" << endl;

        while (true) {
            double time = (get_time() - start) / tl;
            if (time >= 1.0) {
                break;
            }
            
            // 温度スケジュール
            if (iter % 100 == 0) {
                const double T0 = 1000000.0;
                const double T1 = 10.0;
                heat = T0 * std::pow(T1 / T0, time);
            }
            iter++;

            double add = -heat * log_table[iter % 65536];

            // ランダムに経路を1つ選んで再計算
            ll path_idx = rnd.next_int(K - 1);
            vpl old_path = paths[path_idx];
            ll old_moves = max(0LL, (ll)old_path.size() - 1);
            
            ll si = targets[path_idx].x;
            ll sj = targets[path_idx].y;
            ll gi = targets[path_idx + 1].x;
            ll gj = targets[path_idx + 1].y;
            
            // 他の経路で使われているマスをランダムに中間点として選ぶ
            ll mi, mj;
            if (rnd.next_double() < 0.7 && !usedCells.empty()) {
                // 70%の確率で既存の使用マスを中間点に
                auto it = usedCells.begin();
                advance(it, rnd.next_int(usedCells.size()));
                mi = it->first.first;
                mj = it->first.second;
            } else {
                // 30%の確率でランダムな点
                mi = rnd.next_int(N);
                mj = rnd.next_int(N);
            }
            
            // BFSで経路計算（使用済みマスを優先、ただし同一経路内での重複は禁止）
            auto bfs = [&](ll start_i, ll start_j, ll goal_i, ll goal_j) -> vpl {
                priority_queue<pair<ll, pl>, vector<pair<ll, pl>>, greater<>> pq;
                map<pl, pl> parent;
                map<pl, ll> cost;
                
                pl start = {start_i, start_j};
                pl goal = {goal_i, goal_j};
                pq.push({0, start});
                parent[start] = {-1, -1};
                cost[start] = 0;
                
                while (!pq.empty()) {
                    auto [c, curr] = pq.top();
                    pq.pop();
                    
                    if (curr == goal) break;
                    
                    if (cost.count(curr) && c > cost[curr]) continue;
                    
                    ll ci = curr.first;
                    ll cj = curr.second;
                    vector<pl> directions = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
                    
                    for (const auto& dir : directions) {
                        ll ni = ci + dir.first;
                        ll nj = cj + dir.second;
                        
                        if (ni >= 0 && ni < N && nj >= 0 && nj < N) {
                            if (dir.first == -1 && ci > 0 && h[ci - 1][cj] == '1') continue;
                            if (dir.first == 1 && ci < N - 1 && h[ci][cj] == '1') continue;
                            if (dir.second == -1 && cj > 0 && v[ci][cj - 1] == '1') continue;
                            if (dir.second == 1 && cj < N - 1 && v[ci][cj] == '1') continue;
                            
                            pl next = {ni, nj};
                            // 使用済みマスはコスト0、未使用マスはコスト100
                            ll edge_cost = usedCells.count(next) ? 0 : 100;
                            ll new_cost = c + edge_cost;
                            
                            if (!cost.count(next) || new_cost < cost[next]) {
                                cost[next] = new_cost;
                                parent[next] = curr;
                                pq.push({new_cost, next});
                            }
                        }
                    }
                }
                
                vpl path;
                pl step = goal;
                while (step != pl{-1, -1}) {
                    path.push_back(step);
                    if (!parent.count(step)) break;
                    step = parent[step];
                }
                reverse(all(path));
                return path;
            };
            
            vpl path1 = bfs(si, sj, mi, mj);
            vpl path2 = bfs(mi, mj, gi, gj);
            
            if (!path1.empty() && !path2.empty()) {
                vpl new_path = path1;
                new_path.insert(new_path.end(), path2.begin() + 1, path2.end());
                
                // 同一経路内での重複をチェック
                if (hasLoopInPath(new_path)) {
                    continue; // 重複がある場合はスキップ
                }
                
                // T回制約のチェック
                ll new_moves = max(0LL, (ll)new_path.size() - 1);
                ll new_total_moves = (current_total_moves - old_moves) + new_moves;
                
                if (new_total_moves > T) {
                    continue;
                }
                
                paths[path_idx] = new_path;
                
                // usedCellsを更新
                usedCells.clear();
                for (const auto& path : paths) {
                    for (const auto& cell : path) {
                        usedCells[cell]++;
                    }
                }
                
                ll new_score = calculateScore();
                
                if ((double)(current_score - new_score) >= add) {
                    valid++;
                    current_score = new_score;
                    current_total_moves = new_total_moves;
                    
                    if (current_score < best_score) {
                        best_score = current_score;
                        best_paths = paths;
                        
                        // 使用マス数を計算
                        map<pl, ll> cellCount;
                        for (const auto& path : paths) {
                            for (const auto& cell : path) {
                                cellCount[cell]++;
                            }
                        }
                        cerr << "Update best score: " << best_score 
                             << " (unique cells: " << cellCount.size() 
                             << ", moves: " << current_total_moves << ") at iter " << iter << endl;
                    }
                } else {
                    paths[path_idx] = old_path;
                    // usedCellsを元に戻す
                    usedCells.clear();
                    for (const auto& path : paths) {
                        for (const auto& cell : path) {
                            usedCells[cell]++;
                        }
                    }
                }
            }
        }

        paths = best_paths;
        
        // usedCellsを再計算
        usedCells.clear();
        for (const auto& path : paths) {
            for (const auto& cell : path) {
                usedCells[cell]++;
            }
        }
        
        cerr << "Annealing iter = " << iter << endl;
        cerr << "Annealing ratio = " << (double)valid / iter << endl;
        cerr << "Best score = " << best_score << endl;
        cerr << "Final total moves: " << calculateTotalMoves() << " (T=" << T << ")" << endl;
        
        // 使用マス数を出力
        map<pl, ll> cellCount;
        for (const auto& path : paths) {
            for (const auto& cell : path) {
                cellCount[cell]++;
            }
        }
        cerr << "Final unique cells: " << cellCount.size() << endl;
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

    ll numPaths = route.paths.size(); // マージ後の経路数

    // 各pathの各マスでの「次に進む方向」を収集（終点は除く）
    // cell -> (pathIndex -> dir)
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

    // 経路ごとの「交差を含むか」判定
    vector<char> hasCross(numPaths, false);
    for (auto &c : crossCells) {
        for (auto &pd : cellDirs[c]) {
            hasCross[pd.first] = true;
        }
    }

    // 経路間の干渉チェック（セルの共有があるか）
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

    // グラフ彩色問題として経路に色を割り当て
    // 干渉する経路は異なる状態を持つ必要がある
    vector<ll> pathColor(numPaths, -1);
    ll maxColor = 0;
    
    rep(p, 0, numPaths) {
        // このパスと干渉するパスが使っている色を収集
        set<ll> usedColors;
        rep(q, 0, p) {
            if (pathsInterfere(p, q)) {
                usedColors.insert(pathColor[q]);
            }
        }
        
        // 使われていない最小の色を割り当て
        ll color = 0;
        while (usedColors.count(color)) {
            color++;
        }
        pathColor[p] = color;
        maxColor = max(maxColor, color);
    }
    
    cerr << "Path coloring: " << numPaths << " paths -> " << (maxColor + 1) << " colors" << endl;

    // 状態割当: 各色ごとに1つの状態（0は初期状態として予約）
    vector<ll> stateId(numPaths);
    rep(p, 0, numPaths) {
        stateId[p] = pathColor[p] + 1; // 状態0は初期状態
    }

    // 特別色の割当: 交差点 + 各経路の開始マス
    map<pl, ll> specialColor; // 座標 -> 特別色
    ll nextColor = 4; // 0..3 は U/D/L/R
    
    // 交差点
    for (auto &c : crossCells) {
        specialColor[c] = nextColor++;
    }
    
    // 各経路の開始マスを特別色に（状態切替のため）
    rep(p, 0, numPaths) {
        if (!route.paths[p].empty()) {
            pl start = route.paths[p][0]; // 経路の最初のセル
            if (!specialColor.count(start)) {
                specialColor[start] = nextColor++;
            }
        }
    }

    // 盤面を事前塗り
    // - 特別色セルは特別色
    // - それ以外は「次に進む方向」の基本色
    rep(p, 0, numPaths) {
        // pathの各stepのセルを塗る
        for (auto &st : pathSteps[p]) {
            pl c = st.first;
            char d = st.second;
            if (specialColor.count(c)) {
                board.paint(c.first, c.second, specialColor[c]);
            } else {
                board.paint(c.first, c.second, dir2col(d));
            }
        }
        // path長が1（=移動なし）の場合でも、開始マスが特別色なら塗る
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

    // T で使用遷移数を上限（未使用規則を出さない）
    ll useSteps = min<ll>(T, (ll)seq.size());

    // (color,state) ごとに一度だけ規則を生成
    map<pair<ll,ll>, tuple<ll,ll,char>> ruleDef; // key -> (a, s, d)
    auto add_rule_once = [&](ll col, ll sIn, ll a, ll sOut, char d) {
        auto key = make_pair(col, sIn);
        auto it = ruleDef.find(key);
        if (it == ruleDef.end()) {
            ruleDef[key] = make_tuple(a, sOut, d);
            rule.addState(col, sIn, a, sOut, d);
        } else {
            // 既存と矛盾しないことを確認（矛盾は設計ミス）
            auto [ea, es, ed] = it->second;
            // 矛盾があれば何もしない（またはassert等）
            (void)ea; (void)es; (void)ed;
        }
    };

    // 実行時の状態遷移をなぞって必要な規則のみ作成
    ll curState = 0;
    rep(k, 0, useSteps) {
        auto st = seq[k];
        ll col = specialColor.count(st.c) ? specialColor[st.c] : dir2col(st.d);
        ll sIn = curState;
        ll sOut = curState;

        // 経路開始で状態を切替
        if (st.isStart) {
            // その経路の専用状態へ遷移
            sOut = stateId[st.p];
        }

        // 実際に使う規則のみ追加
        add_rule_once(col, sIn, col, sOut, st.d);

        // 状態更新
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
    
    /* 焼きなまし法で経路を最適化（1.9秒）
    double time_limit = get_time() + 1.9;
    route.optimizeByAnnealing(time_limit);*/
    
    // 最適化後に経路をマージ
    route.mergePaths();
    
    Rule rule;
    Board board;
    MakeRuleAndBoardByPath(rule, board, route);
    rule.printInfo();
    board.print();
    rule.printStates();
}

// union1から変更（統合彩色アルゴリズム)
int main(){
    solve();
}