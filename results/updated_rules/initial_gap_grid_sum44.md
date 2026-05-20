# Initial Two-Gap Grid up to Total Empty Length 44

This file summarizes the exact gap-game computation for states of the form

```text
Gap(a, wall, opponent) + Gap(b, opponent, wall)
```

with `a >= 1`, `b >= 1`, and `a + b <= 44`.

These are precisely the non-edge initial moves for board sizes `N <= 45`, where `a` is the initial move index and `b = N - a - 1`. The side to move in this gap state is the player who moves after the fixed initial black move.

Generated files:

- `initial_gap_grid_sum44.csv`: exact CSV table
- `initial_gap_grid_sum44.svg`: heatmap of `black_initial_result`

Computation:

```text
states=247466960
wall time=9:23.59
max RSS=19007500 KB
mismatches=0 against the N=2..45 result rows by construction of the same solver state
```

Observed losing cells for `N >= 20`:

- `a = 1`
- `b = 1`
- `a + b` is even and `|a - b| = 2`

In initial-move coordinates this is the same as:

- even `N >= 20`: only the two near-edge interior moves `1` and `N-2` lose among non-edge moves
- odd `N >= 21`: the two near-edge interior moves `1` and `N-2`, and the two moves adjacent to the center, lose among non-edge moves

The edge moves `0` and `N-1` are excluded from this grid and are losing by the non-capturing edge-move theorem.
