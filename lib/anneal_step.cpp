#include <bits/stdc++.h>
using namespace std;
using ll = long long;

// delta = 新スコア - 旧スコア。最大化用。
bool accept(double delta, double temperature, double u) {
    if (delta >= 0) return true;
    if (temperature <= 0) return false;
    return u < exp(delta / temperature); // u は [0, 1) の一様乱数
}

double temperature(double start, double end, double progress) {
    assert(start > 0 && end > 0);
    return start * pow(end / start, clamp(progress, 0.0, 1.0));
}

ll calc_score(const vector<int>& a) {
    ll score = 0;
    for (int i = 0; i < (int)a.size(); i++) score += (ll)(i + 1) * a[i];
    return score;
}

int main() {
    mt19937_64 rng(42);
    vector<int> current = {3, 1, 4, 0, 2};
    ll current_score = calc_score(current);
    vector<int> best = current; // 現在解とは別に最良解を保持する
    ll best_score = current_score;
    int n = (int)current.size(), iterations = 10000;

    for (int iter = 0; iter < iterations; iter++) {
        int i = uniform_int_distribution<int>(0, n - 1)(rng);
        int j = uniform_int_distribution<int>(0, n - 2)(rng);
        if (j >= i) ++j;
        // この例の重み付き和に対する交換差分。実問題では評価式に合わせる。
        ll delta = ((ll)i - j) * ((ll)current[j] - current[i]);
        double t = temperature(5.0, 0.01, (double)iter / (iterations - 1));
        double u = generate_canonical<double, 53>(rng);
        if (!accept((double)delta, t, u)) continue;
        swap(current[i], current[j]);
        current_score += delta;
        if (current_score > best_score) {
            best_score = current_score;
            best = current;
        }
    }
    assert(best_score == calc_score(best));
    cout << best_score << "\n";
}

/*
焼きなましの採否判定と最良解の保存
- 悪化も採用するので、最後の現在解が最良解とは限らない。出力用の最良解を別に持つ。
- 最小化なら delta を「旧コスト - 新コスト」にする。比較の向きも変える。
- 差分を操作前に計算できる例。操作後に評価する場合は、不採用時に undo する。
- 温度はスコア差の大きさに合わせる。悪化量 d を確率 p で採用する温度は -d/log(p)。
- 例は n >= 2、iterations >= 2。時間制なら進捗を timer.cpp の progress() に置き換える。
- aneal.cpp は既存の時間制テンプレート。この例は採否と解の保存の使い方。
*/
