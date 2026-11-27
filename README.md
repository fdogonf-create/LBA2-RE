# Little Big Adventure 2 para Nintendo Switch

**Little Big Adventure 2** (también conocido como *Twinsen's Odyssey*) es la aventura de 1997 de Adeline Software, la secuela de *Little Big Adventure*.

Este repositorio es un port del juego a **Nintendo Switch** (homebrew), hecho sobre [LBA2 Classic Community](https://github.com/LBALab/lba2-classic-community), el motor original de 1997 liberado por sus dueños y mantenido por la comunidad. Aquí le sumamos lo necesario para jugarlo cómodo en la consola: controles pensados para el mando de Switch, una cámara mejor, filtros de imagen, mapas en alta definición, una videoteca de cinemáticas y bastante más.

> **No incluye el juego.** Necesitas tu propia copia original de LBA2 ([GOG](https://www.gog.com/game/little_big_adventure_2), [Steam](https://store.steampowered.com/app/398000/Little_Big_Adventure_2/) o el CD) y copiar sus archivos a la tarjeta SD.

<!-- CAPTURA: imagen principal del juego corriendo en la Switch (portada) -->

## Qué trae este port

### Controles de Switch

- Los botones siguen las letras de Nintendo: **A** habla, busca y abre; **B** lanza el arma; **X** hace la acción del comportamiento; **Y** abre el holomapa.
- En los menús, **A** acepta y **B** vuelve atrás.
- **Stick izquierdo** para mover a Twinsen de forma analógica, relativo a la cámara.
- **R** abre un menú rápido de objetos, sin pasar por el inventario.
- Cuando hay algo con qué interactuar, aparece en la esquina un **icono del botón A**. En escaleras y ventanas aparece el **icono del stick hacia arriba con la palabra "Trepar"**, y con A Twinsen se gira hacia la pared y empieza a subir.
- En el buggy, **ZR** acelera y **ZL** frena y da marcha atrás.

<!-- CAPTURA: icono del botón A y el icono de "Trepar" con el stick -->

### Cámara mejorada

- La cámara automática sigue a Twinsen en exteriores y se gira con el **stick derecho**.
- **Zoom** con la cruceta (arriba acerca, abajo aleja). El zoom que elijas se guarda entre escenas y entre partidas.
- La cámara mantiene el ángulo que le diste y se acerca sola cuando hay un edificio detrás, en vez de quedarse mirando el techo.
- **Modo de prueba de la cámara:** si pones un archivo llamado `tester_camara.txt` en `sdmc:/lba2/` (o en `sdmc:/switch/lba2cc/user/`), se muestran en pantalla la altura, la distancia y el giro de la cámara. Sirve para ajustar los valores. Borra el archivo para quitarlo.

### Imagen y videos

- **Filtros de escalado** en Opciones → Pantalla → Escalado: *Pixel art*, *Suavizado*, *Tele antigua* y *Suavizado + tele*. **Por defecto viene Suavizado + tele**: suaviza los bordes y le da un toque de tele de tubo.
- **Videos a pantalla completa por defecto.** Las cinemáticas llenan la pantalla. Si quieres verlas en su tamaño original, cámbialo en Opciones.
- Soporta **cinemáticas en alta calidad**: si pones videos de reemplazo (MPEG-1) en `sdmc:/lba2/video/hd/`, se ven a todo color en lugar de los originales.

<!-- CAPTURA: comparación de los filtros de escalado -->

### Videoteca

- Un nuevo menú **Videoteca** para volver a ver las cinemáticas que ya viste en la historia.
- Las que todavía no has visto aparecen como `? ? ?`. Cuenta también las de tus partidas guardadas de antes, sin tener que cargarlas.
- Está en el menú principal antes de empezar una partida, y dentro de Opciones (desde el menú principal) cuando ya hay una partida en curso.

<!-- CAPTURA: menú de la Videoteca -->

### Holomapa

- **Mapas de las islas en alta definición**, incluidos **mapas panorámicos 16:9** que llenan también las franjas negras de los lados.
- **Zoom** con el stick derecho sobre el mapa de una isla, y paso de la esfera al mapa con un **fundido** suave.
- Puedes mirar el mapa de cualquier isla que ya conozcas y pasar de una a otra con **L** y **R** (solo para mirar, Twinsen no se mueve).
- La **barra del nombre** de abajo ahora ocupa todo el ancho del mapa.

<!-- CAPTURA: holomapa con un mapa HD panorámico -->

### Guardado y cambios de escena

- **Las opciones se guardan solas** al salir de cada página, incluso si cierras el juego desde el menú HOME.
- Un **icono animado de "Guardando"** aparece en la esquina cada vez que el juego guarda. Puedes poner tu propio icono (ver más abajo).
- **Cambios de escena más fluidos:** los archivos del juego se cargan en memoria la primera vez y el guardado automático se escribe en segundo plano, así se espera menos al pasar de una zona a otra.

<!-- CAPTURA: icono de "Guardando" -->

### Otras cosas

- El juego arranca **en español** por defecto.
- Usa el modo de CPU más rápido de la Switch para que los exteriores vayan más suaves.
- **Preparación para doblaje:** el script `scripts/dev/lba2_dub.py` exporta las voces, los videos y el guion del juego, y vuelve a empaquetar las voces dobladas. `scripts/dev/smk_decode.py` saca los videos originales como imágenes y audio para poder remasterizarlos.

## Cómo instalarlo

1. **Descarga el paquete.** Entra en la pestaña [Actions](https://github.com/fdogonf-create/LBA2-RE/actions) del repositorio, abre la última compilación correcta de *Nintendo Switch* o *Port Switch + Windows* y baja el archivo `lba2cc-switch` (o `port-switch`). Es un `.zip`.
   <!-- Cuando haya un Release, poner aquí el enlace directo -->
2. **Cópialo a la SD.** Descomprime el zip en la raíz de la tarjeta SD. El juego queda en `sdmc:/switch/lba2cc/lba2cc.nro`.
3. **Copia los archivos del juego.** Pon los archivos de tu LBA2 original (`LBA2.HQR` y los demás `.HQR`, junto con sus carpetas de música, videos y voces) directamente en `sdmc:/lba2/`.
4. **(Opcional) Mapas HD e icono de guardado.** Copia los mapas del holomapa (`holomap_22.png`, `holomap_42.png`, etc.) y, si quieres, tu propio `guardando.png` en `sdmc:/lba2/hd/`.
5. **Abre el juego** desde el Homebrew Menu **en modo aplicación** (manteniendo R al abrir un juego), no desde el Álbum, que tiene muy poca memoria.

Así queda la tarjeta SD:

```text
sdmc:/
├── switch/
│   └── lba2cc/
│       ├── lba2cc.nro        ← el juego
│       └── user/             ← partidas guardadas, opciones y registro
└── lba2/                     ← archivos de tu LBA2 original (LBA2.HQR, ...)
    ├── hd/                   ← mapas HD del holomapa y guardando.png (opcional)
    └── video/
        └── hd/               ← cinemáticas en alta calidad (opcional)
```

> **Antes de actualizar**, haz una copia de `sdmc:/switch/lba2cc/user/`: ahí están tus partidas y tus opciones.

### Icono de "Guardando" propio

El archivo `sdmc:/lba2/hd/guardando.png` es una tira horizontal de cuadros del mismo tamaño, con fondo transparente (por ejemplo, 8 cuadros de 64×64 en una imagen de 512×64). Se reproduce a 12 cuadros por segundo. Si no está, se ve un círculo de puntos girando.

## Controles

| Botón | Jugando | En los menús |
| --- | --- | --- |
| **A** | Hablar, buscar, abrir, trepar | Aceptar |
| **B** | Lanzar el arma | Volver |
| **X** | Acción del comportamiento | |
| **Y** | Holomapa | |
| **L** | Menú de comportamientos | En el holomapa: isla anterior |
| **R** | Menú rápido de objetos | En el holomapa: isla siguiente |
| **ZL** | Cambiar al siguiente comportamiento (en el buggy: frenar) | |
| **ZR** | Hechizo de protección (en el buggy: acelerar) | |
| **−** | Inventario | |
| **+** | Menú del juego | |
| **Stick izquierdo** | Mover a Twinsen (pulsado: esquivar) | Moverse por el menú |
| **Stick derecho** | Girar la cámara (pulsado: centrar la cámara) | Moverse en el inventario y comportamientos; zoom en el holomapa |
| **Cruceta** | Arriba y abajo: zoom de la cámara; izquierda y derecha: atajos | Moverse por el menú |

<!-- IMAGEN: esquema del mando con los botones -->

## Opciones

Las opciones nuevas o cambiadas para Switch, todas dentro del menú **Opciones**:

| Opción | Valores | Por defecto en Switch |
| --- | --- | --- |
| Pantalla → Escalado | Directo, Lineal, Pixel art, Suavizado, Tele antigua, Suavizado + tele | Suavizado + tele |
| Tamaño de los videos | Ajustados a pantalla, A pantalla completa | A pantalla completa |
| Cámara → Exteriores | Auto, Clásica | Auto |
| Idioma | Español, English, Français, Deutsch, Italiano, Português | Español |
| Videoteca | Ver las cinemáticas ya vistas | |

Todo se guarda solo en `sdmc:/switch/lba2cc/user/`.

## Capturas

<!-- Felipe agregará las capturas aquí para el Release -->

*Próximamente.*

## Compilar el juego

### Requisitos

- Para el `.nro` de Switch: solo Docker.
- Para PC: CMake 3.23 o más nuevo, Ninja, un compilador de C/C++ con soporte de C++98 (GCC o Clang) y SDL3. GNU Make es opcional (atajos `make build`, `make run`).

### Switch

Para compilar el `.nro` tú mismo:

```bash
bash scripts/dev/build-switch.sh
```

Esto genera el `.nro` y el zip para la SD dentro de una imagen fija de devkitPro, sin instalar nada más. Cada cambio que se sube a GitHub también se compila solo en Actions. Los detalles técnicos están en [docs/SWITCH.md](docs/SWITCH.md), [docs/CONTROLLER.md](docs/CONTROLLER.md), [docs/CAMERA.md](docs/CAMERA.md) y [docs/GFX_OPTIONS.md](docs/GFX_OPTIONS.md).

### PC y otras plataformas

Para compilar en PC (Linux, macOS, Windows) y el resto de plataformas, sigue las instrucciones del proyecto original: [LBA2 Classic Community](https://github.com/LBALab/lba2-classic-community#building-from-source-quick-start).

## Créditos

Este port existe gracias al trabajo de mucha gente antes que nosotros:

- **Adeline Software International** (Lyon, Francia), que creó el juego en 1997.
- **[2.21](https://github.com/2point21/lba2-classic)**, dueños actuales de la propiedad intelectual, que liberaron el código fuente original.
- **[LBALab y la comunidad de LBA2 Classic Community](https://github.com/LBALab/lba2-classic-community)**, que portaron el motor a C++ y a sistemas modernos (incluido el port base a Switch). Todo este proyecto está construido encima de su trabajo.
- **Felipe**, adaptación para Switch y las funciones de este repositorio.

### Equipo original de desarrollo

* **Dirección:** Frédérick Raynal
* **Programación:** Sébastien Viannay / Laurent Salmeron / Cédric Bermond / Frantz Cournil / Marc Bureau du Colombier
* **Artistas 3D y animación:** Paul-Henri Michaud / Arnaud Lhomme
* **Artistas:** Yaeël Barroz, Sabine Morlat, Didier Quentin
* **Historia y diseño:** Frédérick Raynal / Didier Chanfray / Yaël Barroz / Laurent Salmeron / Marc Albinet
* **Diálogos:** Marc Albinet
* **Programación de la historia:** Frantz Cournil / Lionel Chaze / Pascal Dubois
* **Secuencias de video:** Frédéric Taquet / Benoît Boucher / Ludovic Rubin / Merlin Pardot
* **Música y efectos de sonido:** Philippe Vachey
* **Pruebas:** Bruno Marion / Thomas Ferraz / Alexis Madinier / Christopher Horwood / Bertrand Fillardet
* **Control de calidad:** Emmanuel Oualid

El comando `credits` de la [consola](docs/CONSOLE.md) muestra los créditos originales completos.

### Enlaces de la comunidad

* **Web oficial:** https://twinsenslittlebigadventure.com/
* **Discord:** https://discord.gg/jsTPWYXHsh
* **Documentación:** https://lba-classic-doc.readthedocs.io/

## Licencia

El código fuente está bajo la [Licencia Pública General de GNU (GPL)](LICENSE), igual que el proyecto original.

La licencia solo cubre el código del motor. Los archivos del juego (gráficos, modelos, texturas, música, voces, videos) **no** son libres y no se pueden distribuir: por eso tienes que usar tu propia copia.

La propiedad intelectual pertenece actualmente a [2.21]. Desarrollado originalmente por Adeline Software International.

---

## English summary

**Little Big Adventure 2 for Nintendo Switch** is a homebrew port built on top of [LBA2 Classic Community](https://github.com/LBALab/lba2-classic-community), the community-maintained release of the original 1997 engine. No game assets are included: you need your own copy of LBA2.

What this fork adds for the Switch: Nintendo button layout with on-screen A-button and stick "climb" hints, a quick item menu on R, an improved follow camera with saved zoom and a debug readout (`tester_camara.txt`), shader scaling filters (Pixel art, Smooth, Old TV, Smooth + old TV, the latter by default), full-screen movies by default with optional HD replacement cinematics, a "Videoteca" movie library, HD and 16:9 widescreen holomap maps with zoom and fades, settings that save themselves, an animated "Saving" icon, smoother scene changes (game data cached in memory, background saves), and tooling to export and repack voices and videos for dubbing.

To install, grab the `lba2cc-switch` zip from [Actions](https://github.com/fdogonf-create/LBA2-RE/actions), extract it to the SD card root, copy your game files to `sdmc:/lba2/` (optional HD maps and `guardando.png` go in `sdmc:/lba2/hd/`), and launch from the Homebrew Menu in application mode. Saves and settings live in `sdmc:/switch/lba2cc/user/`.

All credit for the original game goes to Adeline Software International, to 2.21 for releasing the source, and to LBALab and the LBA2 Classic Community contributors for the engine port this work builds on. Licensed under the GPL; game assets are not covered.
