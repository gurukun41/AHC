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

struct Input {
    ll N;
    ll M;
    ll L;
    ll U;
    Input(ll N_=500, ll M_=50, ll L_=1e15 - 2e12, ll U_=1e15 + 2e12) : N(N_), M(M_), L(L_), U(U_) {};
};

struct UsableNumber {
    vl numbers;
    vector<bool> used;
    UsableNumber(Input& in) {
        numbers.resize(in.N);
        ll sep = (in.U - in.L) / ((in.N/5) * 2);
        ll sepsep = sep / ((in.N/5) * 3);
        rep(i,0,in.N/5) {
            if(i*3 > in.N/20*11){
                numbers[i*3] = sep;
                numbers[i*3+1] = sep;
                numbers[i*3+2] = sep;
            }
            numbers[i*3] = sepsep * (i+1);
            numbers[i*3+1] = sepsep * (i+1);
            numbers[i*3+2] = sepsep * (i+1);
        }
        rep(i,in.N/5*3,in.N) {
            numbers[i] = in.L + sep * (i - in.N/5*3 + 1);
        }  
    }
    void output() {
        rep(i,0,numbers.size()) {
            cout << numbers[i];
            if (i != numbers.size()-1) {
                cout << " ";
            } 
            else {
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
        rep(i,0,ans.size()) {
            cout << ans[i]+1;
            if (i != ans.size()-1) {
                cout << " ";
            } 
            else {
                cout << "\n";
            }
        }
    }
    void calsum(UsableNumber & un) {
        rep(i,0,sum.size()) {
            sum[i] = 0;
        }
        rep(i,0,un.numbers.size()) {
            if(ans[i] != -1) {
                sum[ans[i]] += un.numbers[i];
            }
        }
    }
};

int calc_score(const Input& in, const Mountain& mt) {
    ll score = 0;
    rep(i,0,in.M) {
        score += abs(mt.sum[i] - mt.target[i]);
    }
    return round((20 - log10(1+score))*(5e7));
}

void greedy(Mountain & mt, UsableNumber & un, const Input & in) {
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
    rep(i, in.N/20*11, in.N/5*3) {
        if (!un.used[i]) {
            // 最も差を小さくする山を選ぶ（または使わない）
            ll best_j = -1;
            ll min_total_diff = 0;
            
            // 現在の差分の合計（使わない場合のベースライン）
            rep(j, 0, in.M) {
                min_total_diff += abs(mt.sum[j] - mt.target[j]);
            }
            
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
            rep(j, 0, in.M) {
                min_total_diff += abs(mt.sum[j] - mt.target[j]);
            }
            
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

void simulated_annealing(Mountain & mt, UsableNumber & un, const Input & in) {
    // 初期解として貪欲法の結果を使用
    greedy(mt, un, in);
    
    // 焼きなまし用のパラメータ
    double T_start = 10000.0;  // 初期温度
    double T_end = 0.1;        // 終了温度（より低く）
    double T;
    
    // 時間制限（秒）
    const double TIME_LIMIT = 1.8;
    
    // 開始時間を記録
    chrono::system_clock::time_point start = chrono::system_clock::now();
    
    // 現在の解とスコアを保存
    Mountain best_mt = mt;
    ll best_score = calc_score(in, mt);
    
    // 乱数生成器 - 固定シードで再現性を確保
    const unsigned int seed = 42;  // 固定シードを維持
    mt19937 gen(seed);
    uniform_int_distribution<> dis_num(0, in.N - 1);
    uniform_int_distribution<> dis_mountain(-1, in.M - 1);
    uniform_real_distribution<> dis_prob(0.0, 1.0);
    
    // 近傍操作の種類
    enum OperationType { CHANGE_ONE, SWAP_TWO, MOVE_MULTIPLE };
    uniform_int_distribution<> dis_op(0, 2);  // 3種類の操作
    
    // 成功した操作と失敗した操作のカウンタ
    int success = 0, fail = 0;
    
    // 反復回数のカウント（再現性のため）
    int iteration_count = 0;
    
    while (true) {
        iteration_count++;
        
        // 経過時間をチェック
        chrono::system_clock::time_point current = chrono::system_clock::now();
        double elapsed = chrono::duration_cast<chrono::milliseconds>(current - start).count() / 1000.0;
        if (elapsed >= TIME_LIMIT) break;
        
        // 非線形温度スケジュール
        double progress = elapsed / TIME_LIMIT;
        T = T_start * pow(T_end / T_start, progress);
        
        // 現在の状態を保存
        Mountain current_mt = mt;
        ll current_score = calc_score(in, mt);
        
        // 近傍操作を選択
        OperationType op_type = static_cast<OperationType>(dis_op(gen));
        
        // 近傍操作の実行
        switch (op_type) {
            case CHANGE_ONE: {
                // 1つの数字の割り当てを変更（元の操作）
                int idx = dis_num(gen);
                int old_mountain = mt.ans[idx];
                int new_mountain = dis_mountain(gen);
                
                if (old_mountain == new_mountain) continue;
                
                if (old_mountain != -1) {
                    mt.sum[old_mountain] -= un.numbers[idx];
                }
                if (new_mountain != -1) {
                    mt.sum[new_mountain] += un.numbers[idx];
                }
                mt.ans[idx] = new_mountain;
                break;
            }
            case SWAP_TWO: {
                // 2つの数字の割り当てを交換
                int idx1 = dis_num(gen);
                int idx2 = dis_num(gen);
                if (idx1 == idx2) continue;
                
                int mountain1 = mt.ans[idx1];
                int mountain2 = mt.ans[idx2];
                
                // 元の山から取り除く
                if (mountain1 != -1) {
                    mt.sum[mountain1] -= un.numbers[idx1];
                }
                if (mountain2 != -1) {
                    mt.sum[mountain2] -= un.numbers[idx2];
                }
                
                // 山を交換
                mt.ans[idx1] = mountain2;
                mt.ans[idx2] = mountain1;
                
                // 新しい山に追加
                if (mountain2 != -1) {
                    mt.sum[mountain2] += un.numbers[idx1];
                }
                if (mountain1 != -1) {
                    mt.sum[mountain1] += un.numbers[idx2];
                }
                break;
            }
            case MOVE_MULTIPLE: {
                // 差が最も大きい山から差が最も小さい山へ数字を移動
                vector<pair<ll, int>> diff_mountains;
                rep(j, 0, in.M) {
                    diff_mountains.push_back({mt.sum[j] - mt.target[j], j});
                }
                sort(diff_mountains.begin(), diff_mountains.end());
                
                // 最も余剰がある山と最も不足している山
                int excess_mountain = diff_mountains.back().second;
                int deficit_mountain = diff_mountains.front().second;
                
                if (diff_mountains.back().first <= 0 || diff_mountains.front().first >= 0) {
                    // 全ての山が過剰か不足の場合はスキップ
                    continue;
                }
                
                // 過剰な山から数字を探す
                vector<int> candidates;
                rep(i, 0, in.N) {
                    if (mt.ans[i] == excess_mountain) {
                        candidates.push_back(i);
                    }
                }
                
                if (candidates.empty()) continue;
                
                // ランダムに1つの数字を選択して移動（再現性のため複雑な選択は避ける）
                uniform_int_distribution<> dis_candidate(0, candidates.size() - 1);
                int idx = candidates[dis_candidate(gen)];
                
                // 数字を移動
                mt.sum[excess_mountain] -= un.numbers[idx];
                mt.ans[idx] = deficit_mountain;
                mt.sum[deficit_mountain] += un.numbers[idx];
                break;
            }
        }
        
        // スコアを再計算
        ll new_score = calc_score(in, mt);
        ll delta = new_score - current_score;
        
        // スコアが改善または確率的に受け入れる
        if (delta > 0 || dis_prob(gen) < exp(delta / T)) {
            // 解を受け入れる
            success++;
            if (new_score > best_score) {
                best_score = new_score;
                best_mt = mt;
            }
        } else {
            // 解を拒否して元に戻す
            mt = current_mt;
            fail++;
        }
        
        // 適応的なパラメータ調整（一定回数ごと）
        if (iteration_count % 1000 == 0 && fail > 0) {
            double acceptance_rate = (double)success / (success + fail);
            // 受理率に基づいて温度を調整（再現性のため固定的なルール）
            if (acceptance_rate < 0.1) {
                T *= 1.2;
            } else if (acceptance_rate > 0.6) {
                T *= 0.8;
            }
            success = fail = 0;
        }
    }
    
    // 最良の解を返す
    mt = best_mt;
    // デバッグ用：焼きなましのループ回数を表示
    cerr << "simulated_annealing iterations: " << iteration_count << endl;
}
// 焼きなましのやり方の変更
int main(){
    int seed = 35;
    std::ostringstream oss;
    oss << std::setw(4) << std::setfill('0') << seed;
    string input_filename = "in/" + oss.str() + ".txt";
    string output_filename = "out/l_" + oss.str() + ".txt";
    freopen(input_filename.c_str(), "r", stdin);
    freopen(output_filename.c_str(), "w", stdout);
    Input input;
    cin >> input.N >> input.M >> input.L >> input.U;
    UsableNumber usableNumber(input);
    usableNumber.output();
    Mountain mountain(input);
    rep(i,0,input.M) {
        cin >> mountain.target[i];
    }
    simulated_annealing(mountain, usableNumber, input);    
    mountain.output();
    return 0;
}