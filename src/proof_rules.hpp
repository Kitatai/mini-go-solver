#pragma once

#include "bitboard.hpp"

#include <algorithm>
#include <cstdint>

namespace minigo::proof {

enum Boundary : std::uint8_t {
    WALL = 0,
    ME = 1,
    OPP = 2,
};

struct Gap {
    std::uint8_t m;
    std::uint8_t left;
    std::uint8_t right;

    friend bool operator==(Gap a, Gap b) {
        return a.m == b.m && a.left == b.left && a.right == b.right;
    }

    friend bool operator<(Gap a, Gap b) {
        if (a.m != b.m) return a.m < b.m;
        if (a.left != b.left) return a.left < b.left;
        return a.right < b.right;
    }
};

struct GapList {
    Gap gaps[64];
    int size = 0;
};

enum class Result : std::uint8_t {
    Unknown,
    Win,
    Loss,
};

inline Gap canonical_gap(std::uint8_t m, std::uint8_t left, std::uint8_t right) {
    if (right < left) std::swap(left, right);
    return Gap{m, left, right};
}

inline void normalize(GapList& list) {
    std::sort(list.gaps, list.gaps + list.size);
}

template <class Board>
GapList gap_list(Board me, Board opp, int n) {
    GapList list;
    int pos = 0;
    std::uint8_t left = WALL;
    while (pos < n) {
        Board bit = Board{1} << pos;
        if ((me | opp) & bit) {
            left = (me & bit) ? ME : OPP;
            ++pos;
            continue;
        }

        int start = pos;
        while (pos < n && (((me | opp) & (Board{1} << pos)) == 0)) ++pos;
        std::uint8_t right = WALL;
        if (pos < n) {
            Board right_bit = Board{1} << pos;
            right = (me & right_bit) ? ME : OPP;
        }
        list.gaps[list.size++] = canonical_gap(
            static_cast<std::uint8_t>(pos - start),
            left,
            right);
    }
    normalize(list);
    return list;
}

inline bool remove_once(GapList& list, Gap gap) {
    gap = canonical_gap(gap.m, gap.left, gap.right);
    for (int i = 0; i < list.size; ++i) {
        if (list.gaps[i] == gap) {
            for (int j = i + 1; j < list.size; ++j) list.gaps[j - 1] = list.gaps[j];
            --list.size;
            return true;
        }
    }
    return false;
}

inline bool remove_pair(GapList& list, Gap a, Gap b) {
    GapList copy = list;
    if (!remove_once(copy, a)) return false;
    if (!remove_once(copy, b)) return false;
    list = copy;
    return true;
}

inline bool has_current_legal_move(Gap gap) {
    for (int pos = 0; pos < gap.m; ++pos) {
        if (pos == 0 && (gap.left == WALL || gap.left == OPP)) continue;
        if (pos == gap.m - 1 && (gap.right == WALL || gap.right == OPP)) continue;
        return true;
    }
    return false;
}

inline bool remove_inert_component(GapList& list) {
    for (int i = 0; i < list.size; ++i) {
        if (!has_current_legal_move(list.gaps[i])) {
            for (int j = i + 1; j < list.size; ++j) list.gaps[j - 1] = list.gaps[j];
            --list.size;
            return true;
        }
    }
    return false;
}

inline bool remove_t_component(GapList& list) {
    if (remove_once(list, Gap{1, WALL, ME})) return true; // T0

    for (int n = 1; n <= 64; ++n) {
        std::uint8_t k = static_cast<std::uint8_t>(n);
        if (remove_pair(list, Gap{k, ME, OPP}, Gap{k, ME, OPP})) return true; // T1
        if (remove_pair(list, Gap{k, OPP, OPP}, Gap{k, ME, ME})) return true; // T2
        if (remove_pair(list, Gap{k, OPP, OPP}, Gap{k, WALL, ME})) return true; // T3
        if (n < 64 && remove_pair(list, Gap{k, OPP, OPP}, Gap{static_cast<std::uint8_t>(n + 1), WALL, ME})) return true; // T4
        if (remove_pair(list, Gap{k, WALL, OPP}, Gap{k, ME, OPP})) return true; // T5
        if (n < 64 && remove_pair(list, Gap{static_cast<std::uint8_t>(n + 1), WALL, OPP}, Gap{k, ME, OPP})) return true; // T6

        GapList copy = list;
        if (remove_once(copy, Gap{1, WALL, ME})
            && remove_pair(copy, Gap{k, ME, OPP}, Gap{k, ME, OPP})) {
            list = copy; // T7
            return true;
        }

        copy = list;
        if (remove_once(copy, Gap{1, WALL, ME})
            && remove_pair(copy, Gap{k, OPP, OPP}, Gap{k, ME, ME})) {
            list = copy; // T8
            return true;
        }
    }
    return false;
}

inline bool remove_a_component(GapList& list) {
    for (int n = 1; n <= 64; ++n) {
        std::uint8_t k = static_cast<std::uint8_t>(n);
        if (remove_pair(list, Gap{k, WALL, OPP}, Gap{k, WALL, OPP})) return true; // A(n,n)
        if (n < 64 && remove_pair(list, Gap{k, WALL, OPP}, Gap{static_cast<std::uint8_t>(n + 1), WALL, OPP})) return true; // A(n,n+1)
    }
    return false;
}

inline bool remove_r_component(GapList& list) {
    for (int n = 1; n <= 64; ++n) {
        GapList copy = list;
        std::uint8_t k = static_cast<std::uint8_t>(n);
        if (remove_once(copy, Gap{1, ME, OPP})
            && remove_once(copy, Gap{k, WALL, ME})
            && remove_once(copy, Gap{k, WALL, OPP})) {
            list = copy;
            return true;
        }
    }
    return false;
}

inline bool is_proven_loss_sum(GapList list) {
    while (list.size > 0) {
        if (remove_inert_component(list)) continue;
        if (remove_t_component(list)) continue;
        if (remove_a_component(list)) continue;
        if (remove_r_component(list)) continue;
        return false;
    }
    return true;
}

inline bool is_exact_a_win(GapList list) {
    if (list.size != 2) return false;
    Gap a = list.gaps[0];
    Gap b = list.gaps[1];
    if (a.left != WALL || a.right != OPP || b.left != WALL || b.right != OPP) return false;
    if (a.m > b.m) std::swap(a, b);
    if (a.m == 1 && b.m >= 3) return true;
    return b.m == static_cast<std::uint8_t>(a.m + 2);
}

template <class Board>
Result proven_result(Board me, Board opp, int n) {
    GapList gaps = gap_list(me, opp, n);
    if (is_exact_a_win(gaps)) return Result::Win;
    if (is_proven_loss_sum(gaps)) return Result::Loss;
    return Result::Unknown;
}

} // namespace minigo::proof
