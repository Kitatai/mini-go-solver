#!/usr/bin/env python3
from __future__ import annotations

import argparse
import csv
from collections import Counter, defaultdict
from pathlib import Path

from analyze_a2_helper_responses import has_family_h0, has_family_h1
from gap_state import Gap, format_state, parse_state, remove_known_t_components, remove_once


def has_family_k0(gaps: list[Gap]) -> tuple[int, int] | None:
    rest = remove_once(gaps, (2, 0, 2))
    if rest is None:
        return None
    mo_lengths = sorted(gap[0] for gap in rest if gap[1:] == (1, 2))
    if len(rest) == 2 and len(mo_lengths) == 2:
        return mo_lengths[0], mo_lengths[1]
    return None


def has_family_k1(gaps: list[Gap]) -> tuple[int, int] | None:
    rest = remove_once(gaps, (1, 2, 2))
    if rest is None:
        return None
    return has_family_k0(rest)


def classify_response_child(text: str) -> str:
    gaps = parse_state(text)
    _, remainder = remove_known_t_components(gaps)

    h0 = has_family_h0(remainder)
    if h0 is not None:
        return f"H0({h0})+T"
    h1 = has_family_h1(remainder)
    if h1 is not None:
        return f"H1({h1})+T"

    k0 = has_family_k0(remainder)
    if k0 is not None:
        return f"K0({k0[0]},{k0[1]})+T"
    k1 = has_family_k1(remainder)
    if k1 is not None:
        return f"K1({k1[0]},{k1[1]})+T"

    if not remainder:
        return "T-only"
    return "rem=" + format_state(remainder)


def main() -> None:
    parser = argparse.ArgumentParser(description="Summarize response data for K0/K1 helper families.")
    parser.add_argument("csv_path", type=Path)
    args = parser.parse_args()

    rows = list(csv.DictReader(args.csv_path.open()))
    by_family_delta: dict[tuple[str, int], Counter[str]] = defaultdict(Counter)
    by_family_delta_moves: dict[tuple[str, int], set[tuple[int, int, int, int]]] = defaultdict(set)

    for row in rows:
        family = row["family"]
        s = int(row["s"])
        t = int(row["t"])
        delta = t - s
        key = (family, delta)
        by_family_delta[key][classify_response_child(row["response_child_gaps"])] += 1
        by_family_delta_moves[key].add(
            (
                int(row["current_gap_m"]),
                int(row["current_gap_left"]),
                int(row["current_gap_right"]),
                int(row["current_pos"]),
            )
        )

    for (family, delta), counts in sorted(by_family_delta.items()):
        compact = ", ".join(f"{label}:{count}" for label, count in counts.most_common(10))
        print(f"{family} d={delta} moves={len(by_family_delta_moves[(family, delta)])} {compact}")


if __name__ == "__main__":
    main()
