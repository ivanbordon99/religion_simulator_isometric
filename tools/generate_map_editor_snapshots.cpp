#include "Game.hpp"
#include <iostream>
#include <algorithm>

using namespace ReligionSim;

int main() {
    Game game(1280, 720);
    game.init();

    // 1. Snapshot of Main Menu with the new "CREACION DE MAPAS" button
    game.update(0.016f);
    game.render();
    game.getRenderer().exportFrameToPPM("snapshot_main_menu_with_map_creator.ppm");
    std::cout << "[Snapshot] Main Menu exported." << std::endl;

    // 2. Open Map Editor
    const auto& menuButtons = game.getRenderer().getMenuButtons();
    const float btnX = menuButtons[2].x + menuButtons[2].width * 0.5f;
    const float btnY = menuButtons[2].y + menuButtons[2].height * 0.5f;
    game.onMouseDown(1, btnX, btnY);
    game.onMouseUp(1, btnX, btnY);

    game.update(0.016f);
    game.render();
    game.getRenderer().exportFrameToPPM("snapshot_map_editor_initial.ppm");
    std::cout << "[Snapshot] Map Editor Initial exported." << std::endl;

    // 3. Paint some terrain and resources on the editor map
    const auto& editorButtons = game.getRenderer().getMapEditorButtons();
    auto b3x3 = std::find_if(editorButtons.begin(), editorButtons.end(), [](const UIButton& b) { return b.id == 822; });
    if (b3x3 != editorButtons.end()) {
        game.onMouseDown(1, b3x3->x + b3x3->width * 0.5f, b3x3->y + b3x3->height * 0.5f);
        game.onMouseUp(1, b3x3->x + b3x3->width * 0.5f, b3x3->y + b3x3->height * 0.5f);
    }

    // Select Volcanic Caldera (ID 839)
    auto bVolcano = std::find_if(editorButtons.begin(), editorButtons.end(), [](const UIButton& b) { return b.id == 839; });
    if (bVolcano != editorButtons.end()) {
        game.onMouseDown(1, bVolcano->x + bVolcano->width * 0.5f, bVolcano->y + bVolcano->height * 0.5f);
        game.onMouseUp(1, bVolcano->x + bVolcano->width * 0.5f, bVolcano->y + bVolcano->height * 0.5f);
    }

    constexpr float canvasX = 494.0f;
    constexpr float canvasY = 92.0f;
    constexpr float canvasW = 744.0f;
    constexpr float canvasH = 470.0f;
    const float scale = std::min(canvasW / 128.0f, canvasH / 128.0f);
    const float previewStartX = canvasX + (canvasW - 128.0f * scale) * 0.5f;
    const float previewStartY = canvasY + (canvasH - 128.0f * scale) * 0.5f;

    for (int dy = -4; dy <= 4; dy += 2) {
        for (int dx = -4; dx <= 4; dx += 2) {
            float px = previewStartX + (64.0f + dx) * scale;
            float py = previewStartY + (64.0f + dy) * scale;
            game.onMouseDown(1, px, py);
            game.onMouseUp(1, px, py);
        }
    }

    // Select Gold (ID 843)
    auto bGold = std::find_if(editorButtons.begin(), editorButtons.end(), [](const UIButton& b) { return b.id == 843; });
    if (bGold != editorButtons.end()) {
        game.onMouseDown(1, bGold->x + bGold->width * 0.5f, bGold->y + bGold->height * 0.5f);
        game.onMouseUp(1, bGold->x + bGold->width * 0.5f, bGold->y + bGold->height * 0.5f);
    }
    for (int dx = -6; dx <= 6; dx += 3) {
        float px = previewStartX + (64.0f + dx) * scale;
        float py = previewStartY + 74.0f * scale;
        game.onMouseDown(1, px, py);
        game.onMouseUp(1, px, py);
    }

    // Hover near center
    game.onMouseMove(previewStartX + 64.0f * scale, previewStartY + 64.0f * scale);

    game.update(0.016f);
    game.render();
    game.getRenderer().exportFrameToPPM("snapshot_map_editor_sculpted.ppm");
    std::cout << "[Snapshot] Map Editor Sculpted exported." << std::endl;

    // 4. Switch to 2.5D Isometric Mode (TAB key or Button 880)
    // EN: Switch to Isometric 2.5D view mode
    // ES: Cambiar al modo de vista isométrica 2.5D
    game.onKeyPress(9); // TAB key
    game.update(0.016f);

    // Select Terrain Tile Palette tab 0 (ID 871) and select Lava Tile (ID 915)
    const auto& isoButtons = game.getRenderer().getMapEditorIsometricButtons();
    auto bLava = std::find_if(isoButtons.begin(), isoButtons.end(), [](const UIButton& b) { return b.id == 915; });
    if (bLava != isoButtons.end()) {
        game.onMouseDown(1, bLava->x + bLava->width * 0.5f, bLava->y + bLava->height * 0.5f);
        game.onMouseUp(1, bLava->x + bLava->width * 0.5f, bLava->y + bLava->height * 0.5f);
    }

    // Paint in the 2.5D world near center
    constexpr float screenCenterX = 640.0f;
    constexpr float screenCenterY = 360.0f;
    game.onMouseDown(1, screenCenterX, screenCenterY);
    game.onMouseUp(1, screenCenterX, screenCenterY);
    game.onMouseDown(1, screenCenterX + 32.0f, screenCenterY + 16.0f);
    game.onMouseUp(1, screenCenterX + 32.0f, screenCenterY + 16.0f);
    game.onMouseDown(1, screenCenterX - 32.0f, screenCenterY - 16.0f);
    game.onMouseUp(1, screenCenterX - 32.0f, screenCenterY - 16.0f);

    // Hover mouse in center to render the golden diamond isometric reticle
    game.onMouseMove(screenCenterX, screenCenterY);

    game.update(0.016f);
    game.render();
    game.getRenderer().exportFrameToPPM("snapshot_map_editor_isometric.ppm");
    std::cout << "[Snapshot] Map Editor Isometric 2.5D exported." << std::endl;

    return 0;
}
