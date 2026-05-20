# All Winning Responses for Near-Balanced Boundary States up to total length 40

This file summarizes the all-response table

```text
balanced_boundary_all_responses_sum40.csv
```

for near-balanced boundary states

```text
Gap(ceil(s / 2), wall, opponent)
+ Gap(floor(s / 2), current, opponent).
```

Computation:

```text
states=60685394
wall time=1:52.21
max RSS=4671752 KB
```

## Main Finding

For every legal move from these near-balanced states with `s <= 40`, there exists a winning response in the gap that was not touched by the move.

Equivalently, if the player to move plays in one of the two original gaps, the opponent can answer in the other original gap.

This held for all 703 legal moves in the checked range.

This is stronger and more proof-oriented than merely knowing that some winning response exists.

## Failed Simpler Rules

The response position inside the untouched gap is not given by a simple universal rule.

The following candidate rules fail in the checked range:

- always play the same relative position;
- always play the reflected relative position;
- always play position `2`;
- always play within a fixed small set such as `{1,2,3}`.

Therefore, the proof still needs a boundary-sensitive local response rule. However, it may be enough to prove such a rule only inside the untouched opposite gap.

## Proof Direction

The near-balanced two-gap lemma can now be targeted in the following form.

```text
Given P(s), after any legal move in one gap,
there is a response in the other original gap
that moves to a smaller losing state.
```

This removes the need to search all gaps for the response and suggests that the two original gaps remain strategically paired, although not by a simple mirror map.
