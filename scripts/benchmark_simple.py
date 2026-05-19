#!/usr/bin/env python3
from __future__ import annotations

import subprocess
import time
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SOLVER = ROOT / "bin" / "solve_simple"
OUTPUT = ROOT / "docs" / "benchmark_simple.md"


def main() -> None:
    lines = [
        "# Simple Solver Benchmark",
        "",
        "Command: `bin/solve_simple N`",
        "",
        "| N | seconds | result |",
        "|---:|---:|---|",
    ]

    for n in range(11, 65):
        start = time.perf_counter()
        try:
            completed = subprocess.run(
                [str(SOLVER), str(n)],
                cwd=ROOT,
                text=True,
                capture_output=True,
                timeout=60.0,
                check=True,
            )
            elapsed = time.perf_counter() - start
            result = completed.stdout.strip()
            print(f"N={n} {elapsed:.3f}s {result}", flush=True)
            lines.append(f"| {n} | {elapsed:.3f} | `{result}` |")
            if elapsed > 60.0:
                break
        except subprocess.TimeoutExpired:
            elapsed = time.perf_counter() - start
            print(f"N={n} timeout after {elapsed:.3f}s", flush=True)
            lines.append(f"| {n} | >60.000 | timeout |")
            break

    OUTPUT.write_text("\n".join(lines) + "\n", encoding="utf-8")
    print(f"wrote {OUTPUT}", flush=True)


if __name__ == "__main__":
    main()
