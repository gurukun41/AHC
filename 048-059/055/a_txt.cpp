#include <bits/stdc++.h>
#include <atcoder/all>
using namespace std;
using ll = long long;
using ld = long double;
using mint = atcoder::modint998244353;
using vl = vector<ll>;                                   // long long型の一次元
using vvl = vector<vl>;                                  // long long型の二次元配列
using vvvl = vector<vvl>;                                // long long型の三次元配列
using vi = vector<int>;                                  // int型の一次元
using vvi = vector<vi>;                                  // int型の二次元配列
using vvvi = vector<vvi>;                                // int型の三次元配列
using vb = vector<bool>;                                 // bool型の一次元
using vvb = vector<vb>;                                  // bool型の二次元配列
using vvvb = vector<vvb>;                                // bool型の三次元配列
using vs = vector<string>;                               // string型の一次元
using vvs = vector<vs>;                                  // string型の二次元配列
using pl = pair<ll, ll>;                                 // long long型のペア
using vpl = vector<pl>;                                  // long long型のペアの一次元配列
#define rep(i, a, b) for (ll i = (a); i < (ll)(b); i++)  // for文の短縮
#define all(v) v.begin(), v.end()                        // all(v)でvの始まりと終わりのイテレーター

// 無限大の値
const long long INF = 1LL << 60;

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

// Yes/Noを出力
void yn(bool a) {
    if (a)
        cout << "Yes\n";
    else
        cout << "No\n";
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

// 遅延セグメント木(和)
struct LazySegmentTree_Sum {
   private:
    ll size = 1;
    vl node;
    vl lazy;
    ll init_value = 0;

   public:
    // 配列を指定して初期化
    LazySegmentTree_Sum(vl v) {
        ll sz = v.size();
        while (size < sz) size *= 2;
        node.resize(2 * size - 1, init_value);
        lazy.resize(2 * size - 1, init_value);
        rep(i, 0, sz) node[i + size - 1] = v[i];
        for (ll i = size - 2; i >= 0; i--) {
            node[i] = node[2 * i + 1] + node[2 * i + 2];
        }
    }
    // サイズのみ指定
    LazySegmentTree_Sum(ll n) : LazySegmentTree_Sum(vl(n, 0)) {}

    void eval(ll k, ll l, ll r) {
        if (lazy[k] != 0) {
            node[k] += lazy[k] * (r - l);
            if (r - l > 1) {
                lazy[2 * k + 1] += lazy[k];
                lazy[2 * k + 2] += lazy[k];
            }
            lazy[k] = 0;
        }
    }

    // 区間[a, b)にvalを加算
    void update(ll a, ll b, ll val, ll k = 0, ll l = 0, ll r = -1) {
        if (r < 0) r = size;
        eval(k, l, r);
        if (r <= a || b <= l) return;
        if (a <= l && r <= b) {
            lazy[k] += val;
            eval(k, l, r);
        } else {
            update(a, b, val, 2 * k + 1, l, (l + r) / 2);
            update(a, b, val, 2 * k + 2, (l + r) / 2, r);
            node[k] = node[2 * k + 1] + node[2 * k + 2];
        }
    }

    // 区間[a, b)の和を取得
    ll query(ll a, ll b, ll k = 0, ll l = 0, ll r = -1) {
        if (r < 0) r = size;

        eval(k, l, r);

        if (r <= a || b <= l) return 0;
        if (a <= l && r <= b) return node[k];

        ll vl = query(a, b, 2 * k + 1, l, (l + r) / 2);
        ll vr = query(a, b, 2 * k + 2, (l + r) / 2, r);
        return vl + vr;
    }
};

// 遅延セグメント木(最小値)
struct LazySegmentTree_Min {
   private:
    ll size = 1;
    vl node;
    vl lazy;
    vb lazyFlag;
    ll init_value = INF;

   public:
    LazySegmentTree_Min(vl v) {
        ll sz = v.size();
        while (size < sz) size *= 2;
        node.resize(2 * size - 1, init_value);
        lazy.resize(2 * size - 1, init_value);
        lazyFlag.resize(2 * size - 1, false);

        rep(i, 0, sz) node[i + size - 1] = v[i];
        for (ll i = size - 2; i >= 0; i--) {
            node[i] = min(node[2 * i + 1], node[2 * i + 2]);
        }
    }
    LazySegmentTree_Min(ll n) : LazySegmentTree_Min(vl(n, INF)) {}

    void eval(ll k, ll l, ll r) {
        if (lazyFlag[k]) {
            node[k] = lazy[k];
            if (r - l > 1) {
                lazy[2 * k + 1] = lazy[k];
                lazy[2 * k + 2] = lazy[k];
                lazyFlag[2 * k + 1] = true;
                lazyFlag[2 * k + 2] = true;
            }
            lazyFlag[k] = false;
        }
    }

    void update(ll a, ll b, ll val, ll k = 0, ll l = 0, ll r = -1) {
        if (r < 0) r = size;
        eval(k, l, r);
        if (r <= a || b <= l) return;
        if (a <= l && r <= b) {
            lazy[k] = val;
            lazyFlag[k] = true;
            eval(k, l, r);
        } else {
            update(a, b, val, 2 * k + 1, l, (l + r) / 2);
            update(a, b, val, 2 * k + 2, (l + r) / 2, r);
            node[k] = min(node[2 * k + 1], node[2 * k + 2]);
        }
    }

    ll query(ll a, ll b, ll k = 0, ll l = 0, ll r = -1) {
        if (r < 0) r = size;

        eval(k, l, r);

        if (r <= a || b <= l) return INF;
        if (a <= l && r <= b) return node[k];

        ll vl = query(a, b, 2 * k + 1, l, (l + r) / 2);
        ll vr = query(a, b, 2 * k + 2, (l + r) / 2, r);
        return min(vl, vr);
    }
};

struct Input{
    ll N;
    vl H;
    vl C;
    vvl A;
    Input(){
        cin >> N;
        H.resize(N);
        C.resize(N);
        A.resize(N, vl(N));
        rep(i,0,N){
            cin >> H[i];
        }
        rep(i,0,N){
            cin >> C[i];
        }
        rep(i,0,N){
            rep(j,0,N){
                cin >> A[i][j];
            }
        }
    }
};

struct Info{
    ll N;

    vpl H;          // 宝箱の硬さ
    vpl C;          // 武器の耐久度
    vector<vpl> A;  // 宝箱に対する武器の攻撃力

    // ソート済みのデータ
    vpl SH;
    vpl SC;
    vector<vpl> SRA;

    ll numWeapon;   // 使える武器の数
    ll minSHi;       // 壊れていない宝箱の最小のインデックス
    ll maxSCi;       // 壊れた武器の最大のインデックス
    Info(Input& in){
        N = in.N;
        H.resize(N+1);
        C.resize(N+1);
        A.resize(N+1, vpl(N));

        rep(i,0,N+1){
            if(i < N) H[i] = {in.H[i], i};
            else H[i] = {0, i};
        }
        SH = H;
        sort(all(SH));

        rep(i,0,N+1){
            if(i < N) C[i] = {in.C[i], i};
            else C[i] = {INF, i};
        }
        SC = C;
        sort(all(SC));
        
        rep(i,0,N+1){
            if(i < N) {
                rep(j,0,N){
                    A[i][j]= {in.A[i][j], j};
                }
            } 
            else {
                rep(j,0,N){
                    A[i][j] = {1, j};
                }
            }
        }
        SRA.resize(N);
        rep(i,0,N){
            rep(j,0,N+1){
                SRA[i].push_back({A[j][i].first, j});
            }
            sort(all(SRA[i]));
        }
        numWeapon = 0;
        minSHi = 1;
        maxSCi = 0;
    }

    // 武器が使えるかどうか
    bool checkUsable(ll sci){
        return (H[SC[sci].second].first == 0 && SC[sci].first > 0);
    }

    //宝箱が全て壊れているかどうか
    bool allBroken(){
        return (minSHi > N);
    }

    ll hiTOshi(ll hi){
        pl target = H[hi];
        auto it = lower_bound(SH.begin(), SH.end(), target);
        if (it != SH.end() && *it == target) {
            return distance(SH.begin(), it);
        }
        return -1; // 見つからない場合
    }

    ll ciTOsci(ll ci){
        pl target = C[ci];
        auto it = lower_bound(SC.begin(), SC.end(), target);
        if (it != SC.end() && *it == target) {
            return distance(SC.begin(), it);
        }
        return -1; // 見つからない場合
    }

    // 武器で宝箱を攻撃する
    bool attackTreasure(ll shi, ll sci){
        // 既に壊れている宝箱は攻撃できない
        if(SH[shi].first == 0) return false;
        // 武器が使えないなら攻撃できない
        if(!checkUsable(sci)) return false;

        // 宝箱の硬さを減らす
        SH[shi].first -= A[SC[sci].second][SH[shi].second].first;
        if(SH[shi].first <= 0) {
            // 壊れた場合
            SH[shi].first = 0;
            numWeapon++;
            minSHi++;
        }
        H[SH[shi].second].first = SH[shi].first;

        // 武器の耐久値を減らす
        SC[sci].first--;
        if(SC[sci].first == 0) {
            // 壊れた場合
            numWeapon--;
            maxSCi++;
        }
        C[SC[sci].second].first = SC[sci].first;

        sort(all(SH));
        sort(all(SC));
        return true;
    }

};

struct Output{
    vl W = {};
    vl B = {};
    void print(){
        rep(i,0,W.size()){
            cout << W[i] << " " << B[i] << "\n";
        }
    }
    void add(ll w, ll b){
        W.push_back(w);
        B.push_back(b);
    }
};

void greedy(Info& info, Output& out){
    while(!info.allBroken()){
        ll shi = info.minSHi;
        ll b = info.SH[shi].second;
        for(ll i = info.N; i >= 0; i--){
            ll tci = info.SRA[info.SH[shi].second][i].second;
            ll tsci = info.ciTOsci(tci);
            if(info.checkUsable(tsci)){
                ll w = info.SC[tsci].second;

                info.attackTreasure(shi, tsci);
                if(w == info.N) w = -1;
                out.add(w, b);
                break;
            }
        }
    }
}

void solve(int seed = -1) {
    if (seed != -1) {
        std::ostringstream oss;
        oss << std::setw(4) << std::setfill('0') << seed;
        string input_filename = "in/" + oss.str() + ".txt";
        string src_filename = __FILE__;
        size_t last_slash = src_filename.find_last_of("/\\");
        string base = (last_slash == string::npos) ? src_filename : src_filename.substr(last_slash + 1);
        size_t under = base.find('_');
        string X = (under == string::npos) ? base : base.substr(0, under);
        string output_filename = "out/" + X + "_" + oss.str() + ".txt";
        freopen(input_filename.c_str(), "r", stdin);
        freopen(output_filename.c_str(), "w", stdout);
    }
    Input in;
    Info info(in);
    Output out;
    greedy(info, out);
    out.print();
}

int main(){
    solve();
}