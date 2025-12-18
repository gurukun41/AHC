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
#define rep(i,a,b) for(ll i = (a); i < (ll)(b); i++)    //for文の短縮
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

// 最初に受け取る入力
// 森の縦横幅と伝説の花の座標とマップ情報
struct Input {
    ll N;
    pair<ll, ll> target;
    vector<string> b;
    Input() {
        cin >> N;
        cin >> target.first >> target.second;
        b.resize(N);
        rep(i, 0, N) {
            cin >> b[i];
        }
    }
};

// 各ターンの開始時に与えられる標準入力
// 冒険者の現在位置と前のターンに新たに確認済みとなったマスの集合
struct Input2 {
    pair<ll, ll> curr;
    ll n;
    vector<pair<ll, ll>> points;
    void getInput2() {
        cin >> curr.first >> curr.second;
        cin >> n;
        points.resize(n);
        rep(i, 0, n) {
            cin >> points[i].first >> points[i].second;
        }
    }
};

// 各ターンに出す出力
// 新たにトレントを配置するマスの集合
struct Output {
    ll m;
    vector<pair<ll, ll>> points;
    void print() {
        cout << m << " ";
        rep(i, 0, m) {
            cout << points[i].first << " " << points[i].second;
            if (i != m - 1) {
                cout << " ";
            }
            else {
                cout << endl;
            }
        }
    }
};

struct Map {
    ll N;
    vvl mapInfo;                // 0: 木, 1: 空きマス
    vvl searched;               // 確認済みのマス
    pair<ll, ll> curr;          // 冒険者の現在位置
    pair<ll, ll> target;        // 伝説の花の位置
    pair<ll, ll> destination;   // 次に向かうマス(-1,-1なら未定)
    bool test;                  // 提出かどうか(trueなら提出でない)
    Map(Input& in, bool test_ = false) : test(test_) {
        N = in.N;
        mapInfo.resize(N, vl(N, 0));
        rep(i,0,N) {
            rep(j,0,N) {
                if(in.b[i][j] == '.') {
                    mapInfo[i][j] = 1;
                }
                else {
                    mapInfo[i][j] = 0;
                }
            }
        }
        searched.resize(N, vl(N, 0));
        target = in.target;
        curr = {0, N/2};
        destination = {-1, -1};
    }
    void update() {
        if(!test) {
            Input2 in2;
            in2.getInput2();
            curr = in2.curr;
            rep(i,0,in2.n) {
                searched[in2.points[i].first][in2.points[i].second] = 1;
            }
        }
        else{

        }
    }
};

int main(){
    Input in;
    return 0;
}