# Nine-Family Losing Theorem

This note proves a nine-family losing theorem in the reduced gap game obtained
after applying the non-capturing contact and edge-losing principles. The proof
is constructive: every legal move from one of the listed states has an explicit
response that returns to a smaller finite disjoint sum of listed states.

## Notation

Use relative boundary notation:

```text
WM(n) = Gap(n, wall, current)
WO(n) = Gap(n, wall, opponent)
MM(n) = Gap(n, current, current)
MO(n) = Gap(n, current, opponent)
OO(n) = Gap(n, opponent, opponent)
```

Let `T` be the following class of losing states:

```text
T0:  WM(1)
T1:  MO(n) + MO(n)
T2:  OO(n) + MM(n)
T3:  OO(n) + WM(n)
T4:  OO(n) + WM(n + 1)
T5:  WO(n) + MO(n)
T6:  WO(n + 1) + MO(n)
T7:  WM(1) + MO(n) + MO(n)
T8:  WM(1) + OO(n) + MM(n)
```

Here `n >= 1`.

The theorem is:

```text
Every state in T is losing for the player to move.
```

The proof is by induction on total empty count, strengthened to all finite
disjoint sums of states in `T`. The response to a move in one component moves
that `T`-component to a disjoint sum of smaller states in `T`; untouched
`T`-components are unchanged after the two-ply move-response pair.

## Basic Split Rule

If the current player moves at position `p` in `Gap(m, L, R)`, write

```text
r = m - p - 1.
```

After the move and turn pass, the selected gap contributes:

```text
Gap(p, flip(L), opponent) + Gap(r, opponent, flip(R))
```

where zero-length gaps are omitted. All gaps are canonicalized by left-right
reflection.

This rule is the only local transition rule used below.

## Coordinate Convention

The state is normalized after each turn pass. Therefore the response tables use
the responder's normalized coordinates, not the physical coordinates before the
first move.

If a physical gap is the left-right reflection of the displayed component, the
coordinate is reflected with it during normalization.

For example, after a turn pass, an untouched physical `MO(n)` component becomes
`OM(n)`. Since the canonical representative is again `MO(n)`, responder
coordinate `q` in the displayed `MO(n)` component means physical coordinate
`n - 1 - q` in the reflected `OM(n)` gap.

Similarly, a component that was `OO(n)` before the first move is seen as
`MM(n)` after the turn pass. The response descriptions below name this
responder-visible component when the distinction matters.

## T1: `MO(n) + MO(n)`

By symmetry, consider a move in one `MO(n)` at position `p`. The move is legal
for `0 <= p <= n - 2`.

Respond at the same position `p` in the untouched `MO(n)`.

Let

```text
r = n - p - 1.
```

After both moves, the state is:

```text
OO(p) + MM(p) + MO(r) + MO(r).
```

Zero-length terms are omitted. Therefore:

- if `p = 0`, the child is `T1(r)`;
- if `r = 0`, the child is `T2(p)`;
- otherwise, the child is `T2(p) + T1(r)`.

All components have smaller total empty count than the original state.

## T2: `OO(n) + MM(n)`

There are two move types.

### Move in `MM(n)`

For a move at position `p` in `MM(n)`, respond at position `p` in the component
that came from the untouched `OO(n)`. The responder sees it as `MM(n)`.

Let `r = n - p - 1`. After both moves:

```text
OO(p) + MM(p) + OO(r) + MM(r).
```

This decomposes into `T2(p) + T2(r)`, omitting zero terms.

### Move in `OO(n)`

A legal move in `OO(n)` has `1 <= p <= n - 2`. Respond at position `p` in the
component that came from the untouched `MM(n)`. The responder sees it as
`OO(n)`.

Let `r = n - p - 1`. After both moves:

```text
MO(p) + MO(p) + MO(r) + MO(r).
```

This decomposes into `T1(p) + T1(r)`, omitting zero terms.

## T3: `OO(n) + WM(n)`

There are two move types.

### Move in `WM(n)`

Respond in the component that came from the untouched `OO(n)`. The responder
sees it as `MM(n)`. Use position

```text
q = min(p, n - 1 - p).
```

Equivalently, respond so that the smaller side made in `WM(n)` is matched in
`OO(n)`.

The response child decomposes into one `T2` component and one smaller `T3`
component.

More explicitly, let `r = n - p - 1`.

If `p <= r`, then `q = p`, and the child is:

```text
T3(p) + T2(r).
```

If `p > r`, then `q = r`, and the child is:

```text
T2(r) + T3(p).
```

### Move in `OO(n)`

Respond at `q = p` in the component that came from the untouched `WM(n)`. The
responder sees it as `WO(n)`.

The response child decomposes into one `T1` component and one smaller `T5`
component.

With `r = n - p - 1`, the child is:

```text
T5(p) + T1(r).
```

## T4: `OO(n) + WM(n + 1)`

There are two move types.

### Move in `OO(n)`

Respond in the component that came from the untouched `WM(n + 1)` gap. The
responder sees it as `WO(n + 1)`. Use position

```text
q = n - p.
```

This is the mirror response `q = u - 1 - p`, where `u = n + 1`.

With `r = n - p - 1`, the child is:

```text
T1(p) + T6(r).
```

### Move in `WM(n + 1)`

Let the move position be `p`. Respond in the component that came from the
untouched `OO(n)` gap. The responder sees it as `MM(n)`. Use:

```text
q = p - 1           if p <= floor(n / 2)
q = n - p           if p > floor(n / 2)
```

In the earlier length-matching notation, these are `right-mirror` in the first
case and `right-same` in the second case.

Let `r = n - p`. The child is:

```text
T4(p - 1) + T2(r).
```

Here `T4(0)` means `WM(1)`, i.e. `T0`, and `T2(0)` is omitted.

## T5: `WO(n) + MO(n)`

There are two move types.

### Move in `WO(n)`

Respond at `q = p` in the component that came from the untouched `MO(n)`. After
normalization the responder sees it as `MO(n)`.

The response child decomposes into `T1` and `T3` components.

With `r = n - p - 1`, the child is:

```text
T3(p) + T1(r).
```

### Move in `MO(n)`

Respond at

```text
q = n - 1 - p
```

in the component that came from the untouched `WO(n)`. The responder sees it as
`WM(n)`.

The response child decomposes into `T2` and `T5` components.

With `r = n - p - 1`, the child is:

```text
T2(p) + T5(r).
```

## T6: `WO(n + 1) + MO(n)`

There are two move types.

### Move in `MO(n)`

Respond in the component that came from the untouched `WO(n + 1)` gap. The
responder sees it as `WM(n + 1)`. Use position

```text
q = n - p.
```

This is the mirror response `q = u - 1 - p`, where `u = n + 1`.

With `r = n - p - 1`, the child is:

```text
T2(p) + T6(r).
```

### Move in `WO(n + 1)`

Respond at

```text
q = p - 1
```

in the component that came from the untouched `MO(n)`. After normalization the
responder sees it as `MO(n)`. In length-matching notation this is
`right-mirror`.

Let `r = n - p`. The child is:

```text
T4(p - 1) + T1(r).
```

Here `T4(0)` means `WM(1)`, i.e. `T0`.

## T7: `WM(1) + MO(n) + MO(n)`

The component `WM(1)` has no legal move. If `n = 1`, the two `MO(1)`
components also have no legal move. Thus the state is immediately losing.

Assume `n >= 2`. Any move is in one of the two `MO(n)` components.

Respond in the untouched `MO(n)` as in `T1`, at the same position `p`.

Let `r = n - p - 1`. Since the move in `MO(n)` is legal, `0 <= p <= n - 2`,
so `r >= 1`.

After both moves, the response child is:

```text
T0 + OO(p) + MM(p) + MO(r) + MO(r).
```

Therefore:

- if `p = 0`, the child is `T7(r)`;
- if `p > 0`, the child is `T8(p) + T1(r)`.

## T8: `WM(1) + OO(n) + MM(n)`

The component `WM(1)` has no legal move. Moves occur in `OO(n)` or `MM(n)`.

Use the same-position response as in `T2`.

### Move in `MM(n)`

Respond at position `p` in the component that came from the untouched `OO(n)`.
The responder sees it as `MM(n)`.

Let `r = n - p - 1`. After both moves, the response child is:

```text
T0 + OO(p) + MM(p) + OO(r) + MM(r).
```

Therefore:

- if `p = 0` and `r = 0`, the child is `T0`;
- if `p = 0` and `r > 0`, the child is `T8(r)`;
- if `p > 0` and `r = 0`, the child is `T8(p)`;
- if `p > 0` and `r > 0`, the child is `T8(p) + T2(r)`.

### Move in `OO(n)`

A legal move in `OO(n)` has `1 <= p <= n - 2`, so both `p` and
`r = n - p - 1` are positive.

Respond at position `p` in the component that came from the untouched `MM(n)`.
The responder sees it as `OO(n)`. After both moves, the response child is:

```text
T0 + MO(p) + MO(p) + MO(r) + MO(r).
```

This is `T7(p) + T1(r)`.

## Closure Conditions

The response tables above satisfy the following closure conditions:

- every displayed response position `q` is inside the untouched gap;
- every displayed response is legal;
- after the move and response, the moved component is replaced by a finite
  disjoint sum of states in `T`;
- each replacement component has smaller total empty count than the original
  moved component;
- `T4(0)` is interpreted as `T0`;
- zero-length components are omitted.

These conditions are exactly what is needed for induction on total empty count
for finite disjoint sums of `T`-states.

## Legality Bounds

The displayed responses satisfy the required bounds under the legal-move
conditions for each gap:

- in `MO(n)`, legal moves have `0 <= p <= n - 2`;
- in `OO(n)`, legal moves have `1 <= p <= n - 2`;
- in `WM(n)`, legal moves have `1 <= p <= n - 1`;
- in `WO(n)`, legal moves have `1 <= p <= n - 2`;
- in `MM(n)`, all positions are legal, but by reflection it is enough to check
  the canonical half.

These bounds are for the displayed canonical component. If the physical
response gap is reflected before canonicalization, the reflected physical
coordinate is used, as described above.

For `T4`, a move in `WM(n + 1)` has `1 <= p <= n`. The response is:

```text
q = p - 1       if p <= floor(n / 2)
q = n - p       if p > floor(n / 2).
```

In both cases `0 <= q <= n - 1`, so the response lies in the untouched
`OO(n)` gap.

For `T6`, a move in `WO(n + 1)` has `1 <= p <= n - 1`. The response

```text
q = p - 1
```

satisfies `0 <= q <= n - 2`, so it lies in the untouched `MO(n)` gap.

## Induction Form

Let `E(S)` be the total number of empty points in a state `S`.

Use the following stronger statement.

```text
Q(e): every finite disjoint sum of T-states with total empty count e is losing.
```

The proof proceeds by induction on `e`.

Take a state

```text
S = C1 + ... + Ck
```

where every `Ci` belongs to `T`. If all components are `T0`, there is no legal
move.

Otherwise, any legal move occurs inside one `T`-component, say `Ci`. Use the
response specified in the table for the family containing `Ci`; the response is
also played inside `Ci`, although it may be in a different gap of that
`T`-component. After the move and response, the turn returns to the original
orientation. Therefore all untouched `T`-components `Cj (j != i)` have the same
relative boundary labels as before, while `Ci` has been replaced by a finite
disjoint sum of `T`-states. The total empty count has decreased by two, so the
resulting state satisfies the induction hypothesis and is losing for the next
player.

Thus every first move from `S` can be answered by a move to a smaller losing
state. Hence `S` is losing. Applying this to the one-component sums proves
that each state in `T0..T8` is losing.

This is the required disjoint-sum step for this normal-play setting. The
important point is that the response is always made in the same `T`-component
where the first move occurred, so all other `T`-components are flipped twice
and return to their original relative labels.

## Consequence for `P(s)`

The near-balanced state `P(s)` is one of the nine families:

```text
P(2n)     = WO(n) + MO(n)       = T5(n)
P(2n + 1) = WO(n + 1) + MO(n)   = T6(n).
```

Therefore, the nine-family theorem implies that `P(s)` is losing for every
`s >= 2`.

Consequently, for `m >= 3`, the one-gap state

```text
Gap(m, opponent, wall)
```

is winning for the player to move: play near the center so that the opponent
receives `P(m - 1)`.

For the initial near-edge move, however, one must not discard the one-cell edge
gap. If Black opens at `i = 1`, then White sees:

```text
WO(1) + WO(N - 2)
```

up to reflection. White plays in the large `WO(N - 2)` gap at the near-center
position `p = ceil((N - 3) / 2)`. After the turn pass, Black receives:

```text
WM(1) + WO(ceil((N - 3) / 2)) + MO(floor((N - 3) / 2)).
```

This is `T0 + T5` when `N - 3` is even, and `T0 + T6` when `N - 3` is odd.
The strengthened finite-sum theorem therefore makes it losing for Black.

By reflection, the same argument applies to `i = N - 2`. Thus the nine-family
theorem proves the near-edge losing rule:

```text
initial moves i = 1 and i = N - 2 are losing for N >= 5.
```

It does not prove the full observed classification for all first moves.
