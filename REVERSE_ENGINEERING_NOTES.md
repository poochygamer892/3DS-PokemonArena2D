# REA: notas iniciales de análisis

Objetivo del port: una versión jugable en Old 2DS. Estas notas describen lo que se pudo comprobar de los archivos 4.3.0 y separan los datos observados de las inferencias. No se copiaron código ni recursos al proyecto.

## Archivos analizados

Los dos archivos del Drive se descargaron por HTTP directamente, sin el conector:

| Archivo | Tamaño | SHA-256 |
|---|---:|---|
| `Empress-Pokemon-Arena-4.3.0-Windows.zip` | 447,488,842 bytes | `bdb1152abbe2397e6b7b50ad546ab961424e852359bc23f709f0e96c97053ec0` |
| `Empress-Pokemon-Arena-4.3.0-Android.apk` | 474,830,781 bytes | `34da7ec02840ea655c90665931db16ca422ac32f4cd1539c7c6eeb11b1dd2943` |

REA inventarió ambos contenedores sin contradicciones de integridad. El ZIP incluye `Empress Pokemon 2D Arena.exe`, `GameAssembly.dll`, `UnityPlayer.dll` y `global-metadata.dat`. El APK incluye una biblioteca IL2CPP ARM64 y sus propios metadatos. Los hashes de `global-metadata.dat` son distintos entre las compilaciones Windows y Android, así que se tratan como variantes separadas.

## Observaciones estáticas

- Es una aplicación Unity IL2CPP. La compilación Windows es PE x64; REA emparejó sus metadatos IL2CPP versión 39 con `GameAssembly.dll`. El emparejamiento de módulos, cantidades de métodos y tokens concuerda estructuralmente.
- El ensamblado del juego `Assembly-CSharp.dll` contiene 1,717 tipos. Los espacios de nombres propios incluyen `Empress.Combat`, `Empress.Arena`, `Empress.Interface`, `Empress.Network`, `Empress.Story`, `Empress.Raid`, `Empress.Dungeon`, `Empress.Frontier` y `Empress.Replay`.
- `BattleInput` contiene desplazamiento (`Dx`, `Dy`), apuntado (`AimX`, `AimY`), ranura de ataque, ranura de cambio, esquiva y megaevolución. `BattleEngine` expone `Input`, `ApplyInput`, `Step(dt)`, `Dodge`, `SwitchPokemon`, `Cast`, `UpdateAction`, `Release`, `Hit` y cálculo de daño mediante `BattleRules.CalculateDamage`.
- El estado de combate modela equipos y plantillas, criaturas activas, posición y velocidad, HP, estadísticas, movimientos, estados alterados, proyectiles y eventos. La arena tiene ancho y alto, obstáculos, agua y colisiones; hay métodos para movimiento, colisión y comprobar línea de visión.
- La interfaz tiene un pad de movimiento táctil, arrastre para apuntar/atacar y lectura de gestos de cámara. `BattleControls` también guarda apuntado de mando, por lo que el juego contempla controles físicos además del toque.
- Los tipos `BattleRules` contienen catálogos de especies, movimientos, arenas, tipos y habilidades. El código también nombra AI, replay, snapshot, red, equipos, salas y combate online.

## Cómo parece jugarse

**Inferencia con confianza alta:** el combate principal es de acción en tiempo real dentro de arenas 2D. El jugador mueve a la criatura activa, apunta y lanza movimientos, esquiva y puede cambiar de criatura. Los ataques pueden crear proyectiles y resolver impactos con reglas de daño basadas en tipo/estadísticas. Esto se deduce de los nombres, campos y firmas IL2CPP; no se observó una partida en ejecución.

**Aún sin confirmar:** ritmo y duración de las animaciones, número de criaturas por equipo en los modos principales, distribución exacta de botones en pantalla, valores de daño/cooldown, reglas de victoria por modo y el orden de los menús. Las clases indican que existen más modos y servicios, pero no prueban qué está habilitado o cómo se presenta al iniciar el juego.

## Consecuencias para Old 2DS

No es un port binario directo: Unity/IL2CPP compiló el ejecutable Windows para x64 y Android para ARM64, arquitecturas distintas de Old 2DS. La ruta realista es reimplementar el bucle de combate y la presentación como homebrew C/C++ (por ejemplo, `libctru` y `citro2d`) y adaptar movimiento/apuntado a Circle Pad, cruceta, botones y, si conviene, pantalla táctil.

Para un primer prototipo recomiendo limitar el alcance a combate local contra CPU, una arena, equipos pequeños, movimiento, apuntado/ataques, esquiva/cambio y condición de victoria. Red, replays, historia, raids y otros modos pueden quedar para después. Las dos pantallas del 2DS permiten reservar la inferior para equipo, vida y botones contextuales, o presentar una arena más grande usando ambas con una interfaz compacta.

## Límites y trazabilidad

El análisis realizado es estático: REA leyó el inventario y los metadatos, pero no se observó el flujo de instrucciones de las funciones ni una partida. En este entorno no están configurados Ghidra/IDA/Hopper/Rizin, y no hay Wine ni emulador Android/3DS detectado. Por eso las reglas exactas y la experiencia visual quedan abiertas para una fase posterior con descompilación nativa o captura de una partida.

Evidencias REA principales:

- ZIP Windows: `ev_1786a45c4e531fdd8cc0ce6ce3104801e501dda52fca7150f8723cad0319e50f`; inventario: `ev_6ee08c6a0fe711d74836388d4c596a799224824617a23b7674046cd8187ae53a`.
- APK Android: `ev_1265776efb31524e1ec4510433bf3e7ca611112f18f3931b6aad52d2962ecf7a`; inventario: `ev_180342581a0de7e7bc594a5ca64686ff168fe85c22eed15f01893458571d07f6`.
- Metadatos IL2CPP Windows: `ev_ae83d4ad3d2e8767d9cbffcfbb370843bab74aa0663463ff7feaac32ab7b8d8e`.
- Vistas de tipos y métodos del ensamblado del juego: `ev_92a2d2bc9884914ec3b99cfc33afd7ec6d40047bb0879f587ea87f3f328c7456`, `ev_a9762dd78ac210b2af7deff61bf6e14e31b8746b75070e790ee789b74f9c702c`.
