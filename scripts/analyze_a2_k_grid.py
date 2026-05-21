#!/usr/bin/env python3
from __future__ import annotations

import argparse
import csv
from collections import Counter, defaultdict
from pathlib import Path


def main() -> None:
    parser = argparse.ArgumentParser(description="Summarize K0/K1 grid data from solve_gap.")
    parser.add_argument("csv_path", type=Path)
    args = parser.parse_args()

    rows = list(csv.DictReader(args.csv_path.open()))
    by_family: dict[str, dict[int, str]] = defaultdict(dict)
    conflicts: dict[str, list[tuple[int, int, int, str, str]]] = defaultdict(list)
    counts: dict[str, Counter[str]] = defaultdict(Counter)

    for row in rows:
        family = row["family"]
        s = int(row["s"])
        t = int(row["t"])
        delta = t - s
        result = row["current_player_result"]
        counts[family][result] += 1
        previous = by_family[family].setdefault(delta, result)
        if previous != result:
            conflicts[family].append((s, t, delta, previous, result))

    for family in sorted(by_family):
        max_delta = max(by_family[family])
        sequence = "".join(by_family[family][delta] for delta in range(max_delta + 1))
        print(f"{family} counts={dict(counts[family])}")
        print(f"{family} difference_only={not conflicts[family]}")
        print(f"{family} d=0..{max_delta}: {sequence}")
        if conflicts[family]:
            for s, t, delta, previous, result in conflicts[family][:8]:
                print(f"  conflict s={s} t={t} d={delta}: {previous} vs {result}")


if __name__ == "__main__":
    main()
