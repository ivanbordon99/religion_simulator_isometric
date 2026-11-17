#include "Renderer.hpp"
#include "DayNightCycle.hpp"
#include "CloudSystem.hpp"
#include "SaveSystem.hpp"
#include "VillagerSpriteData.hpp"
#include "DejaVuSansMonoFont.hpp"
#include "NemesisSififus.hpp"
#include "ReligiousMoralitySystem.hpp"
#include "ErrorSystem.hpp"
#include "TimeEngine.hpp"
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

    constexpr std::array<const char*, 7> RESOURCE_PROP_PATHS{{
        "assets/sprites/resources/resource_food.rgba",
        "assets/sprites/resources/resource_wood.rgba",
        "assets/sprites/resources/resource_stone.rgba",
        "assets/sprites/resources/resource_gold.rgba",
        "assets/sprites/resources/resource_iron.rgba",
        "assets/sprites/resources/resource_copper.rgba",
        "assets/sprites/resources/resource_diamond.rgba"
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
    loadFireAssets();
    loadWaterAssets();
    loadDestructionAssets();
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

                // EN: Equalize perimeter margins to create mathematically seamless wrapping tiles without borders or seams
                // ES: Ecualizar márgenes perimetrales para crear mosaicos continuos matemáticamente sin bordes ni aristas
                m_seamlessAtlasPixels = m_terrainAtlasPixels;
                constexpr int ATLAS_TILE_DIMENSION = 64;
                constexpr int TOTAL_ATLAS_TILES = TERRAIN_ATLAS_WIDTH / ATLAS_TILE_DIMENSION;
                constexpr int BLEND_MARGIN = 8;
                constexpr int BORDER_CROP = 4;

                for (int atlasTileIndex = 0; atlasTileIndex < TOTAL_ATLAS_TILES; ++atlasTileIndex) {
                    // EN: Extract original tile texels from atlas
                    // ES: Extraer téxeles originales del mosaico desde el atlas
                    std::vector<uint32_t> currentTilePixels(ATLAS_TILE_DIMENSION * ATLAS_TILE_DIMENSION);
                    for (int tileCoordY = 0; tileCoordY < ATLAS_TILE_DIMENSION; ++tileCoordY) {
                        for (int tileCoordX = 0; tileCoordX < ATLAS_TILE_DIMENSION; ++tileCoordX) {
                            currentTilePixels[tileCoordY * ATLAS_TILE_DIMENSION + tileCoordX] =
                                m_terrainAtlasPixels[tileCoordY * TERRAIN_ATLAS_WIDTH + (atlasTileIndex * ATLAS_TILE_DIMENSION) + tileCoordX];
                        }
                    }

                    // EN: For tiles with authored perimeter edge borders (1/5: Bog, 2: Tropical Plains/Coastal Meadow, 4/13: Deep Ocean, 6: Wetlands, 10: Shallow Water, 12: Cave Water, 14: Ocean), crop perimeter shadow borders and upscale
                    // ES: Para mosaicos con marcos o bordes oscuros de autoría (1/5: Pantano, 2: Pradera tropical, 4/13: Océano profundo, 6: Humedales, 10: Agua somera, 12: Agua de cueva, 14: Océano), recortar marcos y escalar
                    const bool tileHasPerimeterBorder = (atlasTileIndex == 1 || atlasTileIndex == 2 ||
                                                         atlasTileIndex == 4 || atlasTileIndex == 5 ||
                                                         atlasTileIndex == 6 || atlasTileIndex == 10 ||
                                                         atlasTileIndex == 12 || atlasTileIndex == 13 ||
                                                         atlasTileIndex == 14);
                    if (tileHasPerimeterBorder) {
                        std::vector<uint32_t> croppedTilePixels(ATLAS_TILE_DIMENSION * ATLAS_TILE_DIMENSION);
                        const float sourceSpan = static_cast<float>(ATLAS_TILE_DIMENSION - 1 - 2 * BORDER_CROP);
                        for (int destY = 0; destY < ATLAS_TILE_DIMENSION; ++destY) {
                            const float sourceCoordY = static_cast<float>(BORDER_CROP) + (static_cast<float>(destY) / static_cast<float>(ATLAS_TILE_DIMENSION - 1)) * sourceSpan;
                            const int sourceY0 = std::clamp(static_cast<int>(std::floor(sourceCoordY)), 0, ATLAS_TILE_DIMENSION - 1);
                            const int sourceY1 = std::clamp(sourceY0 + 1, 0, ATLAS_TILE_DIMENSION - 1);
                            const float fractionY = sourceCoordY - static_cast<float>(sourceY0);

                            for (int destX = 0; destX < ATLAS_TILE_DIMENSION; ++destX) {
                                const float sourceCoordX = static_cast<float>(BORDER_CROP) + (static_cast<float>(destX) / static_cast<float>(ATLAS_TILE_DIMENSION - 1)) * sourceSpan;
                                const int sourceX0 = std::clamp(static_cast<int>(std::floor(sourceCoordX)), 0, ATLAS_TILE_DIMENSION - 1);
                                const int sourceX1 = std::clamp(sourceX0 + 1, 0, ATLAS_TILE_DIMENSION - 1);
                                const float fractionX = sourceCoordX - static_cast<float>(sourceX0);

                                const uint32_t sample00 = currentTilePixels[sourceY0 * ATLAS_TILE_DIMENSION + sourceX0];
                                const uint32_t sample10 = currentTilePixels[sourceY0 * ATLAS_TILE_DIMENSION + sourceX1];
                                const uint32_t sample01 = currentTilePixels[sourceY1 * ATLAS_TILE_DIMENSION + sourceX0];
                                const uint32_t sample11 = currentTilePixels[sourceY1 * ATLAS_TILE_DIMENSION + sourceX1];

                                const float weight00 = (1.0f - fractionX) * (1.0f - fractionY);
                                const float weight10 = fractionX * (1.0f - fractionY);
                                const float weight01 = (1.0f - fractionX) * fractionY;
                                const float weight11 = fractionX * fractionY;

                                const float blendedRed = (sample00 & 0xFF) * weight00 + (sample10 & 0xFF) * weight10 + (sample01 & 0xFF) * weight01 + (sample11 & 0xFF) * weight11;
                                const float blendedGreen = ((sample00 >> 8) & 0xFF) * weight00 + ((sample10 >> 8) & 0xFF) * weight10 + ((sample01 >> 8) & 0xFF) * weight01 + ((sample11 >> 8) & 0xFF) * weight11;
                                const float blendedBlue = ((sample00 >> 16) & 0xFF) * weight00 + ((sample10 >> 16) & 0xFF) * weight10 + ((sample01 >> 16) & 0xFF) * weight01 + ((sample11 >> 16) & 0xFF) * weight11;
                                const uint32_t alphaChannel = (sample00 >> 24) & 0xFF;

                                croppedTilePixels[destY * ATLAS_TILE_DIMENSION + destX] =
                                    (alphaChannel << 24) |
                                    (static_cast<uint32_t>(std::clamp(blendedBlue, 0.0f, 255.0f)) << 16) |
                                    (static_cast<uint32_t>(std::clamp(blendedGreen, 0.0f, 255.0f)) << 8) |
                                     static_cast<uint32_t>(std::clamp(blendedRed, 0.0f, 255.0f));
                            }
                        }
                        currentTilePixels = std::move(croppedTilePixels);
                    }

                    // EN: Pass 1: Horizontal border matching (ensures left margin matches right margin smoothly)
                    // ES: Paso 1: Fusión de bordes horizontal (asegura que el margen izquierdo coincida suavemente con el derecho)
                    for (int tileCoordY = 0; tileCoordY < ATLAS_TILE_DIMENSION; ++tileCoordY) {
                        for (int marginOffset = 0; marginOffset < BLEND_MARGIN; ++marginOffset) {
                            const float blendRatio = static_cast<float>(marginOffset) / static_cast<float>(BLEND_MARGIN);
                            const float smoothWeight = blendRatio * blendRatio * (3.0f - 2.0f * blendRatio);

                            const int leftCoordX = marginOffset;
                            const int rightCoordX = ATLAS_TILE_DIMENSION - 1 - marginOffset;

                            const uint32_t leftPixel = currentTilePixels[tileCoordY * ATLAS_TILE_DIMENSION + leftCoordX];
                            const uint32_t rightPixel = currentTilePixels[tileCoordY * ATLAS_TILE_DIMENSION + rightCoordX];

                            const float averageRed = 0.5f * (static_cast<float>(leftPixel & 0xFF) + static_cast<float>(rightPixel & 0xFF));
                            const float averageGreen = 0.5f * (static_cast<float>((leftPixel >> 8) & 0xFF) + static_cast<float>((rightPixel >> 8) & 0xFF));
                            const float averageBlue = 0.5f * (static_cast<float>((leftPixel >> 16) & 0xFF) + static_cast<float>((rightPixel >> 16) & 0xFF));
                            const uint32_t alphaChannel = (leftPixel >> 24) & 0xFF;

                            const float newLeftRed = averageRed * (1.0f - smoothWeight) + static_cast<float>(leftPixel & 0xFF) * smoothWeight;
                            const float newLeftGreen = averageGreen * (1.0f - smoothWeight) + static_cast<float>((leftPixel >> 8) & 0xFF) * smoothWeight;
                            const float newLeftBlue = averageBlue * (1.0f - smoothWeight) + static_cast<float>((leftPixel >> 16) & 0xFF) * smoothWeight;

                            const float newRightRed = averageRed * (1.0f - smoothWeight) + static_cast<float>(rightPixel & 0xFF) * smoothWeight;
                            const float newRightGreen = averageGreen * (1.0f - smoothWeight) + static_cast<float>((rightPixel >> 8) & 0xFF) * smoothWeight;
                            const float newRightBlue = averageBlue * (1.0f - smoothWeight) + static_cast<float>((rightPixel >> 16) & 0xFF) * smoothWeight;

                            currentTilePixels[tileCoordY * ATLAS_TILE_DIMENSION + leftCoordX] =
                                (alphaChannel << 24) |
                                (static_cast<uint32_t>(std::clamp(newLeftBlue, 0.0f, 255.0f)) << 16) |
                                (static_cast<uint32_t>(std::clamp(newLeftGreen, 0.0f, 255.0f)) << 8) |
                                 static_cast<uint32_t>(std::clamp(newLeftRed, 0.0f, 255.0f));

                            currentTilePixels[tileCoordY * ATLAS_TILE_DIMENSION + rightCoordX] =
                                (alphaChannel << 24) |
                                (static_cast<uint32_t>(std::clamp(newRightBlue, 0.0f, 255.0f)) << 16) |
                                (static_cast<uint32_t>(std::clamp(newRightGreen, 0.0f, 255.0f)) << 8) |
                                 static_cast<uint32_t>(std::clamp(newRightRed, 0.0f, 255.0f));
                        }
                    }

                    // EN: Pass 2: Vertical border matching (ensures top margin matches bottom margin smoothly)
                    // ES: Paso 2: Fusión de bordes vertical (asegura que el margen superior coincida suavemente con el inferior)
                    for (int tileCoordX = 0; tileCoordX < ATLAS_TILE_DIMENSION; ++tileCoordX) {
                        for (int marginOffset = 0; marginOffset < BLEND_MARGIN; ++marginOffset) {
                            const float blendRatio = static_cast<float>(marginOffset) / static_cast<float>(BLEND_MARGIN);
                            const float smoothWeight = blendRatio * blendRatio * (3.0f - 2.0f * blendRatio);

                            const int topCoordY = marginOffset;
                            const int bottomCoordY = ATLAS_TILE_DIMENSION - 1 - marginOffset;

                            const uint32_t topPixel = currentTilePixels[topCoordY * ATLAS_TILE_DIMENSION + tileCoordX];
                            const uint32_t bottomPixel = currentTilePixels[bottomCoordY * ATLAS_TILE_DIMENSION + tileCoordX];

                            const float averageRed = 0.5f * (static_cast<float>(topPixel & 0xFF) + static_cast<float>(bottomPixel & 0xFF));
                            const float averageGreen = 0.5f * (static_cast<float>((topPixel >> 8) & 0xFF) + static_cast<float>((bottomPixel >> 8) & 0xFF));
                            const float averageBlue = 0.5f * (static_cast<float>((topPixel >> 16) & 0xFF) + static_cast<float>((bottomPixel >> 16) & 0xFF));
                            const uint32_t alphaChannel = (topPixel >> 24) & 0xFF;

                            const float newTopRed = averageRed * (1.0f - smoothWeight) + static_cast<float>(topPixel & 0xFF) * smoothWeight;
                            const float newTopGreen = averageGreen * (1.0f - smoothWeight) + static_cast<float>((topPixel >> 8) & 0xFF) * smoothWeight;
                            const float newTopBlue = averageBlue * (1.0f - smoothWeight) + static_cast<float>((topPixel >> 16) & 0xFF) * smoothWeight;

                            const float newBottomRed = averageRed * (1.0f - smoothWeight) + static_cast<float>(bottomPixel & 0xFF) * smoothWeight;
                            const float newBottomGreen = averageGreen * (1.0f - smoothWeight) + static_cast<float>((bottomPixel >> 8) & 0xFF) * smoothWeight;
                            const float newBottomBlue = averageBlue * (1.0f - smoothWeight) + static_cast<float>((bottomPixel >> 16) & 0xFF) * smoothWeight;

                            currentTilePixels[topCoordY * ATLAS_TILE_DIMENSION + tileCoordX] =
                                (alphaChannel << 24) |
                                (static_cast<uint32_t>(std::clamp(newTopBlue, 0.0f, 255.0f)) << 16) |
                                (static_cast<uint32_t>(std::clamp(newTopGreen, 0.0f, 255.0f)) << 8) |
                                 static_cast<uint32_t>(std::clamp(newTopRed, 0.0f, 255.0f));

                            currentTilePixels[bottomCoordY * ATLAS_TILE_DIMENSION + tileCoordX] =
                                (alphaChannel << 24) |
                                (static_cast<uint32_t>(std::clamp(newBottomBlue, 0.0f, 255.0f)) << 16) |
                                (static_cast<uint32_t>(std::clamp(newBottomGreen, 0.0f, 255.0f)) << 8) |
                                 static_cast<uint32_t>(std::clamp(newBottomRed, 0.0f, 255.0f));
                        }
                    }

                    // EN: Commit the seamless tile into the seamless atlas buffer
                    // ES: Guardar el mosaico continuo en el búfer del atlas continuo
                    for (int tileCoordY = 0; tileCoordY < ATLAS_TILE_DIMENSION; ++tileCoordY) {
                        for (int tileCoordX = 0; tileCoordX < ATLAS_TILE_DIMENSION; ++tileCoordX) {
                            m_seamlessAtlasPixels[tileCoordY * TERRAIN_ATLAS_WIDTH + (atlasTileIndex * ATLAS_TILE_DIMENSION) + tileCoordX] =
                                currentTilePixels[tileCoordY * ATLAS_TILE_DIMENSION + tileCoordX];
                        }
                    }
                }
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
    // EN: 1. Load base resource icons (Food=0, Wood=1, Stone=2, Gold=3, Iron=4, Copper=5, Diamond=6)
    // ES: 1. Cargar iconos base de recursos (Comida=0, Madera=1, Piedra=2, Oro=3, Hierro=4, Cobre=5, Diamante=6)
    for (size_t resourceIndex = 0; resourceIndex < RESOURCE_PROP_PATHS.size(); ++resourceIndex) {
        const std::vector<std::string> candidatePaths = {
            RESOURCE_PROP_PATHS[resourceIndex],
            std::string("../") + RESOURCE_PROP_PATHS[resourceIndex],
            std::string("religion_simulator_isometric/") + RESOURCE_PROP_PATHS[resourceIndex],
            std::string("/home/Ivancho/Documentos/religion_simulator_isometric/") + RESOURCE_PROP_PATHS[resourceIndex]
        };
        bool isLoaded = false;
        for (const auto& singlePath : candidatePaths) {
            std::ifstream fileStream(singlePath, std::ios::binary);
            if (!fileStream.is_open()) continue;
            fileStream.seekg(0, std::ios::end);
            if (fileStream.tellg() != static_cast<std::streamoff>(64u * 64u * 4u)) continue;
            fileStream.seekg(0, std::ios::beg);
            std::vector<uint8_t> rawBuffer(64u * 64u * 4u);
            fileStream.read(reinterpret_cast<char*>(rawBuffer.data()), static_cast<std::streamsize>(rawBuffer.size()));
            if (fileStream.gcount() != static_cast<std::streamsize>(rawBuffer.size())) continue;

            auto& targetPixels = m_resourcePropPixels[resourceIndex];
            targetPixels.resize(64u * 64u);
            for (size_t pixelIndex = 0; pixelIndex < targetPixels.size(); ++pixelIndex) {
                targetPixels[pixelIndex] = (static_cast<uint32_t>(rawBuffer[pixelIndex * 4 + 3]) << 24) |
                                          (static_cast<uint32_t>(rawBuffer[pixelIndex * 4 + 2]) << 16) |
                                          (static_cast<uint32_t>(rawBuffer[pixelIndex * 4 + 1]) << 8) |
                                          static_cast<uint32_t>(rawBuffer[pixelIndex * 4]);
            }
            std::cout << "[Renderer] Sprite base de recurso cargado: " << singlePath << std::endl;
            isLoaded = true;
            break;
        }
        if (!isLoaded) {
            std::cerr << "[Renderer] No se pudo cargar el sprite base de recurso: "
                      << RESOURCE_PROP_PATHS[resourceIndex] << std::endl;
        }
    }

    // EN: 2. Load geological and visual variant sprites for terrain mineral fidelity
    // ES: 2. Cargar sprites de variantes geológicas y visuales para fidelidad mineral en el terreno
    struct VariantPathMapping {
        size_t resIdx;
        size_t varIdx;
        const char* relativeFilePath;
    };

    constexpr std::array<VariantPathMapping, 27> VARIANT_DEFINITIONS{{
        // EN: Wood variants: 0=default, 1=logs, 2=oak, 3=pine, 4=stump
        // ES: Variantes de madera: 0=por defecto, 1=troncos, 2=roble, 3=pino, 4=tocón
        {1, 0, "assets/sprites/resources/resource_wood.rgba"},
        {1, 1, "assets/sprites/resources/resource_wood_logs.rgba"},
        {1, 2, "assets/sprites/resources/resource_wood_oak.rgba"},
        {1, 3, "assets/sprites/resources/resource_wood_pine.rgba"},
        {1, 4, "assets/sprites/resources/resource_wood_stump.rgba"},

        // EN: Stone variants: 0=quarry, 1=boulder, 2=rubble, 3=slab, 4=base
        // ES: Variantes de piedra: 0=cantera, 1=peñasco, 2=cascajo, 3=laja, 4=base
        {2, 0, "assets/sprites/resources/resource_stone_quarry.rgba"},
        {2, 1, "assets/sprites/resources/resource_stone_boulder.rgba"},
        {2, 2, "assets/sprites/resources/resource_stone_rubble.rgba"},
        {2, 3, "assets/sprites/resources/resource_stone_slab.rgba"},
        {2, 4, "assets/sprites/resources/resource_stone.rgba"},

        // EN: Gold variants: 0=vein, 1=rock/base, 2=nuggets, 3=hoard
        // ES: Variantes de oro: 0=veta, 1=roca/base, 2=pepitas, 3=tesoro
        {3, 0, "assets/sprites/resources/resource_gold_vein.rgba"},
        {3, 1, "assets/sprites/resources/resource_gold.rgba"},
        {3, 2, "assets/sprites/resources/resource_gold_nuggets.rgba"},
        {3, 3, "assets/sprites/resources/resource_gold_hoard.rgba"},

        // EN: Iron variants: 0=vein, 1=rock, 2=nuggets, 3=hoard/base
        // ES: Variantes de hierro: 0=veta, 1=roca, 2=pepitas, 3=tesoro/base
        {4, 0, "assets/sprites/resources/resource_iron_vein.rgba"},
        {4, 1, "assets/sprites/resources/resource_iron_rock.rgba"},
        {4, 2, "assets/sprites/resources/resource_iron_nuggets.rgba"},
        {4, 3, "assets/sprites/resources/resource_iron.rgba"},

        // EN: Copper variants: 0=vein, 1=rock, 2=nuggets, 3=hoard, 4=base
        // ES: Variantes de cobre: 0=veta, 1=roca, 2=pepitas, 3=tesoro, 4=base
        {5, 0, "assets/sprites/resources/resource_copper_vein.rgba"},
        {5, 1, "assets/sprites/resources/resource_copper_rock.rgba"},
        {5, 2, "assets/sprites/resources/resource_copper_nuggets.rgba"},
        {5, 3, "assets/sprites/resources/resource_copper_hoard.rgba"},
        {5, 4, "assets/sprites/resources/resource_copper.rgba"},

        // EN: Diamond variants: 0=crystal, 1=rock, 2=nuggets/base, 3=hoard
        // ES: Variantes de diamante: 0=cristal, 1=roca, 2=pepitas/base, 3=tesoro
        {6, 0, "assets/sprites/resources/resource_diamond_crystal.rgba"},
        {6, 1, "assets/sprites/resources/resource_diamond_rock.rgba"},
        {6, 2, "assets/sprites/resources/resource_diamond.rgba"},
        {6, 3, "assets/sprites/resources/resource_diamond_hoard.rgba"}
    }};

    for (const auto& variantDef : VARIANT_DEFINITIONS) {
        const std::vector<std::string> candidatePaths = {
            variantDef.relativeFilePath,
            std::string("../") + variantDef.relativeFilePath,
            std::string("religion_simulator_isometric/") + variantDef.relativeFilePath,
            std::string("/home/Ivancho/Documentos/religion_simulator_isometric/") + variantDef.relativeFilePath
        };
        for (const auto& singlePath : candidatePaths) {
            std::ifstream fileStream(singlePath, std::ios::binary);
            if (!fileStream.is_open()) continue;
            fileStream.seekg(0, std::ios::end);
            if (fileStream.tellg() != static_cast<std::streamoff>(64u * 64u * 4u)) continue;
            fileStream.seekg(0, std::ios::beg);
            std::vector<uint8_t> rawBuffer(64u * 64u * 4u);
            fileStream.read(reinterpret_cast<char*>(rawBuffer.data()), static_cast<std::streamsize>(rawBuffer.size()));
            if (fileStream.gcount() != static_cast<std::streamsize>(rawBuffer.size())) continue;

            auto& targetPixels = m_resourceVariantPixels[variantDef.resIdx][variantDef.varIdx];
            targetPixels.resize(64u * 64u);
            for (size_t pixelIndex = 0; pixelIndex < targetPixels.size(); ++pixelIndex) {
                targetPixels[pixelIndex] = (static_cast<uint32_t>(rawBuffer[pixelIndex * 4 + 3]) << 24) |
                                          (static_cast<uint32_t>(rawBuffer[pixelIndex * 4 + 2]) << 16) |
                                          (static_cast<uint32_t>(rawBuffer[pixelIndex * 4 + 1]) << 8) |
                                          static_cast<uint32_t>(rawBuffer[pixelIndex * 4]);
            }
            break;
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

// EN: Load all pixel-art fire, wildfire, campfire, meteor, and volcanic magma assets
// ES: Cargar todos los assets pixel-art de fuego, incendios, hogueras, meteoritos y magma volcánico
void Renderer::loadFireAssets() {
    auto loadRawRGBA = [](const std::string& relPath, size_t expectedBytes, std::vector<uint32_t>& outBuffer) -> bool {
        std::vector<std::string> searchPaths = {
            relPath,
            "../" + relPath,
            "religion_simulator_isometric/" + relPath,
            "/home/Ivancho/Documentos/religion_simulator_isometric/" + relPath
        };
        for (const auto& path : searchPaths) {
            std::ifstream file(path, std::ios::binary);
            if (!file.is_open()) continue;
            file.seekg(0, std::ios::end);
            size_t fileSize = static_cast<size_t>(file.tellg());
            if (fileSize != expectedBytes) continue;
            file.seekg(0, std::ios::beg);
            std::vector<uint8_t> rawData(fileSize);
            file.read(reinterpret_cast<char*>(rawData.data()), fileSize);
            size_t pixelCount = fileSize / 4;
            outBuffer.resize(pixelCount);
            for (size_t i = 0; i < pixelCount; ++i) {
                uint8_t r = rawData[i * 4 + 0];
                uint8_t g = rawData[i * 4 + 1];
                uint8_t b = rawData[i * 4 + 2];
                uint8_t a = rawData[i * 4 + 3];
                outBuffer[i] = (static_cast<uint32_t>(a) << 24) |
                               (static_cast<uint32_t>(b) << 16) |
                               (static_cast<uint32_t>(g) << 8)  |
                                static_cast<uint32_t>(r);
            }
            std::cout << "[Renderer] Sprite de fuego cargado: " << path << std::endl;
            return true;
        }
        std::cerr << "[Renderer] No se pudo cargar el sprite de fuego: " << relPath << std::endl;
        return false;
    };

    loadRawRGBA("assets/sprites/fire/wildfire_sheet.rgba", 512 * 128 * 4, m_wildfirePixels);
    loadRawRGBA("assets/sprites/fire/flame_sheet.rgba", 384 * 128 * 4, m_flamePixels);
    loadRawRGBA("assets/sprites/fire/campfire_sheet.rgba", 512 * 128 * 4, m_campfirePixels);
    loadRawRGBA("assets/sprites/fire/meteor_giant.rgba", 128 * 128 * 4, m_meteorGiantPixels);
    loadRawRGBA("assets/sprites/fire/meteor_fireball.rgba", 128 * 128 * 4, m_meteorFireballPixels);
    loadRawRGBA("assets/sprites/fire/volcano_eruption_sheet.rgba", 256 * 128 * 4, m_volcanoEruptionPixels);
    loadRawRGBA("assets/sprites/fire/fire_pit.rgba", 128 * 128 * 4, m_firePitPixels);
    loadRawRGBA("assets/sprites/fire/flame_burst_sheet.rgba", 256 * 128 * 4, m_flameBurstPixels);
    loadRawRGBA("assets/sprites/fire/lava_tiles.rgba", 512 * 256 * 4, m_lavaTilesPixels);
    loadRawRGBA("assets/sprites/volcano/volcano_sheet.rgba", 1024 * 765 * 4, m_volcanoSheetPixels);
    loadRawRGBA("assets/sprites/fire/atomic_bomb_sheet.rgba", 1024 * 576 * 4, m_atomicBombSheetPixels);
    loadRawRGBA("assets/sprites/weather/tornado_sheet.rgba", 384 * 128 * 4, m_tornadoSheetPixels);
}

// EN: Load all pixel-art water animation, ripple, coastal surf, ocean wave, current, and whirlpool assets
// ES: Cargar todos los assets pixel-art de animación de agua, ondas, espuma costera, olas oceánicas, corrientes y remolinos
void Renderer::loadWaterAssets() {
    auto loadRawRGBA = [](const std::string& relPath, size_t expectedBytes, std::vector<uint32_t>& outBuffer) -> bool {
        std::vector<std::string> searchPaths = {
            relPath,
            "../" + relPath,
            "religion_simulator_isometric/" + relPath,
            "assets/sprites/water/" + relPath
        };

        std::string finalPath = "";
        std::ifstream file;
        for (const auto& candidate : searchPaths) {
            file.open(candidate, std::ios::binary);
            if (file.is_open()) {
                finalPath = candidate;
                break;
            }
        }

        if (file.is_open()) {
            std::vector<uint8_t> rawBytes(expectedBytes);
            file.read(reinterpret_cast<char*>(rawBytes.data()), expectedBytes);
            const size_t pixelCount = expectedBytes / 4;
            outBuffer.resize(pixelCount);
            for (size_t pixelIndex = 0; pixelIndex < pixelCount; ++pixelIndex) {
                const uint8_t redChannel = rawBytes[pixelIndex * 4 + 0];
                const uint8_t greenChannel = rawBytes[pixelIndex * 4 + 1];
                const uint8_t blueChannel = rawBytes[pixelIndex * 4 + 2];
                const uint8_t alphaChannel = rawBytes[pixelIndex * 4 + 3];
                outBuffer[pixelIndex] = (static_cast<uint32_t>(alphaChannel) << 24) |
                                        (static_cast<uint32_t>(blueChannel) << 16) |
                                        (static_cast<uint32_t>(greenChannel) << 8) |
                                         static_cast<uint32_t>(redChannel);
            }
            std::cout << "[Renderer] Sprite de agua cargado: " << finalPath << std::endl;
            return true;
        }
        std::cerr << "[Renderer] No se pudo cargar el sprite de agua: " << relPath << std::endl;
        return false;
    };

    loadRawRGBA("assets/sprites/water/water_sheet.rgba", 1024 * 765 * 4, m_waterSheetPixels);
    loadRawRGBA("assets/sprites/water/water_ripples_sheet.rgba", 1024 * 170 * 4, m_waterRipplesPixels);
    loadRawRGBA("assets/sprites/water/water_borders_sheet.rgba", 1024 * 175 * 4, m_waterBordersPixels);
    loadRawRGBA("assets/sprites/water/water_waves_sheet.rgba", 1024 * 115 * 4, m_waterWavesPixels);
    loadRawRGBA("assets/sprites/water/water_surf_sheet.rgba", 1024 * 116 * 4, m_waterSurfPixels);
    loadRawRGBA("assets/sprites/water/water_currents_sheet.rgba", 853 * 102 * 4, m_waterCurrentsPixels);
    loadRawRGBA("assets/sprites/water/water_whirlpool_sheet.rgba", 510 * 85 * 4, m_waterWhirlpoolPixels);
    loadRawRGBA("assets/sprites/water/water_splash_sheet.rgba", 425 * 85 * 4, m_waterSplashPixels);
}

// EN: Load all destruction tileset assets: burnt trees, charred stumps, craters, and scorched ground
// ES: Cargar todos los activos del tileset de destrucción: árboles quemados, tocones, cráteres y suelo calcinado
void Renderer::loadDestructionAssets() {
    auto loadRawRGBA = [](const std::string& relPath, size_t expectedBytes, std::vector<uint32_t>& outBuffer) -> bool {
        std::vector<std::string> searchPaths = {
            relPath,
            "../" + relPath,
            "religion_simulator_isometric/" + relPath,
            "/home/Ivancho/Documentos/religion_simulator_isometric/" + relPath
        };

        std::string finalPath = "";
        std::ifstream file;
        for (const auto& candidate : searchPaths) {
            file.open(candidate, std::ios::binary);
            if (file.is_open()) {
                finalPath = candidate;
                break;
            }
        }

        if (file.is_open()) {
            std::vector<uint8_t> rawBytes(expectedBytes);
            file.read(reinterpret_cast<char*>(rawBytes.data()), static_cast<std::streamsize>(expectedBytes));
            if (file.gcount() == static_cast<std::streamsize>(expectedBytes)) {
                const size_t pixelCount = expectedBytes / 4;
                outBuffer.resize(pixelCount);
                for (size_t pixelIndex = 0; pixelIndex < pixelCount; ++pixelIndex) {
                    const uint8_t redChannel = rawBytes[pixelIndex * 4 + 0];
                    const uint8_t greenChannel = rawBytes[pixelIndex * 4 + 1];
                    const uint8_t blueChannel = rawBytes[pixelIndex * 4 + 2];
                    const uint8_t alphaChannel = rawBytes[pixelIndex * 4 + 3];
                    outBuffer[pixelIndex] = (static_cast<uint32_t>(alphaChannel) << 24) |
                                            (static_cast<uint32_t>(blueChannel) << 16) |
                                            (static_cast<uint32_t>(greenChannel) << 8) |
                                             static_cast<uint32_t>(redChannel);
                }
                std::cout << "[Renderer] Sprite de destrucción cargado: " << finalPath << std::endl;
                return true;
            }
        }
        std::cerr << "[Renderer] No se pudo cargar el sprite de destrucción: " << relPath << std::endl;
        return false;
    };

    // 1. Burnt Trees and Charred Stumps (11 sprites, 64x64 RGBA)
    constexpr std::array<const char*, 11> DESTRUCTION_TREE_PATHS{{
        "assets/sprites/destruction/trees/burnt_tree_crooked.rgba",
        "assets/sprites/destruction/trees/burnt_tree_branchy.rgba",
        "assets/sprites/destruction/trees/burnt_stump_small.rgba",
        "assets/sprites/destruction/trees/burnt_stump_flared.rgba",
        "assets/sprites/destruction/trees/burnt_trunk_jagged.rgba",
        "assets/sprites/destruction/trees/burnt_stump_flat.rgba",
        "assets/sprites/destruction/trees/burnt_tree_ember.rgba",
        "assets/sprites/destruction/trees/burnt_rubble_ring.rgba",
        "assets/sprites/destruction/trees/burnt_stump_ember_core.rgba",
        "assets/sprites/destruction/trees/burnt_smoking_camp.rgba",
        "assets/sprites/destruction/trees/burnt_smoking_timber.rgba"
    }};
    m_destructionTreePixels.resize(DESTRUCTION_TREE_PATHS.size());
    for (size_t treeIndex = 0; treeIndex < DESTRUCTION_TREE_PATHS.size(); ++treeIndex) {
        loadRawRGBA(DESTRUCTION_TREE_PATHS[treeIndex], 64 * 64 * 4, m_destructionTreePixels[treeIndex]);
    }

    // 2. Impact Craters (8 sprites: 7 at 64x64, 1 at 128x128)
    constexpr std::array<std::pair<const char*, size_t>, 8> DESTRUCTION_CRATER_PATHS{{
        {"assets/sprites/destruction/craters/crater_dirt_small.rgba", 64 * 64 * 4},
        {"assets/sprites/destruction/craters/crater_dirt_medium.rgba", 64 * 64 * 4},
        {"assets/sprites/destruction/craters/crater_dirt_debris.rgba", 64 * 64 * 4},
        {"assets/sprites/destruction/craters/crater_basalt_rock.rgba", 64 * 64 * 4},
        {"assets/sprites/destruction/craters/crater_cluster_pockmarks.rgba", 64 * 64 * 4},
        {"assets/sprites/destruction/craters/crater_deep_bowl.rgba", 64 * 64 * 4},
        {"assets/sprites/destruction/craters/crater_isolated_rim.rgba", 64 * 64 * 4},
        {"assets/sprites/destruction/craters/crater_giant_colossal.rgba", 128 * 128 * 4}
    }};
    m_destructionCraterPixels.resize(DESTRUCTION_CRATER_PATHS.size());
    for (size_t craterIndex = 0; craterIndex < DESTRUCTION_CRATER_PATHS.size(); ++craterIndex) {
        const auto& [craterPath, craterBytes] = DESTRUCTION_CRATER_PATHS[craterIndex];
        loadRawRGBA(craterPath, craterBytes, m_destructionCraterPixels[craterIndex]);
    }

    // 3. Scorched Terrain & Magma Fissures (8 textures, 64x64 RGBA)
    constexpr std::array<const char*, 8> DESTRUCTION_TERRAIN_PATHS{{
        "assets/sprites/destruction/terrain/scorched_ash_soot.rgba",
        "assets/sprites/destruction/terrain/scorched_ash_charcoal.rgba",
        "assets/sprites/destruction/terrain/scorched_ash_pit.rgba",
        "assets/sprites/destruction/terrain/scorched_magma_crust_dense.rgba",
        "assets/sprites/destruction/terrain/scorched_magma_crust_intense.rgba",
        "assets/sprites/destruction/terrain/scorched_cracked_magma_dry.rgba",
        "assets/sprites/destruction/terrain/scorched_cracked_blast_core.rgba",
        "assets/sprites/destruction/terrain/scorched_cracked_magma_veins.rgba"
    }};
    m_destructionTerrainPixels.resize(DESTRUCTION_TERRAIN_PATHS.size());
    for (size_t terrainIndex = 0; terrainIndex < DESTRUCTION_TERRAIN_PATHS.size(); ++terrainIndex) {
        loadRawRGBA(DESTRUCTION_TERRAIN_PATHS[terrainIndex], 64 * 64 * 4, m_destructionTerrainPixels[terrainIndex]);
    }
}

// EN: Render authentic animated tribal campfire with wooden logs and flickering flames
// ES: Renderizar hoguera tribal animada auténtica con leña de madera y llamas parpadeantes
void Renderer::drawCampfire(float cx, float cy, float zoom, float animTime) {
    if (m_campfirePixels.size() != 512u * 128u) return;

    constexpr int frameW = 128;
    constexpr int frameH = 128;
    constexpr int sheetW = 512;
    constexpr int frameCount = 4;
    const int frameIndex = static_cast<int>(animTime * 8.0f) % frameCount;
    const int frameStartX = frameIndex * frameW;

    const int destWidth = std::max(6, static_cast<int>(36.0f * zoom));
    const int destHeight = std::max(6, static_cast<int>(36.0f * zoom));
    const int startScreenX = static_cast<int>(cx) - destWidth / 2;
    const int startScreenY = static_cast<int>(cy) - destHeight + static_cast<int>(6.0f * zoom);

    for (int dy = 0; dy < destHeight; ++dy) {
        const int targetY = startScreenY + dy;
        const int frameY = m_height - 1 - targetY;
        if (frameY < 0 || frameY >= m_height) continue;

        const int srcY = std::clamp((dy * frameH) / destHeight, 0, frameH - 1);

        for (int dx = 0; dx < destWidth; ++dx) {
            const int targetX = startScreenX + dx;
            if (targetX < 0 || targetX >= m_width) continue;

            const int srcX = std::clamp(frameStartX + (dx * frameW) / destWidth, 0, sheetW - 1);
            const uint32_t srcPixel = m_campfirePixels[srcY * sheetW + srcX];
            const uint8_t srcA = (srcPixel >> 24) & 0xFF;
            if (srcA < 15) continue;

            const uint8_t srcR = srcPixel & 0xFF;
            const uint8_t srcG = (srcPixel >> 8) & 0xFF;
            const uint8_t srcB = (srcPixel >> 16) & 0xFF;

            const float alphaFactor = (srcA / 255.0f);
            const uint32_t bgPixel = m_framebuffer[frameY * m_width + targetX];
            const uint8_t bgR = bgPixel & 0xFF;
            const uint8_t bgG = (bgPixel >> 8) & 0xFF;
            const uint8_t bgB = (bgPixel >> 16) & 0xFF;

            const uint8_t outR = static_cast<uint8_t>(srcR * alphaFactor + bgR * (1.0f - alphaFactor));
            const uint8_t outG = static_cast<uint8_t>(srcG * alphaFactor + bgG * (1.0f - alphaFactor));
            const uint8_t outB = static_cast<uint8_t>(srcB * alphaFactor + bgB * (1.0f - alphaFactor));

            m_framebuffer[frameY * m_width + targetX] = (0xFFu << 24) | (outB << 16) | (outG << 8) | outR;
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
    // 1. Controles de simulación JUSTO DEBAJO de la línea de tiempo (teclas de casetera vintage y botón redondo de play)
    // EN: Bottom central panel simulation controls: vintage cassette mechanical keys and centered round play button
    // ES: Controles de simulación del panel central inferior: teclas mecánicas de casetera vintage y botón redondo de play centrado
    const float simY = botY + 24.0f;
    const float simH = 26.0f;
    const float pauseWidth = 100.0f;
    const float playDiameter = 30.0f;
    const float fastForwardWidth = 84.0f;
    const float ultraForwardWidth = 84.0f;
    const float playGap = 12.0f;
    const float buttonGap = 8.0f;
    {
        const float totalControlsWidth = pauseWidth + playGap + playDiameter + playGap + fastForwardWidth + buttonGap + ultraForwardWidth; // 330px
        float startX = static_cast<float>(azulX) + (static_cast<float>(azulW) - totalControlsWidth) * 0.5f;

        // 201: Cassette PAUSE key (❚❚ PAUSA)
        m_buttons.push_back({201, startX, simY, pauseWidth, simH, "PAUSA", false});

        // 202: ROUND PLAY button (▶ centered vertically and horizontally)
        float playY = simY + (simH - playDiameter) * 0.5f;
        float playX = startX + pauseWidth + playGap;
        m_buttons.push_back({202, playX, playY, playDiameter, playDiameter, "", true});

        // 203: Cassette 2x Fast Forward key (▶▶ 2x)
        float ffX = playX + playDiameter + playGap;
        m_buttons.push_back({203, ffX, simY, fastForwardWidth, simH, "2x", false});

        // 204: Cassette 5x Ultra Fast Forward key (▶▶▶ 5x)
        float uffX = ffX + fastForwardWidth + buttonGap;
        m_buttons.push_back({204, uffX, simY, ultraForwardWidth, simH, "5x", false});
    }

    // 2. Poderes de Dios y Catástrofes Naturales DEBAJO de los controles de simulación:
    // Botones pequeños cuadrados con sprites representativos como los botones de las vistas
    {
        const float powSize = 25.0f;
        const float powY = botY + 54.0f;
        const float gap = 5.0f;
        const float sepGap = 12.0f; // Separador entre Poderes Sagrados y Catástrofes

        const float totalWidth = (4 * powSize + 3 * gap) + sepGap + (8 * powSize + 7 * gap); // 362px
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

        // 8 Catástrofes Naturales y Fenómenos Hidrológicos
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
        curX += powSize + gap;

        m_buttons.push_back({726, curX, powY, powSize, powSize, "[L] LLUVIA", false});
        curX += powSize + gap;

        m_buttons.push_back({727, curX, powY, powSize, powSize, "[J] DILUVIO", false});
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

    // EN: Centered liturgical Main Menu buttons with balanced isometric spacing
    // ES: Botones litúrgicos centrados del Menú Principal con espaciado isométrico equilibrado
    constexpr float buttonWidth = 280.0f;
    constexpr float buttonHeight = 44.0f;
    constexpr float buttonSpacing = 14.0f;
    constexpr int totalButtonCount = 6;
    const float totalButtonsHeight = totalButtonCount * buttonHeight + (totalButtonCount - 1) * buttonSpacing;

    const float buttonStartX = (static_cast<float>(m_width) - buttonWidth) * 0.5f;
    const float buttonStartY = (static_cast<float>(m_height) - totalButtonsHeight) * 0.5f + 36.0f;

    m_menuButtons.push_back({1, buttonStartX, buttonStartY + 0.0f * (buttonHeight + buttonSpacing), buttonWidth, buttonHeight, "CREAR MUNDO", false});
    m_menuButtons.push_back({2, buttonStartX, buttonStartY + 1.0f * (buttonHeight + buttonSpacing), buttonWidth, buttonHeight, "CARGAR MUNDO", false});
    m_menuButtons.push_back({6, buttonStartX, buttonStartY + 2.0f * (buttonHeight + buttonSpacing), buttonWidth, buttonHeight, "CREACION DE MAPAS", false});
    m_menuButtons.push_back({3, buttonStartX, buttonStartY + 3.0f * (buttonHeight + buttonSpacing), buttonWidth, buttonHeight, "APRENDE A JUGAR", false});
    m_menuButtons.push_back({4, buttonStartX, buttonStartY + 4.0f * (buttonHeight + buttonSpacing), buttonWidth, buttonHeight, "OPCIONES", false});
    m_menuButtons.push_back({5, buttonStartX, buttonStartY + 5.0f * (buttonHeight + buttonSpacing), buttonWidth, buttonHeight, "SALIR", false});

    // EN: Initialize modular options menu categories, tabs, and action controls
    // ES: Inicializar categorías, pestañas y controles de acción modulares del menú de opciones
    initOptionsMenuUI();

    // EN: Initialize tutorial and guide codex tabs and action controls
    // ES: Inicializar pestañas y controles de acción del códice de tutorial y guía
    initTutorialUI();

    // EN: Initialize Map Creation and Editor UI buttons and tool palette
    // ES: Inicializar botones y paleta de herramientas de la UI de Creación y Edición de Mapas
    initMapEditorUI();
    initMapEditorIsometricUI();

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

void Renderer::initOptionsMenuUI() {
    m_optionsTabButtons.clear();
    m_optionsScreenButtons.clear();
    m_optionsGraphicsButtons.clear();
    m_optionsGameplayButtons.clear();
    m_optionsAudioButtons.clear();
    m_optionsActionButtons.clear();

    // EN: Centered altar dimensions for the liturgical options modal
    // ES: Dimensiones del altar central para el modal litúrgico de opciones
    constexpr float modalWidth = 960.0f;
    constexpr float modalHeight = 590.0f;
    const float modalStartX = (static_cast<float>(m_width) - modalWidth) * 0.5f;
    const float modalStartY = (static_cast<float>(m_height) - modalHeight) * 0.5f;

    // EN: Block 1 - Category navigation tabs along the top header
    // ES: Bloque 1 - Pestañas de navegación de categorías a lo largo de la cabecera superior
    constexpr float categoryTabWidth = 215.0f;
    constexpr float categoryTabHeight = 28.0f;
    constexpr float categoryTabSpacing = 8.0f;
    constexpr float totalCategoryTabsWidth = 4.0f * categoryTabWidth + 3.0f * categoryTabSpacing;
    const float tabsHeaderStartX = modalStartX + (modalWidth - totalCategoryTabsWidth) * 0.5f;
    const float tabsHeaderStartY = modalStartY + 44.0f;

    m_optionsTabButtons.push_back({1001, tabsHeaderStartX + 0.0f * (categoryTabWidth + categoryTabSpacing), tabsHeaderStartY, categoryTabWidth, categoryTabHeight, "PANTALLA Y VIDEO", false});
    m_optionsTabButtons.push_back({1002, tabsHeaderStartX + 1.0f * (categoryTabWidth + categoryTabSpacing), tabsHeaderStartY, categoryTabWidth, categoryTabHeight, "GRAFICOS E IMAGEN", false});
    m_optionsTabButtons.push_back({1003, tabsHeaderStartX + 2.0f * (categoryTabWidth + categoryTabSpacing), tabsHeaderStartY, categoryTabWidth, categoryTabHeight, "JUGABILIDAD", false});
    m_optionsTabButtons.push_back({1004, tabsHeaderStartX + 3.0f * (categoryTabWidth + categoryTabSpacing), tabsHeaderStartY, categoryTabWidth, categoryTabHeight, "AUDIO Y SONIDO", false});

    // EN: Block 2 - Two-column layout grid for interactive option toggles
    // ES: Bloque 2 - Cuadrícula de diseño en dos columnas para los selectores interactivos de opciones
    const float firstColumnStartX = modalStartX + 45.0f;
    const float secondColumnStartX = modalStartX + modalWidth * 0.5f + 15.0f;
    constexpr float optionControlWidth = 425.0f;
    constexpr float optionControlHeight = 38.0f;
    constexpr float verticalRowStep = 48.0f;
    const float gridRowsStartY = modalStartY + 95.0f;

    // --- Pestaña 1: Pantalla y Video ---
    m_optionsScreenButtons.push_back({1010, firstColumnStartX, gridRowsStartY + 0.0f * verticalRowStep, optionControlWidth, optionControlHeight, "RESOLUCION", false});
    m_optionsScreenButtons.push_back({1011, secondColumnStartX, gridRowsStartY + 0.0f * verticalRowStep, optionControlWidth, optionControlHeight, "MODO DE PANTALLA", false});
    m_optionsScreenButtons.push_back({1012, firstColumnStartX, gridRowsStartY + 1.0f * verticalRowStep, optionControlWidth, optionControlHeight, "LIMITE DE FPS", false});
    m_optionsScreenButtons.push_back({1013, secondColumnStartX, gridRowsStartY + 1.0f * verticalRowStep, optionControlWidth, optionControlHeight, "SINCRONIZACION V-SYNC", false});
    m_optionsScreenButtons.push_back({1014, firstColumnStartX, gridRowsStartY + 2.0f * verticalRowStep, optionControlWidth, optionControlHeight, "ESCALA DE INTERFAZ (UI)", false});
    m_optionsScreenButtons.push_back({1015, secondColumnStartX, gridRowsStartY + 2.0f * verticalRowStep, optionControlWidth, optionControlHeight, "BRILLO Y GAMMA", false});

    // --- Pestaña 2: Gráficos e Imagen ---
    m_optionsGraphicsButtons.push_back({1020, firstColumnStartX, gridRowsStartY + 0.0f * verticalRowStep, optionControlWidth, optionControlHeight, "SOMBRAS DE NUBES", false});
    m_optionsGraphicsButtons.push_back({1021, secondColumnStartX, gridRowsStartY + 0.0f * verticalRowStep, optionControlWidth, optionControlHeight, "ILUMINACION DIA/NOCHE", false});
    m_optionsGraphicsButtons.push_back({1022, firstColumnStartX, gridRowsStartY + 1.0f * verticalRowStep, optionControlWidth, optionControlHeight, "DETALLE DE AGUA Y OLAS", false});
    m_optionsGraphicsButtons.push_back({1023, secondColumnStartX, gridRowsStartY + 1.0f * verticalRowStep, optionControlWidth, optionControlHeight, "VINETEADO CINEMATICO", false});
    m_optionsGraphicsButtons.push_back({1024, firstColumnStartX, gridRowsStartY + 2.0f * verticalRowStep, optionControlWidth, optionControlHeight, "SACUDIDA SISMICA (SHAKE)", false});
    m_optionsGraphicsButtons.push_back({1025, secondColumnStartX, gridRowsStartY + 2.0f * verticalRowStep, optionControlWidth, optionControlHeight, "FILTRO LITURGICO", false});
    m_optionsGraphicsButtons.push_back({1026, firstColumnStartX, gridRowsStartY + 3.0f * verticalRowStep, optionControlWidth, optionControlHeight, "RETICULA DE TERRENO", false});
    m_optionsGraphicsButtons.push_back({1027, secondColumnStartX, gridRowsStartY + 3.0f * verticalRowStep, optionControlWidth, optionControlHeight, "MODO DESARROLLADOR [F3]", false});

    // --- Pestaña 3: Jugabilidad y Simulación ---
    m_optionsGameplayButtons.push_back({1030, firstColumnStartX, gridRowsStartY + 0.0f * verticalRowStep, optionControlWidth, optionControlHeight, "VELOCIDAD BASE TEMPORAL", false});
    m_optionsGameplayButtons.push_back({1031, secondColumnStartX, gridRowsStartY + 0.0f * verticalRowStep, optionControlWidth, optionControlHeight, "PAUSA EN CATASTROFES", false});
    m_optionsGameplayButtons.push_back({1032, firstColumnStartX, gridRowsStartY + 1.0f * verticalRowStep, optionControlWidth, optionControlHeight, "VELOCIDAD DE CAMARA", false});
    m_optionsGameplayButtons.push_back({1033, secondColumnStartX, gridRowsStartY + 1.0f * verticalRowStep, optionControlWidth, optionControlHeight, "INVERTIR ARRASTRE RATON", false});
    m_optionsGameplayButtons.push_back({1034, firstColumnStartX, gridRowsStartY + 2.0f * verticalRowStep, optionControlWidth, optionControlHeight, "TEXTOS FLOTANTES DE FE", false});
    m_optionsGameplayButtons.push_back({1035, secondColumnStartX, gridRowsStartY + 2.0f * verticalRowStep, optionControlWidth, optionControlHeight, "NOTIFICACIONES HISTORICAS", false});

    // --- Pestaña 4: Audio y Sonido ---
    m_optionsAudioButtons.push_back({1040, firstColumnStartX, gridRowsStartY + 0.0f * verticalRowStep, optionControlWidth, optionControlHeight, "VOLUMEN MAESTRO", false});
    m_optionsAudioButtons.push_back({1041, secondColumnStartX, gridRowsStartY + 0.0f * verticalRowStep, optionControlWidth, optionControlHeight, "MUSICA SACRA AMBIENTAL", false});
    m_optionsAudioButtons.push_back({1042, firstColumnStartX, gridRowsStartY + 1.0f * verticalRowStep, optionControlWidth, optionControlHeight, "EFECTOS DE SONIDO (SFX)", false});
    m_optionsAudioButtons.push_back({1043, secondColumnStartX, gridRowsStartY + 1.0f * verticalRowStep, optionControlWidth, optionControlHeight, "PAISAJE SONORO NATURAL", false});
    m_optionsAudioButtons.push_back({1044, firstColumnStartX, gridRowsStartY + 2.0f * verticalRowStep, optionControlWidth, optionControlHeight, "SILENCIAR TODO (MUTE)", false});

    // EN: Block 3 - Bottom confirmation and restore action buttons
    // ES: Bloque 3 - Botones de confirmación y restauración de acciones inferiores
    constexpr float bottomActionWidth = 290.0f;
    constexpr float bottomActionHeight = 36.0f;
    const float bottomActionsStartY = modalStartY + modalHeight - 52.0f;

    m_optionsActionButtons.push_back({1090, modalStartX + 45.0f, bottomActionsStartY, bottomActionWidth, bottomActionHeight, "RESTAURAR POR DEFECTO", false});
    m_optionsActionButtons.push_back({1091, modalStartX + modalWidth - 45.0f - bottomActionWidth, bottomActionsStartY, bottomActionWidth, bottomActionHeight, "GUARDAR Y VOLVER", false});

    // EN: Maintain fallback backward compatibility with legacy button list
    // ES: Mantener compatibilidad hacia atrás con la lista de botones legada
    m_optionsButtons = m_optionsActionButtons;
    m_optionsButtons.push_back({12, modalStartX + modalWidth - 45.0f - bottomActionWidth, bottomActionsStartY, bottomActionWidth, bottomActionHeight, "VOLVER AL MENU", false});
}

int Renderer::checkOptionsMenuButtonClicked(float mouseX, float mouseY, OptionsTab currentTab) const {
    // EN: Check category tabs first
    // ES: Chequear pestañas de categoría primero
    for (const auto& tabButton : m_optionsTabButtons) {
        if (mouseX >= tabButton.x && mouseX <= tabButton.x + tabButton.width &&
            mouseY >= tabButton.y && mouseY <= tabButton.y + tabButton.height) {
            return tabButton.id;
        }
    }

    // EN: Check bottom action buttons
    // ES: Chequear botones de acción inferior
    for (const auto& actionButton : m_optionsActionButtons) {
        if (mouseX >= actionButton.x && mouseX <= actionButton.x + actionButton.width &&
            mouseY >= actionButton.y && mouseY <= actionButton.y + actionButton.height) {
            return actionButton.id;
        }
    }

    // EN: Check controls belonging to the active tab
    // ES: Chequear controles pertenecientes a la pestaña activa
    const std::vector<UIButton>* activeControlsList = nullptr;
    switch (currentTab) {
        case OptionsTab::ScreenAndVideo:        activeControlsList = &m_optionsScreenButtons; break;
        case OptionsTab::GraphicsAndImage:      activeControlsList = &m_optionsGraphicsButtons; break;
        case OptionsTab::GameplayAndSimulation: activeControlsList = &m_optionsGameplayButtons; break;
        case OptionsTab::AudioAndSound:          activeControlsList = &m_optionsAudioButtons; break;
    }

    if (activeControlsList) {
        for (const auto& optionControl : *activeControlsList) {
            if (mouseX >= optionControl.x && mouseX <= optionControl.x + optionControl.width &&
                mouseY >= optionControl.y && mouseY <= optionControl.y + optionControl.height) {
                return optionControl.id;
            }
        }
    }

    return 0;
}

int Renderer::checkMenuButtonClicked(float mouseX, float mouseY, bool inOptions) const {
    if (inOptions) {
        return checkOptionsMenuButtonClicked(mouseX, mouseY, OptionsTab::ScreenAndVideo);
    }
    for (const auto& btn : m_menuButtons) {
        if (mouseX >= btn.x && mouseX <= btn.x + btn.width &&
            mouseY >= btn.y && mouseY <= btn.y + btn.height) {
            return btn.id;
        }
    }
    return 0;
}

// EN: Initialize category tabs and navigation controls for "Learn to Play" tutorial codex
// ES: Inicializar pestañas de categorías y controles de navegación para el códice tutorial "Aprende a jugar"
void Renderer::initTutorialUI() {
    m_tutorialTabButtons.clear();
    m_tutorialButtons.clear();

    constexpr float modalWidth = 980.0f;
    constexpr float modalHeight = 600.0f;
    const float modalStartX = (static_cast<float>(m_width) - modalWidth) * 0.5f;
    const float modalStartY = (static_cast<float>(m_height) - modalHeight) * 0.5f;

    // EN: 4 Category Tabs along the upper edge of the sacred codex
    // ES: 4 Pestañas de categorías a lo largo del borde superior del códice sagrado
    constexpr float tabSpacing = 8.0f;
    const float tabWidth = (modalWidth - 60.0f - 3.0f * tabSpacing) / 4.0f;
    constexpr float tabHeight = 32.0f;
    const float tabStartY = modalStartY + 46.0f;

    m_tutorialTabButtons.push_back({2001, modalStartX + 30.0f, tabStartY, tabWidth, tabHeight, "1. FUNDAMENTOS Y FE", false});
    m_tutorialTabButtons.push_back({2002, modalStartX + 30.0f + (tabWidth + tabSpacing), tabStartY, tabWidth, tabHeight, "2. CONTROLES Y CAMARA", false});
    m_tutorialTabButtons.push_back({2003, modalStartX + 30.0f + 2.0f * (tabWidth + tabSpacing), tabStartY, tabWidth, tabHeight, "3. ALDEANOS Y SUPERVIVENCIA", false});
    m_tutorialTabButtons.push_back({2004, modalStartX + 30.0f + 3.0f * (tabWidth + tabSpacing), tabStartY, tabWidth, tabHeight, "4. PODERES Y ERAS", false});

    // EN: Bottom action buttons: Previous page (2010), Return to Menu (2011), Next page (2012)
    // ES: Botones de acción inferior: Página anterior (2010), Volver al Menú (2011), Página siguiente (2012)
    const float bottomActionY = modalStartY + modalHeight - 48.0f;
    constexpr float navButtonWidth = 160.0f;
    constexpr float centerReturnWidth = 240.0f;
    constexpr float bottomButtonHeight = 36.0f;

    m_tutorialButtons.push_back({2010, modalStartX + 35.0f, bottomActionY, navButtonWidth, bottomButtonHeight, "< ANTERIOR", false});
    m_tutorialButtons.push_back({2011, modalStartX + (modalWidth - centerReturnWidth) * 0.5f, bottomActionY, centerReturnWidth, bottomButtonHeight, "VOLVER AL MENU", false});
    m_tutorialButtons.push_back({2012, modalStartX + modalWidth - 35.0f - navButtonWidth, bottomActionY, navButtonWidth, bottomButtonHeight, "SIGUIENTE >", false});
}

// =============================================================================
// EN: Initialize Map Editor & Sacred Cartography workshop interactive controls
// ES: Inicializar controles interactivos del Taller Litúrgico de Creación de Mapas
// =============================================================================
void Renderer::initMapEditorUI() {
    m_mapEditorButtons.clear();

    // EN: Left Control Column: Procedural parameters and tool brushes
    // ES: Columna Izquierda de Controles: Parámetros generativos y pinceles de herramientas
    constexpr float leftStartColX = 38.0f;

    // 1. Procedural generation parameter buttons
    // 801 & 802: Map Size Previous / Next
    m_mapEditorButtons.push_back({801, 110.0f, 88.0f, 28.0f, 24.0f, "<", false});
    m_mapEditorButtons.push_back({802, 410.0f, 88.0f, 28.0f, 24.0f, ">", false});

    // 803 & 804: World Climate Previous / Next
    m_mapEditorButtons.push_back({803, 110.0f, 116.0f, 28.0f, 24.0f, "<", false});
    m_mapEditorButtons.push_back({804, 410.0f, 116.0f, 28.0f, 24.0f, ">", false});

    // 805 & 806: Resource Abundance Previous / Next
    m_mapEditorButtons.push_back({805, 110.0f, 144.0f, 28.0f, 24.0f, "<", false});
    m_mapEditorButtons.push_back({806, 410.0f, 144.0f, 28.0f, 24.0f, ">", false});

    // 807: Map Seed interactive input field
    m_mapEditorButtons.push_back({807, 110.0f, 172.0f, 216.0f, 24.0f, "", false});
    // 808: Dice / Randomize Seed
    m_mapEditorButtons.push_back({808, 332.0f, 172.0f, 106.0f, 24.0f, "DADO", false});

    // 810: Regenerate Procedural Map Button
    m_mapEditorButtons.push_back({810, leftStartColX, 204.0f, 400.0f, 28.0f, "REGENERAR PROCEDIMENTAL", false});

    // 2. Brush radius selection
    m_mapEditorButtons.push_back({821, leftStartColX, 262.0f, 125.0f, 24.0f, "1x1 (PUNTO)", false});
    m_mapEditorButtons.push_back({822, 175.0f, 262.0f, 125.0f, 24.0f, "3x3 (AREA)", false});
    m_mapEditorButtons.push_back({823, 312.0f, 262.0f, 126.0f, 24.0f, "5x5 (COLOSAL)", false});

    // 3. Biome brushes (3 rows x 3 columns)
    m_mapEditorButtons.push_back({831, leftStartColX, 320.0f, 125.0f, 26.0f, "OCEANO", false});
    m_mapEditorButtons.push_back({832, 175.0f, 320.0f, 125.0f, 26.0f, "SOMERA", false});
    m_mapEditorButtons.push_back({833, 312.0f, 320.0f, 126.0f, 26.0f, "VALLE", false});

    m_mapEditorButtons.push_back({834, leftStartColX, 350.0f, 125.0f, 26.0f, "BOSQUE", false});
    m_mapEditorButtons.push_back({835, 175.0f, 350.0f, 125.0f, 26.0f, "COLINAS", false});
    m_mapEditorButtons.push_back({836, 312.0f, 350.0f, 126.0f, 26.0f, "MONTANA", false});

    m_mapEditorButtons.push_back({837, leftStartColX, 380.0f, 125.0f, 26.0f, "DESIERTO", false});
    m_mapEditorButtons.push_back({838, 175.0f, 380.0f, 125.0f, 26.0f, "TUNDRA", false});
    m_mapEditorButtons.push_back({839, 312.0f, 380.0f, 126.0f, 26.0f, "VOLCAN", false});

    // 4. Mineral and resource deposits
    m_mapEditorButtons.push_back({841, leftStartColX, 442.0f, 94.0f, 25.0f, "MADERA", false});
    m_mapEditorButtons.push_back({842, 140.0f, 442.0f, 94.0f, 25.0f, "PIEDRA", false});
    m_mapEditorButtons.push_back({843, 242.0f, 442.0f, 94.0f, 25.0f, "ORO", false});
    m_mapEditorButtons.push_back({844, 344.0f, 442.0f, 94.0f, 25.0f, "HIERRO", false});

    m_mapEditorButtons.push_back({845, leftStartColX, 472.0f, 125.0f, 25.0f, "COBRE", false});
    m_mapEditorButtons.push_back({846, 175.0f, 472.0f, 125.0f, 25.0f, "DIAMANTE", false});
    m_mapEditorButtons.push_back({847, 312.0f, 472.0f, 126.0f, 25.0f, "BORRAR REC.", false});

    // 5. Elevation sculpting and inspection
    m_mapEditorButtons.push_back({851, leftStartColX, 538.0f, 195.0f, 26.0f, "ELEVAR (+)", false});
    m_mapEditorButtons.push_back({852, 243.0f, 538.0f, 195.0f, 26.0f, "BAJAR (-)", false});
    m_mapEditorButtons.push_back({855, leftStartColX, 570.0f, 400.0f, 26.0f, "INSPECCIONAR CELDA", false});

    // 6. Right Panel Bottom Action Buttons
    m_mapEditorButtons.push_back({880, 1010.0f, 62.0f, 228.0f, 24.0f, "VISTA ISOMETRICA 2.5D (TAB)", false});
    m_mapEditorButtons.push_back({890, 494.0f, 642.0f, 260.0f, 42.0f, "JUGAR ESTE MAPA", false});
    m_mapEditorButtons.push_back({891, 768.0f, 642.0f, 240.0f, 42.0f, "GUARDAR MAPA", false});
    m_mapEditorButtons.push_back({892, 1022.0f, 642.0f, 216.0f, 42.0f, "VOLVER AL MENU", false});
}

// EN: Initialize interactive 2.5D Isometric Map Editor toolbar, category tabs, and action buttons
// ES: Inicializar barra de herramientas, pestañas de categoría y botones de acción del Editor Isométrico 2.5D
void Renderer::initMapEditorIsometricUI() {
    m_mapEditorIsoButtons.clear();

    // EN: Top bar controls (X = 14 to 1266, Y = 14, H = 28)
    // ES: Controles de la barra superior (X = 14 a 1266, Y = 14, H = 28)
    m_mapEditorIsoButtons.push_back({880, 560.0f, 14.0f, 200.0f, 28.0f, "VISTA 2D (TAB)", false});
    m_mapEditorIsoButtons.push_back({808, 770.0f, 14.0f, 64.0f, 28.0f, "DADO", false});
    m_mapEditorIsoButtons.push_back({810, 842.0f, 14.0f, 96.0f, 28.0f, "REGENERAR", false});
    m_mapEditorIsoButtons.push_back({891, 946.0f, 14.0f, 110.0f, 28.0f, "GUARDAR", false});
    m_mapEditorIsoButtons.push_back({890, 1064.0f, 14.0f, 100.0f, 28.0f, "JUGAR MAPA", false});
    m_mapEditorIsoButtons.push_back({892, 1172.0f, 14.0f, 94.0f, 28.0f, "SALIR", false});

    // EN: Bottom toolbar altar tabs (Y = 612, H = 24)
    // ES: Pestañas del altar de barra de herramientas inferior (Y = 612, H = 24)
    m_mapEditorIsoButtons.push_back({871, 24.0f, 612.0f, 190.0f, 24.0f, "1. MOSAICOS ATLAS (16)", false});
    m_mapEditorIsoButtons.push_back({872, 222.0f, 612.0f, 190.0f, 24.0f, "2. RECURSOS Y FAUNA", false});
    m_mapEditorIsoButtons.push_back({873, 420.0f, 612.0f, 190.0f, 24.0f, "3. RELIEVE Y BIOMAS", false});

    // EN: Brush radius selection (Y = 612, H = 24)
    // ES: Selección de radio del pincel (Y = 612, H = 24)
    m_mapEditorIsoButtons.push_back({821, 1060.0f, 612.0f, 60.0f, 24.0f, "1x1", false});
    m_mapEditorIsoButtons.push_back({822, 1128.0f, 612.0f, 60.0f, 24.0f, "3x3", false});
    m_mapEditorIsoButtons.push_back({823, 1196.0f, 612.0f, 60.0f, 24.0f, "5x5", false});

    // EN: Tab 0: All 16 Atlas terrain sprites (8 per row x 2 rows)
    // ES: Pestaña 0: Los 16 mosaicos de terreno del atlas (8 por fila x 2 filas)
    constexpr float tileBtnW = 148.0f;
    constexpr float tileSpacingX = 8.0f;
    constexpr float startColX = 24.0f;

    m_mapEditorIsoButtons.push_back({900, startColX + 0 * (tileBtnW + tileSpacingX), 642.0f, tileBtnW, 26.0f, "PASTO (0)", false});
    m_mapEditorIsoButtons.push_back({901, startColX + 1 * (tileBtnW + tileSpacingX), 642.0f, tileBtnW, 26.0f, "ROCA (1)", false});
    m_mapEditorIsoButtons.push_back({902, startColX + 2 * (tileBtnW + tileSpacingX), 642.0f, tileBtnW, 26.0f, "PRADERA (2)", false});
    m_mapEditorIsoButtons.push_back({903, startColX + 3 * (tileBtnW + tileSpacingX), 642.0f, tileBtnW, 26.0f, "TIERRA (3)", false});
    m_mapEditorIsoButtons.push_back({904, startColX + 4 * (tileBtnW + tileSpacingX), 642.0f, tileBtnW, 26.0f, "OC. AZUL (4)", false});
    m_mapEditorIsoButtons.push_back({905, startColX + 5 * (tileBtnW + tileSpacingX), 642.0f, tileBtnW, 26.0f, "BOSQUE (5)", false});
    m_mapEditorIsoButtons.push_back({906, startColX + 6 * (tileBtnW + tileSpacingX), 642.0f, tileBtnW, 26.0f, "COLINA (6)", false});
    m_mapEditorIsoButtons.push_back({907, startColX + 7 * (tileBtnW + tileSpacingX), 642.0f, tileBtnW, 26.0f, "PANTANO (7)", false});

    m_mapEditorIsoButtons.push_back({908, startColX + 0 * (tileBtnW + tileSpacingX), 674.0f, tileBtnW, 26.0f, "ARENA (8)", false});
    m_mapEditorIsoButtons.push_back({909, startColX + 1 * (tileBtnW + tileSpacingX), 674.0f, tileBtnW, 26.0f, "NIEVE (9)", false});
    m_mapEditorIsoButtons.push_back({910, startColX + 2 * (tileBtnW + tileSpacingX), 674.0f, tileBtnW, 26.0f, "SOMERA (10)", false});
    m_mapEditorIsoButtons.push_back({911, startColX + 3 * (tileBtnW + tileSpacingX), 674.0f, tileBtnW, 26.0f, "MAR (11)", false});
    m_mapEditorIsoButtons.push_back({912, startColX + 4 * (tileBtnW + tileSpacingX), 674.0f, tileBtnW, 26.0f, "CUEVA (12)", false});
    m_mapEditorIsoButtons.push_back({913, startColX + 5 * (tileBtnW + tileSpacingX), 674.0f, tileBtnW, 26.0f, "ABISAL (13)", false});
    m_mapEditorIsoButtons.push_back({914, startColX + 6 * (tileBtnW + tileSpacingX), 674.0f, tileBtnW, 26.0f, "OCEANO (14)", false});
    m_mapEditorIsoButtons.push_back({915, startColX + 7 * (tileBtnW + tileSpacingX), 674.0f, tileBtnW, 26.0f, "LAVA (15)", false});

    // EN: Tab 1: Natural Resources, flora and geological props
    // ES: Pestaña 1: Recursos naturales, flora y elementos geológicos
    constexpr float resBtnW = 118.0f;
    constexpr float resSpacingX = 6.0f;
    m_mapEditorIsoButtons.push_back({841, startColX + 0 * (resBtnW + resSpacingX), 650.0f, resBtnW, 36.0f, "MADERA", false});
    m_mapEditorIsoButtons.push_back({842, startColX + 1 * (resBtnW + resSpacingX), 650.0f, resBtnW, 36.0f, "PIEDRA", false});
    m_mapEditorIsoButtons.push_back({843, startColX + 2 * (resBtnW + resSpacingX), 650.0f, resBtnW, 36.0f, "ORO", false});
    m_mapEditorIsoButtons.push_back({844, startColX + 3 * (resBtnW + resSpacingX), 650.0f, resBtnW, 36.0f, "HIERRO", false});
    m_mapEditorIsoButtons.push_back({845, startColX + 4 * (resBtnW + resSpacingX), 650.0f, resBtnW, 36.0f, "COBRE", false});
    m_mapEditorIsoButtons.push_back({846, startColX + 5 * (resBtnW + resSpacingX), 650.0f, resBtnW, 36.0f, "DIAMANTE", false});
    m_mapEditorIsoButtons.push_back({839, startColX + 6 * (resBtnW + resSpacingX), 650.0f, resBtnW, 36.0f, "VOLCAN", false});
    m_mapEditorIsoButtons.push_back({920, startColX + 7 * (resBtnW + resSpacingX), 650.0f, resBtnW, 36.0f, "CRATER", false});
    m_mapEditorIsoButtons.push_back({921, startColX + 8 * (resBtnW + resSpacingX), 650.0f, resBtnW, 36.0f, "CENIZA", false});
    m_mapEditorIsoButtons.push_back({847, startColX + 9 * (resBtnW + resSpacingX), 650.0f, resBtnW, 36.0f, "BORRAR REC.", false});

    // EN: Tab 2: Topography & Biomes
    // ES: Pestaña 2: Topografía y Biomas
    constexpr float topoBtnW = 118.0f;
    constexpr float topoSpacingX = 6.0f;
    m_mapEditorIsoButtons.push_back({851, startColX + 0 * (topoBtnW + topoSpacingX), 642.0f, topoBtnW, 26.0f, "ELEVAR (+)", false});
    m_mapEditorIsoButtons.push_back({852, startColX + 1 * (topoBtnW + topoSpacingX), 642.0f, topoBtnW, 26.0f, "BAJAR (-)", false});
    m_mapEditorIsoButtons.push_back({853, startColX + 2 * (topoBtnW + topoSpacingX), 642.0f, topoBtnW, 26.0f, "APLANAR (=)", false});
    m_mapEditorIsoButtons.push_back({855, startColX + 3 * (topoBtnW + topoSpacingX), 642.0f, topoBtnW, 26.0f, "INSPECCIONAR", false});
    m_mapEditorIsoButtons.push_back({831, startColX + 4 * (topoBtnW + topoSpacingX), 642.0f, topoBtnW, 26.0f, "OCEANO", false});
    m_mapEditorIsoButtons.push_back({832, startColX + 5 * (topoBtnW + topoSpacingX), 642.0f, topoBtnW, 26.0f, "SOMERA", false});
    m_mapEditorIsoButtons.push_back({833, startColX + 6 * (topoBtnW + topoSpacingX), 642.0f, topoBtnW, 26.0f, "VALLE", false});
    m_mapEditorIsoButtons.push_back({834, startColX + 7 * (topoBtnW + topoSpacingX), 642.0f, topoBtnW, 26.0f, "BOSQUE", false});
    m_mapEditorIsoButtons.push_back({835, startColX + 8 * (topoBtnW + topoSpacingX), 642.0f, topoBtnW, 26.0f, "COLINAS", false});
    m_mapEditorIsoButtons.push_back({836, startColX + 9 * (topoBtnW + topoSpacingX), 642.0f, topoBtnW, 26.0f, "MONTANA", false});
    m_mapEditorIsoButtons.push_back({837, startColX + 0 * (topoBtnW + topoSpacingX), 674.0f, topoBtnW, 26.0f, "DESIERTO", false});
    m_mapEditorIsoButtons.push_back({838, startColX + 1 * (topoBtnW + topoSpacingX), 674.0f, topoBtnW, 26.0f, "TUNDRA", false});
}

// EN: Check if cursor clicked any 2.5D Isometric Map Editor button
// ES: Verificar si el cursor hizo clic en algún botón del Editor Isométrico 2.5D
int Renderer::checkMapEditorIsometricButtonClicked(float mouseX, float mouseY) const {
    for (const auto& btn : m_mapEditorIsoButtons) {
        if (mouseX >= btn.x && mouseX <= btn.x + btn.width &&
            mouseY >= btn.y && mouseY <= btn.y + btn.height) {
            return btn.id;
        }
    }
    return 0;
}

// EN: Check if cursor clicked any Map Editor button
// ES: Verificar si el cursor hizo clic en algún botón del Editor de Mapas
int Renderer::checkMapEditorButtonClicked(float mouseX, float mouseY) const {
    for (const auto& btn : m_mapEditorButtons) {
        if (mouseX >= btn.x && mouseX <= btn.x + btn.width &&
            mouseY >= btn.y && mouseY <= btn.y + btn.height) {
            return btn.id;
        }
    }
    return 0;
}

// EN: Convert screen cursor coordinates into Map Editor grid cell
// ES: Convertir coordenadas del cursor en pantalla a celda de la cuadrícula del Editor de Mapas
Point2i Renderer::screenToMapEditorCell(float mouseX, float mouseY, const WorldMap& editorMap) const {
    if (editorMap.getWidth() <= 0 || editorMap.getHeight() <= 0) {
        return {-1, -1};
    }
    constexpr float canvasX = 494.0f;
    constexpr float canvasY = 92.0f;
    constexpr float canvasW = 744.0f;
    constexpr float canvasH = 470.0f;

    const float scale = std::min(canvasW / static_cast<float>(editorMap.getWidth()),
                                 canvasH / static_cast<float>(editorMap.getHeight()));
    const float previewW = static_cast<float>(editorMap.getWidth()) * scale;
    const float previewH = static_cast<float>(editorMap.getHeight()) * scale;
    const float previewStartX = canvasX + (canvasW - previewW) * 0.5f;
    const float previewStartY = canvasY + (canvasH - previewH) * 0.5f;

    if (mouseX >= previewStartX && mouseX < previewStartX + previewW &&
        mouseY >= previewStartY && mouseY < previewStartY + previewH) {
        const int cellX = static_cast<int>((mouseX - previewStartX) / scale);
        const int cellY = static_cast<int>((mouseY - previewStartY) / scale);
        if (editorMap.isValid(cellX, cellY)) {
            return {cellX, cellY};
        }
    }
    return {-1, -1};
}

// EN: Check if mouse cursor is within the live preview cartographic canvas
// ES: Verificar si el cursor del ratón está dentro del lienzo cartográfico de vista previa
bool Renderer::isInsideMapEditorPreview(float mouseX, float mouseY, const WorldMap& editorMap) const {
    const Point2i cell = screenToMapEditorCell(mouseX, mouseY, editorMap);
    return cell.x >= 0 && cell.y >= 0;
}

// EN: Check if cursor clicked any tutorial category tab or action button
// ES: Verificar si el cursor hizo clic en alguna pestaña de categoría o botón de acción del tutorial
int Renderer::checkTutorialButtonClicked(float mouseX, float mouseY) const {
    for (const auto& tabButton : m_tutorialTabButtons) {
        if (mouseX >= tabButton.x && mouseX <= tabButton.x + tabButton.width &&
            mouseY >= tabButton.y && mouseY <= tabButton.y + tabButton.height) {
            return tabButton.id;
        }
    }
    for (const auto& actionButton : m_tutorialButtons) {
        if (mouseX >= actionButton.x && mouseX <= actionButton.x + actionButton.width &&
            mouseY >= actionButton.y && mouseY <= actionButton.y + actionButton.height) {
            return actionButton.id;
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
    constexpr float btnH = 44.0f;
    constexpr float btnW = 192.0f;
    constexpr float gap = 14.0f;

    // EN: ID 904: OBSERVAR MUNDO / VER MAPA
    // ES: ID 904: OBSERVAR MUNDO / VER MAPA
    const float xInspect = mx + 25.0f;
    if (mouseX >= xInspect && mouseX <= xInspect + btnW &&
        mouseY >= btnY && mouseY <= btnY + btnH) {
        return 904;
    }

    // EN: ID 901: REINTENTAR GÉNESIS
    // ES: ID 901: REINTENTAR GÉNESIS
    const float xRetry = xInspect + btnW + gap;
    if (mouseX >= xRetry && mouseX <= xRetry + btnW &&
        mouseY >= btnY && mouseY <= btnY + btnH) {
        return 901;
    }

    // EN: ID 902: CARGAR PARTIDA
    // ES: ID 902: CARGAR PARTIDA
    const float xLoad = xRetry + btnW + gap;
    if (mouseX >= xLoad && mouseX <= xLoad + btnW &&
        mouseY >= btnY && mouseY <= btnY + btnH) {
        return 902;
    }

    // EN: ID 903: MENÚ PRINCIPAL
    // ES: ID 903: MENÚ PRINCIPAL
    const float xMenu = xLoad + btnW + gap;
    if (mouseX >= xMenu && mouseX <= xMenu + btnW &&
        mouseY >= btnY && mouseY <= btnY + btnH) {
        return 903;
    }

    return 0;
}

int Renderer::checkSpectatorButtonClicked(float mouseX, float mouseY) const {
    constexpr float topH = 46.0f;
    if (mouseY > topH) return 0;

    constexpr float btnH = 30.0f;
    constexpr float btnY = 8.0f;
    if (mouseY < btnY || mouseY > btnY + btnH) return 0;

    float curRightX = static_cast<float>(m_width) - 12.0f;

    // EN: Button 903: MENU
    // ES: Botón 903: MENU
    constexpr float wMenu = 85.0f;
    curRightX -= wMenu;
    if (mouseX >= curRightX && mouseX <= curRightX + wMenu) return 903;

    // EN: Button 901: REINTENTAR
    // ES: Botón 901: REINTENTAR
    constexpr float wRetry = 120.0f;
    curRightX -= (wRetry + 8.0f);
    if (mouseX >= curRightX && mouseX <= curRightX + wRetry) return 901;

    // EN: Button 906: NIEBLA
    // ES: Botón 906: NIEBLA
    constexpr float wFog = 155.0f;
    curRightX -= (wFog + 8.0f);
    if (mouseX >= curRightX && mouseX <= curRightX + wFog) return 906;

    // EN: Button 905: VER EPITAFIO
    // ES: Botón 905: VER EPITAFIO
    constexpr float wEpi = 180.0f;
    curRightX -= (wEpi + 8.0f);
    if (mouseX >= curRightX && mouseX <= curRightX + wEpi) return 905;

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
        // EN: Round play button (ID 202) uses Euclidean circular hit testing
        // ES: El botón redondo de play (ID 202) utiliza detección de impactos circular euclidiana
        if (btn.id == 202) {
            float radius = btn.width * 0.5f;
            float centerX = btn.x + radius;
            float centerY = btn.y + radius;
            float deltaX = mouseX - centerX;
            float deltaY = mouseY - centerY;
            if (deltaX * deltaX + deltaY * deltaY <= radius * radius) {
                return btn.id;
            }
            continue;
        }

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

// EN: Draw round play button with centered play symbol, metallic circular bezel and mechanical depression
// ES: Dibujar botón redondo de play con símbolo de play centrado, bisel circular metálico y hundimiento mecánico
void Renderer::drawRoundPlayButton(int bx, int by, int diameter, bool active, bool hovered, bool pressed) {
    if (diameter <= 0) return;

    const float radius = static_cast<float>(diameter) * 0.5f;
    const float centerBaseX = static_cast<float>(bx) + radius;
    const float centerBaseY = static_cast<float>(by) + radius;

    // EN: Pressed physical offset (disc pushes inward-down by 1.5px)
    // ES: Desplazamiento físico al pulsar (el disco se hunde hacia adentro y abajo 1.5px)
    const float pressOffset = pressed ? 1.5f : 0.0f;
    const float discCenterX = centerBaseX + pressOffset;
    const float discCenterY = centerBaseY + pressOffset;

    const int minPixelX = bx - 1;
    const int maxPixelX = bx + diameter + 1;
    const int minPixelY = by - 1;
    const int maxPixelY = by + diameter + 1;

    // EN: 1. Render circular button body with concentric bezel and depth lighting
    // ES: 1. Renderizar el cuerpo circular del botón con bisel concéntrico e iluminación de profundidad
    for (int pixelY = minPixelY; pixelY <= maxPixelY; ++pixelY) {
        int frameY = m_height - 1 - pixelY;
        if (frameY < 0 || frameY >= m_height) continue;
        int fbRowOffset = frameY * m_width;

        for (int pixelX = minPixelX; pixelX <= maxPixelX; ++pixelX) {
            if (pixelX < 0 || pixelX >= m_width) continue;

            float deltaX = (static_cast<float>(pixelX) + 0.5f) - centerBaseX;
            float deltaY = (static_cast<float>(pixelY) + 0.5f) - centerBaseY;
            float distFromCenter = std::sqrt(deltaX * deltaX + deltaY * deltaY);

            // EN: Discard pixels outside circular bounding boundary plus outer rim shadow
            // ES: Descartar píxeles fuera del límite circular más la sombra del borde exterior
            if (distFromCenter > radius + 1.2f) continue;

            // EN: Outer drop shadow just below the circular button
            // ES: Sombra proyectada exterior justo debajo del botón circular
            if (distFromCenter > radius) {
                if (deltaY > 0.0f || deltaX > 0.0f) {
                    int fbIndex = fbRowOffset + pixelX;
                    uint32_t currentPixel = m_framebuffer[fbIndex];
                    uint8_t currR = static_cast<uint8_t>(currentPixel & 0xFF);
                    uint8_t currG = static_cast<uint8_t>((currentPixel >> 8) & 0xFF);
                    uint8_t currB = static_cast<uint8_t>((currentPixel >> 16) & 0xFF);
                    uint8_t blendR = static_cast<uint8_t>((currR * 120) / 255);
                    uint8_t blendG = static_cast<uint8_t>((currG * 120) / 255);
                    uint8_t blendB = static_cast<uint8_t>((currB * 120) / 255);
                    m_framebuffer[fbIndex] = (0xFFu << 24) |
                                            (static_cast<uint32_t>(blendB) << 16) |
                                            (static_cast<uint32_t>(blendG) << 8) |
                                            static_cast<uint32_t>(blendR);
                }
                continue;
            }

            ColorRGB pixelColor{32, 22, 16};

            // EN: Circular bezel ring (outer metallic frame)
            // ES: Anillo biselado circular (marco exterior metálico)
            constexpr float bezelThickness = 3.2f;
            if (distFromCenter >= radius - bezelThickness) {
                float normX = deltaX / (distFromCenter > 0.001f ? distFromCenter : 1.0f);
                float normY = deltaY / (distFromCenter > 0.001f ? distFromCenter : 1.0f);
                // Directional lighting from top-left (-0.707, -0.707)
                float lightFactor = (-normX - normY) * 0.7071f; // [-1.0, 1.0]

                if (pressed) {
                    // EN: Inverted bezel lighting when pressed (sunk into socket)
                    // ES: Iluminación de bisel invertida al pulsar (hundido en su hueco)
                    if (lightFactor > 0.1f) {
                        pixelColor = ColorRGB{14, 9, 6}; // Deep socket shadow at top-left
                    } else {
                        pixelColor = ColorRGB{65, 48, 30}; // Bottom-right reflection
                    }
                } else if (active) {
                    // EN: Radiant golden liturgical bezel when active
                    // ES: Bisel litúrgico dorado radiante cuando está activo
                    if (lightFactor > 0.2f) {
                        pixelColor = ColorRGB{255, 242, 145}; // Intense gold highlight
                    } else if (lightFactor > -0.5f) {
                        pixelColor = Palette::DIVINE_GOLD; // #E6A13B
                    } else {
                        pixelColor = ColorRGB{175, 122, 42}; // Warm gold shadow
                    }
                } else if (hovered) {
                    // EN: Hover warm golden illumination
                    // ES: Iluminación dorada cálida al pasar el cursor
                    if (lightFactor > 0.3f) {
                        pixelColor = ColorRGB{245, 205, 105};
                    } else if (lightFactor > -0.2f) {
                        pixelColor = ColorRGB{185, 140, 65};
                    } else {
                        pixelColor = ColorRGB{75, 52, 28};
                    }
                } else {
                    // EN: Idle vintage liturgical bronze bezel
                    // ES: Bisel de bronce litúrgico vintage en reposo
                    if (lightFactor > 0.3f) {
                        pixelColor = ColorRGB{165, 125, 65};
                    } else if (lightFactor > -0.2f) {
                        pixelColor = ColorRGB{115, 85, 45};
                    } else {
                        pixelColor = ColorRGB{45, 30, 18};
                    }
                }
            } else if (distFromCenter >= radius - bezelThickness - 1.0f) {
                // EN: Dark separator seam between bezel and inner disc face
                // ES: Ranura oscura separadora entre el bisel y la cara interior del disco
                pixelColor = ColorRGB{16, 11, 8};
            } else {
                // EN: Inner disc face with radial gradient
                // ES: Cara interior del disco con degradado radial
                float discDeltaX = (static_cast<float>(pixelX) + 0.5f) - discCenterX;
                float discDeltaY = (static_cast<float>(pixelY) + 0.5f) - discCenterY;
                float discDist = std::sqrt(discDeltaX * discDeltaX + discDeltaY * discDeltaY);
                float discGrad = std::clamp(discDist / (radius - bezelThickness - 1.0f), 0.0f, 1.0f);

                if (pressed) {
                    pixelColor = ColorRGB{
                        static_cast<uint8_t>(22 + static_cast<int>(6 * discGrad)),
                        static_cast<uint8_t>(15 + static_cast<int>(4 * discGrad)),
                        static_cast<uint8_t>(11 + static_cast<int>(3 * discGrad))
                    };
                } else if (active) {
                    pixelColor = ColorRGB{
                        static_cast<uint8_t>(75 - static_cast<int>(25 * discGrad)),
                        static_cast<uint8_t>(52 - static_cast<int>(18 * discGrad)),
                        static_cast<uint8_t>(28 - static_cast<int>(10 * discGrad))
                    };
                } else if (hovered) {
                    pixelColor = ColorRGB{
                        static_cast<uint8_t>(58 - static_cast<int>(18 * discGrad)),
                        static_cast<uint8_t>(42 - static_cast<int>(14 * discGrad)),
                        static_cast<uint8_t>(30 - static_cast<int>(10 * discGrad))
                    };
                } else {
                    pixelColor = ColorRGB{
                        static_cast<uint8_t>(44 - static_cast<int>(16 * discGrad)),
                        static_cast<uint8_t>(32 - static_cast<int>(12 * discGrad)),
                        static_cast<uint8_t>(22 - static_cast<int>(8 * discGrad))
                    };
                }
            }

            int fbIndex = fbRowOffset + pixelX;
            m_framebuffer[fbIndex] = (0xFFu << 24) |
                                    (static_cast<uint32_t>(pixelColor.b) << 16) |
                                    (static_cast<uint32_t>(pixelColor.g) << 8) |
                                    static_cast<uint32_t>(pixelColor.r);
        }
    }

    // EN: 2. Center Play Symbol (▶ mathematically and visually centered sharp equilateral triangle)
    // ES: 2. Símbolo de Play central (▶ triángulo equilátero afilado centrado matemática y visualmente)
    const int triangleCenterPixelX = static_cast<int>(discCenterX);
    const int triangleCenterPixelY = static_cast<int>(discCenterY);
    constexpr int triangleHalfHeight = 5;
    constexpr int triangleTotalWidth = 9;
    const int triangleBaseLeftX = triangleCenterPixelX - 4;

    ColorRGB playSymbolColor = Palette::RELIC_BONE;
    if (active) {
        playSymbolColor = ColorRGB{255, 235, 120}; // Radiant golden glyph
    } else if (hovered) {
        playSymbolColor = ColorRGB{255, 245, 185};
    } else if (pressed) {
        playSymbolColor = ColorRGB{195, 185, 160};
    }

    // EN: Render 1px drop shadow for 3D depth if unpressed
    // ES: Renderizar sombra proyectada de 1px para profundidad 3D si no está pulsado
    if (!pressed) {
        ColorRGB shadowColor{12, 8, 6};
        for (int deltaY = -triangleHalfHeight; deltaY <= triangleHalfHeight; ++deltaY) {
            float rowRatio = 1.0f - static_cast<float>(std::abs(deltaY)) / static_cast<float>(triangleHalfHeight + 1);
            int rowWidth = std::max(1, static_cast<int>(rowRatio * static_cast<float>(triangleTotalWidth)));
            int targetPixelY = triangleCenterPixelY + deltaY + 1;
            int frameY = m_height - 1 - targetPixelY;
            if (frameY < 0 || frameY >= m_height) continue;
            int fbRowOffset = frameY * m_width;

            for (int deltaX = 0; deltaX < rowWidth; ++deltaX) {
                int targetPixelX = triangleBaseLeftX + deltaX + 1;
                if (targetPixelX < 0 || targetPixelX >= m_width) continue;
                int fbIndex = fbRowOffset + targetPixelX;
                m_framebuffer[fbIndex] = (0xFFu << 24) |
                                        (static_cast<uint32_t>(shadowColor.b) << 16) |
                                        (static_cast<uint32_t>(shadowColor.g) << 8) |
                                        static_cast<uint32_t>(shadowColor.r);
            }
        }
    }

    // EN: Render main play triangle glyph
    // ES: Renderizar el glifo triangular principal de play
    for (int deltaY = -triangleHalfHeight; deltaY <= triangleHalfHeight; ++deltaY) {
        float rowRatio = 1.0f - static_cast<float>(std::abs(deltaY)) / static_cast<float>(triangleHalfHeight + 1);
        int rowWidth = std::max(1, static_cast<int>(rowRatio * static_cast<float>(triangleTotalWidth)));
        int targetPixelY = triangleCenterPixelY + deltaY;
        int frameY = m_height - 1 - targetPixelY;
        if (frameY < 0 || frameY >= m_height) continue;
        int fbRowOffset = frameY * m_width;

        for (int deltaX = 0; deltaX < rowWidth; ++deltaX) {
            int targetPixelX = triangleBaseLeftX + deltaX;
            if (targetPixelX < 0 || targetPixelX >= m_width) continue;
            int fbIndex = fbRowOffset + targetPixelX;

            ColorRGB pixelGlyphColor = playSymbolColor;
            // Tip highlight when active
            if (active && deltaX >= rowWidth - 2) {
                pixelGlyphColor = ColorRGB{255, 252, 210};
            }
            m_framebuffer[fbIndex] = (0xFFu << 24) |
                                    (static_cast<uint32_t>(pixelGlyphColor.b) << 16) |
                                    (static_cast<uint32_t>(pixelGlyphColor.g) << 8) |
                                    static_cast<uint32_t>(pixelGlyphColor.r);
        }
    }
}

// EN: Draw vintage cassette mechanical tape deck button that depresses and springs back
// ES: Dibujar botón mecánico de casetera vintage que se hunde al pulsar y regresa a su lugar
void Renderer::drawCassetteButton(int bx, int by, int bw, int bh, const std::string& label,
                                  bool active, bool hovered, bool pressed, int btnId) {
    (void)label;
    if (bw <= 0 || bh <= 0) return;

    // EN: Mechanical vertical travel displacement when pressed (4px sunk down)
    // ES: Desplazamiento mecánico vertical al pulsar (4px hundido hacia abajo)
    const int pressTravel = pressed ? 4 : 0;
    const int totalLandingHeight = bh - 6;
    const int landingStartY = by + pressTravel;
    const int landingEndY = by + totalLandingHeight;
    const int frontLipHeight = pressed ? 1 : 5;

    // EN: 1. If pressed, render the dark exposed chassis socket cavity above the sunken key
    // ES: 1. Si está pulsado, renderizar la cavidad oscura del zócalo del chasis expuesta sobre la tecla hundida
    if (pressTravel > 0) {
        for (int pixelY = by; pixelY < landingStartY; ++pixelY) {
            int frameY = m_height - 1 - pixelY;
            if (frameY < 0 || frameY >= m_height) continue;
            int fbRowOffset = frameY * m_width;
            for (int pixelX = bx; pixelX < bx + bw; ++pixelX) {
                if (pixelX < 0 || pixelX >= m_width) continue;
                ColorRGB slotColor{8, 5, 4};
                if (pixelY == by) slotColor = ColorRGB{5, 3, 2}; // Deep upper shadow
                int fbIndex = fbRowOffset + pixelX;
                m_framebuffer[fbIndex] = (0xFFu << 24) |
                                        (static_cast<uint32_t>(slotColor.b) << 16) |
                                        (static_cast<uint32_t>(slotColor.g) << 8) |
                                        static_cast<uint32_t>(slotColor.r);
            }
        }
    }

    // EN: 2. Render top landing surface (where finger rests on the keycap)
    // ES: 2. Renderizar la superficie superior de apoyo (donde se apoya el dedo en la tecla)
    for (int pixelY = landingStartY; pixelY < landingEndY; ++pixelY) {
        int frameY = m_height - 1 - pixelY;
        if (frameY < 0 || frameY >= m_height) continue;
        int fbRowOffset = frameY * m_width;

        float vertRatio = static_cast<float>(pixelY - landingStartY) / static_cast<float>(landingEndY - landingStartY);

        for (int pixelX = bx; pixelX < bx + bw; ++pixelX) {
            if (pixelX < 0 || pixelX >= m_width) continue;

            ColorRGB keySurfaceColor;

            // EN: Mechanical top edge bevel highlight
            // ES: Bisel superior reflectivo mecánico
            if (pixelY == landingStartY) {
                if (pressed) {
                    keySurfaceColor = ColorRGB{42, 30, 22};
                } else if (active) {
                    keySurfaceColor = ColorRGB{235, 185, 75}; // Divine Gold rim
                } else if (hovered) {
                    keySurfaceColor = ColorRGB{145, 110, 75};
                } else {
                    keySurfaceColor = ColorRGB{95, 72, 50};
                }
            } else if (pixelX == bx) {
                // EN: Left bevel highlight
                // ES: Bisel lateral izquierdo
                keySurfaceColor = active ? Palette::DIVINE_GOLD : (hovered ? ColorRGB{90, 68, 48} : ColorRGB{65, 48, 35});
            } else if (pixelX == bx + bw - 1) {
                // EN: Right bevel shadow
                // ES: Bisel lateral derecho en sombra
                keySurfaceColor = ColorRGB{20, 14, 10};
            } else {
                // EN: Body texture gradient
                // ES: Degradado de la superficie del cuerpo
                if (pressed) {
                    uint8_t r = static_cast<uint8_t>(26 - static_cast<int>(6 * vertRatio));
                    uint8_t g = static_cast<uint8_t>(18 - static_cast<int>(4 * vertRatio));
                    uint8_t b = static_cast<uint8_t>(13 - static_cast<int>(3 * vertRatio));
                    keySurfaceColor = ColorRGB{r, g, b};
                } else if (active) {
                    uint8_t r = static_cast<uint8_t>(52 - static_cast<int>(12 * vertRatio));
                    uint8_t g = static_cast<uint8_t>(38 - static_cast<int>(8 * vertRatio));
                    uint8_t b = static_cast<uint8_t>(24 - static_cast<int>(6 * vertRatio));
                    keySurfaceColor = ColorRGB{r, g, b};
                } else if (hovered) {
                    uint8_t r = static_cast<uint8_t>(62 - static_cast<int>(16 * vertRatio));
                    uint8_t g = static_cast<uint8_t>(46 - static_cast<int>(12 * vertRatio));
                    uint8_t b = static_cast<uint8_t>(34 - static_cast<int>(8 * vertRatio));
                    keySurfaceColor = ColorRGB{r, g, b};
                } else {
                    uint8_t r = static_cast<uint8_t>(46 - static_cast<int>(12 * vertRatio));
                    uint8_t g = static_cast<uint8_t>(34 - static_cast<int>(8 * vertRatio));
                    uint8_t b = static_cast<uint8_t>(24 - static_cast<int>(6 * vertRatio));
                    keySurfaceColor = ColorRGB{r, g, b};
                }

                // EN: Vintage cassette keycap grip grooves on left edge
                // ES: Ranuras de agarre vintage de la tecla en el extremo izquierdo
                if ((pixelX == bx + 4 || pixelX == bx + 7) && pixelY >= landingStartY + 2 && pixelY <= landingEndY - 3) {
                    keySurfaceColor = ColorRGB{22, 15, 10}; // 1px groove
                }
            }

            int fbIndex = fbRowOffset + pixelX;
            m_framebuffer[fbIndex] = (0xFFu << 24) |
                                    (static_cast<uint32_t>(keySurfaceColor.b) << 16) |
                                    (static_cast<uint32_t>(keySurfaceColor.g) << 8) |
                                    static_cast<uint32_t>(keySurfaceColor.r);
        }
    }

    // EN: 3. Front skirt lip (vertical mechanical face that overhangs downwards)
    // ES: 3. Faldón frontal (cara mecánica vertical que sobresale hacia abajo)
    const int lipStartY = landingEndY;
    const int lipEndY = std::min(by + bh, lipStartY + frontLipHeight);
    for (int pixelY = lipStartY; pixelY < lipEndY; ++pixelY) {
        int frameY = m_height - 1 - pixelY;
        if (frameY < 0 || frameY >= m_height) continue;
        int fbRowOffset = frameY * m_width;

        float lipRatio = static_cast<float>(pixelY - lipStartY) / static_cast<float>(std::max(1, lipEndY - lipStartY));

        for (int pixelX = bx; pixelX < bx + bw; ++pixelX) {
            if (pixelX < 0 || pixelX >= m_width) continue;

            // EN: Chamfer bottom corners
            // ES: Achaflanar esquinas inferiores
            if (pixelY == lipEndY - 1 && (pixelX == bx || pixelX == bx + bw - 1)) continue;

            ColorRGB lipColor;
            if (pixelX == bx) {
                lipColor = ColorRGB{45, 32, 22};
            } else if (pixelX == bx + bw - 1) {
                lipColor = ColorRGB{14, 9, 6};
            } else {
                uint8_t r = static_cast<uint8_t>(26 - static_cast<int>(10 * lipRatio));
                uint8_t g = static_cast<uint8_t>(18 - static_cast<int>(8 * lipRatio));
                uint8_t b = static_cast<uint8_t>(13 - static_cast<int>(6 * lipRatio));
                lipColor = ColorRGB{r, g, b};
            }

            int fbIndex = fbRowOffset + pixelX;
            m_framebuffer[fbIndex] = (0xFFu << 24) |
                                    (static_cast<uint32_t>(lipColor.b) << 16) |
                                    (static_cast<uint32_t>(lipColor.g) << 8) |
                                    static_cast<uint32_t>(lipColor.r);
        }
    }

    // EN: 4. Drop shadow below key into cassette chassis bed
    // ES: 4. Sombra proyectada debajo de la tecla sobre el fondo del chasis
    if (!pressed && lipEndY < by + bh) {
        int shadowY = lipEndY;
        int frameY = m_height - 1 - shadowY;
        if (frameY >= 0 && frameY < m_height) {
            int fbRowOffset = frameY * m_width;
            for (int pixelX = bx + 1; pixelX < bx + bw - 1; ++pixelX) {
                if (pixelX < 0 || pixelX >= m_width) continue;
                int fbIndex = fbRowOffset + pixelX;
                ColorRGB shadowColor{8, 5, 4};
                m_framebuffer[fbIndex] = (0xFFu << 24) |
                                        (static_cast<uint32_t>(shadowColor.b) << 16) |
                                        (static_cast<uint32_t>(shadowColor.g) << 8) |
                                        static_cast<uint32_t>(shadowColor.r);
            }
        }
    }

    // EN: 5. Active state jewel indicator (vintage tape deck status LED on upper-left)
    // ES: 5. Indicador joya de estado activo (LED de estado de casetera vintage en esquina superior izquierda)
    if (active) {
        constexpr int jewelWidth = 6;
        constexpr int jewelHeight = 3;
        const int jewelStartX = bx + 10;
        const int jewelStartY = landingStartY + 3;

        for (int deltaJewelY = 0; deltaJewelY < jewelHeight; ++deltaJewelY) {
            int targetPixelY = jewelStartY + deltaJewelY;
            int frameY = m_height - 1 - targetPixelY;
            if (frameY < 0 || frameY >= m_height) continue;
            int fbRowOffset = frameY * m_width;

            for (int deltaJewelX = 0; deltaJewelX < jewelWidth; ++deltaJewelX) {
                int targetPixelX = jewelStartX + deltaJewelX;
                if (targetPixelX < 0 || targetPixelX >= m_width) continue;

                ColorRGB jewelColor{255, 205, 55}; // Radiant Amber
                if (deltaJewelX == 0 || deltaJewelX == jewelWidth - 1) {
                    jewelColor = ColorRGB{190, 130, 30};
                }
                int fbIndex = fbRowOffset + targetPixelX;
                m_framebuffer[fbIndex] = (0xFFu << 24) |
                                        (static_cast<uint32_t>(jewelColor.b) << 16) |
                                        (static_cast<uint32_t>(jewelColor.g) << 8) |
                                        static_cast<uint32_t>(jewelColor.r);
            }
        }
    }

    // EN: 6. Center contents (icons and text) vertically inside the top landing
    // ES: 6. Centrar contenidos (iconos y texto) verticalmente dentro del plano superior
    const int contentCenterY = landingStartY + (landingEndY - landingStartY) / 2 + (active ? 1 : 0);

    ColorRGB contentColor = Palette::RELIC_BONE;
    if (active) {
        contentColor = ColorRGB{255, 235, 120}; // Glowing Divine Gold
    } else if (hovered) {
        contentColor = ColorRGB{255, 245, 185};
    } else if (pressed) {
        contentColor = ColorRGB{190, 180, 155};
    }

    // Helper lambda to draw small right-pointing triangle for cassette fast forward glyphs
    auto drawSmallTriangle = [this](int triX, int triY, int triW, int triH, ColorRGB col) {
        int halfH = triH / 2;
        for (int deltaY = -halfH; deltaY <= halfH; ++deltaY) {
            float ratio = 1.0f - static_cast<float>(std::abs(deltaY)) / static_cast<float>(halfH + 1);
            int rowWidth = std::max(1, static_cast<int>(ratio * static_cast<float>(triW)));
            int targetPixelY = triY + deltaY;
            int frameY = m_height - 1 - targetPixelY;
            if (frameY < 0 || frameY >= m_height) continue;
            int fbRowOffset = frameY * m_width;

            for (int deltaX = 0; deltaX < rowWidth; ++deltaX) {
                int targetPixelX = triX + deltaX;
                if (targetPixelX < 0 || targetPixelX >= m_width) continue;
                int fbIndex = fbRowOffset + targetPixelX;
                m_framebuffer[fbIndex] = (0xFFu << 24) |
                                        (static_cast<uint32_t>(col.b) << 16) |
                                        (static_cast<uint32_t>(col.g) << 8) |
                                        static_cast<uint32_t>(col.r);
            }
        }
    };

    if (btnId == 201) {
        // EN: PAUSE key: Double vertical bars ❚❚ + "PAUSA"
        // ES: Tecla PAUSA: Barras verticales dobles ❚❚ + "PAUSA"
        constexpr int barWidth = 3;
        constexpr int barHeight = 8;
        constexpr int barGap = 2;
        constexpr int iconTotalWidth = barWidth + barGap + barWidth; // 8px
        constexpr int textGap = 6;
        constexpr int textWidth = 5 * 6 + 4; // "PAUSA" is 34px
        constexpr int groupTotalWidth = iconTotalWidth + textGap + textWidth;
        const int startGroupX = bx + (bw - groupTotalWidth) / 2;

        int bar1X = startGroupX;
        int bar2X = startGroupX + barWidth + barGap;
        int barTopY = contentCenterY - barHeight / 2;

        // Shadow if unpressed
        if (!pressed) {
            ColorRGB shadowCol{12, 8, 6};
            for (int deltaY = 0; deltaY < barHeight; ++deltaY) {
                int targetPixelY = barTopY + deltaY + 1;
                int frameY = m_height - 1 - targetPixelY;
                if (frameY < 0 || frameY >= m_height) continue;
                int fbRowOffset = frameY * m_width;
                for (int deltaX = 0; deltaX < barWidth; ++deltaX) {
                    int p1 = bar1X + deltaX + 1;
                    int p2 = bar2X + deltaX + 1;
                    if (p1 >= 0 && p1 < m_width) m_framebuffer[fbRowOffset + p1] = (0xFFu << 24) | (shadowCol.b << 16) | (shadowCol.g << 8) | shadowCol.r;
                    if (p2 >= 0 && p2 < m_width) m_framebuffer[fbRowOffset + p2] = (0xFFu << 24) | (shadowCol.b << 16) | (shadowCol.g << 8) | shadowCol.r;
                }
            }
        }

        // Main bars
        for (int deltaY = 0; deltaY < barHeight; ++deltaY) {
            int targetPixelY = barTopY + deltaY;
            int frameY = m_height - 1 - targetPixelY;
            if (frameY < 0 || frameY >= m_height) continue;
            int fbRowOffset = frameY * m_width;
            for (int deltaX = 0; deltaX < barWidth; ++deltaX) {
                int p1 = bar1X + deltaX;
                int p2 = bar2X + deltaX;
                if (p1 >= 0 && p1 < m_width) m_framebuffer[fbRowOffset + p1] = (0xFFu << 24) | (contentColor.b << 16) | (contentColor.g << 8) | contentColor.r;
                if (p2 >= 0 && p2 < m_width) m_framebuffer[fbRowOffset + p2] = (0xFFu << 24) | (contentColor.b << 16) | (contentColor.g << 8) | contentColor.r;
            }
        }

        // Draw text "PAUSA"
        drawText(startGroupX + iconTotalWidth + textGap, contentCenterY - 4, "PAUSA", contentColor, 1);

    } else if (btnId == 203) {
        // EN: 2x Fast Forward key: Double triangle ▶▶ + "2x"
        // ES: Tecla 2x Avance Rápido: Doble triángulo ▶▶ + "2x"
        constexpr int triWidth = 5;
        constexpr int triHeight = 8;
        constexpr int triGap = 2;
        constexpr int iconTotalWidth = triWidth + triGap + triWidth; // 12px
        constexpr int textGap = 5;
        constexpr int textWidth = 2 * 6 + 1; // "2x" is 13px
        constexpr int groupTotalWidth = iconTotalWidth + textGap + textWidth;
        const int startGroupX = bx + (bw - groupTotalWidth) / 2;

        int tri1X = startGroupX;
        int tri2X = startGroupX + triWidth + triGap;

        if (!pressed) {
            ColorRGB shadowCol{12, 8, 6};
            drawSmallTriangle(tri1X + 1, contentCenterY + 1, triWidth, triHeight, shadowCol);
            drawSmallTriangle(tri2X + 1, contentCenterY + 1, triWidth, triHeight, shadowCol);
        }
        drawSmallTriangle(tri1X, contentCenterY, triWidth, triHeight, contentColor);
        drawSmallTriangle(tri2X, contentCenterY, triWidth, triHeight, contentColor);

        drawText(startGroupX + iconTotalWidth + textGap, contentCenterY - 4, "2x", contentColor, 1);

    } else if (btnId == 204) {
        // EN: 5x Ultra Fast Forward key: Triple triangle ▶▶▶ + "5x"
        // ES: Tecla 5x Avance Ultra Rápido: Triple triángulo ▶▶▶ + "5x"
        constexpr int triWidth = 4;
        constexpr int triHeight = 8;
        constexpr int triGap = 2;
        constexpr int iconTotalWidth = triWidth * 3 + triGap * 2; // 16px
        constexpr int textGap = 5;
        constexpr int textWidth = 2 * 6 + 1; // "5x" is 13px
        constexpr int groupTotalWidth = iconTotalWidth + textGap + textWidth;
        const int startGroupX = bx + (bw - groupTotalWidth) / 2;

        int tri1X = startGroupX;
        int tri2X = startGroupX + triWidth + triGap;
        int tri3X = startGroupX + (triWidth + triGap) * 2;

        if (!pressed) {
            ColorRGB shadowCol{12, 8, 6};
            drawSmallTriangle(tri1X + 1, contentCenterY + 1, triWidth, triHeight, shadowCol);
            drawSmallTriangle(tri2X + 1, contentCenterY + 1, triWidth, triHeight, shadowCol);
            drawSmallTriangle(tri3X + 1, contentCenterY + 1, triWidth, triHeight, shadowCol);
        }
        drawSmallTriangle(tri1X, contentCenterY, triWidth, triHeight, contentColor);
        drawSmallTriangle(tri2X, contentCenterY, triWidth, triHeight, contentColor);
        drawSmallTriangle(tri3X, contentCenterY, triWidth, triHeight, contentColor);

        drawText(startGroupX + iconTotalWidth + textGap, contentCenterY - 4, "5x", contentColor, 1);
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
    // 10: Lluvia Bendita (726)
    // 11: Gran Diluvio (727)
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
        case 726:           iconCol = 10; break;
        case 727:           iconCol = 11; break;
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
            case 726:           initial = "L"; break;
            case 727:           initial = "J"; break;
        }
        drawText(dstIconX + 4, dstIconY + 4, initial, rimColor, 1);
    }

    // Marco o gema divina si está activo
    if (active) {
        drawRectOutline(bx, by, bw, bh, Palette::DIVINE_GOLD);
        drawRectOutline(bx + 1, by + 1, bw - 2, bh - 2, ColorRGB{255, 235, 120});
    }
}

bool Renderer::initWindow(const std::string& title, bool startFullscreen) {
#if HAS_X11_GL
    Display* dpy = XOpenDisplay(nullptr);
    if (!dpy) {
        // Headless fallback
        m_hasActiveWindow = false;
        m_isFullscreen = startFullscreen;
        return false;
    }
    m_display = dpy;

    // EN: Install non-fatal X11 error handler to safely intercept window manager close events
    // ES: Instalar manejador de errores X11 no fatales para interceptar de forma segura eventos de cierre del gestor de ventanas
    static bool s_errorHandlerInstalled = false;
    if (!s_errorHandlerInstalled) {
        XSetErrorHandler([](Display*, XErrorEvent*) -> int { return 0; });
        s_errorHandlerInstalled = true;
    }

    Window root = DefaultRootWindow(dpy);
    const int screen = DefaultScreen(dpy);
    const int screenWidth = DisplayWidth(dpy, screen);
    const int screenHeight = DisplayHeight(dpy, screen);

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

    int initialWinX = 50;
    int initialWinY = 50;
    int initialWinW = m_width;
    int initialWinH = m_height;

    if (startFullscreen) {
        initialWinX = 0;
        initialWinY = 0;
        initialWinW = screenWidth;
        initialWinH = screenHeight;
    }

    Window win = XCreateWindow(dpy, root, initialWinX, initialWinY, initialWinW, initialWinH, 0, vi->depth,
                               InputOutput, vi->visual, CWColormap | CWEventMask, &swa);
    XStoreName(dpy, win, title.c_str());

    // EN: If configured to start in fullscreen, pre-set EWMH state and borderless Motif hints before mapping
    // ES: Si está configurado para arrancar en pantalla completa, pre-establecer estado EWMH y pistas Motif sin bordes antes de mapear
    if (startFullscreen) {
        Atom wmState = XInternAtom(dpy, "_NET_WM_STATE", False);
        Atom wmFullscreen = XInternAtom(dpy, "_NET_WM_STATE_FULLSCREEN", False);
        if (wmState != 0 && wmFullscreen != 0) {
            Atom atomType = XInternAtom(dpy, "ATOM", False);
            XChangeProperty(dpy, win, wmState, atomType, 32, PropModeReplace,
                            reinterpret_cast<const unsigned char*>(&wmFullscreen), 1);
        }

        Atom motifHintsAtom = XInternAtom(dpy, "_MOTIF_WM_HINTS", False);
        if (motifHintsAtom != 0) {
            constexpr unsigned long MWM_HINTS_DECORATIONS = (1L << 1);
            unsigned long hints[5] = { MWM_HINTS_DECORATIONS, 0, 0UL, 0, 0 };
            XChangeProperty(dpy, win, motifHintsAtom, motifHintsAtom, 32, PropModeReplace,
                            reinterpret_cast<const unsigned char*>(hints), 5);
        }
    }

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
    m_windowedX = 50;
    m_windowedY = 50;
    m_windowedWidth = 1280;
    m_windowedHeight = 720;

    // EN: Register WM_DELETE_WINDOW protocol to cleanly handle window close events
    // ES: Registrar protocolo WM_DELETE_WINDOW para gestionar limpiamente eventos de cierre de ventana
    Atom wmDelete = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
    if (wmDelete != 0) {
        XSetWMProtocols(dpy, win, &wmDelete, 1);
    }

    GLXContext glc = glXCreateContext(dpy, vi, nullptr, GL_TRUE);
    XFree(vi);
    glXMakeCurrent(dpy, win, glc);
    m_glContext = glc;
    m_hasActiveWindow = true;

    if (startFullscreen) {
        m_isFullscreen = true;
        resize(screenWidth, screenHeight);
        XRaiseWindow(dpy, win);
        XFlush(dpy);
    } else {
        m_isFullscreen = false;
        glViewport(0, 0, m_width, m_height);
    }
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
        m_isFullscreen = startFullscreen;
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
    m_windowedX = 50;
    m_windowedY = 50;
    m_windowedWidth = m_width;
    m_windowedHeight = m_height;

    if (startFullscreen) {
        SetWindowLongPtr(hwnd, GWL_STYLE, WS_POPUP | WS_VISIBLE);
        int screenW = GetSystemMetrics(SM_CXSCREEN);
        int screenH = GetSystemMetrics(SM_CYSCREEN);
        SetWindowPos(hwnd, HWND_TOP, 0, 0, screenW, screenH, SWP_FRAMECHANGED | SWP_SHOWWINDOW);
        m_isFullscreen = true;
        resize(screenW, screenH);
    } else {
        m_isFullscreen = false;
        glViewport(0, 0, m_width, m_height);
    }
    return true;
#else
    (void)title;
    m_isFullscreen = startFullscreen;
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
        XSync(dpy, False);
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

void Renderer::setFullscreen(bool enable) {
    if (m_isFullscreen == enable) return;

#if HAS_X11_GL
    if (m_hasActiveWindow && m_display && m_window) {
        Display* dpy = static_cast<Display*>(m_display);
        Window win = m_window;
        Window root = DefaultRootWindow(dpy);
        int screen = DefaultScreen(dpy);
        int screenWidth = DisplayWidth(dpy, screen);
        int screenHeight = DisplayHeight(dpy, screen);

        // EN: 1. Send EWMH _NET_WM_STATE_FULLSCREEN ClientMessage to the root window
        // ES: 1. Enviar ClientMessage EWMH _NET_WM_STATE_FULLSCREEN a la ventana raíz
        Atom wmState = XInternAtom(dpy, "_NET_WM_STATE", False);
        Atom wmFullscreen = XInternAtom(dpy, "_NET_WM_STATE_FULLSCREEN", False);

        if (wmState != 0 && wmFullscreen != 0) {
            XEvent xev{};
            xev.type = ClientMessage;
            xev.xclient.window = win;
            xev.xclient.message_type = wmState;
            xev.xclient.format = 32;
            xev.xclient.data.l[0] = enable ? 1 : 0; // 1 = _NET_WM_STATE_ADD, 0 = _NET_WM_STATE_REMOVE
            xev.xclient.data.l[1] = static_cast<long>(wmFullscreen);
            xev.xclient.data.l[2] = 0;
            xev.xclient.data.l[3] = 1; // 1 = normal application
            xev.xclient.data.l[4] = 0;

            XSendEvent(dpy, root, False, SubstructureRedirectMask | SubstructureNotifyMask, &xev);
        }

        // EN: 2. Set Motif hints for borderless display (decorations = 0) or restore normal borders (decorations = 1)
        // ES: 2. Establecer pistas Motif para visualización sin bordes (decorations = 0) o restaurar bordes normales (decorations = 1)
        Atom motifHintsAtom = XInternAtom(dpy, "_MOTIF_WM_HINTS", False);
        if (motifHintsAtom != 0) {
            constexpr unsigned long MWM_HINTS_DECORATIONS = (1L << 1);
            unsigned long hints[5] = { MWM_HINTS_DECORATIONS, 0, enable ? 0UL : 1UL, 0, 0 };
            XChangeProperty(dpy, win, motifHintsAtom, motifHintsAtom, 32, PropModeReplace,
                            reinterpret_cast<const unsigned char*>(hints), 5);
        }

        // EN: 3. Reposition and resize the native OS window
        // ES: 3. Reposicionar y redimensionar la ventana nativa del SO
        if (enable) {
            XMoveResizeWindow(dpy, win, 0, 0,
                              static_cast<unsigned int>(screenWidth),
                              static_cast<unsigned int>(screenHeight));
            XRaiseWindow(dpy, win);
            m_isFullscreen = true;
            resize(screenWidth, screenHeight);
        } else {
            XMoveResizeWindow(dpy, win, m_windowedX, m_windowedY,
                              static_cast<unsigned int>(m_windowedWidth),
                              static_cast<unsigned int>(m_windowedHeight));
            m_isFullscreen = false;
            resize(m_windowedWidth, m_windowedHeight);
        }
        XFlush(dpy);
    } else {
        // EN: Headless / Test environment fallback
        // ES: Fallback para entorno sin interfaz gráfica / Pruebas
        m_isFullscreen = enable;
    }
#elif defined(HAS_WIN32)
    if (m_hasActiveWindow && m_hwnd) {
        HWND hwnd = static_cast<HWND>(m_hwnd);
        if (enable) {
            RECT rect;
            GetWindowRect(hwnd, &rect);
            m_windowedX = rect.left;
            m_windowedY = rect.top;

            SetWindowLongPtr(hwnd, GWL_STYLE, WS_POPUP | WS_VISIBLE);
            int screenW = GetSystemMetrics(SM_CXSCREEN);
            int screenH = GetSystemMetrics(SM_CYSCREEN);
            SetWindowPos(hwnd, HWND_TOP, 0, 0, screenW, screenH, SWP_FRAMECHANGED | SWP_SHOWWINDOW);
            m_isFullscreen = true;
            resize(screenW, screenH);
        } else {
            SetWindowLongPtr(hwnd, GWL_STYLE, WS_OVERLAPPEDWINDOW | WS_VISIBLE);
            RECT wr = { 0, 0, m_windowedWidth, m_windowedHeight };
            AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);
            int winW = wr.right - wr.left;
            int winH = wr.bottom - wr.top;
            SetWindowPos(hwnd, HWND_NOTOPMOST, m_windowedX, m_windowedY, winW, winH, SWP_FRAMECHANGED | SWP_SHOWWINDOW);
            m_isFullscreen = false;
            resize(m_windowedWidth, m_windowedHeight);
        }
    } else {
        m_isFullscreen = enable;
    }
#else
    m_isFullscreen = enable;
#endif
}

void Renderer::setWindowSize(int newWidth, int newHeight) {
    if (newWidth <= 100 || newHeight <= 100) return;
    m_windowedWidth = newWidth;
    m_windowedHeight = newHeight;
    if (m_isFullscreen) return;

#if HAS_X11_GL
    if (m_hasActiveWindow && m_display && m_window) {
        Display* dpy = static_cast<Display*>(m_display);
        XResizeWindow(dpy, m_window, static_cast<unsigned int>(newWidth), static_cast<unsigned int>(newHeight));
        XFlush(dpy);
    }
#elif defined(HAS_WIN32)
    if (m_hasActiveWindow && m_hwnd) {
        RECT wr = { 0, 0, newWidth, newHeight };
        AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);
        int winW = wr.right - wr.left;
        int winH = wr.bottom - wr.top;
        SetWindowPos(static_cast<HWND>(m_hwnd), NULL, 0, 0, winW, winH, SWP_NOMOVE | SWP_NOZORDER);
    }
#endif
    resize(newWidth, newHeight);
}

void Renderer::setWindowedResolution(int windowedWidth, int windowedHeight) {
    if (windowedWidth <= 100 || windowedHeight <= 100) return;
    m_windowedWidth = windowedWidth;
    m_windowedHeight = windowedHeight;
    if (!m_isFullscreen) {
        setWindowSize(windowedWidth, windowedHeight);
    }
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
            } else if (fx.type == DivinePowerType::Rain) {
                addPointLight(pt.x, pt.y, 160.0f * zoom, ColorRGB{120, 190, 255}, 1.15f);
            } else if (fx.type == DivinePowerType::Deluge) {
                float pulse = 1.0f + 0.15f * std::sin(fx.timer * 6.0f);
                addPointLight(pt.x, pt.y, 250.0f * zoom, ColorRGB{70, 150, 255}, 1.45f * pulse);
            } else if (fx.type == DivinePowerType::AtomicBomb) {
                // EN: Colossal nuclear thermal flash and rising glowing fireball
                // ES: Colosal destello térmico nuclear y bola de fuego incandescente ascendente
                const float blastProg = std::clamp(fx.timer / fx.duration, 0.0f, 1.0f);
                if (fx.timer < 1.0f) {
                    const float flashIntensity = 4.5f * (1.0f - fx.timer);
                    addPointLight(pt.x, pt.y - 30.0f * zoom, 650.0f * zoom, ColorRGB{255, 255, 245}, flashIntensity);
                } else {
                    const float fireballDecay = 1.0f - blastProg;
                    const float coreElevationOffset = (30.0f + blastProg * 140.0f) * zoom;
                    addPointLight(pt.x, pt.y - coreElevationOffset, (280.0f + blastProg * 180.0f) * zoom, ColorRGB{255, 170, 50}, 3.0f * fireballDecay);
                }
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
    int deltaX = std::abs(x1 - x0);
    int deltaY = std::abs(y1 - y0);
    int stepX = (x0 < x1) ? 1 : -1;
    int stepY = (y0 < y1) ? 1 : -1;
    int errorValue = deltaX - deltaY;

    const uint32_t packedColor = color.toRGBA();
    const bool isOpaque = (color.a == 255);
    const float normalizedAlpha = static_cast<float>(color.a) / 255.0f;
    const float inverseAlpha = 1.0f - normalizedAlpha;

    while (true) {
        if (x0 >= 0 && x0 < m_width && y0 >= 0 && y0 < m_height) {
            // Flip Y for standard top-left screen coordinates
            int targetPixelY = m_height - 1 - y0;
            if (targetPixelY >= 0 && targetPixelY < m_height) {
                const int pixelIndex = targetPixelY * m_width + x0;
                if (isOpaque) {
                    m_framebuffer[pixelIndex] = packedColor;
                } else if (color.a > 0) {
                    // EN: Alpha blend line pixel with current framebuffer background
                    // ES: Mezclar con canal alfa el píxel de la línea con el fondo del framebuffer
                    const uint32_t backgroundPixel = m_framebuffer[pixelIndex];
                    const uint8_t bgR = static_cast<uint8_t>(backgroundPixel & 0xFF);
                    const uint8_t bgG = static_cast<uint8_t>((backgroundPixel >> 8) & 0xFF);
                    const uint8_t bgB = static_cast<uint8_t>((backgroundPixel >> 16) & 0xFF);
                    const uint8_t blendedR = static_cast<uint8_t>(color.r * normalizedAlpha + bgR * inverseAlpha);
                    const uint8_t blendedG = static_cast<uint8_t>(color.g * normalizedAlpha + bgG * inverseAlpha);
                    const uint8_t blendedB = static_cast<uint8_t>(color.b * normalizedAlpha + bgB * inverseAlpha);
                    m_framebuffer[pixelIndex] = (0xFFu << 24) | (blendedB << 16) | (blendedG << 8) | blendedR;
                }
            }
        }
        if (x0 == x1 && y0 == y1) break;
        int doubleError = 2 * errorValue;
        if (doubleError > -deltaY) { errorValue -= deltaY; x0 += stepX; }
        if (doubleError < deltaX)  { errorValue += deltaX; y0 += stepY; }
    }
}

void Renderer::drawRect(int x, int y, int w, int h, ColorRGB color) {
    const int xEnd = std::min(m_width, x + w);
    const int yEnd = std::min(m_height, y + h);

    if (color.a == 255) {
        // EN: Opaque solid fill fast path
        // ES: Ruta rápida para relleno sólido opaco
        const uint32_t packedColor = color.toRGBA();
        for (int j = std::max(0, y); j < yEnd; ++j) {
            const int py = m_height - 1 - j;
            if (py < 0 || py >= m_height) continue;
            const int rowOffset = py * m_width;
            for (int i = std::max(0, x); i < xEnd; ++i) {
                m_framebuffer[rowOffset + i] = packedColor;
            }
        }
    } else if (color.a > 0) {
        // EN: Alpha blended translucent fill path for frosted panels and veils
        // ES: Ruta de mezcla alfa para paneles translúcidos satinados y velos
        const float alpha = static_cast<float>(color.a) / 255.0f;
        const float invAlpha = 1.0f - alpha;
        const float srcR = static_cast<float>(color.r) * alpha;
        const float srcG = static_cast<float>(color.g) * alpha;
        const float srcB = static_cast<float>(color.b) * alpha;

        for (int j = std::max(0, y); j < yEnd; ++j) {
            const int py = m_height - 1 - j;
            if (py < 0 || py >= m_height) continue;
            const int rowOffset = py * m_width;
            for (int i = std::max(0, x); i < xEnd; ++i) {
                const uint32_t dstPixel = m_framebuffer[rowOffset + i];
                const uint8_t outR = static_cast<uint8_t>((dstPixel & 0xFF) * invAlpha + srcR);
                const uint8_t outG = static_cast<uint8_t>(((dstPixel >> 8) & 0xFF) * invAlpha + srcG);
                const uint8_t outB = static_cast<uint8_t>(((dstPixel >> 16) & 0xFF) * invAlpha + srcB);
                m_framebuffer[rowOffset + i] = (0xFFu << 24) |
                                              (static_cast<uint32_t>(outB) << 16) |
                                              (static_cast<uint32_t>(outG) << 8)  |
                                               static_cast<uint32_t>(outR);
            }
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

void Renderer::getTerrainPropBounds(float cx, float cy, int propIndex, float zoom,
                                    int& outStartX, int& outStartY, int& outWidth, int& outHeight) const {
    // EN: Two-cell height for forest trees (pine and deciduous), natural scaling for rocks and bushes
    // ES: Altura de dos celdas para árboles forestales (pino y caducifolio), escalado natural para rocas y arbustos
    if (propIndex == 0 || propIndex == 1) {
        constexpr float twoCellHeightPixels = 64.0f; // EN: 2 * isometric cellHeight (32px) / ES: 2 * altura celda isométrica (32px)
        const float treeHeight = std::max(12.0f, twoCellHeightPixels * zoom);
        const float aspectRatio = (propIndex == 0) ? 0.78f : 0.88f;
        outHeight = static_cast<int>(treeHeight);
        outWidth = static_cast<int>(treeHeight * aspectRatio);
        outStartX = static_cast<int>(cx - outWidth * 0.5f);
        outStartY = static_cast<int>(cy + 6.0f * zoom - outHeight);
    } else {
        const float propScale = std::clamp(zoom * 0.42f, 0.18f, 1.15f);
        outWidth = static_cast<int>(64.0f * propScale);
        outHeight = static_cast<int>(64.0f * propScale);
        outStartX = static_cast<int>(cx - outWidth * 0.5f);
        outStartY = static_cast<int>(cy + 6.0f * zoom - outHeight);
    }
}

void Renderer::drawTerrainProp(float cx, float cy, int propIndex, float zoom, float opacity) {
    if (propIndex < 0 || propIndex >= static_cast<int>(m_terrainPropPixels.size())) return;
    const auto& propPixels = m_terrainPropPixels[static_cast<size_t>(propIndex)];
    if (propPixels.size() != 64u * 64u) return;

    int destWidth = 0;
    int destHeight = 0;
    int startX = 0;
    int startY = 0;
    getTerrainPropBounds(cx, cy, propIndex, zoom, startX, startY, destWidth, destHeight);
    if (destWidth <= 2 || destHeight <= 2) return;

    const bool isTallTree = (propIndex == 0 || propIndex == 1);
    const float clampedOpacity = std::clamp(opacity, 0.0f, 1.0f);

    // EN: Ground contact shadow integrated with the isometric cell diamond base
    // ES: Sombra de contacto en el suelo integrada con la base romboidal de la celda isométrica
    int shadowRadiusX = std::max(3, static_cast<int>((isTallTree ? 16.0f : 10.0f) * zoom));
    int shadowRadiusY = std::max(2, static_cast<int>((isTallTree ? 8.0f : 5.0f) * zoom));
    int shadowCenterY = static_cast<int>(cy + 3.0f * zoom);
    int shadowCenterX = static_cast<int>(cx);
    for (int offsetY = -shadowRadiusY; offsetY <= shadowRadiusY; ++offsetY) {
        int frameY = m_height - 1 - (shadowCenterY + offsetY);
        if (frameY < 0 || frameY >= m_height) continue;
        for (int offsetX = -shadowRadiusX; offsetX <= shadowRadiusX; ++offsetX) {
            int frameX = shadowCenterX + offsetX;
            if (frameX < 0 || frameX >= m_width) continue;
            float normalizedDistance = (offsetX * offsetX) / static_cast<float>(shadowRadiusX * shadowRadiusX) +
                                       (offsetY * offsetY) / static_cast<float>(shadowRadiusY * shadowRadiusY);
            if (normalizedDistance <= 1.0f) {
                uint32_t backgroundColor = m_framebuffer[frameY * m_width + frameX];
                float shadowDarkness = 0.80f + (1.0f - clampedOpacity) * 0.20f;
                uint8_t baseRed = static_cast<uint8_t>((backgroundColor & 0xFF) * shadowDarkness);
                uint8_t baseGreen = static_cast<uint8_t>(((backgroundColor >> 8) & 0xFF) * shadowDarkness);
                uint8_t baseBlue = static_cast<uint8_t>(((backgroundColor >> 16) & 0xFF) * shadowDarkness);
                m_framebuffer[frameY * m_width + frameX] = (0xFFu << 24) | (baseBlue << 16) | (baseGreen << 8) | baseRed;
            }
        }
    }

    // EN: Blit sprite pixels with vertical-horizontal aspect mapping and translucency support
    // ES: Volcar píxeles del sprite con mapeo de aspecto vertical-horizontal y soporte de translucidez
    for (int deltaY = 0; deltaY < destHeight; ++deltaY) {
        int destinationY = startY + deltaY;
        int frameY = m_height - 1 - destinationY;
        if (frameY < 0 || frameY >= m_height) continue;

        int sourceY = std::clamp((deltaY * 64) / destHeight, 0, 63);

        for (int deltaX = 0; deltaX < destWidth; ++deltaX) {
            int destinationX = startX + deltaX;
            if (destinationX < 0 || destinationX >= m_width) continue;

            int sourceX = std::clamp((deltaX * 64) / destWidth, 0, 63);
            uint32_t sourcePixel = propPixels[static_cast<size_t>(sourceY) * 64u + static_cast<size_t>(sourceX)];
            uint8_t sourceAlpha = (sourcePixel >> 24) & 0xFF;
            if (sourceAlpha < 20) continue;

            uint8_t sourceRed = sourcePixel & 0xFF;
            uint8_t sourceGreen = (sourcePixel >> 8) & 0xFF;
            uint8_t sourceBlue = (sourcePixel >> 16) & 0xFF;

            if (clampedOpacity >= 0.999f && sourceAlpha >= 245) {
                m_framebuffer[frameY * m_width + destinationX] = (0xFFu << 24) | (sourceBlue << 16) | (sourceGreen << 8) | sourceRed;
            } else {
                uint32_t backgroundColor = m_framebuffer[frameY * m_width + destinationX];
                uint8_t backgroundRed = backgroundColor & 0xFF;
                uint8_t backgroundGreen = (backgroundColor >> 8) & 0xFF;
                uint8_t backgroundBlue = (backgroundColor >> 16) & 0xFF;
                float effectiveAlpha = (sourceAlpha / 255.0f) * clampedOpacity;
                if (effectiveAlpha < 0.05f) continue;
                uint8_t outputRed = static_cast<uint8_t>(sourceRed * effectiveAlpha + backgroundRed * (1.0f - effectiveAlpha));
                uint8_t outputGreen = static_cast<uint8_t>(sourceGreen * effectiveAlpha + backgroundGreen * (1.0f - effectiveAlpha));
                uint8_t outputBlue = static_cast<uint8_t>(sourceBlue * effectiveAlpha + backgroundBlue * (1.0f - effectiveAlpha));
                m_framebuffer[frameY * m_width + destinationX] = (0xFFu << 24) | (outputBlue << 16) | (outputGreen << 8) | outputRed;
            }
        }
    }
}

// EN: Calculate bounding box for destruction props: burnt trees, charred stumps, ruins and smoking timbers
// ES: Calcular caja delimitadora para accesorios de destrucción: árboles quemados, tocones, ruinas y maderas humeantes
void Renderer::getDestructionPropBounds(float cx, float cy, int propIndex, float zoom,
                                        int& outStartX, int& outStartY, int& outWidth, int& outHeight) const {
    // EN: Props 0, 1, 6 are tall burned trees; 4, 9, 10 are jagged trunks / smoking timber; 2, 3, 5, 7, 8 are stumps & rubble
    // ES: Props 0, 1, 6 son árboles quemados altos; 4, 9, 10 son troncos dentados / maderas humeantes; 2, 3, 5, 7, 8 son tocones y escombros
    if (propIndex == 0 || propIndex == 1 || propIndex == 6) {
        constexpr float twoCellHeightPixels = 64.0f;
        const float treeHeight = std::max(12.0f, twoCellHeightPixels * zoom);
        const float aspectRatio = (propIndex == 0) ? 0.80f : ((propIndex == 1) ? 0.88f : 0.84f);
        outHeight = static_cast<int>(treeHeight);
        outWidth = static_cast<int>(treeHeight * aspectRatio);
        outStartX = static_cast<int>(cx - outWidth * 0.5f);
        outStartY = static_cast<int>(cy + 6.0f * zoom - outHeight);
    } else if (propIndex == 4 || propIndex == 9 || propIndex == 10) {
        constexpr float mediumHeightPixels = 50.0f;
        const float propHeight = std::max(10.0f, mediumHeightPixels * zoom);
        outHeight = static_cast<int>(propHeight);
        outWidth = static_cast<int>(propHeight * 0.90f);
        outStartX = static_cast<int>(cx - outWidth * 0.5f);
        outStartY = static_cast<int>(cy + 5.0f * zoom - outHeight);
    } else {
        const float propScale = std::clamp(zoom * 0.44f, 0.18f, 1.15f);
        constexpr float baseSpriteDimension = 64.0f;
        outWidth = static_cast<int>(baseSpriteDimension * propScale);
        outHeight = static_cast<int>(baseSpriteDimension * propScale);
        outStartX = static_cast<int>(cx - outWidth * 0.5f);
        outStartY = static_cast<int>(cy + 5.0f * zoom - outHeight);
    }
}

// EN: Render destruction prop sprite (burnt trees, charred stumps, smoking timbers, debris) with ground shadow and alpha blending
// ES: Renderizar sprite de accesorio de destrucción (árboles quemados, tocones, maderas humeantes, escombros) con sombra y mezcla alfa
void Renderer::drawDestructionProp(float cx, float cy, int propIndex, float zoom, float opacity) {
    if (propIndex < 0 || propIndex >= static_cast<int>(m_destructionTreePixels.size())) return;
    const auto& propPixels = m_destructionTreePixels[static_cast<size_t>(propIndex)];
    if (propPixels.size() != 64u * 64u) return;

    int destWidth = 0;
    int destHeight = 0;
    int startX = 0;
    int startY = 0;
    getDestructionPropBounds(cx, cy, propIndex, zoom, startX, startY, destWidth, destHeight);
    if (destWidth <= 2 || destHeight <= 2) return;

    const bool isTallBurntTree = (propIndex == 0 || propIndex == 1 || propIndex == 6);
    const float clampedOpacity = std::clamp(opacity, 0.0f, 1.0f);

    // EN: Scorched ground contact shadow underneath charred prop
    // ES: Sombra de contacto de suelo calcinado bajo el elemento chamuscado
    const int shadowRadiusX = std::max(3, static_cast<int>((isTallBurntTree ? 14.0f : 10.0f) * zoom));
    const int shadowRadiusY = std::max(2, static_cast<int>((isTallBurntTree ? 7.0f : 5.0f) * zoom));
    const int shadowCenterY = static_cast<int>(cy + 3.0f * zoom);
    const int shadowCenterX = static_cast<int>(cx);
    for (int offsetY = -shadowRadiusY; offsetY <= shadowRadiusY; ++offsetY) {
        const int frameY = m_height - 1 - (shadowCenterY + offsetY);
        if (frameY < 0 || frameY >= m_height) continue;
        for (int offsetX = -shadowRadiusX; offsetX <= shadowRadiusX; ++offsetX) {
            const int frameX = shadowCenterX + offsetX;
            if (frameX < 0 || frameX >= m_width) continue;
            const float normalizedDistance = (offsetX * offsetX) / static_cast<float>(shadowRadiusX * shadowRadiusX) +
                                             (offsetY * offsetY) / static_cast<float>(shadowRadiusY * shadowRadiusY);
            if (normalizedDistance <= 1.0f) {
                const uint32_t backgroundColor = m_framebuffer[frameY * m_width + frameX];
                constexpr float shadowDarkness = 0.68f;
                const uint8_t baseRed = static_cast<uint8_t>((backgroundColor & 0xFF) * shadowDarkness);
                const uint8_t baseGreen = static_cast<uint8_t>(((backgroundColor >> 8) & 0xFF) * shadowDarkness);
                const uint8_t baseBlue = static_cast<uint8_t>(((backgroundColor >> 16) & 0xFF) * shadowDarkness);
                m_framebuffer[frameY * m_width + frameX] = (0xFFu << 24) | (baseBlue << 16) | (baseGreen << 8) | baseRed;
            }
        }
    }

    // EN: Blit charred sprite pixels with alpha transparency and subtle fire ember flicker for ember props
    // ES: Volcar píxeles de sprite calcinado con transparencia alfa y parpadeo sutil para props con ascuas
    const bool hasActiveEmbers = (propIndex == 6 || propIndex == 8 || propIndex == 9);
    const float emberFlicker = hasActiveEmbers ? (0.85f + 0.15f * std::sin(m_animTime * 6.0f + cx * 0.1f)) : 1.0f;

    for (int deltaY = 0; deltaY < destHeight; ++deltaY) {
        const int destinationY = startY + deltaY;
        const int frameY = m_height - 1 - destinationY;
        if (frameY < 0 || frameY >= m_height) continue;

        const int sourceY = std::clamp((deltaY * 64) / destHeight, 0, 63);

        for (int deltaX = 0; deltaX < destWidth; ++deltaX) {
            const int destinationX = startX + deltaX;
            if (destinationX < 0 || destinationX >= m_width) continue;

            const int sourceX = std::clamp((deltaX * 64) / destWidth, 0, 63);
            const uint32_t sourcePixel = propPixels[static_cast<size_t>(sourceY) * 64u + static_cast<size_t>(sourceX)];
            const uint8_t sourceAlpha = (sourcePixel >> 24) & 0xFF;
            if (sourceAlpha < 18) continue;

            uint8_t sourceRed = sourcePixel & 0xFF;
            uint8_t sourceGreen = (sourcePixel >> 8) & 0xFF;
            uint8_t sourceBlue = (sourcePixel >> 16) & 0xFF;

            if (hasActiveEmbers && sourceRed > 160 && sourceGreen < 100) {
                sourceRed = static_cast<uint8_t>(std::clamp(sourceRed * emberFlicker, 0.0f, 255.0f));
                sourceGreen = static_cast<uint8_t>(std::clamp(sourceGreen * emberFlicker, 0.0f, 255.0f));
            }

            if (clampedOpacity >= 0.999f && sourceAlpha >= 245) {
                m_framebuffer[frameY * m_width + destinationX] = (0xFFu << 24) | (sourceBlue << 16) | (sourceGreen << 8) | sourceRed;
            } else {
                const uint32_t backgroundColor = m_framebuffer[frameY * m_width + destinationX];
                const uint8_t backgroundRed = backgroundColor & 0xFF;
                const uint8_t backgroundGreen = (backgroundColor >> 8) & 0xFF;
                const uint8_t backgroundBlue = (backgroundColor >> 16) & 0xFF;
                const float effectiveAlpha = (sourceAlpha / 255.0f) * clampedOpacity;
                if (effectiveAlpha < 0.04f) continue;
                const uint8_t outputRed = static_cast<uint8_t>(sourceRed * effectiveAlpha + backgroundRed * (1.0f - effectiveAlpha));
                const uint8_t outputGreen = static_cast<uint8_t>(sourceGreen * effectiveAlpha + backgroundGreen * (1.0f - effectiveAlpha));
                const uint8_t outputBlue = static_cast<uint8_t>(sourceBlue * effectiveAlpha + backgroundBlue * (1.0f - effectiveAlpha));
                m_framebuffer[frameY * m_width + destinationX] = (0xFFu << 24) | (outputBlue << 16) | (outputGreen << 8) | outputRed;
            }
        }
    }
}

// EN: Calculate bounding box for impact craters in 2.5D isometric ground projection
// ES: Calcular caja delimitadora para cráteres de impacto en proyección isométrica 2.5D de suelo
void Renderer::getCraterBounds(float cx, float cy, int craterIndex, float zoom,
                               int& outStartX, int& outStartY, int& outWidth, int& outHeight) const {
    // EN: Colossal crater sprite (index 7) has 128x128 source texture (128x64 in isometric 2:1 projection), others 64x64 (64x32)
    // ES: El sprite de cráter colosal (índice 7) tiene textura origen 128x128 (128x64 en proyección 2:1), otros 64x64 (64x32)
    const int baseDimension = (craterIndex == 7) ? 128 : 64;

    outWidth = std::max(4, static_cast<int>(static_cast<float>(baseDimension) * zoom));
    outHeight = std::max(2, static_cast<int>(static_cast<float>(baseDimension) * 0.5f * zoom));
    outStartX = static_cast<int>(cx - static_cast<float>(outWidth) * 0.5f);
    outStartY = static_cast<int>(cy - static_cast<float>(outHeight) * 0.5f);
}

// EN: Render authentic impact crater sprite onto isometric ground with 2:1 perspective and alpha blending
// ES: Renderizar sprite auténtico de cráter de impacto sobre el suelo isométrico con perspectiva 2:1 y mezcla alfa
void Renderer::drawCraterSprite(float cx, float cy, int craterIndex, float zoom, float opacity) {
    if (craterIndex < 0 || craterIndex >= static_cast<int>(m_destructionCraterPixels.size())) return;
    const auto& craterPixels = m_destructionCraterPixels[static_cast<size_t>(craterIndex)];
    const int baseDimension = (craterIndex == 7) ? 128 : 64;
    if (craterPixels.size() != static_cast<size_t>(baseDimension * baseDimension)) return;

    int destWidth = 0;
    int destHeight = 0;
    int startX = 0;
    int startY = 0;
    getCraterBounds(cx, cy, craterIndex, zoom, startX, startY, destWidth, destHeight);
    if (destWidth <= 2 || destHeight <= 2) return;

    const float clampedOpacity = std::clamp(opacity, 0.0f, 1.0f);

    for (int deltaY = 0; deltaY < destHeight; ++deltaY) {
        const int destinationY = startY + deltaY;
        const int frameY = m_height - 1 - destinationY;
        if (frameY < 0 || frameY >= m_height) continue;

        const int sourceY = std::clamp((deltaY * baseDimension) / destHeight, 0, baseDimension - 1);
        const int sourceRow = sourceY * baseDimension;
        const int frameRow = frameY * m_width;

        for (int deltaX = 0; deltaX < destWidth; ++deltaX) {
            const int destinationX = startX + deltaX;
            if (destinationX < 0 || destinationX >= m_width) continue;

            const int sourceX = std::clamp((deltaX * baseDimension) / destWidth, 0, baseDimension - 1);
            const uint32_t sourcePixel = craterPixels[static_cast<size_t>(sourceRow + sourceX)];
            const uint8_t sourceAlpha = (sourcePixel >> 24) & 0xFF;
            if (sourceAlpha < 8) continue;

            const uint8_t sourceRed = sourcePixel & 0xFF;
            const uint8_t sourceGreen = (sourcePixel >> 8) & 0xFF;
            const uint8_t sourceBlue = (sourcePixel >> 16) & 0xFF;

            const uint32_t backgroundColor = m_framebuffer[frameRow + destinationX];
            const uint8_t backgroundRed = backgroundColor & 0xFF;
            const uint8_t backgroundGreen = (backgroundColor >> 8) & 0xFF;
            const uint8_t backgroundBlue = (backgroundColor >> 16) & 0xFF;

            const float effectiveAlpha = (static_cast<float>(sourceAlpha) / 255.0f) * clampedOpacity;
            const uint8_t outputRed = static_cast<uint8_t>(static_cast<float>(sourceRed) * effectiveAlpha + static_cast<float>(backgroundRed) * (1.0f - effectiveAlpha));
            const uint8_t outputGreen = static_cast<uint8_t>(static_cast<float>(sourceGreen) * effectiveAlpha + static_cast<float>(backgroundGreen) * (1.0f - effectiveAlpha));
            const uint8_t outputBlue = static_cast<uint8_t>(static_cast<float>(sourceBlue) * effectiveAlpha + static_cast<float>(backgroundBlue) * (1.0f - effectiveAlpha));

            m_framebuffer[frameRow + destinationX] = (0xFFu << 24) |
                                                     (static_cast<uint32_t>(outputBlue) << 16) |
                                                     (static_cast<uint32_t>(outputGreen) << 8)  |
                                                      static_cast<uint32_t>(outputRed);
        }
    }
}

void Renderer::getResourceNodeBounds(float cx, float cy, ResourceType type, float zoom,
                                     int& outStartX, int& outStartY, int& outWidth, int& outHeight) const {
    if (type == ResourceType::Wood) {
        // EN: Harvestable resource trees scale to 2 full isometric cells
        // ES: Árboles de recursos recolectables escalan a 2 celdas isométricas completas
        constexpr float twoCellHeightPixels = 64.0f;
        const float treeHeight = std::max(12.0f, twoCellHeightPixels * zoom);
        constexpr float treeAspectRatio = 0.84f;
        outHeight = static_cast<int>(treeHeight);
        outWidth = static_cast<int>(treeHeight * treeAspectRatio);
        outStartX = static_cast<int>(cx - outWidth * 0.5f);
        outStartY = static_cast<int>(cy + 6.0f * zoom - outHeight);
    } else {
        const float propScale = std::clamp(zoom * 0.44f, 0.18f, 1.20f);
        outWidth = static_cast<int>(64.0f * propScale);
        outHeight = static_cast<int>(64.0f * propScale);
        outStartX = static_cast<int>(cx - outWidth * 0.5f);
        outStartY = static_cast<int>(cy + 6.0f * zoom - outHeight);
    }
}

void Renderer::drawResourceNode(float cx, float cy, ResourceType type, float zoom, int amount, float opacity,
                                uint8_t variant, int maxAmount) {
    int resourceIndex = -1;
    switch (type) {
        case ResourceType::Food:    resourceIndex = 0; break;
        case ResourceType::Wood:    resourceIndex = 1; break;
        case ResourceType::Stone:   resourceIndex = 2; break;
        case ResourceType::Gold:    resourceIndex = 3; break;
        case ResourceType::Iron:    resourceIndex = 4; break;
        case ResourceType::Copper:  resourceIndex = 5; break;
        case ResourceType::Diamond: resourceIndex = 6; break;
        default: return;
    }
    if (resourceIndex < 0 || resourceIndex >= static_cast<int>(m_resourcePropPixels.size())) return;

    // EN: Select appropriate texture variant based on geological setting and extraction depletion
    // ES: Seleccionar variante de textura apropiada según contexto geológico y nivel de extracción
    size_t selectedVariantIndex = static_cast<size_t>(variant);
    if (type == ResourceType::Stone || type == ResourceType::Gold ||
        type == ResourceType::Iron || type == ResourceType::Copper ||
        type == ResourceType::Diamond) {
        // EN: Depletion threshold: if deposit is heavily extracted (<35% remaining), show scattered rubble/nuggets
        // ES: Umbral de agotamiento: si el depósito fue fuertemente extraído (<35% restante), mostrar cascajo/pepitas
        if (maxAmount > 0 && amount < (maxAmount * 35) / 100) {
            selectedVariantIndex = 2; // EN: Nuggets / Rubble / ES: Pepitas / Cascajo
        } else if (maxAmount > 0 && amount > (maxAmount * 85) / 100 && variant == 3) {
            selectedVariantIndex = 3; // EN: Hoard / Quarry / ES: Tesoro / Cantera
        }
    } else if (type == ResourceType::Wood) {
        if (maxAmount > 0 && amount < (maxAmount * 25) / 100) {
            selectedVariantIndex = 4; // EN: Stump / ES: Tocón
        }
    }

    if (selectedVariantIndex >= 5) {
        selectedVariantIndex = 0;
    }

    const std::vector<uint32_t>* targetTexturePixels = nullptr;
    const auto& candidateVariant = m_resourceVariantPixels[static_cast<size_t>(resourceIndex)][selectedVariantIndex];
    if (candidateVariant.size() == 64u * 64u) {
        targetTexturePixels = &candidateVariant;
    } else if (m_resourcePropPixels[static_cast<size_t>(resourceIndex)].size() == 64u * 64u) {
        targetTexturePixels = &m_resourcePropPixels[static_cast<size_t>(resourceIndex)];
    }

    if (!targetTexturePixels) return;
    const auto& propPixels = *targetTexturePixels;

    int destWidth = 0;
    int destHeight = 0;
    int startX = 0;
    int startY = 0;
    getResourceNodeBounds(cx, cy, type, zoom, startX, startY, destWidth, destHeight);
    if (destWidth <= 2 || destHeight <= 2) return;

    const bool isTallTree = (type == ResourceType::Wood);
    const float clampedOpacity = std::clamp(opacity, 0.0f, 1.0f);

    // EN: Ground contact shadow for natural resource deposit
    // ES: Sombra de contacto en el suelo para el depósito de recursos naturales
    int shadowRadiusX = std::max(3, static_cast<int>((isTallTree ? 16.0f : 12.0f) * zoom));
    int shadowRadiusY = std::max(2, static_cast<int>((isTallTree ? 8.0f : 6.0f) * zoom));
    int shadowCenterY = static_cast<int>(cy + 3.0f * zoom);
    int shadowCenterX = static_cast<int>(cx);
    for (int offsetY = -shadowRadiusY; offsetY <= shadowRadiusY; ++offsetY) {
        int frameY = m_height - 1 - (shadowCenterY + offsetY);
        if (frameY < 0 || frameY >= m_height) continue;
        for (int offsetX = -shadowRadiusX; offsetX <= shadowRadiusX; ++offsetX) {
            int frameX = shadowCenterX + offsetX;
            if (frameX < 0 || frameX >= m_width) continue;
            float normalizedDistance = (offsetX * offsetX) / static_cast<float>(shadowRadiusX * shadowRadiusX) +
                                       (offsetY * offsetY) / static_cast<float>(shadowRadiusY * shadowRadiusY);
            if (normalizedDistance <= 1.0f) {
                uint32_t backgroundColor = m_framebuffer[frameY * m_width + frameX];
                float shadowDarkness = 0.78f + (1.0f - clampedOpacity) * 0.22f;
                uint8_t baseRed = static_cast<uint8_t>((backgroundColor & 0xFF) * shadowDarkness);
                uint8_t baseGreen = static_cast<uint8_t>(((backgroundColor >> 8) & 0xFF) * shadowDarkness);
                uint8_t baseBlue = static_cast<uint8_t>(((backgroundColor >> 16) & 0xFF) * shadowDarkness);
                m_framebuffer[frameY * m_width + frameX] = (0xFFu << 24) | (baseBlue << 16) | (baseGreen << 8) | baseRed;
            }
        }
    }

    for (int deltaY = 0; deltaY < destHeight; ++deltaY) {
        int destinationY = startY + deltaY;
        int frameY = m_height - 1 - destinationY;
        if (frameY < 0 || frameY >= m_height) continue;

        int sourceY = std::clamp((deltaY * 64) / destHeight, 0, 63);

        for (int deltaX = 0; deltaX < destWidth; ++deltaX) {
            int destinationX = startX + deltaX;
            if (destinationX < 0 || destinationX >= m_width) continue;

            int sourceX = std::clamp((deltaX * 64) / destWidth, 0, 63);
            uint32_t sourcePixel = propPixels[static_cast<size_t>(sourceY) * 64u + static_cast<size_t>(sourceX)];
            uint8_t sourceAlpha = (sourcePixel >> 24) & 0xFF;
            if (sourceAlpha < 15) continue;

            uint8_t sourceRed = sourcePixel & 0xFF;
            uint8_t sourceGreen = (sourcePixel >> 8) & 0xFF;
            uint8_t sourceBlue = (sourcePixel >> 16) & 0xFF;

            if (clampedOpacity >= 0.999f && sourceAlpha >= 245) {
                m_framebuffer[frameY * m_width + destinationX] = (0xFFu << 24) | (sourceBlue << 16) | (sourceGreen << 8) | sourceRed;
            } else {
                uint32_t backgroundColor = m_framebuffer[frameY * m_width + destinationX];
                uint8_t backgroundRed = backgroundColor & 0xFF;
                uint8_t backgroundGreen = (backgroundColor >> 8) & 0xFF;
                uint8_t backgroundBlue = (backgroundColor >> 16) & 0xFF;
                float effectiveAlpha = (sourceAlpha / 255.0f) * clampedOpacity;
                if (effectiveAlpha < 0.05f) continue;
                uint8_t outputRed = static_cast<uint8_t>(sourceRed * effectiveAlpha + backgroundRed * (1.0f - effectiveAlpha));
                uint8_t outputGreen = static_cast<uint8_t>(sourceGreen * effectiveAlpha + backgroundGreen * (1.0f - effectiveAlpha));
                uint8_t outputBlue = static_cast<uint8_t>(sourceBlue * effectiveAlpha + backgroundBlue * (1.0f - effectiveAlpha));
                m_framebuffer[frameY * m_width + destinationX] = (0xFFu << 24) | (outputBlue << 16) | (outputGreen << 8) | outputRed;
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

void Renderer::getMonumentBounds(float cx, float cy, int shrineLevel, float zoom,
                                 int& outStartX, int& outStartY, int& outWidth, int& outHeight) const {
    (void)shrineLevel;
    const float monumentScale = std::clamp(zoom * 0.36f, 0.14f, 1.30f);
    outWidth = static_cast<int>(128.0f * monumentScale);
    outHeight = static_cast<int>(128.0f * monumentScale);
    outStartX = static_cast<int>(cx - outWidth * 0.5f);
    outStartY = static_cast<int>(cy + 11.0f * zoom - outHeight);
}

void Renderer::drawSpriteMonument(float cx, float cy, int shrineLevel, ColorRGB color, float zoom, float opacity) {
    (void)shrineLevel;
    const float clampedOpacity = std::clamp(opacity, 0.0f, 1.0f);
    if (m_templePixels.size() == 128 * 128) {
        // EN: Monument occupying strictly one single isometric diamond tile
        // ES: Monumento ocupando estrictamente una sola baldosa romboidal isométrica
        int destWidth = 0;
        int destHeight = 0;
        int startX = 0;
        int startY = 0;
        getMonumentBounds(cx, cy, shrineLevel, zoom, startX, startY, destWidth, destHeight);
        if (destWidth <= 4 || destHeight <= 4) return;

        // EN: Elliptical contact shadow inscribed in the cell diamond
        // ES: Sombra de contacto elíptica inscrita en el rombo de la celda
        int shadowRadiusX = std::max(4, static_cast<int>(19.0f * zoom));
        int shadowRadiusY = std::max(2, static_cast<int>(9.5f * zoom));
        int shadowCenterX = static_cast<int>(cx);
        int shadowCenterY = static_cast<int>(cy + 1.5f * zoom);
        for (int offsetY = -shadowRadiusY; offsetY <= shadowRadiusY; ++offsetY) {
            int frameY = m_height - 1 - (shadowCenterY + offsetY);
            if (frameY < 0 || frameY >= m_height) continue;
            for (int offsetX = -shadowRadiusX; offsetX <= shadowRadiusX; ++offsetX) {
                int frameX = shadowCenterX + offsetX;
                if (frameX < 0 || frameX >= m_width) continue;
                float normalizedDistance = (offsetX * offsetX) / static_cast<float>(shadowRadiusX * shadowRadiusX) +
                                           (offsetY * offsetY) / static_cast<float>(shadowRadiusY * shadowRadiusY);
                if (normalizedDistance <= 1.0f) {
                    uint32_t backgroundColor = m_framebuffer[frameY * m_width + frameX];
                    float shadowDarkness = 0.72f + (1.0f - clampedOpacity) * 0.28f;
                    uint8_t baseRed = static_cast<uint8_t>((backgroundColor & 0xFF) * shadowDarkness);
                    uint8_t baseGreen = static_cast<uint8_t>(((backgroundColor >> 8) & 0xFF) * shadowDarkness);
                    uint8_t baseBlue = static_cast<uint8_t>(((backgroundColor >> 16) & 0xFF) * shadowDarkness);
                    m_framebuffer[frameY * m_width + frameX] = (0xFFu << 24) | (baseBlue << 16) | (baseGreen << 8) | baseRed;
                }
            }
        }

        for (int deltaY = 0; deltaY < destHeight; ++deltaY) {
            int destinationY = startY + deltaY;
            int frameY = m_height - 1 - destinationY;
            if (frameY < 0 || frameY >= m_height) continue;

            int sourceY = std::clamp((deltaY * 128) / destHeight, 0, 127);

            for (int deltaX = 0; deltaX < destWidth; ++deltaX) {
                int destinationX = startX + deltaX;
                if (destinationX < 0 || destinationX >= m_width) continue;

                int sourceX = std::clamp((deltaX * 128) / destWidth, 0, 127);
                uint32_t sourcePixel = m_templePixels[sourceY * 128 + sourceX];
                uint8_t sourceAlpha = (sourcePixel >> 24) & 0xFF;
                if (sourceAlpha < 20) continue;

                uint8_t sourceRed = sourcePixel & 0xFF;
                uint8_t sourceGreen = (sourcePixel >> 8) & 0xFF;
                uint8_t sourceBlue = (sourcePixel >> 16) & 0xFF;

                // Engastar las gemas facetadas sagradas en los dos medallones solares (63,12 y 63,40)
                int deltaMedallion1X = sourceX - 63;
                int deltaMedallion1Y = sourceY - 12;
                int deltaMedallion2X = sourceX - 63;
                int deltaMedallion2Y = sourceY - 40;
                int distance1Squared = deltaMedallion1X * deltaMedallion1X + deltaMedallion1Y * deltaMedallion1Y;
                int distance2Squared = deltaMedallion2X * deltaMedallion2X + deltaMedallion2Y * deltaMedallion2Y;
                if (distance1Squared <= 9 || distance2Squared <= 16) {
                    bool facetHighlight = (deltaMedallion1X <= 0 && deltaMedallion1Y <= 0 && distance1Squared <= 4) ||
                                          (deltaMedallion2X <= 0 && deltaMedallion2Y <= 0 && distance2Squared <= 6);
                    bool darkRim = (distance1Squared >= 7 && distance1Squared <= 9) || (distance2Squared >= 12 && distance2Squared <= 16);
                    if (facetHighlight) {
                        sourceRed = static_cast<uint8_t>(std::min(255, color.r + 85));
                        sourceGreen = static_cast<uint8_t>(std::min(255, color.g + 85));
                        sourceBlue = static_cast<uint8_t>(std::min(255, color.b + 85));
                    } else if (darkRim) {
                        sourceRed = static_cast<uint8_t>(color.r * 0.55f);
                        sourceGreen = static_cast<uint8_t>(color.g * 0.55f);
                        sourceBlue = static_cast<uint8_t>(color.b * 0.55f);
                    } else {
                        sourceRed = color.r;
                        sourceGreen = color.g;
                        sourceBlue = color.b;
                    }
                }

                if (clampedOpacity >= 0.999f && sourceAlpha >= 245) {
                    m_framebuffer[frameY * m_width + destinationX] = (0xFFu << 24) | (sourceBlue << 16) | (sourceGreen << 8) | sourceRed;
                } else {
                    uint32_t backgroundColor = m_framebuffer[frameY * m_width + destinationX];
                    uint8_t backgroundRed = backgroundColor & 0xFF;
                    uint8_t backgroundGreen = (backgroundColor >> 8) & 0xFF;
                    uint8_t backgroundBlue = (backgroundColor >> 16) & 0xFF;
                    float effectiveAlpha = (sourceAlpha / 255.0f) * clampedOpacity;
                    if (effectiveAlpha < 0.05f) continue;
                    uint8_t outputRed = static_cast<uint8_t>(sourceRed * effectiveAlpha + backgroundRed * (1.0f - effectiveAlpha));
                    uint8_t outputGreen = static_cast<uint8_t>(sourceGreen * effectiveAlpha + backgroundGreen * (1.0f - effectiveAlpha));
                    uint8_t outputBlue = static_cast<uint8_t>(sourceBlue * effectiveAlpha + backgroundBlue * (1.0f - effectiveAlpha));
                    m_framebuffer[frameY * m_width + destinationX] = (0xFFu << 24) | (outputBlue << 16) | (outputGreen << 8) | outputRed;
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

void Renderer::getHouseBounds(float cx, float cy, const House& house, float zoom,
                              int& outStartX, int& outStartY, int& outWidth, int& outHeight) const {
    int sourceWidth = 137;
    int sourceHeight = 100;
    if (house.isUnderConstruction()) {
        if (house.type == HouseType::BoneDwelling) {
            sourceWidth = 157; sourceHeight = 99; // crop_bone_frame
        } else {
            sourceWidth = 136; sourceHeight = 108; // crop_house_frame
        }
    } else {
        switch (house.type) {
            case HouseType::BoneDwelling:
                sourceWidth = 117; sourceHeight = 103; // crop_bone_dwelling
                break;
            case HouseType::LargeRoundhouse:
                sourceWidth = 156; sourceHeight = 156;   // crop_house_roundhouse
                break;
            case HouseType::ChieftainLonghouse:
                // EN: Full intact bounding box for Chieftain Longhouse (includes full base, porch and roof ridge)
                // ES: Caja delimitadora completa e intacta para la Casa del Cacique (incluye base completa, porche y caballete)
                sourceWidth = 187; sourceHeight = 182; // crop_house_chieftain
                break;
            case HouseType::ThatchedHut:
            case HouseType::PrimitiveShelter:
            default:
                sourceWidth = 137; sourceHeight = 100; // crop_house_thatched
                break;
        }
    }

    const float houseScale = std::clamp(zoom * 0.44f, 0.16f, 1.25f);
    outWidth = static_cast<int>(sourceWidth * houseScale);
    outHeight = static_cast<int>(sourceHeight * houseScale);
    outStartX = static_cast<int>(cx - outWidth * 0.5f);
    outStartY = static_cast<int>(cy + 10.0f * zoom - outHeight);
}

void Renderer::drawSpriteHouse(float cx, float cy, const House& house, float zoom, bool isNight, float opacity) {
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
    bool isRuined = house.isRuined();

    if (isRuined) {
        // EN: Ruined structure: charred and fractured skeletal timber frame
        // ES: Estructura en ruinas: armazón esquelético de madera fracturado y calcinado
        srcX = 29; srcY = 428; srcW = 136; srcH = 108;
    } else if (underConstr) {
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
                // EN: Full intact sprite coordinates for Chieftain Longhouse in tribal_buildings_sheet
                // ES: Coordenadas completas del sprite intacto para la Casa del Cacique en tribal_buildings_sheet
                srcX = 682; srcY = 140; srcW = 187; srcH = 182; // crop_house_chieftain
                break;
            case HouseType::ThatchedHut:
            case HouseType::PrimitiveShelter:
            default:
                srcX = 201; srcY = 101; srcW = 137; srcH = 100; // crop_house_thatched
                break;
        }
    }

    const float houseScale = std::clamp(zoom * 0.44f, 0.16f, 1.25f);
    int destW = static_cast<int>(srcW * houseScale);
    int destH = static_cast<int>(srcH * houseScale);
    if (destW <= 4 || destH <= 4) return;

    int startX = static_cast<int>(cx - destW * 0.5f);
    int startY = static_cast<int>(cy + 10.0f * zoom - destH);

    const float clampedOpacity = std::clamp(opacity, 0.0f, 1.0f);

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
                float shadowDarkness = 0.72f + (1.0f - clampedOpacity) * 0.28f;
                uint8_t br = static_cast<uint8_t>((bg & 0xFF) * shadowDarkness);
                uint8_t bgc = static_cast<uint8_t>(((bg >> 8) & 0xFF) * shadowDarkness);
                uint8_t bb = static_cast<uint8_t>(((bg >> 16) & 0xFF) * shadowDarkness);
                m_framebuffer[fy * m_width + fx] = (0xFFu << 24) | (bb << 16) | (bgc << 8) | br;
            }
        }
    }

    // Blit de los píxeles del edificio con soporte de opacidad
    for (int dy = 0; dy < destH; ++dy) {
        int dstY = startY + dy;
        int frameY = m_height - 1 - dstY;
        if (frameY < 0 || frameY >= m_height) continue;

        int sy = srcY + std::clamp(static_cast<int>(dy / houseScale), 0, srcH - 1);

        for (int dx = 0; dx < destW; ++dx) {
            int dstX = startX + dx;
            if (dstX < 0 || dstX >= m_width) continue;

            int sx = srcX + std::clamp(static_cast<int>(dx / houseScale), 0, srcW - 1);
            uint32_t src = m_buildingsPixels[sy * m_buildingsWidth + sx];
            uint8_t a = (src >> 24) & 0xFF;
            if (a < 18) continue;

            uint8_t sr = src & 0xFF;
            uint8_t sg = (src >> 8) & 0xFF;
            uint8_t sb = (src >> 16) & 0xFF;

            if (isRuined) {
                // EN: Charcoal ashen tint for ruined collapsed building
                // ES: Tinte de ceniza y carbón para edificio colapsado en ruinas
                sr = static_cast<uint8_t>(sr * 0.42f);
                sg = static_cast<uint8_t>(sg * 0.36f);
                sb = static_cast<uint8_t>(sb * 0.36f);
            }

            if (clampedOpacity >= 0.999f && a >= 245) {
                m_framebuffer[frameY * m_width + dstX] = (0xFFu << 24) | (sb << 16) | (sg << 8) | sr;
            } else {
                uint32_t bg = m_framebuffer[frameY * m_width + dstX];
                uint8_t br = bg & 0xFF;
                uint8_t bgc = (bg >> 8) & 0xFF;
                uint8_t bb = (bg >> 16) & 0xFF;
                float effectiveAlpha = (a / 255.0f) * clampedOpacity;
                if (effectiveAlpha < 0.05f) continue;
                uint8_t outR = static_cast<uint8_t>(sr * effectiveAlpha + br * (1.0f - effectiveAlpha));
                uint8_t outG = static_cast<uint8_t>(sg * effectiveAlpha + bgc * (1.0f - effectiveAlpha));
                uint8_t outB = static_cast<uint8_t>(sb * effectiveAlpha + bb * (1.0f - effectiveAlpha));
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

void Renderer::drawVolcano(float cx, float cy, float zoom, float animTime, bool isErupting, float eruptionProgress) {
    // EN: If volcano sprite sheet is loaded, rasterize authentic pixel-art volcano stages
    // ES: Si la hoja de sprites del volcán está cargada, rasterizar las etapas auténticas en pixel-art
    if (m_volcanoSheetPixels.size() == 1024u * 765u) {
        constexpr int sheetWidth = 1024;
        constexpr int sheetHeight = 765;
        constexpr int cellHeight = 153;

        // EN: Determine target row and column in the 5x5 volcano evolution matrix
        // ES: Determinar la fila y columna objetivo en la matriz 5x5 de evolución del volcán
        int targetRow = 0;
        int targetCol = 4; // EN: Default: Dormant stratovolcano with resting smoke plume / ES: Por defecto: Estratovolcán inactivo con pluma de humo

        if (isErupting) {
            if (eruptionProgress > 0.0f) {
                // EN: Cataclysmic eruption with progression lifecycle (0.0 to 1.0)
                // ES: Erupción cataclísmica con ciclo de vida progresivo (0.0 a 1.0)
                if (eruptionProgress < 0.20f) {
                    // EN: Phase 1: Awakening & heating caldera (Row 1)
                    // ES: Fase 1: Despertar y calentamiento de caldera (Fila 1)
                    targetRow = 1;
                    targetCol = std::clamp(static_cast<int>((eruptionProgress / 0.20f) * 5.0f), 0, 4);
                } else if (eruptionProgress < 0.55f) {
                    // EN: Phase 2: Explosive blast & flying volcanic bombs (Row 2)
                    // ES: Fase 2: Explosión destructiva y bombas volcánicas volantes (Fila 2)
                    targetRow = 2;
                    targetCol = std::clamp(static_cast<int>(((eruptionProgress - 0.20f) / 0.35f) * 5.0f), 0, 4);
                } else if (eruptionProgress < 0.80f) {
                    // EN: Phase 3: Cataclysmic mushroom plume & fire geysers (Row 3)
                    // ES: Fase 3: Hongo piroclástico colosal y géiseres de fuego (Fila 3)
                    targetRow = 3;
                    targetCol = std::clamp(static_cast<int>(((eruptionProgress - 0.55f) / 0.25f) * 5.0f), 0, 4);
                } else {
                    // EN: Phase 4: Cascading lava rivers & base inundation (Row 4)
                    // ES: Fase 4: Ríos de lava en cascada e inundación basal (Fila 4)
                    targetRow = 4;
                    targetCol = std::clamp(static_cast<int>(((eruptionProgress - 0.80f) / 0.20f) * 5.0f), 0, 4);
                }
            } else {
                // EN: Continuous active eruption cycling through blast and lava river frames
                // ES: Erupción activa continua ciclando entre fotogramas de explosión y ríos de lava
                const int activeCycleFrame = static_cast<int>(animTime * 5.5f) % 15;
                if (activeCycleFrame < 5) {
                    targetRow = 2;
                    targetCol = activeCycleFrame;
                } else if (activeCycleFrame < 10) {
                    targetRow = 3;
                    targetCol = activeCycleFrame - 5;
                } else {
                    targetRow = 4;
                    targetCol = activeCycleFrame - 10;
                }
            }
        } else {
            // EN: Dormant state with living gentle smoke breathing from summit
            // ES: Estado inactivo con suave humo viviente respirando en la cumbre
            const int dormantFrame = static_cast<int>(animTime * 1.6f) % 4;
            if (dormantFrame == 0) {
                targetRow = 0;
                targetCol = 4;
            } else if (dormantFrame == 1) {
                targetRow = 1;
                targetCol = 0;
            } else if (dormantFrame == 2) {
                targetRow = 0;
                targetCol = 4;
            } else {
                targetRow = 0;
                targetCol = 3;
            }
        }

        // EN: Compute atlas column offsets and frame widths
        // ES: Calcular desfases de columnas del atlas y anchos de fotogramas
        const int colStartX[5] = {0, 204, 409, 614, 819};
        const int colWidths[5] = {204, 205, 205, 205, 205};
        const int srcStartX = colStartX[targetCol];
        const int srcStartY = targetRow * cellHeight;
        const int frameWidth = colWidths[targetCol];
        const int frameHeight = cellHeight;

        // EN: Scale and anchor volcano sprite to isometric tile footprint
        // ES: Escalar y anclar el sprite del volcán a la huella de la celda isométrica
        const float volcanoScale = std::clamp(zoom * 0.82f, 0.35f, 2.50f);
        const int destWidth = std::max(12, static_cast<int>(frameWidth * volcanoScale));
        const int destHeight = std::max(12, static_cast<int>(frameHeight * volcanoScale));

        const int startScreenX = static_cast<int>(cx) - destWidth / 2;
        const int bottomScreenY = static_cast<int>(cy + 14.0f * zoom);
        const int startScreenY = bottomScreenY - destHeight;

        // EN: 1. Soft elliptical terrain drop shadow at volcano mountain base
        // ES: 1. Sombra elíptica suave sobre el terreno en la base de la montaña
        const int shadowRadiusX = static_cast<int>(destWidth * 0.44f);
        const int shadowRadiusY = static_cast<int>(destHeight * 0.16f);
        const int shadowCenterX = static_cast<int>(cx);
        const int shadowCenterY = bottomScreenY - static_cast<int>(6.0f * zoom);

        for (int offsetY = -shadowRadiusY; offsetY <= shadowRadiusY; ++offsetY) {
            const int targetY = shadowCenterY + offsetY;
            const int frameY = m_height - 1 - targetY;
            if (frameY < 0 || frameY >= m_height) continue;

            for (int offsetX = -shadowRadiusX; offsetX <= shadowRadiusX; ++offsetX) {
                const int targetX = shadowCenterX + offsetX;
                if (targetX < 0 || targetX >= m_width) continue;

                const float normDist = (offsetX * offsetX) / static_cast<float>(shadowRadiusX * shadowRadiusX) +
                                       (offsetY * offsetY) / static_cast<float>(shadowRadiusY * shadowRadiusY);
                if (normDist <= 1.0f) {
                    const uint32_t bgPixel = m_framebuffer[frameY * m_width + targetX];
                    const float shadowFade = 0.55f + 0.45f * normDist;
                    const uint8_t bgR = static_cast<uint8_t>((bgPixel & 0xFF) * shadowFade);
                    const uint8_t bgG = static_cast<uint8_t>(((bgPixel >> 8) & 0xFF) * shadowFade);
                    const uint8_t bgB = static_cast<uint8_t>(((bgPixel >> 16) & 0xFF) * shadowFade);
                    m_framebuffer[frameY * m_width + targetX] = (0xFFu << 24) | (bgB << 16) | (bgG << 8) | bgR;
                }
            }
        }

        // EN: 2. Rasterize volcano sprite with full alpha transparency blending
        // ES: 2. Rasterizar el sprite del volcán con mezcla completa de transparencia alfa
        for (int destY = 0; destY < destHeight; ++destY) {
            const int screenY = startScreenY + destY;
            const int frameY = m_height - 1 - screenY;
            if (frameY < 0 || frameY >= m_height) continue;

            const int srcY = std::clamp(srcStartY + (destY * frameHeight) / destHeight, 0, sheetHeight - 1);

            for (int destX = 0; destX < destWidth; ++destX) {
                const int screenX = startScreenX + destX;
                if (screenX < 0 || screenX >= m_width) continue;

                const int srcX = std::clamp(srcStartX + (destX * frameWidth) / destWidth, 0, sheetWidth - 1);
                const uint32_t srcPixel = m_volcanoSheetPixels[srcY * sheetWidth + srcX];
                const uint8_t srcA = (srcPixel >> 24) & 0xFF;
                if (srcA < 12) continue;

                const uint8_t srcR = srcPixel & 0xFF;
                const uint8_t srcG = (srcPixel >> 8) & 0xFF;
                const uint8_t srcB = (srcPixel >> 16) & 0xFF;

                const float alphaFactor = srcA / 255.0f;
                const uint32_t bgPixel = m_framebuffer[frameY * m_width + screenX];
                const uint8_t bgR = bgPixel & 0xFF;
                const uint8_t bgG = (bgPixel >> 8) & 0xFF;
                const uint8_t bgB = (bgPixel >> 16) & 0xFF;

                const uint8_t outR = static_cast<uint8_t>(srcR * alphaFactor + bgR * (1.0f - alphaFactor));
                const uint8_t outG = static_cast<uint8_t>(srcG * alphaFactor + bgG * (1.0f - alphaFactor));
                const uint8_t outB = static_cast<uint8_t>(srcB * alphaFactor + bgB * (1.0f - alphaFactor));

                m_framebuffer[frameY * m_width + screenX] = (0xFFu << 24) | (outB << 16) | (outG << 8) | outR;
            }
        }

        // EN: 3. Sparkling volcanic embers dancing above erupting summit
        // ES: 3. Chispas y ascuas volcánicas danzando sobre la cumbre en erupción
        if (isErupting) {
            const int sparkBaseX = static_cast<int>(cx);
            const int sparkBaseY = startScreenY + static_cast<int>(destHeight * 0.42f);
            for (int sparkIndex = 0; sparkIndex < 8; ++sparkIndex) {
                const float sparkAngle = sparkIndex * 0.785f + animTime * 3.5f;
                const float sparkDist = (16.0f + std::sin(animTime * 7.0f + sparkIndex) * 12.0f) * zoom;
                const int sparkX = sparkBaseX + static_cast<int>(std::cos(sparkAngle) * sparkDist);
                const int sparkY = sparkBaseY - static_cast<int>(std::abs(std::sin(sparkAngle)) * sparkDist * 1.5f);
                const int frameY = m_height - 1 - sparkY;
                if (sparkX >= 0 && sparkX < m_width && frameY >= 0 && frameY < m_height) {
                    const ColorRGB emberColor = (sparkIndex % 2 == 0) ? ColorRGB{255, 235, 90} : ColorRGB{255, 120, 20};
                    m_framebuffer[frameY * m_width + sparkX] = emberColor.toRGBA();
                }
            }
        }
        return;
    }

    // EN: Procedural cone fallback if sprite asset is not available
    // ES: Cono procedural de reserva si el asset de sprite no está disponible
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

    // 3B. Fuente de erupción de magma sobre la caldera (sprite de géiser volcánico animado)
    if (!m_volcanoEruptionPixels.empty()) {
        const int erupFrame = (static_cast<int>(animTime * 6.5f) % 2);
        const int erupW = static_cast<int>(34.0f * scale);
        const int erupH = static_cast<int>(46.0f * scale);
        const int startEX = calX - erupW / 2;
        const int startEY = calY - erupH + static_cast<int>(4.0f * scale);
        for (int dy = 0; dy < erupH; ++dy) {
            const int targetY = startEY + dy;
            const int frameY = m_height - 1 - targetY;
            if (frameY < 0 || frameY >= m_height) continue;
            const int srcY = std::clamp((dy * 128) / erupH, 0, 127);
            for (int dx = 0; dx < erupW; ++dx) {
                const int targetX = startEX + dx;
                if (targetX < 0 || targetX >= m_width) continue;
                const int srcX = std::clamp(erupFrame * 128 + (dx * 128) / erupW, 0, 255);
                const uint32_t srcPix = m_volcanoEruptionPixels[srcY * 256 + srcX];
                const uint8_t srcA = (srcPix >> 24) & 0xFF;
                if (srcA < 15) continue;
                const float aF = srcA / 255.0f;
                const uint32_t bgPix = m_framebuffer[frameY * m_width + targetX];
                const uint8_t outR = static_cast<uint8_t>((srcPix & 0xFF) * aF + (bgPix & 0xFF) * (1.0f - aF));
                const uint8_t outG = static_cast<uint8_t>(((srcPix >> 8) & 0xFF) * aF + ((bgPix >> 8) & 0xFF) * (1.0f - aF));
                const uint8_t outB = static_cast<uint8_t>(((srcPix >> 16) & 0xFF) * aF + ((bgPix >> 16) & 0xFF) * (1.0f - aF));
                m_framebuffer[frameY * m_width + targetX] = (0xFFu << 24) | (outB << 16) | (outG << 8) | outR;
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

// EN: Render authentic animated swirling whirlpool vortex on flooded or deep water cells with 2:1 isometric ellipse masking
// ES: Renderizar remolino de vórtice animado auténtico en celdas inundadas o de agua profunda con máscara elíptica isométrica 2:1
void Renderer::drawWhirlpool(float centerX, float centerY, float zoom, float animTime, float intensity) {
    if (m_waterWhirlpoolPixels.size() != 510u * 85u) return;

    constexpr int frameWidth = 85;
    constexpr int frameHeight = 85;
    constexpr int sheetWidth = 510;
    constexpr int frameCount = 6;
    const int frameIndex = static_cast<int>(animTime * 10.0f) % frameCount;
    const int frameStartX = frameIndex * frameWidth;

    const int destWidth = std::max(8, static_cast<int>(48.0f * zoom));
    const int destHeight = std::max(4, static_cast<int>(24.0f * zoom));
    const int startScreenX = static_cast<int>(centerX) - destWidth / 2;
    const int startScreenY = static_cast<int>(centerY) - destHeight / 2;

    const float halfDestWidth = static_cast<float>(destWidth) * 0.5f;
    const float halfDestHeight = static_cast<float>(destHeight) * 0.5f;

    for (int deltaY = 0; deltaY < destHeight; ++deltaY) {
        const int targetY = startScreenY + deltaY;
        const int frameY = m_height - 1 - targetY;
        if (frameY < 0 || frameY >= m_height) continue;

        const float normalizedOffsetY = (static_cast<float>(deltaY) + 0.5f - halfDestHeight) / halfDestHeight;
        const float normYSquared = normalizedOffsetY * normalizedOffsetY;
        if (normYSquared >= 1.0f) continue;

        const int srcY = std::clamp((deltaY * frameHeight) / destHeight, 0, frameHeight - 1);

        for (int deltaX = 0; deltaX < destWidth; ++deltaX) {
            const int targetX = startScreenX + deltaX;
            if (targetX < 0 || targetX >= m_width) continue;

            // EN: Calculate 2:1 isometric elliptical distance to prevent harsh rectangular boundary cutoff
            // ES: Calcular distancia elíptica isométrica 2:1 para prevenir corte rectangular rígido
            const float normalizedOffsetX = (static_cast<float>(deltaX) + 0.5f - halfDestWidth) / halfDestWidth;
            const float ellipticalDistanceSquared = normalizedOffsetX * normalizedOffsetX + normYSquared;
            if (ellipticalDistanceSquared >= 1.0f) continue;

            const int srcX = std::clamp(frameStartX + (deltaX * frameWidth) / destWidth, 0, sheetWidth - 1);
            const uint32_t srcPixel = m_waterWhirlpoolPixels[srcY * sheetWidth + srcX];
            const uint8_t srcAlpha = (srcPixel >> 24) & 0xFF;
            if (srcAlpha < 15) continue;

            // EN: Cubic Hermite radial smoothstep falloff for a soft gradient seamlessly merging into terrain
            // ES: Atenuación suave radial cúbica Hermite para un degradado suave fusionado con el terreno
            const float ellipticalDistance = std::sqrt(ellipticalDistanceSquared);
            const float edgeTransition = std::clamp((1.0f - ellipticalDistance) / 0.40f, 0.0f, 1.0f);
            const float smoothFalloff = edgeTransition * edgeTransition * (3.0f - 2.0f * edgeTransition);

            const float alphaFactor = (srcAlpha / 255.0f) * smoothFalloff * std::clamp(intensity, 0.2f, 1.0f);
            if (alphaFactor < 0.015f) continue;

            const uint32_t backgroundPixel = m_framebuffer[frameY * m_width + targetX];
            const uint8_t outputRed = static_cast<uint8_t>((srcPixel & 0xFF) * alphaFactor + (backgroundPixel & 0xFF) * (1.0f - alphaFactor));
            const uint8_t outputGreen = static_cast<uint8_t>(((srcPixel >> 8) & 0xFF) * alphaFactor + ((backgroundPixel >> 8) & 0xFF) * (1.0f - alphaFactor));
            const uint8_t outputBlue = static_cast<uint8_t>(((srcPixel >> 16) & 0xFF) * alphaFactor + ((backgroundPixel >> 16) & 0xFF) * (1.0f - alphaFactor));

            m_framebuffer[frameY * m_width + targetX] = (0xFFu << 24) | (outputBlue << 16) | (outputGreen << 8) | outputRed;
        }
    }
}

// EN: Render authentic animated water droplet splashes and expanding foam bubbles with soft radial mask
// ES: Renderizar salpicaduras de gotas de agua animadas y burbujas de espuma con máscara radial suave
void Renderer::drawWaterSplash(float centerX, float centerY, float zoom, float animTime, float scale) {
    if (m_waterSplashPixels.size() != 425u * 85u) return;

    constexpr int frameWidth = 85;
    constexpr int frameHeight = 85;
    constexpr int sheetWidth = 425;
    constexpr int frameCount = 5;
    const int frameIndex = static_cast<int>(animTime * 12.0f) % frameCount;
    const int frameStartX = frameIndex * frameWidth;

    const int destWidth = std::max(6, static_cast<int>(28.0f * zoom * scale));
    const int destHeight = std::max(6, static_cast<int>(28.0f * zoom * scale));
    const int startScreenX = static_cast<int>(centerX) - destWidth / 2;
    const int startScreenY = static_cast<int>(centerY) - destHeight / 2;

    const float halfSplashWidth = static_cast<float>(destWidth) * 0.5f;
    const float halfSplashHeight = static_cast<float>(destHeight) * 0.5f;

    for (int deltaY = 0; deltaY < destHeight; ++deltaY) {
        const int targetY = startScreenY + deltaY;
        const int frameY = m_height - 1 - targetY;
        if (frameY < 0 || frameY >= m_height) continue;

        const float normalizedSplashOffsetY = (static_cast<float>(deltaY) + 0.5f - halfSplashHeight) / halfSplashHeight;
        const float splashNormYSquared = normalizedSplashOffsetY * normalizedSplashOffsetY;
        if (splashNormYSquared >= 1.0f) continue;

        const int srcY = std::clamp((deltaY * frameHeight) / destHeight, 0, frameHeight - 1);

        for (int deltaX = 0; deltaX < destWidth; ++deltaX) {
            const int targetX = startScreenX + deltaX;
            if (targetX < 0 || targetX >= m_width) continue;

            const float normalizedSplashOffsetX = (static_cast<float>(deltaX) + 0.5f - halfSplashWidth) / halfSplashWidth;
            const float splashDistanceSquared = normalizedSplashOffsetX * normalizedSplashOffsetX + splashNormYSquared;
            if (splashDistanceSquared >= 1.0f) continue;

            const int srcX = std::clamp(frameStartX + (deltaX * frameWidth) / destWidth, 0, sheetWidth - 1);
            const uint32_t srcPixel = m_waterSplashPixels[srcY * sheetWidth + srcX];
            const uint8_t srcAlpha = (srcPixel >> 24) & 0xFF;
            if (srcAlpha < 15) continue;

            const float splashDistance = std::sqrt(splashDistanceSquared);
            const float edgeTransition = std::clamp((1.0f - splashDistance) / 0.30f, 0.0f, 1.0f);
            const float splashSmoothFalloff = edgeTransition * edgeTransition * (3.0f - 2.0f * edgeTransition);

            const float alphaFactor = (srcAlpha / 255.0f) * splashSmoothFalloff;
            if (alphaFactor < 0.015f) continue;

            const uint32_t backgroundPixel = m_framebuffer[frameY * m_width + targetX];
            const uint8_t outputRed = static_cast<uint8_t>((srcPixel & 0xFF) * alphaFactor + (backgroundPixel & 0xFF) * (1.0f - alphaFactor));
            const uint8_t outputGreen = static_cast<uint8_t>(((srcPixel >> 8) & 0xFF) * alphaFactor + ((backgroundPixel >> 8) & 0xFF) * (1.0f - alphaFactor));
            const uint8_t outputBlue = static_cast<uint8_t>(((srcPixel >> 16) & 0xFF) * alphaFactor + ((backgroundPixel >> 16) & 0xFF) * (1.0f - alphaFactor));

            m_framebuffer[frameY * m_width + targetX] = (0xFFu << 24) | (outputBlue << 16) | (outputGreen << 8) | outputRed;
        }
    }
}

// EN: Render dynamic pixel-art 2.5D fire tongues, billowing smoke, and flying embers
// ES: Renderizar lenguas de fuego pixel-art 2.5D dinámicas, humo ondulante y ascuas volantes
void Renderer::drawFireEffect(float cx, float cy, float intensity, float zoom, float animTime, float windVx, float windVy) {
    if (intensity <= 0.01f) return;

    const float fireScale = std::clamp(zoom, 0.40f, 2.50f) * std::clamp(intensity, 0.25f, 1.30f);
    const float baseWidth = 24.0f * fireScale;
    const float baseHeight = 32.0f * fireScale;

    // EN: Wind advection lean in screen projection space
    // ES: Inclinación por advección de viento en el espacio de proyección de pantalla
    const float windLeanX = (windVx - windVy) * 16.0f * fireScale;
    const float windLeanY = (windVx + windVy) * 8.0f * fireScale;

    // EN: 1. Pulsating thermal ground glow (warm orange-red ellipse at flame base)
    // ES: 1. Resplandor térmico en suelo (elipse naranja-roja cálida en la base de la llama)
    const int glowRadiusX = static_cast<int>(baseWidth * 0.90f);
    const int glowRadiusY = static_cast<int>(baseWidth * 0.45f);
    const int glowCenterX = static_cast<int>(cx);
    const int glowCenterY = static_cast<int>(cy + 2.0f * fireScale);

    for (int deltaY = -glowRadiusY; deltaY <= glowRadiusY; ++deltaY) {
        const int pixelY = glowCenterY + deltaY;
        const int frameY = m_height - 1 - pixelY;
        if (frameY < 0 || frameY >= m_height) continue;

        for (int deltaX = -glowRadiusX; deltaX <= glowRadiusX; ++deltaX) {
            const int pixelX = glowCenterX + deltaX;
            if (pixelX < 0 || pixelX >= m_width) continue;

            const float normalizedDistance = (deltaX * deltaX) / static_cast<float>(glowRadiusX * glowRadiusX) +
                                             (deltaY * deltaY) / static_cast<float>(glowRadiusY * glowRadiusY);
            if (normalizedDistance <= 1.0f) {
                const float glowAlpha = (1.0f - normalizedDistance) * (0.40f + 0.20f * std::sin(animTime * 6.0f));
                const uint32_t currentBg = m_framebuffer[frameY * m_width + pixelX];
                const uint8_t bgR = currentBg & 0xFF;
                const uint8_t bgG = (currentBg >> 8) & 0xFF;
                const uint8_t bgB = (currentBg >> 16) & 0xFF;

                const uint8_t outR = static_cast<uint8_t>(std::min(255.0f, bgR * (1.0f - glowAlpha) + 250.0f * glowAlpha));
                const uint8_t outG = static_cast<uint8_t>(std::min(255.0f, bgG * (1.0f - glowAlpha) + 110.0f * glowAlpha));
                const uint8_t outB = static_cast<uint8_t>(std::min(255.0f, bgB * (1.0f - glowAlpha) + 20.0f * glowAlpha));
                m_framebuffer[frameY * m_width + pixelX] = (0xFFu << 24) | (outB << 16) | (outG << 8) | outR;
            }
        }
    }

    // EN: 2. Authentic Pixel-Art Fire Sprite Rendering (Wildfire & Flame Sheets)
    // ES: 2. Renderizado de sprites de fuego pixel-art auténticos (Hojas de Incendio y Llamas)
    const bool isRagingInferno = (intensity > 0.55f);
    const bool hasSprite = isRagingInferno ? (!m_wildfirePixels.empty()) : (!m_flamePixels.empty());

    if (hasSprite) {
        const std::vector<uint32_t>& spriteSheet = isRagingInferno ? m_wildfirePixels : m_flamePixels;
        const int sheetW = isRagingInferno ? 512 : 384;
        const int frameW = isRagingInferno ? 128 : 64;
        const int frameH = 128;
        const int frameCount = isRagingInferno ? 4 : 6;
        const float animSpeed = isRagingInferno ? 8.5f : 11.0f;
        const int frameIndex = static_cast<int>(animTime * animSpeed) % frameCount;
        const int frameStartX = frameIndex * frameW;

        // Destination dimensions scaled by zoom and fire intensity
        const float destScale = isRagingInferno ? (0.46f * fireScale) : (0.42f * fireScale);
        const int destWidth = std::max(4, static_cast<int>(frameW * destScale));
        const int destHeight = std::max(6, static_cast<int>(frameH * destScale));

        const int startScreenX = static_cast<int>(cx + windLeanX * 0.35f) - destWidth / 2;
        const int startScreenY = static_cast<int>(cy + windLeanY * 0.35f) - destHeight + static_cast<int>(4.0f * fireScale);

        for (int dy = 0; dy < destHeight; ++dy) {
            const int targetY = startScreenY + dy;
            const int frameY = m_height - 1 - targetY;
            if (frameY < 0 || frameY >= m_height) continue;

            const int srcY = std::clamp((dy * frameH) / destHeight, 0, frameH - 1);

            // Progressive wind lean higher up in the flame
            const float progressHeight = 1.0f - (static_cast<float>(dy) / static_cast<float>(destHeight));
            const int dynamicWindX = static_cast<int>(windLeanX * progressHeight * progressHeight);

            for (int dx = 0; dx < destWidth; ++dx) {
                const int targetX = startScreenX + dx + dynamicWindX;
                if (targetX < 0 || targetX >= m_width) continue;

                const int srcX = std::clamp(frameStartX + (dx * frameW) / destWidth, 0, sheetW - 1);
                const uint32_t srcPixel = spriteSheet[srcY * sheetW + srcX];
                const uint8_t srcA = (srcPixel >> 24) & 0xFF;
                if (srcA < 15) continue;

                const uint8_t srcR = srcPixel & 0xFF;
                const uint8_t srcG = (srcPixel >> 8) & 0xFF;
                const uint8_t srcB = (srcPixel >> 16) & 0xFF;

                const float alphaFactor = (srcA / 255.0f);
                const uint32_t bgPixel = m_framebuffer[frameY * m_width + targetX];
                const uint8_t bgR = bgPixel & 0xFF;
                const uint8_t bgG = (bgPixel >> 8) & 0xFF;
                const uint8_t bgB = (bgPixel >> 16) & 0xFF;

                const uint8_t outR = static_cast<uint8_t>(srcR * alphaFactor + bgR * (1.0f - alphaFactor));
                const uint8_t outG = static_cast<uint8_t>(srcG * alphaFactor + bgG * (1.0f - alphaFactor));
                const uint8_t outB = static_cast<uint8_t>(srcB * alphaFactor + bgB * (1.0f - alphaFactor));

                m_framebuffer[frameY * m_width + targetX] = (0xFFu << 24) | (outB << 16) | (outG << 8) | outR;
            }
        }
    } else {
        // Fallback: procedural flames if sprite assets are missing
        constexpr int FLAME_TONGUE_COUNT = 5;
        constexpr float TONGUE_OFFSETS[FLAME_TONGUE_COUNT] = {-0.65f, -0.30f, 0.0f, 0.35f, 0.65f};
        constexpr float TONGUE_HEIGHT_FACTORS[FLAME_TONGUE_COUNT] = {0.70f, 0.95f, 1.15f, 0.90f, 0.65f};
        constexpr float TONGUE_PHASE_OFFSETS[FLAME_TONGUE_COUNT] = {0.0f, 2.1f, 4.3f, 1.2f, 3.4f};

        for (int tongueIndex = 0; tongueIndex < FLAME_TONGUE_COUNT; ++tongueIndex) {
            const float tongueBaseOffset = TONGUE_OFFSETS[tongueIndex] * (baseWidth * 0.70f);
            const float tonguePhase = TONGUE_PHASE_OFFSETS[tongueIndex];
            const float flickerHeight = baseHeight * TONGUE_HEIGHT_FACTORS[tongueIndex] *
                                        (0.80f + 0.25f * std::sin(animTime * 10.0f + tonguePhase) +
                                         0.15f * std::cos(animTime * 17.0f + tonguePhase * 1.5f));
            const int tongueTotalHeight = std::max(4, static_cast<int>(flickerHeight));

            const float tongueOriginX = cx + tongueBaseOffset;
            const float tongueOriginY = cy;

            for (int stepY = 0; stepY < tongueTotalHeight; ++stepY) {
                const float progressY = static_cast<float>(stepY) / static_cast<float>(tongueTotalHeight);
                const float currentFlameWidth = (1.0f - progressY * 0.85f) * (4.5f * fireScale);
                const int halfFlameWidth = std::max(1, static_cast<int>(currentFlameWidth * 0.5f));

                const float swayOffset = std::sin(animTime * 12.0f + progressY * 5.0f + tonguePhase) * (2.2f * fireScale * progressY);
                const float currentCenterX = tongueOriginX + windLeanX * (progressY * progressY) + swayOffset;
                const float currentCenterY = tongueOriginY - (static_cast<float>(stepY) - windLeanY * (progressY * progressY));

                const int pixelY = static_cast<int>(currentCenterY);
                const int frameY = m_height - 1 - pixelY;
                if (frameY < 0 || frameY >= m_height) continue;

                const int startScanX = static_cast<int>(currentCenterX) - halfFlameWidth;
                const int endScanX = static_cast<int>(currentCenterX) + halfFlameWidth;

                for (int scanX = startScanX; scanX <= endScanX; ++scanX) {
                    if (scanX < 0 || scanX >= m_width) continue;

                    const float horizontalDistance = std::abs(static_cast<float>(scanX) - currentCenterX) / std::max(1.0f, static_cast<float>(halfFlameWidth));
                    if (horizontalDistance > 1.0f) continue;

                    uint8_t flameR = 255;
                    uint8_t flameG = 200;
                    uint8_t flameB = 40;

                    if (progressY < 0.28f && horizontalDistance < 0.55f) {
                        flameR = 255;
                        flameG = static_cast<uint8_t>(235 + 20 * (1.0f - horizontalDistance));
                        flameB = static_cast<uint8_t>(120 + 80 * (1.0f - progressY / 0.28f));
                    } else if (progressY < 0.65f) {
                        const float transitionRatio = (progressY - 0.28f) / 0.37f;
                        flameR = 255;
                        flameG = static_cast<uint8_t>(180.0f * (1.0f - transitionRatio) + 70.0f * transitionRatio);
                        flameB = static_cast<uint8_t>(30.0f * (1.0f - transitionRatio));
                    } else {
                        const float tipRatio = (progressY - 0.65f) / 0.35f;
                        flameR = static_cast<uint8_t>(240.0f * (1.0f - tipRatio) + 160.0f * tipRatio);
                        flameG = static_cast<uint8_t>(60.0f * (1.0f - tipRatio) + 15.0f * tipRatio);
                        flameB = 10;
                    }

                    const float edgeAlpha = (1.0f - horizontalDistance * 0.40f) * (1.0f - progressY * 0.30f);
                    const uint32_t currentBg = m_framebuffer[frameY * m_width + scanX];
                    const uint8_t bgR = currentBg & 0xFF;
                    const uint8_t bgG = (currentBg >> 8) & 0xFF;
                    const uint8_t bgB = (currentBg >> 16) & 0xFF;

                    const uint8_t finalR = static_cast<uint8_t>(bgR * (1.0f - edgeAlpha) + flameR * edgeAlpha);
                    const uint8_t finalG = static_cast<uint8_t>(bgG * (1.0f - edgeAlpha) + flameG * edgeAlpha);
                    const uint8_t finalB = static_cast<uint8_t>(bgB * (1.0f - edgeAlpha) + flameB * edgeAlpha);

                    m_framebuffer[frameY * m_width + scanX] = (0xFFu << 24) | (finalB << 16) | (finalG << 8) | finalR;
                }
            }
        }
    }

    // EN: 3. Billowing smoke puffs rising above flame peaks
    // ES: 3. Bocanadas de humo ondulante ascendiendo sobre las cumbres de las llamas
    constexpr int SMOKE_PUFF_COUNT = 4;
    for (int smokeIndex = 0; smokeIndex < SMOKE_PUFF_COUNT; ++smokeIndex) {
        const float smokeCycleTime = animTime * 1.6f + smokeIndex * 1.57f;
        const float smokeElevation = std::fmod(smokeCycleTime * 20.0f * fireScale, 45.0f * fireScale);
        const float smokeNormalizedLife = smokeElevation / (45.0f * fireScale);
        const float smokeDriftX = windLeanX * (1.0f + smokeNormalizedLife * 1.5f) +
                                  std::sin(smokeCycleTime * 2.2f) * (4.0f * fireScale);
        const float smokeCenterScreenX = cx + smokeDriftX;
        const float smokeCenterScreenY = cy - baseHeight * 0.85f - smokeElevation;

        const int smokeRadius = std::max(2, static_cast<int>((3.0f + smokeNormalizedLife * 7.0f) * fireScale));
        const float smokeAlpha = (1.0f - smokeNormalizedLife) * 0.42f;

        for (int deltaY = -smokeRadius; deltaY <= smokeRadius; ++deltaY) {
            const int pixelY = static_cast<int>(smokeCenterScreenY) + deltaY;
            const int frameY = m_height - 1 - pixelY;
            if (frameY < 0 || frameY >= m_height) continue;

            for (int deltaX = -smokeRadius; deltaX <= smokeRadius; ++deltaX) {
                const int pixelX = static_cast<int>(smokeCenterScreenX) + deltaX;
                if (pixelX < 0 || pixelX >= m_width) continue;

                const float smokeDistanceNormalized = (deltaX * deltaX + deltaY * deltaY) / static_cast<float>(smokeRadius * smokeRadius);
                if (smokeDistanceNormalized <= 1.0f) {
                    const float pixelSmokeAlpha = (1.0f - smokeDistanceNormalized) * smokeAlpha;
                    const uint32_t currentBg = m_framebuffer[frameY * m_width + pixelX];
                    const uint8_t bgR = currentBg & 0xFF;
                    const uint8_t bgG = (currentBg >> 8) & 0xFF;
                    const uint8_t bgB = (currentBg >> 16) & 0xFF;

                    constexpr ColorRGB smokeDarkColor{44, 40, 38};
                    const uint8_t outR = static_cast<uint8_t>(bgR * (1.0f - pixelSmokeAlpha) + smokeDarkColor.r * pixelSmokeAlpha);
                    const uint8_t outG = static_cast<uint8_t>(bgG * (1.0f - pixelSmokeAlpha) + smokeDarkColor.g * pixelSmokeAlpha);
                    const uint8_t outB = static_cast<uint8_t>(bgB * (1.0f - pixelSmokeAlpha) + smokeDarkColor.b * pixelSmokeAlpha);
                    m_framebuffer[frameY * m_width + pixelX] = (0xFFu << 24) | (outB << 16) | (outG << 8) | outR;
                }
            }
        }
    }

    // EN: 4. Floating glowing ember sparks pop and drift downwind
    // ES: 4. Ascuas incandescentes flotantes que chisporrotean y se desplazan con el viento
    constexpr int EMBER_COUNT = 6;
    for (int emberIndex = 0; emberIndex < EMBER_COUNT; ++emberIndex) {
        const float emberPhase = emberIndex * 1.256f;
        const float emberTime = animTime * 2.8f + emberPhase;
        const float emberProgress = std::fmod(emberTime, 1.0f);
        const float emberX = cx + windLeanX * (1.2f + emberProgress * 1.4f) +
                             std::sin(emberTime * 5.0f) * (8.0f * fireScale);
        const float emberY = cy - (baseHeight * 0.40f) - emberProgress * (50.0f * fireScale);

        const int sparkPixelX = static_cast<int>(emberX);
        const int sparkPixelY = static_cast<int>(emberY);
        const int frameY = m_height - 1 - sparkPixelY;

        if (sparkPixelX >= 0 && sparkPixelX < m_width && frameY >= 0 && frameY < m_height) {
            const ColorRGB emberColor = (emberIndex % 2 == 0) ? ColorRGB{255, 235, 120} : ColorRGB{255, 130, 30};
            m_framebuffer[frameY * m_width + sparkPixelX] = emberColor.toRGBA();
            if (sparkPixelX + 1 < m_width && emberProgress < 0.6f) {
                m_framebuffer[frameY * m_width + (sparkPixelX + 1)] = emberColor.toRGBA();
            }
        }
    }
}

// EN: Render rising billowing steam puffs when water torrents quench fiery cells
// ES: Renderizar nubes de vapor ascendente y ondulante cuando los torrentes de agua apagan celdas en llamas
void Renderer::drawSteamEffect(float screenCenterX, float screenCenterY, float steamTimer, float zoom, float animTime) {
    if (steamTimer <= 0.02f) return;

    const float steamScale = std::clamp(zoom, 0.40f, 2.50f);
    const float normalizedTimer = std::clamp(steamTimer / 2.5f, 0.10f, 1.0f);
    const float windVelocityX = m_cloudSystem ? m_cloudSystem->getWindX() : 0.20f;
    const float windVelocityY = m_cloudSystem ? m_cloudSystem->getWindY() : 0.05f;
    const float windDriftX = (windVelocityX - windVelocityY) * 14.0f * steamScale;

    // EN: Rasterize multiple billowing expanding steam puffs rising into the air
    // ES: Rasterizar múltiples bocanadas de vapor ondulantes en expansión que ascienden al aire
    constexpr int STEAM_PUFF_COUNT = 4;
    for (int puffIndex = 0; puffIndex < STEAM_PUFF_COUNT; ++puffIndex) {
        const float puffPhase = std::fmod(animTime * 1.6f + static_cast<float>(puffIndex) * 0.25f, 1.0f);
        const float puffHeight = puffPhase * 32.0f * steamScale;
        const float puffRadius = (4.0f + puffPhase * 12.0f) * steamScale;
        const float puffCenterX = screenCenterX + (static_cast<float>(puffIndex) - 1.5f) * 5.0f * steamScale + windDriftX * puffPhase;
        const float puffCenterY = screenCenterY - puffHeight;
        const float puffAlpha = (1.0f - puffPhase) * normalizedTimer * 0.50f;

        const int startPixelX = std::max(0, static_cast<int>(std::floor(puffCenterX - puffRadius)));
        const int endPixelX = std::min(m_width - 1, static_cast<int>(std::ceil(puffCenterX + puffRadius)));
        const int startPixelY = std::max(0, static_cast<int>(std::floor(puffCenterY - puffRadius)));
        const int endPixelY = std::min(m_height - 1, static_cast<int>(std::ceil(puffCenterY + puffRadius)));

        const float invRadiusSquared = 1.0f / (puffRadius * puffRadius + 0.001f);

        for (int pixelY = startPixelY; pixelY <= endPixelY; ++pixelY) {
            const int frameY = m_height - 1 - pixelY;
            if (frameY < 0 || frameY >= m_height) continue;
            const float deltaPixelY = static_cast<float>(pixelY) - puffCenterY;

            for (int pixelX = startPixelX; pixelX <= endPixelX; ++pixelX) {
                const float deltaPixelX = static_cast<float>(pixelX) - puffCenterX;
                const float distanceSquared = deltaPixelX * deltaPixelX + deltaPixelY * deltaPixelY;
                if (distanceSquared <= puffRadius * puffRadius) {
                    const float radialFalloff = 1.0f - distanceSquared * invRadiusSquared;
                    const float finalAlpha = radialFalloff * puffAlpha;

                    const uint32_t currentBg = m_framebuffer[frameY * m_width + pixelX];
                    const uint8_t bgR = currentBg & 0xFF;
                    const uint8_t bgG = (currentBg >> 8) & 0xFF;
                    const uint8_t bgB = (currentBg >> 16) & 0xFF;

                    const uint8_t outR = static_cast<uint8_t>(std::min(255.0f, bgR * (1.0f - finalAlpha) + 230.0f * finalAlpha));
                    const uint8_t outG = static_cast<uint8_t>(std::min(255.0f, bgG * (1.0f - finalAlpha) + 238.0f * finalAlpha));
                    const uint8_t outB = static_cast<uint8_t>(std::min(255.0f, bgB * (1.0f - finalAlpha) + 248.0f * finalAlpha));
                    m_framebuffer[frameY * m_width + pixelX] = (0xFFu << 24) | (outB << 16) | (outG << 8) | outR;
                }
            }
        }
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
    else if (power == DivinePowerType::Rain) col = ColorRGB{90, 190, 255};
    else if (power == DivinePowerType::Deluge) col = ColorRGB{35, 120, 240};
    else if (power == DivinePowerType::AtomicBomb) col = ColorRGB{255, 235, 90};

    // EN: Render clean single-tile targeting reticle on the hovered cell without area grid squares
    // ES: Renderizar retícula de objetivo limpia de una sola celda sobre la casilla señalada sin la cuadrícula de cuadrados de área
    const float groundElevation = map.getCell(hoverCell.x, hoverCell.y).elevation;
    const Vec2 targetPoint = camera.gridTileCenterScreen(hoverCell.x, hoverCell.y, groundElevation);
    drawDiamondOutline(targetPoint.x, targetPoint.y, halfW, halfH, col);
    drawDiamondOutline(targetPoint.x, targetPoint.y, halfW + 1.0f, halfH + 1.0f, ColorRGB{255, 255, 255});
    drawDiamondOutline(targetPoint.x, targetPoint.y, halfW * 0.5f, halfH * 0.5f, col);
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
            // EN: Render authentic 2.5D animated pixel-art tornado sprite sheet (6 frames, 64x128)
            // ES: Renderizar hoja de sprites auténtica de tornado pixel-art 2.5D animado (6 cuadros, 64x128)
            const float elev = map.isValid(fx.gridX, fx.gridY) ? map.getCell(fx.gridX, fx.gridY).elevation : 1.0f;
            const Vec2 center = camera.gridToScreen(fx.worldX, fx.worldY, elev);

            constexpr int TORNADO_SHEET_WIDTH = 384;
            constexpr int TORNADO_FRAME_WIDTH = 64;
            constexpr int TORNADO_FRAME_HEIGHT = 128;
            constexpr int TORNADO_FRAME_COUNT = 6;
            constexpr float TORNADO_ANIM_SPEED = 12.0f;

            const float tornadoScale = std::clamp(zoom * 1.15f, 0.45f, 3.20f);
            const int destW = std::max(8, static_cast<int>(TORNADO_FRAME_WIDTH * tornadoScale));
            const int destH = std::max(16, static_cast<int>(TORNADO_FRAME_HEIGHT * tornadoScale));

            if (!m_tornadoSheetPixels.empty()) {
                const int frameIdx = static_cast<int>(fx.timer * TORNADO_ANIM_SPEED) % TORNADO_FRAME_COUNT;
                const int frameStartX = frameIdx * TORNADO_FRAME_WIDTH;

                // Base anchor on ground contact point
                const int startScreenX = static_cast<int>(center.x) - destW / 2;
                const int startScreenY = static_cast<int>(center.y) - destH + static_cast<int>(14.0f * tornadoScale);

                // Wind sway dynamic lean
                const float windLeanX = (m_cloudSystem ? m_cloudSystem->getWindX() : 0.35f) * 20.0f * tornadoScale;

                for (int dy = 0; dy < destH; ++dy) {
                    const int targetY = startScreenY + dy;
                    const int frameY = m_height - 1 - targetY;
                    if (frameY < 0 || frameY >= m_height) continue;

                    const int srcY = std::clamp((dy * TORNADO_FRAME_HEIGHT) / destH, 0, TORNADO_FRAME_HEIGHT - 1);
                    const float heightProgress = 1.0f - (static_cast<float>(dy) / static_cast<float>(destH));
                    const int dynamicLeanX = static_cast<int>(windLeanX * heightProgress * heightProgress);

                    for (int dx = 0; dx < destW; ++dx) {
                        const int targetX = startScreenX + dx + dynamicLeanX;
                        if (targetX < 0 || targetX >= m_width) continue;

                        const int srcX = std::clamp(frameStartX + (dx * TORNADO_FRAME_WIDTH) / destW, 0, TORNADO_SHEET_WIDTH - 1);
                        const uint32_t srcPixel = m_tornadoSheetPixels[srcY * TORNADO_SHEET_WIDTH + srcX];
                        const uint8_t srcAlpha = (srcPixel >> 24) & 0xFF;
                        if (srcAlpha < 15) continue;

                        const float alphaFactor = static_cast<float>(srcAlpha) / 255.0f;
                        const uint32_t bgPixel = m_framebuffer[frameY * m_width + targetX];
                        const uint8_t bgR = bgPixel & 0xFF;
                        const uint8_t bgG = (bgPixel >> 8) & 0xFF;
                        const uint8_t bgB = (bgPixel >> 16) & 0xFF;

                        const uint8_t outR = static_cast<uint8_t>((srcPixel & 0xFF) * alphaFactor + bgR * (1.0f - alphaFactor));
                        const uint8_t outG = static_cast<uint8_t>(((srcPixel >> 8) & 0xFF) * alphaFactor + bgG * (1.0f - alphaFactor));
                        const uint8_t outB = static_cast<uint8_t>(((srcPixel >> 16) & 0xFF) * alphaFactor + bgB * (1.0f - alphaFactor));

                        m_framebuffer[frameY * m_width + targetX] = (0xFFu << 24) | (outB << 16) | (outG << 8) | outR;
                    }
                }
            }

            const int textPosY = static_cast<int>(center.y - destH - 10.0f);
            drawText(static_cast<int>(center.x - 55), textPosY, "¡TORNADO DEVASTADOR!", ColorRGB{200, 225, 255}, 1);

        } else if (fx.type == DivinePowerType::Earthquake) {
            float elev = map.isValid(fx.gridX, fx.gridY) ? map.getCell(fx.gridX, fx.gridY).elevation : 1.0f;
            Vec2 center = camera.gridToScreen(fx.worldX, fx.worldY, elev);

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

                // EN: 2. Coordinates of the bolide in mid-air (diagonal trajectory from the north-eastern sky)
                // ES: 2. Coordenadas del bólido en el aire (trayectoria diagonal desde el cielo nororiental)
                constexpr float METEOR_TRAJECTORY_DELTA_X = -240.0f;
                constexpr float METEOR_TRAJECTORY_DELTA_Y = 420.0f;

                const float meteorPosX = groundCenter.x + invProg * (-METEOR_TRAJECTORY_DELTA_X) * zoom;
                const float meteorPosY = groundCenter.y - invProg * METEOR_TRAJECTORY_DELTA_Y * zoom;

                // EN: Compute trajectory vector and incoming beam ("haz") unit vectors
                // ES: Calcular el vector de trayectoria y vectores unitarios del haz entrante
                const float hazDeltaX = -METEOR_TRAJECTORY_DELTA_X * zoom;
                const float hazDeltaY = -METEOR_TRAJECTORY_DELTA_Y * zoom;
                const float hazLength = std::sqrt(hazDeltaX * hazDeltaX + hazDeltaY * hazDeltaY);
                const float hazUnitX = hazDeltaX / (hazLength > 0.001f ? hazLength : 1.0f);
                const float hazUnitY = hazDeltaY / (hazLength > 0.001f ? hazLength : 1.0f);
                const int meteorDisplaySize = std::max(16, static_cast<int>((48.0f + prog * 18.0f) * zoom));

                // EN: Bolide fireball head oriented dynamically so flame tips trail directly into the incoming beam
                // ES: Cabeza de bola de fuego del bólido orientada dinámicamente para que las puntas sigan directamente el haz
                if (!m_meteorGiantPixels.empty()) {
                    // EN: Alternate between giant and fireball sprite frames for atmospheric turbulence flicker
                    // ES: Alternar entre cuadros de sprite gigante y bola de fuego para el parpadeo por turbulencia atmosférica
                    const bool useAlternateFrame = (!m_meteorFireballPixels.empty()) && (static_cast<int>(fx.timer * 14.0f) % 2 == 1);
                    const std::vector<uint32_t>& activeFireballPixels = useAlternateFrame ? m_meteorFireballPixels : m_meteorGiantPixels;

                    // EN: In source 128x128 texture, vector from core (80, 77) to flame tips (21, 20) is (-59, -57)
                    // ES: En la textura origen 128x128, el vector del núcleo (80, 77) a las puntas (21, 20) es (-59, -57)
                    constexpr float SPRITE_CENTER_COORD = 64.0f;
                    const float spriteTipsVectorX = -59.0f;
                    const float spriteTipsVectorY = -57.0f;
                    const float spriteTipsAngle = std::atan2(spriteTipsVectorY, spriteTipsVectorX);
                    const float hazBeamAngle = std::atan2(hazUnitY, hazUnitX);

                    // EN: Rotation angle aligns sprite tips with the backward beam vector
                    // ES: El ángulo de rotación alinea las puntas del sprite con el vector hacia atrás del haz
                    const float alignmentRotationAngle = hazBeamAngle - spriteTipsAngle;
                    const float cosRotation = std::cos(alignmentRotationAngle);
                    const float sinRotation = std::sin(alignmentRotationAngle);

                    const float halfDisplaySize = meteorDisplaySize * 0.5f;
                    const int boundingHalfSpan = static_cast<int>(halfDisplaySize * 1.45f) + 1;

                    const int minDestinationY = std::max(0, static_cast<int>(meteorPosY - boundingHalfSpan));
                    const int maxDestinationY = std::min(m_height - 1, static_cast<int>(meteorPosY + boundingHalfSpan));
                    const int minDestinationX = std::max(0, static_cast<int>(meteorPosX - boundingHalfSpan));
                    const int maxDestinationX = std::min(m_width - 1, static_cast<int>(meteorPosX + boundingHalfSpan));

                    for (int destScreenY = minDestinationY; destScreenY <= maxDestinationY; ++destScreenY) {
                        const int frameBufferRowY = m_height - 1 - destScreenY;
                        if (frameBufferRowY < 0 || frameBufferRowY >= m_height) continue;
                        const float deltaScreenY = static_cast<float>(destScreenY) - meteorPosY;

                        for (int destScreenX = minDestinationX; destScreenX <= maxDestinationX; ++destScreenX) {
                            const float deltaScreenX = static_cast<float>(destScreenX) - meteorPosX;

                            // EN: Inverse rotation from screen offset to sprite local coordinate
                            // ES: Rotación inversa desde el desplazamiento en pantalla a la coordenada local del sprite
                            const float normalizedSpriteX = (cosRotation * deltaScreenX + sinRotation * deltaScreenY) / halfDisplaySize;
                            const float normalizedSpriteY = (-sinRotation * deltaScreenX + cosRotation * deltaScreenY) / halfDisplaySize;

                            const int sourcePixelX = static_cast<int>(SPRITE_CENTER_COORD + normalizedSpriteX * SPRITE_CENTER_COORD);
                            const int sourcePixelY = static_cast<int>(SPRITE_CENTER_COORD + normalizedSpriteY * SPRITE_CENTER_COORD);

                            if (sourcePixelX < 0 || sourcePixelX >= 128 || sourcePixelY < 0 || sourcePixelY >= 128) {
                                continue;
                            }

                            const uint32_t sourcePixelColor = activeFireballPixels[sourcePixelY * 128 + sourcePixelX];
                            const uint8_t sourceAlpha = (sourcePixelColor >> 24) & 0xFF;
                            if (sourceAlpha < 15) continue;

                            const float alphaFraction = static_cast<float>(sourceAlpha) / 255.0f;
                            const uint32_t backgroundPixel = m_framebuffer[frameBufferRowY * m_width + destScreenX];
                            const uint8_t outputRed = static_cast<uint8_t>((sourcePixelColor & 0xFF) * alphaFraction + (backgroundPixel & 0xFF) * (1.0f - alphaFraction));
                            const uint8_t outputGreen = static_cast<uint8_t>(((sourcePixelColor >> 8) & 0xFF) * alphaFraction + ((backgroundPixel >> 8) & 0xFF) * (1.0f - alphaFraction));
                            const uint8_t outputBlue = static_cast<uint8_t>(((sourcePixelColor >> 16) & 0xFF) * alphaFraction + ((backgroundPixel >> 16) & 0xFF) * (1.0f - alphaFraction));

                            m_framebuffer[frameBufferRowY * m_width + destScreenX] =
                                (0xFFu << 24) | (outputBlue << 16) | (outputGreen << 8) | outputRed;
                        }
                    }
                } else {
                    // EN: Fallback geometric representation if raw sprite textures are unavailable
                    // ES: Representación geométrica de reserva si las texturas de sprites no están disponibles
                    const int fallbackSize = static_cast<int>((14.0f + prog * 5.0f) * zoom);
                    drawDiamondOutline(meteorPosX, meteorPosY, fallbackSize * 2.2f, fallbackSize * 1.1f, ColorRGB{255, 110, 15});
                    drawDiamondOutline(meteorPosX, meteorPosY, fallbackSize * 1.6f, fallbackSize * 0.8f, ColorRGB{255, 210, 45});
                    drawRect(static_cast<int>(meteorPosX - fallbackSize / 2), static_cast<int>(meteorPosY - fallbackSize / 2),
                             fallbackSize, fallbackSize, ColorRGB{48, 26, 22});
                    drawRect(static_cast<int>(meteorPosX - fallbackSize / 3), static_cast<int>(meteorPosY - fallbackSize / 3),
                             fallbackSize * 2 / 3, fallbackSize * 2 / 3, ColorRGB{255, 255, 250});
                }

                const int warningTextPosY = static_cast<int>(groundCenter.y - 28.0f * zoom);
                drawText(static_cast<int>(groundCenter.x - 52), warningTextPosY, "¡METEORITO INMINENTE!", ColorRGB{255, 80, 20}, 1);

            } else {
                // ==========================================
                // FASE 2: DETONACIÓN, ONDA DE CHOQUE Y CRÁTER
                // ==========================================
                float tExp = fx.timer - IMPACT_TIME;

                // EN: 1. Supersonic airblast waves on ground expanding to physical 50m radius (10 cells)
                // ES: 1. Ondas de choque supersónicas en suelo expandiéndose a radio físico de 50m (10 celdas)
                // EN: 2. Render authentic high-detail meteor impact crater sprite (deep bowl & basalt rim)
                // ES: 2. Renderizar sprite auténtico de cráter de impacto de meteorito (cuenco profundo y borde de basalto)
                drawCraterSprite(groundCenter.x, groundCenter.y, 5, zoom, 1.0f);

                // 5. Bola de fuego y estallido de detonación (primeros 0.8s post impacto)
                if (tExp < 0.8f) {
                    if (!m_flameBurstPixels.empty()) {
                        const int burstFrame = (tExp < 0.40f) ? 0 : 1;
                        const int burstSize = static_cast<int>((75.0f + tExp * 70.0f) * zoom);
                        const int startBX = static_cast<int>(groundCenter.x) - burstSize / 2;
                        const int startBY = static_cast<int>(groundCenter.y) - burstSize + static_cast<int>(14.0f * zoom);
                        for (int dy = 0; dy < burstSize; ++dy) {
                            const int targetY = startBY + dy;
                            const int frameY = m_height - 1 - targetY;
                            if (frameY < 0 || frameY >= m_height) continue;
                            const int srcY = std::clamp((dy * 128) / burstSize, 0, 127);
                            for (int dx = 0; dx < burstSize; ++dx) {
                                const int targetX = startBX + dx;
                                if (targetX < 0 || targetX >= m_width) continue;
                                const int srcX = std::clamp(burstFrame * 128 + (dx * 128) / burstSize, 0, 255);
                                const uint32_t srcPix = m_flameBurstPixels[srcY * 256 + srcX];
                                const uint8_t srcA = (srcPix >> 24) & 0xFF;
                                if (srcA < 15) continue;
                                const float aF = (srcA / 255.0f) * (1.0f - tExp / 0.8f);
                                const uint32_t bgPix = m_framebuffer[frameY * m_width + targetX];
                                const uint8_t outR = static_cast<uint8_t>((srcPix & 0xFF) * aF + (bgPix & 0xFF) * (1.0f - aF));
                                const uint8_t outG = static_cast<uint8_t>(((srcPix >> 8) & 0xFF) * aF + ((bgPix >> 8) & 0xFF) * (1.0f - aF));
                                const uint8_t outB = static_cast<uint8_t>(((srcPix >> 16) & 0xFF) * aF + ((bgPix >> 16) & 0xFF) * (1.0f - aF));
                                m_framebuffer[frameY * m_width + targetX] = (0xFFu << 24) | (outB << 16) | (outG << 8) | outR;
                            }
                        }
                    } else {
                        float blastProg = tExp / 0.6f;
                        float blastR = blastProg * 75.0f * zoom;
                        ColorRGB fireCol{255, static_cast<uint8_t>(255 * (1.0f - blastProg)), static_cast<uint8_t>(100 * (1.0f - blastProg))};
                        drawDiamondOutline(groundCenter.x, groundCenter.y, blastR, blastR * 0.5f, fireCol);
                        drawDiamondOutline(groundCenter.x, groundCenter.y, blastR * 0.7f, blastR * 0.35f, ColorRGB{255, 255, 255});
                    }
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

            // 1. Ondas de temblor sísmico y cráter de caldera incandescente en la base
            if (!m_firePitPixels.empty()) {
                const int pitW = static_cast<int>(56.0f * zoom);
                const int pitH = static_cast<int>(38.0f * zoom);
                const int startPitX = static_cast<int>(groundCenter.x) - pitW / 2;
                const int startPitY = static_cast<int>(groundCenter.y) - pitH / 2;
                for (int dy = 0; dy < pitH; ++dy) {
                    const int targetY = startPitY + dy;
                    const int frameY = m_height - 1 - targetY;
                    if (frameY < 0 || frameY >= m_height) continue;
                    const int srcY = std::clamp((dy * 128) / pitH, 0, 127);
                    for (int dx = 0; dx < pitW; ++dx) {
                        const int targetX = startPitX + dx;
                        if (targetX < 0 || targetX >= m_width) continue;
                        const int srcX = std::clamp((dx * 128) / pitW, 0, 127);
                        const uint32_t srcPix = m_firePitPixels[srcY * 128 + srcX];
                        const uint8_t srcA = (srcPix >> 24) & 0xFF;
                        if (srcA < 15) continue;
                        const float aF = srcA / 255.0f;
                        const uint32_t bgPix = m_framebuffer[frameY * m_width + targetX];
                        const uint8_t outR = static_cast<uint8_t>((srcPix & 0xFF) * aF + (bgPix & 0xFF) * (1.0f - aF));
                        const uint8_t outG = static_cast<uint8_t>(((srcPix >> 8) & 0xFF) * aF + ((bgPix >> 8) & 0xFF) * (1.0f - aF));
                        const uint8_t outB = static_cast<uint8_t>(((srcPix >> 16) & 0xFF) * aF + ((bgPix >> 16) & 0xFF) * (1.0f - aF));
                        m_framebuffer[frameY * m_width + targetX] = (0xFFu << 24) | (outB << 16) | (outG << 8) | outR;
                    }
                }
            }

            // 1B. Erupción de géiser de magma en la cumbre del volcán
            if (!m_volcanoEruptionPixels.empty()) {
                const int erupFrame = (static_cast<int>(fx.timer * 7.5f) % 2);
                const int erupW = static_cast<int>(60.0f * zoom);
                const int erupH = static_cast<int>(82.0f * zoom);
                const int startEX = static_cast<int>(summit.x) - erupW / 2;
                const int startEY = static_cast<int>(summit.y) - erupH + static_cast<int>(10.0f * zoom);
                for (int dy = 0; dy < erupH; ++dy) {
                    const int targetY = startEY + dy;
                    const int frameY = m_height - 1 - targetY;
                    if (frameY < 0 || frameY >= m_height) continue;
                    const int srcY = std::clamp((dy * 128) / erupH, 0, 127);
                    for (int dx = 0; dx < erupW; ++dx) {
                        const int targetX = startEX + dx;
                        if (targetX < 0 || targetX >= m_width) continue;
                        const int srcX = std::clamp(erupFrame * 128 + (dx * 128) / erupW, 0, 255);
                        const uint32_t srcPix = m_volcanoEruptionPixels[srcY * 256 + srcX];
                        const uint8_t srcA = (srcPix >> 24) & 0xFF;
                        if (srcA < 15) continue;
                        const float aF = srcA / 255.0f;
                        const uint32_t bgPix = m_framebuffer[frameY * m_width + targetX];
                        const uint8_t outR = static_cast<uint8_t>((srcPix & 0xFF) * aF + (bgPix & 0xFF) * (1.0f - aF));
                        const uint8_t outG = static_cast<uint8_t>(((srcPix >> 8) & 0xFF) * aF + ((bgPix >> 8) & 0xFF) * (1.0f - aF));
                        const uint8_t outB = static_cast<uint8_t>(((srcPix >> 16) & 0xFF) * aF + ((bgPix >> 16) & 0xFF) * (1.0f - aF));
                        m_framebuffer[frameY * m_width + targetX] = (0xFFu << 24) | (outB << 16) | (outG << 8) | outR;
                    }
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
        } else if (fx.type == DivinePowerType::Rain) {
            // =========================================================================
            // PODER SAGRADO / CLIMÁTICO: LLUVIA BENDITA (Precipitación, Ondas y Brillo de Gotas)
            // =========================================================================
            const float cellElevation = map.isValid(fx.gridX, fx.gridY) ? map.getCell(fx.gridX, fx.gridY).elevation : 1.0f;
            const Vec2 groundCenter = camera.gridTileCenterScreen(fx.worldX, fx.worldY, cellElevation);

            // 1. Ondas concéntricas de agua en el suelo
            const float pulseFactor = 0.5f + 0.5f * std::sin(fx.timer * 4.0f + fx.seed);
            const float outerRadius = fx.radius * 22.0f * zoom;
            drawDiamondOutline(groundCenter.x, groundCenter.y, outerRadius, outerRadius * 0.5f, ColorRGB{90, 190, 255});
            drawDiamondOutline(groundCenter.x, groundCenter.y, outerRadius * 0.65f, outerRadius * 0.325f, ColorRGB{160, 225, 255});
            drawDiamondOutline(groundCenter.x, groundCenter.y, outerRadius * (0.25f + 0.15f * pulseFactor), outerRadius * (0.125f + 0.075f * pulseFactor), ColorRGB{220, 245, 255});

            // 2. Trazas atmosféricas de gotas de lluvia cayendo en diagonal
            constexpr int RAINDROP_COUNT = 32;
            const float windSpeedX = m_cloudSystem ? m_cloudSystem->getWindX() : 0.25f;
            const float rainLean = windSpeedX * 18.0f * zoom;
            for (int dropIndex = 0; dropIndex < RAINDROP_COUNT; ++dropIndex) {
                const float dropSeed = fx.seed + static_cast<float>(dropIndex) * 1.37f;
                const float dropOffsetX = (std::fmod(dropSeed * 73.1f, 1.0f) * 2.0f - 1.0f) * outerRadius;
                const float dropPhase = std::fmod(fx.timer * 2.5f + static_cast<float>(dropIndex) * 0.08f, 1.0f);
                const float dropCurrentY = groundCenter.y - (1.0f - dropPhase) * 110.0f * zoom;
                const float dropCurrentX = groundCenter.x + dropOffsetX + rainLean * dropPhase;

                const int startX = static_cast<int>(dropCurrentX);
                const int startY = static_cast<int>(dropCurrentY);
                const int endX = static_cast<int>(dropCurrentX + rainLean * 0.12f);
                const int endY = static_cast<int>(dropCurrentY + 8.0f * zoom);

                drawLine(startX, startY, endX, endY, ColorRGB{180, 230, 255});

                if (dropPhase > 0.88f) {
                    const float splashSpread = (dropPhase - 0.88f) * 45.0f * zoom;
                    drawDiamondOutline(dropCurrentX, groundCenter.y, splashSpread, splashSpread * 0.4f, ColorRGB{200, 240, 255});
                }
            }

            const int textPosY = static_cast<int>(groundCenter.y - 36.0f * zoom);
            drawText(static_cast<int>(groundCenter.x - 42), textPosY, "¡LLUVIA BENDITA!", ColorRGB{140, 220, 255}, 1);
        } else if (fx.type == DivinePowerType::Deluge) {
            // =========================================================================
            // CATÁSTROFE / CATACLISMO: GRAN DILUVIO (Torrentes, Trombas de Agua y Turbulencia)
            // =========================================================================
            const float cellElevation = map.isValid(fx.gridX, fx.gridY) ? map.getCell(fx.gridX, fx.gridY).elevation : 1.0f;
            const Vec2 groundCenter = camera.gridTileCenterScreen(fx.worldX, fx.worldY, cellElevation);

            // 1. Anillos concéntricos de oleaje impetuoso y turbulencia hidráulica
            const float pulseFactor = 0.5f + 0.5f * std::sin(fx.timer * 7.0f + fx.seed);
            const float delugeRadius = (fx.radius * 24.0f + pulseFactor * 8.0f) * zoom;
            drawDiamondOutline(groundCenter.x, groundCenter.y, delugeRadius, delugeRadius * 0.5f, ColorRGB{35, 120, 240});
            drawDiamondOutline(groundCenter.x, groundCenter.y, delugeRadius * 0.80f, delugeRadius * 0.40f, ColorRGB{70, 175, 255});
            drawDiamondOutline(groundCenter.x, groundCenter.y, delugeRadius * 0.55f, delugeRadius * 0.275f, ColorRGB{180, 235, 255});
            drawDiamondOutline(groundCenter.x, groundCenter.y, delugeRadius * 0.30f, delugeRadius * 0.15f, ColorRGB{240, 250, 255});

            // 2. Trombas de agua densa y cortinas torrenciales de lluvia
            constexpr int DELUGE_SHEET_COUNT = 64;
            const float windSpeedX = m_cloudSystem ? m_cloudSystem->getWindX() : 0.35f;
            const float rainLean = windSpeedX * 28.0f * zoom;
            for (int sheetIndex = 0; sheetIndex < DELUGE_SHEET_COUNT; ++sheetIndex) {
                const float sheetSeed = fx.seed + static_cast<float>(sheetIndex) * 2.19f;
                const float sheetOffsetX = (std::fmod(sheetSeed * 97.3f, 1.0f) * 2.0f - 1.0f) * delugeRadius;
                const float sheetPhase = std::fmod(fx.timer * 3.8f + static_cast<float>(sheetIndex) * 0.04f, 1.0f);
                const float currentY = groundCenter.y - (1.0f - sheetPhase) * 140.0f * zoom;
                const float currentX = groundCenter.x + sheetOffsetX + rainLean * sheetPhase;

                const int startX = static_cast<int>(currentX);
                const int startY = static_cast<int>(currentY);
                const int endX = static_cast<int>(currentX + rainLean * 0.15f);
                const int endY = static_cast<int>(currentY + 14.0f * zoom);

                const ColorRGB dropColor = (sheetIndex % 3 == 0) ? ColorRGB{230, 245, 255} : ColorRGB{80, 160, 255};
                drawLine(startX, startY, endX, endY, dropColor);
                if (sheetIndex % 2 == 0) {
                    drawLine(startX + 1, startY, endX + 1, endY, ColorRGB{140, 210, 255});
                }

                if (sheetPhase > 0.85f) {
                    const float splashRadius = (sheetPhase - 0.85f) * 55.0f * zoom;
                    drawDiamondOutline(currentX, groundCenter.y, splashRadius, splashRadius * 0.45f, ColorRGB{210, 245, 255});
                    drawWaterSplash(currentX, groundCenter.y, zoom, fx.timer + sheetSeed, 0.40f);
                    drawRect(static_cast<int>(currentX - 1), static_cast<int>(groundCenter.y - 1), 2, 2, ColorRGB{255, 255, 255});
                }
            }

            // EN: Central oceanic whirlpool vortex swirling at the deluge epicenter
            // ES: Vórtice de remolino oceánico central girando en el epicentro del diluvio
            drawWhirlpool(groundCenter.x, groundCenter.y, zoom * 1.75f, fx.timer, 1.0f);

            const int textPosY = static_cast<int>(groundCenter.y - 45.0f * zoom);
            drawText(static_cast<int>(groundCenter.x - 70), textPosY, "¡GRAN DILUVIO CATACLÍSMICO!", ColorRGB{80, 200, 255}, 1);
        } else if (fx.type == DivinePowerType::AtomicBomb) {
            // =========================================================================
            // EN: CATACLYSM: ATOMIC BOMB [FUCKINGBOMB] - Nuclear Mushroom Cloud & Shockwave
            // ES: CATACLISMO: BOMBA ATÓMICA [FUCKINGBOMB] - Hongo Atómico y Onda Supersónica
            // =========================================================================
            const float cellElevation = map.isValid(fx.gridX, fx.gridY) ? map.getCell(fx.gridX, fx.gridY).elevation : 1.0f;
            const Vec2 groundCenter = camera.gridTileCenterScreen(fx.worldX, fx.worldY, cellElevation);
            const float blastProgress = std::clamp(fx.timer / fx.duration, 0.0f, 1.0f);
            const float timeSeconds = fx.timer;

            // -------------------------------------------------------------------------
            // HELPER: Rasterize an isometric semi-transparent filled ellipse puff
            // ES: Rasterizar un penacho elíptico isométrico semitransparente con mezcla alfa
            // -------------------------------------------------------------------------
            auto drawTranslucentDustPuff = [&](float puffCenterX, float puffCenterY,
                                               float radiusX, float radiusY,
                                               ColorRGB puffColor, float maxAlpha) {
                if (radiusX <= 1.5f || radiusY <= 1.0f || maxAlpha <= 0.01f) return;
                const int minBoundX = std::max(0, static_cast<int>(std::floor(puffCenterX - radiusX)));
                const int maxBoundX = std::min(m_width - 1, static_cast<int>(std::ceil(puffCenterX + radiusX)));
                const int minBoundY = std::max(0, static_cast<int>(std::floor(puffCenterY - radiusY)));
                const int maxBoundY = std::min(m_height - 1, static_cast<int>(std::ceil(puffCenterY + radiusY)));

                const float invRadXSq = 1.0f / (radiusX * radiusX);
                const float invRadYSq = 1.0f / (radiusY * radiusY);

                for (int screenY = minBoundY; screenY <= maxBoundY; ++screenY) {
                    const int frameY = m_height - 1 - screenY;
                    if (frameY < 0 || frameY >= m_height) continue;
                    const int rowOffset = frameY * m_width;
                    const float deltaY = static_cast<float>(screenY) - puffCenterY;
                    const float normYSq = (deltaY * deltaY) * invRadYSq;
                    if (normYSq > 1.0f) continue;

                    for (int screenX = minBoundX; screenX <= maxBoundX; ++screenX) {
                        const float deltaX = static_cast<float>(screenX) - puffCenterX;
                        const float normDistSq = (deltaX * deltaX) * invRadXSq + normYSq;
                        if (normDistSq <= 1.0f) {
                            const float edgeFalloff = 1.0f - normDistSq;
                            const float pixelAlpha = std::clamp(maxAlpha * edgeFalloff * 1.35f, 0.0f, 0.90f);
                            const float invAlpha = 1.0f - pixelAlpha;

                            const uint32_t bgPixel = m_framebuffer[rowOffset + screenX];
                            const uint8_t outR = static_cast<uint8_t>((bgPixel & 0xFF) * invAlpha + puffColor.r * pixelAlpha);
                            const uint8_t outG = static_cast<uint8_t>(((bgPixel >> 8) & 0xFF) * invAlpha + puffColor.g * pixelAlpha);
                            const uint8_t outB = static_cast<uint8_t>(((bgPixel >> 16) & 0xFF) * invAlpha + puffColor.b * pixelAlpha);
                            m_framebuffer[rowOffset + screenX] = (0xFFu << 24) |
                                                                 (static_cast<uint32_t>(outB) << 16) |
                                                                 (static_cast<uint32_t>(outG) << 8)  |
                                                                  static_cast<uint32_t>(outR);
                        }
                    }
                }
            };

            // -------------------------------------------------------------------------
            // HELPER: Rasterize a perfectly smooth radial ellipse outline with N segments
            // ES: Rasterizar un contorno de elipse radial perfectamente suave con N segmentos
            // -------------------------------------------------------------------------
            auto drawRadialEllipseOutline = [&](float centerX, float centerY,
                                                float radiusX, float radiusY,
                                                ColorRGB ellipseColor, int totalSegments = 64) {
                if (radiusX < 2.0f || radiusY < 1.0f) return;
                float previousPointX = centerX + radiusX;
                float previousPointY = centerY;
                for (int segmentStep = 1; segmentStep <= totalSegments; ++segmentStep) {
                    const float angle = (static_cast<float>(segmentStep) / totalSegments) * 6.2831853f;
                    const float currentPointX = centerX + std::cos(angle) * radiusX;
                    const float currentPointY = centerY + std::sin(angle) * radiusY;
                    drawLine(static_cast<int>(previousPointX), static_cast<int>(previousPointY),
                             static_cast<int>(currentPointX), static_cast<int>(currentPointY),
                             ellipseColor);
                    previousPointX = currentPointX;
                    previousPointY = currentPointY;
                }
            };

            // -------------------------------------------------------------------------
            // 1. SUPERSONIC GROUND SHOCKWAVE (Continuous Translucent Radial Mach Front)
            // -------------------------------------------------------------------------
            // EN: Supersonic blast wave expands radially in all directions across isometric terrain at physical Little Boy Mach speed
            // ES: La onda expansiva supersónica se propaga radialmente en todas direcciones a velocidad Mach física de Little Boy
            constexpr float SHOCKWAVE_SPEED_TILES = LittleBoyScale::SHOCKWAVE_SPEED_CELLS_PER_SEC; // 50.0f cells per second (~250 m/s Mach wave)
            const float currentWaveRadiusTiles = std::min(fx.radius, timeSeconds * SHOCKWAVE_SPEED_TILES);
            const float waveRadiusX = currentWaveRadiusTiles * 64.0f * zoom;
            const float waveRadiusY = waveRadiusX * 0.5f;

            if (waveRadiusX > 4.0f && timeSeconds < 4.5f) {
                const float waveAlpha = std::clamp(1.0f - (timeSeconds / 4.5f), 0.0f, 1.0f);
                const float bandThickness = (14.0f + std::min(3.0f, timeSeconds) * 4.0f) * zoom;
                const float innerRadiusX = std::max(1.0f, waveRadiusX - bandThickness);
                const float innerRadiusY = innerRadiusX * 0.5f;

                // EN: Render continuous luminous translucent supersonic shockwave front with perfect radial curvature
                // ES: Renderizar frente de choque supersónico translúcido, continuo y luminoso con curvatura radial perfecta
                const int minY = std::max(0, static_cast<int>(std::floor(groundCenter.y - waveRadiusY)));
                const int maxY = std::min(m_height - 1, static_cast<int>(std::ceil(groundCenter.y + waveRadiusY)));
                const float invWaveRadYSq = 1.0f / (waveRadiusY * waveRadiusY);
                const float invInnerRadYSq = (innerRadiusY > 0.0f) ? (1.0f / (innerRadiusY * innerRadiusY)) : 1.0f;
                const float innerRatio = innerRadiusX / waveRadiusX;

                for (int screenY = minY; screenY <= maxY; ++screenY) {
                    const int frameY = m_height - 1 - screenY;
                    if (frameY < 0 || frameY >= m_height) continue;
                    const int rowOffset = frameY * m_width;

                    const float deltaY = std::abs(static_cast<float>(screenY) - groundCenter.y);
                    const float normYSqOuter = (deltaY * deltaY) * invWaveRadYSq;
                    if (normYSqOuter >= 1.0f) continue;

                    // Perfect smooth radial ellipse half-spans
                    const float spanOuter = waveRadiusX * std::sqrt(1.0f - normYSqOuter);
                    const float normYSqInner = (deltaY * deltaY) * invInnerRadYSq;
                    const float spanInner = (normYSqInner < 1.0f) ? (innerRadiusX * std::sqrt(1.0f - normYSqInner)) : 0.0f;

                    const int leftOut = std::clamp(static_cast<int>(groundCenter.x - spanOuter), 0, m_width - 1);
                    const int rightOut = std::clamp(static_cast<int>(groundCenter.x + spanOuter), 0, m_width - 1);
                    const int leftIn = std::clamp(static_cast<int>(groundCenter.x - spanInner), 0, m_width - 1);
                    const int rightIn = std::clamp(static_cast<int>(groundCenter.x + spanInner), 0, m_width - 1);

                    auto blendWavePixel = [&](int pixelScreenX) {
                        const float normX = std::abs(static_cast<float>(pixelScreenX) - groundCenter.x) / waveRadiusX;
                        const float normY = deltaY / waveRadiusY;
                        const float radialDistance = std::sqrt(normX * normX + normY * normY);
                        const float fraction = std::clamp((radialDistance - innerRatio) / std::max(0.001f, 1.0f - innerRatio), 0.0f, 1.0f);

                        const float localAlpha = std::clamp((0.25f + 0.65f * fraction) * waveAlpha, 0.0f, 0.88f);
                        const float invAlpha = 1.0f - localAlpha;

                        // Radial color gradient: Leading edge white/cyan -> middle golden plasma -> trailing dusty amber
                        ColorRGB waveColor;
                        if (fraction > 0.70f) {
                            const float subFrac = (fraction - 0.70f) / 0.30f;
                            waveColor = ColorRGB{
                                static_cast<uint8_t>(220 + 35 * subFrac),
                                static_cast<uint8_t>(235 + 20 * subFrac),
                                static_cast<uint8_t>(255)
                            };
                        } else if (fraction > 0.35f) {
                            const float subFrac = (fraction - 0.35f) / 0.35f;
                            waveColor = ColorRGB{
                                static_cast<uint8_t>(255),
                                static_cast<uint8_t>(180 + 55 * subFrac),
                                static_cast<uint8_t>(60 + 175 * subFrac)
                            };
                        } else {
                            const float subFrac = fraction / 0.35f;
                            waveColor = ColorRGB{
                                static_cast<uint8_t>(180 + 75 * subFrac),
                                static_cast<uint8_t>(120 + 60 * subFrac),
                                static_cast<uint8_t>(40 + 20 * subFrac)
                            };
                        }

                        const uint32_t bgPixel = m_framebuffer[rowOffset + pixelScreenX];
                        const uint8_t outR = static_cast<uint8_t>((bgPixel & 0xFF) * invAlpha + waveColor.r * localAlpha);
                        const uint8_t outG = static_cast<uint8_t>(((bgPixel >> 8) & 0xFF) * invAlpha + waveColor.g * localAlpha);
                        const uint8_t outB = static_cast<uint8_t>(((bgPixel >> 16) & 0xFF) * invAlpha + waveColor.b * localAlpha);
                        m_framebuffer[rowOffset + pixelScreenX] = (0xFFu << 24) |
                                                                 (static_cast<uint32_t>(outB) << 16) |
                                                                 (static_cast<uint32_t>(outG) << 8)  |
                                                                  static_cast<uint32_t>(outR);
                    };

                    // Left radial flank
                    for (int pixelX = leftOut; pixelX <= leftIn; ++pixelX) {
                        blendWavePixel(pixelX);
                    }
                    // Right radial flank
                    for (int pixelX = rightIn; pixelX <= rightOut; ++pixelX) {
                        blendWavePixel(pixelX);
                    }
                }

                // EN: Supersonic dynamic radial streak rays radiating outward from epicenter in all 360 degrees
                // ES: Rayos dinámicos supersónicos radiales irradiando hacia afuera en los 360 grados
                if (timeSeconds < 2.5f) {
                    constexpr int RADIAL_STREAK_COUNT = 48;
                    for (int streakIdx = 0; streakIdx < RADIAL_STREAK_COUNT; ++streakIdx) {
                        const float streakAngle = (static_cast<float>(streakIdx) / RADIAL_STREAK_COUNT) * 6.2831853f;
                        const float innerFraction = 0.22f + 0.12f * std::sin(static_cast<float>(streakIdx) * 3.1f + timeSeconds * 8.0f);
                        const float streakStartX = groundCenter.x + std::cos(streakAngle) * (waveRadiusX * innerFraction);
                        const float streakStartY = groundCenter.y + std::sin(streakAngle) * (waveRadiusY * innerFraction);
                        const float streakEndX = groundCenter.x + std::cos(streakAngle) * (waveRadiusX * 0.98f);
                        const float streakEndY = groundCenter.y + std::sin(streakAngle) * (waveRadiusY * 0.98f);
                        const ColorRGB streakColor{static_cast<uint8_t>(255 * waveAlpha),
                                                   static_cast<uint8_t>(220 * waveAlpha),
                                                   static_cast<uint8_t>(140 * waveAlpha)};
                        drawLine(static_cast<int>(streakStartX), static_cast<int>(streakStartY),
                                 static_cast<int>(streakEndX), static_cast<int>(streakEndY), streakColor);
                    }
                }

                // EN: 3D atmospheric spherical condensation shock dome arching into the sky
                // ES: Cúpula esférica 3D de condensación de choque atmosférica arqueándose en el cielo
                if (timeSeconds < 2.8f) {
                    const float domeAlpha = (1.0f - (timeSeconds / 2.8f));
                    const float domeHeight = waveRadiusX * 0.90f;

                    // Concentric atmospheric compression latitude rings
                    for (int ringIdx = 1; ringIdx <= 3; ++ringIdx) {
                        const float altFraction = static_cast<float>(ringIdx) * 0.26f;
                        const float ringRadX = waveRadiusX * std::sqrt(std::max(0.0f, 1.0f - altFraction * altFraction));
                        const float ringRadY = ringRadX * 0.5f;
                        const float ringCenterY = groundCenter.y - altFraction * domeHeight;
                        const ColorRGB domeRingColor{
                            static_cast<uint8_t>(200 * domeAlpha),
                            static_cast<uint8_t>(235 * domeAlpha),
                            static_cast<uint8_t>(255 * domeAlpha)
                        };
                        drawRadialEllipseOutline(groundCenter.x, ringCenterY, ringRadX, ringRadY, domeRingColor, 56);
                    }

                    // Trailing surface Wilson condensation ring right behind the Mach front
                    const float wilsonRadX = waveRadiusX * 0.93f;
                    const float wilsonRadY = wilsonRadX * 0.5f;
                    const ColorRGB surfaceWilsonCol{
                        static_cast<uint8_t>(210 * domeAlpha),
                        static_cast<uint8_t>(245 * domeAlpha),
                        static_cast<uint8_t>(255 * domeAlpha)
                    };
                    drawRadialEllipseOutline(groundCenter.x, groundCenter.y - 4.0f * zoom, wilsonRadX, wilsonRadY, surfaceWilsonCol, 64);
                    drawRadialEllipseOutline(groundCenter.x, groundCenter.y - 4.0f * zoom, wilsonRadX + 1.0f, wilsonRadY + 0.5f, surfaceWilsonCol, 64);
                }

                // EN: Colossal rolling base surge: organic translucent churning wall of dust and soil vortices in full radial circle
                // ES: Oleada de base turbulenta colosal: muro de vórtices de polvo y tierra translúcidos en círculo radial completo
                constexpr int DUST_PUFF_COUNT = 72;
                for (int dustIndex = 0; dustIndex < DUST_PUFF_COUNT; ++dustIndex) {
                    const float dustAngle = (static_cast<float>(dustIndex) / DUST_PUFF_COUNT) * 6.2831853f;
                    const float perturbation = 0.93f + 0.12f * std::sin(static_cast<float>(dustIndex) * 3.7f + fx.seed + timeSeconds * 3.5f);
                    const float dustDistX = waveRadiusX * perturbation;
                    const float dustDistY = waveRadiusY * perturbation;
                    const float verticalRise = (std::sin(static_cast<float>(dustIndex) * 2.1f + timeSeconds * 4.0f) * 6.0f + 4.0f) * zoom;
                    const float dustScreenX = groundCenter.x + std::cos(dustAngle) * dustDistX;
                    const float dustScreenY = groundCenter.y + std::sin(dustAngle) * dustDistY - verticalRise;
                    const float puffRadiusX = (12.0f + std::min(4.0f, timeSeconds) * 4.5f + std::sin(static_cast<float>(dustIndex) * 2.5f) * 3.0f) * zoom;
                    const float puffRadiusY = puffRadiusX * 0.55f;

                    // Multi-pass volumetric shading: shadow base -> midtone body -> sunlit crest
                    const ColorRGB shadowPuff{75, 55, 42};
                    const ColorRGB midtonePuff{155, 130, 102};
                    const ColorRGB highlightPuff{220, 202, 175};

                    drawTranslucentDustPuff(dustScreenX, dustScreenY + 2.0f * zoom, puffRadiusX * 1.15f, puffRadiusY * 1.15f, shadowPuff, 0.40f * waveAlpha);
                    drawTranslucentDustPuff(dustScreenX, dustScreenY, puffRadiusX, puffRadiusY, midtonePuff, 0.50f * waveAlpha);
                    drawTranslucentDustPuff(dustScreenX, dustScreenY - 2.5f * zoom, puffRadiusX * 0.65f, puffRadiusY * 0.65f, highlightPuff, 0.45f * waveAlpha);
                }
            }

            // -------------------------------------------------------------------------
            // 2. COLOSSAL NUCLEAR IMPACT CRATER SPRITE (Authentic 224px Colossal Basin)
            // -------------------------------------------------------------------------
            // EN: Render authentic colossal nuclear impact crater sprite on ground surface ("nada más")
            // ES: Renderizar sprite auténtico de cráter de impacto nuclear colosal sobre el suelo ("nada más")
            drawCraterSprite(groundCenter.x, groundCenter.y, 7, zoom, 1.0f);

            // -------------------------------------------------------------------------
            // 3. PIXEL-ART NUCLEAR EXPLOSION & MUSHROOM CLOUD WITH SUB-FRAME FAKE-FPS
            // -------------------------------------------------------------------------
            if (m_atomicBombSheetPixels.size() == 1024u * 576u) {
                constexpr int SHEET_WIDTH = 1024;
                constexpr int FRAME_WIDTH = 204;
                constexpr int FRAME_HEIGHT = 144;
                constexpr int SHEET_COLUMNS = 5;
                constexpr int TOTAL_KEYFRAMES = 19;
                constexpr float ANCHOR_PIXEL_X = 102.0f;
                constexpr float ANCHOR_PIXEL_Y = 138.0f;

                // EN: Scale factor for colossal nuclear scale in isometric projection
                // ES: Factor de escala colosal para la explosión nuclear en proyección isométrica
                const float bombScale = std::clamp(zoom * 2.25f, 0.65f, 4.50f);
                const int destWidth = std::max(16, static_cast<int>(FRAME_WIDTH * bombScale));
                const int destHeight = std::max(16, static_cast<int>(FRAME_HEIGHT * bombScale));

                const int startScreenX = static_cast<int>(groundCenter.x - ANCHOR_PIXEL_X * bombScale);
                const int startScreenY = static_cast<int>(groundCenter.y - ANCHOR_PIXEL_Y * bombScale);

                // EN: Continuous sub-frame fake-FPS calculation across the explosion duration
                // ES: Cálculo continuo de fake-FPS sub-frame a lo largo de toda la duración de la explosión
                const float normalizedProgress = std::clamp(blastProgress, 0.0f, 0.999f);
                const float continuousFrameIndex = normalizedProgress * static_cast<float>(TOTAL_KEYFRAMES - 1);
                const int frameIndexA = static_cast<int>(std::floor(continuousFrameIndex));
                const int frameIndexB = std::min(TOTAL_KEYFRAMES - 1, frameIndexA + 1);
                const float subFrameTween = continuousFrameIndex - static_cast<float>(frameIndexA);

                const int srcStartXA = (frameIndexA % SHEET_COLUMNS) * FRAME_WIDTH;
                const int srcStartYA = (frameIndexA / SHEET_COLUMNS) * FRAME_HEIGHT;
                const int srcStartXB = (frameIndexB % SHEET_COLUMNS) * FRAME_WIDTH;
                const int srcStartYB = (frameIndexB / SHEET_COLUMNS) * FRAME_HEIGHT;

                // EN: Fade out smoke opacity during the final 12% of fx duration
                // ES: Desvanecer la opacidad del humo durante el último 12% de duración del efecto
                const float globalCloudAlpha = (normalizedProgress > 0.88f)
                    ? std::clamp((1.0f - normalizedProgress) / 0.12f, 0.0f, 1.0f)
                    : 1.0f;

                // EN: Initial blinding white thermal flash overlay (t < 0.28s)
                // ES: Destello térmico blanco cegador inicial (t < 0.28s)
                const float thermalFlashIntensity = (timeSeconds < 0.28f)
                    ? (1.0f - timeSeconds / 0.28f)
                    : 0.0f;

                for (int deltaDestY = 0; deltaDestY < destHeight; ++deltaDestY) {
                    const int screenY = startScreenY + deltaDestY;
                    const int frameY = m_height - 1 - screenY;
                    if (frameY < 0 || frameY >= m_height) continue;

                    const int sampleLocalY = std::clamp((deltaDestY * FRAME_HEIGHT) / destHeight, 0, FRAME_HEIGHT - 1);
                    const int srcRowA = (srcStartYA + sampleLocalY) * SHEET_WIDTH;
                    const int srcRowB = (srcStartYB + sampleLocalY) * SHEET_WIDTH;
                    const int rowOffset = frameY * m_width;

                    for (int deltaDestX = 0; deltaDestX < destWidth; ++deltaDestX) {
                        const int screenX = startScreenX + deltaDestX;
                        if (screenX < 0 || screenX >= m_width) continue;

                        const int sampleLocalX = std::clamp((deltaDestX * FRAME_WIDTH) / destWidth, 0, FRAME_WIDTH - 1);

                        // Sample keyframe A
                        const uint32_t pixelA = m_atomicBombSheetPixels[srcRowA + (srcStartXA + sampleLocalX)];
                        const uint8_t alphaA = (pixelA >> 24) & 0xFF;

                        // Sample keyframe B
                        const uint32_t pixelB = m_atomicBombSheetPixels[srcRowB + (srcStartXB + sampleLocalX)];
                        const uint8_t alphaB = (pixelB >> 24) & 0xFF;

                        if (alphaA < 6 && alphaB < 6) continue;

                        // EN: High-fidelity fake-FPS sub-frame linear interpolation (color & alpha tweening)
                        // ES: Interpolación lineal sub-frame de fake-FPS de alta fidelidad (mezcla de color y alfa)
                        const float blendedAlpha = ((static_cast<float>(alphaA) * (1.0f - subFrameTween)) +
                                                    (static_cast<float>(alphaB) * subFrameTween)) * globalCloudAlpha;
                        const float finalNormalizedAlpha = blendedAlpha / 255.0f;
                        if (finalNormalizedAlpha <= 0.02f) continue;

                        const float rawR = static_cast<float>(pixelA & 0xFF) * (1.0f - subFrameTween) +
                                           static_cast<float>(pixelB & 0xFF) * subFrameTween;
                        const float rawG = static_cast<float>((pixelA >> 8) & 0xFF) * (1.0f - subFrameTween) +
                                           static_cast<float>((pixelB >> 8) & 0xFF) * subFrameTween;
                        const float rawB = static_cast<float>((pixelA >> 16) & 0xFF) * (1.0f - subFrameTween) +
                                           static_cast<float>((pixelB >> 16) & 0xFF) * subFrameTween;

                        // Thermal core bloom boost during early nuclear fireball stages
                        const uint8_t outColorR = static_cast<uint8_t>(std::min(255.0f, rawR + thermalFlashIntensity * 140.0f));
                        const uint8_t outColorG = static_cast<uint8_t>(std::min(255.0f, rawG + thermalFlashIntensity * 120.0f));
                        const uint8_t outColorB = static_cast<uint8_t>(std::min(255.0f, rawB + thermalFlashIntensity * 90.0f));

                        // Alpha blend with framebuffer background
                        const uint32_t bgPixel = m_framebuffer[rowOffset + screenX];
                        const float invAlpha = 1.0f - finalNormalizedAlpha;
                        const uint8_t outR = static_cast<uint8_t>((bgPixel & 0xFF) * invAlpha + outColorR * finalNormalizedAlpha);
                        const uint8_t outG = static_cast<uint8_t>(((bgPixel >> 8) & 0xFF) * invAlpha + outColorG * finalNormalizedAlpha);
                        const uint8_t outB = static_cast<uint8_t>(((bgPixel >> 16) & 0xFF) * invAlpha + outColorB * finalNormalizedAlpha);

                        m_framebuffer[rowOffset + screenX] = (0xFFu << 24) |
                                                             (static_cast<uint32_t>(outB) << 16) |
                                                             (static_cast<uint32_t>(outG) << 8)  |
                                                              static_cast<uint32_t>(outR);
                    }
                }
            } else {
                // Fallback procedural rendering if texture buffer is not loaded
                if (timeSeconds < 1.6f) {
                    const float fireballExpansion = timeSeconds / 1.6f;
                    const float fireballRadius = (25.0f + fireballExpansion * 110.0f) * zoom;
                    drawTranslucentDustPuff(groundCenter.x, groundCenter.y - 10.0f * zoom, fireballRadius * 1.2f, fireballRadius * 0.60f, ColorRGB{255, 120, 20}, 0.70f);
                    drawTranslucentDustPuff(groundCenter.x, groundCenter.y - 12.0f * zoom, fireballRadius * 0.85f, fireballRadius * 0.42f, ColorRGB{255, 210, 60}, 0.80f);
                    drawTranslucentDustPuff(groundCenter.x, groundCenter.y - 14.0f * zoom, fireballRadius * 0.50f, fireballRadius * 0.25f, ColorRGB{255, 255, 240}, 0.88f);
                }
            }

            // -------------------------------------------------------------------------
            // 4. VOLUMETRIC EXPANDING RADIOACTIVE DUST CLOUD (Nube de Polvo Radiactivo)
            // -------------------------------------------------------------------------
            // EN: Massive billowing radioactive dust cloud expanding across the landscape and drifting downwind
            // ES: Nube masiva de polvo radiactivo ondulante expandiéndose por el terreno y derivando con el viento
            if (timeSeconds > 0.35f) {
                const float cloudTime = timeSeconds - 0.35f;
                const float maxDuration = fx.duration - 0.35f;
                const float progress = std::clamp(cloudTime / maxDuration, 0.0f, 1.0f);

                // EN: Progressive radial expansion: reaches ~30 cells in radius across the world
                // ES: Expansión radial progresiva: alcanza ~30 celdas de radio a lo largo del mapa
                const float cloudExpansionRadius = (18.0f + std::min(30.0f, cloudTime * 3.4f + std::sqrt(cloudTime) * 4.6f) * 32.0f) * zoom;
                const float cloudRadiusX = cloudExpansionRadius;
                const float cloudRadiusY = cloudRadiusX * 0.50f; // Isometric 2:1 projection

                // EN: Downwind atmospheric drift vector
                // ES: Vector de deriva atmosférica a favor del viento
                const float windX = (m_cloudSystem ? m_cloudSystem->getWindX() : 0.40f);
                const float windDriftX = cloudTime * windX * 24.0f * zoom;
                const float windDriftY = cloudTime * 5.0f * zoom * 0.5f;

                const float cloudCenterX = groundCenter.x + windDriftX;
                const float cloudCenterY = groundCenter.y + windDriftY;

                // Opacity curve: billows up quickly, maintains heavy toxic presence, then fades into lingering radioactive haze
                float cloudAlpha = 0.84f;
                if (progress < 0.12f) {
                    cloudAlpha = 0.32f + (progress / 0.12f) * 0.52f;
                } else if (progress > 0.65f) {
                    cloudAlpha = 0.84f * (1.0f - (progress - 0.65f) / 0.35f);
                }

                if (cloudAlpha > 0.02f) {
                    // EN: 1. Irradiated fallout ground haze: soft luminous toxic sulfur-green glow over the entire engulfed terrain
                    // ES: 1. Calina radiactiva sobre el terreno: suave resplandor verde-azufre tóxico sobre toda la zona
                    const ColorRGB radioactiveHaze{130, 160, 50};
                    drawTranslucentDustPuff(cloudCenterX, cloudCenterY, cloudRadiusX * 1.06f, cloudRadiusY * 1.06f, radioactiveHaze, cloudAlpha * 0.30f);

                    // EN: 2. Multi-tiered volumetric churning radioactive dust lobes in full radial circle
                    // ES: 2. Lóbulos volumétricos de polvo radiactivo en múltiples capas en círculo radial completo
                    constexpr int RADIOACTIVE_LOBES = 60;
                    for (int lobeIdx = 0; lobeIdx < RADIOACTIVE_LOBES; ++lobeIdx) {
                        const float lobeAngle = (static_cast<float>(lobeIdx) / RADIOACTIVE_LOBES) * 6.2831853f;
                        const float noiseVariation = 0.82f + 0.28f * std::sin(static_cast<float>(lobeIdx) * 4.1f + fx.seed + cloudTime * 1.6f);
                        const float distFraction = (0.28f + 0.72f * (static_cast<float>(lobeIdx % 8) / 7.0f)) * noiseVariation;

                        const float lobeDistX = cloudRadiusX * distFraction;
                        const float lobeDistY = cloudRadiusY * distFraction;
                        const float verticalBillow = (std::sin(static_cast<float>(lobeIdx) * 2.7f + cloudTime * 2.2f) * 14.0f + 10.0f) * zoom;

                        const float lobeScreenX = cloudCenterX + std::cos(lobeAngle) * lobeDistX;
                        const float lobeScreenY = cloudCenterY + std::sin(lobeAngle) * lobeDistY - verticalBillow;

                        const float lobeRadiusX = (24.0f + std::min(22.0f, cloudTime * 2.4f) + std::sin(static_cast<float>(lobeIdx) * 3.3f) * 7.0f) * zoom;
                        const float lobeRadiusY = lobeRadiusX * 0.54f;

                        // EN: Multi-pass toxic fallout shading: deep radioactive soot -> toxic body -> glowing rim
                        // ES: Sombreado multitono de polvo radiactivo: hollín profundo -> cuerpo tóxico -> borde fluorescente
                        const ColorRGB toxicShadowAsh{48, 50, 32};         // Deep radioactive soot
                        const ColorRGB irradiatedDustBody{132, 142, 58};     // Toxic radioactive particulate body
                        const ColorRGB glowingRadioactiveDust{210, 238, 85}; // Glowing ionised radioactive fallout dust

                        drawTranslucentDustPuff(lobeScreenX, lobeScreenY + 3.0f * zoom, lobeRadiusX * 1.12f, lobeRadiusY * 1.12f, toxicShadowAsh, cloudAlpha * 0.40f);
                        drawTranslucentDustPuff(lobeScreenX, lobeScreenY, lobeRadiusX, lobeRadiusY, irradiatedDustBody, cloudAlpha * 0.48f);
                        if (lobeIdx % 3 == 0) {
                            drawTranslucentDustPuff(lobeScreenX + 2.0f * zoom, lobeScreenY - 3.0f * zoom, lobeRadiusX * 0.65f, lobeRadiusY * 0.65f, glowingRadioactiveDust, cloudAlpha * 0.36f);
                        }
                    }
                }
            }

            // -------------------------------------------------------------------------
            // 6. FLOATING CATACLYSM BANNER TEXT
            // -------------------------------------------------------------------------
            const float mushroomCloudHeight = 144.0f * std::clamp(zoom * 2.25f, 0.65f, 4.50f);
            const int textPosY = static_cast<int>(groundCenter.y - (mushroomCloudHeight + 40.0f * zoom));
            if (timeSeconds < 3.5f) {
                drawText(static_cast<int>(groundCenter.x - 110), textPosY, "¡DETONACION ATOMICA [FUCKINGBOMB]!", ColorRGB{255, 240, 90}, 1);
            } else {
                drawText(static_cast<int>(groundCenter.x - 125), textPosY, "¡NUBE DE POLVO RADIACTIVO EN EXPANSION!", ColorRGB{210, 245, 80}, 1);
            }
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
        else if (m_activeDivinePower == DivinePowerType::Rain) pName = "LLUVIA BENDITA (Costo: 30 Fe)";
        else if (m_activeDivinePower == DivinePowerType::Deluge) pName = "GRAN DILUVIO (Costo: 65 Fe)";
        else if (m_activeDivinePower == DivinePowerType::AtomicBomb) pName = "EXPLOSIÓN ATÓMICA [FUCKINGBOMB] (Costo: 0 Fe / TRUCO)";

        drawSims2Panel(banX, banY, banW, banH, ColorRGB{24, 16, 12}, false, Palette::DIVINE_GOLD);
        drawText(banX + 16, banY + 7, "[MODO APUNTAR] Haz clic en el mapa para lanzar " + pName + " | Clic derecho para cancelar", Palette::DIVINE_GOLD, 1);
    }
    renderOptionsDropdown(); // siempre encima del resto del HUD
}

// =============================================================================
// EN: Render simulated world background for menus (Minecraft-inspired panorama)
// ES: Renderizar fondo de mundo simulado para menús (panorama inspirado en Minecraft)
// =============================================================================
void Renderer::renderWorldBackground(const WorldMap& map, const ReligionRegistry& registry,
                                     const ReplicationEngine& replication,
                                     const GestorHumanos& humanos,
                                     const IsometricCamera& camera,
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

    // EN: Draw celestial sky dome and celestial bodies
    // ES: Dibujar domo de cielo celestial y cuerpos celestes
    if (m_dayNight && !m_skyDrawnThisFrame) {
        drawCelestialSky(*m_dayNight);
        m_skyDrawnThisFrame = true;
    }

    // EN: Render isometric world without HUD overlays or targeting reticles
    // ES: Renderizar mundo isométrico sin capas de HUD ni retículas de selección
    renderWorld(map, registry, replication, humanos, camera, {-1, -1}, RenderMapMode::TerrainBiomes);

    // EN: Render cloud shadows projected onto the landscape
    // ES: Renderizar sombras de nubes proyectadas sobre el terreno
    if (m_cloudSystem) {
        renderCloudShadows(*m_cloudSystem, camera, m_dayNight, map.getWidth(), map.getHeight());
    }

    // EN: Apply dynamic daylighting and celestial bodies
    // ES: Aplicar iluminación dinámica y cuerpos celestes
    if (m_dayNight) {
        applyFakeDynamicLighting(*m_dayNight, map, humanos, nullptr, camera);
        renderCelestialBodies(*m_dayNight, camera, map.getWidth(), map.getHeight());
    }

    // EN: Render drifting cloud layer overhead
    // ES: Renderizar capa de nubes flotando en las alturas
    if (m_cloudSystem) {
        renderClouds(*m_cloudSystem, camera, m_dayNight, map.getWidth(), map.getHeight());
    }

    // EN: Apply Minecraft-inspired cinematic vignette for atmospheric depth
    // ES: Aplicar viñeta cinemática inspirada en Minecraft para profundidad atmosférica
    applyCinematicVignette();
}

// =============================================================================
// EN: Apply cinematic radial vignette to the current framebuffer
// ES: Aplicar viñeta radial cinemática al framebuffer actual
// =============================================================================
void Renderer::applyCinematicVignette() {
    const float centerPosX = static_cast<float>(m_width) * 0.5f;
    const float centerPosY = static_cast<float>(m_height) * 0.5f;
    const float maxRadiusSquared = (centerPosX * centerPosX + centerPosY * centerPosY);

    for (int screenY = 0; screenY < m_height; ++screenY) {
        const float deltaY = static_cast<float>(screenY) - centerPosY;
        const int rowOffset = screenY * m_width;
        for (int screenX = 0; screenX < m_width; ++screenX) {
            const float deltaX = static_cast<float>(screenX) - centerPosX;
            const float distanceSquared = deltaX * deltaX + deltaY * deltaY;
            const float normalizedDistance = distanceSquared / maxRadiusSquared;

            // EN: High vibrancy in center (88%), smooth gradual drop to 55% at corners
            // ES: Gran viveza en el centro (88%), caída suave gradual al 55% en las esquinas
            const float vignetteFactor = std::clamp(0.88f - (normalizedDistance * 0.35f), 0.50f, 0.95f);

            uint32_t& targetPixel = m_framebuffer[rowOffset + screenX];
            const uint8_t redChannel = static_cast<uint8_t>((targetPixel & 0xFF) * vignetteFactor);
            const uint8_t greenChannel = static_cast<uint8_t>(((targetPixel >> 8) & 0xFF) * vignetteFactor);
            const uint8_t blueChannel = static_cast<uint8_t>(((targetPixel >> 16) & 0xFF) * vignetteFactor);

            targetPixel = (0xFFu << 24) |
                          (static_cast<uint32_t>(blueChannel) << 16) |
                          (static_cast<uint32_t>(greenChannel) << 8) |
                           static_cast<uint32_t>(redChannel);
        }
    }
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
        // EN: Direct terrain atlas tile override from editor (-1 = procedural mapping)
        // ES: Sobrescritura directa de mosaico del atlas desde el editor (-1 = mapeo procedimental)
        if (cell.customTileIndex >= 0 && cell.customTileIndex < 16) {
            return cell.customTileIndex;
        }
        if (cell.isWater()) {
            // EN: Map the 4 progressive water depth phases to their authentic terrain atlas tiles:
            // 1 - Shoreline Foam -> sand.png (Tile 8) / shallowwater.png (Tile 10)
            // 2 - Shallow Coast  -> shallowwater.png (Tile 10)
            // 3 - Mid Ocean      -> ocean.png (Tile 14) / water.png (Tile 11)
            // 4 - Deep Abyssal   -> deepocean.png (Tile 13)
            // ES: Asignar las 4 fases progresivas de profundidad a sus mosaicos auténticos del atlas:
            // 1 - Espuma de Orilla -> sand.png (Mosaico 8) / shallowwater.png (Mosaico 10)
            // 2 - Costa Somera     -> shallowwater.png (Mosaico 10)
            // 3 - Mar Medio        -> ocean.png (Mosaico 14) / water.png (Mosaico 11)
            // 4 - Abisal Profundo  -> deepocean.png (Mosaico 13)
            switch (cell.getWaterDepthPhase()) {
                case WaterDepthPhase::ShorelineFoam: return 8;  // Sand/shore contact
                case WaterDepthPhase::ShallowCoast:  return 10; // shallowwater.png
                case WaterDepthPhase::MidOcean:      return 14; // ocean.png
                case WaterDepthPhase::DeepAbyssal:   return 13; // deepocean.png
            }
        }
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
            case BiomeType::VolcanicCaldera:return 1; // Suelo rocoso basáltico
            default:                        return 6;
        }
    };

    auto sampleAtlasRGB = [&](int tIdx, [[maybe_unused]] float gxCont, [[maybe_unused]] float gyCont, int wx, int wy, float& outR, float& outG, float& outB) {
        // EN: Sample base terrain or water tile texture directly from the seamless atlas
        // ES: Muestrear la textura base del mosaico de terreno o agua directamente desde el atlas continuo
        const uint32_t baseTexel = (!m_seamlessAtlasPixels.empty())
            ? m_seamlessAtlasPixels[(wy & 63) * TERRAIN_ATLAS_WIDTH + (tIdx * 64) + (wx & 63)]
            : m_terrainAtlasPixels[(wy & 63) * TERRAIN_ATLAS_WIDTH + (tIdx * 64) + (wx & 63)];
        outR = static_cast<float>(baseTexel & 0xFF);
        outG = static_cast<float>((baseTexel >> 8) & 0xFF);
        outB = static_cast<float>((baseTexel >> 16) & 0xFF);
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
            // EN: Lowland terrain and open sea share a common base altitude plane (0.50f)
            // ensuring organic, gap-free coastline blending without artificial vertical drops.
            // ES: El terreno bajo y el mar abierto comparten un plano de altitud base común (0.50f)
            // asegurando una fusión de costa orgánica sin huecos ni desniveles verticales artificiales.
            const float effectiveElevation = c.isWater() ? 0.50f : c.elevation;
            Vec2 screenPt = camera.gridTileCenterScreen(x, y, effectiveElevation);
            float frontDrop = 0.0f;
            if (!c.isWater()) {
                auto evaluateNeighborDrop = [&](int neighborX, int neighborY) {
                    if (neighborX < w && neighborY < h) {
                        const Cell& neighborCell = map.getCell(neighborX, neighborY);
                        const float neighborEffectiveElevation = neighborCell.isWater() ? 0.50f : neighborCell.elevation;
                        frontDrop = std::max(frontDrop, effectiveElevation - neighborEffectiveElevation);
                    } else {
                        frontDrop = std::max(frontDrop, effectiveElevation);
                    }
                };
                evaluateNeighborDrop(x + 1, y);
                evaluateNeighborDrop(x, y + 1);
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

            bool shoreE  = getSafeIsWater(x + 1, y) != isWater;
            bool shoreW  = getSafeIsWater(x - 1, y) != isWater;
            bool shoreN  = getSafeIsWater(x, y + 1) != isWater;
            bool shoreS  = getSafeIsWater(x, y - 1) != isWater;
            bool shoreNE = getSafeIsWater(x + 1, y + 1) != isWater;
            bool shoreNW = getSafeIsWater(x - 1, y + 1) != isWater;
            bool shoreSE = getSafeIsWater(x + 1, y - 1) != isWater;
            bool shoreSW = getSafeIsWater(x - 1, y - 1) != isWater;
            bool hasShoreNeighbor = shoreE || shoreW || shoreN || shoreS || shoreNE || shoreNW || shoreSE || shoreSW;

            // EN: Active volcanic lava bank border detection with neighbor cells
            // ES: Detección de bordes de ribera de lava volcánica activa con celdas vecinas
            auto getSafeHasActiveLava = [&](int neighborGridX, int neighborGridY) -> bool {
                if (neighborGridX >= 0 && neighborGridX < w && neighborGridY >= 0 && neighborGridY < h) {
                    return map.getCell(neighborGridX, neighborGridY).hasActiveLava;
                }
                return false;
            };

            const bool lavaShoreWest  = c.hasActiveLava && !getSafeHasActiveLava(x - 1, y);
            const bool lavaShoreEast  = c.hasActiveLava && !getSafeHasActiveLava(x + 1, y);
            const bool lavaShoreNorth = c.hasActiveLava && !getSafeHasActiveLava(x, y + 1);
            const bool lavaShoreSouth = c.hasActiveLava && !getSafeHasActiveLava(x, y - 1);
            const bool hasLavaShoreNeighbor = lavaShoreWest || lavaShoreEast || lavaShoreNorth || lavaShoreSouth;

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
                            sampleAtlasRGB(texIdx, gxCont, gyCont, wx, wy, r0, g0, b0);
                            float r = r0;
                            float g = g0;
                            float b = b0;

                            if (hasDiffNeighbor || hasShoreNeighbor) {
                                // EN: Continuous world-space organic terrain and coastline synthesis
                                // ES: Síntesis de terreno y costa orgánica continua en espacio mundial
                                const float turbX = organicTurbulence(gxCont * 1.6f, gyCont * 1.6f) * 0.36f;
                                const float turbY = organicTurbulence(gyCont * 1.6f + 14.0f, gxCont * 1.6f + 28.0f) * 0.36f;
                                const float sampleWorldX = gxCont + turbX;
                                const float sampleWorldY = gyCont + turbY;

                                const int cornerGridX0 = static_cast<int>(std::floor(sampleWorldX - 0.5f));
                                const int cornerGridY0 = static_cast<int>(std::floor(sampleWorldY - 0.5f));
                                const int cornerGridX1 = cornerGridX0 + 1;
                                const int cornerGridY1 = cornerGridY0 + 1;

                                const float rawFracX = (sampleWorldX - 0.5f) - static_cast<float>(cornerGridX0);
                                const float rawFracY = (sampleWorldY - 0.5f) - static_cast<float>(cornerGridY0);
                                const float clampedFracX = std::clamp(rawFracX, 0.0f, 1.0f);
                                const float clampedFracY = std::clamp(rawFracY, 0.0f, 1.0f);
                                const float smoothFracX = clampedFracX * clampedFracX * (3.0f - 2.0f * clampedFracX);
                                const float smoothFracY = clampedFracY * clampedFracY * (3.0f - 2.0f * clampedFracY);

                                const float weightCorner00 = (1.0f - smoothFracX) * (1.0f - smoothFracY);
                                const float weightCorner10 = smoothFracX * (1.0f - smoothFracY);
                                const float weightCorner01 = (1.0f - smoothFracX) * smoothFracY;
                                const float weightCorner11 = smoothFracX * smoothFracY;

                                auto evaluateCellWaterDepth = [&](int targetCellX, int targetCellY) -> float {
                                    if (targetCellX < 0 || targetCellX >= w || targetCellY < 0 || targetCellY >= h) {
                                        return 1.0f; // EN: Boundaries default to ocean / ES: Límites por defecto son océano
                                    }
                                    const Cell& targetCell = map.getCell(targetCellX, targetCellY);
                                    if (!targetCell.isWater() && targetCell.waterDepth <= 0.015f) {
                                        return 0.0f; // EN: Land / ES: Tierra
                                    }
                                    if (!targetCell.isNaturalWater() && targetCell.waterDepth > 0.015f) {
                                        // EN: Overland flooded depth mapped into progressive water depth range
                                        // ES: Profundidad de inundación superficial mapeada al rango de agua progresiva
                                        return std::clamp(0.20f + targetCell.waterDepth * 1.25f, 0.20f, 1.0f);
                                    }
                                    // EN: Water depth according to the 4 phases:
                                    // Phase 1: ShorelineFoam (~0.28f)
                                    // Phase 2: ShallowCoast  (~0.45f)
                                    // Phase 3: MidOcean      (~0.72f)
                                    // Phase 4: DeepAbyssal   (1.00f)
                                    // ES: Profundidad del agua según las 4 fases:
                                    // Fase 1: Espuma de Orilla (~0.28f)
                                    // Fase 2: Costa Somera     (~0.45f)
                                    // Fase 3: Mar Medio        (~0.72f)
                                    // Fase 4: Abisal Profundo  (1.00f)
                                    switch (targetCell.getWaterDepthPhase()) {
                                        case WaterDepthPhase::ShorelineFoam: return 0.28f;
                                        case WaterDepthPhase::ShallowCoast:  return 0.45f;
                                        case WaterDepthPhase::MidOcean:      return 0.72f;
                                        case WaterDepthPhase::DeepAbyssal:   return 1.00f;
                                    }
                                    if (targetCell.biome == BiomeType::ShallowWater) return 0.45f;
                                    return 1.00f;
                                };

                                const float waterDepth00 = evaluateCellWaterDepth(cornerGridX0, cornerGridY0);
                                const float waterDepth10 = evaluateCellWaterDepth(cornerGridX1, cornerGridY0);
                                const float waterDepth01 = evaluateCellWaterDepth(cornerGridX0, cornerGridY1);
                                const float waterDepth11 = evaluateCellWaterDepth(cornerGridX1, cornerGridY1);

                                const float continuousWaterDepth =
                                    waterDepth00 * weightCorner00 +
                                    waterDepth10 * weightCorner10 +
                                    waterDepth01 * weightCorner01 +
                                    waterDepth11 * weightCorner11;

                                const bool isWaterOrShore = (isWater || hasShoreNeighbor || continuousWaterDepth >= 0.15f);
                                if (isWaterOrShore) {
                                    // EN: 4-Phase Progressive Water Depth Rendering using authentic sprites:
                                    // Phase 1: Línea de Espuma / Orilla -> sand.png (Tile 8) + water_surf_sheet.rgba / water_borders_sheet.rgba
                                    // Phase 2: Aguas Someras / Costa    -> shallowwater.png (Tile 10) + water_ripples_sheet.rgba
                                    // Phase 3: Mar Medio / Océano       -> ocean.png (Tile 14) + water_waves_sheet.rgba / water_currents_sheet.rgba
                                    // Phase 4: Mar Profundo / Abisal    -> deepocean.png (Tile 13) + abyssal deep swells
                                    // ES: Renderizado progresivo de 4 fases de profundidad del agua usando sprites auténticos:
                                    // Fase 1: Línea de Espuma / Orilla -> sand.png (Mosaico 8) + water_surf_sheet.rgba / water_borders_sheet.rgba
                                    // Fase 2: Aguas Someras / Costa    -> shallowwater.png (Mosaico 10) + water_ripples_sheet.rgba
                                    // Fase 3: Mar Medio / Océano       -> ocean.png (Mosaico 14) + water_waves_sheet.rgba / water_currents_sheet.rgba
                                    // Fase 4: Mar Profundo / Abisal    -> deepocean.png (Mosaico 13) + oleaje abisal profundo
                                    constexpr float BEACH_INLAND_THRESHOLD   = 0.15f; // Land -> Beach sand
                                    constexpr float FOAM_LINE_THRESHOLD      = 0.30f; // Phase 1: Línea de Espuma / Orilla
                                    constexpr float SHALLOW_COAST_THRESHOLD  = 0.48f; // Phase 2: Aguas Someras / Costa
                                    constexpr float MID_OCEAN_THRESHOLD      = 0.72f; // Phase 3: Mar Medio / Océano
                                    constexpr float DEEP_ABYSSAL_THRESHOLD   = 0.88f; // Phase 4: Mar Profundo / Abisal

                                    float sandRed = 0.0f, sandGreen = 0.0f, sandBlue = 0.0f;
                                    sampleAtlasRGB(8, gxCont, gyCont, wx, wy, sandRed, sandGreen, sandBlue);

                                    float shallowWaterRed = 0.0f, shallowWaterGreen = 0.0f, shallowWaterBlue = 0.0f;
                                    sampleAtlasRGB(10, gxCont, gyCont, wx, wy, shallowWaterRed, shallowWaterGreen, shallowWaterBlue);

                                    float midOceanRed = 0.0f, midOceanGreen = 0.0f, midOceanBlue = 0.0f;
                                    sampleAtlasRGB(14, gxCont, gyCont, wx, wy, midOceanRed, midOceanGreen, midOceanBlue);

                                    float deepOceanRed = 0.0f, deepOceanGreen = 0.0f, deepOceanBlue = 0.0f;
                                    sampleAtlasRGB(13, gxCont, gyCont, wx, wy, deepOceanRed, deepOceanGreen, deepOceanBlue);

                                    if (continuousWaterDepth >= MID_OCEAN_THRESHOLD) {
                                        // EN: Transition from Phase 3 (Mid Sea / Ocean) into Phase 4 (Deep Sea / Abyssal)
                                        // ES: Transición desde Fase 3 (Mar Medio / Océano) hacia Fase 4 (Mar Profundo / Abisal)
                                        const float oceanAbyssRatio = std::clamp((continuousWaterDepth - MID_OCEAN_THRESHOLD) / (1.0f - MID_OCEAN_THRESHOLD), 0.0f, 1.0f);
                                        const float smoothAbyss = oceanAbyssRatio * oceanAbyssRatio * (3.0f - 2.0f * oceanAbyssRatio);
                                        r = midOceanRed * (1.0f - smoothAbyss) + deepOceanRed * smoothAbyss;
                                        g = midOceanGreen * (1.0f - smoothAbyss) + deepOceanGreen * smoothAbyss;
                                        b = midOceanBlue * (1.0f - smoothAbyss) + deepOceanBlue * smoothAbyss;

                                        if (continuousWaterDepth >= DEEP_ABYSSAL_THRESHOLD) {
                                            // EN: Phase 4: Deep Sea / Abyssal - deep slow oceanic swells and sunlight attenuation
                                            // ES: Fase 4: Mar Profundo / Abisal - oleaje oceánico lento y atenuación de luz solar
                                            const float abyssalSwell = std::sin((gxCont * 0.45f + gyCont * 0.40f) - m_animTime * 0.75f);
                                            const float abyssalDepthDim = 0.88f + 0.12f * abyssalSwell;
                                            r *= abyssalDepthDim * 0.92f;
                                            g *= abyssalDepthDim * 0.95f;
                                            b *= abyssalDepthDim;
                                        } else {
                                            // EN: Phase 3: Mid Sea / Ocean - rolling ocean swells and specular waves
                                            // ES: Fase 3: Mar Medio / Océano - oleaje ondulante y crestas especulares
                                            if (!m_waterWavesPixels.empty()) {
                                                const int waveTile = (static_cast<int>(gxCont * 1.5f + gyCont)) & 7;
                                                const int waveSampleU = std::clamp(static_cast<int>(std::fmod(std::abs(gxCont * 64.0f + m_animTime * 16.0f), 128.0f)), 0, 127);
                                                const int waveSampleV = std::clamp(static_cast<int>(std::fmod(std::abs(gyCont * 64.0f), 115.0f)), 0, 114);
                                                const uint32_t wavePixel = m_waterWavesPixels[waveSampleV * 1024 + (waveTile * 128 + waveSampleU)];
                                                const uint8_t waveAlpha = (wavePixel >> 24) & 0xFF;
                                                if (waveAlpha > 30) {
                                                    const float waveWeight = (static_cast<float>(waveAlpha) / 255.0f) * 0.45f * (1.0f - smoothAbyss);
                                                    r = r * (1.0f - waveWeight) + static_cast<float>(wavePixel & 0xFF) * waveWeight;
                                                    g = g * (1.0f - waveWeight) + static_cast<float>((wavePixel >> 8) & 0xFF) * waveWeight;
                                                    b = b * (1.0f - waveWeight) + static_cast<float>((wavePixel >> 16) & 0xFF) * waveWeight;
                                                }
                                            }
                                            const float waveNoise = std::sin((gxCont + gyCont) * 5.0f - m_animTime * 2.4f);
                                            if (waveNoise > 0.60f) {
                                                const float specularHighlight = (waveNoise - 0.60f) * 1.8f * (1.0f - smoothAbyss);
                                                r = std::min(255.0f, r + 70.0f * specularHighlight);
                                                g = std::min(255.0f, g + 90.0f * specularHighlight);
                                                b = std::min(255.0f, b + 140.0f * specularHighlight);
                                            }
                                        }
                                    } else if (continuousWaterDepth >= SHALLOW_COAST_THRESHOLD) {
                                        // EN: Transition from Phase 2 (Shallow Waters / Coast) into Phase 3 (Mid Sea / Ocean)
                                        // ES: Transición desde Fase 2 (Aguas Someras / Costa) hacia Fase 3 (Mar Medio / Océano)
                                        const float midRatio = (continuousWaterDepth - SHALLOW_COAST_THRESHOLD) / (MID_OCEAN_THRESHOLD - SHALLOW_COAST_THRESHOLD);
                                        const float smoothMid = midRatio * midRatio * (3.0f - 2.0f * midRatio);
                                        r = shallowWaterRed * (1.0f - smoothMid) + midOceanRed * smoothMid;
                                        g = shallowWaterGreen * (1.0f - smoothMid) + midOceanGreen * smoothMid;
                                        b = shallowWaterBlue * (1.0f - smoothMid) + midOceanBlue * smoothMid;

                                        if (!m_waterWavesPixels.empty()) {
                                            const int waveTile = (static_cast<int>(gxCont * 1.5f + gyCont)) & 7;
                                            const int waveSampleU = std::clamp(static_cast<int>(std::fmod(std::abs(gxCont * 64.0f + m_animTime * 14.0f), 128.0f)), 0, 127);
                                            const int waveSampleV = std::clamp(static_cast<int>(std::fmod(std::abs(gyCont * 64.0f), 115.0f)), 0, 114);
                                            const uint32_t wavePixel = m_waterWavesPixels[waveSampleV * 1024 + (waveTile * 128 + waveSampleU)];
                                            const uint8_t waveAlpha = (wavePixel >> 24) & 0xFF;
                                            if (waveAlpha > 30) {
                                                const float waveWeight = (static_cast<float>(waveAlpha) / 255.0f) * 0.35f * smoothMid;
                                                r = r * (1.0f - waveWeight) + static_cast<float>(wavePixel & 0xFF) * waveWeight;
                                                g = g * (1.0f - waveWeight) + static_cast<float>((wavePixel >> 8) & 0xFF) * waveWeight;
                                                b = b * (1.0f - waveWeight) + static_cast<float>((wavePixel >> 16) & 0xFF) * waveWeight;
                                            }
                                        }
                                    } else if (continuousWaterDepth >= FOAM_LINE_THRESHOLD) {
                                        // EN: Phase 2: Shallow Waters / Coast - crystalline turquoise water over submerged sand bed
                                        // ES: Fase 2: Aguas Someras / Costa - agua turquesa cristalina sobre lecho de arena sumergida
                                        const float shallowRatio = (continuousWaterDepth - FOAM_LINE_THRESHOLD) / (SHALLOW_COAST_THRESHOLD - FOAM_LINE_THRESHOLD);
                                        const float smoothShallow = shallowRatio * shallowRatio * (3.0f - 2.0f * shallowRatio);
                                        const float wetSandRed = sandRed * 0.88f;
                                        const float wetSandGreen = sandGreen * 0.86f;
                                        const float wetSandBlue = sandBlue * 0.82f;
                                        r = wetSandRed * (1.0f - smoothShallow) + shallowWaterRed * smoothShallow;
                                        g = wetSandGreen * (1.0f - smoothShallow) + shallowWaterGreen * smoothShallow;
                                        b = wetSandBlue * (1.0f - smoothShallow) + shallowWaterBlue * smoothShallow;

                                        if (!m_waterRipplesPixels.empty()) {
                                            const int rippleTile = (static_cast<int>(gxCont + gyCont)) & 7;
                                            const int rippleSampleU = std::clamp(static_cast<int>(std::fmod(std::abs(gxCont * 64.0f - m_animTime * 10.0f), 128.0f)), 0, 127);
                                            const int rippleSampleV = std::clamp(static_cast<int>(std::fmod(std::abs(gyCont * 64.0f + m_animTime * 6.0f), 170.0f)), 0, 169);
                                            const uint32_t ripplePixel = m_waterRipplesPixels[rippleSampleV * 1024 + (rippleTile * 128 + rippleSampleU)];
                                            const uint8_t rippleAlpha = (ripplePixel >> 24) & 0xFF;
                                            if (rippleAlpha > 20) {
                                                const float rippleWeight = (static_cast<float>(rippleAlpha) / 255.0f) * 0.32f * smoothShallow;
                                                r = r * (1.0f - rippleWeight) + static_cast<float>(ripplePixel & 0xFF) * rippleWeight;
                                                g = g * (1.0f - rippleWeight) + static_cast<float>((ripplePixel >> 8) & 0xFF) * rippleWeight;
                                                b = b * (1.0f - rippleWeight) + static_cast<float>((ripplePixel >> 16) & 0xFF) * rippleWeight;
                                            }
                                        }

                                        // EN: Phase 1: Foam Line / Shoreline - active surf foam washing over the shore interface
                                        // ES: Fase 1: Línea de Espuma / Orilla - espuma viva de rompiente bañando la interfaz de la orilla
                                        const float tidalWashCycle = 0.5f + 0.5f * std::sin(m_animTime * 2.8f + (gxCont + gyCont) * 2.0f);
                                        const float dynamicFoamPos = FOAM_LINE_THRESHOLD + 0.05f * tidalWashCycle;
                                        if (continuousWaterDepth <= dynamicFoamPos + 0.055f) {
                                            float surfFoamR = 240.0f, surfFoamG = 250.0f, surfFoamB = 255.0f;
                                            float surfFoamAlpha = 0.65f;
                                            if (!m_waterSurfPixels.empty()) {
                                                const int surfTile = (static_cast<int>(gxCont * 1.5f + gyCont * 1.5f)) & 7;
                                                const int surfSampleU = std::clamp(static_cast<int>(std::fmod(std::abs(gxCont * 64.0f - m_animTime * 14.0f), 128.0f)), 0, 127);
                                                const int surfSampleV = std::clamp(static_cast<int>(std::fmod(std::abs(gyCont * 64.0f), 116.0f)), 0, 115);
                                                const uint32_t surfPixel = m_waterSurfPixels[surfSampleV * 1024 + (surfTile * 128 + surfSampleU)];
                                                const uint8_t alphaByte = (surfPixel >> 24) & 0xFF;
                                                if (alphaByte > 40) {
                                                    surfFoamR = static_cast<float>(surfPixel & 0xFF);
                                                    surfFoamG = static_cast<float>((surfPixel >> 8) & 0xFF);
                                                    surfFoamB = static_cast<float>((surfPixel >> 16) & 0xFF);
                                                    surfFoamAlpha = (static_cast<float>(alphaByte) / 255.0f) * 0.85f;
                                                }
                                            }
                                            const float foamDistance = std::abs(continuousWaterDepth - dynamicFoamPos);
                                            const float foamFactor = std::clamp(1.0f - (foamDistance / 0.055f), 0.0f, 1.0f);
                                            const float foamPulse = foamFactor * (0.60f + 0.40f * std::sin((continuousWaterDepth - FOAM_LINE_THRESHOLD) * 45.0f + m_animTime * 4.0f));
                                            const float totalFoamWeight = std::clamp(surfFoamAlpha * foamPulse, 0.0f, 0.95f);
                                            r = r * (1.0f - totalFoamWeight) + surfFoamR * totalFoamWeight;
                                            g = g * (1.0f - totalFoamWeight) + surfFoamG * totalFoamWeight;
                                            b = b * (1.0f - totalFoamWeight) + surfFoamB * totalFoamWeight;
                                        }
                                    } else if (continuousWaterDepth >= BEACH_INLAND_THRESHOLD) {
                                        // EN: Transition from inland terrain into authentic beach sand sprite (Tile 8)
                                        // ES: Transición del terreno interior hacia el sprite auténtico de arena de playa (Mosaico 8)
                                        const float beachFraction = (continuousWaterDepth - BEACH_INLAND_THRESHOLD) / (FOAM_LINE_THRESHOLD - BEACH_INLAND_THRESHOLD);
                                        const float smoothBeach = beachFraction * beachFraction * (3.0f - 2.0f * beachFraction);
                                        r = r0 * (1.0f - smoothBeach) + sandRed * smoothBeach;
                                        g = g0 * (1.0f - smoothBeach) + sandGreen * smoothBeach;
                                        b = b0 * (1.0f - smoothBeach) + sandBlue * smoothBeach;
                                    } else {
                                        // EN: Inland terrain blending smoothly into beach sand fringe
                                        // ES: Terreno interior fusionándose suavemente hacia la franja de arena de playa
                                        const float fringeFraction = continuousWaterDepth / BEACH_INLAND_THRESHOLD;
                                        const float smoothFringe = fringeFraction * fringeFraction * (3.0f - 2.0f * fringeFraction) * 0.45f;
                                        r = r0 * (1.0f - smoothFringe) + sandRed * smoothFringe;
                                        g = g0 * (1.0f - smoothFringe) + sandGreen * smoothFringe;
                                        b = b0 * (1.0f - smoothFringe) + sandBlue * smoothFringe;
                                    }
                                } else {
                                    // EN: Continuous land-to-land multi-biome splatting (Grass, Forest, Desert, Mountains, etc.)
                                    // ES: Fusión multi-bioma continua entre terrenos (Pasto, Bosque, Desierto, Montañas, etc.)
                                    const struct CornerSample {
                                        int gridPosX;
                                        int gridPosY;
                                        float blendWeight;
                                    } cornerSamples[4] = {
                                        {cornerGridX0, cornerGridY0, weightCorner00},
                                        {cornerGridX1, cornerGridY0, weightCorner10},
                                        {cornerGridX0, cornerGridY1, weightCorner01},
                                        {cornerGridX1, cornerGridY1, weightCorner11}
                                    };

                                    float accumulatedRed = 0.0f;
                                    float accumulatedGreen = 0.0f;
                                    float accumulatedBlue = 0.0f;
                                    float totalAccumulatedWeight = 0.0f;

                                    for (const auto& sample : cornerSamples) {
                                        if (sample.blendWeight <= 0.0001f) continue;
                                        const int sampleTexIdx = (sample.gridPosX >= 0 && sample.gridPosX < w && sample.gridPosY >= 0 && sample.gridPosY < h)
                                            ? getCellTexIdx(sample.gridPosX, sample.gridPosY)
                                            : texIdx;
                                        float sampleRed = 0.0f, sampleGreen = 0.0f, sampleBlue = 0.0f;
                                        sampleAtlasRGB(sampleTexIdx, gxCont, gyCont, wx, wy, sampleRed, sampleGreen, sampleBlue);
                                        accumulatedRed += sampleRed * sample.blendWeight;
                                        accumulatedGreen += sampleGreen * sample.blendWeight;
                                        accumulatedBlue += sampleBlue * sample.blendWeight;
                                        totalAccumulatedWeight += sample.blendWeight;
                                    }

                                    if (totalAccumulatedWeight > 0.0001f) {
                                        const float invTotalWeight = 1.0f / totalAccumulatedWeight;
                                        r = accumulatedRed * invTotalWeight;
                                        g = accumulatedGreen * invTotalWeight;
                                        b = accumulatedBlue * invTotalWeight;
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

                            // 3B2. Suelo Quemado / Cenizas Calcinadas o Suelo en Llamas (isBurnt, isBurning, heatAccumulator)
                            // EN: Scorched earth, charcoal blocks, glowing magma embers, and thermal preheating from destruction tileset
                            // ES: Capa de tierra calcinada, bloques de carbón, ascuas incandescentes y precalentamiento térmico del tileset de destrucción
                            if (c.isBurnt) {
                                if (!m_destructionTerrainPixels.empty()) {
                                    const int terrainSampleX = std::clamp(static_cast<int>(std::fmod(std::abs(gxCont * 64.0f), 64.0f)), 0, 63);
                                    const int terrainSampleY = std::clamp(static_cast<int>(std::fmod(std::abs(gyCont * 64.0f), 64.0f)), 0, 63);
                                    size_t ashTextureIndex = 0;
                                    if (c.heatAccumulator > 0.40f && m_destructionTerrainPixels.size() > 6) {
                                        // EN: High residual heat: blast core or dry cracked clay
                                        // ES: Calor residual alto: núcleo de explosión o arcilla agrietada seca
                                        ashTextureIndex = ((c.x + c.y) % 2 == 0) ? 5 : 6;
                                    } else {
                                        // EN: Standard soot, charcoal blocks, and ash pit
                                        // ES: Hollín estándar, bloques de carbón y fosa de ceniza
                                        ashTextureIndex = (static_cast<size_t>(c.x * 7 + c.y * 13) % 3);
                                    }

                                    if (ashTextureIndex < m_destructionTerrainPixels.size() &&
                                        !m_destructionTerrainPixels[ashTextureIndex].empty()) {
                                        const uint32_t packedAsh = m_destructionTerrainPixels[ashTextureIndex][terrainSampleY * 64 + terrainSampleX];
                                        r = static_cast<float>(packedAsh & 0xFF);
                                        g = static_cast<float>((packedAsh >> 8) & 0xFF);
                                        b = static_cast<float>((packedAsh >> 16) & 0xFF);
                                    }
                                }
                            } else if (c.isBurning) {
                                if (!m_destructionTerrainPixels.empty()) {
                                    const int terrainSampleX = std::clamp(static_cast<int>(std::fmod(std::abs(gxCont * 64.0f), 64.0f)), 0, 63);
                                    const int terrainSampleY = std::clamp(static_cast<int>(std::fmod(std::abs(gyCont * 64.0f), 64.0f)), 0, 63);
                                    const size_t flameTexIndex = (c.fireIntensity > 0.55f && m_destructionTerrainPixels.size() > 7) ? 7 : 3;
                                    if (flameTexIndex < m_destructionTerrainPixels.size() &&
                                        !m_destructionTerrainPixels[flameTexIndex].empty()) {
                                        const uint32_t packedFlame = m_destructionTerrainPixels[flameTexIndex][terrainSampleY * 64 + terrainSampleX];
                                        r = static_cast<float>(packedFlame & 0xFF);
                                        g = static_cast<float>((packedFlame >> 8) & 0xFF);
                                        b = static_cast<float>((packedFlame >> 16) & 0xFF);
                                    }
                                }
                            } else if (c.heatAccumulator > 0.35f) {
                                if (!m_destructionTerrainPixels.empty() && m_destructionTerrainPixels.size() > 5 &&
                                    !m_destructionTerrainPixels[5].empty()) {
                                    const int terrainSampleX = std::clamp(static_cast<int>(std::fmod(std::abs(gxCont * 64.0f), 64.0f)), 0, 63);
                                    const int terrainSampleY = std::clamp(static_cast<int>(std::fmod(std::abs(gyCont * 64.0f), 64.0f)), 0, 63);
                                    const float thermalFactor = std::clamp((c.heatAccumulator - 0.35f) / 0.65f, 0.0f, 0.85f);
                                    const uint32_t dryPacked = m_destructionTerrainPixels[5][terrainSampleY * 64 + terrainSampleX];
                                    const float dryR = static_cast<float>(dryPacked & 0xFF);
                                    const float dryG = static_cast<float>((dryPacked >> 8) & 0xFF);
                                    const float dryB = static_cast<float>((dryPacked >> 16) & 0xFF);
                                    r = r * (1.0f - thermalFactor) + dryR * thermalFactor;
                                    g = g * (1.0f - thermalFactor) + dryG * thermalFactor;
                                    b = b * (1.0f - thermalFactor) + dryB * thermalFactor;
                                }
                            }

                            // 3C. Cráteres Permanentes de Terreno: Impactos de Meteorito y Bombas
                            // EN: Permanent terrain craters: meteor impacts and bomb blast bowls
                            // ES: Cráteres permanentes en terreno: impactos de meteorito y cuencas de bombas
                            if (c.hasImpactCrater) {
                                const float effectiveDepth = std::clamp(c.craterDepth, 0.15f, 1.0f);

                                // EN: Sample authentic high-detail impact crater textures from destruction tileset
                                // ES: Muestrear texturas auténticas de cráter de impacto del tileset de destrucción
                                int craterSpriteIndex = 0;
                                if (c.craterType == CraterType::NuclearDetonation) {
                                    if (effectiveDepth > 0.60f) {
                                        craterSpriteIndex = 7; // crater_giant_colossal (128x128)
                                    } else if (effectiveDepth > 0.35f) {
                                        craterSpriteIndex = 5; // crater_deep_bowl (64x64)
                                    } else {
                                        craterSpriteIndex = 2; // crater_dirt_debris (64x64)
                                    }
                                } else if (c.craterType == CraterType::MeteorImpact) {
                                    if (effectiveDepth > 0.60f) {
                                        craterSpriteIndex = 5; // crater_deep_bowl (64x64)
                                    } else if (effectiveDepth > 0.35f) {
                                        craterSpriteIndex = 3; // crater_basalt_rock (64x64)
                                    } else {
                                        craterSpriteIndex = 6; // crater_isolated_rim (64x64)
                                    }
                                } else if (c.craterType == CraterType::TectonicSeismic) {
                                    craterSpriteIndex = (effectiveDepth > 0.45f) ? 4 : 2; // crater_cluster_pockmarks or crater_dirt_debris
                                } else {
                                    craterSpriteIndex = (effectiveDepth > 0.50f) ? 3 : 1; // crater_basalt_rock or crater_dirt_medium
                                }

                                if (!m_destructionCraterPixels.empty() && craterSpriteIndex >= 0 &&
                                    craterSpriteIndex < static_cast<int>(m_destructionCraterPixels.size()) &&
                                    !m_destructionCraterPixels[static_cast<size_t>(craterSpriteIndex)].empty()) {
                                    const int craterDimension = (craterSpriteIndex == 7) ? 128 : 64;
                                    const float normCraterU = std::clamp(du + 0.5f, 0.0f, 1.0f);
                                    const float normCraterV = std::clamp(dv + 0.5f, 0.0f, 1.0f);
                                    const int sampleCraterX = std::clamp(static_cast<int>(normCraterU * static_cast<float>(craterDimension - 1)), 0, craterDimension - 1);
                                    const int sampleCraterY = std::clamp(static_cast<int>(normCraterV * static_cast<float>(craterDimension - 1)), 0, craterDimension - 1);
                                    const uint32_t packedCraterPixel = m_destructionCraterPixels[static_cast<size_t>(craterSpriteIndex)][sampleCraterY * craterDimension + sampleCraterX];
                                    const uint8_t craterAlphaChannel = (packedCraterPixel >> 24) & 0xFF;
                                    if (craterAlphaChannel > 10) {
                                        const float sampledCraterRed = static_cast<float>(packedCraterPixel & 0xFF);
                                        const float sampledCraterGreen = static_cast<float>((packedCraterPixel >> 8) & 0xFF);
                                        const float sampledCraterBlue = static_cast<float>((packedCraterPixel >> 16) & 0xFF);
                                        const float craterAlphaWeight = (static_cast<float>(craterAlphaChannel) / 255.0f);
                                        r = r * (1.0f - craterAlphaWeight) + sampledCraterRed * craterAlphaWeight;
                                        g = g * (1.0f - craterAlphaWeight) + sampledCraterGreen * craterAlphaWeight;
                                        b = b * (1.0f - craterAlphaWeight) + sampledCraterBlue * craterAlphaWeight;
                                    }
                                }
                            }

                            // 3D. Ríos y Charcos de Lava / Magma Activo
                            // EN: High-fidelity volcanic lava and magma simulation with 4-phase lifecycle progression
                            // ES: Simulación de alta fidelidad de lava y magma volcánico con progresión de ciclo de vida de 4 fases
                            if (c.hasActiveLava) {
                                if (!m_lavaTilesPixels.empty()) {
                                    constexpr float LAVA_TILE_DIMENSION = 128.0f;
                                    constexpr float MAX_LAVA_LIFECYCLE_TIME = 35.0f;

                                    // EN: Determine thermal activity level (volcanic vents maintain permanent maximum heat)
                                    // ES: Determinar el nivel de actividad térmica (los respiraderos volcánicos mantienen calor máximo permanente)
                                    const float thermalHeatLevel = c.hasVolcano ? 1.0f : std::clamp(c.lavaTimer / MAX_LAVA_LIFECYCLE_TIME, 0.0f, 1.0f);

                                    // EN: Organic viscous fluid flow drift along world space
                                    // ES: Deriva de flujo de fluido viscoso orgánico a lo largo del espacio mundial
                                    const float viscousFlowDrift = std::sin((gxCont + gyCont) * 2.2f + m_animTime * 1.5f) * 5.0f;
                                    const float dynamicFlowCoordU = std::abs(gxCont * 64.0f + m_animTime * 8.0f + viscousFlowDrift);
                                    const float dynamicFlowCoordV = std::abs(gyCont * 64.0f + viscousFlowDrift * 0.5f);

                                    const int sampleOffsetU = std::clamp(static_cast<int>(std::fmod(dynamicFlowCoordU, LAVA_TILE_DIMENSION)), 0, 127);
                                    const int sampleOffsetV = std::clamp(static_cast<int>(std::fmod(dynamicFlowCoordV, LAVA_TILE_DIMENSION)), 0, 127);

                                    auto sampleLavaTileRGB = [&](int targetTileIndex, float& outRed, float& outGreen, float& outBlue) {
                                        const int pixelCoordX = targetTileIndex * 128 + sampleOffsetU;
                                        const int pixelCoordY = sampleOffsetV;
                                        const uint32_t packedColor = m_lavaTilesPixels[pixelCoordY * 512 + pixelCoordX];
                                        outRed = static_cast<float>(packedColor & 0xFF);
                                        outGreen = static_cast<float>((packedColor >> 8) & 0xFF);
                                        outBlue = static_cast<float>((packedColor >> 16) & 0xFF);
                                    };

                                    float moltenRed = 0.0f, moltenGreen = 0.0f, moltenBlue = 0.0f;
                                    float boilingRed = 0.0f, boilingGreen = 0.0f, boilingBlue = 0.0f;
                                    float crustRed = 0.0f, crustGreen = 0.0f, crustBlue = 0.0f;
                                    float basaltRed = 0.0f, basaltGreen = 0.0f, basaltBlue = 0.0f;

                                    float targetLavaRed = 0.0f;
                                    float targetLavaGreen = 0.0f;
                                    float targetLavaBlue = 0.0f;

                                    // EN: Multi-phase thermal lifecycle transitions:
                                    //     Phase 0: Molten cellular magma (Tile 0) & Liquid boiling lava (Tile 1)
                                    //     Phase 1: Liquid boiling lava (Tile 1) & Semi-cooled magma crust (Tile 2)
                                    //     Phase 2: Semi-cooled magma crust (Tile 2) & Solidified basalt bedrock with magma veins (Tile 3)
                                    // ES: Transiciones de ciclo de vida térmico multifase:
                                    //     Fase 0: Magma celular fundido (Mosaico 0) y lava hirviente líquida (Mosaico 1)
                                    //     Fase 1: Lava hirviente líquida (Mosaico 1) y corteza de magma semicongelada (Mosaico 2)
                                    //     Fase 2: Corteza de magma semicongelada (Mosaico 2) y lecho rocoso de basalto con vetas de magma (Mosaico 3)
                                    if (thermalHeatLevel >= 0.65f) {
                                        sampleLavaTileRGB(0, moltenRed, moltenGreen, moltenBlue);
                                        sampleLavaTileRGB(1, boilingRed, boilingGreen, boilingBlue);

                                        const float superheatProgress = (thermalHeatLevel - 0.65f) / 0.35f;
                                        const float boilingPulse = 0.5f + 0.5f * std::sin(m_animTime * 3.5f + (gxCont + gyCont) * 3.8f);
                                        const float moltenWeight = std::clamp(superheatProgress * 0.70f + boilingPulse * 0.30f, 0.0f, 1.0f);

                                        targetLavaRed = boilingRed * (1.0f - moltenWeight) + moltenRed * moltenWeight;
                                        targetLavaGreen = boilingGreen * (1.0f - moltenWeight) + moltenGreen * moltenWeight;
                                        targetLavaBlue = boilingBlue * (1.0f - moltenWeight) + moltenBlue * moltenWeight;
                                    } else if (thermalHeatLevel >= 0.25f) {
                                        sampleLavaTileRGB(1, boilingRed, boilingGreen, boilingBlue);
                                        sampleLavaTileRGB(2, crustRed, crustGreen, crustBlue);

                                        const float coolingProgress = (thermalHeatLevel - 0.25f) / 0.40f;
                                        const float smoothProgress = coolingProgress * coolingProgress * (3.0f - 2.0f * coolingProgress);

                                        targetLavaRed = crustRed * (1.0f - smoothProgress) + boilingRed * smoothProgress;
                                        targetLavaGreen = crustGreen * (1.0f - smoothProgress) + boilingGreen * smoothProgress;
                                        targetLavaBlue = crustBlue * (1.0f - smoothProgress) + boilingBlue * smoothProgress;
                                    } else {
                                        sampleLavaTileRGB(2, crustRed, crustGreen, crustBlue);
                                        sampleLavaTileRGB(3, basaltRed, basaltGreen, basaltBlue);

                                        const float solidifyProgress = thermalHeatLevel / 0.25f;
                                        const float smoothProgress = solidifyProgress * solidifyProgress * (3.0f - 2.0f * solidifyProgress);

                                        targetLavaRed = basaltRed * (1.0f - smoothProgress) + crustRed * smoothProgress;
                                        targetLavaGreen = basaltGreen * (1.0f - smoothProgress) + crustGreen * smoothProgress;
                                        targetLavaBlue = basaltBlue * (1.0f - smoothProgress) + crustBlue * smoothProgress;
                                    }

                                    // EN: Pulsating incandescent thermal radiance
                                    // ES: Radiancia térmica incandescente pulsante
                                    const float thermalRadiance = 1.0f + (thermalHeatLevel * 0.16f * (0.6f + 0.4f * std::sin(m_animTime * 4.2f + du * 5.0f)));
                                    r = std::clamp(targetLavaRed * thermalRadiance, 0.0f, 255.0f);
                                    g = std::clamp(targetLavaGreen * thermalRadiance, 0.0f, 255.0f);
                                    b = std::clamp(targetLavaBlue * thermalRadiance, 0.0f, 255.0f);

                                    // EN: Volcanic rocky shoreline and bank margin blending (Row 1 of tileset)
                                    // ES: Fusión de riberas rocosas volcánicas y márgenes de orilla (Fila 1 del conjunto de mosaicos)
                                    if (hasLavaShoreNeighbor && diamondDist > 0.68f) {
                                        int bankSubIndex = 0;
                                        if (du < 0.0f && dv >= 0.0f) {
                                            bankSubIndex = 2; // Bottom-left corner bank
                                        } else if (du >= 0.0f && dv >= 0.0f) {
                                            bankSubIndex = 3; // Bottom-right corner bank
                                        } else if (du < 0.0f) {
                                            bankSubIndex = 0; // Left shore bank
                                        } else {
                                            bankSubIndex = 1; // Right shore bank
                                        }

                                        const int bankSampleU = std::clamp(static_cast<int>((du + 0.5f) * 127.0f), 0, 127);
                                        const int bankSampleV = std::clamp(static_cast<int>((dv + 0.5f) * 127.0f), 0, 127);
                                        const int bankPixelCoordX = bankSubIndex * 128 + bankSampleU;
                                        const int bankPixelCoordY = 128 + bankSampleV;
                                        const uint32_t bankPackedColor = m_lavaTilesPixels[bankPixelCoordY * 512 + bankPixelCoordX];
                                        const uint8_t bankAlphaByte = (bankPackedColor >> 24) & 0xFF;

                                        if (bankAlphaByte > 20) {
                                            const float edgeDistanceFactor = (diamondDist - 0.68f) / 0.32f;
                                            const float bankAlphaWeight = (static_cast<float>(bankAlphaByte) / 255.0f) * std::clamp(edgeDistanceFactor, 0.0f, 1.0f) * 0.88f;
                                            const float bankRed = static_cast<float>(bankPackedColor & 0xFF);
                                            const float bankGreen = static_cast<float>((bankPackedColor >> 8) & 0xFF);
                                            const float bankBlue = static_cast<float>((bankPackedColor >> 16) & 0xFF);

                                            r = r * (1.0f - bankAlphaWeight) + bankRed * bankAlphaWeight;
                                            g = g * (1.0f - bankAlphaWeight) + bankGreen * bankAlphaWeight;
                                            b = b * (1.0f - bankAlphaWeight) + bankBlue * bankAlphaWeight;
                                        }
                                    }
                                } else {
                                    // EN: Fallback procedural magma rendering if pixel assets are not loaded
                                    // ES: Renderizado de magma procedural de respaldo si los assets no están cargados
                                    const float flowNoise = std::sin((gxCont + gyCont) * 8.0f + m_animTime * 3.5f) * 0.5f + 0.5f;
                                    const float crustPlate = std::sin(gxCont * 12.0f - m_animTime * 1.5f) * std::cos(gyCont * 12.0f + m_animTime * 1.5f);
                                    if (crustPlate > 0.32f) {
                                        r = 45.0f; g = 22.0f; b = 18.0f;
                                    } else {
                                        r = 255.0f;
                                        g = 65.0f + 140.0f * flowNoise;
                                        b = 18.0f;
                                    }
                                }
                            }

                            // 3G. Acumulación de Agua Superficial, Dinámica de Fluidos y Corrientes
                            // EN: Overland fluid dynamics: directional wave ripples, whitewater foam, and depth coloration
                            // ES: Dinámica de fluidos superficiales: ondas direccionales, espuma de rápidos y coloración por profundidad
                            if (!c.isNaturalWater() && c.waterDepth > 0.015f) {
                                const float waterCoverage = std::clamp(c.waterDepth / 0.45f, 0.35f, 0.82f);
                                const float flowSpeed = c.getFlowSpeed();

                                float waveNoise = 0.0f;
                                float streamRed = 0.0f, streamGreen = 0.0f, streamBlue = 0.0f;
                                bool hasStreamSample = false;

                                if (flowSpeed > 0.06f) {
                                    const float normVx = c.flowVelocityX / flowSpeed;
                                    const float normVy = c.flowVelocityY / flowSpeed;
                                    const float flowCoord = (gxCont * normVx + gyCont * normVy);
                                    const float crossCoord = (-gxCont * normVy + gyCont * normVx);

                                    if (!m_waterCurrentsPixels.empty()) {
                                        // EN: Sample directional river current sprites rushing downstream
                                        // ES: Muestrear sprites de corriente fluvial direccional fluyendo río abajo
                                        const int streamTileIndex = (static_cast<int>(gxCont + gyCont)) % 5;
                                        const int sampleU = std::clamp(static_cast<int>(std::fmod(std::abs(flowCoord * 64.0f - m_animTime * (16.0f + flowSpeed * 32.0f)), 84.0f)), 0, 84);
                                        const int sampleV = std::clamp(static_cast<int>(std::fmod(std::abs(crossCoord * 64.0f), 100.0f)), 0, 101);
                                        const int currentPixelX = std::clamp(streamTileIndex * 85 + sampleU, 0, 852);
                                        const uint32_t currentPixel = m_waterCurrentsPixels[sampleV * 853 + currentPixelX];
                                        const uint8_t streamAlpha = (currentPixel >> 24) & 0xFF;
                                        if (streamAlpha > 20) {
                                            streamRed = static_cast<float>(currentPixel & 0xFF);
                                            streamGreen = static_cast<float>((currentPixel >> 8) & 0xFF);
                                            streamBlue = static_cast<float>((currentPixel >> 16) & 0xFF);
                                            hasStreamSample = true;
                                        }
                                    }

                                    // EN: Continuous directional wave crests with subtle cross-flow undulation
                                    // ES: Crestas continuas de olas direccionales con sutil ondulación transversal
                                    const float crossPerturbation = std::sin(crossCoord * 5.2f) * 0.42f;
                                    waveNoise = std::sin(flowCoord * 13.5f - m_animTime * (3.0f + flowSpeed * 6.5f) + crossPerturbation);
                                } else {
                                    // EN: Ambient omnidirectional gentle pond ripples in standing water
                                    // ES: Ondulaciones suaves omnidireccionales en charcos de agua estancada
                                    waveNoise = std::sin((gxCont + gyCont) * 9.5f + m_animTime * 2.8f + std::sin((gxCont - gyCont) * 6.0f) * 0.5f);
                                }

                                const float specularGlint = (waveNoise > 0.65f) ? (waveNoise - 0.65f) * 2.2f : 0.0f;
                                const float whiteWaterFoam = (flowSpeed > 0.85f && waveNoise > 0.45f) ? (waveNoise - 0.45f) * 1.7f : 0.0f;

                                // EN: Transition from crystal turquoise to rich river sapphire
                                // ES: Transición desde turquesa cristalino hasta azul zafiro fluvial profundo
                                const float depthFactor = std::clamp(c.waterDepth / 0.55f, 0.0f, 1.0f);
                                float waterTintR = (28.0f * (1.0f - depthFactor) + 16.0f * depthFactor) + specularGlint * 180.0f + whiteWaterFoam * 215.0f;
                                float waterTintG = (145.0f * (1.0f - depthFactor) + 90.0f * depthFactor) + specularGlint * 205.0f + whiteWaterFoam * 225.0f;
                                float waterTintB = (195.0f * (1.0f - depthFactor) + 215.0f * depthFactor) + specularGlint * 235.0f + whiteWaterFoam * 245.0f;

                                if (hasStreamSample) {
                                    constexpr float streamWeight = 0.52f;
                                    waterTintR = waterTintR * (1.0f - streamWeight) + streamRed * streamWeight;
                                    waterTintG = waterTintG * (1.0f - streamWeight) + streamGreen * streamWeight;
                                    waterTintB = waterTintB * (1.0f - streamWeight) + streamBlue * streamWeight;
                                }

                                r = r * (1.0f - waterCoverage) + waterTintR * waterCoverage;
                                g = g * (1.0f - waterCoverage) + waterTintG * waterCoverage;
                                b = b * (1.0f - waterCoverage) + waterTintB * waterCoverage;
                            } else if (!c.isNaturalWater() && c.moisture > 0.18f) {
                                // EN: Seamless moisture dampening continuously interpolated across cell boundaries
                                // ES: Atenuación continua de humedad interpolada suavemente a través de los límites de celda
                                const float eastMoisture  = (x + 1 < w) ? map.getCell(x + 1, y).moisture : c.moisture;
                                const float westMoisture  = (x > 0) ? map.getCell(x - 1, y).moisture : c.moisture;
                                const float northMoisture = (y + 1 < h) ? map.getCell(x, y + 1).moisture : c.moisture;
                                const float southMoisture = (y > 0) ? map.getCell(x, y - 1).moisture : c.moisture;

                                const float weightCoordU = std::clamp(du + 0.5f, 0.0f, 1.0f);
                                const float weightCoordV = std::clamp(dv + 0.5f, 0.0f, 1.0f);
                                const float interpolatedMoistureX = (1.0f - weightCoordU) * westMoisture + weightCoordU * eastMoisture;
                                const float interpolatedMoistureY = (1.0f - weightCoordV) * southMoisture + weightCoordV * northMoisture;
                                const float effectiveMoisture = (interpolatedMoistureX + interpolatedMoistureY) * 0.5f;

                                if (effectiveMoisture > 0.20f) {
                                    const float dampFactor = (effectiveMoisture - 0.20f) * 0.45f;
                                    r *= (1.0f - dampFactor * 0.35f);
                                    g *= (1.0f - dampFactor * 0.25f);
                                    b *= (1.0f - dampFactor * 0.15f);
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
                                    // EN: Top skirt lip tinted according to terrain biome or scorched state
                                    // ES: Labio superior del faldón adaptado al bioma del terreno o estado calcinado
                                    if (c.isBurnt) {
                                        sr = 40; sg = 36; sb = 34;
                                    } else if (c.biome == BiomeType::AridDesert || hasShoreNeighbor) {
                                        sr = 210; sg = 185; sb = 125;
                                    } else if (c.biome == BiomeType::RuggedMountains || c.biome == BiomeType::VolcanicCaldera) {
                                        sr = (c.biome == BiomeType::VolcanicCaldera) ? 75 : 120;
                                        sg = (c.biome == BiomeType::VolcanicCaldera) ? 45 : 115;
                                        sb = (c.biome == BiomeType::VolcanicCaldera) ? 40 : 110;
                                    } else {
                                        sr = 78; sg = 102; sb = 48;
                                    }
                                } else {
                                    // EN: Earthen / rocky cliff wall with isometric lateral shading (highlight left, shadow right)
                                    // ES: Pared de tierra / roca con sombreado lateral isométrico (luz a la izquierda, sombra a la derecha)
                                    const float sideShade = (relX < 0.0f) ? 1.06f : 0.84f;
                                    const float depthShade = (1.0f - tDrop * 0.28f) * sideShade;
                                    if (c.biome == BiomeType::RuggedMountains || c.biome == BiomeType::VolcanicCaldera) {
                                        const float baseR = (c.biome == BiomeType::VolcanicCaldera) ? 68.0f : 100.0f;
                                        const float baseG = (c.biome == BiomeType::VolcanicCaldera) ? 40.0f : 96.0f;
                                        const float baseB = (c.biome == BiomeType::VolcanicCaldera) ? 36.0f : 92.0f;
                                        sr = static_cast<uint8_t>(std::clamp(baseR * depthShade, 0.0f, 255.0f));
                                        sg = static_cast<uint8_t>(std::clamp(baseG * depthShade, 0.0f, 255.0f));
                                        sb = static_cast<uint8_t>(std::clamp(baseB * depthShade, 0.0f, 255.0f));
                                    } else {
                                        sr = static_cast<uint8_t>(std::clamp(88.0f * depthShade, 0.0f, 255.0f));
                                        sg = static_cast<uint8_t>(std::clamp(72.0f * depthShade, 0.0f, 255.0f));
                                        sb = static_cast<uint8_t>(std::clamp(56.0f * depthShade, 0.0f, 255.0f));
                                    }
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
    // EN: Optional Isometric Terrain Grid overlay (configurable in settings and toggled with hotkey 'H')
    // ES: Capa opcional de cuadrícula isométrica de terreno (configurable en opciones y alternable con tecla rápida 'H')
    // =========================================================================
    if (m_showGrid) {
        constexpr ColorRGB gridLineColor{195, 215, 235, 95}; // EN: Subtle translucent silver/azure line / ES: Línea sutil translúcida plateada/azurada
        for (int depth = 0; depth < w + h - 1; ++depth) {
            for (int x = 0; x <= depth; ++x) {
                int y = depth - x;
                if (x >= w || y >= h) continue;

                const Cell& currentCell = map.getCell(x, y);
                const float gridElevation = currentCell.isWater() ? 0.50f : currentCell.elevation;
                const Vec2 screenPt = camera.gridTileCenterScreen(x, y, gridElevation);

                // EN: Frustum culling check for grid lines
                // ES: Comprobación de recorte de frustum para líneas de cuadrícula
                if (screenPt.x + halfW < 0.0f || screenPt.x - halfW > static_cast<float>(m_width) ||
                    screenPt.y + halfH < 0.0f || screenPt.y - halfH > static_cast<float>(m_height)) {
                    continue;
                }

                // EN: If fog of war is active, skip obscured unexplored cells
                // ES: Si la niebla de guerra está activa, omitir celdas oscurecidas no exploradas
                if (fogOn && m_fog->visibility(x, y) < 0.10f && m_fog->explored(x, y) < 0.10f) {
                    continue;
                }

                drawDiamondOutline(screenPt.x, screenPt.y, halfW, halfH, gridLineColor);
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
        CraterSprite    = 0,
        Villager        = 1,
        Caravan         = 2,
        Prop            = 3,
        ResourceNode    = 4,
        Temple          = 5,
        Volcano         = 6,
        House           = 7,
        FireEffect      = 8,
        SteamEffect     = 9,
        Whirlpool       = 10,
        DestructionProp = 11
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
        float customIntensity{1.0f};
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

            // EN: If cell has a permanent crater, queue authentic crater sprite on ground surface ("nada más")
            // ES: Si la celda tiene un cráter permanente, encolar sprite auténtico de cráter sobre la superficie ("nada más")
            if (c.hasImpactCrater) {
                const float effectiveDepth = std::clamp(c.craterDepth, 0.15f, 1.0f);
                int craterSpriteIndex = 0;
                if (c.craterType == CraterType::NuclearDetonation) {
                    // EN: Colossal 224x112 nuclear crater basin matching true impact scale
                    // ES: Cuenca colosal de cráter nuclear 224x112 según la escala real de impacto
                    craterSpriteIndex = 7;
                } else if (c.craterType == CraterType::MeteorImpact) {
                    // EN: Deep impact bowl 160x80 for meteorites or basalt rock
                    // ES: Cuenco profundo 160x80 para meteoritos o roca basáltica
                    craterSpriteIndex = (effectiveDepth >= 0.40f) ? 5 : 3;
                } else if (c.craterType == CraterType::TectonicSeismic) {
                    craterSpriteIndex = (effectiveDepth > 0.45f) ? 4 : 2;
                } else {
                    craterSpriteIndex = (effectiveDepth > 0.50f) ? 3 : 1;
                }

                sceneSprites.push_back({
                    static_cast<float>(x + y) + 0.01f,
                    IsoSpriteKind::CraterSprite,
                    x, y, craterSpriteIndex, nullptr, nullptr
                });
                // EN: Complete excavation of impact zone: no trees, stumps, props, or buildings can spawn here ("nada más")
                // ES: Excavación total de zona de impacto: sin árboles, tocones, accesorios ni edificios aquí ("nada más")
            } else if (c.hasVolcano) {
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

                // EN: 1. Mineral deposits (Gold, Iron, Stone, Copper, and Diamond): render reliably whenever resources remain
                // ES: 1. Depósitos de minerales (Oro, Hierro, Piedra, Cobre y Diamante): se muestran de forma consistente mientras tengan existencias
                if ((c.naturalResource == ResourceType::Gold ||
                     c.naturalResource == ResourceType::Iron ||
                     c.naturalResource == ResourceType::Stone ||
                     c.naturalResource == ResourceType::Copper ||
                     c.naturalResource == ResourceType::Diamond) && c.resourceAmount > 0) {
                    sceneSprites.push_back({
                        static_cast<float>(x + y) + 1.05f,
                        IsoSpriteKind::ResourceNode,
                        x, y, static_cast<int>(c.naturalResource), nullptr, nullptr
                    });
                }
                // EN: 2. Organic food bushes and timber forests: organic distribution
                // ES: 2. Fuentes de Comida (arbustos) y Madera (árboles): distribución orgánica
                else if (c.naturalResource != ResourceType::None && c.resourceAmount > 0 && (roll >= 58 && roll < 85)) {
                    // EN: If trees were uprooted or burnt by cataclysm/fire, do not render standing wood resource trees
                    // ES: Si los árboles fueron arrancados o quemados por cataclismo/fuego, no renderizar árboles erguidos
                    if (!((c.hasTreesUprooted || c.isBurnt || c.isBurning) && c.naturalResource == ResourceType::Wood)) {
                        sceneSprites.push_back({
                            static_cast<float>(x + y) + 1.05f,
                            IsoSpriteKind::ResourceNode,
                            x, y, static_cast<int>(c.naturalResource), nullptr, nullptr
                        });
                    }
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

                    // EN: Suppress standing tree and bush props if cataclysm, wildfire or blast impacted this cell
                    // ES: Suprimir árboles y arbustos erguidos si un cataclismo, incendio o explosión impactó esta celda
                    if (c.hasTreesUprooted || c.isBurnt || c.isBurning) {
                        if (propId != 6) {
                            propId = -1;
                        }
                    }

                    if (propId >= 0) {
                        sceneSprites.push_back({
                            static_cast<float>(x + y) + 1.00f,
                            IsoSpriteKind::Prop,
                            x, y, propId, nullptr, nullptr
                        });
                    }
                }

                // EN: Determine whether this cell originally contained standing trees (timber or natural tree props)
                // ES: Determinar si esta celda originalmente contenía árboles erguidos (madera o árboles naturales)
                const bool isResourceTree = (c.naturalResource == ResourceType::Wood);
                bool isNaturalTreeProp = false;
                if (c.biome == BiomeType::DenseForest && roll < 56) {
                    const int potentialProp = (roll < 18) ? 0 : ((roll < 36) ? 1 : ((roll < 46) ? 5 : 3));
                    if (potentialProp == 0 || potentialProp == 1) {
                        isNaturalTreeProp = true;
                    }
                } else if (c.biome == BiomeType::FertileValley && roll < 14) {
                    const int potentialProp = (roll < 6) ? 5 : ((roll < 10) ? 1 : 4);
                    if (potentialProp == 1) {
                        isNaturalTreeProp = true;
                    }
                } else if ((c.biome == BiomeType::HighlandHills || c.biome == BiomeType::RuggedMountains) && roll < 24) {
                    const int potentialProp = (roll < 14) ? 6 : ((roll < 19) ? 0 : 2);
                    if (potentialProp == 0) {
                        isNaturalTreeProp = true;
                    }
                }

                const bool hasTreeOrigin = isResourceTree || isNaturalTreeProp || c.hasTreesUprooted;

                // EN: Burnt trees, charred trunks, and blackened stumps must ONLY replace cells that originally had trees
                // ES: Los árboles quemados, troncos y tocones calcinados ÚNICAMENTE deben reemplazar celdas que tenían árboles
                int destructionPropIndex = -1;
                if (hasTreeOrigin && (c.isBurnt || c.isBurning || c.hasTreesUprooted)) {
                    if (c.isBurning) {
                        // EN: Burning tree with glowing embers, burning stump core, or smoking timber
                        // ES: Árbol ardiendo con ascuas, núcleo de tocón encendido o maderas humeantes
                        constexpr std::array<int, 4> BURNING_VARIANTS = {6, 8, 9, 10};
                        destructionPropIndex = BURNING_VARIANTS[roll % BURNING_VARIANTS.size()];
                    } else if (c.isBurnt) {
                        // EN: Charred crooked/branchy trees, jagged trunk, and blackened stumps
                        // ES: Árboles quemados retorcidos/ramificados, tronco dentado y tocones negros
                        constexpr std::array<int, 6> BURNT_VARIANTS = {0, 1, 2, 3, 4, 5};
                        destructionPropIndex = BURNT_VARIANTS[roll % BURNT_VARIANTS.size()];
                    } else if (c.hasTreesUprooted) {
                        // EN: Broken tree trunks, fallen charred timber, and uprooted stump
                        // ES: Troncos rotos, maderas derribadas calcinadas y tocón arrancado
                        constexpr std::array<int, 4> UPROOTED_VARIANTS = {2, 4, 5, 10};
                        destructionPropIndex = UPROOTED_VARIANTS[roll % UPROOTED_VARIANTS.size()];
                    }
                }

                if (destructionPropIndex >= 0) {
                    sceneSprites.push_back({
                        static_cast<float>(x + y) + 1.00f,
                        IsoSpriteKind::DestructionProp,
                        x, y, destructionPropIndex, nullptr, nullptr
                    });
                }
            }

            // EN: If cell is actively burning, queue dynamic fire effect sprite
            // ES: Si la celda está ardiendo activamente, encolar sprite dinámico de fuego
            if (c.isBurning && c.fireIntensity > 0.03f) {
                sceneSprites.push_back({
                    static_cast<float>(x + y) + 1.15f,
                    IsoSpriteKind::FireEffect,
                    x, y, 0, nullptr, nullptr, nullptr, c.fireIntensity
                });
            }

            // EN: If cell has active steam from extinguished fire, queue steam plume sprite
            // ES: Si la celda tiene vapor activo por fuego extinguido, encolar sprite de nube de vapor
            if (c.steamTimer > 0.05f) {
                sceneSprites.push_back({
                    static_cast<float>(x + y) + 1.18f,
                    IsoSpriteKind::SteamEffect,
                    x, y, 0, nullptr, nullptr, nullptr, c.steamTimer
                });
            }

            // EN: If cell is flooded with significant depth, queue dynamic whirlpool eddy sprite
            // ES: Si la celda está inundada con profundidad considerable, encolar sprite de remolino
            if (c.isFlooded && c.waterDepth > 0.28f) {
                sceneSprites.push_back({
                    static_cast<float>(x + y) + 0.95f,
                    IsoSpriteKind::Whirlpool,
                    x, y, 0, nullptr, nullptr, nullptr, std::min(1.0f, c.waterDepth / 0.5f)
                });
            }
        }
    }

    // EN: Determine if a villager is sheltered inside a completed home (must not be drawn)
    // ES: Determinar si un aldeano está cobijado dentro de una vivienda terminada (no debe dibujarse)
    auto isVillagerInsideHouse = [&](const Humano& villagerUnit) -> bool {
        if (villagerUnit.estaDurmiendoBajoTecho()) return true;
        if (m_buildingSystem && villagerUnit.tieneHogar()) {
            const House* assignedHouse = m_buildingSystem->getHouseById(villagerUnit.getHomeId());
            if (assignedHouse && assignedHouse->isCompleted()) {
                const float houseCenterX = static_cast<float>(assignedHouse->cellX) + 0.5f;
                const float houseCenterY = static_cast<float>(assignedHouse->cellY) + 0.5f;
                const float deltaCoordX = villagerUnit.getX() - houseCenterX;
                const float deltaCoordY = villagerUnit.getY() - houseCenterY;
                if ((deltaCoordX * deltaCoordX + deltaCoordY * deltaCoordY) <= 0.25f &&
                    (villagerUnit.estaDurmiendo() || villagerUnit.getVelocidad() < 0.08f)) {
                    return true;
                }
            }
        }
        return false;
    };

    // 2. Agregar Aldeanos (unidades Humano) con su profundidad continua exacta (hx + hy)
    for (const auto& hObj : humanos.getHumanos()) {
        // Las unidades de otras fes sólo se ven dentro del alcance de la fe del jugador
        if (isHiddenByFog(hObj.getCeldaX(), hObj.getCeldaY(), hObj.getReligionId())) continue;
        // EN: If a unit is inside a house, it must not be visible at all
        // ES: Si algo está dentro de una casa, no se debe de ver
        if (isVillagerInsideHouse(hObj)) continue;

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

    const Humano* selectedVillagerPtr = humanos.getHumanoSeleccionado();
    const int idSeleccionado = selectedVillagerPtr ? selectedVillagerPtr->getId() : -1;

    // =========================================================================
    // EN: LAYER 1 - GROUND DECALS & FLOOR MARKERS
    // All ground rings, selection outlines, and routes lie flat on the terrain,
    // positioned beneath units and structures so they never intersect sprites.
    // ES: CAPA 1 - CALCAS Y MARCADORES DE SUELO
    // Todos los anillos, aros de selección y rutas se apoyan sobre el terreno,
    // situados debajo de unidades y estructuras para no cortar los sprites.
    // =========================================================================
    for (const auto& villagerUnit : humanos.getHumanos()) {
        if (!villagerUnit.estaVivo() || isVillagerInsideHouse(villagerUnit)) continue;
        if (isHiddenByFog(villagerUnit.getCeldaX(), villagerUnit.getCeldaY(), villagerUnit.getReligionId())) continue;

        const int cellCoordX = std::clamp(villagerUnit.getCeldaX(), 0, w - 1);
        const int cellCoordY = std::clamp(villagerUnit.getCeldaY(), 0, h - 1);
        const Vec2 groundFootPos = camera.gridToScreen(villagerUnit.getX(), villagerUnit.getY(),
                                                        map.getCell(cellCoordX, cellCoordY).elevation);

        // EN: Animated water ripple and droplet splash if villager is wading, swimming, or in water
        // ES: Onda de agua animada y salpicadura de gotas si el aldeano está vadeando, nadando o en agua
        const Cell& currentCell = map.getCell(cellCoordX, cellCoordY);
        if (currentCell.isWater() || villagerUnit.isSwimming() || villagerUnit.getRagdoll().isInWater()) {
            drawWaterSplash(groundFootPos.x, groundFootPos.y, zoom, m_animTime + static_cast<float>(villagerUnit.getId()), 0.70f);
        }

        ColorRGB unitAuraColor(230, 161, 59);
        const Religion* religionPtr = registry.getReligion(villagerUnit.getReligionId());
        if (religionPtr) unitAuraColor = religionPtr->color;

        const bool isSelectedUnit = (villagerUnit.getId() == idSeleccionado);

        // EN: Units view mode: faith diamond ring on ground plane under feet
        // ES: Vista UNIDADES: anillo de fe en el plano del suelo bajo los pies
        if (mode == RenderMapMode::Units) {
            const float unitRingRadius = (villagerUnit.esChaman() ? 15.0f : 10.0f) * zoom;
            drawDiamondOutline(groundFootPos.x, groundFootPos.y, unitRingRadius, unitRingRadius * 0.5f, unitAuraColor);
            drawDiamondOutline(groundFootPos.x, groundFootPos.y, unitRingRadius + 1.0f, unitRingRadius * 0.5f + 0.5f, unitAuraColor);
            if (villagerUnit.esChaman()) {
                drawDiamondOutline(groundFootPos.x, groundFootPos.y, unitRingRadius * 0.6f, unitRingRadius * 0.3f, Palette::DIVINE_GOLD);
            }
        }

        // EN: Selected unit floor indicator on ground plane
        // ES: Indicador de unidad seleccionada en el plano del suelo
        if (isSelectedUnit) {
            const float selectRadius = (villagerUnit.esChaman() ? 16.0f : 12.0f) * zoom;
            drawDiamondOutline(groundFootPos.x, groundFootPos.y, selectRadius + 2.0f, (selectRadius + 2.0f) * 0.5f, unitAuraColor);
            drawDiamondOutline(groundFootPos.x, groundFootPos.y, selectRadius + 1.0f, (selectRadius + 1.0f) * 0.5f, ColorRGB{255, 255, 255});
            drawDiamondOutline(groundFootPos.x, groundFootPos.y, selectRadius, selectRadius * 0.5f, unitAuraColor);
            if (villagerUnit.esChaman()) {
                drawDiamondOutline(groundFootPos.x, groundFootPos.y, selectRadius * 0.6f, selectRadius * 0.3f, Palette::DIVINE_GOLD);
            }

            // EN: Manual movement route and target diamond on ground surface
            // ES: Ruta de movimiento manual y rombo de destino en la superficie del suelo
            if (villagerUnit.tieneOrdenManual()) {
                int targetCellCoordX = std::clamp(static_cast<int>(std::floor(villagerUnit.getDestinoX())), 0, w - 1);
                int targetCellCoordY = std::clamp(static_cast<int>(std::floor(villagerUnit.getDestinoY())), 0, h - 1);
                Vec2 destGroundPos = camera.gridToScreen(villagerUnit.getDestinoX(), villagerUnit.getDestinoY(),
                                                         map.getCell(targetCellCoordX, targetCellCoordY).elevation);
                drawLine(static_cast<int>(groundFootPos.x), static_cast<int>(groundFootPos.y),
                         static_cast<int>(destGroundPos.x), static_cast<int>(destGroundPos.y), unitAuraColor);
                drawDiamondOutline(destGroundPos.x, destGroundPos.y, 10.0f * zoom, 5.0f * zoom, unitAuraColor);
                drawDiamondOutline(destGroundPos.x, destGroundPos.y, 6.0f * zoom, 3.0f * zoom, Palette::DIVINE_GOLD);
            }
        }
    }

    // =========================================================================
    // EN: LAYER 2 - UNIFIED ISOMETRIC DEPTH QUEUE (Entities and Structures)
    // Rendered strictly back-to-front by continuous Y-depth (D = x + y).
    // Foreground objects use sprite alpha transparency to reveal background entities.
    // Solid structures/trees occlude organically; only unoccluded parts are visible.
    // ES: CAPA 2 - COLA UNIFICADA DE PROFUNDIDAD ISOMÉTRICA (Entidades y Estructuras)
    // Renderizado estrictamente de atrás hacia adelante por profundidad Y (D = x + y).
    // Objetos en primer plano usan transparencia alfa para revelar entidades detrás.
    // Estructuras y árboles sólidos ocluyen orgánicamente; sólo lo no tapado se ve.
    // =========================================================================
    for (const auto& item : sceneSprites) {
        if (item.kind == IsoSpriteKind::CraterSprite) {
            const Cell& c = map.getCell(item.cellX, item.cellY);
            Vec2 screenPt = camera.gridTileCenterScreen(item.cellX, item.cellY, c.elevation);
            int startX = 0, startY = 0, craterWidth = 0, craterHeight = 0;
            getCraterBounds(screenPt.x, screenPt.y, item.subId, zoom, startX, startY, craterWidth, craterHeight);
            if (startX + craterWidth >= 0 && startX <= m_width &&
                startY + craterHeight >= 0 && startY <= m_height) {
                drawCraterSprite(screenPt.x, screenPt.y, item.subId, zoom, 1.0f);
            }
        } else if (item.kind == IsoSpriteKind::Villager && item.humanoPtr) {
            const Humano& hObj = *item.humanoPtr;
            ColorRGB aura(230, 161, 59);
            const Religion* r = registry.getReligion(hObj.getReligionId());
            if (r) aura = r->color;
            const bool esSel = (hObj.getId() == idSeleccionado);
            const TileSet& ts = hObj.esChaman() ? m_shamanTileSet : (hObj.esMujer() ? m_femaleTileSet : m_tileSet);
            hObj.renderizar(m_framebuffer.data(), m_width, m_height, camera, map, ts, aura, esSel);
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
            int startX = 0, startY = 0, propWidth = 0, propHeight = 0;
            getTerrainPropBounds(screenPt.x, screenPt.y, item.subId, zoom, startX, startY, propWidth, propHeight);

            if (startX + propWidth >= 0 && startX <= m_width &&
                startY + propHeight >= 0 && startY <= m_height) {
                // EN: Solid organic render: natural canopy covers background, gaps reveal units behind
                // ES: Renderizado sólido y orgánico: el follaje cubre lo de atrás, los huecos revelan unidades
                drawTerrainProp(screenPt.x, screenPt.y, item.subId, zoom, 1.0f);
            }
        } else if (item.kind == IsoSpriteKind::DestructionProp) {
            const Cell& c = map.getCell(item.cellX, item.cellY);
            Vec2 screenPt = camera.gridTileCenterScreen(item.cellX, item.cellY, c.elevation);
            int startX = 0, startY = 0, propWidth = 0, propHeight = 0;
            getDestructionPropBounds(screenPt.x, screenPt.y, item.subId, zoom, startX, startY, propWidth, propHeight);

            if (startX + propWidth >= 0 && startX <= m_width &&
                startY + propHeight >= 0 && startY <= m_height) {
                // EN: Render destruction prop (burnt trees, charred stumps, smoking timber, blast debris)
                // ES: Renderizar accesorio de destrucción (árboles quemados, tocones, maderas humeantes, escombros)
                drawDestructionProp(screenPt.x, screenPt.y, item.subId, zoom, 1.0f);
            }
        } else if (item.kind == IsoSpriteKind::ResourceNode) {
            const Cell& c = map.getCell(item.cellX, item.cellY);
            Vec2 screenPt = camera.gridTileCenterScreen(item.cellX, item.cellY, c.elevation);
            int startX = 0, startY = 0, resourceWidth = 0, resourceHeight = 0;
            auto resourceType = static_cast<ResourceType>(item.subId);
            getResourceNodeBounds(screenPt.x, screenPt.y, resourceType, zoom, startX, startY, resourceWidth, resourceHeight);

            if (startX + resourceWidth >= 0 && startX <= m_width &&
                startY + resourceHeight >= 0 && startY <= m_height) {
                drawResourceNode(screenPt.x, screenPt.y, resourceType, zoom, c.resourceAmount, 1.0f, c.resourceVariant, c.maxResourceAmount);
            }
        } else if (item.kind == IsoSpriteKind::Temple) {
            const Cell& c = map.getCell(item.cellX, item.cellY);
            Vec2 screenPt = camera.gridTileCenterScreen(item.cellX, item.cellY, c.elevation);
            int startX = 0, startY = 0, templeWidth = 0, templeHeight = 0;
            getMonumentBounds(screenPt.x, screenPt.y, item.subId, zoom, startX, startY, templeWidth, templeHeight);

            if (startX + templeWidth >= 0 && startX <= m_width &&
                startY + templeHeight >= 0 && startY <= m_height) {
                ColorRGB holyColor(255, 230, 100);
                int hsRel = (c.dominantReligionId > 0) ? c.dominantReligionId : c.holySiteReligionId;
                if (hsRel > 0) {
                    const Religion* r = registry.getReligion(hsRel);
                    if (r) holyColor = r->color;
                }
                drawSpriteMonument(screenPt.x, screenPt.y, item.subId, holyColor, zoom, 1.0f);
                // EN: Campfire display temporarily removed upon request; kept in reserve for future polish
                // ES: Visualización de fogata retirada temporalmente a petición; conservada en reserva para pulido posterior
            }
        } else if (item.kind == IsoSpriteKind::Volcano) {
            const Cell& c = map.getCell(item.cellX, item.cellY);
            Vec2 screenPt = camera.gridTileCenterScreen(item.cellX, item.cellY, c.elevation);
            if (screenPt.x + halfW * 3.0f >= 0 && screenPt.x - halfW * 3.0f <= m_width &&
                screenPt.y + halfH * 2.0f >= 0 && screenPt.y - halfH * 6.0f <= m_height) {
                drawVolcano(screenPt.x, screenPt.y, zoom, m_animTime, c.hasActiveLava, c.lavaTimer);
            }
        } else if (item.kind == IsoSpriteKind::House && item.housePtr) {
            const House& house = *item.housePtr;
            const Cell& c = map.getCell(item.cellX, item.cellY);
            Vec2 screenPt = camera.gridTileCenterScreen(item.cellX, item.cellY, c.elevation);
            int startX = 0, startY = 0, houseWidth = 0, houseHeight = 0;
            getHouseBounds(screenPt.x, screenPt.y, house, zoom, startX, startY, houseWidth, houseHeight);

            if (startX + houseWidth >= 0 && startX <= m_width &&
                startY + houseHeight >= 0 && startY <= m_height) {
                bool isNight = m_dayNight && m_dayNight->isNight();
                // EN: Solid house facade and roof: completely hides anyone inside, blocks what is behind
                // ES: Fachada y techo de casa sólidos: oculta completamente a quien esté dentro, tapa lo que está detrás
                drawSpriteHouse(screenPt.x, screenPt.y, house, zoom, isNight, 1.0f);

                // EN: If house is on an actively burning cell, render flames leaping from thatched eaves
                // ES: Si la casa está en una celda en llamas, renderizar fuego brotando del alero de paja
                if (c.isBurning && !house.isRuined()) {
                    const float windVx = m_cloudSystem ? m_cloudSystem->getWindX() : 0.20f;
                    const float windVy = m_cloudSystem ? m_cloudSystem->getWindY() : 0.05f;
                    drawFireEffect(screenPt.x, screenPt.y - houseHeight * 0.50f, c.fireIntensity * 0.85f, zoom, m_animTime, windVx, windVy);
                }
            }
        } else if (item.kind == IsoSpriteKind::FireEffect) {
            const Cell& c = map.getCell(item.cellX, item.cellY);
            const Vec2 screenPt = camera.gridTileCenterScreen(item.cellX, item.cellY, c.elevation);
            const float windVx = m_cloudSystem ? m_cloudSystem->getWindX() : 0.20f;
            const float windVy = m_cloudSystem ? m_cloudSystem->getWindY() : 0.05f;
            drawFireEffect(screenPt.x, screenPt.y, item.customIntensity, zoom, m_animTime, windVx, windVy);
        } else if (item.kind == IsoSpriteKind::SteamEffect) {
            const Cell& c = map.getCell(item.cellX, item.cellY);
            const Vec2 screenPt = camera.gridTileCenterScreen(item.cellX, item.cellY, c.elevation);
            drawSteamEffect(screenPt.x, screenPt.y, item.customIntensity, zoom, m_animTime);
        } else if (item.kind == IsoSpriteKind::Whirlpool) {
            const Cell& c = map.getCell(item.cellX, item.cellY);
            const float whirlpoolElevation = c.isWater() ? 0.50f : c.elevation;
            const Vec2 screenPt = camera.gridTileCenterScreen(item.cellX, item.cellY, whirlpoolElevation);
            drawWhirlpool(screenPt.x, screenPt.y, zoom, m_animTime, item.customIntensity);
        }
    }

    // =========================================================================
    // EN: LAYER 3 - OVERHEAD CELESTIAL & ATMOSPHERIC EFFECTS
    // Shamanic solar pillars and celestial rays radiating into the sky above.
    // ES: CAPA 3 - EFECTOS CELESTIALES Y ATMOSFÉRICOS SUPERIORES
    // Haces solares chamánicos y rayos celestiales radiando hacia el cielo superior.
    // =========================================================================
    for (const auto& villagerUnit : humanos.getHumanos()) {
        if (!villagerUnit.estaVivo() || !villagerUnit.esChaman() || isVillagerInsideHouse(villagerUnit)) continue;
        if (villagerUnit.getTimerSenalDivina() <= 0.0f) continue;
        if (isHiddenByFog(villagerUnit.getCeldaX(), villagerUnit.getCeldaY(), villagerUnit.getReligionId())) continue;

        int cellCoordX = std::clamp(villagerUnit.getCeldaX(), 0, w - 1);
        int cellCoordY = std::clamp(villagerUnit.getCeldaY(), 0, h - 1);
        Vec2 screenPos = camera.gridToScreen(villagerUnit.getX(), villagerUnit.getY(), map.getCell(cellCoordX, cellCoordY).elevation);
        int beamX = static_cast<int>(screenPos.x);
        int beamYBot = static_cast<int>(screenPos.y);
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
    renderTopBar(map, registry, timeline, playerConfig, &humanos, advSystem);
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
                            const Timeline& timeline, const WorldCreationConfig& playerConfig,
                            const GestorHumanos* humanos,
                            const AdvancementSystem* advSystem) {
    constexpr int topH = 34;

    // EN: Region 1: Village & Resource Info Panel (Left - 42% width)
    // ES: Región 1: Panel de Información de Pueblo y Recursos (Izquierda - 42% ancho)
    const int grisW = static_cast<int>(m_width * 0.42f);
    drawSims2Panel(0, 0, grisW, topH, ColorRGB{18, 11, 10}, false, ColorRGB{75, 52, 38});
    drawLine(0, topH, grisW, topH, Palette::DIVINE_GOLD);

    // EN: Extract player religion state, follower counts and available community resources
    // ES: Extraer estado de la religión del jugador, conteo de fieles y recursos comunitarios disponibles
    const Religion* playerReligion = registry.getReligion(1);
    const Religion* pRel = playerReligion;
    const int playerFollowers = playerReligion ? playerReligion->totalFollowers : 0;
    const int pietyPoints = playerReligion ? playerReligion->theologyPoints : 1000;
    const int totalWorldPopulation = map.getTotalWorldPopulation();
    const int livingBelievers = humanos ? humanos->countLivingHumansOfReligion(1) : playerFollowers;

    // EN: Top row: God, Cult Name and Global World Population summary
    // ES: Fila superior: Dios, Nombre del Culto y resumen de Población Mundial Global
    std::string godDisplay = "DIOS: " + playerConfig.godName + " | CULTO: " + playerConfig.religionName;
    if (totalWorldPopulation > 0) {
        godDisplay += " | MUNDO: " + std::to_string(totalWorldPopulation) + " hab.";
    }
    drawText(10, 4, godDisplay.substr(0, 52), Palette::DIVINE_GOLD, 1);

    // EN: Community survival & mineral resources (Food, Wood, Stone, Copper, Iron, Gold, Diamond, Faith, and Believers)
    // ES: Recursos de supervivencia y minerales de la comunidad (Comida, Madera, Piedra, Cobre, Hierro, Oro, Diamante, Fe y Creyentes)
    const int foodAmount = playerReligion ? playerReligion->resources.food : 500;
    const int woodAmount = playerReligion ? playerReligion->resources.wood : 300;
    const int stoneAmount = playerReligion ? playerReligion->resources.stone : 150;
    const int copperAmount = playerReligion ? playerReligion->resources.copper : 0;
    const int ironAmount = playerReligion ? playerReligion->resources.iron : 80;
    const int goldAmount = playerReligion ? playerReligion->resources.gold : 50;
    const int diamondAmount = playerReligion ? playerReligion->resources.diamond : 0;
    const bool isStarving = playerReligion ? playerReligion->resources.isStarving : false;

    int currentResourceX = 10;
    constexpr int resourceTextRowY = 17;
    constexpr int characterWidth = 6;
    constexpr int resourceElementGap = 6;

    std::string foodStr = "COM: " + std::to_string(foodAmount);
    if (isStarving) foodStr += " (!)";
    const ColorRGB foodColor = isStarving ? ColorRGB{255, 60, 60} : (foodAmount < 100 ? ColorRGB{255, 180, 50} : ColorRGB{120, 220, 100});
    drawText(currentResourceX, resourceTextRowY, foodStr, foodColor, 1);
    currentResourceX += static_cast<int>(foodStr.size() * characterWidth) + resourceElementGap;

    std::string woodStr = "MAD: " + std::to_string(woodAmount);
    drawText(currentResourceX, resourceTextRowY, woodStr, ColorRGB{210, 155, 95}, 1);
    currentResourceX += static_cast<int>(woodStr.size() * characterWidth) + resourceElementGap;

    std::string stoneStr = "PIE: " + std::to_string(stoneAmount);
    drawText(currentResourceX, resourceTextRowY, stoneStr, ColorRGB{180, 195, 210}, 1);
    currentResourceX += static_cast<int>(stoneStr.size() * characterWidth) + resourceElementGap;

    std::string copperStr = "COB: " + std::to_string(copperAmount);
    drawText(currentResourceX, resourceTextRowY, copperStr, ColorRGB{210, 115, 60}, 1);
    currentResourceX += static_cast<int>(copperStr.size() * characterWidth) + resourceElementGap;

    std::string ironStr = "HIE: " + std::to_string(ironAmount);
    drawText(currentResourceX, resourceTextRowY, ironStr, ColorRGB{160, 185, 210}, 1);
    currentResourceX += static_cast<int>(ironStr.size() * characterWidth) + resourceElementGap;

    std::string goldStr = "ORO: " + std::to_string(goldAmount);
    drawText(currentResourceX, resourceTextRowY, goldStr, ColorRGB{255, 215, 60}, 1);
    currentResourceX += static_cast<int>(goldStr.size() * characterWidth) + resourceElementGap;

    std::string diamondStr = "DIA: " + std::to_string(diamondAmount);
    drawText(currentResourceX, resourceTextRowY, diamondStr, ColorRGB{120, 220, 255}, 1);
    currentResourceX += static_cast<int>(diamondStr.size() * characterWidth) + resourceElementGap;

    std::string faithStr = "| FE: " + std::to_string(pietyPoints);
    drawText(currentResourceX, resourceTextRowY, faithStr, Palette::RELIC_BONE, 1);
    currentResourceX += static_cast<int>(faithStr.size() * characterWidth) + resourceElementGap;

    // EN: Dedicated Believer / Faithful indicator in the resource panel with illuminated devotional icon
    // ES: Indicador dedicado de Creyentes / Fieles en el panel de recursos con icono devocional iluminado
    // Purpose: Provide immediate visibility of total believers alongside economic resources
    // Mechanism: Render a 7x8 pixel art prayer silhouette and formatted follower count
    // Preconditions: playerFollowers represents current territorial faith adherents
    const int iconBelieverX = currentResourceX + 2;
    const int iconBelieverY = resourceTextRowY;

    // EN: Sacred halo (Divine Gold) / ES: Aureola sagrada (Oro Divino)
    drawLine(iconBelieverX + 1, iconBelieverY, iconBelieverX + 5, iconBelieverY, Palette::DIVINE_GOLD);
    // EN: Devotee head (Relic Bone) / ES: Cabeza del devoto (Hueso Reliquia)
    drawLine(iconBelieverX + 2, iconBelieverY + 2, iconBelieverX + 4, iconBelieverY + 2, Palette::RELIC_BONE);
    drawLine(iconBelieverX + 2, iconBelieverY + 3, iconBelieverX + 4, iconBelieverY + 3, Palette::RELIC_BONE);
    // EN: Liturgical prayer tunic (Divine Gold) / ES: Túnica litúrgica de oración (Oro Divino)
    drawLine(iconBelieverX + 1, iconBelieverY + 5, iconBelieverX + 5, iconBelieverY + 5, Palette::DIVINE_GOLD);
    drawLine(iconBelieverX, iconBelieverY + 6, iconBelieverX + 6, iconBelieverY + 6, Palette::DIVINE_GOLD);
    drawLine(iconBelieverX, iconBelieverY + 7, iconBelieverX + 6, iconBelieverY + 7, Palette::DIVINE_GOLD);

    const int textBelieverX = iconBelieverX + 9;
    std::string believersStr = "CREYENTES: " + std::to_string(playerFollowers);
    if (humanos && livingBelievers > 0 && livingBelievers != playerFollowers) {
        believersStr += " (" + std::to_string(livingBelievers) + " vivos)";
    }
    drawText(textBelieverX, resourceTextRowY, believersStr, Palette::DIVINE_GOLD, 1);

    // EN: Region 2: Central Sacred Era & Season Banner (Center - 14% width)
    // ES: Región 2: Banner Central Sagrado de Era y Estación (Centro - 14% ancho)
    // Purpose: Display active historical era with advancement progression (X/Y) and liturgical season
    // Mechanism: Paper satin texture with double gold border, displaying era and formatted year with season
    // Preconditions: pRel may be null in panorama mode; fallback to world year calculation
    const int blancaX = grisW;
    const int blancaW = static_cast<int>(m_width * 0.14f);
    drawSims2Panel(blancaX, 0, blancaW, topH, ColorRGB{245, 245, 240}, false, Palette::DIVINE_GOLD);
    drawLine(blancaX, topH, blancaX + blancaW, topH, Palette::DIVINE_GOLD);

    // Ranuras decorativas entre paneles
    drawLine(blancaX - 1, 0, blancaX - 1, topH, ColorRGB{10, 6, 5});
    drawLine(blancaX, 0, blancaX, topH, ColorRGB{75, 52, 38});
    drawLine(blancaX + blancaW, 0, blancaX + blancaW, topH, ColorRGB{10, 6, 5});

    EraType curEra = AdvancementSystem::getEraForYear(timeline.getCurrentYear());
    if (pRel) curEra = pRel->currentEra;
    std::string eraNameShort = AdvancementSystem::getEraName(curEra);
    if (eraNameShort.size() > 14) eraNameShort = eraNameShort.substr(0, 14);

    // EN: Append era advancement completion progress (e.g. "(2/4)")
    // ES: Añadir progreso de convalidación de la era (ej. "(2/4)")
    if (advSystem && pRel) {
        const int convalidatedInEra = advSystem->getConvalidatedCountForEra(*pRel, curEra);
        const int totalInEra = advSystem->getTotalAdvancementsInEra(curEra);
        eraNameShort += " (" + std::to_string(convalidatedInEra) + "/" + std::to_string(totalInEra) + ")";
    }

    const int eraTxtX = blancaX + (blancaW - static_cast<int>(eraNameShort.size() * 6)) / 2;
    drawText(eraTxtX, 5, eraNameShort, ColorRGB{20, 20, 24}, 1);

    std::string yearStr = Timeline::formatYear(timeline.getCurrentYear());
    if (m_dayNight) {
        yearStr += " [" + m_dayNight->getSeasonName() + "]";
    }
    const int yearTxtX = blancaX + (blancaW - static_cast<int>(yearStr.size() * 6)) / 2;
    drawText(yearTxtX, 18, yearStr, Palette::BLOOD_RUBY, 1);

    // EN: Region 3: Options, Divine Controls and Celestial Insignia (Right - Remaining width)
    // ES: Región 3: Opciones, Controles Divinos e Insignia Celestial (Derecha - Ancho restante)
    const int rojaX = blancaX + blancaW;
    const int rojaW = m_width - rojaX;
    drawSims2Panel(rojaX, 0, rojaW, topH, ColorRGB{18, 11, 10}, false, ColorRGB{75, 52, 38});
    drawLine(rojaX, topH, m_width, topH, Palette::DIVINE_GOLD);
    drawLine(rojaX, 0, rojaX, topH, ColorRGB{75, 52, 38});

    // Insignia Litúrgica Celestial: Día / Noche, Fase y Reloj Solar
    if (m_dayNight) {
        const int badgeX = rojaX + 8;
        const int badgeY = 5;
        const int badgeW = 120;
        const int badgeH = 24;

        const bool isDayTime = m_dayNight->isDay();
        const ColorRGB badgeRim = isDayTime ? Palette::DIVINE_GOLD : ColorRGB{120, 160, 240};
        const ColorRGB badgeBase = isDayTime ? ColorRGB{28, 18, 14} : ColorRGB{14, 18, 30};
        drawSims2Panel(badgeX, badgeY, badgeW, badgeH, badgeBase, false, badgeRim);

        const int iconCenterX = badgeX + 13;
        const int iconCenterY = badgeY + 12;

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

            const std::string timeStr = m_dayNight->getPhaseName() + " " + m_dayNight->getTimeFormatted();
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
            const std::string timeStr = m_dayNight->getPhaseName() + " " + m_dayNight->getTimeFormatted();
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
        const VillagerCondition leaderCondition = selHumano->getCondition();
        if (selHumano->esProfeta()) {
            drawText(8, botY + 44, "DON: Cruza Aguas y Peligros", Palette::DIVINE_GOLD, 1);
        } else {
            drawText(8, botY + 44, "LIDER: Conduce Milagros", Palette::DIVINE_GOLD, 1);
        }

        if (leaderCondition != VillagerCondition::Healthy) {
            drawText(8, botY + 56, ("Estado: [" + selHumano->getConditionName() + "]").substr(0, 23),
                     selHumano->getConditionColor(), 1);
        } else if (selHumano->esProfeta()) {
            drawText(8, botY + 56, "Unidad bendecida milagrosa", Palette::RELIC_BONE, 1);
        } else {
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

            // EN: Systemic condition badge of villager (Hungry, Sick, Healthy, Drunk, Full, Aggressive, Scared, etc.)
            // ES: Placa de condición sistémica del aldeano (Hambriento, Enfermo, Sano, Borracho, Lleno, Agresivo, Asustado, etc.)
            const VillagerCondition villagerCondition = selHumano->getCondition();
            const std::string conditionTitle = "Estado: [" + selHumano->getConditionName() + "]";
            const ColorRGB conditionBadgeColor = selHumano->getConditionColor();
            drawText(8, botY + 56, conditionTitle.substr(0, 23), conditionBadgeColor, 1);

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

            // EN: Active behavioral description corresponding to the current condition
            // ES: Descripción conductual activa correspondiente a la condición actual
            std::string conditionBehaviorSubtitle = "";
            switch (villagerCondition) {
                case VillagerCondition::Drunk:
                    conditionBehaviorSubtitle = "Borracho " + std::to_string(static_cast<int>(selHumano->getAlcoholLevel())) + "%: Euforia";
                    break;
                case VillagerCondition::Hungry:
                    conditionBehaviorSubtitle = "Hambre: Busca Alimento";
                    break;
                case VillagerCondition::Full:
                    conditionBehaviorSubtitle = "Lleno: Vigor (+Regen)";
                    break;
                case VillagerCondition::Sick:
                    conditionBehaviorSubtitle = "Enfermo: Busca Reposo";
                    break;
                case VillagerCondition::Aggressive:
                    conditionBehaviorSubtitle = "!Furia: +35% Ataque!";
                    break;
                case VillagerCondition::Scared:
                    conditionBehaviorSubtitle = "!Panico: Huyendo!";
                    break;
                case VillagerCondition::Exhausted:
                    conditionBehaviorSubtitle = "Exhausto: Busca Dormir";
                    break;
                case VillagerCondition::Inspired:
                    conditionBehaviorSubtitle = "Inspirado: Extasis";
                    break;
                case VillagerCondition::Injured:
                    conditionBehaviorSubtitle = "Herido: Salud Critica";
                    break;
                default:
                    break;
            }

            const auto& moralRecord = selHumano->getMoralRecord();
            if (!conditionBehaviorSubtitle.empty() && villagerCondition != VillagerCondition::Healthy) {
                drawText(8, botY + 70, conditionBehaviorSubtitle.substr(0, 28), conditionBadgeColor, 1);
            } else if (selHumano->tieneFuriaProtectora()) {
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
            case 726: return m_activeDivinePower == DivinePowerType::Rain;
            case 727: return m_activeDivinePower == DivinePowerType::Deluge;
            default:  return false;
        }
    };
    auto drawPanelButton = [&](const UIButton& btn) {
        bool active = isButtonActive(btn.id);
        bool hovered = false;
        if (btn.id == 202) {
            float radius = btn.width * 0.5f;
            float centerX = btn.x + radius;
            float centerY = btn.y + radius;
            float deltaX = m_mouseX - centerX;
            float deltaY = m_mouseY - centerY;
            hovered = (deltaX * deltaX + deltaY * deltaY <= radius * radius);
        } else {
            hovered = (m_mouseX >= btn.x && m_mouseX <= btn.x + btn.width &&
                       m_mouseY >= btn.y && m_mouseY <= btn.y + btn.height);
        }
        bool pressed = (m_pressedButtonId == btn.id) || (hovered && m_isMouseDown);

        if (btn.id == 202) {
            drawRoundPlayButton(static_cast<int>(btn.x), static_cast<int>(btn.y),
                                static_cast<int>(btn.width), active, hovered, pressed);
        } else if (btn.id == 201 || btn.id == 203 || btn.id == 204) {
            drawCassetteButton(static_cast<int>(btn.x), static_cast<int>(btn.y),
                               static_cast<int>(btn.width), static_cast<int>(btn.height),
                               btn.label, active, hovered, pressed, btn.id);
        } else if (btn.id >= 101 && btn.id <= 105) {
            drawViewModeButton(static_cast<int>(btn.x), static_cast<int>(btn.y),
                               static_cast<int>(btn.width), static_cast<int>(btn.height),
                               btn.id, btn.label, active, hovered, pressed);
        } else if ((btn.id >= 710 && btn.id <= 713) || (btn.id >= 720 && btn.id <= 727) ||
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

    // EN: Recessed cassette chassis well behind simulation controls (201..204)
    // ES: Bandeja de chasis de casetera empotrada detrás de los controles de simulación (201..204)
    {
        const UIButton* btnPause = nullptr;
        const UIButton* btnUltra = nullptr;
        for (const auto& btn : m_buttons) {
            if (btn.id == 201) btnPause = &btn;
            if (btn.id == 204) btnUltra = &btn;
        }
        if (btnPause && btnUltra) {
            int wellX = static_cast<int>(btnPause->x) - 6;
            int wellY = static_cast<int>(btnPause->y) - 4;
            int wellW = static_cast<int>(btnUltra->x + btnUltra->width - btnPause->x) + 12;
            int wellH = static_cast<int>(btnPause->height) + 8;

            // EN: Draw recessed chassis plate with inner drop shadow and rim highlight
            // ES: Dibujar placa de chasis empotrada con sombra interior y realce en el borde
            drawRect(wellX, wellY, wellW, wellH, ColorRGB{12, 8, 7});
            drawRectOutline(wellX, wellY, wellW, wellH, ColorRGB{32, 22, 16});
            drawLine(wellX + 1, wellY + 1, wellX + wellW - 2, wellY + 1, ColorRGB{6, 4, 3});
            drawLine(wellX + 1, wellY + 2, wellX + wellW - 2, wellY + 2, ColorRGB{8, 5, 4});
            drawLine(wellX + 1, wellY + 1, wellX + 1, wellY + wellH - 2, ColorRGB{6, 4, 3});
            drawLine(wellX + 1, wellY + wellH - 1, wellX + wellW - 1, wellY + wellH - 1, ColorRGB{55, 38, 25});
            drawLine(wellX + wellW - 1, wellY + 1, wellX + wellW - 1, wellY + wellH - 1, ColorRGB{55, 38, 25});
        }
    }

    for (const auto& btn : m_buttons) {
        if (btn.y < botY || btn.x < azulX || btn.x >= azulX + azulW) continue; // zona central
        drawPanelButton(btn);
    }

    // Separador litúrgico vertical entre Poderes Divinos y Catástrofes Naturales
    {
        const float powSize = 25.0f;
        const float gap = 5.0f;
        const float sepGap = 12.0f;
        const float totalWidth = (4 * powSize + 3 * gap) + sepGap + (8 * powSize + 7 * gap);
        const float startX = static_cast<float>(azulX) + (static_cast<float>(azulW) - totalWidth) * 0.5f;
        int sepX = static_cast<int>(startX + 4 * powSize + 3 * gap + sepGap * 0.5f);
        int sepY = static_cast<int>(botY + 54.0f);
        drawLine(sepX, sepY + 3, sepX, sepY + 23, ColorRGB{140, 110, 30});
        drawLine(sepX - 1, sepY + 3, sepX - 1, sepY + 23, ColorRGB{20, 14, 10});
    }

    // Tooltips flotantes para los botones de poderes al pasar el cursor (hover)
    for (const auto& btn : m_buttons) {
        if (btn.y < botY || btn.x < azulX || btn.x >= azulX + azulW) continue;
        bool isPower = (btn.id >= 710 && btn.id <= 713) || (btn.id >= 720 && btn.id <= 727);
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
                case 726:           tip = "[L] LLUVIA BENDITA (30 Fe)"; break;
                case 727:           tip = "[J] GRAN DILUVIO (65 Fe)"; break;
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
        } else if (m_activeDivinePower == DivinePowerType::Rain) {
            pName = "LLUVIA BENDITA";
            pCol = ColorRGB{90, 190, 255};
        } else if (m_activeDivinePower == DivinePowerType::Deluge) {
            pName = "GRAN DILUVIO CATACLÍSMICO";
            pCol = ColorRGB{35, 120, 240};
        } else if (m_activeDivinePower == DivinePowerType::AtomicBomb) {
            pName = "EXPLOSIÓN ATÓMICA [FUCKINGBOMB]";
            pCol = ColorRGB{255, 235, 90};
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
        std::string infoTxt = "INTERMEDIARIO: " + leadTxt + "  |  PODERES: [Z][X][V][C]  CATÁSTROFES: [T][G][B][M][K][F][L][J]";
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

            // EN: Depict permanent craters, seismic faults, and impact scars on minimap
            // ES: Representar cráteres permanentes, fallas sísmicas y cicatrices de impacto en el minimapa
            if (c.hasImpactCrater && cellVisible) {
                if (c.craterType == CraterType::MeteorImpact) {
                    col = ColorRGB{200, 60, 20};
                } else if (c.craterType == CraterType::TectonicSeismic) {
                    col = ColorRGB{42, 34, 30};
                } else if (c.craterType == CraterType::NuclearDetonation) {
                    col = ColorRGB{85, 125, 95};
                } else {
                    col = ColorRGB{65, 60, 58};
                }
            }

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
        "Dense Forest", "Highland Hills", "Arid Desert", "Rugged Mountains", "Frozen Tundra", "Volcanic Caldera"
    };
    std::string bName = BIOME_NAMES[static_cast<int>(c.biome)];
    drawText(panelX + 15, panelY + 52, "Biome: " + bName, ColorRGB(226, 218, 196), 1); // Hueso Reliquia

    std::string popStr = "Population: " + std::to_string(c.population) + " / " + std::to_string(c.maxPopulation);
    drawText(panelX + 15, panelY + 68, popStr, ColorRGB(226, 218, 196), 1);

    // EN: Cell natural resource inspector with scarcity and depletion indicators
    // ES: Inspector de recursos naturales con indicadores de escasez y agotamiento
    std::string resStr = "Resource: ";
    ColorRGB resColor{212, 175, 55};
    if (c.naturalResource != ResourceType::None && c.resourceAmount > 0) {
        resStr += std::string(getResourceTypeName(c.naturalResource)) + " (" +
                  std::to_string(c.resourceAmount) + " / " + std::to_string(c.maxResourceAmount) + ")";
        if (c.maxResourceAmount > 0 && c.resourceAmount <= c.maxResourceAmount / 4) {
            resStr += " [Escaso]";
            resColor = ColorRGB{235, 125, 45};
        }
    } else if (c.isResourceDepleted() || c.maxResourceAmount > 0) {
        resStr += "Agotado (0 / " + std::to_string(c.maxResourceAmount) + ")";
        resColor = ColorRGB{150, 140, 135};
    } else {
        resStr += "Ninguno";
        resColor = ColorRGB{170, 160, 150};
    }
    drawText(panelX + 15, panelY + 84, resStr, resColor, 1);

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
            drawText(panelX + 15, curY + 66, "Creyentes: " + std::to_string(dom->totalFollowers), ColorRGB(230, 161, 59), 1); // Ocre Dorado
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
    // EN: The simulated world is rendered underneath via renderWorldBackground()
    // ES: El mundo simulado se renderiza por debajo a través de renderWorldBackground()

    // Marco ornamental de toda la pantalla (Borde exterior de catedral)
    drawRectOutline(12, 12, m_width - 24, m_height - 24, ColorRGB{77, 77, 77}); // #4D4D4D
    drawRectOutline(16, 16, m_width - 32, m_height - 32, Palette::DIVINE_GOLD); // #E6A13B

    // Esquinas ornamentales del marco principal
    constexpr int CORNER = 14;
    drawRect(12, 12, CORNER, CORNER, Palette::DIVINE_GOLD);
    drawRect(m_width - 12 - CORNER, 12, CORNER, CORNER, Palette::DIVINE_GOLD);
    drawRect(12, m_height - 12 - CORNER, CORNER, CORNER, Palette::DIVINE_GOLD);
    drawRect(m_width - 12 - CORNER, m_height - 12 - CORNER, CORNER, CORNER, Palette::DIVINE_GOLD);

    // Título Central del Juego (Tipografía Sagrada en Oro Divino con Sombra Proyectada)
    const std::string title1 = "MARANGATU: THE FAITH SIMULATOR";
    const std::string title2 = "2.5D ISOMETRIC RELIGION SIMULATOR";
    const std::string subtitle = "Pre-Alpha v0.4.0 - Edicion Liturgica y Espiritual";

    const int title1X = (m_width - static_cast<int>(title1.size() * 12)) / 2;
    const int title2X = (m_width - static_cast<int>(title2.size() * 6)) / 2;
    const int subX = (m_width - static_cast<int>(subtitle.size() * 6)) / 2;

    const int titleY = 105;
    // EN: Drop shadow for sharp high-contrast text against the living simulated world
    // ES: Sombra proyectada para texto de alto contraste sobre el mundo simulado vivo
    drawText(title1X + 2, titleY + 2, title1, ColorRGB{10, 8, 6}, 2);
    drawText(title1X, titleY, title1, Palette::DIVINE_GOLD, 2);

    drawText(title2X + 1, titleY + 33, title2, ColorRGB{10, 8, 6}, 1);
    drawText(title2X, titleY + 32, title2, Palette::RELIC_BONE, 1);

    drawText(subX + 1, titleY + 51, subtitle, ColorRGB{10, 8, 6}, 1);
    drawText(subX, titleY + 50, subtitle, ColorRGB{204, 181, 189}, 1); // Lino Litúrgico #CCB5BD

    // Línea divisoria ceremonial bajo el título
    const int lineW = 440;
    const int lineX = (m_width - lineW) / 2;
    drawLine(lineX, titleY + 68, lineX + lineW, titleY + 68, Palette::DIVINE_GOLD);
    drawRect(m_width / 2 - 4, titleY + 65, 8, 8, Palette::BLOOD_RUBY); // Gema central

    // EN: Container panel for centered liturgical buttons (translucent sacred reliquary)
    // ES: Panel contenedor de los botones centrados litúrgicos (relicario central translúcido)
    constexpr float buttonPanelWidth = 290.0f;
    constexpr float buttonItemHeight = 44.0f;
    constexpr float buttonItemSpacing = 14.0f;
    constexpr int mainButtonCount = 6;
    const float totalButtonClusterHeight = mainButtonCount * buttonItemHeight + (mainButtonCount - 1) * buttonItemSpacing;

    constexpr int panelPaddingOffset = 22;
    const int panelPosX = static_cast<int>((m_width - buttonPanelWidth) * 0.5f) - panelPaddingOffset;
    const int panelPosY = static_cast<int>((m_height - totalButtonClusterHeight) * 0.5f) + 36 - panelPaddingOffset;
    const int panelPixelWidth = static_cast<int>(buttonPanelWidth) + panelPaddingOffset * 2;
    const int panelPixelHeight = static_cast<int>(totalButtonClusterHeight) + panelPaddingOffset * 2;

    // EN: Central liturgical altar panel using in-game sacred wood tones and satin specular volume
    // ES: Panel del altar litúrgico central usando los tonos de madera sacra y volumen satinado de la interfaz de juego
    drawSims2Panel(panelPosX, panelPosY, panelPixelWidth, panelPixelHeight, ColorRGB{22, 14, 12}, false, Palette::DIVINE_GOLD);
    drawRectOutline(panelPosX, panelPosY, panelPixelWidth, panelPixelHeight, ColorRGB{75, 52, 38});
    drawRectOutline(panelPosX + 2, panelPosY + 2, panelPixelWidth - 4, panelPixelHeight - 4, Palette::DIVINE_GOLD);
    drawRectOutline(panelPosX + 3, panelPosY + 3, panelPixelWidth - 6, panelPixelHeight - 6, ColorRGB{120, 85, 55});

    // Gemas Blood Ruby en las esquinas del relicario central
    constexpr int cornerGemDimension = 8;
    drawRect(panelPosX + 3, panelPosY + 3, cornerGemDimension, cornerGemDimension, Palette::BLOOD_RUBY);
    drawRect(panelPosX + panelPixelWidth - 3 - cornerGemDimension, panelPosY + 3, cornerGemDimension, cornerGemDimension, Palette::BLOOD_RUBY);
    drawRect(panelPosX + 3, panelPosY + panelPixelHeight - 3 - cornerGemDimension, cornerGemDimension, cornerGemDimension, Palette::BLOOD_RUBY);
    drawRect(panelPosX + panelPixelWidth - 3 - cornerGemDimension, panelPosY + panelPixelHeight - 3 - cornerGemDimension, cornerGemDimension, cornerGemDimension, Palette::BLOOD_RUBY);

    // Renderizar los 4 Botones centrados con la paleta y estética táctil de la interfaz del juego
    for (const auto& btn : m_menuButtons) {
        const bool isHovered = (btn.id == hoveredButtonId);

        const int bx = static_cast<int>(btn.x);
        const int by = static_cast<int>(btn.y);
        const int bw = static_cast<int>(btn.width);
        const int bh = static_cast<int>(btn.height);

        if (isHovered) {
            // EN: In-Game UI Hover Style: Warm golden amber satin volume, glowing double gold rim, and bright relic text
            // ES: Estilo Hover de la UI del Juego: Volumen satinado ámbar dorado cálido, doble borde de oro brillante y texto en oro sacro
            drawSims2Panel(bx, by, bw, bh, ColorRGB{64, 44, 26}, false, Palette::DIVINE_GOLD);
            drawRectOutline(bx, by, bw, bh, Palette::DIVINE_GOLD);
            drawRectOutline(bx + 1, by + 1, bw - 2, bh - 2, ColorRGB{255, 235, 120});

            // EN: Golden ornamental corner brackets
            // ES: Escuadras ornamentales doradas en las esquinas
            constexpr int cornerBracketSize = 5;
            drawRect(bx, by, cornerBracketSize, cornerBracketSize, ColorRGB{255, 240, 150});
            drawRect(bx + bw - cornerBracketSize, by, cornerBracketSize, cornerBracketSize, ColorRGB{255, 240, 150});
            drawRect(bx, by + bh - cornerBracketSize, cornerBracketSize, cornerBracketSize, ColorRGB{255, 240, 150});
            drawRect(bx + bw - cornerBracketSize, by + bh - cornerBracketSize, cornerBracketSize, cornerBracketSize, ColorRGB{255, 240, 150});

            // EN: Drop-shadowed radiant gold text
            // ES: Texto dorado radiante con sombra grabada
            const int textX = bx + (bw - static_cast<int>(btn.label.size() * 6)) / 2;
            const int textY = by + (bh - 7) / 2;
            drawText(textX + 1, textY + 1, btn.label, ColorRGB{16, 10, 6}, 1);
            drawText(textX, textY, btn.label, ColorRGB{255, 248, 225}, 1);
        } else {
            // EN: In-Game UI Normal Style: Warm terracotta mahogany wood satin volume (#2A1A14), antique bronze rim (#8C663A), and relic bone text
            // ES: Estilo Normal de la UI del Juego: Volumen satinado de madera caoba/terracota cálida (#2A1A14), borde de bronce antiguo (#8C663A) y texto en hueso de reliquia
            drawSims2Panel(bx, by, bw, bh, ColorRGB{42, 26, 20}, false, ColorRGB{140, 102, 60});
            drawRectOutline(bx, by, bw, bh, ColorRGB{85, 58, 38}); // Carved dark wood border
            drawRectOutline(bx + 1, by + 1, bw - 2, bh - 2, ColorRGB{145, 108, 65}); // Antique bronze inner rim
            drawLine(bx + 2, by + 2, bx + bw - 3, by + 2, ColorRGB{195, 150, 85}); // Upper specular highlight
            drawLine(bx + 2, by + bh - 2, bx + bw - 3, by + bh - 2, ColorRGB{24, 14, 10}); // Lower depth shadow

            // EN: Subtle Blood Ruby gemstone corner accents identical to in-game shrines and panels
            // ES: Acentos sutiles de gema Blood Ruby en las esquinas idénticos a los santuarios y paneles del juego
            drawRect(bx + 2, by + 2, 3, 3, Palette::BLOOD_RUBY);
            drawRect(bx + bw - 5, by + 2, 3, 3, Palette::BLOOD_RUBY);
            drawRect(bx + 2, by + bh - 5, 3, 3, Palette::BLOOD_RUBY);
            drawRect(bx + bw - 5, by + bh - 5, 3, 3, Palette::BLOOD_RUBY);

            // EN: Relic bone typography with tactile carved drop-shadow
            // ES: Tipografía en hueso de reliquia con sombra tallada táctil
            const int textX = bx + (bw - static_cast<int>(btn.label.size() * 6)) / 2;
            const int textY = by + (bh - 7) / 2;
            drawText(textX + 1, textY + 1, btn.label, ColorRGB{16, 10, 8}, 1);
            drawText(textX, textY, btn.label, Palette::RELIC_BONE, 1);
        }
    }

    // Pie de página de la pantalla principal con indicación de panorama activo estilo Minecraft
    const std::string footer = "MUNDO VIVO SIMULADO | PANORAMA MINECRAFT | PRE-ALPHA v0.4.0";
    const int footerX = (m_width - static_cast<int>(footer.size() * 6)) / 2;
    drawText(footerX + 1, m_height - 34, footer, ColorRGB{10, 8, 6}, 1);
    drawText(footerX, m_height - 35, footer, ColorRGB{200, 195, 180}, 1);
}

void Renderer::renderOptionsMenu(int hoveredButtonId) {
    // EN: Backward-compatible fallback overload
    // ES: Sobrecarga de reserva compatible hacia atrás
    GameSettings defaultSettings;
    renderOptionsMenu(defaultSettings, OptionsTab::ScreenAndVideo, hoveredButtonId);
}

void Renderer::renderOptionsMenu(const GameSettings& settings, OptionsTab currentTab, int hoveredButtonId) {
    // EN: Translucent backdrop veil over the simulated world background
    // ES: Velo de fondo translúcido sobre el mundo simulado de fondo
    drawRect(0, 0, m_width, m_height, ColorRGB{14, 12, 16, 175});

    // Marco exterior de la pantalla
    drawRectOutline(12, 12, m_width - 24, m_height - 24, ColorRGB{77, 77, 77});
    drawRectOutline(16, 16, m_width - 32, m_height - 32, Palette::DIVINE_GOLD);

    constexpr float modalWidth = 960.0f;
    constexpr float modalHeight = 590.0f;
    const int modalStartX = static_cast<int>((static_cast<float>(m_width) - modalWidth) * 0.5f);
    const int modalStartY = static_cast<int>((static_cast<float>(m_height) - modalHeight) * 0.5f);
    const int modalPixelWidth = static_cast<int>(modalWidth);
    const int modalPixelHeight = static_cast<int>(modalHeight);

    // Fondo del relicario de opciones con cristal ahumado litúrgico
    drawRect(modalStartX, modalStartY, modalPixelWidth, modalPixelHeight, ColorRGB{18, 16, 22, 230});
    drawRectOutline(modalStartX, modalStartY, modalPixelWidth, modalPixelHeight, ColorRGB{75, 75, 80});
    drawRectOutline(modalStartX + 3, modalStartY + 3, modalPixelWidth - 6, modalPixelHeight - 6, Palette::DIVINE_GOLD);

    // Gemas Blood Ruby en las cuatro esquinas
    constexpr int cornerGemDimension = 10;
    drawRect(modalStartX + 3, modalStartY + 3, cornerGemDimension, cornerGemDimension, Palette::BLOOD_RUBY);
    drawRect(modalStartX + modalPixelWidth - 3 - cornerGemDimension, modalStartY + 3, cornerGemDimension, cornerGemDimension, Palette::BLOOD_RUBY);
    drawRect(modalStartX + 3, modalStartY + modalPixelHeight - 3 - cornerGemDimension, cornerGemDimension, cornerGemDimension, Palette::BLOOD_RUBY);
    drawRect(modalStartX + modalPixelWidth - 3 - cornerGemDimension, modalStartY + modalPixelHeight - 3 - cornerGemDimension, cornerGemDimension, cornerGemDimension, Palette::BLOOD_RUBY);

    // Título superior con sombra proyectada
    const std::string titleText = "AJUSTES Y OPCIONES DEL SISTEMA";
    const int titlePositionX = modalStartX + (modalPixelWidth - static_cast<int>(titleText.size() * 12)) / 2;
    drawText(titlePositionX + 1, modalStartY + 17, titleText, ColorRGB{10, 8, 6}, 2);
    drawText(titlePositionX, modalStartY + 16, titleText, Palette::DIVINE_GOLD, 2);

    // Línea divisoria ceremonial bajo las pestañas
    drawLine(modalStartX + 35, modalStartY + 78, modalStartX + modalPixelWidth - 35, modalStartY + 78, Palette::DIVINE_GOLD);
    drawRect(modalStartX + modalPixelWidth / 2 - 4, modalStartY + 75, 8, 8, Palette::BLOOD_RUBY);

    // 1. Dibujar Pestañas Superiores de Categorías
    for (const auto& tabButton : m_optionsTabButtons) {
        bool isCurrentActiveTab = false;
        if (tabButton.id == 1001 && currentTab == OptionsTab::ScreenAndVideo) isCurrentActiveTab = true;
        else if (tabButton.id == 1002 && currentTab == OptionsTab::GraphicsAndImage) isCurrentActiveTab = true;
        else if (tabButton.id == 1003 && currentTab == OptionsTab::GameplayAndSimulation) isCurrentActiveTab = true;
        else if (tabButton.id == 1004 && currentTab == OptionsTab::AudioAndSound) isCurrentActiveTab = true;

        const bool isHovered = (tabButton.id == hoveredButtonId);

        const int buttonX = static_cast<int>(tabButton.x);
        const int buttonY = static_cast<int>(tabButton.y);
        const int buttonWidth = static_cast<int>(tabButton.width);
        const int buttonHeight = static_cast<int>(tabButton.height);

        if (isCurrentActiveTab) {
            drawRect(buttonX, buttonY, buttonWidth, buttonHeight, ColorRGB{52, 42, 24});
            drawRectOutline(buttonX, buttonY, buttonWidth, buttonHeight, Palette::DIVINE_GOLD);
            drawLine(buttonX + 4, buttonY + buttonHeight - 2, buttonX + buttonWidth - 4, buttonY + buttonHeight - 2, ColorRGB{255, 235, 120});
            const int textX = buttonX + (buttonWidth - static_cast<int>(tabButton.label.size() * 6)) / 2;
            const int textY = buttonY + (buttonHeight - 7) / 2;
            drawText(textX, textY, tabButton.label, ColorRGB{255, 245, 215}, 1);
        } else if (isHovered) {
            drawRect(buttonX, buttonY, buttonWidth, buttonHeight, ColorRGB{38, 34, 30});
            drawRectOutline(buttonX, buttonY, buttonWidth, buttonHeight, ColorRGB{160, 140, 90});
            const int textX = buttonX + (buttonWidth - static_cast<int>(tabButton.label.size() * 6)) / 2;
            const int textY = buttonY + (buttonHeight - 7) / 2;
            drawText(textX, textY, tabButton.label, Palette::DIVINE_GOLD, 1);
        } else {
            drawRect(buttonX, buttonY, buttonWidth, buttonHeight, ColorRGB{24, 24, 28});
            drawRectOutline(buttonX, buttonY, buttonWidth, buttonHeight, ColorRGB{65, 65, 70});
            const int textX = buttonX + (buttonWidth - static_cast<int>(tabButton.label.size() * 6)) / 2;
            const int textY = buttonY + (buttonHeight - 7) / 2;
            drawText(textX, textY, tabButton.label, Palette::RELIC_BONE, 1);
        }
    }

    // 2. Dibujar Opciones de la Pestaña Activa
    const std::vector<UIButton>* currentControlsList = nullptr;
    switch (currentTab) {
        case OptionsTab::ScreenAndVideo:        currentControlsList = &m_optionsScreenButtons; break;
        case OptionsTab::GraphicsAndImage:      currentControlsList = &m_optionsGraphicsButtons; break;
        case OptionsTab::GameplayAndSimulation: currentControlsList = &m_optionsGameplayButtons; break;
        case OptionsTab::AudioAndSound:          currentControlsList = &m_optionsAudioButtons; break;
    }

    std::string hoveredTooltipText = "Haz clic en una opcion para alternar su valor o navega entre las pestanas superiores.";

    if (currentControlsList) {
        for (const auto& optionButton : *currentControlsList) {
            const bool isHovered = (optionButton.id == hoveredButtonId);

            const int buttonX = static_cast<int>(optionButton.x);
            const int buttonY = static_cast<int>(optionButton.y);
            const int buttonWidth = static_cast<int>(optionButton.width);
            const int buttonHeight = static_cast<int>(optionButton.height);

            std::string labelText = optionButton.label;
            std::string valueText = "";
            int sliderPercentValue = -1;

            switch (optionButton.id) {
                // Pantalla y Video
                case 1010: valueText = GameSettings::getResolutionName(settings.resolution); break;
                case 1011: valueText = GameSettings::getDisplayModeName(settings.displayMode); break;
                case 1012: valueText = GameSettings::getFrameRateLimitName(settings.frameRateLimit); break;
                case 1013: valueText = settings.verticalSync ? "ACTIVADO" : "DESACTIVADO"; break;
                case 1014: valueText = GameSettings::getUiScaleName(settings.uiScale); break;
                case 1015: valueText = GameSettings::getBrightnessName(settings.brightness); break;

                // Gráficos e Imagen
                case 1020: valueText = GameSettings::getCloudShadowsName(settings.cloudShadows); break;
                case 1021: valueText = GameSettings::getDynamicLightingName(settings.dynamicLighting); break;
                case 1022: valueText = GameSettings::getWaterAnimationName(settings.waterAnimation); break;
                case 1023: valueText = settings.vignetteEnabled ? "ACTIVADO" : "DESACTIVADO"; break;
                case 1024: valueText = GameSettings::getScreenShakeName(settings.screenShake); break;
                case 1025: valueText = GameSettings::getVisualColorFilterName(settings.colorFilter); break;
                case 1026: valueText = settings.showIsometricGrid ? "SI (VISIBLE)" : "NO (OCULTA)"; break;
                case 1027: valueText = settings.developerMode ? "SI (ACTIVADO)" : "NO (DESACTIVADO)"; break;

                // Jugabilidad
                case 1030: valueText = GameSettings::getSimulationSpeedName(settings.defaultSpeed); break;
                case 1031: valueText = settings.autoPauseOnDisasters ? "SI (ACTIVADA)" : "NO"; break;
                case 1032: valueText = GameSettings::getCameraPanSpeedName(settings.cameraPanSpeed); break;
                case 1033: valueText = settings.invertCameraDrag ? "SI" : "NO"; break;
                case 1034: valueText = settings.showFloatingNumbers ? "SI" : "NO"; break;
                case 1035: valueText = GameSettings::getLifeEventNotificationsName(settings.eventNotifications); break;

                // Audio
                case 1040: valueText = std::to_string(settings.masterVolumePercent) + "%"; sliderPercentValue = settings.masterVolumePercent; break;
                case 1041: valueText = std::to_string(settings.musicVolumePercent) + "%"; sliderPercentValue = settings.musicVolumePercent; break;
                case 1042: valueText = std::to_string(settings.sfxVolumePercent) + "%"; sliderPercentValue = settings.sfxVolumePercent; break;
                case 1043: valueText = std::to_string(settings.ambientVolumePercent) + "%"; sliderPercentValue = settings.ambientVolumePercent; break;
                case 1044: valueText = settings.soundMuted ? "SI (MUTED)" : "NO"; break;
            }

            if (isHovered) {
                switch (optionButton.id) {
                    case 1010: hoveredTooltipText = "Define la resolucion nativa del lienzo de renderizado del simulador."; break;
                    case 1011: hoveredTooltipText = "Alterna entre ventana estandar de escritorio y pantalla completa sin bordes."; break;
                    case 1012: hoveredTooltipText = "Limita los fotogramas por segundo para optimizar fluidez o ahorrar energia."; break;
                    case 1013: hoveredTooltipText = "Sincroniza la tasa de cuadros con el refresco vertical del monitor (V-Sync)."; break;
                    case 1014: hoveredTooltipText = "Modifica la escala de fuentes, paneles y botones de la interfaz liturgica."; break;
                    case 1015: hoveredTooltipText = "Calibra la luminosidad global y exposicion gamma de la escena."; break;
                    case 1020: hoveredTooltipText = "Proyecta sombras dinamicas de las nubes sobre el terreno isometrico."; break;
                    case 1021: hoveredTooltipText = "Ajusta el gradiente dinamico de amanecer, mediodia, ocaso y noche sacra."; break;
                    case 1022: hoveredTooltipText = "Activa reflejos y crestas translucidas en rios, lagos y oceanos."; break;
                    case 1023: hoveredTooltipText = "Aplica un suave oscurecimiento radial periferico estilo pelicula clasica."; break;
                    case 1024: hoveredTooltipText = "Regula la intensidad de sacudida de pantalla ante terremotos y meteoros."; break;
                    case 1025: hoveredTooltipText = "Aplica gradientes tonales al lienzo: Dorado Sacro, Pergamino o Basalto."; break;
                    case 1026: hoveredTooltipText = "Muestra u oculta la reticula de cuadricula isometrica sobre el mapa del mundo (Tecla rapida: H)."; break;
                    case 1027: hoveredTooltipText = "Muestra telemetria del motor, FPS, RAM, ejes 3D y propiedades de unidades (Tecla rapida: F3)."; break;
                    case 1030: hoveredTooltipText = "Velocidad temporal estandar al iniciar una nueva era de simulacion."; break;
                    case 1031: hoveredTooltipText = "Detiene el tiempo automaticamente al desencadenarse terremotos o meteoritos."; break;
                    case 1032: hoveredTooltipText = "Sensibilidad y aceleracion de traslacion de la camara isometrica."; break;
                    case 1033: hoveredTooltipText = "Invierte el sentido del arrastre al mantener presionado el boton derecho."; break;
                    case 1034: hoveredTooltipText = "Muestra numeros emergentes (+/- fe, dano de combate) sobre las unidades."; break;
                    case 1035: hoveredTooltipText = "Filtro de eventos de nacimiento y fallecimiento en el registro historico."; break;
                    case 1040: hoveredTooltipText = "Ajusta la ganancia maestra global del motor de audio DSP."; break;
                    case 1041: hoveredTooltipText = "Volumen de las campanas, canticos modales y cuernos sagrados."; break;
                    case 1042: hoveredTooltipText = "Volumen de talas, canteras, pasos, impactos fisicos y poderes divinos."; break;
                    case 1043: hoveredTooltipText = "Volumen del viento, oleaje costero, grillos nocturnos y lluvia."; break;
                    case 1044: hoveredTooltipText = "Desactiva de forma instantanea toda la emision de sonido del simulador."; break;
                }
            }

            if (isHovered) {
                drawRect(buttonX, buttonY, buttonWidth, buttonHeight, ColorRGB{46, 38, 26});
                drawRectOutline(buttonX, buttonY, buttonWidth, buttonHeight, Palette::DIVINE_GOLD);
                drawRectOutline(buttonX + 1, buttonY + 1, buttonWidth - 2, buttonHeight - 2, ColorRGB{210, 175, 75});
            } else {
                drawRect(buttonX, buttonY, buttonWidth, buttonHeight, ColorRGB{28, 28, 34});
                drawRectOutline(buttonX, buttonY, buttonWidth, buttonHeight, ColorRGB{70, 70, 76});
            }

            const int labelPositionX = buttonX + 14;
            const int labelPositionY = buttonY + (buttonHeight - 7) / 2;
            const ColorRGB labelColor = isHovered ? Palette::DIVINE_GOLD : Palette::RELIC_BONE;
            drawText(labelPositionX, labelPositionY, labelText + ":", labelColor, 1);

            if (sliderPercentValue >= 0) {
                constexpr int meterWidth = 90;
                constexpr int meterHeight = 10;
                const int meterPositionX = buttonX + buttonWidth - meterWidth - 14;
                const int meterPositionY = buttonY + (buttonHeight - meterHeight) / 2;
                drawSims2Meter(meterPositionX, meterPositionY, meterWidth, meterHeight,
                               static_cast<float>(sliderPercentValue) / 100.0f,
                               isHovered ? Palette::DIVINE_GOLD : ColorRGB{190, 160, 60});
                const int valuePositionX = meterPositionX - static_cast<int>(valueText.size() * 6) - 8;
                drawText(valuePositionX, labelPositionY, valueText, isHovered ? ColorRGB{255, 240, 150} : ColorRGB{210, 205, 190}, 1);
            } else {
                const int valuePositionX = buttonX + buttonWidth - static_cast<int>(valueText.size() * 6) - 16;
                const ColorRGB valueColor = isHovered ? ColorRGB{255, 235, 130} : ColorRGB{210, 205, 190};
                drawText(valuePositionX, labelPositionY, valueText, valueColor, 1);

                if (isHovered) {
                    drawRect(valuePositionX - 8, labelPositionY + 2, 4, 4, Palette::DIVINE_GOLD);
                }
            }
        }
    }

    // 3. Barra de Descripción / Tooltip Litúrgico
    const int descriptionBoxY = modalStartY + modalPixelHeight - 105;
    drawRect(modalStartX + 45, descriptionBoxY, modalPixelWidth - 90, 36, ColorRGB{14, 12, 18, 220});
    drawRectOutline(modalStartX + 45, descriptionBoxY, modalPixelWidth - 90, 36, ColorRGB{70, 65, 55});
    drawText(modalStartX + 55, descriptionBoxY + 14, "> " + hoveredTooltipText, ColorRGB{220, 215, 195}, 1);

    // 4. Botones de Acción Inferiores (1090: Restaurar, 1091: Guardar y Volver)
    for (const auto& actionButton : m_optionsActionButtons) {
        const bool isHovered = (actionButton.id == hoveredButtonId);

        const int buttonX = static_cast<int>(actionButton.x);
        const int buttonY = static_cast<int>(actionButton.y);
        const int buttonWidth = static_cast<int>(actionButton.width);
        const int buttonHeight = static_cast<int>(actionButton.height);

        if (actionButton.id == 1090) { // Restaurar
            if (isHovered) {
                drawRect(buttonX, buttonY, buttonWidth, buttonHeight, ColorRGB{45, 25, 25});
                drawRectOutline(buttonX, buttonY, buttonWidth, buttonHeight, Palette::BLOOD_RUBY);
                const int textX = buttonX + (buttonWidth - static_cast<int>(actionButton.label.size() * 6)) / 2;
                const int textY = buttonY + (buttonHeight - 7) / 2;
                drawText(textX, textY, actionButton.label, ColorRGB{255, 180, 180}, 1);
            } else {
                drawRect(buttonX, buttonY, buttonWidth, buttonHeight, ColorRGB{32, 28, 30});
                drawRectOutline(buttonX, buttonY, buttonWidth, buttonHeight, ColorRGB{75, 65, 65});
                const int textX = buttonX + (buttonWidth - static_cast<int>(actionButton.label.size() * 6)) / 2;
                const int textY = buttonY + (buttonHeight - 7) / 2;
                drawText(textX, textY, actionButton.label, ColorRGB{190, 160, 160}, 1);
            }
        } else if (actionButton.id == 1091 || actionButton.id == 12) { // Guardar y Volver
            if (isHovered) {
                drawRect(buttonX, buttonY, buttonWidth, buttonHeight, ColorRGB{52, 42, 24});
                drawRectOutline(buttonX, buttonY, buttonWidth, buttonHeight, Palette::DIVINE_GOLD);
                const int textX = buttonX + (buttonWidth - static_cast<int>(actionButton.label.size() * 6)) / 2;
                const int textY = buttonY + (buttonHeight - 7) / 2;
                drawText(textX, textY, actionButton.label, ColorRGB{255, 245, 215}, 1);
            } else {
                drawRect(buttonX, buttonY, buttonWidth, buttonHeight, ColorRGB{34, 30, 26});
                drawRectOutline(buttonX, buttonY, buttonWidth, buttonHeight, ColorRGB{120, 100, 60});
                const int textX = buttonX + (buttonWidth - static_cast<int>(actionButton.label.size() * 6)) / 2;
                const int textY = buttonY + (buttonHeight - 7) / 2;
                drawText(textX, textY, actionButton.label, Palette::DIVINE_GOLD, 1);
            }
        }
    }
}

void Renderer::applyVisualColorFilter(VisualColorFilter filter) {
    if (filter == VisualColorFilter::Neutral) return;

    for (uint32_t& pixelValue : m_framebuffer) {
        const uint32_t redComponent = pixelValue & 0xFF;
        const uint32_t greenComponent = (pixelValue >> 8) & 0xFF;
        const uint32_t blueComponent = (pixelValue >> 16) & 0xFF;
        const uint32_t alphaComponent = (pixelValue >> 24) & 0xFF;

        uint32_t processedRed = redComponent;
        uint32_t processedGreen = greenComponent;
        uint32_t processedBlue = blueComponent;

        switch (filter) {
            case VisualColorFilter::DivineGold: {
                processedRed = std::min(255u, (redComponent * 270u + 10u) / 256u);
                processedGreen = std::min(255u, (greenComponent * 260u + 5u) / 256u);
                processedBlue = (blueComponent * 215u) / 256u;
                break;
            }
            case VisualColorFilter::SepiaParchment: {
                processedRed = std::min(255u, (redComponent * 230u + greenComponent * 25u + 25u) / 256u);
                processedGreen = std::min(255u, (redComponent * 20u + greenComponent * 220u + 15u) / 256u);
                processedBlue = (blueComponent * 175u + 10u) / 256u;
                break;
            }
            case VisualColorFilter::BasaltCold: {
                processedRed = (redComponent * 210u) / 256u;
                processedGreen = (greenComponent * 230u) / 256u;
                processedBlue = std::min(255u, (blueComponent * 265u + 20u) / 256u);
                break;
            }
            default:
                break;
        }

        pixelValue = (alphaComponent << 24) | (processedBlue << 16) | (processedGreen << 8) | processedRed;
    }
}

// EN: Render the sacred "Learn to Play" tutorial codex modal with interactive category tabs and gameplay guides
// ES: Renderizar el modal sagrado del códice tutorial "Aprende a jugar" con pestañas interactivas y guías de juego
void Renderer::renderTutorialMenu(TutorialTab currentTab, int hoveredButtonId) {
    // EN: Translucent backdrop veil over the simulated living panorama
    // ES: Velo de fondo translúcido sobre el panorama vivo simulado
    drawRect(0, 0, m_width, m_height, ColorRGB{14, 12, 16, 185});

    // Marco exterior de la catedral
    drawRectOutline(12, 12, m_width - 24, m_height - 24, ColorRGB{77, 77, 77});
    drawRectOutline(16, 16, m_width - 32, m_height - 32, Palette::DIVINE_GOLD);

    // Modal / Altar sagrado central del códice
    constexpr float modalWidth = 980.0f;
    constexpr float modalHeight = 600.0f;
    const int modalStartX = static_cast<int>((static_cast<float>(m_width) - modalWidth) * 0.5f);
    const int modalStartY = static_cast<int>((static_cast<float>(m_height) - modalHeight) * 0.5f);
    const int modalPixelWidth = static_cast<int>(modalWidth);
    const int modalPixelHeight = static_cast<int>(modalHeight);

    // Fondo del relicario litúrgico con cristal ahumado y bordes en oro divino
    drawRect(modalStartX, modalStartY, modalPixelWidth, modalPixelHeight, ColorRGB{18, 16, 22, 235});
    drawRectOutline(modalStartX, modalStartY, modalPixelWidth, modalPixelHeight, ColorRGB{75, 75, 80});
    drawRectOutline(modalStartX + 3, modalStartY + 3, modalPixelWidth - 6, modalPixelHeight - 6, Palette::DIVINE_GOLD);

    // Gemas Blood Ruby en las cuatro esquinas
    constexpr int cornerGemDimension = 10;
    drawRect(modalStartX + 3, modalStartY + 3, cornerGemDimension, cornerGemDimension, Palette::BLOOD_RUBY);
    drawRect(modalStartX + modalPixelWidth - 3 - cornerGemDimension, modalStartY + 3, cornerGemDimension, cornerGemDimension, Palette::BLOOD_RUBY);
    drawRect(modalStartX + 3, modalStartY + modalPixelHeight - 3 - cornerGemDimension, cornerGemDimension, cornerGemDimension, Palette::BLOOD_RUBY);
    drawRect(modalStartX + modalPixelWidth - 3 - cornerGemDimension, modalStartY + modalPixelHeight - 3 - cornerGemDimension, cornerGemDimension, cornerGemDimension, Palette::BLOOD_RUBY);

    // Título superior sagrado con sombra proyectada
    const std::string titleText = "APRENDE A JUGAR - CODICE SAGRADO DE MARANGATU";
    const int titlePositionX = modalStartX + (modalPixelWidth - static_cast<int>(titleText.size() * 12)) / 2;
    drawText(titlePositionX + 1, modalStartY + 15, titleText, ColorRGB{10, 8, 6}, 2);
    drawText(titlePositionX, modalStartY + 14, titleText, Palette::DIVINE_GOLD, 2);

    // Subtítulo litúrgico
    const std::string subtitleText = "Crono-Guia Teologica para Dioses Principiantes y Guias de la Fe";
    const int subtitlePositionX = modalStartX + (modalPixelWidth - static_cast<int>(subtitleText.size() * 6)) / 2;
    drawText(subtitlePositionX, modalStartY + 32, subtitleText, ColorRGB{204, 181, 189}, 1);

    // 1. Renderizar Pestañas Superiores de Categorías
    for (const auto& tabButton : m_tutorialTabButtons) {
        bool isCurrentActiveTab = false;
        if (tabButton.id == 2001 && currentTab == TutorialTab::BasicsAndFaith) isCurrentActiveTab = true;
        else if (tabButton.id == 2002 && currentTab == TutorialTab::ControlsAndCamera) isCurrentActiveTab = true;
        else if (tabButton.id == 2003 && currentTab == TutorialTab::VillagersAndEcology) isCurrentActiveTab = true;
        else if (tabButton.id == 2004 && currentTab == TutorialTab::MiraclesAndEras) isCurrentActiveTab = true;

        const bool isHovered = (tabButton.id == hoveredButtonId);
        const int buttonPosX = static_cast<int>(tabButton.x);
        const int buttonPosY = static_cast<int>(tabButton.y);
        const int buttonPixelWidth = static_cast<int>(tabButton.width);
        const int buttonPixelHeight = static_cast<int>(tabButton.height);

        if (isCurrentActiveTab) {
            drawRect(buttonPosX, buttonPosY, buttonPixelWidth, buttonPixelHeight, ColorRGB{52, 42, 24});
            drawRectOutline(buttonPosX, buttonPosY, buttonPixelWidth, buttonPixelHeight, Palette::DIVINE_GOLD);
            drawLine(buttonPosX + 4, buttonPosY + buttonPixelHeight - 2, buttonPosX + buttonPixelWidth - 4, buttonPosY + buttonPixelHeight - 2, ColorRGB{255, 235, 120});
            const int textPositionX = buttonPosX + (buttonPixelWidth - static_cast<int>(tabButton.label.size() * 6)) / 2;
            const int textPositionY = buttonPosY + (buttonPixelHeight - 7) / 2;
            drawText(textPositionX, textPositionY, tabButton.label, ColorRGB{255, 245, 215}, 1);
        } else if (isHovered) {
            drawRect(buttonPosX, buttonPosY, buttonPixelWidth, buttonPixelHeight, ColorRGB{38, 34, 30});
            drawRectOutline(buttonPosX, buttonPosY, buttonPixelWidth, buttonPixelHeight, ColorRGB{160, 140, 90});
            const int textPositionX = buttonPosX + (buttonPixelWidth - static_cast<int>(tabButton.label.size() * 6)) / 2;
            const int textPositionY = buttonPosY + (buttonPixelHeight - 7) / 2;
            drawText(textPositionX, textPositionY, tabButton.label, Palette::DIVINE_GOLD, 1);
        } else {
            drawRect(buttonPosX, buttonPosY, buttonPixelWidth, buttonPixelHeight, ColorRGB{24, 24, 28});
            drawRectOutline(buttonPosX, buttonPosY, buttonPixelWidth, buttonPixelHeight, ColorRGB{65, 65, 70});
            const int textPositionX = buttonPosX + (buttonPixelWidth - static_cast<int>(tabButton.label.size() * 6)) / 2;
            const int textPositionY = buttonPosY + (buttonPixelHeight - 7) / 2;
            drawText(textPositionX, textPositionY, tabButton.label, Palette::RELIC_BONE, 1);
        }
    }

    // Línea divisoria ceremonial bajo las pestañas
    drawLine(modalStartX + 25, modalStartY + 82, modalStartX + modalPixelWidth - 25, modalStartY + 82, Palette::DIVINE_GOLD);
    drawRect(modalStartX + modalPixelWidth / 2 - 4, modalStartY + 79, 8, 8, Palette::BLOOD_RUBY);

    // 2. Paneles de Contenido (Tarjeta Izquierda y Tarjeta Derecha)
    constexpr int cardWidth = 455;
    constexpr int cardHeight = 444;
    const int leftCardX = modalStartX + 25;
    const int rightCardX = modalStartX + modalPixelWidth - 25 - cardWidth;
    const int cardStartY = modalStartY + 90;

    // Fondos de tarjetas con madera sacra satinada
    drawSims2Panel(leftCardX, cardStartY, cardWidth, cardHeight, ColorRGB{24, 18, 14}, false, ColorRGB{120, 85, 55});
    drawRectOutline(leftCardX, cardStartY, cardWidth, cardHeight, ColorRGB{70, 50, 35});
    drawRectOutline(leftCardX + 1, cardStartY + 1, cardWidth - 2, cardHeight - 2, ColorRGB{130, 95, 55});

    drawSims2Panel(rightCardX, cardStartY, cardWidth, cardHeight, ColorRGB{24, 18, 14}, false, ColorRGB{120, 85, 55});
    drawRectOutline(rightCardX, cardStartY, cardWidth, cardHeight, ColorRGB{70, 50, 35});
    drawRectOutline(rightCardX + 1, cardStartY + 1, cardWidth - 2, cardHeight - 2, ColorRGB{130, 95, 55});

    // Gemas diminutas en esquinas de tarjetas
    drawRect(leftCardX + 2, cardStartY + 2, 4, 4, Palette::BLOOD_RUBY);
    drawRect(leftCardX + cardWidth - 6, cardStartY + 2, 4, 4, Palette::BLOOD_RUBY);
    drawRect(rightCardX + 2, cardStartY + 2, 4, 4, Palette::BLOOD_RUBY);
    drawRect(rightCardX + cardWidth - 6, cardStartY + 2, 4, 4, Palette::BLOOD_RUBY);

    // Helpers locales para títulos y viñetas
    auto drawSectionHeader = [&](int posX, int posY, const std::string& headerText) {
        drawText(posX + 1, posY + 1, headerText, ColorRGB{12, 8, 6}, 1);
        drawText(posX, posY, headerText, Palette::DIVINE_GOLD, 1);
        drawLine(posX, posY + 11, posX + cardWidth - 28, posY + 11, ColorRGB{140, 100, 50});
    };

    auto drawKeybindingPill = [&](int posX, int posY, const std::string& keyLabel, const std::string& descriptionText) {
        constexpr int pillHeight = 16;
        const int pillWidth = static_cast<int>(keyLabel.size() * 6) + 10;
        drawRect(posX, posY, pillWidth, pillHeight, ColorRGB{38, 28, 20});
        drawRectOutline(posX, posY, pillWidth, pillHeight, Palette::DIVINE_GOLD);
        drawText(posX + 5, posY + 4, keyLabel, ColorRGB{255, 235, 140}, 1);
        drawText(posX + pillWidth + 8, posY + 4, descriptionText, Palette::RELIC_BONE, 1);
    };

    // 3. Renderizado de Contenido Específico según la Pestaña Activa
    if (currentTab == TutorialTab::BasicsAndFaith) {
        // --- TARJETA IZQUIERDA: EL ROL DIVINO Y OBJETIVO ---
        int textOffsetY = cardStartY + 14;
        drawSectionHeader(leftCardX + 14, textOffsetY, "EL ROL DE LA DEIDAD (TU MISION SAGRADA)");
        textOffsetY += 20;

        drawText(leftCardX + 14, textOffsetY, "Eres la deidad inmortal y guia supremo de tu pueblo.", Palette::RELIC_BONE, 1);
        textOffsetY += 14;
        drawText(leftCardX + 14, textOffsetY, "Tu influencia divina emana de la devocion de tus fieles.", ColorRGB{200, 190, 175}, 1);
        textOffsetY += 20;

        drawText(leftCardX + 14, textOffsetY, "OBJETIVO SUPREMO:", Palette::DIVINE_GOLD, 1);
        textOffsetY += 14;
        drawText(leftCardX + 14, textOffsetY, "- Guiar a tu pueblo desde la Prehistoria (Era 0)", Palette::RELIC_BONE, 1);
        textOffsetY += 14;
        drawText(leftCardX + 14, textOffsetY, "  hasta la Era Post-Biologica y de Consciencia Digital.", Palette::RELIC_BONE, 1);
        textOffsetY += 14;
        drawText(leftCardX + 14, textOffsetY, "- Expandir tu culto ante fes rivales y peligros naturales.", Palette::RELIC_BONE, 1);
        textOffsetY += 22;

        drawText(leftCardX + 14, textOffsetY, "CONDICION DE DERROTA:", Palette::BLOOD_RUBY, 1);
        textOffsetY += 14;
        drawText(leftCardX + 14, textOffsetY, "Si todos tus aldeanos mueren, tu fe desaparecera", ColorRGB{240, 120, 120}, 1);
        textOffsetY += 14;
        drawText(leftCardX + 14, textOffsetY, "porque no habra nadie vivo para recordarte ni orar.", ColorRGB{240, 120, 120}, 1);
        textOffsetY += 24;

        drawSectionHeader(leftCardX + 14, textOffsetY, "EL TEMPLO PRINCIPAL Y MONUMENTOS");
        textOffsetY += 20;
        drawText(leftCardX + 14, textOffsetY, "- Centro litúrgico sagrado donde convergen los fieles.", Palette::RELIC_BONE, 1);
        textOffsetY += 14;
        drawText(leftCardX + 14, textOffsetY, "- Emite auras doradas que consolidan la fe comunitaria.", Palette::RELIC_BONE, 1);
        textOffsetY += 14;
        drawText(leftCardX + 14, textOffsetY, "- Protege a los creyentes contra conversiones foraneas.", Palette::RELIC_BONE, 1);

        // --- TARJETA DERECHA: MECANICA DE LA FE Y REPLICACION ---
        textOffsetY = cardStartY + 14;
        drawSectionHeader(rightCardX + 14, textOffsetY, "ATRIBUTOS ESPIRITUALES DEL ALDEANO");
        textOffsetY += 20;

        drawText(rightCardX + 14, textOffsetY, "* Devocion: Grado de lealtad absoluta a tu credo sagrado.", Palette::RELIC_BONE, 1);
        textOffsetY += 14;
        drawText(rightCardX + 14, textOffsetY, "* Fanatismo: Intensidad belica y pasion al orar o combatir.", Palette::RELIC_BONE, 1);
        textOffsetY += 14;
        drawText(rightCardX + 14, textOffsetY, "* Moral Religiosa: Afinidades de Orden, Armonia con Gaia,", Palette::RELIC_BONE, 1);
        textOffsetY += 14;
        drawText(rightCardX + 14, textOffsetY, "  Sacrificio mistico o Fraternidad comunitaria.", ColorRGB{200, 190, 175}, 1);
        textOffsetY += 22;

        drawSectionHeader(rightCardX + 14, textOffsetY, "TRANSMISION Y EXPANSION DE LA FE");
        textOffsetY += 20;

        drawText(rightCardX + 14, textOffsetY, "1. Procreacion Familiar:", Palette::DIVINE_GOLD, 1);
        textOffsetY += 14;
        drawText(rightCardX + 14, textOffsetY, "   Los hijos nacen heredando la fe y ritos de sus padres.", Palette::RELIC_BONE, 1);
        textOffsetY += 18;

        drawText(rightCardX + 14, textOffsetY, "2. Predicacion y Shamanes:", Palette::DIVINE_GOLD, 1);
        textOffsetY += 14;
        drawText(rightCardX + 14, textOffsetY, "   Lideres espirituales instruyen y convierten a incredulos.", Palette::RELIC_BONE, 1);
        textOffsetY += 18;

        drawText(rightCardX + 14, textOffsetY, "3. Fenomeno del Sincretismo:", Palette::DIVINE_GOLD, 1);
        textOffsetY += 14;
        drawText(rightCardX + 14, textOffsetY, "   Pueblos vecinos en paz fusionan doctrinas armoniosas.", Palette::RELIC_BONE, 1);
        textOffsetY += 20;

        drawText(rightCardX + 14, textOffsetY, "CONSEJO DIVINO: Manten siempre alta la moral y salud.", ColorRGB{255, 235, 140}, 1);
    }
    else if (currentTab == TutorialTab::ControlsAndCamera) {
        // --- TARJETA IZQUIERDA: MOVIMIENTO Y RATON ---
        int textOffsetY = cardStartY + 14;
        drawSectionHeader(leftCardX + 14, textOffsetY, "NAVEGACION Y CONTROL DE CAMARA");
        textOffsetY += 20;

        drawKeybindingPill(leftCardX + 14, textOffsetY, "W A S D", "Desplazar la camara por el territorio.");
        textOffsetY += 24;
        drawKeybindingPill(leftCardX + 14, textOffsetY, "CLICK DER", "Arrastrar para mover la vista fluidamente.");
        textOffsetY += 24;
        drawKeybindingPill(leftCardX + 14, textOffsetY, "RUEDA / +-", "Acercar o alejar el zoom isometrico.");
        textOffsetY += 24;
        drawKeybindingPill(leftCardX + 14, textOffsetY, "TECLA R", "Reiniciar y centrar camara en el templo.");
        textOffsetY += 24;
        drawKeybindingPill(leftCardX + 14, textOffsetY, "ESPACIO", "Pausar o reanudar el transcurso del tiempo.");
        textOffsetY += 24;
        drawKeybindingPill(leftCardX + 14, textOffsetY, "CLICK IZQ", "Seleccionar aldeano, recurso, templo o boton.");
        textOffsetY += 24;
        drawKeybindingPill(leftCardX + 14, textOffsetY, "ESCAPE", "Pausa, opciones o volver al menu.");
        textOffsetY += 30;

        drawSectionHeader(leftCardX + 14, textOffsetY, "AUDIO Y SONIDO LITURGICO");
        textOffsetY += 20;
        drawKeybindingPill(leftCardX + 14, textOffsetY, "TECLA U", "Silenciar / Activar sonido (Mute toggle).");
        textOffsetY += 24;
        drawKeybindingPill(leftCardX + 14, textOffsetY, "[  /  ]", "Bajar / Subir volumen maestro del juego.");

        // --- TARJETA DERECHA: VISTAS Y MODALES ---
        textOffsetY = cardStartY + 14;
        drawSectionHeader(rightCardX + 14, textOffsetY, "MODOS DE VISTA DEL MAPA (TECLAS 1 A 4)");
        textOffsetY += 20;

        drawKeybindingPill(rightCardX + 14, textOffsetY, "TECLA 1", "Modo Fe: Territorios teñidos por credo religioso.");
        textOffsetY += 24;
        drawKeybindingPill(rightCardX + 14, textOffsetY, "TECLA 2", "Modo Biomas: Vegetacion, fertilidad y humedad.");
        textOffsetY += 24;
        drawKeybindingPill(rightCardX + 14, textOffsetY, "TECLA 3", "Modo Sincretismo: Contacto doctrinal y tension.");
        textOffsetY += 24;
        drawKeybindingPill(rightCardX + 14, textOffsetY, "TECLA 4", "Modo Eras: Nivel tecnologico de cada poblado.");
        textOffsetY += 30;

        drawSectionHeader(rightCardX + 14, textOffsetY, "LIBROS SAGRADOS Y VENTANAS MODALES");
        textOffsetY += 20;

        drawKeybindingPill(rightCardX + 14, textOffsetY, "C  /  E", "Cronica Sagrada: Registro de eventos mundiales.");
        textOffsetY += 24;
        drawKeybindingPill(rightCardX + 14, textOffsetY, "T  /  A", "Arbol de Fe: Ramas teologicas y dogmas.");
        textOffsetY += 24;
        drawKeybindingPill(rightCardX + 14, textOffsetY, "TECLA M", "Codigo Moral: Tabues y virtudes divinas.");
        textOffsetY += 24;
        drawKeybindingPill(rightCardX + 14, textOffsetY, "ENTER", "Consola de mandatos divinos (truco LUMINUS).");
        textOffsetY += 28;

        drawText(rightCardX + 14, textOffsetY, "TIP: Pulsa espacio durante crisis para pausar y pensar.", ColorRGB{255, 235, 140}, 1);
    }
    else if (currentTab == TutorialTab::VillagersAndEcology) {
        // --- TARJETA IZQUIERDA: NECESIDADES BASICAS ---
        int textOffsetY = cardStartY + 14;
        drawSectionHeader(leftCardX + 14, textOffsetY, "BIOLOGIA Y SUPERVIVENCIA DEL ALDEANO");
        textOffsetY += 20;

        drawText(leftCardX + 14, textOffsetY, "1. SALUD Y VITALIDAD (HP):", Palette::DIVINE_GOLD, 1);
        textOffsetY += 14;
        drawText(leftCardX + 14, textOffsetY, "   Disminuye por desnutricion, incendios, peleas o vejez.", Palette::RELIC_BONE, 1);
        textOffsetY += 18;

        drawText(leftCardX + 14, textOffsetY, "2. HAMBRE Y ALIMENTACION:", Palette::DIVINE_GOLD, 1);
        textOffsetY += 14;
        drawText(leftCardX + 14, textOffsetY, "   Aldeanos recolectan arbustos frutales y cosechas.", Palette::RELIC_BONE, 1);
        textOffsetY += 14;
        drawText(leftCardX + 14, textOffsetY, "   Si el hambre llega a 0, comenzaran a perder salud.", ColorRGB{240, 140, 140}, 1);
        textOffsetY += 18;

        drawText(leftCardX + 14, textOffsetY, "3. DESCANSO, ENERGIA Y CHOZAS:", Palette::DIVINE_GOLD, 1);
        textOffsetY += 14;
        drawText(leftCardX + 14, textOffsetY, "   - Dormir en el suelo restaura poca energia.", Palette::RELIC_BONE, 1);
        textOffsetY += 14;
        drawText(leftCardX + 14, textOffsetY, "   - Dormir bajo techo en una Choza recupera 100% de", Palette::RELIC_BONE, 1);
        textOffsetY += 14;
        drawText(leftCardX + 14, textOffsetY, "     energia, protege del frio y permite procrear.", ColorRGB{180, 240, 180}, 1);
        textOffsetY += 22;

        drawSectionHeader(leftCardX + 14, textOffsetY, "EDIFICIOS Y CRECIMIENTO AUTONOMO");
        textOffsetY += 20;
        drawText(leftCardX + 14, textOffsetY, "- Con madera y piedra, los aldeanos construyen chozas.", Palette::RELIC_BONE, 1);
        textOffsetY += 14;
        drawText(leftCardX + 14, textOffsetY, "- Las parejas con techo procrean de forma autonoma.", Palette::RELIC_BONE, 1);

        // --- TARJETA DERECHA: LOS 5 RECURSOS VITALES ---
        textOffsetY = cardStartY + 14;
        drawSectionHeader(rightCardX + 14, textOffsetY, "LOS 5 RECURSOS DE LA CIVILIZACION");
        textOffsetY += 20;

        drawKeybindingPill(rightCardX + 14, textOffsetY, "COMIDA", "Sustento basico de la vida y crecimiento tribal.");
        textOffsetY += 24;
        drawKeybindingPill(rightCardX + 14, textOffsetY, "MADERA", "Construccion de viviendas y combustible de fogatas.");
        textOffsetY += 24;
        drawKeybindingPill(rightCardX + 14, textOffsetY, "PIEDRA", "Monumentos sagrados, murallas y cimientos liticos.");
        textOffsetY += 24;
        drawKeybindingPill(rightCardX + 14, textOffsetY, "ORO   ", "Tributos divinos, reliquias y estatuas de culto.");
        textOffsetY += 24;
        drawKeybindingPill(rightCardX + 14, textOffsetY, "HIERRO", "Herramientas de labranza y armamento de defensa.");
        textOffsetY += 30;

        drawSectionHeader(rightCardX + 14, textOffsetY, "CLIMA Y CICLO CIRCADIANO");
        textOffsetY += 20;
        drawText(rightCardX + 14, textOffsetY, "- Durante la noche el frio arrecia y la vision mengua.", Palette::RELIC_BONE, 1);
        textOffsetY += 14;
        drawText(rightCardX + 14, textOffsetY, "- Las lluvias fertilizan el suelo y recargan los rios.", Palette::RELIC_BONE, 1);
        textOffsetY += 14;
        drawText(rightCardX + 14, textOffsetY, "- Las sequias extremas pueden desencadenar fuegos.", ColorRGB{255, 180, 120}, 1);
        textOffsetY += 20;
        drawText(rightCardX + 14, textOffsetY, "CONSEJO: Asegura suficientes fuentes de comida cerca.", ColorRGB{255, 235, 140}, 1);
    }
    else if (currentTab == TutorialTab::MiraclesAndEras) {
        // --- TARJETA IZQUIERDA: PODERES DIVINOS Y ESCALAS ---
        int textOffsetY = cardStartY + 14;
        drawSectionHeader(leftCardX + 14, textOffsetY, "MILAGROS Y PODERES (BARRA INFERIOR)");
        textOffsetY += 20;

        drawText(leftCardX + 14, textOffsetY, "1. LLUVIA SAGRADA:", Palette::DIVINE_GOLD, 1);
        textOffsetY += 14;
        drawText(leftCardX + 14, textOffsetY, "   Extingue incendios voraces, riega tierras aridas", Palette::RELIC_BONE, 1);
        textOffsetY += 14;
        drawText(leftCardX + 14, textOffsetY, "   y genera corrientes y remolinos que nutren biomas.", Palette::RELIC_BONE, 1);
        textOffsetY += 18;

        drawText(leftCardX + 14, textOffsetY, "2. BENDICION DIVINA:", Palette::DIVINE_GOLD, 1);
        textOffsetY += 14;
        drawText(leftCardX + 14, textOffsetY, "   Cura las heridas de tus aldeanos e inspira fe viva.", Palette::RELIC_BONE, 1);
        textOffsetY += 18;

        drawText(leftCardX + 14, textOffsetY, "3. FUEGO CELESTIAL:", Palette::DIVINE_GOLD, 1);
        textOffsetY += 14;
        drawText(leftCardX + 14, textOffsetY, "   Purifica o castiga enemigos (¡cuidado con el viento!).", ColorRGB{255, 150, 100}, 1);
        textOffsetY += 18;

        drawText(leftCardX + 14, textOffsetY, "4. IMPACTO DE METEORITO (ESCALA REAL):", Palette::DIVINE_GOLD, 1);
        textOffsetY += 14;
        drawText(leftCardX + 14, textOffsetY, "   Masa sideral con crater real de 20m y onda sismica.", Palette::RELIC_BONE, 1);
        textOffsetY += 18;

        drawText(leftCardX + 14, textOffsetY, "5. BOMBA ATOMICA (ONDA LITTLE BOY):", Palette::BLOOD_RUBY, 1);
        textOffsetY += 14;
        drawText(leftCardX + 14, textOffsetY, "   Onda expansiva que calcina y propaga vientos a 1.8km.", ColorRGB{255, 120, 120}, 1);

        // --- TARJETA DERECHA: EVOLUCION DE LAS 8 ERAS ---
        textOffsetY = cardStartY + 14;
        drawSectionHeader(rightCardX + 14, textOffsetY, "EVOLUCION DE LAS 8 ERAS HISTORICAS");
        textOffsetY += 20;

        drawText(rightCardX + 14, textOffsetY, "Era 0: Animismo Primordial (Espiritus de la tierra)", Palette::RELIC_BONE, 1);
        textOffsetY += 14;
        drawText(rightCardX + 14, textOffsetY, "Era 1: Totemismo y Ancestros (Tumulos y ofrendas)", Palette::RELIC_BONE, 1);
        textOffsetY += 14;
        drawText(rightCardX + 14, textOffsetY, "Era 2: Chamanismo Estructural (Ritos de fertilidad)", Palette::RELIC_BONE, 1);
        textOffsetY += 14;
        drawText(rightCardX + 14, textOffsetY, "Era 3: Politeismo Territorial (Panteones y templos)", Palette::RELIC_BONE, 1);
        textOffsetY += 14;
        drawText(rightCardX + 14, textOffsetY, "Era 4: Religiones Universales (Doctrinas escritas)", Palette::RELIC_BONE, 1);
        textOffsetY += 14;
        drawText(rightCardX + 14, textOffsetY, "Era 5: Racionalizacion (Ciencia, logica y secularismo)", Palette::RELIC_BONE, 1);
        textOffsetY += 14;
        drawText(rightCardX + 14, textOffsetY, "Era 6: Post-Secularismo (Nuevas espiritualidades)", Palette::RELIC_BONE, 1);
        textOffsetY += 14;
        drawText(rightCardX + 14, textOffsetY, "Era 7: Transhumanismo (Mentes sinteticas y deidad IA)", ColorRGB{140, 220, 255}, 1);
        textOffsetY += 22;

        drawSectionHeader(rightCardX + 14, textOffsetY, "SINCRETISMO TEOLOGICO");
        textOffsetY += 20;
        drawText(rightCardX + 14, textOffsetY, "Cuando pueblos de distintas fes conviven amistosamente,", Palette::RELIC_BONE, 1);
        textOffsetY += 14;
        drawText(rightCardX + 14, textOffsetY, "sus dogmas se combinan creando ritos unificados.", Palette::RELIC_BONE, 1);
    }

    // 4. Renderizar Botones Inferiores de Navegación y Salida
    for (const auto& actionButton : m_tutorialButtons) {
        const bool isHovered = (actionButton.id == hoveredButtonId);
        const int buttonPosX = static_cast<int>(actionButton.x);
        const int buttonPosY = static_cast<int>(actionButton.y);
        const int buttonPixelWidth = static_cast<int>(actionButton.width);
        const int buttonPixelHeight = static_cast<int>(actionButton.height);

        if (isHovered) {
            drawSims2Panel(buttonPosX, buttonPosY, buttonPixelWidth, buttonPixelHeight, ColorRGB{64, 44, 26}, false, Palette::DIVINE_GOLD);
            drawRectOutline(buttonPosX, buttonPosY, buttonPixelWidth, buttonPixelHeight, Palette::DIVINE_GOLD);
            drawRectOutline(buttonPosX + 1, buttonPosY + 1, buttonPixelWidth - 2, buttonPixelHeight - 2, ColorRGB{255, 235, 120});
            const int textPositionX = buttonPosX + (buttonPixelWidth - static_cast<int>(actionButton.label.size() * 6)) / 2;
            const int textPositionY = buttonPosY + (buttonPixelHeight - 7) / 2;
            drawText(textPositionX + 1, textPositionY + 1, actionButton.label, ColorRGB{16, 10, 6}, 1);
            drawText(textPositionX, textPositionY, actionButton.label, ColorRGB{255, 248, 225}, 1);
        } else {
            drawSims2Panel(buttonPosX, buttonPosY, buttonPixelWidth, buttonPixelHeight, ColorRGB{42, 26, 20}, false, ColorRGB{140, 102, 60});
            drawRectOutline(buttonPosX, buttonPosY, buttonPixelWidth, buttonPixelHeight, ColorRGB{85, 58, 38});
            drawRectOutline(buttonPosX + 1, buttonPosY + 1, buttonPixelWidth - 2, buttonPixelHeight - 2, ColorRGB{145, 108, 65});
            const int textPositionX = buttonPosX + (buttonPixelWidth - static_cast<int>(actionButton.label.size() * 6)) / 2;
            const int textPositionY = buttonPosY + (buttonPixelHeight - 7) / 2;
            drawText(textPositionX + 1, textPositionY + 1, actionButton.label, ColorRGB{16, 10, 8}, 1);
            drawText(textPositionX, textPositionY, actionButton.label, Palette::RELIC_BONE, 1);
        }
    }
}

void Renderer::renderWorldCreationMenu(const WorldCreationConfig& config, int hoveredButtonId, int activeInputField) {
    // EN: Translucent backdrop veil over the simulated world background
    // ES: Velo de fondo translúcido sobre el mundo simulado de fondo
    drawRect(0, 0, m_width, m_height, ColorRGB{14, 12, 16, 175});

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
    // EN: Expanded map size presets starting from 64x64 up to planetary (320x320)
    // ES: Opciones ampliadas de tamaño de mapa iniciando desde 64x64 hasta planetario (320x320)
    static constexpr int TOTAL_MAP_SIZES = 8;
    static const char* MAP_SIZES[TOTAL_MAP_SIZES] = {
        "Pequeno (64x64)",
        "Mediano (80x80)",
        "Grande (96x96)",
        "Enorme (128x128)",
        "Inmenso (160x160)",
        "Colosal (200x200)",
        "Titanico (256x256)",
        "Planetario (320x320)"
    };
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
            displayText = (config.mapSizeIndex >= 0 && config.mapSizeIndex < TOTAL_MAP_SIZES)
                        ? MAP_SIZES[config.mapSizeIndex] : "Enorme (128x128)";
            textColor = (config.mapSizeIndex >= 3) ? Palette::DIVINE_GOLD : Palette::RELIC_BONE;
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
    // EN: Translucent backdrop veil over the simulated world background
    // ES: Velo de fondo translúcido sobre el mundo simulado de fondo
    drawRect(0, 0, m_width, m_height, ColorRGB{14, 12, 16, 175});

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
    constexpr int btnH = 44;
    constexpr int btnW = 192;
    constexpr int gap = 14;

    int curBtnX = mx + 25;

    // EN: ID 904: OBSERVAR MUNDO (VER MAPA)
    // ES: ID 904: OBSERVAR MUNDO (VER MAPA)
    bool inspectHovered = (hoveredButtonId == 904);
    ColorRGB inspectBg = inspectHovered ? ColorRGB{40, 75, 60} : ColorRGB{20, 45, 38};
    drawRect(curBtnX, btnY, btnW, btnH, inspectBg);
    drawRectOutline(curBtnX, btnY, btnW, btnH, inspectHovered ? ColorRGB{120, 255, 200} : ColorRGB{50, 140, 110});
    std::string inspectText = "OBSERVAR MUNDO (V)";
    drawText(curBtnX + (btnW - static_cast<int>(inspectText.size() * 6)) / 2, btnY + 17, inspectText, ColorRGB{180, 255, 220}, 1);
    curBtnX += btnW + gap;

    // EN: ID 901: REINTENTAR GÉNESIS
    // ES: ID 901: REINTENTAR GÉNESIS
    bool retryHovered = (hoveredButtonId == 901);
    ColorRGB retryBg = retryHovered ? ColorRGB{150, 35, 45} : Palette::BLOOD_RUBY;
    drawRect(curBtnX, btnY, btnW, btnH, retryBg);
    drawRectOutline(curBtnX, btnY, btnW, btnH, retryHovered ? Palette::DIVINE_GOLD : ColorRGB{180, 140, 60});
    std::string retryText = "REINTENTAR GENESIS";
    drawText(curBtnX + (btnW - static_cast<int>(retryText.size() * 6)) / 2, btnY + 17, retryText, ColorRGB{255, 255, 240}, 1);
    curBtnX += btnW + gap;

    // EN: ID 902: CARGAR PARTIDA
    // ES: ID 902: CARGAR PARTIDA
    bool loadHovered = (hoveredButtonId == 902);
    ColorRGB loadBg = loadHovered ? ColorRGB{52, 42, 30} : ColorRGB{34, 30, 32};
    drawRect(curBtnX, btnY, btnW, btnH, loadBg);
    drawRectOutline(curBtnX, btnY, btnW, btnH, loadHovered ? Palette::DIVINE_GOLD : ColorRGB{80, 75, 70});
    std::string loadText = "CARGAR PARTIDA";
    drawText(curBtnX + (btnW - static_cast<int>(loadText.size() * 6)) / 2, btnY + 17, loadText, Palette::DIVINE_GOLD, 1);
    curBtnX += btnW + gap;

    // EN: ID 903: MENÚ PRINCIPAL
    // ES: ID 903: MENÚ PRINCIPAL
    bool menuHovered = (hoveredButtonId == 903);
    ColorRGB menuBg = menuHovered ? ColorRGB{52, 42, 30} : ColorRGB{34, 30, 32};
    drawRect(curBtnX, btnY, btnW, btnH, menuBg);
    drawRectOutline(curBtnX, btnY, btnW, btnH, menuHovered ? Palette::DIVINE_GOLD : ColorRGB{80, 75, 70});
    std::string menuText = "MENU PRINCIPAL";
    drawText(curBtnX + (btnW - static_cast<int>(menuText.size() * 6)) / 2, btnY + 17, menuText, Palette::RELIC_BONE, 1);
}

namespace {

// EN: Convert map size index into readable UI descriptor
// ES: Convertir índice de tamaño de mapa a descriptor legible de interfaz
const char* getMapEditorSizeName(int index) {
    switch (index) {
        case 0: return "64x64 (Pequeno)";
        case 1: return "80x80 (Mediano)";
        case 2: return "96x96 (Grande)";
        case 3: return "128x128 (Enorme)";
        case 4: return "160x160 (Inmenso)";
        case 5: return "200x200 (Colosal)";
        case 6: return "256x256 (Titanico)";
        case 7: return "320x320 (Planetario)";
        default: return "128x128 (Enorme)";
    }
}

// EN: Convert climate enum to descriptive string
// ES: Convertir enumeración de clima a cadena descriptiva
const char* getMapEditorClimateName(WorldClimate climate) {
    switch (climate) {
        case WorldClimate::Temperate:  return "Templado";
        case WorldClimate::Glaciation: return "Glaciacion";
        case WorldClimate::Desert:     return "Desertico";
        case WorldClimate::Tropical:   return "Tropical";
        default:                       return "Templado";
    }
}

// EN: Convert resource abundance enum to descriptive string
// ES: Convertir abundancia de recursos a cadena descriptiva
const char* getMapEditorAbundanceName(ResourceAbundance abundance) {
    switch (abundance) {
        case ResourceAbundance::Scarce:   return "Escaso";
        case ResourceAbundance::Balanced: return "Equilibrado";
        case ResourceAbundance::Abundant: return "Abundante";
        default:                          return "Equilibrado";
    }
}

// EN: Convert active brush to readable label
// ES: Convertir pincel activo a etiqueta legible
const char* getMapEditorBrushName(MapEditorBrush brush) {
    switch (brush) {
        case MapEditorBrush::Inspect:         return "Inspeccionar Celda";
        case MapEditorBrush::Ocean:           return "Oceano Profundo";
        case MapEditorBrush::ShallowWater:    return "Agua Somera";
        case MapEditorBrush::FertileValley:   return "Valle Fertil";
        case MapEditorBrush::DenseForest:     return "Bosque Denso";
        case MapEditorBrush::HighlandHills:   return "Colinas Altas";
        case MapEditorBrush::RuggedMountains: return "Montanas";
        case MapEditorBrush::AridDesert:      return "Desierto Arido";
        case MapEditorBrush::FrozenTundra:    return "Tundra Helada";
        case MapEditorBrush::VolcanicCaldera: return "Caldera Volcanica";
        case MapEditorBrush::Wood:            return "Yacimiento: Madera";
        case MapEditorBrush::Stone:           return "Yacimiento: Piedra";
        case MapEditorBrush::Gold:            return "Yacimiento: Oro";
        case MapEditorBrush::Iron:            return "Yacimiento: Hierro";
        case MapEditorBrush::Copper:          return "Yacimiento: Cobre";
        case MapEditorBrush::Diamond:         return "Yacimiento: Diamante";
        case MapEditorBrush::ClearResource:   return "Limpiar Recurso";
        case MapEditorBrush::RaiseElevation:  return "Elevar Terreno (+)";
        case MapEditorBrush::LowerElevation:  return "Bajar Terreno (-)";
        case MapEditorBrush::FlattenElevation: return "Aplanar Terreno (=)";
        case MapEditorBrush::TileGrass:          return "Mosaico: Pasto (0)";
        case MapEditorBrush::TileRock:           return "Mosaico: Roca (1)";
        case MapEditorBrush::TileTropicalPlains: return "Mosaico: Pradera (2)";
        case MapEditorBrush::TileFarmland:       return "Mosaico: Tierra (3)";
        case MapEditorBrush::TileDeepOceanAlt:   return "Mosaico: Oceano Azul (4)";
        case MapEditorBrush::TileRichForest:     return "Mosaico: Suelo Bosque (5)";
        case MapEditorBrush::TileHighlands:      return "Mosaico: Colina (6)";
        case MapEditorBrush::TileSwamp:          return "Mosaico: Pantano (7)";
        case MapEditorBrush::TileSand:           return "Mosaico: Arena (8)";
        case MapEditorBrush::TileSnow:           return "Mosaico: Nieve (9)";
        case MapEditorBrush::TileShallowWater:   return "Mosaico: Somera (10)";
        case MapEditorBrush::TileMidOcean:       return "Mosaico: Mar (11)";
        case MapEditorBrush::TileCaveWater:      return "Mosaico: Cueva (12)";
        case MapEditorBrush::TileAbyssal:        return "Mosaico: Abisal (13)";
        case MapEditorBrush::TileOpenOcean:      return "Mosaico: Mar Abierto (14)";
        case MapEditorBrush::TileLava:           return "Mosaico: Lava (15)";
        case MapEditorBrush::TileAsh:            return "Ceniza / Calcinado";
        case MapEditorBrush::TileCrater:         return "Crater de Impacto";
        default:                              return "Herramienta Activa";
    }
}

// EN: Convert biome type to readable label
// ES: Convertir tipo de bioma a etiqueta legible
const char* getMapEditorBiomeName(BiomeType biome) {
    switch (biome) {
        case BiomeType::DeepOcean:       return "Oceano Profundo";
        case BiomeType::ShallowWater:    return "Agua Somera";
        case BiomeType::CoastalPlains:   return "Llanuras Costeras";
        case BiomeType::FertileValley:   return "Valle Fertil";
        case BiomeType::DenseForest:     return "Bosque Denso";
        case BiomeType::HighlandHills:   return "Colinas Altas";
        case BiomeType::AridDesert:      return "Desierto Arido";
        case BiomeType::RuggedMountains: return "Montana Escarpada";
        case BiomeType::FrozenTundra:    return "Tundra Helada";
        case BiomeType::VolcanicCaldera: return "Caldera Volcanica";
        default:                         return "Tierra Fertil";
    }
}

} // anonymous namespace

// =============================================================================
// EN: Render the Dedicated Map Editor / Sacred Cartography Workshop
// ES: Renderizar el Taller Litúrgico de Creación de Mapas / Cartografía Sagrada
// =============================================================================
void Renderer::renderMapEditor(const MapEditorConfig& config, const WorldMap& editorMap,
                               int hoveredButtonId, int activeInputField,
                               const Point2i& hoveredCell) {
    // EN: Translucent background veil over cosmic background
    // ES: Velo de fondo translúcido sobre el fondo cósmico
    drawRect(0, 0, m_width, m_height, ColorRGB{14, 12, 16});

    // Marco exterior de catedral
    drawRectOutline(10, 10, m_width - 20, m_height - 20, ColorRGB{70, 65, 65});
    drawRectOutline(14, 14, m_width - 28, m_height - 28, Palette::DIVINE_GOLD);

    // Esquinas ornamentales
    constexpr int cornerGem = 12;
    drawRect(14, 14, cornerGem, cornerGem, Palette::DIVINE_GOLD);
    drawRect(m_width - 14 - cornerGem, 14, cornerGem, cornerGem, Palette::DIVINE_GOLD);
    drawRect(14, m_height - 14 - cornerGem, cornerGem, cornerGem, Palette::DIVINE_GOLD);
    drawRect(m_width - 14 - cornerGem, m_height - 14 - cornerGem, cornerGem, cornerGem, Palette::DIVINE_GOLD);

    // Cabecera Principal
    const std::string titleText = "TALLER LITURGICO DE CREACION DE MAPAS";
    const int titleX = (m_width - static_cast<int>(titleText.size() * 12)) / 2;
    drawText(titleX, 16, titleText, Palette::DIVINE_GOLD, 2);

    const std::string subtitleText = "Disena continentes sagrados, distribuye biomas primordiales y bendice yacimientos minerales";
    const int subX = (m_width - static_cast<int>(subtitleText.size() * 6)) / 2;
    drawText(subX, 36, subtitleText, Palette::RELIC_BONE, 1);

    drawLine(30, 48, m_width - 30, 48, Palette::DIVINE_GOLD);
    drawRect(m_width / 2 - 4, 45, 8, 8, Palette::BLOOD_RUBY);

    // =========================================================================
    // EN: Left Control Panel (Procedural generation parameters and tool palette)
    // ES: Panel Izquierdo de Control (Parámetros generativos y paleta de herramientas)
    // =========================================================================
    constexpr int leftPanelX = 24;
    constexpr int leftPanelY = 56;
    constexpr int leftPanelW = 440;
    constexpr int leftPanelH = 640;

    drawSims2Panel(leftPanelX, leftPanelY, leftPanelW, leftPanelH, ColorRGB{24, 18, 16}, false, Palette::DIVINE_GOLD);
    drawRectOutline(leftPanelX, leftPanelY, leftPanelW, leftPanelH, ColorRGB{75, 55, 40});
    drawRectOutline(leftPanelX + 2, leftPanelY + 2, leftPanelW - 4, leftPanelH - 4, Palette::DIVINE_GOLD);

    // Sección I: Parámetros Generativos
    drawText(38, 70, "I. PARAMETROS GENERATIVOS", Palette::DIVINE_GOLD, 1);
    drawLine(38, 82, 438, 82, ColorRGB{120, 95, 60});

    // Fila 1: Tamaño del Mapa
    drawText(38, 94, "Tamano:", Palette::RELIC_BONE, 1);
    std::string mapSizeStr = getMapEditorSizeName(config.mapSizeIndex);
    drawRect(144, 88, 260, 24, ColorRGB{36, 26, 22});
    drawRectOutline(144, 88, 260, 24, ColorRGB{90, 70, 50});
    const int sizeTxtX = 144 + (260 - static_cast<int>(mapSizeStr.size() * 6)) / 2;
    drawText(sizeTxtX, 95, mapSizeStr, Palette::DIVINE_GOLD, 1);

    // Fila 2: Clima Primordial
    drawText(38, 122, "Clima:", Palette::RELIC_BONE, 1);
    std::string climateStr = getMapEditorClimateName(config.worldClimate);
    drawRect(144, 116, 260, 24, ColorRGB{36, 26, 22});
    drawRectOutline(144, 116, 260, 24, ColorRGB{90, 70, 50});
    const int climTxtX = 144 + (260 - static_cast<int>(climateStr.size() * 6)) / 2;
    drawText(climTxtX, 123, climateStr, Palette::DIVINE_GOLD, 1);

    // Fila 3: Recursos
    drawText(38, 150, "Recursos:", Palette::RELIC_BONE, 1);
    std::string abundStr = getMapEditorAbundanceName(config.resourceAbundance);
    drawRect(144, 144, 260, 24, ColorRGB{36, 26, 22});
    drawRectOutline(144, 144, 260, 24, ColorRGB{90, 70, 50});
    const int abundTxtX = 144 + (260 - static_cast<int>(abundStr.size() * 6)) / 2;
    drawText(abundTxtX, 151, abundStr, Palette::DIVINE_GOLD, 1);

    // Fila 4: Semilla
    drawText(38, 178, "Semilla:", Palette::RELIC_BONE, 1);
    std::string seedStr = std::to_string(config.mapSeed);
    if (activeInputField == 807) seedStr += "_";
    drawRect(110, 172, 216, 24, (activeInputField == 807) ? ColorRGB{52, 38, 24} : ColorRGB{36, 26, 22});
    drawRectOutline(110, 172, 216, 24, (activeInputField == 807) ? Palette::DIVINE_GOLD : ColorRGB{90, 70, 50});
    drawText(118, 179, seedStr, (activeInputField == 807) ? ColorRGB{255, 245, 180} : Palette::DIVINE_GOLD, 1);

    // Sección II: Radio del Pincel
    drawText(38, 244, "II. RADIO DEL PINCEL", Palette::DIVINE_GOLD, 1);
    drawLine(38, 256, 438, 256, ColorRGB{120, 95, 60});

    // Sección III: Biomas y Superficie
    drawText(38, 298, "III. BIOMAS Y SUPERFICIE", Palette::DIVINE_GOLD, 1);
    drawLine(38, 310, 438, 310, ColorRGB{120, 95, 60});

    // Sección IV: Yacimientos Minerales
    drawText(38, 420, "IV. YACIMIENTOS MINERALES", Palette::DIVINE_GOLD, 1);
    drawLine(38, 432, 438, 432, ColorRGB{120, 95, 60});

    // Sección V: Topografía y Elevación
    drawText(38, 516, "V. TOPOGRAFIA Y ELEVACION", Palette::DIVINE_GOLD, 1);
    drawLine(38, 528, 438, 528, ColorRGB{120, 95, 60});

    // =========================================================================
    // EN: Right Cartographic Preview Panel & Interactive Painting Surface
    // ES: Panel Derecho de Vista Previa Cartográfica y Superficie Interactiva
    // =========================================================================
    constexpr int rightPanelX = 476;
    constexpr int rightPanelY = 56;
    constexpr int rightPanelW = 780;
    constexpr int rightPanelH = 640;

    drawSims2Panel(rightPanelX, rightPanelY, rightPanelW, rightPanelH, ColorRGB{20, 16, 18}, false, Palette::DIVINE_GOLD);
    drawRectOutline(rightPanelX, rightPanelY, rightPanelW, rightPanelH, ColorRGB{75, 55, 40});
    drawRectOutline(rightPanelX + 2, rightPanelY + 2, rightPanelW - 4, rightPanelH - 4, Palette::DIVINE_GOLD);

    // Cabecera del Panel Derecho
    drawText(494, 70, "CARTOGRAFIA SAGRADA EN TIEMPO REAL", Palette::DIVINE_GOLD, 1);

    // Indicador de herramienta activa
    std::string toolInfoStr = "Herramienta: " + std::string(getMapEditorBrushName(config.activeBrush)) +
                              " | Radio: " + std::to_string(config.brushRadius == 1 ? 1 : (config.brushRadius == 2 ? 3 : 5)) + "x" +
                              std::to_string(config.brushRadius == 1 ? 1 : (config.brushRadius == 2 ? 3 : 5));
    const int toolTxtX = rightPanelX + rightPanelW - 20 - static_cast<int>(toolInfoStr.size() * 6);
    drawText(toolTxtX, 70, toolInfoStr, ColorRGB{160, 220, 255}, 1);

    // Marco del lienzo cartográfico (Sunken interior)
    constexpr int canvasX = 494;
    constexpr int canvasY = 88;
    constexpr int canvasW = 744;
    constexpr int canvasH = 474;

    drawRect(canvasX - 2, canvasY - 2, canvasW + 4, canvasH + 4, ColorRGB{10, 8, 12});
    drawRectOutline(canvasX - 2, canvasY - 2, canvasW + 4, canvasH + 4, ColorRGB{55, 45, 40});
    drawRectOutline(canvasX - 1, canvasY - 1, canvasW + 2, canvasH + 2, Palette::DIVINE_GOLD);

    // Renderizar la cuadrícula del mundo editor
    const int mapWidth = editorMap.getWidth();
    const int mapHeight = editorMap.getHeight();

    int waterCount = 0;
    int forestCount = 0;
    int mountainCount = 0;
    int plainsCount = 0;
    int resourceDepositCount = 0;

    if (mapWidth > 0 && mapHeight > 0) {
        const float scale = std::min(static_cast<float>(canvasW) / static_cast<float>(mapWidth),
                                     static_cast<float>(canvasH) / static_cast<float>(mapHeight));
        const int previewW = static_cast<int>(static_cast<float>(mapWidth) * scale);
        const int previewH = static_cast<int>(static_cast<float>(mapHeight) * scale);
        const int previewStartX = canvasX + (canvasW - previewW) / 2;
        const int previewStartY = canvasY + (canvasH - previewH) / 2;

        // Fondo oscuro del mapa
        drawRect(previewStartX, previewStartY, previewW, previewH, ColorRGB{16, 24, 40});

        for (int cy = 0; cy < mapHeight; ++cy) {
            for (int cx = 0; cx < mapWidth; ++cx) {
                const Cell& cell = editorMap.getCell(cx, cy);

                // Estadísticas
                if (cell.isNaturalWater() || cell.isWater()) waterCount++;
                else if (cell.biome == BiomeType::DenseForest) forestCount++;
                else if (cell.biome == BiomeType::RuggedMountains) mountainCount++;
                else if (cell.biome == BiomeType::FertileValley || cell.biome == BiomeType::CoastalPlains) plainsCount++;

                if (cell.naturalResource != ResourceType::None) resourceDepositCount++;

                const int px = previewStartX + static_cast<int>(static_cast<float>(cx) * scale);
                const int py = previewStartY + static_cast<int>(static_cast<float>(cy) * scale);
                const int pw = std::max(1, static_cast<int>(std::ceil(scale)));
                const int ph = std::max(1, static_cast<int>(std::ceil(scale)));

                ColorRGB cellCol = cell.getBiomeBaseColor();

                // Sombra de elevación sutil para dar volumen topográfico
                if (!cell.isNaturalWater()) {
                    const int elevShade = (cell.elevation - 1) * 7;
                    cellCol.r = static_cast<uint8_t>(std::clamp(static_cast<int>(cellCol.r) + elevShade, 0, 255));
                    cellCol.g = static_cast<uint8_t>(std::clamp(static_cast<int>(cellCol.g) + elevShade, 0, 255));
                    cellCol.b = static_cast<uint8_t>(std::clamp(static_cast<int>(cellCol.b) + elevShade, 0, 255));
                }

                drawRect(px, py, pw, ph, cellCol);

                // Destacar recursos con un punto brillante si el pixel es de tamaño visible
                if (cell.naturalResource != ResourceType::None && scale >= 2.0f) {
                    ColorRGB resCol = Palette::DIVINE_GOLD;
                    switch (cell.naturalResource) {
                        case ResourceType::Wood:    resCol = ColorRGB{80, 240, 80}; break;
                        case ResourceType::Stone:   resCol = ColorRGB{210, 215, 225}; break;
                        case ResourceType::Gold:    resCol = Palette::DIVINE_GOLD; break;
                        case ResourceType::Iron:    resCol = ColorRGB{170, 210, 245}; break;
                        case ResourceType::Copper:  resCol = ColorRGB{235, 140, 70}; break;
                        case ResourceType::Diamond: resCol = ColorRGB{220, 255, 255}; break;
                        default: break;
                    }
                    const int dotX = px + pw / 2;
                    const int dotY = py + ph / 2;
                    drawRect(dotX, dotY, (scale >= 4.0f ? 2 : 1), (scale >= 4.0f ? 2 : 1), resCol);
                }
            }
        }

        // Si el cursor está sobre una celda, dibujar el área de influencia del pincel
        if (hoveredCell.x >= 0 && hoveredCell.y >= 0 && editorMap.isValid(hoveredCell.x, hoveredCell.y)) {
            const int offset = (config.brushRadius == 1) ? 0 : (config.brushRadius == 2 ? 1 : 2);
            const int minGx = std::max(0, hoveredCell.x - offset);
            const int maxGx = std::min(mapWidth - 1, hoveredCell.x + offset);
            const int minGy = std::max(0, hoveredCell.y - offset);
            const int maxGy = std::min(mapHeight - 1, hoveredCell.y + offset);

            const int bx = previewStartX + static_cast<int>(static_cast<float>(minGx) * scale);
            const int by = previewStartY + static_cast<int>(static_cast<float>(minGy) * scale);
            const int bw = static_cast<int>(static_cast<float>(maxGx - minGx + 1) * scale);
            const int bh = static_cast<int>(static_cast<float>(maxGy - minGy + 1) * scale);

            drawRectOutline(bx, by, bw, bh, ColorRGB{255, 255, 120});
            drawRectOutline(bx - 1, by - 1, bw + 2, bh + 2, ColorRGB{20, 16, 12});
        }
    }

    // Información de la celda bajo el cursor (Y = 568)
    std::string hoverInfoStr;
    if (hoveredCell.x >= 0 && hoveredCell.y >= 0 && editorMap.isValid(hoveredCell.x, hoveredCell.y)) {
        const Cell& cell = editorMap.getCell(hoveredCell.x, hoveredCell.y);
        hoverInfoStr = "Celda: (" + std::to_string(hoveredCell.x) + ", " + std::to_string(hoveredCell.y) +
                       ") | Bioma: " + std::string(getMapEditorBiomeName(cell.biome)) +
                       " | Elevacion: " + std::to_string(static_cast<int>(std::round(cell.elevation))) +
                       " | Recurso: " + std::string(getResourceTypeName(cell.naturalResource));
        if (cell.naturalResource != ResourceType::None) {
            hoverInfoStr += " (" + std::to_string(cell.resourceAmount) + ")";
        }
    } else {
        hoverInfoStr = "Haz clic o arrastra sobre el mapa para esculpir terreno y yacimientos";
    }
    drawText(494, 568, hoverInfoStr, ColorRGB{255, 245, 200}, 1);

    // Barra de Métricas y Estadísticas (Y = 588)
    const int totalCells = std::max(1, mapWidth * mapHeight);
    const int waterPct = (waterCount * 100) / totalCells;
    const int forestPct = (forestCount * 100) / totalCells;
    const int mountainPct = (mountainCount * 100) / totalCells;
    const int plainsPct = (plainsCount * 100) / totalCells;

    std::string statsStr = "Metricas: Agua: " + std::to_string(waterPct) +
                           "% | Valles/Llanos: " + std::to_string(plainsPct) +
                           "% | Bosques: " + std::to_string(forestPct) +
                           "% | Montanas: " + std::to_string(mountainPct) +
                           "% | Yacimientos: " + std::to_string(resourceDepositCount);
    drawText(494, 588, statsStr, Palette::RELIC_BONE, 1);

    // Mensaje de estado de la operación (Y = 608)
    drawText(494, 608, config.statusMessage, ColorRGB{255, 220, 100}, 1);

    // =========================================================================
    // EN: Render interactive UI buttons (tools, parameters, and action controls)
    // ES: Renderizar botones interactivos de UI (herramientas, parámetros y controles de acción)
    // =========================================================================
    for (const auto& editorButton : m_mapEditorButtons) {
        // EN: Skip custom-rendered text input fields
        // ES: Omitir campos de entrada de texto renderizados a medida
        if (editorButton.id == 807) {
            continue;
        }

        const bool isHovered = (editorButton.id == hoveredButtonId);
        const int buttonX = static_cast<int>(editorButton.x);
        const int buttonY = static_cast<int>(editorButton.y);
        const int buttonWidth = static_cast<int>(editorButton.width);
        const int buttonHeight = static_cast<int>(editorButton.height);

        // EN: Determine if this button represents the currently active brush or size mode
        // ES: Determinar si este botón representa el modo de tamaño o pincel actualmente activo
        bool isButtonActive = false;
        if (editorButton.id == 821) isButtonActive = (config.brushRadius == 1);
        else if (editorButton.id == 822) isButtonActive = (config.brushRadius == 2);
        else if (editorButton.id == 823) isButtonActive = (config.brushRadius == 3);
        else if (editorButton.id == 831) isButtonActive = (config.activeBrush == MapEditorBrush::Ocean);
        else if (editorButton.id == 832) isButtonActive = (config.activeBrush == MapEditorBrush::ShallowWater);
        else if (editorButton.id == 833) isButtonActive = (config.activeBrush == MapEditorBrush::FertileValley);
        else if (editorButton.id == 834) isButtonActive = (config.activeBrush == MapEditorBrush::DenseForest);
        else if (editorButton.id == 835) isButtonActive = (config.activeBrush == MapEditorBrush::HighlandHills);
        else if (editorButton.id == 836) isButtonActive = (config.activeBrush == MapEditorBrush::RuggedMountains);
        else if (editorButton.id == 837) isButtonActive = (config.activeBrush == MapEditorBrush::AridDesert);
        else if (editorButton.id == 838) isButtonActive = (config.activeBrush == MapEditorBrush::FrozenTundra);
        else if (editorButton.id == 839) isButtonActive = (config.activeBrush == MapEditorBrush::VolcanicCaldera);
        else if (editorButton.id == 841) isButtonActive = (config.activeBrush == MapEditorBrush::Wood);
        else if (editorButton.id == 842) isButtonActive = (config.activeBrush == MapEditorBrush::Stone);
        else if (editorButton.id == 843) isButtonActive = (config.activeBrush == MapEditorBrush::Gold);
        else if (editorButton.id == 844) isButtonActive = (config.activeBrush == MapEditorBrush::Iron);
        else if (editorButton.id == 845) isButtonActive = (config.activeBrush == MapEditorBrush::Copper);
        else if (editorButton.id == 846) isButtonActive = (config.activeBrush == MapEditorBrush::Diamond);
        else if (editorButton.id == 847) isButtonActive = (config.activeBrush == MapEditorBrush::ClearResource);
        else if (editorButton.id == 851) isButtonActive = (config.activeBrush == MapEditorBrush::RaiseElevation);
        else if (editorButton.id == 852) isButtonActive = (config.activeBrush == MapEditorBrush::LowerElevation);
        else if (editorButton.id == 855) isButtonActive = (config.activeBrush == MapEditorBrush::Inspect);

        // EN: Handle special styling for bottom action buttons (890: Play, 891: Save, 892: Exit)
        // ES: Manejar estilo especial para los botones de acción inferiores (890: Jugar, 891: Guardar, 892: Salir)
        if (editorButton.id == 890) {
            // EN: Play Map Button: Resplendent emerald & gold liturgical presence
            // ES: Botón Jugar Mapa: Resplandeciente presencia litúrgica esmeralda y dorada
            const ColorRGB fillColor = isHovered ? ColorRGB{38, 86, 44} : ColorRGB{24, 52, 28};
            const ColorRGB borderColor = isHovered ? ColorRGB{255, 230, 120} : ColorRGB{90, 190, 95};
            drawSims2Panel(buttonX, buttonY, buttonWidth, buttonHeight, fillColor, false, borderColor);
            drawRectOutline(buttonX, buttonY, buttonWidth, buttonHeight, borderColor);
            drawRectOutline(buttonX + 1, buttonY + 1, buttonWidth - 2, buttonHeight - 2, Palette::DIVINE_GOLD);

            const int textPosX = buttonX + (buttonWidth - static_cast<int>(editorButton.label.size() * 6)) / 2;
            const int textPosY = buttonY + (buttonHeight - 7) / 2;
            drawText(textPosX + 1, textPosY + 1, editorButton.label, ColorRGB{10, 20, 12}, 1);
            drawText(textPosX, textPosY, editorButton.label, isHovered ? ColorRGB{255, 255, 220} : ColorRGB{220, 255, 220}, 1);
            continue;
        }

        if (editorButton.id == 891) {
            // EN: Save Map Button: Sapphire & bronze reliquary theme
            // ES: Botón Guardar Mapa: Tema de relicario zafiro y bronce
            const ColorRGB fillColor = isHovered ? ColorRGB{42, 62, 92} : ColorRGB{26, 38, 58};
            const ColorRGB borderColor = isHovered ? Palette::DIVINE_GOLD : ColorRGB{110, 150, 210};
            drawSims2Panel(buttonX, buttonY, buttonWidth, buttonHeight, fillColor, false, borderColor);
            drawRectOutline(buttonX, buttonY, buttonWidth, buttonHeight, borderColor);
            drawRectOutline(buttonX + 1, buttonY + 1, buttonWidth - 2, buttonHeight - 2, ColorRGB{80, 120, 180});

            const int textPosX = buttonX + (buttonWidth - static_cast<int>(editorButton.label.size() * 6)) / 2;
            const int textPosY = buttonY + (buttonHeight - 7) / 2;
            drawText(textPosX + 1, textPosY + 1, editorButton.label, ColorRGB{10, 14, 24}, 1);
            drawText(textPosX, textPosY, editorButton.label, isHovered ? ColorRGB{255, 255, 240} : ColorRGB{210, 235, 255}, 1);
            continue;
        }

        if (editorButton.id == 892) {
            // EN: Exit to Menu Button: Blood ruby & sacred mahogany theme
            // ES: Botón Volver al Menú: Tema rubí ceremonial y caoba sagrada
            const ColorRGB fillColor = isHovered ? ColorRGB{76, 32, 32} : ColorRGB{48, 22, 22};
            const ColorRGB borderColor = isHovered ? ColorRGB{255, 120, 120} : Palette::BLOOD_RUBY;
            drawSims2Panel(buttonX, buttonY, buttonWidth, buttonHeight, fillColor, false, borderColor);
            drawRectOutline(buttonX, buttonY, buttonWidth, buttonHeight, borderColor);
            drawRectOutline(buttonX + 1, buttonY + 1, buttonWidth - 2, buttonHeight - 2, ColorRGB{140, 40, 40});

            const int textPosX = buttonX + (buttonWidth - static_cast<int>(editorButton.label.size() * 6)) / 2;
            const int textPosY = buttonY + (buttonHeight - 7) / 2;
            drawText(textPosX + 1, textPosY + 1, editorButton.label, ColorRGB{24, 8, 8}, 1);
            drawText(textPosX, textPosY, editorButton.label, isHovered ? ColorRGB{255, 230, 230} : Palette::RELIC_BONE, 1);
            continue;
        }

        // EN: Standard toolbar buttons (parameters, brushes, elevation modifiers)
        // ES: Botones estándar de la barra de herramientas (parámetros, pinceles, modificadores de elevación)
        ColorRGB baseFillColor{34, 24, 20};
        ColorRGB borderRimColor{90, 70, 50};
        ColorRGB labelColor = Palette::RELIC_BONE;

        if (isButtonActive) {
            // EN: Active selected brush / mode highlighting
            // ES: Resaltado de modo / pincel seleccionado activo
            baseFillColor = ColorRGB{64, 46, 26};
            borderRimColor = Palette::DIVINE_GOLD;
            labelColor = ColorRGB{255, 245, 180};
        } else if (isHovered) {
            // EN: Hovered state illumination
            // ES: Iluminación de estado hovered
            baseFillColor = ColorRGB{48, 34, 24};
            borderRimColor = Palette::DIVINE_GOLD;
            labelColor = ColorRGB{255, 235, 150};
        }

        drawSims2Panel(buttonX, buttonY, buttonWidth, buttonHeight, baseFillColor, false, borderRimColor);
        drawRectOutline(buttonX, buttonY, buttonWidth, buttonHeight, borderRimColor);

        if (isButtonActive) {
            drawRectOutline(buttonX + 1, buttonY + 1, buttonWidth - 2, buttonHeight - 2, ColorRGB{255, 220, 90});
            // Esquinas doradas indicando selección activa
            constexpr int cornerMark = 3;
            drawRect(buttonX + 1, buttonY + 1, cornerMark, cornerMark, Palette::DIVINE_GOLD);
            drawRect(buttonX + buttonWidth - 1 - cornerMark, buttonY + 1, cornerMark, cornerMark, Palette::DIVINE_GOLD);
            drawRect(buttonX + 1, buttonY + buttonHeight - 1 - cornerMark, cornerMark, cornerMark, Palette::DIVINE_GOLD);
            drawRect(buttonX + buttonWidth - 1 - cornerMark, buttonY + buttonHeight - 1 - cornerMark, cornerMark, cornerMark, Palette::DIVINE_GOLD);
        }

        const int textPosX = buttonX + (buttonWidth - static_cast<int>(editorButton.label.size() * 6)) / 2;
        const int textPosY = buttonY + (buttonHeight - 7) / 2;
        drawText(textPosX + 1, textPosY + 1, editorButton.label, ColorRGB{12, 8, 6}, 1);
        drawText(textPosX, textPosY, editorButton.label, labelColor, 1);
    }
}

// =============================================================================
// EN: Render the Interactive 2.5D Isometric World Map Editor with free camera
// ES: Renderizar el Editor de Mapas Isométrico 2.5D Interactivo con cámara libre
// =============================================================================
void Renderer::renderMapEditorIsometric(const MapEditorConfig& config, const WorldMap& editorMap,
                                        const IsometricCamera& camera, int hoveredButtonId,
                                        const Point2i& hoveredCell, const ReligionRegistry& registry,
                                        const ReplicationEngine& replication, const GestorHumanos& humanos) {
    // EN: Cosmic backdrop behind the isometric world
    // ES: Telón de fondo cósmico detrás del mundo isométrico
    drawRect(0, 0, m_width, m_height, ColorRGB{12, 10, 16});

    // EN: Render the actual living isometric 2.5D world
    // ES: Renderizar el mundo 2.5D isométrico vivo
    renderWorld(editorMap, registry, replication, humanos, camera, hoveredCell, RenderMapMode::TerrainBiomes);

    // EN: Draw glowing isometric diamond cursor over hovered grid cell taking elevation into account
    // ES: Dibujar cursor de diamante isométrico brillante sobre la celda apuntada considerando la elevación
    if (editorMap.isValid(hoveredCell.x, hoveredCell.y)) {
        const float currentZoom = camera.getZoom();
        const float halfTileW = BASE_TILE_WIDTH * 0.5f * currentZoom;
        const float halfTileH = BASE_TILE_HEIGHT * 0.5f * currentZoom;
        const int radiusOffset = (config.brushRadius == 1) ? 0 :
                                 ((config.brushRadius == 2) ? 1 : 2);

        for (int deltaY = -radiusOffset; deltaY <= radiusOffset; ++deltaY) {
            for (int deltaX = -radiusOffset; deltaX <= radiusOffset; ++deltaX) {
                const int cellGridX = hoveredCell.x + deltaX;
                const int cellGridY = hoveredCell.y + deltaY;
                if (!editorMap.isValid(cellGridX, cellGridY)) continue;

                const Cell& cellRef = editorMap.getCell(cellGridX, cellGridY);
                const Vec2 centerPoint = camera.gridTileCenterScreen(cellGridX, cellGridY, cellRef.elevation);
                const bool isCenterCell = (deltaX == 0 && deltaY == 0);

                if (isCenterCell) {
                    drawDiamondOutline(centerPoint.x, centerPoint.y, halfTileW + 1, halfTileH + 1, Palette::DIVINE_GOLD);
                    drawDiamondOutline(centerPoint.x, centerPoint.y, halfTileW, halfTileH, ColorRGB{255, 255, 255});
                } else {
                    drawDiamondOutline(centerPoint.x, centerPoint.y, halfTileW, halfTileH, ColorRGB{255, 220, 100});
                }
            }
        }
    }

    // =========================================================================
    // EN: Top Navigation and Info Dock
    // ES: Muelle Superior de Navegación e Información
    // =========================================================================
    constexpr int topPanelX = 10;
    constexpr int topPanelY = 10;
    const int topPanelW = m_width - 20;
    constexpr int topPanelH = 36;

    drawSims2Panel(topPanelX, topPanelY, topPanelW, topPanelH, ColorRGB{20, 14, 12}, false, Palette::DIVINE_GOLD);
    drawRectOutline(topPanelX, topPanelY, topPanelW, topPanelH, ColorRGB{75, 55, 40});
    drawRectOutline(topPanelX + 2, topPanelY + 2, topPanelW - 4, topPanelH - 4, Palette::DIVINE_GOLD);

    // EN: Header title and active tool name
    // ES: Título de cabecera y nombre de herramienta activa
    drawText(22, 23, "TALLER LITURGICO: EDITOR ISOMETRICO 2.5D", Palette::DIVINE_GOLD, 1);

    std::string toolInfo = "Herramienta: " + std::string(getMapEditorBrushName(config.activeBrush)) +
                           " (" + std::to_string(config.brushRadius == 1 ? 1 : (config.brushRadius == 2 ? 3 : 5)) + "x" +
                           std::to_string(config.brushRadius == 1 ? 1 : (config.brushRadius == 2 ? 3 : 5)) + ")";
    drawText(320, 23, toolInfo, ColorRGB{160, 220, 255}, 1);

    // =========================================================================
    // EN: Bottom Altar Toolbar Dock (Tabs, Brushes, and Real-Time Inspection HUD)
    // ES: Muelle de Barra de Herramientas Inferior (Pestañas, Pinceles e Inspección)
    // =========================================================================
    constexpr int bottomPanelX = 10;
    constexpr int bottomPanelY = 590;
    const int bottomPanelW = m_width - 20;
    constexpr int bottomPanelH = 120;

    drawSims2Panel(bottomPanelX, bottomPanelY, bottomPanelW, bottomPanelH, ColorRGB{18, 14, 16}, false, Palette::DIVINE_GOLD);
    drawRectOutline(bottomPanelX, bottomPanelY, bottomPanelW, bottomPanelH, ColorRGB{75, 55, 40});
    drawRectOutline(bottomPanelX + 2, bottomPanelY + 2, bottomPanelW - 4, bottomPanelH - 4, Palette::DIVINE_GOLD);

    // EN: Hover Cell & Navigation Info (Y = 596)
    // ES: Información de Celda e Indicaciones de Navegación (Y = 596)
    std::string cellInfo;
    if (editorMap.isValid(hoveredCell.x, hoveredCell.y)) {
        const Cell& hoveredCellRef = editorMap.getCell(hoveredCell.x, hoveredCell.y);
        cellInfo = "Celda: (" + std::to_string(hoveredCell.x) + ", " + std::to_string(hoveredCell.y) +
                   ") | Bioma: " + std::string(getMapEditorBiomeName(hoveredCellRef.biome));
        if (hoveredCellRef.customTileIndex >= 0) {
            cellInfo += " [Mosaico " + std::to_string(hoveredCellRef.customTileIndex) + "]";
        }
        cellInfo += " | Elevacion: " + std::to_string(static_cast<int>(std::round(hoveredCellRef.elevation))) +
                    " | Recurso: " + std::string(getResourceTypeName(hoveredCellRef.naturalResource));
        if (hoveredCellRef.naturalResource != ResourceType::None) {
            cellInfo += " (" + std::to_string(hoveredCellRef.resourceAmount) + ")";
        }
    } else {
        cellInfo = "Arrastra Clic Izq: Pintar | Clic Der: Mover | Rueda: Zoom | WASD: Navegar | R: Centrar | TAB: Vista 2D";
    }
    drawText(24, 596, cellInfo, ColorRGB{255, 240, 180}, 1);

    // EN: Render isometric UI buttons (Tabs, Brushes, Actions)
    // ES: Renderizar botones de UI isométrica (Pestañas, Pinceles, Acciones)
    for (const auto& editorButton : m_mapEditorIsoButtons) {
        // EN: Filter buttons according to current active tab
        // ES: Filtrar botones según la pestaña activa actual
        if (editorButton.id >= 900 && editorButton.id <= 915 && config.paletteTab != 0) continue;
        if (((editorButton.id >= 841 && editorButton.id <= 847) || editorButton.id == 839 || editorButton.id == 920 || editorButton.id == 921) && config.paletteTab != 1) continue;
        if (((editorButton.id >= 851 && editorButton.id <= 855) || (editorButton.id >= 831 && editorButton.id <= 838)) && config.paletteTab != 2) continue;

        const bool isHovered = (editorButton.id == hoveredButtonId);
        const int buttonX = static_cast<int>(editorButton.x);
        const int buttonY = static_cast<int>(editorButton.y);
        const int buttonWidth = static_cast<int>(editorButton.width);
        const int buttonHeight = static_cast<int>(editorButton.height);

        // EN: Determine active selection state
        // ES: Determinar estado de selección activo
        bool isButtonActive = false;
        if (editorButton.id == 871) isButtonActive = (config.paletteTab == 0);
        else if (editorButton.id == 872) isButtonActive = (config.paletteTab == 1);
        else if (editorButton.id == 873) isButtonActive = (config.paletteTab == 2);
        else if (editorButton.id == 821) isButtonActive = (config.brushRadius == 1);
        else if (editorButton.id == 822) isButtonActive = (config.brushRadius == 2);
        else if (editorButton.id == 823) isButtonActive = (config.brushRadius == 3);
        else if (editorButton.id == 831) isButtonActive = (config.activeBrush == MapEditorBrush::Ocean);
        else if (editorButton.id == 832) isButtonActive = (config.activeBrush == MapEditorBrush::ShallowWater);
        else if (editorButton.id == 833) isButtonActive = (config.activeBrush == MapEditorBrush::FertileValley);
        else if (editorButton.id == 834) isButtonActive = (config.activeBrush == MapEditorBrush::DenseForest);
        else if (editorButton.id == 835) isButtonActive = (config.activeBrush == MapEditorBrush::HighlandHills);
        else if (editorButton.id == 836) isButtonActive = (config.activeBrush == MapEditorBrush::RuggedMountains);
        else if (editorButton.id == 837) isButtonActive = (config.activeBrush == MapEditorBrush::AridDesert);
        else if (editorButton.id == 838) isButtonActive = (config.activeBrush == MapEditorBrush::FrozenTundra);
        else if (editorButton.id == 839) isButtonActive = (config.activeBrush == MapEditorBrush::VolcanicCaldera);
        else if (editorButton.id == 841) isButtonActive = (config.activeBrush == MapEditorBrush::Wood);
        else if (editorButton.id == 842) isButtonActive = (config.activeBrush == MapEditorBrush::Stone);
        else if (editorButton.id == 843) isButtonActive = (config.activeBrush == MapEditorBrush::Gold);
        else if (editorButton.id == 844) isButtonActive = (config.activeBrush == MapEditorBrush::Iron);
        else if (editorButton.id == 845) isButtonActive = (config.activeBrush == MapEditorBrush::Copper);
        else if (editorButton.id == 846) isButtonActive = (config.activeBrush == MapEditorBrush::Diamond);
        else if (editorButton.id == 847) isButtonActive = (config.activeBrush == MapEditorBrush::ClearResource);
        else if (editorButton.id == 851) isButtonActive = (config.activeBrush == MapEditorBrush::RaiseElevation);
        else if (editorButton.id == 852) isButtonActive = (config.activeBrush == MapEditorBrush::LowerElevation);
        else if (editorButton.id == 853) isButtonActive = (config.activeBrush == MapEditorBrush::FlattenElevation);
        else if (editorButton.id == 855) isButtonActive = (config.activeBrush == MapEditorBrush::Inspect);
        else if (editorButton.id == 900) isButtonActive = (config.activeBrush == MapEditorBrush::TileGrass);
        else if (editorButton.id == 901) isButtonActive = (config.activeBrush == MapEditorBrush::TileRock);
        else if (editorButton.id == 902) isButtonActive = (config.activeBrush == MapEditorBrush::TileTropicalPlains);
        else if (editorButton.id == 903) isButtonActive = (config.activeBrush == MapEditorBrush::TileFarmland);
        else if (editorButton.id == 904) isButtonActive = (config.activeBrush == MapEditorBrush::TileDeepOceanAlt);
        else if (editorButton.id == 905) isButtonActive = (config.activeBrush == MapEditorBrush::TileRichForest);
        else if (editorButton.id == 906) isButtonActive = (config.activeBrush == MapEditorBrush::TileHighlands);
        else if (editorButton.id == 907) isButtonActive = (config.activeBrush == MapEditorBrush::TileSwamp);
        else if (editorButton.id == 908) isButtonActive = (config.activeBrush == MapEditorBrush::TileSand);
        else if (editorButton.id == 909) isButtonActive = (config.activeBrush == MapEditorBrush::TileSnow);
        else if (editorButton.id == 910) isButtonActive = (config.activeBrush == MapEditorBrush::TileShallowWater);
        else if (editorButton.id == 911) isButtonActive = (config.activeBrush == MapEditorBrush::TileMidOcean);
        else if (editorButton.id == 912) isButtonActive = (config.activeBrush == MapEditorBrush::TileCaveWater);
        else if (editorButton.id == 913) isButtonActive = (config.activeBrush == MapEditorBrush::TileAbyssal);
        else if (editorButton.id == 914) isButtonActive = (config.activeBrush == MapEditorBrush::TileOpenOcean);
        else if (editorButton.id == 915) isButtonActive = (config.activeBrush == MapEditorBrush::TileLava);
        else if (editorButton.id == 920) isButtonActive = (config.activeBrush == MapEditorBrush::TileCrater);
        else if (editorButton.id == 921) isButtonActive = (config.activeBrush == MapEditorBrush::TileAsh);

        // EN: Handle special action buttons
        // ES: Manejar botones de acción especiales
        if (editorButton.id == 890) { // JUGAR MAPA
            const ColorRGB fillColor = isHovered ? ColorRGB{38, 86, 44} : ColorRGB{24, 52, 28};
            const ColorRGB borderColor = isHovered ? ColorRGB{255, 230, 120} : ColorRGB{90, 190, 95};
            drawSims2Panel(buttonX, buttonY, buttonWidth, buttonHeight, fillColor, false, borderColor);
            drawRectOutline(buttonX, buttonY, buttonWidth, buttonHeight, borderColor);
            const int textPosX = buttonX + (buttonWidth - static_cast<int>(editorButton.label.size() * 6)) / 2;
            const int textPosY = buttonY + (buttonHeight - 7) / 2;
            drawText(textPosX, textPosY, editorButton.label, ColorRGB{220, 255, 220}, 1);
            continue;
        }

        if (editorButton.id == 891) { // GUARDAR
            const ColorRGB fillColor = isHovered ? ColorRGB{42, 62, 92} : ColorRGB{26, 38, 58};
            const ColorRGB borderColor = isHovered ? Palette::DIVINE_GOLD : ColorRGB{110, 150, 210};
            drawSims2Panel(buttonX, buttonY, buttonWidth, buttonHeight, fillColor, false, borderColor);
            drawRectOutline(buttonX, buttonY, buttonWidth, buttonHeight, borderColor);
            const int textPosX = buttonX + (buttonWidth - static_cast<int>(editorButton.label.size() * 6)) / 2;
            const int textPosY = buttonY + (buttonHeight - 7) / 2;
            drawText(textPosX, textPosY, editorButton.label, ColorRGB{220, 240, 255}, 1);
            continue;
        }

        if (editorButton.id == 892) { // SALIR
            const ColorRGB fillColor = isHovered ? ColorRGB{76, 32, 32} : ColorRGB{48, 22, 22};
            const ColorRGB borderColor = isHovered ? ColorRGB{255, 120, 120} : Palette::BLOOD_RUBY;
            drawSims2Panel(buttonX, buttonY, buttonWidth, buttonHeight, fillColor, false, borderColor);
            drawRectOutline(buttonX, buttonY, buttonWidth, buttonHeight, borderColor);
            const int textPosX = buttonX + (buttonWidth - static_cast<int>(editorButton.label.size() * 6)) / 2;
            const int textPosY = buttonY + (buttonHeight - 7) / 2;
            drawText(textPosX, textPosY, editorButton.label, Palette::RELIC_BONE, 1);
            continue;
        }

        if (editorButton.id == 880) { // VISTA 2D (TAB)
            const ColorRGB fillColor = isHovered ? ColorRGB{60, 48, 28} : ColorRGB{36, 26, 20};
            const ColorRGB borderColor = isHovered ? ColorRGB{255, 235, 120} : Palette::DIVINE_GOLD;
            drawSims2Panel(buttonX, buttonY, buttonWidth, buttonHeight, fillColor, false, borderColor);
            drawRectOutline(buttonX, buttonY, buttonWidth, buttonHeight, borderColor);
            const int textPosX = buttonX + (buttonWidth - static_cast<int>(editorButton.label.size() * 6)) / 2;
            const int textPosY = buttonY + (buttonHeight - 7) / 2;
            drawText(textPosX, textPosY, editorButton.label, isHovered ? ColorRGB{255, 255, 200} : Palette::DIVINE_GOLD, 1);
            continue;
        }

        // EN: Standard toolbar & palette buttons
        // ES: Botones estándar de barra de herramientas y paleta
        ColorRGB fillColor = ColorRGB{34, 24, 20};
        ColorRGB rimColor = ColorRGB{90, 70, 50};
        ColorRGB textColor = Palette::RELIC_BONE;

        if (isButtonActive) {
            fillColor = ColorRGB{64, 46, 26};
            rimColor = Palette::DIVINE_GOLD;
            textColor = ColorRGB{255, 245, 180};
        } else if (isHovered) {
            fillColor = ColorRGB{48, 34, 24};
            rimColor = Palette::DIVINE_GOLD;
            textColor = ColorRGB{255, 235, 150};
        }

        drawSims2Panel(buttonX, buttonY, buttonWidth, buttonHeight, fillColor, false, rimColor);
        drawRectOutline(buttonX, buttonY, buttonWidth, buttonHeight, rimColor);

        if (isButtonActive) {
            drawRectOutline(buttonX + 1, buttonY + 1, buttonWidth - 2, buttonHeight - 2, ColorRGB{255, 220, 90});
            constexpr int cornerMark = 3;
            drawRect(buttonX + 1, buttonY + 1, cornerMark, cornerMark, Palette::DIVINE_GOLD);
            drawRect(buttonX + buttonWidth - 1 - cornerMark, buttonY + 1, cornerMark, cornerMark, Palette::DIVINE_GOLD);
            drawRect(buttonX + 1, buttonY + buttonHeight - 1 - cornerMark, cornerMark, cornerMark, Palette::DIVINE_GOLD);
            drawRect(buttonX + buttonWidth - 1 - cornerMark, buttonY + buttonHeight - 1 - cornerMark, cornerMark, cornerMark, Palette::DIVINE_GOLD);
        }

        // EN: Draw colored swatch preview for Atlas terrain sprites (900-915)
        // ES: Dibujar muestra de color previa para mosaicos del atlas (900-915)
        if (editorButton.id >= 900 && editorButton.id <= 915) {
            const int swatchTileIdx = editorButton.id - 900;
            ColorRGB swatchCol = Palette::GRASS_PASTURE;
            switch (swatchTileIdx) {
                case 0:  swatchCol = Palette::GRASS_PASTURE; break;
                case 1:  swatchCol = ColorRGB{140, 135, 125}; break;
                case 2:  swatchCol = ColorRGB{125, 168, 65}; break;
                case 3:  swatchCol = ColorRGB{116, 74, 52}; break;
                case 4:  swatchCol = ColorRGB{24, 48, 89}; break;
                case 5:  swatchCol = ColorRGB{46, 110, 40}; break;
                case 6:  swatchCol = ColorRGB{155, 140, 85}; break;
                case 7:  swatchCol = ColorRGB{75, 80, 50}; break;
                case 8:  swatchCol = ColorRGB{215, 185, 115}; break;
                case 9:  swatchCol = ColorRGB{230, 240, 245}; break;
                case 10: swatchCol = ColorRGB{42, 135, 170}; break;
                case 11: swatchCol = ColorRGB{30, 80, 140}; break;
                case 12: swatchCol = ColorRGB{20, 50, 95}; break;
                case 13: swatchCol = ColorRGB{12, 28, 65}; break;
                case 14: swatchCol = ColorRGB{22, 60, 115}; break;
                case 15: swatchCol = ColorRGB{220, 80, 20}; break;
                default: break;
            }
            drawRect(buttonX + 4, buttonY + 5, 12, buttonHeight - 10, swatchCol);
            drawRectOutline(buttonX + 4, buttonY + 5, 12, buttonHeight - 10, ColorRGB{20, 16, 12});
        }

        const int textPosX = buttonX + (buttonWidth - static_cast<int>(editorButton.label.size() * 6)) / 2;
        const int textPosY = buttonY + (buttonHeight - 7) / 2;
        drawText(textPosX + 1, textPosY + 1, editorButton.label, ColorRGB{12, 8, 6}, 1);
        drawText(textPosX, textPosY, editorButton.label, textColor, 1);
    }
}

void Renderer::renderPostDefeatSpectatorHUD(const WorldCreationConfig& config, int extinctionYear,
                                           const Point2i& hoveredGridCell, const WorldMap& map,
                                           const IsometricCamera& camera, bool revealAllFog,
                                           int hoveredButtonId) {
    // EN: 1. Draw glowing isometric diamond reticle on hovered tile considering elevation
    // ES: 1. Dibujar retícula de diamante isométrico brillante sobre la celda apuntada considerando elevación
    if (map.isValid(hoveredGridCell.x, hoveredGridCell.y)) {
        const float currentZoom = camera.getZoom();
        const float halfTileW = BASE_TILE_WIDTH * 0.5f * currentZoom;
        const float halfTileH = BASE_TILE_HEIGHT * 0.5f * currentZoom;
        const Cell& cellRef = map.getCell(hoveredGridCell.x, hoveredGridCell.y);
        const Vec2 centerPoint = camera.gridTileCenterScreen(hoveredGridCell.x, hoveredGridCell.y, cellRef.elevation);

        drawDiamondOutline(centerPoint.x, centerPoint.y, halfTileW + 1, halfTileH + 1, Palette::DIVINE_GOLD);
        drawDiamondOutline(centerPoint.x, centerPoint.y, halfTileW, halfTileH, ColorRGB{255, 255, 255});
    }

    // EN: 2. Top Liturgical Spectator Banner
    // ES: 2. Franja Superior Litúrgica de Modo Espectador
    constexpr int topH = 46;
    drawRect(0, 0, m_width, topH, ColorRGB{16, 12, 14});
    drawRect(0, topH, m_width, 2, Palette::DIVINE_GOLD);

    // EN: Left Deity & Collapse Telemetry
    // ES: Telemetría de Deidad y Colapso a la izquierda
    drawText(16, 10, "[MODO ESPECTADOR: MAPA TRAS LA EXTINCION]", Palette::DIVINE_GOLD, 1);
    std::string relInfo = "Deidad: " + config.godName + " | Culto: " + config.religionName +
                          " | Ano del Ocaso: " + Timeline::formatYear(extinctionYear);
    drawText(16, 26, relInfo, Palette::RELIC_BONE, 1);

    // EN: Center Hovered Cell Inspection Info
    // ES: Información de inspección de celda apuntada en el centro
    if (map.isValid(hoveredGridCell.x, hoveredGridCell.y)) {
        const Cell& cell = map.getCell(hoveredGridCell.x, hoveredGridCell.y);
        std::string cellInfo = "Celda: (" + std::to_string(hoveredGridCell.x) + ", " +
                               std::to_string(hoveredGridCell.y) + ") | Bioma: " +
                               std::string(getMapEditorBiomeName(cell.biome));
        if (cell.customTileIndex >= 0) {
            cellInfo += " [Mosaico " + std::to_string(cell.customTileIndex) + "]";
        }
        cellInfo += " | Elev: " + std::to_string(static_cast<int>(std::round(cell.elevation)));
        if (cell.hasImpactCrater) {
            cellInfo += " [CRATER DE IMPACTO]";
        } else if (cell.hasTreesUprooted) {
            cellInfo += " [ARBOLES ARRANCADOS]";
        } else if (cell.isBurnt) {
            cellInfo += " [TIERRA CALCINADA]";
        } else if (cell.naturalResource != ResourceType::None) {
            cellInfo += " | Rec: " + std::string(getResourceTypeName(cell.naturalResource));
        }
        const int cellW = static_cast<int>(cellInfo.size()) * 6;
        const int centerPosX = (m_width - cellW) / 2;
        if (centerPosX > 380) {
            drawText(centerPosX, 18, cellInfo, ColorRGB{240, 220, 160}, 1);
        }
    }

    // EN: Right Action Buttons (MENU, REINTENTAR, NIEBLA, VER EPITAFIO)
    // ES: Botones de Acción a la derecha (MENU, REINTENTAR, NIEBLA, VER EPITAFIO)
    constexpr int btnH = 30;
    constexpr int btnY = 8;
    int curRightX = m_width - 12;

    // ID 903: MENU
    constexpr int wMenu = 85;
    curRightX -= wMenu;
    bool menuHov = (hoveredButtonId == 903);
    drawRect(curRightX, btnY, wMenu, btnH, menuHov ? ColorRGB{60, 45, 35} : ColorRGB{35, 28, 30});
    drawRectOutline(curRightX, btnY, wMenu, btnH, menuHov ? Palette::DIVINE_GOLD : ColorRGB{80, 70, 60});
    drawText(curRightX + (wMenu - 24) / 2, btnY + 10, "MENU", Palette::RELIC_BONE, 1);

    // ID 901: REINTENTAR
    constexpr int wRetry = 120;
    curRightX -= (wRetry + 8);
    bool retryHov = (hoveredButtonId == 901);
    drawRect(curRightX, btnY, wRetry, btnH, retryHov ? ColorRGB{140, 30, 40} : Palette::BLOOD_RUBY);
    drawRectOutline(curRightX, btnY, wRetry, btnH, retryHov ? Palette::DIVINE_GOLD : ColorRGB{160, 120, 50});
    drawText(curRightX + (wRetry - 60) / 2, btnY + 10, "REINTENTAR", ColorRGB{255, 255, 240}, 1);

    // ID 906: NIEBLA
    constexpr int wFog = 155;
    curRightX -= (wFog + 8);
    bool fogHov = (hoveredButtonId == 906);
    drawRect(curRightX, btnY, wFog, btnH, fogHov ? ColorRGB{45, 55, 65} : ColorRGB{25, 32, 40});
    drawRectOutline(curRightX, btnY, wFog, btnH, fogHov ? Palette::DIVINE_GOLD : ColorRGB{70, 80, 90});
    std::string fogText = revealAllFog ? "NIEBLA: LIBRE (F)" : "NIEBLA: VISIBLE (F)";
    drawText(curRightX + (wFog - static_cast<int>(fogText.size() * 6)) / 2, btnY + 10, fogText, ColorRGB{160, 210, 240}, 1);

    // ID 905: VER EPITAFIO (ESC)
    constexpr int wEpi = 180;
    curRightX -= (wEpi + 8);
    bool epiHov = (hoveredButtonId == 905);
    drawRect(curRightX, btnY, wEpi, btnH, epiHov ? ColorRGB{70, 50, 20} : ColorRGB{40, 32, 18});
    drawRectOutline(curRightX, btnY, wEpi, btnH, epiHov ? Palette::DIVINE_GOLD : ColorRGB{140, 110, 50});
    std::string epiText = "VER EPITAFIO (ESC)";
    drawText(curRightX + (wEpi - static_cast<int>(epiText.size() * 6)) / 2, btnY + 10, epiText, Palette::DIVINE_GOLD, 1);

    // EN: 3. Bottom Help & Guidance Ribbon
    // ES: 3. Franja Inferior de Ayuda y Guía
    constexpr int botH = 26;
    const int botY = m_height - botH;
    drawRect(0, botY, m_width, botH, ColorRGB{14, 10, 12});
    drawRect(0, botY, m_width, 1, ColorRGB{60, 50, 40});
    std::string helpText = "Clic Derecho / WASD: Mover Camara  |  Rueda del Raton: Zoom (+/-)  |  R: Centrar  |  ESC / V: Volver al Epitafio";
    int helpW = static_cast<int>(helpText.size()) * 6;
    drawText((m_width - helpW) / 2, botY + 8, helpText, ColorRGB{180, 160, 140}, 1);
}

// =============================================================================
// EN: Developer Mode - Helper string conversions and telemetry routines
// ES: Modo Desarrollador - Conversiones auxiliares de texto y rutinas de telemetría
// =============================================================================
namespace {

inline const char* getDevActionName(HumanAction action) {
    switch (action) {
        case HumanAction::IDLE:      return "Descansando (Inactivo)";
        case HumanAction::WALK:      return "Caminando";
        case HumanAction::RUN:       return "Corriendo";
        case HumanAction::CHOP:      return "Talando Arboles";
        case HumanAction::GATHER:    return "Recolectando";
        case HumanAction::CARRY:     return "Cargando Recursos";
        case HumanAction::PRAY:      return "Orando ante Altar";
        case HumanAction::ATTACK:    return "En Combate / Guardia";
        case HumanAction::CELEBRATE: return "Celebrando Ritual";
        case HumanAction::DIE:       return "Falleciendo";
        case HumanAction::HURT:      return "Herido / Tambaleo";
        default:                     return "Inactivo";
    }
}

inline const char* getDevDirectionName(IsoDirection dir) {
    switch (dir) {
        case IsoDirection::SE: return "Sureste (+X)";
        case IsoDirection::SW: return "Suroeste (+Y)";
        case IsoDirection::NW: return "Noroeste (-X)";
        case IsoDirection::NE: return "Noreste (-Y)";
        default:               return "Sureste";
    }
}

inline const char* getDevBiomeName(BiomeType biome) {
    switch (biome) {
        case BiomeType::DeepOcean:       return "Oceano Profundo";
        case BiomeType::ShallowWater:    return "Aguas Someras";
        case BiomeType::CoastalPlains:   return "Llanuras Costeras";
        case BiomeType::FertileValley:   return "Valle Fertil";
        case BiomeType::DenseForest:     return "Bosque Denso";
        case BiomeType::HighlandHills:   return "Colinas Altas";
        case BiomeType::AridDesert:      return "Desierto Arido";
        case BiomeType::RuggedMountains: return "Montanas Escarpadas";
        case BiomeType::FrozenTundra:    return "Tundra Helada";
        case BiomeType::VolcanicCaldera: return "Caldera Volcanica";
        default:                         return "Terreno";
    }
}

} // anonymous namespace

// =============================================================================
// EN: Render 3D coordinate axes and tile boundary on hovered terrain cell
// ES: Renderizar ejes coordenados 3D y límite de celda sobre la casilla de terreno señalada
// =============================================================================
void Renderer::renderDeveloperCellAxes(const Point2i& cellCoords, const IsometricCamera& camera, const WorldMap& map) {
    if (!map.isValid(cellCoords.x, cellCoords.y)) return;

    // EN: Compute center screen coordinates factoring in cell elevation
    // ES: Calcular coordenadas de pantalla centrales considerando elevación de la celda
    const float elevation = map.getCell(cellCoords.x, cellCoords.y).elevation;
    const Vec2 centerPos = camera.gridTileCenterScreen(cellCoords.x, cellCoords.y, elevation);
    const float centerScreenX = centerPos.x;
    const float centerScreenY = centerPos.y;

    const int originScreenX = static_cast<int>(std::round(centerScreenX));
    const int originScreenY = static_cast<int>(std::round(centerScreenY));

    // EN: 1. Draw glowing isometric tile bounding diamond
    // ES: 1. Dibujar rombo perimetral iluminado de la casilla isométrica
    const float halfTileWidth = 32.0f * camera.getZoom();
    const float halfTileHeight = 16.0f * camera.getZoom();
    drawDiamondOutline(centerScreenX, centerScreenY, halfTileWidth, halfTileHeight, ColorRGB{60, 220, 255});
    drawDiamondOutline(centerScreenX, centerScreenY, halfTileWidth - 1.0f, halfTileHeight - 1.0f, ColorRGB{255, 255, 255});

    // EN: Directional arrow drawing lambda
    // ES: Lambda para dibujar flechas vectoriales con puntas perpendiculares
    auto drawVectorArrow = [this](int startX, int startY, int endX, int endY, ColorRGB color) {
        drawLine(startX, startY, endX, endY, color);
        const float deltaX = static_cast<float>(endX - startX);
        const float deltaY = static_cast<float>(endY - startY);
        const float arrowLength = std::sqrt(deltaX * deltaX + deltaY * deltaY);
        if (arrowLength > 5.0f) {
            const float normX = deltaX / arrowLength;
            const float normY = deltaY / arrowLength;
            const float perpX = -normY;
            const float perpY = normX;
            constexpr float HEAD_LEN = 8.0f;
            constexpr float HEAD_WIDTH = 5.0f;
            const int leftX = static_cast<int>(std::round(endX - normX * HEAD_LEN + perpX * HEAD_WIDTH));
            const int leftY = static_cast<int>(std::round(endY - normY * HEAD_LEN + perpY * HEAD_WIDTH));
            const int rightX = static_cast<int>(std::round(endX - normX * HEAD_LEN - perpX * HEAD_WIDTH));
            const int rightY = static_cast<int>(std::round(endY - normY * HEAD_LEN - perpY * HEAD_WIDTH));
            drawLine(endX, endY, leftX, leftY, color);
            drawLine(endX, endY, rightX, rightY, color);
        }
    };

    const float axisLength = 54.0f * std::clamp(camera.getZoom(), 0.75f, 1.8f);

    // EN: 2. Draw +X Axis (East-Southeast in isometric projection) in Bright Red
    // ES: 2. Dibujar Eje +X (Este-Sureste en proyección isométrica) en Rojo Brillante
    const Vec2 targetX_Pos = camera.gridToScreen(static_cast<float>(cellCoords.x) + 1.5f,
                                                 static_cast<float>(cellCoords.y) + 0.5f,
                                                 elevation);
    const float dirX_X = targetX_Pos.x - centerScreenX;
    const float dirX_Y = targetX_Pos.y - centerScreenY;
    const float lenX = std::max(0.001f, std::sqrt(dirX_X * dirX_X + dirX_Y * dirX_Y));
    const int endX_X = static_cast<int>(std::round(centerScreenX + (dirX_X / lenX) * axisLength));
    const int endX_Y = static_cast<int>(std::round(centerScreenY + (dirX_Y / lenX) * axisLength));
    drawVectorArrow(originScreenX, originScreenY, endX_X, endX_Y, ColorRGB{255, 50, 50});
    drawRect(endX_X + 2, endX_Y - 8, 22, 14, ColorRGB{20, 20, 24, 220});
    drawRectOutline(endX_X + 2, endX_Y - 8, 22, 14, ColorRGB{255, 60, 60});
    drawText(endX_X + 5, endX_Y - 6, "+X", ColorRGB{255, 255, 255}, 1);

    // EN: 3. Draw +Y Axis (South-Southwest in isometric projection) in Bright Green
    // ES: 3. Dibujar Eje +Y (Sur-Suroeste en proyección isométrica) en Verde Brillante
    const Vec2 targetY_Pos = camera.gridToScreen(static_cast<float>(cellCoords.x) + 0.5f,
                                                 static_cast<float>(cellCoords.y) + 1.5f,
                                                 elevation);
    const float dirY_X = targetY_Pos.x - centerScreenX;
    const float dirY_Y = targetY_Pos.y - centerScreenY;
    const float lenY = std::max(0.001f, std::sqrt(dirY_X * dirY_X + dirY_Y * dirY_Y));
    const int endY_X = static_cast<int>(std::round(centerScreenX + (dirY_X / lenY) * axisLength));
    const int endY_Y = static_cast<int>(std::round(centerScreenY + (dirY_Y / lenY) * axisLength));
    drawVectorArrow(originScreenX, originScreenY, endY_X, endY_Y, ColorRGB{50, 240, 50});
    drawRect(endY_X - 24, endY_Y - 8, 22, 14, ColorRGB{20, 20, 24, 220});
    drawRectOutline(endY_X - 24, endY_Y - 8, 22, 14, ColorRGB{60, 255, 60});
    drawText(endY_X - 21, endY_Y - 6, "+Y", ColorRGB{255, 255, 255}, 1);

    // EN: 4. Draw +Z Axis (Vertical elevation upwards in isometric projection) in Electric Blue
    // ES: 4. Dibujar Eje +Z (Elevación vertical hacia arriba en proyección isométrica) en Azul Eléctrico
    const int endZ_X = originScreenX;
    const int endZ_Y = static_cast<int>(std::round(centerScreenY - axisLength));
    drawVectorArrow(originScreenX, originScreenY, endZ_X, endZ_Y, ColorRGB{50, 160, 255});
    drawRect(endZ_X - 11, endZ_Y - 18, 22, 14, ColorRGB{20, 20, 24, 220});
    drawRectOutline(endZ_X - 11, endZ_Y - 18, 22, 14, ColorRGB{70, 180, 255});
    drawText(endZ_X - 8, endZ_Y - 16, "+Z", ColorRGB{255, 255, 255}, 1);

    // EN: Origin center point indicator
    // ES: Indicador del punto de origen central
    drawRect(originScreenX - 2, originScreenY - 2, 5, 5, ColorRGB{255, 255, 255});
}

// =============================================================================
// EN: Render per-unit developer status badges, local axes and collision footprints
// ES: Renderizar distintivos de estado, ejes locales y huellas de colisión por unidad
// =============================================================================
void Renderer::renderDeveloperUnitOverlays(const GestorHumanos& humanos, const IsometricCamera& camera,
                                          const WorldMap& map, const Humano* hoveredVillager,
                                          const Humano* selectedVillager) {
    const auto& villagers = humanos.getVillagers();
    for (const auto& villagerUnit : villagers) {
        if (!villagerUnit.isAlive()) continue;

        // EN: Compute projected foot position of the unit factoring in elevation
        // ES: Calcular posición de pies proyectada de la unidad considerando elevación
        float unitElevation = 1.0f;
        if (map.isValid(villagerUnit.getCellX(), villagerUnit.getCellY())) {
            unitElevation = map.getCell(villagerUnit.getCellX(), villagerUnit.getCellY()).elevation;
        }
        const Vec2 unitScreenPos = camera.gridToScreen(villagerUnit.getX(), villagerUnit.getY(), unitElevation);
        const float unitScreenX = unitScreenPos.x;
        const float unitScreenY = unitScreenPos.y;

        // EN: Viewport culling with safety margins
        // ES: Descarte de viewport con márgenes de seguridad
        if (unitScreenX < -60.0f || unitScreenX > static_cast<float>(m_width) + 60.0f ||
            unitScreenY < -60.0f || unitScreenY > static_cast<float>(m_height) + 60.0f) {
            continue;
        }

        const bool isHovered = (hoveredVillager && hoveredVillager->getId() == villagerUnit.getId());
        const bool isSelected = (selectedVillager && selectedVillager->getId() == villagerUnit.getId());

        const int footX = static_cast<int>(std::round(unitScreenX));
        const int footY = static_cast<int>(std::round(unitScreenY));

        // EN: 1. Draw collision footprint diamond at unit's feet
        // ES: 1. Dibujar rombo de huella de colisión en los pies de la unidad
        const float footRadius = 9.0f * std::clamp(camera.getZoom(), 0.7f, 1.5f);
        const ColorRGB ringColor = isSelected ? Palette::DIVINE_GOLD : (isHovered ? ColorRGB{255, 255, 130} : ColorRGB{80, 200, 255});
        drawDiamondOutline(unitScreenX, unitScreenY, footRadius, footRadius * 0.5f, ringColor);

        // EN: 2. Local direction vector in movement/facing orientation
        // ES: 2. Vector de dirección local en la orientación de movimiento/mirada
        float dirOffsetX = 0.0f;
        float dirOffsetY = 0.0f;
        switch (villagerUnit.getDirection()) {
            case IsoDirection::SE: dirOffsetX =  1.0f; dirOffsetY =  0.5f; break;
            case IsoDirection::SW: dirOffsetX = -1.0f; dirOffsetY =  0.5f; break;
            case IsoDirection::NE: dirOffsetX =  1.0f; dirOffsetY = -0.5f; break;
            case IsoDirection::NW: dirOffsetX = -1.0f; dirOffsetY = -0.5f; break;
        }
        const float dirLen = std::sqrt(dirOffsetX * dirOffsetX + dirOffsetY * dirOffsetY);
        constexpr float DIR_ARROW_LEN = 14.0f;
        const int dirTargetX = static_cast<int>(std::round(unitScreenX + (dirOffsetX / dirLen) * DIR_ARROW_LEN));
        const int dirTargetY = static_cast<int>(std::round(unitScreenY + (dirOffsetY / dirLen) * DIR_ARROW_LEN));
        drawLine(footX, footY, dirTargetX, dirTargetY, ColorRGB{255, 230, 80});

        // EN: 3. Overhead status tag with ID, name, HP, condition color and mini health bar
        // ES: 3. Etiqueta flotante superior con ID, nombre, HP, color de condición y mini barra de salud
        const std::string nameText = "#" + std::to_string(villagerUnit.getId()) + " " + villagerUnit.getName();
        const int tagWidth = std::max(88, static_cast<int>(nameText.size()) * 7 + 10);
        constexpr int tagHeight = 16;
        const int tagX = footX - tagWidth / 2;
        const int tagY = static_cast<int>(std::round(unitScreenY - 44.0f * camera.getZoom()));

        drawRect(tagX, tagY, tagWidth, tagHeight, ColorRGB{12, 16, 22, 220});
        drawRectOutline(tagX, tagY, tagWidth, tagHeight, ringColor);

        const ColorRGB conditionColor = villagerUnit.getConditionColor();
        drawText(tagX + 4, tagY + 2, nameText, conditionColor, 1);

        // Health bar along bottom of tag
        constexpr float MAX_VILLAGER_HEALTH = 100.0f;
        const float healthPercent = std::clamp(villagerUnit.getSalud() / MAX_VILLAGER_HEALTH, 0.0f, 1.0f);
        drawRect(tagX + 2, tagY + tagHeight - 3, tagWidth - 4, 2, ColorRGB{35, 35, 40});
        const ColorRGB healthColor = (healthPercent > 0.5f) ? ColorRGB{46, 215, 80} : ((healthPercent > 0.25f) ? ColorRGB{245, 165, 35} : ColorRGB{235, 45, 45});
        drawRect(tagX + 2, tagY + tagHeight - 3, static_cast<int>((tagWidth - 4) * healthPercent), 2, healthColor);

        if (isSelected || isHovered) {
            drawLine(footX, tagY + tagHeight, footX, tagY + tagHeight + 5, ringColor);
        }
    }
}

// =============================================================================
// EN: Render screen-space 3D orientation compass gizmo
// ES: Renderizar el gizmo brújula de orientación 3D en espacio de pantalla
// =============================================================================
void Renderer::renderDeveloperOrientationGizmo(int screenPosX, int screenPosY) {
    constexpr int GIZMO_RADIUS = 26;
    drawRect(screenPosX - GIZMO_RADIUS - 4, screenPosY - GIZMO_RADIUS - 4,
             (GIZMO_RADIUS + 4) * 2, (GIZMO_RADIUS + 4) * 2, ColorRGB{14, 18, 26, 215});
    drawRectOutline(screenPosX - GIZMO_RADIUS - 4, screenPosY - GIZMO_RADIUS - 4,
                    (GIZMO_RADIUS + 4) * 2, (GIZMO_RADIUS + 4) * 2, ColorRGB{60, 140, 220});

    auto drawGizmoArrow = [this](int startX, int startY, int endX, int endY, ColorRGB color) {
        drawLine(startX, startY, endX, endY, color);
        const float deltaX = static_cast<float>(endX - startX);
        const float deltaY = static_cast<float>(endY - startY);
        const float len = std::sqrt(deltaX * deltaX + deltaY * deltaY);
        if (len > 3.0f) {
            const float nx = deltaX / len;
            const float ny = deltaY / len;
            drawLine(endX, endY, static_cast<int>(endX - nx * 5.0f - ny * 3.5f), static_cast<int>(endY - ny * 5.0f + nx * 3.5f), color);
            drawLine(endX, endY, static_cast<int>(endX - nx * 5.0f + ny * 3.5f), static_cast<int>(endY - ny * 5.0f - nx * 3.5f), color);
        }
    };

    // +X (Isometric East: +0.866, +0.500)
    constexpr float ISO_LEN = 19.0f;
    const int endX_X = static_cast<int>(screenPosX + 0.866f * ISO_LEN);
    const int endX_Y = static_cast<int>(screenPosY + 0.500f * ISO_LEN);
    drawGizmoArrow(screenPosX, screenPosY, endX_X, endX_Y, ColorRGB{255, 60, 60});
    drawText(endX_X + 2, endX_Y - 4, "X", ColorRGB{255, 100, 100}, 1);

    // +Y (Isometric South: -0.866, +0.500)
    const int endY_X = static_cast<int>(screenPosX - 0.866f * ISO_LEN);
    const int endY_Y = static_cast<int>(screenPosY + 0.500f * ISO_LEN);
    drawGizmoArrow(screenPosX, screenPosY, endY_X, endY_Y, ColorRGB{60, 240, 60});
    drawText(endY_X - 9, endY_Y - 4, "Y", ColorRGB{100, 255, 100}, 1);

    // +Z (Vertical Up: 0.0, -1.0)
    const int endZ_X = screenPosX;
    const int endZ_Y = static_cast<int>(screenPosY - ISO_LEN);
    drawGizmoArrow(screenPosX, screenPosY, endZ_X, endZ_Y, ColorRGB{60, 160, 255});
    drawText(endZ_X - 3, endZ_Y - 12, "Z", ColorRGB{120, 200, 255}, 1);

    // Center pivot
    drawRect(screenPosX - 1, screenPosY - 1, 3, 3, ColorRGB{255, 255, 255});
}

// =============================================================================
// EN: Render comprehensive developer HUD, unit inspector and isometric 3D axes
// ES: Renderizar HUD completo de desarrollador, inspector de unidades y ejes 3D isométricos
// =============================================================================
void Renderer::renderDeveloperOverlay(const WorldMap& map, const ReligionRegistry& registry,
                                      const Timeline& timeline, const GestorHumanos& humanos,
                                      const IsometricCamera& camera, const Point2i& hoveredCell,
                                      const Humano* hoveredVillager, const Humano* selectedVillager,
                                      const WorldCreationConfig& playerConfig,
                                      const TimeEngine* timeEngine,
                                      const DayNightCycle* dayNight) {
    if (!m_developerMode) return;

    // EN: 1. Render per-unit visual markers and local vectors
    // ES: 1. Renderizar marcadores visuales y vectores locales por unidad
    renderDeveloperUnitOverlays(humanos, camera, map, hoveredVillager, selectedVillager);

    // EN: 2. Render 3D coordinate axes and bounding diamond on hovered terrain cell
    // ES: 2. Renderizar ejes coordenados 3D y rombo perimetral en la casilla de terreno señalada
    if (map.isValid(hoveredCell.x, hoveredCell.y)) {
        renderDeveloperCellAxes(hoveredCell, camera, map);
    }

    // EN: 3. Render Orientation Compass Gizmo in upper right
    // ES: 3. Renderizar Gizmo Brújula de Orientación en la esquina superior derecha
    renderDeveloperOrientationGizmo(m_width - 48, 80);

    // =========================================================================
    // EN: 4. Left Panel - Engine Performance, Hardware, World & Hovered Cell
    // ES: 4. Panel Izquierdo - Rendimiento del Motor, Hardware, Mundo y Casilla
    // =========================================================================
    constexpr int leftPanelX = 12;
    constexpr int leftPanelY = 40;
    constexpr int leftPanelWidth = 370;
    constexpr int leftPanelHeight = 295;

    drawRect(leftPanelX, leftPanelY, leftPanelWidth, leftPanelHeight, ColorRGB{12, 16, 24, 225});
    drawRectOutline(leftPanelX, leftPanelY, leftPanelWidth, leftPanelHeight, ColorRGB{60, 140, 220});
    drawRect(leftPanelX, leftPanelY, leftPanelWidth, 22, ColorRGB{22, 38, 58, 240});
    drawText(leftPanelX + 8, leftPanelY + 5, "[MODO DESARROLLADOR / DEV MODE] (F3)", Palette::DIVINE_GOLD, 1);

    const auto healthMetrics = ErrorSystem::instance().getHealthMetrics();
    int curY = leftPanelY + 28;
    constexpr int LINE_STEP = 14;

    // Telemetry: Performance & Memory
    char perfBuffer[128];
    std::snprintf(perfBuffer, sizeof(perfBuffer), "FPS: %.1f (%.1fms) [Prom: %.1fms] | Drops: %u",
                  healthMetrics.currentFps, healthMetrics.currentFrameTimeMs,
                  healthMetrics.averageFrameTimeMs, healthMetrics.droppedFrameCount);
    const ColorRGB fpsColor = (healthMetrics.currentFps >= 50.0f) ? ColorRGB{60, 230, 80}
                            : ((healthMetrics.currentFps >= 28.0f) ? ColorRGB{240, 190, 40} : ColorRGB{255, 60, 60});
    drawText(leftPanelX + 8, curY, perfBuffer, fpsColor, 1);
    curY += LINE_STEP;

    char memBuffer[128];
    std::snprintf(memBuffer, sizeof(memBuffer), "RAM: %.1f MB (Pico: %.1f MB) | Lag: %s",
                  healthMetrics.currentMemoryMb, healthMetrics.peakMemoryMb,
                  healthMetrics.isUnderLagStress ? "STRESS" : "OPTIMO");
    drawText(leftPanelX + 8, curY, memBuffer, ColorRGB{200, 220, 240}, 1);
    curY += LINE_STEP;

    char entBuffer[128];
    std::snprintf(entBuffer, sizeof(entBuffer), "Entidades Vivas: %d / %u Seguras | Capped: %s",
                  static_cast<int>(humanos.getVillagers().size()), healthMetrics.maxSafeEntityLimit,
                  healthMetrics.isPopulationCapped ? "SI" : "NO");
    drawText(leftPanelX + 8, curY, entBuffer, ColorRGB{180, 205, 230}, 1);
    curY += LINE_STEP;

    // Separator
    drawRect(leftPanelX + 8, curY + 2, leftPanelWidth - 16, 1, ColorRGB{40, 70, 100});
    curY += 7;

    // Telemetry: Simulation & World
    uint64_t totalTicks = 0;
    std::string speedStr = "1.0x";
    if (timeEngine) {
        const auto timeState = timeEngine->getTimeState();
        totalTicks = timeState.total_ticks;
        if (timeEngine->isPaused()) {
            speedStr = "PAUSA";
        } else if (timeState.time_multiplier == TimeMultiplier::Fast) {
            speedStr = "2.0x";
        } else if (timeState.time_multiplier == TimeMultiplier::Ultra) {
            speedStr = "5.0x";
        }
    }

    char worldBuffer[128];
    std::snprintf(worldBuffer, sizeof(worldBuffer), "Mundo: %dx%d | Semilla: %u | Ticks: %llu",
                  map.getWidth(), map.getHeight(), playerConfig.mapSeed, static_cast<unsigned long long>(totalTicks));
    drawText(leftPanelX + 8, curY, worldBuffer, ColorRGB{220, 220, 220}, 1);
    curY += LINE_STEP;

    const int currentYearVal = timeline.getCurrentYear();
    char timeBuffer[128];
    std::snprintf(timeBuffer, sizeof(timeBuffer), "Ano: %d %s | Vel: %s | Religiones: %d",
                  std::abs(currentYearVal), (currentYearVal < 0 ? "BCE" : "CE"),
                  speedStr.c_str(), static_cast<int>(registry.getAllReligions().size()));
    drawText(leftPanelX + 8, curY, timeBuffer, ColorRGB{220, 220, 220}, 1);
    curY += LINE_STEP;

    float hourOfDay = 12.0f;
    std::string solarState = "Mediodia";
    if (dayNight) {
        hourOfDay = dayNight->getHour();
        solarState = dayNight->getPhaseName();
    }
    char solarBuffer[128];
    std::snprintf(solarBuffer, sizeof(solarBuffer), "Ciclo Solar: %02d:%02d (%s)",
                  static_cast<int>(hourOfDay), static_cast<int>((hourOfDay - std::floor(hourOfDay)) * 60.0f),
                  solarState.c_str());
    drawText(leftPanelX + 8, curY, solarBuffer, ColorRGB{235, 205, 120}, 1);
    curY += LINE_STEP;

    // Separator
    drawRect(leftPanelX + 8, curY + 2, leftPanelWidth - 16, 1, ColorRGB{40, 70, 100});
    curY += 7;

    // Telemetry: Camera & Viewport
    char camBuffer[128];
    std::snprintf(camBuffer, sizeof(camBuffer), "Camara: Zoom: %.2fx | Pantalla: %dx%d %s",
                  camera.getZoom(), m_width, m_height, isFullscreen() ? "(Full)" : "(Win)");
    drawText(leftPanelX + 8, curY, camBuffer, ColorRGB{190, 220, 240}, 1);
    curY += LINE_STEP;

    // Separator
    drawRect(leftPanelX + 8, curY + 2, leftPanelWidth - 16, 1, ColorRGB{40, 70, 100});
    curY += 7;

    // Telemetry: Hovered Cell
    if (map.isValid(hoveredCell.x, hoveredCell.y)) {
        const auto& cell = map.getCell(hoveredCell.x, hoveredCell.y);
        char cellHeader[128];
        std::snprintf(cellHeader, sizeof(cellHeader), "CASILLA [X: %d, Y: %d] | Elevacion: %.2fm",
                      hoveredCell.x, hoveredCell.y, cell.elevation);
        drawText(leftPanelX + 8, curY, cellHeader, Palette::DIVINE_GOLD, 1);
        curY += LINE_STEP;

        char biomeBuffer[128];
        std::snprintf(biomeBuffer, sizeof(biomeBuffer), "Bioma: %s | Recurso: %s (%d/%d)",
                      getDevBiomeName(cell.biome), getResourceTypeName(cell.naturalResource),
                      cell.resourceAmount, cell.maxResourceAmount);
        drawText(leftPanelX + 8, curY, biomeBuffer, ColorRGB{210, 230, 245}, 1);
        curY += LINE_STEP;

        char waterFireBuffer[128];
        std::snprintf(waterFireBuffer, sizeof(waterFireBuffer), "Agua: %.2fm (%s) | Fuego: %s | Calor: %.1f",
                      cell.waterDepth, (cell.isFlooded ? "Inundada" : "Seca"),
                      (cell.isBurning ? "ARDIDENDO" : (cell.isBurnt ? "Calcinada" : "No")),
                      cell.heatAccumulator);
        drawText(leftPanelX + 8, curY, waterFireBuffer, ColorRGB{210, 230, 245}, 1);
        curY += LINE_STEP;

        std::string structName = "Ninguna";
        if (cell.hasHolySite) structName = "Templo Sagrado (Altar Lv." + std::to_string(cell.shrineLevel) + ")";
        else if (cell.hasHouse) structName = "Vivienda Tribal (#" + std::to_string(cell.houseId) + ")";
        else if (cell.hasVolcano) structName = "Volcan Eruptivo Activo";
        else if (cell.hasImpactCrater) structName = "Crater de Impacto";
        drawText(leftPanelX + 8, curY, "Estructuras: " + structName, ColorRGB{190, 220, 240}, 1);
        curY += LINE_STEP;
    } else {
        drawText(leftPanelX + 8, curY, "CASILLA: Fuera de los limites del mapa", ColorRGB{160, 160, 160}, 1);
        curY += LINE_STEP;
    }

    // Separator
    drawRect(leftPanelX + 8, curY + 2, leftPanelWidth - 16, 1, ColorRGB{40, 70, 100});
    curY += 7;

    // Controls Help Footer
    drawText(leftPanelX + 8, curY, "Atajos: F3: Dev Mode | F11: Pantalla | H: Reticula", ColorRGB{140, 175, 205}, 1);

    // =========================================================================
    // EN: 5. Right Panel - Unit Inspector (Hovered or Selected Villager)
    // ES: 5. Panel Derecho - Inspector de Unidad (Aldeano Bajo Cursor o Seleccionado)
    // =========================================================================
    const Humano* targetVillager = hoveredVillager ? hoveredVillager : selectedVillager;
    constexpr int rightPanelWidth = 380;
    const int rightPanelX = m_width - rightPanelWidth - 12;
    constexpr int rightPanelY = 40;
    constexpr int rightPanelHeight = 330;

    drawRect(rightPanelX, rightPanelY, rightPanelWidth, rightPanelHeight, ColorRGB{12, 16, 24, 225});
    drawRectOutline(rightPanelX, rightPanelY, rightPanelWidth, rightPanelHeight,
                    targetVillager ? Palette::DIVINE_GOLD : ColorRGB{60, 140, 220});

    const std::string unitHeader = targetVillager
        ? ("INSPECTOR DE UNIDAD [" + std::string(hoveredVillager ? "CURSOR" : "SELECCION") + "]")
        : "INSPECTOR DE UNIDADES [MODO DEV]";
    drawRect(rightPanelX, rightPanelY, rightPanelWidth, 22, ColorRGB{35, 28, 16, 240});
    drawText(rightPanelX + 8, rightPanelY + 5, unitHeader, Palette::DIVINE_GOLD, 1);

    int rCurY = rightPanelY + 28;

    if (targetVillager) {
        char nameBuffer[128];
        std::snprintf(nameBuffer, sizeof(nameBuffer), "ID: #%d | Nombre: %s (%s)",
                      targetVillager->getId(), targetVillager->getName().c_str(),
                      targetVillager->isMale() ? "Hombre" : "Mujer");
        drawText(rightPanelX + 8, rCurY, nameBuffer, ColorRGB{255, 255, 255}, 1);
        rCurY += LINE_STEP;

        const std::string roleStr = targetVillager->isShaman() ? "Chaman Sagrado"
                                  : (targetVillager->isProphet() ? "Gran Profeta"
                                  : (targetVillager->isChild() ? "Nino / Descendiente" : "Aldeano"));
        char roleBuffer[128];
        std::snprintf(roleBuffer, sizeof(roleBuffer), "Edad: %.1f anos | Rol: %s",
                      targetVillager->getAge(), roleStr.c_str());
        drawText(rightPanelX + 8, rCurY, roleBuffer, ColorRGB{210, 225, 240}, 1);
        rCurY += LINE_STEP;

        // Condition
        const ColorRGB condColor = targetVillager->getConditionColor();
        drawText(rightPanelX + 8, rCurY, "Condicion: " + targetVillager->getConditionName(true), condColor, 1);
        rCurY += LINE_STEP;

        // Separator
        drawRect(rightPanelX + 8, rCurY + 2, rightPanelWidth - 16, 1, ColorRGB{80, 65, 30});
        rCurY += 7;

        // Coordinates & Axes
        drawText(rightPanelX + 8, rCurY, "EJES & COORDENADAS ESPACIALES:", Palette::DIVINE_GOLD, 1);
        rCurY += LINE_STEP;

        float unitElevation = 1.0f;
        if (map.isValid(targetVillager->getCellX(), targetVillager->getCellY())) {
            unitElevation = map.getCell(targetVillager->getCellX(), targetVillager->getCellY()).elevation;
        }

        char coordMundoBuffer[128];
        std::snprintf(coordMundoBuffer, sizeof(coordMundoBuffer), "  Mundo: (X: %.2f, Y: %.2f, Z: %.2f Elev)",
                      targetVillager->getX(), targetVillager->getY(), unitElevation);
        drawText(rightPanelX + 8, rCurY, coordMundoBuffer, ColorRGB{200, 230, 255}, 1);
        rCurY += LINE_STEP;

        const Vec2 unitScreenPos = camera.gridToScreen(targetVillager->getX(), targetVillager->getY(), unitElevation);
        const float unitScreenX = unitScreenPos.x;
        const float unitScreenY = unitScreenPos.y;

        char coordGridBuffer[128];
        std::snprintf(coordGridBuffer, sizeof(coordGridBuffer), "  Celda: [X: %d, Y: %d] | Pantalla: (%d, %d)px",
                      targetVillager->getCellX(), targetVillager->getCellY(),
                      static_cast<int>(std::round(unitScreenX)), static_cast<int>(std::round(unitScreenY)));
        drawText(rightPanelX + 8, rCurY, coordGridBuffer, ColorRGB{200, 230, 255}, 1);
        rCurY += LINE_STEP;

        char dirBuffer[128];
        std::snprintf(dirBuffer, sizeof(dirBuffer), "  Direccion: %s | Velocidad: %.2f u/s",
                      getDevDirectionName(targetVillager->getDirection()), targetVillager->getSpeed());
        drawText(rightPanelX + 8, rCurY, dirBuffer, ColorRGB{200, 230, 255}, 1);
        rCurY += LINE_STEP;

        // Separator
        drawRect(rightPanelX + 8, rCurY + 2, rightPanelWidth - 16, 1, ColorRGB{80, 65, 30});
        rCurY += 7;

        // Health, Devotion & Biology
        constexpr float MAX_VILLAGER_HP = 100.0f;
        const float hpVal = targetVillager->getSalud();
        const float hpRatio = std::clamp(hpVal / MAX_VILLAGER_HP, 0.0f, 1.0f);
        char hpBuffer[128];
        std::snprintf(hpBuffer, sizeof(hpBuffer), "Salud: %.1f / %.1f (%.0f%%) | Energia: %.1f%%",
                      hpVal, MAX_VILLAGER_HP, hpRatio * 100.0f, targetVillager->getEnergy());
        drawText(rightPanelX + 8, rCurY, hpBuffer, ColorRGB{255, 255, 255}, 1);
        rCurY += LINE_STEP;

        // Health meter bar
        drawRect(rightPanelX + 8, rCurY, rightPanelWidth - 16, 4, ColorRGB{35, 35, 40});
        const ColorRGB barHealthColor = (hpRatio > 0.5f) ? ColorRGB{46, 215, 80} : ((hpRatio > 0.25f) ? ColorRGB{245, 165, 35} : ColorRGB{235, 45, 45});
        drawRect(rightPanelX + 8, rCurY, static_cast<int>((rightPanelWidth - 16) * hpRatio), 4, barHealthColor);
        rCurY += 8;

        const Religion* pRel = registry.getReligion(targetVillager->getReligionId());
        const std::string relName = pRel ? pRel->name : "Ninguna";
        char faithBuffer[128];
        std::snprintf(faithBuffer, sizeof(faithBuffer), "Fe / Devocion: %.0f%% | Religion: %s (#%d)",
                      targetVillager->getDevotion() * 100.0f, relName.c_str(), targetVillager->getReligionId());
        drawText(rightPanelX + 8, rCurY, faithBuffer, ColorRGB{240, 215, 120}, 1);
        rCurY += LINE_STEP;

        const std::string sleepSuffix = targetVillager->isSleeping()
            ? (targetVillager->isSleepingIndoors() ? " [Durmiendo bajo techo]" : " [Durmiendo al aire libre]")
            : "";
        char sleepBuffer[128];
        std::snprintf(sleepBuffer, sizeof(sleepBuffer), "Sueno: %.1f%%%s",
                      targetVillager->getSleepNeed(), sleepSuffix.c_str());
        drawText(rightPanelX + 8, rCurY, sleepBuffer, ColorRGB{190, 220, 240}, 1);
        rCurY += LINE_STEP;

        const auto& moralRecord = targetVillager->getMoralRecord();
        char moralBuffer[128];
        std::snprintf(moralBuffer, sizeof(moralBuffer), "Moral: %.0f%% | Culpa: %.1f%% | Pecados: %d",
                      moralRecord.moralIntegrity * 100.0f, moralRecord.guiltLevel, moralRecord.sinsCommittedCount);
        drawText(rightPanelX + 8, rCurY, moralBuffer, ColorRGB{200, 215, 230}, 1);
        rCurY += LINE_STEP;

        // Separator
        drawRect(rightPanelX + 8, rCurY + 2, rightPanelWidth - 16, 1, ColorRGB{80, 65, 30});
        rCurY += 7;

        // Behavioral & Social
        drawText(rightPanelX + 8, rCurY, "Accion: " + std::string(getDevActionName(targetVillager->getAction())), ColorRGB{255, 255, 255}, 1);
        rCurY += LINE_STEP;

        const std::string spouseStr = targetVillager->hasSpouse()
            ? ("#" + std::to_string(targetVillager->getSpouseId()) + " (Amor: " + std::to_string(static_cast<int>(targetVillager->getSpouseAffection() * 100.0f)) + "%)")
            : "Soltero";
        char socialBuffer[128];
        std::snprintf(socialBuffer, sizeof(socialBuffer), "Pareja: %s | Hijos: %d",
                      spouseStr.c_str(), targetVillager->getChildrenCount());
        drawText(rightPanelX + 8, rCurY, socialBuffer, ColorRGB{210, 225, 240}, 1);
        rCurY += LINE_STEP;

        const std::string homeStr = targetVillager->hasHome()
            ? ("Vivienda Tribal #" + std::to_string(targetVillager->getHomeId()))
            : "Sin Refugio";
        char homeBuffer[128];
        std::snprintf(homeBuffer, sizeof(homeBuffer), "Hogar: %s | Furia: %s | Fuego: %s",
                      homeStr.c_str(),
                      targetVillager->hasProtectiveFury() ? "ACTIVA" : "No",
                      targetVillager->isBurning() ? "EN LLAMAS" : "No");
        drawText(rightPanelX + 8, rCurY, homeBuffer, ColorRGB{210, 225, 240}, 1);
        rCurY += LINE_STEP;
    } else {
        rCurY += 40;
        drawText(rightPanelX + 16, rCurY, "Ninguna unidad seleccionada.", ColorRGB{200, 200, 200}, 1);
        rCurY += LINE_STEP + 4;
        drawText(rightPanelX + 16, rCurY, "Posa el cursor o haz clic en cualquier", ColorRGB{160, 180, 200}, 1);
        rCurY += LINE_STEP;
        drawText(rightPanelX + 16, rCurY, "aldeano para inspeccionar sus ejes 3D,", ColorRGB{160, 180, 200}, 1);
        rCurY += LINE_STEP;
        drawText(rightPanelX + 16, rCurY, "nombre, coordenadas y propiedades vitales.", ColorRGB{160, 180, 200}, 1);
    }
}

} // namespace ReligionSim
