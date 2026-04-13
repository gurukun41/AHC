#pragma GCC target("avx2")
#pragma GCC optimize("O3")
#pragma GCC optimize("unroll-loops")

#include <bits/stdc++.h>
using namespace std;
using ll = long long;
using vi = vector<int>;
using vb = vector<bool>;

struct Pos {
    int x, y;
};

constexpr int DX[4] = {-1, 1, 0, 0}; // U D L R
constexpr int DY[4] = {0, 0, -1, 1};
constexpr char DC[4] = {'U', 'D', 'L', 'R'};

inline int packPosFast(int x, int y) { return (x << 4) | y; }
inline int posXFast(int p) { return p >> 4; }
inline int posYFast(int p) { return p & 15; }

int N, M, C;
int d_arr[512];

// 状態構造体をサイズダウンし、メモリコピーのコストを劇的に下げる
struct alignas(32) FastState {
    uint8_t pos[260]; // 256で十分だが念のためマージン
    uint8_t col[260];
    uint8_t f[256];
    ll cutPenalty;
    ll cutMismatchGain;
    ll evalScore;
    int len;
    int pref;
    int matchCnt;
    int targetIdx;
    int used;
    int firstDir;
};

// 状態の1手進める関数 (memchrでさらに高速化)
inline int applyMoveLocalFast(int dir, FastState &s) {
    int hpos = s.pos[0];
    int nx = posXFast(hpos) + DX[dir], ny = posYFast(hpos) + DY[dir];
    int npos = packPosFast(nx, ny);

    uint8_t old_tail_pos = s.pos[s.len - 1];

    memmove(s.pos + 1, s.pos, s.len - 1);
    s.pos[0] = npos;

    // 食事処理
    if (s.f[npos] != 0) {
        uint8_t c = s.f[npos];
        s.f[npos] = 0;
        s.pos[s.len] = old_tail_pos;
        s.col[s.len] = c;
        s.len++;
    }

    int k = s.len;
    int h = -1;
    
    // 噛みちぎり判定 (memchrによる高速検索)
    if (k > 2) {
        void* ptr = memchr(s.pos + 1, npos, k - 2);
        if (ptr != nullptr) {
            h = (int)((uint8_t*)ptr - s.pos);
        }
    }

    // 噛みちぎり処理
    if (h != -1) {
        for (int p = h + 1; p < k; p++) {
            s.f[s.pos[p]] = s.col[p];
        }
        s.len = h + 1;
    }
    return h;
}

// === 高速な探索用ユーティリティ ===

inline int getSafeDirFast(const FastState &s) {
    uint8_t occ[256] = {0};
    for (int i = 0; i < s.len - 1; i++) occ[s.pos[i]] = 1;

    int hx = posXFast(s.pos[0]), hy = posYFast(s.pos[0]);
    int ut_pos = (s.len >= 2) ? s.pos[1] : -1;

    for (int dir = 0; dir < 4; dir++) {
        int nx = hx + DX[dir], ny = hy + DY[dir];
        if (nx < 0 || nx >= N || ny < 0 || ny >= N) continue;
        int npos = packPosFast(nx, ny);
        if (npos == ut_pos || occ[npos]) continue;
        return dir;
    }
    for (int dir = 0; dir < 4; dir++) {
        int nx = hx + DX[dir], ny = hy + DY[dir];
        if (nx < 0 || nx >= N || ny < 0 || ny >= N) continue;
        int npos = packPosFast(nx, ny);
        if (npos == ut_pos) continue;
        return dir;
    }
    return 0;
}

inline int getDirToNearestColorFast(int targetColor, const FastState &s) {
    uint8_t occ[256] = {0};
    for (int i = 0; i < s.len - 1; i++) occ[s.pos[i]] = 1;

    int hpos = s.pos[0];
    int ut_pos = (s.len >= 2) ? s.pos[1] : -1;

    uint8_t dist[256];
    int8_t prevDir[256];
    memset(dist, 255, sizeof(dist));
    memset(prevDir, -1, sizeof(prevDir));

    int q[256];
    int qhead = 0, qtail = 0;

    q[qtail++] = hpos;
    dist[hpos] = 0;

    int target_pos = -1;
    while (qhead < qtail) {
        int curr = q[qhead++];
        if (curr != hpos && s.f[curr] == targetColor) {
            target_pos = curr;
            break;
        }

        int cx = posXFast(curr), cy = posYFast(curr);
        for (int dir = 0; dir < 4; dir++) {
            int nx = cx + DX[dir], ny = cy + DY[dir];
            if (nx < 0 || nx >= N || ny < 0 || ny >= N) continue;
            int npos = packPosFast(nx, ny);

            if (dist[npos] != 255 || (curr == hpos && npos == ut_pos) || occ[npos]) continue;

            dist[npos] = dist[curr] + 1;
            prevDir[npos] = dir;
            q[qtail++] = npos;
        }
    }
    if (target_pos == -1) return -1;

    int curr = target_pos;
    while (curr != hpos) {
        int dir = prevDir[curr];
        int px = posXFast(curr) - DX[dir], py = posYFast(curr) - DY[dir];
        int ppos = packPosFast(px, py);
        if (ppos == hpos) return dir;
        curr = ppos;
    }
    return -1;
}

inline int getDirToNearestFoodFast(const FastState &s) {
    uint8_t occ[256] = {0};
    for (int i = 0; i < s.len - 1; i++) occ[s.pos[i]] = 1;

    int hpos = s.pos[0];
    int ut_pos = (s.len >= 2) ? s.pos[1] : -1;

    uint8_t dist[256];
    int8_t prevDir[256];
    memset(dist, 255, sizeof(dist));
    memset(prevDir, -1, sizeof(prevDir));

    int q[256];
    int qhead = 0, qtail = 0;

    q[qtail++] = hpos;
    dist[hpos] = 0;

    int target_pos = -1;
    while (qhead < qtail) {
        int curr = q[qhead++];
        if (curr != hpos && s.f[curr] != 0) {
            target_pos = curr;
            break;
        }

        int cx = posXFast(curr), cy = posYFast(curr);
        for (int dir = 0; dir < 4; dir++) {
            int nx = cx + DX[dir], ny = cy + DY[dir];
            if (nx < 0 || nx >= N || ny < 0 || ny >= N) continue;
            int npos = packPosFast(nx, ny);

            if (dist[npos] != 255 || (curr == hpos && npos == ut_pos) || occ[npos]) continue;

            dist[npos] = dist[curr] + 1;
            prevDir[npos] = dir;
            q[qtail++] = npos;
        }
    }
    if (target_pos == -1) return -1;

    int curr = target_pos;
    while (curr != hpos) {
        int dir = prevDir[curr];
        int px = posXFast(curr) - DX[dir], py = posYFast(curr) - DY[dir];
        int ppos = packPosFast(px, py);
        if (ppos == hpos) return dir;
        curr = ppos;
    }
    return -1;
}

inline int getDirToCellFast(int tx, int ty, const FastState &s) {
    uint8_t occ[256] = {0};
    for (int i = 0; i < s.len - 1; i++) occ[s.pos[i]] = 1;

    int hpos = s.pos[0];
    int ut_pos = (s.len >= 2) ? s.pos[1] : -1;
    int tpos = packPosFast(tx, ty);

    uint8_t dist[256];
    int8_t prevDir[256];
    memset(dist, 255, sizeof(dist));
    memset(prevDir, -1, sizeof(prevDir));

    int q[256];
    int qhead = 0, qtail = 0;
    q[qtail++] = hpos;
    dist[hpos] = 0;

    while (qhead < qtail) {
        int curr = q[qhead++];
        if (curr == tpos) break;

        int cx = posXFast(curr), cy = posYFast(curr);
        for (int dir = 0; dir < 4; dir++) {
            int nx = cx + DX[dir], ny = cy + DY[dir];
            if (nx < 0 || nx >= N || ny < 0 || ny >= N) continue;
            int npos = packPosFast(nx, ny);

            if (dist[npos] != 255 || (curr == hpos && npos == ut_pos) || occ[npos]) continue;

            dist[npos] = dist[curr] + 1;
            prevDir[npos] = dir;
            q[qtail++] = npos;
        }
    }
    if (dist[tpos] == 255) return -1;

    int curr = tpos;
    while (curr != hpos) {
        int dir = prevDir[curr];
        int px = posXFast(curr) - DX[dir], py = posYFast(curr) - DY[dir];
        int ppos = packPosFast(px, py);
        if (ppos == hpos) return dir;
        curr = ppos;
    }
    return -1;
}

constexpr int MAX_OPS = 100000;
inline int searchOpsCap() { return 100000; }

// === ビームサーチの中核 ===

constexpr ll BEAM_PREF_WEIGHT = 10000LL;
constexpr ll BEAM_MATCH_POS_WEIGHT = 30LL;
constexpr ll BEAM_CUT_PENALTY_WEIGHT = 30000LL;
constexpr ll BEAM_CUT_MISMATCHED_WEIGHT = 300LL;

inline ll beamEvalFast(const FastState &s) {
    return (ll)s.used
         + BEAM_PREF_WEIGHT * (ll)(M - s.pref)
         + BEAM_MATCH_POS_WEIGHT * (ll)(M - s.matchCnt)
         + BEAM_CUT_PENALTY_WEIGHT * s.cutPenalty
         - BEAM_CUT_MISMATCHED_WEIGHT * s.cutMismatchGain;
}

int chooseBeamNextDirFast(const FastState &startState, int beamWidth = 40, int lookahead = 100) {
    static vector<FastState> cur, nxt, picked;
    static vector<int> idx;
    cur.clear(); nxt.clear(); picked.clear();
    cur.reserve(max(beamWidth, 16));
    nxt.reserve(max(beamWidth * 4, 64));
    picked.reserve(max(beamWidth, 16));

    cur.push_back(startState);
    cur[0].used = 0;
    cur[0].firstDir = -1;
    cur[0].evalScore = beamEvalFast(cur[0]);

    for (int depth = 0; depth < lookahead; depth++) {
        nxt.clear();
        for (const auto &nd : cur) {
            int hx = posXFast(nd.pos[0]), hy = posYFast(nd.pos[0]);

            for (int dir = 0; dir < 4; dir++) {
                int nx = hx + DX[dir], ny = hy + DY[dir];
                if (nx < 0 || nx >= N || ny < 0 || ny >= N) continue;
                int npos = packPosFast(nx, ny);

                if (nd.len >= 2 && nd.pos[1] == npos) continue;

                // コピーを避け、末尾に直接追加して操作する
                nxt.push_back(nd);
                FastState &z = nxt.back();

                int nextFood = z.f[npos];
                int lenBefore = z.len;

                int addedMatch = 0;
                if (nextFood != 0 && lenBefore < M && nextFood == d_arr[lenBefore]) addedMatch = 1;

                int biteIdx = applyMoveLocalFast(dir, z);
                z.used++;
                ll ev = nd.evalScore + 1LL;
                if (z.firstDir == -1) z.firstDir = dir;
                z.matchCnt += addedMatch;
                if (addedMatch) ev -= BEAM_MATCH_POS_WEIGHT;

                if (nextFood != 0 && z.pref == lenBefore && lenBefore < M && nextFood == d_arr[lenBefore]) {
                    z.pref++;
                    ev -= BEAM_PREF_WEIGHT;
                }

                if (biteIdx != -1) {
                    int removedMatch = 0;
                    int removedMismatch = 0;
                    for (int i = biteIdx + 1; i < lenBefore; i++) {
                        if (i < M && z.col[i] == d_arr[i]) removedMatch++;
                        if (i < M && z.col[i] != d_arr[i]) removedMismatch++;
                    }
                    if (nextFood != 0 && lenBefore < M && nextFood == d_arr[lenBefore]) removedMatch++;
                    z.matchCnt -= removedMatch;
                    ev += BEAM_MATCH_POS_WEIGHT * (ll)removedMatch;
                    z.cutMismatchGain += (ll)removedMismatch;
                    ev -= BEAM_CUT_MISMATCHED_WEIGHT * (ll)removedMismatch;

                    int newLen = biteIdx + 1;
                    if (z.pref > newLen) {
                        int prefDrop = z.pref - newLen;
                        z.cutPenalty += (ll)prefDrop;
                        z.pref = newLen;
                        ev += (BEAM_PREF_WEIGHT + BEAM_CUT_PENALTY_WEIGHT) * (ll)prefDrop;
                    }
                }
                if (z.targetIdx < M && nextFood == d_arr[z.targetIdx]) z.targetIdx++;

                z.evalScore = ev;
            }
        }

        if (nxt.empty()) break;

        int keep = min(beamWidth, (int)nxt.size());
        int keepAcc = 0;
        if (keep >= 8) keepAcc = max(1, keep / 10);
        if (keepAcc > keep) keepAcc = keep;
        int keepEval = keep - keepAcc;

        // インデックスのみでソートし、巨大な構造体のスワップを防止
        auto byAccIdx = [&](int a, int b) {
            const FastState &sa = nxt[a], &sb = nxt[b];
            if (sa.pref != sb.pref) return sa.pref > sb.pref;
            if (sa.matchCnt != sb.matchCnt) return sa.matchCnt > sb.matchCnt;
            return sa.evalScore < sb.evalScore;
        };
        auto byEvalIdx = [&](int a, int b) {
            const FastState &sa = nxt[a], &sb = nxt[b];
            if (sa.evalScore != sb.evalScore) return sa.evalScore < sb.evalScore;
            return sa.pref > sb.pref;
        };

        idx.resize(nxt.size());
        iota(idx.begin(), idx.end(), 0);
        picked.clear();

        if (keepAcc > 0) {
            nth_element(idx.begin(), idx.begin() + keepAcc - 1, idx.end(), byAccIdx);
            for (int i = 0; i < keepAcc; i++) picked.push_back(nxt[idx[i]]);
        }

        if (keepEval > 0) {
            auto remBegin = idx.begin() + keepAcc;
            auto remEnd = idx.end();
            int remSize = (int)(remEnd - remBegin);
            int take = min(keepEval, remSize);
            if (take > 0) {
                nth_element(remBegin, remBegin + take - 1, remEnd, byEvalIdx);
                for (int i = 0; i < take; i++) picked.push_back(nxt[*(remBegin + i)]);
            }
        }

        cur.swap(picked);
    }

    if (cur.empty()) return -1;
    auto it = min_element(cur.begin(), cur.end(),
                          [&](const FastState &a, const FastState &b) {
                              if (a.evalScore != b.evalScore) return a.evalScore < b.evalScore;
                              return a.pref > b.pref;
                          });
    return it->firstDir;
}

vi beamConstructMoves(const FastState &initialState,
                      double limitMs = 1700.0,
                      int beamWidth = 80,
                      int lookahead = 24,
                      int bestPerfectTurnsCap = -1) {
    auto st = chrono::steady_clock::now();

    FastState s = initialState;
    vi ans;
    int opsCap = searchOpsCap();
    if (bestPerfectTurnsCap >= 0) opsCap = min(opsCap, bestPerfectTurnsCap);
    ans.reserve(opsCap);

    s.pref = 0;
    int lim = min((int)s.len, M);
    while (s.pref < lim && s.col[s.pref] == d_arr[s.pref]) s.pref++;
    
    s.matchCnt = 0;
    for (int i = 0; i < lim; i++) if (s.col[i] == d_arr[i]) s.matchCnt++;

    s.targetIdx = max(5, s.pref);
    s.cutPenalty = 0;
    s.cutMismatchGain = 0;

    int remFood = 0;
    for (int i = 0; i < 256; i++) if (s.f[i] != 0) remFood++;
    bool collectAllMode = false;

    auto calcOfficialScore = [&](const FastState &stt, int usedOps) -> ll {
        int k = stt.len;
        int lim2 = min(k, M);
        int E = 0;
        for (int i = 0; i < lim2; i++) if (stt.col[i] != d_arr[i]) E++;
        return (ll)usedOps + 10000LL * ((ll)E + 2LL * (ll)(M - k));
    };

    bool hasSavedComplete = false;
    ll savedCompleteScore = (1LL << 62);
    vi savedCompleteAns;

    auto saveIfComplete = [&]() {
        if (remFood != 0) return;
        ll nowScore = calcOfficialScore(s, (int)ans.size());
        if (!hasSavedComplete || nowScore < savedCompleteScore) {
            hasSavedComplete = true;
            savedCompleteScore = nowScore;
            savedCompleteAns = ans;
        }
    };

    // ハッシュ関数も軽量化
    auto snakeHash = [&](const FastState &stt) -> uint64_t {
        uint64_t h = 1469598103934665603ULL;
        for (int i = 0; i < stt.len; i++) {
            h ^= stt.pos[i];
            h *= 1099511628211ULL;
        }
        return h;
    };

    auto nearestFoodManhattan = [&](int hpos, const uint8_t* f_arr) -> int {
        int hx = posXFast(hpos), hy = posYFast(hpos);
        int best = INT_MAX / 4;
        for (int i = 0; i < 256; i++) {
            if (f_arr[i] == 0) continue;
            int md = abs(hx - posXFast(i)) + abs(hy - posYFast(i));
            if (md < best) best = md;
        }
        return best;
    };

    deque<uint64_t> recentHashes;
    unordered_map<uint64_t, int> recentFreq;
    constexpr int RECENT_WINDOW = 1000;
    {
        uint64_t h0 = snakeHash(s);
        recentHashes.push_back(h0);
        recentFreq[h0] = 1;
    }

    int collectModeTurns = 0;
    int totalTurns = 0;

    auto validDir = [&](const FastState &curSp, int dir) -> bool {
        int nx = posXFast(curSp.pos[0]) + DX[dir], ny = posYFast(curSp.pos[0]) + DY[dir];
        if (nx < 0 || nx >= N || ny < 0 || ny >= N) return false;
        int npos = packPosFast(nx, ny);
        if (curSp.len >= 2 && curSp.pos[1] == npos) return false;
        return true;
    };

    while ((int)ans.size() < opsCap) {
        double ms = chrono::duration_cast<chrono::milliseconds>(
                        chrono::steady_clock::now() - st).count();
        if (ms >= limitMs) break;

        saveIfComplete();

        if (!collectAllMode) {
            if (limitMs - ms <= 120.0) collectAllMode = true;
        }

        int dir = -1;
        if (collectAllMode) {
            int bestDir = -1, bestLen = -1, bestAte = -1, bestRepeat = INT_MAX, bestDist = INT_MAX;
            for (int cd = 0; cd < 4; cd++) {
                if (!validDir(s, cd)) continue;

                int npos = packPosFast(posXFast(s.pos[0]) + DX[cd], posYFast(s.pos[0]) + DY[cd]);
                int ate = (s.f[npos] != 0 ? 1 : 0);

                FastState ts = s;
                applyMoveLocalFast(cd, ts);
                
                int lenAfter = ts.len;
                uint64_t h = snakeHash(ts);
                int repeatCnt = 0;
                auto it = recentFreq.find(h);
                if (it != recentFreq.end()) repeatCnt = it->second;
                int distToFood = nearestFoodManhattan(ts.pos[0], ts.f);

                if (lenAfter > bestLen || (lenAfter == bestLen && ate > bestAte) || 
                   (lenAfter == bestLen && ate == bestAte && repeatCnt < bestRepeat) || 
                   (lenAfter == bestLen && ate == bestAte && repeatCnt == bestRepeat && distToFood < bestDist)) {
                    bestLen = lenAfter; bestAte = ate; bestRepeat = repeatCnt;
                    bestDist = distToFood; bestDir = cd;
                }
            }
            dir = (bestDir != -1) ? bestDir : getSafeDirFast(s);
        } else {
            int remainCap = opsCap - (int)ans.size();
            int la = min(lookahead, max(1, remainCap));
            dir = chooseBeamNextDirFast(s, beamWidth, la);
            if (dir == -1) dir = getSafeDirFast(s);
        }

        int npos = packPosFast(posXFast(s.pos[0]) + DX[dir], posYFast(s.pos[0]) + DY[dir]);
        int nextFood = s.f[npos];
        int lenBefore = s.len;
        int kBeforeBite = lenBefore + (nextFood != 0 ? 1 : 0);
        int addedMatch = (nextFood != 0 && lenBefore < M && nextFood == d_arr[lenBefore]) ? 1 : 0;
        
        int biteIdx = applyMoveLocalFast(dir, s);
        s.matchCnt += addedMatch;

        if (nextFood != 0) remFood--;
        if (biteIdx != -1) remFood += max(0, kBeforeBite - (biteIdx + 1));

        if (biteIdx != -1) {
            s.matchCnt = 0;
            int lim2 = min((int)s.len, M);
            for (int i = 0; i < lim2; i++) if (s.col[i] == d_arr[i]) s.matchCnt++;

            int removedMismatch = 0;
            for (int i = biteIdx + 1; i < lenBefore; i++) {
                if (i < M && s.col[i] != d_arr[i]) removedMismatch++;
            }
            s.cutMismatchGain += (ll)removedMismatch;
        }

        if (!collectAllMode) {
            if (nextFood != 0 && s.pref == lenBefore && lenBefore < M && nextFood == d_arr[lenBefore]) {
                s.pref++;
            }
            if (biteIdx != -1) {
                int newLen = biteIdx + 1;
                if (s.pref > newLen) {
                    s.cutPenalty += (ll)(s.pref - newLen);
                    s.pref = newLen;
                }
            }
            if (s.targetIdx < M && nextFood == d_arr[s.targetIdx]) s.targetIdx++;
        }

        ans.push_back(dir);
        if (collectAllMode) collectModeTurns++;
        totalTurns++;
        {
            uint64_t hs = snakeHash(s);
            recentHashes.push_back(hs);
            recentFreq[hs]++;
            if ((int)recentHashes.size() > RECENT_WINDOW) {
                uint64_t old = recentHashes.front();
                recentHashes.pop_front();
                auto it = recentFreq.find(old);
                if (it != recentFreq.end()) {
                    it->second--;
                    if (it->second == 0) recentFreq.erase(it);
                }
            }
        }

        saveIfComplete();

        if (remFood == 0 && s.matchCnt == M) {
            break;
        }
    }

    cerr << "DEBUG: bw=" << beamWidth
         << " la=" << lookahead
         << " cap=" << bestPerfectTurnsCap
         << " collectAllMode_turns=" << collectModeTurns
         << " totalTurns=" << totalTurns
         << " totalMoves=" << ans.size()
         << " hasSavedComplete=" << (hasSavedComplete ? 1 : 0)
         << " savedMoves=" << savedCompleteAns.size()
         << " remFood=" << remFood
         << '\n';

    return hasSavedComplete ? savedCompleteAns : ans;
}

// === 初期の移動ルート構築 ===

vector<Pos> collectInitialFoodsFast(const FastState &s) {
    vector<Pos> foods;
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            if (s.f[i * 16 + j] != 0) foods.push_back({i, j});
        }
    }
    return foods;
}

vi buildInitialVisitOrder(const vector<Pos> &foods) {
    int F = (int)foods.size();
    vi order; order.reserve(F);
    if (F == 0) return order;

    vb used(F, false);
    int cx = 4, cy = 0;
    for (int step = 0; step < F; step++) {
        int bi = -1, bd = 1e9;
        for (int i = 0; i < F; i++) {
            if (used[i]) continue;
            int md = abs(cx - foods[i].x) + abs(cy - foods[i].y);
            if (md < bd) { bd = md; bi = i; }
        }
        used[bi] = true;
        order.push_back(bi);
        cx = foods[bi].x; cy = foods[bi].y;
    }
    return order;
}

vi buildMovesFromOrder(const vi &order, const vector<Pos> &foods, const FastState &initialState) {
    FastState s = initialState;
    vi moves;
    int opsCap = searchOpsCap();
    moves.reserve(opsCap);
    int noEatStreak = 0;

    for (int id : order) {
        if ((int)moves.size() >= opsCap) break;
        int tx = foods[id].x, ty = foods[id].y, tpos = packPosFast(tx, ty);
        if (s.f[tpos] == 0) continue;

        int guard = 0;
        while ((int)moves.size() < opsCap && s.f[tpos] != 0) {
            int dir = getDirToCellFast(tx, ty, s);
            if (dir == -1) dir = getDirToNearestFoodFast(s);
            if (dir == -1) dir = getSafeDirFast(s);

            int npos = packPosFast(posXFast(s.pos[0]) + DX[dir], posYFast(s.pos[0]) + DY[dir]);
            bool ate = (s.f[npos] != 0);

            moves.push_back(dir);
            applyMoveLocalFast(dir, s);

            if (ate) noEatStreak = 0; else noEatStreak++;

            guard++;
            if (guard > N * N * 6 || noEatStreak > N * N * 12) break;
        }
        if (noEatStreak > N * N * 12) break;
    }

    int fillGuard = 0;
    while ((int)moves.size() < opsCap && s.len < M) {
        int dir = getDirToNearestFoodFast(s);
        if (dir == -1) break;

        int npos = packPosFast(posXFast(s.pos[0]) + DX[dir], posYFast(s.pos[0]) + DY[dir]);
        bool ate = (s.f[npos] != 0);

        moves.push_back(dir);
        applyMoveLocalFast(dir, s);

        if (ate) noEatStreak = 0; else noEatStreak++;

        fillGuard++;
        if (fillGuard > opsCap || noEatStreak > N * N * 15) break;
    }
    return moves;
}

struct EvalResult {
    ll score;
    int turns;
    bool perfect;
};

EvalResult evalMoves(const FastState &initialState, const vi &moves) {
    FastState s = initialState;
    int remFood = 0;
    for (int i = 0; i < 256; i++) if (s.f[i] != 0) remFood++;

    for (int dir : moves) {
        int npos = packPosFast(posXFast(s.pos[0]) + DX[dir], posYFast(s.pos[0]) + DY[dir]);
        int nextFood = s.f[npos];
        int lenBefore = s.len;
        int kBeforeBite = lenBefore + (nextFood != 0 ? 1 : 0);

        int biteIdx = applyMoveLocalFast(dir, s);

        if (nextFood != 0) remFood--;
        if (biteIdx != -1) remFood += max(0, kBeforeBite - (biteIdx + 1));
    }

    int k = s.len;
    int lim = min(k, M);
    int E = 0;
    for (int i = 0; i < lim; i++) if (s.col[i] != d_arr[i]) E++;
    ll score = (ll)moves.size() + 10000LL * ((ll)E + 2LL * (ll)(M - k));
    bool perfect = (remFood == 0 && k == M && E == 0);
    return {score, (int)moves.size(), perfect};
}

int main() {
    cin.tie(0);
    ios::sync_with_stdio(0);

    auto globalStart = chrono::steady_clock::now();
    const double TL_MS = 1950.0;
    const double SAFETY_MS = 30.0;

    cin >> N >> M >> C;
    for (int i = 0; i < M; i++) cin >> d_arr[i];

    FastState initialState;
    memset(&initialState, 0, sizeof(initialState));

    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            int f; cin >> f;
            initialState.f[i * 16 + j] = f;
        }
    }

    initialState.len = 5;
    initialState.pos[0] = packPosFast(4, 0);
    initialState.pos[1] = packPosFast(3, 0);
    initialState.pos[2] = packPosFast(2, 0);
    initialState.pos[3] = packPosFast(1, 0);
    initialState.pos[4] = packPosFast(0, 0);
    for (int i = 0; i < 5; i++) initialState.col[i] = 1;
    for (int i = 0; i < 5; i++) initialState.f[initialState.pos[i]] = 0;

    double usedMs = chrono::duration_cast<chrono::milliseconds>(
                        chrono::steady_clock::now() - globalStart).count();
    double remainMs = TL_MS - usedMs - SAFETY_MS;
    
    vi ansDir;
    if (remainMs > 20.0) {
        struct BeamParam { int width, depth; };
        const vector<BeamParam> params = {
            {80, 24}, {96, 20}, {64, 28}, {112, 16}, {48, 32}, {72, 26}
        };

        double firstBudget = min(remainMs, 1950.0);
        ansDir = beamConstructMoves(initialState, firstBudget, 80, 24, -1);
        EvalResult baseEv = evalMoves(initialState, ansDir);

        ll bestScore = baseEv.score;
        int bestPerfectTurns = baseEv.perfect ? baseEv.turns : -1;

        if (bestPerfectTurns != -1) {
            int trial = 0;
            while (true) {
                double usedNow = chrono::duration_cast<chrono::milliseconds>(
                                    chrono::steady_clock::now() - globalStart).count();
                double remNow = TL_MS - usedNow - SAFETY_MS;
                if (remNow <= 20.0) break;

                auto bp = params[trial % (int)params.size()];
                double allocMs = min(220.0, remNow - 10.0);
                if (allocMs <= 5.0) break;

                vi cand = beamConstructMoves(initialState, allocMs, bp.width, bp.depth, bestPerfectTurns);
                EvalResult ev = evalMoves(initialState, cand);

                if (ev.score < bestScore) {
                    bestScore = ev.score;
                    ansDir = cand;
                }

                if (ev.perfect) {
                    if (bestPerfectTurns == -1 || ev.turns < bestPerfectTurns) {
                        bestPerfectTurns = ev.turns;
                    }
                }

                trial++;
            }
        }
    } else {
        auto foods = collectInitialFoodsFast(initialState);
        auto ord = buildInitialVisitOrder(foods);
        ansDir = buildMovesFromOrder(ord, foods, initialState);
    }

    for (int dir : ansDir) cout << DC[dir] << '\n';
    return 0;
}