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



int main(){
    string seedNum = "1345";
    // 入力ファイルと出力ファイルを開く
    ifstream input_file(seedNum + ".txt");
    ofstream output_file("output/d_" + seedNum + ".txt");
    int N,M;input_file >> N >> M;
    vector<vector<bool>> S(N,vector<bool>(N));        // マス目trueなら通過可能
    vector<pair<int,int>> R(M); // いわ
    int Rnum = 0;
    rep(i,0,N){
        rep(j,0,N){
            char c;input_file >> c;
            if(c == '.'){
                S[i][j] = true;
            }
            else{
                S[i][j] = false;
                R[Rnum] = make_pair(i,j);
                Rnum++;
            }
        }
    }


    // 岩の上or横
    rep(c,0,M){
        int ri = R[c].first, rj = R[c].second;
        if(ri > 0 && ri < N){
            rep(j,0,N){
                if(S[ri][j]){
                    S[ri][j] = false;
                    output_file << ri << " " << j <<"\n";
                }
            }
        }
        else{
            rep(i,0,N){
                if(S[i][rj]){
                    S[i][rj] = false;
                    output_file << i << " " << rj <<"\n";
                }
            }
        }
    }
    // 残ったもの
    rep(i,0,N){
        rep(j,0,N){
            if(S[i][j]){
                output_file << i << " " << j << "\n";
            }
        }
    }
}