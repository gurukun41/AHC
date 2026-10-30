#include <bits/stdc++.h>
using namespace std;

struct Candidate {
    int id;
    double cheap; // 軽い評価。大きいほど良い。
    double full = 0; // 重い評価
};

template<class Evaluate>
vector<Candidate> select_two_stage(vector<Candidate> candidates, size_t width,
                                   size_t shortlist, Evaluate evaluate) {
    if (width == 0) return {};
    size_t keep = min(candidates.size(), max(width, shortlist));
    auto cheap_order = [](const auto& a, const auto& b) { return a.cheap > b.cheap; };
    partial_sort(candidates.begin(), candidates.begin() + keep, candidates.end(), cheap_order);
    candidates.resize(keep);
    for (auto& c : candidates) c.full = evaluate(c.id); // 残した候補だけ重い評価
    keep = min(width, candidates.size());
    auto full_order = [](const auto& a, const auto& b) { return a.full > b.full; };
    partial_sort(candidates.begin(), candidates.begin() + keep, candidates.end(), full_order);
    candidates.resize(keep);
    return candidates;
}

int main() {
    vector<Candidate> candidates = {{0, 10}, {1, 9}, {2, 8}, {3, 1}};
    vector<double> scores = {5, 20, 12, 100}; // 実際には重い計算で得る値
    auto selected = select_two_stage(candidates, 2, 3, [&](int id) { return scores[id]; });
    for (auto c : selected) cout << c.id << " "; // 1 2（id=3 は軽い評価で落ちる）
    cout << "\n";
}

/*
軽い評価で候補を絞ってから、重い評価を行う
- shortlist は例としてビーム幅の 3 倍など。評価時間と候補の取りこぼしを比較して決める。
- 軽い評価は上界ではないので、最良候補を捨てる可能性がある。例の id=3 がそれに当たる。
- 実スコア・軽い評価・重い評価を混同しない。NaN を評価値に含めない。
- candidate_selection.cpp は状態コピーの削減。この例は評価計算の削減。
*/
