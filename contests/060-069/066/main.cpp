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

struct MoveSummary {
    int out_len = 0;
    int p_count = 0;
    int end_cell = 0;
    int end_dir = 0;
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
        build_state_transition_table();
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

    MoveSummary basic_move_summary(int from, int from_dir, int to) const {
        int cur = from;
        int dir = from_dir;
        int out_len = 0;
        while (cur != to) {
            int next_dir = first_dir[cur][to];
            if (next_dir == -1) break;
            int diff = (next_dir - dir + 4) % 4;
            if (diff == 1 || diff == 3) {
                ++out_len;
            } else if (diff == 2) {
                out_len += 2;
            }
            ++out_len;
            dir = next_dir;
            cur = neighbor(cur, next_dir);
        }
        return MoveSummary{out_len, 0, to, dir};
    }

    vector<int> build_macro_transition(const vector<char> &macro) const {
        vector<int> transition(total * 4);
        for (int state = 0; state < total * 4; ++state) {
            int cur = state;
            for (char op : macro) {
                if (op == 'R') {
                    cur = state_right[cur];
                } else if (op == 'L') {
                    cur = state_left[cur];
                } else if (op == 'F') {
                    int nxt = state_forward[cur];
                    if (nxt != -1) cur = nxt;
                }
            }
            transition[state] = cur;
        }
        return transition;
    }

    vector<int> build_macro_transition_from_buttons(const vector<char> &buttons, const vector<int> &active_macro_transition) const {
        vector<int> transition(total * 4);
        for (int state = 0; state < total * 4; ++state) {
            int cur = state;
            for (char op : buttons) {
                if (op == 'R') {
                    cur = state_right[cur];
                } else if (op == 'L') {
                    cur = state_left[cur];
                } else if (op == 'F') {
                    int nxt = state_forward[cur];
                    if (nxt != -1) cur = nxt;
                } else if (op == 'P' && !active_macro_transition.empty()) {
                    cur = active_macro_transition[cur];
                }
            }
            transition[state] = cur;
        }
        return transition;
    }

    vector<char> macro_move_ops_with_transition(int from, int from_dir, int to, const vector<int> &macro_transition, int &end_cell, int &end_dir) const {
        int start = from * 4 + from_dir;
        int goal = run_macro_bfs(start, to, macro_transition);

        vector<char> ops;
        if (goal == -1) {
            return basic_move_ops(from, from_dir, to, end_cell, end_dir);
        }

        ops.reserve(macro_bfs_dist[goal]);
        for (int cur = goal; cur != start; cur = macro_bfs_prev[cur]) {
            ops.push_back(macro_bfs_op[cur]);
        }
        reverse(ops.begin(), ops.end());
        end_cell = state_cell[goal];
        end_dir = state_dir[goal];
        return ops;
    }

    MoveSummary macro_move_summary_with_transition(int from, int from_dir, int to, const vector<int> &macro_transition) const {
        int start = from * 4 + from_dir;
        int goal = run_macro_bfs(start, to, macro_transition);
        if (goal == -1) return basic_move_summary(from, from_dir, to);

        int p_count = 0;
        for (int cur = goal; cur != start; cur = macro_bfs_prev[cur]) {
            if (macro_bfs_op[cur] == 'P') ++p_count;
        }
        return MoveSummary{macro_bfs_dist[goal], p_count, state_cell[goal], state_dir[goal]};
    }

  private:
    static constexpr int UP = 0;
    static constexpr int RIGHT = 1;
    static constexpr int DOWN = 2;
    static constexpr int LEFT = 3;
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
    vector<int> state_cell;
    vector<int> state_dir;
    vector<int> state_right;
    vector<int> state_left;
    vector<int> state_forward;
    vector<vector<int>> first_dir;
    mutable vector<int> macro_bfs_dist;
    mutable vector<int> macro_bfs_prev;
    mutable vector<int> macro_bfs_queue;
    mutable vector<int> macro_bfs_seen;
    mutable vector<char> macro_bfs_op;
    mutable int macro_bfs_stamp = 0;

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

    int run_macro_bfs(int start, int target_cell, const vector<int> &macro_transition) const {
        const int states = total * 4;
        ensure_macro_bfs_buffers(states);
        ++macro_bfs_stamp;
        if (macro_bfs_stamp == numeric_limits<int>::max()) {
            fill(macro_bfs_seen.begin(), macro_bfs_seen.end(), 0);
            macro_bfs_stamp = 1;
        }
        const int stamp = macro_bfs_stamp;

        macro_bfs_seen[start] = stamp;
        macro_bfs_dist[start] = 0;
        int q_head = 0;
        int q_tail = 0;
        macro_bfs_queue[q_tail++] = start;
        int goal = -1;

        auto push_state = [&](int cur_state, int ns, char op) {
            if (ns == cur_state || macro_bfs_seen[ns] == stamp) return;
            macro_bfs_seen[ns] = stamp;
            macro_bfs_dist[ns] = macro_bfs_dist[cur_state] + 1;
            macro_bfs_prev[ns] = cur_state;
            macro_bfs_op[ns] = op;
            macro_bfs_queue[q_tail++] = ns;
        };

        while (q_head < q_tail) {
            int cur_state = macro_bfs_queue[q_head++];
            int cell = state_cell[cur_state];
            int dir = state_dir[cur_state];
            if (cell == target_cell) {
                if (goal == -1 || macro_bfs_dist[cur_state] < macro_bfs_dist[goal] ||
                    (macro_bfs_dist[cur_state] == macro_bfs_dist[goal] && dir < state_dir[goal])) {
                    goal = cur_state;
                }
                continue;
            }
            if (goal != -1 && macro_bfs_dist[cur_state] >= macro_bfs_dist[goal]) continue;

            push_state(cur_state, state_right[cur_state], 'R');
            push_state(cur_state, state_left[cur_state], 'L');
            if (state_forward[cur_state] != -1) push_state(cur_state, state_forward[cur_state], 'F');

            if (!macro_transition.empty()) push_state(cur_state, macro_transition[cur_state], 'P');
        }

        return goal;
    }

    void ensure_macro_bfs_buffers(int states) const {
        if ((int)macro_bfs_dist.size() == states) return;
        macro_bfs_dist.resize(states);
        macro_bfs_prev.resize(states);
        macro_bfs_queue.resize(states);
        macro_bfs_seen.assign(states, 0);
        macro_bfs_op.resize(states);
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

    void build_state_transition_table() {
        const int states = total * 4;
        state_cell.resize(states);
        state_dir.resize(states);
        state_right.resize(states);
        state_left.resize(states);
        state_forward.resize(states);
        for (int cell = 0; cell < total; ++cell) {
            for (int dir = 0; dir < 4; ++dir) {
                int s = cell * 4 + dir;
                state_cell[s] = cell;
                state_dir[s] = dir;
                state_right[s] = cell * 4 + (dir + 1) % 4;
                state_left[s] = cell * 4 + (dir + 3) % 4;
                int nxt = next_cell[cell][dir];
                state_forward[s] = (nxt == -1 ? -1 : nxt * 4 + dir);
            }
        }
    }

    void build_shortest_paths() {
        first_dir.assign(total, vector<int>(total, -1));

        for (int s = 0; s < total; ++s) {
            vector<int> dist(total, -1);
            queue<int> q;
            dist[s] = 0;
            q.push(s);

            while (!q.empty()) {
                int cur = q.front();
                q.pop();

                for (int dir = 0; dir < 4; ++dir) {
                    if (!can_move(cur, dir)) continue;
                    int nxt = neighbor(cur, dir);
                    if (dist[nxt] != -1) continue;

                    dist[nxt] = dist[cur] + 1;
                    first_dir[s][nxt] = (cur == s ? dir : first_dir[s][cur]);
                    q.push(nxt);
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
            int next_dir = first_dir[cur][to];
            if (next_dir == -1) break;
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
    int end_cell = 0;
    int end_dir = 0;
    return (int)solver.basic_move_ops(from, dir, to, end_cell, end_dir).size();
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
            vector<int> order;
            order.reserve(m);
            order.push_back(tail);
            used[tail] = 1;

            while ((int)order.size() < m) {
                int head = order.front();
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
                order.insert(order.begin(), best);
                used[best] = 1;
            }
            orders.push_back(std::move(order));
        }
    }

    sort(orders.begin(), orders.end());
    orders.erase(unique(orders.begin(), orders.end()), orders.end());
    return orders;
}

vector<int> build_light_goal_reverse_order(const Solver &solver) {
    vector<vector<int>> orders = build_goal_reverse_orders(solver);
    if (orders.empty()) return solver.build_greedy_order();

    vector<pair<int, int>> scored;
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
};

static constexpr int MIN_P_MACRO_LEN = 4;

struct FixedPMacroPlan {
    static constexpr int CAP = 16;
    array<PMacroPlacement, CAP> data;
    int size = 0;
};

struct MacroContainStats {
    int definitions = 0;
    int contained_definitions = 0;
    int contained_p_count = 0;
    int encoded_saving = 0;
    int consecutive_contained = 0;
    int max_contained_chain = 0;
};

struct PMacroPlanEval {
    int actual_score = numeric_limits<int>::max() / 4;
    int anneal_score = numeric_limits<int>::max() / 4;
    int out_len = 0;
    int expanded_len = 0;
    int definitions = 0;
    int route_p_count = 0;
    int contained_definitions = 0;
    int contained_p_count = 0;
    int encoded_saving = 0;
    int consecutive_contained = 0;
    int max_contained_chain = 0;
    int max_macro_len = 0;
    int soft_bonus = 0;
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
    const vector<int> lens = {4, 5, 6, 7, 8, 10, 12, 16, 20, 24, 32, 40, 56, 72, 96, 120};

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

vector<PMacroPlacement> collect_repeated_p_macro_placements(const vector<vector<char>> &legs) {
    vector<PMacroPlacement> repeated;
    const vector<int> lens = {4, 5, 6, 7, 8, 10, 12, 16, 20, 24, 32, 40, 56, 72, 96, 120};
    constexpr int REPEATED_CAP = 60000;

    for (int len : lens) {
        unordered_map<string, vector<PMacroPlacement>> buckets;
        int estimate = 0;
        for (const auto &leg : legs) estimate += max(0, (int)leg.size() - len + 1);
        buckets.reserve(max(16, estimate * 2));

        for (int leg = 0; leg < (int)legs.size(); ++leg) {
            const int m = (int)legs[leg].size();
            for (int offset = 0; offset + len <= m; ++offset) {
                string key(legs[leg].begin() + offset, legs[leg].begin() + offset + len);
                buckets[key].push_back(PMacroPlacement{leg, offset, len});
            }
        }

        for (auto &entry : buckets) {
            auto &places = entry.second;
            if ((int)places.size() < 2) continue;
            const int keep = min((int)places.size(), 12);
            for (int i = 0; i < keep; ++i) repeated.push_back(places[i]);
            if ((int)repeated.size() >= REPEATED_CAP) break;
        }
        if ((int)repeated.size() >= REPEATED_CAP) break;
    }

    sort(repeated.begin(), repeated.end(), [](const PMacroPlacement &lhs, const PMacroPlacement &rhs) {
        if (lhs.leg != rhs.leg) return lhs.leg < rhs.leg;
        if (lhs.offset != rhs.offset) return lhs.offset < rhs.offset;
        return lhs.len < rhs.len;
    });
    repeated.erase(unique(repeated.begin(), repeated.end(), [](const PMacroPlacement &lhs, const PMacroPlacement &rhs) {
                       return lhs.leg == rhs.leg && lhs.offset == rhs.offset && lhs.len == rhs.len;
                   }),
                   repeated.end());
    return repeated;
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
        p.len = max(MIN_P_MACRO_LEN, p.len);
        if (p.leg == last_leg) continue;
        res.push_back(p);
        last_leg = p.leg;
    }
    return res;
}

FixedPMacroPlan normalize_p_macro_plan_fixed(const vector<PMacroPlacement> &plan, int leg_count) {
    FixedPMacroPlan tmp;
    tmp.size = min((int)plan.size(), FixedPMacroPlan::CAP);
    for (int i = 0; i < tmp.size; ++i) tmp.data[i] = plan[i];

    auto less_placement = [](const PMacroPlacement &lhs, const PMacroPlacement &rhs) {
        if (lhs.leg != rhs.leg) return lhs.leg < rhs.leg;
        if (lhs.offset != rhs.offset) return lhs.offset < rhs.offset;
        return lhs.len > rhs.len;
    };

    for (int i = 1; i < tmp.size; ++i) {
        PMacroPlacement x = tmp.data[i];
        int j = i - 1;
        while (j >= 0 && less_placement(x, tmp.data[j])) {
            tmp.data[j + 1] = tmp.data[j];
            --j;
        }
        tmp.data[j + 1] = x;
    }

    FixedPMacroPlan res;
    int last_leg = -1;
    for (int i = 0; i < tmp.size; ++i) {
        PMacroPlacement p = tmp.data[i];
        p.leg = max(0, min(p.leg, leg_count - 1));
        p.offset = max(0, p.offset);
        p.len = max(MIN_P_MACRO_LEN, p.len);
        if (p.leg == last_leg) continue;
        res.data[res.size++] = p;
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

        vector<int> next_macro_transition = solver.build_macro_transition_from_buttons(definition_buttons, macro_transition);
        macro = std::move(new_macro);
        macro_transition = std::move(next_macro_transition);
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

int score_p_macro_program(const vector<char> &program, int limit) {
    int score = (int)program.size();
    int expanded = expanded_basic_count(program);
    if (score > limit) score += 1000000 + score - limit;
    if (expanded > limit) score += 1000 * (expanded - limit);
    return score;
}

int p_macro_actual_score(int out_len, int expanded_len, int limit) {
    int score = out_len;
    if (out_len > limit) score += 1000000 + out_len - limit;
    if (expanded_len > limit) score += 1000 * (expanded_len - limit);
    return score;
}

static constexpr int PMACRO_ANNEAL_SCALE = 64;
static constexpr int PMACRO_SOFT_BONUS_CAP = 224;

int p_macro_anneal_score(int actual_score, int soft_bonus) {
    return actual_score * PMACRO_ANNEAL_SCALE - soft_bonus;
}

int p_macro_plan_soft_bonus(const PMacroPlanEval &eval, int limit) {
    int bonus = 0;
    bonus += min(80, 4 * eval.route_p_count);
    bonus += min(70, 8 * eval.contained_definitions + 5 * eval.contained_p_count);
    bonus += min(60, eval.encoded_saving / 2);
    bonus += 8 * eval.max_contained_chain;
    bonus += 3 * eval.consecutive_contained;
    bonus += min(30, eval.max_macro_len / 8);
    bonus += min(20, eval.definitions);

    if (eval.out_len > limit || eval.expanded_len > limit) bonus /= 4;
    return min(PMACRO_SOFT_BONUS_CAP, bonus);
}

PMacroPlanEval evaluate_p_macro_plan_fast(const Solver &solver, const vector<int> &order, const vector<PMacroPlacement> &plan, int limit, int cutoff = numeric_limits<int>::max() / 4) {
    const int leg_count = (int)order.size() * 2;
    FixedPMacroPlan norm_plan = normalize_p_macro_plan_fixed(plan, leg_count);

    vector<char> macro;
    vector<int> macro_transition;
    bool has_macro = false;
    int cell = 0;
    int dir = 1;
    int leg = 0;
    int plan_idx = 0;
    int contained_chain = 0;
    PMacroPlanEval eval;

    auto current_score_value = [&]() {
        return p_macro_actual_score(eval.out_len, eval.expanded_len, limit);
    };

    auto exceeded_cutoff = [&]() {
        constexpr int SOFT_CUTOFF_SLACK = PMACRO_ANNEAL_SCALE * 8;
        int actual_lower = current_score_value() * PMACRO_ANNEAL_SCALE - PMACRO_SOFT_BONUS_CAP;
        return actual_lower > cutoff + SOFT_CUTOFF_SLACK;
    };

    auto finish_eval = [&]() {
        eval.actual_score = current_score_value();
        eval.soft_bonus = p_macro_plan_soft_bonus(eval, limit);
        eval.anneal_score = p_macro_anneal_score(eval.actual_score, eval.soft_bonus);
        return eval;
    };

    auto cutoff_eval = [&]() {
        eval.actual_score = numeric_limits<int>::max() / 8;
        eval.soft_bonus = 0;
        eval.anneal_score = cutoff + 1;
        return eval;
    };

    auto append_and_apply = [&](const vector<char> &buttons, const vector<char> &active_macro, bool definition_body = false) {
        eval.out_len += (int)buttons.size();
        for (char op : buttons) {
            if (op == 'P') {
                eval.expanded_len += (int)active_macro.size();
                if (definition_body) {
                    ++eval.contained_p_count;
                } else {
                    ++eval.route_p_count;
                }
                if (!active_macro.empty()) {
                    int next_state = macro_transition[cell * 4 + dir];
                    cell = next_state / 4;
                    dir = next_state % 4;
                }
            } else {
                ++eval.expanded_len;
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

    auto current_summary = [&](int target) {
        if (has_macro) {
            return solver.macro_move_summary_with_transition(cell, dir, target, macro_transition);
        }
        return solver.basic_move_summary(cell, dir, target);
    };

    auto append_summary = [&](const MoveSummary &summary, const vector<char> &active_macro) {
        eval.out_len += summary.out_len;
        eval.expanded_len += summary.out_len + summary.p_count * ((int)active_macro.size() - 1);
        eval.route_p_count += summary.p_count;
        cell = summary.end_cell;
        dir = summary.end_dir;
    };

    auto move_to = [&](int target) {
        bool has_definition_request = false;
        PMacroPlacement def;
        while (plan_idx < norm_plan.size && norm_plan.data[plan_idx].leg < leg) ++plan_idx;
        if (plan_idx < norm_plan.size && norm_plan.data[plan_idx].leg == leg) {
            def = norm_plan.data[plan_idx];
            has_definition_request = true;
            ++plan_idx;
        }

        if (!has_definition_request) {
            MoveSummary summary = current_summary(target);
            append_summary(summary, macro);
            ++leg;
            return;
        }

        vector<char> route = current_route(target);
        if (def.offset + def.len > (int)route.size()) {
            append_and_apply(route, macro);
            ++leg;
            return;
        }

        vector<char> prefix(route.begin(), route.begin() + def.offset);
        append_and_apply(prefix, macro);

        vector<char> raw_definition(route.begin() + def.offset, route.begin() + def.offset + def.len);
        vector<char> new_macro = expand_buttons_with_macro(raw_definition, macro);
        vector<char> definition_buttons = encode_with_previous_macro(new_macro, macro);
        int definition_p_count = (int)count(definition_buttons.begin(), definition_buttons.end(), 'P');
        ++eval.definitions;
        eval.max_macro_len = max(eval.max_macro_len, (int)new_macro.size());
        if (definition_p_count > 0) {
            ++eval.contained_definitions;
            eval.encoded_saving += max(0, (int)new_macro.size() - (int)definition_buttons.size());
            ++contained_chain;
            eval.consecutive_contained += max(0, contained_chain - 1);
            eval.max_contained_chain = max(eval.max_contained_chain, contained_chain);
        } else {
            contained_chain = 0;
        }

        eval.out_len += 2;
        append_and_apply(definition_buttons, macro, true);

        vector<int> next_macro_transition = solver.build_macro_transition_from_buttons(definition_buttons, macro_transition);
        macro = std::move(new_macro);
        macro_transition = std::move(next_macro_transition);
        has_macro = true;

        vector<char> rest = current_route(target);
        append_and_apply(rest, macro);
        ++leg;
    };

    for (int k : order) {
        move_to(solver.ball_cell(k));
        if (exceeded_cutoff()) return cutoff_eval();
        ++eval.out_len;
        ++eval.expanded_len;
        if (exceeded_cutoff()) return cutoff_eval();
        move_to(solver.basket_cell(k));
        if (exceeded_cutoff()) return cutoff_eval();
        ++eval.out_len;
        ++eval.expanded_len;
        if (exceeded_cutoff()) return cutoff_eval();
    }

    return finish_eval();
}

int score_p_macro_plan_fast(const Solver &solver, const vector<int> &order, const vector<PMacroPlacement> &plan, int limit, int cutoff = numeric_limits<int>::max() / 4) {
    return evaluate_p_macro_plan_fast(solver, order, plan, limit, cutoff).actual_score;
}

int macro_containment_bonus(const MacroContainStats &stats) {
    int bonus = 0;
    bonus += 3 * stats.contained_definitions;
    bonus += stats.contained_p_count;
    bonus += 5 * stats.consecutive_contained;
    bonus += 8 * stats.max_contained_chain;
    return min(31, bonus);
}

vector<char> anneal_multi_p_macro_reroute(const Solver &solver, const vector<int> &order, int limit, double time_limit_sec, int seed_rounds = 5, int seed_trials = 80, int max_defs = 10) {
    if (time_limit_sec <= 0.0) return solver.build_ops(order);

    const double start_time = get_time();
    vector<vector<char>> legs = build_basic_movement_legs(solver, order);
    vector<PMacroPlacement> candidates = collect_p_macro_placements(legs);
    vector<PMacroPlacement> repeated_candidates = collect_repeated_p_macro_placements(legs);
    if (candidates.empty()) return solver.build_ops(order);

    auto repair = [&](PMacroPlacement p) {
        p.leg = max(0, min(p.leg, (int)legs.size() - 1));
        int leg_size = (int)legs[p.leg].size();
        if (leg_size < MIN_P_MACRO_LEN) {
            p.offset = 0;
            p.len = 0;
            return p;
        }
        p.offset = max(0, min(p.offset, leg_size - MIN_P_MACRO_LEN));
        p.len = max(MIN_P_MACRO_LEN, min(p.len, leg_size - p.offset));
        return p;
    };

    auto random_def = [&]() {
        int roll = rnd::get(100);
        if (!repeated_candidates.empty() && roll < 55) return repeated_candidates[rnd::get((int)repeated_candidates.size())];
        if (roll < 90) return candidates[rnd::get((int)candidates.size())];
        PMacroPlacement p;
        p.leg = rnd::get((int)legs.size());
        int leg_size = (int)legs[p.leg].size();
        if (leg_size < MIN_P_MACRO_LEN) return candidates[rnd::get((int)candidates.size())];
        p.offset = rnd::get(leg_size - MIN_P_MACRO_LEN + 1);
        int max_len = min(160, leg_size - p.offset);
        p.len = MIN_P_MACRO_LEN + rnd::get(max_len - MIN_P_MACRO_LEN + 1);
        return p;
    };

    auto eval = [&](const vector<PMacroPlacement> &plan, int cutoff = numeric_limits<int>::max() / 4) {
        return evaluate_p_macro_plan_fast(solver, order, plan, limit, cutoff);
    };

    auto better_eval = [](const PMacroPlanEval &lhs, const PMacroPlanEval &rhs) {
        if (lhs.anneal_score != rhs.anneal_score) return lhs.anneal_score < rhs.anneal_score;
        return lhs.actual_score < rhs.actual_score;
    };

    auto better_actual_eval = [](const PMacroPlanEval &lhs, const PMacroPlanEval &rhs) {
        if (lhs.actual_score != rhs.actual_score) return lhs.actual_score < rhs.actual_score;
        return lhs.anneal_score < rhs.anneal_score;
    };

    vector<PMacroPlacement> current_plan;
    PMacroPlanEval current_eval = eval(current_plan);
    int current_score = current_eval.anneal_score;

    for (int round = 0; round < seed_rounds; ++round) {
        if (get_time() - start_time >= time_limit_sec) break;
        vector<PMacroPlacement> best_add_plan = current_plan;
        PMacroPlanEval best_add_eval = current_eval;
        for (int trial = 0; trial < seed_trials; ++trial) {
            if (get_time() - start_time >= time_limit_sec) break;
            vector<PMacroPlacement> next_plan = current_plan;
            next_plan.push_back(random_def());
            PMacroPlanEval next_eval = eval(next_plan, best_add_eval.anneal_score - 1);
            if (better_actual_eval(next_eval, best_add_eval)) {
                best_add_eval = next_eval;
                best_add_plan = std::move(next_plan);
            }
        }
        if (better_actual_eval(best_add_eval, current_eval)) {
            current_plan = std::move(best_add_plan);
            current_eval = best_add_eval;
            current_score = current_eval.anneal_score;
        }
    }

    vector<PMacroPlacement> best_plan = current_plan;
    PMacroPlanEval best_eval = current_eval;
    vector<PMacroPlacement> best_actual_plan = current_plan;
    PMacroPlanEval best_actual_eval = current_eval;
    int best_score = current_score;

    static double log_table[65536];
    static bool initialized = false;
    if (!initialized) {
        for (int i = 0; i < 65536; ++i) log_table[i] = log((i + 0.5) / 65536.0);
        rnd::shuffle(log_table);
        initialized = true;
    }

    const double T0 = 24.0 * PMACRO_ANNEAL_SCALE;
    const double T1 = 0.04 * PMACRO_ANNEAL_SCALE;
    double heat = T0;
    int iter = 0;
    static constexpr int NEIGHBOR_TYPES = 13;
    static constexpr int TIME_BINS = 10;
    array<array<int, NEIGHBOR_TYPES>, TIME_BINS> neighbor_proposed{};
    array<array<int, NEIGHBOR_TYPES>, TIME_BINS> neighbor_accepted{};
    array<array<int, NEIGHBOR_TYPES>, TIME_BINS> neighbor_improved{};

    while (true) {
        double progress = (get_time() - start_time) / time_limit_sec;
        if (progress >= 1.0) break;
        int time_bin = min(TIME_BINS - 1, max(0, (int)(progress * TIME_BINS)));
        if ((iter & 3) == 0) {
            heat = T0 * pow(T1 / T0, progress);
        }
        ++iter;

        vector<PMacroPlacement> next_plan = current_plan;
        int roll = rnd::get(100);
        int type = 0;
        if (next_plan.empty() || roll < 14) {
            type = 0;
        } else if (roll < 21) {
            type = 1;
        } else if (roll < 30) {
            type = 2;
        } else if (roll < 38) {
            type = 3;
        } else if (roll < 47) {
            type = 4;
        } else if (roll < 56) {
            type = 5;
        } else if (roll < 62) {
            type = 6;
        } else if (roll < 66) {
            type = 7;
        } else if (roll < 75) {
            type = 8;
        } else if (roll < 85) {
            type = 9;
        } else if (roll < 93) {
            type = 10;
        } else if (roll < 97) {
            type = 11;
        } else {
            type = 12;
        }
        int neighbor_type = type;
        ++neighbor_proposed[time_bin][neighbor_type];

        if (type == 0 || next_plan.empty()) {
            if ((int)next_plan.size() < max_defs) next_plan.push_back(random_def());
        } else if (type == 1) {
            int idx = rnd::get((int)next_plan.size());
            next_plan.erase(next_plan.begin() + idx);
        } else if (type == 2) {
            int idx = rnd::get((int)next_plan.size());
            next_plan[idx] = random_def();
        } else if (type == 3) {
            int idx = rnd::get((int)next_plan.size());
            next_plan[idx].leg += rnd::range(-5, 6);
            next_plan[idx] = repair(next_plan[idx]);
        } else if (type == 4) {
            int idx = rnd::get((int)next_plan.size());
            next_plan[idx].offset += rnd::range(-14, 15);
            next_plan[idx] = repair(next_plan[idx]);
        } else if (type == 5) {
            int idx = rnd::get((int)next_plan.size());
            next_plan[idx].len += rnd::range(-28, 29);
            next_plan[idx] = repair(next_plan[idx]);
        } else if (type == 6) {
            int cnt = min(3, max(1, (int)next_plan.size()));
            for (int i = 0; i < cnt; ++i) {
                int idx = rnd::get((int)next_plan.size());
                next_plan[idx] = random_def();
            }
        } else if (type == 7) {
            rnd::shuffle(next_plan);
            while ((int)next_plan.size() > 1 && rnd::get(100) < 35) {
                next_plan.erase(next_plan.begin() + rnd::get((int)next_plan.size()));
            }
            while ((int)next_plan.size() < 8 && rnd::get(100) < 55) {
                next_plan.push_back(random_def());
            }
        } else if (type == 8) {
            int idx = rnd::get((int)next_plan.size());
            next_plan[idx].leg += rnd::range(-1, 2);
            next_plan[idx] = repair(next_plan[idx]);
        } else if (type == 9) {
            int idx = rnd::get((int)next_plan.size());
            next_plan[idx].offset += rnd::range(-3, 4);
            next_plan[idx] = repair(next_plan[idx]);
        } else if (type == 10) {
            int idx = rnd::get((int)next_plan.size());
            next_plan[idx].len += rnd::range(-6, 7);
            next_plan[idx] = repair(next_plan[idx]);
        } else if (type == 11) {
            int idx = rnd::get((int)next_plan.size());
            PMacroPlacement p = next_plan[idx];
            p.leg += rnd::range(-2, 3);
            p.offset += rnd::range(-8, 9);
            p.len += rnd::range(-12, 13);
            p = repair(p);
            if (p.len >= MIN_P_MACRO_LEN && (int)next_plan.size() < max_defs) next_plan.push_back(p);
        } else {
            int idx = rnd::get((int)next_plan.size());
            int shift = rnd::range(-8, 9);
            next_plan[idx].offset += shift;
            next_plan[idx].len -= shift;
            next_plan[idx] = repair(next_plan[idx]);
        }

        for (PMacroPlacement &p : next_plan) p = repair(p);
        next_plan.erase(remove_if(next_plan.begin(), next_plan.end(), [](const PMacroPlacement &p) {
                            return p.len < MIN_P_MACRO_LEN;
                        }),
                        next_plan.end());
        while ((int)next_plan.size() > max_defs) {
            next_plan.erase(next_plan.begin() + rnd::get((int)next_plan.size()));
        }

        double accept_margin = -heat * log_table[iter & 65535];
        int accept_cutoff = current_score + (int)floor(accept_margin);
        PMacroPlanEval next_eval = eval(next_plan, accept_cutoff);
        int next_score = next_eval.anneal_score;
        int delta = next_score - current_score;
        if (better_actual_eval(next_eval, best_actual_eval)) {
            best_actual_eval = next_eval;
            best_actual_plan = next_plan;
        }
        if (delta <= accept_margin) {
            ++neighbor_accepted[time_bin][neighbor_type];
            if (delta < 0) ++neighbor_improved[time_bin][neighbor_type];
            current_plan = std::move(next_plan);
            current_eval = next_eval;
            current_score = current_eval.anneal_score;
            if (better_eval(current_eval, best_eval)) {
                best_eval = current_eval;
                best_score = current_score;
                best_plan = current_plan;
            }
        }
    }

    MacroContainStats stats;
    vector<char> answer = build_program_with_p_macro_plan(solver, order, best_actual_plan, &stats);
    PMacroPlanEval final_eval = eval(best_actual_plan);
    cerr << "p_macro_contain defs=" << stats.definitions
         << " contained_defs=" << stats.contained_definitions
         << " p_count=" << stats.contained_p_count
         << " saving=" << stats.encoded_saving
         << " chain=" << stats.max_contained_chain
         << " bonus=" << macro_containment_bonus(stats)
         << " repeated_candidates=" << repeated_candidates.size()
         << " actual=" << final_eval.actual_score
         << " anneal=" << final_eval.anneal_score
         << " best_anneal_actual=" << best_eval.actual_score
         << " best_anneal_score=" << best_eval.anneal_score
         << " soft_bonus=" << final_eval.soft_bonus
         << " iter=" << iter
         << '\n';
    for (int bin = 0; bin < TIME_BINS; ++bin) {
        for (int type = 0; type < NEIGHBOR_TYPES; ++type) {
            cerr << "neigh_stat"
                 << " bin=" << bin
                 << " type=" << type
                 << " prop=" << neighbor_proposed[bin][type]
                 << " acc=" << neighbor_accepted[bin][type]
                 << " improve=" << neighbor_improved[bin][type]
                 << '\n';
        }
    }
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
    vector<char> answer = anneal_multi_p_macro_reroute(solver, order, limit, time_limit_sec, 3, 48, 12);
    vector<char> fallback = best_macro_compress(raw, limit, false);

    vector<char> best;
    int best_score = numeric_limits<int>::max();
    auto consider = [&](const vector<char> &program) {
        if (!valid_program_for_limit(program, limit)) return;
        int score = score_p_macro_program(program, limit);
        if (score < best_score) {
            best_score = score;
            best = program;
        }
    };

    consider(answer);
    consider(fallback);
    consider(raw);
    return best.empty() ? raw : best;
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    const double start_time = get_time();
    constexpr double TOTAL_TIME_LIMIT = 1.50;

    Input in = read_input();
    Solver solver(in);
    vector<int> order = build_light_goal_reverse_order(solver);

    double anneal_time = max(0.05, TOTAL_TIME_LIMIT - (get_time() - start_time));
    vector<char> answer = solve_with_p_macro_reroute(solver, order, in.T, anneal_time);
    for (char op : answer) {
        cout << op << '\n';
    }
    return 0;
}
