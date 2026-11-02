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
#include <cassert>

static void savePPM(const std::string& path, const std::vector<uint32_t>& buffer, int width, int height) {
    std::ofstream out(path, std::ios::binary);
    out << "P6\n" << width << " " << height << "\n255\n";
    for (int y = 0; y < height; ++y) {
        int invY = height - 1 - y;
        for (int x = 0; x < width; ++x) {
            uint32_t px = buffer[invY * width + x];
            uint8_t r = px & 0xFF;
            uint8_t g = (px >> 8) & 0xFF;
            uint8_t b = (px >> 16) & 0xFF;
            out.put(static_cast<char>(r));
            out.put(static_cast<char>(g));
            out.put(static_cast<char>(b));
        }
    }
}

int main() {
    std::cout << "[Test & Snapshot] Renderizando ocultacion de sprites bajo techo, limites de capacidad y reproduccion..." << std::endl;

    const int width = 1280;
    const int height = 720;

    ReligionSim::WorldMap map(32, 32);
    for (int y = 0; y < 32; ++y) {
        for (int x = 0; x < 32; ++x) {
            auto& c = map.getCell(x, y);
            c.elevation = 1.0f;
            c.biome = (x >= 12 && x <= 20 && y >= 12 && y <= 20)
                      ? ReligionSim::BiomeType::FertileValley
                      : ReligionSim::BiomeType::HighlandHills;
            c.population = 25;
            c.hasHouse = false;
        }
    }

    ReligionSim::ReligionRegistry registry;
    int relPlayer = registry.createPrimordialReligion("Clan de la Aurora", "Grog el Sabio",
                                                      ReligionSim::ColorRGB(230, 161, 59), "Tierra", -4000);

    ReligionSim::BuildingSystem buildingSys;
    ReligionSim::DayNightCycle dayNight(90.0f, 23.0f); // Noche para apreciar brillo de hogar y zZ

    ReligionSim::IsometricCamera camera(width, height);
    camera.setZoom(1.40f);
    camera.centerOnCell(16, 16, 32, 32);
    camera.update(2.0f);

    ReligionSim::Renderer renderer(width, height);
    renderer.setBuildingSystem(&buildingSys);
    renderer.setDayNightCycle(&dayNight);

    ReligionSim::GestorHumanos gestor;

    // 1. Crear vivienda ThatchedHut (capacidad 4) en (16, 16)
    int houseId = buildingSys.initiateHouseConstruction(16, 16, 1, 2, relPlayer,
                                                        ReligionSim::HouseType::ThatchedHut, map);
    ReligionSim::House* h = buildingSys.getHouseByIdMutable(houseId);
    assert(h != nullptr);
    h->state = ReligionSim::BuildingState::Completed;
    h->buildProgress = 100.0f;

    // 2. Progenitores durmiendo bajo techo
    ReligionSim::Humano padre(1, "Grog el Protector", 16.5f, 16.5f, ReligionSim::GeneroHumano::HOMBRE);
    padre.setReligionId(relPlayer);
    padre.setHomeId(houseId);
    padre.setDurmiendoBajoTecho(true); // Oculto bajo techo
    padre.setDevocion(0.92f);
    padre.getHumanUnitMutable().nutritionalState = 85.0f;
    padre.getPopGroupMutable().foodSupply = 70.0f;

    ReligionSim::Humano madre(2, "Vea del Hogar", 16.5f, 16.5f, ReligionSim::GeneroHumano::MUJER);
    madre.setReligionId(relPlayer);
    madre.setHomeId(houseId);
    madre.setDurmiendoBajoTecho(true); // Oculta bajo techo
    madre.setDevocion(0.95f);
    madre.getHumanUnitMutable().nutritionalState = 88.0f;
    madre.getPopGroupMutable().foodSupply = 70.0f;

    padre.setParejaId(madre.getId());
    madre.setParejaId(padre.getId());
    padre.setAmorHaciaPareja(0.85f);
    madre.setAmorHaciaPareja(0.85f);

    // 3. Aldeano nómada durmiendo a la intemperie (fogata en 14, 16)
    ReligionSim::Humano nomada(3, "Brak el Errante", 14.5f, 16.5f, ReligionSim::GeneroHumano::HOMBRE);
    nomada.setReligionId(relPlayer);
    nomada.setDurmiendo(true); // Durmiendo en el suelo (sprite visible + zZ)
    nomada.setSueno(80.0f);

    gestor.agregarHumano(padre);
    gestor.agregarHumano(madre);
    gestor.agregarHumano(nomada);

    // Censo del edificio
    buildingSys.update(0.1f, map, gestor);

    // Seleccionar la celda de la choza para examinar el inspector
    ReligionSim::Point2i selectedCell = {16, 16};
    gestor.seleccionarHumano(padre.getId());

    ReligionSim::Timeline timeline;
    ReligionSim::EventLine eventLine;
    ReligionSim::ReplicationEngine repl;
    ReligionSim::WorldCreationConfig config;

    // Renderizar escena 1: Choza con brillo cálido, zZ en tejado, aldeanos ocultos dentro, y nómada en suelo
    renderer.beginFrame();
    renderer.render(map, registry, timeline, eventLine, repl, gestor, camera,
                    selectedCell, ReligionSim::RenderMapMode::TerrainBiomes, config);
    renderer.renderInspector(map, registry, selectedCell);
    renderer.endFrame();

    savePPM("snapshot_indoor_sleep_hidden_sprites.ppm", renderer.getFramebuffer(), width, height);
    std::cout << "  -> [OK] snapshot_indoor_sleep_hidden_sprites.ppm guardado con exito!" << std::endl;

    // Escena 2: Seleccionar al nómada durmiendo en el suelo
    gestor.seleccionarHumano(nomada.getId());
    selectedCell = {nomada.getCeldaX(), nomada.getCeldaY()};

    renderer.beginFrame();
    renderer.render(map, registry, timeline, eventLine, repl, gestor, camera,
                    selectedCell, ReligionSim::RenderMapMode::TerrainBiomes, config);
    renderer.endFrame();

    savePPM("snapshot_outdoor_sleeper_contrast.ppm", renderer.getFramebuffer(), width, height);
    std::cout << "  -> [OK] snapshot_outdoor_sleeper_contrast.ppm guardado con exito!" << std::endl;

    return 0;
}
