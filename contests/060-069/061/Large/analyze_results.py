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

def enrich_results_with_params(results, in_dir='in'):
    """結果にパラメータ情報を追加"""
    enriched_results = []
    for result in results:
        seed = result['seed']
        n, m, t, u = read_test_params(seed, in_dir)
        
        enriched_result = result.copy()
        enriched_result['n'] = int(n) if n and n.isdigit() else None
        enriched_result['m'] = int(m) if m and m.isdigit() else None
        enriched_result['t'] = int(t) if t and t.isdigit() else None
        enriched_result['u'] = int(u) if u and u.isdigit() else None
        enriched_result['exec_time_int'] = int(result['exec_time']) if result['exec_time'].isdigit() else 0
        enriched_results.append(enriched_result)
    
    return enriched_results

def print_results(results, sort_key='score', reverse=True):
    """結果を表形式で出力"""
    print("| Seed | Score      | Exec Time | N  | M | T   | U  |")
    print("|------|------------|-----------|----|----|-----|----|")
    
    # ソートキーに応じてソート（同率の場合はスコア→実行時間でソート）
    def get_sort_key(x):
        # パラメータ値（Noneは最後に来るように大きな負の値を設定）
        param_keys = {
            'n': x['n'] if x['n'] is not None else -float('inf'),
            'm': x['m'] if x['m'] is not None else -float('inf'),
            't': x['t'] if x['t'] is not None else -float('inf'),
            'u': x['u'] if x['u'] is not None else -float('inf'),
        }
        
        if sort_key == 'score':
            # スコアソート: スコア降順 → 実行時間昇順
            return (-x['score'], x['exec_time_int'])
        elif sort_key == 'seed':
            # シードソート: シード順のみ
            return (int(x['seed']),)
        elif sort_key == 'exec_time':
            # 実行時間ソート: 実行時間 → スコア降順
            return (x['exec_time_int'], -x['score']) if reverse else (-x['exec_time_int'], -x['score'])
        elif sort_key in param_keys:
            # パラメータソート: パラメータ値 → スコア降順 → 実行時間昇順
            return (param_keys[sort_key], -x['score'], x['exec_time_int']) if reverse else (-param_keys[sort_key], -x['score'], x['exec_time_int'])
        else:
            return (0,)
    
    sorted_results = sorted(results, key=get_sort_key, reverse=reverse if sort_key == 'score' else False)
    
    for result in sorted_results:
        seed = result['seed']
        score = result['score']
        exec_time = result['exec_time']
        
        # パラメータが取得できなかった場合は"N/A"を表示
        n = str(result['n']) if result['n'] is not None else "N/A"
        m = str(result['m']) if result['m'] is not None else "N/A"
        t = str(result['t']) if result['t'] is not None else "N/A"
        u = str(result['u']) if result['u'] is not None else "N/A"
        
        print(f"| {seed} | {score:>10,} | {exec_time:>9} | {n:>2} | {m:>2} | {t:>3} | {u:>2} |")

def print_statistics(results):
    """統計情報を表示"""
    print(f"\n--- 統計情報 ---")
    total_score = sum(r['score'] for r in results)
    avg_score = total_score / len(results)
    print(f"合計スコア: {total_score:,}")
    print(f"平均スコア: {avg_score:,.2f}")
    print(f"最高スコア: {max(r['score'] for r in results):,}")
    print(f"最低スコア: {min(r['score'] for r in results):,}")

def interactive_sort(results):
    """インタラクティブにソートキーを受け付けて再表示"""
    print("\n" + "="*60)
    print("ソートオプション:")
    print("  score  : スコア順 (デフォルト)")
    print("  seed   : シード番号順")
    print("  exec_time: 実行時間順")
    print("  n, m, t, u: 各パラメータ順")
    print("  q      : 終了")
    print("="*60)
    
    while True:
        try:
            user_input = input("\nソートキーを入力 (例: m, n, score など): ").strip().lower()
            
            if user_input == 'q' or user_input == 'quit' or user_input == 'exit':
                print("終了します。")
                break
            
            if not user_input:
                continue
            
            # 降順・昇順の指定を受け付ける
            reverse = True
            if user_input.endswith('+'):
                user_input = user_input[:-1]
                reverse = False
            elif user_input.endswith('-'):
                user_input = user_input[:-1]
                reverse = True
            
            valid_keys = ['score', 'seed', 'exec_time', 'n', 'm', 't', 'u']
            if user_input not in valid_keys:
                print(f"エラー: '{user_input}' は無効なキーです。有効なキー: {', '.join(valid_keys)}")
                continue
            
            print(f"\n'{user_input}' でソート ({'降順' if reverse else '昇順'}):\n")
            print_results(results, sort_key=user_input, reverse=reverse)
            
        except EOFError:
            print("\n終了します。")
            break
        except KeyboardInterrupt:
            print("\n終了します。")
            break

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
    
    # パラメータ情報を追加
    enriched_results = enrich_results_with_params(results, in_dir)
    
    # 結果を表示（初期はスコア順）
    print("初期表示 (スコア降順):\n")
    print_results(enriched_results, sort_key='score', reverse=True)
    
    # 統計情報を表示
    print_statistics(enriched_results)
    
    # インタラクティブソート
    interactive_sort(enriched_results)

if __name__ == '__main__':
    main()
