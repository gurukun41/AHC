#include <bits/stdc++.h>
using namespace std;

// 候補によらず共通の未来需要を生成する。
vector<vector<int>> make_scenarios(int count, int horizon, uint64_t seed) {
    assert(count > 0 && horizon > 0);
    mt19937_64 rng(seed);
    vector<vector<int>> scenarios(count, vector<int>(horizon));
    for (auto& demand : scenarios) for (int& x : demand) x = uniform_int_distribution<int>(0, 5)(rng);
    return scenarios;
}

// 例: 最初の仕入れ数を選ぶ。その後は在庫を 3 に補充する固定方策。
double evaluate(int first_order, const vector<vector<int>>& scenarios) {
    assert(first_order >= 0 && !scenarios.empty());
    double total = 0;
    for (const auto& demand : scenarios) {
        int stock = 1;
        double profit = 0;
        for (size_t t = 0; t < demand.size(); t++) {
            int order = t == 0 ? first_order : max(0, 3 - stock);
            stock += order;
            int sold = min(stock, demand[t]);
            stock -= sold;
            profit += 3.0 * sold - 2.0 * order;
        }
        total += profit;
    }
    return total / scenarios.size();
}

int main() {
    auto scenarios = make_scenarios(20, 5, 42); // ループの外で一度だけ作る
    int best_order = 0;
    double best_score = -numeric_limits<double>::infinity();
    for (int order = 0; order <= 4; order++) {
        double score = evaluate(order, scenarios);
        if (score > best_score) {
            best_score = score;
            best_order = order;
        }
    }
    cout << best_order << "\n";
}

/*
同じ未来サンプルで候補を比較する
- 候補ごとに別の未来を引かず、同じ外部入力列を使って差を比較する。
- 評価は期待値の推定。候補数が多い場合、別サンプルで上位候補を再確認する方法もある。
- 現在までの観測で未来の分布を推定する。実際には未観測の未来を評価へ混ぜない。
- 方策は時刻 t の判断時に t 以降の需要を参照しない。シミュレータだけが需要を与える。
- 例は需要が行動に依存しない。依存する場合は、共通の外部乱数から各状態に応じて生成する。
- 計算量は候補数 × シナリオ数 × 先読み長。残り時間に合わせて調整する。
*/
