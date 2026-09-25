#include <bits/stdc++.h>
using namespace std;

struct Pos {
    int r, c;
};

struct Op {
    int r, c;
    int k;
    char dir;
    int len;
};

static const int MAX_OP = 100000;

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int N, K;
    cin >> N >> K;

    vector<string> C(N);
    for (auto &s : C) cin >> s;

    const int dr[4] = {-1, 1, 0, 0};
    const int dc[4] = {0, 0, -1, 1};
    const char DIR[4] = {'U', 'D', 'L', 'R'};

    auto inside = [&](int r, int c) {
        return 0 <= r && r < N && 0 <= c && c < N;
    };

    auto floorCell = [&](int r, int c) {
        return inside(r, c) && C[r][c] != '#';
    };

    // nest[color]
    vector<Pos> nest(K);

    // nestColor[r][c] = 巣の色。巣でなければ -1
    vector<vector<int>> nestColor(N, vector<int>(N, -1));

    // 初期スライム位置
    vector<vector<Pos>> initialSlimes(K);

    // tower[r][c] : 下 -> 上
    vector<vector<vector<int>>> tower(
        N, vector<vector<int>>(N)
    );

    for (int r = 0; r < N; ++r) {
        for (int c = 0; c < N; ++c) {
            char ch = C[r][c];

            if ('A' <= ch && ch < 'A' + K) {
                int color = ch - 'A';
                nest[color] = {r, c};
                nestColor[r][c] = color;
            }

            if ('a' <= ch && ch < 'a' + K) {
                int color = ch - 'a';
                tower[r][c].push_back(color);
                initialSlimes[color].push_back({r, c});
            }
        }
    }

    vector<Op> answer;
    bool stopped = false;

    // ------------------------------------------------------------
    // 帰巣処理
    // ------------------------------------------------------------
    auto home = [&](int r, int c) {
        int nc = nestColor[r][c];

        if (nc == -1) return;

        while (!tower[r][c].empty() &&
               tower[r][c].back() == nc) {
            tower[r][c].pop_back();
        }
    };

    // ------------------------------------------------------------
    // color が塔の上に何匹連続しているか
    // ------------------------------------------------------------
    auto topBlockSize = [&](int r, int c, int color) {
        int cnt = 0;

        for (int i = (int)tower[r][c].size() - 1;
             i >= 0;
             --i) {
            if (tower[r][c][i] != color) break;
            ++cnt;
        }

        return cnt;
    };

    // ------------------------------------------------------------
    // 1マス隣への操作を実行
    // 上から q 匹を移動
    // ------------------------------------------------------------
    auto moveBlock = [&](int r1, int c1,
                         int r2, int c2,
                         int q) {
        if (stopped) return false;

        if ((int)answer.size() >= MAX_OP) {
            stopped = true;
            return false;
        }

        int d = -1;

        for (int x = 0; x < 4; ++x) {
            if (r1 + dr[x] == r2 &&
                c1 + dc[x] == c2) {
                d = x;
                break;
            }
        }

        if (d == -1) {
            stopped = true;
            return false;
        }

        int h = (int)tower[r1][c1].size();
        int g = (int)tower[r2][c2].size();

        if (q <= 0 ||
            q > h ||
            g + q > 8) {
            stopped = true;
            return false;
        }

        int k = h - q;

        // len=1 なので 1 <= k+1 は必ず成立
        answer.push_back({
            r1, c1,
            k,
            DIR[d],
            1
        });

        vector<int> moving(
            tower[r1][c1].end() - q,
            tower[r1][c1].end()
        );

        tower[r1][c1].resize(h - q);

        // 宙返り
        reverse(moving.begin(), moving.end());

        for (int x : moving) {
            tower[r2][c2].push_back(x);
        }

        home(r1, c1);
        home(r2, c2);

        return true;
    };

    // ============================================================
    // 色ごとに処理
    // ============================================================

    for (int color = 0; color < K; ++color) {
        if (stopped) break;

        /*
         * この色専用の経路木を作る。
         *
         * root = 巣。
         *
         * 現在の木に最も近い未接続スライムを BFS で探し、
         * そのスライムから既存木まで最短路で接続する。
         *
         * これによって複数スライムの経路がなるべく
         * 同じ幹を共有するようになる。
         */

        vector<vector<bool>> inTree(
            N, vector<bool>(N, false)
        );

        vector<vector<Pos>> parent(
            N, vector<Pos>(N, {-1, -1})
        );

        vector<vector<int>> depth(
            N, vector<int>(N, -1)
        );

        int rr = nest[color].r;
        int rc = nest[color].c;

        inTree[rr][rc] = true;
        depth[rr][rc] = 0;

        auto allConnected = [&]() {
            for (const auto &p : initialSlimes[color]) {
                if (!inTree[p.r][p.c]) {
                    return false;
                }
            }
            return true;
        };

        while (!allConnected()) {
            vector<vector<bool>> vis(
                N, vector<bool>(N, false)
            );

            // prev[x] は BFS で1段木に近いマス
            vector<vector<Pos>> prev(
                N, vector<Pos>(N, {-1, -1})
            );

            queue<Pos> q;

            // 木全体を multi-source BFS の始点にする
            for (int r = 0; r < N; ++r) {
                for (int c = 0; c < N; ++c) {
                    if (inTree[r][c]) {
                        vis[r][c] = true;
                        q.push({r, c});
                    }
                }
            }

            Pos target{-1, -1};

            while (!q.empty()) {
                Pos cur = q.front();
                q.pop();

                // 未接続の同色スライムなら採用
                bool isTarget = false;

                for (const auto &p : initialSlimes[color]) {
                    if (p.r == cur.r &&
                        p.c == cur.c &&
                        !inTree[cur.r][cur.c]) {
                        isTarget = true;
                        break;
                    }
                }

                if (isTarget) {
                    target = cur;
                    break;
                }

                for (int d = 0; d < 4; ++d) {
                    int nr = cur.r + dr[d];
                    int nc = cur.c + dc[d];

                    if (!floorCell(nr, nc)) continue;
                    if (vis[nr][nc]) continue;

                    vis[nr][nc] = true;

                    // cur のほうが木に近い
                    prev[nr][nc] = cur;

                    q.push({nr, nc});
                }
            }

            // 床が連結なので通常起きない
            if (target.r == -1) {
                stopped = true;
                break;
            }

            // target -> 既存木までたどる
            vector<Pos> newPath;

            Pos cur = target;

            while (!inTree[cur.r][cur.c]) {
                newPath.push_back(cur);

                Pos nxt = prev[cur.r][cur.c];

                if (nxt.r == -1) {
                    stopped = true;
                    break;
                }

                parent[cur.r][cur.c] = nxt;
                cur = nxt;
            }

            if (stopped) break;

            /*
             * 木側から順に depth を付ける。
             *
             * newPath は
             * target -> ... -> 木の直前
             * なので reverse する。
             */
            reverse(newPath.begin(), newPath.end());

            for (const Pos &p : newPath) {
                Pos par = parent[p.r][p.c];

                depth[p.r][p.c] =
                    depth[par.r][par.c] + 1;

                inTree[p.r][p.c] = true;
            }
        }

        if (stopped) break;

        // --------------------------------------------------------
        // 木のノードを深い順に処理する。
        // --------------------------------------------------------

        vector<Pos> nodes;

        for (int r = 0; r < N; ++r) {
            for (int c = 0; c < N; ++c) {
                if (inTree[r][c] &&
                    !(r == rr && c == rc)) {
                    nodes.push_back({r, c});
                }
            }
        }

        sort(nodes.begin(), nodes.end(),
             [&](const Pos &a, const Pos &b) {
                 return depth[a.r][a.c] >
                        depth[b.r][b.c];
             });

        /*
         * drain(r,c):
         *
         * この地点の上側にある color の一団を
         * 親方向へすべて流す。
         *
         * 親の塔が満杯に近い場合、
         * 親にある同色ブロックを先にさらに上流へ流す。
         *
         * これによって
         *
         * child1 ---\
         *            +---- parent ---- root
         * child2 ---/
         *
         * の parent で同色スライムが合流する。
         */

        function<void(int,int)> drain =
            [&](int r, int c) {

            if (stopped) return;

            // root から先には流さない
            if (r == rr && c == rc) {
                return;
            }

            Pos par = parent[r][c];

            if (par.r == -1) {
                stopped = true;
                return;
            }

            while (!stopped) {
                int srcBlock =
                    topBlockSize(r, c, color);

                if (srcBlock == 0) {
                    break;
                }

                int pr = par.r;
                int pc = par.c;

                int dstBlock =
                    topBlockSize(pr, pc, color);

                int dstHeight =
                    (int)tower[pr][pc].size();

                int capacity = 8 - dstHeight;

                /*
                 * srcBlock 全部が入り切らず、
                 * 親にも同色ブロックがすでにあるなら、
                 * 先に親を上流へ流してスペースを作る。
                 *
                 * 例:
                 *
                 *     source: 5匹
                 *     parent: 5匹
                 *
                 * のようなときに無理に分割せず、
                 * parent 側を先に進ませる。
                 */
                if (capacity < srcBlock &&
                    dstBlock > 0) {

                    if (pr == rr && pc == rc) {
                        /*
                         * 自色の巣なら本来 dstBlock は
                         * 帰巣済みなのでここには来ない。
                         */
                    } else {
                        drain(pr, pc);

                        if (stopped) return;
                        continue;
                    }
                }

                dstHeight =
                    (int)tower[pr][pc].size();

                capacity = 8 - dstHeight;

                if (capacity <= 0) {
                    /*
                     * 現在の構成では、他色だけで高さ8になる
                     * 状況は基本的に発生しない想定。
                     *
                     * 安全のため停止する。
                     */
                    stopped = true;
                    return;
                }

                srcBlock =
                    topBlockSize(r, c, color);

                int moveCount =
                    min(srcBlock, capacity);

                if (!moveBlock(
                        r, c,
                        pr, pc,
                        moveCount)) {
                    return;
                }
            }
        };

        for (const Pos &p : nodes) {
            if (stopped) break;

            if (topBlockSize(p.r, p.c, color) > 0) {
                drain(p.r, p.c);
            }
        }
    }

    // ------------------------------------------------------------
    // 出力
    // ------------------------------------------------------------

    for (const auto &op : answer) {
        cout << op.r << ' '
             << op.c << ' '
             << op.k << ' '
             << op.dir << ' '
             << op.len << '\n';
    }

    return 0;
}