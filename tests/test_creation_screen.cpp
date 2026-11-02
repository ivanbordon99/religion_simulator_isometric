#include "Renderer.hpp"
#include <iostream>
#include <fstream>

int main() {
    const int width = 1280;
    const int height = 720;
    ReligionSim::Renderer renderer(width, height);
    ReligionSim::WorldCreationConfig config;

    renderer.beginFrame();
    renderer.renderWorldCreationMenu(config, 0, 0);
    renderer.endFrame();

    const auto& fb = renderer.getFramebuffer();
    std::ofstream ppm("test_creation_screen.ppm", std::ios::binary);
    ppm << "P6\n" << width << " " << height << "\n255\n";
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            uint32_t px = fb[y * width + x];
            uint8_t r = px & 0xFF;
            uint8_t g = (px >> 8) & 0xFF;
            uint8_t b = (px >> 16) & 0xFF;
            ppm.write(reinterpret_cast<char*>(&r), 1);
            ppm.write(reinterpret_cast<char*>(&g), 1);
            ppm.write(reinterpret_cast<char*>(&b), 1);
        }
    }
    ppm.close();
    std::cout << "Rendered test_creation_screen.ppm" << std::endl;
    return 0;
}
