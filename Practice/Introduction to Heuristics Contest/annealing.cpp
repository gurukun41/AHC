#include <bits/stdc++.h>
#include <atcoder/all>
using namespace std;
using ll = long long;
using vl = vector<ll>;                                  //long long型の一次元
using vvl = vector<vl>;                                 //long long型の二次元配列
using vvvl = vector<vvl>;                               //long long型の三次元配列
using vi = vector<int>;                                 //int型の一次元
using vvi = vector<vi>;                                 //int型の二次元配列
using vvvi = vector<vvi>;                               //int型の三次元配列
#define rep(i,a,b) for(int i = (a); i < (int)(b); i++)  //for文の短縮
#define all(v) v.begin(), v.end()                       //all(v)でvの始まりと終わりのイテレーター

//入力を受け取る
template <typename T> 
T input(){
    T x;
    cin >> x;
    return x;
}

//a,bのうち最大のものをaに入れる(aがbに置き換わるときはtrueを返す)
template <typename T>
inline bool chmax(T &a, const T& b){
    if(a < b){
        a = b;
        return true;
    }
    return false;
}

//a,bのうち最小のものをaに入れる(aがbに置き換わるときはtrueを返す)
template <typename T>
inline bool chmin(T &a, const T& b){
    if(a > b){
        a = b;
        return true;
    }
    return false;
}

//素数判定
bool is_prime(long long n) {
    if (n <= 1) {
        return false;
    }
    for (long long i = 2; i * i <= n; i++) {
        if (n % i == 0) {
            return false;
        }
    }
    return true;
}

struct Input {
    ll D;
    vl c;
    vvl s;
    Input(ll D_) : D(D_), c(26+1), s(D_+1, vl(26+1)) {}
};

struct Output {
    vl t;
    Output(ll D) : t(D+1, 0) {}

    void print() {
        rep(d, 1, t.size()) {
            cout << t[d] << "\n";
        }
    }
};

struct Satisfaction {
    vl score;         
    vl last;          
    Satisfaction(ll D) : score(D+1, 0), last(26+1, 0) {}

    void print() {
        rep(d, 1, score.size()) {
            cout << score[d] << "\n";
        }
    }
};

void calc_score(const Input& in, const Output& out, Satisfaction& sat) {
    ll S = 0;
    rep(i, 1, 27) S += in.c[i];        // S = Σ c[i]
    ll sum_c_last = 0;                 // Σ c[i] * last[i]（lastはsat.lastを使う）

    rep(d, 1, in.D+1) {
        ll t = out.t[d];

        // 累積スコア（満足度）
        if (d > 1) sat.score[d] = sat.score[d-1];
        sat.score[d] += in.s[d][t];

        // last[t] を d に更新する影響を sum_c_last に取り込む
        sum_c_last += in.c[t] * (d - sat.last[t]);
        sat.last[t] = d;

        // 今日の減衰合計 = S*d - Σ c[i]*last[i]
        ll penalty = S * d - sum_c_last;
        sat.score[d] -= penalty;
    }
}

void build_greedy(const Input& in, Output& out, Satisfaction& sat) {
    rep(d, 1, in.D+1) {
        ll max_i = 0;
        ll max_score = -1e18;
        rep(i, 1, 27) {
            ll score = in.s[d][i];
            rep(j, 1, 27) {
                if(j == i)  continue;
                score -= in.c[j] * (d - sat.last[j]);
            }
            if (score > max_score) {
                max_score = score;
                max_i = i;
            }
        }
        out.t[d] = max_i;
        sat.last[max_i] = d;  // 更新 last
        if (d > 1) {
            sat.score[d] = sat.score[d-1];
        }
        sat.score[d] += max_score;
    } 

}

void change(Output& out, ll d, ll q, const Input& in, double temperature) {
    Satisfaction sat1(out.t.size()-1);
    calc_score(in, out, sat1);
    ll temp = out.t[d];  // 現在の値を保存
    out.t[d] = q;  // 値を変更
    Satisfaction sat2(out.t.size()-1);
    calc_score(in, out, sat2);

    ll delta = sat2.score[in.D] - sat1.score[in.D];
    if (delta >= 0) {
        // スコアが改善された場合はそのまま採用
        return;
    } else {
        // スコアが悪化した場合は確率的に受け入れる
        double probability = exp(delta / temperature);
        if ((double)rand() / RAND_MAX >= probability) {
            out.t[d] = temp;  // 元に戻す
        }
    }
}

int main(){
    ll D;
    cin >> D;
    Input in(D);
    Output out(D);
    Satisfaction sat(D);
    rep(i, 1, 27) cin >> in.c[i];
    rep(d, 1, D+1) rep(i, 1, 27) cin >> in.s[d][i];
    build_greedy(in, out, sat);

    auto start = std::chrono::steady_clock::now();
    const double TIME_LIMIT = 1.5; // 秒
    const double INITIAL_TEMPERATURE = 1000.0;
    const double FINAL_TEMPERATURE = 1.0;

    while(true) {
        auto now = std::chrono::steady_clock::now();
        double elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count() / 1000.0;
        if (elapsed > TIME_LIMIT) break;

        // 温度を線形に減少させる
        double temperature = INITIAL_TEMPERATURE - (INITIAL_TEMPERATURE - FINAL_TEMPERATURE) * (elapsed / TIME_LIMIT);

        ll d, q;
        d = rand() % D + 1; // ランダムに日を選ぶ
        q = rand() % 26 + 1; // ランダムに1から
        change(out, d, q, in, temperature);
    }
    Satisfaction final_sat(D);
    calc_score(in, out, final_sat);
    out.print();
}