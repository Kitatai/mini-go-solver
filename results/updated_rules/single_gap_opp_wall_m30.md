# Single Gap `Gap(m, opponent, wall)` up to m=30

This file summarizes exact search data for the single-gap state

```text
Gap(m, opponent, wall)
```

from the current player's point of view.

Generated file:

- `single_gap_opp_wall_m30.csv`

Computation:

```text
states=1800340
wall time=0:01.60
max RSS=141988 KB
```

Observed result:

- `m=1,2`: losing
- `m=3..30`: winning

This supports the near-edge proof target that `Gap(m, opponent, wall)` is winning for `m >= 3`.

However, the winning first move is not always near an edge. Some even lengths have only central or near-central winning moves in the recorded summary:

```text
m=12: best_edge_distance=5
m=18: best_edge_distance=8
m=22: best_edge_distance=10
m=28: best_edge_distance=13
```

Thus the proof of this single-gap lemma may require a central-placement or parity argument, not only the local edge-response pattern observed in the two-gap response data.

The expanded CSV records all winning first moves for each `m`, together with their edge distances and doubled center distances. The observed winning move sets suggest the following more refined proof targets.

For odd `m >= 9`, position `2` is always a winning first move in the observed range. The center is also always winning. In addition, for odd `m >= 11` with `m` not divisible by `3`, the position three steps to the right of center is also winning in the observed range:

```text
m=11: 2 5 8
m=13: 2 6 9
m=15: 2 7
m=17: 2 8 11
m=19: 2 9 12
m=21: 2 10
m=23: 2 11 14
m=25: 2 12 15
m=27: 2 13
m=29: 2 14 17
```

For even `m`, central or near-central moves are essential in several cases:

```text
m=12: 6
m=18: 9
m=22: 11
m=28: 14
```

Other even lengths admit additional winning moves away from the exact center:

```text
m=20: 7 10 13 16
m=24: 9 12 17 20
m=26: 5 10 13 15 16 18 20 23
m=30: 9 10 12 15 20 23 26
```

These patterns indicate that the single-gap lemma should likely be proved by splitting odd and even `m`. The odd case may have a simple boundary move for all sufficiently large odd `m`, while the even case appears to require central or residue-class dependent moves.
