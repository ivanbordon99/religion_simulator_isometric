// =============================================================================
// EN: Snapshot verification for atomic explosion shockwave and airborne debris physics
// ES: Verificación visual de onda expansiva de explosión atómica y objetos por los aires
// =============================================================================

#include "Game.hpp"
#include <iostream>
#include <string>

int main() {
    using namespace ReligionSim;

    std::cout << "[Snapshot] Initializing headless game engine for atomic shockwave..." << std::endl;
    Game gameEngine(1280, 720);
    gameEngine.init();
    gameEngine.startNewCustomWorld();
    gameEngine.setGameState(GameState::Playing);
    gameEngine.runCommand("LUMINUS");  // EN: Clear fog of war / ES: Disipar niebla de guerra
    gameEngine.runCommand("MEDIODIA"); // EN: Full bright midday sunlight / ES: Plena luz solar

    WorldMap& worldMap = gameEngine.getMap();
    GestorHumanos& villagerManager = gameEngine.getGestorHumanos();
    BuildingSystem& buildingSystem = gameEngine.getBuildingSystem();

    // EN: Choose ground zero coordinates near world center
    // ES: Elegir coordenadas de zona cero cerca del centro del mundo
    const int groundZeroX = 35;
    const int groundZeroY = 35;

    // EN: Setup terrain: lush valley, trees, and small lake adjacent to ground zero
    // ES: Configurar terreno: valle fértil, árboles y pequeño lago adyacente a zona cero
    for (int deltaCellY = -9; deltaCellY <= 9; ++deltaCellY) {
        for (int deltaCellX = -9; deltaCellX <= 9; ++deltaCellX) {
            const int targetCoordX = groundZeroX + deltaCellX;
            const int targetCoordY = groundZeroY + deltaCellY;
            if (!worldMap.isValid(targetCoordX, targetCoordY)) continue;
            Cell& targetCell = worldMap.getCell(targetCoordX, targetCoordY);

            const float distance = std::hypot(static_cast<float>(deltaCellX), static_cast<float>(deltaCellY));
            if (deltaCellX <= -3 && deltaCellY >= 2 && distance <= 7.0f) {
                // Small adjacent lake
                targetCell.biome = BiomeType::ShallowWater;
                targetCell.waterDepth = 0.65f;
                targetCell.elevation = 0.95f;
                targetCell.naturalResource = ResourceType::None;
            } else if (distance >= 3.0f && distance <= 8.5f && (deltaCellX + deltaCellY) % 2 == 0) {
                // Standing forest
                targetCell.biome = BiomeType::DenseForest;
                targetCell.naturalResource = ResourceType::Wood;
                targetCell.resourceAmount = 160;
                targetCell.elevation = 1.30f;
            } else {
                targetCell.biome = BiomeType::FertileValley;
                targetCell.naturalResource = ResourceType::None;
                targetCell.elevation = 1.25f;
            }
            targetCell.hasTreesUprooted = false;
            targetCell.hasImpactCrater = false;
        }
    }

    // EN: Construct a couple of thatched dwellings in intermediate shockwave path
    // ES: Construir un par de viviendas en el paso intermedio de la onda expansiva
    const int houseId1 = buildingSystem.initiateHouseConstruction(groundZeroX + 4, groundZeroY - 2, 101, 102, 1, HouseType::ThatchedHut, worldMap);
    House* housePtr1 = buildingSystem.getHouseByIdMutable(houseId1);
    if (housePtr1) housePtr1->state = BuildingState::Completed;

    const int houseId2 = buildingSystem.initiateHouseConstruction(groundZeroX - 2, groundZeroY - 4, 103, 104, 1, HouseType::LargeRoundhouse, worldMap);
    House* housePtr2 = buildingSystem.getHouseByIdMutable(houseId2);
    if (housePtr2) housePtr2->state = BuildingState::Completed;

    // EN: Spawn several villagers in the path of the shockwave
    // ES: Generar varios aldeanos en el camino de la onda expansiva
    for (int villagerIndex = 0; villagerIndex < 10; ++villagerIndex) {
        const float angle = (static_cast<float>(villagerIndex) / 10.0f) * 6.28318f;
        const float radius = 2.0f + static_cast<float>(villagerIndex % 4) * 1.5f;
        const float spawnX = static_cast<float>(groundZeroX) + std::cos(angle) * radius;
        const float spawnY = static_cast<float>(groundZeroY) + std::sin(angle) * radius;
        Humano testVillager(villagerIndex + 1, "Aldeano Test " + std::to_string(villagerIndex), spawnX, spawnY);
        testVillager.setSalud(100.0f);
        villagerManager.agregarHumano(testVillager);
    }

    // EN: Center camera on ground zero and settle smoothly with panoramic headroom for mushroom cloud
    // ES: Centrar cámara en la zona cero y estabilizar suavemente con margen vertical para el hongo nuclear
    gameEngine.getCamera().setZoom(0.95f);
    gameEngine.getCamera().centerOnCell(groundZeroX, groundZeroY, worldMap.getWidth(), worldMap.getHeight());
    for (int step = 0; step < 40; ++step) {
        gameEngine.getCamera().update(0.1f);
    }
    // EN: Offset slightly downwards so the towering mushroom cloud is completely in frame
    // ES: Desplazar ligeramente hacia abajo para que el colosal hongo quede completamente encuadrado
    gameEngine.getCamera().pan(0, 70);
    gameEngine.getCamera().update(0.1f);

    // EN: Cast Atomic Bomb [FUCKINGBOMB] at ground zero
    // ES: Lanzar Bomba Atómica [FUCKINGBOMB] en la zona cero
    std::cout << "[Snapshot] Detonating atomic bomb at (" << groundZeroX << ", " << groundZeroY << ")..." << std::endl;
    gameEngine.castDivinePower(DivinePowerType::AtomicBomb, groundZeroX, groundZeroY);

    // EN: 1. Advance to t = 0.25s to capture initial crater excavation and airborne debris salvo
    // ES: 1. Avanzar a t = 0.25s para capturar excavación inicial del cráter y salva de escombros por el aire
    for (int step = 0; step < 5; ++step) {
        gameEngine.update(0.05f);
    }
    gameEngine.render();
    const std::string ejectaPpmPath = "snapshot_atomic_ejecta.ppm";
    gameEngine.getRenderer().exportFrameToPPM(ejectaPpmPath);
    std::cout << "[Snapshot] Frame 1 (Airborne Debris Salvo t=0.25s) exported to " << ejectaPpmPath << std::endl;

    // EN: 2. Advance to t = 0.70s to capture supersonic shockwave sweeping terrain and launching objects
    // ES: 2. Avanzar a t = 0.70s para capturar onda expansiva supersónica barriendo el terreno y lanzando objetos
    for (int step = 0; step < 9; ++step) {
        gameEngine.update(0.05f);
    }
    gameEngine.render();
    const std::string shockwavePpmPath = "snapshot_atomic_shockwave.ppm";
    gameEngine.getRenderer().exportFrameToPPM(shockwavePpmPath);
    std::cout << "[Snapshot] Frame 2 (Supersonic Shockwave & Dynamic Flight t=0.70s) exported to " << shockwavePpmPath << std::endl;

    // EN: 3. Advance to t = 1.35s to capture wide Mach stem front and colossal ground base surge
    // ES: 3. Avanzar a t = 1.35s para capturar frente de onda de Mach extendido y oleada de base de polvo
    for (int step = 0; step < 13; ++step) {
        gameEngine.update(0.05f);
    }
    gameEngine.render();
    const std::string wideWavePpmPath = "snapshot_atomic_widewave.ppm";
    gameEngine.getRenderer().exportFrameToPPM(wideWavePpmPath);
    std::cout << "[Snapshot] Frame 3 (Wide Mach Stem & Base Surge t=1.35s) exported to " << wideWavePpmPath << std::endl;

    // EN: 4. Advance to t = 3.20s to capture colossal mushroom cloud, vitrified crater, and fallout
    // ES: 4. Avanzar a t = 3.20s para capturar colosal hongo nuclear, cráter vitrificado y lluvia radiactiva
    for (int step = 0; step < 37; ++step) {
        gameEngine.update(0.05f);
    }
    gameEngine.render();
    const std::string aftermathPpmPath = "snapshot_atomic_aftermath.ppm";
    gameEngine.getRenderer().exportFrameToPPM(aftermathPpmPath);
    std::cout << "[Snapshot] Frame 4 (Nuclear Mushroom Cloud & Crater Aftermath t=3.20s) exported to " << aftermathPpmPath << std::endl;

    std::cout << "[Snapshot] All atomic explosion frames captured successfully!" << std::endl;
    return 0;
}
