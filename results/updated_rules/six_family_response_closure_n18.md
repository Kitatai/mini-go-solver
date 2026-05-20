# Six-Family Response Closure up to n = 18

This note checks the six-family mutual-induction candidate:

```text
MO(n) + MO(n)
OO(n) + MM(n)
OO(n) + WM(n)
OO(n) + WM(n + 1)
WO(n) + MO(n)
WO(n + 1) + MO(n)
```

where

```text
WO = wall-opponent
WM = wall-current
MM = current-current
MO = current-opponent
OO = opponent-opponent
```

The full winning-response table was generated for all legal moves in these
families with `1 <= n <= 18`.

```text
response rows: 8,858
legal move groups: 1,378
states: 27,962,589
wall time: 45.84 s
maximum RSS: 2,294,488 KB
```

## Untouched-Gap Responses

For every legal move in the checked range, there is a winning response in the
other original gap, the one not touched by the move.

Thus the same strategic shape seen for `P(s)` persists in the six-family
candidate: answer in the paired gap and match a sublength.

The tested response forms were:

```text
same:          q = p
mirror:        q = u - 1 - p
right-same:    q = m - p - 1
right-mirror:  q = u - 1 - (m - p - 1)
```

Here `m` is the length of the gap where the first move was played, `u` is the
length of the untouched gap, and `p` and `q` are positions in those gaps.

## Minimal Forms by Family

In the checked range, each family is covered by the following response forms:

```text
MO(n) + MO(n):          same
OO(n) + MM(n):          same
OO(n) + WM(n):          same or mirror
OO(n) + WM(n + 1):      right-same or right-mirror
WO(n) + MO(n):          same or mirror
WO(n + 1) + MO(n):      mirror or right-mirror
```

So no family currently requires more than two response formulas.

## Decomposition Note

If one chooses an arbitrary best response by edge distance, the response child
does not always decompose directly into the six families. This is not a
contradiction: edge-nearest winning responses are not necessarily the responses
needed for a clean proof.

When all winning responses are considered, a clean untouched-gap response exists
for every checked move.

One-cell gaps require care. A one-cell gap may have no legal move for the
current player, but it is not automatically a removable zero component in the
relative-color representation. After a move elsewhere, the turn pass flips
`current/opponent` boundaries, and the same gap may become active.

## Closure Obstruction

The six families are not yet a complete induction class if the proof requires
the response child to decompose directly into a disjoint sum of the same six
families.

For example, from

```text
WO(8) + MO(8)
```

a move in the `MO(8)` gap at position `1` has winning responses, but typical
response children include forms such as:

```text
WO(1) + MM(1) + MO(6) + OO(6)
```

The obstruction is the pair `WO(1) + MM(1)`. The `WO(1)` component has no legal
move for the current player, but it cannot simply be discarded before analyzing
the turn-flip behavior of the whole state.

Similarly, response children of the six families may contain active small
components such as:

```text
MM(1)
MM(1) + MO(k) + OO(l)
WM(2)
WO(2)
```

These are losing in their full context, but they are not represented by the
six-family list.

Thus the current six-family statement is best understood as a response-location
theorem, not yet as a closed induction proof.

## Nine-Family Refinement

The six-family obstruction is largely repaired by adding:

```text
WM(1)
WM(1) + MO(n) + MO(n)
WM(1) + OO(n) + MM(n)
```

to the original six families.

An independent DP closure check for `1 <= n <= 10` found no misses for this
nine-family class. Every legal move from one of the nine families has a winning
response whose child decomposes into smaller nine-family members.

This makes the nine-family class the strongest current finite induction
candidate.

## Current Proof Target

The remaining task is to prove the nine-family response rules symbolically.

The desired proof form is:

1. State the nine families as losing families.
2. For each family, take an arbitrary legal move.
3. Respond in the untouched gap using one of the listed formulas.
4. Show the response is legal.
5. Show the resulting state belongs to a larger structural losing class.

The checked data supports the response-location part through `n = 18`.

The proof should show that this class is closed under the same untouched-gap
response principle, with `WM(1)` as a base losing state.
