#!/usr/bin/env python3
from __future__ import annotations

import argparse
from dataclasses import dataclass


WALL = 0
ME = 1
OPP = 2


@dataclass(frozen=True, order=True)
class Gap:
    m: int
    left: int
    right: int


State = tuple[Gap, ...]


def flip(color: int) -> int:
    if color == ME:
        return OPP
    if color == OPP:
        return ME
    return WALL


def canonical_gap(gap: Gap) -> Gap:
    return min(gap, Gap(gap.m, gap.right, gap.left))


def normalize(gaps: list[Gap] | tuple[Gap, ...]) -> State:
    return tuple(sorted(canonical_gap(gap) for gap in gaps if gap.m > 0))


def pass_turn(gaps: list[Gap]) -> State:
    return normalize([Gap(gap.m, flip(gap.left), flip(gap.right)) for gap in gaps])


def legal(gap: Gap, pos: int) -> bool:
    if pos == 0 and gap.left in (WALL, OPP):
        return False
    if pos == gap.m - 1 and gap.right in (WALL, OPP):
        return False
    return True


def child_after_move(state: State, gap_index: int, pos: int) -> State:
    gap = state[gap_index]
    next_gaps = [g for i, g in enumerate(state) if i != gap_index]
    if pos > 0:
        next_gaps.append(Gap(pos, gap.left, ME))
    right_len = gap.m - pos - 1
    if right_len > 0:
        next_gaps.append(Gap(right_len, ME, gap.right))
    return pass_turn(next_gaps)


def find_gap(state: State, target: Gap) -> int:
    target = canonical_gap(target)
    for i, gap in enumerate(state):
        if gap == target:
            return i
    raise AssertionError(f"missing gap {target} in {state}")


def move_on_gap(state: State, target: Gap, pos: int) -> State:
    index = find_gap(state, target)
    gap = state[index]
    assert legal(gap, pos), f"illegal move: gap={gap} pos={pos} state={state}"
    return child_after_move(state, index, pos)


def wm(n: int) -> Gap:
    return Gap(n, WALL, ME)


def wo(n: int) -> Gap:
    return Gap(n, WALL, OPP)


def mm(n: int) -> Gap:
    return Gap(n, ME, ME)


def mo(n: int) -> Gap:
    return Gap(n, ME, OPP)


def oo(n: int) -> Gap:
    return Gap(n, OPP, OPP)


def state(*gaps: Gap) -> State:
    return normalize(gaps)


def assert_state(actual: State, expected: State, label: str) -> None:
    if actual != expected:
        raise AssertionError(f"{label}\n  actual:   {actual}\n  expected: {expected}")


def check_a_equal(bound: int) -> int:
    checks = 0
    for n in range(1, bound + 1):
        start = state(wo(n), wo(n))
        for p in range(n):
            if not legal(wo(n), p):
                continue
            r = n - p - 1
            after_first = move_on_gap(start, wo(n), p)
            after_response = move_on_gap(after_first, wm(n), r)
            expected = state(wm(p), oo(p), wo(r), mo(r))
            assert_state(after_response, expected, f"A({n},{n}) p={p}")
            checks += 1
    return checks


def check_a_near(bound: int) -> int:
    checks = 0
    for n in range(1, bound + 1):
        start = state(wo(n), wo(n + 1))

        for p in range(n):
            if legal(wo(n), p):
                r = n - p - 1
                after_first = move_on_gap(start, wo(n), p)
                after_response = move_on_gap(after_first, wm(n + 1), r + 1)
                expected = state(wm(p), oo(p), wo(r + 1), mo(r))
                assert_state(after_response, expected, f"A({n},{n + 1}) short p={p}")
                checks += 1

        for p in range(n + 1):
            if legal(wo(n + 1), p):
                r = n - p
                after_first = move_on_gap(start, wo(n + 1), p)
                after_response = move_on_gap(after_first, wm(n), r)
                expected = state(wm(p), oo(p - 1), wo(r), mo(r))
                assert_state(after_response, expected, f"A({n},{n + 1}) long p={p}")
                checks += 1
    return checks


def check_r_family(bound: int) -> int:
    checks = 0
    for a in range(1, bound + 1):
        start = state(mo(1), wm(a), wo(a))

        for p in range(a):
            if legal(wm(a), p):
                after_first = move_on_gap(start, wm(a), p)
                after_response = move_on_gap(after_first, wm(a), p)
                expected = state(mo(1), wm(p), wo(p), oo(a - p - 1), mm(a - p - 1))
                assert_state(after_response, expected, f"R({a}) WM p={p}")
                checks += 1

        for p in range(a):
            if legal(wo(a), p):
                after_first = move_on_gap(start, wo(a), p)
                after_response = move_on_gap(after_first, wo(a), p)
                expected = state(mo(1), wm(p), wo(p), mo(a - p - 1), mo(a - p - 1))
                assert_state(after_response, expected, f"R({a}) WO p={p}")
                checks += 1
    return checks


def main() -> None:
    parser = argparse.ArgumentParser(description="Check documented gap proof response equations.")
    parser.add_argument("--bound", type=int, default=24)
    args = parser.parse_args()

    checks = 0
    checks += check_a_equal(args.bound)
    checks += check_a_near(args.bound)
    checks += check_r_family(args.bound)
    print(f"OK: checked {checks} response equations up to bound={args.bound}")


if __name__ == "__main__":
    main()
