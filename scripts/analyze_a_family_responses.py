#!/usr/bin/env python3
from __future__ import annotations

import argparse
import csv
from collections import Counter, defaultdict
from pathlib import Path

from gap_state import classify_a_family_child


def main() -> None:
    parser = argparse.ArgumentParser(description="Summarize all-response data for A(a,b).")
    parser.add_argument("csv_path", type=Path)
    parser.add_argument("--short-max", type=int, default=8)
    args = parser.parse_args()

    rows = list(csv.DictReader(args.csv_path.open()))

    print("fixed short-side response coverage")
    by_state: dict[tuple[int, int], dict[tuple[int, int], list[dict[str, str]]]] = defaultdict(lambda: defaultdict(list))
    for row in rows:
        a = int(row["left_len"])
        b = int(row["right_len"])
        key = (min(a, b), max(a, b))
        move = (int(row["current_gap_m"]), int(row["current_pos"]))
        by_state[key][move].append(row)

    for (a, b), moves in sorted(by_state.items()):
        if a > args.short_max or b - a <= 1:
            continue
        counts = Counter()
        for responses in moves.values():
            classes = {classify_child(row["response_child_gaps"]) for row in responses}
            if "T-only" in classes:
                counts["T-only-covered"] += 1
            elif "A(2,k)+T" in classes:
                counts["A2-covered"] += 1
            elif "R+T" in classes:
                counts["R-covered"] += 1
            elif "WO(2)+T" in classes:
                counts["WO2-covered"] += 1
            else:
                counts["other-only"] += 1
        total = sum(counts.values())
        print(
            f"A({a},{b}) moves={total} "
            f"T={counts['T-only-covered']} "
            f"A2={counts['A2-covered']} "
            f"R={counts['R-covered']} "
            f"WO2={counts['WO2-covered']} "
            f"other={counts['other-only']}"
        )


def classify_child(text: str) -> str:
    return classify_a_family_child(text)


if __name__ == "__main__":
    main()
