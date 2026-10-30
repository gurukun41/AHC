#include <bits/stdc++.h>
using namespace std;
using ll = long long;

// 例: 容量以内で品物を選ぶ最大化。重さ・価値は非負とする。
struct Item { ll weight, value; };
struct Result { ll score; vector<int> chosen; };

Result search(const vector<Item>& items, ll capacity, size_t node_limit) {
    int n = (int)items.size();
    vector<ll> suffix(n + 1, 0);
    for (int i = n - 1; i >= 0; i--) suffix[i] = suffix[i + 1] + items[i].value;
    Result best{0, {}}; // 空集合も実行可能な解
    vector<int> chosen;
    size_t visited = 0;

    auto dfs = [&](auto&& self, int i, ll weight, ll score) -> void {
        if (visited >= node_limit) return;
        ++visited;
        if (score > best.score) best = {score, chosen};
        // 残りを全部選べると楽観しても、既知の最良解を超えない。
        if (i == n || score + suffix[i] <= best.score) return;
        if (items[i].weight <= capacity - weight) {
            chosen.push_back(i);
            self(self, i + 1, weight + items[i].weight, score + items[i].value);
            chosen.pop_back();
        }
        self(self, i + 1, weight, score);
    };
    dfs(dfs, 0, 0, 0);
    return best;
}

int main() {
    vector<Item> items = {{2, 3}, {3, 4}, {4, 5}};
    auto answer = search(items, 5, 1000);
    cout << answer.score << "\n"; // 7
}

/*
上界を使う枝刈りと探索予算
- 最大化では「この先どれだけ良くても到達できない」上界で枝を切る。
- 推定値を上界として使うと、最適解を捨てることがある。評価値と上界は別物。
- 最小化なら楽観的な下界を使う。現在値だけで切れるかは将来の改善可能性による。
- 全探索が完了すれば最適。予算で止めた場合は、それまでの実行可能な最良解を返す。
- 有望な枝を先に探すと、早く良い基準値が得られる。探索量は最悪で指数的。
- 例は容量・重さ・価値が非負、合計が ll に収まる場合。深い再帰のスタック量にも注意。
*/
