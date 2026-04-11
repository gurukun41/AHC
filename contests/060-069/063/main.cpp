#include <bits/stdc++.h>
#include <atcoder/all>
using namespace std;
using ll = long long;
using vi = vector<int>;
using vvi = vector<vi>;

struct Pos {
    int x, y;
};

constexpr int DX[4] = {-1, 1, 0, 0}; // U D L R
constexpr int DY[4] = {0, 0, -1, 1};
constexpr char DC[4] = {'U', 'D', 'L', 'R'};

int N, M, C;
vi d;
vvi food;

deque<Pos> snakePos;   // head -> tail
deque<int> snakeColor;

inline bool inBoard(int x, int y) {
    return (0 <= x && x < N && 0 <= y && y < N);
}

inline bool isUTurn(int nx, int ny) {
    if ((int)snakePos.size() < 2) return false;
    return snakePos[1].x == nx && snakePos[1].y == ny;
}

uint64_t rng64() {
    static uint64_t x = 88172645463325252ULL;
    x ^= x << 7;
    x ^= x >> 9;
    return x;
}

struct State {
    int cost; // ペナルティ込みのコスト
    int dist; // 実際の移動歩数（ターン数）
    int x, y;
    int first_dir; // 最初の1手（上書きを防ぐためStateに持たせる）
    bool operator>(const State& o) const { return cost > o.cost; }
};

// 自分の体を噛みちぎる（不要なゴミを吐き出す）ための方向を探す
int getDirToBite(int pref) {
    int hx = snakePos[0].x, hy = snakePos[0].y;
    int k = snakePos.size();

    vvi snake_idx(N, vi(N, -1));
    for (int i = 0; i < k; i++) {
        snake_idx[snakePos[i].x][snakePos[i].y] = i;
    }

    // 空間 + 時間(歩数) でコストを管理する。最大250歩まで先読み
    vector<vvi> cost(260, vvi(N, vi(N, 1e9)));
    priority_queue<State, vector<State>, greater<State>> pq;

    for (int dir = 0; dir < 4; dir++) {
        int nx = hx + DX[dir], ny = hy + DY[dir];
        if (!inBoard(nx, ny)) continue;
        if (isUTurn(nx, ny)) continue;

        int dist = 0; 
        if (snake_idx[nx][ny] != -1) {
            int i = snake_idx[nx][ny];
            if (i + dist + 1 < k) {
                if (i + dist + 1 < pref - 1) {
                    return dir; // 1歩目で目的の長さに噛みちぎれる
                } else {
                    continue; // ぶつかるが長すぎるので障害物扱い
                }
            }
        }

        int c = 1;
        if (food[nx][ny] > 0) c += 100; // ゴミを増やさないようになるべく空き地を通る

        cost[1][nx][ny] = c;
        pq.push({c, 1, nx, ny, dir});
    }

    while (!pq.empty()) {
        auto [c, dist, x, y, f_dir] = pq.top();
        pq.pop();

        if (dist >= 250) continue;
        if (c > cost[dist][x][y]) continue;

        for (int dir = 0; dir < 4; dir++) {
            int nx = x + DX[dir], ny = y + DY[dir];
            if (!inBoard(nx, ny)) continue;

            bool is_bite = false;
            if (snake_idx[nx][ny] != -1) {
                int i = snake_idx[nx][ny];
                if (i + dist + 1 < k) {
                    if (i + dist + 1 < pref - 1) {
                        is_bite = true; // ゴール条件達成
                    } else {
                        continue; // 障害物
                    }
                }
            }

            if (is_bite) return f_dir;

            int nc = c + 1;
            if (food[nx][ny] > 0) nc += 100;

            if (cost[dist + 1][nx][ny] > nc) {
                cost[dist + 1][nx][ny] = nc;
                pq.push({nc, dist + 1, nx, ny, f_dir});
            }
        }
    }
    return -1;
}

// 必要な色を探す（尻尾の動きを考慮）
int getDirToColor(int target_color) {
    int hx = snakePos[0].x, hy = snakePos[0].y;
    int k = snakePos.size();

    vvi snake_idx(N, vi(N, -1));
    for (int i = 0; i < k; i++) {
        snake_idx[snakePos[i].x][snakePos[i].y] = i;
    }

    vector<vvi> cost(260, vvi(N, vi(N, 1e9)));
    priority_queue<State, vector<State>, greater<State>> pq;

    for (int dir = 0; dir < 4; dir++) {
        int nx = hx + DX[dir], ny = hy + DY[dir];
        if (!inBoard(nx, ny)) continue;
        if (isUTurn(nx, ny)) continue;

        int dist = 0;
        if (snake_idx[nx][ny] != -1) {
            int i = snake_idx[nx][ny];
            if (i + dist + 1 < k) continue; // まだ尻尾が通り過ぎていない
        }

        int c = 1;
        if (food[nx][ny] > 0 && food[nx][ny] != target_color) c += 100;

        cost[1][nx][ny] = c;
        pq.push({c, 1, nx, ny, dir});
    }

    while (!pq.empty()) {
        auto [c, dist, x, y, f_dir] = pq.top();
        pq.pop();

        if (dist >= 250) continue;
        if (c > cost[dist][x][y]) continue;
        if (food[x][y] == target_color) return f_dir;

        for (int dir = 0; dir < 4; dir++) {
            int nx = x + DX[dir], ny = y + DY[dir];
            if (!inBoard(nx, ny)) continue;

            if (snake_idx[nx][ny] != -1) {
                int i = snake_idx[nx][ny];
                if (i + dist + 1 < k) continue; // 障害物
            }

            int nc = c + 1;
            if (food[nx][ny] > 0 && food[nx][ny] != target_color) nc += 100;

            if (cost[dist + 1][nx][ny] > nc) {
                cost[dist + 1][nx][ny] = nc;
                pq.push({nc, dist + 1, nx, ny, f_dir});
            }
        }
    }
    return -1;
}

// 妥協して一番近い何らかの餌を探す
int getDirToAnyFood() {
    int hx = snakePos[0].x, hy = snakePos[0].y;
    int k = snakePos.size();

    vvi snake_idx(N, vi(N, -1));
    for (int i = 0; i < k; i++) {
        snake_idx[snakePos[i].x][snakePos[i].y] = i;
    }

    vector<vvi> cost(260, vvi(N, vi(N, 1e9)));
    priority_queue<State, vector<State>, greater<State>> pq;

    for (int dir = 0; dir < 4; dir++) {
        int nx = hx + DX[dir], ny = hy + DY[dir];
        if (!inBoard(nx, ny)) continue;
        if (isUTurn(nx, ny)) continue;

        int dist = 0;
        if (snake_idx[nx][ny] != -1) {
            int i = snake_idx[nx][ny];
            if (i + dist + 1 < k) continue;
        }

        cost[1][nx][ny] = 1;
        pq.push({1, 1, nx, ny, dir});
    }

    while (!pq.empty()) {
        auto [c, dist, x, y, f_dir] = pq.top();
        pq.pop();

        if (dist >= 250) continue;
        if (c > cost[dist][x][y]) continue;
        if (food[x][y] > 0) return f_dir;

        for (int dir = 0; dir < 4; dir++) {
            int nx = x + DX[dir], ny = y + DY[dir];
            if (!inBoard(nx, ny)) continue;

            if (snake_idx[nx][ny] != -1) {
                int i = snake_idx[nx][ny];
                if (i + dist + 1 < k) continue;
            }

            int nc = c + 1;
            if (cost[dist + 1][nx][ny] > nc) {
                cost[dist + 1][nx][ny] = nc;
                pq.push({nc, dist + 1, nx, ny, f_dir});
            }
        }
    }
    return -1;
}

// 広く動ける安全な方向（スタック回避用）
int getSafeDir() {
    int hx = snakePos[0].x, hy = snakePos[0].y;
    int k = snakePos.size();
    
    vvi snake_idx(N, vi(N, -1));
    for (int i = 0; i < k; i++) {
        snake_idx[snakePos[i].x][snakePos[i].y] = i;
    }

    int best_dir = -1;
    int max_reach = -1;

    for (int start_d = 0; start_d < 4; start_d++) {
        int nx = hx + DX[start_d], ny = hy + DY[start_d];
        if (!inBoard(nx, ny)) continue;
        if (isUTurn(nx, ny)) continue;
        if (snake_idx[nx][ny] != -1 && snake_idx[nx][ny] + 1 < k) continue;

        vector<vvi> visited(260, vvi(N, vi(N, 0)));
        queue<State> q;
        q.push({0, 1, nx, ny, start_d});
        visited[1][nx][ny] = 1;
        int reach = 0;

        while (!q.empty()) {
            auto [c, dist, x, y, f_dir] = q.front();
            q.pop();
            reach++;
            
            if (dist >= 250) continue; // 250歩生き延びられれば十分安全

            for (int d = 0; d < 4; d++) {
                int nnx = x + DX[d], nny = y + DY[d];
                if (!inBoard(nnx, nny)) continue;
                
                if (snake_idx[nnx][nny] != -1 && snake_idx[nnx][nny] + dist + 1 < k) continue;
                
                if (!visited[dist + 1][nnx][nny]) {
                    visited[dist + 1][nnx][nny] = 1;
                    q.push({0, dist + 1, nnx, nny, start_d});
                }
            }
        }

        if (reach > max_reach) {
            max_reach = reach;
            best_dir = start_d;
        }
    }

    if (best_dir == -1) {
        for (int d = 0; d < 4; d++) {
            int nx = hx + DX[d], ny = hy + DY[d];
            if (inBoard(nx, ny) && !isUTurn(nx, ny)) return d;
        }
        return 0;
    }
    return best_dir;
}

// 完全に行き詰まった時のランダムウォーク用
int getRandomSafeDir() {
    int hx = snakePos[0].x, hy = snakePos[0].y;
    int k = snakePos.size();

    vi valid_dirs;
    for (int d = 0; d < 4; d++) {
        int nx = hx + DX[d], ny = hy + DY[d];
        if (!inBoard(nx, ny)) continue;
        if (isUTurn(nx, ny)) continue;
        
        bool blocked = false;
        for (int i = 0; i < k; i++) {
            if (snakePos[i].x == nx && snakePos[i].y == ny) {
                if (i + 1 < k) blocked = true;
            }
        }
        if (!blocked) valid_dirs.push_back(d);
    }
    if (!valid_dirs.empty()) {
        return valid_dirs[rng64() % valid_dirs.size()];
    }
    for (int d = 0; d < 4; d++) {
        int nx = hx + DX[d], ny = hy + DY[d];
        if (inBoard(nx, ny) && !isUTurn(nx, ny)) return d;
    }
    return 0;
}

void applyMove(int dir) {
    int hx = snakePos.front().x, hy = snakePos.front().y;
    int nx = hx + DX[dir], ny = hy + DY[dir];

    snakePos.push_front({nx, ny});
    snakePos.pop_back();

    if (food[nx][ny] != 0) {
        int c = food[nx][ny];
        food[nx][ny] = 0;
        snakePos.push_back(snakePos.back());
        snakeColor.push_back(c);
    }

    int k = (int)snakePos.size();
    int h = -1;
    for (int i = 1; i <= k - 2; i++) {
        if (snakePos[i].x == snakePos[0].x && snakePos[i].y == snakePos[0].y) {
            h = i;
            break;
        }
    }

    if (h != -1) {
        for (int p = h + 1; p < k; p++) {
            int x = snakePos[p].x, y = snakePos[p].y;
            food[x][y] = snakeColor[p];
        }
        while ((int)snakePos.size() > h + 1) snakePos.pop_back();
        while ((int)snakeColor.size() > h + 1) snakeColor.pop_back();
    }
}

int main() {
    cin.tie(0);
    ios::sync_with_stdio(0);

    cin >> N >> M >> C;
    d.resize(M);
    for (int i = 0; i < M; i++) cin >> d[i];

    food.assign(N, vi(N));
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            cin >> food[i][j];
        }
    }

    snakePos.push_back({4, 0});
    snakePos.push_back({3, 0});
    snakePos.push_back({2, 0});
    snakePos.push_back({1, 0});
    snakePos.push_back({0, 0});
    snakeColor = deque<int>(5, 1);

    for (auto &p : snakePos) food[p.x][p.y] = 0;

    int pref = 5;
    vi ans;

    int no_progress_turns = 0;
    int random_walk_turns = 0;

    while (ans.size() < 100000) {
        int remaining_food = 0;
        for (int i = 0; i < N; i++) {
            for (int j = 0; j < N; j++) {
                if (food[i][j] > 0) remaining_food++;
            }
        }
        if (remaining_food == 0) break; // 全て食べきった

        // スタックしている場合は体をほどく
        if (no_progress_turns > 150) {
            random_walk_turns = 10;
            no_progress_turns = 0;
        }

        int dir = -1;
        if (random_walk_turns > 0) {
            dir = getRandomSafeDir();
            random_walk_turns--;
        } else {
            if ((int)snakePos.size() > pref) {
                // Phase 1: 不要な部分（ゴミ）を切り落とす
                dir = getDirToBite(pref);
                if (dir == -1) dir = getSafeDir();
            } else {
                // Phase 2: 次に必要な色の餌を取りに行く
                if (pref < M) {
                    dir = getDirToColor(d[pref]);
                    if (dir == -1) dir = getDirToAnyFood();
                    if (dir == -1) dir = getSafeDir();
                } else {
                    // 全て揃ったら残りの餌を適当に回収する
                    dir = getDirToAnyFood();
                    if (dir == -1) dir = getSafeDir();
                }
            }
        }

        int nx = snakePos[0].x + DX[dir];
        int ny = snakePos[0].y + DY[dir];
        int ate_color = 0;
        if (inBoard(nx, ny) && food[nx][ny] > 0) {
            ate_color = food[nx][ny];
        }

        int len_before = snakePos.size();
        applyMove(dir);
        int len_after = snakePos.size();

        int old_pref = pref;

        if (len_after > len_before) {
            // 食事をして長さが伸びた
            if (len_before == pref && pref < M && ate_color == d[pref]) {
                pref++; // 目標の色だったのでプレフィックスが1伸びる
            }
        } else if (len_after < len_before) {
            // 噛みちぎりが発生して長さが縮んだ
            if (pref > len_after) {
                pref = len_after; // 必要な部分まで切ってしまった場合はprefを下げる
            }
        }

        if (pref == old_pref) no_progress_turns++;
        else no_progress_turns = 0;

        ans.push_back(dir);
    }

    for (int dir : ans) {
        cout << DC[dir] << '\n';
    }

    return 0;
}