# Análisis de Poke2D Arena para una adaptación a 3DS

Fecha de inspección: 4 de noviembre de 2026. Referencias: [sitio](https://poke2darena.com/), [juego web](https://poke2darena.com/play-online/) y sus guías de [controles](https://poke2darena.com/guides/how-to-play-controls/), [reglas de combate](https://poke2darena.com/guides/battle-rules/) y [modos](https://poke2darena.com/guides/story-mode/).

## Identidad y método

Poke2D Arena es un juego web distinto de la build Unity de Empress Pokémon 2D Arena analizada en [`REVERSE_ENGINEERING_NOTES.md`](REVERSE_ENGINEERING_NOTES.md). Esta nota trata `poke2darena.com` como referencia adicional; no afirma que ambos juegos compartan código o autoría.

Se inspeccionaron la página y el bundle cliente JavaScript, se leyeron las guías públicas y se ejecutó Quick Battle en Chromium. Se observó una partida local contra CPU; el dominio del servidor online y los anuncios/analítica se bloquearon durante esa sesión. No se creó ni se unió a una sala online.

El sitio está hecho con Astro y carga el juego desde un bundle JavaScript minificado de unos 2,65 MB, junto con CSS. El cliente contiene referencias a Phaser, Canvas/WebGL, almacenamiento local, WebSocket y WebRTC (`RTCPeerConnection`); esto concuerda con el lienzo y con los modos documentados, pero no demuestra que cada ruta de red se use en todo modo. No se encontró source map. El lienzo principal observado en Chromium medía 648×408 píxeles y se mostraba a doble escala.

## Lo que se observó al jugar

El menú ofrece Quick Battle, Vs CPU, Story Mode, Your Teams, Online y Settings. La primera opción inicia una pelea de entrenamiento; en la sesión observada Pikachu luchó contra Gyarados en la arena volcánica. La pantalla tiene vista cenital pixel art, paneles de Pokémon y vida en la parte superior, retratos del equipo a la izquierda, reloj/round en el centro y acciones en una barra inferior. La pelea avanzó en tiempo real y la vida de Pikachu bajó durante el intercambio.

Las imágenes, sprites, sonidos y efectos que aparecen en el sitio no se copiaron al repositorio. La página se identifica como un juego fan no oficial; sus recursos pueden tener licencias distintas del código.

## Reglas que describen las guías

- Quick Battle, Vs CPU y Online usan equipos de tres con un Pokémon activo por lado. Story es 1 contra 1. 3 Player Brawl coloca tres equipos en la misma arena. La web anuncia 18 tipos, seis arenas y 1.066 entradas de roster; esos catálogos no se verificaron entrada por entrada.
- Cada Pokémon tiene ataque básico, tres movimientos configurables, un definitivo y esquiva. En escritorio se mueve con WASD/flechas, apunta con el ratón, usa clic izquierdo para el ataque básico, clic derecho para otra habilidad, Space para esquivar, Q para otra habilidad, E para el definitivo y R o 1/2/3 para cambiar de miembro. En móvil hay joystick virtual, botones táctiles y arrastre para apuntar.
- El ataque básico tiene 0,5 s de enfriamiento. El golpe de contacto quita aproximadamente 3,2% de la vida máxima; el proyectil especial tiene alcance de 380 px y quita aproximadamente 2,5%. Contra una inmunidad elemental, el básico aún hace la mitad del daño.
- Los movimientos tienen enfriamientos individuales. La guía da ejemplos de 1,8 s para ataques rápidos, 3,2 s para ataques de potencia 90, 4–4,5 s para estados, 5 s para Hurricane/Earthquake y 5,5 s para Double-Edge/Flare Blitz. STAB multiplica por 1,5; efectividad, resistencia e inmunidad usan 2×, 0,5× y 0×, multiplicados para defensas de doble tipo.
- La esquiva dura 0,3 s, da invulnerabilidad y recarga en 2,5 s. Cambiar de Pokémon recarga en 3,5 s; el relevo tarda unos 0,8 s y da 0,2 s de invulnerabilidad. La vida del Pokémon que se retira se conserva. Un cambio no está permitido durante un lanzamiento o mientras esté en enfriamiento.
- El medidor del definitivo empieza en 50%; se carga al infligir y recibir daño. La guía estima unos 0,3 puntos porcentuales por cada 1% de vida máxima afectada, con un mínimo de 1 por impacto. Un suministro de energía añade 20 puntos.
- Los suministros empiezan a caer a los 10 s y luego cada 18 s. La advertencia aparece 2,5 s antes y el objeto desaparece tras 12 s. Energía (40%) añade 20% al definitivo; curación (30%) restaura 20% de vida; velocidad (30%) multiplica el movimiento por 1,3 durante 6 s.
- Protect bloquea daño directo durante 1,2 s. Quemadura dura 20 s y causa daño cada 4 s; parálisis ralentiza 50% durante 3 s; sueño inmoviliza e impide atacar durante 2,8 s y cancela un lanzamiento en curso.
- La CPU tiene Easy, Normal y Hard. La guía publica tiempos de reacción de 0,22/0,13/0,08 s, error de apuntado de 22/12/5 px y tendencia de esquiva de 25/50/75%. Estos números describen la guía, no una medición independiente de cada decisión de la IA.
- Las peleas offline avanzan sin límite de tiempo; Online usa 90 s. El brawl de tres jugadores termina por eliminación o compara la vida total restante al agotarse el reloj.

## Propuesta de adaptación para 3DS

La consola tiene menos espacio y entradas distintas al navegador. Un primer prototipo compatible con Old 2DS podría usar:

| Acción | Control propuesto |
|---|---|
| Movimiento | Circle Pad |
| Ataque básico / movimientos 1–3 | A / B / X / Y |
| Esquiva / definitivo | L / R |
| Cambio de Pokémon | Cruceta o retratos táctiles |
| Apuntado | Autoapuntado al rival más cercano; arrastre táctil para ajustar dirección |

Propuesta de pantalla: arena y combatientes en la pantalla superior (400×240), HUD compacto arriba y controles, equipo, vida y enfriamientos en la inferior (320×240). Esto conserva la acción principal y usa la pantalla táctil para sustituir ratón/joystick. El layout es una decisión de port, no el diseño original.

La primera versión jugable debería limitarse a una pelea offline 3 contra 3 contra CPU, una arena y pocos Pokémon con movimientos de varios tipos. Después se pueden añadir equipos editables y Story. Online y 3 Player Brawl conviene dejarlos para una etapa posterior: dependen de servicios remotos y de coordinación de red, mientras que el objetivo inicial es validar el bucle de combate local en hardware modesto.

## Límites pendientes

- El bundle está minificado y no tiene source map visible. La cadena `/data/catalog.json` devolvió HTTP 410 durante la inspección; no se exportó el catálogo completo.
- No se recorrieron todos los movimientos, arenas, etapas de Historia, opciones de Settings ni flujos online. Las cifras de las reglas provienen de las guías públicas.
- No se incorporó al proyecto ningún bundle, sprite, efecto o sonido del sitio. La implementación 3DS requerirá código nativo y recursos propios o con permiso compatible con su redistribución.
