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

    // Helper functions
    float getShare(int religionId) const;
    void setShare(int religionId, float share);
    void addShare(int religionId, float delta);
    void normalizeShares();
    void updateDominance();

    bool isWater() const {
        return biome == BiomeType::DeepOcean || biome == BiomeType::ShallowWater;
    }

    bool isHabitable() const {
        return !isWater() && biome != BiomeType::RuggedMountains;
    }

    ColorRGB getBiomeBaseColor() const;
};

} // namespace ReligionSim
