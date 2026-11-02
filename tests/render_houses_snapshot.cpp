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

int main() {
    std::cout << "[Snapshot] Generando vista de asentamiento tribal con casas y obras..." << std::endl;

    const int width = 1280;
    const int height = 720;

    ReligionSim::WorldMap map(32, 32);
    for (int y = 0; y < 32; ++y) {
        for (int x = 0; x < 32; ++x) {
            auto& c = map.getCell(x, y);
            c.elevation = 1.0f;
            if (x >= 8 && x <= 24 && y >= 8 && y <= 24) {
                c.biome = ReligionSim::BiomeType::FertileValley;
            } else {
                c.biome = ReligionSim::BiomeType::CoastalPlains;
            }
            c.population = 40;
            c.hasHouse = false;
            c.houseId = -1;
        }
    }

    ReligionSim::ReligionRegistry registry;
    registry.createPrimordialReligion("Clan del Roble Sagrado", "Grog el Sabio", ReligionSim::ColorRGB(230, 161, 59), "Roble", -4000);

    ReligionSim::IsometricCamera camera(width, height);
    camera.setZoom(1.15f);
    camera.centerOnCell(16, 16, 32, 32);

    ReligionSim::Renderer renderer(width, height);
    ReligionSim::BuildingSystem buildingSys;
    ReligionSim::GestorHumanos gestor;

    // Crear parejas y aldeanos
    // Familia 1: Grog y Tara (Choza de paja completada en 15, 15)
    ReligionSim::Humano grog(1, "Grog", 14.8f, 15.2f, ReligionSim::GeneroHumano::HOMBRE);
    ReligionSim::Humano tara(2, "Tara", 15.2f, 15.3f, ReligionSim::GeneroHumano::MUJER);
    grog.setParejaId(2);
    tara.setParejaId(1);
    grog.setAmorHaciaPareja(0.92f);
    tara.setAmorHaciaPareja(0.92f);
    grog.setHijosCount(1);
    tara.setHijosCount(1);

    ReligionSim::Humano bebe(3, "Kaelen", 15.0f, 15.5f, ReligionSim::GeneroHumano::HOMBRE);
    bebe.setEsNino(true);
    bebe.setEdad(3.0f);
    bebe.setMadreId(2);
    bebe.setPadreId(1);

    // Familia 2: Broll y Vala (En obras: Armazón de madera en 17, 14)
    ReligionSim::Humano broll(4, "Broll", 17.2f, 13.8f, ReligionSim::GeneroHumano::HOMBRE);
    ReligionSim::Humano vala(5, "Vala", 16.8f, 14.2f, ReligionSim::GeneroHumano::MUJER);
    broll.setParejaId(5);
    vala.setParejaId(4);
    broll.setAmorHaciaPareja(0.88f);
    vala.setAmorHaciaPareja(0.88f);
    vala.setEmbarazada(true);
    vala.setEmbarazoProgreso(0.65f);
    broll.cambiarAccion(ReligionSim::AccionHumano::TALAR);
    vala.cambiarAccion(ReligionSim::AccionHumano::CARGAR);

    // Familia 3: Mamut y Pieles (Cabaña de huesos completada en 13, 17)
    ReligionSim::Humano karg(6, "Karg", 13.0f, 17.3f, ReligionSim::GeneroHumano::HOMBRE);
    ReligionSim::Humano morrigh(7, "Morrigh", 13.4f, 16.8f, ReligionSim::GeneroHumano::MUJER);
    karg.setParejaId(7);
    morrigh.setParejaId(6);

    // Chamán de la tribu
    ReligionSim::Humano chaman(8, "Chaman Orok", 16.5f, 16.5f, ReligionSim::GeneroHumano::HOMBRE);
    chaman.setEsChaman(true);
    chaman.setDevocion(0.99f);

    gestor.agregarHumano(grog);
    gestor.agregarHumano(tara);
    gestor.agregarHumano(bebe);
    gestor.agregarHumano(broll);
    gestor.agregarHumano(vala);
    gestor.agregarHumano(karg);
    gestor.agregarHumano(morrigh);
    gestor.agregarHumano(chaman);

    // Construir Casas en el sistema
    // 1. Choza Circular de Paja completada
    int h1 = buildingSys.initiateHouseConstruction(15, 15, 1, 2, 1, ReligionSim::HouseType::ThatchedHut, map);
    ReligionSim::House* house1 = buildingSys.getHouseByIdMutable(h1);
    if (house1) {
        house1->state = ReligionSim::BuildingState::Completed;
        house1->buildProgress = 100.0f;
        house1->residentIds.push_back(3);
    }
    grog.setHomeId(h1);
    tara.setHomeId(h1);
    bebe.setHomeId(h1);

    // 2. Choza de Madera en Construcción (Framing 42%)
    int h2 = buildingSys.initiateHouseConstruction(17, 14, 4, 5, 1, ReligionSim::HouseType::ThatchedHut, map);
    ReligionSim::House* house2 = buildingSys.getHouseByIdMutable(h2);
    if (house2) {
        house2->state = ReligionSim::BuildingState::Framing;
        house2->buildProgress = 42.0f;
        house2->woodDelivered = 8;
    }
    broll.setHomeId(h2);
    vala.setHomeId(h2);
    broll.setEstaConstruyendoHogar(true);
    vala.setEstaConstruyendoHogar(true);

    // 3. Cabaña de Huesos de Mamut completada
    int h3 = buildingSys.initiateHouseConstruction(13, 17, 6, 7, 1, ReligionSim::HouseType::BoneDwelling, map);
    ReligionSim::House* house3 = buildingSys.getHouseByIdMutable(h3);
    if (house3) {
        house3->state = ReligionSim::BuildingState::Completed;
        house3->buildProgress = 100.0f;
    }
    karg.setHomeId(h3);
    morrigh.setHomeId(h3);

    // 4. Gran Casa Comunal Redonda completada
    int h4 = buildingSys.initiateHouseConstruction(18, 17, 8, -1, 1, ReligionSim::HouseType::LargeRoundhouse, map);
    ReligionSim::House* house4 = buildingSys.getHouseByIdMutable(h4);
    if (house4) {
        house4->state = ReligionSim::BuildingState::Completed;
        house4->buildProgress = 100.0f;
    }
    chaman.setHomeId(h4);

    // 5. Armazón de Huesos de Mamut en construcción (25%)
    int h5 = buildingSys.initiateHouseConstruction(14, 13, -1, -1, 1, ReligionSim::HouseType::BoneDwelling, map);
    ReligionSim::House* house5 = buildingSys.getHouseByIdMutable(h5);
    if (house5) {
        house5->state = ReligionSim::BuildingState::Framing;
        house5->buildProgress = 25.0f;
    }

    // Configurar Renderer
    renderer.setBuildingSystem(&buildingSys);

    // Seleccionar la celda de la choza terminada para mostrarla en el Inspector de provincia
    ReligionSim::Point2i selectedCell{15, 15};
    // Seleccionar a Broll (que está construyendo su casa) para que el panel inferior muestre su labor
    gestor.seleccionarHumano(4);

    ReligionSim::Timeline timeline(-4000, 2200);
    ReligionSim::EventLine eventLine;
    ReligionSim::ReplicationEngine replication;
    ReligionSim::WorldCreationConfig config;
    config.religionName = "Clan del Roble Sagrado";
    config.godName = "Solarius";

    renderer.beginFrame();
    renderer.render(map, registry, timeline, eventLine, replication, gestor,
                    camera, selectedCell, ReligionSim::RenderMapMode::ReligionDominance,
                    config);
    renderer.endFrame();

    // Guardar buffer en formato PPM
    const auto& fb = renderer.getFramebuffer();
    std::ofstream ppm("snapshot_village_houses.ppm", std::ios::binary);
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

    std::cout << "[Snapshot] Guardado exitosamente como snapshot_village_houses.ppm" << std::endl;
    return 0;
}
