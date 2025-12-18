#include <iostream>
#include <vector>
using namespace std;

// 渦巻きパターンを生成して表示する関数
void printSpiralPattern(int n) {
    // n×nのマップを用意
    vector<vector<char>> map(n, vector<char>(n, '.'));
    
    // 中心座標
    int centerX = n / 2;
    int centerY = n / 2;
    
    // 渦巻きの方向ベクトル（右、下、左、上の順）
    int dx[] = {0, 1, 0, -1};
    int dy[] = {1, 0, -1, 0};
    
    // 渦巻きパターンを作成
    int dir = 0;         // 現在の方向（0:右, 1:下, 2:左, 3:上）
    int steps = 1;       // 現在の方向で進む歩数
    int stepCounter = 0; // 現在の方向での歩数カウンター
    int turnCount = 0;   // 方向転換の回数
    
    int x = centerX;
    int y = centerY;
    
    // 中心を特殊文字で表示
    map[x][y] = 'C';
    
    // 渦巻きパターンを生成
    while (true) {
        // 次の位置に移動
        x += dx[dir];
        y += dy[dir];
        
        // マップ範囲外の場合は終了
        if (x < 0 || x >= n || y < 0 || y >= n) {
            break;
        }
        
        // 通路を表示
        map[x][y] = '.';
        
        // 通路の両側に木を配置
        for (int i = -1; i <= 1; i++) {
            for (int j = -1; j <= 1; j++) {
                if (i == 0 && j == 0) continue; // 通路自体はスキップ
                
                int nx = x + i;
                int ny = y + j;
                
                // マップ範囲内かつ未設定の場所なら木を配置
                if (nx >= 0 && nx < n && ny >= 0 && ny < n && map[nx][ny] == '.') {
                    map[nx][ny] = 'T';
                }
            }
        }
        
        // 歩数カウンターを増やす
        stepCounter++;
        
        // 指定歩数に達したら方向を変える
        if (stepCounter == steps) {
            stepCounter = 0;
            dir = (dir + 1) % 4;  // 右→下→左→上の順に変更
            turnCount++;
            
            // 左または右に曲がった後（2回曲がるごとに）、歩数を増やす
            if (turnCount % 2 == 0) {
                steps++;
            }
        }
    }
    
    // マップを表示
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            cout << map[i][j];
        }
        cout << endl;
    }
}

int main() {
    int n;
    cout << "マップのサイズを入力してください: ";
    cin >> n;
    
    printSpiralPattern(n);
    
    return 0;
}