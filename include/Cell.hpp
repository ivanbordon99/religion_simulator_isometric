#pragma once

#include "Types.hpp"
#include "MultiNoiseBiome.hpp"
#include <vector>
#include <string>
#include <utility>

namespace ReligionSim {

struct Cell {
    int x{0};
    int y{0};
    float elevation{1.0f}; // 0 = water/ocean, 1 = plains, 2 = hills, 3+ = mountains
    BiomeType biome{BiomeType::CoastalPlains};
    TileBiomeData climate;

    int population{1000};
    int maxPopulation{5000};
    float development{0.1f}; // 0.0 to 1.0 (village -> town -> metropolis)

    // Religion distribution: list of (religionId, fraction of population)
    // where sum of fractions <= 1.0 (remainder is secular/animist folk)
    std::vector<std::pair<int, float>> faithShare;
    int dominantReligionId{0};
    float dominantFraction{0.0f};

    // Holy site & Devotion
    bool hasHolySite{false};
    int shrineLevel{0}; // 0: None, 1: Shrine, 2: Temple, 3: Grand Sanctuary, 4: Wonder
    int holySiteReligionId{0};
    std::string holySiteName;
    float devotion{0.5f}; // Local fervor

    // Tension & Agglutination potential
    float syncreticTension{0.0f};
    int coexistingCenturies{0};

    // Tensión liminal y frontera inter-religiosa
    float borderTension{0.0f};           // 0.0 a 1.0 (intensidad de fricción en el linde)
    bool isContestedBorder{false};       // true si es celda frontera entre dos fes con fricción
    int borderRivalReligionId{0};        // ID de la religión rival al otro lado de la frontera
    bool isSacredTruceActive{false};     // Tregua sagrada o pacto de sangre intertribal
    float sacredTruceTimer{0.0f};        // Duración de la tregua/exogamia en años

    // Recursos naturales generados automáticamente en el mapa
    ResourceType naturalResource{ResourceType::None};
    int resourceAmount{0};
    int maxResourceAmount{0};
    float resourceRegenTimer{0.0f};

    // Viviendas y asentamientos tribales
    bool hasHouse{false};
    int houseId{-1};

    // Cráter permanente de impacto de meteorito
    bool hasImpactCrater{false};
    float craterDepth{0.45f};

    // Estructura y actividad volcánica
    bool hasVolcano{false};
    float volcanoHeight{2.2f};
    bool hasActiveLava{false};
    float lavaTimer{0.0f};

    // Incendios forestales y combustión de bioma
    bool isBurning{false};
    float burnTimer{0.0f};
    bool isBurnt{false};

    int harvestResource(int amount) {
        if (resourceAmount <= 0) return 0;
        int harvested = std::min(amount, resourceAmount);
        resourceAmount -= harvested;
        return harvested;
    }

    // Helper functions
    float getShare(int religionId) const;
    void setShare(int religionId, float share);
    void addShare(int religionId, float delta);
    void normalizeShares();
    void updateDominance();

    bool isWater() const {
        return biome == BiomeType::DeepOcean || biome == BiomeType::ShallowWater;
    }

    bool isDangerousTerrain() const {
        // Terreno peligroso: depresiones profundas/abismales (elevation <= 0.05f)
        // o cumbres de magma/lava escarpada (RuggedMountains o RockyPeak con elevación >= 2.8f)
        return (elevation <= 0.05f) ||
               (biome == BiomeType::RuggedMountains && elevation >= 2.8f) ||
               (climate.primary_biome == BiomeID::RockyPeak && elevation >= 2.8f);
    }

    bool isHabitable() const {
        return !isWater() && biome != BiomeType::RuggedMountains;
    }

    bool isPassableForGround() const {
        return !isWater() && isHabitable() && !isDangerousTerrain();
    }

    ColorRGB getBiomeBaseColor() const;
};

} // namespace ReligionSim
