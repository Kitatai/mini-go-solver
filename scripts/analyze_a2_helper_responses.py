#!/usr/bin/env python3
from __future__ import annotations

import argparse
import csv
from collections import Counter, defaultdict
from pathlib import Path
import sys

sys.path.append(str(Path(__file__).resolve().parent))
from analyze_a_family_responses import (  # noqa: E402
    classify_child,
    parse_state,
    remove_known_t_components,
    remove_once,
)


def has_family_h0(gaps: list[tuple[int, int, int]]) -> int | None:
    rest = remove_once(gaps, (1, 0, 1))
    if rest is None:
        return None
    rest = remove_once(rest, (2, 0, 2))
    if rest is None:
        return None
    for gap in rest:
        if gap[1:] == (1, 2):
            tail = remove_once(rest, gap)
            if tail is not None and not tail:
                return gap[0]
    return None


def has_family_h1(gaps: list[tuple[int, int, int]]) -> int | None:
    rest = remove_once(gaps, (1, 0, 1))
    if rest is None:
        return None
    rest = remove_once(rest, (1, 2, 2))
    if rest is None:
        return None
    rest = remove_once(rest, (2, 0, 2))
    if rest is None:
        return None
    for gap in rest:
        if gap[1:] == (1, 2):
            tail = remove_once(rest, gap)
            if tail is not None and not tail:
                return gap[0]
    return None


def classify_response_child(text: str) -> str:
    gaps = parse_state(text)
    h0 = has_family_h0(gaps)
    if h0 is not None:
        return f"H0({h0})"
    h1 = has_family_h1(gaps)
    if h1 is not None:
        return f"H1({h1})"

    base = classify_child(text)
    if base != "other":
        return base

    _, remainder = remove_known_t_components(gaps)
    return "rem=" + " ".join(f"{m}:{left}:{right}" for m, left, right in remainder)


def main() -> None:
    parser = argparse.ArgumentParser(description="Summarize response data for H0/H1 helper families.")
    parser.add_argument("csv_path", type=Path)
    args = parser.parse_args()

    rows = list(csv.DictReader(args.csv_path.open()))
    by_family_k: dict[tuple[str, int], Counter[str]] = defaultdict(Counter)
    by_family_k_moves: dict[tuple[str, int], set[int]] = defaultdict(set)

    for row in rows:
        key = (row["family"], int(row["k"]))
        by_family_k[key][classify_response_child(row["response_child_gaps"])] += 1
        by_family_k_moves[key].add(int(row["current_pos"]))

    for (family, k), counts in sorted(by_family_k.items()):
        compact = ", ".join(f"{label}:{count}" for label, count in counts.most_common(8))
        print(f"{family} k={k} moves={len(by_family_k_moves[(family, k)])} {compact}")


if __name__ == "__main__":
    main()
