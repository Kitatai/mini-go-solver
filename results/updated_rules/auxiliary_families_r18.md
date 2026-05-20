# Auxiliary Four-Gap Families up to r=18

This file summarizes exact checks for auxiliary losing families suggested by the exceptional responses in

```text
balanced_boundary_responses_sum40.csv
```

Generated file:

- `auxiliary_families_r18.csv`

Computation:

```text
states=52121717
wall time=1:46.06
max RSS=4828432 KB
```

## Families

The exceptional responses suggested two infinite four-gap families and one small special state.

Family A:

```text
Gap(3, current, opponent)
+ Gap(3, current, opponent)
+ Gap(r, opponent, opponent)
+ Gap(r + 1, wall, current)
```

Family B:

```text
Gap(3, current, opponent)
+ Gap(4, wall, opponent)
+ Gap(r, current, current)
+ Gap(r, opponent, opponent)
```

Special state C:

```text
Gap(3, wall, current)
+ Gap(3, current, opponent)
+ Gap(3, current, opponent)
+ Gap(3, opponent, opponent)
```

## Result

For every `1 <= r <= 18`, both Family A and Family B are losing for the player to move. The special state C is also losing.

No winning counterexample was found in this range.

## Interpretation

The near-balanced two-gap lemma does not appear to close under one-step induction by itself. However, the exceptional responses close into a small set of auxiliary four-gap losing families.

This suggests proving the following statements simultaneously:

1. the near-balanced boundary two-gap family is losing;
2. Family A is losing for all `r`;
3. Family B is losing for all `r`;
4. the special state C is losing.

If these families can be proved together, the exceptional length-`3` side-gap cases can be absorbed into the induction rather than treated as sporadic exceptions.
