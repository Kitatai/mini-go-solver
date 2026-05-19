#pragma once

#include "bitboard.hpp"
#include "memo_table.hpp"
#include "pattern_evaluator.hpp"
#include "ranker.hpp"

#include <algorithm>
#include <bit>
#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

class Solver {
public:
    explicit Solver(int n, bool use_symmetry, bool use_sparse, bool use_learning, bool batch_learning, std::uint32_t learn_sample, std::uint64_t sparse_initial_capacity)
        : n_(n),
          use_symmetry_(use_symmetry),
          use_sparse_(use_sparse),
          evaluator_(use_learning, batch_learning, learn_sample),
          mask_(minigo::board_mask<Board>(n)),
          ranker_(n),
          canonical_ranker_(n),
          states_(checked_u64(use_symmetry_ ? canonical_ranker_.total_states() : ranker_.total_states())),
          dense_memo_(use_sparse ? 0 : states_),
          sparse_memo_(use_sparse ? initial_sparse_capacity(states_, sparse_initial_capacity) : 0,
                       use_sparse ? sparse_initial_capacity : 0) {}

    bool first_wins_after(int first) {
        Board black = Board{1} << first;
        bool second_wins = solve(black, 0, false);
        return !second_wins;
    }

    std::string row() {
        std::vector<char> values(n_);
        int first_count = (n_ + 1) / 2;
        for (int i = 0; i < first_count; ++i) {
            char value = first_wins_after(i) ? 'W' : 'L';
            values[i] = value;
            values[n_ - 1 - i] = value;
        }

        std::string result;
        for (int i = 0; i < n_; ++i) {
            if (i > 0) result += ' ';
            result += values[i];
        }
        return result;
    }

    std::uint64_t states() const { return states_; }
    std::size_t memo_bytes() const { return use_sparse_ ? sparse_memo_.bytes() : dense_memo_.bytes(); }
    std::uint64_t filled_memo_entries() const { return nodes_ - hits_; }
    std::uint64_t nodes() const { return nodes_; }
    std::uint64_t hits() const { return hits_; }
    std::uint64_t learning_updates() const { return evaluator_.updates(); }

    void print_sparse_stats() const {
        if (!use_sparse_) return;
        double get_avg = sparse_memo_.get_calls() == 0
            ? 0.0
            : static_cast<double>(sparse_memo_.get_probes()) / static_cast<double>(sparse_memo_.get_calls());
        double set_avg = sparse_memo_.set_calls() == 0
            ? 0.0
            : static_cast<double>(sparse_memo_.set_probes()) / static_cast<double>(sparse_memo_.set_calls());
        double get_collision_rate = sparse_memo_.get_calls() == 0
            ? 0.0
            : static_cast<double>(sparse_memo_.get_collisions()) / static_cast<double>(sparse_memo_.get_calls());
        double set_collision_rate = sparse_memo_.set_calls() == 0
            ? 0.0
            : static_cast<double>(sparse_memo_.set_collisions()) / static_cast<double>(sparse_memo_.set_calls());
        std::cout << "sparse_get_calls=" << sparse_memo_.get_calls()
                  << " sparse_get_avg_probe=" << get_avg
                  << " sparse_get_collision_rate=" << get_collision_rate
                  << " sparse_get_max_probe=" << sparse_memo_.max_get_probe()
                  << '\n';
        std::cout << "sparse_set_calls=" << sparse_memo_.set_calls()
                  << " sparse_set_avg_probe=" << set_avg
                  << " sparse_set_collision_rate=" << set_collision_rate
                  << " sparse_set_max_probe=" << sparse_memo_.max_set_probe()
                  << '\n';
    }

    bool load_weights(const std::string& path) {
        return evaluator_.load(path);
    }

    bool save_weights(const std::string& path) const {
        return evaluator_.save(path);
    }

    void finish_learning_batch() {
        evaluator_.apply_pending_updates();
    }

private:
    struct OrderedMove {
        Board move;
        int opp_legal_count;
        bool gives_opp_win;
        int pattern_order_score;
    };

    static bool better_move(const OrderedMove& a, const OrderedMove& b) {
        if (a.gives_opp_win != b.gives_opp_win) return !a.gives_opp_win;
        if (a.pattern_order_score != b.pattern_order_score) return a.pattern_order_score > b.pattern_order_score;
        if (a.opp_legal_count != b.opp_legal_count) return a.opp_legal_count < b.opp_legal_count;
        return a.move < b.move;
    }

    static void insertion_sort_moves(OrderedMove* moves, int count) {
        for (int i = 1; i < count; ++i) {
            OrderedMove value = moves[i];
            int j = i;
            while (j > 0 && better_move(value, moves[j - 1])) {
                moves[j] = moves[j - 1];
                --j;
            }
            moves[j] = value;
        }
    }

    static std::uint64_t checked_u64(Count value) {
        if (value > std::numeric_limits<std::uint64_t>::max()) {
            throw std::overflow_error("state count does not fit in uint64_t");
        }
        return static_cast<std::uint64_t>(value);
    }

    static std::uint64_t initial_sparse_capacity(std::uint64_t states, std::uint64_t requested_capacity) {
        if (requested_capacity != 0) return requested_capacity;
        constexpr std::uint64_t default_expected = 1ULL << 26;
        return std::min(states, default_expected);
    }

    bool solve(Board black, Board white, bool black_turn) {
        ++nodes_;

        Board me = black_turn ? black : white;
        Board opp = black_turn ? white : black;

        Board memo_black = black;
        Board memo_white = white;
        std::uint64_t memo_key = 0;
        std::uint64_t r = 0;
        std::uint8_t known = 0;
        if (use_sparse_) {
            canonical_board(memo_black, memo_white);
            memo_key = minigo::SparseMemo::make_key(memo_black, memo_white);
            known = sparse_memo_.get_key(memo_key);
        } else {
            r = memo_rank(black, white);
            known = dense_memo_.get(r);
        }
        if (known != 0) {
            ++hits_;
            return known == 2;
        }

        Board empty = mask_ & ~(black | white);
        Board captures = minigo::capture_candidates(opp, empty, mask_);
        Board legal = minigo::non_capture_legal_moves_from_empty(me, empty, mask_) | captures;

        bool win = false;
        if (legal != 0) {
            if (captures != 0) {
                win = true;
            } else {
                OrderedMove ordered[32];
                int ordered_count = 0;
                int base_opp_view_score = evaluator_.evaluate_relative(opp, me, n_);

                Board moves = legal;
                while (moves) {
                    Board move = moves & -moves;
                    moves &= moves - 1;
                    int move_pos = std::countr_zero(move);

                    Board next_me = me | move;
                    Board next_empty = mask_ & ~(next_me | opp);
                    Board opp_wins = minigo::capture_candidates(next_me, next_empty, mask_);
                    Board opp_legal = minigo::non_capture_legal_moves_from_empty(opp, next_empty, mask_) | opp_wins;
                    int child_opp_view_score =
                        base_opp_view_score + evaluator_.delta_add_opp_stone(opp, me, n_, move_pos);
                    int pattern_order_score = -child_opp_view_score;

                    ordered[ordered_count++] = {
                        move,
                        std::popcount(opp_legal),
                        opp_wins != 0,
                        pattern_order_score,
                    };
                }

                insertion_sort_moves(ordered, ordered_count);

                for (int i = 0; i < ordered_count; ++i) {
                    Board move = ordered[i].move;
                    bool child_win = black_turn ? solve(black | move, white, false) : solve(black, white | move, true);
                    if (!child_win) {
                        win = true;
                        break;
                    }
                }
            }
        }

        if (use_sparse_) {
            sparse_memo_.set_key(memo_key, win ? 2 : 1);
        } else {
            dense_memo_.set(r, win ? 2 : 1);
        }
        evaluator_.update(black, white, n_, black_turn, win);
        return win;
    }

    void canonical_board(Board& black, Board& white) const {
        if (!use_symmetry_) return;
        Board reversed_black = minigo::reverse_board(black, n_);
        Board reversed_white = minigo::reverse_board(white, n_);
        if (!lex_leq(black, white, reversed_black, reversed_white)) {
            black = reversed_black;
            white = reversed_white;
        }
    }

    std::uint64_t memo_rank(Board black, Board white) const {
        if (!use_symmetry_) return ranker_.rank(black, white);
        Board reversed_black = minigo::reverse_board(black, n_);
        Board reversed_white = minigo::reverse_board(white, n_);
        if (lex_leq(black, white, reversed_black, reversed_white)) {
            return canonical_ranker_.rank(black, white);
        }
        return canonical_ranker_.rank(reversed_black, reversed_white);
    }

    static bool lex_leq(Board black_a, Board white_a, Board black_b, Board white_b) {
        Board diff = (black_a ^ black_b) | (white_a ^ white_b);
        if (diff == 0) return true;
        Board bit = Board{1} << std::countr_zero(diff);
        int a = (black_a & bit) ? 1 : (white_a & bit) ? 2 : 0;
        int b = (black_b & bit) ? 1 : (white_b & bit) ? 2 : 0;
        return a < b;
    }

    int n_;
    bool use_symmetry_;
    bool use_sparse_;
    PatternEvaluator evaluator_;
    Board mask_;
    Ranker ranker_;
    mutable CanonicalRanker canonical_ranker_;
    std::uint64_t states_;
    minigo::PackedMemo dense_memo_;
    minigo::SparseMemo sparse_memo_;
    std::uint64_t nodes_ = 0;
    std::uint64_t hits_ = 0;
};
