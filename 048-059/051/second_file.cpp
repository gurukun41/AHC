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
        for (int i = 0; i < N; i++) {
            input_file >> processor_pos[i].first >> processor_pos[i].second;
        }

        vector<pair<int, int>> sorter_pos(M);
        for (int i = 0; i < M; i++) {
            input_file >> sorter_pos[i].first >> sorter_pos[i].second;
        }

        vector<vector<double>> prob(K, vector<double>(N));
        for (int i = 0; i < K; i++) {
            for (int j = 0; j < N; j++) {
                input_file >> prob[i][j];
            }
        }

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
                    if (start == belt.first || start == belt.second || end == belt.first || end == belt.second) {
                        continue;
                    }
                    return true;
                }
            }
            return false;
        };

        vector<int> best_sorter_for_type(N);
        vector<double> best_prob_for_type(N, 0.0);
        for (int waste_type = 0; waste_type < N; waste_type++) {
            for (int sorter_type = 0; sorter_type < K; sorter_type++) {
                if (prob[sorter_type][waste_type] > best_prob_for_type[waste_type]) {
                    best_prob_for_type[waste_type] = prob[sorter_type][waste_type];
                    best_sorter_for_type[waste_type] = sorter_type;
                }
            }
        }

        vector<bool> used_sorter_pos(M, false);
        vector<int> sorter_assignment(M, -1);
        vector<int> exit1_dest(M, -1);
        vector<int> exit2_dest(M, -1);

        used_sorter_pos[nearest_sorter] = true;

        double max_separation = 0.0;
        int best_type1 = 0, best_type2 = 1;
        int best_first_sorter = 0;

        for (int s = 0; s < K; s++) {
            for (int t1 = 0; t1 < N; t1++) {
                for (int t2 = 0; t2 < N; t2++) {
                    if (t1 != t2) {
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

        int second_sorter_pos = -1;
        bool second_sorter_connected = false;
        for (int i = 0; i < M; i++) {
            if (used_sorter_pos[i]) continue;
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
            double max_separation_second = 0.0;
            int best_type1_second = best_type2, best_type2_second = (best_type2 + 1) % N;
            int best_second_sorter = 0;

            for (int s = 0; s < K; s++) {
                for (int t1 = 0; t1 < N; t1++) {
                    if (t1 == best_type1) continue;
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

            if (!check_intersection(sorter_pos[second_sorter_pos], processor_pos[best_type1_second])) {
                exit1_dest[second_sorter_pos] = best_type1_second;
                existing_belts.push_back({sorter_pos[second_sorter_pos], processor_pos[best_type1_second]});
            }

            if (!check_intersection(sorter_pos[second_sorter_pos], processor_pos[best_type2_second])) {
                exit2_dest[second_sorter_pos] = best_type2_second;
                existing_belts.push_back({sorter_pos[second_sorter_pos], processor_pos[best_type2_second]});
            }

            // 2段目分別器の未接続出口を処理
            if (exit1_dest[second_sorter_pos] == -1) {
                for (int t = 0; t < N; t++) {
                    if (t != exit1_dest[nearest_sorter] &&
                        !check_intersection(sorter_pos[second_sorter_pos], processor_pos[t])) {
                        exit1_dest[second_sorter_pos] = t;
                        existing_belts.push_back({sorter_pos[second_sorter_pos], processor_pos[t]});
                        break;
                    }
                }
            }

            if (exit2_dest[second_sorter_pos] == -1) {
                for (int t = 0; t < N; t++) {
                    if (t != exit1_dest[nearest_sorter] && t != exit1_dest[second_sorter_pos] &&
                        !check_intersection(sorter_pos[second_sorter_pos], processor_pos[t])) {
                        exit2_dest[second_sorter_pos] = t;
                        existing_belts.push_back({sorter_pos[second_sorter_pos], processor_pos[t]});
                        break;
                    }
                }
            }
        }

        if (exit2_dest[nearest_sorter] == -1 || exit2_dest[nearest_sorter] >= N + M) {
            for (int t = 0; t < N; t++) {
                if (t != exit1_dest[nearest_sorter] &&
                    !check_intersection(sorter_pos[nearest_sorter], processor_pos[t])) {
                    exit2_dest[nearest_sorter] = t;
                    existing_belts.push_back({sorter_pos[nearest_sorter], processor_pos[t]});
                    break;
                }
            }
        }

        if (!exit1_connected) {
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
                if (exit1_dest[i] == -1 || exit1_dest[i] >= N + M) {
                    for (int t = 0; t < N; t++) {
                        if (!check_intersection(sorter_pos[i], processor_pos[t])) {
                            exit1_dest[i] = t;
                            existing_belts.push_back({sorter_pos[i], processor_pos[t]});
                            break;
                        }
                    }
                    // 交差チェックでも見つからない場合は強制的に最初の処理装置に接続
                    if (exit1_dest[i] == -1 || exit1_dest[i] >= N + M) {
                        exit1_dest[i] = 0;
                    }
                }
                // 出口2が未設定または無効な場合
                if (exit2_dest[i] == -1 || exit2_dest[i] >= N + M) {
                    for (int t = 0; t < N; t++) {
                        if (t != exit1_dest[i] && !check_intersection(sorter_pos[i], processor_pos[t])) {
                            exit2_dest[i] = t;
                            existing_belts.push_back({sorter_pos[i], processor_pos[t]});
                            break;
                        }
                    }
                    // 交差チェックでも見つからない場合は強制的に異なる処理装置に接続
                    if (exit2_dest[i] == -1 || exit2_dest[i] >= N + M) {
                        exit2_dest[i] = (exit1_dest[i] + 1) % N;
                    }
                }
            }
        }
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
