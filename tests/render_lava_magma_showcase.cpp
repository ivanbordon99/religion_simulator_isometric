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

    // EN: Setup volcanic terrain around center
    // ES: Configurar terreno volcánico alrededor del centro
    for (int deltaOffsetY = -8; deltaOffsetY <= 8; ++deltaOffsetY) {
        for (int deltaOffsetX = -8; deltaOffsetX <= 8; ++deltaOffsetX) {
            const int targetCellX = centerCellX + deltaOffsetX;
            const int targetCellY = centerCellY + deltaOffsetY;
            if (!worldMap.isValid(targetCellX, targetCellY)) continue;

            ReligionSim::Cell& targetCell = worldMap.getCell(targetCellX, targetCellY);
            targetCell.isBurning = false;
            targetCell.isBurnt = false;
            targetCell.hasTreesUprooted = false;
            targetCell.waterDepth = 0.0f;
            targetCell.isFlooded = false;
            targetCell.hasActiveLava = false;
            targetCell.hasVolcano = false;

            // Surrounding volcanic rock and ash plateau
            targetCell.biome = ReligionSim::BiomeType::RuggedMountains;
            targetCell.elevation = 1.0f;
        }
    }

    // EN: 1. Central Active Volcano Crater Vent (Permanent Superheated Molten Magma Reservoir)
    // ES: 1. Cráter volcánico central activo (Reservorio de magma fundido permanente)
    ReligionSim::Cell& volcanoCenterCell = worldMap.getCell(centerCellX, centerCellY);
    volcanoCenterCell.hasVolcano = true;
    volcanoCenterCell.hasActiveLava = true;
    volcanoCenterCell.lavaTimer = 45.0f;
    volcanoCenterCell.elevation = 1.8f;

    // EN: 2. Surrounding Caldera Pool (Phase 0: Molten Cellular Magma & Boiling Lava, Timer: 34.0s)
    // ES: 2. Cuenca de caldera circundante (Fase 0: Magma celular fundido y lava hirviente, Timer: 34.0s)
    constexpr int calderaCoords[4][2] = {
        {centerCellX + 1, centerCellY},
        {centerCellX - 1, centerCellY},
        {centerCellX, centerCellY + 1},
        {centerCellX, centerCellY - 1}
    };
    for (const auto& coordinatePair : calderaCoords) {
        ReligionSim::Cell& calderaCell = worldMap.getCell(coordinatePair[0], coordinatePair[1]);
        calderaCell.hasActiveLava = true;
        calderaCell.lavaTimer = 34.0f;
        calderaCell.elevation = 1.4f;
    }

    // EN: 3. South-East Lava River Flow (Phase 1: Boiling Lava into Semi-cooled Magma Crust, Timer: 20.0s)
    // ES: 3. Río de lava sureste (Fase 1: Lava hirviente hacia corteza de magma semicongelada, Timer: 20.0s)
    constexpr int riverPhase1Coords[3][2] = {
        {centerCellX + 1, centerCellY + 1},
        {centerCellX + 2, centerCellY + 1},
        {centerCellX + 2, centerCellY + 2}
    };
    for (const auto& coordinatePair : riverPhase1Coords) {
        ReligionSim::Cell& riverCell = worldMap.getCell(coordinatePair[0], coordinatePair[1]);
        riverCell.hasActiveLava = true;
        riverCell.lavaTimer = 20.0f;
        riverCell.elevation = 1.2f;
    }

    // EN: 4. Extended Lava Flow (Phase 2: Magma Crust into Solidifying Basalt Bedrock with Magma Veins, Timer: 8.0s)
    // ES: 4. Flujo extendido de lava (Fase 2: Corteza de magma hacia lecho basáltico con vetas de magma, Timer: 8.0s)
    constexpr int riverPhase2Coords[3][2] = {
        {centerCellX + 3, centerCellY + 2},
        {centerCellX + 3, centerCellY + 3},
        {centerCellX + 4, centerCellY + 3}
    };
    for (const auto& coordinatePair : riverPhase2Coords) {
        ReligionSim::Cell& riverCell = worldMap.getCell(coordinatePair[0], coordinatePair[1]);
        riverCell.hasActiveLava = true;
        riverCell.lavaTimer = 8.0f;
        riverCell.elevation = 1.1f;
    }

    // EN: 5. Terminal Lava Margin (Crystallizing Basalt Bedrock with residual veins, Timer: 2.0s)
    // ES: 5. Margen terminal de lava (Lecho basáltico cristalizante con vetas residuales, Timer: 2.0s)
    ReligionSim::Cell& terminalCell = worldMap.getCell(centerCellX + 5, centerCellY + 3);
    terminalCell.hasActiveLava = true;
    terminalCell.lavaTimer = 2.0f;
    terminalCell.elevation = 1.05f;

    // EN: Center camera on the volcanic lava system
    // ES: Centrar cámara en el sistema volcánico de lava
    ReligionSim::IsometricCamera& camera = gameEngine.getCamera();
    camera.setZoom(1.75f);
    camera.centerOnCell(centerCellX + 1, centerCellY + 1, worldMap.getWidth(), worldMap.getHeight());

    // EN: Advance simulation by a few frames for fluid convective animation
    // ES: Avanzar simulación por unos cuadros para animación fluida convectiva
    for (int frameIndex = 0; frameIndex < 15; ++frameIndex) {
        gameEngine.update(0.05f);
    }

    // EN: Render frame buffer
    // ES: Renderizar búfer de fotogramas
    gameEngine.render();

    // EN: Export PPM snapshot
    // ES: Exportar captura PPM
    const std::string ppmPath = "/home/Ivancho/.gemini/antigravity/brain/1dbff22e-5d97-4436-b0dc-e7a7cb63301c/snapshot_lava_magma_showcase.ppm";
    const bool saveSuccess = gameEngine.getRenderer().exportFrameToPPM(ppmPath);
    if (saveSuccess) {
        std::cout << "[SUCCESS] Lava & Magma showcase PPM snapshot exported: " << ppmPath << std::endl;
    } else {
        std::cerr << "[ERROR] Failed to export PPM snapshot!" << std::endl;
        return 1;
    }

    return 0;
}
