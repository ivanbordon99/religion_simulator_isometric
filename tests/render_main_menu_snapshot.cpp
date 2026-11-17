// EN: Verification test and snapshot generator for Minecraft-inspired living world main menu
// ES: Prueba de verificación y generador de captura para el menú principal con mundo vivo inspirado en Minecraft

#include "Game.hpp"
#include <iostream>
#include <vector>
#include <string>

int main() {
    constexpr int targetCanvasWidth = 1280;
    constexpr int targetCanvasHeight = 720;
    constexpr float simulationDeltaStep = 0.1f;
    constexpr int totalSimulationTicks = 20;

    std::cout << "[MainMenuTest] Initializing game instance..." << std::endl;
    ReligionSim::Game gameInstance(targetCanvasWidth, targetCanvasHeight);
    gameInstance.init();

    if (gameInstance.getGameState() != ReligionSim::GameState::MainMenu) {
        std::cerr << "[MainMenuTest] Error: Expected GameState::MainMenu on startup!" << std::endl;
        return 1;
    }

    for (int currentTick = 0; currentTick < totalSimulationTicks; ++currentTick) {
        gameInstance.update(simulationDeltaStep);
    }

    // 1. Snapshot normal (sin hover)
    gameInstance.render();
    gameInstance.getRenderer().exportFrameToPPM("main_menu_panorama_snapshot.ppm");

    // 2. Snapshot con hover sobre el botón 1 (CREAR MUNDO)
    const auto& menuBtns = gameInstance.getRenderer().getMenuButtons();
    if (!menuBtns.empty()) {
        float hovX = menuBtns[0].x + menuBtns[0].width * 0.5f;
        float hovY = menuBtns[0].y + menuBtns[0].height * 0.5f;
        gameInstance.onMouseMove(hovX, hovY);
    }
    gameInstance.render();
    gameInstance.getRenderer().exportFrameToPPM("main_menu_panorama_hover_snapshot.ppm");

    std::cout << "[MainMenuTest] Successfully generated snapshot PPMs!" << std::endl;
    return 0;
}
