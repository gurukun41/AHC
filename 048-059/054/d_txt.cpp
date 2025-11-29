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

// 伝説の花を中心に特殊な渦巻状に木を配置する関数
vector<pair<ll, ll>> findInitialBlockingPositions(Map& map) {
    vector<pair<ll, ll>> result;

    // 伝説の花の位置
    ll centerRow = map.target.first;
    ll centerCol = map.target.second;

    // 指定された位置に木を配置し、経路が確保されるかチェックする関数
    auto tryPlaceTree = [&](ll row, ll col) -> bool {
        // マップ範囲内かつ条件を満たすか確認
        if (row >= 0 && row < map.N && col >= 0 && col < map.N && map.mapInfo[row][col] == 1 &&  // 空きマス
            row != 0 &&                              // 勇者の初期位置がある行でない
            map.searched[row][col] == 0 &&           // 未確認のマス
            pair<ll, ll>(row, col) != map.target) {  // 伝説の花の位置ではない

            // 一時的に木を配置
            map.mapInfo[row][col] = 0;

            // 実際のマップで経路が遮断されないか確認
            vvl newDist = map.calcShortestPath(map.curr);
            if (newDist[map.target.first][map.target.second] != -1) {
                // 経路が確保されているので採用
                result.push_back({row, col});
                return true;
            } else {
                // 経路が遮断されるので元に戻す
                map.mapInfo[row][col] = 1;
            }
        }
        return false;
    };

    // 渦巻きの方向定義（8方向）
    // 上、右下、下、左下、左、左上、上、右上、右の順
    int dx[] = {-1, 1, 1, 1, 0, -1, -1, -1, 0};
    int dy[] = {0, 1, 0, -1, -1, -1, 0, 1, 1};

    // 直線方向のインデックス（下、左、上、右）
    int straightDirs[] = {2, 4, 6, 8};

    // 初期位置（花の真上）
    ll x = centerRow - 1;
    ll y = centerCol;

    // 最初の木を伝説の花の上に配置
    if (tryPlaceTree(x, y)) {
        // 配置成功
    }

    // マップの大きさに基づいてループの最大回数を設定
    ll maxIterations = map.N * map.N;

    // 繰り返し回数（1からスタート）
    int straightCount = 1;

    // 渦巻き状に木を配置
    for (int iter = 0; iter < maxIterations; iter++) {
        // 8方向を順番に処理
        for (int dir = 1; dir <= 8; dir++) {
            // 配置する木の数を決定
            int steps;
            if (dir % 2 == 1) {
                // 対角線方向は常に1
                steps = 1;
            } else {
                // 直線方向は位置に応じた数
                // 下: 1,5,9,... 左: 2,6,10,... 上: 3,7,11,... 右: 4,8,12,...
                for (int i = 0; i < 4; i++) {
                    if (dir == straightDirs[i]) {
                        steps = 4 * straightCount - 3 + i;
                        break;
                    }
                }
            }

            // この方向に指定した歩数だけ木を配置
            for (int step = 0; step < steps; step++) {
                // 次のセルの座標を計算
                x += dx[dir];
                y += dy[dir];

                // マップ外なら終了
                if (x < 0 || x >= map.N || y < 0 || y >= map.N) {
                    return result;
                }

                // 木を配置
                tryPlaceTree(x, y);
            }
        }

        // 直線方向のカウントを増やす
        straightCount++;

        // 十分な木を配置したら終了
        if (straightCount > map.N / 2) {
            break;
        }
    }

    return result;
}

// ターンごとに確認済みマスの両隣に木を配置する関数
vector<pair<ll, ll>> findOptimalBlockingPositions(Map& map) {
    vector<pair<ll, ll>> result;

    // 最初のターンなら初期パターンで配置
    if (map.curr.first == 0 && map.curr.second == map.N / 2 && map.searched[0][map.N / 2] == 1) {
        return findInitialBlockingPositions(map);
    }

    // 前のターンで確認済みになったマスを特定
    vector<pair<ll, ll>> newlySearched;
    for (ll i = 0; i < map.N; i++) {
        for (ll j = 0; j < map.N; j++) {
            if (map.searched[i][j] == 1 && map.mapInfo[i][j] == 1) {
                // 確認済みの空きマス
                newlySearched.push_back({i, j});
            }
        }
    }

    // 各確認済みマスの両隣を確認
    for (const auto& pos : newlySearched) {
        ll i = pos.first, j = pos.second;

        // 左右の隣接マスを確認
        int dx[2] = {0, 0};
        int dy[2] = {-1, 1};  // 左右

        for (int dir = 0; dir < 2; dir++) {
            ll ni = i + dx[dir], nj = j + dy[dir];

            // マップ範囲内かつ条件を満たすか確認
            if (ni >= 0 && ni < map.N && nj >= 0 && nj < map.N && map.mapInfo[ni][nj] == 1 &&  // 空きマス
                map.searched[ni][nj] == 0 &&                                                   // 未確認のマス
                pair<ll, ll>(ni, nj) != map.target &&                                          // 伝説の花の位置ではない
                ni > map.curr.first) {  // 冒険者よりも下にある位置のみ

                // 一時的に木を配置
                map.mapInfo[ni][nj] = 0;

                // 実際のマップで経路が遮断されないか確認
                vvl newDist = map.calcShortestPath(map.curr);
                if (newDist[map.target.first][map.target.second] != -1) {
                    // 経路が確保されているので採用
                    result.push_back({ni, nj});
                } else {
                    // 経路が遮断されるので元に戻す
                    map.mapInfo[ni][nj] = 1;
                }
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

    // ターンごとの処理
    while (true) {
        // 伝説の花に到達したらゲーム終了
        if (map.curr == map.target) {
            break;
        }

        // 1. 木を配置する場所を決定
        Output out;
        vector<pair<ll, ll>> blockingPositions = findOptimalBlockingPositions(map);

        out.m = blockingPositions.size();
        out.points = blockingPositions;

        // 2. 出力（木の配置を記録）
        out.print();

        // 3. 情報の更新と移動（木を配置した後）
        map.update();
    }
}

int main() {
    vl seeds = {1,2,3,4,5,9,25};
    for (int seed : seeds) {
        solve(seed, true);
    }
    return 0;
}