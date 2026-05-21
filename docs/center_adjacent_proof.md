# Odd Center-Adjacent Losing Theorem

This note proves the observed losing rule for the first moves adjacent to the
center on odd boards, in the reduced gap game.

## Statement

Let the board length be `N = 2k + 1` with `k >= 2`. If Black opens at `k - 1`
or `k + 1`, then the move is losing.

By reflection it is enough to consider the first move `k - 1`.

After this move, White sees:

```text
WO(k - 1) + WO(k + 1).
```

White responds at the reflected point `k + 1`. In the larger `WO(k + 1)` gap,
this is the canonical coordinate `k - 1`. After the turn pass, Black receives:

```text
R(k - 1) = MO(1) + WM(k - 1) + WO(k - 1).
```

It remains to prove that `R(a)` is losing for every `a >= 1`.

## The `R(a)` Lemma

Define:

```text
R(a) = MO(1) + WM(a) + WO(a).
```

Then `R(a)` is losing for every `a >= 1`.

Proof is by induction on `a`.

For `a = 1`, all three components have no legal move:

- `MO(1)` is adjacent to an opponent boundary;
- `WM(1)` is adjacent to a wall boundary;
- `WO(1)` is adjacent to both a wall and an opponent boundary.

Thus `R(1)` is losing.

Assume `a >= 2`.

The component `MO(1)` has no legal move. Therefore a legal move is in either
`WM(a)` or `WO(a)`.

### Move in `WM(a)`

A legal move in `WM(a)` has `1 <= p <= a - 1`.

Respond in the paired `WO(a)` component at the same canonical position `p`.
After the move and response, the child is:

```text
R(p) + T2(a - p - 1).
```

Here `T2(0)` is omitted. Since `p < a`, the `R(p)` component is smaller, and
`T2` is losing by the nine-family theorem.

### Move in `WO(a)`

A legal move in `WO(a)` has `1 <= p <= a - 2`.

Respond in the paired `WM(a)` component at the same canonical position `p`.
After the move and response, the child is:

```text
R(p) + T1(a - p - 1).
```

Again `p < a`, so `R(p)` is smaller, and `T1` is losing by the nine-family
theorem.

Thus every legal move from `R(a)` can be answered by a move to a finite sum of
smaller losing states. Therefore `R(a)` is losing for every `a >= 1`.

Consequently, White's reflected response to the first move `k - 1` sends Black
to `R(k - 1)`, a losing state. The first move `k - 1` is losing. By reflection,
the first move `k + 1` is also losing.

This proves the odd center-adjacent losing rule for every odd `N >= 5`.
