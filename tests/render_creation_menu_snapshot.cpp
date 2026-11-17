// EN: Verification test and snapshot generator for World Creation menu with immense map sizes
// ES: Prueba de verificación y generador de captura para el menú de creación con tamaños de mapa inmensos

#include "Game.hpp"
#include <iostream>

int main() {
    constexpr int targetCanvasWidth = 1280;
    constexpr int targetCanvasHeight = 720;

    std::cout << "[CreationMenuTest] Initializing game instance..." << std::endl;
    ReligionSim::Game gameInstance(targetCanvasWidth, targetCanvasHeight);
    gameInstance.init();

    gameInstance.setGameState(ReligionSim::GameState::WorldCreation);
    gameInstance.render();
    gameInstance.getRenderer().exportFrameToPPM("world_creation_menu_snapshot.ppm");

    std::cout << "[CreationMenuTest] Successfully generated snapshot PPM!" << std::endl;
    return 0;
}
