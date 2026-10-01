#include <bits/stdc++.h>
using namespace std;

struct Op {
    int r, c;
    int k;
    char dir;
    int len;
};

static const int MAX_OP = 100000;
static constexpr int MAX_COLOR = 12;
// Tower sizes bound readiness/fit denominators by eight. An incoming batch
// below twice that denominator needs a numerator of at most fifteen.
static constexpr auto SMALL_RATIO = [] {
    array<array<double, 9>, 17> result{};
    for (int numerator = 0; numerator <= 16; ++numerator)
        for (int denominator = 0; denominator <= 8; ++denominator)
            result[numerator][denominator] = (double)numerator / max(1, denominator);
    return result;
}();

// Only live colors participate in equality; unused slots retain old values.
struct SlimeStack {
    array<unsigned char, 8> color{};
    int size = 0;

    bool operator==(const SlimeStack& other) const {
        return size == other.size &&
               equal(color.begin(), color.begin() + size, other.color.begin());
    }
    int topCount() const {
        if (size == 0) return 0;
        int count = 1;
        while (count < size && color[size - 1 - count] == color[size - 1]) ++count;
        return count;
    }
    int home(int nestColor) {
        int before = size;
        while (size > 0 && color[size - 1] == nestColor) --size;
        return before - size;
    }
};

// Geometry is checked by the caller. Capacity must be checked before homing.
bool canTransfer(const SlimeStack& from, const SlimeStack& to, int kept, int length) {
    return &from != &to && kept >= 0 && kept < from.size &&
           length >= 1 && length <= kept + 1 && to.size + from.size - kept <= 8;
}

// Return the number homed, or -1 without changing either stack on failure.
int transferSlimes(SlimeStack& from, SlimeStack& to, int kept, int length,
                   int fromNest, int toNest) {
    if (!canTransfer(from, to, kept, length)) return -1;
    for (int j = from.size - 1; j >= kept; --j) to.color[to.size++] = from.color[j];
    from.size = kept;
    return from.home(fromNest) + to.home(toNest);
}

// Shared board initialization and geometry checks for explicit output moves.
// The cooperative scheduler uses precomputed rays for its geometry checks.
struct SlimeBoard {
    const vector<string>& grid;
    int n;
    vector<int> nest;
    vector<SlimeStack> initial;

    SlimeBoard(const vector<string>& grid_, int K)
        : grid(grid_), n((int)grid.size()), nest(n * n, -1), initial(n * n) {
        for (int v = 0; v < n * n; ++v) {
            char ch = grid[v / n][v % n];
            if ('a' <= ch && ch < 'a' + K) {
                initial[v].color[0] = ch - 'a';
                initial[v].size = 1;
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

    bool apply(vector<SlimeStack>& board, const Op& op) const {
        int from, to;
        return endpoints(op, from, to) &&
               transferSlimes(board[from], board[to], op.k, op.len, nest[from], nest[to]) >= 0;
    }
};

// Homogeneous blocks are unchanged by reversal. Replace only disjoint,
// consecutive pairs whose intermediate cell starts and ends empty.
vector<Op> compressBaselineMoves(const vector<string>& grid, int K,
                                 const vector<Op>& ops) {
    SlimeBoard sim(grid, K);
    auto board = sim.initial;
    vector<Op> result;
    result.reserve(ops.size());
    for (size_t i = 0; i < ops.size();) {
        const Op& first = ops[i];
        bool canMerge = false;
        if (i + 1 < ops.size() && first.len == 1 && first.k >= 1) {
            const Op& second = ops[i + 1];
            int from, middle, secondFrom, end;
            if (second.len == 1 && second.k == 0 && second.dir == first.dir &&
                sim.endpoints(first, from, middle) && sim.endpoints(second, secondFrom, end) &&
                secondFrom == middle && first.k < board[from].size && board[middle].size == 0 &&
                board[end].size + board[from].size - first.k <= 8) {
                int color = board[from].color[board[from].size - 1];
                // Homing at the intermediate cell would change the payload.
                canMerge = sim.nest[middle] != color;
                for (int j = first.k; j < board[from].size; ++j)
                    if (board[from].color[j] != color) canMerge = false;
            }
        }
        if (!sim.apply(board, first)) return ops;
        if (canMerge) {
            if (!sim.apply(board, ops[i + 1])) return ops;
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

// Collapse a straight chain, allowing independent operations between its
// links. Every skipped operation must avoid every endpoint of the chain.
vector<Op> compressMoves(
    const vector<string>& grid, int K, const vector<Op>& ops,
    chrono::steady_clock::time_point deadline = chrono::steady_clock::time_point::max()) {
    SlimeBoard sim(grid, K);
    auto board = sim.initial;
    // Scratch stacks are initialized only when an endpoint is first touched.
    vector<SlimeStack> original(board.size()), replacement(board.size());
    vector<unsigned> touchedAt(board.size(), 0);
    unsigned generation = 0;
    vector<int> touched;
    touched.reserve(48);
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
        const int firstTarget = current;
        int payload = board[from].size - first.k;
        int base = board[current].size;
        SlimeStack firstFrom = board[from], firstTo = board[current];
        if (transferSlimes(firstFrom, firstTo, first.k, first.len,
                           sim.nest[from], sim.nest[current]) < 0) return ops;
        if (first.len >= first.k + 1 || firstTo.size != base + payload) {
            board[from] = firstFrom;
            board[current] = firstTo;
            result.push_back(first);
            ++i;
            continue;
        }
        ++generation;
        touched.clear();
        auto touch = [&](int v) {
            if (touchedAt[v] == generation) return;
            touchedAt[v] = generation;
            touched.push_back(v);
            original[v] = board[v];
        };
        touch(from);
        touch(current);
        original[from] = firstFrom;
        original[current] = firstTo;
        bool palindrome = true;
        for (int a = first.k, b = board[from].size - 1; a < b; ++a, --b) {
            if (board[from].color[a] != board[from].color[b]) palindrome = false;
        }

        vector<int> chainCells{from, current}, skippedCells;
        vector<Op> skipped, bestReplacement;
        size_t bestEnd = i;
        int distance = first.len, links = 1;
        if (distance < first.k + 1 && original[current].size == base + payload) {
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
                    touch(a);
                    touch(b);
                    if (!sim.apply(original, next)) return ops;
                    skipped.push_back(next);
                    skippedCells.push_back(a);
                    skippedCells.push_back(b);
                    continue;
                }
                if (find(skippedCells.begin(), skippedCells.end(), b) != skippedCells.end()) break;
                touch(b);
                int nextBase = original[b].size;
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
                    for (int v : touched) replacement[v] = board[v];
                    bool valid = sim.apply(replacement, combined);
                    for (const Op& middle : skipped) {
                        if (!valid || !sim.apply(replacement, middle)) {
                            valid = false;
                            break;
                        }
                    }
                    // Both sequences leave every other cell unchanged.
                    if (valid) {
                        for (int v : touched) {
                            if (!(replacement[v] == original[v])) {
                                valid = false;
                                break;
                            }
                        }
                    }
                    if (valid) {
                        bestReplacement = {combined};
                        bestReplacement.insert(bestReplacement.end(), skipped.begin(), skipped.end());
                        bestEnd = j + 1;
                    }
                }
                if (distance == first.k + 1 || original[current].size != base + payload) break;
            }
        }
        if (bestEnd > i) {
            for (const Op& op : bestReplacement)
                if (!sim.apply(board, op)) return ops;
            result.insert(result.end(), bestReplacement.begin(), bestReplacement.end());
            i = bestEnd;
        } else {
            board[from] = firstFrom;
            board[firstTarget] = firstTo;
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
    SlimeBoard sim(grid, K);
    using Cell = SlimeStack;
    using State = array<Cell, 6>;
    struct Edge { int from, to; Op op; };
    struct Node { State state; int parent; Op op; int score; };
    auto board = sim.initial;
    vector<Cell> original(board.size()), verified(board.size());
    vector<Op> result;
    result.reserve(ops.size());
    int searched = 0;
    for (size_t i = 0; i < ops.size();) {
        if (chrono::steady_clock::now() >= deadline || searched >= 128 || i + 3 > ops.size()) {
            result.insert(result.end(), ops.begin() + i, ops.end());
            break;
        }
        vector<int> cells;
        cells.reserve(6);
        size_t end = i;
        for (; end < ops.size() && end < i + 4; ++end) {
            int from, to;
            if (!sim.endpoints(ops[end], from, to)) return ops;
            vector<int> nextCells = cells;
            for (int v : {from, to}) {
                if (find(nextCells.begin(), nextCells.end(), v) == nextCells.end()) nextCells.push_back(v);
            }
            if (nextCells.size() > 6) break;
            for (size_t c = cells.size(); c < nextCells.size(); ++c)
                original[nextCells[c]] = board[nextCells[c]];
            if (!sim.apply(original, ops[end])) return ops;
            cells = std::move(nextCells);
        }
        bool found = false;
        vector<Op> replacement;
        if (end >= i + 3) {
            ++searched;
            const int count = (int)cells.size();
            State start{}, target{};
            array<int, MAX_COLOR> targetCounts{};
            for (int c = 0; c < count; ++c) {
                start[c] = board[cells[c]];
                target[c] = original[cells[c]];
                for (int j = 0; j < target[c].size; ++j) ++targetCounts[target[c].color[j]];
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
                array<int, MAX_COLOR> counts{};
                for (int c = 0; c < count; ++c) {
                    score += 3 * abs(state[c].size - target[c].size);
                    int prefix = 0;
                    while (prefix < min(state[c].size, target[c].size) &&
                           state[c].color[prefix] == target[c].color[prefix]) ++prefix;
                    score += 2 * (min(state[c].size, target[c].size) - prefix);
                    for (int j = 0; j < state[c].size; ++j) ++counts[state[c].color[j]];
                }
                // Homed pieces never return, so losing a required color is
                // irreparable while the search is confined to these cells.
                for (int c = 0; c < K; ++c) {
                    if (counts[c] < targetCounts[c]) return INT_MAX;
                    score += counts[c] - targetCounts[c];
                }
                return score;
            };
            auto hashState = [&](const State& state) {
                uint64_t hash = 1469598103934665603ULL;
                for (int c = 0; c < count; ++c) {
                    hash = (hash ^ (uint64_t)(state[c].size + 1)) * 1099511628211ULL;
                    for (int j = 0; j < state[c].size; ++j) {
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
                        for (int k = edge.op.len - 1; k < state[edge.from].size; ++k) {
                            if (++attempted > 2048 || nodes.size() >= 768 ||
                                ((attempted & 31) == 0 && chrono::steady_clock::now() >= deadline)) {
                                stop = true;
                                break;
                            }
                            State child = state;
                            if (transferSlimes(child[edge.from], child[edge.to], k, edge.op.len,
                                    sim.nest[cells[edge.from]], sim.nest[cells[edge.to]]) < 0) continue;
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
                // Replay exact transitions on all touched cells; every other
                // cell is unchanged by both the window and its replacement.
                for (int v : cells) verified[v] = board[v];
                bool valid = replacement.size() < end - i;
                for (const Op& op : replacement) {
                    if (!valid || !sim.apply(verified, op)) {
                        valid = false;
                        break;
                    }
                }
                if (valid) {
                    for (int v : cells) {
                        if (!(verified[v] == original[v])) {
                            valid = false;
                            break;
                        }
                    }
                }
                if (valid) found = true;
            }
        }
        if (found) {
            result.insert(result.end(), replacement.begin(), replacement.end());
            for (int v : cells) board[v] = original[v];
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
    // fixed geometric rerouting, independent of history rewriting.
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

// Keep a completed color-by-color solution if cooperative construction or
// history repair cannot finish. Its scheduling is independent of the search.
optional<vector<Op>> sequentialSolve(const vector<string>& grid, int K,
                                     const vector<RouteTree>& routes) {
    if ((int)routes.size() != K) return nullopt;
    SlimeBoard sim(grid, K);
    auto tower = sim.initial;
    vector<int> root(K, -1);
    for (int v = 0; v < sim.n * sim.n; ++v)
        if (sim.nest[v] >= 0) root[sim.nest[v]] = v;
    vector<Op> answer;
    bool stopped = false;
    auto topBlockSize = [&](int v, int color) {
        const auto& stack = tower[v];
        return stack.size > 0 && stack.color[stack.size - 1] == color ? stack.topCount() : 0;
    };
    auto moveBlock = [&](int from, int to, int count) {
        if (stopped) return false;
        int dr = to / sim.n - from / sim.n, dc = to % sim.n - from % sim.n;
        if ((int)answer.size() >= MAX_OP || abs(dr) + abs(dc) != 1) {
            stopped = true;
            return false;
        }
        char dir = dr < 0 ? 'U' : dr > 0 ? 'D' : dc < 0 ? 'L' : 'R';
        Op op{from / sim.n, from % sim.n, tower[from].size - count, dir, 1};
        if (!sim.apply(tower, op)) {
            stopped = true;
            return false;
        }
        answer.push_back(op);
        return true;
    };
    for (int color = 0; color < K && !stopped; ++color) {
        const auto& parent = routes[color].parent;
        const auto& depth = routes[color].depth;
        vector<int> nodes;
        for (int v = 0; v < sim.n * sim.n; ++v)
            if (depth[v] >= 0 && v != root[color]) nodes.push_back(v);
        sort(nodes.begin(), nodes.end(), [&](int a, int b) { return depth[a] > depth[b]; });

        // Drain a same-color block toward the nest. If its parent is full,
        // first drain the parent's exposed block to make room for the merge.
        function<void(int)> drain = [&](int v) {
            if (stopped || v == root[color]) return;
            int p = parent[v];
            if (p < 0) { stopped = true; return; }
            while (!stopped) {
                int count = topBlockSize(v, color);
                if (count == 0) break;
                int capacity = 8 - tower[p].size;
                if (capacity < count && topBlockSize(p, color) > 0 && p != root[color]) {
                    drain(p);
                    continue;
                }
                if (capacity <= 0) { stopped = true; return; }
                if (!moveBlock(v, p, min(count, capacity))) return;
            }
        };

        // Keep depth order. Among siblings at the same depth, first choose
        // the largest block that currently fits in the parent's remaining space.
        vector<bool> processed(nodes.size(), false);
        for (int begin = 0; begin < (int)nodes.size() && !stopped;) {
            int end = begin + 1;
            while (end < (int)nodes.size() && depth[nodes[end]] == depth[nodes[begin]]) ++end;
            int first = begin;
            while (!stopped) {
                while (first < end && processed[first]) ++first;
                if (first == end) break;
                int anchor = nodes[first];
                if (topBlockSize(anchor, color) == 0) {
                    processed[first] = true;
                    continue;
                }
                int p = parent[anchor];
                int capacity = 8 - tower[p].size;
                int selected = first, bestFit = 0;
                for (int i = first; i < end; ++i) {
                    if (processed[i] || parent[nodes[i]] != p) continue;
                    int count = topBlockSize(nodes[i], color);
                    if (count <= capacity && count > bestFit) {
                        bestFit = count;
                        selected = i;
                    }
                }
                processed[selected] = true;
                drain(nodes[selected]);
            }
            begin = end;
        }
    }
    if (stopped) return nullopt;
    for (const auto& stack : tower)
        if (stack.size != 0) return nullopt;
    return compressBaselineMoves(grid, K, answer);
}

// Keep the cooperative evaluator: batches may wait for arrivals, preserve
// springboards and carry mixed colors along shared routes.
struct CooperativeScheduler {
    using Clock = chrono::steady_clock;
    using Stack = SlimeStack;
    struct State {
        vector<Stack> board;
        int remaining = 0;
        // Copies retain their identity. Each atomic action receives a unique
        // revision and records the cells changed from its predecessor.
        uint64_t revision = 0, previousRevision = UINT64_MAX;
        array<int, 3> changedCells{};
        int changedCount = 0;
    };
    struct Candidate {
        int source = -1, target = -1, count = 0, length = 0;
        double score = -1e100;
        int secondTarget = -1, secondCount = 0, secondLength = 0;
        int serial = INT_MAX;
        int steps() const { return secondTarget < 0 ? 1 : 2; }
    };
    struct Decision {
        Candidate greedy;
        vector<Candidate> alternatives, related, sameSource;
    };
    struct Forward { int target, length, direction, gain; };

    const vector<string>& grid;
    int n, K, cells;
    vector<vector<int>> parent, depth, order, run, sharedPath;
    vector<vector<array<int, 8>>> jump, reachableGain;
    vector<array<array<int, 8>, 4>> ray;
    vector<signed char> moveInfo;
    vector<int> cellRow, cellColumn;
    vector<vector<vector<Forward>>> treeForward, shortcutForward;
    vector<int> nest;
    State initial, state;
    vector<int> topColor, topCount, batchCount, waitTarget, waitState;
    vector<bool> wholeCanMove, waiting;
    vector<double> sharedPotential;
    vector<vector<int>> local, pending, nearest, arrivalDistance, arrivalSize, nearestSize;
    vector<vector<double>> traffic;
    vector<array<double, 9>> support;
    vector<vector<array<double, 9>>> supportByColor;
    vector<double> originalExposure;
    vector<Stack> cachedBoard;
    vector<int> batchPrefix;
    vector<unsigned char> originalOnTree;
    vector<unsigned> supportAt, exposureAt;
    vector<int> activeCells, dirtySupportCells;
    array<uint64_t, 7> occupied{};
    vector<vector<int>> activeSupport;
    bool boardCacheReady = false, cachedShortcuts = false;
    uint64_t cachedRevision = UINT64_MAX;
    mutable uint64_t nextRevision = 0;
    unsigned decisionGeneration = 0;

    CooperativeScheduler(const vector<string>& grid_, int K_, const vector<RouteTree>& routes)
        : grid(grid_), n((int)grid.size()), K(K_), cells(n * n),
          parent(K), depth(K), order(K), run(K, vector<int>(cells)),
          sharedPath(K * K, vector<int>(cells)),
          jump(K, vector<array<int, 8>>(cells)),
          reachableGain(K, vector<array<int, 8>>(cells)), ray(cells),
          moveInfo(cells * cells, -1), cellRow(cells), cellColumn(cells),
          treeForward(K, vector<vector<Forward>>(cells)),
          shortcutForward(K, vector<vector<Forward>>(cells)),
          topColor(cells, -1), topCount(cells), batchCount(cells), waitTarget(cells), waitState(cells),
          wholeCanMove(cells), waiting(cells), sharedPotential(cells),
          local(K, vector<int>(cells)), pending(local), nearest(local),
          arrivalDistance(local), arrivalSize(local), nearestSize(local),
          traffic(K, vector<double>(cells)), support(cells),
          supportByColor(K, vector<array<double, 9>>(cells)), originalExposure(cells),
          cachedBoard(cells), batchPrefix(cells), originalOnTree(cells),
          supportAt(cells), exposureAt(cells), activeSupport(K) {
        activeCells.reserve(cells);
        dirtySupportCells.reserve(cells);
        SlimeBoard board(grid, K);
        initial.board = std::move(board.initial);
        nest = std::move(board.nest);
        for (const auto& stack : initial.board) initial.remaining += stack.size;
        constexpr int dr[4] = {-1, 1, 0, 0}, dc[4] = {0, 0, -1, 1};
        for (int v = 0; v < cells; ++v) {
            cellRow[v] = v / n;
            cellColumn[v] = v % n;
            for (int d = 0; d < 4; ++d) {
                ray[v][d].fill(-1);
                if (grid[v / n][v % n] == '#') continue;
                for (int len = 1; len <= 8; ++len) {
                    int r = v / n + dr[d] * len, c = v % n + dc[d] * len;
                    if (r < 0 || r >= n || c < 0 || c >= n || grid[r][c] == '#') break;
                    ray[v][d][len - 1] = r * n + c;
                    moveInfo[v * cells + r * n + c] = d * 8 + len - 1;
                }
            }
        }
        for (int color = 0; color < K; ++color) {
            parent[color] = routes[color].parent;
            depth[color] = routes[color].depth;
            for (int v = 0; v < cells; ++v) {
                jump[color][v].fill(-1);
                if (depth[color][v] < 0) continue;
                order[color].push_back(v);
                int p = parent[color][v];
                if (p >= 0) {
                    int d = p / n < v / n ? 0 : p / n > v / n ? 1 : p % n < v % n ? 2 : 3;
                    int at = v;
                    for (int len = 1; len <= 8; ++len) {
                        int next = parent[color][at];
                        if (next < 0 || next / n - at / n != dr[d] ||
                            next % n - at % n != dc[d]) break;
                        jump[color][v][len - 1] = next;
                        run[color][v] = len;
                        treeForward[color][v].push_back({next, len, d, len});
                        at = next;
                    }
                }
                for (int d = 0; d < 4; ++d) {
                    for (int len = 1; len <= 8; ++len) {
                        int target = ray[v][d][len - 1];
                        if (target < 0) break;
                        if (depth[color][target] < 0 || depth[color][target] >= depth[color][v]) continue;
                        int gain = depth[color][v] - depth[color][target];
                        shortcutForward[color][v].push_back({target, len, d, gain});
                        for (int kept = len - 1; kept < 8; ++kept)
                            reachableGain[color][v][kept] = max(reachableGain[color][v][kept], gain);
                    }
                }
            }
            sort(order[color].begin(), order[color].end(),
                 [&](int a, int b) { return depth[color][a] > depth[color][b]; });
            activeSupport[color].reserve(order[color].size());
        }
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
        state = initial;
    }

    static bool better(const Candidate& a, const Candidate& b) {
        if (a.source < 0) return false;
        if (b.source < 0) return true;
        return a.score != b.score ? a.score > b.score : a.serial < b.serial;
    }
    static bool sameAction(const Candidate& a, const Candidate& b) {
        return a.source == b.source && a.target == b.target && a.count == b.count &&
               a.length == b.length && a.secondTarget == b.secondTarget &&
               a.secondCount == b.secondCount && a.secondLength == b.secondLength;
    }
    optional<Decision> decide(int mode, bool allowShortcuts, bool allowTransfers,
                              Clock::time_point deadline, int alternativeLimit = 0,
                              const Candidate* focus = nullptr) {
        auto& board = state.board;
        if (Clock::now() >= deadline) return nullopt;
        if (++decisionGeneration == 0) {
            fill(supportAt.begin(), supportAt.end(), 0);
            fill(exposureAt.begin(), exposureAt.end(), 0);
            ++decisionGeneration;
        }
        // These are scheduling preferences, not legality constraints. Relax
        // them at the edited action only; suffix repair keeps the full policy.
        const bool explore = alternativeLimit > 0;
        const auto& forward = allowShortcuts ? shortcutForward : treeForward;
        unsigned candidateChecks = 0;
        auto deadlineReached = [&]() {
            return (++candidateChecks & 63u) == 0 && Clock::now() >= deadline;
        };
        // Variants are fixed before execution: they trade readiness against
        // preserving useful foreign-color springboards.
        static constexpr double supportWeights[8] =
            {0.45, 0.85, 1.35, 1.85, 0.65, 1.10, 1.60, 2.20};
        static constexpr double readinessWeights[8] =
            {7.0, 7.0, 7.0, 7.0, 10.0, 10.0, 10.0, 10.0};
        const int variant = ((mode % 8) + 8) % 8;
        const double supportWeight = supportWeights[variant];
        const double readinessWeight = readinessWeights[variant];

        const double sharedWeight = 0.45;

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
            if (stack.size < 2) return 0.0;
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

        // Consecutive repair steps expose their two or three changed cells.
        // Restored histories fall back to comparing the full cached board.
        const bool refreshSupport = !boardCacheReady || cachedShortcuts != allowShortcuts;
        unsigned dirtyColors = boardCacheReady ? 0u : (1u << K) - 1;
        auto updateCell = [&](int v) {
            const Stack& stack = board[v];
            if (boardCacheReady && stack == cachedBoard[v]) return;
            const Stack& old = cachedBoard[v];
            array<int, MAX_COLOR> difference{};
            unsigned presentColors = 0;
            for (int j = 0; j < old.size; ++j) {
                int color = old.color[j];
                --local[color][v];
                --difference[color];
                presentColors |= 1u << color;
            }
            originalOnTree[v] = true;
            for (int j = 0; j < stack.size; ++j) {
                int color = stack.color[j];
                ++local[color][v];
                ++difference[color];
                presentColors |= 1u << color;
                if (depth[color][v] < 0) originalOnTree[v] = false;
            }
            for (unsigned mask = presentColors; mask; mask &= mask - 1) {
                int color = __builtin_ctz(mask);
                if (difference[color] != 0) dirtyColors |= 1u << color;
            }
            sharedPotential[v] = sharedValue(v, stack);
            int color = stack.size > 0 ? stack.color[stack.size - 1] : -1;
            int count = stack.topCount();
            if (topColor[v] != color || topCount[v] != count) {
                if (topColor[v] >= 0) dirtyColors |= 1u << topColor[v];
                if (color >= 0) dirtyColors |= 1u << color;
            }
            topColor[v] = color;
            topCount[v] = count;
            uint64_t bit = uint64_t(1) << (v & 63);
            if (stack.size > 0) occupied[v >> 6] |= bit;
            else occupied[v >> 6] &= ~bit;
            batchCount[v] = batchSize(v, stack);
            batchPrefix[v] = batchCount[v] > 0 ? sharedPrefix(v, stack, batchCount[v]) : 0;
            cachedBoard[v] = stack;
        };
        if (!boardCacheReady || (state.revision != cachedRevision &&
                                state.previousRevision != cachedRevision)) {
            for (int v = 0; v < cells; ++v) updateCell(v);
        } else if (state.revision != cachedRevision) {
            for (int i = 0; i < state.changedCount; ++i) updateCell(state.changedCells[i]);
        }
        boardCacheReady = true;
        cachedShortcuts = allowShortcuts;
        cachedRevision = state.revision;
        // Ascending cell order preserves candidate serials and cycle choices.
        activeCells.clear();
        for (int word = 0; word < (int)occupied.size(); ++word) {
            for (uint64_t mask = occupied[word]; mask; mask &= mask - 1)
                activeCells.push_back(word * 64 + __builtin_ctzll(mask));
        }

        // The leading batch waits for an exposed follower that can reach it
        // through empty cells, fit on its tower, and then share an outgoing edge.
        // Stop at the first occupied cell so blocked/hidden arrivals cannot
        // reserve a distant batch indefinitely.
        // Every wait edge connects two occupied cells. Empty cells cannot
        // participate in this graph or become candidate sources.
        for (int v : activeCells) {
            waitTarget[v] = -1;
            waitState[v] = 0;
            waiting[v] = false;
        }
        for (int source : activeCells) {
            const Stack& stack = board[source];
            int count = batchCount[source];
            wholeCanMove[source] = false;
            if (count == 0) continue;
            int color = topColor[source], v = source;
            int length = batchPrefix[source];
            int maximum = min({run[color][source], length, stack.size - count + 1});
            for (int len = 1; len <= maximum; ++len) {
                int target = jump[color][source][len - 1];
                if (board[target].size + count <= 8) {
                    wholeCanMove[source] = true;
                    break;
                }
            }
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
        for (int v : activeCells)
            if (waitState[v] == 0) breakWaitCycles(breakWaitCycles, v);
        for (int v : activeCells)
            if (waitTarget[v] >= 0) waiting[waitTarget[v]] = true;

        // Pending mass includes hidden blocks. Arrival distance uses exposed
        // blocks only, so a blocked distant color does not reserve a tower.
        dirtySupportCells.clear();
        auto markSupportDirty = [&](int v) {
            if (supportAt[v] == decisionGeneration) return;
            supportAt[v] = decisionGeneration;
            dirtySupportCells.push_back(v);
        };
        if (refreshSupport)
            for (int v = 0; v < cells; ++v) markSupportDirty(v);
        for (int color = 0; color < K; ++color) {
            bool dirty = (dirtyColors & (1u << color)) != 0;
            if (!dirty && !refreshSupport) continue;
            if (dirty) {
                fill(nearest[color].begin(), nearest[color].end(), cells + 1);
                fill(nearestSize[color].begin(), nearestSize[color].end(), 1);
                fill(arrivalDistance[color].begin(), arrivalDistance[color].end(), cells + 1);
                fill(arrivalSize[color].begin(), arrivalSize[color].end(), 1);
                for (int v : activeCells) {
                    if (topColor[v] != color) continue;
                    nearest[color][v] = 0;
                    nearestSize[color][v] = topCount[v];
                }
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
            }
            for (int v : activeSupport[color]) {
                traffic[color][v] = 0.0;
                markSupportDirty(v);
            }
            activeSupport[color].clear();
            for (int v : order[color]) {
                int incoming = pending[color][v] - local[color][v];
                int distance = arrivalDistance[color][v];
                bool useful = allowShortcuts ?
                    reachableGain[color][v][7] > reachableGain[color][v][0] : run[color][v] >= 2;
                if (incoming == 0 || distance > cells || !useful) continue;
                int arrival = arrivalSize[color][v];
                double batches = incoming >= 2 * arrival ? 2.0 : SMALL_RATIO[incoming][arrival];
                traffic[color][v] = batches / (1.0 + 0.5 * distance);
                activeSupport[color].push_back(v);
                markSupportDirty(v);
                auto& values = supportByColor[color][v];
                values[0] = values[8] = 0.0;
                for (int height = 1; height < 8; ++height) {
                    int saved = allowShortcuts ?
                        reachableGain[color][v][height] - reachableGain[color][v][0] :
                        min(height, run[color][v] - 1);
                    double fit = min(1.0, SMALL_RATIO[8 - height][arrival]);
                    values[height] = traffic[color][v] * saved * fit;
                }
            }
        }

        // Re-sum affected cells in color order instead of subtracting old
        // contributions, preserving the original floating-point accumulation.
        for (int v : dirtySupportCells) {
            support[v].fill(0.0);
            for (int color = 0; color < K; ++color) {
                if (traffic[color][v] == 0.0) continue;
                const auto& values = supportByColor[color][v];
                for (int height = 1; height < 8; ++height)
                    support[v][height] += values[height];
            }
        }
        auto colorSupport = [&](int color, int v, int height) {
            return traffic[color][v] == 0.0 ? 0.0 : supportByColor[color][v][height];
        };
        auto foreignSupport = [&](int color, int v, int height) {
            return support[v][height] - colorSupport(color, v, height);
        };

        Decision decision;
        if (explore) {
            decision.alternatives.reserve(alternativeLimit + 1);
            decision.related.reserve(alternativeLimit + 1);
            decision.sameSource.reserve(9);
        }
        int serial = 0;
        auto collect = [&](vector<Candidate>& choices, const Candidate& tagged,
                           int limit, int perSource) {
            for (const Candidate& old : choices)
                if (sameAction(old, tagged)) return;
            int sourceCount = 0, weakestSame = -1;
            for (int i = 0; i < (int)choices.size(); ++i) {
                if (choices[i].source != tagged.source) continue;
                ++sourceCount;
                if (weakestSame < 0 || better(choices[weakestSame], choices[i]))
                    weakestSame = i;
            }
            if (sourceCount >= perSource) {
                if (!better(tagged, choices[weakestSame])) return;
                choices.erase(choices.begin() + weakestSame);
            }
            if ((int)choices.size() >= limit && !better(tagged, choices.back())) return;
            auto position = choices.begin();
            while (position != choices.end() && better(*position, tagged)) ++position;
            choices.insert(position, tagged);
            if ((int)choices.size() > limit) choices.pop_back();
        };
        auto offer = [&](const Candidate& action) {
            Candidate tagged = action;
            tagged.serial = serial++;
            if (better(tagged, decision.greedy)) decision.greedy = tagged;
            if (!explore) return;
            collect(decision.alternatives, tagged, alternativeLimit, 2);
            if (!focus) return;
            if (tagged.source == focus->source)
                collect(decision.sameSource, tagged, 8, 8);
            bool interacts = false;
            for (int a : {tagged.source, tagged.target, tagged.secondTarget})
                for (int b : {focus->source, focus->target, focus->secondTarget})
                    if (a >= 0 && a == b) interacts = true;
            if (interacts) collect(decision.related, tagged, alternativeLimit, 3);
        };
        auto canJoinMonochrome = [&](int v, int color, int count) {
            if (topColor[v] < 0 || board[v].size + count > 8) return false;
            int next = parent[topColor[v]][v];
            return next >= 0 && parent[color][v] == next;
        };
        for (int source : activeCells) {
            int color = topColor[source];
            if (color < 0 || forward[color][source].empty() || (!explore && waiting[source])) continue;
            int height = board[source].size;
            int block = topCount[source];
            int bottom = height - block;
            int subtree = pending[color][source];
            int desired = min(8 - bottom, subtree);
            double originalSupport = foreignSupport(color, source, height);
            unsigned tie = (unsigned)(source + 1) * 2654435761u;
            tie ^= (unsigned)(color + 1 + variant * 17) * 2246822519u;
            const double tieScore = 0.00001 * (tie % 1024);
            array<array<int, 9>, 4> skippedPrefix{};
            for (int d = 0; allowShortcuts && d < 4; ++d) {
                for (int distance = 1; distance <= 8; ++distance) {
                    int v = ray[source][d][distance - 1];
                    if (v < 0) break;
                    skippedPrefix[d][distance] = skippedPrefix[d][distance - 1] + local[color][v];
                }
            }

            for (int count = 1; count <= block; ++count) {
                if (!explore && count < batchCount[source] && wholeCanMove[source]) continue;
                int kept = height - count;
                Stack remaining = board[source];
                remaining.size = kept;
                remaining.home(nest[source]);
                const double sourceSupportChange =
                    foreignSupport(color, source, remaining.size) - originalSupport;
                const double sourceSharedChange =
                    sharedValue(source, remaining) - sharedPotential[source];
                array<int, 4> firstJoin;
                firstJoin.fill(9);
                if (allowShortcuts && !explore) {
                    for (int d = 0; d < 4; ++d) {
                        for (int distance = 1; distance <= kept; ++distance) {
                            int v = ray[source][d][distance - 1];
                            if (v < 0) break;
                            if (canJoinMonochrome(v, color, count)) {
                                firstJoin[d] = distance;
                                break;
                            }
                        }
                    }
                }
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
                            if (!explore && canJoinMonochrome(skipped, color, count)) break;
                            skippedSameColor += local[color][skipped];
                        }
                    } else {
                        if (!explore && firstJoin[step.direction] < len) continue;
                        skippedSameColor = skippedPrefix[step.direction][len - 1];
                    }
                    if (destinationHeight + count > 8) continue;

                    int sameAtTarget = topColor[target] == color ? topCount[target] : 0;
                    double ready = min(1.0, SMALL_RATIO[count][desired]);
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

                    // This suffix is monochrome, so reversal changes no colors.
                    // Geometry, kept height and destination capacity were
                    // already checked; the source result is cached above.
                    Stack to = board[target];
                    for (int j = 0; j < count; ++j) to.color[to.size++] = color;
                    to.home(nest[target]);
                    double supportChange = sourceSupportChange;
                    supportChange += foreignSupport(color, target, to.size) -
                                     foreignSupport(color, target, destinationHeight);
                    score += supportWeight * supportChange;

                    // A monochrome arrival may form a useful mixed stack.
                    // Conversely, discourage peeling apart a shared route.
                    double change = sourceSharedChange +
                                    sharedValue(target, to) - sharedPotential[target];
                    score += sharedWeight * change;

                    // A stable, mode-dependent tie break explores different
                    // interleavings without random generators or runtime tuning.
                    score += tieScore;
                    offer({source, target, count, len, score});
                }
            }
        }

        // A one- or two-move action touches at most three cells. Keep the
        // exact stack order here: after one flip the payload's bottom is
        // exposed; after two whole-payload flips its original top returns.
        struct LocalState {
            array<int, 3> cell{-1, -1, -1};
            array<Stack, 3> stack;
            array<int, MAX_COLOR> gain{};
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
        auto simulate = [&](LocalState& state, int source, int target, int count, int len) {
            int direction = moveDirection(source, target, len);
            if (direction < 0) return false;
            Stack& from = editStack(state, source);
            Stack& to = editStack(state, target);
            int kept = from.size - count;
            if (!canTransfer(from, to, kept, len)) return false;

            auto readCurrent = [&](int v) -> const Stack& { return readStack(state, v); };
            int together = batchSize(source, from);
            if (!explore && count < together && batchCanMove(source, from, together, readCurrent))
                return false;

            array<int, MAX_COLOR> moving{}, skipped{};
            unsigned movingColors = 0;
            for (int j = kept; j < from.size; ++j) {
                ++moving[from.color[j]];
                movingColors |= 1u << from.color[j];
            }
            for (int step = 1; step < len; ++step) {
                int v = ray[source][direction][step - 1];
                const Stack& middle = readStack(state, v);
                if (!explore && canJoin(v, from, count, middle)) return false;
                for (int j = 0; j < middle.size; ++j) ++skipped[middle.color[j]];
            }
            for (unsigned mask = movingColors; mask; mask &= mask - 1) {
                int color = __builtin_ctz(mask);
                state.skipped += min(moving[color], skipped[color]);
                // Transfers change only the moving pieces' depths. Homed
                // pieces are removed at their own nest, whose depth is zero.
                // A temporary off-tree depth cancels across the second move.
                state.gain[color] += moving[color] *
                    (depth[color][source] - depth[color][target]);
            }
            if (to.size > 0 && to.color[to.size - 1] == from.color[from.size - 1])
                state.merged += min({count, from.topCount(), to.topCount()});
            if (kept > 0 && from.color[kept - 1] == from.color[kept]) ++state.splits;
            int homed = transferSlimes(from, to, kept, len, nest[source], nest[target]);
            if (homed < 0) return false;
            state.homed += homed;
            return true;
        };
        auto exposure = [&](const LocalState& state, int v, const Stack& stack) {
            if (stack.size == 0) return 0.0;
            int color = stack.color[stack.size - 1], count = stack.topCount();
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
        const LocalState before;
        auto evaluate = [&](const LocalState& state, const Candidate& action,
                            const array<int, MAX_COLOR>& moved, unsigned movingMask,
                            const array<int, MAX_COLOR>& firstIndex) {
            int steps = action.secondTarget < 0 ? 1 : 2;
            const Stack& original = board[action.source];
            int kept = original.size - action.count;
            const auto& gain = state.gain;
            double supportChange = 0.0, exposureChange = 0.0, sharedChange = 0.0;
            for (int i = 0; i < state.size; ++i) {
                int v = state.cell[i];
                const Stack& old = board[v];
                const Stack& now = state.stack[i];
                if (!originalOnTree[v]) return;
                for (int j = 0; j < now.size; ++j) {
                    int color = now.color[j];
                    if (depth[color][v] < 0) return;
                }
                double change = support[v][now.size] - support[v][old.size];
                for (unsigned mask = movingMask; mask; mask &= mask - 1) {
                    int color = __builtin_ctz(mask);
                    change -= colorSupport(color, v, now.size) - colorSupport(color, v, old.size);
                }
                supportChange += change;
                if (exposureAt[v] != decisionGeneration) {
                    exposureAt[v] = decisionGeneration;
                    originalExposure[v] = exposure(before, v, old);
                }
                exposureChange += exposure(state, v, now) - originalExposure[v];
                sharedChange += sharedValue(v, now) - sharedPotential[v];
            }
            double score = 0.0;
            int totalGain = 0;
            for (int color = 0; color < K; ++color) {
                if (gain[color] < 0) return;
                totalGain += gain[color];
                if (moved[color] == 0 || gain[color] == 0) continue;
                int desired = min(8 - firstIndex[color], pending[color][action.source]);
                double ready = min(1.0, SMALL_RATIO[moved[color]][desired]);
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
                double ready = min(1.0, SMALL_RATIO[action.count][desired]);
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

        for (int source : activeCells) {
            const Stack& stack = board[source];
            int top = topColor[source], block = topCount[source];
            if (top < 0 || block == stack.size || forward[top][source].empty() ||
                (!explore && waiting[source])) continue;
            if (Clock::now() >= deadline) return nullopt;
            array<int, MAX_COLOR> firstIndex;
            firstIndex.fill(8);
            for (int j = 0; j < stack.size; ++j)
                firstIndex[stack.color[j]] = min(firstIndex[stack.color[j]], j);
            array<int, MAX_COLOR> moved{};
            moved[top] = block;
            unsigned movingMask = 1u << top;
            for (int count = block + 1; count <= stack.size; ++count) {
                int kept = stack.size - count;
                ++moved[stack.color[kept]];
                movingMask |= 1u << stack.color[kept];
                if (!explore && count < batchCount[source] && wholeCanMove[source]) continue;
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
                    if (allAdvance) evaluate(first, action, moved, movingMask, firstIndex);
                    const Stack& landing = readStack(first, target);
                    int payload = max(0, landing.size - board[target].size);
                    if (payload == 0) continue;

                    if (allowTransfers) {
                        // Release the newly exposed block or carry the
                        // entire surviving payload. Search every physical
                        // direction, including right-angle transfers and
                        // recovery from a temporary off-tree landing.
                        int exposed = min(payload, landing.topCount());
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
                                    evaluate(second, pair, moved, movingMask, firstIndex);
                                }
                            }
                        }
                    } else if (allAdvance) {
                        // Odd: release the newly exposed bottom-color block.
                        // Even: carry the entire surviving payload once more,
                        // restoring its original orientation at the next stop.
                        int exposed = min(payload, landing.topCount());
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
                                evaluate(second, pair, moved, movingMask, firstIndex);
                            }
                        }
                    } else {
                        // Carry the upper colors forward, flip the hidden
                        // bottom block to the top, then use those upper
                        // colors as a springboard to jump back past source.
                        int maxNext = landing.size - bottomCount + 1;
                        for (int nextLen = len; nextLen <= maxNext; ++nextLen) {
                            if (deadlineReached()) return nullopt;
                            int back = nextLen - len;
                            int end = ray[target][step.direction ^ 1][nextLen - 1];
                            if (end < 0 || (back > 0 &&
                                jump[bottomColor][source][back - 1] != end)) continue;
                            LocalState second = first;
                            if (!simulate(second, target, end, bottomCount, nextLen)) continue;
                            Candidate pair = action;
                            pair.secondTarget = end;
                            pair.secondCount = bottomCount;
                            pair.secondLength = nextLen;
                            evaluate(second, pair, moved, movingMask, firstIndex);
                        }
                    }
                }
            }
        }

        if (decision.greedy.source < 0) return nullopt;
        return decision;
    }

    int moveDirection(int source, int target, int length) const {
        if (source < 0 || source >= cells || target < 0 || target >= cells ||
            length < 1 || length > 8) return -1;
        int info = moveInfo[source * cells + target];
        return info >= 0 && (info & 7) == length - 1 ? info >> 3 : -1;
    }

    bool apply(State& value, const Candidate& action, vector<Op>* output = nullptr) const {
        const uint64_t previousRevision = value.revision;
        bool changed = false;
        auto move = [&](int source, int target, int count, int length) {
            int d = moveDirection(source, target, length);
            if (d < 0) return false;
            int kept = value.board[source].size - count;
            int homed = transferSlimes(value.board[source], value.board[target], kept, length,
                                       nest[source], nest[target]);
            if (homed < 0) return false;
            if (!changed) {
                changed = true;
                value.previousRevision = previousRevision;
                value.revision = ++nextRevision;
                value.changedCount = 0;
            }
            for (int v : {source, target}) {
                bool seen = false;
                for (int i = 0; i < value.changedCount; ++i)
                    if (value.changedCells[i] == v) seen = true;
                if (!seen) {
                    assert(value.changedCount < 3);
                    value.changedCells[value.changedCount++] = v;
                }
            }
            value.remaining -= homed;
            if (output) output->push_back({cellRow[source], cellColumn[source], kept, "UDLR"[d], length});
            return true;
        };
        if (!move(action.source, action.target, action.count, action.length)) return false;
        return action.secondTarget < 0 ||
               move(action.target, action.secondTarget, action.secondCount, action.secondLength);
    }

};

struct GreedySeed {
    vector<Op> operations;
    vector<CooperativeScheduler::Candidate> actions;
    int mode = 0;
    bool shortcuts = false, transfers = false;
};

optional<GreedySeed> buildCooperativeSeed(
    CooperativeScheduler& sim, int mode, bool shortcuts, bool transfers,
    chrono::steady_clock::time_point deadline) {
    GreedySeed seed;
    seed.mode = mode;
    seed.shortcuts = shortcuts;
    seed.transfers = transfers;
    sim.state = sim.initial;
    while (sim.state.remaining > 0) {
        if (chrono::steady_clock::now() >= deadline) return nullopt;
        auto decision = sim.decide(mode, shortcuts, transfers, deadline);
        if (!decision) return nullopt;
        const auto& action = decision->greedy;
        if ((int)seed.operations.size() + action.steps() > MAX_OP ||
            !sim.apply(sim.state, action, &seed.operations)) return nullopt;
        seed.actions.push_back(action);
    }
    return seed;
}

struct HistorySearchStats {
    int initialRawOps = -1, bestRawOps = -1, currentRawOps = -1;
    long long attempts = 0, completed = 0, accepted = 0, improvements = 0;
    long long improvedTrials = 0, neutralTrials = 0, worseTrials = 0;
    long long neutralAccepted = 0, worseAccepted = 0;
    long long pruned = 0, deadEnds = 0, timedOut = 0, invalid = 0;
    long long pivotDecisions = 0, cachedPivots = 0, freshDecisions = 0;
    long long prefixOps = 0, restoredActions = 0, rejoined = 0, suffixOps = 0;
    long long sameSource = 0, differentSource = 0;
    array<long long, 3> cutThirds{};
};

// Search complete histories. Only the pivot uses relaxed scheduling choices;
// the repair suffix uses the original cooperative evaluator. Two-step transfers
// are atomic, so no checkpoint or edit can expose their temporary off-tree state.
optional<vector<Op>> rewriteCooperativeHistory(
    CooperativeScheduler& sim, const GreedySeed& seed,
    chrono::steady_clock::time_point deadline, HistorySearchStats& stats) {
    using Clock = chrono::steady_clock;
    using Candidate = CooperativeScheduler::Candidate;
    using State = CooperativeScheduler::State;
    using Stack = CooperativeScheduler::Stack;
    constexpr int CHECKPOINT = 16;
    struct Alternative { Candidate action; double weight; };
    struct Site {
        bool ready = false;
        vector<Alternative> choices;
        uint64_t tried = 0;
        bool available() const {
            // At most 12 global + 12 interacting + 8 same-source choices.
            return !ready || tried != ((uint64_t(1) << choices.size()) - 1);
        }
    };
    struct History {
        vector<Candidate> actions;
        vector<int> cost, weight;
        vector<uint64_t> hash;
        vector<State> checkpoints;
        unordered_multimap<uint64_t, int> boundary;
        vector<Site> sites;
        vector<int> weightTree;
        int totalWeight = 0;
    };
    auto mix = [](uint64_t x) {
        x ^= x >> 30;
        x *= 0xbf58476d1ce4e5b9ULL;
        x ^= x >> 27;
        x *= 0x94d049bb133111ebULL;
        return x ^ (x >> 31);
    };
    auto stackHash = [&](int v, const Stack& stack) {
        uint64_t packed = stack.size;
        for (int j = 0; j < stack.size; ++j)
            packed |= uint64_t(stack.color[j] + 1) << (4 + 4 * j);
        return mix(packed ^ (uint64_t(v + 1) * 0x9e3779b97f4a7c15ULL));
    };
    auto stateHash = [&](const State& state) {
        uint64_t hash = 0;
        for (int v = 0; v < sim.cells; ++v) hash ^= stackHash(v, state.board[v]);
        return hash;
    };
    auto applyTracked = [&](State& state, const Candidate& action, uint64_t& hash) {
        array<int, 3> touched{action.source, action.target, action.secondTarget};
        uint64_t before = 0;
        for (int i = 0; i < 3; ++i) {
            int v = touched[i];
            if (v < 0) continue;
            if (v >= sim.cells) return false;
            for (int j = 0; j < i; ++j)
                if (touched[j] == v) { v = -1; break; }
            if (v >= 0) before ^= stackHash(v, state.board[v]);
        }
        if (!sim.apply(state, action)) return false;
        hash ^= before;
        for (int i = 0; i < 3; ++i) {
            int v = touched[i];
            if (v < 0) continue;
            for (int j = 0; j < i; ++j)
                if (touched[j] == v) { v = -1; break; }
            if (v >= 0) hash ^= stackHash(v, state.board[v]);
        }
        return true;
    };
    auto equalState = [](const State& a, const State& b) {
        return a.remaining == b.remaining && a.board == b.board;
    };
    auto buildHistory = [&](vector<Candidate> actions) -> optional<History> {
        History history;
        history.actions = std::move(actions);
        int count = (int)history.actions.size();
        history.cost.reserve(count + 1);
        history.hash.reserve(count + 1);
        history.boundary.reserve((count + 1) * 2);
        history.checkpoints.reserve(count / CHECKPOINT + 1);
        history.weight.resize(count);
        history.sites.resize(count);
        for (int i = 0; i < count; ++i)
            history.weight[i] = 2 + 2 * i / max(1, count);
        State state = sim.initial;
        uint64_t hash = stateHash(state);
        int cost = 0;
        for (int i = 0; i <= count; ++i) {
            history.cost.push_back(cost);
            history.hash.push_back(hash);
            history.boundary.emplace(hash, i);
            if (i % CHECKPOINT == 0) history.checkpoints.push_back(state);
            if (i == count) break;
            const Candidate& action = history.actions[i];
            if (action.source < 0 || action.source >= sim.cells ||
                action.target < 0 || action.target >= sim.cells) return nullopt;
            // Retain access to the entire history, with extra weight just
            // before merges, springboard removal, and mixed transfers.
            int height = state.board[action.source].size;
            bool event = state.board[action.target].size > 0 || action.steps() == 2 ||
                         (height >= 2 && action.count == height);
            if (event)
                for (int j = max(0, i - 2); j <= i; ++j)
                    history.weight[j] = min(10, history.weight[j] + 2);
            cost += action.steps();
            if (cost > MAX_OP || !applyTracked(state, action, hash)) return nullopt;
        }
        if (state.remaining != 0) return nullopt;
        // Fenwick tree keeps weighted pivot selection logarithmic. A site is
        // removed once its cached alternatives have all been tried.
        history.weightTree.assign(count + 1, 0);
        for (int i = 1; i <= count; ++i) {
            history.totalWeight += history.weight[i - 1];
            history.weightTree[i] += history.weight[i - 1];
            int next = i + (i & -i);
            if (next <= count) history.weightTree[next] += history.weightTree[i];
        }
        return history;
    };
    auto removeSite = [](History& history, int at) {
        int weight = history.weight[at];
        history.totalWeight -= weight;
        for (int i = at + 1; i < (int)history.weightTree.size(); i += i & -i)
            history.weightTree[i] -= weight;
    };
    auto selectSite = [](const History& history, int draw) {
        int at = 0, count = (int)history.actions.size(), step = 1;
        while (step <= count / 2) step <<= 1;
        for (; step > 0; step >>= 1) {
            int next = at + step;
            if (next <= count && history.weightTree[next] <= draw) {
                draw -= history.weightTree[next];
                at = next;
            }
        }
        return at;
    };
    auto restore = [&](const History& history, int at, State& out) {
        int base = at / CHECKPOINT * CHECKPOINT;
        out = history.checkpoints[at / CHECKPOINT];
        for (int i = base; i < at; ++i) {
            ++stats.restoredActions;
            if (!sim.apply(out, history.actions[i])) return false;
        }
        return true;
    };

    stats.initialRawOps = stats.bestRawOps = stats.currentRawOps = (int)seed.operations.size();
    if (seed.actions.empty() || Clock::now() >= deadline) return nullopt;
    auto initial = buildHistory(seed.actions);
    if (!initial || initial->cost.back() != stats.initialRawOps) {
        ++stats.invalid;
        return nullopt;
    }
    History current = std::move(*initial);
    int bestRaw = current.cost.back();
    vector<Candidate> bestActions = current.actions;
    mt19937_64 rng(0x07220260928ULL);
    auto uniform = [&]() {
        return ((rng() >> 11) + 0.5) * (1.0 / 9007199254740992.0);
    };
    const auto searchStart = Clock::now();
    const double duration = max(1e-9, chrono::duration<double>(deadline - searchStart).count());
    const double startTemperature = clamp(bestRaw * 0.01, 1.5, 5.0);

    while (Clock::now() < deadline) {
        if (current.totalWeight == 0) break;
        int cut = selectSite(current, (int)(rng() % current.totalWeight));
        if (!restore(current, cut, sim.state)) { ++stats.invalid; break; }
        Site& site = current.sites[cut];
        if (!site.ready) {
            ++stats.pivotDecisions;
            auto decision = sim.decide(seed.mode, seed.shortcuts, seed.transfers,
                                       deadline, 12, &current.actions[cut]);
            if (Clock::now() >= deadline) { ++stats.timedOut; break; }
            site.ready = true;
            if (!decision) {
                removeSite(current, cut);
                continue;
            }
            auto addChoices = [&](const vector<Candidate>& choices, double weight) {
                for (int rank = 0; rank < (int)choices.size(); ++rank) {
                    const Candidate& action = choices[rank];
                    if (CooperativeScheduler::sameAction(action, current.actions[cut])) continue;
                    double value = weight / (rank + 2.0);
                    auto found = find_if(site.choices.begin(), site.choices.end(),
                        [&](const Alternative& old) {
                            return CooperativeScheduler::sameAction(old.action, action);
                        });
                    if (found == site.choices.end()) site.choices.push_back({action, value});
                    else found->weight += value;
                }
            };
            addChoices(decision->alternatives, 1.0);
            addChoices(decision->related, 3.0);
            addChoices(decision->sameSource, 2.0);
            if (site.choices.empty()) {
                removeSite(current, cut);
                continue;
            }
        } else ++stats.cachedPivots;

        double choiceWeight = 0.0;
        for (int i = 0; i < (int)site.choices.size(); ++i)
            if (!(site.tried & (uint64_t(1) << i))) choiceWeight += site.choices[i].weight;
        double choiceDraw = uniform() * choiceWeight;
        int selected = -1;
        for (int i = 0; i < (int)site.choices.size(); ++i) {
            if (site.tried & (uint64_t(1) << i)) continue;
            selected = i;
            choiceDraw -= site.choices[i].weight;
            if (choiceDraw <= 0.0) break;
        }
        if (selected < 0) break;
        site.tried |= uint64_t(1) << selected;
        if (!site.available()) removeSite(current, cut);
        Candidate next = site.choices[selected].action;
        ++stats.attempts;
        ++stats.cutThirds[min(2, 3 * cut / (int)current.actions.size())];
        if (next.source == current.actions[cut].source) ++stats.sameSource;
        else ++stats.differentSource;
        stats.prefixOps += current.cost[cut];

        double progress = clamp(chrono::duration<double>(Clock::now() - searchStart).count() /
                                    duration, 0.0, 1.0);
        double temperature = startTemperature * pow(0.12 / startTemperature, progress);
        // This only prunes completed-cost bounds, never a board heuristic.
        // A bounded distance from the best history prevents a neutral/worse
        // random walk from consuming the remaining time far from the incumbent.
        int limit = min({MAX_OP, current.cost.back() + (int)ceil(6.0 * temperature),
                         bestRaw + max(3, (int)ceil(3.0 * temperature))});
        int cost = current.cost[cut], join = -1;
        uint64_t hash = current.hash[cut];
        vector<Candidate> rebuilt;
        bool completed = false;
        while (true) {
            if (Clock::now() >= deadline) { ++stats.timedOut; break; }
            if (cost + next.steps() > limit) { ++stats.pruned; break; }
            if (!applyTracked(sim.state, next, hash)) { ++stats.invalid; break; }
            rebuilt.push_back(next);
            cost += next.steps();
            if (sim.state.remaining == 0) { completed = true; break; }
            // At most 16 slimes can disappear from the two endpoints in one
            // operation. This intentionally loose lower bound is always safe.
            if (cost + (sim.state.remaining + 15) / 16 > limit) {
                ++stats.pruned;
                break;
            }
            auto range = current.boundary.equal_range(hash);
            for (auto it = range.first; it != range.second; ++it) {
                int at = it->second;
                if (at <= cut || cost + current.cost.back() - current.cost[at] > limit) continue;
                State exact;
                if (!restore(current, at, exact)) continue;
                // The hash is an index only. Compare every ordered color stack
                // before splicing a suffix, including homing at both endpoints.
                if (!equalState(sim.state, exact)) continue;
                join = at;
                completed = true;
                cost += current.cost.back() - current.cost[at];
                ++stats.rejoined;
                stats.suffixOps += current.cost.back() - current.cost[at];
                break;
            }
            if (completed) break;
            ++stats.freshDecisions;
            auto decision = sim.decide(seed.mode, seed.shortcuts, seed.transfers, deadline);
            if (!decision) {
                if (Clock::now() >= deadline) ++stats.timedOut;
                else ++stats.deadEnds;
                break;
            }
            next = decision->greedy;
        }
        if (!completed) continue;
        ++stats.completed;
        int delta = cost - current.cost.back();
        if (delta < 0) ++stats.improvedTrials;
        else if (delta == 0) ++stats.neutralTrials;
        else ++stats.worseTrials;
        // Independent swaps often reconverge immediately. Admit some neutral
        // histories for new contexts, without rebuilding the cache for each one.
        bool accept = delta < 0 || (delta == 0 ? uniform() < 0.25 :
                                               uniform() < exp(-delta / temperature));
        if (!accept) continue;
        vector<Candidate> actions(current.actions.begin(), current.actions.begin() + cut);
        actions.insert(actions.end(), rebuilt.begin(), rebuilt.end());
        int tailStart = (int)actions.size();
        if (join >= 0)
            actions.insert(actions.end(), current.actions.begin() + join, current.actions.end());
        auto changed = buildHistory(std::move(actions));
        if (!changed || changed->cost.back() != cost) { ++stats.invalid; continue; }
        // Candidate rankings are reusable at identical prefix/suffix states.
        // Reset trial marks because the complete continuation has now changed.
        for (int i = 0; i < cut; ++i) {
            changed->sites[i] = std::move(current.sites[i]);
            changed->sites[i].tried = 0;
        }
        if (join >= 0)
            for (int i = join; i < (int)current.actions.size(); ++i) {
                changed->sites[tailStart + i - join] = std::move(current.sites[i]);
                changed->sites[tailStart + i - join].tried = 0;
            }
        for (int i = 0; i < (int)changed->sites.size(); ++i)
            if (!changed->sites[i].available()) removeSite(*changed, i);
        current = std::move(*changed);
        ++stats.accepted;
        if (delta == 0) ++stats.neutralAccepted;
        if (delta > 0) ++stats.worseAccepted;
        stats.currentRawOps = cost;
        if (cost < bestRaw) {
            bestRaw = stats.bestRawOps = cost;
            bestActions = current.actions;
            ++stats.improvements;
        }
    }
    if (bestRaw >= stats.initialRawOps) return nullopt;
    State replay = sim.initial;
    vector<Op> output;
    output.reserve(bestRaw);
    for (const Candidate& action : bestActions)
        if (!sim.apply(replay, action, &output)) { ++stats.invalid; return nullopt; }
    if (replay.remaining != 0 || (int)output.size() != bestRaw) {
        ++stats.invalid;
        return nullopt;
    }
    return output;
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    const auto start = chrono::steady_clock::now();
    int N, K;
    cin >> N >> K;
    vector<string> grid(N);
    for (auto& row : grid) cin >> row;
    const auto routingDeadline = start + chrono::milliseconds(350);
    const auto baselineDeadline = start + chrono::milliseconds(950);
    const auto searchDeadline = start + chrono::milliseconds(1730);
    const auto postprocessingDeadline = start + chrono::milliseconds(1870);
    auto routes = buildCrossingTrees(grid, K, routingDeadline);
    if ((int)routes.size() != K) return 0;
    auto fallback = sequentialSolve(grid, K, routes);
    if (!fallback) return 0;
    vector<Op> answer = std::move(*fallback);
    const size_t fallbackOps = answer.size();
    CooperativeScheduler scheduler(grid, K, routes);
    optional<GreedySeed> seed;
    int seedScore = INT_MAX, seedAttempts = 0, seedCompleted = 0;
    const int modes[] = {0, 2, 5, 1, 6, 3, 4, 7};
    for (int job = 0; job < 11; ++job) {
        if (chrono::steady_clock::now() >= baselineDeadline) break;
        int mode = job < 8 ? modes[job] : 0;
        bool shortcuts = job >= 9, transfers = job == 8 || job == 10;
        ++seedAttempts;
        auto candidate = buildCooperativeSeed(scheduler, mode, shortcuts, transfers, baselineDeadline);
        if (!candidate) continue;
        ++seedCompleted;
        auto compressed = compressBaselineMoves(grid, K, candidate->operations);
        if (compressed.size() < answer.size()) answer = compressed;
        if (!seed || (int)compressed.size() < seedScore ||
            ((int)compressed.size() == seedScore &&
             candidate->operations.size() < seed->operations.size())) {
            seedScore = (int)compressed.size();
            seed = std::move(candidate);
        }
    }
    const size_t baselineOps = answer.size();
    int seedMode = seed ? seed->mode : -1;
    bool seedShortcuts = seed && seed->shortcuts, seedTransfers = seed && seed->transfers;
    HistorySearchStats stats;
    if (seed) {
        auto rewritten = rewriteCooperativeHistory(scheduler, *seed, searchDeadline, stats);
        if (rewritten) {
            auto compressed = compressBaselineMoves(grid, K, *rewritten);
            compressed = compressMoves(grid, K, compressed, postprocessingDeadline);
            if (compressed.size() < answer.size()) answer = std::move(compressed);
        }
    }
    const size_t beforePostprocessing = answer.size();
    answer = compressMoves(grid, K, answer, postprocessingDeadline);
    answer = optimizeMoveWindows(grid, K, answer, postprocessingDeadline);
    answer = compressMoves(grid, K, answer, postprocessingDeadline);
    cerr << "fallback_ops=" << fallbackOps
         << " baseline_ops=" << baselineOps
         << " seed_attempts=" << seedAttempts
         << " seed_completed=" << seedCompleted
         << " seed_mode=" << seedMode
         << " seed_shortcuts=" << seedShortcuts
         << " seed_transfers=" << seedTransfers
         << " history_initial_raw_ops=" << stats.initialRawOps
         << " history_best_raw_ops=" << stats.bestRawOps
         << " history_current_raw_ops=" << stats.currentRawOps
         << " history_attempts=" << stats.attempts
         << " history_completed=" << stats.completed
         << " history_accepted=" << stats.accepted
         << " history_improvements=" << stats.improvements
         << " history_improved_trials=" << stats.improvedTrials
         << " history_neutral_trials=" << stats.neutralTrials
         << " history_worse_trials=" << stats.worseTrials
         << " history_neutral_accepted=" << stats.neutralAccepted
         << " history_worse_accepted=" << stats.worseAccepted
         << " history_pruned=" << stats.pruned
         << " history_dead_ends=" << stats.deadEnds
         << " history_timed_out=" << stats.timedOut
         << " history_invalid=" << stats.invalid
         << " history_pivot_decisions=" << stats.pivotDecisions
         << " history_cached_pivots=" << stats.cachedPivots
         << " history_fresh_decisions=" << stats.freshDecisions
         << " history_prefix_ops=" << stats.prefixOps
         << " history_restored_actions=" << stats.restoredActions
         << " history_rejoined=" << stats.rejoined
         << " history_suffix_ops=" << stats.suffixOps
         << " history_same_source=" << stats.sameSource
         << " history_different_source=" << stats.differentSource
         << " history_early_cuts=" << stats.cutThirds[0]
         << " history_middle_cuts=" << stats.cutThirds[1]
         << " history_late_cuts=" << stats.cutThirds[2]
         << " postprocess_saved=" << beforePostprocessing - answer.size()
         << " final_ops=" << answer.size() << '\n';
    for (const auto& op : answer)
        cout << op.r << ' ' << op.c << ' ' << op.k << ' ' << op.dir << ' ' << op.len << '\n';
    return 0;
}
