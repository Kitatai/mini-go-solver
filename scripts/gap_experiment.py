#!/usr/bin/env python3
from __future__ import annotations

import argparse
import functools
import re
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_RESULTS = ROOT / "results" / "updated_rules" / "results_new_rules_n2_37.md"

WALL = 0
ME = 1
OPP = 2


def flip(color: int) -> int:
    if color == ME:
        return OPP
    if color == OPP:
        return ME
    return WALL


def canonical_gap(gap: tuple[int, int, int]) -> tuple[int, int, int]:
    m, left, right = gap
    rev = (m, right, left)
    return min(gap, rev)


def normalize(gaps: tuple[tuple[int, int, int], ...]) -> tuple[tuple[int, int, int], ...]:
    return tuple(sorted(canonical_gap(g) for g in gaps if g[0] > 0))


def pass_turn(gaps: list[tuple[int, int, int]]) -> tuple[tuple[int, int, int], ...]:
    return normalize(tuple((m, flip(left), flip(right)) for m, left, right in gaps))


@functools.cache
def gap_sum_win(gaps: tuple[tuple[int, int, int], ...]) -> bool:
    for gap_index, (m, left, right) in enumerate(gaps):
        for pos in range(m):
            if (pos == 0 and left == WALL) or (pos == m - 1 and right == WALL):
                continue
            if (pos == 0 and left == OPP) or (pos == m - 1 and right == OPP):
                continue

            next_gaps = list(gaps[:gap_index] + gaps[gap_index + 1 :])
            if pos > 0:
                next_gaps.append((pos, left, ME))
            if m - pos - 1 > 0:
                next_gaps.append((m - pos - 1, ME, right))

            if not gap_sum_win(pass_turn(next_gaps)):
                return True
    return False


def gap_initial_row(n: int) -> list[str]:
    row: list[str] = []
    for first in range(n):
        if first == 0 or first == n - 1:
            row.append("L")
            continue
        # White is to move. The black first stone is an opponent boundary in
        # the relative-color state used by gap_sum_win.
        gaps = normalize(((first, WALL, OPP), (n - first - 1, OPP, WALL)))
        white_wins = gap_sum_win(gaps)
        row.append("L" if white_wins else "W")
    return row


def parse_results(path: Path) -> dict[int, list[str]]:
    rows: dict[int, list[str]] = {}
    pattern = re.compile(r"N=(\d+):\s+((?:[WL]\s*)+)")
    for line in path.read_text(encoding="utf-8").splitlines():
        match = pattern.search(line.strip().replace("`", ""))
        if not match:
            continue
        rows[int(match.group(1))] = match.group(2).split()
    return rows


def main() -> None:
    parser = argparse.ArgumentParser(description="Compare the gap-sum model with recorded Mini-Go rows.")
    parser.add_argument("--results", type=Path, default=DEFAULT_RESULTS)
    parser.add_argument("--to", type=int, default=37)
    args = parser.parse_args()

    expected = parse_results(args.results)
    mismatches = 0
    for n in range(2, args.to + 1):
        actual = gap_initial_row(n)
        want = expected.get(n)
        status = "OK" if want == actual else "MISMATCH"
        print(f"N={n}: {status} {' '.join(actual)}")
        if want != actual:
            mismatches += 1
            if want is not None:
                print(f"  expected: {' '.join(want)}")
    print(f"states={gap_sum_win.cache_info().currsize} mismatches={mismatches}")
    raise SystemExit(1 if mismatches else 0)


if __name__ == "__main__":
    main()
