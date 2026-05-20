# Boundary Two-Gap Grid up to total length 40

This file summarizes exact results for states of the form

```text
Gap(x, wall, opponent) + Gap(y, current, opponent)
```

with `x >= 1`, `y >= 1`, and `x + y <= 40`.

Generated files:

- `boundary_gap_grid_sum40.csv`
- `boundary_gap_grid_sum40.svg`

Computation:

```text
states=57343542
wall time=1:40.16
max RSS=4679380 KB
```

The CSV records whether the player to move in the two-gap state wins.

## Main Observation

For every total length `s = x + y` with `2 <= s <= 40`, the near-balanced cell is losing for the player to move:

```text
x = ceil(s / 2)
y = floor(s / 2)
```

Equivalently:

- if `s = 2k`, then `Gap(k, wall, opponent) + Gap(k, current, opponent)` is losing;
- if `s = 2k + 1`, then `Gap(k + 1, wall, opponent) + Gap(k, current, opponent)` is losing.

This is the exact child state produced by a central move in `Gap(m, opponent, wall)`, with `s = m - 1`.

Therefore, the following conjectural lemma would imply the near-edge single-gap lemma.

```text
For every s >= 2,
Gap(ceil(s / 2), wall, opponent)
+ Gap(floor(s / 2), current, opponent)
is losing for the player to move.
```

If this lemma holds, then for every `m >= 3`, the player to move in

```text
Gap(m, opponent, wall)
```

can play centrally and move the opponent to the losing near-balanced boundary two-gap state.

## Additional Losing Cells

The near-balanced losing cell is not the only losing cell. For example, the exact table also contains cells such as

```text
(2, 2r) for 3 <= r <= 19
(7, 12), (9, 14), (10, 15), (13, 18), ...
```

These additional cells may be useful for alternative constructive strategies, but the central-move proof target only needs the near-balanced family above.
