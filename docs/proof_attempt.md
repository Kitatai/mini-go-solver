# Proof Attempt for the Near-Edge Losing Rule

This note records the current proof attempt. It is not a completed proof.

## Target Chain

The observed near-edge losing rule would follow from the following chain.

```text
Near-balanced boundary two-gap states are losing
        =>
Gap(m, opponent, wall) is winning for m >= 3
        =>
initial moves i = 1 and i = N - 2 are losing
```

The key target is therefore:

```text
P(s):
Gap(ceil(s / 2), wall, opponent)
+ Gap(floor(s / 2), current, opponent)
is losing for the player to move.
```

Exact computation confirms `P(s)` for `2 <= s <= 40`.

## Base Cases

For `s = 2` and `s = 3`, the player to move has no legal move in the near-balanced boundary state.

Thus, these are natural base cases for induction.

## Inductive Shape

Assume all required losing statements for smaller total empty count are known.

In `P(s)`, the player to move makes an arbitrary legal move. A proof should give an explicit response that moves to a losing state with total empty count `s - 2`.

Computationally, this is exactly what happens for `4 <= s <= 40`: every legal move has a response to a losing child state.

Most responses are local. In the computed range, the best recorded response has edge distance:

```text
0: 56
1: 341
2: 298
3: 6
4: 2
```

So 695 of 703 responses lie within distance `2` from an edge of the response gap.

## Main Obstruction

The simple induction target `P(s)` alone is too narrow.

Some valid responses do not return to another near-balanced two-gap state. The exceptional cases create a side gap of length `3` and then move to four-gap states.

The exceptional pattern is:

```text
before:

  boundary | . . . . . . . . . | opponent
                         ^
                         move

after the move:

  Gap(3, ..., opponent) + Gap(q, ..., ...) + unchanged gap
```

The response then moves to one of a few structured four-gap states.

## Auxiliary Families

The exceptional response targets suggest the following auxiliary losing families.

Family A:

```text
Gap(3, current, opponent)
+ Gap(3, current, opponent)
+ Gap(r, opponent, opponent)
+ Gap(r + 1, wall, current)
```

Family B:

```text
Gap(3, current, opponent)
+ Gap(4, wall, opponent)
+ Gap(r, current, current)
+ Gap(r, opponent, opponent)
```

Special state C:

```text
Gap(3, wall, current)
+ Gap(3, current, opponent)
+ Gap(3, current, opponent)
+ Gap(3, opponent, opponent)
```

Exact computation confirms:

- Family A is losing for `1 <= r <= 18`;
- Family B is losing for `1 <= r <= 18`;
- C is losing.

These families absorb the length-`3` side-gap exceptions in the computed range.

## Failed Closure Attempt

The next natural attempt was to prove `P(s)`, Family A, Family B, and C simultaneously.

However, response analysis for Family A/B up to `r = 10` shows that their responses often move to five-gap or six-gap states, not just back to `P`, A, B, or C.

The response child gap-count distribution was:

```text
2 gaps: 1
3 gaps: 1
4 gaps: 40
5 gaps: 27
6 gaps: 121
```

Thus, the four named families are not closed under one-step response.

This does not disprove the induction approach. It means the induction invariant must be broader than a short finite list of named global shapes.

## Failed Mirror Attempt

Another natural attempt is a direct mirror strategy between the two near-balanced gaps.

This is not sufficient. An all-winning-response table for `P(s)` up to `s = 24` shows that simple same-position or reflected-position responses in the opposite gap are not always winning responses. There are positions where winning responses exist, but none of the naive mirror responses work.

Thus, the proof cannot be a direct copy of the odd-board center proof. The near-balanced two-gap state has a weaker, boundary-sensitive symmetry that requires case distinctions by local gap size and boundary type.

## Untouched-Gap Response Lemma Candidate

The all-winning-response table up to `s = 40` shows a stronger useful fact.

For every legal move from `P(s)`, there is a winning response in the other original gap, namely the gap that was not touched by the move.

This holds for all 703 legal moves in the checked range.

Thus, the two gaps are still strategically paired, but not by a simple position mirror. The response position inside the untouched gap depends on boundary type and local gap sizes.

This suggests replacing the failed mirror rule with the following lemma candidate:

```text
Untouched-gap response lemma:
In P(s), any legal move in one original gap has a response
inside the other original gap that moves to a smaller losing state.
```

If this lemma can be proved with an explicit position rule, then the proof of `P(s)` becomes much simpler.

Further checks show that the response position cannot depend only on the length and boundary type of the untouched gap. For lengths at least `7`, the intersection of all winning response-position sets in the same untouched gap type is often empty. Thus, the response must also depend on the opponent's move position and on the split it creates in the other gap.

The current narrowed target is therefore:

```text
For each legal move position p in one original gap of P(s),
construct a response position q in the other original gap.
The formula for q may depend on:
  - which original gap was played;
  - p;
  - the two lengths created by that move;
  - the boundary type of the untouched gap.
```

This is still much narrower than the original search problem, because the response gap is fixed.

The all-response table suggests the following sharper rule. If the opponent's move in a gap of length `m` at position `p` creates sublengths

```text
p
m - p - 1
```

and the untouched gap has length `u`, then a winning response can always be chosen from

```text
q = p
q = u - 1 - p
q = m - p - 1
q = u - 1 - (m - p - 1)
```

whenever the position is legal. Equivalently, the response can be chosen so that one of the two sublengths it creates in the untouched gap equals one of the two sublengths created by the opponent's move. This held for all 703 legal moves from `P(s)` with `s <= 40`.

This is the strongest current proof candidate: prove that a length-matching response of this form always exists and preserves the induction class.

The response child has a more precise boundary-sensitive structure. Write:

```text
WO = Gap(length, wall, opponent)
WM = Gap(length, wall, current)
MM = Gap(length, current, current)
MO = Gap(length, current, opponent)
OO = Gap(length, opponent, opponent)
```

If the first move is in the `MO` gap of `P(s)`, then the four length-matching
forms create one of the following equal-length pairs after the response:

```text
right-right: OO + MO
right-left:  WO + MO
left-right:  OO + MM
left-left:   WO + MM
```

If the first move is in the `WO` gap of `P(s)`, the corresponding pairs are:

```text
right-right: MO + MO
right-left:  OO + MO
left-right:  MO + WM
left-left:   OO + WM
```

Using the priority order `right-right`, `right-left`, `left-right`, `left-left`,
the computed responses for all 703 legal moves from `P(s)` with `s <= 40`
always contain the predicted equal-length pair.

This shows that the induction class must remember boundary-sensitive matched
pairs, not just equal lengths.

There is an even simpler rule in the checked range:

```text
If the first move is in the WO gap, respond by matching right with right.
If the first move is in the MO gap, respond by matching left with right.
```

This covers all 703 legal moves from `P(s)` with `s <= 40`.

For

```text
P(a, b) = WO(a) + MO(b),  a = b or a = b + 1,
```

the two cases have the following form.

If the first move is in `WO(a)` at position `p`, the response in `MO(b)` at

```text
q = b - a + p
```

creates:

```text
MO(a - p - 1) + MO(a - p - 1)
```

and leaves one of:

```text
OO(p) + WM(p)       when a = b,
OO(p - 1) + WM(p)   when a = b + 1.
```

If the first move is in `MO(b)` at position `p`, the response in `WO(a)` at

```text
q = a - 1 - p
```

creates:

```text
OO(p) + MM(p)
```

and leaves a smaller state of the same `P` form:

```text
WO(b - p - 1) + MO(b - p - 1)       when a = b,
WO(b - p) + MO(b - p - 1)           when a = b + 1.
```

Thus the current proof target can be replaced by a six-family mutual induction:

```text
MO(n) + MO(n)
OO(n) + MM(n)
OO(n) + WM(n)
OO(n) + WM(n + 1)
WO(n) + MO(n)
WO(n + 1) + MO(n)
```

Exact independent DP checks confirm all six families are losing for
`1 <= n <= 13`. Nearby families such as `WO(n) + MM(n)` and `MO(n) + WM(n)`
are winning for `n >= 2`, so the boundary labels are essential.

Full-response analysis for the six families gives a stronger closure candidate.
For every legal move in the checked range `1 <= n <= 18`, there is a winning
response in the untouched original gap. The required response formulas are:

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

This is the current best route to completion. The proof should show for each of
the six families that the stated response is legal and that the resulting state
is a disjoint sum of smaller six-family states, possibly with one-cell gaps that
have no legal move.

## More Promising Proof Form

The computations suggest proving a local response lemma rather than enumerating all global shapes.

A plausible form is:

```text
In every valid losing template of the induction,
any move that does not create a length-3 side gap has a local response
within distance 2 that reduces the total empty count.

If a move creates a length-3 side gap, the response creates one of the
auxiliary length-3 configurations, which is handled by the same local lemma.
```

In this form, the induction invariant is not a fixed list of whole-board shapes. It is a local structural condition:

- no contact with opponent chains except by capture;
- no non-capturing edge move;
- every live chain has two liberties;
- any exceptional length-`3` side gap is paired with a balancing component.

The next proof step is to state this structural invariant precisely and check whether every recorded response preserves it with smaller total empty count.

## Current Status

The proof is not complete.

What has been reduced:

1. The near-edge rule reduces to `P(s)`.
2. `P(s)` is verified up to `s = 40`.
3. The nonlocal exceptions to the simple response rule are exactly length-`3` side-gap cases in the computed range.
4. Those exceptions are absorbed by structured auxiliary four-gap families in the computed range.

What remains:

1. Define a structural induction class broad enough to include the auxiliary response targets.
2. Prove that every legal move from a state in this class has an explicit response to a smaller state in the same class.
3. Derive `P(s)` and hence the near-edge losing rule from that induction.
