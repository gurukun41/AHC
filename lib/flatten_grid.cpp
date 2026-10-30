#include <bits/stdc++.h>
using namespace std;

/*
盤面の一次元化

2 次元 (H, W):       (r, c)    -> r * W + c
3 次元 (A, B, C):    (a, b, c) -> (a * B + b) * C + c
最後の座標が連続するように並べる。座標は 0 始まり、H や W はマス数。

注意:
- 隣接マスは id だけで範囲判定しない。行や層をまたいでしまう。
- 各座標が範囲内かを確認してから id に変換する。
- 復元する id は盤面内、各方向の長さは正とする。関数内では範囲を検査しない。
- 以下は要素数や計算途中が int に収まる例。大きい場合は掛け算の前から
  size_t や long long を使い、配列を確保できる大きさかも確認する。
*/

// 2 次元: (r, c) <-> id。W は列数。
int flatten2(int r, int c, int W) {
    return r * W + c;
}

pair<int, int> unflatten2(int id, int W) {
    return {id / W, id % W};
}

// 3 次元: (a, b, c) <-> id。B, C は各方向の長さ。
int flatten3(int a, int b, int c, int B, int C) {
    return (a * B + b) * C + c;
}

tuple<int, int, int> unflatten3(int id, int B, int C) {
    return {id / (B * C), (id / C) % B, id % C};
}

int main() {
    // --- 2 次元: H 行 W 列 ---
    int H = 2, W = 3;
    vector<int> grid(H * W);
    int r = 1, c = 2;
    int id = flatten2(r, c, W); // 5
    grid[id] = 10;
    auto [back_r, back_c] = unflatten2(id, W);
    assert(back_r == r && back_c == c);

    // 4 近傍: 行・列ごとに境界を確認する。
    int dr[] = {-1, 1, 0, 0};
    int dc[] = {0, 0, -1, 1};
    for (int d = 0; d < 4; d++) {
        int nr = r + dr[d], nc = c + dc[d];
        if (nr < 0 || nr >= H || nc < 0 || nc >= W) continue;
        int next_id = flatten2(nr, nc, W);
        // grid[next_id] が隣接マス
        (void)next_id;
    }
    // 例: (0, W-1) の id + 1 は (1, 0)。右隣ではない。

    // --- 3 次元: A × B × C ---
    int A = 2, B = 3, C = 4;
    vector<int> cube(A * B * C);
    int a = 1, b = 2, z = 3;
    int p = flatten3(a, b, z, B, C); // 23
    cube[p] = 20;
    auto [back_a, back_b, back_z] = unflatten3(p, B, C);
    assert(back_a == a && back_b == b && back_z == z);

    // 6 近傍: 各座標の範囲を確認してから変換する。
    int da[] = {-1, 1, 0, 0, 0, 0};
    int db[] = {0, 0, -1, 1, 0, 0};
    int dz[] = {0, 0, 0, 0, -1, 1};
    for (int d = 0; d < 6; d++) {
        int na = a + da[d], nb = b + db[d], nz = z + dz[d];
        if (na < 0 || na >= A || nb < 0 || nb >= B || nz < 0 || nz >= C) continue;
        int next_p = flatten3(na, nb, nz, B, C);
        // cube[next_p] が隣接マス
        (void)next_p;
    }
}
