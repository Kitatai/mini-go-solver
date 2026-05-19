#!/usr/bin/env python3
from __future__ import annotations

import html
import argparse
import re
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_RESULTS = ROOT / "results" / "updated_rules" / "results_new_rules_n2_32.md"
DEFAULT_OUTPUT = ROOT / "results" / "updated_rules" / "results_new_rules_n2_32.svg"


def parse_results(*paths: Path) -> list[tuple[int, list[str]]]:
    by_n: dict[int, list[str]] = {}
    pattern = re.compile(r"N=(\d+):\s+((?:[WL]\s*)+)")

    for path in paths:
        if not path.exists():
            continue
        for line in path.read_text(encoding="utf-8").splitlines():
            match = pattern.search(line.strip().replace("`", ""))
            if not match:
                continue
            n = int(match.group(1))
            values = match.group(2).split()
            by_n[n] = values

    if not by_n:
        raise ValueError("no result rows found")

    return sorted(by_n.items())


def svg_text(x: int, y: int, text: str, size: int = 13, anchor: str = "middle") -> str:
    return (
        f'<text x="{x}" y="{y}" font-family="Arial, sans-serif" '
        f'font-size="{size}" text-anchor="{anchor}" dominant-baseline="middle">'
        f"{html.escape(text)}</text>"
    )


def render_svg(rows: list[tuple[int, list[str]]]) -> str:
    cell = 36
    gap = 4
    left = 64
    top = 70
    row_label_w = 42
    title_h = 28
    max_cols = max(len(values) for _, values in rows)

    width = left + row_label_w + max_cols * (cell + gap) + 24
    height = top + title_h + len(rows) * (cell + gap) + 42

    win = "#2fb344"
    lose = "#e03131"
    blank = "#f1f3f5"
    stroke = "#ffffff"
    parts: list[str] = [
        f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}" '
        f'viewBox="0 0 {width} {height}">',
        '<rect width="100%" height="100%" fill="#ffffff"/>',
        svg_text(width // 2, 24, f"Mini-Go initial move results (N={rows[0][0]}..{rows[-1][0]})", 18),
        svg_text(width // 2, 47, "Green: first player wins / Red: first player loses", 12),
    ]

    grid_x = left + row_label_w
    grid_y = top + title_h

    for col in range(max_cols):
        x = grid_x + col * (cell + gap) + cell // 2
        parts.append(svg_text(x, top + 12, str(col), 12))

    parts.append(svg_text(left + row_label_w // 2, top + 12, "N", 12))

    for row_idx, (n, values) in enumerate(rows):
        y = grid_y + row_idx * (cell + gap)
        parts.append(svg_text(left + row_label_w // 2, y + cell // 2, str(n), 13))

        for col in range(max_cols):
            x = grid_x + col * (cell + gap)
            if col < len(values):
                value = values[col]
                fill = win if value == "W" else lose
            else:
                fill = blank

            parts.append(
                f'<rect x="{x}" y="{y}" width="{cell}" height="{cell}" rx="3" '
                f'fill="{fill}" stroke="{stroke}" stroke-width="1"/>'
            )

    parts.append("</svg>")
    return "\n".join(parts) + "\n"


def main() -> None:
    parser = argparse.ArgumentParser(description="Render Mini-Go result rows as an SVG heatmap.")
    parser.add_argument("results", nargs="?", type=Path, default=DEFAULT_RESULTS)
    parser.add_argument("-o", "--output", type=Path, default=DEFAULT_OUTPUT)
    args = parser.parse_args()

    rows = parse_results(args.results)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(render_svg(rows), encoding="utf-8")
    print(args.output)


if __name__ == "__main__":
    main()
