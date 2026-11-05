# Poké Arena 2D reference for the 3DS port

The current target is [pokearena2d.com](https://www.pokearena2d.com/), which is
different from `poke2darena.com`. The website's pixel-art home screen has Play,
Online, Customize, Pokédex, Tutorial, Settings, and Credits. Its Play submenu
contains League, VS CPU, 2 Players, Training, Arena, and Back. Its real-time
battle view uses a stadium arena, Pokémon sprites, health and move bars, and a
compact action row.

This project keeps those menu labels and screen structure while splitting the
interface across the 3DS top and touch screens. The home screen uses the site's
logo, scenery, and pixel font style. The local build uses a selection of six
Pokémon and four arenas from the reference game's publicly served assets. The
battle is a native C implementation, not the original browser game or its
JavaScript runtime.

## What works in this build

- Local CPU battle with Easy, Normal, and Hard difficulty.
- Training, a six-fight arena challenge, and a local league progression.
- Team selection, a small Pokédex, controls guide, and settings.
- Touch controls on the lower screen and Circle Pad / buttons during battle.

Online matches and local two-player battles are not implemented. The full web
roster, moves, evolution system, shop, accounts, ranked ladder, and online
services are not included. Combat values and progression are simplified for
this first 3DS adaptation.

For asset origins and third-party notices, see
[`native/assets/pokearena2d/ATTRIBUTION.md`](native/assets/pokearena2d/ATTRIBUTION.md).
