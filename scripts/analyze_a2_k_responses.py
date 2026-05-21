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


BOUNDARY_NAMES = {
    (0, 1): "WM",
    (0, 2): "WO",
    (1, 1): "MM",
    (1, 2): "MO",
    (2, 2): "OO",
}


def gap_name(gap: Gap) -> str:
    return f"{BOUNDARY_NAMES[gap[1:]]}({gap[0]})"


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


def candidate_inert_active_family(gaps: list[Gap]) -> str | None:
    inert_labels, active = remove_current_inert_components(gaps)
    if len(active) != 1:
        return None

    gap = active[0]
    inert_set = set(inert_labels)
    if gap[1:] == (1, 2):
        if any(label.startswith("I(2,2,2)") for label in inert_set):
            if gap[0] != 4:
                return f"Q1({gap[0]})"
            return None
        if any(label.startswith(("I(2,0,2)", "I(1,2,2)")) for label in inert_set):
            if gap[0] <= 2 or (gap[0] >= 6 and gap[0] % 2 == 0):
                return f"Q0({gap[0]})"
            return None
    if gap[1:] == (2, 2):
        if any(label.startswith(("I(2,0,2)", "I(1,2,2)", "I(2,2,2)")) for label in inert_set):
            return f"Q2({gap[0]})"
    return None


def classify_response_child(text: str, use_candidates: bool = False) -> str:
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

    if use_candidates:
        candidate = candidate_inert_active_family(remainder)
        if candidate is not None:
            return f"{candidate}+T?"

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
    if "+T?" in label:
        return 4, label
    if "+T+I[" in label and label.startswith("rem="):
        return 5, label
    if "+I[" in label and label.startswith("rem="):
        return 6, label
    return 7, label


def residual_active_state(label: str) -> list[Gap] | None:
    if not label.startswith("rem="):
        return None
    active_text = label[4:].split("+", 1)[0]
    return parse_state(active_text)


def residual_signature(label: str) -> str | None:
    gaps = residual_active_state(label)
    if gaps is None:
        return None
    return " + ".join(gap_name(gap) for gap in gaps)


def residual_abstract_signature(label: str) -> str | None:
    gaps = residual_active_state(label)
    if gaps is None:
        return None
    return " + ".join(BOUNDARY_NAMES[gap[1:]] for gap in gaps)


def main() -> None:
    parser = argparse.ArgumentParser(description="Summarize response data for K0/K1 helper families.")
    parser.add_argument("csv_path", type=Path)
    parser.add_argument(
        "--best-per-move",
        action="store_true",
        help="If the CSV contains all winning responses, choose the simplest response for each opponent move.",
    )
    parser.add_argument(
        "--residual-summary",
        action="store_true",
        help="Summarize remaining active residual shapes after classification.",
    )
    parser.add_argument(
        "--candidate-families",
        action="store_true",
        help="Use experimental inert+active family labels. These are not proof rules.",
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
            grouped[key].append(classify_response_child(row["response_child_gaps"], args.candidate_families))
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
        label = row.get("classification") or classify_response_child(
            row["response_child_gaps"],
            args.candidate_families,
        )
        by_family_delta[key][label] += 1
        by_family_delta_moves[key].add(
            (
                int(row["current_gap_m"]),
                int(row["current_gap_left"]),
                int(row["current_gap_right"]),
                int(row["current_pos"]),
            )
        )

    if args.residual_summary:
        by_abstract: Counter[str] = Counter()
        by_concrete: Counter[str] = Counter()
        examples: dict[str, str] = {}
        for row in rows:
            label = row.get("classification") or classify_response_child(
                row["response_child_gaps"],
                args.candidate_families,
            )
            abstract = residual_abstract_signature(label)
            concrete = residual_signature(label)
            if abstract is None or concrete is None:
                continue
            by_abstract[abstract] += 1
            by_concrete[concrete] += 1
            examples.setdefault(
                concrete,
                " ".join(
                    [
                        f"family={row['family']}",
                        f"s={row['s']}",
                        f"t={row['t']}",
                        f"move={row['current_gap_m']}:{row['current_gap_left']}:{row['current_gap_right']}@{row['current_pos']}",
                        f"label={label}",
                    ]
                ),
            )

        print("abstract residuals:")
        for signature, count in by_abstract.most_common(20):
            print(f"{signature}: {count}")

        print("\nconcrete residuals:")
        for signature, count in by_concrete.most_common(30):
            print(f"{signature}: {count} | {examples[signature]}")
        return

    for (family, delta), counts in sorted(by_family_delta.items()):
        compact = ", ".join(f"{label}:{count}" for label, count in counts.most_common(10))
        print(f"{family} d={delta} moves={len(by_family_delta_moves[(family, delta)])} {compact}")


if __name__ == "__main__":
    main()
