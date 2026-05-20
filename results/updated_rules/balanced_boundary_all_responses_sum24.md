# All Winning Responses for Near-Balanced Boundary States up to total length 24

This file summarizes the all-response table

```text
balanced_boundary_all_responses_sum24.csv
```

for near-balanced boundary states

```text
Gap(ceil(s / 2), wall, opponent)
+ Gap(floor(s / 2), current, opponent).
```

Computation:

```text
states=249272
wall time=0:00.16
max RSS=26788 KB
```

## Purpose

Previous tables recorded one winning response, chosen by minimum edge distance. That is useful for locality, but not enough to test whether a simpler proof strategy exists.

This table records every winning response for each legal move for `s <= 24`.

## Mirror Strategy Check

The natural first proof attempt is a mirror or copycat strategy between the two near-balanced gaps.

The all-response table shows that no simple same-position or reflected-position rule works uniformly, even for `s <= 24`.

For example, there are legal moves whose winning responses exist, but none of them are the naive mirror response in the opposite gap.

Therefore, a proof of the near-balanced two-gap lemma cannot be just the direct mirror strategy used for the odd-board center theorem.

## Consequence

The proof still appears inductive, but the response rule must depend on local gap sizes and boundary types. The useful structure remains:

- every legal move has at least one winning response in the checked range;
- almost all chosen responses are within distance `2` from an edge;
- the nonlocal cases are controlled by length-`3` side-gap configurations.

This supports a local structural induction rather than a pure symmetry argument.
