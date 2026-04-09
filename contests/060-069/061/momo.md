# A - Multi-Player Territory Game (AHC061)

**実行時間制限:** 2.0 sec / **メモリ制限:** 1024 MB

## ストーリー

高橋くんは複数人で対戦する陣取りゲームで遊ぶことにした。AIを相手に土地の支配を争い、できるだけ大差をつけて勝利することを目指せ。

## 問題文

$N \\times N$ のマス目で表される土地がある。一番左上のマスの座標を $(0,0)$ とし、そこから下方向に $i$ マス、右方向に $j$ マス移動した先のマスの座標を $(i,j)$ とする。

この土地上で、$M$ 人のプレイヤーが陣取りゲームを行う。プレイヤーは $0$ から $M-1$ までの $M$ 人であり、高橋くん（プレイヤー $0$）以外の $M-1$ 人は AI が操作する。

### 初期状態

-   各プレイヤー $p$ は初期状態で 1 マスの初期領土 $(sx\_p, sy\_p)$ を所有し、そのマスの上に駒を置く。
    
-   各マス $(i,j)$ は価値 $V\_{i,j}$ とレベル $L\_{i,j}$ を持つ。
    
    -   **価値** $V\_{i,j}$: ゲームの進行によらず不変。
        
    -   **レベル** $L\_{i,j}$: ゲームの進行に応じて変化する。
        
    -   初期状態において、各プレイヤーの初期領土のレベルは $1$ であり、誰の領土でもないマスのレベルは $0$ である。
        

### ターンの進行

$T$ ターンのゲームを行う。各ターンは以下の手順で進行する。

#### 1\. 移動先の決定

全てのプレイヤーは同時に駒の移動先を決定する。移動先は以下の条件を満たす必要がある。

-   **到達可能領土**: プレイヤーの駒の現在位置から、上下左右に隣接する自分の領土を経由して到達できるマスの集合。
    
-   **移動条件**: 移動先は「到達可能領土に含まれる」か、「到達可能領土のいずれかのマスに隣接」していなければならない。
    
-   移動先に他のプレイヤーの駒が存在してはならない。
    

#### 2\. 競合解決

全ての駒を同時に移動した後、**2つ以上の駒が存在する各マス**について以下の処理を行う。

-   そのマスの所有者の駒が含まれる場合、その駒のみを盤面に残し、残りの駒を全て回収する。
    
-   そのマスが誰の領土でもない場合、またはそのマスの所有者の駒が含まれない場合は、そのマスに存在する全ての駒を回収する。
    

#### 3\. 領土の更新

回収されなかった各プレイヤーの駒について、移動先マスの条件に応じて以下の処理を適用する。

-   **占領**: 誰の領土でもないマスの場合、自分の領土とし、レベルを $1$ にする。
    
-   **強化**: 自分の領土の場合、レベルを $1$ 増加させる。ただし、レベルの上限は入力で与えられる定数 $U$ であり、既に $U$ の場合は変化しない。
    
-   **攻撃**: 他のプレイヤーの領土の場合、レベルを $1$ 減少させる。これによってレベルが $0$ になった場合、そのマスを自分の領土とし、レベルを $1$ にする。レベルが $0$ にならなかった場合、攻撃したプレイヤーの駒を回収する。
    

#### 4\. 駒の復帰

このターンに回収された駒は、このターンの開始時点（移動前）のマスに戻す。

> **Note:** プレイヤーの駒は常に自分の領土上に存在し、駒が存在するマスは他のプレイヤーの攻撃対象にならないため、プレイヤーの領土が 0 マスになることはない。

## 得点 (Scoring)

$T$ ターン終了後、各プレイヤー $p$ の（到達可能でない領土を含む）全ての領土 $(i,j)$ に対する $V\_{i,j} \\times L\_{i,j}$ の総和をプレイヤー $p$ のスコア $S\_p$ とする。

高橋くん（プレイヤー 0）の目的は、スコアの最も高い AI プレイヤーに対する自分のスコアの比率をなるべく大きくすることである。

すなわち、高橋くんのスコアを $S\_0$、スコアの最も高い AI プレイヤーのスコアを $S\_A = \\max\_{1 \\le p \\le M-1} S\_p$ としたとき、以下の比率を最大化せよ。

$$\\frac{S\_0}{S\_A}$$

### 絶対スコア

テストケースごとの絶対スコアは以下のように計算される。

$$\\text{Score} = \\text{round}\\left(10^5 \\times \\log\_2 \\left(1 + \\frac{S\_0}{S\_A}\\right)\\right)$$

## AIの行動決定方法

AI プレイヤー $p$ ($1 \\le p \\le M-1$) は内部パラメータ $wa\_p, wb\_p, wc\_p, wd\_p, \\epsilon\_p$ を持ち、以下のアルゴリズムで移動先を決定する。

### 評価値 $A\_{p,i,j}$ の定義

各マス $(i,j)$ について以下のように計算する。

1.  **誰の領土でもない場合**: $A\_{p,i,j} = V\_{i,j} \\times wa\_p$
    
2.  **自分の領土でレベルが** $U$ **未満の場合**: $A\_{p,i,j} = V\_{i,j} \\times wb\_p$
    
3.  **自分の領土でレベルが** $U$ **の場合**: $A\_{p,i,j} = 0$
    
4.  **他のプレイヤーの領土でレベルが 1 の場合**: $A\_{p,i,j} = V\_{i,j} \\times wc\_p$
    
5.  **他のプレイヤーの領土でレベルが 2 以上の場合**: $A\_{p,i,j} = V\_{i,j} \\times wd\_p$
    

### 移動先の決定

プレイヤー $p$ の移動可能な全てのマスの集合を $B\_p$ とする。

-   確率 $\\epsilon\_p$ で **ランダム行動** を行う。
    
    -   $B\_p$ の中から一様ランダムに 1 つ選ぶ。
        
-   確率 $1 - \\epsilon\_p$ で **貪欲行動** を行う。
    
    -   $B\_p$ の中で評価値 $A\_{p,i,j}$ が最大のマスを選ぶ。複数ある場合は一様ランダムに選ぶ。
        

## 入出力

### 初期入力

盤面サイズ $N$、プレイヤー数 $M$、ターン数 $T$、レベル上限 $U$、価値 $V$、初期位置が与えられる。

```
N M T U
V_{0,0} ... V_{0,N-1}
...
V_{N-1,0} ... V_{N-1,N-1}
sx_0 sy_0
...
sx_{M-1} sy_{M-1}
```

### ターンごとの入出力

各ターン $t$ ($1 \\le t \\le T$) について、以下の処理を繰り返す。

**出力:** プレイヤー 0 の移動先 $(tx\_0, ty\_0)$ を出力せよ。

```
tx_0 ty_0
```

**入力:** ターン $t$ 終了時点の情報が与えられる。 各プレイヤーの移動しようとした先 $(tx\_p, ty\_p)$、確定後の位置 $(ex\_p, ey\_p)$、全マスの所有者 $O\_{i,j}$、全マスのレベル $L\_{i,j}$。

```
tx_0 ty_0
...
tx_{M-1} ty_{M-1}
ex_0 ey_0
...
ex_{M-1} ey_{M-1}
O_{0,0} ... O_{0,N-1}
...
O_{N-1,0} ... O_{N-1,N-1}
L_{0,0} ... L_{0,N-1}
...
L_{N-1,0} ... L_{N-1,N-1}
```

-   $O\_{i,j} = -1$ の場合、そのマスは誰の領土でもない。
    

## 制約

-   $N = 10$
    
-   $2 \\le M \\le 8$
    
-   $T = 100$
    
-   $1 \\le U \\le 5$
    
-   $1 \\le V\_{i,j}$
    
-   $\\sum V\_{i,j} = 1000 \\times N^2$
    
-   $0 \\le sx\_p, sy\_p \\le N-1$
    
-   各プレイヤーの初期領土は異なる。
    
-   入力はすべて整数。
    

## サンプルコード (Python)

```
from collections import deque
import random
import sys

# 座標は (行, 列) = (x, y) として扱われていることに注意
# 問題文では (i, j) = (下方向, 右方向) なので、x=行, y=列に対応する。

DX = [-1, 1, 0, 0]
DY = [0, 0, -1, 1]

def read_initial_input():
    # N: サイズ, M: プレイヤー数, T: ターン数, U: レベル上限
    line1 = sys.stdin.readline().split()
    if not line1: return None # End of input
    N, M, T, U = map(int, line1)
    
    V = []
    for _ in range(N):
        V.append(list(map(int, sys.stdin.readline().split())))
        
    sx = [0] * M
    sy = [0] * M
    for p in range(M):
        sx[p], sy[p] = map(int, sys.stdin.readline().split())

    # ローカルで管理する盤面情報
    owner = [[-1] * N for _ in range(N)]
    level = [[0] * N for _ in range(N)]
    
    # プレイヤーの現在位置
    px = list(sx)
    py = list(sy)
    
    for p in range(M):
        owner[sx[p]][sy[p]] = p
        level[sx[p]][sy[p]] = 1

    return N, M, T, U, V, owner, level, px, py

def get_candidates(N, M, owner, px, py):
    # 到達可能領土をBFSで探索
    # px[0], py[0] はプレイヤー0（自分）の現在位置
    reachable = {(px[0], py[0])}
    queue = deque([(px[0], py[0])])
    
    while queue:
        x, y = queue.popleft()
        for d in range(4):
            nx, ny = x + DX[d], y + DY[d]
            # 盤面内 かつ 未訪問 かつ 自分の領土(owner==0)
            if 0 <= nx < N and 0 <= ny < N:
                if (nx, ny) not in reachable and owner[nx][ny] == 0:
                    reachable.add((nx, ny))
                    queue.append((nx, ny))
    
    # 到達可能領土(reachable) そのもの、またはそれに隣接するマスが移動候補
    candidates = set(reachable)
    for x, y in reachable:
        for d in range(4):
            nx, ny = x + DX[d], y + DY[d]
            if 0 <= nx < N and 0 <= ny < N:
                candidates.add((nx, ny))
    
    # 他のプレイヤーがいる場所へは移動できない（ルール：移動先に他のプレイヤーの駒が存在してはならない）
    # ※問題文には「移動先に他のプレイヤーの駒が存在してはならない」とあるため除外
    for p in range(1, M):
        candidates.discard((px[p], py[p]))
        
    return candidates

def read_turn_result(N, M, owner, level, px, py):
    # 全プレイヤーの移動しようとした先
    for _ in range(M):
        sys.stdin.readline()
        
    # ターン終了時の確定位置
    for p in range(M):
        px[p], py[p] = map(int, sys.stdin.readline().split())
        
    # 所有者情報
    for i in range(N):
        row = list(map(int, sys.stdin.readline().split()))
        for j in range(N):
            owner[i][j] = row[j]
            
    # レベル情報
    for i in range(N):
        row = list(map(int, sys.stdin.readline().split()))
        for j in range(N):
            level[i][j] = row[j]

def main():
    initial_data = read_initial_input()
    if initial_data is None: return
    N, M, T, U, V, owner, level, px, py = initial_data

    for _ in range(T):
        candidates = get_candidates(N, M, owner, px, py)
        
        if not candidates:
            # 万が一動ける場所がない場合は現在地にとどまる（通常ありえないが念のため）
            tx, ty = px[0], py[0]
        else:
            # ランダムに選択
            tx, ty = random.choice(list(candidates))
            
        print(f"{tx} {ty}")
        sys.stdout.flush()
        
        read_turn_result(N, M, owner, level, px, py)

if __name__ == '__main__':
    main()
```