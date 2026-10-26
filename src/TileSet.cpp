#include "TileSet.hpp"
#include <fstream>
#include <iostream>
#include <algorithm>
#include <cmath>

namespace ReligionSim {

TileSet::TileSet() {
    inicializarMapaRectangulos();
    m_pixels.resize(m_width * m_height, 0);

    // Intentar cargar la nueva hoja de sprites del aldeano masculino (256x256, 4x4)
    if (!cargarDesdeArchivo("assets/sprites/units/humano_male/humano_male_sheet.rgba")) {
        if (!cargarDesdeArchivo("../assets/sprites/units/humano_male/humano_male_sheet.rgba")) {
            if (!cargarDesdeArchivo("/home/Ivancho/Documentos/religion_simulator_isometric/assets/sprites/units/humano_male/humano_male_sheet.rgba")) {
                cargarDesdeArchivo("assets/tileset_humano.rgba");
            }
        }
    }
}

void TileSet::inicializarMapaRectangulos() {
    // Hoja de sprites oficial 'humano_male_sheet.png' (256x256 px en cuadrícula 4x4 de 64x64):
    // Fila 0:
    //   Col 0 (0,0):   De pie / Guardia neutral (IDLE SW / SE) -> [x:0, y:0, w:64, h:64]
    //   Col 1 (64,0):  Paso de marcha / Exploración (CAMINAR SW / SE) -> [x:64, y:0, w:64, h:64]
    //   Col 2 (128,0): De espaldas hacia el norte (IDLE / CAMINAR NE / NW) -> [x:128, y:0, w:64, h:64]
    //   Col 3 (192,0): Carrera hacia el frente (CORRER SE / SW) -> [x:192, y:0, w:64, h:64]
    //
    // Fila 1:
    //   Col 0 (0,64):   Carrera hacia la izquierda (CORRER SW / NW) -> [x:0, y:64, w:64, h:64]
    //   Col 1 (64,64):  Ataque / Corte con mazo (ATACAR SW) -> [x:64, y:64, w:64, h:64]
    //   Col 2 (128,64): Golpe con hacha en alto (ATACAR SE) -> [x:128, y:64, w:64, h:64]
    //   Col 3 (192,64): Talar / Golpe descendente al suelo (TALAR / RECOLECTAR) -> [x:192, y:64, w:64, h:64]
    //
    // Fila 2:
    //   Col 0 (0,128):   Postura defensiva con mazo (ATACAR NE / NW) -> [x:0, y:128, w:64, h:64]
    //   Col 1 (64,128):  En cuclillas orando / meditación sagrada (REZAR SE / SW) -> [x:64, y:128, w:64, h:64]
    //   Col 2 (128,128): Sentado en el suelo junto al tótem / fuego (REZAR NE / NW) -> [x:128, y:128, w:64, h:64]
    //   Col 3 (192,128): Golpeado / Herido tambaleándose (HERIDO SE / SW) -> [x:192, y:128, w:64, h:64]
    //
    // Fila 3:
    //   Col 0 (0,192):   Cayendo noqueado (HERIDO NE / NW) -> [x:0, y:192, w:64, h:64]
    //   Col 1 (64,192):  Caído en el suelo / Muerte (MORIR todas las direcciones) -> [x:64, y:192, w:64, h:64]
    //   Col 2 (128,192): Vista posterior neutral (CAMINAR / REZAR de espaldas NE / NW) -> [x:128, y:192, w:64, h:64]
    //   Col 3 (192,192): Celebración de victoria / Manos en alto / Saludo (SALUDAR) -> [x:192, y:192, w:64, h:64]

    // 1. IDLE (Descanso / Guardia)
    m_rectMap[hashAccionDir(AccionHumano::IDLE, DireccionIso::SW)] = {0, 0, 64, 64};
    m_rectMap[hashAccionDir(AccionHumano::IDLE, DireccionIso::SE)] = {0, 0, 64, 64};
    m_rectMap[hashAccionDir(AccionHumano::IDLE, DireccionIso::NE)] = {128, 0, 64, 64};
    m_rectMap[hashAccionDir(AccionHumano::IDLE, DireccionIso::NW)] = {128, 192, 64, 64};

    // 2. CAMINAR (Marcha / Desplazamiento)
    m_rectMap[hashAccionDir(AccionHumano::CAMINAR, DireccionIso::SW)] = {64, 0, 64, 64};
    m_rectMap[hashAccionDir(AccionHumano::CAMINAR, DireccionIso::SE)] = {64, 0, 64, 64};
    m_rectMap[hashAccionDir(AccionHumano::CAMINAR, DireccionIso::NE)] = {128, 0, 64, 64};
    m_rectMap[hashAccionDir(AccionHumano::CAMINAR, DireccionIso::NW)] = {128, 192, 64, 64};

    // 3. CORRER (Desplazamiento rápido)
    m_rectMap[hashAccionDir(AccionHumano::CORRER, DireccionIso::SW)] = {0, 64, 64, 64};
    m_rectMap[hashAccionDir(AccionHumano::CORRER, DireccionIso::SE)] = {192, 0, 64, 64};
    m_rectMap[hashAccionDir(AccionHumano::CORRER, DireccionIso::NE)] = {128, 0, 64, 64};
    m_rectMap[hashAccionDir(AccionHumano::CORRER, DireccionIso::NW)] = {0, 64, 64, 64};

    // 4. TALAR (Trabajo con madera / hacha descendente)
    m_rectMap[hashAccionDir(AccionHumano::TALAR, DireccionIso::SW)] = {192, 64, 64, 64};
    m_rectMap[hashAccionDir(AccionHumano::TALAR, DireccionIso::SE)] = {192, 64, 64, 64};
    m_rectMap[hashAccionDir(AccionHumano::TALAR, DireccionIso::NE)] = {128, 64, 64, 64};
    m_rectMap[hashAccionDir(AccionHumano::TALAR, DireccionIso::NW)] = {64, 64, 64, 64};

    // 5. RECOLECTAR (Recogiendo cosechas / suelo)
    m_rectMap[hashAccionDir(AccionHumano::RECOLECTAR, DireccionIso::SW)] = {192, 64, 64, 64};
    m_rectMap[hashAccionDir(AccionHumano::RECOLECTAR, DireccionIso::SE)] = {192, 64, 64, 64};
    m_rectMap[hashAccionDir(AccionHumano::RECOLECTAR, DireccionIso::NE)] = {128, 64, 64, 64};
    m_rectMap[hashAccionDir(AccionHumano::RECOLECTAR, DireccionIso::NW)] = {192, 64, 64, 64};

    // 6. CARGAR (Transporte de ofrendas o recursos)
    m_rectMap[hashAccionDir(AccionHumano::CARGAR, DireccionIso::SW)] = {64, 0, 64, 64};
    m_rectMap[hashAccionDir(AccionHumano::CARGAR, DireccionIso::SE)] = {64, 0, 64, 64};
    m_rectMap[hashAccionDir(AccionHumano::CARGAR, DireccionIso::NE)] = {128, 0, 64, 64};
    m_rectMap[hashAccionDir(AccionHumano::CARGAR, DireccionIso::NW)] = {128, 192, 64, 64};

    // 7. REZAR (Oración / Devoción ante el altar o tótem)
    m_rectMap[hashAccionDir(AccionHumano::REZAR, DireccionIso::SW)] = {64, 128, 64, 64};
    m_rectMap[hashAccionDir(AccionHumano::REZAR, DireccionIso::SE)] = {64, 128, 64, 64};
    m_rectMap[hashAccionDir(AccionHumano::REZAR, DireccionIso::NE)] = {128, 128, 64, 64};
    m_rectMap[hashAccionDir(AccionHumano::REZAR, DireccionIso::NW)] = {128, 128, 64, 64};

    // 8. ATACAR (Combate / Defensa de la fe)
    m_rectMap[hashAccionDir(AccionHumano::ATACAR, DireccionIso::SW)] = {64, 64, 64, 64};
    m_rectMap[hashAccionDir(AccionHumano::ATACAR, DireccionIso::SE)] = {128, 64, 64, 64};
    m_rectMap[hashAccionDir(AccionHumano::ATACAR, DireccionIso::NE)] = {0, 128, 64, 64};
    m_rectMap[hashAccionDir(AccionHumano::ATACAR, DireccionIso::NW)] = {0, 128, 64, 64};

    // 9. SALUDAR / CELEBRAR (Victoria ritual / Brazo en alto)
    m_rectMap[hashAccionDir(AccionHumano::SALUDAR, DireccionIso::SW)] = {192, 192, 64, 64};
    m_rectMap[hashAccionDir(AccionHumano::SALUDAR, DireccionIso::SE)] = {192, 192, 64, 64};
    m_rectMap[hashAccionDir(AccionHumano::SALUDAR, DireccionIso::NE)] = {192, 192, 64, 64};
    m_rectMap[hashAccionDir(AccionHumano::SALUDAR, DireccionIso::NW)] = {192, 192, 64, 64};

    // 10. MORIR (Caído inerte en el suelo)
    m_rectMap[hashAccionDir(AccionHumano::MORIR, DireccionIso::SW)] = {64, 192, 64, 64};
    m_rectMap[hashAccionDir(AccionHumano::MORIR, DireccionIso::SE)] = {64, 192, 64, 64};
    m_rectMap[hashAccionDir(AccionHumano::MORIR, DireccionIso::NE)] = {64, 192, 64, 64};
    m_rectMap[hashAccionDir(AccionHumano::MORIR, DireccionIso::NW)] = {64, 192, 64, 64};

    // 11. HERIDO (Tambaleándose o noqueado)
    m_rectMap[hashAccionDir(AccionHumano::HERIDO, DireccionIso::SW)] = {192, 128, 64, 64};
    m_rectMap[hashAccionDir(AccionHumano::HERIDO, DireccionIso::SE)] = {192, 128, 64, 64};
    m_rectMap[hashAccionDir(AccionHumano::HERIDO, DireccionIso::NE)] = {0, 192, 64, 64};
    m_rectMap[hashAccionDir(AccionHumano::HERIDO, DireccionIso::NW)] = {0, 192, 64, 64};
}

TileRect TileSet::obtenerRectangulo(AccionHumano accion, DireccionIso direccion) const {
    auto it = m_rectMap.find(hashAccionDir(accion, direccion));
    if (it != m_rectMap.end()) {
        return it->second;
    }
    // Fallback a IDLE (0, 0, 64, 64)
    return {0, 0, 64, 64};
}

bool TileSet::cargarDesdeArchivo(const std::string& rutaArchivo) {
    std::ifstream in(rutaArchivo, std::ios::binary);
    if (!in.is_open()) return false;

    // Formato .rgba binario: 4 bytes width, 4 bytes height, followed by width*height*4 bytes
    uint32_t w = 0, h = 0;
    in.read(reinterpret_cast<char*>(&w), sizeof(w));
    in.read(reinterpret_cast<char*>(&h), sizeof(h));

    if ((w == 256 && h == 256) || (w == 1024 && h == 558) || (w > 0 && h > 0 && w <= 2048 && h <= 2048)) {
        m_width = w;
        m_height = h;
        m_pixels.resize(w * h);
        in.read(reinterpret_cast<char*>(m_pixels.data()), w * h * 4);

        std::cout << "[TileSet] Cargado exitosamente desde archivo: " << rutaArchivo
                  << " (" << m_width << "x" << m_height << " píxeles)" << std::endl;
        return true;
    }

    if (w != 1024 || h != 558) {
        // Podría ser BMP de 32 bits
        in.seekg(0);
        char header[54];
        in.read(header, 54);
        if (header[0] == 'B' && header[1] == 'M') {
            int bmpW = *reinterpret_cast<int*>(&header[18]);
            int bmpH = *reinterpret_cast<int*>(&header[22]);
            if (bmpW > 0 && bmpH != 0) {
                m_width = bmpW;
                m_height = std::abs(bmpH);
                m_pixels.resize(m_width * m_height);
                in.read(reinterpret_cast<char*>(m_pixels.data()), m_pixels.size() * sizeof(uint32_t));
                std::cout << "[TileSet] Cargado exitosamente desde BMP: " << rutaArchivo
                          << " (" << m_width << "x" << m_height << ")" << std::endl;
                return true;
            }
        }
        return false;
    }

    m_width = w;
    m_height = h;
    m_pixels.resize(w * h);
    in.read(reinterpret_cast<char*>(m_pixels.data()), w * h * 4);

    std::cout << "[TileSet] Cargado exitosamente desde archivo: " << rutaArchivo
              << " (" << m_width << "x" << m_height << " píxeles)" << std::endl;
    return true;
}

void TileSet::blitTile(uint32_t* dstBuffer, int dstW, int dstH,
                      const TileRect& rect, float pieX, float pieY,
                      float escala, ColorRGB auraColor, bool seleccionado) const {
    if (rect.width <= 0 || rect.height <= 0) return;

    float destW = rect.width * escala;
    float destH = rect.height * escala;

    int inicioX = static_cast<int>(pieX - destW * 0.5f);
    int inicioY = static_cast<int>(pieY - destH);

    // Aura espiritual en los pies del humano
    int ipieX = static_cast<int>(pieX);
    int ipieY = static_cast<int>(pieY);
    for (int ox = -4; ox <= 4; ++ox) {
        for (int oy = -1; oy <= 1; ++oy) {
            int px = ipieX + ox;
            int py = ipieY + oy;
            int fy = dstH - 1 - py;
            if (px >= 0 && px < dstW && fy >= 0 && fy < dstH) {
                if (seleccionado) {
                    // Base resaltada en el mismo color cian (0, 255, 255) que la selección de terreno
                    dstBuffer[fy * dstW + px] = (0xFFu << 24) | (255u << 16) | (255u << 8) | 0u;
                } else {
                    uint32_t bg = dstBuffer[fy * dstW + px];
                    uint8_t br = bg & 0xFF;
                    uint8_t bgc = (bg >> 8) & 0xFF;
                    uint8_t bb = (bg >> 16) & 0xFF;

                    uint8_t nr = static_cast<uint8_t>((br * 3 + auraColor.r) / 4);
                    uint8_t ng = static_cast<uint8_t>((bgc * 3 + auraColor.g) / 4);
                    uint8_t nb = static_cast<uint8_t>((bb * 3 + auraColor.b) / 4);
                    dstBuffer[fy * dstW + px] = (0xFFu << 24) | (nb << 16) | (ng << 8) | nr;
                }
            }
        }
    }

    int iDestW = static_cast<int>(destW);
    int iDestH = static_cast<int>(destH);

    auto esPixelOpaco = [&](int dx, int dy) -> bool {
        if (dx < 0 || dy < 0 || dx >= iDestW || dy >= iDestH) return false;
        int sx = rect.x + static_cast<int>(dx / escala);
        int sy = rect.y + static_cast<int>(dy / escala);
        if (sx < rect.x || sx >= rect.x + rect.width || sx >= m_width) return false;
        if (sy < rect.y || sy >= rect.y + rect.height || sy >= m_height) return false;
        uint32_t c = m_pixels[sy * m_width + sx];
        return ((c >> 24) & 0xFF) >= 15;
    };

    int margin = seleccionado ? 2 : 0;

    // Copiar píxeles del tileset con alpha blending y contorno de selección cian (0, 255, 255)
    for (int dy = -margin; dy < iDestH + margin; ++dy) {
        int dstPixelY = inicioY + dy;
        int frameY = dstH - 1 - dstPixelY;
        if (frameY < 0 || frameY >= dstH) continue;

        for (int dx = -margin; dx < iDestW + margin; ++dx) {
            int dstPixelX = inicioX + dx;
            if (dstPixelX < 0 || dstPixelX >= dstW) continue;

            bool opaco = false;
            uint32_t srcColor = 0;
            uint8_t a = 0;

            if (dx >= 0 && dy >= 0 && dx < iDestW && dy < iDestH) {
                int srcX = rect.x + static_cast<int>(dx / escala);
                int srcY = rect.y + static_cast<int>(dy / escala);
                if (srcX < rect.x + rect.width && srcX < m_width &&
                    srcY < rect.y + rect.height && srcY < m_height) {
                    srcColor = m_pixels[srcY * m_width + srcX];
                    a = (srcColor >> 24) & 0xFF;
                    opaco = (a >= 15);
                }
            }

            if (!opaco) {
                if (seleccionado) {
                    bool vecinoDist1 = false;
                    bool vecinoDist2 = false;
                    for (int oy = -2; oy <= 2; ++oy) {
                        for (int ox = -2; ox <= 2; ++ox) {
                            if (ox == 0 && oy == 0) continue;
                            if (esPixelOpaco(dx + ox, dy + oy)) {
                                if (std::abs(ox) <= 1 && std::abs(oy) <= 1) {
                                    vecinoDist1 = true;
                                    break;
                                } else {
                                    vecinoDist2 = true;
                                }
                            }
                        }
                        if (vecinoDist1) break;
                    }
                    if (vecinoDist1 || vecinoDist2) {
                        // ColorRGB(0, 255, 255) igual al contorno de selección de terreno (en formato 0xAABBGGRR)
                        dstBuffer[frameY * dstW + dstPixelX] = (0xFFu << 24) | (255u << 16) | (255u << 8) | 0u;
                    }
                }
                continue;
            }

            uint8_t sr = srcColor & 0xFF;
            uint8_t sg = (srcColor >> 8) & 0xFF;
            uint8_t sb = (srcColor >> 16) & 0xFF;

            if (a >= 250) {
                dstBuffer[frameY * dstW + dstPixelX] = (0xFFu << 24) | (sb << 16) | (sg << 8) | sr;
            } else {
                uint32_t bg = dstBuffer[frameY * dstW + dstPixelX];
                uint8_t br = bg & 0xFF;
                uint8_t bgc = (bg >> 8) & 0xFF;
                uint8_t bb = (bg >> 16) & 0xFF;

                float alpha = a / 255.0f;
                uint8_t outR = static_cast<uint8_t>(sr * alpha + br * (1.0f - alpha));
                uint8_t outG = static_cast<uint8_t>(sg * alpha + bgc * (1.0f - alpha));
                uint8_t outB = static_cast<uint8_t>(sb * alpha + bb * (1.0f - alpha));
                dstBuffer[frameY * dstW + dstPixelX] = (0xFFu << 24) | (outB << 16) | (outG << 8) | outR;
            }
        }
    }
}

} // namespace ReligionSim
