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

    // 分別器の確率（今回は近接優先だが、型の選択にのみ使用）
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

    // 近接貪欲で接続していく
    vector<bool> used_sorter_pos(M, false);
    vector<int> sorter_assignment(M, -1);
    vector<int> exit1_dest(M, -1), exit2_dest(M, -1);
    vector<int> out_count(M, 0);
    vector<bool> processor_connected(N, false);

    used_sorter_pos[nearest_sorter] = true;  // 搬入口に繋いだ分別器を使用済みに

    // 必要出口管理: 現在利用可能な出口数と、未接続処理装置数
    int remaining_processors = N;
    int total_available_exits = 2;  // nearest_sorter の2出口

    auto dist2 = [&](const pair<int, int>& a, const pair<int, int>& b) {
        long long dx = (long long)a.first - b.first;
        long long dy = (long long)a.second - b.second;
        return dx * dx + dy * dy;
    };

    auto all_processors_connected = [&]() {
        for (int p = 0; p < N; ++p)
            if (!processor_connected[p]) return false;
        return true;
    };

    // メインの近接貪欲ループ
    while (true) {
        long long best_d = LLONG_MAX;
        int from = -1;
        bool to_processor = false;
        int target = -1;

        for (int i = 0; i < M; ++i) {
            if (!used_sorter_pos[i] || out_count[i] >= 2) continue;  // 使える出口がある分別器のみ

            // 近い未接続の処理装置
            for (int p = 0; p < N; ++p) {
                if (processor_connected[p]) continue;
                if (check_intersection(sorter_pos[i], processor_pos[p])) continue;
                long long d = dist2(sorter_pos[i], processor_pos[p]);
                if (d < best_d) {
                    best_d = d;
                    from = i;
                    to_processor = true;
                    target = p;
                }
            }
            // 近い未使用の分別器（出口が不足しているときのみ分岐を許可）
            if (total_available_exits < remaining_processors) {
                for (int j = 0; j < M; ++j) {
                    if (used_sorter_pos[j]) continue;
                    if (check_intersection(sorter_pos[i], sorter_pos[j])) continue;
                    long long d = dist2(sorter_pos[i], sorter_pos[j]);
                    if (d < best_d) {
                        best_d = d;
                        from = i;
                        to_processor = false;
                        target = j;
                    }
                }
            }
        }

        if (from == -1) break;  // これ以上繋げられない

        int whichExit = (exit1_dest[from] == -1) ? 1 : 2;
        if (to_processor) {
            if (whichExit == 1)
                exit1_dest[from] = target;
            else
                exit2_dest[from] = target;
            existing_belts.push_back({sorter_pos[from], processor_pos[target]});
            processor_connected[target] = true;
            out_count[from]++;
            // カウンタ更新: 出口消費、未接続処理装置減少
            total_available_exits--;
            remaining_processors--;
        } else {
            if (whichExit == 1)
                exit1_dest[from] = N + target;
            else
                exit2_dest[from] = N + target;
            existing_belts.push_back({sorter_pos[from], sorter_pos[target]});
            used_sorter_pos[target] = true;  // 新たに使える分別器が増える
            out_count[from]++;
            // カウンタ更新: 親出口1消費、新分別器の2出口追加 => 正味+1
            total_available_exits += 1;
        }

        if (remaining_processors == 0) break;
    }

    // 残った未接続処理装置を、空き出口のある既使用分別器に最近傍で接続
    for (int p = 0; p < N; ++p) {
        if (processor_connected[p]) continue;
        long long best_d = LLONG_MAX;
        int from = -1;
        for (int i = 0; i < M; ++i) {
            if (!used_sorter_pos[i] || out_count[i] >= 2) continue;
            if (check_intersection(sorter_pos[i], processor_pos[p])) continue;
            long long d = dist2(sorter_pos[i], processor_pos[p]);
            if (d < best_d) {
                best_d = d;
                from = i;
            }
        }
        if (from != -1) {
            int whichExit = (exit1_dest[from] == -1) ? 1 : 2;
            if (whichExit == 1)
                exit1_dest[from] = p;
            else
                exit2_dest[from] = p;
            existing_belts.push_back({sorter_pos[from], processor_pos[p]});
            processor_connected[p] = true;
            out_count[from]++;
            // カウンタ更新
            total_available_exits--;
            remaining_processors--;
        }
    }

    // まだ空き出口がある分別器は、交差しない範囲で一番近い処理装置に接続して埋める
    for (int i = 0; i < M; ++i) {
        if (!used_sorter_pos[i]) continue;
        while (out_count[i] < 2) {
            long long best_d = LLONG_MAX;
            int best_p = -1;
            for (int p = 0; p < N; ++p) {
                if (check_intersection(sorter_pos[i], processor_pos[p])) continue;
                long long d = dist2(sorter_pos[i], processor_pos[p]);
                if (d < best_d) {
                    best_d = d;
                    best_p = p;
                }
            }
            if (best_p == -1) break;  // 交差しない接続が見つからない
            int whichExit = (exit1_dest[i] == -1) ? 1 : 2;
            if (whichExit == 1)
                exit1_dest[i] = best_p;
            else
                exit2_dest[i] = best_p;
            existing_belts.push_back({sorter_pos[i], processor_pos[best_p]});
            out_count[i]++;
            // カウンタ更新
            total_available_exits--;
        }
    }

    // 分別器の型は、接続先の処理装置に対して確率合計が最大の型を選ぶ（簡易）
    for (int i = 0; i < M; ++i) {
        if (!used_sorter_pos[i]) continue;
        vector<int> procs;
        if (0 <= exit1_dest[i] && exit1_dest[i] < N) procs.push_back(exit1_dest[i]);
        if (0 <= exit2_dest[i] && exit2_dest[i] < N) procs.push_back(exit2_dest[i]);
        int best_s = 0;
        double best_v = -1.0;
        for (int s = 0; s < K; ++s) {
            double v = 0.0;
            for (int p : procs) v += prob[s][p];
            if (v > best_v) {
                best_v = v;
                best_s = s;
            }
        }
        sorter_assignment[i] = best_s;
    }

    // 分別器の設置情報を出力
    for (int i = 0; i < M; i++) {
        if (used_sorter_pos[i]) {
            // 空の出口が残っていれば、とりあえず0で埋める（出力形式を満たすため）。
            int e1 = exit1_dest[i];
            int e2 = exit2_dest[i];
            if (e1 == -1) e1 = 0;
            if (e2 == -1) e2 = 0;
            cout << sorter_assignment[i] << " " << e1 << " " << e2 << "\n";
        } else {
            cout << "-1\n";
        }
    }

    return 0;
}
