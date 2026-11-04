# Prototipo homebrew para 3DS

Primera implementación nativa de combate local para el perfil Old 2DS. El mismo `.3dsx` y `.cia` está pensado para New 3DS XL.

Incluye un menú, pelea de entrenamiento 3 contra 3 contra CPU, tres Pokémon por equipo, cambio de miembro, ataque básico, tres habilidades, esquiva, medidor de definitivo y suministros. Los sprites están dibujados para este prototipo en pixel art; no se copiaron las imágenes del sitio web.

## Controles

- Circle Pad: movimiento.
- A: ataque básico.
- B, X, Y: habilidades.
- L: esquiva.
- R: definitivo cuando el medidor llega al 100%.
- Cruceta izquierda/derecha: cambiar de Pokémon.
- Pantalla táctil: acciones y selección de equipo.

## Compilar

Con devkitARM, libctru, citro3d, citro2d y las herramientas de 3DS:

```sh
make -C native
```

La acción de GitHub `Build Nintendo 3DS homebrew` adjunta `.3dsx`, `.cia`, `.elf` y `.smdh` como artefacto.
