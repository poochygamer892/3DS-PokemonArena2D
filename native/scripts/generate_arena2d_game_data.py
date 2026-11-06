#!/usr/bin/env python3
"""Convert the authorized Arena-2D game catalogue into a compact C header.

The JSON is an extracted, data-only snapshot of the upstream project at the
revision documented in ATTRIBUTION.md.  It intentionally contains no music.
"""

import argparse
import json
import re
import unicodedata
from pathlib import Path


def ascii_text(value):
    value = unicodedata.normalize("NFKD", value).encode("ascii", "ignore").decode()
    return value.replace('"', '\\"').replace('\\', '\\\\')


def ident(value):
    return re.sub(r"[^a-z0-9_]", "_", value.lower())


def art_for(mon):
    # The existing PMD sheets bundled for the 3DS prototype are used as a
    # visual fallback until the full SpriteCollab streaming loader is added.
    known = {1: 0, 4: 1, 7: 2, 25: 3, 6: 4, 9: 5}
    if mon["id"] in known:
        return known[mon["id"]]
    types = mon.get("types", [])
    if "Fire" in types:
        return 1
    if "Water" in types or "Ice" in types:
        return 2
    if "Electric" in types:
        return 3
    if "Flying" in types or "Dragon" in types:
        return 4
    if "Steel" in types or "Rock" in types:
        return 5
    return 0


def cstr(value):
    return '"' + ascii_text(str(value)) + '"'


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--input", required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    data = json.loads(Path(args.input).read_text(encoding="utf-8"))
    mons = list(data["MONS"].items())
    moves = list(data["MOVES"].items())
    out = [
        "/* Generated from the authorized Arena-2D data snapshot. Do not edit. */",
        "#ifndef ARENA2D_GAME_DATA_H",
        "#define ARENA2D_GAME_DATA_H",
        "",
        "#define ARENA2D_ROSTER_COUNT %d" % len(mons),
        "#define ARENA2D_MOVE_COUNT %d" % len(moves),
        "",
        "typedef struct {",
        "    const char *key, *name, *type1, *type2;",
        "    const char *moves[4];",
        "    int dex, base[6], fallbackArt;",
        "    bool legendary;",
        "} Arena2dRosterEntry;",
        "",
        "typedef struct {",
        "    const char *key, *name, *type, *kind, *effect;",
        "    int power; float cooldown, range;",
        "} Arena2dMoveEntry;",
        "",
        "static const Arena2dRosterEntry kArena2dRoster[ARENA2D_ROSTER_COUNT] = {",
    ]
    for key, mon in mons:
        types = mon.get("types", []) + [""]
        move_keys = mon.get("moves", []) + [""] * 4
        base = mon.get("base", [0] * 6)
        out.append("    {%s, %s, %s, %s, {%s}, %d, {%s}, %d, %s}," % (
            cstr(key), cstr(mon["name"]), cstr(types[0]), cstr(types[1]),
            ", ".join(cstr(v) for v in move_keys[:4]), int(mon["id"]),
            ", ".join(str(int(v)) for v in base), art_for(mon),
            "true" if mon.get("legend") else "false"))
    out += ["};", "", "static const Arena2dMoveEntry kArena2dMoves[ARENA2D_MOVE_COUNT] = {"]
    for key, move in moves:
        effect = ""
        for name in ("burn", "poison", "slow", "stun", "sleep"):
            if name in move.get("fx", {}):
                effect = name
                break
        out.append("    {%s, %s, %s, %s, %s, %d, %.3ff, %.3ff}," % (
            cstr(key), cstr(move.get("n", key)), cstr(move.get("t", "Normal")),
            cstr(move.get("k", "")), cstr(effect), int(move.get("p", 0)),
            float(move.get("cd", 0)), float(move.get("range", 0))))
    out += ["};", "", "#endif"]
    Path(args.output).write_text("\n".join(out) + "\n", encoding="ascii")


if __name__ == "__main__":
    main()
