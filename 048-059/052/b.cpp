#include <bits/stdc++.h>

#include <atcoder/all>
#include <chrono>
using namespace std;
using ll = long long;
using vl = vector<ll>;                                     // long long型の一次元
using vvl = vector<vl>;                                    // long long型の二次元配列
using vvvl = vector<vvl>;                                  // long long型の三次元配列
using vi = vector<int>;                                    // int型の一次元
using vvi = vector<vi>;                                    // int型の二次元配列
using vvvi = vector<vvi>;                                  // int型の三次元配列
#define rep(i, a, b) for (int i = (a); i < (int)(b); i++)  // for文の短縮
#define all(v) v.begin(), v.end()                          // all(v)でvの始まりと終わりのイテレーター

// 入力を受け取る
template <typename T>
T input() {
    T x;
    cin >> x;
    return x;
}

// a,bのうち最大のものをaに入れる(aがbに置き換わるときはtrueを返す)
template <typename T>
inline bool chmax(T& a, const T& b) {
    if (a < b) {
        a = b;
        return true;
    }
    return false;
}

// a,bのうち最小のものをaに入れる(aがbに置き換わるときはtrueを返す)
template <typename T>
inline bool chmin(T& a, const T& b) {
    if (a > b) {
        a = b;
        return true;
    }
    return false;
}

// 素数判定
bool is_prime(long long n) {
    if (n <= 1) {
        return false;
    }
    for (long long i = 2; i * i <= n; i++) {
        if (n % i == 0) {
            return false;
        }
    }
    return true;
}

struct Point {
    ll x;
    ll y;
};

struct Input {
    ll N;
    ll M;
    ll K;
    vector<Point> p;
    vvi v;
    vvi h;
    Input(ll N_ = 30, ll M_ = 10, ll K_ = 10) : N(N_), M(M_), K(K_), p(N_), v(N_, vi(N_ - 1)), h(N_ - 1, vi(N_)) {};
};

struct Output {
    vector<vector<char>> c;
    vector<int> a;
    Output(ll K, ll M) : c(K, vector<char>(M)), a(0) {};

    void print() {
        rep(i, 0, c.size()) {
            rep(j, 0, c[i].size()) {
                cout << c[i][j];
                if (j == c[i].size() - 1) {
                    cout << "\n";
                } else {
                    cout << " ";
                }
            }
        }
        rep(i, 0, a.size()) { cout << a[i] << "\n"; }
    }
};

struct Map {
    ll N;
    vector<vector<bool>> grid;
    vector<Point> p;
    bool all_filled;
    Map(ll N_, vector<Point> p_) : N(N_), grid(N_, vector<bool>(N_, false)), p(p_), all_filled(false) {}
    void search_filled() {
        for (ll i = 0; i < N; i++) {
            for (ll j = 0; j < N; j++) {
                if (!grid[i][j]) {
                    all_filled = false;
                    return;
                }
            }
        }
    }

    int count_not_filled() {
        int count = 0;
        for (ll i = 0; i < N; i++) {
            for (ll j = 0; j < N; j++) {
                if (!grid[i][j]) count++;
            }
        }
        return count;
    }
};

int calc_score(const Input& in, const Output& out, Map& mp) {
    ll T = out.a.size();
    if (mp.all_filled) {
        return 3 * in.N * in.N - T;
    }
    int remain = mp.count_not_filled();
    return in.N * in.N - remain;
}

void greedy_search(const Input& in, Output& out, Map& mp) {
    int count = 0;
    vector<Point> robots = in.p;
    mp.grid = vector<vector<bool>>(in.N, vector<bool>(in.N, false));
    for (auto& p : robots) mp.grid[p.x][p.y] = true;

    auto is_valid = [&](ll x, ll y) { return 0 <= x && x < in.N && 0 <= y && y < in.N; };

    vector<pair<int, int>> dir = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}, {0, 0}};  // UDLRS
    vector<char> dir_c = {'U', 'D', 'L', 'R', 'S'};

    while (count < 2 * in.N * in.N && !mp.all_filled) {
        // 各指示パターンのシミュレーション結果を比較
        int best_pattern = 0;
        int best_new_cells = -1;

        rep(pattern, 0, in.K) {
            // このパターンを選んだ場合のシミュレーション
            vector<Point> sim_robots = robots;
            int new_cells = 0;
            vector<vector<bool>> visited = mp.grid;

            rep(j, 0, in.M) {
                char move = out.c[pattern][j];
                int k = find(dir_c.begin(), dir_c.end(), move) - dir_c.begin();
                ll nx = sim_robots[j].x + dir[k].first;
                ll ny = sim_robots[j].y + dir[k].second;

                // 壁判定
                if (k == 0 && (sim_robots[j].x == 0 || in.h[sim_robots[j].x - 1][sim_robots[j].y])) continue;
                if (k == 1 && (sim_robots[j].x == in.N - 1 || in.h[sim_robots[j].x][sim_robots[j].y])) continue;
                if (k == 2 && (sim_robots[j].y == 0 || in.v[sim_robots[j].x][sim_robots[j].y - 1])) continue;
                if (k == 3 && (sim_robots[j].y == in.N - 1 || in.v[sim_robots[j].x][sim_robots[j].y])) continue;
                if (!is_valid(nx, ny)) continue;

                sim_robots[j].x = nx;
                sim_robots[j].y = ny;
                if (!visited[nx][ny]) {
                    visited[nx][ny] = true;
                    new_cells++;
                }
            }

            // より多くの新しいセルを訪れるパターンを選ぶ
            if (new_cells > best_new_cells) {
                best_new_cells = new_cells;
                best_pattern = pattern;
            }
        }

        // 移動先がなくなった場合は終了
        if (best_new_cells <= 0) {
            break;
        }

        // 最良のパターンを選択
        out.a.push_back(best_pattern);

        // 実際の移動処理
        rep(j, 0, in.M) {
            char move = out.c[best_pattern][j];
            int k = find(dir_c.begin(), dir_c.end(), move) - dir_c.begin();
            ll nx = robots[j].x + dir[k].first;
            ll ny = robots[j].y + dir[k].second;

            // 壁判定
            if (k == 0 && (robots[j].x == 0 || in.h[robots[j].x - 1][robots[j].y])) continue;
            if (k == 1 && (robots[j].x == in.N - 1 || in.h[robots[j].x][robots[j].y])) continue;
            if (k == 2 && (robots[j].y == 0 || in.v[robots[j].x][robots[j].y - 1])) continue;
            if (k == 3 && (robots[j].y == in.N - 1 || in.v[robots[j].x][robots[j].y])) continue;
            if (!is_valid(nx, ny)) continue;

            robots[j].x = nx;
            robots[j].y = ny;
            mp.grid[nx][ny] = true;
        }

        // 全マス埋まったか判定
        mp.search_filled();

        count++;
    }
}

// パターンの有効性を評価する関数
double evaluate_patterns(const Input& in, const Output& out) {
    double total_score = 0;
    int num_trials = 5;

    for (int trial = 0; trial < num_trials; trial++) {
        Map mp(in.N, in.p);
        vector<Point> robots = in.p;
        mp.grid = vector<vector<bool>>(in.N, vector<bool>(in.N, false));
        for (auto& p : robots) mp.grid[p.x][p.y] = true;

        int visited_cells = in.M;  // 初期位置のセル数
        int max_steps = 50;

        for (int step = 0; step < max_steps; step++) {
            // 各パターンを試して最も良いものを選ぶ
            int best_pattern = 0;
            int best_new_cells = -1;

            rep(pattern, 0, in.K) {
                vector<Point> sim_robots = robots;
                int new_cells = 0;
                vector<vector<bool>> sim_grid = mp.grid;

                rep(j, 0, in.M) {
                    char move = out.c[pattern][j];
                    int k = 0;
                    if (move == 'U')
                        k = 0;
                    else if (move == 'D')
                        k = 1;
                    else if (move == 'L')
                        k = 2;
                    else if (move == 'R')
                        k = 3;
                    else
                        k = 4;

                    vector<pair<int, int>> dir = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}, {0, 0}};  // UDLRS
                    ll nx = sim_robots[j].x + dir[k].first;
                    ll ny = sim_robots[j].y + dir[k].second;

                    // 壁判定
                    if (k == 0 && (sim_robots[j].x == 0 || in.h[sim_robots[j].x - 1][sim_robots[j].y])) continue;
                    if (k == 1 && (sim_robots[j].x == in.N - 1 || in.h[sim_robots[j].x][sim_robots[j].y])) continue;
                    if (k == 2 && (sim_robots[j].y == 0 || in.v[sim_robots[j].x][sim_robots[j].y - 1])) continue;
                    if (k == 3 && (sim_robots[j].y == in.N - 1 || in.v[sim_robots[j].x][sim_robots[j].y])) continue;
                    if (!(0 <= nx && nx < in.N && 0 <= ny && ny < in.N)) continue;

                    sim_robots[j].x = nx;
                    sim_robots[j].y = ny;
                    if (!sim_grid[nx][ny]) {
                        sim_grid[nx][ny] = true;
                        new_cells++;
                    }
                }

                if (new_cells > best_new_cells) {
                    best_new_cells = new_cells;
                    best_pattern = pattern;
                }
            }

            if (best_new_cells <= 0) break;

            // 最良パターンで実際に移動
            vector<pair<int, int>> dir = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}, {0, 0}};  // UDLRS
            vector<char> dir_c = {'U', 'D', 'L', 'R', 'S'};

            rep(j, 0, in.M) {
                char move = out.c[best_pattern][j];
                int k = find(dir_c.begin(), dir_c.end(), move) - dir_c.begin();
                ll nx = robots[j].x + dir[k].first;
                ll ny = robots[j].y + dir[k].second;

                if (k == 0 && (robots[j].x == 0 || in.h[robots[j].x - 1][robots[j].y])) continue;
                if (k == 1 && (robots[j].x == in.N - 1 || in.h[robots[j].x][robots[j].y])) continue;
                if (k == 2 && (robots[j].y == 0 || in.v[robots[j].x][robots[j].y - 1])) continue;
                if (k == 3 && (robots[j].y == in.N - 1 || in.v[robots[j].x][robots[j].y])) continue;
                if (!(0 <= nx && nx < in.N && 0 <= ny && ny < in.N)) continue;

                robots[j].x = nx;
                robots[j].y = ny;
                if (!mp.grid[nx][ny]) {
                    mp.grid[nx][ny] = true;
                    visited_cells++;
                }
            }
        }

        // 訪問したセルの割合をスコアとする
        total_score += (double)visited_cells / (in.N * in.N);
    }

    return total_score / num_trials;
}

void optimize_commands_with_sa(Input& in, Output& out) {
    vector<char> put = {'U', 'D', 'L', 'R', 'S'};

    // 基本パターンの初期化
    rep(i, 0, 5) rep(j, 0, in.M) out.c[i][j] = put[i];

    // 対角線パターンなど追加
    rep(j, 0, in.M) out.c[5][j] = (j % 2 == 0) ? 'U' : 'R';
    rep(j, 0, in.M) out.c[6][j] = (j % 2 == 0) ? 'D' : 'R';
    rep(j, 0, in.M) out.c[7][j] = (j % 2 == 0) ? 'U' : 'L';
    rep(j, 0, in.M) out.c[8][j] = (j % 2 == 0) ? 'D' : 'L';

    // 残りはランダム初期化
    rep(i, 9, in.K) rep(j, 0, in.M) out.c[i][j] = put[rand() % 5];

    // 最良の状態を保持
    auto best_c = out.c;
    double best_score = evaluate_patterns(in, out);

    // 時間ベースの焼きなまし法
    auto start = std::chrono::steady_clock::now();
    const double TIME_LIMIT = 1.5;  // 秒
    const double INITIAL_TEMPERATURE = 10.0;
    const double FINAL_TEMPERATURE = 0.01;

    while (true) {
        auto now = std::chrono::steady_clock::now();
        double elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count() / 1000.0;
        if (elapsed > TIME_LIMIT) break;

        // 温度を線形に減少させる
        double temperature = INITIAL_TEMPERATURE - (INITIAL_TEMPERATURE - FINAL_TEMPERATURE) * (elapsed / TIME_LIMIT);

        // 現在の状態を保存
        auto current_c = out.c;

        // 近傍操作: パターンの一部をランダムに変更
        int pattern = rand() % in.K;
        int num_changes = 1 + rand() % (in.M / 2);

        for (int i = 0; i < num_changes; i++) {
            int robot = rand() % in.M;
            out.c[pattern][robot] = put[rand() % 5];
        }

        // 新しい状態を評価
        double new_score = evaluate_patterns(in, out);
        double delta = new_score - best_score;

        // スコアが改善したか、確率的に受理
        if (delta > 0 || exp(delta / temperature) > (double)rand() / RAND_MAX) {
            if (delta > 0) {
                best_c = out.c;
                best_score = new_score;
            }
        } else {
            out.c = current_c;
        }
    }

    out.c = best_c;
}

int main() {
    ll N, M, K;
    cin >> N >> M >> K;
    Input in(N, M, K);
    rep(i, 0, M) cin >> in.p[i].x >> in.p[i].y;
    rep(i, 0, N) {
        string S;
        cin >> S;
        rep(j, 0, N - 1) { S[j] == '1' ? in.v[i][j] = 1 : in.v[i][j] = 0; }
    }
    rep(i, 0, N - 1) {
        string S;
        cin >> S;
        rep(j, 0, N) { S[j] == '1' ? in.h[i][j] = 1 : in.h[i][j] = 0; }
    }

    Output out(K, M);
    // 焼きなましで指示パターンを最適化
    optimize_commands_with_sa(in, out);

    // 最適化されたパターンを使って貪欲探索
    Map mp(N, in.p);
    greedy_search(in, out, mp);

    out.print();
}