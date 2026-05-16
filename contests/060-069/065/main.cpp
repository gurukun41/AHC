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
    vector<Point> cells;
};

struct Operation {
    int m;
    int d;
};

class Solver {
  public:
    static constexpr int TURN_LIMIT = 100000;

    void run() {
        read_input();
        build_cycle();
        build_conveyors();
        build_initial_state();
        solve_by_adjacent_swaps();
        output();
    }

  private:
    int N;
    vector<vector<int>> a;
    vector<Point> cycle;
    vector<Conveyor> conveyors;
    vector<Operation> operations;

    // box_at[k] is the box currently on cycle[k], or -1 if the cell is empty.
    vector<int> box_at;
    vector<int> pos_of_box;
    int next_box = 0;

    void read_input() {
        cin >> N;
        a.assign(N, vector<int>(N));
        for (int i = 0; i < N; i++) {
            for (int j = 0; j < N; j++) cin >> a[i][j];
        }
    }

    void build_cycle() {
        const int exit_col = N / 2;
        cycle.clear();
        cycle.reserve(N * N);

        // Top row, from the exit to the right edge.
        for (int j = exit_col; j < N; j++) cycle.push_back({0, j});

        // A vertical snake over rows 1..N-1.
        for (int j = N - 1; j >= 0; j--) {
            if ((N - 1 - j) % 2 == 0) {
                for (int i = 1; i < N; i++) cycle.push_back({i, j});
            } else {
                for (int i = N - 1; i >= 1; i--) cycle.push_back({i, j});
            }
        }

        // Top row, from the left edge back to the cell just left of the exit.
        for (int j = 0; j < exit_col; j++) cycle.push_back({0, j});

        assert((int)cycle.size() == N * N);
        assert(cycle.front().i == 0 && cycle.front().j == exit_col);
        assert(adjacent(cycle.back(), cycle.front()));
    }

    void build_conveyors() {
        const int V = N * N;
        conveyors.clear();
        conveyors.reserve(V);
        for (int k = 0; k < V; k++) {
            conveyors.push_back({{cycle[k], cycle[(k + 1) % V]}});
        }
    }

    void build_initial_state() {
        const int V = N * N;
        box_at.assign(V, -1);
        pos_of_box.assign(V, -1);

        for (int k = 0; k < V; k++) {
            int box = a[cycle[k].i][cycle[k].j];
            box_at[k] = box;
            pos_of_box[box] = k;
        }

        // Only box 0 can be removed before the first operation.
        if (box_at[0] == 0) {
            box_at[0] = -1;
            pos_of_box[0] = -1;
            next_box = 1;
        }
    }

    void solve_by_adjacent_swaps() {
        const int V = N * N;

        while (next_box < V) {
            const int target = next_box;
            const int p = pos_of_box[target];
            assert(p >= 0);

            const int counter_clockwise = p;
            const int clockwise = V - p;

            if (counter_clockwise <= clockwise) {
                for (int e = p - 1; e >= 0; e--) add_swap(e);
            } else {
                for (int e = p; e < V; e++) add_swap(e);
            }

            assert(pos_of_box[target] == -1);
            assert((int)operations.size() <= TURN_LIMIT);
        }
    }

    void add_swap(int edge_id) {
        const int V = N * N;
        assert(0 <= edge_id && edge_id < V);
        assert((int)operations.size() < TURN_LIMIT);

        operations.push_back({edge_id, 1});

        int u = edge_id;
        int v = (edge_id + 1) % V;
        swap_cells(u, v);
        eject_if_ready();
    }

    void swap_cells(int u, int v) {
        int bu = box_at[u];
        int bv = box_at[v];
        swap(box_at[u], box_at[v]);
        if (bu != -1) pos_of_box[bu] = v;
        if (bv != -1) pos_of_box[bv] = u;
    }

    void eject_if_ready() {
        if (next_box < N * N && box_at[0] == next_box) {
            pos_of_box[next_box] = -1;
            box_at[0] = -1;
            next_box++;
        }
    }

    static bool adjacent(const Point &a, const Point &b) {
        return abs(a.i - b.i) + abs(a.j - b.j) == 1;
    }

    void output() const {
        cout << conveyors.size() << '\n';
        for (const Conveyor &conveyor : conveyors) {
            cout << conveyor.cells.size();
            for (const Point &p : conveyor.cells) {
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
