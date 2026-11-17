// EN: Comprehensive particles catalog generator for all engine-rendered particle systems
// ES: Generador exhaustivo de catálogo de partículas para todos los sistemas de partículas renderizados por el motor
#include "Game.hpp"
#include "BuildingSystem.hpp"
#include <iostream>
#include <vector>
#include <string>

namespace ReligionSim {

// EN: Helper routine to reset a terrain area with uniform elevation and biome
// ES: Rutina auxiliar para restablecer un área de terreno con elevación y bioma uniformes
void resetShowcaseTerrain(WorldMap& worldMap, int centerCellX, int centerCellY,
                          BiomeType baseBiome = BiomeType::FertileValley, float baseElevation = 1.0f) {
    constexpr int HALF_SPAN = 8;
    for (int deltaY = -HALF_SPAN; deltaY <= HALF_SPAN; ++deltaY) {
        for (int deltaX = -HALF_SPAN; deltaX <= HALF_SPAN; ++deltaX) {
            const int targetX = centerCellX + deltaX;
            const int targetY = centerCellY + deltaY;
            if (!worldMap.isValid(targetX, targetY)) continue;

            Cell& cell = worldMap.getCell(targetX, targetY);
            cell.biome = baseBiome;
            cell.elevation = baseElevation;
            cell.naturalResource = ResourceType::None;
            cell.resourceAmount = 0;
            cell.maxResourceAmount = 0;
            cell.hasHouse = false;
            cell.hasHolySite = false;
            cell.shrineLevel = 0;
            cell.isBurning = false;
            cell.isBurnt = false;
            cell.hasTreesUprooted = false;
            cell.hasImpactCrater = false;
            cell.waterDepth = 0.0f;
            cell.isFlooded = false;
            cell.hasActiveLava = false;
            cell.hasVolcano = false;
            cell.heatAccumulator = 0.0f;
            cell.steamTimer = 0.0f;
        }
    }
}

// EN: Helper routine to position camera smoothly on target coordinates
// ES: Rutina auxiliar para posicionar la cámara suavemente en las coordenadas objetivo
void positionShowcaseCamera(Game& gameEngine, int centerCellX, int centerCellY, float zoomFactor) {
    gameEngine.getCamera().setZoom(zoomFactor);
    gameEngine.getCamera().centerOnCell(centerCellX, centerCellY,
                                       gameEngine.getMap().getWidth(), gameEngine.getMap().getHeight());
    for (int frameIndex = 0; frameIndex < 25; ++frameIndex) {
        gameEngine.getCamera().update(0.1f);
    }
}

// EN: Ensure immortal anchor villagers remain active to prevent Game Over during explosions
// ES: Asegurar que aldeanos ancla inmortales permanezcan activos para evitar Game Over durante explosiones
void spawnImmortalAnchor(GestorHumanos& villagerManager) {
    villagerManager.getHumanos().clear();
    for (int anchorIdx = 0; anchorIdx < 3; ++anchorIdx) {
        Humano anchor(900 + anchorIdx, "Ancla Inmortal",
                       2.5f + static_cast<float>(anchorIdx), 2.5f, HumanGender::Male);
        anchor.setReligionId(1);
        anchor.setSalud(999999.0f);
        villagerManager.addVillager(anchor);
    }
}

} // namespace ReligionSim

int main() {
    std::cout << "========================================================\n"
              << "       RELIGION SIMULATOR - PARTICLES SHOWCASE RENDERER \n"
              << "========================================================\n";

    constexpr int CANVAS_WIDTH = 1280;
    constexpr int CANVAS_HEIGHT = 720;
    ReligionSim::Game gameEngine(CANVAS_WIDTH, CANVAS_HEIGHT);
    gameEngine.init();
    gameEngine.startNewCustomWorld();
    gameEngine.setGameState(ReligionSim::GameState::Playing);
    gameEngine.runCommand("LUMINUS");
    gameEngine.runCommand("MEDIODIA");
    gameEngine.getRenderer().setFogOfWar(nullptr);

    ReligionSim::WorldMap& worldMap = gameEngine.getMap();
    ReligionSim::DivinePowersSystem& powersSystem = gameEngine.getDivinePowers();
    ReligionSim::GestorHumanos& villagerManager = gameEngine.getGestorHumanos();
    constexpr int centerCellX = 32;
    constexpr int centerCellY = 32;

    // -------------------------------------------------------------------------
    // 1. PARTICLES: FIRE EMBERS, BILLOWING SMOKE & STEAM PUFFS
    // -------------------------------------------------------------------------
    std::cout << "[1/10] Rendering Fire Embers, Billowing Smoke & Steam...\n";
    ReligionSim::resetShowcaseTerrain(worldMap, centerCellX, centerCellY, ReligionSim::BiomeType::FertileValley, 1.0f);
    powersSystem.clearAllFX();
    ReligionSim::spawnImmortalAnchor(villagerManager);

    // Forest trees burning vigorously
    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            worldMap.getCell(centerCellX + dx, centerCellY + dy).isBurning = true;
            worldMap.getCell(centerCellX + dx, centerCellY + dy).heatAccumulator = 1.0f;
            worldMap.getCell(centerCellX + dx, centerCellY + dy).biome = ReligionSim::BiomeType::DenseForest;
            worldMap.getCell(centerCellX + dx, centerCellY + dy).naturalResource = ReligionSim::ResourceType::Wood;
            worldMap.getCell(centerCellX + dx, centerCellY + dy).resourceAmount = 150;
        }
    }

    // Quenched cells generating billowing steam puffs
    worldMap.getCell(centerCellX + 2, centerCellY).steamTimer = 2.4f;
    worldMap.getCell(centerCellX - 2, centerCellY + 1).steamTimer = 2.0f;

    gameEngine.getCloudSystem().setWind(0.40f, 0.12f);
    ReligionSim::positionShowcaseCamera(gameEngine, centerCellX, centerCellY, 1.50f);
    for (int step = 0; step < 10; ++step) gameEngine.update(0.1f);
    gameEngine.setGameState(ReligionSim::GameState::Playing);
    gameEngine.render();
    gameEngine.getRenderer().exportFrameToPPM("ppm_01_fire_embers_smoke.ppm");

    // -------------------------------------------------------------------------
    // 2. PARTICLES: ATOMIC NUCLEAR FALLOUT, DUST MOTES & IONIZING SPARKS
    // -------------------------------------------------------------------------
    std::cout << "[2/10] Rendering Atomic Fallout, Dust Motes & Ionizing Radiation Sparks...\n";
    ReligionSim::resetShowcaseTerrain(worldMap, centerCellX, centerCellY, ReligionSim::BiomeType::FertileValley, 1.0f);
    powersSystem.clearAllFX();
    ReligionSim::spawnImmortalAnchor(villagerManager);

    gameEngine.castDivinePower(ReligionSim::DivinePowerType::AtomicBomb, centerCellX, centerCellY);
    gameEngine.getCloudSystem().setWind(0.35f, 0.08f);
    for (int step = 0; step < 18; ++step) gameEngine.update(0.10f); // ~1.8 seconds elapsed
    ReligionSim::positionShowcaseCamera(gameEngine, centerCellX, centerCellY, 1.15f);
    gameEngine.setGameState(ReligionSim::GameState::Playing);
    gameEngine.render();
    gameEngine.getRenderer().exportFrameToPPM("ppm_02_atomic_fallout_sparks.ppm");

    // -------------------------------------------------------------------------
    // 3. PARTICLES: VOLCANO PLINIAN ASH, MAGMA JETS & PYROCLASTIC BOMBS
    // -------------------------------------------------------------------------
    std::cout << "[3/10] Rendering Volcano Plinian Ash, Magma Jets & Pyroclastic Bombs...\n";
    ReligionSim::resetShowcaseTerrain(worldMap, centerCellX, centerCellY, ReligionSim::BiomeType::RuggedMountains, 1.25f);
    powersSystem.clearAllFX();
    ReligionSim::spawnImmortalAnchor(villagerManager);

    gameEngine.castDivinePower(ReligionSim::DivinePowerType::Volcano, centerCellX, centerCellY);
    for (int step = 0; step < 12; ++step) gameEngine.update(0.10f);
    ReligionSim::positionShowcaseCamera(gameEngine, centerCellX, centerCellY, 1.25f);
    gameEngine.setGameState(ReligionSim::GameState::Playing);
    gameEngine.render();
    gameEngine.getRenderer().exportFrameToPPM("ppm_03_volcano_magma_ash.ppm");

    // -------------------------------------------------------------------------
    // 4. PARTICLES: METEOR BOW SHOCK PLASMA TAIL & IMPACT CRATER EJECTA
    // -------------------------------------------------------------------------
    std::cout << "[4/10] Rendering Meteor Bow Shock Plasma & Flame Tail...\n";
    ReligionSim::resetShowcaseTerrain(worldMap, centerCellX, centerCellY, ReligionSim::BiomeType::FertileValley, 1.0f);
    powersSystem.clearAllFX();
    ReligionSim::spawnImmortalAnchor(villagerManager);

    gameEngine.castDivinePower(ReligionSim::DivinePowerType::Meteor, centerCellX, centerCellY);
    for (int step = 0; step < 8; ++step) gameEngine.update(0.10f); // 0.8s descent
    ReligionSim::positionShowcaseCamera(gameEngine, centerCellX, centerCellY, 1.25f);
    gameEngine.setGameState(ReligionSim::GameState::Playing);
    gameEngine.render();
    gameEngine.getRenderer().exportFrameToPPM("ppm_04_meteor_plasma_shock.ppm");

    // -------------------------------------------------------------------------
    // 5. PARTICLES: CELESTIAL MIRACLE ASCENDING GOLDEN STARS & BEAMS
    // -------------------------------------------------------------------------
    std::cout << "[5/10] Rendering Celestial Miracle Golden Stars & Divine Beams...\n";
    ReligionSim::resetShowcaseTerrain(worldMap, centerCellX, centerCellY, ReligionSim::BiomeType::FertileValley, 1.0f);
    powersSystem.clearAllFX();
    ReligionSim::spawnImmortalAnchor(villagerManager);

    gameEngine.castDivinePower(ReligionSim::DivinePowerType::Miracle, centerCellX, centerCellY);
    for (int step = 0; step < 8; ++step) gameEngine.update(0.10f);
    ReligionSim::positionShowcaseCamera(gameEngine, centerCellX, centerCellY, 1.45f);
    gameEngine.setGameState(ReligionSim::GameState::Playing);
    gameEngine.render();
    gameEngine.getRenderer().exportFrameToPPM("ppm_05_miracle_celestial_stars.ppm");

    // -------------------------------------------------------------------------
    // 6. PARTICLES: TORNADO 2.5D ROTATING CYCLONE FUNNEL & DEBRIS
    // -------------------------------------------------------------------------
    std::cout << "[6/10] Rendering Tornado Cyclonic Dust & Orbiting Debris...\n";
    ReligionSim::resetShowcaseTerrain(worldMap, centerCellX, centerCellY, ReligionSim::BiomeType::FertileValley, 1.0f);
    powersSystem.clearAllFX();
    ReligionSim::spawnImmortalAnchor(villagerManager);

    gameEngine.castDivinePower(ReligionSim::DivinePowerType::Tornado, centerCellX, centerCellY);
    for (int step = 0; step < 10; ++step) gameEngine.update(0.10f);
    ReligionSim::positionShowcaseCamera(gameEngine, centerCellX, centerCellY, 1.35f);
    gameEngine.setGameState(ReligionSim::GameState::Playing);
    gameEngine.render();
    gameEngine.getRenderer().exportFrameToPPM("ppm_06_tornado_cyclonic_dust.ppm");

    // -------------------------------------------------------------------------
    // 7. PARTICLES: PANDEMIC & PLAGUE SWIRLING SPORES & TOXIC MIASMA DOME
    // -------------------------------------------------------------------------
    std::cout << "[7/10] Rendering Plague & Pandemic Swirling Toxic Spores...\n";
    ReligionSim::resetShowcaseTerrain(worldMap, centerCellX, centerCellY, ReligionSim::BiomeType::FertileValley, 1.0f);
    powersSystem.clearAllFX();
    ReligionSim::spawnImmortalAnchor(villagerManager);

    gameEngine.castDivinePower(ReligionSim::DivinePowerType::Plague, centerCellX - 1, centerCellY);
    gameEngine.castDivinePower(ReligionSim::DivinePowerType::Pandemic, centerCellX + 1, centerCellY);
    for (int step = 0; step < 10; ++step) gameEngine.update(0.10f);
    ReligionSim::positionShowcaseCamera(gameEngine, centerCellX, centerCellY, 1.40f);
    gameEngine.setGameState(ReligionSim::GameState::Playing);
    gameEngine.render();
    gameEngine.getRenderer().exportFrameToPPM("ppm_07_plague_toxic_spores.ppm");

    // -------------------------------------------------------------------------
    // 8. PARTICLES: RAINDROPS, DELUGE TORRENTS & WATER SPLASHES
    // -------------------------------------------------------------------------
    std::cout << "[8/10] Rendering Raindrops, Deluge Water Splashes & Vortex...\n";
    ReligionSim::resetShowcaseTerrain(worldMap, centerCellX, centerCellY, ReligionSim::BiomeType::ShallowWater, 0.40f);
    powersSystem.clearAllFX();
    ReligionSim::spawnImmortalAnchor(villagerManager);

    gameEngine.castDivinePower(ReligionSim::DivinePowerType::Deluge, centerCellX, centerCellY);
    for (int step = 0; step < 12; ++step) gameEngine.update(0.10f);
    ReligionSim::positionShowcaseCamera(gameEngine, centerCellX, centerCellY, 1.35f);
    gameEngine.setGameState(ReligionSim::GameState::Playing);
    gameEngine.render();
    gameEngine.getRenderer().exportFrameToPPM("ppm_08_rain_water_splash.ppm");

    // -------------------------------------------------------------------------
    // 9. PARTICLES: PHYSICAL 3D AIRBORNE DEBRIS WITH MOTION SPEED BLUR TRAILS
    // -------------------------------------------------------------------------
    std::cout << "[9/10] Rendering 3D Physical Airborne Debris with Motion Blur...\n";
    ReligionSim::resetShowcaseTerrain(worldMap, centerCellX, centerCellY, ReligionSim::BiomeType::FertileValley, 1.0f);
    powersSystem.clearAllFX();
    ReligionSim::spawnImmortalAnchor(villagerManager);

    powersSystem.spawnDebris(ReligionSim::DebrisType::UprootedTreeTrunk, centerCellX - 2.0f, centerCellY, 32.0f, 2.5f, 1.2f, 4.0f);
    powersSystem.spawnDebris(ReligionSim::DebrisType::RoofThatch,        centerCellX - 1.0f, centerCellY - 1.5f, 28.0f, -2.0f, 1.5f, 3.5f);
    powersSystem.spawnDebris(ReligionSim::DebrisType::HouseRubble,       centerCellX,        centerCellY - 1.0f, 24.0f, 1.8f, -2.2f, 2.0f);
    powersSystem.spawnDebris(ReligionSim::DebrisType::PyroclasticBomb,   centerCellX + 1.5f, centerCellY - 1.5f, 35.0f, -1.5f, -2.0f, 5.0f);
    powersSystem.spawnDebris(ReligionSim::DebrisType::ResourceCrate,     centerCellX - 1.5f, centerCellY + 1.5f, 20.0f, 2.2f, 1.8f, 1.5f);
    powersSystem.spawnDebris(ReligionSim::DebrisType::SoilChunk,         centerCellX,        centerCellY + 1.0f, 26.0f, -1.8f, 2.5f, 3.0f);
    powersSystem.spawnDebris(ReligionSim::DebrisType::RockDebris,        centerCellX + 1.8f, centerCellY + 1.0f, 30.0f, 2.0f, -1.5f, 4.0f);
    powersSystem.spawnDebris(ReligionSim::DebrisType::IncandescentSlag,  centerCellX + 0.5f, centerCellY, 34.0f, -2.4f, -1.2f, 4.5f);
    powersSystem.spawnDebris(ReligionSim::DebrisType::WaterSprayGeyser,  centerCellX,        centerCellY, 18.0f, 0.5f, 0.8f, 6.0f);

    ReligionSim::positionShowcaseCamera(gameEngine, centerCellX, centerCellY, 1.45f);
    gameEngine.setGameState(ReligionSim::GameState::Playing);
    gameEngine.render();
    gameEngine.getRenderer().exportFrameToPPM("ppm_09_physical_debris.ppm");

    // -------------------------------------------------------------------------
    // 10. PARTICLES: VILLAGER EXPRESSIONS, FIRE, RAGE SPARKS, SLEEPING zZ & FAITH
    // -------------------------------------------------------------------------
    std::cout << "[10/10] Rendering Villager Expressions: Rage, Sleep zZ, Burning Body & Shaman Aura...\n";
    ReligionSim::resetShowcaseTerrain(worldMap, centerCellX, centerCellY, ReligionSim::BiomeType::FertileValley, 1.0f);
    powersSystem.clearAllFX();
    ReligionSim::spawnImmortalAnchor(villagerManager);

    // 1. Shaman with golden divine blessing & prayer halo
    ReligionSim::Humano shaman(101, "Chaman Liturgico", static_cast<float>(centerCellX) + 0.2f, static_cast<float>(centerCellY) + 0.2f, ReligionSim::HumanGender::Male);
    shaman.setEsChaman(true);
    shaman.setReligionId(1);
    shaman.activarSenalDivina(15.0f);
    villagerManager.addVillager(shaman);

    // 2. Villager in Protective Rage with crimson flickering sparks
    ReligionSim::Humano warrior(102, "Guerrero Furia", static_cast<float>(centerCellX - 1) + 0.5f, static_cast<float>(centerCellY) + 0.4f, ReligionSim::HumanGender::Male);
    warrior.setReligionId(1);
    warrior.activateProtectiveFury(15.0f);
    villagerManager.addVillager(warrior);

    // 3. Sleeping Villager with floating undulating zZ particles
    ReligionSim::Humano sleeper(103, "Aldeano Durmiente", static_cast<float>(centerCellX + 1) + 0.5f, static_cast<float>(centerCellY) + 0.4f, ReligionSim::HumanGender::Female);
    sleeper.setReligionId(1);
    sleeper.setSleeping(true);
    sleeper.setSleepNeed(0.0f);
    villagerManager.addVillager(sleeper);

    // 4. Burning Villager fleeing in panic with body fire sparks & smoke trail
    ReligionSim::Humano burningUnit(104, "Victima Ignea", static_cast<float>(centerCellX) + 0.5f, static_cast<float>(centerCellY - 1) + 0.5f, ReligionSim::HumanGender::Male);
    burningUnit.setReligionId(1);
    burningUnit.ignite(10.0f);
    villagerManager.addVillager(burningUnit);

    for (int step = 0; step < 8; ++step) gameEngine.update(0.10f);
    ReligionSim::positionShowcaseCamera(gameEngine, centerCellX, centerCellY, 1.65f);
    gameEngine.setGameState(ReligionSim::GameState::Playing);
    gameEngine.render();
    gameEngine.getRenderer().exportFrameToPPM("ppm_10_villager_expressions.ppm");

    std::cout << "All 10 showcase scenes rendered successfully to PPM!\n";
    return 0;
}
