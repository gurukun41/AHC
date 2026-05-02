#include <bits/stdc++.h>
#include <chrono>
using namespace std;

namespace ahc064 {

// ------------------------------
// Problem constants
// ------------------------------
constexpr int R = 10;
constexpr int DEPART_CAP = 15;
constexpr int SIDING_CAP = 20;
constexpr int TARGET_LEN = 10;
constexpr int TURN_LIMIT = 4000;

// ------------------------------
// I/O model
// ------------------------------
struct Input {
	int r = R;
	array<array<int, TARGET_LEN>, R> y{};
};

Input read_input() {
	Input in;
	cin >> in.r;
	for (int i = 0; i < in.r; ++i) {
		for (int c = 0; c < TARGET_LEN; ++c) {
			cin >> in.y[i][c];
		}
	}
	return in;
}

// ------------------------------
// Operation model
// ------------------------------
struct Move {
	int type = 0;
	int i = 0;
	int j = 0;
	int k = 1;
};

struct Turn {
	vector<Move> moves;
};

using Plan = vector<Turn>;

// ------------------------------
// State simulator / validator
// ------------------------------
class State {
  public:
	array<deque<int>, R> dep;
	array<deque<int>, R> sid;

	explicit State(const Input& in) {
		for (int i = 0; i < R; ++i) {
			for (int c = 0; c < TARGET_LEN; ++c) {
				dep[i].push_back(in.y[i][c]);
			}
		}
	}

	bool is_goal() const {
		for (int r = 0; r < R; ++r) {
			if ((int)dep[r].size() != TARGET_LEN) return false;
			for (int c = 0; c < TARGET_LEN; ++c) {
				if (dep[r][c] != r * TARGET_LEN + c) return false;
			}
		}
		return true;
	}

	bool can_apply_move(const Move& mv) const {
		if (!(mv.type == 0 || mv.type == 1)) return false;
		if (mv.i < 0 || mv.i >= R || mv.j < 0 || mv.j >= R) return false;
		if (mv.k <= 0) return false;

		if (mv.type == 0) {
			if ((int)dep[mv.i].size() < mv.k) return false;
			if ((int)sid[mv.j].size() + mv.k > SIDING_CAP) return false;
		} else {
			if ((int)sid[mv.j].size() < mv.k) return false;
			if ((int)dep[mv.i].size() + mv.k > DEPART_CAP) return false;
		}
		return true;
	}

	static bool non_crossing(const Turn& t) {
		array<int, R> dep_used{};
		array<int, R> sid_used{};
		for (const auto& mv : t.moves) {
			if (mv.i < 0 || mv.i >= R || mv.j < 0 || mv.j >= R) return false;
			dep_used[mv.i]++;
			sid_used[mv.j]++;
			if (dep_used[mv.i] > 1 || sid_used[mv.j] > 1) return false;
		}

		vector<pair<int, int>> pairs;
		pairs.reserve(t.moves.size());
		for (const auto& mv : t.moves) pairs.emplace_back(mv.i, mv.j);
		sort(pairs.begin(), pairs.end());
		for (int idx = 1; idx < (int)pairs.size(); ++idx) {
			if (pairs[idx - 1].second >= pairs[idx].second) return false;
		}
		return true;
	}

	bool can_apply_turn(const Turn& t) const {
		if (t.moves.empty() || (int)t.moves.size() > R) return false;
		if (!non_crossing(t)) return false;
		for (const auto& mv : t.moves) {
			if (!can_apply_move(mv)) return false;
		}
		return true;
	}

	void apply_move(const Move& mv) {
		if (mv.type == 0) {
			vector<int> block;
			block.reserve(mv.k);
			for (int z = 0; z < mv.k; ++z) {
				block.push_back(dep[mv.i].back());
				dep[mv.i].pop_back();
			}
			for (int x : block) sid[mv.j].push_front(x);
		} else {
			vector<int> block;
			block.reserve(mv.k);
			for (int z = 0; z < mv.k; ++z) {
				block.push_back(sid[mv.j].front());
				sid[mv.j].pop_front();
			}
			for (int x : block) dep[mv.i].push_back(x);
		}
	}

	bool apply_turn_checked(const Turn& t) {
		if (!can_apply_turn(t)) return false;
		for (const auto& mv : t.moves) apply_move(mv);
		return true;
	}
};

bool validate_plan(const Input& in, const Plan& plan) {
	if ((int)plan.size() > TURN_LIMIT) return false;
	State st(in);
	for (const auto& turn : plan) {
		if (!st.apply_turn_checked(turn)) return false;
	}
	return true;
}

class ISolver {
  public:
	virtual ~ISolver() = default;
	virtual Plan solve(const Input& in) = 0;
};

// ブロック構造体
struct Block {
	int head_id;
	int tail_id;
	int len;
};

// 連続した車両をブロックとして取得する共通関数
vector<Block> get_blocks(const deque<int>& q) {
	vector<Block> res;
	if (q.empty()) return res;
	Block cur = {q.front(), q.front(), 1};
	for (int i = 1; i < (int)q.size(); ++i) {
		if (q[i] == cur.tail_id + 1 && q[i] / 10 == cur.tail_id / 10) {
			cur.tail_id = q[i];
			cur.len++;
		} else {
			res.push_back(cur);
			cur = {q[i], q[i], 1};
		}
	}
	res.push_back(cur);
	return res;
}

// ------------------------------
// Fallback: Block Building Solver
// ------------------------------
class BlockBuildingSolver : public ISolver {
	bool can_merge_with_surface(Block B, const array<vector<Block>, R>& dep_blocks, const array<vector<Block>, R>& sid_blocks) {
		for (int r = 0; r < R; ++r) {
			if (!dep_blocks[r].empty()) {
				Block surf = dep_blocks[r].back();
				if (surf.tail_id + 1 == B.head_id || B.tail_id + 1 == surf.head_id) return true;
			}
			if (!sid_blocks[r].empty()) {
				Block surf = sid_blocks[r].front();
				if (surf.tail_id + 1 == B.head_id || B.tail_id + 1 == surf.head_id) return true;
			}
		}
		return false;
	}

	uint64_t compute_hash(const State& st) {
		uint64_t h = 0;
		for (int i = 0; i < R; ++i) {
			for (int x : st.dep[i]) h ^= (h << 5) + (h >> 2) + x;
			h ^= 0x123456789ABCDEFULL;
			for (int x : st.sid[i]) h ^= (h << 5) + (h >> 2) + x;
			h ^= 0xFEDCBA9876543210ULL;
		}
		return h;
	}

  public:
	Plan solve(const Input& in) override {
		State st(in);
		Plan plan;
		mt19937 rng(0xC0FFEE);
		unordered_map<uint64_t, int> visited;
		int stagnant_turns = 0;

		while (!st.is_goal()) {
			if (plan.size() >= TURN_LIMIT) break;

			uint64_t hash = compute_hash(st);
			visited[hash]++;
			if (visited[hash] > 1) stagnant_turns += visited[hash] * 2;
			else stagnant_turns = max(0, stagnant_turns - 1);

			array<vector<Block>, R> dep_blocks, sid_blocks;
			for (int i = 0; i < R; ++i) {
				dep_blocks[i] = get_blocks(st.dep[i]);
				sid_blocks[i] = get_blocks(st.sid[i]);
			}

			vector<pair<Move, double>> candidates;
			for (int i = 0; i < R; ++i) {
				bool is_dep_base_only = (dep_blocks[i].size() == 1 && dep_blocks[i].front().head_id == i * 10);
				bool is_dep_perfect = (is_dep_base_only && dep_blocks[i].front().len == TARGET_LEN);

				for (int j = 0; j < R; ++j) {
					// 出発線 -> 待避線 (複数のブロックをまとめて移動する手も考慮)
					if (!dep_blocks[i].empty() && !is_dep_base_only) {
						int cumulative_k = 0;
						for (int b = (int)dep_blocks[i].size() - 1; b >= 0; --b) {
							cumulative_k += dep_blocks[i][b].len;
							if (b == 0 && dep_blocks[i][0].head_id == i * 10) break;

							Move mv{0, i, j, cumulative_k};
							if (st.can_apply_move(mv)) {
								double score = cumulative_k * 10.0; // まとめて動かすボーナス
								Block X = dep_blocks[i].back();
								Block Y = sid_blocks[j].empty() ? Block{-1, -1, 0} : sid_blocks[j].front();
								
								if (Y.len > 0 && X.tail_id + 1 == Y.head_id) score += 10000;
								else if (Y.len == 0) score += 50;
								else if (X.head_id / 10 == Y.head_id / 10) score -= 100;

								if (b > 0 && can_merge_with_surface(dep_blocks[i][b - 1], dep_blocks, sid_blocks)) score += 200;
								if (Y.len > 0 && can_merge_with_surface(Y, dep_blocks, sid_blocks)) score -= 300;
								if (stagnant_turns > 0) score += (rng() % 200) - 100 + (stagnant_turns > 5 ? (rng() % 10000) - 5000 : 0);
								candidates.push_back({mv, score});
							}
						}
					}
					
					// 待避線 -> 出発線
					if (!sid_blocks[j].empty() && !is_dep_perfect) {
						int cumulative_k = 0;
						for (int b = 0; b < (int)sid_blocks[j].size(); ++b) {
							cumulative_k += sid_blocks[j][b].len;

							Move mv{1, i, j, cumulative_k};
							if (st.can_apply_move(mv)) {
								double score = cumulative_k * 10.0;
								Block X = sid_blocks[j].front();
								Block Y = dep_blocks[i].empty() ? Block{-1, -1, 0} : dep_blocks[i].back();
								
								if (Y.len > 0 && Y.tail_id + 1 == X.head_id) score += 10000;
								else if (X.len == 10 && i == X.head_id / 10 && Y.len == 0) score += 20000;
								else if (Y.len == 0) score += 50;
								else if (X.head_id / 10 == Y.head_id / 10) score -= 100;

								if (b + 1 < sid_blocks[j].size() && can_merge_with_surface(sid_blocks[j][b + 1], dep_blocks, sid_blocks)) score += 200;
								if (Y.len > 0 && can_merge_with_surface(Y, dep_blocks, sid_blocks)) score -= 300;
								if (stagnant_turns > 0) score += (rng() % 200) - 100 + (stagnant_turns > 5 ? (rng() % 10000) - 5000 : 0);
								candidates.push_back({mv, score});
							}
						}
					}
				}
			}

			sort(candidates.begin(), candidates.end(), [](const auto& a, const auto& b){ return a.second > b.second; });
			Turn t;
			array<bool, R> dep_used{}, sid_used{};
			bool moved = false;
			for (auto& cand : candidates) {
				if (cand.second < -1000 && stagnant_turns < 5) continue;
				Move mv = cand.first;
				if (!dep_used[mv.i] && !sid_used[mv.j]) {
					t.moves.push_back(mv);
					if (State::non_crossing(t)) {
						dep_used[mv.i] = true; sid_used[mv.j] = true; moved = true;
					} else t.moves.pop_back();
				}
			}

			if (moved) {
				st.apply_turn_checked(t); plan.push_back(t);
			} else {
				if (!candidates.empty()) {
					Turn rand_t; rand_t.moves.push_back(candidates[rng() % candidates.size()].first);
					st.apply_turn_checked(rand_t); plan.push_back(rand_t);
					stagnant_turns += 3;
				} else break;
			}
		}
		return plan;
	}
};

// ------------------------------
// Main: Beam Search Solver
// ------------------------------
class BeamSearchSolver : public ISolver {
	struct HistoryNode {
		int parent_idx;
		Turn turn;
	};

	struct BeamNode {
		State st;
		double score;
		int history_idx;
	};

	uint64_t compute_hash(const State& st) const {
		uint64_t h = 0;
		for (int i = 0; i < R; ++i) {
			for (int x : st.dep[i]) h ^= (h << 5) + (h >> 2) + x;
			h ^= 0x123456789ABCDEFULL;
			for (int x : st.sid[i]) h ^= (h << 5) + (h >> 2) + x;
			h ^= 0xFEDCBA9876543210ULL;
		}
		return h;
	}

	double evaluate(const State& st, int depth) const {
		if (st.is_goal()) return 1e12 - depth * 10000.0;
		
		double score = -depth * 100.0;

		for (int i = 0; i < R; ++i) {
			auto d_blocks = get_blocks(st.dep[i]);
			int expected_next = i * 10;
			bool is_perfect = true;

			for (const auto& b : d_blocks) {
				double len_sq = b.len * b.len;
				if (is_perfect && b.head_id == expected_next) {
					score += b.len * 100000.0;
					score += len_sq * 5000.0;
					expected_next += b.len;
				} else {
					is_perfect = false;
					if (b.head_id / 10 == i) score += len_sq * 20.0;
					else score += len_sq * 10.0;
				}
			}

			auto s_blocks = get_blocks(st.sid[i]);
			for (const auto& b : s_blocks) {
				double len_sq = b.len * b.len;
				if (b.head_id / 10 == i) score += len_sq * 15.0;
				else score += len_sq * 5.0;
			}

			score -= d_blocks.size() * 10000.0;
			score -= s_blocks.size() * 10000.0;
		}
		return score;
	}

	vector<Turn> generate_candidate_turns(const State& st, int current_depth) const {
		array<vector<Block>, R> dep_blocks, sid_blocks;
		for (int i = 0; i < R; ++i) {
			dep_blocks[i] = get_blocks(st.dep[i]);
			sid_blocks[i] = get_blocks(st.sid[i]);
		}

		vector<Move> valid_moves;
		for (int i = 0; i < R; ++i) {
			bool is_dep_base_only = (dep_blocks[i].size() == 1 && dep_blocks[i].front().head_id == i * 10);
			bool is_dep_perfect = (is_dep_base_only && dep_blocks[i].front().len == TARGET_LEN);

			for (int j = 0; j < R; ++j) {
				// 出発線 -> 待避線 (ブロックの境界単位で複数ブロックをまとめて移動)
				if (!dep_blocks[i].empty() && !is_dep_base_only) {
					int cumulative_k = 0;
					for (int b = (int)dep_blocks[i].size() - 1; b >= 0; --b) {
						cumulative_k += dep_blocks[i][b].len;
						// ベースブロックまで巻き込んで移動するのは禁止
						if (b == 0 && dep_blocks[i][0].head_id == i * 10) break;
						
						Move mv{0, i, j, cumulative_k};
						if (st.can_apply_move(mv)) valid_moves.push_back(mv);
					}
				}
				
				// 待避線 -> 出発線 (ブロックの境界単位で複数ブロックをまとめて移動)
				if (!sid_blocks[j].empty() && !is_dep_perfect) {
					int cumulative_k = 0;
					for (int b = 0; b < (int)sid_blocks[j].size(); ++b) {
						cumulative_k += sid_blocks[j][b].len;
						
						Move mv{1, i, j, cumulative_k};
						if (st.can_apply_move(mv)) valid_moves.push_back(mv);
					}
				}
			}
		}

		vector<pair<double, Move>> eval_moves;
		for (const auto& mv : valid_moves) {
			State nxt = st;
			nxt.apply_move(mv);
			double sc = evaluate(nxt, current_depth + 1);
			eval_moves.push_back({sc, mv});
		}
		sort(eval_moves.begin(), eval_moves.end(), [](const auto& a, const auto& b) {
			return a.first > b.first;
		});

		vector<Turn> turns;
		for (int i = 0; i < min(15, (int)eval_moves.size()); ++i) {
			Turn t;
			array<bool, R> dep_used{}, sid_used{};
			t.moves.push_back(eval_moves[i].second);
			dep_used[eval_moves[i].second.i] = true;
			sid_used[eval_moves[i].second.j] = true;

			for (int j = 0; j < eval_moves.size(); ++j) {
				if (i == j) continue;
				Move mv = eval_moves[j].second;
				if (!dep_used[mv.i] && !sid_used[mv.j]) {
					t.moves.push_back(mv);
					if (State::non_crossing(t)) {
						dep_used[mv.i] = true;
						sid_used[mv.j] = true;
					} else {
						t.moves.pop_back();
					}
				}
			}
			turns.push_back(t);
		}

		for (int i = 0; i < min(5, (int)eval_moves.size()); ++i) {
			Turn t;
			t.moves.push_back(eval_moves[i].second);
			turns.push_back(t);
		}

		return turns;
	}

  public:
	Plan solve(const Input& in) override {
		auto start_time = chrono::steady_clock::now();
		constexpr double TIME_LIMIT = 1.5; 
		constexpr int BEAM_WIDTH = 30;     

		vector<vector<HistoryNode>> history;
		vector<BeamNode> current_beam;
		unordered_set<uint64_t> global_visited;

		State initial_state(in);
		current_beam.push_back({initial_state, evaluate(initial_state, 0), 0});
		history.push_back({{ -1, Turn() }});
		global_visited.insert(compute_hash(initial_state));

		int current_depth = 0;
		int best_goal_depth = -1;
		int best_goal_history_idx = -1;

		while (current_depth < TURN_LIMIT) {
			auto now = chrono::steady_clock::now();
			if (chrono::duration<double>(now - start_time).count() > TIME_LIMIT) {
				break; 
			}

			if (current_beam.empty()) break;

			vector<HistoryNode> next_history;
			vector<BeamNode> next_beam_cands;

			for (int i = 0; i < current_beam.size(); ++i) {
				auto& node = current_beam[i];

				if (node.st.is_goal()) {
					best_goal_depth = current_depth;
					best_goal_history_idx = node.history_idx;
					break;
				}

				auto turns = generate_candidate_turns(node.st, current_depth);
				for (const auto& t : turns) {
					State nxt_st = node.st;
					nxt_st.apply_turn_checked(t);
					uint64_t h = compute_hash(nxt_st);
					
					if (global_visited.count(h)) continue;
					global_visited.insert(h);

					double sc = evaluate(nxt_st, current_depth + 1);
					int next_h_idx = next_history.size();
					next_history.push_back({node.history_idx, t});
					next_beam_cands.push_back({nxt_st, sc, next_h_idx});
				}
			}

			if (best_goal_depth != -1) break;

			sort(next_beam_cands.begin(), next_beam_cands.end(), [](const BeamNode& a, const BeamNode& b) {
				return a.score > b.score;
			});

			vector<BeamNode> next_beam;
			for (int i = 0; i < min(BEAM_WIDTH, (int)next_beam_cands.size()); ++i) {
				next_beam.push_back(next_beam_cands[i]);
			}

			history.push_back(std::move(next_history));
			current_beam = std::move(next_beam);
			current_depth++;
		}

		if (best_goal_depth != -1) {
			Plan plan;
			int d = best_goal_depth;
			int h_idx = best_goal_history_idx;
			while (d > 0) {
				plan.push_back(history[d][h_idx].turn);
				h_idx = history[d][h_idx].parent_idx;
				d--;
			}
			reverse(plan.begin(), plan.end());
			return plan;
		}

		cerr << "Beam search did not reach goal. Falling back to BlockBuildingSolver." << endl;
		BlockBuildingSolver fallback;
		return fallback.solve(in);
	}
};

// ------------------------------
// Output
// ------------------------------
void print_plan(const Plan& plan) {
	cout << plan.size() << '\n';
	for (const auto& turn : plan) {
		cout << turn.moves.size() << '\n';
		for (const auto& mv : turn.moves) {
			cout << mv.type << ' ' << mv.i << ' ' << mv.j << ' ' << mv.k << '\n';
		}
	}
}

}  // namespace ahc064

int main() {
	ios::sync_with_stdio(false);
	cin.tie(nullptr);

	using namespace ahc064;

	const Input in = read_input();

	unique_ptr<ISolver> solver = make_unique<BeamSearchSolver>();
	Plan plan = solver->solve(in);

	if (!validate_plan(in, plan)) {
		plan.clear();
	}

	print_plan(plan);
	return 0;
}