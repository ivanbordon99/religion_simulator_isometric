#include "Game.hpp"
#include <iostream>

// EN: Render showcase snapshot of the new geological mineral system and veins
// ES: Renderizar captura de muestra del nuevo sistema geológico de minerales y vetas
int main() {
    ReligionSim::Game gameEngine(1280, 720);
    gameEngine.init();
    gameEngine.startNewCustomWorld();
    gameEngine.setGameState(ReligionSim::GameState::Playing);
    gameEngine.runCommand("LUMINUS");

    ReligionSim::WorldMap& map = gameEngine.getMap();
    int sumLandX = 0, sumLandY = 0, totalLandCount = 0;
    for (int cellY = 0; cellY < map.getHeight(); ++cellY) {
        for (int cellX = 0; cellX < map.getWidth(); ++cellX) {
            if (!map.getCell(cellX, cellY).isWater()) {
                sumLandX += cellX;
                sumLandY += cellY;
                totalLandCount++;
            }
        }
    }
    const int centerCellX = (totalLandCount > 0) ? (sumLandX / totalLandCount) : (map.getWidth() / 2);
    const int centerCellY = (totalLandCount > 0) ? (sumLandY / totalLandCount) : (map.getHeight() / 2);
    std::cout << "[Inland Continent Center] (" << centerCellX << ", " << centerCellY << ") totalLand=" << totalLandCount << std::endl;

    // EN: Create a majestic geological mountain range with foothills and crags
    // ES: Crear una majestuosa cordillera geológica con colinas y riscos
    for (int offsetY = -6; offsetY <= 6; ++offsetY) {
        for (int offsetX = -6; offsetX <= 6; ++offsetX) {
            const int cellX = centerCellX + offsetX;
            const int cellY = centerCellY + offsetY;
            if (!map.isValid(cellX, cellY)) continue;

            ReligionSim::Cell& cell = map.getCell(cellX, cellY);
            cell.isBurning = false;
            cell.isBurnt = false;
            cell.waterDepth = 0.0f;
            cell.hasImpactCrater = false;
            cell.hasTreesUprooted = false;
            cell.naturalResource = ReligionSim::ResourceType::None;
            cell.resourceAmount = 0;
            cell.maxResourceAmount = 0;
            cell.resourceVariant = 0;

            const float distFromCenter = std::sqrt(static_cast<float>(offsetX * offsetX + offsetY * offsetY));
            if (distFromCenter <= 2.2f) {
                cell.biome = ReligionSim::BiomeType::RuggedMountains;
                cell.elevation = 2.40f;
            } else if (distFromCenter <= 4.5f) {
                cell.biome = ReligionSim::BiomeType::HighlandHills;
                cell.elevation = 1.60f;
            } else {
                cell.biome = ReligionSim::BiomeType::CoastalPlains;
                cell.elevation = 0.80f;
            }
        }
    }

    // EN: 1. Copper Vein (3 contiguous cells in HighlandHills)
    // ES: 1. Veta de Cobre (3 celdas contiguas en HighlandHills)
    // - Origin: Exposed copper vein embedded into rock (variant 0)
    // - Rock: Weathered copper boulder (variant 1)
    // - Nuggets: Partially mined native copper nuggets (variant 2)
    {
        ReligionSim::Cell& copperOrigin = map.getCell(centerCellX - 3, centerCellY);
        copperOrigin.naturalResource = ReligionSim::ResourceType::Copper;
        copperOrigin.resourceAmount = 180;
        copperOrigin.maxResourceAmount = 180;
        copperOrigin.resourceVariant = 0; // resource_copper_vein

        ReligionSim::Cell& copperRock = map.getCell(centerCellX - 3, centerCellY + 1);
        copperRock.naturalResource = ReligionSim::ResourceType::Copper;
        copperRock.resourceAmount = 150;
        copperRock.maxResourceAmount = 150;
        copperRock.resourceVariant = 1; // resource_copper_rock

        ReligionSim::Cell& copperNuggets = map.getCell(centerCellX - 3, centerCellY + 2);
        copperNuggets.naturalResource = ReligionSim::ResourceType::Copper;
        copperNuggets.resourceAmount = 40;
        copperNuggets.maxResourceAmount = 140;
        copperNuggets.resourceVariant = 2; // resource_copper_nuggets
    }

    // EN: 2. Banded Iron Vein (3 contiguous cells along mountain ridge)
    // ES: 2. Veta de Hierro Bandeado (3 celdas contiguas en cresta montañosa)
    // - Vein: Dark magnetite ore seam (variant 0)
    // - Rock: Massive ironstone boulder (variant 1)
    {
        ReligionSim::Cell& ironVein = map.getCell(centerCellX, centerCellY - 1);
        ironVein.naturalResource = ReligionSim::ResourceType::Iron;
        ironVein.resourceAmount = 200;
        ironVein.maxResourceAmount = 200;
        ironVein.resourceVariant = 0; // resource_iron_vein

        ReligionSim::Cell& ironRock = map.getCell(centerCellX + 1, centerCellY - 1);
        ironRock.naturalResource = ReligionSim::ResourceType::Iron;
        ironRock.resourceAmount = 170;
        ironRock.maxResourceAmount = 170;
        ironRock.resourceVariant = 1; // resource_iron_rock
    }

    // EN: 3. Hydrothermal Gold Vein in high crags
    // ES: 3. Veta de Oro Hidrotermal en riscos altos
    {
        ReligionSim::Cell& goldVein = map.getCell(centerCellX + 1, centerCellY + 1);
        goldVein.naturalResource = ReligionSim::ResourceType::Gold;
        goldVein.resourceAmount = 130;
        goldVein.maxResourceAmount = 130;
        goldVein.resourceVariant = 0; // resource_gold_vein

        ReligionSim::Cell& goldNuggets = map.getCell(centerCellX, centerCellY + 2);
        goldNuggets.naturalResource = ReligionSim::ResourceType::Gold;
        goldNuggets.resourceAmount = 30;
        goldNuggets.maxResourceAmount = 100;
        goldNuggets.resourceVariant = 2; // resource_gold_nuggets
    }

    // EN: 4. Kimberlite Diamond Crystal in peak
    // ES: 4. Cristal de Diamante Kimberlítico en cumbre
    {
        ReligionSim::Cell& diamondCell = map.getCell(centerCellX - 1, centerCellY - 1);
        diamondCell.naturalResource = ReligionSim::ResourceType::Diamond;
        diamondCell.resourceAmount = 80;
        diamondCell.maxResourceAmount = 80;
        diamondCell.resourceVariant = 0; // resource_diamond_crystal
    }

    // EN: 5. Stone Quarry and granite boulder outcrop
    // ES: 5. Cantera de Piedra y afloramiento de peñascos de granito
    {
        ReligionSim::Cell& stoneQuarry = map.getCell(centerCellX + 3, centerCellY);
        stoneQuarry.naturalResource = ReligionSim::ResourceType::Stone;
        stoneQuarry.resourceAmount = 250;
        stoneQuarry.maxResourceAmount = 250;
        stoneQuarry.resourceVariant = 0; // resource_stone_quarry

        ReligionSim::Cell& stoneBoulder = map.getCell(centerCellX + 3, centerCellY + 1);
        stoneBoulder.naturalResource = ReligionSim::ResourceType::Stone;
        stoneBoulder.resourceAmount = 220;
        stoneBoulder.maxResourceAmount = 220;
        stoneBoulder.resourceVariant = 1; // resource_stone_boulder
    }

    // EN: Center camera on the geological mountain sector and update smoothly
    // ES: Centrar cámara en el sector geológico de montaña y actualizar suavemente
    gameEngine.getCamera().setZoom(1.9f);

    // EN: Tick camera interpolation multiple frames with centerOnCell each frame so it fully locks onto target at target zoom
    // ES: Ejecutar múltiples pasos con centerOnCell cada fotograma para que fije el objetivo con el zoom final
    for (int frame = 0; frame < 60; ++frame) {
        gameEngine.getCamera().centerOnCell(centerCellX, centerCellY, map.getWidth(), map.getHeight());
        gameEngine.update(0.1f);
    }

    gameEngine.render();
    gameEngine.getRenderer().exportFrameToPPM("snapshot_geological_minerals.ppm");
    std::cout << "[Snapshot] Geological minerals snapshot exported successfully!" << std::endl;
    return 0;
}
