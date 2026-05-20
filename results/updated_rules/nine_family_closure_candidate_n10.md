# Nine-Family Closure Candidate up to n = 10

The six-family response analysis leaves a small closure obstruction involving
one-cell components. The key point is that a one-cell gap with no legal move for
the current player is not automatically removable in the relative-color gap
model: after a move elsewhere, `current/opponent` boundaries flip.

The obstruction is repaired in the checked range by adding three families to
the original six:

```text
WM(1)
WM(1) + MO(n) + MO(n)
WM(1) + OO(n) + MM(n)
```

Together with the original six,

```text
MO(n) + MO(n)
OO(n) + MM(n)
OO(n) + WM(n)
OO(n) + WM(n + 1)
WO(n) + MO(n)
WO(n + 1) + MO(n)
```

this gives a nine-family induction candidate.

## Check

An independent exact DP check was run for `1 <= n <= 10`.

For every legal move from every state in the nine families, there was a winning
response whose child decomposed into smaller members of the same nine-family
class.

No misses were found.

The observed decomposition signatures were:

```text
MO_MO          -> MO_MO
MO_MO          -> MO_MO + OO_MM
OO_MM          -> empty
OO_MM          -> MO_MO + MO_MO
OO_MM          -> OO_MM
OO_MM          -> OO_MM + OO_MM
OO_WM          -> MO_MO + WO_MO
OO_WM          -> OO_MM + OO_WM
OO_WM          -> OO_WM
OO_WM_PLUS     -> MO_MO + WO_PLUS_MO
OO_WM_PLUS     -> OO_MM + OO_WM_PLUS
OO_WM_PLUS     -> OO_MM + WM1
OO_WM_PLUS     -> OO_WM_PLUS
OO_WM_PLUS     -> WM1
WO_MO          -> MO_MO + OO_WM
WO_MO          -> OO_MM + WO_MO
WO_MO          -> WO_MO
WO_PLUS_MO     -> MO_MO + OO_WM_PLUS
WO_PLUS_MO     -> MO_MO + WM1
WO_PLUS_MO     -> OO_MM + WO_PLUS_MO
WO_PLUS_MO     -> WO_PLUS_MO
WM1_MO_MO      -> MO_MO + OO_MM + WM1
WM1_MO_MO      -> MO_MO + WM1
WM1_OO_MM      -> MO_MO + MO_MO + WM1
WM1_OO_MM      -> OO_MM + OO_MM + WM1
WM1_OO_MM      -> OO_MM + WM1
WM1_OO_MM      -> WM1
```

## Interpretation

This is the strongest current finite induction candidate for proving the
near-balanced state `P(s)` is losing.

The next step is to derive symbolic response formulas for these nine families
and prove the decompositions by induction on total empty count.
