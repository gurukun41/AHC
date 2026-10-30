#include <bits/stdc++.h>
using namespace std;
using ll = long long;

struct Job { ll release, duration, weight; };
struct State { ll time = 0, cost = 0; };

State advance(State s, const Job& job) {
    s.time = max(s.time, job.release) + job.duration;
    s.cost += s.time * job.weight; // 完了時刻の重み付き和を最小化する例
    return s;
}

// prefix[i]: 最初の i 個を実行した状態。dirty より前の順序は変わっていない前提。
void rebuild_from(const vector<int>& order, const vector<Job>& jobs,
                  vector<State>& prefix, size_t dirty) {
    assert(dirty <= order.size());
    assert(dirty == 0 || prefix.size() > dirty);
    prefix.resize(order.size() + 1);
    if (dirty == 0) prefix[0] = State{};
    for (size_t i = dirty; i < order.size(); i++) {
        prefix[i + 1] = advance(prefix[i], jobs[order[i]]);
    }
}

int main() {
    vector<Job> jobs = {{0, 2, 1}, {1, 3, 2}, {5, 1, 4}};
    vector<int> order = {0, 1, 2};
    vector<State> prefix;
    rebuild_from(order, jobs, prefix, 0);
    swap(order[1], order[2]);
    rebuild_from(order, jobs, prefix, 1); // 最初の 1 個は再計算しない

    vector<State> full;
    rebuild_from(order, jobs, full, 0);
    assert(prefix.back().cost == full.back().cost);
    assert(prefix.back().time == full.back().time);
    cout << prefix.back().cost << "\n"; // 44
}

/*
途中状態を保存して、変更箇所以降だけ再計算する
- 変更の影響が後ろへ伝わるシミュレーション向け。dirty は最初に影響する位置。
- prefix には時刻だけでなく、盤面・資源・累積評価など再開に必要な情報をすべて持つ。
- 不採用時は order だけを戻さず、prefix も戻すか dirty 以降を作り直す。
- 全状態を保存すると重い場合は、数十手ごとの状態から変更位置まで再生する。
- 再計算は O(残り手数 × 1 手の処理時間)。状態の保存・コピー費用も考える。
- 例は jobs の属性が不変で、時刻・積・合計が ll に収まる場合。
*/
