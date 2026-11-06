# Poké Arena 2D reference assets

This 3DS fan project includes a small selection of publicly served art from
[Poké Arena 2D](https://www.pokearena2d.com/), the current visual and gameplay
reference for this port:

- `source/logo.png` and `source/menu-background.jpg` from `assets/ui/`.
- `source/arena-*.jpg` from `assets/arenas/ovaladas/`.
- Six Pokémon animation sheets in `source/pmd/`, converted to a compact 3DS
  sprite atlas. The upstream public `assets/creditos.json` credits the included
  base sprites to CHUNSOFT and, for some sprites, other PMDCollab contributors.
  Pokémon and their artwork remain the property of their respective owners and
  contributors.

The text glyphs are rasterized from **Open Sans Regular** at 40 px. The font is
Copyright 2010–2011 Google Corporation and licensed under Apache License 2.0;
the full license is included as `source/font/LICENSE-OpenSans.txt`.

## Arena-2D resources used with permission

The ten source arena images in `../arena2d_authorized/` are adapted from
[Freakyy-11/Arena-2D](https://github.com/Freakyy-11/Arena-2D) for the 400 × 240
3DS top screen. They are included under permission granted by that repository's
author to this project's owner.

The core interface and combat sound effects in
`../arena2d_authorized/sfx/` are transcodes of files from that same repository,
used with the same permission. Its README credits the original effects to
Kenney and rubberduck's “80 CC0 RPG SFX”; no music files from Arena-2D are
included in this project.

These files are included to build this personal, unofficial homebrew port. This
repository does not claim ownership of these assets or grant rights beyond the
licenses and permissions of their original owners. The 3DS project is not
affiliated with, endorsed by, or an official port of Poké Arena 2D, PMDCollab,
Nintendo, or The Pokémon Company.

## Arena-2D source and game data

`../arena2d_authorized/data/arena2d_game_data.json` is a data-only extraction
of the authorized [`Freakyy-11/Arena-2D`](https://github.com/Freakyy-11/Arena-2D)
source revision `79787a8`, captured on 2026-11-06. It supplies the 163-Pokémon
roster, 242 moves, teams, difficulty profiles, arenas and mega forms used by
the native port. `scripts/generate_arena2d_game_data.py` converts it to the C
header compiled by the 3DS build. No music data, files or references are part
of this snapshot.
