# Pokemon Arena para Nintendo 3DS

Adaptación homebrew no oficial para Old 2DS y New 3DS XL basada directamente
en el proyecto autorizado [PokeArena2D.com](https://www.pokearena2d.com/).

Versión actual: **v0.12.0** (registrada en [`VERSION`](VERSION)).

La web autorizada es la referencia principal para interfaz, arena, luchadores,
animaciones, reglas y combate. Esta versión carga los 33 luchadores, sus
sprites de combate, las cinco acciones, las evoluciones, las formas mega y
las cuatro arenas seleccionables de la web.
La interfaz se distribuye entre las dos pantallas. La música queda fuera del
proyecto; los efectos de sonido se sintetizan en la consola.

El titular de PokeArena2D.com ha dado permiso directo para esta adaptación.
La música queda excluida. Los sprites PMD conservan la atribución y los
términos de origen indicados por la web.

## Compilar

```sh
make -C native
```

GitHub Actions genera `.3dsx`, `.cia`, `.elf` y `.smdh`.

El juego web incluye servicios de red y pantallas que necesitan servidor o
cuenta. Esas funciones requieren una adaptación específica para 3DS y no se
presentan como disponibles en esta versión.
