#!/usr/bin/env python3
from __future__ import annotations

import argparse
import csv
from collections import Counter, defaultdict
from pathlib import Path


def canon(gap: tuple[int, int, int]) -> tuple[int, int, int]:
    m, left, right = gap
    return min(gap, (m, right, left))


def parse_state(text: str) -> list[tuple[int, int, int]]:
    text = text.strip().strip('"')
    if not text:
        return []
    return sorted(canon(tuple(map(int, item.split(":")))) for item in text.split())


def remove_once(items: list[tuple[int, int, int]], item: tuple[int, int, int]) -> list[tuple[int, int, int]] | None:
    item = canon(item)
    try:
        index = items.index(item)
    except ValueError:
        return None
    return items[:index] + items[index + 1 :]


def remove_known_t_components(
    gaps: list[tuple[int, int, int]],
) -> tuple[list[str], list[tuple[int, int, int]]]:
    gaps = list(gaps)
    labels: list[str] = []
    changed = True
    while changed:
        changed = False
        without_t0 = remove_once(gaps, (1, 0, 1))
        if without_t0 is not None:
            gaps = without_t0
            labels.append("T0")
            changed = True
            continue

        for n in range(1, 128):
            patterns = [
                (f"T1({n})", [(n, 1, 2), (n, 1, 2)]),
                (f"T2({n})", [(n, 2, 2), (n, 1, 1)]),
                (f"T3({n})", [(n, 2, 2), (n, 0, 1)]),
                (f"T4({n})", [(n, 2, 2), (n + 1, 0, 1)]),
                (f"T5({n})", [(n, 0, 2), (n, 1, 2)]),
                (f"T6({n})", [(n + 1, 0, 2), (n, 1, 2)]),
            ]
            for label, pattern in patterns:
                rest = list(gaps)
                for gap in pattern:
                    next_rest = remove_once(rest, gap)
                    if next_rest is None:
                        break
                    rest = next_rest
                else:
                    gaps = rest
                    labels.append(label)
                    changed = True
                    break
            if changed:
                break
    return labels, gaps


def classify_child(text: str) -> str:
    labels, remainder = remove_known_t_components(parse_state(text))
    if not remainder:
        return "T-only"

    rem = remove_once(remainder, (2, 0, 2))
    if rem is not None and len(rem) == 1 and rem[0][1:] == (0, 2):
        return "A(2,k)+T"
    if rem is not None and not rem:
        return "WO(2)+T"
    return "other"


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
            elif "WO(2)+T" in classes:
                counts["WO2-covered"] += 1
            else:
                counts["other-only"] += 1
        total = sum(counts.values())
        print(
            f"A({a},{b}) moves={total} "
            f"T={counts['T-only-covered']} "
            f"A2={counts['A2-covered']} "
            f"WO2={counts['WO2-covered']} "
            f"other={counts['other-only']}"
        )


if __name__ == "__main__":
    main()
