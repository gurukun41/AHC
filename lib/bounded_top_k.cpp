#include <bits/stdc++.h>
using namespace std;

// 大きい値を K 個だけ保持する。候補全体を配列に持たずに済む。
struct TopK {
    size_t k;
    priority_queue<long long, vector<long long>, greater<long long>> heap;

    explicit TopK(size_t width) : k(width) {}
    void add(long long score) {
        if (k == 0) return;
        if (heap.size() < k) heap.push(score);
        else if (score > heap.top()) {
            heap.pop(); // 保持している中で最も悪い候補を捨てる
            heap.push(score);
        }
    }
    vector<long long> take() { // 良い順に取り出し、heap は空になる。
        vector<long long> result;
        while (!heap.empty()) {
            result.push_back(heap.top());
            heap.pop();
        }
        reverse(result.begin(), result.end());
        return result;
    }
};

int main() {
    TopK best(3);
    for (long long x : {10, 40, 20, 50, 30}) best.add(x);
    for (long long x : best.take()) cout << x << " "; // 50 40 30
    cout << "\n";
}

/*
生成しながら上位 K 個を残す
- 全候補を保持しない方法。K >= 2 なら追加 O(log K)、保持メモリ O(K)。
- 最大化では最小ヒープ、最小化では最大ヒープを使い、最悪の候補を取り出す。
- 状態を残すなら score と親番号・操作などを組にする。重複除去は別途必要。
- 全候補を配列に持てる場合は top_n.cpp の nth_element も候補になる。
*/
