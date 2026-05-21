from __future__ import annotations

Gap = tuple[int, int, int]


def canon(gap: Gap) -> Gap:
    m, left, right = gap
    return min(gap, (m, right, left))


def parse_state(text: str) -> list[Gap]:
    text = text.strip().strip('"')
    if not text:
        return []
    return sorted(canon(tuple(map(int, item.split(":")))) for item in text.split())


def format_state(gaps: list[Gap]) -> str:
    return " ".join(f"{m}:{left}:{right}" for m, left, right in gaps)


def remove_once(items: list[Gap], item: Gap) -> list[Gap] | None:
    item = canon(item)
    try:
        index = items.index(item)
    except ValueError:
        return None
    return items[:index] + items[index + 1 :]


def remove_known_t_components(gaps: list[Gap]) -> tuple[list[str], list[Gap]]:
    gaps = list(gaps)
    labels: list[str] = []
    changed = True
    while changed:
        changed = False
        without_t0 = remove_once(gaps, (1, 0, 1))
        if without_t0 is not None:
            gaps = without_t0
            labels.append("T0")
            changed = True
            continue

        for n in range(1, 128):
            patterns = [
                (f"T1({n})", [(n, 1, 2), (n, 1, 2)]),
                (f"T2({n})", [(n, 2, 2), (n, 1, 1)]),
                (f"T3({n})", [(n, 2, 2), (n, 0, 1)]),
                (f"T4({n})", [(n, 2, 2), (n + 1, 0, 1)]),
                (f"T5({n})", [(n, 0, 2), (n, 1, 2)]),
                (f"T6({n})", [(n + 1, 0, 2), (n, 1, 2)]),
            ]
            for label, pattern in patterns:
                rest = list(gaps)
                for gap in pattern:
                    next_rest = remove_once(rest, gap)
                    if next_rest is None:
                        break
                    rest = next_rest
                else:
                    gaps = rest
                    labels.append(label)
                    changed = True
                    break
            if changed:
                break
    return labels, gaps


def has_current_legal_move(gap: Gap) -> bool:
    m, left, right = gap
    for pos in range(m):
        if pos == 0 and left in (0, 2):
            continue
        if pos == m - 1 and right in (0, 2):
            continue
        return True
    return False


def remove_current_inert_components(gaps: list[Gap]) -> tuple[list[str], list[Gap]]:
    labels: list[str] = []
    remainder: list[Gap] = []
    for gap in gaps:
        if has_current_legal_move(gap):
            remainder.append(gap)
        else:
            labels.append(f"I({gap[0]},{gap[1]},{gap[2]})")
    return labels, remainder


def remove_r_component(gaps: list[Gap]) -> list[Gap] | None:
    for n in range(1, 128):
        rest = list(gaps)
        for gap in [(1, 1, 2), (n, 0, 1), (n, 0, 2)]:
            next_rest = remove_once(rest, gap)
            if next_rest is None:
                break
            rest = next_rest
        else:
            return rest
    return None


def classify_a_family_child(text: str) -> str:
    original = parse_state(text)
    rest_after_r = remove_r_component(original)
    if rest_after_r is not None:
        _, r_remainder = remove_known_t_components(rest_after_r)
        if not r_remainder:
            return "R+T"

    _, remainder = remove_known_t_components(original)
    if not remainder:
        return "T-only"

    rest_after_r = remove_r_component(remainder)
    if rest_after_r is not None:
        _, r_remainder = remove_known_t_components(rest_after_r)
        if not r_remainder:
            return "R+T"

    rest_after_wo2 = remove_once(remainder, (2, 0, 2))
    if rest_after_wo2 is not None and len(rest_after_wo2) == 1 and rest_after_wo2[0][1:] == (0, 2):
        return "A(2,k)+T"
    if rest_after_wo2 is not None and not rest_after_wo2:
        return "WO(2)+T"
    return "other"
