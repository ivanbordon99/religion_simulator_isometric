#include "Humano.hpp"
#include "WorldMap.hpp"
#include "Religion.hpp"
#include "EventLine.hpp"
#include "IsometricCamera.hpp"
#include <cmath>
#include <random>
#include <algorithm>
#include <iostream>

namespace ReligionSim {

namespace {
    std::mt19937 g_humanoRng(2026);

    const std::vector<std::string> NOMBRES_GROG = {
        "Grog el Cazador", "Grog el Devoto", "Grog el Leñador", "Grog del Fuego",
        "Grog el Recolector", "Grog el Peregrino", "Grog el Nómada",
        "Grog el Constructor", "Grog el Fiel", "Grog el Anciano", "Grog el Sabio"
    };

    const std::vector<std::string> NOMBRES_CHAMAN = {
        "Chaman Arandu", "Chaman Khep", "Chaman Orph", "Chaman Mithra", "Chaman Dharma"
    };
}

Humano::Humano(int id, const std::string& nombre, float inicioX, float inicioY)
    : m_id(id), m_nombre(nombre), m_x(inicioX), m_y(inicioY),
      m_destinoX(inicioX), m_destinoY(inicioY) {
    m_celdaX = static_cast<int>(std::floor(inicioX));
    m_celdaY = static_cast<int>(std::floor(inicioY));

    // Inicializar estado demográfico y motivacional de Primera Era (FirstEraAIController)
    m_popGroup.populationCount = 18 + (id * 7) % 30;
    m_popGroup.foodSupply = 42.0f + static_cast<float>((id * 19) % 48);
    m_popGroup.divineWill = m_devocion * 100.0f;
    m_popGroup.unityWill = 58.0f + static_cast<float>((id * 13) % 36);
    m_popGroup.state = RealisticVillagerState::COMMUNAL_BONDING;
    m_popGroup.coordinates = {inicioX, inicioY};

    // Inicializar atributos sistémicos individuales de la unidad humana (HumanAIController)
    m_humanUnit.unitName = nombre;
    m_humanUnit.tribeID = "Tribu #" + std::to_string(m_religionId);
    m_humanUnit.leaderOrProphetID = "Chaman Ancestral";
    m_humanUnit.currentEra = ::EraType::PRIMITIVE_TRIBAL;
    if (id % 13 == 0) {
        m_humanUnit.role = CommunityRole::PROPHET;
        m_humanUnit.conversionPower = 32.0f;
        m_humanUnit.conversionInertia = 78.0f;
        m_humanUnit.willpower = 88.0f;
    } else if (id % 9 == 0) {
        m_humanUnit.role = CommunityRole::ZEALOT;
        m_humanUnit.conversionPower = 24.0f;
        m_humanUnit.conversionInertia = 82.0f;
        m_humanUnit.willpower = 90.0f;
    } else if (id % 7 == 0) {
        m_humanUnit.role = CommunityRole::ELDER;
        m_humanUnit.conversionPower = 18.0f;
        m_humanUnit.conversionInertia = 90.0f;
        m_humanUnit.willpower = 85.0f;
    } else {
        m_humanUnit.role = CommunityRole::FORAGER;
        m_humanUnit.conversionPower = 12.0f + static_cast<float>((id * 5) % 10);
        m_humanUnit.conversionInertia = 38.0f + static_cast<float>((id * 11) % 25);
        m_humanUnit.willpower = 65.0f + static_cast<float>((id * 7) % 20);
    }
    m_humanUnit.hitPoints = 100.0f;
    m_humanUnit.attackPower = 10.0f + static_cast<float>((id * 3) % 12);
    m_humanUnit.energy = 60.0f + static_cast<float>((id * 17) % 38);
    m_humanUnit.resistance = 22.0f + static_cast<float>((id * 9) % 28);
    m_humanUnit.devotion = m_devocion * 100.0f;
    m_humanUnit.currentState = HumanAIState::SOCIALIZING;
    m_humanUnit.position = {inicioX, inicioY};
    m_humanUnit.nutritionalState = m_popGroup.foodSupply;
}

void Humano::cambiarAccion(AccionHumano nuevaAccion) {
    m_accion = nuevaAccion;
}

void Humano::cambiarDireccion(DireccionIso nuevaDir) {
    m_direccion = nuevaDir;
}

void Humano::orientarHacia(float destX, float destY) {
    float dx = destX - m_x;
    float dy = destY - m_y;
    if (std::abs(dx) > std::abs(dy)) {
        m_direccion = (dx > 0) ? DireccionIso::SE : DireccionIso::NW;
    } else {
        m_direccion = (dy > 0) ? DireccionIso::SW : DireccionIso::NE;
    }
}

void Humano::caminarHacia(float destX, float destY, bool corriendo) {
    m_destinoX = destX;
    m_destinoY = destY;
    m_accion = corriendo ? AccionHumano::CORRER : AccionHumano::CAMINAR;
    m_velocidad = corriendo ? 1.1f : 0.65f;
}

void Humano::ordenarMovimientoManual(float destX, float destY) {
    m_destinoX = destX;
    m_destinoY = destY;
    m_ordenManual = true;
    m_accion = AccionHumano::CORRER;
    m_velocidad = 2.8f;
    m_timerEstado = 14.0f;
}

TileRect Humano::getTileRect(const TileSet& tileset) const {
    if (m_esChaman && m_timerSenalDivina > 0.0f) {
        // Alternar animación de invocación divina con el báculo resplandeciente
        bool destello = (static_cast<int>(m_animTimer * 4.5f) & 1) != 0;
        return destello ? TileRect{192, 192, 64, 64} : TileRect{64, 64, 64, 64};
    }
    return tileset.obtenerRectangulo(m_accion, m_direccion);
}

void Humano::actualizar(float dt, const WorldMap& mapa) {
    if (m_salud <= 0.0f) {
        if (m_timerMuerte > 0.0f) {
            m_timerMuerte = std::max(0.0f, m_timerMuerte - dt);
        }
        return; // La unidad ha muerto: detiene cualquier movimiento, acción o IA
    }

    m_timerEstado -= dt;
    m_animTimer += dt;
    if (m_timerSenalDivina > 0.0f) {
        m_timerSenalDivina = std::max(0.0f, m_timerSenalDivina - dt);
    }
    if (m_faithIndicatorTimer > 0.0f) {
        m_faithIndicatorTimer = std::max(0.0f, m_faithIndicatorTimer - dt);
        if (m_faithIndicatorTimer <= 0.0f) {
            m_faithIndicatorSign = 0;
        }
    }
    if (std::abs(m_faithDeltaAccum) > 0.0001f) {
        m_faithDeltaAccum *= std::max(0.0f, 1.0f - dt * 1.5f);
    }
    if (m_timerDano > 0.0f) {
        m_timerDano = std::max(0.0f, m_timerDano - dt);
    }
    // Regeneración suave de salud fuera de combate si fue herido
    if (m_timerDano <= 0.0f && m_salud < 100.0f) {
        m_salud = std::min(100.0f, m_salud + 0.40f * dt);
        m_humanUnit.hitPoints = m_salud;
    }

    if (m_accion == AccionHumano::CAMINAR || m_accion == AccionHumano::CORRER) {
        float dx = m_destinoX - m_x;
        float dy = m_destinoY - m_y;
        float dist = std::sqrt(dx * dx + dy * dy);

        if (dist > 0.05f) {
            float paso = m_velocidad * dt;
            if (paso > dist) paso = dist;

            m_x += (dx / dist) * paso;
            m_y += (dy / dist) * paso;

            // Determinar orientación isométrica:
            // En isometría: +X es SE, -X es NW, +Y es SW, -Y es NE
            if (std::abs(dx) > std::abs(dy)) {
                m_direccion = (dx > 0) ? DireccionIso::SE : DireccionIso::NW;
            } else {
                m_direccion = (dy > 0) ? DireccionIso::SW : DireccionIso::NE;
            }

            m_celdaX = static_cast<int>(std::floor(m_x));
            m_celdaY = static_cast<int>(std::floor(m_y));

            // Si la celda tiene un Templo (edificio circular central), rodear su perímetro de forma natural
            if (mapa.isValid(m_celdaX, m_celdaY) && mapa.getCell(m_celdaX, m_celdaY).hasHolySite) {
                float cx = static_cast<float>(m_celdaX) + 0.50f;
                float cy = static_cast<float>(m_celdaY) + 0.50f;
                float rx = m_x - cx;
                float ry = m_y - cy;
                float rDist = std::sqrt(rx * rx + ry * ry);
                if (rDist < 0.42f) {
                    if (rDist < 0.001f) { rx = 0.30f; ry = 0.30f; rDist = 0.424f; }
                    m_x = cx + (rx / rDist) * 0.43f;
                    m_y = cy + (ry / rDist) * 0.43f;
                }
            }
        } else {
            m_x = m_destinoX;
            m_y = m_destinoY;
            m_celdaX = static_cast<int>(std::floor(m_x));
            m_celdaY = static_cast<int>(std::floor(m_y));
            if (m_ordenManual) {
                m_ordenManual = false;
                m_accion = AccionHumano::IDLE;
                m_velocidad = 0.65f;
                m_timerEstado = 10.0f;
            } else {
                tomarDecisionIA(mapa);
            }
        }
    } else {
        if (m_timerEstado <= 0.0f) {
            tomarDecisionIA(mapa);
        }
    }
}

void Humano::tomarDecisionIA(const WorldMap& mapa) {
    std::uniform_real_distribution<float> fDist(0.0f, 1.0f);
    std::uniform_real_distribution<float> durDist(2.5f, 6.0f);
    m_timerEstado = durDist(g_humanoRng);

    int curX = static_cast<int>(std::floor(m_x));
    int curY = static_cast<int>(std::floor(m_y));

    if (!mapa.isValid(curX, curY)) {
        m_x = 20.5f; m_y = 20.5f;
        m_destinoX = 20.5f; m_destinoY = 20.5f;
        m_accion = AccionHumano::IDLE;
        return;
    }

    const Cell& celda = mapa.getCell(curX, curY);
    if (m_religionId <= 0 && celda.dominantReligionId > 0) {
        m_religionId = celda.dominantReligionId;
    }

    float roll = fDist(g_humanoRng);
    auto vecinos = mapa.getNeighbors(curX, curY, false);
    std::vector<Point2i> vecinosHabitables;
    for (const auto& nb : vecinos) {
        if (mapa.getCell(nb.x, nb.y).isHabitable()) {
            vecinosHabitables.push_back(nb);
        }
    }

    // Prioridad de comportamiento individual según HumanAIController
    if (m_humanUnit.currentState == HumanAIState::SURVIVAL_WORKING) {
        m_popGroup.foodSupply = std::min(100.0f, m_popGroup.foodSupply + 14.0f);
        m_accion = (celda.biome == BiomeType::DenseForest || roll < 0.35f)
                   ? AccionHumano::TALAR
                   : AccionHumano::RECOLECTAR;
        return;
    }
    if (m_humanUnit.currentState == HumanAIState::CONVERTING) {
        if (!vecinosHabitables.empty() && roll < 0.55f) {
            std::uniform_int_distribution<size_t> nDist(0, vecinosHabitables.size() - 1);
            Point2i dest = vecinosHabitables[nDist(g_humanoRng)];
            caminarHacia(static_cast<float>(dest.x) + 0.25f + fDist(g_humanoRng) * 0.5f,
                         static_cast<float>(dest.y) + 0.25f + fDist(g_humanoRng) * 0.5f,
                         false);
        } else {
            m_accion = (roll < 0.80f) ? AccionHumano::SALUDAR : AccionHumano::REZAR;
        }
        return;
    }
    if (m_humanUnit.currentState == HumanAIState::WORSHIPING && roll < 0.65f) {
        m_accion = AccionHumano::REZAR;
        return;
    }

    // Comportamiento autónomo según el estado de Primera Era (FirstEraAIController)
    switch (m_popGroup.state) {
        case RealisticVillagerState::SURVIVAL_FORAGING: {
            // 1. Supervivencia estricta: recolección/caza prioritaria y recuperación de alimento
            m_popGroup.foodSupply = std::min(100.0f, m_popGroup.foodSupply + 18.0f);
            if (roll < 0.65f) {
                m_accion = (celda.biome == BiomeType::DenseForest || roll < 0.30f)
                           ? AccionHumano::TALAR
                           : AccionHumano::RECOLECTAR;
            } else if (!vecinosHabitables.empty()) {
                std::uniform_int_distribution<size_t> nDist(0, vecinosHabitables.size() - 1);
                Point2i dest = vecinosHabitables[nDist(g_humanoRng)];
                caminarHacia(static_cast<float>(dest.x) + 0.25f + fDist(g_humanoRng) * 0.5f,
                             static_cast<float>(dest.y) + 0.25f + fDist(g_humanoRng) * 0.5f,
                             false);
            } else {
                m_accion = AccionHumano::RECOLECTAR;
            }
            return;
        }
        case RealisticVillagerState::CAUTIOUS_CONTACT: {
            // 2. Vigilancia y comercio silencioso en frontera con tribu vecina
            if (roll < 0.45f) {
                m_accion = AccionHumano::CARGAR; // Intercambio silencioso
            } else if (roll < 0.75f) {
                m_accion = AccionHumano::ATACAR; // Guardia cautelosa en frontera
            } else {
                m_accion = AccionHumano::IDLE;
            }
            return;
        }
        case RealisticVillagerState::AUTONOMOUS_SPLIT: {
            // 3. Escisión autónoma del clan por presión demográfica -> migrar a nueva celda
            if (!vecinosHabitables.empty()) {
                std::uniform_int_distribution<size_t> nDist(0, vecinosHabitables.size() - 1);
                Point2i dest = vecinosHabitables[nDist(g_humanoRng)];
                caminarHacia(static_cast<float>(dest.x) + 0.3f + fDist(g_humanoRng) * 0.4f,
                             static_cast<float>(dest.y) + 0.3f + fDist(g_humanoRng) * 0.4f,
                             true);
            } else {
                m_accion = AccionHumano::CORRER;
            }
            return;
        }
        case RealisticVillagerState::RURAL_WORSHIP: {
            // 4A. Prácticas chamánicas locales (Dogma y Fervor)
            if (roll < 0.72f) {
                m_accion = AccionHumano::REZAR;
            } else if (roll < 0.88f) {
                m_accion = AccionHumano::SALUDAR;
            } else {
                caminarHacia(static_cast<float>(curX) + 0.25f + fDist(g_humanoRng) * 0.5f,
                             static_cast<float>(curY) + 0.25f + fDist(g_humanoRng) * 0.5f,
                             false);
            }
            return;
        }
        case RealisticVillagerState::COMMUNAL_BONDING:
        default: {
            // 4B. Fortalecimiento de lazos sociales internos (Unidad y Solidaridad)
            if (roll < 0.40f) {
                m_accion = AccionHumano::SALUDAR;
            } else if (roll < 0.78f) {
                if (!vecinosHabitables.empty() && fDist(g_humanoRng) < 0.45f) {
                    std::uniform_int_distribution<size_t> nDist(0, vecinosHabitables.size() - 1);
                    Point2i dest = vecinosHabitables[nDist(g_humanoRng)];
                    caminarHacia(static_cast<float>(dest.x) + 0.3f + fDist(g_humanoRng) * 0.4f,
                                 static_cast<float>(dest.y) + 0.3f + fDist(g_humanoRng) * 0.4f,
                                 false);
                } else {
                    caminarHacia(static_cast<float>(curX) + 0.2f + fDist(g_humanoRng) * 0.6f,
                                 static_cast<float>(curY) + 0.2f + fDist(g_humanoRng) * 0.6f,
                                 false);
                }
            } else {
                m_accion = AccionHumano::IDLE;
            }
            return;
        }
    }
}

void Humano::renderizar(uint32_t* dstBuffer, int dstW, int dstH,
                        const IsometricCamera& camara, const WorldMap& mapa,
                        const TileSet& tileset, ColorRGB auraColor, bool seleccionado) const {
    float elev = 1.0f;
    if (mapa.isValid(m_celdaX, m_celdaY)) {
        elev = mapa.getCell(m_celdaX, m_celdaY).elevation;
    }

    Vec2 piePantalla = camara.gridToScreen(m_x, m_y, elev);

    // Comprobación de frustum
    if (piePantalla.x < -60 || piePantalla.x > dstW + 60 ||
        piePantalla.y < -60 || piePantalla.y > dstH + 60) {
        return;
    }

    TileRect rect = getTileRect(tileset);
    // Escala del humano/chamán acorde al zoom de la cámara isométrica
    float factorBase = m_esChaman ? 0.54f : 0.48f;
    float escala = std::clamp(camara.getZoom() * factorBase, 0.20f, 1.35f);

    tileset.blitTile(dstBuffer, dstW, dstH, rect, piePantalla.x, piePantalla.y, escala, auraColor, seleccionado);

    float destH = rect.height * escala;
    float headX = piePantalla.x;
    float headY = piePantalla.y - destH;

    // 0. Si la unidad ha muerto (HP <= 0), mostrar icono flotante de calavera ancestral y finalizar
    if (m_salud <= 0.0f) {
        float durMuerte = 1.6f;
        float t = 1.0f - std::clamp(m_timerMuerte / durMuerte, 0.0f, 1.0f);
        float floatOffset = t * 16.0f * std::max(0.70f, camara.getZoom());
        int cx = static_cast<int>(headX);
        int cy = static_cast<int>(headY - 6.0f * escala - floatOffset);
        float alpha = std::clamp(m_timerMuerte / 0.40f, 0.0f, 1.0f);

        // Calavera en color hueso con cuencas oculares carmesí
        ColorRGB boneCol{226, 218, 196};
        ColorRGB bloodCol{140, 20, 30};
        ColorRGB outlCol{14, 9, 8};

        for (int dy = -3; dy <= 3; ++dy) {
            int py = cy + dy;
            int fy = dstH - 1 - py;
            if (fy < 0 || fy >= dstH) continue;
            for (int dx = -3; dx <= 3; ++dx) {
                int px = cx + dx;
                if (px < 0 || px >= dstW) continue;

                bool isEye = (dy == -1 && (dx == -1 || dx == 1));
                bool isSkull = (dy == -3 && std::abs(dx) <= 1) ||
                               (dy == -2 && std::abs(dx) <= 2) ||
                               (dy == -1 && (std::abs(dx) <= 2 && !isEye)) ||
                               (dy == 0 && std::abs(dx) <= 1) ||
                               (dy == 1 && (dx == -1 || dx == 1));
                bool isOutline = !isSkull && !isEye &&
                                ((std::abs(dx) <= 2 && std::abs(dy) <= 3) ||
                                 (std::abs(dx) <= 3 && std::abs(dy) <= 2));

                if (isEye || isSkull || isOutline) {
                    ColorRGB col = isEye ? bloodCol : (isSkull ? boneCol : outlCol);
                    uint32_t bg = dstBuffer[fy * dstW + px];
                    uint8_t br = bg & 0xFF;
                    uint8_t bgc = (bg >> 8) & 0xFF;
                    uint8_t bb = (bg >> 16) & 0xFF;

                    uint8_t outR = static_cast<uint8_t>(br * (1.0f - alpha) + col.r * alpha);
                    uint8_t outG = static_cast<uint8_t>(bgc * (1.0f - alpha) + col.g * alpha);
                    uint8_t outB = static_cast<uint8_t>(bb * (1.0f - alpha) + col.b * alpha);
                    dstBuffer[fy * dstW + px] = (0xFFu << 24) | (outB << 16) | (outG << 8) | outR;
                }
            }
        }
        return; // La unidad caída no dibuja barra de HP ni indicador de fe
    }

    // 1. Barra de Vida (HP) y destello de daño cuando la unidad está herida, atacada o seleccionada
    bool mostrarBarraHP = (m_salud < 99.5f || m_timerDano > 0.0f || seleccionado);
    if (mostrarBarraHP) {
        int barW = std::clamp(static_cast<int>(20.0f * std::max(0.70f, camara.getZoom())), 14, 26);
        int barH = 3;
        int barX = static_cast<int>(headX - barW * 0.5f);
        int barY = static_cast<int>(headY - 4.0f * escala);

        // Borde oscuro exterior
        for (int by = barY - 1; by <= barY + barH; ++by) {
            int fy = dstH - 1 - by;
            if (fy < 0 || fy >= dstH) continue;
            for (int bx = barX - 1; bx <= barX + barW; ++bx) {
                if (bx < 0 || bx >= dstW) continue;
                bool esBorde = (by == barY - 1 || by == barY + barH || bx == barX - 1 || bx == barX + barW);
                if (esBorde) {
                    dstBuffer[fy * dstW + bx] = (0xFFu << 24) | (8u << 16) | (8u << 8) | 12u; // ColorRGB{12, 8, 8}
                } else {
                    dstBuffer[fy * dstW + bx] = (0xFFu << 24) | (14u << 16) | (14u << 8) | 30u; // Fondo oscuro #1e0e0e
                }
            }
        }

        // Relleno proporcional de vida
        float hpFrac = std::clamp(m_salud / 100.0f, 0.0f, 1.0f);
        int fillW = static_cast<int>(std::round(barW * hpFrac));
        ColorRGB colHP{46, 215, 80}; // Verde saludable
        if (m_timerDano > 0.0f && (static_cast<int>(m_timerDano * 18.0f) & 1)) {
            colHP = ColorRGB{255, 150, 150}; // Destello de impacto
        } else if (hpFrac <= 0.28f) {
            colHP = ColorRGB{230, 45, 45};  // Rojo crítico
        } else if (hpFrac <= 0.55f) {
            colHP = ColorRGB{235, 185, 45}; // Amarillo advertencia
        }
        uint32_t fillRGBA = colHP.toRGBA();

        for (int by = barY; by < barY + barH; ++by) {
            int fy = dstH - 1 - by;
            if (fy < 0 || fy >= dstH) continue;
            for (int bx = barX; bx < barX + fillW; ++bx) {
                if (bx >= 0 && bx < dstW) {
                    dstBuffer[fy * dstW + bx] = fillRGBA;
                }
            }
        }
    }

    // 2. Chispas de impacto cuando la unidad es atacada
    if (m_timerDano > 0.0f) {
        int sparkBaseX = static_cast<int>(headX);
        int sparkBaseY = static_cast<int>(piePantalla.y - 28.0f * escala);
        int sparkOffsets[4][2] = {{-3, -2}, {2, 3}, {-4, 1}, {3, -3}};
        for (int i = 0; i < 4; ++i) {
            int sx = sparkBaseX + sparkOffsets[i][0];
            int sy = sparkBaseY + sparkOffsets[i][1];
            int fy = dstH - 1 - sy;
            if (sx >= 0 && sx < dstW && fy >= 0 && fy < dstH) {
                ColorRGB sCol = (i & 1) ? ColorRGB{255, 230, 120} : ColorRGB{245, 60, 40};
                dstBuffer[fy * dstW + sx] = sCol.toRGBA();
            }
        }
    }

    // 3. Indicador flotante "+" de cambio de Fe (Verde cuando aumenta, Rojo cuando disminuye)
    if (m_faithIndicatorTimer > 0.0f && m_faithIndicatorSign != 0) {
        float durTotal = 1.35f;
        float t = 1.0f - std::clamp(m_faithIndicatorTimer / durTotal, 0.0f, 1.0f);
        float floatOffset = t * 16.0f * std::max(0.70f, camara.getZoom());
        int cx = static_cast<int>(headX);
        int cy = static_cast<int>(headY - (mostrarBarraHP ? 10.0f : 5.0f) * escala - floatOffset);
        float alpha = std::clamp(m_faithIndicatorTimer / 0.35f, 0.0f, 1.0f);

        // '+' verde intenso cuando aumenta la fe, '+' rojo intenso cuando se pierde
        ColorRGB coreCol = (m_faithIndicatorSign > 0) ? ColorRGB{46, 235, 96} : ColorRGB{235, 48, 48};
        ColorRGB outlCol{14, 9, 8};

        // Dibujar cruz de 5px con contorno oscuro de 1px
        for (int dy = -3; dy <= 3; ++dy) {
            int py = cy + dy;
            int fy = dstH - 1 - py;
            if (fy < 0 || fy >= dstH) continue;

            for (int dx = -3; dx <= 3; ++dx) {
                int px = cx + dx;
                if (px < 0 || px >= dstW) continue;

                // Núcleo del símbolo '+': cruz de 5px vertical y 5px horizontal
                bool isCore = (dx == 0 && std::abs(dy) <= 2) || (dy == 0 && std::abs(dx) <= 2);
                // Borde/sombra exterior de 1px que rodea a la cruz
                bool isOutline = !isCore && ((std::abs(dx) <= 1 && std::abs(dy) <= 3) ||
                                            (std::abs(dy) <= 1 && std::abs(dx) <= 3));

                if (isCore || isOutline) {
                    ColorRGB col = isCore ? coreCol : outlCol;
                    uint32_t bg = dstBuffer[fy * dstW + px];
                    uint8_t br = bg & 0xFF;
                    uint8_t bgc = (bg >> 8) & 0xFF;
                    uint8_t bb = (bg >> 16) & 0xFF;

                    uint8_t outR = static_cast<uint8_t>(br * (1.0f - alpha) + col.r * alpha);
                    uint8_t outG = static_cast<uint8_t>(bgc * (1.0f - alpha) + col.g * alpha);
                    uint8_t outB = static_cast<uint8_t>(bb * (1.0f - alpha) + col.b * alpha);
                    dstBuffer[fy * dstW + px] = (0xFFu << 24) | (outB << 16) | (outG << 8) | outR;
                }
            }
        }
    }
}

// --- Gestor de Humanos ---
GestorHumanos::GestorHumanos() {
    m_humanos.reserve(128);
}

void GestorHumanos::inicializarPoblacion(WorldMap& mapa, const ReligionRegistry& registry, int cantidad) {
    m_humanos.clear();
    int w = mapa.getWidth();
    int h = mapa.getHeight();

    // Capturar santuarios iniciales antes de limpiar las zonas de influencia vacías
    struct HearthNode {
        int x;
        int y;
        int religionId;
    };
    std::vector<HearthNode> hearths;
    for (int cy = 0; cy < h; ++cy) {
        for (int cx = 0; cx < w; ++cx) {
            const Cell& c = mapa.getCell(cx, cy);
            if (c.hasHolySite) {
                int hRel = (c.dominantReligionId > 0) ? c.dominantReligionId : c.holySiteReligionId;
                if (hRel > 0) {
                    hearths.push_back({cx, cy, hRel});
                }
            }
        }
    }

    // Limpiar población y zonas de influencia en TODAS las celdas:
    // Las zonas de influencia sólo existirán donde haya aldeanos reales (nodos de fe).
    for (int cy = 0; cy < h; ++cy) {
        for (int cx = 0; cx < w; ++cx) {
            Cell& c = mapa.getCell(cx, cy);
            c.population = 0;
            c.faithShare.clear();
            c.dominantReligionId = 0;
            c.dominantFraction = 0.0f;
            c.syncreticTension = 0.0f;
        }
    }

    std::vector<int> activeRelIds;
    for (const auto& r : registry.getAllReligions()) {
        if (!r.isExtinct) {
            activeRelIds.push_back(r.id);
        }
    }
    if (activeRelIds.empty()) {
        activeRelIds.push_back(1);
    }

    std::uniform_int_distribution<int> rx(2, w - 3);
    std::uniform_int_distribution<int> ry(2, h - 3);
    std::uniform_int_distribution<size_t> tDist(0, NOMBRES_GROG.size() - 1);
    std::uniform_real_distribution<float> subOffset(0.2f, 0.8f);
    std::uniform_real_distribution<float> pDist(0.60f, 0.95f);

    int creados = 0;

    // 1. Sembrar primero a cada pueblo alrededor de su santuario, con su CHAMÁN INICIAL (k == 0)
    if (!hearths.empty()) {
        std::uniform_int_distribution<int> offsetDist(-2, 2);
        int porSantuario = std::max(2, cantidad / static_cast<int>(hearths.size() * 2));
        size_t hearthIdx = 0;
        for (const auto& hearth : hearths) {
            for (int k = 0; k < porSantuario && creados < cantidad; ++k) {
                int tx = (k == 0) ? hearth.x : std::clamp(hearth.x + offsetDist(g_humanoRng), 2, w - 3);
                int ty = (k == 0) ? hearth.y : std::clamp(hearth.y + offsetDist(g_humanoRng), 2, h - 3);
                Cell& c = mapa.getCell(tx, ty);
                if (!c.isHabitable()) {
                    tx = hearth.x;
                    ty = hearth.y;
                }
                Cell& validCell = mapa.getCell(tx, ty);
                if (validCell.isHabitable()) {
                    float posX = static_cast<float>(tx) + subOffset(g_humanoRng);
                    float posY = static_cast<float>(ty) + subOffset(g_humanoRng);
                    if (validCell.hasHolySite) {
                        // Ubicar al Chamán inicial en el atrio frontal del templo (0.82, 0.82)
                        posX = static_cast<float>(tx) + ((k % 2 == 0) ? 0.82f : 0.14f);
                        posY = static_cast<float>(ty) + ((k % 2 == 0) ? 0.82f : 0.14f);
                    }
                    bool esChamanInicial = (k == 0);
                    std::string unitName = esChamanInicial
                        ? NOMBRES_CHAMAN[hearthIdx % NOMBRES_CHAMAN.size()]
                        : NOMBRES_GROG[tDist(g_humanoRng)];

                    Humano hObj(m_siguienteId++, unitName, posX, posY);
                    hObj.setEsChaman(esChamanInicial);
                    hObj.setReligionId(hearth.religionId);
                    hObj.setDevocion(esChamanInicial ? 0.98f : pDist(g_humanoRng));
                    hObj.cambiarAccion(AccionHumano::IDLE);
                    hObj.cambiarDireccion(DireccionIso::SW);

                    m_humanos.push_back(hObj);
                    if (esChamanInicial && hearth.religionId == 1 && m_seleccionadoId < 0) {
                        m_seleccionadoId = hObj.getId();
                    }

                    validCell.population++;
                    validCell.addShare(hearth.religionId, hObj.getDevocion());
                    validCell.normalizeShares();
                    validCell.updateDominance();
                    creados++;
                }
            }
            hearthIdx++;
        }
    }

    // 2. Sembrar el resto de aldeanos como nodos de fe en celdas habitables
    int intentos = 0;
    int maxIntentos = cantidad * 40;
    while (creados < cantidad && intentos < maxIntentos) {
        intentos++;
        int x = rx(g_humanoRng);
        int y = ry(g_humanoRng);
        Cell& c = mapa.getCell(x, y);

        if (c.isHabitable()) {
            float posX = static_cast<float>(x) + subOffset(g_humanoRng);
            float posY = static_cast<float>(y) + subOffset(g_humanoRng);
            Humano hObj(m_siguienteId++, NOMBRES_GROG[tDist(g_humanoRng)], posX, posY);

            // Si hay santuarios, asignar la religión del santuario más cercano; si no, rotar entre religiones activas
            int rId = activeRelIds[creados % activeRelIds.size()];
            if (!hearths.empty()) {
                int bestDistSq = 1000000;
                for (const auto& hearth : hearths) {
                    int dx = x - hearth.x;
                    int dy = y - hearth.y;
                    int dsq = dx * dx + dy * dy;
                    if (dsq < bestDistSq) {
                        bestDistSq = dsq;
                        rId = hearth.religionId;
                    }
                }
            }

            hObj.setReligionId(rId);
            hObj.setDevocion(pDist(g_humanoRng));
            if (buscarChamanDeReligion(rId) < 0) {
                hObj.setEsChaman(true);
                hObj.setNombre(NOMBRES_CHAMAN[static_cast<size_t>( std::max(0, rId - 1) ) % NOMBRES_CHAMAN.size()]);
                hObj.setDevocion(0.98f);
                if (rId == 1 && m_seleccionadoId < 0) {
                    m_seleccionadoId = hObj.getId();
                }
            }
            hObj.cambiarAccion(AccionHumano::IDLE);
            hObj.cambiarDireccion(DireccionIso::SW);

            m_humanos.push_back(hObj);
            c.population++;
            c.addShare(rId, hObj.getDevocion());
            c.normalizeShares();
            c.updateDominance();
            creados++;
        }
    }
}

void GestorHumanos::actualizar(float dt, WorldMap& mapa) {
    int w = mapa.getWidth();
    int h = mapa.getHeight();

    // 1. Movimiento, IA de Primera Era (FirstEraAIController) e interacción nodo-a-nodo
    if (dt > 0.0f) {
        for (auto& hObj : m_humanos) {
            int cx = std::clamp(hObj.getCeldaX(), 0, w - 1);
            int cy = std::clamp(hObj.getCeldaY(), 0, h - 1);
            const Cell& curCell = mapa.getCell(cx, cy);

            int ownClanCount = 0;
            int rivalTribeCount = 0;
            bool shrineNearby = curCell.hasHolySite;
            float externalConversionPressure = 0.0f;

            for (const auto& other : m_humanos) {
                float dx = other.getX() - hObj.getX();
                float dy = other.getY() - hObj.getY();
                float dsq = dx * dx + dy * dy;
                if (dsq <= 12.25f) {
                    if (other.getReligionId() == hObj.getReligionId()) {
                        ownClanCount++;
                        if (other.esChaman()) shrineNearby = true;
                    } else if (other.getReligionId() > 0) {
                        rivalTribeCount++;
                        float distFactor = 1.0f / (1.0f + std::sqrt(dsq) * 0.45f);
                        externalConversionPressure += other.getHumanUnit().conversionPower * distFactor;
                    }
                }
            }

            if (!shrineNearby) {
                for (int oy = -2; oy <= 2 && !shrineNearby; ++oy) {
                    for (int ox = -2; ox <= 2 && !shrineNearby; ++ox) {
                        if (mapa.isValid(cx + ox, cy + oy) && mapa.getCell(cx + ox, cy + oy).hasHolySite) {
                            shrineNearby = true;
                        }
                    }
                }
            }

            RealisticCellContext context;
            context.hasNeighborTribe = (rivalTribeCount > 0);
            context.neighborTribePowerRatio = (ownClanCount > 0)
                ? std::clamp(static_cast<float>(rivalTribeCount) / static_cast<float>(ownClanCount), 0.1f, 2.0f)
                : 1.0f;
            context.currentBiome = curCell.biome;
            switch (curCell.biome) {
                case BiomeType::FertileValley:
                    context.environmentalCarryingCapacity = 100.0f;
                    break;
                case BiomeType::CoastalPlains:
                    context.environmentalCarryingCapacity = 85.0f;
                    break;
                case BiomeType::DenseForest:
                    context.environmentalCarryingCapacity = 75.0f;
                    break;
                case BiomeType::HighlandHills:
                    context.environmentalCarryingCapacity = 60.0f;
                    break;
                case BiomeType::AridDesert:
                    context.environmentalCarryingCapacity = 30.0f;
                    break;
                case BiomeType::RuggedMountains:
                    context.environmentalCarryingCapacity = 20.0f;
                    break;
                case BiomeType::FrozenTundra:
                    context.environmentalCarryingCapacity = 40.0f;
                    break;
                case BiomeType::DeepOcean:
                case BiomeType::ShallowWater:
                    context.environmentalCarryingCapacity = 0.0f;
                    break;
            }
            context.hasShrine = shrineNearby;

            RealisticPopGroup& pop = hObj.getPopGroupMutable();
            pop.coordinates = {hObj.getX(), hObj.getY()};
            pop.populationCount = std::max(pop.populationCount, ownClanCount * 4);
            if (pop.state != RealisticVillagerState::SURVIVAL_FORAGING) {
                pop.foodSupply = std::max(0.0f, pop.foodSupply - 0.75f * dt);
            }

            m_firstEraAI.UpdateRealisticBehavior(pop, context, dt);

            // Actualizar IA individual de la unidad humana (HumanAIController)
            UnitEnvironmentContext unitEnv;
            unitEnv.hasNearbyShrine = shrineNearby;
            unitEnv.isBorderOrConflictZone = (rivalTribeCount > 0);
            unitEnv.localDangerLevel = std::clamp(static_cast<float>(rivalTribeCount) * 12.0f, 0.0f, 100.0f);
            unitEnv.externalConversionPressure = externalConversionPressure;

            HumanUnit& hu = hObj.getHumanUnitMutable();
            hu.position = {hObj.getX(), hObj.getY()};
            hu.tribeID = "Tribu #" + std::to_string(hObj.getReligionId());
            if (hu.currentState != HumanAIState::SURVIVAL_WORKING) {
                hu.nutritionalState = std::max(0.0f, hu.nutritionalState - 0.65f * dt);
            }
            m_humanAI.UpdateHumanAI(hu, unitEnv, dt);
            hu.nutritionalState = std::clamp(hu.nutritionalState, 0.0f, 100.0f);
            pop.foodSupply = std::clamp(0.5f * (pop.foodSupply + hu.nutritionalState), 0.0f, 100.0f);
            pop.divineWill = std::clamp(0.5f * (pop.divineWill + hu.devotion), 0.0f, 100.0f);

            if (!hObj.esChaman()) {
                hObj.setDevocion(std::clamp(hu.devotion / 100.0f, 0.15f, 1.0f));
            } else {
                hObj.setDevocion(std::clamp(hu.devotion / 100.0f, 0.20f, 1.0f));
            }

            hObj.actualizar(dt, mapa);
        }

        // 2. Combate cuerpo a cuerpo, defensa de la fe y conversión espiritual directa
        const size_t n = m_humanos.size();
        for (size_t i = 0; i < n; ++i) {
            for (size_t j = i + 1; j < n; ++j) {
                Humano& a = m_humanos[i];
                Humano& b = m_humanos[j];
                if (a.getReligionId() == b.getReligionId()) continue;

                float dx = a.getX() - b.getX();
                float dy = a.getY() - b.getY();
                float dsq = dx * dx + dy * dy;

                // A. Combate físico y escaramuzas entre tribus rivales cercanas (alcance <= 3.24, aprox. 1.8 celdas)
                if (dsq <= 3.24f) {
                    // Los fanáticos (ZEALOT) inician ataque al encontrarse con fieles de otra fe
                    if (a.getRole() == CommunityRole::ZEALOT && a.getAccion() != AccionHumano::ATACAR) {
                        a.cambiarAccion(AccionHumano::ATACAR);
                    }
                    if (b.getRole() == CommunityRole::ZEALOT && b.getAccion() != AccionHumano::ATACAR) {
                        b.cambiarAccion(AccionHumano::ATACAR);
                    }

                    // Si A está en postura de ataque, daña a B y reduce sus puntos de vida
                    if (a.getAccion() == AccionHumano::ATACAR && dsq <= 2.56f) {
                        a.orientarHacia(b.getX(), b.getY());
                        float atkPowerA = a.getHumanUnit().attackPower;
                        float resB = b.getHumanUnit().resistance;
                        float baseDmgA = std::max(4.5f, atkPowerA * 0.70f - resB * 0.14f);
                        b.recibirDano(baseDmgA * dt);

                        // B reacciona: si tiene voluntad y salud se defiende atacando, si no huye
                        if (b.getAccion() != AccionHumano::ATACAR && b.getAccion() != AccionHumano::CORRER) {
                            if (b.getSalud() > 25.0f && b.getHumanUnit().willpower >= 35.0f) {
                                b.cambiarAccion(AccionHumano::ATACAR);
                                b.orientarHacia(a.getX(), a.getY());
                            } else {
                                b.caminarHacia(b.getX() - dx * 1.5f, b.getY() - dy * 1.5f, true);
                            }
                        }
                    }

                    // Si B está en postura de ataque, daña a A y reduce sus puntos de vida
                    if (b.getAccion() == AccionHumano::ATACAR && dsq <= 2.56f) {
                        b.orientarHacia(a.getX(), a.getY());
                        float atkPowerB = b.getHumanUnit().attackPower;
                        float resA = a.getHumanUnit().resistance;
                        float baseDmgB = std::max(4.5f, atkPowerB * 0.70f - resA * 0.14f);
                        a.recibirDano(baseDmgB * dt);

                        // A reacciona: si tiene voluntad y salud se defiende atacando, si no huye
                        if (a.getAccion() != AccionHumano::ATACAR && a.getAccion() != AccionHumano::CORRER) {
                            if (a.getSalud() > 25.0f && a.getHumanUnit().willpower >= 35.0f) {
                                a.cambiarAccion(AccionHumano::ATACAR);
                                a.orientarHacia(b.getX(), b.getY());
                            } else {
                                a.caminarHacia(a.getX() + dx * 1.5f, a.getY() + dy * 1.5f, true);
                            }
                        }
                    }

                    // Si la salud de una unidad llega a cero en combate, la unidad muere
                    if (b.getSalud() <= 0.0f) {
                        if (b.esChaman()) {
                            int rId = b.getReligionId();
                            for (auto& cand : m_humanos) {
                                if (cand.getReligionId() == rId && cand.getSalud() > 0.0f && !cand.esChaman()) {
                                    cand.setEsChaman(true);
                                    cand.setNombre("Chaman " + cand.getNombre());
                                    cand.triggerFaithIndicator(+1, 2.0f);
                                    break;
                                }
                            }
                        }
                    }
                    if (a.getSalud() <= 0.0f) {
                        if (a.esChaman()) {
                            int rId = a.getReligionId();
                            for (auto& cand : m_humanos) {
                                if (cand.getReligionId() == rId && cand.getSalud() > 0.0f && !cand.esChaman()) {
                                    cand.setEsChaman(true);
                                    cand.setNombre("Chaman " + cand.getNombre());
                                    cand.triggerFaithIndicator(+1, 2.0f);
                                    break;
                                }
                            }
                        }
                    }
                }

                // B. Conversión e influencia espiritual pacífica directa (sólo entre vivos)
                if (a.estaVivo() && b.estaVivo()) {
                    float radioMax = (a.esChaman() || b.esChaman()) ? 2.6f : 1.8f;
                    if (dsq <= radioMax * radioMax) {
                        float multA = a.esChaman() ? (a.getTimerSenalDivina() > 0.0f ? 2.6f : 1.75f)
                                                   : (a.getAccion() == AccionHumano::REZAR ? 1.45f : 1.0f);
                        float multB = b.esChaman() ? (b.getTimerSenalDivina() > 0.0f ? 2.6f : 1.75f)
                                                   : (b.getAccion() == AccionHumano::REZAR ? 1.45f : 1.0f);
                        float convFactorA = 0.7f + (a.getHumanUnit().conversionPower / 40.0f);
                        float convFactorB = 0.7f + (b.getHumanUnit().conversionPower / 40.0f);
                        float pesoA = a.getDevocion() * multA * convFactorA;
                        float pesoB = b.getDevocion() * multB * convFactorB;

                        if (pesoA >= pesoB && !b.esChaman()) {
                            float resistB = std::clamp(b.getHumanUnit().conversionInertia / 120.0f, 0.15f, 0.75f);
                            float nuevaDevB = b.getDevocion() - 0.16f * dt * pesoA * (1.0f - resistB);
                            if (nuevaDevB < 0.35f) {
                                b.setReligionId(a.getReligionId());
                                b.setDevocion(0.68f);
                                b.triggerFaithIndicator(+1, 1.5f);
                            } else {
                                b.setDevocion(nuevaDevB);
                            }
                        } else if (pesoB > pesoA && !a.esChaman()) {
                            float resistA = std::clamp(a.getHumanUnit().conversionInertia / 120.0f, 0.15f, 0.75f);
                            float nuevaDevA = a.getDevocion() - 0.16f * dt * pesoB * (1.0f - resistA);
                            if (nuevaDevA < 0.35f) {
                                a.setReligionId(b.getReligionId());
                                a.setDevocion(0.68f);
                                a.triggerFaithIndicator(+1, 1.5f);
                            } else {
                                a.setDevocion(nuevaDevA);
                            }
                        }
                    }
                }
            }
        }

        // 3. Retirar del mundo a las unidades fallecidas cuya animación de muerte haya concluido
        m_humanos.erase(
            std::remove_if(m_humanos.begin(), m_humanos.end(), [this](const Humano& h) {
                if (h.getSalud() <= 0.0f && h.getTimerMuerte() <= 0.0f) {
                    if (h.getId() == m_seleccionadoId) {
                        m_seleccionadoId = -1;
                    }
                    return true;
                }
                return false;
            }),
            m_humanos.end()
        );

        // 4. Renovación generacional y nacimientos en clanes prósperos con alimento abundante
        if (m_humanos.size() < 100 && !m_humanos.empty()) {
            m_birthAccumulator += dt;
            if (m_birthAccumulator >= 8.0f) {
                m_birthAccumulator = 0.0f;
                for (const auto& p : m_humanos) {
                    if (p.estaVivo() && p.getSalud() > 70.0f && p.getPopGroup().foodSupply > 60.0f) {
                        std::uniform_real_distribution<float> fDist(0.0f, 1.0f);
                        int nid = m_siguienteId++;
                        float nx = p.getX() + (fDist(g_humanoRng) - 0.5f) * 1.2f;
                        float ny = p.getY() + (fDist(g_humanoRng) - 0.5f) * 1.2f;
                        Humano bebe(nid, "Grog el Joven #" + std::to_string(nid), nx, ny);
                        bebe.setReligionId(p.getReligionId());
                        bebe.setDevocion(p.getDevocion());
                        bebe.triggerFaithIndicator(+1, 1.8f);
                        m_humanos.push_back(bebe);
                        break;
                    }
                }
            }
        }
    }

    // 2. Limpiar todas las celdas: las zonas de influencia sólo existen donde hay aldeanos
    for (int cy = 0; cy < h; ++cy) {
        for (int cx = 0; cx < w; ++cx) {
            Cell& c = mapa.getCell(cx, cy);
            c.population = 0;
            c.faithShare.clear();
            c.dominantReligionId = 0;
            c.dominantFraction = 0.0f;
            c.syncreticTension = 0.0f;
        }
    }

    // 3. Proyectar la fe exclusivamente de los aldeanos vivos en la celda que ocupan
    for (const auto& hObj : m_humanos) {
        if (!hObj.estaVivo()) continue;
        int cx = hObj.getCeldaX();
        int cy = hObj.getCeldaY();
        if (mapa.isValid(cx, cy)) {
            Cell& c = mapa.getCell(cx, cy);
            c.population++;
            if (c.isHabitable() && hObj.getReligionId() > 0) {
                c.addShare(hObj.getReligionId(), std::max(0.25f, hObj.getDevocion()));
            }
        }
    }

    // 4. Normalizar participación y dominancia sólo en celdas con aldeanos
    for (int cy = 0; cy < h; ++cy) {
        for (int cx = 0; cx < w; ++cx) {
            Cell& c = mapa.getCell(cx, cy);
            if (c.population > 0) {
                c.normalizeShares();
                c.updateDominance();
                if (c.faithShare.size() >= 2) {
                    c.syncreticTension = 0.65f;
                }
            }
        }
    }
}

void GestorHumanos::renderizarTodos(uint32_t* dstBuffer, int dstW, int dstH,
                                   const IsometricCamera& camara, const WorldMap& mapa,
                                   const TileSet& tileset, const ReligionRegistry& registry) const {
    // Ordenar humanos por profundidad isométrica (X + Y)
    std::vector<const Humano*> ordenados;
    ordenados.reserve(m_humanos.size());
    for (const auto& h : m_humanos) {
        ordenados.push_back(&h);
    }

    std::sort(ordenados.begin(), ordenados.end(), [](const Humano* a, const Humano* b) {
        return (a->getX() + a->getY()) < (b->getX() + b->getY());
    });

    for (const auto* h : ordenados) {
        ColorRGB aura(255, 215, 0);
        const Religion* rel = registry.getReligion(h->getReligionId());
        if (rel) aura = rel->color;

        bool esSeleccionado = (h->getId() == m_seleccionadoId);
        h->renderizar(dstBuffer, dstW, dstH, camara, mapa, tileset, aura, esSeleccionado);
    }
}

void GestorHumanos::agregarHumano(const Humano& h) {
    m_humanos.push_back(h);
}

const Humano* GestorHumanos::getHumanoSeleccionado() const {
    if (m_seleccionadoId < 0) return nullptr;
    for (const auto& h : m_humanos) {
        if (h.getId() == m_seleccionadoId) return &h;
    }
    return nullptr;
}

Humano* GestorHumanos::getHumanoSeleccionadoMutable() {
    if (m_seleccionadoId < 0) return nullptr;
    for (auto& h : m_humanos) {
        if (h.getId() == m_seleccionadoId) return &h;
    }
    return nullptr;
}

void GestorHumanos::seleccionarHumano(int id) {
    m_seleccionadoId = id;
}

int GestorHumanos::buscarChamanDeReligion(int religionId) const {
    for (const auto& h : m_humanos) {
        if (h.esChaman() && h.getReligionId() == religionId) {
            return h.getId();
        }
    }
    return -1;
}

void GestorHumanos::ejecutarSenalDivina(int tipoSenal, WorldMap& mapa, ReligionRegistry& registry,
                                        EventLine& eventLine, int currentYear) {
    Humano* chaman = getHumanoSeleccionadoMutable();
    if (!chaman || !chaman->esChaman()) return;

    int relId = chaman->getReligionId();
    Religion* rel = registry.getReligion(relId);
    ColorRGB rCol = rel ? rel->color : Palette::DIVINE_GOLD;
    std::string relName = rel ? rel->name : "Fe Ancestral";

    chaman->activarSenalDivina(6.0f);

    if (tipoSenal == 1) {
        // 1. REVELAR VISIÓN: Comunión profética con el Chamán, exalta devoción de su pueblo
        for (auto& h : m_humanos) {
            if (h.getReligionId() == relId) {
                h.setDevocion(std::min(1.0f, h.getDevocion() + 0.25f));
                if (&h != chaman) {
                    h.cambiarAccion(AccionHumano::REZAR);
                }
            }
        }
        if (rel) rel->theologyPoints += 80;
        eventLine.addEvent(
            currentYear, EventCategory::PilgrimageWave,
            "Vision a " + chaman->getNombre(),
            "Te has comunicado con " + chaman->getNombre() + ". El pueblo de " + relName + " entra en fervor mistico.",
            {chaman->getCeldaX(), chaman->getCeldaY()}, relId, 0, rCol
        );
    } else if (tipoSenal == 2) {
        // 2. LLAMAR PUEBLO: Congrega a los fieles alrededor del Chamán para cerrar la triangulación de fe
        int idx = 0;
        for (auto& h : m_humanos) {
            if (&h == chaman) continue;
            float dx = h.getX() - chaman->getX();
            float dy = h.getY() - chaman->getY();
            float dsq = dx * dx + dy * dy;
            if (h.getReligionId() == relId || dsq <= 36.0f) {
                float ang = static_cast<float>(idx++) * 1.15f;
                float rad = 1.35f + static_cast<float>(idx % 3) * 0.65f;
                int tx = std::clamp(static_cast<int>(std::round(chaman->getX() + std::cos(ang) * rad)), 2, mapa.getWidth() - 3);
                int ty = std::clamp(static_cast<int>(std::round(chaman->getY() + std::sin(ang) * rad)), 2, mapa.getHeight() - 3);
                if (mapa.getCell(tx, ty).isHabitable()) {
                    h.caminarHacia(static_cast<float>(tx) + 0.5f, static_cast<float>(ty) + 0.5f, true);
                }
            }
        }
        eventLine.addEvent(
            currentYear, EventCategory::PilgrimageWave,
            "Llamado de " + chaman->getNombre(),
            chaman->getNombre() + " transmite tu senal divina convocando a los nodos de " + relName + ".",
            {chaman->getCeldaX(), chaman->getCeldaY()}, relId, 0, rCol
        );
    } else if (tipoSenal == 3) {
        // 3. MILAGRO DE FE: Pulso divino que convierte aldeanos cercanos al credo del Chamán
        chaman->cambiarAccion(AccionHumano::REZAR);
        int convertidos = 0;
        for (auto& h : m_humanos) {
            if (&h == chaman || h.esChaman()) continue;
            float dx = h.getX() - chaman->getX();
            float dy = h.getY() - chaman->getY();
            if (dx * dx + dy * dy <= 32.0f) { // Radio ~ 5.6 celdas
                if (h.getReligionId() != relId) {
                    h.setReligionId(relId);
                    convertidos++;
                }
                h.setDevocion(0.95f);
                h.cambiarAccion(AccionHumano::SALUDAR);
            }
        }
        if (rel) rel->theologyPoints += 120;
        eventLine.addEvent(
            currentYear, EventCategory::BirthOfReligion,
            "Milagro de " + chaman->getNombre(),
            "Una senal divina a traves de " + chaman->getNombre() + " ilumino la tierra (" +
            std::to_string(convertidos) + " nodos convertidos).",
            {chaman->getCeldaX(), chaman->getCeldaY()}, relId, 0, rCol
        );
    }

    actualizar(0.0f, mapa);
    mapa.recalculateReligions(registry);
}

int GestorHumanos::buscarHumanoCercano(float worldX, float worldY, float radioMax, int soloReligionId) const {
    int mejorId = -1;
    float mejorDistSq = radioMax * radioMax;

    for (const auto& h : m_humanos) {
        if (soloReligionId > 0 && h.getReligionId() != soloReligionId) continue;
        float dx = h.getX() - worldX;
        float dy = h.getY() - worldY;
        float dsq = dx * dx + dy * dy;
        if (dsq < mejorDistSq) {
            mejorDistSq = dsq;
            mejorId = h.getId();
        }
    }
    return mejorId;
}

int GestorHumanos::buscarHumanoEnPantalla(float screenX, float screenY,
                                          const IsometricCamera& camara, const WorldMap& mapa,
                                          int soloReligionId) const {
    int mejorId = -1;
    float mejorDistSq = 1e9f;

    for (const auto& h : m_humanos) {
        if (soloReligionId > 0 && h.getReligionId() != soloReligionId) continue;
        float factorBase = h.esChaman() ? 0.54f : 0.48f;
        float escala = std::clamp(camara.getZoom() * factorBase, 0.20f, 1.35f);
        float destW = 64.0f * escala;
        float destH = 64.0f * escala;

        float elev = 1.0f;
        if (mapa.isValid(h.getCeldaX(), h.getCeldaY())) {
            elev = mapa.getCell(h.getCeldaX(), h.getCeldaY()).elevation;
        }
        Vec2 pie = camara.gridToScreen(h.getX(), h.getY(), elev);

        float minX = pie.x - destW * 0.5f - 8.0f;
        float maxX = pie.x + destW * 0.5f + 8.0f;
        float minY = pie.y - destH - 8.0f;
        float maxY = pie.y + 8.0f;

        if (screenX >= minX && screenX <= maxX && screenY >= minY && screenY <= maxY) {
            float cx = pie.x;
            float cy = pie.y - destH * 0.5f;
            float dx = screenX - cx;
            float dy = screenY - cy;
            float dsq = dx * dx + dy * dy;
            if (dsq < mejorDistSq) {
                mejorDistSq = dsq;
                mejorId = h.getId();
            }
        }
    }
    return mejorId;
}

} // namespace ReligionSim
