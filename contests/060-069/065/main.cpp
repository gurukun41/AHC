#include <bits/stdc++.h>
using namespace std;

using ll = long long;
static constexpr int FIXED_N = 20;
static constexpr int CELL_COUNT = FIXED_N * FIXED_N;
using CellValue = int16_t;

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
    array<CellValue, CELL_COUNT> box_at{};
    array<CellValue, CELL_COUNT> pos_of_box{};
    int next_box = 0;
    int cost = 0;
    int trace_id = -1;
    int pending_parent = -1;
    int pending_start = -1;
    int pending_path_id = -1;
    double eval = 0.0;
};

struct TraceNode {
    int parent = -1;
    int start = -1;
    int path_id = -1;
};

class Solver {
  public:
    static constexpr int TURN_LIMIT = 100000;
    static constexpr int BEAM_WIDTH = 96;
    static constexpr int PATH_CACHE_LIMIT = 64;
    static constexpr int EXPAND_PATH_LIMIT = 20;
    static constexpr int EXTRA_PATH_LEN = 4;
    static constexpr int HEURISTIC_DEPTH = 24;
    static constexpr int STATE_HASH_DEPTH = 40;

    void run() {
        read_input();
        build_conveyors();
        build_memberships();
        build_initial_state();
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
    vector<vector<Move>> sorted_graph;
    vector<vector<int>> rotate_dest;
    vector<int> dist_to_exit;
    vector<vector<vector<Operation>>> candidate_cache;
    vector<char> candidate_ready;

    // box_at[cell] is the box currently on the cell, or -1 if the cell is empty.
    array<CellValue, CELL_COUNT> box_at{};
    array<CellValue, CELL_COUNT> pos_of_box{};
    int next_box = 0;
    int exit_cell = -1;

    void read_input() {
        cin >> N;
        a.assign(N, vector<int>(N));
        for (int i = 0; i < N; i++) {
            for (int j = 0; j < N; j++) cin >> a[i][j];
        }
    }

    void build_conveyors() {
        conveyors.clear();
        conveyors.reserve(N);

        for (int band = 0; band < N / 2; band++) {
            const int r0 = 2 * band;
            const int r1 = r0 + 1;
            vector<int> cells;
            cells.reserve(2 * N);

            for (int j = 0; j < N; j++) cells.push_back(id(r0, j));
            for (int j = N - 1; j >= 0; j--) cells.push_back(id(r1, j));
            conveyors.push_back({cells});
        }

        for (int band = 0; band < N / 2; band++) {
            const int c0 = 2 * band;
            const int c1 = c0 + 1;
            vector<int> cells;
            cells.reserve(2 * N);

            for (int i = 0; i < N; i++) cells.push_back(id(i, c0));
            for (int i = N - 1; i >= 0; i--) cells.push_back(id(i, c1));
            conveyors.push_back({cells});
        }

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
        box_at.fill(-1);
        pos_of_box.fill(-1);
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
        rotate_dest.assign(2 * conveyors.size(), {});

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

        for (int m = 0; m < (int)conveyors.size(); m++) {
            const Conveyor &conveyor = conveyors[m];
            const int L = (int)conveyor.cells.size();

            rotate_dest[2 * m + 0].resize(L);
            rotate_dest[2 * m + 1].resize(L);

            for (int p = 0; p < L; p++) {
                rotate_dest[2 * m + 0][p] = conveyor.cells[(p + L - 1) % L];
                rotate_dest[2 * m + 1][p] = conveyor.cells[(p + 1) % L];
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

        for (int d : dist_to_exit) assert(d != -1);

        sorted_graph = graph;

        for (vector<Move> &moves : sorted_graph) {
            sort(moves.begin(), moves.end(), [&](const Move &lhs, const Move &rhs) {
                if (dist_to_exit[lhs.to] != dist_to_exit[rhs.to]) {
                    return dist_to_exit[lhs.to] < dist_to_exit[rhs.to];
                }
                if (lhs.operation.m != rhs.operation.m) return lhs.operation.m < rhs.operation.m;
                return lhs.operation.d < rhs.operation.d;
            });
        }

        candidate_cache.assign(V, {});
        candidate_ready.assign(V, 0);
    }

    void solve_by_beam_search() {
        const int V = N * N;
        vector<TraceNode> trace;
        trace.reserve(200000);
        trace.push_back({-1, -1, -1});

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

            auto better_state = [](const BeamState &lhs, const BeamState &rhs) {
                if (lhs.eval != rhs.eval) return lhs.eval < rhs.eval;
                return lhs.cost < rhs.cost;
            };

            for (const BeamState &state : beam) {
                assert(state.next_box == target);
                const int start = state.pos_of_box[target];
                assert(start >= 0);
                assert(start != exit_cell);

                const vector<vector<Operation>> &paths = candidate_paths(start);

                vector<BeamState> local;
                local.reserve(EXPAND_PATH_LIMIT + 1);

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
                    child.pending_parent = state.trace_id;
                    child.pending_start = start;
                    child.pending_path_id = path_id;
                    local.push_back(std::move(child));

                    int pos = (int)local.size() - 1;
                    while (pos > 0 && better_state(local[pos], local[pos - 1])) {
                        swap(local[pos], local[pos - 1]);
                        pos--;
                    }

                    if ((int)local.size() > EXPAND_PATH_LIMIT) {
                        local.pop_back();
                    }
                }

                for (BeamState &state : local) {
                    pool.push_back(std::move(state));
                }
            }

            assert(!pool.empty());
            int scan_limit = min((int)pool.size(), BEAM_WIDTH * 8);

            if ((int)pool.size() > scan_limit) {
                nth_element(pool.begin(), pool.begin() + scan_limit, pool.end(), better_state);
            }

            sort(pool.begin(), pool.begin() + scan_limit, better_state);

            beam.clear();
            vector<uint64_t> seen;
            seen.reserve(BEAM_WIDTH);

            for (int i = 0; i < scan_limit; i++) {
                BeamState &state = pool[i];
                const uint64_t h = state_hash(state);
                bool duplicate = false;

                for (uint64_t x : seen) {
                    if (x == h) {
                        duplicate = true;
                        break;
                    }
                }

                if (duplicate) continue;

                seen.push_back(h);

                if (state.pending_parent != -1) {
                    trace.push_back({state.pending_parent, state.pending_start, state.pending_path_id});
                    state.trace_id = (int)trace.size() - 1;
                    state.pending_parent = -1;
                    state.pending_start = -1;
                    state.pending_path_id = -1;
                }

                beam.push_back(std::move(state));
                if ((int)beam.size() >= BEAM_WIDTH) break;
            }

            if (beam.empty()) {
                BeamState state = std::move(pool.front());

                if (state.pending_parent != -1) {
                    trace.push_back({state.pending_parent, state.pending_start, state.pending_path_id});
                    state.trace_id = (int)trace.size() - 1;
                    state.pending_parent = -1;
                    state.pending_start = -1;
                    state.pending_path_id = -1;
                }

                beam.push_back(std::move(state));
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

    const vector<vector<Operation>> &candidate_paths(int start) {
        if (candidate_ready[start]) return candidate_cache[start];
        candidate_ready[start] = 1;

        vector<vector<Operation>> paths;
        if (start == exit_cell) {
            paths.push_back({});
            candidate_cache[start] = paths;
            return candidate_cache[start];
        }

        vector<int> order;
        vector<char> visited(N * N, 0);
        vector<Operation> current_path;
        visited[start] = 1;

        const int base = dist_to_exit[start];
        for (int limit = base; limit <= base + EXTRA_PATH_LEN && (int)paths.size() < PATH_CACHE_LIMIT; limit++) {
            enumerate_paths(start, limit, visited, current_path, paths);
        }

        assert(!paths.empty());
        candidate_cache[start] = paths;
        return candidate_cache[start];
    }

    void enumerate_paths(int cell, int remaining, vector<char> &visited, vector<Operation> &current_path,
                         vector<vector<Operation>> &paths) const {
        if ((int)paths.size() >= PATH_CACHE_LIMIT) return;
        if (dist_to_exit[cell] > remaining) return;

        if (cell == exit_cell) {
            paths.push_back(current_path);
            return;
        }
        if (remaining == 0) return;

        for (const Move &move : sorted_graph[cell]) {
            const int next_cell = move.to;
            if (visited[next_cell]) continue;
            if (dist_to_exit[next_cell] > remaining - 1) continue;

            visited[next_cell] = 1;
            current_path.push_back(move.operation);
            enumerate_paths(next_cell, remaining - 1, visited, current_path, paths);
            current_path.pop_back();
            visited[next_cell] = 0;

            if ((int)paths.size() >= PATH_CACHE_LIMIT) return;
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
        vector<pair<int, int>> chunks;
        while (trace_id != -1) {
            if (trace[trace_id].start != -1) {
                chunks.push_back({trace[trace_id].start, trace[trace_id].path_id});
            }
            trace_id = trace[trace_id].parent;
        }
        reverse(chunks.begin(), chunks.end());

        vector<Operation> result;
        result.reserve(TURN_LIMIT);

        for (auto [start, path_id] : chunks) {
            const vector<Operation> &chunk = candidate_cache[start][path_id];
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
        const vector<int> &dst = rotate_dest[2 * m + (d == 1 ? 1 : 0)];
        const int L = (int)conveyor.cells.size();
        array<CellValue, 64> old{};
        assert(L <= (int)old.size());

        for (int p = 0; p < L; p++) old[p] = state.box_at[conveyor.cells[p]];

        for (int p = 0; p < L; p++) {
            const int next_cell = dst[p];
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
