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
        default:
            return Palette::DIRT_EARTH;
    }
}

} // namespace ReligionSim
