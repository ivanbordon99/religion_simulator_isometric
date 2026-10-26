# Marangatu: The Faith Simulator

**Marangatu: The Faith Simulator** es un simulador de religiones en vista isométrica 2.5D, escrito en C++20. El jugador funda un culto, nombra a su dios y observa cómo la fe se replica, se fusiona, se parte en cismas y atraviesa ocho eras históricas, desde el animismo primordial hasta el transhumanismo postbiológico.

*Marangatu* significa **sagrado / virtuoso** en guaraní: el título apunta a un mundo donde lo santo no es estático, sino que evoluciona con el territorio, la demografía y el conflicto.

![Mapa de fe por año](snapshot_year.png)

![Aldeanos en el mundo](snapshot_villagers.png)

![Personaje humano](snapshot_humano.png)

---

## Qué simula el juego

No es un 4X clásico ni un city-builder. El núcleo es **la vida de las fes** sobre un mapa procedural:

- **Replicación**: la fe se contagia de celda a celda según celo, templos y fricción del terreno (ríos y costas aceleran; montañas aíslan).
- **Aglutinación sincrética**: si dos religiones conviven generaciones con tolerancia mutua, pueden nacer una fe híbrida (nombre, color, doctrinas y símbolo mezclados).
- **Cismas**: la expansión geográfica y las fronteras culturales generan profetas reformadores (*Reformed*, *Orthodox*, *Pure*, *Covenant*, etc.).
- **Eras**: el calendario corre desde **-4000** hasta **2200+**. Cada era cambia mecánicas, amenazas y estructura social.
- **Crónica**: cada milagro, cisma, templo o extinción queda en una línea de eventos clicable; la cámara salta al epicentro.
- **Línea de tiempo**: play, pausa, 1× / 2× / 5× / 10×, y scrubbing por siglos con instantáneas del mapa de religiones.

El jugador interviene: fundar templos, forzar sincretismo o cismas, mover humanos, guardar mundos y usar comandos en partida.

---

## Modos de partida y creación de mundo

Desde el menú (**Crear mundo**, **Cargar mundo**, **Opciones**, **Salir**) se configura:

| Opción | Valores |
|---|---|
| Nombre del dios | p. ej. Solarius |
| Nombre de la religión | p. ej. Culto del Fuego Solar |
| Población inicial | aldeanos reales con IA y sprites (por defecto 80) |
| Era de inicio | cualquiera de las 8 eras |
| Semilla del mapa | generación procedural reproducible |
| Tamaño | 32×32, 48×48, 64×64 |
| Modo | Estándar, Sandbox, Teocracia total, Supervivencia |
| Dificultad de IA | Fácil, Normal, Difícil, Pesadilla |
| Color de influencia | Oro divino, Rubí sangre, Mónada azul, Esmeralda Gaia, Púrpura Dharma |
| Victoria | Hegemonía mundial (75% pop.), Teocrática (5 maravillas), Trascendencia (era 8), Tiempo (año 2200), Sin límite |
| Desastres | bajos / normales / frecuentes |
| Cismas / sincretismo | activables por separado |

Las partidas se guardan en `saves/` (no van al repositorio).

---

## Ocho eras históricas

1. **Primordial Animism** — bandas nómadas; cada río, fuego o tormenta tiene *anima*. Amenaza: extinción.
2. **Totemism & Ancestor Worship** — clanes, tótems, ritos funerarios. Amenaza: guerra tribal.
3. **Structural Shamanism** — agricultura, chamanes, megalitos. Amenaza: sequía y malas cosechas.
4. **Territorial Polytheism** — ciudades-estado, panteones, templos. Amenaza: guerras santas y corrupción sacerdotal.
5. **Universal Religions** — escrituras, misioneros, imperios. Amenaza: herejías y guerras de religión.
6. **Rationalization & Secularization** — Ilustración, ciencia, fe privada. Amenaza: escepticismo masivo.
7. **Post-Secular Age** — redes digitales, microcultos virales. Amenaza: nihilismo y polarización.
8. **Transhumanist Post-Biological** — inmortalidad digital, IA deificada. Amenaza: pérdida de lo humano.

---

## Mapa, biomas y vista isométrica

El continente se genera con un selector Multi-Noise determinista. Cada celda muestrea elevación, humedad y temperatura base continuas en `[-1, 1]`; la temperatura final aplica enfriamiento orográfico según la altura. Se seleccionan los dos objetivos ponderados más cercanos y se guarda un factor de mezcla para transiciones. La tabla de diez objetivos incluye océano, costa, desierto, sabana, llanuras, bosque templado, taiga, selva tropical, montaña nevada y pico rocoso. Para mantener compatibilidad con las mecánicas y sprites existentes, algunos objetivos climáticos comparten la categoría de juego (por ejemplo, taiga y tundra, o bosques templados y tropicales).

Biomas de juego:

Océano profundo, aguas costeras, llanuras, valles fértiles, bosques, colinas, desierto, montañas, tundra.

Cada celda conserva sus ejes climáticos, bioma primario/secundario y mezcla, además de capacidad de carga, demografía logística y conectividad comercial. Los guardados v2 incluyen estos datos y el cargador sigue aceptando partidas v1. La cámara usa proyección diamante `(x, y, h) → pantalla`, faldas de elevación y picking inverso por clic. El fondo alrededor del mapa usa el sprite espacial en mosaico de 256×128 (`assets/sprites/miscellany/space_background.png`). El atlas nuevo de terreno mide 1024×64 y contiene únicamente 16 tiles de suelo de 64×64; los árboles, arbustos y rocas se cargan como sprites independientes desde sus carpetas, sin compartir el atlas.

**Convención de sprites:** `assets/sprites/terrain/` para texturas de terreno y atlas; `units/<nombre_de_unidad>/` para cada unidad y sus animaciones; `trees/` para árboles y arbustos; `mountains/` para montañas y rocas; `buildings/` para edificios y templos; `miscellany/` para fondos y recursos variados. Los sprites nuevos deben colocarse en la categoría correspondiente, y cada unidad debe tener su propia subcarpeta dentro de `units/`.

**Modos de mapa:** `1` fes, `2` terreno, `3` calor de sincretismo, `4` eras. Hay niebla de guerra ligada a la fe.

---

## Controles

| Acción | Control |
|---|---|
| Seleccionar provincia / UI | Clic izquierdo |
| Panorámica | Clic derecho + arrastre, `W A S D` o flechas |
| Zoom | Rueda del ratón o `+` / `-` |
| Reset de cámara | `R` |
| Pausa | `Espacio` |
| Línea de tiempo | Arrastrar la barra inferior |
| Enfocar evento | Clic en una tarjeta de la crónica |
| Comandos / trucos | `Enter` en partida |
| Salir / atrás | `Esc` |

Intervenciones en HUD: **+ SINCRETISMO**, **+ CISMA**, **+ TEMPLO**.

---

## Compilar y ejecutar

El juego es multiplataforma y soporta compilación nativa tanto en **Linux** como en **Windows**.

### En Linux
Requisitos: `g++` o `clang++` con **C++20**, `make`, `libx11-dev` y `libgl1-mesa-dev`.

```bash
make test          # suite de pruebas unitarias
make all           # binario principal
DISPLAY=:0 ./bin/religion_sim
./bin/religion_sim --headless --ticks 250 --export simulation_snapshot.ppm
```

### En Windows (Nativo - genera `religion_sim.exe`)

#### Opción A: Con CMake (Visual Studio / CLion / MinGW)
En PowerShell o CMD en la carpeta del proyecto:
```powershell
cmake -B build
cmake --build build --config Release
.\build\Release\religion_sim.exe
```

#### Opción B: Con Make (MinGW-w64 en Git Bash o MSYS2)
```bash
make all
./bin/religion_sim.exe
```

### En Windows (con WSL2 / WSLg)
Si tienes WSL2 instalado:
```bash
sudo apt update && sudo apt install -y build-essential libx11-dev libgl1-mesa-dev
make all
./bin/religion_sim
```

Opciones CLI: `--headless`, `--ticks N`, `--export archivo.ppm`, `--help`.

---

## Estructura del código

```
include/   Tipos, eras, cámara, religiones, celdas, mapa, difusión,
           aglutinación, aldeanos, humanos, timeline, eventos, renderer, UI, saves
src/       Implementaciones + main.cpp
assets/    Sprites, tilesets, menú, cursor
tests/     test_simulation.cpp
saves/     Partidas locales (ignoradas por git)
```

Motor de render: rasterizador por software + presentador X11/OpenGL. Aldeanos y humanos tienen hojas de sprites, IA por era y órdenes manuales.

---

## Licencia y estado

Proyecto en desarrollo activo. Repositorio: [ivanbordon99/religion_simulator_isometric](https://github.com/ivanbordon99/religion_simulator_isometric).
