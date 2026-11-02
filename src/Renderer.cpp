#include "Renderer.hpp"
#include "DayNightCycle.hpp"
#include "CloudSystem.hpp"
#include "SaveSystem.hpp"
#include "VillagerSpriteData.hpp"
#include "DejaVuSansMonoFont.hpp"
#include "NemesisSififus.hpp"
#include "ReligiousMoralitySystem.hpp"
#include "ErrorSystem.hpp"
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
#ifdef None
#undef None
#endif
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

    constexpr std::array<const char*, 5> RESOURCE_PROP_PATHS{{
        "assets/sprites/resources/resource_food.rgba",
        "assets/sprites/resources/resource_wood.rgba",
        "assets/sprites/resources/resource_stone.rgba",
        "assets/sprites/resources/resource_gold.rgba",
        "assets/sprites/resources/resource_iron.rgba"
    }};

    // Funciones procedurales de ruido orgánico para el Sistema de Cohesión de Terreno
    inline uint32_t hash2D(int x, int y) {
        uint32_t h = static_cast<uint32_t>(x) * 374761393u + static_cast<uint32_t>(y) * 668265263u;
        h = (h ^ (h >> 13)) * 1274126177u;
        return h ^ (h >> 16);
    }

    inline float organicNoise(float x, float y) {
        int ix = static_cast<int>(std::floor(x));
        int iy = static_cast<int>(std::floor(y));
        float fx = x - static_cast<float>(ix);
        float fy = y - static_cast<float>(iy);
        float sx = fx * fx * (3.0f - 2.0f * fx);
        float sy = fy * fy * (3.0f - 2.0f * fy);
        auto grad = [](int gx, int gy) -> float {
            return static_cast<float>(hash2D(gx, gy) & 0xFFFFu) / 65535.0f;
        };
        float v00 = grad(ix, iy);
        float v10 = grad(ix + 1, iy);
        float v01 = grad(ix, iy + 1);
        float v11 = grad(ix + 1, iy + 1);
        float nx0 = v00 * (1.0f - sx) + v10 * sx;
        float nx1 = v01 * (1.0f - sx) + v11 * sx;
        return nx0 * (1.0f - sy) + nx1 * sy;
    }

    inline float organicTurbulence(float gx, float gy) {
        float n1 = organicNoise(gx * 3.2f, gy * 3.2f);
        float n2 = organicNoise(gx * 6.4f + 17.3f, gy * 6.4f + 31.7f) * 0.5f;
        return (n1 + n2) * 0.6667f - 0.5f; // Rango [-0.5f, 0.5f]
    }
}

Renderer::Renderer(int width, int height)
    : m_width(width), m_height(height) {
    m_framebuffer.resize(width * height, 0xFF1A1210);
    loadMenuBackground();
    loadSpaceBackground();
    loadCursor();
    loadTempleAndTerrainAssets();
    loadResourceAssets();
    loadBuildingAssets();
    loadButtonAssets();
    loadViewIconAssets();
    loadPowerIconAssets();
    loadEventIconAssets();
    loadFaithTreeIconAssets();
    initUI();
    initStars();
    initCosmicBuffer();
    initTerrainTables();
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

    // 4. Cargar hoja de sprites de la Mujer / Aldeana Femenina (256x256, cuadrícula 4x4 de 64x64)
    if (!m_femaleTileSet.cargarDesdeArchivo("assets/sprites/units/humano_female/humano_female_sheet.rgba")) {
        if (!m_femaleTileSet.cargarDesdeArchivo("../assets/sprites/units/humano_female/humano_female_sheet.rgba")) {
            if (!m_femaleTileSet.cargarDesdeArchivo("religion_simulator_isometric/assets/sprites/units/humano_female/humano_female_sheet.rgba")) {
                m_femaleTileSet.cargarDesdeArchivo("/home/Ivancho/Documentos/religion_simulator_isometric/assets/sprites/units/humano_female/humano_female_sheet.rgba");
            }
        }
    }
    m_femaleTileSet.inicializarMapaRectangulosFemenino();
}

void Renderer::loadResourceAssets() {
    for (size_t resIndex = 0; resIndex < RESOURCE_PROP_PATHS.size(); ++resIndex) {
        std::vector<std::string> propPaths = {
            RESOURCE_PROP_PATHS[resIndex],
            std::string("../") + RESOURCE_PROP_PATHS[resIndex],
            std::string("religion_simulator_isometric/") + RESOURCE_PROP_PATHS[resIndex],
            std::string("/home/Ivancho/Documentos/religion_simulator_isometric/") + RESOURCE_PROP_PATHS[resIndex]
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

            auto& pixels = m_resourcePropPixels[resIndex];
            pixels.resize(64u * 64u);
            for (size_t i = 0; i < pixels.size(); ++i) {
                pixels[i] = (static_cast<uint32_t>(raw[i * 4 + 3]) << 24) |
                            (static_cast<uint32_t>(raw[i * 4 + 2]) << 16) |
                            (static_cast<uint32_t>(raw[i * 4 + 1]) << 8) |
                            static_cast<uint32_t>(raw[i * 4]);
            }
            std::cout << "[Renderer] Sprite de fuente de recurso cargado: " << path << std::endl;
            loaded = true;
            break;
        }
        if (!loaded) {
            std::cerr << "[Renderer] No se pudo cargar el sprite de recurso: "
                      << RESOURCE_PROP_PATHS[resIndex] << std::endl;
        }
    }
}

void Renderer::loadBuildingAssets() {
    std::vector<std::string> paths = {
        "assets/sprites/buildings/tribal_buildings_sheet.rgba",
        "../assets/sprites/buildings/tribal_buildings_sheet.rgba",
        "religion_simulator_isometric/assets/sprites/buildings/tribal_buildings_sheet.rgba",
        "/home/Ivancho/Documentos/religion_simulator_isometric/assets/sprites/buildings/tribal_buildings_sheet.rgba"
    };
    for (const auto& path : paths) {
        std::ifstream file(path, std::ios::binary);
        if (file.is_open()) {
            uint32_t w = 0, h = 0;
            file.read(reinterpret_cast<char*>(&w), sizeof(w));
            file.read(reinterpret_cast<char*>(&h), sizeof(h));
            if (w == 1024 && h == 558) {
                m_buildingsWidth = static_cast<int>(w);
                m_buildingsHeight = static_cast<int>(h);
                size_t numPixels = w * h;
                std::vector<uint8_t> raw(numPixels * 4);
                file.read(reinterpret_cast<char*>(raw.data()), raw.size());
                m_buildingsPixels.resize(numPixels);
                for (size_t i = 0; i < numPixels; ++i) {
                    uint8_t r = raw[i * 4 + 0];
                    uint8_t g = raw[i * 4 + 1];
                    uint8_t b = raw[i * 4 + 2];
                    uint8_t a = raw[i * 4 + 3];
                    m_buildingsPixels[i] = (static_cast<uint32_t>(a) << 24) |
                                           (static_cast<uint32_t>(b) << 16) |
                                           (static_cast<uint32_t>(g) << 8)  |
                                            static_cast<uint32_t>(r);
                }
                std::cout << "[Renderer] Hoja de Viviendas y Edificios Tribales (" << w << "x" << h << ") cargada exitosamente: " << path << std::endl;
                break;
            }
        }
    }
}

void Renderer::loadButtonAssets() {
    std::vector<std::string> buttonPaths = {
        "assets/ui/buttons_ancient.rgba",
        "../assets/ui/buttons_ancient.rgba",
        "religion_simulator_isometric/assets/ui/buttons_ancient.rgba",
        "/home/Ivancho/Documentos/religion_simulator_isometric/assets/ui/buttons_ancient.rgba"
    };

    for (const auto& path : buttonPaths) {
        std::ifstream file(path, std::ios::binary);
        if (file.is_open()) {
            file.seekg(0, std::ios::end);
            size_t size = file.tellg();
            file.seekg(0, std::ios::beg);
            if (size == 256 * 128 * 4) {
                std::vector<uint8_t> raw(size);
                file.read(reinterpret_cast<char*>(raw.data()), size);
                m_buttonAtlasPixels.resize(256 * 128);
                m_buttonAtlasW = 256;
                m_buttonAtlasH = 128;
                for (size_t i = 0; i < 256 * 128; ++i) {
                    uint8_t r = raw[i * 4 + 0];
                    uint8_t g = raw[i * 4 + 1];
                    uint8_t b = raw[i * 4 + 2];
                    uint8_t a = raw[i * 4 + 3];
                    m_buttonAtlasPixels[i] = (static_cast<uint32_t>(a) << 24) |
                                            (static_cast<uint32_t>(b) << 16) |
                                            (static_cast<uint32_t>(g) << 8)  |
                                             static_cast<uint32_t>(r);
                }
                std::cout << "[Renderer] Atlas de Botones Antiguos (256x128) cargado exitosamente: " << path << std::endl;
                break;
            }
        }
    }
}

void Renderer::loadViewIconAssets() {
    std::vector<std::string> iconPaths = {
        "assets/ui/view_icons.rgba",
        "../assets/ui/view_icons.rgba",
        "religion_simulator_isometric/assets/ui/view_icons.rgba",
        "/home/Ivancho/Documentos/religion_simulator_isometric/assets/ui/view_icons.rgba"
    };

    for (const auto& path : iconPaths) {
        std::ifstream file(path, std::ios::binary);
        if (file.is_open()) {
            file.seekg(0, std::ios::end);
            size_t size = file.tellg();
            file.seekg(0, std::ios::beg);
            if (size == 80 * 64 * 4) {
                std::vector<uint8_t> raw(size);
                file.read(reinterpret_cast<char*>(raw.data()), size);
                m_viewIconPixels.resize(80 * 64);
                m_viewIconW = 80;
                m_viewIconH = 64;
                for (size_t i = 0; i < 80 * 64; ++i) {
                    uint8_t r = raw[i * 4 + 0];
                    uint8_t g = raw[i * 4 + 1];
                    uint8_t b = raw[i * 4 + 2];
                    uint8_t a = raw[i * 4 + 3];
                    m_viewIconPixels[i] = (static_cast<uint32_t>(a) << 24) |
                                          (static_cast<uint32_t>(b) << 16) |
                                          (static_cast<uint32_t>(g) << 8)  |
                                           static_cast<uint32_t>(r);
                }
                std::cout << "[Renderer] Iconos de Vistas de Mapa (80x64) cargados exitosamente: " << path << std::endl;
                break;
            }
        }
    }
}

void Renderer::loadPowerIconAssets() {
    std::vector<std::string> iconPaths = {
        "assets/ui/power_icons.rgba",
        "../assets/ui/power_icons.rgba",
        "religion_simulator_isometric/assets/ui/power_icons.rgba",
        "/home/Ivancho/Documentos/religion_simulator_isometric/assets/ui/power_icons.rgba"
    };

    for (const auto& path : iconPaths) {
        std::ifstream file(path, std::ios::binary);
        if (file.is_open()) {
            file.seekg(0, std::ios::end);
            size_t size = file.tellg();
            file.seekg(0, std::ios::beg);
            if (size > 0 && (size % (64 * 4) == 0)) {
                int w = static_cast<int>(size / (64 * 4));
                std::vector<uint8_t> raw(size);
                file.read(reinterpret_cast<char*>(raw.data()), size);
                m_powerIconPixels.resize(w * 64);
                m_powerIconW = w;
                m_powerIconH = 64;
                for (size_t i = 0; i < static_cast<size_t>(w * 64); ++i) {
                    uint8_t r = raw[i * 4 + 0];
                    uint8_t g = raw[i * 4 + 1];
                    uint8_t b = raw[i * 4 + 2];
                    uint8_t a = raw[i * 4 + 3];
                    m_powerIconPixels[i] = (static_cast<uint32_t>(a) << 24) |
                                          (static_cast<uint32_t>(b) << 16) |
                                          (static_cast<uint32_t>(g) << 8)  |
                                           static_cast<uint32_t>(r);
                }
                std::cout << "[Renderer] Iconos de Poderes Divinos (" << w << "x64) cargados exitosamente: " << path << std::endl;
                break;
            }
        }
    }
}

void Renderer::loadEventIconAssets() {
    std::vector<std::string> iconPaths = {
        "assets/ui/event_icons.rgba",
        "../assets/ui/event_icons.rgba",
        "religion_simulator_isometric/assets/ui/event_icons.rgba",
        "/home/Ivancho/Documentos/religion_simulator_isometric/assets/ui/event_icons.rgba"
    };

    for (const auto& path : iconPaths) {
        std::ifstream file(path, std::ios::binary);
        if (file.is_open()) {
            file.seekg(0, std::ios::end);
            size_t size = file.tellg();
            file.seekg(0, std::ios::beg);
            if (size == 144 * 48 * 4) {
                std::vector<uint8_t> raw(size);
                file.read(reinterpret_cast<char*>(raw.data()), size);
                m_eventIconPixels.resize(144 * 48);
                m_eventIconW = 144;
                m_eventIconH = 48;
                for (size_t i = 0; i < 144 * 48; ++i) {
                    uint8_t r = raw[i * 4 + 0];
                    uint8_t g = raw[i * 4 + 1];
                    uint8_t b = raw[i * 4 + 2];
                    uint8_t a = raw[i * 4 + 3];
                    m_eventIconPixels[i] = (static_cast<uint32_t>(a) << 24) |
                                           (static_cast<uint32_t>(b) << 16) |
                                           (static_cast<uint32_t>(g) << 8)  |
                                            static_cast<uint32_t>(r);
                }
                std::cout << "[Renderer] Iconos de Cronica de Eventos (144x48) cargados exitosamente: " << path << std::endl;
                break;
            }
        }
    }
}

void Renderer::drawEventIcon(int x, int y, int categoryCol, bool glow) {
    if (m_eventIconPixels.size() != 144 * 48) return;
    int col = std::clamp(categoryCol, 0, 5);
    int row = glow ? 1 : 0;
    int srcX = col * 24;
    int srcY = row * 24;

    for (int iy = 0; iy < 24; ++iy) {
        int frameY = m_height - 1 - (y + iy);
        if (frameY < 0 || frameY >= m_height) continue;

        for (int ix = 0; ix < 24; ++ix) {
            int frameX = x + ix;
            if (frameX < 0 || frameX >= m_width) continue;

            uint32_t src = m_eventIconPixels[(srcY + iy) * 144 + (srcX + ix)];
            uint8_t a = (src >> 24) & 0xFF;
            if (a < 15) continue;

            if (a >= 250) {
                m_framebuffer[frameY * m_width + frameX] = (0xFFu << 24) | (src & 0x00FFFFFF);
            } else {
                uint32_t bg = m_framebuffer[frameY * m_width + frameX];
                uint8_t sr = src & 0xFF;
                uint8_t sg = (src >> 8) & 0xFF;
                uint8_t sb = (src >> 16) & 0xFF;

                uint8_t br = bg & 0xFF;
                uint8_t bgc = (bg >> 8) & 0xFF;
                uint8_t bb = (bg >> 16) & 0xFF;

                float alpha = a / 255.0f;
                uint8_t outR = static_cast<uint8_t>(sr * alpha + br * (1.0f - alpha));
                uint8_t outG = static_cast<uint8_t>(sg * alpha + bgc * (1.0f - alpha));
                uint8_t outB = static_cast<uint8_t>(sb * alpha + bb * (1.0f - alpha));
                m_framebuffer[frameY * m_width + frameX] = (0xFFu << 24) | (outB << 16) | (outG << 8) | outR;
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

void Renderer::getMinimapRect(int& x, int& y, int& w, int& h) const {
    // Panel inferior derecho: minimapa expandido ocupando el ancho liberado por los botones compactos
    const int amarilloX = static_cast<int>(m_width * 0.185f) + static_cast<int>(m_width * 0.63f);
    x = amarilloX + 6;
    y = m_height - 118 + 6;
    w = 198;
    h = 106;
}

void Renderer::getMinimapRect(int& x, int& y, int& size) const {
    int w, h;
    getMinimapRect(x, y, w, h);
    size = std::min(w, h);
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
    (void)amarilloW;

    // Top Bar - Panel de Opciones: botón MANDATOS (ID 470), botón AVANCES (ID 460), botón EVENTOS (ID 450) y botón MENU (ID 400)
    m_buttons.push_back({470, static_cast<float>(m_width) - 455.0f, 5.0f, 105.0f, 22.0f, "MANDATOS", false});
    m_buttons.push_back({460, static_cast<float>(m_width) - 340.0f, 5.0f, 105.0f, 22.0f, "AVANCES", false});
    m_buttons.push_back({450, static_cast<float>(m_width) - 225.0f, 5.0f, 105.0f, 22.0f, "EVENTOS", false});
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

    // Panel central inferior:
    // 1. Controles de simulación JUSTO DEBAJO de la línea de tiempo (ampliados y destacados)
    const float simY = botY + 24.0f;
    const float simH = 28.0f;
    {
        const float totalW = 104.0f + 76.0f * 3 + 10.0f * 3; // 362px
        float cx = static_cast<float>(azulX) + (static_cast<float>(azulW) - totalW) * 0.5f;
        m_buttons.push_back({201, cx, simY, 104.0f, simH, "|| PAUSA", false});
        m_buttons.push_back({202, cx + 114.0f, simY, 76.0f, simH, "> 1x", true});
        m_buttons.push_back({203, cx + 200.0f, simY, 76.0f, simH, ">> 2x", false});
        m_buttons.push_back({204, cx + 286.0f, simY, 76.0f, simH, ">>> 5x", false});
    }

    // 2. Poderes de Dios y Catástrofes Naturales DEBAJO de los controles de simulación:
    // Botones pequeños cuadrados con sprites representativos como los botones de las vistas
    {
        const float powSize = 25.0f;
        const float powY = botY + 54.0f;
        const float gap = 5.0f;
        const float sepGap = 12.0f; // Separador entre Poderes Sagrados y Catástrofes

        const float totalWidth = (4 * powSize + 3 * gap) + sepGap + (6 * powSize + 5 * gap); // 302px
        const float startX = static_cast<float>(azulX) + (static_cast<float>(azulW) - totalWidth) * 0.5f;

        float curX = startX;
        // 4 Poderes Sagrados
        m_buttons.push_back({710, curX, powY, powSize, powSize, "[Z] RAYO", false});
        m_buttons.push_back({701, curX, powY, powSize, powSize, "[Z] RAYO", false});
        curX += powSize + gap;

        m_buttons.push_back({711, curX, powY, powSize, powSize, "[X] PLAGA", false});
        curX += powSize + gap;

        m_buttons.push_back({712, curX, powY, powSize, powSize, "[V] MILAGRO", false});
        m_buttons.push_back({703, curX, powY, powSize, powSize, "[V] MILAGRO", false});
        curX += powSize + gap;

        m_buttons.push_back({713, curX, powY, powSize, powSize, "[C] CONGREGAR", false});
        m_buttons.push_back({702, curX, powY, powSize, powSize, "[C] CONGREGAR", false});
        curX += powSize + sepGap;

        // 6 Catástrofes Naturales
        m_buttons.push_back({720, curX, powY, powSize, powSize, "[T] TORNADO", false});
        curX += powSize + gap;

        m_buttons.push_back({721, curX, powY, powSize, powSize, "[G] TERREMOTO", false});
        curX += powSize + gap;

        m_buttons.push_back({722, curX, powY, powSize, powSize, "[B] PANDEMIA", false});
        curX += powSize + gap;

        m_buttons.push_back({723, curX, powY, powSize, powSize, "[M] METEORITO", false});
        curX += powSize + gap;

        m_buttons.push_back({724, curX, powY, powSize, powSize, "[K] VOLCAN", false});
        curX += powSize + gap;

        m_buttons.push_back({725, curX, powY, powSize, powSize, "[F] INCENDIO", false});
    }

    // Panel del minimapa: botones compactos estilo sprite (sin letras) alineados verticalmente a la derecha
    {
        const float vbW = 22.0f;
        const float vbH = 22.0f;
        const float vbX = static_cast<float>(m_width) - vbW - 6.0f;
        const float vbGap = 2.0f;
        const float vbY = botY + 2.0f;
        m_buttons.push_back({101, vbX, vbY + 0 * (vbH + vbGap), vbW, vbH, "", false}); // VISTA FE
        m_buttons.push_back({102, vbX, vbY + 1 * (vbH + vbGap), vbW, vbH, "", true});  // VISTA BIOMAS / TERRENO (Vista por defecto sin nodos de fe)
        m_buttons.push_back({105, vbX, vbY + 2 * (vbH + vbGap), vbW, vbH, "", false}); // VISTA UNIDADES
        m_buttons.push_back({103, vbX, vbY + 3 * (vbH + vbGap), vbW, vbH, "", false}); // VISTA SINCRETISMO
        m_buttons.push_back({104, vbX, vbY + 4 * (vbH + vbGap), vbW, vbH, "", false}); // VISTA ERAS
    }

    // Panel de Opciones de Unidad (Izquierda inferior) - Botones LIDER (ID 700) y RECUERDOS (ID 750)
    {
        const int verdeW = static_cast<int>(m_width * 0.185f);
        const float unitBtnY = botY + 86.0f;
        const float unitBtnH = 24.0f;
        const float halfBtnW = static_cast<float>((verdeW - 20) / 2);
        m_buttons.push_back({700, 8.0f, unitBtnY, halfBtnW, unitBtnH, "LIDER", false});
        m_buttons.push_back({750, 8.0f + halfBtnW + 4.0f, unitBtnY, halfBtnW, unitBtnH, "RECUERDOS", false});
    }

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
    const float modalW = 920.0f;
    const float modalH = 584.0f;
    const float modalX = (m_width - modalW) * 0.5f;
    const float modalY = (m_height - modalH) * 0.5f;

    const float col1X = modalX + 45.0f;
    const float col2X = modalX + modalW * 0.5f + 15.0f;
    const float fieldW = 390.0f;
    const float fieldH = 24.0f;
    const float startFieldY = modalY + 60.0f;
    const float stepY = 38.0f;

    // Columna Izquierda: Identidad, Geografía y Génesis (9 filas perfectamente espaciadas)
    // ID 501: Dios (Input text)
    m_creationButtons.push_back({501, col1X, startFieldY + 0.0f * stepY + 9.0f, fieldW, fieldH, "", false});
    // ID 502: Religión (Input text)
    m_creationButtons.push_back({502, col1X, startFieldY + 1.0f * stepY + 9.0f, fieldW, fieldH, "", false});
    // ID 503 & 504: Semilla (Input / Randomize)
    m_creationButtons.push_back({503, col1X, startFieldY + 2.0f * stepY + 9.0f, fieldW - 65.0f, fieldH, "", false});
    m_creationButtons.push_back({504, col1X + fieldW - 60.0f, startFieldY + 2.0f * stepY + 9.0f, 60.0f, fieldH, "DICE", false});
    // ID 505: Población inicial (Toggle/Cycle)
    m_creationButtons.push_back({505, col1X, startFieldY + 3.0f * stepY + 9.0f, fieldW, fieldH, "", false});
    // ID 516: Recursos iniciales (Caja de texto interactiva y ciclo)
    m_creationButtons.push_back({516, col1X, startFieldY + 4.0f * stepY + 9.0f, fieldW, fieldH, "", false});
    // ID 506: Época de inicio (Toggle/Cycle)
    m_creationButtons.push_back({506, col1X, startFieldY + 5.0f * stepY + 9.0f, fieldW, fieldH, "", false});
    // ID 507: Tamaño del Mapa (Toggle/Cycle)
    m_creationButtons.push_back({507, col1X, startFieldY + 6.0f * stepY + 9.0f, fieldW, fieldH, "", false});
    // ID 520: Clima Primordial del Mundo (Toggle/Cycle)
    m_creationButtons.push_back({520, col1X, startFieldY + 7.0f * stepY + 9.0f, fieldW, fieldH, "", false});
    // ID 517: Abundancia de Recursos Naturales (Toggle/Cycle)
    m_creationButtons.push_back({517, col1X, startFieldY + 8.0f * stepY + 9.0f, fieldW, fieldH, "", false});

    // Columna Derecha: Dogma, Geopolítica e Inteligencia Artificial (9 filas perfectamente espaciadas)
    // ID 509: Tipo de partida (Estandar, Sandbox, Teocracia, Supervivencia)
    m_creationButtons.push_back({509, col2X, startFieldY + 0.0f * stepY + 9.0f, fieldW, fieldH, "", false});
    // ID 511: Dificultad de la IA (Pasiva, Equilibrada, Estratégica, Implacable)
    m_creationButtons.push_back({511, col2X, startFieldY + 1.0f * stepY + 9.0f, fieldW, fieldH, "", false});
    // ID 521: Ritmo Histórico / Escala Temporal (Épico, Clásico, Dinámico)
    m_creationButtons.push_back({521, col2X, startFieldY + 2.0f * stepY + 9.0f, fieldW, fieldH, "", false});
    // ID 522: Beligerancia Tribal / Fricción Liminal (Pacífica, Equilibrada, Hostil)
    m_creationButtons.push_back({522, col2X, startFieldY + 3.0f * stepY + 9.0f, fieldW, fieldH, "", false});
    // ID 508: Desastres / Plagas (Bajas, Normales, Frecuentes)
    m_creationButtons.push_back({508, col2X, startFieldY + 4.0f * stepY + 9.0f, fieldW, fieldH, "", false});
    // ID 512: Color de Influencia / Creyentes (Paleta mística)
    m_creationButtons.push_back({512, col2X, startFieldY + 5.0f * stepY + 9.0f, fieldW, fieldH, "", false});
    // ID 513: Tipo de Victoria (Hegemonía, Teocrática, Trascendencia, Tiempo, Sin Límite)
    m_creationButtons.push_back({513, col2X, startFieldY + 6.0f * stepY + 9.0f, fieldW, fieldH, "", false});
    // ID 510: Cofre de inicio (ON / OFF)
    m_creationButtons.push_back({510, col2X, startFieldY + 7.0f * stepY + 9.0f, fieldW, fieldH, "", false});
    // ID 514 & 515: Evolución Doctrinal (Cismas espontáneos y Sincretismo)
    const float halfFieldW = (fieldW - 10.0f) * 0.5f;
    m_creationButtons.push_back({514, col2X, startFieldY + 8.0f * stepY + 9.0f, halfFieldW, fieldH, "", false});
    m_creationButtons.push_back({515, col2X + halfFieldW + 10.0f, startFieldY + 8.0f * stepY + 9.0f, halfFieldW, fieldH, "", false});

    // Botones de acción inferiores
    const float actBtnW = 200.0f;
    const float actBtnH = 36.0f;
    // ID 550: CANCELAR / VOLVER
    m_creationButtons.push_back({550, modalX + 45.0f, modalY + 508.0f, actBtnW, actBtnH, "CANCELAR", false});
    // ID 551: INICIAR CREACION / GÉNESIS
    m_creationButtons.push_back({551, modalX + modalW - actBtnW - 45.0f, modalY + 508.0f, actBtnW, actBtnH, "COMENZAR GENESIS", false});
}

void Renderer::resize(int newWidth, int newHeight) {
    if (newWidth <= 100 || newHeight <= 100) return;
    if (newWidth == m_width && newHeight == m_height) return;

    m_width = newWidth;
    m_height = newHeight;
    m_framebuffer.resize(m_width * m_height, 0xFF1A1210);
    initStars();

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

int Renderer::checkGameOverButtonClicked(float mouseX, float mouseY) const {
    const float modalW = 860.0f;
    const float modalH = 480.0f;
    const float mx = (m_width - modalW) * 0.5f;
    const float my = (m_height - modalH) * 0.5f;

    const float btnY = my + modalH - 65.0f;
    const float btnH = 44.0f;
    const float btnW = 240.0f;

    // ID 901: REINTENTAR GÉNESIS
    if (mouseX >= mx + 35.0f && mouseX <= mx + 35.0f + btnW &&
        mouseY >= btnY && mouseY <= btnY + btnH) {
        return 901;
    }
    // ID 902: CARGAR PARTIDA
    if (mouseX >= mx + 310.0f && mouseX <= mx + 310.0f + btnW &&
        mouseY >= btnY && mouseY <= btnY + btnH) {
        return 902;
    }
    // ID 903: MENÚ PRINCIPAL
    if (mouseX >= mx + 585.0f && mouseX <= mx + 585.0f + btnW &&
        mouseY >= btnY && mouseY <= btnY + btnH) {
        return 903;
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

void Renderer::update(float deltaTime) {
    m_animTime += deltaTime;
    if (m_pressedAnimTimer > 0.0f) {
        m_pressedAnimTimer -= deltaTime;
        if (m_pressedAnimTimer <= 0.0f) {
            m_pressedAnimTimer = 0.0f;
            if (!m_isMouseDown) {
                m_pressedButtonId = 0;
            }
        }
    }
}

void Renderer::setMousePosition(float mouseX, float mouseY) {
    m_mouseX = mouseX;
    m_mouseY = mouseY;
}

void Renderer::setMouseDown(bool isDown, float mouseX, float mouseY) {
    m_isMouseDown = isDown;
    m_mouseX = mouseX;
    m_mouseY = mouseY;
    if (!isDown && m_pressedAnimTimer <= 0.0f) {
        m_pressedButtonId = 0;
    }
}

void Renderer::triggerButtonPress(int buttonId) {
    m_pressedButtonId = buttonId;
    m_pressedAnimTimer = 0.14f; // 140ms de animación de pulsado táctil
}

void Renderer::drawAntiqueButton(int bx, int by, int bw, int bh,
                                 const std::string& label,
                                 bool active, bool hovered, bool pressed, int btnId) {
    if (m_buttonAtlasPixels.size() != 256 * 128) {
        // Fallback clásico procedural si no está el atlas
        int ty = by + (bh - 7) / 2;
        int tx = bx + (bw - static_cast<int>(label.size() * 6)) / 2;
        if (active) {
            drawRect(bx, by, bw, bh, ColorRGB{52, 32, 24});
            drawRectOutline(bx, by, bw, bh, Palette::DIVINE_GOLD);
            drawText(tx, ty, label, ColorRGB{255, 235, 120}, 1);
        } else {
            drawRect(bx, by, bw, bh, ColorRGB{32, 20, 18});
            drawRectOutline(bx, by, bw, bh, ColorRGB{95, 68, 48});
            drawText(tx, ty, label, Palette::RELIC_BONE, 1);
        }
        return;
    }

    // Determinar fila en el atlas:
    // Fila 0 (y=4):   IDLE
    // Fila 1 (y=36):  HOVER
    // Fila 2 (y=68):  PRESSED
    // Fila 3 (y=100): ACTIVE / TOGGLED
    int rowY = 4;
    int textOffset = 0;
    ColorRGB textColor = Palette::RELIC_BONE;

    if (pressed) {
        rowY = 68;
        textOffset = 1; // Hundimiento físico táctil de 1px
        textColor = ColorRGB{200, 190, 170};
    } else if (active) {
        rowY = 100;
        textOffset = 1; // En bajorrelieve sostenido
        textColor = ColorRGB{255, 235, 120};
    } else if (hovered) {
        rowY = 36;
        textOffset = 0;
        textColor = ColorRGB{255, 245, 180}; // Iluminación dorada
    }

    // Determinar región origen en el atlas de botones
    int srcX = 2;
    bool isExactSprite = false;

    if (btnId == 201 && bw == 44 && bh == 24) { // PAUSA (44x24 exact sprite)
        srcX = 54;
        isExactSprite = true;
    } else if (btnId == 202 && bw == 34 && bh == 24) { // 1x (34x24 exact sprite)
        srcX = 106;
        isExactSprite = true;
    } else if (btnId == 203 && bw == 34 && bh == 24) { // 2x (34x24 exact sprite)
        srcX = 144;
        isExactSprite = true;
    } else if (btnId == 204 && bw == 34 && bh == 24) { // 5x (34x24 exact sprite)
        srcX = 182;
        isExactSprite = true;
    }

    // Renderizar sprite pixel a pixel con 9-slice o mapeo 1:1
    for (int dy = 0; dy < bh; ++dy) {
        int dstY = by + dy;
        int frameY = m_height - 1 - dstY;
        if (frameY < 0 || frameY >= m_height) continue;

        int sy = 0;
        if (isExactSprite) {
            sy = dy;
        } else {
            // Mapeo 9-slice vertical con borde de 6px
            if (dy < 6) sy = dy;
            else if (dy >= bh - 6) sy = 24 - (bh - dy);
            else sy = 6 + ((dy - 6) * 12) / std::max(1, bh - 12);
        }

        int atlasY = rowY + sy;
        if (atlasY < 0 || atlasY >= 128) continue;

        for (int dx = 0; dx < bw; ++dx) {
            int dstX = bx + dx;
            if (dstX < 0 || dstX >= m_width) continue;

            int sx = 0;
            if (isExactSprite) {
                sx = dx;
            } else {
                // Mapeo 9-slice horizontal con borde de 6px
                if (dx < 6) sx = dx;
                else if (dx >= bw - 6) sx = 48 - (bw - dx);
                else sx = 6 + ((dx - 6) * 36) / std::max(1, bw - 12);
            }

            int atlasX = srcX + sx;
            if (atlasX < 0 || atlasX >= 256) continue;

            uint32_t src = m_buttonAtlasPixels[atlasY * 256 + atlasX];
            uint8_t a = (src >> 24) & 0xFF;
            if (a < 15) continue;

            if (a >= 250) {
                m_framebuffer[frameY * m_width + dstX] = (0xFFu << 24) | (src & 0x00FFFFFF);
            } else {
                uint32_t bg = m_framebuffer[frameY * m_width + dstX];
                uint8_t sr = src & 0xFF;
                uint8_t sg = (src >> 8) & 0xFF;
                uint8_t sb = (src >> 16) & 0xFF;

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

    // Halo ambiental dorado en hover (brillo tenue superior e izquierdo)
    if (hovered && !pressed) {
        drawLine(bx + 1, by + 1, bx + bw - 2, by + 1, ColorRGB{255, 235, 140});
        drawLine(bx + 1, by + 1, bx + 1, by + bh - 2, ColorRGB{255, 235, 140});
    }

    // Dibujar texto grabado esculpido con relieve táctil (solo para botones sin glifo integrado en sprite exacto)
    if (!label.empty() && !isExactSprite) {
        int ty = by + (bh - 12) / 2 + textOffset;
        int tx = bx + (bw - static_cast<int>(label.size() * 6)) / 2 + textOffset;

        // Sombra grabada en bajorrelieve
        if (pressed) {
            drawText(tx, ty - 1, label, ColorRGB{20, 14, 10}, 1);
        } else {
            drawText(tx, ty + 1, label, ColorRGB{20, 14, 10}, 1);
        }
        // Runa frontal
        drawText(tx, ty, label, textColor, 1);
    }
}

void Renderer::drawViewModeButton(int bx, int by, int bw, int bh, int btnId,
                                  const std::string& label,
                                  bool active, bool hovered, bool pressed) {
    ColorRGB baseColor;
    ColorRGB rimColor;
    if (active) {
        baseColor = ColorRGB{64, 46, 18};
        rimColor = Palette::DIVINE_GOLD;
    } else if (hovered) {
        baseColor = ColorRGB{46, 32, 22};
        rimColor = ColorRGB{255, 215, 104};
    } else if (pressed) {
        baseColor = ColorRGB{22, 14, 10};
        rimColor = ColorRGB{95, 68, 48};
    } else {
        baseColor = ColorRGB{28, 18, 14};
        rimColor = ColorRGB{85, 60, 42};
    }

    // Panel con bisel volumétrico satinado estilo Sims 2
    drawSims2Panel(bx, by, bw, bh, baseColor, active || pressed, rimColor);

    // Brillo sutil de resalte en hover
    if (hovered && !pressed) {
        drawLine(bx + 1, by + 1, bx + bw - 2, by + 1, ColorRGB{255, 235, 140});
        drawLine(bx + 1, by + 1, bx + 1, by + bh - 2, ColorRGB{255, 235, 140});
    }

    // Columna del sprite según el modo de vista
    int iconCol = 0;
    switch (btnId) {
        case 101: iconCol = 0; break; // FE (Sol radiante / Mónada sagrada)
        case 102: iconCol = 1; break; // BIOMAS (Montaña nevada y bosque verde)
        case 105: iconCol = 2; break; // UNIDADES (Chamán con báculo y cazador)
        case 103: iconCol = 3; break; // SINCRETISMO (Espirales sagradas oro y amatista)
        case 104: iconCol = 4; break; // ERAS (Reloj de arena ceremonial)
        default:  iconCol = 0; break;
    }

    // Fila del sprite según el estado interactivo
    int iconRow = 0; // 0: Idle, 1: Hover, 2: Pressed, 3: Active
    if (pressed) {
        iconRow = 2;
    } else if (active) {
        iconRow = 3;
    } else if (hovered) {
        iconRow = 1;
    }

    int srcIconX = iconCol * 16;
    int srcIconY = iconRow * 16;

    // Desplazamiento táctil (depresión física al presionar o estar activo)
    int shift = (pressed || active) ? 1 : 0;
    int dstIconX = bx + (bw - 16) / 2 + shift;
    int dstIconY = by + (bh - 16) / 2 + shift;

    // Renderizar sprite de icono de 16x16
    if (m_viewIconPixels.size() == 80 * 64) {
        for (int iy = 0; iy < 16; ++iy) {
            int frameY = m_height - 1 - (dstIconY + iy);
            if (frameY < 0 || frameY >= m_height) continue;

            for (int ix = 0; ix < 16; ++ix) {
                int frameX = dstIconX + ix;
                if (frameX < 0 || frameX >= m_width) continue;

                uint32_t src = m_viewIconPixels[(srcIconY + iy) * 80 + (srcIconX + ix)];
                uint8_t a = (src >> 24) & 0xFF;
                if (a < 15) continue;

                if (a >= 250) {
                    m_framebuffer[frameY * m_width + frameX] = (0xFFu << 24) | (src & 0x00FFFFFF);
                } else {
                    uint32_t bg = m_framebuffer[frameY * m_width + frameX];
                    uint8_t sr = src & 0xFF;
                    uint8_t sg = (src >> 8) & 0xFF;
                    uint8_t sb = (src >> 16) & 0xFF;

                    uint8_t br = bg & 0xFF;
                    uint8_t bgc = (bg >> 8) & 0xFF;
                    uint8_t bb = (bg >> 16) & 0xFF;

                    float alpha = a / 255.0f;
                    uint8_t outR = static_cast<uint8_t>(sr * alpha + br * (1.0f - alpha));
                    uint8_t outG = static_cast<uint8_t>(sg * alpha + bgc * (1.0f - alpha));
                    uint8_t outB = static_cast<uint8_t>(sb * alpha + bb * (1.0f - alpha));
                    m_framebuffer[frameY * m_width + frameX] = (0xFFu << 24) | (outB << 16) | (outG << 8) | outR;
                }
            }
        }
    }

    // Texto grabado litúrgico (solo para botones con ancho suficiente para texto)
    if (!label.empty() && bw > 30) {
        int tx = bx + 23 + shift;
        int ty = by + (bh - 7) / 2 + shift;

        ColorRGB textColor = Palette::RELIC_BONE;
        if (active) textColor = ColorRGB{255, 235, 120};
        else if (hovered) textColor = ColorRGB{255, 245, 180};
        else if (pressed) textColor = ColorRGB{190, 180, 160};

        if (pressed) {
            drawText(tx, ty - 1, label, ColorRGB{16, 10, 8}, 1);
        } else {
            drawText(tx, ty + 1, label, ColorRGB{16, 10, 8}, 1);
        }
        drawText(tx, ty, label, textColor, 1);
    }

    // Gema / indicador iluminado activo para botones anchos
    if (active && bw > 30) {
        int pipX = bx + bw - 8;
        int pipY = by + (bh - 6) / 2 + shift;
        drawSims2Bar(pipX, pipY, 5, 6, Palette::DIVINE_GOLD);
    }
}

void Renderer::drawPowerButton(int bx, int by, int bw, int bh, int btnId,
                               const std::string& label,
                               bool active, bool hovered, bool pressed) {
    (void)label;
    ColorRGB baseColor;
    ColorRGB rimColor;
    if (active) {
        baseColor = ColorRGB{64, 46, 18};
        rimColor = Palette::DIVINE_GOLD;
    } else if (hovered) {
        baseColor = ColorRGB{46, 32, 22};
        rimColor = ColorRGB{255, 215, 104};
    } else if (pressed) {
        baseColor = ColorRGB{22, 14, 10};
        rimColor = ColorRGB{95, 68, 48};
    } else {
        baseColor = ColorRGB{28, 18, 14};
        rimColor = ColorRGB{85, 60, 42};
    }

    // Panel con bisel volumétrico satinado estilo Sims 2
    drawSims2Panel(bx, by, bw, bh, baseColor, active || pressed, rimColor);

    // Brillo sutil de resalte en hover
    if (hovered && !pressed) {
        drawLine(bx + 1, by + 1, bx + bw - 2, by + 1, ColorRGB{255, 235, 140});
        drawLine(bx + 1, by + 1, bx + 1, by + bh - 2, ColorRGB{255, 235, 140});
    }

    // Columna del sprite según el poder (16x16 por icono, 9 columnas):
    // 0: Rayo (710 o 701)
    // 1: Plaga (711)
    // 2: Milagro (712 o 703)
    // 3: Congregar (713 o 702)
    // 4: Tornado (720)
    // 5: Terremoto (721)
    // 6: Pandemia (722)
    // 7: Meteorito (723)
    // 8: Volcán (724)
    // 9: Incendio Forestal (725)
    int iconCol = 0;
    switch (btnId) {
        case 710: case 701: iconCol = 0; break;
        case 711:           iconCol = 1; break;
        case 712: case 703: iconCol = 2; break;
        case 713: case 702: iconCol = 3; break;
        case 720:           iconCol = 4; break;
        case 721:           iconCol = 5; break;
        case 722:           iconCol = 6; break;
        case 723:           iconCol = 7; break;
        case 724:           iconCol = 8; break;
        case 725:           iconCol = 9; break;
        default:            iconCol = 0; break;
    }

    // Fila del sprite según el estado interactivo
    int iconRow = 0; // 0: Idle, 1: Hover, 2: Pressed, 3: Active
    if (pressed) {
        iconRow = 2;
    } else if (active) {
        iconRow = 3;
    } else if (hovered) {
        iconRow = 1;
    }

    int srcIconX = iconCol * 16;
    int srcIconY = iconRow * 16;

    // Desplazamiento táctil (depresión física al presionar o estar activo)
    int shift = (pressed || active) ? 1 : 0;
    int dstIconX = bx + (bw - 16) / 2 + shift;
    int dstIconY = by + (bh - 16) / 2 + shift;

    // Renderizar sprite de icono de 16x16
    if (!m_powerIconPixels.empty() && m_powerIconW > 0 && m_powerIconH == 64) {
        for (int iy = 0; iy < 16; ++iy) {
            int frameY = m_height - 1 - (dstIconY + iy);
            if (frameY < 0 || frameY >= m_height) continue;

            for (int ix = 0; ix < 16; ++ix) {
                int frameX = dstIconX + ix;
                if (frameX < 0 || frameX >= m_width) continue;

                uint32_t src = m_powerIconPixels[(srcIconY + iy) * m_powerIconW + (srcIconX + ix)];
                uint8_t a = (src >> 24) & 0xFF;
                if (a < 15) continue;

                if (a >= 250) {
                    m_framebuffer[frameY * m_width + frameX] = (0xFFu << 24) | (src & 0x00FFFFFF);
                } else {
                    uint32_t bg = m_framebuffer[frameY * m_width + frameX];
                    uint8_t sr = src & 0xFF;
                    uint8_t sg = (src >> 8) & 0xFF;
                    uint8_t sb = (src >> 16) & 0xFF;

                    uint8_t br = bg & 0xFF;
                    uint8_t bgc = (bg >> 8) & 0xFF;
                    uint8_t bb = (bg >> 16) & 0xFF;

                    float alpha = a / 255.0f;
                    uint8_t outR = static_cast<uint8_t>(sr * alpha + br * (1.0f - alpha));
                    uint8_t outG = static_cast<uint8_t>(sg * alpha + bgc * (1.0f - alpha));
                    uint8_t outB = static_cast<uint8_t>(sb * alpha + bb * (1.0f - alpha));
                    m_framebuffer[frameY * m_width + frameX] = (0xFFu << 24) | (outB << 16) | (outG << 8) | outR;
                }
            }
        }
    } else {
        // Fallback procedural de iniciales si el atlas no estuviera cargado
        std::string initial = "P";
        switch (btnId) {
            case 710: case 701: initial = "Z"; break;
            case 711:           initial = "X"; break;
            case 712: case 703: initial = "V"; break;
            case 713: case 702: initial = "C"; break;
            case 720:           initial = "T"; break;
            case 721:           initial = "G"; break;
            case 722:           initial = "B"; break;
            case 723:           initial = "M"; break;
            case 724:           initial = "K"; break;
            case 725:           initial = "F"; break;
        }
        drawText(dstIconX + 4, dstIconY + 4, initial, rimColor, 1);
    }

    // Marco o gema divina si está activo
    if (active) {
        drawRectOutline(bx, by, bw, bh, Palette::DIVINE_GOLD);
        drawRectOutline(bx + 1, by + 1, bw - 2, bh - 2, ColorRGB{255, 235, 120});
    }
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
    GLint att[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, 0 };
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
    if (blankPixmap != 0) {
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
            glXMakeCurrent(dpy, 0, nullptr);
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
    m_skyDrawnThisFrame = false;
    if (m_dayNight) {
        drawCelestialSky(*m_dayNight);
        m_skyDrawnThisFrame = true;
    } else {
        clear(ColorRGB(14, 18, 28)); // Deep cosmic obsidian
        drawSpaceBackground();
    }
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

void Renderer::initStars() {
    m_stars.clear();
    m_stars.reserve(480);
    uint32_t seed = 987654321u;
    auto lcg = [&seed]() {
        seed = seed * 1664525u + 1013904223u;
        return seed;
    };

    // Coordenadas cósmicas virtuales más amplias que la pantalla para envolver con suavidad y parallax
    int cosmicW = std::max(3840, m_width * 3);
    int cosmicH = std::max(2160, m_height * 3);

    for (int i = 0; i < 480; ++i) {
        CelestialStar s;
        s.x = static_cast<float>(lcg() % static_cast<uint32_t>(cosmicW));
        s.y = static_cast<float>(lcg() % static_cast<uint32_t>(cosmicH));
        s.parallax = 0.035f + (static_cast<float>(lcg() % 100) / 100.0f) * 0.085f; // Parallax 0.035 a 0.12
        s.brightness = 0.55f + (static_cast<float>(lcg() % 100) / 220.0f);
        s.twinkleSpeed = 1.4f + (static_cast<float>(lcg() % 100) / 32.0f);
        s.twinkleSeed = static_cast<float>(lcg() % 1000) / 100.0f;

        // Clasificación espectral estelar
        uint32_t spec = lcg() % 100;
        if (spec < 56) {
            // Clase A/F: Blanco diamante puro / hielo cósmico
            s.r = static_cast<uint8_t>(225 + (lcg() % 30));
            s.g = static_cast<uint8_t>(235 + (lcg() % 20));
            s.b = 255;
            s.size = (spec < 7) ? 2 : 1;
        } else if (spec < 76) {
            // Clase G: Oro sagrado divino / ámbar cálido
            s.r = 255;
            s.g = static_cast<uint8_t>(210 + (lcg() % 35));
            s.b = static_cast<uint8_t>(110 + (lcg() % 45));
            s.size = (spec < 62) ? 2 : 1;
        } else if (spec < 90) {
            // Clase B: Azul zafiro profundo
            s.r = static_cast<uint8_t>(130 + (lcg() % 40));
            s.g = static_cast<uint8_t>(175 + (lcg() % 40));
            s.b = 255;
            s.size = 1;
        } else {
            // Clase M: Ascua rubí estelar
            s.r = 255;
            s.g = static_cast<uint8_t>(140 + (lcg() % 40));
            s.b = static_cast<uint8_t>(130 + (lcg() % 30));
            s.size = (spec > 97) ? 2 : 1;
        }
        m_stars.push_back(s);
    }
}

void Renderer::initCosmicBuffer() {
    m_cosmicBuffer.resize(COSMIC_BUFFER_WIDTH * COSMIC_BUFFER_HEIGHT);

    for (int y = 0; y < COSMIC_BUFFER_HEIGHT; ++y) {
        float vy = static_cast<float>(y);
        for (int x = 0; x < COSMIC_BUFFER_WIDTH; ++x) {
            float vx = static_cast<float>(x);

            float rSpace = 4.0f;
            float gSpace = 5.0f;
            float bSpace = 12.0f;

            float n1 = organicTurbulence(vx * 0.00118f, vy * 0.00118f);
            float n2 = organicTurbulence((vx + 620.0f) * 0.00277f, (vy + 340.0f) * 0.00277f);
            float n3 = organicTurbulence((vx - 390.0f) * 0.00625f, (vy + 780.0f) * 0.00625f);

            if (n1 > 0.08f) {
                float t1 = std::clamp((n1 - 0.08f) / 0.48f, 0.0f, 1.0f);
                t1 = t1 * t1;
                rSpace += 36.0f * t1;
                gSpace += 10.0f * t1;
                bSpace += 54.0f * t1;
            }
            if (n2 > 0.10f) {
                float t2 = std::clamp((n2 - 0.10f) / 0.44f, 0.0f, 1.0f);
                t2 = t2 * t2;
                rSpace += 8.0f * t2;
                gSpace += 28.0f * t2;
                bSpace += 50.0f * t2;
            }
            if (n3 > 0.14f) {
                float t3 = std::clamp((n3 - 0.14f) / 0.38f, 0.0f, 1.0f);
                t3 = t3 * t3;
                rSpace += 26.0f * t3;
                gSpace += 6.0f * t3;
                bSpace += 22.0f * t3;
            }

            uint8_t fr = static_cast<uint8_t>(std::clamp(rSpace, 0.0f, 255.0f));
            uint8_t fg = static_cast<uint8_t>(std::clamp(gSpace, 0.0f, 255.0f));
            uint8_t fb = static_cast<uint8_t>(std::clamp(bSpace, 0.0f, 255.0f));

            m_cosmicBuffer[y * COSMIC_BUFFER_WIDTH + x] =
                (0x00u << 24) | (static_cast<uint32_t>(fb) << 16) |
                (static_cast<uint32_t>(fg) << 8) | static_cast<uint32_t>(fr);
        }
    }
}

void Renderer::initTerrainTables() {
    for (int wy = 0; wy < 64; ++wy) {
        for (int wx = 0; wx < 64; ++wx) {
            float gx = static_cast<float>(wx) / 64.0f;
            float gy = static_cast<float>(wy) / 64.0f;
            m_turbTableX[wy][wx] = organicTurbulence(gx, gy) * 0.22f;
            m_turbTableY[wy][wx] = organicTurbulence(gy + 13.5f, gx + 27.8f) * 0.22f;
            uint32_t mHash = hash2D(static_cast<int>(gx * 128.0f), static_cast<int>(gy * 128.0f));
            m_ditherTable[wy][wx] = ((static_cast<float>(mHash & 0xFFu) / 255.0f) - 0.5f) * 0.14f;
        }
    }
}

void Renderer::drawCelestialSky(const DayNightCycle& dayNight) {
    float timer = dayNight.getCycleTimer();

    float paraX = m_cameraX * 0.08f;
    float paraY = m_cameraY * 0.08f;

    // 1. Renderizado ultrarrápido del espacio exterior continuo mediante búfer cósmico precomputado
    if (!m_cosmicBuffer.empty()) {
        int baseOffsetX = static_cast<int>(std::floor(-paraX + timer * 1.2f));
        int baseOffsetY = static_cast<int>(std::floor(-paraY));

        for (int y = 0; y < m_height; ++y) {
            int frameY = m_height - 1 - y;
            uint32_t* rowPtr = &m_framebuffer[frameY * m_width];
            int srcY = (y + baseOffsetY) & (COSMIC_BUFFER_HEIGHT - 1);
            if (srcY < 0) srcY += COSMIC_BUFFER_HEIGHT;
            const uint32_t* cosmicRow = &m_cosmicBuffer[srcY * COSMIC_BUFFER_WIDTH];

            int srcX = baseOffsetX & (COSMIC_BUFFER_WIDTH - 1);
            if (srcX < 0) srcX += COSMIC_BUFFER_WIDTH;

            for (int x = 0; x < m_width; ++x) {
                rowPtr[x] = cosmicRow[(srcX + x) & (COSMIC_BUFFER_WIDTH - 1)];
            }
        }
    } else {
        clear(ColorRGB(4, 5, 12));
    }

    // 2. Estrellas cósmicas en todo el firmamento espacial
    for (const auto& star : m_stars) {
        float shiftedX = star.x - m_cameraX * star.parallax;
        float shiftedY = star.y - m_cameraY * star.parallax;
        float sxFloat = std::fmod(std::fmod(shiftedX, static_cast<float>(m_width)) + static_cast<float>(m_width), static_cast<float>(m_width));
        float syFloat = std::fmod(std::fmod(shiftedY, static_cast<float>(m_height)) + static_cast<float>(m_height), static_cast<float>(m_height));
        int sx = static_cast<int>(sxFloat);
        int sy = static_cast<int>(syFloat);
        if (sx < 0 || sx >= m_width || sy < 0 || sy >= m_height) continue;

        float twinkle = (0.55f + 0.45f * std::sin(timer * star.twinkleSpeed + star.twinkleSeed)) * star.brightness;
        uint8_t sr = static_cast<uint8_t>(std::clamp(static_cast<float>(star.r) * twinkle, 0.0f, 255.0f));
        uint8_t sg = static_cast<uint8_t>(std::clamp(static_cast<float>(star.g) * twinkle, 0.0f, 255.0f));
        uint8_t sb = static_cast<uint8_t>(std::clamp(static_cast<float>(star.b) * twinkle, 0.0f, 255.0f));

        int frameY = m_height - 1 - sy;
        uint32_t bg = m_framebuffer[frameY * m_width + sx];
        uint8_t outR = std::min(255, static_cast<int>((bg & 0xFF) + sr));
        uint8_t outG = std::min(255, static_cast<int>(((bg >> 8) & 0xFF) + sg));
        uint8_t outB = std::min(255, static_cast<int>(((bg >> 16) & 0xFF) + sb));
        m_framebuffer[frameY * m_width + sx] = (0x00u << 24) | (outB << 16) | (outG << 8) | outR;

        // Tamaño 2: brazos sutiles en cruz / diamante
        if (star.size > 1 && twinkle > 0.60f) {
            uint8_t hsr = sr / 3;
            uint8_t hsg = sg / 3;
            uint8_t hsb = sb / 3;
            const int nbs[4][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
            for (auto& nb : nbs) {
                int nx = sx + nb[0];
                int ny = sy + nb[1];
                if (nx < 0 || nx >= m_width || ny < 0 || ny >= m_height) continue;
                int nFrameY = m_height - 1 - ny;
                uint32_t nbg = m_framebuffer[nFrameY * m_width + nx];
                uint8_t nR = std::min(255, static_cast<int>((nbg & 0xFF) + hsr));
                uint8_t nG = std::min(255, static_cast<int>(((nbg >> 8) & 0xFF) + hsg));
                uint8_t nB = std::min(255, static_cast<int>(((nbg >> 16) & 0xFF) + hsb));
                m_framebuffer[nFrameY * m_width + nx] = (0x00u << 24) | (nB << 16) | (nG << 8) | nR;
            }
        }
    }

    // 3. Cometas cósmicos / estrellas fugaces en el espacio profundo
    for (int cometIdx = 0; cometIdx < 2; ++cometIdx) {
        float period = (cometIdx == 0) ? 8.0f : 12.5f;
        float cTime = std::fmod(timer + static_cast<float>(cometIdx) * 4.2f, period);
        if (cTime < 1.35f) {
            float progress = cTime / 1.35f;
            float startX = (cometIdx == 0) ? static_cast<float>(m_width) * 0.88f : static_cast<float>(m_width) * 0.20f;
            float startY = (cometIdx == 0) ? 70.0f : 130.0f;
            float dirX = (cometIdx == 0) ? -320.0f : 280.0f;
            float dirY = 170.0f;

            float cx = startX + dirX * progress - paraX * 0.5f;
            float cy = startY + dirY * progress - paraY * 0.5f;
            float cometFade = std::sin(progress * 3.14159f);

            for (int seg = 0; seg < 15; ++seg) {
                float segT = static_cast<float>(seg) / 15.0f;
                float segX = cx - (dirX * 0.10f) * segT;
                float segY = cy - (dirY * 0.10f) * segT;
                int isx = static_cast<int>(segX);
                int isy = static_cast<int>(segY);
                if (isx < 0 || isx >= m_width || isy < 0 || isy >= m_height) continue;

                float segBright = (1.0f - segT) * cometFade * 240.0f;
                int frameY = m_height - 1 - isy;
                uint32_t bg = m_framebuffer[frameY * m_width + isx];
                uint8_t outR = std::min(255, static_cast<int>((bg & 0xFF) + segBright));
                uint8_t outG = std::min(255, static_cast<int>(((bg >> 8) & 0xFF) + segBright * 0.92f));
                uint8_t outB = std::min(255, static_cast<int>(((bg >> 16) & 0xFF) + segBright * 0.72f));
                m_framebuffer[frameY * m_width + isx] = (0x00u << 24) | (outB << 16) | (outG << 8) | outR;
            }
        }
    }
}

void Renderer::renderCelestialBodies(const DayNightCycle& dayNight, const IsometricCamera& camera, int mapWidth, int mapHeight) {
    float curZoom = camera.getZoom();
    float celestialZoomAlpha = 0.0f;
    if (curZoom <= 0.85f) {
        celestialZoomAlpha = 1.0f;
    } else if (curZoom < 1.10f) {
        float t = (1.10f - curZoom) / (1.10f - 0.85f);
        celestialZoomAlpha = t * t * (3.0f - 2.0f * t);
    } else {
        return; // Zoom In: 100% invisible para no obstaculizar la vista de aldeanos ni mapa
    }

    if (celestialZoomAlpha <= 0.005f) return;

    float timer = dayNight.getCycleTimer();

    // 1. Sol diurno referenciado estrictamente al mapa mundial (en la alta bóveda celeste por encima de las nubes)
    float sunWx = 0.0f, sunWy = 0.0f, sunAlt = 0.0f, sAlpha = 0.0f;
    if (dayNight.getSunWorldPosition(mapWidth, mapHeight, sunWx, sunWy, sunAlt, sAlpha)) {
        Vec2 screenPt = camera.gridToScreen(sunWx, sunWy, sunAlt);
        screenPt.y = std::max(48.0f, screenPt.y);
        drawSun(screenPt.x, screenPt.y, sAlpha * celestialZoomAlpha, timer, false);
    }

    // 2. Luna nocturna referenciada estrictamente al mapa mundial (en la alta bóveda celeste por encima de las nubes)
    float moonWx = 0.0f, moonWy = 0.0f, moonAlt = 0.0f, mAlpha = 0.0f;
    if (dayNight.getMoonWorldPosition(mapWidth, mapHeight, moonWx, moonWy, moonAlt, mAlpha)) {
        Vec2 screenPt = camera.gridToScreen(moonWx, moonWy, moonAlt);
        screenPt.y = std::max(48.0f, screenPt.y);
        drawMoon(screenPt.x, screenPt.y, mAlpha * celestialZoomAlpha, timer, false);
    }
}

void Renderer::drawSun(float cx, float cy, float alpha, float timer, bool onlyInSky) {
    if (alpha <= 0.01f) return;

    int icx = static_cast<int>(cx);
    int icy = static_cast<int>(cy);

    // 1. Corona solar y resplandor difuso exterior (radio 36 px)
    const int coronaR = 36;
    for (int dy = -coronaR; dy <= coronaR; ++dy) {
        int py = icy + dy;
        if (py < 0 || py >= m_height) continue;
        int frameY = m_height - 1 - py;
        for (int dx = -coronaR; dx <= coronaR; ++dx) {
            int px = icx + dx;
            if (px < 0 || px >= m_width) continue;
            float d = std::sqrt(static_cast<float>(dx * dx + dy * dy));
            if (d > coronaR) continue;

            uint32_t bg = m_framebuffer[frameY * m_width + px];
            if (onlyInSky && ((bg >> 24) & 0xFF) == 0xFF) continue;

            float t = d / static_cast<float>(coronaR);
            float w = (1.0f - t) * (1.0f - t) * alpha;

            uint8_t outR = std::min(255, static_cast<int>((bg & 0xFF) + 255.0f * w * 0.90f));
            uint8_t outG = std::min(255, static_cast<int>(((bg >> 8) & 0xFF) + 195.0f * w * 0.85f));
            uint8_t outB = std::min(255, static_cast<int>(((bg >> 16) & 0xFF) + 50.0f * w * 0.70f));
            m_framebuffer[frameY * m_width + px] = (0xFFu << 24) | (outB << 16) | (outG << 8) | outR;
        }
    }

    // 2. Destellos y rayos solares radiantes (8 rayos cardinales y diagonales que pulsan suavemente)
    for (int i = 0; i < 8; ++i) {
        float angle = static_cast<float>(i) * 0.785398f + timer * 0.08f;
        float rayLen = 22.0f + 6.0f * std::sin(timer * 2.6f + static_cast<float>(i) * 1.2f);
        float cosA = std::cos(angle);
        float sinA = std::sin(angle);

        for (float r = 12.0f; r <= 12.0f + rayLen; r += 0.8f) {
            int rx = static_cast<int>(cx + cosA * r);
            int ry = static_cast<int>(cy + sinA * r);
            if (rx < 0 || rx >= m_width || ry < 0 || ry >= m_height) continue;
            int frameY = m_height - 1 - ry;

            uint32_t bg = m_framebuffer[frameY * m_width + rx];
            if (onlyInSky && ((bg >> 24) & 0xFF) == 0xFF) continue;

            float rFact = (1.0f - (r - 12.0f) / rayLen) * alpha * 0.75f;
            uint8_t outR = std::min(255, static_cast<int>((bg & 0xFF) + 255.0f * rFact));
            uint8_t outG = std::min(255, static_cast<int>(((bg >> 8) & 0xFF) + 215.0f * rFact));
            uint8_t outB = std::min(255, static_cast<int>(((bg >> 16) & 0xFF) + 70.0f * rFact));
            m_framebuffer[frameY * m_width + rx] = (0xFFu << 24) | (outB << 16) | (outG << 8) | outR;
        }
    }

    // 3. Disco solar brillante (cuerpo dorado radiante, radio 13 px)
    const int bodyR = 13;
    for (int dy = -bodyR; dy <= bodyR; ++dy) {
        int py = icy + dy;
        if (py < 0 || py >= m_height) continue;
        int frameY = m_height - 1 - py;
        for (int dx = -bodyR; dx <= bodyR; ++dx) {
            int px = icx + dx;
            if (px < 0 || px >= m_width) continue;
            float d = std::sqrt(static_cast<float>(dx * dx + dy * dy));
            if (d > bodyR) continue;

            uint32_t bg = m_framebuffer[frameY * m_width + px];
            if (onlyInSky && ((bg >> 24) & 0xFF) == 0xFF) continue;

            float edge = std::clamp((bodyR - d) / 1.5f, 0.0f, 1.0f);
            float innerW = (d < 6.0f) ? 1.0f : ((bodyR - d) / (bodyR - 6.0f));

            uint8_t sr = static_cast<uint8_t>(255.0f);
            uint8_t sg = static_cast<uint8_t>(std::clamp(215.0f + 40.0f * innerW, 0.0f, 255.0f));
            uint8_t sb = static_cast<uint8_t>(std::clamp(60.0f + 175.0f * (innerW * innerW), 0.0f, 255.0f));

            float blendA = alpha * edge;
            uint8_t outR = static_cast<uint8_t>((bg & 0xFF) * (1.0f - blendA) + sr * blendA);
            uint8_t outG = static_cast<uint8_t>(((bg >> 8) & 0xFF) * (1.0f - blendA) + sg * blendA);
            uint8_t outB = static_cast<uint8_t>(((bg >> 16) & 0xFF) * (1.0f - blendA) + sb * blendA);
            m_framebuffer[frameY * m_width + px] = (0xFFu << 24) | (outB << 16) | (outG << 8) | outR;
        }
    }
}

void Renderer::drawMoon(float cx, float cy, float alpha, float timer, bool onlyInSky) {
    (void)timer;
    if (alpha <= 0.01f) return;

    int icx = static_cast<int>(cx);
    int icy = static_cast<int>(cy);

    // 1. Halo etéreo lunar (halo azul plateado suave, radio 28 px)
    const int haloR = 28;
    for (int dy = -haloR; dy <= haloR; ++dy) {
        int py = icy + dy;
        if (py < 0 || py >= m_height) continue;
        int frameY = m_height - 1 - py;
        for (int dx = -haloR; dx <= haloR; ++dx) {
            int px = icx + dx;
            if (px < 0 || px >= m_width) continue;
            float d = std::sqrt(static_cast<float>(dx * dx + dy * dy));
            if (d > haloR) continue;

            uint32_t bg = m_framebuffer[frameY * m_width + px];
            if (onlyInSky && ((bg >> 24) & 0xFF) == 0xFF) continue;

            float t = d / static_cast<float>(haloR);
            float w = (1.0f - t) * (1.0f - t) * alpha * 0.70f;

            uint8_t outR = std::min(255, static_cast<int>((bg & 0xFF) + 130.0f * w));
            uint8_t outG = std::min(255, static_cast<int>(((bg >> 8) & 0xFF) + 180.0f * w));
            uint8_t outB = std::min(255, static_cast<int>(((bg >> 16) & 0xFF) + 245.0f * w));
            m_framebuffer[frameY * m_width + px] = (0xFFu << 24) | (outB << 16) | (outG << 8) | outR;
        }
    }

    // 2. Disco perlado de la Luna (radio 12 px)
    const int moonR = 12;
    for (int dy = -moonR; dy <= moonR; ++dy) {
        int py = icy + dy;
        if (py < 0 || py >= m_height) continue;
        int frameY = m_height - 1 - py;
        for (int dx = -moonR; dx <= moonR; ++dx) {
            int px = icx + dx;
            if (px < 0 || px >= m_width) continue;
            float d = std::sqrt(static_cast<float>(dx * dx + dy * dy));
            if (d > moonR) continue;

            uint32_t bg = m_framebuffer[frameY * m_width + px];
            if (onlyInSky && ((bg >> 24) & 0xFF) == 0xFF) continue;

            float edge = std::clamp((moonR - d) / 1.2f, 0.0f, 1.0f);

            // Sombreado de cráteres lunares / relieve esférico
            float cr1 = std::sqrt(static_cast<float>((dx + 3) * (dx + 3) + (dy - 2) * (dy - 2)));
            float cr2 = std::sqrt(static_cast<float>((dx - 2) * (dx - 2) + (dy + 3) * (dy + 3)));
            float cr3 = std::sqrt(static_cast<float>((dx + 1) * (dx + 1) + (dy + 4) * (dy + 4)));

            float shade = 1.0f;
            if (cr1 < 3.2f) shade -= (1.0f - cr1 / 3.2f) * 0.22f;
            if (cr2 < 2.5f) shade -= (1.0f - cr2 / 2.5f) * 0.18f;
            if (cr3 < 2.0f) shade -= (1.0f - cr3 / 2.0f) * 0.15f;

            // Sutil sombra de limbo esférico en el cuadrante inferior izquierdo
            float limb = (static_cast<float>(dx - dy)) / static_cast<float>(moonR);
            shade = std::clamp(shade + limb * 0.08f, 0.70f, 1.08f);

            uint8_t mr = static_cast<uint8_t>(std::clamp(232.0f * shade, 0.0f, 255.0f));
            uint8_t mg = static_cast<uint8_t>(std::clamp(240.0f * shade, 0.0f, 255.0f));
            uint8_t mb = static_cast<uint8_t>(std::clamp(252.0f * shade, 0.0f, 255.0f));

            float blendA = alpha * edge;
            uint8_t outR = static_cast<uint8_t>((bg & 0xFF) * (1.0f - blendA) + mr * blendA);
            uint8_t outG = static_cast<uint8_t>(((bg >> 8) & 0xFF) * (1.0f - blendA) + mg * blendA);
            uint8_t outB = static_cast<uint8_t>(((bg >> 16) & 0xFF) * (1.0f - blendA) + mb * blendA);
            m_framebuffer[frameY * m_width + px] = (0xFFu << 24) | (outB << 16) | (outG << 8) | outR;
        }
    }
}

void Renderer::applyFakeDynamicLighting(const DayNightCycle& dayNight,
                                       const WorldMap& map,
                                       const GestorHumanos& humanos,
                                       const DivinePowersSystem* powersSystem,
                                       const IsometricCamera& camera) {
    const int LW = (m_width + 3) / 4;
    const int LH = (m_height + 3) / 4;
    const size_t mapSize = static_cast<size_t>(LW * LH);

    if (m_lightmapR.size() != mapSize) {
        m_lightmapR.resize(mapSize);
        m_lightmapG.resize(mapSize);
        m_lightmapB.resize(mapSize);
    }

    // 1. Inicializar el lightmap con el color ambiental actual del ciclo día/noche
    ColorRGB amb = dayNight.getAmbientColor();
    float baseR = static_cast<float>(amb.r) / 205.0f;
    float baseG = static_cast<float>(amb.g) / 205.0f;
    float baseB = static_cast<float>(amb.b) / 205.0f;

    // Relámpago sagrado: destello global enceguecedor
    float flash = dayNight.getLightningFlashIntensity();
    if (flash > 0.01f) {
        baseR += flash * 1.35f;
        baseG += flash * 1.35f;
        baseB += flash * 1.60f;
    }

    std::fill(m_lightmapR.begin(), m_lightmapR.end(), baseR);
    std::fill(m_lightmapG.begin(), m_lightmapG.end(), baseG);
    std::fill(m_lightmapB.begin(), m_lightmapB.end(), baseB);

    // Función auxiliar para acumular luces puntuales (halo radial cuadrático)
    auto addPointLight = [&](float sx, float sy, float radius, ColorRGB col, float intensity) {
        if (radius <= 1.0f || intensity <= 0.01f) return;
        float cRadius = radius * 0.25f;
        float csx = sx * 0.25f;
        float csy = sy * 0.25f;

        int minX = std::max(0, static_cast<int>(std::floor(csx - cRadius)));
        int maxX = std::min(LW - 1, static_cast<int>(std::ceil(csx + cRadius)));
        int minY = std::max(0, static_cast<int>(std::floor(csy - cRadius)));
        int maxY = std::min(LH - 1, static_cast<int>(std::ceil(csy + cRadius)));

        float rSq = cRadius * cRadius;
        float cr = (static_cast<float>(col.r) / 205.0f) * intensity;
        float cg = (static_cast<float>(col.g) / 205.0f) * intensity;
        float cb = (static_cast<float>(col.b) / 205.0f) * intensity;

        for (int ly = minY; ly <= maxY; ++ly) {
            float dy = static_cast<float>(ly) - csy;
            float dySq = dy * dy;
            int rowOffset = ly * LW;
            for (int lx = minX; lx <= maxX; ++lx) {
                float dx = static_cast<float>(lx) - csx;
                float dSq = dx * dx + dySq;
                if (dSq < rSq) {
                    float dist = std::sqrt(dSq);
                    float normDist = dist / cRadius;
                    float atten = (1.0f - normDist) * (1.0f - normDist);

                    int idx = rowOffset + lx;
                    m_lightmapR[idx] = std::min(1.22f, m_lightmapR[idx] + cr * atten);
                    m_lightmapG[idx] = std::min(1.22f, m_lightmapG[idx] + cg * atten);
                    m_lightmapB[idx] = std::min(1.22f, m_lightmapB[idx] + cb * atten);
                }
            }
        }
    };

    float zoom = camera.getZoom();
    float timer = dayNight.getCycleTimer();
    float hour = dayNight.getHour();

    // Atenuación diurna de fogatas y candiles: de día el sol ilumina el mundo y las antorchas son sutiles
    float nightFactor = 1.0f;
    if (hour >= 7.5f && hour <= 17.0f) {
        nightFactor = 0.20f;
    } else if (hour > 5.0f && hour < 7.5f) {
        float t = (hour - 5.0f) / 2.5f;
        nightFactor = 1.0f - t * 0.80f;
    } else if (hour > 17.0f && hour < 19.5f) {
        float t = (hour - 17.0f) / 2.5f;
        nightFactor = 0.20f + t * 0.80f;
    }

    // 2. Fuentes de Luz: A. Santuarios y Templos Sagrados (Hogueras sagradas con parpadeo suave y natural)
    int mapW = map.getWidth();
    int mapH = map.getHeight();
    for (int cy = 0; cy < mapH; ++cy) {
        for (int cx = 0; cx < mapW; ++cx) {
            const Cell& cell = map.getCell(cx, cy);
            if (cell.hasHolySite) {
                Vec2 pt = camera.gridToScreen(static_cast<float>(cx) + 0.5f, static_cast<float>(cy) + 0.5f, cell.elevation);
                if (pt.x >= -100.0f && pt.x <= m_width + 100.0f && pt.y >= -100.0f && pt.y <= m_height + 100.0f) {
                    float flicker = 1.0f + 0.08f * std::sin(timer * 8.5f + static_cast<float>(cx * 17 ^ cy * 29));
                    float hRadius = 42.0f * zoom;
                    float hIntensity = (0.28f + 0.45f * nightFactor) * flicker;
                    addPointLight(pt.x, pt.y - 8.0f * zoom, hRadius, ColorRGB{255, 200, 110}, hIntensity);
                }
            }
        }
    }

    // Fuentes de Luz: B. Chamanes, Profetas, Fanáticos y Fieles
    for (const auto& h : humanos.getHumanos()) {
        if (!h.estaVivo()) continue;
        int cx = std::clamp(h.getCeldaX(), 0, mapW - 1);
        int cy = std::clamp(h.getCeldaY(), 0, mapH - 1);
        Vec2 pt = camera.gridToScreen(h.getX(), h.getY(), map.getCell(cx, cy).elevation);
        if (pt.x < -80.0f || pt.x > m_width + 80.0f || pt.y < -80.0f || pt.y > m_height + 80.0f) continue;

        if (h.esLiderEspiritual()) {
            bool isProphet = h.esProfeta();
            float rad = (isProphet ? 42.0f : 32.0f) * zoom;
            ColorRGB col = isProphet ? ColorRGB{255, 240, 160} : ColorRGB{250, 210, 100};
            float inten = (0.20f + 0.35f * nightFactor) + 0.04f * std::sin(timer * 6.0f + static_cast<float>(h.getId()));

            if (h.getTimerSenalDivina() > 0.0f) {
                rad = 56.0f * zoom;
                col = ColorRGB{255, 245, 175};
                inten = 0.60f;
            }
            addPointLight(pt.x, pt.y - 12.0f * zoom, rad, col, inten);
        } else if (h.esFanatico()) {
            float flicker = 1.0f + 0.08f * std::sin(timer * 9.0f + static_cast<float>(h.getId() * 3));
            addPointLight(pt.x, pt.y - 8.0f * zoom, 26.0f * zoom, ColorRGB{255, 170, 70}, 0.25f * nightFactor * flicker);
        } else if (h.getAccion() == AccionHumano::REZAR || (!dayNight.isDay() && (h.getId() % 6 == 0))) {
            // Hogueras y teas en grupos de aldeanos rezando o nocturnos
            float flicker = 0.95f + 0.08f * std::sin(timer * 7.5f + static_cast<float>(h.getId() * 5));
            addPointLight(pt.x, pt.y - 6.0f * zoom, 22.0f * zoom, ColorRGB{255, 180, 85}, 0.20f * nightFactor * flicker);
        }
    }

    // Fuentes de Luz: C. Efectos y Poderes Divinos
    if (powersSystem) {
        for (const auto& fx : powersSystem->getActiveFX()) {
            Vec2 pt = camera.gridToScreen(fx.worldX, fx.worldY, 1.0f);
            if (fx.type == DivinePowerType::Lightning) {
                addPointLight(pt.x, pt.y, 210.0f * zoom, ColorRGB{220, 245, 255}, 2.5f);
            } else if (fx.type == DivinePowerType::Miracle) {
                addPointLight(pt.x, pt.y, 180.0f * zoom, ColorRGB{255, 250, 160}, 1.9f);
            } else if (fx.type == DivinePowerType::Earthquake) {
                addPointLight(pt.x, pt.y, 140.0f * zoom, ColorRGB{255, 90, 30}, 1.3f);
            } else if (fx.type == DivinePowerType::Tornado) {
                addPointLight(pt.x, pt.y, 150.0f * zoom, ColorRGB{170, 220, 255}, 1.2f);
            } else if (fx.type == DivinePowerType::Meteor) {
                if (fx.timer < 1.25f) {
                    float prog = fx.timer / 1.25f;
                    float fxX = fx.worldX + (1.0f - prog) * 4.0f;
                    float fxY = fx.worldY - (1.0f - prog) * 6.0f;
                    Vec2 fPt = camera.gridToScreen(fxX, fxY, 1.0f);
                    addPointLight(fPt.x, fPt.y - (1.0f - prog) * 200.0f * zoom, 170.0f * zoom, ColorRGB{255, 150, 40}, 2.4f);
                    addPointLight(pt.x, pt.y, 110.0f * zoom, ColorRGB{255, 90, 20}, 1.0f + prog * 1.5f);
                } else {
                    float decay = 1.0f - std::clamp((fx.timer - 1.25f) / 3.75f, 0.0f, 1.0f);
                    addPointLight(pt.x, pt.y, (140.0f + decay * 200.0f) * zoom, ColorRGB{255, 120, 30}, 2.8f * decay + 0.5f);
                }
            } else if (fx.type == DivinePowerType::Volcano) {
                float pulse = 1.0f + 0.15f * std::sin(fx.timer * 8.0f);
                addPointLight(pt.x, pt.y - 25.0f * zoom, 280.0f * zoom, ColorRGB{255, 75, 15}, 2.6f * pulse);
            } else if (fx.type == DivinePowerType::Wildfire) {
                float pulse = 1.0f + 0.22f * std::sin(fx.timer * 11.0f + fx.seed);
                addPointLight(pt.x, pt.y - 10.0f * zoom, (180.0f + 25.0f * std::cos(fx.timer * 7.0f)) * zoom, ColorRGB{255, 110, 20}, 2.2f * pulse);
            }
        }
    }

    // 3. Aplicar modulación con filtrado bilineal por pasos lineales enteros (fixed-point 8.8) ultrarrápido
    // El eje Y de pantalla (0 arriba) se mapea exactamente con el framebuffer OpenGL (0 abajo)
    for (int py = 0; py < m_height; ++py) {
        int rowOffset = py * m_width;
        float v = static_cast<float>(m_height - 1 - py) * 0.25f;
        int ly = std::clamp(static_cast<int>(v), 0, LH - 2);
        float fy = v - static_cast<float>(ly);
        float invFy = 1.0f - fy;

        int row0 = ly * LW;
        int row1 = (ly + 1) * LW;

        for (int lx = 0; lx < LW - 1; ++lx) {
            int pxBase = lx * 4;
            int i00 = row0 + lx;
            int i01 = row1 + lx;

            float r00 = m_lightmapR[i00], r10 = m_lightmapR[i00 + 1], r01 = m_lightmapR[i01], r11 = m_lightmapR[i01 + 1];
            float g00 = m_lightmapG[i00], g10 = m_lightmapG[i00 + 1], g01 = m_lightmapG[i01], g11 = m_lightmapG[i01 + 1];
            float b00 = m_lightmapB[i00], b10 = m_lightmapB[i00 + 1], b01 = m_lightmapB[i01], b11 = m_lightmapB[i01 + 1];

            float ry0 = r00 * invFy + r01 * fy;
            float ry1 = r10 * invFy + r11 * fy;
            float gy0 = g00 * invFy + g01 * fy;
            float gy1 = g10 * invFy + g11 * fy;
            float by0 = b00 * invFy + b01 * fy;
            float by1 = b10 * invFy + b11 * fy;

            int curR = static_cast<int>(ry0 * 256.0f);
            int curG = static_cast<int>(gy0 * 256.0f);
            int curB = static_cast<int>(by0 * 256.0f);

            int stepR = static_cast<int>((ry1 - ry0) * 64.0f);
            int stepG = static_cast<int>((gy1 - gy0) * 64.0f);
            int stepB = static_cast<int>((by1 - by0) * 64.0f);

            int maxSub = std::min(4, m_width - pxBase);
            for (int sub = 0; sub < maxSub; ++sub) {
                int px = pxBase + sub;
                uint32_t pix = m_framebuffer[rowOffset + px];
                if ((pix & 0xFF000000u) == 0xFF000000u) {
                    uint32_t r = std::min(255u, (static_cast<uint32_t>(pix & 0xFFu) * curR) >> 8);
                    uint32_t g = std::min(255u, (static_cast<uint32_t>((pix >> 8) & 0xFFu) * curG) >> 8);
                    uint32_t b = std::min(255u, (static_cast<uint32_t>((pix >> 16) & 0xFFu) * curB) >> 8);
                    m_framebuffer[rowOffset + px] = 0xFF000000u | (b << 16) | (g << 8) | r;
                } else {
                    m_framebuffer[rowOffset + px] = pix | 0xFF000000u;
                }
                curR += stepR;
                curG += stepG;
                curB += stepB;
            }
        }
    }
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

void Renderer::drawSims2Panel(int x, int y, int w, int h, ColorRGB baseColor, bool sunken, ColorRGB rimColor) {
    if (w <= 0 || h <= 0) return;
    int xEnd = std::min(m_width, x + w);
    int yEnd = std::min(m_height, y + h);
    int xStart = std::max(0, x);
    int yStart = std::max(0, y);

    float br = static_cast<float>(baseColor.r);
    float bg = static_cast<float>(baseColor.g);
    float bb = static_cast<float>(baseColor.b);
    float invH = 1.0f / std::max(1.0f, static_cast<float>(h - 1));

    // Precalcular la curva de iluminación vertical estilo Los Sims 2
    std::vector<float> multV(h);
    for (int j = 0; j < h; ++j) {
        float v = static_cast<float>(j) * invH;
        if (!sunken) {
            // Panel con volumen satinado:
            // 1) Gradiente ambiental suave
            float amb = 1.16f - 0.36f * v;
            // 2) Resalte especular satinado en el tercio superior (pico alrededor de v = 0.16)
            float diff = (v - 0.16f) / 0.14f;
            float sheen = 0.32f * std::exp(-diff * diff);
            multV[j] = amb + sheen;
        } else {
            // Bandeja hundida: gradiente invertido (sombra superior profunda, rebote de luz abajo)
            float diffTop = v / 0.12f;
            multV[j] = 0.72f + 0.32f * v - 0.28f * std::exp(-diffTop * diffTop);
        }
    }

    for (int j = yStart; j < yEnd; ++j) {
        int py = m_height - 1 - j;
        if (py < 0 || py >= m_height) continue;
        int localY = j - y;
        float rowMult = multV[localY];

        for (int i = xStart; i < xEnd; ++i) {
            int localX = i - x;

            // Curvatura lateral cilíndrica sutil en los extremos
            float sideFactor = 1.0f;
            if (localX < 6) {
                sideFactor = 0.86f + 0.14f * (static_cast<float>(localX) / 6.0f);
            } else if (localX >= w - 6) {
                sideFactor = 0.86f + 0.14f * (static_cast<float>(w - 1 - localX) / 6.0f);
            }

            float finalMult = rowMult * sideFactor;
            float warmBoost = std::max(0.0f, finalMult - 1.0f) * 24.0f;

            int r = std::clamp(static_cast<int>(br * finalMult + warmBoost * 1.2f), 0, 255);
            int g = std::clamp(static_cast<int>(bg * finalMult + warmBoost * 0.9f), 0, 255);
            int b = std::clamp(static_cast<int>(bb * finalMult + warmBoost * 0.6f), 0, 255);

            // Biselado interno tridimensional Sims 2
            if (!sunken) {
                if (localY == 1 && localX >= 1 && localX <= w - 2) {
                    r = std::min(255, r + 42); g = std::min(255, g + 32); b = std::min(255, b + 22);
                } else if (localX == 1 && localY >= 1 && localY <= h - 2) {
                    r = std::min(255, r + 24); g = std::min(255, g + 18); b = std::min(255, b + 12);
                } else if (localY == h - 2 && localX >= 1 && localX <= w - 2) {
                    r = std::max(0, r - 24); g = std::max(0, g - 24); b = std::max(0, b - 24);
                } else if (localX == w - 2 && localY >= 1 && localY <= h - 2) {
                    r = std::max(0, r - 18); g = std::max(0, g - 18); b = std::max(0, b - 18);
                }
            } else {
                if (localY == 1 && localX >= 1 && localX <= w - 2) {
                    r = std::max(0, r - 38); g = std::max(0, r - 38); b = std::max(0, b - 38);
                } else if (localX == 1 && localY >= 1 && localY <= h - 2) {
                    r = std::max(0, r - 28); g = std::max(0, r - 28); b = std::max(0, b - 28);
                } else if (localY == h - 2 && localX >= 1 && localX <= w - 2) {
                    r = std::min(255, r + 28); g = std::min(255, g + 22); b = std::min(255, b + 16);
                }
            }

            m_framebuffer[py * m_width + i] = (0xFFu << 24) |
                                              (static_cast<uint32_t>(b) << 16) |
                                              (static_cast<uint32_t>(g) << 8) |
                                               static_cast<uint32_t>(r);
        }
    }

    // Contorno perimetral
    drawRectOutline(x, y, w, h, rimColor);
}

void Renderer::drawSims2Bar(int x, int y, int w, int h, ColorRGB barColor) {
    if (w <= 0 || h <= 0) return;
    int xEnd = std::min(m_width, x + w);
    int yEnd = std::min(m_height, y + h);
    int xStart = std::max(0, x);
    int yStart = std::max(0, y);

    float br = static_cast<float>(barColor.r);
    float bg = static_cast<float>(barColor.g);
    float bb = static_cast<float>(barColor.b);

    for (int j = yStart; j < yEnd; ++j) {
        int py = m_height - 1 - j;
        if (py < 0 || py >= m_height) continue;
        int localY = j - y;
        float v = static_cast<float>(localY) / std::max(1.0f, static_cast<float>(h - 1));

        // Curvatura de píldora cilíndrica brillante estilo Sims 2
        float diff = (v - 0.28f) / 0.22f;
        float sheen = 0.45f * std::exp(-diff * diff);
        float baseMult = 0.80f + 0.35f * (1.0f - v) + sheen;

        int r = std::clamp(static_cast<int>(br * baseMult + sheen * 40.0f), 0, 255);
        int g = std::clamp(static_cast<int>(bg * baseMult + sheen * 40.0f), 0, 255);
        int b = std::clamp(static_cast<int>(bb * baseMult + sheen * 40.0f), 0, 255);

        for (int i = xStart; i < xEnd; ++i) {
            int localX = i - x;
            float endShade = 1.0f;
            if (localX == 0 || localX == w - 1) endShade = 0.75f;

            m_framebuffer[py * m_width + i] = (0xFFu << 24) |
                                              (static_cast<uint32_t>(std::clamp(static_cast<int>(b * endShade), 0, 255)) << 16) |
                                              (static_cast<uint32_t>(std::clamp(static_cast<int>(g * endShade), 0, 255)) << 8) |
                                               static_cast<uint32_t>(std::clamp(static_cast<int>(r * endShade), 0, 255));
        }
    }

    // Brillo en el borde superior de la barra
    if (h >= 3) {
        drawLine(x, y, x + w - 1, y, ColorRGB{
            static_cast<uint8_t>(std::min(255, barColor.r + 70)),
            static_cast<uint8_t>(std::min(255, barColor.g + 70)),
            static_cast<uint8_t>(std::min(255, barColor.b + 70))
        });
    }
}

void Renderer::drawSims2Meter(int x, int y, int w, int h, float progress, ColorRGB barColor, ColorRGB trayColor) {
    progress = std::clamp(progress, 0.0f, 1.0f);
    // 1. Bandeja hundida
    drawSims2Panel(x, y, w, h, trayColor, true, ColorRGB{75, 52, 38});
    // 2. Barra de progreso cilíndrica satinada
    int fillW = static_cast<int>((w - 2) * progress);
    if (fillW > 0) {
        drawSims2Bar(x + 1, y + 1, fillW, h - 2, barColor);
    }
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

void Renderer::drawResourceNode(float cx, float cy, ResourceType type, float zoom, int amount) {
    (void)amount;
    int resIndex = -1;
    switch (type) {
        case ResourceType::Food:  resIndex = 0; break;
        case ResourceType::Wood:  resIndex = 1; break;
        case ResourceType::Stone: resIndex = 2; break;
        case ResourceType::Gold:  resIndex = 3; break;
        case ResourceType::Iron:  resIndex = 4; break;
        default: return;
    }
    if (resIndex < 0 || resIndex >= static_cast<int>(m_resourcePropPixels.size())) return;
    const auto& propPixels = m_resourcePropPixels[static_cast<size_t>(resIndex)];
    if (propPixels.size() != 64u * 64u) return;

    float scale = std::clamp(zoom * 0.44f, 0.18f, 1.20f);
    int destW = static_cast<int>(64.0f * scale);
    int destH = static_cast<int>(64.0f * scale);
    if (destW <= 2 || destH <= 2) return;

    int startX = static_cast<int>(cx - destW * 0.5f);
    int startY = static_cast<int>(cy + 6.0f * zoom - destH);

    // Sombra suave en la base de la fuente de recursos
    int shadowRx = std::max(3, static_cast<int>(12.0f * zoom));
    int shadowRy = std::max(2, static_cast<int>(6.0f * zoom));
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
                uint8_t br = static_cast<uint8_t>((bg & 0xFF) * 0.78f);
                uint8_t bgc = static_cast<uint8_t>(((bg >> 8) & 0xFF) * 0.78f);
                uint8_t bb = static_cast<uint8_t>(((bg >> 16) & 0xFF) * 0.78f);
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
            if (a < 15) continue;

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
    if (scale <= 0 || text.empty()) return;

    int cursorX = x;
    size_t i = 0;
    while (i < text.size()) {
        char ch = text[i];
        if (ch == '\n') {
            cursorX = x;
            y += (scale == 1 ? 12 : 12 * scale);
            ++i;
            continue;
        }

        uint32_t codepoint = 0;
        uint8_t b0 = static_cast<uint8_t>(ch);
        if ((b0 & 0x80) == 0) {
            codepoint = b0;
            i += 1;
        } else if ((b0 & 0xE0) == 0xC0) {
            if (i + 1 < text.size()) {
                uint8_t b1 = static_cast<uint8_t>(text[i + 1]);
                codepoint = ((b0 & 0x1F) << 6) | (b1 & 0x3F);
                i += 2;
            } else {
                codepoint = '?';
                i += 1;
            }
        } else if ((b0 & 0xF0) == 0xE0) {
            if (i + 2 < text.size()) {
                uint8_t b1 = static_cast<uint8_t>(text[i + 1]);
                uint8_t b2 = static_cast<uint8_t>(text[i + 2]);
                codepoint = ((b0 & 0x0F) << 12) | ((b1 & 0x3F) << 6) | (b2 & 0x3F);
                i += 3;
            } else {
                codepoint = '?';
                i += 1;
            }
        } else if ((b0 & 0xF8) == 0xF0) {
            if (i + 3 < text.size()) {
                uint8_t b1 = static_cast<uint8_t>(text[i + 1]);
                uint8_t b2 = static_cast<uint8_t>(text[i + 2]);
                uint8_t b3 = static_cast<uint8_t>(text[i + 3]);
                codepoint = ((b0 & 0x07) << 18) | ((b1 & 0x3F) << 12) | ((b2 & 0x3F) << 6) | (b3 & 0x3F);
                i += 4;
            } else {
                codepoint = '?';
                i += 1;
            }
        } else {
            codepoint = '?';
            i += 1;
        }

        int glyphIdx = DejaVuFont::getGlyphIndex(codepoint);
        if (glyphIdx < 0 || glyphIdx >= static_cast<int>(DejaVuFont::GLYPH_COUNT)) {
            glyphIdx = DejaVuFont::getGlyphIndex('?');
        }

        if (scale == 1) {
            const uint8_t* glyphData = DejaVuFont::REGULAR_GLYPHS[glyphIdx];
            for (int row = 0; row < DejaVuFont::REG_HEIGHT; ++row) {
                int py = y + row;
                int frameY = m_height - 1 - py;
                if (frameY < 0 || frameY >= m_height) continue;

                int rowOffset = row * DejaVuFont::REG_WIDTH;
                int fbRowOffset = frameY * m_width;

                for (int col = 0; col < DejaVuFont::REG_WIDTH; ++col) {
                    uint8_t a = glyphData[rowOffset + col];
                    if (a == 0) continue;

                    int px = cursorX + col;
                    if (px < 0 || px >= m_width) continue;

                    uint32_t effAlpha = (static_cast<uint32_t>(a) * color.a) / 255;
                    if (effAlpha == 0) continue;

                    int fbIdx = fbRowOffset + px;
                    if (effAlpha >= 254) {
                        m_framebuffer[fbIdx] = (0xFFu << 24) |
                                               (static_cast<uint32_t>(color.b) << 16) |
                                               (static_cast<uint32_t>(color.g) << 8)  |
                                               static_cast<uint32_t>(color.r);
                    } else {
                        uint32_t bg = m_framebuffer[fbIdx];
                        uint8_t bgR = bg & 0xFF;
                        uint8_t bgG = (bg >> 8) & 0xFF;
                        uint8_t bgB = (bg >> 16) & 0xFF;
                        uint8_t outR = static_cast<uint8_t>((color.r * effAlpha + bgR * (255 - effAlpha)) / 255);
                        uint8_t outG = static_cast<uint8_t>((color.g * effAlpha + bgG * (255 - effAlpha)) / 255);
                        uint8_t outB = static_cast<uint8_t>((color.b * effAlpha + bgB * (255 - effAlpha)) / 255);
                        m_framebuffer[fbIdx] = (0xFFu << 24) |
                                               (static_cast<uint32_t>(outB) << 16) |
                                               (static_cast<uint32_t>(outG) << 8)  |
                                               static_cast<uint32_t>(outR);
                    }
                }
            }
            cursorX += DejaVuFont::REG_ADVANCE;
        } else if (scale == 2) {
            const uint8_t* glyphData = DejaVuFont::BOLD_GLYPHS[glyphIdx];
            for (int row = 0; row < DejaVuFont::BOLD_HEIGHT; ++row) {
                int py = y + row;
                int frameY = m_height - 1 - py;
                if (frameY < 0 || frameY >= m_height) continue;

                int rowOffset = row * DejaVuFont::BOLD_WIDTH;
                int fbRowOffset = frameY * m_width;

                for (int col = 0; col < DejaVuFont::BOLD_WIDTH; ++col) {
                    uint8_t a = glyphData[rowOffset + col];
                    if (a == 0) continue;

                    int px = cursorX + col;
                    if (px < 0 || px >= m_width) continue;

                    uint32_t effAlpha = (static_cast<uint32_t>(a) * color.a) / 255;
                    if (effAlpha == 0) continue;

                    int fbIdx = fbRowOffset + px;
                    if (effAlpha >= 254) {
                        m_framebuffer[fbIdx] = (0xFFu << 24) |
                                               (static_cast<uint32_t>(color.b) << 16) |
                                               (static_cast<uint32_t>(color.g) << 8)  |
                                               static_cast<uint32_t>(color.r);
                    } else {
                        uint32_t bg = m_framebuffer[fbIdx];
                        uint8_t bgR = bg & 0xFF;
                        uint8_t bgG = (bg >> 8) & 0xFF;
                        uint8_t bgB = (bg >> 16) & 0xFF;
                        uint8_t outR = static_cast<uint8_t>((color.r * effAlpha + bgR * (255 - effAlpha)) / 255);
                        uint8_t outG = static_cast<uint8_t>((color.g * effAlpha + bgG * (255 - effAlpha)) / 255);
                        uint8_t outB = static_cast<uint8_t>((color.b * effAlpha + bgB * (255 - effAlpha)) / 255);
                        m_framebuffer[fbIdx] = (0xFFu << 24) |
                                               (static_cast<uint32_t>(outB) << 16) |
                                               (static_cast<uint32_t>(outG) << 8)  |
                                               static_cast<uint32_t>(outR);
                    }
                }
            }
            cursorX += DejaVuFont::BOLD_ADVANCE;
        } else {
            // General scaling for scale >= 3
            const uint8_t* glyphData = DejaVuFont::REGULAR_GLYPHS[glyphIdx];
            for (int row = 0; row < DejaVuFont::REG_HEIGHT; ++row) {
                int rowOffset = row * DejaVuFont::REG_WIDTH;
                for (int col = 0; col < DejaVuFont::REG_WIDTH; ++col) {
                    uint8_t a = glyphData[rowOffset + col];
                    if (a == 0) continue;

                    uint32_t effAlpha = (static_cast<uint32_t>(a) * color.a) / 255;
                    if (effAlpha == 0) continue;

                    for (int sy = 0; sy < scale; ++sy) {
                        int py = y + row * scale + sy;
                        int frameY = m_height - 1 - py;
                        if (frameY < 0 || frameY >= m_height) continue;
                        int fbRowOffset = frameY * m_width;

                        for (int sx = 0; sx < scale; ++sx) {
                            int px = cursorX + col * scale + sx;
                            if (px < 0 || px >= m_width) continue;

                            int fbIdx = fbRowOffset + px;
                            if (effAlpha >= 254) {
                                m_framebuffer[fbIdx] = (0xFFu << 24) |
                                                       (static_cast<uint32_t>(color.b) << 16) |
                                                       (static_cast<uint32_t>(color.g) << 8)  |
                                                       static_cast<uint32_t>(color.r);
                            } else {
                                uint32_t bg = m_framebuffer[fbIdx];
                                uint8_t bgR = bg & 0xFF;
                                uint8_t bgG = (bg >> 8) & 0xFF;
                                uint8_t bgB = (bg >> 16) & 0xFF;
                                uint8_t outR = static_cast<uint8_t>((color.r * effAlpha + bgR * (255 - effAlpha)) / 255);
                                uint8_t outG = static_cast<uint8_t>((color.g * effAlpha + bgG * (255 - effAlpha)) / 255);
                                uint8_t outB = static_cast<uint8_t>((color.b * effAlpha + bgB * (255 - effAlpha)) / 255);
                                m_framebuffer[fbIdx] = (0xFFu << 24) |
                                                       (static_cast<uint32_t>(outB) << 16) |
                                                       (static_cast<uint32_t>(outG) << 8)  |
                                                       static_cast<uint32_t>(outR);
                            }
                        }
                    }
                }
            }
            cursorX += DejaVuFont::REG_ADVANCE * scale;
        }
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

void Renderer::drawSpriteHouse(float cx, float cy, const House& house, float zoom, bool isNight) {
    if (m_buildingsPixels.empty() || m_buildingsWidth <= 0 || m_buildingsHeight <= 0) {
        int icx = static_cast<int>(cx);
        int icy = static_cast<int>(cy);
        ColorRGB woodCol(139, 69, 19);
        ColorRGB thatchCol(218, 165, 32);
        drawRect(icx - 10, icy - 10, 20, 10, woodCol);
        drawRect(icx - 6, icy - 14, 12, 4, thatchCol);
        return;
    }

    int srcX = 201;
    int srcY = 101;
    int srcW = 137;
    int srcH = 100;
    bool underConstr = house.isUnderConstruction();

    if (underConstr) {
        if (house.type == HouseType::BoneDwelling) {
            srcX = 184; srcY = 436; srcW = 157; srcH = 99; // crop_bone_frame
        } else {
            srcX = 29; srcY = 428; srcW = 136; srcH = 108; // crop_house_frame
        }
    } else {
        switch (house.type) {
            case HouseType::BoneDwelling:
                srcX = 377; srcY = 124; srcW = 117; srcH = 103; // crop_bone_dwelling
                break;
            case HouseType::LargeRoundhouse:
                srcX = 23; srcY = 48; srcW = 156; srcH = 156;   // crop_house_roundhouse
                break;
            case HouseType::ChieftainLonghouse:
                srcX = 702; srcY = 144; srcW = 150; srcH = 125; // crop_house_chieftain
                break;
            case HouseType::ThatchedHut:
            case HouseType::PrimitiveShelter:
            default:
                srcX = 201; srcY = 101; srcW = 137; srcH = 100; // crop_house_thatched
                break;
        }
    }

    float scale = std::clamp(zoom * 0.44f, 0.16f, 1.25f);
    int destW = static_cast<int>(srcW * scale);
    int destH = static_cast<int>(srcH * scale);
    if (destW <= 4 || destH <= 4) return;

    int startX = static_cast<int>(cx - destW * 0.5f);
    int startY = static_cast<int>(cy + 10.0f * zoom - destH);

    // Sombra elíptica en la base de la vivienda
    int shRx = std::max(4, static_cast<int>(20.0f * zoom));
    int shRy = std::max(2, static_cast<int>(10.0f * zoom));
    int shCx = static_cast<int>(cx);
    int shCy = static_cast<int>(cy + 2.0f * zoom);
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

    // Blit de los píxeles del edificio
    for (int dy = 0; dy < destH; ++dy) {
        int dstY = startY + dy;
        int frameY = m_height - 1 - dstY;
        if (frameY < 0 || frameY >= m_height) continue;

        int sy = srcY + std::clamp(static_cast<int>(dy / scale), 0, srcH - 1);

        for (int dx = 0; dx < destW; ++dx) {
            int dstX = startX + dx;
            if (dstX < 0 || dstX >= m_width) continue;

            int sx = srcX + std::clamp(static_cast<int>(dx / scale), 0, srcW - 1);
            uint32_t src = m_buildingsPixels[sy * m_buildingsWidth + sx];
            uint8_t a = (src >> 24) & 0xFF;
            if (a < 18) continue;

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

    // Barra de progreso si está bajo construcción
    if (underConstr) {
        int barW = static_cast<int>(32.0f * zoom);
        int barH = std::max(3, static_cast<int>(5.0f * zoom));
        int barX = static_cast<int>(cx - barW * 0.5f);
        int barY = startY - barH - 4;

        drawRect(barX, barY, barW, barH, ColorRGB{20, 15, 12});
        drawRectOutline(barX, barY, barW, barH, ColorRGB{60, 48, 36});
        float pct = std::clamp(house.buildProgress / 100.0f, 0.0f, 1.0f);
        int fillW = static_cast<int>(barW * pct);
        if (fillW > 0) {
            ColorRGB progColor = (pct < 0.5f) ? ColorRGB{220, 160, 40} : ColorRGB{80, 210, 80};
            drawRect(barX, barY, fillW, barH, progColor);
        }
    }

    // Brillo cálido de hogar y humo/zZ cuando hay aldeanos durmiendo bajo techo
    if (house.isCompleted()) {
        bool tieneDurmientes = (house.sleepingOccupants > 0);
        if (isNight || tieneDurmientes) {
            float flicker = 0.85f + 0.15f * std::sin(m_animTime * 3.8f + static_cast<float>(house.id * 3));
            int glowRx = std::max(3, static_cast<int>(14.0f * zoom));
            int glowRy = std::max(2, static_cast<int>(7.0f * zoom));
            int glowCx = static_cast<int>(cx);
            int glowCy = static_cast<int>(cy - 2.0f * zoom);

            for (int gy = -glowRy; gy <= glowRy; ++gy) {
                int fy = m_height - 1 - (glowCy + gy);
                if (fy < 0 || fy >= m_height) continue;
                for (int gx = -glowRx; gx <= glowRx; ++gx) {
                    int fx = glowCx + gx;
                    if (fx < 0 || fx >= m_width) continue;
                    float distNorm = (gx * gx) / static_cast<float>(glowRx * glowRx) +
                                     (gy * gy) / static_cast<float>(glowRy * glowRy);
                    if (distNorm <= 1.0f) {
                        float factor = (1.0f - distNorm) * (tieneDurmientes ? 0.55f : 0.45f) * flicker;
                        uint32_t bg = m_framebuffer[fy * m_width + fx];
                        uint8_t br = bg & 0xFF;
                        uint8_t bgc = (bg >> 8) & 0xFF;
                        uint8_t bb = (bg >> 16) & 0xFF;
                        uint8_t outR = std::min(255, static_cast<int>(br + 250 * factor));
                        uint8_t outG = std::min(255, static_cast<int>(bgc + 160 * factor));
                        uint8_t outB = std::min(255, static_cast<int>(bb + 40 * factor));
                        m_framebuffer[fy * m_width + fx] = (0xFFu << 24) | (outB << 16) | (outG << 8) | outR;
                    }
                }
            }
        }

        // Partículas "zZ" flotando desde la cumbre del tejado / chimenea
        if (tieneDurmientes) {
            float osc = std::sin(m_animTime * 2.5f + static_cast<float>(house.id * 2));
            float rise = std::fmod(m_animTime * 10.0f + static_cast<float>(house.id * 5), 20.0f);
            float zScale = std::clamp(zoom * 0.45f, 0.25f, 1.20f);

            int zx1 = static_cast<int>(cx + osc * 3.5f * zScale);
            int zy1 = static_cast<int>(startY - 3.0f * zScale - rise * zScale);
            int zx2 = zx1 + static_cast<int>(6.0f * zScale);
            int zy2 = zy1 - static_cast<int>(6.0f * zScale);

            ColorRGB zCol{195, 230, 255}; // Celeste etéreo de descanso
            auto drawP = [&](int px, int py, ColorRGB col) {
                int fy = m_height - 1 - py;
                if (px >= 0 && px < m_width && fy >= 0 && fy < m_height) {
                    m_framebuffer[fy * m_width + px] = col.toRGBA();
                }
            };

            // 'z' pequeña flotando
            drawP(zx1, zy1, zCol);     drawP(zx1+1, zy1, zCol);   drawP(zx1+2, zy1, zCol);
                                       drawP(zx1+1, zy1+1, zCol);
            drawP(zx1, zy1+2, zCol);   drawP(zx1+1, zy1+2, zCol); drawP(zx1+2, zy1+2, zCol);

            // 'Z' mediana flotando arriba
            drawP(zx2, zy2, zCol);     drawP(zx2+1, zy2, zCol);   drawP(zx2+2, zy2, zCol); drawP(zx2+3, zy2, zCol);
                                                                  drawP(zx2+2, zy2+1, zCol);
                                       drawP(zx2+1, zy2+2, zCol);
            drawP(zx2, zy2+3, zCol);   drawP(zx2+1, zy2+3, zCol); drawP(zx2+2, zy2+3, zCol); drawP(zx2+3, zy2+3, zCol);
        }
    }
}

void Renderer::drawVolcano(float cx, float cy, float zoom, float animTime) {
    float scale = std::clamp(zoom, 0.45f, 2.80f);
    float baseW = 50.0f * scale;
    float baseH = 25.0f * scale;
    float volcanoH = 68.0f * scale;
    float calW = 16.0f * scale;
    float calH = 8.0f * scale;

    // 1. Sombra elíptica suave en la base del volcán
    int shRx = static_cast<int>(baseW * 1.15f);
    int shRy = static_cast<int>(baseH * 1.15f);
    int shCx = static_cast<int>(cx);
    int shCy = static_cast<int>(cy + 3.0f * scale);
    for (int oy = -shRy; oy <= shRy; ++oy) {
        int dstY = shCy + oy;
        int fy = m_height - 1 - dstY;
        if (fy < 0 || fy >= m_height) continue;
        for (int ox = -shRx; ox <= shRx; ++ox) {
            int fx = shCx + ox;
            if (fx < 0 || fx >= m_width) continue;
            float nd = (ox * ox) / static_cast<float>(shRx * shRx) +
                       (oy * oy) / static_cast<float>(shRy * shRy);
            if (nd <= 1.0f) {
                uint32_t bg = m_framebuffer[fy * m_width + fx];
                float sFade = 0.65f + 0.35f * nd;
                uint8_t br = static_cast<uint8_t>((bg & 0xFF) * sFade);
                uint8_t bgc = static_cast<uint8_t>(((bg >> 8) & 0xFF) * sFade);
                uint8_t bb = static_cast<uint8_t>(((bg >> 16) & 0xFF) * sFade);
                m_framebuffer[fy * m_width + fx] = (0xFFu << 24) | (bb << 16) | (bgc << 8) | br;
            }
        }
    }

    // 2. Cono del estratovolcán (rasterización vertical desde la cumbre hasta la base)
    int summitY = static_cast<int>(cy - volcanoH);
    int bottomY = static_cast<int>(cy + 6.0f * scale);

    for (int y = summitY; y <= bottomY; ++y) {
        int frameY = m_height - 1 - y;
        if (frameY < 0 || frameY >= m_height) continue;

        float tY = std::clamp(static_cast<float>(y - summitY) / std::max(1.0f, static_cast<float>(bottomY - summitY)), 0.0f, 1.0f);
        float coneCurve = tY * tY * 0.4f + tY * 0.6f;
        float curHalfW = calW * (1.0f - coneCurve) + baseW * coneCurve;

        int minX = static_cast<int>(cx - curHalfW);
        int maxX = static_cast<int>(cx + curHalfW);

        for (int x = minX; x <= maxX; ++x) {
            if (x < 0 || x >= m_width) continue;

            float normX = (static_cast<float>(x) - cx) / std::max(1.0f, curHalfW);
            float light = 0.52f - 0.40f * normX;
            float ridge = std::sin(normX * 8.5f + tY * 5.0f) * 0.12f;

            float vr = (44.0f + 36.0f * light + ridge * 25.0f);
            float vg = (35.0f + 28.0f * light + ridge * 20.0f);
            float vb = (38.0f + 24.0f * light + ridge * 20.0f);

            // Ríos de lava y magma activo descendiendo por las laderas del cono
            float s1 = -0.32f + 0.14f * std::sin(tY * 4.5f + 0.8f);
            float s2 =  0.08f + 0.10f * std::cos(tY * 5.5f);
            float s3 =  0.42f - 0.12f * std::sin(tY * 3.8f + 1.8f);

            float d1 = std::abs(normX - s1);
            float d2 = std::abs(normX - s2);
            float d3 = std::abs(normX - s3);
            float dStream = std::min({d1, d2, d3});

            if (dStream < 0.085f) {
                float flow = 0.5f + 0.5f * std::sin(tY * 16.0f - animTime * 6.5f + normX * 4.0f);
                float coreW = dStream / 0.085f;
                float magmaR = 255.0f;
                float magmaG = 65.0f + 140.0f * flow + (1.0f - coreW) * 40.0f;
                float magmaB = 18.0f + (1.0f - coreW) * 20.0f;

                vr = vr * coreW + magmaR * (1.0f - coreW);
                vg = vg * coreW + magmaG * (1.0f - coreW);
                vb = vb * coreW + magmaB * (1.0f - coreW);
            } else if (dStream < 0.16f) {
                float glowW = (0.16f - dStream) / 0.075f;
                vr = std::min(255.0f, vr + 120.0f * glowW);
                vg = std::min(255.0f, vg + 40.0f * glowW);
            }

            uint8_t outR = static_cast<uint8_t>(std::clamp(vr, 0.0f, 255.0f));
            uint8_t outG = static_cast<uint8_t>(std::clamp(vg, 0.0f, 255.0f));
            uint8_t outB = static_cast<uint8_t>(std::clamp(vb, 0.0f, 255.0f));
            m_framebuffer[frameY * m_width + x] = (0xFFu << 24) | (outB << 16) | (outG << 8) | outR;
        }
    }

    // 3. Caldera / Lago de lava en la cima del cráter
    int calX = static_cast<int>(cx);
    int calY = summitY;
    int cRx = static_cast<int>(calW);
    int cRy = static_cast<int>(calH);

    for (int oy = -cRy; oy <= cRy; ++oy) {
        int py = calY + oy;
        int frameY = m_height - 1 - py;
        if (frameY < 0 || frameY >= m_height) continue;

        for (int ox = -cRx; ox <= cRx; ++ox) {
            int px = calX + ox;
            if (px < 0 || px >= m_width) continue;

            float elDist = (ox * ox) / static_cast<float>(cRx * cRx) +
                           (oy * oy) / static_cast<float>(cRy * cRy);

            if (elDist <= 1.0f) {
                if (elDist > 0.72f) {
                    m_framebuffer[frameY * m_width + px] = (0xFFu << 24) | (25u << 16) | (22u << 8) | 35u;
                } else {
                    float lakePulse = 0.5f + 0.5f * std::sin(animTime * 5.0f + ox * 0.4f + oy * 0.4f);
                    float bubble = std::sin(ox * 0.8f + animTime * 4.0f) * std::cos(oy * 0.8f - animTime * 3.0f);
                    uint8_t mr = 255;
                    uint8_t mg = static_cast<uint8_t>(130.0f + 85.0f * lakePulse + (bubble > 0.4f ? 40.0f : 0.0f));
                    uint8_t mb = static_cast<uint8_t>(25.0f + (bubble > 0.4f ? 80.0f : 0.0f));
                    m_framebuffer[frameY * m_width + px] = (0xFFu << 24) | (mb << 16) | (mg << 8) | mr;
                }
            }
        }
    }

    // 4. Volutas de humo y fumarolas en la cumbre
    for (int sm = 0; sm < 4; ++sm) {
        float smTime = animTime * 1.8f + sm * 1.57f;
        float smRise = std::fmod(smTime * 18.0f * scale, 35.0f * scale);
        float smDrift = std::sin(smTime * 1.2f) * 8.0f * scale;
        int smX = static_cast<int>(cx + smDrift);
        int smY = static_cast<int>(calY - smRise);
        int smR = static_cast<int>((4.0f + (smRise / (35.0f * scale)) * 8.0f) * scale);
        ColorRGB smCol = (sm % 2 == 0) ? ColorRGB{55, 45, 42} : ColorRGB{80, 70, 68};
        drawDiamondOutline(smX, smY, smR, smR * 0.5f, smCol);
    }
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

void Renderer::renderTargetingReticle(const Point2i& hoverCell, DivinePowerType power,
                                      const IsometricCamera& camera, const WorldMap& map) {
    if (!map.isValid(hoverCell.x, hoverCell.y) || power == DivinePowerType::Inactive) return;

    float zoom = camera.getZoom();
    float halfW = BASE_TILE_WIDTH * 0.5f * zoom;
    float halfH = BASE_TILE_HEIGHT * 0.5f * zoom;

    ColorRGB col = ColorRGB{255, 235, 120};
    if (power == DivinePowerType::Lightning) col = ColorRGB{120, 220, 255};
    else if (power == DivinePowerType::Plague) col = ColorRGB{140, 225, 70};
    else if (power == DivinePowerType::Miracle) col = ColorRGB{255, 230, 90};
    else if (power == DivinePowerType::Tornado) col = ColorRGB{180, 205, 230};
    else if (power == DivinePowerType::Earthquake) col = ColorRGB{230, 140, 70};
    else if (power == DivinePowerType::Pandemic) col = ColorRGB{120, 230, 80};
    else if (power == DivinePowerType::Meteor) col = ColorRGB{255, 95, 25};
    else if (power == DivinePowerType::Volcano) col = ColorRGB{255, 50, 15};
    else if (power == DivinePowerType::Wildfire) col = ColorRGB{255, 110, 20};

    int rad = 1;
    if (power == DivinePowerType::Volcano || power == DivinePowerType::Meteor || power == DivinePowerType::Wildfire) rad = 4;
    else if (power == DivinePowerType::Earthquake) rad = 3;
    else if (power == DivinePowerType::Tornado || power == DivinePowerType::Pandemic) rad = 2;
    else if (power == DivinePowerType::Plague || power == DivinePowerType::Miracle) rad = 1;
    else rad = 0; // Lightning = 1 celda

    if (rad == 0) {
        float elev = map.getCell(hoverCell.x, hoverCell.y).elevation;
        Vec2 pt = camera.gridTileCenterScreen(hoverCell.x, hoverCell.y, elev);
        drawDiamondOutline(pt.x, pt.y, halfW, halfH, col);
        drawDiamondOutline(pt.x, pt.y, halfW + 1, halfH + 1, ColorRGB{255, 255, 255});
        drawDiamondOutline(pt.x, pt.y, halfW * 0.5f, halfH * 0.5f, col);
    } else {
        for (int dy = -rad; dy <= rad; ++dy) {
            for (int dx = -rad; dx <= rad; ++dx) {
                if (dx * dx + dy * dy > (rad * rad + 1)) continue;
                int cx = hoverCell.x + dx;
                int cy = hoverCell.y + dy;
                if (!map.isValid(cx, cy)) continue;
                float elev = map.getCell(cx, cy).elevation;
                Vec2 pt = camera.gridTileCenterScreen(cx, cy, elev);
                drawDiamondOutline(pt.x, pt.y, halfW, halfH, col);
            }
        }
    }
}

void Renderer::renderDivineEffects(const DivinePowersSystem& powers,
                                   const IsometricCamera& camera, const WorldMap& map) {
    float zoom = camera.getZoom();
    for (const auto& fx : powers.getActiveFX()) {
        if (!map.isValid(fx.gridX, fx.gridY)) continue;
        float elev = map.getCell(fx.gridX, fx.gridY).elevation;
        Vec2 sp = camera.gridTileCenterScreen(fx.gridX, fx.gridY, elev);
        float t = std::clamp(fx.timer / std::max(0.01f, fx.duration), 0.0f, 1.0f);

        if (fx.type == DivinePowerType::Lightning) {
            // Relámpago en zigzag desde el cielo hasta el punto de impacto
            int startX = static_cast<int>(sp.x + std::sin(fx.seed * 3.14f) * 15.0f);
            int startY = 0;
            int targetX = static_cast<int>(sp.x);
            int targetY = static_cast<int>(sp.y);

            int segs = 7;
            int prevX = startX;
            int prevY = startY;
            for (int s = 1; s <= segs; ++s) {
                float segT = static_cast<float>(s) / segs;
                int nextX = static_cast<int>(startX + (targetX - startX) * segT);
                int nextY = static_cast<int>(startY + (targetY - startY) * segT);
                if (s < segs) {
                    float jitter = std::sin(fx.seed + s * 1.7f) * 18.0f * (1.0f - segT * 0.3f);
                    nextX += static_cast<int>(jitter);
                }
                drawLine(prevX, prevY, nextX, nextY, ColorRGB{255, 255, 255});
                drawLine(prevX + 1, prevY, nextX + 1, nextY, ColorRGB{120, 220, 255});
                drawLine(prevX - 1, prevY, nextX - 1, nextY, ColorRGB{120, 220, 255});
                prevX = nextX;
                prevY = nextY;
            }

            // Destello expansivo e impacto en el suelo
            float shockR = (12.0f + 28.0f * t) * zoom;
            drawDiamondOutline(sp.x, sp.y, shockR, shockR * 0.5f, ColorRGB{255, 255, 255});
            drawDiamondOutline(sp.x, sp.y, shockR * 0.7f, shockR * 0.35f, ColorRGB{120, 220, 255});

            // Texto flotante de impacto
            int ty = static_cast<int>(sp.y - 32.0f - t * 24.0f);
            drawText(static_cast<int>(sp.x - 45), ty, "¡RAYO DIVINO!", ColorRGB{255, 245, 140}, 1);
            if (fx.converts > 0) {
                drawText(static_cast<int>(sp.x - 55), ty + 12, "+" + std::to_string(fx.converts) + " CONVERTIDOS", ColorRGB{120, 255, 120}, 1);
            }
        } else if (fx.type == DivinePowerType::Plague) {
            // Nube de plaga verde y morada en espiral
            int particleCount = 18;
            for (int p = 0; p < particleCount; ++p) {
                float angle = fx.timer * 3.5f + static_cast<float>(p) * (6.283f / particleCount);
                float radius = (18.0f + 16.0f * std::sin(fx.seed + p * 1.3f)) * zoom;
                int px = static_cast<int>(sp.x + std::cos(angle) * radius);
                int py = static_cast<int>(sp.y + std::sin(angle) * (radius * 0.5f) - (fx.timer * 6.0f));
                ColorRGB pCol = (p % 2 == 0) ? ColorRGB{140, 225, 70} : ColorRGB{160, 90, 210};
                drawRect(px - 1, py - 1, 3, 3, pCol);
            }
            int ty = static_cast<int>(sp.y - 28.0f - (fx.timer * 8.0f));
            drawText(static_cast<int>(sp.x - 50), ty, "¡PLAGA SAGRADA!", ColorRGB{170, 240, 90}, 1);
            if (fx.converts > 0) {
                drawText(static_cast<int>(sp.x - 55), ty + 12, "+" + std::to_string(fx.converts) + " ARREPENTIDOS", ColorRGB{120, 255, 120}, 1);
            }
        } else if (fx.type == DivinePowerType::Miracle) {
            // Haces de luz celestial y estrellas doradas ascendentes
            for (int b = -1; b <= 1; ++b) {
                int bx = static_cast<int>(sp.x + b * 24.0f * zoom);
                int byTop = 0;
                int byBot = static_cast<int>(sp.y);
                int halfW = static_cast<int>(5.0f * zoom);
                for (int y = byTop; y <= byBot; ++y) {
                    int fy = m_height - 1 - y;
                    if (fy < 0 || fy >= m_height) continue;
                    float fade = static_cast<float>(y) / std::max(1, byBot);
                    for (int x = bx - halfW; x <= bx + halfW; ++x) {
                        if (x < 0 || x >= m_width) continue;
                        uint32_t bg = m_framebuffer[fy * m_width + x];
                        uint8_t br = bg & 0xFF;
                        uint8_t bgc = (bg >> 8) & 0xFF;
                        uint8_t bb = (bg >> 16) & 0xFF;
                        float alpha = 0.14f * fade;
                        uint8_t outR = static_cast<uint8_t>(255.0f * alpha + br * (1.0f - alpha));
                        uint8_t outG = static_cast<uint8_t>(235.0f * alpha + bgc * (1.0f - alpha));
                        uint8_t outB = static_cast<uint8_t>(100.0f * alpha + bb * (1.0f - alpha));
                        m_framebuffer[fy * m_width + x] = (0xFFu << 24) | (outB << 16) | (outG << 8) | outR;
                    }
                }
            }

            // Estrellas / partículas sagradas ascendentes
            for (int p = 0; p < 12; ++p) {
                float rise = std::fmod(fx.timer * 25.0f + p * 8.0f, 50.0f);
                int sx = static_cast<int>(sp.x + std::sin(p * 2.1f + fx.seed) * 26.0f * zoom);
                int sy = static_cast<int>(sp.y - rise);
                drawRect(sx - 1, sy - 1, 3, 3, Palette::DIVINE_GOLD);
                drawRect(sx, sy, 1, 1, ColorRGB{255, 255, 255});
            }

            int ty = static_cast<int>(sp.y - 32.0f - (fx.timer * 8.0f));
            drawText(static_cast<int>(sp.x - 55), ty, "¡MILAGRO CELESTIAL!", ColorRGB{255, 235, 120}, 1);
            if (fx.converts > 0) {
                drawText(static_cast<int>(sp.x - 50), ty + 12, "+" + std::to_string(fx.converts) + " CONVERTIDOS", ColorRGB{120, 255, 120}, 1);
            }
        } else if (fx.type == DivinePowerType::Tornado) {
            // Embudo ciclónico procedural 2.5D en rotación continua
            float elev = map.isValid(fx.gridX, fx.gridY) ? map.getCell(fx.gridX, fx.gridY).elevation : 1.0f;
            Vec2 center = camera.gridToScreen(fx.worldX, fx.worldY, elev);

            // Sombra elíptica y vórtice de polvo en el suelo
            float baseR = (16.0f + std::sin(fx.timer * 8.0f) * 2.5f) * zoom;
            drawDiamondOutline(center.x, center.y, baseR, baseR * 0.5f, ColorRGB{55, 45, 35});
            drawDiamondOutline(center.x, center.y, baseR * 1.5f, baseR * 0.75f, ColorRGB{70, 60, 50});

            // Partículas de polvo y escombros orbitando el embudo en espiral
            int particleCount = 48;
            float funnelH = 135.0f * zoom;
            for (int p = 0; p < particleCount; ++p) {
                float frac = static_cast<float>(p) / particleCount;
                float angle = fx.timer * (7.5f + (1.0f - frac) * 5.0f) + p * 0.65f;
                float height = frac * funnelH;
                float radius = (12.0f + frac * 48.0f) * zoom;

                int px = static_cast<int>(center.x + std::cos(angle) * radius);
                int py = static_cast<int>(center.y - height + std::sin(angle) * (radius * 0.40f));

                ColorRGB pCol;
                if (p % 4 == 0) pCol = ColorRGB{240, 245, 255}; // Nube blanca de condensación
                else if (p % 3 == 0) pCol = ColorRGB{140, 120, 95}; // Polvo y escombros de tierra
                else if (p % 2 == 0) pCol = ColorRGB{75, 80, 90}; // Gris tormenta
                else pCol = ColorRGB{35, 38, 45}; // Basalto de sombra ciclónica

                int pSize = (frac > 0.6f) ? 3 : 2;
                drawRect(px - pSize / 2, py - pSize / 2, pSize, pSize, pCol);
            }

            int ty = static_cast<int>(center.y - funnelH - 18.0f);
            drawText(static_cast<int>(center.x - 55), ty, "¡TORNADO DEVASTADOR!", ColorRGB{200, 225, 255}, 1);

        } else if (fx.type == DivinePowerType::Earthquake) {
            float elev = map.isValid(fx.gridX, fx.gridY) ? map.getCell(fx.gridX, fx.gridY).elevation : 1.0f;
            Vec2 center = camera.gridToScreen(fx.worldX, fx.worldY, elev);

            // Ondas concéntricas sísmicas propagándose en el suelo
            for (int r = 0; r < 4; ++r) {
                float waveProgress = std::fmod(fx.timer * 2.2f + r * 0.25f, 1.0f);
                float radius = waveProgress * 65.0f * zoom;
                ColorRGB waveCol{
                    static_cast<uint8_t>(220 * (1.0f - waveProgress)),
                    static_cast<uint8_t>(140 * (1.0f - waveProgress)),
                    static_cast<uint8_t>(70 * (1.0f - waveProgress))
                };
                drawDiamondOutline(center.x, center.y, radius, radius * 0.5f, waveCol);
            }

            // Fisuras tectónicas fracturadas en el epicentro
            for (int i = 0; i < 6; ++i) {
                float fAngle = i * 1.047f + fx.seed;
                int crackLen = static_cast<int>((16.0f + std::sin(fx.seed + i) * 8.0f) * zoom);
                int ex = static_cast<int>(center.x + std::cos(fAngle) * crackLen);
                int ey = static_cast<int>(center.y + std::sin(fAngle) * (crackLen * 0.5f));
                drawLine(static_cast<int>(center.x), static_cast<int>(center.y), ex, ey, ColorRGB{42, 28, 20});
                drawRect(ex - 1, ey - 1, 3, 3, ColorRGB{195, 120, 50});
            }

            int ty = static_cast<int>(center.y - 36.0f * zoom);
            drawText(static_cast<int>(center.x - 52), ty, "¡TERREMOTO SÍSMICO!", ColorRGB{255, 170, 80}, 1);

        } else if (fx.type == DivinePowerType::Pandemic) {
            float elev = map.isValid(fx.gridX, fx.gridY) ? map.getCell(fx.gridX, fx.gridY).elevation : 1.0f;
            Vec2 center = camera.gridToScreen(fx.worldX, fx.worldY, elev);

            // Domo de miasma tóxico y vapores pestilentes
            int sporeCount = 32;
            for (int p = 0; p < sporeCount; ++p) {
                float angle = fx.timer * 1.8f + p * 0.785f;
                float rad = (14.0f + std::sin(fx.timer * 3.0f + p * 1.4f) * 10.0f + (p % 5) * 8.0f) * zoom;
                int px = static_cast<int>(center.x + std::cos(angle) * rad);
                int py = static_cast<int>(center.y - (12.0f + (p % 4) * 10.0f) * zoom + std::sin(angle) * (rad * 0.40f));

                ColorRGB mCol = (p % 2 == 0) ? ColorRGB{120, 230, 60} : ColorRGB{80, 160, 40};
                drawRect(px - 1, py - 1, 3, 3, mCol);
            }

            // Anillo de peligro biohazard / cuarentena
            float pRing = (35.0f + std::sin(fx.timer * 4.0f) * 4.0f) * zoom;
            drawDiamondOutline(center.x, center.y, pRing, pRing * 0.5f, ColorRGB{130, 235, 70});
            drawDiamondOutline(center.x, center.y, pRing * 0.7f, pRing * 0.35f, ColorRGB{60, 120, 30});

            int ty = static_cast<int>(center.y - 42.0f * zoom);
            drawText(static_cast<int>(center.x - 48), ty, "¡PANDEMIA VIRAL!", ColorRGB{150, 245, 90}, 1);

        } else if (fx.type == DivinePowerType::Meteor) {
            float elev = map.isValid(fx.gridX, fx.gridY) ? map.getCell(fx.gridX, fx.gridY).elevation : 1.0f;
            Vec2 groundCenter = camera.gridToScreen(fx.worldX, fx.worldY, elev);
            const float IMPACT_TIME = 1.25f;

            if (fx.timer < IMPACT_TIME) {
                // ==========================================
                // FASE 1: DESCENSO HIPERSÓNICO DEL BÓLIDO
                // ==========================================
                float prog = fx.timer / IMPACT_TIME; // 0.0 -> 1.0
                float invProg = 1.0f - prog;

                // 1. Advertencia de impacto en tierra (sombra y runas ígneas)
                float warnRadius = (18.0f + prog * 45.0f) * zoom;
                float pulse = 0.8f + 0.2f * std::sin(fx.timer * 18.0f);
                ColorRGB warnCol{
                    static_cast<uint8_t>(255 * pulse),
                    static_cast<uint8_t>(90 * pulse),
                    static_cast<uint8_t>(20 * pulse)
                };
                drawDiamondOutline(groundCenter.x, groundCenter.y, warnRadius, warnRadius * 0.5f, warnCol);
                drawDiamondOutline(groundCenter.x, groundCenter.y, warnRadius * 0.6f, warnRadius * 0.3f, ColorRGB{255, 200, 50});
                drawDiamondOutline(groundCenter.x, groundCenter.y, warnRadius * 0.2f, warnRadius * 0.1f, ColorRGB{255, 255, 255});

                // 2. Coordenadas del bólido en el aire (trayectoria diagonal desde el cielo nororiental)
                float meteorX = groundCenter.x + invProg * 240.0f * zoom;
                float meteorY = groundCenter.y - invProg * 420.0f * zoom;

                // Estela incandescente y arco de compresión atmosférica supersónico
                for (int sk = 1; sk <= 3; ++sk) {
                    float skDist = sk * 10.0f * zoom;
                    float skX1 = meteorX - skDist * 0.7f;
                    float skY1 = meteorY + skDist * 1.2f;
                    float skX2 = meteorX + skDist * 1.3f;
                    float skY2 = meteorY + skDist * 0.6f;
                    ColorRGB bowCol = (sk == 1) ? ColorRGB{255, 255, 240} : ColorRGB{255, 180, 40};
                    drawLine(static_cast<int>(meteorX), static_cast<int>(meteorY), static_cast<int>(skX1), static_cast<int>(skY1), bowCol);
                    drawLine(static_cast<int>(meteorX), static_cast<int>(meteorY), static_cast<int>(skX2), static_cast<int>(skY2), bowCol);
                }

                int trailSegments = 30;
                for (int s = 0; s < trailSegments; ++s) {
                    float sFrac = static_cast<float>(s) / trailSegments;
                    float trailX = meteorX + sFrac * 195.0f * zoom;
                    float trailY = meteorY - sFrac * 340.0f * zoom;
                    float trailW = (6.0f + sFrac * 22.0f) * zoom;

                    float smokeWobble = std::sin(fx.timer * 14.0f + s * 0.8f) * 4.0f * zoom;
                    drawDiamondOutline(trailX + smokeWobble, trailY, trailW * 1.6f, trailW * 0.80f, ColorRGB{38, 26, 22});
                    drawDiamondOutline(trailX, trailY, trailW, trailW * 0.5f, ColorRGB{240, 55, 15});
                    drawDiamondOutline(trailX, trailY, trailW * 0.5f, trailW * 0.25f, ColorRGB{255, 245, 170});

                    if (s % 2 == 0) {
                        float sparkJitter = std::sin(fx.seed + s * 2.1f + fx.timer * 16.0f) * (14.0f + s * 0.8f) * zoom;
                        drawRect(static_cast<int>(trailX + sparkJitter), static_cast<int>(trailY), 3, 3, ColorRGB{255, 215, 60});
                        drawRect(static_cast<int>(trailX + sparkJitter + 1), static_cast<int>(trailY + 1), 1, 1, ColorRGB{255, 255, 255});
                    }
                }

                // Cabeza del bólido (núcleo rocoso fundido y frente de choque incandescente)
                int mSize = static_cast<int>((14.0f + prog * 5.0f) * zoom);
                drawDiamondOutline(meteorX, meteorY, mSize * 2.2f, mSize * 1.1f, ColorRGB{255, 110, 15});
                drawDiamondOutline(meteorX, meteorY, mSize * 1.6f, mSize * 0.8f, ColorRGB{255, 210, 45});
                drawRect(static_cast<int>(meteorX - mSize / 2), static_cast<int>(meteorY - mSize / 2), mSize, mSize, ColorRGB{48, 26, 22});
                drawRect(static_cast<int>(meteorX - mSize / 3), static_cast<int>(meteorY - mSize / 3), mSize * 2 / 3, mSize * 2 / 3, ColorRGB{255, 255, 250});

                int ty = static_cast<int>(groundCenter.y - 28.0f * zoom);
                drawText(static_cast<int>(groundCenter.x - 52), ty, "¡METEORITO INMINENTE!", ColorRGB{255, 80, 20}, 1);

            } else {
                // ==========================================
                // FASE 2: DETONACIÓN, ONDA DE CHOQUE Y CRÁTER
                // ==========================================
                float tExp = fx.timer - IMPACT_TIME;
                float expProg = std::clamp(tExp / (fx.duration - IMPACT_TIME), 0.0f, 1.0f);
                float invExp = 1.0f - expProg;

                // 1. Ondas de choque expansivas en el suelo
                for (int w = 0; w < 3; ++w) {
                    float waveProg = std::fmod(tExp * 1.8f + w * 0.33f, 1.0f);
                    float waveRadius = waveProg * 140.0f * zoom;
                    uint8_t alphaFade = static_cast<uint8_t>(255 * (1.0f - waveProg) * invExp);
                    ColorRGB waveCol{alphaFade, static_cast<uint8_t>(alphaFade * 0.6f), static_cast<uint8_t>(alphaFade * 0.2f)};
                    drawDiamondOutline(groundCenter.x, groundCenter.y, waveRadius, waveRadius * 0.5f, waveCol);
                }

                // 2. Cráter humeante en el suelo
                float craterR = 36.0f * zoom;
                drawDiamondOutline(groundCenter.x, groundCenter.y, craterR, craterR * 0.5f, ColorRGB{28, 16, 14});
                drawDiamondOutline(groundCenter.x, groundCenter.y, craterR * 0.75f, craterR * 0.38f, ColorRGB{42, 24, 20});
                drawDiamondOutline(groundCenter.x, groundCenter.y, craterR * 0.45f, craterR * 0.22f, ColorRGB{200, 75, 20}); // Magma central

                // Fisuras ardientes que irradian del cráter
                for (int f = 0; f < 8; ++f) {
                    float fAng = f * 0.785f + fx.seed;
                    int fLen = static_cast<int>((20.0f + std::sin(fx.seed + f * 1.5f) * 12.0f) * zoom);
                    int fxX = static_cast<int>(groundCenter.x + std::cos(fAng) * fLen);
                    int fxY = static_cast<int>(groundCenter.y + std::sin(fAng) * (fLen * 0.5f));
                    drawLine(static_cast<int>(groundCenter.x), static_cast<int>(groundCenter.y), fxX, fxY, ColorRGB{230, 95, 25});
                    drawRect(fxX - 1, fxY - 1, 3, 3, ColorRGB{255, 180, 50});
                }

                // 3. Eyección de magma y escombros volando en arcos parabólicos
                int debrisCount = 18;
                for (int d = 0; d < debrisCount; ++d) {
                    float dAng = d * (6.283f / debrisCount) + fx.seed;
                    float dSpeed = (45.0f + (d % 5) * 18.0f) * zoom;
                    float debDist = dSpeed * tExp;
                    float debHeight = std::max(0.0f, (60.0f * zoom * tExp - 45.0f * zoom * tExp * tExp));

                    int debX = static_cast<int>(groundCenter.x + std::cos(dAng) * debDist);
                    int debY = static_cast<int>(groundCenter.y + std::sin(dAng) * (debDist * 0.5f) - debHeight);

                    ColorRGB debCol = (d % 2 == 0) ? ColorRGB{255, 140, 30} : ColorRGB{80, 40, 30};
                    drawRect(debX - 1, debY - 1, 3, 3, debCol);
                }

                // 4. Columna de humo negro y ascuas ascendentes
                int smokePuffs = 20;
                for (int p = 0; p < smokePuffs; ++p) {
                    float pTime = tExp + p * 0.15f;
                    float pHeight = std::fmod(pTime * 28.0f, 90.0f) * zoom;
                    float pDrift = std::sin(fx.seed + p * 1.2f + tExp * 2.0f) * (14.0f + p * 1.5f) * zoom;
                    int sx = static_cast<int>(groundCenter.x + pDrift);
                    int sy = static_cast<int>(groundCenter.y - pHeight);
                    int sSize = static_cast<int>((4.0f + (pHeight / (90.0f * zoom)) * 10.0f));

                    ColorRGB sCol = (p % 3 == 0) ? ColorRGB{255, 110, 20} : ColorRGB{45, 38, 35};
                    drawRect(sx - sSize / 2, sy - sSize / 2, sSize, sSize, sCol);
                }

                // 5. Bola de fuego de detonación inicial (primeros 0.6s post impacto)
                if (tExp < 0.6f) {
                    float blastProg = tExp / 0.6f;
                    float blastR = blastProg * 75.0f * zoom;
                    ColorRGB fireCol{255, static_cast<uint8_t>(255 * (1.0f - blastProg)), static_cast<uint8_t>(100 * (1.0f - blastProg))};
                    drawDiamondOutline(groundCenter.x, groundCenter.y, blastR, blastR * 0.5f, fireCol);
                    drawDiamondOutline(groundCenter.x, groundCenter.y, blastR * 0.7f, blastR * 0.35f, ColorRGB{255, 255, 255});
                }

                int ty = static_cast<int>(groundCenter.y - 48.0f * zoom);
                drawText(static_cast<int>(groundCenter.x - 58), ty, "¡METEORITO CATACLÍSMICO!", ColorRGB{255, 160, 40}, 1);
            }
        } else if (fx.type == DivinePowerType::Volcano) {
            // =========================================================================
            // CATÁSTROFE 5: ERUPCIÓN VOLCÁNICA (Fuentes de Magma, Bombas y Nube Pliniana)
            // =========================================================================
            float elev = map.isValid(fx.gridX, fx.gridY) ? map.getCell(fx.gridX, fx.gridY).elevation : 1.0f;
            Vec2 groundCenter = camera.gridTileCenterScreen(fx.worldX, fx.worldY, elev);
            float volcanoH = 68.0f * zoom;
            Vec2 summit = {groundCenter.x, groundCenter.y - volcanoH};

            // 1. Ondas de temblor sísmico y agrietamiento del terreno en la base
            for (int w = 0; w < 3; ++w) {
                float waveProg = std::fmod(fx.timer * 1.5f + w * 0.33f, 1.0f);
                float waveRadius = (45.0f + waveProg * 75.0f) * zoom;
                uint8_t waveAlpha = static_cast<uint8_t>(220 * (1.0f - waveProg));
                ColorRGB waveCol{waveAlpha, static_cast<uint8_t>(waveAlpha * 0.45f), static_cast<uint8_t>(waveAlpha * 0.15f)};
                drawDiamondOutline(groundCenter.x, groundCenter.y, waveRadius, waveRadius * 0.5f, waveCol);
            }

            // 2. Colosal columna pliniana de tefra y cenizas volcánicas con relámpagos internos
            int ashPuffs = 28;
            for (int a = 0; a < ashPuffs; ++a) {
                float aProg = std::fmod(fx.timer * 22.0f + a * 6.5f, 160.0f);
                float aFrac = aProg / 160.0f;
                float aSpread = (8.0f + aFrac * 42.0f) * zoom;
                float aDrift = std::sin(fx.seed + a * 1.1f + fx.timer * 1.2f) * aSpread;
                int ax = static_cast<int>(summit.x + aDrift);
                int ay = static_cast<int>(summit.y - aProg * zoom);
                int aSize = static_cast<int>((6.0f + aFrac * 22.0f) * zoom);

                ColorRGB ashCol;
                if (aFrac < 0.22f) {
                    ashCol = ColorRGB{255, 110, 20};
                } else if (aFrac < 0.45f) {
                    ashCol = ColorRGB{90, 50, 40};
                } else {
                    ashCol = (a % 2 == 0) ? ColorRGB{32, 26, 28} : ColorRGB{52, 44, 46};
                }
                drawDiamondOutline(ax, ay, aSize, aSize * 0.6f, ashCol);
                drawRect(ax - aSize / 4, ay - aSize / 4, std::max(2, aSize / 2), std::max(2, aSize / 2), ashCol);
            }

            // Relámpagos volcánicos intra-nube
            if (static_cast<int>(fx.timer * 7.0f) % 5 == 0) {
                float lStartY = summit.y - (40.0f + std::sin(fx.timer * 19.0f) * 20.0f) * zoom;
                float lEndY = lStartY - 35.0f * zoom;
                float lX = summit.x + std::sin(fx.seed + fx.timer * 13.0f) * 18.0f * zoom;
                drawLine(static_cast<int>(lX), static_cast<int>(lStartY),
                         static_cast<int>(lX + 8.0f * zoom), static_cast<int>((lStartY + lEndY) * 0.5f),
                         ColorRGB{240, 245, 255});
                drawLine(static_cast<int>(lX + 8.0f * zoom), static_cast<int>((lStartY + lEndY) * 0.5f),
                         static_cast<int>(lX - 4.0f * zoom), static_cast<int>(lEndY),
                         ColorRGB{200, 220, 255});
            }

            // 3. Fuentes de magma incandescente eyectadas continuamente desde la caldera
            int fontJets = 16;
            for (int j = 0; j < fontJets; ++j) {
                float jCycle = std::fmod(fx.timer * 3.5f + j * 0.45f, 1.0f);
                float jAngle = j * (6.283f / fontJets) + fx.seed;
                float jHeight = (45.0f + std::sin(fx.seed + j * 2.3f) * 20.0f) * (1.0f - (jCycle - 0.5f) * (jCycle - 0.5f) * 4.0f) * zoom;
                float jDist = jCycle * 22.0f * zoom;
                int jx = static_cast<int>(summit.x + std::cos(jAngle) * jDist);
                int jy = static_cast<int>(summit.y + std::sin(jAngle) * (jDist * 0.45f) - jHeight);

                ColorRGB fCol = (j % 3 == 0) ? ColorRGB{255, 240, 160} : ColorRGB{255, 120, 25};
                drawRect(jx - 1, jy - 1, 3, 3, fCol);
                drawRect(jx, jy, 1, 1, ColorRGB{255, 255, 255});
            }

            // 4. Bombas volcánicas balísticas lanzadas en trayectorias parabólicas
            int bombCount = 12;
            for (int b = 0; b < bombCount; ++b) {
                float bCycle = std::fmod(fx.timer * 0.65f + b * 0.35f, 1.8f);
                float bFrac = bCycle / 1.8f;
                float bAng = b * 0.523f + fx.seed;
                float bSpeed = (40.0f + (b % 4) * 18.0f) * zoom;
                float bDist = bSpeed * bCycle;
                float bHeight = std::max(0.0f, (95.0f * bCycle - 52.0f * bCycle * bCycle) * zoom);

                int bx = static_cast<int>(summit.x + std::cos(bAng) * bDist);
                int by = static_cast<int>(summit.y + std::sin(bAng) * (bDist * 0.45f) - bHeight);

                int bSize = 4;
                drawRect(bx - bSize / 2, by - bSize / 2, bSize, bSize, ColorRGB{255, 95, 20});
                drawRect(bx - 1, by - 1, 2, 2, ColorRGB{255, 240, 140});

                if (bFrac > 0.88f) {
                    float impProg = (bFrac - 0.88f) / 0.12f;
                    float splashR = impProg * 14.0f * zoom;
                    drawDiamondOutline(bx, by, splashR, splashR * 0.5f, ColorRGB{255, 140, 30});
                    drawRect(bx - 1, by - 1, 3, 3, ColorRGB{255, 220, 80});
                }
            }

            int ty = static_cast<int>(summit.y - 30.0f * zoom);
            drawText(static_cast<int>(summit.x - 62), ty, "¡ERUPCIÓN VOLCÁNICA COLOSAL!", ColorRGB{255, 65, 18}, 1);
        } else if (fx.type == DivinePowerType::Wildfire) {
            // =========================================================================
            // CATÁSTROFE 6: INCENDIO FORESTAL (Llamaradas, Chispas, Ascuas y Humo Calcinante)
            // =========================================================================
            float elev = map.isValid(fx.gridX, fx.gridY) ? map.getCell(fx.gridX, fx.gridY).elevation : 1.0f;
            Vec2 groundCenter = camera.gridTileCenterScreen(fx.worldX, fx.worldY, elev);

            // 1. Frente de fuego en expansión: ondas concéntricas de fuego devorador
            float pulse = 1.0f + 0.18f * std::sin(fx.timer * 9.0f + fx.seed);
            float fireR = (fx.radius * 22.0f * pulse) * zoom;
            drawDiamondOutline(groundCenter.x, groundCenter.y, fireR, fireR * 0.5f, ColorRGB{255, 90, 20});
            drawDiamondOutline(groundCenter.x, groundCenter.y, fireR * 0.75f, fireR * 0.375f, ColorRGB{255, 170, 30});
            drawDiamondOutline(groundCenter.x, groundCenter.y, fireR * 0.4f, fireR * 0.2f, ColorRGB{255, 240, 110});

            // 2. Columna de llamas y ascuas ardientes en suspensión
            int flamePuffs = 28;
            for (int p = 0; p < flamePuffs; ++p) {
                float pTime = fx.timer * 1.8f + p * 0.22f;
                float pHeight = std::fmod(pTime * 34.0f, 75.0f) * zoom;
                float pDrift = std::sin(fx.seed + p * 1.4f + fx.timer * 3.5f) * (18.0f + p * 1.2f) * zoom;
                int fxPos = static_cast<int>(groundCenter.x + pDrift);
                int fyPos = static_cast<int>(groundCenter.y - pHeight);
                int pSize = static_cast<int>((3.0f + (1.0f - pHeight / (75.0f * zoom)) * 6.0f));

                ColorRGB fCol;
                if (p % 4 == 0) fCol = ColorRGB{255, 245, 150}; // Núcleo amarillo brillante
                else if (p % 2 == 0) fCol = ColorRGB{255, 120, 20}; // Llama naranja
                else fCol = ColorRGB{220, 45, 10}; // Ascua rojiza

                drawRect(fxPos - pSize / 2, fyPos - pSize / 2, pSize, pSize, fCol);
                drawRect(fxPos, fyPos, 1, 1, ColorRGB{255, 255, 255});
            }

            // 3. Nubes de humo denso calcinante ascendente
            int smokeCount = 14;
            for (int s = 0; s < smokeCount; ++s) {
                float sTime = fx.timer * 1.1f + s * 0.45f;
                float sHeight = (40.0f + std::fmod(sTime * 26.0f, 95.0f)) * zoom;
                float sDrift = std::cos(fx.seed * 0.7f + s * 1.8f + fx.timer * 1.5f) * (26.0f + s * 2.2f) * zoom;
                int sx = static_cast<int>(groundCenter.x + sDrift);
                int sy = static_cast<int>(groundCenter.y - sHeight);
                int sSize = static_cast<int>((8.0f + (sHeight / (135.0f * zoom)) * 14.0f));

                drawRect(sx - sSize / 2, sy - sSize / 2, sSize, sSize, ColorRGB{48, 42, 38});
            }

            int ty = static_cast<int>(groundCenter.y - 42.0f * zoom);
            drawText(static_cast<int>(groundCenter.x - 56), ty, "¡INCENDIO FORESTAL SAGRADO!", ColorRGB{255, 120, 25}, 1);
        }
    }
}

void Renderer::renderPreacherSpeeches(const DivinePowersSystem& powers,
                                      const IsometricCamera& camera, const WorldMap& map) {
    for (const auto& sp : powers.getActiveSpeeches()) {
        int gx = std::clamp(static_cast<int>(sp.worldX), 0, map.getWidth() - 1);
        int gy = std::clamp(static_cast<int>(sp.worldY), 0, map.getHeight() - 1);
        float elev = map.getCell(gx, gy).elevation;
        Vec2 foot = camera.gridToScreen(sp.worldX, sp.worldY, elev);

        int bw = 340;
        int bh = 46;
        int bx = static_cast<int>(foot.x - bw * 0.5f);
        int by = static_cast<int>(foot.y - 68.0f * camera.getZoom());
        bx = std::clamp(bx, 10, m_width - bw - 10);
        by = std::clamp(by, 45, m_height - 130);

        // Bocadillo ceremonial estilo Sims 2 con halo dorado
        drawSims2Panel(bx, by, bw, bh, ColorRGB{24, 16, 12}, false, Palette::DIVINE_GOLD);
        drawSims2Bar(bx + 2, by + 2, 3, bh - 4, Palette::DIVINE_GOLD);

        // Título del orador (CHAMÁN o PROFETA)
        std::string header = "[" + sp.title + " " + sp.name + " PREDICA]:";
        drawText(bx + 10, by + 6, header, ColorRGB{255, 235, 120}, 1);

        // Texto del sermón en DejaVu Sans Mono
        std::string line1 = sp.speechText.substr(0, 52);
        std::string line2 = (sp.speechText.size() > 52) ? sp.speechText.substr(52, 52) : "";
        drawText(bx + 10, by + 19, "\"" + line1, Palette::RELIC_BONE, 1);
        if (!line2.empty()) {
            drawText(bx + 10, by + 30, line2 + "\"", Palette::RELIC_BONE, 1);
        } else {
            drawText(bx + 10 + static_cast<int>((line1.size() + 1) * 6), by + 19, "\"", Palette::RELIC_BONE, 1);
        }
    }
}

void Renderer::renderCloudShadows(const CloudSystem& clouds, const IsometricCamera& camera, const DayNightCycle* dayNight, int mapWidth, int mapHeight) {
    float zoomVis = CloudSystem::calculateZoomVisibility(camera.getZoom());
    if (zoomVis <= 0.005f) return;

    ColorRGB sunlitCol, shadedCol, shadowCol;
    float shadowBaseAlpha = 0.32f;
    clouds.getLightingColors(dayNight, sunlitCol, shadedCol, shadowCol, shadowBaseAlpha);

    float zoom = camera.getZoom();
    float halfW = BASE_TILE_WIDTH * 0.5f;
    float halfH = BASE_TILE_HEIGHT * 0.5f;

    // Desplazamiento de proyección de sombra según ángulo del sol
    float hour = dayNight ? dayNight->getHour() : 12.0f;
    float sunDirX = std::cos((hour - 6.0f) * 0.261799f);
    float shadowOffsetGx = -sunDirX * 1.8f;
    float shadowOffsetGy = 1.2f;

    float effectiveShadowAlpha = shadowBaseAlpha * zoomVis;
    if (effectiveShadowAlpha <= 0.01f) return;

    for (const auto& cloud : clouds.getClouds()) {
        if (cloud.worldX < 0.0f || cloud.worldX >= static_cast<float>(mapWidth) ||
            cloud.worldY < 0.0f || cloud.worldY >= static_cast<float>(mapHeight)) {
            continue;
        }

        Vec2 shadowCenter = camera.gridToScreen(cloud.worldX + shadowOffsetGx,
                                                cloud.worldY + shadowOffsetGy,
                                                0.0f);

        float maxScreenR = 6.0f * halfW * zoom * cloud.scale;
        if (shadowCenter.x < -maxScreenR || shadowCenter.x > m_width + maxScreenR ||
            shadowCenter.y < -maxScreenR || shadowCenter.y > m_height + maxScreenR) {
            continue;
        }

        for (const auto& lobe : cloud.lobes) {
            float lobeSx = shadowCenter.x + (lobe.offsetX - lobe.offsetY) * halfW * zoom;
            float lobeSy = shadowCenter.y + (lobe.offsetX + lobe.offsetY) * halfH * zoom;
            float radX = lobe.radiusCells * halfW * zoom * cloud.scale * lobe.aspectX;
            float radY = lobe.radiusCells * halfH * zoom * cloud.scale * lobe.aspectY;

            if (radX < 2.0f || radY < 2.0f) continue;

            int minPx = std::max(0, static_cast<int>(std::floor(lobeSx - radX)));
            int maxPx = std::min(m_width - 1, static_cast<int>(std::ceil(lobeSx + radX)));
            int minPy = std::max(0, static_cast<int>(std::floor(lobeSy - radY)));
            int maxPy = std::min(m_height - 1, static_cast<int>(std::ceil(lobeSy + radY)));

            float invRadX = 1.0f / radX;
            float invRadY = 1.0f / radY;

            for (int py = minPy; py <= maxPy; ++py) {
                float dy = (static_cast<float>(py) - lobeSy) * invRadY;
                float dySq = dy * dy;
                if (dySq >= 1.0f) continue;
                int frameY = m_height - 1 - py;

                for (int px = minPx; px <= maxPx; ++px) {
                    float dx = (static_cast<float>(px) - lobeSx) * invRadX;
                    float distSq = dx * dx + dySq;
                    if (distSq >= 1.0f) continue;

                    // Comprobación geométrica: pixel de sombra debe caer dentro del mapa
                    int gx = 0, gy = 0;
                    camera.screenToGrid(static_cast<float>(px), static_cast<float>(py), gx, gy);
                    if (gx < 0 || gx >= mapWidth || gy < 0 || gy >= mapHeight) continue;

                    uint32_t pix = m_framebuffer[frameY * m_width + px];
                    if (((pix >> 24) & 0xFF) != 0xFF) continue; // Solo sombrear terreno/unidades

                    float edgeX = std::min(static_cast<float>(gx), static_cast<float>(mapWidth - 1 - gx));
                    float edgeY = std::min(static_cast<float>(gy), static_cast<float>(mapHeight - 1 - gy));
                    float borderFade = std::clamp(std::min(edgeX, edgeY) / 1.5f, 0.0f, 1.0f);

                    float dist = std::sqrt(distSq);
                    float edgeFade = (1.0f - dist * dist) * (1.0f - dist);
                    float finalShadeAlpha = edgeFade * lobe.density * cloud.baseAlpha * effectiveShadowAlpha * borderFade;

                    uint8_t r = pix & 0xFF;
                    uint8_t g = (pix >> 8) & 0xFF;
                    uint8_t b = (pix >> 16) & 0xFF;

                    float mult = 1.0f - finalShadeAlpha;
                    uint8_t outR = static_cast<uint8_t>(r * mult + shadowCol.r * (1.0f - mult) * 0.35f);
                    uint8_t outG = static_cast<uint8_t>(g * mult + shadowCol.g * (1.0f - mult) * 0.35f);
                    uint8_t outB = static_cast<uint8_t>(b * mult + shadowCol.b * (1.0f - mult) * 0.35f);

                    m_framebuffer[frameY * m_width + px] = (0xFFu << 24) | (static_cast<uint32_t>(outB) << 16) | (static_cast<uint32_t>(outG) << 8) | static_cast<uint32_t>(outR);
                }
            }
        }
    }
}

void Renderer::renderClouds(const CloudSystem& clouds, const IsometricCamera& camera, const DayNightCycle* dayNight, int mapWidth, int mapHeight) {
    float zoomVis = CloudSystem::calculateZoomVisibility(camera.getZoom());
    if (zoomVis <= 0.005f) return; // Zoom In: nubes 100% invisibles ("cuando hago zoom in ya no")

    ColorRGB sunlitCol, shadedCol, shadowCol;
    float shadowBaseAlpha = 0.32f;
    clouds.getLightingColors(dayNight, sunlitCol, shadedCol, shadowCol, shadowBaseAlpha);

    float zoom = camera.getZoom();
    float halfW = BASE_TILE_WIDTH * 0.5f;
    float halfH = BASE_TILE_HEIGHT * 0.5f;
    float altitudeScreenOffset = ELEVATION_STEP * zoom;

    for (const auto& cloud : clouds.getClouds()) {
        // Verificar que la nube esté confinada dentro del mapa
        if (cloud.worldX < 0.0f || cloud.worldX >= static_cast<float>(mapWidth) ||
            cloud.worldY < 0.0f || cloud.worldY >= static_cast<float>(mapHeight)) {
            continue;
        }

        // Posición proyectada con ALTURA DETERMINADA en 2.5D
        Vec2 cloudCenter = camera.gridToScreen(cloud.worldX, cloud.worldY, cloud.altitude);

        float maxScreenR = 7.0f * halfW * zoom * cloud.scale;
        if (cloudCenter.x < -maxScreenR || cloudCenter.x > m_width + maxScreenR ||
            cloudCenter.y < -maxScreenR || cloudCenter.y > m_height + maxScreenR) {
            continue;
        }

        float pulse = 1.0f + 0.04f * std::sin(cloud.pulseTimer);
        float cloudAltOffset = cloud.altitude * altitudeScreenOffset;

        for (const auto& lobe : cloud.lobes) {
            float lobeSx = cloudCenter.x + (lobe.offsetX - lobe.offsetY) * halfW * zoom;
            float lobeSy = cloudCenter.y + (lobe.offsetX + lobe.offsetY) * halfH * zoom;
            float radX = lobe.radiusCells * halfW * zoom * cloud.scale * lobe.aspectX * pulse;
            float radY = lobe.radiusCells * halfH * zoom * cloud.scale * lobe.aspectY * pulse;

            if (radX < 2.0f || radY < 2.0f) continue;

            int minPx = std::max(0, static_cast<int>(std::floor(lobeSx - radX)));
            int maxPx = std::min(m_width - 1, static_cast<int>(std::ceil(lobeSx + radX)));
            int minPy = std::max(0, static_cast<int>(std::floor(lobeSy - radY)));
            int maxPy = std::min(m_height - 1, static_cast<int>(std::ceil(lobeSy + radY)));

            float invRadX = 1.0f / radX;
            float invRadY = 1.0f / radY;

            for (int py = minPy; py <= maxPy; ++py) {
                float dy = (static_cast<float>(py) - lobeSy) * invRadY;
                float dySq = dy * dy;
                if (dySq >= 1.0f) continue;
                int frameY = m_height - 1 - py;

                // Proyectar el punto de la nube al suelo para comprobar si está dentro de los límites del mapa
                float groundPy = static_cast<float>(py) + cloudAltOffset;
                float vertLight = std::clamp((lobeSy - static_cast<float>(py)) / radY * 0.6f + 0.5f, 0.0f, 1.0f);
                uint8_t lobeColR = static_cast<uint8_t>(std::lerp(static_cast<float>(shadedCol.r), static_cast<float>(sunlitCol.r), vertLight));
                uint8_t lobeColG = static_cast<uint8_t>(std::lerp(static_cast<float>(shadedCol.g), static_cast<float>(sunlitCol.g), vertLight));
                uint8_t lobeColB = static_cast<uint8_t>(std::lerp(static_cast<float>(shadedCol.b), static_cast<float>(sunlitCol.b), vertLight));

                for (int px = minPx; px <= maxPx; ++px) {
                    float dx = (static_cast<float>(px) - lobeSx) * invRadX;
                    float distSq = dx * dx + dySq;
                    if (distSq >= 1.0f) continue;

                    // Comprobación geométrica: el píxel debe estar estrictamente dentro del mapa isométrico
                    int gx = 0, gy = 0;
                    camera.screenToGrid(static_cast<float>(px), groundPy, gx, gy);
                    if (gx < 0 || gx >= mapWidth || gy < 0 || gy >= mapHeight) {
                        continue; // ¡FUERA DEL MAPA! No renderizar la nube fuera de los límites del mapa
                    }

                    // Atenuación suave en los bordes del mapa para evitar cortes bruscos
                    float edgeX = std::min(static_cast<float>(gx), static_cast<float>(mapWidth - 1 - gx));
                    float edgeY = std::min(static_cast<float>(gy), static_cast<float>(mapHeight - 1 - gy));
                    float borderFade = std::clamp(std::min(edgeX, edgeY) / 1.5f, 0.0f, 1.0f);

                    float dist = std::sqrt(distSq);
                    float edgeFade = (1.0f - dist * dist) * (1.0f - dist);
                    float alpha = edgeFade * lobe.density * cloud.baseAlpha * zoomVis * borderFade;
                    if (alpha <= 0.005f) continue;

                    uint32_t bg = m_framebuffer[frameY * m_width + px];
                    uint8_t bgR = bg & 0xFF;
                    uint8_t bgG = (bg >> 8) & 0xFF;
                    uint8_t bgB = (bg >> 16) & 0xFF;

                    uint8_t outR = static_cast<uint8_t>(std::lerp(static_cast<float>(bgR), static_cast<float>(lobeColR), alpha));
                    uint8_t outG = static_cast<uint8_t>(std::lerp(static_cast<float>(bgG), static_cast<float>(lobeColG), alpha));
                    uint8_t outB = static_cast<uint8_t>(std::lerp(static_cast<float>(bgB), static_cast<float>(lobeColB), alpha));

                    uint32_t outA = (bg >> 24) & 0xFF;
                    m_framebuffer[frameY * m_width + px] = (outA << 24) | (static_cast<uint32_t>(outB) << 16) | (static_cast<uint32_t>(outG) << 8) | static_cast<uint32_t>(outR);
                }
            }
        }
    }
}

void Renderer::render(const WorldMap& map, const ReligionRegistry& registry,
                      const Timeline& timeline, const EventLine& eventLine,
                      const ReplicationEngine& replication, const GestorHumanos& humanos,
                      const IsometricCamera& camera,
                      const Point2i& selectedCell, RenderMapMode mode,
                      const WorldCreationConfig& playerConfig,
                      const AdvancementSystem* advSystem,
                      const DivinePowersSystem* powersSystem,
                      Point2i hoverGridCell,
                      const DayNightCycle* dayNight,
                      const CloudSystem* clouds) {
    if (dayNight) {
        m_dayNight = dayNight;
    }
    if (clouds) {
        m_cloudSystem = clouds;
    }
    m_cameraZoom = camera.getZoom();
    m_cameraX = camera.getCameraX();
    m_cameraY = camera.getCameraY();
    m_mapWidth = map.getWidth();
    m_mapHeight = map.getHeight();

    // Dibujar cielo celestial y cuerpos celestes referenciados al mapa antes de proyectar la isla (si no se dibujó ya en beginFrame)
    if (m_dayNight && !m_skyDrawnThisFrame) {
        drawCelestialSky(*m_dayNight);
        m_skyDrawnThisFrame = true;
    }

    renderWorld(map, registry, replication, humanos, camera, selectedCell, mode);

    // 1. Sombras dinámicas proyectadas de las nubes confinadas sobre el mapa
    if (m_cloudSystem) {
        renderCloudShadows(*m_cloudSystem, camera, m_dayNight, map.getWidth(), map.getHeight());
    }

    if (powersSystem) {
        renderDivineEffects(*powersSystem, camera, map);
        renderPreacherSpeeches(*powersSystem, camera, map);
    }
    if (m_activeDivinePower != DivinePowerType::Inactive && map.isValid(hoverGridCell.x, hoverGridCell.y)) {
        renderTargetingReticle(hoverGridCell, m_activeDivinePower, camera, map);
    }

    // 2. Aplicar Iluminación Dinámica Falsa sobre el mundo
    if (m_dayNight) {
        applyFakeDynamicLighting(*m_dayNight, map, humanos, powersSystem, camera);
        renderCelestialBodies(*m_dayNight, camera, map.getWidth(), map.getHeight());
    }

    // 3. Capa de Nubes a Altura Determinada (Zoom Out visibles, Zoom In invisibles, confinadas al mapa)
    if (m_cloudSystem) {
        renderClouds(*m_cloudSystem, camera, m_dayNight, map.getWidth(), map.getHeight());
    }

    renderUI(map, registry, timeline, eventLine, selectedCell, mode, humanos, camera, playerConfig, advSystem);
    if (m_activeDivinePower != DivinePowerType::Inactive) {
        // Banner de modo apuntar en la parte superior
        int banW = 620;
        int banH = 24;
        int banX = (m_width - banW) / 2;
        int banY = 40;
        std::string pName = "PODER DIVINO";
        if (m_activeDivinePower == DivinePowerType::Lightning) pName = "RAYO DIVINO (Costo: 25 Fe)";
        else if (m_activeDivinePower == DivinePowerType::Plague) pName = "PLAGA SAGRADA (Costo: 35 Fe)";
        else if (m_activeDivinePower == DivinePowerType::Miracle) pName = "MILAGRO CELESTIAL (Costo: 30 Fe)";
        else if (m_activeDivinePower == DivinePowerType::Tornado) pName = "TORNADO DEVASTADOR (Costo: 40 Fe)";
        else if (m_activeDivinePower == DivinePowerType::Earthquake) pName = "TERREMOTO SÍSMICO (Costo: 50 Fe)";
        else if (m_activeDivinePower == DivinePowerType::Pandemic) pName = "PANDEMIA VIRAL (Costo: 45 Fe)";
        else if (m_activeDivinePower == DivinePowerType::Meteor) pName = "METEORITO CATACLÍSMICO (Costo: 60 Fe)";
        else if (m_activeDivinePower == DivinePowerType::Volcano) pName = "ERUPCIÓN VOLCÁNICA (Costo: 70 Fe)";
        else if (m_activeDivinePower == DivinePowerType::Wildfire) pName = "INCENDIO FORESTAL (Costo: 55 Fe)";

        drawSims2Panel(banX, banY, banW, banH, ColorRGB{24, 16, 12}, false, Palette::DIVINE_GOLD);
        drawText(banX + 16, banY + 7, "[MODO APUNTAR] Haz clic en el mapa para lanzar " + pName + " | Clic derecho para cancelar", Palette::DIVINE_GOLD, 1);
    }
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

            // Sistema de Cohesión: Obtención de vecinos cardinales y diagonales
            auto getSafeTexIdx = [&](int nx, int ny) -> int {
                if (nx >= 0 && nx < w && ny >= 0 && ny < h) {
                    return getCellTexIdx(nx, ny);
                }
                return texIdx;
            };
            auto getSafeIsWater = [&](int nx, int ny) -> bool {
                if (nx >= 0 && nx < w && ny >= 0 && ny < h) {
                    return map.getCell(nx, ny).isWater();
                }
                return isWater;
            };

            int nbE  = getSafeTexIdx(x + 1, y);
            int nbW  = getSafeTexIdx(x - 1, y);
            int nbN  = getSafeTexIdx(x, y + 1);
            int nbS  = getSafeTexIdx(x, y - 1);

            int nbNE = getSafeTexIdx(x + 1, y + 1);
            int nbNW = getSafeTexIdx(x - 1, y + 1);
            int nbSE = getSafeTexIdx(x + 1, y - 1);
            int nbSW = getSafeTexIdx(x - 1, y - 1);

            bool shoreE = getSafeIsWater(x + 1, y) != isWater;
            bool shoreW = getSafeIsWater(x - 1, y) != isWater;
            bool shoreN = getSafeIsWater(x, y + 1) != isWater;
            bool shoreS = getSafeIsWater(x, y - 1) != isWater;
            bool hasShoreNeighbor = shoreE || shoreW || shoreN || shoreS;

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
                bool hasDiffNeighbor = (nbE != texIdx) || (nbW != texIdx) || (nbN != texIdx) || (nbS != texIdx) ||
                                       (nbNE != texIdx) || (nbNW != texIdx) || (nbSE != texIdx) || (nbSW != texIdx);

                int minX = std::max(0, static_cast<int>(std::floor(screenPt.x - halfW)));
                int maxX = std::min(m_width - 1, static_cast<int>(std::ceil(screenPt.x + halfW)));
                int minY = std::max(0, static_cast<int>(std::floor(screenPt.y - halfH)));
                int maxY = std::min(m_height - 1, static_cast<int>(std::ceil(screenPt.y + halfH + skirtH)));

                float invHalfH = 1.0f / halfH;
                float invHalfW = 1.0f / halfW;

                for (int py = minY; py <= maxY; ++py) {
                    int frameY = m_height - 1 - py;
                    if (frameY < 0 || frameY >= m_height) continue;

                    float relY = (static_cast<float>(py) - screenPt.y) * invHalfH;
                    float dy = std::abs(relY);

                    // Optimización de scanlines: solo iterar dentro del rombo si no hay faldón en esta línea
                    int lineMinX = minX;
                    int lineMaxX = maxX;
                    bool canHaveSkirt = (!isWater && skirtH > 0.0f && py > screenPt.y);
                    if (!canHaveSkirt) {
                        if (dy > 1.045f) continue;
                        float spanX = (1.045f - dy) * halfW;
                        lineMinX = std::max(minX, static_cast<int>(std::floor(screenPt.x - spanX)));
                        lineMaxX = std::min(maxX, static_cast<int>(std::ceil(screenPt.x + spanX)));
                    }

                    for (int px = lineMinX; px <= lineMaxX; ++px) {
                        float relX = (static_cast<float>(px) - screenPt.x) * invHalfW;
                        float dx = std::abs(relX);
                        float diamondDist = dx + dy;

                        if (diamondDist <= 1.045f) {
                            // Coordenadas isométricas dentro del rombo: du (eje +X), dv (eje +Y)
                            float du = (relX + relY) * 0.5f;
                            float dv = (relY - relX) * 0.5f;

                            // Coordenadas continuas en el espacio de la cuadrícula mundial
                            float gxCont = static_cast<float>(x) + 0.5f + du;
                            float gyCont = static_cast<float>(y) + 0.5f + dv;

                            int wx = static_cast<int>(std::floor(gxCont * 64.0f)) & 63;
                            int wy = static_cast<int>(std::floor(gyCont * 64.0f)) & 63;

                            float r0, g0, b0;
                            sampleAtlasRGB(texIdx, wx, wy, r0, g0, b0);
                            float r, g, b;

                            if (!hasDiffNeighbor && !hasShoreNeighbor) {
                                r = r0;
                                g = g0;
                                b = b0;
                            } else {
                                float turbX = m_turbTableX[wy][wx];
                                float turbY = m_turbTableY[wy][wx];
                                float dither = m_ditherTable[wy][wx];

                                float w0 = 1.0f;
                                float accumR = r0 * w0;
                                float accumG = g0 * w0;
                                float accumB = b0 * w0;
                                float totalW = w0;

                                // Cardinal Este (+X)
                                if (nbE != texIdx) {
                                    float dE = (0.5f - du) + turbX;
                                    if (dE < 0.32f) {
                                        float t = std::clamp((0.32f - dE) / 0.32f, 0.0f, 1.0f);
                                        float wE = t * t * (3.0f - 2.0f * t) + (t > 0.1f ? dither : 0.0f);
                                        if (wE > 0.0f) {
                                            float nr, ng, nb;
                                            sampleAtlasRGB(nbE, wx, wy, nr, ng, nb);
                                            accumR += nr * wE; accumG += ng * wE; accumB += nb * wE;
                                            totalW += wE;
                                        }
                                    }
                                }
                                // Cardinal Oeste (-X)
                                if (nbW != texIdx) {
                                    float dW = (0.5f + du) - turbX;
                                    if (dW < 0.32f) {
                                        float t = std::clamp((0.32f - dW) / 0.32f, 0.0f, 1.0f);
                                        float wW = t * t * (3.0f - 2.0f * t) + (t > 0.1f ? dither : 0.0f);
                                        if (wW > 0.0f) {
                                            float nr, ng, nb;
                                            sampleAtlasRGB(nbW, wx, wy, nr, ng, nb);
                                            accumR += nr * wW; accumG += ng * wW; accumB += nb * wW;
                                            totalW += wW;
                                        }
                                    }
                                }
                                // Cardinal Norte (+Y)
                                if (nbN != texIdx) {
                                    float dN = (0.5f - dv) + turbY;
                                    if (dN < 0.32f) {
                                        float t = std::clamp((0.32f - dN) / 0.32f, 0.0f, 1.0f);
                                        float wN = t * t * (3.0f - 2.0f * t) + (t > 0.1f ? dither : 0.0f);
                                        if (wN > 0.0f) {
                                            float nr, ng, nb;
                                            sampleAtlasRGB(nbN, wx, wy, nr, ng, nb);
                                            accumR += nr * wN; accumG += ng * wN; accumB += nb * wN;
                                            totalW += wN;
                                        }
                                    }
                                }
                                // Cardinal Sur (-Y)
                                if (nbS != texIdx) {
                                    float dS = (0.5f + dv) - turbY;
                                    if (dS < 0.32f) {
                                        float t = std::clamp((0.32f - dS) / 0.32f, 0.0f, 1.0f);
                                        float wS = t * t * (3.0f - 2.0f * t) + (t > 0.1f ? dither : 0.0f);
                                        if (wS > 0.0f) {
                                            float nr, ng, nb;
                                            sampleAtlasRGB(nbS, wx, wy, nr, ng, nb);
                                            accumR += nr * wS; accumG += ng * wS; accumB += nb * wS;
                                            totalW += wS;
                                        }
                                    }
                                }

                                // Diagonales (esquinas) - sólo si aportan un bioma distinto a los adyacentes
                                if (nbNE != texIdx && nbNE != nbE && nbNE != nbN) {
                                    float dNE = std::sqrt((0.5f - du) * (0.5f - du) + (0.5f - dv) * (0.5f - dv)) + (turbX + turbY) * 0.5f;
                                    if (dNE < 0.34f) {
                                        float t = std::clamp((0.34f - dNE) / 0.34f, 0.0f, 1.0f);
                                        float wNE = t * t * (3.0f - 2.0f * t) * 0.7f;
                                        float nr, ng, nb;
                                        sampleAtlasRGB(nbNE, wx, wy, nr, ng, nb);
                                        accumR += nr * wNE; accumG += ng * wNE; accumB += nb * wNE;
                                        totalW += wNE;
                                    }
                                }
                                if (nbNW != texIdx && nbNW != nbW && nbNW != nbN) {
                                    float dNW = std::sqrt((0.5f + du) * (0.5f + du) + (0.5f - dv) * (0.5f - dv)) + (-turbX + turbY) * 0.5f;
                                    if (dNW < 0.34f) {
                                        float t = std::clamp((0.34f - dNW) / 0.34f, 0.0f, 1.0f);
                                        float wNW = t * t * (3.0f - 2.0f * t) * 0.7f;
                                        float nr, ng, nb;
                                        sampleAtlasRGB(nbNW, wx, wy, nr, ng, nb);
                                        accumR += nr * wNW; accumG += ng * wNW; accumB += nb * wNW;
                                        totalW += wNW;
                                    }
                                }
                                if (nbSE != texIdx && nbSE != nbE && nbSE != nbS) {
                                    float dSE = std::sqrt((0.5f - du) * (0.5f - du) + (0.5f + dv) * (0.5f + dv)) + (turbX - turbY) * 0.5f;
                                    if (dSE < 0.34f) {
                                        float t = std::clamp((0.34f - dSE) / 0.34f, 0.0f, 1.0f);
                                        float wSE = t * t * (3.0f - 2.0f * t) * 0.7f;
                                        float nr, ng, nb;
                                        sampleAtlasRGB(nbSE, wx, wy, nr, ng, nb);
                                        accumR += nr * wSE; accumG += ng * wSE; accumB += nb * wSE;
                                        totalW += wSE;
                                    }
                                }
                                if (nbSW != texIdx && nbSW != nbW && nbSW != nbS) {
                                    float dSW = std::sqrt((0.5f + du) * (0.5f + du) + (0.5f + dv) * (0.5f + dv)) + (-turbX - turbY) * 0.5f;
                                    if (dSW < 0.34f) {
                                        float t = std::clamp((0.34f - dSW) / 0.34f, 0.0f, 1.0f);
                                        float wSW = t * t * (3.0f - 2.0f * t) * 0.7f;
                                        float nr, ng, nb;
                                        sampleAtlasRGB(nbSW, wx, wy, nr, ng, nb);
                                        accumR += nr * wSW; accumG += ng * wSW; accumB += nb * wSW;
                                        totalW += wSW;
                                    }
                                }

                                float invTotalW = 1.0f / totalW;
                                r = accumR * invTotalW;
                                g = accumG * invTotalW;
                                b = accumB * invTotalW;

                                // 4. Transición natural de costa con meandros orgánicos
                                if (hasShoreNeighbor) {
                                    float dShore = 1.0f;
                                    if (shoreE) dShore = std::min(dShore, (0.5f - du) + turbX);
                                    if (shoreW) dShore = std::min(dShore, (0.5f + du) - turbX);
                                    if (shoreN) dShore = std::min(dShore, (0.5f - dv) + turbY);
                                    if (shoreS) dShore = std::min(dShore, (0.5f + dv) - turbY);

                                    if (!isWater) {
                                        if (dShore < 0.24f) {
                                            float sFact = std::clamp((0.24f - dShore) / 0.24f, 0.0f, 1.0f);
                                            float sandW = sFact * 0.65f;
                                            r = r * (1.0f - sandW) + 155.0f * sandW;
                                            g = g * (1.0f - sandW) + 140.0f * sandW;
                                            b = b * (1.0f - sandW) + 105.0f * sandW;
                                            if (sFact > 0.75f) {
                                                r *= 0.82f; g *= 0.78f; b *= 0.74f;
                                            }
                                        }
                                    } else {
                                        if (dShore < 0.26f) {
                                            float sFact = std::clamp((0.26f - dShore) / 0.26f, 0.0f, 1.0f);
                                            float waveBand = std::sin(sFact * 10.0f + (wx * 0.20f)) * 0.5f + 0.5f;
                                            float foamW = sFact * (0.35f + 0.35f * waveBand);
                                            r = r * (1.0f - foamW) + 175.0f * foamW;
                                            g = g * (1.0f - foamW) + 230.0f * foamW;
                                            b = b * (1.0f - foamW) + 225.0f * foamW;
                                        }
                                    }
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

                            // 3B. Tensión de Frontera Contendida / Tabú Liminal o Alianza de Paz
                            if (c.isContestedBorder && diamondDist > 0.86f && diamondDist <= 1.0f) {
                                float tFactor = std::clamp(c.borderTension * 0.75f, 0.25f, 0.85f);
                                r = r * (1.0f - tFactor) + 215.0f * tFactor;
                                g = g * (1.0f - tFactor) + 38.0f * tFactor;
                                b = b * (1.0f - tFactor) + 28.0f * tFactor;
                            } else if (c.isSacredTruceActive && diamondDist > 0.86f && diamondDist <= 1.0f) {
                                float tFactor = 0.60f;
                                r = r * (1.0f - tFactor) + 245.0f * tFactor;
                                g = g * (1.0f - tFactor) + 215.0f * tFactor;
                                b = b * (1.0f - tFactor) + 80.0f * tFactor;
                            }

                            // 3C. Cráter de Impacto Permanente de Meteorito
                            if (c.hasImpactCrater) {
                                if (diamondDist <= 0.94f) {
                                    if (diamondDist <= 0.65f) {
                                        float bowlDist = diamondDist / 0.65f;
                                        float magmaVein = std::sin(du * 18.0f + std::cos(dv * 18.0f) * 4.0f) * std::cos(dv * 18.0f);
                                        if (std::abs(magmaVein) < 0.22f) {
                                            float glow = 0.5f + 0.5f * std::sin(m_animTime * 3.5f + du * 10.0f);
                                            r = 255.0f;
                                            g = 80.0f + 90.0f * glow;
                                            b = 20.0f;
                                        } else {
                                            float shade = 0.55f + 0.45f * bowlDist;
                                            r = (24.0f + 10.0f * bowlDist) * shade;
                                            g = (18.0f + 8.0f * bowlDist) * shade;
                                            b = (20.0f + 8.0f * bowlDist) * shade;
                                        }
                                    } else {
                                        float rimFactor = (diamondDist - 0.65f) / 0.29f;
                                        float rimDark = 0.35f + 0.55f * rimFactor;
                                        r = r * rimDark + 38.0f * (1.0f - rimFactor);
                                        g = g * rimDark + 28.0f * (1.0f - rimFactor);
                                        b = b * rimDark + 24.0f * (1.0f - rimFactor);
                                    }
                                }
                            }

                            // 3D. Ríos y Charcos de Lava / Magma Activo
                            if (c.hasActiveLava) {
                                float flowNoise = std::sin((gxCont + gyCont) * 8.0f + m_animTime * 3.5f) * 0.5f + 0.5f;
                                float crustPlate = std::sin(gxCont * 12.0f - m_animTime * 1.5f) * std::cos(gyCont * 12.0f + m_animTime * 1.5f);
                                if (crustPlate > 0.32f) {
                                    r = 45.0f; g = 22.0f; b = 18.0f;
                                } else {
                                    r = 255.0f;
                                    g = 65.0f + 140.0f * flowNoise;
                                    b = 18.0f;
                                }
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

    // EN: Full highlight diamond of selected cell on terrain surface (beneath buildings and units)
    // ES: Resaltado completo de la celda seleccionada sobre la superficie del terreno (debajo de edificios y unidades)
    if (map.isValid(selectedCell.x, selectedCell.y)) {
        const Cell& selectedCellRef = map.getCell(selectedCell.x, selectedCell.y);
        const Vec2 selectedScreenPoint = camera.gridTileCenterScreen(selectedCell.x, selectedCell.y, selectedCellRef.elevation);

        ColorRGB selectorBorderColor(0, 255, 255); // EN: Default cyan for neutral terrain / ES: Cian por defecto
        const Humano* selectedVillager = humanos.getHumanoSeleccionado();
        if (selectedVillager && selectedVillager->estaVivo()) {
            const Religion* villagerReligion = registry.getReligion(selectedVillager->getReligionId());
            if (villagerReligion) {
                selectorBorderColor = villagerReligion->color;
            } else {
                selectorBorderColor = Palette::DIVINE_GOLD;
            }
        }
        drawDiamondOutline(selectedScreenPoint.x, selectedScreenPoint.y, halfW + 1, halfH + 1, selectorBorderColor);
        drawDiamondOutline(selectedScreenPoint.x, selectedScreenPoint.y, halfW, halfH, ColorRGB(255, 255, 255));
    }

    // =========================================================================
    // PASADA 2: COLA UNIFICADA DE PROFUNDIDAD ISOMÉTRICA (Edificios, Árboles y Unidades)
    // Garantiza que cuando un aldeano u otra unidad pase por detrás de los templos
    // o árboles, las partes ocultas del sprite queden cubiertas de manera natural.
    // =========================================================================
    enum class IsoSpriteKind : uint8_t {
        Villager     = 0,
        Caravan      = 1,
        Prop         = 2,
        ResourceNode = 3,
        Temple       = 4,
        Volcano      = 5,
        House        = 6
    };

    struct IsoSpriteItem {
        float isoDepth;
        IsoSpriteKind kind;
        int cellX;
        int cellY;
        int subId;
        const Humano* humanoPtr;
        const MissionaryCaravan* caravanPtr;
        const House* housePtr{nullptr};
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

            if (c.hasVolcano) {
                sceneSprites.push_back({
                    static_cast<float>(x + y) + 1.45f,
                    IsoSpriteKind::Volcano,
                    x, y, 0, nullptr, nullptr
                });
            } else if (c.hasHolySite) {
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

                // 1. Fuentes de minerales nobles (Oro y Hierro): se muestran siempre cuando tienen existencias
                if ((c.naturalResource == ResourceType::Gold || c.naturalResource == ResourceType::Iron) && c.resourceAmount > 0) {
                    sceneSprites.push_back({
                        static_cast<float>(x + y) + 1.05f,
                        IsoSpriteKind::ResourceNode,
                        x, y, static_cast<int>(c.naturalResource), nullptr, nullptr
                    });
                }
                // 2. Fuentes de Comida, Madera y Canteras de Piedra: distribución orgánica
                else if (c.naturalResource != ResourceType::None && c.resourceAmount > 0 && (roll >= 58 && roll < 85)) {
                    sceneSprites.push_back({
                        static_cast<float>(x + y) + 1.05f,
                        IsoSpriteKind::ResourceNode,
                        x, y, static_cast<int>(c.naturalResource), nullptr, nullptr
                    });
                } else {
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
    }

    // 2. Agregar Aldeanos (unidades Humano) con su profundidad continua exacta (hx + hy)
    for (const auto& hObj : humanos.getHumanos()) {
        // Las unidades de otras fes sólo se ven dentro del alcance de la fe del jugador
        if (isHiddenByFog(hObj.getCeldaX(), hObj.getCeldaY(), hObj.getReligionId())) continue;
        // Si el aldeano está durmiendo protegido dentro de su hogar, su sprite desaparece (está bajo techo)
        if (hObj.estaDurmiendoBajoTecho()) continue;
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

    // 4. Agregar Viviendas y Asentamientos Tribales (BuildingSystem)
    if (m_buildingSystem) {
        for (const auto& house : m_buildingSystem->getHouses()) {
            if (isHiddenByFog(house.cellX, house.cellY, house.religionId)) continue;
            sceneSprites.push_back({
                static_cast<float>(house.cellX + house.cellY) + 1.25f,
                IsoSpriteKind::House,
                house.cellX, house.cellY, house.id, nullptr, nullptr, &house
            });
        }
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
            const TileSet& ts = hObj.esChaman() ? m_shamanTileSet : (hObj.esMujer() ? m_femaleTileSet : m_tileSet);

            // Vista UNIDADES: anillo isométrico con el color de fe bajo cada aldeano (doble anillo para chamanes)
            if (mode == RenderMapMode::Units && hObj.estaVivo()) {
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

            // Anillo selector isométrico con el color de su fe bajo los pies de la unidad seleccionada
            if (esSel && hObj.estaVivo()) {
                int ucx = std::clamp(hObj.getCeldaX(), 0, w - 1);
                int ucy = std::clamp(hObj.getCeldaY(), 0, h - 1);
                Vec2 ufoot = camera.gridToScreen(hObj.getX(), hObj.getY(), map.getCell(ucx, ucy).elevation);
                float selR = (hObj.esChaman() ? 16.0f : 12.0f) * zoom;
                drawDiamondOutline(ufoot.x, ufoot.y, selR + 2.0f, (selR + 2.0f) * 0.5f, aura);
                drawDiamondOutline(ufoot.x, ufoot.y, selR + 1.0f, (selR + 1.0f) * 0.5f, ColorRGB{255, 255, 255});
                drawDiamondOutline(ufoot.x, ufoot.y, selR, selR * 0.5f, aura);
                if (hObj.esChaman()) {
                    drawDiamondOutline(ufoot.x, ufoot.y, selR * 0.6f, selR * 0.3f, Palette::DIVINE_GOLD);
                }
            }
            hObj.renderizar(m_framebuffer.data(), m_width, m_height, camera, map, ts, aura, esSel);

            // Si es un Chamán vivo canalizando una señal divina, proyectar haz solar sagrado
            if (hObj.estaVivo() && hObj.esChaman() && hObj.getTimerSenalDivina() > 0.0f) {
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
                         static_cast<int>(pDest.x), static_cast<int>(pDest.y), aura);
                drawDiamondOutline(pDest.x, pDest.y, 10.0f * zoom, 5.0f * zoom, aura);
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
        } else if (item.kind == IsoSpriteKind::ResourceNode) {
            const Cell& c = map.getCell(item.cellX, item.cellY);
            Vec2 screenPt = camera.gridTileCenterScreen(item.cellX, item.cellY, c.elevation);
            if (screenPt.x + halfW >= 0 && screenPt.x - halfW <= m_width &&
                screenPt.y + halfH >= 0 && screenPt.y - halfH * 3.0f <= m_height) {
                drawResourceNode(screenPt.x, screenPt.y, static_cast<ResourceType>(item.subId), zoom, c.resourceAmount);
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
        } else if (item.kind == IsoSpriteKind::Volcano) {
            const Cell& c = map.getCell(item.cellX, item.cellY);
            Vec2 screenPt = camera.gridTileCenterScreen(item.cellX, item.cellY, c.elevation);
            if (screenPt.x + halfW * 3.0f >= 0 && screenPt.x - halfW * 3.0f <= m_width &&
                screenPt.y + halfH * 2.0f >= 0 && screenPt.y - halfH * 6.0f <= m_height) {
                drawVolcano(screenPt.x, screenPt.y, zoom, m_animTime);
            }
        } else if (item.kind == IsoSpriteKind::House && item.housePtr) {
            const House& house = *item.housePtr;
            const Cell& c = map.getCell(item.cellX, item.cellY);
            Vec2 screenPt = camera.gridTileCenterScreen(item.cellX, item.cellY, c.elevation);
            if (screenPt.x + halfW * 2.5f >= 0 && screenPt.x - halfW * 2.5f <= m_width &&
                screenPt.y + halfH * 2.5f >= 0 && screenPt.y - halfH * 4.5f <= m_height) {
                bool isNight = m_dayNight && m_dayNight->isNight();
                drawSpriteHouse(screenPt.x, screenPt.y, house, zoom, isNight);
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
        float leaderDevotion;
        bool isShaman;
        bool isProphet;
        bool isFanatic;
        bool isTemple;
        float signalTimer;
    };

    std::map<int, std::vector<AuraNode>> nodesByRel;

    // 1. Recolectar todos los nodos vivos (Aldeanos, Líderes y Fanáticos)
    for (const auto& hObj : humanos.getHumanos()) {
        if (!hObj.estaVivo()) continue;
        int rId = hObj.getReligionId();
        if (rId <= 0) continue;
        int cx = std::clamp(hObj.getCeldaX(), 0, w - 1);
        int cy = std::clamp(hObj.getCeldaY(), 0, h - 1);
        if (isHiddenByFog(cx, cy, rId)) continue; // aura rival oculta en la niebla
        float elev = map.getCell(cx, cy).elevation;
        Vec2 sp = camera.gridToScreen(hObj.getX(), hObj.getY(), elev);

        float rad = (hObj.esLiderEspiritual() ? 52.0f : (hObj.esFanatico() ? 42.0f : 34.0f)) * zoom;
        if (hObj.getTimerSenalDivina() > 0.0f) {
            rad = 68.0f * zoom;
        }
        nodesByRel[rId].push_back({
            hObj.getX(), hObj.getY(),
            sp.x, sp.y,
            rad, hObj.getDevocion(),
            hObj.getLeaderDevotion(),
            hObj.esChaman(),
            hObj.esProfeta(),
            hObj.esFanatico(),
            false,
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
                    1.0f,
                    false, false, false, true, 0.0f
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
        const float baseBridgeRad = std::max(7.0f, 22.0f * zoom);
        const float invIsoY = 1.0f / 0.52f;

        for (const auto& edge : edges) {
            const auto& nA = nodes[edge.first];
            const auto& nB = nodes[edge.second];

            // Ponderación de devoción combinada y devoción a líderes en la arista de fe
            float devFactorA = nA.devotion * (0.35f + 0.65f * nA.leaderDevotion);
            float devFactorB = nB.devotion * (0.35f + 0.65f * nB.leaderDevotion);
            float edgeDev = std::clamp(0.5f * (devFactorA + devFactorB), 0.10f, 1.0f);
            bool isFanaticEdge = nA.isFanatic || nB.isFanatic;
            bool isLeaderEdge = nA.isShaman || nA.isProphet || nB.isShaman || nB.isProphet;

            if (isFanaticEdge) {
                edgeDev = std::min(1.0f, edgeDev * 1.25f);
            }

            // Radio del puente y grosor de línea notable y proporcional a la devoción
            // Baja devoción: línea estrecha (0.55x). Alta devoción: línea ancha, gruesa y llamativa (1.50x)
            float bridgeRad = baseBridgeRad * (0.55f + 0.95f * edgeDev);
            float bridgeRadY = bridgeRad * 0.52f;

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

            // Umbral del hilo luminoso central: con alta devoción la línea es mucho más gruesa y notable
            float coreThreshold = 0.08f + 0.22f * edgeDev;
            if (isFanaticEdge) coreThreshold += 0.08f;
            if (isLeaderEdge) coreThreshold += 0.06f;

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
                        float val = (0.22f + 0.28f * edgeDev) * oneMinusU2 * oneMinusU2;

                        // Hilo de energía luminoso: mientras más devoción hay, la línea es más ancha y brillante
                        if (u < coreThreshold) {
                            float lineFactor = 1.0f - (u / coreThreshold);
                            float glowBase = 0.32f + 0.45f * edgeDev;
                            if (isFanaticEdge) glowBase += 0.18f;
                            if (isLeaderEdge) glowBase += 0.12f;
                            val = std::max(val, glowBase + 0.30f * lineFactor);
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

                    // Anillo de fanático: destello exterior de fervor
                    if (n.isFanatic && (u >= 0.22f && u <= 0.32f)) {
                        val = std::max(val, 0.78f);
                    }

                    // Anillo de nodo en los pies (doble anillo solar para el Chamán)
                    if (n.isShaman || n.isProphet) {
                        if ((u >= 0.16f && u <= 0.24f) || (u >= 0.36f && u <= 0.42f)) {
                            val = std::max(val, 0.85f);
                        }
                        if (n.signalTimer > 0.0f) {
                            float wavePhase = std::fmod((4.5f - n.signalTimer) * 0.85f, 1.0f);
                            if (std::abs(u - wavePhase) < 0.08f) {
                                float wBoost = 1.0f - std::abs(u - wavePhase) / 0.08f;
                                val = std::max(val, 0.92f * wBoost);
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
                        const WorldCreationConfig& playerConfig,
                        const AdvancementSystem* advSystem) {
    (void)selectedCell;
    renderTopBar(map, registry, timeline, playerConfig);
    renderBottomBar(map, registry, timeline, eventLine, humanos, camera, mode);
    if (m_chronicleOpen) {
        renderChronicleModal(eventLine, timeline, m_chronicleFilter, m_chronicleScrollIndex,
                             m_mouseX, m_mouseY, m_isMouseDown);
    }
    if (m_faithTreeOpen && advSystem) {
        renderFaithTreeModal(registry, *advSystem, playerConfig, m_selectedAdvancementId,
                             m_mouseX, m_mouseY, m_isMouseDown);
    }
    if (m_memoryTreeOpen) {
        renderMemoryTreeModal(humanos, registry, timeline, m_mouseX, m_mouseY, m_isMouseDown);
    }
    if (m_moralRulesModalOpen && m_moralitySystem) {
        renderMoralRulesModal(*m_moralitySystem, playerReligionId(), registry, humanos, timeline, m_mouseX, m_mouseY, m_isMouseDown);
    }
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
        badgeY += 26;
    }

    // 1c. Indicador de salvaguarda de memoria y mitigador de lag de ErrorSystem
    const auto healthMetrics = ErrorSystem::instance().getHealthMetrics();
    if (healthMetrics.isUnderLagStress || healthMetrics.isMemoryWarningActive || healthMetrics.isPopulationCapped) {
        int badgeX = m_width - 324;
        int alertY = 42;
        int badgeW = 312;
        int badgeH = 22;
        ColorRGB badgeBg = healthMetrics.isMemoryWarningActive ? ColorRGB{60, 18, 18} : ColorRGB{48, 36, 12};
        drawRect(badgeX, alertY, badgeW, badgeH, badgeBg);
        drawRectOutline(badgeX, alertY, badgeW, badgeH, Palette::DIVINE_GOLD);
        std::string alertText = healthMetrics.isMemoryWarningActive
            ? "[ALERTA RAM: " + std::to_string(static_cast<int>(healthMetrics.currentMemoryMb)) + "MB] PURGA ACTIVA"
            : (healthMetrics.isUnderLagStress
                ? "[ANTI-LAG: " + std::to_string(static_cast<int>(healthMetrics.currentFps)) + " FPS] MITIGACION ACTIVA"
                : "[TOPE POBLACION: " + std::to_string(healthMetrics.activeEntityCount) + "] NACIMIENTOS PAUSADOS");
        drawText(badgeX + 8, alertY + 7, alertText, ColorRGB{255, 235, 120}, 1);
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
                 "COMANDOS Y SALUD: 'STATUS' | 'PURGE' | 'SELECTOR' | 'LUMINUS' (ESC: CERRAR)",
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
    (void)map;
    const int topH = 34;

    // Región 1: Panel de Información de Pueblo (Izquierda - 44% ancho)
    // Información de creyentes del jugador, Dios y recursos sagrados (Fondo #120b0a con volumen satinado Sims 2)
    const int grisW = static_cast<int>(m_width * 0.44f);
    drawSims2Panel(0, 0, grisW, topH, ColorRGB{18, 11, 10}, false, ColorRGB{75, 52, 38});
    drawLine(0, topH, grisW, topH, Palette::DIVINE_GOLD);

    // Datos del jugador, creyentes y recursos de supervivencia
    const Religion* pRel = registry.getReligion(1);
    int playerFollowers = pRel ? pRel->totalFollowers : 0;
    int pietyPoints = pRel ? pRel->theologyPoints : 1000;
    std::string godDisplay = "DIOS: " + playerConfig.godName + " | CULTO: " + playerConfig.religionName +
                             " | POB: " + std::to_string(playerFollowers);
    drawText(10, 4, godDisplay.substr(0, 52), Palette::DIVINE_GOLD, 1);

    // Recursos del pueblo para sobrevivir (Comida, Madera, Piedra, Oro, Hierro)
    int food = pRel ? pRel->resources.food : 500;
    int wood = pRel ? pRel->resources.wood : 300;
    int stone = pRel ? pRel->resources.stone : 150;
    int gold = pRel ? pRel->resources.gold : 50;
    int iron = pRel ? pRel->resources.iron : 80;
    bool starving = pRel ? pRel->resources.isStarving : false;

    int rx = 10;
    std::string foodStr = "COM: " + std::to_string(food);
    if (starving) foodStr += " (!)";
    ColorRGB foodColor = starving ? ColorRGB{255, 60, 60} : (food < 100 ? ColorRGB{255, 180, 50} : ColorRGB{120, 220, 100});
    drawText(rx, 17, foodStr, foodColor, 1);
    rx += static_cast<int>(foodStr.size() * 6) + 10;

    std::string woodStr = "MAD: " + std::to_string(wood);
    drawText(rx, 17, woodStr, ColorRGB{210, 155, 95}, 1);
    rx += static_cast<int>(woodStr.size() * 6) + 10;

    std::string stoneStr = "PIE: " + std::to_string(stone);
    drawText(rx, 17, stoneStr, ColorRGB{180, 195, 210}, 1);
    rx += static_cast<int>(stoneStr.size() * 6) + 10;

    std::string goldStr = "ORO: " + std::to_string(gold);
    drawText(rx, 17, goldStr, ColorRGB{255, 215, 60}, 1);
    rx += static_cast<int>(goldStr.size() * 6) + 10;

    std::string ironStr = "HIE: " + std::to_string(iron);
    drawText(rx, 17, ironStr, ColorRGB{160, 185, 210}, 1);
    rx += static_cast<int>(ironStr.size() * 6) + 10;

    std::string feStr = "| FE: " + std::to_string(pietyPoints) + " pts";
    drawText(rx, 17, feStr, Palette::RELIC_BONE, 1);

    // Región 2: BLANCO (Centro - 12% ancho)
    // Era actual del pueblo de Dios con banner sagrado estilo papiro satinado
    const int blancaX = grisW;
    const int blancaW = static_cast<int>(m_width * 0.12f);
    drawSims2Panel(blancaX, 0, blancaW, topH, ColorRGB{245, 245, 240}, false, Palette::DIVINE_GOLD);
    drawLine(blancaX, topH, blancaX + blancaW, topH, Palette::DIVINE_GOLD);

    // Ranuras decorativas entre paneles
    drawLine(blancaX - 1, 0, blancaX - 1, topH, ColorRGB{10, 6, 5});
    drawLine(blancaX, 0, blancaX, topH, ColorRGB{75, 52, 38});
    drawLine(blancaX + blancaW, 0, blancaX + blancaW, topH, ColorRGB{10, 6, 5});

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
    // Opciones de partida, velocidad de tiempo y comandos del sistema (Fondo #120b0a con volumen satinado Sims 2)
    const int rojaX = blancaX + blancaW;
    const int rojaW = m_width - rojaX;
    drawSims2Panel(rojaX, 0, rojaW, topH, ColorRGB{18, 11, 10}, false, ColorRGB{75, 52, 38});
    drawLine(rojaX, topH, m_width, topH, Palette::DIVINE_GOLD);
    drawLine(rojaX, 0, rojaX, topH, ColorRGB{75, 52, 38});

    // Insignia Litúrgica Celestial: Día / Noche, Fase y Reloj Solar
    if (m_dayNight) {
        int badgeX = rojaX + 10;
        int badgeY = 5;
        int badgeW = 185;
        int badgeH = 24;

        bool isDayTime = m_dayNight->isDay();
        ColorRGB badgeRim = isDayTime ? Palette::DIVINE_GOLD : ColorRGB{120, 160, 240};
        ColorRGB badgeBase = isDayTime ? ColorRGB{28, 18, 14} : ColorRGB{14, 18, 30};
        drawSims2Panel(badgeX, badgeY, badgeW, badgeH, badgeBase, false, badgeRim);

        int iconCenterX = badgeX + 14;
        int iconCenterY = badgeY + 12;

        if (isDayTime) {
            // Icono Sol brillante
            for (int dy = -4; dy <= 4; ++dy) {
                for (int dx = -4; dx <= 4; ++dx) {
                    if (dx * dx + dy * dy <= 16) {
                        int px = iconCenterX + dx;
                        int py = iconCenterY + dy;
                        int fy = m_height - 1 - py;
                        if (px >= 0 && px < m_width && fy >= 0 && fy < m_height) {
                            m_framebuffer[fy * m_width + px] = (0xFFu << 24) | (40u << 16) | (215u << 8) | 255u;
                        }
                    }
                }
            }
            drawLine(iconCenterX - 6, iconCenterY, iconCenterX + 6, iconCenterY, Palette::DIVINE_GOLD);
            drawLine(iconCenterX, iconCenterY - 6, iconCenterX, iconCenterY + 6, Palette::DIVINE_GOLD);

            std::string timeStr = m_dayNight->getPhaseName() + " (" + m_dayNight->getTimeFormatted() + ")";
            drawText(badgeX + 26, badgeY + 6, timeStr, Palette::DIVINE_GOLD, 1);
        } else {
            // Icono Luna plateada
            for (int dy = -4; dy <= 4; ++dy) {
                for (int dx = -4; dx <= 4; ++dx) {
                    float d1 = std::sqrt(static_cast<float>(dx * dx + dy * dy));
                    float d2 = std::sqrt(static_cast<float>((dx + 2) * (dx + 2) + (dy - 1) * (dy - 1)));
                    if (d1 <= 4.2f && d2 >= 3.0f) {
                        int px = iconCenterX + dx;
                        int py = iconCenterY + dy;
                        int fy = m_height - 1 - py;
                        if (px >= 0 && px < m_width && fy >= 0 && fy < m_height) {
                            m_framebuffer[fy * m_width + px] = (0xFFu << 24) | (255u << 16) | (235u << 8) | 215u;
                        }
                    }
                }
            }
            std::string timeStr = m_dayNight->getPhaseName() + " (" + m_dayNight->getTimeFormatted() + ")";
            drawText(badgeX + 26, badgeY + 6, timeStr, ColorRGB{185, 215, 255}, 1);
        }
    }

    // Panel de Opciones: botón MENU que despliega las opciones de partida
    (void)timeline;
    for (const auto& btn : m_buttons) {
        if (btn.y >= topH) continue; // solo botones del top bar

        bool isActive = (btn.id == 400 && m_dropdownOpen) ||
                        (btn.id == 450 && m_chronicleOpen) ||
                        (btn.id == 460 && m_faithTreeOpen) ||
                        (btn.id == 470 && m_moralRulesModalOpen);

        int bx = static_cast<int>(btn.x);
        int by = static_cast<int>(btn.y);
        int bw = static_cast<int>(btn.width);
        int bh = static_cast<int>(btn.height);

        std::string label = btn.label;
        if (btn.id == 400) label += m_dropdownOpen ? " ^" : " v";

        bool hovered = (m_mouseX >= btn.x && m_mouseX <= btn.x + btn.width &&
                        m_mouseY >= btn.y && m_mouseY <= btn.y + btn.height);
        bool pressed = (m_pressedButtonId == btn.id) || (hovered && m_isMouseDown);
        drawAntiqueButton(bx, by, bw, bh, label, isActive, hovered, pressed, btn.id);
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

    drawSims2Panel(px, py, pw, ph, ColorRGB{18, 11, 10}, false, Palette::DIVINE_GOLD);

    for (const auto& btn : m_dropdownButtons) {
        int bx = static_cast<int>(btn.x);
        int by = static_cast<int>(btn.y);
        int bw = static_cast<int>(btn.width);
        int bh = static_cast<int>(btn.height);
        bool danger = (btn.id == 402);
        bool hovered = (m_mouseX >= btn.x && m_mouseX <= btn.x + btn.width &&
                        m_mouseY >= btn.y && m_mouseY <= btn.y + btn.height);
        bool pressed = (m_pressedButtonId == btn.id) || (hovered && m_isMouseDown);

        ColorRGB itemBase = danger ? (hovered ? ColorRGB{80, 24, 20} : ColorRGB{52, 18, 16})
                                   : (hovered ? ColorRGB{46, 32, 24} : ColorRGB{28, 18, 16});
        ColorRGB itemRim = danger ? ColorRGB{180, 70, 60} : (hovered ? Palette::DIVINE_GOLD : ColorRGB{95, 68, 48});
        drawSims2Panel(bx, by, bw, bh, itemBase, pressed, itemRim);
        drawText(bx + 8, by + (bh - 7) / 2 + (pressed ? 1 : 0), btn.label,
                 danger ? ColorRGB{255, 190, 170} : (hovered ? ColorRGB{255, 245, 180} : Palette::RELIC_BONE), 1);
    }
}

void Renderer::renderChronicleModal(const EventLine& eventLine, const Timeline& timeline,
                                   int activeFilter, int scrollIndex,
                                   float mouseX, float mouseY, bool isMouseDown) {
    // 1. Telón translúcido oscurecedor (Vignette litúrgica sobre la simulación activa)
    for (int y = 0; y < m_height; ++y) {
        int fy = m_height - 1 - y;
        for (int x = 0; x < m_width; ++x) {
            uint32_t px = m_framebuffer[fy * m_width + x];
            uint8_t r = static_cast<uint8_t>(((px & 0xFF) * 45) / 255);
            uint8_t g = static_cast<uint8_t>((((px >> 8) & 0xFF) * 45) / 255);
            uint8_t b = static_cast<uint8_t>((((px >> 16) & 0xFF) * 45) / 255);
            m_framebuffer[fy * m_width + x] = (0xFFu << 24) | (b << 16) | (g << 8) | r;
        }
    }

    const int modalW = 1140;
    const int modalH = 590;
    const int modalX = (m_width - modalW) / 2;
    const int modalY = (m_height - modalH) / 2;

    // 2. Marco exterior estilo Sims 2 con acabado litúrgico biselado
    drawSims2Panel(modalX, modalY, modalW, modalH, ColorRGB{20, 13, 10}, false, Palette::DIVINE_GOLD);
    drawRectOutline(modalX + 2, modalY + 2, modalW - 4, modalH - 4, ColorRGB{75, 52, 38});
    drawRectOutline(modalX + 4, modalY + 4, modalW - 8, modalH - 8, ColorRGB{140, 110, 30});

    // Remates angulares ornamentales en Oro Divino
    drawRect(modalX + 2, modalY + 2, 7, 7, Palette::DIVINE_GOLD);
    drawRect(modalX + modalW - 9, modalY + 2, 7, 7, Palette::DIVINE_GOLD);
    drawRect(modalX + 2, modalY + modalH - 9, 7, 7, Palette::DIVINE_GOLD);
    drawRect(modalX + modalW - 9, modalY + modalH - 9, 7, 7, Palette::DIVINE_GOLD);

    // 3. Placa de Cabecera
    const int headX = modalX + 16;
    const int headY = modalY + 8;
    const int headW = modalW - 68;
    const int headH = 28;
    drawSims2Panel(headX, headY, headW, headH, ColorRGB{38, 26, 16}, false, Palette::DIVINE_GOLD);
    drawEventIcon(headX + 8, headY + 2, 2, false); // Icono de pergamino / revelación
    drawText(headX + 40, headY + 10, "CRONICA SAGRADA: HISTORIAL DE HITOS, PERDIDAS Y PUNTOS DE QUIEBRE", Palette::DIVINE_GOLD, 1);

    // Botón Cerrar [X]
    const int closeX = modalX + modalW - 40;
    const int closeY = modalY + 8;
    const int closeW = 28;
    const int closeH = 28;
    bool closeHov = (mouseX >= closeX && mouseX <= closeX + closeW && mouseY >= closeY && mouseY <= closeY + closeH);
    bool closePress = closeHov && isMouseDown;
    drawSims2Panel(closeX, closeY, closeW, closeH, closeHov ? ColorRGB{160, 28, 28} : ColorRGB{70, 20, 20}, closePress, Palette::DIVINE_GOLD);
    drawText(closeX + 10, closeY + 10, "X", Palette::RELIC_BONE, 1);

    // 4. Barra de Filtros temáticos
    struct FilterTab {
        std::string label;
        int iconCol;
        int width;
    };
    std::vector<FilterTab> tabs = {
        {"TODOS", -1, 60},
        {"PUNTOS DE QUIEBRE", -1, 155},
        {"PERDIDAS", 0, 105},
        {"AVANCES", 1, 95},
        {"DESCUBRIMIENTOS", 2, 140},
        {"CISMAS", 3, 90},
        {"SINCRETISMO", 4, 120}
    };

    int tabX = modalX + 20;
    const int tabY = modalY + 42;
    const int tabH = 24;

    for (size_t i = 0; i < tabs.size(); ++i) {
        bool isAct = (activeFilter == static_cast<int>(i));
        bool hov = (mouseX >= tabX && mouseX <= tabX + tabs[i].width && mouseY >= tabY && mouseY <= tabY + tabH);
        bool prs = hov && isMouseDown;

        ColorRGB tabBg = isAct ? ColorRGB{65, 46, 20} : (hov ? ColorRGB{42, 28, 20} : ColorRGB{26, 17, 13});
        ColorRGB tabRim = isAct ? Palette::DIVINE_GOLD : (hov ? ColorRGB{255, 215, 104} : ColorRGB{85, 60, 42});
        drawSims2Panel(tabX, tabY, tabs[i].width, tabH, tabBg, isAct || prs, tabRim);

        int textOffset = (isAct || prs) ? 1 : 0;
        int curTx = tabX + 8 + textOffset;
        if (tabs[i].iconCol >= 0) {
            drawEventIcon(tabX + 4 + textOffset, tabY + (tabH - 24) / 2 + textOffset, tabs[i].iconCol, isAct || hov);
            curTx += 22;
        }
        drawText(curTx, tabY + 8 + textOffset, tabs[i].label, isAct ? ColorRGB{255, 235, 120} : Palette::RELIC_BONE, 1);
        tabX += tabs[i].width + 6;
    }

    // Obtener lista filtrada de eventos
    std::vector<GameEvent> filtered = eventLine.getFilteredEvents(activeFilter);

    // Contador de eventos registrados
    std::string countStr = "TOTAL: " + std::to_string(filtered.size()) + " HITOS";
    drawText(modalX + modalW - 165, tabY + 8, countStr, Palette::DIVINE_GOLD, 1);

    // 5. Carril de Línea de Tiempo Horizontal (Left to Right Ribbon)
    const int railX = modalX + 45;
    const int railY = modalY + 74;
    const int railW = modalW - 90;
    const int railH = 8;
    drawSims2Panel(railX, railY, railW, railH, ColorRGB{32, 22, 14}, true, ColorRGB{140, 110, 30});
    drawLine(railX + 2, railY + 3, railX + railW - 2, railY + 3, Palette::DIVINE_GOLD);

    // Indicadores temporales en los extremos
    drawText(railX, railY - 12, "-4000 BCE", ColorRGB{210, 195, 160}, 1);
    std::string curYearStr = Timeline::formatYear(timeline.getCurrentYear());
    drawText(railX + railW - static_cast<int>(curYearStr.size() * 6), railY - 12, curYearStr, Palette::DIVINE_GOLD, 1);

    // Botones de navegación horizontal [<] y [>]
    const int navW = 28;
    const int navH = 60;
    const int navLeftX = modalX + 10;
    const int navRightX = modalX + modalW - 38;
    const int navY = modalY + 240;

    bool canScrollLeft = (scrollIndex > 0);
    bool hovL = canScrollLeft && (mouseX >= navLeftX && mouseX <= navLeftX + navW && mouseY >= navY && mouseY <= navY + navH);
    drawSims2Panel(navLeftX, navY, navW, navH, hovL ? ColorRGB{55, 38, 22} : ColorRGB{28, 18, 14}, hovL && isMouseDown, canScrollLeft ? Palette::DIVINE_GOLD : ColorRGB{70, 50, 35});
    drawText(navLeftX + 9, navY + 24, "<", canScrollLeft ? Palette::DIVINE_GOLD : ColorRGB{100, 80, 60}, 2);

    int maxVisibleCards = 4;
    int maxScroll = (static_cast<int>(filtered.size()) > maxVisibleCards) ? static_cast<int>(filtered.size()) - maxVisibleCards : 0;
    bool canScrollRight = (scrollIndex < maxScroll);
    bool hovR = canScrollRight && (mouseX >= navRightX && mouseX <= navRightX + navW && mouseY >= navY && mouseY <= navY + navH);
    drawSims2Panel(navRightX, navY, navW, navH, hovR ? ColorRGB{55, 38, 22} : ColorRGB{28, 18, 14}, hovR && isMouseDown, canScrollRight ? Palette::DIVINE_GOLD : ColorRGB{70, 50, 35});
    drawText(navRightX + 9, navY + 24, ">", canScrollRight ? Palette::DIVINE_GOLD : ColorRGB{100, 80, 60}, 2);

    // 6. Tarjetas de Eventos dispuestas de izquierda a derecha
    const int cardW = 246;
    const int cardH = 465;
    const int cardPitch = 264;
    const int cardsStartX = modalX + 44;
    const int cardY = modalY + 90;

    if (filtered.empty()) {
        drawSims2Panel(modalX + 200, modalY + 220, modalW - 400, 100, ColorRGB{28, 18, 14}, false, Palette::DIVINE_GOLD);
        drawText(modalX + 260, modalY + 250, "NO SE ENCONTRARON EVENTOS PARA EL FILTRO SELECCIONADO", Palette::RELIC_BONE, 1);
        drawText(modalX + 320, modalY + 270, "Continua la simulacion para registrar nuevos hitos sagrados.", ColorRGB{200, 180, 140}, 1);
    }

    auto wrapText = [](const std::string& str, size_t maxLen) {
        std::vector<std::string> lines;
        size_t start = 0;
        while (start < str.size()) {
            if (str.size() - start <= maxLen) {
                lines.push_back(str.substr(start));
                break;
            }
            size_t end = start + maxLen;
            size_t space = str.rfind(' ', end);
            if (space != std::string::npos && space > start) {
                lines.push_back(str.substr(start, space - start));
                start = space + 1;
            } else {
                lines.push_back(str.substr(start, maxLen));
                start = end;
            }
        }
        return lines;
    };

    for (int i = 0; i < maxVisibleCards; ++i) {
        int evIdx = scrollIndex + i;
        if (evIdx < 0 || evIdx >= static_cast<int>(filtered.size())) break;

        const auto& ev = filtered[evIdx];
        int cX = cardsStartX + i * cardPitch;

        bool cardHov = (mouseX >= cX && mouseX <= cX + cardW && mouseY >= cardY && mouseY <= cardY + cardH);

        // Vástago de conexión vertical desde el carril de la línea de tiempo a la tarjeta
        int stemX = cX + cardW / 2;
        drawLine(stemX, railY + railH, stemX, cardY, Palette::DIVINE_GOLD);
        // Nodo joya en el riel
        drawRect(stemX - 3, railY + 1, 7, 7, ev.badgeColor);
        drawRect(stemX - 1, railY + 3, 3, 3, Palette::DIVINE_GOLD);

        // Cuerpo de la tarjeta con volumen biselado Sims 2
        ColorRGB cardBase = cardHov ? ColorRGB{32, 22, 18} : ColorRGB{24, 16, 13};
        drawSims2Panel(cX, cardY, cardW, cardH, cardBase, false, ev.badgeColor);

        // A. Píldora de Año
        std::string yBadge = "[" + Timeline::formatYear(ev.year) + "]";
        drawSims2Panel(cX + 8, cardY + 8, 86, 18, ColorRGB{40, 28, 16}, false, Palette::DIVINE_GOLD);
        drawText(cX + 12, cardY + 13, yBadge, Palette::DIVINE_GOLD, 1);

        // B. Insignia de Punto de Quiebre
        if (ev.isTurningPoint) {
            drawSims2Panel(cX + 98, cardY + 8, cardW - 106, 18, ColorRGB{80, 16, 20}, false, ColorRGB{255, 65, 65});
            drawText(cX + 104, cardY + 13, "* QUIEBRE *", ColorRGB{255, 230, 120}, 1);
        }

        // C. Marco del Icono Genérico 2D Pixel Art
        int iconCol = 5;
        std::string catLabel = "MONUMENTO";
        switch (ev.category) {
            case EventCategory::MajorLoss:
            case EventCategory::Extinction:
            case EventCategory::BorderConflict:
                iconCol = 0; catLabel = (ev.category == EventCategory::BorderConflict) ? "CONFLICTO FRONTERIZO" : "PERDIDA TRAGICA"; break;
            case EventCategory::EraAdvancement:
                iconCol = 1; catLabel = "AVANCE DE ERA"; break;
            case EventCategory::Discovery:
            case EventCategory::PilgrimageWave:
                iconCol = 2; catLabel = "DESCUBRIMIENTO"; break;
            case EventCategory::DoctrinalSchism:
            case EventCategory::ReligiousReformation:
                iconCol = 3; catLabel = "CISMA DOCTRINAL"; break;
            case EventCategory::SyncreticAgglutination:
            case EventCategory::ExogamicPact:
                iconCol = 4; catLabel = (ev.category == EventCategory::ExogamicPact) ? "PACTO EXOGAMICO" : "FUSION SINCRETICA"; break;
            case EventCategory::HolySiteErected:
            case EventCategory::BirthOfReligion:
            default:
                iconCol = 5; catLabel = "FUNDACION SAGRADA"; break;
        }

        const int iconBoxX = cX + (cardW - 36) / 2;
        const int iconBoxY = cardY + 32;
        drawSims2Panel(iconBoxX, iconBoxY, 36, 36, ColorRGB{16, 10, 8}, true, ev.badgeColor);
        drawEventIcon(iconBoxX + 6, iconBoxY + 6, iconCol, cardHov);

        // Etiqueta de Categoría
        int catTx = cX + (cardW - static_cast<int>(catLabel.size() * 6)) / 2;
        drawText(catTx, cardY + 74, catLabel, ColorRGB{220, 190, 140}, 1);

        // D. Título Específico (hasta 2 líneas)
        auto titleLines = wrapText(ev.title, 22);
        int ty = cardY + 90;
        for (size_t tl = 0; tl < std::min<size_t>(2, titleLines.size()); ++tl) {
            int tX = cX + (cardW - static_cast<int>(titleLines[tl].size() * 6)) / 2;
            drawText(tX, ty, titleLines[tl], Palette::DIVINE_GOLD, 1);
            ty += 11;
        }

        // Línea divisoria ceremonial
        drawLine(cX + 12, cardY + 116, cX + cardW - 12, cardY + 116, ColorRGB{70, 50, 35});

        // E. Narrativa Histórica Específica (hasta 6 líneas)
        auto descLines = wrapText(ev.description, 24);
        int dy = cardY + 124;
        for (size_t dl = 0; dl < std::min<size_t>(6, descLines.size()); ++dl) {
            drawText(cX + 10, dy, descLines[dl], Palette::RELIC_BONE, 1);
            dy += 12;
        }

        // F. Caja de Consecuencia / Punto de Quiebre
        std::string cons = ev.consequence.empty() ? "Hito registrado en la memoria de la fe." : ev.consequence;
        drawSims2Panel(cX + 8, cardY + 224, cardW - 16, 68, ColorRGB{16, 10, 8}, true, ColorRGB{110, 80, 45});
        drawText(cX + 12, cardY + 229, "IMPACTO HISTORICO:", ColorRGB{240, 190, 70}, 1);
        auto consLines = wrapText(cons, 24);
        int cy = cardY + 242;
        for (size_t cl = 0; cl < std::min<size_t>(4, consLines.size()); ++cl) {
            drawText(cX + 12, cy, consLines[cl], ColorRGB{215, 205, 185}, 1);
            cy += 11;
        }

        // G. Protagonista / Contexto
        if (!ev.actorName.empty()) {
            std::string actStr = "Figura: " + ev.actorName.substr(0, 22);
            drawText(cX + 10, cardY + 302, actStr, ColorRGB{190, 180, 160}, 1);
        }

        // H. Botón de Acción "IR AL LUGAR (X, Y)"
        int btnW = cardW - 20;
        int btnH = 24;
        int btnX = cX + 10;
        int btnY = cardY + cardH - 34;

        bool btnHov = (mouseX >= btnX && mouseX <= btnX + btnW && mouseY >= btnY && mouseY <= btnY + btnH);
        bool btnPrs = btnHov && isMouseDown;
        drawSims2Panel(btnX, btnY, btnW, btnH, btnHov ? ColorRGB{55, 40, 20} : ColorRGB{32, 22, 16}, btnPrs, Palette::DIVINE_GOLD);

        std::string locLabel = "IR AL LUGAR (" + std::to_string(ev.location.x) + "," + std::to_string(ev.location.y) + ")";
        int locTx = btnX + (btnW - static_cast<int>(locLabel.size() * 6)) / 2 + (btnPrs ? 1 : 0);
        int locTy = btnY + 7 + (btnPrs ? 1 : 0);
        drawText(locTx, locTy, locLabel, btnHov ? ColorRGB{255, 240, 160} : Palette::DIVINE_GOLD, 1);
    }

    // 7. Mini barra de desplazamiento / Scrubber inferior
    if (maxScroll > 0) {
        const int sTrackX = modalX + 80;
        const int sTrackY = modalY + modalH - 24;
        const int sTrackW = modalW - 160;
        const int sTrackH = 8;
        drawSims2Panel(sTrackX, sTrackY, sTrackW, sTrackH, ColorRGB{16, 10, 8}, true, ColorRGB{100, 75, 45});

        float sProg = static_cast<float>(scrollIndex) / static_cast<float>(maxScroll);
        int thumbW = 36;
        int thumbX = sTrackX + static_cast<int>((sTrackW - thumbW) * sProg);
        drawSims2Bar(thumbX, sTrackY - 1, thumbW, sTrackH + 2, Palette::DIVINE_GOLD);
    }
}

int Renderer::checkChronicleModalClick(float mouseX, float mouseY, const EventLine& eventLine,
                                       int activeFilter, int scrollIndex, Point2i& outJumpTarget) {
    const int modalW = 1140;
    const int modalH = 590;
    const int modalX = (m_width - modalW) / 2;
    const int modalY = (m_height - modalH) / 2;

    // Si el clic fue fuera del modal, retorna 1 para cerrar
    if (mouseX < modalX || mouseX > modalX + modalW || mouseY < modalY || mouseY > modalY + modalH) {
        return 1;
    }

    // Botón Cerrar [X]
    const int closeX = modalX + modalW - 40;
    const int closeY = modalY + 8;
    if (mouseX >= closeX && mouseX <= closeX + 28 && mouseY >= closeY && mouseY <= closeY + 28) {
        return 1;
    }

    // Pestañas de Filtros
    struct FilterTab { int width; };
    std::vector<FilterTab> tabs = {{60}, {155}, {105}, {95}, {140}, {90}, {120}};
    int tabX = modalX + 20;
    const int tabY = modalY + 42;
    const int tabH = 24;

    for (size_t i = 0; i < tabs.size(); ++i) {
        if (mouseX >= tabX && mouseX <= tabX + tabs[i].width && mouseY >= tabY && mouseY <= tabY + tabH) {
            return 10 + static_cast<int>(i); // Retorna 10..16
        }
        tabX += tabs[i].width + 6;
    }

    // Botones de navegación horizontal
    const int navW = 28;
    const int navH = 60;
    const int navLeftX = modalX + 10;
    const int navRightX = modalX + modalW - 38;
    const int navY = modalY + 240;

    if (mouseX >= navLeftX && mouseX <= navLeftX + navW && mouseY >= navY && mouseY <= navY + navH) {
        return 20; // Scroll izquierda
    }
    if (mouseX >= navRightX && mouseX <= navRightX + navW && mouseY >= navY && mouseY <= navY + navH) {
        return 21; // Scroll derecha
    }

    // Botones "IR AL LUGAR" dentro de las tarjetas visibles
    std::vector<GameEvent> filtered = eventLine.getFilteredEvents(activeFilter);
    const int cardW = 246;
    const int cardH = 465;
    const int cardPitch = 264;
    const int cardsStartX = modalX + 44;
    const int cardY = modalY + 90;

    for (int i = 0; i < 4; ++i) {
        int evIdx = scrollIndex + i;
        if (evIdx < 0 || evIdx >= static_cast<int>(filtered.size())) break;

        int cX = cardsStartX + i * cardPitch;
        int btnW = cardW - 20;
        int btnH = 24;
        int btnX = cX + 10;
        int btnY = cardY + cardH - 34;

        if (mouseX >= btnX && mouseX <= btnX + btnW && mouseY >= btnY && mouseY <= btnY + btnH) {
            outJumpTarget = filtered[evIdx].location;
            return 30; // Salto a ubicación
        }
    }

    return 2; // Clic dentro del modal pero en área no interactiva
}

void Renderer::loadFaithTreeIconAssets() {
    std::vector<std::string> iconPaths = {
        "assets/ui/faith_tree_icons.rgba",
        "../assets/ui/faith_tree_icons.rgba",
        "religion_simulator_isometric/assets/ui/faith_tree_icons.rgba",
        "/home/Ivancho/Documentos/religion_simulator_isometric/assets/ui/faith_tree_icons.rgba"
    };

    for (const auto& path : iconPaths) {
        std::ifstream file(path, std::ios::binary);
        if (file.is_open()) {
            file.seekg(0, std::ios::end);
            size_t size = file.tellg();
            file.seekg(0, std::ios::beg);
            if (size == 192 * 48 * 4) {
                std::vector<uint8_t> raw(size);
                file.read(reinterpret_cast<char*>(raw.data()), size);
                m_faithTreeIconPixels.resize(192 * 48);
                m_faithTreeIconW = 192;
                m_faithTreeIconH = 48;
                for (size_t i = 0; i < 192 * 48; ++i) {
                    uint8_t r = raw[i * 4 + 0];
                    uint8_t g = raw[i * 4 + 1];
                    uint8_t b = raw[i * 4 + 2];
                    uint8_t a = raw[i * 4 + 3];
                    m_faithTreeIconPixels[i] = (static_cast<uint32_t>(a) << 24) |
                                               (static_cast<uint32_t>(b) << 16) |
                                               (static_cast<uint32_t>(g) << 8)  |
                                                static_cast<uint32_t>(r);
                }
                std::cout << "[Renderer] Iconos del Arbol de Fe (192x48) cargados exitosamente: " << path << std::endl;
                break;
            }
        }
    }
}

void Renderer::drawFaithTreeIcon(int x, int y, int iconCol, bool glow) {
    if (m_faithTreeIconPixels.size() != 192 * 48) return;
    int col = std::clamp(iconCol, 0, 7);
    int row = glow ? 1 : 0;
    int srcX = col * 24;
    int srcY = row * 24;

    for (int iy = 0; iy < 24; ++iy) {
        int frameY = m_height - 1 - (y + iy);
        if (frameY < 0 || frameY >= m_height) continue;

        for (int ix = 0; ix < 24; ++ix) {
            int frameX = x + ix;
            if (frameX < 0 || frameX >= m_width) continue;

            uint32_t src = m_faithTreeIconPixels[(srcY + iy) * 192 + (srcX + ix)];
            uint8_t a = (src >> 24) & 0xFF;
            if (a == 0) continue;

            if (a == 255) {
                m_framebuffer[frameY * m_width + frameX] = src;
            } else {
                uint32_t dst = m_framebuffer[frameY * m_width + frameX];
                float fa = a / 255.0f;
                uint8_t dr = dst & 0xFF;
                uint8_t dg = (dst >> 8) & 0xFF;
                uint8_t db = (dst >> 16) & 0xFF;
                uint8_t sr = src & 0xFF;
                uint8_t sg = (src >> 8) & 0xFF;
                uint8_t sb = (src >> 16) & 0xFF;
                uint8_t outR = static_cast<uint8_t>(dr * (1.0f - fa) + sr * fa);
                uint8_t outG = static_cast<uint8_t>(dg * (1.0f - fa) + sg * fa);
                uint8_t outB = static_cast<uint8_t>(db * (1.0f - fa) + sb * fa);
                m_framebuffer[frameY * m_width + frameX] = (0xFFu << 24) | (outB << 16) | (outG << 8) | outR;
            }
        }
    }
}

namespace {
    struct AdvSlot {
        int id;
        int col;
        int row;
    };

    static const AdvSlot kAdvSlots[16] = {
        // Era 0: Animismo Primordial
        {101, 0, 0}, {103, 0, 1}, {105, 0, 2}, {100, 0, 3},
        // Era 1: Chamanismo Estructural
        {201, 1, 0}, {203, 1, 1}, {205, 1, 2}, {200, 1, 3},
        // Era 2: Politeísmo Territorial
        {301, 2, 0}, {302, 2, 1}, {303, 2, 2}, {300, 2, 3},
        // Era 3: Religiones Universales
        {401, 3, 0}, {402, 3, 1}, {403, 3, 2}, {400, 3, 3}
    };

    void getAdvCardRect(int modalX, int modalY, int col, int row, int& rx, int& ry, int& rw, int& rh) {
        int startX = modalX + 24;
        rx = startX + col * 290;
        rw = 276;
        if (row == 0)      { ry = modalY + 74;  rh = 68; }
        else if (row == 1) { ry = modalY + 148; rh = 68; }
        else if (row == 2) { ry = modalY + 222; rh = 68; }
        else               { ry = modalY + 296; rh = 76; }
    }
}

void Renderer::renderFaithTreeModal(const ReligionRegistry& registry,
                                   const AdvancementSystem& advSystem,
                                   const WorldCreationConfig& playerConfig,
                                   int selectedAdvId,
                                   float mouseX, float mouseY, bool isMouseDown) {
    (void)isMouseDown;
    // 1. Telón translúcido oscurecedor (Vignette 80% sobre la simulación)
    for (int y = 0; y < m_height; ++y) {
        int fy = m_height - 1 - y;
        for (int x = 0; x < m_width; ++x) {
            uint32_t px = m_framebuffer[fy * m_width + x];
            uint8_t r = static_cast<uint8_t>(((px & 0xFF) * 45) / 255);
            uint8_t g = static_cast<uint8_t>((((px >> 8) & 0xFF) * 45) / 255);
            uint8_t b = static_cast<uint8_t>((((px >> 16) & 0xFF) * 45) / 255);
            m_framebuffer[fy * m_width + x] = (0xFFu << 24) | (b << 16) | (g << 8) | r;
        }
    }

    const int modalW = 1200;
    const int modalH = 630;
    const int modalX = (m_width - modalW) / 2;
    const int modalY = (m_height - modalH) / 2;

    // 2. Marco exterior estilo Sims 2 con acabado litúrgico biselado
    drawSims2Panel(modalX, modalY, modalW, modalH, ColorRGB{18, 12, 10}, false, Palette::DIVINE_GOLD);
    drawRectOutline(modalX + 2, modalY + 2, modalW - 4, modalH - 4, ColorRGB{75, 52, 38});
    drawRectOutline(modalX + 4, modalY + 4, modalW - 8, modalH - 8, ColorRGB{140, 110, 30});

    // Remates angulares ornamentales en Oro Divino
    drawRect(modalX + 2, modalY + 2, 7, 7, Palette::DIVINE_GOLD);
    drawRect(modalX + modalW - 9, modalY + 2, 7, 7, Palette::DIVINE_GOLD);
    drawRect(modalX + 2, modalY + modalH - 9, 7, 7, Palette::DIVINE_GOLD);
    drawRect(modalX + modalW - 9, modalY + modalH - 9, 7, 7, Palette::DIVINE_GOLD);

    // 3. Placa de Cabecera
    const int headX = modalX + 16;
    const int headY = modalY + 8;
    const int headW = modalW - 68;
    const int headH = 34;
    drawSims2Panel(headX, headY, headW, headH, ColorRGB{34, 22, 16}, false, Palette::DIVINE_GOLD);
    drawFaithTreeIcon(headX + 8, headY + 5, 6, true); // Corona iluminada

    const Religion* pRel = registry.getReligion(1);
    int theologyPts = pRel ? pRel->theologyPoints : 0;
    std::string eraStr = pRel ? AdvancementSystem::getEraName(pRel->currentEra) : "Primordial Animism";
    std::string relName = pRel ? pRel->name : playerConfig.religionName;

    drawText(headX + 38, headY + 5, "ARBOL TEOLOGICO DE LA FE: DOCTRINAS, REPLICACION Y ERAS", Palette::DIVINE_GOLD, 1);
    std::string subHeader = "DIOS: " + playerConfig.godName + " | CULTO: " + relName +
                            " | FE/PIEDAD: " + std::to_string(theologyPts) + " pts | ERA: " + eraStr;
    drawText(headX + 38, headY + 18, subHeader.substr(0, 80), Palette::RELIC_BONE, 1);

    // Botón Cerrar [X]
    const int closeX = modalX + modalW - 44;
    const int closeY = modalY + 8;
    const int closeW = 32;
    const int closeH = 34;
    bool closeHover = (mouseX >= closeX && mouseX <= closeX + closeW && mouseY >= closeY && mouseY <= closeY + closeH);
    drawSims2Panel(closeX, closeY, closeW, closeH, ColorRGB{120, 20, 20}, closeHover, Palette::DIVINE_GOLD);
    drawText(closeX + 11, closeY + 11, "X", Palette::RELIC_BONE, 1);

    // 4. Cabeceras de Columnas de Eras
    const char* eraHeaders[4] = {
        "ERA 0: ANIMISMO (-4000)",
        "ERA 1: CHAMANISMO (-3000)",
        "ERA 2: POLITEISMO (-1000)",
        "ERA 3: UNIVERSAL (200)"
    };

    HistoricalEra eras[4] = {
        HistoricalEra::PrimordialAnimism,
        HistoricalEra::StructuralShamanism,
        HistoricalEra::TerritorialPolytheism,
        HistoricalEra::UniversalReligions
    };

    int startX = modalX + 24;
    for (int col = 0; col < 4; ++col) {
        int colX = startX + col * 290;
        int colY = modalY + 46;
        int colW = 276;
        int colH = 22;

        bool eraReached = pRel && (static_cast<int>(pRel->currentEra) >= static_cast<int>(eras[col]));
        ColorRGB hdrBg = eraReached ? ColorRGB{38, 30, 20} : ColorRGB{24, 18, 16};
        ColorRGB hdrTrim = eraReached ? Palette::DIVINE_GOLD : ColorRGB{70, 50, 35};

        drawSims2Panel(colX, colY, colW, colH, hdrBg, false, hdrTrim);
        drawText(colX + 10, colY + 6, eraHeaders[col], eraReached ? Palette::DIVINE_GOLD : ColorRGB{160, 140, 120}, 1);

        if (eraReached) {
            drawText(colX + colW - 74, colY + 6, "[ALCANZADA]", ColorRGB{120, 220, 90}, 1);
        }
    }

    // 5. Cableado de Conexiones de Prerrequisitos entre Nodos
    for (int i = 0; i < 16; ++i) {
        const auto& slot = kAdvSlots[i];
        const FaithAdvancement* adv = advSystem.getAdvancement(slot.id);
        if (!adv) continue;

        int cx, cy, cw, ch;
        getAdvCardRect(modalX, modalY, slot.col, slot.row, cx, cy, cw, ch);

        for (int reqId : adv->prerequisites) {
            // Buscar slot del prerrequisito
            for (int p = 0; p < 16; ++p) {
                if (kAdvSlots[p].id == reqId) {
                    int px, py, pw, ph;
                    getAdvCardRect(modalX, modalY, kAdvSlots[p].col, kAdvSlots[p].row, px, py, pw, ph);

                    bool reqMet = pRel && pRel->hasAdvancement(reqId);
                    ColorRGB wireColor = reqMet ? Palette::DIVINE_GOLD : ColorRGB{65, 45, 30};

                    int srcX = px + pw;
                    int srcY = py + ph / 2;
                    int dstX = cx;
                    int dstY = cy + ch / 2;

                    int midX = (srcX + dstX) / 2;
                    drawLine(srcX, srcY, midX, srcY, wireColor);
                    drawLine(midX, srcY, midX, dstY, wireColor);
                    drawLine(midX, dstY, dstX, dstY, wireColor);
                    break;
                }
            }
        }
    }

    // 6. Tarjetas de Nodos del Árbol de Avances
    for (int i = 0; i < 16; ++i) {
        const auto& slot = kAdvSlots[i];
        const FaithAdvancement* adv = advSystem.getAdvancement(slot.id);
        if (!adv) continue;

        int cx, cy, cw, ch;
        getAdvCardRect(modalX, modalY, slot.col, slot.row, cx, cy, cw, ch);

        bool isUnlocked = pRel && pRel->hasAdvancement(adv->id);
        bool canUnlock = pRel && advSystem.canConvalidate(*pRel, adv->id);
        bool isSelected = (adv->id == selectedAdvId);
        bool isHovered = (mouseX >= cx && mouseX <= cx + cw && mouseY >= cy && mouseY <= cy + ch);

        ColorRGB cardBg = ColorRGB{20, 16, 14};
        ColorRGB cardTrim = ColorRGB{65, 45, 30};

        if (isUnlocked) {
            cardBg = ColorRGB{26, 34, 22};
            cardTrim = Palette::DIVINE_GOLD;
        } else if (canUnlock) {
            cardBg = isHovered ? ColorRGB{45, 32, 18} : ColorRGB{34, 24, 14};
            cardTrim = Palette::DIVINE_GOLD;
        } else {
            cardBg = isHovered ? ColorRGB{26, 20, 18} : ColorRGB{18, 14, 12};
            cardTrim = ColorRGB{55, 40, 30};
        }

        drawSims2Panel(cx, cy, cw, ch, cardBg, isHovered, cardTrim);

        if (isSelected) {
            drawRectOutline(cx + 1, cy + 1, cw - 2, ch - 2, ColorRGB{255, 235, 120});
            drawRectOutline(cx + 2, cy + 2, cw - 4, ch - 4, ColorRGB{255, 235, 120});
        }

        // Icono de Fe
        drawFaithTreeIcon(cx + 6, cy + 8, adv->iconCol, isUnlocked || isSelected);

        // Título del Avance
        ColorRGB titleColor = isUnlocked ? Palette::DIVINE_GOLD : (canUnlock ? ColorRGB{255, 225, 120} : ColorRGB{170, 150, 130});
        drawText(cx + 34, cy + 8, adv->shortTitle.substr(0, 24), titleColor, 1);

        // Resumen Mecánico breve
        std::string brief = adv->mechanicalBonus;
        if (brief.size() > 30) brief = brief.substr(0, 30);
        drawText(cx + 34, cy + 22, brief, ColorRGB{200, 180, 140}, 1);

        // Píldora de Estado al pie de la tarjeta
        int pillX = cx + 34;
        int pillY = cy + ch - 22;
        int pillW = cw - 40;
        int pillH = 16;

        if (isUnlocked) {
            drawSims2Panel(pillX, pillY, pillW, pillH, ColorRGB{25, 55, 25}, false, ColorRGB{100, 200, 80});
            drawText(pillX + (pillW - 108) / 2, pillY + 3, "[ CONVALIDADO ]", ColorRGB{140, 240, 110}, 1);
        } else if (canUnlock) {
            if (adv->isEraCapstone) {
                drawSims2Panel(pillX, pillY, pillW, pillH, ColorRGB{110, 20, 25}, isHovered, Palette::DIVINE_GOLD);
                std::string capStr = "* ASCENDER ERA * (" + std::to_string(adv->faithCost) + " FE)";
                drawText(pillX + 6, pillY + 3, capStr.substr(0, 32), Palette::DIVINE_GOLD, 1);
            } else {
                drawSims2Panel(pillX, pillY, pillW, pillH, ColorRGB{45, 30, 12}, isHovered, Palette::DIVINE_GOLD);
                std::string btnStr = "CONVALIDAR (" + std::to_string(adv->faithCost) + " FE)";
                drawText(pillX + (pillW - static_cast<int>(btnStr.size() * 6)) / 2, pillY + 3, btnStr, Palette::DIVINE_GOLD, 1);
            }
        } else {
            bool hasPrereqs = true;
            for (int reqId : adv->prerequisites) {
                if (!pRel || !pRel->hasAdvancement(reqId)) { hasPrereqs = false; break; }
            }

            if (!hasPrereqs) {
                drawSims2Panel(pillX, pillY, pillW, pillH, ColorRGB{20, 15, 14}, false, ColorRGB{55, 40, 32});
                drawText(pillX + (pillW - 78) / 2, pillY + 3, "[ BLOQUEADO ]", ColorRGB{140, 120, 100}, 1);
            } else {
                drawSims2Panel(pillX, pillY, pillW, pillH, ColorRGB{45, 18, 18}, false, ColorRGB{110, 45, 35});
                std::string lackStr = "FALTA FE (" + std::to_string(theologyPts) + "/" + std::to_string(adv->faithCost) + ")";
                drawText(pillX + (pillW - static_cast<int>(lackStr.size() * 6)) / 2, pillY + 3, lackStr.substr(0, 32), ColorRGB{255, 130, 110}, 1);
            }
        }
    }

    // 7. Panel Inferior de Inspección y Convalidación (Detail Inspector)
    const int panelX = modalX + 24;
    const int panelY = modalY + 386;
    const int panelW = 1146;
    const int panelH = 228;

    drawSims2Panel(panelX, panelY, panelW, panelH, ColorRGB{22, 15, 12}, false, Palette::DIVINE_GOLD);
    drawRectOutline(panelX + 2, panelY + 2, panelW - 4, panelH - 4, ColorRGB{75, 52, 38});

    const FaithAdvancement* selAdv = advSystem.getAdvancement(selectedAdvId);
    if (!selAdv) selAdv = advSystem.getAdvancement(101);

    if (selAdv) {
        // --- Sub-panel Izquierdo de Detalles (Ancho: 760px) ---
        drawFaithTreeIcon(panelX + 16, panelY + 12, selAdv->iconCol, true);
        drawText(panelX + 48, panelY + 14, selAdv->name, Palette::DIVINE_GOLD, 1);

        std::string branchTag = "[ RAMA: REPLICACION Y DIFUSION ]";
        ColorRGB tagColor = Palette::DIVINE_GOLD;
        if (selAdv->branch == FaithBranch::HarvestDemographics) {
            branchTag = "[ RAMA: DEMOGRAFIA Y COSECHAS ]";
            tagColor = ColorRGB{120, 210, 90};
        } else if (selAdv->branch == FaithBranch::DogmaCohesion) {
            branchTag = "[ RAMA: DOGMA Y COHESION ]";
            tagColor = ColorRGB{100, 170, 255};
        } else if (selAdv->branch == FaithBranch::EraCapstone) {
            branchTag = "[ HITO HISTORICO DE ASCENSION DE ERA ]";
            tagColor = ColorRGB{255, 80, 110};
        }

        std::string metaLine = branchTag + " | " + AdvancementSystem::getEraName(selAdv->era) +
                               " | Costo: " + std::to_string(selAdv->faithCost) + " Puntos de Fe";
        drawText(panelX + 48, panelY + 28, metaLine.substr(0, 80), tagColor, 1);

        // Narrativa Teológica / Lore
        drawText(panelX + 16, panelY + 48, "TRADICION SAGRADA:", ColorRGB{180, 150, 100}, 1);
        drawText(panelX + 16, panelY + 62, selAdv->description, Palette::RELIC_BONE, 1);

        // Cuadro de Bonificación Mecánica Concreta en la Simulación
        drawSims2Panel(panelX + 16, panelY + 88, 740, 48, ColorRGB{32, 22, 16}, false, Palette::DIVINE_GOLD);
        drawText(panelX + 24, panelY + 96, "EFECTO DIRECTO EN LA SIMULACION Y REPLICACION:", Palette::DIVINE_GOLD, 1);
        drawText(panelX + 24, panelY + 112, selAdv->mechanicalBonus, ColorRGB{255, 235, 140}, 1);

        // Prerrequisitos y Estado
        drawText(panelX + 16, panelY + 148, "REQUISITOS PREVIOS:", ColorRGB{180, 150, 100}, 1);
        if (selAdv->prerequisites.empty()) {
            drawText(panelX + 140, panelY + 148, "Ninguno (Doctrina Fundacional Raiz de la Fe)", ColorRGB{120, 210, 90}, 1);
        } else {
            std::string reqStr;
            for (size_t rIdx = 0; rIdx < selAdv->prerequisites.size(); ++rIdx) {
                int rId = selAdv->prerequisites[rIdx];
                const FaithAdvancement* pr = advSystem.getAdvancement(rId);
                std::string prName = pr ? pr->shortTitle : ("#" + std::to_string(rId));
                bool hasIt = pRel && pRel->hasAdvancement(rId);
                reqStr += (hasIt ? "[OK] " : "[X] ") + prName;
                if (rIdx + 1 < selAdv->prerequisites.size()) reqStr += " + ";
            }
            drawText(panelX + 140, panelY + 148, reqStr.substr(0, 72), ColorRGB{220, 200, 160}, 1);
        }

        // --- Sub-panel Derecho: Botón de Acción y Progreso de Era (Ancho: 340px) ---
        drawLine(panelX + 780, panelY + 8, panelX + 780, panelY + panelH - 8, ColorRGB{75, 52, 38});

        const int btnX = panelX + 796;
        const int btnY = panelY + 36;
        const int btnW = 330;
        const int btnH = 52;

        bool isUnlocked = pRel && pRel->hasAdvancement(selAdv->id);
        bool canUnlock = pRel && advSystem.canConvalidate(*pRel, selAdv->id);
        bool btnHover = (mouseX >= btnX && mouseX <= btnX + btnW && mouseY >= btnY && mouseY <= btnY + btnH);

        if (isUnlocked) {
            drawSims2Panel(btnX, btnY, btnW, btnH, ColorRGB{25, 55, 25}, false, ColorRGB{100, 200, 80});
            drawText(btnX + (btnW - 162) / 2, btnY + 18, "CONVALIDADO (ACTIVO)", ColorRGB{140, 240, 110}, 1);
        } else if (canUnlock) {
            if (selAdv->isEraCapstone) {
                drawSims2Panel(btnX, btnY, btnW, btnH, ColorRGB{130, 20, 30}, btnHover, Palette::DIVINE_GOLD);
                std::string actionStr = "! ASCENDER DE ERA ! (" + std::to_string(selAdv->faithCost) + " FE)";
                drawText(btnX + (btnW - static_cast<int>(actionStr.size() * 6)) / 2, btnY + 18, actionStr, Palette::DIVINE_GOLD, 1);
            } else {
                drawSims2Panel(btnX, btnY, btnW, btnH, ColorRGB{55, 36, 16}, btnHover, Palette::DIVINE_GOLD);
                std::string actionStr = "CONVALIDAR AVANCE (" + std::to_string(selAdv->faithCost) + " FE)";
                drawText(btnX + (btnW - static_cast<int>(actionStr.size() * 6)) / 2, btnY + 18, actionStr, Palette::DIVINE_GOLD, 1);
            }
        } else {
            bool hasPrereqs = true;
            for (int reqId : selAdv->prerequisites) {
                if (!pRel || !pRel->hasAdvancement(reqId)) { hasPrereqs = false; break; }
            }

            if (!hasPrereqs) {
                drawSims2Panel(btnX, btnY, btnW, btnH, ColorRGB{24, 18, 16}, false, ColorRGB{60, 45, 35});
                drawText(btnX + (btnW - 174) / 2, btnY + 18, "BLOQUEADO (FALTAN REQUISITOS)", ColorRGB{160, 140, 120}, 1);
            } else {
                drawSims2Panel(btnX, btnY, btnW, btnH, ColorRGB{48, 20, 20}, false, ColorRGB{120, 50, 40});
                std::string lackStr = "FE INSUFICIENTE (" + std::to_string(theologyPts) + "/" + std::to_string(selAdv->faithCost) + ")";
                drawText(btnX + (btnW - static_cast<int>(lackStr.size() * 6)) / 2, btnY + 18, lackStr.substr(0, 36), ColorRGB{255, 120, 100}, 1);
            }
        }

        // Progreso en la Era Actual de la Civilización
        HistoricalEra curEra = pRel ? pRel->currentEra : HistoricalEra::PrimordialAnimism;
        int eraAdvCount = pRel ? advSystem.getConvalidatedCountForEra(*pRel, curEra) : 0;
        std::string progStr = "Progreso en " + AdvancementSystem::getEraName(curEra) + ": " +
                              std::to_string(eraAdvCount) + " / 4 avances";
        drawText(btnX + 4, panelY + 115, progStr.substr(0, 50), Palette::RELIC_BONE, 1);

        // Barra de progreso de Era
        int barX = btnX + 4;
        int barY = panelY + 132;
        int barW = btnW - 8;
        int barH = 14;
        drawRect(barX, barY, barW, barH, ColorRGB{15, 10, 8});
        drawRectOutline(barX, barY, barW, barH, Palette::DIVINE_GOLD);

        float fillPct = std::clamp(eraAdvCount / 4.0f, 0.0f, 1.0f);
        int fillW = static_cast<int>((barW - 2) * fillPct);
        if (fillW > 0) {
            drawRect(barX + 1, barY + 1, fillW, barH - 2, ColorRGB{120, 180, 50});
        }
    }
}

int Renderer::checkFaithTreeModalClick(float mouseX, float mouseY,
                                      const ReligionRegistry& registry,
                                      const AdvancementSystem& advSystem,
                                      int& outSelectedAdvId) {
    const int modalW = 1200;
    const int modalH = 630;
    const int modalX = (m_width - modalW) / 2;
    const int modalY = (m_height - modalH) / 2;

    // 1. Botón Cerrar [X]
    const int closeX = modalX + modalW - 44;
    const int closeY = modalY + 8;
    const int closeW = 32;
    const int closeH = 34;
    if (mouseX >= closeX && mouseX <= closeX + closeW && mouseY >= closeY && mouseY <= closeY + closeH) {
        return 1; // Cerrar modal
    }

    // 2. Clic fuera del modal -> cerrar
    if (mouseX < modalX || mouseX > modalX + modalW || mouseY < modalY || mouseY > modalY + modalH) {
        return 1;
    }

    const Religion* pRel = registry.getReligion(1);

    // 3. Botón de Acción Principal "CONVALIDAR AVANCE" en el panel inferior
    const int panelX = modalX + 24;
    const int panelY = modalY + 386;
    const int btnX = panelX + 796;
    const int btnY = panelY + 36;
    const int btnW = 330;
    const int btnH = 52;

    if (mouseX >= btnX && mouseX <= btnX + btnW && mouseY >= btnY && mouseY <= btnY + btnH) {
        if (pRel && advSystem.canConvalidate(*pRel, outSelectedAdvId)) {
            return 2; // Convalidar avance seleccionado
        }
        return 0; // Clic en botón deshabilitado
    }

    // 4. Clic sobre cualquiera de las 16 tarjetas de avances
    for (int i = 0; i < 16; ++i) {
        const auto& slot = kAdvSlots[i];
        int cx, cy, cw, ch;
        getAdvCardRect(modalX, modalY, slot.col, slot.row, cx, cy, cw, ch);

        if (mouseX >= cx && mouseX <= cx + cw && mouseY >= cy && mouseY <= cy + ch) {
            int prevSelected = outSelectedAdvId;
            outSelectedAdvId = slot.id;

            // Si se hace clic directamente sobre la píldora inferior y ya estaba seleccionado
            int pillX = cx + 34;
            int pillY = cy + ch - 22;
            int pillW = cw - 40;
            int pillH = 16;
            if (prevSelected == slot.id && mouseX >= pillX && mouseX <= pillX + pillW &&
                mouseY >= pillY && mouseY <= pillY + pillH) {
                if (pRel && advSystem.canConvalidate(*pRel, slot.id)) {
                    return 2; // Convalidar directamente
                }
            }
            return 100 + slot.id; // Seleccionar tarjeta
        }
    }

    return 0; // Absorber clics dentro del modal
}

// =========================================================================
// VENTANA MODAL DEL ÁRBOL DE RECUERDOS (SISTEMA NÉMESIS & SÍSIFO)
// =========================================================================
void Renderer::renderMemoryTreeModal(const GestorHumanos& humanos, const ReligionRegistry& registry,
                                     const Timeline& timeline, float mouseX, float mouseY, bool isMouseDown) {
    (void)isMouseDown;
    // 1. Telón translúcido oscurecedor (Vignette litúrgica sobre la simulación activa)
    for (int y = 0; y < m_height; ++y) {
        int fy = m_height - 1 - y;
        for (int x = 0; x < m_width; ++x) {
            uint32_t px = m_framebuffer[fy * m_width + x];
            uint8_t r = static_cast<uint8_t>(((px & 0xFF) * 45) / 255);
            uint8_t g = static_cast<uint8_t>((((px >> 8) & 0xFF) * 45) / 255);
            uint8_t b = static_cast<uint8_t>((((px >> 16) & 0xFF) * 45) / 255);
            m_framebuffer[fy * m_width + x] = (0xFFu << 24) | (b << 16) | (g << 8) | r;
        }
    }

    const int modalW = 1140;
    const int modalH = 620;
    const int modalX = (m_width - modalW) / 2;
    const int modalY = (m_height - modalH) / 2;

    // 2. Marco exterior estilo Sims 2 con volumen de basalto y bordes dorados
    drawSims2Panel(modalX, modalY, modalW, modalH, ColorRGB{18, 12, 10}, false, Palette::DIVINE_GOLD);
    drawRectOutline(modalX + 2, modalY + 2, modalW - 4, modalH - 4, ColorRGB{75, 52, 38});
    drawRectOutline(modalX + 4, modalY + 4, modalW - 8, modalH - 8, ColorRGB{140, 110, 30});

    // Remates angulares dorados
    drawRect(modalX + 2, modalY + 2, 7, 7, Palette::DIVINE_GOLD);
    drawRect(modalX + modalW - 9, modalY + 2, 7, 7, Palette::DIVINE_GOLD);
    drawRect(modalX + 2, modalY + modalH - 9, 7, 7, Palette::DIVINE_GOLD);
    drawRect(modalX + modalW - 9, modalY + modalH - 9, 7, 7, Palette::DIVINE_GOLD);

    // 3. Placa de Cabecera
    const int headX = modalX + 16;
    const int headY = modalY + 8;
    const int headW = modalW - 68;
    const int headH = 34;
    drawSims2Panel(headX, headY, headW, headH, ColorRGB{34, 22, 16}, false, Palette::DIVINE_GOLD);

    // Identificar la unidad inspeccionada
    const Humano* target = humanos.getHumanoSeleccionado();
    if (!target) {
        int lId = humanos.buscarLiderEspiritualDeReligion(playerReligionId());
        if (lId > 0) {
            for (const auto& h : humanos.getHumanos()) {
                if (h.getId() == lId) {
                    target = &h;
                    break;
                }
            }
        }
    }

    std::string unitName = target ? target->getNombre() : "SIN SELECCION";
    const Nemesis::NemesisProfile* profile = (target && m_nemesisSififus) ? m_nemesisSififus->getProfileOrNull(target->getId()) : nullptr;
    std::string epithetStr = profile ? profile->epithet : "Habitante Primordial";

    drawText(headX + 12, headY + 5, "ARBOL DE RECUERDOS Y VIVENCIAS : SISTEMA NEMESIS & SISIFO", Palette::DIVINE_GOLD, 1);
    std::string subHeader = "SUJETO: " + unitName + " (\"" + epithetStr + "\") | ANO SIMULADO: " +
                            std::to_string(std::abs(timeline.getCurrentYear())) + (timeline.getCurrentYear() < 0 ? " a.C." : " d.C.");
    drawText(headX + 12, headY + 18, subHeader.substr(0, 90), Palette::RELIC_BONE, 1);

    // Botón Cerrar [X]
    const int closeX = modalX + modalW - 44;
    const int closeY = modalY + 8;
    const int closeW = 32;
    const int closeH = 34;
    bool closeHover = (mouseX >= closeX && mouseX <= closeX + closeW && mouseY >= closeY && mouseY <= closeY + closeH);
    drawSims2Panel(closeX, closeY, closeW, closeH, ColorRGB{120, 20, 20}, closeHover, Palette::DIVINE_GOLD);
    drawText(closeX + 11, closeY + 11, "X", Palette::RELIC_BONE, 1);

    if (!target) {
        // Mensaje cuando no hay unidad seleccionada
        int msgY = modalY + 180;
        drawSims2Panel(modalX + 180, msgY, modalW - 360, 200, ColorRGB{26, 18, 14}, false, Palette::DIVINE_GOLD);
        drawText(modalX + 220, msgY + 30, "NO HAY NINGUNA UNIDAD VIVA SELECCIONADA", Palette::DIVINE_GOLD, 1);
        drawText(modalX + 220, msgY + 60, "Selecciona a cualquier aldeano, chaman, profeta o", Palette::RELIC_BONE, 1);
        drawText(modalX + 220, msgY + 80, "guerrero en el mapa para ver su arbol de memorias,", Palette::RELIC_BONE, 1);
        drawText(modalX + 220, msgY + 100, "rencores individuales y los mitos orales de Sisifo.", Palette::RELIC_BONE, 1);

        int bCloseX = modalX + (modalW - 160) / 2;
        int bCloseY = msgY + 140;
        bool bCloseHov = (mouseX >= bCloseX && mouseX <= bCloseX + 160 && mouseY >= bCloseY && mouseY <= bCloseY + 28);
        drawAntiqueButton(bCloseX, bCloseY, 160, 28, "CERRAR", false, bCloseHov, false, 1);
        return;
    }

    const Religion* rel = registry.getReligion(target->getReligionId());
    const Sififus::SififusEngine* sififus = m_nemesisSififus ? &m_nemesisSififus->getSififus() : nullptr;

    const int contentY = modalY + 48;
    const int contentH = modalH - 96;

    // =========================================================================
    // COLUMNA 1: IDENTIDAD & PERFIL NÉMESIS (Ancho 320px)
    // =========================================================================
    const int col1X = modalX + 16;
    const int col1W = 320;
    drawSims2Panel(col1X, contentY, col1W, contentH, ColorRGB{24, 16, 14}, false, ColorRGB{95, 68, 48});
    drawSims2Panel(col1X, contentY, col1W, 24, ColorRGB{38, 24, 18}, false, Palette::DIVINE_GOLD);
    drawText(col1X + 8, contentY + 6, "PERFIL PSICOLOGICO Y NEMESIS", Palette::DIVINE_GOLD, 1);

    int curY = contentY + 32;

    // Nombre y Título
    drawText(col1X + 10, curY, ("NOMBRE: " + target->getNombre()).substr(0, 48), Palette::RELIC_BONE, 1);
    curY += 16;
    drawText(col1X + 10, curY, ("EPITETO: \"" + epithetStr + "\"").substr(0, 48), ColorRGB{255, 230, 110}, 1);
    curY += 18;

    // Rol, Género y Religión
    std::string roleTag = "ALDEANO";
    if (target->esNino()) {
        roleTag = target->esMujer() ? "NINA" : "NINO";
    } else if (target->esMujer()) {
        roleTag = "ALDEANA";
    }
    switch (target->getHumanUnit().role) {
        case CommunityRole::FORAGER: roleTag = target->esMujer() ? "RECOLECTORA" : "RECOLECTOR"; break;
        case CommunityRole::SHAMAN:  roleTag = target->esMujer() ? "SACERDOTISA" : "CHAMAN TRIBAL"; break;
        case CommunityRole::PROPHET: roleTag = "PROFETA SAGRADO"; break;
        case CommunityRole::ELDER:   roleTag = target->esMujer() ? "ANCIANA SABIA" : "ANCIANO SABIO"; break;
        case CommunityRole::ZEALOT:  roleTag = target->esMujer() ? "FANATICA / GUERRERA" : "FANATICO / GUERRERO"; break;
    }
    std::string genStr = target->esNino() ? "Infante" : (target->esMujer() ? "Mujer" : "Hombre");
    drawText(col1X + 10, curY, ("ROL: " + roleTag + " (" + genStr + ")").substr(0, 48), Palette::RELIC_BONE, 1);
    curY += 15;

    std::string famStr = "LINAJE: ";
    if (target->tienePareja()) {
        famStr += "Casado(a) Amor:" + std::to_string(static_cast<int>(target->getAmorHaciaPareja() * 100.0f)) + "%";
    } else {
        famStr += "Soltero(a)";
    }
    famStr += " | Hij:" + std::to_string(target->getHijosCount());
    if (target->estaEmbarazada()) {
        famStr += " [Gestando " + std::to_string(static_cast<int>(target->getEmbarazoProgreso() * 100.0f)) + "%]";
    }
    drawText(col1X + 10, curY, famStr.substr(0, 48), ColorRGB{255, 205, 160}, 1);
    curY += 15;

    std::string relName = rel ? rel->name : "Animismo Silvestre";
    drawText(col1X + 10, curY, ("FE: " + relName).substr(0, 48), rel ? rel->color : Palette::DIVINE_GOLD, 1);
    curY += 18;

    // Salud y Devoción
    drawText(col1X + 10, curY, "SALUD:", Palette::RELIC_BONE, 1);
    ColorRGB hpCol = (target->getSalud() > 50.0f) ? ColorRGB{46, 215, 80} : ColorRGB{220, 50, 50};
    drawSims2Meter(col1X + 60, curY, col1W - 74, 8, target->getSalud() / 100.0f, hpCol, ColorRGB{12, 7, 6});
    curY += 16;

    drawText(col1X + 10, curY, "DEVOCION:", Palette::RELIC_BONE, 1);
    drawSims2Meter(col1X + 80, curY, col1W - 94, 8, target->getDevocion(), Palette::DIVINE_GOLD, ColorRGB{12, 7, 6});
    curY += 22;

    // Rango Némesis & Personalidad
    const char* rankName = "Aldeano Ordinario (Rango 0)";
    ColorRGB rankCol = ColorRGB{180, 180, 180};
    if (profile) {
        switch (profile->rank) {
            case Nemesis::NemesisRank::Peasant:      rankName = "Aldeano Ordinario (Rango 0)"; rankCol = ColorRGB{180, 180, 180}; break;
            case Nemesis::NemesisRank::Survivor:     rankName = "Sobreviviente Curtido (Rango 1)"; rankCol = ColorRGB{100, 215, 100}; break;
            case Nemesis::NemesisRank::Avenger:      rankName = "Vengador Juramentado (Rango 2)"; rankCol = ColorRGB{240, 140, 50}; break;
            case Nemesis::NemesisRank::Warlord:      rankName = "Caudillo Belico (Rango 3)"; rankCol = ColorRGB{230, 60, 50}; break;
            case Nemesis::NemesisRank::Heresiarch:   rankName = "Heresiarca Cismatico (Rango 4)"; rankCol = ColorRGB{190, 80, 230}; break;
            case Nemesis::NemesisRank::LivingLegend: rankName = "Leyenda Viviente (Rango 5)"; rankCol = Palette::DIVINE_GOLD; break;
        }
    }
    drawText(col1X + 10, curY, "RANGO NEMESIS:", Palette::RELIC_BONE, 1);
    curY += 14;
    drawSims2Panel(col1X + 10, curY, col1W - 20, 20, ColorRGB{20, 14, 12}, false, rankCol);
    drawText(col1X + 16, curY + 4, rankName, rankCol, 1);
    curY += 26;

    const char* persName = "Devoto Comunitario";
    if (profile) {
        switch (profile->personality) {
            case Nemesis::NemesisPersonality::ZealotVengeful:   persName = "Fanatico Vengativo"; break;
            case Nemesis::NemesisPersonality::ApostateReformer: persName = "Apostata Reformador"; break;
            case Nemesis::NemesisPersonality::BloodswornBrute:  persName = "Juramentado de Sangre"; break;
            case Nemesis::NemesisPersonality::FanaticDevotee:   persName = "Devoto por Gracia Santa"; break;
        }
    }
    drawText(col1X + 10, curY, ("PERSONALIDAD: " + std::string(persName)).substr(0, 32), Palette::RELIC_BONE, 1);
    curY += 20;

    // Matriz de Rencor (Grudge Bar)
    drawText(col1X + 10, curY, "RENCOR VS JUGADOR / DIOS:", Palette::RELIC_BONE, 1);
    curY += 14;
    float gp = profile ? profile->grudgePlayer : 0.0f;
    int barW = col1W - 24;
    int barX = col1X + 12;
    drawRect(barX, curY, barW, 10, ColorRGB{15, 10, 8});
    drawRectOutline(barX, curY, barW, 10, ColorRGB{80, 60, 45});
    int midX = barX + barW / 2;
    drawLine(midX, curY, midX, curY + 10, Palette::RELIC_BONE);
    if (gp < 0.0f) {
        int fill = static_cast<int>((barW / 2) * std::clamp(-gp, 0.0f, 1.0f));
        drawRect(midX - fill, curY + 1, fill, 8, ColorRGB{80, 200, 255}); // Lealtad / Gracia
    } else if (gp > 0.0f) {
        int fill = static_cast<int>((barW / 2) * std::clamp(gp, 0.0f, 1.0f));
        drawRect(midX, curY + 1, fill, 8, Palette::BLOOD_RUBY); // Rencor Deicida
    }
    curY += 14;
    drawText(barX, curY, "< Lealtad", ColorRGB{100, 200, 255}, 1);
    drawText(barX + barW - 54, curY, "Rencor >", Palette::BLOOD_RUBY, 1);
    curY += 22;

    // Atributos de Ascenso
    float combatP = profile ? profile->combatProwess : 1.0f;
    float eloq = profile ? profile->eloquence : 1.0f;
    float zeal = profile ? profile->zealotry : 1.0f;

    char bufStats[64];
    std::snprintf(bufStats, sizeof(bufStats), "Fuerza: %.2fx | Eloc: %.2fx | Fanat: %.2fx", combatP, eloq, zeal);
    drawText(col1X + 10, curY, "MODIFICADORES DE ASCENSO:", Palette::DIVINE_GOLD, 1);
    curY += 14;
    drawText(col1X + 10, curY, bufStats, Palette::RELIC_BONE, 1);

    // =========================================================================
    // COLUMNA 2: ÁRBOL DE RECUERDOS (MEMORIA VIVENCIAL) (Ancho 430px)
    // =========================================================================
    const int col2X = col1X + col1W + 12;
    const int col2W = 430;
    drawSims2Panel(col2X, contentY, col2W, contentH, ColorRGB{20, 14, 12}, false, ColorRGB{95, 68, 48});
    drawSims2Panel(col2X, contentY, col2W, 24, ColorRGB{38, 24, 18}, false, Palette::DIVINE_GOLD);
    drawText(col2X + 8, contentY + 6, "ARBOL DE RECUERDOS (MEMORIA VIVENCIAL)", Palette::DIVINE_GOLD, 1);

    // Botones de scroll en cabecera de la columna 2
    const int upX = col2X + col2W - 54;
    const int upY = contentY + 3;
    bool upHov = (mouseX >= upX && mouseX <= upX + 22 && mouseY >= upY && mouseY <= upY + 18);
    drawSims2Panel(upX, upY, 22, 18, ColorRGB{30, 20, 16}, upHov, Palette::DIVINE_GOLD);
    drawText(upX + 6, upY + 3, "^", Palette::RELIC_BONE, 1);

    const int downX = upX + 26;
    bool downHov = (mouseX >= downX && mouseX <= downX + 22 && mouseY >= upY && mouseY <= upY + 18);
    drawSims2Panel(downX, upY, 22, 18, ColorRGB{30, 20, 16}, downHov, Palette::DIVINE_GOLD);
    drawText(downX + 6, upY + 5, "v", Palette::RELIC_BONE, 1);

    size_t memCount = profile ? profile->memory.getEventCount() : 0;
    if (memCount == 0) {
        int noMemY = contentY + 140;
        drawText(col2X + 24, noMemY, "ESTA UNIDAD NO HA REGISTRADO TRAUMAS NI", Palette::RELIC_BONE, 1);
        drawText(col2X + 24, noMemY + 20, "MILAGROS DIRECTOS EN SU MEMORIA.", Palette::RELIC_BONE, 1);
        drawText(col2X + 24, noMemY + 50, "Vive en paz animista ordinaria, libre de rencores", ColorRGB{180, 160, 140}, 1);
        drawText(col2X + 24, noMemY + 70, "o devociones fanatizadas. Al sobrevivir a duelos", ColorRGB{180, 160, 140}, 1);
        drawText(col2X + 24, noMemY + 90, "o rayos divinos, se forjara su arbol de recuerdos.", ColorRGB{180, 160, 140}, 1);
    } else {
        const int trunkX = col2X + 32;
        drawLine(trunkX, contentY + 36, trunkX, contentY + contentH - 16, ColorRGB{130, 100, 60});

        int maxVis = 5;
        int maxStart = (memCount > static_cast<size_t>(maxVis)) ? static_cast<int>(memCount - maxVis) : 0;
        int startIdx = std::clamp(m_memoryTreeScrollIndex, 0, maxStart);

        int nodeY = contentY + 36;
        for (int i = 0; i < maxVis && (startIdx + i) < static_cast<int>(memCount); ++i) {
            const Nemesis::LifeEvent& ev = profile->memory.getEvent(startIdx + i);

            // Nodo en el tronco
            drawRect(trunkX - 4, nodeY + 16, 9, 9, Palette::DIVINE_GOLD);
            drawRect(trunkX - 2, nodeY + 18, 5, 5, ColorRGB{15, 10, 8});
            drawLine(trunkX + 5, nodeY + 20, col2X + 50, nodeY + 20, ColorRGB{130, 100, 60});

            // Tarjeta del evento
            int cardX = col2X + 50;
            int cardW = col2W - 62;
            int cardH = 68;

            const char* evTitle = "EVENTO DE VIDA";
            ColorRGB evCol = Palette::DIVINE_GOLD;
            switch (ev.type) {
                case Nemesis::MemoryEventType::SurvivedCombat:
                    evTitle = "SUPERVIVENCIA A COMBATE";
                    evCol = ColorRGB{225, 75, 75};
                    break;
                case Nemesis::MemoryEventType::WoundedByDivinePower:
                    evTitle = "CASTIGO DIVINO SOBREVIVIDO";
                    evCol = ColorRGB{255, 215, 60};
                    break;
                case Nemesis::MemoryEventType::SparedByPlayer:
                    evTitle = "MILAGRO Y GRACIA CELESTIAL";
                    evCol = ColorRGB{100, 220, 255};
                    break;
                case Nemesis::MemoryEventType::WitnessedSacrilege:
                    evTitle = "SACRILEGIO PRESENCIADO";
                    evCol = ColorRGB{200, 110, 230};
                    break;
                case Nemesis::MemoryEventType::BetrayedByLeader:
                    evTitle = "TRAICION Y DESTIERRO";
                    evCol = ColorRGB{240, 140, 50};
                    break;
                case Nemesis::MemoryEventType::RivalKilledKinsman:
                    evTitle = "MUERTE DE PARIENTE EN COMBATE";
                    evCol = ColorRGB{215, 50, 50};
                    break;
                case Nemesis::MemoryEventType::StoleHolyRelic:
                    evTitle = "SAQUEO DE RELIQUIA SAGRADA";
                    evCol = ColorRGB{230, 190, 70};
                    break;
                case Nemesis::MemoryEventType::Excommunicated:
                    evTitle = "ANATEMA Y EXCOMUNION";
                    evCol = ColorRGB{170, 170, 170};
                    break;
            }

            drawSims2Panel(cardX, nodeY, cardW, cardH, ColorRGB{28, 18, 16}, false, evCol);
            drawText(cardX + 8, nodeY + 6, evTitle, evCol, 1);

            char bufYear[48];
            std::snprintf(bufYear, sizeof(bufYear), "Ano: %d %s | Severidad: %u/255",
                          std::abs(ev.timestampYear), (ev.timestampYear < 0 ? "a.C." : "d.C."), ev.severity);
            drawText(cardX + 8, nodeY + 22, bufYear, Palette::RELIC_BONE, 1);

            char bufDelta[48];
            std::snprintf(bufDelta, sizeof(bufDelta), "Impacto Rencor: %+.2f | Fac: #%d",
                          ev.grudgeDelta, ev.targetReligionId);
            drawText(cardX + 8, nodeY + 38, bufDelta,
                     ev.grudgeDelta > 0.0f ? Palette::BLOOD_RUBY : ColorRGB{100, 220, 255}, 1);

            // Barra de severidad
            drawSims2Meter(cardX + 8, nodeY + 54, cardW - 16, 6, ev.severity / 255.0f, evCol, ColorRGB{12, 7, 6});

            nodeY += cardH + 10;
        }
    }

    // =========================================================================
    // COLUMNA 3: SISTEMA SÍSIFO — RELATOS Y ETHOS (Ancho restante)
    // =========================================================================
    const int col3X = col2X + col2W + 12;
    const int col3W = modalW - (col3X - modalX) - 16;
    drawSims2Panel(col3X, contentY, col3W, contentH, ColorRGB{24, 16, 14}, false, ColorRGB{95, 68, 48});
    drawSims2Panel(col3X, contentY, col3W, 24, ColorRGB{38, 24, 18}, false, Palette::DIVINE_GOLD);
    drawText(col3X + 8, contentY + 6, "RELATOS COLECTIVOS (SISIFO)", Palette::DIVINE_GOLD, 1);

    int sY = contentY + 32;

    // Buscar mito activo vinculado a esta unidad o a su clan
    const Sififus::MythRecord* selMyth = nullptr;
    if (sififus) {
        for (const auto& m : sififus->getActiveMyths()) {
            if (m.protagonistEntityId == target->getId()) {
                selMyth = &m;
                break;
            }
        }
        if (!selMyth) {
            for (const auto& m : sififus->getActiveMyths()) {
                if (m.originReligionId == target->getReligionId()) {
                    selMyth = &m;
                    break;
                }
            }
        }
    }

    if (selMyth) {
        drawText(col3X + 10, sY, "CANTO HEROICO / RELATO ACTIVO:", Palette::DIVINE_GOLD, 1);
        sY += 16;
        drawText(col3X + 10, sY, selMyth->title, Palette::DIVINE_GOLD, 1);
        sY += 18;

        // Cuadro de cita oral en pergamino oscuro
        int chantH = 64;
        drawSims2Panel(col3X + 8, sY, col3W - 16, chantH, ColorRGB{14, 9, 8}, true, ColorRGB{70, 50, 35});
        std::string chant = "\"" + std::string(selMyth->oralSnippet) + "\"";
        drawText(col3X + 14, sY + 8, chant.substr(0, 38), ColorRGB{255, 240, 180}, 1);
        if (chant.size() > 38) {
            drawText(col3X + 14, sY + 22, chant.substr(38, 38), ColorRGB{255, 240, 180}, 1);
        }
        if (chant.size() > 76) {
            drawText(col3X + 14, sY + 36, chant.substr(76, 38), ColorRGB{255, 240, 180}, 1);
        }
        sY += chantH + 12;

        // Resonancia del relato y creyentes
        drawText(col3X + 10, sY, "RESONANCIA CULTURAL:", Palette::RELIC_BONE, 1);
        drawSims2Meter(col3X + 10, sY + 14, col3W - 20, 8, selMyth->resonance, Palette::DIVINE_GOLD, ColorRGB{12, 7, 6});
        sY += 26;

        char bufBel[64];
        std::snprintf(bufBel, sizeof(bufBel), "Fieles que transmiten el mito: %u", selMyth->believerCount);
        drawText(col3X + 10, sY, bufBel, Palette::RELIC_BONE, 1);
        sY += 24;
    } else {
        drawText(col3X + 10, sY, "NO HAY MITO ORAL ASOCIADO", Palette::DIVINE_GOLD, 1);
        sY += 18;
        drawText(col3X + 10, sY, "Cuando esta unidad sobreviva", ColorRGB{180, 160, 140}, 1);
        sY += 14;
        drawText(col3X + 10, sY, "a batallas o milagros, Sisifo", ColorRGB{180, 160, 140}, 1);
        sY += 14;
        drawText(col3X + 10, sY, "compondra un canto sagrado.", ColorRGB{180, 160, 140}, 1);
        sY += 30;
    }

    // Ethos Comunitario del Clan
    drawText(col3X + 10, sY, "ETHOS DE LA COMUNIDAD:", Palette::DIVINE_GOLD, 1);
    sY += 16;
    Sififus::EthosVector ethos = sififus ? sififus->getCollectiveEthos(target->getReligionId()) : Sififus::EthosVector{};

    auto drawEthosRow = [&](const char* label, float val, ColorRGB col, bool bipolar = false) {
        drawText(col3X + 10, sY, label, Palette::RELIC_BONE, 1);
        sY += 12;
        int eW = col3W - 24;
        int eX = col3X + 10;
        if (bipolar) {
            drawRect(eX, sY, eW, 8, ColorRGB{12, 7, 6});
            drawRectOutline(eX, sY, eW, 8, ColorRGB{70, 50, 40});
            int mX = eX + eW / 2;
            drawLine(mX, sY, mX, sY + 8, Palette::RELIC_BONE);
            if (val > 0.0f) {
                int fill = static_cast<int>((eW / 2) * std::clamp(val, 0.0f, 1.0f));
                drawRect(mX, sY + 1, fill, 6, col);
            } else if (val < 0.0f) {
                int fill = static_cast<int>((eW / 2) * std::clamp(-val, 0.0f, 1.0f));
                drawRect(mX - fill, sY + 1, fill, 6, ColorRGB{100, 180, 240});
            }
        } else {
            drawSims2Meter(eX, sY, eW, 8, std::clamp(val, 0.0f, 1.0f), col, ColorRGB{12, 7, 6});
        }
        sY += 16;
    };

    drawEthosRow("Agresion / Sed de Sangre:", ethos.aggressionBias, Palette::BLOOD_RUBY, true);
    drawEthosRow("Temor Sagrado al Castigo:", ethos.divineFearBias, ColorRGB{255, 200, 70}, true);
    drawEthosRow("Prioridad de Culto y Ofrenda:", ethos.devotionPriority, Palette::DIVINE_GOLD, false);
    drawEthosRow("Xenofobia hacia Otras Fes:", ethos.xenophobiaBias, ColorRGB{220, 120, 50}, false);
    drawEthosRow("Impulso de Inmolacion/Martirio:", ethos.martyrdomDrive, ColorRGB{200, 50, 50}, false);

    // =========================================================================
    // PIE DE VENTANA: BOTÓN CERRAR
    // =========================================================================
    const int cBtnW = 180;
    const int cBtnH = 26;
    const int cBtnX = modalX + (modalW - cBtnW) / 2;
    const int cBtnY = modalY + modalH - 36;
    bool cBtnHov = (mouseX >= cBtnX && mouseX <= cBtnX + cBtnW && mouseY >= cBtnY && mouseY <= cBtnY + cBtnH);
    drawAntiqueButton(cBtnX, cBtnY, cBtnW, cBtnH, "CERRAR VENTANA", false, cBtnHov, false, 1);
}

int Renderer::checkMemoryTreeModalClick(float mouseX, float mouseY) {
    const int modalW = 1140;
    const int modalH = 620;
    const int modalX = (m_width - modalW) / 2;
    const int modalY = (m_height - modalH) / 2;

    // 1. Botón Cerrar [X] en cabecera
    const int closeX = modalX + modalW - 44;
    const int closeY = modalY + 8;
    const int closeW = 32;
    const int closeH = 34;
    if (mouseX >= closeX && mouseX <= closeX + closeW && mouseY >= closeY && mouseY <= closeY + closeH) {
        return 1; // Cerrar modal
    }

    // 2. Botón inferior [ CERRAR VENTANA ]
    const int cBtnW = 180;
    const int cBtnH = 26;
    const int cBtnX = modalX + (modalW - cBtnW) / 2;
    const int cBtnY = modalY + modalH - 36;
    if (mouseX >= cBtnX && mouseX <= cBtnX + cBtnW && mouseY >= cBtnY && mouseY <= cBtnY + cBtnH) {
        return 1; // Cerrar modal
    }

    // 3. Clic fuera del modal -> cerrar
    if (mouseX < modalX || mouseX > modalX + modalW || mouseY < modalY || mouseY > modalY + modalH) {
        return 1;
    }

    // 4. Flechas de scroll en la columna de recuerdos
    const int col2X = modalX + 16 + 320 + 12;
    const int upX = col2X + 430 - 54;
    const int upY = modalY + 48 + 3;
    if (mouseX >= upX && mouseX <= upX + 22 && mouseY >= upY && mouseY <= upY + 18) {
        scrollMemoryTree(-1);
        return 2;
    }
    const int downX = upX + 26;
    if (mouseX >= downX && mouseX <= downX + 22 && mouseY >= upY && mouseY <= upY + 18) {
        scrollMemoryTree(1);
        return 2;
    }

    return 0; // Absorber clics dentro del modal
}

// =========================================================================
// VENTANA MODAL DEL SISTEMA DE MORAL RELIGIOSA Y REGLAS (MANDATOS & GRACIA)
// =========================================================================
void Renderer::renderMoralRulesModal(const ReligiousMoralitySystem& morality, int playerReligionId,
                                     const ReligionRegistry& registry, const GestorHumanos& humanos,
                                     const Timeline& timeline, float mouseX, float mouseY, bool isMouseDown) {
    (void)isMouseDown;
    // 1. Telón translúcido oscurecedor (Vignette litúrgica sobre la simulación activa)
    for (int y = 0; y < m_height; ++y) {
        int fy = m_height - 1 - y;
        for (int x = 0; x < m_width; ++x) {
            uint32_t px = m_framebuffer[fy * m_width + x];
            uint8_t r = static_cast<uint8_t>(((px & 0xFF) * 45) / 255);
            uint8_t g = static_cast<uint8_t>((((px >> 8) & 0xFF) * 45) / 255);
            uint8_t b = static_cast<uint8_t>((((px >> 16) & 0xFF) * 45) / 255);
            m_framebuffer[fy * m_width + x] = (0xFFu << 24) | (b << 16) | (g << 8) | r;
        }
    }

    const int modalW = 1140;
    const int modalH = 620;
    const int modalX = (m_width - modalW) / 2;
    const int modalY = (m_height - modalH) / 2;

    // 2. Marco exterior estilo Sims 2 con volumen de basalto y bordes dorados
    drawSims2Panel(modalX, modalY, modalW, modalH, ColorRGB{16, 11, 10}, false, Palette::DIVINE_GOLD);
    drawRectOutline(modalX + 2, modalY + 2, modalW - 4, modalH - 4, ColorRGB{75, 52, 38});
    drawRectOutline(modalX + 4, modalY + 4, modalW - 8, modalH - 8, ColorRGB{140, 110, 30});

    // Remates angulares dorados
    drawRect(modalX + 2, modalY + 2, 7, 7, Palette::DIVINE_GOLD);
    drawRect(modalX + modalW - 9, modalY + 2, 7, 7, Palette::DIVINE_GOLD);
    drawRect(modalX + 2, modalY + modalH - 9, 7, 7, Palette::DIVINE_GOLD);
    drawRect(modalX + modalW - 9, modalY + modalH - 9, 7, 7, Palette::DIVINE_GOLD);

    // 3. Placa de Cabecera
    const int headX = modalX + 16;
    const int headY = modalY + 8;
    const int headW = modalW - 68;
    const int headH = 34;
    drawSims2Panel(headX, headY, headW, headH, ColorRGB{34, 22, 16}, false, Palette::DIVINE_GOLD);

    const Religion* rel = registry.getReligion(playerReligionId);
    const ReligionMoralProfile* prof = morality.getProfile(playerReligionId);
    std::string relName = rel ? rel->name : "La Fe Sagrada";
    float grace = prof ? prof->communalGrace : 85.0f;
    MoralGraceStatus status = prof ? prof->getStatus() : MoralGraceStatus::Righteous;
    int theologyPts = rel ? rel->theologyPoints : 0;

    drawText(headX + 12, headY + 5, "TABLAS DE LA LEY Y MORALIDAD : MANDATOS, TABUES Y GRACIA COMUNITARIA", Palette::DIVINE_GOLD, 1);
    std::string subHeader = "RELIGION: " + relName + " | GRACIA: " + std::string(getMoralGraceStatusName(status)) +
                            " (" + std::to_string(static_cast<int>(grace)) + "%) | TEOLOGIA DISPONIBLE: " +
                            std::to_string(theologyPts) + " Fe | ANO: " +
                            std::to_string(std::abs(timeline.getCurrentYear())) + (timeline.getCurrentYear() < 0 ? " a.C." : " d.C.");
    drawText(headX + 12, headY + 18, subHeader.substr(0, 95), Palette::RELIC_BONE, 1);

    // Botón Cerrar [X]
    const int closeX = modalX + modalW - 44;
    const int closeY = modalY + 8;
    const int closeW = 32;
    const int closeH = 34;
    bool closeHover = (mouseX >= closeX && mouseX <= closeX + closeW && mouseY >= closeY && mouseY <= closeY + closeH);
    drawSims2Panel(closeX, closeY, closeW, closeH, ColorRGB{120, 20, 20}, closeHover, Palette::DIVINE_GOLD);
    drawText(closeX + 11, closeY + 11, "X", Palette::RELIC_BONE, 1);

    const int contentY = modalY + 48;
    const int contentH = modalH - 96;

    // =========================================================================
    // COLUMNA 1: CATÁLOGO DOCTRINAL DE MANDATOS Y TABÚES (Ancho 470px)
    // =========================================================================
    const int col1X = modalX + 16;
    const int col1W = 470;
    drawSims2Panel(col1X, contentY, col1W, contentH, ColorRGB{22, 15, 13}, false, ColorRGB{95, 68, 48});
    drawSims2Panel(col1X, contentY, col1W, 24, ColorRGB{38, 24, 18}, false, Palette::DIVINE_GOLD);
    drawText(col1X + 8, contentY + 6, "MANDATOS Y TABUES SAGRADOS (CANON DOCTRINAL)", Palette::DIVINE_GOLD, 1);

    const auto& catalog = morality.getCatalog();
    for (size_t i = 0; i < catalog.size() && i < 8; ++i) {
        const auto& rDef = catalog[i];
        bool isActive = prof && prof->hasRule(rDef.type);
        int itemY = contentY + 28 + static_cast<int>(i) * 60;
        int itemH = 57;

        drawSims2Panel(col1X + 6, itemY, col1W - 12, itemH,
                       isActive ? ColorRGB{32, 23, 16} : ColorRGB{18, 13, 11},
                       false,
                       isActive ? Palette::DIVINE_GOLD : ColorRGB{70, 50, 35});

        // Título del mandato y estado
        drawText(col1X + 12, itemY + 4, std::to_string(i + 1) + ". " + rDef.name,
                 isActive ? Palette::DIVINE_GOLD : Palette::RELIC_BONE, 1);
        if (isActive) {
            drawText(col1X + 270, itemY + 4, "[ACTIVO]", ColorRGB{80, 220, 100}, 1);
        } else {
            drawText(col1X + 260, itemY + 4, "[INACTIVO]", ColorRGB{160, 140, 130}, 1);
        }

        // Texto canónico
        std::string cmdTxt = "\"" + rDef.commandment;
        if (cmdTxt.size() > 48) cmdTxt = cmdTxt.substr(0, 48) + "...\"";
        else cmdTxt += "\"";
        drawText(col1X + 12, itemY + 16, cmdTxt, ColorRGB{195, 180, 160}, 1);

        // Virtud / Costo
        std::string effStr = isActive ? ("Virtud: " + rDef.virtueDescription)
                                      : ("Costo: " + std::to_string(rDef.theologyCost) + " Fe | Culpa: +" +
                                         std::to_string(static_cast<int>(rDef.guiltSeverity)));
        drawText(col1X + 12, itemY + 28, effStr.substr(0, 50),
                 isActive ? ColorRGB{130, 210, 240} : ColorRGB{200, 130, 90}, 1);

        // Pecado
        std::string sinStr = "Pecado: " + rDef.sinDescription;
        drawText(col1X + 12, itemY + 40, sinStr.substr(0, 50), ColorRGB{170, 110, 110}, 1);

        // Botón de Promulgación / Derogación
        const int btnW = 90;
        const int btnH = 22;
        const int btnX = col1X + col1W - 12 - btnW - 4;
        const int btnY = itemY + 18;
        bool btnHov = (mouseX >= btnX && mouseX <= btnX + btnW && mouseY >= btnY && mouseY <= btnY + btnH);

        if (isActive) {
            drawAntiqueButton(btnX, btnY, btnW, btnH, "DEROGAR", false, btnHov, false, 800 + static_cast<int>(i));
        } else {
            bool canAfford = rel && (rel->theologyPoints >= rDef.theologyCost);
            drawAntiqueButton(btnX, btnY, btnW, btnH, "PROMULGAR", false, btnHov && canAfford, false, 800 + static_cast<int>(i));
        }
    }

    // =========================================================================
    // COLUMNA 2: GRACIA COMUNITARIA & TEOLOGÍA (Ancho 330px)
    // =========================================================================
    const int col2X = col1X + col1W + 12;
    const int col2W = 330;
    drawSims2Panel(col2X, contentY, col2W, contentH, ColorRGB{20, 14, 12}, false, ColorRGB{95, 68, 48});
    drawSims2Panel(col2X, contentY, col2W, 24, ColorRGB{38, 24, 18}, false, Palette::DIVINE_GOLD);
    drawText(col2X + 8, contentY + 6, "ESTADO DE LA GRACIA COLECTIVA", Palette::DIVINE_GOLD, 1);

    int curY = contentY + 34;

    // Medidor de Gracia Colectiva
    drawText(col2X + 14, curY, "GRACIA SAGRADA DE LA COMUNIDAD:", Palette::DIVINE_GOLD, 1);
    curY += 16;
    ColorRGB graceCol = ColorRGB{46, 215, 80};
    if (status == MoralGraceStatus::DivineBlessing) graceCol = Palette::DIVINE_GOLD;
    else if (status == MoralGraceStatus::TenuousGrace) graceCol = ColorRGB{230, 170, 40};
    else if (status == MoralGraceStatus::MoralCrisis) graceCol = ColorRGB{220, 90, 40};
    else if (status == MoralGraceStatus::HereticalDecadence) graceCol = Palette::BLOOD_RUBY;

    drawSims2Meter(col2X + 14, curY, col2W - 28, 14, std::clamp(grace / 100.0f, 0.0f, 1.0f), graceCol, ColorRGB{12, 8, 6});
    curY += 20;

    std::string badgeStr = "[ " + std::string(getMoralGraceStatusName(status)) + " - " +
                           std::to_string(static_cast<int>(grace)) + "% ]";
    drawText(col2X + 14, curY, badgeStr, graceCol, 1);
    curY += 26;

    // Métricas de conducta del pueblo
    drawText(col2X + 14, curY, "METRICAS DE CONCIENCIA COLECTIVA:", Palette::RELIC_BONE, 1);
    curY += 16;
    drawText(col2X + 14, curY, "Mandatos Activos: " + std::to_string(prof ? prof->activeRules.size() : 0) + " / 4 max", ColorRGB{210, 195, 170}, 1);
    curY += 15;
    drawText(col2X + 14, curY, "Actos Virtuosos Registrados: " + std::to_string(prof ? prof->totalVirtuesCount : 0), ColorRGB{80, 220, 120}, 1);
    curY += 15;
    drawText(col2X + 14, curY, "Transgresiones Cometidas: " + std::to_string(prof ? prof->totalTransgressionsCount : 0), ColorRGB{230, 90, 90}, 1);
    curY += 15;
    int purityPct = static_cast<int>((prof ? prof->moralPurityRatio : 0.90f) * 100.0f);
    drawText(col2X + 14, curY, "Pureza de Fieles Virtuosos: " + std::to_string(purityPct) + "%", ColorRGB{130, 210, 240}, 1);
    curY += 15;
    drawText(col2X + 14, curY, "Apostasias y Cismas: " + std::to_string(prof ? prof->totalApostatesCount : 0),
             (prof && prof->totalApostatesCount > 0) ? ColorRGB{240, 60, 60} : ColorRGB{160, 140, 130}, 1);
    curY += 24;

    // Efectos doctrinales pasivos
    drawText(col2X + 14, curY, "EFECTOS LITURGICOS EN VIGOR:", Palette::DIVINE_GOLD, 1);
    curY += 16;
    if (status == MoralGraceStatus::DivineBlessing) {
        drawText(col2X + 14, curY, "* +25% Teologia por oraciones y ritos", ColorRGB{245, 220, 110}, 1); curY += 14;
        drawText(col2X + 14, curY, "* Cohesion tribal +0.02 por ano", ColorRGB{245, 220, 110}, 1); curY += 14;
        drawText(col2X + 14, curY, "* Inmunidad comunitaria a cismas", ColorRGB{245, 220, 110}, 1);
    } else if (status == MoralGraceStatus::Righteous) {
        drawText(col2X + 14, curY, "* Armonia y templanza ordinaria", ColorRGB{180, 220, 160}, 1); curY += 14;
        drawText(col2X + 14, curY, "* Generacion estandar de Teologia", ColorRGB{180, 220, 160}, 1); curY += 14;
        drawText(col2X + 14, curY, "* Los altares irradian serenidad", ColorRGB{180, 220, 160}, 1);
    } else if (status == MoralGraceStatus::TenuousGrace) {
        drawText(col2X + 14, curY, "* Friccion espiritual y dudas", ColorRGB{220, 180, 90}, 1); curY += 14;
        drawText(col2X + 14, curY, "* Mayor lentitud en conversiones", ColorRGB{220, 180, 90}, 1); curY += 14;
        drawText(col2X + 14, curY, "* Se exige penitencia en altares", ColorRGB{220, 180, 90}, 1);
    } else {
        drawText(col2X + 14, curY, "* Crisis moral y riesgo de cisma", ColorRGB{240, 70, 70}, 1); curY += 14;
        drawText(col2X + 14, curY, "* Caida de Cohesion (-0.05/ano)", ColorRGB{240, 70, 70}, 1); curY += 14;
        drawText(col2X + 14, curY, "* Herejes murmuran en el templo", ColorRGB{240, 70, 70}, 1);
    }

    // Botón de Expiación Colectiva
    const int absW = col2W - 28;
    const int absH = 26;
    const int absX = col2X + 14;
    const int absY = contentY + contentH - 36;
    bool absHov = (mouseX >= absX && mouseX <= absX + absW && mouseY >= absY && mouseY <= absY + absH);
    bool canAbsolve = rel && (rel->theologyPoints >= 60);
    drawAntiqueButton(absX, absY, absW, absH, "RITO DE EXPIACION (60 Fe)", false, absHov && canAbsolve, false, 850);

    // =========================================================================
    // COLUMNA 3: CONCIENCIA DEL FIEL (Ancho ~296px)
    // =========================================================================
    const int col3X = col2X + col2W + 12;
    const int col3W = modalX + modalW - 16 - col3X;
    drawSims2Panel(col3X, contentY, col3W, contentH, ColorRGB{24, 16, 14}, false, ColorRGB{95, 68, 48});
    drawSims2Panel(col3X, contentY, col3W, 24, ColorRGB{38, 24, 18}, false, Palette::DIVINE_GOLD);
    drawText(col3X + 8, contentY + 6, "CONCIENCIA DEL FIEL", Palette::DIVINE_GOLD, 1);

    const Humano* target = humanos.getHumanoSeleccionado();
    if (!target) {
        int lId = humanos.buscarLiderEspiritualDeReligion(playerReligionId);
        if (lId > 0) {
            for (const auto& h : humanos.getHumanos()) {
                if (h.getId() == lId) {
                    target = &h;
                    break;
                }
            }
        }
    }

    int unitY = contentY + 34;
    if (target) {
        const auto& mor = target->getMoralRecord();
        drawText(col3X + 12, unitY, ("FIEL: " + target->getNombre()).substr(0, 32), Palette::RELIC_BONE, 1);
        unitY += 16;
        std::string roleTag = "ALDEANO";
        switch (target->getHumanUnit().role) {
            case CommunityRole::FORAGER: roleTag = "RECOLECTOR"; break;
            case CommunityRole::ZEALOT:  roleTag = "ZELOTE FANATICO"; break;
            case CommunityRole::ELDER:   roleTag = "ANCIANO SABIO"; break;
            case CommunityRole::SHAMAN:  roleTag = "CHAMAN DE LA FE"; break;
            case CommunityRole::PROPHET: roleTag = "PROFETA SAGRADO"; break;
            default: break;
        }
        if (target->esChaman()) roleTag = "CHAMAN DE LA FE";
        if (target->esProfeta()) roleTag = "PROFETA SAGRADO";
        drawText(col3X + 12, unitY, "ROL: " + roleTag, Palette::DIVINE_GOLD, 1);
        unitY += 22;

        // Barra de Integridad Moral
        drawText(col3X + 12, unitY, "INTEGRIDAD MORAL:", Palette::RELIC_BONE, 1);
        unitY += 14;
        ColorRGB intCol = (mor.moralIntegrity >= 0.70f) ? ColorRGB{50, 210, 80} : ColorRGB{220, 160, 40};
        drawSims2Meter(col3X + 12, unitY, col3W - 24, 10, mor.moralIntegrity, intCol, ColorRGB{12, 8, 6});
        unitY += 16;
        drawText(col3X + 12, unitY, std::to_string(static_cast<int>(mor.moralIntegrity * 100.0f)) + "% Integridad espiritual", ColorRGB{200, 190, 175}, 1);
        unitY += 22;

        // Barra de Culpa Espiritual
        drawText(col3X + 12, unitY, "MANCHA DE CULPA:", Palette::RELIC_BONE, 1);
        unitY += 14;
        ColorRGB guiltCol = (mor.guiltLevel > 50.0f) ? Palette::BLOOD_RUBY : ColorRGB{230, 140, 40};
        drawSims2Meter(col3X + 12, unitY, col3W - 24, 10, mor.guiltLevel / 100.0f, guiltCol, ColorRGB{12, 8, 6});
        unitY += 16;
        drawText(col3X + 12, unitY, std::to_string(static_cast<int>(mor.guiltLevel)) + "% Culpa acumulada", ColorRGB{200, 190, 175}, 1);
        unitY += 22;

        // Estado espiritual del individuo
        drawText(col3X + 12, unitY, "ESTADO:", Palette::DIVINE_GOLD, 1);
        unitY += 14;
        if (mor.isApostateOrHeretic) {
            drawText(col3X + 12, unitY, "!APOSTATA / HEREJE!", ColorRGB{240, 50, 50}, 1);
        } else if (mor.hasGuilt()) {
            drawText(col3X + 12, unitY, "REQUIERE PENITENCIA", ColorRGB{235, 140, 40}, 1);
        } else {
            drawText(col3X + 12, unitY, "PURO Y EN GRACIA", ColorRGB{80, 220, 100}, 1);
        }
        unitY += 20;

        drawText(col3X + 12, unitY, "Pecados Cometidos: " + std::to_string(mor.sinsCommittedCount), ColorRGB{210, 130, 130}, 1);
        unitY += 14;
        drawText(col3X + 12, unitY, "Actos Virtuosos: " + std::to_string(mor.righteousActsCount), ColorRGB{130, 210, 130}, 1);
        unitY += 18;

        drawText(col3X + 12, unitY, "ULTIMA TRANSGRESION:", Palette::DIVINE_GOLD, 1);
        unitY += 14;
        std::string transTxt = mor.lastTransgression;
        if (mor.lastTransgressionYear != -4000) {
            transTxt += " (Ano " + std::to_string(std::abs(mor.lastTransgressionYear)) + ")";
        }
        drawText(col3X + 12, unitY, transTxt.substr(0, 34), ColorRGB{210, 180, 160}, 1);
        unitY += 26;

        drawText(col3X + 12, unitY, "EXPIACION Y PERDON:", Palette::RELIC_BONE, 1);
        unitY += 14;
        drawText(col3X + 12, unitY, "Orar junto al altar", ColorRGB{190, 180, 160}, 1); unitY += 13;
        drawText(col3X + 12, unitY, "en presencia de un", ColorRGB{190, 180, 160}, 1); unitY += 13;
        drawText(col3X + 12, unitY, "Chaman purifica la culpa", ColorRGB{190, 180, 160}, 1); unitY += 13;
        drawText(col3X + 12, unitY, "y sana el alma del fiel.", ColorRGB{190, 180, 160}, 1);
    } else {
        drawText(col3X + 12, unitY, "SIN SELECCION", Palette::DIVINE_GOLD, 1);
        unitY += 20;
        drawText(col3X + 12, unitY, "Selecciona a un", Palette::RELIC_BONE, 1); unitY += 14;
        drawText(col3X + 12, unitY, "aldeano en el mapa", Palette::RELIC_BONE, 1); unitY += 14;
        drawText(col3X + 12, unitY, "para examinar su", Palette::RELIC_BONE, 1); unitY += 14;
        drawText(col3X + 12, unitY, "conciencia individual.", Palette::RELIC_BONE, 1);
    }

    // Pie de ventana: Botón Cerrar
    const int cBtnW = 180;
    const int cBtnH = 26;
    const int cBtnX = modalX + (modalW - cBtnW) / 2;
    const int cBtnY = modalY + modalH - 36;
    bool cBtnHov = (mouseX >= cBtnX && mouseX <= cBtnX + cBtnW && mouseY >= cBtnY && mouseY <= cBtnY + cBtnH);
    drawAntiqueButton(cBtnX, cBtnY, cBtnW, cBtnH, "CERRAR VENTANA", false, cBtnHov, false, 1);
}

int Renderer::checkMoralRulesModalClick(float mouseX, float mouseY, ReligiousMoralitySystem& morality,
                                        int playerReligionId, ReligionRegistry& registry) {
    const int modalW = 1140;
    const int modalH = 620;
    const int modalX = (m_width - modalW) / 2;
    const int modalY = (m_height - modalH) / 2;

    // 1. Botón Cerrar [X] en cabecera
    const int closeX = modalX + modalW - 44;
    const int closeY = modalY + 8;
    const int closeW = 32;
    const int closeH = 34;
    if (mouseX >= closeX && mouseX <= closeX + closeW && mouseY >= closeY && mouseY <= closeY + closeH) {
        return 1;
    }

    // 2. Botón inferior [ CERRAR VENTANA ]
    const int cBtnW = 180;
    const int cBtnH = 26;
    const int cBtnX = modalX + (modalW - cBtnW) / 2;
    const int cBtnY = modalY + modalH - 36;
    if (mouseX >= cBtnX && mouseX <= cBtnX + cBtnW && mouseY >= cBtnY && mouseY <= cBtnY + cBtnH) {
        return 1;
    }

    // 3. Clic fuera del modal -> cerrar
    if (mouseX < modalX || mouseX > modalX + modalW || mouseY < modalY || mouseY > modalY + modalH) {
        return 1;
    }

    const int contentY = modalY + 48;
    const int col1X = modalX + 16;
    const int col1W = 470;

    // 4. Botones de Promulgar / Derogar de la Columna 1 (8 mandatos)
    const auto& catalog = morality.getCatalog();
    for (size_t i = 0; i < catalog.size() && i < 8; ++i) {
        const auto& rDef = catalog[i];
        int itemY = contentY + 28 + static_cast<int>(i) * 60;
        const int btnW = 90;
        const int btnH = 22;
        const int btnX = col1X + col1W - 12 - btnW - 4;
        const int btnY = itemY + 18;

        if (mouseX >= btnX && mouseX <= btnX + btnW && mouseY >= btnY && mouseY <= btnY + btnH) {
            if (morality.isRuleActive(playerReligionId, rDef.type)) {
                morality.repealRule(playerReligionId, rDef.type);
            } else {
                morality.enactRule(playerReligionId, rDef.type, registry);
            }
            return 2;
        }
    }

    // 5. Botón de Rito de Expiación Colectiva (Columna 2)
    const int contentH = modalH - 96;
    const int col2X = col1X + col1W + 12;
    const int col2W = 330;
    const int absW = col2W - 28;
    const int absH = 26;
    const int absX = col2X + 14;
    const int absY = contentY + contentH - 36;
    if (mouseX >= absX && mouseX <= absX + absW && mouseY >= absY && mouseY <= absY + absH) {
        Religion* rel = registry.getReligion(playerReligionId);
        if (rel && rel->theologyPoints >= 60) {
            rel->theologyPoints -= 60;
            auto& prof = morality.getOrCreateProfile(playerReligionId);
            prof.communalGrace = std::min(100.0f, prof.communalGrace + 15.0f);
            return 2;
        }
    }

    return 0; // Absorber clics dentro del modal
}

void Renderer::renderBottomBar(const WorldMap& map, const ReligionRegistry& registry,
                               const Timeline& timeline, const EventLine& eventLine,
                               const GestorHumanos& humanos, const IsometricCamera& camera,
                               RenderMapMode mode) {
    (void)eventLine;
    const int botH = 118;
    const int botY = m_height - botH;

    // 1. ZONA IZQUIERDA (18.5% ancho) - Panel de Opciones de Unidad / Información de Pueblo-Unidad
    const int verdeW = static_cast<int>(m_width * 0.185f);
    drawSims2Panel(0, botY, verdeW, botH, ColorRGB{18, 11, 10}, false, ColorRGB{75, 52, 38});
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

    // Las opciones divinas e intermediario del pueblo del jugador (Chamán o Profeta según la era)
    if (selHumano && selHumano->esLiderEspiritual() && selHumano->getReligionId() == playerRel) {
        const Religion* hRel = registry.getReligion(selHumano->getReligionId());
        HistoricalEra era = hRel ? hRel->currentEra : HistoricalEra::PrimordialAnimism;
        std::string leaderTitle = (era >= HistoricalEra::UniversalReligions) ? "PROFETA" : "CHAMAN";

        drawText(8, botY + 6, leaderTitle + ": " + selHumano->getNombre().substr(0, 14), Palette::DIVINE_GOLD, 1);

        std::string feName = hRel ? hRel->name : "Fe Ancestral";
        drawText(8, botY + 18, ("Fe: " + feName).substr(0, 22), hRel ? hRel->color : Palette::RELIC_BONE, 1);

        // Barra compacta de Vida (HP) y devoción del Líder (estilo medidor Sims 2)
        int halfW = (verdeW - 56) / 2;
        drawText(8, botY + 30, "HP:", Palette::RELIC_BONE, 1);
        ColorRGB hpCol = (selHumano->getSalud() > 50.0f) ? ColorRGB{46, 215, 80} : ColorRGB{220, 50, 50};
        drawSims2Meter(30, botY + 30, halfW, 7, selHumano->getSalud() / 100.0f, hpCol, ColorRGB{12, 7, 6});

        int devX = 30 + halfW + 10;
        drawText(devX - 22, botY + 30, "Fe:", Palette::RELIC_BONE, 1);
        drawSims2Meter(devX, botY + 30, halfW, 7, selHumano->getDevocion(), Palette::DIVINE_GOLD, ColorRGB{12, 7, 6});

        // Rasgos sagrados y estado del líder espiritual
        bool isChanneling = (selHumano->getTimerSenalDivina() > 0.0f);
        if (selHumano->esProfeta()) {
            drawText(8, botY + 44, "DON: Cruza Aguas y Peligros", Palette::DIVINE_GOLD, 1);
            drawText(8, botY + 56, "Unidad bendecida milagrosa", Palette::RELIC_BONE, 1);
        } else {
            drawText(8, botY + 44, "LIDER: Conduce Milagros", Palette::DIVINE_GOLD, 1);
            drawText(8, botY + 56, "Predica sermones sagrados", Palette::RELIC_BONE, 1);
        }

        const std::string leaderActionText = isChanneling ? "CANALIZANDO ACTO DIVINO!" : "Guiando a la comunidad";
        drawText(8, botY + 70, leaderActionText.substr(0, 23), isChanneling ? ColorRGB{255, 235, 120} : ColorRGB{200, 185, 165}, 1);

        // EN: Bottom action buttons: CENTER CAMERA (700) and MEMORY TREE (750)
        // ES: Botones inferiores de acción: CENTRAR CÁMARA (700) y ÁRBOL DE RECUERDOS (750)
        const int buttonHalfWidth = (verdeW - 20) / 2;
        const int buttonFirstStartX = 8;
        const int buttonStartY = botY + 86;
        const int buttonHeight = 24;
        const bool isButtonHovered1 = (m_mouseX >= buttonFirstStartX && m_mouseX <= buttonFirstStartX + buttonHalfWidth &&
                                      m_mouseY >= buttonStartY && m_mouseY <= buttonStartY + buttonHeight);
        const bool isButtonPressed1 = (m_pressedButtonId == 700) || (isButtonHovered1 && m_isMouseDown);
        drawAntiqueButton(buttonFirstStartX, buttonStartY, buttonHalfWidth, buttonHeight, "CENTRAR", false, isButtonHovered1, isButtonPressed1, 700);

        const int buttonSecondStartX = 8 + buttonHalfWidth + 4;
        const bool isButtonHovered2 = (m_mouseX >= buttonSecondStartX && m_mouseX <= buttonSecondStartX + buttonHalfWidth &&
                                      m_mouseY >= buttonStartY && m_mouseY <= buttonStartY + buttonHeight);
        const bool isButtonPressed2 = (m_pressedButtonId == 750) || (isButtonHovered2 && m_isMouseDown);
        drawAntiqueButton(buttonSecondStartX, buttonStartY, buttonHalfWidth, buttonHeight, "RECUERDOS", m_memoryTreeOpen, isButtonHovered2, isButtonPressed2, 750);
    } else if (selHumano) {
        if (selHumano->estaMuerto()) {
            // EN: Deceased fallen unit inspector view
            // ES: Vista de inspector para unidad caída y fallecida
            drawText(8, botY + 6, ("CAIDO: " + selHumano->getNombre()).substr(0, 23), ColorRGB{220, 50, 50}, 1);
            drawText(8, botY + 18, "Ha muerto en combate", Palette::BLOOD_RUBY, 1);
            drawText(8, botY + 32, "HP: 0/100 (Fallecido)", ColorRGB{200, 70, 70}, 1);
            drawText(8, botY + 46, "Retornando a la tierra...", Palette::RELIC_BONE, 1);

            const int buttonHalfWidth = (verdeW - 20) / 2;
            const int buttonFirstStartX = 8;
            const int buttonStartY = botY + 86;
            const int buttonHeight = 24;
            const bool isButtonHovered1 = (m_mouseX >= buttonFirstStartX && m_mouseX <= buttonFirstStartX + buttonHalfWidth &&
                                          m_mouseY >= buttonStartY && m_mouseY <= buttonStartY + buttonHeight);
            const bool isButtonPressed1 = (m_pressedButtonId == 700) || (isButtonHovered1 && m_isMouseDown);
            drawAntiqueButton(buttonFirstStartX, buttonStartY, buttonHalfWidth, buttonHeight, "IR A LIDER", false, isButtonHovered1, isButtonPressed1, 700);

            const int buttonSecondStartX = 8 + buttonHalfWidth + 4;
            const bool isButtonHovered2 = (m_mouseX >= buttonSecondStartX && m_mouseX <= buttonSecondStartX + buttonHalfWidth &&
                                          m_mouseY >= buttonStartY && m_mouseY <= buttonStartY + buttonHeight);
            const bool isButtonPressed2 = (m_pressedButtonId == 750) || (isButtonHovered2 && m_isMouseDown);
            drawAntiqueButton(buttonSecondStartX, buttonStartY, buttonHalfWidth, buttonHeight, "RECUERDOS", m_memoryTreeOpen, isButtonHovered2, isButtonPressed2, 750);
        } else {
            // EN: Active living villager inspector view (vitals, roles, circadian and spiritual states)
            // ES: Vista de inspector de aldeano vivo activo (vitales, roles, estados circadianos y espirituales)
            const HumanUnit& humanUnit = selHumano->getHumanUnit();
            std::string roleTag = "ALDEANO";
            if (selHumano->esNino()) {
                roleTag = selHumano->esMujer() ? "NINA" : "NINO";
            } else if (selHumano->esMujer()) {
                roleTag = "ALDEANA";
            }
            switch (humanUnit.role) {
                case CommunityRole::FORAGER: roleTag = selHumano->esMujer() ? "RECOLECTORA" : "RECOLECTOR"; break;
                case CommunityRole::SHAMAN:  roleTag = selHumano->esMujer() ? "SACERDOTISA" : "CHAMAN"; break;
                case CommunityRole::PROPHET: roleTag = "PROFETA"; break;
                case CommunityRole::ELDER:   roleTag = selHumano->esMujer() ? "ANCIANA" : "ANCIANO"; break;
                case CommunityRole::ZEALOT:  roleTag = selHumano->esMujer() ? "FANATICA" : "FANATICO"; break;
            }
            if (selHumano->esNino()) roleTag = selHumano->esMujer() ? "NINA" : "NINO";
            const bool isRivalReligion = (selHumano->getReligionId() != playerRel);
            if (isRivalReligion) roleTag = "AJENO " + roleTag;

            const std::string genderBadge = selHumano->esNino() ? " [INF]" : (selHumano->esMujer() ? " [M]" : " [H]");
            const std::string unitTitleString = roleTag + genderBadge + ": " + selHumano->getNombre();
            drawText(8, botY + 6, unitTitleString.substr(0, 26),
                     isRivalReligion ? ColorRGB{230, 90, 80} : Palette::DIVINE_GOLD, 1);

            const Religion* villagerReligion = registry.getReligion(selHumano->getReligionId());
            const std::string faithName = villagerReligion ? villagerReligion->name : "Fiel Animista";
            drawText(8, botY + 18, ("Fe: " + faithName).substr(0, 22), villagerReligion ? villagerReligion->color : Palette::RELIC_BONE, 1);

            const std::string vitalStatsString = "En:" + std::to_string(static_cast<int>(humanUnit.energy)) +
                                                 " Nut:" + std::to_string(static_cast<int>(humanUnit.nutritionalState)) +
                                                 " Sue:" + std::to_string(static_cast<int>(selHumano->getSueno()));
            drawText(8, botY + 30, vitalStatsString.substr(0, 24), Palette::RELIC_BONE, 1);

            // EN: HP health bar and faith devotion meter (Sims 2 style UI gauge)
            // ES: Barra de Vida (HP) y medidor de Devoción (estilo medidor Sims 2)
            const int barHalfWidth = (verdeW - 56) / 2;
            drawText(8, botY + 42, "HP:", Palette::RELIC_BONE, 1);
            const ColorRGB healthBarColor = (selHumano->getSalud() > 50.0f) ? ColorRGB{46, 215, 80} : ColorRGB{220, 50, 50};
            drawSims2Meter(30, botY + 42, barHalfWidth, 7, selHumano->getSalud() / 100.0f, healthBarColor, ColorRGB{12, 7, 6});

            const int devotionBarPosX = 30 + barHalfWidth + 10;
            drawText(devotionBarPosX - 22, botY + 42, "Fe:", Palette::RELIC_BONE, 1);
            drawSims2Meter(devotionBarPosX, botY + 42, barHalfWidth, 7, selHumano->getDevocion(), Palette::DIVINE_GOLD, ColorRGB{12, 7, 6});

            // EN: Systemic state of unit (FirstEraAIController / HumanAIController)
            // ES: Estado sistémico de la unidad (FirstEraAIController / HumanAIController)
            std::string systemicStateString = "IA: ";
            if (selHumano->tieneFuriaProtectora()) {
                systemicStateString = "!Furia Protectora!";
            } else if (selHumano->estaDurmiendo()) {
                systemicStateString = selHumano->estaDurmiendoBajoTecho() ? "Durmiendo Bajo Techo" : "Durmiendo (Fogata)";
            } else {
                switch (humanUnit.currentState) {
                    case HumanAIState::DEFENDING:        systemicStateString += "Defensa de la Fe"; break;
                    case HumanAIState::ATTACKING:        systemicStateString += "Ataque / Combate"; break;
                    case HumanAIState::SURVIVAL_WORKING: systemicStateString += "Superviv./Recolec."; break;
                    case HumanAIState::WORSHIPING:       systemicStateString += "Culto / Ofrenda"; break;
                    case HumanAIState::SOCIALIZING:      systemicStateString += "Cohesion / Tribu"; break;
                    case HumanAIState::CONVERTING:       systemicStateString += "Convirtiendo Fe"; break;
                    case HumanAIState::MIGRATING:        systemicStateString += "Migrando"; break;
                    default:                             systemicStateString += "En Reposo"; break;
                }
            }
            const ColorRGB systemicStateColor = selHumano->tieneFuriaProtectora() ? ColorRGB{255, 90, 80} :
                                                (selHumano->estaDurmiendo() ? ColorRGB{165, 215, 255} : Palette::RELIC_BONE);
            drawText(8, botY + 56, systemicStateString.substr(0, 23), systemicStateColor, 1);

            // EN: Physical animation action and carried resource/offering state
            // ES: Estado de animación física y carga de recursos/ofrendas
            std::string physicalActionString = "";
            if (selHumano->getCantidadRecursoCargado() > 0) {
                if (selHumano->isDeliveringOffering()) {
                    physicalActionString = "Ofrenda Sagrada Altar!";
                } else {
                    physicalActionString = "Carga: " + std::to_string(selHumano->getCantidadRecursoCargado()) + " " +
                                           std::string(getResourceTypeName(selHumano->getTipoRecursoRecolectando()));
                }
            } else {
                physicalActionString = "Accion: ";
                switch (selHumano->getAccion()) {
                    case AccionHumano::IDLE: physicalActionString += "Orando / Quieto"; break;
                    case AccionHumano::CAMINAR: physicalActionString += "Peregrinando"; break;
                    case AccionHumano::CORRER: physicalActionString += "En Carrera"; break;
                    case AccionHumano::RECOLECTAR:
                        physicalActionString += "Recol. " + std::string(getResourceTypeName(selHumano->getTipoRecursoRecolectando()));
                        break;
                    case AccionHumano::TALAR: physicalActionString += "Talando Madera"; break;
                    case AccionHumano::ATACAR: physicalActionString += "Combate Melee"; break;
                    case AccionHumano::REZAR: physicalActionString += "Orando en Altar"; break;
                    default: physicalActionString += "Carga / Mineria"; break;
                }
            }
            const auto& moralRecord = selHumano->getMoralRecord();
            if (selHumano->tieneFuriaProtectora()) {
                drawText(8, botY + 70, "!DEFENDIENDO FAMILIA!", ColorRGB{255, 90, 80}, 1);
            } else if (selHumano->estaDurmiendo()) {
                const std::string sleepDescription = selHumano->estaDurmiendoBajoTecho() ?
                    ("Bajo Techo (Choza #" + std::to_string(selHumano->getHomeId()) + ")") : "Descanso en Fogata";
                drawText(8, botY + 70, sleepDescription.substr(0, 23), ColorRGB{170, 220, 255}, 1);
            } else if (moralRecord.isApostateOrHeretic) {
                drawText(8, botY + 70, "[HEREJE / APOSTATA]", ColorRGB{240, 50, 50}, 1);
            } else if (moralRecord.hasGuilt()) {
                const std::string guiltString = "Culpa:" + std::to_string(static_cast<int>(moralRecord.guiltLevel)) + "% [Penitencia]";
                drawText(8, botY + 70, guiltString.substr(0, 23), ColorRGB{235, 140, 40}, 1);
            } else if (selHumano->estaConstruyendoHogar()) {
                drawText(8, botY + 70, "Construyendo Choza...", ColorRGB{240, 205, 90}, 1);
            } else if (selHumano->tieneHogar()) {
                const std::string homeHouseString = "Hogar: Choza #" + std::to_string(selHumano->getHomeId());
                drawText(8, botY + 70, homeHouseString.substr(0, 23), ColorRGB{160, 235, 160}, 1);
            } else if (selHumano->getNeedForShelter() >= 0.50f) {
                const std::string needShelterString = "Nec. Refugio: " + std::to_string(static_cast<int>(selHumano->getNeedForShelter() * 100.0f)) + "%";
                drawText(8, botY + 70, needShelterString.substr(0, 23), ColorRGB{235, 160, 70}, 1);
            } else if (selHumano->estaEmbarazada()) {
                const std::string gestationString = "Gestando (" + std::to_string(static_cast<int>(selHumano->getEmbarazoProgreso() * 100.0f)) + "%)";
                drawText(8, botY + 70, gestationString.substr(0, 23), ColorRGB{255, 180, 220}, 1);
            } else if (selHumano->tienePareja()) {
                const Humano* spouseVillager = nullptr;
                for (const auto& candidateSpouse : humanos.getHumanos()) {
                    if (candidateSpouse.getId() == selHumano->getParejaId()) { spouseVillager = &candidateSpouse; break; }
                }
                if (spouseVillager && !selHumano->esNino()) {
                    const float reproductionDesireProbability = selHumano->calcularDeseoReproduccion(*spouseVillager, map, m_buildingSystem);
                    const int reproductionDesirePercentage = static_cast<int>(std::round(reproductionDesireProbability * 100.0f));
                    const std::string spouseStatusString = "Deseo Repr:" + std::to_string(reproductionDesirePercentage) + "% | Hij:" + std::to_string(selHumano->getHijosCount());
                    drawText(8, botY + 70, spouseStatusString.substr(0, 23), ColorRGB{255, 215, 160}, 1);
                } else {
                    const std::string spouseStatusString = "Amor:" + std::to_string(static_cast<int>(selHumano->getAmorHaciaPareja() * 100.0f)) +
                                                          "% | Hij:" + std::to_string(selHumano->getHijosCount());
                    drawText(8, botY + 70, spouseStatusString.substr(0, 23), ColorRGB{255, 215, 160}, 1);
                }
            } else if (selHumano->esNino()) {
                const std::string childStatusString = "Infante (" + std::to_string(static_cast<int>(selHumano->getEdad())) + " a.)";
                drawText(8, botY + 70, childStatusString.substr(0, 23), ColorRGB{160, 225, 255}, 1);
            } else {
                drawText(8, botY + 70, physicalActionString.substr(0, 23),
                         selHumano->isDeliveringOffering() ? Palette::DIVINE_GOLD : ColorRGB{200, 185, 165}, 1);
            }

            // EN: Unit Action Buttons: GOTO LEADER (700) and MEMORIES (750)
            // ES: Botones en Opciones de Unidad: IR A LIDER (700) y RECUERDOS (750)
            const int buttonHalfWidth = (verdeW - 20) / 2;
            const int buttonFirstStartX = 8;
            const int buttonStartY = botY + 86;
            const int buttonHeight = 24;
            const bool isButtonHovered1 = (m_mouseX >= buttonFirstStartX && m_mouseX <= buttonFirstStartX + buttonHalfWidth &&
                                          m_mouseY >= buttonStartY && m_mouseY <= buttonStartY + buttonHeight);
            const bool isButtonPressed1 = (m_pressedButtonId == 700) || (isButtonHovered1 && m_isMouseDown);
            drawAntiqueButton(buttonFirstStartX, buttonStartY, buttonHalfWidth, buttonHeight, "IR A LIDER", false, isButtonHovered1, isButtonPressed1, 700);

            const int buttonSecondStartX = 8 + buttonHalfWidth + 4;
            const bool isButtonHovered2 = (m_mouseX >= buttonSecondStartX && m_mouseX <= buttonSecondStartX + buttonHalfWidth &&
                                          m_mouseY >= buttonStartY && m_mouseY <= buttonStartY + buttonHeight);
            const bool isButtonPressed2 = (m_pressedButtonId == 750) || (isButtonHovered2 && m_isMouseDown);
            drawAntiqueButton(buttonSecondStartX, buttonStartY, buttonHalfWidth, buttonHeight, "RECUERDOS", m_memoryTreeOpen, isButtonHovered2, isButtonPressed2, 750);
        }
    } else {
        drawText(8, botY + 8, "LIDER ESPIRITUAL", Palette::DIVINE_GOLD, 1);
        drawText(8, botY + 28, "Selecciona al Chaman", Palette::RELIC_BONE, 1);
        drawText(8, botY + 42, "o Profeta del pueblo", Palette::RELIC_BONE, 1);
        drawText(8, botY + 56, "para manifestar poder", Palette::RELIC_BONE, 1);

        int btnHalfW = (verdeW - 20) / 2;
        int bx1 = 8;
        int by = botY + 86;
        int bw1 = btnHalfW;
        int bh = 24;
        bool hovered1 = (m_mouseX >= bx1 && m_mouseX <= bx1 + bw1 &&
                         m_mouseY >= by && m_mouseY <= by + bh);
        bool pressed1 = (m_pressedButtonId == 700) || (hovered1 && m_isMouseDown);
        drawAntiqueButton(bx1, by, bw1, bh, "IR A LIDER", false, hovered1, pressed1, 700);

        int bx2 = 8 + btnHalfW + 4;
        int bw2 = btnHalfW;
        bool hovered2 = (m_mouseX >= bx2 && m_mouseX <= bx2 + bw2 &&
                         m_mouseY >= by && m_mouseY <= by + bh);
        bool pressed2 = (m_pressedButtonId == 750) || (hovered2 && m_isMouseDown);
        drawAntiqueButton(bx2, by, bw2, bh, "RECUERDOS", m_memoryTreeOpen, hovered2, pressed2, 750);
    }

    // 2. PANEL DE SIMULACION (Centro - 63% ancho)
    // Información de la simulación, crónica de eventos, línea de tiempo y controles (Fondo #120b0a con volumen satinado Sims 2)
    const int azulX = verdeW;
    const int azulW = static_cast<int>(m_width * 0.63f);
    drawSims2Panel(azulX, botY, azulW, botH, ColorRGB{18, 11, 10}, false, ColorRGB{75, 52, 38});
    drawLine(azulX, botY, azulX + azulW, botY, Palette::DIVINE_GOLD);

    // Ranuras decorativas entre paneles
    drawLine(azulX - 1, botY, azulX - 1, m_height, ColorRGB{10, 6, 5});
    drawLine(azulX, botY, azulX, m_height, ColorRGB{75, 52, 38});

    // Timeline Scrub Track (bandeja hundida con barra cilíndrica satinada Sims 2)
    int trackX = azulX + 16;
    int trackY = botY + 14;
    int trackW = azulW - 32;
    int trackH = 8;
    drawSims2Panel(trackX, trackY, trackW, trackH, ColorRGB{12, 7, 6}, true, ColorRGB{75, 52, 38});

    float prog = timeline.getTimelineProgress();
    int fillW = static_cast<int>((trackW - 2) * prog);
    if (fillW > 0) {
        drawSims2Bar(trackX + 1, trackY + 1, fillW, trackH - 2, Palette::DIVINE_GOLD);
    }
    int thumbX = trackX + fillW;
    drawSims2Bar(thumbX - 4, trackY - 4, 8, 16, Palette::BLOOD_RUBY);
    drawRectOutline(thumbX - 4, trackY - 4, 8, 16, Palette::DIVINE_GOLD);

    // Botones inferiores: controles de simulación (justo bajo la línea de tiempo),
    // poderes de Dios (justo debajo de los controles), intervenciones teológicas y vistas
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
            case 710: return m_activeDivinePower == DivinePowerType::Lightning;
            case 711: return m_activeDivinePower == DivinePowerType::Plague;
            case 712: return m_activeDivinePower == DivinePowerType::Miracle;
            case 713: return selHumano && (selHumano->getTimerSenalDivina() > 0.0f);
            case 720: return m_activeDivinePower == DivinePowerType::Tornado;
            case 721: return m_activeDivinePower == DivinePowerType::Earthquake;
            case 722: return m_activeDivinePower == DivinePowerType::Pandemic;
            case 723: return m_activeDivinePower == DivinePowerType::Meteor;
            case 724: return m_activeDivinePower == DivinePowerType::Volcano;
            case 725: return m_activeDivinePower == DivinePowerType::Wildfire;
            default:  return false;
        }
    };
    auto drawPanelButton = [&](const UIButton& btn) {
        bool active = isButtonActive(btn.id);
        bool hovered = (m_mouseX >= btn.x && m_mouseX <= btn.x + btn.width &&
                        m_mouseY >= btn.y && m_mouseY <= btn.y + btn.height);
        bool pressed = (m_pressedButtonId == btn.id) || (hovered && m_isMouseDown);
        if (btn.id >= 101 && btn.id <= 105) {
            drawViewModeButton(static_cast<int>(btn.x), static_cast<int>(btn.y),
                               static_cast<int>(btn.width), static_cast<int>(btn.height),
                               btn.id, btn.label, active, hovered, pressed);
        } else if ((btn.id >= 710 && btn.id <= 713) || (btn.id >= 720 && btn.id <= 725) ||
                   btn.id == 701 || btn.id == 702 || btn.id == 703) {
            drawPowerButton(static_cast<int>(btn.x), static_cast<int>(btn.y),
                            static_cast<int>(btn.width), static_cast<int>(btn.height),
                            btn.id, btn.label, active, hovered, pressed);
        } else {
            drawAntiqueButton(static_cast<int>(btn.x), static_cast<int>(btn.y),
                              static_cast<int>(btn.width), static_cast<int>(btn.height),
                              btn.label, active, hovered, pressed, btn.id);
        }
    };

    for (const auto& btn : m_buttons) {
        if (btn.y < botY || btn.x < azulX || btn.x >= azulX + azulW) continue; // zona central
        drawPanelButton(btn);
    }

    // Separador litúrgico vertical entre Poderes Divinos y Catástrofes Naturales
    {
        const float powSize = 25.0f;
        const float gap = 5.0f;
        const float sepGap = 12.0f;
        const float totalWidth = (4 * powSize + 3 * gap) + sepGap + (6 * powSize + 5 * gap);
        const float startX = static_cast<float>(azulX) + (static_cast<float>(azulW) - totalWidth) * 0.5f;
        int sepX = static_cast<int>(startX + 4 * powSize + 3 * gap + sepGap * 0.5f);
        int sepY = static_cast<int>(botY + 54.0f);
        drawLine(sepX, sepY + 3, sepX, sepY + 23, ColorRGB{140, 110, 30});
        drawLine(sepX - 1, sepY + 3, sepX - 1, sepY + 23, ColorRGB{20, 14, 10});
    }

    // Tooltips flotantes para los botones de poderes al pasar el cursor (hover)
    for (const auto& btn : m_buttons) {
        if (btn.y < botY || btn.x < azulX || btn.x >= azulX + azulW) continue;
        bool isPower = (btn.id >= 710 && btn.id <= 713) || (btn.id >= 720 && btn.id <= 725);
        if (!isPower) continue;
        bool hovered = (m_mouseX >= btn.x && m_mouseX <= btn.x + btn.width &&
                        m_mouseY >= btn.y && m_mouseY <= btn.y + btn.height);
        if (hovered && !m_isMouseDown) {
            std::string tip = "";
            switch (btn.id) {
                case 710: case 701: tip = "[Z] RAYO DIVINO (25 Fe)"; break;
                case 711:           tip = "[X] PLAGA SAGRADA (35 Fe)"; break;
                case 712: case 703: tip = "[V] MILAGRO CELESTIAL (30 Fe)"; break;
                case 713: case 702: tip = "[C] CONGREGAR FIELES (10 Fe)"; break;
                case 720:           tip = "[T] TORNADO DEVASTADOR (40 Fe)"; break;
                case 721:           tip = "[G] TERREMOTO SÍSMICO (50 Fe)"; break;
                case 722:           tip = "[B] PANDEMIA VIRAL (45 Fe)"; break;
                case 723:           tip = "[M] METEORITO CATACLÍSMICO (60 Fe)"; break;
                case 724:           tip = "[K] ERUPCIÓN VOLCÁNICA (70 Fe)"; break;
                case 725:           tip = "[F] INCENDIO FORESTAL (55 Fe)"; break;
            }
            if (!tip.empty()) {
                int tw = static_cast<int>(tip.size()) * 6 + 12;
                int tx = std::clamp(static_cast<int>(btn.x + btn.width * 0.5f) - tw / 2, azulX + 4, azulX + azulW - tw - 4);
                int ty = static_cast<int>(btn.y) - 18;
                drawSims2Panel(tx, ty, tw, 16, ColorRGB{24, 16, 12}, false, Palette::DIVINE_GOLD);
                drawText(tx + 6, ty + 4, tip, Palette::DIVINE_GOLD, 1);
            }
        }
    }

    // Franja inferior central: estado de Intervención Divina y Guía Teológica
    int bannerY = botY + 88;
    int bannerH = 24;
    int bannerX = azulX + 16;
    int bannerW = azulW - 32;

    if (m_activeDivinePower != DivinePowerType::Inactive) {
        // Modo de apuntar activo
        std::string pName = "PODER DIVINO";
        ColorRGB pCol = Palette::DIVINE_GOLD;
        if (m_activeDivinePower == DivinePowerType::Lightning) {
            pName = "RAYO CELESTIAL";
            pCol = ColorRGB{255, 235, 120};
        } else if (m_activeDivinePower == DivinePowerType::Plague) {
            pName = "PLAGA PURIFICADORA";
            pCol = ColorRGB{210, 110, 240};
        } else if (m_activeDivinePower == DivinePowerType::Miracle) {
            pName = "MILAGRO DE FERTILIDAD";
            pCol = ColorRGB{90, 220, 255};
        } else if (m_activeDivinePower == DivinePowerType::Tornado) {
            pName = "TORNADO DEVASTADOR";
            pCol = ColorRGB{120, 220, 255};
        } else if (m_activeDivinePower == DivinePowerType::Earthquake) {
            pName = "TERREMOTO SÍSMICO";
            pCol = ColorRGB{255, 140, 50};
        } else if (m_activeDivinePower == DivinePowerType::Pandemic) {
            pName = "PANDEMIA VIRAL";
            pCol = ColorRGB{140, 240, 60};
        } else if (m_activeDivinePower == DivinePowerType::Meteor) {
            pName = "METEORITO CATACLÍSMICO";
            pCol = ColorRGB{255, 95, 25};
        } else if (m_activeDivinePower == DivinePowerType::Volcano) {
            pName = "ERUPCIÓN VOLCÁNICA";
            pCol = ColorRGB{255, 60, 20};
        } else if (m_activeDivinePower == DivinePowerType::Wildfire) {
            pName = "INCENDIO FORESTAL";
            pCol = ColorRGB{255, 110, 20};
        }
        drawSims2Panel(bannerX, bannerY, bannerW, bannerH, ColorRGB{35, 18, 14}, true, pCol);
        std::string bannerTxt = "OBJETIVO: " + pName + " | Haz clic en el mapa para desatar el poder | [ESC] Cancelar";
        drawText(bannerX + 10, bannerY + 6, bannerTxt.substr(0, 75), pCol, 1);
    } else {
        // Información del líder espiritual intermediario y economía de Fe
        int pRel = 1;
        if (selHumano) pRel = selHumano->getReligionId();
        int leadId = humanos.buscarLiderEspiritualDeReligion(pRel);
        std::string leadTxt = "Sin guia consagrado";
        for (const auto& h : humanos.getHumanos()) {
            if (h.getId() == leadId && h.estaVivo()) {
                std::string t = h.esProfeta() ? "Profeta " : "Chaman ";
                std::string propTrait = h.esProfeta() ? " (Cruza aguas/peligros)" : " (Guia)";
                leadTxt = t + h.getNombre() + propTrait;
                break;
            }
        }
        drawSims2Panel(bannerX, bannerY, bannerW, bannerH, ColorRGB{14, 9, 8}, true, ColorRGB{70, 50, 36});
        std::string infoTxt = "INTERMEDIARIO: " + leadTxt + "  |  PODERES: [Z] [X] [V] [C]  CATÁSTROFES: [T] [G] [B] [M] [K] [F]";
        drawText(bannerX + 10, bannerY + 6, infoTxt.substr(0, 78), Palette::RELIC_BONE, 1);
    }

    // 3. ZONA AMARILLA (Derecha - 18.5% ancho)
    // Minimapa interactivo que mueve la cámara al hacer clic, con opciones de vista (Ámbar satinado Sims 2)
    const int amarilloX = azulX + azulW;
    const int amarilloW = m_width - amarilloX;
    (void)amarilloW;
    drawSims2Panel(amarilloX, botY, amarilloW, botH, ColorRGB{60, 48, 12}, false, ColorRGB{140, 110, 30});
    drawLine(amarilloX, botY, m_width, botY, Palette::DIVINE_GOLD);

    // Ranuras decorativas entre paneles
    drawLine(amarilloX - 1, botY, amarilloX - 1, m_height, ColorRGB{10, 6, 5});
    drawLine(amarilloX, botY, amarilloX, m_height, ColorRGB{140, 110, 30});

    // Marco del Minimapa (expandido para aprovechar el panel completo)
    int mmX = 0, mmY = 0, mmW = 0, mmH = 0;
    getMinimapRect(mmX, mmY, mmW, mmH);

    renderMinimap(mmX, mmY, mmW, mmH, map, registry, humanos, camera, mode);

    // Botones de vista del mapa en miniatura (sprites 22x22 sin letras)
    for (const auto& btn : m_buttons) {
        if (btn.y < botY || btn.x < amarilloX) continue;
        drawPanelButton(btn);

        // Tooltip flotante informativo en hover
        bool hovered = (m_mouseX >= btn.x && m_mouseX <= btn.x + btn.width &&
                        m_mouseY >= btn.y && m_mouseY <= btn.y + btn.height);
        if (btn.id >= 101 && btn.id <= 105 && hovered && !m_isMouseDown) {
            std::string tip = "";
            switch (btn.id) {
                case 101: tip = "VISTA: FE"; break;
                case 102: tip = "VISTA: BIOMAS"; break;
                case 105: tip = "VISTA: UNIDADES"; break;
                case 103: tip = "VISTA: SINCRETISMO"; break;
                case 104: tip = "VISTA: ERAS"; break;
            }
            int tw = static_cast<int>(tip.size()) * 6 + 10;
            int tx = static_cast<int>(btn.x) - tw - 6;
            int ty = static_cast<int>(btn.y) + 3;
            drawSims2Panel(tx, ty, tw, 16, ColorRGB{24, 16, 12}, false, Palette::DIVINE_GOLD);
            drawText(tx + 5, ty + 4, tip, Palette::DIVINE_GOLD, 1);
        }
    }
}

void Renderer::renderMinimap(int mmX, int mmY, int mmW, int mmH,
                             const WorldMap& map, const ReligionRegistry& registry,
                             const GestorHumanos& humanos,
                             const IsometricCamera& camera, RenderMapMode mode) {
    // Marco exterior con bandeja hundida estilo Sims 2
    drawSims2Panel(mmX - 2, mmY - 2, mmW + 4, mmH + 4, ColorRGB{14, 14, 18}, true, Palette::DIVINE_GOLD);
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
                if (c.isContestedBorder) {
                    col = ColorRGB::blend(col, ColorRGB{235, 40, 30}, 0.50f);
                } else if (c.isSacredTruceActive) {
                    col = ColorRGB::blend(col, ColorRGB{250, 220, 80}, 0.50f);
                }
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
            if (!hObj.estaVivo()) continue;
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
    int boxRw = std::clamp(static_cast<int>((14.0f / camera.getZoom()) * (static_cast<float>(mmW) / std::max(1, mmH))), 4, 30);
    int boxRh = std::clamp(static_cast<int>(14.0f / camera.getZoom()), 4, 20);

    drawRectOutline(camMmX - boxRw, camMmY - boxRh, boxRw * 2, boxRh * 2, Palette::DIVINE_GOLD);
    drawRectOutline(camMmX - boxRw + 1, camMmY - boxRh + 1, (boxRw - 1) * 2, (boxRh - 1) * 2, ColorRGB{255, 255, 255});
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

    std::string resStr = "Resource: " + std::string(getResourceTypeName(c.naturalResource));
    if (c.naturalResource != ResourceType::None) {
        resStr += " (" + std::to_string(c.resourceAmount) + " / " + std::to_string(c.maxResourceAmount) + ")";
    }
    drawText(panelX + 15, panelY + 84, resStr, ColorRGB(212, 175, 55), 1);

    // Vivienda tribal o Sitio Sagrado (Resaltado con Oro Divino o Lino)
    const House* house = m_buildingSystem ? m_buildingSystem->getHouseAt(selectedCell.x, selectedCell.y) : nullptr;
    if (house) {
        std::string hTitle = "[HOGAR] " + std::string(house->getTypeName());
        drawText(panelX + 15, panelY + 102, hTitle.substr(0, 24), ColorRGB(230, 161, 59), 1);
        std::string hState = "Cap: " + std::to_string(house->getOccupantCount()) + " / " + std::to_string(house->capacity);
        if (house->sleepingOccupants > 0) {
            hState += " (" + std::to_string(house->sleepingOccupants) + " durmiendo)";
        } else if (house->isCompleted()) {
            hState += " (Habitada)";
        } else {
            hState += " (" + std::to_string(static_cast<int>(house->buildProgress)) + "%)";
        }
        drawText(panelX + 15, panelY + 118, hState.substr(0, 26), ColorRGB(212, 175, 55), 1);
    } else if (c.hasHolySite) {
        drawText(panelX + 15, panelY + 102, "[!] " + c.holySiteName, ColorRGB(230, 161, 59), 1); // Ocre Dorado #E6A13B
        std::string lvlStr = "Sanctuary Tier: " + std::to_string(c.shrineLevel);
        drawText(panelX + 15, panelY + 118, lvlStr, ColorRGB(212, 175, 55), 1);
    } else {
        drawText(panelX + 15, panelY + 102, "Holy Site: None", ColorRGB(120, 115, 110), 1);
    }

    // Faiths breakdown title
    drawText(panelX + 15, panelY + 140, "RELIGIOUS BREAKDOWN:", ColorRGB(212, 175, 55), 1);

    int curY = panelY + 158;
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
    const std::string subtitle = "Pre-Alpha v0.4.0 - Edicion Liturgica y Espiritual";

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
    const std::string footer = "C++20 ISOMETRIC ENGINE | PRE-ALPHA v0.4.0 | TECLAS: ESC PARA SALIR";
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

    // Modal / Altar central de Creación (Ancho 920, Alto 584)
    const float modalW = 920.0f;
    const float modalH = 584.0f;
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
    drawText(titleX, my + 16, title, Palette::DIVINE_GOLD, 2);

    // Línea divisoria ceremonial
    drawLine(mx + 30, my + 36, mx + static_cast<int>(modalW) - 30, my + 36, Palette::DIVINE_GOLD);
    drawRect(mx + static_cast<int>(modalW) / 2 - 4, my + 33, 8, 8, Palette::BLOOD_RUBY);

    // Subtítulos de columnas con subrayado litúrgico (distanciados de los campos para evitar superposición)
    const int col1X = mx + 45;
    const int col2X = mx + static_cast<int>(modalW) / 2 + 15;
    drawText(col1X, my + 44, "I. IDENTIDAD DIVINA Y TIERRA", Palette::DIVINE_GOLD, 1);
    drawLine(col1X, my + 54, col1X + 390, my + 54, ColorRGB{110, 95, 60});
    drawText(col2X, my + 44, "II. DOGMA Y CONDICIONES DEL MUNDO", Palette::DIVINE_GOLD, 1);
    drawLine(col2X, my + 54, col2X + 390, my + 54, ColorRGB{110, 95, 60});

    // Paleta de colores de influencia
    static const ColorRGB INFLUENCE_PALETTE[5] = {
        ColorRGB{230, 161, 59},  // Oro Divino
        ColorRGB{128, 0, 32},    // Rubí Sangre
        ColorRGB{52, 152, 219},  // Mónada Azul
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
        "Nivel 1: Pasiva (Tribal Ingenua)",
        "Nivel 2: Equilibrada (Tradicional)",
        "Nivel 3: Estrategica (Tactico-Teocratica)",
        "Nivel 4: Implacable (Gran Teocracia)"
    };

    // Nuevas Opciones: Clima, Pacing, Beligerancia, Abundancia
    static const char* CLIMATE_NAMES[4] = {
        "Templado (Llanuras & Valles)",
        "Glaciacion (Era de Hielo, Tundra)",
        "Desertico (Tierras Aridas & Oasis)",
        "Tropical (Selvas Humedas & Pantanos)"
    };

    static const char* PACING_NAMES[3] = {
        "Epico (1 ano/s - Contemplativo)",
        "Clasico (4 anos/s - Estandar)",
        "Dinamico (12 anos/s - Acelerado)"
    };

    static const char* BELLIGERENCE_NAMES[3] = {
        "Pacifica (Pactos & Coexistencia)",
        "Equilibrada (Friccion Historica)",
        "Hostil (Pugnas & Celo Liminal)"
    };

    static const char* ABUNDANCE_NAMES[3] = {
        "Escaso (Supervivencia austera)",
        "Equilibrado (Balance ecologico)",
        "Abundante (Tierras ricas y fertiles)"
    };

    // Tipo de Victoria
    static const char* VICTORY_NAMES[5] = {
        "Hegemonia (75% Poblacion)", "Teocratica (5 Maravillas)",
        "Trascendencia (Era 8)", "Tiempo (Mayor Fe al 2200)", "Sin Limite (Eterno)"
    };

    // Tamaños de Mapa y Cataclismos
    static const char* MAP_SIZES[3] = { "Pequeno (32x32)", "Mediano (48x48)", "Grande (64x64)" };
    static const char* DISASTER_LEVELS[3] = { "Bajas", "Normales", "Frecuentes / Apocalipticas" };

    // Etiquetas fijas de cada campo con espaciado no superpuesto (startFieldY = my + 60, stepFieldY = 38)
    auto drawFieldLabel = [&](int x, int y, const std::string& label) {
        drawText(x, y, label, Palette::RELIC_BONE, 1);
    };

    const int startFieldY = my + 60;
    const int stepFieldY = 38;

    // Columna 1 Labels
    drawFieldLabel(col1X, startFieldY + 0 * stepFieldY, "Nombre del Dios que seras:");
    drawFieldLabel(col1X, startFieldY + 1 * stepFieldY, "Nombre de la Religion:");
    drawFieldLabel(col1X, startFieldY + 2 * stepFieldY, "Semilla del Mundo (Seed):");
    drawFieldLabel(col1X, startFieldY + 3 * stepFieldY, "Poblacion Inicial del Mundo:");
    drawFieldLabel(col1X, startFieldY + 4 * stepFieldY, "Cantidad Inicial de Recursos (Comida/Bienes):");
    drawFieldLabel(col1X, startFieldY + 5 * stepFieldY, "Epoca Historica de Inicio:");
    drawFieldLabel(col1X, startFieldY + 6 * stepFieldY, "Tamano del Mundo Isometrico:");
    drawFieldLabel(col1X, startFieldY + 7 * stepFieldY, "Clima Primordial del Mundo:");
    drawFieldLabel(col1X, startFieldY + 8 * stepFieldY, "Abundancia de Recursos Naturales:");

    // Columna 2 Labels
    drawFieldLabel(col2X, startFieldY + 0 * stepFieldY, "Tipo de Partida:");
    drawFieldLabel(col2X, startFieldY + 1 * stepFieldY, "Dificultad de la Inteligencia Artificial:");
    drawFieldLabel(col2X, startFieldY + 2 * stepFieldY, "Ritmo Historico / Velocidad de Eras:");
    drawFieldLabel(col2X, startFieldY + 3 * stepFieldY, "Beligerancia Tribal / Friccion Liminal:");
    drawFieldLabel(col2X, startFieldY + 4 * stepFieldY, "Frecuencia de Cataclismos y Plagas:");
    drawFieldLabel(col2X, startFieldY + 5 * stepFieldY, "Color Sagrado de Influencia / Creyentes:");
    drawFieldLabel(col2X, startFieldY + 6 * stepFieldY, "Condicion de Victoria:");
    drawFieldLabel(col2X, startFieldY + 7 * stepFieldY, "Cofre Sagrado de Inicio:");
    drawFieldLabel(col2X, startFieldY + 8 * stepFieldY, "Evolucion Doctrinal (Herejias / Comunion):");

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
            displayText = std::to_string(config.mapSeed) + (isActive ? "_" : "");
            textColor = ColorRGB{255, 235, 170};
        } else if (btn.id == 504) {
            displayText = "[RAND]";
            textColor = Palette::DIVINE_GOLD;
        } else if (btn.id == 505) {
            displayText = std::to_string(config.initialPopulation) + " aldeanos reales";
        } else if (btn.id == 516) {
            displayText = std::to_string(config.initialResources) + (isActive ? "_" : "") + " unidades";
            textColor = ColorRGB{255, 235, 170};
        } else if (btn.id == 506) {
            int idx = static_cast<int>(config.startingEra);
            displayText = (idx >= 0 && idx < 8) ? ERA_NAMES[idx] : "Primordial Animism";
        } else if (btn.id == 507) {
            displayText = (config.mapSizeIndex >= 0 && config.mapSizeIndex < 3) ? MAP_SIZES[config.mapSizeIndex] : "Mediano";
        } else if (btn.id == 520) {
            int idx = static_cast<int>(config.worldClimate);
            displayText = (idx >= 0 && idx < 4) ? CLIMATE_NAMES[idx] : "Templado";
            textColor = (config.worldClimate == WorldClimate::Glaciacion) ? ColorRGB{170, 220, 255} :
                        ((config.worldClimate == WorldClimate::Desertico) ? ColorRGB{255, 210, 140} :
                        ((config.worldClimate == WorldClimate::Tropical) ? ColorRGB{140, 240, 150} : ColorRGB{255, 240, 200}));
        } else if (btn.id == 517) {
            int idx = static_cast<int>(config.resourceAbundance);
            displayText = (idx >= 0 && idx < 3) ? ABUNDANCE_NAMES[idx] : "Equilibrado";
            textColor = (config.resourceAbundance == ResourceAbundance::Abundante) ? ColorRGB{120, 240, 140} :
                        ((config.resourceAbundance == ResourceAbundance::Escaso) ? ColorRGB{240, 170, 140} : ColorRGB{240, 230, 190});
        } else if (btn.id == 509) {
            int idx = static_cast<int>(config.gameMode);
            displayText = (idx >= 0 && idx < 4) ? MODE_NAMES[idx] : "Estandar";
        } else if (btn.id == 511) {
            int idx = static_cast<int>(config.aiDifficulty);
            displayText = (idx >= 0 && idx < 4) ? DIFF_NAMES[idx] : "Normal";
            textColor = (config.aiDifficulty == AIDifficulty::Implacable) ? ColorRGB{255, 110, 110} :
                        ((config.aiDifficulty == AIDifficulty::Estrategica) ? ColorRGB{255, 190, 90} :
                        ((config.aiDifficulty == AIDifficulty::Equilibrada) ? ColorRGB{255, 240, 170} : ColorRGB{160, 220, 255}));
        } else if (btn.id == 521) {
            int idx = static_cast<int>(config.timePacing);
            displayText = (idx >= 0 && idx < 3) ? PACING_NAMES[idx] : "Clasico";
        } else if (btn.id == 522) {
            int idx = static_cast<int>(config.tribalBelligerence);
            displayText = (idx >= 0 && idx < 3) ? BELLIGERENCE_NAMES[idx] : "Equilibrada";
            textColor = (config.tribalBelligerence == TribalBelligerence::Hostil) ? ColorRGB{255, 130, 130} :
                        ((config.tribalBelligerence == TribalBelligerence::Pacifica) ? ColorRGB{140, 230, 180} : ColorRGB{240, 230, 190});
        } else if (btn.id == 508) {
            displayText = (config.disasterFrequency >= 0 && config.disasterFrequency < 3) ? DISASTER_LEVELS[config.disasterFrequency] : "Normales";
        } else if (btn.id == 512) {
            int idx = config.influenceColorIndex % 5;
            displayText = std::string(INFLUENCE_NAMES[idx]);
            textColor = INFLUENCE_PALETTE[idx];
        } else if (btn.id == 513) {
            int idx = static_cast<int>(config.victoryCondition);
            displayText = (idx >= 0 && idx < 5) ? VICTORY_NAMES[idx] : "Hegemonia";
        } else if (btn.id == 510) {
            displayText = config.starterChest ? "[x] ACTIVADO (+1000 Fe & Reliquia)" : "[ ] DESACTIVADO";
            textColor = config.starterChest ? ColorRGB{120, 230, 120} : ColorRGB{180, 180, 180};
        } else if (btn.id == 514) {
            displayText = config.allowSchisms ? "[x] Cismas Doctrinales" : "[ ] Sin Cismas";
            textColor = config.allowSchisms ? Palette::BLOOD_RUBY : ColorRGB{180, 180, 180};
        } else if (btn.id == 515) {
            displayText = config.allowSyncretism ? "[x] Sincretismo Ecumenico" : "[ ] Aislamiento";
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
                drawRect(bx + bw - 24, by + 3, 18, bh - 6, INFLUENCE_PALETTE[colIdx]);
                drawRectOutline(bx + bw - 24, by + 3, 18, bh - 6, Palette::DIVINE_GOLD);
            }

            int tx = bx + 8;
            int ty = by + (bh - 7) / 2;
            drawText(tx, ty, displayText, textColor, 1);
        }
    }

    // III. Relicario Táctico de Inteligencia Artificial (Dossier sin trampas estadísticas)
    const int cardX = mx + 45;
    const int cardY = my + 406;
    const int cardW = static_cast<int>(modalW) - 90;
    const int cardH = 92;

    drawRect(cardX, cardY, cardW, cardH, ColorRGB{26, 26, 32});
    drawRectOutline(cardX, cardY, cardW, cardH, ColorRGB{70, 70, 75});
    drawRectOutline(cardX + 1, cardY + 1, cardW - 2, cardH - 2, ColorRGB{50, 48, 55});

    drawText(cardX + 12, cardY + 7, "III. PERFIL TACTICO DE INTELIGENCIA ARTIFICIAL (IA 100% CONDUCTUAL)", Palette::DIVINE_GOLD, 1);
    drawText(cardX + cardW - 270, cardY + 7, "[SIN TRAMPAS NI STAT-BOOSTS]", ColorRGB{120, 230, 120}, 1);
    drawLine(cardX + 12, cardY + 20, cardX + cardW - 12, cardY + 20, ColorRGB{80, 75, 65});

    if (config.aiDifficulty == AIDifficulty::Pasiva) {
        drawText(cardX + 14, cardY + 26, "NIVEL 1: IA TRIBAL INGENUA - Subsistencia domestica y ritos locales.", ColorRGB{255, 230, 140}, 1);
        drawText(cardX + 14, cardY + 40, "* Comportamiento: Los aldeanos rivales recolectan y oran unicamente en su aldea natal.", ColorRGB{200, 195, 185}, 1);
        drawText(cardX + 14, cardY + 54, "* Doctrina: No envian caravanas misioneras ofensivas ni disputan celdas fronterizas.", ColorRGB{200, 195, 185}, 1);
        drawText(cardX + 14, cardY + 68, "* Ideal para partidas de construccion pacifica o aprendizaje de las mecanicas de fe.", ColorRGB{140, 200, 255}, 1);
    } else if (config.aiDifficulty == AIDifficulty::Equilibrada) {
        drawText(cardX + 14, cardY + 26, "NIVEL 2: IA TRADICIONAL HISTORICA - Difusion organica y devocion balanceada.", ColorRGB{255, 230, 140}, 1);
        drawText(cardX + 14, cardY + 40, "* Comportamiento: Construyen y expanden santuarios, defienden aldeas y realizan peregrinaciones.", ColorRGB{200, 195, 185}, 1);
        drawText(cardX + 14, cardY + 54, "* Doctrina: Caravanas misioneras cuando la devocion decae; expansion natural por el mapa.", ColorRGB{200, 195, 185}, 1);
        drawText(cardX + 14, cardY + 68, "* Ritmo historico clasico con pugnas doctrinales y conversiones naturales.", ColorRGB{140, 200, 255}, 1);
    } else if (config.aiDifficulty == AIDifficulty::Estrategica) {
        drawText(cardX + 14, cardY + 26, "NIVEL 3: IA TACTICA TEOCRATICA - Inteligencia de expansion y proteccion sacerdotal.", ColorRGB{255, 200, 100}, 1);
        drawText(cardX + 14, cardY + 40, "* Comportamiento: Escolta armada a lideres y chamanes; retirada tactica a templos si son heridos.", ColorRGB{200, 195, 185}, 1);
        drawText(cardX + 14, cardY + 54, "* Doctrina: Escaneo selectivo de celdas debiles o no consagradas para priorizar conversion rapida.", ColorRGB{200, 195, 185}, 1);
        drawText(cardX + 14, cardY + 68, "* Desafio tactico avanzado sin trampas numericas: las IAs razonan sus movimientos.", ColorRGB{255, 160, 120}, 1);
    } else { // Implacable
        drawText(cardX + 14, cardY + 26, "NIVEL 4: GRAN TEOCRACIA IMPLACABLE - Geopolitica religiosa, contrasermones y resiliencia.", ColorRGB{255, 120, 120}, 1);
        drawText(cardX + 14, cardY + 40, "* Comportamiento: Evacuacion ante incendios/cataclismos y reconstruccion prioritaria de refugios.", ColorRGB{200, 195, 185}, 1);
        drawText(cardX + 14, cardY + 54, "* Doctrina: Predicacion pastoral anti-cisma en fronteras y caravanas estrategicas a urbes rivales.", ColorRGB{200, 195, 185}, 1);
        drawText(cardX + 14, cardY + 68, "* Maxima eficiencia cognitiva: pugna religiosa despiadada basada puramente en decisiones.", ColorRGB{255, 100, 100}, 1);
    }

    // Pie de la ventana modal
    const std::string hint = "CLIC EN CAMPOS PARA EDITAR O CAMBIAR | ENTER O BOTON ROJO PARA INICIAR | ESC PARA CANCELAR";
    const int hintX = mx + (static_cast<int>(modalW) - static_cast<int>(hint.size() * 6)) / 2;
    drawText(hintX, my + static_cast<int>(modalH) - 16, hint, ColorRGB{150, 145, 140}, 1);
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

void Renderer::renderGameOver(const WorldCreationConfig& config, int currentYear, int eraIndex, int hoveredButtonId) {
    // 1. Shroud / Velo de extinción litúrgica (oscurecimiento profundo con tinte carmesí basáltico sobre la simulación)
    for (int y = 0; y < m_height; ++y) {
        int fy = m_height - 1 - y;
        for (int x = 0; x < m_width; ++x) {
            uint32_t px = m_framebuffer[fy * m_width + x];
            uint8_t r = static_cast<uint8_t>(((px & 0xFF) * 35) / 255);
            uint8_t g = static_cast<uint8_t>((((px >> 8) & 0xFF) * 25) / 255);
            uint8_t b = static_cast<uint8_t>((((px >> 16) & 0xFF) * 25) / 255);
            m_framebuffer[fy * m_width + x] = (0xFFu << 24) | (b << 16) | (g << 8) | r;
        }
    }

    const int modalW = 860;
    const int modalH = 480;
    const int mx = (m_width - modalW) / 2;
    const int my = (m_height - modalH) / 2;

    // 2. Fondo del sarcófago/tablilla ancestral (Basalto oscuro)
    drawRect(mx, my, modalW, modalH, ColorRGB{20, 16, 18});
    
    // Doble borde litúrgico: Exterior Sangre de Rubí, Interior Oro Divino
    drawRectOutline(mx, my, modalW, modalH, ColorRGB{128, 20, 32});
    drawRectOutline(mx + 2, my + 2, modalW - 4, modalH - 4, ColorRGB{40, 12, 18});
    drawRectOutline(mx + 4, my + 4, modalW - 8, modalH - 8, Palette::DIVINE_GOLD);

    // Ornamentos de esquinas en oro
    for (int ox : {mx + 6, mx + modalW - 14}) {
        for (int oy : {my + 6, my + modalH - 14}) {
            drawRect(ox, oy, 8, 8, Palette::DIVINE_GOLD);
            drawRect(ox + 2, oy + 2, 4, 4, ColorRGB{128, 20, 32});
        }
    }

    // 3. Franja superior de encabezado fúnebre
    const int headH = 56;
    drawRect(mx + 6, my + 6, modalW - 12, headH, ColorRGB{34, 12, 16});
    drawRect(mx + 6, my + 6 + headH, modalW - 12, 1, Palette::DIVINE_GOLD);

    std::string mainTitle = "EXTINCION TOTAL DE LA FE";
    int titleW = static_cast<int>(mainTitle.size()) * 12; // escala 2 = 6*2 = 12
    drawText(mx + (modalW - titleW) / 2, my + 14, mainTitle, Palette::DIVINE_GOLD, 2);

    std::string subTitle = "DERROTA TRASCENDENTAL - FIN DEL CULTO SAGRADO";
    int subW = static_cast<int>(subTitle.size()) * 6;
    drawText(mx + (modalW - subW) / 2, my + 42, subTitle, ColorRGB{230, 70, 70}, 1);

    // 4. Epitafio teológico (El porqué de la derrota)
    const int epiY = my + 72;
    const int epiH = 88;
    drawRect(mx + 25, epiY, modalW - 50, epiH, ColorRGB{14, 11, 13});
    drawRectOutline(mx + 25, epiY, modalW - 50, epiH, ColorRGB{65, 45, 50});

    std::string line1 = "Todos tus fieles y sacerdotes han perecido. Sin almas vivas para orar y replicar tu fe,";
    std::string line2 = "tu presencia divina se desvanece inexorablemente en las arenas del olvido eterno.";
    std::string line3 = "Los altares se han enfriado y tus mandamientos han dejado de resonar en la tierra.";
    
    drawText(mx + 45, epiY + 16, line1, Palette::RELIC_BONE, 1);
    drawText(mx + 45, epiY + 36, line2, Palette::RELIC_BONE, 1);
    drawText(mx + 45, epiY + 56, line3, ColorRGB{190, 150, 150}, 1);

    // 5. Registro Histórico / Métricas de la Religión Caída
    const int regY = my + 172;
    const int regH = 215;
    drawRect(mx + 25, regY, modalW - 50, regH, ColorRGB{16, 13, 15});
    drawRectOutline(mx + 25, regY, modalW - 50, regH, ColorRGB{55, 45, 38});

    // Título de la sección
    drawText(mx + 45, regY + 14, "CRONICA DEL OCASO DIVINO", Palette::DIVINE_GOLD, 1);
    drawRect(mx + 45, regY + 28, modalW - 90, 1, ColorRGB{70, 55, 40});

    // Columna 1
    int col1X = mx + 45;
    int curY = regY + 40;
    
    drawText(col1X, curY, "DEIDAD OLVIDADA:", ColorRGB{160, 160, 160}, 1);
    drawText(col1X + 170, curY, config.godName, Palette::DIVINE_GOLD, 1);
    curY += 24;

    drawText(col1X, curY, "CULTO EXTINTO:", ColorRGB{160, 160, 160}, 1);
    drawText(col1X + 170, curY, config.religionName, Palette::RELIC_BONE, 1);
    curY += 24;

    drawText(col1X, curY, "ANO DEL COLAPSO:", ColorRGB{160, 160, 160}, 1);
    drawText(col1X + 170, curY, Timeline::formatYear(currentYear), ColorRGB{240, 200, 100}, 1);
    curY += 24;

    // Columna 2
    int col2X = mx + 440;
    int cur2Y = regY + 40;

    std::string eraName = AdvancementSystem::getEraName(static_cast<HistoricalEra>(eraIndex));
    drawText(col2X, cur2Y, "ERA ALCANZADA:", ColorRGB{160, 160, 160}, 1);
    drawText(col2X + 150, cur2Y, eraName, Palette::RELIC_BONE, 1);
    cur2Y += 24;

    drawText(col2X, cur2Y, "CONDICION FATAL:", ColorRGB{160, 160, 160}, 1);
    drawText(col2X + 150, cur2Y, "Extincion de Fieles (0 vivos)", ColorRGB{230, 60, 60}, 1);
    cur2Y += 24;

    drawText(col2X, cur2Y, "ESTADO TEOLOGICO:", ColorRGB{160, 160, 160}, 1);
    drawText(col2X + 150, cur2Y, "Culto Muerto (Sin replicacion)", ColorRGB{180, 160, 140}, 1);
    cur2Y += 24;

    // Sentencia filosófica litúrgica
    drawRect(mx + 45, regY + 135, modalW - 90, 1, ColorRGB{50, 40, 35});
    std::string quote = "\"Sin mortales que transmitan el dogma de generacion en generacion, los dioses mueren.\"";
    int quoteW = static_cast<int>(quote.size()) * 6;
    drawText(mx + (modalW - quoteW) / 2, regY + 152, quote, ColorRGB{200, 180, 130}, 1);

    std::string quoteRef = "-- Tratado de Replicacion Teologica y Demografia Sagrada";
    int qrefW = static_cast<int>(quoteRef.size()) * 6;
    drawText(mx + (modalW - qrefW) / 2, regY + 174, quoteRef, ColorRGB{140, 120, 100}, 1);

    // 6. Botones de acción inferiores
    const int btnY = my + modalH - 65;
    const int btnH = 44;
    const int btnW = 240;

    // ID 901: REINTENTAR GÉNESIS
    bool retryHovered = (hoveredButtonId == 901);
    ColorRGB retryBg = retryHovered ? ColorRGB{150, 35, 45} : Palette::BLOOD_RUBY;
    drawRect(mx + 35, btnY, btnW, btnH, retryBg);
    drawRectOutline(mx + 35, btnY, btnW, btnH, retryHovered ? Palette::DIVINE_GOLD : ColorRGB{180, 140, 60});
    std::string retryText = "REINTENTAR GENESIS";
    drawText(mx + 35 + (btnW - static_cast<int>(retryText.size() * 6)) / 2, btnY + 17, retryText, ColorRGB{255, 255, 240}, 1);

    // ID 902: CARGAR PARTIDA
    bool loadHovered = (hoveredButtonId == 902);
    ColorRGB loadBg = loadHovered ? ColorRGB{52, 42, 30} : ColorRGB{34, 30, 32};
    drawRect(mx + 310, btnY, btnW, btnH, loadBg);
    drawRectOutline(mx + 310, btnY, btnW, btnH, loadHovered ? Palette::DIVINE_GOLD : ColorRGB{80, 75, 70});
    std::string loadText = "CARGAR PARTIDA";
    drawText(mx + 310 + (btnW - static_cast<int>(loadText.size() * 6)) / 2, btnY + 17, loadText, Palette::DIVINE_GOLD, 1);

    // ID 903: MENÚ PRINCIPAL
    bool menuHovered = (hoveredButtonId == 903);
    ColorRGB menuBg = menuHovered ? ColorRGB{52, 42, 30} : ColorRGB{34, 30, 32};
    drawRect(mx + 585, btnY, btnW, btnH, menuBg);
    drawRectOutline(mx + 585, btnY, btnW, btnH, menuHovered ? Palette::DIVINE_GOLD : ColorRGB{80, 75, 70});
    std::string menuText = "MENU PRINCIPAL";
    drawText(mx + 585 + (btnW - static_cast<int>(menuText.size() * 6)) / 2, btnY + 17, menuText, Palette::RELIC_BONE, 1);
}

} // namespace ReligionSim
