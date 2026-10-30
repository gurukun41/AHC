#include <bits/stdc++.h>
using namespace std;

struct Candidate { long long score; int progress; };

// 進捗の良い候補に quota 枠、残りはスコア順。返り値は候補の添字。
vector<int> select_diverse(const vector<Candidate>& candidates, size_t width, size_t quota) {
    width = min(width, candidates.size());
    quota = min(quota, width);
    vector<int> ids(candidates.size());
    iota(ids.begin(), ids.end(), 0);
    auto by_progress = [&](int a, int b) {
        if (candidates[a].progress != candidates[b].progress) return candidates[a].progress > candidates[b].progress;
        if (candidates[a].score != candidates[b].score) return candidates[a].score > candidates[b].score;
        return a < b;
    };
    partial_sort(ids.begin(), ids.begin() + quota, ids.end(), by_progress);
    vector<int> result(ids.begin(), ids.begin() + quota);
    ids.erase(ids.begin(), ids.begin() + quota); // 選んだものを次の選択対象から除く
    auto by_score = [&](int a, int b) {
        if (candidates[a].score != candidates[b].score) return candidates[a].score > candidates[b].score;
        return a < b;
    };
    size_t rest = width - quota;
    partial_sort(ids.begin(), ids.begin() + rest, ids.end(), by_score);
    result.insert(result.end(), ids.begin(), ids.begin() + rest);
    return result;
}

int main() {
    vector<Candidate> candidates = {{100, 1}, {90, 2}, {10, 9}, {80, 0}};
    auto selected = select_diverse(candidates, 3, 1);
    for (int id : selected) cout << id << " "; // 2 0 1
    cout << "\n";
}

/*
複数の評価基準でビーム枠を分ける
- 現在の点が高い候補だけでなく、進捗など別の特徴が良い候補も残す。
- 同じ候補を二重に選ばない。別候補が同じ状態になる重複は、先に除いておく。
- 枠を分ければ必ず改善するわけではない。別基準の枠を増やすほどスコア順の枠は減る。
- この返り値は全体のスコア順ではない。先頭が最良とは限らない。
*/
