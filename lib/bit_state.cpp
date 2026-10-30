#include <bits/stdc++.h>
using namespace std;
using ull = uint64_t;

ull full_mask(int n) { // 下位 n bit。0 <= n <= 64。
    assert(0 <= n && n <= 64);
    return n == 64 ? ~ull(0) : (ull(1) << n) - 1;
}

vector<int> members(ull mask) {
    vector<int> result;
    while (mask != 0) {
        int p = __builtin_ctzll(mask); // 立っている最下位 bit の番号
        result.push_back(p);
        mask &= mask - 1; // その bit を消す
    }
    return result;
}

int main() {
    int n = 10, p = 3; // p は [0, n)
    ull visited = 0;
    visited |= ull(1) << p;          // 追加
    bool has = (visited >> p) & 1;   // 存在確認
    int count = __builtin_popcountll(visited);
    ull unvisited = full_mask(n) & ~visited; // n より上の bit を落とす
    visited &= ~(ull(1) << p);       // 削除
    assert(has && count == 1 && visited == 0);
    assert(members(unvisited).size() == 9);

    bitset<256> larger; // 64 個を超える固定サイズの集合
    larger.set(100);
    larger.reset(100);
    cout << larger.count() << "\n";
}

/*
訪問済み・所持品などを bit で持つ
- 64 個以内なら uint64_t 一つでコピー・一致判定・集合演算ができる。
- 1 << p ではなく ull(1) << p。p >= 64 のシフトや ctzll(0) は行わない。
- XOR は反転なので、既に入っている要素に使うと消える。追加には OR を使う。
- 順序や個数は表せない。それらが状態に必要なら別に持つ。
- bitset の大きさはコンパイル時に決める。可変長なら複数の 64 bit 語などで管理する。
*/
