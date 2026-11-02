#include "WorldMap.hpp"
#include "Humano.hpp"
#include <iostream>
#include <fstream>
#include <cmath>

int main() {
    using namespace ReligionSim;
    const int mapDim = 48;
    WorldMap map(mapDim, mapDim);
    uint32_t seed = 3477209493u;
    map.generateProceduralWorld(seed, WorldClimate::Templado, ResourceAbundance::Equilibrado);

    auto pueblos = map.generatePuebloLocations(5, seed);

    ReligionRegistry reg;
    for (int i = 0; i < 5; ++i) {
        reg.createPrimordialReligion("Religion " + std::to_string(i + 1), "Founder " + std::to_string(i + 1),
                                     ColorRGB{200, 100, 50}, "Sun", -4000);
        map.consecrateHolySite(pueblos[i].x, pueblos[i].y, "Santuario " + std::to_string(i + 1), i + 1);
    }

    GestorHumanos gestor;
    gestor.inicializarPoblacion(map, reg, 80);

    const int imgW = 768;
    const int imgH = 768;
    std::vector<uint8_t> pixels(imgW * imgH * 3, 20);

    int tilePx = imgW / mapDim;
    for (int y = 0; y < mapDim; ++y) {
        for (int x = 0; x < mapDim; ++x) {
            const Cell& c = map.getCell(x, y);
            ColorRGB col = c.getBiomeBaseColor();
            if (c.hasHolySite) col = ColorRGB{255, 215, 0};

            for (int py = 0; py < tilePx; ++py) {
                int iy = y * tilePx + py;
                for (int px = 0; px < tilePx; ++px) {
                    int ix = x * tilePx + px;
                    int idx = (iy * imgW + ix) * 3;
                    pixels[idx] = col.r;
                    pixels[idx + 1] = col.g;
                    pixels[idx + 2] = col.b;
                }
            }
        }
    }

    static const ColorRGB puebloColors[5] = {
        ColorRGB{255, 230, 80},  // Jugador: Oro
        ColorRGB{60, 220, 100},  // Gaia: Verde
        ColorRGB{80, 180, 255},  // Monada: Azul
        ColorRGB{255, 80, 80},   // Llama: Rojo
        ColorRGB{200, 100, 255}  // Dharma: Purpura
    };

    for (size_t p = 0; p < pueblos.size(); ++p) {
        int cx = pueblos[p].x * tilePx + tilePx / 2;
        int cy = pueblos[p].y * tilePx + tilePx / 2;
        ColorRGB col = puebloColors[p % 5];

        for (int dy = -16; dy <= 16; ++dy) {
            for (int dx = -16; dx <= 16; ++dx) {
                int r2 = dx * dx + dy * dy;
                if (r2 >= 100 && r2 <= 144) {
                    int ix = cx + dx;
                    int iy = cy + dy;
                    if (ix >= 0 && ix < imgW && iy >= 0 && iy < imgH) {
                        int idx = (iy * imgW + ix) * 3;
                        pixels[idx] = col.r;
                        pixels[idx + 1] = col.g;
                        pixels[idx + 2] = col.b;
                    }
                }
            }
        }
    }

    for (const auto& h : gestor.getHumanos()) {
        int ix = static_cast<int>(h.getX() * tilePx);
        int iy = static_cast<int>(h.getY() * tilePx);
        ColorRGB uCol = puebloColors[(h.getReligionId() - 1) % 5];
        if (h.esChaman()) uCol = ColorRGB{255, 255, 255};

        for (int dy = -2; dy <= 2; ++dy) {
            for (int dx = -2; dx <= 2; ++dx) {
                int px = ix + dx;
                int py = iy + dy;
                if (px >= 0 && px < imgW && py >= 0 && py < imgH) {
                    int idx = (py * imgW + px) * 3;
                    pixels[idx] = uCol.r;
                    pixels[idx + 1] = uCol.g;
                    pixels[idx + 2] = uCol.b;
                }
            }
        }
    }

    std::ofstream ppm("pueblo_spread.ppm", std::ios::binary);
    ppm << "P6\n" << imgW << " " << imgH << "\n255\n";
    ppm.write(reinterpret_cast<char*>(pixels.data()), pixels.size());
    ppm.close();

    std::cout << "Rendered pueblo_spread.ppm successfully!" << std::endl;
    return 0;
}
