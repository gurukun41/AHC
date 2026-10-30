#include <bits/stdc++.h>
using namespace std;

// 壊れた順列から、[0, n) を一度ずつ含む順列を作る。
vector<int> repair_permutation(const vector<int>& candidate, int n) {
    assert(n >= 0);
    vector<char> used(n, false);
    vector<int> result;
    for (int x : candidate) {
        if (x < 0 || x >= n || used[x]) continue;
        used[x] = true;
        result.push_back(x);
    }
    for (int x = 0; x < n; x++) if (!used[x]) result.push_back(x);
    return result;
}

int main() {
    vector<int> candidate = {3, 3, -1, 8, 1};
    auto order = repair_permutation(candidate, 5);
    assert(order == vector<int>({3, 1, 0, 2, 4}));
    for (int x : order) cout << x << " ";
    cout << "\n";
}

/*
候補を修復してから評価する
- 複数の操作を混ぜた候補や、解同士を組み合わせた候補が制約を破る場合に使う。
- 例は範囲外・重複を除き、足りない要素を末尾に追加する。残した要素の相対順は保つ。
- 修復後のスコア・ハッシュ・逆引き配列を作り直す。修復前の評価で採否を決めない。
- 足りない要素を小さい順に足すと偏りが出る。必要なら乱数や挿入コストで位置を選ぶ。
- 修復で大きく変わるなら、初めから制約を保つ近傍も検討する。
- 容量や先行関係など別の制約までは保証しない。修復後にも合法性を確認する。
*/
