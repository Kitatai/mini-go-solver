# Proof Attempt for the Near-Edge Losing Rule

This note records the current proof structure. It is not a completed proof.

## Notation

A gap is written as `Gap(m, L, R)`, where `m` is the number of empty points and
`L`, `R` are the boundary types. In relative notation:

```text
WO = Gap(length, wall, opponent)
WM = Gap(length, wall, current)
MM = Gap(length, current, current)
MO = Gap(length, current, opponent)
OO = Gap(length, opponent, opponent)
```

The main two-gap state is

```text
P(a, b) = WO(a) + MO(b),  where a = b or a = b + 1.
```

Equivalently, for total empty count `s`,

```text
P(s) = WO(ceil(s / 2)) + MO(floor(s / 2)).
```

Exact computation confirms that `P(s)` is losing for `2 <= s <= 40`.

## Target Chain

The near-edge losing rule follows from the following chain:

```text
P(s) is losing for all s >= 2
        =>
Gap(m, opponent, wall) is winning for m >= 3
        =>
initial moves i = 1 and i = N - 2 are losing.
```

The implication from `P(s)` to `Gap(m, opponent, wall)` is by a central move:
the current player moves in `Gap(m, opponent, wall)` so that the opponent
receives the near-balanced state `P(m - 1)`.

## Established Local Facts

The following local facts are already proved in `docs/gap_game.md`.

1. A non-capturing move adjacent to an opponent chain is losing.
2. A non-capturing move at a board edge is losing.
3. On an odd board, the center first move is winning by reflection.

Thus the proof search can ignore non-capturing contact moves and non-capturing
edge moves, except when they occur as terminal losing moves.

## Response Rule for P

Let the first move in one original gap have length `m` and position `p`. It
splits that gap into lengths

```text
p
m - p - 1.
```

Let the untouched original gap have length `u`. For every legal move from
`P(s)` with `s <= 40`, a winning response exists in the untouched gap at a
position that matches one of those two lengths:

```text
q = p
q = u - 1 - p
q = m - p - 1
q = u - 1 - (m - p - 1)
```

For `P(a,b) = WO(a) + MO(b)`, the checked range admits an even simpler rule.

If the first move is in `WO(a)` at position `p`, respond in the untouched
`MO(b)` gap at

```text
q = b - a + p.
```

This matches the right sublengths and creates

```text
MO(a - p - 1) + MO(a - p - 1)
```

together with one of:

```text
OO(p) + WM(p)       when a = b,
OO(p - 1) + WM(p)   when a = b + 1.
```

If the first move is in `MO(b)` at position `p`, respond in the untouched
`WO(a)` gap at

```text
q = a - 1 - p.
```

This matches the left sublengths and creates

```text
OO(p) + MM(p)
```

together with a smaller `P`-type residual:

```text
WO(b - p - 1) + MO(b - p - 1)       when a = b,
WO(b - p) + MO(b - p - 1)           when a = b + 1.
```

This explains why the following six losing families appear naturally.

## Six Response Families

The response rule for `P` leads to these six two-gap families:

```text
MO(n) + MO(n)
OO(n) + MM(n)
OO(n) + WM(n)
OO(n) + WM(n + 1)
WO(n) + MO(n)
WO(n + 1) + MO(n)
```

Exact checks confirm all six are losing for `1 <= n <= 18`. Full response
analysis also shows that every legal move from these families has a winning
response in the untouched original gap.

The required response forms in the checked range are:

```text
MO(n) + MO(n):          q = p
OO(n) + MM(n):          q = p
OO(n) + WM(n):          q = p or q = u - 1 - p
OO(n) + WM(n + 1):      q = m - p - 1 or q = u - 1 - (m - p - 1)
WO(n) + MO(n):          q = p or q = u - 1 - p
WO(n + 1) + MO(n):      q = u - 1 - p or q = u - 1 - (m - p - 1)
```

Here `m` is the length of the gap where the first move was played, `u` is the
length of the untouched gap, `p` is the first move position, and `q` is the
response position.

This is a response-location theorem candidate: it identifies where a response
can be found. It is not yet a closed induction proof.

## Closure Obstruction and Refinement

The six families alone are not a closed induction class.

For example, from

```text
WO(8) + MO(8)
```

a move in the `MO(8)` gap at position `1` has winning responses whose children
include:

```text
WO(1) + MM(1) + MO(6) + OO(6)
```

The obstruction is the pair `WO(1) + MM(1)`, not just `MM(1)` alone. It cannot
be deleted merely because `WO(1)` has no legal move for the current player.
In the relative-color gap representation, a gap that has no legal move now can
become active after a move elsewhere, because the turn pass flips
`current/opponent` boundaries.

Similar active remnants include:

```text
MM(1)
MM(1) + MO(k) + OO(l)
WM(2)
WO(2)
```

These remnants occur inside losing response children, but they are not
represented by the six-family list. A complete induction must therefore use a
larger structural losing class.

The next refinement is a nine-family candidate. Add the base state

```text
WM(1)
```

and the two `WM(1)`-decorated families

```text
WM(1) + MO(n) + MO(n)
WM(1) + OO(n) + MM(n)
```

to the previous six families.

An independent DP closure check for `1 <= n <= 10` found no misses for this
nine-family class: every legal move from one of these families has a winning
response whose child decomposes into smaller members of the same nine-family
class.

This is now the most promising finite induction candidate.

## Current Invariant Candidate

The current candidate is the nine-family class above, or a structural class
generated by it. It has these features:

1. Moves are considered only in effective gaps after excluding immediate
   losing contact and edge moves.
2. A move in one component should be answered in a paired untouched component.
3. The response should match one of the sublengths created by the move.
4. One-cell components must not be discarded merely because they have no
   current legal move; their behavior after color flip matters.
5. The response child may contain `WM(1)`-decorated smaller families.

The missing step is to state this structural class precisely enough that it is
closed under all legal moves and the length-matching response.

The current detailed version of this candidate is written in
`docs/nine_family_proof.md`. There the response positions and decompositions are
expanded case by case.

## Current Status

What is currently established by proof:

```text
non-capturing contact with opponent chain is losing
non-capturing edge move is losing
odd-board center first move is winning
```

What is currently established by exact computation:

```text
P(s) is losing for 2 <= s <= 40
the length-matching untouched-gap response exists for all moves from P(s), s <= 40
the six response families are losing for 1 <= n <= 18
the six response families have untouched-gap responses for all moves, n <= 18
the nine-family closure candidate has no misses for 1 <= n <= 10
```

What remains to complete the proof:

1. Define the larger structural losing class that includes the six families and
   their active remnants.
2. Prove that every legal move from a state in this class has a legal
   length-matching response.
3. Prove that the response child is a smaller state in the same class.
4. Deduce `P(s)` for all `s`, and hence the near-edge losing rule.
