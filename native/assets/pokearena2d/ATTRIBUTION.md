# Asset attribution

## Authorized Pokemon Arena source

This port follows the code, interaction design and local Scyther/Lucario walk
sheets from [sunsunyul2014-ai/pokemon-arena](https://github.com/sunsunyul2014-ai/pokemon-arena),
revision `8826a92`. The repository author granted permission to use those files
and sprites for this 3DS adaptation. The granted scope excludes music, and this
project contains no music tracks.

`../korean_arena_authorized/scyther_walk.png` and
`../korean_arena_authorized/lucario_walk.png` are the two local sheets supplied
by that source. `source/pmd/` contains the pre-existing PMD SpriteCollab sheets
for Charmander, Squirtle, Bulbasaur and Pikachu; their upstream attribution and
terms continue to apply.

The stadium is rendered with native geometry. Sound effects are synthesized at
runtime; no sound recording from another project is bundled.
