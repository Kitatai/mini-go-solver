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

        if family == "M3_MM_MO_OO":
            by_a = Counter(a for a, _, _ in losses)
            by_b = Counter(b for _, b, _ in losses)
            by_c = Counter(c for _, _, c in losses)
            by_delta = Counter(c - b for _, b, c in losses)
            by_parity = Counter((a % 2, b % 2, c % 2) for a, b, c in losses)
            diagonal = [loss for loss in losses if loss[1] == loss[2]]
            t2 = [loss for loss in losses if loss[2] == loss[1] + 1]
            print("  by a:", " ".join(f"{k}:{v}" for k, v in sorted(by_a.items())))
            print("  by b:", " ".join(f"{k}:{v}" for k, v in sorted(by_b.items())))
            print("  by c:", " ".join(f"{k}:{v}" for k, v in sorted(by_c.items())))
            print("  by c-b:", " ".join(f"{k}:{v}" for k, v in sorted(by_delta.items())))
            print("  parity:", " ".join(f"{k}:{v}" for k, v in sorted(by_parity.items())))
            print("  b=c:", " ".join(f"({a},{b},{c})" for a, b, c in diagonal[:80]))
            if len(diagonal) > 80:
                print(f"  ... {len(diagonal) - 80} more b=c")
            print("  c=b+1:", " ".join(f"({a},{b},{c})" for a, b, c in t2[:80]))
            if len(t2) > 80:
                print(f"  ... {len(t2) - 80} more c=b+1")


if __name__ == "__main__":
    main()
