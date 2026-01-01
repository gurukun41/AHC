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
vector<vector<vector<unsigned long long>>> zobrist;

struct MachineState {
    ll num;
    ll power;
};

struct State {
    ll sum;
    ll weighted_power;
    ll next_cost_sum; 
    unsigned long long hash; 
    vector<vector<MachineState>> machines;

    State() {
        sum = K;
        weighted_power = 0;
        next_cost_sum = 0; 
        hash = 0;
        machines.resize(L, vector<MachineState>(N));
        rep(i, 0, L) {
            rep(j, 0, N) {
                machines[i][j] = {1, 0};
                next_cost_sum += C[i][j]; 
                if (!zobrist.empty()) { 
                    hash ^= zobrist[i][j][0];
                }
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

        hash ^= zobrist[i][j][machines[i][j].power];
        machines[i][j].power++;
        hash ^= zobrist[i][j][machines[i][j].power];
        
        next_cost_sum += C[i][j];

        ll weight = 1;
        rep(k, 0, i) {
            weight *= machines[k][j].power;
        }
        weighted_power += weight;
    }

    bool operator<(const State& other) const {
        return (unsigned __int128)weighted_power < (unsigned __int128)other.weighted_power ;
    }

    bool operator>(const State& other) const {
        return (unsigned __int128)weighted_power > (unsigned __int128)other.weighted_power;
    }
};

struct Node {
    State state;
    shared_ptr<Node> parent;
    pl op; 

    Node(const State& s, shared_ptr<Node> p, pl o) : state(s), parent(p), op(o) {}

    bool operator>(const Node& other) const {
        return state > other.state;
    }
};

void solve() {
    const int BEAM_WIDTH = 70;
    
    vector<shared_ptr<Node>> beam;
    beam.push_back(make_shared<Node>(State(), nullptr, make_pair(-1, -1)));

    rep(t, 0, T) {
        map<unsigned long long, shared_ptr<Node>> candidates_map;
        
        for (const auto& curr_node : beam) {
            const State& curr_state = curr_node->state;

            auto add_candidate = [&](State& next_state, pl op) {

                unsigned long long h = next_state.hash;
                if (candidates_map.find(h) == candidates_map.end()) {
                    candidates_map[h] = make_shared<Node>(next_state, curr_node, op);
                } else {
                    if (next_state > candidates_map[h]->state) {
                        candidates_map[h] = make_shared<Node>(next_state, curr_node, op);
                    }
                }
            };

            {
                State next_state = curr_state;
                next_state.simulate_turn();
                add_candidate(next_state, {-1, -1});
            }

            rep(i, 0, L) {
                rep(j, 0, N) {
                    if (curr_state.can_upgrade(i, j)) {
                        State next_state = curr_state;
                        next_state.upgrade(i, j);
                        next_state.simulate_turn();
                        add_candidate(next_state, {i, j});
                    }
                }
            }
        }

        vector<shared_ptr<Node>> next_beam;
        next_beam.reserve(candidates_map.size());
        for (auto const& [key, val] : candidates_map) {
            next_beam.push_back(val);
        }

        sort(all(next_beam), [](const shared_ptr<Node>& a, const shared_ptr<Node>& b) {
            return *a > *b;
        });

        if (next_beam.size() > BEAM_WIDTH) {
            next_beam.resize(BEAM_WIDTH);
        }
        beam = next_beam;
    }

    if (!beam.empty()) {
        vector<pl> history;
        auto curr = beam[0];
        
        while (curr->parent != nullptr) {
            history.push_back(curr->op);
            curr = curr->parent;
        }
        
        reverse(all(history));

        for (const auto& op : history) {
            if (op.first == -1) {
                cout << -1 << "\n";
            } else {
                cout << op.first << " " << op.second << "\n";
            }
        }
        cerr << "Final Sum: " << beam[0]->state.sum << "\n";
    }
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    cin >> N >> L >> T >> K;
    
    mt19937_64 rng(chrono::steady_clock::now().time_since_epoch().count());
    zobrist.resize(L, vector<vector<unsigned long long>>(N, vector<unsigned long long>(T + 5)));
    rep(i, 0, L) {
        rep(j, 0, N) {
            rep(k, 0, T + 5) {
                zobrist[i][j][k] = rng();
            }
        }
    }

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