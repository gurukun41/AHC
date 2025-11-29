# URL
https://www.terry-u16.net/entry/how-to-use-pahcer

# 初期化
```bash
pahcer init -p <PROBLEM_NAME> -o <OBJECTIVE> -l <LANGUAGE> [-i]
```
- ```<PROBLEM_NAME>``` : コンテスト名
- ```<OBJECTIVE>``` : スコアを最大化するか最小化するか
    - max: スコアが大きい方が良い
    - min: スコアが小さい方が良い
- ```<LANGUAGE>``` : 使用言語
    - cpp: C++
    - python: Python
    - rust: Rust
    - go: Go
- ```-i``` : インタラクティブ問題かどうか

# テストケース実行
```bash
pahcer run
```


コメントを付けるには
```bash
pahcer run -c <comment>
```

# 実行結果の確認
```bash
pahcer list
```

