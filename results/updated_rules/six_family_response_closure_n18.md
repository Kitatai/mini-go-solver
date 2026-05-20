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

Some response children contain one-cell gaps with no legal moves, such as
`Gap(1, wall, current)`. These are zero components in normal-play terms and can
be ignored in a decomposition, but the proof should state this explicitly.

## Current Proof Target

The remaining task is to prove the six response rules above symbolically.

The desired proof form is:

1. State the six families as losing families.
2. For each family, take an arbitrary legal move.
3. Respond in the untouched gap using one of the listed formulas.
4. Show the response is legal.
5. Show the resulting state is a disjoint sum of smaller six-family states and
   zero one-cell gaps.

The checked data supports this plan through `n = 18`.
