#include "Game.hpp"
#include <iostream>

int main() {
    ReligionSim::Game gameEngine(1280, 720);
    gameEngine.init();
    gameEngine.startNewCustomWorld();
    gameEngine.setGameState(ReligionSim::GameState::Playing);
    gameEngine.runCommand("LUMINUS"); // Disipar toda niebla de guerra
    gameEngine.getCamera().setZoom(1.0f);
    gameEngine.getCamera().centerOnCell(25, 25, 50, 50);
    gameEngine.update(1.0f);
    gameEngine.render();
    gameEngine.getRenderer().exportFrameToPPM("snapshot_luminus.ppm");
    return 0;
}
