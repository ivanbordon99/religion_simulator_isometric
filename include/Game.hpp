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
#include <memory>
#include <string>

namespace ReligionSim {

class Game {
public:
    Game(int windowWidth = 1280, int windowHeight = 720);
    ~Game();

    bool init();
    void run();
    void runHeadless(int maxTicks = 200, const std::string& exportPPM = "snapshot.ppm");

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
    const ReligionRegistry& getRegistry() const { return m_registry; }
    ReligionRegistry& getRegistryMutable() { return m_registry; }
    int getPlayerReligionId() const { return resolvePlayerReligionId(); }
    const Timeline& getTimeline() const { return m_timeline; }
    const EventLine& getEventLine() const { return m_eventLine; }
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

    // EN: Error and performance monitoring sentinel
    // ES: Centinela de monitoreo de errores y rendimiento
    const ErrorSystem& getErrorSystem() const { return ErrorSystem::instance(); }
    ErrorSystem& getErrorSystem() { return ErrorSystem::instance(); }
    void performEmergencyMemoryPurge();

    void runCommand(const std::string& cmd) { executeCommand(cmd); }
    void stop() { m_isRunning = false; }

    void startNewCustomWorld();
    void loadStateFromSave(const WorldMap& map, const ReligionRegistry& registry,
                           int currentYear, const std::string& godName,
                           const std::string& religionName, uint32_t seed);
    void saveCurrentGame(const std::string& slotName = "");

private:
    int m_windowWidth{1280};
    int m_windowHeight{720};
    bool m_isRunning{true};
    GameState m_gameState{GameState::MainMenu};
    int m_hoveredMenuButtonId{0};
    int m_activeInputField{0}; // 501: GodName, 502: ReligionName, 503: MapSeed
    int m_selectedSaveIndex{0};

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
