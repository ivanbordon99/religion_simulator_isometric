#pragma once

#include "Cell.hpp"
#include "Religion.hpp"
#include <vector>
#include <memory>

namespace ReligionSim {

class BuildingSystem;
class GestorHumanos;

class WorldMap {
public:
    WorldMap(int width = 64, int height = 64);

    void generateProceduralWorld(uint32_t seed = 42, WorldClimate climate = WorldClimate::Templado, ResourceAbundance abundance = ResourceAbundance::Equilibrado);
    void seedPrimordialFaiths(ReligionRegistry& registry, int currentYear);

    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }

    Cell& getCell(int cellX, int cellY);
    const Cell& getCell(int cellX, int cellY) const;

    // EN: Check if the coordinate is within world boundaries
    // ES: Verificar si la coordenada está dentro de los límites del mundo
    bool isValid(int cellX, int cellY) const {
        return cellX >= 0 && cellX < m_width && cellY >= 0 && cellY < m_height;
    }

    std::vector<Point2i> getNeighbors(int cellX, int cellY, bool includeDiagonals = false) const;
    float getTravelEase(const Cell& from, const Cell& to) const;

    int getTotalWorldPopulation() const;
    void recalculateReligions(ReligionRegistry& registry);

    // EN: Consecrate and upgrade holy sites on specific map coordinates
    // ES: Consagrar y mejorar sitios sagrados en coordenadas específicas del mapa
    void consecrateHolySite(int cellX, int cellY, const std::string& name, int religionId = 0);
    void upgradeHolySite(int cellX, int cellY);

    // EN: Procedural generation of village settlement locations with dispersed distances
    // ES: Generación procedimental de ubicaciones de asentamientos con dispersión de distancia
    std::vector<Point2i> generatePuebloLocations(int count, uint32_t seed) const;

    void assignNaturalResources(uint32_t seed, ResourceAbundance abundance = ResourceAbundance::Equilibrado);
    void updateResources(float deltaTime);

    // EN: Fire physics simulation: ignition, directional wind spread, fuel depletion, extinction, and ecological recovery
    // ES: Simulación física del fuego: ignición, propagación por viento, agotamiento de combustible, extinción y recuperación ecológica
    bool igniteCell(int cellX, int cellY, float durationSeconds = 14.0f, float initialIntensity = 1.0f);
    void extinguishCell(int cellX, int cellY);
    void updateFirePhysics(float deltaTime, BuildingSystem* buildingSystem = nullptr,
                           GestorHumanos* villagerManager = nullptr,
                           float windVelocityX = 0.22f, float windVelocityY = 0.08f);
    [[nodiscard]] bool hasActiveFires() const { return !m_burningCells.empty(); }
    [[nodiscard]] const std::vector<Point2i>& getBurningCells() const { return m_burningCells; }
    [[nodiscard]] int getBurningCellCount() const { return static_cast<int>(m_burningCells.size()); }
    [[nodiscard]] const std::vector<Point2i>& getBurntCells() const { return m_burntCells; }

    // EN: Water and fluid physics simulation: precipitation, runoff, gravity flow, flooding, evaporation, and fire interaction
    // ES: Simulación física del agua y fluidos: precipitación, escorrentía, flujo por gravedad, inundación, evaporación e interacción con el fuego
    void addWaterToCell(int cellX, int cellY, float amount);
    void updateWaterPhysics(float deltaTime, BuildingSystem* buildingSystem = nullptr,
                            GestorHumanos* villagerManager = nullptr);
    [[nodiscard]] bool hasFloodedCells() const { return !m_floodedCells.empty(); }
    [[nodiscard]] const std::vector<Point2i>& getFloodedCells() const { return m_floodedCells; }
    [[nodiscard]] int getFloodedCellCount() const { return static_cast<int>(m_floodedCells.size()); }

    // EN: Update the 4 water depth phases across the entire map based on bathymetry and proximity to shore
    // ES: Actualizar las 4 fases de profundidad del agua en todo el mapa según batimetría y proximidad a la costa
    void updateWaterDepthPhases() noexcept;

    // EN: Tectonic and cataclysmic terrain deformation methods
    // ES: Métodos de deformación tectónica y cataclísmica del terreno
    bool formCellCrater(int cellX, int cellY, CraterType type, float depth, float elevationDrop = 0.20f);
    void createEarthquakeFissureZone(int centerX, int centerY, float intensity, float radius, float angleRad,
                                     BuildingSystem* buildingSystem = nullptr, GestorHumanos* villagerManager = nullptr);
    void createImpactCraterZone(int centerX, int centerY, CraterType type, float radius, float maxDepth, float maxElevationDrop);

private:
    int m_width{48};
    int m_height{48};
    std::vector<Cell> m_cells;
    std::vector<Point2i> m_burningCells;
    std::vector<Point2i> m_burntCells;
    std::vector<Point2i> m_floodedCells;

    void assignBiomesAndElevations(uint32_t seed, WorldClimate climate = WorldClimate::Templado);
    void assignPopulationsAndRivers();
};

} // namespace ReligionSim
