# AHC069 主要知見

詳細な根拠・全診断値は[README.md](README.md)から対応する履歴を選んで読む。このファイルは再利用する結論だけを残す。

## 実験の読み方

- seed 0だけでは逆転が多い。100 seed paired比較は行動互換性と最初の効果確認には使えるが、高分散な性能・model判断の十分条件ではない。公式generatorからseedを自動生成し、train / validation / finalをseed非重複で分ける。
- compile-time Controlが既存oracleとscore・stdoutで一致することをhard gateにする。
- 公式暫定ケースは母数とbest scoreが変動する。方策比較より、提出時間の確認資料として扱う。
- 浮動小数点の集計順やFMA差だけでも後続盤面が連鎖分岐する。A/B外の経路はbyte一致まで確認する。
- 受理数だけでは良否を判断しない。受理集合の理想料金、shape loss、再配置料金低下、移動費へ分解する。
- rollout予測marginと最終差の相関はしばしば弱い。予測利益だけで採用せずpaired実測する。

## 大きく効いたもの

- `shadow-price-v1`: 固定分位拒否を時刻別容量価格へ変更し、当時の`admission-v3`比約`+2.57%`。
- `placement-fit-v1`: 周長ladder、connected-growth、境界増分、future compact-fitで`shadow-price-v1`比`+11.927%`、99勝1敗。最大の改善源。
- `grow-and-trim`: connected-growthへ追加して`+0.6975%`。受理数よりshape loss改善が効いた。
- sampled DLP: 旧64 bucket shadow置換で`+1.2338%`。低価値到着を早く断り、高価値到着の容量を残した。

## 小幅に効いたもの・基盤へ残ったもの

- Compact rescueと候補幅2の共通未来rolloutは小幅改善。ただし即時利益の相当部分を下流で失う。
- cross-fitted rootでは通常runner-up枝が正、Reject枝が大幅負。最終v7はRejectを削除し、公式`3,409,424,714`。
- NoRegion Push-outはfragmentation救済の共通基盤として残ったが、複雑なhelper拡張の費用対効果は小さい。
- 異周長tier v23はControl比`+0.0346%`だが29勝30敗、bootstrap区間が0をまたぎ、CPU`+17.67%`。確定採用ではない。`+2`は見込み、`+4`は悪化の兆候。
- v26のconnected risk gateは全fallbackの0.51%だけをRejectし、Control比`+0.1515%`。shape lossを21.15M、fragmentation ideal feeを14.42M改善したが、bootstrap区間は0をまたぎ、solver CPU平均`+24.25%`で不採用。blanket Rejectではなく「ごく一部が有害」という仮説は残る。
- v27ではEarlyMidが`+0.1022%`、Lateが`-0.0160%`で時期仮説と部分整合したが、GateAllはEarlyMidをさらに`+0.0493%`上回り、全CIが0をまたいだため`mixed/inconclusive`。単独Rejectの記述差はwindow 2へ集中した。
- Bfs / Multi / GrowTrim限定Gateはそれぞれ`-0.0435% / -0.0957% / -0.1069%`。sourceそのものより、3 sourceが各window 2枠を奪い合う希少な共有screen予算が暗黙の正則化になっている可能性が高い。
- connected risk `R`はReject優先度として未較正で、GateAllの`R>=32`は53 screen中最終Reject 0。risk magnitudeを有害度と同一視しない。
- v28では新規1200 seedをtrain / validation / finalへ分離し、未開封final 200 seedではbaseline Acceptedの29.84%がReject有利、経済Rejectの55.77%がAccept有利、placement rootの重み付き35.69%に良い次点があり、局所的な反転余地自体は存在した。
- finalのconnected AcceptedはReject有利率39.82%とMinimumの25.89%より高いが、平均`Reject-Accept`は`-9.9k`でAccept有利。一律Rejectではなく高信頼な選別が必要である。
- v28でvalidationからfamily・lambdaを選び、train+validationでrefitした固定modelはfinalでadmission `-521.0M`、placement `-928.2M`。いずれもbootstrap CIが0をまたぎ、統計的な有害性はinconclusiveだが昇格根拠がないため不採用とした。admissionでは有害overrideはoverride重みの39.7%でも平均損失が有益側の約2倍であり、符号正解率や勝敗数より損失裾が支配した。
- placementはfinal rootのraw 34.56%、`stratum_population`重み付き35.69%に改善次点があったが、次点行単位の重み付き正率は27.78%で、全次点の重み付き平均gainは負。現行shortlist順位は長期価値順ではなく、無条件に候補幅を広げたり次点へ寄せたりする根拠にはならない。
- 池・topologyは無価値と確定したわけではないが、現行ridgeでは採用根拠を示せなかった。placementのpond championは完全no-opであり、有害overrideを避けただけだった。
- v29の`dlp_scale_milli=1300`はsearch 400 seedで`+1.9274%`、独立block validation 300 seedでも`+1.9887%`と平均には強い正の信号を示した。ただしp05は0.97766で固定0.98 gateを満たさず、final holdoutも未開封なので採用根拠にはしない。合計改善と下位裾の安全性は別に判定する。
- 初期外周率`E/G`で固定1.30 / 1.00を選ぶと、同じfresh validation cache合成で旧full比`+2.018205%`、p05 `0.989437`、worst `0.971320`となり、固定1.30の悪い裾を分離できた。今回の単一adaptive binaryの実測ではないが、未較正のturn別classifierではなく初期盤面だけの静的portfolioとして、後の明示指示でfullへ統合した。
- full + 静的外周適応DLPの開発100 seed実測は平均`66,163,871.34`で新最高。旧full比`+1.553174%`、直前lean比`+0.193411%`だった。constrained 30 seedは旧fullと全件同点で直前leanから`+0.814583%`を回復し、smoothも直前lean比`+0.021424%`を保った。「各ケースで一方しか動かない静的expert選択」は、大きな二solver統合をせず同じコード基盤で実現できる。
- 保存済みv29 cacheを4つの初期外周率領域へ分けたpost-hoc portfolioは、一つ前の静的DLP版比`+0.087686%`だった。`0.55<=E/G<0.70`だけDLP 1.25、`E/G>=0.80`だけ保存p2 placementへ替え、それ以外を保護する。閾値選択と評価が同じvalidation集合なのでfresh保証ではなく、排他的expertを統合する事前根拠としてのみ扱う。
- 通常placementの論理anchorに対する合法率は`1.4388%`、anchor数とCPUのseed相関は`r=0.709`だった。N=50の行bitsetと行方向OR sparse tableなら、テンプレート合法集合・列挙順・tie-breakを変えず合法anchorだけを列挙できる。スコア方策と計算量改善を分ける意味保存最適化として統合し、下記100 seedで実測した。
- 4-expert + `LegalAnchorIndex`の開発100 seed実測は平均`66,219,410.47`で新最高。一つ前比`+0.083942%`、17勝76分7敗だった。変更対象外のexpert 0・2は76/76完全同点、expert 1は`+0.456272%`、expert 3は`+0.537166%`で、排他的portfolioの両変更枝が正だった。ただしpaired bootstrap 95%区間`[-0.0770%,+0.2570%]`は0をまたぎ、seed p05/worstも事前cacheより悪い。
- `LegalAnchorIndex`統合後の内部solver CPUはmean `1644.464→1240.126ms (-24.59%)`、p95 `2314.446→1747.041ms`、2秒超`13→1/100`。同点76 seedと全診断error 0は意味保存の強い実測確認である。Pahcer wallは改善していないため、CPU計測とwall待機を引き続き分ける。
- v29 placement bestは独立validationで`+0.1629%`でもp05 0.96793、root weight 100は`+0.0561%`でもp05 0.97792だった。root 900 / 1100はscore gateを通ったが効果は`+0.0124% / +0.0077%`と小さく、絶対CPU gateで停止した。全block baseline fallbackとなり、Optunaは不採用で終了した。
- root weight 100を地形全体へ広げず、`.70<=E/G<.80 && R<.060`だけへ限定した保存cache合成はsearch/validationの両方で正、全700 seedで`+0.032788%`、11勝686分3敗だった。`E/G>=.80`はvalidationで負なので、静的expertは「良い部分領域だけを切り出し、他枝のplacementも含めて保護する」必要がある。これはroot個別の実行前根拠であり、v31の100 seed実測ではpolishと同時に動いたため個別寄与を確定できない。
- accepted growth残差へ最小周長Ferrers形を固定列挙しても、厳密再生上の改善上限は268件・約2.81Mだった。一方、現障害物に合わせる`P..P+16` near-template boxの存在gateは4,608件・旧shape-loss約159.9Mを覆い、shortlist16 + trim近似でも2,203件・即時料金約26.96Mだった。極値形の理論は固定形一覧より、近正方形envelope内を局所変形する探索へ使う方が残差に合う。
- 非関節1セルを凹部へ移すstrict perimeter descentは、`P>=50`の保存軌跡で811件、周長`-2,052`、即時料金約4.17M。大域再配置ではなくold Acceptedを同面積・同連結のまま磨き、strict料金増・future-fit非悪化・old rollbackを要求する構成なら、幾何proxyをhard rejectへ昇格したv24とは因果境界を分けられる。
- v31の無制限polishは1ケース平均151.87、p95 310回で時間制約に危険だった。denseだけを理論最大料金改善`U>=10,000`かつcase最大24実走査にすると呼出しを84%削減し、swapを全eligibleで残した保存replay概算は約13.12M、無制限unionの44.3%を保持した。先着budgetだけではK24でも29.7%しか残らず、P・周長超過filterも劣後した。高価な生成器と安い補助を別budgetにし、得点の厳密上界で高価な側を選別する。
- v31合成版の開発100 seed実測は平均`66,344,055.84`で新最高。直前4-expert比`+0.188231%`、62勝6分32敗。内部solver CPUはmean `1200.498ms`、p95 `1629.677ms`、max `1916.647ms`、2秒超0/100、53種のerror/mismatchは全件0だった。一方でbootstrap 95%区間`[-0.120014%,+0.498061%]`は0をまたぎ、繰り返し参照した同じ100 seedなのでfresh保証ではない。expert 4の5 seedは全てpolishも動き、合計`-340,051`だったため、限定root単独の効果は分離していない。
- v31差を初期`E/G`で分けると`<.55`は`+18.59M`、`.625-.70`は`-1.71M`、`>=.80`は`-4.64M`だった。同じ開発100 seed上のpost-hocなので性能推定には使えないが、polishを初期盤面だけでsmooth expertへ限定し、悪い地形を途中状態なしで保護する根拠になる。
- 固定面積領域は`L=4P-2E_internal`なので、strict one-cell swap停止形は1-exchange局所最適である。zero-gain交換の両端をlockして同周長plateauだけを渡り、各stateからstrict descentを再開すれば、途中周長を悪化させず別basinへ移れる。負gainを許す本来のFM/KL passやdestroy/repair VNSより狭いが、一回実行で既存候補を保護しやすい。
- scalar future-fitは、ある未来snapshot・正方形sideの悪化を別sideの改善で相殺する。tail guardに使うなら、旧形との共通snapshotごと・sideごとのPareto非悪化と、最大空き成分・`sum floor(component/q)`のcapacity profileを候補限定で併用できる。proxy単独でadmissionを反転せず、old候補を残すことが前提である。
- v32のsmooth限定polish + dense `U>=50,000` + zero plateau + Pareto guardは100 seedで平均`66,341,814.82`、v31比`-224,102 (-0.003378%)`、39勝18分43敗だった。正差`+39.74M`と負差`-39.96M`がほぼ相殺し、保存140 run中raw合計2位。単一seedや勝数ではなくtotalとtailを同時に見る必要がある。
- v32−v31を初期`E/G`で分けると、非smooth 30 seedは合計`+7.256M`、smooth 70 seedは`-7.480M`だった。高外周率でv31 polishを止める静的保護は合計正だった一方、smoothへ同時導入したdense予算・descent Pareto・plateauの合成差は負。複数変更を同時導入すると、全体ほぼ同点でも改善枝と悪化枝が相殺されるため、個別寄与は結果だけから決めない。
- v33では正だった既存`E/G<.55`静的gateだけをv32から残し、smooth側は実測済みv31のdense `U>=10,000`・strict descent・scalar future-fitへ一括復元した。100 seed実測は平均`66,416,618.58`で、実行前のcasewise反実仮想と100/100 seed一致しraw新最高。`.625`まで広げる方が同じ集合上では高くても、結果を見て新境界を足すpost-hoc過適合を避け、既存expert境界で合成する。ただし同じ開発集合の再利用なのでfresh保証ではない。
- v34の同一最大gain limited-discrepancy分岐は、100 seedでv33比`-714,127 (-0.010752%)`、3勝95分2敗だった。追加枝の即時fee gain合計`+30,216`より後続影響`-744,343`が大きく、候補幅を増やしてproxyで選ぶだけでは真の未来を改善しない。負だった2 seedを除くpost-hoc gateは作らず、分岐全体を撤去した。候補portfolioを増やすときは、探索器だけでなく独立な選択精度・CPU・旧action集合の保護を一体で設計する。
- `P>=50`はconnected shape lossの93.69%を覆うimpact/CPU gateであり、strict 1-cell descentの合法性条件ではない。高価なdense boxはこのgateへ残し、既存のstrict料金増・future-fit非悪化・old rollbackを保った安いdescentだけを`P<50`へ一般化すれば、新しい近傍や学習selectorを増やさず約6.31%の残差を攻められる。各removeでfrontier最大近傍数がremove近傍数以下なら正gain不可能なので、意味保存prefilterで追加CPUを抑えられる。
- v35の小規模strict descent一般化は100 seedでv33比`+2,203,649 (+0.033179%)`、19勝81分0敗、negative gross 0だった。seed 42を除いても`+149,729`・18勝81分0敗なので小さな直接改善は広く再現した一方、合計の93.205%はseed 42の後続連鎖に集中した。大きな合計差だけで新しい分類器を作らず、「負seedなし」と「効果量集中」を分けて次の判断へ使う。
- 終了済みAHCの上位解では、AHC031の全期間短冊、AHC040の衝突しない制約、AHC065の全域を覆う巡回路のように、強い骨格を先に固定してから低次元近傍だけを最適化する例が繰り返し現れる。候補portfolioを増やして既存proxyへ選ばせるより、価値の高い不変条件を守る近傍へ未探索領域を限定する方が、v34のoptimizer's curseとも整合する。
- v36はこの原則を、非smoothかつ`P<50`のstrict descentへ限定して実装した。removeセルが既存空きへ接し、removeを空きへ戻した中間グラフでaddセルが非関節点なら、swap後の空き連結成分数は増えない。これは空き成分を評価値で最大化する過去の失敗とは異なり、料金・future-fit・rootを置換しない二値の近傍制限である。smoothのv35、非smoothの`P>=50`、dense、admission/repackingは不変とする。
- v36の100 seed実測はv35比`-3,079,288 (-0.046348%)`・8勝88分4敗だった。smooth 70 seedの完全同点により経路隔離は確認できたが、非smoothの正差`+41,707`に対して負差は`-3,120,995`で、4敗のtailが支配した。空き連結成分数を増やさない局所不変条件だけでは、空き骨格の形状・将来の連鎖価値までは保存できない。結果を見た後に`E/G`境界や負seed除外gateを足さず、v36固有コードを全撤去して実行時v35へ完全復元した。
- 残り未来組数`R`と、各組が現在配置の退去までに到着する条件付き確率`q`の積`Rq`は期待干渉件数だが、目的関数上`Rq=1`に不連続点はない。v37のhard gateは3000 caseでv35比`-0.004618%`となり、問題構造の再検討後に撤去した。未来価値は`Rq`へ連続的に掛け、`R=0`だけ厳密停止する。
- 公式生成式では`fee/(P D)≈compactness*2^Z*D^-0.1`となり、面積`P`はcell-time価値密度から消える。Pの主要な役割を連結配置とcompactnessとみなし、v38では実到着の同一料金候補を公式P分布・最良周長template列のfractional cell priceで選んだ。しかし100 / 3000 caseでv35比`-0.196676% / -0.119609%`、評価turnの57.734%で既存選択を変更し、solver CPU平均`2083.124ms`だったため撤去した。公式分布由来でも、全合法列への一様fractional配分は逐次競合の将来価値を十分較正できない可能性があり、「同一料金内だけ」も安全性を意味しない。結果から係数・seed・地形gateを後付けせず、v35全体へ戻した。

## 明確に失敗した方向

- 空き連結成分の大きさを直接最大化するcomponent評価は、compact領域とshape feeを壊して大敗。
- 静的な局所改善だけで選ぶ再配置は後続連鎖を読めない。比較するならcommon-random-numberの同一方策rolloutが必要。
- Deadline Layer大域再構成はControlの82.69%、0勝100敗。独自配置がshape retentionを失った。
- one-helper Push-outは探索を広げても効果が約`+0.045%`に留まり、専用複雑性を削除した。
- predictive sparse DP v17はControlの89.76%、quality-first v19は5 seedで81.14%かつ平均33秒。メモリではなく状態表現・目的の問題。
- 複数assignment v20、固定移動費shortlist v21、ActualFeeRejected救済v22は全てControl未満。
- v24の周長hard reject・未来障壁rejectは各armとも0勝100敗。幾何信号を受理拒否へ昇格したことが中心的失敗。
- v25のcell×time空間DLPはSameFee`-4.53%`、Full`-3.98%`。予測改善と実差の符号が逆で、価格順位が未較正。
- 通常templateを`Lmin+4`から`Lmin+8`へ一律拡張したv26 Wideは`-0.1174%`。connected選択を0.72%しか減らせず、shape loss改善よりaccepted ideal fee減少が大きかった。
- 全候補を共通12標本の`配置可能な独立未来料金`で比較した案は、同一最短周長内future-fit版比`-1.1388%`。未来組を盤面へ逐次反映せず、各標本の合法anchor有無だけで料金を重複加算した値は期待総料金ではない。候補数を広げるほど少数標本のwinner's curseも強くなる。
- `log(duration/theta)`上の連続受入率を未来標本の母数へ掛けた案はbaseline比`-6.1045%`。実Yes/Noをそのまま潜在未来需要から間引く補正は、DLP内部の料金選別と重なり、将来需要を過小評価した可能性が高い。

## 設計原則

1. admission、placement、repackingを分離する。
2. 料金・形状品質を主軸から外さない。空間proxy単独で受理拒否しない。
3. 既存baselineを候補に残し、同点時は動かさない。
4. 安全側の候補だけに狭めない。広い案を試す場合はSameFee等のablationを併設して失敗原因を分ける。
5. 移動費と過去最大周長による恒久料金低下を一度だけ正確に控除する。
6. rolloutはcase内one-shot、候補幅2、十分な深さが比較的良かった。余り予算を後続turnへ回した移動は悪化した。
7. 予測モデルの診断は最終scoreだけでなく、予測margin対実差、overload、price coverage、候補source、下流損失で見る。
8. Pahcer wallは背景負荷と対話待ちを含む。コード内のsolver CPUを主資料にする。
9. 学習・選択ではno-changeを明示候補にし、refit後もcross-fitで安定性を確認する。CIや十分な安全marginがなければoverrideせずabstainする。
10. admissionはMinimum / Extended / connected Accepted / UpperBoundRejected / ActualFeeRejectedという排他的cohortとして扱う。双方向の反転を1本の線形modelへ混ぜず、頻度ではなくtail regretを抑える。
11. 絶対CPU gateは実行環境のbaseline分布へ合わせる。v29では上限2000msに対して同一集合のbaseline自身がp95 2146ms・最大2715msとなり、全非baseline候補を絶対値だけで拒否した。相対平均・p95と制限時間余裕を分けて固定する。
