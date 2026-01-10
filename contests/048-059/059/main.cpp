#include <bits/stdc++.h>
#include <atcoder/all>
using namespace std;
using ll = long long;
using ld = long double;
using mint = atcoder::modint998244353;
using vl = vector<ll>;
using vvl = vector<vl>;
using vvvl = vector<vvl>;
using vi = vector<int>;
using vvi = vector<vi>;
using vvvi = vector<vvi>;
using vb = vector<bool>;
using vvb = vector<vb>;
using vvvb = vector<vvb>;
using vs = vector<string>;
using vvs = vector<vs>;
using pl = pair<ll, ll>;
using vpl = vector<pl>;
#define rep(i, a, b) for (ll i = (a); i < (ll)(b); i++)
#define all(v) v.begin(), v.end()

template <typename T>
inline bool chmax(T &a, const T &b) {
    if (a < b) {
        a = b;
        return true;
    }
    return false;
}

template <typename T>
inline bool chmin(T &a, const T &b) {
    if (a > b) {
        a = b;
        return true;
    }
    return false;
}

ll N = 20;
vvl a(20, vl(20));

void solve() {
    vector<char> moves;
    ll i = 0, j = 0;
    stack<ll> st;
    vvl now = a;
    ll count = 0;
    ll turn = 0;

    // 目的地リスト（順番に消化していく）
    deque<pl> target_queue;

    while (turn < 16000 && count < 200) {
        // 現在の目的地がある場合、そこへ向かう
        if (!target_queue.empty()) {
            pl target = target_queue.front();
            
            if (target != pl{i, j}) {
                // 移動処理
                if (target.first < i) { moves.push_back('U'); i--; }
                else if (target.first > i) { moves.push_back('D'); i++; }
                else if (target.second < j) { moves.push_back('L'); j--; }
                else if (target.second > j) { moves.push_back('R'); j++; }
                turn++;
                continue;
            } else {
                // 目的地に到着
                target_queue.pop_front();
                moves.push_back('Z');
                if (!st.empty() && st.top() == now[i][j]) {
                    st.pop();
                    count++;
                } else {
                    st.push(now[i][j]);
                }
                now[i][j] = -1;
                turn++;
                continue;
            }
        }

        // 目的地リストが空の場合、新しい経路を計画する
        if (target_queue.empty()) {
            ll min_dist = 1e9;
            pl a1 = {-1, -1};
            
            // 1. 最も近いカード(A1)を探す
            rep(ni, 0, N) rep(nj, 0, N) {
                if (now[ni][nj] != -1) {
                    ll dist = abs(i - ni) + abs(j - nj);
                    if (dist < min_dist) {
                        min_dist = dist;
                        a1 = {ni, nj};
                    }
                }
            }

            if (a1 == pl{-1, -1}) break; // カードがない

            // 2. A1のペア(A2)を探す
            pl a2 = {-1, -1};
            ll val_a = now[a1.first][a1.second];
            rep(ni, 0, N) rep(nj, 0, N) {
                if ((ni != a1.first || nj != a1.second) && now[ni][nj] == val_a) {
                    a2 = {ni, nj};
                    break;
                }
            }

            // 3. A1からA2への経路計画（ネスト構造の構築）
            // 基本ルート: A1 -> A2
            target_queue.push_back(a1);
            
            // 4. A1とA2の矩形範囲内に、他のペア(B1, B2)がまるごと入っていないか探す
            ll r_min = min(a1.first, a2.first), r_max = max(a1.first, a2.first);
            ll c_min = min(a1.second, a2.second), c_max = max(a1.second, a2.second);

            vector<ll> nested_vals;
            rep(ni, r_min, r_max + 1) rep(nj, c_min, c_max + 1) {
                ll val_b = now[ni][nj];
                if (val_b != -1 && val_b != val_a) {
                    // ペアのもう片方もこの範囲内に隠れているか？
                    pl b1 = {ni, nj};
                    pl b2 = {-1, -1};
                    rep(mi, r_min, r_max + 1) rep(mj, c_min, c_max + 1) {
                        if ((mi != ni || mj != nj) && now[mi][mj] == val_b) {
                            b2 = {mi, mj};
                            break;
                        }
                    }
                    // ペアが両方とも矩形内にあり、まだリストに入れていなければ採用
                    if (b2 != pl{-1, -1}) {
                        bool already = false;
                        for(auto v : nested_vals) if(v == val_b) already = true;
                        if(!already) {
                            // A1 -> B1 -> B2 -> A2 の形にするためキューに入れる
                            // ※簡易化のため1つのペアだけネストさせる（複数入れる場合はソートが必要）
                            target_queue.push_back(b1);
                            target_queue.push_back(b2);
                            nested_vals.push_back(val_b);
                            
                            // 他のペアをこれ以上探さない（スタックが深くなりすぎるのを防ぐ）
                            if(nested_vals.size() >= 1) goto end_planning; 
                        }
                    }
                }
            }
            
            end_planning:
            target_queue.push_back(a2);
        }
    }

    // 出力
    for (char m : moves) cout << m << "\n";
}

int main() {
    if (!(cin >> N)) return 0;
    a.assign(N, vl(N));
    rep(i, 0, N) rep(j, 0, N) cin >> a[i][j];
    solve();
    return 0;
}