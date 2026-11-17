#include "Game.hpp"
#include "BuildingSystem.hpp"
#include <iostream>
#include <cmath>

int main() {
    // EN: Initialize simulation engine in 1280x720 canvas
    // ES: Inicializar el motor de simulación en un lienzo de 1280x720
    ReligionSim::Game gameEngine(1280, 720);
    gameEngine.init();
    gameEngine.startNewCustomWorld();
    gameEngine.setGameState(ReligionSim::GameState::Playing);
    gameEngine.getRenderer().setFogOfWar(nullptr);

    ReligionSim::WorldMap& worldMap = gameEngine.getMap();
    constexpr int volcanoCenterCoordX = 26;
    constexpr int volcanoCenterCoordY = 26;

    // EN: 1. Setup authentic Volcanic Caldera biome with basalt rock, scorched flanks, and transition zones
    // ES: 1. Configurar auténtico bioma de Caldera Volcánica con roca basáltica, laderas calcinadas y zonas de transición
    for (int deltaOffsetY = -9; deltaOffsetY <= 9; ++deltaOffsetY) {
        for (int deltaOffsetX = -9; deltaOffsetX <= 9; ++deltaOffsetX) {
            const int targetCellX = volcanoCenterCoordX + deltaOffsetX;
            const int targetCellY = volcanoCenterCoordY + deltaOffsetY;
            if (!worldMap.isValid(targetCellX, targetCellY)) continue;

            const float distanceToCalderaCenter = std::sqrt(static_cast<float>(deltaOffsetX * deltaOffsetX + deltaOffsetY * deltaOffsetY));
            ReligionSim::Cell& targetCell = worldMap.getCell(targetCellX, targetCellY);
            targetCell.isBurning = false;
            targetCell.isBurnt = false;
            targetCell.hasTreesUprooted = false;
            targetCell.waterDepth = 0.0f;
            targetCell.isFlooded = false;
            targetCell.hasActiveLava = false;
            targetCell.hasVolcano = false;
            targetCell.naturalResource = ReligionSim::ResourceType::None;

            if (distanceToCalderaCenter <= 4.2f) {
                // EN: Caldera core and volcanic mountain crater
                // ES: Núcleo de la caldera y cráter montañoso volcánico
                targetCell.biome = ReligionSim::BiomeType::VolcanicCaldera;
                targetCell.elevation = 1.45f + (4.5f - distanceToCalderaCenter) * 0.20f;
                targetCell.isBurnt = true;
                targetCell.hasTreesUprooted = true;
            } else if (distanceToCalderaCenter <= 6.5f) {
                // EN: Scorched volcanic slopes and rocky scree
                // ES: Laderas volcánicas calcinadas y canchales rocosos
                targetCell.biome = ReligionSim::BiomeType::RuggedMountains;
                targetCell.elevation = 1.15f + (6.5f - distanceToCalderaCenter) * 0.12f;
                if (distanceToCalderaCenter <= 5.2f) {
                    targetCell.isBurnt = true;
                }
            } else {
                // EN: Foothills transitioning into fertile grassland valley
                // ES: Lomeríos en transición hacia el valle de pastizal fértil
                targetCell.biome = (deltaOffsetX + deltaOffsetY > 8) ? ReligionSim::BiomeType::FertileValley : ReligionSim::BiomeType::HighlandHills;
                targetCell.elevation = 0.65f;
            }
        }
    }

    // EN: 2. Central Active Volcano Crater Vent (Boiling Magma Caldera)
    // ES: 2. Chimenea y cráter del volcán central activo (Caldera de magma hirviente)
    ReligionSim::Cell& centralVolcanoCell = worldMap.getCell(volcanoCenterCoordX, volcanoCenterCoordY);
    centralVolcanoCell.biome = ReligionSim::BiomeType::VolcanicCaldera;
    centralVolcanoCell.hasVolcano = true;
    centralVolcanoCell.hasActiveLava = true;
    centralVolcanoCell.lavaTimer = 45.0f;
    centralVolcanoCell.elevation = 2.45f;
    centralVolcanoCell.isBurnt = true;

    // EN: 3. Surrounding Caldera Magma Pools & Active Lava Rivers (Phases 0 and 1)
    // ES: 3. Piscinas de magma de caldera circundantes y ríos de lava activa (Fases 0 y 1)
    constexpr std::array<std::pair<int, int>, 4> INNER_CALDERA_OFFSETS{{
        {1, 0}, {-1, 0}, {0, 1}, {0, -1}
    }};
    for (const auto& [deltaX, deltaY] : INNER_CALDERA_OFFSETS) {
        ReligionSim::Cell& calderaCell = worldMap.getCell(volcanoCenterCoordX + deltaX, volcanoCenterCoordY + deltaY);
        calderaCell.biome = ReligionSim::BiomeType::VolcanicCaldera;
        calderaCell.hasActiveLava = true;
        calderaCell.lavaTimer = 34.0f; // Boiling liquid magma
        calderaCell.elevation = 1.70f;
    }

    // EN: Dynamic river flows descending the flanks
    // ES: Flujos dinámicos de río de lava descendiendo por las laderas
    constexpr std::array<std::pair<int, int>, 4> LAVA_RIVER_OFFSETS{{
        {2, 0}, {2, 1}, {3, 1}, {1, 2}
    }};
    for (size_t flowIndex = 0; flowIndex < LAVA_RIVER_OFFSETS.size(); ++flowIndex) {
        const auto& [deltaX, deltaY] = LAVA_RIVER_OFFSETS[flowIndex];
        ReligionSim::Cell& riverCell = worldMap.getCell(volcanoCenterCoordX + deltaX, volcanoCenterCoordY + deltaY);
        riverCell.biome = ReligionSim::BiomeType::VolcanicCaldera;
        riverCell.hasActiveLava = true;
        riverCell.lavaTimer = 22.0f - static_cast<float>(flowIndex) * 4.0f; // Progressing downstream
        riverCell.elevation = 1.45f - static_cast<float>(flowIndex) * 0.08f;
    }

    // EN: 4. Geological Mineral Deposits (Kimberlite Diamonds, Gold, Copper, Basalt Obsidian)
    // ES: 4. Yacimientos minerales geológicos (Diamantes de kimberlita, oro, cobre, obsidiana basáltica)
    // Kimberlite Diamond pipe in deep caldera crag
    ReligionSim::Cell& diamondCell = worldMap.getCell(volcanoCenterCoordX - 2, volcanoCenterCoordY + 1);
    diamondCell.biome = ReligionSim::BiomeType::VolcanicCaldera;
    diamondCell.naturalResource = ReligionSim::ResourceType::Diamond;
    diamondCell.resourceAmount = 180;
    diamondCell.maxResourceAmount = 180;
    diamondCell.resourceVariant = 0; // Diamond Crystal

    // Hydrothermal Gold vein in volcanic flank
    ReligionSim::Cell& goldCell = worldMap.getCell(volcanoCenterCoordX - 1, volcanoCenterCoordY - 2);
    goldCell.biome = ReligionSim::BiomeType::VolcanicCaldera;
    goldCell.naturalResource = ReligionSim::ResourceType::Gold;
    goldCell.resourceAmount = 240;
    goldCell.maxResourceAmount = 240;
    goldCell.resourceVariant = 0; // Gold Vein

    // Copper outcrop on caldera rim
    ReligionSim::Cell& copperCell = worldMap.getCell(volcanoCenterCoordX - 2, volcanoCenterCoordY - 1);
    copperCell.biome = ReligionSim::BiomeType::VolcanicCaldera;
    copperCell.naturalResource = ReligionSim::ResourceType::Copper;
    copperCell.resourceAmount = 280;
    copperCell.maxResourceAmount = 280;
    copperCell.resourceVariant = 0; // Copper Vein

    // Basalt / Obsidian stone deposit
    ReligionSim::Cell& stoneCell = worldMap.getCell(volcanoCenterCoordX + 1, volcanoCenterCoordY - 2);
    stoneCell.biome = ReligionSim::BiomeType::VolcanicCaldera;
    stoneCell.naturalResource = ReligionSim::ResourceType::Stone;
    stoneCell.resourceAmount = 350;
    stoneCell.maxResourceAmount = 350;
    stoneCell.resourceVariant = 1; // Stone Boulder

    // EN: 5. Spawn sacred worship shaman and prospectors revering the volcanic deity
    // ES: 5. Generar chamán de adoración sagrada y exploradores venerando a la deidad volcánica
    ReligionSim::GestorHumanos& villagerManager = gameEngine.getGestorHumanos();
    villagerManager.getHumanos().clear();

    ReligionSim::Humano fireShaman(1, "Chaman del Magma", static_cast<float>(volcanoCenterCoordX - 3) + 0.5f, static_cast<float>(volcanoCenterCoordY) + 0.5f, ReligionSim::HumanGender::Male);
    fireShaman.setEsChaman(true);
    fireShaman.setSalud(100.0f);
    fireShaman.setReligionId(1);
    villagerManager.addVillager(fireShaman);

    ReligionSim::Humano worshipperUnit(2, "Fiel Devoto", static_cast<float>(volcanoCenterCoordX - 3) + 0.2f, static_cast<float>(volcanoCenterCoordY + 1) + 0.3f, ReligionSim::HumanGender::Female);
    worshipperUnit.setSalud(100.0f);
    worshipperUnit.setReligionId(1);
    villagerManager.addVillager(worshipperUnit);

    // EN: Advance game systems and particles
    // ES: Avanzar sistemas y partículas
    gameEngine.update(0.15f);

    // EN: Position camera centered on the volcanic caldera complex
    // ES: Posicionar la cámara centrada en el complejo de la caldera volcánica
    gameEngine.getCamera().setZoom(1.35f);
    gameEngine.getCamera().centerOnCell(volcanoCenterCoordX, volcanoCenterCoordY, worldMap.getWidth(), worldMap.getHeight());
    for (int frameIndex = 0; frameIndex < 25; ++frameIndex) {
        gameEngine.getCamera().update(0.1f);
    }

    gameEngine.render();

    const std::string exportPpmPath = "snapshot_volcanic_caldera_showcase.ppm";
    gameEngine.getRenderer().exportFrameToPPM(exportPpmPath);
    std::cout << "[Snapshot] Volcanic caldera showcase exported to " << exportPpmPath << std::endl;
    return 0;
}
