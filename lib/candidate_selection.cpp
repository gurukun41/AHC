#include <bits/stdc++.h>
using namespace std;
using ll = long long;

struct State {
    vector<int> a; // コピーが重い部分
    ll score;
};
struct Candidate {
    ll score;
    int parent; // 現在の beam 内の位置
    int i, j;   // 親に適用する交換
};

// 重み付き和の例。状態を作らずに交換後のスコアを求める。
ll swap_score(const State& s, int i, int j) {
    return s.score + ((ll)i - j) * ((ll)s.a[j] - s.a[i]);
}

vector<State> next_beam(const vector<State>& beam, size_t width) {
    vector<Candidate> candidates;
    for (int parent = 0; parent < (int)beam.size(); parent++) {
        int n = (int)beam[parent].a.size();
        for (int i = 0; i < n; i++) for (int j = i + 1; j < n; j++) {
            candidates.push_back({swap_score(beam[parent], i, j), parent, i, j});
        }
    }
    auto better = [](const Candidate& a, const Candidate& b) { return a.score > b.score; };
    if (candidates.size() > width) {
        nth_element(candidates.begin(), candidates.begin() + width, candidates.end(), better);
        candidates.resize(width);
    }

    vector<State> next;
    next.reserve(candidates.size());
    for (const auto& c : candidates) {
        State s = beam[c.parent]; // 選ばれた候補だけコピーする
        swap(s.a[c.i], s.a[c.j]);
        s.score = c.score;
        next.push_back(std::move(s));
    }
    return next;
}

int main() {
    vector<State> beam = {{{3, 1, 2}, 11}};
    beam = next_beam(beam, 2);
    for (const auto& s : beam) cout << s.score << " "; // 13 13
    cout << "\n";
}

/*
候補を絞ってから重い状態を作る
- 全候補の状態コピーを避け、評価値・親番号・操作だけを一時的に持つ。
- 例は操作前に評価できる。必要なら親に apply -> 評価 -> undo して候補を記録する。
- parent は今回の beam の添字。候補の実体化が終わるまで beam を変更しない。
- この例は重複除去を省いている。必要なら候補を選ぶ前に hash などで除く。
- 復元用の永続的なノード番号と、今回だけの beam 内の位置を混同しない。
*/
