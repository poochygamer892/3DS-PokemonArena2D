# Poké Arena 2D homebrew para Nintendo 3DS

Port nativo en progreso del juego web [Poké Arena 2D](https://www.pokearena2d.com/).
La interfaz adapta los menús del sitio y usa su logo, fondos, cuatro arenas y
seis Pokémon animados. El texto usa la fuente del sistema con el ajuste de
filtro de Citro2D. Las etiquetas se dibujan delante de los paneles del menú; la
versión anterior las dejaba detrás debido al orden de profundidad.

La versión local incluye combate contra CPU, Training, una progresión League,
Arena Challenge de seis peleas, selección de equipos, Pokédex, Tutorial y
Settings. Online y 2 Players aparecen en los menús, pero no están implementados.
Tampoco están incluidos el roster completo, la evolución web, la tienda, cuentas
o el juego por red. Los detalles están en
[`../POKEARENA2D_REFERENCE.md`](../POKEARENA2D_REFERENCE.md); el origen de los
recursos está en `assets/pokearena2d/ATTRIBUTION.md`.

## Controles

- Circle Pad: movimiento y apuntado automático al rival.
- A: ataque básico.
- B, X, Y: movimientos especiales.
- L: esquiva.
- R: definitivo cuando el medidor llega al 100%.
- Cruceta izquierda/derecha: cambiar de Pokémon.
- Pantalla táctil: seleccionar menús y acciones.
- START durante el combate: pausa, reinicio o regreso al menú.
- Menús: cruceta para navegar, A para elegir y B para volver.
- Ajustes: cambia entre Stadium, Neon, Crater y Ruins, el nivel de CPU y el
tamaño del texto.

## Compilar

Con devkitARM, libctru, citro3d, citro2d, `tex3ds` y Pillow:

```sh
make -C native
```

El proceso convierte las hojas de animación a 75% del tamaño original en un
atlas RGBA4, y mantiene los fondos en RGB565 filtrado para preservar el color
sin aumentar el uso de VRAM. GitHub Actions genera
`.3dsx`, `.cia`, `.elf` y `.smdh`.
