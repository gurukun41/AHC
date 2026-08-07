#include <bits/stdc++.h>

/*
 * 池なし・未来既知の独立実験解法。
 *
 * 元のインタラクティブ入力ではなく、入力ファイル全体を直接読み込んで
 * 全ターンの操作列を一括出力する。棚分割、未来の幅影価格、受入LNS、
 * 棚内の再配置ビームを組み合わせ、目的関数は利用料合計-移動費とする。
 * 盤面行も入力から読むが、池を含むケースはこの実験の対象外である。
 */
using namespace std;

using ll = long long;
using ld = long double;

static constexpr int MAX_M = 1000;
static constexpr int MAX_S = 25;
static constexpr int SQ_B = 32;
static constexpr int MAX_B = (MAX_M + SQ_B - 1) / SQ_B;
static constexpr int BOARD_W = 50;
static constexpr ll INF64 = (1LL << 62);

struct Timer {
    chrono::steady_clock::time_point st = chrono::steady_clock::now();
    double elapsed() const {
        return chrono::duration<double>(chrono::steady_clock::now() - st).count();
    }
};

struct Group {
    int S = 0, T = 0, P = 0;
    ll V = 0;
    ll moveCost = 0;
    int endIdx = 0; // active on arrival indices [id, endIdx)
};

struct ShapeOpt {
    int h = 0;
    int w = 0;
    int perimeter = 0;
    ll revenue = 0;
};

struct Choice {
    int shelf = -1;
    ShapeOpt sh{};
    bool accepted() const { return shelf >= 0; }
};

struct SqrtRange {
    int n = 0;
    int nb = 0;
    array<int, MAX_M> a{};      // raw values, block lazy excluded
    array<int, MAX_B> lazy{};
    array<int, MAX_B> mx{};     // raw max
    array<int, MAX_B> sm{};     // raw sum

    void init(int n_) {
        n = n_;
        nb = (n + SQ_B - 1) / SQ_B;
        a.fill(0);
        lazy.fill(0);
        mx.fill(0);
        sm.fill(0);
    }

    int blockL(int b) const { return b * SQ_B; }
    int blockR(int b) const { return min(n, (b + 1) * SQ_B); }

    void rebuild(int b) {
        const int l = blockL(b), r = blockR(b);
        int m = INT_MIN;
        int s = 0;
        for (int i = l; i < r; ++i) {
            m = max(m, a[i]);
            s += a[i];
        }
        if (l == r) m = 0;
        mx[b] = m;
        sm[b] = s;
    }

    void push(int b) {
        if (lazy[b] == 0) return;
        const int d = lazy[b];
        const int l = blockL(b), r = blockR(b);
        for (int i = l; i < r; ++i) a[i] += d;
        lazy[b] = 0;
        rebuild(b);
    }

    void rangeAdd(int l, int r, int d) {
        if (l >= r || d == 0) return;
        const int bl = l / SQ_B;
        const int br = (r - 1) / SQ_B;
        for (int b = bl; b <= br; ++b) {
            const int L = max(l, blockL(b));
            const int R = min(r, blockR(b));
            if (L == blockL(b) && R == blockR(b)) {
                lazy[b] += d;
            } else {
                push(b);
                for (int i = L; i < R; ++i) a[i] += d;
                rebuild(b);
            }
        }
    }

    int rangeMax(int l, int r) const {
        if (l >= r) return 0;
        int ans = INT_MIN;
        const int bl = l / SQ_B;
        const int br = (r - 1) / SQ_B;
        for (int b = bl; b <= br; ++b) {
            const int L = max(l, blockL(b));
            const int R = min(r, blockR(b));
            if (L == blockL(b) && R == blockR(b)) {
                ans = max(ans, mx[b] + lazy[b]);
            } else {
                for (int i = L; i < R; ++i) ans = max(ans, a[i] + lazy[b]);
            }
        }
        return ans;
    }

    ll rangeSum(int l, int r) const {
        if (l >= r) return 0;
        ll ans = 0;
        const int bl = l / SQ_B;
        const int br = (r - 1) / SQ_B;
        for (int b = bl; b <= br; ++b) {
            const int L = max(l, blockL(b));
            const int R = min(r, blockR(b));
            if (L == blockL(b) && R == blockR(b)) {
                ans += sm[b] + 1LL * lazy[b] * (R - L);
            } else {
                for (int i = L; i < R; ++i) ans += a[i] + lazy[b];
            }
        }
        return ans;
    }

    int point(int i) const {
        const int b = i / SQ_B;
        return a[i] + lazy[b];
    }
};

struct Plan {
    int M = 0;
    int K = 0;
    const vector<Group>* groups = nullptr;
    array<SqrtRange, MAX_S> load{};
    array<Choice, MAX_M> choice{};
    ll revenue = 0;

    Plan() = default;

    Plan(int M_, int K_, const vector<Group>* groups_) { init(M_, K_, groups_); }

    void init(int M_, int K_, const vector<Group>* groups_) {
        M = M_;
        K = K_;
        groups = groups_;
        revenue = 0;
        for (int s = 0; s < MAX_S; ++s) load[s].init(M);
        for (int i = 0; i < MAX_M; ++i) choice[i] = Choice{};
    }

    bool canAdd(int i, int shelf, const ShapeOpt& sh) const {
        const Group& g = (*groups)[i];
        return load[shelf].rangeMax(i, g.endIdx) + sh.w <= BOARD_W;
    }

    void setChoice(int i, int shelf, const ShapeOpt& sh) {
        const Group& g = (*groups)[i];
        if (choice[i].accepted()) {
            const Choice old = choice[i];
            load[old.shelf].rangeAdd(i, g.endIdx, -old.sh.w);
            revenue -= old.sh.revenue;
        }
        choice[i].shelf = shelf;
        choice[i].sh = sh;
        load[shelf].rangeAdd(i, g.endIdx, sh.w);
        revenue += sh.revenue;
    }

    void reject(int i) {
        if (!choice[i].accepted()) return;
        const Group& g = (*groups)[i];
        const Choice old = choice[i];
        load[old.shelf].rangeAdd(i, g.endIdx, -old.sh.w);
        revenue -= old.sh.revenue;
        choice[i] = Choice{};
    }

    bool feasible() const {
        for (int s = 0; s < K; ++s) {
            if (load[s].rangeMax(0, M) > BOARD_W) return false;
        }
        return true;
    }
};

struct Config {
    vector<int> H;
    vector<int> y0;
};

using Catalog = vector<vector<vector<ShapeOpt>>>; // [group][shelf][Pareto option]

struct FutureStats {
    vector<ll> bestRevenue;
    vector<int> bestWidth;
    vector<ld> scarcitySum;
    vector<ld> bestAdjusted;
    vector<ld> bestRatio;
};

static ll rounded(ld x) {
    return (ll)floor(x + 0.5L);
}

static Config makeConfig(vector<int> heights) {
    Config c;
    c.H = std::move(heights);
    c.y0.resize(c.H.size());
    int y = 0;
    for (int s = 0; s < (int)c.H.size(); ++s) {
        c.y0[s] = y;
        y += c.H[s];
    }
    return c;
}

static Catalog buildCatalog(const vector<Group>& g, const Config& cfg) {
    const int M = (int)g.size();
    const int K = (int)cfg.H.size();
    Catalog cat(M, vector<vector<ShapeOpt>>(K));

    for (int i = 0; i < M; ++i) {
        for (int s = 0; s < K; ++s) {
            map<int, ShapeOpt> bestByWidth;
            const int hmax = min(cfg.H[s], g[i].P);
            for (int h = 1; h <= hmax; ++h) {
                const int w = (g[i].P + h - 1) / h;
                const int L = 2 * (h + w);
                const ld C = 4.0L * sqrt((ld)g[i].P) / (ld)L;
                if (w > BOARD_W) continue;
                ShapeOpt o{h, w, L, rounded((ld)g[i].V * C)};
                auto it = bestByWidth.find(w);
                if (it == bestByWidth.end() || o.revenue > it->second.revenue ||
                    (o.revenue == it->second.revenue && o.perimeter < it->second.perimeter)) {
                    bestByWidth[w] = o;
                }
            }
            ll bestRevSoFar = -1;
            for (auto& [w, o] : bestByWidth) {
                if (o.revenue > bestRevSoFar) {
                    cat[i][s].push_back(o);
                    bestRevSoFar = o.revenue;
                }
            }
        }
    }
    return cat;
}

static FutureStats buildFutureStats(const vector<Group>& g, const Config& cfg,
                                    const Catalog& cat) {
    const int M = (int)g.size();
    const int K = (int)cfg.H.size();
    FutureStats fs;
    fs.bestRevenue.assign(M, 0);
    fs.bestWidth.assign(M, 1);
    fs.scarcitySum.assign(M, 0);
    fs.bestAdjusted.assign(M, 0);
    fs.bestRatio.assign(M, 0);

    for (int i = 0; i < M; ++i) {
        ll br = 0;
        int bw = BOARD_W + 1;
        for (int s = 0; s < K; ++s) {
            for (const auto& o : cat[i][s]) {
                if (o.revenue > br || (o.revenue == br && o.w < bw)) {
                    br = o.revenue;
                    bw = o.w;
                }
            }
        }
        fs.bestRevenue[i] = br;
        fs.bestWidth[i] = bw;
    }

    vector<ld> lambda(M, 0.0L);
    const int capacity = K * BOARD_W;
    vector<pair<ld, int>> items;
    items.reserve(M);
    for (int t = 0; t < M; ++t) {
        items.clear();
        int totalWidth = 0;
        for (int i = 0; i <= t; ++i) {
            if (t >= g[i].endIdx) continue;
            if (fs.bestRevenue[i] <= 0 || fs.bestWidth[i] > BOARD_W) continue;
            const int len = max(1, g[i].endIdx - i);
            const ld density = (ld)fs.bestRevenue[i] /
                               ((ld)fs.bestWidth[i] * (ld)len);
            items.push_back({density, fs.bestWidth[i]});
            totalWidth += fs.bestWidth[i];
        }
        if (totalWidth <= capacity) {
            lambda[t] = 0.0L;
            continue;
        }
        sort(items.begin(), items.end(), [](const auto& a, const auto& b) {
            return a.first > b.first;
        });
        int used = 0;
        ld threshold = 0.0L;
        for (auto [d, w] : items) {
            used += w;
            threshold = d;
            if (used >= capacity) break;
        }
        lambda[t] = threshold;
    }

    vector<ld> pref(M + 1, 0.0L);
    for (int t = 0; t < M; ++t) pref[t + 1] = pref[t] + lambda[t];
    for (int i = 0; i < M; ++i) {
        fs.scarcitySum[i] = pref[g[i].endIdx] - pref[i];
        ld bestAdj = 0.0L;
        ld bestRat = 0.0L;
        for (int s = 0; s < K; ++s) {
            for (const auto& o : cat[i][s]) {
                const ld cost = (ld)o.w * fs.scarcitySum[i];
                bestAdj = max(bestAdj, (ld)o.revenue - cost);
                bestRat = max(bestRat, (ld)o.revenue / (1.0L + cost));
            }
        }
        fs.bestAdjusted[i] = bestAdj;
        fs.bestRatio[i] = bestRat;
    }
    return fs;
}

struct Pick {
    bool ok = false;
    int shelf = -1;
    ShapeOpt sh{};
    ld adjusted = -1e100L;
    ll pressure = 0;
};

static Pick bestFeasiblePick(const Plan& p, int i, const Catalog& cat,
                             const FutureStats& fs, ld alpha) {
    Pick best;
    const Group& g = (*p.groups)[i];
    for (int s = 0; s < p.K; ++s) {
        const int mx = p.load[s].rangeMax(i, g.endIdx);
        const ll loadSum = p.load[s].rangeSum(i, g.endIdx);
        for (const auto& o : cat[i][s]) {
            if (mx + o.w > BOARD_W) continue;
            const ld adj = (ld)o.revenue - alpha * (ld)o.w * fs.scarcitySum[i];
            const ll pressure = loadSum * o.w + 1LL * (mx + o.w) * (mx + o.w);
            bool take = false;
            if (!best.ok || adj > best.adjusted + 1e-12L) take = true;
            else if (fabsl(adj - best.adjusted) <= 1e-12L) {
                if (o.revenue > best.sh.revenue) take = true;
                else if (o.revenue == best.sh.revenue && pressure < best.pressure) take = true;
            }
            if (take) {
                best.ok = true;
                best.shelf = s;
                best.sh = o;
                best.adjusted = adj;
                best.pressure = pressure;
            }
        }
    }
    return best;
}

static void fillRejected(Plan& p, const Catalog& cat, const FutureStats& fs,
                         vector<int> order) {
    sort(order.begin(), order.end(), [&](int a, int b) {
        if (fs.bestRevenue[a] != fs.bestRevenue[b])
            return fs.bestRevenue[a] > fs.bestRevenue[b];
        return fs.bestRatio[a] > fs.bestRatio[b];
    });
    for (int i : order) {
        if (p.choice[i].accepted()) continue;
        Pick q = bestFeasiblePick(p, i, cat, fs, 0.0L);
        if (q.ok) p.setChoice(i, q.shelf, q.sh);
    }
}

static void upgradePlan(Plan& p, const Catalog& cat, const FutureStats& fs,
                        mt19937_64& rng, int passes = 2) {
    vector<int> ids(p.M);
    iota(ids.begin(), ids.end(), 0);
    for (int pass = 0; pass < passes; ++pass) {
        if (pass == 0) {
            sort(ids.begin(), ids.end(), [&](int a, int b) {
                const ll ga = fs.bestRevenue[a] - (p.choice[a].accepted() ? p.choice[a].sh.revenue : 0);
                const ll gb = fs.bestRevenue[b] - (p.choice[b].accepted() ? p.choice[b].sh.revenue : 0);
                return ga > gb;
            });
        } else {
            shuffle(ids.begin(), ids.end(), rng);
        }
        for (int i : ids) {
            if (!p.choice[i].accepted()) continue;
            const Choice old = p.choice[i];
            p.reject(i);
            Pick q = bestFeasiblePick(p, i, cat, fs, 0.0L);
            if (q.ok) p.setChoice(i, q.shelf, q.sh);
            else p.setChoice(i, old.shelf, old.sh); // defensive, should never occur
        }
    }
}

static Plan buildGreedyPlan(const vector<Group>& g, const Config& cfg,
                            const Catalog& cat, const FutureStats& fs,
                            int variant, mt19937_64& rng) {
    const int M = (int)g.size();
    const int K = (int)cfg.H.size();
    Plan p(M, K, &g);
    vector<int> order(M);
    iota(order.begin(), order.end(), 0);
    vector<ld> key(M, 0.0L);

    const array<ld, 6> alphaTable{0.95L, 0.10L, 0.60L, 0.80L, 1.10L, 0.45L};
    const ld alpha = alphaTable[min(variant, 5)];
    uniform_real_distribution<double> noise(-0.18, 0.18);

    for (int i = 0; i < M; ++i) {
        const int len = max(1, g[i].endIdx - i);
        const int safeWidth = min(fs.bestWidth[i], BOARD_W);
        const ld dualCost = (ld)safeWidth * fs.scarcitySum[i];
        switch (variant) {
            case 0:
                key[i] = fs.bestAdjusted[i];
                break;
            case 1:
                key[i] = (ld)fs.bestRevenue[i];
                break;
            case 2:
                key[i] = fs.bestRatio[i];
                break;
            case 3:
                key[i] = (ld)fs.bestRevenue[i] /
                         ((ld)safeWidth * sqrt((ld)len));
                break;
            case 4:
                key[i] = log1pl((ld)fs.bestRevenue[i]) - log1pl(dualCost) + (ld)noise(rng);
                break;
            default:
                key[i] = (ld)fs.bestRevenue[i] /
                         (1.0L + (ld)safeWidth * log1pl((ld)len)) +
                         (ld)noise(rng) * 0.01L;
                break;
        }
    }
    stable_sort(order.begin(), order.end(), [&](int a, int b) {
        if (key[a] != key[b]) return key[a] > key[b];
        return fs.bestRevenue[a] > fs.bestRevenue[b];
    });

    for (int i : order) {
        Pick q = bestFeasiblePick(p, i, cat, fs, alpha);
        if (!q.ok) continue;
        if (q.adjusted > 0.0L || variant == 1 || variant == 5) {
            p.setChoice(i, q.shelf, q.sh);
        }
    }

    vector<int> all(M);
    iota(all.begin(), all.end(), 0);
    fillRejected(p, cat, fs, all);
    upgradePlan(p, cat, fs, rng, 2);
    fillRejected(p, cat, fs, all);
    return p;
}

struct RepairAction {
    bool ok = false;
    int victim = -1;
    int newShelf = -1; // -1 means reject
    ShapeOpt newShape{};
    ll loss = 0;
    ll gain = 0;
    ld metric = 0;
};

static bool betterRepairAction(const RepairAction& a, const RepairAction& b) {
    if (!a.ok) return false;
    if (!b.ok) return true;
    if (a.metric < b.metric - 1e-18L) return true;
    if (a.metric > b.metric + 1e-18L) return false;
    if (a.loss != b.loss) return a.loss < b.loss;
    return a.gain > b.gain;
}

static Plan bestInsertionRepair(const Plan& base, int candidate,
                                const Catalog& cat, Timer& timer, double deadline) {
    Plan best = base;
    const auto& g = *base.groups;

    struct TargetTry {
        int shelf = -1;
        ShapeOpt sh{};
        int overload = 0;
        ll pressure = 0;
        ld key = 0;
    };
    vector<TargetTry> targets;
    for (int s = 0; s < base.K; ++s) {
        const int mx = base.load[s].rangeMax(candidate, g[candidate].endIdx);
        const ll sm = base.load[s].rangeSum(candidate, g[candidate].endIdx);
        for (const ShapeOpt& o : cat[candidate][s]) {
            const int over = max(0, mx + o.w - BOARD_W);
            TargetTry t;
            t.shelf = s;
            t.sh = o;
            t.overload = over;
            t.pressure = sm * o.w;
            t.key = (ld)o.revenue / (1.0L + (ld)over * (ld)over);
            targets.push_back(t);
        }
    }
    sort(targets.begin(), targets.end(), [](const TargetTry& a, const TargetTry& b) {
        const bool fa = (a.overload == 0), fb = (b.overload == 0);
        if (fa != fb) return fa > fb;
        if (a.key != b.key) return a.key > b.key;
        if (a.sh.revenue != b.sh.revenue) return a.sh.revenue > b.sh.revenue;
        return a.pressure < b.pressure;
    });
    if ((int)targets.size() > 8) targets.resize(8);

    for (const TargetTry& tt : targets) {
        if (timer.elapsed() > deadline) break;
        const int target = tt.shelf;
        const ShapeOpt candShape = tt.sh;
        Plan trial = base;
        trial.setChoice(candidate, target, candShape); // may temporarily overload
        array<unsigned char, MAX_M> touched{};
        bool failed = false;

        for (int step = 0; step < 16; ++step) {
            int worst = BOARD_W;
            int tWorst = -1;
            for (int t = candidate; t < g[candidate].endIdx; ++t) {
                const int v = trial.load[target].point(t);
                if (v > worst) {
                    worst = v;
                    tWorst = t;
                }
            }
            if (tWorst < 0) break;

            if (timer.elapsed() > deadline) { failed = true; break; }

            // Prefix sums of overload reduction for each possible released width.
            static ll gainPref[BOARD_W + 1][MAX_M + 1];
            for (int rel = 1; rel <= BOARD_W; ++rel) {
                gainPref[rel][candidate] = 0;
                for (int t = candidate; t < g[candidate].endIdx; ++t) {
                    const int over = max(0, trial.load[target].point(t) - BOARD_W);
                    gainPref[rel][t + 1] = gainPref[rel][t] + min(over, rel);
                }
            }

            RepairAction bestAct;
            for (int v = 0; v < trial.M; ++v) {
                if (v == candidate || touched[v]) continue;
                const Choice cur = trial.choice[v];
                if (!cur.accepted() || cur.shelf != target) continue;
                if (!(v <= tWorst && tWorst < g[v].endIdx)) continue;

                auto consider = [&](int newShelf, const ShapeOpt* newShape,
                                    int relief, ll loss) {
                    if (relief <= 0) return;
                    relief = min(relief, BOARD_W);
                    const int l = max(candidate, v);
                    const int r = min(g[candidate].endIdx, g[v].endIdx);
                    const ll gain = gainPref[relief][r] - gainPref[relief][l];
                    if (gain <= 0) return;
                    RepairAction a;
                    a.ok = true;
                    a.victim = v;
                    a.newShelf = newShelf;
                    if (newShape) a.newShape = *newShape;
                    a.loss = loss;
                    a.gain = gain;
                    a.metric = (ld)loss / (ld)gain;
                    if (betterRepairAction(a, bestAct)) bestAct = a;
                };

                // Reject victim.
                consider(-1, nullptr, cur.sh.w, cur.sh.revenue);

                // Compress within the same shelf.
                for (const auto& o : cat[v][target]) {
                    if (o.w >= cur.sh.w) continue;
                    consider(target, &o, cur.sh.w - o.w,
                             cur.sh.revenue - o.revenue);
                }

                // Reassign the whole lifetime to another shelf. Find the best
                // feasible destination first; source relief is identical.
                bool foundDst = false;
                int bestShelf = -1;
                ShapeOpt bestDst;
                for (int dst = 0; dst < trial.K; ++dst) {
                    if (dst == target) continue;
                    const int mxDst = trial.load[dst].rangeMax(v, g[v].endIdx);
                    for (const auto& o : cat[v][dst]) {
                        if (mxDst + o.w > BOARD_W) continue;
                        if (!foundDst || o.revenue > bestDst.revenue) {
                            foundDst = true;
                            bestShelf = dst;
                            bestDst = o;
                        }
                    }
                }
                if (foundDst) {
                    consider(bestShelf, &bestDst, cur.sh.w,
                             cur.sh.revenue - bestDst.revenue);
                }
            }

            if (!bestAct.ok) {
                failed = true;
                break;
            }
            touched[bestAct.victim] = 1;
            if (bestAct.newShelf < 0) trial.reject(bestAct.victim);
            else trial.setChoice(bestAct.victim, bestAct.newShelf, bestAct.newShape);
        }

        if (!failed && trial.feasible() && trial.revenue > best.revenue) {
            best = std::move(trial);
        }
    }
    return best;
}

static void improvePlanLNS(Plan& p, const Catalog& cat, const FutureStats& fs,
                           mt19937_64& rng, Timer& timer, double deadline) {
    vector<int> ids(p.M);
    iota(ids.begin(), ids.end(), 0);

    for (int round = 0; round < 2; ++round) {
        vector<int> rejected;
        rejected.reserve(p.M);
        for (int i = 0; i < p.M; ++i) if (!p.choice[i].accepted()) rejected.push_back(i);
        sort(rejected.begin(), rejected.end(), [&](int a, int b) {
            const ld ka = (ld)fs.bestRevenue[a] + 0.25L * fs.bestAdjusted[a];
            const ld kb = (ld)fs.bestRevenue[b] + 0.25L * fs.bestAdjusted[b];
            if (ka != kb) return ka > kb;
            return a < b;
        });
        const int limit = min((round == 0 ? 180 : 100), (int)rejected.size());
        for (int z = 0; z < limit; ++z) {
            if ((z & 7) == 0 && timer.elapsed() > deadline) return;
            const int i = rejected[z];
            if (p.choice[i].accepted()) continue;
            Plan q = bestInsertionRepair(p, i, cat, timer, deadline);
            if (q.revenue > p.revenue) p = std::move(q);
        }
        fillRejected(p, cat, fs, ids);
        upgradePlan(p, cat, fs, rng, 1);
    }
}

struct CandidatePlan {
    Config cfg;
    Catalog cat;
    FutureStats fs;
    Plan plan;
};

struct TurnAction {
    bool accept = false;
    int newX = -1;
    vector<pair<int, int>> moves; // (group id, new x)
};

struct HistNode {
    shared_ptr<HistNode> parent;
    int gid = -1;
    bool accept = false;
    int newX = -1;
    vector<pair<int, int>> moves;
};

struct BeamState {
    ll score = 0;
    vector<int8_t> pos; // local group positions; -1 = not active
    shared_ptr<HistNode> hist;
};

struct BeamCandidate {
    ll score = 0;
    vector<int8_t> pos;
    shared_ptr<HistNode> parent;
    int gid = -1;
    bool accept = false;
    int newX = -1;
    vector<pair<int, int>> moves;
    ll layoutQuality = 0;
    ld rankScore = 0;
    uint64_t hash = 0;
};

static uint64_t splitmix64(uint64_t x) {
    x += 0x9e3779b97f4a7c15ULL;
    x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
    x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
    return x ^ (x >> 31);
}

static uint64_t blockMask(int x, int w) {
    if (w == 64) return ~0ULL;
    return ((1ULL << w) - 1ULL) << x;
}

static ll maskLayoutQuality(uint64_t occ) {
    int largest = 0, gaps = 0, cur = 0;
    ll sumsq = 0;
    for (int x = 0; x <= BOARD_W; ++x) {
        const bool freeCell = (x < BOARD_W) && (((occ >> x) & 1ULL) == 0);
        if (freeCell) {
            ++cur;
        } else if (cur > 0) {
            ++gaps;
            largest = max(largest, cur);
            sumsq += 1LL * cur * cur;
            cur = 0;
        }
    }
    return 100000LL * largest + 100LL * sumsq - 1000LL * gaps;
}

static uint64_t stateHash(const vector<int8_t>& pos, int upto) {
    uint64_t h = 0x123456789abcdef0ULL;
    for (int i = 0; i <= upto; ++i) {
        if (pos[i] < 0) continue;
        h ^= splitmix64((uint64_t)(i + 1) * 1315423911ULL + (uint64_t)(pos[i] + 1));
    }
    return h;
}

struct RepackResult {
    bool ok = false;
    ll cost = 0;
    int newX = -1;
    vector<pair<int, int>> movedLocal; // (local index, new x)
};

struct DPPrev {
    int16_t pk = -1;
    int8_t pz = -1;
    int8_t pe = -1;
    int16_t item = -3; // -1 = new group, >=0 = local existing index
    int8_t x = -1;
};

struct DPCost {
    ll cost = INF64;
    int movedCount = INT_MAX;
};

static bool betterCost(const DPCost& a, const DPCost& b) {
    if (a.cost != b.cost) return a.cost < b.cost;
    return a.movedCount < b.movedCount;
}

static RepackResult repackPreservingOrder(const BeamState& st, int curLocal,
                                          const vector<int>& ids,
                                          const Plan& plan,
                                          const vector<Group>& groups) {
    vector<int> active;
    for (int j = 0; j < curLocal; ++j) if (st.pos[j] >= 0) active.push_back(j);
    sort(active.begin(), active.end(), [&](int a, int b) {
        return st.pos[a] < st.pos[b];
    });
    const int n = (int)active.size();
    const int newGid = ids[curLocal];
    const int newW = plan.choice[newGid].sh.w;

    static DPCost dp[52][2][51];
    static DPPrev pre[52][2][51];
    for (int k = 0; k <= n; ++k) {
        for (int z = 0; z < 2; ++z) {
            for (int e = 0; e <= BOARD_W; ++e) {
                dp[k][z][e] = DPCost{};
                pre[k][z][e] = DPPrev{};
            }
        }
    }
    dp[0][0][0] = DPCost{0, 0};

    auto relax = [&](int nk, int nz, int ne, const DPCost& nc,
                     int pk, int pz, int pe, int item, int x) {
        if (betterCost(nc, dp[nk][nz][ne])) {
            dp[nk][nz][ne] = nc;
            pre[nk][nz][ne].pk = (int16_t)pk;
            pre[nk][nz][ne].pz = (int8_t)pz;
            pre[nk][nz][ne].pe = (int8_t)pe;
            pre[nk][nz][ne].item = (int16_t)item;
            pre[nk][nz][ne].x = (int8_t)x;
        }
    };

    for (int k = 0; k <= n; ++k) {
        for (int z = 0; z < 2; ++z) {
            for (int e = 0; e <= BOARD_W; ++e) {
                const DPCost cur = dp[k][z][e];
                if (cur.cost >= INF64 / 2) continue;

                if (z == 0) {
                    for (int x = e; x + newW <= BOARD_W; ++x) {
                        relax(k, 1, x + newW, cur, k, z, e, -1, x);
                    }
                }

                if (k < n) {
                    const int loc = active[k];
                    const int gid = ids[loc];
                    const int w = plan.choice[gid].sh.w;
                    for (int x = e; x + w <= BOARD_W; ++x) {
                        DPCost nc = cur;
                        if (x != st.pos[loc]) {
                            nc.cost += groups[gid].moveCost;
                            ++nc.movedCount;
                        }
                        relax(k + 1, z, x + w, nc, k, z, e, loc, x);
                    }
                }
            }
        }
    }

    int bestE = -1;
    DPCost best;
    for (int e = 0; e <= BOARD_W; ++e) {
        if (betterCost(dp[n][1][e], best)) {
            best = dp[n][1][e];
            bestE = e;
        }
    }
    if (bestE < 0 || best.cost >= INF64 / 2) return RepackResult{};

    vector<int> assigned(ids.size(), -1);
    int k = n, z = 1, e = bestE;
    while (!(k == 0 && z == 0 && e == 0)) {
        const DPPrev p = pre[k][z][e];
        if (p.pk < 0) return RepackResult{};
        if (p.item == -1) assigned[curLocal] = p.x;
        else if (p.item >= 0) assigned[p.item] = p.x;
        k = p.pk;
        z = p.pz;
        e = p.pe;
    }

    RepackResult rr;
    rr.ok = true;
    rr.cost = best.cost;
    rr.newX = assigned[curLocal];
    for (int loc : active) {
        if (assigned[loc] < 0) return RepackResult{};
        if (assigned[loc] != st.pos[loc]) rr.movedLocal.push_back({loc, assigned[loc]});
    }
    return rr;
}

static uint64_t occupancyMask(const vector<int8_t>& pos, int upto,
                              const vector<int>& ids, const Plan& plan) {
    uint64_t occ = 0;
    for (int j = 0; j <= upto; ++j) {
        if (pos[j] < 0) continue;
        const int gid = ids[j];
        occ |= blockMask(pos[j], plan.choice[gid].sh.w);
    }
    return occ;
}

static ll candidateLayoutQuality(const vector<int8_t>& pos, int upto,
                                 const vector<int>& ids, const Plan& plan,
                                 const vector<Group>& groups) {
    const uint64_t occ = occupancyMask(pos, upto, ids, plan);
    ll q = maskLayoutQuality(occ);
    vector<int> act;
    for (int j = 0; j <= upto; ++j) if (pos[j] >= 0) act.push_back(j);
    sort(act.begin(), act.end(), [&](int a, int b) { return pos[a] < pos[b]; });
    for (int z = 1; z < (int)act.size(); ++z) {
        const int a = ids[act[z - 1]], b = ids[act[z]];
        q -= min(500, abs(groups[a].endIdx - groups[b].endIdx));
    }
    return q;
}

static ld estimateRisk(const BeamCandidate& c, int curLocal,
                       const vector<int>& ids, const Plan& plan,
                       const vector<Group>& groups) {
    const int Kloc = (int)ids.size();
    ld risk = 0.0L;
    const int lim = min(Kloc, curLocal + 7);
    for (int f = curLocal + 1; f < lim; ++f) {
        const int futureGid = ids[f];
        uint64_t occ = 0;
        ll minMove = INF64;
        for (int j = 0; j <= curLocal; ++j) {
            if (c.pos[j] < 0) continue;
            const int gid = ids[j];
            if (groups[gid].T < groups[futureGid].S) continue;
            occ |= blockMask(c.pos[j], plan.choice[gid].sh.w);
            minMove = min(minMove, groups[gid].moveCost);
        }
        int largest = 0, run = 0;
        for (int x = 0; x <= BOARD_W; ++x) {
            const bool freeCell = (x < BOARD_W) && (((occ >> x) & 1ULL) == 0);
            if (freeCell) ++run;
            else {
                largest = max(largest, run);
                run = 0;
            }
        }
        if (plan.choice[futureGid].sh.w > largest && minMove < INF64) {
            risk += (ld)minMove / (ld)(f - curLocal);
        }
    }
    return risk;
}

static vector<int> bestDirectPositions(const BeamState& st, int curLocal,
                                       const vector<int>& ids, const Plan& plan,
                                       const vector<Group>& groups) {
    const int gid = ids[curLocal];
    const int w = plan.choice[gid].sh.w;
    const uint64_t occ = occupancyMask(st.pos, curLocal - 1, ids, plan);
    array<int, BOARD_W> owner;
    owner.fill(-1);
    for (int j = 0; j < curLocal; ++j) {
        if (st.pos[j] < 0) continue;
        const int wj = plan.choice[ids[j]].sh.w;
        for (int x = st.pos[j]; x < st.pos[j] + wj; ++x) owner[x] = j;
    }

    vector<pair<ll, int>> cand;
    for (int x = 0; x + w <= BOARD_W; ++x) {
        const uint64_t m = blockMask(x, w);
        if (occ & m) continue;
        ll q = maskLayoutQuality(occ | m);
        if (x > 0 && owner[x - 1] >= 0) {
            q -= 25LL * min(1000, abs(groups[ids[owner[x - 1]]].endIdx - groups[gid].endIdx));
        }
        if (x + w < BOARD_W && owner[x + w] >= 0) {
            q -= 25LL * min(1000, abs(groups[ids[owner[x + w]]].endIdx - groups[gid].endIdx));
        }
        cand.push_back({q, x});
    }
    sort(cand.begin(), cand.end(), [](const auto& a, const auto& b) {
        if (a.first != b.first) return a.first > b.first;
        return a.second < b.second;
    });
    vector<int> xs;
    const int branch = min(8, (int)cand.size());
    for (int z = 0; z < branch; ++z) xs.push_back(cand[z].second);
    if (!cand.empty()) {
        int left = cand[0].second, right = cand[0].second;
        for (auto [q, x] : cand) {
            left = min(left, x);
            right = max(right, x);
        }
        if (find(xs.begin(), xs.end(), left) == xs.end()) xs.push_back(left);
        if (find(xs.begin(), xs.end(), right) == xs.end()) xs.push_back(right);
    }
    return xs;
}

static vector<BeamState> pruneBeam(vector<BeamCandidate>& cand, int curLocal,
                                   const vector<int>& ids, const Plan& plan,
                                   const vector<Group>& groups, int beamWidth) {
    // Deduplicate identical active layouts, retaining the highest score.
    unordered_map<uint64_t, int> at;
    at.reserve(cand.size() * 2 + 1);
    vector<BeamCandidate> uniq;
    uniq.reserve(cand.size());
    for (auto& c : cand) {
        c.hash = stateHash(c.pos, curLocal);
        auto it = at.find(c.hash);
        if (it == at.end()) {
            int idx = (int)uniq.size();
            at[c.hash] = idx;
            uniq.push_back(std::move(c));
        } else {
            BeamCandidate& d = uniq[it->second];
            if (c.score > d.score || (c.score == d.score && c.layoutQuality > d.layoutQuality)) {
                d = std::move(c);
            }
        }
    }

    sort(uniq.begin(), uniq.end(), [](const BeamCandidate& a, const BeamCandidate& b) {
        if (a.score != b.score) return a.score > b.score;
        return a.layoutQuality > b.layoutQuality;
    });

    const int poolSize = min<int>((int)uniq.size(), max(beamWidth * 4, beamWidth));
    vector<int> pool(poolSize);
    iota(pool.begin(), pool.end(), 0);
    for (int z = 0; z < poolSize; ++z) {
        BeamCandidate& c = uniq[z];
        const ld risk = estimateRisk(c, curLocal, ids, plan, groups);
        c.rankScore = (ld)c.score - 0.70L * risk + (ld)c.layoutQuality * 1e-9L;
    }

    vector<int> selected;
    selected.reserve(beamWidth);
    const int mainCount = max(1, beamWidth - 4);
    for (int z = 0; z < poolSize && (int)selected.size() < mainCount; ++z) selected.push_back(z);

    vector<int> byRank = pool;
    sort(byRank.begin(), byRank.end(), [&](int a, int b) {
        if (uniq[a].rankScore != uniq[b].rankScore) return uniq[a].rankScore > uniq[b].rankScore;
        return uniq[a].layoutQuality > uniq[b].layoutQuality;
    });
    for (int z : byRank) {
        if ((int)selected.size() >= beamWidth - 1) break;
        if (find(selected.begin(), selected.end(), z) == selected.end()) selected.push_back(z);
    }

    // Preserve one branch that rejects the current group, even when its
    // immediate score is outside the score-only pool.
    int bestReject = -1;
    for (int z = 0; z < (int)uniq.size(); ++z) {
        if (!uniq[z].accept &&
            (bestReject < 0 || uniq[z].score > uniq[bestReject].score ||
             (uniq[z].score == uniq[bestReject].score &&
              uniq[z].layoutQuality > uniq[bestReject].layoutQuality))) {
            bestReject = z;
        }
    }
    if (bestReject >= 0 && find(selected.begin(), selected.end(), bestReject) == selected.end()) {
        if ((int)selected.size() >= beamWidth) selected.back() = bestReject;
        else selected.push_back(bestReject);
    }
    for (int z = 0; z < poolSize && (int)selected.size() < beamWidth; ++z) {
        if (find(selected.begin(), selected.end(), z) == selected.end()) selected.push_back(z);
    }

    vector<BeamState> next;
    next.reserve(selected.size());
    for (int idx : selected) {
        BeamCandidate& c = uniq[idx];
        auto node = make_shared<HistNode>();
        node->parent = c.parent;
        node->gid = c.gid;
        node->accept = c.accept;
        node->newX = c.newX;
        node->moves = std::move(c.moves);
        BeamState st;
        st.score = c.score;
        st.pos = std::move(c.pos);
        st.hist = std::move(node);
        next.push_back(std::move(st));
    }
    return next;
}

struct PhysicalResult {
    ll score = 0;
    vector<TurnAction> turn;
};

static PhysicalResult simulatePhysical(const CandidatePlan& cp,
                                       const vector<Group>& groups,
                                       int beamWidth) {
    const int M = (int)groups.size();
    const int K = (int)cp.cfg.H.size();
    PhysicalResult result;
    result.turn.assign(M, TurnAction{});
    ll totalScore = 0;

    for (int shelf = 0; shelf < K; ++shelf) {
        vector<int> ids;
        for (int i = 0; i < M; ++i) {
            if (cp.plan.choice[i].accepted() && cp.plan.choice[i].shelf == shelf) ids.push_back(i);
        }
        if (ids.empty()) continue;
        const int L = (int)ids.size();

        BeamState init;
        init.score = 0;
        init.pos.assign(L, (int8_t)-1);
        vector<BeamState> beam{std::move(init)};

        for (int cur = 0; cur < L; ++cur) {
            const int gid = ids[cur];
            // Expire groups before processing this arrival.
            for (auto& st : beam) {
                for (int j = 0; j < cur; ++j) {
                    if (st.pos[j] >= 0 && groups[ids[j]].T < groups[gid].S) st.pos[j] = -1;
                }
            }

            vector<BeamCandidate> cand;
            cand.reserve(beam.size() * 12);
            for (const auto& st : beam) {
                // Reject branch.
                {
                    BeamCandidate c;
                    c.score = st.score;
                    c.pos = st.pos;
                    c.parent = st.hist;
                    c.gid = gid;
                    c.accept = false;
                    c.newX = -1;
                    c.layoutQuality = candidateLayoutQuality(c.pos, cur, ids, cp.plan, groups);
                    cand.push_back(std::move(c));
                }

                vector<int> xs = bestDirectPositions(st, cur, ids, cp.plan, groups);
                if (!xs.empty()) {
                    for (int x : xs) {
                        BeamCandidate c;
                        c.score = st.score + cp.plan.choice[gid].sh.revenue;
                        c.pos = st.pos;
                        c.pos[cur] = (int8_t)x;
                        c.parent = st.hist;
                        c.gid = gid;
                        c.accept = true;
                        c.newX = x;
                        c.layoutQuality = candidateLayoutQuality(c.pos, cur, ids, cp.plan, groups);
                        cand.push_back(std::move(c));
                    }
                } else {
                    RepackResult rr = repackPreservingOrder(st, cur, ids, cp.plan, groups);
                    if (rr.ok) {
                        BeamCandidate c;
                        c.score = st.score + cp.plan.choice[gid].sh.revenue - rr.cost;
                        c.pos = st.pos;
                        for (auto [loc, x] : rr.movedLocal) {
                            c.pos[loc] = (int8_t)x;
                            c.moves.push_back({ids[loc], x});
                        }
                        c.pos[cur] = (int8_t)rr.newX;
                        c.parent = st.hist;
                        c.gid = gid;
                        c.accept = true;
                        c.newX = rr.newX;
                        c.layoutQuality = candidateLayoutQuality(c.pos, cur, ids, cp.plan, groups);
                        cand.push_back(std::move(c));
                    }
                }
            }
            beam = pruneBeam(cand, cur, ids, cp.plan, groups, beamWidth);
            if (beam.empty()) {
                // Defensive fallback: reject all remaining groups in this shelf.
                BeamState st;
                st.score = 0;
                st.pos.assign(L, (int8_t)-1);
                beam.push_back(std::move(st));
            }
        }

        auto bestIt = max_element(beam.begin(), beam.end(), [](const BeamState& a, const BeamState& b) {
            return a.score < b.score;
        });
        totalScore += bestIt->score;
        for (shared_ptr<HistNode> p = bestIt->hist; p; p = p->parent) {
            TurnAction& tr = result.turn[p->gid];
            tr.accept = p->accept;
            tr.newX = p->newX;
            tr.moves = p->moves;
        }
    }

    result.score = max(0LL, totalScore);
    return result;
}

static vector<pair<int, int>> makeCells(const Group& g, const Choice& ch,
                                        const Config& cfg, int xPos) {
    vector<pair<int, int>> cells;
    cells.reserve(g.P);
    const int h = ch.sh.h;
    const int base = g.P / h;
    const int rem = g.P % h;
    for (int r = 0; r < h; ++r) {
        const int len = base + (r < rem ? 1 : 0);
        for (int c = 0; c < len; ++c) {
            cells.push_back({cfg.y0[ch.shelf] + r, xPos + c});
        }
    }
    return cells;
}

static bool validateSolution(const PhysicalResult& sol, const CandidatePlan& cp,
                             const vector<Group>& groups, int N) {
    const int M = (int)groups.size();
    vector<unsigned char> active(M, 0);
    vector<int> xpos(M, -1);
    ll score = 0;

    for (int i = 0; i < M; ++i) {
        for (int j = 0; j < i; ++j) {
            if (active[j] && groups[j].T < groups[i].S) active[j] = 0;
        }
        array<unsigned char, MAX_M> seenMove{};
        for (auto [j, x] : sol.turn[i].moves) {
            if (j < 0 || j >= i || !active[j] || seenMove[j]) return false;
            seenMove[j] = 1;
            if (!cp.plan.choice[j].accepted()) return false;
            if (x < 0 || x + cp.plan.choice[j].sh.w > BOARD_W) return false;
        }
        for (auto [j, x] : sol.turn[i].moves) {
            xpos[j] = x;
            score -= groups[j].moveCost;
        }
        if (sol.turn[i].accept) {
            if (!cp.plan.choice[i].accepted()) return false;
            const int x = sol.turn[i].newX;
            if (x < 0 || x + cp.plan.choice[i].sh.w > BOARD_W) return false;
            active[i] = 1;
            xpos[i] = x;
            score += cp.plan.choice[i].sh.revenue;
        }

        vector<int> board(N * N, -1);
        for (int j = 0; j <= i; ++j) {
            if (!active[j]) continue;
            const auto cells = makeCells(groups[j], cp.plan.choice[j], cp.cfg, xpos[j]);
            if ((int)cells.size() != groups[j].P) return false;
            for (auto [r, c] : cells) {
                if (r < 0 || r >= N || c < 0 || c >= N) return false;
                const int id = r * N + c;
                if (board[id] != -1) return false; // also catches duplicates inside one region
                board[id] = j;
            }
            // makeCells creates positive, left-aligned row lengths, hence every
            // row intersects the next row and the region is 4-neighbour connected.
        }
    }
    return max(0LL, score) == sol.score;
}

static void outputAllNo(int M) {
    for (int i = 0; i < M; ++i) {
        cout << 0 << '\n';
        cout << "No\n";
    }
}

static void outputSolution(const PhysicalResult& sol, const CandidatePlan& cp,
                           const vector<Group>& groups) {
    const int M = (int)groups.size();
    for (int i = 0; i < M; ++i) {
        cout << sol.turn[i].moves.size() << '\n';
        for (auto [j, x] : sol.turn[i].moves) {
            cout << j << '\n';
            const auto cells = makeCells(groups[j], cp.plan.choice[j], cp.cfg, x);
            for (auto [r, c] : cells) cout << r << ' ' << c << '\n';
        }
        if (!sol.turn[i].accept) {
            cout << "No\n";
        } else {
            cout << "Yes\n";
            const auto cells = makeCells(groups[i], cp.plan.choice[i], cp.cfg, sol.turn[i].newX);
            for (auto [r, c] : cells) cout << r << ' ' << c << '\n';
        }
    }
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    Timer timer;
    int N, M;
    ld R;
    if (!(cin >> N >> M >> R)) return 0;
    vector<string> park(N);
    for (string& row : park) cin >> row;
    bool pondFree = N == BOARD_W;
    for (const string& row : park) {
        pondFree &= (int)row.size() == N;
        pondFree &= count(row.begin(), row.end(), '#') == 0;
    }
    if (!pondFree) {
        outputAllNo(M);
        return 0;
    }

    vector<Group> groups(M);
    vector<int> starts(M);
    for (int i = 0; i < M; ++i) {
        int inputId;
        cin >> inputId >> groups[i].S >> groups[i].T >> groups[i].P >> groups[i].V;
        if (!cin || inputId != i) {
            outputAllNo(M);
            return 0;
        }
        starts[i] = groups[i].S;
        groups[i].moveCost = max(1LL, rounded((ld)groups[i].V * R));
    }
    for (int i = 0; i < M; ++i) {
        groups[i].endIdx = (int)(lower_bound(starts.begin(), starts.end(), groups[i].T) - starts.begin());
        groups[i].endIdx = max(groups[i].endIdx, i + 1);
    }

    // Fixed seed: reproducible multi-start ordering.
    mt19937_64 rng(0x6a09e667f3bcc909ULL);

    vector<Config> configs;
    // Versatile mixed layout: ten small-group shelves and five medium shelves.
    configs.push_back(makeConfig({2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
                                  6, 6, 6, 6, 6}));
    // Large-group oriented, nearly compact up to P = 150.
    configs.push_back(makeConfig({10, 10, 10, 10, 10}));
    // Balanced uniform shelves.
    configs.push_back(makeConfig({5, 5, 5, 5, 5, 5, 5, 5, 5, 5}));
    // Small-group oriented mixed layout; height-3 shelves still fit P = 150.
    configs.push_back(makeConfig({2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
                                  3, 3, 3, 3, 3, 3, 3, 3, 3, 3}));

    vector<CandidatePlan> candidates;
    candidates.reserve(configs.size());

    for (int ci = 0; ci < (int)configs.size(); ++ci) {
        if (ci > 0 && timer.elapsed() > 0.98) break;
        CandidatePlan cp;
        cp.cfg = configs[ci];
        cp.cat = buildCatalog(groups, cp.cfg);
        cp.fs = buildFutureStats(groups, cp.cfg, cp.cat);

        Plan best;
        ll bestRevenue = -1;
        int startsCount = (ci == 0 ? 4 : 3);
        if (timer.elapsed() > 0.82) startsCount = 2;
        for (int v = 0; v < startsCount; ++v) {
            Plan p = buildGreedyPlan(groups, cp.cfg, cp.cat, cp.fs, v, rng);
            if (p.revenue > bestRevenue) {
                bestRevenue = p.revenue;
                best = std::move(p);
            }
        }
        cp.plan = std::move(best);
        candidates.push_back(std::move(cp));
    }

    if (candidates.empty()) {
        outputAllNo(M);
        return 0;
    }

    vector<int> order(candidates.size());
    iota(order.begin(), order.end(), 0);
    sort(order.begin(), order.end(), [&](int a, int b) {
        return candidates[a].plan.revenue > candidates[b].plan.revenue;
    });

    // Spend the remaining planning budget on the most promising partitions.
    for (int z = 0; z < (int)order.size() && z < 2; ++z) {
        if (timer.elapsed() > 1.16) break;
        const int idx = order[z];
        const double localDeadline = min(1.24, timer.elapsed() + 0.12);
        improvePlanLNS(candidates[idx].plan, candidates[idx].cat,
                       candidates[idx].fs, rng, timer, localDeadline);
    }
    sort(order.begin(), order.end(), [&](int a, int b) {
        return candidates[a].plan.revenue > candidates[b].plan.revenue;
    });

    PhysicalResult bestSol;
    int bestCandidate = -1;
    for (int z = 0; z < (int)order.size(); ++z) {
        if (z > 0 && timer.elapsed() > 1.68) break;
        const int idx = order[z];
        int beamWidth = 14;
        if (timer.elapsed() > 1.35) beamWidth = 10;
        if (timer.elapsed() > 1.55) beamWidth = 6;
        PhysicalResult sol = simulatePhysical(candidates[idx], groups, beamWidth);
        if (bestCandidate < 0 || sol.score > bestSol.score) {
            bestCandidate = idx;
            bestSol = std::move(sol);
        }
    }

    if (bestCandidate < 0 || !validateSolution(bestSol, candidates[bestCandidate], groups, N)) {
        outputAllNo(M);
        return 0;
    }
    outputSolution(bestSol, candidates[bestCandidate], groups);
    return 0;
}
