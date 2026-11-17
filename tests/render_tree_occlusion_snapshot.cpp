#include "Game.hpp"
#include <iostream>

int main() {
    // EN: Initialize simulation engine in 1280x720 resolution
    // ES: Inicializar motor de simulación en resolución 1280x720
    ReligionSim::Game gameEngine(1280, 720);
    gameEngine.init();
    gameEngine.startNewCustomWorld();
    gameEngine.setGameState(ReligionSim::GameState::Playing);

    ReligionSim::WorldMap& worldMap = gameEngine.getMap();
    constexpr int targetCellX = 30;
    constexpr int targetCellY = 30;

    // EN: Configure cell (30, 30) as a dense forest tile with a 2-cell tall tree
    // ES: Configurar la celda (30, 30) como tesela de bosque denso con árbol de 2 celdas de alto
    ReligionSim::Cell& targetCell = worldMap.getCell(targetCellX, targetCellY);
    targetCell.biome = ReligionSim::BiomeType::DenseForest;
    targetCell.naturalResource = ReligionSim::ResourceType::Wood;
    targetCell.resourceAmount = 250;
    targetCell.elevation = 1.0f;
    targetCell.hasVolcano = false;
    targetCell.hasHolySite = false;

    // EN: Also configure neighboring tiles as forest for realistic context
    // ES: Configurar también teselas vecinas como bosque para contexto realista
    for (int deltaOffsetY = -2; deltaOffsetY <= 2; ++deltaOffsetY) {
        for (int deltaOffsetX = -2; deltaOffsetX <= 2; ++deltaOffsetX) {
            int neighborCellX = targetCellX + deltaOffsetX;
            int neighborCellY = targetCellY + deltaOffsetY;
            if (neighborCellX >= 0 && neighborCellX < worldMap.getWidth() &&
                neighborCellY >= 0 && neighborCellY < worldMap.getHeight()) {
                ReligionSim::Cell& neighborCell = worldMap.getCell(neighborCellX, neighborCellY);
                neighborCell.elevation = 1.0f;
                if (neighborCellX != targetCellX || neighborCellY != targetCellY) {
                    neighborCell.biome = ReligionSim::BiomeType::DenseForest;
                }
            }
        }
    }

    // EN: Position an occluded villager behind the tree canopy (isoDepth < treeDepth)
    // ES: Posicionar a un aldeano oculto detrás de la copa del árbol (isoDepth < treeDepth)
    ReligionSim::GestorHumanos& villagerManager = gameEngine.getGestorHumanos();
    villagerManager.getHumanos().clear();

    ReligionSim::Humano occludedVillager(1, "Aldeano Oculto", static_cast<float>(targetCellX) + 0.1f, static_cast<float>(targetCellY) - 0.2f);
    occludedVillager.setSalud(100.0f);
    occludedVillager.setDevocion(0.95f);
    villagerManager.agregarHumano(occludedVillager);

    // EN: Position a second villager in front of the tree base (isoDepth > treeDepth)
    // ES: Posicionar a un segundo aldeano delante de la base del árbol (isoDepth > treeDepth)
    ReligionSim::Humano foregroundVillager(2, "Aldeano Al Frente", static_cast<float>(targetCellX) + 0.35f, static_cast<float>(targetCellY) + 0.45f);
    foregroundVillager.setSalud(100.0f);
    foregroundVillager.setDevocion(0.85f);
    villagerManager.agregarHumano(foregroundVillager);

    // EN: Select the occluded villager so player visual feedback activates (Divine Gold X-Ray silhouette)
    // ES: Seleccionar al aldeano oculto para activar retroalimentación visual del jugador (silueta rayos X oro divino)
    villagerManager.seleccionarHumano(1);

    // EN: Setup isometric camera close-up centered on the target tree
    // ES: Configurar cámara isométrica en primer plano centrada en el árbol objetivo
    gameEngine.getRenderer().setFogOfWar(nullptr);
    gameEngine.getCamera().setZoom(1.5f);
    gameEngine.getCamera().centerOnCell(targetCellX, targetCellY, worldMap.getWidth(), worldMap.getHeight());
    for (int frameIndex = 0; frameIndex < 20; ++frameIndex) {
        gameEngine.getCamera().update(0.1f);
    }

    gameEngine.update(0.1f);
    gameEngine.render();

    const std::string exportPpmPath = "tree_occlusion_snapshot.ppm";
    gameEngine.getRenderer().exportFrameToPPM(exportPpmPath);
    std::cout << "Tree occlusion snapshot PPM written to: " << exportPpmPath << std::endl;
    return 0;
}
