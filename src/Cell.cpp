#include "Cell.hpp"
#include <algorithm>

namespace ReligionSim {

float Cell::getShare(int religionId) const {
    for (const auto& pair : faithShare) {
        if (pair.first == religionId) return pair.second;
    }
    return 0.0f;
}

void Cell::setShare(int religionId, float share) {
    for (auto& pair : faithShare) {
        if (pair.first == religionId) {
            pair.second = std::clamp(share, 0.0f, 1.0f);
            normalizeShares();
            updateDominance();
            return;
        }
    }
    if (share > 0.001f) {
        faithShare.emplace_back(religionId, std::clamp(share, 0.0f, 1.0f));
        normalizeShares();
        updateDominance();
    }
}

void Cell::addShare(int religionId, float delta) {
    float cur = getShare(religionId);
    setShare(religionId, cur + delta);
}

void Cell::normalizeShares() {
    float sum = 0.0f;
    for (const auto& pair : faithShare) {
        sum += pair.second;
    }

    if (sum > 1.0f && sum > 0.0001f) {
        for (auto& pair : faithShare) {
            pair.second /= sum;
        }
    }

    // Filter out minuscule remnants
    faithShare.erase(
        std::remove_if(faithShare.begin(), faithShare.end(),
                       [](const std::pair<int, float>& p) { return p.second < 0.005f; }),
        faithShare.end()
    );
}

void Cell::updateDominance() {
    dominantReligionId = 0;
    dominantFraction = 0.0f;

    for (const auto& pair : faithShare) {
        if (pair.second > dominantFraction) {
            dominantFraction = pair.second;
            dominantReligionId = pair.first;
        }
    }
}

ColorRGB Cell::getBiomeBaseColor() const {
    if (isBurnt) {
        return ColorRGB{52, 46, 42}; // Suelo calcinado y cenizas
    }
    if (isBurning) {
        return ColorRGB{220, 95, 25}; // Suelo en combustión ígnea
    }
    // EN: If a custom atlas terrain sprite is assigned, reflect its base color
    // ES: Si se asigna un sprite de terreno del atlas personalizado, reflejar su color base
    if (customTileIndex >= 0 && customTileIndex < 16) {
        switch (customTileIndex) {
            case 0:  return Palette::GRASS_PASTURE;      // Pasto
            case 1:  return ColorRGB{140, 135, 125};     // Roca / Montaña
            case 2:  return ColorRGB{125, 168, 65};      // Pradera Tropical
            case 3:  return ColorRGB{116, 74, 52};       // Tierra Labranza
            case 4:  return ColorRGB{24, 48, 89};        // Océano Profundo Azul
            case 5:  return ColorRGB{46, 110, 40};       // Suelo Bosque
            case 6:  return ColorRGB{155, 140, 85};      // Pastizal Alto
            case 7:  return ColorRGB{75, 80, 50};        // Pantano / Humedal
            case 8:  return ColorRGB{215, 185, 115};     // Arena Desierto
            case 9:  return ColorRGB{230, 240, 245};     // Nieve / Tundra
            case 10: return ColorRGB{42, 135, 170};      // Costa Somera
            case 11: return ColorRGB{30, 80, 140};       // Mar Templado
            case 12: return ColorRGB{20, 50, 95};        // Agua de Cueva
            case 13: return ColorRGB{12, 28, 65};        // Abisal Profundo
            case 14: return ColorRGB{22, 60, 115};       // Mar Abierto
            case 15: return ColorRGB{220, 80, 20};       // Lava / Magma
            default: break;
        }
    }
    switch (biome) {
        case BiomeType::DeepOcean:
            return {24, 48, 89};
        case BiomeType::ShallowWater:
            return {42, 92, 138};
        case BiomeType::CoastalPlains:
            // Pasto / Llanuras verdes (#74993C)
            return Palette::GRASS_PASTURE;
        case BiomeType::FertileValley:
            // Valle fértil: verde pastizal vibrante (#74993C)
            return Palette::GRASS_PASTURE;
        case BiomeType::DenseForest:
            // Bosque denso: follaje profundo mezclado sobre pasto
            return {46, 110, 40};
        case BiomeType::HighlandHills:
            // Colinas y tierra fértil (#744A34)
            return Palette::DIRT_EARTH;
        case BiomeType::AridDesert:
            return {215, 185, 115};
        case BiomeType::RuggedMountains:
            return {160, 160, 165};
        case BiomeType::FrozenTundra:
            return {215, 230, 240};
        case BiomeType::VolcanicCaldera:
            // EN: Volcanic Caldera: dark basalt volcanic crust with glowing incandescent undertones
            // ES: Caldera Volcánica: corteza volcánica de basalto oscuro con matices incandescentes brillantes
            return {42, 28, 26};
        default:
            return Palette::DIRT_EARTH;
    }
}

} // namespace ReligionSim
