#include "Game.hpp"
#include <iostream>

int main() {
    ReligionSim::Game gameEngine(1280, 720);
    gameEngine.init();
    gameEngine.startNewCustomWorld();
    gameEngine.setGameState(ReligionSim::GameState::Playing);
    gameEngine.runCommand("LUMINUS");
    gameEngine.runCommand("MEDIODIA");
    gameEngine.getRenderer().setFogOfWar(nullptr);

    ReligionSim::WorldMap& map = gameEngine.getMap();
    constexpr int targetGridX = 25;
    constexpr int targetGridY = 25;

    // Center camera on target
    gameEngine.getCamera().setZoom(1.0f);
    gameEngine.getCamera().centerOnCell(targetGridX, targetGridY, map.getWidth(), map.getHeight());

    // Cast Meteor Cataclysm at (targetGridX, targetGridY)
    gameEngine.castDivinePower(ReligionSim::DivinePowerType::Meteor, targetGridX, targetGridY);

    // Advance 0.60 seconds (halfway through descent at timer = 0.60s / 1.25s)
    gameEngine.update(0.60f);

    // Render frame 1: atmospheric descent
    gameEngine.render();
    const std::string exportPpmPath = "snapshot_meteor_alignment.ppm";
    gameEngine.getRenderer().exportFrameToPPM(exportPpmPath);
    std::cout << "[Snapshot] Meteor alignment snapshot exported to " << exportPpmPath << std::endl;

    // Advance 1.0 second (t = 1.60s, which is 0.35s after impact at 1.25s)
    gameEngine.update(1.00f);

    // Render frame 2: post-impact crater, ejecta blanket, and supersonic airblast
    gameEngine.render();
    const std::string exportImpactPath = "snapshot_meteor_impact.ppm";
    gameEngine.getRenderer().exportFrameToPPM(exportImpactPath);
    std::cout << "[Snapshot] Meteor impact crater snapshot exported to " << exportImpactPath << std::endl;
    return 0;
}
