#include <bits/stdc++.h>
using namespace std;

struct State { int r, c, energy; };
struct Validation { bool valid; size_t step; string reason; };
constexpr int DR[] = {-1, 1, 0, 0};
constexpr int DC[] = {0, 0, -1, 1};

bool can_apply(const State& s, int direction, const vector<string>& board) {
    if (direction < 0 || direction >= 4 || s.energy <= 0) return false;
    int r = s.r + DR[direction], c = s.c + DC[direction];
    return 0 <= r && r < (int)board.size() && 0 <= c && c < (int)board[0].size()
        && board[r][c] != '#';
}

bool apply_checked(State& s, int direction, const vector<string>& board) {
    if (!can_apply(s, direction, board)) return false; // 不正なら変更しない
    s.r += DR[direction]; s.c += DC[direction]; --s.energy;
    return true;
}

Validation validate_plan(State initial, const vector<int>& plan, const vector<string>& board,
                         pair<int, int> goal, size_t max_steps) {
    if (plan.size() > max_steps) return {false, max_steps, "too many steps"};
    for (size_t i = 0; i < plan.size(); i++) {
        if (!apply_checked(initial, plan[i], board)) return {false, i, "illegal move"};
    }
    if (make_pair(initial.r, initial.c) != goal) return {false, plan.size(), "goal not reached"};
    return {true, plan.size(), "ok"};
}

int main() {
    vector<string> board = {"..", ".#"};
    State initial{0, 0, 3};
    assert(validate_plan(initial, {3}, board, {0, 1}, 3).valid); // 右へ移動
    auto bad = validate_plan(initial, {3, 1}, board, {0, 1}, 3); // 次に壁へ移動
    assert(!bad.valid && bad.step == 1);
    cout << bad.reason << "\n";
}

/*
合法性の確認と操作列の再生
- 状態を変える前に条件を確認する。複数の同時操作なら資源の競合なども一括で調べる。
- 完成した解は必ず初期状態から再生する。探索中に保持した最終状態だけでは検証にならない。
- 操作回数・資源・ゴールなど、出力の成立条件をまとめて確認する。step は 0 始まり。
- 例は空でない長方形盤面と、合法な初期状態を前提とする。実問題の採点規則に合わせて変更する。
- 遷移と検証で共通のバグを持たないよう、小例の手計算や公式ツールでも確認する。
*/
