#include <bits/stdc++.h>

#include <atcoder/all>
using namespace std;
using ll = long long;
using vl = vector<ll>;
using vvl = vector<vl>;
using vvvl = vector<vvl>;
using vi = vector<int>;
using vvi = vector<vi>;
using vvvi = vector<vvi>;
#define rep(i, a, b) for (ll i = (a); i < (ll)(b); i++)
#define all(v) v.begin(), v.end()

// --- テンプレート（変更なし） ---
template <typename T>
T input() {
    T x;
    cin >> x;
    return x;
}
template <typename T>
inline bool chmax(T& a, const T& b) {
    if (a < b) {
        a = b;
        return true;
    }
    return false;
}
template <typename T>
inline bool chmin(T& a, const T& b) {
    if (a > b) {
        a = b;
        return true;
    }
    return false;
}
bool is_prime(long long n) {
    if (n <= 1) return false;
    for (long long i = 2; i * i <= n; i++) {
        if (n % i == 0) return false;
    }
    return true;
}
// --- 構造体定義（変更なし） ---
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
struct Output {
    ll m;
    vector<pair<ll, ll>> points;
    void print() {
        cout << m;
        if (m > 0) cout << " ";
        rep(i, 0, m) {
            cout << points[i].first << " " << points[i].second;
            if (i != m - 1) cout << " ";
        }
        cout << endl;
    }
};

// --- Mapクラス（変更なし） ---
// ゲームの状態を管理するクラス
struct Map {
    ll N;
    vvl mapInfo;               // 0: 木, 1: 空きマス
    vvl searched;              // 確認済みのマス
    pair<ll, ll> curr;         // 冒険者の現在位置
    pair<ll, ll> target;       // 伝説の花の位置
    pair<ll, ll> destination;  // 次に向かうマス(-1,-1なら未定)
    bool test;                 // 提出かどうか(trueなら提出でない)
    vector<pair<ll, ll>> q;    // ランダムに並べられたマスの順序

    Map() : N(0), mapInfo(), searched(), curr({0, 0}), target({0, 0}), destination({-1, -1}), test(false), q() {}

    Map(Input& in, bool test_ = false) : test(test_) {
        N = in.N;
        mapInfo.resize(N, vl(N, 0));
        rep(i, 0, N) {
            rep(j, 0, N) {
                mapInfo[i][j] = (in.b[i][j] == '.');
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

    // 実際のマップ情報に基づいた最短距離を計算する関数（BFS）
    vvl calcShortestPath(pair<ll, ll> start) {
        if (start.first < 0 || start.first >= N || start.second < 0 || start.second >= N) return vvl(N, vl(N, -1));
        vvl dist(N, vl(N, -1));
        queue<pair<ll, ll>> bfs;
        bfs.push(start);
        dist[start.first][start.second] = 0;

        const int dx[] = {-1, 1, 0, 0};
        const int dy[] = {0, 0, -1, 1};

        while (!bfs.empty()) {
            auto [x, y] = bfs.front();
            bfs.pop();
            rep(i, 0, 4) {
                int nx = x + dx[i], ny = y + dy[i];
                if (nx >= 0 && nx < N && ny >= 0 && ny < N && mapInfo[nx][ny] == 1 && dist[nx][ny] == -1) {
                    dist[nx][ny] = dist[x][y] + 1;
                    bfs.push({nx, ny});
                }
            }
        }
        return dist;
    }

    // 冒険者の認識（未確認マスは空きマスと仮定）に基づいた最短距離計算
    vvl calcPerceivedShortestPath(pair<ll, ll> start) {
        if (start.first < 0 || start.first >= N || start.second < 0 || start.second >= N) return vvl(N, vl(N, -1));
        vvl dist(N, vl(N, -1));
        queue<pair<ll, ll>> bfs;
        bfs.push(start);
        dist[start.first][start.second] = 0;

        const int dx[] = {-1, 1, 0, 0};
        const int dy[] = {0, 0, -1, 1};

        while (!bfs.empty()) {
            auto [x, y] = bfs.front();
            bfs.pop();
            rep(i, 0, 4) {
                int nx = x + dx[i], ny = y + dy[i];
                if (nx >= 0 && nx < N && ny >= 0 && ny < N && dist[nx][ny] == -1) {
                    // 確認済みの空きマス、または未確認マスなら移動可能とみなす
                    if ((searched[nx][ny] == 1 && mapInfo[nx][ny] == 1) || (searched[nx][ny] == 0)) {
                        dist[nx][ny] = dist[x][y] + 1;
                        bfs.push({nx, ny});
                    }
                }
            }
        }
        return dist;
    }

    // qの順序に基づいて到達可能な未確認マスを選択
    pair<ll, ll> selectUnexploredCellByOrder() {
        vvl dist = calcPerceivedShortestPath(curr);
        for (const auto& cell : q) {
            if (searched[cell.first][cell.second] == 0 && dist[cell.first][cell.second] != -1) {
                return cell;
            }
        }
        return {-1, -1};
    }
    
    // ローカルテスト用の冒険者シミュレーションロジック
    void simulateAdventurerMove() {
        if (curr == target) return;

        const int dx[] = {-1, 1, 0, 0};
        const int dy[] = {0, 0, -1, 1};

        rep(i, 0, 4) {
            ll nx = curr.first, ny = curr.second;
            while (nx >= 0 && nx < N && ny >= 0 && ny < N) {
                searched[nx][ny] = 1;
                if (mapInfo[nx][ny] == 0) break;
                nx += dx[i];
                ny += dy[i];
            }
             if (nx >= 0 && nx < N && ny >= 0 && ny < N && mapInfo[nx][ny] == 0) {
                searched[nx][ny] = 1;
            }
        }

        if (searched[target.first][target.second] == 1) {
            destination = target;
        }

        if (destination.first != -1) {
            vvl dist = calcPerceivedShortestPath(curr);
            if (dist[destination.first][destination.second] == -1) {
                destination = {-1, -1};
            }
        }

        if (destination.first == -1 || (destination != target && searched[destination.first][destination.second] == 1)) {
            destination = selectUnexploredCellByOrder();
        }
        
        if (destination.first == -1) return;

        vvl dist_to_dest = calcPerceivedShortestPath(destination);
        pair<ll, ll> nextPos = curr;
        ll minDist = LLONG_MAX;

        rep(i, 0, 4) {
            ll nx = curr.first + dx[i], ny = curr.second + dy[i];
            if (nx >= 0 && nx < N && ny >= 0 && ny < N &&
                ((searched[nx][ny] == 1 && mapInfo[nx][ny] == 1) || searched[nx][ny] == 0) &&
                dist_to_dest[nx][ny] != -1 && dist_to_dest[nx][ny] < minDist) {
                minDist = dist_to_dest[nx][ny];
                nextPos = {nx, ny};
            }
        }
        curr = nextPos;
    }


    void update() {
        if (!test) {
            Input2 in2;
            in2.getInput2();
            curr = in2.curr;
            rep(i, 0, in2.n) { searched[in2.points[i].first][in2.points[i].second] = 1; }
        } else {
            simulateAdventurerMove();
        }
    }
    
    Map(const Map& other) = default;
    Map& operator=(const Map& other) = default;
};


// ビームサーチで使用する状態を表す構造体
struct BeamState {
    Map mapState;
    vector<pair<ll, ll>> placements;
    ll score;  // 評価スコア（大きいほど良い）

    BeamState() : score(0) {}

    BeamState(Map map, vector<pair<ll, ll>> place, ll s)
        : mapState(std::move(map)), placements(std::move(place)), score(s) {}

    // スコアで比較するように変更
    bool operator<(const BeamState& other) const {
        return score < other.score;
    }
};

// 評価関数：木の配置後、冒険者の認識上で現在地から目的地までの距離がどれだけ長くなるか。
// これにより、重いシミュレーションを回避し、BFSを1回呼ぶだけで評価が可能になる。
ll evaluatePlacement(Map& evaluatedMap) {
    // 冒険者の目的地がなければ評価できない
    if (evaluatedMap.destination.first == -1) {
        // 目的地を探すフェーズでは、未探索領域を狭めさせないことが重要だが、
        // ここでは単純化のため低いスコアを返す。
        return -1;
    }

    // 冒険者の認識に基づいた、現在位置から目的地までの最短経路を計算
    vvl perceived_dist = evaluatedMap.calcPerceivedShortestPath(evaluatedMap.curr);
    ll distance = perceived_dist[evaluatedMap.destination.first][evaluatedMap.destination.second];

    // 到達不能になる配置はペナルティ
    if (distance == -1) {
        return -LLONG_MAX;
    }

    // 冒険者を遠回りさせられる（認識上の距離が長くなる）ほど高評価
    return distance;
}

// 木を配置する候補を生成する関数
vector<vector<pair<ll, ll>>> generateCandidatePlacements(const Map& map, int maxCandidates = 10) {
    vector<vector<pair<ll, ll>>> candidates;
    vector<pair<ll, ll>> availableCells;
    rep(i, 0, map.N) {
        rep(j, 0, map.N) {
            if (map.mapInfo[i][j] == 1 && map.searched[i][j] == 0 &&
                pair<ll, ll>(i, j) != map.target && pair<ll, ll>(i, j) != map.curr) {
                availableCells.push_back({i, j});
            }
        }
    }

    random_device rd;
    mt19937 g(rd());
    shuffle(all(availableCells), g);

    for (const auto& cell : availableCells) {
        candidates.push_back({cell});
        if (candidates.size() >= maxCandidates) break;
    }
    // 複数の木を同時に置く候補も生成可能（今回は単純化のため1つずつ）
    return candidates;
}

// 木の配置をマップに適用する関数
void applyPlacements(Map& map, const vector<pair<ll, ll>>& placements) {
    for (const auto& p : placements) {
        map.mapInfo[p.first][p.second] = 0;
    }
}


// 初手の配置パターンを決定する関数
vector<pair<ll, ll>> findInitialBlockingPositions(Map& map) {
    vector<pair<ll, ll>> result;
    ll N = map.N;
    pair<ll, ll> target = map.target;
    pair<ll, ll> curr = map.curr;

    // この関数は非常に複雑なヒューリスティックなロジックを含んでいます。
    // 基本的なアイデアは、冒険者の進路を妨害するように、しかし花への道を完全に塞がないように
    // 壁や障害物を形成することです。'now'変数で状態を管理し、列ごとに配置パターンを切り替えています。
    int now = 1;
    rep(j, 0, N) {
        ll start = -1, end = -1;
        if (now == 0) {
            rep(i, 0, N) {
                if (start == -1 && map.mapInfo[i][j] == 1 && map.mapInfo[i][j - 1] == 1 && j + 1 < N &&
                    map.mapInfo[i][j + 1] == 1) {
                    start = i + 1;
                }
                if (end == -1 && map.mapInfo[N - 1 - i][j] == 1 && map.mapInfo[N - 1 - i][j - 1] == 1 && j + 1 < N &&
                    map.mapInfo[N - 1 - i][j + 1] == 1) {
                    end = N - 2 - i;
                }
            }
            if (start == -1 || end == -1 || start > end) {
                now = 1;
                continue;
            }
        }
        rep(i, 1, N - 1) {
            if (now == 0) {
                if (i < start || i > end) continue;
                if (map.mapInfo[i][j] == 1 && pair<ll, ll>(i, j) != target) {
                    // 横のブロックを避ける
                    if (map.mapInfo[i + 1][j - 1] == 0 && map.mapInfo[i + 1][j - 2] == 0) continue;
                    if (map.mapInfo[i][j - 1] == 0 && map.mapInfo[i][j - 2] == 0) continue;
                    if (map.mapInfo[i - 1][j - 1] == 0 && map.mapInfo[i - 1][j - 2] == 0) continue;
                    // 斜めのブロックを避ける
                    if (map.mapInfo[i - 1][j - 1] == 0 && map.mapInfo[i][j - 2] == 0) continue;
                    if (map.mapInfo[i][j - 1] == 0 && map.mapInfo[i + 1][j - 2] == 0) continue;
                    if (i + 2 < N && map.mapInfo[i + 1][j - 1] == 0 && map.mapInfo[i + 2][j - 2] == 0) continue;
                    if (map.mapInfo[i + 1][j - 1] == 0 && map.mapInfo[i][j - 2] == 0) continue;
                    if (map.mapInfo[i][j - 1] == 0 && map.mapInfo[i - 1][j - 2] == 0) continue;
                    if (i - 2 >= 0 && map.mapInfo[i - 1][j - 1] == 0 && map.mapInfo[i - 2][j - 2] == 0) continue;

                    map.mapInfo[i][j] = 0;
                    if (map.calcShortestPath(curr)[target.first][target.second] != -1) {
                        result.push_back({i, j});
                    } else {
                        map.mapInfo[i][j] = 1;
                    }
                } else if (pair<ll, ll>(i, j) == target) {
                    if (j != N - 1 && map.mapInfo[i][j + 1] == 1) {
                        map.mapInfo[i][j + 1] = 0;
                        if (map.calcShortestPath(curr)[target.first][target.second] != -1) {
                            result.push_back({i, j + 1});
                        } else {
                            map.mapInfo[i][j + 1] = 1;
                        }
                    }
                }
            } else if (now == 1) {
                if (j == N - 1) break;
                if (j > 0 && map.mapInfo[i][j - 1] == 1) continue;
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
        if (now == 0) now = 1;
        else {
            now = 0;
            j++;
        }
    }
    return result;
}

// 冒険者が次に花を発見するのを防ぐためのヒューリスティックな関数
vector<pair<ll, ll>> findOptimalBlockingPositions(Map& map, bool first) {
    if (first) {
        return findInitialBlockingPositions(map);
    }
    vector<pair<ll, ll>> result;
    bool willSeeFlowerNext = false;
    vi badDirection;
    vi watchDirection;
    int dx[] = {-1, 1, 0, 0};
    int dy[] = {0, 0, -1, 1};

    rep(i, 0, 4) {
        ll nnx = map.curr.first + dx[i], nny = map.curr.second + dy[i];
        if (nnx < 0 || nnx >= map.N || nny < 0 || nny >= map.N || map.mapInfo[nnx][nny] == 0) continue;
        
        rep(j, 0, 4) {
            ll nx = nnx, ny = nny;
            while (nx >= 0 && nx < map.N && ny >= 0 && ny < map.N) {
                if (pair<ll, ll>(nx, ny) == map.target) {
                    willSeeFlowerNext = true;
                    badDirection.push_back(i);
                    watchDirection.push_back(j);
                    break;
                }
                if (map.mapInfo[nx][ny] == 0) break;
                nx += dx[j];
                ny += dy[j];
            }
        }
    }

    if (willSeeFlowerNext) {
        rep(k, 0, badDirection.size()) {
            int i = badDirection[k];
            int j = watchDirection[k];
            ll nx = map.curr.first + dx[i], ny = map.curr.second + dy[i];
            while (nx >= 0 && nx < map.N && ny >= 0 && ny < map.N) {
                if (map.mapInfo[nx][ny] == 1 && map.searched[nx][ny] == 0 && pair<ll, ll>(nx, ny) != map.target) {
                    map.mapInfo[nx][ny] = 0;
                    if (map.calcShortestPath(map.curr)[map.target.first][map.target.second] != -1) {
                        result.push_back({nx, ny});
                        break; 
                    } else {
                        map.mapInfo[nx][ny] = 1;
                    }
                }
                if (map.mapInfo[nx][ny] == 0) break;
                nx += dx[j];
                ny += dy[j];
            }
        }
    }
    return result;
}


// ビームサーチの実装を新しい評価関数を使うように修正
vector<pair<ll, ll>> findOptimalBlockingPositionsByBeamSearch(Map& map, bool first, int beamWidth = 8, int depth = 2) {
    if (first) {
        // 初手は計算コストが高いビームサーチを避け、固定パターンで配置
        return findInitialBlockingPositions(map);
    }

    vector<BeamState> beam;
    beam.push_back(BeamState(map, {}, 0));

    for (int d = 0; d < depth; d++) {
        vector<BeamState> nextBeam;
        for (auto& state : beam) {
            vector<vector<pair<ll, ll>>> candidates = generateCandidatePlacements(state.mapState, 5);
            
            // 何も置かないという選択肢も評価
            candidates.push_back({});

            for (const auto& placement : candidates) {
                Map newMap = state.mapState;
                applyPlacements(newMap, placement);

                // 配置によって花への経路が遮断されていないか確認
                if (newMap.calcShortestPath(newMap.curr)[newMap.target.first][newMap.target.second] == -1) {
                    continue; // 遮断されているので無効な手
                }
                
                // 軽量化された評価関数で距離スコアを計算
                ll distance_score = evaluatePlacement(newMap);
                if (distance_score == -LLONG_MAX) continue;

                vector<pair<ll, ll>> newPlacements = state.placements;
                newPlacements.insert(newPlacements.end(), all(placement));
                
                // ★★★ 変更点: 木の配置数もスコアに反映 ★★★
                // 冒険者の移動距離を主目的としつつ、木の数が少ないほど高評価になるように調整。
                // 距離の価値を木のコストより十分大きくするため、係数をかける。
                ll final_score = distance_score * 100 - newPlacements.size();
                
                nextBeam.push_back(BeamState(newMap, newPlacements, final_score));
            }
        }

        if (nextBeam.empty()) break;
        
        // スコアが高い順にソート
        sort(all(nextBeam), [](const BeamState& a, const BeamState& b) {
            return a.score > b.score;
        });

        if (nextBeam.size() > beamWidth) {
            nextBeam.resize(beamWidth);
        }
        beam = nextBeam;
    }

    if (beam.empty()) {
        // 有効な手が見つからなかった場合は、単純なヒューリスティックにフォールバック
        return findOptimalBlockingPositions(map, false);
    }

    return beam[0].placements;
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
        freopen("dev/null", "w", stdout);
    }

    Input in;
    Map map(in, test);

    if (!map.test) {
        map.update();
    }

    bool first = true;
    ll turn = 0;
    while(true) {
        if (map.curr == map.target) break;

        Output out;
        vector<pair<ll, ll>> blockingPositions = findOptimalBlockingPositionsByBeamSearch(map, first, 8, 1);
        if (first) first = false;

        out.m = blockingPositions.size();
        out.points = blockingPositions;

        out.print();

        // 配置を実際のマップに反映
        applyPlacements(map, blockingPositions);

        map.update();
        turn++;
        
    }
    if (test) {
       cerr << "Seed: " << seed << " Score: " << turn << endl;
    }
}

int main() {
    for(int i = 0; i < 100; i++) {
        solve(i, true);
    }
    return 0;
}


