# N=2..45 Search Results Under Updated Legal Move Rule

`W`: the first player wins after playing the fixed initial move.
`L`: the first player loses after playing the fixed initial move.

Columns are initial move indices from left to right, `0..N-1`.

Updated legal move rule: a move is legal if it is a capture move or it is not a suicide move.

Rows `N=38..45` were obtained with the gap decomposition solver.

```text
N=2:  L L
N=3:  L W L
N=4:  L W W L
N=5:  L L W L L
N=6:  L L W W L L
N=7:  L L L W L L L
N=8:  L L W W W W L L
N=9:  L L W L W L W L L
N=10: L L W L W W L W L L
N=11: L L W W L W L W W L L
N=12: L L W W L W W L W W L L
N=13: L L W W L L W L L W W L L
N=14: L L W W W W W W W W W W L L
N=15: L L W L W W L W L W W L W L L
N=16: L L W W W W L W W L W W W W L L
N=17: L L W W W W W L W L W W W W W L L
N=18: L L W W W W W W W W W W W W W W L L
N=19: L L W W L W W L L W L L W W L W W L L
N=20: L L W W W W W W W W W W W W W W W W L L
N=21: L L W W W W W W W L W L W W W W W W W L L
N=22: L L W W W W W W W W W W W W W W W W W W L L
N=23: L L W W W W W W W W L W L W W W W W W W W L L
N=24: L L W W W W W W W W W W W W W W W W W W W W L L
N=25: L L W W W W W W W W W L W L W W W W W W W W W L L
N=26: L L W W W W W W W W W W W W W W W W W W W W W W L L
N=27: L L W W W W W W W W W W L W L W W W W W W W W W W L L
N=28: L L W W W W W W W W W W W W W W W W W W W W W W W W L L
N=29: L L W W W W W W W W W W W L W L W W W W W W W W W W W L L
N=30: L L W W W W W W W W W W W W W W W W W W W W W W W W W W L L
N=31: L L W W W W W W W W W W W W L W L W W W W W W W W W W W W L L
N=32: L L W W W W W W W W W W W W W W W W W W W W W W W W W W W W L L
N=33: L L W W W W W W W W W W W W W L W L W W W W W W W W W W W W W L L
N=34: L L W W W W W W W W W W W W W W W W W W W W W W W W W W W W W W L L
N=35: L L W W W W W W W W W W W W W W L W L W W W W W W W W W W W W W W L L
N=36: L L W W W W W W W W W W W W W W W W W W W W W W W W W W W W W W W W L L
N=37: L L W W W W W W W W W W W W W W W L W L W W W W W W W W W W W W W W W L L
N=38: L L W W W W W W W W W W W W W W W W W W W W W W W W W W W W W W W W W W L L
N=39: L L W W W W W W W W W W W W W W W W L W L W W W W W W W W W W W W W W W W L L
N=40: L L W W W W W W W W W W W W W W W W W W W W W W W W W W W W W W W W W W W W L L
N=41: L L W W W W W W W W W W W W W W W W W L W L W W W W W W W W W W W W W W W W W L L
N=42: L L W W W W W W W W W W W W W W W W W W W W W W W W W W W W W W W W W W W W W W L L
N=43: L L W W W W W W W W W W W W W W W W W W L W L W W W W W W W W W W W W W W W W W W L L
N=44: L L W W W W W W W W W W W W W W W W W W W W W W W W W W W W W W W W W W W W W W W W L L
N=45: L L W W W W W W W W W W W W W W W W W W W L W L W W W W W W W W W W W W W W W W W W W L L
```
