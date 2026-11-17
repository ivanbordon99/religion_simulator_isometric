#include "Game.hpp"
#include <iostream>

int main() {
    // EN: Initialize simulation engine in 1280x720 canvas
    // ES: Inicializar el motor de simulación en un lienzo de 1280x720
    ReligionSim::Game gameEngine(1280, 720);
    gameEngine.init();
    gameEngine.startNewCustomWorld();
    gameEngine.setGameState(ReligionSim::GameState::Playing);
    gameEngine.getRenderer().setFogOfWar(nullptr);

    ReligionSim::WorldMap& worldMap = gameEngine.getMap();
    constexpr int centerCellX = 25;
    constexpr int centerCellY = 25;

    // EN: Construct a dynamic landscape showcasing ocean, beaches, rivers, waterfalls and flooded pools
    // ES: Construir un paisaje dinámico exhibiendo océano, playas, ríos, cascadas y pozas inundadas
    for (int deltaOffsetY = -7; deltaOffsetY <= 7; ++deltaOffsetY) {
        for (int deltaOffsetX = -7; deltaOffsetX <= 7; ++deltaOffsetX) {
            const int targetCellX = centerCellX + deltaOffsetX;
            const int targetCellY = centerCellY + deltaOffsetY;
            if (!worldMap.isValid(targetCellX, targetCellY)) continue;

            ReligionSim::Cell& targetCell = worldMap.getCell(targetCellX, targetCellY);
            targetCell.elevation = 0.85f;
            targetCell.isBurning = false;
            targetCell.isBurnt = false;
            targetCell.hasTreesUprooted = false;
            targetCell.waterDepth = 0.0f;
            targetCell.flowVelocityX = 0.0f;
            targetCell.flowVelocityY = 0.0f;
            targetCell.isFlooded = false;

            if (deltaOffsetX <= -4) {
                // EN: Deep Ocean zone with rolling swells and cresting waves
                // ES: Zona de océano profundo con oleaje y crestas de olas
                targetCell.biome = ReligionSim::BiomeType::DeepOcean;
                targetCell.elevation = 0.10f;
            } else if (deltaOffsetX >= -3 && deltaOffsetX <= -2) {
                // EN: Shallow Water coastal lagoon with glistening caustics & ripples
                // ES: Laguna costera de agua poco profunda con cáusticas y ondas relucientes
                targetCell.biome = ReligionSim::BiomeType::ShallowWater;
                targetCell.elevation = 0.35f;
            } else if (deltaOffsetX >= -1 && deltaOffsetX <= 0) {
                // EN: Sandy shoreline coastal plains receiving crashing surf foam
                // ES: Llanuras costeras de arena recibiendo espuma viva de rompiente
                targetCell.biome = ReligionSim::BiomeType::CoastalPlains;
                targetCell.elevation = 0.65f;
            } else if (deltaOffsetX >= 4 && deltaOffsetY <= -3) {
                // EN: Highland mountains with high elevation ridge
                // ES: Montañas altas con cresta de gran elevación
                targetCell.biome = ReligionSim::BiomeType::RuggedMountains;
                targetCell.elevation = 1.65f;
            } else if (deltaOffsetX >= 1 && deltaOffsetX <= 3 && deltaOffsetY == 0) {
                // EN: Overland active river stream rushing downhill
                // ES: Río superficial activo fluyendo cuesta abajo
                targetCell.biome = ReligionSim::BiomeType::FertileValley;
                targetCell.elevation = 0.70f;
                targetCell.waterDepth = 0.22f;
                targetCell.flowVelocityX = -0.45f;
                targetCell.flowVelocityY = 0.12f;
            } else {
                targetCell.biome = ReligionSim::BiomeType::FertileValley;
                targetCell.elevation = 0.85f;
            }
        }
    }

    // EN: Setup deep flooded sacred spring with active whirlpool vortex at (centerCellX + 1, centerCellY + 2)
    // ES: Configurar manantial sagrado inundado con vórtice de remolino activo en (centerCellX + 1, centerCellY + 2)
    ReligionSim::Cell& vortexCell = worldMap.getCell(centerCellX + 1, centerCellY + 2);
    vortexCell.isFlooded = true;
    vortexCell.waterDepth = 0.45f;
    vortexCell.elevation = 0.60f;

    // EN: Center camera on the water interaction landscape with 1.0f zoom
    // ES: Centrar la cámara en el paisaje de interacción acuática con zoom 1.0f
    gameEngine.getCamera().setZoom(1.0f);
    gameEngine.getCamera().centerOnCell(centerCellX, centerCellY, worldMap.getWidth(), worldMap.getHeight());

    // EN: Update simulation to advance water animation time
    // ES: Actualizar simulación para avanzar el tiempo de animación de agua
    gameEngine.update(1.20f);

    // EN: Render frame with water movement sprites, ocean waves, surf foam and currents
    // ES: Renderizar cuadro con sprites de movimiento de agua, olas oceánicas, espuma y corrientes
    gameEngine.render();

    const std::string exportPpmPath = "snapshot_water_movement.ppm";
    gameEngine.getRenderer().exportFrameToPPM(exportPpmPath);
    std::cout << "[Snapshot] Water movement showcase snapshot exported to " << exportPpmPath << std::endl;

    return 0;
}
