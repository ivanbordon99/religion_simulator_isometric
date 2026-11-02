#include "TileSet.hpp"
#include <fstream>
#include <iostream>
#include <algorithm>
#include <cmath>

namespace ReligionSim {

TileSet::TileSet() {
    initializeRectangleMap();
    m_pixels.resize(m_width * m_height, 0);

    // Intentar cargar la nueva hoja de sprites del aldeano masculino (256x256, 4x4)
    if (!loadFromFile("assets/sprites/units/humano_male/humano_male_sheet.rgba")) {
        if (!loadFromFile("../assets/sprites/units/humano_male/humano_male_sheet.rgba")) {
            if (!loadFromFile("/home/Ivancho/Documentos/religion_simulator_isometric/assets/sprites/units/humano_male/humano_male_sheet.rgba")) {
                loadFromFile("assets/tileset_humano.rgba");
            }
        }
    }
}

// EN: Initialize bounding rectangles for standard male/tribal spritesheet
// ES: Inicializa los rectángulos de la hoja de sprites masculina estándar
void TileSet::initializeRectangleMap() {
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
    m_rectMap[hashActionDirection(HumanAction::IDLE, IsoDirection::SW)] = {0, 0, 64, 64};
    m_rectMap[hashActionDirection(HumanAction::IDLE, IsoDirection::SE)] = {0, 0, 64, 64};
    m_rectMap[hashActionDirection(HumanAction::IDLE, IsoDirection::NE)] = {128, 0, 64, 64};
    m_rectMap[hashActionDirection(HumanAction::IDLE, IsoDirection::NW)] = {128, 192, 64, 64};

    // 2. WALK (Marcha / Desplazamiento)
    m_rectMap[hashActionDirection(HumanAction::WALK, IsoDirection::SW)] = {64, 0, 64, 64};
    m_rectMap[hashActionDirection(HumanAction::WALK, IsoDirection::SE)] = {64, 0, 64, 64};
    m_rectMap[hashActionDirection(HumanAction::WALK, IsoDirection::NE)] = {128, 0, 64, 64};
    m_rectMap[hashActionDirection(HumanAction::WALK, IsoDirection::NW)] = {128, 192, 64, 64};

    // 3. RUN (Desplazamiento rápido)
    m_rectMap[hashActionDirection(HumanAction::RUN, IsoDirection::SW)] = {0, 64, 64, 64};
    m_rectMap[hashActionDirection(HumanAction::RUN, IsoDirection::SE)] = {192, 0, 64, 64};
    m_rectMap[hashActionDirection(HumanAction::RUN, IsoDirection::NE)] = {128, 0, 64, 64};
    m_rectMap[hashActionDirection(HumanAction::RUN, IsoDirection::NW)] = {0, 64, 64, 64};

    // 4. CHOP (Trabajo con madera / hacha descendente)
    m_rectMap[hashActionDirection(HumanAction::CHOP, IsoDirection::SW)] = {192, 64, 64, 64};
    m_rectMap[hashActionDirection(HumanAction::CHOP, IsoDirection::SE)] = {192, 64, 64, 64};
    m_rectMap[hashActionDirection(HumanAction::CHOP, IsoDirection::NE)] = {128, 64, 64, 64};
    m_rectMap[hashActionDirection(HumanAction::CHOP, IsoDirection::NW)] = {64, 64, 64, 64};

    // 5. GATHER (Recogiendo cosechas / suelo)
    m_rectMap[hashActionDirection(HumanAction::GATHER, IsoDirection::SW)] = {192, 64, 64, 64};
    m_rectMap[hashActionDirection(HumanAction::GATHER, IsoDirection::SE)] = {192, 64, 64, 64};
    m_rectMap[hashActionDirection(HumanAction::GATHER, IsoDirection::NE)] = {128, 64, 64, 64};
    m_rectMap[hashActionDirection(HumanAction::GATHER, IsoDirection::NW)] = {192, 64, 64, 64};

    // 6. CARRY (Transporte de ofrendas o recursos)
    m_rectMap[hashActionDirection(HumanAction::CARRY, IsoDirection::SW)] = {64, 0, 64, 64};
    m_rectMap[hashActionDirection(HumanAction::CARRY, IsoDirection::SE)] = {64, 0, 64, 64};
    m_rectMap[hashActionDirection(HumanAction::CARRY, IsoDirection::NE)] = {128, 0, 64, 64};
    m_rectMap[hashActionDirection(HumanAction::CARRY, IsoDirection::NW)] = {128, 192, 64, 64};

    // 7. PRAY (Oración / Devoción ante el altar o tótem)
    m_rectMap[hashActionDirection(HumanAction::PRAY, IsoDirection::SW)] = {64, 128, 64, 64};
    m_rectMap[hashActionDirection(HumanAction::PRAY, IsoDirection::SE)] = {64, 128, 64, 64};
    m_rectMap[hashActionDirection(HumanAction::PRAY, IsoDirection::NE)] = {128, 128, 64, 64};
    m_rectMap[hashActionDirection(HumanAction::PRAY, IsoDirection::NW)] = {128, 128, 64, 64};

    // 8. ATTACK (Combate / Defensa de la fe)
    m_rectMap[hashActionDirection(HumanAction::ATTACK, IsoDirection::SW)] = {64, 64, 64, 64};
    m_rectMap[hashActionDirection(HumanAction::ATTACK, IsoDirection::SE)] = {128, 64, 64, 64};
    m_rectMap[hashActionDirection(HumanAction::ATTACK, IsoDirection::NE)] = {0, 128, 64, 64};
    m_rectMap[hashActionDirection(HumanAction::ATTACK, IsoDirection::NW)] = {0, 128, 64, 64};

    // 9. CELEBRATE / SALUTE (Victoria ritual / Brazo en alto)
    m_rectMap[hashActionDirection(HumanAction::CELEBRATE, IsoDirection::SW)] = {192, 192, 64, 64};
    m_rectMap[hashActionDirection(HumanAction::CELEBRATE, IsoDirection::SE)] = {192, 192, 64, 64};
    m_rectMap[hashActionDirection(HumanAction::CELEBRATE, IsoDirection::NE)] = {192, 192, 64, 64};
    m_rectMap[hashActionDirection(HumanAction::CELEBRATE, IsoDirection::NW)] = {192, 192, 64, 64};

    // 10. DIE (Caído inerte en el suelo)
    m_rectMap[hashActionDirection(HumanAction::DIE, IsoDirection::SW)] = {64, 192, 64, 64};
    m_rectMap[hashActionDirection(HumanAction::DIE, IsoDirection::SE)] = {64, 192, 64, 64};
    m_rectMap[hashActionDirection(HumanAction::DIE, IsoDirection::NE)] = {64, 192, 64, 64};
    m_rectMap[hashActionDirection(HumanAction::DIE, IsoDirection::NW)] = {64, 192, 64, 64};

    // 11. HURT (Tambaleándose o noqueado)
    m_rectMap[hashActionDirection(HumanAction::HURT, IsoDirection::SW)] = {192, 128, 64, 64};
    m_rectMap[hashActionDirection(HumanAction::HURT, IsoDirection::SE)] = {192, 128, 64, 64};
    m_rectMap[hashActionDirection(HumanAction::HURT, IsoDirection::NE)] = {0, 192, 64, 64};
    m_rectMap[hashActionDirection(HumanAction::HURT, IsoDirection::NW)] = {0, 192, 64, 64};
}

// EN: Initialize female villager sprite bounds mapping
// ES: Inicializa el mapeo de rectángulos de la hoja de aldeana femenina
void TileSet::initializeFemaleRectangleMap() {
    m_rectMap.clear();

    // Hoja de sprites oficial de la aldeana / mujer tribal (humano_female_sheet.rgba)
    // 256x256 px en cuadrícula 4x4 de 64x64:
    // Fila 0 (y=0):   Oración / Comunión sagrada [x:0,64,128,192, y:0]
    // Fila 1 (y=64):  Caída inerte / Herida / Reposo [x:0,64,128,192, y:64]
    // Fila 2 (y=128): Postura erguida / Marcha / Báculo defensivo [x:0,64,128,192, y:128]
    // Fila 3 (y=192): Afecto conyugal / Recolección / Celebración victoriosa [x:0,64,128,192, y:192]

    // 1. IDLE (Descanso / Vigilia)
    m_rectMap[hashActionDirection(HumanAction::IDLE, IsoDirection::SW)] = {0, 128, 64, 64};
    m_rectMap[hashActionDirection(HumanAction::IDLE, IsoDirection::SE)] = {64, 128, 64, 64};
    m_rectMap[hashActionDirection(HumanAction::IDLE, IsoDirection::NE)] = {128, 128, 64, 64};
    m_rectMap[hashActionDirection(HumanAction::IDLE, IsoDirection::NW)] = {128, 128, 64, 64};

    // 2. WALK (Marcha / Desplazamiento tribal)
    m_rectMap[hashActionDirection(HumanAction::WALK, IsoDirection::SW)] = {0, 128, 64, 64};
    m_rectMap[hashActionDirection(HumanAction::WALK, IsoDirection::SE)] = {64, 128, 64, 64};
    m_rectMap[hashActionDirection(HumanAction::WALK, IsoDirection::NE)] = {128, 128, 64, 64};
    m_rectMap[hashActionDirection(HumanAction::WALK, IsoDirection::NW)] = {128, 128, 64, 64};

    // 3. RUN (Carrera ágil)
    m_rectMap[hashActionDirection(HumanAction::RUN, IsoDirection::SW)] = {0, 128, 64, 64};
    m_rectMap[hashActionDirection(HumanAction::RUN, IsoDirection::SE)] = {64, 128, 64, 64};
    m_rectMap[hashActionDirection(HumanAction::RUN, IsoDirection::NE)] = {128, 128, 64, 64};
    m_rectMap[hashActionDirection(HumanAction::RUN, IsoDirection::NW)] = {128, 128, 64, 64};

    // 4. CHOP (Trabajo / Herramienta comunitaria)
    m_rectMap[hashActionDirection(HumanAction::CHOP, IsoDirection::SW)] = {192, 128, 64, 64};
    m_rectMap[hashActionDirection(HumanAction::CHOP, IsoDirection::SE)] = {192, 128, 64, 64};
    m_rectMap[hashActionDirection(HumanAction::CHOP, IsoDirection::NE)] = {128, 128, 64, 64};
    m_rectMap[hashActionDirection(HumanAction::CHOP, IsoDirection::NW)] = {128, 128, 64, 64};

    // 5. GATHER (Recolección de frutos, hierbas y sustento)
    m_rectMap[hashActionDirection(HumanAction::GATHER, IsoDirection::SW)] = {64, 192, 64, 64};
    m_rectMap[hashActionDirection(HumanAction::GATHER, IsoDirection::SE)] = {64, 192, 64, 64};
    m_rectMap[hashActionDirection(HumanAction::GATHER, IsoDirection::NE)] = {128, 192, 64, 64};
    m_rectMap[hashActionDirection(HumanAction::GATHER, IsoDirection::NW)] = {128, 192, 64, 64};

    // 6. CARRY (Transporte de cestas / ofrendas)
    m_rectMap[hashActionDirection(HumanAction::CARRY, IsoDirection::SW)] = {0, 128, 64, 64};
    m_rectMap[hashActionDirection(HumanAction::CARRY, IsoDirection::SE)] = {64, 128, 64, 64};
    m_rectMap[hashActionDirection(HumanAction::CARRY, IsoDirection::NE)] = {128, 128, 64, 64};
    m_rectMap[hashActionDirection(HumanAction::CARRY, IsoDirection::NW)] = {128, 128, 64, 64};

    // 7. PRAY (Oración / Devoción ante el altar y comunión amorosa)
    m_rectMap[hashActionDirection(HumanAction::PRAY, IsoDirection::SW)] = {0, 0, 64, 64};
    m_rectMap[hashActionDirection(HumanAction::PRAY, IsoDirection::SE)] = {64, 0, 64, 64};
    m_rectMap[hashActionDirection(HumanAction::PRAY, IsoDirection::NE)] = {128, 0, 64, 64};
    m_rectMap[hashActionDirection(HumanAction::PRAY, IsoDirection::NW)] = {128, 0, 64, 64};

    // 8. ATTACK (Defensa de la tribu / Báculo sagrado)
    m_rectMap[hashActionDirection(HumanAction::ATTACK, IsoDirection::SW)] = {192, 128, 64, 64};
    m_rectMap[hashActionDirection(HumanAction::ATTACK, IsoDirection::SE)] = {192, 128, 64, 64};
    m_rectMap[hashActionDirection(HumanAction::ATTACK, IsoDirection::NE)] = {192, 128, 64, 64};
    m_rectMap[hashActionDirection(HumanAction::ATTACK, IsoDirection::NW)] = {192, 128, 64, 64};

    // 9. CELEBRATE / SALUTE (Gozo y brazos en alto)
    m_rectMap[hashActionDirection(HumanAction::CELEBRATE, IsoDirection::SW)] = {192, 192, 64, 64};
    m_rectMap[hashActionDirection(HumanAction::CELEBRATE, IsoDirection::SE)] = {192, 192, 64, 64};
    m_rectMap[hashActionDirection(HumanAction::CELEBRATE, IsoDirection::NE)] = {192, 0, 64, 64};
    m_rectMap[hashActionDirection(HumanAction::CELEBRATE, IsoDirection::NW)] = {192, 0, 64, 64};

    // 10. DIE (Caída en tierra)
    m_rectMap[hashActionDirection(HumanAction::DIE, IsoDirection::SW)] = {0, 64, 64, 64};
    m_rectMap[hashActionDirection(HumanAction::DIE, IsoDirection::SE)] = {0, 64, 64, 64};
    m_rectMap[hashActionDirection(HumanAction::DIE, IsoDirection::NE)] = {64, 64, 64, 64};
    m_rectMap[hashActionDirection(HumanAction::DIE, IsoDirection::NW)] = {64, 64, 64, 64};

    // 11. HURT (Tambaleo / Recuperación)
    m_rectMap[hashActionDirection(HumanAction::HURT, IsoDirection::SW)] = {64, 64, 64, 64};
    m_rectMap[hashActionDirection(HumanAction::HURT, IsoDirection::SE)] = {64, 64, 64, 64};
    m_rectMap[hashActionDirection(HumanAction::HURT, IsoDirection::NE)] = {128, 64, 64, 64};
    m_rectMap[hashActionDirection(HumanAction::HURT, IsoDirection::NW)] = {128, 64, 64, 64};
}

// EN: Look up bounding rectangle for given action and isometric direction
// ES: Busca el rectángulo para una acción y dirección determinada
TileRect TileSet::getRectangle(HumanAction action, IsoDirection direction) const {
    auto it = m_rectMap.find(hashActionDirection(action, direction));
    if (it != m_rectMap.end()) {
        return it->second;
    }
    // Fallback a IDLE (0, 0, 64, 64)
    return {0, 0, 64, 64};
}

// EN: Load raw pixel data from file
// ES: Cargar datos de píxeles desde archivo
bool TileSet::loadFromFile(const std::string& filePath) {
    std::ifstream in(filePath, std::ios::binary);
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

        std::cout << "[TileSet] Cargado exitosamente desde archivo: " << filePath
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
                std::cout << "[TileSet] Cargado exitosamente desde BMP: " << filePath
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

    std::cout << "[TileSet] Cargado exitosamente desde archivo: " << filePath
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
                    // Base resaltada en el color de su fe (formato 0xAABBGGRR)
                    dstBuffer[fy * dstW + px] = (0xFFu << 24) | (auraColor.b << 16) | (auraColor.g << 8) | auraColor.r;
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
                        // Contorno de selección del color de la fe de la unidad seleccionada
                        if (vecinoDist1) {
                            uint8_t lr = static_cast<uint8_t>(std::min(255, static_cast<int>(auraColor.r * 1.25f) + 30));
                            uint8_t lg = static_cast<uint8_t>(std::min(255, static_cast<int>(auraColor.g * 1.25f) + 30));
                            uint8_t lb = static_cast<uint8_t>(std::min(255, static_cast<int>(auraColor.b * 1.25f) + 30));
                            dstBuffer[frameY * dstW + dstPixelX] = (0xFFu << 24) | (lb << 16) | (lg << 8) | lr;
                        } else {
                            dstBuffer[frameY * dstW + dstPixelX] = (0xFFu << 24) | (auraColor.b << 16) | (auraColor.g << 8) | auraColor.r;
                        }
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

void TileSet::blitTileRotated(uint32_t* dstBuffer, int dstW, int dstH,
                             const TileRect& rect, float centroX, float centroY,
                             float anguloRad, float escala, ColorRGB auraColor,
                             float alpha, bool submergedInWater) const {
    (void)auraColor;
    if (rect.width <= 0 || rect.height <= 0 || alpha <= 0.01f) return;

    float destW = rect.width * escala;
    float destH = rect.height * escala;

    float cosA = std::cos(anguloRad);
    float sinA = std::sin(anguloRad);

    float boundRadius = std::sqrt(destW * destW + destH * destH) * 0.55f;
    int minX = std::max(0, static_cast<int>(centroX - boundRadius));
    int maxX = std::min(dstW - 1, static_cast<int>(centroX + boundRadius));
    int minY = std::max(0, static_cast<int>(centroY - boundRadius));
    int maxY = std::min(dstH - 1, static_cast<int>(centroY + boundRadius));

    float halfW = destW * 0.5f;
    float halfH = destH * 0.5f;

    for (int dstPixelY = minY; dstPixelY <= maxY; ++dstPixelY) {
        int frameY = dstH - 1 - dstPixelY;
        if (frameY < 0 || frameY >= dstH) continue;

        float dy = static_cast<float>(dstPixelY) - centroY;

        for (int dstPixelX = minX; dstPixelX <= maxX; ++dstPixelX) {
            float dx = static_cast<float>(dstPixelX) - centroX;

            float rx = dx * cosA + dy * sinA;
            float ry = -dx * sinA + dy * cosA;

            if (rx < -halfW || rx >= halfW || ry < -halfH || ry >= halfH) continue;

            int srcX = rect.x + static_cast<int>((rx + halfW) / escala);
            int srcY = rect.y + static_cast<int>((ry + halfH) / escala);

            if (srcX < rect.x || srcX >= rect.x + rect.width || srcX >= m_width) continue;
            if (srcY < rect.y || srcY >= rect.y + rect.height || srcY >= m_height) continue;

            uint32_t srcColor = m_pixels[srcY * m_width + srcX];
            uint8_t a = (srcColor >> 24) & 0xFF;
            if (a < 15) continue;

            float effAlpha = (a / 255.0f) * alpha;

            uint8_t sr = srcColor & 0xFF;
            uint8_t sg = (srcColor >> 8) & 0xFF;
            uint8_t sb = (srcColor >> 16) & 0xFF;

            if (submergedInWater) {
                sr = static_cast<uint8_t>(sr * 0.45f);
                sg = static_cast<uint8_t>(sg * 0.70f + 35);
                sb = static_cast<uint8_t>(sb * 0.85f + 70);
            }

            uint32_t bg = dstBuffer[frameY * dstW + dstPixelX];
            uint8_t br = bg & 0xFF;
            uint8_t bgc = (bg >> 8) & 0xFF;
            uint8_t bb = (bg >> 16) & 0xFF;

            uint8_t outR = static_cast<uint8_t>(br * (1.0f - effAlpha) + sr * effAlpha);
            uint8_t outG = static_cast<uint8_t>(bgc * (1.0f - effAlpha) + sg * effAlpha);
            uint8_t outB = static_cast<uint8_t>(bb * (1.0f - effAlpha) + sb * effAlpha);

            dstBuffer[frameY * dstW + dstPixelX] = (0xFFu << 24) | (outB << 16) | (outG << 8) | outR;
        }
    }
}

} // namespace ReligionSim
