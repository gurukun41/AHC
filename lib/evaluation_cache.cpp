#include <bits/stdc++.h>
using namespace std;

struct Key {
    int version, from, to; // version は盤面・ルールなどの変更を区別する番号
    bool operator==(const Key& k) const {
        return version == k.version && from == k.from && to == k.to;
    }
};
struct KeyHash {
    size_t operator()(const Key& k) const {
        size_t h = hash<int>{}(k.version);
        for (int x : {k.from, k.to}) h ^= hash<int>{}(x) + size_t(0x9e3779b9) + (h << 6) + (h >> 2);
        return h;
    }
};

int bfs_distance(const vector<vector<int>>& graph, int from, int to) {
    vector<int> dist(graph.size(), -1);
    queue<int> q;
    dist[from] = 0;
    q.push(from);
    while (!q.empty()) {
        int v = q.front(); q.pop();
        if (v == to) return dist[v];
        for (int u : graph[v]) if (dist[u] == -1) {
            dist[u] = dist[v] + 1;
            q.push(u);
        }
    }
    return -1;
}

struct DistanceCache {
    size_t capacity;
    unordered_map<Key, int, KeyHash> values;
    explicit DistanceCache(size_t limit) : capacity(limit) {}

    int get(const vector<vector<int>>& graph, int version, int from, int to) {
        Key key{version, from, to};
        auto it = values.find(key);
        if (it != values.end()) return it->second;
        int result = bfs_distance(graph, from, to);
        if (capacity > 0) {
            if (values.size() >= capacity) values.clear(); // 簡単な容量制限
            values.emplace(key, result); // 到達不能の -1 も保存する
        }
        return result;
    }
};

int main() {
    vector<vector<int>> graph = {{1}, {2}, {}};
    DistanceCache cache(100);
    assert(cache.get(graph, 0, 0, 2) == 2);
    assert(cache.get(graph, 0, 0, 2) == 2); // ここでは BFS を省く
    graph[0].push_back(2);
    assert(cache.get(graph, 1, 0, 2) == 1); // グラフ変更時は version も変更
}

/*
同じ条件の重い計算を再利用する
- 位置だけで結果が決まらない場合、向き・資源・ルールなどもキーに含める。
- 別のグラフに同じ version を使い回さない。変更時にキャッシュを clear する方法もある。
- unordered_map は hash が衝突しても == でキーを区別する。
- 例は距離だけを保存する。経路も必要なら操作列を別に保存・復元する。
- 保存量を制限する。全消去は簡単だが直後の計算が増えるため、必要なら世代別などにする。
*/
