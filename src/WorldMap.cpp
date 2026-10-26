#include "WorldMap.hpp"
#include <cmath>
#include <algorithm>

namespace ReligionSim {

namespace {

bool isWaterBiome(BiomeType biome) {
    return biome == BiomeType::DeepOcean || biome == BiomeType::ShallowWater;
}

BiomeType mapBiomeID(BiomeID biome) {
    switch (biome) {
        case BiomeID::Ocean:              return BiomeType::DeepOcean;
        case BiomeID::Coast:              return BiomeType::ShallowWater;
        case BiomeID::Desert:             return BiomeType::AridDesert;
        case BiomeID::Savanna:             return BiomeType::CoastalPlains;
        case BiomeID::Plains:              return BiomeType::FertileValley;
        case BiomeID::TemperateForest:     return BiomeType::DenseForest;
        case BiomeID::Taiga:               return BiomeType::FrozenTundra;
        case BiomeID::TropicalRainforest:  return BiomeType::DenseForest;
        case BiomeID::SnowyMountain:       return BiomeType::FrozenTundra;
        case BiomeID::RockyPeak:           return BiomeType::RuggedMountains;
        case BiomeID::Count:               break;
    }
    return BiomeType::FertileValley;
}

std::vector<BiomeType> smoothBiomeRegions(const std::vector<BiomeType>& source, int width, int height) {
    std::vector<BiomeType> current = source;
    for (int pass = 0; pass < 3; ++pass) {
        std::vector<BiomeType> next = current;
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                const size_t index = static_cast<size_t>(y * width + x);
                const BiomeType biome = current[index];
                int scores[9]{};
                scores[static_cast<size_t>(biome)] += 3;

                for (int oy = -1; oy <= 1; ++oy) {
                    for (int ox = -1; ox <= 1; ++ox) {
                        if (ox == 0 && oy == 0) continue;
                        const int nx = x + ox;
                        const int ny = y + oy;
                        if (nx < 0 || nx >= width || ny < 0 || ny >= height) continue;
                        const BiomeType neighbor = current[static_cast<size_t>(ny * width + nx)];
                        if (isWaterBiome(neighbor) == isWaterBiome(biome)) {
                            ++scores[static_cast<size_t>(neighbor)];
                        }
                    }
                }

                BiomeType best = biome;
                for (int candidate = 0; candidate < 9; ++candidate) {
                    const BiomeType candidateBiome = static_cast<BiomeType>(candidate);
                    if (isWaterBiome(candidateBiome) != isWaterBiome(biome)) continue;
                    if (scores[static_cast<size_t>(candidateBiome)] > scores[static_cast<size_t>(best)]) {
                        best = candidateBiome;
                    }
                }
                next[index] = best;
            }
        }
        current.swap(next);
    }

    for (int pass = 0; pass < 3; ++pass) {
        std::vector<bool> visited(current.size(), false);
        std::vector<std::pair<size_t, BiomeType>> repairs;

        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                const size_t start = static_cast<size_t>(y * width + x);
                if (visited[start]) continue;

                const BiomeType biome = current[start];
                std::vector<size_t> region{start};
                visited[start] = true;
                for (size_t cursor = 0; cursor < region.size(); ++cursor) {
                    const size_t index = region[cursor];
                    const int cellX = static_cast<int>(index % static_cast<size_t>(width));
                    const int cellY = static_cast<int>(index / static_cast<size_t>(width));
                    for (int oy = -1; oy <= 1; ++oy) {
                        for (int ox = -1; ox <= 1; ++ox) {
                            const int nx = cellX + ox;
                            const int ny = cellY + oy;
                            if ((ox == 0 && oy == 0) || nx < 0 || nx >= width ||
                                ny < 0 || ny >= height) continue;
                            const size_t neighborIndex =
                                static_cast<size_t>(ny * width + nx);
                            if (!visited[neighborIndex] && current[neighborIndex] == biome) {
                                visited[neighborIndex] = true;
                                region.push_back(neighborIndex);
                            }
                        }
                    }
                }

                if (region.size() >= 3) continue;
                int votes[9]{};
                for (size_t index : region) {
                    const int cellX = static_cast<int>(index % static_cast<size_t>(width));
                    const int cellY = static_cast<int>(index / static_cast<size_t>(width));
                    for (int oy = -1; oy <= 1; ++oy) {
                        for (int ox = -1; ox <= 1; ++ox) {
                            const int nx = cellX + ox;
                            const int ny = cellY + oy;
                            if ((ox == 0 && oy == 0) || nx < 0 || nx >= width ||
                                ny < 0 || ny >= height) continue;
                            const BiomeType neighbor =
                                current[static_cast<size_t>(ny * width + nx)];
                            if (neighbor != biome &&
                                isWaterBiome(neighbor) == isWaterBiome(biome)) {
                                ++votes[static_cast<size_t>(neighbor)];
                            }
                        }
                    }
                }

                BiomeType replacement = biome;
                for (int candidate = 0; candidate < 9; ++candidate) {
                    const BiomeType candidateBiome = static_cast<BiomeType>(candidate);
                    if (isWaterBiome(candidateBiome) != isWaterBiome(biome)) continue;
                    if (votes[static_cast<size_t>(candidateBiome)] >
                        votes[static_cast<size_t>(replacement)]) {
                        replacement = candidateBiome;
                    }
                }
                if (replacement != biome) {
                    for (size_t index : region) repairs.emplace_back(index, replacement);
                }
            }
        }

        if (repairs.empty()) break;
        for (const auto& repair : repairs) current[repair.first] = repair.second;
    }
    return current;
}

} // namespace

WorldMap::WorldMap(int width, int height)
    : m_width(width), m_height(height) {
    m_cells.resize(width * height);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            Cell& c = m_cells[y * width + x];
            c.x = x;
            c.y = y;
        }
    }
}

Cell& WorldMap::getCell(int x, int y) {
    return m_cells[y * m_width + x];
}

const Cell& WorldMap::getCell(int x, int y) const {
    return m_cells[y * m_width + x];
}

std::vector<Point2i> WorldMap::getNeighbors(int x, int y, bool includeDiagonals) const {
    std::vector<Point2i> result;
    result.reserve(includeDiagonals ? 8 : 4);

    static const int DX4[] = {1, -1, 0, 0};
    static const int DY4[] = {0, 0, 1, -1};

    for (int i = 0; i < 4; ++i) {
        int nx = x + DX4[i];
        int ny = y + DY4[i];
        if (isValid(nx, ny)) {
            result.push_back({nx, ny});
        }
    }

    if (includeDiagonals) {
        static const int DX_DIAG[] = {1, 1, -1, -1};
        static const int DY_DIAG[] = {1, -1, 1, -1};
        for (int i = 0; i < 4; ++i) {
            int nx = x + DX_DIAG[i];
            int ny = y + DY_DIAG[i];
            if (isValid(nx, ny)) {
                result.push_back({nx, ny});
            }
        }
    }

    return result;
}

float WorldMap::getTravelEase(const Cell& from, const Cell& to) const {
    if (from.isWater() || to.isWater()) {
        // Coastal navigation is quick, deep ocean is a barrier early on
        if (from.biome == BiomeType::DeepOcean || to.biome == BiomeType::DeepOcean) {
            return 0.15f;
        }
        return 0.9f; // Shallow coastal trade
    }

    if (to.biome == BiomeType::FertileValley || to.biome == BiomeType::CoastalPlains) {
        return 1.2f;
    }
    if (to.biome == BiomeType::DenseForest) {
        return 0.7f;
    }
    if (to.biome == BiomeType::AridDesert) {
        return 0.4f;
    }
    if (to.biome == BiomeType::RuggedMountains) {
        return 0.2f; // Mountain passes isolate cultures
    }
    if (to.biome == BiomeType::FrozenTundra) {
        return 0.3f;
    }
    return 1.0f;
}

void WorldMap::generateProceduralWorld(uint32_t seed) {
    assignBiomesAndElevations(seed);
    assignPopulationsAndRivers();
}

void WorldMap::assignBiomesAndElevations(uint32_t seed) {
    std::vector<BiomeType> initialBiomes(static_cast<size_t>(m_width * m_height));
    for (int y = 0; y < m_height; ++y) {
        for (int x = 0; x < m_width; ++x) {
            Cell& cell = getCell(x, y);
            cell.climate = MultiNoiseBiomeGenerator::sample(x, y, m_width, m_height, seed);
            initialBiomes[static_cast<size_t>(y * m_width + x)] =
                mapBiomeID(cell.climate.primary_biome);
        }
    }

    // Resolve every cell's biome first, then validate the map-wide regions before rendering.
    const std::vector<BiomeType> smoothedBiomes =
        smoothBiomeRegions(initialBiomes, m_width, m_height);

    for (int y = 0; y < m_height; ++y) {
        for (int x = 0; x < m_width; ++x) {
            Cell& cell = getCell(x, y);
            cell.biome = smoothedBiomes[static_cast<size_t>(y * m_width + x)];
            cell.elevation = isWaterBiome(cell.biome)
                ? 0.0f
                : ((cell.biome == BiomeType::HighlandHills || cell.biome == BiomeType::RuggedMountains)
                    ? 0.95f
                    : 0.50f);
        }
    }
}

void WorldMap::assignPopulationsAndRivers() {
    for (int y = 0; y < m_height; ++y) {
        for (int x = 0; x < m_width; ++x) {
            Cell& c = getCell(x, y);
            c.faithShare.clear();
            c.dominantReligionId = 0;
            c.dominantFraction = 0.0f;
            c.hasHolySite = false;
            c.shrineLevel = 0;

            if (c.isWater()) {
                c.population = 0;
                c.maxPopulation = 0;
                c.development = 0.0f;
            } else {
                switch (c.biome) {
                    case BiomeType::FertileValley:
                        c.maxPopulation = 10;
                        c.population = 4;
                        c.development = 0.25f;
                        break;
                    case BiomeType::CoastalPlains:
                        c.maxPopulation = 8;
                        c.population = 3;
                        c.development = 0.20f;
                        break;
                    case BiomeType::DenseForest:
                        c.maxPopulation = 5;
                        c.population = 2;
                        c.development = 0.12f;
                        break;
                    case BiomeType::HighlandHills:
                        c.maxPopulation = 4;
                        c.population = 1;
                        c.development = 0.10f;
                        break;
                    case BiomeType::AridDesert:
                        c.maxPopulation = 2;
                        c.population = 1;
                        c.development = 0.05f;
                        break;
                    case BiomeType::RuggedMountains:
                        c.maxPopulation = 1;
                        c.population = 0;
                        c.development = 0.04f;
                        break;
                    case BiomeType::FrozenTundra:
                        c.maxPopulation = 2;
                        c.population = 1;
                        c.development = 0.05f;
                        break;
                    default:
                        c.maxPopulation = 4;
                        c.population = 1;
                        c.development = 0.10f;
                        break;
                }
            }
        }
    }
}

void WorldMap::seedPrimordialFaiths(ReligionRegistry& registry, int currentYear) {
    // Hearth 1: Solar Atenism (East / Fertile River delta)
    int rel1 = registry.createPrimordialReligion(
        "Solar Atenism", "Akhen-Ra",
        ColorRGB(244, 162, 97), "Sun", currentYear
    );

    // Hearth 2: Way of the Whispering Earth (Forest / Mountains)
    int rel2 = registry.createPrimordialReligion(
        "Animist Gaia-Cult", "Zarah-Khep",
        ColorRGB(46, 204, 113), "Tree", currentYear
    );

    // Hearth 3: Order of the Azure Monad (Highland Plateau)
    int rel3 = registry.createPrimordialReligion(
        "The Sacred Monad", "Melchizedek",
        ColorRGB(52, 152, 219), "Spiral", currentYear
    );

    // Hearth 4: Flame of Mithras (Desert / Canyon)
    int rel4 = registry.createPrimordialReligion(
        "Order of the Eternal Flame", "Mithras-Gard",
        ColorRGB(230, 57, 70), "Flame", currentYear
    );

    // Hearth 5: Celestial Wheel of Dharma (Southern Basin)
    int rel5 = registry.createPrimordialReligion(
        "Wheel of the Star-Dharma", "Siddhartha-Yin",
        ColorRGB(155, 89, 182), "Wheel", currentYear
    );

    // Find good habitable locations for each hearth
    std::vector<Point2i> fertileLocations;
    for (int y = 5; y < m_height - 5; ++y) {
        for (int x = 5; x < m_width - 5; ++x) {
            const Cell& c = getCell(x, y);
            if (c.biome == BiomeType::FertileValley || c.biome == BiomeType::CoastalPlains) {
                fertileLocations.push_back({x, y});
            }
        }
    }

    if (fertileLocations.size() >= 5) {
        int step = static_cast<int>(fertileLocations.size()) / 5;
        int faiths[] = {rel1, rel2, rel3, rel4, rel5};
        const char* holyNames[] = {
            "Sun Spire of Heliopolis",
            "Whispering Grove of the Ancients",
            "Monad Citadel of the Heavens",
            "Sacred Flame of the Red Mesa",
            "Sanctuary of the Eternal Wheel"
        };

        for (int i = 0; i < 5; ++i) {
            Point2i pt = fertileLocations[i * step];
            Cell& c = getCell(pt.x, pt.y);
            c.setShare(faiths[i], 0.95f);
            c.devotion = 0.90f;
            consecrateHolySite(pt.x, pt.y, holyNames[i]);
            c.shrineLevel = 2; // Established Temple

            // Also seed immediate neighbors with moderate faith
            auto neighbors = getNeighbors(pt.x, pt.y, true);
            for (const auto& nb : neighbors) {
                Cell& nbc = getCell(nb.x, nb.y);
                if (nbc.isHabitable()) {
                    nbc.setShare(faiths[i], 0.65f);
                    nbc.devotion = 0.70f;
                }
            }
        }
    }

    recalculateReligions(registry);
}

void WorldMap::consecrateHolySite(int x, int y, const std::string& name) {
    if (!isValid(x, y)) return;
    Cell& c = getCell(x, y);
    int targetRel = (c.dominantReligionId > 0) ? c.dominantReligionId : ((c.holySiteReligionId > 0) ? c.holySiteReligionId : 1);

    // Garantizar que cada religión tenga como máximo una sola celda de Templo principal
    for (auto& existing : m_cells) {
        if (existing.hasHolySite && existing.holySiteReligionId == targetRel &&
            (existing.x != x || existing.y != y)) {
            existing.hasHolySite = false;
            existing.shrineLevel = 0;
            existing.holySiteReligionId = 0;
            existing.holySiteName.clear();
        }
    }

    c.hasHolySite = true;
    c.shrineLevel = std::max(c.shrineLevel, 1);
    c.holySiteReligionId = targetRel;
    c.holySiteName = name;
    c.devotion = std::min(1.0f, c.devotion + 0.25f);
}

void WorldMap::upgradeHolySite(int x, int y) {
    if (!isValid(x, y)) return;
    Cell& c = getCell(x, y);
    if (c.hasHolySite) {
        c.shrineLevel = std::min(4, c.shrineLevel + 1);
        c.devotion = std::min(1.0f, c.devotion + 0.15f);
    }
}

int WorldMap::getTotalWorldPopulation() const {
    int total = 0;
    for (const auto& c : m_cells) {
        total += c.population;
    }
    return total;
}

void WorldMap::recalculateReligions(ReligionRegistry& registry) {
    for (auto& r : registry.getAllReligions()) {
        r.totalFollowers = 0;
        r.totalCellsDominant = 0;
        r.holySitesCount = 0;
    }

    for (const auto& c : m_cells) {
        if (!c.isHabitable()) continue;

        if (c.hasHolySite) {
            int hsRel = (c.dominantReligionId > 0) ? c.dominantReligionId : c.holySiteReligionId;
            if (hsRel > 0) {
                Religion* r = registry.getReligion(hsRel);
                if (r) r->holySitesCount++;
            }
        }

        if (c.dominantReligionId > 0 && c.dominantFraction > 0.40f) {
            Religion* r = registry.getReligion(c.dominantReligionId);
            if (r) r->totalCellsDominant++;
        }

        for (const auto& p : c.faithShare) {
            Religion* r = registry.getReligion(p.first);
            if (r) {
                r->totalFollowers += static_cast<int>(std::round(static_cast<float>(c.population) * p.second));
            }
        }
    }

    registry.updateStats(getTotalWorldPopulation());
}

} // namespace ReligionSim
