#include "Game.hpp"
#include "BuildingSystem.hpp"
#include "Renderer.hpp"
#include "IsometricCamera.hpp"
#include "WorldMap.hpp"
#include "Religion.hpp"
#include "Humano.hpp"
#include "DayNightCycle.hpp"
#include <iostream>
#include <fstream>
#include <vector>

int main() {
    std::cout << "[Snapshot] Generando vista nocturna de aldea con fogatas de hogar..." << std::endl;

    const int width = 1280;
    const int height = 720;

    ReligionSim::WorldMap map(32, 32);
    for (int y = 0; y < 32; ++y) {
        for (int x = 0; x < 32; ++x) {
            auto& c = map.getCell(x, y);
            c.elevation = 1.0f;
            c.biome = (x >= 8 && x <= 24 && y >= 8 && y <= 24)
                      ? ReligionSim::BiomeType::FertileValley
                      : ReligionSim::BiomeType::CoastalPlains;
            c.population = 40;
            c.hasHouse = false;
            c.houseId = -1;
        }
    }

    ReligionSim::ReligionRegistry registry;
    registry.createPrimordialReligion("Clan del Roble Sagrado", "Grog el Sabio", ReligionSim::ColorRGB(230, 161, 59), "Roble", -4000);

    ReligionSim::IsometricCamera camera(width, height);
    camera.setZoom(1.20f);
    camera.centerOnCell(16, 16, 32, 32);

    ReligionSim::Renderer renderer(width, height);
    ReligionSim::BuildingSystem buildingSys;
    ReligionSim::GestorHumanos gestor;

    // Día/Noche en plena medianoche (0.0h)
    ReligionSim::DayNightCycle dayNight(90.0f, 0.0f);
    renderer.setDayNightCycle(&dayNight);

    // Casas
    int h1 = buildingSys.initiateHouseConstruction(15, 15, 1, 2, 1, ReligionSim::HouseType::ThatchedHut, map);
    ReligionSim::House* house1 = buildingSys.getHouseByIdMutable(h1);
    if (house1) { house1->state = ReligionSim::BuildingState::Completed; house1->buildProgress = 100.0f; }

    int h2 = buildingSys.initiateHouseConstruction(17, 14, 4, 5, 1, ReligionSim::HouseType::ThatchedHut, map);
    ReligionSim::House* house2 = buildingSys.getHouseByIdMutable(h2);
    if (house2) { house2->state = ReligionSim::BuildingState::Framing; house2->buildProgress = 65.0f; }

    int h3 = buildingSys.initiateHouseConstruction(13, 17, 6, 7, 1, ReligionSim::HouseType::BoneDwelling, map);
    ReligionSim::House* house3 = buildingSys.getHouseByIdMutable(h3);
    if (house3) { house3->state = ReligionSim::BuildingState::Completed; house3->buildProgress = 100.0f; }

    int h4 = buildingSys.initiateHouseConstruction(18, 17, 8, -1, 1, ReligionSim::HouseType::LargeRoundhouse, map);
    ReligionSim::House* house4 = buildingSys.getHouseByIdMutable(h4);
    if (house4) { house4->state = ReligionSim::BuildingState::Completed; house4->buildProgress = 100.0f; }

    // Aldeanos durmiendo cerca de sus hogares
    ReligionSim::Humano grog(1, "Grog", 15.0f, 15.2f, ReligionSim::GeneroHumano::HOMBRE);
    ReligionSim::Humano tara(2, "Tara", 15.3f, 15.1f, ReligionSim::GeneroHumano::MUJER);
    grog.setHomeId(h1);
    tara.setHomeId(h1);
    gestor.agregarHumano(grog);
    gestor.agregarHumano(tara);

    renderer.setBuildingSystem(&buildingSys);

    ReligionSim::Timeline timeline(-4000, 2200);
    ReligionSim::EventLine eventLine;
    ReligionSim::ReplicationEngine replication;
    ReligionSim::WorldCreationConfig config;
    config.religionName = "Clan del Roble Sagrado";
    config.godName = "Solarius";

    renderer.beginFrame();
    renderer.render(map, registry, timeline, eventLine, replication, gestor,
                    camera, {15, 15}, ReligionSim::RenderMapMode::ReligionDominance,
                    config, nullptr, nullptr, {-1, -1}, &dayNight);
    renderer.endFrame();

    const auto& fb = renderer.getFramebuffer();
    std::ofstream ppm("snapshot_village_night.ppm", std::ios::binary);
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

    std::cout << "[Snapshot] Guardado exitosamente como snapshot_village_night.ppm" << std::endl;
    return 0;
}
