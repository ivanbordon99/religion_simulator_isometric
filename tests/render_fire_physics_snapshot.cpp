#include "Game.hpp"
#include "BuildingSystem.hpp"
#include <iostream>

int main() {
    // EN: Initialize game engine in 1280x720 window
    // ES: Inicializar motor de juego en ventana de 1280x720
    ReligionSim::Game gameEngine(1280, 720);
    gameEngine.init();
    gameEngine.startNewCustomWorld();
    gameEngine.setGameState(ReligionSim::GameState::Playing);

    ReligionSim::WorldMap& worldMap = gameEngine.getMap();
    constexpr int centerCellX = 25;
    constexpr int centerCellY = 25;

    // EN: Configure a forest clearing, thatched houses, and a nearby river
    // ES: Configurar un claro de bosque, chozas de paja y un río cercano
    for (int deltaOffsetY = -5; deltaOffsetY <= 5; ++deltaOffsetY) {
        for (int deltaOffsetX = -5; deltaOffsetX <= 5; ++deltaOffsetX) {
            const int currentCellX = centerCellX + deltaOffsetX;
            const int currentCellY = centerCellY + deltaOffsetY;
            if (!worldMap.isValid(currentCellX, currentCellY)) continue;

            ReligionSim::Cell& cellRef = worldMap.getCell(currentCellX, currentCellY);
            cellRef.elevation = 1.0f;
            cellRef.isBurning = false;
            cellRef.isBurnt = false;
            cellRef.hasTreesUprooted = false;

            // Nearby natural river canal at X = centerCellX - 3
            if (deltaOffsetX == -3) {
                cellRef.biome = ReligionSim::BiomeType::ShallowWater;
                cellRef.elevation = 0.5f;
            } else if (deltaOffsetX >= -1 && deltaOffsetX <= 3 && deltaOffsetY >= -2 && deltaOffsetY <= 2) {
                cellRef.biome = ReligionSim::BiomeType::DenseForest;
                cellRef.naturalResource = ReligionSim::ResourceType::Wood;
                cellRef.resourceAmount = 180;
                cellRef.maxResourceAmount = 200;
            } else {
                cellRef.biome = ReligionSim::BiomeType::FertileValley;
            }
        }
    }

    // EN: Place a thatched tribal dwelling at (centerCellX + 1, centerCellY)
    // ES: Colocar una choza tribal de paja en (centerCellX + 1, centerCellY)
    ReligionSim::BuildingSystem& buildingSystem = gameEngine.getBuildingSystem();
    ReligionSim::House tribalHut;
    tribalHut.id = 101;
    tribalHut.cellX = centerCellX + 1;
    tribalHut.cellY = centerCellY;
    tribalHut.type = ReligionSim::HouseType::ThatchedHut;
    tribalHut.state = ReligionSim::BuildingState::Completed;
    tribalHut.health = 75.0f;
    tribalHut.maxHealth = 100.0f;
    tribalHut.ownerId = 1;
    buildingSystem.getHouses().push_back(tribalHut);
    worldMap.getCell(tribalHut.cellX, tribalHut.cellY).hasHouse = true;

    // EN: Setup villagers: one running on fire, one running towards river, one shaman
    // ES: Configurar aldeanos: uno corriendo en llamas, otro huyendo hacia el río, un chamán
    ReligionSim::GestorHumanos& villagerManager = gameEngine.getGestorHumanos();
    villagerManager.getHumanos().clear();

    // Burning panicked villager
    ReligionSim::Humano burningVillager(1, "Grog", static_cast<float>(centerCellX) + 0.6f, static_cast<float>(centerCellY) + 0.4f, ReligionSim::HumanGender::Male);
    burningVillager.setSalud(82.0f);
    burningVillager.ignite(12.0f);
    burningVillager.fleeInPanicFrom(static_cast<float>(centerCellX), static_cast<float>(centerCellY), 6.0f);
    villagerManager.addVillager(burningVillager);

    // Villager near the river escaping the heat
    ReligionSim::Humano escapingVillager(2, "Tara", static_cast<float>(centerCellX - 2) + 0.3f, static_cast<float>(centerCellY) + 0.1f, ReligionSim::HumanGender::Female);
    escapingVillager.setSalud(95.0f);
    villagerManager.addVillager(escapingVillager);

    // Shaman chanting prayers
    ReligionSim::Humano shamanLeader(3, "Orok", static_cast<float>(centerCellX) + 0.2f, static_cast<float>(centerCellY + 2) + 0.2f, ReligionSim::HumanGender::Male);
    shamanLeader.setEsChaman(true);
    shamanLeader.setSalud(100.0f);
    villagerManager.addVillager(shamanLeader);

    // EN: Configure wind vector blowing towards east/southeast
    // ES: Configurar vector de viento soplando hacia el este/sureste
    gameEngine.getCloudSystem().setWind(0.55f, 0.15f);

    // EN: Ignite fire front at epicenter (centerCellX, centerCellY) and downwind
    // ES: Iniciar frente de fuego en el epicentro (centerCellX, centerCellY) y a favor del viento
    worldMap.igniteCell(centerCellX, centerCellY, 18.0f, 1.0f);
    worldMap.igniteCell(centerCellX + 1, centerCellY, 16.0f, 0.90f);

    // EN: Set a scorched ash cell to the rear (demonstrating fire dissipation and charcoal ground)
    // ES: Establecer una celda de ceniza calcinada atrás (mostrando disipación de fuego y suelo de carbón)
    ReligionSim::Cell& scorchedCell = worldMap.getCell(centerCellX - 1, centerCellY);
    scorchedCell.isBurning = false;
    scorchedCell.isBurnt = true;
    scorchedCell.ashRecoveryTimer = 45.0f;
    scorchedCell.hasTreesUprooted = true;
    scorchedCell.naturalResource = ReligionSim::ResourceType::None;

    // EN: Advance fire physics and game systems for dynamic particle motion
    // ES: Avanzar física del fuego y sistemas del juego para movimiento dinámico de partículas
    worldMap.updateFirePhysics(1.2f, &buildingSystem, &villagerManager, 0.55f, 0.15f);
    gameEngine.update(0.1f);

    // EN: Configure camera zoom and centering for optimal dramatic perspective
    // ES: Configurar zoom y centrado de cámara para perspectiva dramática óptima
    gameEngine.getRenderer().setFogOfWar(nullptr);
    gameEngine.getCamera().setZoom(1.40f);
    gameEngine.getCamera().centerOnCell(centerCellX, centerCellY, worldMap.getWidth(), worldMap.getHeight());
    for (int frameIndex = 0; frameIndex < 25; ++frameIndex) {
        gameEngine.getCamera().update(0.1f);
    }

    gameEngine.render();

    const std::string exportPpmPath = "snapshot_fire_physics.ppm";
    gameEngine.getRenderer().exportFrameToPPM(exportPpmPath);
    std::cout << "[Snapshot] Fire physics snapshot exported to " << exportPpmPath << std::endl;
    return 0;
}
