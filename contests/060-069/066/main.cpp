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
                Candidate cand = build_candidate(k, cur, dir);
                if (cand.cost < best.cost) best = std::move(cand);
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
        vector<int> transition(total * 4);
        for (int cell = 0; cell < total; ++cell) {
            for (int dir = 0; dir < 4; ++dir) {
                int next_cell = cell;
                int next_dir = dir;
                apply_basic_ops(macro, next_cell, next_dir);
                transition[cell * 4 + dir] = next_cell * 4 + next_dir;
            }
        }
        return transition;
    }

    vector<int> build_macro_transition_from_buttons(const vector<char> &buttons, const vector<int> &old_transition) const {
        const int states = total * 4;
        vector<int> transition(states);
        const bool has_old = !old_transition.empty();
        for (int st = 0; st < states; ++st) {
            int cell = st / 4;
            int dir = st & 3;
            for (char op : buttons) {
                if (op == 'P') {
                    if (has_old) {
                        int ns = old_transition[cell * 4 + dir];
                        cell = ns / 4;
                        dir = ns & 3;
                    }
                } else {
                    apply_basic_op(op, cell, dir);
                }
            }
            transition[st] = cell * 4 + dir;
        }
        return transition;
    }

    vector<char> macro_move_ops_with_transition(int from, int from_dir, int to, const vector<int> &macro_transition, int &end_cell, int &end_dir) const {
        const int states = total * 4;
        auto sid = [](int cell, int dir) {
            return cell * 4 + dir;
        };

        ensure_macro_bfs_buffers(states);
        next_macro_bfs_stamp();

        int start = sid(from, from_dir);
        macro_bfs_dist[start] = 0;
        macro_bfs_seen[start] = macro_bfs_stamp;
        int q_head = 0;
        int q_tail = 0;
        macro_bfs_queue[q_tail++] = start;
        int goal = -1;

        auto push_next = [&](int cur_state, int next_cell, int next_dir, char op) {
            int ns = sid(next_cell, next_dir);
            if (ns == cur_state || macro_bfs_seen[ns] == macro_bfs_stamp) return;
            macro_bfs_seen[ns] = macro_bfs_stamp;
            macro_bfs_dist[ns] = macro_bfs_dist[cur_state] + 1;
            macro_bfs_prev[ns] = cur_state;
            macro_bfs_op[ns] = op;
            macro_bfs_queue[q_tail++] = ns;
        };

        while (q_head < q_tail) {
            int cur_state = macro_bfs_queue[q_head++];
            int cell = cur_state / 4;
            int dir = cur_state & 3;
            if (cell == to) {
                if (goal == -1 || macro_bfs_dist[cur_state] < macro_bfs_dist[goal] ||
                    (macro_bfs_dist[cur_state] == macro_bfs_dist[goal] && dir < (goal & 3))) {
                    goal = cur_state;
                }
                continue;
            }
            if (goal != -1 && macro_bfs_dist[cur_state] >= macro_bfs_dist[goal]) continue;

            push_next(cur_state, cell, (dir + 1) & 3, 'R');
            push_next(cur_state, cell, (dir + 3) & 3, 'L');
            if (next_cell[cell][dir] != -1) push_next(cur_state, next_cell[cell][dir], dir, 'F');

            if (!macro_transition.empty()) {
                int next_state = macro_transition[cur_state];
                push_next(cur_state, next_state / 4, next_state & 3, 'P');
            }
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
        end_cell = goal / 4;
        end_dir = goal & 3;
        return ops;
    }

    RouteInfo macro_move_info_with_transition(int from, int from_dir, int to, const vector<int> &macro_transition) const {
        const int states = total * 4;
        auto sid = [](int cell, int dir) {
            return cell * 4 + dir;
        };

        ensure_macro_bfs_buffers(states);
        next_macro_bfs_stamp();

        int start = sid(from, from_dir);
        macro_bfs_dist[start] = 0;
        macro_bfs_seen[start] = macro_bfs_stamp;
        int q_head = 0;
        int q_tail = 0;
        macro_bfs_queue[q_tail++] = start;
        int goal = -1;

        auto push_next = [&](int cur_state, int next_state, char op) {
            if (next_state == cur_state || macro_bfs_seen[next_state] == macro_bfs_stamp) return;
            macro_bfs_seen[next_state] = macro_bfs_stamp;
            macro_bfs_dist[next_state] = macro_bfs_dist[cur_state] + 1;
            macro_bfs_prev[next_state] = cur_state;
            macro_bfs_op[next_state] = op;
            macro_bfs_queue[q_tail++] = next_state;
        };

        while (q_head < q_tail) {
            int cur_state = macro_bfs_queue[q_head++];
            int cell = cur_state / 4;
            int dir = cur_state & 3;
            if (cell == to) {
                if (goal == -1 || macro_bfs_dist[cur_state] < macro_bfs_dist[goal] ||
                    (macro_bfs_dist[cur_state] == macro_bfs_dist[goal] && dir < (goal & 3))) {
                    goal = cur_state;
                }
                continue;
            }
            if (goal != -1 && macro_bfs_dist[cur_state] >= macro_bfs_dist[goal]) continue;

            push_next(cur_state, sid(cell, (dir + 1) & 3), 'R');
            push_next(cur_state, sid(cell, (dir + 3) & 3), 'L');
            if (next_cell[cell][dir] != -1) push_next(cur_state, sid(next_cell[cell][dir], dir), 'F');
            if (!macro_transition.empty()) push_next(cur_state, macro_transition[cur_state], 'P');
        }

        if (goal == -1) return basic_move_info(from, from_dir, to);

        RouteInfo info;
        info.len = macro_bfs_dist[goal];
        info.end_cell = goal / 4;
        info.end_dir = goal & 3;
        int p_count = 0;
        for (int cur = goal; cur != start; cur = macro_bfs_prev[cur]) {
            if (macro_bfs_op[cur] == 'P') ++p_count;
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
    vector<char> raw = solver.build_ops(order);
    if ((int)raw.size() > solver.time_limit()) {
        return INF / 2 + (int)raw.size() - solver.time_limit();
    }
    return (int)raw.size();
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
    const int original_best = scored.front().first;

    const int refine_keep = min((int)scored.size(), 8);
    int refined_best = original_best;
    for (int rank = 0; rank < refine_keep; ++rank) {
        auto [refined, score] = refine_by_local_moves(orders[scored[rank].second], scored[rank].first);
        if (score < refined_best) refined_best = score;
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

    cerr << "order_select candidates=" << orders.size()
         << " refine_keep=" << refine_keep
         << " original_raw=" << original_best
         << " refined_raw=" << refined_best
         << " chosen_raw=" << scored.front().first
         << '\n';

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
};


struct MacroSite {
    int ball = 0;
    int phase = 0;  // 0: 現在地から ball へ移動する leg, 1: ball から basket へ移動する leg
    int offset = 0;
    int len = 0;
};

struct JointSearchState {
    vector<int> order;
    vector<MacroSite> sites;
};

vector<int> build_pos_in_order(const vector<int> &order, int m) {
    vector<int> pos(m, -1);
    for (int i = 0; i < (int)order.size(); ++i) pos[order[i]] = i;
    return pos;
}

vector<int> build_basic_leg_lengths(const Solver &solver, const vector<int> &order) {
    vector<int> leg_len;
    leg_len.reserve((int)order.size() * 2);

    int cell = 0;
    int dir = 1;
    for (int k : order) {
        Solver::RouteInfo to_ball = solver.basic_move_info(cell, dir, solver.ball_cell(k));
        leg_len.push_back(to_ball.len);
        cell = to_ball.end_cell;
        dir = to_ball.end_dir;

        Solver::RouteInfo to_basket = solver.basic_move_info(cell, dir, solver.basket_cell(k));
        leg_len.push_back(to_basket.len);
        cell = to_basket.end_cell;
        dir = to_basket.end_dir;
    }
    return leg_len;
}

struct OrderContext {
    vector<int> pos;
    vector<int> basic_leg_len;
};

OrderContext build_order_context(const Solver &solver, const vector<int> &order) {
    OrderContext ctx;
    ctx.pos = build_pos_in_order(order, solver.ball_count());
    ctx.basic_leg_len = build_basic_leg_lengths(solver, order);
    return ctx;
}

MacroSite repair_macro_site(const Solver &solver, const vector<int> &order, const OrderContext &ctx, MacroSite s) {
    const int m = solver.ball_count();
    if (m <= 0 || order.empty()) {
        s.ball = 0;
        s.phase = 0;
        s.offset = 0;
        s.len = 0;
        return s;
    }

    s.ball = max(0, min(s.ball, m - 1));
    s.phase = s.phase & 1;

    int p = ctx.pos[s.ball];
    if (p < 0) {
        s.offset = 0;
        s.len = 0;
        return s;
    }

    int leg = p * 2 + s.phase;
    if (leg < 0 || leg >= (int)ctx.basic_leg_len.size() || ctx.basic_leg_len[leg] < 6) {
        s.offset = 0;
        s.len = 0;
        return s;
    }

    const int L = ctx.basic_leg_len[leg];
    s.offset = max(0, min(s.offset, L - 6));
    int max_len = min(160, L - s.offset);
    if (max_len < 6) {
        s.len = 0;
        return s;
    }
    s.len = max(6, min(s.len, max_len));
    return s;
}

MacroSite repair_macro_site(const Solver &solver, const vector<int> &order, MacroSite s) {
    OrderContext ctx = build_order_context(solver, order);
    return repair_macro_site(solver, order, ctx, s);
}

void normalize_macro_sites_inplace(const Solver &solver, const vector<int> &order, const OrderContext &ctx,
                                   vector<MacroSite> &sites, int max_defs) {
    for (MacroSite &s : sites) s = repair_macro_site(solver, order, ctx, s);
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

void normalize_macro_sites_inplace(const Solver &solver, const vector<int> &order, vector<MacroSite> &sites, int max_defs) {
    OrderContext ctx = build_order_context(solver, order);
    normalize_macro_sites_inplace(solver, order, ctx, sites, max_defs);
}

vector<PMacroPlacement> convert_sites_to_plan(const Solver &solver, const vector<int> &order,
                                              const OrderContext &ctx, const vector<MacroSite> &sites) {
    vector<PMacroPlacement> plan;
    plan.reserve(sites.size());

    for (MacroSite s : sites) {
        s = repair_macro_site(solver, order, ctx, s);
        if (s.len < 6) continue;
        int p = ctx.pos[s.ball];
        if (p < 0) continue;
        plan.push_back(PMacroPlacement{p * 2 + s.phase, s.offset, s.len});
    }
    return plan;
}

vector<PMacroPlacement> convert_sites_to_plan(const Solver &solver, const vector<int> &order, const vector<MacroSite> &sites) {
    OrderContext ctx = build_order_context(solver, order);
    return convert_sites_to_plan(solver, order, ctx, sites);
}

// normalize_macro_sites_inplace() 済みの sites を PMacroPlacement に変換する軽量版。
// repair_macro_site() をここで再実行しないことで、評価時の重複計算を避ける。
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
        plan.push_back(PMacroPlacement{p * 2 + s.phase, s.offset, s.len});
    }

    // order が変わると ball 順と leg 順は一致しないので、既存処理に渡す前に leg 順へ整える。
    sort(plan.begin(), plan.end(), [](const PMacroPlacement &lhs, const PMacroPlacement &rhs) {
        if (lhs.leg != rhs.leg) return lhs.leg < rhs.leg;
        if (lhs.offset != rhs.offset) return lhs.offset < rhs.offset;
        return lhs.len > rhs.len;
    });

    return plan;
}


MacroSite random_macro_site(const Solver &solver, const vector<int> &order, const OrderContext &ctx) {
    (void)solver;
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
        return s;
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

int sample_weighted_type(const array<int, 8> &weights) {
    int total = 0;
    for (int w : weights) total += w;
    if (total <= 0) return rnd::get(8);

    int x = rnd::get(total);
    for (int i = 0; i < 8; ++i) {
        if (x < weights[i]) return i;
        x -= weights[i];
    }
    return 7;
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

    cerr << "initial_order candidates=" << orders.size()
         << " chosen_raw=" << best_score << '\n';
    return orders[best_idx];
}

struct MacroContainStats {
    int definitions = 0;
    int contained_definitions = 0;
    int contained_p_count = 0;
    int encoded_saving = 0;
    int consecutive_contained = 0;
    int max_contained_chain = 0;
};

vector<vector<char>> build_basic_movement_legs(const Solver &solver, const vector<int> &order) {
    vector<vector<char>> legs;
    int cell = 0;
    int dir = 1;

    for (int k : order) {
        int end_cell, end_dir;
        legs.push_back(solver.basic_move_ops(cell, dir, solver.ball_cell(k), end_cell, end_dir));
        cell = end_cell;
        dir = end_dir;
        legs.push_back(solver.basic_move_ops(cell, dir, solver.basket_cell(k), end_cell, end_dir));
        cell = end_cell;
        dir = end_dir;
    }

    return legs;
}

vector<PMacroPlacement> collect_p_macro_placements(const vector<vector<char>> &legs) {
    vector<PMacroPlacement> candidates;
    const vector<int> lens = {6, 8, 10, 12, 16, 20, 24, 32, 40, 56, 72, 96, 120};

    for (int leg = 0; leg < (int)legs.size(); ++leg) {
        int m = (int)legs[leg].size();
        for (int offset = 0; offset < m; ++offset) {
            for (int len : lens) {
                if (offset + len <= m) {
                    candidates.push_back(PMacroPlacement{leg, offset, len});
                }
            }
        }
    }

    return candidates;
}

void apply_buttons_with_macro(const Solver &solver, const vector<char> &buttons, const vector<char> &macro, int &cell, int &dir) {
    for (char op : buttons) {
        if (op == 'P') {
            solver.apply_basic_ops(macro, cell, dir);
        } else {
            solver.apply_basic_op(op, cell, dir);
        }
    }
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

vector<char> build_program_with_p_macro_plan(const Solver &solver, const vector<int> &order, vector<PMacroPlacement> plan, MacroContainStats *stats = nullptr) {
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
    int contained_chain = 0;

    auto append_and_apply = [&](const vector<char> &buttons, const vector<char> &active_macro) {
        program.insert(program.end(), buttons.begin(), buttons.end());
        for (char op : buttons) {
            if (op == 'P') {
                if (!active_macro.empty()) {
                    int next_state = macro_transition[cell * 4 + dir];
                    cell = next_state / 4;
                    dir = next_state % 4;
                }
            } else {
                solver.apply_basic_op(op, cell, dir);
            }
        }
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
            if (def.offset + def.len <= (int)route.size()) {
                define_here = true;
            }
            ++plan_idx;
        }

        if (!define_here) {
            append_and_apply(route, macro);
            ++leg;
            return;
        }

        vector<char> prefix(route.begin(), route.begin() + def.offset);
        append_and_apply(prefix, macro);

        vector<char> raw_definition(route.begin() + def.offset, route.begin() + def.offset + def.len);
        vector<char> new_macro = expand_buttons_with_macro(raw_definition, macro);
        vector<char> definition_buttons = encode_with_previous_macro(new_macro, macro);

        if (stats) {
            ++stats->definitions;
            int p_count = (int)count(definition_buttons.begin(), definition_buttons.end(), 'P');
            if (p_count > 0) {
                ++stats->contained_definitions;
                stats->contained_p_count += p_count;
                stats->encoded_saving += max(0, (int)new_macro.size() - (int)definition_buttons.size());
                ++contained_chain;
                stats->consecutive_contained += max(0, contained_chain - 1);
                stats->max_contained_chain = max(stats->max_contained_chain, contained_chain);
            } else {
                contained_chain = 0;
            }
        }

        program.push_back('M');
        program.insert(program.end(), definition_buttons.begin(), definition_buttons.end());
        program.push_back('M');
        for (char op : definition_buttons) {
            if (op == 'P') {
                if (!macro.empty()) {
                    int next_state = macro_transition[cell * 4 + dir];
                    cell = next_state / 4;
                    dir = next_state % 4;
                }
            } else {
                solver.apply_basic_op(op, cell, dir);
            }
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

struct CachedRouteOps {
    vector<char> ops;
};

struct PlanEvalCache {
    unordered_map<RouteCacheKey, Solver::RouteInfo, RouteCacheKeyHash> info_cache;
    unordered_map<RouteCacheKey, CachedRouteOps, RouteCacheKeyHash> ops_cache;
    long long info_hits = 0;
    long long info_misses = 0;
    long long ops_hits = 0;
    long long ops_misses = 0;
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

int score_p_macro_program(const vector<char> &program, int limit) {
    int score = (int)program.size();
    int expanded = expanded_basic_count(program);
    if (score > limit) score += 1000000 + score - limit;
    if (expanded > limit) score += 1000 * (expanded - limit);
    return score;
}

int score_p_macro_plan_fast(const Solver &solver, const vector<int> &order, vector<PMacroPlacement> plan, int limit, int cutoff = numeric_limits<int>::max() / 4, PlanEvalCache *cache = nullptr) {
    const int leg_count = (int)order.size() * 2;
    plan = normalize_p_macro_plan(std::move(plan), leg_count);

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
        for (char op : buttons) {
            if (op == 'P') {
                expanded_len += (int)active_macro.size();
                if (!active_macro.empty()) {
                    int next_state = macro_transition[cell * 4 + dir];
                    cell = next_state / 4;
                    dir = next_state % 4;
                }
            } else {
                ++expanded_len;
                solver.apply_basic_op(op, cell, dir);
            }
        }
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
                ++cache->ops_hits;
                return it->second.ops;
            }
            ++cache->ops_misses;
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
            auto it = cache->info_cache.find(key);
            if (it != cache->info_cache.end()) {
                ++cache->info_hits;
                return it->second;
            }
            ++cache->info_misses;
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
        PMacroPlacement def;
        bool has_plan_here = false;
        while (plan_idx < (int)plan.size() && plan[plan_idx].leg < leg) ++plan_idx;
        if (plan_idx < (int)plan.size() && plan[plan_idx].leg == leg) {
            def = plan[plan_idx];
            has_plan_here = true;
            ++plan_idx;
        }

        if (!has_plan_here) {
            append_info(current_route_info(target));
            ++leg;
            return;
        }

        vector<char> route = current_route(target);
        bool define_here = (def.offset + def.len <= (int)route.size());
        if (!define_here) {
            append_and_apply(route, macro);
            ++leg;
            return;
        }

        vector<char> prefix(route.begin(), route.begin() + def.offset);
        append_and_apply(prefix, macro);

        vector<char> raw_definition(route.begin() + def.offset, route.begin() + def.offset + def.len);
        vector<char> new_macro = expand_buttons_with_macro(raw_definition, macro);
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

int macro_containment_bonus(const MacroContainStats &stats) {
    int bonus = 0;
    bonus += 3 * stats.contained_definitions;
    bonus += stats.contained_p_count;
    bonus += 5 * stats.consecutive_contained;
    bonus += 8 * stats.max_contained_chain;
    return min(31, bonus);
}


vector<char> anneal_order_and_p_macro_reroute(const Solver &solver, const vector<int> &initial_order, int limit,
                                              double time_limit_sec, int seed_rounds = 3,
                                              int seed_trials = 32, int max_defs = 10) {
    if (time_limit_sec <= 0.0) return solver.build_ops(initial_order);

    const double start_time = get_time();
    if (initial_order.empty()) return solver.build_ops(initial_order);

    PlanEvalCache eval_cache;
    //eval_cache.reserve_memory();

    auto eval = [&](const vector<int> &ord, const OrderContext &ctx, const vector<MacroSite> &sites,
                    int cutoff = numeric_limits<int>::max() / 4) {
        vector<PMacroPlacement> plan = convert_sites_to_plan_no_repair(ord, ctx, sites);
        return score_p_macro_plan_fast(solver, ord, plan, limit, cutoff, &eval_cache);
    };

    auto build_answer = [&](const vector<int> &ord, const OrderContext &ctx,
                            const vector<MacroSite> &sites, MacroContainStats *stats) {
        vector<PMacroPlacement> plan = convert_sites_to_plan_no_repair(ord, ctx, sites);
        return build_program_with_p_macro_plan(solver, ord, plan, stats);
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
    int order_mutations = 0;
    int macro_mutations = 0;
    int accepted_order_mutations = 0;
    int accepted_macro_mutations = 0;
    array<int, 8> order_type_attempt{};
    array<int, 8> order_type_accept{};
    array<int, 8> macro_type_attempt{};
    array<int, 8> macro_type_accept{};

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
        static constexpr array<int, 8> MACRO_TYPE_WEIGHT = {
            16,  // 0: macro追加
            6,   // 1: macro削除
            10,  // 2: macro置換
            9,   // 3: ball/phase変更
            18,  // 4: offset変更
            18,  // 5: len変更
            5,   // 6: 複数macro置換
            18,  // 7: shuffle + 削除/追加
        };

        int type = -1;
        int order_type = -1;
        int macro_type = -1;
        const bool choose_macro = rnd::get(2) == 0;

        if (choose_macro) {
            macro_type = sample_weighted_type(MACRO_TYPE_WEIGHT);
            type = macro_type;
            ++macro_mutations;
            ++macro_type_attempt[macro_type];
            if (type == 0 || next_sites.empty()) {
                if ((int)next_sites.size() < max_defs) {
                    MacroSite s = random_macro_site(solver, *next_order, *next_ctx);
                    if (s.len >= 6) next_sites.push_back(s);
                }
            } else if (type == 1) {
                int idx = rnd::get((int)next_sites.size());
                next_sites.erase(next_sites.begin() + idx);
            } else if (type == 2) {
                int idx = rnd::get((int)next_sites.size());
                MacroSite s = random_macro_site(solver, *next_order, *next_ctx);
                if (s.len >= 6) next_sites[idx] = s;
            } else if (type == 3) {
                int idx = rnd::get((int)next_sites.size());
                next_sites[idx].ball = (*next_order)[rnd::get((int)next_order->size())];
                next_sites[idx].phase ^= rnd::get(2);
            } else if (type == 4) {
                int idx = rnd::get((int)next_sites.size());
                next_sites[idx].offset += rnd::range(-16, 17);
            } else if (type == 5) {
                int idx = rnd::get((int)next_sites.size());
                next_sites[idx].len += rnd::range(-32, 33);
            } else if (type == 6) {
                int cnt = min(3, max(1, (int)next_sites.size()));
                for (int i = 0; i < cnt; ++i) {
                    int idx = rnd::get((int)next_sites.size());
                    MacroSite s = random_macro_site(solver, *next_order, *next_ctx);
                    if (s.len >= 6) next_sites[idx] = s;
                }
            } else {
                rnd::shuffle(next_sites);
                while ((int)next_sites.size() > 1 && rnd::get(100) < 35) {
                    next_sites.erase(next_sites.begin() + rnd::get((int)next_sites.size()));
                }
                while ((int)next_sites.size() < max_defs && rnd::get(100) < 55) {
                    MacroSite s = random_macro_site(solver, *next_order, *next_ctx);
                    if (s.len >= 6) next_sites.push_back(s);
                }
            }
        } else {
            order_type = sample_weighted_type(ORDER_TYPE_WEIGHT);
            type = 8 + order_type;
            ++order_mutations;
            ++order_type_attempt[order_type];
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

            next_ctx_storage = build_order_context(solver, next_order_storage);
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
        int next_score = eval(*next_order, *next_ctx, next_sites, accept_cutoff);
        int delta = next_score - current_score;

        if (delta <= accept_margin) {
            if (touched_order) {
                current_order = std::move(next_order_storage);
                current_ctx = std::move(next_ctx_storage);
                ++accepted_order_mutations;
                if (order_type >= 0) ++order_type_accept[order_type];
            } else {
                ++accepted_macro_mutations;
                if (macro_type >= 0) ++macro_type_accept[macro_type];
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

    MacroContainStats stats;
    vector<char> answer = build_answer(best_order, best_ctx, best_sites, &stats);
    cerr << "joint_order_macro"
         << " initial_raw=" << raw_order_score(solver, initial_order)
         << " best_score=" << best_score
         << " best_raw=" << raw_order_score(solver, best_order)
         << " defs=" << stats.definitions
         << " contained_defs=" << stats.contained_definitions
         << " p_count=" << stats.contained_p_count
         << " saving=" << stats.encoded_saving
         << " chain=" << stats.max_contained_chain
         << " bonus=" << macro_containment_bonus(stats)
         << " iter=" << iter
         << " order_mut=" << accepted_order_mutations << "/" << order_mutations
         << " macro_mut=" << accepted_macro_mutations << "/" << macro_mutations
         << " route_info_cache=" << eval_cache.info_hits << "/" << (eval_cache.info_hits + eval_cache.info_misses)
         << " route_ops_cache=" << eval_cache.ops_hits << "/" << (eval_cache.ops_hits + eval_cache.ops_misses)
         << '\n';

    cerr << "order_type_attempt=";
    for (int i = 0; i < 8; ++i) {
        if (i) cerr << ',';
        cerr << order_type_attempt[i];
    }
    cerr << " order_type_accept=";
    for (int i = 0; i < 8; ++i) {
        if (i) cerr << ',';
        cerr << order_type_accept[i];
    }
    cerr << " macro_type_attempt=";
    for (int i = 0; i < 8; ++i) {
        if (i) cerr << ',';
        cerr << macro_type_attempt[i];
    }
    cerr << " macro_type_accept=";
    for (int i = 0; i < 8; ++i) {
        if (i) cerr << ',';
        cerr << macro_type_accept[i];
    }
    cerr << '\n';

    return answer;
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
