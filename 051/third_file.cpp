#include <bits/stdc++.h>
using namespace std;

// 線分交差判定のための関数群
int sign(long long x) { return x > 0 ? 1 : (x < 0 ? -1 : 0); }
int orientation(pair<int, int> a, pair<int, int> b, pair<int, int> c) {
    long long cross =
        (long long)(b.first - a.first) * (c.second - a.second) - (long long)(b.second - a.second) * (c.first - a.first);
    return sign(cross);
}
bool segments_intersect(pair<int, int> p1, pair<int, int> p2, pair<int, int> q1, pair<int, int> q2) {
    if (max(p1.first, p2.first) < min(q1.first, q2.first) || max(q1.first, q2.first) < min(p1.first, p2.first) ||
        max(p1.second, p2.second) < min(q1.second, q2.second) ||
        max(q1.second, q2.second) < min(p1.second, p2.second)) {
        return false;
    }
    int o1 = orientation(p1, p2, q1);
    int o2 = orientation(p1, p2, q2);
    int o3 = orientation(q1, q2, p1);
    int o4 = orientation(q1, q2, p2);
    return (o1 * o2 <= 0) && (o3 * o4 <= 0);
}

int main() {
    vector<string> seedNums = {"0000", "0001", "0002"};  // テストケース番号
    for (const string &seedNum : seedNums) {
        ifstream input_file("in/" + seedNum + ".txt");
        ofstream output_file("output/" + seedNum + ".txt");
        if (!input_file.is_open()) {
            cerr << "Error: Cannot open input file in/" << seedNum << ".txt" << endl;
            continue;
        }

        int N, M, K;
        input_file >> N >> M >> K;
        vector<pair<int, int>> processor_pos(N);
        for (int i = 0; i < N; i++) input_file >> processor_pos[i].first >> processor_pos[i].second;
        vector<pair<int, int>> sorter_pos(M);
        for (int i = 0; i < M; i++) input_file >> sorter_pos[i].first >> sorter_pos[i].second;
        vector<vector<double>> prob(K, vector<double>(N));
        for (int i = 0; i < K; i++)
            for (int j = 0; j < N; j++) input_file >> prob[i][j];

        // 処理装置の配置: i番目の場所にi番目の処理装置
        for (int i = 0; i < N; i++) {
            output_file << i;
            if (i < N - 1) output_file << " ";
        }
        output_file << "\n";

        // 搬入口から最も近い分別器場所を見つける
        pair<int, int> inlet = {0, 5000};
        int nearest_sorter = 0;
        long long min_dist = LLONG_MAX;
        for (int i = 0; i < M; i++) {
            long long dx = sorter_pos[i].first - inlet.first;
            long long dy = sorter_pos[i].second - inlet.second;
            long long dist = dx * dx + dy * dy;
            if (dist < min_dist) {
                min_dist = dist;
                nearest_sorter = i;
            }
        }
        output_file << N + nearest_sorter << "\n";

        // ベルトコンベアの管理
        vector<pair<pair<int, int>, pair<int, int>>> existing_belts;
        existing_belts.push_back({inlet, sorter_pos[nearest_sorter]});

        // ベルトコンベアの交差チェック関数
        auto check_intersection = [&](pair<int, int> start, pair<int, int> end) -> bool {
            for (auto &belt : existing_belts) {
                if (segments_intersect(start, end, belt.first, belt.second)) {
                    // 端点を共有する場合は交差ではない
                    if (start == belt.first || start == belt.second || end == belt.first || end == belt.second) {
                        continue;
                    }
                    return true;  // 交差している
                }
            }
            return false;  // 交差していない
        };

        // 各ごみ種に対して最適な分別器と経路を見つける
        vector<int> best_sorter_for_type(N);
        vector<double> best_prob_for_type(N, 0.0);

        // 各ごみ種について最も高い確率を持つ分別器を探す
        for (int waste_type = 0; waste_type < N; waste_type++) {
            for (int sorter_type = 0; sorter_type < K; sorter_type++) {
                if (prob[sorter_type][waste_type] > best_prob_for_type[waste_type]) {
                    best_prob_for_type[waste_type] = prob[sorter_type][waste_type];
                    best_sorter_for_type[waste_type] = sorter_type;
                }
            }
        }

        // 使用する分別器設置場所を決定（より多くの場所を使用）
        vector<bool> used_sorter_pos(M, false);
        vector<int> sorter_assignment(M, -1);
        vector<int> exit1_dest(M, -1);
        vector<int> exit2_dest(M, -1);

        // 最初の分別器（搬入口に接続）
        used_sorter_pos[nearest_sorter] = true;

        // 最初の分別器で最も分別しやすいごみ種のペアを見つける
        double max_separation = 0.0;
        int best_type1 = 0, best_type2 = 1;
        int best_first_sorter = 0;

        for (int s = 0; s < K; s++) {
            for (int t1 = 0; t1 < N; t1++) {
                for (int t2 = 0; t2 < N; t2++) {
                    if (t1 != t2) {
                        // t1は出口1、t2は出口2に送る場合の分別効果
                        double separation = prob[s][t1] - prob[s][t2];
                        if (separation > max_separation) {
                            max_separation = separation;
                            best_type1 = t1;
                            best_type2 = t2;
                            best_first_sorter = s;
                        }
                    }
                }
            }
        }

        sorter_assignment[nearest_sorter] = best_first_sorter;

        // 1段目分別器の出口1を処理装置に接続（交差チェック）
        bool exit1_connected = false;
        if (!check_intersection(sorter_pos[nearest_sorter], processor_pos[best_type1])) {
            exit1_dest[nearest_sorter] = best_type1;
            existing_belts.push_back({sorter_pos[nearest_sorter], processor_pos[best_type1]});
            exit1_connected = true;
        }

        // 2段目の分別器を設置
        int second_sorter_pos = -1;
        bool second_sorter_connected = false;

        for (int i = 0; i < M; i++) {
            if (used_sorter_pos[i]) continue;

            // 1段目分別器から2段目分別器への接続が交差しないかチェック
            if (!check_intersection(sorter_pos[nearest_sorter], sorter_pos[i])) {
                second_sorter_pos = i;
                used_sorter_pos[i] = true;
                exit2_dest[nearest_sorter] = N + i;
                existing_belts.push_back({sorter_pos[nearest_sorter], sorter_pos[i]});
                second_sorter_connected = true;
                break;
            }
        }

        if (second_sorter_connected && second_sorter_pos != -1) {
            // 2段目分別器で残りのごみ種を分別
            double max_separation_second = 0.0;
            int best_type1_second = best_type2, best_type2_second = (best_type2 + 1) % N;
            int best_second_sorter = 0;

            for (int s = 0; s < K; s++) {
                for (int t1 = 0; t1 < N; t1++) {
                    if (t1 == best_type1) continue;  // 既に1段目で分別済み
                    for (int t2 = 0; t2 < N; t2++) {
                        if (t2 == best_type1 || t1 == t2) continue;

                        double separation = prob[s][t1] - prob[s][t2];
                        if (separation > max_separation_second) {
                            max_separation_second = separation;
                            best_type1_second = t1;
                            best_type2_second = t2;
                            best_second_sorter = s;
                        }
                    }
                }
            }

            sorter_assignment[second_sorter_pos] = best_second_sorter;

            // 2段目分別器の出口を処理装置に接続（交差チェック）
            bool exit1_second_connected = false;
            bool exit2_second_connected = false;

            if (!check_intersection(sorter_pos[second_sorter_pos], processor_pos[best_type1_second])) {
                exit1_dest[second_sorter_pos] = best_type1_second;
                existing_belts.push_back({sorter_pos[second_sorter_pos], processor_pos[best_type1_second]});
                exit1_second_connected = true;
            }

            if (!check_intersection(sorter_pos[second_sorter_pos], processor_pos[best_type2_second])) {
                exit2_dest[second_sorter_pos] = best_type2_second;
                existing_belts.push_back({sorter_pos[second_sorter_pos], processor_pos[best_type2_second]});
                exit2_second_connected = true;
            }

            // 2段目分別器の未接続出口を処理
            if (!exit1_second_connected) {
                for (int t = 0; t < N; t++) {
                    if (t != best_type1 && !check_intersection(sorter_pos[second_sorter_pos], processor_pos[t])) {
                        exit1_dest[second_sorter_pos] = t;
                        existing_belts.push_back({sorter_pos[second_sorter_pos], processor_pos[t]});
                        break;
                    }
                }
            }

            if (!exit2_second_connected) {
                for (int t = 0; t < N; t++) {
                    if (t != best_type1 && t != exit1_dest[second_sorter_pos] &&
                        !check_intersection(sorter_pos[second_sorter_pos], processor_pos[t])) {
                        exit2_dest[second_sorter_pos] = t;
                        existing_belts.push_back({sorter_pos[second_sorter_pos], processor_pos[t]});
                        break;
                    }
                }
            }
        }

        // 3段目以降の分別器も積極的に使用
        vector<int> remaining_processors;
        vector<bool> processor_used(N, false);

        // 既に使用されている処理装置をマーク
        for (int i = 0; i < M; i++) {
            if (used_sorter_pos[i]) {
                if (exit1_dest[i] >= 0 && exit1_dest[i] < N) {
                    processor_used[exit1_dest[i]] = true;
                }
                if (exit2_dest[i] >= 0 && exit2_dest[i] < N) {
                    processor_used[exit2_dest[i]] = true;
                }
            }
        }

        // 未使用の処理装置を収集
        for (int i = 0; i < N; i++) {
            if (!processor_used[i]) {
                remaining_processors.push_back(i);
            }
        }

        // 追加の分別器を設置（3段目以降）
        for (int i = 0; i < M && remaining_processors.size() >= 2; i++) {
            if (used_sorter_pos[i]) continue;

            // この分別器設置場所に対して最適な分別器種類と処理装置ペアを探す
            double best_separation = 0.0;
            int best_sorter_type = 0;
            int best_proc1 = -1, best_proc2 = -1;
            bool connection_possible = false;

            // 各分別器種類について評価
            for (int s = 0; s < K; s++) {
                // 未使用の処理装置ペアで最も分別効果が高いものを探す
                for (int idx1 = 0; idx1 < (int)remaining_processors.size(); idx1++) {
                    for (int idx2 = idx1 + 1; idx2 < (int)remaining_processors.size(); idx2++) {
                        int proc1 = remaining_processors[idx1];
                        int proc2 = remaining_processors[idx2];

                        // 交差チェック
                        if (!check_intersection(sorter_pos[i], processor_pos[proc1]) &&
                            !check_intersection(sorter_pos[i], processor_pos[proc2])) {
                            double separation = abs(prob[s][proc1] - prob[s][proc2]);
                            if (separation > best_separation) {
                                best_separation = separation;
                                best_sorter_type = s;
                                best_proc1 = proc1;
                                best_proc2 = proc2;
                                connection_possible = true;
                            }
                        }
                    }
                }
            }

            // 良い組み合わせが見つかった場合、分別器を設置
            if (connection_possible && best_proc1 != -1 && best_proc2 != -1) {
                used_sorter_pos[i] = true;
                sorter_assignment[i] = best_sorter_type;
                exit1_dest[i] = best_proc1;
                exit2_dest[i] = best_proc2;

                existing_belts.push_back({sorter_pos[i], processor_pos[best_proc1]});
                existing_belts.push_back({sorter_pos[i], processor_pos[best_proc2]});

                // 使用した処理装置を残りリストから削除
                remaining_processors.erase(remove(remaining_processors.begin(), remaining_processors.end(), best_proc1),
                                           remaining_processors.end());
                remaining_processors.erase(remove(remaining_processors.begin(), remaining_processors.end(), best_proc2),
                                           remaining_processors.end());
            }
        }

        // 1段目分別器の出口2が未接続の場合、直接処理装置に接続
        if (exit2_dest[nearest_sorter] == -1) {
            for (int t = 0; t < N; t++) {
                if (t != best_type1 && !check_intersection(sorter_pos[nearest_sorter], processor_pos[t])) {
                    exit2_dest[nearest_sorter] = t;
                    existing_belts.push_back({sorter_pos[nearest_sorter], processor_pos[t]});
                    break;
                }
            }
        }

        // 出口1が未接続の場合の処理
        if (!exit1_connected) {
            // 交差しない他の処理装置を探す
            for (int t = 0; t < N; t++) {
                if (!check_intersection(sorter_pos[nearest_sorter], processor_pos[t])) {
                    exit1_dest[nearest_sorter] = t;
                    existing_belts.push_back({sorter_pos[nearest_sorter], processor_pos[t]});
                    break;
                }
            }
        }

        // 全ての分別器の出口が有効な範囲内であることを確認
        for (int i = 0; i < M; i++) {
            if (used_sorter_pos[i]) {
                // 出口1が未設定または無効な場合
                if (exit1_dest[i] == -1 || exit1_dest[i] >= N) {
                    for (int t = 0; t < N; t++) {
                        if (!check_intersection(sorter_pos[i], processor_pos[t])) {
                            exit1_dest[i] = t;
                            existing_belts.push_back({sorter_pos[i], processor_pos[t]});
                            break;
                        }
                    }
                    // 交差チェックでも見つからない場合は強制的に最初の処理装置に接続
                    if (exit1_dest[i] == -1 || exit1_dest[i] >= N) {
                        exit1_dest[i] = 0;
                    }
                }
                // 出口2が未設定または無効な場合
                if (exit2_dest[i] == -1 || exit2_dest[i] >= N) {
                    for (int t = 0; t < N; t++) {
                        if (t != exit1_dest[i] && !check_intersection(sorter_pos[i], processor_pos[t])) {
                            exit2_dest[i] = t;
                            existing_belts.push_back({sorter_pos[i], processor_pos[t]});
                            break;
                        }
                    }
                    // 交差チェックでも見つからない場合は強制的に異なる処理装置に接続
                    if (exit2_dest[i] == -1 || exit2_dest[i] >= N) {
                        // 出口1と異なる処理装置を選ぶ
                        for (int t = 0; t < N; t++) {
                            if (t != exit1_dest[i]) {
                                exit2_dest[i] = t;
                                break;
                            }
                        }
                    }
                }
            }
        }

        // 分別器の設置情報を出力
        for (int i = 0; i < M; i++) {
            if (used_sorter_pos[i]) {
                output_file << sorter_assignment[i] << " " << exit1_dest[i] << " " << exit2_dest[i] << "\n";
            } else {
                output_file << "-1\n";
            }
        }

        input_file.close();
        output_file.close();
    }
    return 0;
}
