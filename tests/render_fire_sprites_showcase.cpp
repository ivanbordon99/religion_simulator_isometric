#include "Game.hpp"
#include "BuildingSystem.hpp"
#include <iostream>

int main() {
    // EN: Initialize game engine in high-definition 1280x720 canvas
    // ES: Inicializar el motor del juego en un lienzo de alta definición 1280x720
    ReligionSim::Game gameEngine(1280, 720);
    gameEngine.init();
    gameEngine.startNewCustomWorld();
    gameEngine.setGameState(ReligionSim::GameState::Playing);

    ReligionSim::WorldMap& worldMap = gameEngine.getMap();
    constexpr int centerCellX = 25;
    constexpr int centerCellY = 25;

    // EN: Configure terrain: forest, fertile clearings, volcanic ridge and holy ritual grounds
    // ES: Configurar terreno: bosque, claros fértiles, cresta volcánica y terrenos rituales sagrados
    for (int deltaOffsetY = -6; deltaOffsetY <= 6; ++deltaOffsetY) {
        for (int deltaOffsetX = -6; deltaOffsetX <= 6; ++deltaOffsetX) {
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

            // Forest cluster on the right
            if (deltaOffsetX >= 0 && deltaOffsetX <= 3 && deltaOffsetY >= -2 && deltaOffsetY <= 2) {
                targetCell.biome = ReligionSim::BiomeType::DenseForest;
                targetCell.naturalResource = ReligionSim::ResourceType::Wood;
                targetCell.resourceAmount = 180;
                targetCell.maxResourceAmount = 200;
            } else if (deltaOffsetX <= -3) {
                // Volcanic basalt terrace with active lava pool and fire pit
                targetCell.biome = ReligionSim::BiomeType::RuggedMountains;
                targetCell.elevation = 1.35f;
            } else {
                targetCell.biome = ReligionSim::BiomeType::FertileValley;
            }
        }
    }

    // EN: Configure volcanic lava pool and crater vent on the basalt ridge
    // ES: Configurar piscina de lava volcánica y respiradero de cráter en la cresta de basalto
    ReligionSim::Cell& lavaCellA = worldMap.getCell(centerCellX - 3, centerCellY - 1);
    lavaCellA.hasActiveLava = true;
    lavaCellA.elevation = 1.2f;

    ReligionSim::Cell& lavaCellB = worldMap.getCell(centerCellX - 4, centerCellY);
    lavaCellB.hasActiveLava = true;
    lavaCellB.elevation = 1.2f;

    ReligionSim::Cell& firePitCell = worldMap.getCell(centerCellX - 3, centerCellY + 1);
    firePitCell.hasVolcano = true;
    firePitCell.elevation = 1.3f;

    // EN: Configure sacred tribal temple monument with campfire
    // ES: Configurar monumento sagrado de templo tribal con fogata
    ReligionSim::Cell& templeCell = worldMap.getCell(centerCellX - 1, centerCellY + 2);
    templeCell.hasHolySite = true;
    templeCell.shrineLevel = 1;
    templeCell.holySiteReligionId = 1;

    // EN: Place thatched tribal hut engulfed in flames
    // ES: Colocar choza tribal de paja envuelta en llamas
    ReligionSim::BuildingSystem& buildingSystem = gameEngine.getBuildingSystem();
    ReligionSim::House burningHut;
    burningHut.id = 101;
    burningHut.cellX = centerCellX + 1;
    burningHut.cellY = centerCellY - 1;
    burningHut.type = ReligionSim::HouseType::ThatchedHut;
    burningHut.state = ReligionSim::BuildingState::Completed;
    burningHut.health = 60.0f;
    burningHut.maxHealth = 100.0f;
    burningHut.ownerId = 1;
    buildingSystem.getHouses().push_back(burningHut);
    worldMap.getCell(burningHut.cellX, burningHut.cellY).hasHouse = true;

    // EN: Ignite forest wildfire front
    // ES: Iniciar frente de incendio forestal
    worldMap.igniteCell(centerCellX + 1, centerCellY - 1, 20.0f, 0.95f);
    worldMap.igniteCell(centerCellX + 2, centerCellY, 18.0f, 0.88f);
    worldMap.igniteCell(centerCellX + 1, centerCellY + 1, 15.0f, 0.65f);

    // Scorched ash cell showing dissipation
    ReligionSim::Cell& ashCell = worldMap.getCell(centerCellX + 3, centerCellY + 1);
    ashCell.isBurnt = true;
    ashCell.ashRecoveryTimer = 40.0f;
    ashCell.hasTreesUprooted = true;

    // EN: Configure villagers: running panicked villager on fire and praying shaman
    // ES: Configurar aldeanos: aldeano en pánico ardiendo en llamas y chamán orando
    ReligionSim::GestorHumanos& villagerManager = gameEngine.getGestorHumanos();
    villagerManager.getHumanos().clear();

    // Burning panicked villager
    ReligionSim::Humano burningVillager(1, "Grog en Llamas", static_cast<float>(centerCellX) + 0.4f, static_cast<float>(centerCellY) + 0.2f, ReligionSim::HumanGender::Male);
    burningVillager.setSalud(75.0f);
    burningVillager.ignite(15.0f);
    burningVillager.fleeInPanicFrom(static_cast<float>(centerCellX + 1), static_cast<float>(centerCellY), 6.0f);
    villagerManager.addVillager(burningVillager);

    // Devout villager at temple campfire
    ReligionSim::Humano devoutVillager(2, "Tara Devota", static_cast<float>(centerCellX - 1) + 0.3f, static_cast<float>(centerCellY + 2) + 0.5f, ReligionSim::HumanGender::Female);
    devoutVillager.setSalud(100.0f);
    villagerManager.addVillager(devoutVillager);

    // Tribal shaman casting blessings
    ReligionSim::Humano tribalShaman(3, "Gran Chaman", static_cast<float>(centerCellX - 2) + 0.2f, static_cast<float>(centerCellY + 1) + 0.3f, ReligionSim::HumanGender::Male);
    tribalShaman.setEsChaman(true);
    tribalShaman.setSalud(100.0f);
    villagerManager.addVillager(tribalShaman);

    // EN: Configure wind blowing gently towards East
    // ES: Configurar viento soplando suavemente hacia el Este
    gameEngine.getCloudSystem().setWind(0.45f, 0.10f);

    // EN: Update simulation for dynamic sprite frames and particle systems
    // ES: Actualizar simulación para fotogramas dinámicos de sprites y sistemas de partículas
    worldMap.updateFirePhysics(0.5f, &buildingSystem, &villagerManager, 0.45f, 0.10f);
    gameEngine.update(0.1f);

    // EN: Position camera focused on the fire display
    // ES: Posicionar la cámara enfocada en la exhibición del fuego
    gameEngine.getRenderer().setFogOfWar(nullptr);
    gameEngine.getCamera().setZoom(1.35f);
    gameEngine.getCamera().centerOnCell(centerCellX, centerCellY, worldMap.getWidth(), worldMap.getHeight());
    for (int frame = 0; frame < 30; ++frame) {
        gameEngine.getCamera().update(0.1f);
    }

    gameEngine.render();

    const std::string exportPpmPath = "snapshot_fire_sprites_showcase.ppm";
    gameEngine.getRenderer().exportFrameToPPM(exportPpmPath);
    std::cout << "[Snapshot] Fire sprites showcase snapshot exported to " << exportPpmPath << std::endl;
    return 0;
}
