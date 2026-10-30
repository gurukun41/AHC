#include <bits/stdc++.h>
using namespace std;

// 状態 = cell * 4 + dir。dir は上・右・下・左。
// 操作は 0:前進、1:右回転、2:左回転。不可能な遷移は -1。
vector<array<int, 3>> build_transitions(const vector<string>& board) {
    int h = (int)board.size(), w = (int)board[0].size();
    int dr[] = {-1, 0, 1, 0}, dc[] = {0, 1, 0, -1};
    vector<array<int, 3>> next(h * w * 4, array<int, 3>{-1, -1, -1});
    for (int r = 0; r < h; r++) for (int c = 0; c < w; c++) {
        if (board[r][c] == '#') continue;
        int cell = r * w + c;
        for (int dir = 0; dir < 4; dir++) {
            int s = cell * 4 + dir;
            next[s][1] = cell * 4 + (dir + 1) % 4;
            next[s][2] = cell * 4 + (dir + 3) % 4;
            int nr = r + dr[dir], nc = c + dc[dir];
            if (0 <= nr && nr < h && 0 <= nc && nc < w && board[nr][nc] != '#') {
                next[s][0] = (nr * w + nc) * 4 + dir;
            }
        }
    }
    return next;
}

int main() {
    vector<string> board = {"...", ".#."};
    auto next = build_transitions(board);
    int state = 0 * 4 + 1; // 左上、右向き
    state = next[state][0]; // 右へ前進
    state = next[state][1]; // 下へ回転
    assert(next[state][0] == -1); // 下は壁
    int cell = state / 4, direction = state % 4;
    cout << cell << " " << direction << "\n"; // 1 2
}

/*
固定部分の遷移を表にする
- 状態数 × 操作数ぶんの表を作り、探索中は next[state][action] を参照する。
- 例は位置と向きで遷移が決まる固定盤面。壁やルールが変われば表の更新が必要。
- 資源や他の駒に依存する合法性は、表だけでは分からない。動的な条件は別に判定する。
- -1 を次の配列添字にしない。壁で停止するルールなら -1 の代わりに自分自身を入れる。
- 盤面は空でない長方形。積が int に収まり、表を確保できるサイズとする。
*/
