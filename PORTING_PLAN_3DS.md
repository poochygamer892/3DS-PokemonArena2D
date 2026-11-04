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

## Referencia: PokéSurvivor 3DS

Se descargaron desde Drive y se inspeccionaron estáticamente los dos paquetes proporcionados:

| Paquete | Tamaño | SHA-256 |
|---|---:|---|
| `pokesurvivor-3ds.3dsx` | 41,307,932 bytes | `f5625764b2c087d655174e1c19060cb5ec0973b19cb974b4762673f4e31687d3` |
| `pokesurvivor-3ds.cia` | 40,809,408 bytes | `6d7f066fb4a12ed741ae604dd7450809b6e704a930f5303c4011b9ace059b523` |

El `.3dsx` se reconoce como una aplicación homebrew de Nintendo 3DS. Los metadatos SMDH embebidos en ambos archivos muestran el título `PokéSurvivor 3DS` y el publicador `juanluismm520`. El CIA contiene el Title ID `000400000F5A3D00` y el código de producto `CTR-H-PSV3`.

Las cadenas estáticas nombran `curl` y Mbed TLS de devkitPro, el servicio HTTP de 3DS, configuración en `sdmc:/3ds/pokesurvivor-3ds/` y un flujo de consulta/descarga de actualizaciones. Esto ofrece una referencia de empaquetado y actualización; no prueba que esas funciones se ejecuten en cada modo. No ejecuté los paquetes.

El binario acredita a `juanluismm520` como autor del port y contiene un enlace a `juanluismm521/pokesurvivor3ds`, que no se pudo resolver en GitHub. El repositorio público relacionado que sí apareció, [`DanFranza/pokesurvivor3DS`](https://github.com/DanFranza/pokesurvivor3DS), es un fork del juego web original [`JPTER78/pokesurvivor`](https://github.com/JPTER78/pokesurvivor): su README describe HTML/CSS/JavaScript sobre Canvas 2D y no incluye el código nativo que produjo estos paquetes. La licencia del código web actual es PolyForm Noncommercial; sus recursos también tienen licencias propias. Por ahora se toma el paquete como referencia técnica, sin copiar código ni assets.

REA no admite estos formatos para `inspect_artifact`; el reconocimiento de 3DSX, el encabezado CIA, SMDH, hashes y cadenas se obtuvieron mediante inspección estática local.

La nota de análisis estático está en [`REVERSE_ENGINEERING_NOTES.md`](REVERSE_ENGINEERING_NOTES.md).
