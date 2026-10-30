#include <bits/stdc++.h>
using namespace std;
using ll = long long;

ll calc_score(const vector<int>& a) {
    ll score = 0;
    for (int i = 0; i < (int)a.size(); i++) score += (ll)(i + 1) * a[i];
    return score;
}

// 近傍を試し続け、一定回数改善がなければ終了する。
void climb(vector<int>& a, ll& score, mt19937_64& rng, int stall_limit) {
    int n = (int)a.size();
    if (n < 2) return;
    int stall = 0;
    while (stall < stall_limit) {
        int i = uniform_int_distribution<int>(0, n - 1)(rng);
        int j = uniform_int_distribution<int>(0, n - 2)(rng);
        if (j >= i) ++j;
        ll delta = ((ll)i - j) * ((ll)a[j] - a[i]);
        if (delta > 0) {
            swap(a[i], a[j]);
            score += delta;
            stall = 0;
        } else ++stall;
    }
}

int main() {
    mt19937_64 rng(42);
    vector<int> initial = {4, 0, 3, 1, 2};
    vector<int> best = initial;
    ll best_score = calc_score(best);
    for (int trial = 0; trial < 10; trial++) {
        vector<int> current = initial; // 試行ごとに作り直す
        shuffle(current.begin(), current.end(), rng);
        ll score = calc_score(current);
        climb(current, score, rng, 100);
        if (score > best_score) {
            best_score = score;
            best = current;
        }
    }
    assert(best_score == calc_score(best));
    cout << best_score << "\n";
}

/*
多点スタートと停滞打ち切り
- 異なる初期解から探索し、全試行を通した最良解を別に保持する。
- stall_limit 回改善しなくても、近傍に改善がない証明にはならない。
- 改善が続くと停滞判定では止まらない。時間制なら climb 内にも時間確認が必要。
- 次の試行を最良解から始めて数回ランダムに変更する方法もある。最良解自体は壊さない。
- 初期解を作り直す際は、スコア・ハッシュ・作業用の配列も一致させる。
*/
