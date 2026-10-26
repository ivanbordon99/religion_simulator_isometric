#pragma once

#include "Types.hpp"
#include "IsometricCamera.hpp"
#include "WorldMap.hpp"
#include "Religion.hpp"
#include "AdvancementSystem.hpp"
#include "Timeline.hpp"
#include "EventLine.hpp"
#include "ReplicationEngine.hpp"
#include "Villager.hpp"
#include "TileSet.hpp"
#include "Humano.hpp"
#include "FogOfWar.hpp"
#include <array>
#include <vector>
#include <string>

namespace ReligionSim {

struct UIButton {
    int id;
    float x, y, width, height;
    std::string label;
    bool isToggled{false};
};

class Renderer {
public:
    Renderer(int width = 1280, int height = 720);
    ~Renderer();

    bool initWindow(const std::string& title = "Marangatu: The Faith Simulator");
    void closeWindow();

    void beginFrame();
    void endFrame();

    void render(const WorldMap& map, const ReligionRegistry& registry,
                const Timeline& timeline, const EventLine& eventLine,
                const ReplicationEngine& replication, const GestorHumanos& humanos,
                const IsometricCamera& camera,
                const Point2i& selectedCell, RenderMapMode mode,
                const WorldCreationConfig& playerConfig);
    void renderCommandConsole(bool isOpen, const std::string& inputText,
                              bool selectorActive, const std::string& feedbackMsg,
                              bool luminusActive = false);

    // Niebla de Guerra de la Fe: la visión proviene sólo del pueblo del jugador
    void setFogOfWar(const FaithFogOfWar* fog) { m_fog = fog; }

    // Desplegable de opciones de partida (botón MENU de la barra superior)
    void setOptionsDropdownOpen(bool open) { m_dropdownOpen = open; }
    bool isOptionsDropdownOpen() const { return m_dropdownOpen; }

    // Rectángulo del minimapa dentro del panel inferior derecho (compartido con el hit-test de Game)
    void getMinimapRect(int& x, int& y, int& size) const;

    // Pantalla de Menú Principal, Creación de Mundo y Opciones
    void renderMainMenu(int hoveredButtonId);
    void renderOptionsMenu(int hoveredButtonId);
    void renderWorldCreationMenu(const WorldCreationConfig& config, int hoveredButtonId, int activeInputField);
    void renderLoadWorldMenu(int hoveredButtonId, int selectedSaveIndex);
    int checkMenuButtonClicked(float mouseX, float mouseY, bool inOptions = false) const;
    int checkCreationButtonClicked(float mouseX, float mouseY) const;
    int checkLoadMenuButtonClicked(float mouseX, float mouseY, int savesCount) const;

    // Save snapshot to PPM/BMP file
    bool exportFrameToPPM(const std::string& filepath);

    // Viewport dimensions & dynamic resize
    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }
    void resize(int newWidth, int newHeight);

    TileSet& getTileSet() { return m_tileSet; }
    const TileSet& getTileSet() const { return m_tileSet; }

    // Hit-testing for UI elements
    int checkButtonClicked(float mouseX, float mouseY) const;

    // Direct pixel drawing routines for software rasterizer
    void clear(ColorRGB color);
    void drawLine(int x0, int y0, int x1, int y1, ColorRGB color);
    void drawFilledDiamond(float cx, float cy, float halfW, float halfH, ColorRGB topColor, ColorRGB skirtColor, float skirtH);
    void drawTexturedDiamond(float cx, float cy, float halfW, float halfH, int texIndex, ColorRGB tintColor, float tintStrength, ColorRGB skirtColor, float skirtH);
    void drawTerrainProp(float cx, float cy, int propIndex, float zoom);
    void drawDiamondOutline(float cx, float cy, float halfW, float halfH, ColorRGB color);
    void drawRect(int x, int y, int w, int h, ColorRGB color);
    void drawRectOutline(int x, int y, int w, int h, ColorRGB color);
    void drawText(int x, int y, const std::string& text, ColorRGB color, int scale = 1);
    void drawSpriteMonument(float cx, float cy, int shrineLevel, ColorRGB color, float zoom = 1.0f);
    void drawVillagerSprite(float footX, float footY, const std::string& spriteName, ColorRGB auraColor, float scale = 1.0f);

    void* getDisplay() const { return m_display; }
    unsigned long getWindow() const { return m_window; }
    void* getHwnd() const { return m_hwnd; }
    bool hasActiveWindow() const { return m_hasActiveWindow; }

    void drawCursor(int mouseX, int mouseY);

    const std::vector<uint32_t>& getFramebuffer() const { return m_framebuffer; }

private:
    int m_width{1280};
    int m_height{720};
    std::vector<uint32_t> m_framebuffer;

    // Windowing internals (X11 / OpenGL / Win32)
    void* m_display{nullptr};
    unsigned long m_window{0};
    void* m_hwnd{nullptr};
    void* m_hdc{nullptr};
    void* m_glContext{nullptr};
    bool m_hasActiveWindow{false};

    // Niebla de Guerra de la Fe (propiedad de Game; nullptr = sin niebla)
    const FaithFogOfWar* m_fog{nullptr};
    bool fogActive() const { return m_fog && m_fog->isEnabled(); }
    int playerReligionId() const { return m_fog ? m_fog->getPlayerReligionId() : 1; }
    // Una unidad/elemento rival sólo se muestra si su celda es visible por la fe del jugador
    bool isHiddenByFog(int cellX, int cellY, int religionId) const {
        return fogActive() && religionId != playerReligionId() && !m_fog->isVisible(cellX, cellY);
    }

    TileSet m_tileSet;
    TileSet m_shamanTileSet;
    std::vector<UIButton> m_buttons;
    std::vector<UIButton> m_dropdownButtons; // sólo clicables mientras el desplegable está abierto
    bool m_dropdownOpen{false};
    std::vector<UIButton> m_menuButtons;
    std::vector<UIButton> m_optionsButtons;
    std::vector<UIButton> m_creationButtons;

    // Fondos cósmico (256x128) y de menú, Cursor RTS, Templo y atlas de terreno
    int m_bgWidth{0};
    int m_bgHeight{0};
    std::vector<uint32_t> m_menuBgPixels;
    std::vector<uint32_t> m_spaceBackgroundPixels;
    std::vector<uint32_t> m_cursorPixels;
    std::vector<uint32_t> m_templePixels;
    std::vector<uint32_t> m_terrainAtlasPixels;
    std::array<std::vector<uint32_t>, 7> m_terrainPropPixels;
    void loadMenuBackground();
    void loadSpaceBackground();
    void drawSpaceBackground();
    void loadCursor();
    void loadTempleAndTerrainAssets();

    void initUI();
    void initMenuUI();
    void renderWorld(const WorldMap& map, const ReligionRegistry& registry,
                     const ReplicationEngine& replication, const GestorHumanos& humanos,
                     const IsometricCamera& camera,
                     const Point2i& selectedCell, RenderMapMode mode);
    void renderTriangulatedFaithAuras(const WorldMap& map, const ReligionRegistry& registry,
                                      const GestorHumanos& humanos, const IsometricCamera& camera);
    void renderUI(const WorldMap& map, const ReligionRegistry& registry,
                  const Timeline& timeline, const EventLine& eventLine,
                  const Point2i& selectedCell, RenderMapMode mode,
                  const GestorHumanos& humanos, const IsometricCamera& camera,
                  const WorldCreationConfig& playerConfig);
    void renderTopBar(const WorldMap& map, const ReligionRegistry& registry,
                      const Timeline& timeline, const WorldCreationConfig& playerConfig);
    void renderBottomBar(const WorldMap& map, const ReligionRegistry& registry,
                         const Timeline& timeline, const EventLine& eventLine,
                         const GestorHumanos& humanos, const IsometricCamera& camera,
                         RenderMapMode mode);
    void renderMinimap(int mmX, int mmY, int mmW, int mmH,
                       const WorldMap& map, const ReligionRegistry& registry,
                       const GestorHumanos& humanos,
                       const IsometricCamera& camera, RenderMapMode mode);
    void renderInspector(const WorldMap& map, const ReligionRegistry& registry,
                         const Point2i& selectedCell);
    void renderTimelineAndEvents(const Timeline& timeline, const EventLine& eventLine);
    void renderOptionsDropdown();
};

} // namespace ReligionSim
