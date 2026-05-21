#include <algorithm>
#include <array>
#include <cstring>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <regex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

enum Boundary : std::uint8_t {
    WALL = 0,
    ME = 1,
    OPP = 2,
};

struct Gap {
    std::uint8_t m;
    std::uint8_t left;
    std::uint8_t right;
};

constexpr std::size_t MAX_GAPS = 256;

struct GapList {
    std::array<Gap, MAX_GAPS> gaps{};
    std::size_t size = 0;

    const Gap& operator[](std::size_t index) const {
        return gaps[index];
    }

    Gap& operator[](std::size_t index) {
        return gaps[index];
    }

    void push_back(Gap gap) {
        gaps[size++] = gap;
    }
};

struct Move {
    std::uint8_t gap_m = 0;
    std::uint8_t gap_left = 0;
    std::uint8_t gap_right = 0;
    std::uint8_t pos = 0;
};

struct ResponseSummary {
    Move first{};
    Move best{};
    std::uint32_t winning_count = 0;
    std::uint8_t best_edge_distance = 0;
};

struct ResponseDetail {
    ResponseSummary summary{};
    GapList first_child{};
    GapList best_child{};
};

bool operator<(const Gap& a, const Gap& b) {
    if (a.m != b.m) return a.m < b.m;
    if (a.left != b.left) return a.left < b.left;
    return a.right < b.right;
}

bool operator==(const Gap& a, const Gap& b) {
    return a.m == b.m && a.left == b.left && a.right == b.right;
}

std::uint8_t flip(std::uint8_t b) {
    if (b == ME) return OPP;
    if (b == OPP) return ME;
    return WALL;
}

Gap canonical_gap(Gap g) {
    Gap r{g.m, g.right, g.left};
    return r < g ? r : g;
}

void normalize_in_place(GapList& gaps) {
    std::size_t out = 0;
    for (std::size_t i = 0; i < gaps.size; ++i) {
        Gap g = gaps[i];
        if (g.m == 0) continue;
        gaps[out++] = canonical_gap(g);
    }
    gaps.size = out;
    std::sort(gaps.gaps.begin(), gaps.gaps.begin() + static_cast<std::ptrdiff_t>(gaps.size));
}

void pass_turn_in_place(GapList& gaps) {
    for (std::size_t i = 0; i < gaps.size; ++i) {
        Gap& g = gaps[i];
        g.left = flip(g.left);
        g.right = flip(g.right);
    }
    normalize_in_place(gaps);
}

struct PackedKey {
    std::array<char, 512> bytes{};
    std::size_t length = 0;

    std::string_view view() const {
        return std::string_view(bytes.data(), length);
    }
};

PackedKey pack_state(const GapList& gaps) {
    PackedKey key;
    for (std::size_t i = 0; i < gaps.size; ++i) {
        Gap g = gaps[i];
        std::uint16_t packed = static_cast<std::uint16_t>(
            (static_cast<std::uint16_t>(g.m) << 4)
            | (static_cast<std::uint16_t>(g.left) << 2)
            | static_cast<std::uint16_t>(g.right));
        key.bytes[key.length++] = static_cast<char>(packed & 0xff);
        key.bytes[key.length++] = static_cast<char>(packed >> 8);
    }
    return key;
}

std::uint64_t hash_key(std::string_view key) {
    std::uint64_t h = 0x9e3779b97f4a7c15ULL ^ key.size();
    for (unsigned char c : key) {
        h ^= static_cast<std::uint64_t>(c) + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2);
    }
    h = (h ^ (h >> 30)) * 0xbf58476d1ce4e5b9ULL;
    h = (h ^ (h >> 27)) * 0x94d049bb133111ebULL;
    h ^= h >> 31;
    return h;
}

class MemoTable {
public:
    MemoTable() {
        rehash(1ULL << 20);
    }

    bool find(std::string_view key, bool& value) const {
        std::uint64_t hash = hash_key(key);
        std::size_t index = static_cast<std::size_t>(hash) & mask_;
        while (flags_[index] != 0) {
            if (hashes_[index] == hash && key_equals(index, key)) {
                value = flags_[index] == 2;
                return true;
            }
            index = (index + 1) & mask_;
        }
        return false;
    }

    void emplace(std::string_view key, bool value) {
        if ((size_ + 1) * 10 >= hashes_.size() * 7) {
            rehash(hashes_.size() * 2);
        }

        std::uint64_t hash = hash_key(key);
        std::size_t index = static_cast<std::size_t>(hash) & mask_;
        while (flags_[index] != 0) {
            if (hashes_[index] == hash && key_equals(index, key)) {
                flags_[index] = value ? 2 : 1;
                return;
            }
            index = (index + 1) & mask_;
        }

        std::uint64_t offset = key_bytes_.size();
        key_bytes_.insert(key_bytes_.end(), key.begin(), key.end());
        hashes_[index] = hash;
        offsets_[index] = offset;
        lengths_[index] = static_cast<std::uint16_t>(key.size());
        flags_[index] = value ? 2 : 1;
        ++size_;
    }

    std::uint64_t size() const {
        return size_;
    }

private:
    bool key_equals(std::size_t index, std::string_view key) const {
        return lengths_[index] == key.size()
            && std::memcmp(key_bytes_.data() + offsets_[index], key.data(), lengths_[index]) == 0;
    }

    void rehash(std::size_t capacity) {
        std::vector<std::uint64_t> old_hashes = std::move(hashes_);
        std::vector<std::uint64_t> old_offsets = std::move(offsets_);
        std::vector<std::uint16_t> old_lengths = std::move(lengths_);
        std::vector<std::uint8_t> old_flags = std::move(flags_);

        hashes_.assign(capacity, 0);
        offsets_.assign(capacity, 0);
        lengths_.assign(capacity, 0);
        flags_.assign(capacity, 0);
        mask_ = hashes_.size() - 1;

        for (std::size_t old_index = 0; old_index < old_flags.size(); ++old_index) {
            if (old_flags[old_index] == 0) continue;
            std::uint64_t hash = old_hashes[old_index];
            std::size_t index = static_cast<std::size_t>(hash) & mask_;
            while (flags_[index] != 0) {
                index = (index + 1) & mask_;
            }
            hashes_[index] = hash;
            offsets_[index] = old_offsets[old_index];
            lengths_[index] = old_lengths[old_index];
            flags_[index] = old_flags[old_index];
        }
    }

    std::vector<std::uint64_t> hashes_;
    std::vector<std::uint64_t> offsets_;
    std::vector<std::uint16_t> lengths_;
    std::vector<std::uint8_t> flags_;
    std::vector<char> key_bytes_;
    std::uint64_t size_ = 0;
    std::size_t mask_ = 0;
};

class GapSolver {
public:
    bool win(std::vector<Gap> gaps) {
        GapList state;
        for (Gap gap : gaps) state.push_back(gap);
        normalize_in_place(state);
        return win_state(state);
    }

    bool win_initial_gap(int left_len, int right_len) {
        return win_state(initial_gap_state(left_len, right_len));
    }

    bool write_initial_response_csv(int max_sum, const std::string& path) {
        std::ofstream out(path);
        if (!out) return false;

        out << "left_len,right_len,total_empty,n,first,"
            << "current_gap_m,current_gap_left,current_gap_right,current_pos,"
            << "response_gap_m,response_gap_left,response_gap_right,response_pos,"
            << "best_response_gap_m,best_response_gap_left,best_response_gap_right,best_response_pos,"
            << "best_response_edge_distance,winning_response_count\n";
        for (int left_len = 1; left_len <= max_sum; ++left_len) {
            for (int right_len = 1; right_len + left_len <= max_sum; ++right_len) {
                GapList state = initial_gap_state(left_len, right_len);
                if (win_state(state)) continue;

                for (std::size_t gap_index = 0; gap_index < state.size; ++gap_index) {
                    if (gap_index > 0 && state[gap_index] == state[gap_index - 1]) continue;
                    Gap gap = state[gap_index];
                    int pos_limit = gap.left == gap.right ? (gap.m + 1) / 2 : gap.m;
                    for (int pos = 0; pos < pos_limit; ++pos) {
                        if (!is_legal_move(gap, pos)) continue;

                        GapList child = child_after_move(state, gap_index, pos);
                        ResponseSummary response;
                        bool has_response = summarize_winning_moves(child, response);
                        if (!has_response) {
                            std::cerr << "missing response for left_len=" << left_len
                                      << " right_len=" << right_len << '\n';
                            return false;
                        }

                        int total_empty = left_len + right_len;
                        out << left_len << ','
                            << right_len << ','
                            << total_empty << ','
                            << total_empty + 1 << ','
                            << left_len << ','
                            << static_cast<int>(gap.m) << ','
                            << static_cast<int>(gap.left) << ','
                            << static_cast<int>(gap.right) << ','
                            << pos << ','
                            << static_cast<int>(response.first.gap_m) << ','
                            << static_cast<int>(response.first.gap_left) << ','
                            << static_cast<int>(response.first.gap_right) << ','
                            << static_cast<int>(response.first.pos) << ','
                            << static_cast<int>(response.best.gap_m) << ','
                            << static_cast<int>(response.best.gap_left) << ','
                            << static_cast<int>(response.best.gap_right) << ','
                            << static_cast<int>(response.best.pos) << ','
                            << static_cast<int>(response.best_edge_distance) << ','
                            << response.winning_count << '\n';
                    }
                }
            }
        }
        return true;
    }

    bool write_initial_all_response_csv(int max_sum, const std::string& path) {
        std::ofstream out(path);
        if (!out) return false;

        out << "left_len,right_len,total_empty,n,first,"
            << "current_gap_m,current_gap_left,current_gap_right,current_pos,"
            << "response_gap_m,response_gap_left,response_gap_right,response_pos,"
            << "response_child_gap_count,response_child_gaps\n";
        for (int left_len = 1; left_len <= max_sum; ++left_len) {
            for (int right_len = 1; right_len + left_len <= max_sum; ++right_len) {
                GapList state = initial_gap_state(left_len, right_len);
                if (win_state(state)) continue;

                for (std::size_t gap_index = 0; gap_index < state.size; ++gap_index) {
                    if (gap_index > 0 && state[gap_index] == state[gap_index - 1]) continue;
                    Gap gap = state[gap_index];
                    int pos_limit = gap.left == gap.right ? (gap.m + 1) / 2 : gap.m;
                    for (int pos = 0; pos < pos_limit; ++pos) {
                        if (!is_legal_move(gap, pos)) continue;

                        GapList child = child_after_move(state, gap_index, pos);
                        if (!write_all_initial_winning_responses(
                                out,
                                left_len,
                                right_len,
                                gap,
                                pos,
                                child)) {
                            return false;
                        }
                    }
                }
            }
        }
        return true;
    }

    bool write_single_gap_csv(int max_m, const std::string& path) {
        std::ofstream out(path);
        if (!out) return false;

        out << "m,current_player_result,first_win_pos,best_edge_pos,best_edge_distance,winning_move_count,winning_positions,edge_distances,center_distances\n";
        for (int m = 1; m <= max_m; ++m) {
            GapList state = single_gap_state(m, OPP, WALL);
            std::vector<int> winning_positions;
            bool current_wins = collect_winning_positions(state, winning_positions);
            out << m << ','
                << (current_wins ? 'W' : 'L') << ',';
            if (current_wins) {
                ResponseSummary summary = summarize_positions(Gap{static_cast<std::uint8_t>(m), OPP, WALL}, winning_positions);
                out << static_cast<int>(summary.first.pos) << ','
                    << static_cast<int>(summary.best.pos) << ','
                    << static_cast<int>(summary.best_edge_distance) << ','
                    << summary.winning_count << ','
                    << quote_join(winning_positions) << ','
                    << quote_join(edge_distances(Gap{static_cast<std::uint8_t>(m), OPP, WALL}, winning_positions)) << ','
                    << quote_join(center_distances(m, winning_positions)) << '\n';
            } else {
                out << ",,,0,\"\",\"\",\"\"\n";
            }
        }
        return true;
    }

    bool write_single_gap_children_csv(int max_m, const std::string& path) {
        std::ofstream out(path);
        if (!out) return false;

        out << "m,pos,edge_distance,center_distance,child_gap_count,child_gaps\n";
        for (int m = 1; m <= max_m; ++m) {
            GapList state = single_gap_state(m, OPP, WALL);
            if (!win_state(state)) continue;
            Gap gap = state[0];
            int pos_limit = gap.left == gap.right ? (gap.m + 1) / 2 : gap.m;
            for (int pos = 0; pos < pos_limit; ++pos) {
                if (!is_legal_move(gap, pos)) continue;
                GapList child = child_after_move(state, 0, pos);
                if (win_state(child)) continue;
                out << m << ','
                    << pos << ','
                    << static_cast<int>(edge_distance(gap, pos)) << ','
                    << std::abs(2 * pos - (m - 1)) << ','
                    << child.size << ','
                    << quote_gap_list(child) << '\n';
            }
        }
        return true;
    }

    bool write_gap_type_grid_csv(int max_m, const std::string& path) {
        std::ofstream out(path);
        if (!out) return false;

        out << "family,m,current_player_result,state\n";
        for (int m = 1; m <= max_m; ++m) {
            write_named_state_row(out, "WM", single_gap_state(m, WALL, ME));
            write_named_state_row(out, "WO", single_gap_state(m, WALL, OPP));
            write_named_state_row(out, "MM", single_gap_state(m, ME, ME));
            write_named_state_row(out, "MO", single_gap_state(m, ME, OPP));
            write_named_state_row(out, "OO", single_gap_state(m, OPP, OPP));
        }
        return true;
    }

    bool write_boundary_grid_csv(int max_sum, const std::string& path) {
        std::ofstream out(path);
        if (!out) return false;

        out << "wall_opp_len,me_opp_len,total_empty,current_player_result\n";
        for (int wall_opp_len = 1; wall_opp_len <= max_sum; ++wall_opp_len) {
            for (int me_opp_len = 1; me_opp_len + wall_opp_len <= max_sum; ++me_opp_len) {
                GapList state = boundary_state(wall_opp_len, me_opp_len);
                bool current_wins = win_state(state);
                out << wall_opp_len << ','
                    << me_opp_len << ','
                    << wall_opp_len + me_opp_len << ','
                    << (current_wins ? 'W' : 'L') << '\n';
            }
        }
        return true;
    }

    bool write_obstruction_grid_csv(int max_p, int max_delta, const std::string& path) {
        std::ofstream out(path);
        if (!out) return false;

        out << "family,delta,p,total_empty,current_player_result,state\n";
        for (int delta = -max_delta; delta <= max_delta; ++delta) {
            for (int p = 1; p <= max_p; ++p) {
                int oo_len = p + delta;
                if (oo_len <= 0) continue;
                GapList d_state = two_gap_state(p, WALL, ME, oo_len, OPP, OPP);
                write_family_grid_row(out, "D_WM_OO", delta, p, d_state);
            }
        }

        for (int delta = 0; delta <= max_delta; ++delta) {
            for (int p = 1; p <= max_p; ++p) {
                GapList b_state = two_gap_state(p + delta, WALL, OPP, p, ME, OPP);
                write_family_grid_row(out, "B_WO_MO", delta, p, b_state);
            }
        }
        return true;
    }

    bool write_balanced_boundary_response_csv(int max_sum, const std::string& path) {
        std::ofstream out(path);
        if (!out) return false;

        out << "total_empty,wall_opp_len,me_opp_len,"
            << "current_gap_m,current_gap_left,current_gap_right,current_pos,"
            << "child_gap_count,child_gaps,"
            << "response_gap_m,response_gap_left,response_gap_right,response_pos,"
            << "best_response_gap_m,best_response_gap_left,best_response_gap_right,best_response_pos,"
            << "best_response_edge_distance,winning_response_count,"
            << "response_child_gap_count,response_child_gaps\n";
        for (int total = 2; total <= max_sum; ++total) {
            int wall_opp_len = (total + 1) / 2;
            int me_opp_len = total / 2;
            GapList state = boundary_state(wall_opp_len, me_opp_len);
            if (win_state(state)) {
                std::cerr << "near-balanced boundary state is not losing for total="
                          << total << '\n';
                return false;
            }

            for (std::size_t gap_index = 0; gap_index < state.size; ++gap_index) {
                if (gap_index > 0 && state[gap_index] == state[gap_index - 1]) continue;
                Gap gap = state[gap_index];
                int pos_limit = gap.left == gap.right ? (gap.m + 1) / 2 : gap.m;
                for (int pos = 0; pos < pos_limit; ++pos) {
                    if (!is_legal_move(gap, pos)) continue;

                    GapList child = child_after_move(state, gap_index, pos);
                    ResponseDetail response;
                    bool has_response = summarize_winning_moves(child, response);
                    if (!has_response) {
                        std::cerr << "missing response for total=" << total
                                  << " gap_m=" << static_cast<int>(gap.m)
                                  << " pos=" << pos << '\n';
                        return false;
                    }

                    out << total << ','
                        << wall_opp_len << ','
                        << me_opp_len << ','
                        << static_cast<int>(gap.m) << ','
                        << static_cast<int>(gap.left) << ','
                        << static_cast<int>(gap.right) << ','
                        << pos << ','
                        << child.size << ','
                        << quote_gap_list(child) << ','
                        << static_cast<int>(response.summary.first.gap_m) << ','
                        << static_cast<int>(response.summary.first.gap_left) << ','
                        << static_cast<int>(response.summary.first.gap_right) << ','
                        << static_cast<int>(response.summary.first.pos) << ','
                        << static_cast<int>(response.summary.best.gap_m) << ','
                        << static_cast<int>(response.summary.best.gap_left) << ','
                        << static_cast<int>(response.summary.best.gap_right) << ','
                        << static_cast<int>(response.summary.best.pos) << ','
                        << static_cast<int>(response.summary.best_edge_distance) << ','
                        << response.summary.winning_count << ','
                        << response.best_child.size << ','
                        << quote_gap_list(response.best_child) << '\n';
                }
            }
        }
        return true;
    }

    bool write_balanced_boundary_all_responses_csv(int max_sum, const std::string& path) {
        std::ofstream out(path);
        if (!out) return false;

        out << "total_empty,wall_opp_len,me_opp_len,"
            << "current_gap_m,current_gap_left,current_gap_right,current_pos,"
            << "response_gap_m,response_gap_left,response_gap_right,response_pos,"
            << "response_child_gap_count,response_child_gaps\n";
        for (int total = 2; total <= max_sum; ++total) {
            int wall_opp_len = (total + 1) / 2;
            int me_opp_len = total / 2;
            GapList state = boundary_state(wall_opp_len, me_opp_len);
            if (win_state(state)) {
                std::cerr << "near-balanced boundary state is not losing for total="
                          << total << '\n';
                return false;
            }

            for (std::size_t gap_index = 0; gap_index < state.size; ++gap_index) {
                if (gap_index > 0 && state[gap_index] == state[gap_index - 1]) continue;
                Gap gap = state[gap_index];
                int pos_limit = gap.left == gap.right ? (gap.m + 1) / 2 : gap.m;
                for (int pos = 0; pos < pos_limit; ++pos) {
                    if (!is_legal_move(gap, pos)) continue;

                    GapList child = child_after_move(state, gap_index, pos);
                    if (!write_all_winning_responses(
                            out,
                            total,
                            wall_opp_len,
                            me_opp_len,
                            gap,
                            pos,
                            child)) {
                        return false;
                    }
                }
            }
        }
        return true;
    }

    bool write_auxiliary_family_csv(int max_r, const std::string& path) {
        std::ofstream out(path);
        if (!out) return false;

        out << "family,r,total_empty,current_player_result,state\n";
        for (int r = 1; r <= max_r; ++r) {
            write_auxiliary_row(out, "A", r, auxiliary_family_a(r));
            write_auxiliary_row(out, "B", r, auxiliary_family_b(r));
        }
        write_auxiliary_row(out, "C", 3, auxiliary_family_c());
        return true;
    }

    bool write_a2_helper_grid_csv(int max_k, const std::string& path) {
        std::ofstream out(path);
        if (!out) return false;

        out << "family,k,total_empty,current_player_result,state\n";
        for (int k = 1; k <= max_k; ++k) {
            write_auxiliary_row(out, "H0_WM1_WO2_MO", k, a2_helper_h0(k));
            write_auxiliary_row(out, "H1_WM1_OO1_WO2_MO", k, a2_helper_h1(k));
        }
        return true;
    }

    bool write_a2_helper_response_csv(int max_k, const std::string& path) {
        std::ofstream out(path);
        if (!out) return false;

        out << "family,k,total_empty,state,"
            << "current_gap_m,current_gap_left,current_gap_right,current_pos,"
            << "child_gap_count,child_gaps,"
            << "best_response_gap_m,best_response_gap_left,best_response_gap_right,best_response_pos,"
            << "best_response_edge_distance,winning_response_count,"
            << "response_child_gap_count,response_child_gaps\n";
        for (int k = 1; k <= max_k; ++k) {
            GapList h0 = a2_helper_h0(k);
            if (!win_state(h0)) {
                if (!write_auxiliary_responses(out, "H0_WM1_WO2_MO", k, h0)) return false;
            }
            GapList h1 = a2_helper_h1(k);
            if (!win_state(h1)) {
                if (!write_auxiliary_responses(out, "H1_WM1_OO1_WO2_MO", k, h1)) return false;
            }
        }
        return true;
    }

    bool write_a2_k_grid_csv(int max_len, const std::string& path) {
        std::ofstream out(path);
        if (!out) return false;

        out << "family,s,t,total_empty,current_player_result,state\n";
        for (int s = 1; s <= max_len; ++s) {
            for (int t = s; t <= max_len; ++t) {
                write_two_parameter_row(out, "K0_WO2_MO_MO", s, t, a2_k0(s, t));
                write_two_parameter_row(out, "K1_OO1_WO2_MO_MO", s, t, a2_k1(s, t));
            }
        }
        return true;
    }

    bool write_a2_k_response_csv(int max_len, const std::string& path) {
        std::ofstream out(path);
        if (!out) return false;

        out << "family,s,t,total_empty,state,"
            << "current_gap_m,current_gap_left,current_gap_right,current_pos,"
            << "child_gap_count,child_gaps,"
            << "best_response_gap_m,best_response_gap_left,best_response_gap_right,best_response_pos,"
            << "best_response_edge_distance,winning_response_count,"
            << "response_child_gap_count,response_child_gaps\n";
        for (int s = 1; s <= max_len; ++s) {
            for (int t = s; t <= max_len; ++t) {
                GapList k0 = a2_k0(s, t);
                if (!win_state(k0)) {
                    if (!write_two_parameter_responses(out, "K0_WO2_MO_MO", s, t, k0)) return false;
                }
                GapList k1 = a2_k1(s, t);
                if (!win_state(k1)) {
                    if (!write_two_parameter_responses(out, "K1_OO1_WO2_MO_MO", s, t, k1)) return false;
                }
            }
        }
        return true;
    }

    bool write_a2_k_all_response_csv(int max_len, const std::string& path) {
        std::ofstream out(path);
        if (!out) return false;

        out << "family,s,t,total_empty,state,"
            << "current_gap_m,current_gap_left,current_gap_right,current_pos,"
            << "response_gap_m,response_gap_left,response_gap_right,response_pos,"
            << "response_child_gap_count,response_child_gaps\n";
        for (int s = 1; s <= max_len; ++s) {
            for (int t = s; t <= max_len; ++t) {
                GapList k0 = a2_k0(s, t);
                if (!win_state(k0)) {
                    if (!write_two_parameter_all_responses(out, "K0_WO2_MO_MO", s, t, k0)) return false;
                }
                GapList k1 = a2_k1(s, t);
                if (!win_state(k1)) {
                    if (!write_two_parameter_all_responses(out, "K1_OO1_WO2_MO_MO", s, t, k1)) return false;
                }
            }
        }
        return true;
    }

    bool write_auxiliary_response_csv(int max_r, const std::string& path) {
        std::ofstream out(path);
        if (!out) return false;

        out << "family,r,total_empty,state,"
            << "current_gap_m,current_gap_left,current_gap_right,current_pos,"
            << "child_gap_count,child_gaps,"
            << "best_response_gap_m,best_response_gap_left,best_response_gap_right,best_response_pos,"
            << "best_response_edge_distance,winning_response_count,"
            << "response_child_gap_count,response_child_gaps\n";
        for (int r = 1; r <= max_r; ++r) {
            if (!write_auxiliary_responses(out, "A", r, auxiliary_family_a(r))) return false;
            if (!write_auxiliary_responses(out, "B", r, auxiliary_family_b(r))) return false;
        }
        if (!write_auxiliary_responses(out, "C", 3, auxiliary_family_c())) return false;
        return true;
    }

    bool write_six_family_response_csv(int max_n, const std::string& path) {
        std::ofstream out(path);
        if (!out) return false;

        out << "family,n,total_empty,state,"
            << "current_gap_m,current_gap_left,current_gap_right,current_pos,"
            << "child_gap_count,child_gaps,"
            << "best_response_gap_m,best_response_gap_left,best_response_gap_right,best_response_pos,"
            << "best_response_edge_distance,winning_response_count,"
            << "response_child_gap_count,response_child_gaps\n";
        for (int n = 1; n <= max_n; ++n) {
            if (!write_six_family_responses(out, "MO_MO", n, six_family_mo_mo(n))) return false;
            if (!write_six_family_responses(out, "OO_MM", n, six_family_oo_mm(n))) return false;
            if (!write_six_family_responses(out, "OO_WM", n, six_family_oo_wm(n))) return false;
            if (!write_six_family_responses(out, "OO_WM_PLUS", n, six_family_oo_wm_plus(n))) return false;
            if (!write_six_family_responses(out, "WO_MO", n, six_family_wo_mo(n))) return false;
            if (!write_six_family_responses(out, "WO_PLUS_MO", n, six_family_wo_plus_mo(n))) return false;
        }
        return true;
    }

    bool write_six_family_all_response_csv(int max_n, const std::string& path) {
        std::ofstream out(path);
        if (!out) return false;

        out << "family,n,total_empty,state,"
            << "current_gap_m,current_gap_left,current_gap_right,current_pos,"
            << "response_gap_m,response_gap_left,response_gap_right,response_pos,"
            << "response_child_gap_count,response_child_gaps\n";
        for (int n = 1; n <= max_n; ++n) {
            if (!write_six_family_all_responses(out, "MO_MO", n, six_family_mo_mo(n))) return false;
            if (!write_six_family_all_responses(out, "OO_MM", n, six_family_oo_mm(n))) return false;
            if (!write_six_family_all_responses(out, "OO_WM", n, six_family_oo_wm(n))) return false;
            if (!write_six_family_all_responses(out, "OO_WM_PLUS", n, six_family_oo_wm_plus(n))) return false;
            if (!write_six_family_all_responses(out, "WO_MO", n, six_family_wo_mo(n))) return false;
            if (!write_six_family_all_responses(out, "WO_PLUS_MO", n, six_family_wo_plus_mo(n))) return false;
        }
        return true;
    }

    std::uint64_t states() const {
        return memo_.size();
    }

private:
    GapList initial_gap_state(int left_len, int right_len) {
        GapList state;
        state.push_back(Gap{static_cast<std::uint8_t>(left_len), WALL, OPP});
        state.push_back(Gap{static_cast<std::uint8_t>(right_len), OPP, WALL});
        normalize_in_place(state);
        return state;
    }

    GapList boundary_state(int wall_opp_len, int me_opp_len) {
        GapList state;
        state.push_back(Gap{static_cast<std::uint8_t>(wall_opp_len), WALL, OPP});
        state.push_back(Gap{static_cast<std::uint8_t>(me_opp_len), ME, OPP});
        normalize_in_place(state);
        return state;
    }

    GapList auxiliary_family_a(int r) {
        GapList state;
        state.push_back(Gap{3, ME, OPP});
        state.push_back(Gap{3, ME, OPP});
        state.push_back(Gap{static_cast<std::uint8_t>(r), OPP, OPP});
        state.push_back(Gap{static_cast<std::uint8_t>(r + 1), WALL, ME});
        normalize_in_place(state);
        return state;
    }

    GapList auxiliary_family_b(int r) {
        GapList state;
        state.push_back(Gap{3, ME, OPP});
        state.push_back(Gap{4, WALL, OPP});
        state.push_back(Gap{static_cast<std::uint8_t>(r), ME, ME});
        state.push_back(Gap{static_cast<std::uint8_t>(r), OPP, OPP});
        normalize_in_place(state);
        return state;
    }

    GapList auxiliary_family_c() {
        GapList state;
        state.push_back(Gap{3, WALL, ME});
        state.push_back(Gap{3, ME, OPP});
        state.push_back(Gap{3, ME, OPP});
        state.push_back(Gap{3, OPP, OPP});
        normalize_in_place(state);
        return state;
    }

    GapList a2_helper_h0(int k) {
        GapList state;
        state.push_back(Gap{1, WALL, ME});
        state.push_back(Gap{2, WALL, OPP});
        state.push_back(Gap{static_cast<std::uint8_t>(k), ME, OPP});
        normalize_in_place(state);
        return state;
    }

    GapList a2_helper_h1(int k) {
        GapList state;
        state.push_back(Gap{1, WALL, ME});
        state.push_back(Gap{1, OPP, OPP});
        state.push_back(Gap{2, WALL, OPP});
        state.push_back(Gap{static_cast<std::uint8_t>(k), ME, OPP});
        normalize_in_place(state);
        return state;
    }

    GapList a2_k0(int s, int t) {
        GapList state;
        state.push_back(Gap{2, WALL, OPP});
        state.push_back(Gap{static_cast<std::uint8_t>(s), ME, OPP});
        state.push_back(Gap{static_cast<std::uint8_t>(t), ME, OPP});
        normalize_in_place(state);
        return state;
    }

    GapList a2_k1(int s, int t) {
        GapList state;
        state.push_back(Gap{1, OPP, OPP});
        state.push_back(Gap{2, WALL, OPP});
        state.push_back(Gap{static_cast<std::uint8_t>(s), ME, OPP});
        state.push_back(Gap{static_cast<std::uint8_t>(t), ME, OPP});
        normalize_in_place(state);
        return state;
    }

    GapList two_gap_state(int a, std::uint8_t a_left, std::uint8_t a_right,
                          int b, std::uint8_t b_left, std::uint8_t b_right) {
        GapList state;
        state.push_back(Gap{static_cast<std::uint8_t>(a), a_left, a_right});
        state.push_back(Gap{static_cast<std::uint8_t>(b), b_left, b_right});
        normalize_in_place(state);
        return state;
    }

    GapList six_family_mo_mo(int n) {
        return two_gap_state(n, ME, OPP, n, ME, OPP);
    }

    GapList six_family_oo_mm(int n) {
        return two_gap_state(n, OPP, OPP, n, ME, ME);
    }

    GapList six_family_oo_wm(int n) {
        return two_gap_state(n, OPP, OPP, n, WALL, ME);
    }

    GapList six_family_oo_wm_plus(int n) {
        return two_gap_state(n, OPP, OPP, n + 1, WALL, ME);
    }

    GapList six_family_wo_mo(int n) {
        return two_gap_state(n, WALL, OPP, n, ME, OPP);
    }

    GapList six_family_wo_plus_mo(int n) {
        return two_gap_state(n + 1, WALL, OPP, n, ME, OPP);
    }

    int total_empty(const GapList& state) const {
        int total = 0;
        for (std::size_t i = 0; i < state.size; ++i) {
            total += state[i].m;
        }
        return total;
    }

    void write_auxiliary_row(std::ofstream& out, const char* family, int r, GapList state) {
        bool current_wins = win_state(state);
        out << family << ','
            << r << ','
            << total_empty(state) << ','
            << (current_wins ? 'W' : 'L') << ','
            << quote_gap_list(state) << '\n';
    }

    void write_named_state_row(std::ofstream& out, const char* family, GapList state) {
        bool current_wins = win_state(state);
        out << family << ','
            << static_cast<int>(state[0].m) << ','
            << (current_wins ? 'W' : 'L') << ','
            << quote_gap_list(state) << '\n';
    }

    void write_two_parameter_row(
        std::ofstream& out,
        const char* family,
        int s,
        int t,
        GapList state) {
        bool current_wins = win_state(state);
        out << family << ','
            << s << ','
            << t << ','
            << total_empty(state) << ','
            << (current_wins ? 'W' : 'L') << ','
            << quote_gap_list(state) << '\n';
    }

    void write_family_grid_row(
        std::ofstream& out,
        const char* family,
        int delta,
        int p,
        GapList state) {
        bool current_wins = win_state(state);
        out << family << ','
            << delta << ','
            << p << ','
            << total_empty(state) << ','
            << (current_wins ? 'W' : 'L') << ','
            << quote_gap_list(state) << '\n';
    }

    bool write_auxiliary_responses(std::ofstream& out, const char* family, int r, GapList state) {
        if (win_state(state)) {
            std::cerr << "auxiliary state is not losing: family=" << family
                      << " r=" << r << '\n';
            return false;
        }

        for (std::size_t gap_index = 0; gap_index < state.size; ++gap_index) {
            if (gap_index > 0 && state[gap_index] == state[gap_index - 1]) continue;
            Gap gap = state[gap_index];
            int pos_limit = gap.left == gap.right ? (gap.m + 1) / 2 : gap.m;
            for (int pos = 0; pos < pos_limit; ++pos) {
                if (!is_legal_move(gap, pos)) continue;

                GapList child = child_after_move(state, gap_index, pos);
                ResponseDetail response;
                bool has_response = summarize_winning_moves(child, response);
                if (!has_response) {
                    std::cerr << "missing auxiliary response: family=" << family
                              << " r=" << r
                              << " gap_m=" << static_cast<int>(gap.m)
                              << " pos=" << pos << '\n';
                    return false;
                }

                out << family << ','
                    << r << ','
                    << total_empty(state) << ','
                    << quote_gap_list(state) << ','
                    << static_cast<int>(gap.m) << ','
                    << static_cast<int>(gap.left) << ','
                    << static_cast<int>(gap.right) << ','
                    << pos << ','
                    << child.size << ','
                    << quote_gap_list(child) << ','
                    << static_cast<int>(response.summary.best.gap_m) << ','
                    << static_cast<int>(response.summary.best.gap_left) << ','
                    << static_cast<int>(response.summary.best.gap_right) << ','
                    << static_cast<int>(response.summary.best.pos) << ','
                    << static_cast<int>(response.summary.best_edge_distance) << ','
                    << response.summary.winning_count << ','
                    << response.best_child.size << ','
                    << quote_gap_list(response.best_child) << '\n';
            }
        }
        return true;
    }

    bool write_two_parameter_responses(
        std::ofstream& out,
        const char* family,
        int s,
        int t,
        GapList state) {
        if (win_state(state)) {
            std::cerr << "two-parameter state is not losing: family=" << family
                      << " s=" << s << " t=" << t << '\n';
            return false;
        }

        for (std::size_t gap_index = 0; gap_index < state.size; ++gap_index) {
            if (gap_index > 0 && state[gap_index] == state[gap_index - 1]) continue;
            Gap gap = state[gap_index];
            int pos_limit = gap.left == gap.right ? (gap.m + 1) / 2 : gap.m;
            for (int pos = 0; pos < pos_limit; ++pos) {
                if (!is_legal_move(gap, pos)) continue;

                GapList child = child_after_move(state, gap_index, pos);
                ResponseDetail response;
                bool has_response = summarize_winning_moves(child, response);
                if (!has_response) {
                    std::cerr << "missing two-parameter response: family=" << family
                              << " s=" << s
                              << " t=" << t
                              << " gap_m=" << static_cast<int>(gap.m)
                              << " pos=" << pos << '\n';
                    return false;
                }

                out << family << ','
                    << s << ','
                    << t << ','
                    << total_empty(state) << ','
                    << quote_gap_list(state) << ','
                    << static_cast<int>(gap.m) << ','
                    << static_cast<int>(gap.left) << ','
                    << static_cast<int>(gap.right) << ','
                    << pos << ','
                    << child.size << ','
                    << quote_gap_list(child) << ','
                    << static_cast<int>(response.summary.best.gap_m) << ','
                    << static_cast<int>(response.summary.best.gap_left) << ','
                    << static_cast<int>(response.summary.best.gap_right) << ','
                    << static_cast<int>(response.summary.best.pos) << ','
                    << static_cast<int>(response.summary.best_edge_distance) << ','
                    << response.summary.winning_count << ','
                    << response.best_child.size << ','
                    << quote_gap_list(response.best_child) << '\n';
            }
        }
        return true;
    }

    bool write_two_parameter_all_responses(
        std::ofstream& out,
        const char* family,
        int s,
        int t,
        GapList state) {
        if (win_state(state)) {
            std::cerr << "two-parameter state is not losing: family=" << family
                      << " s=" << s << " t=" << t << '\n';
            return false;
        }

        for (std::size_t gap_index = 0; gap_index < state.size; ++gap_index) {
            if (gap_index > 0 && state[gap_index] == state[gap_index - 1]) continue;
            Gap gap = state[gap_index];
            int pos_limit = gap.left == gap.right ? (gap.m + 1) / 2 : gap.m;
            for (int pos = 0; pos < pos_limit; ++pos) {
                if (!is_legal_move(gap, pos)) continue;

                GapList child = child_after_move(state, gap_index, pos);
                if (!write_all_two_parameter_winning_responses(out, family, s, t, state, gap, pos, child)) {
                    return false;
                }
            }
        }
        return true;
    }

    bool write_all_two_parameter_winning_responses(
        std::ofstream& out,
        const char* family,
        int s,
        int t,
        GapList original_state,
        Gap move_gap,
        int move_pos,
        const GapList& state) {
        if (!win_state(state)) return false;
        bool found = false;
        for (std::size_t gap_index = 0; gap_index < state.size; ++gap_index) {
            if (gap_index > 0 && state[gap_index] == state[gap_index - 1]) continue;
            Gap gap = state[gap_index];
            int pos_limit = gap.left == gap.right ? (gap.m + 1) / 2 : gap.m;
            for (int pos = 0; pos < pos_limit; ++pos) {
                if (!is_legal_move(gap, pos)) continue;
                GapList child = child_after_move(state, gap_index, pos);
                if (win_state(child)) continue;
                found = true;
                out << family << ','
                    << s << ','
                    << t << ','
                    << total_empty(original_state) << ','
                    << quote_gap_list(original_state) << ','
                    << static_cast<int>(move_gap.m) << ','
                    << static_cast<int>(move_gap.left) << ','
                    << static_cast<int>(move_gap.right) << ','
                    << move_pos << ','
                    << static_cast<int>(gap.m) << ','
                    << static_cast<int>(gap.left) << ','
                    << static_cast<int>(gap.right) << ','
                    << pos << ','
                    << child.size << ','
                    << quote_gap_list(child) << '\n';
            }
        }
        if (!found) {
            std::cerr << "missing all two-parameter response: family=" << family
                      << " s=" << s
                      << " t=" << t
                      << " gap_m=" << static_cast<int>(move_gap.m)
                      << " pos=" << move_pos << '\n';
        }
        return found;
    }

    bool write_six_family_responses(std::ofstream& out, const char* family, int n, GapList state) {
        if (win_state(state)) {
            std::cerr << "six-family state is not losing: family=" << family
                      << " n=" << n << '\n';
            return false;
        }

        for (std::size_t gap_index = 0; gap_index < state.size; ++gap_index) {
            if (gap_index > 0 && state[gap_index] == state[gap_index - 1]) continue;
            Gap gap = state[gap_index];
            int pos_limit = gap.left == gap.right ? (gap.m + 1) / 2 : gap.m;
            for (int pos = 0; pos < pos_limit; ++pos) {
                if (!is_legal_move(gap, pos)) continue;

                GapList child = child_after_move(state, gap_index, pos);
                ResponseDetail response;
                bool has_response = summarize_winning_moves(child, response);
                if (!has_response) {
                    std::cerr << "missing six-family response: family=" << family
                              << " n=" << n
                              << " gap_m=" << static_cast<int>(gap.m)
                              << " pos=" << pos << '\n';
                    return false;
                }

                out << family << ','
                    << n << ','
                    << total_empty(state) << ','
                    << quote_gap_list(state) << ','
                    << static_cast<int>(gap.m) << ','
                    << static_cast<int>(gap.left) << ','
                    << static_cast<int>(gap.right) << ','
                    << pos << ','
                    << child.size << ','
                    << quote_gap_list(child) << ','
                    << static_cast<int>(response.summary.best.gap_m) << ','
                    << static_cast<int>(response.summary.best.gap_left) << ','
                    << static_cast<int>(response.summary.best.gap_right) << ','
                    << static_cast<int>(response.summary.best.pos) << ','
                    << static_cast<int>(response.summary.best_edge_distance) << ','
                    << response.summary.winning_count << ','
                    << response.best_child.size << ','
                    << quote_gap_list(response.best_child) << '\n';
            }
        }
        return true;
    }

    bool write_six_family_all_responses(std::ofstream& out, const char* family, int n, GapList state) {
        if (win_state(state)) {
            std::cerr << "six-family state is not losing: family=" << family
                      << " n=" << n << '\n';
            return false;
        }

        for (std::size_t gap_index = 0; gap_index < state.size; ++gap_index) {
            if (gap_index > 0 && state[gap_index] == state[gap_index - 1]) continue;
            Gap gap = state[gap_index];
            int pos_limit = gap.left == gap.right ? (gap.m + 1) / 2 : gap.m;
            for (int pos = 0; pos < pos_limit; ++pos) {
                if (!is_legal_move(gap, pos)) continue;

                GapList child = child_after_move(state, gap_index, pos);
                if (!write_all_six_family_winning_responses(out, family, n, state, gap, pos, child)) {
                    return false;
                }
            }
        }
        return true;
    }

    bool write_all_six_family_winning_responses(
        std::ofstream& out,
        const char* family,
        int n,
        GapList original_state,
        Gap move_gap,
        int move_pos,
        const GapList& state) {
        if (!win_state(state)) return false;
        bool found = false;
        for (std::size_t gap_index = 0; gap_index < state.size; ++gap_index) {
            if (gap_index > 0 && state[gap_index] == state[gap_index - 1]) continue;
            Gap gap = state[gap_index];
            int pos_limit = gap.left == gap.right ? (gap.m + 1) / 2 : gap.m;
            for (int pos = 0; pos < pos_limit; ++pos) {
                if (!is_legal_move(gap, pos)) continue;
                GapList child = child_after_move(state, gap_index, pos);
                if (win_state(child)) continue;
                found = true;
                out << family << ','
                    << n << ','
                    << total_empty(original_state) << ','
                    << quote_gap_list(original_state) << ','
                    << static_cast<int>(move_gap.m) << ','
                    << static_cast<int>(move_gap.left) << ','
                    << static_cast<int>(move_gap.right) << ','
                    << move_pos << ','
                    << static_cast<int>(gap.m) << ','
                    << static_cast<int>(gap.left) << ','
                    << static_cast<int>(gap.right) << ','
                    << pos << ','
                    << child.size << ','
                    << quote_gap_list(child) << '\n';
            }
        }
        if (!found) {
            std::cerr << "missing all six-family response: family=" << family
                      << " n=" << n
                      << " gap_m=" << static_cast<int>(move_gap.m)
                      << " pos=" << move_pos << '\n';
        }
        return found;
    }

    bool write_all_winning_responses(
        std::ofstream& out,
        int total,
        int wall_opp_len,
        int me_opp_len,
        Gap move_gap,
        int move_pos,
        const GapList& state) {
        if (!win_state(state)) return false;
        bool found = false;
        for (std::size_t gap_index = 0; gap_index < state.size; ++gap_index) {
            if (gap_index > 0 && state[gap_index] == state[gap_index - 1]) continue;
            Gap gap = state[gap_index];
            int pos_limit = gap.left == gap.right ? (gap.m + 1) / 2 : gap.m;
            for (int pos = 0; pos < pos_limit; ++pos) {
                if (!is_legal_move(gap, pos)) continue;
                GapList child = child_after_move(state, gap_index, pos);
                if (win_state(child)) continue;
                found = true;
                out << total << ','
                    << wall_opp_len << ','
                    << me_opp_len << ','
                    << static_cast<int>(move_gap.m) << ','
                    << static_cast<int>(move_gap.left) << ','
                    << static_cast<int>(move_gap.right) << ','
                    << move_pos << ','
                    << static_cast<int>(gap.m) << ','
                    << static_cast<int>(gap.left) << ','
                    << static_cast<int>(gap.right) << ','
                    << pos << ','
                    << child.size << ','
                    << quote_gap_list(child) << '\n';
            }
        }
        if (!found) {
            std::cerr << "missing all-response row for total=" << total
                      << " gap_m=" << static_cast<int>(move_gap.m)
                      << " pos=" << move_pos << '\n';
        }
        return found;
    }

    bool write_all_initial_winning_responses(
        std::ofstream& out,
        int left_len,
        int right_len,
        Gap move_gap,
        int move_pos,
        const GapList& state) {
        if (!win_state(state)) return false;
        bool found = false;
        for (std::size_t gap_index = 0; gap_index < state.size; ++gap_index) {
            if (gap_index > 0 && state[gap_index] == state[gap_index - 1]) continue;
            Gap gap = state[gap_index];
            int pos_limit = gap.left == gap.right ? (gap.m + 1) / 2 : gap.m;
            for (int pos = 0; pos < pos_limit; ++pos) {
                if (!is_legal_move(gap, pos)) continue;
                GapList child = child_after_move(state, gap_index, pos);
                if (win_state(child)) continue;
                found = true;
                int total_empty = left_len + right_len;
                out << left_len << ','
                    << right_len << ','
                    << total_empty << ','
                    << total_empty + 1 << ','
                    << left_len << ','
                    << static_cast<int>(move_gap.m) << ','
                    << static_cast<int>(move_gap.left) << ','
                    << static_cast<int>(move_gap.right) << ','
                    << move_pos << ','
                    << static_cast<int>(gap.m) << ','
                    << static_cast<int>(gap.left) << ','
                    << static_cast<int>(gap.right) << ','
                    << pos << ','
                    << child.size << ','
                    << quote_gap_list(child) << '\n';
            }
        }
        if (!found) {
            std::cerr << "missing initial all-response row for left_len="
                      << left_len
                      << " right_len=" << right_len
                      << " gap_m=" << static_cast<int>(move_gap.m)
                      << " pos=" << move_pos << '\n';
        }
        return found;
    }

    GapList single_gap_state(int m, std::uint8_t left, std::uint8_t right) {
        GapList state;
        state.push_back(Gap{static_cast<std::uint8_t>(m), left, right});
        normalize_in_place(state);
        return state;
    }

    bool is_legal_move(Gap gap, int pos) const {
        if ((pos == 0 && gap.left == WALL) || (pos == gap.m - 1 && gap.right == WALL)) {
            return false;
        }
        if ((pos == 0 && gap.left == OPP) || (pos == gap.m - 1 && gap.right == OPP)) {
            return false;
        }
        return true;
    }

    GapList child_after_move(const GapList& state, std::size_t gap_index, int pos) {
        Gap gap = state[gap_index];
        GapList next;
        for (std::size_t i = 0; i < state.size; ++i) {
            if (i != gap_index) next.push_back(state[i]);
        }
        if (pos > 0) {
            next.push_back(Gap{static_cast<std::uint8_t>(pos), gap.left, ME});
        }
        int right_len = gap.m - pos - 1;
        if (right_len > 0) {
            next.push_back(Gap{static_cast<std::uint8_t>(right_len), ME, gap.right});
        }

        pass_turn_in_place(next);
        return next;
    }

    std::uint8_t edge_distance(Gap gap, int pos) const {
        int right = static_cast<int>(gap.m) - 1 - pos;
        int dist = std::min(pos, right);
        return static_cast<std::uint8_t>(dist);
    }

    bool summarize_winning_moves(const GapList& state, ResponseSummary& summary) {
        if (!win_state(state)) return false;
        bool found = false;
        for (std::size_t gap_index = 0; gap_index < state.size; ++gap_index) {
            if (gap_index > 0 && state[gap_index] == state[gap_index - 1]) continue;
            Gap gap = state[gap_index];
            int pos_limit = gap.left == gap.right ? (gap.m + 1) / 2 : gap.m;
            for (int pos = 0; pos < pos_limit; ++pos) {
                if (!is_legal_move(gap, pos)) continue;
                GapList child = child_after_move(state, gap_index, pos);
                if (!win_state(child)) {
                    Move move{
                        gap.m,
                        gap.left,
                        gap.right,
                        static_cast<std::uint8_t>(pos),
                    };
                    std::uint8_t distance = edge_distance(gap, pos);
                    if (!found) {
                        summary.first = move;
                        summary.best = move;
                        summary.best_edge_distance = distance;
                        found = true;
                    } else if (distance < summary.best_edge_distance) {
                        summary.best = move;
                        summary.best_edge_distance = distance;
                    }
                    ++summary.winning_count;
                }
            }
        }
        return found;
    }

    bool summarize_winning_moves(const GapList& state, ResponseDetail& detail) {
        if (!win_state(state)) return false;
        bool found = false;
        for (std::size_t gap_index = 0; gap_index < state.size; ++gap_index) {
            if (gap_index > 0 && state[gap_index] == state[gap_index - 1]) continue;
            Gap gap = state[gap_index];
            int pos_limit = gap.left == gap.right ? (gap.m + 1) / 2 : gap.m;
            for (int pos = 0; pos < pos_limit; ++pos) {
                if (!is_legal_move(gap, pos)) continue;
                GapList child = child_after_move(state, gap_index, pos);
                if (!win_state(child)) {
                    Move move{
                        gap.m,
                        gap.left,
                        gap.right,
                        static_cast<std::uint8_t>(pos),
                    };
                    std::uint8_t distance = edge_distance(gap, pos);
                    if (!found) {
                        detail.summary.first = move;
                        detail.summary.best = move;
                        detail.summary.best_edge_distance = distance;
                        detail.first_child = child;
                        detail.best_child = child;
                        found = true;
                    } else if (distance < detail.summary.best_edge_distance) {
                        detail.summary.best = move;
                        detail.summary.best_edge_distance = distance;
                        detail.best_child = child;
                    }
                    ++detail.summary.winning_count;
                }
            }
        }
        return found;
    }

    bool collect_winning_positions(const GapList& state, std::vector<int>& positions) {
        if (!win_state(state)) return false;
        for (std::size_t gap_index = 0; gap_index < state.size; ++gap_index) {
            if (gap_index > 0 && state[gap_index] == state[gap_index - 1]) continue;
            Gap gap = state[gap_index];
            int pos_limit = gap.left == gap.right ? (gap.m + 1) / 2 : gap.m;
            for (int pos = 0; pos < pos_limit; ++pos) {
                if (!is_legal_move(gap, pos)) continue;
                GapList child = child_after_move(state, gap_index, pos);
                if (!win_state(child)) positions.push_back(pos);
            }
        }
        return !positions.empty();
    }

    ResponseSummary summarize_positions(Gap gap, const std::vector<int>& positions) const {
        ResponseSummary summary;
        bool found = false;
        for (int pos : positions) {
            Move move{
                gap.m,
                gap.left,
                gap.right,
                static_cast<std::uint8_t>(pos),
            };
            std::uint8_t distance = edge_distance(gap, pos);
            if (!found) {
                summary.first = move;
                summary.best = move;
                summary.best_edge_distance = distance;
                found = true;
            } else if (distance < summary.best_edge_distance) {
                summary.best = move;
                summary.best_edge_distance = distance;
            }
            ++summary.winning_count;
        }
        return summary;
    }

    std::vector<int> edge_distances(Gap gap, const std::vector<int>& positions) const {
        std::vector<int> values;
        values.reserve(positions.size());
        for (int pos : positions) values.push_back(edge_distance(gap, pos));
        return values;
    }

    std::vector<int> center_distances(int m, const std::vector<int>& positions) const {
        std::vector<int> values;
        values.reserve(positions.size());
        int center2 = m - 1;
        for (int pos : positions) values.push_back(std::abs(2 * pos - center2));
        return values;
    }

    std::string quote_join(const std::vector<int>& values) const {
        std::string out = "\"";
        for (std::size_t i = 0; i < values.size(); ++i) {
            if (i > 0) out.push_back(' ');
            out += std::to_string(values[i]);
        }
        out.push_back('"');
        return out;
    }

    std::string quote_gap_list(const GapList& gaps) const {
        std::string out = "\"";
        for (std::size_t i = 0; i < gaps.size; ++i) {
            if (i > 0) out.push_back(' ');
            const Gap& gap = gaps[i];
            out += std::to_string(gap.m);
            out.push_back(':');
            out += std::to_string(gap.left);
            out.push_back(':');
            out += std::to_string(gap.right);
        }
        out.push_back('"');
        return out;
    }

    bool win_state(const GapList& state) {
        PackedKey key = pack_state(state);
        std::string_view key_view = key.view();
        bool memo_value = false;
        if (memo_.find(key_view, memo_value)) return memo_value;

        for (std::size_t gap_index = 0; gap_index < state.size; ++gap_index) {
            if (gap_index > 0 && state[gap_index] == state[gap_index - 1]) {
                continue;
            }
            Gap gap = state[gap_index];
            int pos_limit = gap.left == gap.right ? (gap.m + 1) / 2 : gap.m;
            for (int pos = 0; pos < pos_limit; ++pos) {
                if (!is_legal_move(gap, pos)) continue;
                GapList next = child_after_move(state, gap_index, pos);
                if (!win_state(next)) {
                    memo_.emplace(key_view, true);
                    return true;
                }
            }
        }

        memo_.emplace(key_view, false);
        return false;
    }

    MemoTable memo_;
};

std::vector<char> gap_row(int n, GapSolver& solver) {
    std::vector<char> row(n);
    for (int first = 0; first < n; ++first) {
        if (first == 0 || first == n - 1) {
            row[first] = 'L';
            continue;
        }
        std::vector<Gap> gaps = {
            Gap{static_cast<std::uint8_t>(first), WALL, OPP},
            Gap{static_cast<std::uint8_t>(n - first - 1), OPP, WALL},
        };
        bool white_wins = solver.win(std::move(gaps));
        row[first] = white_wins ? 'L' : 'W';
    }
    return row;
}

bool initial_gap_current_player_wins(int left_len, int right_len, GapSolver& solver) {
    return solver.win_initial_gap(left_len, right_len);
}

bool write_initial_grid_csv(int max_sum, const std::string& path, GapSolver& solver) {
    std::ofstream out(path);
    if (!out) return false;

    out << "left_len,right_len,total_empty,n,first,current_player_result,black_initial_result\n";
    for (int left_len = 1; left_len <= max_sum; ++left_len) {
        for (int right_len = 1; right_len + left_len <= max_sum; ++right_len) {
            bool current_wins = initial_gap_current_player_wins(left_len, right_len, solver);
            char current_result = current_wins ? 'W' : 'L';
            char black_result = current_wins ? 'L' : 'W';
            int total_empty = left_len + right_len;
            int n = total_empty + 1;
            int first = left_len;
            out << left_len << ','
                << right_len << ','
                << total_empty << ','
                << n << ','
                << first << ','
                << current_result << ','
                << black_result << '\n';
        }
    }
    return true;
}

std::unordered_map<int, std::vector<char>> parse_results(const std::string& path) {
    std::unordered_map<int, std::vector<char>> rows;
    std::ifstream in(path);
    if (!in) return rows;

    std::regex pattern(R"(N=(\d+):\s+((?:[WL]\s*)+))");
    std::string line;
    while (std::getline(in, line)) {
        line.erase(std::remove(line.begin(), line.end(), '`'), line.end());
        std::smatch match;
        if (!std::regex_search(line, match, pattern)) continue;

        int n = std::stoi(match[1]);
        std::vector<char> row;
        for (char c : match[2].str()) {
            if (c == 'W' || c == 'L') row.push_back(c);
        }
        rows[n] = std::move(row);
    }
    return rows;
}

} // namespace

int main(int argc, char** argv) {
    int to = 37;
    int initial_grid_sum = -1;
    int initial_response_sum = -1;
    int initial_all_response_sum = -1;
    int single_gap_max = -1;
    int single_gap_children_max = -1;
    int gap_type_grid_max = -1;
    int boundary_grid_sum = -1;
    int obstruction_grid_max = -1;
    int obstruction_grid_delta = 12;
    int balanced_boundary_response_sum = -1;
    int balanced_boundary_all_response_sum = -1;
    int auxiliary_family_max = -1;
    int auxiliary_response_max = -1;
    int a2_helper_grid_max = -1;
    int a2_helper_response_max = -1;
    int a2_k_grid_max = -1;
    int a2_k_response_max = -1;
    int a2_k_all_response_max = -1;
    int six_family_response_max = -1;
    int six_family_all_response_max = -1;
    std::string results_path = "results/updated_rules/results_new_rules_n2_37.md";
    std::string initial_grid_csv;
    std::string initial_response_csv;
    std::string initial_all_response_csv;
    std::string single_gap_csv;
    std::string single_gap_children_csv;
    std::string gap_type_grid_csv;
    std::string boundary_grid_csv;
    std::string obstruction_grid_csv;
    std::string balanced_boundary_response_csv;
    std::string balanced_boundary_all_response_csv;
    std::string auxiliary_family_csv;
    std::string auxiliary_response_csv;
    std::string a2_helper_grid_csv;
    std::string a2_helper_response_csv;
    std::string a2_k_grid_csv;
    std::string a2_k_response_csv;
    std::string a2_k_all_response_csv;
    std::string six_family_response_csv;
    std::string six_family_all_response_csv;
    for (int i = 1; i < argc; ++i) {
        std::string_view arg = argv[i];
        if (arg == "--to" && i + 1 < argc) {
            to = std::atoi(argv[++i]);
        } else if (arg == "--results" && i + 1 < argc) {
            results_path = argv[++i];
        } else if (arg == "--initial-grid-sum" && i + 1 < argc) {
            initial_grid_sum = std::atoi(argv[++i]);
        } else if (arg == "--initial-grid-csv" && i + 1 < argc) {
            initial_grid_csv = argv[++i];
        } else if (arg == "--initial-response-sum" && i + 1 < argc) {
            initial_response_sum = std::atoi(argv[++i]);
        } else if (arg == "--initial-response-csv" && i + 1 < argc) {
            initial_response_csv = argv[++i];
        } else if (arg == "--initial-all-response-sum" && i + 1 < argc) {
            initial_all_response_sum = std::atoi(argv[++i]);
        } else if (arg == "--initial-all-response-csv" && i + 1 < argc) {
            initial_all_response_csv = argv[++i];
        } else if (arg == "--single-gap-max" && i + 1 < argc) {
            single_gap_max = std::atoi(argv[++i]);
        } else if (arg == "--single-gap-csv" && i + 1 < argc) {
            single_gap_csv = argv[++i];
        } else if (arg == "--single-gap-children-max" && i + 1 < argc) {
            single_gap_children_max = std::atoi(argv[++i]);
        } else if (arg == "--single-gap-children-csv" && i + 1 < argc) {
            single_gap_children_csv = argv[++i];
        } else if (arg == "--gap-type-grid-max" && i + 1 < argc) {
            gap_type_grid_max = std::atoi(argv[++i]);
        } else if (arg == "--gap-type-grid-csv" && i + 1 < argc) {
            gap_type_grid_csv = argv[++i];
        } else if (arg == "--boundary-grid-sum" && i + 1 < argc) {
            boundary_grid_sum = std::atoi(argv[++i]);
        } else if (arg == "--boundary-grid-csv" && i + 1 < argc) {
            boundary_grid_csv = argv[++i];
        } else if (arg == "--obstruction-grid-max" && i + 1 < argc) {
            obstruction_grid_max = std::atoi(argv[++i]);
        } else if (arg == "--obstruction-grid-delta" && i + 1 < argc) {
            obstruction_grid_delta = std::atoi(argv[++i]);
        } else if (arg == "--obstruction-grid-csv" && i + 1 < argc) {
            obstruction_grid_csv = argv[++i];
        } else if (arg == "--balanced-boundary-response-sum" && i + 1 < argc) {
            balanced_boundary_response_sum = std::atoi(argv[++i]);
        } else if (arg == "--balanced-boundary-response-csv" && i + 1 < argc) {
            balanced_boundary_response_csv = argv[++i];
        } else if (arg == "--balanced-boundary-all-response-sum" && i + 1 < argc) {
            balanced_boundary_all_response_sum = std::atoi(argv[++i]);
        } else if (arg == "--balanced-boundary-all-response-csv" && i + 1 < argc) {
            balanced_boundary_all_response_csv = argv[++i];
        } else if (arg == "--auxiliary-family-max" && i + 1 < argc) {
            auxiliary_family_max = std::atoi(argv[++i]);
        } else if (arg == "--auxiliary-family-csv" && i + 1 < argc) {
            auxiliary_family_csv = argv[++i];
        } else if (arg == "--auxiliary-response-max" && i + 1 < argc) {
            auxiliary_response_max = std::atoi(argv[++i]);
        } else if (arg == "--auxiliary-response-csv" && i + 1 < argc) {
            auxiliary_response_csv = argv[++i];
        } else if (arg == "--a2-helper-grid-max" && i + 1 < argc) {
            a2_helper_grid_max = std::atoi(argv[++i]);
        } else if (arg == "--a2-helper-grid-csv" && i + 1 < argc) {
            a2_helper_grid_csv = argv[++i];
        } else if (arg == "--a2-helper-response-max" && i + 1 < argc) {
            a2_helper_response_max = std::atoi(argv[++i]);
        } else if (arg == "--a2-helper-response-csv" && i + 1 < argc) {
            a2_helper_response_csv = argv[++i];
        } else if (arg == "--a2-k-grid-max" && i + 1 < argc) {
            a2_k_grid_max = std::atoi(argv[++i]);
        } else if (arg == "--a2-k-grid-csv" && i + 1 < argc) {
            a2_k_grid_csv = argv[++i];
        } else if (arg == "--a2-k-response-max" && i + 1 < argc) {
            a2_k_response_max = std::atoi(argv[++i]);
        } else if (arg == "--a2-k-response-csv" && i + 1 < argc) {
            a2_k_response_csv = argv[++i];
        } else if (arg == "--a2-k-all-response-max" && i + 1 < argc) {
            a2_k_all_response_max = std::atoi(argv[++i]);
        } else if (arg == "--a2-k-all-response-csv" && i + 1 < argc) {
            a2_k_all_response_csv = argv[++i];
        } else if (arg == "--six-family-response-max" && i + 1 < argc) {
            six_family_response_max = std::atoi(argv[++i]);
        } else if (arg == "--six-family-response-csv" && i + 1 < argc) {
            six_family_response_csv = argv[++i];
        } else if (arg == "--six-family-all-response-max" && i + 1 < argc) {
            six_family_all_response_max = std::atoi(argv[++i]);
        } else if (arg == "--six-family-all-response-csv" && i + 1 < argc) {
            six_family_all_response_csv = argv[++i];
        }
    }

    GapSolver solver;
    if (six_family_all_response_max >= 0) {
        if (six_family_all_response_csv.empty()) {
            std::cerr << "--six-family-all-response-csv is required with --six-family-all-response-max\n";
            return 2;
        }
        if (!solver.write_six_family_all_response_csv(six_family_all_response_max, six_family_all_response_csv)) {
            std::cerr << "failed to write " << six_family_all_response_csv << '\n';
            return 2;
        }
        std::cout << "wrote " << six_family_all_response_csv << '\n';
        std::cout << "states=" << solver.states() << '\n';
        return 0;
    }

    if (six_family_response_max >= 0) {
        if (six_family_response_csv.empty()) {
            std::cerr << "--six-family-response-csv is required with --six-family-response-max\n";
            return 2;
        }
        if (!solver.write_six_family_response_csv(six_family_response_max, six_family_response_csv)) {
            std::cerr << "failed to write " << six_family_response_csv << '\n';
            return 2;
        }
        std::cout << "wrote " << six_family_response_csv << '\n';
        std::cout << "states=" << solver.states() << '\n';
        return 0;
    }

    if (balanced_boundary_all_response_sum >= 0) {
        if (balanced_boundary_all_response_csv.empty()) {
            std::cerr << "--balanced-boundary-all-response-csv is required with --balanced-boundary-all-response-sum\n";
            return 2;
        }
        if (!solver.write_balanced_boundary_all_responses_csv(
                balanced_boundary_all_response_sum,
                balanced_boundary_all_response_csv)) {
            std::cerr << "failed to write " << balanced_boundary_all_response_csv << '\n';
            return 2;
        }
        std::cout << "wrote " << balanced_boundary_all_response_csv << '\n';
        std::cout << "states=" << solver.states() << '\n';
        return 0;
    }

    if (auxiliary_response_max >= 0) {
        if (auxiliary_response_csv.empty()) {
            std::cerr << "--auxiliary-response-csv is required with --auxiliary-response-max\n";
            return 2;
        }
        if (!solver.write_auxiliary_response_csv(auxiliary_response_max, auxiliary_response_csv)) {
            std::cerr << "failed to write " << auxiliary_response_csv << '\n';
            return 2;
        }
        std::cout << "wrote " << auxiliary_response_csv << '\n';
        std::cout << "states=" << solver.states() << '\n';
        return 0;
    }

    if (auxiliary_family_max >= 0) {
        if (auxiliary_family_csv.empty()) {
            std::cerr << "--auxiliary-family-csv is required with --auxiliary-family-max\n";
            return 2;
        }
        if (!solver.write_auxiliary_family_csv(auxiliary_family_max, auxiliary_family_csv)) {
            std::cerr << "failed to write " << auxiliary_family_csv << '\n';
            return 2;
        }
        std::cout << "wrote " << auxiliary_family_csv << '\n';
        std::cout << "states=" << solver.states() << '\n';
        return 0;
    }

    if (a2_helper_response_max >= 0) {
        if (a2_helper_response_csv.empty()) {
            std::cerr << "--a2-helper-response-csv is required with --a2-helper-response-max\n";
            return 2;
        }
        if (!solver.write_a2_helper_response_csv(a2_helper_response_max, a2_helper_response_csv)) {
            std::cerr << "failed to write " << a2_helper_response_csv << '\n';
            return 2;
        }
        std::cout << "wrote " << a2_helper_response_csv << '\n';
        std::cout << "states=" << solver.states() << '\n';
        return 0;
    }

    if (a2_helper_grid_max >= 0) {
        if (a2_helper_grid_csv.empty()) {
            std::cerr << "--a2-helper-grid-csv is required with --a2-helper-grid-max\n";
            return 2;
        }
        if (!solver.write_a2_helper_grid_csv(a2_helper_grid_max, a2_helper_grid_csv)) {
            std::cerr << "failed to write " << a2_helper_grid_csv << '\n';
            return 2;
        }
        std::cout << "wrote " << a2_helper_grid_csv << '\n';
        std::cout << "states=" << solver.states() << '\n';
        return 0;
    }

    if (a2_k_grid_max >= 0) {
        if (a2_k_grid_csv.empty()) {
            std::cerr << "--a2-k-grid-csv is required with --a2-k-grid-max\n";
            return 2;
        }
        if (!solver.write_a2_k_grid_csv(a2_k_grid_max, a2_k_grid_csv)) {
            std::cerr << "failed to write " << a2_k_grid_csv << '\n';
            return 2;
        }
        std::cout << "wrote " << a2_k_grid_csv << '\n';
        std::cout << "states=" << solver.states() << '\n';
        return 0;
    }

    if (a2_k_response_max >= 0) {
        if (a2_k_response_csv.empty()) {
            std::cerr << "--a2-k-response-csv is required with --a2-k-response-max\n";
            return 2;
        }
        if (!solver.write_a2_k_response_csv(a2_k_response_max, a2_k_response_csv)) {
            std::cerr << "failed to write " << a2_k_response_csv << '\n';
            return 2;
        }
        std::cout << "wrote " << a2_k_response_csv << '\n';
        std::cout << "states=" << solver.states() << '\n';
        return 0;
    }

    if (a2_k_all_response_max >= 0) {
        if (a2_k_all_response_csv.empty()) {
            std::cerr << "--a2-k-all-response-csv is required with --a2-k-all-response-max\n";
            return 2;
        }
        if (!solver.write_a2_k_all_response_csv(a2_k_all_response_max, a2_k_all_response_csv)) {
            std::cerr << "failed to write " << a2_k_all_response_csv << '\n';
            return 2;
        }
        std::cout << "wrote " << a2_k_all_response_csv << '\n';
        std::cout << "states=" << solver.states() << '\n';
        return 0;
    }

    if (balanced_boundary_response_sum >= 0) {
        if (balanced_boundary_response_csv.empty()) {
            std::cerr << "--balanced-boundary-response-csv is required with --balanced-boundary-response-sum\n";
            return 2;
        }
        if (!solver.write_balanced_boundary_response_csv(
                balanced_boundary_response_sum,
                balanced_boundary_response_csv)) {
            std::cerr << "failed to write " << balanced_boundary_response_csv << '\n';
            return 2;
        }
        std::cout << "wrote " << balanced_boundary_response_csv << '\n';
        std::cout << "states=" << solver.states() << '\n';
        return 0;
    }

    if (boundary_grid_sum >= 0) {
        if (boundary_grid_csv.empty()) {
            std::cerr << "--boundary-grid-csv is required with --boundary-grid-sum\n";
            return 2;
        }
        if (!solver.write_boundary_grid_csv(boundary_grid_sum, boundary_grid_csv)) {
            std::cerr << "failed to write " << boundary_grid_csv << '\n';
            return 2;
        }
        std::cout << "wrote " << boundary_grid_csv << '\n';
        std::cout << "states=" << solver.states() << '\n';
        return 0;
    }

    if (gap_type_grid_max >= 0) {
        if (gap_type_grid_csv.empty()) {
            std::cerr << "--gap-type-grid-csv is required with --gap-type-grid-max\n";
            return 2;
        }
        if (!solver.write_gap_type_grid_csv(gap_type_grid_max, gap_type_grid_csv)) {
            std::cerr << "failed to write " << gap_type_grid_csv << '\n';
            return 2;
        }
        std::cout << "wrote " << gap_type_grid_csv << '\n';
        std::cout << "states=" << solver.states() << '\n';
        return 0;
    }

    if (obstruction_grid_max >= 0) {
        if (obstruction_grid_csv.empty()) {
            std::cerr << "--obstruction-grid-csv is required with --obstruction-grid-max\n";
            return 2;
        }
        if (!solver.write_obstruction_grid_csv(
                obstruction_grid_max,
                obstruction_grid_delta,
                obstruction_grid_csv)) {
            std::cerr << "failed to write " << obstruction_grid_csv << '\n';
            return 2;
        }
        std::cout << "wrote " << obstruction_grid_csv << '\n';
        std::cout << "states=" << solver.states() << '\n';
        return 0;
    }

    if (single_gap_children_max >= 0) {
        if (single_gap_children_csv.empty()) {
            std::cerr << "--single-gap-children-csv is required with --single-gap-children-max\n";
            return 2;
        }
        if (!solver.write_single_gap_children_csv(single_gap_children_max, single_gap_children_csv)) {
            std::cerr << "failed to write " << single_gap_children_csv << '\n';
            return 2;
        }
        std::cout << "wrote " << single_gap_children_csv << '\n';
        std::cout << "states=" << solver.states() << '\n';
        return 0;
    }

    if (single_gap_max >= 0) {
        if (single_gap_csv.empty()) {
            std::cerr << "--single-gap-csv is required with --single-gap-max\n";
            return 2;
        }
        if (!solver.write_single_gap_csv(single_gap_max, single_gap_csv)) {
            std::cerr << "failed to write " << single_gap_csv << '\n';
            return 2;
        }
        std::cout << "wrote " << single_gap_csv << '\n';
        std::cout << "states=" << solver.states() << '\n';
        return 0;
    }

    if (initial_response_sum >= 0) {
        if (initial_response_csv.empty()) {
            std::cerr << "--initial-response-csv is required with --initial-response-sum\n";
            return 2;
        }
        if (!solver.write_initial_response_csv(initial_response_sum, initial_response_csv)) {
            std::cerr << "failed to write " << initial_response_csv << '\n';
            return 2;
        }
        std::cout << "wrote " << initial_response_csv << '\n';
        std::cout << "states=" << solver.states() << '\n';
        return 0;
    }

    if (initial_all_response_sum >= 0) {
        if (initial_all_response_csv.empty()) {
            std::cerr << "--initial-all-response-csv is required with --initial-all-response-sum\n";
            return 2;
        }
        if (!solver.write_initial_all_response_csv(initial_all_response_sum, initial_all_response_csv)) {
            std::cerr << "failed to write " << initial_all_response_csv << '\n';
            return 2;
        }
        std::cout << "wrote " << initial_all_response_csv << '\n';
        std::cout << "states=" << solver.states() << '\n';
        return 0;
    }

    if (initial_grid_sum >= 0) {
        if (initial_grid_csv.empty()) {
            std::cerr << "--initial-grid-csv is required with --initial-grid-sum\n";
            return 2;
        }
        if (!write_initial_grid_csv(initial_grid_sum, initial_grid_csv, solver)) {
            std::cerr << "failed to write " << initial_grid_csv << '\n';
            return 2;
        }
        std::cout << "wrote " << initial_grid_csv << '\n';
        std::cout << "states=" << solver.states() << '\n';
        return 0;
    }

    auto expected = parse_results(results_path);
    int mismatches = 0;
    for (int n = 2; n <= to; ++n) {
        std::vector<char> row = gap_row(n, solver);
        auto it = expected.find(n);
        bool has_expected = it != expected.end();
        bool ok = has_expected && it->second == row;
        std::cout << "N=" << n << ": ";
        if (has_expected) {
            std::cout << (ok ? "OK" : "MISMATCH");
        } else {
            std::cout << "NEW";
        }
        for (char c : row) std::cout << ' ' << c;
        std::cout << '\n';
        if (has_expected && !ok) {
            ++mismatches;
            std::cout << "  expected:";
            for (char c : it->second) std::cout << ' ' << c;
            std::cout << '\n';
        }
    }

    std::cout << "states=" << solver.states() << " mismatches=" << mismatches << '\n';
    return mismatches == 0 ? 0 : 1;
}
