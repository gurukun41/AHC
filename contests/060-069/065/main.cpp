#include <bits/stdc++.h>
using namespace std;

using ll = long long;

struct Timer {
    chrono::steady_clock::time_point start;

    Timer() : start(chrono::steady_clock::now()) {}

    double elapsed_ms() const {
        auto now = chrono::steady_clock::now();
        return chrono::duration<double, milli>(now - start).count();
    }
};

struct XorShift {
    uint64_t x = 88172645463325252ULL;

    explicit XorShift(uint64_t seed = 88172645463325252ULL) : x(seed) {}

    uint64_t next_u64() {
        x ^= x << 7;
        x ^= x >> 9;
        return x;
    }

    int next_int(int l, int r) {
        return l + (int)(next_u64() % (uint64_t)(r - l));
    }

    double next_double() {
        return (next_u64() >> 11) * (1.0 / 9007199254740992.0);
    }
};

struct Point {
    int i;
    int j;
};

struct Conveyor {
    vector<int> cells;
};

struct Operation {
    int m;
    int d;
};

struct Move {
    int to;
    Operation operation;
};

struct BeamState {
    vector<int> box_at;
    vector<int> pos_of_box;
    int next_box = 0;
    int cost = 0;
    int trace_id = -1;
    double eval = 0.0;
};

struct TraceNode {
    int parent = -1;
    vector<Operation> path;
};

struct LayoutChoice {
    vector<Conveyor> layout;
    double score = 1e100;
};

struct LoopCandidate {
    Conveyor conveyor;
    array<uint64_t, 7> mask{};
    double prior = 0.0;
};

class Solver {
  public:
    static constexpr int TURN_LIMIT = 100000;
    static constexpr int BEAM_WIDTH = 64;
    static constexpr int PATH_CACHE_LIMIT = 48;
    static constexpr int EXPAND_PATH_LIMIT = 16;
    static constexpr int EXTRA_PATH_LEN = 4;
    static constexpr int HEURISTIC_DEPTH = 24;
    static constexpr int STATE_HASH_DEPTH = 40;
    static constexpr int LAYOUT_BEAM_WIDTH = 4;
    static constexpr int LAYOUT_PATH_CACHE_LIMIT = 12;
    static constexpr int LAYOUT_EXPAND_PATH_LIMIT = 4;
    static constexpr int LAYOUT_EXTRA_PATH_LEN = 2;
    static constexpr int LAYOUT_BOX_LIMIT = 30;
    static constexpr int LAYOUT_HEURISTIC_DEPTH = 30;
    static constexpr int LAYOUT_POOL_LIMIT = 220;
    static constexpr int LAYOUT_SEARCH_STEPS = 120;

    void run() {
        read_input();
        build_initial_state();
        build_conveyors();
        build_memberships();
        build_graph();
        build_distances();
        solve_by_beam_search();
        output();
    }

  private:
    int N;
    vector<vector<int>> a;
    vector<Conveyor> conveyors;
    vector<Operation> operations;
    vector<vector<pair<int, int>>> memberships;
    vector<vector<Move>> graph;
    vector<int> dist_to_exit;
    vector<vector<vector<Operation>>> candidate_cache;
    vector<char> candidate_ready;

    // box_at[cell] is the box currently on the cell, or -1 if the cell is empty.
    vector<int> box_at;
    vector<int> pos_of_box;
    int next_box = 0;
    int exit_cell = -1;

    void read_input() {
        cin >> N;
        a.assign(N, vector<int>(N));
        for (int i = 0; i < N; i++) {
            for (int j = 0; j < N; j++) cin >> a[i][j];
        }
    }

    vector<int> row_loop(int r0, int r1) const {
        vector<int> cells;
        cells.reserve(2 * N);

        for (int j = 0; j < N; j++) cells.push_back(id(r0, j));
        for (int j = N - 1; j >= 0; j--) cells.push_back(id(r1, j));
        return cells;
    }

    vector<int> col_loop(int c0, int c1) const {
        vector<int> cells;
        cells.reserve(2 * N);

        for (int i = 0; i < N; i++) cells.push_back(id(i, c0));
        for (int i = N - 1; i >= 0; i--) cells.push_back(id(i, c1));
        return cells;
    }

    vector<int> rect_loop(int top, int left, int h, int w) const {
        vector<int> cells;

        for (int j = left; j < left + w; j++) cells.push_back(id(top, j));
        for (int i = top + 1; i < top + h; i++) cells.push_back(id(i, left + w - 1));
        for (int j = left + w - 2; j >= left; j--) cells.push_back(id(top + h - 1, j));
        for (int i = top + h - 2; i > top; i--) cells.push_back(id(i, left));
        return cells;
    }

    vector<Conveyor> horizontal_stripes(int offset) const {
        vector<Conveyor> res;

        for (int r = offset; r + 1 < N; r += 2) {
            res.push_back({row_loop(r, r + 1)});
        }

        return res;
    }

    vector<Conveyor> vertical_stripes(int offset) const {
        vector<Conveyor> res;

        for (int c = offset; c + 1 < N; c += 2) {
            res.push_back({col_loop(c, c + 1)});
        }

        return res;
    }

    vector<Conveyor> layout_h_base_with_col_pattern(const vector<int> &long_col) const {
        vector<Conveyor> res = horizontal_stripes(0);

        for (int band = 0; band < N / 2; band++) {
            const int c = 2 * band;

            if (long_col[band]) {
                res.push_back({col_loop(c, c + 1)});
            } else {
                for (int r = 0; r < N; r += 2) {
                    res.push_back({rect_loop(r, c, 2, 2)});
                }
            }
        }

        return res;
    }

    vector<Conveyor> layout_v_base_with_row_pattern(const vector<int> &long_row) const {
        vector<Conveyor> res = vertical_stripes(0);

        for (int band = 0; band < N / 2; band++) {
            const int r = 2 * band;

            if (long_row[band]) {
                res.push_back({row_loop(r, r + 1)});
            } else {
                for (int c = 0; c < N; c += 2) {
                    res.push_back({rect_loop(r, c, 2, 2)});
                }
            }
        }

        return res;
    }

    array<uint64_t, 7> make_mask(const vector<int> &cells) const {
        array<uint64_t, 7> mask{};
        mask.fill(0);

        for (int cell : cells) {
            mask[cell >> 6] |= 1ULL << (cell & 63);
        }

        return mask;
    }

    bool overlap_mask(const array<uint64_t, 7> &a, const array<uint64_t, 7> &b) const {
        for (int i = 0; i < 7; i++) {
            if (a[i] & b[i]) return true;
        }

        return false;
    }

    string mask_key(const array<uint64_t, 7> &mask) const {
        string key;
        key.reserve(7 * 8);

        for (uint64_t x : mask) {
            for (int k = 0; k < 8; k++) {
                key.push_back((char)((x >> (8 * k)) & 255));
            }
        }

        return key;
    }

    double loop_prior(const vector<int> &cells) const {
        vector<char> inside(N * N, 0);

        for (int cell : cells) {
            inside[cell] = 1;
        }

        double score = 0.02 * (double)cells.size();

        const int K = min(N * N - 1, next_box + 180);

        for (int b = next_box; b + 1 < K; b++) {
            int p = pos_of_box[b];
            int q = pos_of_box[b + 1];

            if (p >= 0 && q >= 0 && inside[p] && inside[q]) {
                score += 2.0;
            }
        }

        for (int b = next_box; b + 8 < K; b++) {
            int p = pos_of_box[b];
            if (p < 0 || !inside[p]) continue;

            for (int d = 2; d <= 8; d++) {
                int q = pos_of_box[b + d];

                if (q >= 0 && inside[q]) {
                    score += 0.18;
                }
            }
        }

        if (inside[exit_cell]) {
            score += 4.0;
        }

        return score;
    }

    vector<LoopCandidate> build_second_layer_candidates() {
        vector<LoopCandidate> candidates;
        unordered_set<string> seen;

        auto add_loop = [&](vector<int> cells, double bonus = 0.0) {
            if ((int)cells.size() < 2) return;

            array<uint64_t, 7> mask = make_mask(cells);
            string key = mask_key(mask);

            if (!seen.insert(key).second) return;

            LoopCandidate candidate;
            candidate.conveyor = {std::move(cells)};
            candidate.mask = mask;
            candidate.prior = loop_prior(candidate.conveyor.cells) + bonus;
            candidates.push_back(std::move(candidate));
        };

        for (int c = 0; c + 1 < N; c++) {
            add_loop(col_loop(c, c + 1), 80.0);
        }

        for (int r = 0; r + 1 < N; r++) {
            add_loop(row_loop(r, r + 1), 80.0);
        }

        for (int i = 0; i + 2 <= N; i++) {
            for (int j = 0; j + 2 <= N; j++) {
                add_loop(rect_loop(i, j, 2, 2), 0.6);
            }
        }

        for (int h = 2; h <= 9; h++) {
            for (int w = 2; w <= 9; w++) {
                if (h == 2 && w == 2) continue;

                for (int i = 0; i + h <= N; i++) {
                    for (int j = 0; j + w <= N; j++) {
                        add_loop(rect_loop(i, j, h, w));
                    }
                }
            }
        }

        for (int b = next_box; b < N * N && b < next_box + 180; b++) {
            int p = pos_of_box[b];
            if (p < 0) continue;

            int r = point(p).i;
            int c = point(p).j;

            for (int h : {3, 4, 5, 6, 8, 10, 12}) {
                for (int w : {3, 4, 5, 6, 8, 10, 12}) {
                    if (2 * h + 2 * w - 4 > 64) continue;

                    int top = max(0, min(N - h, r - h / 2));
                    int left = max(0, min(N - w, c - w / 2));
                    add_loop(rect_loop(top, left, h, w), 1.5);
                }
            }
        }

        sort(candidates.begin(), candidates.end(), [](const LoopCandidate &lhs, const LoopCandidate &rhs) {
            if (lhs.prior != rhs.prior) return lhs.prior > rhs.prior;
            return lhs.conveyor.cells.size() > rhs.conveyor.cells.size();
        });

        if ((int)candidates.size() > LAYOUT_POOL_LIMIT) {
            candidates.resize(LAYOUT_POOL_LIMIT);
        }

        return candidates;
    }

    double evaluate_layout_candidate(const vector<Conveyor> &layout) {
        conveyors = layout;
        build_memberships();
        build_graph();
        build_distances();

        if (!all_reachable()) {
            return 1e100;
        }

        return evaluate_layout_by_light_beam();
    }

    vector<Conveyor> compose_layout(
        const vector<Conveyor> &base,
        const vector<LoopCandidate> &candidates,
        const vector<int> &selected
    ) const {
        vector<Conveyor> layout = base;
        layout.reserve(base.size() + selected.size());

        for (int id : selected) {
            layout.push_back(candidates[id].conveyor);
        }

        return layout;
    }

    LayoutChoice search_second_layer(
        const vector<Conveyor> &base,
        const vector<LoopCandidate> &candidates,
        const vector<int> &seed_selected
    ) {
        vector<int> selected = seed_selected;
        vector<Conveyor> cur_layout = compose_layout(base, candidates, selected);
        double cur_score = evaluate_layout_candidate(cur_layout);

        LayoutChoice best;
        best.layout = cur_layout;
        best.score = cur_score;

        vector<int> order(candidates.size());
        iota(order.begin(), order.end(), 0);

        int steps = 0;

        for (int pass = 0; pass < 3 && steps < LAYOUT_SEARCH_STEPS; pass++) {
            bool changed = false;

            for (int cid : order) {
                if (steps >= LAYOUT_SEARCH_STEPS) break;

                bool already_selected = false;
                vector<int> next_selected;
                next_selected.reserve(selected.size() + 1);

                for (int sid : selected) {
                    if (sid == cid) {
                        already_selected = true;
                        continue;
                    }

                    if (!overlap_mask(candidates[sid].mask, candidates[cid].mask)) {
                        next_selected.push_back(sid);
                    }
                }

                if (!already_selected) {
                    next_selected.push_back(cid);
                }

                if (next_selected == selected) {
                    continue;
                }

                steps++;

                vector<Conveyor> next_layout = compose_layout(base, candidates, next_selected);
                double score = evaluate_layout_candidate(next_layout);

                if (score + 1e-9 < cur_score) {
                    selected = std::move(next_selected);
                    cur_layout = std::move(next_layout);
                    cur_score = score;
                    changed = true;

                    if (score < best.score) {
                        best.layout = cur_layout;
                        best.score = score;
                    }
                }
            }

            if (!changed) {
                break;
            }
        }

        return best;
    }

    vector<int> select_seed_indices(
        const vector<LoopCandidate> &candidates,
        bool want_vertical
    ) const {
        vector<int> selected;
        array<uint64_t, 7> used{};
        used.fill(0);

        for (int i = 0; i < (int)candidates.size(); i++) {
            const auto &cells = candidates[i].conveyor.cells;
            if ((int)cells.size() != 2 * N) continue;

            array<char, 20> row_used{};
            array<char, 20> col_used{};
            row_used.fill(0);
            col_used.fill(0);

            for (int cell : cells) {
                row_used[point(cell).i] = 1;
                col_used[point(cell).j] = 1;
            }

            int row_count = 0;
            int col_count = 0;

            for (int i = 0; i < N; i++) {
                row_count += row_used[i];
                col_count += col_used[i];
            }

            bool vertical = (row_count == N && col_count == 2);
            bool horizontal = (row_count == 2 && col_count == N);

            if ((want_vertical && !vertical) || (!want_vertical && !horizontal)) {
                continue;
            }

            if (overlap_mask(used, candidates[i].mask)) {
                continue;
            }

            selected.push_back(i);

            for (int k = 0; k < 7; k++) {
                used[k] |= candidates[i].mask[k];
            }
        }

        return selected;
    }

    void build_conveyors() {
        vector<LoopCandidate> candidates = build_second_layer_candidates();

        LayoutChoice best;

        {
            vector<Conveyor> base = horizontal_stripes(0);
            vector<int> seed = select_seed_indices(candidates, true);
            LayoutChoice choice = search_second_layer(base, candidates, seed);

            if (choice.score < best.score) {
                best = std::move(choice);
            }
        }

        {
            vector<Conveyor> base = vertical_stripes(0);
            vector<int> seed = select_seed_indices(candidates, false);
            LayoutChoice choice = search_second_layer(base, candidates, seed);

            if (choice.score < best.score) {
                best = std::move(choice);
            }
        }

        if (best.layout.empty()) {
            vector<int> fallback(N / 2, 1);
            best.layout = layout_h_base_with_col_pattern(fallback);
        }

        conveyors = std::move(best.layout);
        validate_conveyors();
    }

    void build_memberships() {
        memberships.assign(N * N, {});
        for (int m = 0; m < (int)conveyors.size(); m++) {
            const int L = (int)conveyors[m].cells.size();
            for (int p = 0; p < L; p++) {
                memberships[conveyors[m].cells[p]].push_back({m, p});
            }
        }

        for (int cell = 0; cell < N * N; cell++) {
            assert((int)memberships[cell].size() <= 2);
        }
    }

    void build_initial_state() {
        const int V = N * N;
        exit_cell = id(0, N / 2);
        box_at.assign(V, -1);
        pos_of_box.assign(V, -1);
        next_box = 0;

        for (int i = 0; i < N; i++) {
            for (int j = 0; j < N; j++) {
                const int cell = id(i, j);
                const int box = a[i][j];
                box_at[cell] = box;
                pos_of_box[box] = cell;
            }
        }

        // Only box 0 can be removed before the first operation.
        if (box_at[exit_cell] == 0) {
            box_at[exit_cell] = -1;
            pos_of_box[0] = -1;
            next_box = 1;
        }
    }

    void build_graph() {
        const int V = N * N;
        graph.assign(V, {});

        for (int cell = 0; cell < V; cell++) {
            for (const auto &[m, pos] : memberships[cell]) {
                const Conveyor &conveyor = conveyors[m];
                const int L = (int)conveyor.cells.size();
                for (const int d : {-1, 1}) {
                    const int next_pos = (pos + d + L) % L;
                    graph[cell].push_back({conveyor.cells[next_pos], {m, d}});
                }
            }
        }
    }

    void build_distances() {
        const int V = N * N;
        dist_to_exit.assign(V, -1);
        queue<int> que;

        dist_to_exit[exit_cell] = 0;
        que.push(exit_cell);
        while (!que.empty()) {
            const int cell = que.front();
            que.pop();

            for (const Move &move : graph[cell]) {
                const int next_cell = move.to;
                if (dist_to_exit[next_cell] != -1) continue;

                dist_to_exit[next_cell] = dist_to_exit[cell] + 1;
                que.push(next_cell);
            }
        }

        candidate_cache.assign(V, {});
        candidate_ready.assign(V, 0);
    }

    bool all_reachable() const {
        for (int d : dist_to_exit) {
            if (d == -1) return false;
        }

        return true;
    }

    void solve_by_beam_search() {
        const int V = N * N;
        vector<TraceNode> trace;
        trace.reserve(200000);
        trace.push_back({-1, {}});

        BeamState initial;
        initial.box_at = box_at;
        initial.pos_of_box = pos_of_box;
        initial.next_box = next_box;
        initial.cost = 0;
        initial.trace_id = 0;
        initial.eval = evaluate(initial);

        vector<BeamState> beam;
        beam.push_back(std::move(initial));

        while (!beam.empty() && beam.front().next_box < V) {
            const int target = beam.front().next_box;
            vector<BeamState> pool;
            pool.reserve(BEAM_WIDTH * EXPAND_PATH_LIMIT);

            for (const BeamState &state : beam) {
                assert(state.next_box == target);
                const int start = state.pos_of_box[target];
                assert(start >= 0);
                assert(start != exit_cell);

                const vector<vector<Operation>> &paths = candidate_paths(start);

                struct LocalCandidate {
                    BeamState state;
                    int path_id;
                    double score;
                };

                vector<LocalCandidate> local;
                local.reserve(EXPAND_PATH_LIMIT + 1);

                auto better_local = [](const LocalCandidate &lhs, const LocalCandidate &rhs) {
                    if (lhs.score != rhs.score) return lhs.score < rhs.score;
                    if (lhs.state.eval != rhs.state.eval) return lhs.state.eval < rhs.state.eval;
                    return lhs.state.cost < rhs.state.cost;
                };

                for (int path_id = 0; path_id < (int)paths.size(); path_id++) {
                    const vector<Operation> &path = paths[path_id];
                    BeamState child;
                    child.box_at = state.box_at;
                    child.pos_of_box = state.pos_of_box;
                    child.next_box = state.next_box;
                    child.cost = state.cost + (int)path.size();
                    if (child.cost > TURN_LIMIT) continue;

                    for (const Operation &operation : path) {
                        apply_operation(child, operation);
                    }
                    if (child.next_box != target + 1) continue;

                    child.eval = evaluate(child);
                    const double score = child.eval;

                    LocalCandidate candidate{std::move(child), path_id, score};
                    local.push_back(std::move(candidate));

                    int pos = (int)local.size() - 1;
                    while (pos > 0 && better_local(local[pos], local[pos - 1])) {
                        swap(local[pos], local[pos - 1]);
                        pos--;
                    }

                    if ((int)local.size() > EXPAND_PATH_LIMIT) {
                        local.pop_back();
                    }
                }

                for (LocalCandidate &candidate : local) {
                    trace.push_back({state.trace_id, paths[candidate.path_id]});
                    candidate.state.trace_id = (int)trace.size() - 1;
                    pool.push_back(std::move(candidate.state));
                }
            }

            assert(!pool.empty());
            sort(pool.begin(), pool.end(), [](const BeamState &lhs, const BeamState &rhs) {
                if (lhs.eval != rhs.eval) return lhs.eval < rhs.eval;
                return lhs.cost < rhs.cost;
            });

            beam.clear();
            unordered_set<uint64_t> seen;
            seen.reserve(BEAM_WIDTH * 4);

            for (BeamState &state : pool) {
                const uint64_t h = state_hash(state);
                if (!seen.insert(h).second) continue;

                beam.push_back(std::move(state));
                if ((int)beam.size() >= BEAM_WIDTH) break;
            }

            if (beam.empty()) {
                beam.push_back(std::move(pool.front()));
            }
        }

        assert(!beam.empty());
        const BeamState *best = &beam.front();
        for (const BeamState &state : beam) {
            if (state.cost < best->cost) best = &state;
        }

        operations = restore_operations(trace, best->trace_id);
        assert((int)operations.size() <= TURN_LIMIT);
    }

    double evaluate_light_state(const BeamState &state) const {
        double value = state.cost;
        double weight = 0.85;

        for (int k = 0; k < LAYOUT_HEURISTIC_DEPTH; k++) {
            const int box = state.next_box + k;
            if (box >= N * N) break;

            const int pos = state.pos_of_box[box];
            if (pos != -1) {
                value += weight * dist_to_exit[pos];
            }

            weight *= 0.88;
        }

        return value;
    }

    double evaluate_layout_by_light_beam() {
        const int V = N * N;
        const int target_limit = min(V, next_box + LAYOUT_BOX_LIMIT);

        vector<vector<vector<Operation>>> light_cache(V);
        vector<char> light_ready(V, 0);

        BeamState initial;
        initial.box_at = box_at;
        initial.pos_of_box = pos_of_box;
        initial.next_box = next_box;
        initial.cost = 0;
        initial.eval = evaluate_light_state(initial);

        vector<BeamState> beam;
        beam.push_back(std::move(initial));

        while (!beam.empty() && beam.front().next_box < target_limit) {
            const int target = beam.front().next_box;
            vector<BeamState> pool;
            pool.reserve(LAYOUT_BEAM_WIDTH * LAYOUT_EXPAND_PATH_LIMIT);

            for (const BeamState &state : beam) {
                if (state.next_box != target) continue;

                const int start = state.pos_of_box[target];
                if (start < 0 || start == exit_cell || dist_to_exit[start] == -1) continue;

                const auto &paths = candidate_paths_limited(
                    start,
                    light_cache,
                    light_ready,
                    LAYOUT_PATH_CACHE_LIMIT,
                    LAYOUT_EXTRA_PATH_LEN
                );

                struct LocalCandidate {
                    BeamState state;
                    double score;
                };

                vector<LocalCandidate> local;
                local.reserve(LAYOUT_EXPAND_PATH_LIMIT + 1);

                auto better_local = [](const LocalCandidate &lhs, const LocalCandidate &rhs) {
                    if (lhs.score != rhs.score) return lhs.score < rhs.score;
                    return lhs.state.cost < rhs.state.cost;
                };

                for (const vector<Operation> &path : paths) {
                    BeamState child;
                    child.box_at = state.box_at;
                    child.pos_of_box = state.pos_of_box;
                    child.next_box = state.next_box;
                    child.cost = state.cost + (int)path.size();

                    for (const Operation &operation : path) {
                        apply_operation(child, operation);
                    }

                    if (child.next_box != target + 1) continue;

                    child.eval = evaluate_light_state(child);
                    const double score = child.eval;
                    local.push_back({std::move(child), score});

                    int pos = (int)local.size() - 1;
                    while (pos > 0 && better_local(local[pos], local[pos - 1])) {
                        swap(local[pos], local[pos - 1]);
                        pos--;
                    }

                    if ((int)local.size() > LAYOUT_EXPAND_PATH_LIMIT) {
                        local.pop_back();
                    }
                }

                for (LocalCandidate &candidate : local) {
                    pool.push_back(std::move(candidate.state));
                }
            }

            if (pool.empty()) {
                break;
            }

            sort(pool.begin(), pool.end(), [](const BeamState &lhs, const BeamState &rhs) {
                if (lhs.eval != rhs.eval) return lhs.eval < rhs.eval;
                return lhs.cost < rhs.cost;
            });

            beam.clear();
            unordered_set<uint64_t> seen;
            seen.reserve(LAYOUT_BEAM_WIDTH * 4);

            for (BeamState &state : pool) {
                const uint64_t h = state_hash(state);
                if (!seen.insert(h).second) continue;

                beam.push_back(std::move(state));
                if ((int)beam.size() >= LAYOUT_BEAM_WIDTH) break;
            }
        }

        double best = 1e100;

        for (const BeamState &state : beam) {
            const int remaining = max(0, target_limit - state.next_box);
            const double score = state.eval + 10000.0 * remaining;
            best = min(best, score);
        }

        return best;
    }

    const vector<vector<Operation>> &candidate_paths(int start) {
        return candidate_paths_limited(start, candidate_cache, candidate_ready, PATH_CACHE_LIMIT, EXTRA_PATH_LEN);
    }

    const vector<vector<Operation>> &candidate_paths_limited(
        int start,
        vector<vector<vector<Operation>>> &cache,
        vector<char> &ready,
        int path_limit,
        int extra_path_len
    ) const {
        if (ready[start]) return cache[start];
        ready[start] = 1;

        vector<vector<Operation>> paths;
        if (start == exit_cell) {
            paths.push_back({});
            cache[start] = paths;
            return cache[start];
        }

        vector<int> order;
        vector<char> visited(N * N, 0);
        vector<Operation> current_path;
        visited[start] = 1;

        const int base = dist_to_exit[start];
        for (int limit = base; limit <= base + extra_path_len && (int)paths.size() < path_limit; limit++) {
            enumerate_paths(start, limit, visited, current_path, paths, path_limit);
        }

        assert(!paths.empty());
        cache[start] = paths;
        return cache[start];
    }

    void enumerate_paths(int cell, int remaining, vector<char> &visited, vector<Operation> &current_path,
                         vector<vector<Operation>> &paths, int path_limit) const {
        if ((int)paths.size() >= path_limit) return;
        if (dist_to_exit[cell] > remaining) return;

        if (cell == exit_cell) {
            paths.push_back(current_path);
            return;
        }
        if (remaining == 0) return;

        vector<Move> moves = graph[cell];
        sort(moves.begin(), moves.end(), [&](const Move &lhs, const Move &rhs) {
            if (dist_to_exit[lhs.to] != dist_to_exit[rhs.to]) {
                return dist_to_exit[lhs.to] < dist_to_exit[rhs.to];
            }
            if (lhs.operation.m != rhs.operation.m) return lhs.operation.m < rhs.operation.m;
            return lhs.operation.d < rhs.operation.d;
        });

        for (const Move &move : moves) {
            const int next_cell = move.to;
            if (visited[next_cell]) continue;
            if (dist_to_exit[next_cell] > remaining - 1) continue;

            visited[next_cell] = 1;
            current_path.push_back(move.operation);
            enumerate_paths(next_cell, remaining - 1, visited, current_path, paths, path_limit);
            current_path.pop_back();
            visited[next_cell] = 0;

            if ((int)paths.size() >= path_limit) return;
        }
    }

    double evaluate(const BeamState &state) const {
        static constexpr double weights[HEURISTIC_DEPTH] = {
            1.10, 0.95, 0.80, 0.66, 0.54, 0.44, 0.36, 0.30,
            0.25, 0.21, 0.18, 0.15, 0.13, 0.11, 0.09, 0.075,
            0.062, 0.052, 0.043, 0.036, 0.030, 0.025, 0.020, 0.016,
        };

        double value = state.cost;
        for (int k = 0; k < HEURISTIC_DEPTH; k++) {
            const int box = state.next_box + k;
            if (box >= N * N) break;

            const int pos = state.pos_of_box[box];
            if (pos == -1) continue;
            value += weights[k] * dist_to_exit[pos];
        }
        return value;
    }

    uint64_t state_hash(const BeamState &state) const {
        uint64_t h = 1469598103934665603ULL;

        auto mix = [&](uint64_t v) {
            h ^= v + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2);
        };

        mix((uint64_t)state.next_box);

        for (int k = 0; k < STATE_HASH_DEPTH; k++) {
            const int box = state.next_box + k;
            if (box >= N * N) break;

            mix((uint64_t)(state.pos_of_box[box] + 2));
        }

        return h;
    }

    vector<Operation> restore_operations(const vector<TraceNode> &trace, int trace_id) const {
        vector<vector<Operation>> chunks;
        while (trace_id != -1) {
            chunks.push_back(trace[trace_id].path);
            trace_id = trace[trace_id].parent;
        }
        reverse(chunks.begin(), chunks.end());

        vector<Operation> result;
        for (const vector<Operation> &chunk : chunks) {
            result.insert(result.end(), chunk.begin(), chunk.end());
        }
        return result;
    }

    void apply_operation(BeamState &state, const Operation &operation) const {
        rotate_conveyor(state, operation.m, operation.d);
        eject_if_ready(state);
    }

    void rotate_conveyor(BeamState &state, int m, int d) const {
        const Conveyor &conveyor = conveyors[m];
        const int L = (int)conveyor.cells.size();
        vector<int> old(L);

        for (int p = 0; p < L; p++) old[p] = state.box_at[conveyor.cells[p]];

        for (int p = 0; p < L; p++) {
            const int next_p = (p + d + L) % L;
            const int next_cell = conveyor.cells[next_p];
            const int box = old[p];
            state.box_at[next_cell] = box;
            if (box != -1) state.pos_of_box[box] = next_cell;
        }
    }

    void eject_if_ready(BeamState &state) const {
        if (state.next_box < N * N && state.box_at[exit_cell] == state.next_box) {
            state.pos_of_box[state.next_box] = -1;
            state.box_at[exit_cell] = -1;
            state.next_box++;
        }
    }

    int id(int i, int j) const {
        return i * N + j;
    }

    Point point(int cell) const {
        return {cell / N, cell % N};
    }

    static bool adjacent(const Point &a, const Point &b) {
        return abs(a.i - b.i) + abs(a.j - b.j) == 1;
    }

    void validate_conveyors() const {
        vector<int> used_count(N * N, 0);
        for (const Conveyor &conveyor : conveyors) {
            const int L = (int)conveyor.cells.size();
            assert(L >= 2);

            set<int> seen;
            for (int p = 0; p < L; p++) {
                const int cell = conveyor.cells[p];
                assert(0 <= cell && cell < N * N);
                assert(seen.insert(cell).second);
                used_count[cell]++;

                const Point cur = point(cell);
                const Point nxt = point(conveyor.cells[(p + 1) % L]);
                assert(adjacent(cur, nxt));
            }
        }

        for (int count : used_count) assert(count <= 2);
    }

    void output() const {
        cout << conveyors.size() << '\n';
        for (const Conveyor &conveyor : conveyors) {
            cout << conveyor.cells.size();
            for (int cell : conveyor.cells) {
                const Point p = point(cell);
                cout << ' ' << p.i << ' ' << p.j;
            }
            cout << '\n';
        }

        cout << operations.size() << '\n';
        for (const Operation &operation : operations) {
            cout << operation.m << ' ' << operation.d << '\n';
        }
    }
};

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    Solver solver;
    solver.run();
    return 0;
}
