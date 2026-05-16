#include <bits/stdc++.h>
using namespace std;

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

static const int N = 20;
static const int L = N * N;
static const int B = 10;
static const int K = B * B;

int cid(int i, int j) {
    return i * N + j;
}

int rid(int x) {
    return x / N;
}

int cjd(int x) {
    return x % N;
}

int bid(int r, int c) {
    return r * B + c;
}

vector<pair<int, int>> macro_edges;
int edge_id[K][K];

void add_grid_edge(array<unsigned char, L>& mask, int u, int v) {
    int du = rid(v) - rid(u);
    int dv = cjd(v) - cjd(u);

    int bu = -1, bv = -1;

    if (du == -1) {
        bu = 0;
        bv = 1;
    } else if (du == 1) {
        bu = 1;
        bv = 0;
    } else if (dv == -1) {
        bu = 2;
        bv = 3;
    } else if (dv == 1) {
        bu = 3;
        bv = 2;
    }

    mask[u] |= (1u << bu);
    mask[v] |= (1u << bv);
}

void remove_grid_edge(array<unsigned char, L>& mask, int u, int v) {
    int du = rid(v) - rid(u);
    int dv = cjd(v) - cjd(u);

    int bu = -1, bv = -1;

    if (du == -1) {
        bu = 0;
        bv = 1;
    } else if (du == 1) {
        bu = 1;
        bv = 0;
    } else if (dv == -1) {
        bu = 2;
        bv = 3;
    } else if (dv == 1) {
        bu = 3;
        bv = 2;
    }

    mask[u] &= ~(1u << bu);
    mask[v] &= ~(1u << bv);
}

array<int, 4> block4(int r, int c) {
    int tl = cid(2 * r, 2 * c);
    int tr = cid(2 * r, 2 * c + 1);
    int br = cid(2 * r + 1, 2 * c + 1);
    int bl = cid(2 * r + 1, 2 * c);
    return {tl, tr, br, bl};
}

vector<int> build_cycle_from_tree(const vector<unsigned char>& in_tree) {
    array<unsigned char, L> mask{};
    mask.fill(0);

    for (int r = 0; r < B; r++) {
        for (int c = 0; c < B; c++) {
            auto q = block4(r, c);
            int tl = q[0];
            int tr = q[1];
            int br = q[2];
            int bl = q[3];

            add_grid_edge(mask, tl, tr);
            add_grid_edge(mask, tr, br);
            add_grid_edge(mask, br, bl);
            add_grid_edge(mask, bl, tl);
        }
    }

    for (int e = 0; e < (int)macro_edges.size(); e++) {
        if (!in_tree[e]) continue;

        auto [u, v] = macro_edges[e];

        int r1 = u / B;
        int c1 = u % B;
        int r2 = v / B;
        int c2 = v % B;

        if (r1 == r2) {
            if (c1 > c2) {
                swap(r1, r2);
                swap(c1, c2);
            }

            auto A = block4(r1, c1);
            auto C = block4(r2, c2);

            int tr1 = A[1];
            int br1 = A[2];
            int tl2 = C[0];
            int bl2 = C[3];

            remove_grid_edge(mask, tr1, br1);
            remove_grid_edge(mask, tl2, bl2);

            add_grid_edge(mask, tr1, tl2);
            add_grid_edge(mask, br1, bl2);
        } else {
            if (r1 > r2) {
                swap(r1, r2);
                swap(c1, c2);
            }

            auto A = block4(r1, c1);
            auto C = block4(r2, c2);

            int br1 = A[2];
            int bl1 = A[3];
            int tl2 = C[0];
            int tr2 = C[1];

            remove_grid_edge(mask, bl1, br1);
            remove_grid_edge(mask, tl2, tr2);

            add_grid_edge(mask, bl1, tl2);
            add_grid_edge(mask, br1, tr2);
        }
    }

    vector<int> cyc;
    cyc.reserve(L);

    int cur = 0;
    int prev = -1;

    for (int step = 0; step < L; step++) {
        cyc.push_back(cur);

        int neigh[2];
        int cnt = 0;

        int i = rid(cur);
        int j = cjd(cur);

        if ((mask[cur] & 1) && i > 0) neigh[cnt++] = cid(i - 1, j);
        if ((mask[cur] & 2) && i + 1 < N) neigh[cnt++] = cid(i + 1, j);
        if ((mask[cur] & 4) && j > 0) neigh[cnt++] = cid(i, j - 1);
        if ((mask[cur] & 8) && j + 1 < N) neigh[cnt++] = cid(i, j + 1);

        int nxt = (neigh[0] == prev ? neigh[1] : neigh[0]);

        prev = cur;
        cur = nxt;
    }

    return cyc;
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int n;
    cin >> n;

    vector<int> box_at_cell(L), cell_of_box(L);

    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            int x;
            cin >> x;

            int c = cid(i, j);
            box_at_cell[c] = x;
            cell_of_box[x] = c;
        }
    }

    memset(edge_id, -1, sizeof(edge_id));

    for (int r = 0; r < B; r++) {
        for (int c = 0; c < B; c++) {
            int u = bid(r, c);

            if (c + 1 < B) {
                int v = bid(r, c + 1);
                edge_id[u][v] = edge_id[v][u] = (int)macro_edges.size();
                macro_edges.push_back({u, v});
            }

            if (r + 1 < B) {
                int v = bid(r + 1, c);
                edge_id[u][v] = edge_id[v][u] = (int)macro_edges.size();
                macro_edges.push_back({u, v});
            }
        }
    }

    const int E = (int)macro_edges.size();

    vector<unsigned char> in_tree(E, 0);

    vector<int> order;
    order.reserve(K);

    for (int r = 0; r < B; r++) {
        if (r % 2 == 0) {
            for (int c = 0; c < B; c++) order.push_back(bid(r, c));
        } else {
            for (int c = B - 1; c >= 0; c--) order.push_back(bid(r, c));
        }
    }

    for (int i = 0; i + 1 < (int)order.size(); i++) {
        int e = edge_id[order[i]][order[i + 1]];
        in_tree[e] = 1;
    }

    const int exit_cell = cid(0, N / 2);
    const int first_box = (box_at_cell[exit_cell] == 0 ? 1 : 0);

    auto calc_score = [&](const vector<int>& cyc) -> int {
        int pos[L];

        for (int i = 0; i < L; i++) {
            pos[cyc[i]] = i;
        }

        int exit_pos = pos[exit_cell];

        int cur = 0;
        int total = 0;

        for (int b = first_box; b < L; b++) {
            int need = (exit_pos - pos[cell_of_box[b]] + L) % L;
            int delta = (need - cur + L) % L;

            total += min(delta, L - delta);
            cur = need;
        }

        return total;
    };

    vector<int> cur_cycle = build_cycle_from_tree(in_tree);
    int cur_score = calc_score(cur_cycle);

    vector<unsigned char> best_tree = in_tree;
    vector<int> best_cycle = cur_cycle;
    int best_score = cur_score;

    XorShift rng((uint64_t)chrono::high_resolution_clock::now().time_since_epoch().count());

    auto start_time = chrono::high_resolution_clock::now();

    const double TIME_LIMIT = 1.65;
    const double T0 = 600.0;
    const double T1 = 0.05;

    double temp = T0;
    int iter = 0;

    auto find_path_edges = [&](const vector<unsigned char>& tree, int s, int t) {
        int par[K];
        int pare[K];

        fill(par, par + K, -1);
        fill(pare, pare + K, -1);

        queue<int> q;
        par[s] = s;
        q.push(s);

        while (!q.empty()) {
            int u = q.front();
            q.pop();

            if (u == t) break;

            int r = u / B;
            int c = u % B;

            const int dr[4] = {-1, 1, 0, 0};
            const int dc[4] = {0, 0, -1, 1};

            for (int z = 0; z < 4; z++) {
                int nr = r + dr[z];
                int nc = c + dc[z];

                if (nr < 0 || nr >= B || nc < 0 || nc >= B) continue;

                int v = bid(nr, nc);
                int e = edge_id[u][v];

                if (e < 0) continue;
                if (!tree[e]) continue;
                if (par[v] != -1) continue;

                par[v] = u;
                pare[v] = e;
                q.push(v);
            }
        }

        vector<int> path;

        for (int v = t; v != s; v = par[v]) {
            path.push_back(pare[v]);
        }

        return path;
    };

    while (true) {
        if ((iter & 63) == 0) {
            double elapsed = chrono::duration<double>(
                chrono::high_resolution_clock::now() - start_time
            ).count();

            if (elapsed >= TIME_LIMIT) break;

            double p = elapsed / TIME_LIMIT;
            temp = T0 * pow(T1 / T0, p);
        }

        iter++;

        int add_e;

        do {
            add_e = rng.randint(E);
        } while (in_tree[add_e]);

        int s = macro_edges[add_e].first;
        int t = macro_edges[add_e].second;

        vector<int> path = find_path_edges(in_tree, s, t);
        int rem_e = path[rng.randint((int)path.size())];

        in_tree[add_e] = 1;
        in_tree[rem_e] = 0;

        vector<int> new_cycle = build_cycle_from_tree(in_tree);
        int new_score = calc_score(new_cycle);

        int diff = new_score - cur_score;

        bool accept = false;

        if (diff <= 0) {
            accept = true;
        } else {
            double prob = exp(-(double)diff / temp);
            if (rng.uniform01() < prob) accept = true;
        }

        if (accept) {
            cur_score = new_score;
            cur_cycle.swap(new_cycle);

            if (cur_score < best_score) {
                best_score = cur_score;
                best_tree = in_tree;
                best_cycle = cur_cycle;
            }
        } else {
            in_tree[add_e] = 0;
            in_tree[rem_e] = 1;
        }
    }

    int pos[L];

    for (int i = 0; i < L; i++) {
        pos[best_cycle[i]] = i;
    }

    int exit_pos = pos[exit_cell];

    vector<pair<int, int>> ops;
    ops.reserve(best_score);

    int cur_offset = 0;

    for (int b = first_box; b < L; b++) {
        int need = (exit_pos - pos[cell_of_box[b]] + L) % L;
        int delta = (need - cur_offset + L) % L;

        if (delta <= L - delta) {
            for (int z = 0; z < delta; z++) {
                ops.push_back({0, 1});
            }
        } else {
            for (int z = 0; z < L - delta; z++) {
                ops.push_back({0, -1});
            }
        }

        cur_offset = need;
    }

    cout << 1 << '\n';

    cout << L;

    for (int c : best_cycle) {
        cout << ' ' << rid(c) << ' ' << cjd(c);
    }

    cout << '\n';

    if ((int)ops.size() > 100000) {
        ops.resize(100000);
    }

    cout << ops.size() << '\n';

    for (auto [m, d] : ops) {
        cout << m << ' ' << d << '\n';
    }

    return 0;
}