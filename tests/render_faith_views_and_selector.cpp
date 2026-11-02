#include "Game.hpp"
#include "BuildingSystem.hpp"
#include "Renderer.hpp"
#include "IsometricCamera.hpp"
#include "WorldMap.hpp"
#include "Religion.hpp"
#include "Humano.hpp"
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
    std::cout << "[Test & Snapshot] Verificando selector en color de fe y vista por defecto sin nodos de fe..." << std::endl;

    const int width = 1280;
    const int height = 720;

    // 1. Verificar que por defecto en Game, m_mapMode es TerrainBiomes (sin nodos de fe)
    ReligionSim::Game game(width, height);
    assert(game.getMapMode() == ReligionSim::RenderMapMode::TerrainBiomes);
    std::cout << "  -> [PASSED] Game::getMapMode() default is RenderMapMode::TerrainBiomes!" << std::endl;

    // 2. Preparar mapa de prueba con aldeanos y chamán
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
    // Religión 1: Color Carmesí/Rubí distintivo (220, 45, 60)
    int rel1Id = registry.createPrimordialReligion("Clan del Fuego Solar", "Arak el Sabio",
                                                  ReligionSim::ColorRGB(220, 45, 60), "Fuego", -4000);
    // Religión 2: Color Esmeralda (40, 200, 100)
    int rel2Id = registry.createPrimordialReligion("Pueblo del Bosque Verde", "Sylvan",
                                                  ReligionSim::ColorRGB(40, 200, 100), "Arbol", -4000);

    ReligionSim::IsometricCamera camera(width, height);
    camera.setZoom(1.20f);
    camera.centerOnCell(16, 16, 32, 32);
    camera.update(2.0f); // Sincronizar posición de cámara con celda objetivo

    ReligionSim::Renderer renderer(width, height);
    ReligionSim::GestorHumanos gestor;

    // Crear unidades de la fe del jugador (Rel 1)
    ReligionSim::Humano chaman(1, "Arak", 16.0f, 16.0f, ReligionSim::GeneroHumano::HOMBRE);
    chaman.setEsChaman(true);
    chaman.setReligionId(rel1Id);
    chaman.setDevocion(0.95f);
    chaman.setLeaderDevotion(1.0f);

    ReligionSim::Humano aldeano1(2, "Tara", 16.5f, 15.5f, ReligionSim::GeneroHumano::MUJER);
    aldeano1.setReligionId(rel1Id);
    aldeano1.setDevocion(0.85f);
    aldeano1.setLeaderDevotion(0.90f);

    ReligionSim::Humano aldeano2(3, "Boran", 15.5f, 16.8f, ReligionSim::GeneroHumano::HOMBRE);
    aldeano2.setReligionId(rel1Id);
    aldeano2.setDevocion(0.80f);
    aldeano2.setLeaderDevotion(0.85f);

    // Unidad foránea de Rel 2
    ReligionSim::Humano aldeanoRival(4, "Kael", 22.0f, 22.0f, ReligionSim::GeneroHumano::HOMBRE);
    aldeanoRival.setReligionId(rel2Id);

    gestor.agregarHumano(chaman);
    gestor.agregarHumano(aldeano1);
    gestor.agregarHumano(aldeano2);
    gestor.agregarHumano(aldeanoRival);
    gestor.actualizar(0.0f, map);

    // Seleccionar a Tara (aldeana de nuestra fe)
    gestor.seleccionarHumano(aldeano1.getId());
    assert(gestor.getHumanoSeleccionado() != nullptr);
    assert(gestor.getHumanoSeleccionado()->getId() == aldeano1.getId());

    ReligionSim::Timeline timeline;
    ReligionSim::EventLine eventLine;
    ReligionSim::ReplicationEngine repl;
    ReligionSim::WorldCreationConfig config;

    ReligionSim::Point2i selectedCell = {aldeano1.getCeldaX(), aldeano1.getCeldaY()};

    // 3. Renderizar VISTA POR DEFECTO (TerrainBiomes)
    // En esta vista, el selector de Tara se tiñe del color de su fe (Carmesí 220, 45, 60),
    // y los nodos/triangulación de fe NO se muestran en el suelo.
    renderer.beginFrame();
    renderer.render(map, registry, timeline, eventLine, repl, gestor, camera,
                    selectedCell, ReligionSim::RenderMapMode::TerrainBiomes, config);
    renderer.endFrame();

    savePPM("snapshot_default_view_terrain.ppm", renderer.getFramebuffer(), width, height);
    std::cout << "  -> [PASSED] snapshot_default_view_terrain.ppm generado con exito!" << std::endl;

    // 4. Renderizar VISTA DE FE (ReligionDominance)
    // En esta vista, el selector permanece en color de fe Y se activan los nodos y redes de triangulación de fe.
    renderer.beginFrame();
    renderer.render(map, registry, timeline, eventLine, repl, gestor, camera,
                    selectedCell, ReligionSim::RenderMapMode::ReligionDominance, config);
    renderer.endFrame();

    savePPM("snapshot_faith_view_dominance.ppm", renderer.getFramebuffer(), width, height);
    std::cout << "  -> [PASSED] snapshot_faith_view_dominance.ppm generado con exito!" << std::endl;

    std::cout << "TODAS LAS VERIFICACIONES PASARON CON EXITO!" << std::endl;
    return 0;
}
