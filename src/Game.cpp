#include "Game.hpp"
#include "SaveSystem.hpp"
#include <iostream>
#include <chrono>
#include <thread>
#include <map>
#include <cctype>
#include <ctime>

#if defined(_WIN32)
#define HAS_WIN32 1
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <windowsx.h>
#elif __has_include(<X11/Xlib.h>) && __has_include(<X11/keysym.h>)
#define HAS_X11 1
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#endif

#ifdef HAS_WIN32
static ReligionSim::Game* g_win32GameInstance = nullptr;

LRESULT CALLBACK MainWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (!g_win32GameInstance) return DefWindowProcA(hwnd, uMsg, wParam, lParam);

    switch (uMsg) {
        case WM_DESTROY:
        case WM_CLOSE:
            g_win32GameInstance->stop();
            PostQuitMessage(0);
            return 0;

        case WM_SIZE: {
            int nw = LOWORD(lParam);
            int nh = HIWORD(lParam);
            if (nw > 0 && nh > 0) {
                g_win32GameInstance->onWindowResize(nw, nh);
            }
            return 0;
        }

        case WM_LBUTTONDOWN: {
            float mx = static_cast<float>(GET_X_LPARAM(lParam));
            float my = static_cast<float>(GET_Y_LPARAM(lParam));
            SetCapture(hwnd);
            g_win32GameInstance->onMouseDown(1, mx, my);
            return 0;
        }
        case WM_LBUTTONUP: {
            float mx = static_cast<float>(GET_X_LPARAM(lParam));
            float my = static_cast<float>(GET_Y_LPARAM(lParam));
            ReleaseCapture();
            g_win32GameInstance->onMouseUp(1, mx, my);
            return 0;
        }
        case WM_RBUTTONDOWN: {
            float mx = static_cast<float>(GET_X_LPARAM(lParam));
            float my = static_cast<float>(GET_Y_LPARAM(lParam));
            SetCapture(hwnd);
            g_win32GameInstance->onMouseDown(3, mx, my);
            return 0;
        }
        case WM_RBUTTONUP: {
            float mx = static_cast<float>(GET_X_LPARAM(lParam));
            float my = static_cast<float>(GET_Y_LPARAM(lParam));
            ReleaseCapture();
            g_win32GameInstance->onMouseUp(3, mx, my);
            return 0;
        }
        case WM_MOUSEMOVE: {
            float mx = static_cast<float>(GET_X_LPARAM(lParam));
            float my = static_cast<float>(GET_Y_LPARAM(lParam));
            g_win32GameInstance->onMouseMove(mx, my);
            return 0;
        }
        case WM_MOUSEWHEEL: {
            short delta = GET_WHEEL_DELTA_WPARAM(wParam);
            POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            ScreenToClient(hwnd, &pt);
            g_win32GameInstance->onMouseWheel(delta > 0 ? 1.0f : -1.0f, static_cast<float>(pt.x), static_cast<float>(pt.y));
            return 0;
        }
        case WM_CHAR: {
            if (wParam >= 32 && wParam <= 126) {
                g_win32GameInstance->onKeyPress(static_cast<uint8_t>(wParam));
                return 0;
            }
            break;
        }
        case WM_KEYDOWN: {
            if (wParam == VK_ESCAPE) {
                g_win32GameInstance->onKeyPress(27);
            } else if (wParam == VK_RETURN) {
                g_win32GameInstance->onKeyPress(13);
            } else if (wParam == VK_BACK) {
                g_win32GameInstance->onKeyPress(8);
            } else if (wParam == VK_SPACE) {
                g_win32GameInstance->onKeyPress(32);
            } else if (wParam == 'W') {
                g_win32GameInstance->onKeyPress('w');
            } else if (wParam == 'A') {
                g_win32GameInstance->onKeyPress('a');
            } else if (wParam == 'S') {
                g_win32GameInstance->onKeyPress('s');
            } else if (wParam == 'D') {
                g_win32GameInstance->onKeyPress('d');
            } else if (wParam == 'R') {
                g_win32GameInstance->onKeyPress('r');
            } else if (wParam >= '0' && wParam <= '9') {
                g_win32GameInstance->onKeyPress(static_cast<uint8_t>(wParam));
            } else if (wParam == VK_OEM_PLUS || wParam == VK_ADD) {
                g_win32GameInstance->onKeyPress('+');
            } else if (wParam == VK_OEM_MINUS || wParam == VK_SUBTRACT) {
                g_win32GameInstance->onKeyPress('-');
            }
            return 0;
        }
        default:
            break;
    }
    return DefWindowProcA(hwnd, uMsg, wParam, lParam);
}
#endif

namespace ReligionSim {

Game::Game(int windowWidth, int windowHeight)
    : m_windowWidth(windowWidth), m_windowHeight(windowHeight),
      m_map(48, 48),
      m_camera(windowWidth, windowHeight),
      m_timeline(-4000, 2200),
      m_renderer(windowWidth, windowHeight) {
    m_fog.reset(m_map.getWidth(), m_map.getHeight());
    m_fog.setPlayerReligionId(PLAYER_RELIGION_ID);
    m_renderer.setFogOfWar(&m_fog);
}

Game::~Game() {}

bool Game::init() {
#ifdef HAS_WIN32
    g_win32GameInstance = this;
#endif
    // Generar una semilla aleatoria por defecto para el primer arranque en la ventana de creación
    m_creationConfig.mapSeed = static_cast<uint32_t>(std::time(nullptr)) ^ 0xA5A5A5A5;

    // Try initializing window (X11 / Win32 / OpenGL)
    bool windowOk = m_renderer.initWindow("Marangatu: The Faith Simulator");
    if (windowOk) {
#if HAS_X11
        std::cout << "[Simulator] X11/OpenGL Display initialized successfully." << std::endl;
#elif defined(HAS_WIN32)
        std::cout << "[Simulator] Win32/OpenGL Display initialized successfully." << std::endl;
#else
        std::cout << "[Simulator] Window initialized successfully." << std::endl;
#endif
    } else {
        std::cout << "[Simulator] Running in headless mode (no display detected)." << std::endl;
    }

    return true;
}

void Game::update(float deltaTime) {
    if (m_gameState != GameState::Playing) {
        return; // No avanzar la simulación en menús
    }

    m_camera.update(deltaTime);

    // La niebla de la fe se recalcula a partir del pueblo del jugador cada fotograma
    m_fog.update(m_map, m_gestorHumanos);

    if (m_commandFeedbackTimer > 0.0f) {
        m_commandFeedbackTimer = std::max(0.0f, m_commandFeedbackTimer - deltaTime);
        if (m_commandFeedbackTimer <= 0.0f) {
            m_commandFeedback.clear();
        }
    }

    // 1. Sincronizar simulación y paso del tiempo con el movimiento de los sprites
    float dtYears = 0.0f;
    m_timeline.update(deltaTime, dtYears);
    m_timeEngine.setYear(static_cast<double>(m_timeline.getCurrentYear()));

    float speedMult = m_timeline.isPaused() ? 0.0f : static_cast<float>(m_timeline.getSpeedMultiplier());
    float spriteDt = deltaTime * speedMult;

    // Actualizar objetos Humano con IA y animación sobre el tileset (sincroniza población y fe 1:1 en celdas)
    if (spriteDt <= 0.0f && m_cheatSelectorActive) {
        bool anyManualMove = false;
        for (auto& hObj : m_gestorHumanos.getHumanos()) {
            if (hObj.tieneOrdenManual()) {
                hObj.actualizar(deltaTime, m_map);
                anyManualMove = true;
            }
        }
        if (anyManualMove) {
            m_gestorHumanos.actualizar(0.0f, m_map);
            m_map.recalculateReligions(m_registry);
        }
    } else {
        m_gestorHumanos.actualizar(spriteDt, m_map);
    }

    if (spriteDt > 0.0f && dtYears <= 0.0f) {
        m_map.recalculateReligions(m_registry);
    }

    if (dtYears > 0.0f) {
        std::vector<GameEvent> newEvents;

        // 1. Advance eras & theological doctrines
        m_advancement.updateAdvancements(m_registry, m_timeline.getCurrentYear(), newEvents);

        // 2. Replication (spatial diffusion, demographic growth, missionary caravans)
        m_replication.update(m_map, m_registry, dtYears, newEvents, m_timeline.getCurrentYear());

        // 3. Agglutination (syncretic fusion & doctrinal schisms)
        m_agglutination.update(m_map, m_registry, dtYears, newEvents, m_timeline.getCurrentYear());

        // Sincronizar los aldeanos (nodos de fe) si nació una nueva religión por cisma o sincretismo
        for (const auto& ev : newEvents) {
            if (ev.category == EventCategory::SyncreticAgglutination ||
                ev.category == EventCategory::DoctrinalSchism) {
                int newRelId = m_map.isValid(ev.location.x, ev.location.y)
                               ? m_map.getCell(ev.location.x, ev.location.y).dominantReligionId : 0;
                if (newRelId > 0) {
                    for (auto& hObj : m_gestorHumanos.getHumanos()) {
                        if (std::abs(hObj.getCeldaX() - ev.location.x) <= 1 &&
                            std::abs(hObj.getCeldaY() - ev.location.y) <= 1) {
                            hObj.setReligionId(newRelId);
                        }
                    }
                }
            }
        }
        m_gestorHumanos.actualizar(0.0f, m_map);

        // 4. Update stats
        m_map.recalculateReligions(m_registry);

        // 5. Push events to chronicle
        for (const auto& ev : newEvents) {
            m_eventLine.addEvent(ev);
            std::cout << "[Event " << Timeline::formatYear(ev.year) << "] "
                      << ev.title << " - " << ev.description << std::endl;
        }

        // 6. Record history snapshots for timeline scrubbing
        int w = m_map.getWidth();
        int h = m_map.getHeight();
        std::vector<uint8_t> domMap(w * h, 0);
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                domMap[y * w + x] = static_cast<uint8_t>(m_map.getCell(x, y).dominantReligionId);
            }
        }

        std::vector<std::pair<int, int>> followers;
        for (const auto& r : m_registry.getAllReligions()) {
            followers.push_back({r.id, r.totalFollowers});
        }
        m_timeline.recordSnapshot(m_map.getTotalWorldPopulation(), domMap, followers);
    }

    refreshFaithFog();
}

void Game::render() {
    m_renderer.beginFrame();

    if (m_gameState == GameState::MainMenu) {
        m_renderer.renderMainMenu(m_hoveredMenuButtonId);
    } else if (m_gameState == GameState::WorldCreation) {
        m_renderer.renderWorldCreationMenu(m_creationConfig, m_hoveredMenuButtonId, m_activeInputField);
    } else if (m_gameState == GameState::LoadWorldMenu) {
        m_renderer.renderLoadWorldMenu(m_hoveredMenuButtonId, m_selectedSaveIndex);
    } else if (m_gameState == GameState::Options) {
        m_renderer.renderOptionsMenu(m_hoveredMenuButtonId);
    } else {
        m_renderer.setFogOfWar(&m_fog);
        m_renderer.render(m_map, m_registry, m_timeline, m_eventLine,
                          m_replication, m_gestorHumanos, m_camera, m_selectedCell, m_mapMode,
                          m_creationConfig);
        m_renderer.renderCommandConsole(m_commandBoxOpen, m_commandInput,
                                        m_cheatSelectorActive, m_commandFeedback,
                                        m_cheatLuminusActive);
    }

    m_renderer.drawCursor(static_cast<int>(m_mouseX), static_cast<int>(m_mouseY));
    m_renderer.endFrame();
}

void Game::onWindowResize(int newWidth, int newHeight) {
    if (newWidth <= 100 || newHeight <= 100) return;
    if (newWidth == m_windowWidth && newHeight == m_windowHeight) return;

    m_windowWidth = newWidth;
    m_windowHeight = newHeight;

    m_renderer.resize(newWidth, newHeight);
    m_camera.setScreenSize(newWidth, newHeight);
}

void Game::selectCell(int gx, int gy) {
    if (m_map.isValid(gx, gy)) {
        m_selectedCell = {gx, gy};
    }
}

void Game::triggerSyncretismAtSelection() {
    std::vector<GameEvent> evs;
    int id = m_agglutination.triggerForcedSyncretism(
        m_map, m_registry, m_selectedCell.x, m_selectedCell.y, evs, m_timeline.getCurrentYear()
    );
    if (id > 0) {
        for (auto& hObj : m_gestorHumanos.getHumanos()) {
            if (hObj.getCeldaX() == m_selectedCell.x && hObj.getCeldaY() == m_selectedCell.y) {
                hObj.setReligionId(id);
            }
        }
        m_gestorHumanos.actualizar(0.0f, m_map);
    }
    for (const auto& ev : evs) {
        m_eventLine.addEvent(ev);
    }
    m_map.recalculateReligions(m_registry);
}

void Game::triggerSchismAtSelection() {
    std::vector<GameEvent> evs;
    int id = m_agglutination.triggerForcedSchism(
        m_map, m_registry, m_selectedCell.x, m_selectedCell.y, evs, m_timeline.getCurrentYear()
    );
    if (id > 0) {
        for (auto& hObj : m_gestorHumanos.getHumanos()) {
            if (hObj.getCeldaX() == m_selectedCell.x && hObj.getCeldaY() == m_selectedCell.y) {
                hObj.setReligionId(id);
            }
        }
        m_gestorHumanos.actualizar(0.0f, m_map);
    }
    for (const auto& ev : evs) {
        m_eventLine.addEvent(ev);
    }
    m_map.recalculateReligions(m_registry);
}

void Game::buildHolySiteAtSelection() {
    if (m_map.isValid(m_selectedCell.x, m_selectedCell.y)) {
        Cell& c = m_map.getCell(m_selectedCell.x, m_selectedCell.y);
        if (!c.hasHolySite) {
            m_map.consecrateHolySite(m_selectedCell.x, m_selectedCell.y, "Grand Sanctuary of the Chosen");
        } else {
            m_map.upgradeHolySite(m_selectedCell.x, m_selectedCell.y);
        }

        m_eventLine.addEvent(
            m_timeline.getCurrentYear(), EventCategory::HolySiteErected,
            "Monument Consecrated",
            "A monumental sacred sanctuary was consecrated at grid (" +
            std::to_string(m_selectedCell.x) + ", " + std::to_string(m_selectedCell.y) + ").",
            m_selectedCell, c.dominantReligionId, 0, ColorRGB(255, 215, 0)
        );
        m_map.recalculateReligions(m_registry);
    }
}

void Game::processButtonAction(int buttonId) {
    // El desplegable de opciones sólo se mantiene abierto al pulsar su propio botón MENU
    if (buttonId != 400) {
        m_renderer.setOptionsDropdownOpen(false);
    }

    switch (buttonId) {
        case 400: // MENU -> abrir/cerrar el desplegable de opciones de partida
            m_renderer.setOptionsDropdownOpen(!m_renderer.isOptionsDropdownOpen());
            break;
        case 405: // CONSOLA DE TRUCOS (equivale a pulsar Enter)
            m_commandBoxOpen = true;
            m_commandInput.clear();
            break;

        case 101: m_mapMode = RenderMapMode::ReligionDominance; break;
        case 102: m_mapMode = RenderMapMode::TerrainBiomes; break;
        case 103: m_mapMode = RenderMapMode::SyncretismHeatmap; break;
        case 104: m_mapMode = RenderMapMode::AdvancementEra; break;
        case 105: m_mapMode = RenderMapMode::Units; break;

        case 201:
            m_timeEngine.togglePause();
            m_timeline.togglePause();
            break;
        case 202:
            m_timeEngine.setMultiplier(TimeMultiplier::Normal);
            m_timeline.setPaused(false);
            m_timeline.setSpeedMultiplier(1);
            break;
        case 203:
            m_timeEngine.setMultiplier(TimeMultiplier::Fast);
            m_timeline.setPaused(false);
            m_timeline.setSpeedMultiplier(2);
            break;
        case 204:
            m_timeEngine.setMultiplier(TimeMultiplier::Ultra);
            m_timeline.setPaused(false);
            m_timeline.setSpeedMultiplier(5);
            break;

        case 301: triggerSyncretismAtSelection(); break;
        case 302: triggerSchismAtSelection(); break;
        case 303: buildHolySiteAtSelection(); break;
        case 304: m_camera.reset(m_map.getWidth(), m_map.getHeight()); break;
        case 401:
            saveCurrentGame();
            showFeedback("PARTIDA GUARDADA EXITOSAMENTE", 3.5f);
            break;
        case 402: // Menu
            saveCurrentGame();
            m_gameState = GameState::MainMenu;
            break;

        // Opciones de la Unidad / Señales Divinas del Chamán (Zona Verde)
        case 701:
        case 702:
        case 703: {
            const Humano* sel = m_gestorHumanos.getHumanoSeleccionado();
            if (!sel && !m_gestorHumanos.getHumanos().empty()) {
                m_gestorHumanos.seleccionarHumano(m_gestorHumanos.getHumanos()[0].getId());
                sel = m_gestorHumanos.getHumanoSeleccionado();
            }
            if (sel && sel->esChaman()) {
                int tipoSenal = (buttonId == 701) ? 1 : ((buttonId == 702) ? 2 : 3);
                m_gestorHumanos.ejecutarSenalDivina(tipoSenal, m_map, m_registry, m_eventLine, m_timeline.getCurrentYear());
            } else if (buttonId == 703) {
                int targetRel = sel ? sel->getReligionId() : 1;
                int chamanId = m_gestorHumanos.buscarChamanDeReligion(targetRel);
                if (chamanId <= 0) chamanId = m_gestorHumanos.buscarChamanDeReligion(1);
                if (chamanId > 0) {
                    m_gestorHumanos.seleccionarHumano(chamanId);
                    const Humano* ch = m_gestorHumanos.getHumanoSeleccionado();
                    if (ch && m_map.isValid(ch->getCeldaX(), ch->getCeldaY())) {
                        m_camera.centerOnCell(ch->getCeldaX(), ch->getCeldaY(), m_map.getWidth(), m_map.getHeight());
                        selectCell(ch->getCeldaX(), ch->getCeldaY());
                    }
                }
            }
            break;
        }
        default: break;
    }
}

void Game::onMouseDown(int button, float x, float y) {
    m_mouseX = x;
    m_mouseY = y;
    if (m_gameState == GameState::MainMenu) {
        if (button == 1) {
            int btnId = m_renderer.checkMenuButtonClicked(x, y, false);
            if (btnId == 1) { // CREAR MUNDO -> Abrir ventana de configuración
                m_gameState = GameState::WorldCreation;
                m_activeInputField = 0;
            } else if (btnId == 2) { // CARGAR MUNDO -> Abrir selector estilo Minecraft
                m_gameState = GameState::LoadWorldMenu;
                m_selectedSaveIndex = 0;
            } else if (btnId == 3) { // OPCIONES
                m_gameState = GameState::Options;
            } else if (btnId == 4) { // SALIR
                m_isRunning = false;
            }
        }
        return;
    }

    if (m_gameState == GameState::WorldCreation) {
        if (button == 1) {
            int btnId = m_renderer.checkCreationButtonClicked(x, y);
            if (btnId == 550) { // Cancelar y volver
                m_gameState = GameState::MainMenu;
                m_activeInputField = 0;
            } else if (btnId == 551) { // Comenzar Génesis
                startNewCustomWorld();
            } else if (btnId == 501) { // Input Dios
                m_activeInputField = 501;
            } else if (btnId == 502) { // Input Religion
                m_activeInputField = 502;
            } else if (btnId == 503) { // Input Seed
                m_activeInputField = 503;
            } else if (btnId == 504) { // Randomize Seed
                m_creationConfig.mapSeed = static_cast<uint32_t>(std::time(nullptr)) ^ 0x5F3759DF;
            } else if (btnId == 505) { // Ciclar población de aldeanos reales
                if (m_creationConfig.initialPopulation == 40) m_creationConfig.initialPopulation = 80;
                else if (m_creationConfig.initialPopulation == 80) m_creationConfig.initialPopulation = 120;
                else if (m_creationConfig.initialPopulation == 120) m_creationConfig.initialPopulation = 180;
                else if (m_creationConfig.initialPopulation == 180) m_creationConfig.initialPopulation = 250;
                else m_creationConfig.initialPopulation = 40;
            } else if (btnId == 506) { // Ciclar época de inicio
                uint8_t nextEra = (static_cast<uint8_t>(m_creationConfig.startingEra) + 1) % 8;
                m_creationConfig.startingEra = static_cast<HistoricalEra>(nextEra);
            } else if (btnId == 507) { // Ciclar tamaño del mapa
                m_creationConfig.mapSizeIndex = (m_creationConfig.mapSizeIndex + 1) % 3;
            } else if (btnId == 508) { // Ciclar cataclismos
                m_creationConfig.disasterFrequency = (m_creationConfig.disasterFrequency + 1) % 3;
            } else if (btnId == 509) { // Ciclar tipo de partida
                uint8_t nextMode = (static_cast<uint8_t>(m_creationConfig.gameMode) + 1) % 4;
                m_creationConfig.gameMode = static_cast<GameModeType>(nextMode);
            } else if (btnId == 510) { // Cofre inicial toggle
                m_creationConfig.starterChest = !m_creationConfig.starterChest;
            } else if (btnId == 511) { // Ciclar dificultad IA
                uint8_t nextDiff = (static_cast<uint8_t>(m_creationConfig.aiDifficulty) + 1) % 4;
                m_creationConfig.aiDifficulty = static_cast<AIDifficulty>(nextDiff);
            } else if (btnId == 512) { // Ciclar color de influencia
                m_creationConfig.influenceColorIndex = (m_creationConfig.influenceColorIndex + 1) % 5;
            } else if (btnId == 513) { // Ciclar condición de victoria
                uint8_t nextVic = (static_cast<uint8_t>(m_creationConfig.victoryCondition) + 1) % 5;
                m_creationConfig.victoryCondition = static_cast<VictoryConditionType>(nextVic);
            } else if (btnId == 514) { // Cismas toggle
                m_creationConfig.allowSchisms = !m_creationConfig.allowSchisms;
            } else if (btnId == 515) { // Sincretismo toggle
                m_creationConfig.allowSyncretism = !m_creationConfig.allowSyncretism;
            }
        }
        return;
    }

    if (m_gameState == GameState::LoadWorldMenu) {
        if (button == 1) {
            auto saves = SaveSystem::scanSaveSlots();
            int btnId = m_renderer.checkLoadMenuButtonClicked(x, y, static_cast<int>(saves.size()));
            if (btnId >= 1000) {
                // Seleccionar slot de mundo
                m_selectedSaveIndex = btnId - 1000;
            } else if (btnId == 601) { // JUGAR MUNDO
                if (!saves.empty() && m_selectedSaveIndex >= 0 && m_selectedSaveIndex < static_cast<int>(saves.size())) {
                    SaveSystem::loadWorld(*this, saves[m_selectedSaveIndex].folderName);
                }
            } else if (btnId == 602) { // CREAR NUEVO
                m_gameState = GameState::WorldCreation;
                m_activeInputField = 0;
            } else if (btnId == 603) { // ELIMINAR
                if (!saves.empty() && m_selectedSaveIndex >= 0 && m_selectedSaveIndex < static_cast<int>(saves.size())) {
                    SaveSystem::deleteWorld(saves[m_selectedSaveIndex].folderName);
                    m_selectedSaveIndex = 0;
                }
            } else if (btnId == 604) { // CANCELAR
                m_gameState = GameState::MainMenu;
            }
        }
        return;
    }

    if (m_gameState == GameState::Options) {
        if (button == 1) {
            int btnId = m_renderer.checkMenuButtonClicked(x, y, true);
            if (btnId == 12) { // VOLVER AL MENU
                m_gameState = GameState::MainMenu;
            }
        }
        return;
    }

    if (button == 1) { // Left Click
        // 1. Check UI button clicks
        int btnId = m_renderer.checkButtonClicked(x, y);
        if (btnId > 0) {
            processButtonAction(btnId);
            return;
        }
        // Un clic fuera del desplegable de opciones lo cierra (sin actuar sobre el mundo)
        if (m_renderer.isOptionsDropdownOpen()) {
            m_renderer.setOptionsDropdownOpen(false);
            return;
        }

        const int botH = 118;
        const int botY = m_windowHeight - botH;

        // 2. Minimapa Clic (panel inferior derecho)
        // Mover la cámara directamente al hacer clic en el minimapa
        const int amarilloX = static_cast<int>(m_windowWidth * 0.815f);
        if (x >= amarilloX && y >= botY) {
            int mmSize = 92, mmX = 0, mmY = 0;
            m_renderer.getMinimapRect(mmX, mmY, mmSize);
            if (x >= mmX && x <= mmX + mmSize && y >= mmY && y <= mmY + mmSize) {
                float normX = (x - mmX) / static_cast<float>(mmSize);
                float normY = (y - mmY) / static_cast<float>(mmSize);
                int targetGx = static_cast<int>(normX * m_map.getWidth());
                int targetGy = static_cast<int>(normY * m_map.getHeight());
                m_camera.centerOnCell(targetGx, targetGy, m_map.getWidth(), m_map.getHeight());
                selectCell(targetGx, targetGy);
                return;
            }
        }

        // 3. Timeline Scrubber (Zona Azul central)
        const int verdeW = static_cast<int>(m_windowWidth * 0.185f);
        const int azulX = verdeW;
        const int azulW = static_cast<int>(m_windowWidth * 0.63f);
        int trackX = azulX + 16;
        int trackY = botY + 14;
        int trackW = azulW - 32;

        if (y >= trackY - 8 && y <= trackY + 16 && x >= trackX && x <= trackX + trackW) {
            float prog = (x - trackX) / static_cast<float>(trackW);
            m_timeline.scrubToProgress(prog);
            m_scrubbingTimeline = true;
            return;
        }

        // 4. Check Event Ribbon cards click (dentro de la zona azul)
        int cardY = botY + 28;
        int cardW = (azulW - 48) / 3;
        int cardH = 46;
        if (y >= cardY && y <= cardY + cardH && x >= trackX && x <= trackX + trackW) {
            auto recents = m_eventLine.getRecentEvents(3);
            for (size_t i = 0; i < recents.size(); ++i) {
                int cx = trackX + static_cast<int>(i) * (cardW + 8);
                if (x >= cx && x <= cx + cardW) {
                    m_camera.centerOnCell(recents[i].location.x, recents[i].location.y,
                                          m_map.getWidth(), m_map.getHeight());
                    selectCell(recents[i].location.x, recents[i].location.y);
                    return;
                }
            }
        }

        // 5. Clic en el Mundo Isométrico (Selección de Aldeano o Celda)
        if (y < botY && y > 34) {
            int gx = 0, gy = 0;
            m_camera.screenToGrid(x, y, gx, gy);

            // Primero buscar si se hizo clic directamente sobre el sprite de un aldeano en pantalla.
            // Fog of War: sólo se pueden seleccionar unidades del pueblo del jugador
            // (salvo con el truco SELECTOR, que permite seleccionar cualquier unidad).
            const int onlyRel = m_cheatSelectorActive ? -1 : PLAYER_RELIGION_ID;
            int humanoId = m_gestorHumanos.buscarHumanoEnPantalla(x, y, m_camera, m_map, onlyRel);
            if (humanoId <= 0) {
                humanoId = m_gestorHumanos.buscarHumanoCercano(static_cast<float>(gx) + 0.5f,
                                                               static_cast<float>(gy) + 0.5f, 0.85f, onlyRel);
            }
            if (humanoId > 0 && !m_cheatSelectorActive) {
                const Humano* cand = nullptr;
                for (const auto& hh : m_gestorHumanos.getHumanos()) {
                    if (hh.getId() == humanoId) { cand = &hh; break; }
                }
                if (!cand || cand->getReligionId() != PLAYER_RELIGION_ID) humanoId = -1;
            }

            if (humanoId > 0) {
                m_gestorHumanos.seleccionarHumano(humanoId);
                const Humano* sel = m_gestorHumanos.getHumanoSeleccionado();
                if (sel && m_map.isValid(sel->getCeldaX(), sel->getCeldaY())) {
                    selectCell(sel->getCeldaX(), sel->getCeldaY());
                }
            } else {
                m_gestorHumanos.seleccionarHumano(-1);
                if (m_map.isValid(gx, gy)) {
                    // Las celdas ocultas por la niebla no revelan información
                    selectCell(gx, gy);
                }
            }
        }
    } else if (button == 3) { // Right Click: mover unidad seleccionada si el truco SELECTOR está activo, o arrastrar cámara
        const int botH = 118;
        const int botY = m_windowHeight - botH;
        Humano* sel = m_gestorHumanos.getHumanoSeleccionadoMutable();
        if (m_cheatSelectorActive && sel && y > 34 && y < botY) {
            float halfW = BASE_TILE_WIDTH * 0.5f;
            float halfH = BASE_TILE_HEIGHT * 0.5f;
            float isoX = (x - m_camera.getCameraX()) / m_camera.getZoom();
            float isoY = (y - m_camera.getCameraY()) / m_camera.getZoom() + 0.50f * ELEVATION_STEP;
            float u = isoX / halfW;
            float v = isoY / halfH;
            float gx = (u + v) * 0.5f;
            float gy = (v - u) * 0.5f;

            int cellX = std::clamp(static_cast<int>(std::floor(gx)), 0, m_map.getWidth() - 1);
            int cellY = std::clamp(static_cast<int>(std::floor(gy)), 0, m_map.getHeight() - 1);
            float realElev = m_map.getCell(cellX, cellY).elevation;

            isoY = (y - m_camera.getCameraY()) / m_camera.getZoom() + realElev * ELEVATION_STEP;
            v = isoY / halfH;
            gx = std::clamp((u + v) * 0.5f, 0.5f, static_cast<float>(m_map.getWidth()) - 0.5f);
            gy = std::clamp((v - u) * 0.5f, 0.5f, static_cast<float>(m_map.getHeight()) - 0.5f);

            sel->ordenarMovimientoManual(gx, gy);
            selectCell(static_cast<int>(std::floor(gx)), static_cast<int>(std::floor(gy)));
        } else {
            m_camera.startDrag(x, y);
        }
    }
}

void Game::onMouseUp(int button, float x, float y) {
    (void)x;
    (void)y;
    if (button == 1) {
        m_scrubbingTimeline = false;
    } else if (button == 3) {
        m_camera.endDrag();
    }
}

void Game::onMouseMove(float x, float y) {
    m_mouseX = x;
    m_mouseY = y;
    if (m_gameState == GameState::MainMenu) {
        m_hoveredMenuButtonId = m_renderer.checkMenuButtonClicked(x, y, false);
        return;
    }
    if (m_gameState == GameState::WorldCreation) {
        m_hoveredMenuButtonId = m_renderer.checkCreationButtonClicked(x, y);
        return;
    }
    if (m_gameState == GameState::LoadWorldMenu) {
        auto saves = SaveSystem::scanSaveSlots();
        m_hoveredMenuButtonId = m_renderer.checkLoadMenuButtonClicked(x, y, static_cast<int>(saves.size()));
        return;
    }

    if (m_gameState == GameState::Options) {
        m_hoveredMenuButtonId = m_renderer.checkMenuButtonClicked(x, y, true);
        return;
    }

    if (m_camera.isDragging()) {
        m_camera.updateDrag(x, y);
    } else if (m_scrubbingTimeline) {
        int trackX = 40;
        int trackW = m_windowWidth - 80;
        float prog = (x - trackX) / static_cast<float>(trackW);
        m_timeline.scrubToProgress(prog);
    }
}

void Game::onMouseWheel(float delta, float x, float y) {
    float factor = (delta > 0.0f) ? 1.15f : 0.85f;
    m_camera.zoomBy(factor, x, y);
}

void Game::onKeyPress(int keySym) {
    if (m_gameState == GameState::WorldCreation) {
        if (keySym == 27) { // ESC -> Salir al menú
            m_gameState = GameState::MainMenu;
            m_activeInputField = 0;
            return;
        }
        if (keySym == 13 || keySym == 10 || keySym == 32) { // Enter
            if (keySym != 32) {
                startNewCustomWorld();
                return;
            }
        }
        if (m_activeInputField == 501) { // GodName
            if (keySym == 8) { // Backspace
                if (!m_creationConfig.godName.empty()) m_creationConfig.godName.pop_back();
            } else if (keySym >= 32 && keySym <= 126 && m_creationConfig.godName.size() < 24) {
                m_creationConfig.godName.push_back(static_cast<char>(keySym));
            }
            return;
        }
        if (m_activeInputField == 502) { // ReligionName
            if (keySym == 8) { // Backspace
                if (!m_creationConfig.religionName.empty()) m_creationConfig.religionName.pop_back();
            } else if (keySym >= 32 && keySym <= 126 && m_creationConfig.religionName.size() < 28) {
                m_creationConfig.religionName.push_back(static_cast<char>(keySym));
            }
            return;
        }
        if (m_activeInputField == 503) { // Seed
            if (keySym == 8) {
                m_creationConfig.mapSeed /= 10;
            } else if (keySym >= '0' && keySym <= '9') {
                m_creationConfig.mapSeed = m_creationConfig.mapSeed * 10 + (keySym - '0');
            }
            return;
        }
        return;
    }

    if (m_gameState == GameState::LoadWorldMenu) {
        if (keySym == 27) { // ESC -> Volver al menú
            m_gameState = GameState::MainMenu;
            return;
        }
        if (keySym == 13 || keySym == 10) { // Enter -> Jugar mundo
            auto saves = SaveSystem::scanSaveSlots();
            if (!saves.empty() && m_selectedSaveIndex >= 0 && m_selectedSaveIndex < static_cast<int>(saves.size())) {
                SaveSystem::loadWorld(*this, saves[m_selectedSaveIndex].folderName);
            }
            return;
        }
        return;
    }

    if (m_gameState == GameState::Options) {
        if (keySym == 27) { // ESC -> Volver al menú principal
            m_gameState = GameState::MainMenu;
            return;
        }
    } else if (m_gameState == GameState::MainMenu) {
        if (keySym == 27) { // ESC -> Salir
            m_isRunning = false;
            return;
        }
    }

    // Si la caja de comandos / trucos está abierta en partida
    if (m_gameState == GameState::Playing && m_commandBoxOpen) {
        if (keySym == 27) { // ESC -> Cerrar caja de comandos
            m_commandBoxOpen = false;
            m_commandInput.clear();
            return;
        }
        if (keySym == 13 || keySym == 10) { // ENTER -> Ejecutar comando/truco y cerrar caja
            executeCommand(m_commandInput);
            m_commandBoxOpen = false;
            m_commandInput.clear();
            return;
        }
        if (keySym == 8) { // Backspace
            if (!m_commandInput.empty()) {
                m_commandInput.pop_back();
            }
            return;
        }
        if (keySym >= 32 && keySym <= 126 && m_commandInput.size() < 48) {
            m_commandInput.push_back(static_cast<char>(keySym));
        }
        return;
    }

    // Si presionan ENTER durante la partida, abrir la caja de texto de comandos/trucos
    if (m_gameState == GameState::Playing && (keySym == 13 || keySym == 10)) {
        m_commandBoxOpen = true;
        m_commandInput.clear();
        return;
    }

    switch (keySym) {
        case 32: // Space
            m_timeEngine.togglePause();
            m_timeline.togglePause();
            break;
        case '1': m_mapMode = RenderMapMode::ReligionDominance; break;
        case '2': m_mapMode = RenderMapMode::TerrainBiomes; break;
        case '3': m_mapMode = RenderMapMode::SyncretismHeatmap; break;
        case '4': m_mapMode = RenderMapMode::AdvancementEra; break;
        case '5': m_mapMode = RenderMapMode::Units; break;
        case '+': case '=': m_camera.zoomBy(1.2f, m_windowWidth / 2.0f, m_windowHeight / 2.0f); break;
        case '-': case '_': m_camera.zoomBy(0.83f, m_windowWidth / 2.0f, m_windowHeight / 2.0f); break;
        case 'w': case 'W': m_camera.pan(0, 40); break;
        case 's': case 'S': m_camera.pan(0, -40); break;
        case 'a': case 'A': m_camera.pan(40, 0); break;
        case 'd': case 'D': m_camera.pan(-40, 0); break;
        case 'r': case 'R': m_camera.reset(m_map.getWidth(), m_map.getHeight()); break;
        case 27: // ESC -> Guardar automáticamente estilo Minecraft y salir al Menú Principal
            saveCurrentGame();
            m_gameState = GameState::MainMenu;
            break;
        default: break;
    }
}

void Game::executeCommand(const std::string& rawCmd) {
    std::string cmd;
    for (char ch : rawCmd) {
        if (!std::isspace(static_cast<unsigned char>(ch))) {
            cmd.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(ch))));
        }
    }
    if (cmd.empty()) return;

    if (cmd == "SELECTOR") {
        m_cheatSelectorActive = !m_cheatSelectorActive;
        if (m_cheatSelectorActive) {
            m_commandFeedback = "TRUCO 'SELECTOR' ACTIVADO: SELECCIONA CUALQUIER UNIDAD Y MUEVELA CON CLIC DERECHO";
            if (!m_gestorHumanos.getHumanoSeleccionado() && !m_gestorHumanos.getHumanos().empty()) {
                m_gestorHumanos.seleccionarHumano(m_gestorHumanos.getHumanos()[0].getId());
            }
        } else {
            m_commandFeedback = "TRUCO 'SELECTOR' DESACTIVADO";
        }
        showFeedback(m_commandFeedback, 5.0f);
    } else if (cmd == "LUMINUS") {
        m_cheatLuminusActive = !m_cheatLuminusActive;
        m_fog.setEnabled(!m_cheatLuminusActive);
        if (!m_cheatLuminusActive) {
            refreshFaithFog();
        }
        showFeedback(m_cheatLuminusActive
                         ? "TRUCO 'LUMINUS' ACTIVADO: NIEBLA DE LA FE DISIPADA"
                         : "TRUCO 'LUMINUS' DESACTIVADO: LA NIEBLA DE LA FE REGRESA",
                     5.0f);
    } else {
        showFeedback("COMANDO DESCONOCIDO: '" + cmd + "' (USA: SELECTOR | LUMINUS)", 4.0f);
    }
}

void Game::showFeedback(const std::string& msg, float seconds) {
    m_commandFeedback = msg;
    m_commandFeedbackTimer = seconds;
    std::cout << "[Comando] " << msg << std::endl;
}

int Game::resolvePlayerReligionId() const {
    for (const auto& r : m_registry.getAllReligions()) {
        if (r.name == m_creationConfig.religionName) {
            return r.id;
        }
    }
    return PLAYER_RELIGION_ID;
}

void Game::refreshFaithFog() {
    m_fog.setEnabled(!m_cheatLuminusActive);
    if (!m_fog.isEnabled()) {
        return;
    }
    m_fog.update(m_map, m_gestorHumanos);
}

void Game::initFaithFog(int playerRelId) {
    m_cheatLuminusActive = false;
    m_fog.reset(m_map.getWidth(), m_map.getHeight());
    m_fog.setPlayerReligionId(playerRelId > 0 ? playerRelId : PLAYER_RELIGION_ID);
    m_fog.setEnabled(true);
    m_fog.update(m_map, m_gestorHumanos);
    m_renderer.setFogOfWar(&m_fog);
    focusCameraOnPlayerFaith();
    std::cout << "[Niebla] Vision de la fe activa para religion #" << m_fog.getPlayerReligionId()
              << " (truco LUMINUS para disiparla)." << std::endl;
}

void Game::focusCameraOnPlayerFaith() {
    const int rel = m_fog.getPlayerReligionId();
    int chamanId = m_gestorHumanos.buscarChamanDeReligion(rel);
    if (chamanId > 0) {
        m_gestorHumanos.seleccionarHumano(chamanId);
        const Humano* ch = m_gestorHumanos.getHumanoSeleccionado();
        if (ch && m_map.isValid(ch->getCeldaX(), ch->getCeldaY())) {
            m_camera.centerOnCell(ch->getCeldaX(), ch->getCeldaY(), m_map.getWidth(), m_map.getHeight());
            selectCell(ch->getCeldaX(), ch->getCeldaY());
            return;
        }
    }

    for (const auto& hObj : m_gestorHumanos.getHumanos()) {
        if (hObj.getReligionId() == rel && m_map.isValid(hObj.getCeldaX(), hObj.getCeldaY())) {
            m_camera.centerOnCell(hObj.getCeldaX(), hObj.getCeldaY(), m_map.getWidth(), m_map.getHeight());
            selectCell(hObj.getCeldaX(), hObj.getCeldaY());
            return;
        }
    }

    for (int y = 0; y < m_map.getHeight(); ++y) {
        for (int x = 0; x < m_map.getWidth(); ++x) {
            const Cell& c = m_map.getCell(x, y);
            int hsRel = (c.holySiteReligionId > 0) ? c.holySiteReligionId : c.dominantReligionId;
            if (c.hasHolySite && hsRel == rel) {
                m_camera.centerOnCell(x, y, m_map.getWidth(), m_map.getHeight());
                selectCell(x, y);
                return;
            }
        }
    }
}

void Game::handleX11Events() {
#if HAS_X11
    if (!m_renderer.hasActiveWindow() || !m_renderer.getDisplay()) return;

    Display* dpy = static_cast<Display*>(m_renderer.getDisplay());

    while (XPending(dpy) > 0) {
        XEvent ev;
        XNextEvent(dpy, &ev);

        switch (ev.type) {
            case ConfigureNotify: {
                int nw = ev.xconfigure.width;
                int nh = ev.xconfigure.height;
                if (nw != m_windowWidth || nh != m_windowHeight) {
                    onWindowResize(nw, nh);
                }
                break;
            }
            case ButtonPress: {
                int btn = ev.xbutton.button;
                float mx = static_cast<float>(ev.xbutton.x);
                float my = static_cast<float>(ev.xbutton.y);

                if (btn == Button1) {
                    onMouseDown(1, mx, my);
                } else if (btn == Button3) {
                    onMouseDown(3, mx, my);
                } else if (btn == Button4) { // Wheel Up
                    onMouseWheel(1.0f, mx, my);
                } else if (btn == Button5) { // Wheel Down
                    onMouseWheel(-1.0f, mx, my);
                }
                break;
            }
            case ButtonRelease: {
                int btn = ev.xbutton.button;
                float mx = static_cast<float>(ev.xbutton.x);
                float my = static_cast<float>(ev.xbutton.y);
                if (btn == Button1) {
                    onMouseUp(1, mx, my);
                } else if (btn == Button3) {
                    onMouseUp(3, mx, my);
                }
                break;
            }
            case MotionNotify: {
                float mx = static_cast<float>(ev.xmotion.x);
                float my = static_cast<float>(ev.xmotion.y);
                onMouseMove(mx, my);
                break;
            }
            case KeyPress: {
                char buf[32];
                KeySym ks = 0;
                int len = XLookupString(&ev.xkey, buf, sizeof(buf) - 1, &ks, nullptr);
                if (len > 0) {
                    buf[len] = '\0';
                }

                if (ks == XK_Escape) {
                    onKeyPress(27);
                } else if (ks == XK_Return || ks == XK_KP_Enter) {
                    onKeyPress(13);
                } else if (ks == XK_BackSpace) {
                    onKeyPress(8);
                } else if (ks == XK_space) {
                    onKeyPress(32);
                } else if (len == 1) {
                    onKeyPress(static_cast<uint8_t>(buf[0]));
                } else if (ks >= XK_0 && ks <= XK_9) {
                    onKeyPress('0' + (ks - XK_0));
                } else if (ks == XK_w || ks == XK_W) {
                    onKeyPress('w');
                } else if (ks == XK_a || ks == XK_A) {
                    onKeyPress('a');
                } else if (ks == XK_s || ks == XK_S) {
                    onKeyPress('s');
                } else if (ks == XK_d || ks == XK_D) {
                    onKeyPress('d');
                } else if (ks == XK_r || ks == XK_R) {
                    onKeyPress('r');
                } else if (ks == XK_plus || ks == XK_equal || ks == XK_KP_Add) {
                    onKeyPress('+');
                } else if (ks == XK_minus || ks == XK_underscore || ks == XK_KP_Subtract) {
                    onKeyPress('-');
                }
                break;
            }
            case ClientMessage: {
                m_isRunning = false;
                break;
            }
            default:
                break;
        }
    }
#endif
}

void Game::handleWindowEvents() {
#if HAS_X11
    handleX11Events();
#elif defined(HAS_WIN32)
    MSG msg;
    while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) {
            m_isRunning = false;
            break;
        }
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
#endif
}

void Game::run() {
    auto lastTime = std::chrono::high_resolution_clock::now();

    while (m_isRunning) {
        auto now = std::chrono::high_resolution_clock::now();
        float dt = std::chrono::duration<float>(now - lastTime).count();
        lastTime = now;
        dt = std::min(dt, 0.1f); // Cap dt

        handleWindowEvents();
        update(dt);
        render();

        std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60 FPS
    }
}

void Game::startNewCustomWorld() {
    std::cout << "==========================================================" << std::endl;
    std::cout << "[Génesis] Creando nuevo mundo sagrado personalizado..." << std::endl;
    std::cout << "  - Dios Guía: " << m_creationConfig.godName << std::endl;
    std::cout << "  - Religión Elegida: " << m_creationConfig.religionName << std::endl;
    std::cout << "  - Semilla (Seed): " << m_creationConfig.mapSeed << std::endl;
    std::cout << "  - Población Inicial: " << m_creationConfig.initialPopulation << std::endl;
    std::cout << "  - Época de Inicio: Era " << static_cast<int>(m_creationConfig.startingEra) << std::endl;
    std::cout << "  - Cofre Sagrado: " << (m_creationConfig.starterChest ? "ACTIVADO" : "DESACTIVADO") << std::endl;
    std::cout << "==========================================================" << std::endl;

    // 1. Determinar tamaño de mapa
    int mapDim = (m_creationConfig.mapSizeIndex == 0) ? 32 : (m_creationConfig.mapSizeIndex == 2 ? 64 : 48);
    m_map = WorldMap(mapDim, mapDim);
    m_map.generateProceduralWorld(m_creationConfig.mapSeed);

    // Ajustar población total de las celdas según la población inicial solicitada (escala 1:1 con aldeanos reales)
    // Inicialmente reseteamos la población de celdas a 0 para que cada aldeano real instanciado sume 1
    for (int y = 0; y < mapDim; ++y) {
        for (int x = 0; x < mapDim; ++x) {
            Cell& c = m_map.getCell(x, y);
            c.population = 0;
        }
    }

    // 2. Determinar año de inicio según la época
    int startYear = -4000;
    switch (m_creationConfig.startingEra) {
        case HistoricalEra::PrimordialAnimism: startYear = -4000; break;
        case HistoricalEra::TotemismAncestorWorship: startYear = -2500; break;
        case HistoricalEra::StructuralShamanism: startYear = -1000; break;
        case HistoricalEra::TerritorialPolytheism: startYear = 0; break;
        case HistoricalEra::UniversalReligions: startYear = 1000; break;
        case HistoricalEra::RationalizationSecularization: startYear = 1800; break;
        case HistoricalEra::PostSecular: startYear = 2020; break;
        case HistoricalEra::TranshumanistPostBiological: startYear = 2100; break;
        default: startYear = -4000; break;
    }

    m_registry = ReligionRegistry{};
    m_timeline.reset(startYear, 2200);
    m_timeEngine.reset(startYear);

    // Paleta de colores
    static const ColorRGB INFLUENCE_PALETTE[5] = {
        ColorRGB{230, 161, 59},  // Oro Divino
        ColorRGB{128, 0, 32},    // Rubí Sangre
        ColorRGB{52, 152, 219},  // Monada Azul
        ColorRGB{46, 204, 113},  // Esmeralda Gaia
        ColorRGB{155, 89, 182}   // Púrpura Dharma
    };
    ColorRGB playerColor = INFLUENCE_PALETTE[m_creationConfig.influenceColorIndex % 5];

    // 3. Crear Religión del Jugador
    int playerRelId = m_registry.createPrimordialReligion(
        m_creationConfig.religionName,
        m_creationConfig.godName,
        playerColor,
        "Sun",
        startYear
    );
    Religion* pRel = m_registry.getReligion(playerRelId);
    if (pRel) {
        pRel->currentEra = m_creationConfig.startingEra;
        if (m_creationConfig.starterChest) {
            pRel->theologyPoints += 1000;
            pRel->spiritualPiety = 0.95f;
            pRel->zeal = 0.85f;
        }
    }

    // 4. Crear 4 religiones rivales (con dificultad adaptada)
    float rivalZeal = 0.5f;
    if (m_creationConfig.aiDifficulty == AIDifficulty::Facil) rivalZeal = 0.35f;
    else if (m_creationConfig.aiDifficulty == AIDifficulty::Dificil) rivalZeal = 0.75f;
    else if (m_creationConfig.aiDifficulty == AIDifficulty::Pesadilla) rivalZeal = 0.95f;

    struct RivalDef { const char* name; const char* founder; ColorRGB color; const char* symbol; };
    RivalDef rivals[4] = {
        {"Animist Gaia-Cult", "Zarah-Khep", ColorRGB(46, 204, 113), "Tree"},
        {"The Sacred Monad", "Melchizedek", ColorRGB(52, 152, 219), "Spiral"},
        {"Order of the Eternal Flame", "Mithras-Gard", ColorRGB(230, 57, 70), "Flame"},
        {"Wheel of the Star-Dharma", "Siddhartha-Yin", ColorRGB(155, 89, 182), "Wheel"}
    };

    int rivalIds[4];
    for (int i = 0; i < 4; ++i) {
        rivalIds[i] = m_registry.createPrimordialReligion(
            rivals[i].name, rivals[i].founder, rivals[i].color, rivals[i].symbol, startYear
        );
        Religion* r = m_registry.getReligion(rivalIds[i]);
        if (r) {
            r->currentEra = m_creationConfig.startingEra;
            r->zeal = rivalZeal;
        }
    }

    // 5. Sembrar templos y creyentes en el mapa
    std::vector<Point2i> fertileLocations;
    for (int y = 5; y < mapDim - 5; ++y) {
        for (int x = 5; x < mapDim - 5; ++x) {
            const Cell& c = m_map.getCell(x, y);
            if (c.biome == BiomeType::FertileValley || c.biome == BiomeType::CoastalPlains) {
                fertileLocations.push_back({x, y});
            }
        }
    }

    if (!fertileLocations.empty()) {
        // Asignar el hogar central al Jugador
        size_t centerIdx = fertileLocations.size() / 2;
        Point2i playerHearth = fertileLocations[centerIdx];
        Cell& c = m_map.getCell(playerHearth.x, playerHearth.y);
        c.setShare(playerRelId, 0.98f);
        c.devotion = 0.95f;
        m_map.consecrateHolySite(playerHearth.x, playerHearth.y, "Santuario Sagrado de " + m_creationConfig.godName);
        c.shrineLevel = m_creationConfig.starterChest ? 3 : 2;

        // Sembrar rivales en los 4 cuadrantes alrededor
        int step = std::max(1, static_cast<int>(fertileLocations.size()) / 5);
        const int rivalSlots[4] = {0, 1, 3, 4};
        for (int i = 0; i < 4; ++i) {
            size_t idx = std::min(static_cast<size_t>(rivalSlots[i] * step), fertileLocations.size() - 1);
            if (idx == centerIdx && idx + 1 < fertileLocations.size()) idx++;
            Point2i pt = fertileLocations[idx];
            Cell& rc = m_map.getCell(pt.x, pt.y);
            rc.setShare(rivalIds[i], 0.90f);
            rc.devotion = 0.85f;
            m_map.consecrateHolySite(pt.x, pt.y, "Templo Arcano de " + std::string(rivals[i].founder));
            rc.shrineLevel = 2;
        }
    }

    m_map.recalculateReligions(m_registry);
    m_camera.reset(m_map.getWidth(), m_map.getHeight());

    // 6. Inicializar población humana de aldeanos sobre el mapa (1 aldeano real = 1 habitante)
    m_gestorHumanos.inicializarPoblacion(m_map, m_registry, m_creationConfig.initialPopulation);
    m_map.recalculateReligions(m_registry);

    if (const Humano* selChaman = m_gestorHumanos.getHumanoSeleccionado()) {
        selectCell(selChaman->getCeldaX(), selChaman->getCeldaY());
    }

    initFaithFog(playerRelId);

    // Registrar evento inicial de la epopeya
    m_eventLine.addEvent(
        startYear, EventCategory::BirthOfReligion,
        "Manifestacion Divina de " + m_creationConfig.godName,
        "El Dios " + m_creationConfig.godName + " ha establecido su pacto eterno fundando " +
        m_creationConfig.religionName + (m_creationConfig.starterChest ? " con el bendito Cofre Sagrado." : "."),
        {mapDim / 2, mapDim / 2}, playerRelId, 0, playerColor
    );

    // Cambiar estado a Playing
    m_gameState = GameState::Playing;
    m_activeInputField = 0;

    // Guardado inicial del nuevo mundo estilo Minecraft (level.dat + icon.ppm)
    saveCurrentGame();
}

void Game::loadStateFromSave(const WorldMap& map, const ReligionRegistry& registry,
                           int currentYear, const std::string& godName,
                           const std::string& religionName, uint32_t seed) {
    m_map = map;
    m_registry = registry;
    m_creationConfig.godName = godName;
    m_creationConfig.religionName = religionName;
    m_creationConfig.mapSeed = seed;

    m_timeline.reset(currentYear, 2200);
    m_timeline.setCurrentYear(currentYear);
    m_timeEngine.reset(currentYear);

    m_camera.reset(m_map.getWidth(), m_map.getHeight());

    // Normalizar maxPopulation y elevaciones cohesivas para partidas guardadas
    for (int y = 0; y < m_map.getHeight(); ++y) {
        for (int x = 0; x < m_map.getWidth(); ++x) {
            Cell& c = m_map.getCell(x, y);
            if (c.isWater()) {
                c.elevation = 0.0f;
            } else if (c.biome == BiomeType::RuggedMountains) {
                // Convertir antiguas losas grises centrales en colinas verdes habitables
                c.biome = ((x + y) % 3 == 0) ? BiomeType::HighlandHills : BiomeType::FertileValley;
                c.elevation = (c.biome == BiomeType::HighlandHills) ? 0.95f : 0.50f;
                c.maxPopulation = 6;
            } else if (c.biome == BiomeType::HighlandHills) {
                c.elevation = 0.95f;
            } else {
                c.elevation = 0.50f;
            }
            if (c.isHabitable() && c.maxPopulation > 15) {
                c.maxPopulation = 6;
            }
        }
    }

    int totalSavedPop = m_map.getTotalWorldPopulation();
    if (totalSavedPop > 250 || totalSavedPop <= 0) {
        // Migrar partidas antiguas (ej. 600,000 hab) a 80 aldeanos reales 1:1 para evitar colapso
        m_gestorHumanos.inicializarPoblacion(m_map, m_registry, 80);
    } else {
        // Restaurar aldeanos reales 1:1 en cada celda poblada según el mapa guardado
        m_gestorHumanos.getHumanos().clear();
        std::map<int, bool> hasShamanForRel;
        static const char* NOMBRES_CHAMAN[6] = {
            "Chaman Arandu", "Chaman Arandu", "Chaman Khep",
            "Chaman Orph", "Chaman Mithra", "Chaman Dharma"
        };
        int sId = 1;
        int firstShamanId = -1;
        for (int y = 0; y < m_map.getHeight(); ++y) {
            for (int x = 0; x < m_map.getWidth(); ++x) {
                Cell& c = m_map.getCell(x, y);
                int count = std::clamp(c.population, 0, 10);
                c.population = count;
                for (int p = 0; p < count; ++p) {
                    float offX = 0.25f + 0.15f * static_cast<float>(p % 3);
                    float offY = 0.25f + 0.15f * static_cast<float>((p / 3) % 3);
                    int curId = sId++;
                    int relId = (c.dominantReligionId > 0) ? c.dominantReligionId : 1;
                    bool makeShaman = !hasShamanForRel[relId];
                    std::string hName = makeShaman
                        ? std::string(NOMBRES_CHAMAN[std::clamp(relId, 0, 5)])
                        : ("Grog #" + std::to_string(curId));
                    Humano h(curId, hName, static_cast<float>(x) + offX, static_cast<float>(y) + offY);
                    h.setEsChaman(makeShaman);
                    h.setReligionId(relId);
                    h.setDevocion(makeShaman ? 0.98f : c.devotion);
                    if (makeShaman) {
                        hasShamanForRel[relId] = true;
                        if (relId == 1 || firstShamanId < 0) firstShamanId = curId;
                    }
                    m_gestorHumanos.agregarHumano(h);
                }
            }
        }
        if (firstShamanId > 0) {
            m_gestorHumanos.seleccionarHumano(firstShamanId);
        }
        m_gestorHumanos.actualizar(0.0f, m_map);
    }
    m_map.recalculateReligions(m_registry);

    initFaithFog(resolvePlayerReligionId());

    m_gameState = GameState::Playing;
    saveCurrentGame();
}

void Game::saveCurrentGame(const std::string& slotName) {
    std::string folder = slotName;
    if (folder.empty()) {
        std::string safeGod = m_creationConfig.godName;
        for (char& c : safeGod) {
            if (!std::isalnum(static_cast<unsigned char>(c))) c = '_';
        }
        folder = "Mundo_" + safeGod + "_" + std::to_string(m_creationConfig.mapSeed);
    }
    SaveSystem::saveWorld(*this, folder);
}

void Game::runHeadless(int maxTicks, const std::string& exportPPM) {
    if (m_registry.getAllReligions().empty()) {
        startNewCustomWorld();
    }
    m_gameState = GameState::Playing;
    std::cout << "[Simulator] Running headless simulation for " << maxTicks << " steps..." << std::endl;
    float dt = 0.2f;

    for (int i = 0; i < maxTicks; ++i) {
        update(dt);
    }

    render();
    if (!exportPPM.empty()) {
        if (m_renderer.exportFrameToPPM(exportPPM)) {
            std::cout << "[Simulator] Frame exported to: " << exportPPM << std::endl;
        } else {
            std::cerr << "[Simulator] Failed to export frame to: " << exportPPM << std::endl;
        }
    }

    std::cout << "[Simulator] Simulation completed. Final Year: "
              << Timeline::formatYear(m_timeline.getCurrentYear()) << std::endl;
    std::cout << "[Simulator] Total Religions active: " << m_registry.getAllReligions().size() << std::endl;
    for (const auto& r : m_registry.getAllReligions()) {
        std::cout << "  - " << r.name << " (" << r.symbol << ") Followers: "
                  << r.totalFollowers << " Dominant Cells: " << r.totalCellsDominant
                  << " Era: " << AdvancementSystem::getEraName(r.currentEra)
                  << (r.isExtinct ? " [EXTINCT]" : "") << std::endl;
    }
}

} // namespace ReligionSim
