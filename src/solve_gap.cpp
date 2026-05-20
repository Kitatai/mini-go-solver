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

std::string pack_state(const std::vector<Gap>& gaps) {
    std::string key;
    key.reserve(gaps.size() * 2);
    for (Gap g : gaps) {
        std::uint16_t packed = static_cast<std::uint16_t>(
            (static_cast<std::uint16_t>(g.m) << 4)
            | (static_cast<std::uint16_t>(g.left) << 2)
            | static_cast<std::uint16_t>(g.right));
        key.push_back(static_cast<char>(packed & 0xff));
        key.push_back(static_cast<char>(packed >> 8));
    }
    return key;
}

class GapSolver {
public:
    bool win(std::vector<Gap> gaps) {
        return win_state(normalize(std::move(gaps)));
    }

    std::uint64_t states() const {
        return memo_.size();
    }

private:
    bool win_state(const std::vector<Gap>& state) {
        std::string key = pack_state(state);
        auto it = memo_.find(key);
        if (it != memo_.end()) return it->second;

        for (std::size_t gap_index = 0; gap_index < state.size(); ++gap_index) {
            if (gap_index > 0 && state[gap_index] == state[gap_index - 1]) {
                continue;
            }
            Gap gap = state[gap_index];
            for (int pos = 0; pos < gap.m; ++pos) {
                if ((pos == 0 && gap.left == WALL) || (pos == gap.m - 1 && gap.right == WALL)) {
                    continue;
                }
                if ((pos == 0 && gap.left == OPP) || (pos == gap.m - 1 && gap.right == OPP)) {
                    continue;
                }

                std::vector<Gap> next;
                next.reserve(state.size() + 1);
                for (std::size_t i = 0; i < state.size(); ++i) {
                    if (i != gap_index) next.push_back(state[i]);
                }
                if (pos > 0) {
                    next.push_back(Gap{static_cast<std::uint8_t>(pos), gap.left, ME});
                }
                int right_len = gap.m - pos - 1;
                if (right_len > 0) {
                    next.push_back(Gap{static_cast<std::uint8_t>(right_len), ME, gap.right});
                }

                std::vector<Gap> child = pass_turn(std::move(next));
                if (!win_state(child)) {
                    memo_.emplace(std::move(key), true);
                    return true;
                }
            }
        }

        memo_.emplace(std::move(key), false);
        return false;
    }

    std::unordered_map<std::string, bool> memo_;
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
