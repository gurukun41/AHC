#if defined(__GNUC__) && !defined(__clang__)
// Keep standard floating-point semantics for candidate scores and cache checks.
#pragma GCC optimize("O3", "unroll-loops")
#endif

// AtCoder defines ONLINE_JUDGE. Local builds retain diagnostics by default;
// -DENABLE_DEBUG=0 also selects submission-style diagnostics locally.
#ifndef ENABLE_DEBUG
#ifdef ONLINE_JUDGE
#define ENABLE_DEBUG 0
#else
#define ENABLE_DEBUG 1
#endif
#endif

// Define this before standard headers so runtime assertions are compiled out.
#if !ENABLE_DEBUG && !defined(NDEBUG)
#define NDEBUG
#endif

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

struct ModelConfig {
    double supportWeight, readinessWeight;
};

static constexpr auto MODELS = [] {
    array<ModelConfig, 32> models{};
    int next = 0;
    auto addModel = [&](double support, double readiness) {
        for (int i = 0; i < next; ++i)
            if (models[i].supportWeight == support && models[i].readinessWeight == readiness)
                return;
        models[next++] = {support, readiness};
    };
    // Keep the existing eight models first, covering the full range before refinement.
    for (double readiness : {4.0, 13.0})
        for (double support : {0.0, 0.75, 1.50, 3.0})
            addModel(support, readiness);
    // Complete the denser 8x4 grid without duplicating the existing models.
    for (double readiness : {4.0, 7.0, 10.0, 13.0})
        for (double support : {0.0, 0.375, 0.75, 1.125, 1.50, 2.0, 2.50, 3.0})
            addModel(support, readiness);
    return models;
}();

span<const int> candidateModelIds(int load) {
    static constexpr auto allModels = [] {
        array<int, MODELS.size()> ids{};
        for (int i = 0; i < (int)ids.size(); ++i) ids[i] = i;
        return ids;
    }();
    // Nested subsets retain the original model IDs and relative order.
    // Sixteen models cover a 4x4 grid; eight keep its four corners plus
    // four interior points; four keep the full weight range's corners.
    static constexpr array<int, 16> models16{
        0, 1, 2, 3, 4, 5, 6, 7, 12, 14, 16, 19, 20, 22, 24, 27};
    static constexpr array<int, 8> models8{0, 3, 4, 7, 14, 16, 22, 24};
    static constexpr array<int, 4> models4{0, 3, 4, 7};
    if (load <= 1000) return allModels;
    if (load <= 1800) return models16;
    if (load <= 2800) return models8;
    return models4;
}

// Tower sizes bound readiness/fit denominators by eight. An incoming batch
// below twice that denominator needs a numerator of at most fifteen.
static constexpr auto SMALL_RATIO = [] {
    array<array<double, 9>, 17> result{};
    for (int numerator = 0; numerator <= 16; ++numerator)
        for (int denominator = 0; denominator <= 8; ++denominator)
            result[numerator][denominator] = (double)numerator / max(1, denominator);
    return result;
}();

uint64_t mixHash(uint64_t value) {
    value ^= value >> 30;
    value *= 0xbf58476d1ce4e5b9ULL;
    value ^= value >> 27;
    value *= 0x94d049bb133111ebULL;
    return value ^ (value >> 31);
}

template<class Better>
void retainBestNodes(vector<int>& ids, int width, const Better& better) {
    if ((int)ids.size() > width) {
        nth_element(ids.begin(), ids.begin() + width, ids.end(), better);
        ids.resize(width);
    }
    sort(ids.begin(), ids.end(), better);
}

// Only live colors participate in equality and hashing; unused slots retain old values.
struct SlimeStack {
    array<unsigned char, 8> color{};
    uint8_t size = 0; // 0..8; arithmetic is promoted to int.

    bool operator==(const SlimeStack& other) const {
        return size == other.size &&
               equal(color.begin(), color.begin() + size, other.color.begin());
    }
    int commonPrefix(const SlimeStack& other) const {
        int count = 0;
        while (count < min(size, other.size) && color[count] == other.color[count]) ++count;
        return count;
    }
    uint64_t appendToHash(uint64_t hash) const {
        hash = (hash ^ uint64_t(size + 1)) * 1099511628211ULL;
        for (int i = 0; i < size; ++i)
            hash = (hash ^ uint64_t(color[i] + 17)) * 1099511628211ULL;
        return hash;
    }
    uint64_t positionHash(int cell) const {
        uint64_t packed = size;
        for (int i = 0; i < size; ++i)
            packed |= uint64_t(color[i] + 1) << (4 + 4 * i);
        return mixHash(packed ^ (uint64_t(cell + 1) * 0x9e3779b97f4a7c15ULL));
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
static_assert(sizeof(SlimeStack) == 9);

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

// Index only geometric endpoint dependencies. No board snapshots are kept:
// the current board is replayed once by each compression pass.
struct MoveDependencies {
    using Clock = chrono::steady_clock;
    bool valid = false;
    vector<int> from, to;
    vector<vector<int>> uses;

    MoveDependencies(const SlimeBoard& sim, const vector<Op>& ops,
                     Clock::time_point deadline = Clock::time_point::max())
        : from(ops.size()), to(ops.size()), uses(sim.initial.size()) {
        for (int i = 0; i < (int)ops.size(); ++i) {
            if ((i & 31) == 0 && Clock::now() >= deadline) return;
            if (!sim.endpoints(ops[i], from[i], to[i])) return;
            uses[from[i]].push_back(i);
            uses[to[i]].push_back(i);
        }
        valid = Clock::now() < deadline;
    }

    int nextUse(int cell, int after) const {
        const auto& events = uses[cell];
        auto it = upper_bound(events.begin(), events.end(), after);
        return it == events.end() ? (int)from.size() : *it;
    }
};

struct DependencyRegion {
    vector<int> cells, selected;
    int end = 0;
};

// Extend through endpoint dependencies, rather than a fixed number of moves.
// When a new cell enters, absorb all its earlier uses in this interval too;
// every unselected interior move then commutes with every replacement move.
DependencyRegion collectDependencyRegion(
    const MoveDependencies& deps, int begin, int maxCells,
    chrono::steady_clock::time_point deadline) {
    using Clock = chrono::steady_clock;
    DependencyRegion region;
    region.end = begin;
    vector<unsigned char> selected(deps.from.size(), 0);
    vector<int> pendingAt(deps.from.size(), 0);
    int generation = 0, candidate = begin, visited = 0;
    while (candidate < (int)deps.from.size() && Clock::now() < deadline) {
        ++generation;
        vector<int> cells = region.cells, pending;
        bool oversized = false, timedOut = false;
        auto contains = [&](int cell) {
            return find(cells.begin(), cells.end(), cell) != cells.end();
        };
        auto absorb = [&](int at) {
            if (selected[at] || pendingAt[at] == generation) return;
            pendingAt[at] = generation;
            pending.push_back(at);
            for (int cell : {deps.from[at], deps.to[at]}) {
                if (contains(cell)) continue;
                cells.push_back(cell);
                if ((int)cells.size() > maxCells) oversized = true;
            }
        };
        absorb(candidate);
        // Existing cells have no unselected use before candidate: candidate
        // is the earliest next use of any current cell. Only new cells need
        // backward closure, including cells discovered by that closure.
        for (size_t c = region.cells.size(); c < cells.size() && !oversized; ++c) {
            const auto& events = deps.uses[cells[c]];
            auto it = lower_bound(events.begin(), events.end(), begin);
            for (; it != events.end() && *it <= candidate; ++it) {
                if (((++visited) & 31) == 0 && Clock::now() >= deadline) {
                    timedOut = true;
                    break;
                }
                absorb(*it);
                if (oversized) break;
            }
            if (timedOut) break;
        }
        if (oversized || timedOut) break;
        region.cells = std::move(cells);
        for (int at : pending) {
            selected[at] = 1;
            region.selected.push_back(at);
        }
        region.end = candidate + 1;
        candidate = (int)deps.from.size();
        for (int cell : region.cells)
            candidate = min(candidate, deps.nextUse(cell, region.end - 1));
    }
    sort(region.selected.begin(), region.selected.end());
    return region;
}

// Collapse a straight chain by jumping to the next use of its endpoints.
// Independent operations can be arbitrarily far apart in the original list.
vector<Op> compressMoves(
    const vector<string>& grid, int K, const vector<Op>& ops,
    chrono::steady_clock::time_point deadline = chrono::steady_clock::time_point::max()) {
    using Clock = chrono::steady_clock;
    SlimeBoard sim(grid, K);
    MoveDependencies deps(sim, ops, deadline);
    if (!deps.valid) return ops;
    auto board = sim.initial;
    vector<SlimeStack> original(board.size()), replacement(board.size());
    vector<Op> result;
    result.reserve(ops.size());
    for (int i = 0; i < (int)ops.size();) {
        if (Clock::now() >= deadline) {
            result.insert(result.end(), ops.begin() + i, ops.end());
            break;
        }
        const Op& first = ops[i];
        int from = deps.from[i], current = deps.to[i];
        int payload = board[from].size - first.k;
        int base = board[current].size;
        original[from] = board[from];
        original[current] = board[current];
        if (transferSlimes(original[from], original[current], first.k, first.len,
                           sim.nest[from], sim.nest[current]) < 0) return ops;
        vector<int> chainCells{from, current}, chainOps{i};
        bool palindrome = true;
        for (int a = first.k, b = board[from].size - 1; a < b; ++a, --b)
            if (board[from].color[a] != board[from].color[b]) palindrome = false;
        int distance = first.len, last = i, bestEnd = i, bestCount = 0;
        Op bestMove{};
        while (distance < first.k + 1 && original[current].size == base + payload &&
               Clock::now() < deadline) {
            int j = (int)ops.size();
            for (int cell : chainCells) j = min(j, deps.nextUse(cell, last));
            if (j == (int)ops.size()) break;
            const Op& next = ops[j];
            int a = deps.from[j], b = deps.to[j];
            if (a != current || next.dir != first.dir || next.k != base ||
                distance + next.len > first.k + 1) break;
            // The newly introduced destination must also be untouched by
            // skipped moves, so moving the complete chain earlier is safe.
            if (deps.nextUse(b, i - 1) != j) break;
            original[b] = board[b];
            int nextBase = original[b].size;
            if (!sim.apply(original, next)) return ops;
            chainCells.push_back(b);
            chainOps.push_back(j);
            current = b;
            base = nextBase;
            distance += next.len;
            last = j;
            if ((chainOps.size() & 1) || palindrome) {
                Op combined = first;
                combined.len = distance;
                for (int cell : chainCells) replacement[cell] = board[cell];
                bool valid = sim.apply(replacement, combined);
                for (int cell : chainCells)
                    if (!(replacement[cell] == original[cell])) valid = false;
                if (valid) {
                    bestMove = combined;
                    bestEnd = j + 1;
                    bestCount = (int)chainOps.size();
                }
            }
        }
        if (bestCount > 0) {
            const size_t committedSize = result.size();
            if (!sim.apply(board, bestMove)) return ops;
            result.push_back(bestMove);
            int chainAt = 0;
            for (int j = i; j < bestEnd; ++j) {
                if (((j - i) & 31) == 0 && Clock::now() >= deadline) {
                    result.resize(committedSize);
                    result.insert(result.end(), ops.begin() + i, ops.end());
                    return result;
                }
                if (chainAt < bestCount && chainOps[chainAt] == j) {
                    ++chainAt;
                    continue;
                }
                if (!sim.apply(board, ops[j])) return ops;
                result.push_back(ops[j]);
            }
            i = bestEnd;
        } else {
            if (!sim.apply(board, first)) return ops;
            result.push_back(first);
            ++i;
        }
    }
    return result;
}

struct RegionCompressionStats {
    int windows = 0, accepted = 0, savedOps = 0, windowTimeouts = 0;
    long long selectedOps = 0, spanOps = 0, acceptedSpanOps = 0;
    int maxSelectedOps = 0, maxSpanOps = 0, acceptedMaxSpan = 0;
    bool deadlineReached = false;
    long long elapsedNs = 0, transitions = 0;
};

// The two region passes share their exact transitions, search, validation,
// and commit path. Their policies only change search resources and guidance.
template<bool Large>
vector<Op> compressDependencyGroupsImpl(
    const vector<string>& grid, int K, const vector<Op>& ops,
    chrono::steady_clock::time_point deadline, RegionCompressionStats& stats) {
    using Clock = chrono::steady_clock;
    constexpr int MAX_ENDPOINTS = Large ? 12 : 6, MAX_CELLS = Large ? 14 : 6;
    constexpr int BEAM_WIDTH = Large ? 24 : 20, MAX_ATTEMPTS = Large ? 20000 : 2048;
    constexpr int MAX_NODES = Large ? 4096 : 768, MAX_WINDOWS = 128, INF = 1000;
    constexpr int MIN_SELECTED = Large ? 3 : 2, GROUP_MICROS = Large ? 1500 : 500;
    using State = array<SlimeStack, MAX_CELLS>;
    struct Edge { int from, to; Op op; };
    struct Node { State state; int parent; Op op; int score; };
    enum class ReplayResult { Valid, Invalid, TimedOut };

    const auto started = Clock::now();
    stats = {};
    auto finish = [&](vector<Op> result) {
        stats.elapsedNs = chrono::duration_cast<chrono::nanoseconds>(Clock::now() - started).count();
        return result;
    };
    if (started >= deadline) {
        stats.deadlineReached = true;
        return finish(ops);
    }
    SlimeBoard sim(grid, K);
    MoveDependencies deps(sim, ops, deadline);
    if (!deps.valid) {
        stats.deadlineReached = Clock::now() >= deadline;
        return finish(ops);
    }
    auto board = sim.initial;
    vector<SlimeStack> verified(board.size());
    vector<Op> result;
    result.reserve(ops.size());
    auto invalidOriginal = [&]() {
        stats.accepted = stats.savedOps = 0;
        stats.acceptedSpanOps = stats.acceptedMaxSpan = 0;
        return finish(ops);
    };
    auto straightDistance = [&](int a, int b) {
        int ar = a / sim.n, ac = a % sim.n, br = b / sim.n, bc = b % sim.n;
        if (a == b || (ar != br && ac != bc)) return INF;
        int len = abs(ar - br) + abs(ac - bc);
        if (len > 8) return INF;
        int dr = (br > ar) - (br < ar), dc = (bc > ac) - (bc < ac);
        for (int step = 1; step <= len; ++step)
            if (!sim.floorCell(ar + dr * step, ac + dc * step)) return INF;
        return len;
    };

    for (size_t i = 0; i < ops.size();) {
        if (Clock::now() >= deadline || stats.windows >= MAX_WINDOWS) {
            stats.deadlineReached = Clock::now() >= deadline;
            result.insert(result.end(), ops.begin() + i, ops.end());
            break;
        }
        const auto groupDeadline = min(deadline, Clock::now() + chrono::microseconds(GROUP_MICROS));
        DependencyRegion region = collectDependencyRegion(deps, (int)i, MAX_ENDPOINTS, groupDeadline);
        vector<int>& cells = region.cells;
        cells.reserve(MAX_CELLS);
        const int selectedCount = (int)region.selected.size();
        const int lastSelected = region.end - 1;
        auto contains = [](const vector<int>& group, int v) {
            return find(group.begin(), group.end(), v) != group.end();
        };
        State start{}, target{};
        auto replayTarget = [&](Clock::time_point replayDeadline) {
            for (int c = 0; c < (int)cells.size(); ++c)
                start[c] = target[c] = board[cells[c]];
            int replayed = 0;
            for (int at : region.selected) {
                if (((replayed++) & 31) == 0 && Clock::now() >= replayDeadline)
                    return ReplayResult::TimedOut;
                int a = find(cells.begin(), cells.end(), deps.from[at]) - cells.begin();
                int b = find(cells.begin(), cells.end(), deps.to[at]) - cells.begin();
                if (a == (int)cells.size() || b == (int)cells.size() ||
                    transferSlimes(target[a], target[b], ops[at].k, ops[at].len,
                                   sim.nest[deps.from[at]], sim.nest[deps.to[at]]) < 0)
                    return ReplayResult::Invalid;
            }
            return ReplayResult::Valid;
        };
        bool replayTimedOut = false;
        if constexpr (!Large) {
            ReplayResult replay = replayTarget(groupDeadline);
            if (replay == ReplayResult::Invalid) return invalidOriginal();
            replayTimedOut = replay == ReplayResult::TimedOut;
        }
        bool found = false;
        vector<Op> replacement;
        if (!replayTimedOut && selectedCount >= MIN_SELECTED && Clock::now() < groupDeadline) {
            ++stats.windows;
            stats.selectedOps += selectedCount;
            stats.spanOps += region.end - i;
            stats.maxSelectedOps = max(stats.maxSelectedOps, selectedCount);
            stats.maxSpanOps = max(stats.maxSpanOps, region.end - (int)i);
            if constexpr (Large) {
                vector<int> connectorCandidates;
                auto addConnector = [&](int r, int c) {
                    if (!sim.floorCell(r, c)) return;
                    int v = r * sim.n + c;
                    if (board[v].size != 0 || contains(cells, v) || contains(connectorCandidates, v)) return;
                    // A new connector must be untouched throughout the region;
                    // otherwise its skipped uses would no longer commute.
                    if (deps.nextUse(v, (int)i - 1) <= lastSelected) return;
                    connectorCandidates.push_back(v);
                };
                const int endpointCount = (int)cells.size();
                for (int a = 0; a < endpointCount; ++a) {
                    int r = cells[a] / sim.n, c = cells[a] % sim.n;
                    addConnector(r - 1, c);
                    addConnector(r + 1, c);
                    addConnector(r, c - 1);
                    addConnector(r, c + 1);
                    for (int b = 0; b < a; ++b) {
                        addConnector(r, cells[b] % sim.n);
                        addConnector(cells[b] / sim.n, c);
                    }
                }
                for (int extra = 0; extra < 2; ++extra) {
                    int best = -1, bestScore = INT_MIN;
                    for (int v : connectorCandidates) {
                        if (contains(cells, v)) continue;
                        int links = 0, score = 0;
                        for (int u : cells) {
                            int len = straightDistance(u, v);
                            if (len == INF) continue;
                            ++links;
                            score += 10 - len + (len <= 2 ? 8 : 0);
                        }
                        if (links >= 2 && score > bestScore) {
                            best = v;
                            bestScore = score;
                        }
                    }
                    if (best < 0) break;
                    cells.push_back(best);
                }
                ReplayResult replay = replayTarget(deadline);
                if (replay == ReplayResult::Invalid) return invalidOriginal();
                if (replay == ReplayResult::TimedOut) {
                    stats.deadlineReached = true;
                    result.insert(result.end(), ops.begin() + i, ops.end());
                    return finish(std::move(result));
                }
            }
            const int count = (int)cells.size();
            array<int, MAX_COLOR> targetCounts{};
            array<unsigned, MAX_COLOR> nestMask{};
            for (int c = 0; c < count; ++c) {
                for (int j = 0; j < target[c].size; ++j) ++targetCounts[target[c].color[j]];
                if constexpr (Large)
                    if (sim.nest[cells[c]] >= 0) nestMask[sim.nest[cells[c]]] |= 1u << c;
            }
            vector<Edge> edges;
            array<array<int, MAX_CELLS>, MAX_CELLS> distance{};
            if constexpr (Large)
                for (auto& row : distance) row.fill(INF);
            for (int a = 0; a < count; ++a) {
                if constexpr (Large) distance[a][a] = 0;
                for (int b = 0; b < count; ++b) {
                    int len = straightDistance(cells[a], cells[b]);
                    if (len == INF) continue;
                    int ar = cells[a] / sim.n, ac = cells[a] % sim.n;
                    int br = cells[b] / sim.n, bc = cells[b] % sim.n;
                    char dir = ar == br ? (ac < bc ? 'R' : 'L') : (ar < br ? 'D' : 'U');
                    edges.push_back({a, b, Op{ar, ac, len - 1, dir, len}});
                    if constexpr (Large) distance[a][b] = 1;
                }
            }
            if constexpr (Large)
                for (int middle = 0; middle < count; ++middle)
                    for (int a = 0; a < count; ++a)
                        for (int b = 0; b < count; ++b)
                            distance[a][b] = min(distance[a][b], distance[a][middle] + distance[middle][b]);
            auto scoreState = [&](const State& state) {
                if constexpr (Large) {
                    array<int, MAX_COLOR> counts{};
                    array<unsigned, MAX_COLOR> needMask{};
                    array<array<unsigned char, MAX_COLOR>, MAX_CELLS> loose{};
                    int misplaced = 0;
                    for (int c = 0; c < count; ++c) {
                        int prefix = state[c].commonPrefix(target[c]);
                        misplaced += state[c].size + target[c].size - 2 * prefix;
                        for (int j = 0; j < state[c].size; ++j) {
                            ++counts[state[c].color[j]];
                            if (j >= prefix) ++loose[c][state[c].color[j]];
                        }
                        for (int j = prefix; j < target[c].size; ++j)
                            needMask[target[c].color[j]] |= 1u << c;
                    }
                    for (int color = 0; color < K; ++color) {
                        if (counts[color] < targetCounts[color]) return INT_MAX;
                        if (counts[color] > targetCounts[color]) {
                            if (nestMask[color] == 0) return INT_MAX;
                            needMask[color] |= nestMask[color];
                        }
                    }
                    int score = 2 * misplaced;
                    for (int c = 0; c < count; ++c) {
                        int stackDistance = 0;
                        for (int color = 0; color < K; ++color) {
                            if (loose[c][color] == 0) continue;
                            int nearest = INF;
                            for (unsigned mask = needMask[color]; mask; mask &= mask - 1) {
                                int dest = __builtin_ctz(mask);
                                nearest = min(nearest, distance[c][dest]);
                            }
                            if (nearest == INF) return INT_MAX;
                            score += 4 * nearest * loose[c][color];
                            stackDistance = max(stackDistance, nearest);
                        }
                        // Shared carriage pays this stack term once, even when
                        // it carries several pieces. It guides search only.
                        score += 5 * stackDistance;
                    }
                    return score;
                } else {
                    int score = 0;
                    array<int, MAX_COLOR> counts{};
                    for (int c = 0; c < count; ++c) {
                        score += 3 * abs(state[c].size - target[c].size);
                        int prefix = state[c].commonPrefix(target[c]);
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
                }
            };
            auto hashState = [&](const State& state) {
                uint64_t hash = 1469598103934665603ULL;
                for (int c = 0; c < count; ++c) hash = state[c].appendToHash(hash);
                if constexpr (Large) return hash == 0 ? uint64_t(1) : hash;
                else return hash;
            };
            conditional_t<Large, vector<uint64_t>, unordered_set<uint64_t>> seen;
            if constexpr (Large) seen.assign(8192, 0);
            else seen.reserve(1024);
            auto firstVisit = [&](uint64_t hash) {
                if constexpr (Large) {
                    size_t slot = hash & (seen.size() - 1);
                    while (seen[slot] != 0 && seen[slot] != hash) slot = (slot + 1) & (seen.size() - 1);
                    if (seen[slot] == hash) return false;
                    seen[slot] = hash;
                    return true;
                } else {
                    return seen.insert(hash).second;
                }
            };
            vector<Node> nodes;
            nodes.reserve(MAX_NODES);
            nodes.push_back({start, -1, Op{}, scoreState(start)});
            firstVisit(hashState(start));
            vector<int> beam{0};
            int finishId = start == target ? 0 : -1, attempted = 0, expanded = 0;
            bool stop = Large && nodes[0].score == INT_MAX, windowTimedOut = false;
            for (int depth = 1; depth < selectedCount && finishId < 0 && !stop; ++depth) {
                vector<int> nextBeam;
                for (int id : beam) {
                    if constexpr (!Large) {
                        if (++expanded > 48) { stop = true; break; }
                    }
                    if (Clock::now() >= groupDeadline) {
                        windowTimedOut = true;
                        stop = true;
                        break;
                    }
                    // Child insertion must never invalidate the parent state.
                    State state = nodes[id].state;
                    for (const Edge& edge : edges) {
                        for (int kept = edge.op.len - 1; kept < state[edge.from].size; ++kept) {
                            if constexpr (Large) {
                                if (attempted >= MAX_ATTEMPTS || nodes.size() >= MAX_NODES) {
                                    stop = true;
                                    break;
                                }
                                if ((attempted & 31) == 0 && Clock::now() >= groupDeadline) {
                                    windowTimedOut = true;
                                    stop = true;
                                    break;
                                }
                                ++attempted;
                            } else {
                                if (++attempted > MAX_ATTEMPTS || nodes.size() >= MAX_NODES ||
                                    ((attempted & 31) == 0 && Clock::now() >= groupDeadline)) {
                                    stop = true;
                                    break;
                                }
                            }
                            State child = state;
                            if (transferSlimes(child[edge.from], child[edge.to], kept, edge.op.len,
                                    sim.nest[cells[edge.from]], sim.nest[cells[edge.to]]) < 0) continue;
                            int score = scoreState(child);
                            if (score == INT_MAX || !firstVisit(hashState(child))) continue;
                            Op op = edge.op;
                            op.k = kept;
                            int childId = (int)nodes.size();
                            nodes.push_back({child, id, op, score});
                            if (child == target) {
                                finishId = childId;
                                break;
                            }
                            nextBeam.push_back(childId);
                        }
                        if (stop || finishId >= 0) break;
                    }
                    if (stop || finishId >= 0) break;
                }
                if (nextBeam.empty()) break;
                auto better = [&](int a, int b) {
                    return nodes[a].score != nodes[b].score ? nodes[a].score < nodes[b].score : a < b;
                };
                retainBestNodes(nextBeam, BEAM_WIDTH, better);
                beam = std::move(nextBeam);
            }
            stats.transitions += attempted;
            if (windowTimedOut) ++stats.windowTimeouts;
            if (finishId >= 0) {
                for (int id = finishId; nodes[id].parent >= 0; id = nodes[id].parent)
                    replacement.push_back(nodes[id].op);
                reverse(replacement.begin(), replacement.end());
                bool valid = replacement.size() < (size_t)selectedCount;
                for (int c = 0; c < count; ++c) verified[cells[c]] = start[c];
                for (const Op& op : replacement)
                    if (!valid || !sim.apply(verified, op)) { valid = false; break; }
                if (valid)
                    for (int c = 0; c < count; ++c)
                        if (!(verified[cells[c]] == target[c])) { valid = false; break; }
                // Hashes prune candidates only. Acceptance requires exact
                // replay, including color order and every added connector.
                found = valid;
            }
        }
        if (found) {
            const size_t committedSize = result.size();
            result.insert(result.end(), replacement.begin(), replacement.end());
            for (int c = 0; c < (int)cells.size(); ++c) board[cells[c]] = target[c];
            size_t selectedAt = 0;
            for (int at = (int)i; at < region.end; ++at) {
                if (((at - (int)i) & 31) == 0 && Clock::now() >= deadline) {
                    stats.deadlineReached = true;
                    result.resize(committedSize);
                    result.insert(result.end(), ops.begin() + i, ops.end());
                    return finish(std::move(result));
                }
                if (selectedAt < region.selected.size() && region.selected[selectedAt] == at) {
                    ++selectedAt;
                    continue;
                }
                if (!sim.apply(board, ops[at])) return invalidOriginal();
                result.push_back(ops[at]);
            }
            ++stats.accepted;
            stats.savedOps += selectedCount - (int)replacement.size();
            stats.acceptedSpanOps += region.end - i;
            stats.acceptedMaxSpan = max(stats.acceptedMaxSpan, region.end - (int)i);
            i = region.end;
        } else {
            if (!sim.apply(board, ops[i])) return invalidOriginal();
            result.push_back(ops[i]);
            ++i;
        }
    }
    return finish(std::move(result));
}

// Small groups use the inexpensive height-and-order score on at most six cells.
vector<Op> optimizeDependencyGroups(
    const vector<string>& grid, int K, const vector<Op>& ops,
    chrono::steady_clock::time_point deadline) {
    RegionCompressionStats unusedStats;
    return compressDependencyGroupsImpl<false>(grid, K, ops, deadline, unusedStats);
}

// Larger groups may use two additional empty connectors and a distance score.
vector<Op> compressDependencyRegions(
    const vector<string>& grid, int K, const vector<Op>& ops,
    chrono::steady_clock::time_point deadline, RegionCompressionStats& stats) {
    return compressDependencyGroupsImpl<true>(grid, K, ops, deadline, stats);
}

struct DependencyRepairStats {
    int attempts = 0, accepted = 0, savedOps = 0, spanMax = 0, changedMax = 0, timeouts = 0;
    int quantityStarts = 0, quantityAccepted = 0, quantitySavedOps = 0;
    long long visited = 0, spanTotal = 0, elapsedNs = 0;
    int passes = 0, repeatSavedOps = 0;
    bool deadlineReached = false;

    void add(const DependencyRepairStats& other) {
        attempts += other.attempts;
        accepted += other.accepted;
        savedOps += other.savedOps;
        spanMax = max(spanMax, other.spanMax);
        changedMax = max(changedMax, other.changedMax);
        timeouts += other.timeouts;
        visited += other.visited;
        spanTotal += other.spanTotal;
        elapsedNs += other.elapsedNs;
        passes += other.passes;
        repeatSavedOps += other.repeatSavedOps;
        quantityStarts += other.quantityStarts;
        quantityAccepted += other.quantityAccepted;
        quantitySavedOps += other.quantitySavedOps;
        deadlineReached = deadlineReached || other.deadlineReached;
    }
};

// Delete a move or change its transferred amount, then follow affected moves.
// A trace stores each operation's two operands, not periodic board snapshots.
// Unaffected operations keep their order, however far apart the repairs are.
vector<Op> repairDependentMovesPass(
    const vector<string>& grid, int K, const vector<Op>& ops,
    chrono::steady_clock::time_point deadline, DependencyRepairStats& stats) {
    using Clock = chrono::steady_clock;
    constexpr int MAX_NODES = 512, MAX_EXPANDED = 128, BEAM_WIDTH = 16;
    struct Operands { SlimeStack beforeFrom, beforeTo, afterFrom, afterTo; };
    struct Difference { int cell; SlimeStack value, reference; };
    struct Edit { int index, parent; Op op; bool omitted; };
    struct Node {
        vector<Difference> differences;
        int cursor, edit, omitted, changed, score;
        bool quantityStart;
    };
    const auto started = Clock::now();
    stats = {};
    auto finish = [&](vector<Op> result) {
        stats.deadlineReached = Clock::now() >= deadline;
        stats.elapsedNs = chrono::duration_cast<chrono::nanoseconds>(Clock::now() - started).count();
        return result;
    };
    if (started >= deadline || ops.empty()) return finish(ops);
    SlimeBoard sim(grid, K);
    MoveDependencies dependencies(sim, ops, deadline);
    if (!dependencies.valid) return finish(ops);
    vector<Operands> trace(ops.size());
    auto board = sim.initial;
    for (int i = 0; i < (int)ops.size(); ++i) {
        if ((i & 31) == 0 && Clock::now() >= deadline) return finish(ops);
        int from = dependencies.from[i], to = dependencies.to[i];
        trace[i].beforeFrom = board[from];
        trace[i].beforeTo = board[to];
        if (!sim.apply(board, ops[i])) return finish(ops);
        trace[i].afterFrom = board[from];
        trace[i].afterTo = board[to];
    }
    board = sim.initial;
    vector<Op> result;
    result.reserve(ops.size());
    auto invalidOriginal = [&]() {
        stats.accepted = stats.savedOps = stats.spanMax = stats.changedMax = 0;
        stats.quantityAccepted = stats.quantitySavedOps = 0;
        stats.spanTotal = 0;
        return finish(ops);
    };
    auto findDifference = [](const vector<Difference>& differences, int cell) {
        return lower_bound(differences.begin(), differences.end(), cell,
            [](const Difference& entry, int value) { return entry.cell < value; });
    };
    auto setDifference = [](vector<Difference>& differences, int cell,
                            const SlimeStack& value, const SlimeStack& reference) {
        auto it = lower_bound(differences.begin(), differences.end(), cell,
            [](const Difference& entry, int position) { return entry.cell < position; });
        if (value == reference) {
            if (it != differences.end() && it->cell == cell) differences.erase(it);
        } else if (it != differences.end() && it->cell == cell) {
            it->value = value;
            it->reference = reference;
        } else {
            differences.insert(it, Difference{cell, value, reference});
        }
    };
    auto differenceScore = [](const vector<Difference>& differences) {
        int score = 4 * (int)differences.size();
        for (const Difference& entry : differences) {
            int prefix = entry.value.commonPrefix(entry.reference);
            score += entry.value.size + entry.reference.size - 2 * prefix;
        }
        return score;
    };
    auto hashNode = [](const Node& node) {
        uint64_t hash = (1469598103934665603ULL ^ (uint64_t)node.cursor) * 1099511628211ULL;
        for (const Difference& entry : node.differences) {
            hash = (hash ^ (uint64_t)(entry.cell + 1)) * 1099511628211ULL;
            hash = entry.value.appendToHash(hash);
        }
        return hash;
    };

    for (int anchor = 0; anchor < (int)ops.size();) {
        if (Clock::now() >= deadline) {
            result.insert(result.end(), ops.begin() + anchor, ops.end());
            break;
        }
        ++stats.attempts;
        const auto attemptDeadline = min(deadline, Clock::now() + chrono::microseconds(1000));
        vector<Edit> edits;
        edits.reserve(MAX_NODES);
        vector<Node> nodes;
        nodes.reserve(MAX_NODES);
        vector<int> beam;
        unordered_map<uint64_t, int> seen;
        seen.reserve(MAX_NODES * 2);
        int winner = -1;
        int expanded = 0;
        bool timedOut = false, stopped = false;
        auto addRoot = [&](bool omit, int kept) {
            if (winner >= 0 || stopped) return;
            if (Clock::now() >= attemptDeadline) { timedOut = stopped = true; return; }
            const int from = dependencies.from[anchor], to = dependencies.to[anchor];
            SlimeStack afterFrom = trace[anchor].beforeFrom, afterTo = trace[anchor].beforeTo;
            if (!omit && transferSlimes(afterFrom, afterTo, kept, ops[anchor].len,
                    sim.nest[from], sim.nest[to]) < 0) return;
            Node first{{}, anchor, (int)edits.size(), omit ? 1 : 0, 1, 0, !omit};
            setDifference(first.differences, from, afterFrom, trace[anchor].afterFrom);
            setDifference(first.differences, to, afterTo, trace[anchor].afterTo);
            // Rejoining without removing a move cannot help later operations:
            // the unchanged suffix already begins from exactly the same board.
            if (first.differences.empty() && first.omitted == 0) return;
            first.score = differenceScore(first.differences);
            const uint64_t hash = hashNode(first);
            auto visited = seen.find(hash);
            if (visited != seen.end() && visited->second >= first.omitted) return;
            seen[hash] = first.omitted;
            Op replacement = ops[anchor];
            replacement.k = kept;
            edits.push_back({anchor, -1, replacement, omit});
            const int rootId = (int)nodes.size();
            nodes.push_back(std::move(first));
            if (!omit) ++stats.quantityStarts;
            if (nodes.back().differences.empty()) winner = rootId;
            else beam.push_back(rootId);
        };
        addRoot(true, ops[anchor].k);
        for (int kept = ops[anchor].len - 1; kept < trace[anchor].beforeFrom.size; ++kept)
            if (kept != ops[anchor].k) addRoot(false, kept);
        while (winner < 0 && !beam.empty() && !stopped) {
            vector<int> nextBeam;
            for (int id : beam) {
                if (Clock::now() >= attemptDeadline) {
                    timedOut = stopped = true;
                    break;
                }
                if (++expanded > MAX_EXPANDED || nodes.size() >= MAX_NODES) {
                    stopped = true;
                    break;
                }
                ++stats.visited;
                // Keep a copy: inserting child nodes may invalidate references.
                const Node parent = nodes[id];
                auto bestKnown = seen.find(hashNode(parent));
                if (bestKnown != seen.end() && bestKnown->second > parent.omitted) continue;
                int at = (int)ops.size();
                bool canRejoin = true;
                for (const Difference& entry : parent.differences) {
                    int next = dependencies.nextUse(entry.cell, parent.cursor);
                    // No allowed repair can alter a cell after its final use.
                    if (next == (int)ops.size()) { canRejoin = false; break; }
                    at = min(at, next);
                }
                if (!canRejoin || at == (int)ops.size()) continue;
                const int from = dependencies.from[at], to = dependencies.to[at];
                SlimeStack actualFrom = trace[at].beforeFrom, actualTo = trace[at].beforeTo;
                auto fromIt = findDifference(parent.differences, from);
                auto toIt = findDifference(parent.differences, to);
                if (fromIt != parent.differences.end() && fromIt->cell == from) actualFrom = fromIt->value;
                if (toIt != parent.differences.end() && toIt->cell == to) actualTo = toIt->value;

                auto addChild = [&](bool omit, int kept) {
                    if (winner >= 0 || stopped) return;
                    if (nodes.size() >= MAX_NODES) { stopped = true; return; }
                    if (Clock::now() >= attemptDeadline) { timedOut = stopped = true; return; }
                    SlimeStack afterFrom = actualFrom, afterTo = actualTo;
                    if (!omit && transferSlimes(afterFrom, afterTo, kept, ops[at].len,
                            sim.nest[from], sim.nest[to]) < 0) return;
                    Node child = parent;
                    child.cursor = at;
                    setDifference(child.differences, from, afterFrom, trace[at].afterFrom);
                    setDifference(child.differences, to, afterTo, trace[at].afterTo);
                    const bool changed = omit || kept != ops[at].k;
                    child.omitted += omit;
                    child.changed += changed;
                    if (child.differences.empty() && child.omitted == 0) return;
                    child.score = differenceScore(child.differences);
                    const uint64_t hash = hashNode(child);
                    auto visited = seen.find(hash);
                    // Hash collisions can only discard a candidate; exact
                    // replay below, never this table, authorizes a replacement.
                    if (visited != seen.end() && visited->second >= child.omitted) return;
                    seen[hash] = child.omitted;
                    if (changed) {
                        Op replacement = ops[at];
                        replacement.k = kept;
                        child.edit = (int)edits.size();
                        edits.push_back({at, parent.edit, replacement, omit});
                    }
                    int childId = (int)nodes.size();
                    nodes.push_back(std::move(child));
                    if (nodes.back().differences.empty()) winner = childId;
                    else nextBeam.push_back(childId);
                };
                addChild(false, ops[at].k);
                addChild(true, ops[at].k);
                for (int kept = ops[at].len - 1; kept < actualFrom.size; ++kept)
                    if (kept != ops[at].k) addChild(false, kept);
                if (winner >= 0 || stopped) break;
            }
            auto better = [&](int a, int b) {
                if (nodes[a].score != nodes[b].score) return nodes[a].score < nodes[b].score;
                if (nodes[a].changed != nodes[b].changed) return nodes[a].changed < nodes[b].changed;
                if (nodes[a].omitted != nodes[b].omitted) return nodes[a].omitted > nodes[b].omitted;
                return a < b;
            };
            retainBestNodes(nextBeam, BEAM_WIDTH, better);
            beam = std::move(nextBeam);
        }

        bool accepted = false;
        if (winner >= 0) {
            vector<int> chosenEdits;
            for (int edit = nodes[winner].edit; edit >= 0; edit = edits[edit].parent)
                chosenEdits.push_back(edit);
            reverse(chosenEdits.begin(), chosenEdits.end());
            const int end = nodes[winner].cursor;
            auto verified = board, expected = board;
            vector<Op> replacement;
            replacement.reserve(end - anchor + 1);
            size_t nextEdit = 0;
            bool valid = true;
            for (int at = anchor; at <= end; ++at) {
                if ((at - anchor) % 16 == 0 && Clock::now() >= attemptDeadline) {
                    timedOut = true;
                    valid = false;
                    break;
                }
                expected[dependencies.from[at]] = trace[at].afterFrom;
                expected[dependencies.to[at]] = trace[at].afterTo;
                const Op* op = &ops[at];
                if (nextEdit < chosenEdits.size() && edits[chosenEdits[nextEdit]].index == at) {
                    const Edit& edit = edits[chosenEdits[nextEdit++]];
                    if (edit.omitted) continue;
                    op = &edit.op;
                }
                if (!sim.apply(verified, *op)) { valid = false; break; }
                replacement.push_back(*op);
            }
            if (valid && nextEdit == chosenEdits.size() &&
                replacement.size() < (size_t)(end - anchor + 1) && verified == expected) {
                result.insert(result.end(), replacement.begin(), replacement.end());
                board = std::move(expected);
                ++stats.accepted;
                const int saved = end - anchor + 1 - (int)replacement.size();
                stats.savedOps += saved;
                if (nodes[winner].quantityStart) {
                    ++stats.quantityAccepted;
                    stats.quantitySavedOps += saved;
                }
                stats.spanTotal += end - anchor + 1;
                stats.spanMax = max(stats.spanMax, end - anchor + 1);
                stats.changedMax = max(stats.changedMax, nodes[winner].changed);
                anchor = end + 1;
                accepted = true;
            }
        }
        if (timedOut) ++stats.timeouts;
        if (!accepted) {
            if (!sim.apply(board, ops[anchor])) return invalidOriginal();
            result.push_back(ops[anchor]);
            ++anchor;
        }
    }
    return finish(std::move(result));
}

// Rebuild endpoint traces from the shortened output on every pass. A successful
// long repair may have skipped unrelated anchors inside its accepted interval.
vector<Op> repairDependentMoves(
    const vector<string>& grid, int K, const vector<Op>& ops,
    chrono::steady_clock::time_point deadline, DependencyRepairStats& stats) {
    using Clock = chrono::steady_clock;
    const auto started = Clock::now();
    stats = {};
    vector<Op> current = ops;
    while (!current.empty() && Clock::now() < deadline) {
        DependencyRepairStats passStats;
        auto shortened = repairDependentMovesPass(grid, K, current, deadline, passStats);
        if (stats.passes > 0) stats.repeatSavedOps += passStats.savedOps;
        ++stats.passes;
        stats.add(passStats);
        // Each pass returns only verified, strictly shorter replacements.
        // Without a reduction another pass would repeat the same search.
        if (shortened.size() >= current.size()) break;
        current = std::move(shortened);
    }
    stats.deadlineReached = stats.deadlineReached || Clock::now() >= deadline;
    stats.elapsedNs = chrono::duration_cast<chrono::nanoseconds>(Clock::now() - started).count();
    return current;
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
    return answer;
}

// Keep the cooperative evaluator: batches may wait for arrivals, preserve
// springboards and carry mixed colors along shared routes.
struct CooperativeScheduler {
    using Clock = chrono::steady_clock;
    using Stack = SlimeStack;
    struct State {
        vector<Stack> board;
        int remaining = 0;
        int occupiedCount = 0;
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
        vector<Candidate> alternatives;
    };
    static constexpr int AMOUNT_ALTERNATIVES = 6;
    static constexpr int ROUTE_ALTERNATIVES = 6;
    static constexpr int NEARBY_ROLE_ALTERNATIVES = 4;
    static constexpr int REMOTE_BEST_ALTERNATIVES = 4;
    static constexpr int REMOTE_SAMPLE_ALTERNATIVES = 4;
    static constexpr int MAX_ALTERNATIVES = AMOUNT_ALTERNATIVES + ROUTE_ALTERNATIVES +
        3 * NEARBY_ROLE_ALTERNATIVES + REMOTE_BEST_ALTERNATIVES + REMOTE_SAMPLE_ALTERNATIVES;
    static_assert(MAX_ALTERNATIVES < 64);
    struct Forward {
        int target;
        uint8_t length, direction; // Length 1..8, direction 0..3.
        int gain; // Cell IDs and tree-depth gains can exceed 255.
    };
    static_assert(sizeof(Forward) == 12);
    struct ProgressMask { uint16_t strict = 0, nonworsening = 0; };
    struct TreeChildren { array<int, 4> cell{}; int size = 0; };
    struct SupportInput { int incoming = -1, distance = -1, arrival = -1; };

    // Rows are appended in order during construction, then accessed as spans.
    // Both the elements and row boundaries occupy single contiguous vectors.
    template<class T>
    struct FlatLists {
        vector<T> values;
        vector<int> offsets;

        explicit FlatLists(int rows) : offsets(rows + 1) {}
        span<T> operator[](int row) {
            return span<T>(values).subspan(offsets[row], offsets[row + 1] - offsets[row]);
        }
        span<const T> operator[](int row) const {
            return span<const T>(values).subspan(offsets[row], offsets[row + 1] - offsets[row]);
        }
    };

    const vector<string>& grid;
    int n, K, cells;
    // Fixed tables use color-major rows: color * cells + cell.
    vector<int> parent, depth;
    vector<uint8_t> run; // Straight-run lengths: 0..8.
    vector<int> sharedPath, orderIndex;
    FlatLists<int> order;
    vector<TreeChildren> children;
    vector<ProgressMask> progressMask;
    vector<array<int, 8>> jump, reachableGain;
    vector<array<array<int, 8>, 4>> ray;
    vector<signed char> moveInfo;
    vector<int> cellRow, cellColumn;
    FlatLists<Forward> treeForward, shortcutForward, shortcutExposure;
    vector<int> nest;
    State initial, state;
    vector<int8_t> topColor; // -1 for empty, otherwise 0..K-1.
    vector<uint8_t> topCount, batchCount; // 0..8.
    vector<int> waitTarget;
    vector<uint8_t> waitState; // DFS states: 0..2.
    vector<uint8_t> wholeCanMove, waiting; // Byte flags avoid bit read-modify-write.
    vector<double> sharedPotential;
    vector<uint8_t> local; // Per-cell color counts: 0..8.
    vector<int> pending, nearest, arrivalDistance;
    vector<uint8_t> arrivalSize, nearestSize; // Exposed batch sizes: 1..8.
    vector<double> traffic;
    vector<array<double, 9>> support;
    vector<array<double, 9>> supportByColor;
    vector<SupportInput> supportInput;
    vector<double> originalExposure;
    vector<Stack> cachedBoard;
    vector<double> arrivalSharedValue;
    vector<uint8_t> arrivalHomedSize;
    vector<unsigned> arrivalCachedColors;
    vector<int> batchPrefix;
    vector<unsigned char> originalOnTree;
    vector<unsigned> supportAt, exposureAt;
    vector<int> activeCells, dirtySupportCells;
    array<uint64_t, 7> occupied{};
    bool boardCacheReady = false, cachedShortcuts = false;
    uint64_t cachedRevision = UINT64_MAX;
    mutable uint64_t nextRevision = 0;
    unsigned decisionGeneration = 0;

    int cellIndex(int color, int v) const { return color * cells + v; }

    CooperativeScheduler(const vector<string>& grid_, int K_, const vector<RouteTree>& routes)
        : grid(grid_), n((int)grid.size()), K(K_), cells(n * n),
          parent(K * cells), depth(K * cells), run(K * cells), sharedPath(K * K * cells),
          orderIndex(K * cells, -1), order(K), children(K * cells),
          progressMask(cells * cells), jump(K * cells), reachableGain(K * cells), ray(cells),
          moveInfo(cells * cells, -1), cellRow(cells), cellColumn(cells),
          treeForward(K * cells), shortcutForward(K * cells), shortcutExposure(K * cells),
          topColor(cells, -1), topCount(cells), batchCount(cells), waitTarget(cells), waitState(cells),
          wholeCanMove(cells), waiting(cells), sharedPotential(cells),
          local(K * cells), pending(K * cells), nearest(K * cells, cells + 1),
          arrivalDistance(nearest), arrivalSize(K * cells, 1), nearestSize(arrivalSize),
          traffic(K * cells), support(cells), supportByColor(K * cells),
          supportInput(K * cells), originalExposure(cells),
          cachedBoard(cells), arrivalSharedValue(K * cells), arrivalHomedSize(K * cells),
          arrivalCachedColors(cells), batchPrefix(cells), originalOnTree(cells),
          supportAt(cells), exposureAt(cells) {
        activeCells.reserve(cells);
        dirtySupportCells.reserve(cells);
        SlimeBoard board(grid, K);
        initial.board = std::move(board.initial);
        nest = std::move(board.nest);
        for (const auto& stack : initial.board) {
            initial.remaining += stack.size;
            initial.occupiedCount += stack.size > 0;
        }
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
            copy(routes[color].parent.begin(), routes[color].parent.end(), parent.begin() + color * cells);
            copy(routes[color].depth.begin(), routes[color].depth.end(), depth.begin() + color * cells);
            for (int v = 0; v < cells; ++v) {
                int index = cellIndex(color, v);
                // Record empty rows as well, including walls and off-tree cells.
                treeForward.offsets[index] = (int)treeForward.values.size();
                shortcutForward.offsets[index] = (int)shortcutForward.values.size();
                jump[cellIndex(color, v)].fill(-1);
                if (depth[cellIndex(color, v)] < 0) continue;
                order.values.push_back(v);
                int p = parent[cellIndex(color, v)];
                if (p >= 0) {
                    int d = p / n < v / n ? 0 : p / n > v / n ? 1 : p % n < v % n ? 2 : 3;
                    int at = v;
                    for (int len = 1; len <= 8; ++len) {
                        int next = parent[cellIndex(color, at)];
                        if (next < 0 || next / n - at / n != dr[d] ||
                            next % n - at % n != dc[d]) break;
                        jump[cellIndex(color, v)][len - 1] = next;
                        run[cellIndex(color, v)] = len;
                        treeForward.values.push_back({next, static_cast<uint8_t>(len),
                                                      static_cast<uint8_t>(d), len});
                        at = next;
                    }
                }
                for (int d = 0; d < 4; ++d) {
                    for (int len = 1; len <= 8; ++len) {
                        int target = ray[v][d][len - 1];
                        if (target < 0) break;
                        if (depth[cellIndex(color, target)] < 0 || depth[cellIndex(color, target)] >= depth[cellIndex(color, v)]) continue;
                        int gain = depth[cellIndex(color, v)] - depth[cellIndex(color, target)];
                        shortcutForward.values.push_back({target, static_cast<uint8_t>(len),
                                                          static_cast<uint8_t>(d), gain});
                        for (int kept = len - 1; kept < 8; ++kept)
                            reachableGain[cellIndex(color, v)][kept] = max(reachableGain[cellIndex(color, v)][kept], gain);
                    }
                }
            }
            order.offsets[color + 1] = (int)order.values.size();
            sort(order[color].begin(), order[color].end(),
                 [&](int a, int b) { return depth[cellIndex(color, a)] > depth[cellIndex(color, b)]; });
            // Strict '<' arrival ties originally followed this order. Retain
            // it when recomputing a parent directly from its children.
            for (int i = 0; i < (int)order[color].size(); ++i) {
                int v = order[color][i], p = parent[cellIndex(color, v)];
                orderIndex[cellIndex(color, v)] = i;
                if (p >= 0) {
                    auto& child = children[cellIndex(color, p)];
                    assert(child.size < (int)child.cell.size());
                    child.cell[child.size++] = v;
                }
            }
            for (int source = 0; source < cells; ++source) {
                for (int target = 0; target < cells; ++target) {
                    if (depth[cellIndex(color, target)] < 0) continue;
                    auto& mask = progressMask[source * cells + target];
                    if (depth[cellIndex(color, target)] <= depth[cellIndex(color, source)])
                        mask.nonworsening |= 1u << color;
                    if (depth[cellIndex(color, target)] < depth[cellIndex(color, source)])
                        mask.strict |= 1u << color;
                }
            }
        }
        treeForward.offsets.back() = (int)treeForward.values.size();
        shortcutForward.offsets.back() = (int)shortcutForward.values.size();
        // Exposure needs only the greatest feasible gain. Keep a separate
        // ordering so candidate serials and tie-breaking stay unchanged.
        shortcutExposure = shortcutForward;
        for (int index = 0; index < K * cells; ++index) {
            auto steps = shortcutExposure[index];
            sort(steps.begin(), steps.end(),
                 [](const Forward& a, const Forward& b) { return a.gain > b.gain; });
        }
        for (int a = 0; a < K; ++a) {
            for (int b = a + 1; b < K; ++b) {
                auto length = span<int>(sharedPath).subspan((a * K + b) * cells, cells);
                auto reverseLength = span<int>(sharedPath).subspan((b * K + a) * cells, cells);
                for (auto it = order[a].rbegin(); it != order[a].rend(); ++it) {
                    int v = *it, p = parent[cellIndex(a, v)];
                    if (p >= 0 && parent[cellIndex(b, v)] == p)
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
    // Keep distinct ways to edit the focused action, including SA-only
    // springboard loans. The final union contains at most 6 + 6 + 12 + 8.
    struct AlternativeCollector {
        const vector<Stack>& board;
        Candidate focus;
        uint64_t salt = 0;
        vector<Candidate> amounts, routes, remoteBest;
        array<vector<Candidate>, 3> nearby;
        struct Sample {
            Candidate action;
            uint64_t sourceKey, actionKey;
        };
        vector<Sample> remoteSample;

        uint64_t actionKey(const Candidate& action) const {
            uint64_t payload = uint64_t(action.count) | (uint64_t(action.secondCount) << 4) |
                               (uint64_t(action.length) << 8) | (uint64_t(action.secondLength) << 12);
            return mixHash(salt ^ uint64_t(action.source + 1) * 0x9e3779b97f4a7c15ULL ^
                       uint64_t(action.target + 1) * 0xbf58476d1ce4e5b9ULL ^
                       uint64_t(action.secondTarget + 1) * 0x94d049bb133111ebULL ^ payload);
        }
        AlternativeCollector(const vector<Stack>& board, const Candidate& focus)
            : board(board), focus(focus) {
            // A stable board/action salt makes sampling independent of scores,
            // candidate enumeration order, and the number of actions per source.
            salt = actionKey(focus);
            for (int v = 0; v < (int)board.size(); ++v) {
                const Stack& stack = board[v];
                if (stack.size == 0) continue;
                salt ^= stack.positionHash(v);
            }
            amounts.reserve(16);
            routes.reserve(ROUTE_ALTERNATIVES + 1);
            for (auto& role : nearby) role.reserve(NEARBY_ROLE_ALTERNATIVES + 1);
            remoteBest.reserve(REMOTE_BEST_ALTERNATIVES + 1);
            remoteSample.reserve(REMOTE_BEST_ALTERNATIVES + REMOTE_SAMPLE_ALTERNATIVES + 1);
        }
        static bool sameRoute(const Candidate& a, const Candidate& b) {
            return a.source == b.source && a.target == b.target && a.length == b.length &&
                   a.secondTarget == b.secondTarget && a.secondLength == b.secondLength;
        }
        static bool sameSource(const Candidate& a, const Candidate& b) {
            return a.source == b.source;
        }
        template<class SameGroup>
        static void collect(vector<Candidate>& pool, const Candidate& action, int limit,
                            SameGroup sameGroup) {
            for (auto it = pool.begin(); it != pool.end(); ++it) {
                if (!sameGroup(*it, action)) continue;
                if (!better(action, *it)) return;
                pool.erase(it);
                break;
            }
            if ((int)pool.size() >= limit && !better(action, pool.back())) return;
            auto at = lower_bound(pool.begin(), pool.end(), action, better);
            pool.insert(at, action);
            if ((int)pool.size() > limit) pool.pop_back();
        }
        void sampleRemote(const Candidate& action) {
            for (Sample& sample : remoteSample) {
                if (sample.action.source != action.source) continue;
                uint64_t key = actionKey(action);
                if (key < sample.actionKey) {
                    sample.action = action;
                    sample.actionKey = key;
                }
                return;
            }
            uint64_t key = mixHash(salt ^ uint64_t(action.source + 1) * 0xd6e8feb86659fd93ULL);
            auto order = [](const Sample& a, const Sample& b) {
                return pair{a.sourceKey, a.action.source} < pair{b.sourceKey, b.action.source};
            };
            Sample sample{action, key, 0};
            // Keep eight sources: excluding up to four score-selected sources
            // at finalization still leaves four independent sample sources.
            constexpr int capacity = REMOTE_BEST_ALTERNATIVES + REMOTE_SAMPLE_ALTERNATIVES;
            if ((int)remoteSample.size() >= capacity && !order(sample, remoteSample.back())) return;
            sample.actionKey = actionKey(action);
            remoteSample.insert(lower_bound(remoteSample.begin(), remoteSample.end(), sample, order), sample);
            if ((int)remoteSample.size() > capacity) remoteSample.pop_back();
        }
        void add(const Candidate& action) {
            if (sameAction(action, focus)) return;
            if (action.source == focus.source) {
                if (sameRoute(action, focus)) {
                    for (const Candidate& old : amounts)
                        if (sameAction(old, action)) return;
                    amounts.push_back(action);
                } else {
                    collect(routes, action, ROUTE_ALTERNATIVES, sameRoute);
                }
                return;
            }
            auto focusDestination = [&](int v) {
                return v >= 0 && (v == focus.target || v == focus.secondTarget);
            };
            bool intoSource = action.target == focus.source || action.secondTarget == focus.source;
            bool outOfDestination = focusDestination(action.source) ||
                                    (action.secondTarget >= 0 && focusDestination(action.target));
            bool intoDestination = focusDestination(action.target) || focusDestination(action.secondTarget);
            if (intoSource) collect(nearby[0], action, NEARBY_ROLE_ALTERNATIVES, sameSource);
            // Departures have only one or two focused source cells; preserve
            // different routes here instead of allowing just one per source.
            if (outOfDestination) collect(nearby[1], action, NEARBY_ROLE_ALTERNATIVES, sameRoute);
            if (intoDestination) collect(nearby[2], action, NEARBY_ROLE_ALTERNATIVES, sameSource);
            if (intoSource || outOfDestination || intoDestination) return;
            collect(remoteBest, action, REMOTE_BEST_ALTERNATIVES, sameSource);
            sampleRemote(action);
        }
        vector<Candidate> finish() const {
            vector<Candidate> result;
            result.reserve(MAX_ALTERNATIVES);
            auto contains = [&](const Candidate& action) {
                return any_of(result.begin(), result.end(), [&](const Candidate& old) {
                    return sameAction(old, action);
                });
            };
            auto append = [&](const Candidate& action) {
                if (!contains(action)) result.push_back(action);
            };
            auto takeAmount = [&](auto eligible, auto key) {
                const Candidate* best = nullptr;
                for (const Candidate& action : amounts) {
                    if (!eligible(action) || contains(action)) continue;
                    if (!best || key(action) < key(*best) ||
                        (key(action) == key(*best) && better(action, *best))) best = &action;
                }
                if (!best) return false;
                append(*best);
                return true;
            };
            auto anyAmount = [](const Candidate&) { return true; };
            auto scoreOnly = [](const Candidate&) { return pair{0, 0}; };
            // Score cannot crowd out small/large payloads or an entire color
            // block. Both payload counts matter for a two-step action.
            takeAmount(anyAmount, [](const Candidate& action) {
                return pair{action.count, action.secondCount};
            });
            takeAmount(anyAmount, [](const Candidate& action) {
                return pair{-action.count, -action.secondCount};
            });
            takeAmount([&](const Candidate& action) {
                const Stack& stack = board[action.source];
                int kept = stack.size - action.count;
                return kept == 0 || stack.color[kept - 1] != stack.color[kept];
            }, scoreOnly);
            takeAmount(anyAmount, [&](const Candidate& action) {
                return pair{abs(action.count - focus.count) + abs(action.secondCount - focus.secondCount),
                            abs(action.count - focus.count)};
            });
            while ((int)result.size() < AMOUNT_ALTERNATIVES && takeAmount(anyAmount, scoreOnly)) {}
            for (const Candidate& action : routes) append(action);
            for (const auto& role : nearby)
                for (const Candidate& action : role) append(action);
            for (const Candidate& action : remoteBest) append(action);
            int sampled = 0;
            for (const Sample& sample : remoteSample) {
                if (any_of(remoteBest.begin(), remoteBest.end(), [&](const Candidate& action) {
                    return action.source == sample.action.source;
                })) continue;
                append(sample.action);
                if (++sampled == REMOTE_SAMPLE_ALTERNATIVES) break;
            }
            assert(result.size() <= MAX_ALTERNATIVES);
            return result;
        }
    };
    int sharedPrefix(int v, const Stack& stack, int count) const {
        int reference = stack.color[stack.size - 1];
        int length = depth[cellIndex(reference, v)];
        for (int j = stack.size - count; j < stack.size && length > 0; ++j) {
            int color = stack.color[j];
            if (color != reference)
                length = min(length, sharedPath[cellIndex(reference * K + color, v)]);
        }
        return max(0, length);
    }

    double sharedValue(int v, const Stack& stack) const {
        if (stack.size < 2) return 0.0;
        int reference = stack.color[stack.size - 1];
        int length = depth[cellIndex(reference, v)], blocks = 0, previous = -1;
        double best = 0.0;
        for (int j = stack.size - 1; j >= 0 && length > 0; --j) {
            int color = stack.color[j];
            if (color != reference)
                length = min(length, sharedPath[cellIndex(reference * K + color, v)]);
            if (color != previous) ++blocks;
            previous = color;
            // Estimate future departures saved by keeping a mixed top suffix
            // together. A short horizon limits speculative merge rewards.
            best = max(best, (double)(blocks - 1) * min(6, length));
        }
        return best;
    }

    void cacheMonochromeArrival(int v, int color) {
        const auto& board = state.board;
        const unsigned bit = 1u << color;
        if (arrivalCachedColors[v] & bit) return;
        // Shared value depends on color blocks, not their multiplicity.
        // Appending one slime gives the same value as any positive count;
        // at its nest, the whole appended block is removed either way.
        Stack arrived = board[v];
        assert(arrived.size < 8);
        arrived.color[arrived.size++] = color;
        arrived.home(nest[v]);
        int index = cellIndex(color, v);
        arrivalSharedValue[index] = sharedValue(v, arrived);
        arrivalHomedSize[index] = arrived.size;
        arrivalCachedColors[v] |= bit;
    }

    int batchSize(int v, const Stack& stack) const {
        if (stack.size == 0) return 0;
        int next = parent[cellIndex(stack.color[stack.size - 1], v)];
        if (next < 0) return 0;
        int count = 0;
        while (count < stack.size &&
               parent[cellIndex(stack.color[stack.size - 1 - count], v)] == next)
            ++count;
        return count;
    }

    bool batchCanMove(int v, const Stack& stack, int count, const auto& read) const {
        if (count == 0) return false;
        int color = stack.color[stack.size - 1];
        int maximum = min<int>({run[cellIndex(color, v)], sharedPrefix(v, stack, count),
                                stack.size - count + 1});
        for (int len = 1; len <= maximum; ++len) {
            int target = jump[cellIndex(color, v)][len - 1];
            if (read(target).size + count <= 8) return true;
        }
        return false;
    }

    bool canJoin(int v, const Stack& moving, int count, const Stack& resident) const {
        if (resident.size == 0 || resident.size + count > 8) return false;
        int next = parent[cellIndex(resident.color[resident.size - 1], v)];
        if (next < 0) return false;
        for (int j = moving.size - count; j < moving.size; ++j)
            if (parent[cellIndex(moving.color[j], v)] != next) return false;
        return true;
    }

    double colorSupport(int color, int v, int height) const {
        return traffic[cellIndex(color, v)] == 0.0 ? 0.0 : supportByColor[cellIndex(color, v)][height];
    }

    double foreignSupport(int color, int v, int height) const {
        return support[v][height] - colorSupport(color, v, height);
    }

    bool canJoinMonochrome(int v, int color, int count) const {
        const auto& board = state.board;
        if (topColor[v] < 0 || board[v].size + count > 8) return false;
        int next = parent[cellIndex(topColor[v], v)];
        return next >= 0 && parent[cellIndex(color, v)] == next;
    }

    void refreshWaitingDependencies() {
        const auto& board = state.board;
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
            int maximum = min<int>({run[cellIndex(color, source)], length, stack.size - count + 1});
            for (int len = 1; len <= maximum; ++len) {
                int target = jump[cellIndex(color, source)][len - 1];
                if (board[target].size + count <= 8) {
                    wholeCanMove[source] = true;
                    break;
                }
            }
            for (int step = 0; step < length; ++step) {
                v = parent[cellIndex(color, v)];
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
    }

    void prepareDecisionCache(bool allowShortcuts) {
        const auto& board = state.board;
        // Consecutive repair steps expose their two or three changed cells.
        // Restored histories fall back to comparing the full cached board.
        const bool refreshSupport = !boardCacheReady || cachedShortcuts != allowShortcuts;
        unsigned dirtyColors = boardCacheReady ? 0u : (1u << K) - 1;
        array<array<uint64_t, 7>, MAX_COLOR> dirtyArrivals{};
        array<int, MAX_COLOR> dirtyArrivalCount{};
        auto markArrivalDirty = [&](int color, int v) {
            int index = orderIndex[cellIndex(color, v)];
            if (index < 0) {
                // Off-tree cells have no children or outgoing tree edge.
                // Keep their local values consistent after history restores.
                const Stack& stack = board[v];
                bool exposed = stack.size > 0 && stack.color[stack.size - 1] == color;
                pending[cellIndex(color, v)] = local[cellIndex(color, v)];
                nearest[cellIndex(color, v)] = exposed ? 0 : cells + 1;
                nearestSize[cellIndex(color, v)] = exposed ? stack.topCount() : 1;
                return;
            }
            dirtyColors |= 1u << color;
            uint64_t bit = uint64_t(1) << (index & 63);
            auto& word = dirtyArrivals[color][index >> 6];
            if (!(word & bit)) {
                word |= bit;
                ++dirtyArrivalCount[color];
            }
        };
        auto updateCell = [&](int v) {
            const Stack& stack = board[v];
            if (boardCacheReady && stack == cachedBoard[v]) return;
            // Routes are fixed, so arrivals remain reusable across decisions,
            // models, and option masks until this destination stack changes.
            arrivalCachedColors[v] = 0;
            const Stack& old = cachedBoard[v];
            array<int, MAX_COLOR> difference{};
            unsigned presentColors = 0;
            for (int j = 0; j < old.size; ++j) {
                int color = old.color[j];
                --local[cellIndex(color, v)];
                --difference[color];
                presentColors |= 1u << color;
            }
            originalOnTree[v] = true;
            for (int j = 0; j < stack.size; ++j) {
                int color = stack.color[j];
                ++local[cellIndex(color, v)];
                ++difference[color];
                presentColors |= 1u << color;
                if (depth[cellIndex(color, v)] < 0) originalOnTree[v] = false;
            }
            for (unsigned mask = presentColors; mask; mask &= mask - 1) {
                int color = __builtin_ctz(mask);
                if (difference[color] != 0) markArrivalDirty(color, v);
            }
            sharedPotential[v] = sharedValue(v, stack);
            int color = stack.size > 0 ? stack.color[stack.size - 1] : -1;
            int count = stack.topCount();
            if (topColor[v] != color || topCount[v] != count) {
                if (topColor[v] >= 0) markArrivalDirty(topColor[v], v);
                if (color >= 0) markArrivalDirty(color, v);
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

        refreshWaitingDependencies();

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
        auto updateSupport = [&](int color, int v) {
            int incoming = pending[cellIndex(color, v)] - local[cellIndex(color, v)];
            int distance = arrivalDistance[cellIndex(color, v)], arrival = arrivalSize[cellIndex(color, v)];
            auto& previous = supportInput[cellIndex(color, v)];
            bool sameArrival = previous.arrival == arrival;
            if (!refreshSupport && previous.incoming == incoming &&
                previous.distance == distance && sameArrival) return;
            previous = {incoming, distance, arrival};
            bool useful = allowShortcuts ?
                reachableGain[cellIndex(color, v)][7] > reachableGain[cellIndex(color, v)][0] : run[cellIndex(color, v)] >= 2;
            if (incoming == 0 || distance > cells || !useful) {
                if (traffic[cellIndex(color, v)] != 0.0) {
                    traffic[cellIndex(color, v)] = 0.0;
                    markSupportDirty(v);
                }
                return;
            }
            double batches = incoming >= 2 * arrival ? 2.0 : SMALL_RATIO[incoming][arrival];
            double nextTraffic = batches / (1.0 + 0.5 * distance);
            // For a fixed route mode, every height depends only on traffic
            // and arrival size. Saturation can preserve both despite changed
            // inputs. Compare exactly; keep the updated raw inputs above.
            if (!refreshSupport && sameArrival && nextTraffic == traffic[cellIndex(color, v)]) return;
            traffic[cellIndex(color, v)] = nextTraffic;
            markSupportDirty(v);
            auto& values = supportByColor[cellIndex(color, v)];
            values[0] = values[8] = 0.0;
            for (int height = 1; height < 8; ++height) {
                int saved = allowShortcuts ?
                    reachableGain[cellIndex(color, v)][height] - reachableGain[cellIndex(color, v)][0] :
                    min(height, run[cellIndex(color, v)] - 1);
                double fit = min(1.0, SMALL_RATIO[8 - height][arrival]);
                values[height] = traffic[cellIndex(color, v)] * saved * fit;
            }
        };
        for (int color = 0; color < K; ++color) {
            bool dirty = (dirtyColors & (1u << color)) != 0;
            if (!dirty && !refreshSupport) continue;
            // Broad history restores are cheaper with a linear tree pass.
            // Normal repair steps update only changed nodes and ancestors.
            bool rebuild = refreshSupport ||
                dirtyArrivalCount[color] >= max(8, (int)order[color].size() / 4);
            if (rebuild) {
                fill_n(nearest.begin() + color * cells, cells, cells + 1);
                fill_n(nearestSize.begin() + color * cells, cells, 1);
                fill_n(arrivalDistance.begin() + color * cells, cells, cells + 1);
                fill_n(arrivalSize.begin() + color * cells, cells, 1);
                for (int v : activeCells) {
                    if (topColor[v] != color) continue;
                    nearest[cellIndex(color, v)] = 0;
                    nearestSize[cellIndex(color, v)] = topCount[v];
                }
                copy_n(local.begin() + color * cells, cells, pending.begin() + color * cells);
                for (int v : order[color]) {
                    int p = parent[cellIndex(color, v)];
                    if (p < 0) continue;
                    pending[cellIndex(color, p)] += pending[cellIndex(color, v)];
                    int distance = nearest[cellIndex(color, v)] + 1;
                    if (distance < arrivalDistance[cellIndex(color, p)]) {
                        arrivalDistance[cellIndex(color, p)] = distance;
                        arrivalSize[cellIndex(color, p)] = nearestSize[cellIndex(color, v)];
                    }
                    if (distance < nearest[cellIndex(color, p)]) {
                        nearest[cellIndex(color, p)] = distance;
                        nearestSize[cellIndex(color, p)] = nearestSize[cellIndex(color, v)];
                    }
                }
                for (int v : order[color]) updateSupport(color, v);
                continue;
            }
            auto& queued = dirtyArrivals[color];
            int words = ((int)order[color].size() + 63) / 64;
            for (int word = 0; word < words; ++word) {
                while (queued[word]) {
                    int index = word * 64 + __builtin_ctzll(queued[word]);
                    queued[word] &= queued[word] - 1;
                    int v = order[color][index];
                    int mass = local[cellIndex(color, v)], distance = cells + 1, arrival = 1;
                    const auto& child = children[cellIndex(color, v)];
                    for (int i = 0; i < child.size; ++i) {
                        int u = child.cell[i];
                        mass += pending[cellIndex(color, u)];
                        int nextDistance = nearest[cellIndex(color, u)] + 1;
                        if (nextDistance < distance) {
                            distance = nextDistance;
                            arrival = nearestSize[cellIndex(color, u)];
                        }
                    }
                    bool exposed = topColor[v] == color;
                    int nextNearest = exposed ? 0 : distance;
                    int nextSize = exposed ? topCount[v] : arrival;
                    bool changed = mass != pending[cellIndex(color, v)] ||
                        nextNearest != nearest[cellIndex(color, v)] || nextSize != nearestSize[cellIndex(color, v)];
                    pending[cellIndex(color, v)] = mass;
                    nearest[cellIndex(color, v)] = nextNearest;
                    nearestSize[cellIndex(color, v)] = nextSize;
                    arrivalDistance[cellIndex(color, v)] = distance;
                    arrivalSize[cellIndex(color, v)] = arrival;
                    updateSupport(color, v);
                    int p = parent[cellIndex(color, v)];
                    if (changed && p >= 0) {
                        // Parents occur later than every child, including
                        // when another changed branch already queued them.
                        int parentIndex = orderIndex[cellIndex(color, p)];
                        queued[parentIndex >> 6] |= uint64_t(1) << (parentIndex & 63);
                    }
                }
            }
        }

        // Re-sum affected cells in color order instead of subtracting old
        // contributions, preserving the original floating-point accumulation.
        for (int v : dirtySupportCells) {
            support[v].fill(0.0);
            for (int color = 0; color < K; ++color) {
                if (traffic[cellIndex(color, v)] == 0.0) continue;
                const auto& values = supportByColor[cellIndex(color, v)];
                for (int height = 1; height < 8; ++height)
                    support[v][height] += values[height];
            }
        }
    }

    optional<Decision> decide(int mode, bool allowShortcuts, bool allowTransfers,
                              Clock::time_point deadline, const Candidate* focus = nullptr) {
        auto& board = state.board;
        if (Clock::now() >= deadline) return nullopt;
        if (++decisionGeneration == 0) {
            fill(supportAt.begin(), supportAt.end(), 0);
            fill(exposureAt.begin(), exposureAt.end(), 0);
            ++decisionGeneration;
        }
        // These are scheduling preferences, not legality constraints. Relax
        // them at the edited action only; suffix repair keeps the full policy.
        const bool explore = focus != nullptr;
        const auto& forward = allowShortcuts ? shortcutForward : treeForward;
        unsigned candidateChecks = 0;
        auto deadlineReached = [&]() {
            return (++candidateChecks & 63u) == 0 && Clock::now() >= deadline;
        };
        // Models are fixed before execution: they trade readiness against
        // preserving useful foreign-color springboards.
        const auto& model = MODELS[mode];
        const double supportWeight = model.supportWeight;
        const double readinessWeight = model.readinessWeight;

        const double sharedWeight = 0.45;

        prepareDecisionCache(allowShortcuts);

        Decision decision;
        optional<AlternativeCollector> alternatives;
        if (explore) alternatives.emplace(board, *focus);
        int serial = 0;
        auto offer = [&](const Candidate& action) {
            Candidate tagged = action;
            tagged.serial = serial++;
            if (better(tagged, decision.greedy)) decision.greedy = tagged;
            if (explore) alternatives->add(tagged);
        };

        for (int source : activeCells) {
            int color = topColor[source];
            if (color < 0 || forward[cellIndex(color, source)].empty() || (!explore && waiting[source])) continue;
            int height = board[source].size;
            int block = topCount[source];
            int bottom = height - block;
            int subtree = pending[cellIndex(color, source)];
            int desired = min(8 - bottom, subtree);
            double originalSupport = foreignSupport(color, source, height);
            unsigned tie = (unsigned)(source + 1) * 2654435761u;
            tie ^= (unsigned)(color + 1) * 2246822519u;
            const double tieScore = 0.00001 * (tie % 1024);
            array<array<int, 9>, 4> skippedPrefix{};
            for (int d = 0; allowShortcuts && d < 4; ++d) {
                for (int distance = 1; distance <= 8; ++distance) {
                    int v = ray[source][d][distance - 1];
                    if (v < 0) break;
                    skippedPrefix[d][distance] = skippedPrefix[d][distance - 1] + local[cellIndex(color, v)];
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
                for (const Forward& step : forward[cellIndex(color, source)]) {
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
                            int skipped = jump[cellIndex(color, source)][len - 2];
                            if (!explore && canJoinMonochrome(skipped, color, count)) break;
                            skippedSameColor += local[cellIndex(color, skipped)];
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
                    cacheMonochromeArrival(target, color);
                    const int arrivalIndex = cellIndex(color, target);
                    const int arrivalHeight = nest[target] == color ?
                        arrivalHomedSize[arrivalIndex] : destinationHeight + count;
                    double supportChange = sourceSupportChange;
                    supportChange += foreignSupport(color, target, arrivalHeight) -
                                     foreignSupport(color, target, destinationHeight);
                    score += supportWeight * supportChange;

                    // A monochrome arrival may form a useful mixed stack.
                    // Conversely, discourage peeling apart a shared route.
                    double change = sourceSharedChange +
                                    arrivalSharedValue[arrivalIndex] - sharedPotential[target];
                    score += sharedWeight * change;

                    // A stable tie break keeps candidate ordering deterministic.
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
        struct Packet {
            array<uint8_t, MAX_COLOR> counts{}; // Each payload contains at most eight slimes.
            unsigned colors = 0;
            uint8_t exposed = 0;
            bool split = false;
        };
        auto makePacket = [&](const Stack& stack, int count, int exposed) {
            Packet packet;
            int kept = stack.size - count;
            for (int j = kept; j < stack.size; ++j) {
                ++packet.counts[stack.color[j]];
                packet.colors |= 1u << stack.color[j];
            }
            packet.exposed = min(count, exposed);
            packet.split = kept > 0 && stack.color[kept - 1] == stack.color[kept];
            return packet;
        };
        // Batch policy and payload metadata are fixed before enumerating
        // destinations. Only capacity and intermediate cells vary here.
        auto simulate = [&](LocalState& state, int source, int target, int count, int len,
                            const Packet& packet) {
            int direction = moveDirection(source, target, len);
            if (direction < 0) return false;
            Stack& from = editStack(state, source);
            Stack& to = editStack(state, target);
            int kept = from.size - count;
            if (!canTransfer(from, to, kept, len)) return false;

            array<int, MAX_COLOR> skipped{};
            for (int step = 1; step < len; ++step) {
                int v = ray[source][direction][step - 1];
                const Stack& middle = readStack(state, v);
                if (!explore && canJoin(v, from, count, middle)) return false;
                for (int j = 0; j < middle.size; ++j) ++skipped[middle.color[j]];
            }
            for (unsigned mask = packet.colors; mask; mask &= mask - 1) {
                int color = __builtin_ctz(mask);
                state.skipped += min<int>(packet.counts[color], skipped[color]);
                // Transfers change only the moving pieces' depths. Homed
                // pieces are removed at their own nest, whose depth is zero.
                // A temporary off-tree depth cancels across the second move.
                state.gain[color] += packet.counts[color] *
                    (depth[cellIndex(color, source)] - depth[cellIndex(color, target)]);
            }
            if (to.size > 0 && to.color[to.size - 1] == from.color[from.size - 1])
                state.merged += min<int>(packet.exposed, to.topCount());
            state.splits += packet.split;
            int homed = transferSlimes(from, to, kept, len, nest[source], nest[target]);
            if (homed < 0) return false;
            state.homed += homed;
            return true;
        };
        auto exposure = [&](const LocalState& state, int v, const Stack& stack) {
            if (stack.size == 0) return 0.0;
            int color = stack.color[stack.size - 1], count = stack.topCount();
            if (!allowShortcuts) {
                int maximum = min<int>(run[cellIndex(color, v)], stack.size - count + 1);
                for (int len = maximum; len >= 1; --len) {
                    int target = jump[cellIndex(color, v)][len - 1];
                    if (readStack(state, target).size + count <= 8)
                        return 0.5 * count + len - 1;
                }
                return 0.0;
            }
            int maximum = stack.size - count + 1;
            for (const Forward& step : shortcutExposure[cellIndex(color, v)])
                if (step.length <= maximum && readStack(state, step.target).size + count <= 8)
                    return 0.5 * count + step.gain - 1;
            return 0.0;
        };
        const LocalState before;
        struct MixedReadiness {
            array<double, MAX_COLOR> ready{};
            double combinedReady = 0.0;
            int colors = 0;
        };
        auto evaluate = [&](const LocalState& state, const Candidate& action,
                            const array<uint8_t, MAX_COLOR>& moved, unsigned movingMask,
                            const MixedReadiness& readiness, int sourcePrefix) {
            int steps = action.secondTarget < 0 ? 1 : 2;
            const Stack& original = board[action.source];
            const auto& gain = state.gain;
            double supportChange = 0.0, exposureChange = 0.0, sharedChange = 0.0;
            for (int i = 0; i < state.size; ++i) {
                int v = state.cell[i];
                const Stack& old = board[v];
                const Stack& now = state.stack[i];
                if (!originalOnTree[v]) return;
                for (int j = 0; j < now.size; ++j) {
                    int color = now.color[j];
                    if (depth[cellIndex(color, v)] < 0) return;
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
            const bool commonRoute = steps == 1 &&
                jump[cellIndex(original.color[original.size - 1], action.source)][action.length - 1] == action.target &&
                sourcePrefix >= action.length;
            double score = 0.0;
            int totalGain = 0;
            for (int color = 0; color < K; ++color) {
                if (gain[color] < 0) return;
                totalGain += gain[color];
                // The combined-batch formula below replaces the per-color score.
                if (commonRoute || moved[color] == 0 || gain[color] == 0) continue;
                double distance = (double)gain[color] / moved[color];
                score += 1.0 + 0.55 * moved[color] + 1.65 * (distance - 1.0);
                score -= readinessWeight * (1.0 - readiness.ready[color]);
            }
            if (totalGain <= 0) return;
            if (commonRoute) {
                // On a common route the colors depart as one batch. Apply
                // the readiness penalty once to the combined load, instead
                // of making every color wait independently for its subtree.
                double distance = (double)totalGain / action.count;
                score = 1.0 + 0.55 * action.count + 1.65 * (distance - 1.0) +
                        0.85 * (readiness.colors - 1) -
                        readinessWeight * (1.0 - readiness.combinedReady);
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
            if (top < 0 || block == stack.size || forward[cellIndex(top, source)].empty() ||
                (!explore && waiting[source])) continue;
            if (Clock::now() >= deadline) return nullopt;
            array<int, MAX_COLOR> firstIndex;
            firstIndex.fill(8);
            for (int j = 0; j < stack.size; ++j)
                firstIndex[stack.color[j]] = min(firstIndex[stack.color[j]], j);
            array<unsigned, 9> suffixColors{};
            for (int j = stack.size - 1; j >= 0; --j)
                suffixColors[j] = suffixColors[j + 1] | (1u << stack.color[j]);
            Packet packet;
            packet.counts[top] = packet.exposed = block;
            packet.colors = 1u << top;
            const ProgressMask* progress = progressMask.data() + source * cells;
            for (int count = block + 1; count <= stack.size; ++count) {
                int kept = stack.size - count;
                ++packet.counts[stack.color[kept]];
                packet.colors |= 1u << stack.color[kept];
                if (!explore && count < batchCount[source] && wholeCanMove[source]) continue;
                packet.split = kept > 0 && stack.color[kept - 1] == stack.color[kept];
                int sourcePrefix = sharedPrefix(source, stack, count);
                int bottomColor = stack.color[kept], bottomCount = 1;
                while (bottomCount < count && stack.color[kept + bottomCount] == bottomColor)
                    ++bottomCount;
                // The same color can occur both in the bottom block and
                // above it, so use the actual upper suffix's color mask.
                unsigned upperColors = suffixColors[kept + bottomCount];
                // Readiness depends on the source and payload, not the landing
                // cell or second move. Keep weighting in evaluate for the same
                // floating-point expression and accumulation order.
                MixedReadiness readiness;
                int incoming = 0;
                for (int color = 0; color < K; ++color) {
                    if (packet.counts[color] == 0) continue;
                    int pendingCount = pending[cellIndex(color, source)];
                    int desired = min(8 - firstIndex[color], pendingCount);
                    readiness.ready[color] = min(1.0, SMALL_RATIO[packet.counts[color]][desired]);
                    incoming += pendingCount;
                    ++readiness.colors;
                }
                int combinedDesired = min(8 - kept, incoming);
                readiness.combinedReady = min(1.0, SMALL_RATIO[count][combinedDesired]);
                for (const Forward& step : forward[cellIndex(top, source)]) {
                    if (deadlineReached()) return nullopt;
                    int len = step.length, target = step.target;
                    if (len > kept + 1) continue;
                    if (board[target].size + count > 8) continue;
                    // Only the lowest homogeneous block may temporarily
                    // leave its tree: reversal exposes it for the next move.
                    unsigned advances = progress[target].strict;
                    if (upperColors & ~advances) continue;

                    Candidate action{source, target, count, len, -1e100};
                    LocalState first;
                    if (!simulate(first, source, target, count, len, packet)) continue;
                    bool allAdvance = (advances & (1u << bottomColor)) != 0;
                    if (allAdvance)
                        evaluate(first, action, packet.counts, packet.colors, readiness, sourcePrefix);
                    const Stack& landing = readStack(first, target);
                    int payload = max(0, landing.size - board[target].size);
                    if (payload == 0) continue;
                    int landingTopCount = landing.topCount();
                    int together = explore ? 0 : batchSize(target, landing);
                    bool checkedBatch = false, canMoveBatch = false;
                    auto allowSecondCount = [&](int nextCount) {
                        if (nextCount <= 0 || nextCount > landing.size) return false;
                        if (nextCount >= together) return true;
                        if (!checkedBatch) {
                            checkedBatch = true;
                            auto read = [&](int v) -> const Stack& { return readStack(first, v); };
                            canMoveBatch = batchCanMove(target, landing, together, read);
                        }
                        return !canMoveBatch;
                    };

                    if (allowTransfers) {
                        // Release the newly exposed block or carry the
                        // entire surviving payload. Search every physical
                        // direction, including right-angle transfers and
                        // recovery from a temporary off-tree landing.
                        int exposed = min(payload, landingTopCount);
                        array<int, 2> choices{exposed, payload};
                        for (int choice = 0; choice < 2; ++choice) {
                            if (choice == 1 && exposed == payload) continue;
                            int nextCount = choices[choice];
                            if (!allowSecondCount(nextCount)) continue;
                            Packet nextPacket = makePacket(landing, nextCount, landingTopCount);
                            int maxNext = min(8, landing.size - nextCount + 1);
                            for (int d = 0; d < 4; ++d) {
                                for (int nextLen = 1; nextLen <= maxNext; ++nextLen) {
                                    if (deadlineReached()) return nullopt;
                                    int end = ray[target][d][nextLen - 1];
                                    if (end < 0) break;
                                    if (nextPacket.colors & ~progress[end].nonworsening) continue;
                                    LocalState second = first;
                                    if (!simulate(second, target, end, nextCount, nextLen, nextPacket)) continue;
                                    Candidate pair = action;
                                    pair.secondTarget = end;
                                    pair.secondCount = nextCount;
                                    pair.secondLength = nextLen;
                                    evaluate(second, pair, packet.counts, packet.colors, readiness, sourcePrefix);
                                }
                            }
                        }
                    } else if (allAdvance) {
                        // Odd: release the newly exposed bottom-color block.
                        // Even: carry the entire surviving payload once more,
                        // restoring its original orientation at the next stop.
                        int exposed = min(payload, landingTopCount);
                        array<int, 2> choices{exposed, payload};
                        int color = landing.color[landing.size - 1];
                        for (int choice = 0; choice < 2; ++choice) {
                            if (choice == 1 && payload == exposed) continue;
                            int nextCount = choices[choice];
                            if (!allowSecondCount(nextCount)) continue;
                            Packet nextPacket = makePacket(landing, nextCount, landingTopCount);
                            int maxNext = min<int>(run[cellIndex(color, target)], landing.size - nextCount + 1);
                            for (int nextLen = 1; nextLen <= maxNext; ++nextLen) {
                                if (deadlineReached()) return nullopt;
                                int end = jump[cellIndex(color, target)][nextLen - 1];
                                if (nextPacket.colors & ~progress[end].nonworsening) continue;
                                LocalState second = first;
                                if (!simulate(second, target, end, nextCount, nextLen, nextPacket)) continue;
                                Candidate pair = action;
                                pair.secondTarget = end;
                                pair.secondCount = nextCount;
                                pair.secondLength = nextLen;
                                evaluate(second, pair, packet.counts, packet.colors, readiness, sourcePrefix);
                            }
                        }
                    } else {
                        // Carry the upper colors forward, flip the hidden
                        // bottom block to the top, then use those upper
                        // colors as a springboard to jump back past source.
                        if (!allowSecondCount(bottomCount)) continue;
                        Packet nextPacket = makePacket(landing, bottomCount, landingTopCount);
                        int maxNext = landing.size - bottomCount + 1;
                        for (int nextLen = len; nextLen <= maxNext; ++nextLen) {
                            if (deadlineReached()) return nullopt;
                            int back = nextLen - len;
                            int end = ray[target][step.direction ^ 1][nextLen - 1];
                            if (end < 0 || (back > 0 &&
                                jump[cellIndex(bottomColor, source)][back - 1] != end)) continue;
                            LocalState second = first;
                            if (!simulate(second, target, end, bottomCount, nextLen, nextPacket)) continue;
                            Candidate pair = action;
                            pair.secondTarget = end;
                            pair.secondCount = bottomCount;
                            pair.secondLength = nextLen;
                            evaluate(second, pair, packet.counts, packet.colors, readiness, sourcePrefix);
                        }
                    }
                }
            }
        }

        if (explore) {
            // Borrow an occupied tower with a monochrome suffix, then move
            // exactly that suffix onward. Only the intermediate landing may
            // go backward or leave the color's tree; the loan must restore
            // its host stack and finish strictly ahead on the original tree.
            // This SA neighborhood is independent of the seed's options and
            // is never enumerated during initial construction or greedy repair.
            auto borrowSpringboard = [&](int source, int host, int firstLength) {
                const Stack& original = board[source];
                const Stack& springboard = board[host];
                if (original.size == 0 || springboard.size == 0 || springboard.size == 8 ||
                    !originalOnTree[source] || !originalOnTree[host]) return true;
                int color = original.color[original.size - 1];
                if (nest[host] == color || depth[cellIndex(color, source)] <= 0) return true;
                int block = original.topCount();
                int maximumCount = min<int>({block, 8 - springboard.size,
                                             original.size - firstLength + 1});
                for (int count = 1; count <= maximumCount; ++count) {
                    if (deadlineReached()) return false;
                    Packet packet = makePacket(original, count, count);
                    LocalState first;
                    if (!simulate(first, source, host, count, firstLength, packet)) continue;
                    const Stack& landing = readStack(first, host);
                    if (landing.size != springboard.size + count) continue;
                    // The temporary landing is not a lasting merge. Splitting
                    // it again merely returns the borrowed tower unchanged.
                    first.merged = first.skipped = 0.0;
                    Packet departure = makePacket(landing, count, count);
                    departure.split = false;
                    MixedReadiness readiness;
                    int desired = min(8 - (original.size - block),
                                      pending[cellIndex(color, source)]);
                    readiness.ready[color] = SMALL_RATIO[count][desired];
                    readiness.ready[color] = min(1.0, readiness.ready[color]);
                    readiness.combinedReady = readiness.ready[color];
                    readiness.colors = 1;
                    for (int d = 0; d < 4; ++d) {
                        for (int len = 1; len <= springboard.size + 1; ++len) {
                            if (deadlineReached()) return false;
                            int end = ray[host][d][len - 1];
                            if (end < 0) break;
                            if (!(progressMask[source * cells + end].strict & packet.colors) ||
                                board[end].size + count > 8) continue;
                            // A direct monochrome transfer has the same final
                            // stacks in one move, so it strictly dominates this loan.
                            int direct = moveInfo[source * cells + end];
                            if (direct >= 0 && canTransfer(original, board[end],
                                                          original.size - count, (direct & 7) + 1))
                                continue;
                            LocalState second = first;
                            if (!simulate(second, host, end, count, len, departure)) continue;
                            if (!(readStack(second, host) == springboard)) continue;
                            Candidate action{source, host, count, firstLength, -1e100};
                            action.secondTarget = end;
                            action.secondCount = count;
                            action.secondLength = len;
                            evaluate(second, action, packet.counts, packet.colors, readiness, 0);
                        }
                    }
                }
                return true;
            };
            // Also borrow the focused source as a host: otherwise a pivot
            // that removes a springboard cannot discover how to use it first.
            int at = focus->source;
            if (at >= 0 && at < cells && board[at].size > 0) {
                for (int d = 0; d < 4; ++d) {
                    for (int len = 1; len <= 8; ++len) {
                        if (deadlineReached()) return nullopt;
                        int other = ray[at][d][len - 1];
                        if (other < 0) break;
                        if (board[other].size == 0) continue;
                        if (!borrowSpringboard(at, other, len) ||
                            !borrowSpringboard(other, at, len)) return nullopt;
                    }
                }
            }
        }

        if (decision.greedy.source < 0) return nullopt;
        if (explore) decision.alternatives = alternatives->finish();
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
            int occupiedBefore = (value.board[source].size > 0) +
                                 (value.board[target].size > 0);
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
            value.occupiedCount += (value.board[source].size > 0) +
                                   (value.board[target].size > 0) - occupiedBefore;
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
    chrono::steady_clock::time_point deadline, int bestOps, bool& pruned) {
    pruned = false;
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
        // Initial seeds are compared before compression, with the first seed
        // winning ties. Further actions cannot reduce the accumulated count.
        if ((int)seed.operations.size() + action.steps() >= bestOps) {
            pruned = true;
            return nullopt;
        }
        if ((int)seed.operations.size() + action.steps() > MAX_OP ||
            !sim.apply(sim.state, action, &seed.operations)) return nullopt;
        seed.actions.push_back(action);
    }
    return seed;
}

struct HistorySearchStats {
    int initialRawOps = -1, bestRawOps = -1;
    long long attempts = 0, completed = 0, accepted = 0, improvements = 0;
    long long invalid = 0;
    enum Counter {
        Visits, Attempts, Completed, Accepted, Improvements, SavedOps,
        RebuiltActions, RebuiltOps, Rejoined, EqualRejoined, EqualShort,
        EqualShortAccepted, NoChoices, PrepareTimeout, PrepareInvalid,
        CommitInvalid, DoubleRequested, DoubleApplied, DoubleFallback,
        DoubleCompleted, DoubleAccepted, DoubleImprovements, DoubleSavedOps,
        CounterCount
    };
    enum Failure { StepLimit, RemainingBound, NoMove, Timeout, Invalid, FailureCount };
    // Sign order: better / equal / worse relative to the current history.
    // Stage order: pivot restore / choices / suffix repair / acceptance and commit.
    struct Bucket {
        array<long long, CounterCount> count{};
        array<long long, FailureCount> failures{}, failureNs{};
        array<long long, 3> proposed{}, adopted{}, deltaSum{}, adoptedDeltaSum{};
        array<long long, 3> proposalNs{}, adoptedCommitNs{};
        array<long long, 4> stageNs{};
        long long totalNs = 0;
        void add(const Bucket& other) {
            auto sum = [](auto& into, const auto& from) {
                for (int i = 0; i < (int)into.size(); ++i) into[i] += from[i];
            };
            sum(count, other.count);
            sum(failures, other.failures);
            sum(failureNs, other.failureNs);
            sum(proposed, other.proposed);
            sum(adopted, other.adopted);
            sum(deltaSum, other.deltaSum);
            sum(adoptedDeltaSum, other.adoptedDeltaSum);
            sum(proposalNs, other.proposalNs);
            sum(adoptedCommitNs, other.adoptedCommitNs);
            sum(stageNs, other.stageNs);
            totalNs += other.totalNs;
        }
    };
    Bucket total;
    array<Bucket, 4> position, phase;
    long long initialHistoryNs = 0, finalizeNs = 0, lastImprovementNs = -1;
    bool exhausted = false;

    void writeProfile(ostream& out) const {
        constexpr const char* names[] = {
            "visits", "attempts", "completed", "accepted", "improvements", "saved_ops",
            "rebuilt_actions", "rebuilt_ops", "rejoined", "equal_rejoined", "equal_short",
            "equal_short_accepted", "no_choices", "prepare_timeout", "prepare_invalid",
            "commit_invalid", "double_requested", "double_applied", "double_fallback",
            "double_completed", "double_accepted", "double_improvements", "double_saved_ops"};
        static_assert(size(names) == CounterCount);
        auto list = [&](const auto& values) {
            out << '[';
            for (int i = 0; i < (int)values.size(); ++i) {
                if (i) out << ',';
                out << values[i];
            }
            out << ']';
        };
        auto bucket = [&](const Bucket& b) {
            out << '{';
            for (int i = 0; i < CounterCount; ++i) {
                if (i) out << ',';
                out << '"' << names[i] << "\":" << b.count[i];
            }
            out << ",\"failures\":"; list(b.failures);
            out << ",\"failure_ns\":"; list(b.failureNs);
            out << ",\"proposed\":"; list(b.proposed);
            out << ",\"adopted\":"; list(b.adopted);
            out << ",\"delta_sum\":"; list(b.deltaSum);
            out << ",\"adopted_delta_sum\":"; list(b.adoptedDeltaSum);
            out << ",\"proposal_ns\":"; list(b.proposalNs);
            out << ",\"adopted_commit_ns\":"; list(b.adoptedCommitNs);
            out << ",\"stage_ns\":"; list(b.stageNs);
            out << ",\"total_ns\":" << b.totalNs << '}';
        };
        out << " sa_profile={\"version\":1,\"initial_history_ns\":" << initialHistoryNs
            << ",\"finalize_ns\":" << finalizeNs
            << ",\"last_improvement_ns\":" << lastImprovementNs
            << ",\"exhausted\":" << exhausted << ",\"total\":";
        bucket(total);
        out << ",\"position\":[";
        for (int i = 0; i < 4; ++i) { if (i) out << ','; bucket(position[i]); }
        out << "],\"phase\":[";
        for (int i = 0; i < 4; ++i) { if (i) out << ','; bucket(phase[i]); }
        out << "]}";
    }
};

// Aggregate one visit, including early exits. Profiling is compiled away with
// ENABLE_DEBUG=0; it never draws random numbers or changes search decisions.
struct HistoryIterationProfile {
    using Clock = chrono::steady_clock;
    HistorySearchStats& stats;
    HistorySearchStats::Bucket data;
    Clock::time_point start{}, stageStart{};
    int position = 0, phase = 0, stage = 0;
    explicit HistoryIterationProfile(HistorySearchStats& stats_, Clock::time_point origin,
                                     double duration) : stats(stats_) {
        if constexpr (ENABLE_DEBUG) {
            start = stageStart = Clock::now();
            phase = clamp((int)(4 * chrono::duration<double>(start - origin).count() / duration), 0, 3);
            data.count[HistorySearchStats::Visits] = 1;
        }
    }
    void advance(int nextStage) {
        if constexpr (ENABLE_DEBUG) {
            auto now = Clock::now();
            data.stageNs[stage] += chrono::duration_cast<chrono::nanoseconds>(now - stageStart).count();
            stageStart = now;
            stage = nextStage;
        }
    }
    ~HistoryIterationProfile() {
        if constexpr (ENABLE_DEBUG) {
            auto end = Clock::now();
            data.stageNs[stage] += chrono::duration_cast<chrono::nanoseconds>(end - stageStart).count();
            data.totalNs = chrono::duration_cast<chrono::nanoseconds>(end - start).count();
            for (int i = 0; i < HistorySearchStats::FailureCount; ++i)
                if (data.failures[i]) data.failureNs[i] = data.totalNs;
            for (int i = 0; i < 3; ++i) {
                if (data.proposed[i]) data.proposalNs[i] = data.totalNs;
                if (data.adopted[i]) data.adoptedCommitNs[i] = data.stageNs[3];
            }
            stats.total.add(data);
            stats.position[position].add(data);
            stats.phase[phase].add(data);
        }
    }
};

// Search complete histories with one or occasionally two relaxed scheduling
// choices. Other repair steps use the original cooperative evaluator. Two-step
// transfers stay atomic, including when choosing the second edit position.
optional<vector<Op>> rewriteCooperativeHistory(
    CooperativeScheduler& sim, const GreedySeed& seed,
    chrono::steady_clock::time_point deadline, HistorySearchStats& stats) {
    using Clock = chrono::steady_clock;
    using Candidate = CooperativeScheduler::Candidate;
    using State = CooperativeScheduler::State;
    using Profile = HistorySearchStats;
    Clock::time_point profileStart{};
    if constexpr (ENABLE_DEBUG) profileStart = Clock::now();
    constexpr int CHECKPOINT = 16;
    constexpr int DOUBLE_PIVOT_DENOMINATOR = 10;
    constexpr int MAX_SECOND_PIVOT_OFFSET = 8;
    struct Site {
        bool ready = false;
        vector<Candidate> choices;
        uint64_t tried = 0;
        bool available() const {
            // The four edit families contribute at most 32 unique choices.
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
    auto stateHash = [&](const State& state) {
        uint64_t hash = 0;
        for (int v = 0; v < sim.cells; ++v) hash ^= state.board[v].positionHash(v);
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
            if (v >= 0) before ^= state.board[v].positionHash(v);
        }
        if (!sim.apply(state, action)) return false;
        hash ^= before;
        for (int i = 0; i < 3; ++i) {
            int v = touched[i];
            if (v < 0) continue;
            for (int j = 0; j < i; ++j)
                if (touched[j] == v) { v = -1; break; }
            if (v >= 0) hash ^= state.board[v].positionHash(v);
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
            if (i % CHECKPOINT == 0) {
                history.checkpoints.push_back(state);
            }
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
            if (!sim.apply(out, history.actions[i])) return false;
        }
        return true;
    };

    const int initialRawOps = (int)seed.operations.size();
    if constexpr (ENABLE_DEBUG)
        stats.initialRawOps = stats.bestRawOps = initialRawOps;
    if (seed.actions.empty() || Clock::now() >= deadline) return nullopt;
    auto initial = buildHistory(seed.actions);
    if (!initial || initial->cost.back() != initialRawOps) {
        if constexpr (ENABLE_DEBUG) ++stats.invalid;
        return nullopt;
    }
    History current = std::move(*initial);
    int bestRaw = current.cost.back();
    vector<Candidate> bestActions = current.actions;
    mt19937_64 rng(0x07220260928ULL);
    auto uniform = [&]() {
        return ((rng() >> 11) + 0.5) * (1.0 / 9007199254740992.0);
    };
    auto chooseAlternative = [&](const vector<Candidate>& choices, uint64_t tried) {
        assert(choices.size() <= CooperativeScheduler::MAX_ALTERNATIVES);
        uint64_t available = ((uint64_t(1) << choices.size()) - 1) & ~tried;
        int count = __builtin_popcountll(available);
        if (count == 0) return -1;
        // Every distinct, untried action has the same probability, regardless
        // of its score or how many nearby roles it fills.
        int draw = uniform_int_distribution<int>(0, count - 1)(rng);
        while (draw-- > 0) available &= available - 1;
        return __builtin_ctzll(available);
    };
    const auto searchStart = Clock::now();
    if constexpr (ENABLE_DEBUG)
        stats.initialHistoryNs = chrono::duration_cast<chrono::nanoseconds>(searchStart - profileStart).count();
    const double duration = max(1e-9, chrono::duration<double>(deadline - searchStart).count());
    const double startTemperature = clamp(bestRaw * 0.01, 1.5, 5.0);

    while (Clock::now() < deadline) {
        if (current.totalWeight == 0) {
            if constexpr (ENABLE_DEBUG) stats.exhausted = true;
            break;
        }
        HistoryIterationProfile profile(stats, searchStart, duration);
        int cut = selectSite(current, (int)(rng() % current.totalWeight));
        if constexpr (ENABLE_DEBUG)
            profile.position = min(3, 4 * cut / max(1, (int)current.actions.size()));
        if (!restore(current, cut, sim.state)) {
            if constexpr (ENABLE_DEBUG) {
                ++stats.invalid;
                ++profile.data.count[Profile::PrepareInvalid];
            }
            break;
        }
        profile.advance(1);
        Site& site = current.sites[cut];
        if (!site.ready) {
            auto decision = sim.decide(seed.mode, seed.shortcuts, seed.transfers,
                                       deadline, &current.actions[cut]);
            if (Clock::now() >= deadline) {
                if constexpr (ENABLE_DEBUG) ++profile.data.count[Profile::PrepareTimeout];
                break;
            }
            site.ready = true;
            if (!decision) {
                if constexpr (ENABLE_DEBUG) ++profile.data.count[Profile::NoChoices];
                removeSite(current, cut);
                continue;
            }
            site.choices = std::move(decision->alternatives);
            if (site.choices.empty()) {
                if constexpr (ENABLE_DEBUG) ++profile.data.count[Profile::NoChoices];
                removeSite(current, cut);
                continue;
            }
        }

        int selected = chooseAlternative(site.choices, site.tried);
        if (selected < 0) {
            if constexpr (ENABLE_DEBUG) ++profile.data.count[Profile::PrepareInvalid];
            break;
        }
        int remainingActions = (int)current.actions.size() - cut;
        const bool doubleRequested = remainingActions > 1 &&
                                     rng() % DOUBLE_PIVOT_DENOMINATOR == 0;
        const int secondPivotAfter = doubleRequested ?
            1 + (int)(rng() % min(MAX_SECOND_PIVOT_OFFSET, remainingActions - 1)) : 0;
        // A two-edit attempt must not consume the corresponding one-edit trial.
        if (!doubleRequested) {
            site.tried |= uint64_t(1) << selected;
            if (!site.available()) removeSite(current, cut);
        }
        Candidate next = site.choices[selected];
        if constexpr (ENABLE_DEBUG) {
            ++stats.attempts;
            ++profile.data.count[Profile::Attempts];
            profile.data.count[Profile::DoubleRequested] += doubleRequested;
        }
        profile.advance(2);

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
        bool secondPending = doubleRequested;
        bool secondApplied = false, nextIsSecond = false;
        auto tryJoin = [&]() {
            auto range = current.boundary.equal_range(hash);
            for (auto it = range.first; it != range.second; ++it) {
                int at = it->second;
                if (at <= cut || cost + current.cost.back() - current.cost[at] > limit) continue;
                State exact;
                if (!restore(current, at, exact)) continue;
                // The hash only indexes candidates; compare the ordered stacks.
                if (!equalState(sim.state, exact)) continue;
                join = at;
                cost += current.cost.back() - current.cost[at];
                return true;
            }
            return false;
        };
        while (true) {
            if (Clock::now() >= deadline) {
                if constexpr (ENABLE_DEBUG) ++profile.data.failures[Profile::Timeout];
                break;
            }
            if (cost + next.steps() > limit) {
                if constexpr (ENABLE_DEBUG) ++profile.data.failures[Profile::StepLimit];
                break;
            }
            {
                if (!applyTracked(sim.state, next, hash)) {
                    if constexpr (ENABLE_DEBUG) {
                        ++stats.invalid;
                        ++profile.data.failures[Profile::Invalid];
                    }
                    break;
                }
                rebuilt.push_back(next);
                cost += next.steps();
                if constexpr (ENABLE_DEBUG) {
                    ++profile.data.count[Profile::RebuiltActions];
                    profile.data.count[Profile::RebuiltOps] += next.steps();
                }
                if (nextIsSecond) {
                    secondApplied = true;
                    nextIsSecond = false;
                    if constexpr (ENABLE_DEBUG) ++profile.data.count[Profile::DoubleApplied];
                }
            }
            if (sim.state.remaining == 0) {
                if constexpr (ENABLE_DEBUG)
                    if (secondPending) ++profile.data.count[Profile::DoubleFallback];
                completed = true;
                break;
            }
            // Every stack is already homed. An occupied destination keeps its
            // old non-nest top, so only the source can become empty in one move.
            // Each occupied cell therefore requires at least one more move.
            if (cost + sim.state.occupiedCount > limit) {
                if constexpr (ENABLE_DEBUG) ++profile.data.failures[Profile::RemainingBound];
                break;
            }
            // Do not let an early rejoin swallow the scheduled second edit.
            if (!secondPending && tryJoin()) {
                completed = true;
                break;
            }
            auto decision = sim.decide(seed.mode, seed.shortcuts, seed.transfers, deadline);
            if (!decision) {
                bool timedOut = Clock::now() >= deadline;
                if (secondPending && !timedOut) {
                    secondPending = false;
                    if constexpr (ENABLE_DEBUG) ++profile.data.count[Profile::DoubleFallback];
                    if (tryJoin()) {
                        completed = true;
                        break;
                    }
                }
                if constexpr (ENABLE_DEBUG)
                    ++profile.data.failures[timedOut ? Profile::Timeout : Profile::NoMove];
                break;
            }
            next = decision->greedy;
            if (secondPending && (int)rebuilt.size() >= secondPivotAfter) {
                // Compare against the normal greedy choice. Exploration relaxes
                // scheduling rules, so its own greedy choice may already differ.
                const Candidate greedy = next;
                auto alternatives = sim.decide(seed.mode, seed.shortcuts, seed.transfers,
                                               deadline, &greedy);
                if (Clock::now() >= deadline) {
                    if constexpr (ENABLE_DEBUG) ++profile.data.failures[Profile::Timeout];
                    break;
                }
                secondPending = false;
                // This board differs from the saved history: never reuse or
                // modify its Site cache for the second edit.
                auto choices = alternatives ? std::move(alternatives->alternatives) : vector<Candidate>{};
                if (!choices.empty()) {
                    int second = chooseAlternative(choices, 0);
                    assert(second >= 0);
                    next = choices[second];
                    nextIsSecond = true;
                } else {
                    if constexpr (ENABLE_DEBUG) ++profile.data.count[Profile::DoubleFallback];
                    if (tryJoin()) {
                        completed = true;
                        break;
                    }
                }
            }
        }
        profile.advance(3);
        if (!completed) continue;
        if constexpr (ENABLE_DEBUG) ++stats.completed;
        int delta = cost - current.cost.back();
        int sign = delta < 0 ? 0 : delta == 0 ? 1 : 2;
        bool shortEqualJoin = false;
        if constexpr (ENABLE_DEBUG) {
            ++profile.data.count[Profile::Completed];
            profile.data.count[Profile::DoubleCompleted] += secondApplied;
            ++profile.data.proposed[sign];
            profile.data.deltaSum[sign] = delta;
            if (join >= 0) {
                ++profile.data.count[Profile::Rejoined];
                if (delta == 0) {
                    ++profile.data.count[Profile::EqualRejoined];
                    // Both the new segment and the replaced old segment use
                    // at most two physical operations, not two atomic actions.
                    shortEqualJoin = profile.data.count[Profile::RebuiltOps] <= 2 &&
                                     current.cost[join] - current.cost[cut] <= 2;
                    if (shortEqualJoin) ++profile.data.count[Profile::EqualShort];
                }
            }
        }
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
        if (!changed || changed->cost.back() != cost) {
            if constexpr (ENABLE_DEBUG) {
                ++stats.invalid;
                ++profile.data.count[Profile::CommitInvalid];
            }
            continue;
        }
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
        if constexpr (ENABLE_DEBUG) {
            ++stats.accepted;
            ++profile.data.count[Profile::Accepted];
            profile.data.count[Profile::DoubleAccepted] += secondApplied;
            ++profile.data.adopted[sign];
            profile.data.adoptedDeltaSum[sign] = delta;
            if (shortEqualJoin) ++profile.data.count[Profile::EqualShortAccepted];
        }
        if (cost < bestRaw) {
            if constexpr (ENABLE_DEBUG) {
                ++profile.data.count[Profile::Improvements];
                profile.data.count[Profile::SavedOps] = bestRaw - cost;
                if (secondApplied) {
                    ++profile.data.count[Profile::DoubleImprovements];
                    profile.data.count[Profile::DoubleSavedOps] = bestRaw - cost;
                }
                stats.lastImprovementNs = chrono::duration_cast<chrono::nanoseconds>(Clock::now() - searchStart).count();
            }
            bestRaw = cost;
            bestActions = current.actions;
            if constexpr (ENABLE_DEBUG) {
                stats.bestRawOps = cost;
                ++stats.improvements;
            }
        }
    }
    Clock::time_point finalizeStart{};
    if constexpr (ENABLE_DEBUG) finalizeStart = Clock::now();
    if (bestRaw >= initialRawOps) return nullopt;
    State replay = sim.initial;
    vector<Op> output;
    output.reserve(bestRaw);
    for (const Candidate& action : bestActions) {
        if (!sim.apply(replay, action, &output)) {
            if constexpr (ENABLE_DEBUG) ++stats.invalid;
            return nullopt;
        }
    }
    if (replay.remaining != 0 || (int)output.size() != bestRaw) {
        if constexpr (ENABLE_DEBUG) ++stats.invalid;
        return nullopt;
    }
    if constexpr (ENABLE_DEBUG)
        stats.finalizeNs = chrono::duration_cast<chrono::nanoseconds>(Clock::now() - finalizeStart).count();
    return output;
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    const auto start = chrono::steady_clock::now();
    auto debugNow = [] {
        if constexpr (ENABLE_DEBUG) return chrono::steady_clock::now();
        else return chrono::steady_clock::time_point{};
    };
    int N, K;
    cin >> N >> K;
    vector<string> grid(N);
    for (auto& row : grid) cin >> row;
    const auto routingDeadline = start + chrono::milliseconds(350);
    const auto baselineDeadline = start + chrono::milliseconds(950);
    const auto overallDeadline = start + chrono::milliseconds(1950);
    // Reserve 100 ms for compression. Repeated sparse repairs share their
    // deadlines; retain 10 ms to revisit the output of the region searches.
    const auto searchDeadline = overallDeadline - chrono::milliseconds(100);
    const auto straightCompressionDeadline = overallDeadline - chrono::milliseconds(90);
    const auto repairCompressionDeadline = overallDeadline - chrono::milliseconds(50);
    const auto regionCompressionDeadline = overallDeadline - chrono::milliseconds(20);
    const auto smallRegionDeadline = overallDeadline - chrono::milliseconds(13);
    const auto repairRescanDeadline = overallDeadline - chrono::milliseconds(3);
    const auto postprocessingDeadline = overallDeadline;
    auto routes = buildCrossingTrees(grid, K, routingDeadline);
    if ((int)routes.size() != K) return 0;
    auto fallback = sequentialSolve(grid, K, routes);
    if (!fallback) return 0;
    vector<Op> answer = std::move(*fallback);
    const size_t fallbackOps = answer.size();
    const auto candidateSetupStart = debugNow();
    CooperativeScheduler scheduler(grid, K, routes);
    const auto candidateBuildStart = debugNow();
    const int initialSlimes = scheduler.initial.remaining;
    const int candidateLoad = initialSlimes * N;
    const auto selectedModels = candidateModelIds(candidateLoad);
    optional<GreedySeed> seed;
    int seedScore = INT_MAX, seedAttempts = 0, seedCompleted = 0, seedPruned = 0;
    const char* answerSource = "fallback";
    constexpr int optionMasks[] = {0, 1, 2, 3};
    constexpr int optionCount = size(optionMasks);
    const int candidateCount = (int)selectedModels.size() * optionCount;
    // Preserve the retained models' order; equal-length candidates keep the first seed.
    for (int job = 0; job < candidateCount; ++job) {
        if (chrono::steady_clock::now() >= baselineDeadline) break;
        int mode = selectedModels[job / optionCount], options = optionMasks[job % optionCount];
        // Option bits: 1 = transfers, 2 = shortcuts, 3 = both; 0 enables neither.
        bool shortcuts = (options & 2) != 0, transfers = (options & 1) != 0;
        if constexpr (ENABLE_DEBUG) ++seedAttempts;
        bool pruned = false;
        auto candidate = buildCooperativeSeed(scheduler, mode, shortcuts, transfers,
                                              baselineDeadline, seedScore, pruned);
        if constexpr (ENABLE_DEBUG) seedPruned += pruned;
        if (!candidate) continue;
        if constexpr (ENABLE_DEBUG) ++seedCompleted;
        // Select by the complete, uncompressed initial operation count.
        const int candidateScore = (int)candidate->operations.size();
        if (candidate->operations.size() < answer.size()) {
            answer = candidate->operations;
            answerSource = "seed";
        }
        if (!seed || candidateScore < seedScore) {
            seedScore = candidateScore;
            seed = std::move(candidate);
        }
    }
    const auto candidateBuildEnd = debugNow();
    const size_t baselineOps = answer.size();
    HistorySearchStats stats;
    long long searchNs = 0, straightNs = 0, smallRegionNs = 0;
    size_t compressionInputOps = answer.size(), straightSaved = 0;
    auto straighten = [&](const vector<Op>& input, auto deadline) {
        const auto stageStart = debugNow();
        auto output = compressMoves(grid, K, input, deadline);
        if constexpr (ENABLE_DEBUG)
            straightNs += chrono::duration_cast<chrono::nanoseconds>(debugNow() - stageStart).count();
        return output;
    };
    auto postprocessingStart = debugNow();
    if (seed) {
        const auto searchStart = debugNow();
        auto rewritten = rewriteCooperativeHistory(scheduler, *seed, searchDeadline, stats);
        if constexpr (ENABLE_DEBUG)
            searchNs = chrono::duration_cast<chrono::nanoseconds>(debugNow() - searchStart).count();
        postprocessingStart = debugNow();
        if (rewritten) {
            auto compressed = straighten(*rewritten, straightCompressionDeadline);
            if (compressed.size() < answer.size()) {
                compressionInputOps = rewritten->size();
                straightSaved += rewritten->size() - compressed.size();
                answer = std::move(compressed);
                answerSource = "history";
            }
        }
    }
    const size_t beforePostprocessing = answer.size();
    answer = straighten(answer, straightCompressionDeadline);
    straightSaved += beforePostprocessing - answer.size();
    DependencyRepairStats repairStats;
    const auto repairDeadline = min(repairCompressionDeadline,
        chrono::steady_clock::now() + chrono::milliseconds(40));
    answer = repairDependentMoves(grid, K, answer, repairDeadline, repairStats);
    const size_t afterFirstRepair = answer.size();
    RegionCompressionStats regionStats;
    const auto regionDeadline = min(regionCompressionDeadline,
        chrono::steady_clock::now() + chrono::milliseconds(30));
    answer = compressDependencyRegions(grid, K, answer, regionDeadline, regionStats);
    const size_t beforeSmallRegion = answer.size();
    const auto smallRegionStart = debugNow();
    answer = optimizeDependencyGroups(grid, K, answer, smallRegionDeadline);
    if constexpr (ENABLE_DEBUG)
        smallRegionNs = chrono::duration_cast<chrono::nanoseconds>(debugNow() - smallRegionStart).count();
    const size_t smallRegionSaved = beforeSmallRegion - answer.size();
    DependencyRepairStats afterRegionRepairStats;
    // Only revisit if another compressor changed the procedure. This is a new
    // trace over the current output, with no indices retained from earlier passes.
    if (answer.size() < afterFirstRepair) {
        const auto lateRepairDeadline = min(repairRescanDeadline,
            chrono::steady_clock::now() + chrono::milliseconds(10));
        answer = repairDependentMoves(grid, K, answer, lateRepairDeadline, afterRegionRepairStats);
        repairStats.add(afterRegionRepairStats);
    }
    const size_t beforeLastStraight = answer.size();
    answer = straighten(answer, postprocessingDeadline);
    straightSaved += beforeLastStraight - answer.size();
    const auto solveEnd = debugNow();
    if constexpr (ENABLE_DEBUG) {
        auto nanoseconds = [](auto duration) {
            return chrono::duration_cast<chrono::nanoseconds>(duration).count();
        };
        cerr << "fallback_ops=" << fallbackOps
             << " baseline_ops=" << baselineOps
             << " answer_source=" << answerSource
             << " initial_slimes=" << initialSlimes
             << " candidate_load=" << candidateLoad
             << " candidate_model_count=" << selectedModels.size()
             << " candidate_count=" << candidateCount
             << " seed_attempts=" << seedAttempts
             << " seed_completed=" << seedCompleted
             << " seed_pruned=" << seedPruned
             << " seed_mode=" << (seed ? seed->mode : -1)
             << " seed_shortcuts=" << (seed && seed->shortcuts)
             << " seed_transfers=" << (seed && seed->transfers)
             << " history_initial_raw_ops=" << stats.initialRawOps
             << " history_best_raw_ops=" << stats.bestRawOps
             << " history_attempts=" << stats.attempts
             << " history_completed=" << stats.completed
             << " history_accepted=" << stats.accepted
             << " history_improvements=" << stats.improvements
             << " history_invalid=" << stats.invalid
             << " compression_input_ops=" << compressionInputOps
             << " compression_saved=" << compressionInputOps - answer.size()
             << " straight_saved_ops=" << straightSaved
             << " straight_ns=" << straightNs
             << " repair_attempts=" << repairStats.attempts
             << " repair_accepted=" << repairStats.accepted
             << " repair_saved_ops=" << repairStats.savedOps
             << " repair_passes=" << repairStats.passes
             << " repair_repeat_saved_ops=" << repairStats.repeatSavedOps
             << " repair_quantity_starts=" << repairStats.quantityStarts
             << " repair_quantity_accepted=" << repairStats.quantityAccepted
             << " repair_quantity_saved_ops=" << repairStats.quantitySavedOps
             << " repair_span_total=" << repairStats.spanTotal
             << " repair_span_max=" << repairStats.spanMax
             << " repair_changed_max=" << repairStats.changedMax
             << " repair_visited=" << repairStats.visited
             << " repair_timeouts=" << repairStats.timeouts
             << " repair_deadline_reached=" << repairStats.deadlineReached
             << " repair_ns=" << repairStats.elapsedNs
             << " repair_after_region_passes=" << afterRegionRepairStats.passes
             << " repair_after_region_saved_ops=" << afterRegionRepairStats.savedOps
             << " repair_after_region_deadline_reached=" << afterRegionRepairStats.deadlineReached
             << " repair_after_region_ns=" << afterRegionRepairStats.elapsedNs
             << " region_windows=" << regionStats.windows
             << " region_accepted=" << regionStats.accepted
             << " region_saved_ops=" << regionStats.savedOps
             << " region_selected_ops=" << regionStats.selectedOps
             << " region_span_ops=" << regionStats.spanOps
             << " region_max_selected_ops=" << regionStats.maxSelectedOps
             << " region_max_span_ops=" << regionStats.maxSpanOps
             << " region_accepted_span_ops=" << regionStats.acceptedSpanOps
             << " region_accepted_max_span=" << regionStats.acceptedMaxSpan
             << " region_window_timeouts=" << regionStats.windowTimeouts
             << " region_transitions=" << regionStats.transitions
             << " region_deadline_reached=" << regionStats.deadlineReached
             << " region_ns=" << regionStats.elapsedNs
             << " small_region_saved_ops=" << smallRegionSaved
             << " small_region_ns=" << smallRegionNs
             << " postprocess_ns=" << nanoseconds(solveEnd - postprocessingStart)
             << " postprocess_saved=" << beforePostprocessing - answer.size()
             << " final_ops=" << answer.size()
             << " timing_version=1"
             << " candidate_setup_ns=" << nanoseconds(candidateBuildStart - candidateSetupStart)
             << " candidate_build_ns=" << nanoseconds(candidateBuildEnd - candidateBuildStart)
             << " candidate_total_ns=" << nanoseconds(candidateBuildEnd - candidateSetupStart)
             << " candidate_end_ns=" << nanoseconds(candidateBuildEnd - start)
             << " candidate_budget_exhausted=" << (candidateBuildEnd >= baselineDeadline)
             << " search_ns=" << searchNs
             << " solve_ns=" << nanoseconds(solveEnd - start);
        stats.writeProfile(cerr);
        cerr << '\n';
    }
    for (const auto& op : answer)
        cout << op.r << ' ' << op.c << ' ' << op.k << ' ' << op.dir << ' ' << op.len << '\n';
    return 0;
}
