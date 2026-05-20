#!/usr/bin/env python3
from __future__ import annotations

import argparse
import csv
import html
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_INPUT = ROOT / "results" / "updated_rules" / "initial_gap_grid_sum44.csv"
DEFAULT_OUTPUT = ROOT / "results" / "updated_rules" / "initial_gap_grid_sum44.svg"


def svg_text(x: int, y: int, text: str, size: int = 12, anchor: str = "middle") -> str:
    return (
        f'<text x="{x}" y="{y}" font-family="Arial, sans-serif" '
        f'font-size="{size}" text-anchor="{anchor}" dominant-baseline="middle">'
        f"{html.escape(text)}</text>"
    )


def read_grid(path: Path, column: str) -> tuple[int, dict[tuple[int, int], str]]:
    cells: dict[tuple[int, int], str] = {}
    max_coord = 0
    with path.open(newline="", encoding="utf-8") as f:
        reader = csv.DictReader(f)
        for row in reader:
            left = int(row["left_len"])
            right = int(row["right_len"])
            cells[(left, right)] = row[column]
            max_coord = max(max_coord, left, right)
    return max_coord, cells


def render_svg(max_coord: int, cells: dict[tuple[int, int], str], title: str) -> str:
    cell = 18
    left_pad = 58
    top_pad = 78
    width = left_pad + (max_coord + 1) * cell + 28
    height = top_pad + (max_coord + 1) * cell + 42
    win = "#2fb344"
    lose = "#e03131"
    blank = "#f1f3f5"
    stroke = "#ffffff"

    parts: list[str] = [
        f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}" '
        f'viewBox="0 0 {width} {height}">',
        '<rect width="100%" height="100%" fill="#ffffff"/>',
        svg_text(width // 2, 24, title, 18),
        svg_text(width // 2, 47, "Green: W / Red: L", 12),
        svg_text(left_pad - 34, top_pad - 20, "left", 12),
        svg_text(left_pad + (max_coord + 1) * cell // 2, top_pad - 20, "right_len", 12),
    ]

    for right in range(max_coord + 1):
        x = left_pad + right * cell + cell // 2
        if right % 5 == 0:
            parts.append(svg_text(x, top_pad - 6, str(right), 10))

    for left in range(max_coord + 1):
        y = top_pad + left * cell + cell // 2
        if left % 5 == 0:
            parts.append(svg_text(left_pad - 12, y, str(left), 10, "end"))
        for right in range(max_coord + 1):
            value = cells.get((left, right))
            fill = blank
            if value == "W":
                fill = win
            elif value == "L":
                fill = lose
            x = left_pad + right * cell
            parts.append(
                f'<rect x="{x}" y="{top_pad + left * cell}" width="{cell}" height="{cell}" '
                f'fill="{fill}" stroke="{stroke}" stroke-width="1"/>'
            )

    parts.append("</svg>")
    return "\n".join(parts) + "\n"


def main() -> None:
    parser = argparse.ArgumentParser(description="Render initial two-gap grid CSV as an SVG heatmap.")
    parser.add_argument("csv", nargs="?", type=Path, default=DEFAULT_INPUT)
    parser.add_argument("-o", "--output", type=Path, default=DEFAULT_OUTPUT)
    parser.add_argument(
        "--column",
        choices=("current_player_result", "black_initial_result"),
        default="black_initial_result",
    )
    args = parser.parse_args()

    max_coord, cells = read_grid(args.csv, args.column)
    title = f"Initial two-gap grid: {args.column}"
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(render_svg(max_coord, cells, title), encoding="utf-8")
    print(args.output)


if __name__ == "__main__":
    main()
