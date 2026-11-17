#include "Game.hpp"
#include "BuildingSystem.hpp"
#include <iostream>

int main() {
    // EN: Initialize simulation engine in 1280x720 canvas
    // ES: Inicializar el motor de simulación en un lienzo de 1280x720
    ReligionSim::Game gameEngine(1280, 720);
    gameEngine.init();
    gameEngine.startNewCustomWorld();
    gameEngine.setGameState(ReligionSim::GameState::Playing);

    ReligionSim::WorldMap& worldMap = gameEngine.getMap();
    constexpr int centerCellX = 25;
    constexpr int centerCellY = 25;

    // EN: Configure terrain with basalt ridges, mountain peaks, and fertile valleys
    // ES: Configurar terreno con crestas de basalto, picos montañosos y valles fértiles
    for (int deltaOffsetY = -7; deltaOffsetY <= 7; ++deltaOffsetY) {
        for (int deltaOffsetX = -7; deltaOffsetX <= 7; ++deltaOffsetX) {
            const int targetCellX = centerCellX + deltaOffsetX;
            const int targetCellY = centerCellY + deltaOffsetY;
            if (!worldMap.isValid(targetCellX, targetCellY)) continue;

            ReligionSim::Cell& targetCell = worldMap.getCell(targetCellX, targetCellY);
            targetCell.elevation = 1.0f;
            targetCell.isBurning = false;
            targetCell.isBurnt = false;
            targetCell.hasTreesUprooted = false;
            targetCell.hasActiveLava = false;
            targetCell.hasVolcano = false;

            if (deltaOffsetX <= -2 && deltaOffsetY <= 0) {
                // Northern volcanic basalt plateau
                targetCell.biome = ReligionSim::BiomeType::RuggedMountains;
                targetCell.elevation = 1.45f;
            } else if (deltaOffsetX >= 1 && deltaOffsetY >= -1) {
                // Eastern volcanic caldera ridge
                targetCell.biome = ReligionSim::BiomeType::AridDesert;
                targetCell.elevation = 1.30f;
            } else {
                targetCell.biome = ReligionSim::BiomeType::FertileValley;
            }
        }
    }

    // EN: 1. Setup Active Explosive Volcano at (centerCellX + 1, centerCellY - 1)
    // ES: 1. Configurar volcán explosivo activo en (centerCellX + 1, centerCellY - 1)
    ReligionSim::Cell& eruptingVolcanoCell = worldMap.getCell(centerCellX + 1, centerCellY - 1);
    eruptingVolcanoCell.hasVolcano = true;
    eruptingVolcanoCell.hasActiveLava = true;
    eruptingVolcanoCell.lavaTimer = 0.45f; // Phase 2: Explosive blast & flying volcanic bombs
    eruptingVolcanoCell.elevation = 1.40f;

    // Surrounding lava flows around the erupting volcano
    worldMap.getCell(centerCellX + 2, centerCellY - 1).hasActiveLava = true;
    worldMap.getCell(centerCellX + 1, centerCellY).hasActiveLava = true;
    worldMap.getCell(centerCellX, centerCellY - 1).hasActiveLava = true;

    // EN: 2. Setup Dormant Stratovolcano at (centerCellX - 3, centerCellY - 2)
    // ES: 2. Configurar estratovolcán inactivo en (centerCellX - 3, centerCellY - 2)
    ReligionSim::Cell& dormantVolcanoCell = worldMap.getCell(centerCellX - 3, centerCellY - 2);
    dormantVolcanoCell.hasVolcano = true;
    dormantVolcanoCell.hasActiveLava = false;
    dormantVolcanoCell.elevation = 1.55f;

    // EN: 3. Setup Lava Flow River Volcano at (centerCellX - 1, centerCellY + 3)
    // ES: 3. Configurar volcán de ríos de lava en (centerCellX - 1, centerCellY + 3)
    ReligionSim::Cell& lavaRiverVolcanoCell = worldMap.getCell(centerCellX - 1, centerCellY + 3);
    lavaRiverVolcanoCell.hasVolcano = true;
    lavaRiverVolcanoCell.hasActiveLava = true;
    lavaRiverVolcanoCell.lavaTimer = 0.88f; // Phase 4: Cascading lava rivers
    lavaRiverVolcanoCell.elevation = 1.25f;

    worldMap.getCell(centerCellX, centerCellY + 3).hasActiveLava = true;
    worldMap.getCell(centerCellX - 1, centerCellY + 4).hasActiveLava = true;

    // EN: Setup tribal shrine with holy campfire honoring the Fire Spirit
    // ES: Configurar santuario tribal con fogata sagrada en honor al Espíritu del Fuego
    ReligionSim::Cell& shrineCell = worldMap.getCell(centerCellX - 3, centerCellY + 1);
    shrineCell.hasHolySite = true;
    shrineCell.shrineLevel = 1;
    shrineCell.holySiteReligionId = 1;

    // EN: Setup villagers: shaman conducting ritual and panicked villager
    // ES: Configurar aldeanos: chamán dirigiendo ritual y aldeano en pánico
    ReligionSim::GestorHumanos& villagerManager = gameEngine.getGestorHumanos();
    villagerManager.getHumanos().clear();

    ReligionSim::Humano fireShaman(1, "Chaman del Fuego", static_cast<float>(centerCellX - 3) + 0.3f, static_cast<float>(centerCellY + 1) + 0.4f, ReligionSim::HumanGender::Male);
    fireShaman.setEsChaman(true);
    fireShaman.setSalud(100.0f);
    villagerManager.addVillager(fireShaman);

    ReligionSim::Humano observantVillager(2, "Testigo Asombrado", static_cast<float>(centerCellX - 2) + 0.2f, static_cast<float>(centerCellY) + 0.5f, ReligionSim::HumanGender::Female);
    observantVillager.setSalud(100.0f);
    villagerManager.addVillager(observantVillager);

    ReligionSim::Humano panickedVillager(3, "Aldeano en Fuga", static_cast<float>(centerCellX + 3) + 0.4f, static_cast<float>(centerCellY + 2) + 0.3f, ReligionSim::HumanGender::Male);
    panickedVillager.fleeInPanicFrom(static_cast<float>(centerCellX + 1), static_cast<float>(centerCellY - 1), 8.0f);
    panickedVillager.setSalud(90.0f);
    villagerManager.addVillager(panickedVillager);

    // EN: Configure gentle ambient wind
    // ES: Configurar viento suave ambiental
    gameEngine.getCloudSystem().setWind(0.35f, 0.08f);

    // EN: Advance game systems for dynamic particle motion
    // ES: Avanzar sistemas para movimiento dinámico de partículas
    gameEngine.update(0.1f);

    // EN: Position camera centered on the volcanic complex
    // ES: Posicionar cámara centrada en el complejo volcánico
    gameEngine.getRenderer().setFogOfWar(nullptr);
    gameEngine.getCamera().setZoom(1.25f);
    gameEngine.getCamera().centerOnCell(centerCellX, centerCellY, worldMap.getWidth(), worldMap.getHeight());
    for (int frameIndex = 0; frameIndex < 30; ++frameIndex) {
        gameEngine.getCamera().update(0.1f);
    }

    gameEngine.render();

    const std::string exportPpmPath = "snapshot_volcano_showcase.ppm";
    gameEngine.getRenderer().exportFrameToPPM(exportPpmPath);
    std::cout << "[Snapshot] Volcano showcase snapshot exported to " << exportPpmPath << std::endl;
    return 0;
}
