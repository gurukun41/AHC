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
vector<Op> compressMoves(const vector<string>& grid, int K,
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

struct RouteTree {
    vector<int> parent, depth, direction, straight;
};

// Grow each color's tree toward its slimes. Crossing another color is cheaper
// when both routes have a long straight run toward their respective nests.
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

    using Support = vector<array<double, 2>>;
    const Support emptySupport(cells, array<double, 2>{0.0, 0.0});
    auto build = [&](int color, const Support& foreign, bool timed) -> optional<RouteTree> {
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
                    double reward = 0.70 * crossing[v][towardNest / 2] *
                                    (0.25 + 0.75 * lengthValue);
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
            support[v][tree.direction[v] / 2] += sign * value;
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

// Interleave all colors so that a temporarily parked block can serve as a
// springboard for a different color. Mixed two-move actions may temporarily
// leave a color's tree, but must return all survivors to their own trees without
// increasing any payload's depth. Only completed candidates are returned.
optional<vector<Op>> cooperativeSolve(
    const vector<string>& grid, int K,
    const vector<vector<int>>& parent,
    const vector<vector<int>>& depth, int operationLimit, int mode,
    chrono::steady_clock::time_point deadline) {
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

    // Variants are fixed before execution: they trade readiness against
    // preserving useful foreign-color springboards.
    static constexpr double supportWeights[8] =
        {0.45, 0.85, 1.35, 1.85, 0.65, 1.10, 1.60, 2.20};
    static constexpr double readinessWeights[8] =
        {7.0, 7.0, 7.0, 7.0, 10.0, 10.0, 10.0, 10.0};
    const int variant = ((mode % 8) + 8) % 8;
    const bool useMixed = mode >= 8;
    const double supportWeight = supportWeights[variant];
    const double readinessWeight = readinessWeights[variant];

    vector<int> topColor(cells), topCount(cells);
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
                if (incoming == 0 || run[color][v] < 2 || distance > cells) continue;
                double batches = min(2.0, (double)incoming / max(1, arrivalSize[color][v]));
                traffic[color][v] = batches / (1.0 + 0.5 * distance);
            }
        }

        auto colorSupport = [&](int color, int v, int height) {
            if (height <= 0 || height >= 8 || traffic[color][v] == 0.0) return 0.0;
            int saved = min(height, run[color][v] - 1);
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
        for (int source = 0; source < cells; ++source) {
            int color = topColor[source];
            if (color < 0 || run[color][source] == 0) continue;
            int height = board[source].size;
            int block = topCount[source];
            int bottom = height - block;
            int subtree = pending[color][source];
            int desired = min(8 - bottom, subtree);
            double originalSupport = foreignSupport(color, source, height);

            for (int count = 1; count <= block; ++count) {
                int kept = height - count;
                int maxLength = min(run[color][source], kept + 1);
                int skippedSameColor = 0;
                for (int len = 1; len <= maxLength; ++len) {
                    int target = jump[color][source][len - 1];
                    int destinationHeight = board[target].size;
                    if (len > 1) {
                        int skipped = jump[color][source][len - 2];
                        skippedSameColor += local[color][skipped];
                    }
                    if (destinationHeight + count > 8) continue;

                    int sameAtTarget = topColor[target] == color ? topCount[target] : 0;
                    double ready = min(1.0, (double)count / max(1, desired));
                    // Once a block is full, waiting for more descendants cannot
                    // save an additional batch on this departure.
                    double score = 1.0 + 0.55 * count + 1.65 * (len - 1);
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

                    // A stable, mode-dependent tie break explores different
                    // interleavings without random generators or runtime tuning.
                    unsigned tie = (unsigned)(source + 1) * 2654435761u;
                    tie ^= (unsigned)(color + 1 + variant * 17) * 2246822519u;
                    score += 0.00001 * (tie % 1024);
                    if (score > best.score) best = {source, target, count, len, score};
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

                array<int, 26> moving{}, skipped{};
                for (int j = kept; j < from.size; ++j) ++moving[from.color[j]];
                for (int step = 1; step < len; ++step) {
                    int v = (source / n + dr * step) * n + source % n + dc * step;
                    const Stack& middle = readStack(state, v);
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
                int maximum = min(run[color][v], stack.size - count + 1);
                for (int len = maximum; len >= 1; --len) {
                    int target = jump[color][v][len - 1];
                    if (readStack(state, target).size + count <= 8)
                        return 0.5 * count + len - 1;
                }
                return 0.0;
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
                double supportChange = 0.0, exposureChange = 0.0;
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
                score += 1.25 * state.merged - 0.85 * state.splits - 0.8 * state.skipped;
                score += 0.20 * state.homed + supportWeight * supportChange;
                score += 0.30 * exposureChange;
                score /= steps; // Compare one- and two-move actions per operation.
                if (score > best.score) {
                    best = action;
                    best.score = score;
                }
            };

            for (int source = 0; source < cells; ++source) {
                if (chrono::steady_clock::now() >= deadline) return nullopt;
                const Stack& stack = board[source];
                int top = topColor[source], block = topCount[source];
                if (top < 0 || block == stack.size || run[top][source] == 0) continue;
                for (int count = block + 1; count <= stack.size; ++count) {
                    int kept = stack.size - count;
                    int bottomColor = stack.color[kept], bottomCount = 1;
                    while (bottomCount < count && stack.color[kept + bottomCount] == bottomColor)
                        ++bottomCount;
                    int maximum = min(run[top][source], kept + 1);
                    for (int len = 1; len <= maximum; ++len) {
                        int target = jump[top][source][len - 1];
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

                        if (allAdvance) {
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

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    const auto searchStart = chrono::steady_clock::now();

    int N, K;
    cin >> N >> K;

    vector<string> C(N);
    for (auto &s : C) cin >> s;

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

    // 全色の経路を先に作り、交差点とそこから巣方向への直線を優遇する。
    const auto routingDeadline = searchStart + chrono::milliseconds(350);
    auto routes = buildCrossingTrees(C, K, routingDeadline);
    if ((int)routes.size() != K) return 0;
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

    // 生成した操作列の盤面遷移を保ったまま、連続する2移動を圧縮する。
    answer = compressMoves(C, K, answer);

    // 前半は既存の単色移動、後半は反転の偶奇を考慮した混色移動も使う。
    // 完走して操作数が減った候補だけ採用し、途中停止した候補は捨てる。
    const auto monochromeDeadline = searchStart + chrono::milliseconds(800);
    const auto searchDeadline = searchStart + chrono::milliseconds(1650);
    int cooperativeMode = -1;
    if (!stopped) {
        for (int mode = 0; mode < 16; ++mode) {
            const auto deadline = mode < 8 ? monochromeDeadline : searchDeadline;
            if (chrono::steady_clock::now() >= deadline) continue;
            const int limit = min(MAX_OP, 2 * (int)answer.size() + 1);
            auto candidate = cooperativeSolve(C, K, parentTrees, treeDepths,
                                               limit, mode, deadline);
            if (!candidate) continue;
            auto compressed = compressMoves(C, K, *candidate);
            if (compressed.size() < answer.size()) {
                answer = std::move(compressed);
                cooperativeMode = mode;
            }
        }
    }
    cerr << "cooperative_mode=" << cooperativeMode << '\n';

    // ------------------------------------------------------------
    // 出力
    // ------------------------------------------------------------

    for (const auto &op : answer) {
        cout << op.r << ' '
             << op.c << ' '
             << op.k << ' '
             << op.dir << ' '
             << op.len << '\n';
    }

    return 0;
}
