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
    Wildfire = 10   // Incendio Forestal (Fuego sagrado devorador, propagación ígnea celular, calcinación y pánico)
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
                         EventLine& eventLine, int playerRelId);

    // Actualización de efectos visuales y sermones temporales
    void update(float dt, WorldMap& map, GestorHumanos& humanos);

    // Efectos activos y sermones para renderizado
    const std::vector<ActiveDivineFX>& getActiveFX() const { return m_activeFX; }
    const std::vector<PreacherSpeech>& getActiveSpeeches() const { return m_activeSpeeches; }

    // Búsqueda o consagración del intermediario (Chamán o Profeta según la era)
    static int ensureIntermediary(GestorHumanos& humanos, ReligionRegistry& registry,
                                  int playerRelId, std::string& outTitle, std::string& outName);
    static std::string getTitleForEra(HistoricalEra era);

    // Limpiar efectos
    void clearAllFX();

private:
    std::vector<ActiveDivineFX> m_activeFX;
    std::vector<PreacherSpeech> m_activeSpeeches;
};

} // namespace ReligionSim
