#include <bits/stdc++.h>
using namespace std;

// 同じ配列を使って何度も「今回訪問したか」を記録する。
struct VisitStamp {
    vector<uint32_t> seen;
    uint32_t generation = 0;
    explicit VisitStamp(int n) : seen(n, 0) {}

    void next_search() {
        if (++generation == 0) { // オーバーフロー時だけ全体を消す
            fill(seen.begin(), seen.end(), 0);
            generation = 1;
        }
    }
    bool contains(int v) const { return seen[v] == generation; }
    bool mark(int v) { // 今回初めて訪問するなら true
        if (contains(v)) return false;
        seen[v] = generation;
        return true;
    }
};

int main() {
    VisitStamp visited(100);
    visited.next_search(); // 検索を始める前に必ず呼ぶ
    assert(visited.mark(5));
    assert(!visited.mark(5));
    visited.next_search();
    assert(!visited.contains(5));
    assert(visited.mark(5));
}

/*
訪問済み配列の初期化を省く
- BFS、局所探索、影響範囲の重複除去を繰り返すときに使う。
- 毎回の全配列クリアを通常 O(1) の世代更新に置き換える。mark と contains は O(1)。
- seen は今回の generation と一致するときだけ有効。以前の距離などもその条件で読む。
- それぞれの探索結果を同時に保持する用途には向かない。v は配列内の番号とする。
*/
