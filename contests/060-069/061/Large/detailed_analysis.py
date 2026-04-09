#!/usr/bin/env python3
import re
import sys
from pathlib import Path
from collections import defaultdict
import statistics
from typing import List, Dict, Any

def parse_table(table_text):
    """表データをパースしてシード、スコア、実行時間を抽出"""
    results = []
    lines = table_text.strip().split('\n')
    
    for line in lines:
        if '/' in line and '|' in line:
            parts = [p.strip() for p in line.split('|')]
            if len(parts) >= 8:
                try:
                    seed = parts[2].strip()
                    if seed and seed.isdigit():
                        score = parts[3].strip().split()[0].replace(',', '')
                        exec_time = parts[7].strip().replace(',', '').replace(' ms', '')
                        
                        results.append({
                            'seed': seed,
                            'score': int(score),
                            'exec_time': int(exec_time) if exec_time.isdigit() else 0
                        })
                except (ValueError, IndexError):
                    continue
    
    return results

def read_test_params(seed, in_dir='in'):
    """テストケースファイルからN, M, T, U、グリッドの価値、敵の配置を読み取る"""
    test_file = Path(in_dir) / f"{seed}.txt"
    
    if not test_file.exists():
        return None, None, None, None, None, None, None
    
    try:
        with open(test_file, 'r') as f:
            first_line = f.readline().strip()
            params = first_line.split()
            if len(params) >= 4:
                n = int(params[0])
                m = params[1]
                t = params[2]
                u = int(params[3])
                
                # グリッドの価値を2次元配列として読み取る
                grid = []
                for _ in range(n):
                    line = f.readline().strip()
                    if line:
                        values = list(map(int, line.split()))
                        grid.append(values)
                
                # 敵の位置を読み取る
                enemies = []
                for _ in range(u):
                    line = f.readline().strip()
                    if line:
                        coords = line.split()
                        if len(coords) >= 2:
                            try:
                                # 最初の2つの値のみを整数として読み取る
                                y = int(coords[0])
                                x = int(coords[1])
                                enemies.append((y, x))
                            except (ValueError, IndexError):
                                # 変換できない場合はスキップ
                                pass
                
                # グリッド統計を計算
                if grid and len(grid) == n:
                    grid_values = [v for row in grid for v in row]
                    
                    # 空間的特徴の計算
                    spatial_features = calculate_spatial_features(grid, n)
                    
                    grid_stats = {
                        'sum': sum(grid_values),
                        'mean': statistics.mean(grid_values),
                        'median': statistics.median(grid_values),
                        'min': min(grid_values),
                        'max': max(grid_values),
                        'std': statistics.stdev(grid_values) if len(grid_values) > 1 else 0,
                        'count': len(grid_values)
                    }
                    
                    # 空間的特徴を追加
                    grid_stats.update(spatial_features)
                    
                    # 敵配置の特徴を計算
                    enemy_features = None
                    if enemies:
                        enemy_features = calculate_enemy_features(enemies, grid, n)
                    
                    return params[0], m, t, u, grid_stats, grid, enemy_features
                
                return params[0], m, t, u, None, None, None
    except Exception as e:
        print(f"Error reading {test_file}: {e}", file=sys.stderr)
    
    return None, None, None, None, None, None, None

def calculate_enemy_features(enemies, grid, n):
    """敵配置の特徴を計算"""
    features = {}
    
    # 敵の数
    features['enemy_count'] = len(enemies)
    
    if len(enemies) == 0:
        return features
    
    # 敵同士の平均距離
    if len(enemies) > 1:
        distances = []
        for i in range(len(enemies)):
            for j in range(i + 1, len(enemies)):
                dist = abs(enemies[i][0] - enemies[j][0]) + abs(enemies[i][1] - enemies[j][1])
                distances.append(dist)
        features['enemy_avg_distance'] = statistics.mean(distances)
        features['enemy_min_distance'] = min(distances)
        features['enemy_max_distance'] = max(distances)
    else:
        features['enemy_avg_distance'] = 0
        features['enemy_min_distance'] = 0
        features['enemy_max_distance'] = 0
    
    # 敵の重心
    centroid_y = statistics.mean([e[0] for e in enemies])
    centroid_x = statistics.mean([e[1] for e in enemies])
    
    # グリッド中心からの敵の重心距離
    grid_center = (n - 1) / 2
    features['enemy_centroid_from_center'] = abs(centroid_y - grid_center) + abs(centroid_x - grid_center)
    
    # 敵の重心からの各敵の距離の標準偏差（散らばり具合）
    if len(enemies) > 1:
        enemy_spread = [abs(e[0] - centroid_y) + abs(e[1] - centroid_x) for e in enemies]
        features['enemy_spread'] = statistics.stdev(enemy_spread)
    else:
        features['enemy_spread'] = 0
    
    # 敵の位置におけるグリッドの価値
    enemy_values = []
    for y, x in enemies:
        if 0 <= y < n and 0 <= x < n:
            enemy_values.append(grid[y][x])
    
    if enemy_values:
        features['enemy_avg_value'] = statistics.mean(enemy_values)
        features['enemy_min_value'] = min(enemy_values)
        features['enemy_max_value'] = max(enemy_values)
    else:
        features['enemy_avg_value'] = 0
        features['enemy_min_value'] = 0
        features['enemy_max_value'] = 0
    
    # 高価値マス（上位10%）との最短距離
    grid_values = [v for row in grid for v in row]
    threshold = sorted(grid_values, reverse=True)[len(grid_values) // 10]
    
    high_value_positions = []
    for i in range(n):
        for j in range(n):
            if grid[i][j] >= threshold:
                high_value_positions.append((i, j))
    
    if high_value_positions:
        min_distances_to_high_value = []
        for enemy in enemies:
            distances_to_high = [abs(enemy[0] - hv[0]) + abs(enemy[1] - hv[1]) 
                                for hv in high_value_positions]
            min_distances_to_high_value.append(min(distances_to_high))
        
        features['enemy_min_dist_to_high_value'] = statistics.mean(min_distances_to_high_value)
    else:
        features['enemy_min_dist_to_high_value'] = 0
    
    return features

def calculate_spatial_features(grid, n):
    """グリッドの空間的特徴を計算"""
    features = {}
    
    # 全マスの値をリスト化
    grid_values = [v for row in grid for v in row]
    
    # 最大値の位置を特定
    max_val = max(grid_values)
    max_positions = []
    for i in range(n):
        for j in range(n):
            if grid[i][j] == max_val:
                max_positions.append((i, j))
    
    # 最大値の中心からの距離
    if max_positions:
        center = (n - 1) / 2
        max_distances = [abs(i - center) + abs(j - center) for i, j in max_positions]
        features['max_dist_from_center'] = statistics.mean(max_distances)
    else:
        features['max_dist_from_center'] = 0
    
    # 隣接マス間の価値差の平均（勾配）
    gradients = []
    for i in range(n):
        for j in range(n):
            if i + 1 < n:
                gradients.append(abs(grid[i][j] - grid[i+1][j]))
            if j + 1 < n:
                gradients.append(abs(grid[i][j] - grid[i][j+1]))
    
    features['avg_gradient'] = statistics.mean(gradients) if gradients else 0
    features['max_gradient'] = max(gradients) if gradients else 0
    
    # 上位k%のマスの集中度（重心からの距離）
    threshold = sorted(grid_values, reverse=True)[len(grid_values) // 10]  # 上位10%
    high_value_positions = []
    for i in range(n):
        for j in range(n):
            if grid[i][j] >= threshold:
                high_value_positions.append((i, j))
    
    if len(high_value_positions) > 1:
        # 高価値マスの重心を計算
        centroid_i = statistics.mean([pos[0] for pos in high_value_positions])
        centroid_j = statistics.mean([pos[1] for pos in high_value_positions])
        
        # 重心からの平均距離（集中度）
        distances = [abs(i - centroid_i) + abs(j - centroid_j) for i, j in high_value_positions]
        features['high_value_concentration'] = statistics.mean(distances)
    else:
        features['high_value_concentration'] = 0
    
    # 中心部と端の価値の比較
    center_region = []
    edge_region = []
    margin = n // 4
    
    for i in range(n):
        for j in range(n):
            if margin <= i < n - margin and margin <= j < n - margin:
                center_region.append(grid[i][j])
            elif i < margin or i >= n - margin or j < margin or j >= n - margin:
                edge_region.append(grid[i][j])
    
    if center_region and edge_region:
        features['center_mean'] = statistics.mean(center_region)
        features['edge_mean'] = statistics.mean(edge_region)
        features['center_edge_ratio'] = features['center_mean'] / features['edge_mean'] if features['edge_mean'] > 0 else 0
    else:
        features['center_mean'] = 0
        features['edge_mean'] = 0
        features['center_edge_ratio'] = 0
    
    # 四隅の平均値
    corners = [grid[0][0], grid[0][n-1], grid[n-1][0], grid[n-1][n-1]]
    features['corners_mean'] = statistics.mean(corners)
    
    return features

def enrich_results_with_params(results, in_dir='in'):
    """結果にパラメータ情報を追加"""
    enriched_results = []
    for result in results:
        seed = result['seed']
        n, m, t, u, grid_stats, grid, enemy_features = read_test_params(seed, in_dir)
        
        enriched_result = result.copy()
        enriched_result['n'] = int(n) if n and n.isdigit() else None
        enriched_result['m'] = int(m) if m and m.isdigit() else None
        enriched_result['t'] = int(t) if t and t.isdigit() else None
        enriched_result['u'] = int(u) if isinstance(u, int) else (int(u) if u and u.isdigit() else None)
        enriched_result['grid_stats'] = grid_stats
        enriched_result['grid'] = grid
        enriched_result['enemy_features'] = enemy_features
        enriched_results.append(enriched_result)
    
    return enriched_results

def basic_statistics(results):
    """基本統計情報を表示"""
    print("=" * 80)
    print("基本統計情報")
    print("=" * 80)
    
    scores = [r['score'] for r in results]
    exec_times = [r['exec_time'] for r in results]
    
    print(f"\n【スコア統計】")
    print(f"  件数:       {len(scores):>10,}")
    print(f"  合計:       {sum(scores):>10,}")
    print(f"  平均:       {statistics.mean(scores):>10,.2f}")
    print(f"  中央値:     {statistics.median(scores):>10,.2f}")
    print(f"  標準偏差:   {statistics.stdev(scores) if len(scores) > 1 else 0:>10,.2f}")
    print(f"  最小値:     {min(scores):>10,}")
    print(f"  最大値:     {max(scores):>10,}")
    print(f"  範囲:       {max(scores) - min(scores):>10,}")
    
    # 四分位数
    sorted_scores = sorted(scores)
    q1 = sorted_scores[len(sorted_scores) // 4]
    q3 = sorted_scores[len(sorted_scores) * 3 // 4]
    print(f"  第1四分位:  {q1:>10,}")
    print(f"  第3四分位:  {q3:>10,}")
    print(f"  四分位範囲: {q3 - q1:>10,}")
    
    print(f"\n【実行時間統計 (ms)】")
    print(f"  平均:       {statistics.mean(exec_times):>10,.2f}")
    print(f"  中央値:     {statistics.median(exec_times):>10,.2f}")
    print(f"  標準偏差:   {statistics.stdev(exec_times) if len(exec_times) > 1 else 0:>10,.2f}")
    print(f"  最小値:     {min(exec_times):>10,}")
    print(f"  最大値:     {max(exec_times):>10,}")

def parameter_analysis(results):
    """パラメータごとの分析"""
    print("\n" + "=" * 80)
    print("パラメータごとの分析")
    print("=" * 80)
    
    params = ['n', 'm', 't', 'u']
    
    for param in params:
        print(f"\n【パラメータ {param.upper()} の分析】")
        
        # パラメータ値でグループ化
        grouped = defaultdict(list)
        for r in results:
            if r[param] is not None:
                grouped[r[param]].append(r['score'])
        
        if not grouped:
            print(f"  データなし")
            continue
        
        # 各パラメータ値の統計
        print(f"\n  値 | 件数 |     平均スコア |   中央値 |    最小値 |    最大値 | 標準偏差")
        print(f"  " + "-" * 78)
        
        for value in sorted(grouped.keys()):
            scores = grouped[value]
            avg = statistics.mean(scores)
            med = statistics.median(scores)
            std = statistics.stdev(scores) if len(scores) > 1 else 0
            print(f"  {value:>2} | {len(scores):>4} | {avg:>14,.2f} | {med:>8,.0f} | {min(scores):>9,} | {max(scores):>9,} | {std:>8,.2f}")

def score_distribution(results):
    """スコア分布の分析"""
    print("\n" + "=" * 80)
    print("スコア分布")
    print("=" * 80)
    
    scores = [r['score'] for r in results]
    min_score = min(scores)
    max_score = max(scores)
    
    # 10個の区間に分割
    bins = 10
    bin_width = (max_score - min_score) / bins
    
    print(f"\n  スコア範囲: {min_score:,} ~ {max_score:,}")
    print(f"  区間幅: {bin_width:,.2f}\n")
    
    distribution = [0] * bins
    for score in scores:
        bin_idx = min(int((score - min_score) / bin_width), bins - 1)
        distribution[bin_idx] += 1
    
    print(f"  区間             | 件数 | 割合   | ヒストグラム")
    print(f"  " + "-" * 70)
    
    max_count = max(distribution)
    for i, count in enumerate(distribution):
        start = min_score + i * bin_width
        end = min_score + (i + 1) * bin_width
        percentage = count / len(scores) * 100
        bar_length = int(count / max_count * 40) if max_count > 0 else 0
        bar = '█' * bar_length
        print(f"  {start:>9,.0f}-{end:>9,.0f} | {count:>4} | {percentage:>5.1f}% | {bar}")

def find_outliers(results):
    """外れ値の検出（IQR法）"""
    print("\n" + "=" * 80)
    print("外れ値検出")
    print("=" * 80)
    
    scores = [r['score'] for r in results]
    sorted_scores = sorted(scores)
    
    q1 = sorted_scores[len(sorted_scores) // 4]
    q3 = sorted_scores[len(sorted_scores) * 3 // 4]
    iqr = q3 - q1
    
    lower_bound = q1 - 1.5 * iqr
    upper_bound = q3 + 1.5 * iqr
    
    print(f"\n  IQR法による外れ値検出")
    print(f"  第1四分位 (Q1): {q1:,}")
    print(f"  第3四分位 (Q3): {q3:,}")
    print(f"  四分位範囲 (IQR): {iqr:,}")
    print(f"  下限: {lower_bound:,.2f}")
    print(f"  上限: {upper_bound:,.2f}")
    
    # 外れ値を抽出
    outliers_low = [r for r in results if r['score'] < lower_bound]
    outliers_high = [r for r in results if r['score'] > upper_bound]
    
    print(f"\n  低スコア外れ値: {len(outliers_low)} 件")
    if outliers_low and len(outliers_low) <= 20:
        print(f"\n  Seed | Score      | N  | M  | T   | U")
        print(f"  " + "-" * 45)
        for r in sorted(outliers_low, key=lambda x: x['score'])[:20]:
            print(f"  {r['seed']:>4} | {r['score']:>10,} | {r['n'] or 'N/A':>2} | {r['m'] or 'N/A':>2} | {r['t'] or 'N/A':>3} | {r['u'] or 'N/A':>2}")
    
    print(f"\n  高スコア外れ値: {len(outliers_high)} 件")
    if outliers_high and len(outliers_high) <= 20:
        print(f"\n  Seed | Score      | N  | M  | T   | U")
        print(f"  " + "-" * 45)
        for r in sorted(outliers_high, key=lambda x: x['score'], reverse=True)[:20]:
            print(f"  {r['seed']:>4} | {r['score']:>10,} | {r['n'] or 'N/A':>2} | {r['m'] or 'N/A':>2} | {r['t'] or 'N/A':>3} | {r['u'] or 'N/A':>2}")

def correlation_analysis(results):
    """パラメータとスコアの相関分析"""
    print("\n" + "=" * 80)
    print("相関分析")
    print("=" * 80)
    
    params = ['n', 'm', 't', 'u']
    
    print("\n  パラメータとスコアの相関係数:")
    print(f"  パラメータ | 相関係数")
    print(f"  " + "-" * 25)
    
    for param in params:
        valid_data = [(r[param], r['score']) for r in results if r[param] is not None]
        if len(valid_data) < 2:
            print(f"  {param.upper():>10} | データ不足")
            continue
        
        param_values = [d[0] for d in valid_data]
        scores = [d[1] for d in valid_data]
        
        # 全て同じ値かチェック
        if len(set(param_values)) == 1:
            print(f"  {param.upper():>10} | 固定値 ({param_values[0]})")
            continue
        
        # ピアソン相関係数
        try:
            correlation = statistics.correlation(param_values, scores)
            print(f"  {param.upper():>10} | {correlation:>8.4f}")
        except statistics.StatisticsError:
            print(f"  {param.upper():>10} | 計算不可")
    
    # グリッド統計との相関
    print("\n  グリッド統計とスコアの相関係数:")
    print(f"  統計値                     | 相関係数")
    print(f"  " + "-" * 40)
    
    grid_stat_keys = [
        ('mean', 'グリッド平均値'),
        ('std', 'グリッド標準偏差'),
        ('max', 'グリッド最大値'),
        ('min', 'グリッド最小値'),
        ('avg_gradient', '平均勾配'),
        ('max_gradient', '最大勾配'),
        ('high_value_concentration', '高価値マスの集中度'),
        ('max_dist_from_center', '最大値の中心距離'),
        ('center_edge_ratio', '中心/端の比'),
        ('center_mean', '中心部平均値'),
        ('edge_mean', '端部平均値'),
        ('corners_mean', '四隅平均値')
    ]
    
    for stat_key, stat_name in grid_stat_keys:
        valid_data = [(r['grid_stats'][stat_key], r['score']) 
                     for r in results if r['grid_stats'] is not None and stat_key in r['grid_stats']]
        
        if len(valid_data) < 2:
            print(f"  {stat_name:>26} | データ不足")
            continue
        
        stat_values = [d[0] for d in valid_data]
        scores = [d[1] for d in valid_data]
        
        if len(set(stat_values)) == 1:
            print(f"  {stat_name:>26} | 固定値")
            continue
        
        try:
            correlation = statistics.correlation(stat_values, scores)
            print(f"  {stat_name:>26} | {correlation:>8.4f}")
        except statistics.StatisticsError:
            print(f"  {stat_name:>26} | 計算不可")
    
    # 敵配置統計との相関
    print("\n  敵配置統計とスコアの相関係数:")
    print(f"  統計値                     | 相関係数")
    print(f"  " + "-" * 40)
    
    enemy_stat_keys = [
        ('enemy_avg_distance', '敵同士の平均距離'),
        ('enemy_spread', '敵の散らばり度'),
        ('enemy_centroid_from_center', '敵重心の中心距離'),
        ('enemy_avg_value', '敵位置の平均価値'),
        ('enemy_min_dist_to_high_value', '高価値マスへの距離')
    ]
    
    for stat_key, stat_name in enemy_stat_keys:
        valid_data = [(r['enemy_features'][stat_key], r['score']) 
                     for r in results if r['enemy_features'] is not None and stat_key in r['enemy_features']]
        
        if len(valid_data) < 2:
            continue
        
        stat_values = [d[0] for d in valid_data]
        scores = [d[1] for d in valid_data]
        
        if len(set(stat_values)) == 1:
            continue
        
        try:
            correlation = statistics.correlation(stat_values, scores)
            print(f"  {stat_name:>26} | {correlation:>8.4f}")
        except statistics.StatisticsError:
            pass

def top_bottom_cases(results, n=20):
    """トップとボトムのケースを表示"""
    print("\n" + "=" * 80)
    print(f"トップ{n}とワースト{n}のケース")
    print("=" * 80)
    
    sorted_results = sorted(results, key=lambda x: x['score'], reverse=True)
    
    print(f"\n【トップ{n}】")
    print(f"  順位 | Seed | Score      | Exec Time | N  | M  | T   | U")
    print(f"  " + "-" * 65)
    for i, r in enumerate(sorted_results[:n], 1):
        print(f"  {i:>4} | {r['seed']:>4} | {r['score']:>10,} | {r['exec_time']:>9,} | {r['n'] or 'N/A':>2} | {r['m'] or 'N/A':>2} | {r['t'] or 'N/A':>3} | {r['u'] or 'N/A':>2}")
    
    print(f"\n【ワースト{n}】")
    print(f"  順位 | Seed | Score      | Exec Time | N  | M  | T   | U")
    print(f"  " + "-" * 65)
    for i, r in enumerate(sorted_results[-n:][::-1], 1):
        rank = len(results) - i + 1
        print(f"  {rank:>4} | {r['seed']:>4} | {r['score']:>10,} | {r['exec_time']:>9,} | {r['n'] or 'N/A':>2} | {r['m'] or 'N/A':>2} | {r['t'] or 'N/A':>3} | {r['u'] or 'N/A':>2}")

def parameter_combination_analysis(results):
    """パラメータの組み合わせによる分析"""
    print("\n" + "=" * 80)
    print("パラメータ組み合わせ分析")
    print("=" * 80)
    
    # 各パラメータが固定値かチェック
    n_values = set(r['n'] for r in results if r['n'] is not None)
    m_values = set(r['m'] for r in results if r['m'] is not None)
    t_values = set(r['t'] for r in results if r['t'] is not None)
    u_values = set(r['u'] for r in results if r['u'] is not None)
    
    n_is_fixed = len(n_values) == 1
    m_is_fixed = len(m_values) == 1
    t_is_fixed = len(t_values) == 1
    u_is_fixed = len(u_values) == 1
    
    # N×M の組み合わせ分析（両方とも固定でない場合のみ）
    if not n_is_fixed and not m_is_fixed:
        print("\n【N × M の組み合わせ】")
        grouped = defaultdict(list)
        for r in results:
            if r['n'] is not None and r['m'] is not None:
                key = (r['n'], r['m'])
                grouped[key].append(r['score'])
        
        if grouped:
            print(f"\n  N  | M  | 件数 |     平均スコア")
            print(f"  " + "-" * 40)
            for (n, m) in sorted(grouped.keys()):
                scores = grouped[(n, m)]
                avg = statistics.mean(scores)
                print(f"  {n:>2} | {m:>2} | {len(scores):>4} | {avg:>14,.2f}")
    elif n_is_fixed:
        print(f"\n  ※ N は固定値 ({list(n_values)[0]}) のため N×M の組み合わせ分析はスキップ")
    
    # T×U の組み合わせ分析（両方とも固定でない場合のみ）
    if not t_is_fixed and not u_is_fixed:
        print("\n【T × U の組み合わせ】")
        grouped = defaultdict(list)
        for r in results:
            if r['t'] is not None and r['u'] is not None:
                key = (r['t'], r['u'])
                grouped[key].append(r['score'])
        
        if grouped:
            print(f"\n  T   | U  | 件数 |     平均スコア")
            print(f"  " + "-" * 40)
            for (t, u) in sorted(grouped.keys()):
                scores = grouped[(t, u)]
                avg = statistics.mean(scores)
                print(f"  {t:>3} | {u:>2} | {len(scores):>4} | {avg:>14,.2f}")
    elif t_is_fixed:
        print(f"\n  ※ T は固定値 ({list(t_values)[0]}) のため T×U の組み合わせ分析はスキップ")
    
    # M×U の組み合わせ分析（両方とも固定でない場合）
    if not m_is_fixed and not u_is_fixed:
        print("\n【M × U の組み合わせ】")
        grouped = defaultdict(list)
        for r in results:
            if r['m'] is not None and r['u'] is not None:
                key = (r['m'], r['u'])
                grouped[key].append(r['score'])
        
        if grouped:
            print(f"\n  M  | U  | 件数 |     平均スコア")
            print(f"  " + "-" * 40)
            for (m, u) in sorted(grouped.keys()):
                scores = grouped[(m, u)]
                avg = statistics.mean(scores)
                print(f"  {m:>2} | {u:>2} | {len(scores):>4} | {avg:>14,.2f}")

def execution_time_analysis(results):
    """実行時間の詳細分析"""
    print("\n" + "=" * 80)
    print("実行時間分析")
    print("=" * 80)
    
    # 実行時間が長いケースと短いケースを比較
    sorted_by_time = sorted(results, key=lambda x: x['exec_time'], reverse=True)
    
    print(f"\n【実行時間が最も長い20ケース】")
    print(f"  Seed | Exec Time | Score      | N  | M  | T   | U")
    print(f"  " + "-" * 60)
    for r in sorted_by_time[:20]:
        print(f"  {r['seed']:>4} | {r['exec_time']:>9,} | {r['score']:>10,} | {r['n'] or 'N/A':>2} | {r['m'] or 'N/A':>2} | {r['t'] or 'N/A':>3} | {r['u'] or 'N/A':>2}")
    
    print(f"\n【実行時間が最も短い20ケース】")
    print(f"  Seed | Exec Time | Score      | N  | M  | T   | U")
    print(f"  " + "-" * 60)
    for r in sorted_by_time[-20:]:
        print(f"  {r['seed']:>4} | {r['exec_time']:>9,} | {r['score']:>10,} | {r['n'] or 'N/A':>2} | {r['m'] or 'N/A':>2} | {r['t'] or 'N/A':>3} | {r['u'] or 'N/A':>2}")

def grid_value_analysis(results):
    """グリッドの価値に関する詳細分析"""
    print("\n" + "=" * 80)
    print("グリッドの空間的配置分析")
    print("=" * 80)
    
    # グリッド統計がある結果のみをフィルタ
    valid_results = [r for r in results if r['grid_stats'] is not None]
    
    if not valid_results:
        print("\n  グリッドデータがありません")
        return
    
    # 空間的特徴の統計
    print("\n【空間的特徴の基本統計】")
    
    spatial_features = [
        ('avg_gradient', '平均勾配（隣接マス差）'),
        ('max_gradient', '最大勾配'),
        ('high_value_concentration', '高価値マス集中度'),
        ('max_dist_from_center', '最大値の中心距離'),
        ('center_edge_ratio', '中心/端の価値比'),
        ('center_mean', '中心部平均値'),
        ('edge_mean', '端部平均値'),
        ('corners_mean', '四隅平均値')
    ]
    
    print(f"\n  特徴量                     |       最小 |       平均 |     中央値 |       最大")
    print(f"  " + "-" * 80)
    
    for feat_key, feat_name in spatial_features:
        feat_values = [r['grid_stats'][feat_key] for r in valid_results 
                      if feat_key in r['grid_stats']]
        if feat_values:
            print(f"  {feat_name:>26} | {min(feat_values):>10,.2f} | {statistics.mean(feat_values):>10,.2f} | {statistics.median(feat_values):>10,.2f} | {max(feat_values):>10,.2f}")
    
    # 高勾配ケース（価値の変化が激しい）
    print(f"\n【平均勾配が最も高い20ケース（価値の変化が激しい）】")
    sorted_by_gradient = sorted(valid_results, 
                               key=lambda x: x['grid_stats'].get('avg_gradient', 0), 
                               reverse=True)
    
    print(f"  Seed | 平均勾配 | 最大勾配 | Score      | M  | U")
    print(f"  " + "-" * 60)
    for r in sorted_by_gradient[:20]:
        gs = r['grid_stats']
        print(f"  {r['seed']:>4} | {gs.get('avg_gradient', 0):>8,.2f} | {gs.get('max_gradient', 0):>8,.0f} | {r['score']:>10,} | {r['m'] or 'N/A':>2} | {r['u'] or 'N/A':>2}")
    
    # 高価値マスの集中度分析
    print(f"\n【高価値マスの集中度が高い20ケース（固まっている）】")
    sorted_by_concentration = sorted(valid_results, 
                                    key=lambda x: x['grid_stats'].get('high_value_concentration', 0))
    
    print(f"  Seed | 集中度   | 中心/端比 | Score      | M  | U")
    print(f"  " + "-" * 60)
    for r in sorted_by_concentration[:20]:
        gs = r['grid_stats']
        print(f"  {r['seed']:>4} | {gs.get('high_value_concentration', 0):>8,.2f} | {gs.get('center_edge_ratio', 0):>9,.2f} | {r['score']:>10,} | {r['m'] or 'N/A':>2} | {r['u'] or 'N/A':>2}")
    
    print(f"\n【高価値マスの集中度が低い20ケース（散らばっている）】")
    print(f"  Seed | 集中度   | 中心/端比 | Score      | M  | U")
    print(f"  " + "-" * 60)
    for r in sorted_by_concentration[-20:]:
        gs = r['grid_stats']
        print(f"  {r['seed']:>4} | {gs.get('high_value_concentration', 0):>8,.2f} | {gs.get('center_edge_ratio', 0):>9,.2f} | {r['score']:>10,} | {r['m'] or 'N/A':>2} | {r['u'] or 'N/A':>2}")
    
    # 中心と端の比較
    print(f"\n【中心部の価値が高い20ケース】")
    sorted_by_center_ratio = sorted(valid_results, 
                                   key=lambda x: x['grid_stats'].get('center_edge_ratio', 0), 
                                   reverse=True)
    
    print(f"  Seed | 中心/端比 | 中心平均 | 端平均   | Score      | M  | U")
    print(f"  " + "-" * 70)
    for r in sorted_by_center_ratio[:20]:
        gs = r['grid_stats']
        print(f"  {r['seed']:>4} | {gs.get('center_edge_ratio', 0):>9,.2f} | {gs.get('center_mean', 0):>8,.0f} | {gs.get('edge_mean', 0):>8,.0f} | {r['score']:>10,} | {r['m'] or 'N/A':>2} | {r['u'] or 'N/A':>2}")
    
    print(f"\n【端部の価値が高い20ケース】")
    print(f"  Seed | 中心/端比 | 中心平均 | 端平均   | Score      | M  | U")
    print(f"  " + "-" * 70)
    for r in sorted_by_center_ratio[-20:]:
        gs = r['grid_stats']
        print(f"  {r['seed']:>4} | {gs.get('center_edge_ratio', 0):>9,.2f} | {gs.get('center_mean', 0):>8,.0f} | {gs.get('edge_mean', 0):>8,.0f} | {r['score']:>10,} | {r['m'] or 'N/A':>2} | {r['u'] or 'N/A':>2}")
    
    # 最大値の位置分析
    print(f"\n【最大値が中心から遠い20ケース】")
    sorted_by_max_dist = sorted(valid_results, 
                               key=lambda x: x['grid_stats'].get('max_dist_from_center', 0), 
                               reverse=True)
    
    print(f"  Seed | 中心距離 | 最大値   | Score      | M  | U")
    print(f"  " + "-" * 60)
    for r in sorted_by_max_dist[:20]:
        gs = r['grid_stats']
        print(f"  {r['seed']:>4} | {gs.get('max_dist_from_center', 0):>8,.2f} | {gs.get('max', 0):>8,.0f} | {r['score']:>10,} | {r['m'] or 'N/A':>2} | {r['u'] or 'N/A':>2}")

def enemy_placement_analysis(results):
    """敵配置の詳細分析"""
    print("\n" + "=" * 80)
    print("敵配置分析")
    print("=" * 80)
    
    # 敵配置データがある結果のみをフィルタ
    valid_results = [r for r in results if r['enemy_features'] is not None]
    
    if not valid_results:
        print("\n  敵配置データがありません")
        return
    
    # 敵配置の基本統計
    print("\n【敵配置の基本統計】")
    
    enemy_features = [
        ('enemy_avg_distance', '敵同士の平均距離'),
        ('enemy_min_distance', '敵同士の最短距離'),
        ('enemy_max_distance', '敵同士の最長距離'),
        ('enemy_spread', '敵の散らばり度'),
        ('enemy_centroid_from_center', '敵重心の中心距離'),
        ('enemy_avg_value', '敵位置の平均価値'),
        ('enemy_min_dist_to_high_value', '高価値マスへの平均距離')
    ]
    
    print(f"\n  特徴量                     |       最小 |       平均 |     中央値 |       最大")
    print(f"  " + "-" * 80)
    
    for feat_key, feat_name in enemy_features:
        feat_values = [r['enemy_features'][feat_key] for r in valid_results 
                      if feat_key in r['enemy_features']]
        if feat_values:
            print(f"  {feat_name:>26} | {min(feat_values):>10,.2f} | {statistics.mean(feat_values):>10,.2f} | {statistics.median(feat_values):>10,.2f} | {max(feat_values):>10,.2f}")
    
    # 敵が固まっているケース（平均距離が短い）
    print(f"\n【敵が固まっている20ケース】")
    sorted_by_distance = sorted(valid_results, 
                               key=lambda x: x['enemy_features'].get('enemy_avg_distance', 999))
    
    print(f"  Seed | 敵平均距離 | 散らばり度 | Score      | M  | U")
    print(f"  " + "-" * 65)
    for r in sorted_by_distance[:20]:
        ef = r['enemy_features']
        print(f"  {r['seed']:>4} | {ef.get('enemy_avg_distance', 0):>10,.2f} | {ef.get('enemy_spread', 0):>10,.2f} | {r['score']:>10,} | {r['m'] or 'N/A':>2} | {r['u'] or 'N/A':>2}")
    
    # 敵が散らばっているケース
    print(f"\n【敵が散らばっている20ケース】")
    print(f"  Seed | 敵平均距離 | 散らばり度 | Score      | M  | U")
    print(f"  " + "-" * 65)
    for r in sorted_by_distance[-20:]:
        ef = r['enemy_features']
        print(f"  {r['seed']:>4} | {ef.get('enemy_avg_distance', 0):>10,.2f} | {ef.get('enemy_spread', 0):>10,.2f} | {r['score']:>10,} | {r['m'] or 'N/A':>2} | {r['u'] or 'N/A':>2}")
    
    # 敵が高価値マスに近いケース
    print(f"\n【敵が高価値マスに近い20ケース】")
    sorted_by_high_value_dist = sorted(valid_results, 
                                      key=lambda x: x['enemy_features'].get('enemy_min_dist_to_high_value', 999))
    
    print(f"  Seed | 高価値距離 | 敵位置価値 | Score      | M  | U")
    print(f"  " + "-" * 65)
    for r in sorted_by_high_value_dist[:20]:
        ef = r['enemy_features']
        print(f"  {r['seed']:>4} | {ef.get('enemy_min_dist_to_high_value', 0):>10,.2f} | {ef.get('enemy_avg_value', 0):>10,.2f} | {r['score']:>10,} | {r['m'] or 'N/A':>2} | {r['u'] or 'N/A':>2}")
    
    # 敵が高価値マスから遠いケース
    print(f"\n【敵が高価値マスから遠い20ケース】")
    print(f"  Seed | 高価値距離 | 敵位置価値 | Score      | M  | U")
    print(f"  " + "-" * 65)
    for r in sorted_by_high_value_dist[-20:]:
        ef = r['enemy_features']
        print(f"  {r['seed']:>4} | {ef.get('enemy_min_dist_to_high_value', 0):>10,.2f} | {ef.get('enemy_avg_value', 0):>10,.2f} | {r['score']:>10,} | {r['m'] or 'N/A':>2} | {r['u'] or 'N/A':>2}")
    
    # 敵の重心が中心から遠いケース
    print(f"\n【敵の重心が中心から遠い20ケース】")
    sorted_by_centroid = sorted(valid_results, 
                               key=lambda x: x['enemy_features'].get('enemy_centroid_from_center', 0), 
                               reverse=True)
    
    print(f"  Seed | 重心中心距離 | 敵平均距離 | Score      | M  | U")
    print(f"  " + "-" * 65)
    for r in sorted_by_centroid[:20]:
        ef = r['enemy_features']
        print(f"  {r['seed']:>4} | {ef.get('enemy_centroid_from_center', 0):>12,.2f} | {ef.get('enemy_avg_distance', 0):>10,.2f} | {r['score']:>10,} | {r['m'] or 'N/A':>2} | {r['u'] or 'N/A':>2}")
    
    # 敵の位置の価値が高い/低いケース
    print(f"\n【敵の位置の価値が高い20ケース】")
    sorted_by_enemy_value = sorted(valid_results, 
                                  key=lambda x: x['enemy_features'].get('enemy_avg_value', 0), 
                                  reverse=True)
    
    print(f"  Seed | 敵位置価値 | 高価値距離 | Score      | M  | U")
    print(f"  " + "-" * 65)
    for r in sorted_by_enemy_value[:20]:
        ef = r['enemy_features']
        print(f"  {r['seed']:>4} | {ef.get('enemy_avg_value', 0):>10,.2f} | {ef.get('enemy_min_dist_to_high_value', 0):>10,.2f} | {r['score']:>10,} | {r['m'] or 'N/A':>2} | {r['u'] or 'N/A':>2}")
    
    print(f"\n【敵の位置の価値が低い20ケース】")
    print(f"  Seed | 敵位置価値 | 高価値距離 | Score      | M  | U")
    print(f"  " + "-" * 65)
    for r in sorted_by_enemy_value[-20:]:
        ef = r['enemy_features']
        print(f"  {r['seed']:>4} | {ef.get('enemy_avg_value', 0):>10,.2f} | {ef.get('enemy_min_dist_to_high_value', 0):>10,.2f} | {r['score']:>10,} | {r['m'] or 'N/A':>2} | {r['u'] or 'N/A':>2}")

def generate_csv_report(results, output_file='analysis_report.csv'):
    """CSV形式でレポートを出力"""
    print(f"\n" + "=" * 80)
    print(f"CSV形式でレポートを出力: {output_file}")
    print("=" * 80)
    
    with open(output_file, 'w') as f:
        f.write("Seed,Score,ExecTime,N,M,T,U,GridMean,GridStd,GridMax,GridMin,")
        f.write("AvgGradient,MaxGradient,HighValueConcentration,MaxDistFromCenter,")
        f.write("CenterEdgeRatio,CenterMean,EdgeMean,CornersMean,")
        f.write("EnemyAvgDistance,EnemySpread,EnemyCentroidFromCenter,EnemyAvgValue,EnemyMinDistToHighValue\n")
        
        for r in sorted(results, key=lambda x: int(x['seed'])):
            n = r['n'] if r['n'] is not None else ''
            m = r['m'] if r['m'] is not None else ''
            t = r['t'] if r['t'] is not None else ''
            u = r['u'] if r['u'] is not None else ''
            
            if r['grid_stats'] is not None:
                gs = r['grid_stats']
                grid_mean = f"{gs['mean']:.2f}"
                grid_std = f"{gs['std']:.2f}"
                grid_max = gs['max']
                grid_min = gs['min']
                avg_gradient = f"{gs.get('avg_gradient', 0):.2f}"
                max_gradient = gs.get('max_gradient', 0)
                concentration = f"{gs.get('high_value_concentration', 0):.2f}"
                max_dist = f"{gs.get('max_dist_from_center', 0):.2f}"
                center_edge = f"{gs.get('center_edge_ratio', 0):.2f}"
                center_mean = f"{gs.get('center_mean', 0):.2f}"
                edge_mean = f"{gs.get('edge_mean', 0):.2f}"
                corners = f"{gs.get('corners_mean', 0):.2f}"
            else:
                grid_mean = grid_std = grid_max = grid_min = ''
                avg_gradient = max_gradient = concentration = max_dist = ''
                center_edge = center_mean = edge_mean = corners = ''
            
            if r['enemy_features'] is not None:
                ef = r['enemy_features']
                enemy_avg_dist = f"{ef.get('enemy_avg_distance', 0):.2f}"
                enemy_spread = f"{ef.get('enemy_spread', 0):.2f}"
                enemy_centroid = f"{ef.get('enemy_centroid_from_center', 0):.2f}"
                enemy_avg_value = f"{ef.get('enemy_avg_value', 0):.2f}"
                enemy_min_dist_high = f"{ef.get('enemy_min_dist_to_high_value', 0):.2f}"
            else:
                enemy_avg_dist = enemy_spread = enemy_centroid = ''
                enemy_avg_value = enemy_min_dist_high = ''
            
            f.write(f"{r['seed']},{r['score']},{r['exec_time']},{n},{m},{t},{u},")
            f.write(f"{grid_mean},{grid_std},{grid_max},{grid_min},")
            f.write(f"{avg_gradient},{max_gradient},{concentration},{max_dist},")
            f.write(f"{center_edge},{center_mean},{edge_mean},{corners},")
            f.write(f"{enemy_avg_dist},{enemy_spread},{enemy_centroid},{enemy_avg_value},{enemy_min_dist_high}\n")
    
    print(f"  ✓ {output_file} に出力完了")

def main():
    # 表データを読み取る
    if len(sys.argv) > 1:
        with open(sys.argv[1], 'r') as f:
            table_text = f.read()
    else:
        print("表データを貼り付けてください（Ctrl+D で終了）:")
        table_text = sys.stdin.read()
    
    # 入力ファイルのディレクトリを指定
    in_dir = sys.argv[2] if len(sys.argv) > 2 else 'tools/in'
    
    # 表をパース
    results = parse_table(table_text)
    
    if not results:
        print("エラー: 表データをパースできませんでした", file=sys.stderr)
        sys.exit(1)
    
    print(f"\n解析開始: {len(results)} 件のテストケース\n")
    
    # パラメータ情報を追加
    enriched_results = enrich_results_with_params(results, in_dir)
    
    # 各種分析を実行
    basic_statistics(enriched_results)
    parameter_analysis(enriched_results)
    score_distribution(enriched_results)
    correlation_analysis(enriched_results)
    grid_value_analysis(enriched_results)
    enemy_placement_analysis(enriched_results)
    parameter_combination_analysis(enriched_results)
    find_outliers(enriched_results)
    top_bottom_cases(enriched_results, n=20)
    execution_time_analysis(enriched_results)
    
    # CSV出力
    generate_csv_report(enriched_results)
    
    print("\n" + "=" * 80)
    print("分析完了")
    print("=" * 80)

if __name__ == '__main__':
    main()
