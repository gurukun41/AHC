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

        // 移動先がなくなった場合は、BFSで未訪問マスへの経路を探索
        if (best_new_cells <= 0) {
            // 全ロボットのBFSによる経路を格納
            vector<vector<char>> paths(in.M);
            vector<bool> has_path(in.M, false);
            bool any_path_found = false;

            // 各ロボットについて最短経路を探索
            rep(j, 0, in.M) {
                queue<pair<Point, vector<char>>> q;
                vector<vector<bool>> visited(in.N, vector<bool>(in.N, false));
                q.push({robots[j], {}});
                visited[robots[j].x][robots[j].y] = true;

                while (!q.empty()) {
                    auto [current, path] = q.front();
                    q.pop();

                    // 未訪問マスを見つけた場合
                    if (!mp.grid[current.x][current.y]) {
                        paths[j] = path;
                        has_path[j] = true;
                        any_path_found = true;
                        break;
                    }

                    // 4方向を探索
                    rep(k, 0, 4) {  // S（止まる）は除外
                        ll nx = current.x + dir[k].first;
                        ll ny = current.y + dir[k].second;

                        // 壁判定と有効性チェック
                        if (!is_valid(nx, ny)) continue;
                        if (k == 0 && (current.x == 0 || in.h[current.x - 1][current.y])) continue;
                        if (k == 1 && (current.x == in.N - 1 || in.h[current.x][current.y])) continue;
                        if (k == 2 && (current.y == 0 || in.v[current.x][current.y - 1])) continue;
                        if (k == 3 && (current.y == in.N - 1 || in.v[current.x][current.y])) continue;
                        if (visited[nx][ny]) continue;

                        vector<char> new_path = path;
                        new_path.push_back(dir_c[k]);
                        q.push({{nx, ny}, new_path});
                        visited[nx][ny] = true;
                    }
                }
            }

            // 経路が見つからなかった場合は終了
            if (!any_path_found) {
                break;
            }

            // 最短経路を持つロボットを見つける
            int min_path_length = INT_MAX;
            int best_robot = -1;

            rep(j, 0, in.M) {
                if (has_path[j] && (int)paths[j].size() < min_path_length) {
                    min_path_length = paths[j].size();
                    best_robot = j;
                }
            }

            if (best_robot == -1) break;  // 経路が見つからない場合

            // 最短経路を持つロボットの経路を復元して移動
            for (int step = 0; step < min_path_length; step++) {
                // 最短経路を持つロボットの移動方向
                char target_move = paths[best_robot][step];

                // この移動方向を実現する最適なパターンを選択
                int best_pattern_step = -1;

                // まず、目標のロボットが正しく移動できるパターンを見つける
                rep(pattern, 0, in.K) {
                    if (out.c[pattern][best_robot] == target_move) {
                        best_pattern_step = pattern;
                        break;
                    }
                }

                // 適切なパターンが見つからなかった場合の対応
                if (best_pattern_step == -1) {
                    // ランダムに選択するか、特別なパターンを用意しておく
                    best_pattern_step = rand() % in.K;
                }

                // 選択したパターンを追加
                out.a.push_back(best_pattern_step);

                // 実際に移動
                rep(j, 0, in.M) {
                    char move = out.c[best_pattern_step][j];
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
                mp.all_filled = true;
                rep(i, 0, in.N) rep(j, 0, in.N) if (!mp.grid[i][j]) mp.all_filled = false;
                if (mp.all_filled) break;

                count++;
                if (count >= 2 * in.N * in.N) break;
            }

            // 経路に沿った移動が終了したら次のイテレーションへ
            continue;
        }

        // 通常の移動（新しいマスを訪問できる場合）
        out.a.push_back(best_pattern);

        rep(j, 0, in.M) {
            char move = out.c[best_pattern][j];
            int k = find(dir_c.begin(), dir_c.end(), move) - dir_c.begin();
            ll nx = robots[j].x + dir[k].first;
            ll ny = robots[j].y + dir[k].second;

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
        mp.all_filled = true;
        rep(i, 0, in.N) rep(j, 0, in.N) if (!mp.grid[i][j]) mp.all_filled = false;

        count++;
    }
}

// ロボットを位置に基づいてグループ分けする関数
vector<int> group_robots_by_distance(const Input& in, int k) {
    vector<int> groups(in.M, -1);  // -1は未割り当て

    // 各ロボットの距離行列を計算
    vector<vector<double>> distances(in.M, vector<double>(in.M));
    for (int i = 0; i < in.M; i++) {
        for (int j = 0; j < in.M; j++) {
            double dx = in.p[i].x - in.p[j].x;
            double dy = in.p[i].y - in.p[j].y;
            distances[i][j] = sqrt(dx * dx + dy * dy);
        }
    }

    // 距離が近いロボットペアを取得してソート
    vector<tuple<double, int, int>> robot_pairs;
    for (int i = 0; i < in.M; i++) {
        for (int j = i + 1; j < in.M; j++) {
            robot_pairs.push_back({distances[i][j], i, j});
        }
    }
    sort(robot_pairs.begin(), robot_pairs.end());

    // 距離が近いペアから順に異なるグループに割り当て
    for (auto& pair : robot_pairs) {
        int i = get<1>(pair);
        int j = get<2>(pair);

        // 距離が近いペアは異なるグループに
        if (groups[i] == -1 && groups[j] == -1) {
            groups[i] = 0;
            groups[j] = 1;
        } else if (groups[i] == -1) {
            // 使われていないグループを見つける
            set<int> used_groups;
            for (int g = 0; g < k; g++) used_groups.insert(g);
            for (int r = 0; r < in.M; r++) {
                if (groups[r] != -1 && distances[i][r] < in.N / 3.0) {  // 近いロボットのグループを除外
                    used_groups.erase(groups[r]);
                }
            }

            if (!used_groups.empty()) {
                groups[i] = *used_groups.begin();
            } else {
                // 使えるグループがなければランダム
                groups[i] = rand() % k;
            }
        } else if (groups[j] == -1) {
            // 使われていないグループを見つける
            set<int> used_groups;
            for (int g = 0; g < k; g++) used_groups.insert(g);
            for (int r = 0; r < in.M; r++) {
                if (groups[r] != -1 && distances[j][r] < in.N / 3.0) {  // 近いロボットのグループを除外
                    used_groups.erase(groups[r]);
                }
            }

            if (!used_groups.empty()) {
                groups[j] = *used_groups.begin();
            } else {
                // 使えるグループがなければランダム
                groups[j] = rand() % k;
            }
        }
    }

    // 未割り当てのロボットを割り当て
    for (int i = 0; i < in.M; i++) {
        if (groups[i] == -1) {
            // 使われていないグループを見つける
            set<int> used_groups;
            for (int g = 0; g < k; g++) used_groups.insert(g);
            for (int r = 0; r < in.M; r++) {
                if (groups[r] != -1 && distances[i][r] < in.N / 3.0) {  // 近いロボットのグループを除外
                    used_groups.erase(groups[r]);
                }
            }

            if (!used_groups.empty()) {
                groups[i] = *used_groups.begin();
            } else {
                // 使えるグループがなければランダム
                groups[i] = rand() % k;
            }
        }
    }

    return groups;
}

// 修正したパターン変更による焼きなまし法
Output anneal_pattern(const Input& in, Output& initial_out) {
    // 初期解と初期パターン
    Output current_out = initial_out;
    Output best_out = current_out;

    // 現在の初期スコア計算
    Map current_map(in.N, in.p);
    greedy_search(in, current_out, current_map);
    int current_score = calc_score(in, current_out, current_map);
    bool current_all_filled = current_map.all_filled;
    int best_score = current_score;

    // 時間制限とパラメータ設定
    auto start = std::chrono::steady_clock::now();
    const double TIME_LIMIT = 1.5;  // 秒
    const double INITIAL_TEMPERATURE = 1000.0;
    const double FINAL_TEMPERATURE = 10.0;

    // 乱数生成器
    std::random_device rd;
    std::mt19937 gen(rd());
    vector<char> dir_c = {'U', 'D', 'L', 'R', 'S'};

    while (true) {
        // 経過時間チェック
        auto now = std::chrono::steady_clock::now();
        double elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count() / 1000.0;
        if (elapsed > TIME_LIMIT) break;

        // 温度を線形に減少
        double temperature = INITIAL_TEMPERATURE - (INITIAL_TEMPERATURE - FINAL_TEMPERATURE) * (elapsed / TIME_LIMIT);

        // 近傍解の生成：パターンを変更
        Output new_out = current_out;

        // 変更するパターン番号とロボット番号をランダムに選択
        int pattern_id = std::uniform_int_distribution<int>(0, in.K - 1)(gen);
        int robot_id = std::uniform_int_distribution<int>(0, in.M - 1)(gen);

        // 現在と異なる方向をランダムに選択
        char current_dir = new_out.c[pattern_id][robot_id];
        char new_dir;
        do {
            new_dir = dir_c[std::uniform_int_distribution<int>(0, 4)(gen)];
        } while (new_dir == current_dir);

        // パターンを変更
        new_out.c[pattern_id][robot_id] = new_dir;

        // 指示列を再構築
        new_out.a.clear();  // 既存の指示列をクリア
        Map new_map(in.N, in.p);
        greedy_search(in, new_out, new_map);

        // スコア計算
        int new_score = calc_score(in, new_out, new_map);
        bool new_all_filled = new_map.all_filled;

        // 「全マスが埋まる→埋まらない」への変更は常に拒否
        if (current_all_filled && !new_all_filled) {
            continue;  // この変更は受け入れない
        }

        // スコア差分
        int delta = new_score - current_score;

        // 解の更新判定
        if (delta > 0 || (delta <= 0 && exp(delta / temperature) > std::uniform_real_distribution<double>(0, 1)(gen))) {
            current_out = new_out;
            current_score = new_score;
            current_all_filled = new_all_filled;

            if (current_score > best_score) {
                best_out = current_out;
                best_score = current_score;
            }
        }
    }

    return best_out;
}

// めも焼きなまし
int main() {
    string input_filename = "in/input.txt";
    string output_filename = "out/j_input.txt";
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

    // 基本パターン
    vector<char> put = {'U', 'D', 'L', 'R', 'S'};
    rep(i, 0, 4) rep(j, 0, M) out.c[i][j] = put[i];

    // 距離ベースのグループ分け（4グループ）
    vector<int> distance_groups = group_robots_by_distance(in, 4);

    // グループに基づくパターン（各グループは異なる方向に移動）
    rep(i, 4, K) {
        vi rand_nums;
        rep(j, 0, 4) rand_nums.push_back(rand() % 4);
        rep(j, 0, M) {
            switch (distance_groups[j]) {
                case 0:
                    out.c[i][j] = put[rand_nums[0]];
                    break;
                case 1:
                    out.c[i][j] = put[rand_nums[1]];
                    break;
                case 2:
                    out.c[i][j] = put[rand_nums[2]];
                    break;
                case 3:
                    out.c[i][j] = put[rand_nums[3]];
                    break;
                default:
                    out.c[i][j] = 'S';
                    break;
            }
        }
    }

    // 貪欲法で初期解を生成
    Map mp(N, in.p);
    greedy_search(in, out, mp);

    // パターン変更による焼きなましで解を改善
    Output annealed_out = anneal_pattern(in, out);

    // 最終的な出力
    annealed_out.print();

    return 0;
}