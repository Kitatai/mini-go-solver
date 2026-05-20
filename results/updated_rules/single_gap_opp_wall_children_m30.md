# Child States of Winning Moves in `Gap(m, opponent, wall)` up to m=30

This file summarizes the child states reached by every winning first move in

```text
Gap(m, opponent, wall)
```

for `m <= 30`.

Generated file:

- `single_gap_opp_wall_children_m30.csv`

Computation:

```text
states=1800340
wall time=0:01.60
max RSS=141856 KB
```

Every winning move produces exactly two gaps after turn passing and normalization. The child states have the form

```text
Gap(x, wall, opponent) + Gap(y, current, opponent)
```

where `x + y = m - 1`.

Examples:

```text
m=9,  pos=2 -> 2:0:2 6:1:2
m=9,  pos=4 -> 4:0:2 4:1:2
m=18, pos=9 -> 8:1:2 9:0:2
m=28, pos=14 -> 13:1:2 14:0:2
```

This means the near-edge lemma for `Gap(m, opponent, wall)` reduces to a family of two-gap losing statements with boundary type `(wall, opponent)` plus `(current, opponent)`. This family is related to, but not identical with, the initial two-gap family

```text
Gap(a, wall, opponent) + Gap(b, opponent, wall).
```

Observed proof direction:

- For odd `m`, both a small move `pos=2` and a central move often send the opponent to a losing two-gap state.
- For even `m`, central or residue-class dependent moves send the opponent to a losing two-gap state.
- The next proof target is therefore a boundary-sensitive two-gap classification, not only the initial two-gap classification.
