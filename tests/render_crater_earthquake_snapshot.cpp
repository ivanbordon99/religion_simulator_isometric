// =============================================================================
// EN: Snapshot verification for terrain crater formation during earthquakes and heavy impacts
// ES: Verificación visual de formación de cráteres en terreno durante terremotos e impactos fuertes
// =============================================================================

#include "Game.hpp"
#include <iostream>
#include <string>

int main() {
    using namespace ReligionSim;

    std::cout << "[Snapshot] Initializing headless game engine for crater and earthquake snapshot..." << std::endl;
    Game gameEngine(1280, 720);
    gameEngine.init();
    gameEngine.startNewCustomWorld();
    gameEngine.setGameState(GameState::Playing);
    gameEngine.runCommand("LUMINUS"); // EN: Clear fog of war / ES: Disipar niebla de guerra
    gameEngine.runCommand("MEDIODIA"); // EN: Full bright midday sunlight / ES: Plena luz solar de mediodía

    WorldMap& worldMap = gameEngine.getMap();

    // EN: Find deepest continental interior with maximum land density
    // ES: Encontrar el interior continental más profundo con máxima densidad de tierra firme
    int hearthX = 64;
    int hearthY = 64;
    int bestLandScore = -1;

    for (int cellY = 12; cellY < worldMap.getHeight() - 12; ++cellY) {
        for (int cellX = 12; cellX < worldMap.getWidth() - 12; ++cellX) {
            int landCount = 0;
            for (int dy = -8; dy <= 8; ++dy) {
                for (int dx = -8; dx <= 8; ++dx) {
                    if (worldMap.isValid(cellX + dx, cellY + dy) && !worldMap.getCell(cellX + dx, cellY + dy).isWater()) {
                        landCount++;
                    }
                }
            }
            if (landCount > bestLandScore) {
                bestLandScore = landCount;
                hearthX = cellX;
                hearthY = cellY;
            }
        }
    }

    std::cout << "[Snapshot] Deep continental interior located at (" << hearthX << ", " << hearthY
              << ") with land score " << bestLandScore << std::endl;

    // EN: Establish lush fertile valley and forest baseline for clear crater demonstration
    // ES: Establecer valle fértil exuberante y bosque base para demostración clara de cráteres
    for (int deltaCellY = -8; deltaCellY <= 8; ++deltaCellY) {
        for (int deltaCellX = -8; deltaCellX <= 8; ++deltaCellX) {
            const int targetCoordX = hearthX + deltaCellX;
            const int targetCoordY = hearthY + deltaCellY;
            if (!worldMap.isValid(targetCoordX, targetCoordY)) continue;
            Cell& targetCell = worldMap.getCell(targetCoordX, targetCoordY);
            targetCell.biome = ((deltaCellX + deltaCellY) % 3 == 0) ? BiomeType::DenseForest : BiomeType::FertileValley;
            targetCell.elevation = 1.35f;
            targetCell.naturalResource = ResourceType::Wood;
            targetCell.resourceAmount = 140;
            targetCell.hasTreesUprooted = false;
            targetCell.hasImpactCrater = false;
            targetCell.craterDepth = 0.0f;
        }
    }

    const int quakeTargetX = hearthX - 2;
    const int quakeTargetY = hearthY + 2;
    const int meteorTargetX = hearthX + 2;
    const int meteorTargetY = hearthY - 2;

    // EN: Center camera on the valley and update smoothly until reaching position
    gameEngine.getCamera().setZoom(1.25f);
    gameEngine.getCamera().centerOnCell(hearthX, hearthY, worldMap.getWidth(), worldMap.getHeight());
    for (int step = 0; step < 40; ++step) {
        gameEngine.getCamera().update(0.1f);
    }

    // EN: 1. Capture pristine baseline terrain before catastrophes
    // ES: 1. Capturar terreno base prístino antes de las catástrofes
    gameEngine.getCamera().centerOnCell(hearthX, hearthY, worldMap.getWidth(), worldMap.getHeight());
    for (int step = 0; step < 40; ++step) {
        gameEngine.getCamera().update(0.1f);
    }
    gameEngine.render();
    const std::string beforePpmPath = "snapshot_crater_before.ppm";
    gameEngine.getRenderer().exportFrameToPPM(beforePpmPath);
    std::cout << "[Snapshot] Baseline snapshot exported to " << beforePpmPath << std::endl;

    // EN: 2. Cast Earthquake at (22, 22) - forms seismic fault rifts and graben trenches
    // ES: 2. Lanzar Terremoto Sísmico en (22, 22) - forma fallas sísmicas y fosas tectónicas
    gameEngine.castDivinePower(DivinePowerType::Earthquake, quakeTargetX, quakeTargetY);

    // EN: 3. Cast Meteor Cataclysm at (28, 20) - forms incandescent impact crater bowl
    // ES: 3. Lanzar Cataclismo de Meteorito en (28, 20) - forma cuenca de cráter de impacto incandescente
    gameEngine.castDivinePower(DivinePowerType::Meteor, meteorTargetX, meteorTargetY);

    // EN: 4. Advance simulation to let meteor strike ground and earthquake rumble
    // ES: 4. Avanzar la simulación para permitir el impacto del meteorito y la sacudida del sismo
    constexpr float SIMULATION_ADVANCE_TIME = 2.0f;
    constexpr float TIME_STEP = 0.05f;
    float accumulatedTime = 0.0f;
    while (accumulatedTime < SIMULATION_ADVANCE_TIME) {
        gameEngine.update(TIME_STEP);
        accumulatedTime += TIME_STEP;
    }

    // EN: 5. Align camera on the identical original position and render frame showing permanent craters and fault rifts
    // ES: 5. Alinear cámara en la posición original idéntica y renderizar cuadro mostrando cráteres y fallas
    gameEngine.getCamera().centerOnCell(hearthX, hearthY, worldMap.getWidth(), worldMap.getHeight());
    for (int step = 0; step < 40; ++step) {
        gameEngine.getCamera().update(0.1f);
    }

    gameEngine.render();
    const std::string afterPpmPath = "snapshot_crater_after.ppm";
    gameEngine.getRenderer().exportFrameToPPM(afterPpmPath);
    std::cout << "[Snapshot] Earthquake and impact crater snapshot exported to " << afterPpmPath << std::endl;

    return 0;
}
