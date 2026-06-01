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

inline double nextf() {
    uint64_t v = 0x3ff0000000000000ULL | ((uint64_t)next() << 20);
    double d;
    memcpy(&d, &v, sizeof(double));
    return d - 1.0;
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

    vector<char> solve() const {
        return build_ops(build_greedy_order());
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
            if (can_move(cell, dir)) cell = neighbor(cell, dir);
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

        vector<int> dist(states, -1), prev_state(states, -1);
        vector<char> prev_op(states, 0);
        queue<int> q;

        int start = sid(from, from_dir);
        dist[start] = 0;
        q.push(start);

        auto push_next = [&](int cur_state, int next_cell, int next_dir, char op) {
            int ns = sid(next_cell, next_dir);
            if (ns == cur_state || dist[ns] != -1) return;
            dist[ns] = dist[cur_state] + 1;
            prev_state[ns] = cur_state;
            prev_op[ns] = op;
            q.push(ns);
        };

        while (!q.empty()) {
            int cur_state = q.front();
            q.pop();
            int cell = cur_state / 4;
            int dir = cur_state % 4;

            push_next(cur_state, cell, (dir + 1) % 4, 'R');
            push_next(cur_state, cell, (dir + 3) % 4, 'L');
            if (can_move(cell, dir)) push_next(cur_state, neighbor(cell, dir), dir, 'F');

            if (!macro_transition.empty()) {
                int next_state = macro_transition[cur_state];
                push_next(cur_state, next_state / 4, next_state % 4, 'P');
            }
        }

        int goal = -1;
        for (int dir = 0; dir < 4; ++dir) {
            int s = sid(to, dir);
            if (dist[s] != -1 && (goal == -1 || dist[s] < dist[goal])) goal = s;
        }

        vector<char> ops;
        if (goal == -1) {
            return basic_move_ops(from, from_dir, to, end_cell, end_dir);
        }

        for (int cur = goal; cur != start; cur = prev_state[cur]) {
            ops.push_back(prev_op[cur]);
        }
        reverse(ops.begin(), ops.end());
        end_cell = goal / 4;
        end_dir = goal % 4;
        return ops;
    }

    vector<char> macro_move_ops(int from, int from_dir, int to, const vector<char> &macro, int &end_cell, int &end_dir) const {
        vector<int> macro_transition = build_macro_transition(macro);
        return macro_move_ops_with_transition(from, from_dir, to, macro_transition, end_cell, end_dir);
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
    vector<vector<int>> first_dir;

    int id(Pos p) const {
        return p.r * in.N + p.c;
    }

    Pos pos(int cell) const {
        return Pos{cell / in.N, cell % in.N};
    }

    bool can_move(int cell, int dir) const {
        Pos p = pos(cell);
        int nr = p.r + DR[dir];
        int nc = p.c + DC[dir];
        if (nr < 0 || nr >= in.N || nc < 0 || nc >= in.N) return false;

        if (dir == UP) return in.h[p.r - 1][p.c] == '0';
        if (dir == DOWN) return in.h[p.r][p.c] == '0';
        if (dir == LEFT) return in.v[p.r][p.c - 1] == '0';
        return in.v[p.r][p.c] == '0';
    }

    int neighbor(int cell, int dir) const {
        Pos p = pos(cell);
        return id(Pos{p.r + DR[dir], p.c + DC[dir]});
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

    vector<int> path_dirs(int from, int to) const {
        vector<int> path;
        int cur = from;
        while (cur != to) {
            int dir = first_dir[cur][to];
            if (dir == -1) break;
            path.push_back(dir);
            cur = neighbor(cur, dir);
        }
        return path;
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
        vector<int> path = path_dirs(from, to);
        for (int next_dir : path) {
            append_turns(next_dir, dir, ops);
            ops.push_back('F');
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

int macro_aware_score(const Solver &solver, const vector<int> &order, bool fast) {
    constexpr int INF = 1000000000;
    vector<char> raw = solver.build_ops(order);
    if ((int)raw.size() > solver.time_limit()) {
        return INF / 2 + (int)raw.size() - solver.time_limit();
    }
    vector<char> encoded = best_macro_compress(raw, solver.time_limit(), fast);
    return (int)encoded.size();
}

vector<int> anneal_order_with_macro(const Solver &solver, const vector<int> &initial_order, double time_limit_sec) {
    const int m = (int)initial_order.size();
    if (m < 2 || time_limit_sec <= 0.0) return initial_order;

    vector<int> current_order = initial_order;
    vector<int> best_order = initial_order;
    int current_score = macro_aware_score(solver, current_order, true);
    int best_score = current_score;

    static double log_table[65536];
    static bool initialized = false;
    if (!initialized) {
        for (int i = 0; i < 65536; ++i) {
            log_table[i] = log((i + 0.5) / 65536.0);
        }
        rnd::shuffle(log_table);
        initialized = true;
    }

    const double start = get_time();
    const double T0 = 18.0;
    const double T1 = 0.05;
    double heat = T0;
    int iter = 0;

    while (true) {
        if ((iter & 3) == 0) {
            double progress = (get_time() - start) / time_limit_sec;
            if (progress >= 1.0) break;
            heat = T0 * pow(T1 / T0, progress);
        }
        ++iter;

        vector<int> next_order = current_order;
        int type = rnd::get(3);

        if (type == 0) {
            int i = rnd::get(m);
            int j = rnd::get(m - 1);
            if (j >= i) ++j;
            swap(next_order[i], next_order[j]);
        } else if (type == 1) {
            int l = rnd::get(m);
            int r = rnd::get(m);
            if (l > r) swap(l, r);
            if (l == r) continue;
            reverse(next_order.begin() + l, next_order.begin() + r + 1);
        } else {
            int i = rnd::get(m);
            int j = rnd::get(m);
            if (i == j) continue;
            int x = next_order[i];
            next_order.erase(next_order.begin() + i);
            if (j > i) --j;
            next_order.insert(next_order.begin() + j, x);
        }

        int next_score = macro_aware_score(solver, next_order, true);
        int delta = next_score - current_score;
        double accept_margin = -heat * log_table[iter & 65535];

        if (delta <= accept_margin) {
            current_order = std::move(next_order);
            current_score = next_score;
            if (current_score < best_score) {
                best_score = current_score;
                best_order = current_order;
            }
        }
    }

    return best_order;
}

struct MacroAction {
    int start = 0;
    int len = 0;
};

vector<MacroAction> collect_macro_action_candidates(const vector<char> &base, int max_len_cap = 140, int keep = 4000) {
    const int n = (int)base.size();
    string s(base.begin(), base.end());
    const int max_len = min(n / 2, max_len_cap);
    if (max_len < 2) return {};

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

    struct ScoredAction {
        int score = 0;
        MacroAction action;
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

    vector<ScoredAction> scored;
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

                for (int idx = 0; idx + 1 < m; ++idx) {
                    int last = positions[idx];
                    int future_count = 0;
                    for (int j = idx + 1; j < m; ++j) {
                        if (positions[j] < last + len) continue;
                        ++future_count;
                        last = positions[j];
                    }

                    int saving = future_count * (len - 1) - 2;
                    if (saving > 0) {
                        scored.push_back(ScoredAction{saving, MacroAction{positions[idx], len}});
                    }
                }
            }

            l = r;
        }
    }

    sort(scored.begin(), scored.end(), [](const ScoredAction &lhs, const ScoredAction &rhs) {
        if (lhs.score != rhs.score) return lhs.score > rhs.score;
        if (lhs.action.len != rhs.action.len) return lhs.action.len > rhs.action.len;
        return lhs.action.start < rhs.action.start;
    });

    vector<MacroAction> candidates;
    candidates.reserve(min(keep, (int)scored.size()));
    set<pair<int, int>> seen;
    for (const ScoredAction &x : scored) {
        if ((int)candidates.size() >= keep) break;
        pair<int, int> key{x.action.start, x.action.len};
        if (seen.insert(key).second) candidates.push_back(x.action);
    }
    return candidates;
}

vector<MacroAction> normalize_macro_actions(vector<MacroAction> actions, int n) {
    sort(actions.begin(), actions.end(), [](const MacroAction &lhs, const MacroAction &rhs) {
        if (lhs.start != rhs.start) return lhs.start < rhs.start;
        return lhs.len > rhs.len;
    });

    vector<MacroAction> normalized;
    int last_end = 0;
    for (MacroAction action : actions) {
        action.start = max(0, min(action.start, n));
        action.len = max(1, min(action.len, n - action.start));
        if (action.len <= 1) continue;
        if (action.start < last_end) continue;
        normalized.push_back(action);
        last_end = action.start + action.len;
    }
    return normalized;
}

vector<char> encode_macro_program_from_actions(const vector<char> &base, const vector<MacroAction> &actions) {
    const int n = (int)base.size();
    string s(base.begin(), base.end());
    vector<MacroAction> normalized = normalize_macro_actions(actions, n);
    vector<char> encoded;
    encoded.reserve(n);

    string last_macro;
    bool has_macro = false;

    auto append_segment_using_last_macro = [&](const string &segment) {
        int p = 0;
        while (p < (int)segment.size()) {
            if (has_macro && !last_macro.empty() && p + (int)last_macro.size() <= (int)segment.size() &&
                segment.compare(p, last_macro.size(), last_macro) == 0) {
                encoded.push_back('P');
                p += (int)last_macro.size();
            } else {
                encoded.push_back(segment[p]);
                ++p;
            }
        }
    };

    int action_idx = 0;
    int i = 0;
    while (i < n) {
        if (action_idx < (int)normalized.size() && normalized[action_idx].start == i) {
            int len = normalized[action_idx].len;
            string segment = s.substr(i, len);
            encoded.push_back('M');
            append_segment_using_last_macro(segment);
            encoded.push_back('M');
            last_macro = segment;
            has_macro = true;
            i += len;
            ++action_idx;
        } else if (has_macro && !last_macro.empty() && i + (int)last_macro.size() <= n &&
                   s.compare(i, last_macro.size(), last_macro) == 0) {
            encoded.push_back('P');
            i += (int)last_macro.size();
        } else {
            encoded.push_back(base[i]);
            ++i;
        }
    }

    return encoded;
}

int score_macro_program_actions(const vector<char> &base, const vector<MacroAction> &actions, int limit) {
    vector<char> encoded = encode_macro_program_from_actions(base, actions);
    if (!same_expansion(encoded, base)) return 1000000000;
    if ((int)encoded.size() > limit) return 500000000 + (int)encoded.size() - limit;
    return (int)encoded.size();
}

vector<char> anneal_macro_program_direct(const vector<char> &base, int limit, double time_limit_sec) {
    const int n = (int)base.size();
    if (n < 6 || time_limit_sec <= 0.0) return base;

    vector<MacroAction> candidates = collect_macro_action_candidates(base);
    if (candidates.empty()) return base;

    auto random_action = [&]() -> MacroAction {
        if (!candidates.empty() && rnd::get(100) < 90) {
            return candidates[rnd::get((int)candidates.size())];
        }
        int start = rnd::get(n - 1);
        int max_len = min(140, n - start);
        int len = 2 + rnd::get(max(1, max_len - 1));
        return MacroAction{start, len};
    };

    vector<MacroAction> current_actions;
    int current_score = score_macro_program_actions(base, current_actions, limit);

    for (int i = 0; i < min(600, (int)candidates.size()); ++i) {
        vector<MacroAction> next_actions = current_actions;
        next_actions.push_back(candidates[i]);
        int next_score = score_macro_program_actions(base, next_actions, limit);
        if (next_score < current_score) {
            current_actions = std::move(next_actions);
            current_score = next_score;
        }
    }

    vector<MacroAction> best_actions = current_actions;
    int best_score = current_score;

    static double log_table[65536];
    static bool initialized = false;
    if (!initialized) {
        for (int i = 0; i < 65536; ++i) {
            log_table[i] = log((i + 0.5) / 65536.0);
        }
        rnd::shuffle(log_table);
        initialized = true;
    }

    const double start_time = get_time();
    const double T0 = 28.0;
    const double T1 = 0.04;
    double heat = T0;
    int iter = 0;

    while (true) {
        if ((iter & 7) == 0) {
            double progress = (get_time() - start_time) / time_limit_sec;
            if (progress >= 1.0) break;
            heat = T0 * pow(T1 / T0, progress);
        }
        ++iter;

        vector<MacroAction> next_actions = current_actions;
        int type = rnd::get(6);

        if (type == 0 || next_actions.empty()) {
            next_actions.push_back(random_action());
        } else if (type == 1) {
            int idx = rnd::get((int)next_actions.size());
            next_actions.erase(next_actions.begin() + idx);
        } else if (type == 2) {
            int idx = rnd::get((int)next_actions.size());
            next_actions[idx] = random_action();
        } else if (type == 3) {
            int idx = rnd::get((int)next_actions.size());
            int delta = rnd::range(-20, 21);
            next_actions[idx].start = max(0, min(n - 2, next_actions[idx].start + delta));
            next_actions[idx].len = max(2, min(next_actions[idx].len, n - next_actions[idx].start));
        } else if (type == 4) {
            int idx = rnd::get((int)next_actions.size());
            int delta = rnd::range(-24, 25);
            next_actions[idx].len = max(2, min(180, next_actions[idx].len + delta));
            next_actions[idx].len = min(next_actions[idx].len, n - next_actions[idx].start);
        } else {
            int remove_count = min((int)next_actions.size(), 1 + rnd::get(3));
            for (int k = 0; k < remove_count && !next_actions.empty(); ++k) {
                int idx = rnd::get((int)next_actions.size());
                next_actions.erase(next_actions.begin() + idx);
            }
        }

        while ((int)next_actions.size() > 80) {
            int idx = rnd::get((int)next_actions.size());
            next_actions.erase(next_actions.begin() + idx);
        }

        int next_score = score_macro_program_actions(base, next_actions, limit);
        int delta = next_score - current_score;
        double accept_margin = -heat * log_table[iter & 65535];
        if (delta <= accept_margin) {
            current_actions = std::move(next_actions);
            current_score = next_score;
            if (current_score < best_score) {
                best_score = current_score;
                best_actions = current_actions;
            }
        }
    }

    vector<char> encoded = encode_macro_program_from_actions(base, best_actions);
    return encoded;
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

vector<char> build_program_with_p_macro(const Solver &solver, const vector<int> &order, PMacroPlacement placement) {
    vector<char> program;
    vector<char> macro;
    vector<int> macro_transition;
    bool registered = false;
    int cell = 0;
    int dir = 1;
    int leg = 0;

    auto append_ops = [&](const vector<char> &ops) {
        program.insert(program.end(), ops.begin(), ops.end());
        solver.apply_basic_ops(ops, cell, dir);
    };

    auto move_to = [&](int target) {
        if (!registered) {
            int end_cell, end_dir;
            vector<char> basic = solver.basic_move_ops(cell, dir, target, end_cell, end_dir);

            if (leg == placement.leg && placement.offset + placement.len <= (int)basic.size()) {
                vector<char> prefix(basic.begin(), basic.begin() + placement.offset);
                append_ops(prefix);

                macro.assign(basic.begin() + placement.offset, basic.begin() + placement.offset + placement.len);
                program.push_back('M');
                program.insert(program.end(), macro.begin(), macro.end());
                program.push_back('M');
                solver.apply_basic_ops(macro, cell, dir);
                macro_transition = solver.build_macro_transition(macro);
                registered = true;

                int next_cell, next_dir;
                vector<char> rest = solver.macro_move_ops_with_transition(cell, dir, target, macro_transition, next_cell, next_dir);
                program.insert(program.end(), rest.begin(), rest.end());
                for (char op : rest) {
                    if (op == 'P') solver.apply_basic_ops(macro, cell, dir);
                    else solver.apply_basic_op(op, cell, dir);
                }
            } else {
                append_ops(basic);
            }
        } else {
            int next_cell, next_dir;
            vector<char> ops = solver.macro_move_ops_with_transition(cell, dir, target, macro_transition, next_cell, next_dir);
            program.insert(program.end(), ops.begin(), ops.end());
            for (char op : ops) {
                if (op == 'P') solver.apply_basic_ops(macro, cell, dir);
                else solver.apply_basic_op(op, cell, dir);
            }
        }
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

vector<char> anneal_p_macro_reroute(const Solver &solver, const vector<int> &order, int limit, double time_limit_sec) {
    vector<vector<char>> legs = build_basic_movement_legs(solver, order);
    vector<PMacroPlacement> candidates = collect_p_macro_placements(legs);
    if (candidates.empty()) return solver.build_ops(order);

    auto eval = [&](PMacroPlacement placement) {
        vector<char> program = build_program_with_p_macro(solver, order, placement);
        return score_p_macro_program(program, limit);
    };

    PMacroPlacement current = candidates[rnd::get((int)candidates.size())];
    int current_score = eval(current);
    PMacroPlacement best = current;
    int best_score = current_score;

    int warmup = min(150, (int)candidates.size());
    for (int i = 0; i < warmup; ++i) {
        PMacroPlacement cand = candidates[rnd::get((int)candidates.size())];
        int score = eval(cand);
        if (score < best_score) {
            best = cand;
            best_score = score;
            current = cand;
            current_score = score;
        }
    }

    static double log_table[65536];
    static bool initialized = false;
    if (!initialized) {
        for (int i = 0; i < 65536; ++i) log_table[i] = log((i + 0.5) / 65536.0);
        rnd::shuffle(log_table);
        initialized = true;
    }

    const double start_time = get_time();
    const double T0 = 35.0;
    const double T1 = 0.05;
    double heat = T0;
    int iter = 0;

    while (true) {
        if ((iter & 3) == 0) {
            double progress = (get_time() - start_time) / time_limit_sec;
            if (progress >= 1.0) break;
            heat = T0 * pow(T1 / T0, progress);
        }
        ++iter;

        PMacroPlacement next = current;
        int type = rnd::get(5);
        if (type == 0) {
            next = candidates[rnd::get((int)candidates.size())];
        } else if (type == 1) {
            next.offset += rnd::range(-8, 9);
        } else if (type == 2) {
            next.len += rnd::range(-12, 13);
        } else if (type == 3) {
            next.leg += rnd::range(-3, 4);
        } else {
            next.leg = rnd::get((int)legs.size());
            next.offset = legs[next.leg].empty() ? 0 : rnd::get((int)legs[next.leg].size());
            next.len = 6 + rnd::get(115);
        }

        next.leg = max(0, min(next.leg, (int)legs.size() - 1));
        next.offset = max(0, min(next.offset, max(0, (int)legs[next.leg].size() - 1)));
        next.len = max(6, min(next.len, (int)legs[next.leg].size() - next.offset));
        if (next.len < 6) continue;

        int next_score = eval(next);
        int delta = next_score - current_score;
        double accept_margin = -heat * log_table[iter & 65535];
        if (delta <= accept_margin) {
            current = next;
            current_score = next_score;
            if (current_score < best_score) {
                best = current;
                best_score = current_score;
            }
        }
    }

    return build_program_with_p_macro(solver, order, best);
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    Input in = read_input();
    Solver solver(in);
    vector<int> greedy_order = solver.build_greedy_order();
    vector<int> annealed_order = anneal_order_with_macro(solver, greedy_order, 0.20);

    vector<char> annealed_raw = solver.build_ops(annealed_order);
    vector<char> answer = anneal_p_macro_reroute(solver, annealed_order, in.T, 0.25);
    for (char op : answer) {
        cout << op << '\n';
    }
    return 0;
}
