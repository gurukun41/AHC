#include <bits/stdc++.h>

/*
 * AHC069 池なし・未来既知配置 oracle
 *
 * 通常の対話実行ではなく、入力ファイル全体を直接読み込む実験用実装である。
 * 全グループの滞在区間を先に使い、次の順で固定配置を計画する。
 *
 * 1. 各時刻の面積超過を、解消量当たりの失点が小さい組から外して解消する。
 * 2. 利用料の高い組から、コンパクトなテンプレートを固定配置する。
 * 3. 拒否組を全件走査し、「新規料金-衝突組の料金」が正なら入れ替える。
 * 4. 追い出された組も後続passで別位置へ再挿入し、局所的な配置順依存を直す。
 * 5. 敷き詰め評価は同じ料金の候補を比較する場合だけ使う。
 *
 * 出力はビジュアライザ用の完全な操作列であり、ローカルテスタ経由では実行しない。
 */

using namespace std;
using ll = long long;
using Cell = pair<int, int>;

struct Rect {
    int x;
    int y;
    int h;
    int w;
};

// 面積pを主矩形と端数の1行または1列で表す。
struct Shape {
    Rect main_rect;
    Rect extra_rect;
    int h;
    int w;
    int perimeter;
    ll relative_sum_x;
    ll relative_sum_y;
};

struct Request {
    int id;
    ll s;
    ll t;
    int p;
    ll v;
    ll best_fee = 0;
};

struct Placement {
    bool accepted = false;
    ll fee = 0;
    vector<Cell> cells;
};

struct Candidate {
    const Shape* shape = nullptr;
    int base_x = 0;
    int base_y = 0;
    ll score = numeric_limits<ll>::min();
};

struct PlannedRegion {
    vector<Cell> cells;
    int perimeter;
};

struct ReplacementCandidate {
    PlannedRegion region;
    vector<int> conflicts;
    ll delta = numeric_limits<ll>::min();
    ll layout_score = numeric_limits<ll>::min();
};

bool overlaps(const Request& lhs, const Request& rhs) {
    return lhs.s < rhs.t && rhs.s < lhs.t;
}

int rectangle_sum(const vector<vector<int>>& prefix,
                  int x,
                  int y,
                  int h,
                  int w) {
    if (h == 0 || w == 0) return 0;
    return prefix[x + h][y + w] - prefix[x][y + w] -
           prefix[x + h][y] + prefix[x][y];
}

ll rectangle_sum(const vector<vector<ll>>& prefix,
                 int x,
                 int y,
                 int h,
                 int w) {
    if (h == 0 || w == 0) return 0;
    return prefix[x + h][y + w] - prefix[x][y + w] -
           prefix[x + h][y] + prefix[x][y];
}

template <class T>
vector<vector<T>> make_prefix(const vector<vector<T>>& values) {
    int n = static_cast<int>(values.size());
    vector<vector<T>> prefix(n + 1, vector<T>(n + 1));
    for (int x = 0; x < n; ++x) {
        for (int y = 0; y < n; ++y) {
            prefix[x + 1][y + 1] =
                values[x][y] + prefix[x][y + 1] +
                prefix[x + 1][y] - prefix[x][y];
        }
    }
    return prefix;
}

vector<Shape> make_shapes(int p, int n) {
    vector<Shape> shapes;

    auto coordinate_sum = [](const Rect& rect, bool x_axis) {
        ll coordinate = x_axis ? rect.x : rect.y;
        ll length = x_axis ? rect.h : rect.w;
        ll copies = x_axis ? rect.w : rect.h;
        return copies * (length * coordinate + length * (length - 1) / 2);
    };
    auto add_shape = [&](Rect main_rect,
                         Rect extra_rect,
                         int h,
                         int w,
                         int perimeter) {
        if (h > n || w > n) return;
        ll sum_x = coordinate_sum(main_rect, true) +
                   coordinate_sum(extra_rect, true);
        ll sum_y = coordinate_sum(main_rect, false) +
                   coordinate_sum(extra_rect, false);
        shapes.push_back({main_rect, extra_rect, h, w, perimeter,
                          sum_x, sum_y});
    };

    for (int width = 1; width <= min(p, n); ++width) {
        int full = p / width;
        int rem = p % width;
        if (rem == 0) {
            add_shape({0, 0, full, width}, {0, 0, 0, 0},
                      full, width, 2 * (full + width));
            continue;
        }

        int perimeter = 2 * (full + width) + 2;
        for (int extra_below = 0; extra_below < 2; ++extra_below) {
            for (int extra_right = 0; extra_right < 2; ++extra_right) {
                Rect main_rect{extra_below ? 0 : 1, 0, full, width};
                Rect extra_rect{extra_below ? full : 0,
                                extra_right ? width - rem : 0,
                                1, rem};
                add_shape(main_rect, extra_rect, full + 1, width, perimeter);
            }
        }
        for (int extra_right = 0; extra_right < 2; ++extra_right) {
            for (int extra_bottom = 0; extra_bottom < 2; ++extra_bottom) {
                Rect main_rect{0, extra_right ? 0 : 1, width, full};
                Rect extra_rect{extra_bottom ? width - rem : 0,
                                extra_right ? full : 0,
                                rem, 1};
                add_shape(main_rect, extra_rect, width, full + 1, perimeter);
            }
        }
    }

    auto key = [](const Shape& shape) {
        return tuple(shape.perimeter,
                     shape.h,
                     shape.w,
                     shape.main_rect.x,
                     shape.main_rect.y,
                     shape.main_rect.h,
                     shape.main_rect.w,
                     shape.extra_rect.x,
                     shape.extra_rect.y,
                     shape.extra_rect.h,
                     shape.extra_rect.w);
    };
    sort(shapes.begin(), shapes.end(),
         [&](const Shape& lhs, const Shape& rhs) {
             return key(lhs) < key(rhs);
         });
    shapes.erase(unique(shapes.begin(), shapes.end(),
                        [&](const Shape& lhs, const Shape& rhs) {
                            return key(lhs) == key(rhs);
                        }),
                 shapes.end());
    return shapes;
}

vector<Cell> materialize(const Shape& shape, int base_x, int base_y) {
    vector<Cell> cells;
    auto append_rect = [&](const Rect& rect) {
        for (int dx = 0; dx < rect.h; ++dx) {
            for (int dy = 0; dy < rect.w; ++dy) {
                cells.emplace_back(base_x + rect.x + dx,
                                   base_y + rect.y + dy);
            }
        }
    };
    append_rect(shape.main_rect);
    append_rect(shape.extra_rect);
    return cells;
}

ll payment(ll value, int p, int perimeter) {
    long double compactness = 4.0L * sqrtl(static_cast<long double>(p)) /
                              perimeter;
    return static_cast<ll>(floorl(value * compactness + 0.5L));
}

// 時間容量だけを見たoracle受入集合。
// 全到着時刻の超過面積に対して、1点を失うことで解消できる超過量が大きい組を
// 外す。単純な料金/(人数×滞在時間)と異なり、空いている時間の占有は罰しない。
vector<char> select_by_exact_timeline(const vector<Request>& requests,
                                      int capacity) {
    int m = static_cast<int>(requests.size());
    vector<char> selected(m, true);
    vector<int> load(m);
    for (int event_id = 0; event_id < m; ++event_id) {
        ll time = requests[event_id].s;
        for (const Request& request : requests) {
            if (request.s <= time && time < request.t) {
                load[event_id] += request.p;
            }
        }
    }

    while (true) {
        bool has_overflow = false;
        for (int event_load : load) has_overflow |= event_load > capacity;
        if (!has_overflow) break;

        int remove_id = -1;
        long double best_loss_per_relief =
            numeric_limits<long double>::infinity();
        for (int j = 0; j < m; ++j) {
            const Request& request = requests[j];
            if (!selected[j]) continue;

            ll relief = 0;
            for (int event_id = 0; event_id < m; ++event_id) {
                ll time = requests[event_id].s;
                if (load[event_id] <= capacity ||
                    request.s > time || time >= request.t) {
                    continue;
                }
                relief += min(request.p, load[event_id] - capacity);
            }
            if (relief == 0) continue;

            long double loss_per_relief =
                static_cast<long double>(request.best_fee) / relief;
            if (loss_per_relief < best_loss_per_relief - 1e-18L ||
                (fabsl(loss_per_relief - best_loss_per_relief) <= 1e-18L &&
                 (remove_id == -1 || request.best_fee < requests[remove_id].best_fee))) {
                best_loss_per_relief = loss_per_relief;
                remove_id = j;
            }
        }
        if (remove_id == -1) break;
        selected[remove_id] = false;
        for (int event_id = 0; event_id < m; ++event_id) {
            ll time = requests[event_id].s;
            if (requests[remove_id].s <= time && time < requests[remove_id].t) {
                load[event_id] -= requests[remove_id].p;
            }
        }
    }

    // 除去順の貪欲性で空いた容量を、高料金の組から安全に埋め直す。
    vector<int> rejected;
    for (int i = 0; i < m; ++i) {
        if (!selected[i]) rejected.push_back(i);
    }
    sort(rejected.begin(), rejected.end(), [&](int lhs, int rhs) {
        return tuple(requests[lhs].best_fee, requests[lhs].p, -lhs) >
               tuple(requests[rhs].best_fee, requests[rhs].p, -rhs);
    });
    for (int request_id : rejected) {
        bool feasible = true;
        for (int event_id = 0; event_id < m; ++event_id) {
            ll time = requests[event_id].s;
            if (requests[request_id].s <= time && time < requests[request_id].t &&
                load[event_id] + requests[request_id].p > capacity) {
                feasible = false;
                break;
            }
        }
        if (!feasible) continue;
        selected[request_id] = true;
        for (int event_id = 0; event_id < m; ++event_id) {
            ll time = requests[event_id].s;
            if (requests[request_id].s <= time && time < requests[request_id].t) {
                load[event_id] += requests[request_id].p;
            }
        }
    }
    return selected;
}

optional<PlannedRegion> choose_fixed_region(
    int request_id,
    int n,
    const vector<string>& park,
    const vector<Request>& requests,
    const vector<vector<Shape>>& shapes,
    const vector<Placement>& placements,
    const vector<int>& use_count) {
    const Request& request = requests[request_id];
    vector<vector<int>> blocked(n, vector<int>(n));

    // この組と時間が重なる配置だけが、固定位置を選ぶ際の障害物になる。
    for (int x = 0; x < n; ++x) {
        for (int y = 0; y < n; ++y) {
            blocked[x][y] = park[x][y] == '#';
        }
    }
    for (int other_id = 0;
         other_id < static_cast<int>(placements.size());
         ++other_id) {
        if (!placements[other_id].accepted) continue;
        if (!overlaps(request, requests[other_id])) continue;
        for (auto [x, y] : placements[other_id].cells) {
            blocked[x][y] = 1;
        }
    }

    vector<vector<int>> reuse(n, vector<int>(n));
    vector<vector<ll>> contact(n, vector<ll>(n));
    constexpr int DX[4] = {-1, 1, 0, 0};
    constexpr int DY[4] = {0, 0, -1, 1};
    for (int x = 0; x < n; ++x) {
        for (int y = 0; y < n; ++y) {
            if (blocked[x][y]) continue;
            reuse[x][y] = use_count[x * n + y] > 0;
            for (int dir = 0; dir < 4; ++dir) {
                int nx = x + DX[dir];
                int ny = y + DY[dir];
                if (nx < 0 || nx >= n || ny < 0 || ny >= n) {
                    contact[x][y] += 2;  // まず盤面の縁から埋める。
                } else if (blocked[nx][ny]) {
                    contact[x][y] += 3;  // 同時利用領域との隙間を閉じる。
                }
            }
        }
    }

    vector<vector<int>> blocked_prefix = make_prefix(blocked);
    vector<vector<int>> reuse_prefix = make_prefix(reuse);
    vector<vector<ll>> contact_prefix = make_prefix(contact);

    auto shape_sum = [&](const auto& prefix,
                         const Shape& shape,
                         int base_x,
                         int base_y) {
        using Result = decay_t<decltype(prefix[0][0])>;
        Result result{};
        for (const Rect& rect : {shape.main_rect, shape.extra_rect}) {
            result += rectangle_sum(prefix,
                                    base_x + rect.x,
                                    base_y + rect.y,
                                    rect.h,
                                    rect.w);
        }
        return result;
    };

    int minimum_perimeter = shapes[request.p].front().perimeter;
    for (size_t first = 0; first < shapes[request.p].size();) {
        size_t last = first + 1;
        while (last < shapes[request.p].size() &&
               shapes[request.p][last].perimeter ==
                   shapes[request.p][first].perimeter) {
            ++last;
        }

        Candidate best;
        for (size_t shape_index = first; shape_index < last; ++shape_index) {
            const Shape& shape = shapes[request.p][shape_index];
            for (int base_x = 0; base_x + shape.h <= n; ++base_x) {
                for (int base_y = 0; base_y + shape.w <= n; ++base_y) {
                    if (shape_sum(blocked_prefix, shape, base_x, base_y) != 0) {
                        continue;
                    }
                    ll contact_score =
                        shape_sum(contact_prefix, shape, base_x, base_y);
                    ll reused_cells =
                        shape_sum(reuse_prefix, shape, base_x, base_y);
                    ll coordinate_sum =
                        static_cast<ll>(request.p) * (base_x + base_y) +
                        shape.relative_sum_x + shape.relative_sum_y;

                    // 接触辺を最優先し、その後に過去の固定領域の再利用、左上への
                    // 寄せを評価する。係数は各下位項が上位1単位を覆さない大きさ。
                    ll score = contact_score * 1000000LL +
                               reused_cells * 1000LL - coordinate_sum;
                    if (score > best.score) {
                        best = {&shape, base_x, base_y, score};
                    }
                }
            }
        }
        if (best.shape != nullptr) {
            return PlannedRegion{
                materialize(*best.shape, best.base_x, best.base_y),
                best.shape->perimeter};
        }

        // 最小周長+4までを通常候補とする。それでも置けない場合だけ、後続の
        // 周長tierも順に試す。未来既知でも細長い領域を無理に採用しないため、
        // 周長が最小値+12を超えたところで断る。
        if (shapes[request.p][first].perimeter > minimum_perimeter + 12) break;
        first = last;
    }
    return nullopt;
}

// 現在拒否されている1組について、既存配置と衝突してもよい全候補を調べる。
// 全anchorはセル当たりに按分した衝突料金で粗く順位付けし、上位だけについて
// 衝突する組を重複なく復元して、正確なスコア差分を計算する。
optional<ReplacementCandidate> find_best_replacement(
    int request_id,
    int n,
    const vector<string>& park,
    const vector<Request>& requests,
    const vector<vector<Shape>>& shapes,
    const vector<Placement>& placements,
    const vector<int>& use_count) {
    constexpr ll COST_SCALE = 1024;
    constexpr int ANCHOR_SHORTLIST_LIMIT = 64;

    const Request& request = requests[request_id];
    int m = static_cast<int>(requests.size());
    vector<vector<int>> pond(n, vector<int>(n));
    vector<vector<ll>> approximate_conflict(n, vector<ll>(n));
    vector<vector<ll>> contact(n, vector<ll>(n));
    vector<vector<int>> reuse(n, vector<int>(n));
    vector<vector<int>> cell_users(n * n);

    for (int x = 0; x < n; ++x) {
        for (int y = 0; y < n; ++y) {
            pond[x][y] = park[x][y] == '#';
            reuse[x][y] = use_count[x * n + y] > 0;
        }
    }
    for (int other_id = 0; other_id < m; ++other_id) {
        if (!placements[other_id].accepted) continue;
        ll unit_cost =
            (placements[other_id].fee * COST_SCALE +
             requests[other_id].p - 1) /
            requests[other_id].p;
        for (auto [x, y] : placements[other_id].cells) {
            cell_users[x * n + y].push_back(other_id);
            if (overlaps(request, requests[other_id])) {
                approximate_conflict[x][y] += unit_cost;
            }
        }
    }

    constexpr int DX[4] = {-1, 1, 0, 0};
    constexpr int DY[4] = {0, 0, -1, 1};
    for (int x = 0; x < n; ++x) {
        for (int y = 0; y < n; ++y) {
            if (pond[x][y]) continue;
            for (int dir = 0; dir < 4; ++dir) {
                int nx = x + DX[dir];
                int ny = y + DY[dir];
                if (nx < 0 || nx >= n || ny < 0 || ny >= n) {
                    contact[x][y] += 2;
                } else if (approximate_conflict[nx][ny] > 0) {
                    contact[x][y] += 1;
                }
            }
        }
    }

    vector<vector<int>> pond_prefix = make_prefix(pond);
    vector<vector<ll>> conflict_prefix = make_prefix(approximate_conflict);
    vector<vector<ll>> contact_prefix = make_prefix(contact);
    vector<vector<int>> reuse_prefix = make_prefix(reuse);

    auto shape_sum = [&](const auto& prefix,
                         const Shape& shape,
                         int base_x,
                         int base_y) {
        using Result = decay_t<decltype(prefix[0][0])>;
        Result result{};
        for (const Rect& rect : {shape.main_rect, shape.extra_rect}) {
            result += rectangle_sum(prefix,
                                    base_x + rect.x,
                                    base_y + rect.y,
                                    rect.h,
                                    rect.w);
        }
        return result;
    };

    struct ApproximateAnchor {
        const Shape* shape;
        int base_x;
        int base_y;
        ll approximate_delta;
        ll layout_score;
    };
    auto key = [](const ApproximateAnchor& anchor) {
        return tuple(anchor.approximate_delta,
                     anchor.layout_score,
                     -anchor.base_x,
                     -anchor.base_y);
    };
    auto better = [&](const ApproximateAnchor& lhs,
                      const ApproximateAnchor& rhs) {
        return key(lhs) > key(rhs);
    };
    // betterを比較関数にすると、topにはshortlist中の最悪候補が来る。
    priority_queue<ApproximateAnchor,
                   vector<ApproximateAnchor>,
                   decltype(better)>
        shortlist(better);

    int minimum_perimeter = shapes[request.p].front().perimeter;
    for (const Shape& shape : shapes[request.p]) {
        if (shape.perimeter > minimum_perimeter + 12) break;
        ll reward = payment(request.v, request.p, shape.perimeter);
        for (int base_x = 0; base_x + shape.h <= n; ++base_x) {
            for (int base_y = 0; base_y + shape.w <= n; ++base_y) {
                if (shape_sum(pond_prefix, shape, base_x, base_y) != 0) continue;
                ll approximate_loss =
                    shape_sum(conflict_prefix, shape, base_x, base_y);
                ll contact_score =
                    shape_sum(contact_prefix, shape, base_x, base_y);
                ll reused_cells =
                    shape_sum(reuse_prefix, shape, base_x, base_y);
                ll coordinate_sum =
                    static_cast<ll>(request.p) * (base_x + base_y) +
                    shape.relative_sum_x + shape.relative_sum_y;
                ApproximateAnchor anchor{
                    &shape,
                    base_x,
                    base_y,
                    reward * COST_SCALE - approximate_loss,
                    contact_score * 1000000LL +
                        reused_cells * 1000LL - coordinate_sum};
                if (static_cast<int>(shortlist.size()) <
                    ANCHOR_SHORTLIST_LIMIT) {
                    shortlist.push(anchor);
                } else if (better(anchor, shortlist.top())) {
                    shortlist.pop();
                    shortlist.push(anchor);
                }
            }
        }
    }

    vector<ApproximateAnchor> anchors;
    while (!shortlist.empty()) {
        anchors.push_back(shortlist.top());
        shortlist.pop();
    }
    sort(anchors.begin(), anchors.end(), better);

    optional<ReplacementCandidate> best;
    vector<int> seen(m, -1);
    int stamp = 0;
    for (const ApproximateAnchor& anchor : anchors) {
        ++stamp;
        vector<Cell> cells =
            materialize(*anchor.shape, anchor.base_x, anchor.base_y);
        vector<int> conflicts;
        ll conflict_fee = 0;
        for (auto [x, y] : cells) {
            for (int other_id : cell_users[x * n + y]) {
                if (seen[other_id] == stamp ||
                    !overlaps(request, requests[other_id])) {
                    continue;
                }
                seen[other_id] = stamp;
                conflicts.push_back(other_id);
                conflict_fee += placements[other_id].fee;
            }
        }
        ll reward = payment(request.v, request.p, anchor.shape->perimeter);
        ll delta = reward - conflict_fee;
        if (!best || delta > best->delta ||
            (delta == best->delta && anchor.layout_score > best->layout_score)) {
            best = ReplacementCandidate{
                PlannedRegion{std::move(cells), anchor.shape->perimeter},
                std::move(conflicts),
                delta,
                anchor.layout_score};
        }
    }
    return best;
}

void erase_placement(int request_id,
                     vector<Placement>& placements,
                     vector<int>& use_count,
                     int n) {
    Placement& placement = placements[request_id];
    for (auto [x, y] : placement.cells) {
        --use_count[x * n + y];
    }
    placement = Placement{};
}

void install_placement(int request_id,
                       PlannedRegion region,
                       const vector<Request>& requests,
                       vector<Placement>& placements,
                       vector<int>& use_count,
                       int n) {
    Placement& placement = placements[request_id];
    placement.accepted = true;
    placement.fee = payment(requests[request_id].v,
                            requests[request_id].p,
                            region.perimeter);
    placement.cells = std::move(region.cells);
    for (auto [x, y] : placement.cells) {
        ++use_count[x * n + y];
    }
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int n, m;
    long double relocation_rate;
    if (!(cin >> n >> m >> relocation_rate)) return 0;
    (void)relocation_rate;

    vector<string> park(n);
    for (string& row : park) cin >> row;
    int pond_cells = 0;
    for (const string& row : park) {
        pond_cells += static_cast<int>(count(row.begin(), row.end(), '#'));
    }
    if (pond_cells != 0) {
        cerr << "oracle error: this implementation requires a pond-free board\n";
        return 1;
    }

    vector<Request> requests(m);
    for (Request& request : requests) {
        cin >> request.id >> request.s >> request.t >> request.p >> request.v;
    }

    vector<vector<Shape>> shapes(151);
    for (int p = 4; p <= 150; ++p) {
        shapes[p] = make_shapes(p, n);
    }
    for (Request& request : requests) {
        int perimeter = shapes[request.p].front().perimeter;
        request.best_fee = payment(request.v, request.p, perimeter);
    }

    vector<char> selected = select_by_exact_timeline(requests, n * n);

    // 初期配置では料金を最優先する。conflict_pressureは同料金なら、後で置き直し
    // にくい組を先にするためだけに使い、料金判断を覆さない。
    vector<ll> conflict_pressure(m);
    for (int i = 0; i < m; ++i) {
        if (!selected[i]) continue;
        for (int j = i + 1; j < m; ++j) {
            if (!selected[j] || !overlaps(requests[i], requests[j])) continue;
            ll overlap_length = min(requests[i].t, requests[j].t) -
                                max(requests[i].s, requests[j].s);
            conflict_pressure[i] += overlap_length * requests[j].p;
            conflict_pressure[j] += overlap_length * requests[i].p;
        }
    }

    vector<int> order;
    for (int i = 0; i < m; ++i) {
        if (selected[i]) order.push_back(i);
    }
    sort(order.begin(), order.end(), [&](int lhs, int rhs) {
        return tuple(requests[lhs].best_fee,
                     conflict_pressure[lhs],
                     requests[lhs].p,
                     -lhs) >
               tuple(requests[rhs].best_fee,
                     conflict_pressure[rhs],
                     requests[rhs].p,
                     -rhs);
    });

    vector<Placement> placements(m);
    vector<int> use_count(n * n);
    for (int request_id : order) {
        optional<PlannedRegion> region = choose_fixed_region(
            request_id, n, park, requests, shapes,
            placements, use_count);
        if (!region) continue;
        install_placement(request_id, std::move(*region), requests,
                          placements, use_count, n);
    }

    ll score_before_lns = 0;
    for (const Placement& placement : placements) {
        if (placement.accepted) score_before_lns += placement.fee;
    }

    // 1回の置換は必ず正の正確スコア差分を持つ。追い出された組を次passで
    // 再挿入することで、単純な配置順による局所解を段階的に崩す。
    constexpr int LNS_PASS_LIMIT = 6;
    int applied_replacements = 0;
    for (int pass = 0; pass < LNS_PASS_LIMIT; ++pass) {
        vector<int> rejected;
        for (int i = 0; i < m; ++i) {
            if (!placements[i].accepted) rejected.push_back(i);
        }
        sort(rejected.begin(), rejected.end(), [&](int lhs, int rhs) {
            return tuple(requests[lhs].best_fee,
                         requests[lhs].p,
                         -lhs) >
                   tuple(requests[rhs].best_fee,
                         requests[rhs].p,
                         -rhs);
        });

        bool improved = false;
        for (int request_id : rejected) {
            if (placements[request_id].accepted) continue;
            optional<ReplacementCandidate> replacement =
                find_best_replacement(request_id, n, park, requests,
                                      shapes, placements, use_count);
            if (!replacement || replacement->delta <= 0) continue;

            for (int conflict_id : replacement->conflicts) {
                erase_placement(conflict_id, placements, use_count, n);
            }
            install_placement(request_id, std::move(replacement->region),
                              requests, placements, use_count, n);
            ++applied_replacements;
            improved = true;
        }
        if (!improved) break;
    }

    ll estimated_fee = 0;
    int accepted_count = 0;
    for (int i = 0; i < m; ++i) {
        cout << 0 << '\n';
        if (!placements[i].accepted) {
            cout << "No\n";
            continue;
        }
        ++accepted_count;
        estimated_fee += placements[i].fee;

        cout << "Yes\n";
        for (auto [x, y] : placements[i].cells) {
            cout << x << ' ' << y << '\n';
        }
    }

    cerr << "oracle accepted=" << accepted_count << '/' << m
         << " estimated_fee=" << estimated_fee
         << " lns_gain=" << estimated_fee - score_before_lns
         << " replacements=" << applied_replacements << '\n';
    return 0;
}
