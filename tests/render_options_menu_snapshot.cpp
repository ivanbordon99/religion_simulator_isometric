// EN: Visual snapshot generator for the new modular Options Menu
// ES: Generador de capturas visuales para el nuevo Menú de Opciones modular

#include "Game.hpp"
#include <iostream>
#include <string>

int main() {
    constexpr int targetWidth = 1280;
    constexpr int targetHeight = 720;

    ReligionSim::Game game(targetWidth, targetHeight);
    game.init();

    // EN: Transition to Options state
    // ES: Transicionar al estado de Opciones
    game.setGameState(ReligionSim::GameState::Options);

    // EN: Simulate 5 ticks to advance the background panorama
    // ES: Simular 5 cuadros para avanzar el panorama de fondo
    for (int tick = 0; tick < 5; ++tick) {
        game.update(0.1f);
    }

    // 1. Render Screen & Video Tab
    game.setCurrentOptionsTab(ReligionSim::OptionsTab::ScreenAndVideo);
    game.render();
    game.getRenderer().exportFrameToPPM("options_tab1_screen.ppm");

    // 2. Render Graphics & Image Tab
    game.setCurrentOptionsTab(ReligionSim::OptionsTab::GraphicsAndImage);
    game.render();
    game.getRenderer().exportFrameToPPM("options_tab2_graphics.ppm");

    // 3. Render Gameplay Tab
    game.setCurrentOptionsTab(ReligionSim::OptionsTab::GameplayAndSimulation);
    game.render();
    game.getRenderer().exportFrameToPPM("options_tab3_gameplay.ppm");

    // 4. Render Audio Tab
    game.setCurrentOptionsTab(ReligionSim::OptionsTab::AudioAndSound);
    game.render();
    game.getRenderer().exportFrameToPPM("options_tab4_audio.ppm");

    std::cout << "[OptionsSnapshot] All 4 options tabs exported to PPM successfully!" << std::endl;
    return 0;
}
