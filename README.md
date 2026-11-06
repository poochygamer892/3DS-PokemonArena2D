# Pokemon Arena para Nintendo 3DS

Adaptación homebrew no oficial para Old 2DS y New 3DS XL basada directamente
en el proyecto autorizado [PokeArena2D.com](https://www.pokearena2d.com/).

Versión actual: **v0.11.0** (registrada en [`VERSION`](VERSION)).

La web autorizada es la referencia principal para interfaz, arena, luchadores,
animaciones, reglas y combate. La adaptación actual incorpora su textura de
estadio oficial y continúa trasladando sus sistemas al formato de dos pantallas.
Los efectos de sonido se sintetizan en la consola y no se incluye música.

El titular de PokeArena2D.com ha dado permiso directo para esta adaptación.
La música queda excluida del port. Los sprites PMD conservan su atribución y
términos de origen indicados por la web.

## Compilar

```sh
make -C native
```

GitHub Actions genera `.3dsx`, `.cia`, `.elf` y `.smdh`.
