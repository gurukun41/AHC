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

// Homogeneous blocks are unchanged by reversal. Replace only disjoint,
// consecutive pairs whose intermediate cell starts and ends empty.
vector<Op> compressMoves(const vector<string>& grid, int K,
                         const vector<Op>& ops) {
    const int n = (int)grid.size();
    vector<vector<vector<int>>> board(n, vector<vector<int>>(n));
    vector<vector<int>> nests(n, vector<int>(n, -1));
    for (int r = 0; r < n; ++r) {
        for (int c = 0; c < n; ++c) {
            char ch = grid[r][c];
            if ('a' <= ch && ch < 'a' + K) {
                board[r][c].push_back(ch - 'a');
            } else if ('A' <= ch && ch < 'A' + K) {
                nests[r][c] = ch - 'A';
            }
        }
    }

    auto floorCell = [&](int r, int c) {
        return 0 <= r && r < n && 0 <= c && c < n &&
               grid[r][c] != '#';
    };
    auto direction = [](char dir) -> pair<int, int> {
        if (dir == 'U') return {-1, 0};
        if (dir == 'D') return {1, 0};
        if (dir == 'L') return {0, -1};
        if (dir == 'R') return {0, 1};
        return {0, 0};
    };
    auto home = [&](int r, int c) {
        int color = nests[r][c];
        if (color < 0) return;
        while (!board[r][c].empty() && board[r][c].back() == color) {
            board[r][c].pop_back();
        }
    };

    // Replay the original operations, including reversal and homing at both
    // endpoints. Any unexpected invalid operation disables postprocessing.
    auto replay = [&](const Op& op) {
        if (!floorCell(op.r, op.c)) return false;
        auto [dr, dc] = direction(op.dir);
        int h = (int)board[op.r][op.c].size();
        if ((dr == 0 && dc == 0) || op.k < 0 || op.k >= h ||
            op.len < 1 || op.len > op.k + 1) {
            return false;
        }
        for (int step = 1; step <= op.len; ++step) {
            if (!floorCell(op.r + dr * step, op.c + dc * step)) {
                return false;
            }
        }
        int tr = op.r + dr * op.len;
        int tc = op.c + dc * op.len;
        if ((int)board[tr][tc].size() + h - op.k > 8) return false;

        vector<int> moving(board[op.r][op.c].begin() + op.k,
                           board[op.r][op.c].end());
        board[op.r][op.c].resize(op.k);
        reverse(moving.begin(), moving.end());
        board[tr][tc].insert(board[tr][tc].end(), moving.begin(), moving.end());
        home(op.r, op.c);
        home(tr, tc);
        return true;
    };

    vector<Op> result;
    result.reserve(ops.size());
    for (size_t i = 0; i < ops.size();) {
        const Op& first = ops[i];
        bool canMerge = false;
        if (i + 1 < ops.size() && first.len == 1 && first.k >= 1 &&
            floorCell(first.r, first.c)) {
            const Op& second = ops[i + 1];
            auto [dr, dc] = direction(first.dir);
            int br = first.r + dr;
            int bc = first.c + dc;
            int cr = br + dr;
            int cc = bc + dc;
            int h = (int)board[first.r][first.c].size();
            if ((dr != 0 || dc != 0) && first.k < h &&
                second.len == 1 && second.k == 0 &&
                second.dir == first.dir && second.r == br && second.c == bc &&
                floorCell(br, bc) && floorCell(cr, cc) &&
                board[br][bc].empty() &&
                (int)board[cr][cc].size() + h - first.k <= 8) {
                int color = board[first.r][first.c].back();
                // Homing at B would change the payload. C may be its own nest.
                canMerge = nests[br][bc] != color;
                for (int j = first.k; j < h; ++j) {
                    if (board[first.r][first.c][j] != color) canMerge = false;
                }
            }
        }

        if (!replay(first)) return ops;
        if (canMerge) {
            if (!replay(ops[i + 1])) return ops;
            Op combined = first;
            combined.len = 2;
            result.push_back(combined);
            i += 2;
        } else {
            result.push_back(first);
            ++i;
        }
    }
    return result;
}

// Interleave all colors so that a temporarily parked block can serve as a
// springboard for a different color. Every moved slime stays on its own tree
// and strictly approaches its root; an unfinished candidate is never returned.
optional<vector<Op>> cooperativeSolve(
    const vector<string>& grid, int K,
    const vector<vector<int>>& parent,
    const vector<vector<int>>& depth, int operationLimit, int mode,
    chrono::steady_clock::time_point deadline) {
    const int n = (int)grid.size();
    const int cells = n * n;
    if (operationLimit <= 0) return nullopt;

    struct Stack {
        array<int, 8> color{};
        int size = 0;
    };
    vector<Stack> board(cells);
    vector<int> nest(cells, -1);
    int remaining = 0;
    for (int v = 0; v < cells; ++v) {
        char ch = grid[v / n][v % n];
        if ('a' <= ch && ch < 'a' + K) {
            board[v].color[board[v].size++] = ch - 'a';
            ++remaining;
        } else if ('A' <= ch && ch < 'A' + K) {
            nest[v] = ch - 'A';
        }
    }

    // Ancestors reachable in one straight jump, and a bottom-up tree order.
    vector<vector<array<int, 8>>> jump(K, vector<array<int, 8>>(cells));
    vector<vector<int>> run(K, vector<int>(cells));
    vector<vector<int>> order(K);
    for (int color = 0; color < K; ++color) {
        for (int v = 0; v < cells; ++v) {
            jump[color][v].fill(-1);
            if (depth[color][v] < 0) continue;
            order[color].push_back(v);
            int p = parent[color][v];
            if (p < 0) continue;
            int dr = p / n - v / n;
            int dc = p % n - v % n;
            int cur = v;
            for (int len = 1; len <= 8; ++len) {
                int next = parent[color][cur];
                if (next < 0 || next / n - cur / n != dr ||
                    next % n - cur % n != dc) break;
                jump[color][v][len - 1] = next;
                run[color][v] = len;
                cur = next;
            }
        }
        sort(order[color].begin(), order[color].end(),
             [&](int a, int b) { return depth[color][a] > depth[color][b]; });
    }

    // Variants are fixed before execution: they trade readiness against
    // preserving useful foreign-color springboards.
    static constexpr double supportWeights[8] =
        {0.45, 0.85, 1.35, 1.85, 0.65, 1.10, 1.60, 2.20};
    static constexpr double readinessWeights[8] =
        {7.0, 7.0, 7.0, 7.0, 10.0, 10.0, 10.0, 10.0};
    const int variant = ((mode % 8) + 8) % 8;
    const double supportWeight = supportWeights[variant];
    const double readinessWeight = readinessWeights[variant];

    vector<int> topColor(cells), topCount(cells);
    vector<vector<int>> local(K, vector<int>(cells));
    vector<vector<int>> pending(K, vector<int>(cells));
    vector<vector<int>> nearest(K, vector<int>(cells));
    vector<vector<int>> arrivalDistance(K, vector<int>(cells));
    vector<vector<int>> arrivalSize(K, vector<int>(cells));
    vector<vector<int>> nearestSize(K, vector<int>(cells));
    vector<vector<double>> traffic(K, vector<double>(cells));
    vector<array<double, 9>> support(cells);
    vector<Op> answer;
    answer.reserve(operationLimit);

    auto home = [&](int v) {
        if (nest[v] < 0) return;
        while (board[v].size > 0 &&
               board[v].color[board[v].size - 1] == nest[v]) {
            --board[v].size;
            --remaining;
        }
    };

    while (remaining > 0) {
        if ((int)answer.size() >= operationLimit ||
            chrono::steady_clock::now() >= deadline) return nullopt;

        fill(topColor.begin(), topColor.end(), -1);
        fill(topCount.begin(), topCount.end(), 0);
        for (int color = 0; color < K; ++color) {
            fill(local[color].begin(), local[color].end(), 0);
            fill(nearest[color].begin(), nearest[color].end(), cells + 1);
            fill(nearestSize[color].begin(), nearestSize[color].end(), 1);
            fill(arrivalDistance[color].begin(), arrivalDistance[color].end(), cells + 1);
            fill(arrivalSize[color].begin(), arrivalSize[color].end(), 1);
            fill(traffic[color].begin(), traffic[color].end(), 0.0);
        }
        for (int v = 0; v < cells; ++v) {
            const Stack& stack = board[v];
            for (int j = 0; j < stack.size; ++j) ++local[stack.color[j]][v];
            if (stack.size == 0) continue;
            int color = stack.color[stack.size - 1];
            topColor[v] = color;
            int count = 0;
            while (count < stack.size && stack.color[stack.size - 1 - count] == color)
                ++count;
            topCount[v] = count;
            nearest[color][v] = 0;
            nearestSize[color][v] = count;
        }

        // Pending mass includes hidden blocks. Arrival distance uses exposed
        // blocks only, so a blocked distant color does not reserve a tower.
        for (int color = 0; color < K; ++color) {
            pending[color] = local[color];
            for (int v : order[color]) {
                int p = parent[color][v];
                if (p < 0) continue;
                pending[color][p] += pending[color][v];
                int distance = nearest[color][v] + 1;
                if (distance < arrivalDistance[color][p]) {
                    arrivalDistance[color][p] = distance;
                    arrivalSize[color][p] = nearestSize[color][v];
                }
                if (distance < nearest[color][p]) {
                    nearest[color][p] = distance;
                    nearestSize[color][p] = nearestSize[color][v];
                }
            }
            for (int v : order[color]) {
                int incoming = pending[color][v] - local[color][v];
                int distance = arrivalDistance[color][v];
                if (incoming == 0 || run[color][v] < 2 || distance > cells) continue;
                double batches = min(2.0, (double)incoming / max(1, arrivalSize[color][v]));
                traffic[color][v] = batches / (1.0 + 0.5 * distance);
            }
        }

        auto colorSupport = [&](int color, int v, int height) {
            if (height <= 0 || height >= 8 || traffic[color][v] == 0.0) return 0.0;
            int saved = min(height, run[color][v] - 1);
            double fit = min(1.0, (double)(8 - height) / arrivalSize[color][v]);
            return traffic[color][v] * saved * fit;
        };
        for (int v = 0; v < cells; ++v) {
            support[v].fill(0.0);
            for (int color = 0; color < K; ++color) {
                if (traffic[color][v] == 0.0) continue;
                for (int height = 1; height < 8; ++height)
                    support[v][height] += colorSupport(color, v, height);
            }
        }
        auto foreignSupport = [&](int color, int v, int height) {
            return support[v][height] - colorSupport(color, v, height);
        };

        struct Candidate {
            int source = -1, target = -1, count = 0, length = 0;
            double score = -1e100;
        } best;
        for (int source = 0; source < cells; ++source) {
            int color = topColor[source];
            if (color < 0 || run[color][source] == 0) continue;
            int height = board[source].size;
            int block = topCount[source];
            int bottom = height - block;
            int subtree = pending[color][source];
            int desired = min(8 - bottom, subtree);
            double originalSupport = foreignSupport(color, source, height);

            for (int count = 1; count <= block; ++count) {
                int kept = height - count;
                int maxLength = min(run[color][source], kept + 1);
                int skippedSameColor = 0;
                for (int len = 1; len <= maxLength; ++len) {
                    int target = jump[color][source][len - 1];
                    int destinationHeight = board[target].size;
                    if (len > 1) {
                        int skipped = jump[color][source][len - 2];
                        skippedSameColor += local[color][skipped];
                    }
                    if (destinationHeight + count > 8) continue;

                    int sameAtTarget = topColor[target] == color ? topCount[target] : 0;
                    double ready = min(1.0, (double)count / max(1, desired));
                    // Once a block is full, waiting for more descendants cannot
                    // save an additional batch on this departure.
                    double score = 1.0 + 0.55 * count + 1.65 * (len - 1);
                    score -= readinessWeight * (1.0 - ready);
                    if (sameAtTarget > 0)
                        score += 1.25 * min(count, sameAtTarget);
                    score -= 0.8 * min(count, skippedSameColor);
                    if (count < block) score -= 0.85;
                    if (nest[target] == color) score += 0.20 * count;
                    if (count == block && bottom > 0) score += 0.15 * bottom;

                    // Evaluate support after automatic homing at both ends.
                    int sourceAfter = kept;
                    while (sourceAfter > 0 && nest[source] >= 0 &&
                           board[source].color[sourceAfter - 1] == nest[source])
                        --sourceAfter;
                    int targetAfter = destinationHeight + count;
                    if (nest[target] == color) {
                        targetAfter = destinationHeight;
                        while (targetAfter > 0 && board[target].color[targetAfter - 1] == color)
                            --targetAfter;
                    }
                    double supportChange = foreignSupport(color, source, sourceAfter) - originalSupport;
                    supportChange += foreignSupport(color, target, targetAfter) -
                                     foreignSupport(color, target, destinationHeight);
                    score += supportWeight * supportChange;

                    // A stable, mode-dependent tie break explores different
                    // interleavings without random generators or runtime tuning.
                    unsigned tie = (unsigned)(source + 1) * 2654435761u;
                    tie ^= (unsigned)(color + 1 + variant * 17) * 2246822519u;
                    score += 0.00001 * (tie % 1024);
                    if (score > best.score) best = {source, target, count, len, score};
                }
            }
        }
        if (best.source < 0) return nullopt;

        Stack& from = board[best.source];
        Stack& to = board[best.target];
        int kept = from.size - best.count;
        char direction;
        if (best.target / n < best.source / n) direction = 'U';
        else if (best.target / n > best.source / n) direction = 'D';
        else if (best.target % n < best.source % n) direction = 'L';
        else direction = 'R';
        answer.push_back({best.source / n, best.source % n, kept, direction, best.length});
        // The payload is monochrome, but reverse it explicitly to mirror the
        // problem rule and keep board simulation locally complete.
        for (int j = from.size - 1; j >= kept; --j)
            to.color[to.size++] = from.color[j];
        from.size = kept;
        home(best.source);
        home(best.target);
    }
    if ((int)answer.size() >= operationLimit ||
        chrono::steady_clock::now() >= deadline) return nullopt;
    return answer;
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    const auto searchStart = chrono::steady_clock::now();

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

    // 色をまたぐ運搬でも、同色を合流させる既存の木を共有する。
    vector<vector<int>> parentTrees(K, vector<int>(N * N, -1));
    vector<vector<int>> treeDepths(K, vector<int>(N * N, -1));
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

        for (int r = 0; r < N; ++r) {
            for (int c = 0; c < N; ++c) {
                treeDepths[color][r * N + c] = depth[r][c];
                const Pos par = parent[r][c];
                if (par.r >= 0) parentTrees[color][r * N + c] = par.r * N + par.c;
            }
        }

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

        // 深い順は維持し、同じ親を持つ同深さの枝だけ順序を変える。
        // 親の空きに収まる束を優先し、その中では大きい束を選ぶ。
        // drain によって親の高さが変わるので、毎回選び直す。
        vector<bool> processed(nodes.size(), false);
        for (int begin = 0; begin < (int)nodes.size() && !stopped;) {
            int end = begin + 1;
            while (end < (int)nodes.size() &&
                   depth[nodes[end].r][nodes[end].c] ==
                       depth[nodes[begin].r][nodes[begin].c]) {
                ++end;
            }

            int first = begin;
            while (!stopped) {
                while (first < end && processed[first]) ++first;
                if (first == end) break;

                const Pos anchor = nodes[first];
                if (topBlockSize(anchor.r, anchor.c, color) == 0) {
                    processed[first] = true;
                    continue;
                }

                const Pos par = parent[anchor.r][anchor.c];
                const int capacity = 8 - (int)tower[par.r][par.c].size();
                int selected = first;
                int bestFit = 0;
                for (int i = first; i < end; ++i) {
                    if (processed[i]) continue;
                    const Pos p = nodes[i];
                    const Pos candidateParent = parent[p.r][p.c];
                    if (candidateParent.r != par.r || candidateParent.c != par.c) {
                        continue;
                    }
                    const int block = topBlockSize(p.r, p.c, color);
                    if (block <= capacity && block > bestFit) {
                        bestFit = block;
                        selected = i;
                    }
                }

                const Pos p = nodes[selected];
                processed[selected] = true;
                drain(p.r, p.c);
            }
            begin = end;
        }
    }

    // 生成した操作列の盤面遷移を保ったまま、連続する2移動を圧縮する。
    answer = compressMoves(C, K, answer);

    // 集約待ちと足場保持の重みを変え、全色を交互に進める候補を作る。
    // 完走して操作数が減った候補だけ採用し、途中停止した候補は捨てる。
    const auto searchDeadline = searchStart + chrono::milliseconds(1200);
    int cooperativeMode = -1;
    if (!stopped) {
        for (int mode = 0; mode < 8; ++mode) {
            if (chrono::steady_clock::now() >= searchDeadline) break;
            const int limit = min(MAX_OP, 2 * (int)answer.size() + 1);
            auto candidate = cooperativeSolve(C, K, parentTrees, treeDepths,
                                               limit, mode, searchDeadline);
            if (!candidate) continue;
            auto compressed = compressMoves(C, K, *candidate);
            if (compressed.size() < answer.size()) {
                answer = std::move(compressed);
                cooperativeMode = mode;
            }
        }
    }
    cerr << "cooperative_mode=" << cooperativeMode << '\n';

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
