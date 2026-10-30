#include <bits/stdc++.h>
using namespace std;
using ull = uint64_t;

struct RecentStates {
    size_t window;
    deque<ull> history;
    unordered_map<ull, int> frequency;
    explicit RecentStates(size_t length) : window(length) {}

    int count(ull hash) const {
        auto it = frequency.find(hash);
        return it == frequency.end() ? 0 : it->second;
    }
    void push(ull hash) {
        history.push_back(hash);
        ++frequency[hash];
        if (history.size() > window) {
            ull old = history.front(); history.pop_front();
            auto it = frequency.find(old);
            if (--it->second == 0) frequency.erase(it);
        }
    }
};

int main() {
    RecentStates recent(3);
    for (ull h : {10, 20, 10}) recent.push(h);
    assert(recent.count(10) == 2);
    recent.push(30); // 古い 10 が一つだけ外れる
    assert(recent.count(10) == 1);
    cout << recent.count(30) << "\n"; // 1
}

/*
直近の履歴でループを避ける
- 候補の評価時に count(hash) を調べ、再訪回数の少ない状態を優先するなどに使う。
- 採用して実際に進んだ状態を push する。捨てた候補まで履歴に入れない。
- 同じ状態が複数回入るので、古い履歴を外すときは回数を 1 減らす。
- 再訪が必要な問題もあるため、全面禁止が適切とは限らない。窓幅や罰点を調整する。
- 例は 64 bit のハッシュ衝突を無視する。ビームの枝ごとの履歴が必要なら別々に持つ。
*/
