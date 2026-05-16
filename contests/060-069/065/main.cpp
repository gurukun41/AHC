#include <bits/stdc++.h>
using namespace std;

static constexpr int N = 20;
static constexpr int V = N * N;
static constexpr int INF = 1e9;

int id(int i, int j) { return i * N + j; }
int rr(int x) { return x / N; }
int cc(int x) { return x % N; }

struct XorShift {
    uint64_t x;

    XorShift(uint64_t seed = 88172645463325252ull) : x(seed) {}

    uint64_t next64() {
        x ^= x << 7;
        x ^= x >> 9;
        return x;
    }

    int randint(int n) {
        return (int)(next64() % (uint64_t)n);
    }

    double uniform01() {
        return (next64() >> 11) * (1.0 / 9007199254740992.0);
    }
};

struct Move {
    int to;
    int m;
    int d;
};

struct Step {
    int m;
    int d;
    int to;
};

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int inputN;
    cin >> inputN;

    array<int, V> cell_box{};
    array<int, V> box_cell{};

    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            int a;
            cin >> a;

            int c = id(i, j);
            cell_box[c] = a;
            box_cell[a] = c;
        }
    }

    vector<array<int, 4>> loops;
    loops.reserve(181);

    // Layer 0:
    // 偶数偶数始点の 2x2 タイル。
    // 全マスをちょうど1回覆う。
    for (int i = 0; i < N; i += 2) {
        for (int j = 0; j < N; j += 2) {
            loops.push_back({
                id(i, j),
                id(i, j + 1),
                id(i + 1, j + 1),
                id(i + 1, j)
            });
        }
    }

    // Layer 1:
    // 1マスずらした 2x2 タイル。
    // 内側のマスだけ2個目のループに属する。
    for (int i = 1; i + 1 < N; i += 2) {
        for (int j = 1; j + 1 < N; j += 2) {
            loops.push_back({
                id(i, j),
                id(i, j + 1),
                id(i + 1, j + 1),
                id(i + 1, j)
            });
        }
    }

    const int M = (int)loops.size();
    const int exit_cell = id(0, N / 2);

    vector<vector<Move>> moves(V);

    for (int m = 0; m < M; m++) {
        for (int k = 0; k < 4; k++) {
            int from = loops[m][k];

            moves[from].push_back({
                loops[m][(k + 1) & 3],
                m,
                1
            });

            moves[from].push_back({
                loops[m][(k + 3) & 3],
                m,
                -1
            });
        }
    }

    // dist[c] = c にいる箱を出口まで運ぶ最短 2x2 回転数。
    array<int, V> dist{};
    dist.fill(INF);
    dist[exit_cell] = 0;

    queue<int> q;
    q.push(exit_cell);

    while (!q.empty()) {
        int v = q.front();
        q.pop();

        for (const auto& mv : moves[v]) {
            if (dist[mv.to] > dist[v] + 1) {
                dist[mv.to] = dist[v] + 1;
                q.push(mv.to);
            }
        }
    }

    XorShift rng(
        (uint64_t)chrono::high_resolution_clock::now()
            .time_since_epoch()
            .count()
    );

    auto start_time = chrono::high_resolution_clock::now();
    const double TIME_LIMIT = 1.80;

    auto elapsed = [&]() -> double {
        return chrono::duration<double>(
            chrono::high_resolution_clock::now() - start_time
        ).count();
    };

    auto apply_local = [&](array<int, V>& cb, array<int, V>& bc, int m, int d) {
        array<int, 4> old{};

        for (int k = 0; k < 4; k++) {
            old[k] = cb[loops[m][k]];
        }

        if (d == 1) {
            for (int k = 0; k < 4; k++) {
                cb[loops[m][(k + 1) & 3]] = old[k];
            }
        } else {
            for (int k = 0; k < 4; k++) {
                cb[loops[m][(k + 3) & 3]] = old[k];
            }
        }

        for (int k = 0; k < 4; k++) {
            int c = loops[m][k];
            int b = cb[c];

            if (b >= 0) {
                bc[b] = c;
            }
        }
    };

    auto shortest_path = [&](int start) {
        vector<Step> path;
        int cur = start;

        while (cur != exit_cell) {
            Move chosen = moves[cur][0];

            for (const auto& mv : moves[cur]) {
                if (dist[mv.to] < dist[chosen.to]) {
                    chosen = mv;
                }
            }

            path.push_back({chosen.m, chosen.d, chosen.to});
            cur = chosen.to;
        }

        return path;
    };

    auto random_path = [&](int start, double temp, int slack) {
        vector<Step> path;
        int cur = start;

        if (cur == exit_cell) {
            return path;
        }

        int max_len = dist[start] + slack;

        array<unsigned char, V> seen{};
        seen.fill(0);
        seen[cur] = 1;

        while (cur != exit_cell && (int)path.size() < max_len) {
            int best_next_dist = INF;

            for (const auto& mv : moves[cur]) {
                best_next_dist = min(best_next_dist, dist[mv.to]);
            }

            struct Cand {
                Move mv;
                double w;
            };

            vector<Cand> cand;
            double sum_w = 0.0;

            for (const auto& mv : moves[cur]) {
                int nd = dist[mv.to];

                if (nd >= INF) continue;
                if ((int)path.size() + 1 + nd > max_len) continue;
                if (seen[mv.to] >= 2) continue;

                int penalty = nd - best_next_dist;

                if (seen[mv.to]) penalty += 2;
                if (nd > dist[cur]) penalty += 1;

                double w = exp(-(double)penalty / max(0.05, temp));

                cand.push_back({mv, w});
                sum_w += w;
            }

            Move chosen;

            if (cand.empty()) {
                chosen = moves[cur][0];

                for (const auto& mv : moves[cur]) {
                    if (dist[mv.to] < dist[chosen.to]) {
                        chosen = mv;
                    }
                }
            } else {
                double x = rng.uniform01() * sum_w;
                chosen = cand.back().mv;

                for (const auto& ca : cand) {
                    x -= ca.w;

                    if (x <= 0) {
                        chosen = ca.mv;
                        break;
                    }
                }
            }

            path.push_back({chosen.m, chosen.d, chosen.to});
            cur = chosen.to;

            if (seen[cur] < 255) {
                seen[cur]++;
            }
        }

        // 失敗時は最短路に戻す。
        if (cur != exit_cell) {
            path.clear();
            cur = start;

            while (cur != exit_cell) {
                Move chosen = moves[cur][0];

                for (const auto& mv : moves[cur]) {
                    if (dist[mv.to] < dist[chosen.to]) {
                        chosen = mv;
                    }
                }

                path.push_back({chosen.m, chosen.d, chosen.to});
                cur = chosen.to;
            }
        }

        return path;
    };

    auto eval_path = [&](const vector<Step>& path, int target) -> long long {
        array<int, V> cb = cell_box;
        array<int, V> bc = box_cell;

        for (const auto& st : path) {
            apply_local(cb, bc, st.m, st.d);
        }

        if (cb[exit_cell] != target) {
            return (long long)4e18;
        }

        cb[exit_cell] = -1;
        bc[target] = -1;

        long long score = (long long)path.size() * 350;

        // 次の箱たちが出口に近づく副作用を評価する。
        const int W = 28;

        for (int b = target + 1; b < V && b <= target + W; b++) {
            int p = bc[b];

            if (p < 0) continue;

            int w = W - (b - target) + 1;
            score += (long long)w * dist[p] * 18;
        }

        // かなり未来の箱が出口を塞ぐのは少し嫌う。
        int b_at_exit = cb[exit_cell];

        if (b_at_exit >= target + 5) {
            score += 800;
        }

        return score;
    };

    vector<pair<int, int>> ops;
    ops.reserve(20000);

    int next_box = 0;

    // 初期状態で箱0が出口にある場合だけ、操作前に消える。
    if (cell_box[exit_cell] == 0) {
        cell_box[exit_cell] = -1;
        box_cell[0] = -1;
        next_box = 1;
    }

    auto apply_actual = [&](int m, int d) {
        ops.push_back({m, d});
        apply_local(cell_box, box_cell, m, d);

        if (next_box < V && cell_box[exit_cell] == next_box) {
            cell_box[exit_cell] = -1;
            box_cell[next_box] = -1;
            next_box++;
        }
    };

    auto select_path_by_sa = [&](int target) {
        int start = box_cell[target];

        if (start == exit_cell) {
            return vector<Step>{};
        }

        vector<Step> cur = shortest_path(start);
        long long cur_score = eval_path(cur, target);

        vector<Step> best = cur;
        long long best_score = cur_score;

        int iter_limit = 70;

        if (target > 250) iter_limit = 45;
        if (target > 340) iter_limit = 25;
        if (elapsed() > TIME_LIMIT * 0.75) iter_limit = 12;
        if (elapsed() > TIME_LIMIT * 0.90) iter_limit = 3;

        for (int it = 0; it < iter_limit && elapsed() < TIME_LIMIT; it++) {
            double p = (iter_limit <= 1)
                ? 1.0
                : (double)it / (iter_limit - 1);

            double temp_path = 2.5 * pow(0.08 / 2.5, p);
            double temp_accept = 900.0 * pow(5.0 / 900.0, p);

            int slack = 2 + rng.randint(9);

            if (p > 0.75) {
                slack = rng.randint(4);
            }

            vector<Step> nxt = random_path(start, temp_path, slack);
            long long nxt_score = eval_path(nxt, target);

            bool accept = false;

            if (nxt_score <= cur_score) {
                accept = true;
            } else {
                double prob = exp(
                    -(double)(nxt_score - cur_score)
                    / max(1.0, temp_accept)
                );

                if (rng.uniform01() < prob) {
                    accept = true;
                }
            }

            if (accept) {
                cur.swap(nxt);
                cur_score = nxt_score;

                if (cur_score < best_score) {
                    best = cur;
                    best_score = cur_score;
                }
            }
        }

        return best;
    };

    while (next_box < V && (int)ops.size() < 100000) {
        int target = next_box;

        vector<Step> path = select_path_by_sa(target);

        if ((int)ops.size() + (int)path.size() > 100000) {
            path = shortest_path(box_cell[target]);
        }

        for (const auto& st : path) {
            if ((int)ops.size() >= 100000) break;

            apply_actual(st.m, st.d);

            if (box_cell[target] == -1) {
                break;
            }
        }

        // 念のための保険。
        // 基本的には入らない。
        while (box_cell[target] != -1 && (int)ops.size() < 100000) {
            vector<Step> fallback = shortest_path(box_cell[target]);

            for (const auto& st : fallback) {
                if ((int)ops.size() >= 100000) break;

                apply_actual(st.m, st.d);

                if (box_cell[target] == -1) {
                    break;
                }
            }
        }
    }

    cout << M << '\n';

    for (const auto& lp : loops) {
        cout << 4;

        for (int c : lp) {
            cout << ' ' << rr(c) << ' ' << cc(c);
        }

        cout << '\n';
    }

    cout << ops.size() << '\n';

    for (auto [m, d] : ops) {
        cout << m << ' ' << d << '\n';
    }

    return 0;
}