#include <bits/stdc++.h>
using namespace std;

int main() {
    vector<string> seedNums = {"0000", "0001", "0002"};  // テストケース番号

    for (const string &seedNum : seedNums) {
        // 入力ファイルと出力ファイルを開く
        ifstream input_file("in/" + seedNum + ".txt");
        ofstream output_file("output/" + seedNum + ".txt");

        if (!input_file.is_open()) {
            cout << "Error: Cannot open input file input/" << seedNum << ".txt" << endl;
            continue;
        }

        int N, M, K;
        input_file >> N >> M >> K;

        // 処理装置設置場所の座標
        vector<pair<int, int>> processor_pos(N);
        for (int i = 0; i < N; i++) {
            input_file >> processor_pos[i].first >> processor_pos[i].second;
        }

        // 分別器設置場所の座標
        vector<pair<int, int>> sorter_pos(M);
        for (int i = 0; i < M; i++) {
            input_file >> sorter_pos[i].first >> sorter_pos[i].second;
        }

        // 分別器の確率
        vector<vector<double>> prob(K, vector<double>(N));
        for (int i = 0; i < K; i++) {
            for (int j = 0; j < N; j++) {
                input_file >> prob[i][j];
            }
        }

        // 基本戦略:
        // 1. i番目の処理装置をi番目の場所に設置
        // 2. 搬入口から最も近い分別器場所に接続
        // 3. その分別器で0番の分別器を使用し、最も確率の高いごみ種と低いごみ種に分ける

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

        // 搬入口の接続先
        output_file << N + nearest_sorter << "\n";

        // 0番分別器で最も確率の高いごみ種と低いごみ種を見つける
        int max_prob_type = 0, min_prob_type = 0;
        double max_prob = prob[0][0], min_prob = prob[0][0];

        for (int j = 0; j < N; j++) {
            if (prob[0][j] > max_prob) {
                max_prob = prob[0][j];
                max_prob_type = j;
            }
            if (prob[0][j] < min_prob) {
                min_prob = prob[0][j];
                min_prob_type = j;
            }
        }

        // 分別器の設置
        for (int i = 0; i < M; i++) {
            if (i == nearest_sorter) {
                // 0番分別器を設置
                // 出口1: 最も確率の高いごみ種の処理装置
                // 出口2: 最も確率の低いごみ種の処理装置
                output_file << "0 " << max_prob_type << " " << min_prob_type << "\n";
            } else {
                // 分別器を設置しない
                output_file << "-1\n";
            }
        }

        input_file.close();
        output_file.close();
        cout << "Processed seed: " << seedNum << endl;
    }

    return 0;
}
