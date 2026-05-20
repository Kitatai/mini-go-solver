# Length-Matching Child Structure up to total length 40

This note refines the length-matching response rule for the near-balanced
boundary states

```text
P(s) =
Gap(ceil(s / 2), wall, opponent)
+ Gap(floor(s / 2), current, opponent).
```

The all-winning-response table up to `s = 40` shows that a response can always
be chosen in the untouched original gap so that one of the two sublengths made
by the response equals one of the two sublengths made by the first move.

This refinement records what that response creates after both players have
moved and the turn has returned to the original player.

## Notation

Write

```text
WO = Gap(length, wall, opponent)
WM = Gap(length, wall, current)
MM = Gap(length, current, current)
MO = Gap(length, current, opponent)
OO = Gap(length, opponent, opponent)
```

Suppose the first move is at position `p` in a gap of length `m`, so the first
move creates sublengths

```text
left  = p
right = m - p - 1
```

Let the untouched gap have length `u`.

The checked candidate response positions are:

```text
ll: q = p
lr: q = u - 1 - p
rl: q = m - p - 1
rr: q = u - 1 - (m - p - 1)
```

The two letters indicate which side length is matched: for example `rr` matches
the right sublength of the first move with the right sublength of the response.

## Matched Pairs Created

If the first move is in the `MO` gap of `P(s)`, then after the response and turn
pass, the four possible matching forms create the following equal-length pair:

```text
rr: OO + MO, with common length right
rl: WO + MO, with common length right
lr: OO + MM, with common length left
ll: WO + MM, with common length left
```

If the first move is in the `WO` gap of `P(s)`, then the matching forms create:

```text
rr: MO + MO, with common length right
rl: OO + MO, with common length right
lr: MO + WM, with common length left
ll: OO + WM, with common length left
```

In the checked range, using the deterministic priority order

```text
rr, rl, lr, ll
```

all 703 legal moves from `P(s)` with `s <= 40` were covered, and every selected
response child contained the predicted equal-length pair.

The selected cases were:

```text
first move in WO gap, rr: 342
first move in MO gap, rr: 187
first move in MO gap, rl: 110
first move in MO gap, lr:  64
```

There were no selected `ll` cases in this priority order.

A simpler rule also covers all checked moves:

```text
If the first move is in the WO gap, respond by rr in the untouched MO gap.
If the first move is in the MO gap, respond by lr in the untouched WO gap.
```

This had no misses among the 703 legal moves from `P(s)` with `s <= 40`.

Under this simpler rule:

- a move in the `WO` gap creates an equal-length `MO + MO` pair and leaves an
  `OO + WM` residual whose lengths differ by at most one;
- a move in the `MO` gap creates an equal-length `OO + MM` pair and leaves a
  smaller `WO + MO` residual whose lengths differ by at most one.

This is a much more useful induction shape than the priority rule.

## Small Family Checks

The simpler response rule suggests the following losing component families:

```text
MO(n) + MO(n)
OO(n) + MM(n)
OO(n) + WM(n)
OO(n) + WM(n + 1)
WO(n) + MO(n)
WO(n + 1) + MO(n)
```

An exact independent DP check for `1 <= n <= 13` gave:

```text
OO(n) + WM(n):       LLLLLLLLLLLLL
OO(n) + WM(n + 1):   LLLLLLLLLLLLL
OO(n) + MM(n):       LLLLLLLLLLLLL
WO(n) + MO(n):       LLLLLLLLLLLLL
WO(n + 1) + MO(n):   LLLLLLLLLLLLL
MO(n) + MO(n):       LLLLLLLLLLLLL
```

Two nearby families are not losing in general:

```text
WO(n) + MM(n):       LWWWWWWWWWWWW
MO(n) + WM(n):       LWWWWWWWWWWWW
```

So the boundary labels in the candidate induction are essential.

## Consequence for the Proof Search

The invariant should not merely say that equal-length gaps are paired. It must
remember the boundary type of the equal-length pair created by a
length-matching response.

The promising induction class is now the six-family mutual induction above.

The next proof task is to prove the six families simultaneously by explicit
responses. The computational evidence suggests that `MO + MO` and `OO + MM`
are zero-like components, while `WO + MO` and `OO + WM` are the two
near-balanced wall families needed to close the induction.
