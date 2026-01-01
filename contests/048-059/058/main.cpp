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

struct MachineState {
    ll num;
    ll power;
};

struct State {
    ll sum;
    ll total_power;
    vector<vector<MachineState>> machines;
    vector<pl> history;

    State() {
        sum = K;
        total_power = 0;
        machines.resize(L, vector<MachineState>(N));
        rep(i, 0, L) {
            rep(j, 0, N) {
                machines[i][j] = {1, 0};
            }
        }
    }

    void simulate_turn() {
        rep(j, 0, N) {
            sum += A[j] * machines[0][j].num * machines[0][j].power;
            rep(i, 1, L) {
                machines[i - 1][j].num += machines[i][j].num * machines[i][j].power;
            }
        }
    }

    bool can_upgrade(int i, int j) const {
        return sum >= C[i][j] * (machines[i][j].power + 1);
    }

    void upgrade(int i, int j) {
        ll cost = C[i][j] * (machines[i][j].power + 1);
        sum -= cost;
        machines[i][j].power++;
        total_power++;
    }

    // レベルの総和 * 所持金 で比較
    bool operator<(const State& other) const {
        return (unsigned __int128) total_power < (unsigned __int128) other.total_power;
    }
    
    bool operator>(const State& other) const {
        return (unsigned __int128) total_power > (unsigned __int128) other.total_power;
    }
};

void solve() {
    const int BEAM_WIDTH = 50;
    vector<State> beam;
    beam.push_back(State());

    rep(t, 0, T) {
        vector<State> next_beam;
        
        for (const auto& curr : beam) {
            {
                State next_state = curr;
                next_state.history.push_back({-1, -1});
                next_state.simulate_turn();
                next_beam.push_back(next_state);
            }

            rep(i, 0, L) {
                rep(j, 0, N) {
                    if (curr.can_upgrade(i, j)) {
                        State next_state = curr;
                        next_state.upgrade(i, j);
                        next_state.history.push_back({i, j});
                        next_state.simulate_turn();
                        next_beam.push_back(next_state);
                    }
                }
            }
        }

        sort(all(next_beam), greater<State>());
        if (next_beam.size() > BEAM_WIDTH) {
            next_beam.resize(BEAM_WIDTH);
        }
        beam = next_beam;
    }

    if (!beam.empty()) {
        const auto& best_state = beam[0];
        for (const auto& op : best_state.history) {
            if (op.first == -1) {
                cout << -1 << "\n";
            } else {
                cout << op.first << " " << op.second << "\n";
            }
        }
        cerr << "Final Sum: " << best_state.sum << "\n";
    }
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
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