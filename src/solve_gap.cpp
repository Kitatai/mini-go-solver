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
    int single_gap_max = -1;
    int single_gap_children_max = -1;
    int boundary_grid_sum = -1;
    std::string results_path = "results/updated_rules/results_new_rules_n2_37.md";
    std::string initial_grid_csv;
    std::string initial_response_csv;
    std::string single_gap_csv;
    std::string single_gap_children_csv;
    std::string boundary_grid_csv;
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
        } else if (arg == "--single-gap-max" && i + 1 < argc) {
            single_gap_max = std::atoi(argv[++i]);
        } else if (arg == "--single-gap-csv" && i + 1 < argc) {
            single_gap_csv = argv[++i];
        } else if (arg == "--single-gap-children-max" && i + 1 < argc) {
            single_gap_children_max = std::atoi(argv[++i]);
        } else if (arg == "--single-gap-children-csv" && i + 1 < argc) {
            single_gap_children_csv = argv[++i];
        } else if (arg == "--boundary-grid-sum" && i + 1 < argc) {
            boundary_grid_sum = std::atoi(argv[++i]);
        } else if (arg == "--boundary-grid-csv" && i + 1 < argc) {
            boundary_grid_csv = argv[++i];
        }
    }

    GapSolver solver;
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
