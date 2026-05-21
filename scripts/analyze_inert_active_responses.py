#!/usr/bin/env python3
from __future__ import annotations

import argparse
import csv
from collections import Counter, defaultdict
from pathlib import Path

from analyze_a2_k_responses import classify_response_child, proof_friendliness


def is_candidate_family(row: dict[str, str]) -> bool:
    inert = row["inert"]
    active = row["active"]
    m = int(row["m"])
    if active == "MO":
        if inert in ("WO2", "OO1"):
            return m <= 2 or (m >= 6 and m % 2 == 0)
        if inert == "OO2":
            return m != 4
    if active == "OO":
        return inert in ("WO2", "OO1", "OO2")
    return False


def main() -> None:
    parser = argparse.ArgumentParser(description="Summarize response data for inert+active helper families.")
    parser.add_argument("csv_path", type=Path)
    parser.add_argument(
        "--candidate-only",
        action="store_true",
        help="Only summarize Q0/Q1/Q2 candidate losing families.",
    )
    parser.add_argument(
        "--best-per-move",
        action="store_true",
        help="Choose the simplest winning response for each opponent move.",
    )
    args = parser.parse_args()

    rows = list(csv.DictReader(args.csv_path.open()))
    if args.candidate_only:
        rows = [row for row in rows if is_candidate_family(row)]

    if args.best_per_move:
        grouped: dict[tuple[str, str, int, int, int, int, int], list[str]] = defaultdict(list)
        row_by_move: dict[tuple[str, str, int, int, int, int, int], dict[str, str]] = {}
        for row in rows:
            key = (
                row["inert"],
                row["active"],
                int(row["m"]),
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

    by_family_m: dict[tuple[str, str, int], Counter[str]] = defaultdict(Counter)
    by_family_m_moves: dict[tuple[str, str, int], set[tuple[int, int, int, int]]] = defaultdict(set)
    aggregate: Counter[str] = Counter()

    for row in rows:
        key = (row["inert"], row["active"], int(row["m"]))
        label = row.get("classification") or classify_response_child(
            row["response_child_gaps"],
            use_candidates=True,
        )
        by_family_m[key][label] += 1
        by_family_m_moves[key].add(
            (
                int(row["current_gap_m"]),
                int(row["current_gap_left"]),
                int(row["current_gap_right"]),
                int(row["current_pos"]),
            )
        )
        aggregate[label] += 1

    print("aggregate:")
    for label, count in aggregate.most_common(30):
        print(f"{label}: {count}")

    print("\nby family:")
    for (inert, active, m), counts in sorted(by_family_m.items()):
        compact = ", ".join(f"{label}:{count}" for label, count in counts.most_common(8))
        moves = len(by_family_m_moves[(inert, active, m)])
        print(f"{inert}+{active}({m}) moves={moves} {compact}")


if __name__ == "__main__":
    main()
