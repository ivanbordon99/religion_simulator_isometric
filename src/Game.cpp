#include "Game.hpp"
#include "SaveSystem.hpp"
#include <iostream>
#include <chrono>
#include <thread>
#include <map>
#include <cctype>
#include <ctime>
#include <random>

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
#ifdef None
#undef None
#endif
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
            } else if (wParam == VK_F11) {
                g_win32GameInstance->onKeyPress(1011);
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
    m_renderer.setDayNightCycle(&m_dayNightCycle);
    m_renderer.setCloudSystem(&m_cloudSystem);
    m_renderer.setNemesisSififus(&m_nemesisSififus);
    m_renderer.setMoralitySystem(&m_moralitySystem);
    m_renderer.setBuildingSystem(&m_buildingSystem);
}

Game::~Game() {
    m_soundSystem.shutdown();
}

bool Game::init() {
#ifdef HAS_WIN32
    g_win32GameInstance = this;
#endif
    // Generar una semilla aleatoria por defecto para el primer arranque en la ventana de creación
    m_creationConfig.mapSeed = generateUniqueSeed();

    // Inicializar sistema de sonido y música de la placa de sonido
    m_soundSystem.init();

    // EN: Load persisted user preferences and synchronize subsystems
    // ES: Cargar preferencias de usuario persistidas y sincronizar subsistemas
    loadSettings();
    applySettings(m_settings);

    // EN: Setup living simulated world panorama for the Minecraft-inspired main menu
    // ES: Configurar panorama de mundo simulado vivo para la pantalla principal inspirada en Minecraft
    setupMenuPanoramaWorld();

    // Try initializing window (X11 / Win32 / OpenGL)
    const bool shouldStartFullscreen = (m_settings.displayMode == DisplayMode::BorderlessFullscreen);
    bool windowOk = m_renderer.initWindow("Marangatu: The Faith Simulator [Pre-Alpha v0.4.0]", shouldStartFullscreen);
    if (windowOk) {
        // EN: Synchronize window viewport and camera dimensions with fullscreen status
        // ES: Sincronizar dimensiones del viewport y cámara con el estado de pantalla completa
        if (shouldStartFullscreen) {
            m_renderer.setFullscreen(true);
            m_windowWidth = m_renderer.getWidth();
            m_windowHeight = m_renderer.getHeight();
            m_camera.setScreenSize(m_windowWidth, m_windowHeight);
        }
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
    m_renderer.update(deltaTime);

    if (m_gameState == GameState::MainMenu ||
        m_gameState == GameState::WorldCreation ||
        m_gameState == GameState::LoadWorldMenu ||
        m_gameState == GameState::Options ||
        m_gameState == GameState::Tutorial) {

        // EN: Update cinematic panoramic camera drift inspired by Minecraft title screen
        // ES: Actualizar desplazamiento cinemático panorámico de cámara inspirado en la pantalla de inicio de Minecraft
        m_menuPanoramaTime += deltaTime;
        const float panVelocityX = std::cos(m_menuPanoramaTime * 0.08f) * 14.0f * deltaTime;
        const float panVelocityY = std::sin(m_menuPanoramaTime * 0.05f) * 8.0f * deltaTime;
        m_camera.pan(panVelocityX, panVelocityY);
        m_camera.update(deltaTime);

        // EN: Advance living simulated world in menu background (units, circadian cycle, clouds)
        // ES: Avanzar mundo simulado vivo en el fondo del menú (unidades, ciclo circadiano, nubes)
        m_dayNightCycle.update(deltaTime * 0.25f);
        m_cloudSystem.update(deltaTime * 0.20f);
        m_gestorHumanos.actualizar(deltaTime * 0.70f, m_map, &m_eventLine, m_timeline.getCurrentYear(),
                                   &m_registry, &m_soundSystem, &m_camera, nullptr, nullptr,
                                   &m_dayNightCycle, &m_buildingSystem);

        m_soundSystem.update(deltaTime, m_map, m_camera, &m_dayNightCycle, nullptr,
                             m_gestorHumanos, false, 1.0f, m_timeline.getCurrentYear());
        return;
    }

    if (m_gameState == GameState::MapEditor) {
        if (m_mapEditorConfig.viewMode == MapEditorViewMode::Isometric25D) {
            m_camera.update(deltaTime);
        }
        if (m_mapEditorConfig.statusTimer > 0.0f) {
            m_mapEditorConfig.statusTimer -= deltaTime;
            if (m_mapEditorConfig.statusTimer <= 0.0f) {
                m_mapEditorConfig.statusMessage = "LISTO PARA ESCULPIR Y PINTAR EL MUNDO";
            }
        }
        m_soundSystem.setSimulationTimeState(true, 0.0f, m_timeline.getCurrentYear());
        return;
    }

    if (m_gameState == GameState::GameOver) {
        if (m_isSpectatingPostDefeat) {
            m_camera.update(deltaTime);
        }
        return;
    }

    if (m_gameState != GameState::Playing) {
        m_soundSystem.setSimulationTimeState(true, 0.0f, m_timeline.getCurrentYear());
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

    // EN: 1. Synchronize solar day/night cycle, hour counter and timeline progression with sprites
    // ES: 1. Sincronizar ciclo solar día/noche, contador de horas y línea temporal con sprites
    bool isPaused = m_timeline.isPaused();
    float speedMult = isPaused ? 0.0f : static_cast<float>(m_timeline.getSpeedMultiplier());
    float spriteDt = deltaTime * speedMult;

    // EN: Update solar 24-hour cycle (hours pass contemplatively according to speed multiplier)
    // ES: Actualizar ciclo solar de 24 horas (las horas pasan contemplativamente según el multiplicador)
    m_dayNightCycle.setPaused(isPaused);
    m_dayNightCycle.update(spriteDt);

    // EN: Synchronize timeline strictly with completed days and hour progress
    // ES: Sincronizar la línea de tiempo estrictamente con los días completados y el avance de horas
    float dtYears = 0.0f;
    m_timeline.updateSynchronized(deltaTime, m_dayNightCycle, dtYears);
    int currentYear = m_timeline.getCurrentYear();
    m_timeEngine.setYear(static_cast<double>(currentYear));

    // Actualizar paisaje sonoro ambiental y música adaptativa sincronizados exactamente con el tiempo de la simulación
    m_soundSystem.update(deltaTime, m_map, m_camera, &m_dayNightCycle, &m_divinePowers, m_gestorHumanos, isPaused, speedMult, currentYear);

    float cloudDt = isPaused ? (deltaTime * 0.35f) : (spriteDt + deltaTime * 0.20f);
    m_cloudSystem.update(cloudDt);

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
        m_gestorHumanos.actualizar(spriteDt, m_map, &m_eventLine, m_timeline.getCurrentYear(),
                                   &m_registry, &m_soundSystem, &m_camera, &m_nemesisSififus,
                                   &m_moralitySystem, &m_dayNightCycle, &m_buildingSystem);
    }

    // Actualización del Sistema Némesis y Motor Sísifo (propagación de mitos y modulación de fieles)
    m_nemesisSififus.update(dtYears > 0.0f ? dtYears : (spriteDt * 0.02f), currentYear, m_gestorHumanos, m_map, &m_registry, &m_eventLine);

    // Actualización del Sistema de Moral Religiosa y Reglas (Gracia comunitaria y mandatos)
    m_moralitySystem.update(dtYears > 0.0f ? dtYears : (spriteDt * 0.02f), currentYear, m_gestorHumanos, m_map, m_registry, &m_eventLine);

    // Actualización del Sistema de Construcción de Casas por Necesidad
    if (spriteDt > 0.0f) {
        m_buildingSystem.update(spriteDt, m_map, m_gestorHumanos, &m_eventLine, currentYear, &m_soundSystem, &m_camera, &m_moralitySystem);
    }

    if (spriteDt > 0.0f && dtYears <= 0.0f) {
        m_map.recalculateReligions(m_registry);
    }

    if (dtYears > 0.0f) {
        std::vector<GameEvent> newEvents;

        // 1. Advance eras & theological doctrines
        size_t prevEvCount = newEvents.size();
        m_advancement.updateAdvancements(m_registry, m_timeline.getCurrentYear(), newEvents);
        if (newEvents.size() > prevEvCount) {
            m_soundSystem.playSFX(SFXType::EraAscension, 0.95f);
        }

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

        // EN: 6. Record history snapshots for timeline scrubbing
        // ES: 6. Registrar instantáneas históricas para el desplazamiento temporal
        const int mapWidth = m_map.getWidth();
        const int mapHeight = m_map.getHeight();
        std::vector<uint8_t> dominanceMap(mapWidth * mapHeight, 0);
        for (int cellY = 0; cellY < mapHeight; ++cellY) {
            for (int cellX = 0; cellX < mapWidth; ++cellX) {
                dominanceMap[cellY * mapWidth + cellX] = static_cast<uint8_t>(m_map.getCell(cellX, cellY).dominantReligionId);
            }
        }

        std::vector<std::pair<int, int>> religionFollowers;
        for (const auto& religionItem : m_registry.getAllReligions()) {
            religionFollowers.push_back({religionItem.id, religionItem.totalFollowers});
        }
        m_timeline.recordSnapshot(m_map.getTotalWorldPopulation(), dominanceMap, religionFollowers);
    }

    for (const auto& fx : m_divinePowers.getActiveFX()) {
        if (fx.type == DivinePowerType::Meteor && !fx.impactTriggered && (fx.timer + deltaTime >= 1.25f)) {
            m_camera.addScreenShake(20.0f, 4.0f);
            m_dayNightCycle.triggerLightningFlash(1.2f);
        } else if (fx.type == DivinePowerType::Volcano) {
            if (std::fmod(fx.timer, 2.5f) < deltaTime) {
                m_camera.addScreenShake(3.5f, 1.0f);
            }
        } else if (fx.type == DivinePowerType::Wildfire) {
            if (std::fmod(fx.timer, 3.0f) < deltaTime) {
                m_camera.addScreenShake(1.5f, 0.5f);
            }
        } else if (fx.type == DivinePowerType::AtomicBomb) {
            // EN: Supersonic blast wave shakes camera dynamically as it sweeps the world
            // ES: La onda expansiva supersónica sacude la cámara dinámicamente al barrer el mundo
            if (fx.timer < 3.2f) {
                const float waveAlpha = 1.0f - (fx.timer / 3.2f);
                m_camera.addScreenShake(14.0f * waveAlpha, 0.25f);
            }
        }
    }

    // EN: Update divine powers, natural disasters, and physical debris simulation
    // ES: Actualizar poderes divinos, desastres naturales y simulación de escombros físicos
    m_divinePowers.update(deltaTime, m_map, m_gestorHumanos, &m_buildingSystem, &m_replication);

    // EN: Update thermodynamic fire physics across the entire world (spread, dissipation, and contact burns)
    // ES: Actualizar física termodinámica del fuego en todo el mundo (expansión, disipación y quemaduras)
    m_map.updateFirePhysics(deltaTime, &m_buildingSystem, &m_gestorHumanos,
                            m_cloudSystem.getWindX(), m_cloudSystem.getWindY());

    // EN: Update hydrology, fluid dynamics and water physics (surface runoff, drainage, fire quenching, and absorption)
    // ES: Actualizar hidrología, dinámica de fluidos y física del agua (escorrentía, drenaje, extinción y absorción)
    m_map.updateWaterPhysics(deltaTime, &m_buildingSystem, &m_gestorHumanos);

    refreshFaithFog();

    // 7. Verificación de Condición de Derrota: Extinción Total de la Fe
    // "recuerda que pierdes la partida cuando todas tus unidades mueren, porque ya no pueden replicar tu fe"
    if (m_gameOverTriggered) {
        m_gameOverTimer -= deltaTime;
        if (m_gameOverTimer <= 0.0f) {
            m_gameState = GameState::GameOver;
        }
    } else {
        int playerRel = resolvePlayerReligionId();
        int livingBelievers = m_gestorHumanos.contarHumanosVivosDeReligion(playerRel);
        int totalBelievers = m_gestorHumanos.contarHumanosTotalesDeReligion(playerRel);

        if (totalBelievers > 0 && livingBelievers == 0) {
            m_gameOverTriggered = true;
            m_gameOverTimer = 1.8f; // Pausa dramática fúnebre de 1.8 segundos antes de mostrar el epitafio
            m_extinctionYear = currentYear;
            m_soundSystem.playSFX(SFXType::UnitDeath, 1.0f);

            GameEvent extEv;
            extEv.year = currentYear;
            extEv.title = "EXTINCION TOTAL DE LA FE";
            extEv.description = "Todos tus fieles han muerto. Sin almas vivas para orar y replicar tu dogma, tu divinidad se extingue para siempre.";
            extEv.category = EventCategory::Extinction;
            extEv.location = m_selectedCell;
            extEv.religionAId = playerRel;
            extEv.badgeColor = {200, 30, 40};
            extEv.isTurningPoint = true;
            m_eventLine.addEvent(extEv);

            std::cout << "[GameOver] " << extEv.title << " en " << Timeline::formatYear(currentYear)
                      << " - Todos los fieles han muerto." << std::endl;
        }
    }
}

void Game::render() {
    m_renderer.setDayNightCycle(&m_dayNightCycle);
    m_renderer.setCloudSystem(&m_cloudSystem);
    m_renderer.beginFrame();

    if (m_gameState == GameState::MainMenu) {
        // EN: Render living simulated world in background of main menu (Minecraft-style panorama)
        // ES: Renderizar mundo simulado vivo en el fondo del menú principal (panorama estilo Minecraft)
        m_renderer.setFogOfWar(nullptr);
        m_renderer.renderWorldBackground(m_map, m_registry, m_replication,
                                         m_gestorHumanos, m_camera,
                                         &m_dayNightCycle, &m_cloudSystem);
        m_renderer.renderMainMenu(m_hoveredMenuButtonId);
    } else if (m_gameState == GameState::Tutorial) {
        m_renderer.setFogOfWar(nullptr);
        m_renderer.renderWorldBackground(m_map, m_registry, m_replication,
                                         m_gestorHumanos, m_camera,
                                         &m_dayNightCycle, &m_cloudSystem);
        if (m_settings.vignetteEnabled) {
            m_renderer.applyCinematicVignette();
        }
        m_renderer.renderTutorialMenu(m_currentTutorialTab, m_hoveredMenuButtonId);
    } else if (m_gameState == GameState::WorldCreation) {
        m_renderer.setFogOfWar(nullptr);
        m_renderer.renderWorldBackground(m_map, m_registry, m_replication,
                                         m_gestorHumanos, m_camera,
                                         &m_dayNightCycle, &m_cloudSystem);
        m_renderer.renderWorldCreationMenu(m_creationConfig, m_hoveredMenuButtonId, m_activeInputField);
    } else if (m_gameState == GameState::LoadWorldMenu) {
        m_renderer.setFogOfWar(nullptr);
        m_renderer.renderWorldBackground(m_map, m_registry, m_replication,
                                         m_gestorHumanos, m_camera,
                                         &m_dayNightCycle, &m_cloudSystem);
        m_renderer.renderLoadWorldMenu(m_hoveredMenuButtonId, m_selectedSaveIndex);
    } else if (m_gameState == GameState::Options) {
        m_renderer.setFogOfWar(nullptr);
        m_renderer.renderWorldBackground(m_map, m_registry, m_replication,
                                         m_gestorHumanos, m_camera,
                                         &m_dayNightCycle, &m_cloudSystem);
        if (m_settings.vignetteEnabled) {
            m_renderer.applyCinematicVignette();
        }
        m_renderer.renderOptionsMenu(m_settings, m_currentOptionsTab, m_hoveredMenuButtonId);
    } else if (m_gameState == GameState::MapEditor) {
        m_renderer.setFogOfWar(nullptr);
        if (m_mapEditorConfig.viewMode == MapEditorViewMode::Cartographic2D) {
            const Point2i hoveredEditorCell = m_renderer.screenToMapEditorCell(m_mouseX, m_mouseY, m_editorMap);
            m_renderer.renderMapEditor(m_mapEditorConfig, m_editorMap, m_hoveredMenuButtonId, m_activeInputField, hoveredEditorCell);
        } else {
            int hoveredCellGridX = -1;
            int hoveredCellGridY = -1;
            m_camera.screenToGrid(m_mouseX, m_mouseY, hoveredCellGridX, hoveredCellGridY);
            m_renderer.renderMapEditorIsometric(m_mapEditorConfig, m_editorMap, m_camera, m_hoveredMenuButtonId,
                                                Point2i{hoveredCellGridX, hoveredCellGridY},
                                                m_registry, m_replication, m_gestorHumanos);
        }
    } else if (m_gameState == GameState::GameOver) {
        if (m_isSpectatingPostDefeat) {
            // EN: In spectator mode, lift fog if reveal is enabled and render pure world without gameplay HUD
            // ES: En modo espectador, levantar niebla si está habilitado y renderizar mundo puro sin HUD de juego
            m_renderer.setFogOfWar(m_spectatorRevealAll ? nullptr : &m_fog);
            m_renderer.renderWorldBackground(m_map, m_registry, m_replication,
                                             m_gestorHumanos, m_camera,
                                             &m_dayNightCycle, &m_cloudSystem);
            int hoverGx = -1, hoverGy = -1;
            m_camera.screenToGrid(m_mouseX, m_mouseY, hoverGx, hoverGy);
            m_renderer.renderPostDefeatSpectatorHUD(m_creationConfig, m_extinctionYear,
                                                   Point2i{hoverGx, hoverGy}, m_map,
                                                   m_camera, m_spectatorRevealAll, m_hoveredMenuButtonId);
        } else {
            m_renderer.setFogOfWar(&m_fog);
            int hoverGx = -1, hoverGy = -1;
            m_camera.screenToGrid(m_mouseX, m_mouseY, hoverGx, hoverGy);
            m_renderer.render(m_map, m_registry, m_timeline, m_eventLine,
                              m_replication, m_gestorHumanos, m_camera, m_selectedCell, m_mapMode,
                              m_creationConfig, &m_advancement, &m_divinePowers, {hoverGx, hoverGy},
                              &m_dayNightCycle, &m_cloudSystem);
            const Religion* pRel = m_registry.getReligion(resolvePlayerReligionId());
            int eraIdx = pRel ? static_cast<int>(pRel->currentEra) : 0;
            m_renderer.renderGameOver(m_creationConfig, m_extinctionYear, eraIdx, m_hoveredMenuButtonId);
        }
    } else {
        m_renderer.setFogOfWar(&m_fog);
        int hoverGx = -1, hoverGy = -1;
        m_camera.screenToGrid(m_mouseX, m_mouseY, hoverGx, hoverGy);
        m_renderer.render(m_map, m_registry, m_timeline, m_eventLine,
                          m_replication, m_gestorHumanos, m_camera, m_selectedCell, m_mapMode,
                          m_creationConfig, &m_advancement, &m_divinePowers, {hoverGx, hoverGy},
                          &m_dayNightCycle, &m_cloudSystem);
        m_renderer.renderCommandConsole(m_commandBoxOpen, m_commandInput,
                          m_cheatSelectorActive, m_commandFeedback,
                          m_cheatLuminusActive);
    }

    // EN: Render developer mode telemetry HUD, unit inspector and isometric 3D axes
    // ES: Renderizar HUD de telemetría, inspector de unidades y ejes 3D isométricos del modo desarrollador
    if (m_renderer.isDeveloperMode()) {
        int hoverCellX = -1;
        int hoverCellY = -1;
        m_camera.screenToGrid(m_mouseX, m_mouseY, hoverCellX, hoverCellY);

        const int hoveredVillagerId = m_gestorHumanos.findVillagerOnScreen(m_mouseX, m_mouseY, m_camera, m_map, -1);
        const Humano* hoveredVillager = nullptr;
        if (hoveredVillagerId > 0) {
            for (const auto& villagerUnit : m_gestorHumanos.getVillagers()) {
                if (villagerUnit.getId() == hoveredVillagerId) {
                    hoveredVillager = &villagerUnit;
                    break;
                }
            }
        }
        const Humano* selectedVillager = m_gestorHumanos.getSelectedVillager();

        m_renderer.renderDeveloperOverlay(m_map, m_registry, m_timeline, m_gestorHumanos,
                                          m_camera, Point2i{hoverCellX, hoverCellY},
                                          hoveredVillager, selectedVillager,
                                          m_creationConfig, &m_timeEngine, &m_dayNightCycle);
    }

    // EN: Apply post-processing visual tint filter selected in options
    // ES: Aplicar filtro de tinte visual de postprocesamiento seleccionado en opciones
    if (m_settings.colorFilter != VisualColorFilter::Neutral) {
        m_renderer.applyVisualColorFilter(m_settings.colorFilter);
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

void Game::selectCell(int gridCellX, int gridCellY) {
    if (m_map.isValid(gridCellX, gridCellY)) {
        m_selectedCell = {gridCellX, gridCellY};
    }
}

void Game::castDivinePower(DivinePowerType divineType, int targetGridX, int targetGridY) {
    const int playerRel = resolvePlayerReligionId();
    const CastResult castResult = m_divinePowers.castPower(divineType, targetGridX, targetGridY, m_map, m_registry,
                                                           m_gestorHumanos, m_timeline.getCurrentYear(),
                                                           m_eventLine, playerRel,
                                                           &m_buildingSystem, &m_replication);
    if (castResult.success) {
        showFeedback(castResult.message, 5.0f);
        if (divineType == DivinePowerType::Lightning) {
            m_dayNightCycle.triggerLightningFlash(0.35f);
            m_soundSystem.playSpatialSFX(SFXType::ThunderStrike, targetGridX, targetGridY, m_camera, 1.25f);
        } else if (divineType == DivinePowerType::Earthquake) {
            m_camera.addScreenShake(9.0f, 5.0f);
            m_soundSystem.playSpatialSFX(SFXType::Earthquake, targetGridX, targetGridY, m_camera, 1.15f);
        } else if (divineType == DivinePowerType::Tornado) {
            m_camera.addScreenShake(3.5f, 2.5f);
            m_soundSystem.playSpatialSFX(SFXType::TornadoWhoosh, targetGridX, targetGridY, m_camera, 1.05f);
        } else if (divineType == DivinePowerType::Meteor) {
            m_camera.addScreenShake(5.0f, 1.25f);
            m_soundSystem.playSpatialSFX(SFXType::MeteorImpact, targetGridX, targetGridY, m_camera, 1.40f);
        } else if (divineType == DivinePowerType::Volcano) {
            m_camera.addScreenShake(10.0f, 4.0f);
            m_dayNightCycle.triggerLightningFlash(0.6f);
            m_soundSystem.playSpatialSFX(SFXType::MeteorImpact, targetGridX, targetGridY, m_camera, 1.30f);
        } else if (divineType == DivinePowerType::Wildfire) {
            m_camera.addScreenShake(4.0f, 2.0f);
            m_dayNightCycle.triggerLightningFlash(0.4f);
            m_soundSystem.playSpatialSFX(SFXType::MeteorImpact, targetGridX, targetGridY, m_camera, 0.95f);
        } else if (divineType == DivinePowerType::Rain) {
            m_soundSystem.playSpatialSFX(SFXType::TornadoWhoosh, targetGridX, targetGridY, m_camera, 0.65f);
        } else if (divineType == DivinePowerType::Deluge) {
            m_camera.addScreenShake(7.0f, 3.5f);
            m_dayNightCycle.triggerLightningFlash(0.5f);
            m_soundSystem.playSpatialSFX(SFXType::Earthquake, targetGridX, targetGridY, m_camera, 1.10f);
        } else if (divineType == DivinePowerType::AtomicBomb) {
            // EN: Cataclysmic nuclear whiteout thermal flash, extreme screen shake, and authentic atomic explosion sound
            // ES: Destello blanco térmico nuclear cataclísmico, sacudida extrema de pantalla y sonido auténtico de explosión atómica
            m_camera.addScreenShake(30.0f, 6.0f);
            m_dayNightCycle.triggerLightningFlash(2.0f);
            m_soundSystem.playSpatialSFX(SFXType::AtomicExplosion, targetGridX, targetGridY, m_camera, 2.0f);
        } else if (divineType == DivinePowerType::Plague) {
            m_soundSystem.playSpatialSFX(SFXType::PlagueSwarm, targetGridX, targetGridY, m_camera, 1.0f);
        } else if (divineType == DivinePowerType::Miracle) {
            m_soundSystem.playSpatialSFX(SFXType::FaithGain, targetGridX, targetGridY, m_camera, 1.1f);
        } else if (divineType == DivinePowerType::Congregate) {
            m_soundSystem.playSpatialSFX(SFXType::PrayerChant, targetGridX, targetGridY, m_camera, 1.0f);
        }

        // EN: Mythogenesis and recording survivors in Nemesis & Sisyphus system
        // ES: Mitogénesis y Registro de Sobrevivientes en el Sistema Némesis & Sísifo
        for (auto& livingVillager : m_gestorHumanos.getHumanos()) {
            if (!livingVillager.estaVivo()) continue;
            const float deltaWorldX = livingVillager.getX() - static_cast<float>(targetGridX);
            const float deltaWorldY = livingVillager.getY() - static_cast<float>(targetGridY);
            if (std::hypot(deltaWorldX, deltaWorldY) <= 4.8f) {
                if (divineType == DivinePowerType::Miracle || divineType == DivinePowerType::Rain) {
                    m_nemesisSififus.onUnitSparedOrBlessed(livingVillager, m_timeline.getCurrentYear(), &m_eventLine, &m_registry);
                } else if (divineType == DivinePowerType::Lightning || divineType == DivinePowerType::Meteor ||
                           divineType == DivinePowerType::Earthquake || divineType == DivinePowerType::Volcano ||
                           divineType == DivinePowerType::Wildfire || divineType == DivinePowerType::Deluge ||
                           divineType == DivinePowerType::AtomicBomb) {
                    m_nemesisSififus.onUnitHitByDivinePower(livingVillager, 35.0f, m_timeline.getCurrentYear(), &m_eventLine, &m_registry);
                }
            }
        }
        if (m_map.isValid(targetGridX, targetGridY)) {
            selectCell(targetGridX, targetGridY);
        }
    } else {
        showFeedback(castResult.message, 3.5f);
    }
}

void Game::toggleDivinePowerTargeting(DivinePowerType type) {
    if (m_activeDivinePower == type) {
        m_activeDivinePower = DivinePowerType::Inactive;
        m_renderer.setActiveDivinePower(DivinePowerType::Inactive);
        showFeedback("Modo apuntar desactivado.", 2.0f);
    } else {
        int cost = m_divinePowers.getPowerCost(type);
        Religion* pRel = m_registry.getReligion(resolvePlayerReligionId());
        if (pRel && pRel->theologyPoints < cost) {
            showFeedback("FE INSUFICIENTE: Se requieren " + std::to_string(cost) +
                         " pts de Fe para " + m_divinePowers.getPowerName(type) + ".", 3.5f);
            return;
        }
        m_activeDivinePower = type;
        m_renderer.setActiveDivinePower(type);
        showFeedback("[MODO APUNTAR] " + m_divinePowers.getPowerName(type) + " (Costo: " +
                     std::to_string(cost) + " Fe) - Haz clic en el mapa.", 4.0f);
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

// EN: Consecrate or upgrade a holy site at the selected cell
// ES: Consagrar o mejorar un sitio sagrado en la celda seleccionada
void Game::buildHolySiteAtSelection() {
    if (m_map.isValid(m_selectedCell.x, m_selectedCell.y)) {
        Cell& selectedCellRef = m_map.getCell(m_selectedCell.x, m_selectedCell.y);
        if (!selectedCellRef.hasHolySite) {
            m_map.consecrateHolySite(m_selectedCell.x, m_selectedCell.y, "Grand Sanctuary of the Chosen");
        } else {
            m_map.upgradeHolySite(m_selectedCell.x, m_selectedCell.y);
        }

        m_eventLine.addEvent(
            m_timeline.getCurrentYear(), EventCategory::HolySiteErected,
            "Monument Consecrated",
            "A monumental sacred sanctuary was consecrated at grid (" +
            std::to_string(m_selectedCell.x) + ", " + std::to_string(m_selectedCell.y) + ").",
            m_selectedCell, selectedCellRef.dominantReligionId, 0, ColorRGB(255, 215, 0)
        );
        m_map.recalculateReligions(m_registry);
    }
}

void Game::processButtonAction(int buttonId) {
    m_soundSystem.playSFX(SFXType::UIClick, 0.65f);

    // El desplegable de opciones sólo se mantiene abierto al pulsar su propio botón MENU
    if (buttonId != 400) {
        m_renderer.setOptionsDropdownOpen(false);
    }

    switch (buttonId) {
        case 400: // MENU -> abrir/cerrar el desplegable de opciones de partida
            m_renderer.setOptionsDropdownOpen(!m_renderer.isOptionsDropdownOpen());
            break;
        case 450: // EVENTOS -> abrir/cerrar la ventana de eventos avanzados / crónica sagrada
            m_renderer.toggleChronicle();
            if (m_renderer.isChronicleOpen()) {
                m_renderer.setFaithTreeOpen(false);
            }
            break;
        case 470: // MANDATOS -> abrir/cerrar modal de moral religiosa y mandatos
            m_renderer.toggleMoralRulesModal();
            if (m_renderer.isMoralRulesModalOpen()) {
                m_renderer.setChronicleOpen(false);
                m_renderer.setFaithTreeOpen(false);
                m_renderer.setMemoryTreeOpen(false);
            }
            break;
        case 460: // AVANCES -> abrir/cerrar el árbol teológico de avances de la fe
            m_renderer.toggleFaithTree();
            if (m_renderer.isFaithTreeOpen()) {
                m_renderer.setChronicleOpen(false);
                m_renderer.setMoralRulesModalOpen(false);
            }
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
            returnToMainMenu();
            break;

        // Selección del líder espiritual (Chamán o Profeta)
        case 700: {
            int pRel = resolvePlayerReligionId();
            int lId = m_gestorHumanos.buscarLiderEspiritualDeReligion(pRel);
            if (lId <= 0) {
                std::string t, n;
                lId = DivinePowersSystem::ensureIntermediary(m_gestorHumanos, m_registry, pRel, t, n);
            }
            if (lId > 0) {
                m_gestorHumanos.seleccionarHumano(lId);
                const Humano* ch = m_gestorHumanos.getHumanoSeleccionado();
                if (ch && m_map.isValid(ch->getCeldaX(), ch->getCeldaY())) {
                    m_camera.centerOnCell(ch->getCeldaX(), ch->getCeldaY(), m_map.getWidth(), m_map.getHeight());
                    selectCell(ch->getCeldaX(), ch->getCeldaY());
                }
            }
            break;
        }

        // Ventana Modal del Árbol de Recuerdos (Sistema Sísifo & Némesis)
        case 750: {
            m_renderer.toggleMemoryTree();
            if (m_renderer.isMemoryTreeOpen()) {
                m_renderer.setChronicleOpen(false);
                m_renderer.setFaithTreeOpen(false);
            }
            break;
        }

        // Poderes Divinos del Usuario e Intermediario
        case 710:
        case 701:
            toggleDivinePowerTargeting(DivinePowerType::Lightning);
            break;
        case 711:
            toggleDivinePowerTargeting(DivinePowerType::Plague);
            break;
        case 712:
        case 703:
            toggleDivinePowerTargeting(DivinePowerType::Miracle);
            break;
        case 713:
        case 702:
            castDivinePower(DivinePowerType::Congregate, 0, 0);
            break;
        case 720:
            toggleDivinePowerTargeting(DivinePowerType::Tornado);
            break;
        case 721:
            toggleDivinePowerTargeting(DivinePowerType::Earthquake);
            break;
        case 722:
            toggleDivinePowerTargeting(DivinePowerType::Pandemic);
            break;
        case 723:
            toggleDivinePowerTargeting(DivinePowerType::Meteor);
            break;
        case 724:
            toggleDivinePowerTargeting(DivinePowerType::Volcano);
            break;
        case 725:
            toggleDivinePowerTargeting(DivinePowerType::Wildfire);
            break;
        case 726:
            toggleDivinePowerTargeting(DivinePowerType::Rain);
            break;
        case 727:
            toggleDivinePowerTargeting(DivinePowerType::Deluge);
            break;
        default: break;
    }
}

void Game::onMouseDown(int mouseButton, float screenPosX, float screenPosY) {
    m_mouseX = screenPosX;
    m_mouseY = screenPosY;
    if (m_gameState == GameState::MainMenu) {
        if (mouseButton == 1) {
            const int clickedButtonId = m_renderer.checkMenuButtonClicked(screenPosX, screenPosY, false);
            if (clickedButtonId == 1) { // CREAR MUNDO -> Abrir ventana de configuración con nueva semilla fresca
                prepareForWorldCreation(true);
            } else if (clickedButtonId == 2) { // CARGAR MUNDO -> Abrir selector estilo Minecraft
                m_gameState = GameState::LoadWorldMenu;
                m_selectedSaveIndex = 0;
            } else if (clickedButtonId == 6) { // CREACION DE MAPAS -> Taller litúrgico de creación y edición
                setupMapEditor();
                m_soundSystem.playSFX(SFXType::UIClick);
            } else if (clickedButtonId == 3) { // APRENDE A JUGAR -> Códice de aprendizaje y tutorial sagrado
                m_previousGameState = GameState::MainMenu;
                m_gameState = GameState::Tutorial;
                m_currentTutorialTab = TutorialTab::BasicsAndFaith;
                m_soundSystem.playSFX(SFXType::UIClick);
            } else if (clickedButtonId == 4) { // OPCIONES
                m_previousGameState = GameState::MainMenu;
                m_gameState = GameState::Options;
                m_soundSystem.playSFX(SFXType::UIClick);
            } else if (clickedButtonId == 5) { // SALIR
                m_isRunning = false;
            }
        }
        return;
    }

    if (m_gameState == GameState::MapEditor) {
        if (m_mapEditorConfig.viewMode == MapEditorViewMode::Cartographic2D) {
            if (mouseButton == 1) {
                const int btnId = m_renderer.checkMapEditorButtonClicked(screenPosX, screenPosY);
                if (btnId > 0) {
                    m_soundSystem.playSFX(SFXType::UIClick);
                    // Botones de acción principales
                    if (btnId == 880) { // Alternar a vista isométrica 2.5D
                        m_mapEditorConfig.viewMode = MapEditorViewMode::Isometric25D;
                        m_camera.reset(m_editorMap.getWidth(), m_editorMap.getHeight());
                        m_camera.centerOnCell(m_editorMap.getWidth() / 2, m_editorMap.getHeight() / 2,
                                              m_editorMap.getWidth(), m_editorMap.getHeight());
                        m_camera.setZoom(1.0f);
                        return;
                    } else if (btnId == 890) { // JUGAR ESTE MAPA
                        startPlayWithEditorMap();
                        return;
                    } else if (btnId == 891) { // GUARDAR MAPA
                        m_currentSaveSlot = SaveSystem::generateUniqueSlotName("CustomMap", m_mapEditorConfig.mapSeed);
                        SaveSystem::saveWorld(*this, m_currentSaveSlot);
                        m_mapEditorConfig.statusMessage = "MAPA GUARDADO EN PARTIDAS EXITOSAMENTE (" + m_currentSaveSlot + ")";
                        m_mapEditorConfig.statusTimer = 5.0f;
                        return;
                    } else if (btnId == 892) { // VOLVER AL MENÚ
                        returnToMainMenu();
                        return;
                    }
                    // Controles del Generador Procedimental
                    else if (btnId == 801) { // Tamaño Anterior
                        constexpr int TOTAL_MAP_SIZES = 8;
                        m_mapEditorConfig.mapSizeIndex = (m_mapEditorConfig.mapSizeIndex + TOTAL_MAP_SIZES - 1) % TOTAL_MAP_SIZES;
                        regenerateEditorMap();
                    } else if (btnId == 802) { // Tamaño Siguiente
                        constexpr int TOTAL_MAP_SIZES = 8;
                        m_mapEditorConfig.mapSizeIndex = (m_mapEditorConfig.mapSizeIndex + 1) % TOTAL_MAP_SIZES;
                        regenerateEditorMap();
                    } else if (btnId == 803) { // Clima Anterior
                        constexpr int TOTAL_CLIMATES = 4;
                        uint8_t prevClim = (static_cast<uint8_t>(m_mapEditorConfig.worldClimate) + TOTAL_CLIMATES - 1) % TOTAL_CLIMATES;
                        m_mapEditorConfig.worldClimate = static_cast<WorldClimate>(prevClim);
                        regenerateEditorMap();
                    } else if (btnId == 804) { // Clima Siguiente
                        constexpr int TOTAL_CLIMATES = 4;
                        uint8_t nextClim = (static_cast<uint8_t>(m_mapEditorConfig.worldClimate) + 1) % TOTAL_CLIMATES;
                        m_mapEditorConfig.worldClimate = static_cast<WorldClimate>(nextClim);
                        regenerateEditorMap();
                    } else if (btnId == 805) { // Abundancia Anterior
                        constexpr int TOTAL_ABUNDANCES = 3;
                        uint8_t prevAbund = (static_cast<uint8_t>(m_mapEditorConfig.resourceAbundance) + TOTAL_ABUNDANCES - 1) % TOTAL_ABUNDANCES;
                        m_mapEditorConfig.resourceAbundance = static_cast<ResourceAbundance>(prevAbund);
                        regenerateEditorMap();
                    } else if (btnId == 806) { // Abundancia Siguiente
                        constexpr int TOTAL_ABUNDANCES = 3;
                        uint8_t nextAbund = (static_cast<uint8_t>(m_mapEditorConfig.resourceAbundance) + 1) % TOTAL_ABUNDANCES;
                        m_mapEditorConfig.resourceAbundance = static_cast<ResourceAbundance>(nextAbund);
                        regenerateEditorMap();
                    } else if (btnId == 807) { // Semilla Cuadro de Texto
                        m_activeInputField = 807;
                    } else if (btnId == 808) { // Semilla Dado / Aleatoria
                        m_mapEditorConfig.mapSeed = generateUniqueSeed();
                        regenerateEditorMap();
                    } else if (btnId == 810) { // Regenerar Procedimental
                        regenerateEditorMap();
                    }
                    // Radio del Pincel
                    else if (btnId == 821) { m_mapEditorConfig.brushRadius = 1; }
                    else if (btnId == 822) { m_mapEditorConfig.brushRadius = 2; }
                    else if (btnId == 823) { m_mapEditorConfig.brushRadius = 3; }
                    // Biomas y Terreno
                    else if (btnId == 831) { m_mapEditorConfig.activeBrush = MapEditorBrush::Ocean; }
                    else if (btnId == 832) { m_mapEditorConfig.activeBrush = MapEditorBrush::ShallowWater; }
                    else if (btnId == 833) { m_mapEditorConfig.activeBrush = MapEditorBrush::FertileValley; }
                    else if (btnId == 834) { m_mapEditorConfig.activeBrush = MapEditorBrush::DenseForest; }
                    else if (btnId == 835) { m_mapEditorConfig.activeBrush = MapEditorBrush::HighlandHills; }
                    else if (btnId == 836) { m_mapEditorConfig.activeBrush = MapEditorBrush::RuggedMountains; }
                    else if (btnId == 837) { m_mapEditorConfig.activeBrush = MapEditorBrush::AridDesert; }
                    else if (btnId == 838) { m_mapEditorConfig.activeBrush = MapEditorBrush::FrozenTundra; }
                    else if (btnId == 839) { m_mapEditorConfig.activeBrush = MapEditorBrush::VolcanicCaldera; }
                    // Yacimientos y Recursos
                    else if (btnId == 841) { m_mapEditorConfig.activeBrush = MapEditorBrush::Wood; }
                    else if (btnId == 842) { m_mapEditorConfig.activeBrush = MapEditorBrush::Stone; }
                    else if (btnId == 843) { m_mapEditorConfig.activeBrush = MapEditorBrush::Gold; }
                    else if (btnId == 844) { m_mapEditorConfig.activeBrush = MapEditorBrush::Iron; }
                    else if (btnId == 845) { m_mapEditorConfig.activeBrush = MapEditorBrush::Copper; }
                    else if (btnId == 846) { m_mapEditorConfig.activeBrush = MapEditorBrush::Diamond; }
                    else if (btnId == 847) { m_mapEditorConfig.activeBrush = MapEditorBrush::ClearResource; }
                    // Elevación e Inspección
                    else if (btnId == 851) { m_mapEditorConfig.activeBrush = MapEditorBrush::RaiseElevation; }
                    else if (btnId == 852) { m_mapEditorConfig.activeBrush = MapEditorBrush::LowerElevation; }
                    else if (btnId == 855) { m_mapEditorConfig.activeBrush = MapEditorBrush::Inspect; }
                    return;
                }

                // Clic sobre el lienzo de vista previa cartográfica -> Pintar con el pincel activo
                const Point2i clickedCell = m_renderer.screenToMapEditorCell(screenPosX, screenPosY, m_editorMap);
                if (clickedCell.x >= 0 && clickedCell.y >= 0) {
                    m_isPaintingMapEditor = true;
                    applyMapEditorBrush(clickedCell.x, clickedCell.y);
                }
            }
            return;
        }

        // EN: Isometric 2.5D View Mode Mouse Handling
        // ES: Manejo del ratón en Modo de Vista Isométrica 2.5D
        if (mouseButton == 1) {
            const int isoBtnId = m_renderer.checkMapEditorIsometricButtonClicked(screenPosX, screenPosY);
            if (isoBtnId > 0) {
                m_soundSystem.playSFX(SFXType::UIClick);
                if (isoBtnId == 880) { // Alternar a vista cartográfica 2D
                    m_mapEditorConfig.viewMode = MapEditorViewMode::Cartographic2D;
                    return;
                } else if (isoBtnId == 890) { // JUGAR ESTE MAPA
                    startPlayWithEditorMap();
                    return;
                } else if (isoBtnId == 891) { // GUARDAR MAPA
                    m_currentSaveSlot = SaveSystem::generateUniqueSlotName("CustomMap", m_mapEditorConfig.mapSeed);
                    SaveSystem::saveWorld(*this, m_currentSaveSlot);
                    m_mapEditorConfig.statusMessage = "MAPA GUARDADO EN PARTIDAS EXITOSAMENTE (" + m_currentSaveSlot + ")";
                    m_mapEditorConfig.statusTimer = 5.0f;
                    return;
                } else if (isoBtnId == 892) { // SALIR
                    returnToMainMenu();
                    return;
                } else if (isoBtnId == 808) { // DADO
                    m_mapEditorConfig.mapSeed = generateUniqueSeed();
                    regenerateEditorMap();
                    return;
                } else if (isoBtnId == 810) { // REGENERAR
                    regenerateEditorMap();
                    return;
                }
                // Categorías de pestañas
                else if (isoBtnId == 871) { m_mapEditorConfig.paletteTab = 0; return; }
                else if (isoBtnId == 872) { m_mapEditorConfig.paletteTab = 1; return; }
                else if (isoBtnId == 873) { m_mapEditorConfig.paletteTab = 2; return; }
                // Radios de pincel
                else if (isoBtnId == 821) { m_mapEditorConfig.brushRadius = 1; return; }
                else if (isoBtnId == 822) { m_mapEditorConfig.brushRadius = 2; return; }
                else if (isoBtnId == 823) { m_mapEditorConfig.brushRadius = 3; return; }
                // Mosaicos del Atlas de terreno (900-915)
                else if (isoBtnId == 900) { m_mapEditorConfig.activeBrush = MapEditorBrush::TileGrass; return; }
                else if (isoBtnId == 901) { m_mapEditorConfig.activeBrush = MapEditorBrush::TileRock; return; }
                else if (isoBtnId == 902) { m_mapEditorConfig.activeBrush = MapEditorBrush::TileTropicalPlains; return; }
                else if (isoBtnId == 903) { m_mapEditorConfig.activeBrush = MapEditorBrush::TileFarmland; return; }
                else if (isoBtnId == 904) { m_mapEditorConfig.activeBrush = MapEditorBrush::TileDeepOceanAlt; return; }
                else if (isoBtnId == 905) { m_mapEditorConfig.activeBrush = MapEditorBrush::TileRichForest; return; }
                else if (isoBtnId == 906) { m_mapEditorConfig.activeBrush = MapEditorBrush::TileHighlands; return; }
                else if (isoBtnId == 907) { m_mapEditorConfig.activeBrush = MapEditorBrush::TileSwamp; return; }
                else if (isoBtnId == 908) { m_mapEditorConfig.activeBrush = MapEditorBrush::TileSand; return; }
                else if (isoBtnId == 909) { m_mapEditorConfig.activeBrush = MapEditorBrush::TileSnow; return; }
                else if (isoBtnId == 910) { m_mapEditorConfig.activeBrush = MapEditorBrush::TileShallowWater; return; }
                else if (isoBtnId == 911) { m_mapEditorConfig.activeBrush = MapEditorBrush::TileMidOcean; return; }
                else if (isoBtnId == 912) { m_mapEditorConfig.activeBrush = MapEditorBrush::TileCaveWater; return; }
                else if (isoBtnId == 913) { m_mapEditorConfig.activeBrush = MapEditorBrush::TileAbyssal; return; }
                else if (isoBtnId == 914) { m_mapEditorConfig.activeBrush = MapEditorBrush::TileOpenOcean; return; }
                else if (isoBtnId == 915) { m_mapEditorConfig.activeBrush = MapEditorBrush::TileLava; return; }
                // Recursos, Flora y Elementos
                else if (isoBtnId == 841) { m_mapEditorConfig.activeBrush = MapEditorBrush::Wood; return; }
                else if (isoBtnId == 842) { m_mapEditorConfig.activeBrush = MapEditorBrush::Stone; return; }
                else if (isoBtnId == 843) { m_mapEditorConfig.activeBrush = MapEditorBrush::Gold; return; }
                else if (isoBtnId == 844) { m_mapEditorConfig.activeBrush = MapEditorBrush::Iron; return; }
                else if (isoBtnId == 845) { m_mapEditorConfig.activeBrush = MapEditorBrush::Copper; return; }
                else if (isoBtnId == 846) { m_mapEditorConfig.activeBrush = MapEditorBrush::Diamond; return; }
                else if (isoBtnId == 839) { m_mapEditorConfig.activeBrush = MapEditorBrush::VolcanicCaldera; return; }
                else if (isoBtnId == 920) { m_mapEditorConfig.activeBrush = MapEditorBrush::TileCrater; return; }
                else if (isoBtnId == 921) { m_mapEditorConfig.activeBrush = MapEditorBrush::TileAsh; return; }
                else if (isoBtnId == 847) { m_mapEditorConfig.activeBrush = MapEditorBrush::ClearResource; return; }
                // Topografía y Biomas
                else if (isoBtnId == 851) { m_mapEditorConfig.activeBrush = MapEditorBrush::RaiseElevation; return; }
                else if (isoBtnId == 852) { m_mapEditorConfig.activeBrush = MapEditorBrush::LowerElevation; return; }
                else if (isoBtnId == 853) { m_mapEditorConfig.activeBrush = MapEditorBrush::FlattenElevation; return; }
                else if (isoBtnId == 855) { m_mapEditorConfig.activeBrush = MapEditorBrush::Inspect; return; }
                else if (isoBtnId == 831) { m_mapEditorConfig.activeBrush = MapEditorBrush::Ocean; return; }
                else if (isoBtnId == 832) { m_mapEditorConfig.activeBrush = MapEditorBrush::ShallowWater; return; }
                else if (isoBtnId == 833) { m_mapEditorConfig.activeBrush = MapEditorBrush::FertileValley; return; }
                else if (isoBtnId == 834) { m_mapEditorConfig.activeBrush = MapEditorBrush::DenseForest; return; }
                else if (isoBtnId == 835) { m_mapEditorConfig.activeBrush = MapEditorBrush::HighlandHills; return; }
                else if (isoBtnId == 836) { m_mapEditorConfig.activeBrush = MapEditorBrush::RuggedMountains; return; }
                else if (isoBtnId == 837) { m_mapEditorConfig.activeBrush = MapEditorBrush::AridDesert; return; }
                else if (isoBtnId == 838) { m_mapEditorConfig.activeBrush = MapEditorBrush::FrozenTundra; return; }
                return;
            }

            // EN: Clicked on the live isometric 2.5D world -> Paint cell under cursor
            // ES: Clic sobre el mundo 2.5D isométrico vivo -> Pintar celda bajo el cursor
            int isoTargetGridX = -1;
            int isoTargetGridY = -1;
            m_camera.screenToGrid(screenPosX, screenPosY, isoTargetGridX, isoTargetGridY);
            if (m_editorMap.isValid(isoTargetGridX, isoTargetGridY)) {
                m_isPaintingMapEditor = true;
                applyMapEditorBrush(isoTargetGridX, isoTargetGridY);
            }
        } else if (mouseButton == 2 || mouseButton == 3) {
            // EN: Right-click / Middle-click drags and pans isometric camera
            // ES: Clic derecho o central arrastra y desplaza la cámara isométrica
            m_camera.startDrag(screenPosX, screenPosY);
        }
        return;
    }

    if (m_gameState == GameState::Tutorial) {
        if (mouseButton == 1) {
            const int clickedTutorialButtonId = m_renderer.checkTutorialButtonClicked(screenPosX, screenPosY);
            // EN: Tab switching between the 4 codex categories
            // ES: Cambio de pestaña entre las 4 categorías del códice
            if (clickedTutorialButtonId == 2001) {
                m_currentTutorialTab = TutorialTab::BasicsAndFaith;
                m_soundSystem.playSFX(SFXType::UIClick);
            } else if (clickedTutorialButtonId == 2002) {
                m_currentTutorialTab = TutorialTab::ControlsAndCamera;
                m_soundSystem.playSFX(SFXType::UIClick);
            } else if (clickedTutorialButtonId == 2003) {
                m_currentTutorialTab = TutorialTab::VillagersAndEcology;
                m_soundSystem.playSFX(SFXType::UIClick);
            } else if (clickedTutorialButtonId == 2004) {
                m_currentTutorialTab = TutorialTab::MiraclesAndEras;
                m_soundSystem.playSFX(SFXType::UIClick);
            }
            // EN: Bottom pagination and return action controls
            // ES: Controles inferiores de paginación y acción de regreso
            else if (clickedTutorialButtonId == 2010) { // < ANTERIOR
                const int currentTabIndex = static_cast<int>(m_currentTutorialTab);
                if (currentTabIndex > 0) {
                    m_currentTutorialTab = static_cast<TutorialTab>(currentTabIndex - 1);
                    m_soundSystem.playSFX(SFXType::UIClick);
                }
            } else if (clickedTutorialButtonId == 2011) { // VOLVER AL MENU
                m_soundSystem.playSFX(SFXType::UIClick);
                returnToMainMenu();
            } else if (clickedTutorialButtonId == 2012) { // SIGUIENTE >
                const int currentTabIndex = static_cast<int>(m_currentTutorialTab);
                if (currentTabIndex + 1 < static_cast<int>(TutorialTab::Count)) {
                    m_currentTutorialTab = static_cast<TutorialTab>(currentTabIndex + 1);
                    m_soundSystem.playSFX(SFXType::UIClick);
                }
            }
        }
        return;
    }

    if (m_gameState == GameState::WorldCreation) {
        if (mouseButton == 1) {
            int btnId = m_renderer.checkCreationButtonClicked(screenPosX, screenPosY);
            if (btnId == 550) { // Cancelar y volver
                returnToMainMenu();
            } else if (btnId == 551) { // Comenzar Génesis
                startNewCustomWorld();
            } else if (btnId == 501) { // Input Dios
                m_activeInputField = 501;
            } else if (btnId == 502) { // Input Religion
                m_activeInputField = 502;
            } else if (btnId == 503) { // Input Seed
                m_activeInputField = 503;
            } else if (btnId == 504) { // Randomize Seed
                m_creationConfig.mapSeed = generateUniqueSeed();
            } else if (btnId == 505) { // Ciclar población de aldeanos reales
                if (m_creationConfig.initialPopulation == 40) m_creationConfig.initialPopulation = 80;
                else if (m_creationConfig.initialPopulation == 80) m_creationConfig.initialPopulation = 120;
                else if (m_creationConfig.initialPopulation == 120) m_creationConfig.initialPopulation = 180;
                else if (m_creationConfig.initialPopulation == 180) m_creationConfig.initialPopulation = 250;
                else if (m_creationConfig.initialPopulation == 250) m_creationConfig.initialPopulation = 350;
                else if (m_creationConfig.initialPopulation == 350) m_creationConfig.initialPopulation = 500;
                else m_creationConfig.initialPopulation = 40;
            } else if (btnId == 516) { // Input Recursos Iniciales (Caja de texto o ciclo al hacer clic)
                if (m_creationConfig.initialResources == 100) m_creationConfig.initialResources = 250;
                else if (m_creationConfig.initialResources == 250) m_creationConfig.initialResources = 500;
                else if (m_creationConfig.initialResources == 500) m_creationConfig.initialResources = 1000;
                else if (m_creationConfig.initialResources == 1000) m_creationConfig.initialResources = 2000;
                else m_creationConfig.initialResources = 100;
                m_activeInputField = 516;
            } else if (btnId == 506) { // Ciclar época de inicio
                uint8_t nextEra = (static_cast<uint8_t>(m_creationConfig.startingEra) + 1) % 8;
                m_creationConfig.startingEra = static_cast<HistoricalEra>(nextEra);
            } else if (btnId == 507) { // Ciclar tamaño del mapa
                constexpr int TOTAL_MAP_SIZES = 8;
                m_creationConfig.mapSizeIndex = (m_creationConfig.mapSizeIndex + 1) % TOTAL_MAP_SIZES;
            } else if (btnId == 520) { // Ciclar clima primordial del mundo
                uint8_t nextClimate = (static_cast<uint8_t>(m_creationConfig.worldClimate) + 1) % 4;
                m_creationConfig.worldClimate = static_cast<WorldClimate>(nextClimate);
            } else if (btnId == 517) { // Ciclar abundancia de recursos naturales
                uint8_t nextAbundance = (static_cast<uint8_t>(m_creationConfig.resourceAbundance) + 1) % 3;
                m_creationConfig.resourceAbundance = static_cast<ResourceAbundance>(nextAbundance);
            } else if (btnId == 508) { // Ciclar cataclismos
                m_creationConfig.disasterFrequency = (m_creationConfig.disasterFrequency + 1) % 3;
            } else if (btnId == 509) { // Ciclar tipo de partida
                uint8_t nextMode = (static_cast<uint8_t>(m_creationConfig.gameMode) + 1) % 4;
                m_creationConfig.gameMode = static_cast<GameModeType>(nextMode);
            } else if (btnId == 510) { // Cofre inicial toggle
                m_creationConfig.starterChest = !m_creationConfig.starterChest;
            } else if (btnId == 511) { // Ciclar dificultad IA (100% conductual sin ventajas numéricas)
                uint8_t nextDiff = (static_cast<uint8_t>(m_creationConfig.aiDifficulty) + 1) % 4;
                m_creationConfig.aiDifficulty = static_cast<AIDifficulty>(nextDiff);
            } else if (btnId == 521) { // Ciclar ritmo histórico / escala temporal
                uint8_t nextPacing = (static_cast<uint8_t>(m_creationConfig.timePacing) + 1) % 3;
                m_creationConfig.timePacing = static_cast<TimePacing>(nextPacing);
            } else if (btnId == 522) { // Ciclar beligerancia tribal / fricción liminal
                uint8_t nextBell = (static_cast<uint8_t>(m_creationConfig.tribalBelligerence) + 1) % 3;
                m_creationConfig.tribalBelligerence = static_cast<TribalBelligerence>(nextBell);
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
        if (mouseButton == 1) {
            auto saves = SaveSystem::scanSaveSlots();
            int btnId = m_renderer.checkLoadMenuButtonClicked(screenPosX, screenPosY, static_cast<int>(saves.size()));
            if (btnId >= 1000) {
                // Seleccionar slot de mundo
                m_selectedSaveIndex = btnId - 1000;
            } else if (btnId == 601) { // JUGAR MUNDO
                if (!saves.empty() && m_selectedSaveIndex >= 0 && m_selectedSaveIndex < static_cast<int>(saves.size())) {
                    SaveSystem::loadWorld(*this, saves[m_selectedSaveIndex].folderName);
                }
            } else if (btnId == 602) { // CREAR NUEVO
                prepareForWorldCreation(true);
            } else if (btnId == 603) { // ELIMINAR
                if (!saves.empty() && m_selectedSaveIndex >= 0 && m_selectedSaveIndex < static_cast<int>(saves.size())) {
                    SaveSystem::deleteWorld(saves[m_selectedSaveIndex].folderName);
                    m_selectedSaveIndex = 0;
                }
            } else if (btnId == 604) { // CANCELAR
                returnToMainMenu();
            }
        }
        return;
    }

    if (m_gameState == GameState::Options) {
        if (mouseButton == 1) {
            const int clickedButtonId = m_renderer.checkOptionsMenuButtonClicked(screenPosX, screenPosY, m_currentOptionsTab);
            if (clickedButtonId > 0) {
                m_soundSystem.playSFX(SFXType::UIClick);
            }

            // EN: Category navigation tabs
            // ES: Pestañas de navegación de categorías
            if (clickedButtonId == 1001) {
                m_currentOptionsTab = OptionsTab::ScreenAndVideo;
            } else if (clickedButtonId == 1002) {
                m_currentOptionsTab = OptionsTab::GraphicsAndImage;
            } else if (clickedButtonId == 1003) {
                m_currentOptionsTab = OptionsTab::GameplayAndSimulation;
            } else if (clickedButtonId == 1004) {
                m_currentOptionsTab = OptionsTab::AudioAndSound;
            }
            // EN: Screen and video options
            // ES: Opciones de pantalla y video
            else if (clickedButtonId == 1010) {
                m_settings.cycleResolution();
                applySettings(m_settings);
                saveSettings();
            } else if (clickedButtonId == 1011) {
                m_settings.cycleDisplayMode();
                applySettings(m_settings);
                saveSettings();
            } else if (clickedButtonId == 1012) {
                m_settings.cycleFrameRateLimit();
                applySettings(m_settings);
            } else if (clickedButtonId == 1013) {
                m_settings.toggleVerticalSync();
                applySettings(m_settings);
            } else if (clickedButtonId == 1014) {
                m_settings.cycleUiScale();
                applySettings(m_settings);
            } else if (clickedButtonId == 1015) {
                m_settings.cycleBrightness();
                applySettings(m_settings);
            }
            // EN: Graphics and image options
            // ES: Opciones de gráficos e imagen
            else if (clickedButtonId == 1020) {
                m_settings.cycleCloudShadows();
                applySettings(m_settings);
            } else if (clickedButtonId == 1021) {
                m_settings.cycleDynamicLighting();
                applySettings(m_settings);
            } else if (clickedButtonId == 1022) {
                m_settings.cycleWaterAnimation();
                applySettings(m_settings);
            } else if (clickedButtonId == 1023) {
                m_settings.toggleVignette();
                applySettings(m_settings);
            } else if (clickedButtonId == 1024) {
                m_settings.cycleScreenShake();
                applySettings(m_settings);
            } else if (clickedButtonId == 1025) {
                m_settings.cycleVisualColorFilter();
                applySettings(m_settings);
            } else if (clickedButtonId == 1026) {
                // EN: Toggle terrain isometric grid visualization
                // ES: Alternar visualización de retícula isométrica de terreno
                m_settings.toggleIsometricGrid();
                applySettings(m_settings);
            } else if (clickedButtonId == 1027) {
                // EN: Toggle developer mode telemetry and axes overlay
                // ES: Alternar telemetría y ejes del modo desarrollador
                m_settings.toggleDeveloperMode();
                applySettings(m_settings);
            }
            // EN: Gameplay and simulation options
            // ES: Opciones de jugabilidad y simulación
            else if (clickedButtonId == 1030) {
                m_settings.cycleSimulationSpeed();
                applySettings(m_settings);
            } else if (clickedButtonId == 1031) {
                m_settings.toggleAutoPauseOnDisasters();
                applySettings(m_settings);
            } else if (clickedButtonId == 1032) {
                m_settings.cycleCameraPanSpeed();
                applySettings(m_settings);
            } else if (clickedButtonId == 1033) {
                m_settings.toggleInvertCameraDrag();
                applySettings(m_settings);
            } else if (clickedButtonId == 1034) {
                m_settings.toggleFloatingNumbers();
                applySettings(m_settings);
            } else if (clickedButtonId == 1035) {
                m_settings.cycleLifeEventNotifications();
                applySettings(m_settings);
            }
            // EN: Audio and sound options
            // ES: Opciones de audio y sonido
            else if (clickedButtonId == 1040) {
                m_settings.cycleMasterVolume();
                applySettings(m_settings);
            } else if (clickedButtonId == 1041) {
                m_settings.cycleMusicVolume();
                applySettings(m_settings);
            } else if (clickedButtonId == 1042) {
                m_settings.cycleSfxVolume();
                applySettings(m_settings);
            } else if (clickedButtonId == 1043) {
                m_settings.cycleAmbientVolume();
                applySettings(m_settings);
            } else if (clickedButtonId == 1044) {
                m_settings.toggleSoundMute();
                applySettings(m_settings);
            }
            // EN: Bottom modal actions
            // ES: Acciones del pie del modal
            else if (clickedButtonId == 1090) { // Restaurar
                m_settings.resetToDefaults();
                applySettings(m_settings);
            } else if (clickedButtonId == 1091 || clickedButtonId == 12) { // Guardar y Volver
                saveSettings();
                m_gameState = m_previousGameState;
            }
        }
        return;
    }

    if (m_gameState == GameState::GameOver) {
        if (m_isSpectatingPostDefeat) {
            if (mouseButton == 1) {
                const int specBtnId = m_renderer.checkSpectatorButtonClicked(screenPosX, screenPosY);
                if (specBtnId == 905) { // VER EPITAFIO
                    m_isSpectatingPostDefeat = false;
                    m_soundSystem.playSFX(SFXType::UIClick);
                    return;
                } else if (specBtnId == 906) { // NIEBLA
                    m_spectatorRevealAll = !m_spectatorRevealAll;
                    m_soundSystem.playSFX(SFXType::UIClick);
                    return;
                } else if (specBtnId == 901) { // REINTENTAR GÉNESIS
                    prepareForWorldCreation(true);
                    return;
                } else if (specBtnId == 903) { // MENÚ PRINCIPAL
                    returnToMainMenu();
                    return;
                }
            }
            if (mouseButton == 1 || mouseButton == 2 || mouseButton == 3) {
                m_camera.startDrag(screenPosX, screenPosY);
            }
        } else {
            if (mouseButton == 1) {
                const int btnId = m_renderer.checkGameOverButtonClicked(screenPosX, screenPosY);
                if (btnId == 904) { // OBSERVAR MUNDO / VER MAPA
                    m_isSpectatingPostDefeat = true;
                    m_soundSystem.playSFX(SFXType::UIClick);
                } else if (btnId == 901) { // REINTENTAR GÉNESIS
                    prepareForWorldCreation(true);
                } else if (btnId == 902) { // CARGAR PARTIDA
                    m_gameState = GameState::LoadWorldMenu;
                    m_selectedSaveIndex = 0;
                    m_gameOverTriggered = false;
                    m_gameOverTimer = 0.0f;
                } else if (btnId == 903) { // MENÚ PRINCIPAL
                    returnToMainMenu();
                }
            }
        }
        return;
    }

    if (mouseButton == 1) { // Left Click
        m_renderer.setMouseDown(true, screenPosX, screenPosY);
        // 1. Check UI button clicks
        int btnId = m_renderer.checkButtonClicked(screenPosX, screenPosY);
        if (btnId > 0) {
            m_renderer.triggerButtonPress(btnId);
            processButtonAction(btnId);
            return;
        }
        // Un clic fuera del desplegable de opciones lo cierra (sin actuar sobre el mundo)
        if (m_renderer.isOptionsDropdownOpen()) {
            m_renderer.setOptionsDropdownOpen(false);
            return;
        }

        // Si el modal de la crónica de eventos avanzados está abierto, procesar sus clics interactivos
        if (m_renderer.isChronicleOpen()) {
            Point2i jumpTarget{-1, -1};
            int action = m_renderer.checkChronicleModalClick(screenPosX, screenPosY, m_eventLine,
                                                            m_renderer.getChronicleFilter(),
                                                            m_renderer.getChronicleScroll(),
                                                            jumpTarget);
            if (action == 1) { // Cerrar modal [X] o clic fuera
                m_renderer.setChronicleOpen(false);
                return;
            } else if (action >= 10 && action <= 16) { // Cambiar filtro
                m_renderer.setChronicleFilter(action - 10);
                return;
            } else if (action == 20) { // Scroll izq [<]
                m_renderer.scrollChronicle(-1);
                return;
            } else if (action == 21) { // Scroll der [>]
                m_renderer.scrollChronicle(1);
                return;
            } else if (action == 30) { // Salto a ubicación "IR AL LUGAR"
                if (m_map.isValid(jumpTarget.x, jumpTarget.y)) {
                    m_camera.centerOnCell(jumpTarget.x, jumpTarget.y, m_map.getWidth(), m_map.getHeight());
                    selectCell(jumpTarget.x, jumpTarget.y);
                }
                m_renderer.setChronicleOpen(false);
                return;
            }
            return; // Absorber clics dentro del modal
        }

        // Si el modal del árbol de avances está abierto, procesar sus clics interactivos
        if (m_renderer.isFaithTreeOpen()) {
            int selAdvId = m_renderer.getSelectedAdvancementId();
            int action = m_renderer.checkFaithTreeModalClick(screenPosX, screenPosY, m_registry, m_advancement, selAdvId);
            m_renderer.setSelectedAdvancementId(selAdvId);
            if (action == 1) { // Cerrar modal [X] o clic fuera
                m_renderer.setFaithTreeOpen(false);
                return;
            } else if (action == 2) { // Convalidar avance
                Religion* pRel = m_registry.getReligion(PLAYER_RELIGION_ID);
                if (pRel) {
                    const FaithAdvancement* targetAdv = m_advancement.getAdvancement(selAdvId);
                    const bool isEraCapstone = (targetAdv && targetAdv->isEraCapstone);
                    HistoricalEra unlockedEra = targetAdv ? targetAdv->unlocksEra : HistoricalEra::PrimordialAnimism;

                    std::vector<GameEvent> newEvents;
                    if (m_advancement.convalidateAdvancement(*pRel, selAdvId, m_timeline.getCurrentYear(), newEvents)) {
                        for (const auto& ev : newEvents) {
                            m_eventLine.addEvent(ev);
                        }

                        // EN: Apply civilizational momentum to the timeline
                        // ES: Aplicar impulso civilizatorio a la línea temporal
                        float leapDtYears = 0.0f;
                        if (isEraCapstone) {
                            const int targetEraStart = AdvancementSystem::getEraStartYear(unlockedEra);
                            const int currentYearVal = m_timeline.getCurrentYear();
                            const int leapAmount = std::max(50, targetEraStart - currentYearVal);
                            m_timeline.applyAdvancementHistoricalLeap(leapAmount, leapDtYears);
                            m_soundSystem.playSFX(SFXType::EraAscension, 1.0f);
                        } else {
                            constexpr int standardDoctrineLeapYears = 25;
                            m_timeline.applyAdvancementHistoricalLeap(standardDoctrineLeapYears, leapDtYears);
                            m_soundSystem.playSFX(SFXType::FaithGain, 0.9f);
                        }

                        if (leapDtYears > 0.0f) {
                            m_timeEngine.setYear(static_cast<double>(m_timeline.getCurrentYear()));
                            // EN: Propagate macro-historical changes
                            // ES: Propagar cambios macro-históricos
                            m_advancement.updateAdvancements(m_registry, m_timeline.getCurrentYear(), newEvents);
                            m_replication.update(m_map, m_registry, leapDtYears, newEvents, m_timeline.getCurrentYear());
                            m_agglutination.update(m_map, m_registry, leapDtYears, newEvents, m_timeline.getCurrentYear());
                            for (const auto& ev : newEvents) {
                                m_eventLine.addEvent(ev);
                            }
                        }
                    }
                }
                return;
            }
            return; // Absorber clics dentro del modal
        }

        // Si el modal del árbol de recuerdos (Sísifo & Némesis) está abierto, procesar sus clics interactivos
        if (m_renderer.isMemoryTreeOpen()) {
            int action = m_renderer.checkMemoryTreeModalClick(screenPosX, screenPosY);
            if (action == 1) { // Cerrar modal [X] o clic fuera
                m_renderer.setMemoryTreeOpen(false);
            }
            return; // Absorber clics dentro del modal
        }

        // Si el modal de moral religiosa y mandatos está abierto, procesar sus clics interactivos
        if (m_renderer.isMoralRulesModalOpen()) {
            int action = m_renderer.checkMoralRulesModalClick(screenPosX, screenPosY, m_moralitySystem, resolvePlayerReligionId(), m_registry);
            if (action == 1) { // Cerrar modal [X] o clic fuera
                m_renderer.setMoralRulesModalOpen(false);
            }
            return; // Absorber clics dentro del modal
        }

        const int botH = 118;
        const int botY = m_windowHeight - botH;

        // 2. Minimapa Clic (panel inferior derecho)
        // Mover la cámara directamente al hacer clic en el minimapa
        const int amarilloX = static_cast<int>(m_windowWidth * 0.815f);
        if (screenPosX >= amarilloX && screenPosY >= botY) {
            int mmX = 0, mmY = 0, mmW = 0, mmH = 0;
            m_renderer.getMinimapRect(mmX, mmY, mmW, mmH);
            if (screenPosX >= mmX && screenPosX <= mmX + mmW && screenPosY >= mmY && screenPosY <= mmY + mmH) {
                float normX = (screenPosX - mmX) / static_cast<float>(mmW);
                float normY = (screenPosY - mmY) / static_cast<float>(mmH);
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

        if (screenPosY >= trackY - 8 && screenPosY <= trackY + 16 && screenPosX >= trackX && screenPosX <= trackX + trackW) {
            float prog = (screenPosX - trackX) / static_cast<float>(trackW);
            m_timeline.scrubToProgress(prog);
            m_scrubbingTimeline = true;
            return;
        }

        // 4. Check Event Ribbon cards click (dentro de la zona azul)
        int cardY = botY + 28;
        int cardW = (azulW - 48) / 3;
        int cardH = 46;
        if (screenPosY >= cardY && screenPosY <= cardY + cardH && screenPosX >= trackX && screenPosX <= trackX + trackW) {
            auto recents = m_eventLine.getRecentEvents(3);
            for (size_t eventIndex = 0; eventIndex < recents.size(); ++eventIndex) {
                int cardPosX = trackX + static_cast<int>(eventIndex) * (cardW + 8);
                if (screenPosX >= cardPosX && screenPosX <= cardPosX + cardW) {
                    m_camera.centerOnCell(recents[eventIndex].location.x, recents[eventIndex].location.y,
                                          m_map.getWidth(), m_map.getHeight());
                    selectCell(recents[eventIndex].location.x, recents[eventIndex].location.y);
                    return;
                }
            }
        }

        // 5. Clic en el Mundo Isométrico (Lanzamiento de Poder Divino, o Selección de Aldeano/Celda)
        if (screenPosY < botY && screenPosY > 34) {
            int gridCellX = 0, gridCellY = 0;
            m_camera.screenToGrid(screenPosX, screenPosY, gridCellX, gridCellY);

            if (m_activeDivinePower != DivinePowerType::Inactive) {
                if (m_map.isValid(gridCellX, gridCellY)) {
                    DivinePowerType pow = m_activeDivinePower;
                    m_activeDivinePower = DivinePowerType::Inactive;
                    m_renderer.setActiveDivinePower(DivinePowerType::Inactive);
                    castDivinePower(pow, gridCellX, gridCellY);
                    return;
                }
            }

            // Primero buscar si se hizo clic directamente sobre el sprite de un aldeano en pantalla.
            // Fog of War: sólo se pueden seleccionar unidades del pueblo del jugador
            // (salvo con el truco SELECTOR o Modo Desarrollador, que permite seleccionar e inspeccionar cualquier unidad).
            const int onlyRel = (m_cheatSelectorActive || m_developerMode) ? -1 : PLAYER_RELIGION_ID;
            int humanoId = m_gestorHumanos.buscarHumanoEnPantalla(screenPosX, screenPosY, m_camera, m_map, onlyRel);
            if (humanoId <= 0) {
                humanoId = m_gestorHumanos.buscarHumanoCercano(static_cast<float>(gridCellX) + 0.5f,
                                                               static_cast<float>(gridCellY) + 0.5f, 0.85f, onlyRel);
            }
            if (humanoId > 0 && !m_cheatSelectorActive && !m_developerMode) {
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
                if (m_map.isValid(gridCellX, gridCellY)) {
                    // Las celdas ocultas por la niebla no revelan información
                    selectCell(gridCellX, gridCellY);
                }
            }
        }
    } else if (mouseButton == 3) { // Right Click: cancelar apuntado de poder divino, o mover unidad con SELECTOR, o arrastrar cámara
        if (m_activeDivinePower != DivinePowerType::Inactive) {
            m_activeDivinePower = DivinePowerType::Inactive;
            m_renderer.setActiveDivinePower(DivinePowerType::Inactive);
            showFeedback("Modo apuntar cancelado.", 2.0f);
            return;
        }
        const int botH = 118;
        const int botY = m_windowHeight - botH;
        Humano* selectedVillager = m_gestorHumanos.getHumanoSeleccionadoMutable();
        if (m_cheatSelectorActive && selectedVillager && screenPosY > 34 && screenPosY < botY) {
            const float halfTileWidth = BASE_TILE_WIDTH * 0.5f;
            const float halfTileHeight = BASE_TILE_HEIGHT * 0.5f;
            const float isoX = (screenPosX - m_camera.getCameraX()) / m_camera.getZoom();
            float isoY = (screenPosY - m_camera.getCameraY()) / m_camera.getZoom() + 0.50f * ELEVATION_STEP;
            const float normalizedIsoX = isoX / halfTileWidth;
            float normalizedIsoY = isoY / halfTileHeight;
            float gridWorldX = (normalizedIsoX + normalizedIsoY) * 0.5f;
            float gridWorldY = (normalizedIsoY - normalizedIsoX) * 0.5f;

            const int cellX = std::clamp(static_cast<int>(std::floor(gridWorldX)), 0, m_map.getWidth() - 1);
            const int cellY = std::clamp(static_cast<int>(std::floor(gridWorldY)), 0, m_map.getHeight() - 1);
            const float realElevation = m_map.getCell(cellX, cellY).elevation;

            isoY = (screenPosY - m_camera.getCameraY()) / m_camera.getZoom() + realElevation * ELEVATION_STEP;
            normalizedIsoY = isoY / halfTileHeight;
            gridWorldX = std::clamp((normalizedIsoX + normalizedIsoY) * 0.5f, 0.5f, static_cast<float>(m_map.getWidth()) - 0.5f);
            gridWorldY = std::clamp((normalizedIsoY - normalizedIsoX) * 0.5f, 0.5f, static_cast<float>(m_map.getHeight()) - 0.5f);

            const int targetCellX = std::clamp(static_cast<int>(std::floor(gridWorldX)), 0, m_map.getWidth() - 1);
            const int targetCellY = std::clamp(static_cast<int>(std::floor(gridWorldY)), 0, m_map.getHeight() - 1);
            if (!selectedVillager->puedeCaminarSobreAguaYTerrenoPeligroso() && !m_map.getCell(targetCellX, targetCellY).isPassableForGround()) {
                showFeedback("Las unidades terrestres no pueden transitar aguas ni terreno peligroso.", 2.5f);
            } else {
                selectedVillager->ordenarMovimientoManual(gridWorldX, gridWorldY);
                selectCell(targetCellX, targetCellY);
            }
        } else {
            m_camera.startDrag(screenPosX, screenPosY);
        }
    }
}

void Game::onMouseUp(int mouseButton, float screenPosX, float screenPosY) {
    if (m_gameState == GameState::GameOver && m_isSpectatingPostDefeat) {
        if (mouseButton == 1 || mouseButton == 2 || mouseButton == 3) {
            m_camera.endDrag();
        }
        return;
    }
    if (mouseButton == 1) {
        m_scrubbingTimeline = false;
        m_isPaintingMapEditor = false;
        m_renderer.setMouseDown(false, screenPosX, screenPosY);
    } else if (mouseButton == 2 || mouseButton == 3) {
        m_camera.endDrag();
    }
}

void Game::onMouseMove(float screenPosX, float screenPosY) {
    m_mouseX = screenPosX;
    m_mouseY = screenPosY;
    m_renderer.setMousePosition(screenPosX, screenPosY);
    if (m_gameState == GameState::MainMenu) {
        m_hoveredMenuButtonId = m_renderer.checkMenuButtonClicked(screenPosX, screenPosY, false);
        return;
    }
    if (m_gameState == GameState::MapEditor) {
        if (m_mapEditorConfig.viewMode == MapEditorViewMode::Cartographic2D) {
            m_hoveredMenuButtonId = m_renderer.checkMapEditorButtonClicked(screenPosX, screenPosY);
            if (m_isPaintingMapEditor) {
                const Point2i cell = m_renderer.screenToMapEditorCell(screenPosX, screenPosY, m_editorMap);
                if (cell.x >= 0 && cell.y >= 0) {
                    applyMapEditorBrush(cell.x, cell.y);
                }
            }
        } else {
            m_hoveredMenuButtonId = m_renderer.checkMapEditorIsometricButtonClicked(screenPosX, screenPosY);
            if (m_camera.isDragging()) {
                m_camera.updateDrag(screenPosX, screenPosY);
            } else if (m_isPaintingMapEditor) {
                int isoDragGridX = -1;
                int isoDragGridY = -1;
                m_camera.screenToGrid(screenPosX, screenPosY, isoDragGridX, isoDragGridY);
                if (m_editorMap.isValid(isoDragGridX, isoDragGridY)) {
                    applyMapEditorBrush(isoDragGridX, isoDragGridY);
                }
            }
        }
        return;
    }
    if (m_gameState == GameState::Tutorial) {
        m_hoveredMenuButtonId = m_renderer.checkTutorialButtonClicked(screenPosX, screenPosY);
        return;
    }
    if (m_gameState == GameState::WorldCreation) {
        m_hoveredMenuButtonId = m_renderer.checkCreationButtonClicked(screenPosX, screenPosY);
        return;
    }
    if (m_gameState == GameState::LoadWorldMenu) {
        auto saves = SaveSystem::scanSaveSlots();
        m_hoveredMenuButtonId = m_renderer.checkLoadMenuButtonClicked(screenPosX, screenPosY, static_cast<int>(saves.size()));
        return;
    }

    if (m_gameState == GameState::Options) {
        m_hoveredMenuButtonId = m_renderer.checkOptionsMenuButtonClicked(screenPosX, screenPosY, m_currentOptionsTab);
        return;
    }

    if (m_gameState == GameState::GameOver) {
        if (m_isSpectatingPostDefeat) {
            if (m_camera.isDragging()) {
                m_camera.updateDrag(screenPosX, screenPosY);
            }
            m_hoveredMenuButtonId = m_renderer.checkSpectatorButtonClicked(screenPosX, screenPosY);
        } else {
            m_hoveredMenuButtonId = m_renderer.checkGameOverButtonClicked(screenPosX, screenPosY);
        }
        return;
    }

    if (m_camera.isDragging()) {
        m_camera.updateDrag(screenPosX, screenPosY);
    } else if (m_scrubbingTimeline) {
        const int trackX = 40;
        const int trackW = m_windowWidth - 80;
        const float prog = (screenPosX - trackX) / static_cast<float>(trackW);
        m_timeline.scrubToProgress(prog);
    }
}

void Game::onMouseWheel(float wheelDelta, float screenPosX, float screenPosY) {
    if (m_renderer.isChronicleOpen()) {
        if (wheelDelta > 0.0f) {
            m_renderer.scrollChronicle(-1);
        } else {
            m_renderer.scrollChronicle(1);
        }
        return;
    }
    if (m_renderer.isMemoryTreeOpen()) {
        if (wheelDelta > 0.0f) {
            m_renderer.scrollMemoryTree(-1);
        } else {
            m_renderer.scrollMemoryTree(1);
        }
        return;
    }
    const float zoomFactor = (wheelDelta > 0.0f) ? 1.15f : 0.85f;
    m_camera.zoomBy(zoomFactor, screenPosX, screenPosY);
}

void Game::onKeyPress(int keySym) {
    // EN: Global F3 key shortcut to toggle Developer Mode telemetry and axes overlay
    // ES: Atajo global de tecla F3 para alternar telemetría y ejes del Modo Desarrollador
    constexpr int KEYSYM_F3_CUSTOM = 1003;
    constexpr int KEYSYM_F3_X11 = 0xFFBE; // XK_F3
    if (keySym == KEYSYM_F3_CUSTOM || keySym == KEYSYM_F3_X11 || keySym == '`' || keySym == '~') {
        toggleDeveloperMode();
        return;
    }

    // EN: Global F11 key shortcut to toggle Borderless Fullscreen / Windowed display mode
    // ES: Atajo global de tecla F11 para alternar modo de pantalla completa sin bordes / ventana
    constexpr int KEYSYM_F11_CUSTOM = 1011;
    constexpr int KEYSYM_F11_X11 = 0xFFC8; // XK_F11
    if (keySym == KEYSYM_F11_CUSTOM || keySym == KEYSYM_F11_X11) {
        m_settings.cycleDisplayMode();
        applySettings(m_settings);
        saveSettings();
        return;
    }

    if (m_gameState == GameState::GameOver) {
        if (m_isSpectatingPostDefeat) {
            // EN: Return to GameOver epitafio summary on ESC, V, or M
            // ES: Regresar al resumen del epitafio de derrota con ESC, V o M
            if (keySym == 27 || keySym == 'v' || keySym == 'V' || keySym == 'm' || keySym == 'M') {
                m_isSpectatingPostDefeat = false;
                m_soundSystem.playSFX(SFXType::UIClick);
                return;
            }
            if (keySym == 'f' || keySym == 'F') {
                m_spectatorRevealAll = !m_spectatorRevealAll;
                m_soundSystem.playSFX(SFXType::UIClick);
                return;
            }
            if (keySym == 'w' || keySym == 'W') {
                m_camera.pan(0.0f, -40.0f);
                return;
            }
            if (keySym == 's' || keySym == 'S') {
                m_camera.pan(0.0f, 40.0f);
                return;
            }
            if (keySym == 'a' || keySym == 'A') {
                m_camera.pan(-40.0f, 0.0f);
                return;
            }
            if (keySym == 'd' || keySym == 'D') {
                m_camera.pan(40.0f, 0.0f);
                return;
            }
            if (keySym == 'r' || keySym == 'R') {
                m_camera.centerOnCell(m_map.getWidth() / 2, m_map.getHeight() / 2,
                                      m_map.getWidth(), m_map.getHeight());
                return;
            }
            if (keySym == '+' || keySym == '=') {
                m_camera.zoomBy(1.15f, m_windowWidth * 0.5f, m_windowHeight * 0.5f);
                return;
            }
            if (keySym == '-' || keySym == '_') {
                m_camera.zoomBy(0.85f, m_windowWidth * 0.5f, m_windowHeight * 0.5f);
                return;
            }
            if (keySym == 'q' || keySym == 'Q') {
                returnToMainMenu();
                return;
            }
            return;
        }

        // EN: V or M key opens spectator map view from Game Over screen
        // ES: Tecla V o M abre la vista de espectador del mapa desde la pantalla de derrota
        if (keySym == 'v' || keySym == 'V' || keySym == 'm' || keySym == 'M') {
            m_isSpectatingPostDefeat = true;
            m_soundSystem.playSFX(SFXType::UIClick);
            return;
        }
        if (keySym == 27) { // ESC -> Salir al menú principal con panorama fresco
            returnToMainMenu();
            return;
        }
        if (keySym == 13 || keySym == 10 || keySym == 32) { // Enter o Espacio -> Reintentar Génesis con nueva semilla
            prepareForWorldCreation(true);
            return;
        }
        return;
    }

    if (m_gameState == GameState::WorldCreation) {
        if (keySym == 27) { // ESC -> Salir al menú
            returnToMainMenu();
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
        if (m_activeInputField == 516) { // Cantidad Inicial de Recursos
            if (keySym == 8) {
                m_creationConfig.initialResources /= 10;
            } else if (keySym >= '0' && keySym <= '9') {
                int digit = keySym - '0';
                if (m_creationConfig.initialResources < 100000) {
                    m_creationConfig.initialResources = m_creationConfig.initialResources * 10 + digit;
                }
            }
            return;
        }
        return;
    }

    if (m_gameState == GameState::LoadWorldMenu) {
        if (keySym == 27) { // ESC -> Volver al menú
            returnToMainMenu();
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

    if (m_gameState == GameState::MapEditor) {
        // EN: TAB or V key switches between 2D Cartographic and 2.5D Isometric view
        // ES: Tecla TAB o V alterna entre Vista Cartográfica 2D e Isométrica 2.5D
        if (keySym == 9 || keySym == 65289 || keySym == 'v' || keySym == 'V') {
            if (m_mapEditorConfig.viewMode == MapEditorViewMode::Cartographic2D) {
                m_mapEditorConfig.viewMode = MapEditorViewMode::Isometric25D;
                m_camera.reset(m_editorMap.getWidth(), m_editorMap.getHeight());
                m_camera.centerOnCell(m_editorMap.getWidth() / 2, m_editorMap.getHeight() / 2,
                                      m_editorMap.getWidth(), m_editorMap.getHeight());
                m_camera.setZoom(1.0f);
            } else {
                m_mapEditorConfig.viewMode = MapEditorViewMode::Cartographic2D;
            }
            m_soundSystem.playSFX(SFXType::UIClick);
            return;
        }

        if (keySym == 27) { // ESC
            if (m_mapEditorConfig.viewMode == MapEditorViewMode::Isometric25D) {
                m_mapEditorConfig.viewMode = MapEditorViewMode::Cartographic2D;
                m_soundSystem.playSFX(SFXType::UIClick);
            } else {
                returnToMainMenu();
            }
            return;
        }

        if (m_activeInputField == 807) { // Semilla del mapa
            if (keySym == 8) { // Backspace
                m_mapEditorConfig.mapSeed /= 10;
            } else if (keySym >= '0' && keySym <= '9') {
                m_mapEditorConfig.mapSeed = m_mapEditorConfig.mapSeed * 10 + (keySym - '0');
            } else if (keySym == 13 || keySym == 10) { // Enter para aplicar y regenerar
                m_activeInputField = 0;
                regenerateEditorMap();
            }
            return;
        }

        if (m_mapEditorConfig.viewMode == MapEditorViewMode::Isometric25D) {
            // EN: Free camera navigation with WASD / Arrow keys
            // ES: Navegación de cámara libre con WASD o flechas
            constexpr float panStep = 50.0f;
            if (keySym == 'w' || keySym == 'W' || keySym == 65362) {
                m_camera.pan(0.0f, -panStep);
                return;
            } else if (keySym == 's' || keySym == 'S' || keySym == 65364) {
                m_camera.pan(0.0f, panStep);
                return;
            } else if (keySym == 'a' || keySym == 'A' || keySym == 65361) {
                m_camera.pan(-panStep, 0.0f);
                return;
            } else if (keySym == 'd' || keySym == 'D' || keySym == 65363) {
                m_camera.pan(panStep, 0.0f);
                return;
            } else if (keySym == 'r' || keySym == 'R') {
                m_camera.centerOnCell(m_editorMap.getWidth() / 2, m_editorMap.getHeight() / 2,
                                      m_editorMap.getWidth(), m_editorMap.getHeight());
                return;
            } else if (keySym == '+' || keySym == '=') {
                m_camera.zoomBy(1.20f, m_mouseX, m_mouseY);
                return;
            } else if (keySym == '-' || keySym == '_') {
                m_camera.zoomBy(0.80f, m_mouseX, m_mouseY);
                return;
            }
        }

        if (keySym == 13 || keySym == 10) { // Enter -> Jugar este mapa
            startPlayWithEditorMap();
            return;
        }
        return;
    }

    if (m_gameState == GameState::Tutorial) {
        // EN: Escape returns cleanly to the main menu
        // ES: Escape regresa limpiamente al menú principal
        if (keySym == 27) {
            returnToMainMenu();
            return;
        }
        // EN: Arrow keys and A/D switch between tabs
        // ES: Teclas de flecha y A/D cambian entre pestañas
        if (keySym == 65361 || keySym == 'a' || keySym == 'A') { // Flecha Izquierda / A -> Pestaña anterior
            const int currentTabIndex = static_cast<int>(m_currentTutorialTab);
            if (currentTabIndex > 0) {
                m_currentTutorialTab = static_cast<TutorialTab>(currentTabIndex - 1);
                m_soundSystem.playSFX(SFXType::UIClick);
            }
            return;
        }
        if (keySym == 65363 || keySym == 'd' || keySym == 'D') { // Flecha Derecha / D -> Pestaña siguiente
            const int currentTabIndex = static_cast<int>(m_currentTutorialTab);
            if (currentTabIndex + 1 < static_cast<int>(TutorialTab::Count)) {
                m_currentTutorialTab = static_cast<TutorialTab>(currentTabIndex + 1);
                m_soundSystem.playSFX(SFXType::UIClick);
            }
            return;
        }
        // EN: Direct number keys 1-4 jump to corresponding category tab
        // ES: Teclas numéricas directas 1-4 saltan a la pestaña de categoría correspondiente
        if (keySym >= '1' && keySym <= '4') {
            m_currentTutorialTab = static_cast<TutorialTab>(keySym - '1');
            m_soundSystem.playSFX(SFXType::UIClick);
            return;
        }
        return;
    }

    if (m_gameState == GameState::Options) {
        if (keySym == 27) { // ESC -> Guardar opciones y volver
            saveSettings();
            m_gameState = m_previousGameState;
            return;
        }
    } else if (m_gameState == GameState::MainMenu) {
        if (keySym == 27) { // ESC -> Salir
            m_isRunning = false;
            return;
        }
    }

    // Si el modal de la crónica de eventos avanzados está abierto
    if (m_gameState == GameState::Playing && m_renderer.isChronicleOpen()) {
        if (keySym == 27 || keySym == 'e' || keySym == 'E' || keySym == 'c' || keySym == 'C') { // ESC, E o C -> Cerrar
            m_renderer.setChronicleOpen(false);
            return;
        }
        if (keySym == 65361) { // Left Arrow
            m_renderer.scrollChronicle(-1);
            return;
        }
        if (keySym == 65363) { // Right Arrow
            m_renderer.scrollChronicle(1);
            return;
        }
        return;
    }

    // Si el modal del árbol de avances está abierto
    if (m_gameState == GameState::Playing && m_renderer.isFaithTreeOpen()) {
        if (keySym == 27 || keySym == 't' || keySym == 'T' || keySym == 'a' || keySym == 'A') { // ESC, T o A -> Cerrar
            m_renderer.setFaithTreeOpen(false);
            return;
        }
        return;
    }

    // Si el modal de moral religiosa y mandatos está abierto
    if (m_gameState == GameState::Playing && m_renderer.isMoralRulesModalOpen()) {
        if (keySym == 27 || keySym == 'm' || keySym == 'M') { // ESC o M -> Cerrar
            m_renderer.setMoralRulesModalOpen(false);
            return;
        }
        return;
    }

    // Si el modal del árbol de recuerdos (Sísifo & Némesis) está abierto
    if (m_gameState == GameState::Playing && m_renderer.isMemoryTreeOpen()) {
        if (keySym == 27 || keySym == 'n' || keySym == 'N') { // ESC o N -> Cerrar
            m_renderer.setMemoryTreeOpen(false);
            return;
        }
        if (keySym == 65362 || keySym == 'w' || keySym == 'W') { // Flecha arriba o W
            m_renderer.scrollMemoryTree(-1);
            return;
        }
        if (keySym == 65364 || keySym == 's' || keySym == 'S') { // Flecha abajo o S
            m_renderer.scrollMemoryTree(1);
            return;
        }
        return;
    }

    // Tecla 'E' o 'C' para abrir la crónica sagrada de eventos
    if (m_gameState == GameState::Playing && !m_commandBoxOpen && (keySym == 'e' || keySym == 'E' || keySym == 'c' || keySym == 'C')) {
        m_renderer.toggleChronicle();
        if (m_renderer.isChronicleOpen()) {
            m_renderer.setFaithTreeOpen(false);
            m_renderer.setMemoryTreeOpen(false);
        }
        return;
    }

    // Tecla 'T' o 'A' para abrir el árbol de avances de la fe
    if (m_gameState == GameState::Playing && !m_commandBoxOpen && (keySym == 't' || keySym == 'T' || keySym == 'a' || keySym == 'A')) {
        m_renderer.toggleFaithTree();
        if (m_renderer.isFaithTreeOpen()) {
            m_renderer.setChronicleOpen(false);
            m_renderer.setMemoryTreeOpen(false);
        }
        return;
    }

    // Tecla 'N' para abrir el árbol de recuerdos y mitos (Némesis & Sísifo)
    if (m_gameState == GameState::Playing && !m_commandBoxOpen && (keySym == 'n' || keySym == 'N')) {
        m_renderer.toggleMemoryTree();
        if (m_renderer.isMemoryTreeOpen()) {
            m_renderer.setChronicleOpen(false);
            m_renderer.setFaithTreeOpen(false);
            m_renderer.setMoralRulesModalOpen(false);
        }
        return;
    }

    // Tecla 'M' para abrir el modal de Mandatos y Moral Religiosa
    if (m_gameState == GameState::Playing && !m_commandBoxOpen && (keySym == 'm' || keySym == 'M')) {
        m_renderer.toggleMoralRulesModal();
        if (m_renderer.isMoralRulesModalOpen()) {
            m_renderer.setChronicleOpen(false);
            m_renderer.setFaithTreeOpen(false);
            m_renderer.setMemoryTreeOpen(false);
        }
        return;
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
            m_renderer.triggerButtonPress(201);
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
        case 'z': case 'Z':
            toggleDivinePowerTargeting(DivinePowerType::Lightning);
            break;
        case 'x': case 'X':
            toggleDivinePowerTargeting(DivinePowerType::Plague);
            break;
        case 'v': case 'V':
            toggleDivinePowerTargeting(DivinePowerType::Miracle);
            break;
        case 'c': case 'C':
            castDivinePower(DivinePowerType::Congregate, 0, 0);
            break;
        case 't': case 'T':
            toggleDivinePowerTargeting(DivinePowerType::Tornado);
            break;
        case 'g': case 'G':
            toggleDivinePowerTargeting(DivinePowerType::Earthquake);
            break;
        case 'b': case 'B':
            toggleDivinePowerTargeting(DivinePowerType::Pandemic);
            break;
        case 'm': case 'M':
            toggleDivinePowerTargeting(DivinePowerType::Meteor);
            break;
        case 'k': case 'K':
            toggleDivinePowerTargeting(DivinePowerType::Volcano);
            break;
        case 'f': case 'F':
            toggleDivinePowerTargeting(DivinePowerType::Wildfire);
            break;
        case 'l': case 'L':
            toggleDivinePowerTargeting(DivinePowerType::Rain);
            break;
        case 'j': case 'J':
            toggleDivinePowerTargeting(DivinePowerType::Deluge);
            break;
        case 'o': case 'O':
            // EN: Hotkey 'O' toggles Atomic Bomb cheat targeting mode / ES: Tecla 'O' activa modo apuntar del truco Bomba Atómica
            toggleDivinePowerTargeting(DivinePowerType::AtomicBomb);
            break;
        case 'u': case 'U':
            m_soundSystem.toggleMute();
            showFeedback(m_soundSystem.isMuted() ? "AUDIO: SILENCIADO [Mute]" : "AUDIO: ACTIVADO", 2.5f);
            break;
        case 'h': case 'H':
            // EN: Hotkey 'H' toggles terrain isometric grid overlay
            // ES: Tecla 'H' conmuta la capa de retícula isométrica de terreno
            m_settings.toggleIsometricGrid();
            applySettings(m_settings);
            showFeedback(m_renderer.isGridVisible() ? "RETICULA DE TERRENO: VISIBLE [H]" : "RETICULA DE TERRENO: OCULTA [H]", 2.5f);
            break;
        case '[': {
            const float newVolume = std::max(0.0f, m_soundSystem.getMasterVolume() - 0.10f);
            m_soundSystem.setMasterVolume(newVolume);
            showFeedback("VOLUMEN: " + std::to_string(static_cast<int>(newVolume * 100)) + "%", 2.0f);
            break;
        }
        case ']': {
            const float newVolume = std::min(1.0f, m_soundSystem.getMasterVolume() + 0.10f);
            m_soundSystem.setMasterVolume(newVolume);
            showFeedback("VOLUMEN: " + std::to_string(static_cast<int>(newVolume * 100)) + "%", 2.0f);
            break;
        }
        case 27: // ESC
            if (m_activeDivinePower != DivinePowerType::Inactive) {
                m_activeDivinePower = DivinePowerType::Inactive;
                m_renderer.setActiveDivinePower(DivinePowerType::Inactive);
                showFeedback("Modo apuntar cancelado.", 2.0f);
                break;
            }
            saveCurrentGame();
            returnToMainMenu();
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

    if (cmd == "STATUS" || cmd == "DIAG" || cmd == "SALUD") {
        const auto healthMetrics = ErrorSystem::instance().getHealthMetrics();
        char healthStatusBuffer[128];
        std::snprintf(healthStatusBuffer, sizeof(healthStatusBuffer),
                      "SALUD SISTEMA: %.1f FPS (%.1fms) | RAM: %.1f MB | ENTIDADES: %u/%u | ESTADO: %s",
                      healthMetrics.currentFps, healthMetrics.currentFrameTimeMs,
                      healthMetrics.currentMemoryMb, healthMetrics.activeEntityCount,
                      healthMetrics.maxSafeEntityLimit,
                      healthMetrics.isUnderLagStress ? "LAG STRESS" : "OPTIMO");
        showFeedback(healthStatusBuffer, 6.0f);
    } else if (cmd == "PURGE" || cmd == "LIMPIAR") {
        performEmergencyMemoryPurge();
        showFeedback("PURGA DE MEMORIA EJECUTADA: CACHES Y CADAVERES COMPACTADOS CON EXITO", 5.0f);
    } else if (cmd == "SELECTOR") {
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
    } else if (cmd == "DIA" || cmd == "DAY" || cmd == "MEDIODIA") {
        m_dayNightCycle.setTimeByHour(12.0f);
        showFeedback("CICLO DIURNO: FIJADO A MEDIODIA (12:00) - PLENA ILUMINACION SOLAR", 4.0f);
    } else if (cmd == "NOCHE" || cmd == "NIGHT") {
        m_dayNightCycle.setTimeByHour(23.0f);
        showFeedback("CICLO DIURNO: FIJADO A NOCHE (23:00) - LUNA Y LUCES DINAMICAS", 4.0f);
    } else if (cmd == "AMANECER" || cmd == "DAWN") {
        m_dayNightCycle.setTimeByHour(6.5f);
        showFeedback("CICLO DIURNO: FIJADO A AMANECER (06:30) - AURORA Y RESPLANDOR DORADO", 4.0f);
    } else if (cmd == "ATARDECER" || cmd == "DUSK" || cmd == "OCASO") {
        m_dayNightCycle.setTimeByHour(18.5f);
        showFeedback("CICLO DIURNO: FIJADO A ATARDECER (18:30) - CREPUSCULO Y HOGUERAS", 4.0f);
    } else if (cmd == "METEORITO" || cmd == "METEORO" || cmd == "METEOR") {
        toggleDivinePowerTargeting(DivinePowerType::Meteor);
        showFeedback("MODO APUNTAR: METEORITO CATACLYSMICO ACTIVADO", 4.0f);
    } else if (cmd == "VOLCAN" || cmd == "VOLCANO" || cmd == "ERUPCION" || cmd == "MAGMA") {
        toggleDivinePowerTargeting(DivinePowerType::Volcano);
        showFeedback("MODO APUNTAR: ERUPCIÓN VOLCÁNICA ACTIVADA", 4.0f);
    } else if (cmd == "INCENDIO" || cmd == "FUEGO" || cmd == "WILDFIRE" || cmd == "FIRE") {
        toggleDivinePowerTargeting(DivinePowerType::Wildfire);
        showFeedback("MODO APUNTAR: INCENDIO FORESTAL ACTIVADO", 4.0f);
    } else if (cmd == "LLUVIA" || cmd == "RAIN") {
        toggleDivinePowerTargeting(DivinePowerType::Rain);
        showFeedback("MODO APUNTAR: LLUVIA BENDITA ACTIVADA", 4.0f);
    } else if (cmd == "DILUVIO" || cmd == "DELUGE" || cmd == "FLOOD" || cmd == "INUNDACION") {
        toggleDivinePowerTargeting(DivinePowerType::Deluge);
        showFeedback("MODO APUNTAR: GRAN DILUVIO CATACLÍSMICO ACTIVADO", 4.0f);
    } else if (cmd == "FUCKINGBOMB" || cmd == "ATOMICBOMB" || cmd == "NUKE" || cmd == "BOMBA" || cmd == "ATOMICA") {
        // EN: Secret apocalyptic nuclear cheat command / ES: Comando truco apocalíptico nuclear secreto
        toggleDivinePowerTargeting(DivinePowerType::AtomicBomb);
        showFeedback("¡TRUCO ACTIVADO! MODO APUNTAR: EXPLOSION ATOMICA [FUCKINGBOMB] (0 Fe)", 5.0f);
    } else if (cmd == "MUTE" || cmd == "SILENCIO") {
        m_soundSystem.toggleMute();
        showFeedback(m_soundSystem.isMuted() ? "AUDIO: SILENCIADO [Mute]" : "AUDIO: ACTIVADO", 4.0f);
    } else if (cmd == "GRID" || cmd == "RETICULA" || cmd == "CUADRICULA") {
        // EN: Toggle terrain isometric grid visualization command
        // ES: Comando para alternar la visualización de la retícula isométrica de terreno
        m_settings.toggleIsometricGrid();
        applySettings(m_settings);
        showFeedback(m_renderer.isGridVisible()
                         ? "RETICULA DE TERRENO: VISIBLE [H]"
                         : "RETICULA DE TERRENO: OCULTA [H]",
                     4.0f);
    } else if (cmd == "DEV" || cmd == "DEVELOPER" || cmd == "DEBUG" || cmd == "DESARROLLADOR") {
        // EN: Toggle developer mode telemetry and axes overlay
        // ES: Comando para alternar telemetría y ejes del modo desarrollador
        toggleDeveloperMode();
    } else if (cmd == "SONIDO" || cmd == "AUDIO") {
        showFeedback("DISPOSITIVO DE SONIDO: " + m_soundSystem.getDeviceName() + " | VOL: " +
                     std::to_string(static_cast<int>(m_soundSystem.getMasterVolume() * 100)) + "%", 4.0f);
    } else if (cmd == "BORRACHO" || cmd == "DRUNK") {
        // EN: Drunk villager condition cheat command
        // ES: Comando truco de condición aldeano borracho
        Humano* selectedVillager = m_gestorHumanos.getHumanoSeleccionadoMutable();
        if (selectedVillager && selectedVillager->estaVivo()) {
            selectedVillager->resetConditions();
            selectedVillager->consumeAlcohol(80.0f);
            showFeedback("ESTADO MODIFICADO: " + selectedVillager->getNombre() + " -> [BORRACHO] (Alcohol 80%)", 4.0f);
        } else {
            for (auto& villager : m_gestorHumanos.getHumanos()) {
                if (villager.estaVivo() && villager.getReligionId() == 1) {
                    villager.resetConditions();
                    villager.consumeAlcohol(80.0f);
                }
            }
            showFeedback("ESTADO MODIFICADO: Aldeanos de la tribu -> [BORRACHO] (Alcohol 80%)", 4.0f);
        }
    } else if (cmd == "HAMBRIENTO" || cmd == "HUNGRY") {
        // EN: Hungry villager condition cheat command
        // ES: Comando truco de condición aldeano hambriento
        Humano* selectedVillager = m_gestorHumanos.getHumanoSeleccionadoMutable();
        if (selectedVillager && selectedVillager->estaVivo()) {
            selectedVillager->resetConditions();
            selectedVillager->getHumanUnitMutable().nutritionalState = 12.0f;
            selectedVillager->getPopGroupMutable().foodSupply = 10.0f;
            showFeedback("ESTADO MODIFICADO: " + selectedVillager->getNombre() + " -> [HAMBRIENTO]", 4.0f);
        } else {
            for (auto& villager : m_gestorHumanos.getHumanos()) {
                if (villager.estaVivo() && villager.getReligionId() == 1) {
                    villager.resetConditions();
                    villager.getHumanUnitMutable().nutritionalState = 12.0f;
                    villager.getPopGroupMutable().foodSupply = 10.0f;
                }
            }
            showFeedback("ESTADO MODIFICADO: Aldeanos de la tribu -> [HAMBRIENTO]", 4.0f);
        }
    } else if (cmd == "ENFERMO" || cmd == "SICK") {
        // EN: Sick villager condition cheat command
        // ES: Comando truco de condición aldeano enfermo
        Humano* selectedVillager = m_gestorHumanos.getHumanoSeleccionadoMutable();
        if (selectedVillager && selectedVillager->estaVivo()) {
            selectedVillager->resetConditions();
            selectedVillager->infectWithPandemic(30.0f);
            showFeedback("ESTADO MODIFICADO: " + selectedVillager->getNombre() + " -> [ENFERMO]", 4.0f);
        } else {
            for (auto& villager : m_gestorHumanos.getHumanos()) {
                if (villager.estaVivo() && villager.getReligionId() == 1) {
                    villager.resetConditions();
                    villager.infectWithPandemic(30.0f);
                }
            }
            showFeedback("ESTADO MODIFICADO: Aldeanos de la tribu -> [ENFERMO]", 4.0f);
        }
    } else if (cmd == "LLENO" || cmd == "FULL" || cmd == "SACIADO") {
        // EN: Full villager condition cheat command
        // ES: Comando truco de condición aldeano lleno/saciado
        Humano* selectedVillager = m_gestorHumanos.getHumanoSeleccionadoMutable();
        if (selectedVillager && selectedVillager->estaVivo()) {
            selectedVillager->resetConditions();
            selectedVillager->getHumanUnitMutable().nutritionalState = 100.0f;
            selectedVillager->getPopGroupMutable().foodSupply = 100.0f;
            selectedVillager->setEnergia(100.0f);
            showFeedback("ESTADO MODIFICADO: " + selectedVillager->getNombre() + " -> [LLENO]", 4.0f);
        } else {
            for (auto& villager : m_gestorHumanos.getHumanos()) {
                if (villager.estaVivo() && villager.getReligionId() == 1) {
                    villager.resetConditions();
                    villager.getHumanUnitMutable().nutritionalState = 100.0f;
                    villager.getPopGroupMutable().foodSupply = 100.0f;
                    villager.setEnergia(100.0f);
                }
            }
            showFeedback("ESTADO MODIFICADO: Aldeanos de la tribu -> [LLENO]", 4.0f);
        }
    } else if (cmd == "AGRESIVO" || cmd == "AGGRESSIVE" || cmd == "FURIA") {
        // EN: Aggressive villager condition cheat command
        // ES: Comando truco de condición aldeano agresivo
        Humano* selectedVillager = m_gestorHumanos.getHumanoSeleccionadoMutable();
        if (selectedVillager && selectedVillager->estaVivo()) {
            selectedVillager->resetConditions();
            selectedVillager->provokeAggression(90.0f);
            selectedVillager->activarFuriaProtectora(15.0f);
            showFeedback("ESTADO MODIFICADO: " + selectedVillager->getNombre() + " -> [AGRESIVO] (+35% Danio)", 4.0f);
        } else {
            for (auto& villager : m_gestorHumanos.getHumanos()) {
                if (villager.estaVivo() && villager.getReligionId() == 1) {
                    villager.resetConditions();
                    villager.provokeAggression(90.0f);
                    villager.activarFuriaProtectora(15.0f);
                }
            }
            showFeedback("ESTADO MODIFICADO: Aldeanos de la tribu -> [AGRESIVO] (+35% Danio)", 4.0f);
        }
    } else if (cmd == "ASUSTADO" || cmd == "SCARED" || cmd == "PANICO") {
        // EN: Scared villager condition cheat command
        // ES: Comando truco de condición aldeano asustado
        Humano* selectedVillager = m_gestorHumanos.getHumanoSeleccionadoMutable();
        if (selectedVillager && selectedVillager->estaVivo()) {
            selectedVillager->resetConditions();
            selectedVillager->huirEnPanicoDesde(selectedVillager->getX() + 0.5f, selectedVillager->getY() + 0.5f, 4.0f);
            showFeedback("ESTADO MODIFICADO: " + selectedVillager->getNombre() + " -> [ASUSTADO]", 4.0f);
        } else {
            for (auto& villager : m_gestorHumanos.getHumanos()) {
                if (villager.estaVivo() && villager.getReligionId() == 1) {
                    villager.resetConditions();
                    villager.huirEnPanicoDesde(villager.getX() + 0.5f, villager.getY() + 0.5f, 4.0f);
                }
            }
            showFeedback("ESTADO MODIFICADO: Aldeanos de la tribu -> [ASUSTADO]", 4.0f);
        }
    } else if (cmd == "SANO" || cmd == "HEALTHY" || cmd == "CURAR") {
        // EN: Healthy villager condition cheat command
        // ES: Comando truco de condición aldeano sano
        Humano* selectedVillager = m_gestorHumanos.getHumanoSeleccionadoMutable();
        if (selectedVillager && selectedVillager->estaVivo()) {
            selectedVillager->resetConditions();
            selectedVillager->curar(100.0f);
            selectedVillager->getHumanUnitMutable().nutritionalState = 65.0f;
            selectedVillager->getPopGroupMutable().foodSupply = 65.0f;
            selectedVillager->setEnergia(100.0f);
            showFeedback("ESTADO MODIFICADO: " + selectedVillager->getNombre() + " -> [SANO]", 4.0f);
        } else {
            for (auto& villager : m_gestorHumanos.getHumanos()) {
                if (villager.estaVivo() && villager.getReligionId() == 1) {
                    villager.resetConditions();
                    villager.curar(100.0f);
                    villager.getHumanUnitMutable().nutritionalState = 65.0f;
                    villager.getPopGroupMutable().foodSupply = 65.0f;
                    villager.setEnergia(100.0f);
                }
            }
            showFeedback("ESTADO MODIFICADO: Aldeanos de la tribu -> [SANO]", 4.0f);
        }
    } else if (cmd == "EXHAUSTO" || cmd == "EXHAUSTED") {
        // EN: Exhausted villager condition cheat command
        // ES: Comando truco de condición aldeano exhausto
        Humano* selectedVillager = m_gestorHumanos.getHumanoSeleccionadoMutable();
        if (selectedVillager && selectedVillager->estaVivo()) {
            selectedVillager->resetConditions();
            selectedVillager->setEnergia(5.0f);
            selectedVillager->setSueno(95.0f);
            showFeedback("ESTADO MODIFICADO: " + selectedVillager->getNombre() + " -> [EXHAUSTO]", 4.0f);
        } else {
            for (auto& villager : m_gestorHumanos.getHumanos()) {
                if (villager.estaVivo() && villager.getReligionId() == 1) {
                    villager.resetConditions();
                    villager.setEnergia(5.0f);
                    villager.setSueno(95.0f);
                }
            }
            showFeedback("ESTADO MODIFICADO: Aldeanos de la tribu -> [EXHAUSTO]", 4.0f);
        }
    } else if (cmd == "INSPIRADO" || cmd == "INSPIRED") {
        // EN: Inspired villager condition cheat command
        // ES: Comando truco de condición aldeano inspirado
        Humano* selectedVillager = m_gestorHumanos.getHumanoSeleccionadoMutable();
        if (selectedVillager && selectedVillager->estaVivo()) {
            selectedVillager->resetConditions();
            selectedVillager->activarSenalDivina(25.0f);
            selectedVillager->setDevocion(1.0f);
            showFeedback("ESTADO MODIFICADO: " + selectedVillager->getNombre() + " -> [INSPIRADO]", 4.0f);
        } else {
            for (auto& villager : m_gestorHumanos.getHumanos()) {
                if (villager.estaVivo() && villager.getReligionId() == 1) {
                    villager.resetConditions();
                    villager.activarSenalDivina(25.0f);
                    villager.setDevocion(1.0f);
                }
            }
            showFeedback("ESTADO MODIFICADO: Aldeanos de la tribu -> [INSPIRADO]", 4.0f);
        }
    } else if (cmd == "HERIDO" || cmd == "INJURED") {
        // EN: Injured villager condition cheat command
        // ES: Comando truco de condición aldeano herido
        Humano* selectedVillager = m_gestorHumanos.getHumanoSeleccionadoMutable();
        if (selectedVillager && selectedVillager->estaVivo()) {
            selectedVillager->resetConditions();
            selectedVillager->setSalud(25.0f);
            selectedVillager->getHumanUnitMutable().hitPoints = 25.0f;
            showFeedback("ESTADO MODIFICADO: " + selectedVillager->getNombre() + " -> [HERIDO]", 4.0f);
        } else {
            for (auto& villager : m_gestorHumanos.getHumanos()) {
                if (villager.estaVivo() && villager.getReligionId() == 1) {
                    villager.resetConditions();
                    villager.setSalud(25.0f);
                    villager.getHumanUnitMutable().hitPoints = 25.0f;
                }
            }
            showFeedback("ESTADO MODIFICADO: Aldeanos de la tribu -> [HERIDO]", 4.0f);
        }
    } else {
        showFeedback("COMANDO DESCONOCIDO: '" + cmd + "' (USA: SELECTOR | LUMINUS | DIA | NOCHE | AMANECER | ATARDECER | METEORITO | VOLCAN | FUCKINGBOMB | MUTE | BORRACHO | HAMBRIENTO | ENFERMO | SANO | LLENO | AGRESIVO | ASUSTADO)", 4.0f);
    }
}

void Game::showFeedback(const std::string& msg, float seconds) {
    m_commandFeedback = msg;
    m_commandFeedbackTimer = seconds;
    std::cout << "[Comando] " << msg << std::endl;
}

// EN: Resolve the player religion ID by matching the configured religion name
// ES: Resolver el ID de la religión del jugador buscando el nombre configurado
int Game::resolvePlayerReligionId() const {
    for (const auto& religionEntry : m_registry.getAllReligions()) {
        if (religionEntry.name == m_creationConfig.religionName) {
            return religionEntry.id;
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

    // EN: Search for holy site of player religion if no villagers found
    // ES: Buscar el sitio sagrado de la religión del jugador si no se hallan aldeanos
    for (int cellY = 0; cellY < m_map.getHeight(); ++cellY) {
        for (int cellX = 0; cellX < m_map.getWidth(); ++cellX) {
            const Cell& candidateCell = m_map.getCell(cellX, cellY);
            const int holySiteReligion = (candidateCell.holySiteReligionId > 0) ? candidateCell.holySiteReligionId : candidateCell.dominantReligionId;
            if (candidateCell.hasHolySite && holySiteReligion == rel) {
                m_camera.centerOnCell(cellX, cellY, m_map.getWidth(), m_map.getHeight());
                selectCell(cellX, cellY);
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
                } else if (ks == XK_F11) {
                    onKeyPress(1011);
                } else if (ks == XK_F3) {
                    onKeyPress(1003);
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
                // EN: Intercept window close event (WM_DELETE_WINDOW)
                // ES: Interceptar evento de cierre de ventana (WM_DELETE_WINDOW)
                Atom wmProtocols = XInternAtom(dpy, "WM_PROTOCOLS", False);
                Atom wmDelete = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
                if (ev.xclient.message_type == wmProtocols && static_cast<Atom>(ev.xclient.data.l[0]) == wmDelete) {
                    m_isRunning = false;
                }
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
    uint32_t frameCounter = 0;

    while (m_isRunning) {
        auto now = std::chrono::high_resolution_clock::now();
        float rawDt = std::chrono::duration<float>(now - lastTime).count();
        lastTime = now;

        // EN: Clamp delta time through ErrorSystem to strictly prevent the simulation spiral of death
        // ES: Limitar el delta time mediante ErrorSystem para evitar la espiral de la muerte
        float dt = ErrorSystem::instance().clampDeltaTime(rawDt);

        handleWindowEvents();
        update(dt);
        render();

        auto frameEndTime = std::chrono::high_resolution_clock::now();
        float frameDurationMs = std::chrono::duration<float, std::milli>(frameEndTime - now).count();

        // EN: Report frame timing to the lag detector
        // ES: Informar el tiempo de fotograma al detector de lag
        ErrorSystem::instance().reportFrameTime(frameDurationMs);

        // EN: Periodic memory auditing and leak prevention (every 60 frames)
        // ES: Auditoría periódica de memoria y prevención de fugas (cada 60 fotogramas)
        if (++frameCounter % 60 == 0) {
            ErrorSystem::instance().auditMemoryUsage();
            if (ErrorSystem::instance().shouldTriggerEmergencyPurge()) {
                performEmergencyMemoryPurge();
                ErrorSystem::instance().acknowledgeEmergencyPurge();
            }
        }

        auto frameElapsed = std::chrono::duration_cast<std::chrono::microseconds>(frameEndTime - now);
        constexpr auto targetFrameDuration = std::chrono::microseconds(16666); // 60 FPS
        if (frameElapsed < targetFrameDuration) {
            std::this_thread::sleep_for(targetFrameDuration - frameElapsed);
        }
    }
}

// =============================================================================
// EN: Emergency memory purge - trims bloated caches when RAM reaches critical levels
// ES: Purga de emergencia de memoria - poda cachés infladas si la RAM llega a niveles críticos
// =============================================================================
void Game::performEmergencyMemoryPurge() {
    std::cout << "[Game:EmergencyPurge] Executing emergency memory compacting..." << std::endl;

    // EN: 1. Trim historical timeline snapshots to essential baseline
    // ES: 1. Podar las capturas históricas de la línea de tiempo a un mínimo esencial
    m_timeline.trimHistory(100);

    // EN: 2. Prune old deceased entities whose decay timers have elapsed
    // ES: 2. Purgar entidades fallecidas cuyos temporizadores de muerte hayan concluido
    auto& villagers = m_gestorHumanos.getHumanos();
    villagers.erase(
        std::remove_if(villagers.begin(), villagers.end(), [](const Humano& villager) {
            return villager.getSalud() <= 0.0f;
        }),
        villagers.end()
    );

    // EN: 3. Audit updated entity count in ErrorSystem
    // ES: 3. Auditar el conteo de entidades actualizado en ErrorSystem
    ErrorSystem::instance().updateEntityCount(static_cast<uint32_t>(villagers.size()));

    // EN: 4. Re-audit memory usage post-purge
    // ES: 4. Reauditar el consumo de memoria tras la purga
    float postPurgeMemoryMb = ErrorSystem::instance().auditMemoryUsage();
    std::cout << "[Game:EmergencyPurge] Purge complete. Resident RAM: "
              << postPurgeMemoryMb << " MB." << std::endl;
}

// =============================================================================
// EN: Setup living simulated world panorama for the Minecraft-inspired main menu
// ES: Configurar panorama de mundo simulado vivo para la pantalla principal inspirada en Minecraft
// =============================================================================
void Game::setupMenuPanoramaWorld() {
    constexpr int panoramaMapDimension = 64;
    constexpr uint32_t panoramaScenicSeed = 3477892567U; // Semilla escénica con valles fértiles, ríos y costas

    m_map = WorldMap(panoramaMapDimension, panoramaMapDimension);
    m_map.generateProceduralWorld(panoramaScenicSeed, WorldClimate::Templado, ResourceAbundance::Abundante);
    m_cloudSystem.reset(panoramaMapDimension, panoramaMapDimension);
    m_renderer.setCloudSystem(&m_cloudSystem);

    m_registry = ReligionRegistry{};
    m_timeline.reset(-4000, 2200);
    m_dayNightCycle.resetDaysPassed();
    m_timeline.setYearsPerSecond(4.0f);
    m_timeEngine.reset(-4000);

    // EN: Seed diverse religions across the panorama world
    // ES: Sembrar diversas religiones en el mundo del panorama
    m_registry.createPrimordialReligion("Culto del Fuego Solar", "Solarius", ColorRGB{230, 161, 59}, "Sun", -4000);
    m_registry.createPrimordialReligion("Animist Gaia-Cult", "Gaia", ColorRGB{46, 204, 113}, "Tree", -4000);
    m_registry.createPrimordialReligion("The Sacred Monad", "Monas", ColorRGB{52, 152, 219}, "Spiral", -4000);
    m_registry.createPrimordialReligion("Order of the Eternal Flame", "Ignis", ColorRGB{128, 0, 32}, "Flame", -4000);
    m_registry.createPrimordialReligion("Wheel of the Star-Dharma", "Dharma", ColorRGB{155, 89, 182}, "Wheel", -4000);

    // EN: Generate pueblos, holy shrines and tribal shelters
    // ES: Generar pueblos, santuarios sagrados y refugios tribales
    std::vector<Point2i> puebloLocations = m_map.generatePuebloLocations(5, panoramaScenicSeed);

    if (!puebloLocations.empty()) {
        const Point2i& centerPueblo = puebloLocations[0];
        m_map.consecrateHolySite(centerPueblo.x, centerPueblo.y, "Gran Templo de Solarius");
        if (m_map.isValid(centerPueblo.x, centerPueblo.y)) {
            m_map.getCell(centerPueblo.x, centerPueblo.y).shrineLevel = 3;
            m_map.getCell(centerPueblo.x, centerPueblo.y).setShare(1, 0.95f);
        }
        for (size_t i = 1; i < puebloLocations.size(); ++i) {
            const Point2i& p = puebloLocations[i];
            m_map.consecrateHolySite(p.x, p.y, "Santuario Ancestral");
            if (m_map.isValid(p.x, p.y)) {
                m_map.getCell(p.x, p.y).shrineLevel = 2;
                m_map.getCell(p.x, p.y).setShare(static_cast<int>(i + 1), 0.90f);
            }
        }

        // EN: Construct tribal huts in valid non-water neighboring plots around center pueblo
        // ES: Construir chozas tribales en parcelas vecinas no acuáticas válidas alrededor del pueblo central
        int housesBuilt = 0;
        for (const auto& neighborPoint : m_map.getNeighbors(centerPueblo.x, centerPueblo.y, true)) {
            if (housesBuilt >= 3) break;
            if (!m_map.isValid(neighborPoint.x, neighborPoint.y)) continue;
            const Cell& neighborCell = m_map.getCell(neighborPoint.x, neighborPoint.y);
            if (neighborCell.isWater() || neighborCell.hasHouse || neighborCell.hasHolySite) continue;

            const HouseType houseType = (housesBuilt == 2) ? HouseType::ChieftainLonghouse : HouseType::ThatchedHut;
            const int houseId = m_buildingSystem.initiateHouseConstruction(neighborPoint.x, neighborPoint.y, 0, 0, 1, houseType, m_map);
            if (House* targetHouse = m_buildingSystem.getHouseByIdMutable(houseId)) {
                targetHouse->buildProgress = 100.0f;
                ++housesBuilt;
            }
        }
    }

    // EN: Populate the world with living autonomous villagers
    // ES: Poblar el mundo con aldeanos autónomos vivos
    m_gestorHumanos.getHumanos().clear();
    m_gestorHumanos.inicializarPoblacion(m_map, m_registry, 36);

    // EN: Set bright golden dawn/morning atmospheric lighting
    // ES: Establecer iluminación atmosférica dorada de amanecer/mañana
    m_dayNightCycle.setTimeByHour(8.5f);
    m_dayNightCycle.setPaused(false);

    // EN: Position camera centered on the village with cinematic zoom
    // ES: Posicionar cámara centrada en la aldea con zoom cinemático
    if (!puebloLocations.empty()) {
        m_camera.centerOnCell(puebloLocations[0].x, puebloLocations[0].y, panoramaMapDimension, panoramaMapDimension);
    } else {
        m_camera.centerOnCell(panoramaMapDimension / 2, panoramaMapDimension / 2, panoramaMapDimension, panoramaMapDimension);
    }
    m_camera.setZoom(1.15f);

    m_menuPanoramaTime = 0.0f;
    m_menuPanoramaInitialized = true;
}

// =============================================================================
// EN: Convert map size configuration index into grid width and height (starts at 64x64)
// ES: Convertir índice de configuración de tamaño de mapa a ancho y alto de cuadrícula (inicia en 64x64)
// =============================================================================
int Game::getMapDimensionFromIndex(int index) {
    switch (index) {
        case 0: return 64;   // Pequeño (64x64, 320m x 320m)
        case 1: return 80;   // Mediano (80x80, 400m x 400m)
        case 2: return 96;   // Grande (96x96, 480m x 480m)
        case 3: return 128;  // Enorme (128x128, 640m x 640m)
        case 4: return 160;  // Inmenso (160x160, 800m x 800m)
        case 5: return 200;  // Colosal (200x200, 1000m x 1000m / 1.0 km²)
        case 6: return 256;  // Titánico (256x256, 1280m x 1280m / 1.28 km)
        case 7: return 320;  // Planetario (320x320, 1600m x 1600m / 1.6 km)
        default: return 128; // Enorme por defecto (128x128)
    }
}

// =============================================================================
// EN: Generate a high-entropy seed using random_device and high-resolution timer
// ES: Generar una semilla de alta entropía usando random_device y temporizador de alta resolución
// =============================================================================
uint32_t Game::generateUniqueSeed() {
    std::random_device randomDevice;
    uint32_t seedValue = randomDevice();
    auto timestamp = static_cast<uint32_t>(std::chrono::high_resolution_clock::now().time_since_epoch().count());
    seedValue ^= (timestamp + 0x9e3779b9 + (seedValue << 6) + (seedValue >> 2));
    if (seedValue == 0) seedValue = 123456789U;
    return seedValue;
}

// =============================================================================
// EN: Reset all game subsystems to eliminate cross-session data leakage
// ES: Reiniciar todos los subsistemas del juego para eliminar fugas de datos entre partidas
// =============================================================================
void Game::resetAllGameSubsystems() {
    m_gestorHumanos.getHumanos().clear();
    m_buildingSystem.clear();
    m_eventLine.clear();
    m_divinePowers.clearAllFX();
    m_moralitySystem.clear();
    m_nemesisSififus.clear();
    m_activeDivinePower = DivinePowerType::Inactive;
    m_renderer.setActiveDivinePower(DivinePowerType::Inactive);

    m_gameOverTriggered = false;
    m_gameOverTimer = 0.0f;
    m_chronicleOpen = false;
    m_commandBoxOpen = false;
    m_commandInput.clear();
    m_commandFeedback.clear();
    m_commandFeedbackTimer = 0.0f;
    m_cheatSelectorActive = false;
    m_cheatLuminusActive = false;
}

// =============================================================================
// EN: Prepare game state for custom world creation with a fresh unique seed
// ES: Preparar el estado de juego para crear un mundo personalizado con semilla única fresca
// =============================================================================
void Game::prepareForWorldCreation(bool generateNewSeed) {
    if (generateNewSeed) {
        m_creationConfig.mapSeed = generateUniqueSeed();
    }
    m_isSpectatingPostDefeat = false;
    m_spectatorRevealAll = true;
    m_activeInputField = 0;
    m_gameState = GameState::WorldCreation;
}

// =============================================================================
// EN: Return to Main Menu with a fresh, living simulated panorama background
// ES: Regresar al Menú Principal con un panorama simulado vivo y fresco de fondo
// =============================================================================
void Game::returnToMainMenu() {
    m_isSpectatingPostDefeat = false;
    m_spectatorRevealAll = true;
    m_gameOverTriggered = false;
    m_gameOverTimer = 0.0f;
    m_activeInputField = 0;
    setupMenuPanoramaWorld();
    m_gameState = GameState::MainMenu;
}

void Game::startNewCustomWorld() {
    // EN: Purge previous subsystems to ensure clean slate without lingering data
    // ES: Purgar subsistemas previos para asegurar un estado limpio sin datos residuales
    resetAllGameSubsystems();

    // EN: Generate unique isolated save slot for this playthrough
    // ES: Generar espacio de guardado aislado único para esta partida
    m_currentSaveSlot = SaveSystem::generateUniqueSlotName(m_creationConfig.godName, m_creationConfig.mapSeed);

    std::cout << "==========================================================" << std::endl;
    std::cout << "[Génesis] Creando nuevo mundo sagrado personalizado..." << std::endl;
    std::cout << "  - Espacio de Partida: " << m_currentSaveSlot << std::endl;
    std::cout << "  - Dios Guía: " << m_creationConfig.godName << std::endl;
    std::cout << "  - Religión Elegida: " << m_creationConfig.religionName << std::endl;
    std::cout << "  - Semilla (Seed): " << m_creationConfig.mapSeed << std::endl;
    std::cout << "  - Población Inicial: " << m_creationConfig.initialPopulation << std::endl;
    std::cout << "  - Recursos Iniciales: " << m_creationConfig.initialResources << " (Comida/Materiales)" << std::endl;
    std::cout << "  - Época de Inicio: Era " << static_cast<int>(m_creationConfig.startingEra) << std::endl;
    std::cout << "  - Cofre Sagrado: " << (m_creationConfig.starterChest ? "ACTIVADO" : "DESACTIVADO") << std::endl;
    std::cout << "==========================================================" << std::endl;

    // 1. Determinar tamaño de mapa y generar geografía procedimental adaptada al Clima y Abundancia elegidos
    const int mapDim = getMapDimensionFromIndex(m_creationConfig.mapSizeIndex);
    m_map = WorldMap(mapDim, mapDim);
    m_map.generateProceduralWorld(m_creationConfig.mapSeed, m_creationConfig.worldClimate, m_creationConfig.resourceAbundance);
    m_cloudSystem.reset(mapDim, mapDim);
    m_renderer.setCloudSystem(&m_cloudSystem);
    m_fog.reset(mapDim, mapDim);
    ErrorSystem::instance().setMaxSafeEntities(std::max(450u, static_cast<uint32_t>(mapDim * 4)));

    // EN: Reset cell populations to 0 so instantiated living units increment counts 1:1
    // ES: Reiniciar la población de celdas a 0 para que cada aldeano real instanciado sume 1:1
    for (int cellY = 0; cellY < mapDim; ++cellY) {
        for (int cellX = 0; cellX < mapDim; ++cellX) {
            Cell& targetCell = m_map.getCell(cellX, cellY);
            targetCell.population = 0;
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
    m_dayNightCycle.resetDaysPassed();
    float yearsPerSec = (m_creationConfig.timePacing == TimePacing::Epico) ? 1.0f : ((m_creationConfig.timePacing == TimePacing::Dinamico) ? 12.0f : 4.0f);
    m_timeline.setYearsPerSecond(yearsPerSec);
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
        pRel->resources.food = m_creationConfig.initialResources;
        pRel->resources.wood = std::max(50, m_creationConfig.initialResources * 6 / 10);
        pRel->resources.stone = std::max(30, m_creationConfig.initialResources * 3 / 10);
        pRel->resources.gold = std::max(10, m_creationConfig.initialResources * 1 / 10);
        if (m_creationConfig.starterChest) {
            pRel->theologyPoints += 1000;
            pRel->spiritualPiety = 0.95f;
            pRel->zeal = 0.85f;
            pRel->resources.food += 200;
            pRel->resources.gold += 50;
        }
    }

    // 4. Crear 4 religiones rivales con equidad estadística (0% trampas ni ventajas numéricas).
    // La dificultad radica estrictamente en la sofisticación de decisiones e inteligencia de la IA.
    const float rivalZeal = 0.5f;

    struct RivalDef { const char* name; const char* founder; ColorRGB color; const char* symbol; };
    RivalDef rivals[4] = {
        {"Animist Gaia-Cult", "Zarah-Khep", ColorRGB(46, 204, 113), "Tree"},
        {"The Sacred Monad", "Melchizedek", ColorRGB(52, 152, 219), "Spiral"},
        {"Order of the Eternal Flame", "Mithras-Gard", ColorRGB(230, 57, 70), "Flame"},
        {"Wheel of the Star-Dharma", "Siddhartha-Yin", ColorRGB(155, 89, 182), "Wheel"}
    };

    int rivalIds[4];
    for (int rivalIndex = 0; rivalIndex < 4; ++rivalIndex) {
        rivalIds[rivalIndex] = m_registry.createPrimordialReligion(
            rivals[rivalIndex].name, rivals[rivalIndex].founder, rivals[rivalIndex].color, rivals[rivalIndex].symbol, startYear
        );
        Religion* rivalRelPtr = m_registry.getReligion(rivalIds[rivalIndex]);
        if (rivalRelPtr) {
            rivalRelPtr->currentEra = m_creationConfig.startingEra;
            rivalRelPtr->zeal = rivalZeal;
            rivalRelPtr->resources.food = m_creationConfig.initialResources;
            rivalRelPtr->resources.wood = std::max(50, m_creationConfig.initialResources * 6 / 10);
            rivalRelPtr->resources.stone = std::max(30, m_creationConfig.initialResources * 3 / 10);
            rivalRelPtr->resources.gold = std::max(10, m_creationConfig.initialResources * 1 / 10);
        }
    }

    // 5. Asignación espacial procedimental de pueblos con dispersión aleatoria y límites de distancia
    // EN: Procedural spatial assignment of settlements with random dispersion and distance margins
    // ES: Asignación espacial procedimental de pueblos con dispersión aleatoria y límites de distancia
    std::vector<Point2i> puebloLocations = m_map.generatePuebloLocations(5, m_creationConfig.mapSeed);
    Point2i playerHearth{mapDim / 2, mapDim / 2};

    if (puebloLocations.size() >= 5) {
        playerHearth = puebloLocations[0];

        // EN: Consecrate the player settlement hearth
        // ES: Consagrar el asentamiento del Jugador
        Cell& playerHearthCell = m_map.getCell(playerHearth.x, playerHearth.y);
        playerHearthCell.setShare(playerRelId, 0.98f);
        playerHearthCell.devotion = 0.95f;
        m_map.consecrateHolySite(playerHearth.x, playerHearth.y, "Santuario Sagrado de " + m_creationConfig.godName);
        playerHearthCell.shrineLevel = m_creationConfig.starterChest ? 3 : 2;

        // EN: Consecrate player local territory (radius 2)
        // ES: Consagrar la comarca local del Jugador (radio 2 de soberanía territorial)
        for (int deltaCellY = -2; deltaCellY <= 2; ++deltaCellY) {
            for (int deltaCellX = -2; deltaCellX <= 2; ++deltaCellX) {
                int neighborCellX = playerHearth.x + deltaCellX;
                int neighborCellY = playerHearth.y + deltaCellY;
                float distance = std::sqrt(static_cast<float>(deltaCellX * deltaCellX + deltaCellY * deltaCellY));
                if (m_map.isValid(neighborCellX, neighborCellY) && distance <= 2.5f && (deltaCellX != 0 || deltaCellY != 0)) {
                    Cell& neighborCell = m_map.getCell(neighborCellX, neighborCellY);
                    if (neighborCell.isHabitable()) {
                        float share = std::max(0.60f, 0.90f - distance * 0.12f);
                        neighborCell.setShare(playerRelId, share);
                        neighborCell.devotion = std::max(0.55f, 0.85f - distance * 0.12f);
                    }
                }
            }
        }

        // EN: Consecrate the 4 rival settlements across their territories
        // ES: Consagrar los 4 pueblos rivales en sus respectivos territorios distantes
        for (int rivalIndex = 0; rivalIndex < 4; ++rivalIndex) {
            Point2i rivalHearth = puebloLocations[rivalIndex + 1];
            Cell& rivalHearthCell = m_map.getCell(rivalHearth.x, rivalHearth.y);
            rivalHearthCell.setShare(rivalIds[rivalIndex], 0.95f);
            rivalHearthCell.devotion = 0.90f;
            m_map.consecrateHolySite(rivalHearth.x, rivalHearth.y, "Templo Arcano de " + std::string(rivals[rivalIndex].founder));
            rivalHearthCell.shrineLevel = 2;

            // EN: Consecrate rival local territory (radius 2)
            // ES: Consagrar la comarca local del rival (radio 2)
            for (int deltaCellY = -2; deltaCellY <= 2; ++deltaCellY) {
                for (int deltaCellX = -2; deltaCellX <= 2; ++deltaCellX) {
                    int neighborCellX = rivalHearth.x + deltaCellX;
                    int neighborCellY = rivalHearth.y + deltaCellY;
                    float distance = std::sqrt(static_cast<float>(deltaCellX * deltaCellX + deltaCellY * deltaCellY));
                    if (m_map.isValid(neighborCellX, neighborCellY) && distance <= 2.5f && (deltaCellX != 0 || deltaCellY != 0)) {
                        Cell& neighborCell = m_map.getCell(neighborCellX, neighborCellY);
                        if (neighborCell.isHabitable()) {
                            float share = std::max(0.60f, 0.90f - distance * 0.12f);
                            neighborCell.setShare(rivalIds[rivalIndex], share);
                            neighborCell.devotion = std::max(0.55f, 0.85f - distance * 0.12f);
                        }
                    }
                }
            }
        }
    }

    m_map.recalculateReligions(m_registry);
    m_camera.reset(m_map.getWidth(), m_map.getHeight());
    m_camera.centerOnCell(playerHearth.x, playerHearth.y, m_map.getWidth(), m_map.getHeight());
    selectCell(playerHearth.x, playerHearth.y);

    // 6. Inicializar población humana de aldeanos sobre el mapa (1 aldeano real = 1 habitante)
    m_gestorHumanos.inicializarPoblacion(m_map, m_registry, m_creationConfig.initialPopulation);
    m_gestorHumanos.setAIDifficulty(m_creationConfig.aiDifficulty);
    m_gestorHumanos.setTribalBelligerence(m_creationConfig.tribalBelligerence);
    m_replication.setAIDifficulty(m_creationConfig.aiDifficulty);
    m_replication.setTribalBelligerence(m_creationConfig.tribalBelligerence);
    m_map.recalculateReligions(m_registry);

    if (const Humano* selChaman = m_gestorHumanos.getHumanoSeleccionado()) {
        selectCell(selChaman->getCeldaX(), selChaman->getCeldaY());
        m_camera.centerOnCell(selChaman->getCeldaX(), selChaman->getCeldaY(), m_map.getWidth(), m_map.getHeight());
    }

    initFaithFog(playerRelId);
    m_moralitySystem.getOrCreateProfile(playerRelId);
    m_buildingSystem.clear();
    m_renderer.setBuildingSystem(&m_buildingSystem);

    // Registrar evento inicial de la epopeya (Punto de Quiebre Fundacional)
    m_eventLine.addEvent(
        startYear, EventCategory::BirthOfReligion,
        "Manifestacion Divina de " + m_creationConfig.godName,
        "El Dios " + m_creationConfig.godName + " ha establecido su pacto eterno fundando " +
        m_creationConfig.religionName + (m_creationConfig.starterChest ? " con el bendito Cofre Sagrado." : "."),
        {playerHearth.x, playerHearth.y}, playerRelId, 0, playerColor,
        true, m_creationConfig.godName, "Pacto sagrado eterno y genesis de tu civilizacion"
    );

    m_gameOverTriggered = false;
    m_gameOverTimer = 0.0f;
    m_extinctionYear = startYear;

    // Cambiar estado a Playing
    m_gameState = GameState::Playing;
    m_activeInputField = 0;

    // Guardado inicial del nuevo mundo estilo Minecraft (level.dat + icon.ppm)
    saveCurrentGame();
}

void Game::loadStateFromSave(const WorldMap& map, const ReligionRegistry& registry,
                             int currentYear, const std::string& godName,
                             const std::string& religionName, uint32_t seed,
                             bool villagersLoadedFromSnapshot,
                             float timeOfDay,
                             const std::string& slotName,
                             int daysPassed,
                             int advancementBonusYears,
                             int startYear) {
    m_map = map;
    m_registry = registry;
    m_creationConfig.godName = godName;
    m_creationConfig.religionName = religionName;
    m_creationConfig.mapSeed = seed;

    if (!slotName.empty()) {
        m_currentSaveSlot = slotName;
    } else if (m_currentSaveSlot.empty()) {
        m_currentSaveSlot = SaveSystem::generateUniqueSlotName(godName, seed);
    }

    m_dayNightCycle.setTimeOfDay(timeOfDay);
    m_dayNightCycle.setDaysPassed(daysPassed);

    m_timeline.reset(startYear, 2200);
    m_timeline.setCurrentYear(currentYear);
    m_timeline.setAdvancementBonusYears(advancementBonusYears);
    m_timeEngine.reset(currentYear);

    m_camera.reset(m_map.getWidth(), m_map.getHeight());

    if (villagersLoadedFromSnapshot) {
        // EN: Villagers were already restored 1:1 with exact identities from villagers.dat
        // ES: Los aldeanos ya fueron restaurados 1:1 con identidades exactas desde villagers.dat
        m_gestorHumanos.actualizar(0.0f, m_map);
    } else {
        // EN: Fallback demographic reconstruction for legacy saves lacking villagers.dat
        // ES: Reconstrucción demográfica de respaldo para partidas antiguas sin villagers.dat
        int totalSavedPop = m_map.getTotalWorldPopulation();
        if (totalSavedPop > 250 || totalSavedPop <= 0) {
            m_gestorHumanos.inicializarPoblacion(m_map, m_registry, 80);
        } else {
            m_gestorHumanos.getHumanos().clear();
            std::map<int, bool> hasShamanForRel;
            static const char* NOMBRES_CHAMAN[6] = {
                "Chaman Arandu", "Chaman Arandu", "Chaman Khep",
                "Chaman Orph", "Chaman Mithra", "Chaman Dharma"
            };
            int nextVillagerId = 1;
            int firstShamanId = -1;
            for (int cellY = 0; cellY < m_map.getHeight(); ++cellY) {
                for (int cellX = 0; cellX < m_map.getWidth(); ++cellX) {
                    Cell& targetCell = m_map.getCell(cellX, cellY);
                    int count = std::clamp(targetCell.population, 0, 10);
                    targetCell.population = count;
                    for (int villagerIndex = 0; villagerIndex < count; ++villagerIndex) {
                        float subOffsetX = 0.25f + 0.15f * static_cast<float>(villagerIndex % 3);
                        float subOffsetY = 0.25f + 0.15f * static_cast<float>((villagerIndex / 3) % 3);
                        int currentVillagerId = nextVillagerId++;
                        int religionId = (targetCell.dominantReligionId > 0) ? targetCell.dominantReligionId : 1;
                        bool makeShaman = !hasShamanForRel[religionId];
                        std::string unitName = makeShaman
                            ? std::string(NOMBRES_CHAMAN[std::clamp(religionId, 0, 5)])
                            : ("Grog #" + std::to_string(currentVillagerId));
                        Humano newVillager(currentVillagerId, unitName, static_cast<float>(cellX) + subOffsetX, static_cast<float>(cellY) + subOffsetY);
                        newVillager.setEsChaman(makeShaman);
                        newVillager.setReligionId(religionId);
                        newVillager.setDevocion(makeShaman ? 0.98f : targetCell.devotion);
                        if (makeShaman) {
                            hasShamanForRel[religionId] = true;
                            if (religionId == 1 || firstShamanId < 0) firstShamanId = currentVillagerId;
                        }
                        m_gestorHumanos.agregarHumano(newVillager);
                    }
                }
            }
            if (firstShamanId > 0) {
                m_gestorHumanos.seleccionarHumano(firstShamanId);
            }
            m_gestorHumanos.actualizar(0.0f, m_map);
        }
    }
    m_map.recalculateReligions(m_registry);

    initFaithFog(resolvePlayerReligionId());
    m_renderer.setBuildingSystem(&m_buildingSystem);

    m_gameOverTriggered = false;
    m_gameOverTimer = 0.0f;
    m_extinctionYear = currentYear;

    m_gameState = GameState::Playing;
    saveCurrentGame(m_currentSaveSlot);
}

void Game::saveCurrentGame(const std::string& slotName) {
    std::string folder = slotName;
    if (folder.empty()) {
        if (!m_currentSaveSlot.empty()) {
            folder = m_currentSaveSlot;
        } else {
            m_currentSaveSlot = SaveSystem::generateUniqueSlotName(m_creationConfig.godName, m_creationConfig.mapSeed);
            folder = m_currentSaveSlot;
        }
    } else {
        m_currentSaveSlot = folder;
    }
    SaveSystem::saveWorld(*this, folder);
}

void Game::runHeadless(int maxTicks, const std::string& exportPPM, bool revealFog) {
    startNewCustomWorld();
    m_gameState = GameState::Playing;
    if (revealFog) {
        executeCommand("LUMINUS");
    }
    std::cout << "[Simulator] Running headless simulation for " << maxTicks << " steps..." << std::endl;
    float deltaSimulationTime = 0.2f;

    for (int tickIndex = 0; tickIndex < maxTicks; ++tickIndex) {
        update(deltaSimulationTime);
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
    for (const auto& religionEntry : m_registry.getAllReligions()) {
        std::cout << "  - " << religionEntry.name << " (" << religionEntry.symbol << ") Followers: "
                  << religionEntry.totalFollowers << " Dominant Cells: " << religionEntry.totalCellsDominant
                  << " Era: " << AdvancementSystem::getEraName(religionEntry.currentEra)
                  << (religionEntry.isExtinct ? " [EXTINCT]" : "") << std::endl;
    }
}

void Game::applySettings(const GameSettings& settings) {
    // EN: Synchronize audio system gains and mute toggles
    // ES: Sincronizar ganancias del sistema de audio y conmutadores de silencio
    m_soundSystem.setMasterVolume(static_cast<float>(settings.masterVolumePercent) / 100.0f);
    m_soundSystem.setMusicVolume(static_cast<float>(settings.musicVolumePercent) / 100.0f);
    m_soundSystem.setSFXVolume(static_cast<float>(settings.sfxVolumePercent) / 100.0f);
    m_soundSystem.setAmbientVolume(static_cast<float>(settings.ambientVolumePercent) / 100.0f);
    m_soundSystem.setMuted(settings.soundMuted);

    // EN: Synchronize default timeline simulation pacing
    // ES: Sincronizar ritmo de simulación de la línea temporal por defecto
    m_timeline.setYearsPerSecond(GameSettings::getSimulationSpeedValue(settings.defaultSpeed));

    // EN: Synchronize isometric terrain grid visualization with renderer
    // ES: Sincronizar visualización de retícula isométrica de terreno con el renderizador
    m_renderer.setShowGrid(settings.showIsometricGrid);
    m_renderer.setDeveloperMode(settings.developerMode);
    m_developerMode = settings.developerMode;

    // EN: Target window dimensions according to the selected resolution setting
    // ES: Dimensiones de ventana objetivo de acuerdo con el ajuste de resolución seleccionado
    const int targetCanvasWidth = GameSettings::getResolutionWidth(settings.resolution);
    const int targetCanvasHeight = GameSettings::getResolutionHeight(settings.resolution);
    m_renderer.setWindowedResolution(targetCanvasWidth, targetCanvasHeight);

    // EN: Synchronize display mode (Fullscreen vs Windowed)
    // ES: Sincronizar modo de pantalla (Pantalla completa vs Ventana)
    const bool shouldBeFullscreen = (settings.displayMode == DisplayMode::BorderlessFullscreen);
    if (m_renderer.isFullscreen() != shouldBeFullscreen) {
        m_renderer.setFullscreen(shouldBeFullscreen);
    }

    // EN: Ensure Game viewport dimensions and camera synchronize with active renderer
    // ES: Asegurar que las dimensiones del viewport de Game y la cámara se sincronicen con el renderizador activo
    if (m_renderer.getWidth() != m_windowWidth || m_renderer.getHeight() != m_windowHeight) {
        m_windowWidth = m_renderer.getWidth();
        m_windowHeight = m_renderer.getHeight();
        m_camera.setScreenSize(m_windowWidth, m_windowHeight);
    }
}

void Game::loadSettings() {
    const std::string settingsFilePath = SaveSystem::getSavesRootDirectory() + "/game_settings.cfg";
    if (m_settings.loadFromFile(settingsFilePath)) {
        std::cout << "[GameSettings] Opciones cargadas desde: " << settingsFilePath << std::endl;
    } else {
        std::cout << "[GameSettings] Usando configuracion predeterminada (sin archivo previo)." << std::endl;
    }
}

void Game::saveSettings() {
    const std::string settingsFilePath = SaveSystem::getSavesRootDirectory() + "/game_settings.cfg";
    if (m_settings.saveToFile(settingsFilePath)) {
        std::cout << "[GameSettings] Opciones guardadas exitosamente en: " << settingsFilePath << std::endl;
    } else {
        std::cerr << "[GameSettings] Error al guardar opciones en: " << settingsFilePath << std::endl;
    }
}

// =============================================================================
// EN: Developer mode toggle and direct control methods
// ES: Métodos de alternancia y control directo del modo desarrollador
// =============================================================================
void Game::toggleDeveloperMode() {
    m_developerMode = !m_developerMode;
    m_settings.developerMode = m_developerMode;
    m_renderer.setDeveloperMode(m_developerMode);
    m_soundSystem.playSFX(SFXType::UIClick);
    showFeedback(m_developerMode ? "MODO DESARROLLADOR: ACTIVADO [F3]" : "MODO DESARROLLADOR: DESACTIVADO [F3]", 3.0f);
}

void Game::setDeveloperMode(bool enable) {
    m_developerMode = enable;
    m_settings.developerMode = m_developerMode;
    m_renderer.setDeveloperMode(m_developerMode);
}

// =============================================================================
// EN: Setup Map Editor state, initializing map and parameters if not yet created
// ES: Configurar estado del Editor de Mapas, inicializando mapa y parámetros
// =============================================================================
void Game::setupMapEditor() {
    m_previousGameState = m_gameState;
    m_gameState = GameState::MapEditor;
    m_isPaintingMapEditor = false;
    m_activeInputField = 0;

    const int mapDim = getMapDimensionFromIndex(m_mapEditorConfig.mapSizeIndex);
    if (m_editorMap.getWidth() != mapDim || m_editorMap.getHeight() != mapDim) {
        regenerateEditorMap();
    } else {
        m_camera.reset(mapDim, mapDim);
        m_camera.centerOnCell(mapDim / 2, mapDim / 2, mapDim, mapDim);
        m_camera.setZoom(1.0f);
        m_mapEditorConfig.statusMessage = "TALLER DE CARTOGRAFIA LITURGICA ACTIVO";
        m_mapEditorConfig.statusTimer = 3.0f;
    }
}

// =============================================================================
// EN: Regenerate procedural world in editor according to seed, climate & size
// ES: Regenerar mundo procedimental en el editor según semilla, clima y tamaño
// =============================================================================
void Game::regenerateEditorMap() {
    const int mapDim = getMapDimensionFromIndex(m_mapEditorConfig.mapSizeIndex);
    m_editorMap = WorldMap(mapDim, mapDim);
    m_editorMap.generateProceduralWorld(m_mapEditorConfig.mapSeed,
                                        m_mapEditorConfig.worldClimate,
                                        m_mapEditorConfig.resourceAbundance);
    m_editorMap.updateWaterDepthPhases();
    m_camera.reset(mapDim, mapDim);
    m_camera.centerOnCell(mapDim / 2, mapDim / 2, mapDim, mapDim);
    m_camera.setZoom(1.0f);

    m_mapEditorConfig.statusMessage = "MUNDO PROCEDIMENTAL GENERADO (" +
                                      std::to_string(mapDim) + "x" +
                                      std::to_string(mapDim) + " CELDAS)";
    m_mapEditorConfig.statusTimer = 3.5f;
}

// =============================================================================
// EN: Apply the active brush to the editor world map at given cell coordinates
// ES: Aplicar el pincel activo al mapa de mundo del editor en las coordenadas dadas
// =============================================================================
void Game::applyMapEditorBrush(int centerCellX, int centerCellY) {
    if (!m_editorMap.isValid(centerCellX, centerCellY)) return;

    if (m_mapEditorConfig.activeBrush == MapEditorBrush::Inspect) {
        const Cell& cell = m_editorMap.getCell(centerCellX, centerCellY);
        m_mapEditorConfig.statusMessage = "Celda (" + std::to_string(centerCellX) + ", " +
                                          std::to_string(centerCellY) + ") Elev:" +
                                          std::to_string(cell.elevation) + " Rec:" +
                                          getResourceTypeName(cell.naturalResource);
        m_mapEditorConfig.statusTimer = 2.5f;
        return;
    }

    const int radiusOffset = (m_mapEditorConfig.brushRadius == 1) ? 0 :
                             ((m_mapEditorConfig.brushRadius == 2) ? 1 : 2);
    bool modifiedTerrain = false;

    for (int deltaY = -radiusOffset; deltaY <= radiusOffset; ++deltaY) {
        for (int deltaX = -radiusOffset; deltaX <= radiusOffset; ++deltaX) {
            const int targetX = centerCellX + deltaX;
            const int targetY = centerCellY + deltaY;
            if (!m_editorMap.isValid(targetX, targetY)) continue;

            Cell& cell = m_editorMap.getCell(targetX, targetY);
            modifiedTerrain = true;

            switch (m_mapEditorConfig.activeBrush) {
                case MapEditorBrush::Ocean:
                    cell.customTileIndex = -1;
                    cell.biome = BiomeType::DeepOcean;
                    cell.elevation = 0.0f;
                    cell.waterDepth = 1.0f;
                    cell.naturalResource = ResourceType::None;
                    cell.resourceAmount = 0;
                    break;
                case MapEditorBrush::ShallowWater:
                    cell.customTileIndex = -1;
                    cell.biome = BiomeType::ShallowWater;
                    cell.elevation = 1.0f;
                    cell.waterDepth = 0.5f;
                    cell.naturalResource = ResourceType::None;
                    cell.resourceAmount = 0;
                    break;
                case MapEditorBrush::FertileValley:
                    cell.customTileIndex = -1;
                    cell.biome = BiomeType::FertileValley;
                    cell.elevation = std::max(1.0f, cell.elevation);
                    cell.waterDepth = 0.0f;
                    break;
                case MapEditorBrush::DenseForest:
                    cell.customTileIndex = -1;
                    cell.biome = BiomeType::DenseForest;
                    cell.elevation = std::max(1.0f, cell.elevation);
                    cell.waterDepth = 0.0f;
                    cell.naturalResource = ResourceType::Wood;
                    cell.resourceAmount = 300;
                    break;
                case MapEditorBrush::HighlandHills:
                    cell.customTileIndex = -1;
                    cell.biome = BiomeType::HighlandHills;
                    cell.elevation = std::clamp(cell.elevation, 2.0f, 4.0f);
                    cell.waterDepth = 0.0f;
                    break;
                case MapEditorBrush::RuggedMountains:
                    cell.customTileIndex = -1;
                    cell.biome = BiomeType::RuggedMountains;
                    cell.elevation = std::clamp(cell.elevation, 4.0f, 7.0f);
                    cell.waterDepth = 0.0f;
                    break;
                case MapEditorBrush::AridDesert:
                    cell.customTileIndex = -1;
                    cell.biome = BiomeType::AridDesert;
                    cell.elevation = std::max(1.0f, cell.elevation);
                    cell.waterDepth = 0.0f;
                    break;
                case MapEditorBrush::FrozenTundra:
                    cell.customTileIndex = -1;
                    cell.biome = BiomeType::FrozenTundra;
                    cell.elevation = std::max(1.0f, cell.elevation);
                    cell.waterDepth = 0.0f;
                    break;
                case MapEditorBrush::VolcanicCaldera:
                    cell.customTileIndex = -1;
                    cell.biome = BiomeType::VolcanicCaldera;
                    cell.elevation = std::max(3.0f, cell.elevation);
                    cell.waterDepth = 0.0f;
                    cell.hasVolcano = true;
                    cell.hasActiveLava = true;
                    cell.lavaTimer = 10.0f;
                    break;
                case MapEditorBrush::Wood:
                    if (!cell.isNaturalWater()) {
                        cell.naturalResource = ResourceType::Wood;
                        cell.resourceAmount = 250;
                        cell.hasTreesUprooted = false;
                    }
                    break;
                case MapEditorBrush::Stone:
                    if (!cell.isNaturalWater()) {
                        cell.naturalResource = ResourceType::Stone;
                        cell.resourceAmount = 250;
                    }
                    break;
                case MapEditorBrush::Gold:
                    if (!cell.isNaturalWater()) {
                        cell.naturalResource = ResourceType::Gold;
                        cell.resourceAmount = 200;
                    }
                    break;
                case MapEditorBrush::Iron:
                    if (!cell.isNaturalWater()) {
                        cell.naturalResource = ResourceType::Iron;
                        cell.resourceAmount = 250;
                    }
                    break;
                case MapEditorBrush::Copper:
                    if (!cell.isNaturalWater()) {
                        cell.naturalResource = ResourceType::Copper;
                        cell.resourceAmount = 250;
                    }
                    break;
                case MapEditorBrush::Diamond:
                    if (!cell.isNaturalWater()) {
                        cell.naturalResource = ResourceType::Diamond;
                        cell.resourceAmount = 150;
                    }
                    break;
                case MapEditorBrush::ClearResource:
                    cell.naturalResource = ResourceType::None;
                    cell.resourceAmount = 0;
                    cell.hasVolcano = false;
                    cell.hasActiveLava = false;
                    cell.hasImpactCrater = false;
                    cell.isBurnt = false;
                    break;
                case MapEditorBrush::RaiseElevation:
                    cell.elevation = std::min(7.0f, cell.elevation + 1.0f);
                    if (cell.elevation > 0.5f && cell.isNaturalWater()) {
                        cell.biome = BiomeType::CoastalPlains;
                        cell.waterDepth = 0.0f;
                    }
                    break;
                case MapEditorBrush::LowerElevation:
                    cell.elevation = std::max(0.0f, cell.elevation - 1.0f);
                    if (cell.elevation <= 0.05f) {
                        cell.biome = BiomeType::DeepOcean;
                        cell.waterDepth = 1.0f;
                    }
                    break;
                case MapEditorBrush::FlattenElevation:
                    cell.elevation = 1.0f;
                    if (cell.isNaturalWater()) {
                        cell.waterDepth = 0.0f;
                        cell.biome = BiomeType::CoastalPlains;
                    }
                    break;
                case MapEditorBrush::TileGrass:
                    cell.customTileIndex = 0;
                    cell.biome = BiomeType::FertileValley;
                    cell.waterDepth = 0.0f;
                    cell.elevation = std::max(1.0f, cell.elevation);
                    break;
                case MapEditorBrush::TileRock:
                    cell.customTileIndex = 1;
                    cell.biome = BiomeType::RuggedMountains;
                    cell.waterDepth = 0.0f;
                    cell.elevation = std::max(2.0f, cell.elevation);
                    break;
                case MapEditorBrush::TileTropicalPlains:
                    cell.customTileIndex = 2;
                    cell.biome = BiomeType::CoastalPlains;
                    cell.waterDepth = 0.0f;
                    cell.elevation = std::max(1.0f, cell.elevation);
                    break;
                case MapEditorBrush::TileFarmland:
                    cell.customTileIndex = 3;
                    cell.biome = BiomeType::FertileValley;
                    cell.waterDepth = 0.0f;
                    cell.elevation = std::max(1.0f, cell.elevation);
                    break;
                case MapEditorBrush::TileDeepOceanAlt:
                    cell.customTileIndex = 4;
                    cell.biome = BiomeType::DeepOcean;
                    cell.waterDepth = 1.0f;
                    cell.elevation = 0.0f;
                    cell.naturalResource = ResourceType::None;
                    break;
                case MapEditorBrush::TileRichForest:
                    cell.customTileIndex = 5;
                    cell.biome = BiomeType::DenseForest;
                    cell.waterDepth = 0.0f;
                    cell.elevation = std::max(1.0f, cell.elevation);
                    cell.naturalResource = ResourceType::Wood;
                    cell.resourceAmount = 300;
                    cell.hasTreesUprooted = false;
                    break;
                case MapEditorBrush::TileHighlands:
                    cell.customTileIndex = 6;
                    cell.biome = BiomeType::HighlandHills;
                    cell.waterDepth = 0.0f;
                    cell.elevation = std::clamp(cell.elevation, 2.0f, 4.0f);
                    break;
                case MapEditorBrush::TileSwamp:
                    cell.customTileIndex = 7;
                    cell.biome = BiomeType::CoastalPlains;
                    cell.waterDepth = 0.20f;
                    cell.elevation = 0.8f;
                    break;
                case MapEditorBrush::TileSand:
                    cell.customTileIndex = 8;
                    cell.biome = BiomeType::AridDesert;
                    cell.waterDepth = 0.0f;
                    cell.elevation = std::max(1.0f, cell.elevation);
                    break;
                case MapEditorBrush::TileSnow:
                    cell.customTileIndex = 9;
                    cell.biome = BiomeType::FrozenTundra;
                    cell.waterDepth = 0.0f;
                    cell.elevation = std::max(1.0f, cell.elevation);
                    break;
                case MapEditorBrush::TileShallowWater:
                    cell.customTileIndex = 10;
                    cell.biome = BiomeType::ShallowWater;
                    cell.waterDepth = 0.5f;
                    cell.elevation = 1.0f;
                    cell.naturalResource = ResourceType::None;
                    break;
                case MapEditorBrush::TileMidOcean:
                    cell.customTileIndex = 11;
                    cell.biome = BiomeType::DeepOcean;
                    cell.waterDepth = 0.8f;
                    cell.elevation = 0.0f;
                    cell.naturalResource = ResourceType::None;
                    break;
                case MapEditorBrush::TileCaveWater:
                    cell.customTileIndex = 12;
                    cell.biome = BiomeType::ShallowWater;
                    cell.waterDepth = 0.6f;
                    cell.elevation = 0.0f;
                    cell.naturalResource = ResourceType::None;
                    break;
                case MapEditorBrush::TileAbyssal:
                    cell.customTileIndex = 13;
                    cell.biome = BiomeType::DeepOcean;
                    cell.waterDepth = 1.0f;
                    cell.elevation = 0.0f;
                    cell.naturalResource = ResourceType::None;
                    break;
                case MapEditorBrush::TileOpenOcean:
                    cell.customTileIndex = 14;
                    cell.biome = BiomeType::DeepOcean;
                    cell.waterDepth = 0.9f;
                    cell.elevation = 0.0f;
                    cell.naturalResource = ResourceType::None;
                    break;
                case MapEditorBrush::TileLava:
                    cell.customTileIndex = 15;
                    cell.biome = BiomeType::VolcanicCaldera;
                    cell.hasActiveLava = true;
                    cell.lavaTimer = 10.0f;
                    cell.elevation = std::max(1.0f, cell.elevation);
                    break;
                case MapEditorBrush::TileAsh:
                    cell.isBurnt = true;
                    cell.naturalResource = ResourceType::None;
                    cell.hasTreesUprooted = true;
                    break;
                case MapEditorBrush::TileCrater:
                    cell.formCrater(CraterType::MeteorImpact, 0.8f, 0.3f);
                    break;
                default:
                    break;
            }
        }
    }

    if (modifiedTerrain) {
        m_editorMap.updateWaterDepthPhases();
    }
}

// =============================================================================
// EN: Start Playing a new game directly using the custom-crafted editor world map
// ES: Comenzar a Jugar una nueva partida directamente usando el mapa personalizado del editor
// =============================================================================
void Game::startPlayWithEditorMap() {
    resetAllGameSubsystems();

    m_currentSaveSlot = SaveSystem::generateUniqueSlotName("CustomMap", m_mapEditorConfig.mapSeed);

    const int mapDim = m_editorMap.getWidth();
    m_map = m_editorMap; // Assign full edited map
    m_cloudSystem.reset(mapDim, mapDim);
    m_renderer.setCloudSystem(&m_cloudSystem);
    m_fog.reset(mapDim, mapDim);
    ErrorSystem::instance().setMaxSafeEntities(std::max(450u, static_cast<uint32_t>(mapDim * 4)));

    for (int cellY = 0; cellY < mapDim; ++cellY) {
        for (int cellX = 0; cellX < mapDim; ++cellX) {
            m_map.getCell(cellX, cellY).population = 0;
        }
    }

    constexpr int startYear = -4000;
    m_registry = ReligionRegistry{};
    m_timeline.reset(startYear, 2200);
    m_dayNightCycle.resetDaysPassed();
    m_timeline.setYearsPerSecond(4.0f);
    m_timeEngine.reset(startYear);

    // Color Oro Divino para el jugador
    ColorRGB playerColor = Palette::DIVINE_GOLD;

    int playerRelId = m_registry.createPrimordialReligion(
        "Culto Primordial",
        "Creador Divino",
        playerColor,
        "Sun",
        startYear
    );
    Religion* pRel = m_registry.getReligion(playerRelId);
    if (pRel) {
        pRel->currentEra = HistoricalEra::PrimordialAnimism;
        pRel->resources.food = 600;
        pRel->resources.wood = 350;
        pRel->resources.stone = 200;
        pRel->resources.gold = 50;
        pRel->theologyPoints += 500;
        pRel->spiritualPiety = 0.90f;
        pRel->zeal = 0.80f;
    }

    // 4 Religiones rivales
    struct RivalDef { const char* name; const char* founder; ColorRGB color; const char* symbol; };
    RivalDef rivals[4] = {
        {"Animist Gaia-Cult", "Zarah-Khep", ColorRGB(46, 204, 113), "Tree"},
        {"The Sacred Monad", "Melchizedek", ColorRGB(52, 152, 219), "Spiral"},
        {"Order of the Eternal Flame", "Mithras-Gard", ColorRGB(230, 57, 70), "Flame"},
        {"Wheel of the Star-Dharma", "Siddhartha-Yin", ColorRGB(155, 89, 182), "Wheel"}
    };

    for (int rivalIndex = 0; rivalIndex < 4; ++rivalIndex) {
        int rivalId = m_registry.createPrimordialReligion(
            rivals[rivalIndex].name, rivals[rivalIndex].founder, rivals[rivalIndex].color, rivals[rivalIndex].symbol, startYear
        );
        Religion* rivalRelPtr = m_registry.getReligion(rivalId);
        if (rivalRelPtr) {
            rivalRelPtr->currentEra = HistoricalEra::PrimordialAnimism;
            rivalRelPtr->zeal = 0.5f;
            rivalRelPtr->resources.food = 400;
            rivalRelPtr->resources.wood = 250;
            rivalRelPtr->resources.stone = 150;
            rivalRelPtr->resources.gold = 30;
        }
    }

    // Ubicaciones de pueblos
    std::vector<Point2i> puebloLocations = m_map.generatePuebloLocations(5, m_mapEditorConfig.mapSeed);
    Point2i playerHearth{mapDim / 2, mapDim / 2};

    // Si el mapa fue modificado manualmente y no hay suficientes posiciones generadas, buscar celdas habitables
    if (puebloLocations.size() < 5) {
        puebloLocations.clear();
        for (int cellY = 2; cellY < mapDim - 2 && puebloLocations.size() < 5; cellY += 4) {
            for (int cellX = 2; cellX < mapDim - 2 && puebloLocations.size() < 5; cellX += 4) {
                if (m_map.getCell(cellX, cellY).isPassableForGround()) {
                    puebloLocations.push_back({cellX, cellY});
                }
            }
        }
    }

    if (!puebloLocations.empty()) {
        playerHearth = puebloLocations[0];
    } else {
        // En caso extremo de mapa todo agua, crear una pequeña isla sagrada
        playerHearth = {mapDim / 2, mapDim / 2};
        Cell& islandCell = m_map.getCell(playerHearth.x, playerHearth.y);
        islandCell.biome = BiomeType::FertileValley;
        islandCell.elevation = 2.0f;
        islandCell.waterDepth = 0.0f;
    }

    Cell& playerHearthCell = m_map.getCell(playerHearth.x, playerHearth.y);
    playerHearthCell.setShare(playerRelId, 0.98f);
    playerHearthCell.devotion = 0.95f;
    m_map.consecrateHolySite(playerHearth.x, playerHearth.y, "Santuario Sagrado Primordial");
    playerHearthCell.shrineLevel = 3;

    // Asignar territorio sagrado alrededor del santuario del jugador
    for (int deltaCellY = -2; deltaCellY <= 2; ++deltaCellY) {
        for (int deltaCellX = -2; deltaCellX <= 2; ++deltaCellX) {
            int neighborCellX = playerHearth.x + deltaCellX;
            int neighborCellY = playerHearth.y + deltaCellY;
            float distance = std::sqrt(static_cast<float>(deltaCellX * deltaCellX + deltaCellY * deltaCellY));
            if (m_map.isValid(neighborCellX, neighborCellY) && distance <= 2.5f && (deltaCellX != 0 || deltaCellY != 0)) {
                Cell& neighborCell = m_map.getCell(neighborCellX, neighborCellY);
                if (neighborCell.isHabitable()) {
                    float share = std::max(0.60f, 0.90f - distance * 0.12f);
                    neighborCell.setShare(playerRelId, share);
                    neighborCell.devotion = std::max(0.55f, 0.85f - distance * 0.12f);
                }
            }
        }
    }

    // Inicializar población humana de aldeanos sobre el mapa personalizado
    m_gestorHumanos.inicializarPoblacion(m_map, m_registry, 80);
    m_map.recalculateReligions(m_registry);

    initFaithFog(playerRelId);
    m_camera.centerOnCell(playerHearth.x, playerHearth.y, mapDim, mapDim);
    selectCell(playerHearth.x, playerHearth.y);

    m_moralitySystem.getOrCreateProfile(playerRelId);
    m_buildingSystem.clear();
    m_renderer.setBuildingSystem(&m_buildingSystem);

    m_gameOverTriggered = false;
    m_gameOverTimer = 0.0f;
    m_extinctionYear = startYear;

    m_gameState = GameState::Playing;
    showFeedback("¡Comenzando partida con el mundo personalizado!", 4.0f);
    saveCurrentGame(m_currentSaveSlot);
}

} // namespace ReligionSim
