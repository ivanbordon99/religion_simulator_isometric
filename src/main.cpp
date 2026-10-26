#include "Game.hpp"
#include <iostream>
#include <string>
#include <cstdlib>

int main(int argc, char** argv) {
    std::cout << "==========================================================" << std::endl;
    std::cout << "             MARANGATU: THE FAITH SIMULATOR               " << std::endl;
    std::cout << "  2D Isometric World | Replication | Agglutination | Era  " << std::endl;
    std::cout << "==========================================================" << std::endl;

    bool headless = false;
    int ticks = 180;
    std::string exportFile = "simulation_snapshot.ppm";

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--headless") {
            headless = true;
        } else if (arg == "--ticks" && i + 1 < argc) {
            ticks = std::atoi(argv[++i]);
        } else if (arg == "--export" && i + 1 < argc) {
            exportFile = argv[++i];
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "Marangatu: The Faith Simulator\n"
                      << "Usage: ./bin/religion_sim [options]\n"
                      << "  --headless              Run simulation without opening an X11 window\n"
                      << "  --ticks <N>             Number of simulation ticks to run (default: 180)\n"
                      << "  --export <file.ppm>     Export map snapshot to PPM image\n"
                      << "  --help                  Show this help text\n";
            return 0;
        }
    }

#if !defined(_WIN32)
    const char* displayEnv = std::getenv("DISPLAY");
    if (!displayEnv || std::string(displayEnv).empty()) {
        std::cout << "[Note] DISPLAY environment variable is not set. Switching to headless mode.\n"
                  << "       To run native window with GUI, run: DISPLAY=:0 ./bin/religion_sim\n";
        headless = true;
    }
#endif

    ReligionSim::Game game(1280, 720);
    if (!game.init()) {
        std::cerr << "[Error] Failed to initialize simulation." << std::endl;
        return 1;
    }

    if (headless) {
        game.runHeadless(ticks, exportFile);
    } else {
        std::cout << "[Simulator] Controls:\n"
                  << "  - Left Click: Select cell / Click UI buttons / Scrub timeline\n"
                  << "  - Right Click Drag: Pan camera\n"
                  << "  - Mouse Wheel / (+/-): Zoom in and out\n"
                  << "  - Space: Pause / Resume time\n"
                  << "  - 1-4: Switch view modes (Faiths, Biomes, Syncretism, Eras)\n"
                  << "  - WASD: Pan camera\n"
                  << "  - R: Reset camera\n"
                  << "  - ESC: Exit\n";
        game.run();
    }

    return 0;
}
