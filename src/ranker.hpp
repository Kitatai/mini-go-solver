#pragma once

#include "bitboard.hpp"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

using Board = minigo::Board32;
using Count = unsigned __int128;

enum DfaState {
    START = 0,
    EMPTY = 1,
    BLACK_NO_LIB = 2,
    BLACK_HAS_LIB = 3,
    WHITE_NO_LIB = 4,
    WHITE_HAS_LIB = 5,
    DFA_STATES = 6,
};

inline bool accept_state(int state) {
    return state == EMPTY || state == BLACK_HAS_LIB || state == WHITE_HAS_LIB;
}

inline bool dfa_transition(int state, int symbol, int& next_state, int& diff_delta) {
    diff_delta = symbol == 1 ? 1 : symbol == 2 ? -1 : 0;

    switch (state) {
    case START:
        next_state = symbol == 0 ? EMPTY : symbol == 1 ? BLACK_NO_LIB : WHITE_NO_LIB;
        return true;
    case EMPTY:
        next_state = symbol == 0 ? EMPTY : symbol == 1 ? BLACK_HAS_LIB : WHITE_HAS_LIB;
        return true;
    case BLACK_NO_LIB:
        if (symbol == 2) return false;
        next_state = symbol == 0 ? EMPTY : BLACK_NO_LIB;
        return true;
    case BLACK_HAS_LIB:
        next_state = symbol == 0 ? EMPTY : symbol == 1 ? BLACK_HAS_LIB : WHITE_NO_LIB;
        return true;
    case WHITE_NO_LIB:
        if (symbol == 1) return false;
        next_state = symbol == 0 ? EMPTY : WHITE_NO_LIB;
        return true;
    case WHITE_HAS_LIB:
        next_state = symbol == 0 ? EMPTY : symbol == 1 ? BLACK_NO_LIB : WHITE_HAS_LIB;
        return true;
    default:
        return false;
    }
}

inline std::string decimal(Count value) {
    if (value == 0) return "0";
    std::string s;
    while (value > 0) {
        int digit = static_cast<int>(value % 10);
        s.push_back(static_cast<char>('0' + digit));
        value /= 10;
    }
    std::reverse(s.begin(), s.end());
    return s;
}

class Ranker {
public:
    explicit Ranker(int n) : n_(n), diff_width_(2 * n + 1), offset_(n) {
        init_transitions();
        suffix_.assign((n_ + 1) * DFA_STATES * diff_width_, 0);

        for (int state = 0; state < DFA_STATES; ++state) {
            for (int diff = -n_; diff <= n_; ++diff) {
                if (accept_state(state) && (diff == 0 || diff == 1)) {
                    at(n_, state, diff) = 1;
                }
            }
        }

        for (int pos = n_ - 1; pos >= 0; --pos) {
            for (int state = 0; state < DFA_STATES; ++state) {
                for (int diff = -n_; diff <= n_; ++diff) {
                    Count total = 0;
                    for (int symbol = 0; symbol < 3; ++symbol) {
                        if (!valid_[state][symbol]) continue;
                        int next_diff = diff + delta_[symbol];
                        if (next_diff < -n_ || next_diff > n_) continue;
                        total += get(pos + 1, next_[state][symbol], next_diff);
                    }
                    at(pos, state, diff) = total;
                }
            }
        }
    }

    Count total_states() const {
        return get(0, START, 0);
    }

    std::uint64_t rank(Board black, Board white) const {
        Count rank = 0;
        int state = START;
        int diff = 0;

        for (int pos = 0; pos < n_; ++pos) {
            Board bit = Board{1} << pos;
            int actual = (black & bit) ? 1 : (white & bit) ? 2 : 0;

            rank += prefix_count(pos, state, diff, actual);

            if (!valid_[state][actual]) {
                throw std::logic_error("attempted to rank an invalid board");
            }
            state = next_[state][actual];
            diff += delta_[actual];
        }

        if (!accept_state(state) || !(diff == 0 || diff == 1)) {
            throw std::logic_error("attempted to rank an invalid final board");
        }
        if (rank > std::numeric_limits<std::uint64_t>::max()) {
            throw std::overflow_error("rank does not fit in uint64_t");
        }
        return static_cast<std::uint64_t>(rank);
    }

private:
    void init_transitions() {
        for (int state = 0; state < DFA_STATES; ++state) {
            for (int symbol = 0; symbol < 3; ++symbol) {
                int next_state = 0;
                int delta = 0;
                valid_[state][symbol] = dfa_transition(state, symbol, next_state, delta);
                next_[state][symbol] = next_state;
            }
        }
    }

    Count prefix_count(int pos, int state, int diff, int actual) const {
        Count total = 0;
        for (int symbol = 0; symbol < actual; ++symbol) {
            if (!valid_[state][symbol]) continue;
            int next_diff = diff + delta_[symbol];
            if (next_diff < -n_ || next_diff > n_) continue;
            total += get(pos + 1, next_[state][symbol], next_diff);
        }
        return total;
    }

    Count& at(int pos, int state, int diff) {
        return suffix_[index(pos, state, diff)];
    }

    Count get(int pos, int state, int diff) const {
        return suffix_[index(pos, state, diff)];
    }

    std::size_t index(int pos, int state, int diff) const {
        return (static_cast<std::size_t>(pos) * DFA_STATES + state) * diff_width_
             + static_cast<std::size_t>(diff + offset_);
    }

    int n_;
    int diff_width_;
    int offset_;
    int next_[DFA_STATES][3] = {};
    bool valid_[DFA_STATES][3] = {};
    int delta_[3] = {0, 1, -1};
    std::vector<Count> suffix_;
};

struct Transform {
    int8_t next[DFA_STATES];
    int8_t delta;
};

inline Transform identity_transform() {
    Transform t{};
    for (int i = 0; i < DFA_STATES; ++i) t.next[i] = static_cast<int8_t>(i);
    t.delta = 0;
    return t;
}

inline std::uint64_t pack_transform(const Transform& t, int n) {
    std::uint64_t key = static_cast<std::uint64_t>(t.delta + n);
    for (int i = 0; i < DFA_STATES; ++i) {
        std::uint64_t v = t.next[i] < 0 ? 7ULL : static_cast<std::uint64_t>(t.next[i]);
        key = (key << 3) | v;
    }
    return key;
}

inline Transform prepend_symbol(const Transform& suffix, int symbol) {
    Transform out{};
    out.delta = static_cast<int8_t>(suffix.delta + (symbol == 1 ? 1 : symbol == 2 ? -1 : 0));
    for (int state = 0; state < DFA_STATES; ++state) {
        int next_state = 0;
        int delta = 0;
        if (!dfa_transition(state, symbol, next_state, delta) || suffix.next[next_state] < 0) {
            out.next[state] = -1;
        } else {
            out.next[state] = suffix.next[next_state];
        }
    }
    return out;
}

class CanonicalRanker {
public:
    explicit CanonicalRanker(int n) : n_(n), identity_(identity_transform()) {}

    Count total_states() {
        return count(0, START, 0, identity_, false);
    }

    std::uint64_t rank(Board black, Board white) {
        Count r = rank_count(black, white);
        if (r > std::numeric_limits<std::uint64_t>::max()) {
            throw std::overflow_error("rank does not fit in uint64_t");
        }
        return static_cast<std::uint64_t>(r);
    }

private:
    Count rank_count(Board black, Board white) {
        Count rank = 0;
        int state = START;
        int diff = 0;
        Transform suffix = identity_;
        bool less = false;

        for (int k = 0; k < (n_ + 1) / 2; ++k) {
            int l = k;
            int r = n_ - 1 - k;
            int actual_left = symbol_at(black, white, l);

            if (l == r) {
                for (int symbol = 0; symbol < actual_left; ++symbol) {
                    int next_state = 0;
                    int delta = 0;
                    if (!dfa_transition(state, symbol, next_state, delta)) continue;
                    rank += count(k + 1, next_state, diff + delta, suffix, less);
                }

                int next_state = 0;
                int delta = 0;
                if (!dfa_transition(state, actual_left, next_state, delta)) {
                    throw std::logic_error("invalid canonical board");
                }
                state = next_state;
                diff += delta;
                continue;
            }

            int actual_right = symbol_at(black, white, r);

            for (int left = 0; left <= actual_left; ++left) {
                for (int right = 0; right < 3; ++right) {
                    if (left == actual_left && right >= actual_right) break;
                    if (!less && left > right) continue;

                    int next_state = 0;
                    int delta = 0;
                    if (!dfa_transition(state, left, next_state, delta)) continue;

                    Transform next_suffix = prepend_symbol(suffix, right);
                    rank += count(k + 1, next_state, diff + delta, next_suffix, less || left < right);
                }
            }

            if (!less && actual_left > actual_right) {
                throw std::logic_error("attempted to rank a non-canonical board");
            }

            int next_state = 0;
            int delta = 0;
            if (!dfa_transition(state, actual_left, next_state, delta)) {
                throw std::logic_error("invalid canonical board");
            }
            state = next_state;
            diff += delta;
            suffix = prepend_symbol(suffix, actual_right);
            less = less || actual_left < actual_right;
        }

        if (!accept_final(state, diff, suffix)) {
            throw std::logic_error("invalid canonical final board");
        }
        return rank;
    }

    Count count(int k, int state, int diff, const Transform& suffix, bool less) {
        std::uint64_t key = pack_key(k, state, diff, suffix, less);
        auto it = memo_.find(key);
        if (it != memo_.end()) return it->second;

        int l = k;
        int r = n_ - 1 - k;
        Count result = 0;

        if (l > r) {
            result = accept_final(state, diff, suffix) ? 1 : 0;
        } else if (l == r) {
            for (int symbol = 0; symbol < 3; ++symbol) {
                int next_state = 0;
                int delta = 0;
                if (!dfa_transition(state, symbol, next_state, delta)) continue;
                result += count(k + 1, next_state, diff + delta, suffix, less);
            }
        } else {
            for (int left = 0; left < 3; ++left) {
                for (int right = 0; right < 3; ++right) {
                    if (!less && left > right) continue;

                    int next_state = 0;
                    int delta = 0;
                    if (!dfa_transition(state, left, next_state, delta)) continue;

                    Transform next_suffix = prepend_symbol(suffix, right);
                    result += count(k + 1, next_state, diff + delta, next_suffix, less || left < right);
                }
            }
        }

        memo_.emplace(key, result);
        return result;
    }

    bool accept_final(int state, int diff, const Transform& suffix) const {
        if (suffix.next[state] < 0) return false;
        int final_state = suffix.next[state];
        int final_diff = diff + suffix.delta;
        return accept_state(final_state) && (final_diff == 0 || final_diff == 1);
    }

    static int symbol_at(Board black, Board white, int pos) {
        Board bit = Board{1} << pos;
        return (black & bit) ? 1 : (white & bit) ? 2 : 0;
    }

    std::uint64_t pack_key(int k, int state, int diff, const Transform& suffix, bool less) const {
        std::uint64_t key = static_cast<std::uint64_t>(k);
        key = (key << 3) | static_cast<std::uint64_t>(state);
        key = (key << 6) | static_cast<std::uint64_t>(diff + n_);
        key = (key << 1) | static_cast<std::uint64_t>(less);
        key = (key << 24) | pack_transform(suffix, n_);
        return key;
    }

    int n_;
    Transform identity_;
    std::unordered_map<std::uint64_t, Count> memo_;
};
