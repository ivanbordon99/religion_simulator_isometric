#pragma once

#include "Cell.hpp"
#include "Religion.hpp"
#include <vector>
#include <memory>

namespace ReligionSim {

class WorldMap {
public:
    WorldMap(int width = 48, int height = 48);

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

private:
    int m_width{48};
    int m_height{48};
    std::vector<Cell> m_cells;

    void assignBiomesAndElevations(uint32_t seed, WorldClimate climate = WorldClimate::Templado);
    void assignPopulationsAndRivers();
};

} // namespace ReligionSim
