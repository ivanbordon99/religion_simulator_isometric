#include "Game.hpp"
#include <iostream>

int main() {
    ReligionSim::Game game(1280, 720);
    game.init();
    game.startNewCustomWorld();
    game.setGameState(ReligionSim::GameState::Playing);
    
    int w = game.getMap().getWidth();
    int h = game.getMap().getHeight();
    
    game.getRenderer().setFogOfWar(nullptr);
    game.getCamera().setZoom(0.35f);
    for (int i = 0; i < 30; ++i) game.getCamera().update(0.1f);
    game.getCamera().centerOnCell(w / 2, h / 2, w, h);
    for (int i = 0; i < 30; ++i) game.getCamera().update(0.1f);
    
    game.update(0.1f);
    game.render();
    game.getRenderer().exportFrameToPPM("immense_world_overview.ppm");
    std::cout << "Immense overview exported!" << std::endl;
    return 0;
}
