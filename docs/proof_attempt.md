# Near-Edge Losing Rule and Open Classification

This note records the proof structure for the near-edge losing rule and
separates it from the still-open full first-move classification.

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

Exact computation first identified that `P(s)` is losing for `2 <= s <= 40`;
the nine-family theorem below upgrades this to all `s >= 2` in the reduced gap
game.

## Target Chain

The near-edge losing rule follows from the following chain:

```text
P(s) is losing for all s >= 2
        =>
Gap(m, opponent, wall) is winning for m >= 3
        =>
initial moves i = 1 and i = N - 2 are losing,
using the extra one-cell edge gap as a T0 component.
```

The implication from `P(s)` to `Gap(m, opponent, wall)` is by a central move:
the current player moves in `Gap(m, opponent, wall)` so that the opponent
receives the near-balanced state `P(m - 1)`.

For the actual initial move `i = 1`, the opponent sees `WO(1) + WO(N - 2)`,
not just the large one-gap state. The near-center response in `WO(N - 2)`
leaves `WM(1) + P(N - 3)`, so the proof needs the strengthened finite-sum
form of the nine-family theorem.

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

The finite structural class that closes the response argument is the
nine-family class. It adds the base state

```text
WM(1)
```

and the two `WM(1)`-decorated families

```text
WM(1) + MO(n) + MO(n)
WM(1) + OO(n) + MM(n)
```

to the previous six families.

The detailed proof is now written in `docs/nine_family_proof.md`. It gives
explicit response positions and decompositions for every family and proves the
finite-sum induction form needed to keep one-cell components such as `WM(1)`.

The earlier DP closure check for `1 <= n <= 10` remains useful as an
independent guard against table mistakes: every legal move from one of these
families had a response whose child decomposed into smaller members of the same
nine-family class.

## Current Invariant

The current invariant is the finite disjoint sum of nine-family states. It has
these features:

1. Moves are considered only in effective gaps after excluding immediate
   losing contact and edge moves.
2. A move in one component is answered in a paired untouched component from the
   same family.
3. The response matches one of the sublengths created by the move.
4. One-cell components must not be discarded merely because they have no
   current legal move; their behavior after color flip matters.
5. The response child may contain `WM(1)`-decorated smaller families.

## Current Status

What is currently established by proof:

```text
non-capturing contact with opponent chain is losing
non-capturing edge move is losing
odd-board center first move is winning
the nine-family finite-sum theorem in the reduced gap game
P(s) is losing for all s >= 2 in the reduced gap game
initial moves i = 1 and i = N - 2 are losing for N >= 5
odd-board center-adjacent first moves are losing for N >= 5
```

What is currently established by exact computation:

```text
P(s) is losing for 2 <= s <= 40
the length-matching untouched-gap response exists for all moves from P(s), s <= 40
the six response families are losing for 1 <= n <= 18
the six response families have untouched-gap responses for all moves, n <= 18
the nine-family closure table has no misses for 1 <= n <= 10
```

What remains open:

1. Turn the reduced-gap-game proof into a polished presentation that explicitly
   cites the reduction from the original game.
2. Determine whether the observed full first-move classification for `N >= 20`
   is always true.
3. If the full classification is true, find a separate argument for the
   remaining winning first moves.

## Reformulation of the Full Classification

After Black opens at position `i`, with `1 <= i <= N - 2`, White sees:

```text
A(i, N - i - 1) = WO(i) + WO(N - i - 1).
```

Therefore Black's first move is winning exactly when this two-gap state is
losing for the player to move.

The proved losing first moves correspond to winning `A`-states for White:

```text
i = 1 or N - 2:
  A(1, N - 2) is winning for White.

N = 2k + 1 and i = k - 1 or k + 1:
  A(k - 1, k + 1) is winning for White.
```

The center first move on odd boards says:

```text
A(k, k) is losing for White.
```

Thus the remaining classification problem is to prove that `A(a, b)` is losing
for the player to move for the remaining observed winning first moves. This is
the dual direction to the losing-move theorems above.

## Odd Center-Adjacent Moves

Let `N = 2k + 1`. If Black opens at the left neighbor of the center, position
`k - 1`, then White sees the two-gap state

```text
WO(k - 1) + WO(k + 1).
```

This is the structural form behind the loss of the center-adjacent first moves
on odd boards. It is not covered directly by the nine-family theorem: after a
White move at position `p` in the larger gap, Black receives

```text
WM(k - 1) + WO(p) + MO(k - p).
```

The useful response is the reflected point, corresponding to `p = k - 1`.
Then Black receives

```text
R(k - 1) = MO(1) + WM(k - 1) + WO(k - 1).
```

The family

```text
R(a) = MO(1) + WM(a) + WO(a)
```

is losing for all `a >= 1`: moves in `WM(a)` are answered in `WO(a)` at the
same position, giving `R(p) + T2(a - p - 1)`, and moves in `WO(a)` are answered
in `WM(a)` at the same position, giving `R(p) + T1(a - p - 1)`.

The detailed proof is written in `docs/center_adjacent_proof.md`.
