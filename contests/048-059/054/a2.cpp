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
                    searched[nx][ny] = 1;             // 確認済みにする
                    if (mapInfo[nx][ny] == 0) break;  // 木に到達したら終了
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

// 木の配置位置をランダムに選ぶ関数
vector<pair<ll, ll>> findOptimalBlockingPositions(Map& map) {
    vector<pair<ll, ll>> result;

    // 未確認かつ花のマスでない空きマスを全て集める
    vector<pair<ll, ll>> candidateBlocks;
    for (ll i = 0; i < map.N; i++) {
        for (ll j = 0; j < map.N; j++) {
            if (map.mapInfo[i][j] == 1 &&            // 空きマス
                map.searched[i][j] == 0 &&           // 未確認のマス
                pair<ll, ll>(i, j) != map.target) {  // 伝説の花の位置ではない
                candidateBlocks.push_back({i, j});
            }
        }
    }

    // 候補がなければ何もしない
    if (candidateBlocks.empty()) return result;

    // 木の配置前の実際のマップでの最短距離
    vvl originalDist = map.calcShortestPath(map.curr);
    ll originalShortestDist = originalDist[map.target.first][map.target.second];
    if (originalShortestDist == -1) return result;  // 既に到達不可能な場合

    // シードを固定してシャッフル
    mt19937 gen(42);
    shuffle(candidateBlocks.begin(), candidateBlocks.end(), gen);

    // 最大10の木を配置
    int maxTrees = min(10, (int)candidateBlocks.size());
    for (int i = 0; i < maxTrees; i++) {
        auto pos = candidateBlocks[i];

        // 一時的に木を配置（実際のマップには反映するが、勇者の認識には反映しない）
        map.mapInfo[pos.first][pos.second] = 0;

        // 実際のマップで経路が完全に遮断されないか確認
        vvl newDist = map.calcShortestPath(map.curr);
        ll newShortestDist = newDist[map.target.first][map.target.second];

        if (newShortestDist != -1) {
            // 経路が残っているので採用
            result.push_back(pos);
        } else {
            // 経路が遮断されるので元に戻す
            map.mapInfo[pos.first][pos.second] = 1;
        }
    }

    return result;
}

int main() {
    Input in;
    Map map(in, false);

    if(!map.test) {
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

    return 0;
}