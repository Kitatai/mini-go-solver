#!/usr/bin/env python3
from __future__ import annotations

import argparse
import csv
from collections import Counter, defaultdict
from pathlib import Path

from analyze_a2_k_responses import classify_response_child, proof_friendliness


def main() -> None:
    parser = argparse.ArgumentParser(description="Summarize response data for active residual families.")
    parser.add_argument("csv_path", type=Path)
    parser.add_argument(
        "--best-per-move",
        action="store_true",
        help="Choose the simplest winning response for each opponent move.",
    )
    args = parser.parse_args()

    rows = list(csv.DictReader(args.csv_path.open()))
    if args.best_per_move:
        grouped: dict[tuple[str, int, int, int, int, int, int, int], list[str]] = defaultdict(list)
        row_by_move: dict[tuple[str, int, int, int, int, int, int, int], dict[str, str]] = {}
        for row in rows:
            key = (
                row["family"],
                int(row["a"]),
                int(row["b"]),
                int(row["c"]),
                int(row["current_gap_m"]),
                int(row["current_gap_left"]),
                int(row["current_gap_right"]),
                int(row["current_pos"]),
            )
            grouped[key].append(classify_response_child(row["response_child_gaps"], use_candidates=True))
            row_by_move[key] = row
        rows = []
        for key, labels in grouped.items():
            row = dict(row_by_move[key])
            row["classification"] = min(labels, key=proof_friendliness)
            rows.append(row)

    aggregate: Counter[str] = Counter()
    by_family: dict[tuple[str, int, int, int], Counter[str]] = defaultdict(Counter)
    by_family_moves: dict[tuple[str, int, int, int], set[tuple[int, int, int, int]]] = defaultdict(set)
    for row in rows:
        key = (row["family"], int(row["a"]), int(row["b"]), int(row["c"]))
        label = row.get("classification") or classify_response_child(
            row["response_child_gaps"],
            use_candidates=True,
        )
        aggregate[label] += 1
        by_family[key][label] += 1
        by_family_moves[key].add(
            (
                int(row["current_gap_m"]),
                int(row["current_gap_left"]),
                int(row["current_gap_right"]),
                int(row["current_pos"]),
            )
        )

    print("aggregate:")
    for label, count in aggregate.most_common(40):
        print(f"{label}: {count}")

    print("\nby family:")
    for (family, a, b, c), counts in sorted(by_family.items()):
        compact = ", ".join(f"{label}:{count}" for label, count in counts.most_common(8))
        moves = len(by_family_moves[(family, a, b, c)])
        print(f"{family}({a},{b},{c}) moves={moves} {compact}")


if __name__ == "__main__":
    main()
