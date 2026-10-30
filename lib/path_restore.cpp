#include <bits/stdc++.h>
using namespace std;

struct Node {
    int parent; // 親ノードの番号。根は -1。
    int action; // 親からこのノードに来るときの操作
};

int add_node(vector<Node>& nodes, int parent, int action) {
    nodes.push_back({parent, action});
    return (int)nodes.size() - 1;
}

vector<int> restore(const vector<Node>& nodes, int id) {
    vector<int> actions;
    while (nodes[id].parent != -1) {
        actions.push_back(nodes[id].action);
        id = nodes[id].parent;
    }
    reverse(actions.begin(), actions.end());
    return actions;
}

int main() {
    vector<Node> nodes = {{-1, -1}}; // 根。根の action は使用しない。
    int a = add_node(nodes, 0, 3);
    int b = add_node(nodes, a, 7);
    add_node(nodes, a, 9); // 別候補。a までの履歴は共有できる。
    auto actions = restore(nodes, b); // {3, 7}
    for (int action : actions) cout << action << " ";
    cout << "\n";
}

/*
親をたどって操作列を復元する
- 操作列を候補ごとにコピーせず、親番号と最後の操作だけ記録する。
- ビームでは候補を絞ってからノードを追加すると、捨てる候補の履歴を減らせる。
- nodes は後続ノードから参照される。探索途中で clear や並べ替えをしない。
- push_back で参照やポインタが無効になる場合があるので、番号で保持する。
- 復元は O(経路長)。履歴のメモリは残したノードの総数に比例する。
*/
