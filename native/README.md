# Adaptación homebrew de Poke2D Arena para Nintendo 3DS

Port nativo en progreso del juego web [Poke2D Arena](https://poke2darena.com/). La batalla de entrenamiento usa la arena volcánica, los sprites direccionales y las animaciones de combate que carga la versión web. El equipo por defecto es Pikachu, Steelix y Scizor contra Gyarados, Charizard y Blastoise. La interfaz adapta el HUD y los movimientos de Quick Battle a las dos pantallas de 3DS.

Esta es una adaptación local en progreso, no el juego completo: el primer objetivo es el combate offline 3 contra 3. Online, Story Mode, Arena Run, selección de equipos y el catálogo completo todavía no están implementados. Los recursos capturados y su atribución están en `assets/poke2d/ATTRIBUTION.md`.

## Controles

- Circle Pad: movimiento.
- A: ataque básico.
- B, X, Y: Charge Beam, Thunder Punch y Thunder Wave.
- L: Space Dash.
- R: Thunderbolt cuando el medidor llega al 100%.
- Cruceta izquierda/derecha: cambiar de Pokémon.
- Pantalla táctil: acciones y selección de equipo.

## Compilar

Con devkitARM, libctru, citro3d, citro2d, `tex3ds` y Pillow:

```sh
make -C native
```

El proceso de compilación convierte los fondos y las hojas de animación en un
atlas RGBA4 para reducir el uso de memoria en Old 2DS.

La acción de GitHub `Build Nintendo 3DS homebrew` adjunta `.3dsx`, `.cia`, `.elf` y `.smdh` como artefactos.
