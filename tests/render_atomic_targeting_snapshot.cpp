#include "Game.hpp"
#include <iostream>

int main() {
    ReligionSim::Game gameEngine(1280, 720);
    gameEngine.init();
    gameEngine.startNewCustomWorld();
    gameEngine.setGameState(ReligionSim::GameState::Playing);

    ReligionSim::WorldMap& worldMap = gameEngine.getMap();
    constexpr int targetX = 25;
    constexpr int targetY = 25;

    // EN: Activate FUCKINGBOMB cheat targeting mode
    // ES: Activar modo apuntar del truco FUCKINGBOMB
    gameEngine.toggleDivinePowerTargeting(ReligionSim::DivinePowerType::AtomicBomb);

    // EN: Set hover cell to center to show reticle
    // ES: Fijar celda hover al centro para mostrar la retícula
    gameEngine.getCamera().setZoom(1.20f);
    gameEngine.getCamera().centerOnCell(targetX, targetY, worldMap.getWidth(), worldMap.getHeight());
    for (int frameIndex = 0; frameIndex < 20; ++frameIndex) {
        gameEngine.getCamera().update(0.1f);
    }

    gameEngine.getRenderer().setFogOfWar(nullptr);
    gameEngine.render();

    const std::string exportPpmPath = "snapshot_atomic_targeting.ppm";
    gameEngine.getRenderer().exportFrameToPPM(exportPpmPath);
    std::cout << "[Snapshot] Atomic targeting reticle exported to " << exportPpmPath << std::endl;
    return 0;
}
