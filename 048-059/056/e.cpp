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


// 変更点: pair<ll, ll> (pl) を unordered_map のキーにするためのハッシュ関数
struct PairHash {
    template <class T1, class T2>
    std::size_t operator () (const std::pair<T1, T2> &p) const {
        auto h1 = std::hash<T1>{}(p.first);
        auto h2 = std::hash<T2>{}(p.second);
        // 簡易的なXOR (より良いハッシュ関数も存在する)
        return h1 ^ (h2 << 1); 
    }
};


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

// 変更点: BFSの高速化 (map を vector<vector> に変更)
// Route::DecideRoute と calculatePath の両方から呼び出される共通関数
vpl commonBFS(ll si, ll sj, ll gi, ll gj, const vpl& directions) {
    vpl path;
    queue<pl> q;
    // map<pl, pl> parent; -> vector<vector<pl>> に変更
    // parent[i][j] = {親のi, 親のj}
    // {-2, -2}: 未訪問, {-1, -1}: スタート地点
    vector<vvl> parent(N, vvl(N, vl(2, -2LL))); 
    
    q.push({si, sj});
    parent[si][sj] = {-1, -1};
    bool found = false;
    
    while (!q.empty() && !found) {
        pl curr = q.front();
        q.pop();
        ll i = curr.x;
        ll j = curr.y;
        
        for (const auto& dir : directions) {
            ll ni = i + dir.x;
            ll nj = j + dir.y;
            if (ni >= 0 && ni < N && nj >= 0 && nj < N) {
                // 壁チェック (変更なし)
                if (dir.x == -1 && i > 0 && h[i - 1][j] == '1') continue;
                if (dir.x == 1 && i < N - 1 && h[i][j] == '1') continue;
                if (dir.y == -1 && j > 0 && v[i][j - 1] == '1') continue;
                if (dir.y == 1 && j < N - 1 && v[i][j] == '1') continue;

                pl next = {ni, nj};
                // if (parent.find(next) == parent.end()) {
                if (parent[ni][nj][0] == -2LL) { // 未訪問チェック
                    // parent[next] = curr;
                    parent[ni][nj] = {i, j}; // 親を記録
                    q.push(next);
                    if (next == pl{gi, gj}) {
                        found = true;
                        break;
                    }
                }
            }
        }
    }
    
    pl step = {gi, gj};
    // while (step != pl{-1, -1}) {
    // 経路が見つかった場合 (parent[gi][gj] が未訪問 -2 でない)
    if (parent[step.x][step.y][0] != -2LL) {
        while (true) {
            path.push_back(step);
            ll pi = parent[step.x][step.y][0];
            ll pj = parent[step.x][step.y][1];
            if (pi == -1 && pj == -1) break; // スタート地点に到達
            step = {pi, pj};
        }
    } else if (si == gi && sj == gj) {
        path.push_back({si, sj}); // スタートとゴールが同じ
    }
    
    reverse(all(path));
    return path;
}


struct Route {
    vector<vpl> paths; // 移動経路(ターゲットからターゲットへの経路)

    Route() {}

    void DecideRoute() {
        vpl directions = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}}; // 標準の4方向
        rep(i,0,K-1) {
            ll si = targets[i].x;
            ll sj = targets[i].y;
            ll gi = targets[i+1].x;
            ll gj = targets[i+1].y;
            // 変更点: 高速化された共通BFSを呼び出す
            paths.push_back(commonBFS(si, sj, gi, gj, directions));
        }
    }
};


// 変更点: Rule構造体をリファクタリング
// 焼きなましループ内では RuleTable (unordered_map) のみを使い、
// Rule構造体は最終的な出力のためだけに使用する。
struct Rule {
    ll C;               // 使う色の数
    ll Q;               // 内部状態の数
    ll M;               // 遷移規則の数
    
    // 変更点: RuleTableの型定義 (unordered_map を使用)
    using RuleKey = pair<ll, ll>;
    using RuleValue = tuple<ll, ll, char>;
    using RuleTable = unordered_map<RuleKey, RuleValue, PairHash>;
    
    RuleTable finalRuleTable; // 最終的なルールを保持

    Rule() : C(1), Q(1), M(0) {}

    // 変更点: 最終的なRuleTableからC, Q, Mを計算し、出力用情報を設定する
    void finalize(const RuleTable& ruleTable) {
        finalRuleTable = ruleTable; // 最終結果をコピー
        M = finalRuleTable.size();
        for (const auto& [state, action] : finalRuleTable) {
            auto [a, s, d] = action;
            C = max(C, a + 1);
            Q = max(Q, s + 1);
        }
    }

    void printInfo() {
        cout << C << " " << Q << " " << M << "\n";
    }

    void printStates() {
        // 最終的なルールを出力
        for (const auto& [state, action] : finalRuleTable) {
            auto [color, internalState] = state;
            auto [a, s, d] = action;
            cout << color << " " << internalState << " " << a << " " << s << " " << d << "\n";
        }
    }
    
    // addState は MakeRuleAndBoardByPath 内の add_rule_once に置き換えられる
};

// 変更点: RuleTable の型エイリアスをグローバルに定義
using RuleTable = Rule::RuleTable;


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

// 変更点: Rule& rule を引数から削除し、RuleTable& ruleTable を引数に取る
// 経路に基づいて *Board* と *RuleTable* を生成
void MakeRuleAndBoardByPath(Board &board, RuleTable &ruleTable, Route &route) {
    auto dir_from = [&](const pl& a, const pl& b) -> char {
        if (b.first == a.first - 1) return 'U';
        if (b.first == a.first + 1) return 'D';
        if (b.second == a.second - 1) return 'L';
        return 'R';
    };
    auto dir2col = [&](char d) -> ll {
        if (d == 'U') return 1;
        if (d == 'D') return 2;
        if (d == 'L') return 3;
        return 4; // 'R'
    };

    // cell -> (pathIndex -> dir)
    // 変更点: map -> unordered_map
    // map<pl, map<ll, char>> cellDirs;
    unordered_map<pl, map<ll, char>, PairHash> cellDirs;

    vector<vector<pair<pl,char>>> pathSteps(K-1);
    rep(p, 0, K-1) {
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
    // 変更点: set -> unordered_set
    // set<pl> crossCells;
    unordered_set<pl, PairHash> crossCells;
    for (auto &kv : cellDirs) {
        set<char> uniq;
        for (auto &pd : kv.second) uniq.insert(pd.second);
        if ((ll)uniq.size() >= 2) crossCells.insert(kv.first);
    }    

    // 経路ごとの「交差を含むか」判定 (変更なし)
    vector<char> hasCross(K-1, false);
    for (auto &c : crossCells) {
        for (auto &pd : cellDirs[c]) {
            hasCross[pd.first] = true;
        }
    }

    // 状態割当: 交差を含む経路のみ専用状態を付与（0は共通状態） (変更なし)
    vector<ll> stateId(K-1, 0);
    ll nextState = 1;
    rep(p, 0, K-1) {
        if (hasCross[p]) stateId[p] = nextState++;
    }

    // 特別色の割当: 交差点 + すべての経路の開始マス
    // 変更点: map -> unordered_map
    // map<pl, ll> specialColor; 
    unordered_map<pl, ll, PairHash> specialColor;
    ll nextColor = 5; // 1..4 は U/D/L/R
    // 交差点
    for (auto &c : crossCells) {
        specialColor[c] = nextColor++;
    }
    // すべての経路の開始マスを特別色に（状態切替のため）
    rep(p, 0, K-1) {
        pl start = targets[p];
        if (!specialColor.count(start)) {
            specialColor[start] = nextColor++;
        }
    }

    // 盤面を事前塗り (変更なし)
    rep(p, 0, K-1) {
        for (auto &st : pathSteps[p]) {
            pl c = st.first;
            char d = st.second;
            if (specialColor.count(c)) {
                board.paint(c.first, c.second, specialColor[c]);
            } else {
                board.paint(c.first, c.second, dir2col(d));
            }
        }
        if (route.paths[p].size() <= 1) {
            pl s = targets[p];
            if (specialColor.count(s)) {
                board.paint(s.first, s.second, specialColor[s]);
            }
        }
    }

    // 通し経路（実際に使う遷移のみ生成） (変更なし)
    struct Step { pl c; char d; ll p; bool isStart; };
    vector<Step> seq;
    seq.reserve(1024);
    rep(p, 0, K-1) {
        auto &ps = pathSteps[p];
        rep(i, 0, (ll)ps.size()) {
            seq.push_back(Step{ps[i].first, ps[i].second, p, i==0});
        }
    }

    // T で使用遷移数を上限（未使用規則を出さない） (変更なし)
    ll useSteps = min<ll>(T, (ll)seq.size());

    // (color,state) ごとに一度だけ規則を生成
    // 変更点: ruleDef (ローカル変数) をやめ、引数の ruleTable に直接書き込む
    ruleTable.clear(); // 既存のルールをクリア
    auto add_rule_once = [&](ll col, ll sIn, ll a, ll sOut, char d) {
        auto key = make_pair(col, sIn);
        
        // ruleTable に emplace (キーが既に存在すれば何もしない)
        // これで ruleDef.find を行う必要がなくなる
        ruleTable.emplace(key, make_tuple(a, sOut, d));

        // 変更点: rule.addState(...) は不要になった
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
            if (hasCross[st.p]) {
                // 交差を含む経路 → 専用状態へ
                sOut = stateId[st.p];
            } else {
                // 交差を含まない経路 → 状態0へ（リセット）
                sOut = 0;
            }
        }

        // 実際に使う規則のみ追加
        add_rule_once(col, sIn, col, sOut, st.d);

        // 状態更新
        curState = sOut;
    }
}


// 変更点: Estimator のリファクタリング
struct Estimator {
    Board board; // Boardはシミュレーション中に変更されるのでコピーを持つ
    // 変更点: Rule& rule -> const RuleTable& ruleTable
    const RuleTable& ruleTable;

    pl currentPosition;
    ll currentState;

    // 変更点: score は不要。V_result (到達した目的地数) を保持する
    ll V_result;

    // 変更点: 引数を Rule& r -> const RuleTable& rt に変更
    Estimator(Board &b, const RuleTable &rt) : board(b), ruleTable(rt) {
        currentPosition = targets[0];
        currentState = 0;
        // 変更点: calculateScore() を削除し、estimate() を直接呼び出し V を保存
        V_result = estimate();
    }

    // 変更点: calculateScore() と getScore() は不要になった
    
    // 変更点: V_result を返すゲッター
    ll getV() {
        return V_result;
    }

    // 変更点: scoreDecider は Estimator の外 (evaluateRoute) に移動

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

            // 変更点: rule.states.find -> ruleTable.find
            // unordered_map::find なので高速 (平均 O(1))
            auto it = ruleTable.find(state);
            if (it == ruleTable.end()) {
                break; // 現在の状態に対応するルールがない場合、終了
            }

            // ルールに従って行動
            // 変更点: rule.A[state] など -> it->second から取得
            auto [newColor, newState, direction] = it->second;
            
            board.paint(i, j, newColor);
            currentState = newState;
            // 位置の更新 (変更なし)
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


#include <chrono>

// 時間取得関数 (変更なし)
double get_time() {
    using namespace std::chrono;
    return duration_cast<duration<double>>(steady_clock::now().time_since_epoch()).count();
}

// 乱数生成器 (変更なし)
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

// 経路の評価（スコアを計算）
// 変更点: リファクタリング
ll evaluateRoute(Route &route) {
    Board board;
    RuleTable ruleTable; // 評価用の RuleTable (unordered_map)
    
    // board と ruleTable を生成
    MakeRuleAndBoardByPath(board, ruleTable, route);
    
    Estimator estimator(board, ruleTable);
    ll V = estimator.getV(); // シミュレーション結果 (V) を取得

    // 変更点: scoreDecider のロジックをここに移動
    if(V == K) {
        // V=K の場合のみ、C と Q を計算する
        ll C = 1, Q = 1;
        for (const auto& [state, action] : ruleTable) {
            auto [a, s, d] = action;
            C = max(C, a + 1);
            Q = max(Q, s + 1);
        }
        return C + Q;
    } else {
        // V < K の場合は C, Q の計算は不要
        return N*N * (2*N*N + K - V);
    }
}

// 変更点: commonBFS の代わりに、重み付きBFS(Dijkstra)を定義
// 他の経路が使用するマス (occupancy) を避けようとする
vpl weightedBFS(ll si, ll sj, ll gi, ll gj, const vpl& directions, const vvi& occupancy, const ll PENALTY) {
    vpl path;
    // 優先度付きキュー (コスト, x, y)
    // コストが小さい順に取り出す (min-heap)
    priority_queue<tuple<ll, ll, ll>, 
                   vector<tuple<ll, ll, ll>>, 
                   greater<tuple<ll, ll, ll>>> pq;
    
    // (x, y) への最小コスト
    vvl min_cost(N, vl(N, -1LL)); 
    // 経路復元用の親ノード
    vector<vpl> parent(N, vpl(N, {-1LL, -1LL})); 

    pq.push({0, si, sj}); // (コスト, x, y)
    min_cost[si][sj] = 0;
    
    bool found = false;
    while (!pq.empty() && !found) {
        auto [cost, i, j] = pq.top();
        pq.pop();

        // 既により安い経路で見つかっている場合はスキップ
        if (cost > min_cost[i][j] && min_cost[i][j] != -1LL) {
            continue;
        }

        if (i == gi && j == gj) {
            found = true;
            break;
        }
        
        for (const auto& dir : directions) {
            ll ni = i + dir.x;
            ll nj = j + dir.y;
            if (ni >= 0 && ni < N && nj >= 0 && nj < N) {
                // 壁チェック (変更なし)
                if (dir.x == -1 && i > 0 && h[i - 1][j] == '1') continue;
                if (dir.x == 1 && i < N - 1 && h[i][j] == '1') continue;
                if (dir.y == -1 && j > 0 && v[i][j - 1] == '1') continue;
                if (dir.y == 1 && j < N - 1 && v[i][j] == '1') continue;

                // 変更点: コスト計算
                ll move_cost = 1; // 1マス移動するコスト
                if (occupancy[ni][nj] == 1) {
                    move_cost += PENALTY; // 他の経路が使うマスならペナルティ
                }
                ll new_cost = cost + move_cost;

                pl next = {ni, nj};
                if (min_cost[ni][nj] == -1LL || new_cost < min_cost[ni][nj]) {
                    min_cost[ni][nj] = new_cost;
                    parent[ni][nj] = {i, j};
                    pq.push({new_cost, ni, nj});
                }
            }
        }
    }
    
    // 経路復元
    if (found) {
        pl step = {gi, gj};
        while (step.x != -1LL) {
            path.push_back(step);
            step = parent[step.x][step.y];
        }
    } else if (si == gi && sj == gj) {
        path.push_back({si, sj}); // スタートとゴールが同じ
    }
    // (もし経路が見つからなかったら path は empty のまま)
    
    reverse(all(path));
    return path;
}


// BFSで経路を計算
vpl calculatePath(ll si, ll sj, ll gi, ll gj, vpl directions) {
    // 変更点: 高速化された共通BFSを呼び出す
    return commonBFS(si, sj, gi, gj, directions);
}



// 焼きなまし法
// 変更点: 交差解消ロジック と T回制約 を追加
void anneal(Route &route, double tl) {
    double start = get_time();
    tl = tl - start;
    assert(0. < tl);

    ll valid = 0;
    ll iter = 0;
    double heat = 0.;
    vector<double> log(65536);
    rep(i, 0, 65536) {
        log[i] = std::log((i + 0.5) / 65536.0);
    }
    
    std::shuffle(all(log), rnd.rng);

    // 初期スコア計算
    ll current_score = evaluateRoute(route);
    ll best_score = current_score;
    vector<vpl> best_paths = route.paths;

    // 変更点: T回制約のための「合計移動回数」を計算
    // path.size() が 1 なら 0回, 2 なら 1回, N なら N-1回
    ll current_total_moves = 0;
    for(const auto& p : route.paths) {
        current_total_moves += max(0LL, (ll)p.size() - 1);
    }

    cerr << "Initial score: " << current_score << endl;
    cerr << "Initial total moves: " << current_total_moves << " (T=" << T << ")" << endl;

    while (true) {
        double time = (get_time() - start) / tl;
        if (time >= 1.0) {
            break;
        }
        if (iter % 20 == 0) {
            const double T0 = 100000.0;
            const double T1 = 10.0;
            heat = T0 * std::pow(T1 / T0, time);
        }
        iter++;

        double add = -heat * log[iter % 65536]; // 最小化

        // 近傍解の生成: ランダムに経路を1つ選んで再計算
        ll path_idx = rnd.next_int(K - 1);
        
        // 元の経路を保存
        vpl old_path = route.paths[path_idx];
        ll old_moves = max(0LL, (ll)old_path.size() - 1);
        
        ll si = targets[path_idx].x;
        ll sj = targets[path_idx].y;
        ll gi = targets[path_idx + 1].x;
        ll gj = targets[path_idx + 1].y;
        
        vpl directions = {{-1,0},{1,0},{0,-1},{0,1}};
        std::shuffle(all(directions), rnd.rng);

        // --- 変更点: 交差解消のための「占有マップ」を作成 ---
        vvi occupancy(N, vi(N, 0)); // 0: 空き, 1: 他の経路が使用
        const ll PENALTY = N; // 交差ペナルティ (N程度が妥当)
        rep(p_idx, 0, K - 1) {
            if (p_idx == path_idx) continue; // 自分自身の経路はペナルティに入れない
            
            // 経路上のすべてのセルにペナルティ
            for (const auto& cell : route.paths[p_idx]) {
                occupancy[cell.x][cell.y] = 1;
            }
        }
        // --- 占有マップ作成完了 ---
        
        // 変更点: 重み付きBFSで新経路を計算
        vpl new_path = weightedBFS(si, sj, gi, gj, directions, occupancy, PENALTY);

        // 経路が見つからない(empty)か、変わらない場合はスキップ
        if (new_path.empty() || new_path == old_path) {
            continue;
        }
        
        // --- 変更点: T回制約のチェック ---
        ll new_moves = max(0LL, (ll)new_path.size() - 1);
        ll new_total_moves = (current_total_moves - old_moves) + new_moves;

        if (new_total_moves > T) {
            // 合計移動回数がTを超えるため、この経路は無効
            continue; 
        }
        // --- T回制約チェック完了 ---

        
        route.paths[path_idx] = new_path;
        ll new_score = evaluateRoute(route);

        if ((double)(current_score - new_score) >= add) { // 最小化
            valid++;
            current_score = new_score;
            current_total_moves = new_total_moves; // 変更点: 合計移動回数を更新
            
            if (new_score < best_score) {
                best_score = new_score;
                best_paths = route.paths;
                cerr << "Update best score: " << best_score << " (moves: " << current_total_moves << ") at iter " << iter << endl;
            }
        } else {
            // 元に戻す
            route.paths[path_idx] = old_path;
            // (current_total_moves は変更しない)
        }
    }

    // ベストな経路を採用
    route.paths = best_paths;
    
    cerr << "iter = " << iter << endl;
    cerr << "ratio = " << (double)valid / iter << endl;
    cerr << "best_score = " << best_score << endl;
}



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
    route.DecideRoute(); // 高速化されたBFSを使用
    
    // 焼きなまし法で経路を最適化（1.9秒）
    double time_limit = get_time() + 1.9;
    anneal(route, time_limit);
    
    // 変更点: 最終的な Rule と Board を生成
    Rule rule; // 最終出力用のRule
    Board board;
    RuleTable finalRuleTable;
    
    // ベストな route から最終的な board と ruleTable を生成
    MakeRuleAndBoardByPath(board, finalRuleTable, route);
    
    // ruleTable から C, Q, M を計算し、出力用に rule オブジェクトを初期化
    rule.finalize(finalRuleTable); 

    rule.printInfo();
    board.print();
    rule.printStates();
}


// 高速化
int main(){
    solve(0);
}