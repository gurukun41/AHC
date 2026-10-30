#include <bits/stdc++.h>
using namespace std;
using ll = long long;
using Distance = vector<vector<ll>>;

ll tour_cost(const vector<int>& tour, const Distance& dist) {
    ll total = 0;
    int n = (int)tour.size();
    for (int i = 0; i < n; i++) total += dist[tour[i]][tour[(i + 1) % n]];
    return total;
}

// 閉路の区間 [l, r] を反転したときの「新コスト - 旧コスト」。
ll reversal_delta(const vector<int>& tour, int l, int r, const Distance& dist) {
    int n = (int)tour.size();
    if (l == 0 && r == n - 1) return 0; // 閉路全体の反転
    int a = tour[(l + n - 1) % n], b = tour[l];
    int c = tour[r], d = tour[(r + 1) % n];
    return dist[a][c] + dist[b][d] - dist[a][b] - dist[c][d];
}

int main() {
    // 対称な距離行列。辺を逆向きに通ってもコストは同じ。
    Distance dist = {{0, 1, 2, 1}, {1, 0, 1, 2}, {2, 1, 0, 1}, {1, 2, 1, 0}};
    vector<int> tour = {0, 2, 1, 3};
    ll cost = tour_cost(tour, dist); // 6
    int l = 1, r = 2;
    ll delta = reversal_delta(tour, l, r, dist); // -2
    if (delta < 0) {
        reverse(tour.begin() + l, tour.begin() + r + 1);
        cost += delta;
    }
    assert(cost == tour_cost(tour, dist));
    cout << cost << "\n"; // 4
}

/*
巡回路の 2-opt
- n >= 1、0 <= l <= r < n。末尾から先頭へ戻る閉路で、対称距離の場合の式。
- 区間内部の辺は向きが逆になるだけなので、切り替わる境界の 2 辺だけ評価する。
- 有向・非対称のコスト、開いた経路ではこの式をそのまま使えない。
- 差分計算は O(1) でも、実際の反転は O(r-l+1)。不採用なら反転しなくてよい。
- 取り消す場合は同じ区間を反転し、保存しておいた旧コストも戻す。
*/
