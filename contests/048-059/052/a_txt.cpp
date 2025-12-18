#include <bits/stdc++.h>

#include <atcoder/all>
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

int main() {
    int seed = 0;
    std::ostringstream oss;
    oss << std::setw(4) << std::setfill('0') << seed;
    string input_filename = "in/" + oss.str() + ".txt";
    string output_filename = "out/a_" + oss.str() + ".txt";
    freopen(input_filename.c_str(), "r", stdin);
    freopen(output_filename.c_str(), "w", stdout);
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
    vector<char> put = {'U', 'D', 'L', 'R', 'S'};
    rep(i, 0, 5) rep(j, 0, M) out.c[i][j] = put[i];
    rep(i, 5, K) rep(j, 0, M) out.c[i][j] = put[rand() % 5];
    Map mp(N, in.p);
    greedy_search(in, out, mp);
    out.print();
}