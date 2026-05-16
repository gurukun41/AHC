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

void yn(bool a) {
    if (a)
        cout << "Yes\n";
    else
        cout << "No\n";
}

template <typename T>
vector<T> read_vector(int n) {
    vector<T> a(n);
    rep(i, 0, n) cin >> a[i];
    return a;
}

template <typename T>
vector<vector<T>> read_matrix(int h, int w) {
    vector<vector<T>> a(h, vector<T>(w));
    rep(i, 0, h) rep(j, 0, w) cin >> a[i][j];
    return a;
}

template <typename T>
void print_vector(const vector<T> &v, string sep = " ", string end = "\n") {
    rep(i, 0, v.size()) {
        if (i) cout << sep;
        cout << v[i];
    }
    cout << end;
}

template <typename T>
void print_lines(const vector<T> &v) {
    for (const T &x : v) cout << x << "\n";
}

bool inside(int x, int y, int h, int w) {
    return 0 <= x && x < h && 0 <= y && y < w;
}

vl t(100);  // 来る順番
mt19937 rng(0); // 乱数


pl getPosition(ll num, const vvl& map){
    ll count = 0;
    rep(i,0,10){
        rep(j,0,10){
            if(map[i][j] == -1){
                count++;
            }
            if(count == num){
                return pl{i,j};
            }
        }
    }
    return pl{-1,-1};
}

void change(vvl& map, char dir){
    if(dir == 'L'||dir == 'R'){
        vvl newMap(10,vl(10,-1));
        rep(i,0,10){
            ll now = 0;
            if(dir == 'R'){
                now = 9;
            }
            rep(j,0,10){
                if(dir == 'R'){
                    j = 9-j;
                }
                if(map[i][j] != -1){
                    newMap[i][now] = map[i][j];
                    if(dir == 'L'){
                        now++;
                    } else {
                        now--;
                    }
                }
                if(dir == 'R'){
                    j = 9-j;
                }
            }
        }
        map = newMap;
    } else {
        vvl newMap(10,vl(10,-1));
        rep(j,0,10){
            ll now = 0;
            if(dir == 'B'){
                now = 9;
            }
            rep(i,0,10){
                if(dir == 'B'){
                    i = 9-i;
                }
                if(map[i][j] != -1){
                    newMap[now][j] = map[i][j];
                    if(dir == 'F'){
                        now++;
                    } else {
                        now--;
                    }
                }
                if(dir == 'B'){
                    i = 9-i;
                }
            }
        }
        map = newMap;
    }
}

ll getScore(const vvl& map){
    ll countSpace = 0;
    vpl dir = {pl{1,0}, pl{-1,0}, pl{0,1}, pl{0,-1}};
    atcoder::dsu d(100);
    rep(i,0,10){
        rep(j,0,10){
            if(map[i][j] == -1) {
                countSpace++;
                continue;
            } else {
                rep(k,0,4){
                    ll ni = i+dir[k].first;
                    ll nj = j+dir[k].second;
                    if(inside(ni,nj,10,10)){
                        if(map[i][j] == map[ni][nj]){
                            d.merge(i*10+j, ni*10+nj);
                        }
                    }
                }
            }
        }
    }
    vl sz(100, 0);
    rep(i,0,10){
        rep(j,0,10){
            if(map[i][j] == -1) continue;
            sz[d.leader(i*10+j)]++;
        }
    }

    ll score = 0;
    rep(i,0,100){
        score += sz[i] * sz[i];
    }
    return score;
}

char decideDir(ll turn, const vvl& map){
    string dirs = "FRBL";
    ll maxScore = 0;
    ll maxDir = turn%4;
    rep(i,turn,turn+4){
        vvl premap = map;
        change(premap,dirs[i%4]);
        ll score = getScore(premap);
        if(maxScore < score){
            maxScore = score;
            maxDir = i%4;
        }
    }
    return dirs[maxDir];
}

vvl makeScenarios(ll turn, ll depthMax, ll width){
    ll depth = min(depthMax, 99 - turn);

    vvl scenarios(width, vl(depth));

    rep(i, 0, width){
        rep(j, 0, depth){
            ll empty = 100 - (turn + 1 + j);

            uniform_int_distribution<ll> dist(1, empty);
            scenarios[i][j] = dist(rng);
        }
    }

    return scenarios;
}

ld monteCarloScore(const vvl& map, ll turn, char firstDir, const vvl& scenarios){
    ld sum = 0;

    rep(i,0,scenarios.size()){
        vvl tmp = map;

        change(tmp, firstDir);

        rep(j, 0, scenarios[i].size()){
            pl point = getPosition(scenarios[i][j], tmp);

            tmp[point.first][point.second] = t[turn + 1 + j];

            char dir = decideDir(turn + 1 + j, tmp);
            change(tmp, dir);
        }

        sum += getScore(tmp);
    }

    return sum / scenarios.size();
}

void onePhase(vvl& map, ll turn){
    ll in;cin >> in;
    pl point = getPosition(in, map);
    map[point.first][point.second] = t[turn];

    string dirs = "FRBL";

    ll depth = 3;
    ll width = 40;

    auto scenarios = makeScenarios(turn, depth, width);

    ld maxScore = 0;
    char maxDir = dirs[turn % 4];

    for(char dir : dirs){
        ld score = monteCarloScore(map, turn, dir, scenarios);

        if(score > maxScore){
            maxScore = score;
            maxDir = dir;
        }
    }

    cout << maxDir << endl;
    change(map, maxDir);
}

void solve(){
    vvl map(10,vl(10,-1));
    rep(i,0,100){
        onePhase(map, i);
    }
}

int main(){
    rep(i,0,100) cin >> t[i];
    solve();
}