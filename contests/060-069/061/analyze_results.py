#!/usr/bin/env python3
import re
import sys
from pathlib import Path

def parse_table(table_text):
    """表データをパースしてシード、スコア、実行時間を抽出"""
    results = []
    lines = table_text.strip().split('\n')
    
    for line in lines:
        # データ行のみを処理（ヘッダーや区切り線をスキップ）
        if '/' in line and '|' in line:
            parts = [p.strip() for p in line.split('|')]
            if len(parts) >= 8:
                try:
                    # シード番号を抽出
                    seed = parts[2].strip()
                    if seed and seed.isdigit():
                        # スコアを抽出（カンマを除去）
                        score = parts[3].strip().split()[0].replace(',', '')
                        # 実行時間を抽出（ms を除去）
                        exec_time = parts[7].strip().replace(',', '').replace(' ms', '')
                        
                        results.append({
                            'seed': seed,
                            'score': int(score),
                            'exec_time': exec_time
                        })
                except (ValueError, IndexError):
                    continue
    
    return results

def read_test_params(seed, in_dir='in'):
    """テストケースファイルからN, M, T, Uを読み取る"""
    test_file = Path(in_dir) / f"{seed}.txt"
    
    if not test_file.exists():
        return None, None, None, None
    
    try:
        with open(test_file, 'r') as f:
            first_line = f.readline().strip()
            params = first_line.split()
            if len(params) >= 4:
                return params[0], params[1], params[2], params[3]
    except Exception as e:
        print(f"Error reading {test_file}: {e}", file=sys.stderr)
    
    return None, None, None, None

def print_results(results, in_dir='in'):
    """結果を表形式で出力"""
    print("| Seed | Score      | Exec Time | N  | M | T   | U  |")
    print("|------|------------|-----------|----|----|-----|----|")
    
    # スコアでソート（降順）
    sorted_results = sorted(results, key=lambda x: x['score'], reverse=True)
    
    for result in sorted_results:
        seed = result['seed']
        score = result['score']
        exec_time = result['exec_time']
        
        # テストケースからパラメータを取得
        n, m, t, u = read_test_params(seed, in_dir)
        
        # パラメータが取得できなかった場合は"N/A"を表示
        n = n if n else "N/A"
        m = m if m else "N/A"
        t = t if t else "N/A"
        u = u if u else "N/A"
        
        print(f"| {seed} | {score:>10,} | {exec_time:>9} | {n:>2} | {m:>2} | {t:>3} | {u:>2} |")

def main():
    # 表データ（コマンドライン引数または標準入力から読み取る）
    if len(sys.argv) > 1:
        # ファイルから読み取る場合
        with open(sys.argv[1], 'r') as f:
            table_text = f.read()
    else:
        # 標準入力から読み取る
        print("表データを貼り付けてください（Ctrl+D で終了）:")
        table_text = sys.stdin.read()
    
    # 入力ファイルのディレクトリを指定（デフォルトは 'tools/in'）
    in_dir = sys.argv[2] if len(sys.argv) > 2 else 'tools/in'
    
    # 表をパース
    results = parse_table(table_text)
    
    if not results:
        print("エラー: 表データをパースできませんでした", file=sys.stderr)
        sys.exit(1)
    
    print(f"\n解析結果: {len(results)} 件のテストケース\n")
    
    # 結果を表示
    print_results(results, in_dir)
    
    # 統計情報を表示
    print(f"\n--- 統計情報 ---")
    total_score = sum(r['score'] for r in results)
    avg_score = total_score / len(results)
    print(f"合計スコア: {total_score:,}")
    print(f"平均スコア: {avg_score:,.2f}")
    print(f"最高スコア: {max(r['score'] for r in results):,}")
    print(f"最低スコア: {min(r['score'] for r in results):,}")

if __name__ == '__main__':
    main()
