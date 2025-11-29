#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <utility> // pairのため

// 名前空間stdを省略
using namespace std;

// グローバル変数としてN, v, hを定義 (can_move関数から参照するため)
int N;
vector<string> v;
vector<string> h;

// 移動方向と座標の変化量を定義
map<char, pair<int, int>> DIJ;

// Pythonのcan_move関数に相当
bool can_move(int i, int j, char d) {
    if (d == 'U') {
        if (i == 0) return false;
        return h[i - 1][j] == '0';
    } else if (d == 'D') {
        if (i == N - 1) return false;
        return h[i][j] == '0';
    } else if (d == 'L') {
        if (j == 0) return false;
        return v[i][j - 1] == '0';
    } else if (d == 'R') {
        if (j == N - 1) return false;
        return v[i][j] == '0';
    }
    return false; // 'S' やその他の文字
}

// ルールを格納するための構造体
struct Rule {
    int c, q, A, S;
    char D;
};

int main() {
    // C++の入出力を高速化
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    freopen("out.txt", "w", stdout);

    int K, T;
    cin >> N >> K >> T;

    // Pythonの v = [input().strip() for _ in range(N)] に相当
    v.resize(N);
    for (int i = 0; i < N; ++i) {
        cin >> v[i];
    }

    // Pythonの h = [input().strip() for _ in range(N - 1)] に相当
    h.resize(N - 1);
    for (int i = 0; i < N - 1; ++i) {
        cin >> h[i];
    }

    // Pythonの targets = [tuple(map(int, input().split())) for _ in range(K)] に相当
    // C++では std::pair をタプルの代わりによく使います
    vector<pair<int, int>> targets(K);
    for (int i = 0; i < K; ++i) {
        cin >> targets[i].first >> targets[i].second;
    }

    // DIJ (座標変化量マップ) の初期化
    DIJ['U'] = {-1, 0};
    DIJ['D'] = {1, 0};
    DIJ['L'] = {0, -1};
    DIJ['R'] = {0, 1};

    // --- 戦略 (Pythonコードのロジックをそのまま移植) ---
    int C = N * N;
    int Q = 1;

    // Pythonの s = [[0] * N for _ in range(N)] に相当
    vector<vector<int>> s(N, vector<int>(N));
    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N; ++j) {
            s[i][j] = i * N + j;
        }
    }

    vector<Rule> rules;
    int i = targets[0].first;  // 現在位置 (i)
    int j = targets[0].second; // 現在位置 (j)
    int gi = targets[1].first; // 目的地 (i)
    int gj = targets[1].second; // 目的地 (j)

    for (int t = 0; t < T; ++t) {
        char d = 'S'; // デフォルトは停止
        if (i > gi && can_move(i, j, 'U')) {
            d = 'U';
        } else if (i < gi && can_move(i, j, 'D')) {
            d = 'D';
        } else if (j > gj && can_move(i, j, 'L')) {
            d = 'L';
        } else if (j < gj && can_move(i, j, 'R')) {
            d = 'R';
        }

        if (d == 'S') {
            break; // 移動先がない、または目的地に到達した（このロジックではマンハッタン距離が0になっても停止しないが、Pythonコードに合わせる）
        }

        // ルールを追加
        rules.push_back({s[i][j], 0, s[i][j], 0, d});

        // 現在位置を更新
        int di = DIJ[d].first;
        int dj = DIJ[d].second;
        i += di;
        j += dj;
    }

    // --- 出力 ---
    cout << C << " " << Q << " " << rules.size() << "\n";
    for (int r = 0; r < N; ++r) {
        for (int c = 0; c < N; ++c) {
            cout << s[r][c] << (c == N - 1 ? "" : " "); // 最後の要素の後ろにスペースを入れない
        }
        cout << "\n";
    }

    for (const auto& r : rules) {
        cout << r.c << " " << r.q << " " << r.A << " " << r.S << " " << r.D << "\n";
    }

    return 0;
}