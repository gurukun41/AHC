#include <bits/stdc++.h>

#include <atcoder/all>
using namespace std;
using ll = long long;
using vl = vector<ll>;                                   // long long型の一次元
using vvl = vector<vl>;                                  // long long型の二次元配列
using vvvl = vector<vvl>;                                // long long型の三次元配列
using vi = vector<int>;                                  // int型の一次元
using vvi = vector<vi>;                                  // int型の二次元配列
using vvvi = vector<vvi>;                                // int型の三次元配列
#define rep(i, a, b) for (ll i = (a); i < (ll)(b); i++)  // for文の短縮
#define all(v) v.begin(), v.end()                        // all(v)でvの始まりと終わりのイテレーター

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

// 最初に受け取る入力
// 森の縦横幅と伝説の花の座標とマップ情報
struct Input {
    ll N;
    pair<ll, ll> target;
    vector<string> b;
    Input() {
        cin >> N;
        cin >> target.first >> target.second;
        b.resize(N);
        rep(i, 0, N) { cin >> b[i]; }
    }
};

// 各ターンの開始時に与えられる標準入力
// 冒険者の現在位置と前のターンに新たに確認済みとなったマスの集合
struct Input2 {
    pair<ll, ll> curr;
    ll n;
    vector<pair<ll, ll>> points;
    void getInput2() {
        cin >> curr.first >> curr.second;
        cin >> n;
        points.resize(n);
        rep(i, 0, n) { cin >> points[i].first >> points[i].second; }
    }
};

// 各ターンに出す出力
// 新たにトレントを配置するマスの集合
struct Output {
    ll m;
    vector<pair<ll, ll>> points;
    void print() {
        cout << m;
        if (m > 0) {
            cout << " ";
        }
        rep(i, 0, m) {
            cout << points[i].first << " " << points[i].second;
            if (i != m - 1) {
                cout << " ";
            }
        }
        cout << endl;
    }
};

struct Map {
    ll N;
    vvl mapInfo;               // 0: 木, 1: 空きマス
    vvl searched;              // 確認済みのマス
    pair<ll, ll> curr;         // 冒険者の現在位置
    pair<ll, ll> target;       // 伝説の花の位置
    pair<ll, ll> destination;  // 次に向かうマス(-1,-1なら未定)
    bool test;                 // 提出かどうか(trueなら提出でない)
    vector<pair<ll, ll>> q;    // ランダムに並べられたマスの順序

    Map(Input& in, bool test_ = false) : test(test_) {
        N = in.N;
        mapInfo.resize(N, vl(N, 0));
        rep(i, 0, N) {
            rep(j, 0, N) {
                if (in.b[i][j] == '.') {
                    mapInfo[i][j] = 1;
                } else {
                    mapInfo[i][j] = 0;
                }
            }
        }
        searched.resize(N, vl(N, 0));
        target = in.target;
        curr = {0, N / 2};
        destination = {-1, -1};
        searched[0][N / 2] = 1;

        if (test) {
            q.resize(N * N - 1);
            rep(i, 0, N * N - 1) {
                ll qi, qj;
                cin >> qi >> qj;
                q[i] = {qi, qj};
            }
        }
    }

    // 最短距離を計算する関数（BFS）
    vvl calcShortestPath(pair<ll, ll> start) {
        vvl dist(N, vl(N, -1));  // -1は未訪問
        queue<pair<ll, ll>> bfs;
        bfs.push(start);
        dist[start.first][start.second] = 0;

        // 上下左右の移動方向
        int dx[4] = {-1, 1, 0, 0};
        int dy[4] = {0, 0, -1, 1};

        while (!bfs.empty()) {
            auto [x, y] = bfs.front();
            bfs.pop();

            for (int i = 0; i < 4; i++) {
                int nx = x + dx[i], ny = y + dy[i];
                if (nx >= 0 && nx < N && ny >= 0 && ny < N && mapInfo[nx][ny] == 1 && dist[nx][ny] == -1) {
                    dist[nx][ny] = dist[x][y] + 1;
                    bfs.push({nx, ny});
                }
            }
        }
        return dist;
    }

    // 与えられたqの順序に基づいて到達可能な未確認マスを選択する
    pair<ll, ll> selectUnexploredCellByOrder() {
        vvl dist = calcPerceivedShortestPath(curr);  // 勇者の認識に基づく経路計算に変更

        // qの順序に従ってマスを調べる
        for (const auto& cell : q) {
            ll i = cell.first, j = cell.second;
            // 未確認かつ到達可能なマスを選択
            if (searched[i][j] == 0 && dist[i][j] != -1) {
                return {i, j};
            }
        }

        return {-1, -1};  // 適切なマスが見つからない場合
    }

    // 勇者の認識に基づく最短距離計算（未確認マスは全て空きマスとして扱う）
    vvl calcPerceivedShortestPath(pair<ll, ll> start) {
        vvl dist(N, vl(N, -1));  // -1は未訪問
        queue<pair<ll, ll>> bfs;
        bfs.push(start);
        dist[start.first][start.second] = 0;

        // 上下左右の移動方向
        int dx[4] = {-1, 1, 0, 0};
        int dy[4] = {0, 0, -1, 1};

        while (!bfs.empty()) {
            auto [x, y] = bfs.front();
            bfs.pop();

            for (int i = 0; i < 4; i++) {
                int nx = x + dx[i], ny = y + dy[i];
                if (nx >= 0 && nx < N && ny >= 0 && ny < N && dist[nx][ny] == -1) {
                    // 確認済みのマスは実際の状態、未確認のマスは全て空きマスとして扱う
                    if ((searched[nx][ny] == 1 && mapInfo[nx][ny] == 1) ||  // 確認済みの空きマス
                        (searched[nx][ny] == 0)) {                          // または未確認マス
                        dist[nx][ny] = dist[x][y] + 1;
                        bfs.push({nx, ny});
                    }
                }
            }
        }
        return dist;
    }

    void update() {
        if (!test) {
            Input2 in2;
            in2.getInput2();
            curr = in2.curr;
            rep(i, 0, in2.n) { searched[in2.points[i].first][in2.points[i].second] = 1; }
        } else {
            // 1. 現在位置に伝説の花があるか確認
            if (curr == target) {
                return;  // 目的達成、終了
            }

            // 2. 上下左右の方向に対して、現在位置から最初の「木」までの未確認マスを確認済みに
            int dx[4] = {-1, 1, 0, 0};  // 上下左右
            int dy[4] = {0, 0, -1, 1};

            rep(i, 0, 4) {
                ll nx = curr.first, ny = curr.second;
                while (nx >= 0 && nx < N && ny >= 0 && ny < N) {
                    searched[nx][ny] = 1;  // 確認済みにする
                    if (mapInfo[nx][ny] == 0) {
                        break;  // 木に到達したら終了
                    }
                    nx += dx[i];
                    ny += dy[i];
                }
                // 木のマスも確認済みにする
                if (nx >= 0 && nx < N && ny >= 0 && ny < N && mapInfo[nx][ny] == 0) {
                    searched[nx][ny] = 1;
                }
            }

            // 3. 伝説の花が確認済みマスに含まれているか
            if (searched[target.first][target.second] == 1) {
                destination = target;
            }

            // 4. 目的地が未定でなく、到達不能な場合
            if (destination.first != -1 && destination.second != -1) {
                vvl dist = calcPerceivedShortestPath(curr);  // 勇者の認識に基づく経路計算に変更
                if (dist[destination.first][destination.second] == -1) {
                    destination = {-1, -1};  // 目的地を未定に
                }
            }

            // 5. 目的地が未定または伝説の花以外の確認済みマスにある場合
            if ((destination.first == -1 && destination.second == -1) ||
                (destination != target && searched[destination.first][destination.second] == 1)) {
                destination = selectUnexploredCellByOrder();  // 指定された順序で選択
            }

            // 6. 目的地への最短距離を計算し、次に進むマスを決定
            vvl dist = calcPerceivedShortestPath(destination);  // 勇者の認識に基づく経路計算に変更
            pair<ll, ll> nextPos = curr;
            ll minDist = LLONG_MAX;

            int dx2[4] = {-1, 1, 0, 0};  // 上、下、左、右の優先順位
            int dy2[4] = {0, 0, -1, 1};

            rep(i, 0, 4) {
                ll nx = curr.first + dx2[i], ny = curr.second + dy2[i];
                if (nx >= 0 && nx < N && ny >= 0 && ny < N &&
                    ((searched[nx][ny] == 1 && mapInfo[nx][ny] == 1) || searched[nx][ny] == 0) &&  // 移動可能条件の修正
                    dist[nx][ny] != -1 && dist[nx][ny] < minDist) {
                    minDist = dist[nx][ny];
                    nextPos = {nx, ny};
                }
            }

            // 冒険者の位置を更新
            curr = nextPos;
        }
    }
};

vector<pair<ll, ll>> findInitialBlockingPositions(Map& map) {
    vector<pair<ll, ll>> result;
    ll N = map.N;
    pair<ll, ll> target = map.target;
    pair<ll, ll> curr = map.curr;
    int now = 1;
    rep(j, 0, N) {
        ll start = -1, end = -1;
        if (now == 0) {
            rep(i, 0, N) {
                if (start == -1 && map.mapInfo[i][j] == 1 && map.mapInfo[i][j-1] == 1 && j + 1 < N && map.mapInfo[i][j+1] == 1) {
                    start = i + 1;
                }
                if (end == -1 && map.mapInfo[N - 1 - i][j] == 1 && map.mapInfo[N - 1 - i][j-1] == 1 && j+1 < N && map.mapInfo[N - 1 - i][j+1] == 1) {
                    end = N - 2 - i;
                }
            }
            if (start == -1 || end == -1) {
                now = 1;
                continue;
            } else if (start > end) {
                now = 1;
                continue;
            }
        }
        rep(i, 1, N - 1) {
            if (now == 0) {
                if (i < start || i > end) continue;
                if (map.mapInfo[i][j] == 1 && pair<ll, ll>(i, j) != target) {
                    // ここ変えるかどうか
                    // 横のブロックを避ける
                    if(map.mapInfo[i+1][j-1] == 0 && map.mapInfo[i+1][j-2] == 0) {
                        continue;
                    }
                    if(map.mapInfo[i][j-1] == 0 && map.mapInfo[i][j-2] == 0) {
                        continue;
                    }
                    if(map.mapInfo[i-1][j-1] == 0 && map.mapInfo[i-1][j-2] == 0) {
                        continue;
                    }

                    // 斜めのブロックを避ける
                    if(map.mapInfo[i-1][j-1] == 0 && map.mapInfo[i][j-2] == 0) {
                        continue;
                    }
                    if(map.mapInfo[i][j-1] == 0 && map.mapInfo[i+1][j-2] == 0) {
                        continue;
                    }
                    if(i+2 < N && map.mapInfo[i+1][j-1] == 0 && map.mapInfo[i+2][j-2] == 0) {
                        continue;
                    }
                    if(map.mapInfo[i+1][j-1] == 0 && map.mapInfo[i][j-2] == 0) {
                        continue;
                    }
                    if(map.mapInfo[i][j-1] == 0 && map.mapInfo[i-1][j-2] == 0) {
                        continue;
                    }
                    if(i-2 >= 0 && map.mapInfo[i-1][j-1] == 0 && map.mapInfo[i-2][j-2] == 0) {
                        continue;
                    }


                    map.mapInfo[i][j] = 0;
                    if (map.calcShortestPath(curr)[target.first][target.second] != -1) {
                        result.push_back({i, j});
                    } else {
                        map.mapInfo[i][j] = 1;
                    }
                } 
                else if (pair<ll, ll>(i, j) == target) {
                    if (j != N - 1 && map.mapInfo[i][j + 1] == 1) {
                        map.mapInfo[i][j + 1] = 0;
                        if (map.calcShortestPath(curr)[target.first][target.second] != -1) {
                            result.push_back({i, j + 1});
                        } else {
                            map.mapInfo[i][j + 1] = 1;
                        }
                    }
                }
            } 
            else if (now == 1) {
                if (j == N - 1) break;

                // ここ変えるかどうか
                // 壁に空きマスがある時スキップ
                if(j - 1 > 0 && map.mapInfo[i][j-1] == 1) {
                    continue;
                }

                if (map.mapInfo[i][j] == 1 && pair<ll, ll>(i, j) != target && map.mapInfo[i + 1][j] == 1 &&
                    map.mapInfo[i][j + 1] == 1 && map.mapInfo[i + 1][j + 1] == 1 && map.mapInfo[i - 1][j] == 1 &&
                    map.mapInfo[i - 1][j + 1] == 1) {
                    map.mapInfo[i][j] = 0;
                    if (map.calcShortestPath(curr)[target.first][target.second] != -1) {
                        result.push_back({i, j});
                        now = 2;
                    } else {
                        map.mapInfo[i][j] = 1;
                    }
                }
            } else if (now == 2) {
                if (j == N - 1) break;

                if (map.mapInfo[i][j + 1] == 1 && pair<ll, ll>(i, j + 1) != target && map.mapInfo[i + 1][j] == 1 &&
                    map.mapInfo[i + 1][j + 1] == 1 && map.mapInfo[i][j] == 1 && map.mapInfo[i - 1][j] == 1 &&
                    map.mapInfo[i - 1][j + 1] == 1) {
                    map.mapInfo[i][j + 1] = 0;
                    if (map.calcShortestPath(curr)[target.first][target.second] != -1) {
                        result.push_back({i, j + 1});
                        now = 1;
                    } else {
                        map.mapInfo[i][j + 1] = 1;
                    }
                }
            }
        }
        if(now == 0) {
            now = 1;
        } 
        else {
            now = 0;
            j++;
        }
    }
    return result;
}

vector<pair<ll, ll>> findOptimalBlockingPositions(Map& map, bool first) {
    vector<pair<ll, ll>> result = {};



    // 冒険者が次動くことで花を観測にリーチとなるかを確認
    bool willSeeFlowerNext = false;
    vi badDirection = {};
    vi watchDirection = {};
    int dx[4] = {-1, 1, 0, 0};  // 上下左右
    int dy[4] = {0, 0, -1, 1};
    rep(i, 0, 4) {
        ll nnx = map.curr.first + dx[i], nny = map.curr.second + dy[i];
        if (nnx < 0 || nnx >= map.N || nny < 0 || nny >= map.N) continue;
        if (map.mapInfo[nnx][nny] == 0) continue;  // 木
        rep(j, 0, 4) {
            ll nx = nnx, ny = nny;
            while (nx >= 0 && nx < map.N && ny >= 0 && ny < map.N) {
                if (pair<ll, ll>(nx, ny) == map.target) {
                    willSeeFlowerNext = true;
                    badDirection.push_back(i);
                    watchDirection.push_back(j);
                    break;
                }
                else {
                    for(int k = 0; k < 4; k++) {
                        if (nx + dx[k] == map.target.first && ny + dy[k] == map.target.second) {
                            willSeeFlowerNext = true;
                            badDirection.push_back(i);
                            watchDirection.push_back(j);
                            break;
                        }
                    }
                }
                if(willSeeFlowerNext) break;
                if (map.mapInfo[nx][ny] == 0) {
                    break;  // 木に到達したら終了
                }
                nx += dx[j];
                ny += dy[j];
            }
        }
    }

    // 冒険者が次動くことで花を観測できるなら、その方向に木を配置
    if (willSeeFlowerNext) {
        rep(k, 0, badDirection.size()) {
            int i = badDirection[k];
            int j = watchDirection[k];
            ll nx = map.curr.first + dx[i], ny = map.curr.second + dy[i];
            while (nx >= 0 && nx < map.N && ny >= 0 && ny < map.N) {
                if (map.mapInfo[nx][ny] == 1 && map.searched[nx][ny] == 0 && pair<ll, ll>(nx, ny) != map.target) {
                    // 一時的に木を配置
                    map.mapInfo[nx][ny] = 0;

                    // 実際のマップで経路が遮断されないか確認
                    vvl newDist = map.calcShortestPath(map.curr);
                    if (newDist[map.target.first][map.target.second] != -1) {
                        // 経路が確保されているので採用
                        result.push_back({nx, ny});
                        break;
                    } else {
                        // 経路が遮断されるので元に戻す
                        map.mapInfo[nx][ny] = 1;
                    }
                }
                if (map.mapInfo[nx][ny] == 0) {
                    break;  // 木に到達したら終了
                }
                nx += dx[j];
                ny += dy[j];
            }
        }
    }

    return result;
}

void solve(int seed = -1, bool test = false) {
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

    Input in;
    Map map(in, test);

    if (!map.test) {
        map.update();
    }

    bool first = true;
    // ターンごとの処理
    while (true) {
        // 伝説の花に到達したらゲーム終了
        if (map.curr == map.target) {
            break;
        }

        // 1. 木を配置する場所を決定
        Output out;
        vector<pair<ll, ll>> blockingPositions = findOptimalBlockingPositions(map, first);
        if (first) first = false;

        out.m = blockingPositions.size();
        out.points = blockingPositions;

        // 2. 出力（木の配置を記録）
        out.print();

        // 3. 情報の更新と移動（木を配置した後）
        map.update();
    }
}

int main() {
    vl seeds = {1};
    for (int seed : seeds) {
        solve(seed, true);
    }
    return 0;
}