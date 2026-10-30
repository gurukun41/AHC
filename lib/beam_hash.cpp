/*
ビームサーチの重複除去用ハッシュ

同じ状態なら同じハッシュになる。候補を重複除去してからビーム幅分を選ぶ。
各情報に対応する値を XOR し、変更時は古い値と新しい値を XOR する。
初期計算は O(状態の大きさ)、差分更新は O(変更箇所数)。

問題に合わせて State、Kind、calc_hash、更新関数を変える。
- 今後の操作・評価に影響する情報（位置、資源、手番など）を含める。
- 盤面の各位置や項目の種類を区別する。集合は要素、個数は (要素, 個数) で表す。
- 可変長の配列は長さも含める。途中挿入で位置がずれる要素も更新する。
- 同じ状態からの将来が同じで、良いスコアが常に不利にならない場合のみ良い方を残す。
- ハッシュ衝突を許容しない場合は、同じハッシュの候補の実際の状態も比較する。
- 開発中は差分更新後に calc_hash で再計算して確認する。
*/
#include <bits/stdc++.h>
using namespace std;
using ull = uint64_t; // 64 bit の符号なし整数。ハッシュの値を入れる型

// x: 混ぜる元の値。返り値: x から作った、乱数のように見える値。
ull mix64(ull x) {
    x += 0x9e3779b97f4a7c15ULL;
    x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
    x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
    return x ^ (x >> 31);
}

// 各情報の種類を区別する番号。LENGTH は board の要素数。
enum Kind : ull { BOARD = 1, POSITION = 2, ENERGY = 3, LENGTH = 4 };

// kind: 情報の種類、index: 要素の位置、value: その値。
// 返り値: この組に対応するハッシュ用の値。同じ組なら毎回同じ値になる。
// 配列以外の項目は index = 0 とする。負の value も扱える。
ull feature(ull kind, ull index, long long value) {
    ull h = mix64(kind); // 種類・位置・値を順に混ぜる途中の値
    h = mix64(h ^ mix64(index));
    return mix64(h ^ mix64((ull)value));
}

struct State {
    vector<int> board;
    int position;
    int energy;
    int score; // この例では、同じ状態なら大きい方を残せると仮定
    ull hash; // この状態全体のハッシュ
};

// 返り値: 状態に含める各情報を XOR したハッシュ。
ull calc_hash(const State& s) {
    ull h = feature(LENGTH, 0, (long long)s.board.size()); // 計算中のハッシュ
    for (size_t i = 0; i < s.board.size(); i++) {
        h ^= feature(BOARD, i, s.board[i]);
    }
    h ^= feature(POSITION, 0, s.position);
    h ^= feature(ENERGY, 0, s.energy);
    return h;
}

void set_cell(State& s, size_t i, int value) {
    s.hash ^= feature(BOARD, i, s.board[i]); // 古い値を除く
    s.board[i] = value;
    s.hash ^= feature(BOARD, i, s.board[i]); // 新しい値を加える
}

void set_position(State& s, int position) {
    s.hash ^= feature(POSITION, 0, s.position);
    s.position = position;
    s.hash ^= feature(POSITION, 0, s.position);
}

void set_energy(State& s, int energy) {
    s.hash ^= feature(ENERGY, 0, s.energy);
    s.energy = energy;
    s.hash ^= feature(ENERGY, 0, s.energy);
}

int main() {
    State first = {{0, 1, 2}, 0, 10, 10, 0};
    first.hash = calc_hash(first); // 初期状態を作った後に全体から計算

    State next = first;            // 候補をコピーすると hash もコピーされる
    set_cell(next, 1, 5);
    set_position(next, 2);
    set_energy(next, 9);
    assert(next.hash == calc_hash(next)); // 差分更新の確認

    State same = {{0, 5, 2}, 2, 9, 20, 0};
    same.hash = calc_hash(same);

    // 同じターンの候補から重複を除く。衝突は無視する。
    // 同じ状態なら score が大きい方を残せると仮定している。
    vector<State> candidates = {next, same};
    vector<State> unique_states;
    unordered_map<ull, size_t> index; // キー: 状態の hash、値: unique_states の位置
    for (const State& s : candidates) {
        auto it = index.find(s.hash);
        if (it == index.end()) {
            index[s.hash] = unique_states.size();
            unique_states.push_back(s);
        } else if (s.score > unique_states[it->second].score) {
            unique_states[it->second] = s;
        }
    }
    // unique_states からビーム幅分を選ぶ（top_n.cpp）。
    cout << unique_states.size() << " " << unique_states[0].score << "\n"; // 1 20
}
