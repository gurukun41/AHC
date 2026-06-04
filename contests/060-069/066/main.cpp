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

    vector<char> macro_move_ops_with_transition(int from, int from_dir, int to, const vector<int> &macro_transition, int &end_cell, int &end_dir) const {
        const int states = total * 4;
        auto sid = [](int cell, int dir) {
            return cell * 4 + dir;
        };

        ensure_macro_bfs_buffers(states);
        fill(macro_bfs_dist.begin(), macro_bfs_dist.end(), -1);

        int start = sid(from, from_dir);
        macro_bfs_dist[start] = 0;
        int q_head = 0;
        int q_tail = 0;
        macro_bfs_queue[q_tail++] = start;
        int goal = -1;

        auto push_next = [&](int cur_state, int next_cell, int next_dir, char op) {
            int ns = sid(next_cell, next_dir);
            if (ns == cur_state || macro_bfs_dist[ns] != -1) return;
            macro_bfs_dist[ns] = macro_bfs_dist[cur_state] + 1;
            macro_bfs_prev[ns] = cur_state;
            macro_bfs_op[ns] = op;
            macro_bfs_queue[q_tail++] = ns;
        };

        while (q_head < q_tail) {
            int cur_state = macro_bfs_queue[q_head++];
            int cell = cur_state / 4;
            int dir = cur_state % 4;
            if (cell == to) {
                if (goal == -1 || macro_bfs_dist[cur_state] < macro_bfs_dist[goal] ||
                    (macro_bfs_dist[cur_state] == macro_bfs_dist[goal] && dir < goal % 4)) {
                    goal = cur_state;
                }
                continue;
            }
            if (goal != -1 && macro_bfs_dist[cur_state] >= macro_bfs_dist[goal]) continue;

            push_next(cur_state, cell, (dir + 1) % 4, 'R');
            push_next(cur_state, cell, (dir + 3) % 4, 'L');
            if (next_cell[cell][dir] != -1) push_next(cur_state, next_cell[cell][dir], dir, 'F');

            if (!macro_transition.empty()) {
                int next_state = macro_transition[cur_state];
                push_next(cur_state, next_state / 4, next_state % 4, 'P');
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
        end_dir = goal % 4;
        return ops;
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
    vector<vector<int>> first_dir;
    mutable vector<int> macro_bfs_dist;
    mutable vector<int> macro_bfs_prev;
    mutable vector<int> macro_bfs_queue;
    mutable vector<char> macro_bfs_op;

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

    void ensure_macro_bfs_buffers(int states) const {
        if ((int)macro_bfs_dist.size() == states) return;
        macro_bfs_dist.resize(states);
        macro_bfs_prev.resize(states);
        macro_bfs_queue.resize(states);
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

        macro = std::move(new_macro);
        macro_transition = solver.build_macro_transition(macro);
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

int score_p_macro_plan_fast(const Solver &solver, const vector<int> &order, vector<PMacroPlacement> plan, int limit, int cutoff = numeric_limits<int>::max() / 4) {
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
            if (def.offset + def.len <= (int)route.size()) define_here = true;
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

        out_len += 2;
        append_and_apply(definition_buttons, macro);

        macro = std::move(new_macro);
        macro_transition = solver.build_macro_transition(macro);
        has_macro = true;

        vector<char> rest = current_route(target);
        append_and_apply(rest, macro);
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

vector<char> anneal_multi_p_macro_reroute(const Solver &solver, const vector<int> &order, int limit, double time_limit_sec, int seed_rounds = 5, int seed_trials = 80, int max_defs = 10) {
    if (time_limit_sec <= 0.0) return solver.build_ops(order);

    const double start_time = get_time();
    vector<vector<char>> legs = build_basic_movement_legs(solver, order);
    vector<PMacroPlacement> candidates = collect_p_macro_placements(legs);
    if (candidates.empty()) return solver.build_ops(order);

    auto repair = [&](PMacroPlacement p) {
        p.leg = max(0, min(p.leg, (int)legs.size() - 1));
        int leg_size = (int)legs[p.leg].size();
        if (leg_size < 6) {
            p.offset = 0;
            p.len = 0;
            return p;
        }
        p.offset = max(0, min(p.offset, leg_size - 6));
        p.len = max(6, min(p.len, leg_size - p.offset));
        return p;
    };

    auto random_def = [&]() {
        if (rnd::get(100) < 85) return candidates[rnd::get((int)candidates.size())];
        PMacroPlacement p;
        p.leg = rnd::get((int)legs.size());
        int leg_size = (int)legs[p.leg].size();
        if (leg_size < 6) return candidates[rnd::get((int)candidates.size())];
        p.offset = rnd::get(leg_size - 5);
        p.len = 6 + rnd::get(min(160, leg_size - p.offset) - 5);
        return p;
    };

    auto eval = [&](const vector<PMacroPlacement> &plan, int cutoff = numeric_limits<int>::max() / 4) {
        return score_p_macro_plan_fast(solver, order, plan, limit, cutoff);
    };

    vector<PMacroPlacement> current_plan;
    int current_score = eval(current_plan);

    for (int round = 0; round < seed_rounds; ++round) {
        if (get_time() - start_time >= time_limit_sec) break;
        vector<PMacroPlacement> best_add_plan = current_plan;
        int best_add_score = current_score;
        for (int trial = 0; trial < seed_trials; ++trial) {
            if (get_time() - start_time >= time_limit_sec) break;
            vector<PMacroPlacement> next_plan = current_plan;
            next_plan.push_back(random_def());
            int score = eval(next_plan, best_add_score - 1);
            if (score < best_add_score) {
                best_add_score = score;
                best_add_plan = std::move(next_plan);
            }
        }
        if (best_add_score < current_score) {
            current_plan = std::move(best_add_plan);
            current_score = best_add_score;
        }
    }

    vector<PMacroPlacement> best_plan = current_plan;
    int best_score = current_score;

    static double log_table[65536];
    static bool initialized = false;
    if (!initialized) {
        for (int i = 0; i < 65536; ++i) log_table[i] = log((i + 0.5) / 65536.0);
        rnd::shuffle(log_table);
        initialized = true;
    }

    const double T0 = 60.0;
    const double T1 = 0.08;
    double heat = T0;
    int iter = 0;

    while (true) {
        double progress = (get_time() - start_time) / time_limit_sec;
        if (progress >= 1.0) break;
        if ((iter & 3) == 0) {
            heat = T0 * pow(T1 / T0, progress);
        }
        ++iter;

        vector<PMacroPlacement> next_plan = current_plan;
        int type = rnd::get(8);

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
        } else {
            rnd::shuffle(next_plan);
            while ((int)next_plan.size() > 1 && rnd::get(100) < 35) {
                next_plan.erase(next_plan.begin() + rnd::get((int)next_plan.size()));
            }
            while ((int)next_plan.size() < 8 && rnd::get(100) < 55) {
                next_plan.push_back(random_def());
            }
        }

        for (PMacroPlacement &p : next_plan) p = repair(p);
        next_plan.erase(remove_if(next_plan.begin(), next_plan.end(), [](const PMacroPlacement &p) {
                            return p.len < 6;
                        }),
                        next_plan.end());
        while ((int)next_plan.size() > max_defs) {
            next_plan.erase(next_plan.begin() + rnd::get((int)next_plan.size()));
        }

        double accept_margin = -heat * log_table[iter & 65535];
        int accept_cutoff = current_score + (int)floor(accept_margin);
        int next_score = eval(next_plan, accept_cutoff);
        int delta = next_score - current_score;
        if (delta <= accept_margin) {
            current_plan = std::move(next_plan);
            current_score = next_score;
            if (current_score < best_score) {
                best_score = current_score;
                best_plan = current_plan;
            }
        }
    }

    MacroContainStats stats;
    vector<char> answer = build_program_with_p_macro_plan(solver, order, best_plan, &stats);
    cerr << "p_macro_contain defs=" << stats.definitions
         << " contained_defs=" << stats.contained_definitions
         << " p_count=" << stats.contained_p_count
         << " saving=" << stats.encoded_saving
         << " chain=" << stats.max_contained_chain
         << " bonus=" << macro_containment_bonus(stats)
         << " iter=" << iter
         << '\n';
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
    vector<char> answer = anneal_multi_p_macro_reroute(solver, order, limit, time_limit_sec, 2, 24, 10);
    if (valid_program_for_limit(answer, limit)) return answer;

    vector<char> fallback = best_macro_compress(raw, limit, false);
    if (valid_program_for_limit(fallback, limit)) return fallback;
    return raw;
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    const double start_time = get_time();
    constexpr double TOTAL_TIME_LIMIT = 1.70;

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
