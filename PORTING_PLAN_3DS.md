# Port a Old 2DS: estado y ruta

## Estado actual

El material disponible son dos compilaciones finales: Windows (`GameAssembly.dll` PE x64 + Unity IL2CPP) y Android (IL2CPP ARM64). El repositorio no contiene código fuente de Unity, escenas ni recursos organizados como proyecto (`Assets/`, `Packages/`, `ProjectSettings/`).

`.3dsx` y `.cia` son formatos de programas nativos de 3DS. No son contenedores que puedan envolver una compilación Unity x64/ARM64. Para obtenerlos hay que construir un programa que use la arquitectura y las bibliotecas del 3DS. La compilación directa de Unity tampoco está disponible para este proyecto.

## Ruta técnica

1. Reimplementar el bucle de juego en C/C++ para Old 2DS con devkitARM/libctru y citro2d.
2. Empezar por un prototipo local: arena, controles físicos/táctiles, movimiento, ataque, daño, HP y victoria contra CPU.
3. Compilar el ejecutable homebrew y generar `.3dsx`; empaquetar el mismo programa como `.cia` con metadatos e icono.
4. Probar los dos formatos en emulador o en una consola Old 2DS.
5. Añadir los modos y contenido restantes por etapas.

## Bloqueos de compilación

La carpeta de trabajo no tiene aún código nativo y este entorno no tiene devkitARM, libctru, citro2d, `3dsxtool` ni `makerom`. Hace falta instalar la toolchain antes de producir cualquier paquete. Un primer paquete reproduciría solo el prototipo del paso 2; no sería una conversión completa del juego original.

Los paquetes de referencia adjuntos (`pokesurvivor-3ds.cia` y `pokesurvivor-3ds.3dsx`) llegaron como archivos Sediment mayores de 32 MiB; el mecanismo de descarga del entorno los rechaza por ese límite. Todavía no se ha inspeccionado su estructura ni su versión. Para analizarlos hace falta un enlace de descarga accesible (por ejemplo, Drive o GitHub Release) o copias menores de 32 MiB.

La nota de análisis estático está en [`REVERSE_ENGINEERING_NOTES.md`](REVERSE_ENGINEERING_NOTES.md).
