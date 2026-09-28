#include <bits/stdc++.h>
using namespace std;

struct Pos {
    int r, c;
};

struct Op {
    int r, c;
    int k;
    char dir;
    int len;
};

static const int MAX_OP = 100000;

// Homogeneous blocks are unchanged by reversal. Replace only disjoint,
// consecutive pairs whose intermediate cell starts and ends empty. Mixed
// payloads must keep their number of flips, so they are never merged here.
vector<Op> compressBaselineMoves(const vector<string>& grid, int K,
                         const vector<Op>& ops) {
    const int n = (int)grid.size();
    vector<vector<vector<int>>> board(n, vector<vector<int>>(n));
    vector<vector<int>> nests(n, vector<int>(n, -1));
    for (int r = 0; r < n; ++r) {
        for (int c = 0; c < n; ++c) {
            char ch = grid[r][c];
            if ('a' <= ch && ch < 'a' + K) {
                board[r][c].push_back(ch - 'a');
            } else if ('A' <= ch && ch < 'A' + K) {
                nests[r][c] = ch - 'A';
            }
        }
    }

    auto floorCell = [&](int r, int c) {
        return 0 <= r && r < n && 0 <= c && c < n &&
               grid[r][c] != '#';
    };
    auto direction = [](char dir) -> pair<int, int> {
        if (dir == 'U') return {-1, 0};
        if (dir == 'D') return {1, 0};
        if (dir == 'L') return {0, -1};
        if (dir == 'R') return {0, 1};
        return {0, 0};
    };
    auto home = [&](int r, int c) {
        int color = nests[r][c];
        if (color < 0) return;
        while (!board[r][c].empty() && board[r][c].back() == color) {
            board[r][c].pop_back();
        }
    };

    // Replay the original operations, including reversal and homing at both
    // endpoints. Any unexpected invalid operation disables postprocessing.
    auto replay = [&](const Op& op) {
        if (!floorCell(op.r, op.c)) return false;
        auto [dr, dc] = direction(op.dir);
        int h = (int)board[op.r][op.c].size();
        if ((dr == 0 && dc == 0) || op.k < 0 || op.k >= h ||
            op.len < 1 || op.len > op.k + 1) {
            return false;
        }
        for (int step = 1; step <= op.len; ++step) {
            if (!floorCell(op.r + dr * step, op.c + dc * step)) {
                return false;
            }
        }
        int tr = op.r + dr * op.len;
        int tc = op.c + dc * op.len;
        if ((int)board[tr][tc].size() + h - op.k > 8) return false;

        vector<int> moving(board[op.r][op.c].begin() + op.k,
                           board[op.r][op.c].end());
        board[op.r][op.c].resize(op.k);
        reverse(moving.begin(), moving.end());
        board[tr][tc].insert(board[tr][tc].end(), moving.begin(), moving.end());
        home(op.r, op.c);
        home(tr, tc);
        return true;
    };

    vector<Op> result;
    result.reserve(ops.size());
    for (size_t i = 0; i < ops.size();) {
        const Op& first = ops[i];
        bool canMerge = false;
        if (i + 1 < ops.size() && first.len == 1 && first.k >= 1 &&
            floorCell(first.r, first.c)) {
            const Op& second = ops[i + 1];
            auto [dr, dc] = direction(first.dir);
            int br = first.r + dr;
            int bc = first.c + dc;
            int cr = br + dr;
            int cc = bc + dc;
            int h = (int)board[first.r][first.c].size();
            if ((dr != 0 || dc != 0) && first.k < h &&
                second.len == 1 && second.k == 0 &&
                second.dir == first.dir && second.r == br && second.c == bc &&
                floorCell(br, bc) && floorCell(cr, cc) &&
                board[br][bc].empty() &&
                (int)board[cr][cc].size() + h - first.k <= 8) {
                int color = board[first.r][first.c].back();
                // Homing at B would change the payload. C may be its own nest.
                canMerge = nests[br][bc] != color;
                for (int j = first.k; j < h; ++j) {
                    if (board[first.r][first.c][j] != color) canMerge = false;
                }
            }
        }

        if (!replay(first)) return ops;
        if (canMerge) {
            if (!replay(ops[i + 1])) return ops;
            Op combined = first;
            combined.len = 2;
            result.push_back(combined);
            i += 2;
        } else {
            result.push_back(first);
            ++i;
        }
    }
    return result;
}

// A compact exact simulator shared by both postprocessors. Padding above h is
// deliberately excluded from equality: only the ordered live stack matters.
struct PostprocessBoard {
    struct Cell {
        array<unsigned char, 8> color{};
        int h = 0;
        bool operator==(const Cell& other) const {
            return h == other.h &&
                   equal(color.begin(), color.begin() + h, other.color.begin());
        }
    };
    const vector<string>& grid;
    int n;
    vector<int> nest;
    vector<Cell> initial;

    PostprocessBoard(const vector<string>& grid_, int K)
        : grid(grid_), n((int)grid.size()), nest(n * n, -1), initial(n * n) {
        for (int v = 0; v < n * n; ++v) {
            char ch = grid[v / n][v % n];
            if ('a' <= ch && ch < 'a' + K) {
                initial[v].color[0] = ch - 'a';
                initial[v].h = 1;
            } else if ('A' <= ch && ch < 'A' + K) {
                nest[v] = ch - 'A';
            }
        }
    }

    bool floorCell(int r, int c) const {
        return 0 <= r && r < n && 0 <= c && c < n && grid[r][c] != '#';
    }

    bool endpoints(const Op& op, int& from, int& to) const {
        if (!floorCell(op.r, op.c) || op.k < 0 || op.k >= 8 ||
            op.len < 1 || op.len > op.k + 1) return false;
        int dr = 0, dc = 0;
        if (op.dir == 'U') dr = -1;
        else if (op.dir == 'D') dr = 1;
        else if (op.dir == 'L') dc = -1;
        else if (op.dir == 'R') dc = 1;
        else return false;
        for (int step = 1; step <= op.len; ++step) {
            if (!floorCell(op.r + dr * step, op.c + dc * step)) return false;
        }
        from = op.r * n + op.c;
        to = (op.r + dr * op.len) * n + op.c + dc * op.len;
        return true;
    }

    static bool transfer(Cell& from, Cell& to, int k, int fromNest, int toNest) {
        if (k < 0 || k >= from.h || to.h + from.h - k > 8) return false;
        // Capacity is checked before either endpoint homes its top slimes.
        for (int j = from.h - 1; j >= k; --j) to.color[to.h++] = from.color[j];
        from.h = k;
        while (from.h > 0 && from.color[from.h - 1] == fromNest) --from.h;
        while (to.h > 0 && to.color[to.h - 1] == toNest) --to.h;
        return true;
    }

    bool apply(vector<Cell>& board, const Op& op) const {
        int from, to;
        return endpoints(op, from, to) &&
               transfer(board[from], board[to], op.k, nest[from], nest[to]);
    }
};

// Collapse a straight chain, allowing independent operations between its
// links. Every skipped operation must avoid every endpoint of the chain.
vector<Op> compressMoves(
    const vector<string>& grid, int K, const vector<Op>& ops,
    chrono::steady_clock::time_point deadline = chrono::steady_clock::time_point::max()) {
    PostprocessBoard sim(grid, K);
    auto board = sim.initial;
    vector<Op> result;
    result.reserve(ops.size());
    for (size_t i = 0; i < ops.size();) {
        if (chrono::steady_clock::now() >= deadline) {
            result.insert(result.end(), ops.begin() + i, ops.end());
            break;
        }
        const Op& first = ops[i];
        int from, current;
        if (!sim.endpoints(first, from, current)) return ops;
        auto original = board;
        int payload = board[from].h - first.k;
        int base = board[current].h;
        if (!sim.apply(original, first)) return ops;
        bool palindrome = true;
        for (int a = first.k, b = board[from].h - 1; a < b; ++a, --b) {
            if (board[from].color[a] != board[from].color[b]) palindrome = false;
        }

        vector<int> chainCells{from, current}, skippedCells;
        vector<Op> skipped, bestReplacement;
        vector<PostprocessBoard::Cell> bestBoard;
        size_t bestEnd = i;
        int distance = first.len, links = 1;
        if (distance < first.k + 1 && original[current].h == base + payload) {
            for (size_t j = i + 1; j < ops.size() && j < i + 24; ++j) {
                if (chrono::steady_clock::now() >= deadline) break;
                const Op& next = ops[j];
                int a, b;
                if (!sim.endpoints(next, a, b)) return ops;
                bool continues = a == current && next.dir == first.dir &&
                                 next.k == base && distance + next.len <= first.k + 1;
                if (!continues) {
                    if (find(chainCells.begin(), chainCells.end(), a) != chainCells.end() ||
                        find(chainCells.begin(), chainCells.end(), b) != chainCells.end()) break;
                    if (!sim.apply(original, next)) return ops;
                    skipped.push_back(next);
                    skippedCells.push_back(a);
                    skippedCells.push_back(b);
                    continue;
                }
                if (find(skippedCells.begin(), skippedCells.end(), b) != skippedCells.end()) break;
                int nextBase = original[b].h;
                if (!sim.apply(original, next)) return ops;
                distance += next.len;
                ++links;
                chainCells.push_back(b);
                current = b;
                base = nextBase;

                // One replacement flips once. Even-length chains can only
                // match that ordering when their payload is palindromic.
                if ((links & 1) || palindrome) {
                    Op combined = first;
                    combined.len = distance;
                    auto replacement = board;
                    bool valid = sim.apply(replacement, combined);
                    for (const Op& middle : skipped) {
                        if (!valid || !sim.apply(replacement, middle)) {
                            valid = false;
                            break;
                        }
                    }
                    if (valid && replacement == original) {
                        bestReplacement = {combined};
                        bestReplacement.insert(bestReplacement.end(), skipped.begin(), skipped.end());
                        bestBoard = original;
                        bestEnd = j + 1;
                    }
                }
                if (distance == first.k + 1 || original[current].h != base + payload) break;
            }
        }
        if (bestEnd > i) {
            result.insert(result.end(), bestReplacement.begin(), bestReplacement.end());
            board = std::move(bestBoard);
            i = bestEnd;
        } else {
            if (!sim.apply(board, first)) return ops;
            result.push_back(first);
            ++i;
        }
    }
    return result;
}

// Search only the cells touched by a short original window. All search moves
// use the same exact transition as final replacement verification.
vector<Op> optimizeMoveWindows(
    const vector<string>& grid, int K, const vector<Op>& ops,
    chrono::steady_clock::time_point deadline) {
    PostprocessBoard sim(grid, K);
    using Cell = PostprocessBoard::Cell;
    using State = array<Cell, 6>;
    struct Edge { int from, to; Op op; };
    struct Node { State state; int parent; Op op; int score; };
    auto board = sim.initial;
    vector<Op> result;
    result.reserve(ops.size());
    int searched = 0;
    for (size_t i = 0; i < ops.size();) {
        if (chrono::steady_clock::now() >= deadline || searched >= 128 || i + 3 > ops.size()) {
            result.insert(result.end(), ops.begin() + i, ops.end());
            break;
        }
        vector<int> cells;
        auto original = board;
        size_t end = i;
        for (; end < ops.size() && end < i + 4; ++end) {
            int from, to;
            if (!sim.endpoints(ops[end], from, to)) return ops;
            vector<int> nextCells = cells;
            for (int v : {from, to}) {
                if (find(nextCells.begin(), nextCells.end(), v) == nextCells.end()) nextCells.push_back(v);
            }
            if (nextCells.size() > 6) break;
            if (!sim.apply(original, ops[end])) return ops;
            cells = std::move(nextCells);
        }
        bool found = false;
        vector<Op> replacement;
        if (end >= i + 3) {
            ++searched;
            const int count = (int)cells.size();
            State start{}, target{};
            array<int, 26> targetCounts{};
            for (int c = 0; c < count; ++c) {
                start[c] = board[cells[c]];
                target[c] = original[cells[c]];
                for (int j = 0; j < target[c].h; ++j) ++targetCounts[target[c].color[j]];
            }
            vector<Edge> edges;
            for (int a = 0; a < count; ++a) {
                for (int b = 0; b < count; ++b) {
                    if (a == b) continue;
                    int ar = cells[a] / sim.n, ac = cells[a] % sim.n;
                    int br = cells[b] / sim.n, bc = cells[b] % sim.n;
                    if (ar != br && ac != bc) continue;
                    int len = abs(ar - br) + abs(ac - bc);
                    if (len > 8) continue;
                    char dir = ar == br ? (ac < bc ? 'R' : 'L') : (ar < br ? 'D' : 'U');
                    Op op{ar, ac, len - 1, dir, len};
                    int from, to;
                    if (sim.endpoints(op, from, to)) edges.push_back({a, b, op});
                }
            }
            auto scoreState = [&](const State& state) {
                int score = 0;
                array<int, 26> counts{};
                for (int c = 0; c < count; ++c) {
                    score += 3 * abs(state[c].h - target[c].h);
                    int prefix = 0;
                    while (prefix < min(state[c].h, target[c].h) &&
                           state[c].color[prefix] == target[c].color[prefix]) ++prefix;
                    score += 2 * (min(state[c].h, target[c].h) - prefix);
                    for (int j = 0; j < state[c].h; ++j) ++counts[state[c].color[j]];
                }
                // Homed pieces never return, so losing a required color is
                // irreparable while the search is confined to these cells.
                for (int c = 0; c < 26; ++c) {
                    if (counts[c] < targetCounts[c]) return INT_MAX;
                    score += counts[c] - targetCounts[c];
                }
                return score;
            };
            auto hashState = [&](const State& state) {
                uint64_t hash = 1469598103934665603ULL;
                for (int c = 0; c < count; ++c) {
                    hash = (hash ^ (uint64_t)(state[c].h + 1)) * 1099511628211ULL;
                    for (int j = 0; j < state[c].h; ++j) {
                        hash = (hash ^ (uint64_t)(state[c].color[j] + 17)) * 1099511628211ULL;
                    }
                }
                return hash;
            };
            vector<Node> nodes;
            nodes.reserve(768);
            nodes.push_back({start, -1, Op{}, scoreState(start)});
            vector<int> beam{0};
            unordered_set<uint64_t> seen;
            seen.reserve(1024);
            seen.insert(hashState(start));
            int finish = start == target ? 0 : -1;
            int expanded = 0, attempted = 0;
            bool stop = false;
            for (int depth = 1; depth < (int)(end - i) && finish < 0 && !stop; ++depth) {
                vector<int> nextBeam;
                for (int id : beam) {
                    if (++expanded > 48 || chrono::steady_clock::now() >= deadline) {
                        stop = true;
                        break;
                    }
                    // Copy the parent because pushing children can move the
                    // vector storage if its configured limit is changed.
                    State state = nodes[id].state;
                    for (const Edge& edge : edges) {
                        for (int k = edge.op.len - 1; k < state[edge.from].h; ++k) {
                            if (++attempted > 2048 || nodes.size() >= 768 ||
                                ((attempted & 31) == 0 && chrono::steady_clock::now() >= deadline)) {
                                stop = true;
                                break;
                            }
                            State child = state;
                            if (!PostprocessBoard::transfer(child[edge.from], child[edge.to], k,
                                    sim.nest[cells[edge.from]], sim.nest[cells[edge.to]])) continue;
                            int score = scoreState(child);
                            if (score == INT_MAX) continue;
                            if (!seen.insert(hashState(child)).second) continue;
                            Op op = edge.op;
                            op.k = k;
                            int childId = (int)nodes.size();
                            nodes.push_back({child, id, op, score});
                            if (child == target) {
                                finish = childId;
                                break;
                            }
                            nextBeam.push_back(childId);
                        }
                        if (stop || finish >= 0) break;
                    }
                    if (stop || finish >= 0) break;
                }
                if (nextBeam.empty()) break;
                auto better = [&](int a, int b) {
                    return nodes[a].score != nodes[b].score ? nodes[a].score < nodes[b].score : a < b;
                };
                if (nextBeam.size() > 20) {
                    nth_element(nextBeam.begin(), nextBeam.begin() + 20, nextBeam.end(), better);
                    nextBeam.resize(20);
                }
                sort(nextBeam.begin(), nextBeam.end(), better);
                beam = std::move(nextBeam);
            }
            if (finish >= 0) {
                for (int id = finish; nodes[id].parent >= 0; id = nodes[id].parent) replacement.push_back(nodes[id].op);
                reverse(replacement.begin(), replacement.end());
                // Revalidate on the entire board, not just the searched cells.
                auto verified = board;
                bool valid = replacement.size() < end - i;
                for (const Op& op : replacement) {
                    if (!valid || !sim.apply(verified, op)) {
                        valid = false;
                        break;
                    }
                }
                if (valid && verified == original) found = true;
            }
        }
        if (found) {
            result.insert(result.end(), replacement.begin(), replacement.end());
            board = std::move(original);
            i = end;
        } else {
            if (!sim.apply(board, ops[i])) return ops;
            result.push_back(ops[i]);
            ++i;
        }
    }
    return result;
}

struct RouteTree {
    vector<int> parent, depth, direction, straight;
};

struct RouteFamily {
    string name;
    vector<RouteTree> trees;
};

// Grow each color's tree toward its slimes. Favor both crossing springboards
// and edges shared in the same direction, where mixed stacks can travel together.
vector<RouteTree> buildCrossingTrees(
    const vector<string>& grid, int K,
    chrono::steady_clock::time_point refinementDeadline) {
    const int n = (int)grid.size(), cells = n * n;
    const int dr[4] = {-1, 1, 0, 0};
    const int dc[4] = {0, 0, -1, 1};
    vector<array<int, 4>> neighbor(cells);
    vector<int> root(K, -1), slimeColor(cells, -1);
    vector<vector<int>> slimes(K);
    for (int v = 0; v < cells; ++v) {
        neighbor[v].fill(-1);
        char ch = grid[v / n][v % n];
        if (ch == '#') continue;
        if ('A' <= ch && ch < 'A' + K) root[ch - 'A'] = v;
        if ('a' <= ch && ch < 'a' + K) {
            slimeColor[v] = ch - 'a';
            slimes[ch - 'a'].push_back(v);
        }
        for (int d = 0; d < 4; ++d) {
            int r = v / n + dr[d], c = v % n + dc[d];
            if (0 <= r && r < n && 0 <= c && c < n && grid[r][c] != '#')
                neighbor[v][d] = r * n + c;
        }
    }

    // Bound every route by its shortest distance to the nest plus a small
    // allowance. This prevents shared corridors from causing long detours.
    vector<vector<int>> shortest(K, vector<int>(cells, -1));
    for (int color = 0; color < K; ++color) {
        queue<int> q;
        shortest[color][root[color]] = 0;
        q.push(root[color]);
        while (!q.empty()) {
            int v = q.front();
            q.pop();
            for (int next : neighbor[v]) {
                if (next < 0 || shortest[color][next] >= 0) continue;
                shortest[color][next] = shortest[color][v] + 1;
                q.push(next);
            }
        }
    }

    struct RouteSupport {
        array<double, 2> axis{};
        array<double, 4> direction{};
    };
    using Support = vector<RouteSupport>;
    const Support emptySupport(cells);
    auto build = [&](int color, const Support& foreign, bool timed) -> optional<RouteTree> {
        RouteTree tree;
        tree.parent.assign(cells, -1);
        tree.depth.assign(cells, -1);
        tree.direction.assign(cells, -1);
        tree.straight.assign(cells, 0);
        tree.depth[root[color]] = 0;
        int remaining = (int)slimes[color].size();

        // Keep crossing and same-direction sharing separate: opposite traffic
        // can provide a springboard, but cannot be carried as one mixed stack.
        vector<array<double, 2>> crossing(cells);
        vector<array<double, 4>> sharing(cells);
        for (int v = 0; v < cells; ++v) {
            for (int axis = 0; axis < 2; ++axis) {
                double amount = max(0.0, foreign[v].axis[1 - axis]) +
                                0.5 * max(0.0, foreign[v].axis[axis]);
                crossing[v][axis] = 1.0 - exp(-amount);
            }
            for (int d = 0; d < 4; ++d)
                sharing[v][d] = 1.0 - exp(-max(0.0, foreign[v].direction[d]));
        }

        while (remaining > 0) {
            if (timed && chrono::steady_clock::now() >= refinementDeadline) return nullopt;
            vector<double> distance(cells, 1e100);
            vector<int> previous(cells, -1), routeDepth = tree.depth;
            vector<int> direction = tree.direction, straight = tree.straight;
            vector<bool> settled(cells, false);
            using Entry = pair<double, int>;
            priority_queue<Entry, vector<Entry>, greater<Entry>> q;
            for (int v = 0; v < cells; ++v) {
                if (tree.depth[v] < 0) continue;
                distance[v] = 0.04 * tree.depth[v];
                q.push({distance[v], v});
            }

            int target = -1;
            // One settled label per cell is a routing heuristic, rather than
            // an exact direction-state shortest path. Every predecessor is
            // already settled, so the resulting physical path cannot cycle.
            while (!q.empty()) {
                auto [cost, p] = q.top();
                q.pop();
                if (settled[p] || cost > distance[p]) continue;
                settled[p] = true;
                if (tree.depth[p] < 0 && slimeColor[p] == color) {
                    target = p;
                    break;
                }
                for (int outward = 0; outward < 4; ++outward) {
                    int v = neighbor[p][outward];
                    if (v < 0 || tree.depth[v] >= 0 || settled[v]) continue;
                    int nextDepth = routeDepth[p] + 1;
                    int direct = shortest[color][v];
                    if (nextDepth > direct + max(2, direct / 3)) continue;
                    int towardNest = outward ^ 1;
                    int run = direction[p] == towardNest ? min(8, straight[p] + 1) : 1;
                    double lengthValue = (run - 1) / 7.0;
                    double reward = 0.45 * crossing[v][towardNest / 2] *
                                    (0.25 + 0.75 * lengthValue) +
                                    0.25 * sharing[v][towardNest];
                    bool bends = direction[p] >= 0 && direction[p] != towardNest;
                    double edgeCost = 1.0 + 0.12 * bends - 0.08 * lengthValue - reward;
                    // edgeCost >= 0.22 even with maximal crossing support.
                    double nextCost = cost + edgeCost;
                    if (nextCost >= distance[v]) continue;
                    distance[v] = nextCost;
                    previous[v] = p;
                    routeDepth[v] = nextDepth;
                    direction[v] = towardNest;
                    straight[v] = run;
                    q.push({nextCost, v});
                }
            }
            if (target < 0) return nullopt;

            // The downstream path, including its outgoing straight length,
            // is now fixed and becomes a seed for the next slime connection.
            for (int v = target; tree.depth[v] < 0; v = previous[v]) {
                tree.parent[v] = previous[v];
                tree.depth[v] = routeDepth[v];
                tree.direction[v] = direction[v];
                tree.straight[v] = straight[v];
                if (slimeColor[v] == color) --remaining;
            }
        }
        return tree;
    };

    vector<RouteTree> trees(K);
    for (int color = 0; color < K; ++color) {
        auto tree = build(color, emptySupport, false);
        if (!tree) return {}; // Connected floors make this unreachable.
        trees[color] = std::move(*tree);
    }
    Support total = emptySupport;
    auto addSupport = [&](Support& support, const RouteTree& tree, double sign) {
        for (int v = 0; v < cells; ++v) {
            if (tree.parent[v] < 0) continue; // No departures from a nest.
            double value = 0.25 + 0.75 * (tree.straight[v] - 1) / 7.0;
            support[v].axis[tree.direction[v] / 2] += sign * value;
            support[v].direction[tree.direction[v]] += sign;
        }
    };
    for (const auto& tree : trees) addSupport(total, tree, 1.0);

    // Revisiting colors lets earlier routes see later colors as well. This is
    // fixed geometric rerouting, with no schedule annealing or score trials.
    for (int pass = 0; pass < 3; ++pass) {
        for (int i = 0; i < K; ++i) {
            if (chrono::steady_clock::now() >= refinementDeadline) return trees;
            int color = pass % 2 == 0 ? i : K - 1 - i;
            Support foreign = total;
            addSupport(foreign, trees[color], -1.0);
            auto tree = build(color, foreign, true);
            if (!tree) return trees;
            addSupport(total, trees[color], -1.0);
            trees[color] = std::move(*tree);
            addSupport(total, trees[color], 1.0);
        }
    }
    return trees;
}

// Keep the plain routes as an independent candidate. Geometric crossings and
// capacity-aware shared corridors are separate refinements of that candidate.
vector<RouteFamily> buildRouteFamilies(
    const vector<string>& grid, int K,
    chrono::steady_clock::time_point refinementDeadline) {
    const int n = (int)grid.size(), cells = n * n;
    const int dr[4] = {-1, 1, 0, 0};
    const int dc[4] = {0, 0, -1, 1};
    vector<array<int, 4>> neighbor(cells);
    vector<int> root(K, -1), slimeColor(cells, -1);
    vector<vector<int>> slimes(K);
    for (int v = 0; v < cells; ++v) {
        neighbor[v].fill(-1);
        char ch = grid[v / n][v % n];
        if (ch == '#') continue;
        if ('A' <= ch && ch < 'A' + K) root[ch - 'A'] = v;
        if ('a' <= ch && ch < 'a' + K) {
            slimeColor[v] = ch - 'a';
            slimes[ch - 'a'].push_back(v);
        }
        for (int d = 0; d < 4; ++d) {
            int r = v / n + dr[d], c = v % n + dc[d];
            if (0 <= r && r < n && 0 <= c && c < n && grid[r][c] != '#')
                neighbor[v][d] = r * n + c;
        }
    }

    // Bound every route by its shortest distance to the nest plus a small
    // allowance. This prevents shared corridors from causing long detours.
    vector<vector<int>> shortest(K, vector<int>(cells, -1));
    vector<vector<int>> bfsOrder(K);
    for (int color = 0; color < K; ++color) {
        queue<int> q;
        shortest[color][root[color]] = 0;
        q.push(root[color]);
        while (!q.empty()) {
            int v = q.front();
            q.pop();
            bfsOrder[color].push_back(v);
            for (int next : neighbor[v]) {
                if (next < 0 || shortest[color][next] >= 0) continue;
                shortest[color][next] = shortest[color][v] + 1;
                q.push(next);
            }
        }
    }

    // Always have a complete route before attempting a timed weighted rebuild.
    // Pruning a shortest-path tree keeps exactly the paths used by this color.
    auto fallback = [&](int color) {
        RouteTree tree;
        tree.parent.assign(cells, -1);
        tree.depth = shortest[color];
        tree.direction.assign(cells, -1);
        tree.straight.assign(cells, 0);
        for (int v : bfsOrder[color]) {
            if (v == root[color]) continue;
            int best = -1, bestRun = -1;
            for (int d = 0; d < 4; ++d) {
                int p = neighbor[v][d];
                if (p < 0 || shortest[color][p] + 1 != shortest[color][v]) continue;
                int run = tree.direction[p] == d ? min(8, tree.straight[p] + 1) : 1;
                if (run > bestRun) {
                    best = d;
                    bestRun = run;
                }
            }
            tree.parent[v] = neighbor[v][best];
            tree.direction[v] = best;
            tree.straight[v] = bestRun;
        }
        vector<bool> needed(cells, false);
        needed[root[color]] = true;
        for (int s : slimes[color]) {
            for (int v = s; v >= 0 && !needed[v]; v = tree.parent[v]) needed[v] = true;
        }
        for (int v = 0; v < cells; ++v) {
            if (needed[v]) continue;
            tree.parent[v] = tree.depth[v] = tree.direction[v] = -1;
            tree.straight[v] = 0;
        }
        return tree;
    };

    auto treeLoad = [&](int color, const RouteTree& tree) {
        vector<int> load(cells), order;
        for (int s : slimes[color]) if (tree.depth[s] >= 0) ++load[s];
        for (int v = 0; v < cells; ++v) if (tree.depth[v] >= 0) order.push_back(v);
        sort(order.begin(), order.end(), [&](int a, int b) { return tree.depth[a] > tree.depth[b]; });
        for (int v : order) if (tree.parent[v] >= 0) load[tree.parent[v]] += load[v];
        return load;
    };
    using Support = vector<array<double, 2>>;
    using Flow = vector<array<int, 4>>;
    const Support emptySupport(cells, array<double, 2>{0.0, 0.0});
    const Flow emptyFlow(cells, array<int, 4>{0, 0, 0, 0});
    auto build = [&](int color, const Support& foreign, const Flow& foreignFlow,
                     int style, chrono::steady_clock::time_point deadline) -> optional<RouteTree> {
        RouteTree tree;
        tree.parent.assign(cells, -1);
        tree.depth.assign(cells, -1);
        tree.direction.assign(cells, -1);
        tree.straight.assign(cells, 0);
        tree.depth[root[color]] = 0;
        int remaining = (int)slimes[color].size();

        // Crossing discounts are fixed during this rebuild. Perpendicular
        // routes are preferred; sharing a corridor can also provide support.
        Support crossing(cells);
        for (int v = 0; v < cells; ++v) {
            for (int axis = 0; axis < 2; ++axis) {
                double amount = max(0.0, foreign[v][1 - axis]) +
                                0.5 * max(0.0, foreign[v][axis]);
                crossing[v][axis] = 1.0 - exp(-amount);
            }
        }

        while (remaining > 0) {
            if (chrono::steady_clock::now() >= deadline) return nullopt;
            vector<double> distance(cells, 1e100);
            vector<int> previous(cells, -1), routeDepth = tree.depth;
            vector<int> direction = tree.direction, straight = tree.straight;
            vector<bool> settled(cells, false);
            using Entry = pair<double, int>;
            priority_queue<Entry, vector<Entry>, greater<Entry>> q;
            vector<double> seedCost(cells);
            if (style == 2) {
                vector<int> load = treeLoad(color, tree), order;
                for (int v = 0; v < cells; ++v) if (tree.depth[v] >= 0) order.push_back(v);
                sort(order.begin(), order.end(), [&](int a, int b) { return tree.depth[a] < tree.depth[b]; });
                for (int v : order) {
                    int p = tree.parent[v];
                    if (p < 0) continue;
                    int flow = foreignFlow[v][tree.direction[v]] + load[v];
                    int extraBatch = (flow + 8) / 8 - (flow + 7) / 8;
                    seedCost[v] = seedCost[p] + 0.04 + 0.12 * extraBatch;
                }
            }
            for (int v = 0; v < cells; ++v) {
                if (tree.depth[v] < 0) continue;
                distance[v] = style == 2 ? seedCost[v] : 0.04 * tree.depth[v];
                q.push({distance[v], v});
            }

            int target = -1;
            int settledCount = 0;
            // One settled label per cell is a routing heuristic, rather than
            // an exact direction-state shortest path. Every predecessor is
            // already settled, so the resulting physical path cannot cycle.
            while (!q.empty()) {
                auto [cost, p] = q.top();
                q.pop();
                if (settled[p] || cost > distance[p]) continue;
                settled[p] = true;
                if ((++settledCount & 127) == 0 && chrono::steady_clock::now() >= deadline)
                    return nullopt;
                if (tree.depth[p] < 0 && slimeColor[p] == color) {
                    target = p;
                    break;
                }
                for (int outward = 0; outward < 4; ++outward) {
                    int v = neighbor[p][outward];
                    if (v < 0 || tree.depth[v] >= 0 || settled[v]) continue;
                    int nextDepth = routeDepth[p] + 1;
                    int direct = shortest[color][v];
                    if (nextDepth > direct + max(2, direct / 3)) continue;
                    int towardNest = outward ^ 1;
                    int run = direction[p] == towardNest ? min(8, straight[p] + 1) : 1;
                    double lengthValue = (run - 1) / 7.0;
                    double reward = 0.0;
                    if (style == 1) {
                        reward = 0.70 * crossing[v][towardNest / 2] *
                                 (0.25 + 0.75 * lengthValue);
                    } else if (style == 2) {
                        // A single newly connected slime occupies one slot.
                        // Count only traffic using this exact directed edge;
                        // opposite traffic cannot share its transport batch.
                        int flow = foreignFlow[v][towardNest];
                        int extraBatch = (flow + 8) / 8 - (flow + 7) / 8;
                        int sharedRun = 0, at = v;
                        while (at >= 0 && sharedRun < 8 && foreignFlow[at][towardNest] > 0) {
                            ++sharedRun;
                            at = neighbor[at][towardNest];
                        }
                        reward = 0.70 * (1 - extraBatch) * (0.60 + 0.40 * sharedRun / 8.0);
                    }
                    bool bends = direction[p] >= 0 && direction[p] != towardNest;
                    double edgeCost = 1.0 + 0.12 * bends - 0.08 * lengthValue - reward;
                    // edgeCost >= 0.22 even with maximal crossing support.
                    double nextCost = cost + edgeCost;
                    if (nextCost >= distance[v]) continue;
                    distance[v] = nextCost;
                    previous[v] = p;
                    routeDepth[v] = nextDepth;
                    direction[v] = towardNest;
                    straight[v] = run;
                    q.push({nextCost, v});
                }
            }
            if (target < 0) return nullopt;

            // The downstream path, including its outgoing straight length,
            // is now fixed and becomes a seed for the next slime connection.
            for (int v = target; tree.depth[v] < 0; v = previous[v]) {
                tree.parent[v] = previous[v];
                tree.depth[v] = routeDepth[v];
                tree.direction[v] = direction[v];
                tree.straight[v] = straight[v];
                if (slimeColor[v] == color) --remaining;
            }
        }
        return tree;
    };

    RouteFamily plain{"plain", vector<RouteTree>(K)};
    for (int color = 0; color < K; ++color) plain.trees[color] = fallback(color);
    auto start = chrono::steady_clock::now();
    auto plainDeadline = start + max(chrono::steady_clock::duration::zero(),
                                     refinementDeadline - start) / 3;
    for (int color = 0; color < K; ++color) {
        auto tree = build(color, emptySupport, emptyFlow, 0, plainDeadline);
        if (tree) plain.trees[color] = std::move(*tree);
        if (chrono::steady_clock::now() >= plainDeadline) break;
    }
    vector<RouteFamily> families;
    families.push_back(plain);
    families.push_back({"crossing", plain.trees});
    families.push_back({"shared", plain.trees});
    auto addSupport = [&](Support& support, Flow& flow, int color,
                          const RouteTree& tree, int sign) {
        vector<int> load = treeLoad(color, tree);
        for (int v = 0; v < cells; ++v) {
            if (tree.parent[v] < 0) continue; // No departures from a nest.
            double value = 0.25 + 0.75 * (tree.straight[v] - 1) / 7.0;
            support[v][tree.direction[v] / 2] += sign * value;
            flow[v][tree.direction[v]] += sign * load[v];
        }
    };
    array<Support, 2> totalSupport{emptySupport, emptySupport};
    array<Flow, 2> totalFlow{emptyFlow, emptyFlow};
    for (int style = 1; style <= 2; ++style) {
        for (int color = 0; color < K; ++color)
            addSupport(totalSupport[style - 1], totalFlow[style - 1], color,
                       families[style].trees[color], 1);
    }

    // Interleave the two refinements so neither can consume the entire budget.
    // A rebuild is committed only after every slime has a path to its nest.
    for (int pass = 0; pass < 3; ++pass) {
        for (int i = 0; i < K; ++i) {
            int color = pass % 2 == 0 ? i : K - 1 - i;
            for (int style = 1; style <= 2; ++style) {
                if (chrono::steady_clock::now() >= refinementDeadline) return families;
                Support foreign = totalSupport[style - 1];
                Flow foreignFlow = totalFlow[style - 1];
                auto& trees = families[style].trees;
                addSupport(foreign, foreignFlow, color, trees[color], -1);
                auto tree = build(color, foreign, foreignFlow, style, refinementDeadline);
                if (!tree) continue;
                addSupport(totalSupport[style - 1], totalFlow[style - 1], color, trees[color], -1);
                trees[color] = std::move(*tree);
                addSupport(totalSupport[style - 1], totalFlow[style - 1], color, trees[color], 1);
            }
        }
    }
    return families;
}

// Full-floor routes accept arbitrary intermediate positions when rebuilding a
// suffix. Every parent is exactly one BFS layer nearer the nest; old routing
// preferences break ties without importing old detours or creating cycles.
RouteFamily buildFullFloorFamily(const vector<string>& grid, int K,
                                const RouteFamily& base) {
    const int n = (int)grid.size(), cells = n * n;
    const int dr[4] = {-1, 1, 0, 0}, dc[4] = {0, 0, -1, 1};
    vector<array<int, 4>> neighbor(cells);
    vector<int> root(K, -1);
    for (int v = 0; v < cells; ++v) {
        neighbor[v].fill(-1);
        char ch = grid[v / n][v % n];
        if (ch == '#') continue;
        if ('A' <= ch && ch < 'A' + K) root[ch - 'A'] = v;
        for (int d = 0; d < 4; ++d) {
            int r = v / n + dr[d], c = v % n + dc[d];
            if (0 <= r && r < n && 0 <= c && c < n && grid[r][c] != '#')
                neighbor[v][d] = r * n + c;
        }
    }
    RouteFamily result{base.name + "_full", vector<RouteTree>(K)};
    for (int color = 0; color < K; ++color) {
        const RouteTree& old = base.trees[color];
        RouteTree& tree = result.trees[color];
        tree.parent.assign(cells, -1);
        tree.depth.assign(cells, -1);
        tree.direction.assign(cells, -1);
        tree.straight.assign(cells, 0);
        vector<int> order{root[color]}, oldOrder, load(cells);
        for (int v = 0; v < cells; ++v) {
            if (old.depth[v] >= 0) oldOrder.push_back(v);
            if (grid[v / n][v % n] == 'a' + color) ++load[v];
        }
        sort(oldOrder.begin(), oldOrder.end(), [&](int a, int b) { return old.depth[a] > old.depth[b]; });
        for (int v : oldOrder) if (old.parent[v] >= 0) load[old.parent[v]] += load[v];
        tree.depth[root[color]] = 0;
        for (size_t i = 0; i < order.size(); ++i) {
            int v = order[i];
            for (int p : neighbor[v]) {
                if (p < 0 || tree.depth[p] >= 0) continue;
                tree.depth[p] = tree.depth[v] + 1;
                order.push_back(p);
            }
        }
        for (int v : order) {
            if (v == root[color]) continue;
            tuple<int, int, int, int> best{-1, -1, -1, -1};
            for (int d = 0; d < 4; ++d) {
                int p = neighbor[v][d];
                if (p < 0 || tree.depth[p] + 1 != tree.depth[v]) continue;
                int run = tree.direction[p] == d ? min(8, tree.straight[p] + 1) : 1;
                auto value = make_tuple(old.parent[v] == p, old.direction[v] == d, load[p], run);
                if (value <= best) continue;
                best = value;
                tree.parent[v] = p;
                tree.direction[v] = d;
                tree.straight[v] = run;
            }
        }
    }
    return result;
}

// Interleave all colors so that a temporarily parked block can serve as a
// springboard for a different color. Mixed two-move actions may temporarily
// leave a color's tree, but must return all survivors to their own trees without
// increasing any payload's depth. Only completed candidates are returned.
struct ResumeConfig {
    const vector<Op>* solution = nullptr;
    int prefixLength = 0;
    int alternative = 1;
};

optional<vector<Op>> cooperativeSolve(
    const vector<string>& grid, int K,
    const vector<vector<int>>& parent,
    const vector<vector<int>>& depth, int operationLimit, int mode,
    chrono::steady_clock::time_point deadline, bool allowShortcuts = false,
    bool allowTransfers = false, const ResumeConfig* resume = nullptr) {
    const int n = (int)grid.size();
    const int cells = n * n;
    if (operationLimit <= 0) return nullopt;

    struct Stack {
        array<int, 8> color{};
        int size = 0;
    };
    vector<Stack> board(cells);
    vector<int> nest(cells, -1);
    int remaining = 0;
    for (int v = 0; v < cells; ++v) {
        char ch = grid[v / n][v % n];
        if ('a' <= ch && ch < 'a' + K) {
            board[v].color[board[v].size++] = ch - 'a';
            ++remaining;
        } else if ('A' <= ch && ch < 'A' + K) {
            nest[v] = ch - 'A';
        }
    }

    // Ancestors reachable in one straight jump, and a bottom-up tree order.
    vector<vector<array<int, 8>>> jump(K, vector<array<int, 8>>(cells));
    vector<vector<int>> run(K, vector<int>(cells));
    vector<vector<int>> order(K);
    for (int color = 0; color < K; ++color) {
        for (int v = 0; v < cells; ++v) {
            jump[color][v].fill(-1);
            if (depth[color][v] < 0) continue;
            order[color].push_back(v);
            int p = parent[color][v];
            if (p < 0) continue;
            int dr = p / n - v / n;
            int dc = p % n - v % n;
            int cur = v;
            for (int len = 1; len <= 8; ++len) {
                int next = parent[color][cur];
                if (next < 0 || next / n - cur / n != dr ||
                    next % n - cur % n != dc) break;
                jump[color][v][len - 1] = next;
                run[color][v] = len;
                cur = next;
            }
        }
        sort(order[color].begin(), order[color].end(),
             [&](int a, int b) { return depth[color][a] > depth[color][b]; });
    }

    // Physical rays are distinct from depth progress: crossing to a different
    // branch can save more tree edges than the number of traversed cells.
    constexpr int dr[4] = {-1, 1, 0, 0};
    constexpr int dc[4] = {0, 0, -1, 1};
    constexpr char directions[4] = {'U', 'D', 'L', 'R'};
    vector<array<array<int, 8>, 4>> ray(cells);
    for (int v = 0; v < cells && (allowShortcuts || allowTransfers || resume); ++v) {
        for (int d = 0; d < 4; ++d) {
            ray[v][d].fill(-1);
            if (grid[v / n][v % n] == '#') continue;
            for (int len = 1; len <= 8; ++len) {
                int r = v / n + dr[d] * len, c = v % n + dc[d] * len;
                if (r < 0 || r >= n || c < 0 || c >= n || grid[r][c] == '#') break;
                ray[v][d][len - 1] = r * n + c;
            }
        }
    }
    struct Forward {
        int target, length, direction, gain;
    };
    vector<vector<vector<Forward>>> forward(K, vector<vector<Forward>>(cells));
    vector<vector<array<int, 8>>> reachableGain(K, vector<array<int, 8>>(cells));
    for (int color = 0; color < K; ++color) {
        for (int v : order[color]) {
            if (!allowShortcuts) {
                int p = parent[color][v];
                if (p < 0) continue;
                int d = p / n < v / n ? 0 : p / n > v / n ? 1 : p % n < v % n ? 2 : 3;
                forward[color][v].reserve(run[color][v]);
                for (int len = 1; len <= run[color][v]; ++len)
                    forward[color][v].push_back({jump[color][v][len - 1], len, d, len});
                continue;
            }
            for (int d = 0; d < 4; ++d) {
                for (int len = 1; len <= 8; ++len) {
                    int target = ray[v][d][len - 1];
                    if (target < 0) break;
                    if (depth[color][target] < 0 || depth[color][target] >= depth[color][v]) continue;
                    int gain = depth[color][v] - depth[color][target];
                    forward[color][v].push_back({target, len, d, gain});
                    for (int kept = len - 1; kept < 8; ++kept)
                        reachableGain[color][v][kept] = max(reachableGain[color][v][kept], gain);
                }
            }
        }
        if (chrono::steady_clock::now() >= deadline) return nullopt;
    }

    // Variants are fixed before execution: they trade readiness against
    // preserving useful foreign-color springboards.
    static constexpr double supportWeights[8] =
        {0.45, 0.85, 1.35, 1.85, 0.65, 1.10, 1.60, 2.20};
    static constexpr double readinessWeights[8] =
        {7.0, 7.0, 7.0, 7.0, 10.0, 10.0, 10.0, 10.0};
    const int variant = ((mode % 8) + 8) % 8;
    // All variants retain the submitted mixed-batch policy.
    const bool useMixed = true;
    const double supportWeight = supportWeights[variant];
    const double readinessWeight = readinessWeights[variant];

    const double sharedWeight = 0.45;

    // Count consecutive common parent edges, including turns. Merely sharing
    // a cell or using the same corridor in opposite directions is not enough.
    vector<vector<int>> sharedPath;
    if (useMixed) {
        sharedPath.assign(K * K, vector<int>(cells));
        for (int a = 0; a < K; ++a) {
            for (int b = a + 1; b < K; ++b) {
                auto& length = sharedPath[a * K + b];
                auto& reverseLength = sharedPath[b * K + a];
                for (auto it = order[a].rbegin(); it != order[a].rend(); ++it) {
                    int v = *it, p = parent[a][v];
                    if (p >= 0 && parent[b][v] == p)
                        length[v] = reverseLength[v] = 1 + length[p];
                }
            }
        }
    }
    auto sharedPrefix = [&](int v, const Stack& stack, int count) {
        int reference = stack.color[stack.size - 1];
        int length = depth[reference][v];
        for (int j = stack.size - count; j < stack.size && length > 0; ++j) {
            int color = stack.color[j];
            if (color != reference)
                length = min(length, sharedPath[reference * K + color][v]);
        }
        return max(0, length);
    };
    auto sharedValue = [&](int v, const Stack& stack) {
        if (!useMixed || stack.size < 2) return 0.0;
        int reference = stack.color[stack.size - 1];
        int length = depth[reference][v], blocks = 0, previous = -1;
        double best = 0.0;
        for (int j = stack.size - 1; j >= 0 && length > 0; --j) {
            int color = stack.color[j];
            if (color != reference)
                length = min(length, sharedPath[reference * K + color][v]);
            if (color != previous) ++blocks;
            previous = color;
            // Estimate future departures saved by keeping a mixed top suffix
            // together. A short horizon limits speculative merge rewards.
            best = max(best, (double)(blocks - 1) * min(6, length));
        }
        return best;
    };
    auto batchSize = [&](int v, const Stack& stack) {
        if (stack.size == 0) return 0;
        int next = parent[stack.color[stack.size - 1]][v];
        if (next < 0) return 0;
        int count = 0;
        while (count < stack.size &&
               parent[stack.color[stack.size - 1 - count]][v] == next)
            ++count;
        return count;
    };
    auto batchCanMove = [&](int v, const Stack& stack, int count, const auto& read) {
        if (count == 0) return false;
        int color = stack.color[stack.size - 1];
        int maximum = min({run[color][v], sharedPrefix(v, stack, count),
                           stack.size - count + 1});
        for (int len = 1; len <= maximum; ++len) {
            int target = jump[color][v][len - 1];
            if (read(target).size + count <= 8) return true;
        }
        return false;
    };
    auto canJoin = [&](int v, const Stack& moving, int count, const Stack& resident) {
        if (resident.size == 0 || resident.size + count > 8) return false;
        int next = parent[resident.color[resident.size - 1]][v];
        if (next < 0) return false;
        for (int j = moving.size - count; j < moving.size; ++j)
            if (parent[moving.color[j]][v] != next) return false;
        return true;
    };

    vector<int> topColor(cells), topCount(cells);
    vector<int> batchCount(cells), waitTarget(cells), waitState(cells);
    vector<bool> wholeCanMove(cells), waiting(cells);
    vector<double> sharedPotential(cells);
    vector<vector<int>> local(K, vector<int>(cells));
    vector<vector<int>> pending(K, vector<int>(cells));
    vector<vector<int>> nearest(K, vector<int>(cells));
    vector<vector<int>> arrivalDistance(K, vector<int>(cells));
    vector<vector<int>> arrivalSize(K, vector<int>(cells));
    vector<vector<int>> nearestSize(K, vector<int>(cells));
    vector<vector<double>> traffic(K, vector<double>(cells));
    vector<array<double, 9>> support(cells);
    vector<Op> answer;
    answer.reserve(operationLimit);

    auto home = [&](int v) {
        if (nest[v] < 0) return;
        while (board[v].size > 0 &&
               board[v].color[board[v].size - 1] == nest[v]) {
            --board[v].size;
            --remaining;
        }
    };

    auto readBoard = [&](int v) -> const Stack& { return board[v]; };

    // Replay a chosen prefix exactly, including reversal and homing at both
    // endpoints. Reject survivors outside the supplied trees; with full-floor
    // trees, a prefix may also end between the two moves of a transfer.
    bool chooseAlternative = resume != nullptr;
    if (resume) {
        if (!resume->solution || resume->prefixLength < 0 ||
            resume->prefixLength >= (int)resume->solution->size() ||
            resume->prefixLength >= operationLimit ||
            resume->alternative < 1 || resume->alternative > 4) return nullopt;
        for (int i = 0; i < resume->prefixLength; ++i) {
            if ((i & 63) == 0 && chrono::steady_clock::now() >= deadline) return nullopt;
            const Op& op = (*resume->solution)[i];
            if (op.r < 0 || op.r >= n || op.c < 0 || op.c >= n ||
                op.len < 1 || op.len > 8) return nullopt;
            int d = -1;
            for (int j = 0; j < 4; ++j) if (op.dir == directions[j]) d = j;
            int source = op.r * n + op.c;
            if (d < 0 || ray[source][d][op.len - 1] < 0) return nullopt;
            int target = ray[source][d][op.len - 1];
            Stack& from = board[source];
            Stack& to = board[target];
            if (op.k < 0 || op.k >= from.size || op.len > op.k + 1 ||
                to.size + from.size - op.k > 8) return nullopt;
            for (int j = from.size - 1; j >= op.k; --j) to.color[to.size++] = from.color[j];
            from.size = op.k;
            home(source);
            home(target);
            answer.push_back(op);
        }
        for (int v = 0; v < cells; ++v)
            for (int j = 0; j < board[v].size; ++j)
                if (depth[board[v].color[j]][v] < 0) return nullopt;
    }

    unsigned candidateChecks = 0;
    auto deadlineReached = [&]() {
        return (++candidateChecks & 63u) == 0 && chrono::steady_clock::now() >= deadline;
    };

    while (remaining > 0) {
        if ((int)answer.size() >= operationLimit ||
            chrono::steady_clock::now() >= deadline) return nullopt;

        fill(topColor.begin(), topColor.end(), -1);
        fill(topCount.begin(), topCount.end(), 0);
        for (int color = 0; color < K; ++color) {
            fill(local[color].begin(), local[color].end(), 0);
            fill(nearest[color].begin(), nearest[color].end(), cells + 1);
            fill(nearestSize[color].begin(), nearestSize[color].end(), 1);
            fill(arrivalDistance[color].begin(), arrivalDistance[color].end(), cells + 1);
            fill(arrivalSize[color].begin(), arrivalSize[color].end(), 1);
            fill(traffic[color].begin(), traffic[color].end(), 0.0);
        }
        for (int v = 0; v < cells; ++v) {
            const Stack& stack = board[v];
            sharedPotential[v] = sharedValue(v, stack);
            for (int j = 0; j < stack.size; ++j) ++local[stack.color[j]][v];
            if (stack.size == 0) continue;
            int color = stack.color[stack.size - 1];
            topColor[v] = color;
            int count = 0;
            while (count < stack.size && stack.color[stack.size - 1 - count] == color)
                ++count;
            topCount[v] = count;
            nearest[color][v] = 0;
            nearestSize[color][v] = count;
        }

        // The leading batch waits for an exposed follower that can reach it
        // through empty cells, fit on its tower, and then share an outgoing edge.
        // Stop at the first occupied cell so blocked/hidden arrivals cannot
        // reserve a distant batch indefinitely.
        fill(waitTarget.begin(), waitTarget.end(), -1);
        fill(waitState.begin(), waitState.end(), 0);
        fill(waiting.begin(), waiting.end(), false);
        for (int source = 0; source < cells; ++source) {
            const Stack& stack = board[source];
            int count = batchCount[source] = batchSize(source, stack);
            wholeCanMove[source] = batchCanMove(source, stack, count, readBoard);
            if (count == 0) continue;
            int color = topColor[source], v = source;
            int length = sharedPrefix(source, stack, count);
            for (int step = 0; step < length; ++step) {
                v = parent[color][v];
                if (board[v].size == 0) continue;
                if (canJoin(v, stack, count, board[v])) waitTarget[source] = v;
                break;
            }
        }
        // Different colors can induce cyclic waits even though each individual
        // tree is acyclic. Remove one dependency per cycle before scheduling.
        auto breakWaitCycles = [&](auto&& self, int v) -> void {
            waitState[v] = 1;
            int next = waitTarget[v];
            if (next >= 0) {
                if (waitState[next] == 1) waitTarget[v] = -1;
                else if (waitState[next] == 0) self(self, next);
            }
            waitState[v] = 2;
        };
        for (int v = 0; v < cells; ++v)
            if (waitState[v] == 0) breakWaitCycles(breakWaitCycles, v);
        for (int v = 0; v < cells; ++v)
            if (waitTarget[v] >= 0) waiting[waitTarget[v]] = true;

        // Pending mass includes hidden blocks. Arrival distance uses exposed
        // blocks only, so a blocked distant color does not reserve a tower.
        for (int color = 0; color < K; ++color) {
            pending[color] = local[color];
            for (int v : order[color]) {
                int p = parent[color][v];
                if (p < 0) continue;
                pending[color][p] += pending[color][v];
                int distance = nearest[color][v] + 1;
                if (distance < arrivalDistance[color][p]) {
                    arrivalDistance[color][p] = distance;
                    arrivalSize[color][p] = nearestSize[color][v];
                }
                if (distance < nearest[color][p]) {
                    nearest[color][p] = distance;
                    nearestSize[color][p] = nearestSize[color][v];
                }
            }
            for (int v : order[color]) {
                int incoming = pending[color][v] - local[color][v];
                int distance = arrivalDistance[color][v];
                bool useful = allowShortcuts ?
                    reachableGain[color][v][7] > reachableGain[color][v][0] : run[color][v] >= 2;
                if (incoming == 0 || distance > cells || !useful) continue;
                double batches = min(2.0, (double)incoming / max(1, arrivalSize[color][v]));
                traffic[color][v] = batches / (1.0 + 0.5 * distance);
            }
        }

        auto colorSupport = [&](int color, int v, int height) {
            if (height <= 0 || height >= 8 || traffic[color][v] == 0.0) return 0.0;
            int saved = allowShortcuts ?
                reachableGain[color][v][height] - reachableGain[color][v][0] :
                min(height, run[color][v] - 1);
            double fit = min(1.0, (double)(8 - height) / arrivalSize[color][v]);
            return traffic[color][v] * saved * fit;
        };
        for (int v = 0; v < cells; ++v) {
            support[v].fill(0.0);
            for (int color = 0; color < K; ++color) {
                if (traffic[color][v] == 0.0) continue;
                for (int height = 1; height < 8; ++height)
                    support[v][height] += colorSupport(color, v, height);
            }
        }
        auto foreignSupport = [&](int color, int v, int height) {
            return support[v][height] - colorSupport(color, v, height);
        };

        struct Candidate {
            int source = -1, target = -1, count = 0, length = 0;
            double score = -1e100;
            int secondTarget = -1, secondCount = 0, secondLength = 0;
        } best;
        array<Candidate, 4> alternatives;
        int alternativeCount = 0;
        auto sameAction = [](const Candidate& a, const Candidate& b) {
            return a.source == b.source && a.target == b.target && a.count == b.count &&
                   a.length == b.length && a.secondTarget == b.secondTarget &&
                   a.secondCount == b.secondCount && a.secondLength == b.secondLength;
        };
        auto offer = [&](const Candidate& action) {
            int steps = action.secondTarget < 0 ? 1 : 2;
            if ((int)answer.size() + steps >= operationLimit) return;
            if (!chooseAlternative) {
                if (action.score > best.score) best = action;
                return;
            }
            const Op& old = (*resume->solution)[resume->prefixLength];
            int d = action.target / n < action.source / n ? 0 :
                    action.target / n > action.source / n ? 1 :
                    action.target % n < action.source % n ? 2 : 3;
            if (old.r == action.source / n && old.c == action.source % n &&
                old.k == board[action.source].size - action.count &&
                old.dir == directions[d] && old.len == action.length) return;
            for (int i = 0; i < alternativeCount; ++i)
                if (sameAction(action, alternatives[i])) return;
            int position = 0;
            while (position < alternativeCount && alternatives[position].score >= action.score)
                ++position;
            if (position == 4) return;
            for (int i = min(3, alternativeCount); i > position; --i)
                alternatives[i] = alternatives[i - 1];
            alternatives[position] = action;
            alternativeCount = min(4, alternativeCount + 1);
        };
        for (int source = 0; source < cells; ++source) {
            int color = topColor[source];
            if (color < 0 || forward[color][source].empty() || waiting[source]) continue;
            int height = board[source].size;
            int block = topCount[source];
            int bottom = height - block;
            int subtree = pending[color][source];
            int desired = min(8 - bottom, subtree);
            double originalSupport = foreignSupport(color, source, height);
            array<array<int, 9>, 4> skippedPrefix{};
            for (int d = 0; allowShortcuts && d < 4; ++d) {
                for (int distance = 1; distance <= 8; ++distance) {
                    int v = ray[source][d][distance - 1];
                    if (v < 0) break;
                    skippedPrefix[d][distance] = skippedPrefix[d][distance - 1] + local[color][v];
                }
            }

            for (int count = 1; count <= block; ++count) {
                if (count < batchCount[source] && wholeCanMove[source]) continue;
                int kept = height - count;
                int skippedSameColor = 0;
                for (const Forward& step : forward[color][source]) {
                    if (deadlineReached()) return nullopt;
                    int len = step.length, target = step.target;
                    if (len > kept + 1) {
                        if (!allowShortcuts) break;
                        continue;
                    }
                    int destinationHeight = board[target].size;
                    if (!allowShortcuts) {
                        // Keep the submitted single-pass ancestor scan on the
                        // baseline path, including its first merge stop.
                        if (len > 1) {
                            int skipped = jump[color][source][len - 2];
                            if (canJoin(skipped, board[source], count, board[skipped])) break;
                            skippedSameColor += local[color][skipped];
                        }
                    } else {
                        bool skipsJoin = false;
                        for (int distance = 1; distance < len; ++distance) {
                            int middle = ray[source][step.direction][distance - 1];
                            if (canJoin(middle, board[source], count, board[middle])) {
                                skipsJoin = true;
                                break;
                            }
                        }
                        if (skipsJoin) continue;
                        skippedSameColor = skippedPrefix[step.direction][len - 1];
                    }
                    if (destinationHeight + count > 8) continue;

                    int sameAtTarget = topColor[target] == color ? topCount[target] : 0;
                    double ready = min(1.0, (double)count / max(1, desired));
                    // Once a block is full, waiting for more descendants cannot
                    // save an additional batch on this departure.
                    double score = 1.0 + 0.55 * count + 1.65 * (step.gain - 1);
                    score -= readinessWeight * (1.0 - ready);
                    if (sameAtTarget > 0)
                        score += 1.25 * min(count, sameAtTarget);
                    score -= 0.8 * min(count, skippedSameColor);
                    if (count < block) score -= 0.85;
                    if (nest[target] == color) score += 0.20 * count;
                    if (count == block && bottom > 0) score += 0.15 * bottom;

                    // Evaluate support after automatic homing at both ends.
                    int sourceAfter = kept;
                    while (sourceAfter > 0 && nest[source] >= 0 &&
                           board[source].color[sourceAfter - 1] == nest[source])
                        --sourceAfter;
                    int targetAfter = destinationHeight + count;
                    if (nest[target] == color) {
                        targetAfter = destinationHeight;
                        while (targetAfter > 0 && board[target].color[targetAfter - 1] == color)
                            --targetAfter;
                    }
                    double supportChange = foreignSupport(color, source, sourceAfter) - originalSupport;
                    supportChange += foreignSupport(color, target, targetAfter) -
                                     foreignSupport(color, target, destinationHeight);
                    score += supportWeight * supportChange;

                    if (useMixed) {
                        Stack from = board[source], to = board[target];
                        from.size = sourceAfter;
                        for (int j = destinationHeight; j < targetAfter; ++j)
                            to.color[j] = color;
                        to.size = targetAfter;
                        // A monochrome arrival may form a useful mixed stack.
                        // Conversely, discourage peeling apart a shared route.
                        double change = sharedValue(source, from) - sharedPotential[source] +
                                        sharedValue(target, to) - sharedPotential[target];
                        score += sharedWeight * change;
                    }

                    // A stable, mode-dependent tie break explores different
                    // interleavings without random generators or runtime tuning.
                    unsigned tie = (unsigned)(source + 1) * 2654435761u;
                    tie ^= (unsigned)(color + 1 + variant * 17) * 2246822519u;
                    score += 0.00001 * (tie % 1024);
                    offer({source, target, count, len, score});
                }
            }
        }

        if (useMixed) {
            // A one- or two-move action touches at most three cells. Keep the
            // exact stack order here: after one flip the payload's bottom is
            // exposed; after two whole-payload flips its original top returns.
            struct LocalState {
                array<int, 3> cell{-1, -1, -1};
                array<Stack, 3> stack;
                int size = 0, homed = 0, splits = 0;
                double merged = 0.0, skipped = 0.0;
            };
            auto readStack = [&](const LocalState& state, int v) -> const Stack& {
                for (int i = 0; i < state.size; ++i)
                    if (state.cell[i] == v) return state.stack[i];
                return board[v];
            };
            auto editStack = [&](LocalState& state, int v) -> Stack& {
                for (int i = 0; i < state.size; ++i)
                    if (state.cell[i] == v) return state.stack[i];
                assert(state.size < 3);
                int i = state.size++;
                state.cell[i] = v;
                state.stack[i] = board[v];
                return state.stack[i];
            };
            auto topRun = [](const Stack& stack) {
                if (stack.size == 0) return 0;
                int count = 1;
                while (count < stack.size &&
                       stack.color[stack.size - 1 - count] == stack.color[stack.size - 1])
                    ++count;
                return count;
            };
            auto simulate = [&](LocalState& state, int source, int target, int count, int len) {
                if (source == target || target < 0 || target >= cells || len < 1 || len > 8)
                    return false;
                int dr = (target / n - source / n) / len;
                int dc = (target % n - source % n) / len;
                if (abs(dr) + abs(dc) != 1 || source / n + dr * len != target / n ||
                    source % n + dc * len != target % n) return false;
                for (int step = 1; step <= len; ++step) {
                    int r = source / n + dr * step, c = source % n + dc * step;
                    if (r < 0 || r >= n || c < 0 || c >= n || grid[r][c] == '#') return false;
                }
                Stack& from = editStack(state, source);
                Stack& to = editStack(state, target);
                int kept = from.size - count;
                if (count <= 0 || kept < 0 || len > kept + 1 || to.size + count > 8) return false;

                auto readCurrent = [&](int v) -> const Stack& { return readStack(state, v); };
                int together = batchSize(source, from);
                if (count < together && batchCanMove(source, from, together, readCurrent))
                    return false;

                array<int, 26> moving{}, skipped{};
                for (int j = kept; j < from.size; ++j) ++moving[from.color[j]];
                for (int step = 1; step < len; ++step) {
                    int v = (source / n + dr * step) * n + source % n + dc * step;
                    const Stack& middle = readStack(state, v);
                    if (canJoin(v, from, count, middle)) return false;
                    for (int j = 0; j < middle.size; ++j) ++skipped[middle.color[j]];
                }
                for (int color = 0; color < K; ++color)
                    state.skipped += min(moving[color], skipped[color]);
                if (to.size > 0 && to.color[to.size - 1] == from.color[from.size - 1])
                    state.merged += min({count, topRun(from), topRun(to)});
                if (kept > 0 && from.color[kept - 1] == from.color[kept]) ++state.splits;
                for (int j = from.size - 1; j >= kept; --j) to.color[to.size++] = from.color[j];
                from.size = kept;
                auto localHome = [&](int v, Stack& stack) {
                    while (stack.size > 0 && stack.color[stack.size - 1] == nest[v]) {
                        --stack.size;
                        ++state.homed;
                    }
                };
                localHome(source, from);
                localHome(target, to);
                return true;
            };
            auto exposure = [&](const LocalState& state, int v, const Stack& stack) {
                if (stack.size == 0) return 0.0;
                int color = stack.color[stack.size - 1], count = topRun(stack);
                if (!allowShortcuts) {
                    int maximum = min(run[color][v], stack.size - count + 1);
                    for (int len = maximum; len >= 1; --len) {
                        int target = jump[color][v][len - 1];
                        if (readStack(state, target).size + count <= 8)
                            return 0.5 * count + len - 1;
                    }
                    return 0.0;
                }
                int maximum = stack.size - count + 1, gain = 0;
                for (const Forward& step : forward[color][v])
                    if (step.length <= maximum && readStack(state, step.target).size + count <= 8)
                        gain = max(gain, step.gain);
                return gain > 0 ? 0.5 * count + gain - 1 : 0.0;
            };
            auto evaluate = [&](const LocalState& state, const Candidate& action) {
                int steps = action.secondTarget < 0 ? 1 : 2;
                if ((int)answer.size() + steps >= operationLimit) return;
                const Stack& original = board[action.source];
                int kept = original.size - action.count;
                array<int, 26> moved{}, gain{}, firstIndex;
                firstIndex.fill(8);
                unsigned movingMask = 0;
                for (int j = 0; j < original.size; ++j)
                    firstIndex[original.color[j]] = min(firstIndex[original.color[j]], j);
                for (int j = kept; j < original.size; ++j) {
                    ++moved[original.color[j]];
                    movingMask |= 1u << original.color[j];
                }
                double supportChange = 0.0, exposureChange = 0.0, sharedChange = 0.0;
                LocalState before;
                for (int i = 0; i < state.size; ++i) {
                    int v = state.cell[i];
                    const Stack& old = board[v];
                    const Stack& now = state.stack[i];
                    for (int j = 0; j < old.size; ++j) {
                        int color = old.color[j];
                        if (depth[color][v] < 0) return;
                        gain[color] += depth[color][v];
                    }
                    for (int j = 0; j < now.size; ++j) {
                        int color = now.color[j];
                        if (depth[color][v] < 0) return;
                        gain[color] -= depth[color][v];
                    }
                    double change = support[v][now.size] - support[v][old.size];
                    for (int color = 0; color < K; ++color) {
                        if (movingMask & (1u << color))
                            change -= colorSupport(color, v, now.size) - colorSupport(color, v, old.size);
                    }
                    supportChange += change;
                    exposureChange += exposure(state, v, now) - exposure(before, v, old);
                    sharedChange += sharedValue(v, now) - sharedPotential[v];
                }
                double score = 0.0;
                int totalGain = 0;
                for (int color = 0; color < K; ++color) {
                    if (gain[color] < 0) return;
                    totalGain += gain[color];
                    if (moved[color] == 0 || gain[color] == 0) continue;
                    int desired = min(8 - firstIndex[color], pending[color][action.source]);
                    double ready = min(1.0, (double)moved[color] / max(1, desired));
                    double distance = (double)gain[color] / moved[color];
                    score += 1.0 + 0.55 * moved[color] + 1.65 * (distance - 1.0);
                    score -= readinessWeight * (1.0 - ready);
                }
                if (totalGain <= 0) return;
                if (steps == 1 &&
                    jump[original.color[original.size - 1]][action.source][action.length - 1] == action.target &&
                    sharedPrefix(action.source, original, action.count) >= action.length) {
                    // On a common route the colors depart as one batch. Apply
                    // the readiness penalty once to the combined load, instead
                    // of making every color wait independently for its subtree.
                    int incoming = 0, colors = 0;
                    for (int color = 0; color < K; ++color) {
                        if (moved[color] == 0) continue;
                        incoming += pending[color][action.source];
                        ++colors;
                    }
                    int desired = min(8 - kept, incoming);
                    double ready = min(1.0, (double)action.count / max(1, desired));
                    double distance = (double)totalGain / action.count;
                    score = 1.0 + 0.55 * action.count + 1.65 * (distance - 1.0) +
                            0.85 * (colors - 1) - readinessWeight * (1.0 - ready);
                }
                score += 1.25 * state.merged - 0.85 * state.splits - 0.8 * state.skipped;
                score += 0.20 * state.homed + supportWeight * supportChange;
                score += 0.30 * exposureChange;
                score += sharedWeight * sharedChange;
                score /= steps; // Compare one- and two-move actions per operation.
                Candidate scored = action;
                scored.score = score;
                offer(scored);
            };

            for (int source = 0; source < cells; ++source) {
                if (chrono::steady_clock::now() >= deadline) return nullopt;
                const Stack& stack = board[source];
                int top = topColor[source], block = topCount[source];
                if (top < 0 || block == stack.size || forward[top][source].empty() ||
                    waiting[source]) continue;
                for (int count = block + 1; count <= stack.size; ++count) {
                    if (count < batchCount[source] && wholeCanMove[source]) continue;
                    int kept = stack.size - count;
                    int bottomColor = stack.color[kept], bottomCount = 1;
                    while (bottomCount < count && stack.color[kept + bottomCount] == bottomColor)
                        ++bottomCount;
                    for (const Forward& step : forward[top][source]) {
                        if (deadlineReached()) return nullopt;
                        int len = step.length, target = step.target;
                        if (len > kept + 1) continue;
                        if (board[target].size + count > 8) continue;
                        auto advances = [&](int color) {
                            return depth[color][target] >= 0 &&
                                   depth[color][target] < depth[color][source];
                        };
                        // Only the lowest homogeneous block may temporarily
                        // leave its tree: reversal exposes it for the next move.
                        bool upperAdvances = true;
                        for (int j = kept + bottomCount; j < stack.size; ++j)
                            if (!advances(stack.color[j])) upperAdvances = false;
                        if (!upperAdvances) continue;

                        Candidate action{source, target, count, len, -1e100};
                        LocalState first;
                        if (!simulate(first, source, target, count, len)) continue;
                        bool allAdvance = advances(bottomColor);
                        if (allAdvance) evaluate(first, action);
                        if ((int)answer.size() + 2 >= operationLimit) continue;
                        const Stack& landing = readStack(first, target);
                        int payload = max(0, landing.size - board[target].size);
                        if (payload == 0) continue;

                        if (allowTransfers) {
                            // Release the newly exposed block or carry the
                            // entire surviving payload. Search every physical
                            // direction, including right-angle transfers and
                            // recovery from a temporary off-tree landing.
                            int exposed = min(payload, topRun(landing));
                            array<int, 2> choices{exposed, payload};
                            for (int choice = 0; choice < 2; ++choice) {
                                if (choice == 1 && exposed == payload) continue;
                                int nextCount = choices[choice];
                                int maxNext = min(8, landing.size - nextCount + 1);
                                for (int d = 0; d < 4; ++d) {
                                    for (int nextLen = 1; nextLen <= maxNext; ++nextLen) {
                                        if (deadlineReached()) return nullopt;
                                        int end = ray[target][d][nextLen - 1];
                                        if (end < 0) break;
                                        bool returnsToTrees = true;
                                        for (int j = landing.size - nextCount; j < landing.size; ++j) {
                                            int color = landing.color[j];
                                            if (depth[color][end] < 0 ||
                                                depth[color][end] > depth[color][source])
                                                returnsToTrees = false;
                                        }
                                        if (!returnsToTrees) continue;
                                        LocalState second = first;
                                        if (!simulate(second, target, end, nextCount, nextLen)) continue;
                                        Candidate pair = action;
                                        pair.secondTarget = end;
                                        pair.secondCount = nextCount;
                                        pair.secondLength = nextLen;
                                        evaluate(second, pair);
                                    }
                                }
                            }
                        } else if (allAdvance) {
                            // Odd: release the newly exposed bottom-color block.
                            // Even: carry the entire surviving payload once more,
                            // restoring its original orientation at the next stop.
                            int exposed = min(payload, topRun(landing));
                            array<int, 2> choices{exposed, payload};
                            int color = landing.color[landing.size - 1];
                            for (int choice = 0; choice < 2; ++choice) {
                                if (choice == 1 && payload == exposed) continue;
                                int nextCount = choices[choice];
                                int maxNext = min(run[color][target], landing.size - nextCount + 1);
                                for (int nextLen = 1; nextLen <= maxNext; ++nextLen) {
                                    if (deadlineReached()) return nullopt;
                                    int end = jump[color][target][nextLen - 1];
                                    bool returnsToTrees = true;
                                    for (int j = landing.size - nextCount; j < landing.size; ++j) {
                                        int c = landing.color[j];
                                        if (depth[c][end] < 0 || depth[c][end] > depth[c][source])
                                            returnsToTrees = false;
                                    }
                                    if (!returnsToTrees) continue;
                                    LocalState second = first;
                                    if (!simulate(second, target, end, nextCount, nextLen)) continue;
                                    Candidate pair = action;
                                    pair.secondTarget = end;
                                    pair.secondCount = nextCount;
                                    pair.secondLength = nextLen;
                                    evaluate(second, pair);
                                }
                            }
                        } else {
                            // Carry the upper colors forward, flip the hidden
                            // bottom block to the top, then use those upper
                            // colors as a springboard to jump back past source.
                            int dr = (target / n - source / n) / len;
                            int dc = (target % n - source % n) / len;
                            int maxNext = landing.size - bottomCount + 1;
                            for (int nextLen = len; nextLen <= maxNext; ++nextLen) {
                                if (deadlineReached()) return nullopt;
                                int back = nextLen - len;
                                int end = source;
                                if (back > 0) {
                                    end = jump[bottomColor][source][back - 1];
                                    if (end < 0 || end / n != source / n - dr * back ||
                                        end % n != source % n - dc * back) continue;
                                }
                                LocalState second = first;
                                if (!simulate(second, target, end, bottomCount, nextLen)) continue;
                                Candidate pair = action;
                                pair.secondTarget = end;
                                pair.secondCount = bottomCount;
                                pair.secondLength = nextLen;
                                evaluate(second, pair);
                            }
                        }
                    }
                }
            }
        }
        if (chooseAlternative) {
            if (alternativeCount < resume->alternative) return nullopt;
            best = alternatives[resume->alternative - 1];
            chooseAlternative = false;
        }
        if (best.source < 0) return nullopt;

        auto execute = [&](int source, int target, int count, int length) {
            Stack& from = board[source];
            Stack& to = board[target];
            int kept = from.size - count;
            char direction;
            if (target / n < source / n) direction = 'U';
            else if (target / n > source / n) direction = 'D';
            else if (target % n < source % n) direction = 'L';
            else direction = 'R';
            answer.push_back({source / n, source % n, kept, direction, length});
            for (int j = from.size - 1; j >= kept; --j) to.color[to.size++] = from.color[j];
            from.size = kept;
            home(source);
            home(target);
        };
        execute(best.source, best.target, best.count, best.length);
        if (best.secondTarget >= 0)
            execute(best.target, best.secondTarget, best.secondCount, best.secondLength);
    }
    if ((int)answer.size() >= operationLimit ||
        chrono::steady_clock::now() >= deadline) return nullopt;
    return answer;
}

// Always keep a completed color-by-color solution while searching for
// cooperative schedules. Incomplete candidates never replace this fallback.
optional<vector<Op>> sequentialSolve(const vector<string>& C, int K,
                                     const vector<RouteTree>& routes) {
    const int N = (int)C.size();
    const int dr[4] = {-1, 1, 0, 0};
    const int dc[4] = {0, 0, -1, 1};
    const char DIR[4] = {'U', 'D', 'L', 'R'};

    // nest[color]
    vector<Pos> nest(K);

    // nestColor[r][c] = 巣の色。巣でなければ -1
    vector<vector<int>> nestColor(N, vector<int>(N, -1));

    // tower[r][c] : 下 -> 上
    vector<vector<vector<int>>> tower(
        N, vector<vector<int>>(N)
    );

    for (int r = 0; r < N; ++r) {
        for (int c = 0; c < N; ++c) {
            char ch = C[r][c];

            if ('A' <= ch && ch < 'A' + K) {
                int color = ch - 'A';
                nest[color] = {r, c};
                nestColor[r][c] = color;
            }

            if ('a' <= ch && ch < 'a' + K) {
                int color = ch - 'a';
                tower[r][c].push_back(color);
            }
        }
    }

    vector<Op> answer;
    bool stopped = false;

    // ------------------------------------------------------------
    // 帰巣処理
    // ------------------------------------------------------------
    auto home = [&](int r, int c) {
        int nc = nestColor[r][c];

        if (nc == -1) return;

        while (!tower[r][c].empty() &&
               tower[r][c].back() == nc) {
            tower[r][c].pop_back();
        }
    };

    // ------------------------------------------------------------
    // color が塔の上に何匹連続しているか
    // ------------------------------------------------------------
    auto topBlockSize = [&](int r, int c, int color) {
        int cnt = 0;

        for (int i = (int)tower[r][c].size() - 1;
             i >= 0;
             --i) {
            if (tower[r][c][i] != color) break;
            ++cnt;
        }

        return cnt;
    };

    // ------------------------------------------------------------
    // 1マス隣への操作を実行
    // 上から q 匹を移動
    // ------------------------------------------------------------
    auto moveBlock = [&](int r1, int c1,
                         int r2, int c2,
                         int q) {
        if (stopped) return false;

        if ((int)answer.size() >= MAX_OP) {
            stopped = true;
            return false;
        }

        int d = -1;

        for (int x = 0; x < 4; ++x) {
            if (r1 + dr[x] == r2 &&
                c1 + dc[x] == c2) {
                d = x;
                break;
            }
        }

        if (d == -1) {
            stopped = true;
            return false;
        }

        int h = (int)tower[r1][c1].size();
        int g = (int)tower[r2][c2].size();

        if (q <= 0 ||
            q > h ||
            g + q > 8) {
            stopped = true;
            return false;
        }

        int k = h - q;

        // len=1 なので 1 <= k+1 は必ず成立
        answer.push_back({
            r1, c1,
            k,
            DIR[d],
            1
        });

        vector<int> moving(
            tower[r1][c1].end() - q,
            tower[r1][c1].end()
        );

        tower[r1][c1].resize(h - q);

        // 宙返り
        reverse(moving.begin(), moving.end());

        for (int x : moving) {
            tower[r2][c2].push_back(x);
        }

        home(r1, c1);
        home(r2, c2);

        return true;
    };

    // ============================================================
    // 色ごとに処理
    // ============================================================

    if ((int)routes.size() != K) return nullopt;
    vector<vector<int>> parentTrees(K), treeDepths(K);
    for (int color = 0; color < K; ++color) {
        parentTrees[color] = routes[color].parent;
        treeDepths[color] = routes[color].depth;
    }
    for (int color = 0; color < K; ++color) {
        if (stopped) break;
        vector<vector<bool>> inTree(N, vector<bool>(N, false));
        vector<vector<Pos>> parent(N, vector<Pos>(N, {-1, -1}));
        vector<vector<int>> depth(N, vector<int>(N, -1));
        int rr = nest[color].r, rc = nest[color].c;
        for (int v = 0; v < N * N; ++v) {
            int r = v / N, c = v % N;
            depth[r][c] = treeDepths[color][v];
            inTree[r][c] = depth[r][c] >= 0;
            int p = parentTrees[color][v];
            if (p >= 0) parent[r][c] = {p / N, p % N};
        }

        // --------------------------------------------------------
        // 木のノードを深い順に処理する。
        // --------------------------------------------------------

        vector<Pos> nodes;

        for (int r = 0; r < N; ++r) {
            for (int c = 0; c < N; ++c) {
                if (inTree[r][c] &&
                    !(r == rr && c == rc)) {
                    nodes.push_back({r, c});
                }
            }
        }

        sort(nodes.begin(), nodes.end(),
             [&](const Pos &a, const Pos &b) {
                 return depth[a.r][a.c] >
                        depth[b.r][b.c];
             });

        /*
         * drain(r,c):
         *
         * この地点の上側にある color の一団を
         * 親方向へすべて流す。
         *
         * 親の塔が満杯に近い場合、
         * 親にある同色ブロックを先にさらに上流へ流す。
         *
         * これによって
         *
         * child1 ---\
         *            +---- parent ---- root
         * child2 ---/
         *
         * の parent で同色スライムが合流する。
         */

        function<void(int,int)> drain =
            [&](int r, int c) {

            if (stopped) return;

            // root から先には流さない
            if (r == rr && c == rc) {
                return;
            }

            Pos par = parent[r][c];

            if (par.r == -1) {
                stopped = true;
                return;
            }

            while (!stopped) {
                int srcBlock =
                    topBlockSize(r, c, color);

                if (srcBlock == 0) {
                    break;
                }

                int pr = par.r;
                int pc = par.c;

                int dstBlock =
                    topBlockSize(pr, pc, color);

                int dstHeight =
                    (int)tower[pr][pc].size();

                int capacity = 8 - dstHeight;

                /*
                 * srcBlock 全部が入り切らず、
                 * 親にも同色ブロックがすでにあるなら、
                 * 先に親を上流へ流してスペースを作る。
                 *
                 * 例:
                 *
                 *     source: 5匹
                 *     parent: 5匹
                 *
                 * のようなときに無理に分割せず、
                 * parent 側を先に進ませる。
                 */
                if (capacity < srcBlock &&
                    dstBlock > 0) {

                    if (pr == rr && pc == rc) {
                        /*
                         * 自色の巣なら本来 dstBlock は
                         * 帰巣済みなのでここには来ない。
                         */
                    } else {
                        drain(pr, pc);

                        if (stopped) return;
                        continue;
                    }
                }

                dstHeight =
                    (int)tower[pr][pc].size();

                capacity = 8 - dstHeight;

                if (capacity <= 0) {
                    /*
                     * 現在の構成では、他色だけで高さ8になる
                     * 状況は基本的に発生しない想定。
                     *
                     * 安全のため停止する。
                     */
                    stopped = true;
                    return;
                }

                srcBlock =
                    topBlockSize(r, c, color);

                int moveCount =
                    min(srcBlock, capacity);

                if (!moveBlock(
                        r, c,
                        pr, pc,
                        moveCount)) {
                    return;
                }
            }
        };

        // 深い順は維持し、同じ親を持つ同深さの枝だけ順序を変える。
        // 親の空きに収まる束を優先し、その中では大きい束を選ぶ。
        // drain によって親の高さが変わるので、毎回選び直す。
        vector<bool> processed(nodes.size(), false);
        for (int begin = 0; begin < (int)nodes.size() && !stopped;) {
            int end = begin + 1;
            while (end < (int)nodes.size() &&
                   depth[nodes[end].r][nodes[end].c] ==
                       depth[nodes[begin].r][nodes[begin].c]) {
                ++end;
            }

            int first = begin;
            while (!stopped) {
                while (first < end && processed[first]) ++first;
                if (first == end) break;

                const Pos anchor = nodes[first];
                if (topBlockSize(anchor.r, anchor.c, color) == 0) {
                    processed[first] = true;
                    continue;
                }

                const Pos par = parent[anchor.r][anchor.c];
                const int capacity = 8 - (int)tower[par.r][par.c].size();
                int selected = first;
                int bestFit = 0;
                for (int i = first; i < end; ++i) {
                    if (processed[i]) continue;
                    const Pos p = nodes[i];
                    const Pos candidateParent = parent[p.r][p.c];
                    if (candidateParent.r != par.r || candidateParent.c != par.c) {
                        continue;
                    }
                    const int block = topBlockSize(p.r, p.c, color);
                    if (block <= capacity && block > bestFit) {
                        bestFit = block;
                        selected = i;
                    }
                }

                const Pos p = nodes[selected];
                processed[selected] = true;
                drain(p.r, p.c);
            }
            begin = end;
        }
    }

    if (stopped) return nullopt;
    for (const auto& row : tower)
        for (const auto& stack : row)
            if (!stack.empty()) return nullopt;
    return compressBaselineMoves(C, K, answer);
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    const auto searchStart = chrono::steady_clock::now();

    int N, K;
    cin >> N >> K;
    vector<string> C(N);
    for (auto& row : C) cin >> row;

    const auto routingDeadline = searchStart + chrono::milliseconds(350);
    const auto constructionDeadline = searchStart + chrono::milliseconds(1650);
    const auto searchDeadline = searchStart + chrono::milliseconds(1730);
    const auto postprocessingDeadline = searchStart + chrono::milliseconds(1870);

    // Complete the submitted routing and all eight mixed scheduling variants
    // before spending any time on additional routes, shortcuts or transfers.
    auto routes = buildCrossingTrees(C, K, routingDeadline);
    if ((int)routes.size() != K) return 0;
    vector<RouteFamily> families;
    families.push_back({"submission4", std::move(routes)});
    vector<vector<vector<int>>> parents(1), depths(1);
    auto loadFamily = [&](int family) {
        parents[family].resize(K);
        depths[family].resize(K);
        for (int color = 0; color < K; ++color) {
            parents[family][color] = families[family].trees[color].parent;
            depths[family][color] = families[family].trees[color].depth;
        }
    };
    loadFamily(0);
    auto fallback = sequentialSolve(C, K, families[0].trees);
    if (!fallback) return 0;
    vector<Op> answer = std::move(*fallback);
    vector<Op> baselineAnswer;
    int baselineMode = -1, baselineAttempts = 0, baselineCompleted = 0;
    for (int mode = 0; mode < 8; ++mode) {
        if (chrono::steady_clock::now() >= constructionDeadline) break;
        // Match the submitted search bound independently of the fallback and
        // of later postprocessing, so those cannot prune a baseline variant.
        const int limit = baselineMode < 0 ? MAX_OP :
                          min(MAX_OP, 2 * (int)baselineAnswer.size() + 1);
        ++baselineAttempts;
        auto candidate = cooperativeSolve(C, K, parents[0], depths[0], limit,
                                           mode, constructionDeadline, false, false);
        if (!candidate) continue;
        ++baselineCompleted;
        auto compressed = compressBaselineMoves(C, K, *candidate);
        if (baselineMode < 0 || compressed.size() < baselineAnswer.size()) {
            baselineAnswer = std::move(compressed);
            baselineMode = mode;
        }
    }
    int bestFamily = 0, cooperativeMode = -1;
    bool bestShortcuts = false, bestTransfers = false;
    if (baselineMode >= 0 && baselineAnswer.size() <= answer.size()) {
        answer = std::move(baselineAnswer);
        cooperativeMode = baselineMode;
    }
    const size_t baselineOps = answer.size();

    // Additional geometries use only time left after the submitted search.
    // Their construction is capped, and they never replace a completed answer
    // until a shorter complete schedule has been found.
    auto now = chrono::steady_clock::now();
    if (now + chrono::milliseconds(100) < constructionDeadline) {
        auto extra = buildRouteFamilies(C, K, now + chrono::milliseconds(100));
        for (auto& family : extra) families.push_back(std::move(family));
    }
    const int sparseFamilyCount = (int)families.size();
    vector<int> fullFamily(sparseFamilyCount, -1), sparseOrigin(sparseFamilyCount);
    iota(sparseOrigin.begin(), sparseOrigin.end(), 0);
    for (int family = 0; family < sparseFamilyCount; ++family) {
        if (chrono::steady_clock::now() >= constructionDeadline) break;
        auto full = buildFullFloorFamily(C, K, families[family]);
        fullFamily[family] = (int)families.size();
        families.push_back(std::move(full));
        sparseOrigin.push_back(family);
    }
    const int familyCount = (int)families.size();
    parents.resize(familyCount);
    depths.resize(familyCount);
    for (int family = 1; family < familyCount; ++family) loadFamily(family);

    struct SearchJob {
        int family, mode;
        bool shortcuts, transfers;
    };
    vector<SearchJob> jobs{{0, 0, false, true}, {0, 0, true, false}, {0, 0, true, true}};
    for (int family = 1; family < sparseFamilyCount; ++family)
        jobs.push_back({family, 0, false, false});
    const int variants[] = {0, 2, 5, 1, 6, 3, 4, 7};
    for (int variant : variants) {
        for (int family = 0; family < familyCount; ++family) {
            if (family == 0 && variant == 0) continue;
            jobs.push_back({family, variant, true, true});
        }
    }
    int extraAttempts = 0, extraCompleted = 0, extraImprovements = 0;
    for (const auto& job : jobs) {
        now = chrono::steady_clock::now();
        if (now >= constructionDeadline) break;
        const auto deadline = min(constructionDeadline, now + chrono::milliseconds(160));
        const int limit = min(MAX_OP, 2 * (int)answer.size() + 1);
        ++extraAttempts;
        auto candidate = cooperativeSolve(C, K, parents[job.family], depths[job.family],
                                           limit, job.mode, deadline, job.shortcuts,
                                           job.transfers);
        if (!candidate) continue;
        ++extraCompleted;
        auto compressed = compressBaselineMoves(C, K, *candidate);
        compressed = compressMoves(C, K, compressed, constructionDeadline);
        if (compressed.size() < answer.size()) {
            answer = std::move(compressed);
            bestFamily = job.family;
            cooperativeMode = job.mode;
            bestShortcuts = job.shortcuts;
            bestTransfers = job.transfers;
            ++extraImprovements;
        }
    }

    // First rebuild on the incumbent's own routes to preserve its merge plan.
    // Alternate with full-floor routes only when such a family was constructed.
    const int cutEighths[] = {7, 6, 4, 7, 5, 2, 6, 3};
    const int rebuildModes[] = {0, 2, 5, 1, 6, 4, 3, 7};
    int rebuildAttempts = 0, rebuildCompleted = 0, rebuildImprovements = 0;
    for (int attempt = 0; attempt < 32 && !answer.empty(); ++attempt) {
        now = chrono::steady_clock::now();
        if (now >= searchDeadline) break;
        int full = fullFamily[sparseOrigin[bestFamily]];
        bool expand = attempt % 2 == 1 && full >= 0;
        int family = expand ? full : bestFamily;
        bool shortcuts = expand || bestShortcuts;
        bool transfers = expand || bestTransfers;
        int cut = (int)answer.size() * cutEighths[attempt % 8] / 8;
        if (attempt >= 8) {
            unsigned salt = (unsigned)(attempt + 1) * 2654435761u;
            cut += salt % max(1, (int)answer.size() / 8);
        }
        cut = min(cut, (int)answer.size() - 1);
        ResumeConfig resume;
        resume.solution = &answer;
        resume.prefixLength = cut;
        resume.alternative = 1 + (attempt / 8) % 4;
        const int mode = rebuildModes[attempt % 8];
        const auto deadline = min(searchDeadline, now + chrono::milliseconds(90));
        const int limit = min(MAX_OP, 2 * (int)answer.size() + 1);
        ++rebuildAttempts;
        auto candidate = cooperativeSolve(C, K, parents[family], depths[family],
                                           limit, mode, deadline, shortcuts, transfers, &resume);
        if (!candidate) continue;
        ++rebuildCompleted;
        auto compressed = compressBaselineMoves(C, K, *candidate);
        compressed = compressMoves(C, K, compressed, searchDeadline);
        if (compressed.size() < answer.size()) {
            answer = std::move(compressed);
            bestFamily = family;
            cooperativeMode = mode;
            bestShortcuts = shortcuts;
            bestTransfers = transfers;
            ++rebuildImprovements;
        }
    }

    const size_t beforePostprocessing = answer.size();
    answer = compressMoves(C, K, answer, postprocessingDeadline);
    answer = optimizeMoveWindows(C, K, answer, postprocessingDeadline);
    answer = compressMoves(C, K, answer, postprocessingDeadline);
    cerr << "baseline_mode=" << baselineMode
         << " baseline_attempts=" << baselineAttempts
         << " baseline_completed=" << baselineCompleted
         << " baseline_ops=" << baselineOps
         << " route_family=" << families[bestFamily].name
         << " cooperative_mode=" << cooperativeMode
         << " shortcuts=" << bestShortcuts
         << " transfers=" << bestTransfers
         << " extra_attempts=" << extraAttempts
         << " extra_completed=" << extraCompleted
         << " extra_improvements=" << extraImprovements
         << " rebuild_attempts=" << rebuildAttempts
         << " rebuild_completed=" << rebuildCompleted
         << " rebuild_improvements=" << rebuildImprovements
         << " postprocess_saved=" << beforePostprocessing - answer.size()
         << " final_ops=" << answer.size() << '\n';

    for (const auto& op : answer)
        cout << op.r << ' ' << op.c << ' ' << op.k << ' '
             << op.dir << ' ' << op.len << '\n';
    return 0;
}
