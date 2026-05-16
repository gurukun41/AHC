#include <bits/stdc++.h>
using namespace std;

static constexpr int N = 20;
static constexpr int V = N * N;
static constexpr int INF = 1e9;
static constexpr int EXIT = N / 2; // id(0, N/2)

int id(int i, int j) { return i * N + j; }
int rr(int x) { return x / N; }
int cc(int x) { return x % N; }

struct XorShift {
    uint64_t x;
    XorShift(uint64_t seed = 88172645463325252ull) : x(seed) {}
    uint64_t next64() { x ^= x << 7; x ^= x >> 9; return x; }
    int randint(int n) { return int(next64() % uint64_t(n)); }
    double uniform01() { return (next64() >> 11) * (1.0 / 9007199254740992.0); }
};

struct Candidate {
    vector<int> cells;
    bool fixed = false;
};

struct Move {
    int to;
    int m;
    int d;
};

struct Op {
    int m;
    int d;
};

struct Plan {
    vector<Op> ops;
    int gain = 0;
    bool ok = false;
};

struct State {
    array<int, V> cell_box{};
    array<int, V> box_cell{};
    int next_box = 0;
};

struct Context {
    vector<vector<int>> loops;
    vector<vector<Move>> moves;
    vector<array<int, V>> nxt_pos; // 2*m+0 => d=-1, 2*m+1 => d=+1
    array<int, V> dist{};
    int unreachable = 0;
    int total_len = 0;
};

void remove_ready(State& st) {
    while (st.next_box < V && st.cell_box[EXIT] == st.next_box) {
        int b = st.next_box;
        st.cell_box[EXIT] = -1;
        st.box_cell[b] = -1;
        st.next_box++;
    }
}

vector<int> rectangle_loop(int top, int left, int h, int w) {
    vector<int> res;

    for (int j = left; j < left + w; j++) {
        res.push_back(id(top, j));
    }

    for (int i = top + 1; i < top + h; i++) {
        res.push_back(id(i, left + w - 1));
    }

    for (int j = left + w - 2; j >= left; j--) {
        res.push_back(id(top + h - 1, j));
    }

    for (int i = top + h - 2; i > top; i--) {
        res.push_back(id(i, left));
    }

    return res;
}

void add_grid_edge(array<unsigned char, V>& mask, int a, int b) {
    int ai = rr(a);
    int aj = cc(a);
    int bi = rr(b);
    int bj = cc(b);

    if (bi == ai - 1 && bj == aj) {
        mask[a] |= 1;
        mask[b] |= 2;
    } else if (bi == ai + 1 && bj == aj) {
        mask[a] |= 2;
        mask[b] |= 1;
    } else if (bi == ai && bj == aj - 1) {
        mask[a] |= 4;
        mask[b] |= 8;
    } else if (bi == ai && bj == aj + 1) {
        mask[a] |= 8;
        mask[b] |= 4;
    }
}

void remove_grid_edge(array<unsigned char, V>& mask, int a, int b) {
    int ai = rr(a);
    int aj = cc(a);
    int bi = rr(b);
    int bj = cc(b);

    if (bi == ai - 1 && bj == aj) {
        mask[a] &= ~1u;
        mask[b] &= ~2u;
    } else if (bi == ai + 1 && bj == aj) {
        mask[a] &= ~2u;
        mask[b] &= ~1u;
    } else if (bi == ai && bj == aj - 1) {
        mask[a] &= ~4u;
        mask[b] &= ~8u;
    } else if (bi == ai && bj == aj + 1) {
        mask[a] &= ~8u;
        mask[b] &= ~4u;
    }
}

array<int, 4> block_cells(int top, int left) {
    return {
        id(top, left),
        id(top, left + 1),
        id(top + 1, left + 1),
        id(top + 1, left)
    };
}

vector<int> build_cycle_from_macro_tree(
    int off_i,
    int off_j,
    int H,
    int W,
    const vector<int>& blocks,
    const vector<pair<int, int>>& tree_edges
) {
    if (blocks.empty()) {
        return {};
    }

    vector<unsigned char> in(H * W, 0);

    for (int b : blocks) {
        in[b] = 1;
    }

    array<unsigned char, V> mask{};
    mask.fill(0);

    vector<int> used_cells;
    used_cells.reserve(4 * blocks.size());

    for (int b : blocks) {
        int r = b / W;
        int c = b % W;
        int top = off_i + 2 * r;
        int left = off_j + 2 * c;

        if (top < 0 || top + 1 >= N || left < 0 || left + 1 >= N) {
            return {};
        }

        auto q = block_cells(top, left);

        add_grid_edge(mask, q[0], q[1]);
        add_grid_edge(mask, q[1], q[2]);
        add_grid_edge(mask, q[2], q[3]);
        add_grid_edge(mask, q[3], q[0]);

        for (int x : q) {
            used_cells.push_back(x);
        }
    }

    for (auto [u, v] : tree_edges) {
        int r1 = u / W;
        int c1 = u % W;
        int r2 = v / W;
        int c2 = v % W;

        if (!in[u] || !in[v]) {
            return {};
        }

        if (abs(r1 - r2) + abs(c1 - c2) != 1) {
            return {};
        }

        if (r1 == r2) {
            if (c1 > c2) {
                swap(r1, r2);
                swap(c1, c2);
            }

            auto A = block_cells(off_i + 2 * r1, off_j + 2 * c1);
            auto B = block_cells(off_i + 2 * r2, off_j + 2 * c2);

            remove_grid_edge(mask, A[1], A[2]);
            remove_grid_edge(mask, B[0], B[3]);
            add_grid_edge(mask, A[1], B[0]);
            add_grid_edge(mask, A[2], B[3]);
        } else {
            if (r1 > r2) {
                swap(r1, r2);
                swap(c1, c2);
            }

            auto A = block_cells(off_i + 2 * r1, off_j + 2 * c1);
            auto B = block_cells(off_i + 2 * r2, off_j + 2 * c2);

            remove_grid_edge(mask, A[2], A[3]);
            remove_grid_edge(mask, B[0], B[1]);
            add_grid_edge(mask, A[3], B[0]);
            add_grid_edge(mask, A[2], B[1]);
        }
    }

    vector<unsigned char> mark(V, 0);

    for (int c : used_cells) {
        if (mark[c]) {
            return {};
        }

        mark[c] = 1;
    }

    auto neighs = [&](int cur) {
        vector<int> ns;
        int i = rr(cur);
        int j = cc(cur);

        if ((mask[cur] & 1) && i > 0) {
            ns.push_back(id(i - 1, j));
        }

        if ((mask[cur] & 2) && i + 1 < N) {
            ns.push_back(id(i + 1, j));
        }

        if ((mask[cur] & 4) && j > 0) {
            ns.push_back(id(i, j - 1));
        }

        if ((mask[cur] & 8) && j + 1 < N) {
            ns.push_back(id(i, j + 1));
        }

        return ns;
    };

    for (int c : used_cells) {
        if ((int)neighs(c).size() != 2) {
            return {};
        }
    }

    int start = used_cells[0];

    vector<int> cyc;
    cyc.reserve(used_cells.size());

    vector<unsigned char> seen(V, 0);

    int prev = -1;
    int cur = start;

    for (int step = 0; step < (int)used_cells.size(); step++) {
        if (seen[cur]) {
            return {};
        }

        seen[cur] = 1;
        cyc.push_back(cur);

        auto ns = neighs(cur);
        int nxt = (ns[0] == prev ? ns[1] : ns[0]);

        prev = cur;
        cur = nxt;
    }

    if (cur != start) {
        return {};
    }

    return cyc;
}

Context build_context_from_loops(const vector<vector<int>>& loops) {
    Context ctx;

    ctx.loops = loops;
    ctx.moves.assign(V, {});
    ctx.nxt_pos.resize(2 * loops.size());
    ctx.total_len = 0;

    for (auto& a : ctx.nxt_pos) {
        a.fill(-1);
    }

    for (int m = 0; m < (int)loops.size(); m++) {
        const auto& lp = loops[m];
        int L = (int)lp.size();

        ctx.total_len += L;

        for (int i = 0; i < L; i++) {
            int c = lp[i];
            int plus = lp[(i + 1) % L];
            int minus = lp[(i + L - 1) % L];

            ctx.moves[c].push_back({plus, m, 1});
            ctx.moves[c].push_back({minus, m, -1});

            ctx.nxt_pos[2 * m + 1][c] = plus;
            ctx.nxt_pos[2 * m + 0][c] = minus;
        }
    }

    ctx.dist.fill(INF);

    queue<int> q;
    ctx.dist[EXIT] = 0;
    q.push(EXIT);

    while (!q.empty()) {
        int v = q.front();
        q.pop();

        for (const auto& mv : ctx.moves[v]) {
            if (ctx.dist[mv.to] > ctx.dist[v] + 1) {
                ctx.dist[mv.to] = ctx.dist[v] + 1;
                q.push(mv.to);
            }
        }
    }

    ctx.unreachable = 0;

    for (int c = 0; c < V; c++) {
        if (ctx.dist[c] >= INF) {
            ctx.unreachable++;
        }
    }

    return ctx;
}

void apply_operation(State& st, const Context& ctx, int m, int d, bool remove_boxes) {
    const auto& lp = ctx.loops[m];
    int L = (int)lp.size();

    static int old[1000];

    for (int i = 0; i < L; i++) {
        old[i] = st.cell_box[lp[i]];
    }

    if (d == 1) {
        for (int i = 0; i < L; i++) {
            st.cell_box[lp[(i + 1) % L]] = old[i];
        }
    } else {
        for (int i = 0; i < L; i++) {
            st.cell_box[lp[(i + L - 1) % L]] = old[i];
        }
    }

    for (int c : lp) {
        int b = st.cell_box[c];

        if (b >= 0) {
            st.box_cell[b] = c;
        }
    }

    if (remove_boxes) {
        remove_ready(st);
    }
}

int collateral_gain(const State& st, const Context& ctx, int target, int m, int d, int W) {
    int idx = 2 * m + (d == 1 ? 1 : 0);
    int gain = 0;

    for (int b = target + 1; b < V && b <= target + W; b++) {
        int p = st.box_cell[b];

        if (p < 0) {
            continue;
        }

        int q = ctx.nxt_pos[idx][p];

        if (q < 0) {
            continue;
        }

        if (ctx.dist[q] < ctx.dist[p]) {
            int w = W - (b - target) + 1;
            gain += w;
        }
    }

    return gain;
}

Plan greedy_path_with_gain(const State& base, const Context& ctx, int target, int W) {
    Plan res;

    if (target >= V) {
        res.ok = true;
        return res;
    }

    int start = base.box_cell[target];

    if (start < 0) {
        res.ok = true;
        return res;
    }

    if (start == EXIT) {
        res.ok = true;
        return res;
    }

    if (ctx.dist[start] >= INF) {
        return res;
    }

    State tmp = base;
    int guard = 0;

    while (tmp.box_cell[target] != EXIT) {
        int cur = tmp.box_cell[target];

        if (++guard > 1000) {
            return Plan{};
        }

        int best_i = -1;
        int best_value = -INF;

        for (int i = 0; i < (int)ctx.moves[cur].size(); i++) {
            const auto& mv = ctx.moves[cur][i];

            if (ctx.dist[mv.to] != ctx.dist[cur] - 1) {
                continue;
            }

            int g = collateral_gain(tmp, ctx, target, mv.m, mv.d, W);
            int value = g * 100 + (int)ctx.loops[mv.m].size();

            if (value > best_value) {
                best_value = value;
                best_i = i;
            }
        }

        if (best_i == -1) {
            return Plan{};
        }

        Move mv = ctx.moves[cur][best_i];
        int g = collateral_gain(tmp, ctx, target, mv.m, mv.d, W);

        res.gain += g;
        res.ops.push_back({mv.m, mv.d});

        apply_operation(tmp, ctx, mv.m, mv.d, false);
    }

    res.ok = true;
    return res;
}

Plan random_path_with_gain(
    const State& base,
    const Context& ctx,
    int target,
    int W,
    int slack,
    XorShift& rng
) {
    Plan res;

    int start = base.box_cell[target];

    if (start < 0) {
        res.ok = true;
        return res;
    }

    if (start == EXIT) {
        res.ok = true;
        return res;
    }

    if (ctx.dist[start] >= INF) {
        return res;
    }

    int max_len = ctx.dist[start] + slack;

    State tmp = base;

    array<unsigned char, V> visit{};
    visit.fill(0);
    visit[start] = 1;

    while (tmp.box_cell[target] != EXIT && (int)res.ops.size() < max_len) {
        int cur = tmp.box_cell[target];

        struct Cand {
            Move mv;
            double weight;
        };

        vector<Cand> cs;
        double sum = 0.0;

        for (const auto& mv : ctx.moves[cur]) {
            if (ctx.dist[mv.to] >= INF) {
                continue;
            }

            if ((int)res.ops.size() + 1 + ctx.dist[mv.to] > max_len) {
                continue;
            }

            int g = collateral_gain(tmp, ctx, target, mv.m, mv.d, W);
            int detour = ctx.dist[mv.to] - max(0, ctx.dist[cur] - 1);

            double quality = 0.27 * g
                - 0.72 * detour
                - 0.18 * (int)visit[mv.to]
                + 0.006 * (int)ctx.loops[mv.m].size();

            double weight = exp(max(-40.0, min(40.0, quality)));

            cs.push_back({mv, weight});
            sum += weight;
        }

        if (cs.empty()) {
            return greedy_path_with_gain(base, ctx, target, W);
        }

        double x = rng.uniform01() * sum;
        Move chosen = cs.back().mv;

        for (const auto& ca : cs) {
            x -= ca.weight;

            if (x <= 0) {
                chosen = ca.mv;
                break;
            }
        }

        int g = collateral_gain(tmp, ctx, target, chosen.m, chosen.d, W);

        res.gain += g;
        res.ops.push_back({chosen.m, chosen.d});

        apply_operation(tmp, ctx, chosen.m, chosen.d, false);

        int now = tmp.box_cell[target];

        if (now < 0) {
            return Plan{};
        }

        if (visit[now] < 250) {
            visit[now]++;
        }
    }

    if (tmp.box_cell[target] != EXIT) {
        return greedy_path_with_gain(base, ctx, target, W);
    }

    res.ok = true;
    return res;
}

long long plan_score_secondary(const State& st_after, const Context& ctx, int target, int W) {
    long long s = 0;

    for (int b = target + 1; b < V && b <= target + W; b++) {
        int p = st_after.box_cell[b];

        if (p < 0) {
            continue;
        }

        if (ctx.dist[p] >= INF) {
            s += 10000;
        } else {
            s += 1LL * (W - (b - target) + 1) * ctx.dist[p];
        }
    }

    return s;
}

Plan choose_plan(const State& st, const Context& ctx, int target, int W, int tries, XorShift& rng) {
    Plan best = greedy_path_with_gain(st, ctx, target, W);

    if (!best.ok) {
        return best;
    }

    auto eval_plan = [&](const Plan& p) -> long long {
        State tmp = st;

        for (const auto& op : p.ops) {
            apply_operation(tmp, ctx, op.m, op.d, false);
        }

        if (target < V && tmp.box_cell[target] != EXIT && tmp.box_cell[target] != -1) {
            return (long long)4e18;
        }

        long long score = 0;

        score += 1LL * p.ops.size() * 1000LL;
        score -= 1LL * p.gain * 58LL;
        score += plan_score_secondary(tmp, ctx, target, min(W, 18)) * 3LL;

        return score;
    };

    long long best_score = eval_plan(best);

    for (int t = 0; t < tries; t++) {
        int slack = (t < tries / 3) ? rng.randint(3) : 2 + rng.randint(9);

        Plan p = random_path_with_gain(st, ctx, target, W, slack, rng);

        if (!p.ok) {
            continue;
        }

        long long sc = eval_plan(p);

        if (sc < best_score) {
            best_score = sc;
            best = move(p);
        }
    }

    return best;
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int inputN;
    cin >> inputN;

    State initial;

    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            int a;
            cin >> a;

            int c = id(i, j);

            initial.cell_box[c] = a;
            initial.box_cell[a] = c;
        }
    }

    initial.next_box = 0;
    remove_ready(initial);

    XorShift rng(
        (uint64_t)chrono::high_resolution_clock::now()
            .time_since_epoch()
            .count()
    );

    vector<Candidate> cand;
    vector<int> initial_selected;

    auto valid_loop = [&](const vector<int>& lp) -> bool {
        if ((int)lp.size() < 2 || (int)lp.size() > 72) {
            return false;
        }

        array<unsigned char, V> seen{};
        seen.fill(0);

        for (int c : lp) {
            if (c < 0 || c >= V || seen[c]) {
                return false;
            }

            seen[c] = 1;
        }

        int L = (int)lp.size();

        for (int i = 0; i < L; i++) {
            int a = lp[i];
            int b = lp[(i + 1) % L];

            if (abs(rr(a) - rr(b)) + abs(cc(a) - cc(b)) != 1) {
                return false;
            }
        }

        return true;
    };

    auto add_candidate = [&](const vector<int>& cells, bool fixed = false) -> int {
        if (!valid_loop(cells)) {
            return -1;
        }

        int idx = (int)cand.size();
        cand.push_back({cells, fixed});

        return idx;
    };

    vector<int> fixed_base;

    // Layer A: disjoint 2x2 loops. Fixed, short, and covers every cell exactly once.
    for (int i = 0; i < N; i += 2) {
        for (int j = 0; j < N; j += 2) {
            int k = add_candidate(rectangle_loop(i, j, 2, 2), true);
            fixed_base.push_back(k);
            initial_selected.push_back(k);
        }
    }

    // Layer B: shifted 2x2 loops. This gives initial local connectivity without any long conveyor.
    for (int i = 1; i + 1 < N; i += 2) {
        for (int j = 1; j + 1 < N; j += 2) {
            int k = add_candidate(rectangle_loop(i, j, 2, 2), false);
            initial_selected.push_back(k);
        }
    }

    // All 2x2 positions are candidates.
    for (int i = 0; i + 2 <= N; i++) {
        for (int j = 0; j + 2 <= N; j++) {
            add_candidate(rectangle_loop(i, j, 2, 2), false);
        }
    }

    // Small rectangle candidates remain, but no long row/column stripe is included.
    for (int h = 2; h <= 6; h++) {
        for (int w = 2; w <= 6; w++) {
            if (h == 2 && w == 2) {
                continue;
            }

            for (int i = 0; i + h <= N; i++) {
                for (int j = 0; j + w <= N; j++) {
                    add_candidate(rectangle_loop(i, j, h, w), false);
                }
            }
        }
    }

    int complex_start = (int)cand.size();

    auto add_random_tree_cycles = [&](int off_i, int off_j, int H, int W, int count) {
        if (H <= 0 || W <= 0) {
            return;
        }

        for (int t = 0; t < count; t++) {
            int target_blocks = 2 + rng.randint(15); // length <= 64
            target_blocks = min(target_blocks, H * W);

            vector<unsigned char> in(H * W, 0);
            vector<int> blocks;
            vector<pair<int, int>> edges;

            int s = rng.randint(H * W);

            in[s] = 1;
            blocks.push_back(s);

            vector<pair<int, int>> frontier;

            auto push_frontier = [&](int u) {
                int r = u / W;
                int c = u % W;

                const int dr[4] = {-1, 1, 0, 0};
                const int dc[4] = {0, 0, -1, 1};

                for (int z = 0; z < 4; z++) {
                    int nr = r + dr[z];
                    int nc = c + dc[z];

                    if (nr < 0 || nr >= H || nc < 0 || nc >= W) {
                        continue;
                    }

                    int v = nr * W + nc;

                    if (!in[v]) {
                        frontier.push_back({u, v});
                    }
                }
            };

            push_frontier(s);

            while ((int)blocks.size() < target_blocks && !frontier.empty()) {
                int p = rng.randint((int)frontier.size());

                auto [u, v] = frontier[p];

                frontier[p] = frontier.back();
                frontier.pop_back();

                if (in[v]) {
                    continue;
                }

                in[v] = 1;
                blocks.push_back(v);
                edges.push_back({u, v});

                push_frontier(v);
            }

            if ((int)blocks.size() >= 2) {
                auto cyc = build_cycle_from_macro_tree(off_i, off_j, H, W, blocks, edges);

                if (!cyc.empty()) {
                    add_candidate(cyc, false);
                }
            }
        }
    };

    add_random_tree_cycles(0, 0, 10, 10, 650);
    add_random_tree_cycles(0, 1, 10, 9, 450);
    add_random_tree_cycles(1, 0, 9, 10, 450);
    add_random_tree_cycles(1, 1, 9, 9, 450);

    const int C = (int)cand.size();

    vector<vector<int>> cand_by_cell(V);

    for (int k = 0; k < C; k++) {
        for (int c : cand[k].cells) {
            cand_by_cell[c].push_back(k);
        }
    }

    vector<int> selected;
    vector<int> pos(C, -1);
    vector<unsigned char> used(C, 0);

    array<unsigned char, V> cnt{};
    cnt.fill(0);

    auto add_loop_state = [&](int k) {
        used[k] = 1;
        pos[k] = (int)selected.size();
        selected.push_back(k);

        for (int c : cand[k].cells) {
            cnt[c]++;
        }
    };

    auto remove_loop_state = [&](int k) {
        int p = pos[k];
        int last = selected.back();

        selected[p] = last;
        pos[last] = p;

        selected.pop_back();

        pos[k] = -1;
        used[k] = 0;

        for (int c : cand[k].cells) {
            cnt[c]--;
        }
    };

    auto can_add = [&](int k) -> bool {
        if (k < 0 || k >= C || used[k]) {
            return false;
        }

        for (int c : cand[k].cells) {
            if (cnt[c] >= 2) {
                return false;
            }
        }

        return true;
    };

    for (int k : initial_selected) {
        add_loop_state(k);
    }

    auto selected_to_loops = [&](const vector<int>& sel) {
        vector<vector<int>> loops;
        loops.reserve(sel.size());

        for (int k : sel) {
            loops.push_back(cand[k].cells);
        }

        return loops;
    };

    auto evaluate_selected = [&](const vector<int>& sel, int tries_per_box) -> long long {
        vector<vector<int>> loops = selected_to_loops(sel);

        if (loops.empty()) {
            return (long long)4e18;
        }

        Context ctx = build_context_from_loops(loops);

        long long score = 0;

        if (ctx.unreachable > 0) {
            score += 1LL * ctx.unreachable * 3000000000LL;
        }

        score += 1LL * ctx.total_len * 2LL;

        State st = initial;

        const int K = 50;
        const int W = 28;

        int last_next = st.next_box;

        while (st.next_box < V && st.next_box < K) {
            remove_ready(st);

            int target = st.next_box;

            if (target >= K) {
                break;
            }

            if (st.box_cell[target] < 0) {
                st.next_box++;
                continue;
            }

            if (ctx.dist[st.box_cell[target]] >= INF) {
                score += 1000000000000LL;
                break;
            }

            Plan p = choose_plan(st, ctx, target, W, tries_per_box, rng);

            if (!p.ok) {
                score += 1000000000000LL;
                break;
            }

            // Main objective:
            // fewer ops is good, but shared useful movement for future boxes is rewarded.
            score += 1LL * p.ops.size() * 1000LL;
            score -= 1LL * p.gain * 72LL;

            for (const auto& op : p.ops) {
                apply_operation(st, ctx, op.m, op.d, true);
            }

            remove_ready(st);

            if (st.next_box == last_next) {
                score += 1000000000000LL;
                break;
            }

            last_next = st.next_box;
        }

        score -= 1LL * st.next_box * 3000LL;

        long long tail = 0;
        int lim = min(V, st.next_box + 80);

        for (int b = st.next_box; b < lim; b++) {
            int p = st.box_cell[b];

            if (p < 0) {
                continue;
            }

            tail += (ctx.dist[p] >= INF ? 10000 : ctx.dist[p]);
        }

        score += tail * 7LL;

        return score;
    };

    long long cur_score = evaluate_selected(selected, 2);
    long long best_score = cur_score;

    vector<int> best_selected = selected;

    auto start_time = chrono::high_resolution_clock::now();

    auto elapsed = [&]() -> double {
        return chrono::duration<double>(
            chrono::high_resolution_clock::now() - start_time
        ).count();
    };

    const double TIME_LIMIT = 1.35;
    const double T0 = max(2000.0, (double)cur_score * 0.02);
    const double T1 = 15.0;

    auto pick_candidate_any = [&]() -> int {
        int mode = rng.randint(100);

        if (mode < 35 && complex_start < C) {
            return complex_start + rng.randint(C - complex_start);
        }

        if (mode < 60) {
            int ei = rr(EXIT);
            int ej = cc(EXIT);

            int ni = max(0, min(N - 1, ei + rng.randint(11) - 5));
            int nj = max(0, min(N - 1, ej + rng.randint(11) - 5));

            auto& v = cand_by_cell[id(ni, nj)];

            if (!v.empty()) {
                return v[rng.randint((int)v.size())];
            }
        }

        if (mode < 85) {
            int cell = rng.randint(V);
            auto& v = cand_by_cell[cell];

            if (!v.empty()) {
                return v[rng.randint((int)v.size())];
            }
        }

        return rng.randint(C);
    };

    auto pick_removable = [&]() -> int {
        for (int trial = 0; trial < 80; trial++) {
            int k = selected[rng.randint((int)selected.size())];

            if (!cand[k].fixed) {
                return k;
            }
        }

        for (int k : selected) {
            if (!cand[k].fixed) {
                return k;
            }
        }

        return -1;
    };

    auto collect_blockers_for = [&](int k) {
        vector<pair<int, int>> scored;

        array<unsigned char, V> need{};
        need.fill(0);

        for (int c : cand[k].cells) {
            if (cnt[c] >= 2) {
                need[c] = 1;
            }
        }

        for (int s : selected) {
            if (cand[s].fixed) {
                continue;
            }

            int overlap = 0;

            for (int c : cand[s].cells) {
                if (need[c]) {
                    overlap++;
                }
            }

            if (overlap > 0) {
                scored.push_back({-overlap, s});
            }
        }

        sort(scored.begin(), scored.end());

        vector<int> res;

        for (auto [neg, s] : scored) {
            res.push_back(s);
        }

        return res;
    };

    while (true) {
        double t = elapsed();

        if (t >= TIME_LIMIT) {
            break;
        }

        double ptime = t / TIME_LIMIT;
        double temp = T0 * pow(T1 / T0, ptime);

        vector<int> removed;
        vector<int> added;

        int type = rng.randint(100);

        if (type < 15) {
            int k = pick_removable();

            if (k != -1) {
                remove_loop_state(k);
                removed.push_back(k);
            }
        } else if (type < 82) {
            int k = pick_candidate_any();

            if (!used[k]) {
                if (!can_add(k)) {
                    vector<int> blockers = collect_blockers_for(k);

                    int limit = (k >= complex_start ? 14 : 8);

                    for (int s : blockers) {
                        if (can_add(k)) {
                            break;
                        }

                        if ((int)removed.size() >= limit) {
                            break;
                        }

                        remove_loop_state(s);
                        removed.push_back(s);
                    }
                }

                if (can_add(k)) {
                    add_loop_state(k);
                    added.push_back(k);
                }
            }
        } else {
            int rmcnt = 1 + rng.randint(3);

            for (int z = 0; z < rmcnt; z++) {
                int k = pick_removable();

                if (k == -1) {
                    break;
                }

                remove_loop_state(k);
                removed.push_back(k);
            }

            int addcnt = 1 + rng.randint(3);

            for (int z = 0; z < addcnt; z++) {
                int k = pick_candidate_any();

                if (can_add(k)) {
                    add_loop_state(k);
                    added.push_back(k);
                }
            }
        }

        if (removed.empty() && added.empty()) {
            continue;
        }

        long long nxt_score = evaluate_selected(selected, 1);
        long long diff = nxt_score - cur_score;

        bool accept = false;

        if (diff <= 0) {
            accept = true;
        } else if (rng.uniform01() < exp(-(double)diff / max(1.0, temp))) {
            accept = true;
        }

        if (accept) {
            cur_score = nxt_score;

            if (cur_score < best_score) {
                best_score = cur_score;
                best_selected = selected;
            }
        } else {
            for (int k : added) {
                remove_loop_state(k);
            }

            for (int i = (int)removed.size() - 1; i >= 0; i--) {
                add_loop_state(removed[i]);
            }
        }
    }

    long long base_score = evaluate_selected(initial_selected, 4);
    long long refined_best_score = evaluate_selected(best_selected, 4);

    if (base_score < refined_best_score) {
        best_selected = initial_selected;
    }

    vector<vector<int>> final_loops = selected_to_loops(best_selected);
    Context final_ctx = build_context_from_loops(final_loops);

    if (final_ctx.unreachable > 0) {
        final_loops = selected_to_loops(initial_selected);
        final_ctx = build_context_from_loops(final_loops);
    }

    State st = initial;

    vector<pair<int, int>> answer_ops;
    answer_ops.reserve(30000);

    while (st.next_box < V && (int)answer_ops.size() < 100000) {
        remove_ready(st);

        int target = st.next_box;

        if (target >= V) {
            break;
        }

        if (st.box_cell[target] < 0) {
            st.next_box++;
            continue;
        }

        if (final_ctx.dist[st.box_cell[target]] >= INF) {
            break;
        }

        Plan p = choose_plan(st, final_ctx, target, 34, 10, rng);

        if (!p.ok || (p.ops.empty() && st.box_cell[target] != EXIT)) {
            p = greedy_path_with_gain(st, final_ctx, target, 34);
        }

        if (!p.ok) {
            break;
        }

        int before = st.next_box;

        for (const auto& op : p.ops) {
            if ((int)answer_ops.size() >= 100000) {
                break;
            }

            answer_ops.push_back({op.m, op.d});
            apply_operation(st, final_ctx, op.m, op.d, true);

            if (st.next_box > before) {
                break;
            }
        }

        remove_ready(st);

        if (st.next_box == before) {
            int cur = st.box_cell[target];
            bool moved = false;

            for (const auto& mv : final_ctx.moves[cur]) {
                if (final_ctx.dist[mv.to] == final_ctx.dist[cur] - 1) {
                    if ((int)answer_ops.size() >= 100000) {
                        break;
                    }

                    answer_ops.push_back({mv.m, mv.d});
                    apply_operation(st, final_ctx, mv.m, mv.d, true);

                    moved = true;
                    break;
                }
            }

            if (!moved) {
                break;
            }
        }
    }

    cout << final_loops.size() << '\n';

    for (const auto& lp : final_loops) {
        cout << lp.size();

        for (int c : lp) {
            cout << ' ' << rr(c) << ' ' << cc(c);
        }

        cout << '\n';
    }

    if ((int)answer_ops.size() > 100000) {
        answer_ops.resize(100000);
    }

    cout << answer_ops.size() << '\n';

    for (auto [m, d] : answer_ops) {
        cout << m << ' ' << d << '\n';
    }

    return 0;
}
