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

    // EN: Construct a dynamic terrain slope: high ridge down to river valley
    // ES: Construir una pendiente de terreno dinámica: cresta alta bajando hacia el valle
    for (int deltaOffsetY = -6; deltaOffsetY <= 6; ++deltaOffsetY) {
        for (int deltaOffsetX = -6; deltaOffsetX <= 6; ++deltaOffsetX) {
            const int currentCellX = centerCellX + deltaOffsetX;
            const int currentCellY = centerCellY + deltaOffsetY;
            if (!worldMap.isValid(currentCellX, currentCellY)) continue;

            ReligionSim::Cell& cellRef = worldMap.getCell(currentCellX, currentCellY);
            cellRef.isBurning = false;
            cellRef.isBurnt = false;
            cellRef.hasTreesUprooted = false;
            cellRef.waterDepth = 0.0f;
            cellRef.moisture = 0.35f;

            // River basin on west side
            if (deltaOffsetX <= -3) {
                cellRef.biome = ReligionSim::BiomeType::ShallowWater;
                cellRef.elevation = 0.40f;
            } else if (deltaOffsetX >= 2 && deltaOffsetY <= -2) {
                cellRef.biome = ReligionSim::BiomeType::HighlandHills;
                cellRef.elevation = 1.60f;
            } else if (deltaOffsetX >= 0 && deltaOffsetX <= 3 && deltaOffsetY >= 0 && deltaOffsetY <= 3) {
                cellRef.biome = ReligionSim::BiomeType::DenseForest;
                cellRef.naturalResource = ReligionSim::ResourceType::Wood;
                cellRef.resourceAmount = 150;
                cellRef.elevation = 1.0f;
            } else {
                cellRef.biome = ReligionSim::BiomeType::FertileValley;
                cellRef.elevation = 0.85f;
            }
        }
    }

    // EN: Place a thatched dwelling in the depression
    // ES: Colocar una choza de paja en la depresión
    ReligionSim::BuildingSystem& buildingSystem = gameEngine.getBuildingSystem();
    ReligionSim::House tribalHut;
    tribalHut.id = 201;
    tribalHut.cellX = centerCellX - 1;
    tribalHut.cellY = centerCellY + 1;
    tribalHut.type = ReligionSim::HouseType::ThatchedHut;
    tribalHut.state = ReligionSim::BuildingState::Completed;
    tribalHut.health = 80.0f;
    tribalHut.maxHealth = 100.0f;
    tribalHut.ownerId = 1;
    buildingSystem.getHouses().push_back(tribalHut);
    worldMap.getCell(tribalHut.cellX, tribalHut.cellY).hasHouse = true;

    // EN: Setup living units: shaman praising the heavens, villager wading through water
    // ES: Configurar unidades vivas: chamán orando a los cielos, aldeano vadeando agua
    ReligionSim::GestorHumanos& villagerManager = gameEngine.getGestorHumanos();
    villagerManager.getHumanos().clear();

    ReligionSim::Humano shamanLeader(1, "Chaman Pluvial", static_cast<float>(centerCellX) + 0.5f, static_cast<float>(centerCellY) + 0.5f, ReligionSim::HumanGender::Male);
    shamanLeader.setEsChaman(true);
    shamanLeader.activarSenalDivina(12.0f);
    villagerManager.addVillager(shamanLeader);

    ReligionSim::Humano wadingVillager(2, "Brea", static_cast<float>(centerCellX - 1) + 0.5f, static_cast<float>(centerCellY) + 0.5f, ReligionSim::HumanGender::Female);
    wadingVillager.setSwimming(true);
    villagerManager.addVillager(wadingVillager);

    // EN: Configure water runoff on slope and floodwaters in the valley
    // ES: Configurar escorrentía de agua en ladera e inundación en el valle
    worldMap.addWaterToCell(centerCellX - 1, centerCellY, 0.40f);
    worldMap.addWaterToCell(centerCellX, centerCellY, 0.28f);
    worldMap.addWaterToCell(centerCellX + 1, centerCellY, 0.18f);
    worldMap.addWaterToCell(centerCellX - 2, centerCellY + 1, 0.35f);
    worldMap.addWaterToCell(centerCellX - 1, centerCellY + 1, 0.22f);

    // EN: Cell where fire was quenched producing billowing steam
    // ES: Celda donde el fuego fue apagado produciendo vapor denso
    ReligionSim::Cell& steamCell = worldMap.getCell(centerCellX + 1, centerCellY + 1);
    steamCell.isBurning = false;
    steamCell.isBurnt = true;
    steamCell.steamTimer = 2.4f;
    steamCell.moisture = 1.0f;
    steamCell.waterDepth = 0.12f;

    // EN: Active fire at forest edge that water is approaching
    // ES: Fuego activo en el borde del bosque hacia donde fluye el agua
    worldMap.igniteCell(centerCellX + 2, centerCellY + 2, 12.0f, 0.85f);

    // EN: Cast Great Deluge power over the center to trigger atmospheric rain streaks and divine rings
    // ES: Lanzar Gran Diluvio sobre el centro para activar cortinas de lluvia y anillos divinos
    gameEngine.castDivinePower(ReligionSim::DivinePowerType::Deluge, centerCellX, centerCellY);

    // EN: Advance game physics and animation time
    // ES: Avanzar físicas del juego y tiempo de animación
    gameEngine.getCloudSystem().setWind(0.35f, 0.10f);
    gameEngine.update(0.15f);

    // EN: Position camera over the inundated scene
    // ES: Posicionar la cámara sobre la escena inundada
    gameEngine.getRenderer().setFogOfWar(nullptr);
    gameEngine.getCamera().setZoom(1.35f);
    gameEngine.getCamera().centerOnCell(centerCellX, centerCellY, worldMap.getWidth(), worldMap.getHeight());
    for (int frameIndex = 0; frameIndex < 20; ++frameIndex) {
        gameEngine.getCamera().update(0.1f);
    }

    gameEngine.render();

    const std::string exportPpmPath = "snapshot_water_physics.ppm";
    gameEngine.getRenderer().exportFrameToPPM(exportPpmPath);
    std::cout << "[Snapshot] Water, rain, deluge, and steam snapshot exported to " << exportPpmPath << std::endl;
    return 0;
}
