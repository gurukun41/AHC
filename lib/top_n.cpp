#include <bits/stdc++.h>
using namespace std;

/*
ビームサーチなどで、候補 M 個からスコアの高い上位 N 個を残す。

nth_element: 上位 N 個を選ぶ。内部の順序は保証されない。平均 O(M)
partial_sort: 上位 N 個を選び、スコア順にも並べる。O(M log N)
全体を sort すると O(M log M)。

同じスコアの候補が境界にあるとき、どれが残るかは決まらない。
*/
struct State {
    int score;
    int id; // 状態を区別するための例
};

int main() {
    vector<State> candidates = {{30, 0}, {10, 1}, {50, 2}, {20, 3}, {40, 4}};
    int width = 3; // ビーム幅。0 以上を指定する

    // score が大きいほど良い（最小化なら < に変える）。
    auto better = [](const State& a, const State& b) {
        return a.score > b.score;
    };

    // --- 1. 上位 N 個を残すだけなら nth_element ---
    vector<State> beam = candidates;
    if (beam.size() > (size_t)width) {
        // [begin, begin + width) に上位 width 個が入る。
        // その中はソートされていない。begin + width 自体は残さない。
        nth_element(beam.begin(), beam.begin() + width, beam.end(), better);
        beam.resize(width);
    }
    // beam の score は 50, 40, 30（順序は不定）。
    // 順番も必要なら、ここで sort(beam.begin(), beam.end(), better);

    // --- 2. 上位 N 個をスコア順に並べるなら partial_sort ---
    vector<State> sorted_beam = candidates;
    size_t n = min(sorted_beam.size(), (size_t)width);
    partial_sort(sorted_beam.begin(), sorted_beam.begin() + n, sorted_beam.end(), better);
    sorted_beam.resize(n);
    // sorted_beam の score は 50, 40, 30 の順。

    for (const State& s : sorted_beam) cout << s.score << " ";
    cout << "\n";
}
