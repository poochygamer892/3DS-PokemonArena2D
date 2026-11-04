# Port a Nintendo 3DS: estado y ruta

## Estado actual

El proyecto conserva dos referencias distintas: las compilaciones finales de Empress Pokémon 2D Arena para Windows (PE x64 + Unity IL2CPP) y Android (IL2CPP ARM64), y el juego web Poke2D Arena en `poke2darena.com`. El README mantiene la atribución que pidió el usuario a Empress; el análisis web más reciente es una referencia separada. No se dispone del proyecto fuente de Unity ni del código fuente del juego web.

`.3dsx` y `.cia` son formatos de programas nativos de 3DS. No pueden envolver directamente la compilación Unity x64/ARM64 ni la aplicación web. Hace falta implementar el juego como programa nativo para 3DS. El perfil objetivo es Old 2DS; New 3DS XL puede ejecutar el mismo binario.

## Ruta técnica

1. Implementar el bucle nativo en C con devkitARM/libctru, citro3d y citro2d. El primer prototipo está en [`native/`](native/).
2. Completar la pelea offline: movimiento, ataques, HP, enfriamientos, cambios, esquiva, definitivo y suministros.
3. Compilar `.3dsx` y empaquetar el mismo juego como `.cia`; `.github/workflows/build-3ds.yml` hace esta compilación en GitHub Actions.
4. Comprobar ambos formatos en emulador o consola. Añadir Historia, editor de equipos, más Pokémon y arenas después del combate local.

## Bloqueos de compilación

El host local no tiene una toolchain 3DS ejecutable. Docker descargó imágenes de devkitPro, pero no pudo crear sus sistemas de archivos por falta de espacio; por eso la compilación se trasladó a GitHub Actions. Todavía no hay binarios. La primera versión contiene un equipo reducido y una arena dibujada en el código; usa sprites Pokémon propios para el prototipo, no los sprites o escenarios de Poke2D Arena.

## Referencia web: Poke2D Arena

La página y una batalla Quick Battle se inspeccionaron en navegador. La guía describe equipos de tres, un Pokémon activo por lado, seis arenas, 18 tipos, Quick Battle, Vs CPU, Story y Online. El cliente usa un lienzo observado de 648×408 píxeles; la documentación de reglas ofrece datos concretos de cooldowns, cambios, esquiva, estados, suministros y combate. La nota completa, con observaciones separadas de propuestas y puntos por confirmar, está en [`POKE2D_WEB_ANALYSIS.md`](POKE2D_WEB_ANALYSIS.md).

Los modos online y 3 Player Brawl quedan fuera del primer objetivo: el juego web depende de servicios remotos y coordinación WebSocket/WebRTC, que no son necesarios para validar el combate nativo local.

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
