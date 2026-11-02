#include "WorldMap.hpp"
#include <cmath>
#include <algorithm>
#include <random>

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

Cell& WorldMap::getCell(int cellX, int cellY) {
    return m_cells[cellY * m_width + cellX];
}

const Cell& WorldMap::getCell(int cellX, int cellY) const {
    return m_cells[cellY * m_width + cellX];
}

// EN: Retrieve cardinal or 8-way neighbors within map bounds
// ES: Obtener vecinos cardinales o en 8 direcciones dentro de los límites del mapa
std::vector<Point2i> WorldMap::getNeighbors(int cellX, int cellY, bool includeDiagonals) const {
    std::vector<Point2i> result;
    result.reserve(includeDiagonals ? 8 : 4);

    static const int DX4[] = {1, -1, 0, 0};
    static const int DY4[] = {0, 0, 1, -1};

    for (int i = 0; i < 4; ++i) {
        int neighborX = cellX + DX4[i];
        int neighborY = cellY + DY4[i];
        if (isValid(neighborX, neighborY)) {
            result.push_back({neighborX, neighborY});
        }
    }

    if (includeDiagonals) {
        static const int DX_DIAG[] = {1, 1, -1, -1};
        static const int DY_DIAG[] = {1, -1, 1, -1};
        for (int i = 0; i < 4; ++i) {
            int neighborX = cellX + DX_DIAG[i];
            int neighborY = cellY + DY_DIAG[i];
            if (isValid(neighborX, neighborY)) {
                result.push_back({neighborX, neighborY});
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

void WorldMap::generateProceduralWorld(uint32_t seed, WorldClimate climate, ResourceAbundance abundance) {
    assignBiomesAndElevations(seed, climate);
    assignPopulationsAndRivers();
    assignNaturalResources(seed, abundance);
}

void WorldMap::assignBiomesAndElevations(uint32_t seed, WorldClimate climate) {
    std::vector<BiomeType> initialBiomes(static_cast<size_t>(m_width * m_height));
    for (int y = 0; y < m_height; ++y) {
        for (int x = 0; x < m_width; ++x) {
            Cell& cell = getCell(x, y);
            cell.climate = MultiNoiseBiomeGenerator::sample(x, y, m_width, m_height, seed, climate);
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

void WorldMap::assignNaturalResources(uint32_t seed, ResourceAbundance abundance) {
    float mult = 1.0f;
    if (abundance == ResourceAbundance::Escaso) mult = 0.55f;
    else if (abundance == ResourceAbundance::Abundante) mult = 1.65f;

    for (int y = 0; y < m_height; ++y) {
        for (int x = 0; x < m_width; ++x) {
            Cell& c = getCell(x, y);
            uint32_t h = seed ^ (static_cast<uint32_t>(x) * 73856093u ^ static_cast<uint32_t>(y) * 19349663u);
            h = (h ^ (h >> 13)) * 0x5bd1e995;
            h ^= (h >> 15);

            if (abundance == ResourceAbundance::Escaso && (h % 5 == 0) && c.biome != BiomeType::FertileValley) {
                c.naturalResource = ResourceType::None;
                c.resourceAmount = 0;
                c.maxResourceAmount = 0;
                continue;
            }

            switch (c.biome) {
                case BiomeType::FertileValley:
                    c.naturalResource = ResourceType::Food;
                    c.resourceAmount = static_cast<int>((220 + static_cast<int>(h % 140)) * mult);
                    c.maxResourceAmount = c.resourceAmount + static_cast<int>(120 * mult);
                    break;
                case BiomeType::CoastalPlains:
                    c.naturalResource = ResourceType::Food;
                    c.resourceAmount = static_cast<int>((160 + static_cast<int>(h % 110)) * mult);
                    c.maxResourceAmount = c.resourceAmount + static_cast<int>(90 * mult);
                    break;
                case BiomeType::ShallowWater:
                    c.naturalResource = ResourceType::Food;
                    c.resourceAmount = static_cast<int>((130 + static_cast<int>(h % 90)) * mult);
                    c.maxResourceAmount = c.resourceAmount + static_cast<int>(70 * mult);
                    break;
                case BiomeType::DenseForest:
                    c.naturalResource = ResourceType::Wood;
                    c.resourceAmount = static_cast<int>((280 + static_cast<int>(h % 180)) * mult);
                    c.maxResourceAmount = c.resourceAmount + static_cast<int>(140 * mult);
                    break;
                case BiomeType::HighlandHills: {
                    int roll = static_cast<int>(h % 100);
                    if (roll < 45) {
                        c.naturalResource = ResourceType::Stone;
                        c.resourceAmount = static_cast<int>((220 + static_cast<int>(h % 130)) * mult);
                        c.maxResourceAmount = c.resourceAmount + static_cast<int>(100 * mult);
                    } else if (roll < 75) {
                        c.naturalResource = ResourceType::Iron;
                        c.resourceAmount = static_cast<int>((140 + static_cast<int>(h % 100)) * mult);
                        c.maxResourceAmount = c.resourceAmount + static_cast<int>(80 * mult);
                    } else {
                        c.naturalResource = ResourceType::Gold;
                        c.resourceAmount = static_cast<int>((70 + static_cast<int>(h % 80)) * mult);
                        c.maxResourceAmount = c.resourceAmount + static_cast<int>(60 * mult);
                    }
                    break;
                }
                case BiomeType::RuggedMountains: {
                    int roll = static_cast<int>(h % 100);
                    if (roll < 40) {
                        c.naturalResource = ResourceType::Stone;
                        c.resourceAmount = static_cast<int>((320 + static_cast<int>(h % 180)) * mult);
                        c.maxResourceAmount = c.resourceAmount + static_cast<int>(120 * mult);
                    } else if (roll < 75) {
                        c.naturalResource = ResourceType::Iron;
                        c.resourceAmount = static_cast<int>((180 + static_cast<int>(h % 120)) * mult);
                        c.maxResourceAmount = c.resourceAmount + static_cast<int>(100 * mult);
                    } else {
                        c.naturalResource = ResourceType::Gold;
                        c.resourceAmount = static_cast<int>((110 + static_cast<int>(h % 140)) * mult);
                        c.maxResourceAmount = c.resourceAmount + static_cast<int>(90 * mult);
                    }
                    break;
                }
                case BiomeType::AridDesert:
                    c.naturalResource = ResourceType::Stone;
                    c.resourceAmount = static_cast<int>((90 + static_cast<int>(h % 60)) * mult);
                    c.maxResourceAmount = c.resourceAmount + static_cast<int>(50 * mult);
                    break;
                case BiomeType::FrozenTundra:
                    if ((h % 2) == 0) {
                        c.naturalResource = ResourceType::Food;
                        c.resourceAmount = static_cast<int>((90 + static_cast<int>(h % 50)) * mult);
                        c.maxResourceAmount = c.resourceAmount + static_cast<int>(40 * mult);
                    } else {
                        c.naturalResource = ResourceType::Stone;
                        c.resourceAmount = static_cast<int>((80 + static_cast<int>(h % 50)) * mult);
                        c.maxResourceAmount = c.resourceAmount + static_cast<int>(40 * mult);
                    }
                    break;
                case BiomeType::DeepOcean:
                default:
                    c.naturalResource = ResourceType::None;
                    c.resourceAmount = 0;
                    c.maxResourceAmount = 0;
                    break;
            }
        }
    }
}

// EN: Regenerate natural resources periodically up to their cell capacity
// ES: Regenerar recursos naturales periódicamente hasta la capacidad de la celda
void WorldMap::updateResources(float deltaTime) {
    if (deltaTime <= 0.0f) return;
    for (int cellY = 0; cellY < m_height; ++cellY) {
        for (int cellX = 0; cellX < m_width; ++cellX) {
            Cell& currentCell = getCell(cellX, cellY);
            if (currentCell.naturalResource != ResourceType::None && currentCell.resourceAmount < currentCell.maxResourceAmount) {
                currentCell.resourceRegenTimer += deltaTime;
                if (currentCell.resourceRegenTimer >= 10.0f) {
                    currentCell.resourceRegenTimer = 0.0f;
                    currentCell.resourceAmount = std::min(currentCell.maxResourceAmount, currentCell.resourceAmount + 1);
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

    // Ubicar pueblos fundacionales con separación mínima garantizada
    std::vector<Point2i> puebloLocations = generatePuebloLocations(5, 42);

    if (puebloLocations.size() >= 5) {
        int faiths[] = {rel1, rel2, rel3, rel4, rel5};
        const char* holyNames[] = {
            "Sun Spire of Heliopolis",
            "Whispering Grove of the Ancients",
            "Monad Citadel of the Heavens",
            "Sacred Flame of the Red Mesa",
            "Sanctuary of the Eternal Wheel"
        };

        for (int i = 0; i < 5; ++i) {
            Point2i pt = puebloLocations[i];
            Cell& c = getCell(pt.x, pt.y);
            c.setShare(faiths[i], 0.95f);
            c.devotion = 0.90f;
            consecrateHolySite(pt.x, pt.y, holyNames[i]);
            c.shrineLevel = 2; // Established Temple

            // Sembrar la comarca territorial del pueblo (radio 2)
            for (int dy = -2; dy <= 2; ++dy) {
                for (int dx = -2; dx <= 2; ++dx) {
                    int nx = pt.x + dx;
                    int ny = pt.y + dy;
                    float d = std::sqrt(static_cast<float>(dx * dx + dy * dy));
                    if (isValid(nx, ny) && d <= 2.5f && (dx != 0 || dy != 0)) {
                        Cell& nbc = getCell(nx, ny);
                        if (nbc.isHabitable()) {
                            nbc.setShare(faiths[i], std::max(0.60f, 0.90f - d * 0.12f));
                            nbc.devotion = std::max(0.55f, 0.85f - d * 0.12f);
                        }
                    }
                }
            }
        }
    }

    recalculateReligions(registry);
}

std::vector<Point2i> WorldMap::generatePuebloLocations(int count, uint32_t seed) const {
    std::vector<Point2i> result;
    if (count <= 0) return result;

    const int margin = std::clamp(static_cast<int>(std::min(m_width, m_height) * 0.08f), 3, 5);

    // 1. Recolectar celdas candidatas viables para asentamientos humanos
    struct Candidate {
        Point2i pt;
        int habitableNeighbors;
        int qualityScore;
    };
    std::vector<Candidate> candidates;
    candidates.reserve(m_width * m_height / 4);

    for (int y = margin; y < m_height - margin; ++y) {
        for (int x = margin; x < m_width - margin; ++x) {
            const Cell& c = getCell(x, y);
            if (!c.isHabitable()) continue;

            // Evaluar vecindario 5x5 (radio 2) para asegurar masa de tierra sólida y segura
            int habCount = 0;
            int resourceBonus = 0;
            for (int dy = -2; dy <= 2; ++dy) {
                for (int dx = -2; dx <= 2; ++dx) {
                    int nx = x + dx;
                    int ny = y + dy;
                    if (isValid(nx, ny) && getCell(nx, ny).isHabitable()) {
                        habCount++;
                        if (getCell(nx, ny).naturalResource != ResourceType::None) {
                            resourceBonus += 2;
                        }
                    }
                }
            }

            // Exigir al menos 13 celdas habitables de las 25 posibles en su entorno
            if (habCount >= 13) {
                int score = habCount + resourceBonus;
                if (c.biome == BiomeType::FertileValley) score += 15;
                else if (c.biome == BiomeType::CoastalPlains) score += 10;
                else if (c.biome == BiomeType::DenseForest) score += 8;
                else if (c.biome == BiomeType::HighlandHills) score += 5;

                candidates.push_back({Point2i{x, y}, habCount, score});
            }
        }
    }

    // Fallback si el mapa es extremadamente agreste o archipiélago angosto
    if (static_cast<int>(candidates.size()) < count * 2) {
        for (int y = 2; y < m_height - 2; ++y) {
            for (int x = 2; x < m_width - 2; ++x) {
                const Cell& c = getCell(x, y);
                if (c.isHabitable()) {
                    candidates.push_back({Point2i{x, y}, 8, 10});
                }
            }
        }
    }

    if (candidates.empty()) {
        result.push_back({m_width / 2, m_height / 2});
        return result;
    }

    // PRNG reproducible basado en la semilla del mundo
    std::mt19937 rng(seed ^ 0x9E3779B9u);
    std::shuffle(candidates.begin(), candidates.end(), rng);

    // Distancia euclídea al cuadrado
    auto distSq = [](const Point2i& a, const Point2i& b) {
        float dx = static_cast<float>(a.x - b.x);
        float dy = static_cast<float>(a.y - b.y);
        return dx * dx + dy * dy;
    };

    // Calcular distancia mínima objetivo y cota inferior segura
    float targetMinDist = std::min(m_width, m_height) * 0.38f;
    float minDistFloor = std::max(8.0f, std::min(m_width, m_height) * 0.22f);
    float currentMinDist = targetMinDist;

    bool success = false;
    while (currentMinDist >= minDistFloor && !success) {
        float reqSq = currentMinDist * currentMinDist;

        // Intentar varias semillas de muestreo para maximizar el confort del mapa
        for (int attempt = 0; attempt < 60; ++attempt) {
            std::vector<Point2i> chosen;

            // Escoger el primer pueblo (Hogar del Jugador): preferir un candidato con alto score cerca de la zona central/balanceada
            std::vector<Point2i> poolFirst;
            for (const auto& cand : candidates) {
                float distFromCenter = std::sqrt(distSq(cand.pt, {m_width / 2, m_height / 2}));
                if (distFromCenter <= m_width * 0.35f && cand.qualityScore >= 16) {
                    poolFirst.push_back(cand.pt);
                }
            }
            if (poolFirst.empty()) {
                poolFirst.push_back(candidates[0].pt);
            }
            std::uniform_int_distribution<size_t> firstDist(0, poolFirst.size() - 1);
            chosen.push_back(poolFirst[firstDist(rng)]);

            // Escoger los siguientes pueblos maximizando la distancia a todos los ya elegidos
            for (int p = 1; p < count; ++p) {
                float bestMinDistToChosen = -1.0f;
                Point2i bestCandidate{-1, -1};

                for (const auto& cand : candidates) {
                    float closestSq = 1e9f;
                    for (const auto& ch : chosen) {
                        float d = distSq(cand.pt, ch);
                        if (d < closestSq) closestSq = d;
                    }

                    if (closestSq >= reqSq) {
                        // Cumple la distancia mínima; ponderar por distancia mínima y calidad
                        float weight = closestSq + static_cast<float>(cand.qualityScore) * 6.0f;
                        if (weight > bestMinDistToChosen) {
                            bestMinDistToChosen = weight;
                            bestCandidate = cand.pt;
                        }
                    }
                }

                if (bestCandidate.x != -1) {
                    chosen.push_back(bestCandidate);
                } else {
                    break; // No cupo otro pueblo con esta distancia
                }
            }

            if (static_cast<int>(chosen.size()) == count) {
                result = std::move(chosen);
                success = true;
                break;
            }
        }

        if (!success) {
            currentMinDist -= 1.0f; // Relajar levemente
        }
    }

    // Si aún faltasen (mapas extremos), rellenar con los candidatos más lejanos disponibles
    while (static_cast<int>(result.size()) < count && !candidates.empty()) {
        float maxMinDist = -1.0f;
        Point2i bestCand = candidates[0].pt;
        for (const auto& cand : candidates) {
            float minDist = 1e9f;
            for (const auto& ch : result) {
                float d = distSq(cand.pt, ch);
                if (d < minDist) minDist = d;
            }
            if (minDist > maxMinDist) {
                maxMinDist = minDist;
                bestCand = cand.pt;
            }
        }
        result.push_back(bestCand);
    }

    return result;
}

// EN: Consecrate a cell as a designated holy site and assign its patron religion
// ES: Consagrar una celda como sitio sagrado y asignar su religión patrona
void WorldMap::consecrateHolySite(int cellX, int cellY, const std::string& name, int religionId) {
    if (!isValid(cellX, cellY)) return;
    Cell& targetCell = getCell(cellX, cellY);
    int targetRel = (religionId > 0) ? religionId : ((targetCell.dominantReligionId > 0) ? targetCell.dominantReligionId : ((targetCell.holySiteReligionId > 0) ? targetCell.holySiteReligionId : 1));
    if (religionId > 0) {
        targetCell.setShare(targetRel, std::max(targetCell.getShare(targetRel), 0.90f));
    }

    // EN: Ensure each religion has at most one main temple site
    // ES: Garantizar que cada religión tenga como máximo una sola celda de Templo principal
    for (auto& existing : m_cells) {
        if (existing.hasHolySite && existing.holySiteReligionId == targetRel &&
            (existing.x != cellX || existing.y != cellY)) {
            existing.hasHolySite = false;
            existing.shrineLevel = 0;
            existing.holySiteReligionId = 0;
            existing.holySiteName.clear();
        }
    }

    targetCell.hasHolySite = true;
    targetCell.shrineLevel = std::max(targetCell.shrineLevel, 1);
    targetCell.holySiteReligionId = targetRel;
    targetCell.holySiteName = name;
    targetCell.devotion = std::min(1.0f, targetCell.devotion + 0.25f);
}

// EN: Upgrade temple structure tiers (Shrine -> Temple -> Grand Sanctuary -> Wonder)
// ES: Mejorar el nivel estructural del templo (Santuario -> Templo -> Gran Santuario -> Maravilla)
void WorldMap::upgradeHolySite(int cellX, int cellY) {
    if (!isValid(cellX, cellY)) return;
    Cell& targetCell = getCell(cellX, cellY);
    if (targetCell.hasHolySite) {
        targetCell.shrineLevel = std::min(4, targetCell.shrineLevel + 1);
        targetCell.devotion = std::min(1.0f, targetCell.devotion + 0.15f);
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
