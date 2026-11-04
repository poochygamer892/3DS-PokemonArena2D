# Prototipo homebrew para Nintendo 3DS

Combate en tiempo real 3 contra 3 para Old 2DS y New 3DS XL. Incluye equipos con Pikachu, Bulbasaur, Squirtle, Charizard, Gyarados y Eevee; sprites Pokémon de frente y espalda; una arena de estadio; habilidades a distancia, esquiva, cambio de Pokémon, suministros y CPU que ataca cuerpo a cuerpo y con proyectiles.

Los sprites incluidos proceden de [PokéAPI Sprites](https://github.com/PokeAPI/sprites). Se conservan los PNG originales en `assets/pokemon/` y el código dibuja sus versiones compactadas de `source/pokemon_sprites.h`. Consulta `assets/pokemon/ATTRIBUTION.md` para la procedencia.

## Controles

- Circle Pad: movimiento.
- A: ataque básico.
- B, X, Y: habilidades.
- L: esquiva.
- R: definitivo cuando el medidor llega al 100%.
- Cruceta izquierda/derecha: cambiar de Pokémon.
- Pantalla táctil: acciones y selección de equipo.

## Compilar

Con devkitARM, libctru, citro3d, citro2d y las herramientas de 3DS:

```sh
make -C native
```

La acción de GitHub `Build Nintendo 3DS homebrew` adjunta `.3dsx`, `.cia`, `.elf` y `.smdh` como artefactos.
