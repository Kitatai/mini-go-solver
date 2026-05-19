#pragma once

#include "bitboard.hpp"

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <string>

using Board = minigo::Board32;

class PatternEvaluator {
public:
    explicit PatternEvaluator(bool enabled, bool batch_updates, std::uint32_t sample_rate)
        : enabled_(enabled), batch_updates_(batch_updates), sample_rate_(std::max<std::uint32_t>(1, sample_rate)) {
        init_window_table();
    }

    bool enabled() const { return enabled_; }

    int evaluate(Board black, Board white, int n, bool black_turn) const {
        if (!enabled_) return 0;

        int total = 0;
        int windows = std::max(1, n - 4);
        Board me = black_turn ? black : white;
        Board opp = black_turn ? white : black;
        for (int start = 0; start < windows; ++start) {
            int id = pattern_id_fast(me, opp, start);
            total += weights_[id];
        }
        return total;
    }

    int evaluate_relative(Board me, Board opp, int n) const {
        if (!enabled_) return 0;

        int total = 0;
        int windows = std::max(1, n - 4);
        for (int start = 0; start < windows; ++start) {
            total += weights_[pattern_id_fast(me, opp, start)];
        }
        return total;
    }

    int delta_add_opp_stone(Board me, Board opp, int n, int pos) const {
        if (!enabled_) return 0;

        int delta = 0;
        int first = std::max(0, pos - 4);
        int last = std::min(pos, n - 5);
        if (n < 5) {
            first = 0;
            last = 0;
        }

        Board new_opp = opp | (Board{1} << pos);
        for (int start = first; start <= last; ++start) {
            int before = pattern_id_fast(me, opp, start);
            int after = pattern_id_fast(me, new_opp, start);
            delta += weights_[after] - weights_[before];
        }
        return delta;
    }

    void update(Board black, Board white, int n, bool black_turn, bool turn_player_wins) {
        if (!enabled_ || n < 10) return;
        ++seen_;
        if ((seen_ % sample_rate_) != 0) return;

        int target = turn_player_wins ? 1 : -1;
        int ids[32] = {};
        int score = 0;
        int windows = std::max(1, n - 4);
        Board me = black_turn ? black : white;
        Board opp = black_turn ? white : black;
        for (int start = 0; start < windows; ++start) {
            int id = pattern_id_fast(me, opp, start);
            ids[start] = id;
            score += weights_[id];
        }
        int pred = score >= 0 ? 1 : -1;
        if (pred == target) return;

        for (int start = 0; start < windows; ++start) {
            int id = ids[start];
            if (batch_updates_) {
                pending_delta_[id] += target;
            } else {
                weights_[id] += target;
            }
        }
        ++updates_;
    }

    std::uint64_t updates() const { return updates_; }

    void apply_pending_updates() {
        if (!enabled_ || !batch_updates_) return;
        for (int i = 0; i < 243; ++i) {
            weights_[i] += pending_delta_[i];
            pending_delta_[i] = 0;
        }
    }

    bool load(const std::string& path) {
        if (path.empty()) return true;
        std::ifstream in(path);
        if (!in) return false;
        for (int i = 0; i < 243; ++i) {
            if (!(in >> weights_[i])) return false;
        }
        return true;
    }

    bool save(const std::string& path) const {
        if (path.empty()) return true;
        std::ofstream out(path);
        if (!out) return false;
        for (int i = 0; i < 243; ++i) {
            if (i) out << ' ';
            out << weights_[i];
        }
        out << '\n';
        return true;
    }

private:
    static void init_window_table() {
        static bool initialized = false;
        if (initialized) return;
        for (int me = 0; me < 32; ++me) {
            for (int opp = 0; opp < 32; ++opp) {
                int id = 0;
                int mul = 1;
                for (int offset = 0; offset < 5; ++offset) {
                    int symbol = 0;
                    if (me & (1 << offset)) symbol = 1;
                    if (opp & (1 << offset)) symbol = 2;
                    id += symbol * mul;
                    mul *= 3;
                }
                window_table_[(me << 5) | opp] = id;
            }
        }
        initialized = true;
    }

    static int pattern_id_fast(Board me, Board opp, int start) {
        int me_window = static_cast<int>((me >> start) & 31u);
        int opp_window = static_cast<int>((opp >> start) & 31u);
        return window_table_[(me_window << 5) | opp_window];
    }

    bool enabled_;
    bool batch_updates_;
    std::uint32_t sample_rate_;
    int weights_[243] = {};
    int pending_delta_[243] = {};
    std::uint64_t seen_ = 0;
    std::uint64_t updates_ = 0;
    inline static int window_table_[1024] = {};
};
