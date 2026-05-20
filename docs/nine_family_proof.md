# Nine-Family Proof Candidate

This note gives a constructive proof plan for the nine-family losing class. It
is still a proof candidate: the response tables below must be checked for every
case, but the intended form is now explicit and finite.

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

The intended theorem is:

```text
Every state in T is losing for the player to move.
```

The proof should be by induction on total empty count. The response to a move
must move to a disjoint sum of smaller states in `T`.

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

For a move at position `p` in `MM(n)`, respond at position `p` in the untouched
`OO(n)`.

Let `r = n - p - 1`. After both moves:

```text
OO(p) + MM(p) + OO(r) + MM(r).
```

This decomposes into `T2(p) + T2(r)`, omitting zero terms.

### Move in `OO(n)`

A legal move in `OO(n)` has `1 <= p <= n - 2`. Respond at position `p` in the
untouched `MM(n)`.

Let `r = n - p - 1`. After both moves:

```text
MO(p) + MO(p) + MO(r) + MO(r).
```

This decomposes into `T1(p) + T1(r)`, omitting zero terms.

## T3: `OO(n) + WM(n)`

There are two move types.

### Move in `WM(n)`

Respond in the untouched `OO(n)` at

```text
q = min(p, n - 1 - p).
```

Equivalently, respond so that the smaller side made in `WM(n)` is matched in
`OO(n)`.

The response child decomposes into one `T2` component and one smaller `T3`
component.

### Move in `OO(n)`

Respond at `q = p` in the untouched `WM(n)`.

The response child decomposes into one `T1` component and one smaller `T5`
component.

## T4: `OO(n) + WM(n + 1)`

There are two move types.

### Move in `OO(n)`

Respond in the untouched `WM(n + 1)` gap at

```text
q = n - p.
```

This is the mirror response `q = u - 1 - p`, where `u = n + 1`.

### Move in `WM(n + 1)`

Let the move position be `p`. Respond in the untouched `OO(n)` gap by:

```text
q = n - p - 1       if p <= floor(n / 2)
q = n - p           if p > floor(n / 2)
```

In the earlier length-matching notation, these are `right-mirror` in the first
case and `right-same` in the second case.

The response child decomposes into smaller components of types:

```text
T4
T2 + T4
T1 + T6
T2 + T0
T0
```

The last two outcomes occur only in small boundary cases.

## T5: `WO(n) + MO(n)`

There are two move types.

### Move in `WO(n)`

Respond at `q = p` in the untouched `MO(n)`.

The response child decomposes into `T1` and `T3` components.

### Move in `MO(n)`

Respond at

```text
q = n - 1 - p
```

in the untouched `WO(n)`.

The response child decomposes into `T2` and `T5` components.

## T6: `WO(n + 1) + MO(n)`

There are two move types.

### Move in `MO(n)`

Respond in the untouched `WO(n + 1)` gap at

```text
q = n - p.
```

This is the mirror response `q = u - 1 - p`, where `u = n + 1`.

### Move in `WO(n + 1)`

Respond at

```text
q = n - 1 - p
```

in the untouched `MO(n)`. In length-matching notation this is `right-mirror`.

The response child decomposes into smaller components of types:

```text
T6
T2 + T6
T1 + T4
T1 + T0
```

The `T0` outcome occurs only in small boundary cases.

## T7: `WM(1) + MO(n) + MO(n)`

The component `WM(1)` has no legal move. Thus any move is in one of the two
`MO(n)` components.

Respond in the untouched `MO(n)` as in `T1`, at the same position `p`.

The response child decomposes into:

```text
T0 + T1
T0 + T1 + T2
```

with zero-size components omitted.

## T8: `WM(1) + OO(n) + MM(n)`

The component `WM(1)` has no legal move. Moves occur in `OO(n)` or `MM(n)`.

Use the same-position response as in `T2`.

The response child decomposes into:

```text
T0 + T2
T0 + T1 + T1
T0 + T2 + T2
```

with zero-size components omitted.

## Remaining Work

The tables for `T1`, `T2`, `T7`, and `T8` are already explicit enough to turn
into a formal proof.

The tables for `T3` through `T6` still need full case expansion. In particular,
the phrases "using the legal one" and "decomposes into" must be replaced by
explicit inequalities on `p` and `n`, together with the exact resulting
components.

Once those four tables are expanded, the induction proof of all nine families
will be mechanical.
