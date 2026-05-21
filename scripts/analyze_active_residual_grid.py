#!/usr/bin/env python3
from __future__ import annotations

import argparse
import csv
from collections import Counter, defaultdict
from pathlib import Path


def main() -> None:
    parser = argparse.ArgumentParser(description="Summarize active residual family grids.")
    parser.add_argument("csv_path", type=Path)
    args = parser.parse_args()

    rows = list(csv.DictReader(args.csv_path.open()))
    by_family: dict[str, list[dict[str, str]]] = defaultdict(list)
    for row in rows:
        by_family[row["family"]].append(row)

    for family, family_rows in sorted(by_family.items()):
        counts = Counter(row["current_player_result"] for row in family_rows)
        print(f"{family} total={len(family_rows)} W={counts['W']} L={counts['L']}")

        losses = [
            (int(row["a"]), int(row["b"]), int(row["c"]))
            for row in family_rows
            if row["current_player_result"] == "L"
        ]
        if not losses:
            print("  losses: none")
            continue

        print("  losses:", " ".join(f"({a},{b},{c})" for a, b, c in losses[:80]))
        if len(losses) > 80:
            print(f"  ... {len(losses) - 80} more")

        if family == "M3_MO_MO_MO":
            explained = [(a, b, c) for a, b, c in losses if a == 1 and b == c]
            exceptional = [loss for loss in losses if loss not in explained]
            print(f"  inert+T1 losses={len(explained)} exceptional={exceptional}")


if __name__ == "__main__":
    main()
