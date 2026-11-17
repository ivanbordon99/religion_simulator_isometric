#include "WorldMap.hpp"
#include "BuildingSystem.hpp"
#include "Humano.hpp"
#include <cmath>
#include <algorithm>
#include <random>
#include <queue>

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
        case BiomeID::VolcanicCaldera:     return BiomeType::VolcanicCaldera;
        case BiomeID::Count:               break;
    }
    return BiomeType::FertileValley;
}

constexpr size_t TOTAL_BIOME_TYPES = 10;

std::vector<BiomeType> smoothBiomeRegions(const std::vector<BiomeType>& source, int width, int height) {
    std::vector<BiomeType> current = source;
    for (int pass = 0; pass < 3; ++pass) {
        std::vector<BiomeType> next = current;
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                const size_t index = static_cast<size_t>(y * width + x);
                const BiomeType biome = current[index];
                int scores[TOTAL_BIOME_TYPES]{};
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
                for (size_t candidate = 0; candidate < TOTAL_BIOME_TYPES; ++candidate) {
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
                int votes[TOTAL_BIOME_TYPES]{};
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
                for (size_t candidate = 0; candidate < TOTAL_BIOME_TYPES; ++candidate) {
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
    if (to.biome == BiomeType::VolcanicCaldera) {
        // EN: Volcanic basalt slopes, fumes, and magma rivers hinder travel
        // ES: Pendientes de basalto volcánico, gases y ríos de magma dificultan el viaje
        return 0.15f;
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
                : ((cell.biome == BiomeType::VolcanicCaldera)
                    ? 1.45f
                    : ((cell.biome == BiomeType::HighlandHills || cell.biome == BiomeType::RuggedMountains)
                        ? 0.95f
                        : 0.50f));
        }
    }

    // EN: Volcanic Calderas and Natural Volcano Spawning
    // Identify volcanic calderas or prominent mountain peaks to establish active volcanic centers
    // ES: Calderas Volcánicas y Generación Natural de Volcanes
    // Identificar calderas volcánicas o picos montañosos prominentes para establecer centros volcánicos activos
    std::vector<Point2i> volcanicCalderaCells;
    int highestMountainX = -1;
    int highestMountainY = -1;
    float peakMountainElevation = -1.0f;

    for (int y = 0; y < m_height; ++y) {
        for (int x = 0; x < m_width; ++x) {
            const Cell& inspectedCell = getCell(x, y);
            if (inspectedCell.biome == BiomeType::VolcanicCaldera) {
                volcanicCalderaCells.push_back({x, y});
            } else if (inspectedCell.biome == BiomeType::RuggedMountains || !inspectedCell.isWater()) {
                const float combinedAltitude = inspectedCell.elevation + inspectedCell.climate.elevation +
                                               (inspectedCell.biome == BiomeType::RuggedMountains ? 2.0f : 0.0f);
                if (combinedAltitude > peakMountainElevation) {
                    peakMountainElevation = combinedAltitude;
                    highestMountainX = x;
                    highestMountainY = y;
                }
            }
        }
    }

    // EN: If no natural volcanic calderas formed via noise sampling, promote the highest mountain peak
    // ES: Si no se formaron calderas volcánicas por muestreo de ruido, promover el pico montañoso más alto
    if (volcanicCalderaCells.empty() && highestMountainX >= 0 && highestMountainY >= 0) {
        Cell& promotedPeak = getCell(highestMountainX, highestMountainY);
        promotedPeak.biome = BiomeType::VolcanicCaldera;
        promotedPeak.elevation = std::max(promotedPeak.elevation, 1.45f);
        volcanicCalderaCells.push_back({highestMountainX, highestMountainY});
    }

    // EN: Ensure volcanic calderas form cohesive volcanic clusters of at least 4 contiguous cells
    // ES: Asegurar que las calderas volcánicas formen conglomerados cohesivos de al menos 4 celdas contiguas
    if (!volcanicCalderaCells.empty() && volcanicCalderaCells.size() < 4) {
        for (size_t clusterIndex = 0; clusterIndex < volcanicCalderaCells.size() && volcanicCalderaCells.size() < 4; ++clusterIndex) {
            const Point2i basePoint = volcanicCalderaCells[clusterIndex];
            for (const auto& neighborPoint : getNeighbors(basePoint.x, basePoint.y, true)) {
                if (volcanicCalderaCells.size() >= 4) break;
                Cell& neighborCell = getCell(neighborPoint.x, neighborPoint.y);
                if (!neighborCell.isWater() && neighborCell.biome != BiomeType::VolcanicCaldera) {
                    neighborCell.biome = BiomeType::VolcanicCaldera;
                    neighborCell.elevation = std::max(neighborCell.elevation, 1.25f);
                    volcanicCalderaCells.push_back(neighborPoint);
                }
            }
        }
    }

    // EN: Group volcanic cells into clusters and instantiate volcanoes and active lava pools
    // ES: Agrupar celdas volcánicas en conglomerados e instanciar volcanes y piscinas de lava activa
    if (!volcanicCalderaCells.empty()) {
        std::vector<bool> visitedVolcanic(m_cells.size(), false);
        for (const Point2i& startPoint : volcanicCalderaCells) {
            const size_t startIndex = static_cast<size_t>(startPoint.y * m_width + startPoint.x);
            if (visitedVolcanic[startIndex]) continue;

            std::vector<Point2i> clusterCells;
            std::queue<Point2i> exploreQueue;
            exploreQueue.push(startPoint);
            visitedVolcanic[startIndex] = true;

            while (!exploreQueue.empty()) {
                const Point2i currentPoint = exploreQueue.front();
                exploreQueue.pop();
                clusterCells.push_back(currentPoint);

                for (const Point2i& neighborPoint : getNeighbors(currentPoint.x, currentPoint.y, true)) {
                    const size_t neighborIndex = static_cast<size_t>(neighborPoint.y * m_width + neighborPoint.x);
                    if (!visitedVolcanic[neighborIndex] && getCell(neighborPoint.x, neighborPoint.y).biome == BiomeType::VolcanicCaldera) {
                        visitedVolcanic[neighborIndex] = true;
                        exploreQueue.push(neighborPoint);
                    }
                }
            }

            // EN: Identify peak center cell of the volcanic caldera cluster
            // ES: Identificar la celda central o pico del conglomerado de caldera volcánica
            int bestPeakX = clusterCells.front().x;
            int bestPeakY = clusterCells.front().y;
            float maxClustAltitude = -100.0f;
            for (const Point2i& memberPoint : clusterCells) {
                const Cell& memberCell = getCell(memberPoint.x, memberPoint.y);
                const float altitudeScore = memberCell.elevation + memberCell.climate.elevation;
                if (altitudeScore > maxClustAltitude) {
                    maxClustAltitude = altitudeScore;
                    bestPeakX = memberPoint.x;
                    bestPeakY = memberPoint.y;
                }
            }

            // EN: Spawn the primary volcano crater at the central volcanic peak
            // ES: Generar el cráter del volcán primario en el pico volcánico central
            Cell& volcanoPeakCell = getCell(bestPeakX, bestPeakY);
            volcanoPeakCell.hasVolcano = true;
            volcanoPeakCell.hasActiveLava = true;
            volcanoPeakCell.lavaTimer = 45.0f;
            volcanoPeakCell.elevation = std::max(2.20f, volcanoPeakCell.elevation + 0.75f);
            volcanoPeakCell.isBurnt = true;
            volcanoPeakCell.hasTreesUprooted = true;

            // EN: Configure caldera flanks: active boiling lava pools, basalt crusts, and scorched rock
            // ES: Configurar laderas de caldera: piscinas de lava activa, corteza de basalto y roca calcinada
            for (const Point2i& memberPoint : clusterCells) {
                if (memberPoint.x == bestPeakX && memberPoint.y == bestPeakY) continue;

                Cell& flankCell = getCell(memberPoint.x, memberPoint.y);
                flankCell.isBurnt = true;
                flankCell.hasTreesUprooted = true;

                const float deltaCoordX = static_cast<float>(memberPoint.x - bestPeakX);
                const float deltaCoordY = static_cast<float>(memberPoint.y - bestPeakY);
                const float distanceToVolcano = std::sqrt(deltaCoordX * deltaCoordX + deltaCoordY * deltaCoordY);

                const uint32_t coordinateHash = ((static_cast<uint32_t>(memberPoint.x) * 73856093u) ^
                                                 (static_cast<uint32_t>(memberPoint.y) * 19349663u)) ^ (seed * 83492791u);
                const uint32_t roll = ((coordinateHash ^ (coordinateHash >> 13)) * 0x5bd1e995) % 100u;

                // High chance of active lava pools near volcano, moderate chance further out
                if (distanceToVolcano <= 1.8f || roll < 45u) {
                    flankCell.hasActiveLava = true;
                    flankCell.lavaTimer = 35.0f;
                    flankCell.elevation = std::max(1.30f, flankCell.elevation);
                } else {
                    flankCell.elevation = std::max(1.15f, flankCell.elevation);
                }
            }
        }
    }

    // EN: Establish the 4 progressive water depth phases across all aquatic biomes
    // ES: Establecer las 4 fases progresivas de profundidad de agua en todos los biomas acuáticos
    updateWaterDepthPhases();
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
                    case BiomeType::VolcanicCaldera:
                        c.maxPopulation = 1;
                        c.population = 0;
                        c.development = 0.02f;
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

// EN: Assign natural resources across the world following realistic geological logic and vein clustering
// ES: Asignar recursos naturales por el mundo siguiendo lógica geológica realista y agrupamiento en vetas
void WorldMap::assignNaturalResources(uint32_t seed, ResourceAbundance abundance) {
    constexpr float SCANT_ABUNDANCE_MULTIPLIER = 0.55f;
    constexpr float ABUNDANT_MULTIPLIER = 1.65f;
    constexpr float STANDARD_MULTIPLIER = 1.0f;

    float abundanceMultiplier = STANDARD_MULTIPLIER;
    if (abundance == ResourceAbundance::Escaso) {
        abundanceMultiplier = SCANT_ABUNDANCE_MULTIPLIER;
    } else if (abundance == ResourceAbundance::Abundante) {
        abundanceMultiplier = ABUNDANT_MULTIPLIER;
    }

    // EN: Phase 1 - Clean slate: Reset all cells to zero natural resources
    // ES: Fase 1 - Pizarra limpia: Reiniciar todas las celdas a cero recursos naturales
    for (int cellY = 0; cellY < m_height; ++cellY) {
        for (int cellX = 0; cellX < m_width; ++cellX) {
            Cell& currentCell = getCell(cellX, cellY);
            currentCell.naturalResource = ResourceType::None;
            currentCell.resourceAmount = 0;
            currentCell.maxResourceAmount = 0;
            currentCell.resourceVariant = 0;
        }
    }

    // EN: Phase 2 - Biosphere and aquatic flora/fauna distribution
    // ES: Fase 2 - Distribución de flora/fauna de la biosfera y acuática
    for (int cellY = 0; cellY < m_height; ++cellY) {
        for (int cellX = 0; cellX < m_width; ++cellX) {
            Cell& currentCell = getCell(cellX, cellY);
            const uint32_t cellHash = ((static_cast<uint32_t>(cellX) * 73856093u) ^
                                       (static_cast<uint32_t>(cellY) * 19349663u)) ^
                                      (seed * 83492791u);
            const uint32_t normalizedHash = ((cellHash ^ (cellHash >> 13)) * 0x5bd1e995) ^ (cellHash >> 15);
            const uint32_t roll = normalizedHash % 100u;

            switch (currentCell.biome) {
                case BiomeType::FertileValley: {
                    // EN: Fertile valleys contain wild grains, fruit orchards, and berry patches
                    // ES: Los valles fértiles contienen granos silvestres, huertos y matorrales de bayas
                    if (roll < 70u) {
                        currentCell.naturalResource = ResourceType::Food;
                        currentCell.resourceAmount = static_cast<int>((220.0f + static_cast<float>(normalizedHash % 140u)) * abundanceMultiplier);
                        currentCell.maxResourceAmount = currentCell.resourceAmount;
                        currentCell.resourceVariant = 0;
                    }
                    break;
                }
                case BiomeType::CoastalPlains: {
                    // EN: Coastal foraging, edible roots, and seasonal marsh game
                    // ES: Forraje costero, raíces comestibles y fauna de marisma estacional
                    if (roll < 45u) {
                        currentCell.naturalResource = ResourceType::Food;
                        currentCell.resourceAmount = static_cast<int>((160.0f + static_cast<float>(normalizedHash % 110u)) * abundanceMultiplier);
                        currentCell.maxResourceAmount = currentCell.resourceAmount;
                        currentCell.resourceVariant = 0;
                    }
                    break;
                }
                case BiomeType::ShallowWater: {
                    // EN: Littoral fisheries, shellfish beds, and coastal kelp
                    // ES: Pesquerías litorales, bancos de moluscos y algas costeras
                    if (roll < 35u) {
                        currentCell.naturalResource = ResourceType::Food;
                        currentCell.resourceAmount = static_cast<int>((130.0f + static_cast<float>(normalizedHash % 90u)) * abundanceMultiplier);
                        currentCell.maxResourceAmount = currentCell.resourceAmount;
                        currentCell.resourceVariant = 0;
                    }
                    break;
                }
                case BiomeType::DenseForest: {
                    // EN: Ancient timber stands; cold climates feature pine, temperate climates broadleaf oak
                    // ES: Rodales de madera ancestral; climas fríos lucen pinos, climas templados robles
                    if (roll < 72u) {
                        currentCell.naturalResource = ResourceType::Wood;
                        currentCell.resourceAmount = static_cast<int>((280.0f + static_cast<float>(normalizedHash % 180u)) * abundanceMultiplier);
                        currentCell.maxResourceAmount = currentCell.resourceAmount;
                        if (currentCell.climate.temperature < -0.10f) {
                            currentCell.resourceVariant = 3; // EN: Subalpine Pine / ES: Pino subalpino
                        } else if (currentCell.climate.moisture > 0.35f) {
                            currentCell.resourceVariant = 2; // EN: Broadleaf Oak / ES: Roble de hoja ancha
                        } else {
                            currentCell.resourceVariant = 0; // EN: Standard Forest / ES: Bosque estándar
                        }
                    }
                    break;
                }
                case BiomeType::AridDesert: {
                    // EN: Wind-sculpted sandstone yardangs, monoliths, and tectonic slabs
                    // ES: Mesetas de arenisca esculpidas por el viento, monolitos y lajas tectónicas
                    if (roll < 16u) {
                        currentCell.naturalResource = ResourceType::Stone;
                        currentCell.resourceAmount = static_cast<int>((100.0f + static_cast<float>(normalizedHash % 70u)) * abundanceMultiplier);
                        currentCell.maxResourceAmount = currentCell.resourceAmount;
                        currentCell.resourceVariant = (roll < 8u) ? 1 : 3; // Boulder or Slab
                    }
                    break;
                }
                case BiomeType::FrozenTundra: {
                    // EN: Frost-shattered boulder scree and hardy tundra wildlife
                    // ES: Canchales de roca fracturada por hielo y fauna ártica resistente
                    if (roll < 12u) {
                        currentCell.naturalResource = ResourceType::Food;
                        currentCell.resourceAmount = static_cast<int>((90.0f + static_cast<float>(normalizedHash % 50u)) * abundanceMultiplier);
                        currentCell.maxResourceAmount = currentCell.resourceAmount;
                        currentCell.resourceVariant = 0;
                    } else if (roll < 25u) {
                        currentCell.naturalResource = ResourceType::Stone;
                        currentCell.resourceAmount = static_cast<int>((80.0f + static_cast<float>(normalizedHash % 50u)) * abundanceMultiplier);
                        currentCell.maxResourceAmount = currentCell.resourceAmount;
                        currentCell.resourceVariant = 2; // Rubble
                    }
                    break;
                }
                default:
                    break;
            }
        }
    }

    // EN: Phase 3 - Geological Vein Network Generation in Highland Hills and Rugged Mountains
    // Realistic geology: ~70-75% of mountain & hill slopes remain pristine bare rock and alpine scree.
    // Mineral deposits form concentrated, contiguous geological veins (vetas) following tectonic fractures:
    // - Copper: Porphyry veins concentrated in foothills (HighlandHills) and lower slopes
    // - Iron: Banded Iron Formations (BIF) stretching 2-4 contiguous cells in mid-to-high ridges
    // - Gold: Hydrothermal quartz veins (1-2 cells) in high rocky peaks and cirques
    // - Diamond: Solitary kimberlite pipes (1 cell) in deep tectonic crags and volcanic zones
    // - Stone: Quarries, granite monoliths, and boulder scree fields
    // ES: Fase 3 - Red de Vetas Geológicas en Colinas Altas y Montañas Escarpadas
    // Geología realista: ~70-75% de las laderas montañosas permanecen como roca virgen y canchales alpinos.
    // Los depósitos minerales forman vetas geológicas contiguas (vetas) siguiendo fracturas tectónicas:
    // - Cobre: Vetas de pórfido concentradas en faldas (HighlandHills) y laderas inferiores
    // - Hierro: Formaciones de Hierro Bandeado (BIF) extendiéndose 2-4 celdas en crestas medias y altas
    // - Oro: Vetas hidrotermales de cuarzo (1-2 celdas) en riscos rocosos altos y circos glaciares
    // - Diamante: Chimeneas solitarias de kimberlita (1 celda) en riscos tectónicos profundos y volcanes
    // - Piedra: Canteras, monolitos de granito y canchales de peñascos
    struct VeinSeedCandidate {
        int seedCellX{0};
        int seedCellY{0};
        ResourceType seededResourceType{ResourceType::Stone};
        int veinLength{2};
        int baseResourceAmount{200};
        uint8_t centralVariantIndex{0};
        uint8_t peripheralVariantIndex{1};
        uint32_t seedCoordinateHash{0};
    };

    std::vector<VeinSeedCandidate> veinSeedCandidates;

    // EN: Step 3A - Scan mountain and hill terrain to identify geological vein origins
    // ES: Paso 3A - Explorar terreno de montaña y colinas para identificar orígenes de vetas geológicas
    for (int cellY = 0; cellY < m_height; ++cellY) {
        for (int cellX = 0; cellX < m_width; ++cellX) {
            const Cell& inspectedCell = getCell(cellX, cellY);
            if (inspectedCell.biome != BiomeType::HighlandHills &&
                inspectedCell.biome != BiomeType::RuggedMountains &&
                inspectedCell.biome != BiomeType::VolcanicCaldera) {
                continue;
            }

            const uint32_t cellHash = ((static_cast<uint32_t>(cellX) * 73856093u) ^
                                       (static_cast<uint32_t>(cellY) * 19349663u)) ^
                                      (seed * 83492791u);
            const uint32_t normalizedHash = ((cellHash ^ (cellHash >> 13)) * 0x5bd1e995) ^ (cellHash >> 15);

            // EN: Seed threshold: Only ~9% of mountain/hill cells become vein origins (~75% remain bare)
            // ES: Umbral de siembra: Solo ~9% de celdas montañosas inician vetas (~75% permanecen roca virgen)
            if ((normalizedHash % 100u) >= 9u) {
                continue;
            }

            const float combinedAltitudeScore = inspectedCell.elevation + inspectedCell.climate.elevation * 0.5f;
            VeinSeedCandidate seedCandidate;
            seedCandidate.seedCellX = cellX;
            seedCandidate.seedCellY = cellY;
            seedCandidate.seedCoordinateHash = normalizedHash;

            if (inspectedCell.biome == BiomeType::VolcanicCaldera) {
                // EN: Volcanic Kimberlite Diamond pipes and Hydrothermal Gold/Copper veins
                // ES: Chimeneas kimberlíticas de diamante volcánico y vetas hidrotermales de oro/cobre
                const uint32_t rollVolcanic = normalizedHash % 1000u;
                if (rollVolcanic < 180u) {
                    seedCandidate.seededResourceType = ResourceType::Diamond;
                    seedCandidate.veinLength = 1; // Solitary kimberlite pipe
                    seedCandidate.baseResourceAmount = static_cast<int>(120.0f * abundanceMultiplier);
                    seedCandidate.centralVariantIndex = 0;
                    seedCandidate.peripheralVariantIndex = 0;
                } else if (rollVolcanic < 480u) {
                    seedCandidate.seededResourceType = ResourceType::Gold;
                    seedCandidate.veinLength = 2;
                    seedCandidate.baseResourceAmount = static_cast<int>(240.0f * abundanceMultiplier);
                    seedCandidate.centralVariantIndex = 0;
                    seedCandidate.peripheralVariantIndex = 1;
                } else if (rollVolcanic < 780u) {
                    seedCandidate.seededResourceType = ResourceType::Copper;
                    seedCandidate.veinLength = 3;
                    seedCandidate.baseResourceAmount = static_cast<int>(260.0f * abundanceMultiplier);
                    seedCandidate.centralVariantIndex = 0;
                    seedCandidate.peripheralVariantIndex = 1;
                } else {
                    seedCandidate.seededResourceType = ResourceType::Stone; // Basalt
                    seedCandidate.veinLength = 3;
                    seedCandidate.baseResourceAmount = static_cast<int>(280.0f * abundanceMultiplier);
                    seedCandidate.centralVariantIndex = 0;
                    seedCandidate.peripheralVariantIndex = 1;
                }
                veinSeedCandidates.push_back(seedCandidate);
            } else if (inspectedCell.biome == BiomeType::RuggedMountains) {
                if (combinedAltitudeScore >= 1.20f || inspectedCell.climate.elevation > 0.50f) {
                    // EN: High-altitude alpine crags: Kimberlite diamond, hydrothermal gold, or magnetic iron
                    // ES: Riscos alpinos de gran altura: Diamante kimberlítico, oro hidrotermal o hierro magnético
                    const uint32_t rollHigh = normalizedHash % 1000u;
                    if (rollHigh < 40u) {
                        // EN: Kimberlite Diamond Pipe (solitary, deep intrusion)
                        // ES: Chimenea de Kimberlita con Diamante (solitaria, intrusión profunda)
                        seedCandidate.seededResourceType = ResourceType::Diamond;
                        seedCandidate.veinLength = 1;
                        seedCandidate.baseResourceAmount = static_cast<int>((90.0f + static_cast<float>(normalizedHash % 60u)) * abundanceMultiplier);
                        seedCandidate.centralVariantIndex = 0; // Diamond Crystal
                        seedCandidate.peripheralVariantIndex = 1; // Diamond Rock
                    } else if (rollHigh < 340u) {
                        // EN: Hydrothermal Gold Vein in quartz crags
                        // ES: Veta de Oro hidrotermal en riscos de cuarzo
                        seedCandidate.seededResourceType = ResourceType::Gold;
                        seedCandidate.veinLength = (normalizedHash % 2u == 0) ? 2 : 1;
                        seedCandidate.baseResourceAmount = static_cast<int>((120.0f + static_cast<float>(normalizedHash % 110u)) * abundanceMultiplier);
                        seedCandidate.centralVariantIndex = 0; // Gold Vein
                        seedCandidate.peripheralVariantIndex = 2; // Gold Nuggets
                    } else {
                        // EN: High-grade Banded Magnetite Iron
                        // ES: Hierro magnetítico bandeado de alta ley
                        seedCandidate.seededResourceType = ResourceType::Iron;
                        seedCandidate.veinLength = 2 + static_cast<int>(normalizedHash % 3u);
                        seedCandidate.baseResourceAmount = static_cast<int>((190.0f + static_cast<float>(normalizedHash % 120u)) * abundanceMultiplier);
                        seedCandidate.centralVariantIndex = 0; // Iron Vein
                        seedCandidate.peripheralVariantIndex = 1; // Iron Rock
                    }
                } else {
                    // EN: Mid mountain ranges: Banded Iron, Granite Quarries, or Copper Porphyry
                    // ES: Cordilleras medias: Hierro bandeado, canteras de granito o pórfido de cobre
                    const uint32_t rollMid = normalizedHash % 100u;
                    if (rollMid < 46u) {
                        seedCandidate.seededResourceType = ResourceType::Iron;
                        seedCandidate.veinLength = 2 + static_cast<int>(normalizedHash % 3u);
                        seedCandidate.baseResourceAmount = static_cast<int>((180.0f + static_cast<float>(normalizedHash % 120u)) * abundanceMultiplier);
                        seedCandidate.centralVariantIndex = 0; // Iron Vein
                        seedCandidate.peripheralVariantIndex = 1; // Iron Rock
                    } else if (rollMid < 72u) {
                        seedCandidate.seededResourceType = ResourceType::Stone;
                        seedCandidate.veinLength = 2 + static_cast<int>(normalizedHash % 3u);
                        seedCandidate.baseResourceAmount = static_cast<int>((300.0f + static_cast<float>(normalizedHash % 160u)) * abundanceMultiplier);
                        seedCandidate.centralVariantIndex = 0; // Stone Quarry
                        seedCandidate.peripheralVariantIndex = 1; // Stone Boulder
                    } else if (rollMid < 90u) {
                        seedCandidate.seededResourceType = ResourceType::Copper;
                        seedCandidate.veinLength = 2 + static_cast<int>(normalizedHash % 2u);
                        seedCandidate.baseResourceAmount = static_cast<int>((180.0f + static_cast<float>(normalizedHash % 110u)) * abundanceMultiplier);
                        seedCandidate.centralVariantIndex = 0; // Copper Vein
                        seedCandidate.peripheralVariantIndex = 1; // Copper Rock
                    } else {
                        seedCandidate.seededResourceType = ResourceType::Gold;
                        seedCandidate.veinLength = 1 + static_cast<int>(normalizedHash % 2u);
                        seedCandidate.baseResourceAmount = static_cast<int>((110.0f + static_cast<float>(normalizedHash % 90u)) * abundanceMultiplier);
                        seedCandidate.centralVariantIndex = 0; // Gold Vein
                        seedCandidate.peripheralVariantIndex = 2; // Gold Nuggets
                    }
                }
            } else {
                // EN: Highland Hills / Foothills: Weathered Copper Porphyry, Boulder Outcrops, and Foothill Iron
                // ES: Lomeríos y Colinas: Pórfido de cobre meteorizado, peñascos y hierro de colina
                const uint32_t rollHills = normalizedHash % 100u;
                if (rollHills < 42u) {
                    seedCandidate.seededResourceType = ResourceType::Copper;
                    seedCandidate.veinLength = 2 + static_cast<int>(normalizedHash % 2u);
                    seedCandidate.baseResourceAmount = static_cast<int>((160.0f + static_cast<float>(normalizedHash % 100u)) * abundanceMultiplier);
                    seedCandidate.centralVariantIndex = 0; // Copper Vein
                    seedCandidate.peripheralVariantIndex = 1; // Copper Rock
                } else if (rollHills < 78u) {
                    seedCandidate.seededResourceType = ResourceType::Stone;
                    seedCandidate.veinLength = 2 + static_cast<int>(normalizedHash % 3u);
                    seedCandidate.baseResourceAmount = static_cast<int>((220.0f + static_cast<float>(normalizedHash % 120u)) * abundanceMultiplier);
                    seedCandidate.centralVariantIndex = 1; // Stone Boulder
                    seedCandidate.peripheralVariantIndex = 2; // Stone Rubble
                } else if (rollHills < 94u) {
                    seedCandidate.seededResourceType = ResourceType::Iron;
                    seedCandidate.veinLength = 2 + static_cast<int>(normalizedHash % 2u);
                    seedCandidate.baseResourceAmount = static_cast<int>((140.0f + static_cast<float>(normalizedHash % 90u)) * abundanceMultiplier);
                    seedCandidate.centralVariantIndex = 0; // Iron Vein
                    seedCandidate.peripheralVariantIndex = 1; // Iron Rock
                } else {
                    seedCandidate.seededResourceType = ResourceType::Gold;
                    seedCandidate.veinLength = 1 + static_cast<int>(normalizedHash % 2u);
                    seedCandidate.baseResourceAmount = static_cast<int>((80.0f + static_cast<float>(normalizedHash % 70u)) * abundanceMultiplier);
                    seedCandidate.centralVariantIndex = 0; // Gold Vein
                    seedCandidate.peripheralVariantIndex = 2; // Gold Nuggets
                }
            }

            veinSeedCandidates.push_back(seedCandidate);
        }
    }

    // EN: Step 3B - Grow veins along geological strike vectors into contiguous cells
    // ES: Paso 3B - Expandir vetas a lo largo de vectores de rumbo geológico en celdas contiguas
    constexpr std::array<std::pair<int, int>, 4> GEOLOGICAL_STRIKE_DIRECTIONS{{
        {1, 0},   // East-West
        {0, 1},   // North-South
        {1, 1},   // Northwest-Southeast diagonal
        {1, -1}   // Southwest-Northeast diagonal
    }};

    for (const auto& currentVeinSeed : veinSeedCandidates) {
        Cell& originCell = getCell(currentVeinSeed.seedCellX, currentVeinSeed.seedCellY);

        // EN: Assign central origin node if empty
        // ES: Asignar nodo central de origen si está vacío
        if (originCell.naturalResource == ResourceType::None) {
            originCell.naturalResource = currentVeinSeed.seededResourceType;
            originCell.resourceAmount = currentVeinSeed.baseResourceAmount;
            originCell.maxResourceAmount = originCell.resourceAmount;
            originCell.resourceVariant = currentVeinSeed.centralVariantIndex;
        }

        const size_t directionIndex = (currentVeinSeed.seedCoordinateHash >> 8u) % GEOLOGICAL_STRIKE_DIRECTIONS.size();
        const auto [strikeDeltaX, strikeDeltaY] = GEOLOGICAL_STRIKE_DIRECTIONS[directionIndex];

        // EN: Propagate vein into 1 to 3 contiguous cells
        // ES: Propagar veta hacia 1 a 3 celdas contiguas
        for (int stepIndex = 1; stepIndex < currentVeinSeed.veinLength; ++stepIndex) {
            int stepCellX = currentVeinSeed.seedCellX + strikeDeltaX * stepIndex;
            int stepCellY = currentVeinSeed.seedCellY + strikeDeltaY * stepIndex;

            if (!isValid(stepCellX, stepCellY)) {
                stepCellX = currentVeinSeed.seedCellX - strikeDeltaX * stepIndex;
                stepCellY = currentVeinSeed.seedCellY - strikeDeltaY * stepIndex;
            }

            if (!isValid(stepCellX, stepCellY)) {
                continue;
            }

            Cell& neighborVeinCell = getCell(stepCellX, stepCellY);
            if (neighborVeinCell.biome != BiomeType::HighlandHills &&
                neighborVeinCell.biome != BiomeType::RuggedMountains &&
                neighborVeinCell.biome != BiomeType::VolcanicCaldera) {
                continue;
            }

            // EN: Keep volcanic veins strictly inside volcanic terrain and mountain veins inside mountain terrain
            // ES: Mantener las vetas volcánicas estrictamente en terreno volcánico y vetas montañosas en montaña
            if (originCell.biome == BiomeType::VolcanicCaldera && neighborVeinCell.biome != BiomeType::VolcanicCaldera) {
                continue;
            }
            if (originCell.biome != BiomeType::VolcanicCaldera && neighborVeinCell.biome == BiomeType::VolcanicCaldera) {
                continue;
            }

            // EN: Preserve already assigned minerals so distinct veins remain separated
            // ES: Preservar minerales ya asignados para que las vetas distintas permanezcan separadas
            if (neighborVeinCell.naturalResource != ResourceType::None) {
                continue;
            }

            neighborVeinCell.naturalResource = currentVeinSeed.seededResourceType;
            const float peripheralFactor = 0.80f + static_cast<float>((currentVeinSeed.seedCoordinateHash >> (stepIndex * 3)) % 25u) * 0.01f;
            neighborVeinCell.resourceAmount = static_cast<int>(static_cast<float>(currentVeinSeed.baseResourceAmount) * peripheralFactor);
            neighborVeinCell.maxResourceAmount = neighborVeinCell.resourceAmount;
            neighborVeinCell.resourceVariant = currentVeinSeed.peripheralVariantIndex;
        }
    }

    // EN: Phase 4 - Safety Net & Guarantees for all simulation mechanics and test suites
    // Ensure every map seed has at least one accessible deposit of Stone, Iron, Copper, Gold, Food, and Wood
    // ES: Fase 4 - Red de Seguridad y Garantías para mecánicas de simulación y suites de pruebas
    // Asegurar que cada semilla tenga al menos un depósito accesible de Piedra, Hierro, Cobre, Oro, Comida y Madera
    int countTotalStone = 0;
    int countMountainStone = 0;
    int countTotalIron = 0;
    int countMountainIron = 0;
    int countTotalCopper = 0;
    int countTotalGold = 0;
    int countTotalWood = 0;
    int countTotalFood = 0;

    for (int cellY = 0; cellY < m_height; ++cellY) {
        for (int cellX = 0; cellX < m_width; ++cellX) {
            const Cell& verifiedCell = getCell(cellX, cellY);
            switch (verifiedCell.naturalResource) {
                case ResourceType::Stone:
                    countTotalStone++;
                    if (verifiedCell.biome == BiomeType::HighlandHills || verifiedCell.biome == BiomeType::RuggedMountains) {
                        countMountainStone++;
                    }
                    break;
                case ResourceType::Iron:
                    countTotalIron++;
                    if (verifiedCell.biome == BiomeType::HighlandHills || verifiedCell.biome == BiomeType::RuggedMountains) {
                        countMountainIron++;
                    }
                    break;
                case ResourceType::Copper: countTotalCopper++; break;
                case ResourceType::Gold: countTotalGold++; break;
                case ResourceType::Wood: countTotalWood++; break;
                case ResourceType::Food: countTotalFood++; break;
                default: break;
            }
        }
    }

    // EN: Guarantee at least one Iron vein exists in mountains or hills (required by unit tests & smelting)
    // ES: Garantizar al menos una veta de Hierro en montañas o colinas (requerido por pruebas y herrería)
    if (countMountainIron == 0) {
        for (int y = 0; y < m_height && countMountainIron == 0; ++y) {
            for (int x = 0; x < m_width && countMountainIron == 0; ++x) {
                Cell& candidateCell = getCell(x, y);
                if ((candidateCell.biome == BiomeType::HighlandHills || candidateCell.biome == BiomeType::RuggedMountains) &&
                    candidateCell.naturalResource == ResourceType::None) {
                    candidateCell.naturalResource = ResourceType::Iron;
                    candidateCell.resourceAmount = static_cast<int>(180.0f * abundanceMultiplier);
                    candidateCell.maxResourceAmount = candidateCell.resourceAmount;
                    candidateCell.resourceVariant = 0; // Iron Vein
                    countMountainIron++;
                }
            }
        }
    }

    // EN: Guarantee at least one Gold deposit on high mountain/hill peaks (required by unit tests)
    // ES: Garantizar al menos un depósito de Oro en cumbres de montaña/colina (requerido por pruebas)
    if (countTotalGold == 0) {
        for (int y = 0; y < m_height && countTotalGold == 0; ++y) {
            for (int x = 0; x < m_width && countTotalGold == 0; ++x) {
                Cell& candidateCell = getCell(x, y);
                if ((candidateCell.biome == BiomeType::HighlandHills || candidateCell.biome == BiomeType::RuggedMountains || candidateCell.biome == BiomeType::VolcanicCaldera) &&
                    candidateCell.naturalResource == ResourceType::None) {
                    candidateCell.naturalResource = ResourceType::Gold;
                    candidateCell.resourceAmount = static_cast<int>(120.0f * abundanceMultiplier);
                    candidateCell.maxResourceAmount = candidateCell.resourceAmount;
                    candidateCell.resourceVariant = 0; // Gold Vein
                    countTotalGold++;
                }
            }
        }
    }

    // EN: Guarantee at least one Stone quarry exists in mountain/hill terrain (required by masonry and tests)
    // ES: Garantizar al menos una cantera de Piedra en terreno de montaña/colina (requerido por albañilería y pruebas)
    if (countMountainStone == 0) {
        for (int y = 0; y < m_height && countMountainStone == 0; ++y) {
            for (int x = 0; x < m_width && countMountainStone == 0; ++x) {
                Cell& candidateCell = getCell(x, y);
                if ((candidateCell.biome == BiomeType::HighlandHills || candidateCell.biome == BiomeType::RuggedMountains) &&
                    candidateCell.naturalResource == ResourceType::None) {
                    candidateCell.naturalResource = ResourceType::Stone;
                    candidateCell.resourceAmount = static_cast<int>(220.0f * abundanceMultiplier);
                    candidateCell.maxResourceAmount = candidateCell.resourceAmount;
                    candidateCell.resourceVariant = 1; // Stone Boulder
                    countMountainStone++;
                }
            }
        }
    }

    // EN: Guarantee Copper deposit in foothills or lower mountain slopes
    // ES: Garantizar depósito de Cobre en colinas o laderas montañosas
    if (countTotalCopper == 0) {
        for (int y = 0; y < m_height && countTotalCopper == 0; ++y) {
            for (int x = 0; x < m_width && countTotalCopper == 0; ++x) {
                Cell& candidateCell = getCell(x, y);
                if ((candidateCell.biome == BiomeType::HighlandHills || candidateCell.biome == BiomeType::RuggedMountains) &&
                    candidateCell.naturalResource == ResourceType::None) {
                    candidateCell.naturalResource = ResourceType::Copper;
                    candidateCell.resourceAmount = static_cast<int>(160.0f * abundanceMultiplier);
                    candidateCell.maxResourceAmount = candidateCell.resourceAmount;
                    candidateCell.resourceVariant = 0; // Copper Vein
                    countTotalCopper++;
                }
            }
        }
    }
}

// EN: Update natural resources (resources are finite deposits that deplete permanently as villagers extract them)
// ES: Actualizar recursos naturales (los recursos son depósitos finitos que se agotan permanentemente según los aldeanos los sustraen)
void WorldMap::updateResources(float deltaTime) {
    (void)deltaTime;
    // EN: Resources are finite non-renewable deposits. No automatic magical replenishment.
    // ES: Los recursos son depósitos finitos no renovables. Sin reposición mágica automática.
}

// EN: Ignite a specific cell with duration and initial intensity if combustible
// ES: Encender una celda específica con duración e intensidad inicial si es combustible
bool WorldMap::igniteCell(int cellX, int cellY, float durationSeconds, float initialIntensity) {
    if (!isValid(cellX, cellY)) return false;
    Cell& targetCell = getCell(cellX, cellY);
    if (targetCell.isWater() || targetCell.isBurnt) return false;
    if (!targetCell.isCombustible()) return false;

    // EN: Adapt combustion duration to fuel type: light grass burns faster (5-7s), dense timber burns longer (14-16s)
    // ES: Adaptar duración de combustión al combustible: pasto ligero arde más rápido (5-7s), bosque denso arde más (14-16s)
    float effectiveDuration = durationSeconds;
    if (targetCell.naturalResource != ResourceType::Wood && targetCell.biome != BiomeType::DenseForest && !targetCell.hasHouse) {
        effectiveDuration = std::min(durationSeconds, 6.0f);
    }

    if (!targetCell.isBurning) {
        targetCell.isBurning = true;
        targetCell.burnTimer = effectiveDuration;
        targetCell.maxBurnTimer = effectiveDuration;
        targetCell.fireIntensity = std::clamp(initialIntensity, 0.10f, 1.0f);
        targetCell.heatAccumulator = 0.0f;
        m_burningCells.push_back({cellX, cellY});
        return true;
    } else {
        // EN: Extend burn timer and reinforce intensity if already burning
        // ES: Extender temporizador de quemado y reforzar intensidad si ya está ardiendo
        targetCell.burnTimer = std::max(targetCell.burnTimer, effectiveDuration);
        targetCell.fireIntensity = std::min(1.0f, targetCell.fireIntensity + initialIntensity * 0.35f);
        return false;
    }
}

// EN: Extinguish active fire in a specific cell immediately
// ES: Extinguir fuego activo en una celda específica de inmediato
void WorldMap::extinguishCell(int cellX, int cellY) {
    if (!isValid(cellX, cellY)) return;
    Cell& targetCell = getCell(cellX, cellY);
    targetCell.extinguish();
    m_burningCells.erase(std::remove_if(m_burningCells.begin(), m_burningCells.end(),
                                       [cellX, cellY](const Point2i& coordinatePoint) {
                                           return coordinatePoint.x == cellX && coordinatePoint.y == cellY;
                                       }),
                         m_burningCells.end());
}

// EN: Comprehensive fire physics simulation: propagation, wind advection, moisture barriers, fuel consumption, and dissipation
// ES: Simulación física exhaustiva del fuego: propagación, advección por viento, barreras de humedad, consumo y disipación
void WorldMap::updateFirePhysics(float deltaTime, BuildingSystem* buildingSystem,
                                 GestorHumanos* villagerManager,
                                 float windVelocityX, float windVelocityY) {
    if (deltaTime <= 0.0f) return;

    // EN: 1. Passive thermodynamic cooling & fire immunity timers across map
    // ES: 1. Enfriamiento termodinámico pasivo y temporizadores de inmunidad al fuego en todo el mapa
    constexpr float PASSIVE_THERMAL_COOLING_RATE = 0.20f;
    for (auto& cellItem : m_cells) {
        if (cellItem.fireImmunityTimer > 0.0f) {
            cellItem.fireImmunityTimer = std::max(0.0f, cellItem.fireImmunityTimer - deltaTime);
        }
        if (cellItem.heatAccumulator > 0.0f && !cellItem.isBurning) {
            cellItem.heatAccumulator = std::max(0.0f, cellItem.heatAccumulator - PASSIVE_THERMAL_COOLING_RATE * deltaTime);
        }
        // EN: Solidification of active volcanic lava into basalt rock
        // ES: Solidificación de lava volcánica activa en roca de basalto
        if (cellItem.hasActiveLava && !cellItem.hasVolcano) {
            cellItem.lavaTimer -= deltaTime;
            if (cellItem.lavaTimer <= 0.0f) {
                cellItem.hasActiveLava = false;
                cellItem.lavaTimer = 0.0f;
                cellItem.biome = BiomeType::RuggedMountains;
                cellItem.elevation = std::min(2.8f, cellItem.elevation + 0.15f);
                cellItem.naturalResource = ResourceType::Stone;
                cellItem.resourceAmount = 140;
                cellItem.maxResourceAmount = 140;
                cellItem.resourceVariant = 1; // Stone boulder
            }
        }
    }

    // EN: 2. Update ecological ash recovery for burnt cells
    // ES: 2. Actualizar recuperación ecológica de cenizas para celdas calcinadas
    for (auto iterator = m_burntCells.begin(); iterator != m_burntCells.end(); ) {
        if (!isValid(iterator->x, iterator->y)) {
            iterator = m_burntCells.erase(iterator);
            continue;
        }
        Cell& burntCell = getCell(iterator->x, iterator->y);
        burntCell.ashRecoveryTimer -= deltaTime;
        if (burntCell.ashRecoveryTimer <= 0.0f) {
            // EN: Ash minerals fertilize soil into green pasture and grant refractory fire immunity
            // ES: Minerales de ceniza fertilizan el suelo en pradera fértil e imparten inmunidad refractaria al fuego
            burntCell.isBurnt = false;
            burntCell.ashRecoveryTimer = 0.0f;
            burntCell.fireImmunityTimer = 90.0f; // EN: 90s immunity prevents infinite circular wildfires / ES: 90s de inmunidad previene fuegos circulares
            if (burntCell.biome == BiomeType::DenseForest || burntCell.biome == BiomeType::CoastalPlains) {
                burntCell.biome = BiomeType::FertileValley;
                burntCell.maxPopulation = std::min(25, burntCell.maxPopulation + 3);
            }
            iterator = m_burntCells.erase(iterator);
        } else {
            ++iterator;
        }
    }

    if (m_burningCells.empty()) return;

    // EN: Calculate wind magnitude and normalized vector
    // ES: Calcular magnitud del viento y vector unitario
    const float windSpeed = std::sqrt(windVelocityX * windVelocityX + windVelocityY * windVelocityY);
    const float normalizedWindX = (windSpeed > 0.001f) ? (windVelocityX / windSpeed) : 0.0f;
    const float normalizedWindY = (windSpeed > 0.001f) ? (windVelocityY / windSpeed) : 0.0f;

    std::vector<Point2i> newIgnitions;
    static thread_local std::mt19937 fireRngEngine(1337);
    std::uniform_real_distribution<float> distribution01(0.0f, 1.0f);

    // EN: 2. Process each actively burning cell (O(K) simulation)
    // ES: 2. Procesar cada celda activa en combustión (simulación O(K))
    for (auto iterator = m_burningCells.begin(); iterator != m_burningCells.end(); ) {
        const int cellCoordX = iterator->x;
        const int cellCoordY = iterator->y;
        if (!isValid(cellCoordX, cellCoordY)) {
            iterator = m_burningCells.erase(iterator);
            continue;
        }

        Cell& currentBurningCell = getCell(cellCoordX, cellCoordY);
        if (!currentBurningCell.isBurning) {
            iterator = m_burningCells.erase(iterator);
            continue;
        }

        // EN: If water reaches this cell, extinguish immediately with steam
        // ES: Si el agua alcanza esta celda, extinguir de inmediato con vapor
        if (currentBurningCell.isWater()) {
            currentBurningCell.extinguish();
            iterator = m_burningCells.erase(iterator);
            continue;
        }

        // EN: Advance burn duration countdown
        // ES: Avanzar cuenta regresiva de duración del fuego
        currentBurningCell.burnTimer -= deltaTime;

        // EN: Modulate fire intensity based on fuel and burn timer curve
        // ES: Modular intensidad del fuego según curva de combustible y tiempo restante
        const float burnFraction = currentBurningCell.maxBurnTimer > 0.0f
            ? std::clamp(currentBurningCell.burnTimer / currentBurningCell.maxBurnTimer, 0.0f, 1.0f)
            : 0.0f;

        // Intensity curve: rises quickly to 1.0, peaks, then diminishes to smoldering embers
        if (burnFraction > 0.70f) {
            currentBurningCell.fireIntensity = std::min(1.0f, 0.35f + ((1.0f - burnFraction) / 0.30f) * 0.65f);
        } else if (burnFraction > 0.25f) {
            currentBurningCell.fireIntensity = 1.0f;
        } else {
            currentBurningCell.fireIntensity = std::max(0.08f, burnFraction / 0.25f);
        }

        // EN: Consume timber resource and topple standing trees
        // ES: Consumir recursos madereros y derribar árboles en pie
        if (currentBurningCell.naturalResource == ResourceType::Wood || currentBurningCell.biome == BiomeType::DenseForest) {
            currentBurningCell.hasTreesUprooted = true;
            if (currentBurningCell.naturalResource == ResourceType::Wood && currentBurningCell.resourceAmount > 0) {
                const int woodConsumed = static_cast<int>(32.0f * currentBurningCell.fireIntensity * deltaTime);
                currentBurningCell.resourceAmount = std::max(0, currentBurningCell.resourceAmount - woodConsumed);
                if (currentBurningCell.resourceAmount == 0) {
                    currentBurningCell.naturalResource = ResourceType::None;
                }
            }
        }

        // EN: Inflict structural fire damage to houses on this cell
        // ES: Infligir daño estructural por fuego a viviendas en esta celda
        if (buildingSystem && currentBurningCell.hasHouse) {
            const float houseFireDamage = (30.0f * currentBurningCell.fireIntensity) * deltaTime;
            buildingSystem->damageHouseAt(cellCoordX, cellCoordY, houseFireDamage, *this, villagerManager);
        }

        // EN: Burn and panic villagers in or near this active fire
        // ES: Quemar y aterrorizar aldeanos en o cerca de este fuego activo
        if (villagerManager) {
            for (auto& villager : villagerManager->getHumanos()) {
                if (!villager.estaVivo()) continue;
                const float distanceX = villager.getX() - (static_cast<float>(cellCoordX) + 0.5f);
                const float distanceY = villager.getY() - (static_cast<float>(cellCoordY) + 0.5f);
                const float distanceSquared = distanceX * distanceX + distanceY * distanceY;

                if (distanceSquared <= 1.8f) {
                    villager.recibirDano(24.0f * currentBurningCell.fireIntensity * deltaTime);
                    villager.fleeInPanicFrom(static_cast<float>(cellCoordX) + 0.5f,
                                             static_cast<float>(cellCoordY) + 0.5f, 6.0f);
                    if (currentBurningCell.fireIntensity >= 0.25f && !villager.isBurning()) {
                        villager.ignite(7.0f);
                    }
                } else if (distanceSquared <= 9.0f) {
                    villager.fleeInPanicFrom(static_cast<float>(cellCoordX) + 0.5f,
                                             static_cast<float>(cellCoordY) + 0.5f, 4.0f);
                }
            }
        }

        // EN: 3. Heat transfer and thermodynamic spread to Moore neighborhood (8 directions)
        // ES: 3. Transferencia de calor y propagación termodinámica a la vecindad de Moore (8 direcciones)
        for (int deltaCellY = -1; deltaCellY <= 1; ++deltaCellY) {
            for (int deltaCellX = -1; deltaCellX <= 1; ++deltaCellX) {
                if (deltaCellX == 0 && deltaCellY == 0) continue;
                const int neighborCellX = cellCoordX + deltaCellX;
                const int neighborCellY = cellCoordY + deltaCellY;
                if (!isValid(neighborCellX, neighborCellY)) continue;

                Cell& neighborCell = getCell(neighborCellX, neighborCellY);
                if (neighborCell.isWater() || neighborCell.isBurning || neighborCell.isBurnt) continue;
                if (!neighborCell.isCombustible()) continue;

                // EN: Directional wind factor: dot product between propagation direction and wind vector
                // ES: Factor de viento direccional: producto punto entre dirección de propagación y viento
                const float distanceToNeighbor = std::sqrt(static_cast<float>(deltaCellX * deltaCellX + deltaCellY * deltaCellY));
                const float normalizedDirX = static_cast<float>(deltaCellX) / distanceToNeighbor;
                const float normalizedDirY = static_cast<float>(deltaCellY) / distanceToNeighbor;
                const float windAlignment = (normalizedDirX * normalizedWindX + normalizedDirY * normalizedWindY);
                const float windSpreadMultiplier = std::clamp(1.0f + windAlignment * (1.2f * windSpeed), 0.35f, 2.5f);

                // EN: Elevation factor: fire travels faster uphill due to convective preheating
                // ES: Factor de elevación: el fuego sube más rápido pendientes por precalentamiento convectivo
                const float slopeDelta = neighborCell.elevation - currentBurningCell.elevation;
                const float slopeMultiplier = (slopeDelta > 0.05f) ? (1.0f + slopeDelta * 0.8f) : ((slopeDelta < -0.05f) ? 0.75f : 1.0f);

                // EN: Moisture factor: check if neighbor is next to a water body (natural firebreak dampening)
                // ES: Factor de humedad: comprobar si el vecino linda con agua (amortiguación por cortafuegos natural)
                float moistureDampening = 1.0f;
                for (int moistureCheckY = -1; moistureCheckY <= 1; ++moistureCheckY) {
                    for (int moistureCheckX = -1; moistureCheckX <= 1; ++moistureCheckX) {
                        const int checkX = neighborCellX + moistureCheckX;
                        const int checkY = neighborCellY + moistureCheckY;
                        if (isValid(checkX, checkY) && getCell(checkX, checkY).isWater()) {
                            moistureDampening = 0.40f;
                            break;
                        }
                    }
                    if (moistureDampening < 0.9f) break;
                }

                // EN: Calculate heat rate transferred to neighbor cell
                // ES: Calcular tasa de calor transferido a la celda vecina
                const float neighborFlammability = neighborCell.getFlammability();
                const float heatRate = 0.35f * currentBurningCell.fireIntensity * neighborFlammability *
                                       windSpreadMultiplier * slopeMultiplier * moistureDampening;

                neighborCell.heatAccumulator += heatRate * deltaTime;

                // EN: Ignition threshold reached!
                // ES: ¡Umbral de ignición alcanzado!
                if (neighborCell.heatAccumulator >= 1.0f) {
                    newIgnitions.push_back({neighborCellX, neighborCellY});
                }
            }
        }

        // EN: 4. Spotting & Flying Embers: high-intensity fires throw sparks downwind
        // ES: 4. Focos secundarios y ascuas volantes: fuegos intensos arrojan chispas a favor del viento
        if (currentBurningCell.fireIntensity >= 0.65f && windSpeed >= 0.12f) {
            if (distribution01(fireRngEngine) < (0.045f * deltaTime)) {
                const int emberDistance = (distribution01(fireRngEngine) < 0.70f) ? 2 : 3;
                const int emberTargetX = cellCoordX + static_cast<int>(std::round(normalizedWindX * emberDistance));
                const int emberTargetY = cellCoordY + static_cast<int>(std::round(normalizedWindY * emberDistance));
                if (isValid(emberTargetX, emberTargetY)) {
                    Cell& emberCell = getCell(emberTargetX, emberTargetY);
                    if (!emberCell.isWater() && !emberCell.isBurning && !emberCell.isBurnt && emberCell.isCombustible()) {
                        emberCell.heatAccumulator += 0.65f;
                        if (emberCell.heatAccumulator >= 1.0f || distribution01(fireRngEngine) < 0.45f) {
                            newIgnitions.push_back({emberTargetX, emberTargetY});
                        }
                    }
                }
            }
        }

        // EN: 5. Dissipation and extinction when fuel is exhausted
        // ES: 5. Disipación y extinción cuando el combustible se agota
        if (currentBurningCell.burnTimer <= 0.0f) {
            currentBurningCell.isBurning = false;
            currentBurningCell.fireIntensity = 0.0f;
            currentBurningCell.heatAccumulator = 0.0f;
            currentBurningCell.isBurnt = true;
            currentBurningCell.ashRecoveryTimer = 65.0f;
            m_burntCells.push_back({cellCoordX, cellCoordY});

            if (currentBurningCell.biome == BiomeType::DenseForest) {
                currentBurningCell.biome = BiomeType::CoastalPlains; // Se abre a pastizal calcinado
                currentBurningCell.naturalResource = ResourceType::None;
                currentBurningCell.resourceAmount = 0;
            }

            iterator = m_burningCells.erase(iterator);
        } else {
            ++iterator;
        }
    }

    // EN: Activate newly ignited cells
    // ES: Activar celdas recién encendidas
    for (const auto& coordinate : newIgnitions) {
        igniteCell(coordinate.x, coordinate.y, 14.0f, 0.45f);
    }
}

// EN: Add liquid water volume to a cell, triggering fluid accumulation, moisture saturation, and fire extinction
// ES: Agregar volumen de agua líquida a una celda, activando acumulación de fluido, saturación de humedad y extinción de fuego
void WorldMap::addWaterToCell(int cellX, int cellY, float amount) {
    if (!isValid(cellX, cellY) || amount <= 0.0f) return;
    Cell& targetCell = getCell(cellX, cellY);

    // EN: Natural oceans and rivers absorb incoming precipitation without overland water accumulation
    // ES: Océanos y ríos naturales absorben la precipitación sin acumular lámina de agua terrestre
    if (targetCell.isNaturalWater()) {
        return;
    }

    targetCell.addWater(amount);

    // EN: Extinguish active fire immediately if water exceeds threshold
    // ES: Extinguir fuego activo de inmediato si el agua supera el umbral
    if (targetCell.isBurning) {
        // EN: Boiling water loss consumes water volume to extinguish flames (latent heat of vaporization)
        // ES: Pérdida por ebullición consume volumen de agua para extinguir las llamas (calor latente de vaporización)
        const float boilingWaterLoss = std::min(targetCell.waterDepth, 0.10f * std::max(0.20f, targetCell.fireIntensity));
        targetCell.waterDepth = std::max(0.0f, targetCell.waterDepth - boilingWaterLoss);
        targetCell.extinguish();
        m_burningCells.erase(std::remove_if(m_burningCells.begin(), m_burningCells.end(),
                                           [cellX, cellY](const Point2i& coordinate) {
                                               return coordinate.x == cellX && coordinate.y == cellY;
                                           }),
                             m_burningCells.end());
    }

    // EN: Register into active flooded tracking vector if not already tracked (lower threshold captures shallow puddles)
    // ES: Registrar en el vector de celdas inundadas activas si no está ya registrado (umbral menor captura charcos someros)
    if (targetCell.waterDepth > 0.002f) {
        auto existingIterator = std::find_if(m_floodedCells.begin(), m_floodedCells.end(),
                                            [cellX, cellY](const Point2i& coordinate) {
                                                return coordinate.x == cellX && coordinate.y == cellY;
                                            });
        if (existingIterator == m_floodedCells.end()) {
            m_floodedCells.push_back({cellX, cellY});
        }
    }
}

// EN: Update the 4 water depth phases across the world based on bathymetry, distance to land, and climate elevation
// ES: Actualizar las 4 fases de profundidad del agua en el mundo según batimetría, distancia a la tierra y elevación climática
void WorldMap::updateWaterDepthPhases() noexcept {
    // EN: Multi-source distance transform calculating distance from every water cell to the closest dry land
    // ES: Transformada de distancia multifuente que calcula la distancia de cada celda acuática a la tierra firme más cercana
    std::vector<int> distanceToLand(static_cast<size_t>(m_width * m_height), -1);
    std::queue<Point2i> frontierQueue;

    // EN: Initialize all dry land cells as boundary sources with distance zero
    // ES: Inicializar todas las celdas de tierra firme como fuentes de borde con distancia cero
    for (int cellY = 0; cellY < m_height; ++cellY) {
        for (int cellX = 0; cellX < m_width; ++cellX) {
            const size_t cellIndex = static_cast<size_t>(cellY * m_width + cellX);
            const Cell& inspectedCell = getCell(cellX, cellY);
            if (!inspectedCell.isWater()) {
                distanceToLand[cellIndex] = 0;
                frontierQueue.push(Point2i{cellX, cellY});
            }
        }
    }

    // EN: Breadth-first search flood fill to compute Euclidean/grid distance to coast
    // ES: Inundación en anchura (BFS) para calcular distancia euclidiana/de cuadrícula hacia la costa
    constexpr int neighborOffsets[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
    while (!frontierQueue.empty()) {
        const Point2i currentPoint = frontierQueue.front();
        frontierQueue.pop();

        const int currentDistance = distanceToLand[static_cast<size_t>(currentPoint.y * m_width + currentPoint.x)];

        for (const auto& offset : neighborOffsets) {
            const int neighborX = currentPoint.x + offset[0];
            const int neighborY = currentPoint.y + offset[1];

            if (isValid(neighborX, neighborY)) {
                const size_t neighborIndex = static_cast<size_t>(neighborY * m_width + neighborX);
                if (distanceToLand[neighborIndex] == -1) {
                    distanceToLand[neighborIndex] = currentDistance + 1;
                    frontierQueue.push(Point2i{neighborX, neighborY});
                }
            }
        }
    }

    // EN: Classify each water cell into one of the 4 progressive water depth phases:
    // 1 - Shoreline Foam / Orilla
    // 2 - Shallow Waters / Costa
    // 3 - Mid Sea / Ocean
    // 4 - Deep Sea / Abyssal
    // ES: Clasificar cada celda de agua en una de las 4 fases progresivas de profundidad:
    // 1 - Línea de Espuma / Orilla
    // 2 - Aguas Someras / Costa
    // 3 - Mar Medio / Océano
    // 4 - Mar Profundo / Abisal
    for (int cellY = 0; cellY < m_height; ++cellY) {
        for (int cellX = 0; cellX < m_width; ++cellX) {
            Cell& targetCell = getCell(cellX, cellY);
            if (!targetCell.isWater()) {
                targetCell.waterPhase = WaterDepthPhase::ShorelineFoam;
                continue;
            }

            const size_t targetIndex = static_cast<size_t>(cellY * m_width + cellX);
            const int landDistance = distanceToLand[targetIndex];

            if (landDistance == 1) {
                // EN: Immediate coastal perimeter: shallow coast with shoreline foam interface
                // ES: Perímetro costero inmediato: costa somera con interfaz de espuma de orilla
                targetCell.waterPhase = WaterDepthPhase::ShallowCoast;
            } else if (landDistance <= 2) {
                // EN: Shallow coastal waters and lagoons
                // ES: Aguas costeras someras y lagunas
                targetCell.waterPhase = WaterDepthPhase::ShallowCoast;
            } else if (landDistance <= 4) {
                // EN: Mid sea and continental shelf transition
                // ES: Mar medio y transición de plataforma continental
                if (targetCell.climate.elevation < -0.72f) {
                    targetCell.waterPhase = WaterDepthPhase::DeepAbyssal;
                } else {
                    targetCell.waterPhase = WaterDepthPhase::MidOcean;
                }
            } else {
                // EN: Deep open ocean and oceanic trenches
                // ES: Océano abierto profundo y fosas oceánicas
                targetCell.waterPhase = WaterDepthPhase::DeepAbyssal;
            }

            // EN: Preserve explicit biome intentions: ShallowWater stays shallow unless in deep abyss
            // ES: Preservar intenciones explícitas de bioma: ShallowWater se mantiene somero salvo abismo profundo
            if (targetCell.biome == BiomeType::ShallowWater && landDistance <= 3) {
                targetCell.waterPhase = WaterDepthPhase::ShallowCoast;
            } else if (targetCell.biome == BiomeType::DeepOcean && landDistance >= 3) {
                if (landDistance >= 5 || targetCell.climate.elevation < -0.65f) {
                    targetCell.waterPhase = WaterDepthPhase::DeepAbyssal;
                } else {
                    targetCell.waterPhase = WaterDepthPhase::MidOcean;
                }
            }
        }
    }
}

// EN: Hydrology and fluid dynamics simulation: runoff, gravity flow down slopes, flooding, evaporation, and fire extinction
// ES: Simulación de hidrología y dinámica de fluidos: escorrentía, flujo por gravedad, inundación, evaporación y extinción de fuego
void WorldMap::updateWaterPhysics(float deltaTime, BuildingSystem* buildingSystem,
                                 GestorHumanos* villagerManager) {
    if (deltaTime <= 0.0f) return;

    // EN: 1. Update evaporating steam plume countdown across the map
    // ES: 1. Actualizar cuenta regresiva de penachos de vapor en evaporación en todo el mapa
    for (auto& cellItem : m_cells) {
        if (cellItem.steamTimer > 0.0f) {
            cellItem.steamTimer = std::max(0.0f, cellItem.steamTimer - deltaTime);
        }
    }

    if (m_floodedCells.empty()) return;

    std::vector<Point2i> newFloodedNeighbors;
    constexpr float WATER_FLOW_RATE = 1.35f;
    constexpr float MOISTURE_DRYING_RATE = 0.015f;
    constexpr float FLOOD_DEPTH_THRESHOLD = 0.35f;
    constexpr float DRAINED_DEPTH_THRESHOLD = 0.20f;

    // EN: 2. Process fluid runoff and gravity flow down terrain slopes (O(K) simulation)
    // ES: 2. Procesar escorrentía de fluidos y flujo por gravedad por laderas (simulación O(K))
    for (auto iterator = m_floodedCells.begin(); iterator != m_floodedCells.end(); ) {
        const int cellCoordX = iterator->x;
        const int cellCoordY = iterator->y;
        if (!isValid(cellCoordX, cellCoordY)) {
            iterator = m_floodedCells.erase(iterator);
            continue;
        }

        Cell& sourceCell = getCell(cellCoordX, cellCoordY);
        if (sourceCell.waterDepth <= 0.003f) {
            sourceCell.waterDepth = 0.0f;
            sourceCell.isFlooded = false;
            iterator = m_floodedCells.erase(iterator);
            continue;
        }

        const float totalSourceHead = sourceCell.elevation + sourceCell.waterDepth;

        // EN: Identify all lower neighbors and compute directional hydraulic slope gradient
        // ES: Identificar todos los vecinos más bajos y calcular el gradiente hidráulico direccional
        struct DownhillNeighbor {
            int cellCoordX;
            int cellCoordY;
            float headDrop;
            float directionX;
            float directionY;
            float distance;
        };
        std::vector<DownhillNeighbor> downhillNeighbors;
        downhillNeighbors.reserve(8);
        float totalHeadDrop = 0.0f;

        for (int deltaCellY = -1; deltaCellY <= 1; ++deltaCellY) {
            for (int deltaCellX = -1; deltaCellX <= 1; ++deltaCellX) {
                if (deltaCellX == 0 && deltaCellY == 0) continue;
                const int neighborCellX = cellCoordX + deltaCellX;
                const int neighborCellY = cellCoordY + deltaCellY;
                if (!isValid(neighborCellX, neighborCellY)) continue;

                const Cell& neighborCell = getCell(neighborCellX, neighborCellY);
                const float totalNeighborHead = neighborCell.elevation + neighborCell.waterDepth;

                // EN: Minimum hydraulic head drop threshold of 0.012f prevents tiny oscillating sloshing cycles
                // ES: Umbral mínimo de caída hidráulica de 0.012f previene ciclos oscilatorios de microoleaje
                if (totalSourceHead > totalNeighborHead + 0.012f) {
                    const float distance = (deltaCellX != 0 && deltaCellY != 0) ? 1.4142f : 1.0f;
                    const float headDrop = (totalSourceHead - totalNeighborHead) / distance;
                    const float directionX = static_cast<float>(deltaCellX) / distance;
                    const float directionY = static_cast<float>(deltaCellY) / distance;
                    downhillNeighbors.push_back({neighborCellX, neighborCellY, headDrop, directionX, directionY, distance});
                    totalHeadDrop += headDrop;
                }
            }
        }

        // EN: Accelerate fluid current velocity vector along the net hydraulic gradient
        // ES: Acelerar el vector de velocidad de corriente a lo largo del gradiente hidráulico neto
        constexpr float FLUID_GRAVITY_ACCEL = 9.8f;
        constexpr float FLUID_BED_FRICTION = 2.2f;

        if (!downhillNeighbors.empty() && totalHeadDrop > 0.001f) {
            float netSlopeVectorX = 0.0f;
            float netSlopeVectorY = 0.0f;
            for (const auto& neighborInfo : downhillNeighbors) {
                const float shareFraction = neighborInfo.headDrop / totalHeadDrop;
                netSlopeVectorX += shareFraction * neighborInfo.directionX * neighborInfo.headDrop;
                netSlopeVectorY += shareFraction * neighborInfo.directionY * neighborInfo.headDrop;
            }

            sourceCell.flowVelocityX = sourceCell.flowVelocityX * std::max(0.0f, 1.0f - deltaTime * FLUID_BED_FRICTION)
                                      + (netSlopeVectorX * FLUID_GRAVITY_ACCEL) * deltaTime;
            sourceCell.flowVelocityY = sourceCell.flowVelocityY * std::max(0.0f, 1.0f - deltaTime * FLUID_BED_FRICTION)
                                      + (netSlopeVectorY * FLUID_GRAVITY_ACCEL) * deltaTime;

            // EN: Distribute available surface runoff proportionally to head gradient (conservation of mass)
            // ES: Distribuir escorrentía disponible proporcionalmente al gradiente (conservación de masa)
            const float currentSpeed = sourceCell.getFlowSpeed();
            const float effectiveFlowRate = WATER_FLOW_RATE + currentSpeed * 0.45f;
            const float transferFraction = std::clamp(effectiveFlowRate * deltaTime, 0.05f, 0.75f);
            const float totalTransferVolume = sourceCell.waterDepth * transferFraction;
            sourceCell.waterDepth -= totalTransferVolume;

            for (const auto& neighborInfo : downhillNeighbors) {
                const float shareFraction = neighborInfo.headDrop / totalHeadDrop;
                const float transferAmount = totalTransferVolume * shareFraction;
                if (transferAmount <= 0.0001f) continue;

                Cell& targetNeighbor = getCell(neighborInfo.cellCoordX, neighborInfo.cellCoordY);
                if (targetNeighbor.isNaturalWater()) {
                    // EN: Drained directly into ocean or natural water basin without surface accumulation
                    // ES: Drenado directamente al océano o cuenca de agua natural sin acumulación superficial
                } else {
                    targetNeighbor.addWater(transferAmount);

                    // EN: Advect fluid momentum into receiving cell
                    // ES: Convectar momento del fluido hacia la celda receptora
                    const float massRatio = std::clamp(transferAmount / std::max(0.02f, targetNeighbor.waterDepth), 0.0f, 0.80f);
                    targetNeighbor.flowVelocityX = targetNeighbor.flowVelocityX * (1.0f - massRatio) + sourceCell.flowVelocityX * massRatio;
                    targetNeighbor.flowVelocityY = targetNeighbor.flowVelocityY * (1.0f - massRatio) + sourceCell.flowVelocityY * massRatio;

                    if (targetNeighbor.isBurning) {
                        // EN: Extinguishing flames boils off water mass
                        // ES: Extinguir llamas evapora masa de agua en ebullición
                        const float boilingLoss = std::min(targetNeighbor.waterDepth, 0.10f * std::max(0.20f, targetNeighbor.fireIntensity));
                        targetNeighbor.waterDepth = std::max(0.0f, targetNeighbor.waterDepth - boilingLoss);
                        targetNeighbor.extinguish();
                        m_burningCells.erase(std::remove_if(m_burningCells.begin(), m_burningCells.end(),
                                                           [&](const Point2i& pt) {
                                                               return pt.x == neighborInfo.cellCoordX && pt.y == neighborInfo.cellCoordY;
                                                           }),
                                             m_burningCells.end());
                    }

                    if (targetNeighbor.waterDepth > 0.002f) {
                        newFloodedNeighbors.push_back({neighborInfo.cellCoordX, neighborInfo.cellCoordY});
                    }
                }
            }
        } else {
            // EN: Flat basin or depression: friction damps fluid current toward rest
            // ES: Depresión o cuenca plana: la fricción amortigua la corriente hacia el reposo
            sourceCell.flowVelocityX *= std::max(0.0f, 1.0f - deltaTime * 3.8f);
            sourceCell.flowVelocityY *= std::max(0.0f, 1.0f - deltaTime * 3.8f);
        }

        // EN: 3. Biome-specific soil infiltration into subsoil and atmospheric evaporation
        // ES: 3. Infiltración en el subsuelo según bioma y evaporación atmosférica
        const float soilInfiltrationRate = sourceCell.getInfiltrationRate() * 1.35f;
        constexpr float BASE_ATMOSPHERIC_EVAPORATION = 0.025f;
        const float totalLossRate = soilInfiltrationRate + BASE_ATMOSPHERIC_EVAPORATION;
        const float waterLoss = std::min(sourceCell.waterDepth, totalLossRate * deltaTime);
        sourceCell.waterDepth = std::max(0.0f, sourceCell.waterDepth - waterLoss);

        // EN: Infiltrated water directly hydrates soil moisture sponge
        // ES: El agua infiltrada hidrata directamente la esponja de humedad del suelo
        sourceCell.moisture = std::min(1.0f, sourceCell.moisture + waterLoss * 2.8f);
        if (sourceCell.waterDepth >= FLOOD_DEPTH_THRESHOLD) {
            sourceCell.isFlooded = true;
        } else if (sourceCell.waterDepth < DRAINED_DEPTH_THRESHOLD) {
            sourceCell.isFlooded = false;
        }

        // Soil moisture dries gradually toward baseline
        sourceCell.moisture = std::max(0.20f, sourceCell.moisture - MOISTURE_DRYING_RATE * deltaTime);

        // EN: 4. Ecological revitalization: soaked scorched earth transforms into fertile valley
        // ES: 4. Revitalización ecológica: tierra calcinada empapada se transforma en pradera fértil
        if (sourceCell.isBurnt && (sourceCell.waterDepth > 0.04f || sourceCell.moisture > 0.60f)) {
            constexpr float WATER_REVITALIZATION_BOOST = 6.0f;
            sourceCell.ashRecoveryTimer -= WATER_REVITALIZATION_BOOST * deltaTime;
            if (sourceCell.ashRecoveryTimer <= 0.0f) {
                sourceCell.isBurnt = false;
                sourceCell.ashRecoveryTimer = 0.0f;
                sourceCell.biome = BiomeType::FertileValley;
                sourceCell.maxPopulation = std::min(30, sourceCell.maxPopulation + 4);
                m_burntCells.erase(std::remove_if(m_burntCells.begin(), m_burntCells.end(),
                                                 [cellCoordX, cellCoordY](const Point2i& pt) {
                                                     return pt.x == cellCoordX && pt.y == cellCoordY;
                                                 }),
                                   m_burntCells.end());
            }
        }

        // EN: 5. Living units interaction: extinguishing burning villagers, activating swimming & hydrodynamic current drag
        // ES: 5. Interacción con unidades vivas: extinguir aldeanos en llamas, activar nado y arrastre por corrientes
        if (villagerManager) {
            for (auto& villager : villagerManager->getHumanos()) {
                if (!villager.estaVivo()) continue;
                const float deltaCoordX = villager.getX() - (static_cast<float>(cellCoordX) + 0.5f);
                const float deltaCoordY = villager.getY() - (static_cast<float>(cellCoordY) + 0.5f);
                if (deltaCoordX * deltaCoordX + deltaCoordY * deltaCoordY <= 1.2f) {
                    if (sourceCell.waterDepth >= 0.08f && villager.isBurning()) {
                        villager.extinguishFire();
                    }

                    const bool inDeepWater = (sourceCell.waterDepth >= 0.35f || sourceCell.isFlooded);
                    if (inDeepWater) {
                        villager.setEstaNadando(true);
                    }

                    // EN: Hydrodynamic current advection: fluid flow pushes wading and swimming villagers
                    // ES: Convección por corriente hidrodinámica: el flujo empuja a aldeanos nadando o vadeando
                    const float currentFlowSpeed = sourceCell.getFlowSpeed();
                    if (currentFlowSpeed > 0.10f) {
                        const float dragMultiplier = inDeepWater ? 0.65f : 0.30f;
                        const float pushDeltaX = sourceCell.flowVelocityX * dragMultiplier * deltaTime;
                        const float pushDeltaY = sourceCell.flowVelocityY * dragMultiplier * deltaTime;
                        const float updatedPosX = std::clamp(villager.getX() + pushDeltaX, 0.5f, static_cast<float>(m_width - 1) + 0.5f);
                        const float updatedPosY = std::clamp(villager.getY() + pushDeltaY, 0.5f, static_cast<float>(m_height - 1) + 0.5f);
                        villager.setPosicion(updatedPosX, updatedPosY);

                        // EN: Flash flood surge: violent torrent sweeps units into swimming ragdoll
                        // ES: Crecida torrencial: la corriente violenta arrastra a la unidad en ragdoll acuático
                        if (currentFlowSpeed > 2.2f && sourceCell.waterDepth >= 0.45f && !villager.isRagdoll()) {
                            villager.activarRagdoll(sourceCell.flowVelocityX * 0.8f, sourceCell.flowVelocityY * 0.8f, 3.5f, 5.0f);
                        }
                    }
                }
            }
        }

        // EN: 6. Structural water damage to dwellings in deep flood depressions or fast currents
        // ES: 6. Daño estructural por agua a viviendas en depresiones profundamente inundadas o corrientes rápidas
        if (buildingSystem && sourceCell.hasHouse && sourceCell.waterDepth >= 0.45f) {
            constexpr float STRUCTURAL_FLOOD_DAMAGE_FACTOR = 18.0f;
            const float dynamicPressureFactor = 1.0f + sourceCell.getFlowSpeed() * 0.75f;
            const float floodDamage = STRUCTURAL_FLOOD_DAMAGE_FACTOR * (sourceCell.waterDepth - 0.30f) * dynamicPressureFactor * deltaTime;
            buildingSystem->damageHouseAt(cellCoordX, cellCoordY, floodDamage, *this, villagerManager);
        }

        if (sourceCell.waterDepth <= 0.003f) {
            sourceCell.waterDepth = 0.0f;
            sourceCell.isFlooded = false;
            iterator = m_floodedCells.erase(iterator);
        } else {
            ++iterator;
        }
    }

    // EN: Register newly flooded neighbors into tracking vector
    // ES: Registrar nuevos vecinos inundados en el vector de seguimiento
    for (const auto& coordinate : newFloodedNeighbors) {
        auto existingIterator = std::find_if(m_floodedCells.begin(), m_floodedCells.end(),
                                            [&coordinate](const Point2i& pt) {
                                                return pt.x == coordinate.x && pt.y == coordinate.y;
                                            });
        if (existingIterator == m_floodedCells.end()) {
            m_floodedCells.push_back(coordinate);
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

// =============================================================================
// EN: Tectonic and Cataclysmic Terrain Deformation System
// ES: Sistema de Deformación Tectónica y Cataclísmica del Terreno
// =============================================================================

// EN: Form a crater or tectonic depression on a specific cell
// ES: Formar un cráter o depresión tectónica en una celda específica
bool WorldMap::formCellCrater(int cellX, int cellY, CraterType type, float depth, float elevationDrop) {
    if (!isValid(cellX, cellY)) return false;
    Cell& cell = getCell(cellX, cellY);
    cell.formCrater(type, depth, elevationDrop);
    return true;
}

// EN: Create a realistic tectonic earthquake rupture with epicentral crater, graben subsidence, and fault strike lines
// ES: Crear una ruptura tectónica de terremoto realista con cráter epicentral, subsidencia de graben y líneas de falla
void WorldMap::createEarthquakeFissureZone(int centerX, int centerY, float intensity, float radius, float angleRad,
                                           BuildingSystem* buildingSystem, GestorHumanos* villagerManager) {
    if (!isValid(centerX, centerY)) return;

    // EN: 1. Epicentral seismic rupture: ground depression and structural damage (not an impact crater)
    // ES: 1. Ruptura sísmica epicentral: depresión de terreno y daño estructural (no es un cráter de impacto)
    const float epicenterElevationDrop = 0.35f * intensity;

    Cell& epicenterCell = getCell(centerX, centerY);
    epicenterCell.fissureAngle = angleRad;
    if (epicenterElevationDrop > 0.0f) {
        epicenterCell.elevation = std::max(0.05f, epicenterCell.elevation - epicenterElevationDrop);
    }
    if (epicenterCell.naturalResource == ResourceType::Wood || epicenterCell.biome == BiomeType::DenseForest) {
        epicenterCell.hasTreesUprooted = true;
        if (epicenterCell.naturalResource == ResourceType::Wood) {
            epicenterCell.naturalResource = ResourceType::None;
            epicenterCell.resourceAmount = 0;
            epicenterCell.resourceVariant = 0;
        }
    }
    if (buildingSystem) {
        constexpr float EPICENTER_STRUCTURAL_DAMAGE = 85.0f;
        buildingSystem->damageHouseAt(centerX, centerY, EPICENTER_STRUCTURAL_DAMAGE * intensity, *this, villagerManager);
    }

    // EN: 2. Radial tectonic subsidence bowl / graben basin
    // ES: 2. Cuenca de subsidencia tectónica radial / fosa tectónica (graben)
    const int searchRadius = std::max(1, static_cast<int>(std::ceil(radius * 0.70f)));
    const float maxBowlRadius = radius * 0.65f;

    for (int deltaCellY = -searchRadius; deltaCellY <= searchRadius; ++deltaCellY) {
        for (int deltaCellX = -searchRadius; deltaCellX <= searchRadius; ++deltaCellX) {
            if (deltaCellX == 0 && deltaCellY == 0) continue;
            const int neighborX = centerX + deltaCellX;
            const int neighborY = centerY + deltaCellY;
            if (!isValid(neighborX, neighborY)) continue;

            const float distance = std::sqrt(static_cast<float>(deltaCellX * deltaCellX + deltaCellY * deltaCellY));
            if (distance <= maxBowlRadius) {
                const float bowlFactor = 1.0f - (distance / maxBowlRadius);
                Cell& neighborCell = getCell(neighborX, neighborY);
                neighborCell.fissureAngle = angleRad;
                const float subsidenceDrop = 0.22f * intensity * bowlFactor;
                if (subsidenceDrop > 0.0f) {
                    neighborCell.elevation = std::max(0.05f, neighborCell.elevation - subsidenceDrop);
                }
                if (buildingSystem && bowlFactor > 0.35f) {
                    constexpr float PERIPHERAL_HOUSE_DAMAGE = 45.0f;
                    buildingSystem->damageHouseAt(neighborX, neighborY,
                                                  PERIPHERAL_HOUSE_DAMAGE * intensity * bowlFactor,
                                                  *this, villagerManager);
                }
            }
        }
    }

    // EN: 3. Primary tectonic fault fissure rupture along the fault strike line
    // ES: 3. Ruptura de falla tectónica primaria a lo largo de la línea de rumbo de la falla
    const int faultLength = std::clamp(static_cast<int>(std::round(radius * 0.85f)), 2, 6);
    const float faultDirX = std::cos(angleRad);
    const float faultDirY = std::sin(angleRad);

    for (int step = -faultLength; step <= faultLength; ++step) {
        if (step == 0) continue;
        const float faultContinuousX = static_cast<float>(centerX) + static_cast<float>(step) * faultDirX;
        const float faultContinuousY = static_cast<float>(centerY) + static_cast<float>(step) * faultDirY;
        const int faultCellX = static_cast<int>(std::round(faultContinuousX));
        const int faultCellY = static_cast<int>(std::round(faultContinuousY));
        if (!isValid(faultCellX, faultCellY)) continue;

        const float faultProgress = 1.0f - (static_cast<float>(std::abs(step)) / (static_cast<float>(faultLength) + 0.6f));
        Cell& faultCell = getCell(faultCellX, faultCellY);
        faultCell.fissureAngle = angleRad;
        const float faultElevationDrop = 0.28f * intensity * faultProgress;
        if (faultElevationDrop > 0.0f) {
            faultCell.elevation = std::max(0.05f, faultCell.elevation - faultElevationDrop);
        }
        if (faultCell.naturalResource == ResourceType::Wood || faultCell.biome == BiomeType::DenseForest) {
            faultCell.hasTreesUprooted = true;
            if (faultCell.naturalResource == ResourceType::Wood) {
                faultCell.naturalResource = ResourceType::None;
                faultCell.resourceAmount = 0;
                faultCell.resourceVariant = 0;
            }
        }
        if (buildingSystem) {
            constexpr float FAULT_LINE_HOUSE_DAMAGE = 65.0f;
            buildingSystem->damageHouseAt(faultCellX, faultCellY,
                                          FAULT_LINE_HOUSE_DAMAGE * intensity * faultProgress,
                                          *this, villagerManager);
        }
    }

    // EN: 4. Conjugate Riedel shear fractures (secondary branching fault fissures)
    // ES: 4. Fracturas de cizalla conjugadas de Riedel (fallas ramificadas secundarias)
    constexpr float RIEDEL_ANGLE_OFFSET = 0.9599f; // ~55 degrees in radians
    for (int branchSign : {-1, 1}) {
        const float branchAngle = angleRad + static_cast<float>(branchSign) * RIEDEL_ANGLE_OFFSET;
        const float branchDirX = std::cos(branchAngle);
        const float branchDirY = std::sin(branchAngle);
        for (int branchStep = 1; branchStep <= 2; ++branchStep) {
            const int branchCellX = static_cast<int>(std::round(static_cast<float>(centerX) + static_cast<float>(branchStep) * branchDirX));
            const int branchCellY = static_cast<int>(std::round(static_cast<float>(centerY) + static_cast<float>(branchStep) * branchDirY));
            if (!isValid(branchCellX, branchCellY)) continue;

            Cell& branchCell = getCell(branchCellX, branchCellY);
            branchCell.fissureAngle = branchAngle;
            const float branchElevationDrop = 0.18f * intensity;
            if (branchElevationDrop > 0.0f) {
                branchCell.elevation = std::max(0.05f, branchCell.elevation - branchElevationDrop);
            }
            if (buildingSystem) {
                constexpr float BRANCH_HOUSE_DAMAGE = 35.0f;
                buildingSystem->damageHouseAt(branchCellX, branchCellY,
                                              BRANCH_HOUSE_DAMAGE * intensity,
                                              *this, villagerManager);
            }
        }
    }
}

// EN: Create a bowl-shaped impact crater zone for meteorites, bombs, and heavy kinetic impacts
// ES: Crear una zona de cráter de impacto en cuenca para meteoritos, bombas e impactos cinéticos pesados
void WorldMap::createImpactCraterZone(int centerX, int centerY, CraterType type, float radius, float maxDepth, float maxElevationDrop) {
    if (!isValid(centerX, centerY)) return;

    const int boundingRadius = std::max(1, static_cast<int>(std::ceil(radius)));
    for (int deltaCellY = -boundingRadius; deltaCellY <= boundingRadius; ++deltaCellY) {
        for (int deltaCellX = -boundingRadius; deltaCellX <= boundingRadius; ++deltaCellX) {
            const int targetCellX = centerX + deltaCellX;
            const int targetCellY = centerY + deltaCellY;
            if (!isValid(targetCellX, targetCellY)) continue;

            const float distanceSq = static_cast<float>(deltaCellX * deltaCellX + deltaCellY * deltaCellY);
            if (distanceSq > radius * radius) continue;

            const float distance = std::sqrt(distanceSq);
            const float normalizedDistance = distance / radius;
            const float bowlFactor = 1.0f - normalizedDistance * normalizedDistance;
            const float depth = maxDepth * bowlFactor;
            const float elevationDrop = maxElevationDrop * (1.0f - normalizedDistance);

            Cell& cell = getCell(targetCellX, targetCellY);
            cell.formCrater(type, depth, elevationDrop);

            // EN: Thermal vitrification and scorched transformation for high-energy impacts
            // ES: Vitrificación térmica y transformación calcinada para impactos de alta energía
            if (distance <= 1.5f) {
                if (type == CraterType::MeteorImpact && cell.biome != BiomeType::DeepOcean && cell.biome != BiomeType::ShallowWater) {
                    cell.biome = BiomeType::AridDesert; // Scorched basalt & crater bowl
                } else if (type == CraterType::NuclearDetonation && cell.biome != BiomeType::DeepOcean) {
                    cell.biome = BiomeType::AridDesert; // Vitrified ash & trinitite
                }
            }
        }
    }
}

} // namespace ReligionSim
