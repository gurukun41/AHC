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
    int N, M, K;
    cin >> N >> M >> K;

    // 処理装置設置場所の座標
    vector<pair<int, int>> processor_pos(N);
    for (int i = 0; i < N; i++) {
        cin >> processor_pos[i].first >> processor_pos[i].second;
    }

    // 分別器設置場所の座標
    vector<pair<int, int>> sorter_pos(M);
    for (int i = 0; i < M; i++) {
        cin >> sorter_pos[i].first >> sorter_pos[i].second;
    }

    // 分別器の確率
    vector<vector<double>> prob(K, vector<double>(N));
    for (int i = 0; i < K; i++) {
        for (int j = 0; j < N; j++) {
            cin >> prob[i][j];
        }
    }

    // 処理装置の配置: i番目の場所にi番目の処理装置
    for (int i = 0; i < N; i++) {
        cout << i;
        if (i < N - 1) cout << " ";
    }
    cout << "\n";

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

    // 搬入口の接続先
    cout << N + nearest_sorter << "\n";

    // ベルトコンベアの管理
    vector<pair<pair<int, int>, pair<int, int>>> existing_belts;
    existing_belts.push_back({inlet, sorter_pos[nearest_sorter]});

    // ベルトコンベアの交差チェック関数
    auto check_intersection = [&](pair<int, int> start, pair<int, int> end) -> bool {
        for (auto& belt : existing_belts) {
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

        if (!exit1_second_connected || best_type2_second != best_type1_second) {
            if (!check_intersection(sorter_pos[second_sorter_pos], processor_pos[best_type2_second])) {
                exit2_dest[second_sorter_pos] = best_type2_second;
                existing_belts.push_back({sorter_pos[second_sorter_pos], processor_pos[best_type2_second]});
                exit2_second_connected = true;
            }
        }

        // 2段目分別器の未接続出口を処理
        if (!exit1_second_connected) {
            for (int t = 0; t < N; t++) {
                if (t != exit1_dest[nearest_sorter] &&
                    !check_intersection(sorter_pos[second_sorter_pos], processor_pos[t])) {
                    exit1_dest[second_sorter_pos] = t;
                    existing_belts.push_back({sorter_pos[second_sorter_pos], processor_pos[t]});
                    break;
                }
            }
        }

        if (!exit2_second_connected) {
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

    // 1段目分別器の出口2が未接続の場合、直接処理装置に接続
    if (exit2_dest[nearest_sorter] == -1 || exit2_dest[nearest_sorter] >= N + M) {
        for (int t = 0; t < N; t++) {
            if (t != exit1_dest[nearest_sorter] && !check_intersection(sorter_pos[nearest_sorter], processor_pos[t])) {
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
    }  // 分別器の設置情報を出力
    for (int i = 0; i < M; i++) {
        if (used_sorter_pos[i]) {
            cout << sorter_assignment[i] << " " << exit1_dest[i] << " " << exit2_dest[i] << "\n";
        } else {
            cout << "-1\n";
        }
    }

    return 0;
}
