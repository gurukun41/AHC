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

// 無限大の値
const long long INF = 1LL << 60;

int main() {
    string seedNum = "1253";
    // 入力ファイルと出力ファイルを開く
    ifstream input_file(seedNum + ".txt");
    ofstream output_file("output/c_" + seedNum + ".txt");

    int N, M;
    input_file >> N >> M;
    cout << N << " " << M << "\n";

    vector<vector<bool>> S(N, vector<bool>(N));  // マス目trueなら通過可能
    vector<pair<int, int>> R(M);                 // いわ
    int Rnum = 0;
    rep(i, 0, N) {
        rep(j, 0, N) {
            char c;
            input_file >> c;
            cout << j << "\n";
            if (c == '.') {
                S[i][j] = true;
            } else {
                S[i][j] = false;
                R[Rnum] = make_pair(i, j);
                Rnum++;
            }
        }
    }

    input_file.close();
    cout << "out" << "\n";

    // 右斜め上から出力
    // 各対角線を右上から左下に向かって処理
    for (int d = 0; d < 2 * N - 1; d++) {
        // 対角線d上のマスを処理
        for (int i = 0; i < N; i++) {
            int j = d - i;
            if (j >= 0 && j < N && S[i][j]) {
                output_file << i << " " << j << "\n";
            }
        }
    }

    output_file.close();
}