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

struct Route {
    vector<vpl> paths; // 移動経路(ターゲットからターゲットへの経路)
    map<pl, ll> usedCells; // これまでに使用したマス -> 使用回数

    Route() {}

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
            
            // タイブレーカー: 未使用マスを優先(微小な値)
            auto tie_breaker = [&](ll pi, ll pj) -> ld {
                pl cell = {pi, pj};
                // 使用済みマスには使用回数に応じたペナルティ
                if (usedCells.count(cell)) {
                    return usedCells[cell] * 0.1; // 使用回数 × 0.1のペナルティ
                }
                return 0.0;
            };


            // priority_queue: (f値(double), (g値, 座標))
            priority_queue<pair<ld, pair<ll, pl>>, vector<pair<ld, pair<ll, pl>>>, greater<>> pq;
            map<pl, pl> parent; // 経路復元用
            map<pl, ll> gScore; // スタートからの実コスト（整数）
            
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
                
                // ゴール到達
                if (curr == pl{gi, gj}) {
                    found = true;
                    break;
                }
                
                // 既により良い経路で訪問済みならスキップ
                if (gScore.count(curr) && g > gScore[curr]) continue;
                
                // 4方向に移動
                vector<pl> directions = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
                for (const auto& dir : directions) {
                    ll ni = ci + dir.x;
                    ll nj = cj + dir.y;
                    if (ni >= 0 && ni < N && nj >= 0 && nj < N) {
                        // 壁のチェック
                        if (dir.x == -1 && ci > 0 && h[ci - 1][cj] == '1') continue; // 上
                        if (dir.x == 1 && ci < N - 1 && h[ci][cj] == '1') continue;  // 下
                        if (dir.y == -1 && cj > 0 && v[ci][cj - 1] == '1') continue; // 左
                        if (dir.y == 1 && cj < N - 1 && v[ci][cj] == '1') continue;  // 右

                        pl next = {ni, nj};
                        ll tentative_g = g + 1; // 実コストは常に1
                        
                        // より良い経路が見つかった場合のみ更新
                        if (!gScore.count(next) || tentative_g < gScore[next]) {
                            gScore[next] = tentative_g;
                            ld f_next = tentative_g + heuristic(ni, nj) + tie_breaker(ni, nj);
                            parent[next] = curr;
                            pq.push({f_next, {tentative_g, next}});
                        }
                    }
                }
            }
            
            // 経路復元
            pl step = {gi, gj};
            while (step != pl{-1, -1}) {
                path.push_back(step);
                step = parent[step];
            }
            reverse(all(path));
            paths.push_back(path);
            
            // 使用したマスを記録(使用回数をカウント)
            for (const auto& cell : path) {
                usedCells[cell]++;
            }
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
        if (d == 'U') return 1;
        if (d == 'D') return 2;
        if (d == 'L') return 3;
        return 4; // 'R'
    };

    // 各pathの各マスでの「次に進む方向」を収集（終点は除く）
    // cell -> (pathIndex -> dir)
    map<pl, map<ll, char>> cellDirs;
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
    // 改善版: マスが複数経路で使われる場合も交差とみなす
    set<pl> crossCells;
    for (auto &kv : cellDirs) {
        if (kv.second.size() >= 2) {  // 2つ以上の経路がこのマスを使用
            crossCells.insert(kv.first);
        }
    }  

    // 経路ごとの「交差を含むか」判定
    vector<char> hasCross(K-1, false);
    for (auto &c : crossCells) {
        for (auto &pd : cellDirs[c]) {
            hasCross[pd.first] = true;
        }
    }

    // 状態割当: 交差を含む経路のみ専用状態を付与（0は共通状態）
    vector<ll> stateId(K-1, 0);
    ll nextState = 1;
    rep(p, 0, K-1) {
        if (hasCross[p]) stateId[p] = nextState++;
    }

    // 特別色の割当: 交差点 + すべての経路の開始マス
    map<pl, ll> specialColor; // 座標 -> 特別色
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

    // 盤面を事前塗り
    // - 特別色セルは特別色
    // - それ以外は「次に進む方向」の基本色
    rep(p, 0, K-1) {
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
        if (route.paths[p].size() <= 1) {
            pl s = targets[p];
            if (specialColor.count(s)) {
                board.paint(s.first, s.second, specialColor[s]);
            }
        }
    }

    // 通し経路（実際に使う遷移のみ生成）
    struct Step { pl c; char d; ll p; bool isStart; };
    vector<Step> seq;
    seq.reserve(1024);
    rep(p, 0, K-1) {
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
    Rule rule;
    Board board;
    MakeRuleAndBoardByPath(rule, board, route);
    rule.printInfo();
    board.print();
    rule.printStates();
}


int main(){
    solve(147);
}