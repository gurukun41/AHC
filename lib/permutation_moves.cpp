#include <bits/stdc++.h>
using namespace std;

// a[from] を移動後の位置 to に挿入。他の要素の順序は保つ。
void relocate(vector<int>& a, int from, int to) {
    if (from < to) rotate(a.begin() + from, a.begin() + from + 1, a.begin() + to + 1);
    if (from > to) rotate(a.begin() + to, a.begin() + from, a.begin() + from + 1);
}

// 両端を含む区間 [l, r] を反転する。
void reverse_segment(vector<int>& a, int l, int r) {
    reverse(a.begin() + l, a.begin() + r + 1);
}

int main() {
    vector<int> a = {0, 1, 2, 3, 4};
    relocate(a, 1, 3); // {0, 2, 3, 1, 4}
    relocate(a, 3, 1); // 取り消し: from と to を入れ替える
    reverse_segment(a, 1, 3);
    reverse_segment(a, 1, 3); // 同じ反転をもう一度行うと戻る
    swap(a[0], a[4]);        // 2 点交換も同じ交換で戻る
    swap(a[0], a[4]);
    assert(a == vector<int>({0, 1, 2, 3, 4}));
}

/*
順列の近傍操作
- 位置は 0 始まりで配列内。反転では l <= r とする。
- 交換は O(1)、挿入・反転は動かす要素数に比例する。
- relocate の to は移動後の添字。元の配列の「この要素の前」と混同しない。
- 操作だけでなく、スコア・ハッシュ・位置の逆引き配列なども更新する。
*/
