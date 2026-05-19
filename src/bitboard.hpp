#pragma once

#include <bit>
#include <cstdint>
#include <limits>

namespace minigo {

using Board32 = std::uint32_t;
using Board64 = std::uint64_t;

template <class Board>
inline Board board_mask(int n) {
    constexpr int bits = std::numeric_limits<Board>::digits;
    return n == bits ? ~Board{0} : ((Board{1} << n) - 1);
}

inline Board32 reverse_bits32(Board32 x) {
    x = ((x & 0x55555555u) << 1) | ((x >> 1) & 0x55555555u);
    x = ((x & 0x33333333u) << 2) | ((x >> 2) & 0x33333333u);
    x = ((x & 0x0f0f0f0fu) << 4) | ((x >> 4) & 0x0f0f0f0fu);
    x = ((x & 0x00ff00ffu) << 8) | ((x >> 8) & 0x00ff00ffu);
    x = (x << 16) | (x >> 16);
    return x;
}

inline Board64 reverse_bits64(Board64 x) {
    x = ((x & 0x5555555555555555ULL) << 1) | ((x >> 1) & 0x5555555555555555ULL);
    x = ((x & 0x3333333333333333ULL) << 2) | ((x >> 2) & 0x3333333333333333ULL);
    x = ((x & 0x0f0f0f0f0f0f0f0fULL) << 4) | ((x >> 4) & 0x0f0f0f0f0f0f0f0fULL);
    x = ((x & 0x00ff00ff00ff00ffULL) << 8) | ((x >> 8) & 0x00ff00ff00ff00ffULL);
    x = ((x & 0x0000ffff0000ffffULL) << 16) | ((x >> 16) & 0x0000ffff0000ffffULL);
    x = (x << 32) | (x >> 32);
    return x;
}

inline Board32 reverse_board(Board32 x, int n) {
    return reverse_bits32(x) >> (32 - n);
}

inline Board64 reverse_board(Board64 x, int n) {
    return reverse_bits64(x) >> (64 - n);
}

template <class Board>
inline Board flood_right(Board seed, Board stones, Board mask) {
    Board x = seed;
    Board s = stones;
    x |= (x << 1) & s & mask;
    s &= (s << 1) & mask;
    x |= (x << 2) & s & mask;
    s &= (s << 2) & mask;
    x |= (x << 4) & s & mask;
    s &= (s << 4) & mask;
    x |= (x << 8) & s & mask;
    s &= (s << 8) & mask;
    x |= (x << 16) & s & mask;
    if constexpr (std::numeric_limits<Board>::digits > 32) {
        s &= (s << 16) & mask;
        x |= (x << 32) & s & mask;
    }
    return x;
}

template <class Board>
inline Board flood_left(Board seed, Board stones) {
    Board x = seed;
    Board s = stones;
    x |= (x >> 1) & s;
    s &= s >> 1;
    x |= (x >> 2) & s;
    s &= s >> 2;
    x |= (x >> 4) & s;
    s &= s >> 4;
    x |= (x >> 8) & s;
    s &= s >> 8;
    x |= (x >> 16) & s;
    if constexpr (std::numeric_limits<Board>::digits > 32) {
        s &= s >> 16;
        x |= (x >> 32) & s;
    }
    return x;
}

template <class Board>
inline Board two_liberty_points(Board stones, Board empty, Board mask) {
    Board left_lib = empty & (stones >> 1);
    Board right_lib = empty & ((stones << 1) & mask);

    Board from_left = flood_right(left_lib, stones, mask);
    Board paired_right = (from_left << 1) & right_lib & mask;

    Board from_right = flood_left(right_lib, stones);
    Board paired_left = (from_right >> 1) & left_lib;

    return paired_left | paired_right;
}

template <class Board>
inline Board capture_candidates(Board opp, Board empty, Board mask) {
    Board left_lib = empty & (opp >> 1);
    Board right_lib = empty & ((opp << 1) & mask);

    Board from_left = flood_right(left_lib, opp, mask);
    Board paired_right = (from_left << 1) & right_lib & mask;

    Board from_right = flood_left(right_lib, opp);
    Board paired_left = (from_right >> 1) & left_lib;

    Board one_lib_left = left_lib & ~paired_left;
    Board one_lib_right = right_lib & ~paired_right;
    return one_lib_left | one_lib_right;
}

template <class Board>
inline Board non_capture_legal_moves_from_empty(Board me, Board empty, Board mask) {
    Board has_empty_neighbor = empty & (((empty << 1) & mask) | (empty >> 1));
    Board safe = two_liberty_points(me, empty, mask);
    return empty & (has_empty_neighbor | safe);
}

template <class Board>
inline Board legal_moves_from_empty(Board me, Board opp, Board empty, Board mask) {
    return non_capture_legal_moves_from_empty(me, empty, mask)
         | capture_candidates(opp, empty, mask);
}

template <class Board>
inline Board legal_moves(Board me, Board opp, Board mask) {
    Board empty = mask & ~(me | opp);
    return legal_moves_from_empty(me, opp, empty, mask);
}

} // namespace minigo
