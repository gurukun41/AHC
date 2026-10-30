#include <bits/stdc++.h>
using namespace std;

struct Timer {
    using Clock = chrono::steady_clock;
    Clock::time_point start = Clock::now();
    double limit; // このタイマーの作成時点から使える秒数

    explicit Timer(double seconds) : limit(seconds) { assert(seconds > 0); }
    double elapsed() const {
        return chrono::duration<double>(Clock::now() - start).count();
    }
    double remaining() const { return max(0.0, limit - elapsed()); }
    double progress() const { return clamp(elapsed() / limit, 0.0, 1.0); }
    bool expired() const { return elapsed() >= limit; }
};

int main() {
    Timer timer(0.01);
    long long iter = 0;
    while (true) {
        if (iter % 256 == 0 && timer.expired()) break;
        // ここで候補を試し、最良解を更新する。
        ++iter;
    }
    cerr << "iter = " << iter << ", time = " << timer.elapsed() << "\n";
}

/*
経過時間と打ち切り
- steady_clock は時計の時刻修正に影響されない経過時間の計測に使う。
- 制限時間から出力などの余裕を引き、入力・前処理をどこまで含めるか決める。
- 256 回に 1 回の確認では、その間の処理時間だけ超過し得る。重い処理は毎回確認する。
- progress() は温度調整、remaining() は残りの試行への時間配分に使える。
- 実時間による終了では、同じ乱数 seed でも環境によって試行回数が変わる。
*/
