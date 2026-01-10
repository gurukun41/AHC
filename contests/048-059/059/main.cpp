#include <bits/stdc++.h>
#include <atcoder/all>
using namespace std;
using ll = long long;
using ld = long double;
using mint = atcoder::modint998244353;
using vl = vector<ll>;
using vvl = vector<vl>;
using vvvl = vector<vvl>;
using vi = vector<int>;
using vvi = vector<vi>;
using vvvi = vector<vvi>;
using vb = vector<bool>;
using vvb = vector<vb>;
using vvvb = vector<vvb>;
using vs = vector<string>;
using vvs = vector<vs>;
using pl = pair<ll, ll>;
using vpl = vector<pl>;
#define rep(i, a, b) for (ll i = (a); i < (ll)(b); i++)
#define all(v) v.begin(), v.end()

template <typename T>
inline bool chmax(T &a, const T &b) {
    if (a < b) {
        a = b;
        return true;
    }
    return false;
}

template <typename T>
inline bool chmin(T &a, const T &b) {
    if (a > b) {
        a = b;
        return true;
    }
    return false;
}


ll N = 20;
vvl a(20, vl(20));


void solve() {
    vector<char> moves; // ターンごとの行動 U, D, L, Rで移動、Zで取る、Xで置く
    ll i = 0, j = 0; // 現在位置
    stack<ll> st; // 山札
    vvl now = a; // 現在の盤面
    ll count = 0; // 取ったカードの組み数
    ll turn = 0; // ターン数

    pl target = {-1,-1}; // 目標位置
    while (turn < 16000 && count < 200) {
        if(target != pl{-1,-1} && target != pl{i,j}){
            // 目標位置に向かう
            if(target.first < i){
                moves.push_back('U');
                i--;
            } else if(target.first > i){
                moves.push_back('D');
                i++;
            } else if(target.second < j){
                moves.push_back('L');
                j--;
            } else if(target.second > j){
                moves.push_back('R');
                j++;
            }
            turn++;
            continue;
        } else if(target != pl{-1,-1} && target == pl{i,j}){
            target = pl{-1,-1};
            moves.push_back('Z');
            if(!st.empty() && st.top() == now[i][j]){
                st.pop();
                count++;
            } else {
                st.push(now[i][j]);
            }
            now[i][j] = -1;
            turn++;
            continue;
        }

        if(st.empty()) {
            ll min_dist = 1e9;
            pl best = {-1, -1};
            rep(ni,0,N) rep(nj,0,N) {
                if(now[ni][nj] != -1) {
                    ll dist = abs(i - ni) + abs(j - nj);
                    if(dist < min_dist) {
                        min_dist = dist;
                        best = pl{ni, nj};
                    }
                }
            }
            target = best;
        } else {
            rep(k,0,(N-1)*(N-1)+1){
                rep(ni,0,k+1){
                    if(0<=ni && ni<N && 0<=k-ni && k-ni<N ){
                        if(now[ni][k-ni] == st.top()){
                            target = pl{ni,k-ni};
                            break;
                        }
                    }
                }
                if(target != pl{-1,-1}){
                    break;
                }
            }
        }
    }

    rep(index,0,moves.size()) {
        cout << moves[index] << "\n";
    }
}

int main(){
    cin >> N;
    a.resize(N, vl(N));
    rep(i,0,N)rep(j,0,N) cin >> a[i][j];
    solve();
}