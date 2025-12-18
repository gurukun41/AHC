#include <bits/stdc++.h>

#include <atcoder/all>
using namespace std;
using ll = long long;
using vl = vector<ll>;                                     // long long型の一次元
using vvl = vector<vl>;                                    // long long型の二次元配列
using vvvl = vector<vvl>;                                  // long long型の三次元配列
using vi = vector<int>;                                    // int型の一次元
using vvi = vector<vi>;                                    // int型の二次元配列
using vvvi = vector<vvi>;                                  // int型の三次元配列
#define rep(i, a, b) for (int i = (a); i < (int)(b); i++)  // for文の短縮
#define all(v) v.begin(), v.end()                          // all(v)でvの始まりと終わりのイテレーター

// 入力を受け取る
template <typename T>
T input() {
    T x;
    cin >> x;
    return x;
}

// a,bのうち最大のものをaに入れる(aがbに置き換わるときはtrueを返す)
template <typename T>
inline bool chmax(T &a, const T &b) {
    if (a < b) {
        a = b;
        return true;
    }
    return false;
}

// a,bのうち最小のものをaに入れる(aがbに置き換わるときはtrueを返す)
template <typename T>
inline bool chmin(T &a, const T &b) {
    if (a > b) {
        a = b;
        return true;
    }
    return false;
}

// 素数判定
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

int main() {
    int N, M;
    cin >> N >> M;
    vector<vector<bool>> S(N, vector<bool>(N));  // マス目trueなら通過可能
    vector<pair<int, int>> R(M);                 // いわ
    int Rnum = 0;
    rep(i, 0, N) {
        rep(j, 0, N) {
            char c;
            cin >> c;
            if (c == '.') {
                S[i][j] = true;
            } else {
                S[i][j] = false;
                R[Rnum] = make_pair(i, j);
                Rnum++;
            }
        }
    }

    // 周囲に岩がないなら置く
    rep(i, 1, N - 1) {
        rep(j, 1, N - 1) {
            if (!S[i][j]) continue;
            if (!S[i - 1][j - 1] || !S[i - 1][j + 1] || !S[i + 1][j - 1] || !S[i + 1][j + 1]) {
                continue;
            } else {
                cout << i << " " << j << "\n";
                S[i][j] = false;
            }
        }
    }

    // 左右または上下に岩がないなら置く
    rep(i, 1, N-1) {
        rep(j, 1, N-1) {
            if (!S[i][j]) continue;
            if ((S[i][j - 1] && S[i][j + 1]) || (S[i - 1][j] && S[i + 1][j])) {
                cout << i << " " << j << "\n";
                S[i][j] = false;
            } else {
                continue;
            }
        }
    }

    // チェックがら
    rep(i, 0, N) {
        rep(j, 0, N) {
            if (S[i][j] && (i + j) % 2 == 1) {
                cout << i << " " << j << "\n";
            }
        }
    }

    rep(i, 0, N) {
        rep(j, 0, N) {
            if (S[i][j] && (i + j) % 2 == 0) {
                cout << i << " " << j << "\n";
            }
        }
    }
}