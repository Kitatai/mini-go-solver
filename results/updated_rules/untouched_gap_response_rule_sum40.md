# Untouched-Gap Response Rule up to total length 40

This note analyzes the all-response table

```text
balanced_boundary_all_responses_sum40.csv
```

for the near-balanced boundary states

```text
P(s) =
Gap(ceil(s / 2), wall, opponent)
+ Gap(floor(s / 2), current, opponent).
```

## Candidate Rule

Suppose the player to move plays at position `p` in one original gap of length `m`.

This splits that gap into lengths

```text
p
m - p - 1
```

Let the untouched original gap have length `u`.

The checked data shows that there is always a winning response in the untouched gap at one of the positions

```text
p
u - 1 - p
m - p - 1
u - 1 - (m - p - 1)
```

when the position is legal inside the untouched gap.

Equivalently, the response can be chosen so that one of the two lengths created in the untouched gap is equal to one of the two lengths created by the opponent's move.

This held for all 703 legal moves from `P(s)` with `s <= 40`.

## Coverage

For each legal move, the four candidate forms above were tested against the all-winning-response table. No miss was found.

The number of moves covered by each candidate form was:

```text
q = p:                         359
q = u - 1 - p:                 529
q = m - p - 1:                 445
q = u - 1 - (m - p - 1):       529
```

These counts overlap because many moves have more than one winning response of this form.

One deterministic priority order that covers all checked moves is:

```text
1. q = u - 1 - (m - p - 1)
2. q = m - p - 1
3. q = u - 1 - p
4. q = p
```

Using this priority order, the selected cases were:

```text
q = u - 1 - (m - p - 1): 529
q = m - p - 1:           110
q = u - 1 - p:            64
q = p:                     0
```

## Interpretation

The response is not a pure mirror of the move position. It is a mirror of one of the two sublengths created by the move.

This is a stronger and more structured statement than the earlier untouched-gap lemma. It may be the right form for a proof:

```text
match one of the two newly created lengths in the other original gap.
```

The remaining proof task is to show that at least one of these length-matching responses is always legal and moves to a smaller losing state in the intended induction class.
