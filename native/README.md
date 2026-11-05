# Poké Arena 2D homebrew para Nintendo 3DS

Port nativo en progreso del juego web [Poké Arena 2D](https://www.pokearena2d.com/).
La interfaz adapta los menús del sitio y usa su logo, fondos, cuatro arenas y
seis Pokémon animados. Los glifos se generan desde Pixelify Sans, la fuente que
carga la web, y se almacenan en el atlas a una resolución mayor. Esto evita el
fallback de fuente que Azahar mostraba como texto deformado. Las etiquetas se
dibujan delante de los paneles del menú.

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
atlas RGBA5551 y mantiene los fondos en RGB888 para conservar las paletas de la
web. GitHub Actions genera
`.3dsx`, `.cia`, `.elf` y `.smdh`.
