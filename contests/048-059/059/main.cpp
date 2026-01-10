#include <bits/stdc++.h>
#include <atcoder/all>
using namespace std;
using ll = long long;
using pl = pair<ll, ll>;
using vvl = vector<vector<ll>>;

#define rep(i, a, b) for (ll i = (a); i < (ll)(b); i++)

ll N = 20;
vvl a(20, vector<ll>(20));
pl card_home[200]; // 各数字の割り当てられたホーム
bool is_cleared[200];

void solve() {
    vector<char> moves;
    ll cur_i = 0, cur_j = 0;
    stack<ll> st;
    vvl now = a; 
    ll count = 0;
    ll turn = 0;

    auto move_to = [&](ll ti, ll tj) {
        while (cur_i != ti || cur_j != tj) {
            if (cur_i < ti) { moves.push_back('D'); cur_i++; }
            else if (cur_i > ti) { moves.push_back('U'); cur_i--; }
            else if (cur_j < tj) { moves.push_back('R'); cur_j++; }
            else if (cur_j > tj) { moves.push_back('L'); cur_j--; }
            turn++;
            if (turn >= 16000) return;
        }
    };

    auto action_z = [&]() {
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
    };

    auto action_x = [&]() {
        if (turn >= 16000 || st.empty()) return;
        moves.push_back('X');
        now[cur_i][cur_j] = st.top();
        st.pop();
        turn++;
    };

    // --- 1. オープニング・フェーズ: 特定エリア内に両方あるペアを消す ---
    auto clear_pairs_in_range = [&](int r_start, int r_end) {
        while (true) {
            int target_val = -1;
            ll min_d = 1e9;
            rep(v, 0, 200) {
                if (is_cleared[v]) continue;
                vector<pl> pos;
                rep(r, 0, N) rep(c, 0, N) if (now[r][c] == v) pos.push_back({r, c});
                if (pos.size() < 2) continue;
                if (pos[0].first >= r_start && pos[0].first < r_end && 
                    pos[1].first >= r_start && pos[1].first < r_end) {
                    ll d = abs(cur_i - pos[0].first) + abs(cur_j - pos[0].second);
                    if (d < min_d) { min_d = d; target_val = v; }
                }
            }
            if (target_val == -1) break;
            vector<pl> pos;
            rep(r, 0, N) rep(c, 0, N) if (now[r][c] == target_val) pos.push_back({r, c});
            move_to(pos[0].first, pos[0].second); action_z();
            move_to(pos[1].first, pos[1].second); action_z();
        }
    };

    clear_pairs_in_range(0, 5);   // 上
    clear_pairs_in_range(5, 15);  // 中央
    clear_pairs_in_range(15, 20); // 下

    // --- 2. ホームの決定 ---
    rep(v, 0, 200) card_home[v] = {-1, -1};
    rep(r, 5, 15) rep(c, 0, 20) {
        if (now[r][c] != -1) {
            ll v = now[r][c];
            if (card_home[v].first == -1) card_home[v] = {r, c};
        }
    }
    vector<pl> empty_slots;
    rep(r, 5, 15) rep(c, 0, 20) if (now[r][c] == -1) empty_slots.push_back({r, c});
    int slot_ptr = 0;
    rep(v, 0, 200) {
        if (is_cleared[v]) continue;
        if (card_home[v].first == -1 && slot_ptr < (int)empty_slots.size()) {
            card_home[v] = empty_slots[slot_ptr++];
        }
    }

    // --- 3. 整理フェーズ (一括回収と一括配置) ---
    auto distribute_stack = [&]() {
        while (!st.empty() && turn < 16000) {
            ll v = st.top();
            pl h = card_home[v];
            if (h.first == -1) { st.pop(); continue; }

            move_to(h.first, h.second);
            if (now[cur_i][cur_j] == -1) {
                action_x();
            } else if (now[cur_i][cur_j] == v) {
                action_z();
            } else {
                // ホームに居座っている別のカードを拾って後回しにする
                action_z();
            }
        }
    };

    // 上部エリア(0-4)を一括回収（蛇行移動で往復を削減）
    rep(r, 0, 5) {
        if (r % 2 == 0) {
            rep(c, 0, 20) {
                if (now[r][c] != -1) {
                    move_to(r, c);
                    action_z();
                }
            }
        } else {
            for (int c = 19; c >= 0; c--) {
                if (now[r][c] != -1) {
                    move_to(r, c);
                    action_z();
                }
            }
        }
    }
    distribute_stack(); // 中央へ移動して配置

    // 下部エリア(15-19)を一括回収（蛇行移動で往復を削減）
    for (int r = 19; r >= 15; r--) {
        // 行のインデックス（19から数えて何行目か）で向きを決める
        if ((19 - r) % 2 == 0) {
            rep(c, 0, 20) {
                if (now[r][c] != -1) {
                    move_to(r, c);
                    action_z();
                }
            }
        } else {
            for (int c = 19; c >= 0; c--) {
                if (now[r][c] != -1) {
                    move_to(r, c);
                    action_z();
                }
            }
        }
    }
    distribute_stack(); // 中央へ移動して配置

    // 最後に中央エリアに残っている「ホームにいないカード」を最終整理
    while (turn < 16000 && count < 200) {
        if (st.empty()) {
            ll min_d = 1e9; pl target = {-1, -1};
            rep(r, 5, 15) rep(c, 0, 20) {
                if (now[r][c] == -1) continue;
                ll v = now[r][c];
                if (card_home[v] != pl{r, c}) {
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

    for (char m : moves) cout << m << "\n";
}

int main() {
    ios::sync_with_stdio(false); cin.tie(nullptr);
    if (!(cin >> N)) return 0;
    a.assign(N, vector<ll>(N));
    rep(i, 0, N) rep(j, 0, N) cin >> a[i][j];
    solve();
}