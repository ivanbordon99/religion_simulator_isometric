#include "Game.hpp"
#include "BuildingSystem.hpp"
#include "DivinePowersSystem.hpp"
#include "Renderer.hpp"
#include "IsometricCamera.hpp"
#include "WorldMap.hpp"
#include "Religion.hpp"
#include "Humano.hpp"
#include <iostream>
#include <fstream>
#include <vector>

int main() {
    std::cout << "[Snapshot] Generando vista de Incendio Forestal y barra de poderes con iconos..." << std::endl;

    const int width = 1280;
    const int height = 720;

    ReligionSim::WorldMap map(32, 32);
    for (int y = 0; y < 32; ++y) {
        for (int x = 0; x < 32; ++x) {
            auto& c = map.getCell(x, y);
            c.elevation = 1.0f;
            if (x >= 12 && x <= 22 && y >= 12 && y <= 22) {
                c.biome = ReligionSim::BiomeType::DenseForest;
                c.naturalResource = ReligionSim::ResourceType::Wood;
                c.resourceAmount = 80;
                c.maxResourceAmount = 100;
            } else {
                c.biome = ReligionSim::BiomeType::FertileValley;
            }
            c.population = 40;
            c.hasHouse = false;
            c.houseId = -1;
        }
    }

    ReligionSim::ReligionRegistry registry;
    int relId = registry.createPrimordialReligion("Clan del Roble Sagrado", "Solarius", ReligionSim::ColorRGB(230, 161, 59), "Roble", -4000);
    auto* pRel = registry.getReligion(relId);
    if (pRel) {
        pRel->theologyPoints = 120;
    }

    ReligionSim::IsometricCamera camera(width, height);
    camera.setZoom(1.20f);
    camera.centerOnCell(16, 16, 32, 32);

    ReligionSim::Renderer renderer(width, height);
    ReligionSim::DivinePowersSystem powersSystem;
    ReligionSim::GestorHumanos gestor;
    ReligionSim::EventLine eventLine;

    // Crear líder espiritual y aldeanos
    ReligionSim::Humano chaman(1, "Grog", 14.5f, 15.0f, ReligionSim::GeneroHumano::HOMBRE);
    chaman.setReligionId(relId);
    chaman.setEsChaman(true);
    gestor.agregarHumano(chaman);

    ReligionSim::Humano aldeano1(2, "Tara", 17.0f, 16.0f, ReligionSim::GeneroHumano::MUJER);
    aldeano1.setReligionId(relId);
    gestor.agregarHumano(aldeano1);

    ReligionSim::Humano aldeano2(3, "Broll", 18.0f, 15.5f, ReligionSim::GeneroHumano::HOMBRE);
    aldeano2.setReligionId(relId);
    gestor.agregarHumano(aldeano2);

    // Lanzar Incendio Forestal en (16, 16)
    powersSystem.castPower(ReligionSim::DivinePowerType::Wildfire, 16, 16, map, registry, gestor, -4000, eventLine, relId);

    // Avanzar la simulación unos segundos para ver el frente de fuego, consumo y cenizas
    for (int i = 0; i < 8; ++i) {
        powersSystem.update(0.5f, map, gestor);
    }

    // Configurar una celda como ya calcinada para mostrar el suelo de ceniza
    map.getCell(15, 16).isBurning = false;
    map.getCell(15, 16).isBurnt = true;
    map.getCell(15, 16).biome = ReligionSim::BiomeType::CoastalPlains;

    map.getCell(16, 15).isBurning = false;
    map.getCell(16, 15).isBurnt = true;
    map.getCell(16, 15).biome = ReligionSim::BiomeType::CoastalPlains;

    ReligionSim::Point2i selectedCell{16, 16};
    gestor.seleccionarHumano(1);

    ReligionSim::Timeline timeline(-4000, 2200);
    ReligionSim::ReplicationEngine replication;
    ReligionSim::WorldCreationConfig config;
    config.religionName = "Clan del Roble Sagrado";
    config.godName = "Solarius";

    renderer.beginFrame();
    renderer.render(map, registry, timeline, eventLine, replication, gestor,
                    camera, selectedCell, ReligionSim::RenderMapMode::ReligionDominance,
                    config, nullptr, &powersSystem);
    renderer.endFrame();

    // Guardar buffer en formato PPM
    const auto& fb = renderer.getFramebuffer();
    std::ofstream ppm("snapshot_wildfire.ppm", std::ios::binary);
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

    std::cout << "[Snapshot] Guardado exitosamente como snapshot_wildfire.ppm" << std::endl;
    return 0;
}
