# Initial Two-Gap Response Data up to Total Empty Length 44

This file summarizes the exact response extraction for initial two-gap states

```text
Gap(a, wall, opponent) + Gap(b, opponent, wall)
```

with `a >= 1`, `b >= 1`, and `a + b <= 44`.

The CSV contains only states where the player to move in the two-gap state is losing. Equivalently, the fixed initial black move is winning. For each legal move by the player to move, the solver records one winning response in the resulting child state.

Generated file:

- `initial_gap_responses_sum44.csv`

Computation:

```text
states=247466960
wall time=9:21.73
max RSS=19007648 KB
response rows=21772
```

The recorded response is the first winning move in the solver's deterministic move order. It is not asserted to be unique.

Observed response-position distribution for `N >= 20`:

```text
response_pos=0: 128
response_pos=1: 6524
response_pos=2: 13688
response_pos=3: 524
response_pos>3: 66
total: 20930
```

Thus, among the observed `N >= 20` response rows, `20864 / 20930` responses are at position `0..3` of the selected response gap.

This suggests a possible proof direction: many winning responses in the stable range may be expressible as local moves near a boundary of one gap, rather than moves depending on the full global length. This is an observation from exact search data, not a theorem.
