#include "Game.hpp"
#include <iostream>

int main() {
    // EN: Initialize simulation engine in 1280x720 canvas
    // ES: Inicializar el motor de simulación en un lienzo de 1280x720
    ReligionSim::Game gameEngine(1280, 720);
    gameEngine.init();
    gameEngine.startNewCustomWorld();
    gameEngine.setGameState(ReligionSim::GameState::Playing);
    gameEngine.runCommand("LUMINUS");  // EN: Clear fog of war / ES: Disipar niebla de guerra
    gameEngine.runCommand("MEDIODIA"); // EN: Full daylight / ES: Plena luz diurna
    gameEngine.getRenderer().setFogOfWar(nullptr);

    ReligionSim::WorldMap& worldMap = gameEngine.getMap();

    int centerCellX = worldMap.getWidth() / 2;
    int centerCellY = worldMap.getHeight() / 2;
    for (int y = 0; y < worldMap.getHeight(); ++y) {
        for (int x = 0; x < worldMap.getWidth(); ++x) {
            const ReligionSim::Cell& c = worldMap.getCell(x, y);
            if (c.hasHolySite && c.holySiteName.find("Santuario Sagrado") != std::string::npos) {
                centerCellX = x;
                centerCellY = y;
                break;
            }
        }
    }

    std::cout << "[Showcase] Player Holy Site: (" << centerCellX << ", " << centerCellY << ")" << std::endl;
    for (size_t humanIdx = 0; humanIdx < gameEngine.getGestorHumanos().getHumanos().size() && humanIdx < 3; ++humanIdx) {
        const auto& humanRef = gameEngine.getGestorHumanos().getHumanos()[humanIdx];
        std::cout << "  Human " << humanIdx << ": (" << humanRef.getX() << ", " << humanRef.getY() << ")" << std::endl;
    }
    centerCellX += 3;
    centerCellY += 3;

    // EN: Clear surrounding sector and set base temperate woodland terrain
    // ES: Limpiar sector circundante y establecer terreno base de bosque templado
    for (int deltaOffsetY = -10; deltaOffsetY <= 10; ++deltaOffsetY) {
        for (int deltaOffsetX = -10; deltaOffsetX <= 10; ++deltaOffsetX) {
            const int targetCellX = centerCellX + deltaOffsetX;
            const int targetCellY = centerCellY + deltaOffsetY;
            if (!worldMap.isValid(targetCellX, targetCellY)) continue;

            ReligionSim::Cell& targetCell = worldMap.getCell(targetCellX, targetCellY);
            targetCell.hasHouse = false;
            targetCell.hasHolySite = false;
            targetCell.shrineLevel = 0;
            targetCell.isBurning = false;
            targetCell.isBurnt = false;
            targetCell.hasTreesUprooted = false;
            targetCell.hasImpactCrater = false;
            targetCell.waterDepth = 0.0f;
            targetCell.isFlooded = false;
            targetCell.hasActiveLava = false;
            targetCell.hasVolcano = false;
            targetCell.heatAccumulator = 0.0f;
            targetCell.biome = ReligionSim::BiomeType::DenseForest;
            targetCell.elevation = 1.0f;
        }
    }

    // EN: 1. Colossal Bomb Ground Zero (Nuclear / Super-Impact Basin with Colossal Crater)
    // ES: 1. Zona cero de bomba colosal (Cuenca nuclear / super-impacto con cráter colosal)
    ReligionSim::Cell& bombEpicenterCell = worldMap.getCell(centerCellX, centerCellY);
    bombEpicenterCell.formCrater(ReligionSim::CraterType::NuclearDetonation, 0.90f, 0.35f);
    bombEpicenterCell.isBurnt = true;
    bombEpicenterCell.hasTreesUprooted = false;
    bombEpicenterCell.heatAccumulator = 0.85f;

    // Periphery blast wave of bomb epicenter (inside blast bowl: ground burnt, all vegetation vaporized)
    for (int deltaCellY = -1; deltaCellY <= 1; ++deltaCellY) {
        for (int deltaCellX = -1; deltaCellX <= 1; ++deltaCellX) {
            if (deltaCellX == 0 && deltaCellY == 0) continue;
            ReligionSim::Cell& shockwaveCell = worldMap.getCell(centerCellX + deltaCellX, centerCellY + deltaCellY);
            shockwaveCell.isBurnt = true;
            shockwaveCell.hasTreesUprooted = false; // EN: Disintegrated by fireball, no stumps in crater bowl / ES: Desintegrado por bola de fuego, sin tocones en cráter
            shockwaveCell.heatAccumulator = 0.60f;
            shockwaveCell.naturalResource = ReligionSim::ResourceType::None;
            shockwaveCell.biome = ReligionSim::BiomeType::AridDesert;
        }
    }

    // EN: 2. Meteorite Impact Basin (Incandescent Basalt Rock Crater with Glowing Veins)
    // ES: 2. Cuenca de impacto de meteorito (Cráter de roca basáltica incandescente con vetas)
    ReligionSim::Cell& meteorCell = worldMap.getCell(centerCellX - 3, centerCellY + 2);
    meteorCell.formCrater(ReligionSim::CraterType::MeteorImpact, 0.75f, 0.25f);
    meteorCell.isBurnt = true;
    meteorCell.hasTreesUprooted = false;
    meteorCell.heatAccumulator = 0.70f;
    meteorCell.naturalResource = ReligionSim::ResourceType::None;
    meteorCell.biome = ReligionSim::BiomeType::AridDesert;

    // EN: 3. Active Wildfire Inferno Sector (Flaming Ground, Glowing Magma Veins, Burning Trees & Embers)
    // ES: 3. Sector de infierno de incendio forestal (Suelo en llamas, vetas de magma, árboles ardiendo y ascuas)
    const std::array<std::array<int, 2>, 6> wildfireCoords = {{
        {centerCellX + 3, centerCellY - 2},
        {centerCellX + 4, centerCellY - 2},
        {centerCellX + 3, centerCellY - 1},
        {centerCellX + 4, centerCellY - 1},
        {centerCellX + 3, centerCellY},
        {centerCellX + 4, centerCellY}
    }};
    for (const auto& coordinatePair : wildfireCoords) {
        ReligionSim::Cell& fireCell = worldMap.getCell(coordinatePair[0], coordinatePair[1]);
        fireCell.isBurning = true;
        fireCell.fireIntensity = 0.85f;
        fireCell.heatAccumulator = 0.95f;
        fireCell.burnTimer = 8.0f;
        fireCell.naturalResource = ReligionSim::ResourceType::Wood;
    }

    // EN: 4. Scorched Post-Wildfire Ash Wasteland (Soot, Charcoal, Ash Pits, Charred Crooked Trees & Stumps)
    // ES: 4. Páramo de ceniza post-incendio calcinado (Hollín, carbón, fosas de ceniza, árboles quemados y tocones)
    const std::array<std::array<int, 2>, 8> ashWastelandCoords = {{
        {centerCellX - 3, centerCellY - 3},
        {centerCellX - 2, centerCellY - 3},
        {centerCellX - 1, centerCellY - 3},
        {centerCellX - 3, centerCellY - 2},
        {centerCellX - 2, centerCellY - 2},
        {centerCellX - 1, centerCellY - 2},
        {centerCellX - 2, centerCellY - 1},
        {centerCellX - 1, centerCellY - 1}
    }};
    for (const auto& coordinatePair : ashWastelandCoords) {
        ReligionSim::Cell& ashCell = worldMap.getCell(coordinatePair[0], coordinatePair[1]);
        ashCell.isBurnt = true;
        ashCell.hasTreesUprooted = true;
        ashCell.naturalResource = ReligionSim::ResourceType::Wood;
    }

    // EN: 5. Tectonic Seismic Rupture Chasm with Pulverized Debris
    // ES: 5. Sima de ruptura tectónica sísmica con escombros pulverizados
    ReligionSim::Cell& earthquakeCell = worldMap.getCell(centerCellX + 1, centerCellY + 3);
    earthquakeCell.elevation = 0.80f; // EN: Tectonic subsidence elevation drop / ES: Depresión de elevación por subsidencia tectónica
    earthquakeCell.fissureAngle = 0.785f; // 45 degrees
    earthquakeCell.hasTreesUprooted = true;

    // Advance simulation frames for animation time and dynamic particle update
    for (int frameIndex = 0; frameIndex < 12; ++frameIndex) {
        gameEngine.update(0.05f);
    }

    // EN: Position camera centered on destruction complex right before rendering
    // ES: Posicionar cámara centrada en el complejo de destrucción justo antes de renderizar
    ReligionSim::IsometricCamera& camera = gameEngine.getCamera();
    camera.setZoom(1.95f);
    camera.centerOnCell(centerCellX, centerCellY, worldMap.getWidth(), worldMap.getHeight());
    camera.snapToTarget();

    // Render frame
    gameEngine.render();

    // Export PPM
    const std::string ppmPath = "/home/Ivancho/.gemini/antigravity/brain/1dbff22e-5d97-4436-b0dc-e7a7cb63301c/snapshot_destruction_showcase.ppm";
    const bool saveSuccess = gameEngine.getRenderer().exportFrameToPPM(ppmPath);
    if (saveSuccess) {
        std::cout << "[SUCCESS] Destruction tileset showcase exported: " << ppmPath << std::endl;
    } else {
        std::cerr << "[ERROR] Failed to export PPM snapshot!" << std::endl;
        return 1;
    }

    return 0;
}
