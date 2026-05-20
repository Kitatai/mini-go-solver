# Responses from Near-Balanced Boundary Two-Gap States up to total length 40

This file summarizes exact response data for the near-balanced losing states

```text
Gap(ceil(s / 2), wall, opponent)
+ Gap(floor(s / 2), current, opponent)
```

for `2 <= s <= 40`.

Generated file:

- `balanced_boundary_responses_sum40.csv`

Computation:

```text
states=60685394
wall time=1:51.07
max RSS=4671616 KB
```

The states for `s = 2` and `s = 3` have no legal moves by the player to move. For `4 <= s <= 40`, the CSV contains every legal move from the near-balanced state and one exact winning response in the resulting child state.

## Response Locality

There are 703 legal moves in these states for `4 <= s <= 40`.

The best recorded winning response, chosen by minimum edge distance within its response gap, has the following edge-distance distribution:

```text
0: 56
1: 341
2: 298
3: 6
4: 2
```

Thus, 695 of 703 moves have a winning response within distance 2 from an edge of the response gap. The remaining 8 moves have a winning response within distance 4.

The exceptional rows with best response edge distance at least 3 occur at total lengths

```text
14, 15, 21, 27, 33, 37
```

## Proof Target

The data supports a constructive proof of the near-balanced two-gap losing lemma by induction on `s`.

The base cases are `s = 2` and `s = 3`, where the player to move has no legal move.

For the induction step, it is enough to classify the current player's legal move and give a response that moves to a previously established losing state. The observed responses are predominantly local edge responses, with a small number of exceptional central cases.
