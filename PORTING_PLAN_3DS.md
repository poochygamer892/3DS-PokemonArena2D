# Adaptación de Poke2D Arena a Nintendo 3DS

## Estado actual

El objetivo es llevar a Old 2DS y New 3DS XL el juego web [Poke2D Arena](https://poke2darena.com/play-online/). No se puede empaquetar el navegador ni su bundle de Phaser directamente como `.3dsx` o `.cia`; la aplicación se está reimplementando en C con libctru, citro3d y citro2d.

La base de la versión actual es Quick Battle offline 3 contra 3. La arena y los sprites/animaciones direccionales provienen de los recursos que sirve el juego web. La consola muestra la batalla y el HUD arriba, y el equipo y botones táctiles abajo. El equipo inicial de la adaptación es Pikachu, Steelix y Scizor contra Gyarados, Charizard y Blastoise.

## Trabajo pendiente

1. Validar la compilación y ajustar la textura atlas para que quepa y funcione en Old 2DS.
2. Afinar el combate contra las reglas web: movimientos, tipos, estados, apuntado, CPU y cambios.
3. Añadir más arenas y Pokémon, además de edición de equipos y modos locales.
4. Evaluar el acceso a red y los servicios necesarios para Online. Story Mode y Arena Run también requieren contenido y reglas adicionales.
5. Probar `.3dsx` y `.cia` en Citra/Lime3DS y en hardware Old 2DS/New 3DS XL.

## Port y controles

| Acción | Control |
|---|---|
| Movimiento | Circle Pad |
| Ataque básico | A o botón táctil |
| Movimientos | B, X, Y o botones táctiles |
| Space Dash | L o botón táctil |
| Definitivo | R o botón táctil al 100% |
| Cambiar Pokémon | Cruceta izquierda/derecha o tarjeta táctil |

Online y 3 Player Brawl no forman parte de la primera entrega: dependen de coordinación entre varios clientes y servicios remotos. El objetivo inmediato es que una partida local en la 3DS se vea y se juegue como Quick Battle.

## Análisis y límites

La web y una partida Quick Battle se revisaron en navegador; las guías públicas aportan reglas y controles. El bundle está minificado y no tiene source map visible. El análisis documentado separa lo observado de las reglas descritas por el sitio y de las decisiones propias del port.

La adaptación aún no equivale al juego web completo. La CPU, parte del cálculo de daño y algunos movimientos son aproximaciones nativas; todavía no hay paridad del catálogo, menús, historia, juego online ni contenido completo. La compilación anterior del prototipo usaba gráficos geométricos y sprites distintos; se está reemplazando por los recursos reales de Poke2D Arena.
