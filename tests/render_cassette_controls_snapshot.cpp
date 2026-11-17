#include "Game.hpp"
#include <iostream>

int main() {
    ReligionSim::Game gameEngine(1280, 720);
    gameEngine.init();
    gameEngine.startNewCustomWorld();
    gameEngine.setGameState(ReligionSim::GameState::Playing);
    gameEngine.getRenderer().setFogOfWar(nullptr);
    gameEngine.getCamera().setZoom(1.0f);
    gameEngine.getCamera().centerOnCell(25, 25, 50, 50);

    const auto& buttons = gameEngine.getRenderer().getButtons();
    const ReligionSim::UIButton* btnPause = nullptr;
    const ReligionSim::UIButton* btnPlay = nullptr;
    const ReligionSim::UIButton* btnFast = nullptr;

    for (const auto& btn : buttons) {
        if (btn.id == 201) btnPause = &btn;
        if (btn.id == 202) btnPlay = &btn;
        if (btn.id == 203) btnFast = &btn;
    }

    // EN: State 1: Normal 1x Speed (Play button active, cursor away, press animation finished)
    if (btnPlay) {
        gameEngine.onMouseDown(1, btnPlay->x + btnPlay->width * 0.5f, btnPlay->y + btnPlay->height * 0.5f);
        gameEngine.onMouseUp(1, btnPlay->x + btnPlay->width * 0.5f, btnPlay->y + btnPlay->height * 0.5f);
    }
    gameEngine.onMouseMove(300.0f, 300.0f);
    gameEngine.update(0.3f); // 300ms: click bounce finishes, returning to unpressed active state
    gameEngine.render();
    gameEngine.getRenderer().exportFrameToPPM("snapshot_state_play.ppm");

    // EN: State 2: Paused (Pause cassette key active with amber jewel, cursor away)
    if (btnPause) {
        gameEngine.onMouseDown(1, btnPause->x + 10.0f, btnPause->y + 10.0f);
        gameEngine.onMouseUp(1, btnPause->x + 10.0f, btnPause->y + 10.0f);
    }
    gameEngine.onMouseMove(300.0f, 300.0f);
    gameEngine.update(0.3f);
    gameEngine.render();
    gameEngine.getRenderer().exportFrameToPPM("snapshot_state_pause.ppm");

    // EN: State 3: Fast Forward 2x active
    if (btnFast) {
        gameEngine.onMouseDown(1, btnFast->x + 10.0f, btnFast->y + 10.0f);
        gameEngine.onMouseUp(1, btnFast->x + 10.0f, btnFast->y + 10.0f);
    }
    gameEngine.onMouseMove(300.0f, 300.0f);
    gameEngine.update(0.3f);
    gameEngine.render();
    gameEngine.getRenderer().exportFrameToPPM("snapshot_state_2x.ppm");

    // EN: State 4: Pressed simulation key (tactile mechanical depression)
    if (btnPause) {
        gameEngine.onMouseDown(1, btnPause->x + 10.0f, btnPause->y + 10.0f);
    }
    gameEngine.render();
    gameEngine.getRenderer().exportFrameToPPM("snapshot_state_pressed.ppm");

    std::cout << "[Showcase] Exported all 4 states successfully!" << std::endl;
    return 0;
}
