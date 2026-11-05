# Adaptación de Poké Arena 2D a Nintendo 3DS

## Estado en v0.3.0

La interfaz ya sigue la estructura de menús de la web: Play, Online, Customize,
Pokédex, Tutorial, Settings y Credits, con una pantalla separada para los modos
de juego. Las pantallas usan el logo y el fondo de Poké Arena 2D. El texto ahora
sale de un atlas bitmap incluido en la build, así que no depende de fuentes del
sistema de 3DS/Azahar.

El port nativo ofrece combate offline contra CPU, tres dificultades, Training,
progresión local de 16 duelos, un reto de seis arenas, editor de equipo y seis
Pokémon. El combate usa cuatro fondos de arena y sprites direccionales
recortados de las hojas del juego web.

## Diferencias pendientes

- La red, ranked, salas y el modo de dos jugadores no están implementados.
- El roster de seis Pokémon no representa el catálogo web completo.
- La evolución, tienda, cuentas, efectos y el sistema de combate completo aún no
  están adaptados; los ataques y el progreso son aproximaciones nativas.
- La nueva versión debe comprobarse en Azahar y en una Old 2DS. La compilación
  automática valida el paquete, pero no sustituye esa prueba de ejecución.

## Controles

| Acción | Control |
|---|---|
| Movimiento | Circle Pad |
| Ataque básico | A o botón táctil |
| Movimientos | B, X, Y o botones táctiles |
| Esquiva | L o botón táctil |
| Definitivo | R o botón táctil al 100% |
| Cambiar Pokémon | Cruceta izquierda/derecha o tarjeta táctil |

El diseño usa la pantalla superior (400×240) para la arena y los combatientes;
la inferior (320×240) muestra el menú o los controles táctiles. El detalle de la
referencia y los recursos está en [`POKEARENA2D_REFERENCE.md`](POKEARENA2D_REFERENCE.md)
y [`native/assets/pokearena2d/ATTRIBUTION.md`](native/assets/pokearena2d/ATTRIBUTION.md).
