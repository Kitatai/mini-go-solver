#include "bitboard.hpp"

#include <cstdlib>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

using U64 = minigo::Board64;
using minigo::capture_candidates;
using minigo::legal_moves;

static int negamax(U64 me, U64 opp, U64 mask, int alpha, int beta, std::uint64_t& nodes) {
    ++nodes;

    U64 empty = mask & ~(me | opp);
    U64 legal = legal_moves(me, opp, mask);
    if (legal == 0) return -1;

    U64 wins = capture_candidates(opp, empty, mask);
    if (wins != 0) return 1;

    U64 moves = legal;
    while (moves) {
        U64 move = moves & -moves;
        moves &= moves - 1;

        int score = -negamax(opp, me | move, mask, -beta, -alpha, nodes);
        if (score > alpha) alpha = score;
        if (alpha >= beta) break;
    }

    return alpha;
}

static int negamax_pv(U64 me, U64 opp, U64 mask, int alpha, int beta, std::vector<int>& pv) {
    U64 empty = mask & ~(me | opp);
    U64 legal = legal_moves(me, opp, mask);
    if (legal == 0) return -1;

    U64 wins = capture_candidates(opp, empty, mask);
    if (wins != 0) {
        pv = {std::countr_zero(wins)};
        return 1;
    }

    std::vector<int> best_pv;
    U64 moves = legal;
    while (moves) {
        U64 move = moves & -moves;
        moves &= moves - 1;

        std::vector<int> child_pv;
        int score = -negamax_pv(opp, me | move, mask, -beta, -alpha, child_pv);
        if (score > alpha || best_pv.empty()) {
            alpha = score;
            best_pv.clear();
            best_pv.push_back(std::countr_zero(move));
            best_pv.insert(best_pv.end(), child_pv.begin(), child_pv.end());
        }
        if (alpha >= beta) break;
    }

    pv = best_pv;
    return alpha;
}

static std::string result_row(int n) {
    U64 mask = minigo::board_mask<U64>(n);
    std::string row;

    for (int i = 0; i < n; ++i) {
        U64 black = 1ULL << i;
        U64 white = 0;
        std::uint64_t nodes = 0;

        int second_score = negamax(white, black, mask, -1, 1, nodes);
        bool first_can_win = second_score < 0;

        if (i > 0) row += ' ';
        row += first_can_win ? 'W' : 'L';
    }

    return row;
}

int main(int argc, char** argv) {
    if (argc == 2) {
        int n = std::atoi(argv[1]);
        if (n < 1 || n > 64) {
            std::cerr << "N must be in 1..64\n";
            return 1;
        }
        std::cout << "N=" << n << ": " << result_row(n) << '\n';
        return 0;
    }

    if (argc == 3) {
        int n = std::atoi(argv[1]);
        int first = std::atoi(argv[2]);
        U64 mask = minigo::board_mask<U64>(n);
        U64 black = 1ULL << first;
        std::vector<int> pv;
        int second_score = negamax_pv(0, black, mask, -1, 1, pv);

        std::cout << "N=" << n << ", first=" << first
                  << ", result=" << (second_score < 0 ? "W" : "L") << '\n';
        std::cout << "line:";
        for (int p : pv) std::cout << ' ' << p;
        std::cout << '\n';
        return 0;
    }

    std::cout << "N, initial moves from left index 0..N-1, W=first player wins, L=first player loses\n";

    for (int n = 2; n <= 10; ++n) {
        std::cout << "N=" << n << ": " << result_row(n) << '\n';
    }

    return 0;
}
