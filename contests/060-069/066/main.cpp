#pragma GCC optimize("O2")

#include <bits/stdc++.h>
using namespace std;

double get_time() {
    using namespace std::chrono;
    static const auto start = steady_clock::now();
    return duration<double>(steady_clock::now() - start).count();
}

namespace rnd {
static uint32_t X2 = 12345;
static uint32_t X3 = 0xcafef00d;
static uint64_t C_X1 = 0xd15ea5e5ULL << 32 | 23456;

inline uint32_t next() {
    uint64_t work = (uint64_t)X3 * 3487286589ULL;
    uint32_t ret = (X3 ^ X2) + ((uint32_t)C_X1 ^ (uint32_t)(work >> 32));
    X3 = X2;
    X2 = (uint32_t)C_X1;
    C_X1 = work + (C_X1 >> 32);
    return ret;
}

inline int get(int n) {
    return (int)((uint64_t)next() * (uint32_t)n >> 32);
}

inline int range(int l, int r) {
    return l + get(r - l);
}

template <class T>
void shuffle(vector<T> &a) {
    for (int i = (int)a.size() - 1; i > 0; --i) {
        swap(a[i], a[get(i + 1)]);
    }
}

template <class T, size_t N>
void shuffle(T (&a)[N]) {
    for (int i = (int)N - 1; i > 0; --i) {
        swap(a[i], a[get(i + 1)]);
    }
}
}

struct Pos {
    int r = 0;
    int c = 0;
};

struct Input {
    int N = 0;
    int M = 0;
    int T = 0;
    vector<string> v;
    vector<string> h;
    vector<Pos> ball;
    vector<Pos> basket;
};

Input read_input() {
    Input in;
    cin >> in.N >> in.M >> in.T;
    in.v.resize(in.N);
    for (int i = 0; i < in.N; ++i) cin >> in.v[i];
    in.h.resize(in.N - 1);
    for (int i = 0; i < in.N - 1; ++i) cin >> in.h[i];

    in.ball.resize(in.M);
    in.basket.resize(in.M);
    for (int k = 0; k < in.M; ++k) {
        cin >> in.ball[k].r >> in.ball[k].c >> in.basket[k].r >> in.basket[k].c;
    }
    return in;
}

class Solver {
  public:
    explicit Solver(Input input) : in(std::move(input)), total(in.N * in.N) {
        ball_id.resize(in.M);
        basket_id.resize(in.M);
        for (int k = 0; k < in.M; ++k) {
            ball_id[k] = id(in.ball[k]);
            basket_id[k] = id(in.basket[k]);
        }
        build_neighbor_table();
        build_basic_state_transition();
        build_shortest_paths();
    }

    vector<int> build_greedy_order() const {
        vector<int> order;
        vector<char> used(in.M, 0);
        int cur = id(Pos{0, 0});
        int dir = RIGHT;
        int remaining = in.M;
        int current_len = 0;

        while (remaining > 0) {
            Candidate best;
            best.cost = numeric_limits<int>::max();

            for (int k = 0; k < in.M; ++k) {
                if (used[k]) continue;

                Candidate cand;
                cand.k = k;
                cand.end_pos = basket_id[k];
                int d1 = dir;
                cand.cost = basic_move_len_fast(cur, dir, ball_id[k], d1) + 1;  // S
                cand.cost += basic_move_len_fast(ball_id[k], d1, basket_id[k], cand.end_dir) + 1;  // S

                if (cand.cost < best.cost) best = cand;
            }

            if (best.k == -1) break;
            if (current_len + best.cost > in.T) break;

            order.push_back(best.k);
            current_len += best.cost;
            used[best.k] = 1;
            cur = best.end_pos;
            dir = best.end_dir;
            --remaining;
        }

        return order;
    }

    vector<char> build_ops(const vector<int> &order) const {
        vector<char> answer;
        int cur = id(Pos{0, 0});
        int dir = RIGHT;

        for (int k : order) {
            Candidate cand = build_candidate(k, cur, dir);
            answer.insert(answer.end(), cand.ops.begin(), cand.ops.end());
            cur = cand.end_pos;
            dir = cand.end_dir;
        }

        return answer;
    }

    int time_limit() const {
        return in.T;
    }

    int ball_count() const {
        return in.M;
    }

    int ball_cell(int k) const {
        return ball_id[k];
    }

    int basket_cell(int k) const {
        return basket_id[k];
    }

    void apply_basic_op(char op, int &cell, int &dir) const {
        if (op == 'R') {
            dir = (dir + 1) % 4;
        } else if (op == 'L') {
            dir = (dir + 3) % 4;
        } else if (op == 'F') {
            int nxt = next_cell[cell][dir];
            if (nxt != -1) cell = nxt;
        }
    }

    void apply_basic_ops(const vector<char> &ops, int &cell, int &dir) const {
        for (char op : ops) apply_basic_op(op, cell, dir);
    }

    int apply_basic_state(int state, char op) const {
        int8_t code = basic_op_code(op);
        return code >= 0 ? basic_state_transition[state][code] : state;
    }

    vector<char> basic_move_ops(int from, int from_dir, int to, int &end_cell, int &end_dir) const {
        vector<char> ops;
        int dir = from_dir;
        append_path(from, to, dir, ops);
        end_cell = to;
        end_dir = dir;
        return ops;
    }

    int basic_move_len_fast(int from, int from_dir, int to, int &end_dir) const {
        int cur = from;
        int dir = from_dir;
        int len = 0;
        while (cur != to) {
            int next_dir = first_dir_at(cur, to);
            if (next_dir == NO_DIR) break;
            int diff = (next_dir - dir + 4) & 3;
            if (diff == 1 || diff == 3) {
                ++len;
            } else if (diff == 2) {
                len += 2;
            }
            ++len;  // F
            dir = next_dir;
            cur = neighbor(cur, next_dir);
        }
        end_dir = dir;
        return len;
    }

    struct RouteInfo {
        int len = 0;
        int p_count = 0;
        int end_cell = 0;
        int end_dir = 0;
    };

    RouteInfo basic_move_info(int from, int from_dir, int to) const {
        RouteInfo info;
        info.end_cell = to;
        info.len = basic_move_len_fast(from, from_dir, to, info.end_dir);
        return info;
    }

    vector<int> build_macro_transition(const vector<char> &macro) const {
        const int states = total * 4;
        vector<int> transition(states);

        vector<int8_t> codes;
        codes.reserve(macro.size());
        for (char op : macro) codes.push_back(basic_op_code(op));

        for (int st = 0; st < states; ++st) {
            int cur = st;
            for (int8_t code : codes) {
                if (code >= 0) cur = basic_state_transition[cur][code];
            }
            transition[st] = cur;
        }
        return transition;
    }

    vector<int> build_macro_transition_from_buttons(const vector<char> &buttons, const vector<int> &old_transition) const {
        const int states = total * 4;
        vector<int> transition(states);
        const bool has_old = !old_transition.empty();

        vector<int8_t> codes;
        codes.reserve(buttons.size());
        for (char op : buttons) {
            if (op == 'P') {
                codes.push_back(-1);
            } else {
                codes.push_back(basic_op_code(op));
            }
        }

        for (int st = 0; st < states; ++st) {
            int cur = st;
            for (int8_t code : codes) {
                if (code == -1) {
                    if (has_old) cur = old_transition[cur];
                } else if (code >= 0) {
                    cur = basic_state_transition[cur][code];
                }
            }
            transition[st] = cur;
        }
        return transition;
    }

    vector<char> macro_move_ops_with_transition(int from, int from_dir, int to, const vector<int> &macro_transition, int &end_cell, int &end_dir) const {
        const int states = total * 4;

        ensure_macro_bfs_buffers(states);
        next_macro_bfs_stamp();

        int start = from * 4 + from_dir;
        macro_bfs_dist[start] = 0;
        macro_bfs_seen[start] = macro_bfs_stamp;
        int q_head = 0;
        int q_tail = 0;
        macro_bfs_queue[q_tail++] = start;
        int goal = -1;
        const bool has_macro = !macro_transition.empty();

        auto push_next = [&](int cur_state, int ns, char op) {
            if (ns == cur_state || macro_bfs_seen[ns] == macro_bfs_stamp) return;
            macro_bfs_seen[ns] = macro_bfs_stamp;
            macro_bfs_dist[ns] = macro_bfs_dist[cur_state] + 1;
            macro_bfs_prev[ns] = cur_state;
            macro_bfs_op[ns] = op;
            macro_bfs_queue[q_tail++] = ns;
        };

        while (q_head < q_tail) {
            int cur_state = macro_bfs_queue[q_head++];
            int cur_dist = macro_bfs_dist[cur_state];
            if (goal != -1 && cur_dist > macro_bfs_dist[goal]) break;

            int dir = cur_state & 3;
            if ((cur_state >> 2) == to) {
                if (goal == -1 || cur_dist < macro_bfs_dist[goal] ||
                    (cur_dist == macro_bfs_dist[goal] && dir < (goal & 3))) {
                    goal = cur_state;
                }
                continue;
            }
            if (goal != -1 && cur_dist >= macro_bfs_dist[goal]) continue;

            push_next(cur_state, basic_state_transition[cur_state][1], 'R');
            push_next(cur_state, basic_state_transition[cur_state][2], 'L');
            push_next(cur_state, basic_state_transition[cur_state][0], 'F');
            if (has_macro) push_next(cur_state, macro_transition[cur_state], 'P');
        }

        vector<char> ops;
        if (goal == -1) {
            return basic_move_ops(from, from_dir, to, end_cell, end_dir);
        }

        ops.reserve(macro_bfs_dist[goal]);
        for (int cur = goal; cur != start; cur = macro_bfs_prev[cur]) {
            ops.push_back(macro_bfs_op[cur]);
        }
        reverse(ops.begin(), ops.end());
        end_cell = goal >> 2;
        end_dir = goal & 3;
        return ops;
    }

    RouteInfo macro_move_info_with_transition(int from, int from_dir, int to, const vector<int> &macro_transition) const {
        const int states = total * 4;

        ensure_macro_bfs_buffers(states);
        next_macro_bfs_stamp();

        int start = from * 4 + from_dir;
        macro_bfs_dist[start] = 0;
        macro_bfs_seen[start] = macro_bfs_stamp;
        int q_head = 0;
        int q_tail = 0;
        macro_bfs_queue[q_tail++] = start;
        int goal = -1;
        const bool has_macro = !macro_transition.empty();

        auto push_next = [&](int cur_state, int next_state, int is_p) {
            if (next_state == cur_state || macro_bfs_seen[next_state] == macro_bfs_stamp) return;
            macro_bfs_seen[next_state] = macro_bfs_stamp;
            macro_bfs_dist[next_state] = macro_bfs_dist[cur_state] + 1;
            macro_bfs_prev[next_state] = (cur_state << 1) | is_p;
            macro_bfs_queue[q_tail++] = next_state;
        };

        while (q_head < q_tail) {
            int cur_state = macro_bfs_queue[q_head++];
            int cur_dist = macro_bfs_dist[cur_state];
            if (goal != -1 && cur_dist > macro_bfs_dist[goal]) break;

            int dir = cur_state & 3;
            if ((cur_state >> 2) == to) {
                if (goal == -1 || cur_dist < macro_bfs_dist[goal] ||
                    (cur_dist == macro_bfs_dist[goal] && dir < (goal & 3))) {
                    goal = cur_state;
                }
                continue;
            }
            if (goal != -1 && cur_dist >= macro_bfs_dist[goal]) continue;

            push_next(cur_state, basic_state_transition[cur_state][1], 0);
            push_next(cur_state, basic_state_transition[cur_state][2], 0);
            push_next(cur_state, basic_state_transition[cur_state][0], 0);
            if (has_macro) push_next(cur_state, macro_transition[cur_state], 1);
        }

        if (goal == -1) return basic_move_info(from, from_dir, to);

        RouteInfo info;
        info.len = macro_bfs_dist[goal];
        info.end_cell = goal >> 2;
        info.end_dir = goal & 3;
        int p_count = 0;
        for (int cur = goal; cur != start;) {
            int packed_prev = macro_bfs_prev[cur];
            p_count += packed_prev & 1;
            cur = packed_prev >> 1;
        }
        info.p_count = p_count;
        return info;
    }


    template <class Emit>
    RouteInfo greedy_macro_route_with_transition(int from, int from_dir, int to, const vector<int> &macro_transition, Emit emit) const {
        RouteInfo info;
        int cell = from;
        int dir = from_dir;
        const bool has_macro = !macro_transition.empty();

        auto state_id = [](int c, int d) {
            return c * 4 + d;
        };

        auto basic_len = [&](int c, int d) {
            int end_dir = d;
            return basic_move_len_fast(c, d, to, end_dir);
        };

        auto apply_basic_emit = [&](char op) {
            emit(op);
            ++info.len;
            apply_basic_op(op, cell, dir);
        };

        auto apply_p_emit = [&]() {
            emit('P');
            ++info.len;
            ++info.p_count;
            int ns = macro_transition[state_id(cell, dir)];
            cell = ns / 4;
            dir = ns & 3;
        };

        auto basic_first_op = [&]() -> char {
            if (cell == to) return 0;
            int next_dir = first_dir_at(cell, to);
            if (next_dir == NO_DIR) return 0;
            int diff = (next_dir - dir + 4) & 3;
            if (diff == 0) return 'F';
            if (diff == 1) return 'R';
            if (diff == 2) return 'R';
            return 'L';
        };

        int start_basic = basic_len(cell, dir);
        int safety = max(20, start_basic * 4 + 200);

        while (cell != to && safety-- > 0) {
            int base = basic_len(cell, dir);
            if (base <= 0) break;

            char first = basic_first_op();
            if (first == 0) break;

            struct Choice {
                int est = INT_MAX;
                char a = 0;
                char b = 0;
                int p_count = 0;
            } best;
            best.est = base;
            best.a = first;


            auto consider_p = [&](int c, int d, int prefix_cost, char a, char b) {
                if (!has_macro) return;
                int st = state_id(c, d);
                int ns = macro_transition[st];
                if (ns == st) return;
                int nc = ns / 4;
                int nd = ns & 3;
                int est = prefix_cost + 1 + basic_len(nc, nd);
                if (est < best.est) {
                    best.est = est;
                    best.a = a;
                    best.b = b;
                    best.p_count = 1;
                }
            };

            consider_p(cell, dir, 0, 'P', 0);
            consider_p(cell, (dir + 1) & 3, 1, 'R', 'P');
            consider_p(cell, (dir + 3) & 3, 1, 'L', 'P');
            if (next_cell[cell][dir] != -1) {
                consider_p(next_cell[cell][dir], dir, 1, 'F', 'P');
            }

            if (best.p_count == 0) {
                apply_basic_emit(best.a);
            } else {
                if (best.a == 'P') {
                    apply_p_emit();
                } else {
                    apply_basic_emit(best.a);
                    apply_p_emit();
                }
            }
        }

        if (cell != to) {
            int end_cell = cell;
            int end_dir = dir;
            vector<char> rest = basic_move_ops(cell, dir, to, end_cell, end_dir);
            for (char op : rest) {
                emit(op);
            }
            info.len += (int)rest.size();
            cell = end_cell;
            dir = end_dir;
        }

        info.end_cell = cell;
        info.end_dir = dir;
        return info;
    }

    RouteInfo greedy_macro_move_info_with_transition(int from, int from_dir, int to, const vector<int> &macro_transition) const {
        auto noop = [](char) {};
        return greedy_macro_route_with_transition(from, from_dir, to, macro_transition, noop);
    }

    vector<char> greedy_macro_move_ops_with_transition(int from, int from_dir, int to, const vector<int> &macro_transition, int &end_cell, int &end_dir) const {
        vector<char> ops;
        int reserve_len = basic_move_len_fast(from, from_dir, to, end_dir);
        ops.reserve(max(0, reserve_len));
        auto emit = [&](char op) {
            ops.push_back(op);
        };
        RouteInfo info = greedy_macro_route_with_transition(from, from_dir, to, macro_transition, emit);
        end_cell = info.end_cell;
        end_dir = info.end_dir;
        return ops;
    }

  private:
    static constexpr int UP = 0;
    static constexpr int RIGHT = 1;
    static constexpr int DOWN = 2;
    static constexpr int LEFT = 3;
    static constexpr uint8_t NO_DIR = 255;
    static constexpr array<int, 4> DR = {-1, 0, 1, 0};
    static constexpr array<int, 4> DC = {0, 1, 0, -1};

    struct Candidate {
        int k = -1;
        int cost = 0;
        int end_pos = 0;
        int end_dir = RIGHT;
        vector<char> ops;
    };

    Input in;
    int total = 0;
    vector<int> ball_id;
    vector<int> basket_id;
    vector<array<int, 4>> next_cell;
    vector<array<int, 4>> basic_state_transition;  // F, R, L, S/other identity
    vector<uint8_t> first_dir;
    mutable vector<int> macro_bfs_dist;
    mutable vector<int> macro_bfs_prev;
    mutable vector<int> macro_bfs_queue;
    mutable vector<int> macro_bfs_seen;
    mutable vector<char> macro_bfs_op;
    mutable int macro_bfs_stamp = 1;

    int id(Pos p) const {
        return p.r * in.N + p.c;
    }

    Pos pos(int cell) const {
        return Pos{cell / in.N, cell % in.N};
    }

    bool can_move(int cell, int dir) const {
        return next_cell[cell][dir] != -1;
    }

    int neighbor(int cell, int dir) const {
        return next_cell[cell][dir];
    }

    uint8_t &first_dir_ref(int from, int to) {
        return first_dir[from * total + to];
    }

    uint8_t first_dir_at(int from, int to) const {
        return first_dir[from * total + to];
    }

    static int8_t basic_op_code(char op) {
        if (op == 'F') return 0;
        if (op == 'R') return 1;
        if (op == 'L') return 2;
        if (op == 'S') return 3;
        return -2;
    }

    void ensure_macro_bfs_buffers(int states) const {
        if ((int)macro_bfs_dist.size() == states) return;
        macro_bfs_dist.resize(states);
        macro_bfs_prev.resize(states);
        macro_bfs_queue.resize(states);
        macro_bfs_seen.assign(states, 0);
        macro_bfs_op.resize(states);
    }

    void next_macro_bfs_stamp() const {
        ++macro_bfs_stamp;
        if (macro_bfs_stamp == numeric_limits<int>::max()) {
            fill(macro_bfs_seen.begin(), macro_bfs_seen.end(), 0);
            macro_bfs_stamp = 1;
        }
    }

    void build_neighbor_table() {
        next_cell.assign(total, array<int, 4>{});
        for (int cell = 0; cell < total; ++cell) {
            Pos p = pos(cell);
            for (int dir = 0; dir < 4; ++dir) {
                int nr = p.r + DR[dir];
                int nc = p.c + DC[dir];
                next_cell[cell][dir] = -1;
                if (nr < 0 || nr >= in.N || nc < 0 || nc >= in.N) continue;

                bool open = false;
                if (dir == UP) open = in.h[p.r - 1][p.c] == '0';
                if (dir == DOWN) open = in.h[p.r][p.c] == '0';
                if (dir == LEFT) open = in.v[p.r][p.c - 1] == '0';
                if (dir == RIGHT) open = in.v[p.r][p.c] == '0';
                if (open) next_cell[cell][dir] = id(Pos{nr, nc});
            }
        }
    }


    void build_basic_state_transition() {
        const int states = total * 4;
        basic_state_transition.assign(states, array<int, 4>{});
        for (int cell = 0; cell < total; ++cell) {
            for (int dir = 0; dir < 4; ++dir) {
                const int st = cell * 4 + dir;
                const int f_cell = next_cell[cell][dir] == -1 ? cell : next_cell[cell][dir];
                basic_state_transition[st][0] = f_cell * 4 + dir;                  // F
                basic_state_transition[st][1] = cell * 4 + ((dir + 1) & 3);        // R
                basic_state_transition[st][2] = cell * 4 + ((dir + 3) & 3);        // L
                basic_state_transition[st][3] = st;                               // S / identity
            }
        }
    }

    void build_shortest_paths() {
        first_dir.assign(total * total, NO_DIR);
        vector<int> dist(total);
        vector<int> q(total);

        for (int s = 0; s < total; ++s) {
            fill(dist.begin(), dist.end(), -1);
            int head = 0;
            int tail = 0;
            dist[s] = 0;
            q[tail++] = s;

            while (head < tail) {
                int cur = q[head++];

                for (int dir = 0; dir < 4; ++dir) {
                    if (!can_move(cur, dir)) continue;
                    int nxt = neighbor(cur, dir);
                    if (dist[nxt] != -1) continue;

                    dist[nxt] = dist[cur] + 1;
                    first_dir_ref(s, nxt) = (cur == s ? (uint8_t)dir : first_dir_at(s, cur));
                    q[tail++] = nxt;
                }
            }
        }
    }

    static void append_turns(int next_dir, int &cur_dir, vector<char> &ops) {
        int diff = (next_dir - cur_dir + 4) % 4;
        if (diff == 1) {
            ops.push_back('R');
        } else if (diff == 2) {
            ops.push_back('R');
            ops.push_back('R');
        } else if (diff == 3) {
            ops.push_back('L');
        }
        cur_dir = next_dir;
    }

    void append_path(int from, int to, int &dir, vector<char> &ops) const {
        int cur = from;
        while (cur != to) {
            int next_dir = first_dir_at(cur, to);
            if (next_dir == NO_DIR) break;
            append_turns(next_dir, dir, ops);
            ops.push_back('F');
            cur = neighbor(cur, next_dir);
        }
    }

    Candidate build_candidate(int k, int cur, int dir) const {
        Candidate cand;
        cand.k = k;
        cand.end_pos = basket_id[k];
        cand.end_dir = dir;

        append_path(cur, ball_id[k], cand.end_dir, cand.ops);
        cand.ops.push_back('S');
        append_path(ball_id[k], basket_id[k], cand.end_dir, cand.ops);
        cand.ops.push_back('S');

        cand.cost = (int)cand.ops.size();
        return cand;
    }
};

bool is_basic_op(char op) {
    return op == 'F' || op == 'R' || op == 'L' || op == 'S';
}

bool same_expansion(const vector<char> &encoded, const vector<char> &target) {
    vector<char> expanded;
    vector<char> last_macro;
    vector<char> recording_macro;
    expanded.reserve(target.size());

    bool recording = false;
    bool has_macro = false;

    auto append_basic = [&](char op) -> bool {
        expanded.push_back(op);
        if (expanded.size() > target.size()) return false;
        if (recording) recording_macro.push_back(op);
        return true;
    };

    for (char op : encoded) {
        if (is_basic_op(op)) {
            if (!append_basic(op)) return false;
        } else if (op == 'M') {
            if (recording) {
                last_macro = recording_macro;
                recording_macro.clear();
                recording = false;
                has_macro = true;
            } else {
                recording_macro.clear();
                recording = true;
            }
        } else if (op == 'P') {
            if (!has_macro) continue;
            for (char basic : last_macro) {
                if (!append_basic(basic)) return false;
            }
        } else {
            return false;
        }
    }

    return expanded == target;
}

vector<char> compress_with_single_macro(const vector<char> &base, int limit, int max_len_cap = 120) {
    const int n = (int)base.size();
    if (n < 6) return base;

    string s(base.begin(), base.end());
    const int max_len = min(n / 2, max_len_cap);
    if (max_len < 2) return base;

    struct HashKey {
        unsigned long long a = 0;
        unsigned long long b = 0;

        bool operator<(const HashKey &other) const {
            if (a != other.a) return a < other.a;
            return b < other.b;
        }

        bool operator==(const HashKey &other) const {
            return a == other.a && b == other.b;
        }
    };

    constexpr unsigned long long BASE1 = 1000003ULL;
    constexpr unsigned long long BASE2 = 1000033ULL;

    vector<unsigned long long> pow1(max_len + 1, 1), pow2(max_len + 1, 1);
    for (int i = 1; i <= max_len; ++i) {
        pow1[i] = pow1[i - 1] * BASE1;
        pow2[i] = pow2[i - 1] * BASE2;
    }

    vector<unsigned long long> pref1(n + 1, 0), pref2(n + 1, 0);
    for (int i = 0; i < n; ++i) {
        unsigned long long x = (unsigned long long)(unsigned char)s[i] + 1;
        pref1[i + 1] = pref1[i] * BASE1 + x;
        pref2[i + 1] = pref2[i] * BASE2 + x;
    }

    auto get_hash = [&](int l, int len) -> HashKey {
        return HashKey{
            pref1[l + len] - pref1[l] * pow1[len],
            pref2[l + len] - pref2[l] * pow2[len],
        };
    };

    struct Candidate {
        int saving = 0;
        int start = -1;
        int len = 0;
        int count = 0;
    };

    Candidate best;
    vector<pair<HashKey, int>> substrings;

    for (int len = 2; len <= max_len; ++len) {
        substrings.clear();
        substrings.reserve(n - len + 1);
        for (int i = 0; i + len <= n; ++i) {
            substrings.push_back({get_hash(i, len), i});
        }

        sort(substrings.begin(), substrings.end(), [](const auto &lhs, const auto &rhs) {
            if (!(lhs.first == rhs.first)) return lhs.first < rhs.first;
            return lhs.second < rhs.second;
        });

        for (int l = 0; l < (int)substrings.size();) {
            int r = l + 1;
            while (r < (int)substrings.size() && substrings[l].first == substrings[r].first) ++r;

            if (r - l >= 2) {
                int count = 0;
                int first = -1;
                int last = -len;
                for (int idx = l; idx < r; ++idx) {
                    int pos = substrings[idx].second;
                    if (pos < last + len) continue;
                    if (count == 0) first = pos;
                    ++count;
                    last = pos;
                }

                if (count >= 2) {
                    int saving = (count - 1) * (len - 1) - 2;
                    if (saving > best.saving || (saving == best.saving && len > best.len)) {
                        best = Candidate{saving, first, len, count};
                    }
                }
            }

            l = r;
        }
    }

    if (best.start == -1 || best.saving <= 0) return base;

    string pattern = s.substr(best.start, best.len);
    vector<char> encoded;
    encoded.reserve(n - best.saving);

    bool registered = false;
    for (int i = 0; i < n;) {
        bool match = i + best.len <= n && s.compare(i, best.len, pattern) == 0;

        if (i == best.start) {
            encoded.push_back('M');
            encoded.insert(encoded.end(), pattern.begin(), pattern.end());
            encoded.push_back('M');
            registered = true;
            i += best.len;
        } else if (registered && match) {
            encoded.push_back('P');
            i += best.len;
        } else {
            encoded.push_back(base[i]);
            ++i;
        }
    }

    if ((int)encoded.size() > limit) return base;
    if ((int)encoded.size() >= n) return base;
    if (!same_expansion(encoded, base)) return base;
    return encoded;
}

vector<char> compress_with_multiple_macros(const vector<char> &base, int limit, int max_len_cap = 120, int episode_keep = 40, int max_used_count = 6) {
    const int n = (int)base.size();
    if (n < 6) return base;

    string s(base.begin(), base.end());
    const int max_len = min(n / 2, max_len_cap);
    if (max_len < 2) return base;

    struct HashKey {
        unsigned long long a = 0;
        unsigned long long b = 0;

        bool operator<(const HashKey &other) const {
            if (a != other.a) return a < other.a;
            return b < other.b;
        }

        bool operator==(const HashKey &other) const {
            return a == other.a && b == other.b;
        }
    };

    struct Episode {
        int len = 0;
        int end = 0;
        int saving = 0;
    };

    constexpr unsigned long long BASE1 = 1000003ULL;
    constexpr unsigned long long BASE2 = 1000033ULL;

    vector<unsigned long long> pow1(max_len + 1, 1), pow2(max_len + 1, 1);
    for (int i = 1; i <= max_len; ++i) {
        pow1[i] = pow1[i - 1] * BASE1;
        pow2[i] = pow2[i - 1] * BASE2;
    }

    vector<unsigned long long> pref1(n + 1, 0), pref2(n + 1, 0);
    for (int i = 0; i < n; ++i) {
        unsigned long long x = (unsigned long long)(unsigned char)s[i] + 1;
        pref1[i + 1] = pref1[i] * BASE1 + x;
        pref2[i + 1] = pref2[i] * BASE2 + x;
    }

    auto get_hash = [&](int l, int len) -> HashKey {
        return HashKey{
            pref1[l + len] - pref1[l] * pow1[len],
            pref2[l + len] - pref2[l] * pow2[len],
        };
    };

    vector<vector<Episode>> episodes(n);
    vector<pair<HashKey, int>> substrings;

    for (int len = 2; len <= max_len; ++len) {
        substrings.clear();
        substrings.reserve(n - len + 1);
        for (int i = 0; i + len <= n; ++i) {
            substrings.push_back({get_hash(i, len), i});
        }

        sort(substrings.begin(), substrings.end(), [](const auto &lhs, const auto &rhs) {
            if (!(lhs.first == rhs.first)) return lhs.first < rhs.first;
            return lhs.second < rhs.second;
        });

        for (int l = 0; l < (int)substrings.size();) {
            int r = l + 1;
            while (r < (int)substrings.size() && substrings[l].first == substrings[r].first) ++r;

            int m = r - l;
            if (m >= 2) {
                vector<int> positions;
                positions.reserve(m);
                for (int idx = l; idx < r; ++idx) positions.push_back(substrings[idx].second);

                vector<int> nxt(m, m);
                int ptr = 0;
                for (int idx = 0; idx < m; ++idx) {
                    ptr = max(ptr, idx + 1);
                    while (ptr < m && positions[ptr] < positions[idx] + len) ++ptr;
                    nxt[idx] = ptr;
                }

                vector<int> cnt(m, 1), last(m);
                for (int idx = m - 1; idx >= 0; --idx) {
                    if (nxt[idx] < m) {
                        cnt[idx] = cnt[nxt[idx]] + 1;
                        last[idx] = last[nxt[idx]];
                    } else {
                        cnt[idx] = 1;
                        last[idx] = idx;
                    }
                }

                for (int idx = 0; idx < m; ++idx) {
                    if (cnt[idx] < 2) continue;

                    int start = positions[idx];
                    int cur = idx;
                    for (int used_count = 2; used_count <= min(cnt[idx], max_used_count); ++used_count) {
                        cur = nxt[cur];
                        if (cur >= m) break;
                        int saving = (used_count - 1) * (len - 1) - 2;
                        if (saving > 0) {
                            episodes[start].push_back(Episode{len, positions[cur] + len, saving});
                        }
                    }

                    int all_count = cnt[idx];
                    int saving = (all_count - 1) * (len - 1) - 2;
                    if (saving > 0) {
                        episodes[start].push_back(Episode{len, positions[last[idx]] + len, saving});
                    }
                }
            }

            l = r;
        }
    }

    for (auto &v : episodes) {
        if (v.empty()) continue;
        sort(v.begin(), v.end(), [](const Episode &lhs, const Episode &rhs) {
            if (lhs.len != rhs.len) return lhs.len < rhs.len;
            if (lhs.end != rhs.end) return lhs.end < rhs.end;
            return lhs.saving > rhs.saving;
        });
        v.erase(unique(v.begin(), v.end(), [](const Episode &lhs, const Episode &rhs) {
                    return lhs.len == rhs.len && lhs.end == rhs.end;
                }),
                v.end());

        sort(v.begin(), v.end(), [](const Episode &lhs, const Episode &rhs) {
            long long lhs_density = 1LL * lhs.saving * max(1, rhs.end - rhs.len);
            long long rhs_density = 1LL * rhs.saving * max(1, lhs.end - lhs.len);
            if (lhs.saving != rhs.saving) return lhs.saving > rhs.saving;
            if (lhs_density != rhs_density) return lhs_density > rhs_density;
            return lhs.end < rhs.end;
        });
        if ((int)v.size() > episode_keep) v.resize(episode_keep);
    }

    const int INF = 1e9;
    vector<int> dp(n + 1, INF);
    vector<int> choice_len(n, 0), choice_end(n, 0);
    dp[n] = 0;

    for (int i = n - 1; i >= 0; --i) {
        dp[i] = 1 + dp[i + 1];
        for (const Episode &ep : episodes[i]) {
            if (ep.end > n || ep.end <= i) continue;
            int encoded_segment_len = (ep.end - i) - ep.saving;
            int cand = encoded_segment_len + dp[ep.end];
            if (cand < dp[i]) {
                dp[i] = cand;
                choice_len[i] = ep.len;
                choice_end[i] = ep.end;
            }
        }
    }

    if (dp[0] >= n) return base;

    vector<char> encoded;
    encoded.reserve(dp[0]);

    for (int i = 0; i < n;) {
        int len = choice_len[i];
        int end = choice_end[i];
        if (len == 0) {
            encoded.push_back(base[i]);
            ++i;
            continue;
        }

        string pattern = s.substr(i, len);
        encoded.push_back('M');
        encoded.insert(encoded.end(), pattern.begin(), pattern.end());
        encoded.push_back('M');

        int j = i + len;
        while (j < end) {
            if (j + len <= end && s.compare(j, len, pattern) == 0) {
                encoded.push_back('P');
                j += len;
            } else {
                encoded.push_back(base[j]);
                ++j;
            }
        }

        i = end;
    }

    if ((int)encoded.size() > limit) return base;
    if ((int)encoded.size() >= n) return base;
    if (!same_expansion(encoded, base)) return base;
    return encoded;
}

vector<char> best_macro_compress(const vector<char> &base, int limit, bool fast) {
    vector<char> single = fast ? compress_with_single_macro(base, limit, 48)
                               : compress_with_single_macro(base, limit);
    vector<char> multiple = fast ? compress_with_multiple_macros(base, limit, 48, 16, 4)
                                 : compress_with_multiple_macros(base, limit);
    return multiple.size() < single.size() ? multiple : single;
}

int raw_order_score(const Solver &solver, const vector<int> &order) {
    constexpr int INF = 1000000000;
    int cell = 0;
    int dir = 1;
    int len = 0;

    for (int k : order) {
        int end_dir = dir;
        len += solver.basic_move_len_fast(cell, dir, solver.ball_cell(k), end_dir) + 1;  // S
        cell = solver.ball_cell(k);
        dir = end_dir;

        len += solver.basic_move_len_fast(cell, dir, solver.basket_cell(k), end_dir) + 1;  // S
        cell = solver.basket_cell(k);
        dir = end_dir;
    }

    if (len > solver.time_limit()) {
        return INF / 2 + len - solver.time_limit();
    }
    return len;
}

int basic_move_len(const Solver &solver, int from, int dir, int to) {
    int end_dir = 0;
    return solver.basic_move_len_fast(from, dir, to, end_dir);
}

vector<vector<int>> build_goal_reverse_orders(const Solver &solver) {
    const int m = solver.ball_count();
    vector<int> delivery(m, 0);
    vector<int> start_cost(m, 0);
    vector<vector<int>> link(m, vector<int>(m, 0));

    for (int k = 0; k < m; ++k) {
        int best_delivery = numeric_limits<int>::max();
        for (int dir = 0; dir < 4; ++dir) {
            best_delivery = min(best_delivery, basic_move_len(solver, solver.ball_cell(k), dir, solver.basket_cell(k)) + 2);
        }
        delivery[k] = best_delivery;
        vector<int> single{k};
        start_cost[k] = (int)solver.build_ops(single).size();
    }

    for (int from = 0; from < m; ++from) {
        for (int to = 0; to < m; ++to) {
            if (from == to) continue;
            int best = numeric_limits<int>::max();
            for (int dir = 0; dir < 4; ++dir) {
                best = min(best, basic_move_len(solver, solver.basket_cell(from), dir, solver.ball_cell(to)));
            }
            link[from][to] = best;
        }
    }

    auto edge_score = [&](int prev, int head, int tail, int mode) {
        int jitter = (prev * 119 + head * 31 + tail * 17 + mode * 13) % 17;
        if (mode == 0) return 100 * link[prev][head] + 45 * delivery[prev] + jitter;
        if (mode == 1) return 135 * link[prev][head] + 20 * delivery[prev] + jitter;
        if (mode == 2) return 75 * link[prev][head] + 85 * delivery[prev] + jitter;
        if (mode == 3) return 110 * link[prev][head] + 10 * start_cost[prev] + jitter;
        if (mode == 4) return 95 * link[prev][head] + 35 * delivery[prev] + 12 * start_cost[prev] + jitter;
        if (mode == 5) return 100 * link[prev][head] + jitter;
        if (mode == 6) return 100 * link[prev][head] + 8 * delivery[prev] + jitter;
        return 100 * link[prev][head] + 8 * start_cost[prev] + jitter;
    };

    vector<vector<int>> orders;
    orders.push_back(solver.build_greedy_order());

    for (int tail = 0; tail < m; ++tail) {
        for (int mode = 0; mode < 8; ++mode) {
            vector<char> used(m, 0);
            vector<int> order_rev;
            order_rev.reserve(m);
            order_rev.push_back(tail);
            used[tail] = 1;

            while ((int)order_rev.size() < m) {
                int head = order_rev.back();
                int best = -1;
                int best_score = numeric_limits<int>::max();
                for (int k = 0; k < m; ++k) {
                    if (used[k]) continue;
                    int score = edge_score(k, head, tail, mode);
                    if (score < best_score) {
                        best_score = score;
                        best = k;
                    }
                }
                if (best == -1) break;
                order_rev.push_back(best);
                used[best] = 1;
            }
            reverse(order_rev.begin(), order_rev.end());
            orders.push_back(std::move(order_rev));
        }
    }

    sort(orders.begin(), orders.end());
    orders.erase(unique(orders.begin(), orders.end()), orders.end());
    return orders;
}

vector<int> build_light_goal_reverse_order(const Solver &solver) {
    vector<vector<int>> orders = build_goal_reverse_orders(solver);
    if (orders.empty()) return solver.build_greedy_order();

    auto refine_by_local_moves = [&](vector<int> order, int current_score) {
        const int m = (int)order.size();
        auto move_one = [&](int from, int to) {
            if (from == to) return;
            int value = order[from];
            if (from < to) {
                for (int k = from; k < to; ++k) order[k] = order[k + 1];
            } else {
                for (int k = from; k > to; --k) order[k] = order[k - 1];
            }
            order[to] = value;
        };

        for (int pass = 0; pass < 2; ++pass) {
            int best_type = 0;
            int best_i = -1;
            int best_j = -1;
            int best_score = current_score;

            for (int i = 0; i < m; ++i) {
                for (int j = i + 1; j < m; ++j) {
                    swap(order[i], order[j]);
                    int score = raw_order_score(solver, order);
                    if (score < best_score) {
                        best_type = 1;
                        best_score = score;
                        best_i = i;
                        best_j = j;
                    }
                    swap(order[i], order[j]);
                }
            }

            for (int i = 0; i < m; ++i) {
                for (int j = 0; j < m; ++j) {
                    if (i == j) continue;
                    move_one(i, j);
                    int score = raw_order_score(solver, order);
                    if (score < best_score) {
                        best_type = 2;
                        best_score = score;
                        best_i = i;
                        best_j = j;
                    }
                    move_one(j, i);
                }
            }

            if (best_type == 0) break;
            if (best_type == 1) {
                swap(order[best_i], order[best_j]);
            } else {
                move_one(best_i, best_j);
            }
            current_score = best_score;
        }
        return pair<vector<int>, int>(order, current_score);
    };

    vector<pair<int, int>> scored;
    scored.reserve(orders.size());
    for (int i = 0; i < (int)orders.size(); ++i) {
        scored.push_back({raw_order_score(solver, orders[i]), i});
    }

    sort(scored.begin(), scored.end());
    const int refine_keep = min((int)scored.size(), 8);
    for (int rank = 0; rank < refine_keep; ++rank) {
        auto [refined, score] = refine_by_local_moves(orders[scored[rank].second], scored[rank].first);
        (void)score;
        orders.push_back(std::move(refined));
    }

    sort(orders.begin(), orders.end());
    orders.erase(unique(orders.begin(), orders.end()), orders.end());

    scored.clear();
    scored.reserve(orders.size());
    for (int i = 0; i < (int)orders.size(); ++i) {
        scored.push_back({raw_order_score(solver, orders[i]), i});
    }
    sort(scored.begin(), scored.end());

    return orders[scored[0].second];
}

int expanded_basic_count(const vector<char> &encoded) {
    int count = 0;
    vector<char> last_macro;
    vector<char> recording_macro;
    bool recording = false;
    bool has_macro = false;

    auto append_basic = [&](char op) {
        ++count;
        if (recording) recording_macro.push_back(op);
    };

    for (char op : encoded) {
        if (is_basic_op(op)) {
            append_basic(op);
        } else if (op == 'M') {
            if (recording) {
                last_macro = recording_macro;
                recording_macro.clear();
                recording = false;
                has_macro = true;
            } else {
                recording_macro.clear();
                recording = true;
            }
        } else if (op == 'P' && has_macro) {
            for (char basic : last_macro) append_basic(basic);
        }
    }

    return count;
}

struct PMacroPlacement {
    int leg = 0;
    int offset = 0;
    int len = 0;
    bool explicit_body = true;
    vector<char> body;
};

struct MacroBody {
    static constexpr int CAPACITY = 160;
    array<char, CAPACITY> data{};
    int n = 0;

    using iterator = char *;
    using const_iterator = const char *;

    int size() const { return n; }
    bool empty() const { return n == 0; }
    iterator begin() { return data.data(); }
    iterator end() { return data.data() + n; }
    const_iterator begin() const { return data.data(); }
    const_iterator end() const { return data.data() + n; }

    char &operator[](int idx) { return data[idx]; }
    const char &operator[](int idx) const { return data[idx]; }

    void clear() { n = 0; }

    void resize(int sz) {
        sz = max(0, min(sz, CAPACITY));
        while (n < sz) data[n++] = 'F';
        n = sz;
    }

    void push_back(char ch) {
        if (n < CAPACITY) data[n++] = ch;
    }

    iterator erase(iterator first, iterator last) {
        int l = max(0, min((int)(first - begin()), n));
        int r = max(l, min((int)(last - begin()), n));
        int removed = r - l;
        for (int i = r; i < n; ++i) data[i - removed] = data[i];
        n -= removed;
        return begin() + l;
    }

    iterator erase(iterator pos) {
        return erase(pos, pos + 1);
    }

    template <class It>
    iterator insert(iterator pos, It first, It last) {
        int p = max(0, min((int)(pos - begin()), n));
        array<char, CAPACITY> tmp{};
        int add = 0;
        for (; first != last && add < CAPACITY; ++first) tmp[add++] = *first;
        add = min(add, CAPACITY - n);
        if (add <= 0) return begin() + p;
        for (int i = n - 1; i >= p; --i) data[i + add] = data[i];
        for (int i = 0; i < add; ++i) data[p + i] = tmp[i];
        n += add;
        return begin() + p;
    }

    iterator insert(iterator pos, char ch) {
        int p = max(0, min((int)(pos - begin()), n));
        if (n >= CAPACITY) return begin() + p;
        for (int i = n - 1; i >= p; --i) data[i + 1] = data[i];
        data[p] = ch;
        ++n;
        return begin() + p;
    }

    template <class It>
    void assign(It first, It last) {
        n = 0;
        for (; first != last && n < CAPACITY; ++first) data[n++] = *first;
    }

    vector<char> to_vector() const {
        return vector<char>(begin(), end());
    }

    operator vector<char>() const {
        return to_vector();
    }

    bool operator==(const MacroBody &other) const {
        return n == other.n && equal(begin(), end(), other.begin());
    }
};

struct MacroSite {
    int ball = 0;
    int phase = 0;  // 0: 現在地から ball へ移動する leg, 1: ball から basket へ移動する leg
    int offset = 0;
    int len = 0;
    bool explicit_body = true;   // body-state 版では基本的に常に true。false は旧 slice 候補の一時状態のみ
    MacroBody body;              // 探索対象の expanded basic body
};

vector<int> build_pos_in_order(const vector<int> &order, int m) {
    vector<int> pos(m, -1);
    for (int i = 0; i < (int)order.size(); ++i) pos[order[i]] = i;
    return pos;
}

struct OrderContext {
    vector<int> pos;
    vector<int> basic_leg_len;
    vector<int> leg_start_cell;
    vector<int> leg_start_dir;
};

OrderContext build_order_context(const Solver &solver, const vector<int> &order) {
    OrderContext ctx;
    ctx.pos = build_pos_in_order(order, solver.ball_count());

    const int leg_count = (int)order.size() * 2;
    ctx.basic_leg_len.reserve(leg_count);
    ctx.leg_start_cell.reserve(leg_count);
    ctx.leg_start_dir.reserve(leg_count);

    int cell = 0;
    int dir = 1;
    for (int k : order) {
        ctx.leg_start_cell.push_back(cell);
        ctx.leg_start_dir.push_back(dir);
        Solver::RouteInfo to_ball = solver.basic_move_info(cell, dir, solver.ball_cell(k));
        ctx.basic_leg_len.push_back(to_ball.len);
        cell = to_ball.end_cell;
        dir = to_ball.end_dir;

        ctx.leg_start_cell.push_back(cell);
        ctx.leg_start_dir.push_back(dir);
        Solver::RouteInfo to_basket = solver.basic_move_info(cell, dir, solver.basket_cell(k));
        ctx.basic_leg_len.push_back(to_basket.len);
        cell = to_basket.end_cell;
        dir = to_basket.end_dir;
    }

    return ctx;
}

OrderContext build_order_context_reusing_prefix(const Solver &solver, const vector<int> &prev_order,
                                                const OrderContext &prev_ctx, const vector<int> &order) {
    OrderContext ctx;
    ctx.pos.assign(solver.ball_count(), -1);
    for (int i = 0; i < (int)order.size(); ++i) ctx.pos[order[i]] = i;

    const int leg_count = (int)order.size() * 2;
    ctx.basic_leg_len.reserve(leg_count);
    ctx.leg_start_cell.reserve(leg_count);
    ctx.leg_start_dir.reserve(leg_count);

    int prefix = 0;
    const int common_limit = min((int)prev_order.size(), (int)order.size());
    while (prefix < common_limit && prev_order[prefix] == order[prefix]) ++prefix;

    const int copied_legs = min(2 * prefix, leg_count);
    if (copied_legs > (int)prev_ctx.basic_leg_len.size() ||
        copied_legs > (int)prev_ctx.leg_start_cell.size() ||
        copied_legs > (int)prev_ctx.leg_start_dir.size()) {
        return build_order_context(solver, order);
    }
    ctx.basic_leg_len.insert(ctx.basic_leg_len.end(), prev_ctx.basic_leg_len.begin(),
                             prev_ctx.basic_leg_len.begin() + copied_legs);
    ctx.leg_start_cell.insert(ctx.leg_start_cell.end(), prev_ctx.leg_start_cell.begin(),
                              prev_ctx.leg_start_cell.begin() + copied_legs);
    ctx.leg_start_dir.insert(ctx.leg_start_dir.end(), prev_ctx.leg_start_dir.begin(),
                             prev_ctx.leg_start_dir.begin() + copied_legs);

    if (prefix >= (int)order.size()) return ctx;

    int cell = 0;
    int dir = 1;
    if (copied_legs > 0) {
        cell = prev_ctx.leg_start_cell[copied_legs];
        dir = prev_ctx.leg_start_dir[copied_legs];
    }

    for (int i = prefix; i < (int)order.size(); ++i) {
        int k = order[i];
        ctx.leg_start_cell.push_back(cell);
        ctx.leg_start_dir.push_back(dir);
        Solver::RouteInfo to_ball = solver.basic_move_info(cell, dir, solver.ball_cell(k));
        ctx.basic_leg_len.push_back(to_ball.len);
        cell = to_ball.end_cell;
        dir = to_ball.end_dir;

        ctx.leg_start_cell.push_back(cell);
        ctx.leg_start_dir.push_back(dir);
        Solver::RouteInfo to_basket = solver.basic_move_info(cell, dir, solver.basket_cell(k));
        ctx.basic_leg_len.push_back(to_basket.len);
        cell = to_basket.end_cell;
        dir = to_basket.end_dir;
    }

    return ctx;
}

void repair_macro_site_inplace(const Solver &solver, const vector<int> &order, const OrderContext &ctx, MacroSite &s) {
    const int m = solver.ball_count();
    if (m <= 0 || order.empty()) {
        s.ball = 0;
        s.phase = 0;
        s.offset = 0;
        s.len = 0;
        s.explicit_body = false;
        s.body.clear();
        return;
    }

    s.ball = max(0, min(s.ball, m - 1));
    s.phase = s.phase & 1;

    int p = ctx.pos[s.ball];
    if (p < 0) {
        s.offset = 0;
        s.len = 0;
        s.explicit_body = false;
        s.body.clear();
        return;
    }

    int leg = p * 2 + s.phase;
    if (leg < 0 || leg >= (int)ctx.basic_leg_len.size()) {
        s.offset = 0;
        s.len = 0;
        s.explicit_body = false;
        s.body.clear();
        return;
    }

    const int L = ctx.basic_leg_len[leg];

    if (s.explicit_body) {
        // explicit body は「どこで定義するか」だけ leg 上に置き、長さは body 側で管理する。
        s.offset = max(0, min(s.offset, L));
        s.body.erase(remove_if(s.body.begin(), s.body.end(), [](char ch) {
                         return ch != 'F' && ch != 'R' && ch != 'L';
                     }),
                     s.body.end());
        if ((int)s.body.size() < 6) {
            s.len = 0;
            s.explicit_body = false;
            s.body.clear();
            return;
        }
        if ((int)s.body.size() > 160) s.body.resize(160);
        s.len = (int)s.body.size();
        return;
    }

    if (L < 6) {
        s.offset = 0;
        s.len = 0;
        return;
    }

    s.offset = max(0, min(s.offset, L - 6));
    int max_len = min(160, L - s.offset);
    if (max_len < 6) {
        s.len = 0;
        return;
    }
    s.len = max(6, min(s.len, max_len));
    s.body.clear();
}

void normalize_macro_sites_inplace(const Solver &solver, const vector<int> &order, const OrderContext &ctx,
                                   vector<MacroSite> &sites, int max_defs) {
    for (MacroSite &s : sites) repair_macro_site_inplace(solver, order, ctx, s);
    sites.erase(remove_if(sites.begin(), sites.end(), [](const MacroSite &s) {
                    return s.len < 6;
                }),
                sites.end());

    sort(sites.begin(), sites.end(), [](const MacroSite &a, const MacroSite &b) {
        if (a.ball != b.ball) return a.ball < b.ball;
        if (a.phase != b.phase) return a.phase < b.phase;
        if (a.offset != b.offset) return a.offset < b.offset;
        return a.len > b.len;
    });
    sites.erase(unique(sites.begin(), sites.end(), [](const MacroSite &a, const MacroSite &b) {
                    return a.ball == b.ball && a.phase == b.phase;
                }),
                sites.end());

    while ((int)sites.size() > max_defs) {
        sites.erase(sites.begin() + rnd::get((int)sites.size()));
    }
}

// normalize_macro_sites_inplace() 済みの sites を PMacroPlacement に変換する軽量版。
// repair_macro_site_inplace() をここで再実行しないことで、評価時の重複計算を避ける。
vector<PMacroPlacement> convert_sites_to_plan_no_repair(const vector<int> &order,
                                                        const OrderContext &ctx,
                                                        const vector<MacroSite> &sites) {
    (void)order;
    vector<PMacroPlacement> plan;
    plan.reserve(sites.size());

    for (const MacroSite &s : sites) {
        if (s.len < 6) continue;
        if (s.phase < 0 || s.phase > 1) continue;
        if (s.ball < 0 || s.ball >= (int)ctx.pos.size()) continue;

        int p = ctx.pos[s.ball];
        if (p < 0) continue;
        PMacroPlacement pm;
        pm.leg = p * 2 + s.phase;
        pm.offset = s.offset;
        pm.len = s.len;
        pm.explicit_body = s.explicit_body;
        if (s.explicit_body) pm.body = s.body;
        plan.push_back(std::move(pm));
    }

    // order が変わると ball 順と leg 順は一致しないので、既存処理に渡す前に leg 順へ整える。
    sort(plan.begin(), plan.end(), [](const PMacroPlacement &lhs, const PMacroPlacement &rhs) {
        if (lhs.leg != rhs.leg) return lhs.leg < rhs.leg;
        if (lhs.offset != rhs.offset) return lhs.offset < rhs.offset;
        return lhs.len > rhs.len;
    });

    return plan;
}


vector<char> build_basic_leg_ops_for_site(const Solver &solver, const vector<int> &order, const OrderContext &ctx, const MacroSite &site) {
    vector<char> empty;
    if (site.ball < 0 || site.ball >= (int)ctx.pos.size()) return empty;
    int pos = ctx.pos[site.ball];
    if (pos < 0 || pos >= (int)order.size()) return empty;

    const int leg = pos * 2 + (site.phase & 1);
    if (leg < 0 || leg >= (int)ctx.leg_start_cell.size() || leg >= (int)ctx.leg_start_dir.size()) return empty;

    int end_cell = ctx.leg_start_cell[leg];
    int end_dir = ctx.leg_start_dir[leg];
    const int target = ((site.phase & 1) == 0) ? solver.ball_cell(site.ball) : solver.basket_cell(site.ball);
    return solver.basic_move_ops(end_cell, end_dir, target, end_cell, end_dir);
}

bool materialize_macro_site_body(const Solver &solver, const vector<int> &order, const OrderContext &ctx, MacroSite &site) {
    site.explicit_body = false;
    site.body.clear();
    repair_macro_site_inplace(solver, order, ctx, site);
    if (site.len < 6) return false;

    vector<char> leg_ops = build_basic_leg_ops_for_site(solver, order, ctx, site);
    if (site.offset < 0 || site.offset + site.len > (int)leg_ops.size()) return false;

    site.body.assign(leg_ops.begin() + site.offset, leg_ops.begin() + site.offset + site.len);
    if ((int)site.body.size() < 6) return false;
    site.explicit_body = true;
    site.len = (int)site.body.size();
    return true;
}


bool refresh_explicit_body_from_route(const Solver &solver, const vector<int> &order, const OrderContext &ctx, MacroSite &site) {
    // 既存の場所情報は維持しつつ、現在の basic route slice から body を作り直す。
    // explicit body が壊れたときの再接地、または slice 型から explicit 型への昇格に使う。
    int desired_len = site.explicit_body && !site.body.empty() ? (int)site.body.size() : site.len;
    site.explicit_body = false;
    site.body.clear();
    site.len = max(6, min(160, desired_len));
    return materialize_macro_site_body(solver, order, ctx, site);
}

bool splice_explicit_body_with_route(const Solver &solver, const vector<int> &order, const OrderContext &ctx, MacroSite &site) {
    if (!site.explicit_body || site.body.empty()) return refresh_explicit_body_from_route(solver, order, ctx, site);

    MacroSite slice = site;
    slice.explicit_body = false;
    slice.body.clear();
    slice.len = max(6, min(160, (int)site.body.size() + rnd::range(-12, 13)));
    slice.offset += rnd::range(-12, 13);
    if (!materialize_macro_site_body(solver, order, ctx, slice)) return false;

    if (rnd::get(100) < 40) {
        site.body = std::move(slice.body);
    } else {
        int a = rnd::get((int)site.body.size());
        int b = rnd::get((int)site.body.size());
        if (a > b) swap(a, b);
        if (a == b) b = min((int)site.body.size(), a + 1);

        int take = min((int)slice.body.size(), max(1, b - a));
        int from = rnd::get((int)slice.body.size() - take + 1);
        site.body.erase(site.body.begin() + a, site.body.begin() + b);
        site.body.insert(site.body.begin() + a, slice.body.begin() + from, slice.body.begin() + from + take);
        if ((int)site.body.size() > 160) site.body.resize(160);
    }

    if ((int)site.body.size() < 6) return false;
    site.explicit_body = true;
    site.len = (int)site.body.size();
    return true;
}

void mutate_explicit_body_inplace(MacroSite &site) {
    if (!site.explicit_body || site.body.empty()) return;

    int type = rnd::get(5);
    if (type == 0) {
        // 1 文字置換
        int i = rnd::get((int)site.body.size());
        static constexpr char ops[3] = {'F', 'R', 'L'};
        site.body[i] = ops[rnd::get(3)];
    } else if (type == 1) {
        // 挿入
        if ((int)site.body.size() < 160) {
            int i = rnd::get((int)site.body.size() + 1);
            static constexpr char ops[3] = {'F', 'R', 'L'};
            site.body.insert(site.body.begin() + i, ops[rnd::get(3)]);
        }
    } else if (type == 2) {
        // 削除
        if ((int)site.body.size() > 6) {
            int i = rnd::get((int)site.body.size());
            site.body.erase(site.body.begin() + i);
        }
    } else if (type == 3) {
        // 短い区間を反転
        int n = (int)site.body.size();
        if (n >= 2) {
            int l = rnd::get(n);
            int r = rnd::get(n);
            if (l > r) swap(l, r);
            if (l < r) reverse(site.body.begin() + l, site.body.begin() + r + 1);
        }
    } else {
        // 短い区間を複製
        int n = (int)site.body.size();
        if (n >= 2 && n < 150) {
            int len = 1 + rnd::get(min(8, n));
            int l = rnd::get(n - len + 1);
            int to = rnd::get(n + 1);
            vector<char> part(site.body.begin() + l, site.body.begin() + l + len);
            if ((int)site.body.size() + len > 160) part.resize(160 - (int)site.body.size());
            site.body.insert(site.body.begin() + to, part.begin(), part.end());
        }
    }
    site.len = (int)site.body.size();
}

void crossover_explicit_bodies(MacroSite &dst, const MacroSite &src) {
    if (!dst.explicit_body || !src.explicit_body || dst.body.empty() || src.body.empty()) return;

    int dn = (int)dst.body.size();
    int sn = (int)src.body.size();
    int dl = rnd::get(dn);
    int dr = rnd::get(dn);
    if (dl > dr) swap(dl, dr);
    if (dl == dr) dr = min(dn, dl + 1);

    int take = 1 + rnd::get(min(24, sn));
    int sl = rnd::get(sn - take + 1);

    dst.body.erase(dst.body.begin() + dl, dst.body.begin() + dr);
    dst.body.insert(dst.body.begin() + dl, src.body.begin() + sl, src.body.begin() + sl + take);
    if ((int)dst.body.size() > 160) dst.body.resize(160);
    if ((int)dst.body.size() < 6) {
        while ((int)dst.body.size() < 6) dst.body.push_back('F');
    }
    dst.len = (int)dst.body.size();
    dst.explicit_body = true;
}

void randomize_macro_site_location_inplace(const vector<int> &order, const OrderContext &ctx, MacroSite &site) {
    const int m = (int)order.size();
    if (m <= 0) return;

    for (int trial = 0; trial < 20; ++trial) {
        int leg = rnd::get(max(1, 2 * m));
        if (leg >= (int)ctx.basic_leg_len.size()) continue;
        int L = ctx.basic_leg_len[leg];
        site.ball = order[leg / 2];
        site.phase = leg & 1;
        site.offset = (L <= 0 ? 0 : rnd::get(L + 1));
        return;
    }

    int leg = rnd::get(max(1, 2 * m));
    site.ball = order[leg / 2];
    site.phase = leg & 1;
    site.offset = 0;
}

MacroSite random_macro_site(const Solver &solver, const vector<int> &order, const OrderContext &ctx) {
    static const vector<int> lens = {6, 8, 10, 12, 16, 20, 24, 32, 40, 56, 72, 96, 120};
    const int m = (int)order.size();

    for (int trial = 0; trial < 30; ++trial) {
        int leg = rnd::get(max(1, 2 * m));
        if (leg >= (int)ctx.basic_leg_len.size() || ctx.basic_leg_len[leg] < 6) continue;

        int L = ctx.basic_leg_len[leg];
        MacroSite s;
        s.ball = order[leg / 2];
        s.phase = leg & 1;

        if (rnd::get(100) < 80) {
            int len = lens[rnd::get((int)lens.size())];
            if (len > L) continue;
            s.len = len;
            s.offset = rnd::get(L - len + 1);
        } else {
            s.offset = rnd::get(L - 5);
            s.len = 6 + rnd::get(min(160, L - s.offset) - 5);
        }
        // body-state 版では、MacroSite は「場所」ではなく「定義する body」を主状態として持つ。
        // そのため、新規 site は必ず現在の route slice から explicit body として materialize する。
        MacroSite explicit_site = s;
        if (materialize_macro_site_body(solver, order, ctx, explicit_site)) return explicit_site;
    }

    MacroSite s;
    if (m > 0) s.ball = order[0];
    s.phase = 0;
    s.offset = 0;
    s.len = 0;
    return s;
}

MacroSite random_macro_site(const Solver &solver, const vector<int> &order) {
    OrderContext ctx = build_order_context(solver, order);
    return random_macro_site(solver, order, ctx);
}

template <size_t N>
int sample_weighted_type(const array<int, N> &weights) {
    int total = 0;
    for (int w : weights) total += w;
    if (total <= 0) return rnd::get((int)N);

    int x = rnd::get(total);
    for (int i = 0; i < (int)N; ++i) {
        if (x < weights[i]) return i;
        x -= weights[i];
    }
    return (int)N - 1;
}

void mutate_order_inplace(vector<int> &order, int type) {
    const int m = (int)order.size();
    if (m <= 1) return;

    if (type == 0) {
        int i = rnd::get(m);
        int j = rnd::get(m);
        if (i != j) swap(order[i], order[j]);
    } else if (type == 1) {
        int i = rnd::get(m);
        int j = rnd::get(m);
        if (i == j) return;
        int value = order[i];
        if (i < j) {
            for (int p = i; p < j; ++p) order[p] = order[p + 1];
        } else {
            for (int p = i; p > j; --p) order[p] = order[p - 1];
        }
        order[j] = value;
    } else if (type == 2) {
        int l = rnd::get(m);
        int r = rnd::get(m);
        if (l > r) swap(l, r);
        if (l < r) reverse(order.begin() + l, order.begin() + r + 1);
    } else if (type == 3) {
        int len = 1 + rnd::get(min(8, m));
        int from = rnd::get(m - len + 1);
        int to = rnd::get(m - len + 1);
        if (from == to) return;

        vector<int> block(order.begin() + from, order.begin() + from + len);
        order.erase(order.begin() + from, order.begin() + from + len);
        if (to > from) to -= len;
        to = max(0, min(to, (int)order.size()));
        order.insert(order.begin() + to, block.begin(), block.end());
    } else {
        int len1 = 1 + rnd::get(min(6, m));
        int a = rnd::get(m - len1 + 1);
        int len2 = 1 + rnd::get(min(6, m));
        int b = rnd::get(m - len2 + 1);
        if (a > b) {
            swap(a, b);
            swap(len1, len2);
        }
        if (a + len1 > b) return;
        vector<int> A(order.begin() + a, order.begin() + a + len1);
        vector<int> B(order.begin() + b, order.begin() + b + len2);
        order.erase(order.begin() + b, order.begin() + b + len2);
        order.erase(order.begin() + a, order.begin() + a + len1);
        order.insert(order.begin() + a, B.begin(), B.end());
        int nb = b - len1 + len2;
        order.insert(order.begin() + nb, A.begin(), A.end());
    }
}

vector<int> build_initial_order_fast(const Solver &solver) {
    vector<vector<int>> orders = build_goal_reverse_orders(solver);
    if (orders.empty()) return solver.build_greedy_order();

    int best_idx = 0;
    int best_score = raw_order_score(solver, orders[0]);
    for (int i = 1; i < (int)orders.size(); ++i) {
        int score = raw_order_score(solver, orders[i]);
        if (score < best_score) {
            best_score = score;
            best_idx = i;
        }
    }

    return orders[best_idx];
}

vector<char> expand_buttons_with_macro(const vector<char> &buttons, const vector<char> &macro) {
    vector<char> expanded;
    for (char op : buttons) {
        if (op == 'P') {
            expanded.insert(expanded.end(), macro.begin(), macro.end());
        } else if (is_basic_op(op)) {
            expanded.push_back(op);
        }
    }
    return expanded;
}

vector<char> encode_with_previous_macro(const vector<char> &target_basic, const vector<char> &macro) {
    const int n = (int)target_basic.size();
    const int m = (int)macro.size();
    if (m <= 1 || n < m) return target_basic;

    vector<int> dp(n + 1, 0), use_p(n, 0);
    dp[n] = 0;
    for (int i = n - 1; i >= 0; --i) {
        dp[i] = 1 + dp[i + 1];
        if (i + m <= n && equal(macro.begin(), macro.end(), target_basic.begin() + i)) {
            int cand = 1 + dp[i + m];
            if (cand < dp[i]) {
                dp[i] = cand;
                use_p[i] = 1;
            }
        }
    }

    vector<char> encoded;
    encoded.reserve(dp[0]);
    for (int i = 0; i < n;) {
        if (use_p[i]) {
            encoded.push_back('P');
            i += m;
        } else {
            encoded.push_back(target_basic[i]);
            ++i;
        }
    }
    return encoded;
}

vector<PMacroPlacement> normalize_p_macro_plan(vector<PMacroPlacement> plan, int leg_count) {
    sort(plan.begin(), plan.end(), [](const PMacroPlacement &lhs, const PMacroPlacement &rhs) {
        if (lhs.leg != rhs.leg) return lhs.leg < rhs.leg;
        if (lhs.offset != rhs.offset) return lhs.offset < rhs.offset;
        return lhs.len > rhs.len;
    });

    vector<PMacroPlacement> res;
    int last_leg = -1;
    for (PMacroPlacement p : plan) {
        p.leg = max(0, min(p.leg, leg_count - 1));
        p.offset = max(0, p.offset);
        p.len = max(6, p.len);
        if (p.leg == last_leg) continue;
        res.push_back(p);
        last_leg = p.leg;
    }
    return res;
}

vector<char> build_program_with_p_macro_plan(const Solver &solver, const vector<int> &order, vector<PMacroPlacement> plan) {
    const int leg_count = (int)order.size() * 2;
    plan = normalize_p_macro_plan(std::move(plan), leg_count);

    vector<char> program;
    vector<char> macro;
    vector<int> macro_transition;
    bool has_macro = false;
    int cell = 0;
    int dir = 1;
    int leg = 0;
    int plan_idx = 0;

    auto append_and_apply = [&](const vector<char> &buttons, const vector<char> &active_macro) {
        program.insert(program.end(), buttons.begin(), buttons.end());
        int state = cell * 4 + dir;
        for (char op : buttons) {
            if (op == 'P') {
                if (!active_macro.empty()) state = macro_transition[state];
            } else {
                state = solver.apply_basic_state(state, op);
            }
        }
        cell = state >> 2;
        dir = state & 3;
    };

    auto append_and_apply_slice = [&](const vector<char> &buttons, int l, int r, const vector<char> &active_macro) {
        program.insert(program.end(), buttons.begin() + l, buttons.begin() + r);
        int state = cell * 4 + dir;
        for (int i = l; i < r; ++i) {
            char op = buttons[i];
            if (op == 'P') {
                if (!active_macro.empty()) state = macro_transition[state];
            } else {
                state = solver.apply_basic_state(state, op);
            }
        }
        cell = state >> 2;
        dir = state & 3;
    };

    auto current_route = [&](int target) {
        int end_cell, end_dir;
        if (has_macro) {
            return solver.macro_move_ops_with_transition(cell, dir, target, macro_transition, end_cell, end_dir);
        }
        return solver.basic_move_ops(cell, dir, target, end_cell, end_dir);
    };

    auto move_to = [&](int target) {
        vector<char> route = current_route(target);

        bool define_here = false;
        PMacroPlacement def;
        while (plan_idx < (int)plan.size() && plan[plan_idx].leg < leg) ++plan_idx;
        if (plan_idx < (int)plan.size() && plan[plan_idx].leg == leg) {
            def = plan[plan_idx];
            if (def.explicit_body) {
                if (def.offset <= (int)route.size() && !def.body.empty()) define_here = true;
            } else if (def.offset + def.len <= (int)route.size()) {
                define_here = true;
            }
            ++plan_idx;
        }

        if (!define_here) {
            append_and_apply(route, macro);
            ++leg;
            return;
        }

        append_and_apply_slice(route, 0, def.offset, macro);

        vector<char> new_macro;
        if (def.explicit_body) {
            new_macro = def.body;
        } else {
            vector<char> raw_definition(route.begin() + def.offset, route.begin() + def.offset + def.len);
            new_macro = expand_buttons_with_macro(raw_definition, macro);
        }
        vector<char> definition_buttons = encode_with_previous_macro(new_macro, macro);

        program.push_back('M');
        program.insert(program.end(), definition_buttons.begin(), definition_buttons.end());
        program.push_back('M');
        {
            int state = cell * 4 + dir;
            for (char op : definition_buttons) {
                if (op == 'P') {
                    if (!macro.empty()) state = macro_transition[state];
                } else {
                    state = solver.apply_basic_state(state, op);
                }
            }
            cell = state >> 2;
            dir = state & 3;
        }

        vector<int> new_transition = solver.build_macro_transition_from_buttons(definition_buttons, macro_transition);
        macro = std::move(new_macro);
        macro_transition = std::move(new_transition);
        has_macro = true;

        vector<char> rest = current_route(target);
        append_and_apply(rest, macro);
        ++leg;
    };

    for (int k : order) {
        move_to(solver.ball_cell(k));
        program.push_back('S');
        move_to(solver.basket_cell(k));
        program.push_back('S');
    }

    return program;
}


static inline uint64_t splitmix64_u64(uint64_t x) {
    x += 0x9e3779b97f4a7c15ULL;
    x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
    x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
    return x ^ (x >> 31);
}

uint64_t hash_definition_buttons(uint64_t prev_hash, const vector<char> &buttons) {
    uint64_t h = splitmix64_u64(prev_hash ^ (uint64_t)buttons.size() * 0x9e3779b97f4a7c15ULL);
    for (char ch : buttons) {
        h = splitmix64_u64(h ^ (uint64_t)(unsigned char)ch);
    }
    return h;
}

struct RouteCacheKey {
    uint64_t macro_hash = 0;
    int start_state = 0;
    int target = 0;

    bool operator==(const RouteCacheKey &other) const {
        return macro_hash == other.macro_hash && start_state == other.start_state && target == other.target;
    }
};

struct RouteCacheKeyHash {
    size_t operator()(const RouteCacheKey &key) const {
        uint64_t h = splitmix64_u64(key.macro_hash);
        h ^= splitmix64_u64((uint64_t)key.start_state + 0x123456789abcdef0ULL);
        h ^= splitmix64_u64((uint64_t)key.target + 0xfedcba9876543210ULL);
        return (size_t)h;
    }
};

static inline uint64_t route_cache_hash_value(const RouteCacheKey &key) {
    uint64_t h = key.macro_hash;
    h ^= (uint64_t)(uint32_t)key.start_state * 0x9e3779b97f4a7c15ULL;
    h ^= (uint64_t)(uint32_t)key.target * 0xbf58476d1ce4e5b9ULL;
    h ^= h >> 30;
    h *= 0xbf58476d1ce4e5b9ULL;
    h ^= h >> 27;
    return h;
}

struct RouteInfoFlatCache {
    static constexpr int CAPACITY = 1 << 19;
    static constexpr int MASK = CAPACITY - 1;

    vector<RouteCacheKey> keys;
    vector<Solver::RouteInfo> values;
    vector<unsigned char> used;
    int count = 0;

    void reserve(size_t) {
        if (!keys.empty()) return;
        keys.resize(CAPACITY);
        values.resize(CAPACITY);
        used.assign(CAPACITY, 0);
        count = 0;
    }

    int size() const {
        return count;
    }

    void clear() {
        if (!used.empty()) fill(used.begin(), used.end(), 0);
        count = 0;
    }

    bool find(const RouteCacheKey &key, Solver::RouteInfo &value) const {
        if (used.empty()) return false;
        uint32_t slot = (uint32_t)route_cache_hash_value(key) & MASK;
        while (used[slot]) {
            if (keys[slot] == key) {
                value = values[slot];
                return true;
            }
            slot = (slot + 1) & MASK;
        }
        return false;
    }

    void emplace(const RouteCacheKey &key, const Solver::RouteInfo &value) {
        if (used.empty()) reserve(0);
        uint32_t slot = (uint32_t)route_cache_hash_value(key) & MASK;
        while (used[slot]) {
            if (keys[slot] == key) {
                values[slot] = value;
                return;
            }
            slot = (slot + 1) & MASK;
        }
        used[slot] = 1;
        keys[slot] = key;
        values[slot] = value;
        ++count;
    }
};

struct CachedRouteOps {
    vector<char> ops;
};

struct PlanEvalCache {
    RouteInfoFlatCache info_cache;
    unordered_map<RouteCacheKey, CachedRouteOps, RouteCacheKeyHash> ops_cache;
    int trim_counter = 0;

    void reserve_memory() {
        info_cache.reserve(300000);
        ops_cache.reserve(80000);
    }

    void trim_if_needed() {
        if (((++trim_counter) & 16383) != 0) return;

        if (info_cache.size() > 300000) {
            info_cache.clear();
        }
        if (ops_cache.size() > 80000) {
            ops_cache.clear();
        }
    }
};

struct MacroActiveTraceState {
    uint64_t macro_hash = 0;
    shared_ptr<const vector<char>> macro;
    shared_ptr<const vector<int>> macro_transition;
};

struct LegPrefixTraceState {
    int cell = 0;
    int dir = 1;
    int out_len = 0;
    int expanded_len = 0;
    int macro_state = 0;
};

struct MacroEvalTrace {
    bool valid = false;
    vector<LegPrefixTraceState> leg_start;
    vector<MacroActiveTraceState> macro_states;
};

struct SmallIntOrder {
    static constexpr int INLINE_CAP = 32;
    array<int, INLINE_CAP> small{};
    vector<int> large;
    int n = 0;
    bool large_mode = false;

    void reserve(int sz) {
        if (sz > INLINE_CAP) {
            large_mode = true;
            large.reserve(sz);
        }
    }

    void push_back(int value) {
        if (large_mode) {
            large.push_back(value);
            return;
        }
        if (n < INLINE_CAP) {
            small[n++] = value;
            return;
        }
        large_mode = true;
        large.assign(small.begin(), small.begin() + n);
        large.push_back(value);
    }

    bool empty() const {
        return size() == 0;
    }

    int size() const {
        return large_mode ? (int)large.size() : n;
    }

    int &operator[](int idx) {
        return large_mode ? large[idx] : small[idx];
    }

    const int &operator[](int idx) const {
        return large_mode ? large[idx] : small[idx];
    }

    void resize(int sz) {
        if (large_mode) {
            large.resize(sz);
        } else {
            n = max(0, min(sz, n));
        }
    }

    template <class Compare>
    void sort_by(Compare comp) {
        if (large_mode) {
            sort(large.begin(), large.end(), comp);
        } else {
            sort(small.begin(), small.begin() + n, comp);
        }
    }
};

int score_p_macro_sites_fast(const Solver &solver, const vector<int> &order, const OrderContext &ctx,
                             const vector<MacroSite> &sites, int limit,
                             int cutoff = numeric_limits<int>::max() / 4,
                             PlanEvalCache *cache = nullptr) {
    SmallIntOrder site_order;
    site_order.reserve(sites.size());
    for (int i = 0; i < (int)sites.size(); ++i) {
        const MacroSite &s = sites[i];
        if (s.len < 6) continue;
        if (s.phase < 0 || s.phase > 1) continue;
        if (s.ball < 0 || s.ball >= (int)ctx.pos.size()) continue;
        int p = ctx.pos[s.ball];
        if (p < 0) continue;
        site_order.push_back(i);
    }

    auto site_leg = [&](int idx) {
        const MacroSite &s = sites[idx];
        return ctx.pos[s.ball] * 2 + s.phase;
    };

    site_order.sort_by([&](int lhs_idx, int rhs_idx) {
        const MacroSite &lhs = sites[lhs_idx];
        const MacroSite &rhs = sites[rhs_idx];
        int lhs_leg = site_leg(lhs_idx);
        int rhs_leg = site_leg(rhs_idx);
        if (lhs_leg != rhs_leg) return lhs_leg < rhs_leg;
        if (lhs.offset != rhs.offset) return lhs.offset < rhs.offset;
        return lhs.len > rhs.len;
    });

    // normalize_p_macro_plan() と同じく、同じ leg の定義は最初の1つだけ使う。
    if (!site_order.empty()) {
        int write = 0;
        int last_leg = -1;
        for (int i = 0; i < site_order.size(); ++i) {
            int idx = site_order[i];
            int leg_id = site_leg(idx);
            if (leg_id == last_leg) continue;
            site_order[write++] = idx;
            last_leg = leg_id;
        }
        site_order.resize(write);
    }

    vector<char> macro;
    vector<int> macro_transition;
    bool has_macro = false;
    int cell = 0;
    int dir = 1;
    int leg = 0;
    int plan_idx = 0;
    int out_len = 0;
    int expanded_len = 0;

    auto current_score_value = [&]() {
        int score = out_len;
        if (out_len > limit) score += 1000000 + out_len - limit;
        if (expanded_len > limit) score += 1000 * (expanded_len - limit);
        return score;
    };

    auto exceeded_cutoff = [&]() {
        return current_score_value() > cutoff;
    };

    auto append_and_apply = [&](const vector<char> &buttons, const vector<char> &active_macro) {
        out_len += (int)buttons.size();
        int state = cell * 4 + dir;
        for (char op : buttons) {
            if (op == 'P') {
                expanded_len += (int)active_macro.size();
                if (!active_macro.empty()) state = macro_transition[state];
            } else {
                ++expanded_len;
                state = solver.apply_basic_state(state, op);
            }
        }
        cell = state >> 2;
        dir = state & 3;
    };

    auto append_and_apply_slice = [&](const vector<char> &buttons, int l, int r, const vector<char> &active_macro) {
        out_len += r - l;
        int state = cell * 4 + dir;
        for (int i = l; i < r; ++i) {
            char op = buttons[i];
            if (op == 'P') {
                expanded_len += (int)active_macro.size();
                if (!active_macro.empty()) state = macro_transition[state];
            } else {
                ++expanded_len;
                state = solver.apply_basic_state(state, op);
            }
        }
        cell = state >> 2;
        dir = state & 3;
    };

    uint64_t macro_hash = 0;

    auto current_route = [&](int target) {
        int end_cell, end_dir;
        if (!has_macro) {
            return solver.basic_move_ops(cell, dir, target, end_cell, end_dir);
        }

        RouteCacheKey key{macro_hash, cell * 4 + dir, target};
        if (cache) {
            auto it = cache->ops_cache.find(key);
            if (it != cache->ops_cache.end()) {
                return it->second.ops;
            }
        }

        vector<char> ops = solver.macro_move_ops_with_transition(cell, dir, target, macro_transition, end_cell, end_dir);
        if (cache) {
            cache->ops_cache.emplace(key, CachedRouteOps{ops});
            cache->trim_if_needed();
        }
        return ops;
    };

    auto current_route_info = [&](int target) {
        if (!has_macro) {
            return solver.basic_move_info(cell, dir, target);
        }

        RouteCacheKey key{macro_hash, cell * 4 + dir, target};
        if (cache) {
            Solver::RouteInfo cached_info;
            if (cache->info_cache.find(key, cached_info)) {
                return cached_info;
            }
        }

        Solver::RouteInfo info = solver.macro_move_info_with_transition(cell, dir, target, macro_transition);
        if (cache) {
            cache->info_cache.emplace(key, info);
            cache->trim_if_needed();
        }
        return info;
    };

    auto append_info = [&](const Solver::RouteInfo &info) {
        out_len += info.len;
        expanded_len += info.len + info.p_count * ((int)macro.size() - 1);
        cell = info.end_cell;
        dir = info.end_dir;
    };

    auto move_to = [&](int target) {
        const MacroSite *def = nullptr;
        while (plan_idx < (int)site_order.size() && site_leg(site_order[plan_idx]) < leg) ++plan_idx;
        if (plan_idx < (int)site_order.size() && site_leg(site_order[plan_idx]) == leg) {
            def = &sites[site_order[plan_idx]];
            ++plan_idx;
        }

        if (def == nullptr) {
            append_info(current_route_info(target));
            ++leg;
            return;
        }

        vector<char> route = current_route(target);
        bool define_here = false;
        if (def->explicit_body) {
            define_here = (def->offset <= (int)route.size() && !def->body.empty());
        } else {
            define_here = (def->offset + def->len <= (int)route.size());
        }
        if (!define_here) {
            append_and_apply(route, macro);
            ++leg;
            return;
        }

        append_and_apply_slice(route, 0, def->offset, macro);

        vector<char> new_macro;
        if (def->explicit_body) {
            new_macro = def->body;
        } else {
            vector<char> raw_definition(route.begin() + def->offset, route.begin() + def->offset + def->len);
            new_macro = expand_buttons_with_macro(raw_definition, macro);
        }
        vector<char> definition_buttons = encode_with_previous_macro(new_macro, macro);

        out_len += 2;
        append_and_apply(definition_buttons, macro);

        vector<int> new_transition = solver.build_macro_transition_from_buttons(definition_buttons, macro_transition);
        macro_hash = hash_definition_buttons(macro_hash, definition_buttons);
        macro = std::move(new_macro);
        macro_transition = std::move(new_transition);
        has_macro = true;

        append_info(current_route_info(target));
        ++leg;
    };

    for (int k : order) {
        move_to(solver.ball_cell(k));
        if (exceeded_cutoff()) return cutoff + 1;
        ++out_len;
        ++expanded_len;
        if (exceeded_cutoff()) return cutoff + 1;
        move_to(solver.basket_cell(k));
        if (exceeded_cutoff()) return cutoff + 1;
        ++out_len;
        ++expanded_len;
        if (exceeded_cutoff()) return cutoff + 1;
    }

    return current_score_value();
}

int macro_site_leg_id(const OrderContext &ctx, const MacroSite &s) {
    if (s.len < 6) return numeric_limits<int>::max() / 4;
    if (s.phase < 0 || s.phase > 1) return numeric_limits<int>::max() / 4;
    if (s.ball < 0 || s.ball >= (int)ctx.pos.size()) return numeric_limits<int>::max() / 4;
    int p = ctx.pos[s.ball];
    if (p < 0) return numeric_limits<int>::max() / 4;
    return p * 2 + s.phase;
}

bool same_macro_site_definition(const MacroSite &a, const MacroSite &b) {
    return a.ball == b.ball &&
           a.phase == b.phase &&
           a.offset == b.offset &&
           a.len == b.len &&
           a.explicit_body == b.explicit_body &&
           a.body == b.body;
}

int earliest_changed_macro_leg(const OrderContext &ctx, const vector<MacroSite> &lhs,
                               const vector<MacroSite> &rhs, int leg_count) {
    auto key_less = [](const MacroSite &a, const MacroSite &b) {
        if (a.ball != b.ball) return a.ball < b.ball;
        return a.phase < b.phase;
    };
    auto key_equal = [](const MacroSite &a, const MacroSite &b) {
        return a.ball == b.ball && a.phase == b.phase;
    };

    int dirty = leg_count;
    int i = 0;
    int j = 0;
    while (i < (int)lhs.size() || j < (int)rhs.size()) {
        if (i == (int)lhs.size()) {
            dirty = min(dirty, macro_site_leg_id(ctx, rhs[j]));
            ++j;
            continue;
        }
        if (j == (int)rhs.size()) {
            dirty = min(dirty, macro_site_leg_id(ctx, lhs[i]));
            ++i;
            continue;
        }
        if (key_equal(lhs[i], rhs[j])) {
            if (!same_macro_site_definition(lhs[i], rhs[j])) {
                dirty = min(dirty, macro_site_leg_id(ctx, lhs[i]));
            }
            ++i;
            ++j;
        } else if (key_less(lhs[i], rhs[j])) {
            dirty = min(dirty, macro_site_leg_id(ctx, lhs[i]));
            ++i;
        } else {
            dirty = min(dirty, macro_site_leg_id(ctx, rhs[j]));
            ++j;
        }
    }

    if (dirty >= leg_count) {
        return leg_count;
    }
    return dirty;
}

int score_p_macro_sites_fast_prefix(const Solver &solver, const vector<int> &order, const OrderContext &ctx,
                                    const vector<MacroSite> &sites, int limit,
                                    int cutoff, PlanEvalCache *cache,
                                    const MacroEvalTrace *prefix_trace, int start_leg,
                                    MacroEvalTrace *out_trace = nullptr) {
    const int leg_count = (int)order.size() * 2;
    start_leg = max(0, min(start_leg, leg_count));

    SmallIntOrder site_order;
    site_order.reserve(sites.size());
    for (int i = 0; i < (int)sites.size(); ++i) {
        const MacroSite &s = sites[i];
        int leg_id = macro_site_leg_id(ctx, s);
        if (leg_id >= leg_count) continue;
        site_order.push_back(i);
    }

    auto site_leg = [&](int idx) {
        return macro_site_leg_id(ctx, sites[idx]);
    };

    site_order.sort_by([&](int lhs_idx, int rhs_idx) {
        const MacroSite &lhs = sites[lhs_idx];
        const MacroSite &rhs = sites[rhs_idx];
        int lhs_leg = site_leg(lhs_idx);
        int rhs_leg = site_leg(rhs_idx);
        if (lhs_leg != rhs_leg) return lhs_leg < rhs_leg;
        if (lhs.offset != rhs.offset) return lhs.offset < rhs.offset;
        return lhs.len > rhs.len;
    });

    if (!site_order.empty()) {
        int write = 0;
        int last_leg = -1;
        for (int i = 0; i < site_order.size(); ++i) {
            int idx = site_order[i];
            int leg_id = site_leg(idx);
            if (leg_id == last_leg) continue;
            site_order[write++] = idx;
            last_leg = leg_id;
        }
        site_order.resize(write);
    }

    vector<char> owned_macro;
    vector<int> owned_macro_transition;
    const vector<char> *macro = &owned_macro;
    const vector<int> *macro_transition = &owned_macro_transition;
    bool has_macro = false;
    uint64_t macro_hash = 0;
    int macro_state_idx = 0;
    int cell = 0;
    int dir = 1;
    int leg = start_leg;
    int plan_idx = 0;
    int out_len = 0;
    int expanded_len = 0;

    const bool can_use_prefix = prefix_trace != nullptr &&
                                prefix_trace->valid &&
                                start_leg > 0 &&
                                start_leg < (int)prefix_trace->leg_start.size();
    if (can_use_prefix) {
        const LegPrefixTraceState &pref = prefix_trace->leg_start[start_leg];
        cell = pref.cell;
        dir = pref.dir;
        out_len = pref.out_len;
        expanded_len = pref.expanded_len;
        macro_state_idx = pref.macro_state;
        if (0 <= macro_state_idx && macro_state_idx < (int)prefix_trace->macro_states.size()) {
            const MacroActiveTraceState &ms = prefix_trace->macro_states[macro_state_idx];
            if (ms.macro && ms.macro_transition) {
                macro_hash = ms.macro_hash;
                macro = ms.macro.get();
                macro_transition = ms.macro_transition.get();
                has_macro = !macro_transition->empty();
            } else {
                macro_state_idx = 0;
            }
        } else {
            macro_state_idx = 0;
        }
    } else {
        leg = 0;
        start_leg = 0;
    }

    if (out_trace) {
        out_trace->valid = false;
        out_trace->leg_start.clear();
        out_trace->macro_states.clear();

        if (can_use_prefix) {
            out_trace->macro_states = prefix_trace->macro_states;
            out_trace->leg_start.assign(prefix_trace->leg_start.begin(),
                                        prefix_trace->leg_start.begin() + start_leg + 1);
        } else {
            MacroActiveTraceState empty_state;
            out_trace->macro_states.push_back(std::move(empty_state));
        }
    }

    auto current_score_value = [&]() {
        int score = out_len;
        if (out_len > limit) score += 1000000 + out_len - limit;
        if (expanded_len > limit) score += 1000 * (expanded_len - limit);
        return score;
    };

    auto exceeded_cutoff = [&]() {
        return current_score_value() > cutoff;
    };

    auto record_leg_start = [&](int target_leg) {
        if (!out_trace) return;
        if ((int)out_trace->leg_start.size() == target_leg) {
            out_trace->leg_start.push_back(LegPrefixTraceState{cell, dir, out_len, expanded_len, macro_state_idx});
        } else if (target_leg < (int)out_trace->leg_start.size()) {
            out_trace->leg_start[target_leg] = LegPrefixTraceState{cell, dir, out_len, expanded_len, macro_state_idx};
        }
    };

    auto append_and_apply = [&](const vector<char> &buttons, const vector<char> &active_macro,
                                const vector<int> &active_macro_transition) {
        out_len += (int)buttons.size();
        int state = cell * 4 + dir;
        for (char op : buttons) {
            if (op == 'P') {
                expanded_len += (int)active_macro.size();
                if (!active_macro.empty()) state = active_macro_transition[state];
            } else {
                ++expanded_len;
                state = solver.apply_basic_state(state, op);
            }
        }
        cell = state >> 2;
        dir = state & 3;
    };

    auto append_and_apply_slice = [&](const vector<char> &buttons, int l, int r,
                                      const vector<char> &active_macro,
                                      const vector<int> &active_macro_transition) {
        out_len += r - l;
        int state = cell * 4 + dir;
        for (int i = l; i < r; ++i) {
            char op = buttons[i];
            if (op == 'P') {
                expanded_len += (int)active_macro.size();
                if (!active_macro.empty()) state = active_macro_transition[state];
            } else {
                ++expanded_len;
                state = solver.apply_basic_state(state, op);
            }
        }
        cell = state >> 2;
        dir = state & 3;
    };

    auto current_route = [&](int target) {
        int end_cell, end_dir;
        if (!has_macro) {
            return solver.basic_move_ops(cell, dir, target, end_cell, end_dir);
        }

        RouteCacheKey key{macro_hash, cell * 4 + dir, target};
        if (cache) {
            auto it = cache->ops_cache.find(key);
            if (it != cache->ops_cache.end()) {
                return it->second.ops;
            }
        }

        vector<char> ops = solver.macro_move_ops_with_transition(cell, dir, target, *macro_transition, end_cell, end_dir);
        if (cache) {
            cache->ops_cache.emplace(key, CachedRouteOps{ops});
            cache->trim_if_needed();
        }
        return ops;
    };

    auto current_route_info = [&](int target) {
        if (!has_macro) {
            return solver.basic_move_info(cell, dir, target);
        }

        RouteCacheKey key{macro_hash, cell * 4 + dir, target};
        if (cache) {
            Solver::RouteInfo cached_info;
            if (cache->info_cache.find(key, cached_info)) {
                return cached_info;
            }
        }

        Solver::RouteInfo info = solver.macro_move_info_with_transition(cell, dir, target, *macro_transition);
        if (cache) {
            cache->info_cache.emplace(key, info);
            cache->trim_if_needed();
        }
        return info;
    };

    auto append_info = [&](const Solver::RouteInfo &info) {
        out_len += info.len;
        expanded_len += info.len + info.p_count * ((int)macro->size() - 1);
        cell = info.end_cell;
        dir = info.end_dir;
    };

    auto move_to = [&](int target) {
        const MacroSite *def = nullptr;
        while (plan_idx < (int)site_order.size() && site_leg(site_order[plan_idx]) < leg) ++plan_idx;
        if (plan_idx < (int)site_order.size() && site_leg(site_order[plan_idx]) == leg) {
            def = &sites[site_order[plan_idx]];
            ++plan_idx;
        }

        if (def == nullptr) {
            append_info(current_route_info(target));
            ++leg;
            return;
        }

        vector<char> route = current_route(target);
        bool define_here = false;
        if (def->explicit_body) {
            define_here = (def->offset <= (int)route.size() && !def->body.empty());
        } else {
            define_here = (def->offset + def->len <= (int)route.size());
        }
        if (!define_here) {
            append_and_apply(route, *macro, *macro_transition);
            ++leg;
            return;
        }

        append_and_apply_slice(route, 0, def->offset, *macro, *macro_transition);

        vector<char> new_macro;
        if (def->explicit_body) {
            new_macro = def->body;
        } else {
            vector<char> raw_definition(route.begin() + def->offset, route.begin() + def->offset + def->len);
            new_macro = expand_buttons_with_macro(raw_definition, *macro);
        }
        vector<char> definition_buttons = encode_with_previous_macro(new_macro, *macro);

        out_len += 2;
        append_and_apply(definition_buttons, *macro, *macro_transition);

        vector<int> new_transition = solver.build_macro_transition_from_buttons(definition_buttons, *macro_transition);
        macro_hash = hash_definition_buttons(macro_hash, definition_buttons);
        if (out_trace) {
            shared_ptr<const vector<char>> macro_owner = make_shared<vector<char>>(std::move(new_macro));
            shared_ptr<const vector<int>> transition_owner = make_shared<vector<int>>(std::move(new_transition));
            macro = macro_owner.get();
            macro_transition = transition_owner.get();
            out_trace->macro_states.push_back(
                MacroActiveTraceState{macro_hash, std::move(macro_owner), std::move(transition_owner)});
            macro_state_idx = (int)out_trace->macro_states.size() - 1;
        } else {
            owned_macro = std::move(new_macro);
            owned_macro_transition = std::move(new_transition);
            macro = &owned_macro;
            macro_transition = &owned_macro_transition;
        }
        has_macro = true;

        append_info(current_route_info(target));
        ++leg;
    };

    while (plan_idx < (int)site_order.size() && site_leg(site_order[plan_idx]) < leg) ++plan_idx;

    for (; leg < leg_count;) {
        record_leg_start(leg);
        int ball = order[leg / 2];
        int target = (leg & 1) == 0 ? solver.ball_cell(ball) : solver.basket_cell(ball);
        move_to(target);
        if (exceeded_cutoff()) return cutoff + 1;
        ++out_len;
        ++expanded_len;
        if (exceeded_cutoff()) return cutoff + 1;
    }

    record_leg_start(leg_count);
    if (out_trace) out_trace->valid = ((int)out_trace->leg_start.size() == leg_count + 1);
    return current_score_value();
}

vector<char> anneal_order_and_p_macro_reroute(const Solver &solver, const vector<int> &initial_order, int limit,
                                              double time_limit_sec, int seed_rounds = 3,
                                              int seed_trials = 32, int max_defs = 10) {
    if (time_limit_sec <= 0.0) return solver.build_ops(initial_order);

    const double start_time = get_time();
    if (initial_order.empty()) return solver.build_ops(initial_order);

    PlanEvalCache eval_cache;
    eval_cache.reserve_memory();

    auto eval = [&](const vector<int> &ord, const OrderContext &ctx, const vector<MacroSite> &sites,
                    int cutoff = numeric_limits<int>::max() / 4) {
        return score_p_macro_sites_fast(solver, ord, ctx, sites, limit, cutoff, &eval_cache);
    };
    auto eval_trace = [&](const vector<int> &ord, const OrderContext &ctx, const vector<MacroSite> &sites,
                          const MacroEvalTrace *prefix, int start_leg, MacroEvalTrace *trace_out,
                          int cutoff = numeric_limits<int>::max() / 4) {
        return score_p_macro_sites_fast_prefix(solver, ord, ctx, sites, limit, cutoff, &eval_cache,
                                               prefix, start_leg, trace_out);
    };

    auto build_answer = [&](const vector<int> &ord, const OrderContext &ctx,
                            const vector<MacroSite> &sites) {
        vector<PMacroPlacement> plan = convert_sites_to_plan_no_repair(ord, ctx, sites);
        return build_program_with_p_macro_plan(solver, ord, plan);
    };

    vector<int> current_order = initial_order;
    OrderContext current_ctx = build_order_context(solver, current_order);
    vector<MacroSite> current_sites;
    int current_score = eval(current_order, current_ctx, current_sites);

    // まずは現在の order に対して、良いマクロ定義を少しだけ貪欲に足す。
    // current_ctx を使い回して、pos / leg 長の再計算を避ける。
    for (int round = 0; round < seed_rounds; ++round) {
        if (get_time() - start_time >= time_limit_sec) break;

        vector<MacroSite> best_add_sites = current_sites;
        int best_add_score = current_score;

        for (int trial = 0; trial < seed_trials; ++trial) {
            if (get_time() - start_time >= time_limit_sec) break;

            vector<MacroSite> next_sites = current_sites;
            MacroSite s = random_macro_site(solver, current_order, current_ctx);
            if (s.len < 6) continue;
            next_sites.push_back(s);
            normalize_macro_sites_inplace(solver, current_order, current_ctx, next_sites, max_defs);

            int score = eval(current_order, current_ctx, next_sites, best_add_score - 1);
            if (score < best_add_score) {
                best_add_score = score;
                best_add_sites = std::move(next_sites);
            }
        }

        if (best_add_score < current_score) {
            current_sites = std::move(best_add_sites);
            current_score = best_add_score;
        }
    }

    MacroEvalTrace current_trace;
    current_score = eval_trace(current_order, current_ctx, current_sites, nullptr, 0, &current_trace);

    vector<int> best_order = current_order;
    OrderContext best_ctx = current_ctx;
    vector<MacroSite> best_sites = current_sites;
    int best_score = current_score;

    static double log_table[65536];
    static bool initialized = false;
    if (!initialized) {
        for (int i = 0; i < 65536; ++i) log_table[i] = log((i + 0.5) / 65536.0);
        rnd::shuffle(log_table);
        initialized = true;
    }

    const double T0 = 120.0;
    const double T1 = 0.05;
    double heat = T0;
    int iter = 0;

    while (true) {
        double progress = (get_time() - start_time) / time_limit_sec;
        if (progress >= 1.0) break;
        if ((iter & 3) == 0) heat = T0 * pow(T1 / T0, progress);
        ++iter;

        vector<MacroSite> next_sites = current_sites;
        vector<int> next_order_storage;
        OrderContext next_ctx_storage;
        const vector<int> *next_order = &current_order;
        const OrderContext *next_ctx = &current_ctx;
        bool touched_order = false;

        static constexpr array<int, 8> ORDER_TYPE_WEIGHT = {
            10,  // 0: swap
            12,  // 1: insert
            8,   // 2: reverse
            18,  // 3: block insert
            26,  // 4: block swap
            6,   // 5: swap + site削除の可能性あり
            14,  // 6: insert + macro追加の可能性あり
            6,   // 7: reverse + site間引き
        };
        static constexpr array<int, 10> MACRO_TYPE_WEIGHT = {
            14,  // 0: body macro 追加
            5,   // 1: macro削除
            9,   // 2: body macro 置換
            12,  // 3: 定義場所だけ移動し、body は保持
            16,  // 4: 現在 route slice と body を splice
            18,  // 5: body の局所編集
            8,   // 6: 2つの body を crossover
            12,  // 7: shuffle + 削除/追加
            14,  // 8: 現在 route slice で body を再生成
            10,  // 9: 定義場所をランダム leg へジャンプ
        };

        int order_type = -1;
        int macro_type = -1;
        const bool choose_macro = rnd::get(2) == 0;

        if (choose_macro) {
            macro_type = sample_weighted_type(MACRO_TYPE_WEIGHT);

            if (macro_type == 0 || next_sites.empty()) {
                if ((int)next_sites.size() < max_defs) {
                    MacroSite s = random_macro_site(solver, *next_order, *next_ctx);
                    if (s.len >= 6 && s.explicit_body) next_sites.push_back(std::move(s));
                }
            } else if (macro_type == 1) {
                int idx = rnd::get((int)next_sites.size());
                next_sites.erase(next_sites.begin() + idx);
            } else if (macro_type == 2) {
                int idx = rnd::get((int)next_sites.size());
                MacroSite s = random_macro_site(solver, *next_order, *next_ctx);
                if (s.len >= 6 && s.explicit_body) next_sites[idx] = std::move(s);
            } else if (macro_type == 3) {
                // body は保持したまま、定義する ball/phase と offset だけを近傍移動する。
                int idx = rnd::get((int)next_sites.size());
                if (rnd::get(100) < 50) next_sites[idx].ball = (*next_order)[rnd::get((int)next_order->size())];
                if (rnd::get(100) < 50) next_sites[idx].phase ^= 1;
                next_sites[idx].offset += rnd::range(-24, 25);
                next_sites[idx].explicit_body = true;
            } else if (macro_type == 4) {
                // 現在 route の slice と既存 body を混ぜる。body を探索状態として育てる主近傍。
                int idx = rnd::get((int)next_sites.size());
                if (!splice_explicit_body_with_route(solver, *next_order, *next_ctx, next_sites[idx])) {
                    refresh_explicit_body_from_route(solver, *next_order, *next_ctx, next_sites[idx]);
                }
            } else if (macro_type == 5) {
                // route から離れた純粋な body 編集。
                int idx = rnd::get((int)next_sites.size());
                if (!next_sites[idx].explicit_body || next_sites[idx].body.empty()) {
                    refresh_explicit_body_from_route(solver, *next_order, *next_ctx, next_sites[idx]);
                } else {
                    int repeat = 1 + (rnd::get(100) < 20);
                    for (int rep = 0; rep < repeat; ++rep) mutate_explicit_body_inplace(next_sites[idx]);
                }
            } else if (macro_type == 6) {
                // 複数 body の交叉。MacroSite の場所ではなく body 自体を組み替える。
                if ((int)next_sites.size() >= 2) {
                    int a = rnd::get((int)next_sites.size());
                    int b = rnd::get((int)next_sites.size());
                    if (a != b) crossover_explicit_bodies(next_sites[a], next_sites[b]);
                } else {
                    MacroSite s = random_macro_site(solver, *next_order, *next_ctx);
                    if (s.len >= 6 && s.explicit_body && (int)next_sites.size() < max_defs) next_sites.push_back(std::move(s));
                }
            } else if (macro_type == 7) {
                rnd::shuffle(next_sites);
                while ((int)next_sites.size() > 1 && rnd::get(100) < 30) {
                    next_sites.erase(next_sites.begin() + rnd::get((int)next_sites.size()));
                }
                while ((int)next_sites.size() < max_defs && rnd::get(100) < 50) {
                    MacroSite s = random_macro_site(solver, *next_order, *next_ctx);
                    if (s.len >= 6 && s.explicit_body) next_sites.push_back(std::move(s));
                }
            } else if (macro_type == 8) {
                // 同じ場所の現在 route slice で body を完全に再生成する。
                int idx = rnd::get((int)next_sites.size());
                if (!refresh_explicit_body_from_route(solver, *next_order, *next_ctx, next_sites[idx])) {
                    MacroSite s = random_macro_site(solver, *next_order, *next_ctx);
                    if (s.len >= 6 && s.explicit_body) next_sites[idx] = std::move(s);
                }
            } else {
                // body は保持し、定義場所をランダムな leg へ大きく移動する。
                int idx = rnd::get((int)next_sites.size());
                randomize_macro_site_location_inplace(*next_order, *next_ctx, next_sites[idx]);
                next_sites[idx].explicit_body = true;
            }
        } else {
            order_type = sample_weighted_type(ORDER_TYPE_WEIGHT);
            touched_order = true;
            next_order_storage = current_order;

            if (order_type == 0) {
                mutate_order_inplace(next_order_storage, 0);  // swap
            } else if (order_type == 1) {
                mutate_order_inplace(next_order_storage, 1);  // insert
            } else if (order_type == 2) {
                mutate_order_inplace(next_order_storage, 2);  // reverse
            } else if (order_type == 3) {
                mutate_order_inplace(next_order_storage, 3);  // block insert
            } else if (order_type == 4) {
                mutate_order_inplace(next_order_storage, 4);  // block swap
            } else if (order_type == 5) {
                mutate_order_inplace(next_order_storage, 0);
                if (!next_sites.empty() && rnd::get(100) < 35) {
                    next_sites.erase(next_sites.begin() + rnd::get((int)next_sites.size()));
                }
            } else if (order_type == 6) {
                mutate_order_inplace(next_order_storage, 1);
            } else {
                mutate_order_inplace(next_order_storage, 2);
                // 大きめの order 変更時は、古い offset が合わないことが多いので少し間引く。
                while (!next_sites.empty() && rnd::get(100) < 30) {
                    next_sites.erase(next_sites.begin() + rnd::get((int)next_sites.size()));
                }
            }

            next_ctx_storage = build_order_context_reusing_prefix(solver, current_order, current_ctx, next_order_storage);
            next_order = &next_order_storage;
            next_ctx = &next_ctx_storage;

            if (order_type == 6 && (int)next_sites.size() < max_defs) {
                MacroSite s = random_macro_site(solver, *next_order, *next_ctx);
                if (s.len >= 6) next_sites.push_back(s);
            }
        }

        normalize_macro_sites_inplace(solver, *next_order, *next_ctx, next_sites, max_defs);

        double accept_margin = -heat * log_table[iter & 65535];
        int accept_cutoff = current_score + (int)floor(accept_margin);
        int dirty_leg = 0;
        bool tried_prefix_eval = false;
        bool can_reuse_current_trace = false;
        const int leg_count = (int)current_order.size() * 2;
        if (!touched_order && current_trace.valid && next_order == &current_order && next_ctx == &current_ctx) {
            dirty_leg = earliest_changed_macro_leg(current_ctx, current_sites, next_sites, leg_count);
            tried_prefix_eval = true;
            can_reuse_current_trace = true;
        }

        int next_score;
        if (tried_prefix_eval) {
            if (dirty_leg >= leg_count) {
                next_score = current_score;
            } else {
                next_score = eval_trace(current_order, current_ctx, next_sites,
                                        &current_trace, dirty_leg, nullptr, accept_cutoff);
            }
        } else {
            next_score = eval(*next_order, *next_ctx, next_sites, accept_cutoff);
        }
        int delta = next_score - current_score;

        if (delta <= accept_margin) {
            if (touched_order) {
                current_order = std::move(next_order_storage);
                current_ctx = std::move(next_ctx_storage);
                current_trace.valid = false;
            } else {
                if (dirty_leg < leg_count) {
                    MacroEvalTrace next_trace;
                    if (can_reuse_current_trace) {
                        eval_trace(current_order, current_ctx, next_sites,
                                   &current_trace, dirty_leg, &next_trace);
                    } else {
                        eval_trace(current_order, current_ctx, next_sites,
                                   nullptr, 0, &next_trace);
                    }
                    current_trace = std::move(next_trace);
                }
            }
            current_sites = std::move(next_sites);
            current_score = next_score;

            if (current_score < best_score) {
                best_score = current_score;
                best_order = current_order;
                best_ctx = current_ctx;
                best_sites = current_sites;
            }
        }
    }

    return build_answer(best_order, best_ctx, best_sites);
}

bool valid_program_for_limit(const vector<char> &program, int limit) {
    if (program.empty()) return false;
    if ((int)program.size() > limit) return false;
    if (expanded_basic_count(program) > limit) return false;
    return true;
}

vector<char> solve_with_p_macro_reroute(const Solver &solver, const vector<int> &order, int limit, double time_limit_sec) {
    vector<char> raw = solver.build_ops(order);
    vector<char> answer = anneal_order_and_p_macro_reroute(solver, order, limit, time_limit_sec, 3, 32, 10);
    if (valid_program_for_limit(answer, limit)) return answer;

    vector<char> fallback = best_macro_compress(raw, limit, false);
    if (valid_program_for_limit(fallback, limit)) return fallback;
    return raw;
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    const double start_time = get_time();
    constexpr double TOTAL_TIME_LIMIT = 1.95;

    Input in = read_input();
    Solver solver(in);
    vector<int> order = build_initial_order_fast(solver);

    double anneal_time = max(0.05, TOTAL_TIME_LIMIT - (get_time() - start_time));
    vector<char> answer = solve_with_p_macro_reroute(solver, order, in.T, anneal_time);
    for (char op : answer) {
        cout << op << '\n';
    }
    return 0;
}
