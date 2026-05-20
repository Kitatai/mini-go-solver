# Patterns in Near-Balanced Boundary Responses up to total length 40

This note analyzes the response table

```text
balanced_boundary_responses_sum40.csv
```

for the near-balanced losing states

```text
Gap(ceil(s / 2), wall, opponent)
+ Gap(floor(s / 2), current, opponent).
```

## Overall Structure

For `4 <= s <= 40`, every legal move by the player to move has a winning response.

The best recorded response was chosen by minimum edge distance in its response gap. Its edge-distance distribution is:

```text
distance 0:  56
distance 1: 341
distance 2: 298
distance 3:   6
distance 4:   2
```

Thus, almost every move can be answered by a response at distance at most `2` from an edge.

## Exceptional Rows

The only rows requiring response edge distance at least `3` are:

```text
s=14  state=(7,7)    move 7:wall:opponent @3   split=(3,3)
s=15  state=(8,7)    move 8:wall:opponent @4   split=(4,3)
s=21  state=(11,10)  move 10:current:opponent @6 split=(6,3)
s=21  state=(11,10)  move 11:wall:opponent @7   split=(7,3)
s=27  state=(14,13)  move 13:current:opponent @9 split=(9,3)
s=27  state=(14,13)  move 14:wall:opponent @10  split=(10,3)
s=33  state=(17,16)  move 17:wall:opponent @13  split=(13,3)
s=37  state=(19,18)  move 19:wall:opponent @15  split=(15,3)
```

Every exceptional row has the same local feature: the move splits a gap so that one of the two newly produced gaps has length `3`.

In schematic form:

```text
before:

  boundary | . . . . . . . . . | opponent
                         ^
                         move

after:

  Gap(3, ..., opponent) + Gap(q, ..., ...) + unchanged gap
```

The exact child states in the exceptional rows are:

```text
s=14: 3:wall:opponent     3:current:opponent  7:current:opponent
s=15: 3:current:opponent  4:wall:opponent     7:current:opponent
s=21: 3:current:opponent  6:opponent:opponent 11:wall:current
s=21: 3:current:opponent  7:wall:opponent     10:current:opponent
s=27: 3:current:opponent  9:opponent:opponent 14:wall:current
s=27: 3:current:opponent  10:wall:opponent    13:current:opponent
s=33: 3:current:opponent  13:wall:opponent    16:current:opponent
s=37: 3:current:opponent  15:wall:opponent    18:current:opponent
```

## Interpretation

These are not exceptions to the near-balanced losing lemma. They are exceptions only to the stronger heuristic that a response within edge distance `2` is always enough.

The data suggests the following proof split.

1. If the player's move does not create the special length-`3` side gap, a local response at distance at most `2` should be enough.
2. If the player's move creates the length-`3` side gap, handle this as a separate finite local pattern.

The second case is still structured. The recorded responses always move to four-gap states that contain the length-`3` component and reduce the large component:

```text
s=14 -> 3,3,3,3
s=15 -> 3,3,3,4
s=21 -> 3,4,6,6 or 3,3,6,7
s=27 -> 3,4,9,9 or 3,3,9,10
s=33 -> 3,3,12,13
s=37 -> 3,3,14,15
```

This supports an induction proof that includes a small set of auxiliary losing families, rather than only the single near-balanced two-gap family.

## Current Proof Direction

The near-balanced two-gap lemma alone may be too narrow for a direct one-step induction, because some responses move to four-gap states rather than another near-balanced two-gap state.

A more realistic induction target is a finite family of losing state shapes, including:

```text
Gap(ceil(s / 2), wall, opponent)
+ Gap(floor(s / 2), current, opponent)
```

and selected four-gap states containing a length-`3` component. The response data indicates that the required auxiliary family is small and highly structured.
