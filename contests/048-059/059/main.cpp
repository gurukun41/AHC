#include <bits/stdc++.h>
#include <atcoder/all>
using namespace std;
using ll = long long;
using pl = pair<ll, ll>;
using vvl = vector<vector<ll>>;

#define rep(i, a, b) for (ll i = (a); i < (ll)(b); i++)

ll N = 20;
vvl a(20, vector<ll>(20));

pl get_home(ll val) {
    return {val / 20, val % 20};
}

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

    auto find_nearest_empty = [&](ll si, ll sj) {
        ll min_d = 1e9;
        pl best = {-1, -1};
        rep(r, 0, N) rep(c, 0, N) {
            if (now[r][c] == -1) {
                ll d = abs(si - r) + abs(sj - c);
                if (r >= 10) d -= 2; 
                if (d < min_d) {
                    min_d = d;
                    best = {r, c};
                }
            }
        }
        return best;
    };

    while (turn < 16000 && count < 200) {
        if (st.empty()) {
            ll min_dist = 1e9;
            pl best_target = {-1, -1};
            rep(r, 0, N) rep(c, 0, N) {
                if (now[r][c] != -1) {
                    pl home = get_home(now[r][c]);
                    if (r == home.first && c == home.second) continue; 
                    ll dist = abs(cur_i - r) + abs(cur_j - c);
                    if (dist < min_dist) {
                        min_dist = dist;
                        best_target = {r, c};
                    }
                }
            }
            if (best_target.first == -1) break; 
            move_to(best_target.first, best_target.second);
            if (turn >= 16000) break;
            moves.push_back('Z');
            st.push(now[cur_i][cur_j]);
            now[cur_i][cur_j] = -1;
            turn++;
        } else {
            ll val = st.top();
            pl home = get_home(val);
            if (now[home.first][home.second] == -1 || now[home.first][home.second] == val) {
                move_to(home.first, home.second);
            } else {
                ll occupant = now[home.first][home.second];
                pl occ_home = get_home(occupant);
                if (home.first == occ_home.first && home.second == occ_home.second) {
                    pl alt = find_nearest_empty(cur_i, cur_j);
                    move_to(alt.first, alt.second);
                } else {
                    move_to(home.first, home.second);
                }
            }
            if (turn >= 16000) break;
            if (now[cur_i][cur_j] == -1) {
                moves.push_back('X');
                now[cur_i][cur_j] = val;
                st.pop();
            } else if (now[cur_i][cur_j] == val) {
                moves.push_back('Z');
                now[cur_i][cur_j] = -1;
                st.pop();
                count++;
            } else {
                moves.push_back('Z');
                st.push(now[cur_i][cur_j]);
                now[cur_i][cur_j] = -1;
            }
            turn++;
        }
    }
    for (char m : moves) cout << m << "\n";
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    if (!(cin >> N)) return 0;
    a.assign(N, vector<ll>(N));
    rep(i, 0, N) rep(j, 0, N) cin >> a[i][j];
    solve();
    return 0;
}