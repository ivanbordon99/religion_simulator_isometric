#include "Game.hpp"
#include <iostream>
#include <algorithm>

using namespace ReligionSim;

int main() {
    Game game(1280, 720);
    game.init();

    // EN: 1. Setup world and play
    // ES: 1. Configurar mundo y jugar
    game.prepareForWorldCreation(false);
    game.startNewCustomWorld();
    game.update(0.1f);

    // EN: Trigger atomic catastrophe to create dramatic craters, charred trees, and extinction
    // ES: Detonar catástrofe atómica para crear cráteres dramáticos, árboles quemados y extinción
    game.castDivinePower(DivinePowerType::AtomicBomb, 64, 48);
    for (int step = 0; step < 10; ++step) {
        game.update(0.1f);
    }

    // EN: Ensure all remaining units perish to trigger total faith extinction
    // ES: Asegurar que todas las unidades restantes perezcan para detonar la extinción total de la fe
    for (auto& villager : game.getGestorHumanos().getHumanos()) {
        villager.matar();
    }

    // EN: Advance dramatic funerary pause (1.8s) into GameState::GameOver
    // ES: Avanzar la pausa dramática fúnebre (1.8s) hacia GameState::GameOver
    for (int step = 0; step < 25; ++step) {
        game.update(0.1f);
    }

    // EN: 2. Export Game Over modal frame (with new OBSERVAR MUNDO button)
    // ES: 2. Exportar fotograma del modal de Game Over (con el nuevo botón OBSERVAR MUNDO)
    game.render();
    game.getRenderer().exportFrameToPPM("snapshot_gameover_modal.ppm");
    std::cout << "[Snapshot] Game Over modal exported." << std::endl;

    // EN: 3. Switch to Post-Defeat Spectator Mode via key 'V' (or Button 904)
    // ES: 3. Cambiar al Modo Espectador tras la derrota mediante tecla 'V' (o Botón 904)
    game.onKeyPress('v');
    game.getCamera().centerOnCell(64, 48, game.getMap().getWidth(), game.getMap().getHeight());
    game.onMouseMove(640.0f, 360.0f);
    game.update(0.1f);
    game.render();
    game.getRenderer().exportFrameToPPM("snapshot_postdefeat_spectator.ppm");
    std::cout << "[Snapshot] Post-defeat Spectator Mode exported." << std::endl;

    return 0;
}
