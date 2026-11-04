# 3DS Pokémon Arena 2D

Proyecto homebrew para Nintendo 3DS basado en [Empress Pokémon 2D Arena](https://omniscye.itch.io/empress-pokemon-2d-arena).

El objetivo es adaptar su concepto y mecánicas principales a Old 2DS mediante una implementación nativa para la consola. Es un proyecto no oficial y no está afiliado al creador del juego original.

También se inspeccionó [Poke2D Arena](https://poke2darena.com/) como referencia web adicional. Es un juego distinto de la build Unity de Empress analizada anteriormente; sus observaciones y una propuesta de controles para 3DS están en [POKE2D_WEB_ANALYSIS.md](POKE2D_WEB_ANALYSIS.md).

## Referencia técnica de 3DS

La build de 3DS de PokéSurvivor facilitada para este proyecto se usa como referencia técnica para estudiar formatos y despliegue en la consola. Su Title ID es `000400000F5A3D00`. El [repositorio del juego original](https://github.com/JPTER78/pokesurvivor) contiene su código web; no se han copiado ese código ni sus recursos.

El diseño apunta a Old 2DS como perfil mínimo y a New 3DS XL como dispositivo compatible con el mismo homebrew. Ya hay un prototipo nativo con Pikachu, Bulbasaur, Squirtle, Charizard, Gyarados y Eevee dibujados para el proyecto. GitHub Actions generó los formatos `.3dsx` y `.cia`; están en los [artefactos de esta compilación](https://github.com/poochygamer892/3DS-PokemonArena2D/actions/runs/41822110482). Aún falta probar el juego en emulador o consola. Consulta [`native/README.md`](native/README.md) y el [plan de port](PORTING_PLAN_3DS.md).
