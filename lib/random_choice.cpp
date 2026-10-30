#include <bits/stdc++.h>
using namespace std;

struct Random {
    mt19937_64 engine;
    explicit Random(uint64_t seed = 12345) : engine(seed) {}

    int integer(int lo, int hi) { // [lo, hi)。hi は含まない。
        assert(lo < hi);
        return uniform_int_distribution<int>(lo, hi - 1)(engine);
    }
    double real() { // [0, 1)
        return generate_canonical<double, 53>(engine);
    }
    pair<int, int> distinct_pair(int n) { // [0, n) から異なる 2 点
        assert(n >= 2);
        int a = integer(0, n), b = integer(0, n - 1);
        if (b >= a) ++b; // a を飛ばす。引き直しは不要。
        return {a, b};
    }
};

int main() {
    Random rnd(42);
    vector<int> a = {0, 1, 2, 3};
    shuffle(a.begin(), a.end(), rnd.engine); // 空配列でも使える
    auto [i, j] = rnd.distinct_pair((int)a.size());
    swap(a[i], a[j]);

    // 近傍操作を比率 6:3:1 で選ぶ。重みは非負、合計は正にする。
    discrete_distribution<int> operation({6.0, 3.0, 1.0});
    int type = operation(rnd.engine);
    cout << i << " " << j << " " << type << "\n";
}

/*
乱数による選択
- 候補が空なら選べない。異なる 2 点が必要なら候補数が 2 以上か確認する。
- 固定 seed は再現確認用。比較実験では同じ入力・seed・試行回数を使う。
- engine() % n は偏りが出ることがある。均等に選ぶ例は integer()。
- 重みが変わらないなら discrete_distribution はループの外で作る。
- 標準の分布や shuffle の結果は、異なる標準ライブラリ間で同じとは限らない。
*/
