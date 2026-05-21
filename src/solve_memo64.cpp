#include "ranker.hpp"
#include "solver64.hpp"

#include <algorithm>
#include <cstdlib>
#include <exception>
#include <fstream>
#include <iostream>
#include <string>

namespace {

struct Options {
    int n = 33;
    int from_n = -1;
    int to_n = -1;
    bool use_symmetry = true;
    bool use_learning = false;
    bool batch_learning = false;
    bool use_proof_rules = false;
    std::uint32_t learn_sample = 1;
    std::uint64_t sparse_initial_capacity = 0;
    std::string weights_path;
};

std::uint64_t sparse_capacity_from_gib(double gib) {
    if (gib <= 0.0) return 0;
    long double bytes = static_cast<long double>(gib) * 1024.0L * 1024.0L * 1024.0L;
    std::uint64_t max_entries = static_cast<std::uint64_t>(bytes / minigo::SparseMemo64::entry_bytes());
    std::uint64_t capacity = 1;
    while ((capacity << 1) != 0 && (capacity << 1) <= max_entries) {
        capacity <<= 1;
    }
    return capacity;
}

Options parse_options(int argc, char** argv) {
    Options opt;
    bool n_set = false;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--sym") {
            opt.use_symmetry = true;
        } else if (arg == "--no-sym") {
            opt.use_symmetry = false;
        } else if (arg == "--sparse") {
            // solve_memo64 always uses sparse memoization.
        } else if (arg == "--learn") {
            opt.use_learning = true;
        } else if (arg == "--proof-rules") {
            opt.use_proof_rules = true;
        } else if (arg == "--no-proof-rules") {
            opt.use_proof_rules = false;
        } else if (arg == "--learn-batch") {
            opt.use_learning = true;
            opt.batch_learning = true;
        } else if (arg == "--learn-sample" && i + 1 < argc) {
            opt.learn_sample = static_cast<std::uint32_t>(std::max(1, std::atoi(argv[++i])));
        } else if (arg == "--sparse-gib" && i + 1 < argc) {
            opt.sparse_initial_capacity = sparse_capacity_from_gib(std::atof(argv[++i]));
        } else if (arg == "--weights" && i + 1 < argc) {
            opt.weights_path = argv[++i];
        } else if (arg == "--from" && i + 1 < argc) {
            opt.from_n = std::atoi(argv[++i]);
        } else if (arg == "--to" && i + 1 < argc) {
            opt.to_n = std::atoi(argv[++i]);
        } else if (!arg.empty() && arg[0] != '-' && !n_set) {
            opt.n = std::atoi(arg.c_str());
            n_set = true;
        }
    }
    return opt;
}

Count state_count(int n, bool use_symmetry) {
    Ranker ranker(n);
    CanonicalRanker canonical_ranker(n);
    return use_symmetry ? canonical_ranker.total_states() : ranker.total_states();
}

long double count_to_long_double(Count value) {
    constexpr Count chunk_base = 1000000000000000000ULL;
    long double result = 0.0L;
    long double scale = 1.0L;
    while (value > 0) {
        std::uint64_t chunk = static_cast<std::uint64_t>(value % chunk_base);
        result += static_cast<long double>(chunk) * scale;
        scale *= static_cast<long double>(chunk_base);
        value /= chunk_base;
    }
    return result;
}

bool load_weights_if_present(Solver64& solver, const std::string& path) {
    if (path.empty()) return true;
    std::ifstream probe(path);
    return !probe.good() || solver.load_weights(path);
}

void print_run_stats(const Solver64& solver) {
    std::uint64_t filled = solver.filled_memo_entries();
    long double filled_ratio = static_cast<long double>(filled) / count_to_long_double(solver.states());
    std::cout << "nodes=" << solver.nodes()
              << " memo_hits=" << solver.hits()
              << " memo_mode=sparse64"
              << " allocated_bytes=" << solver.memo_bytes()
              << " filled_memo_entries=" << filled
              << " filled_ratio=" << static_cast<double>(filled_ratio)
              << " pruned_edge_moves=" << solver.pruned_edge_moves()
              << " pruned_opponent_capture_replies=" << solver.pruned_opponent_capture_replies()
              << " proof_rule_hits=" << solver.proof_rule_hits()
              << " learning_updates=" << solver.learning_updates() << '\n';
    solver.print_sparse_stats();
}

int solve_one(int n, const Options& opt) {
    Count states = state_count(n, opt.use_symmetry);
    Count dense_bytes = (states * 2 + 7) / 8;
    std::cout << "N=" << n
              << " compressed_states=" << decimal(states)
              << " memo_bytes=" << decimal(dense_bytes) << '\n';

    Solver64 solver(n, opt.use_symmetry, opt.use_learning, opt.batch_learning,
                    opt.use_proof_rules,
                    opt.learn_sample, opt.sparse_initial_capacity);
    if (!load_weights_if_present(solver, opt.weights_path)) {
        std::cerr << "failed to load weights\n";
        return 1;
    }

    std::cout << "N=" << n << ": " << solver.row() << '\n';
    solver.finish_learning_batch();
    print_run_stats(solver);

    if (!opt.weights_path.empty() && !solver.save_weights(opt.weights_path)) {
        std::cerr << "failed to save weights\n";
        return 1;
    }
    return 0;
}

int solve_range(const Options& opt) {
    if (opt.from_n < 2 || opt.to_n < opt.from_n || opt.to_n > 64) {
        std::cerr << "--from/--to must define a range in 2..64\n";
        return 1;
    }
    for (int n = opt.from_n; n <= opt.to_n; ++n) {
        int rc = solve_one(n, opt);
        if (rc != 0) return rc;
    }
    return 0;
}

} // namespace

int main(int argc, char** argv) {
    Options opt = parse_options(argc, argv);

    try {
        if (opt.from_n != -1 || opt.to_n != -1) {
            return solve_range(opt);
        }
        if (opt.n < 2 || opt.n > 64) {
            std::cerr << "N must be in 2..64\n";
            return 1;
        }
        return solve_one(opt.n, opt);
    } catch (const std::bad_alloc&) {
        std::cerr << "allocation failed\n";
        return 2;
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << '\n';
        return 1;
    }
}
