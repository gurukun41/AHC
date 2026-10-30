#include <bits/stdc++.h>
using namespace std;
using ll = long long;
using Clock = chrono::steady_clock;

struct Config { int swaps; double weight; }; // 近傍の交換回数と時間配分の重み

ll calc_score(const vector<int>& a) {
    ll score = 0;
    for (int i = 0; i < (int)a.size(); i++) score += (ll)(i + 1) * a[i];
    return score;
}

vector<int> search_portfolio(const vector<int>& initial, const vector<Config>& configs,
                             double seconds, uint64_t seed) {
    assert(seconds > 0);
    auto deadline = Clock::now() + chrono::duration_cast<Clock::duration>(chrono::duration<double>(seconds));
    mt19937_64 rng(seed);
    vector<int> best = initial; // 一度も探索できなくても出力できる解を持つ
    ll best_score = calc_score(best);
    int n = (int)initial.size();
    double weight_left = 0;
    for (const auto& c : configs) {
        assert(c.swaps >= 1 && c.weight > 0);
        weight_left += c.weight;
    }
    if (n < 2) return best;
    for (const auto& config : configs) {
        auto now = Clock::now();
        if (now >= deadline) break;
        double remaining = chrono::duration<double>(deadline - now).count();
        double allocated = remaining * min(1.0, config.weight / weight_left);
        auto trial_deadline = min(deadline, now + chrono::duration_cast<Clock::duration>(chrono::duration<double>(allocated)));
        weight_left -= config.weight;

        vector<int> current = initial;
        ll score = calc_score(current);
        while (Clock::now() < trial_deadline) {
            auto candidate = current;
            for (int k = 0; k < config.swaps; k++) {
                int i = uniform_int_distribution<int>(0, n - 1)(rng);
                int j = uniform_int_distribution<int>(0, n - 2)(rng);
                if (j >= i) ++j;
                swap(candidate[i], candidate[j]);
            }
            ll next_score = calc_score(candidate);
            if (next_score >= score) {
                current = std::move(candidate);
                score = next_score;
                if (score > best_score) { best = current; best_score = score; }
            }
        }
    }
    return best;
}

int main() {
    vector<int> initial = {4, 0, 3, 1, 2};
    auto best = search_portfolio(initial, {{1, 2}, {2, 1}, {3, 1}}, 0.02, 42);
    assert(calc_score(best) >= calc_score(initial));
    cout << calc_score(best) << "\n";
}

/*
残り時間を複数の探索設定へ配る
- 例は異なる近傍の交換回数を試す。ビーム幅・深さや初期解の作り方にも応用できる。
- 各試行の開始時に「残り時間 × 今回の重み / 未実行の重み合計」で期限を決め直す。
- 全試行共通の最良解を保持する。途中終了しても実行可能な解を返せるようにする。
- 初期化や 1 回の重い評価で期限を超えることがある。出力の余裕を引き、必要なら評価内でも確認する。
- 各試行は同じ基準の実スコアで比較する。設定ごとに違う評価値では比較できない。
- 固定 seed でも時間制では試行回数が変わる。再現確認には回数制も使う。
*/
