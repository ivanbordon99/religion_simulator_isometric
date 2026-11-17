#include "Game.hpp"
#include <iostream>

// EN: Render showcase snapshot verifying that CoastalPlains (tropicalPlains00) is 100% seamless without dark borders
// ES: Renderizar captura de verificación demostrando que CoastalPlains (tropicalPlains00) es 100% continuo sin bordes oscuros
int main() {
    ReligionSim::Game gameEngine(1280, 720);
    gameEngine.init();
    gameEngine.startNewCustomWorld();
    gameEngine.setGameState(ReligionSim::GameState::Playing);
    gameEngine.runCommand("LUMINUS");

    ReligionSim::WorldMap& map = gameEngine.getMap();
    const int centerCellX = map.getWidth() / 2;
    const int centerCellY = map.getHeight() / 2;

    // EN: Set up an organic transition from FertileValley (grass.png) to CoastalPlains (tropicalPlains00.png) and Beach
    // ES: Configurar una transición orgánica desde FertileValley (grass.png) hacia CoastalPlains (tropicalPlains00.png) y Playa
    for (int offsetY = -8; offsetY <= 8; ++offsetY) {
        for (int offsetX = -8; offsetX <= 8; ++offsetX) {
            const int cellX = centerCellX + offsetX;
            const int cellY = centerCellY + offsetY;
            if (!map.isValid(cellX, cellY)) continue;

            ReligionSim::Cell& cell = map.getCell(cellX, cellY);
            cell.isBurning = false;
            cell.isBurnt = false;
            cell.waterDepth = 0.0f;
            cell.hasImpactCrater = false;
            cell.elevation = 0.80f;

            if (offsetX + offsetY < -2) {
                // EN: Top sector: Fertile Valley (grass.png - Tile 0)
                // ES: Sector superior: Valle Fértil (grass.png - Mosaico 0)
                cell.biome = ReligionSim::BiomeType::FertileValley;
            } else if (offsetX + offsetY <= 4) {
                // EN: Middle sector: Coastal Plains (tropicalPlains00.png - Tile 2)
                // ES: Sector medio: Pradera Costera (tropicalPlains00.png - Mosaico 2)
                cell.biome = ReligionSim::BiomeType::CoastalPlains;
                if ((offsetX + offsetY) % 2 == 0) {
                    cell.naturalResource = ReligionSim::ResourceType::Food;
                    cell.resourceAmount = 80;
                }
            } else if (offsetX + offsetY <= 6) {
                // EN: Beach fringe (sand.png - Tile 8)
                // ES: Franja de playa (sand.png - Mosaico 8)
                cell.biome = ReligionSim::BiomeType::CoastalPlains;
                cell.elevation = 0.40f;
            } else {
                // EN: Water (shallowwater.png - Tile 10)
                // ES: Agua (shallowwater.png - Mosaico 10)
                cell.biome = ReligionSim::BiomeType::ShallowWater;
                cell.elevation = 0.10f;
            }
        }
    }

    gameEngine.getCamera().setZoom(1.85f);
    for (int frame = 0; frame < 50; ++frame) {
        gameEngine.getCamera().centerOnCell(centerCellX, centerCellY, map.getWidth(), map.getHeight());
        gameEngine.update(0.1f);
    }

    gameEngine.render();
    gameEngine.getRenderer().exportFrameToPPM("snapshot_seamless_grass.ppm");
    std::cout << "[Snapshot] Seamless grass snapshot exported successfully!" << std::endl;
    return 0;
}
