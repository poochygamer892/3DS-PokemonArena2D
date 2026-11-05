# Poke2D Arena para Nintendo 3DS

Adaptación homebrew no oficial del juego web [Poke2D Arena](https://poke2darena.com/) para Old 2DS y New 3DS XL. El juego original corre en navegador; este proyecto reimplementa el combate como una aplicación nativa de 3DS.

La versión actual se centra en Quick Battle offline 3 contra 3. Ya usa la arena volcánica, los sprites direccionales y las animaciones de Pokémon que carga el juego web, con un HUD adaptado a las dos pantallas y controles táctiles/botones. Online, Story Mode, Arena Run, selección de equipos y el catálogo completo aún quedan pendientes.

## Compilar

Con devkitARM, libctru, citro3d, citro2d, `tex3ds` y Python con Pillow:

```sh
make -C native
```

El flujo de GitHub Actions compila `.3dsx` y `.cia`; consulta sus artefactos en [Actions](https://github.com/poochygamer892/3DS-PokemonArena2D/actions).

## Estado y recursos

- [Análisis de la web y reglas observadas](POKE2D_WEB_ANALYSIS.md)
- [Plan de adaptación y limitaciones](PORTING_PLAN_3DS.md)
- [Port nativo y controles](native/README.md)
- [Procedencia de los recursos web incluidos](native/assets/poke2d/ATTRIBUTION.md)

El proyecto no está afiliado a los desarrolladores de Poke2D Arena ni a Nintendo.
