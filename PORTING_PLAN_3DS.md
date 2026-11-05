# Adaptación de Poke2D Arena a Nintendo 3DS

## Estado actual

El objetivo es llevar a Old 2DS y New 3DS XL el juego web [Poke2D Arena](https://poke2darena.com/play-online/). No se puede empaquetar el navegador ni su bundle de Phaser directamente como `.3dsx` o `.cia`; la aplicación se está reimplementando en C con libctru, citro3d y citro2d.

La versión actual tiene pantallas de menú para Quick Battle, Vs CPU, Story Mode, Arena Run, Your Teams, Online y Settings. Incluye combate offline 3 contra 3, tres niveles de CPU, un esqueleto local de 16 capítulos, una carrera de seis pisos con vida acumulada y un editor de equipos que usa los seis Pokémon con arte incluido. La arena y los sprites/animaciones direccionales provienen de los recursos que sirve el juego web.

## Trabajo pendiente

1. Probar el texto ampliado y el atlas en hardware Old 2DS y New 3DS XL.
2. Afinar el combate contra las reglas web: movimientos, tipos, estados, apuntado y CPU.
3. Añadir contenido real para los capítulos de Story, más arenas y más Pokémon.
4. Implementar el juego Online y guardar el progreso y equipos en SD.
5. Validar `.3dsx` y `.cia` en emulador y hardware.

## Port y controles

| Acción | Control |
|---|---|
| Movimiento | Circle Pad |
| Ataque básico | A o botón táctil |
| Movimientos | B, X, Y o botones táctiles |
| Space Dash | L o botón táctil |
| Definitivo | R o botón táctil al 100% |
| Cambiar Pokémon | Cruceta izquierda/derecha o tarjeta táctil |

El menú Online informa que no hay juego en red en esta build; no crea ni busca salas. La historia usa una selección pequeña de oponentes y conserva el avance solo mientras la aplicación sigue abierta. El objetivo inmediato es completar la experiencia local y validar la legibilidad y los controles en la consola.

## Análisis y límites

La web y una partida Quick Battle se revisaron en navegador; las guías públicas aportan reglas y controles. El bundle está minificado y no tiene source map visible. El análisis documentado separa lo observado de las reglas descritas por el sitio y de las decisiones propias del port.

La adaptación aún no equivale al juego web completo. La CPU, parte del cálculo de daño y algunos movimientos son aproximaciones nativas; faltan el juego online, el catálogo y el contenido completo de historia y arenas.
