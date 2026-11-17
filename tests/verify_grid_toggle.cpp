#include "Game.hpp"
#include <iostream>
#include <cassert>

int main() {
    // EN: Initialize game engine in 1280x720 window
    // ES: Inicializar motor de juego en ventana de 1280x720
    ReligionSim::Game game(1280, 720);
    game.init();
    game.startNewCustomWorld();
    game.setGameState(ReligionSim::GameState::Playing);
    game.runCommand("LUMINUS");

    ReligionSim::WorldMap& worldMap = game.getMap();
    constexpr int centerCellX = 25;
    constexpr int centerCellY = 25;

    // EN: Ensure mixed terrain and water around center
    // ES: Asegurar terreno mixto y agua alrededor del centro
    for (int deltaY = -6; deltaY <= 6; ++deltaY) {
        for (int deltaX = -6; deltaX <= 6; ++deltaX) {
            const int cx = centerCellX + deltaX;
            const int cy = centerCellY + deltaY;
            if (!worldMap.isValid(cx, cy)) continue;

            ReligionSim::Cell& cell = worldMap.getCell(cx, cy);
            if (deltaX <= -2) {
                cell.biome = (deltaX <= -4) ? ReligionSim::BiomeType::DeepOcean : ReligionSim::BiomeType::ShallowWater;
                cell.elevation = (deltaX <= -4) ? 0.1f : 0.35f;
            } else if (deltaX == -1) {
                cell.biome = ReligionSim::BiomeType::CoastalPlains;
                cell.elevation = 0.55f;
            } else {
                cell.biome = ReligionSim::BiomeType::FertileValley;
                cell.elevation = 0.85f;
            }
        }
    }

    game.getCamera().setZoom(1.0f);
    game.getCamera().centerOnCell(centerCellX, centerCellY, worldMap.getWidth(), worldMap.getHeight());
    game.update(0.5f);

    // EN: 1. Render default frame with Grid OFF
    // ES: 1. Renderizar cuadro por defecto con Retícula APAGADA
    assert(!game.getSettings().showIsometricGrid);
    assert(!game.getRenderer().isGridVisible());
    game.render();
    game.getRenderer().exportFrameToPPM("snapshot_grid_off.ppm");
    std::cout << "[Verification] Exported snapshot_grid_off.ppm" << std::endl;

    // EN: 2. Toggle Grid ON
    // ES: 2. Alternar Retícula ENCENDIDA
    game.getSettingsMutable().toggleIsometricGrid();
    game.applySettings(game.getSettings());
    assert(game.getSettings().showIsometricGrid);
    assert(game.getRenderer().isGridVisible());
    game.render();
    game.getRenderer().exportFrameToPPM("snapshot_grid_on.ppm");
    std::cout << "[Verification] Exported snapshot_grid_on.ppm" << std::endl;

    // EN: 3. Render Options Menu showing the Grid option button
    // ES: 3. Renderizar Menú de Opciones mostrando el botón de opción de Retícula
    game.setGameState(ReligionSim::GameState::Options);
    game.setCurrentOptionsTab(ReligionSim::OptionsTab::GraphicsAndImage);
    game.render();
    game.getRenderer().exportFrameToPPM("snapshot_options_grid.ppm");
    std::cout << "[Verification] Exported snapshot_options_grid.ppm" << std::endl;

    return 0;
}
