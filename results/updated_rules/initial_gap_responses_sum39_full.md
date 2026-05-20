# Initial Two-Gap Full Response Data up to Total Empty Length 39

This file summarizes exact response extraction for initial two-gap states

```text
Gap(a, wall, opponent) + Gap(b, opponent, wall)
```

with `a >= 1`, `b >= 1`, and `a + b <= 39`. This is the non-edge initial-move range for board sizes `N <= 40`.

The CSV contains only states where the player to move in the two-gap state is losing. For each legal move by that player, the solver records:

- the first winning response in deterministic solver order
- a winning response with minimum edge distance `min(pos, gap_m - 1 - pos)`
- the minimum edge distance
- the total number of winning responses found

Generated file:

- `initial_gap_responses_sum39_full.csv`

Computation:

```text
states=63870159
wall time=2:00.62
max RSS=4686428 KB
response rows=13795
```

Observed best-response edge-distance distribution for `N >= 20`:

```text
best_response_edge_distance=0: 320
best_response_edge_distance=1: 13165
best_response_edge_distance=2: 308
best_response_edge_distance=3: 2
best_response_edge_distance>3: 0
total: 13795
```

Thus, in this range every recorded losing two-gap state has a winning response within distance `3` from an edge of the selected response gap.

For comparison, the first response in the solver's deterministic order can be farther away:

```text
first_response_pos=0: 98
first_response_pos=1: 4630
first_response_pos=2: 8657
first_response_pos=3: 358
first_response_pos=4: 40
first_response_pos=5: 6
first_response_pos=6: 4
first_response_pos=9: 2
```

This shows that the earlier `response_pos > 3` rows were artifacts of deterministic move order, not evidence that all winning responses must be far from an edge, at least for `N <= 40`.
