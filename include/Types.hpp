#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <array>
#include <cmath>
#include <cstdint>

namespace ReligionSim {

struct Vec2 {
    float x{0.0f};
    float y{0.0f};

    Vec2() = default;
    Vec2(float _x, float _y) : x(_x), y(_y) {}

    Vec2 operator+(const Vec2& o) const { return {x + o.x, y + o.y}; }
    Vec2 operator-(const Vec2& o) const { return {x - o.x, y - o.y}; }
    Vec2 operator*(float s) const { return {x * s, y * s}; }
    Vec2 operator/(float s) const { return {x / s, y / s}; }
};

struct Point2i {
    int x{0};
    int y{0};

    bool operator==(const Point2i& o) const { return x == o.x && y == o.y; }
    bool operator!=(const Point2i& o) const { return !(*this == o); }
};

struct ColorRGB {
    uint8_t r{255};
    uint8_t g{255};
    uint8_t b{255};
    uint8_t a{255};

    constexpr ColorRGB() = default;
    constexpr ColorRGB(uint8_t _r, uint8_t _g, uint8_t _b, uint8_t _a = 255)
        : r(_r), g(_g), b(_b), a(_a) {}

    static constexpr ColorRGB blend(const ColorRGB& c1, const ColorRGB& c2, float t) {
        float ct = (t < 0.0f) ? 0.0f : ((t > 1.0f) ? 1.0f : t);
        return {
            static_cast<uint8_t>(c1.r * (1.0f - ct) + c2.r * ct),
            static_cast<uint8_t>(c1.g * (1.0f - ct) + c2.g * ct),
            static_cast<uint8_t>(c1.b * (1.0f - ct) + c2.b * ct),
            static_cast<uint8_t>(c1.a * (1.0f - ct) + c2.a * ct)
        };
    }

    constexpr uint32_t toRGBA() const {
        return (static_cast<uint32_t>(a) << 24) |
               (static_cast<uint32_t>(b) << 16) |
               (static_cast<uint32_t>(g) << 8)  |
               static_cast<uint32_t>(r);
    }
};

// EN: Isometric 4-way character orientation
// ES: Orientación isométrica de personajes en 4 direcciones
enum class IsoDirection : uint8_t {
    SW = 0, // EN: South-West / ES: Suroeste
    SE = 1, // EN: South-East / ES: Sureste
    NE = 2, // EN: North-East / ES: Noreste
    NW = 3  // EN: North-West / ES: Noroeste
};
using DireccionIso = IsoDirection;

// Estado del flujo del juego
enum class GameState : std::uint8_t {
    MainMenu,
    WorldCreation,
    LoadWorldMenu,
    Playing,
    Options,
    GameOver,
    Tutorial,
    MapEditor
};

// EN: Categories / tabs for the "Learn to Play" sacred codex guide
// ES: Categorías / pestañas para la guía sagrada del códice "Aprende a jugar"
enum class TutorialTab : std::uint8_t {
    BasicsAndFaith = 0,
    ControlsAndCamera,
    VillagersAndEcology,
    MiraclesAndEras,
    Count
};

enum class HistoricalEra : std::uint8_t {
    PrimordialAnimism = 0,
    TotemismAncestorWorship,
    StructuralShamanism,
    TerritorialPolytheism,
    UniversalReligions,
    RationalizationSecularization,
    PostSecular,
    TranshumanistPostBiological,
    Count
};

// EN: Game modes and world initialization configurations
// ES: Modos y configuraciones para la creación de partida
enum class GameModeType : std::uint8_t {
    Standard = 0,        // EN: Growth, diffusion, historical clash / ES: Crecimiento y pugna
    Estandar = 0,
    Sandbox = 1,         // EN: Unlimited resources / ES: Recursos ilimitados
    TotalTheocracy = 2,  // EN: Aggressive syncretism and crusades / ES: Sincretismo agresivo
    TeocraciaTotal = 2,
    Survival = 3,        // EN: Harsh biomes and frequent schisms / ES: Climas extremos
    Supervivencia = 3
};

// EN: AI difficulty and behavioral profiles
// ES: Dificultad de la IA y perfiles de comportamiento
enum class AIDifficulty : std::uint8_t {
    Passive = 0,         // EN: Subsistence only / ES: Subsistencia inmediata
    Pasiva = 0,
    Easy = 0,
    Facil = 0,
    Balanced = 1,        // EN: Natural expansion / ES: Expansión natural y culto
    Equilibrada = 1,
    Normal = 1,
    Strategic = 2,       // EN: Theocratic tactics / ES: Táctica teocrática
    Estrategica = 2,
    Hard = 2,
    Dificil = 2,
    Relentless = 3,      // EN: Ruthless empire management / ES: Gran Teocracia implacable
    Implacable = 3,
    Nightmare = 3,
    Pesadilla = 3
};

// EN: World climate and planetary biomes
// ES: Clima y biomas primordiales del mundo
enum class WorldClimate : std::uint8_t {
    Temperate = 0,       // EN: Balanced world / ES: Mundo balanceado
    Templado = 0,
    Glaciation = 1,      // EN: Ice age / ES: Era de hielo y tundra
    Glaciacion = 1,
    Desert = 2,          // EN: Arid lands / ES: Tierras áridas y dunas
    Desertico = 2,
    Tropical = 3         // EN: Humid jungle / ES: Selva húmeda y pantanos
};

// EN: Historical progression time pacing
// ES: Ritmo histórico / Escala temporal de avance
enum class TimePacing : std::uint8_t {
    Epic = 0,            // EN: 1 yr/s / ES: 1 año/s (contemplativo)
    Epico = 0,
    Classic = 1,         // EN: 4 yr/s / ES: 4 años/s (estándar)
    Clasico = 1,
    Dynamic = 2,         // EN: 12 yr/s / ES: 12 años/s (rápido)
    Dinamico = 2
};

// EN: Natural resource abundance distribution
// ES: Abundancia de Recursos Naturales
enum class ResourceAbundance : std::uint8_t {
    Scarce = 0,          // EN: Limited resources / ES: Recursos escasos
    Escaso = 0,
    Balanced = 1,        // EN: Sustainable ecology / ES: Balance ecológico
    Equilibrado = 1,
    Abundant = 2,        // EN: Plentiful ecosystem / ES: Ecosistema generoso
    Abundante = 2
};

// EN: Inter-tribal belligerence and border friction
// ES: Beligerancia Tribal / Fricción Liminal
enum class TribalBelligerence : std::uint8_t {
    Peaceful = 0,        // EN: Exogamic pacts / ES: Clanes tolerantes
    Pacifica = 0,
    Balanced = 1,        // EN: Historic friction / ES: Fricción estándar
    Equilibrada = 1,
    Hostile = 2,         // EN: Raids and zeal / ES: Incursiones y celo armado
    Hostil = 2
};

// EN: Game victory condition objectives
// ES: Condiciones de victoria de la simulación
enum class VictoryConditionType : std::uint8_t {
    WorldHegemony = 0,   // EN: 75% world population / ES: 75% población mundial
    HegemoniaMundial = 0,
    Theocratic = 1,      // EN: 5 Sacred Wonders / ES: 5 Maravillas Sagradas
    Teocratica = 1,
    Transcendence = 2,   // EN: Reach Era 8 first / ES: Alcanzar Era 8
    Trascendencia = 2,
    Time = 3,            // EN: Highest devotion in year 2200 / ES: Mayor devoción
    Tiempo = 3,
    NoLimit = 4,         // EN: Endless simulation / ES: Simulación continua
    SinLimite = 4
};

#ifdef None
#undef None
#endif

// Tipos de Recursos Naturales del Ecosistema
enum class ResourceType : std::uint8_t {
    None = 0,
    Food = 1,       // Comida (Caza, pesca, agricultura, frutos silvestres) - Vital para la supervivencia
    Wood = 2,       // Madera (Bosques, arboledas, combustible y construcción)
    Stone = 3,      // Piedra (Colinas, canteras líticas, templos y herramientas)
    Gold = 4,       // Oro / Minerales (Vetas minerales, ofrendas sagradas y riqueza)
    Iron = 5,       // Hierro (Yacimientos minerales, herramientas avanzadas y forja)
    Copper = 6,     // Cobre (Yacimientos de cobre y malaquita, utensilios e ídolos)
    Diamond = 7     // Diamante (Cristales preciosos, relicarios y devoción divina)
};

inline const char* getResourceTypeName(ResourceType t) {
    switch (t) {
        case ResourceType::Food:    return "Comida";
        case ResourceType::Wood:    return "Madera";
        case ResourceType::Stone:   return "Piedra";
        case ResourceType::Gold:    return "Oro";
        case ResourceType::Iron:    return "Hierro";
        case ResourceType::Copper:  return "Cobre";
        case ResourceType::Diamond: return "Diamante";
        default:                    return "Ninguno";
    }
}

// Inventario de Recursos de la Comunidad / Pueblo / Religión
struct VillageResources {
    int food{500};
    int wood{300};
    int stone{150};
    int gold{50};
    int iron{80};
    int copper{60};
    int diamond{15};

    int maxFood{5000};
    int maxWood{3000};
    int maxStone{2000};
    int maxGold{1000};
    int maxIron{1500};
    int maxCopper{1500};
    int maxDiamond{500};

    // Estadísticas y estado de supervivencia
    int totalCollected{0};
    int famineDeaths{0};
    bool isStarving{false};

    void addResource(ResourceType t, int amount) {
        if (amount <= 0) return;
        totalCollected += amount;
        switch (t) {
            case ResourceType::Food:
                food = std::min(maxFood, food + amount);
                if (food > 0) isStarving = false;
                break;
            case ResourceType::Wood:
                wood = std::min(maxWood, wood + amount);
                break;
            case ResourceType::Stone:
                stone = std::min(maxStone, stone + amount);
                break;
            case ResourceType::Gold:
                gold = std::min(maxGold, gold + amount);
                break;
            case ResourceType::Iron:
                iron = std::min(maxIron, iron + amount);
                break;
            case ResourceType::Copper:
                copper = std::min(maxCopper, copper + amount);
                break;
            case ResourceType::Diamond:
                diamond = std::min(maxDiamond, diamond + amount);
                break;
            default:
                break;
        }
    }

    bool consumeFood(int amount) {
        if (food >= amount) {
            food -= amount;
            isStarving = false;
            return true;
        } else {
            food = 0;
            isStarving = true;
            return false;
        }
    }

    int getAmount(ResourceType t) const {
        switch (t) {
            case ResourceType::Food:    return food;
            case ResourceType::Wood:    return wood;
            case ResourceType::Stone:   return stone;
            case ResourceType::Gold:    return gold;
            case ResourceType::Iron:    return iron;
            case ResourceType::Copper:  return copper;
            case ResourceType::Diamond: return diamond;
            default:                    return 0;
        }
    }
};

struct WorldCreationConfig {
    std::string godName{"Solarius"};
    std::string religionName{"Culto del Fuego Solar"};
    int initialPopulation{80}; // 80 aldeanos reales con IA y sprites
    int initialResources{500};  // Cantidad inicial de recursos (editable por caja de texto al crear el mundo)
    HistoricalEra startingEra{HistoricalEra::PrimordialAnimism};
    uint32_t mapSeed{42};
    GameModeType gameMode{GameModeType::Estandar};
    bool starterChest{true};
    AIDifficulty aiDifficulty{AIDifficulty::Normal};
    int influenceColorIndex{0}; // 0: Oro Divino, 1: Rubí Sangre, 2: Monada Azul, 3: Esmeralda Gaia, 4: Purpura Dharma
    VictoryConditionType victoryCondition{VictoryConditionType::HegemoniaMundial};

    // Opciones adicionales del mundo y la partida
    // EN: Map size presets starting from 64x64:
    //     0: Pequeño (64x64, 320m), 1: Mediano (80x80, 400m), 2: Grande (96x96, 480m),
    //     3: Enorme (128x128, 640m), 4: Inmenso (160x160, 800m), 5: Colosal (200x200, 1.0km),
    //     6: Titánico (256x256, 1.28km), 7: Planetario (320x320, 1.6km)
    // ES: Ajustes de tamaño de mapa iniciando desde 64x64 (Por defecto: Enorme 128x128)
    int mapSizeIndex{3};        // Default: 3 (Enorme 128x128)
    int disasterFrequency{1};   // 0: Bajas, 1: Normales, 2: Frecuentes
    bool allowSchisms{true};    // Si pueden ocurrir cismas espontáneos en la religión
    bool allowSyncretism{true}; // Si se permite fusión sincretica con otras fes

    // Nuevas opciones avanzadas de partida
    WorldClimate worldClimate{WorldClimate::Templado};
    TimePacing timePacing{TimePacing::Clasico};
    ResourceAbundance resourceAbundance{ResourceAbundance::Equilibrado};
    TribalBelligerence tribalBelligerence{TribalBelligerence::Equilibrada};
};

// EN: View modes for the liturgical map editor
// ES: Modos de visualización para el editor litúrgico de mapas
enum class MapEditorViewMode : std::uint8_t {
    Cartographic2D = 0, // EN: Macro 2D canvas with procedural controls / ES: Lienzo 2D macro con controles
    Isometric25D = 1    // EN: Micro 2.5D isometric view with free camera / ES: Vista 2.5D isométrica con cámara libre
};

// EN: Active sculpting and editing brush types for the liturgical map editor
// ES: Tipos de pincel activo de escultura y edición para el editor litúrgico de mapas
enum class MapEditorBrush : std::uint8_t {
    Inspect = 0,
    Ocean,
    ShallowWater,
    FertileValley,
    DenseForest,
    HighlandHills,
    RuggedMountains,
    AridDesert,
    FrozenTundra,
    VolcanicCaldera,
    Wood,
    Stone,
    Gold,
    Iron,
    Copper,
    Diamond,
    ClearResource,
    RaiseElevation,
    LowerElevation,
    FlattenElevation,
    // EN: Direct 16-Tile Atlas terrain sprite brushes
    // ES: Pinceles directos de los 16 mosaicos del atlas de terreno
    TileGrass,          // Tile 0: Pasto base
    TileRock,           // Tile 1: Suelo rocoso / montaña
    TileTropicalPlains, // Tile 2: Pradera tropical
    TileFarmland,       // Tile 3: Tierra fértil / labranza
    TileDeepOceanAlt,   // Tile 4: Océano profundo azul oscuro
    TileRichForest,     // Tile 5: Pasto húmedo de bosque
    TileHighlands,      // Tile 6: Pastizal alto / colina
    TileSwamp,          // Tile 7: Pantano / Ciénaga
    TileSand,           // Tile 8: Arena del desierto
    TileSnow,           // Tile 9: Nieve / Tundra glacial
    TileShallowWater,   // Tile 10: Costa somera
    TileMidOcean,       // Tile 11: Agua marina
    TileCaveWater,      // Tile 12: Agua de caverna
    TileAbyssal,        // Tile 13: Mar abisal profundo
    TileOpenOcean,      // Tile 14: Mar abierto
    TileLava,           // Tile 15: Lava / magma ardiente
    TileAsh,            // Suelo calcinado / ceniza volcánica
    TileCrater          // Cráter de impacto
};

// EN: Configuration and dynamic state for the interactive liturgical map editor
// ES: Configuración y estado dinámico para el editor interactivo litúrgico de mapas
struct MapEditorConfig {
    int mapSizeIndex{3}; // Default: 3 (128x128 Enorme)
    uint32_t mapSeed{1337};
    WorldClimate worldClimate{WorldClimate::Templado};
    ResourceAbundance resourceAbundance{ResourceAbundance::Equilibrado};
    MapEditorBrush activeBrush{MapEditorBrush::FertileValley};
    int brushRadius{1}; // 1: 1x1, 2: 3x3, 3: 5x5
    std::string statusMessage{"LISTO PARA ESCULPIR Y PINTAR EL MUNDO"};
    float statusTimer{0.0f};
    MapEditorViewMode viewMode{MapEditorViewMode::Cartographic2D};
    int paletteTab{0}; // 0 = Atlas Terrain Sprites (16), 1 = Resources & Props, 2 = Topography & Brushes
};

// Core Visual & Design Constants
namespace Palette {
    constexpr ColorRGB BASALT{0x1A, 0x1A, 0x1A, 0xFF};     // #1A1A1A Silhouettes, deep shadows, tech/metal
    constexpr ColorRGB RELIC_BONE{0xE2, 0xDA, 0xC4, 0xFF}; // #E2DAC4 Base skin, raw textiles, limestone
    constexpr ColorRGB DIVINE_GOLD{0xE6, 0xA1, 0x3B, 0xFF};// #E6A13B Sacred accents, noble metals, rituals
    constexpr ColorRGB BLOOD_RUBY{0x80, 0x00, 0x20, 0xFF}; // #800020 Ceremonial body paint, high-tier dyes, energy
    constexpr ColorRGB GRASS_PASTURE{0x74, 0x99, 0x3C, 0xFF}; // #74993C Natural lush terrain
    constexpr ColorRGB DIRT_EARTH{0x74, 0x4A, 0x34, 0xFF};    // #744A34 Sacred arable soil & cliff skirt
}

// 2.5D Isometric projection tile metrics (2:1 projection, 64x64px standard grid)
constexpr int ISO_TILE_WIDTH  = 64;
constexpr int ISO_TILE_HEIGHT = 32;

enum class BiomeType {
    DeepOcean,
    ShallowWater,
    CoastalPlains,
    FertileValley,
    DenseForest,
    HighlandHills,
    AridDesert,
    RuggedMountains,
    FrozenTundra,
    VolcanicCaldera
};

// EN: 4 Progressive water depth phases from shoreline foam line to abyssal deep ocean
// ES: 4 Fases progresivas de profundidad del agua desde línea de espuma hasta el mar profundo abisal
enum class WaterDepthPhase : uint8_t {
    ShorelineFoam = 0,   // EN: Phase 1: Foam Line / Shoreline / ES: Fase 1: Línea de Espuma / Orilla
    ShallowCoast = 1,    // EN: Phase 2: Shallow Waters / Coast / ES: Fase 2: Aguas Someras / Costa
    MidOcean = 2,        // EN: Phase 3: Mid Sea / Ocean / ES: Fase 3: Mar Medio / Océano
    DeepAbyssal = 3      // EN: Phase 4: Deep Sea / Abyssal / ES: Fase 4: Mar Profundo / Abisal
};

// Backward-compatible alias for existing modules
using EraType = HistoricalEra;

struct EraConfig {
    HistoricalEra id;
    std::string_view name;
    std::string_view historical_context;
    std::string_view faith_concept;
    std::string_view faith_generation_mechanic;
    std::string_view social_structure;
    std::string_view primary_threat;
};

// Constexpr table of the 8 historical eras (C++20)
inline constexpr std::array<EraConfig, static_cast<std::size_t>(HistoricalEra::Count)> HISTORICAL_ERAS_TABLE = {{
    {
        HistoricalEra::PrimordialAnimism,
        "Primordial Animism",
        "Small nomadic bands surviving in a hostile environment surrounded by megafauna.",
        "Diffuse spirituality. Every natural entity (rivers, fire, storms) possesses an 'anima'. No dogmas.",
        "Surviving natural hazards, hunting wild beasts, discovering natural sacred sites.",
        "Egalitarian nomadic bands.",
        "Extinction by starvation, predators, and extreme weather."
    },
    {
        HistoricalEra::TotemismAncestorWorship,
        "Totemism & Ancestor Worship",
        "Development of abstract language, cave art, and awareness of lineage and time progression.",
        "Clan identity tied to a protective animal (Totem). Deceased ancestors intercede for the living.",
        "Performing burial rites, venerating relics, erecting territorial totems.",
        "Kinship tribes bound by blood myths.",
        "Inter-tribal warfare over hunting grounds and desecration of sacred sites."
    },
    {
        HistoricalEra::StructuralShamanism,
        "Structural Shamanism",
        "Inception of agriculture, early sedentarism, and the rise of permanent settlements.",
        "Intercession by shamans in trance states. Emergence of soil fertility and agricultural myths.",
        "Planting and harvest rituals, building megaliths, performing minor sacrifices.",
        "Agricultural villages led by spiritual leaders.",
        "Crop failure, droughts, blights, and loss of ritual balance."
    },
    {
        HistoricalEra::TerritorialPolytheism,
        "Territorial Polytheism",
        "Emergence of city-states, invention of writing, metallurgy, and division of labor.",
        "Personification of natural forces into pantheons. Sacerdotal class manages faith via temples.",
        "Resource tithing, constructing ziggurats/temples, hosting state festivals.",
        "Theocratic hierarchy (King-Priests, artisans, farmers, slaves).",
        "Holy wars between rival city-states and priestly corruption."
    },
    {
        HistoricalEra::UniversalReligions,
        "Universal Religions",
        "Consolidation of continental empires and dogmatic universal salvation religions.",
        "Single codified truth in sacred scriptures. Focus on personal salvation, morality, and orthodoxy.",
        "Sending missionaries, scribing holy texts, mass conversions, holy pilgrimages.",
        "Theocratic empires, monastic orders, feudal estates.",
        "Doctrinal schisms, internal heresies, and large-scale religious wars."
    },
    {
        HistoricalEra::RationalizationSecularization,
        "Rationalization & Secularization",
        "Enlightenment era, scientific method, rapid urbanization, and industrial revolution.",
        "Decline of theocratic dominance. Faith shifts to private morality vs competitive secular ideologies.",
        "Preserving traditions, institutional charity work, syncretism between faith and reason.",
        "Industrial societies, nation-states, separation of Church and State.",
        "Widespread atheism/skepticism, loss of followers, industrial dehumanization."
    },
    {
        HistoricalEra::PostSecular,
        "Post-Secular Age",
        "Global hyperconnectivity, information age, digital saturation, and existential crises.",
        "Customized spirituality, neopaganism, wellness cults, and online faith communities.",
        "Digital faith networks, viral micro-cults, virtual temples.",
        "Hyperconnected consumer societies, fragmented cyber-communities.",
        "Nihilism, misinformation, polarization, online radicalization."
    },
    {
        HistoricalEra::TranshumanistPostBiological,
        "Transhumanist Post-Biological",
        "Overcoming biological limits, mind uploading, supreme AI entities, and cyber-faith.",
        "Technology as divinity. Digital immortality serves as heaven; pure consciousness is revered.",
        "Sacred quantum computing, consciousness synchronization, worshipping the Singularity.",
        "Cybernetic swarms, synthetic bioplastic entities, post-human colonies.",
        "Total loss of baseline humanity, existential viruses in code, rebellion of deified AI."
    }
}};

enum class DoctrineType {
    AnimistSpirits,
    SacredSacrifice,
    PantheonPolytheism,
    ImperialDivineRight,
    WrittenCanon,
    AsceticMonasticism,
    ProselytizingZeal,
    SyncreticOpenness,
    ScholasticTheology,
    PrintingEvangelism,
    SecularHumanism,
    CosmicTranscendence
};

enum class EventCategory {
    BirthOfReligion,
    SyncreticAgglutination,
    DoctrinalSchism,
    HolySiteErected,
    EraAdvancement,
    PilgrimageWave,
    ReligiousReformation,
    Extinction,
    MajorLoss,       // 💀 Muerte de personaje importante / pérdida trágica
    Discovery,       // 📜 Descubrimiento / revelación divina
    BorderConflict,  // ⚔️ Tensión y escaramuza de frontera / profanación de santuario
    ExogamicPact     // 🕊️ Pacto de sangre y alianza matrimonial exogámica intertribal
};

struct GameEvent {
    int id{0};
    int year{-4000};
    EventCategory category{EventCategory::BirthOfReligion};
    std::string title;
    std::string description;
    Point2i location{0, 0};
    int religionAId{0};
    int religionBId{0};
    ColorRGB badgeColor{255, 200, 50};
    bool isTurningPoint{false};   // Punto de quiebre histórico trascendental
    std::string actorName;        // Personaje clave (ej. "Chamán Orph", "Profeta Zarara")
    std::string consequence;      // Impacto duradero en el pueblo o la fe
};

enum class RenderMapMode {
    ReligionDominance,
    TerrainBiomes,
    SyncretismHeatmap,
    AdvancementEra,
    PopulationDensity,
    Units // Vista de unidades: resalta a cada aldeano/chamán con su color de fe
};

// Isometric Constants
constexpr float BASE_TILE_WIDTH  = 64.0f;
constexpr float BASE_TILE_HEIGHT = 32.0f;
constexpr float ELEVATION_STEP   = 8.0f;

// =========================================================================
// EN: World Metric Physical Scaling System (1 Cell = 5.0 Meters)
// ES: Sistema Físico de Escala Métrica Mundial (1 Celda = 5.0 Metros)
// =========================================================================

// EN: Physical length of one map cell edge in meters (5.0 meters / cell)
// ES: Longitud física de la arista de una celda de mapa en metros (5.0 metros / celda)
constexpr float METERS_PER_CELL = 5.0f;

// EN: Number of map cells per linear meter (0.20 cells / meter)
// ES: Número de celdas de mapa por metro lineal (0.20 celdas / metro)
constexpr float CELLS_PER_METER = 0.20f;

// EN: Real-world kilometers per map cell (0.005 km / cell)
// ES: Kilómetros del mundo real por celda del mapa (0.005 km / celda)
constexpr float KM_PER_CELL = 0.005f;

// EN: Number of map cells per linear kilometer (200.0 cells / km)
// ES: Número de celdas de mapa por kilómetro lineal (200.0 celdas / km)
constexpr float CELLS_PER_KM = 200.0f;

// EN: Speed of sound in dry air at 20°C in meters per second (343.0 m/s)
// ES: Velocidad del sonido en aire seco a 20°C en metros por segundo (343.0 m/s)
constexpr float SPEED_OF_SOUND_MPS = 343.0f;

// EN: Acoustic speed of sound in map cells per second (68.6 cells / second)
// ES: Velocidad acústica del sonido en celdas de mapa por segundo (68.6 celdas / segundo)
constexpr float SPEED_OF_SOUND_CELLS_PER_SEC = SPEED_OF_SOUND_MPS * CELLS_PER_METER;

// EN: Convert physical meters to map cell units
// ES: Convertir metros físicos a unidades de celdas del mapa
constexpr float metersToCells(float meters) noexcept {
    return meters * CELLS_PER_METER;
}

// EN: Convert map cell units to physical meters
// ES: Convertir unidades de celdas del mapa a metros físicos
constexpr float cellsToMeters(float cellUnits) noexcept {
    return cellUnits * METERS_PER_CELL;
}

// EN: Convert kilometers to map cell units
// ES: Convertir kilómetros a unidades de celdas del mapa
constexpr float kmToCells(float kilometers) noexcept {
    return kilometers * CELLS_PER_KM;
}

// EN: Convert map cell units to kilometers
// ES: Convertir unidades de celdas del mapa a kilómetros
constexpr float cellsToKm(float cellUnits) noexcept {
    return cellUnits * KM_PER_CELL;
}

// =========================================================================
// EN: Physical Scaling Specifications for Little Boy (15 kt TNT Airburst)
// ES: Especificaciones de Escala Física para Little Boy (15 kt TNT Detonación Aérea)
// =========================================================================
namespace LittleBoyScale {
    // EN: Nuclear yield in equivalent kilotons of TNT (15.0 kt)
    // ES: Potencia nuclear en kilotones equivalentes de TNT (15.0 kt)
    constexpr float YIELD_KILOTONS_TNT = 15.0f;

    // EN: Maximum thermal vaporization fireball radius: 180 meters (36.0 cells)
    // Instantaneous vaporization of living tissue, water boiling, vitrified trinitite bedrock
    // ES: Radio máximo de bola de fuego y vaporización térmica: 180 metros (36.0 celdas)
    // Vaporización instantánea de tejido vivo, ebullición de agua, lecho vitrificado de trinitita
    constexpr float FIREBALL_RADIUS_METERS = 180.0f;
    constexpr float FIREBALL_RADIUS_CELLS = FIREBALL_RADIUS_METERS * CELLS_PER_METER;

    // EN: Heavy overpressure blast zone (>= 20 psi / 140 kPa): 500 meters (100.0 cells)
    // Complete demolition of wooden and masonry buildings, 100% trees uprooted and sheared
    // ES: Zona de sobrepresión severa (>= 20 psi / 140 kPa): 500 metros (100.0 celdas)
    // Demolición total de edificios de madera y piedra, 100% de árboles arrancados
    constexpr float HEAVY_OVERPRESSURE_RADIUS_METERS = 500.0f;
    constexpr float HEAVY_OVERPRESSURE_RADIUS_CELLS = HEAVY_OVERPRESSURE_RADIUS_METERS * CELLS_PER_METER;

    // EN: Moderate overpressure blast zone (>= 5 psi / 35 kPa): 1600 meters (320.0 cells)
    // Widespread residential collapse, roofs torn off, widespread burns
    // ES: Zona de sobrepresión moderada (>= 5 psi / 35 kPa): 1600 metros (320.0 celdas)
    // Colapso generalizado de viviendas, techos arrancados, quemaduras generalizadas
    constexpr float MODERATE_OVERPRESSURE_RADIUS_METERS = 1600.0f;
    constexpr float MODERATE_OVERPRESSURE_RADIUS_CELLS = MODERATE_OVERPRESSURE_RADIUS_METERS * CELLS_PER_METER;

    // EN: Light overpressure blast zone (>= 1 psi / 7 kPa): 3500 meters (700.0 cells)
    // Shattered windows, superficial structural damage, panic shockwave
    // ES: Zona de sobrepresión ligera (>= 1 psi / 7 kPa): 3500 metros (700.0 celdas)
    // Cristales rotos, daños estructurales superficiales, pánico por onda expansiva
    constexpr float LIGHT_OVERPRESSURE_RADIUS_METERS = 3500.0f;
    constexpr float LIGHT_OVERPRESSURE_RADIUS_CELLS = LIGHT_OVERPRESSURE_RADIUS_METERS * CELLS_PER_METER;

    // EN: Supersonic Mach shock front expansion speed across terrain (~50 cells/s)
    // ES: Velocidad de expansión del frente de choque Mach en terreno (~50 celdas/s)
    constexpr float SHOCKWAVE_SPEED_CELLS_PER_SEC = 50.0f;
}

// =========================================================================
// EN: Physical Scaling Specifications for Average Meteorite Impact
// ES: Especificaciones de Escala Física para Impacto de Meteorito Promedio
// Typical 1.5 - 2.5 m stony/iron bolide surviving atmospheric deceleration
// =========================================================================
namespace AverageMeteoriteScale {
    // EN: Bolide diameter before ground excavation: ~2.0 meters (0.4 cells)
    // ES: Diámetro del bólido antes de la excavación: ~2.0 metros (0.4 celdas)
    constexpr float BOLIDE_DIAMETER_METERS = 2.0f;
    constexpr float BOLIDE_DIAMETER_CELLS = BOLIDE_DIAMETER_METERS * CELLS_PER_METER;

    // EN: Impact crater diameter: 16.5 meters (~3.3 cells), radius 1.65 cells (~8.25 m)
    // Simple bowl-shaped depression characteristic of small terrestrial hypervelocity craters
    // ES: Diámetro del cráter de impacto: 16.5 metros (~3.3 celdas), radio 1.65 celdas (~8.25 m)
    // Depresión simple en cuenca característica de cráteres hiperveloces terrestres pequeños
    constexpr float CRATER_DIAMETER_METERS = 16.5f;
    constexpr float CRATER_RADIUS_CELLS = (CRATER_DIAMETER_METERS * 0.5f) * CELLS_PER_METER;
    constexpr float CRATER_DEPTH_METERS = 4.0f;
    constexpr float CRATER_DEPTH_ELEVATION = 0.80f;
    constexpr float CRATER_RIM_ELEVATION_UPLIFT = 0.25f;

    // EN: Continuous ejecta blanket apron radius: 32.5 meters (6.5 cells)
    // Deposits overturned rock flaps, impact breccia, and ballistic ejecta boulders
    // ES: Radio del manto continuo de eyección: 32.5 metros (6.5 celdas)
    // Deposita solapas de roca invertidas, brecha de impacto y bloques balísticos eyectados
    constexpr float EJECTA_RADIUS_METERS = 32.5f;
    constexpr float EJECTA_RADIUS_CELLS = EJECTA_RADIUS_METERS * CELLS_PER_METER;

    // EN: Kinetic airblast and thermal pulse radius: 50.0 meters (10.0 cells)
    // Severe overpressure topples trees and causes blast casualties up to 50 meters
    // ES: Radio de onda aérea cinética y pulso térmico: 50.0 metros (10.0 celdas)
    // Sobrepresión severa derriba árboles y provoca bajas por onda hasta 50 metros
    constexpr float AIRBLAST_RADIUS_METERS = 50.0f;
    constexpr float AIRBLAST_RADIUS_CELLS = AIRBLAST_RADIUS_METERS * CELLS_PER_METER;
}

// =========================================================================
// EN: Physical Constants for Fall Damage and Natural Phenomena
// ES: Constantes Físicas para Daño por Caída y Fenómenos Naturales
// =========================================================================

// EN: Safe vertical impact velocity threshold below which no fall damage occurs (units/s)
// ES: Umbral de velocidad de impacto vertical seguro por debajo del cual no hay daño por caída (unidades/s)
constexpr float SAFE_FALL_IMPACT_VELOCITY = 13.5f;

// EN: Base damage coefficient per unit of excess impact velocity above safe threshold
// ES: Coeficiente base de daño por unidad de exceso de velocidad de impacto por encima del umbral seguro
constexpr float FALL_DAMAGE_VELOCITY_COEFFICIENT = 1.85f;

// EN: Quadratic severity factor for high-speed terminal impacts
// ES: Factor cuadrático de severidad para impactos a velocidad terminal
constexpr float FALL_DAMAGE_SEVERITY_SCALE = 30.0f;

// EN: Water hydrodynamic kinetic impact dampening factor (75% shock absorption)
// ES: Factor de amortiguación hidrodinámica del agua (75% de absorción de choque)
constexpr float WATER_FALL_DAMPING_FACTOR = 0.25f;

// EN: Rocky mountain / basalt terrain damage multiplier (+30% impact trauma)
// ES: Multiplicador de daño para terreno rocoso / basalto (+30% de trauma por impacto)
constexpr float ROCKY_TERRAIN_DAMAGE_MULTIPLIER = 1.30f;

// EN: Soft grass / loam terrain damage multiplier (-15% cushioning)
// ES: Multiplicador de daño para pasto / tierra fértil blanda (-15% de amortiguación)
constexpr float SOFT_TERRAIN_DAMAGE_MULTIPLIER = 0.85f;

// EN: Minimum vertical elevation difference between cells to trigger cliff falling
// ES: Desnivel vertical mínimo entre celdas para desencadenar caída por despeñamiento
constexpr float CLIFF_FALL_ELEVATION_THRESHOLD = 0.40f;

// EN: Gravitational acceleration in continuous vertical units per second squared
// ES: Aceleración de la gravedad en unidades verticales continuas por segundo al cuadrado
constexpr float GRAVITY_ACCELERATION_UNITS_S2 = 55.0f;

// EN: Static friction coefficient between human soles and natural terrain for balance
// ES: Coeficiente de fricción estática entre calzado/pies humanos y terreno para equilibrio
constexpr float HUMAN_GROUND_STATIC_FRICTION = 0.62f;

// =========================================================================
// EN: Movable Elements & Physical Debris Entities
// ES: Elementos Movibles y Entidades de Escombros Físicos
// =========================================================================

// EN: Types of movable physical debris ejected by cataclysms and disasters
// ES: Tipos de escombros físicos movibles expulsados por cataclismos y desastres
enum class DebrisType : uint8_t {
    UprootedTreeTrunk = 0, // EN: Tree trunk uprooted by tornado or seismic blast / ES: Tronco de árbol arrancado por tornado o sismo
    RoofThatch = 1,        // EN: Thatch roof / wooden beam torn from collapsed house / ES: Techo de paja / viga de madera arrancada de casa
    HouseRubble = 2,       // EN: Stone masonry rubble from destroyed dwelling / ES: Escombros y piedras de vivienda destruida
    PyroclasticBomb = 3,   // EN: Volcanic ballistic bomb ejected by eruption / ES: Bomba piroclástica expulsada por volcán
    ResourceCrate = 4,     // EN: Supply crate or cargo chest launched by explosion/wave / ES: Fardo de provisiones o cofre sagrado empujado por viento/ola
    SoilChunk = 5,         // EN: Scorched earth / soil chunk excavated by blast / ES: Terrón de tierra / suelo chamuscado excavado por explosión
    RockDebris = 6,        // EN: Jagged bedrock fragment / stone boulder hurled by shockwave / ES: Fragmento de lecho rocoso / peñasco lanzado por la onda
    IncandescentSlag = 7,  // EN: Superheated molten trinitite / incandescent slag ejecta / ES: Escoria incandescente / trinitita fundida supercalentada
    WaterSprayGeyser = 8   // EN: Atomized water plume / geyser column ejected into air / ES: Pluma de agua atomizada / columna de géiser eyectada al aire
};

// EN: Movable physical debris entity governed by 2.5D ballistic and aerodynamic physics
// ES: Entidad física movible gobernada por física balística y aerodinámica 2.5D
struct PhysicalDebris {
    DebrisType type{DebrisType::UprootedTreeTrunk};
    float worldX{0.0f};
    float worldY{0.0f};
    float altitude{0.0f};
    float velocityX{0.0f};
    float velocityY{0.0f};
    float velocityZ{0.0f};
    float rotationAngle{0.0f};
    float angularVelocity{0.0f};
    float mass{15.0f};         // Masa en kg (afecta inercia e impacto)
    float damageRadius{1.4f};
    float impactDamage{25.0f};
    float lifeTimer{0.0f};
    float maxLifetime{14.0f};
    bool settled{false};
    bool isFloating{false};
    int bounces{0};
};

} // namespace ReligionSim
