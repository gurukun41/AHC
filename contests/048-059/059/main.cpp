#include <bits/stdc++.h>
#include <atcoder/all>
using namespace std;
using ll = long long;
using pl = pair<ll, ll>;
using vpl = vector<pl>;
using vl = vector<ll>;
using vvl = vector<vl>;

#define rep(i, a, b) for (ll i = (a); i < (ll)(b); i++)
#define all(v) v.begin(), v.end()

ll N = 20;
vvl initial_board(20, vl(20));

struct CardPair {
    ll val;
    pl p1, p2;
    bool cleared;
};

// シミュレーション結果を保持する構造体
struct Result {
    ll score;
    ll pair_count;
    vector<char> moves;
};

// 指定した順序でシミュレーションを実行する関数
Result simulate(const vector<int>& order, const vector<CardPair>& original_pairs) {
    vector<CardPair> pairs = original_pairs;
    vvl now = initial_board;
    ll i = 0, j = 0;
    ll turn = 0;
    ll count = 0;
    vector<char> moves;
    stack<ll> st;

    auto move_to = [&](pl target) {
        while (pl{i, j} != target && turn < 16000) {
            if (target.first < i) { moves.push_back('U'); i--; }
            else if (target.first > i) { moves.push_back('D'); i++; }
            else if (target.second < j) { moves.push_back('L'); j--; }
            else if (target.second > j) { moves.push_back('R'); j++; }
            turn++;
        }
    };

    auto action_z = [&]() {
        if (turn >= 16000) return;
        moves.push_back('Z');
        ll val = now[i][j];
        if (!st.empty() && st.top() == val) {
            st.pop();
            count++;
        } else {
            st.push(val);
        }
        now[i][j] = -1;
        turn++;
    };

    vector<bool> pair_done(pairs.size(), false);

    for (int idx : order) {
        if (pair_done[idx]) continue;
        if (turn >= 16000) break;

        CardPair& p = pairs[idx];
        
        // A1へ移動
        move_to(p.p1);
        action_z();

        // A1からA2への経路の矩形範囲内に他のペアがあるかチェック（ネスト）
        ll r_min = min(p.p1.first, p.p2.first), r_max = max(p.p1.first, p.p2.first);
        ll c_min = min(p.p1.second, p.p2.second), c_max = max(p.p1.second, p.p2.second);

        for (int next_idx : order) {
            if (pair_done[next_idx] || next_idx == idx) continue;
            CardPair& np = pairs[next_idx];
            // 両方のカードが矩形内にあればネスト回収
            if (np.p1.first >= r_min && np.p1.first <= r_max && np.p1.second >= c_min && np.p1.second <= c_max &&
                np.p2.first >= r_min && np.p2.first <= r_max && np.p2.second >= c_min && np.p2.second <= c_max) {
                
                move_to(np.p1);
                action_z();
                move_to(np.p2);
                action_z();
                pair_done[next_idx] = true;
            }
        }

        // 本来の目的地 A2へ移動
        move_to(p.p2);
        action_z();
        pair_done[idx] = true;
    }

    return {turn, count, moves};
}

void solve() {
    // 1. ペア情報の抽出
    map<ll, vpl> card_positions;
    rep(r, 0, N) rep(c, 0, N) {
        card_positions[initial_board[r][c]].push_back({r, c});
    }

    vector<CardPair> pairs;
    for (auto const& [val, pos] : card_positions) {
        pairs.push_back({val, pos[0], pos[1], false});
    }

    int M = pairs.size();
    vector<int> current_order(M);
    iota(all(current_order), 0);

    // 初期の貪欲解（近い順に並び替え）
    // ここでは単純な初期解からスタート
    Result best_res = simulate(current_order, pairs);

    // 2. 焼きなましループ
    auto start_time = chrono::system_clock::now();
    double duration = 0;
    int iter = 0;
    
    // 時間制限 (例: 1.8秒)
    const double TIME_LIMIT = 1800.0; 

    random_device rd;
    mt19937 engine(rd());
    
    while (true) {
        auto now_time = chrono::system_clock::now();
        duration = chrono::duration_cast<chrono::milliseconds>(now_time - start_time).count();
        if (duration > TIME_LIMIT) break;

        vector<int> next_order = current_order;
        
        // 近傍: 2点スワップ
        int idx1 = engine() % M;
        int idx2 = engine() % M;
        swap(next_order[idx1], next_order[idx2]);

        Result next_res = simulate(next_order, pairs);

        // 評価: ペア数が多いほど良く、同じなら手数が少ないほど良い
        bool accept = false;
        if (next_res.pair_count > best_res.pair_count) {
            accept = true;
        } else if (next_res.pair_count == best_res.pair_count) {
            if (next_res.score < best_res.score) {
                accept = true;
            } else {
                // 焼きなましの確率判定（簡易版）
                double temp = (TIME_LIMIT - duration) / TIME_LIMIT;
                double prob = exp((double)(best_res.score - next_res.score) / (temp * 10.0 + 0.1));
                if (prob > (double)(engine() % 1000) / 1000.0) {
                    accept = true;
                }
            }
        }

        if (accept) {
            current_order = next_order;
            if (next_res.pair_count > best_res.pair_count || 
               (next_res.pair_count == best_res.pair_count && next_res.score < best_res.score)) {
                best_res = next_res;
            }
        }
        iter++;
    }

    // 最終結果の出力
    for (char m : best_res.moves) {
        cout << m << "\n";
    }
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    if (!(cin >> N)) return 0;
    initial_board.assign(N, vl(N));
    rep(i, 0, N) rep(j, 0, N) cin >> initial_board[i][j];
    solve();
    return 0;
}