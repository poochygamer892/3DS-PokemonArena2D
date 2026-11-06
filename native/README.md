# Poké Arena 2D homebrew para Nintendo 3DS

Port nativo en progreso del juego web [Poké Arena 2D](https://www.pokearena2d.com/).
La interfaz adapta los menús del sitio y usa su logo, fondos, diez arenas y
seis Pokémon animados. Los glifos se generan desde Open Sans Regular a 40 px y
se almacenan en un atlas RGBA8 filtrado para reducir el grosor y los bordes
dentados. Las etiquetas se dibujan delante de los paneles del menú.

El combate incluye una cadena de tres golpes básicos, embestidas, ataques de
contacto, rayos, ráfagas, explosiones, retroceso y estados de quemadura, lentitud
y aturdimiento. Los tipos de movimiento tienen ventajas y resistencias. La
versión local también incluye CPU, Training, una progresión League, Arena
Challenge de seis peleas, selección de equipos, Pokédex, Tutorial y Settings.
Online y 2 Players aparecen en los menús, pero no están implementados.
Tampoco están incluidos el roster completo, la evolución web, la tienda, cuentas
o el juego por red. Los detalles están en
[`../POKEARENA2D_REFERENCE.md`](../POKEARENA2D_REFERENCE.md); el origen de los
recursos está en `assets/pokearena2d/ATTRIBUTION.md`.

## Controles

- Circle Pad o cruceta: movimiento; el apuntado sigue automáticamente al rival.
- A: ataque básico.
- B, X, Y: movimientos especiales.
- L: esquiva.
- R: definitivo cuando el medidor llega al 100%.
- SELECT: cambiar al siguiente Pokémon; también se puede tocar su retrato.
- Pantalla táctil: seleccionar menús y acciones.
- START durante el combate: pausa, reinicio o regreso al menú.
- Menús: Circle Pad o cruceta para navegar, A para elegir y B para volver.
- Ajustes: cambia entre diez arenas, el nivel de CPU y el tamaño del texto.

## Recursos autorizados de Arena-2D

La v0.7.0 adapta diez texturas de arena de
[`Freakyy-11/Arena-2D`](https://github.com/Freakyy-11/Arena-2D) a 400 × 240 y
reproduce una selección de sus efectos de sonido durante el combate y los
menús. Se usan con permiso del autor comunicado al propietario del proyecto.
No se incluye ninguna pista musical de ese repositorio. Consulta
`assets/pokearena2d/ATTRIBUTION.md` para procedencia y créditos de los recursos.

## Compilar

Con devkitARM, libctru, citro3d, citro2d, `tex3ds` y Pillow:

```sh
make -C native
```

El proceso convierte las hojas de animación a 75% del tamaño original en un
atlas RGBA5551, la fuente en un atlas RGBA8 y mantiene los fondos en RGB888 para
conservar las paletas de la web. GitHub Actions genera
`.3dsx`, `.cia`, `.elf` y `.smdh`.
