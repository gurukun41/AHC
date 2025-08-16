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

struct BNode {
    long long score;
    long long sum_c_last;
    array<int, 27> last; // last[1..26]
    int parent;          // 前日の層でのインデックス
    int choice;          // 今日選んだコンテスト
};

void build_beam(const Input& in, Output& out, Satisfaction& sat, int BEAM = 50) {
    // S = Σ c[i]
    long long S = 0;
    rep(i, 1, 27) S += in.c[i];

    vector<vector<BNode>> layers;
    layers.reserve(in.D + 1);

    // 初期状態（日0）
    BNode root{};
    root.score = 0;
    root.sum_c_last = 0;
    root.last.fill(0);
    root.parent = -1;
    root.choice = 0;
    layers.push_back(vector<BNode>{root});

    // 各日dを展開
    for (int d = 1; d <= in.D; d++) {
        const auto& prev = layers.back();
        vector<BNode> cand;
        cand.reserve((int)prev.size() * 26);

        for (int p = 0; p < (int)prev.size(); p++) {
            const auto& node = prev[p];
            for (int i = 1; i <= 26; i++) {
                BNode nxt;
                nxt.parent = p;
                nxt.choice = i;
                nxt.last = node.last;

                // 増分更新
                long long added = in.c[i] * (long long)(d - nxt.last[i]);
                nxt.sum_c_last = node.sum_c_last + added;
                long long penalty = S * (long long)d - nxt.sum_c_last;
                nxt.score = node.score + in.s[d][i] - penalty;

                nxt.last[i] = d;
                cand.push_back(std::move(nxt));
            }
        }

        // 上位BEAM件（スコア降順）に絞る
        auto cmp_desc = [](const BNode& a, const BNode& b) {
            return a.score > b.score;
        };
        if ((int)cand.size() > BEAM) {
            nth_element(cand.begin(), cand.begin() + BEAM, cand.end(), cmp_desc);
            cand.resize(BEAM);
        }

        layers.push_back(std::move(cand));
    }

    // 最終日の層から最良を選び、親を辿って復元
    const auto& last_layer = layers.back();
    int best_idx = 0;
    for (int i = 1; i < (int)last_layer.size(); i++) {
        if (last_layer[i].score > last_layer[best_idx].score) best_idx = i;
    }
    // 復元
    vi t(in.D + 1, 0);
    int cur = best_idx;
    for (int d = in.D; d >= 1; d--) {
        t[d] = layers[d][cur].choice;
        cur = layers[d][cur].parent;
    }
    rep(d, 1, in.D + 1) out.t[d] = t[d];

    // スコア配列などを埋める（そのまま出力に使える）
    calc_score(in, out, sat);
}

int main(){
    ll D;
    cin >> D;
    Input in(D);
    Output out(D);
    Satisfaction sat(D);
    rep(i, 1, 27) cin >> in.c[i];
    rep(d, 1, D+1) rep(i, 1, 27) cin >> in.s[d][i];

    // ビームサーチ
    int BEAM = 50;
    build_beam(in, out, sat, BEAM);

    out.print();
    return 0;
}