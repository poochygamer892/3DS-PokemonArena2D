# Poke2D Arena para Nintendo 3DS

Adaptación homebrew no oficial del juego web [Poke2D Arena](https://poke2darena.com/) para Old 2DS y New 3DS XL. El juego original corre en navegador; este proyecto reimplementa el combate como una aplicación nativa de 3DS.

Versión actual: **v0.2.0** (registrada en [`VERSION`](VERSION)).

El menú de 3DS incluye Quick Battle, Vs CPU con tres dificultades, Story Mode de 16 capítulos, Arena Run de seis combates, editor de equipos, Online y Settings. La batalla usa los sprites animados y las arenas que carga la web; los textos del HUD se ampliaron para que se lean mejor en las pantallas pequeñas.

Story Mode y Arena Run son adaptaciones locales iniciales. Story guarda el avance solo durante la sesión y reutiliza el grupo de seis Pokémon incluido. El editor permite formar un equipo con esos seis. La pantalla Online muestra su estado, pero el juego por red todavía no está implementado. Tampoco están incluidos el catálogo completo ni todos los modos y contenidos de la web.

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
