#!/usr/bin/env python3
from __future__ import annotations

import argparse
import csv
from collections import Counter, defaultdict
from pathlib import Path

from analyze_a2_helper_responses import has_family_h0, has_family_h1
from gap_state import (
    Gap,
    format_state,
    parse_state,
    remove_current_inert_components,
    remove_known_t_components,
    remove_once,
)


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
    t_labels, remainder = remove_known_t_components(gaps)

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

    inert_labels, active_remainder = remove_current_inert_components(remainder)
    if inert_labels:
        inert_summary = ",".join(
            f"{label}x{count}" if count > 1 else label
            for label, count in sorted(Counter(inert_labels).items())
        )
        prefix = "T+" if t_labels else ""
        if not active_remainder:
            return f"{prefix}I-only[{inert_summary}]"
        return f"rem={format_state(active_remainder)}+{prefix}I[{inert_summary}]"

    return "rem=" + format_state(remainder)


def proof_friendliness(label: str) -> tuple[int, str]:
    if label == "T-only":
        return 0, label
    if "I-only" in label:
        return 1, label
    if label.startswith("K0(") or label.startswith("K1("):
        return 2, label
    if label.startswith("H0(") or label.startswith("H1("):
        return 3, label
    if "+T+I[" in label and label.startswith("rem="):
        return 4, label
    if "+I[" in label and label.startswith("rem="):
        return 5, label
    return 6, label


def main() -> None:
    parser = argparse.ArgumentParser(description="Summarize response data for K0/K1 helper families.")
    parser.add_argument("csv_path", type=Path)
    parser.add_argument(
        "--best-per-move",
        action="store_true",
        help="If the CSV contains all winning responses, choose the simplest response for each opponent move.",
    )
    args = parser.parse_args()

    rows = list(csv.DictReader(args.csv_path.open()))
    by_family_delta: dict[tuple[str, int], Counter[str]] = defaultdict(Counter)
    by_family_delta_moves: dict[tuple[str, int], set[tuple[int, int, int, int]]] = defaultdict(set)

    if args.best_per_move:
        grouped: dict[tuple[str, int, int, int, int, int, int], list[str]] = defaultdict(list)
        row_by_move: dict[tuple[str, int, int, int, int, int, int], dict[str, str]] = {}
        for row in rows:
            key = (
                row["family"],
                int(row["s"]),
                int(row["t"]),
                int(row["current_gap_m"]),
                int(row["current_gap_left"]),
                int(row["current_gap_right"]),
                int(row["current_pos"]),
            )
            grouped[key].append(classify_response_child(row["response_child_gaps"]))
            row_by_move[key] = row
        rows = []
        for key, labels in grouped.items():
            row = dict(row_by_move[key])
            row["classification"] = min(labels, key=proof_friendliness)
            rows.append(row)

    for row in rows:
        family = row["family"]
        s = int(row["s"])
        t = int(row["t"])
        delta = t - s
        key = (family, delta)
        label = row.get("classification") or classify_response_child(row["response_child_gaps"])
        by_family_delta[key][label] += 1
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
