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

    Route() {}

    void DecideRoute() {
        rep(i,0,K-1) {
            // targets[i] から targets[i+1] への経路を決定し、pathsに追加
            vpl path;
            ll si = targets[i].x;
            ll sj = targets[i].y;
            ll gi = targets[i+1].x;
            ll gj = targets[i+1].y;

            // 採点経路をBFSで探索
            queue<pl> q;
            map<pl, pl> parent; // 経路復元用
            q.push({si, sj});
            parent[{si, sj}] = {-1, -1};
            bool found = false;
            while (!q.empty() && !found) {
                pl curr = q.front();
                q.pop();
                ll i = curr.x;
                ll j = curr.y;
                // 4方向に移動
                vector<pl> directions = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
                for (const auto& dir : directions) {
                    ll ni = i + dir.x;
                    ll nj = j + dir.y;
                    if (ni >= 0 && ni < N && nj >= 0 && nj < N) {
                        // 壁のチェック
                        if (dir.x == -1 && i > 0 && h[i - 1][j] == '1') continue; // 上
                        if (dir.x == 1 && i < N - 1 && h[i][j] == '1') continue;  // 下
                        if (dir.y == -1 && j > 0 && v[i][j - 1] == '1') continue; // 左
                        if (dir.y == 1 && j < N - 1 && v[i][j] == '1') continue;  // 右

                        pl next = {ni, nj};
                        if (parent.find(next) == parent.end()) {
                            parent[next] = curr;
                            q.push(next);
                            if (next == pl{gi, gj}) {
                                found = true;
                                break;
                            }
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

    // 通し経路を作成
    vector<pl> seq;
    seq.reserve(1); 
    if (K >= 1) seq.push_back(targets[0]);

    for (auto &path : route.paths) {
        // 各 path は開始点を含むので重複除去のため i=1 から
        for (size_t i = 1; i < path.size(); ++i) {
            seq.push_back(path[i]);
        }
    }

    // 経路長 L (= seq.size()) に応じて遷移追加
    if (seq.empty()) {
        // 何も無いケース: 盤面開始位置で停止
        rule.addState(0, 0, 0, 0, 'S');
        return;
    }

    // ステップ遷移
    for (size_t s = 0; s + 1 < seq.size(); ++s) {
        pl a = seq[s];
        pl b = seq[s + 1];
        char dir;
        if (b.first == a.first - 1) dir = 'U';
        else if (b.first == a.first + 1) dir = 'D';
        else if (b.second == a.second - 1) dir = 'L';
        else dir = 'R';

        // (色0, 内部状態s) -> (色0, 内部状態s+1, dir)
        rule.addState(0, (ll)s, 0, (ll)(s + 1), dir);
    }

    // 終端: 停止
    rule.addState(0, (ll)(seq.size() - 1), 0, (ll)(seq.size() - 1), 'S');
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
    solve(0);
}