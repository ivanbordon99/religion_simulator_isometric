#pragma once

#include "Types.hpp"
#include "Cell.hpp"
#include "Humano.hpp"
#include "EventLine.hpp"
#include "SoundSystem.hpp"
#include "IsometricCamera.hpp"
#include "ReligiousMoralitySystem.hpp"
#include <vector>
#include <string>
#include <algorithm>
#include <cmath>

namespace ReligionSim {

class WorldMap;

enum class HouseType : uint8_t {
    ThatchedHut = 0,         // Choza circular de paja y armazón de ramas
    BoneDwelling = 1,        // Cabaña de costillas de mamut y pieles (tundra/montaña)
    LargeRoundhouse = 2,     // Gran casa comunal redonda (familias extensas/ancianos)
    ChieftainLonghouse = 3,  // Residencia del líder o chamán
    PrimitiveShelter = 4     // Refugio temporal de ramas
};

enum class BuildingState : uint8_t {
    SitePlanned = 0,         // Parcela delimitada
    GatheringMaterials = 1,  // Acopio de madera y ramaje
    Framing = 2,             // Estructura y armazón (0% - 50%)
    Thatching = 3,           // Cerramiento y techado (50% - 99%)
    Completed = 4,           // Vivienda habitable al 100%
    Ruined = 5               // Ruina tras catástrofe
};

struct House {
    int id{-1};
    int cellX{0};
    int cellY{0};
    int religionId{1};
    HouseType type{HouseType::ThatchedHut};
    BuildingState state{BuildingState::SitePlanned};
    float buildProgress{0.0f};  // 0.0f a 100.0f
    int woodDelivered{0};
    int woodRequired{12};
    int ownerId{-1};            // Cabeza de familia
    int partnerId{-1};          // Pareja / cónyuge
    std::vector<int> residentIds; // Hijos u otros convivientes
    int capacity{4};
    int sleepingOccupants{0};   // Número de aldeanos durmiendo bajo techo actualmente
    float health{100.0f};
    float maxHealth{100.0f};
    float comfort{1.0f};
    float hearthGlowTimer{0.0f};

    [[nodiscard]] bool isCompleted() const { return state == BuildingState::Completed; }
    [[nodiscard]] bool isUnderConstruction() const {
        return state == BuildingState::Framing || state == BuildingState::Thatching ||
               state == BuildingState::GatheringMaterials || state == BuildingState::SitePlanned;
    }
    [[nodiscard]] bool isRuined() const { return state == BuildingState::Ruined; }

    [[nodiscard]] int getOccupantCount() const {
        int count = 0;
        if (ownerId > 0) count++;
        if (partnerId > 0) count++;
        count += static_cast<int>(residentIds.size());
        return count;
    }

    [[nodiscard]] bool hasSpace() const { return getOccupantCount() < capacity; }
    [[nodiscard]] bool canAccommodate(int count = 1) const { return (getOccupantCount() + count) <= capacity; }

    bool addResident(int humanId) {
        if (!hasSpace()) return false;
        for (int rid : residentIds) {
            if (rid == humanId) return true;
        }
        residentIds.push_back(humanId);
        return true;
    }

    bool removeResident(int humanId) {
        auto it = std::remove(residentIds.begin(), residentIds.end(), humanId);
        if (it != residentIds.end()) {
            residentIds.erase(it, residentIds.end());
            return true;
        }
        return false;
    }

    [[nodiscard]] const char* getTypeName() const {
        switch (type) {
            case HouseType::ThatchedHut: return "Choza Circular de Paja";
            case HouseType::BoneDwelling: return "Cabana de Huesos de Mamut";
            case HouseType::LargeRoundhouse: return "Gran Casa Comunal Redonda";
            case HouseType::ChieftainLonghouse: return "Casa del Cacique / Chaman";
            case HouseType::PrimitiveShelter: return "Refugio Primitivo";
            default: return "Vivienda Tribal";
        }
    }

    [[nodiscard]] const char* getStateName() const {
        switch (state) {
            case BuildingState::SitePlanned: return "Parcela Planificada";
            case BuildingState::GatheringMaterials: return "Acopiando Materiales";
            case BuildingState::Framing: return "Armando Estructura";
            case BuildingState::Thatching: return "Techando";
            case BuildingState::Completed: return "Terminada y Habitada";
            case BuildingState::Ruined: return "En Ruinas";
            default: return "Desconocido";
        }
    }
};

class BuildingSystem {
public:
    BuildingSystem();

    // EN: Update village house construction, integrity, and resident welfare
    // ES: Actualizar la construcción, integridad y bienestar de viviendas
    void update(float deltaTime, WorldMap& mapRef, GestorHumanos& villagerManagerRef,
                EventLine* eventLine = nullptr, int currentYear = -4000,
                SoundSystem* soundSystem = nullptr,
                const IsometricCamera* camera = nullptr,
                ReligiousMoralitySystem* moralitySystem = nullptr);

    int initiateHouseConstruction(int cellX, int cellY, int ownerId, int partnerId,
                                  int religionId, HouseType type, WorldMap& mapRef);

    bool findSuitableHousePlot(int originCellX, int originCellY, const WorldMap& mapRef,
                               int& outCellX, int& outCellY, int searchRadius = 4) const;

    [[nodiscard]] const House* getHouseAt(int cellX, int cellY) const;
    [[nodiscard]] House* getHouseAtMutable(int cellX, int cellY);

    [[nodiscard]] const House* getHouseById(int id) const;
    [[nodiscard]] House* getHouseByIdMutable(int id);

    [[nodiscard]] const std::vector<House>& getHouses() const { return m_houses; }
    [[nodiscard]] std::vector<House>& getHouses() { return m_houses; }

    void evictResident(int humanId);
    void destroyHouse(int id, WorldMap& map, GestorHumanos* humanos = nullptr);
    void clear();

    [[nodiscard]] int getTotalHouseCount() const { return static_cast<int>(m_houses.size()); }
    [[nodiscard]] int getCompletedHouseCount() const;
    [[nodiscard]] int getUnderConstructionCount() const;

private:
    std::vector<House> m_houses;
    int m_nextHouseId{1};
    float m_plotEvaluationTimer{0.0f};

    void cleanOrphanedResidents(GestorHumanos& humanos);
};

} // namespace ReligionSim
