#include "Renderer.hpp"
#include "SaveSystem.hpp"
#include "VillagerSpriteData.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <cstring>
#include <iterator>
#include <map>

#if defined(_WIN32)
#define HAS_WIN32 1
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <windowsx.h>
#include <GL/gl.h>
#elif __has_include(<X11/Xlib.h>) && __has_include(<GL/gl.h>) && __has_include(<GL/glx.h>)
#define HAS_X11_GL 1
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <GL/gl.h>
#include <GL/glx.h>
#endif

#ifdef HAS_WIN32
LRESULT CALLBACK MainWndProc(HWND, UINT, WPARAM, LPARAM);
#endif

namespace ReligionSim {

namespace {
    constexpr int TERRAIN_ATLAS_WIDTH = 1024;
    constexpr int TERRAIN_ATLAS_HEIGHT = 64;
    constexpr int TERRAIN_TEXTURE_COUNT = 16;
    constexpr std::array<const char*, 7> TERRAIN_PROP_PATHS{{
        "assets/sprites/trees/pine_tree.rgba",
        "assets/sprites/trees/deciduous_tree.rgba",
        "assets/sprites/trees/bush1.rgba",
        "assets/sprites/trees/bush2.rgba",
        "assets/sprites/trees/bush3.rgba",
        "assets/sprites/trees/bush4.rgba",
        "assets/sprites/mountains/rock1.rgba"
    }};

    // 5x7 ASCII bitmap font
    // 0-9, A-Z, space, colon, minus, plus, slash, period, percent
    const uint8_t FONT_DATA[96][5] = {
        {0x00, 0x00, 0x00, 0x00, 0x00}, // 32 ' '
        {0x00, 0x00, 0x5F, 0x00, 0x00}, // 33 '!'
        {0x00, 0x07, 0x00, 0x07, 0x00}, // 34 '"'
        {0x14, 0x7F, 0x14, 0x7F, 0x14}, // 35 '#'
        {0x24, 0x2A, 0x7F, 0x2A, 0x12}, // 36 '$'
        {0x23, 0x13, 0x08, 0x64, 0x62}, // 37 '%'
        {0x36, 0x49, 0x55, 0x22, 0x50}, // 38 '&'
        {0x00, 0x05, 0x03, 0x00, 0x00}, // 39 '\''
        {0x00, 0x1C, 0x22, 0x41, 0x00}, // 40 '('
        {0x00, 0x41, 0x22, 0x1C, 0x00}, // 41 ')'
        {0x14, 0x08, 0x3E, 0x08, 0x14}, // 42 '*'
        {0x08, 0x08, 0x3E, 0x08, 0x08}, // 43 '+'
        {0x00, 0x50, 0x30, 0x00, 0x00}, // 44 ','
        {0x08, 0x08, 0x08, 0x08, 0x08}, // 45 '-'
        {0x00, 0x60, 0x60, 0x00, 0x00}, // 46 '.'
        {0x20, 0x10, 0x08, 0x04, 0x02}, // 47 '/'
        {0x3E, 0x51, 0x49, 0x45, 0x3E}, // 48 '0'
        {0x00, 0x42, 0x7F, 0x40, 0x00}, // 49 '1'
        {0x42, 0x61, 0x51, 0x49, 0x46}, // 50 '2'
        {0x21, 0x41, 0x45, 0x4B, 0x31}, // 51 '3'
        {0x18, 0x14, 0x12, 0x7F, 0x10}, // 52 '4'
        {0x27, 0x45, 0x45, 0x45, 0x39}, // 53 '5'
        {0x3C, 0x4A, 0x49, 0x49, 0x30}, // 54 '6'
        {0x01, 0x71, 0x09, 0x05, 0x03}, // 55 '7'
        {0x36, 0x49, 0x49, 0x49, 0x36}, // 56 '8'
        {0x06, 0x49, 0x49, 0x29, 0x1E}, // 57 '9'
        {0x00, 0x36, 0x36, 0x00, 0x00}, // 58 ':'
        {0x00, 0x56, 0x36, 0x00, 0x00}, // 59 ';'
        {0x08, 0x14, 0x22, 0x41, 0x00}, // 60 '<'
        {0x14, 0x14, 0x14, 0x14, 0x14}, // 61 '='
        {0x00, 0x41, 0x22, 0x14, 0x08}, // 62 '>'
        {0x02, 0x01, 0x51, 0x09, 0x06}, // 63 '?'
        {0x32, 0x49, 0x79, 0x41, 0x3E}, // 64 '@'
        {0x7E, 0x11, 0x11, 0x11, 0x7E}, // 65 'A'
        {0x7F, 0x49, 0x49, 0x49, 0x36}, // 66 'B'
        {0x3E, 0x41, 0x41, 0x41, 0x22}, // 67 'C'
        {0x7F, 0x41, 0x41, 0x22, 0x1C}, // 68 'D'
        {0x7F, 0x49, 0x49, 0x49, 0x41}, // 69 'E'
        {0x7F, 0x09, 0x09, 0x09, 0x01}, // 70 'F'
        {0x3E, 0x41, 0x49, 0x49, 0x7A}, // 71 'G'
        {0x7F, 0x08, 0x08, 0x08, 0x7F}, // 72 'H'
        {0x00, 0x41, 0x7F, 0x41, 0x00}, // 73 'I'
        {0x20, 0x40, 0x41, 0x3F, 0x01}, // 74 'J'
        {0x7F, 0x08, 0x14, 0x22, 0x41}, // 75 'K'
        {0x7F, 0x40, 0x40, 0x40, 0x40}, // 76 'L'
        {0x7F, 0x02, 0x0C, 0x02, 0x7F}, // 77 'M'
        {0x7F, 0x04, 0x08, 0x10, 0x7F}, // 78 'N'
        {0x3E, 0x41, 0x41, 0x41, 0x3E}, // 79 'O'
        {0x7F, 0x09, 0x09, 0x09, 0x06}, // 80 'P'
        {0x3E, 0x41, 0x51, 0x21, 0x5E}, // 81 'Q'
        {0x7F, 0x09, 0x19, 0x29, 0x46}, // 82 'R'
        {0x46, 0x49, 0x49, 0x49, 0x31}, // 83 'S'
        {0x01, 0x01, 0x7F, 0x01, 0x01}, // 84 'T'
        {0x3F, 0x40, 0x40, 0x40, 0x3F}, // 85 'U'
        {0x1F, 0x20, 0x40, 0x20, 0x1F}, // 86 'V'
        {0x3F, 0x40, 0x38, 0x40, 0x3F}, // 87 'W'
        {0x63, 0x14, 0x08, 0x14, 0x63}, // 88 'X'
        {0x07, 0x08, 0x70, 0x08, 0x07}, // 89 'Y'
        {0x61, 0x51, 0x49, 0x45, 0x43}, // 90 'Z'
        {0x00, 0x7F, 0x41, 0x41, 0x00}, // 91 '['
        {0x02, 0x04, 0x08, 0x10, 0x20}, // 92 '\'
        {0x00, 0x41, 0x41, 0x7F, 0x00}, // 93 ']'
        {0x04, 0x02, 0x01, 0x02, 0x04}, // 94 '^'
        {0x40, 0x40, 0x40, 0x40, 0x40}, // 95 '_'
    };
}

Renderer::Renderer(int width, int height)
    : m_width(width), m_height(height) {
    m_framebuffer.resize(width * height, 0xFF1A1210);
    loadMenuBackground();
    loadSpaceBackground();
    loadCursor();
    loadTempleAndTerrainAssets();
    initUI();
}

void Renderer::loadMenuBackground() {
    // Intenta cargar la textura del fondo de menú
    std::vector<std::string> searchPaths = {
        "assets/ui/main_menu_bg.rgba",
        "../assets/ui/main_menu_bg.rgba",
        "religion_simulator_isometric/assets/ui/main_menu_bg.rgba",
        "/home/Ivancho/Documentos/religion_simulator_isometric/assets/ui/main_menu_bg.rgba"
    };

    for (const auto& path : searchPaths) {
        std::ifstream file(path, std::ios::binary);
        if (file.is_open()) {
            file.seekg(0, std::ios::end);
            size_t size = file.tellg();
            file.seekg(0, std::ios::beg);

            // Verificamos dimensiones según el tamaño
            if (size == 1024 * 576 * 4) {
                m_bgWidth = 1024;
                m_bgHeight = 576;
            } else if (size == 1280 * 720 * 4) {
                m_bgWidth = 1280;
                m_bgHeight = 720;
            } else {
                continue;
            }

            std::vector<uint8_t> raw(size);
            file.read(reinterpret_cast<char*>(raw.data()), size);
            m_menuBgPixels.resize(m_bgWidth * m_bgHeight);

            for (size_t i = 0; i < m_menuBgPixels.size(); ++i) {
                uint8_t r = raw[i * 4 + 0];
                uint8_t g = raw[i * 4 + 1];
                uint8_t b = raw[i * 4 + 2];
                uint8_t a = raw[i * 4 + 3];
                // Empaquetado 0xAABBGGRR
                m_menuBgPixels[i] = (static_cast<uint32_t>(a) << 24) |
                                    (static_cast<uint32_t>(b) << 16) |
                                    (static_cast<uint32_t>(g) << 8)  |
                                     static_cast<uint32_t>(r);
            }
            std::cout << "[Renderer] Fondo de pantalla principal cargado: " << path
                      << " (" << m_bgWidth << "x" << m_bgHeight << ")" << std::endl;
            return;
        }
    }
}

void Renderer::loadSpaceBackground() {
    constexpr int backgroundWidth = 256;
    constexpr int backgroundHeight = 128;
    const std::vector<std::string> paths = {
        "assets/sprites/miscellany/space_background.rgba",
        "../assets/sprites/miscellany/space_background.rgba",
        "religion_simulator_isometric/assets/sprites/miscellany/space_background.rgba"
    };

    for (const auto& path : paths) {
        std::ifstream file(path, std::ios::binary);
        if (!file.is_open()) continue;

        std::vector<uint8_t> raw(backgroundWidth * backgroundHeight * 4);
        file.read(reinterpret_cast<char*>(raw.data()), static_cast<std::streamsize>(raw.size()));
        if (file.gcount() != static_cast<std::streamsize>(raw.size())) {
            continue;
        }

        m_spaceBackgroundPixels.resize(backgroundWidth * backgroundHeight);
        for (size_t i = 0; i < m_spaceBackgroundPixels.size(); ++i) {
            const uint8_t r = raw[i * 4];
            const uint8_t g = raw[i * 4 + 1];
            const uint8_t b = raw[i * 4 + 2];
            const uint8_t a = raw[i * 4 + 3];
            m_spaceBackgroundPixels[i] = (static_cast<uint32_t>(a) << 24) |
                                          (static_cast<uint32_t>(b) << 16) |
                                          (static_cast<uint32_t>(g) << 8) |
                                          static_cast<uint32_t>(r);
        }
        std::cout << "[Renderer] Fondo espacial (256x128) cargado: " << path << std::endl;
        return;
    }
}

void Renderer::drawSpaceBackground() {
    constexpr int backgroundWidth = 256;
    constexpr int backgroundHeight = 128;
    if (m_spaceBackgroundPixels.size() != backgroundWidth * backgroundHeight) {
        return;
    }

    for (int y = 0; y < m_height; ++y) {
        const int frameY = m_height - 1 - y;
        const int srcY = y % backgroundHeight;
        for (int x = 0; x < m_width; ++x) {
            const int srcX = x % backgroundWidth;
            m_framebuffer[frameY * m_width + x] =
                m_spaceBackgroundPixels[srcY * backgroundWidth + srcX];
        }
    }
}

void Renderer::loadCursor() {
    std::vector<std::string> searchPaths = {
        "assets/ui/cursor_rts.rgba",
        "../assets/ui/cursor_rts.rgba",
        "religion_simulator_isometric/assets/ui/cursor_rts.rgba",
        "/home/Ivancho/Documentos/religion_simulator_isometric/assets/ui/cursor_rts.rgba"
    };

    for (const auto& path : searchPaths) {
        std::ifstream file(path, std::ios::binary);
        if (file.is_open()) {
            file.seekg(0, std::ios::end);
            size_t size = file.tellg();
            file.seekg(0, std::ios::beg);

            if (size == 32 * 32 * 4) {
                std::vector<uint8_t> raw(size);
                file.read(reinterpret_cast<char*>(raw.data()), size);
                m_cursorPixels.resize(32 * 32);
                for (size_t i = 0; i < 32 * 32; ++i) {
                    uint8_t r = raw[i * 4 + 0];
                    uint8_t g = raw[i * 4 + 1];
                    uint8_t b = raw[i * 4 + 2];
                    uint8_t a = raw[i * 4 + 3];
                    m_cursorPixels[i] = (static_cast<uint32_t>(a) << 24) |
                                        (static_cast<uint32_t>(b) << 16) |
                                        (static_cast<uint32_t>(g) << 8)  |
                                         static_cast<uint32_t>(r);
                }
                std::cout << "[Renderer] Cursor RTS (32x32) cargado exitosamente: " << path << std::endl;
                break;
            }
        }
    }
}

void Renderer::loadTempleAndTerrainAssets() {
    // 1. Cargar sprite del Templo Prehistórico/Animista (128x128)
    std::vector<std::string> templePaths = {
        "assets/sprites/buildings/temple_main.rgba",
        "../assets/sprites/buildings/temple_main.rgba",
        "religion_simulator_isometric/assets/sprites/buildings/temple_main.rgba",
        "/home/Ivancho/Documentos/religion_simulator_isometric/assets/sprites/buildings/temple_main.rgba"
    };
    for (const auto& path : templePaths) {
        std::ifstream file(path, std::ios::binary);
        if (file.is_open()) {
            file.seekg(0, std::ios::end);
            size_t size = file.tellg();
            file.seekg(0, std::ios::beg);
            if (size == 128 * 128 * 4) {
                std::vector<uint8_t> raw(size);
                file.read(reinterpret_cast<char*>(raw.data()), size);
                m_templePixels.resize(128 * 128);
                for (size_t i = 0; i < 128 * 128; ++i) {
                    uint8_t r = raw[i * 4 + 0];
                    uint8_t g = raw[i * 4 + 1];
                    uint8_t b = raw[i * 4 + 2];
                    uint8_t a = raw[i * 4 + 3];
                    m_templePixels[i] = (static_cast<uint32_t>(a) << 24) |
                                        (static_cast<uint32_t>(b) << 16) |
                                        (static_cast<uint32_t>(g) << 8)  |
                                         static_cast<uint32_t>(r);
                }
                std::cout << "[Renderer] Templo Principal (128x128) cargado exitosamente: " << path << std::endl;
                break;
            }
        }
    }

    // 2. Cargar atlas exclusivo de terreno (1024x64).
    std::vector<std::string> terrainPaths = {
        "assets/sprites/terrain/terrain_atlas.rgba",
        "../assets/sprites/terrain/terrain_atlas.rgba",
        "religion_simulator_isometric/assets/sprites/terrain/terrain_atlas.rgba",
        "/home/Ivancho/Documentos/religion_simulator_isometric/assets/sprites/terrain/terrain_atlas.rgba"
    };
    for (const auto& path : terrainPaths) {
        std::ifstream file(path, std::ios::binary);
        if (file.is_open()) {
            file.seekg(0, std::ios::end);
            size_t size = file.tellg();
            file.seekg(0, std::ios::beg);
            if (size == TERRAIN_ATLAS_WIDTH * TERRAIN_ATLAS_HEIGHT * 4) {
                std::vector<uint8_t> raw(size);
                file.read(reinterpret_cast<char*>(raw.data()), size);
                m_terrainAtlasPixels.resize(TERRAIN_ATLAS_WIDTH * TERRAIN_ATLAS_HEIGHT);
                for (size_t i = 0; i < m_terrainAtlasPixels.size(); ++i) {
                    uint8_t r = raw[i * 4 + 0];
                    uint8_t g = raw[i * 4 + 1];
                    uint8_t b = raw[i * 4 + 2];
                    uint8_t a = raw[i * 4 + 3];
                    m_terrainAtlasPixels[i] = (static_cast<uint32_t>(a) << 24) |
                                              (static_cast<uint32_t>(b) << 16) |
                                              (static_cast<uint32_t>(g) << 8)  |
                                               static_cast<uint32_t>(r);
                }
                std::cout << "[Renderer] Atlas de terreno (1024x64) cargado exitosamente: " << path << std::endl;
                break;
            }
        }
    }

    for (size_t propIndex = 0; propIndex < TERRAIN_PROP_PATHS.size(); ++propIndex) {
        std::vector<std::string> propPaths = {
            TERRAIN_PROP_PATHS[propIndex],
            std::string("../") + TERRAIN_PROP_PATHS[propIndex],
            std::string("religion_simulator_isometric/") + TERRAIN_PROP_PATHS[propIndex],
            std::string("/home/Ivancho/Documentos/religion_simulator_isometric/") + TERRAIN_PROP_PATHS[propIndex]
        };
        bool loaded = false;
        for (const auto& path : propPaths) {
            std::ifstream file(path, std::ios::binary);
            if (!file.is_open()) continue;
            file.seekg(0, std::ios::end);
            if (file.tellg() != static_cast<std::streamoff>(64u * 64u * 4u)) continue;
            file.seekg(0, std::ios::beg);
            std::vector<uint8_t> raw(64u * 64u * 4u);
            file.read(reinterpret_cast<char*>(raw.data()), static_cast<std::streamsize>(raw.size()));
            if (file.gcount() != static_cast<std::streamsize>(raw.size())) continue;

            auto& pixels = m_terrainPropPixels[propIndex];
            pixels.resize(64u * 64u);
            for (size_t i = 0; i < pixels.size(); ++i) {
                pixels[i] = (static_cast<uint32_t>(raw[i * 4 + 3]) << 24) |
                            (static_cast<uint32_t>(raw[i * 4 + 2]) << 16) |
                            (static_cast<uint32_t>(raw[i * 4 + 1]) << 8) |
                            static_cast<uint32_t>(raw[i * 4]);
            }
            loaded = true;
            break;
        }
        if (!loaded) {
            std::cerr << "[Renderer] No se pudo cargar el sprite de terreno: "
                      << TERRAIN_PROP_PATHS[propIndex] << std::endl;
        }
    }

    // 3. Cargar hoja de sprites del Chamán Sagrado (256x256, cuadrícula 4x4 de 64x64)
    if (!m_shamanTileSet.cargarDesdeArchivo("assets/sprites/units/shaman/chaman_sheet.rgba")) {
        if (!m_shamanTileSet.cargarDesdeArchivo("../assets/sprites/units/shaman/chaman_sheet.rgba")) {
            if (!m_shamanTileSet.cargarDesdeArchivo("religion_simulator_isometric/assets/sprites/units/shaman/chaman_sheet.rgba")) {
                m_shamanTileSet.cargarDesdeArchivo("/home/Ivancho/Documentos/religion_simulator_isometric/assets/sprites/units/shaman/chaman_sheet.rgba");
            }
        }
    }
}

void Renderer::drawCursor(int mouseX, int mouseY) {
    if (m_cursorPixels.size() != 32 * 32) return;

    // Hotspot en la punta del dedo índice (x=2, y=0) dentro del tamaño estándar 32x32 RTS
    const int hotspotX = 2;
    const int hotspotY = 0;
    const int startX = mouseX - hotspotX;
    const int startY = mouseY - hotspotY;

    for (int cy = 0; cy < 32; ++cy) {
        int dstY = startY + cy;
        int frameY = m_height - 1 - dstY;
        if (frameY < 0 || frameY >= m_height) continue;

        for (int cx = 0; cx < 32; ++cx) {
            int dstX = startX + cx;
            if (dstX < 0 || dstX >= m_width) continue;

            uint32_t src = m_cursorPixels[cy * 32 + cx];
            uint8_t a = (src >> 24) & 0xFF;
            if (a < 15) continue;

            uint8_t sr = src & 0xFF;
            uint8_t sg = (src >> 8) & 0xFF;
            uint8_t sb = (src >> 16) & 0xFF;

            if (a >= 250) {
                m_framebuffer[frameY * m_width + dstX] = (0xFFu << 24) | (sb << 16) | (sg << 8) | sr;
            } else {
                uint32_t bg = m_framebuffer[frameY * m_width + dstX];
                uint8_t br = bg & 0xFF;
                uint8_t bgc = (bg >> 8) & 0xFF;
                uint8_t bb = (bg >> 16) & 0xFF;

                float alpha = a / 255.0f;
                uint8_t outR = static_cast<uint8_t>(sr * alpha + br * (1.0f - alpha));
                uint8_t outG = static_cast<uint8_t>(sg * alpha + bgc * (1.0f - alpha));
                uint8_t outB = static_cast<uint8_t>(sb * alpha + bb * (1.0f - alpha));
                m_framebuffer[frameY * m_width + dstX] = (0xFFu << 24) | (outB << 16) | (outG << 8) | outR;
            }
        }
    }
}

Renderer::~Renderer() {
    closeWindow();
}

void Renderer::getMinimapRect(int& x, int& y, int& size) const {
    // Panel inferior derecho: minimapa a la izquierda, botones de vista a su derecha
    const int amarilloX = static_cast<int>(m_width * 0.185f) + static_cast<int>(m_width * 0.63f);
    size = 92;
    x = amarilloX + 8;
    y = m_height - 118 + 20;
}

void Renderer::initUI() {
    m_buttons.clear();
    m_dropdownButtons.clear();

    const int botH = 118;
    const float botY = static_cast<float>(m_height - botH);
    const int azulX = static_cast<int>(m_width * 0.185f);
    const int azulW = static_cast<int>(m_width * 0.63f);
    const int amarilloX = azulX + azulW;
    const int amarilloW = m_width - amarilloX;

    // Top Bar - Panel de Opciones: sólo el botón MENU (abre el desplegable de opciones de partida)
    m_buttons.push_back({400, static_cast<float>(m_width) - 110.0f, 5.0f, 100.0f, 22.0f, "MENU", false});

    // Desplegable de opciones de partida (debajo del botón MENU)
    {
        const float ddW = 200.0f;
        const float ddX = static_cast<float>(m_width) - ddW - 10.0f;
        const float ddY = 40.0f;
        const float itemH = 24.0f;
        const float gap = 4.0f;
        m_dropdownButtons.push_back({401, ddX + 6.0f, ddY + 6.0f + 0 * (itemH + gap), ddW - 12.0f, itemH, "GUARDAR PARTIDA", false});
        m_dropdownButtons.push_back({304, ddX + 6.0f, ddY + 6.0f + 1 * (itemH + gap), ddW - 12.0f, itemH, "REINICIAR CAMARA", false});
        m_dropdownButtons.push_back({405, ddX + 6.0f, ddY + 6.0f + 2 * (itemH + gap), ddW - 12.0f, itemH, "CONSOLA DE TRUCOS", false});
        m_dropdownButtons.push_back({402, ddX + 6.0f, ddY + 6.0f + 3 * (itemH + gap), ddW - 12.0f, itemH, "SALIR AL MENU PRINCIPAL", false});
    }

    // Panel central inferior: controles de la simulación centrados DEBAJO de la línea de tiempo
    const float rowY = static_cast<float>(m_height) - 36.0f;
    const float rowH = 24.0f;
    {
        const float totalW = 52.0f + 34.0f * 3 + 4.0f * 3;
        float cx = static_cast<float>(azulX) + (static_cast<float>(azulW) - totalW) * 0.5f;
        m_buttons.push_back({201, cx, rowY, 52.0f, rowH, "PAUSA", false});
        m_buttons.push_back({202, cx + 56.0f, rowY, 34.0f, rowH, "1x", true});
        m_buttons.push_back({203, cx + 94.0f, rowY, 34.0f, rowH, "2x", false});
        m_buttons.push_back({204, cx + 132.0f, rowY, 34.0f, rowH, "5x", false});
    }

    // Intervenciones divinas del jugador (alineadas a la derecha de la misma fila)
    {
        const float totalW = 90.0f + 70.0f + 76.0f + 4.0f * 2;
        float ix = static_cast<float>(azulX + azulW) - 16.0f - totalW;
        m_buttons.push_back({301, ix, rowY, 90.0f, rowH, "+ SINCRETISMO", false});
        m_buttons.push_back({302, ix + 94.0f, rowY, 70.0f, rowH, "+ CISMA", false});
        m_buttons.push_back({303, ix + 168.0f, rowY, 76.0f, rowH, "+ TEMPLO", false});
    }

    // Panel del minimapa: botones para cambiar la vista del mapa (a la derecha del minimapa)
    {
        const float vbX = static_cast<float>(amarilloX) + 8.0f + 92.0f + 10.0f;
        const float vbW = static_cast<float>(amarilloW) - (vbX - static_cast<float>(amarilloX)) - 8.0f;
        const float vbH = 18.0f;
        const float vbGap = 3.0f;
        const float vbY = botY + 8.0f;
        m_buttons.push_back({101, vbX, vbY + 0 * (vbH + vbGap), vbW, vbH, "FE", true});
        m_buttons.push_back({102, vbX, vbY + 1 * (vbH + vbGap), vbW, vbH, "BIOMAS", false});
        m_buttons.push_back({105, vbX, vbY + 2 * (vbH + vbGap), vbW, vbH, "UNIDADES", false});
        m_buttons.push_back({103, vbX, vbY + 3 * (vbH + vbGap), vbW, vbH, "SINCRETISMO", false});
        m_buttons.push_back({104, vbX, vbY + 4 * (vbH + vbGap), vbW, vbH, "ERAS", false});
    }

    // Bottom Bar - Sección Verde (Opciones de la Unidad / Señales Divinas del Chamán):
    float verdeW = m_width * 0.185f;
    float halfVerdeBtnW = (verdeW - 20.0f) * 0.5f;
    m_buttons.push_back({701, 8.0f, botY + 58.0f, halfVerdeBtnW, 24.0f, "1. VISION", false});
    m_buttons.push_back({702, 12.0f + halfVerdeBtnW, botY + 58.0f, halfVerdeBtnW, 24.0f, "2. CONGREGAR", false});
    m_buttons.push_back({703, 8.0f, botY + 86.0f, verdeW - 16.0f, 24.0f, "3. MILAGRO DE FE", false});

    initMenuUI();
}

void Renderer::initMenuUI() {
    m_menuButtons.clear();
    m_optionsButtons.clear();

    // Botones del Menú Principal centrados en pantalla
    const float btnW = 280.0f;
    const float btnH = 46.0f;
    const float spacing = 18.0f;
    const float totalH = 4 * btnH + 3 * spacing;

    const float startX = (m_width - btnW) * 0.5f;
    const float startY = (m_height - totalH) * 0.5f + 40.0f;

    m_menuButtons.push_back({1, startX, startY, btnW, btnH, "CREAR MUNDO", false});
    m_menuButtons.push_back({2, startX, startY + (btnH + spacing), btnW, btnH, "CARGAR MUNDO", false});
    m_menuButtons.push_back({3, startX, startY + 2 * (btnH + spacing), btnW, btnH, "OPCIONES", false});
    m_menuButtons.push_back({4, startX, startY + 3 * (btnH + spacing), btnW, btnH, "SALIR", false});

    // Botones de Opciones
    const float optY = startY + 2 * (btnH + spacing);
    m_optionsButtons.push_back({11, startX, optY, btnW, btnH, "RESOLUCION: NATIVA", false});
    m_optionsButtons.push_back({12, startX, optY + (btnH + spacing), btnW, btnH, "VOLVER AL MENU", false});

    // Botones de la Pantalla de Creación de Mundo
    m_creationButtons.clear();
    const float modalW = 880.0f;
    const float modalH = 590.0f;
    const float modalX = (m_width - modalW) * 0.5f;
    const float modalY = (m_height - modalH) * 0.5f;

    const float col1X = modalX + 40.0f;
    const float col2X = modalX + modalW * 0.5f + 20.0f;
    const float fieldW = 360.0f;
    const float fieldH = 32.0f;

    // Columna Izquierda: Identidad y Génesis
    // ID 501: Dios (Input text)
    m_creationButtons.push_back({501, col1X, modalY + 80.0f, fieldW, fieldH, "", false});
    // ID 502: Religión (Input text)
    m_creationButtons.push_back({502, col1X, modalY + 140.0f, fieldW, fieldH, "", false});
    // ID 503: Semilla (Input / Randomize)
    m_creationButtons.push_back({503, col1X, modalY + 200.0f, fieldW - 70.0f, fieldH, "", false});
    m_creationButtons.push_back({504, col1X + fieldW - 65.0f, modalY + 200.0f, 65.0f, fieldH, "DICE", false});
    // ID 505: Población inicial (Toggle/Cycle)
    m_creationButtons.push_back({505, col1X, modalY + 260.0f, fieldW, fieldH, "", false});
    // ID 506: Época de inicio (Toggle/Cycle)
    m_creationButtons.push_back({506, col1X, modalY + 320.0f, fieldW, fieldH, "", false});
    // ID 507: Tamaño del Mapa (Toggle/Cycle)
    m_creationButtons.push_back({507, col1X, modalY + 380.0f, fieldW, fieldH, "", false});
    // ID 508: Desastres / Plagas (Toggle/Cycle)
    m_creationButtons.push_back({508, col1X, modalY + 440.0f, fieldW, fieldH, "", false});

    // Columna Derecha: Dogma, Partida y Desafío
    // ID 509: Tipo de partida (Estandar, Sandbox, Teocracia, Supervivencia)
    m_creationButtons.push_back({509, col2X, modalY + 80.0f, fieldW, fieldH, "", false});
    // ID 510: Cofre de inicio (ON / OFF)
    m_creationButtons.push_back({510, col2X, modalY + 140.0f, fieldW, fieldH, "", false});
    // ID 511: Dificultad de la IA (Fácil, Normal, Difícil, Pesadilla)
    m_creationButtons.push_back({511, col2X, modalY + 200.0f, fieldW, fieldH, "", false});
    // ID 512: Color de Influencia / Creyentes (Paleta mística)
    m_creationButtons.push_back({512, col2X, modalY + 260.0f, fieldW, fieldH, "", false});
    // ID 513: Tipo de Victoria
    m_creationButtons.push_back({513, col2X, modalY + 320.0f, fieldW, fieldH, "", false});
    // ID 514: Cismas espontáneos (ON / OFF)
    m_creationButtons.push_back({514, col2X, modalY + 380.0f, fieldW, fieldH, "", false});
    // ID 515: Sincretismo / Fusión de fes (ON / OFF)
    m_creationButtons.push_back({515, col2X, modalY + 440.0f, fieldW, fieldH, "", false});

    // Botones de acción inferiores
    const float actBtnW = 200.0f;
    const float actBtnH = 44.0f;
    // ID 550: CANCELAR / VOLVER
    m_creationButtons.push_back({550, modalX + 40.0f, modalY + modalH - 60.0f, actBtnW, actBtnH, "CANCELAR", false});
    // ID 551: INICIAR CREACION / GÉNESIS
    m_creationButtons.push_back({551, modalX + modalW - actBtnW - 40.0f, modalY + modalH - 60.0f, actBtnW, actBtnH, "COMENZAR GENESIS", false});
}

void Renderer::resize(int newWidth, int newHeight) {
    if (newWidth <= 100 || newHeight <= 100) return;
    if (newWidth == m_width && newHeight == m_height) return;

    m_width = newWidth;
    m_height = newHeight;
    m_framebuffer.resize(m_width * m_height, 0xFF1A1210);

#if HAS_X11_GL
    if (m_hasActiveWindow) {
        glViewport(0, 0, m_width, m_height);
    }
#elif defined(HAS_WIN32)
    if (m_hasActiveWindow) {
        glViewport(0, 0, m_width, m_height);
    }
#endif
    initUI();
}

int Renderer::checkMenuButtonClicked(float mouseX, float mouseY, bool inOptions) const {
    const auto& list = inOptions ? m_optionsButtons : m_menuButtons;
    for (const auto& btn : list) {
        if (mouseX >= btn.x && mouseX <= btn.x + btn.width &&
            mouseY >= btn.y && mouseY <= btn.y + btn.height) {
            return btn.id;
        }
    }
    return 0;
}

int Renderer::checkCreationButtonClicked(float mouseX, float mouseY) const {
    for (const auto& btn : m_creationButtons) {
        if (mouseX >= btn.x && mouseX <= btn.x + btn.width &&
            mouseY >= btn.y && mouseY <= btn.y + btn.height) {
            return btn.id;
        }
    }
    return 0;
}

int Renderer::checkLoadMenuButtonClicked(float mouseX, float mouseY, int savesCount) const {
    const float modalW = 880.0f;
    const float modalH = 590.0f;
    const float mx = (m_width - modalW) * 0.5f;
    const float my = (m_height - modalH) * 0.5f;

    // Clic en slots de mundo (IDs 1000 + i)
    const float listX = mx + 40.0f;
    const float listY = my + 80.0f;
    const float slotW = modalW - 80.0f;
    const float slotH = 76.0f;
    const float spacing = 12.0f;

    for (int i = 0; i < savesCount && i < 5; ++i) {
        float sy = listY + i * (slotH + spacing);
        if (mouseX >= listX && mouseX <= listX + slotW &&
            mouseY >= sy && mouseY <= sy + slotH) {
            return 1000 + i;
        }
    }

    // Botones de acción inferiores
    const float btnY = my + modalH - 56.0f;
    // ID 601: JUGAR EN EL MUNDO SELECCIONADO
    if (mouseX >= mx + 40.0f && mouseX <= mx + 260.0f && mouseY >= btnY && mouseY <= btnY + 42.0f) {
        return 601;
    }
    // ID 602: CREAR UN NUEVO MUNDO
    if (mouseX >= mx + 280.0f && mouseX <= mx + 500.0f && mouseY >= btnY && mouseY <= btnY + 42.0f) {
        return 602;
    }
    // ID 603: ELIMINAR MUNDO
    if (mouseX >= mx + 520.0f && mouseX <= mx + 680.0f && mouseY >= btnY && mouseY <= btnY + 42.0f) {
        return 603;
    }
    // ID 604: CANCELAR / VOLVER
    if (mouseX >= mx + 700.0f && mouseX <= mx + 840.0f && mouseY >= btnY && mouseY <= btnY + 42.0f) {
        return 604;
    }

    return 0;
}

int Renderer::checkButtonClicked(float mouseX, float mouseY) const {
    // El desplegable de opciones tiene prioridad mientras está abierto
    if (m_dropdownOpen) {
        for (const auto& btn : m_dropdownButtons) {
            if (mouseX >= btn.x && mouseX <= btn.x + btn.width &&
                mouseY >= btn.y && mouseY <= btn.y + btn.height) {
                return btn.id;
            }
        }
    }
    for (const auto& btn : m_buttons) {
        if (mouseX >= btn.x && mouseX <= btn.x + btn.width &&
            mouseY >= btn.y && mouseY <= btn.y + btn.height) {
            return btn.id;
        }
    }
    return 0;
}

bool Renderer::initWindow(const std::string& title) {
#if HAS_X11_GL
    Display* dpy = XOpenDisplay(nullptr);
    if (!dpy) {
        // Headless fallback
        m_hasActiveWindow = false;
        return false;
    }
    m_display = dpy;

    Window root = DefaultRootWindow(dpy);
    GLint att[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None };
    XVisualInfo* vi = glXChooseVisual(dpy, 0, att);
    if (!vi) {
        XCloseDisplay(dpy);
        m_display = nullptr;
        return false;
    }

    Colormap cmap = XCreateColormap(dpy, root, vi->visual, AllocNone);
    XSetWindowAttributes swa;
    swa.colormap = cmap;
    swa.event_mask = ExposureMask | KeyPressMask | KeyReleaseMask |
                     ButtonPressMask | ButtonReleaseMask | PointerMotionMask | StructureNotifyMask;

    Window win = XCreateWindow(dpy, root, 50, 50, m_width, m_height, 0, vi->depth,
                               InputOutput, vi->visual, CWColormap | CWEventMask, &swa);
    XStoreName(dpy, win, title.c_str());

    // Ocultar el cursor nativo de X11 para mostrar únicamente el cursor RTS 32x32 del juego
    char blankData[1] = {0};
    Pixmap blankPixmap = XCreateBitmapFromData(dpy, win, blankData, 1, 1);
    if (blankPixmap != None) {
        XColor dummyColor{};
        Cursor invisibleCursor = XCreatePixmapCursor(dpy, blankPixmap, blankPixmap, &dummyColor, &dummyColor, 0, 0);
        XDefineCursor(dpy, win, invisibleCursor);
        XFreePixmap(dpy, blankPixmap);
    }

    XMapWindow(dpy, win);
    m_window = win;

    GLXContext glc = glXCreateContext(dpy, vi, nullptr, GL_TRUE);
    XFree(vi);
    glXMakeCurrent(dpy, win, glc);
    m_glContext = glc;
    m_hasActiveWindow = true;

    glViewport(0, 0, m_width, m_height);
    return true;
#elif defined(HAS_WIN32)
    HINSTANCE hInstance = GetModuleHandleA(NULL);
    const char* className = "ReligionSimWindowClass";

    WNDCLASSEXA wc{};
    wc.cbSize = sizeof(WNDCLASSEXA);
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wc.lpfnWndProc = ::MainWndProc;
    wc.hInstance = hInstance;
    wc.hCursor = NULL;
    wc.lpszClassName = className;

    RegisterClassExA(&wc);

    RECT wr = { 0, 0, m_width, m_height };
    AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);
    int winW = wr.right - wr.left;
    int winH = wr.bottom - wr.top;

    HWND hwnd = CreateWindowExA(
        0, className, title.c_str(),
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, winW, winH,
        NULL, NULL, hInstance, NULL
    );

    if (!hwnd) {
        m_hasActiveWindow = false;
        return false;
    }

    HDC hdc = GetDC(hwnd);
    PIXELFORMATDESCRIPTOR pfd{};
    pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.cDepthBits = 24;

    int pixelFormat = ChoosePixelFormat(hdc, &pfd);
    SetPixelFormat(hdc, pixelFormat, &pfd);

    HGLRC glc = wglCreateContext(hdc);
    wglMakeCurrent(hdc, glc);

    ShowCursor(FALSE); // Ocultar cursor nativo para usar el cursor RTS 32x32

    m_hwnd = hwnd;
    m_hdc = hdc;
    m_glContext = glc;
    m_hasActiveWindow = true;

    glViewport(0, 0, m_width, m_height);
    return true;
#else
    (void)title;
    return false;
#endif
}

void Renderer::closeWindow() {
#if HAS_X11_GL
    if (m_display) {
        Display* dpy = static_cast<Display*>(m_display);
        if (m_glContext) {
            glXMakeCurrent(dpy, None, nullptr);
            glXDestroyContext(dpy, static_cast<GLXContext>(m_glContext));
            m_glContext = nullptr;
        }
        if (m_window) {
            XDestroyWindow(dpy, m_window);
            m_window = 0;
        }
        XCloseDisplay(dpy);
        m_display = nullptr;
    }
#elif defined(HAS_WIN32)
    if (m_glContext) {
        wglMakeCurrent(NULL, NULL);
        wglDeleteContext(static_cast<HGLRC>(m_glContext));
        m_glContext = nullptr;
    }
    if (m_hdc && m_hwnd) {
        ReleaseDC(static_cast<HWND>(m_hwnd), static_cast<HDC>(m_hdc));
        m_hdc = nullptr;
    }
    if (m_hwnd) {
        DestroyWindow(static_cast<HWND>(m_hwnd));
        m_hwnd = nullptr;
    }
#endif
    m_hasActiveWindow = false;
}

void Renderer::beginFrame() {
    clear(ColorRGB(14, 18, 28)); // Deep cosmic obsidian
    drawSpaceBackground();
}

void Renderer::endFrame() {
#if HAS_X11_GL
    if (m_hasActiveWindow && m_display) {
        glDrawPixels(m_width, m_height, GL_RGBA, GL_UNSIGNED_BYTE, m_framebuffer.data());
        glXSwapBuffers(static_cast<Display*>(m_display), m_window);
    }
#elif defined(HAS_WIN32)
    if (m_hasActiveWindow && m_hdc) {
        glDrawPixels(m_width, m_height, GL_RGBA, GL_UNSIGNED_BYTE, m_framebuffer.data());
        SwapBuffers(static_cast<HDC>(m_hdc));
    }
#endif
}

void Renderer::clear(ColorRGB color) {
    uint32_t val = color.toRGBA();
    std::fill(m_framebuffer.begin(), m_framebuffer.end(), val);
}

void Renderer::drawLine(int x0, int y0, int x1, int y1, ColorRGB color) {
    int dx = std::abs(x1 - x0);
    int dy = std::abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;

    uint32_t c = color.toRGBA();

    while (true) {
        if (x0 >= 0 && x0 < m_width && y0 >= 0 && y0 < m_height) {
            // Flip Y for standard top-left screen coordinates
            int py = m_height - 1 - y0;
            if (py >= 0 && py < m_height) {
                m_framebuffer[py * m_width + x0] = c;
            }
        }
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 < dx)  { err += dx; y0 += sy; }
    }
}

void Renderer::drawRect(int x, int y, int w, int h, ColorRGB color) {
    uint32_t c = color.toRGBA();
    int xEnd = std::min(m_width, x + w);
    int yEnd = std::min(m_height, y + h);

    for (int j = std::max(0, y); j < yEnd; ++j) {
        int py = m_height - 1 - j;
        if (py < 0 || py >= m_height) continue;
        for (int i = std::max(0, x); i < xEnd; ++i) {
            m_framebuffer[py * m_width + i] = c;
        }
    }
}

void Renderer::drawRectOutline(int x, int y, int w, int h, ColorRGB color) {
    drawLine(x, y, x + w, y, color);
    drawLine(x + w, y, x + w, y + h, color);
    drawLine(x + w, y + h, x, y + h, color);
    drawLine(x, y + h, x, y, color);
}

void Renderer::drawFilledDiamond(float cx, float cy, float halfW, float halfH,
                                ColorRGB topColor, ColorRGB skirtColor, float skirtH) {
    int minX = static_cast<int>(std::floor(cx - halfW));
    int maxX = static_cast<int>(std::ceil(cx + halfW));
    int minY = static_cast<int>(std::floor(cy - halfH));
    int maxY = static_cast<int>(std::ceil(cy + halfH + skirtH));

    minX = std::max(0, minX);
    maxX = std::min(m_width - 1, maxX);
    minY = std::max(0, minY);
    maxY = std::min(m_height - 1, maxY);

    uint32_t topC = topColor.toRGBA();
    uint32_t skirtC = skirtColor.toRGBA();

    for (int py = minY; py <= maxY; ++py) {
        int frameY = m_height - 1 - py;
        if (frameY < 0 || frameY >= m_height) continue;

        for (int px = minX; px <= maxX; ++px) {
            float dx = std::abs(static_cast<float>(px) - cx) / halfW;
            float dy = std::abs(static_cast<float>(py) - cy) / halfH;

            // Diamond test: |dx| + |dy| <= 1.0
            if (dx + dy <= 1.0f) {
                m_framebuffer[frameY * m_width + px] = topC;
            } else if (skirtH > 0.0f && py > cy) {
                // Skirt region below diamond
                if (dx <= 1.0f && (py - cy) <= skirtH + (1.0f - dx) * halfH) {
                    m_framebuffer[frameY * m_width + px] = skirtC;
                }
            }
        }
    }
}

void Renderer::drawTexturedDiamond(float cx, float cy, float halfW, float halfH,
                                   int texIndex, ColorRGB tintColor, float tintStrength,
                                   ColorRGB skirtColor, float skirtH) {
    if (m_terrainAtlasPixels.size() != TERRAIN_ATLAS_WIDTH * TERRAIN_ATLAS_HEIGHT ||
        texIndex < 0 || texIndex >= TERRAIN_TEXTURE_COUNT) {
        drawFilledDiamond(cx, cy, halfW, halfH, tintColor, skirtColor, skirtH);
        return;
    }

    int minX = std::max(0, static_cast<int>(std::floor(cx - halfW)));
    int maxX = std::min(m_width - 1, static_cast<int>(std::ceil(cx + halfW)));
    int minY = std::max(0, static_cast<int>(std::floor(cy - halfH)));
    int maxY = std::min(m_height - 1, static_cast<int>(std::ceil(cy + halfH + skirtH)));

    uint32_t skirtC = skirtColor.toRGBA();
    const int baseTexX = texIndex * 64;

    for (int py = minY; py <= maxY; ++py) {
        int frameY = m_height - 1 - py;
        if (frameY < 0 || frameY >= m_height) continue;

        float relY = (static_cast<float>(py) - cy) / halfH;
        float dy = std::abs(relY);
        int ty = std::clamp(static_cast<int>((relY + 1.0f) * 16.0f), 0, 31);

        for (int px = minX; px <= maxX; ++px) {
            float relX = (static_cast<float>(px) - cx) / halfW;
            float dx = std::abs(relX);

            if (dx + dy <= 1.0f) {
                int tx = std::clamp(static_cast<int>((relX + 1.0f) * 32.0f), 0, 63);

                uint32_t texel = m_terrainAtlasPixels[ty * TERRAIN_ATLAS_WIDTH + baseTexX + tx];
                uint8_t tr = texel & 0xFF;
                uint8_t tg = (texel >> 8) & 0xFF;
                uint8_t tb = (texel >> 16) & 0xFF;

                if (tintStrength > 0.01f) {
                    tr = static_cast<uint8_t>(tr * (1.0f - tintStrength) + tintColor.r * tintStrength);
                    tg = static_cast<uint8_t>(tg * (1.0f - tintStrength) + tintColor.g * tintStrength);
                    tb = static_cast<uint8_t>(tb * (1.0f - tintStrength) + tintColor.b * tintStrength);
                }

                m_framebuffer[frameY * m_width + px] = (0xFFu << 24) | (tb << 16) | (tg << 8) | tr;
            } else if (skirtH > 0.0f && py > cy) {
                if (dx <= 1.0f && (py - cy) <= skirtH + (1.0f - dx) * halfH) {
                    m_framebuffer[frameY * m_width + px] = skirtC;
                }
            }
        }
    }
}

void Renderer::drawTerrainProp(float cx, float cy, int propIndex, float zoom) {
    if (propIndex < 0 || propIndex >= static_cast<int>(m_terrainPropPixels.size())) return;
    const auto& propPixels = m_terrainPropPixels[static_cast<size_t>(propIndex)];
    if (propPixels.size() != 64u * 64u) return;

    float scale = std::clamp(zoom * 0.42f, 0.18f, 1.15f);
    int destW = static_cast<int>(64.0f * scale);
    int destH = static_cast<int>(64.0f * scale);
    if (destW <= 2 || destH <= 2) return;

    int startX = static_cast<int>(cx - destW * 0.5f);
    int startY = static_cast<int>(cy + 6.0f * zoom - destH);
    // Sombra suave en la base de la vegetación/roca para integrarla con el terreno
    int shadowRx = std::max(3, static_cast<int>(10.0f * zoom));
    int shadowRy = std::max(2, static_cast<int>(5.0f * zoom));
    int shadowCy = static_cast<int>(cy + 3.0f * zoom);
    int shadowCx = static_cast<int>(cx);
    for (int oy = -shadowRy; oy <= shadowRy; ++oy) {
        int fy = m_height - 1 - (shadowCy + oy);
        if (fy < 0 || fy >= m_height) continue;
        for (int ox = -shadowRx; ox <= shadowRx; ++ox) {
            int fx = shadowCx + ox;
            if (fx < 0 || fx >= m_width) continue;
            float nd = (ox * ox) / static_cast<float>(shadowRx * shadowRx) +
                       (oy * oy) / static_cast<float>(shadowRy * shadowRy);
            if (nd <= 1.0f) {
                uint32_t bg = m_framebuffer[fy * m_width + fx];
                uint8_t br = static_cast<uint8_t>((bg & 0xFF) * 0.80f);
                uint8_t bgc = static_cast<uint8_t>(((bg >> 8) & 0xFF) * 0.80f);
                uint8_t bb = static_cast<uint8_t>(((bg >> 16) & 0xFF) * 0.80f);
                m_framebuffer[fy * m_width + fx] = (0xFFu << 24) | (bb << 16) | (bgc << 8) | br;
            }
        }
    }

    for (int dy = 0; dy < destH; ++dy) {
        int dstY = startY + dy;
        int frameY = m_height - 1 - dstY;
        if (frameY < 0 || frameY >= m_height) continue;

        int sy = std::clamp(static_cast<int>(dy / scale), 0, 63);

        for (int dx = 0; dx < destW; ++dx) {
            int dstX = startX + dx;
            if (dstX < 0 || dstX >= m_width) continue;

            int sx = std::clamp(static_cast<int>(dx / scale), 0, 63);
            uint32_t src = propPixels[static_cast<size_t>(sy) * 64u + static_cast<size_t>(sx)];
            uint8_t a = (src >> 24) & 0xFF;
            if (a < 20) continue;

            uint8_t sr = src & 0xFF;
            uint8_t sg = (src >> 8) & 0xFF;
            uint8_t sb = (src >> 16) & 0xFF;

            if (a >= 245) {
                m_framebuffer[frameY * m_width + dstX] = (0xFFu << 24) | (sb << 16) | (sg << 8) | sr;
            } else {
                uint32_t bg = m_framebuffer[frameY * m_width + dstX];
                uint8_t br = bg & 0xFF;
                uint8_t bgc = (bg >> 8) & 0xFF;
                uint8_t bb = (bg >> 16) & 0xFF;
                float alpha = a / 255.0f;
                uint8_t outR = static_cast<uint8_t>(sr * alpha + br * (1.0f - alpha));
                uint8_t outG = static_cast<uint8_t>(sg * alpha + bgc * (1.0f - alpha));
                uint8_t outB = static_cast<uint8_t>(sb * alpha + bb * (1.0f - alpha));
                m_framebuffer[frameY * m_width + dstX] = (0xFFu << 24) | (outB << 16) | (outG << 8) | outR;
            }
        }
    }
}

void Renderer::drawDiamondOutline(float cx, float cy, float halfW, float halfH, ColorRGB color) {
    int topX = static_cast<int>(cx), topY = static_cast<int>(cy - halfH);
    int rightX = static_cast<int>(cx + halfW), rightY = static_cast<int>(cy);
    int bottomX = static_cast<int>(cx), bottomY = static_cast<int>(cy + halfH);
    int leftX = static_cast<int>(cx - halfW), leftY = static_cast<int>(cy);

    drawLine(topX, topY, rightX, rightY, color);
    drawLine(rightX, rightY, bottomX, bottomY, color);
    drawLine(bottomX, bottomY, leftX, leftY, color);
    drawLine(leftX, leftY, topX, topY, color);
}

void Renderer::drawText(int x, int y, const std::string& text, ColorRGB color, int scale) {
    uint32_t c = color.toRGBA();
    int cursorX = x;

    for (char ch : text) {
        if (ch == '\n') {
            cursorX = x;
            y += 8 * scale;
            continue;
        }

        uint8_t ascii = static_cast<uint8_t>(ch);
        if (ascii < 32 || ascii > 127) ascii = '?';

        // Treat lowercase as uppercase for simple 5x7 font
        if (ascii >= 'a' && ascii <= 'z') ascii -= 32;

        const uint8_t* colBytes = FONT_DATA[ascii - 32];

        for (int col = 0; col < 5; ++col) {
            uint8_t b = colBytes[col];
            for (int row = 0; row < 7; ++row) {
                if (b & (1 << row)) {
                    for (int sy = 0; sy < scale; ++sy) {
                        for (int sx = 0; sx < scale; ++sx) {
                            int px = cursorX + col * scale + sx;
                            int py = y + row * scale + sy;
                            int frameY = m_height - 1 - py;
                            if (px >= 0 && px < m_width && frameY >= 0 && frameY < m_height) {
                                m_framebuffer[frameY * m_width + px] = c;
                            }
                        }
                    }
                }
            }
        }
        cursorX += (5 + 1) * scale;
    }
}

void Renderer::drawSpriteMonument(float cx, float cy, int shrineLevel, ColorRGB color, float zoom) {
    (void)shrineLevel;
    if (m_templePixels.size() == 128 * 128) {
        // El Templo ocupa estrictamente UNA sola celda isométrica (base circular inscrita en el rombo de 1 celda)
        float scale = std::clamp(zoom * 0.36f, 0.14f, 1.30f);
        int destW = static_cast<int>(128.0f * scale);
        int destH = static_cast<int>(128.0f * scale);
        if (destW <= 4 || destH <= 4) return;

        int startX = static_cast<int>(cx - destW * 0.5f);
        // La base del templo (y=124 en el sprite 128x128) se apoya exactamente dentro del rombo de su celda (cy + 11*zoom)
        int startY = static_cast<int>(cy + 11.0f * zoom - destH);

        // Sombra de contacto elíptica inscrita en la única celda del templo
        int shRx = std::max(4, static_cast<int>(19.0f * zoom));
        int shRy = std::max(2, static_cast<int>(9.5f * zoom));
        int shCx = static_cast<int>(cx);
        int shCy = static_cast<int>(cy + 1.5f * zoom);
        for (int oy = -shRy; oy <= shRy; ++oy) {
            int fy = m_height - 1 - (shCy + oy);
            if (fy < 0 || fy >= m_height) continue;
            for (int ox = -shRx; ox <= shRx; ++ox) {
                int fx = shCx + ox;
                if (fx < 0 || fx >= m_width) continue;
                float nd = (ox * ox) / static_cast<float>(shRx * shRx) +
                           (oy * oy) / static_cast<float>(shRy * shRy);
                if (nd <= 1.0f) {
                    uint32_t bg = m_framebuffer[fy * m_width + fx];
                    uint8_t br = static_cast<uint8_t>((bg & 0xFF) * 0.72f);
                    uint8_t bgc = static_cast<uint8_t>(((bg >> 8) & 0xFF) * 0.72f);
                    uint8_t bb = static_cast<uint8_t>(((bg >> 16) & 0xFF) * 0.72f);
                    m_framebuffer[fy * m_width + fx] = (0xFFu << 24) | (bb << 16) | (bgc << 8) | br;
                }
            }
        }

        for (int dy = 0; dy < destH; ++dy) {
            int dstY = startY + dy;
            int frameY = m_height - 1 - dstY;
            if (frameY < 0 || frameY >= m_height) continue;

            int sy = std::clamp(static_cast<int>(dy / scale), 0, 127);

            for (int dx = 0; dx < destW; ++dx) {
                int dstX = startX + dx;
                if (dstX < 0 || dstX >= m_width) continue;

                int sx = std::clamp(static_cast<int>(dx / scale), 0, 127);
                uint32_t src = m_templePixels[sy * 128 + sx];
                uint8_t a = (src >> 24) & 0xFF;
                if (a < 20) continue;

                uint8_t sr = src & 0xFF;
                uint8_t sg = (src >> 8) & 0xFF;
                uint8_t sb = (src >> 16) & 0xFF;

                // Engastar las gemas facetadas sagradas en los dos medallones solares (63,12 y 63,40)
                int d1x = sx - 63, d1y = sy - 12;
                int d2x = sx - 63, d2y = sy - 40;
                int dist1Sq = d1x * d1x + d1y * d1y;
                int dist2Sq = d2x * d2x + d2y * d2y;
                if (dist1Sq <= 9 || dist2Sq <= 16) {
                    bool facetHighlight = (d1x <= 0 && d1y <= 0 && dist1Sq <= 4) ||
                                          (d2x <= 0 && d2y <= 0 && dist2Sq <= 6);
                    bool darkRim = (dist1Sq >= 7 && dist1Sq <= 9) || (dist2Sq >= 12 && dist2Sq <= 16);
                    if (facetHighlight) {
                        sr = static_cast<uint8_t>(std::min(255, color.r + 85));
                        sg = static_cast<uint8_t>(std::min(255, color.g + 85));
                        sb = static_cast<uint8_t>(std::min(255, color.b + 85));
                    } else if (darkRim) {
                        sr = static_cast<uint8_t>(color.r * 0.55f);
                        sg = static_cast<uint8_t>(color.g * 0.55f);
                        sb = static_cast<uint8_t>(color.b * 0.55f);
                    } else {
                        sr = color.r;
                        sg = color.g;
                        sb = color.b;
                    }
                }

                if (a >= 245) {
                    m_framebuffer[frameY * m_width + dstX] = (0xFFu << 24) | (sb << 16) | (sg << 8) | sr;
                } else {
                    uint32_t bg = m_framebuffer[frameY * m_width + dstX];
                    uint8_t br = bg & 0xFF;
                    uint8_t bgc = (bg >> 8) & 0xFF;
                    uint8_t bb = (bg >> 16) & 0xFF;
                    float alpha = a / 255.0f;
                    uint8_t outR = static_cast<uint8_t>(sr * alpha + br * (1.0f - alpha));
                    uint8_t outG = static_cast<uint8_t>(sg * alpha + bgc * (1.0f - alpha));
                    uint8_t outB = static_cast<uint8_t>(sb * alpha + bb * (1.0f - alpha));
                    m_framebuffer[frameY * m_width + dstX] = (0xFFu << 24) | (outB << 16) | (outG << 8) | outR;
                }
            }
        }
        return;
    }

    int icx = static_cast<int>(cx);
    int icy = static_cast<int>(cy);
    ColorRGB gold(255, 215, 0);
    drawRect(icx - 8, icy - 10, 16, 10, color);
    drawRect(icx - 4, icy - 14, 8, 4, gold);
}

void Renderer::drawVillagerSprite(float footX, float footY, const std::string& spriteName, ColorRGB auraColor, float scale) {
    const VillagerFrame* frame = getVillagerSprite(spriteName);
    if (!frame) return;

    // Sprite dimensions
    float sprW = static_cast<float>(VILLAGER_SPRITE_W) * scale;
    float sprH = static_cast<float>(VILLAGER_SPRITE_H) * scale;

    int startX = static_cast<int>(footX - sprW * 0.5f);
    int startY = static_cast<int>(footY - sprH);

    // Subtle spiritual aura ring under feet
    int footIntX = static_cast<int>(footX);
    int footIntY = static_cast<int>(footY);
    for (int ox = -4; ox <= 4; ++ox) {
        for (int oy = -1; oy <= 1; ++oy) {
            int px = footIntX + ox;
            int py = footIntY + oy;
            int fy = m_height - 1 - py;
            if (px >= 0 && px < m_width && fy >= 0 && fy < m_height) {
                uint32_t bg = m_framebuffer[fy * m_width + px];
                uint8_t br = bg & 0xFF;
                uint8_t bgc = (bg >> 8) & 0xFF;
                uint8_t bb = (bg >> 16) & 0xFF;

                uint8_t nr = static_cast<uint8_t>((br * 3 + auraColor.r) / 4);
                uint8_t ng = static_cast<uint8_t>((bgc * 3 + auraColor.g) / 4);
                uint8_t nb = static_cast<uint8_t>((bb * 3 + auraColor.b) / 4);
                m_framebuffer[fy * m_width + px] = (0xFF << 24) | (nb << 16) | (ng << 8) | nr;
            }
        }
    }

    // Alpha-blended blit of villager sprite
    for (int sy = 0; sy < VILLAGER_SPRITE_H; ++sy) {
        int dy = startY + static_cast<int>(sy * scale);
        int frameY = m_height - 1 - dy;
        if (frameY < 0 || frameY >= m_height) continue;

        for (int sx = 0; sx < VILLAGER_SPRITE_W; ++sx) {
            int dx = startX + static_cast<int>(sx * scale);
            if (dx < 0 || dx >= m_width) continue;

            uint32_t pixel = frame->pixels[sy * VILLAGER_SPRITE_W + sx];
            uint8_t a = pixel & 0xFF;
            if (a < 15) continue; // transparent pixel

            uint8_t sr = (pixel >> 24) & 0xFF;
            uint8_t sg = (pixel >> 16) & 0xFF;
            uint8_t sb = (pixel >> 8)  & 0xFF;

            if (a >= 250) {
                m_framebuffer[frameY * m_width + dx] = (0xFF << 24) | (sb << 16) | (sg << 8) | sr;
            } else {
                // Alpha blend with background
                uint32_t bg = m_framebuffer[frameY * m_width + dx];
                uint8_t br = bg & 0xFF;
                uint8_t bgc = (bg >> 8) & 0xFF;
                uint8_t bb = (bg >> 16) & 0xFF;

                float alpha = a / 255.0f;
                uint8_t outR = static_cast<uint8_t>(sr * alpha + br * (1.0f - alpha));
                uint8_t outG = static_cast<uint8_t>(sg * alpha + bgc * (1.0f - alpha));
                uint8_t outB = static_cast<uint8_t>(sb * alpha + bb * (1.0f - alpha));
                m_framebuffer[frameY * m_width + dx] = (0xFF << 24) | (outB << 16) | (outG << 8) | outR;
            }
        }
    }
}

void Renderer::render(const WorldMap& map, const ReligionRegistry& registry,
                      const Timeline& timeline, const EventLine& eventLine,
                      const ReplicationEngine& replication, const GestorHumanos& humanos,
                      const IsometricCamera& camera,
                      const Point2i& selectedCell, RenderMapMode mode,
                      const WorldCreationConfig& playerConfig) {
    renderWorld(map, registry, replication, humanos, camera, selectedCell, mode);
    renderUI(map, registry, timeline, eventLine, selectedCell, mode, humanos, camera, playerConfig);
    renderOptionsDropdown(); // siempre encima del resto del HUD
}

void Renderer::renderWorld(const WorldMap& map, const ReligionRegistry& registry,
                           const ReplicationEngine& replication, const GestorHumanos& humanos,
                           const IsometricCamera& camera,
                           const Point2i& selectedCell, RenderMapMode mode) {
    int w = map.getWidth();
    int h = map.getHeight();
    float zoom = camera.getZoom();
    float halfW = BASE_TILE_WIDTH * 0.5f * zoom;
    float halfH = BASE_TILE_HEIGHT * 0.5f * zoom;
    float camX = camera.getCameraX();
    float camY = camera.getCameraY();

    auto getCellTexIdx = [&](int cx, int cy) -> int {
        const Cell& cell = map.getCell(cx, cy);
        switch (cell.biome) {
            case BiomeType::DeepOcean:      return 13;
            case BiomeType::ShallowWater:   return 10;
            case BiomeType::CoastalPlains:  return 2; // Pradera costera suave
            case BiomeType::FertileValley:  return 0; // Pasto (Grass) - Base continuo
            case BiomeType::DenseForest:    return 5; // Pasto rico de bosque
            case BiomeType::HighlandHills:  return 6; // Pastizal alto / colina
            case BiomeType::AridDesert:     return 8;
            case BiomeType::RuggedMountains:return 1; // Suelo rocoso
            case BiomeType::FrozenTundra:   return 9; // Arena clara / nieve
            default:                        return 6;
        }
    };

    auto sampleAtlasRGB = [&](int tIdx, int wx, int wy, float& outR, float& outG, float& outB) {
        uint32_t texel = m_terrainAtlasPixels[(wy & 63) * TERRAIN_ATLAS_WIDTH + (tIdx * 64) + (wx & 63)];
        outR = static_cast<float>(texel & 0xFF);
        outG = static_cast<float>((texel >> 8) & 0xFF);
        outB = static_cast<float>((texel >> 16) & 0xFF);
    };

    // Niebla de Guerra de la Fe: combina visión actual (vis) y memoria de exploración (expl)
    //  - Zona visible: color pleno.
    //  - Zona explorada pero sin creyentes cerca: recuerdo apagado y desaturado.
    //  - Zona nunca vista: bruma oscura #120b0a con leve textura neblinosa.
    const bool fogOn = fogActive();
    auto applyFog = [&](float& r, float& g, float& b, float vis, float expl, int px, int py) {
        if (vis >= 0.999f) return;
        float lum = 0.30f * r + 0.59f * g + 0.11f * b;
        float memR = (lum * 0.55f + r * 0.45f) * 0.46f;
        float memG = (lum * 0.55f + g * 0.45f) * 0.44f;
        float memB = (lum * 0.55f + b * 0.45f) * 0.48f;

        int wxN = static_cast<int>(std::floor((static_cast<float>(px) - camX) / (zoom * 3.0f)));
        int wyN = static_cast<int>(std::floor((static_cast<float>(py) - camY) / (zoom * 3.0f)));
        uint32_t hsh = static_cast<uint32_t>(wxN) * 73856093u ^ static_cast<uint32_t>(wyN) * 19349663u;
        hsh ^= hsh >> 13;
        float mist = static_cast<float>(hsh % 100u) / 100.0f;
        float mistR = 18.0f + mist * 12.0f;
        float mistG = 11.0f + mist * 9.0f;
        float mistB = 10.0f + mist * 10.0f;

        float fr = mistR * (1.0f - expl) + memR * expl;
        float fg = mistG * (1.0f - expl) + memG * expl;
        float fb = mistB * (1.0f - expl) + memB * expl;
        r = fr * (1.0f - vis) + r * vis;
        g = fg * (1.0f - vis) + g * vis;
        b = fb * (1.0f - vis) + b * vis;
    };

    // =========================================================================
    // PASADA 1: TERRENO COHESIVO Y ARMONIOSO (Mapeo continuo + Auto-transiciones)
    // =========================================================================
    for (int depth = 0; depth < w + h - 1; ++depth) {
        for (int x = 0; x <= depth; ++x) {
            int y = depth - x;
            if (x >= w || y >= h) continue;

            const Cell& c = map.getCell(x, y);
            Vec2 screenPt = camera.gridTileCenterScreen(x, y, c.elevation);
            float frontDrop = 0.0f;
            if (!c.isWater()) {
                if (x + 1 < w) {
                    frontDrop = std::max(frontDrop, c.elevation - map.getCell(x + 1, y).elevation);
                } else {
                    frontDrop = std::max(frontDrop, c.elevation);
                }
                if (y + 1 < h) {
                    frontDrop = std::max(frontDrop, c.elevation - map.getCell(x, y + 1).elevation);
                } else {
                    frontDrop = std::max(frontDrop, c.elevation);
                }
            }
            float skirtH = std::max(0.0f, frontDrop * ELEVATION_STEP) * zoom;
            if (skirtH < zoom) skirtH = 0.0f;

            // Frustum culling
            if (screenPt.x + halfW < 0 || screenPt.x - halfW > m_width ||
                screenPt.y + halfH + skirtH < 0 || screenPt.y - halfH > m_height) {
                continue;
            }

            int texIdx = getCellTexIdx(x, y);
            bool isWater = c.isWater();

            // Información de los 4 vecinos cardinales para transiciones suaves sin costuras
            int nbTexXNeg = (x > 0 && map.getCell(x - 1, y).isWater() == isWater) ? getCellTexIdx(x - 1, y) : texIdx;
            int nbTexXPos = (x + 1 < w && map.getCell(x + 1, y).isWater() == isWater) ? getCellTexIdx(x + 1, y) : texIdx;
            int nbTexYNeg = (y > 0 && map.getCell(x, y - 1).isWater() == isWater) ? getCellTexIdx(x, y - 1) : texIdx;
            int nbTexYPos = (y + 1 < h && map.getCell(x, y + 1).isWater() == isWater) ? getCellTexIdx(x, y + 1) : texIdx;

            bool shoreXNeg = (x > 0 && map.getCell(x - 1, y).isWater() != isWater);
            bool shoreXPos = (x + 1 < w && map.getCell(x + 1, y).isWater() != isWater);
            bool shoreYNeg = (y > 0 && map.getCell(x, y - 1).isWater() != isWater);
            bool shoreYPos = (y + 1 < h && map.getCell(x, y + 1).isWater() != isWater);

            ColorRGB overlayColor = c.getBiomeBaseColor();
            float overlayStrength = 0.0f;
            bool hasFaithBorder = false;

            if (mode == RenderMapMode::SyncretismHeatmap) {
                if (!isWater && c.population > 0) {
                    float synFriction = std::clamp(c.syncreticTension + (c.faithShare.size() > 1 ? 0.4f : 0.0f), 0.0f, 1.0f);
                    overlayColor = ColorRGB(255, 60, 220);
                    overlayStrength = synFriction * 0.55f;
                }
            } else if (mode == RenderMapMode::AdvancementEra) {
                if (!isWater) {
                    overlayColor = ColorRGB(240, 200, 70);
                    overlayStrength = c.development * 0.45f;
                }
            }

            if (m_terrainAtlasPixels.size() != TERRAIN_ATLAS_WIDTH * TERRAIN_ATLAS_HEIGHT) {
                drawFilledDiamond(screenPt.x, screenPt.y, halfW, halfH, overlayColor, Palette::DIRT_EARTH, skirtH);
            } else {
                int minX = std::max(0, static_cast<int>(std::floor(screenPt.x - halfW)));
                int maxX = std::min(m_width - 1, static_cast<int>(std::ceil(screenPt.x + halfW)));
                int minY = std::max(0, static_cast<int>(std::floor(screenPt.y - halfH)));
                int maxY = std::min(m_height - 1, static_cast<int>(std::ceil(screenPt.y + halfH + skirtH)));

                for (int py = minY; py <= maxY; ++py) {
                    int frameY = m_height - 1 - py;
                    if (frameY < 0 || frameY >= m_height) continue;

                    float relY = (static_cast<float>(py) - screenPt.y) / halfH;
                    float dy = std::abs(relY);

                    for (int px = minX; px <= maxX; ++px) {
                        float relX = (static_cast<float>(px) - screenPt.x) / halfW;
                        float dx = std::abs(relX);
                        float diamondDist = dx + dy;

                        if (diamondDist <= 1.045f) {
                            int wx = std::clamp(static_cast<int>((relX + 1.0f) * 32.0f), 0, 63);
                            int wy = std::clamp(static_cast<int>((relY + 1.0f) * 16.0f), 0, 31);

                            float r, g, b;
                            sampleAtlasRGB(texIdx, wx, wy, r, g, b);

                            // Coordenadas locales isométricas dentro de la celda: du, dv en [-0.5, 0.5]
                            float du = (relX + relY) * 0.5f;
                            float dv = (relY - relX) * 0.5f;

                            auto blendBiomeEdge = [&](float edgeDistance, int neighborTexture) {
                                if (neighborTexture == texIdx) return;
                                const float t = std::clamp((edgeDistance - 0.12f) / 0.38f, 0.0f, 1.0f);
                                const float weight = t * t * (3.0f - 2.0f * t) * 0.34f;
                                if (weight <= 0.0f) return;

                                float nr, ng, nb;
                                sampleAtlasRGB(neighborTexture, wx, wy, nr, ng, nb);
                                r = r * (1.0f - weight) + nr * weight;
                                g = g * (1.0f - weight) + ng * weight;
                                b = b * (1.0f - weight) + nb * weight;
                            };

                            // Mezclar biomas cercanos con una transición amplia y suave, sin variar
                            // arbitrariamente la textura de todas las celdas de una misma región.
                            if (du < -0.15f && nbTexXNeg != texIdx) {
                                blendBiomeEdge(-du, nbTexXNeg);
                            } else if (du > 0.15f && nbTexXPos != texIdx) {
                                blendBiomeEdge(du, nbTexXPos);
                            }
                            if (dv < -0.15f && nbTexYNeg != texIdx) {
                                blendBiomeEdge(-dv, nbTexYNeg);
                            } else if (dv > 0.15f && nbTexYPos != texIdx) {
                                blendBiomeEdge(dv, nbTexYPos);
                            }

                            // 2. Transición natural de costa (orilla de tierra/arena y espuma de olas en el agua)
                            float shoreFactor = 0.0f;
                            if (shoreXNeg && du < -0.18f) shoreFactor = std::max(shoreFactor, (-0.18f - du) / 0.32f);
                            if (shoreXPos && du > 0.18f)  shoreFactor = std::max(shoreFactor, (du - 0.18f) / 0.32f);
                            if (shoreYNeg && dv < -0.18f) shoreFactor = std::max(shoreFactor, (-0.18f - dv) / 0.32f);
                            if (shoreYPos && dv > 0.18f)  shoreFactor = std::max(shoreFactor, (dv - 0.18f) / 0.32f);

                            if (shoreFactor > 0.0f) {
                                shoreFactor = std::clamp(shoreFactor, 0.0f, 1.0f);
                                if (!isWater) {
                                    // Borde de tierra y arena costera cálida como en Pasto (Grass) - Base
                                    float sandW = shoreFactor * 0.65f;
                                    r = r * (1.0f - sandW) + 146.0f * sandW;
                                    g = g * (1.0f - sandW) + 135.0f * sandW;
                                    b = b * (1.0f - sandW) + 104.0f * sandW;
                                    if (shoreFactor > 0.78f) {
                                        r *= 0.78f; g *= 0.75f; b *= 0.72f;
                                    }
                                } else {
                                    // Espuma y ondas costeras turquesas como en Agua - Transitional
                                    float waveBand = std::sin(shoreFactor * 9.5f + (wx * 0.15f)) * 0.5f + 0.5f;
                                    float foamW = shoreFactor * (0.35f + 0.35f * waveBand);
                                    r = r * (1.0f - foamW) + 168.0f * foamW;
                                    g = g * (1.0f - foamW) + 226.0f * foamW;
                                    b = b * (1.0f - foamW) + 220.0f * foamW;
                                }
                            }

                            // 3. Zona de influencia de fe (sólo en celdas con aldeanos)
                            if (overlayStrength > 0.01f) {
                                float effStrength = overlayStrength;
                                if (hasFaithBorder && diamondDist > 0.80f && diamondDist <= 0.95f) {
                                    effStrength = 0.68f; // Halo espiritual nítido en el borde de la celda ocupada
                                }
                                r = r * (1.0f - effStrength) + overlayColor.r * effStrength;
                                g = g * (1.0f - effStrength) + overlayColor.g * effStrength;
                                b = b * (1.0f - effStrength) + overlayColor.b * effStrength;
                            }

                            // 4. Niebla de Guerra de la Fe con bordes suaves (muestreo bilineal continuo)
                            if (fogOn) {
                                float gxC = static_cast<float>(x) + 0.5f + du;
                                float gyC = static_cast<float>(y) + 0.5f + dv;
                                applyFog(r, g, b, m_fog->sampleVisibility(gxC, gyC),
                                         m_fog->sampleExplored(gxC, gyC), px, py);
                            }

                            uint8_t outR = static_cast<uint8_t>(std::clamp(r, 0.0f, 255.0f));
                            uint8_t outG = static_cast<uint8_t>(std::clamp(g, 0.0f, 255.0f));
                            uint8_t outB = static_cast<uint8_t>(std::clamp(b, 0.0f, 255.0f));
                            m_framebuffer[frameY * m_width + px] = (0xFFu << 24) | (outB << 16) | (outG << 8) | outR;
                        } else if (!isWater && skirtH > 0.0f && py > screenPt.y) {
                            // Faldón de tierra estratificada (#564637 -> #3B2F25) con reborde de pasto superior
                            float drop = static_cast<float>(py) - (screenPt.y + (1.0f - dx) * halfH);
                            if (dx <= 1.0f && drop >= 0.0f && drop <= skirtH) {
                                float tDrop = drop / std::max(1.0f, skirtH);
                                uint8_t sr, sg, sb;
                                if (drop < 1.8f * zoom) {
                                    // Labio superior de pasto oscuro
                                    sr = 78; sg = 102; sb = 48;
                                } else {
                                    // Pared de tierra con sombreado lateral isométrico (más luz a la izquierda, sombra a la derecha)
                                    float sideShade = (relX < 0.0f) ? 1.06f : 0.84f;
                                    float depthShade = (1.0f - tDrop * 0.28f) * sideShade;
                                    sr = static_cast<uint8_t>(std::clamp(88.0f * depthShade, 0.0f, 255.0f));
                                    sg = static_cast<uint8_t>(std::clamp(72.0f * depthShade, 0.0f, 255.0f));
                                    sb = static_cast<uint8_t>(std::clamp(56.0f * depthShade, 0.0f, 255.0f));
                                }
                                if (fogOn) {
                                    float fr = sr, fg = sg, fb = sb;
                                    applyFog(fr, fg, fb, m_fog->visibility(x, y), m_fog->explored(x, y), px, py);
                                    sr = static_cast<uint8_t>(std::clamp(fr, 0.0f, 255.0f));
                                    sg = static_cast<uint8_t>(std::clamp(fg, 0.0f, 255.0f));
                                    sb = static_cast<uint8_t>(std::clamp(fb, 0.0f, 255.0f));
                                }
                                m_framebuffer[frameY * m_width + px] = (0xFFu << 24) | (sb << 16) | (sg << 8) | sr;
                            }
                        }
                    }
                }
            }
        }
    }

    // =========================================================================
    // PASADA 1.5: AURA DE INFLUENCIA TRIANGULADA ENTRE NODOS DE LA FE
    // La zona de influencia no es la celda en sí, sino un aura orgánica que
    // cubre el área de cada creyente/chamán/templo y se triangula dinámicamente
    // con cada nodo cercano de su misma fe.
    // =========================================================================
    if (mode == RenderMapMode::ReligionDominance) {
        renderTriangulatedFaithAuras(map, registry, humanos, camera);
    }

    // Resaltado completo de la celda seleccionada sobre la superficie del terreno (debajo de edificios y unidades)
    if (map.isValid(selectedCell.x, selectedCell.y)) {
        const Cell& selC = map.getCell(selectedCell.x, selectedCell.y);
        Vec2 selPt = camera.gridTileCenterScreen(selectedCell.x, selectedCell.y, selC.elevation);
        drawDiamondOutline(selPt.x, selPt.y, halfW + 1, halfH + 1, ColorRGB(0, 255, 255));
        drawDiamondOutline(selPt.x, selPt.y, halfW, halfH, ColorRGB(255, 255, 255));
    }

    // =========================================================================
    // PASADA 2: COLA UNIFICADA DE PROFUNDIDAD ISOMÉTRICA (Edificios, Árboles y Unidades)
    // Garantiza que cuando un aldeano u otra unidad pase por detrás de los templos
    // o árboles, las partes ocultas del sprite queden cubiertas de manera natural.
    // =========================================================================
    enum class IsoSpriteKind : uint8_t {
        Villager = 0,
        Caravan  = 1,
        Prop     = 2,
        Temple   = 3
    };

    struct IsoSpriteItem {
        float isoDepth;
        IsoSpriteKind kind;
        int cellX;
        int cellY;
        int subId;
        const Humano* humanoPtr;
        const MissionaryCaravan* caravanPtr;
    };

    std::vector<IsoSpriteItem> sceneSprites;
    sceneSprites.reserve(humanos.getHumanos().size() + replication.getActiveCaravans().size() + 256);

    // 1. Agregar Templos (1 celda) y Vegetación/Rocas del mapa
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            const Cell& c = map.getCell(x, y);
            if (c.isWater()) continue;
            // Nada se dibuja sobre la niebla nunca explorada
            if (fogOn && !m_fog->isExplored(x, y)) continue;

            if (c.hasHolySite) {
                // La base frontal del templo se apoya en (x + 0.65, y + 0.65) -> profundidad = x + y + 1.30f
                // Cualquier unidad con hx + hy < x + y + 1.30f está detrás de la fachada y queda oculta de forma natural
                sceneSprites.push_back({
                    static_cast<float>(x + y) + 1.30f,
                    IsoSpriteKind::Temple,
                    x, y, c.shrineLevel, nullptr, nullptr
                });
            } else {
                uint32_t cellHash = static_cast<uint32_t>(x * 374761393u + y * 668265263u);
                uint32_t roll = (cellHash >> 8) % 100;
                int propId = -1;
                if (c.biome == BiomeType::DenseForest && roll < 56) {
                    propId = (roll < 18) ? 0 : ((roll < 36) ? 1 : ((roll < 46) ? 5 : 3));
                } else if (c.biome == BiomeType::FertileValley && roll < 14) {
                    propId = (roll < 6) ? 5 : ((roll < 10) ? 1 : 4);
                } else if (c.biome == BiomeType::CoastalPlains && roll < 10) {
                    propId = (roll < 5) ? 4 : 2;
                } else if ((c.biome == BiomeType::HighlandHills || c.biome == BiomeType::RuggedMountains) && roll < 24) {
                    propId = (roll < 14) ? 6 : ((roll < 19) ? 0 : 2); // Rocas naturales o árbol retorcido
                }

                if (propId >= 0) {
                    sceneSprites.push_back({
                        static_cast<float>(x + y) + 1.00f,
                        IsoSpriteKind::Prop,
                        x, y, propId, nullptr, nullptr
                    });
                }
            }
        }
    }

    // 2. Agregar Aldeanos (unidades Humano) con su profundidad continua exacta (hx + hy)
    for (const auto& hObj : humanos.getHumanos()) {
        // Las unidades de otras fes sólo se ven dentro del alcance de la fe del jugador
        if (isHiddenByFog(hObj.getCeldaX(), hObj.getCeldaY(), hObj.getReligionId())) continue;
        sceneSprites.push_back({
            hObj.getX() + hObj.getY(),
            IsoSpriteKind::Villager,
            hObj.getCeldaX(), hObj.getCeldaY(), hObj.getId(), &hObj, nullptr
        });
    }

    // 3. Agregar Caravanas Misioneras con su profundidad continua interpolada
    int carIdx = 0;
    for (const auto& car : replication.getActiveCaravans()) {
        float gx = (static_cast<float>(car.from.x) + 0.5f) + (car.to.x - car.from.x) * car.progress;
        float gy = (static_cast<float>(car.from.y) + 0.5f) + (car.to.y - car.from.y) * car.progress;
        if (isHiddenByFog(static_cast<int>(gx), static_cast<int>(gy), car.religionId)) {
            carIdx++;
            continue;
        }
        sceneSprites.push_back({
            gx + gy,
            IsoSpriteKind::Caravan,
            car.from.x, car.from.y, carIdx++, nullptr, &car
        });
    }

    // Ordenar de atrás hacia adelante (Norte a Sur) para superposición natural y creíble
    std::sort(sceneSprites.begin(), sceneSprites.end(), [](const IsoSpriteItem& a, const IsoSpriteItem& b) {
        if (std::abs(a.isoDepth - b.isoDepth) > 0.0005f) {
            return a.isoDepth < b.isoDepth;
        }
        if (a.kind != b.kind) {
            return static_cast<uint8_t>(a.kind) < static_cast<uint8_t>(b.kind);
        }
        return a.subId < b.subId;
    });

    const Humano* selPtr = humanos.getHumanoSeleccionado();
    int idSeleccionado = selPtr ? selPtr->getId() : -1;

    for (const auto& item : sceneSprites) {
        if (item.kind == IsoSpriteKind::Villager && item.humanoPtr) {
            const Humano& hObj = *item.humanoPtr;
            ColorRGB aura(230, 161, 59);
            const Religion* r = registry.getReligion(hObj.getReligionId());
            if (r) aura = r->color;
            bool esSel = (hObj.getId() == idSeleccionado);
            const TileSet& ts = hObj.esChaman() ? m_shamanTileSet : m_tileSet;

            // Vista UNIDADES: anillo isométrico con el color de fe bajo cada aldeano (doble anillo para chamanes)
            if (mode == RenderMapMode::Units) {
                int ucx = std::clamp(hObj.getCeldaX(), 0, w - 1);
                int ucy = std::clamp(hObj.getCeldaY(), 0, h - 1);
                Vec2 ufoot = camera.gridToScreen(hObj.getX(), hObj.getY(), map.getCell(ucx, ucy).elevation);
                float ur = (hObj.esChaman() ? 15.0f : 10.0f) * zoom;
                drawDiamondOutline(ufoot.x, ufoot.y, ur, ur * 0.5f, aura);
                drawDiamondOutline(ufoot.x, ufoot.y, ur + 1.0f, ur * 0.5f + 0.5f, aura);
                if (hObj.esChaman()) {
                    drawDiamondOutline(ufoot.x, ufoot.y, ur * 0.6f, ur * 0.3f, Palette::DIVINE_GOLD);
                }
            }
            hObj.renderizar(m_framebuffer.data(), m_width, m_height, camera, map, ts, aura, esSel);

            // Si es un Chamán canalizando una señal divina, proyectar haz solar sagrado
            if (hObj.esChaman() && hObj.getTimerSenalDivina() > 0.0f) {
                int cx = std::clamp(hObj.getCeldaX(), 0, w - 1);
                int cy = std::clamp(hObj.getCeldaY(), 0, h - 1);
                Vec2 sp = camera.gridToScreen(hObj.getX(), hObj.getY(), map.getCell(cx, cy).elevation);
                int beamX = static_cast<int>(sp.x);
                int beamYBot = static_cast<int>(sp.y);
                int beamYTop = std::max(0, beamYBot - static_cast<int>(64.0f * zoom));
                int halfBeamW = std::max(2, static_cast<int>(6.0f * zoom));
                for (int by = beamYTop; by <= beamYBot; ++by) {
                    int fy = m_height - 1 - by;
                    if (fy < 0 || fy >= m_height) continue;
                    float vFade = 1.0f - (beamYBot - by) / std::max(1.0f, static_cast<float>(beamYBot - beamYTop));
                    for (int bx = beamX - halfBeamW; bx <= beamX + halfBeamW; ++bx) {
                        if (bx < 0 || bx >= m_width) continue;
                        float hFade = 1.0f - std::abs(bx - beamX) / static_cast<float>(halfBeamW + 1);
                        float bAlpha = hFade * (0.20f + 0.25f * vFade);
                        uint32_t bg = m_framebuffer[fy * m_width + bx];
                        uint8_t br = bg & 0xFF;
                        uint8_t bgc = (bg >> 8) & 0xFF;
                        uint8_t bb = (bg >> 16) & 0xFF;
                        uint8_t outR = static_cast<uint8_t>(std::min(255.0f, br * (1.0f - bAlpha) + 255.0f * bAlpha));
                        uint8_t outG = static_cast<uint8_t>(std::min(255.0f, bgc * (1.0f - bAlpha) + 225.0f * bAlpha));
                        uint8_t outB = static_cast<uint8_t>(std::min(255.0f, bb * (1.0f - bAlpha) + 110.0f * bAlpha));
                        m_framebuffer[fy * m_width + bx] = (0xFFu << 24) | (outB << 16) | (outG << 8) | outR;
                    }
                }
            }

            // Si es la unidad seleccionada y tiene una orden manual de movimiento (truco SELECTOR),
            // mostrar la línea de ruta y el marcador de destino sobre el terreno
            if (esSel && hObj.tieneOrdenManual()) {
                int cx = std::clamp(hObj.getCeldaX(), 0, w - 1);
                int cy = std::clamp(hObj.getCeldaY(), 0, h - 1);
                int dxCell = std::clamp(static_cast<int>(std::floor(hObj.getDestinoX())), 0, w - 1);
                int dyCell = std::clamp(static_cast<int>(std::floor(hObj.getDestinoY())), 0, h - 1);
                Vec2 pStart = camera.gridToScreen(hObj.getX(), hObj.getY(), map.getCell(cx, cy).elevation);
                Vec2 pDest  = camera.gridToScreen(hObj.getDestinoX(), hObj.getDestinoY(), map.getCell(dxCell, dyCell).elevation);
                drawLine(static_cast<int>(pStart.x), static_cast<int>(pStart.y),
                         static_cast<int>(pDest.x), static_cast<int>(pDest.y), ColorRGB(0, 255, 255));
                drawDiamondOutline(pDest.x, pDest.y, 10.0f * zoom, 5.0f * zoom, ColorRGB(0, 255, 255));
                drawDiamondOutline(pDest.x, pDest.y, 6.0f * zoom, 3.0f * zoom, Palette::DIVINE_GOLD);
            }
        } else if (item.kind == IsoSpriteKind::Caravan && item.caravanPtr) {
            const MissionaryCaravan& car = *item.caravanPtr;
            const Cell& fromC = map.getCell(car.from.x, car.from.y);
            const Cell& toC = map.getCell(car.to.x, car.to.y);
            Vec2 p0 = camera.gridTileCenterScreen(car.from.x, car.from.y, fromC.elevation);
            Vec2 p1 = camera.gridTileCenterScreen(car.to.x, car.to.y, toC.elevation);
            float curX = p0.x + (p1.x - p0.x) * car.progress;
            float curY = p0.y + (p1.y - p0.y) * car.progress;

            ColorRGB carColor(255, 235, 140);
            const Religion* r = registry.getReligion(car.religionId);
            if (r) carColor = r->color;

            float escala = std::clamp(zoom * 0.44f, 0.20f, 1.15f);
            TileRect rect = m_tileSet.obtenerRectangulo(AccionHumano::CARGAR, DireccionIso::SE);
            m_tileSet.blitTile(m_framebuffer.data(), m_width, m_height, rect, curX, curY, escala, carColor, false);
        } else if (item.kind == IsoSpriteKind::Prop) {
            const Cell& c = map.getCell(item.cellX, item.cellY);
            Vec2 screenPt = camera.gridTileCenterScreen(item.cellX, item.cellY, c.elevation);
            if (screenPt.x + halfW >= 0 && screenPt.x - halfW <= m_width &&
                screenPt.y + halfH >= 0 && screenPt.y - halfH * 3.0f <= m_height) {
                drawTerrainProp(screenPt.x, screenPt.y, item.subId, zoom);
            }
        } else if (item.kind == IsoSpriteKind::Temple) {
            const Cell& c = map.getCell(item.cellX, item.cellY);
            Vec2 screenPt = camera.gridTileCenterScreen(item.cellX, item.cellY, c.elevation);
            if (screenPt.x + halfW * 2.0f >= 0 && screenPt.x - halfW * 2.0f <= m_width &&
                screenPt.y + halfH * 2.0f >= 0 && screenPt.y - halfH * 4.0f <= m_height) {
                ColorRGB holyColor(255, 230, 100);
                int hsRel = (c.dominantReligionId > 0) ? c.dominantReligionId : c.holySiteReligionId;
                if (hsRel > 0) {
                    const Religion* r = registry.getReligion(hsRel);
                    if (r) holyColor = r->color;
                }
                drawSpriteMonument(screenPt.x, screenPt.y, item.subId, holyColor, zoom);
            }
        }
    }
}

void Renderer::renderTriangulatedFaithAuras(const WorldMap& map, const ReligionRegistry& registry,
                                            const GestorHumanos& humanos, const IsometricCamera& camera) {
    int w = map.getWidth();
    int h = map.getHeight();
    float zoom = camera.getZoom();

    struct AuraNode {
        float gx, gy;
        float sx, sy;
        float radiusPx;
        float devotion;
        bool isShaman;
        bool isTemple;
        float signalTimer;
    };

    std::map<int, std::vector<AuraNode>> nodesByRel;

    // 1. Recolectar todos los nodos vivos (Aldeanos y Chamanes)
    for (const auto& hObj : humanos.getHumanos()) {
        int rId = hObj.getReligionId();
        if (rId <= 0) continue;
        int cx = std::clamp(hObj.getCeldaX(), 0, w - 1);
        int cy = std::clamp(hObj.getCeldaY(), 0, h - 1);
        if (isHiddenByFog(cx, cy, rId)) continue; // aura rival oculta en la niebla
        float elev = map.getCell(cx, cy).elevation;
        Vec2 sp = camera.gridToScreen(hObj.getX(), hObj.getY(), elev);

        float rad = (hObj.esChaman() ? 48.0f : 34.0f) * zoom;
        if (hObj.getTimerSenalDivina() > 0.0f) {
            rad = 64.0f * zoom;
        }
        nodesByRel[rId].push_back({
            hObj.getX(), hObj.getY(),
            sp.x, sp.y,
            rad, hObj.getDevocion(),
            hObj.esChaman(), false,
            hObj.getTimerSenalDivina()
        });
    }

    // 2. Anclar también el Templo sagrado si hay creyentes de su fe en las cercanías
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            const Cell& c = map.getCell(x, y);
            if (!c.hasHolySite || c.isWater()) continue;
            int rId = (c.holySiteReligionId > 0) ? c.holySiteReligionId : c.dominantReligionId;
            if (rId <= 0) continue;
            if (isHiddenByFog(x, y, rId)) continue;
            auto it = nodesByRel.find(rId);
            if (it == nodesByRel.end() || it->second.empty()) continue;

            float tx = static_cast<float>(x) + 0.5f;
            float ty = static_cast<float>(y) + 0.5f;
            bool hasCloseBeliever = false;
            for (const auto& n : it->second) {
                float dx = n.gx - tx;
                float dy = n.gy - ty;
                if (dx * dx + dy * dy <= 36.0f) {
                    hasCloseBeliever = true;
                    break;
                }
            }
            if (hasCloseBeliever) {
                Vec2 sp = camera.gridTileCenterScreen(x, y, c.elevation);
                it->second.push_back({
                    tx, ty,
                    sp.x, sp.y,
                    36.0f * zoom, 1.0f,
                    false, true, 0.0f
                });
            }
        }
    }

    // 3. Para cada religión, construir su triangulación (Gabriel Graph + triángulos de fe)
    //    y rasterizar el campo de aura continua sobre la superficie del terreno
    std::vector<float> field;

    for (const auto& kv : nodesByRel) {
        int rId = kv.first;
        const auto& nodes = kv.second;
        if (nodes.empty()) continue;

        const Religion* rel = registry.getReligion(rId);
        if (!rel) continue;
        ColorRGB relColor = rel->color;

        int minX = m_width;
        int maxX = -1;
        int minY = m_height;
        int maxY = -1;
        for (const auto& n : nodes) {
            int padX = static_cast<int>(std::ceil(n.radiusPx + 6.0f));
            int padY = static_cast<int>(std::ceil(n.radiusPx * 0.56f + 6.0f));
            minX = std::min(minX, static_cast<int>(std::floor(n.sx)) - padX);
            maxX = std::max(maxX, static_cast<int>(std::ceil(n.sx)) + padX);
            minY = std::min(minY, static_cast<int>(std::floor(n.sy)) - padY);
            maxY = std::max(maxY, static_cast<int>(std::ceil(n.sy)) + padY);
        }
        minX = std::max(0, minX);
        maxX = std::min(m_width - 1, maxX);
        minY = std::max(0, minY);
        maxY = std::min(m_height - 1, maxY);
        if (minX > maxX || minY > maxY) continue;

        int boxW = maxX - minX + 1;
        int boxH = maxY - minY + 1;
        field.assign(static_cast<size_t>(boxW * boxH), 0.0f);

        size_t N = nodes.size();
        std::vector<std::vector<bool>> adj(N, std::vector<bool>(N, false));
        std::vector<std::pair<size_t, size_t>> edges;
        std::vector<int> deg(N, 0);

        // Aristas de triangulación por proximidad + criterio de Gabriel
        const float maxDistSq = 30.25f; // hasta 5.5 celdas de alcance entre nodos cercanos
        for (size_t i = 0; i < N; ++i) {
            for (size_t j = i + 1; j < N; ++j) {
                float dx = nodes[i].gx - nodes[j].gx;
                float dy = nodes[i].gy - nodes[j].gy;
                float dsq = dx * dx + dy * dy;
                if (dsq > maxDistSq || dsq < 0.0001f) continue;

                float mx = 0.5f * (nodes[i].gx + nodes[j].gx);
                float my = 0.5f * (nodes[i].gy + nodes[j].gy);
                int imx = std::clamp(static_cast<int>(mx), 0, w - 1);
                int imy = std::clamp(static_cast<int>(my), 0, h - 1);
                if (map.getCell(imx, imy).isWater()) continue;

                float diskR2 = dsq * 0.23f;

                bool blocked = false;
                for (size_t k = 0; k < N; ++k) {
                    if (k == i || k == j) continue;
                    float kx = nodes[k].gx - mx;
                    float ky = nodes[k].gy - my;
                    if (kx * kx + ky * ky < diskR2) {
                        blocked = true;
                        break;
                    }
                }
                if (!blocked) {
                    adj[i][j] = adj[j][i] = true;
                    edges.push_back({i, j});
                    deg[i]++;
                    deg[j]++;
                }
            }
        }

        // Asegurar que ningún nodo cercano quede sin conexión por colinealidad
        for (size_t i = 0; i < N; ++i) {
            if (deg[i] >= 2) continue;
            size_t bestJ = N;
            float bestDsq = 36.0f;
            for (size_t j = 0; j < N; ++j) {
                if (i == j || adj[i][j]) continue;
                float dx = nodes[i].gx - nodes[j].gx;
                float dy = nodes[i].gy - nodes[j].gy;
                float dsq = dx * dx + dy * dy;
                int imx = std::clamp(static_cast<int>(0.5f * (nodes[i].gx + nodes[j].gx)), 0, w - 1);
                int imy = std::clamp(static_cast<int>(0.5f * (nodes[i].gy + nodes[j].gy)), 0, h - 1);
                if (map.getCell(imx, imy).isWater()) continue;
                if (dsq < bestDsq) {
                    bestDsq = dsq;
                    bestJ = j;
                }
            }
            if (bestJ < N) {
                size_t a = std::min(i, bestJ);
                size_t b = std::max(i, bestJ);
                adj[a][b] = adj[b][a] = true;
                edges.push_back({a, b});
                deg[a]++;
                deg[b]++;
            }
        }

        // A. Rellenar el interior de cada triángulo (i, j, k) de la red de fe
        for (size_t i = 0; i < N; ++i) {
            for (size_t j = i + 1; j < N; ++j) {
                if (!adj[i][j]) continue;
                for (size_t k = j + 1; k < N; ++k) {
                    if (!adj[j][k] || !adj[i][k]) continue;

                    float ax = nodes[i].sx, ay = nodes[i].sy;
                    float bx = nodes[j].sx, by = nodes[j].sy;
                    float cx = nodes[k].sx, cy = nodes[k].sy;

                    float det = (by - cy) * (ax - cx) + (cx - bx) * (ay - cy);
                    if (std::abs(det) < 1.5f) continue;
                    float invDet = 1.0f / det;

                    int tMinX = std::max(minX, static_cast<int>(std::floor(std::min({ax, bx, cx}))));
                    int tMaxX = std::min(maxX, static_cast<int>(std::ceil(std::max({ax, bx, cx}))));
                    int tMinY = std::max(minY, static_cast<int>(std::floor(std::min({ay, by, cy}))));
                    int tMaxY = std::min(maxY, static_cast<int>(std::ceil(std::max({ay, by, cy}))));

                    for (int py = tMinY; py <= tMaxY; ++py) {
                        float fpy = static_cast<float>(py);
                        float* rowPtr = &field[(py - minY) * boxW];
                        for (int px = tMinX; px <= tMaxX; ++px) {
                            float fpx = static_cast<float>(px);
                            float w1 = ((by - cy) * (fpx - cx) + (cx - bx) * (fpy - cy)) * invDet;
                            float w2 = ((cy - ay) * (fpx - cx) + (ax - cx) * (fpy - cy)) * invDet;
                            float w3 = 1.0f - w1 - w2;
                            if (w1 >= -0.015f && w2 >= -0.015f && w3 >= -0.015f) {
                                float minBary = std::clamp(std::min({w1, w2, w3}) * 3.0f, 0.0f, 1.0f);
                                // Aura translúcida en el corazón del triángulo y más luminosa hacia las aristas
                                float triAlpha = 0.30f + 0.12f * (1.0f - minBary);
                                int idx = px - minX;
                                if (triAlpha > rowPtr[idx]) rowPtr[idx] = triAlpha;
                            }
                        }
                    }
                }
            }
        }

        // B. Rasterizar puentes de aura y líneas luminosas de triangulación en cada arista (i, j)
        const float bridgeRad = std::max(7.0f, 22.0f * zoom);
        const float bridgeRadY = bridgeRad * 0.52f;
        const float invIsoY = 1.0f / 0.52f;

        for (const auto& edge : edges) {
            const auto& nA = nodes[edge.first];
            const auto& nB = nodes[edge.second];

            float ax = nA.sx, ay = nA.sy * invIsoY;
            float bx = nB.sx, by = nB.sy * invIsoY;
            float abx = bx - ax, aby = by - ay;
            float abLenSq = abx * abx + aby * aby;
            if (abLenSq < 1.0f) continue;
            float invAbLenSq = 1.0f / abLenSq;

            int eMinX = std::max(minX, static_cast<int>(std::floor(std::min(nA.sx, nB.sx) - bridgeRad)));
            int eMaxX = std::min(maxX, static_cast<int>(std::ceil(std::max(nA.sx, nB.sx) + bridgeRad)));
            int eMinY = std::max(minY, static_cast<int>(std::floor(std::min(nA.sy, nB.sy) - bridgeRadY)));
            int eMaxY = std::min(maxY, static_cast<int>(std::ceil(std::max(nA.sy, nB.sy) + bridgeRadY)));

            for (int py = eMinY; py <= eMaxY; ++py) {
                float pyIso = static_cast<float>(py) * invIsoY;
                float* rowPtr = &field[(py - minY) * boxW];
                for (int px = eMinX; px <= eMaxX; ++px) {
                    float pxIso = static_cast<float>(px);
                    float t = std::clamp(((pxIso - ax) * abx + (pyIso - ay) * aby) * invAbLenSq, 0.0f, 1.0f);
                    float qx = ax + t * abx;
                    float qy = ay + t * aby;
                    float d2 = (pxIso - qx) * (pxIso - qx) + (pyIso - qy) * (pyIso - qy);

                    if (d2 < bridgeRad * bridgeRad) {
                        float u = std::sqrt(d2) / bridgeRad;
                        float oneMinusU2 = 1.0f - u * u;
                        float val = 0.36f * oneMinusU2 * oneMinusU2;
                        // Hilo de energía luminoso que conecta directamente los nodos de la triangulación
                        if (u < 0.14f) {
                            float lineFactor = 1.0f - (u / 0.14f);
                            val = std::max(val, 0.42f + 0.36f * lineFactor);
                        }
                        int idx = px - minX;
                        if (val > rowPtr[idx]) rowPtr[idx] = val;
                    }
                }
            }
        }

        // C. Rasterizar el halo radial isométrico de cada nodo (Aldeano, Chamán, Templo)
        for (const auto& n : nodes) {
            float rx = std::max(6.0f, n.radiusPx);
            float ry = rx * 0.52f;
            float invRx = 1.0f / rx;
            float invRy = 1.0f / ry;

            int nMinX = std::max(minX, static_cast<int>(std::floor(n.sx - rx)));
            int nMaxX = std::min(maxX, static_cast<int>(std::ceil(n.sx + rx)));
            int nMinY = std::max(minY, static_cast<int>(std::floor(n.sy - ry)));
            int nMaxY = std::min(maxY, static_cast<int>(std::ceil(n.sy + ry)));

            for (int py = nMinY; py <= nMaxY; ++py) {
                float dy = (static_cast<float>(py) - n.sy) * invRy;
                float dy2 = dy * dy;
                float* rowPtr = &field[(py - minY) * boxW];

                for (int px = nMinX; px <= nMaxX; ++px) {
                    float dx = (static_cast<float>(px) - n.sx) * invRx;
                    float u2 = dx * dx + dy2;
                    if (u2 > 1.0f) continue;

                    float u = std::sqrt(u2);
                    float oneMinusU2 = 1.0f - u2;
                    float val = 0.42f * oneMinusU2 * oneMinusU2;

                    // Borde exterior suave del aura que delimita la zona de influencia
                    if (u > 0.78f && u < 0.96f) {
                        float rim = 1.0f - std::abs(u - 0.87f) / 0.09f;
                        val = std::max(val, 0.28f * rim);
                    }

                    // Anillo de nodo en los pies (doble anillo solar para el Chamán)
                    if (n.isShaman) {
                        if ((u >= 0.16f && u <= 0.24f) || (u >= 0.36f && u <= 0.42f)) {
                            val = std::max(val, 0.82f);
                        }
                        if (n.signalTimer > 0.0f) {
                            float wavePhase = std::fmod((4.5f - n.signalTimer) * 0.85f, 1.0f);
                            if (std::abs(u - wavePhase) < 0.08f) {
                                float wBoost = 1.0f - std::abs(u - wavePhase) / 0.08f;
                                val = std::max(val, 0.90f * wBoost);
                            }
                        }
                    } else if (u >= 0.14f && u <= 0.20f) {
                        val = std::max(val, 0.62f);
                    }

                    int idx = px - minX;
                    if (val > rowPtr[idx]) rowPtr[idx] = val;
                }
            }
        }

        // D. Componer el campo de aura triangulada sobre el terreno en m_framebuffer
        for (int py = minY; py <= maxY; ++py) {
            int frameY = m_height - 1 - py;
            if (frameY < 0 || frameY >= m_height) continue;

            const float* rowPtr = &field[(py - minY) * boxW];
            uint32_t* dstRow = &m_framebuffer[frameY * m_width];

            for (int px = minX; px <= maxX; ++px) {
                float a = rowPtr[px - minX];
                if (a <= 0.01f) continue;

                uint32_t bg = dstRow[px];
                float br = static_cast<float>(bg & 0xFF);
                float bgc = static_cast<float>((bg >> 8) & 0xFF);
                float bb = static_cast<float>((bg >> 16) & 0xFF);

                // En las líneas de triangulación y anillos de nodo (a > 0.50), iluminar hacia Oro Divino
                float targetR = static_cast<float>(relColor.r);
                float targetG = static_cast<float>(relColor.g);
                float targetB = static_cast<float>(relColor.b);
                if (a > 0.50f) {
                    float glow = std::clamp((a - 0.50f) / 0.40f, 0.0f, 1.0f) * 0.55f;
                    targetR = targetR * (1.0f - glow) + 255.0f * glow;
                    targetG = targetG * (1.0f - glow) + 235.0f * glow;
                    targetB = targetB * (1.0f - glow) + 150.0f * glow;
                }

                uint8_t outR = static_cast<uint8_t>(std::clamp(br * (1.0f - a) + targetR * a, 0.0f, 255.0f));
                uint8_t outG = static_cast<uint8_t>(std::clamp(bgc * (1.0f - a) + targetG * a, 0.0f, 255.0f));
                uint8_t outB = static_cast<uint8_t>(std::clamp(bb * (1.0f - a) + targetB * a, 0.0f, 255.0f));
                dstRow[px] = (0xFFu << 24) | (outB << 16) | (outG << 8) | outR;
            }
        }
    }
}

void Renderer::renderUI(const WorldMap& map, const ReligionRegistry& registry,
                        const Timeline& timeline, const EventLine& eventLine,
                        const Point2i& selectedCell, RenderMapMode mode,
                        const GestorHumanos& humanos, const IsometricCamera& camera,
                        const WorldCreationConfig& playerConfig) {
    (void)selectedCell;
    renderTopBar(map, registry, timeline, playerConfig);
    renderBottomBar(map, registry, timeline, eventLine, humanos, camera, mode);
}

void Renderer::renderCommandConsole(bool isOpen, const std::string& inputText,
                                    bool selectorActive, const std::string& feedbackMsg,
                                    bool luminusActive) {
    // 1. Indicador activo en pantalla si el truco "SELECTOR" está habilitado
    int badgeY = 42;
    if (selectorActive) {
        int badgeX = 12;
        int badgeW = 472;
        int badgeH = 22;
        drawRect(badgeX, badgeY, badgeW, badgeH, ColorRGB{18, 48, 32});
        drawRectOutline(badgeX, badgeY, badgeW, badgeH, Palette::DIVINE_GOLD);
        drawText(badgeX + 8, badgeY + 7,
                 "[TRUCO: SELECTOR ACTIVO] CLIC IZQ: SELECCIONAR | CLIC DER: MOVER UNIDAD",
                 ColorRGB{120, 255, 220}, 1);
        badgeY += 26;
    }

    // 1b. Indicador del truco "LUMINUS" (niebla de guerra de la fe disipada)
    if (luminusActive) {
        int badgeX = 12;
        int badgeW = 340;
        int badgeH = 22;
        drawRect(badgeX, badgeY, badgeW, badgeH, ColorRGB{48, 40, 16});
        drawRectOutline(badgeX, badgeY, badgeW, badgeH, Palette::DIVINE_GOLD);
        drawText(badgeX + 8, badgeY + 7, "[TRUCO: LUMINUS ACTIVO] NIEBLA DE LA FE DISIPADA",
                 ColorRGB{255, 240, 150}, 1);
    }

    // 2. Mensaje de confirmación reciente al ejecutar un comando o truco
    if (!feedbackMsg.empty()) {
        int fbW = std::min(m_width - 40, static_cast<int>(feedbackMsg.size() * 6 + 24));
        int fbX = (m_width - fbW) / 2;
        int fbY = 44;
        drawRect(fbX, fbY, fbW, 22, ColorRGB{34, 24, 16});
        drawRectOutline(fbX, fbY, fbW, 22, Palette::DIVINE_GOLD);
        drawText(fbX + 12, fbY + 7, feedbackMsg, ColorRGB{255, 235, 130}, 1);
    }

    // 3. Caja de texto de comandos / trucos al presionar ENTER
    if (isOpen) {
        int boxW = 480;
        int boxH = 58;
        int boxX = (m_width - boxW) / 2;
        int boxY = m_height - 118 - boxH - 12;

        drawRect(boxX, boxY, boxW, boxH, ColorRGB{18, 20, 28});
        drawRectOutline(boxX, boxY, boxW, boxH, Palette::DIVINE_GOLD);
        drawRectOutline(boxX + 2, boxY + 2, boxW - 4, boxH - 4, ColorRGB{90, 70, 30});

        drawText(boxX + 12, boxY + 8,
                 "TRUCOS: 'SELECTOR' | 'LUMINUS' + ENTER (ESC: CERRAR)",
                 Palette::DIVINE_GOLD, 1);

        int fieldX = boxX + 12;
        int fieldY = boxY + 24;
        int fieldW = boxW - 24;
        int fieldH = 24;
        drawRect(fieldX, fieldY, fieldW, fieldH, ColorRGB{8, 10, 14});
        drawRectOutline(fieldX, fieldY, fieldW, fieldH, ColorRGB{0, 255, 255});

        std::string displayLine = "> " + inputText + "_";
        drawText(fieldX + 8, fieldY + 8, displayLine.substr(0, 72), ColorRGB{255, 250, 200}, 1);
    }
}

void Renderer::renderTopBar(const WorldMap& map, const ReligionRegistry& registry,
                            const Timeline& timeline, const WorldCreationConfig& playerConfig) {
    const int topH = 34;

    // Región 1: Panel de Información de Pueblo (Izquierda - 44% ancho)
    // Información de creyentes del jugador, Dios y recursos sagrados (Fondo #120b0a)
    const int grisW = static_cast<int>(m_width * 0.44f);
    drawRect(0, 0, grisW, topH, ColorRGB{18, 11, 10}); // #120b0a
    drawRectOutline(0, 0, grisW, topH, ColorRGB{75, 52, 38});
    drawLine(0, topH, grisW, topH, Palette::DIVINE_GOLD);

    // Datos del jugador y creyentes
    const Religion* pRel = registry.getReligion(1);
    int playerFollowers = pRel ? pRel->totalFollowers : 0;
    int pietyPoints = pRel ? pRel->theologyPoints : 1000;
    std::string godDisplay = "DIOS: " + playerConfig.godName + " | CULTO: " + playerConfig.religionName;
    drawText(10, 6, godDisplay.substr(0, 38), Palette::DIVINE_GOLD, 1);

    std::string resDisplay = "CREYENTES: " + std::to_string(playerFollowers) +
                             " (" + std::to_string(map.getTotalWorldPopulation()) + " tot.) | FE/PIEDAD: " +
                             std::to_string(pietyPoints) + " pts";
    drawText(10, 18, resDisplay.substr(0, 48), Palette::RELIC_BONE, 1);

    // Región 2: BLANCO (Centro - 12% ancho)
    // Era actual del pueblo de Dios con banner sagrado
    const int blancaX = grisW;
    const int blancaW = static_cast<int>(m_width * 0.12f);
    drawRect(blancaX, 0, blancaW, topH, ColorRGB{245, 245, 240}); // Blanco lino sagrado
    drawRectOutline(blancaX, 0, blancaW, topH, Palette::DIVINE_GOLD);
    drawLine(blancaX, topH, blancaX + blancaW, topH, Palette::DIVINE_GOLD);

    EraType curEra = AdvancementSystem::getEraForYear(timeline.getCurrentYear());
    if (pRel) curEra = pRel->currentEra;
    std::string eraNameShort = AdvancementSystem::getEraName(curEra);
    // Extraer nombre corto para encajar en el banner
    if (eraNameShort.size() > 16) eraNameShort = eraNameShort.substr(0, 16);

    int eraTxtX = blancaX + (blancaW - static_cast<int>(eraNameShort.size() * 6)) / 2;
    drawText(eraTxtX, 5, eraNameShort, ColorRGB{20, 20, 24}, 1);

    std::string yearStr = Timeline::formatYear(timeline.getCurrentYear());
    int yearTxtX = blancaX + (blancaW - static_cast<int>(yearStr.size() * 6)) / 2;
    drawText(yearTxtX, 18, yearStr, Palette::BLOOD_RUBY, 1);

    // Región 3: Panel de Opciones (Derecha - 44% ancho)
    // Opciones de partida, velocidad de tiempo y comandos del sistema (Fondo #120b0a)
    const int rojaX = blancaX + blancaW;
    const int rojaW = m_width - rojaX;
    drawRect(rojaX, 0, rojaW, topH, ColorRGB{18, 11, 10}); // #120b0a
    drawRectOutline(rojaX, 0, rojaW, topH, ColorRGB{75, 52, 38});
    drawLine(rojaX, topH, m_width, topH, Palette::DIVINE_GOLD);

    // Panel de Opciones: botón MENU que despliega las opciones de partida
    (void)timeline;
    for (const auto& btn : m_buttons) {
        if (btn.y >= topH) continue; // solo botones del top bar

        bool isActive = (btn.id == 400 && m_dropdownOpen);

        int bx = static_cast<int>(btn.x);
        int by = static_cast<int>(btn.y);
        int bw = static_cast<int>(btn.width);
        int bh = static_cast<int>(btn.height);

        std::string label = btn.label;
        if (btn.id == 400) label += m_dropdownOpen ? " ^" : " v";

        if (isActive) {
            drawRect(bx, by, bw, bh, ColorRGB{52, 32, 24});
            drawRectOutline(bx, by, bw, bh, Palette::DIVINE_GOLD);
            drawText(bx + (bw - static_cast<int>(label.size() * 6)) / 2, by + 8, label, ColorRGB{255, 235, 120}, 1);
        } else {
            drawRect(bx, by, bw, bh, ColorRGB{32, 20, 18});
            drawRectOutline(bx, by, bw, bh, ColorRGB{95, 68, 48});
            drawText(bx + (bw - static_cast<int>(label.size() * 6)) / 2, by + 8, label, Palette::RELIC_BONE, 1);
        }
    }
}

void Renderer::renderOptionsDropdown() {
    if (!m_dropdownOpen || m_dropdownButtons.empty()) return;

    // Panel contenedor que envuelve a todos los ítems del desplegable
    const UIButton& first = m_dropdownButtons.front();
    const UIButton& last = m_dropdownButtons.back();
    int px = static_cast<int>(first.x) - 6;
    int py = static_cast<int>(first.y) - 6;
    int pw = static_cast<int>(first.width) + 12;
    int ph = static_cast<int>(last.y + last.height) - py + 6;

    drawRect(px, py, pw, ph, ColorRGB{18, 11, 10}); // #120b0a
    drawRectOutline(px, py, pw, ph, ColorRGB{75, 52, 38});
    drawRectOutline(px + 2, py + 2, pw - 4, ph - 4, Palette::DIVINE_GOLD);

    for (const auto& btn : m_dropdownButtons) {
        int bx = static_cast<int>(btn.x);
        int by = static_cast<int>(btn.y);
        int bw = static_cast<int>(btn.width);
        int bh = static_cast<int>(btn.height);
        bool danger = (btn.id == 402);
        drawRect(bx, by, bw, bh, danger ? ColorRGB{58, 18, 16} : ColorRGB{32, 20, 18});
        drawRectOutline(bx, by, bw, bh, danger ? ColorRGB{150, 60, 50} : ColorRGB{95, 68, 48});
        drawText(bx + 8, by + (bh - 7) / 2, btn.label, danger ? ColorRGB{255, 190, 170} : Palette::RELIC_BONE, 1);
    }
}

void Renderer::renderBottomBar(const WorldMap& map, const ReligionRegistry& registry,
                               const Timeline& timeline, const EventLine& eventLine,
                               const GestorHumanos& humanos, const IsometricCamera& camera,
                               RenderMapMode mode) {
    const int botH = 118;
    const int botY = m_height - botH;

    // 1. ZONA IZQUIERDA (18.5% ancho) - Panel de Opciones de Unidad / Información de Pueblo-Unidad
    const int verdeW = static_cast<int>(m_width * 0.185f);
    drawRect(0, botY, verdeW, botH, ColorRGB{18, 11, 10}); // #120b0a
    drawRectOutline(0, botY, verdeW, botH, ColorRGB{75, 52, 38});
    drawLine(0, botY, verdeW, botY, Palette::DIVINE_GOLD);

    // Obtener humano seleccionado o, por defecto, el Chamán del pueblo del jugador
    const Humano* selHumano = humanos.getHumanoSeleccionado();
    const int playerRel = playerReligionId();
    if (!selHumano) {
        int chId = humanos.buscarChamanDeReligion(playerRel);
        for (const auto& hObj : humanos.getHumanos()) {
            if ((chId > 0 && hObj.getId() == chId) || (chId <= 0 && hObj.getReligionId() == playerRel)) {
                selHumano = &hObj;
                break;
            }
        }
    }

    // Las opciones divinas sólo están disponibles con el Chamán de tu propio pueblo
    if (selHumano && selHumano->esChaman() && selHumano->getReligionId() == playerRel) {
        drawText(8, botY + 6, "LIDER: " + selHumano->getNombre().substr(0, 15), Palette::DIVINE_GOLD, 1);

        const Religion* hRel = registry.getReligion(selHumano->getReligionId());
        std::string feName = hRel ? hRel->name : "Fiel Animista";
        drawText(8, botY + 18, ("Fe: " + feName).substr(0, 22), hRel ? hRel->color : Palette::RELIC_BONE, 1);

        // Barra compacta de Vida (HP) y devoción del Chamán
        int halfW = (verdeW - 56) / 2;
        drawText(8, botY + 30, "HP:", Palette::RELIC_BONE, 1);
        drawRect(30, botY + 30, halfW, 7, ColorRGB{12, 7, 6});
        drawRectOutline(30, botY + 30, halfW, 7, ColorRGB{95, 68, 48});
        int hpW = static_cast<int>(halfW * (selHumano->getSalud() / 100.0f));
        ColorRGB hpCol = (selHumano->getSalud() > 50.0f) ? ColorRGB{46, 215, 80} : ColorRGB{220, 50, 50};
        drawRect(30, botY + 30, std::clamp(hpW, 0, halfW), 7, hpCol);

        int devX = 30 + halfW + 10;
        drawText(devX - 22, botY + 30, "Fe:", Palette::RELIC_BONE, 1);
        drawRect(devX, botY + 30, halfW, 7, ColorRGB{12, 7, 6});
        drawRectOutline(devX, botY + 30, halfW, 7, ColorRGB{95, 68, 48});
        int devW = static_cast<int>(halfW * selHumano->getDevocion());
        drawRect(devX, botY + 30, std::clamp(devW, 0, halfW), 7, Palette::DIVINE_GOLD);

        // Sección de Opciones de la Unidad: Comunicación y Señales Divinas
        bool isChanneling = (selHumano->getTimerSenalDivina() > 0.0f);
        drawText(8, botY + 44, isChanneling ? "SENAL DIVINA ACTIVA!" : "OPCIONES: SENALES DIVINAS",
                 isChanneling ? ColorRGB{255, 240, 130} : Palette::RELIC_BONE, 1);

        for (const auto& btn : m_buttons) {
            if (btn.id < 701 || btn.id > 703) continue;
            int bx = static_cast<int>(btn.x);
            int by = static_cast<int>(btn.y);
            int bw = static_cast<int>(btn.width);
            int bh = static_cast<int>(btn.height);

            ColorRGB fillCol = isChanneling ? ColorRGB{52, 32, 24} : ColorRGB{32, 20, 18};
            ColorRGB borderCol = (btn.id == 703 || isChanneling) ? Palette::DIVINE_GOLD : ColorRGB{95, 68, 48};
            drawRect(bx, by, bw, bh, fillCol);
            drawRectOutline(bx, by, bw, bh, borderCol);
            int txtX = bx + std::max(2, (bw - static_cast<int>(btn.label.size() * 6)) / 2);
            drawText(txtX, by + 8, btn.label, ColorRGB{255, 240, 180}, 1);
        }
    } else if (selHumano) {
        if (selHumano->estaMuerto()) {
            drawText(8, botY + 6, ("CAIDO: " + selHumano->getNombre()).substr(0, 23), ColorRGB{220, 50, 50}, 1);
            drawText(8, botY + 18, "Ha muerto en combate", Palette::BLOOD_RUBY, 1);
            drawText(8, botY + 32, "HP: 0/100 (Fallecido)", ColorRGB{200, 70, 70}, 1);
            drawText(8, botY + 46, "Retornando a la tierra...", Palette::RELIC_BONE, 1);

            int bx = 8;
            int by = botY + 86;
            int bw = verdeW - 16;
            int bh = 24;
            drawRect(bx, by, bw, bh, ColorRGB{32, 20, 18});
            drawRectOutline(bx, by, bw, bh, Palette::DIVINE_GOLD);
            std::string chLabel = "SELECCIONAR CHAMAN";
            drawText(bx + std::max(2, (bw - static_cast<int>(chLabel.size() * 6)) / 2), by + 8, chLabel, ColorRGB{255, 235, 140}, 1);
        } else {
            const HumanUnit& hu = selHumano->getHumanUnit();
            std::string roleTag = "ALDEANO";
            switch (hu.role) {
                case CommunityRole::FORAGER: roleTag = "RECOLECTOR"; break;
                case CommunityRole::SHAMAN:  roleTag = "CHAMAN"; break;
                case CommunityRole::PROPHET: roleTag = "PROFETA"; break;
                case CommunityRole::ELDER:   roleTag = "ANCIANO"; break;
                case CommunityRole::ZEALOT:  roleTag = "FANATICO"; break;
            }
            bool esRival = (selHumano->getReligionId() != playerRel);
            if (esRival) roleTag = "AJENO " + roleTag; // unidad de otra fe (sólo accesible con SELECTOR)
            drawText(8, botY + 6, (roleTag + ": " + selHumano->getNombre()).substr(0, 23),
                     esRival ? ColorRGB{230, 90, 80} : Palette::DIVINE_GOLD, 1);

            const Religion* hRel = registry.getReligion(selHumano->getReligionId());
            std::string feName = hRel ? hRel->name : "Fiel Animista";
            drawText(8, botY + 18, ("Fe: " + feName).substr(0, 22), hRel ? hRel->color : Palette::RELIC_BONE, 1);

            std::string axesStr = "En:" + std::to_string(static_cast<int>(hu.energy)) +
                                  " Nut:" + std::to_string(static_cast<int>(hu.nutritionalState)) +
                                  " Iner:" + std::to_string(static_cast<int>(hu.conversionInertia));
            drawText(8, botY + 30, axesStr.substr(0, 24), Palette::RELIC_BONE, 1);

            // Barra de Vida (HP) y Devoción
            int halfW = (verdeW - 56) / 2;
            drawText(8, botY + 42, "HP:", Palette::RELIC_BONE, 1);
            drawRect(30, botY + 42, halfW, 7, ColorRGB{12, 7, 6});
            drawRectOutline(30, botY + 42, halfW, 7, ColorRGB{95, 68, 48});
            int hpW = static_cast<int>(halfW * (selHumano->getSalud() / 100.0f));
            ColorRGB hpCol = (selHumano->getSalud() > 50.0f) ? ColorRGB{46, 215, 80} : ColorRGB{220, 50, 50};
            drawRect(30, botY + 42, std::clamp(hpW, 0, halfW), 7, hpCol);

            int devX = 30 + halfW + 10;
            drawText(devX - 22, botY + 42, "Fe:", Palette::RELIC_BONE, 1);
            drawRect(devX, botY + 42, halfW, 7, ColorRGB{12, 7, 6});
            drawRectOutline(devX, botY + 42, halfW, 7, ColorRGB{95, 68, 48});
            int devW = static_cast<int>(halfW * selHumano->getDevocion());
            drawRect(devX, botY + 42, std::clamp(devW, 0, halfW), 7, Palette::DIVINE_GOLD);

            // Estado sistémico de la unidad (HumanAIController)
            std::string stateStr = "IA: ";
            switch (hu.currentState) {
                case HumanAIState::SURVIVAL_WORKING: stateStr += "Superviv./Energia"; break;
                case HumanAIState::WORSHIPING:       stateStr += "Culto / Devocion"; break;
                case HumanAIState::SOCIALIZING:      stateStr += "Cohesion / Tribu"; break;
                case HumanAIState::CONVERTING:       stateStr += "Convirtiendo Fe"; break;
                case HumanAIState::MIGRATING:        stateStr += "Migrando"; break;
                default:                             stateStr += "En Reposo"; break;
            }
            drawText(8, botY + 56, stateStr.substr(0, 23), Palette::RELIC_BONE, 1);

            // Estado de animación física
            std::string actStr = "Accion: ";
            switch (selHumano->getAccion()) {
                case AccionHumano::IDLE: actStr += "Orando / Quieto"; break;
                case AccionHumano::CAMINAR: actStr += "Peregrinando"; break;
                case AccionHumano::CORRER: actStr += "Migrando"; break;
                case AccionHumano::RECOLECTAR: actStr += "Recolectando"; break;
                case AccionHumano::TALAR: actStr += "Cazando / Talando"; break;
                case AccionHumano::ATACAR: actStr += "Guardia Frontera"; break;
                case AccionHumano::REZAR: actStr += "Rito Chamanico"; break;
                default: actStr += "Comercio / Carga"; break;
            }
            drawText(8, botY + 70, actStr.substr(0, 22), ColorRGB{200, 185, 165}, 1);

            // Botón en Opciones de Unidad para seleccionar al Chamán del pueblo
            int bx = 8;
            int by = botY + 86;
            int bw = verdeW - 16;
            int bh = 24;
            drawRect(bx, by, bw, bh, ColorRGB{32, 20, 18});
            drawRectOutline(bx, by, bw, bh, Palette::DIVINE_GOLD);
            std::string chLabel = "IR AL CHAMAN DEL PUEBLO";
            drawText(bx + std::max(2, (bw - static_cast<int>(chLabel.size() * 6)) / 2), by + 8, chLabel, ColorRGB{255, 235, 140}, 1);
        }
    } else {
        drawText(8, botY + 8, "UNIDAD / CHAMAN", Palette::DIVINE_GOLD, 1);
        drawText(8, botY + 28, "Selecciona al Chaman", Palette::RELIC_BONE, 1);
        drawText(8, botY + 42, "de un pueblo para", Palette::RELIC_BONE, 1);
        drawText(8, botY + 56, "enviar senales divinas", Palette::RELIC_BONE, 1);

        int bx = 8;
        int by = botY + 86;
        int bw = verdeW - 16;
        int bh = 24;
        drawRect(bx, by, bw, bh, ColorRGB{32, 20, 18});
        drawRectOutline(bx, by, bw, bh, Palette::DIVINE_GOLD);
        std::string chLabel = "SELECCIONAR CHAMAN";
        drawText(bx + std::max(2, (bw - static_cast<int>(chLabel.size() * 6)) / 2), by + 8, chLabel, ColorRGB{255, 235, 140}, 1);
    }

    // 2. PANEL DE SIMULACION (Centro - 63% ancho)
    // Información de la simulación, crónica de eventos, línea de tiempo y controles (Fondo #120b0a)
    const int azulX = verdeW;
    const int azulW = static_cast<int>(m_width * 0.63f);
    drawRect(azulX, botY, azulW, botH, ColorRGB{18, 11, 10}); // #120b0a
    drawRectOutline(azulX, botY, azulW, botH, ColorRGB{75, 52, 38});
    drawLine(azulX, botY, azulX + azulW, botY, Palette::DIVINE_GOLD);

    // Timeline Scrub Track
    int trackX = azulX + 16;
    int trackY = botY + 14;
    int trackW = azulW - 32;
    int trackH = 8;
    drawRect(trackX, trackY, trackW, trackH, ColorRGB{12, 7, 6});
    drawRectOutline(trackX, trackY, trackW, trackH, ColorRGB{95, 68, 48});

    float prog = timeline.getTimelineProgress();
    int fillW = static_cast<int>(trackW * prog);
    if (fillW > 0) {
        drawRect(trackX, trackY, fillW, trackH, Palette::DIVINE_GOLD);
    }
    int thumbX = trackX + fillW;
    drawRect(thumbX - 4, trackY - 4, 8, 16, Palette::BLOOD_RUBY);
    drawRectOutline(thumbX - 4, trackY - 4, 8, 16, Palette::DIVINE_GOLD);

    // Eventos recientes en tarjetas ceremoniales dentro del panel de simulación
    auto recents = eventLine.getRecentEvents(3);
    int cardY = botY + 28;
    int cardW = (azulW - 48) / 3;
    int cardH = 46;

    for (size_t i = 0; i < recents.size(); ++i) {
        int cx = trackX + static_cast<int>(i) * (cardW + 8);
        drawRect(cx, cardY, cardW, cardH, ColorRGB{28, 18, 16});
        drawRectOutline(cx, cardY, cardW, cardH, recents[i].badgeColor);

        std::string evTitle = "[" + Timeline::formatYear(recents[i].year) + "] " + recents[i].title;
        drawText(cx + 6, cardY + 6, evTitle.substr(0, 24), Palette::DIVINE_GOLD, 1);
        drawText(cx + 6, cardY + 20, recents[i].description.substr(0, 26), Palette::RELIC_BONE, 1);
        drawText(cx + 6, cardY + 32, recents[i].description.substr(26, 26), ColorRGB{200, 185, 165}, 1);
    }

    // Botones inferiores: controles de simulación (centro, bajo la línea de tiempo), intervenciones
    // divinas, y botones de vista del mapa (panel del minimapa, a la derecha)
    auto isButtonActive = [&](int id) {
        switch (id) {
            case 101: return mode == RenderMapMode::ReligionDominance;
            case 102: return mode == RenderMapMode::TerrainBiomes;
            case 103: return mode == RenderMapMode::SyncretismHeatmap;
            case 104: return mode == RenderMapMode::AdvancementEra;
            case 105: return mode == RenderMapMode::Units;
            case 201: return timeline.isPaused();
            case 202: return !timeline.isPaused() && timeline.getSpeedMultiplier() == 1;
            case 203: return !timeline.isPaused() && timeline.getSpeedMultiplier() == 2;
            case 204: return !timeline.isPaused() && timeline.getSpeedMultiplier() >= 5;
            default:  return false;
        }
    };
    auto drawPanelButton = [&](const UIButton& btn) {
        bool active = isButtonActive(btn.id);
        int bx = static_cast<int>(btn.x);
        int by = static_cast<int>(btn.y);
        int bw = static_cast<int>(btn.width);
        int bh = static_cast<int>(btn.height);
        int ty = by + (bh - 7) / 2;
        int tx = bx + (bw - static_cast<int>(btn.label.size() * 6)) / 2;
        if (active) {
            drawRect(bx, by, bw, bh, ColorRGB{52, 32, 24});
            drawRectOutline(bx, by, bw, bh, Palette::DIVINE_GOLD);
            drawText(tx, ty, btn.label, ColorRGB{255, 235, 120}, 1);
        } else {
            drawRect(bx, by, bw, bh, ColorRGB{32, 20, 18});
            drawRectOutline(bx, by, bw, bh, ColorRGB{95, 68, 48});
            drawText(tx, ty, btn.label, Palette::RELIC_BONE, 1);
        }
    };

    for (const auto& btn : m_buttons) {
        if (btn.y < botY || btn.x < azulX || btn.x >= azulX + azulW) continue; // zona central
        drawPanelButton(btn);
    }

    // 3. ZONA AMARILLA (Derecha - 18.5% ancho)
    // Minimapa interactivo que mueve la cámara al hacer clic, con opciones de vista
    const int amarilloX = azulX + azulW;
    const int amarilloW = m_width - amarilloX;
    drawRect(amarilloX, botY, amarilloW, botH, ColorRGB{60, 48, 12}); // Ámbar dorado litúrgico
    drawRectOutline(amarilloX, botY, amarilloW, botH, ColorRGB{140, 110, 30});
    drawLine(amarilloX, botY, m_width, botY, Palette::DIVINE_GOLD);

    drawText(amarilloX + 10, botY + 6, "MINIMAPA SAGRADO", Palette::DIVINE_GOLD, 1);

    // Marco del Minimapa (a la izquierda del panel)
    int mmSize = 92, mmX = 0, mmY = 0;
    getMinimapRect(mmX, mmY, mmSize);

    renderMinimap(mmX, mmY, mmSize, mmSize, map, registry, humanos, camera, mode);

    // Botones de vista del mapa (Fe, Biomas, Unidades, Sincretismo, Eras) junto al minimapa
    for (const auto& btn : m_buttons) {
        if (btn.y < botY || btn.x < amarilloX) continue;
        drawPanelButton(btn);
    }
}

void Renderer::renderMinimap(int mmX, int mmY, int mmW, int mmH,
                             const WorldMap& map, const ReligionRegistry& registry,
                             const GestorHumanos& humanos,
                             const IsometricCamera& camera, RenderMapMode mode) {
    drawRect(mmX, mmY, mmW, mmH, ColorRGB{14, 14, 18});
    drawRectOutline(mmX, mmY, mmW, mmH, Palette::DIVINE_GOLD);

    int mapW = map.getWidth();
    int mapH = map.getHeight();

    for (int y = 0; y < mmH; ++y) {
        int gy = (y * mapH) / mmH;
        int frameY = m_height - 1 - (mmY + y);
        if (frameY < 0 || frameY >= m_height) continue;

        for (int x = 0; x < mmW; ++x) {
            int gx = (x * mapW) / mmW;
            int px = mmX + x;
            if (px < 0 || px >= m_width) continue;

            const Cell& c = map.getCell(gx, gy);
            ColorRGB col = c.getBiomeBaseColor();
            bool cellVisible = !fogActive() || m_fog->isVisible(gx, gy);

            if (mode == RenderMapMode::ReligionDominance && c.population > 0 && c.dominantReligionId > 0 &&
                (cellVisible || c.dominantReligionId == playerReligionId())) {
                const Religion* r = registry.getReligion(c.dominantReligionId);
                if (r) col = r->color;
            } else if (mode == RenderMapMode::SyncretismHeatmap && !c.isWater() && c.population > 0 && cellVisible) {
                col = ColorRGB::blend(col, ColorRGB{255, 60, 220}, c.syncreticTension);
            } else if (mode == RenderMapMode::AdvancementEra && !c.isWater() && cellVisible) {
                // Vista ERAS: el desarrollo de cada celda tiñe el terreno de dorado
                col = ColorRGB::blend(col, ColorRGB{240, 200, 70}, std::clamp(c.development * 0.8f, 0.0f, 1.0f));
            } else if (mode == RenderMapMode::Units && !c.isWater()) {
                // Vista UNIDADES: terreno atenuado para que resalten los puntos de cada unidad
                col = ColorRGB::blend(col, ColorRGB{18, 11, 10}, 0.45f);
            }

            // Niebla de Guerra de la Fe en el minimapa: bruma #120b0a y memoria atenuada
            if (fogActive()) {
                float vis = m_fog->visibility(gx, gy);
                float expl = m_fog->explored(gx, gy);
                ColorRGB memCol{static_cast<uint8_t>(col.r * 0.42f), static_cast<uint8_t>(col.g * 0.40f),
                                static_cast<uint8_t>(col.b * 0.44f)};
                ColorRGB fogCol = ColorRGB::blend(ColorRGB{18, 11, 10}, memCol, expl);
                col = ColorRGB::blend(fogCol, col, vis);
            }

            m_framebuffer[frameY * m_width + px] = col.toRGBA();
        }
    }

    // Vista UNIDADES: un punto por unidad con el color de su fe (los rivales ocultos por la niebla no aparecen)
    if (mode == RenderMapMode::Units) {
        for (const auto& hObj : humanos.getHumanos()) {
            if (isHiddenByFog(hObj.getCeldaX(), hObj.getCeldaY(), hObj.getReligionId())) continue;
            ColorRGB dot = Palette::RELIC_BONE;
            if (const Religion* r = registry.getReligion(hObj.getReligionId())) dot = r->color;
            int ux = mmX + static_cast<int>((hObj.getX() / static_cast<float>(mapW)) * mmW);
            int uy = mmY + static_cast<int>((hObj.getY() / static_cast<float>(mapH)) * mmH);
            if (hObj.esChaman()) {
                drawRect(ux - 2, uy - 2, 5, 5, Palette::DIVINE_GOLD);
                drawRect(ux - 1, uy - 1, 3, 3, dot);
            } else {
                drawRect(ux - 1, uy - 1, 2, 2, dot);
            }
        }
    }

    // Visor de Cámara en el Minimapa (Rectángulo blanco de visión actual)
    int camGx = 0, camGy = 0;
    camera.screenToGrid(m_width * 0.5f, m_height * 0.5f, camGx, camGy);
    float camTargetX = static_cast<float>(std::clamp(camGx, 0, mapW));
    float camTargetY = static_cast<float>(std::clamp(camGy, 0, mapH));
    int camMmX = mmX + static_cast<int>((camTargetX / mapW) * mmW);
    int camMmY = mmY + static_cast<int>((camTargetY / mapH) * mmH);
    int boxR = std::clamp(static_cast<int>(14.0f / camera.getZoom()), 4, 20);

    drawRectOutline(camMmX - boxR, camMmY - boxR, boxR * 2, boxR * 2, ColorRGB{255, 255, 255});
    drawRect(camMmX - 1, camMmY - 1, 3, 3, Palette::BLOOD_RUBY);
}

void Renderer::renderInspector(const WorldMap& map, const ReligionRegistry& registry,
                               const Point2i& selectedCell) {
    if (!map.isValid(selectedCell.x, selectedCell.y)) return;

    int panelX = m_width - 280;
    int panelY = 60;
    int panelW = 270;
    int panelH = 430;

    // Relicario / Panel de Información de Pueblo: #120b0a con doble marco
    drawRect(panelX, panelY, panelW, panelH, ColorRGB(18, 11, 10, 245)); // #120b0a
    drawRectOutline(panelX, panelY, panelW, panelH, ColorRGB(75, 52, 38));
    drawRectOutline(panelX + 2, panelY + 2, panelW - 4, panelH - 4, ColorRGB(212, 175, 55)); // #D4AF37 Oro Divino
    
    // Esquinas ornamentales en el relicario
    drawRect(panelX, panelY, 5, 5, ColorRGB(230, 161, 59));
    drawRect(panelX + panelW - 5, panelY, 5, 5, ColorRGB(230, 161, 59));
    drawRect(panelX, panelY + panelH - 5, 5, 5, ColorRGB(230, 161, 59));
    drawRect(panelX + panelW - 5, panelY + panelH - 5, 5, 5, ColorRGB(230, 161, 59));

    const Cell& c = map.getCell(selectedCell.x, selectedCell.y);

    drawText(panelX + 15, panelY + 15, "PROVINCE INSPECTOR", ColorRGB(212, 175, 55), 1); // Oro Divino
    std::string coords = "Grid: (" + std::to_string(selectedCell.x) + ", " + std::to_string(selectedCell.y) + ")";
    drawText(panelX + 15, panelY + 32, coords, ColorRGB(204, 181, 189), 1); // Lino Litúrgico #CCB5BD

    // Biome & Population
    static const char* BIOME_NAMES[] = {
        "Deep Ocean", "Shallow Water", "Coastal Plains", "Fertile Valley",
        "Dense Forest", "Highland Hills", "Arid Desert", "Rugged Mountains", "Frozen Tundra"
    };
    std::string bName = BIOME_NAMES[static_cast<int>(c.biome)];
    drawText(panelX + 15, panelY + 52, "Biome: " + bName, ColorRGB(226, 218, 196), 1); // Hueso Reliquia

    std::string popStr = "Population: " + std::to_string(c.population) + " / " + std::to_string(c.maxPopulation);
    drawText(panelX + 15, panelY + 68, popStr, ColorRGB(226, 218, 196), 1);

    // Holy Site (Resaltado con Oro Divino o Lino)
    if (c.hasHolySite) {
        drawText(panelX + 15, panelY + 88, "[!] " + c.holySiteName, ColorRGB(230, 161, 59), 1); // Ocre Dorado #E6A13B
        std::string lvlStr = "Sanctuary Tier: " + std::to_string(c.shrineLevel);
        drawText(panelX + 15, panelY + 104, lvlStr, ColorRGB(212, 175, 55), 1);
    } else {
        drawText(panelX + 15, panelY + 88, "Holy Site: None", ColorRGB(120, 115, 110), 1);
    }

    // Faiths breakdown title
    drawText(panelX + 15, panelY + 130, "RELIGIOUS BREAKDOWN:", ColorRGB(212, 175, 55), 1);

    int curY = panelY + 150;
    if (c.faithShare.empty()) {
        drawText(panelX + 15, curY, "Animist Folk (Unaffiliated)", ColorRGB(180, 170, 160), 1);
    } else {
        for (const auto& p : c.faithShare) {
            const Religion* r = registry.getReligion(p.first);
            if (!r) continue;

            int pct = static_cast<int>(p.second * 100.0f);
            std::string line = r->name.substr(0, 16) + " (" + std::to_string(pct) + "%)";
            drawText(panelX + 15, curY, line, r->color, 1);

            // Barra de ranura / relicario estilo sección 3 y 4 de la guía
            drawRect(panelX + 15, curY + 14, 240, 8, ColorRGB(34, 34, 34)); // Fondo ranura
            drawRectOutline(panelX + 15, curY + 14, 240, 8, ColorRGB(77, 77, 77)); // Borde mármol
            int fillW = std::clamp(static_cast<int>(240.0f * p.second), 0, 240);
            drawRect(panelX + 15, curY + 14, fillW, 8, r->color);

            curY += 28;
            if (curY > panelY + 280) break;
        }
    }

    // Dominant Religion Details Card (Marco con borde Oro Divino #D4AF37)
    if (c.dominantReligionId > 0) {
        const Religion* dom = registry.getReligion(c.dominantReligionId);
        if (dom) {
            drawRect(panelX + 10, curY + 10, panelW - 20, 120, ColorRGB(32, 28, 25)); // Placa sagrada
            drawRectOutline(panelX + 10, curY + 10, panelW - 20, 120, ColorRGB(212, 175, 55)); // Borde Oro
            drawText(panelX + 15, curY + 18, "DOMINANT: " + dom->name.substr(0, 18), dom->color, 1);
            drawText(panelX + 15, curY + 34, "Prophet: " + dom->founderName, ColorRGB(226, 218, 196), 1); // Hueso Reliquia
            drawText(panelX + 15, curY + 50, "Zeal: " + std::to_string(static_cast<int>(dom->zeal * 100)) +
                     "%  Syncretism: " + std::to_string(static_cast<int>(dom->syncretismTolerance * 100)) + "%",
                     ColorRGB(204, 181, 189), 1); // Lino Litúrgico
            drawText(panelX + 15, curY + 66, "Adherents: " + std::to_string(dom->totalFollowers), ColorRGB(230, 161, 59), 1); // Ocre Dorado
            drawText(panelX + 15, curY + 82, "Doctrines: " + std::to_string(dom->doctrines.size()), ColorRGB(212, 175, 55), 1);
        }
    }
}

void Renderer::renderTimelineAndEvents(const Timeline& timeline, const EventLine& eventLine) {
    int barY = m_height - 110;
    int barH = 105;

    // Cinta inferior de la Línea de Tiempo: Basalto Monástico #1A1A1A, borde Mármol #4D4D4D y acento Oro #D4AF37
    drawRect(0, barY, m_width, barH, ColorRGB(26, 26, 26, 250)); // #1A1A1A
    drawRectOutline(0, barY, m_width, barH, ColorRGB(77, 77, 77)); // #4D4D4D
    drawLine(0, barY, m_width, barY, ColorRGB(212, 175, 55)); // Filamento Oro Divino

    // Timeline Track (Inspirado en la Barra de Energía Espiritual del HUD de la guía)
    int trackX = 40;
    int trackY = barY + 25;
    int trackW = m_width - 80;
    int trackH = 10;

    // Relieve de la ranura de progreso
    drawRect(trackX, trackY, trackW, trackH, ColorRGB(32, 28, 24)); // Ranura oscura
    drawRectOutline(trackX, trackY, trackW, trackH, ColorRGB(77, 77, 77)); // Borde mármol
    drawRectOutline(trackX - 1, trackY - 1, trackW + 2, trackH + 2, ColorRGB(40, 40, 40));

    // Progress bar fill (Oro Divino #D4AF37 y Ocre #E6A13B)
    float prog = timeline.getTimelineProgress();
    int fillW = static_cast<int>(trackW * prog);
    if (fillW > 0) {
        drawRect(trackX, trackY, fillW, trackH, ColorRGB(212, 175, 55)); // #D4AF37
        drawLine(trackX, trackY + 1, trackX + fillW - 1, trackY + 1, ColorRGB(245, 230, 160)); // Brillo sagrado
    }

    // Scrubber thumb / Marcador de Cruz o Gema Sagrada
    int thumbX = trackX + fillW;
    drawRect(thumbX - 6, trackY - 6, 12, 22, ColorRGB(191, 30, 45)); // Rubí Sangre Sacrificial #BF1E2D
    drawRectOutline(thumbX - 6, trackY - 6, 12, 22, ColorRGB(212, 175, 55)); // Borde Oro Divino
    drawRect(thumbX - 2, trackY - 2, 4, 14, ColorRGB(255, 230, 120)); // Centro dorado brillante

    // Century tick marks & labels (Hueso Reliquia #E2DAC4)
    drawText(trackX, trackY - 15, "-4000 BCE", ColorRGB(226, 218, 196), 1);
    drawText(trackX + trackW / 2 - 20, trackY - 15, "0 CE", ColorRGB(212, 175, 55), 1); // Centro sagrado en Oro
    drawText(trackX + trackW - 60, trackY - 15, "+2200 CE", ColorRGB(226, 218, 196), 1);

    // Event Line Cards (Tarjetas de Hitos de Avance estilo relicarios / ranuras de inventario)
    auto recentEvents = eventLine.getRecentEvents(4);
    int cardW = 270;
    int cardH = 52;
    int cardY = barY + 44;

    for (size_t i = 0; i < recentEvents.size(); ++i) {
        const auto& ev = recentEvents[i];
        int cardX = trackX + static_cast<int>(i) * (cardW + 15);
        if (cardX + cardW > m_width - 20) break;

        // Ranura con fondo piedra tallada y borde relicario
        drawRect(cardX, cardY, cardW, cardH, ColorRGB(34, 34, 34)); // Ranura interna
        drawRectOutline(cardX, cardY, cardW, cardH, ColorRGB(77, 77, 77)); // Mármol Sacrificado
        drawRectOutline(cardX + 2, cardY + 2, cardW - 4, cardH - 4, ev.badgeColor); // Color de la fe / evento
        // Esquinas doradas
        drawRect(cardX, cardY, 3, 3, ColorRGB(212, 175, 55));
        drawRect(cardX + cardW - 3, cardY, 3, 3, ColorRGB(212, 175, 55));

        std::string tag = "[" + Timeline::formatYear(ev.year) + "] " + ev.title;
        drawText(cardX + 8, cardY + 8, tag.substr(0, 30), ColorRGB(212, 175, 55), 1); // Título en Oro Divino

        std::string sub = ev.description.substr(0, 35) + "...";
        drawText(cardX + 8, cardY + 28, sub, ColorRGB(226, 218, 196), 1); // Texto en Hueso Reliquia
    }
}

bool Renderer::exportFrameToPPM(const std::string& filepath) {
    std::ofstream out(filepath, std::ios::binary);
    if (!out.is_open()) return false;

    out << "P6\n" << m_width << " " << m_height << "\n255\n";

    std::vector<uint8_t> rgbBuf(m_width * m_height * 3);
    for (int y = 0; y < m_height; ++y) {
        // PPM is written top-to-bottom
        int srcY = m_height - 1 - y;
        for (int x = 0; x < m_width; ++x) {
            uint32_t val = m_framebuffer[srcY * m_width + x];
            uint8_t r = val & 0xFF;
            uint8_t g = (val >> 8) & 0xFF;
            uint8_t b = (val >> 16) & 0xFF;

            int dstIdx = (y * m_width + x) * 3;
            rgbBuf[dstIdx + 0] = r;
            rgbBuf[dstIdx + 1] = g;
            rgbBuf[dstIdx + 2] = b;
        }
    }

    out.write(reinterpret_cast<const char*>(rgbBuf.data()), rgbBuf.size());
    return true;
}

void Renderer::renderMainMenu(int hoveredButtonId) {
    if (!m_menuBgPixels.empty() && m_bgWidth > 0 && m_bgHeight > 0) {
        // Renderizar la textura de pared de basalto/ladrillos litúrgicos
        for (int y = 0; y < m_height; ++y) {
            int srcY = (y * m_bgHeight) / m_height;
            // Invertir Y para la convención del framebuffer (abajo-arriba vs arriba-abajo)
            int frameY = m_height - 1 - y;
            int rowOffset = frameY * m_width;
            int srcRowOffset = srcY * m_bgWidth;

            for (int x = 0; x < m_width; ++x) {
                int srcX = (x * m_bgWidth) / m_width;
                m_framebuffer[rowOffset + x] = m_menuBgPixels[srcRowOffset + srcX];
            }
        }
    } else {
        // Fondo cósmico sagrado en Basalto Monástico (#1A1A1A) de respaldo
        clear(Palette::BASALT);

        // Patrón geométrico sutil de fondo (Líneas sacras en bajorrelieve)
        for (int y = 0; y < m_height; y += 40) {
            drawLine(0, y, m_width, y, ColorRGB{26, 28, 34});
        }
        for (int x = 0; x < m_width; x += 40) {
            drawLine(x, 0, x, m_height, ColorRGB{26, 28, 34});
        }
    }

    // Marco ornamental de toda la pantalla (Borde exterior de catedral)
    drawRectOutline(12, 12, m_width - 24, m_height - 24, ColorRGB{77, 77, 77}); // #4D4D4D
    drawRectOutline(16, 16, m_width - 32, m_height - 32, Palette::DIVINE_GOLD); // #E6A13B

    // Esquinas ornamentales del marco principal
    constexpr int CORNER = 14;
    drawRect(12, 12, CORNER, CORNER, Palette::DIVINE_GOLD);
    drawRect(m_width - 12 - CORNER, 12, CORNER, CORNER, Palette::DIVINE_GOLD);
    drawRect(12, m_height - 12 - CORNER, CORNER, CORNER, Palette::DIVINE_GOLD);
    drawRect(m_width - 12 - CORNER, m_height - 12 - CORNER, CORNER, CORNER, Palette::DIVINE_GOLD);

    // Título Central del Juego (Tipografía Sagrada en Oro Divino)
    const std::string title1 = "MARANGATU: THE FAITH SIMULATOR";
    const std::string title2 = "2.5D ISOMETRIC RELIGION SIMULATOR";
    const std::string subtitle = "Edicion Liturgica y Espiritual";

    const int title1X = (m_width - static_cast<int>(title1.size() * 12)) / 2;
    const int title2X = (m_width - static_cast<int>(title2.size() * 6)) / 2;
    const int subX = (m_width - static_cast<int>(subtitle.size() * 6)) / 2;

    const int titleY = 110;
    drawText(title1X, titleY, title1, Palette::DIVINE_GOLD, 2);
    drawText(title2X, titleY + 32, title2, Palette::RELIC_BONE, 1);
    drawText(subX, titleY + 50, subtitle, ColorRGB{204, 181, 189}, 1); // Lino Litúrgico #CCB5BD

    // Línea divisoria ceremonial bajo el título
    const int lineW = 420;
    const int lineX = (m_width - lineW) / 2;
    drawLine(lineX, titleY + 68, lineX + lineW, titleY + 68, Palette::DIVINE_GOLD);
    drawRect(m_width / 2 - 4, titleY + 65, 8, 8, Palette::BLOOD_RUBY); // Gema central

    // Panel contenedor de los botones centrados (Relicario central)
    const float btnW = 280.0f;
    const float btnH = 46.0f;
    const float spacing = 18.0f;
    const float totalH = 4 * btnH + 3 * spacing;

    const int panelPadding = 24;
    const int panelX = static_cast<int>((m_width - btnW) * 0.5f) - panelPadding;
    const int panelY = static_cast<int>((m_height - totalH) * 0.5f) + 40 - panelPadding;
    const int panelW = static_cast<int>(btnW) + panelPadding * 2;
    const int panelH = static_cast<int>(totalH) + panelPadding * 2;

    drawRect(panelX, panelY, panelW, panelH, ColorRGB{24, 24, 28, 230});
    drawRectOutline(panelX, panelY, panelW, panelH, ColorRGB{77, 77, 77});
    drawRectOutline(panelX + 2, panelY + 2, panelW - 4, panelH - 4, Palette::DIVINE_GOLD);

    // Renderizar los 4 Botones centrados: Crear mundo, Cargar Mundo, Opciones, Salir
    for (const auto& btn : m_menuButtons) {
        bool isHovered = (btn.id == hoveredButtonId);

        const int bx = static_cast<int>(btn.x);
        const int by = static_cast<int>(btn.y);
        const int bw = static_cast<int>(btn.width);
        const int bh = static_cast<int>(btn.height);

        if (isHovered) {
            // Estado Hover / Activo: Fondo cálido, doble marco de oro y esquinas iluminadas
            drawRect(bx, by, bw, bh, ColorRGB{48, 40, 26});
            drawRectOutline(bx, by, bw, bh, Palette::DIVINE_GOLD);
            drawRectOutline(bx + 1, by + 1, bw - 2, bh - 2, ColorRGB{255, 215, 80});

            // Esquinas ornamentales doradas
            drawRect(bx, by, 4, 4, ColorRGB{255, 230, 120});
            drawRect(bx + bw - 4, by, 4, 4, ColorRGB{255, 230, 120});
            drawRect(bx, by + bh - 4, 4, 4, ColorRGB{255, 230, 120});
            drawRect(bx + bw - 4, by + bh - 4, 4, 4, ColorRGB{255, 230, 120});

            // Texto brillante en Oro Divino
            int tx = bx + (bw - static_cast<int>(btn.label.size() * 6)) / 2;
            int ty = by + (bh - 7) / 2;
            drawText(tx, ty, btn.label, ColorRGB{255, 245, 220}, 1);
        } else {
            // Estado Normal: Relieve de mármol/basalto
            drawRect(bx, by, bw, bh, ColorRGB{36, 36, 40});
            drawRectOutline(bx, by, bw, bh, ColorRGB{77, 77, 77}); // #4D4D4D
            drawLine(bx + 1, by + 1, bx + bw - 2, by + 1, ColorRGB{90, 90, 95}); // Bisel superior

            int tx = bx + (bw - static_cast<int>(btn.label.size() * 6)) / 2;
            int ty = by + (bh - 7) / 2;
            drawText(tx, ty, btn.label, Palette::RELIC_BONE, 1);
        }
    }

    // Pie de página de la pantalla principal
    const std::string footer = "C++20 ISOMETRIC ENGINE | PALETA LITURGICA | TECLAS: ESC PARA SALIR";
    const int footerX = (m_width - static_cast<int>(footer.size() * 6)) / 2;
    drawText(footerX, m_height - 35, footer, ColorRGB{120, 120, 130}, 1);
}

void Renderer::renderOptionsMenu(int hoveredButtonId) {
    clear(ColorRGB{18, 11, 10}); // #120b0a

    // Marco exterior
    drawRectOutline(12, 12, m_width - 24, m_height - 24, ColorRGB{77, 77, 77});
    drawRectOutline(16, 16, m_width - 32, m_height - 32, Palette::DIVINE_GOLD);

    // Título
    const std::string title = "OPCIONES DEL SISTEMA";
    const int titleX = (m_width - static_cast<int>(title.size() * 12)) / 2;
    drawText(titleX, 140, title, Palette::DIVINE_GOLD, 2);

    // Botones de opciones
    for (const auto& btn : m_optionsButtons) {
        bool isHovered = (btn.id == hoveredButtonId);

        const int bx = static_cast<int>(btn.x);
        const int by = static_cast<int>(btn.y);
        const int bw = static_cast<int>(btn.width);
        const int bh = static_cast<int>(btn.height);

        if (isHovered) {
            drawRect(bx, by, bw, bh, ColorRGB{48, 40, 26});
            drawRectOutline(bx, by, bw, bh, Palette::DIVINE_GOLD);
            int tx = bx + (bw - static_cast<int>(btn.label.size() * 6)) / 2;
            int ty = by + (bh - 7) / 2;
            drawText(tx, ty, btn.label, ColorRGB{255, 245, 220}, 1);
        } else {
            drawRect(bx, by, bw, bh, ColorRGB{36, 36, 40});
            drawRectOutline(bx, by, bw, bh, ColorRGB{77, 77, 77});
            int tx = bx + (bw - static_cast<int>(btn.label.size() * 6)) / 2;
            int ty = by + (bh - 7) / 2;
            drawText(tx, ty, btn.label, Palette::RELIC_BONE, 1);
        }
    }
}

void Renderer::renderWorldCreationMenu(const WorldCreationConfig& config, int hoveredButtonId, int activeInputField) {
    // Fondo de muro de basalto litúrgico
    if (!m_menuBgPixels.empty() && m_bgWidth > 0 && m_bgHeight > 0) {
        for (int y = 0; y < m_height; ++y) {
            int srcY = (y * m_bgHeight) / m_height;
            int frameY = m_height - 1 - y;
            int rowOffset = frameY * m_width;
            int srcRowOffset = srcY * m_bgWidth;
            for (int x = 0; x < m_width; ++x) {
                int srcX = (x * m_bgWidth) / m_width;
                m_framebuffer[rowOffset + x] = m_menuBgPixels[srcRowOffset + srcX];
            }
        }
    } else {
        clear(Palette::BASALT);
    }

    // Marco exterior de la pantalla
    drawRectOutline(12, 12, m_width - 24, m_height - 24, ColorRGB{77, 77, 77});
    drawRectOutline(16, 16, m_width - 32, m_height - 32, Palette::DIVINE_GOLD);

    // Modal / Altar central de Creación
    const float modalW = 880.0f;
    const float modalH = 590.0f;
    const int mx = static_cast<int>((m_width - modalW) * 0.5f);
    const int my = static_cast<int>((m_height - modalH) * 0.5f);

    // Fondo del relicario con semitransparencia simulada
    drawRect(mx, my, static_cast<int>(modalW), static_cast<int>(modalH), ColorRGB{20, 20, 24});
    drawRectOutline(mx, my, static_cast<int>(modalW), static_cast<int>(modalH), ColorRGB{80, 80, 85});
    drawRectOutline(mx + 3, my + 3, static_cast<int>(modalW) - 6, static_cast<int>(modalH) - 6, Palette::DIVINE_GOLD);

    // Esquinas ornamentales del modal
    constexpr int CORNER = 12;
    drawRect(mx + 3, my + 3, CORNER, CORNER, Palette::DIVINE_GOLD);
    drawRect(mx + static_cast<int>(modalW) - 3 - CORNER, my + 3, CORNER, CORNER, Palette::DIVINE_GOLD);
    drawRect(mx + 3, my + static_cast<int>(modalH) - 3 - CORNER, CORNER, CORNER, Palette::DIVINE_GOLD);
    drawRect(mx + static_cast<int>(modalW) - 3 - CORNER, my + static_cast<int>(modalH) - 3 - CORNER, CORNER, CORNER, Palette::DIVINE_GOLD);

    // Título del Relicario de Génesis
    const std::string title = "GENESIS: CREACION DEL MUNDO SAGRADO";
    const int titleX = mx + (static_cast<int>(modalW) - static_cast<int>(title.size() * 12)) / 2;
    drawText(titleX, my + 24, title, Palette::DIVINE_GOLD, 2);

    // Línea divisoria ceremonial
    drawLine(mx + 30, my + 52, mx + static_cast<int>(modalW) - 30, my + 52, Palette::DIVINE_GOLD);
    drawRect(mx + static_cast<int>(modalW) / 2 - 4, my + 49, 8, 8, Palette::BLOOD_RUBY);

    // Subtítulos de columnas
    const int col1X = mx + 40;
    const int col2X = mx + static_cast<int>(modalW) / 2 + 20;
    drawText(col1X, my + 62, "I. IDENTIDAD DIVINA Y TIERRA", Palette::DIVINE_GOLD, 1);
    drawText(col2X, my + 62, "II. DOGMA Y CONDICIONES DEL MUNDO", Palette::DIVINE_GOLD, 1);

    // Paleta de colores de influencia
    static const ColorRGB INFLUENCE_PALETTE[5] = {
        ColorRGB{230, 161, 59},  // Oro Divino
        ColorRGB{128, 0, 32},    // Rubí Sangre
        ColorRGB{52, 152, 219},  // Monada Azul
        ColorRGB{46, 204, 113},  // Esmeralda Gaia
        ColorRGB{155, 89, 182}   // Púrpura Dharma
    };
    static const char* INFLUENCE_NAMES[5] = {
        "Oro Divino", "Rubi Sangre", "Monada Azul", "Esmeralda Gaia", "Purpura Dharma"
    };

    // Nombres de Épocas
    static const char* ERA_NAMES[8] = {
        "Primordial Animism (-4000)",
        "Totemism & Ancestor (-2500)",
        "Structural Shamanism (-1000)",
        "Territorial Polytheism (0)",
        "Universal Religions (1000)",
        "Rationalization (1800)",
        "Post-Secular Age (2020)",
        "Transhumanist Singularity (2100)"
    };

    // Nombres de Tipo de Partida
    static const char* MODE_NAMES[4] = {
        "Estandar (Historico)", "Sandbox (Libre)", "Teocracia Total", "Supervivencia Extrema"
    };

    // Dificultad IA
    static const char* DIFF_NAMES[4] = {
        "Facil (Pasiva)", "Normal (Equilibrada)", "Dificil (Celosa)", "Pesadilla (Fanatica)"
    };

    // Tipo de Victoria
    static const char* VICTORY_NAMES[5] = {
        "Hegemonia (75% Poblacion)", "Teocratica (5 Maravillas)",
        "Trascendencia (Era 8)", "Tiempo (Mayor Fe al 2200)", "Sin Limite (Eterno)"
    };

    // Tamaños de Mapa
    static const char* MAP_SIZES[3] = { "Pequeno (32x32)", "Mediano (48x48)", "Grande (64x64)" };
    static const char* DISASTER_LEVELS[3] = { "Bajas", "Normales", "Frecuentes / Apocalipticas" };

    // Etiquetas fijas de cada campo
    auto drawFieldLabel = [&](int x, int y, const std::string& label) {
        drawText(x, y - 12, label, Palette::RELIC_BONE, 1);
    };

    drawFieldLabel(col1X, my + 80, "Nombre del Dios que seras:");
    drawFieldLabel(col1X, my + 140, "Nombre de la Religion:");
    drawFieldLabel(col1X, my + 200, "Semilla del Mapa (Seed):");
    drawFieldLabel(col1X, my + 260, "Poblacion Inicial del Mundo:");
    drawFieldLabel(col1X, my + 320, "Epoca Historica de Inicio:");
    drawFieldLabel(col1X, my + 380, "Tamano del Mundo Isomtrico:");
    drawFieldLabel(col1X, my + 440, "Frecuencia de Cataclismos/Plagas:");

    drawFieldLabel(col2X, my + 80, "Tipo de Partida:");
    drawFieldLabel(col2X, my + 140, "Cofre Sagrado de Inicio:");
    drawFieldLabel(col2X, my + 200, "Dificultad de las IAs Rival:");
    drawFieldLabel(col2X, my + 260, "Color de Influencia / Creyentes:");
    drawFieldLabel(col2X, my + 320, "Condicion de Victoria:");
    drawFieldLabel(col2X, my + 380, "Cismas Doctrinales Espontaneos:");
    drawFieldLabel(col2X, my + 440, "Sincretismo / Fusion Interreligiosa:");

    // Renderizado de botones y campos interactivos
    for (const auto& btn : m_creationButtons) {
        bool isHovered = (btn.id == hoveredButtonId);
        bool isActive = (btn.id == activeInputField);

        const int bx = static_cast<int>(btn.x);
        const int by = static_cast<int>(btn.y);
        const int bw = static_cast<int>(btn.width);
        const int bh = static_cast<int>(btn.height);

        // Determinación del contenido y color a mostrar
        std::string displayText = btn.label;
        ColorRGB textColor = Palette::RELIC_BONE;

        if (btn.id == 501) {
            displayText = config.godName + (isActive ? "_" : "");
            textColor = ColorRGB{255, 235, 170};
        } else if (btn.id == 502) {
            displayText = config.religionName + (isActive ? "_" : "");
            textColor = ColorRGB{255, 235, 170};
        } else if (btn.id == 503) {
            displayText = std::to_string(config.mapSeed);
            textColor = ColorRGB{255, 235, 170};
        } else if (btn.id == 504) {
            displayText = "[RAND]";
            textColor = Palette::DIVINE_GOLD;
        } else if (btn.id == 505) {
            displayText = std::to_string(config.initialPopulation) + " aldeanos";
        } else if (btn.id == 506) {
            int idx = static_cast<int>(config.startingEra);
            displayText = (idx >= 0 && idx < 8) ? ERA_NAMES[idx] : "Primordial Animism";
        } else if (btn.id == 507) {
            displayText = (config.mapSizeIndex >= 0 && config.mapSizeIndex < 3) ? MAP_SIZES[config.mapSizeIndex] : "Mediano";
        } else if (btn.id == 508) {
            displayText = (config.disasterFrequency >= 0 && config.disasterFrequency < 3) ? DISASTER_LEVELS[config.disasterFrequency] : "Normales";
        } else if (btn.id == 509) {
            int idx = static_cast<int>(config.gameMode);
            displayText = (idx >= 0 && idx < 4) ? MODE_NAMES[idx] : "Estandar";
        } else if (btn.id == 510) {
            displayText = config.starterChest ? "[x] ACTIVADO (+1000 Fe & Reliquia)" : "[ ] DESACTIVADO";
            textColor = config.starterChest ? ColorRGB{120, 230, 120} : ColorRGB{180, 180, 180};
        } else if (btn.id == 511) {
            int idx = static_cast<int>(config.aiDifficulty);
            displayText = (idx >= 0 && idx < 4) ? DIFF_NAMES[idx] : "Normal";
        } else if (btn.id == 512) {
            int idx = config.influenceColorIndex % 5;
            displayText = std::string(INFLUENCE_NAMES[idx]);
            textColor = INFLUENCE_PALETTE[idx];
        } else if (btn.id == 513) {
            int idx = static_cast<int>(config.victoryCondition);
            displayText = (idx >= 0 && idx < 5) ? VICTORY_NAMES[idx] : "Hegemonia";
        } else if (btn.id == 514) {
            displayText = config.allowSchisms ? "[x] PERMITIDOS (Herejias dinamicas)" : "[ ] BLOQUEADOS (Ortodoxia pura)";
            textColor = config.allowSchisms ? Palette::BLOOD_RUBY : ColorRGB{180, 180, 180};
        } else if (btn.id == 515) {
            displayText = config.allowSyncretism ? "[x] PERMITIDO (Comunion ecumenica)" : "[ ] BLOQUEADO (Aislamiento)";
            textColor = config.allowSyncretism ? ColorRGB{120, 200, 255} : ColorRGB{180, 180, 180};
        }

        // Renderizado del botón/caja
        if (btn.id == 551) {
            // Botón principal COMENZAR GENESIS
            drawRect(bx, by, bw, bh, isHovered ? ColorRGB{140, 30, 40} : Palette::BLOOD_RUBY);
            drawRectOutline(bx, by, bw, bh, Palette::DIVINE_GOLD);
            drawRectOutline(bx + 1, by + 1, bw - 2, bh - 2, ColorRGB{255, 215, 80});
            int tx = bx + (bw - static_cast<int>(displayText.size() * 6)) / 2;
            int ty = by + (bh - 7) / 2;
            drawText(tx, ty, displayText, ColorRGB{255, 255, 230}, 1);
        } else if (btn.id == 550) {
            // Botón CANCELAR
            drawRect(bx, by, bw, bh, isHovered ? ColorRGB{55, 40, 40} : ColorRGB{36, 36, 40});
            drawRectOutline(bx, by, bw, bh, isHovered ? Palette::DIVINE_GOLD : ColorRGB{80, 80, 80});
            int tx = bx + (bw - static_cast<int>(displayText.size() * 6)) / 2;
            int ty = by + (bh - 7) / 2;
            drawText(tx, ty, displayText, Palette::RELIC_BONE, 1);
        } else {
            // Campos de selección y entrada
            ColorRGB bgCol = isActive ? ColorRGB{45, 40, 30} : (isHovered ? ColorRGB{40, 36, 34} : ColorRGB{28, 28, 32});
            ColorRGB borderCol = isActive ? ColorRGB{255, 215, 80} : (isHovered ? Palette::DIVINE_GOLD : ColorRGB{70, 70, 75});

            drawRect(bx, by, bw, bh, bgCol);
            drawRectOutline(bx, by, bw, bh, borderCol);

            // Muestra de color al lado del selector de color de influencia
            if (btn.id == 512) {
                int colIdx = config.influenceColorIndex % 5;
                drawRect(bx + bw - 24, by + 4, 18, bh - 8, INFLUENCE_PALETTE[colIdx]);
                drawRectOutline(bx + bw - 24, by + 4, 18, bh - 8, Palette::DIVINE_GOLD);
            }

            int tx = bx + 8;
            int ty = by + (bh - 7) / 2;
            drawText(tx, ty, displayText, textColor, 1);
        }
    }

    // Pie de la ventana modal
    const std::string hint = "CLIC EN CAMPOS PARA EDITAR O CAMBIAR | ENTER O BOTON ROJO PARA INICIAR | ESC PARA CANCELAR";
    const int hintX = mx + (static_cast<int>(modalW) - static_cast<int>(hint.size() * 6)) / 2;
    drawText(hintX, my + static_cast<int>(modalH) - 18, hint, ColorRGB{150, 145, 140}, 1);
}

void Renderer::renderLoadWorldMenu(int hoveredButtonId, int selectedSaveIndex) {
    // Fondo de muro de basalto litúrgico
    if (!m_menuBgPixels.empty() && m_bgWidth > 0 && m_bgHeight > 0) {
        for (int y = 0; y < m_height; ++y) {
            int srcY = (y * m_bgHeight) / m_height;
            int frameY = m_height - 1 - y;
            int rowOffset = frameY * m_width;
            int srcRowOffset = srcY * m_bgWidth;
            for (int x = 0; x < m_width; ++x) {
                int srcX = (x * m_bgWidth) / m_width;
                m_framebuffer[rowOffset + x] = m_menuBgPixels[srcRowOffset + srcX];
            }
        }
    } else {
        clear(Palette::BASALT);
    }

    // Marco exterior de la catedral
    drawRectOutline(12, 12, m_width - 24, m_height - 24, ColorRGB{77, 77, 77});
    drawRectOutline(16, 16, m_width - 32, m_height - 32, Palette::DIVINE_GOLD);

    // Modal central estilo "Select World" de Minecraft
    const float modalW = 880.0f;
    const float modalH = 590.0f;
    const int mx = static_cast<int>((m_width - modalW) * 0.5f);
    const int my = static_cast<int>((m_height - modalH) * 0.5f);

    drawRect(mx, my, static_cast<int>(modalW), static_cast<int>(modalH), ColorRGB{20, 20, 24});
    drawRectOutline(mx, my, static_cast<int>(modalW), static_cast<int>(modalH), ColorRGB{80, 80, 85});
    drawRectOutline(mx + 3, my + 3, static_cast<int>(modalW) - 6, static_cast<int>(modalH) - 6, Palette::DIVINE_GOLD);

    // Esquinas doradas
    constexpr int CORNER = 12;
    drawRect(mx + 3, my + 3, CORNER, CORNER, Palette::DIVINE_GOLD);
    drawRect(mx + static_cast<int>(modalW) - 3 - CORNER, my + 3, CORNER, CORNER, Palette::DIVINE_GOLD);
    drawRect(mx + 3, my + static_cast<int>(modalH) - 3 - CORNER, CORNER, CORNER, Palette::DIVINE_GOLD);
    drawRect(mx + static_cast<int>(modalW) - 3 - CORNER, my + static_cast<int>(modalH) - 3 - CORNER, CORNER, CORNER, Palette::DIVINE_GOLD);

    // Título Superior
    const std::string title = "SELECCIONAR MUNDO / CRONICAS GUARDADAS";
    const int titleX = mx + (static_cast<int>(modalW) - static_cast<int>(title.size() * 12)) / 2;
    drawText(titleX, my + 22, title, Palette::DIVINE_GOLD, 2);

    drawLine(mx + 30, my + 50, mx + static_cast<int>(modalW) - 30, my + 50, Palette::DIVINE_GOLD);
    drawRect(mx + static_cast<int>(modalW) / 2 - 4, my + 47, 8, 8, Palette::BLOOD_RUBY);

    // Obtener lista de mundos reales
    auto saves = SaveSystem::scanSaveSlots();

    const int listX = mx + 40;
    const int listY = my + 72;
    const int slotW = static_cast<int>(modalW) - 80;
    const int slotH = 76;
    const int spacing = 12;

    if (saves.empty()) {
        std::string emptyMsg = "No se encontraron mundos guardados. Presiona 'CREAR UN NUEVO MUNDO' abajo.";
        int ex = mx + (static_cast<int>(modalW) - static_cast<int>(emptyMsg.size() * 6)) / 2;
        drawText(ex, my + 240, emptyMsg, Palette::RELIC_BONE, 1);
    } else {
        for (size_t i = 0; i < saves.size() && i < 5; ++i) {
            int sy = listY + static_cast<int>(i) * (slotH + spacing);
            bool isSelected = (static_cast<int>(i) == selectedSaveIndex);
            bool isHovered = (hoveredButtonId == static_cast<int>(1000 + i));

            ColorRGB bgCol = isSelected ? ColorRGB{48, 42, 30} : (isHovered ? ColorRGB{38, 36, 40} : ColorRGB{26, 26, 30});
            ColorRGB borderCol = isSelected ? ColorRGB{255, 215, 80} : (isHovered ? Palette::DIVINE_GOLD : ColorRGB{70, 70, 75});

            drawRect(listX, sy, slotW, slotH, bgCol);
            drawRectOutline(listX, sy, slotW, slotH, borderCol);

            // Thumbnail / Icon estilo Minecraft (cuadrado 64x64)
            int iconBoxX = listX + 6;
            int iconBoxY = sy + 6;
            int iconBoxS = 64;
            drawRect(iconBoxX, iconBoxY, iconBoxS, iconBoxS, ColorRGB{14, 14, 18});
            drawRectOutline(iconBoxX, iconBoxY, iconBoxS, iconBoxS, Palette::DIVINE_GOLD);

            // Si tiene icono/ppm o dibujo simbólico
            drawText(iconBoxX + 16, iconBoxY + 26, "FAITH", Palette::DIVINE_GOLD, 1);

            // Información del Mundo
            int textX = listX + 80;
            // 1. Nombre del mundo
            drawText(textX, sy + 10, saves[i].worldName, ColorRGB{255, 240, 200}, 1);

            // 2. Carpeta y fecha de última partida
            std::string subline = saves[i].folderName + " (" + SaveSystem::formatTimestamp(saves[i].lastPlayedTimestamp) + ")";
            drawText(textX, sy + 28, subline, ColorRGB{160, 160, 170}, 1);

            // 3. Resumen (Época, Año, Población, Religiones)
            std::string statsLine = SaveSystem::formatSummary(saves[i]) + " | " + std::to_string(saves[i].activeReligions) + " fes activas";
            drawText(textX, sy + 46, statsLine, Palette::RELIC_BONE, 1);

            // Indicador de selección lateral
            if (isSelected) {
                drawRect(listX + slotW - 12, sy + 6, 6, slotH - 12, Palette::DIVINE_GOLD);
            }
        }
    }

    // Botones de Acción Inferiores (Idénticos al layout de Minecraft)
    const int btnY = my + static_cast<int>(modalH) - 56;

    // ID 601: JUGAR EN EL MUNDO SELECCIONADO
    bool playHovered = (hoveredButtonId == 601);
    bool canPlay = !saves.empty() && selectedSaveIndex >= 0 && selectedSaveIndex < static_cast<int>(saves.size());
    ColorRGB playBg = canPlay ? (playHovered ? ColorRGB{140, 30, 40} : Palette::BLOOD_RUBY) : ColorRGB{34, 34, 38};
    drawRect(mx + 40, btnY, 220, 42, playBg);
    drawRectOutline(mx + 40, btnY, 220, 42, canPlay ? Palette::DIVINE_GOLD : ColorRGB{70, 70, 70});
    drawText(mx + 40 + (220 - static_cast<int>(std::string("JUGAR MUNDO").size() * 6)) / 2, btnY + 17, "JUGAR MUNDO", canPlay ? ColorRGB{255, 255, 230} : ColorRGB{120, 120, 120}, 1);

    // ID 602: CREAR UN NUEVO MUNDO
    bool createHovered = (hoveredButtonId == 602);
    drawRect(mx + 280, btnY, 220, 42, createHovered ? ColorRGB{48, 40, 26} : ColorRGB{36, 36, 40});
    drawRectOutline(mx + 280, btnY, 220, 42, createHovered ? Palette::DIVINE_GOLD : ColorRGB{80, 80, 80});
    drawText(mx + 280 + (220 - static_cast<int>(std::string("CREAR NUEVO").size() * 6)) / 2, btnY + 17, "CREAR NUEVO", Palette::DIVINE_GOLD, 1);

    // ID 603: ELIMINAR MUNDO
    bool delHovered = (hoveredButtonId == 603);
    ColorRGB delBg = canPlay ? (delHovered ? ColorRGB{60, 20, 20} : ColorRGB{36, 36, 40}) : ColorRGB{28, 28, 30};
    drawRect(mx + 520, btnY, 160, 42, delBg);
    drawRectOutline(mx + 520, btnY, 160, 42, canPlay ? (delHovered ? ColorRGB{220, 80, 80} : ColorRGB{80, 80, 80}) : ColorRGB{60, 60, 60});
    drawText(mx + 520 + (160 - static_cast<int>(std::string("ELIMINAR").size() * 6)) / 2, btnY + 17, "ELIMINAR", canPlay ? ColorRGB{220, 140, 140} : ColorRGB{100, 100, 100}, 1);

    // ID 604: CANCELAR
    bool cancelHovered = (hoveredButtonId == 604);
    drawRect(mx + 700, btnY, 140, 42, cancelHovered ? ColorRGB{48, 40, 26} : ColorRGB{36, 36, 40});
    drawRectOutline(mx + 700, btnY, 140, 42, cancelHovered ? Palette::DIVINE_GOLD : ColorRGB{80, 80, 80});
    drawText(mx + 700 + (140 - static_cast<int>(std::string("CANCELAR").size() * 6)) / 2, btnY + 17, "CANCELAR", Palette::RELIC_BONE, 1);
}

} // namespace ReligionSim
