#pragma once

#include "Types.hpp"
#include "IsometricCamera.hpp"
#include "WorldMap.hpp"
#include "Religion.hpp"
#include "AdvancementSystem.hpp"
#include "ReplicationEngine.hpp"
#include "AgglutinationEngine.hpp"
#include "Villager.hpp"
#include "Humano.hpp"
#include "Timeline.hpp"
#include "TimeEngine.hpp"
#include "EventLine.hpp"
#include "Renderer.hpp"
#include "FogOfWar.hpp"
#include "DivinePowersSystem.hpp"
#include "DayNightCycle.hpp"
#include "CloudSystem.hpp"
#include "SoundSystem.hpp"
#include "NemesisSififus.hpp"
#include "ReligiousMoralitySystem.hpp"
#include "BuildingSystem.hpp"
#include "ErrorSystem.hpp"
#include "GameSettings.hpp"
#include <memory>
#include <string>

namespace ReligionSim {

class Game {
public:
    Game(int windowWidth = 1280, int windowHeight = 720);
    ~Game();

    bool init();
    void run();
    void runHeadless(int maxTicks = 200, const std::string& exportPPM = "snapshot.ppm", bool revealFog = false);

    void update(float deltaTime);
    void render();

    // EN: Input event handlers with descriptive screen coordinates
    // ES: Manejadores de eventos de entrada con coordenadas descriptivas de pantalla
    void onMouseDown(int mouseButton, float screenPosX, float screenPosY);
    void onMouseUp(int mouseButton, float screenPosX, float screenPosY);
    void onMouseMove(float screenPosX, float screenPosY);
    void onMouseWheel(float wheelDelta, float screenPosX, float screenPosY);
    void onKeyPress(int keySym);

    // EN: Dynamic window resize handler / ES: Manejador de redimensionado dinámico de ventana
    void onWindowResize(int newWidth, int newHeight);

    // EN: Gameplay interaction commands / ES: Comandos de interacción de juego
    void selectCell(int gridCellX, int gridCellY);
    void triggerSyncretismAtSelection();
    void triggerSchismAtSelection();
    void buildHolySiteAtSelection();

    // EN: Divine Powers and Intermediary system (Shaman / Prophet)
    // ES: Sistema de Poderes Divinos e Intermediario (Chamán / Profeta)
    void castDivinePower(DivinePowerType divineType, int targetGridX, int targetGridY);
    void toggleDivinePowerTargeting(DivinePowerType type);
    DivinePowerType getActiveDivinePower() const { return m_activeDivinePower; }
    const DivinePowersSystem& getDivinePowers() const { return m_divinePowers; }
    DivinePowersSystem& getDivinePowers() { return m_divinePowers; }
    const DayNightCycle& getDayNightCycle() const { return m_dayNightCycle; }
    DayNightCycle& getDayNightCycle() { return m_dayNightCycle; }
    const CloudSystem& getCloudSystem() const { return m_cloudSystem; }
    CloudSystem& getCloudSystem() { return m_cloudSystem; }
    const SoundSystem& getSoundSystem() const { return m_soundSystem; }
    SoundSystem& getSoundSystem() { return m_soundSystem; }

    // Accessors for testing
    const WorldMap& getMap() const { return m_map; }
    WorldMap& getMap() { return m_map; }
    const IsometricCamera& getCamera() const { return m_camera; }
    IsometricCamera& getCamera() { return m_camera; }
    const ReligionRegistry& getRegistry() const { return m_registry; }
    ReligionRegistry& getRegistryMutable() { return m_registry; }
    int getPlayerReligionId() const { return resolvePlayerReligionId(); }
    const Timeline& getTimeline() const { return m_timeline; }
    Timeline& getTimelineMutable() { return m_timeline; }
    const EventLine& getEventLine() const { return m_eventLine; }
    EventLine& getEventLine() { return m_eventLine; }
    const TimeEngine& getTimeEngine() const { return m_timeEngine; }
    TimeEngine& getTimeEngine() { return m_timeEngine; }
    const GestorHumanos& getGestorHumanos() const { return m_gestorHumanos; }
    GestorHumanos& getGestorHumanos() { return m_gestorHumanos; }
    RenderMapMode getMapMode() const { return m_mapMode; }
    GameState getGameState() const { return m_gameState; }
    void setGameState(GameState state) { m_gameState = state; }
    const WorldCreationConfig& getCreationConfig() const { return m_creationConfig; }
    WorldCreationConfig& getCreationConfig() { return m_creationConfig; }
    const Renderer& getRenderer() const { return m_renderer; }
    Renderer& getRenderer() { return m_renderer; }
    const NemesisSififusManager& getNemesisSififus() const { return m_nemesisSififus; }
    NemesisSififusManager& getNemesisSififus() { return m_nemesisSififus; }
    const ReligiousMoralitySystem& getMoralitySystem() const { return m_moralitySystem; }
    ReligiousMoralitySystem& getMoralitySystem() { return m_moralitySystem; }
    const BuildingSystem& getBuildingSystem() const { return m_buildingSystem; }
    BuildingSystem& getBuildingSystem() { return m_buildingSystem; }
    const FaithFogOfWar& getFogOfWar() const { return m_fog; }
    bool isLuminusActive() const { return m_cheatLuminusActive; }

    // EN: Developer mode telemetry and visual debugging
    // ES: Telemetría del modo desarrollador y depuración visual
    void toggleDeveloperMode();
    void setDeveloperMode(bool enable);
    [[nodiscard]] bool isDeveloperMode() const noexcept { return m_developerMode; }

    // EN: Error and performance monitoring sentinel
    // ES: Centinela de monitoreo de errores y rendimiento
    const ErrorSystem& getErrorSystem() const { return ErrorSystem::instance(); }
    ErrorSystem& getErrorSystem() { return ErrorSystem::instance(); }
    void performEmergencyMemoryPurge();

    void runCommand(const std::string& cmd) { executeCommand(cmd); }
    void stop() { m_isRunning = false; }

    void startNewCustomWorld();
    void prepareForWorldCreation(bool generateNewSeed = true);
    void returnToMainMenu();
    void resetAllGameSubsystems();
    static uint32_t generateUniqueSeed();

    const std::string& getCurrentSaveSlot() const { return m_currentSaveSlot; }
    void setCurrentSaveSlot(const std::string& slot) { m_currentSaveSlot = slot; }

    void loadStateFromSave(const WorldMap& map, const ReligionRegistry& registry,
                           int currentYear, const std::string& godName,
                           const std::string& religionName, uint32_t seed,
                           bool villagersLoadedFromSnapshot = false,
                           float timeOfDay = 0.375f,
                           const std::string& slotName = "",
                           int daysPassed = 0,
                           int advancementBonusYears = 0,
                           int startYear = -4000);
    void saveCurrentGame(const std::string& slotName = "");

    // EN: Settings accessors and synchronization routines
    // ES: Métodos de acceso a ajustes y rutinas de sincronización
    const GameSettings& getSettings() const { return m_settings; }
    GameSettings& getSettingsMutable() { return m_settings; }
    OptionsTab getCurrentOptionsTab() const { return m_currentOptionsTab; }
    void setCurrentOptionsTab(OptionsTab tab) { m_currentOptionsTab = tab; }
    TutorialTab getCurrentTutorialTab() const { return m_currentTutorialTab; }
    void setCurrentTutorialTab(TutorialTab tab) { m_currentTutorialTab = tab; }

    // EN: Map Editor accessors and configuration
    // ES: Métodos de acceso y configuración del Editor de Mapas
    const MapEditorConfig& getMapEditorConfig() const { return m_mapEditorConfig; }
    MapEditorConfig& getMapEditorConfig() { return m_mapEditorConfig; }
    const WorldMap& getEditorMap() const { return m_editorMap; }
    WorldMap& getEditorMap() { return m_editorMap; }

    void setupMapEditor();
    void regenerateEditorMap();
    void startPlayWithEditorMap();
    void applyMapEditorBrush(int centerCellX, int centerCellY);

    void applySettings(const GameSettings& settings);
    void loadSettings();
    void saveSettings();

    // EN: Post-defeat spectator mode state and inspection accessors
    // ES: Estado del modo espectador tras la derrota y métodos de acceso
    bool isSpectatingPostDefeat() const { return m_isSpectatingPostDefeat; }
    void setSpectatingPostDefeat(bool enable) { m_isSpectatingPostDefeat = enable; }
    bool isSpectatorRevealAll() const { return m_spectatorRevealAll; }
    void setSpectatorRevealAll(bool reveal) { m_spectatorRevealAll = reveal; }

    // EN: Convert map size selection index into grid dimensions
    // ES: Convertir índice de selección de tamaño de mapa en dimensiones de cuadrícula
    static int getMapDimensionFromIndex(int index);

private:
    int m_windowWidth{1280};
    int m_windowHeight{720};
    bool m_isRunning{true};
    GameState m_gameState{GameState::MainMenu};
    GameState m_previousGameState{GameState::MainMenu};
    int m_hoveredMenuButtonId{0};
    int m_activeInputField{0}; // 501: GodName, 502: ReligionName, 503: MapSeed
    int m_selectedSaveIndex{0};
    std::string m_currentSaveSlot{};

    // EN: Global configuration, options, and tutorial guide state
    // ES: Configuración global, opciones y estado de la guía tutorial
    GameSettings m_settings;
    OptionsTab m_currentOptionsTab{OptionsTab::ScreenAndVideo};
    TutorialTab m_currentTutorialTab{TutorialTab::BasicsAndFaith};

    // EN: Minecraft-inspired main menu panorama state
    // ES: Estado del panorama de la pantalla principal inspirado en Minecraft
    float m_menuPanoramaTime{0.0f};
    bool m_menuPanoramaInitialized{false};
    void setupMenuPanoramaWorld();

    WorldCreationConfig m_creationConfig;

    WorldMap m_map;
    ReligionRegistry m_registry;
    IsometricCamera m_camera;
    AdvancementSystem m_advancement;
    ReplicationEngine m_replication;
    AgglutinationEngine m_agglutination;
    GestorHumanos m_gestorHumanos;
    Timeline m_timeline;
    TimeEngine m_timeEngine;
    EventLine m_eventLine;
    Renderer m_renderer;
    DivinePowersSystem m_divinePowers;
    DivinePowerType m_activeDivinePower{DivinePowerType::Inactive};
    DayNightCycle m_dayNightCycle{90.0f, 9.0f};
    CloudSystem m_cloudSystem{64, 64};
    SoundSystem m_soundSystem;
    NemesisSififusManager m_nemesisSififus;
    ReligiousMoralitySystem m_moralitySystem;
    BuildingSystem m_buildingSystem;

    Point2i m_selectedCell{24, 24};
    RenderMapMode m_mapMode{RenderMapMode::TerrainBiomes};
    bool m_scrubbingTimeline{false};
    float m_mouseX{640.0f};
    float m_mouseY{360.0f};

    // Estado de Extinción de la Fe (Game Over / Derrota al morir todos los fieles)
    bool m_gameOverTriggered{false};
    float m_gameOverTimer{0.0f};
    int m_extinctionYear{-4000};

    // Ventana Modal de Crónica Sagrada y Puntos de Quiebre (Línea de tiempo horizontal)
    bool m_chronicleOpen{false};
    int m_chronicleFilter{0};         // 0: Todos, 1: Puntos de quiebre, 2: Pérdidas, 3: Avances, 4: Descubrimientos, 5: Cismas, 6: Sincretismo
    int m_chronicleScrollIndex{0};    // Desplazamiento horizontal de las tarjetas

    // Consola de comandos/trucos (tecla Enter) y truco "SELECTOR"
    bool m_commandBoxOpen{false};
    std::string m_commandInput{};
    bool m_cheatSelectorActive{false};
    std::string m_commandFeedback{};
    float m_commandFeedbackTimer{0.0f};

    // Niebla de Guerra de la Fe: sólo se ve y controla lo que alcanza la fe del pueblo del jugador.
    // El truco "LUMINUS" la disipa por completo.
    FaithFogOfWar m_fog;
    bool m_cheatLuminusActive{false};
    static constexpr int PLAYER_RELIGION_ID = 1;

    // EN: Dedicated Map Editor / Creation mode state and methods
    // ES: Estado y métodos del modo dedicado de Creación y Edición de Mapas
    MapEditorConfig m_mapEditorConfig;
    WorldMap m_editorMap;
    bool m_isPaintingMapEditor{false};

    // EN: Post-defeat spectator mode state and vision controls
    // ES: Estado del modo espectador tras la derrota y controles de visión
    bool m_isSpectatingPostDefeat{false};
    bool m_spectatorRevealAll{true};

    // EN: Developer mode telemetry and visual inspection toggle state
    // ES: Estado de conmutación de telemetría e inspección visual del modo desarrollador
    bool m_developerMode{false};

    void processButtonAction(int buttonId);
    void executeCommand(const std::string& rawCmd);
    void handleX11Events();
    void handleWindowEvents();
    void initFaithFog(int playerRelId);
    void refreshFaithFog();
    void focusCameraOnPlayerFaith();
    int resolvePlayerReligionId() const;
    bool isCellHiddenByFog(int gx, int gy) const { return m_fog.isEnabled() && !m_fog.isVisible(gx, gy); }
    void showFeedback(const std::string& msg, float seconds = 4.0f);
};

} // namespace ReligionSim
