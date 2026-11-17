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

    // EN: Create a continuous oceanic transect displaying the 4 progressive water depth phases:
    // 1 - Shoreline Foam / Orilla (DeltaX = 0 to 1)
    // 2 - Shallow Coast (DeltaX = -1 to -3)
    // 3 - Mid Sea / Ocean (DeltaX = -4 to -6)
    // 4 - Deep Sea / Abyssal (DeltaX = -7 to -10)
    // Land / Beach (DeltaX = 2 to 6)
    // ES: Crear un transecto oceánico continuo exhibiendo las 4 fases progresivas de profundidad:
    // 1 - Línea de Espuma / Orilla (DeltaX = 0 a 1)
    // 2 - Aguas Someras / Costa (DeltaX = -1 a -3)
    // 3 - Mar Medio / Océano (DeltaX = -4 a -6)
    // 4 - Mar Profundo / Abisal (DeltaX = -7 a -10)
    // Tierra / Playa (DeltaX = 2 a 6)
    for (int deltaOffsetY = -8; deltaOffsetY <= 8; ++deltaOffsetY) {
        for (int deltaOffsetX = -10; deltaOffsetX <= 8; ++deltaOffsetX) {
            const int targetCellX = centerCellX + deltaOffsetX;
            const int targetCellY = centerCellY + deltaOffsetY;
            if (!worldMap.isValid(targetCellX, targetCellY)) continue;

            ReligionSim::Cell& targetCell = worldMap.getCell(targetCellX, targetCellY);
            targetCell.isBurning = false;
            targetCell.isBurnt = false;
            targetCell.hasTreesUprooted = false;
            targetCell.waterDepth = 0.0f;
            targetCell.flowVelocityX = 0.0f;
            targetCell.flowVelocityY = 0.0f;
            targetCell.isFlooded = false;

            if (deltaOffsetX <= -7) {
                // EN: Phase 4: Deep Sea / Abyssal trench
                // ES: Fase 4: Mar Profundo / Abisal
                targetCell.biome = ReligionSim::BiomeType::DeepOcean;
                targetCell.waterPhase = ReligionSim::WaterDepthPhase::DeepAbyssal;
                targetCell.elevation = 0.0f;
                targetCell.climate.elevation = -0.85f;
            } else if (deltaOffsetX >= -6 && deltaOffsetX <= -4) {
                // EN: Phase 3: Mid Sea / Ocean
                // ES: Fase 3: Mar Medio / Océano
                targetCell.biome = ReligionSim::BiomeType::DeepOcean;
                targetCell.waterPhase = ReligionSim::WaterDepthPhase::MidOcean;
                targetCell.elevation = 0.0f;
                targetCell.climate.elevation = -0.55f;
            } else if (deltaOffsetX >= -3 && deltaOffsetX <= -1) {
                // EN: Phase 2: Shallow Waters / Coast
                // ES: Fase 2: Aguas Someras / Costa
                targetCell.biome = ReligionSim::BiomeType::ShallowWater;
                targetCell.waterPhase = ReligionSim::WaterDepthPhase::ShallowCoast;
                targetCell.elevation = 0.15f;
                targetCell.climate.elevation = -0.32f;
            } else if (deltaOffsetX >= 0 && deltaOffsetX <= 1) {
                // EN: Phase 1: Shoreline Foam interface
                // ES: Fase 1: Línea de Espuma / Orilla
                targetCell.biome = ReligionSim::BiomeType::ShallowWater;
                targetCell.waterPhase = ReligionSim::WaterDepthPhase::ShorelineFoam;
                targetCell.elevation = 0.28f;
                targetCell.climate.elevation = -0.18f;
            } else if (deltaOffsetX >= 2 && deltaOffsetX <= 3) {
                // EN: Sandy beach coastal fringe
                // ES: Franja costera de arena de playa
                targetCell.biome = ReligionSim::BiomeType::CoastalPlains;
                targetCell.elevation = 0.55f;
                targetCell.climate.elevation = 0.05f;
            } else {
                // EN: Inland fertile valley
                // ES: Valle fértil interior
                targetCell.biome = ReligionSim::BiomeType::FertileValley;
                targetCell.elevation = 0.85f;
                targetCell.climate.elevation = 0.25f;
            }
        }
    }

    // EN: Center camera on the transect with 1.25f zoom
    // ES: Centrar la cámara en el transecto con zoom 1.25f
    gameEngine.getCamera().setZoom(1.25f);
    gameEngine.getCamera().centerOnCell(centerCellX - 1, centerCellY, worldMap.getWidth(), worldMap.getHeight());

    // EN: Update simulation to advance water animation frame time
    // ES: Actualizar simulación para avanzar el tiempo de animación de agua
    gameEngine.update(1.50f);

    // EN: Render frame showcasing all 4 progressive water depth phases
    // ES: Renderizar cuadro exhibiendo las 4 fases progresivas de profundidad del agua
    gameEngine.render();

    const std::string exportPpmPath = "snapshot_water_depth_phases.ppm";
    gameEngine.getRenderer().exportFrameToPPM(exportPpmPath);
    std::cout << "[Snapshot] Water depth phases snapshot exported to " << exportPpmPath << std::endl;

    return 0;
}
