CXX ?= g++
CXXFLAGS ?= -std=c++20 -O3 -march=native -Wall -Wextra

BIN_DIR := bin
RESULTS_DIR := results/updated_rules

.PHONY: all clean test proof-check plot png

all: $(BIN_DIR)/solve_memo $(BIN_DIR)/solve_memo64 $(BIN_DIR)/solve_gap $(BIN_DIR)/solve_simple

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(BIN_DIR)/solve_memo: src/solve_memo.cpp | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $< -o $@

$(BIN_DIR)/solve_memo64: src/solve_memo64.cpp | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $< -o $@

$(BIN_DIR)/solve_gap: src/solve_gap.cpp | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $< -o $@

$(BIN_DIR)/solve_simple: src/solve_simple.cpp | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $< -o $@

test: all
	python3 scripts/check_small.py

proof-check:
	python3 scripts/check_gap_proof_rules.py --bound 24

plot:
	python3 scripts/plot_results.py

png: plot
	rsvg-convert $(RESULTS_DIR)/results_new_rules_n2_32.svg -o $(RESULTS_DIR)/results_new_rules_n2_32.png

clean:
	rm -rf $(BIN_DIR) scripts/__pycache__ scripts/.pytest_cache
