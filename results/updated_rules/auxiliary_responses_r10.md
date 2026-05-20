# Auxiliary Family Responses up to r=10

This file summarizes response data for the auxiliary four-gap families A and B up to `r = 10`, plus the special state C.

Generated file:

- `auxiliary_responses_r10.csv`

Computation:

```text
states=256729
wall time=0:00.20
max RSS=27284 KB
```

## Result

All legal moves from the checked auxiliary losing states have a winning response.

There are 190 response rows:

```text
Family A: 95
Family B: 90
Family C: 5
```

The best recorded response edge-distance distribution is:

```text
0: 69
1: 91
2: 28
3: 2
```

## Closure Check

The response child states are not confined to the original four named families. Their gap-count distribution is:

```text
2 gaps: 1
3 gaps: 1
4 gaps: 40
5 gaps: 27
6 gaps: 121
```

This shows that the proof should not rely on a short finite list of whole-state templates. A broader structural induction invariant is needed.

## Interpretation

The auxiliary families are useful, but they are not by themselves a closed induction system.

The better proof target is a local structural lemma: local responses reduce the total empty count while preserving a class of states characterized by safe boundaries and controlled length-`3` side-gap configurations.
