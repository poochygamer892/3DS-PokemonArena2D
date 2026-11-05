# Análisis de Poke2D Arena para una adaptación a 3DS

Fecha de inspección: 4 y 5 de noviembre de 2026. Referencias: [sitio](https://poke2darena.com/), [juego web](https://poke2darena.com/play-online/) y sus guías de [controles](https://poke2darena.com/guides/how-to-play-controls/), [reglas de combate](https://poke2darena.com/guides/battle-rules/) y [modos](https://poke2darena.com/guides/story-mode/).

## Identidad y método

Poke2D Arena es el juego web que se está adaptando a Nintendo 3DS. Es distinto de la build Unity de Empress Pokémon 2D Arena analizada anteriormente en [`REVERSE_ENGINEERING_NOTES.md`](REVERSE_ENGINEERING_NOTES.md); no se afirma que ambos compartan código o autoría.

Se inspeccionaron la página y el bundle cliente JavaScript, se leyeron las guías públicas y se ejecutó Quick Battle en Chromium. Se observó una partida local contra CPU; el dominio del servidor online y los anuncios/analítica se bloquearon durante esa sesión. No se creó ni se unió a una sala online.

El sitio está hecho con Astro y carga el juego desde un bundle JavaScript minificado de unos 2,65 MB, junto con CSS. El cliente contiene referencias a Phaser, Canvas/WebGL, almacenamiento local, WebSocket y WebRTC (`RTCPeerConnection`); esto concuerda con el lienzo y con los modos documentados, pero no demuestra que cada ruta de red se use en todo modo. No se encontró source map. El lienzo principal observado en Chromium medía 648×408 píxeles y se mostraba a doble escala.

## Lo que se observó al jugar

El menú ofrece Quick Battle, Vs CPU, Story Mode, Your Teams, Online y Settings. La primera opción inicia una pelea de entrenamiento; en la sesión observada Pikachu luchó contra Gyarados en la arena volcánica. La pantalla tiene vista cenital pixel art, paneles de Pokémon y vida en la parte superior, retratos del equipo a la izquierda, reloj/round en el centro y acciones en una barra inferior. La pelea avanzó en tiempo real y la vida de Pikachu bajó durante el intercambio.

La sesión más reciente volvió a confirmar que Quick Battle usa la arena volcánica, muestra los equipos en retratos a la izquierda y coloca el HUD de ambos Pokémon en la parte superior. El menú del juego incluye Quick Battle (Training 3 vs 3), Vs CPU, Story Mode, Arena Run, Your Teams y Online. Los recursos artísticos y el código pueden tener permisos y licencias distintos.

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

## Adaptación para 3DS

La consola tiene menos espacio y entradas distintas al navegador. La adaptación compatible con Old 2DS usa este mapeo de controles:

| Acción | Control propuesto |
|---|---|
| Movimiento | Circle Pad |
| Ataque básico / movimientos 1–3 | A / B / X / Y |
| Esquiva / definitivo | L / R |
| Cambio de Pokémon | Cruceta o retratos táctiles |
| Apuntado | Autoapuntado al rival más cercano; arrastre táctil para ajustar dirección |

La adaptación actual muestra la arena y combatientes en la pantalla superior (400×240), con HUD compacto arriba. La inferior (320×240) muestra miembros del equipo, medidor definitivo y botones táctiles para reemplazar parte del ratón/joystick. Este layout es una decisión del port, no el diseño web original.

La primera entrega jugable se limita a una pelea offline 3 contra 3 contra CPU, una arena y seis Pokémon. Story, Arena Run, equipos configurables, Online y 3 Player Brawl siguen pendientes.

## Límites pendientes

- El bundle está minificado y no tiene source map visible. La cadena `/data/catalog.json` devolvió HTTP 410 durante la inspección; no se exportó el catálogo completo.
- No se recorrieron todos los movimientos, arenas, etapas de Historia, opciones de Settings ni flujos online. Las cifras de las reglas provienen de las guías públicas.
- El proyecto incluye una selección de la arena y las hojas de Idle/Walk/Shoot que descarga el propio juego web, con su procedencia anotada en [`native/assets/poke2d/ATTRIBUTION.md`](native/assets/poke2d/ATTRIBUTION.md). No incluye el bundle JavaScript, el código de combate web, sonidos, efectos ni el catálogo completo.
