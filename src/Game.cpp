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
    m_creationConfig.mapSeed = static_cast<uint32_t>(std::time(nullptr)) ^ 0xA5A5A5A5;

    // Inicializar sistema de sonido y música de la placa de sonido
    m_soundSystem.init();

    // Try initializing window (X11 / Win32 / OpenGL)
    bool windowOk = m_renderer.initWindow("Marangatu: The Faith Simulator [Pre-Alpha v0.4.0]");
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
    m_renderer.update(deltaTime);

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

    // 1. Sincronizar simulación y paso del tiempo con el movimiento de los sprites
    float dtYears = 0.0f;
    m_timeline.update(deltaTime, dtYears);
    m_timeEngine.setYear(static_cast<double>(m_timeline.getCurrentYear()));

    bool isPaused = m_timeline.isPaused();
    float speedMult = isPaused ? 0.0f : static_cast<float>(m_timeline.getSpeedMultiplier());
    int currentYear = m_timeline.getCurrentYear();
    float spriteDt = deltaTime * speedMult;

    // Actualizar paisaje sonoro ambiental y música adaptativa sincronizados exactamente con el tiempo de la simulación
    m_soundSystem.update(deltaTime, m_map, m_camera, &m_dayNightCycle, &m_divinePowers, m_gestorHumanos, isPaused, speedMult, currentYear);

    m_dayNightCycle.setPaused(isPaused);
    m_dayNightCycle.update(spriteDt);
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
        }
    }

    m_divinePowers.update(deltaTime, m_map, m_gestorHumanos);
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
    if (m_gameState == GameState::Playing) {
        m_renderer.setDayNightCycle(&m_dayNightCycle);
    } else {
        m_renderer.setDayNightCycle(nullptr);
    }
    m_renderer.beginFrame();

    if (m_gameState == GameState::MainMenu) {
        m_renderer.renderMainMenu(m_hoveredMenuButtonId);
    } else if (m_gameState == GameState::WorldCreation) {
        m_renderer.renderWorldCreationMenu(m_creationConfig, m_hoveredMenuButtonId, m_activeInputField);
    } else if (m_gameState == GameState::LoadWorldMenu) {
        m_renderer.renderLoadWorldMenu(m_hoveredMenuButtonId, m_selectedSaveIndex);
    } else if (m_gameState == GameState::Options) {
        m_renderer.renderOptionsMenu(m_hoveredMenuButtonId);
    } else if (m_gameState == GameState::GameOver) {
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
                                                           m_eventLine, playerRel);
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
                if (divineType == DivinePowerType::Miracle) {
                    m_nemesisSififus.onUnitSparedOrBlessed(livingVillager, m_timeline.getCurrentYear(), &m_eventLine, &m_registry);
                } else if (divineType == DivinePowerType::Lightning || divineType == DivinePowerType::Meteor ||
                           divineType == DivinePowerType::Earthquake || divineType == DivinePowerType::Volcano ||
                           divineType == DivinePowerType::Wildfire) {
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
            m_gameState = GameState::MainMenu;
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
        default: break;
    }
}

void Game::onMouseDown(int mouseButton, float screenPosX, float screenPosY) {
    m_mouseX = screenPosX;
    m_mouseY = screenPosY;
    if (m_gameState == GameState::MainMenu) {
        if (mouseButton == 1) {
            int btnId = m_renderer.checkMenuButtonClicked(screenPosX, screenPosY, false);
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
        if (mouseButton == 1) {
            int btnId = m_renderer.checkCreationButtonClicked(screenPosX, screenPosY);
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
                m_creationConfig.mapSizeIndex = (m_creationConfig.mapSizeIndex + 1) % 3;
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
        if (mouseButton == 1) {
            int btnId = m_renderer.checkMenuButtonClicked(screenPosX, screenPosY, true);
            if (btnId == 12) { // VOLVER AL MENU
                m_gameState = GameState::MainMenu;
            }
        }
        return;
    }

    if (m_gameState == GameState::GameOver) {
        if (mouseButton == 1) {
            int btnId = m_renderer.checkGameOverButtonClicked(screenPosX, screenPosY);
            if (btnId == 901) { // REINTENTAR GÉNESIS
                m_gameState = GameState::WorldCreation;
                m_activeInputField = 0;
                m_gameOverTriggered = false;
                m_gameOverTimer = 0.0f;
            } else if (btnId == 902) { // CARGAR PARTIDA
                m_gameState = GameState::LoadWorldMenu;
                m_selectedSaveIndex = 0;
                m_gameOverTriggered = false;
                m_gameOverTimer = 0.0f;
            } else if (btnId == 903) { // MENÚ PRINCIPAL
                m_gameState = GameState::MainMenu;
                m_gameOverTriggered = false;
                m_gameOverTimer = 0.0f;
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
                    std::vector<GameEvent> newEvents;
                    if (m_advancement.convalidateAdvancement(*pRel, selAdvId, m_timeline.getCurrentYear(), newEvents)) {
                        for (const auto& ev : newEvents) {
                            m_eventLine.addEvent(ev);
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
            // (salvo con el truco SELECTOR, que permite seleccionar cualquier unidad).
            const int onlyRel = m_cheatSelectorActive ? -1 : PLAYER_RELIGION_ID;
            int humanoId = m_gestorHumanos.buscarHumanoEnPantalla(screenPosX, screenPosY, m_camera, m_map, onlyRel);
            if (humanoId <= 0) {
                humanoId = m_gestorHumanos.buscarHumanoCercano(static_cast<float>(gridCellX) + 0.5f,
                                                               static_cast<float>(gridCellY) + 0.5f, 0.85f, onlyRel);
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
    if (mouseButton == 1) {
        m_scrubbingTimeline = false;
        m_renderer.setMouseDown(false, screenPosX, screenPosY);
    } else if (mouseButton == 3) {
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
        m_hoveredMenuButtonId = m_renderer.checkMenuButtonClicked(screenPosX, screenPosY, true);
        return;
    }

    if (m_gameState == GameState::GameOver) {
        m_hoveredMenuButtonId = m_renderer.checkGameOverButtonClicked(screenPosX, screenPosY);
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
    if (m_gameState == GameState::GameOver) {
        if (keySym == 27) { // ESC -> Salir al menú principal
            m_gameState = GameState::MainMenu;
            m_gameOverTriggered = false;
            m_gameOverTimer = 0.0f;
            return;
        }
        if (keySym == 13 || keySym == 10 || keySym == 32) { // Enter o Espacio -> Reintentar Génesis
            m_gameState = GameState::WorldCreation;
            m_activeInputField = 0;
            m_gameOverTriggered = false;
            m_gameOverTimer = 0.0f;
            return;
        }
        return;
    }

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
        case 'u': case 'U':
            m_soundSystem.toggleMute();
            showFeedback(m_soundSystem.isMuted() ? "AUDIO: SILENCIADO [Mute]" : "AUDIO: ACTIVADO", 2.5f);
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
    } else if (cmd == "MUTE" || cmd == "SILENCIO") {
        m_soundSystem.toggleMute();
        showFeedback(m_soundSystem.isMuted() ? "AUDIO: SILENCIADO [Mute]" : "AUDIO: ACTIVADO", 4.0f);
    } else if (cmd == "SONIDO" || cmd == "AUDIO") {
        showFeedback("DISPOSITIVO DE SONIDO: " + m_soundSystem.getDeviceName() + " | VOL: " +
                     std::to_string(static_cast<int>(m_soundSystem.getMasterVolume() * 100)) + "%", 4.0f);
    } else {
        showFeedback("COMANDO DESCONOCIDO: '" + cmd + "' (USA: SELECTOR | LUMINUS | DIA | NOCHE | AMANECER | ATARDECER | METEORITO | VOLCAN | MUTE | AUDIO)", 4.0f);
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

void Game::startNewCustomWorld() {
    std::cout << "==========================================================" << std::endl;
    std::cout << "[Génesis] Creando nuevo mundo sagrado personalizado..." << std::endl;
    std::cout << "  - Dios Guía: " << m_creationConfig.godName << std::endl;
    std::cout << "  - Religión Elegida: " << m_creationConfig.religionName << std::endl;
    std::cout << "  - Semilla (Seed): " << m_creationConfig.mapSeed << std::endl;
    std::cout << "  - Población Inicial: " << m_creationConfig.initialPopulation << std::endl;
    std::cout << "  - Recursos Iniciales: " << m_creationConfig.initialResources << " (Comida/Materiales)" << std::endl;
    std::cout << "  - Época de Inicio: Era " << static_cast<int>(m_creationConfig.startingEra) << std::endl;
    std::cout << "  - Cofre Sagrado: " << (m_creationConfig.starterChest ? "ACTIVADO" : "DESACTIVADO") << std::endl;
    std::cout << "==========================================================" << std::endl;

    // 1. Determinar tamaño de mapa y generar geografía procedimental adaptada al Clima y Abundancia elegidos
    int mapDim = (m_creationConfig.mapSizeIndex == 0) ? 32 : (m_creationConfig.mapSizeIndex == 2 ? 64 : 48);
    m_map = WorldMap(mapDim, mapDim);
    m_map.generateProceduralWorld(m_creationConfig.mapSeed, m_creationConfig.worldClimate, m_creationConfig.resourceAbundance);
    m_cloudSystem.reset(mapDim, mapDim);
    m_renderer.setCloudSystem(&m_cloudSystem);

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

    // EN: Normalize max population and cohesive elevations for saved games
    // ES: Normalizar maxPopulation y elevaciones cohesivas para partidas guardadas
    for (int cellY = 0; cellY < m_map.getHeight(); ++cellY) {
        for (int cellX = 0; cellX < m_map.getWidth(); ++cellX) {
            Cell& targetCell = m_map.getCell(cellX, cellY);
            if (targetCell.isWater()) {
                targetCell.elevation = 0.0f;
            } else if (targetCell.biome == BiomeType::RuggedMountains) {
                // EN: Convert ancient central gray slabs into habitable green hills
                // ES: Convertir antiguas losas grises centrales en colinas verdes habitables
                targetCell.biome = ((cellX + cellY) % 3 == 0) ? BiomeType::HighlandHills : BiomeType::FertileValley;
                targetCell.elevation = (targetCell.biome == BiomeType::HighlandHills) ? 0.95f : 0.50f;
                targetCell.maxPopulation = 6;
            } else if (targetCell.biome == BiomeType::HighlandHills) {
                targetCell.elevation = 0.95f;
            } else {
                targetCell.elevation = 0.50f;
            }
            if (targetCell.isHabitable() && targetCell.maxPopulation > 15) {
                targetCell.maxPopulation = 6;
            }
        }
    }

    int totalSavedPop = m_map.getTotalWorldPopulation();
    if (totalSavedPop > 250 || totalSavedPop <= 0) {
        // EN: Migrate ancient saves (e.g. 600,000 pop) to 80 real 1:1 villagers to avoid collapse
        // ES: Migrar partidas antiguas (ej. 600,000 hab) a 80 aldeanos reales 1:1 para evitar colapso
        m_gestorHumanos.inicializarPoblacion(m_map, m_registry, 80);
    } else {
        // EN: Restore real 1:1 villagers in each populated cell according to the saved map
        // ES: Restaurar aldeanos reales 1:1 en cada celda poblada según el mapa guardado
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
    m_map.recalculateReligions(m_registry);

    initFaithFog(resolvePlayerReligionId());
    m_buildingSystem.clear();
    m_renderer.setBuildingSystem(&m_buildingSystem);

    m_gameOverTriggered = false;
    m_gameOverTimer = 0.0f;
    m_extinctionYear = currentYear;

    m_gameState = GameState::Playing;
    saveCurrentGame();
}

void Game::saveCurrentGame(const std::string& slotName) {
    std::string folder = slotName;
    if (folder.empty()) {
        std::string safeGod = m_creationConfig.godName;
        for (char& charElem : safeGod) {
            if (!std::isalnum(static_cast<unsigned char>(charElem))) charElem = '_';
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

} // namespace ReligionSim
