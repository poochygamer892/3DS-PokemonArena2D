# Pokemon Arena 3DS

Port nativo en progreso del proyecto autorizado
[sunsunyul2014-ai/pokemon-arena](https://github.com/sunsunyul2014-ai/pokemon-arena),
revisión `8826a92`.

El port reproduce su estadio procedural y sus seis luchadores: Charmander,
Squirtle, Bulbasaur, Pikachu, Scyther y Lucario. Cada luchador conserva ataque
normal y cuatro habilidades: A ataca; B, X, Y y R activan habilidades; L esquiva;
SELECT cambia de luchador.

Las hojas de Scyther y Lucario provienen de ese repositorio bajo el permiso del
autor. Los efectos se sintetizan nativamente a partir del enfoque WebAudio del
proyecto. No se empaqueta música.

```sh
make -C native
```
