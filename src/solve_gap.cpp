#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <regex>
#include <string>
#include <string_view>
#include <unordered_map>
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

bool operator<(const Gap& a, const Gap& b) {
    if (a.m != b.m) return a.m < b.m;
    if (a.left != b.left) return a.left < b.left;
    return a.right < b.right;
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

std::uint64_t pack_gap(Gap g) {
    return static_cast<std::uint64_t>(g.m) * 9ULL
         + static_cast<std::uint64_t>(g.left) * 3ULL
         + static_cast<std::uint64_t>(g.right);
}

std::vector<Gap> normalize(std::vector<Gap> gaps) {
    std::vector<Gap> out;
    out.reserve(gaps.size());
    for (Gap g : gaps) {
        if (g.m == 0) continue;
        out.push_back(canonical_gap(g));
    }
    std::sort(out.begin(), out.end());
    return out;
}

std::vector<Gap> pass_turn(std::vector<Gap> gaps) {
    for (Gap& g : gaps) {
        g.left = flip(g.left);
        g.right = flip(g.right);
    }
    return normalize(std::move(gaps));
}

struct GapState {
    std::vector<Gap> gaps;

    bool operator==(const GapState& other) const {
        if (gaps.size() != other.gaps.size()) return false;
        for (std::size_t i = 0; i < gaps.size(); ++i) {
            const Gap& a = gaps[i];
            const Gap& b = other.gaps[i];
            if (a.m != b.m || a.left != b.left || a.right != b.right) return false;
        }
        return true;
    }
};

struct GapStateHash {
    std::size_t operator()(const GapState& state) const {
        std::uint64_t h = 0x9e3779b97f4a7c15ULL ^ state.gaps.size();
        for (Gap g : state.gaps) {
            std::uint64_t x = pack_gap(g) + 0x9e3779b97f4a7c15ULL;
            x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
            x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
            x ^= x >> 31;
            h ^= x + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2);
        }
        return static_cast<std::size_t>(h);
    }
};

class GapSolver {
public:
    bool win(std::vector<Gap> gaps) {
        GapState state{normalize(std::move(gaps))};
        return win_state(state);
    }

    std::uint64_t states() const {
        return memo_.size();
    }

private:
    bool win_state(const GapState& state) {
        auto it = memo_.find(state);
        if (it != memo_.end()) return it->second;

        for (std::size_t gap_index = 0; gap_index < state.gaps.size(); ++gap_index) {
            Gap gap = state.gaps[gap_index];
            for (int pos = 0; pos < gap.m; ++pos) {
                if ((pos == 0 && gap.left == WALL) || (pos == gap.m - 1 && gap.right == WALL)) {
                    continue;
                }
                if ((pos == 0 && gap.left == OPP) || (pos == gap.m - 1 && gap.right == OPP)) {
                    continue;
                }

                std::vector<Gap> next;
                next.reserve(state.gaps.size() + 1);
                for (std::size_t i = 0; i < state.gaps.size(); ++i) {
                    if (i != gap_index) next.push_back(state.gaps[i]);
                }
                if (pos > 0) {
                    next.push_back(Gap{static_cast<std::uint8_t>(pos), gap.left, ME});
                }
                int right_len = gap.m - pos - 1;
                if (right_len > 0) {
                    next.push_back(Gap{static_cast<std::uint8_t>(right_len), ME, gap.right});
                }

                GapState child{pass_turn(std::move(next))};
                if (!win_state(child)) {
                    memo_.emplace(state, true);
                    return true;
                }
            }
        }

        memo_.emplace(state, false);
        return false;
    }

    std::unordered_map<GapState, bool, GapStateHash> memo_;
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
    std::string results_path = "results/updated_rules/results_new_rules_n2_37.md";
    for (int i = 1; i < argc; ++i) {
        std::string_view arg = argv[i];
        if (arg == "--to" && i + 1 < argc) {
            to = std::atoi(argv[++i]);
        } else if (arg == "--results" && i + 1 < argc) {
            results_path = argv[++i];
        }
    }

    auto expected = parse_results(results_path);
    GapSolver solver;
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
