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
    rep(d, 1, in.D+1) {
        ll t = out.t[d];
        sat.last[t] = d;
        if (d > 1){
            sat.score[d] = sat.score[d-1];
        }
        sat.score[d] += in.s[d][t];
        rep(i, 1, 27){
            sat.score[d] -= in.c[i] * (d - sat.last[i]);
        }
    }
}

int main(){
    ll D;
    cin >> D;
    Input in(D);
    rep(i, 1, 27) cin >> in.c[i];
    rep(d, 1, D+1) rep(i, 1, 27) cin >> in.s[d][i];
    Output out(D);
    rep(d, 1, D+1) cin >> out.t[d];

    Satisfaction sat(D);
    calc_score(in, out, sat);
    sat.print();
}