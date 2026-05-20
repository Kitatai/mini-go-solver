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
