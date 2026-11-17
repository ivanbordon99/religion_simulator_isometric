#include "Game.hpp"
#include "BuildingSystem.hpp"
#include "DivinePowersSystem.hpp"
#include <iostream>

int main() {
    // EN: Initialize game engine with 1280x720 canvas
    // ES: Inicializar motor de juego con resolución 1280x720
    ReligionSim::Game gameEngine(1280, 720);
    gameEngine.init();
    gameEngine.startNewCustomWorld();
    gameEngine.setGameState(ReligionSim::GameState::Playing);

    // EN: Activate LUMINUS cheat to fully reveal the world map and remove fog of war
    // ES: Activar truco LUMINUS para revelar completamente el mapa del mundo y remover niebla de guerra
    gameEngine.runCommand("LUMINUS");

    ReligionSim::WorldMap& worldMap = gameEngine.getMap();
    std::vector<ReligionSim::Point2i> pueblos = worldMap.generatePuebloLocations(5, 3478625906);
    const int centerCellX = pueblos.empty() ? 64 : pueblos[0].x;
    const int centerCellY = pueblos.empty() ? 64 : pueblos[0].y;
    std::cout << "[FluidSnapshot] Hearth at: (" << centerCellX << ", " << centerCellY << ")" << std::endl;

    // EN: Clear a 16x16 area around center into a gentle fertile valley
    // ES: Limpiar un área de 16x16 alrededor del centro como un valle fértil
    for (int deltaOffsetY = -8; deltaOffsetY <= 8; ++deltaOffsetY) {
        for (int deltaOffsetX = -8; deltaOffsetX <= 8; ++deltaOffsetX) {
            const int currentCellX = centerCellX + deltaOffsetX;
            const int currentCellY = centerCellY + deltaOffsetY;
            if (!worldMap.isValid(currentCellX, currentCellY)) continue;

            ReligionSim::Cell& cellRef = worldMap.getCell(currentCellX, currentCellY);
            cellRef.isBurning = false;
            cellRef.isBurnt = false;
            cellRef.hasTreesUprooted = false;
            cellRef.elevation = 0.50f;
            cellRef.biome = ReligionSim::BiomeType::FertileValley;
            cellRef.waterDepth = 0.0f;
            cellRef.flowVelocityX = 0.0f;
            cellRef.flowVelocityY = 0.0f;
            cellRef.moisture = 0.45f;
            cellRef.naturalResource = ReligionSim::ResourceType::None;
            cellRef.resourceAmount = 0;
            cellRef.hasHouse = false;
        }
    }

    // EN: Sculpt completely contiguous river channel flowing South-East across the screen
    // ES: Esculpir canal fluvial completamente continuo fluyendo al Sureste a través de la pantalla
    for (int stepX = -7; stepX <= 7; ++stepX) {
        for (int widthY = -1; widthY <= 1; ++widthY) {
            const int riverCellX = centerCellX + stepX;
            const int riverCellY = centerCellY + widthY;
            if (!worldMap.isValid(riverCellX, riverCellY)) continue;

            ReligionSim::Cell& riverCell = worldMap.getCell(riverCellX, riverCellY);
            riverCell.elevation = 0.35f;
            riverCell.biome = ReligionSim::BiomeType::FertileValley;
            riverCell.waterDepth = (widthY == 0) ? 0.52f : 0.38f;
            riverCell.isFlooded = true;
            // Strong directional fluid current vector flowing downstream (+X direction)
            riverCell.flowVelocityX = 1.35f;
            riverCell.flowVelocityY = 0.0f;
            riverCell.moisture = 1.0f;
        }
    }

    // Moist river banks
    for (int stepX = -7; stepX <= 7; ++stepX) {
        for (int bankSide : {-2, 2}) {
            const int bankCellX = centerCellX + stepX;
            const int bankCellY = centerCellY + bankSide;
            if (worldMap.isValid(bankCellX, bankCellY)) {
                worldMap.getCell(bankCellX, bankCellY).moisture = 0.85f;
            }
        }
    }

    // EN: Place thatched tribal dwellings on the fertile riverbank
    // ES: Colocar viviendas tribales de paja en la orilla del río
    ReligionSim::BuildingSystem& buildingSystem = gameEngine.getBuildingSystem();
    buildingSystem.getHouses().clear();

    ReligionSim::House tribalHut1;
    tribalHut1.id = 301;
    tribalHut1.cellX = centerCellX + 2;
    tribalHut1.cellY = centerCellY - 2;
    tribalHut1.type = ReligionSim::HouseType::ThatchedHut;
    tribalHut1.state = ReligionSim::BuildingState::Completed;
    tribalHut1.health = 95.0f;
    tribalHut1.maxHealth = 100.0f;
    tribalHut1.ownerId = 1;
    buildingSystem.getHouses().push_back(tribalHut1);
    worldMap.getCell(tribalHut1.cellX, tribalHut1.cellY).hasHouse = true;

    // Forest trees along northern bank
    worldMap.getCell(centerCellX - 3, centerCellY - 2).naturalResource = ReligionSim::ResourceType::Wood;
    worldMap.getCell(centerCellX - 3, centerCellY - 2).resourceAmount = 100;
    worldMap.getCell(centerCellX - 4, centerCellY - 3).naturalResource = ReligionSim::ResourceType::Wood;
    worldMap.getCell(centerCellX - 4, centerCellY - 3).resourceAmount = 100;
    worldMap.getCell(centerCellX + 3, centerCellY + 2).naturalResource = ReligionSim::ResourceType::Wood;
    worldMap.getCell(centerCellX + 3, centerCellY + 2).resourceAmount = 100;

    // EN: Setup living units: swimming villager in river and wading shaman on bank
    // ES: Configurar unidades vivas: aldeano nadador en río y chamán vadeando en la orilla
    ReligionSim::GestorHumanos& villagerManager = gameEngine.getGestorHumanos();
    villagerManager.getHumanos().clear();

    ReligionSim::Humano swimmingVillager(1, "Nadador Fluvial", static_cast<float>(centerCellX) + 0.5f, static_cast<float>(centerCellY) + 0.5f, ReligionSim::HumanGender::Male);
    swimmingVillager.setSwimming(true);
    villagerManager.addVillager(swimmingVillager);

    ReligionSim::Humano shamanWader(2, "Chaman del Rio", static_cast<float>(centerCellX + 1) + 0.5f, static_cast<float>(centerCellY - 2) + 0.5f, ReligionSim::HumanGender::Male);
    shamanWader.setEsChaman(true);
    shamanWader.activarSenalDivina(15.0f);
    villagerManager.addVillager(shamanWader);

    // EN: Spawn floating physical debris in the river current
    // ES: Generar escombros físicos flotantes en la corriente del río
    ReligionSim::DivinePowersSystem& powersSystem = gameEngine.getDivinePowers();
    powersSystem.clearAllFX();

    powersSystem.spawnDebris(ReligionSim::DebrisType::UprootedTreeTrunk,
                             static_cast<float>(centerCellX - 2) + 0.5f, static_cast<float>(centerCellY) + 0.5f,
                             0.05f, 0.6f, 0.0f, 0.0f, 30.0f, 15.0f, 1.2f);
    powersSystem.spawnDebris(ReligionSim::DebrisType::ResourceCrate,
                             static_cast<float>(centerCellX + 2) + 0.3f, static_cast<float>(centerCellY) + 0.3f,
                             0.05f, 0.7f, 0.0f, 0.0f, 12.0f, 10.0f, 1.0f);
    powersSystem.spawnDebris(ReligionSim::DebrisType::RoofThatch,
                             static_cast<float>(centerCellX - 1) + 0.2f, static_cast<float>(centerCellY - 1) + 0.6f,
                             0.05f, 0.5f, 0.0f, 0.0f, 8.0f, 5.0f, 0.8f);

    // Update debris to set isFloating
    powersSystem.update(0.05f, worldMap, villagerManager, &buildingSystem, nullptr);

    // EN: Focus camera on the fluid mechanics demonstration scene
    // ES: Enfocar cámara en la escena de demostración de mecánica de fluidos
    gameEngine.getRenderer().setFogOfWar(nullptr);
    gameEngine.getCamera().setZoom(1.65f);
    for (int frameIndex = 0; frameIndex < 30; ++frameIndex) {
        gameEngine.getCamera().update(0.1f);
        gameEngine.getCamera().centerOnCell(centerCellX, centerCellY, worldMap.getWidth(), worldMap.getHeight());
    }

    gameEngine.render();

    const std::string exportPpmPath = "snapshot_fluid_mechanics.ppm";
    gameEngine.getRenderer().exportFrameToPPM(exportPpmPath);
    std::cout << "[Snapshot] Fluid mechanics snapshot exported to " << exportPpmPath << std::endl;
    return 0;
}
