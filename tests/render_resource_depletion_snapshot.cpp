#include "Game.hpp"
#include "BuildingSystem.hpp"
#include <iostream>

int main() {
    // EN: Initialize game engine with 1280x720 canvas
    // ES: Inicializar motor de juego con resolución 1280x720
    ReligionSim::Game gameEngine(1280, 720);
    gameEngine.init();
    gameEngine.startNewCustomWorld();
    gameEngine.setGameState(ReligionSim::GameState::Playing);

    ReligionSim::WorldMap& worldMap = gameEngine.getMap();
    constexpr int centerCellX = 25;
    constexpr int centerCellY = 25;

    // EN: Setup a terrain sector with varied biomes and resource deposits
    // ES: Configurar un sector de terreno con biomas variados y depósitos de recursos
    for (int deltaOffsetY = -7; deltaOffsetY <= 7; ++deltaOffsetY) {
        for (int deltaOffsetX = -7; deltaOffsetX <= 7; ++deltaOffsetX) {
            const int currentCellX = centerCellX + deltaOffsetX;
            const int currentCellY = centerCellY + deltaOffsetY;
            if (!worldMap.isValid(currentCellX, currentCellY)) continue;

            ReligionSim::Cell& cellRef = worldMap.getCell(currentCellX, currentCellY);
            cellRef.isBurning = false;
            cellRef.isBurnt = false;
            cellRef.hasTreesUprooted = false;
            cellRef.waterDepth = 0.0f;
            cellRef.moisture = 0.25f;
            cellRef.elevation = 0.80f;
            cellRef.biome = ReligionSim::BiomeType::CoastalPlains;
            cellRef.naturalResource = ReligionSim::ResourceType::None;
            cellRef.resourceAmount = 0;
            cellRef.maxResourceAmount = 0;
        }
    }

    // EN: 1. Active rich timber forest (Wood 280 / 280)
    // ES: 1. Bosque activo con abundante madera (Madera 280 / 280)
    ReligionSim::Cell& richWoodCell = worldMap.getCell(centerCellX - 2, centerCellY - 1);
    richWoodCell.biome = ReligionSim::BiomeType::DenseForest;
    richWoodCell.naturalResource = ReligionSim::ResourceType::Wood;
    richWoodCell.resourceAmount = 280;
    richWoodCell.maxResourceAmount = 280;

    // EN: 2. Dwindling timber node (< 25% stock, triggers [Escaso] in inspector)
    // ES: 2. Depósito de madera en escasez (< 25% de reservas, activa [Escaso] en inspector)
    ReligionSim::Cell& scarceWoodCell = worldMap.getCell(centerCellX, centerCellY);
    scarceWoodCell.biome = ReligionSim::BiomeType::DenseForest;
    scarceWoodCell.naturalResource = ReligionSim::ResourceType::Wood;
    scarceWoodCell.resourceAmount = 35;
    scarceWoodCell.maxResourceAmount = 280;

    // EN: 3. Fully depleted timber cell: felled trees, cleared meadow, resourceAmount = 0
    // ES: 3. Celda de madera completamente agotada: árboles talados, pradera despejada, resourceAmount = 0
    ReligionSim::Cell& depletedWoodCell = worldMap.getCell(centerCellX + 2, centerCellY);
    depletedWoodCell.biome = ReligionSim::BiomeType::CoastalPlains;
    depletedWoodCell.naturalResource = ReligionSim::ResourceType::None;
    depletedWoodCell.resourceAmount = 0;
    depletedWoodCell.maxResourceAmount = 280;
    depletedWoodCell.hasTreesUprooted = true;

    // EN: 4. Gold mineral vein (Gold 160 / 160) in hills
    // ES: 4. Veta mineral de oro (Oro 160 / 160) en colinas
    ReligionSim::Cell& goldCell = worldMap.getCell(centerCellX - 2, centerCellY + 2);
    goldCell.biome = ReligionSim::BiomeType::HighlandHills;
    goldCell.elevation = 1.30f;
    goldCell.naturalResource = ReligionSim::ResourceType::Gold;
    goldCell.resourceAmount = 160;
    goldCell.maxResourceAmount = 160;

    // EN: 5. Exhausted stone quarry (Agotado 0 / 200)
    // ES: 5. Cantera de piedra agotada (Agotado 0 / 200)
    ReligionSim::Cell& exhaustedStoneCell = worldMap.getCell(centerCellX + 2, centerCellY + 2);
    exhaustedStoneCell.biome = ReligionSim::BiomeType::HighlandHills;
    exhaustedStoneCell.elevation = 1.25f;
    exhaustedStoneCell.naturalResource = ReligionSim::ResourceType::None;
    exhaustedStoneCell.resourceAmount = 0;
    exhaustedStoneCell.maxResourceAmount = 200;

    // EN: 6. Fertile farm field with wheat (Food 240 / 240)
    // ES: 6. Campo de cultivo fértil con trigo (Alimento 240 / 240)
    ReligionSim::Cell& foodCell = worldMap.getCell(centerCellX, centerCellY + 3);
    foodCell.biome = ReligionSim::BiomeType::FertileValley;
    foodCell.naturalResource = ReligionSim::ResourceType::Food;
    foodCell.resourceAmount = 240;
    foodCell.maxResourceAmount = 240;

    // EN: Setup living villagers performing logging, carrying cargo backpacks, and migrating
    // ES: Configurar aldeanos realizando tala, transportando fardos de carga y migrando
    ReligionSim::GestorHumanos& villagerManager = gameEngine.getGestorHumanos();
    villagerManager.getHumanos().clear();

    // Woodcutter actively chopping at the scarce node
    ReligionSim::Humano woodcutter(1, "Grog el Lenador", static_cast<float>(centerCellX) + 0.3f, static_cast<float>(centerCellY) + 0.5f, ReligionSim::HumanGender::Male);
    woodcutter.cambiarAccion(ReligionSim::HumanAction::TALAR);
    woodcutter.setCarriedResourceAmount(6);
    woodcutter.setGatheringResourceType(ReligionSim::ResourceType::Wood);
    villagerManager.addVillager(woodcutter);

    // Villager carrying gathered food to settlement
    ReligionSim::Humano carrier(2, "Tara Recolectora", static_cast<float>(centerCellX + 1) + 0.5f, static_cast<float>(centerCellY + 1) + 0.5f, ReligionSim::HumanGender::Female);
    carrier.cambiarAccion(ReligionSim::HumanAction::CAMINAR);
    carrier.setCarriedResourceAmount(10);
    carrier.setGatheringResourceType(ReligionSim::ResourceType::Food);
    villagerManager.addVillager(carrier);

    // Shaman blessing the tribe
    ReligionSim::Humano tribalShaman(3, "Gran Chaman", static_cast<float>(centerCellX - 1) + 0.5f, static_cast<float>(centerCellY + 1) + 0.5f, ReligionSim::HumanGender::Male);
    tribalShaman.setEsChaman(true);
    tribalShaman.cambiarAccion(ReligionSim::HumanAction::REZAR);
    villagerManager.addVillager(tribalShaman);

    // Select the scarce wood cell to inspect it in the bottom left inspector panel
    gameEngine.selectCell(centerCellX, centerCellY);

    // Position camera
    gameEngine.getRenderer().setFogOfWar(nullptr);
    gameEngine.getCamera().setZoom(1.40f);
    gameEngine.getCamera().centerOnCell(centerCellX, centerCellY, worldMap.getWidth(), worldMap.getHeight());
    for (int frameIndex = 0; frameIndex < 20; ++frameIndex) {
        gameEngine.getCamera().update(0.1f);
    }

    gameEngine.render();

    const std::string exportPpmPath = "snapshot_resource_depletion.ppm";
    gameEngine.getRenderer().exportFrameToPPM(exportPpmPath);
    std::cout << "[Snapshot] Resource depletion snapshot exported to " << exportPpmPath << std::endl;
    return 0;
}
