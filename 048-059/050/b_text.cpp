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
    string seedNum = "1253";
    // 入力ファイルと出力ファイルを開く
    ifstream input_file(seedNum + ".txt");
    ofstream output_file("b_" + seedNum + ".txt");

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

    // 囲ってみる
    vector<vector<bool>> visited(N, vector<bool>(N, false));

    int top = 0, bottom = N - 1, left = 0, right = N - 1;

    while (top <= bottom && left <= right) {
        // 上の行を左から右へ
        for (int j = left; j <= right; j++) {
            if (S[top][j] && !visited[top][j]) {
                output_file << top << " " << j << "\n";
                visited[top][j] = true;
            }
        }
        top++;

        // 右の列を上から下へ
        for (int i = top; i <= bottom; i++) {
            if (S[i][right] && !visited[i][right]) {
                output_file << i << " " << right << "\n";
                visited[i][right] = true;
            }
        }
        right--;

        // 下の行を右から左へ
        if (top <= bottom) {
            for (int j = right; j >= left; j--) {
                if (S[bottom][j] && !visited[bottom][j]) {
                    output_file << bottom << " " << j << "\n";
                    visited[bottom][j] = true;
                }
            }
            bottom--;
        }

        // 左の列を下から上へ
        if (left <= right) {
            for (int i = bottom; i >= top; i--) {
                if (S[i][left] && !visited[i][left]) {
                    output_file << i << " " << left << "\n";
                    visited[i][left] = true;
                }
            }
            left++;
        }
    }

    output_file.close();
}