#include <bits/stdc++.h>
using namespace std;

double wrap(double x, double length) { // [0, length) に戻す。length > 0。
    assert(length > 0);
    x = fmod(x, length);
    if (x < 0) x += length;
    return x == length ? 0 : x; // 加算の丸めで length になった場合
}

// from から to への最短の符号付き差。ちょうど半周は負方向とする。
double wrapped_delta(double from, double to, double length) {
    return wrap(to - from + length / 2, length) - length / 2;
}

double torus_distance(double x1, double y1, double x2, double y2, double w, double h) {
    return hypot(wrapped_delta(x1, x2, w), wrapped_delta(y1, y2, h));
}

int main() {
    assert(wrap(-1, 10) == 9);
    assert(wrapped_delta(9, 1, 10) == 2); // 端をまたぐ方が近い
    double distance = torus_distance(9, 0, 1, 0, 10, 10);
    assert(abs(distance - 2) < 1e-9);
    // 9 と 1 の中点は、通常の平均 5 ではなく、端をまたいだ 0。
    double midpoint = wrap(9 + wrapped_delta(9, 1, 10) / 2, 10);
    cout << distance << " " << midpoint << "\n"; // 2 0
}

/*
端がつながる座標・距離
- 負の fmod 結果は正側へ戻す。引数は有限で、各辺の長さは正とする。
- 2 点の間を移動・補間するときは、符号付きの最短差を使ってから座標を wrap する。
- ちょうど半周では両方向が最短になるため、方向の決め方を統一する。
- 複数点の重心には別の定義が必要なことがある。2 点の補間をそのまま一般化しない。
- 距離の大小だけなら dx*dx + dy*dy を比べ、平方根を省ける。
*/
