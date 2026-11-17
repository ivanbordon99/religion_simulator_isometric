#pragma once

#include "Types.hpp"
#include "EventLine.hpp"
#include <string>
#include <vector>

namespace ReligionSim {

class WorldMap;
class ReligionRegistry;
class GestorHumanos;
class IsometricCamera;
class BuildingSystem;
class ReplicationEngine;

enum class DivinePowerType : uint8_t {
    Inactive = 0,
    Lightning = 1,  // Rayo Divino
    Plague = 2,     // Plaga Sagrada
    Miracle = 3,    // Milagro Celestial
    Congregate = 4, // Llamado de Comunión
    Tornado = 5,    // Tornado Devastador (Ciclón móvil, succión vorticial y aldeanos volando)
    Earthquake = 6, // Terremoto Sísmico (Sacudida tectónica, fisuras y pánico)
    Pandemic = 7,   // Pandemia Viral (Vector biológico de contagio humano a humano)
    Meteor = 8,     // Meteorito Cataclísmico (Bólido celeste en llamas, onda expansiva supersónica y cráter sagrado)
    Volcano = 9,    // Erupción Volcánica (Aparición de gran volcán, eyección de magma, ríos de lava y expansión de tierra)
    Wildfire = 10,  // Incendio Forestal (Fuego sagrado devorador, propagación ígnea celular, calcinación y pánico)
    Rain = 11,      // Lluvia Bendita (Precipitación bienhechora, apaga incendios y fertiliza la tierra)
    Deluge = 12,    // Gran Diluvio (Cataclismo bíblico de inundación masiva, crecida de aguas y física de escorrentía)
    AtomicBomb = 13 // Explosión Atómica [FUCKINGBOMB] (Cataclismo nuclear, hongo atómico, choque supersónico y cráter vitrificado)
};

struct ActiveDivineFX {
    DivinePowerType type{DivinePowerType::Inactive};
    int gridX{0};
    int gridY{0};
    float worldX{0.0f};      // Coordenadas continuas para desastres móviles
    float worldY{0.0f};
    float velocityX{0.0f};   // Deriva física del tornado
    float velocityY{0.0f};
    float duration{1.5f};
    float timer{0.0f};
    float intensity{1.0f};
    float radius{3.0f};
    int converts{0};
    int inspired{0};
    int faithGenerated{0};
    float seed{0.0f};
    bool impactTriggered{false};
    float secondaryTimer{0.0f}; // Temporizador auxiliar para emisión de proyectiles/pulsos
};

// EN: Volcanic ballistic ejecta projectile (pyroclastic bomb)
// ES: Proyectil piroclástico balístico volcánico (bomba volcánica)
struct PyroclasticBomb {
    float worldX{0.0f};
    float worldY{0.0f};
    float altitude{0.0f};
    float velocityX{0.0f};
    float velocityY{0.0f};
    float velocityZ{0.0f};
    float blastRadius{2.0f};
    bool exploded{false};
};

struct PreacherSpeech {
    int humanId{-1};
    std::string name;
    std::string title; // "CHAMÁN" o "PROFETA"
    std::string speechText;
    float worldX{0.0f};
    float worldY{0.0f};
    float timer{0.0f};
    float maxDuration{5.0f};
};

struct CastResult {
    bool success{false};
    std::string message;
    int converts{0};
    int inspired{0};
    int faithGenerated{0};
    int casualties{0};
    int intermediaryId{-1};
    std::string intermediaryTitle;
    std::string intermediaryName;
    std::string sermon;
};

class DivinePowersSystem {
public:
    DivinePowersSystem();

    // Consultas de poderes
    int getPowerCost(DivinePowerType type) const;
    std::string getPowerName(DivinePowerType type) const;
    std::string getPowerDescription(DivinePowerType type) const;
    int getPowerRadius(DivinePowerType type) const;
    ColorRGB getPowerColor(DivinePowerType type) const;

    // Ejecución del acto divino del usuario
    CastResult castPower(DivinePowerType type, int targetGx, int targetGy,
                         WorldMap& map, ReligionRegistry& registry,
                         GestorHumanos& humanos, int currentYear,
                         EventLine& eventLine, int playerRelId,
                         BuildingSystem* buildingSystem = nullptr,
                         ReplicationEngine* replicationEngine = nullptr);

    // Actualización de efectos visuales y sermones temporales
    void update(float dt, WorldMap& map, GestorHumanos& humanos,
                BuildingSystem* buildingSystem = nullptr,
                ReplicationEngine* replicationEngine = nullptr);

    // EN: Spawn movable physical debris (uprooted trees, roof thatch, rubble, crates)
    // ES: Generar escombros físicos movibles (árboles arrancados, techos, ruinas, fardos)
    void spawnDebris(DebrisType type, float x, float y, float z,
                     float vx, float vy, float vz, float mass = 15.0f,
                     float impactDmg = 25.0f, float radius = 1.4f);

    // Efectos activos, proyectiles y escombros físicos para renderizado
    const std::vector<ActiveDivineFX>& getActiveFX() const { return m_activeFX; }
    const std::vector<PreacherSpeech>& getActiveSpeeches() const { return m_activeSpeeches; }
    const std::vector<PyroclasticBomb>& getActiveBombs() const { return m_activeBombs; }
    const std::vector<PhysicalDebris>& getActiveDebris() const { return m_activeDebris; }

    // Búsqueda o consagración del intermediario (Chamán o Profeta según la era)
    static int ensureIntermediary(GestorHumanos& humanos, ReligionRegistry& registry,
                                  int playerRelId, std::string& outTitle, std::string& outName);
    static std::string getTitleForEra(HistoricalEra era);

    // Limpiar efectos
    void clearAllFX();

private:
    std::vector<ActiveDivineFX> m_activeFX;
    std::vector<PreacherSpeech> m_activeSpeeches;
    std::vector<PyroclasticBomb> m_activeBombs;
    std::vector<PhysicalDebris> m_activeDebris;
};

} // namespace ReligionSim
