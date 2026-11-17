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
#include "DivinePowersSystem.hpp"
#include "BuildingSystem.hpp"
#include "GameSettings.hpp"
#include <array>
#include <vector>
#include <string>

namespace ReligionSim {

class DayNightCycle;
class CloudSystem;
class NemesisSififusManager;
class ReligiousMoralitySystem;
class TimeEngine;

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

    // EN: Native OS window and graphics context initialization (supports starting directly in fullscreen)
    // ES: Inicialización de ventana nativa del SO y contexto gráfico (soporta arrancar directamente en pantalla completa)
    bool initWindow(const std::string& title = "Marangatu: The Faith Simulator", bool startFullscreen = false);
    void closeWindow();

    void beginFrame();
    void endFrame();

    void render(const WorldMap& map, const ReligionRegistry& registry,
                const Timeline& timeline, const EventLine& eventLine,
                const ReplicationEngine& replication, const GestorHumanos& humanos,
                const IsometricCamera& camera,
                const Point2i& selectedCell, RenderMapMode mode,
                const WorldCreationConfig& playerConfig,
                const AdvancementSystem* advSystem = nullptr,
                const DivinePowersSystem* powersSystem = nullptr,
                Point2i hoverGridCell = {-1, -1},
                const DayNightCycle* dayNight = nullptr,
                const CloudSystem* clouds = nullptr);
    void renderCommandConsole(bool isOpen, const std::string& inputText,
                              bool selectorActive, const std::string& feedbackMsg,
                              bool luminusActive = false);
    void renderWorldBackground(const WorldMap& map, const ReligionRegistry& registry,
                               const ReplicationEngine& replication,
                               const GestorHumanos& humanos,
                               const IsometricCamera& camera,
                               const DayNightCycle* dayNight = nullptr,
                               const CloudSystem* clouds = nullptr);
    void applyCinematicVignette();
    void renderTopBar(const WorldMap& map, const ReligionRegistry& registry,
                      const Timeline& timeline, const WorldCreationConfig& playerConfig,
                      const GestorHumanos* humanos = nullptr,
                      const AdvancementSystem* advSystem = nullptr);

    // Niebla de Guerra de la Fe: la visión proviene sólo del pueblo del jugador
    void setFogOfWar(const FaithFogOfWar* fog) { m_fog = fog; }

    // Ciclo de Día/Noche e Iluminación Dinámica Falsa
    void setDayNightCycle(const DayNightCycle* dn) { m_dayNight = dn; }
    const DayNightCycle* getDayNightCycle() const { return m_dayNight; }

    // Sistema de Capa de Nubes a Altura Determinada
    void setCloudSystem(const CloudSystem* clouds) { m_cloudSystem = clouds; }
    const CloudSystem* getCloudSystem() const { return m_cloudSystem; }

    // Sistema de Viviendas y Asentamientos Tribales
    void setBuildingSystem(const BuildingSystem* bs) { m_buildingSystem = bs; }
    const BuildingSystem* getBuildingSystem() const { return m_buildingSystem; }

    // Desplegable de opciones de partida (botón MENU de la barra superior)
    void setOptionsDropdownOpen(bool open) { m_dropdownOpen = open; }
    bool isOptionsDropdownOpen() const { return m_dropdownOpen; }

    // Ventana Modal de Crónica Sagrada y Puntos de Quiebre (Línea de tiempo horizontal)
    void setChronicleOpen(bool open) { m_chronicleOpen = open; }
    bool isChronicleOpen() const { return m_chronicleOpen; }
    void toggleChronicle() { m_chronicleOpen = !m_chronicleOpen; }
    void setChronicleFilter(int f) { m_chronicleFilter = f; m_chronicleScrollIndex = 0; }
    int getChronicleFilter() const { return m_chronicleFilter; }
    void scrollChronicle(int delta) { m_chronicleScrollIndex = std::max(0, m_chronicleScrollIndex + delta); }
    int getChronicleScroll() const { return m_chronicleScrollIndex; }

    // Ventana Modal del Árbol de Avances de la Fe (Doctrinas, Replicación y Eras)
    void setFaithTreeOpen(bool open) { m_faithTreeOpen = open; }
    bool isFaithTreeOpen() const { return m_faithTreeOpen; }
    void toggleFaithTree() { m_faithTreeOpen = !m_faithTreeOpen; }
    void setSelectedAdvancementId(int id) { m_selectedAdvancementId = id; }
    int getSelectedAdvancementId() const { return m_selectedAdvancementId; }

    // Ventana Modal del Árbol de Recuerdos de la Unidad (Sistema Sísifo & Némesis)
    void setNemesisSififus(const NemesisSififusManager* ns) { m_nemesisSififus = ns; }
    const NemesisSififusManager* getNemesisSififus() const { return m_nemesisSififus; }
    void setMemoryTreeOpen(bool open) { m_memoryTreeOpen = open; }
    bool isMemoryTreeOpen() const { return m_memoryTreeOpen; }
    void toggleMemoryTree() { m_memoryTreeOpen = !m_memoryTreeOpen; }
    void scrollMemoryTree(int delta) { m_memoryTreeScrollIndex = std::max(0, m_memoryTreeScrollIndex + delta); }
    int getMemoryTreeScroll() const { return m_memoryTreeScrollIndex; }
    void renderMemoryTreeModal(const GestorHumanos& humanos, const ReligionRegistry& registry,
                               const Timeline& timeline, float mouseX, float mouseY, bool isMouseDown);
    int checkMemoryTreeModalClick(float mouseX, float mouseY);

    // Ventana Modal del Sistema de Moral Religiosa y Reglas (Mandatos, Tabúes, Gracia y Culpa)
    void setMoralitySystem(const ReligiousMoralitySystem* ms) { m_moralitySystem = ms; }
    const ReligiousMoralitySystem* getMoralitySystem() const { return m_moralitySystem; }
    void setMoralRulesModalOpen(bool open) { m_moralRulesModalOpen = open; }
    bool isMoralRulesModalOpen() const { return m_moralRulesModalOpen; }
    void toggleMoralRulesModal() { m_moralRulesModalOpen = !m_moralRulesModalOpen; }
    void renderMoralRulesModal(const ReligiousMoralitySystem& morality, int playerReligionId,
                               const ReligionRegistry& registry, const GestorHumanos& humanos,
                               const Timeline& timeline, float mouseX, float mouseY, bool isMouseDown);
    int checkMoralRulesModalClick(float mouseX, float mouseY, ReligiousMoralitySystem& morality,
                                  int playerReligionId, ReligionRegistry& registry);

    // Sistema de Poderes Divinos del Usuario e Intermediario (Chamán o Profeta)
    void setActiveDivinePower(DivinePowerType p) { m_activeDivinePower = p; }
    DivinePowerType getActiveDivinePower() const { return m_activeDivinePower; }
    void renderDivineEffects(const DivinePowersSystem& powers, const IsometricCamera& camera, const WorldMap& map);
    void renderTargetingReticle(const Point2i& hoverCell, DivinePowerType power, const IsometricCamera& camera, const WorldMap& map);
    void renderPreacherSpeeches(const DivinePowersSystem& powers, const IsometricCamera& camera, const WorldMap& map);

    // Rectángulo del minimapa dentro del panel inferior derecho (compartido con el hit-test de Game)
    void getMinimapRect(int& x, int& y, int& w, int& h) const;
    void getMinimapRect(int& x, int& y, int& size) const;

    // Pantalla de Menú Principal, Creación de Mundo y Opciones
    void renderMainMenu(int hoveredButtonId);
    void renderOptionsMenu(int hoveredButtonId);
    void renderOptionsMenu(const GameSettings& settings, OptionsTab currentTab, int hoveredButtonId);
    // EN: Render the sacred "Learn to Play" tutorial and guide codex modal
    // ES: Renderizar el modal del códice sagrado de tutorial y guía "Aprende a jugar"
    void renderTutorialMenu(TutorialTab currentTab, int hoveredButtonId);
    void renderWorldCreationMenu(const WorldCreationConfig& config, int hoveredButtonId, int activeInputField);
    void renderLoadWorldMenu(int hoveredButtonId, int selectedSaveIndex);
    void renderGameOver(const WorldCreationConfig& config, int currentYear, int eraIndex, int hoveredButtonId);
    // EN: Post-defeat spectator HUD and world inspection controls
    // ES: HUD de espectador tras la derrota y controles de inspección del mundo
    void renderPostDefeatSpectatorHUD(const WorldCreationConfig& config, int extinctionYear,
                                     const Point2i& hoveredGridCell, const WorldMap& map,
                                     const IsometricCamera& camera, bool revealAllFog,
                                     int hoveredButtonId);

    // EN: Dedicated Map Editor / Creation mode rendering and interaction
    // ES: Renderizado e interacción del modo dedicado de Creación y Edición de Mapas
    void renderMapEditor(const MapEditorConfig& config, const WorldMap& editorMap,
                         int hoveredButtonId, int activeInputField,
                         const Point2i& hoveredCell);
    void renderMapEditorIsometric(const MapEditorConfig& config, const WorldMap& editorMap,
                                  const IsometricCamera& camera, int hoveredButtonId,
                                  const Point2i& hoveredCell, const ReligionRegistry& registry,
                                  const ReplicationEngine& replication, const GestorHumanos& humanos);
    int checkMapEditorButtonClicked(float mouseX, float mouseY) const;
    int checkMapEditorIsometricButtonClicked(float mouseX, float mouseY) const;
    void initMapEditorIsometricUI();
    Point2i screenToMapEditorCell(float mouseX, float mouseY, const WorldMap& editorMap) const;
    bool isInsideMapEditorPreview(float mouseX, float mouseY, const WorldMap& editorMap) const;
    const std::vector<UIButton>& getMapEditorButtons() const { return m_mapEditorButtons; }
    const std::vector<UIButton>& getMapEditorIsometricButtons() const { return m_mapEditorIsoButtons; }

    int checkMenuButtonClicked(float mouseX, float mouseY, bool inOptions = false) const;
    int checkOptionsMenuButtonClicked(float mouseX, float mouseY, OptionsTab currentTab) const;
    int checkTutorialButtonClicked(float mouseX, float mouseY) const;
    int checkCreationButtonClicked(float mouseX, float mouseY) const;
    int checkLoadMenuButtonClicked(float mouseX, float mouseY, int savesCount) const;
    int checkGameOverButtonClicked(float mouseX, float mouseY) const;
    int checkSpectatorButtonClicked(float mouseX, float mouseY) const;
    const std::vector<UIButton>& getCreationButtons() const { return m_creationButtons; }
    const std::vector<UIButton>& getMenuButtons() const { return m_menuButtons; }
    const std::vector<UIButton>& getTutorialButtons() const { return m_tutorialButtons; }
    const std::vector<UIButton>& getTutorialTabButtons() const { return m_tutorialTabButtons; }

    // Save snapshot to PPM/BMP file
    bool exportFrameToPPM(const std::string& filepath);

    // Viewport dimensions & dynamic resize
    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }
    void resize(int newWidth, int newHeight);

    // EN: Window display mode, fullscreen and dimension controls
    // ES: Modo de visualización de ventana, controles de pantalla completa y dimensiones
    void setFullscreen(bool enable);
    bool isFullscreen() const { return m_isFullscreen; }
    void setWindowSize(int newWidth, int newHeight);
    void setWindowedResolution(int windowedWidth, int windowedHeight);

    // EN: Terrain isometric grid visualization controls (disabled by default)
    // ES: Controles de visualización de retícula isométrica de terreno (desactivada por defecto)
    void setShowGrid(bool showGrid) { m_showGrid = showGrid; }
    [[nodiscard]] bool isGridVisible() const { return m_showGrid; }
    void toggleGrid() { m_showGrid = !m_showGrid; }

    // EN: Developer mode telemetry and visual debugging
    // ES: Telemetría del modo desarrollador y depuración visual
    void setDeveloperMode(bool enable) { m_developerMode = enable; }
    [[nodiscard]] bool isDeveloperMode() const { return m_developerMode; }
    void toggleDeveloperMode() { m_developerMode = !m_developerMode; }

    // EN: Render comprehensive developer HUD, unit inspector and isometric 3D axes
    // ES: Renderizar HUD completo de desarrollador, inspector de unidades y ejes 3D isométricos
    void renderDeveloperOverlay(const WorldMap& map, const ReligionRegistry& registry,
                                const Timeline& timeline, const GestorHumanos& humanos,
                                const IsometricCamera& camera, const Point2i& hoveredCell,
                                const Humano* hoveredVillager, const Humano* selectedVillager,
                                const WorldCreationConfig& playerConfig,
                                const TimeEngine* timeEngine = nullptr,
                                const DayNightCycle* dayNight = nullptr);

    // EN: Helper routine to render 3D coordinate axes and tile boundary on hovered terrain cell
    // ES: Rutina auxiliar para renderizar ejes coordenados 3D y límite de celda sobre la celda de terreno señalada
    void renderDeveloperCellAxes(const Point2i& cellCoords, const IsometricCamera& camera, const WorldMap& map);

    // EN: Helper routine to render per-unit developer status badges, local axes and collision footprints
    // ES: Rutina auxiliar para renderizar distintivos de estado, ejes locales y huellas de colisión por unidad
    void renderDeveloperUnitOverlays(const GestorHumanos& humanos, const IsometricCamera& camera,
                                    const WorldMap& map, const Humano* hoveredVillager,
                                    const Humano* selectedVillager);

    // EN: Helper routine to render screen-space 3D orientation compass gizmo
    // ES: Rutina auxiliar para renderizar el gizmo brújula de orientación 3D en espacio de pantalla
    void renderDeveloperOrientationGizmo(int screenPosX, int screenPosY);

    TileSet& getTileSet() { return m_tileSet; }
    const TileSet& getTileSet() const { return m_tileSet; }
    const std::array<std::vector<uint32_t>, 7>& getResourcePropPixels() const { return m_resourcePropPixels; }
    const std::array<std::array<std::vector<uint32_t>, 5>, 7>& getResourceVariantPixels() const { return m_resourceVariantPixels; }
    const std::vector<uint32_t>& getPowerIconPixels() const { return m_powerIconPixels; }
    int getPowerIconWidth() const { return m_powerIconW; }
    int getPowerIconHeight() const { return m_powerIconH; }

    // Hit-testing for UI elements
    int checkButtonClicked(float mouseX, float mouseY) const;

    // Actualización y estados interactivos de UI (hover y animación de pulsado)
    void update(float deltaTime);
    void setMousePosition(float mouseX, float mouseY);
    void setMouseDown(bool isDown, float mouseX, float mouseY);
    void triggerButtonPress(int buttonId);
    void drawAntiqueButton(int bx, int by, int bw, int bh,
                           const std::string& label,
                           bool active, bool hovered, bool pressed, int btnId = 0);
    // EN: Round play button with centered play triangle glyph, metallic liturgical circular bezel
    // ES: Botón redondo de play con glifo triangular centrado, bisel circular metálico litúrgico
    void drawRoundPlayButton(int bx, int by, int diameter, bool active, bool hovered, bool pressed);
    // EN: Vintage cassette mechanical piano keys that depress on click and spring back up
    // ES: Teclas mecánicas de casetera vintage que se hunden al pulsar y regresan elásticamente
    void drawCassetteButton(int bx, int by, int bw, int bh, const std::string& label,
                            bool active, bool hovered, bool pressed, int btnId);
    void drawViewModeButton(int bx, int by, int bw, int bh, int btnId,
                           const std::string& label,
                           bool active, bool hovered, bool pressed);
    void drawPowerButton(int bx, int by, int bw, int bh, int btnId,
                         const std::string& label,
                         bool active, bool hovered, bool pressed);
    void drawEventIcon(int x, int y, int categoryCol, bool glow = false);
    void renderChronicleModal(const EventLine& eventLine, const Timeline& timeline,
                             int activeFilter, int scrollIndex,
                             float mouseX, float mouseY, bool isMouseDown);
    int checkChronicleModalClick(float mouseX, float mouseY, const EventLine& eventLine,
                                 int activeFilter, int scrollIndex, Point2i& outJumpTarget);

    void drawFaithTreeIcon(int x, int y, int iconCol, bool glow = false);
    void renderFaithTreeModal(const ReligionRegistry& registry,
                              const AdvancementSystem& advSystem,
                              const WorldCreationConfig& playerConfig,
                              int selectedAdvId,
                              float mouseX, float mouseY, bool isMouseDown);
    int checkFaithTreeModalClick(float mouseX, float mouseY,
                                 const ReligionRegistry& registry,
                                 const AdvancementSystem& advSystem,
                                 int& outSelectedAdvId);

    // EN: Visual post-processing color grading and options modal initialization
    // ES: Postprocesamiento visual de gradación de color e inicialización del modal de opciones
    void applyVisualColorFilter(VisualColorFilter filter);
    void initOptionsMenuUI();

    // Direct pixel drawing routines for software rasterizer
    void clear(ColorRGB color);
    void drawLine(int x0, int y0, int x1, int y1, ColorRGB color);
    void drawFilledDiamond(float cx, float cy, float halfW, float halfH, ColorRGB topColor, ColorRGB skirtColor, float skirtH);
    void drawTexturedDiamond(float cx, float cy, float halfW, float halfH, int texIndex, ColorRGB tintColor, float tintStrength, ColorRGB skirtColor, float skirtH);
    void drawTerrainProp(float cx, float cy, int propIndex, float zoom, float opacity = 1.0f);
    void drawDestructionProp(float cx, float cy, int propIndex, float zoom, float opacity = 1.0f);
    void drawCraterSprite(float cx, float cy, int craterIndex, float zoom, float opacity = 1.0f);
    void drawResourceNode(float cx, float cy, ResourceType type, float zoom, int amount = 100, float opacity = 1.0f,
                          uint8_t variant = 0, int maxAmount = 100);
    void drawVolcano(float cx, float cy, float zoom, float animTime, bool isErupting = false, float eruptionProgress = 0.0f);
    void drawFireEffect(float cx, float cy, float intensity, float zoom, float animTime, float windVx = 0.0f, float windVy = 0.0f);
    void drawCampfire(float cx, float cy, float zoom, float animTime);
    void drawSteamEffect(float cx, float cy, float timer, float zoom, float animTime);
    void drawWhirlpool(float centerX, float centerY, float zoom, float animTime, float intensity = 1.0f);
    void drawWaterSplash(float centerX, float centerY, float zoom, float animTime, float scale = 1.0f);
    void drawDiamondOutline(float cx, float cy, float halfW, float halfH, ColorRGB color);
    void drawRect(int x, int y, int w, int h, ColorRGB color);
    void drawRectOutline(int x, int y, int w, int h, ColorRGB color);

    // EN: Bounding box queries for isometric sprites and dynamic occlusion detection
    // ES: Consultas de caja delimitadora para sprites isométricos y detección dinámica de oclusión
    void getTerrainPropBounds(float cx, float cy, int propIndex, float zoom,
                              int& outStartX, int& outStartY, int& outWidth, int& outHeight) const;
    void getDestructionPropBounds(float cx, float cy, int propIndex, float zoom,
                                  int& outStartX, int& outStartY, int& outWidth, int& outHeight) const;
    void getCraterBounds(float cx, float cy, int craterIndex, float zoom,
                         int& outStartX, int& outStartY, int& outWidth, int& outHeight) const;
    void getResourceNodeBounds(float cx, float cy, ResourceType type, float zoom,
                               int& outStartX, int& outStartY, int& outWidth, int& outHeight) const;
    void getHouseBounds(float cx, float cy, const House& house, float zoom,
                        int& outStartX, int& outStartY, int& outWidth, int& outHeight) const;
    void getMonumentBounds(float cx, float cy, int shrineLevel, float zoom,
                           int& outStartX, int& outStartY, int& outWidth, int& outHeight) const;

    // Renderizado procedural de paneles y medidores estilo Sims 2 (volumen satinado, biseles y barras cilíndricas)
    void drawSims2Panel(int x, int y, int w, int h, ColorRGB baseColor, bool sunken = false, ColorRGB rimColor = Palette::DIVINE_GOLD);
    void drawSims2Bar(int x, int y, int w, int h, ColorRGB barColor);
    void drawSims2Meter(int x, int y, int w, int h, float progress, ColorRGB barColor, ColorRGB trayColor = ColorRGB{12, 7, 6});
    void drawText(int x, int y, const std::string& text, ColorRGB color, int scale = 1);
    void drawSpriteMonument(float cx, float cy, int shrineLevel, ColorRGB color, float zoom = 1.0f, float opacity = 1.0f);
    void drawSpriteHouse(float cx, float cy, const House& house, float zoom, bool isNight = false, float opacity = 1.0f);
    void drawVillagerSprite(float footX, float footY, const std::string& spriteName, ColorRGB auraColor, float scale = 1.0f);

    void* getDisplay() const { return m_display; }
    unsigned long getWindow() const { return m_window; }
    void* getHwnd() const { return m_hwnd; }
    bool hasActiveWindow() const { return m_hasActiveWindow; }

    // EN: Accessors for fire sprite pixel buffers used in testing and diagnostics
    // ES: Métodos de acceso para los búferes de píxeles de fuego usados en pruebas y diagnósticos
    const std::vector<uint32_t>& getWildfirePixels() const { return m_wildfirePixels; }
    const std::vector<uint32_t>& getFlamePixels() const { return m_flamePixels; }
    const std::vector<uint32_t>& getCampfirePixels() const { return m_campfirePixels; }
    const std::vector<uint32_t>& getMeteorGiantPixels() const { return m_meteorGiantPixels; }
    const std::vector<uint32_t>& getMeteorFireballPixels() const { return m_meteorFireballPixels; }
    const std::vector<uint32_t>& getVolcanoEruptionPixels() const { return m_volcanoEruptionPixels; }
    const std::vector<uint32_t>& getFirePitPixels() const { return m_firePitPixels; }
    const std::vector<uint32_t>& getFlameBurstPixels() const { return m_flameBurstPixels; }
    const std::vector<uint32_t>& getLavaTilesPixels() const { return m_lavaTilesPixels; }
    const std::vector<uint32_t>& getVolcanoSheetPixels() const { return m_volcanoSheetPixels; }
    const std::vector<uint32_t>& getAtomicBombSheetPixels() const { return m_atomicBombSheetPixels; }
    const std::vector<uint32_t>& getTornadoSheetPixels() const { return m_tornadoSheetPixels; }

    // EN: Accessors for water sprite pixel buffers used in testing and diagnostics
    // ES: Métodos de acceso para los búferes de píxeles de agua usados en pruebas y diagnósticos
    const std::vector<uint32_t>& getWaterSheetPixels() const { return m_waterSheetPixels; }
    const std::vector<uint32_t>& getWaterRipplesPixels() const { return m_waterRipplesPixels; }
    const std::vector<uint32_t>& getWaterBordersPixels() const { return m_waterBordersPixels; }
    const std::vector<uint32_t>& getWaterWavesPixels() const { return m_waterWavesPixels; }
    const std::vector<uint32_t>& getWaterSurfPixels() const { return m_waterSurfPixels; }
    const std::vector<uint32_t>& getWaterCurrentsPixels() const { return m_waterCurrentsPixels; }
    const std::vector<uint32_t>& getWaterWhirlpoolPixels() const { return m_waterWhirlpoolPixels; }
    const std::vector<uint32_t>& getWaterSplashPixels() const { return m_waterSplashPixels; }

    // EN: Accessors for destruction sprite pixel buffers used in testing and diagnostics
    // ES: Métodos de acceso para los búferes de píxeles de destrucción usados en pruebas y diagnósticos
    const std::vector<std::vector<uint32_t>>& getDestructionTreePixels() const { return m_destructionTreePixels; }
    const std::vector<std::vector<uint32_t>>& getDestructionCraterPixels() const { return m_destructionCraterPixels; }
    const std::vector<std::vector<uint32_t>>& getDestructionTerrainPixels() const { return m_destructionTerrainPixels; }

    void drawCursor(int mouseX, int mouseY);
    void renderInspector(const WorldMap& map, const ReligionRegistry& registry,
                         const Point2i& selectedCell);

    const std::vector<uint32_t>& getFramebuffer() const { return m_framebuffer; }

    const std::vector<UIButton>& getButtons() const { return m_buttons; }

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
    bool m_isFullscreen{false};
    int m_windowedWidth{1280};
    int m_windowedHeight{720};
    int m_windowedX{50};
    int m_windowedY{50};

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
    TileSet m_femaleTileSet;
    const TileSet& getFemaleTileSet() const { return m_femaleTileSet; }
    std::vector<UIButton> m_buttons;
    std::vector<UIButton> m_dropdownButtons; // sólo clicables mientras el desplegable está abierto
    bool m_dropdownOpen{false};
    std::vector<UIButton> m_menuButtons;
    std::vector<UIButton> m_optionsButtons;
    std::vector<UIButton> m_optionsTabButtons;
    std::vector<UIButton> m_optionsScreenButtons;
    std::vector<UIButton> m_optionsGraphicsButtons;
    std::vector<UIButton> m_optionsGameplayButtons;
    std::vector<UIButton> m_optionsAudioButtons;
    std::vector<UIButton> m_optionsActionButtons;
    std::vector<UIButton> m_creationButtons;
    std::vector<UIButton> m_mapEditorButtons;
    std::vector<UIButton> m_mapEditorIsoButtons;
    std::vector<UIButton> m_tutorialButtons;
    std::vector<UIButton> m_tutorialTabButtons;

    // Fondos cósmico (256x128) y de menú, Cursor RTS, Templo y atlas de terreno
    int m_bgWidth{0};
    int m_bgHeight{0};
    std::vector<uint32_t> m_menuBgPixels;
    std::vector<uint32_t> m_spaceBackgroundPixels;
    std::vector<uint32_t> m_cursorPixels;
    std::vector<uint32_t> m_templePixels;
    std::vector<uint32_t> m_buildingsPixels;
    int m_buildingsWidth{0};
    int m_buildingsHeight{0};
    const BuildingSystem* m_buildingSystem{nullptr};
    std::vector<uint32_t> m_terrainAtlasPixels;
    std::vector<uint32_t> m_seamlessAtlasPixels;
    std::array<std::vector<uint32_t>, 7> m_terrainPropPixels;
    std::array<std::vector<uint32_t>, 7> m_resourcePropPixels; // Food=0, Wood=1, Stone=2, Gold=3, Iron=4, Copper=5, Diamond=6
    // EN: Multi-variant textures indexed by [resourceIndex][variantIndex] (64x64 RGBA)
    // ES: Texturas de variantes múltiples indexadas por [resourceIndex][variantIndex] (64x64 RGBA)
    std::array<std::array<std::vector<uint32_t>, 5>, 7> m_resourceVariantPixels;
    void loadMenuBackground();
    void loadSpaceBackground();
    void drawSpaceBackground();
    void loadCursor();
    void loadTempleAndTerrainAssets();
    void loadResourceAssets();
    void loadBuildingAssets();
    void loadDestructionAssets();

    // EN: Destruction tileset assets: burnt trees, charred stumps, craters, and scorched terrain
    // ES: Activos de tileset de destrucción: árboles quemados, tocones calcinados, cráteres y tierra quemada
    std::vector<std::vector<uint32_t>> m_destructionTreePixels;
    std::vector<std::vector<uint32_t>> m_destructionCraterPixels;
    std::vector<std::vector<uint32_t>> m_destructionTerrainPixels;

    // Sprites de botones antiguos 2D y estado interactivo
    std::vector<uint32_t> m_buttonAtlasPixels;
    int m_buttonAtlasW{256};
    int m_buttonAtlasH{128};
    float m_mouseX{0.0f};
    float m_mouseY{0.0f};
    bool m_isMouseDown{false};
    int m_pressedButtonId{0};
    float m_pressedAnimTimer{0.0f};
    float m_animTime{0.0f};
    void loadButtonAssets();

    // Sprites de iconos de vistas de mapa (FE, BIOMAS, UNIDADES, SINCRETISMO, ERAS)
    std::vector<uint32_t> m_viewIconPixels;
    int m_viewIconW{80};
    int m_viewIconH{64};
    void loadViewIconAssets();

    // Sprites de iconos de poderes divinos y catástrofes (Rayo, Plaga, Milagro, Congregar, Tornado, Terremoto, Pandemia)
    std::vector<uint32_t> m_powerIconPixels;
    int m_powerIconW{112};
    int m_powerIconH{64};
    void loadPowerIconAssets();

    // Sprites de iconos de eventos para la Crónica Histórica (Muerte/Pérdida, Avance, Descubrimiento, Cisma, Sincretismo, Monumento)
    std::vector<uint32_t> m_eventIconPixels;
    int m_eventIconW{144};
    int m_eventIconH{48};
    void loadEventIconAssets();

    bool m_chronicleOpen{false};
    int m_chronicleFilter{0};
    int m_chronicleScrollIndex{0};

    // Sprites de iconos del Árbol de Avances de la Fe (192x48: 8 columnas x 2 filas)
    std::vector<uint32_t> m_faithTreeIconPixels;
    int m_faithTreeIconW{192};
    int m_faithTreeIconH{48};
    void loadFaithTreeIconAssets();

    // EN: Fire sprite textures, campfires, meteorites, volcanic fountains and magma assets
    // ES: Texturas de sprites de fuego, hogueras, meteoritos, fuentes volcánicas y magma
    std::vector<uint32_t> m_wildfirePixels;          // 512x128 (4 frames 128x128)
    std::vector<uint32_t> m_flamePixels;             // 384x128 (6 frames 64x128)
    std::vector<uint32_t> m_campfirePixels;          // 512x128 (4 frames 128x128)
    std::vector<uint32_t> m_meteorGiantPixels;       // 128x128
    std::vector<uint32_t> m_meteorFireballPixels;    // 128x128
    std::vector<uint32_t> m_volcanoEruptionPixels;   // 256x128 (2 frames 128x128)
    std::vector<uint32_t> m_firePitPixels;           // 128x128
    std::vector<uint32_t> m_flameBurstPixels;        // 256x128 (2 frames 128x128)
    std::vector<uint32_t> m_lavaTilesPixels;         // 512x256 (4 tiles 128x128)
    std::vector<uint32_t> m_volcanoSheetPixels;      // 1024x765 (5x5 matrix of volcano evolution & eruption stages)
    int m_volcanoSheetW{1024};
    int m_volcanoSheetH{765};
    std::vector<uint32_t> m_atomicBombSheetPixels;   // 1024x576 (5x4 matrix of 19 nuclear explosion animation frames)
    int m_atomicBombSheetW{1024};
    int m_atomicBombSheetH{576};
    std::vector<uint32_t> m_tornadoSheetPixels;      // 384x128 (6 frames 64x128 animated cyclonic vortex)
    void loadFireAssets();

    // EN: Water animation sprite textures, ripples, ocean waves, coastal surf, currents, whirlpools and splashes
    // ES: Texturas de sprites de animación de agua, ondas, olas oceánicas, espuma costera, corrientes, remolinos y salpicaduras
    std::vector<uint32_t> m_waterSheetPixels;         // 1024x765 master water sheet
    std::vector<uint32_t> m_waterRipplesPixels;       // 1024x170 calm & rippling surface animation
    std::vector<uint32_t> m_waterBordersPixels;       // 1024x175 coastal autotile borders
    std::vector<uint32_t> m_waterWavesPixels;         // 1024x115 great ocean swells and cresting waves
    std::vector<uint32_t> m_waterSurfPixels;          // 1024x116 shoreline surf and breaking foam
    std::vector<uint32_t> m_waterCurrentsPixels;      // 853x102 directional streamflow currents
    std::vector<uint32_t> m_waterWhirlpoolPixels;     // 510x85 (6 frames 85x85) spinning vortex
    std::vector<uint32_t> m_waterSplashPixels;        // 425x85 (5 frames 85x85) droplet splashes
    int m_waterSheetW{1024};
    int m_waterSheetH{765};
    void loadWaterAssets();

    bool m_faithTreeOpen{false};
    int m_selectedAdvancementId{101};

    // Modal del Árbol de Recuerdos (Sistema Sísifo & Némesis)
    bool m_memoryTreeOpen{false};
    int m_memoryTreeScrollIndex{0};
    const NemesisSififusManager* m_nemesisSififus{nullptr};

    // Modal del Sistema de Moral Religiosa y Reglas (Mandatos, Tabúes, Gracia y Culpa)
    bool m_moralRulesModalOpen{false};
    const ReligiousMoralitySystem* m_moralitySystem{nullptr};

    DivinePowerType m_activeDivinePower{DivinePowerType::Inactive};

    void initUI();
    void initMenuUI();
    void initTutorialUI();
    void initMapEditorUI();
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
                  const WorldCreationConfig& playerConfig,
                  const AdvancementSystem* advSystem = nullptr);
    void renderBottomBar(const WorldMap& map, const ReligionRegistry& registry,
                         const Timeline& timeline, const EventLine& eventLine,
                         const GestorHumanos& humanos, const IsometricCamera& camera,
                         RenderMapMode mode);
    void renderMinimap(int mmX, int mmY, int mmW, int mmH,
                       const WorldMap& map, const ReligionRegistry& registry,
                       const GestorHumanos& humanos,
                       const IsometricCamera& camera, RenderMapMode mode);
    void renderTimelineAndEvents(const Timeline& timeline, const EventLine& eventLine);
    void renderOptionsDropdown();

    // Sistema de Día/Noche, Cuerpos Celestiales e Iluminación Dinámica
    const DayNightCycle* m_dayNight{nullptr};
    const CloudSystem* m_cloudSystem{nullptr};
    float m_cameraZoom{0.85f};
    float m_cameraX{0.0f};
    float m_cameraY{0.0f};
    int m_mapWidth{64};
    int m_mapHeight{64};
    std::vector<float> m_lightmapR;
    std::vector<float> m_lightmapG;
    std::vector<float> m_lightmapB;

    struct CelestialStar {
        float x;
        float y;
        float parallax;
        float brightness;
        float twinkleSpeed;
        float twinkleSeed;
        uint8_t size;
        uint8_t r;
        uint8_t g;
        uint8_t b;
    };
    std::vector<CelestialStar> m_stars;
    void initStars();

    // Búfer precomputado de nebulosas cósmicas para acelerar el cielo de 12 FPS a 60+ FPS
    static constexpr int COSMIC_BUFFER_WIDTH = 2048;
    static constexpr int COSMIC_BUFFER_HEIGHT = 1024;
    std::vector<uint32_t> m_cosmicBuffer;
    void initCosmicBuffer();
    bool m_skyDrawnThisFrame{false};
    bool m_showGrid{false}; // EN: By default disabled (seamless terrain) / ES: Desactivado por defecto (terreno continuo)
    bool m_developerMode{false}; // EN: Developer mode telemetry and axes overlay / ES: Capa de telemetría y ejes del modo desarrollador

    // Tablas precomputadas para splatting y cohesión orgánica de terreno
    float m_turbTableX[64][64];
    float m_turbTableY[64][64];
    float m_ditherTable[64][64];
    void initTerrainTables();

    void drawCelestialSky(const DayNightCycle& dayNight);
    void renderCelestialBodies(const DayNightCycle& dayNight, const IsometricCamera& camera, int mapWidth, int mapHeight);
    void drawSun(float cx, float cy, float alpha, float timer, bool onlyInSky = false);
    void drawMoon(float cx, float cy, float alpha, float timer, bool onlyInSky = false);
    void applyFakeDynamicLighting(const DayNightCycle& dayNight,
                                 const WorldMap& map,
                                 const GestorHumanos& humanos,
                                 const DivinePowersSystem* powersSystem,
                                 const IsometricCamera& camera);

    // Renderizado de Capa de Nubes volumétricas confinadas estrictamente sobre el mapa y Sombras
    void renderClouds(const CloudSystem& clouds, const IsometricCamera& camera, const DayNightCycle* dayNight, int mapWidth, int mapHeight);
    void renderCloudShadows(const CloudSystem& clouds, const IsometricCamera& camera, const DayNightCycle* dayNight, int mapWidth, int mapHeight);
};

} // namespace ReligionSim
