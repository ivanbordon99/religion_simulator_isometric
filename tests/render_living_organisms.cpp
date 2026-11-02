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
    std::cout << "[Test & Snapshot] Renderizando organismos vivos: descanso circadiano y furia protectora..." << std::endl;

    const int width = 1280;
    const int height = 720;

    // 1. Preparar mapa con biomas habitables
    ReligionSim::WorldMap map(32, 32);
    for (int y = 0; y < 32; ++y) {
        for (int x = 0; x < 32; ++x) {
            auto& c = map.getCell(x, y);
            c.elevation = 1.0f;
            c.biome = (x >= 10 && x <= 22 && y >= 10 && y <= 22)
                      ? ReligionSim::BiomeType::FertileValley
                      : ReligionSim::BiomeType::HighlandHills;
            c.population = 25;
            c.hasHouse = false;
        }
    }

    ReligionSim::ReligionRegistry registry;
    int relPlayer = registry.createPrimordialReligion("Clan del Fuego Solar", "Arak el Sabio",
                                                      ReligionSim::ColorRGB(235, 95, 45), "Fuego", -4000);
    int relRival = registry.createPrimordialReligion("Invasores de las Sombras", "Malor",
                                                     ReligionSim::ColorRGB(60, 75, 160), "Sombra", -4000);

    ReligionSim::BuildingSystem buildingSys;
    ReligionSim::DayNightCycle dayNight(90.0f, 23.5f); // 23:30 Noche cerrada

    ReligionSim::IsometricCamera camera(width, height);
    camera.setZoom(1.30f);
    camera.centerOnCell(16, 16, 32, 32);
    camera.update(2.0f);

    ReligionSim::Renderer renderer(width, height);
    ReligionSim::GestorHumanos gestor;

    // Crear núcleo familiar
    ReligionSim::Humano padre(1, "Grog el Protector", 16.0f, 16.0f, ReligionSim::GeneroHumano::HOMBRE);
    padre.setReligionId(relPlayer);
    padre.setDevocion(0.92f);
    padre.setLeaderDevotion(0.85f);

    ReligionSim::Humano madre(2, "Vea del Hogar", 16.2f, 15.8f, ReligionSim::GeneroHumano::MUJER);
    madre.setReligionId(relPlayer);
    madre.setDevocion(0.95f);
    madre.setLeaderDevotion(0.90f);

    padre.setParejaId(madre.getId());
    madre.setParejaId(padre.getId());
    padre.setAmorHaciaPareja(0.88f);
    madre.setAmorHaciaPareja(0.88f);

    ReligionSim::Humano hijo(3, "Torin", 16.3f, 16.1f, ReligionSim::GeneroHumano::HOMBRE);
    hijo.setReligionId(relPlayer);
    hijo.setEsNino(true);
    hijo.setEdad(3.0f);
    hijo.setMadreId(madre.getId());
    hijo.setPadreId(padre.getId());
    padre.registrarHijo(hijo.getId());
    madre.registrarHijo(hijo.getId());

    // Choza terminada en celda (16, 16)
    int houseId = buildingSys.initiateHouseConstruction(16, 16, padre.getId(), madre.getId(), relPlayer,
                                                        ReligionSim::HouseType::ThatchedHut, map);
    ReligionSim::House* h = buildingSys.getHouseByIdMutable(houseId);
    assert(h != nullptr);
    h->state = ReligionSim::BuildingState::Completed;
    h->buildProgress = 100.0f;
    padre.setHomeId(houseId);
    madre.setHomeId(houseId);
    hijo.setHomeId(houseId);

    // Familia durmiendo plácidamente
    padre.setDurmiendo(true);
    padre.setSueno(65.0f);
    padre.getHumanUnitMutable().energy = 45.0f;

    madre.setDurmiendo(true);
    madre.setSueno(70.0f);

    hijo.setDurmiendo(true);
    hijo.setSueno(60.0f);

    gestor.agregarHumano(padre);
    gestor.agregarHumano(madre);
    gestor.agregarHumano(hijo);

    // Seleccionar al padre durmiendo en su choza para inspeccionar HUD
    gestor.seleccionarHumano(padre.getId());

    ReligionSim::Timeline timeline;
    ReligionSim::EventLine eventLine;
    ReligionSim::ReplicationEngine repl;
    ReligionSim::WorldCreationConfig config;
    ReligionSim::Point2i selectedCell = {padre.getCeldaX(), padre.getCeldaY()};

    // ESCENA 1: Familia durmiendo bajo la noche en su hogar
    renderer.beginFrame();
    renderer.render(map, registry, timeline, eventLine, repl, gestor, camera,
                    selectedCell, ReligionSim::RenderMapMode::TerrainBiomes, config);
    renderer.endFrame();

    savePPM("snapshot_sleeping_villager.ppm", renderer.getFramebuffer(), width, height);
    std::cout << "  -> [OK] snapshot_sleeping_villager.ppm generado con éxito!" << std::endl;

    // ESCENA 2: Invasor ataca a la familia -> Despiertan con Furia Protectora
    ReligionSim::Humano enemigo(99, "Invasor Malor", 16.4f, 16.0f, ReligionSim::GeneroHumano::HOMBRE);
    enemigo.setReligionId(relRival);
    enemigo.cambiarAccion(ReligionSim::AccionHumano::ATACAR);
    enemigo.setAttackCooldown(0.0f);
    gestor.agregarHumano(enemigo);

    // Ejecutar actualización de combate: el padre recibe impacto y la madre e hijo desatan furia protectora
    gestor.actualizar(0.5f, map, nullptr, -4000, &registry, nullptr, nullptr, nullptr, nullptr, &dayNight, &buildingSys);

    // Seleccionar a la madre que ahora tiene Furia Protectora activa
    gestor.seleccionarHumano(madre.getId());
    selectedCell = {madre.getCeldaX(), madre.getCeldaY()};

    renderer.beginFrame();
    renderer.render(map, registry, timeline, eventLine, repl, gestor, camera,
                    selectedCell, ReligionSim::RenderMapMode::TerrainBiomes, config);
    renderer.endFrame();

    savePPM("snapshot_family_defense_fury.ppm", renderer.getFramebuffer(), width, height);
    std::cout << "  -> [OK] snapshot_family_defense_fury.ppm generado con éxito!" << std::endl;

    std::cout << "Renderizados de verificación completados exitosamente!" << std::endl;
    return 0;
}
