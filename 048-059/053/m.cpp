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
inline bool chmax(T& a, const T& b) {
    if (a < b) {
        a = b;
        return true;
    }
    return false;
}

// a,bのうち最小のものをaに入れる(aがbに置き換わるときはtrueを返す)
template <typename T>
inline bool chmin(T& a, const T& b) {
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

struct Input {
    ll N;
    ll M;
    ll L;
    ll U;
    Input(ll N_ = 500, ll M_ = 50, ll L_ = 1e15 - 2e12, ll U_ = 1e15 + 2e12) : N(N_), M(M_), L(L_), U(U_) {};
};

struct UsableNumber {
    vl numbers;
    vector<bool> used;
    UsableNumber(Input& in) {
        numbers.resize(in.N);
        ll sep = (in.U - in.L) / ((in.N / 5) * 2);
        ll sepsep = sep / ((in.N / 5) * 3);
        rep(i, 0, in.N / 5) {
            if (i * 3 > in.N / 20 * 11) {
                numbers[i * 3] = sep;
                numbers[i * 3 + 1] = sep;
                numbers[i * 3 + 2] = sep;
            }
            numbers[i * 3] = sepsep * (i + 1);
            numbers[i * 3 + 1] = sepsep * (i + 1);
            numbers[i * 3 + 2] = sepsep * (i + 1);
        }
        rep(i, in.N / 5 * 3, in.N) { numbers[i] = in.L + sep * (i - in.N / 5 * 3 + 1); }
    }
    void output() {
        rep(i, 0, numbers.size()) {
            cout << numbers[i];
            if (i != numbers.size() - 1) {
                cout << " ";
            } else {
                cout << "\n";
            }
        }
    }
};

struct Mountain {
    vl ans;
    vl target;
    vl sum;
    Mountain(Input& in) {
        ans.resize(in.N, -1);
        target.resize(in.M, 0);
        sum.resize(in.M, 0);
    }
    void output() {
        rep(i, 0, ans.size()) {
            cout << ans[i] + 1;
            if (i != ans.size() - 1) {
                cout << " ";
            } else {
                cout << "\n";
            }
        }
    }
    void calsum(UsableNumber& un) {
        rep(i, 0, sum.size()) { sum[i] = 0; }
        rep(i, 0, un.numbers.size()) {
            if (ans[i] != -1) {
                sum[ans[i]] += un.numbers[i];
            }
        }
    }
};

int calc_score(const Input& in, const Mountain& mt) {
    ll score = 0;
    rep(i, 0, in.M) { score += abs(mt.sum[i] - mt.target[i]); }
    return round((20 - log10(1 + score)) * (5e7));
}

void greedy(Mountain& mt, UsableNumber& un, const Input& in) {
    // 使用済みフラグの初期化
    un.used.assign(un.numbers.size(), false);

    // Phase 1: 二分探索を使って各山について、ターゲット以下で最も大きい数を選ぶ
    rep(j, 0, in.M) {
        // 二分探索でmt.target[j]以下の最大の要素の位置を探す
        int left = 0;
        int right = in.N - 1;
        int candidate = -1;

        while (left <= right) {
            int mid = left + (right - left) / 2;
            if (un.numbers[mid] <= mt.target[j]) {
                candidate = mid;
                left = mid + 1;  // より大きい値を探す
            } else {
                right = mid - 1;  // より小さい値を探す
            }
        }

        // 候補が見つかったら、使用済みでない最大の要素を探す
        int best_idx = -1;
        if (candidate != -1) {
            for (int i = candidate; i >= 0; i--) {
                if (!un.used[i] && un.numbers[i] <= mt.target[j]) {
                    best_idx = i;
                    break;
                }
            }
        }

        // 適切な数が見つかれば割り当てる
        if (best_idx != -1) {
            mt.ans[best_idx] = j;
            un.used[best_idx] = true;
            mt.sum[j] += un.numbers[best_idx];
        }
    }

    // Phase 2: 残りの数字を使って差分を埋める（使わない選択肢あり）
    rep(i, in.N / 20 * 11, in.N / 5 * 3) {
        if (!un.used[i]) {
            // 最も差を小さくする山を選ぶ（または使わない）
            ll best_j = -1;
            ll min_total_diff = 0;

            // 現在の差分の合計（使わない場合のベースライン）
            rep(j, 0, in.M) { min_total_diff += abs(mt.sum[j] - mt.target[j]); }

            // 各山に割り当てた場合の総差分を計算
            rep(j, 0, in.M) {
                ll total_diff = 0;

                // j番目の山に数字iを割り当てたと仮定して計算
                rep(k, 0, in.M) {
                    if (k == j) {
                        total_diff += abs(mt.sum[k] + un.numbers[i] - mt.target[k]);
                    } else {
                        total_diff += abs(mt.sum[k] - mt.target[k]);
                    }
                }

                // より良い結果なら更新
                if (total_diff < min_total_diff) {
                    min_total_diff = total_diff;
                    best_j = j;
                }
            }

            // 最適な選択（使うか使わないか）を適用
            if (best_j != -1) {
                // 使う場合は対応する山に割り当て
                mt.ans[i] = best_j;
                mt.sum[best_j] += un.numbers[i];
            } else {
                // 使わない場合は-1のまま
                mt.ans[i] = -1;
            }

            un.used[i] = true;  // 処理済みとしてマーク
        }
    }
    rep(i, 0, in.N) {
        if (!un.used[i]) {
            // 最も差を小さくする山を選ぶ（または使わない）
            ll best_j = -1;
            ll min_total_diff = 0;

            // 現在の差分の合計（使わない場合のベースライン）
            rep(j, 0, in.M) { min_total_diff += abs(mt.sum[j] - mt.target[j]); }

            // 各山に割り当てた場合の総差分を計算
            rep(j, 0, in.M) {
                ll total_diff = 0;

                // j番目の山に数字iを割り当てたと仮定して計算
                rep(k, 0, in.M) {
                    if (k == j) {
                        total_diff += abs(mt.sum[k] + un.numbers[i] - mt.target[k]);
                    } else {
                        total_diff += abs(mt.sum[k] - mt.target[k]);
                    }
                }

                // より良い結果なら更新
                if (total_diff < min_total_diff) {
                    min_total_diff = total_diff;
                    best_j = j;
                }
            }

            // 最適な選択（使うか使わないか）を適用
            if (best_j != -1) {
                // 使う場合は対応する山に割り当て
                mt.ans[i] = best_j;
                mt.sum[best_j] += un.numbers[i];
            } else {
                // 使わない場合は-1のまま
                mt.ans[i] = -1;
            }

            un.used[i] = true;  // 処理済みとしてマーク
        }
    }
}

struct BeamState {
    Mountain mt;
    ll score;

    BeamState(const Mountain& mt_, ll score_) : mt(mt_), score(score_) {}

    bool operator<(const BeamState& other) const {
        return score < other.score;  // スコアが高い方が良い
    }
};

void beam_search(Mountain& mt, UsableNumber& un, const Input& in) {
    // 時間制限（秒）
    const double TIME_LIMIT = 1.8;

    // 開始時間を記録
    chrono::system_clock::time_point start = chrono::system_clock::now();

    // ビームサーチのパラメータ
    const int BEAM_WIDTH = 100;  // ビーム幅
    const int MAX_DEPTH = 20;    // 最大探索深度

    // 乱数生成器
    mt19937 gen(42);  // 固定シード
    uniform_int_distribution<> dis_num(0, in.N - 1);
    uniform_int_distribution<> dis_mountain(-1, in.M - 1);

    // 初期解として貪欲法の結果を使用
    greedy(mt, un, in);
    mt.calsum(un);

    // 初期状態をビームに追加
    priority_queue<BeamState> beam;
    beam.push(BeamState(mt, calc_score(in, mt)));

    Mountain best_mt = mt;
    ll best_score = calc_score(in, mt);

    int iteration_count = 0;

    for (int depth = 0; depth < MAX_DEPTH; depth++) {
        // 経過時間をチェック
        chrono::system_clock::time_point current = chrono::system_clock::now();
        double elapsed = chrono::duration_cast<chrono::milliseconds>(current - start).count() / 1000.0;
        if (elapsed >= TIME_LIMIT) break;

        priority_queue<BeamState> next_beam;
        vector<BeamState> current_states;

        // 現在のビームから状態を取り出す
        int beam_size = min(BEAM_WIDTH, (int)beam.size());
        for (int i = 0; i < beam_size && !beam.empty(); i++) {
            current_states.push_back(beam.top());
            beam.pop();
        }

        // 各状態について近傍操作を試す
        for (const auto& state : current_states) {
            iteration_count++;

            // 時間チェック
            current = chrono::system_clock::now();
            elapsed = chrono::duration_cast<chrono::milliseconds>(current - start).count() / 1000.0;
            if (elapsed >= TIME_LIMIT) break;

            // 現在の状態をベースに近傍操作を生成
            vector<Mountain> neighbors;

            // 操作1: 数字の割り当てを変更
            for (int trial = 0; trial < 20; trial++) {  // 試行回数を制限
                Mountain neighbor = state.mt;
                int idx = dis_num(gen);
                int old_mountain = neighbor.ans[idx];
                int new_mountain = dis_mountain(gen);

                if (old_mountain == new_mountain) continue;

                if (old_mountain != -1) {
                    neighbor.sum[old_mountain] -= un.numbers[idx];
                }
                if (new_mountain != -1) {
                    neighbor.sum[new_mountain] += un.numbers[idx];
                }
                neighbor.ans[idx] = new_mountain;

                neighbors.push_back(neighbor);
            }

            // 操作2: 2つの数字の割り当てを交換
            for (int trial = 0; trial < 10; trial++) {
                Mountain neighbor = state.mt;
                int idx1 = dis_num(gen);
                int idx2 = dis_num(gen);
                if (idx1 == idx2) continue;

                int mountain1 = neighbor.ans[idx1];
                int mountain2 = neighbor.ans[idx2];

                // 元の山から取り除く
                if (mountain1 != -1) {
                    neighbor.sum[mountain1] -= un.numbers[idx1];
                }
                if (mountain2 != -1) {
                    neighbor.sum[mountain2] -= un.numbers[idx2];
                }

                // 山を交換
                neighbor.ans[idx1] = mountain2;
                neighbor.ans[idx2] = mountain1;

                // 新しい山に追加
                if (mountain2 != -1) {
                    neighbor.sum[mountain2] += un.numbers[idx1];
                }
                if (mountain1 != -1) {
                    neighbor.sum[mountain1] += un.numbers[idx2];
                }

                neighbors.push_back(neighbor);
            }

            // 操作3: 差が大きい山から小さい山へ移動
            for (int trial = 0; trial < 5; trial++) {
                Mountain neighbor = state.mt;
                vector<pair<ll, int>> diff_mountains;
                rep(j, 0, in.M) { diff_mountains.push_back({neighbor.sum[j] - neighbor.target[j], j}); }
                sort(diff_mountains.begin(), diff_mountains.end());

                if (diff_mountains.back().first <= 0 || diff_mountains.front().first >= 0) {
                    continue;
                }

                int excess_mountain = diff_mountains.back().second;
                int deficit_mountain = diff_mountains.front().second;

                vector<int> candidates;
                rep(i, 0, in.N) {
                    if (neighbor.ans[i] == excess_mountain) {
                        candidates.push_back(i);
                    }
                }

                if (candidates.empty()) continue;

                uniform_int_distribution<> dis_candidate(0, candidates.size() - 1);
                int idx = candidates[dis_candidate(gen)];

                neighbor.sum[excess_mountain] -= un.numbers[idx];
                neighbor.ans[idx] = deficit_mountain;
                neighbor.sum[deficit_mountain] += un.numbers[idx];

                neighbors.push_back(neighbor);
            }

            // 近傍解を評価して次のビームに追加
            for (auto& neighbor : neighbors) {
                ll score = calc_score(in, neighbor);

                if (score > best_score) {
                    best_score = score;
                    best_mt = neighbor;
                }

                next_beam.push(BeamState(neighbor, score));

                // ビーム幅を超えた場合は下位を削除
                if (next_beam.size() > BEAM_WIDTH * 2) {
                    priority_queue<BeamState> temp;
                    for (int i = 0; i < BEAM_WIDTH && !next_beam.empty(); i++) {
                        temp.push(next_beam.top());
                        next_beam.pop();
                    }
                    next_beam = temp;
                }
            }
        }

        beam = next_beam;

        if (beam.empty()) break;
    }

    // 最良の解を返す
    mt = best_mt;
    // デバッグ用：ビームサーチの反復回数を表示
    cerr << "beam_search iterations: " << iteration_count << endl;
}

// 焼きなましのやり方の変更
int main() {
    Input input;
    cin >> input.N >> input.M >> input.L >> input.U;
    UsableNumber usableNumber(input);
    usableNumber.output();
    Mountain mountain(input);
    rep(i, 0, input.M) { cin >> mountain.target[i]; }
    beam_search(mountain, usableNumber, input);
    mountain.output();
    return 0;
}