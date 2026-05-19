#!/usr/bin/env python3
from __future__ import annotations

import subprocess
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


def main() -> None:
    for n in range(2, 16):
        simple = subprocess.check_output([str(ROOT / "bin" / "solve_simple"), str(n)], text=True).strip()
        out = subprocess.check_output(
            [str(ROOT / "bin" / "solve_memo"), str(n), "--sparse", "--learn"],
            text=True,
        )
        memo = next(line for line in out.splitlines() if line.startswith(f"N={n}:"))
        if simple != memo:
            raise SystemExit(f"N={n} mismatch\nsimple: {simple}\nmemo:   {memo}")
    print("OK: solve_simple and solve_memo agree for N=2..15")


if __name__ == "__main__":
    main()
