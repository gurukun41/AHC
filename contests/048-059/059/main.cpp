#include <bits/stdc++.h>
#include <atcoder/all>
using namespace std;
using ll = long long;
using pl = pair<ll, ll>;
using vvl = vector<vector<ll>>;

#define rep(i, a, b) for (ll i = (a); i < (ll)(b); i++)
#define all(v) v.begin(), v.end()

ll N = 20;
vvl board(20, vector<ll>(20));
pl card_home[200];
bool is_cleared[200];

struct Solver {
    vvl now;
    ll cur_i, cur_j;
    ll turn, count;
    vector<char> moves;
    stack<ll> st;

    Solver(const vvl& initial_board) : now(initial_board), cur_i(0), cur_j(0), turn(0), count(0) {
        fill(is_cleared, is_cleared + 200, false);
        rep(i, 0, 200) card_home[i] = {-1, -1};
    }

    void move_to(ll ti, ll tj) {
        while ((cur_i != ti || cur_j != tj) && turn < 16000) {
            if (cur_i < ti) { moves.push_back('D'); cur_i++; }
            else if (cur_i > ti) { moves.push_back('U'); cur_i--; }
            else if (cur_j < tj) { moves.push_back('R'); cur_j++; }
            else if (cur_j > tj) { moves.push_back('L'); cur_j--; }
            turn++;
        }
    }

    void action_z() {
        if (turn >= 16000) return;
        moves.push_back('Z');
        ll val = now[cur_i][cur_j];
        if (!st.empty() && st.top() == val) {
            st.pop();
            count++;
            is_cleared[val] = true;
        } else {
            st.push(val);
        }
        now[cur_i][cur_j] = -1;
        turn++;
    }

    void action_x() {
        if (turn >= 16000 || st.empty()) return;
        moves.push_back('X');
        now[cur_i][cur_j] = st.top();
        st.pop();
        turn++;
    }

    // スタックのカードをホームに配置
    void distribute_stack() {
        while (!st.empty() && turn < 16000) {
            ll v = st.top();
            pl h = card_home[v];
            if (h.first == -1) { st.pop(); continue; }
            move_to(h.first, h.second);
            if (now[cur_i][cur_j] == -1) action_x();
            else action_z(); 
        }
    }

    // A-B-B-A や A-B-C-C-B-A のような入れ子構造で再帰的にペアを回収
    void recursive_clear_step(int target_val, int r_start, int r_end) {
        if (is_cleared[target_val]) return;

        vector<pl> pos;
        rep(r, r_start, r_end) rep(c, 0, 20) if (now[r][c] == target_val) pos.push_back({r, c});
        if (pos.size() != 2) return;

        pl p1 = pos[0], p2 = pos[1];
        if (abs(cur_i - p2.first) + abs(cur_j - p2.second) < abs(cur_i - p1.first) + abs(cur_j - p1.second)) {
            swap(p1, p2);
        }

        move_to(p1.first, p1.second);
        action_z();

        while (true) {
            int nested_val = -1;
            ll min_d = 1e9;
            int rs = min(p1.first, p2.first), re = max(p1.first, p2.first);
            int cs = min(p1.second, p2.second), ce = max(p1.second, p2.second);

            rep(v, 0, 200) {
                if (is_cleared[v] || v == target_val) continue;
                vector<pl> n_pos;
                rep(r, rs, re + 1) rep(c, cs, ce + 1) if (now[r][c] == v) n_pos.push_back({r, c});
                if (n_pos.size() == 2) {
                    ll d = abs(cur_i - n_pos[0].first) + abs(cur_j - n_pos[0].second);
                    if (d < min_d) { min_d = d; nested_val = v; }
                }
            }
            if (nested_val == -1) break;
            recursive_clear_step(nested_val, r_start, r_end);
        }

        move_to(p2.first, p2.second);
        action_z();
    }

    void clear_pairs_in_range(int r_start, int r_end) {
        if (r_start >= r_end) return;
        while (true) {
            int best_val = -1;
            ll min_d = 1e9;
            rep(v, 0, 200) {
                if (is_cleared[v]) continue;
                vector<pl> pos;
                rep(r, r_start, r_end) rep(c, 0, 20) if (now[r][c] == v) pos.push_back({r, c});
                if (pos.size() == 2) {
                    ll d = min(abs(cur_i - pos[0].first) + abs(cur_j - pos[0].second),
                               abs(cur_i - pos[1].first) + abs(cur_j - pos[1].second));
                    if (d < min_d) { min_d = d; best_val = v; }
                }
            }
            if (best_val == -1) break;
            recursive_clear_step(best_val, r_start, r_end);
        }
    }

    void compress_area(int r_start, int r_end, int storage_r_start) {
        if (r_start >= r_end || storage_r_start >= 20) return;

        // 1. ホームの割り当て
        rep(r, r_start, r_end) rep(c, 0, 20) {
            ll v = now[r][c];
            if (v == -1 || is_cleared[v]) continue;
            if (card_home[v].first != -1) continue;

            pl partner = {-1, -1};
            rep(nr, storage_r_start, 20) rep(nc, 0, 20) {
                if (now[nr][nc] == v && (nr != r || nc != c)) {
                    partner = {nr, nc}; break;
                }
            }

            if (partner.first != -1) {
                card_home[v] = partner;
            } else {
                bool found = false;
                for (int nr = 19; nr >= storage_r_start; nr--) {
                    for (int nc = 19; nc >= 0; nc--) {
                        if (now[nr][nc] == -1) {
                            bool occupied = false;
                            rep(i, 0, 200) if (card_home[i] == pl{nr, nc}) occupied = true;
                            if (!occupied) { card_home[v] = {nr, nc}; found = true; break; }
                        }
                    }
                    if (found) break;
                }
            }
        }

        // 2. 蛇行回収
        rep(r, r_start, r_end) {
            if (r % 2 == 0) {
                rep(c, 0, 20) if (now[r][c] != -1) { move_to(r, c); action_z(); }
            } else {
                for (int c = 19; c >= 0; c--) if (now[r][c] != -1) { move_to(r, c); action_z(); }
            }
        }
        distribute_stack();
    }

    void final_clear() {
        while (turn < 16000 && count < 200) {
            if (st.empty()) {
                ll min_d = 1e9; pl target = {-1, -1};
                rep(r, 0, 20) rep(c, 0, 20) {
                    if (now[r][c] != -1) {
                        ll d = abs(cur_i - r) + abs(cur_j - c);
                        if (d < min_d) { min_d = d; target = {r, c}; }
                    }
                }
                if (target.first == -1) break;
                move_to(target.first, target.second);
                action_z();
            } else {
                distribute_stack();
            }
        }
    }
};

void solve() {
    Solver s(board);
    vector<pair<int, int>> ranges = {
        {0, 6}, {6, 10}, {10, 13}, {13, 15}, {15, 16}, {16, 20}
    };

    for (int i = 0; i < 5; i++) {
        int r_start = ranges[i].first;
        int r_end = ranges[i].second;

        // --- フェーズ0: 「上(現在)・中央・下」の3階層で掃除 ---
        // 1. 現在の階層 (Top)
        s.clear_pairs_in_range(r_start, r_end);
        
        // 残りの領域を「中央」と「下」に動的に分割
        int remaining_height = 20 - r_end;
        if (remaining_height > 0) {
            int mid_r = r_end + remaining_height / 2;
            // 2. 中央の掃除
            s.clear_pairs_in_range(r_end, mid_r);
            // 3. 下の掃除
            s.clear_pairs_in_range(mid_r, 20);
        }

        // --- フェーズ1: 現在の階層を下のストレージへ圧縮 ---
        s.compress_area(r_start, r_end, r_end);
    }

    s.final_clear();
    for (char m : s.moves) cout << m << "\n";
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    if (!(cin >> N)) return 0;
    board.assign(N, vector<ll>(N));
    rep(i, 0, N) rep(j, 0, N) cin >> board[i][j];
    solve();
}