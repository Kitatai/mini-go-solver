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
        out64 = subprocess.check_output(
            [str(ROOT / "bin" / "solve_memo64"), str(n), "--sparse", "--learn"],
            text=True,
        )
        memo64 = next(line for line in out64.splitlines() if line.startswith(f"N={n}:"))
        if simple != memo64:
            raise SystemExit(f"N={n} mismatch\nsimple: {simple}\nmemo64: {memo64}")
    print("OK: solve_simple, solve_memo, and solve_memo64 agree for N=2..15")


if __name__ == "__main__":
    main()
