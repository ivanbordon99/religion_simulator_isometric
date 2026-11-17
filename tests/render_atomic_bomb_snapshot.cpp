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
    constexpr int hypocenterX = 25;
    constexpr int hypocenterY = 25;

    // EN: Populate surrounding area with forest, river, and settlement
    // ES: Poblar área circundante con bosque, río y asentamiento
    for (int deltaOffsetY = -8; deltaOffsetY <= 8; ++deltaOffsetY) {
        for (int deltaOffsetX = -8; deltaOffsetX <= 8; ++deltaOffsetX) {
            const int currentCellX = hypocenterX + deltaOffsetX;
            const int currentCellY = hypocenterY + deltaOffsetY;
            if (!worldMap.isValid(currentCellX, currentCellY)) continue;

            ReligionSim::Cell& cellRef = worldMap.getCell(currentCellX, currentCellY);
            cellRef.isBurning = false;
            cellRef.isBurnt = false;
            cellRef.hasTreesUprooted = false;
            cellRef.waterDepth = 0.0f;
            cellRef.moisture = 0.25f;

            if (deltaOffsetX >= 2 && deltaOffsetY <= -1) {
                cellRef.biome = ReligionSim::BiomeType::DenseForest;
                cellRef.naturalResource = ReligionSim::ResourceType::Wood;
                cellRef.resourceAmount = 140;
                cellRef.elevation = 1.0f;
            } else if (deltaOffsetX <= -4) {
                cellRef.biome = ReligionSim::BiomeType::ShallowWater;
                cellRef.elevation = 0.45f;
            } else {
                cellRef.biome = ReligionSim::BiomeType::FertileValley;
                cellRef.elevation = 0.90f;
            }
        }
    }

    // EN: Place tribal houses in the blast radius
    // ES: Colocar chozas tribales en el radio de impacto
    ReligionSim::BuildingSystem& buildingSystem = gameEngine.getBuildingSystem();
    ReligionSim::House house1;
    house1.id = 301;
    house1.cellX = hypocenterX + 3;
    house1.cellY = hypocenterY + 2;
    house1.type = ReligionSim::HouseType::ThatchedHut;
    house1.state = ReligionSim::BuildingState::Completed;
    house1.health = 100.0f;
    house1.maxHealth = 100.0f;
    buildingSystem.getHouses().push_back(house1);
    worldMap.getCell(house1.cellX, house1.cellY).hasHouse = true;

    ReligionSim::House house2;
    house2.id = 302;
    house2.cellX = hypocenterX - 2;
    house2.cellY = hypocenterY + 3;
    house2.type = ReligionSim::HouseType::BoneDwelling;
    house2.state = ReligionSim::BuildingState::Completed;
    house2.health = 120.0f;
    house2.maxHealth = 120.0f;
    buildingSystem.getHouses().push_back(house2);
    worldMap.getCell(house2.cellX, house2.cellY).hasHouse = true;

    // EN: Spawn villagers around the hypocenter
    // ES: Generar aldeanos alrededor del hipocentro
    ReligionSim::GestorHumanos& villagerManager = gameEngine.getGestorHumanos();
    villagerManager.getHumanos().clear();

    ReligionSim::Humano shamanUnit(1, "Chaman Atomo", static_cast<float>(hypocenterX) + 0.5f, static_cast<float>(hypocenterY) + 0.5f, ReligionSim::HumanGender::Male);
    shamanUnit.setEsChaman(true);
    shamanUnit.activarSenalDivina(15.0f);
    villagerManager.addVillager(shamanUnit);

    ReligionSim::Humano intermediateUnit(2, "Grog", static_cast<float>(hypocenterX + 4) + 0.5f, static_cast<float>(hypocenterY + 2) + 0.5f, ReligionSim::HumanGender::Male);
    villagerManager.addVillager(intermediateUnit);

    ReligionSim::Humano peripheralUnit(3, "Brea", static_cast<float>(hypocenterX - 6) + 0.5f, static_cast<float>(hypocenterY + 1) + 0.5f, ReligionSim::HumanGender::Female);
    villagerManager.addVillager(peripheralUnit);

    ReligionSim::Humano blastUnit1(4, "Korg", static_cast<float>(hypocenterX + 2) + 0.5f, static_cast<float>(hypocenterY - 1) + 0.5f, ReligionSim::HumanGender::Male);
    villagerManager.addVillager(blastUnit1);

    ReligionSim::Humano blastUnit2(5, "Tara", static_cast<float>(hypocenterX - 3) + 0.5f, static_cast<float>(hypocenterY - 2) + 0.5f, ReligionSim::HumanGender::Female);
    villagerManager.addVillager(blastUnit2);

    ReligionSim::Humano blastUnit3(6, "Vorn", static_cast<float>(hypocenterX + 1) + 0.5f, static_cast<float>(hypocenterY + 3) + 0.5f, ReligionSim::HumanGender::Male);
    villagerManager.addVillager(blastUnit3);

    // EN: Place a distant survivor so the religion remains active during the detonation
    // ES: Colocar un sobreviviente distante para que la fe continúe activa durante la detonación
    ReligionSim::Humano distantSurvivor(7, "Noe Sobreviviente", 8.5f, 8.5f, ReligionSim::HumanGender::Male);
    distantSurvivor.setReligionId(1);
    villagerManager.addVillager(distantSurvivor);

    // EN: Cast Atomic Bomb cheat power at hypocenter
    // ES: Lanzar truco de bomba atómica en el hipocentro
    gameEngine.castDivinePower(ReligionSim::DivinePowerType::AtomicBomb, hypocenterX, hypocenterY);

    // EN: Advance time to develop the mushroom cloud and supersonic shockwave
    // ES: Avanzar el tiempo para desarrollar el hongo atómico y la onda supersónica
    gameEngine.getCloudSystem().setWind(0.35f, 0.05f);
    for (int step = 0; step < 18; ++step) {
        gameEngine.update(0.10f); // 1.8 seconds total elapsed
    }

    // EN: Center camera over the cataclysmic nuclear explosion
    // ES: Centrar cámara sobre la cataclísmica explosión nuclear
    gameEngine.getRenderer().setFogOfWar(nullptr);
    gameEngine.getCamera().setZoom(1.15f);
    gameEngine.getCamera().centerOnCell(hypocenterX, hypocenterY, worldMap.getWidth(), worldMap.getHeight());
    for (int frameIndex = 0; frameIndex < 25; ++frameIndex) {
        gameEngine.getCamera().update(0.1f);
    }

    gameEngine.setGameState(ReligionSim::GameState::Playing);
    gameEngine.render();

    const std::string exportPpmPath = "snapshot_atomic_bomb.ppm";
    gameEngine.getRenderer().exportFrameToPPM(exportPpmPath);
    std::cout << "[Snapshot] Atomic bomb detonation snapshot exported to " << exportPpmPath << std::endl;
    return 0;
}
