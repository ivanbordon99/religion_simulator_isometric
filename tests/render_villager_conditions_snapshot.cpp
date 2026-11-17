#include "Game.hpp"
#include <iostream>

int main() {
    ReligionSim::Game gameEngine(1280, 720);
    gameEngine.init();
    gameEngine.startNewCustomWorld();
    gameEngine.setGameState(ReligionSim::GameState::Playing);
    gameEngine.runCommand("LUMINUS");

    // EN: Select the first player villager and apply the BORRACHO condition
    // ES: Seleccionar el primer aldeano del jugador y aplicar la condición BORRACHO
    if (!gameEngine.getGestorHumanos().getHumanos().empty()) {
        const int firstVillagerId = gameEngine.getGestorHumanos().getHumanos()[0].getId();
        gameEngine.getGestorHumanos().selectVillager(firstVillagerId);
        gameEngine.runCommand("BORRACHO");

        const auto* selected = gameEngine.getGestorHumanos().getHumanoSeleccionado();
        if (selected) {
            gameEngine.getCamera().setZoom(1.8f);
            gameEngine.getCamera().centerOnCell(static_cast<int>(selected->getX()),
                                                static_cast<int>(selected->getY()), 50, 50);
        }
    }

    gameEngine.update(0.5f);
    gameEngine.render();
    gameEngine.getRenderer().exportFrameToPPM("snapshot_villager_conditions.ppm");
    std::cout << "[Snapshot] Exportado exitosamente a snapshot_villager_conditions.ppm" << std::endl;
    return 0;
}
