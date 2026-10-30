#include <bits/stdc++.h>
using namespace std;
using ll = long long;

struct State {
    vector<int> a;
    ll score;
};

// 例: 位置ごとに重みが違うので、交換によってスコアが変わる。
ll calc_score(const State& s) {
    ll score = 0;
    for (int i = 0; i < (int)s.a.size(); i++) score += (ll)(i + 1) * s.a[i];
    return score;
}

// 取り消すために、変更前の情報を保存する。
struct UndoSwap {
    int i, j;
    int old_i, old_j;
    ll old_score;
};

UndoSwap apply_swap(State& s, int i, int j) {
    UndoSwap undo = {i, j, s.a[i], s.a[j], s.score};
    swap(s.a[i], s.a[j]);
    s.score = calc_score(s); // 分かりやすさのため全体を再計算: O(N)
    return undo;
}

void undo_swap(State& s, const UndoSwap& undo) {
    s.a[undo.i] = undo.old_i;
    s.a[undo.j] = undo.old_j;
    s.score = undo.old_score;
}

int main() {
    State s = {{1, 2, 3}, 0};
    s.score = calc_score(s); // 14

    UndoSwap undo = apply_swap(s, 0, 2); // {3, 2, 1}, score = 10
    bool accept = s.score >= undo.old_score; // 例: スコアを最大化する山登り法
    if (!accept) undo_swap(s, undo);

    assert(s.a == vector<int>({1, 2, 3}) && s.score == 14);
    cout << s.score << "\n"; // 14
}

/*
操作の適用と取り消し

状態を変更して候補を評価し、採用しない場合は操作前に戻す。
この例は配列の 2 要素を交換する。各位置の値とスコアを保存して戻す。

- 交換のように逆操作で元に戻せる場合は、逆操作を使う方法もある。
- 上書きのように元の値を復元できない場合は、変更前の値を保存する。
- 変更箇所だけでスコアを差分計算できれば、全体の再計算を省ける。
  開発中は全体から再計算したスコアと比較し、更新漏れを確認する。

- 状態がハッシュ、資源、手番なども持つ場合は、それらも必要に応じて戻す。
- 操作が複数なら逆順に取り消す。使用する index は事前に範囲を確認する。
- ビームサーチで候補を残す場合は、undo 前に候補や復元情報を保存する。
*/
