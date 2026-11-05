# Poké Arena 2D para Nintendo 3DS

Adaptación homebrew no oficial para Old 2DS y New 3DS XL. El proyecto tomó
como inspiración inicial [Empress Pokémon 2D Arena](https://omniscye.itch.io/empress-pokemon-2d-arena)
y ahora usa [Poké Arena 2D](https://www.pokearena2d.com/) como referencia
principal para los menús, el arte y el combate.

Versión actual: **v0.4.0** (registrada en [`VERSION`](VERSION)). Los paquetes
generados llevan la versión en el nombre del archivo.

La interfaz incluye Play, Online, Customize, Pokédex, Tutorial, Settings y
Credits. Play abre League, VS CPU, 2 Players, Training y Arena. La build local
tiene combate contra CPU, selección de equipos, seis Pokémon y cuatro arenas,
con el logo, fondos de color mejorado y Pokémon animados. Esta versión corrige
las etiquetas ocultas por el orden de profundidad, usa la fuente del sistema
para que los textos se lean y conserva más detalle en los sprites de combate.

Esta es una reimplementación nativa en C para 3DS; no empaqueta el juego web ni
incluye su catálogo completo. La red y el modo de dos jugadores siguen sin
implementarse. El combate, la progresión y el roster son versiones reducidas.
La adaptación no está afiliada a los autores de los juegos, PMDCollab, Nintendo
ni The Pokémon Company.

## Compilar

Con devkitARM, libctru, citro3d, citro2d, `tex3ds` y Python con Pillow:

```sh
make -C native
```

GitHub Actions compila `.3dsx` y `.cia`; consulta los artefactos en
[Actions](https://github.com/poochygamer892/3DS-PokemonArena2D/actions).

## Documentación y recursos

- [Análisis de la referencia actual](POKEARENA2D_REFERENCE.md)
- [Plan de adaptación a 3DS](PORTING_PLAN_3DS.md)
- [Port nativo y controles](native/README.md)
- [Procedencia y licencias de los recursos](native/assets/pokearena2d/ATTRIBUTION.md)

La pantalla de 3DS tiene una resolución nativa de 400 × 240; esta adaptación
mantiene el estilo pixel art del juego web y ajusta sus recursos a esa pantalla.
