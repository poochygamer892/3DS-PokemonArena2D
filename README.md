# Pokemon Arena para Nintendo 3DS

Adaptación homebrew no oficial para Old 2DS y New 3DS XL basada en el proyecto
autorizado [Pokemon Arena](https://github.com/sunsunyul2014-ai/pokemon-arena).

Versión actual: **v0.10.0** (registrada en [`VERSION`](VERSION)).

Esta línea de desarrollo porta el estadio procedural y los seis luchadores del
proyecto coreano: Charmander, Squirtle, Bulbasaur, Pikachu, Scyther y Lucario.
Sus cuatro habilidades se asignan a B, X, Y y R; A mantiene el ataque normal y
L permite esquivar. Los efectos de sonido se sintetizan en la consola y no se
incluye música.

El código y los recursos visuales de `sunsunyul2014-ai/pokemon-arena` se usan
con permiso directo de su autor para este port. La revisión de referencia es
`8826a92`. Las referencias al proyecto francés y sus archivos han sido retiradas
del árbol activo.

## Compilar

```sh
make -C native
```

GitHub Actions genera `.3dsx`, `.cia`, `.elf` y `.smdh`.
