# Pokémon sprite assets

This fan project uses 96×96 front and back Pokémon sprites from the PokéAPI sprites repository:

- https://github.com/PokeAPI/sprites
- Individual image paths: `sprites/pokemon/<id>.png` and `sprites/pokemon/back/<id>.png`

The game downsamples these files to 48×48 nearest-neighbor pixel art and packs them into `native/source/pokemon_sprites.h` with `native/scripts/build_sprite_header.py` (requires Pillow). The original character names, designs, and sprite art belong to their respective rights holders. This attribution does not grant additional rights.
