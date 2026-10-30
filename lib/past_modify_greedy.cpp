#include <bits/stdc++.h>
using namespace std;
using ll = long long;

// 例: 仕事を追加するたび、既存の実行順序の途中にも挿入してよい。
// 各仕事の締切を守り、完了時刻 × 重みの合計を小さくする。
struct Job { ll release, duration, deadline, weight; };
struct State {
    ll time = 0, cost = 0;
    vector<char> done;
};
struct Plan {
    vector<int> actions;   // 仕事の番号
    vector<State> prefix; // prefix[t]: 最初の t 操作を実行した状態
};

bool apply(State& s, int id, const vector<Job>& jobs) {
    if (id < 0 || id >= (int)jobs.size() || s.done[id]) return false;
    const auto& job = jobs[id];
    ll end = max(s.time, job.release) + job.duration;
    if (end > job.deadline) return false; // 不正なら s は変更しない
    s.time = end;
    s.cost += end * job.weight;
    s.done[id] = true;
    return true;
}

// at は「元の at 番目の操作の直前」。actions.size() なら末尾へ挿入。
optional<Plan> try_insert(const Plan& current, int at, int action, const vector<Job>& jobs) {
    assert(0 <= at && at <= (int)current.actions.size());
    Plan candidate;
    candidate.actions = current.actions;
    candidate.actions.insert(candidate.actions.begin() + at, action);
    candidate.prefix.assign(current.prefix.begin(), current.prefix.begin() + at + 1);

    // 挿入直前の状態から、新しい操作と既存の後続操作を再生する。
    for (int t = at; t < (int)candidate.actions.size(); t++) {
        State next = candidate.prefix.back();
        if (!apply(next, candidate.actions[t], jobs)) return nullopt;
        candidate.prefix.push_back(std::move(next));
    }
    return candidate;
}

bool insert_greedily(Plan& current, int action, const vector<Job>& jobs) {
    optional<Plan> best;
    for (int at = 0; at <= (int)current.actions.size(); at++) {
        auto candidate = try_insert(current, at, action, jobs);
        if (candidate && (!best || candidate->prefix.back().cost < best->prefix.back().cost)) {
            best = std::move(candidate);
        }
    }
    if (!best) return false; // 全候補が不正なら、元の解は変更しない
    current = std::move(*best); // 操作列と途中状態をまとめて更新
    return true;
}

int main() {
    vector<Job> jobs = {{0, 3, 10, 1}, {0, 1, 2, 10}, {0, 2, 7, 2}};
    Plan plan{{}, {{0, 0, vector<char>(jobs.size(), false)}}};
    for (int id = 0; id < (int)jobs.size(); id++) {
        if (!insert_greedily(plan, id, jobs)) {
            cerr << "cannot insert job " << id << "\n";
            return 1;
        }
    }
    for (int id : plan.actions) cout << id << " "; // 1 2 0
    cout << "\n" << plan.prefix.back().cost << "\n"; // 22
}

/*
過去の操作列へ挿入する貪欲
1. 既存の解の各時点の状態を保存する。
2. 挿入する操作と位置を決め、その直前の状態から後ろを再生する。
3. 後続操作の合法性を確認し、完成した候補の評価で最良の挿入位置を選ぶ。

- 例では仕事 0 の後に仕事 1 を追加すると締切に間に合わず、過去へ挿入すると実行できる。
- 元の後続操作が合法でも、挿入後は不正になり得る。挿入操作だけの判定では足りない。
- 問題に合わせて Job/State、apply、挿入候補の生成、最終評価を変更する。
  盤面なら過去の位置での設置・回収などを候補にし、後の移動や資源への影響を検証する。
- action を追加した候補同士で比較する。追加前と比べると仕事数が異なり、コストは普通増える。
- 貪欲なので、後で挿入不能になっても問題全体が不可能とは限らない。仕事の追加順も探索できる。
- prefix は操作の直前。挿入すると後ろの添字がずれるため、古い途中状態を流用しない。
- この例は候補を別に作る。重い場合は保存間隔を広げる、候補位置を絞るなどを検討する。
  操作数 T の全位置を試すと再生は O(T^2) 回。状態コピーの費用も別にかかる。
- 後ろを貪欲に作り直す型なら、挿入後の再生部分を「次の操作を選んで適用」に置き換える。
- 全操作列を出力前に計画する場面向け。実際に実行・出力済みの操作を変更できるわけではない。
- jobs の属性は不変、duration/weight は非負、時刻・積・合計は ll に収まる前提。
  最終的には初期状態から全操作を再生して確認する（validate_plan.cpp）。

過去への操作挿入の実例（このサンプルは別の簡単な題材で実装）:
https://utac.hateblo.jp/entry/2025/04/28/151927
*/
