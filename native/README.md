# Adaptación homebrew de Poke2D Arena para Nintendo 3DS

Port nativo en progreso del juego web [Poke2D Arena](https://poke2darena.com/). Usa la arena y los sprites animados que carga la versión web. La interfaz tiene dos pantallas, tipografía ampliada y menús para Quick Battle, Vs CPU, Story Mode, Arena Run, Your Teams, Online y Settings.

La build incluye combate offline 3 contra 3, Vs CPU con tres niveles, Story Mode local de 16 capítulos, Arena Run de seis pisos con vida acumulada, y edición de equipos con los seis Pokémon incluidos. El progreso de Story solo dura la sesión. Online tiene pantalla de estado, pero el juego en red, el catálogo completo y el contenido original de cada capítulo siguen pendientes. Los recursos capturados y su atribución están en `assets/poke2d/ATTRIBUTION.md`.

## Controles

- Circle Pad: movimiento.
- A: ataque básico.
- B, X, Y: Charge Beam, Thunder Punch y Thunder Wave.
- L: Space Dash.
- R: Thunderbolt cuando el medidor llega al 100%.
- Cruceta izquierda/derecha: cambiar de Pokémon.
- Pantalla táctil: acciones y selección de equipo.
- START durante el combate: pausa, reinicio o regreso al menú.
- Menús: cruceta para navegar, A para elegir y B para volver.
- Editor de equipo: cruceta arriba/abajo selecciona un hueco; izquierda/derecha cambia el Pokémon; START inicia el combate.
- Ajustes: izquierda/derecha cambia arena, nivel de CPU y tamaño del texto.

## Compilar

Con devkitARM, libctru, citro3d, citro2d, `tex3ds` y Pillow:

```sh
make -C native
```

El proceso de compilación convierte los fondos y las hojas de animación en un
atlas RGBA4 para reducir el uso de memoria en Old 2DS.

La acción de GitHub `Build Nintendo 3DS homebrew` compila `.3dsx`, `.cia`, `.elf` y `.smdh` como artefactos.
