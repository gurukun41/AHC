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

ll N, L, T, K;
vl A;
vvl C;

struct machine {
    ll index_i;
    ll index_j;
    ll num;
    ll power;
    machine *lower;
    machine *higher;
    machine(){
        index_i = -1;
        index_j = -1;
        num = 0;
        power = 0;
        lower = nullptr;
        higher = nullptr;
    }
    machine(ll i, ll j, machine *l = nullptr, machine *h = nullptr) {
        index_i = i;
        index_j = j;
        num = 1;
        power = 0;
        lower = l;
        higher = h;
    }
    int update() {
        if (lower == nullptr) {
            ll ret = A[index_j] * num * power;
            if(higher != nullptr) higher->update();
            return ret;
        } else {
            lower->num += num*power;
            if(higher != nullptr) higher->update();
            return 0;
        }
    }
};

struct state {
    ll turn;
    ll sum;
    vector<vector<machine>> machines;
    state() {
        turn = 0;
        sum = K;
        machines.resize(L);
        rep(i,0,L){
            machines[i].resize(N);
            rep(j,0,N){
                if(i == 0){
                    machines[i][j] = machine(i,j,nullptr,nullptr);
                } else if(i == L-1){
                    machines[i][j] = machine(i,j,&machines[i-1][j],nullptr);
                } else {
                    machines[i][j] = machine(i,j,&machines[i-1][j],&machines[i+1][j]);
                }
            }
        }
    }
    void next_turn(ll ti = -1, ll tj = -1) {
        assert(turn < T);
        if(ti != -1 && tj != -1){
            sum -= C[ti][tj]*(machines[ti][tj].power+1);
            assert(sum >= 0);
            machines[ti][tj].power++;
        }
        rep(i,0,L){
            sum += machines[i][0].update();
        }
        turn++;
    }
};


void solve() {
    state st;
    rep(_,0,T){
        ll ti = -1, tj = -1;
        ll min = LLONG_MAX;
        rep(i, 0, L){
            rep(j, 0, N){
                if(st.sum >= C[i][j]*(st.machines[i][j].power+1)){
                    if(chmin(min, C[i][j]*(st.machines[i][j].power+1))){
                        ti = i;
                        tj = j;
                    }
                }
            }
        }
        st.next_turn(ti, tj);
        if(ti != -1 && tj != -1){
            cout << ti << " " << tj << "\n";
        } else {
            cout << -1 << "\n";
        }
        cerr << "Final Sum: " << st.sum << "\n";
    }
}

int main() {
    cin >> N >> L >> T >> K;
    A.resize(N);
    rep(i,0,N) {
        cin >> A[i];
    }
    C.resize(L);
    rep(i,0,L){
        C[i].resize(N);
        rep(j,0,N){
            cin >> C[i][j];
        }
    }
    solve();
}