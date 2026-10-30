#include <bits/stdc++.h>
using namespace std;

// 要素は [0, n) の整数。順序を保たない集合。
struct RandomSet {
    vector<int> values; // 現在存在する要素
    vector<int> pos;    // 要素 -> values 内の位置。未登録は -1。
    explicit RandomSet(int n) : pos(n, -1) {}

    bool contains(int x) const { return pos[x] != -1; }
    bool insert(int x) {
        if (contains(x)) return false;
        pos[x] = (int)values.size();
        values.push_back(x);
        return true;
    }
    bool erase(int x) {
        if (!contains(x)) return false;
        int i = pos[x], last = values.back();
        values[i] = last; // 穴を末尾の要素で埋める
        pos[last] = i;
        values.pop_back();
        pos[x] = -1;
        return true;
    }
    int sample(mt19937_64& rng) const {
        assert(!values.empty());
        int i = uniform_int_distribution<int>(0, (int)values.size() - 1)(rng);
        return values[i];
    }
};

int main() {
    RandomSet available(10);
    available.insert(2);
    available.insert(7);
    available.erase(2);
    mt19937_64 rng(42);
    cout << available.sample(rng) << "\n"; // 7
}

/*
追加・削除しながらランダムに選ぶ
- 例: 操作可能なマスや未使用の要素を維持する。全要素からの引き直しを避けられる。
- 追加は償却 O(1)、削除・存在確認・選択は O(1)。pos のメモリは値域に比例する。
- 削除で順番が変わるので、values 内の添字を別の場所に保存しない。
- erase の後に insert しても元の順序には戻らない。完全な再現が必要な undo では注意。
- sample は空集合では呼べない。x は必ず [0, n) とする。
*/
