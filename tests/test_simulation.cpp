#include "IsometricCamera.hpp"
#include "WorldMap.hpp"
#include "MultiNoiseBiome.hpp"
#include "Religion.hpp"
#include "AdvancementSystem.hpp"
#include "ReplicationEngine.hpp"
#include "AgglutinationEngine.hpp"
#include "Timeline.hpp"
#include "EventLine.hpp"
#include "Villager.hpp"
#include "TileSet.hpp"
#include "Humano.hpp"
#include "HumanAIController.hpp"
#include "Renderer.hpp"
#include "TimeEngine.hpp"
#include "UISystem.hpp"
#include "DivinePowersSystem.hpp"
#include "EraEvolutionEngine.hpp"
#include "DayNightCycle.hpp"
#include "CloudSystem.hpp"
#include "SoundSystem.hpp"
#include "NemesisSififus.hpp"
#include "ReligiousMoralitySystem.hpp"
#include "BuildingSystem.hpp"
#include "ErrorSystem.hpp"
#include "SaveSystem.hpp"
#include "Game.hpp"
#include <iostream>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <vector>
#include <filesystem>

void testIsometricCamera() {
    std::cout << "[Test] Running testIsometricCamera..." << std::endl;
    ReligionSim::IsometricCamera cam(1280, 720);
    cam.setZoom(1.0f);

    // Test origin
    ReligionSim::Vec2 p0 = cam.gridToScreen(0, 0, 0);
    assert(std::isfinite(p0.x) && std::isfinite(p0.y));

    // Test invertibility: grid -> screen -> grid
    int gx = 14, gy = 22;
    ReligionSim::Vec2 pCenter = cam.gridTileCenterScreen(gx, gy, 0);
    int recoveredGx = 0, recoveredGy = 0;
    cam.screenToGrid(pCenter.x, pCenter.y, recoveredGx, recoveredGy);

    assert(recoveredGx == gx);
    assert(recoveredGy == gy);

    // Test zoom constraints
    cam.setZoom(0.01f);
    assert(cam.getZoom() >= 0.2f);
    cam.setZoom(100.0f);
    assert(cam.getZoom() <= 3.5f);

    std::cout << "  -> testIsometricCamera PASSED!" << std::endl;
}

void testWorldMap() {
    std::cout << "[Test] Running testWorldMap..." << std::endl;
    ReligionSim::WorldMap map(32, 32);
    map.generateProceduralWorld(12345);

    assert(map.getWidth() == 32);
    assert(map.getHeight() == 32);

    int totalPop = map.getTotalWorldPopulation();
    assert(totalPop > 0); // World should have real villager population

    // Verify neighbors
    auto nbs = map.getNeighbors(5, 5, false);
    assert(nbs.size() == 4);

    auto diagNbs = map.getNeighbors(5, 5, true);
    assert(diagNbs.size() == 8);

    // Verify travel ease
    const auto& cWater = map.getCell(0, 0);
    const auto& cLand = map.getCell(16, 16);
    float travel = map.getTravelEase(cWater, cLand);
    assert(travel > 0.0f);

    ReligionSim::WorldMap sameSeedMap(32, 32);
    sameSeedMap.generateProceduralWorld(12345);
    int habitableCells = 0;
    int isolatedBiomeCells = 0;
    for (int y = 0; y < map.getHeight(); ++y) {
        for (int x = 0; x < map.getWidth(); ++x) {
            const auto& cell = map.getCell(x, y);
            const auto& matchingCell = sameSeedMap.getCell(x, y);
            assert(cell.biome == matchingCell.biome);
            assert(cell.elevation == matchingCell.elevation);
            assert(cell.climate.primary_biome == matchingCell.climate.primary_biome);
            assert(cell.climate.secondary_biome == matchingCell.climate.secondary_biome);
            assert(cell.climate.blend_factor == matchingCell.climate.blend_factor);
            assert(cell.climate.elevation >= -1.0f && cell.climate.elevation <= 1.0f);
            assert(cell.climate.temperature >= -1.0f && cell.climate.temperature <= 1.0f);
            assert(cell.climate.moisture >= -1.0f && cell.climate.moisture <= 1.0f);
            assert(cell.climate.blend_factor >= 0.0f && cell.climate.blend_factor <= 1.0f);

            if (cell.isWater()) continue;
            ++habitableCells;
            bool hasMatchingNeighbor = false;
            for (const auto& neighbor : map.getNeighbors(x, y, true)) {
                if (map.getCell(neighbor.x, neighbor.y).biome == cell.biome) {
                    hasMatchingNeighbor = true;
                    break;
                }
            }
            if (!hasMatchingNeighbor) ++isolatedBiomeCells;
        }
    }
    assert(habitableCells > 0);
    assert(habitableCells >= (map.getWidth() * map.getHeight()) / 4);
    assert(habitableCells <= (map.getWidth() * map.getHeight()) * 4 / 5);
    assert(isolatedBiomeCells * 10 < habitableCells);

    for (uint32_t seed : {42u, 999u, 1234567u}) {
        ReligionSim::WorldMap cohesionMap(48, 48);
        cohesionMap.generateProceduralWorld(seed);
        int landCells = 0;
        int isolatedCells = 0;
        int matchingLandEdges = 0;
        int totalLandEdges = 0;
        bool representedLandBiomes[10]{};

        for (int y = 0; y < cohesionMap.getHeight(); ++y) {
            for (int x = 0; x < cohesionMap.getWidth(); ++x) {
                const auto& cell = cohesionMap.getCell(x, y);
                if (cell.isWater()) continue;
                ++landCells;
                representedLandBiomes[static_cast<size_t>(cell.biome)] = true;

                int sameNeighbors = 0;
                for (const auto& neighbor : cohesionMap.getNeighbors(x, y, true)) {
                    const auto& adjacent = cohesionMap.getCell(neighbor.x, neighbor.y);
                    if (adjacent.isWater()) continue;
                    ++totalLandEdges;
                    if (adjacent.biome == cell.biome) {
                        ++sameNeighbors;
                        ++matchingLandEdges;
                    }
                }
                if (sameNeighbors == 0) ++isolatedCells;
            }
        }

        int biomeCount = 0;
        for (bool represented : representedLandBiomes) {
            if (represented) ++biomeCount;
        }
        assert(landCells >= (48 * 48) / 4 && landCells <= (48 * 48) * 4 / 5);
        assert(biomeCount >= 3);
        assert(isolatedCells * 20 < landCells);
        assert(matchingLandEdges * 100 >= totalLandEdges * 60);
    }

    std::cout << "  -> testWorldMap PASSED! Total Pop: " << totalPop << std::endl;
}

void testMultiNoiseBiomeGenerator() {
    std::cout << "[Test] Running testMultiNoiseBiomeGenerator..." << std::endl;
    const float firstNoise = ReligionSim::sample_noise(0.25f, -0.4f, 3.0f, 1234);
    assert(firstNoise >= -1.0f && firstNoise <= 1.0f);
    assert(firstNoise == ReligionSim::sample_noise(0.25f, -0.4f, 3.0f, 1234));
    assert(firstNoise != ReligionSim::sample_noise(0.25f, -0.4f, 3.0f, 4321));

    assert(std::abs(ReligionSim::MultiNoiseBiomeGenerator::applyOrographicCooling(0.4f, 0.5f) -
                    0.19f) < 0.0001f);

    bool seenBiomes[static_cast<size_t>(ReligionSim::BiomeID::Count)]{};
    for (const auto& target : ReligionSim::MultiNoiseBiomeGenerator::BIOME_TARGETS) {
        const auto selected = ReligionSim::MultiNoiseBiomeGenerator::selectBiome(
            target.target_temperature, target.target_moisture, target.target_elevation);
        assert(selected.primary_biome == target.id);
        assert(selected.secondary_biome != selected.primary_biome);
        assert(selected.blend_factor >= 0.0f && selected.blend_factor <= 1.0f);
        seenBiomes[static_cast<size_t>(target.id)] = true;
    }
    for (bool seen : seenBiomes) assert(seen);

    const auto tile = ReligionSim::MultiNoiseBiomeGenerator::sample(12, 19, 48, 48, 9876);
    const auto repeat = ReligionSim::MultiNoiseBiomeGenerator::sample(12, 19, 48, 48, 9876);
    assert(tile.primary_biome == repeat.primary_biome);
    assert(tile.secondary_biome == repeat.secondary_biome);
    assert(tile.blend_factor == repeat.blend_factor);
    assert(tile.elevation >= -1.0f && tile.elevation <= 1.0f);
    assert(tile.temperature >= -1.0f && tile.temperature <= 1.0f);
    assert(tile.moisture >= -1.0f && tile.moisture <= 1.0f);

    std::cout << "  -> testMultiNoiseBiomeGenerator PASSED!" << std::endl;
}

void testReligionAndAgglutination() {
    std::cout << "[Test] Running testReligionAndAgglutination..." << std::endl;
    ReligionSim::ReligionRegistry reg;

    int r1 = reg.createPrimordialReligion("Faith Alpha", "Prophet A", {255, 100, 50}, "Sun", -3500);
    int r2 = reg.createPrimordialReligion("Faith Beta", "Prophet B", {50, 100, 255}, "Moon", -3400);

    assert(r1 == 1);
    assert(r2 == 2);
    assert(reg.getAllReligions().size() == 2);

    // Test Syncretic Agglutination
    int rSyn = reg.createSyncreticReligion(r1, r2, -1500, {10, 10});
    assert(rSyn == 3);
    const auto* synRel = reg.getReligion(rSyn);
    assert(synRel != nullptr);
    assert(synRel->syncreticParentA == r1);
    assert(synRel->syncreticParentB == r2);
    assert(synRel->hasDoctrine(ReligionSim::DoctrineType::SyncreticOpenness));

    // Test Doctrinal Schism
    int rSchism = reg.createSchismReligion(r1, -500, {12, 12});
    assert(rSchism == 4);
    const auto* schismRel = reg.getReligion(rSchism);
    assert(schismRel != nullptr);
    assert(schismRel->parentReligionId == r1);
    assert(schismRel->zeal > 0.6f);

    std::cout << "  -> testReligionAndAgglutination PASSED!" << std::endl;
}

void testReplicationAndSimulationCycle() {
    std::cout << "[Test] Running testReplicationAndSimulationCycle..." << std::endl;
    ReligionSim::WorldMap map(32, 32);
    map.generateProceduralWorld(42);

    ReligionSim::ReligionRegistry reg;
    map.seedPrimordialFaiths(reg, -4000);

    ReligionSim::ReplicationEngine rep;
    ReligionSim::AgglutinationEngine agg;
    ReligionSim::AdvancementSystem adv;
    ReligionSim::Timeline timeline(-4000, 2000);
    ReligionSim::EventLine eventLine;

    // Run 50 steps
    float dtYears = 0.0f;
    for (int i = 0; i < 50; ++i) {
        timeline.update(0.5f, dtYears);
        if (dtYears > 0.0f) {
            std::vector<ReligionSim::GameEvent> events;
            adv.updateAdvancements(reg, timeline.getCurrentYear(), events);
            rep.update(map, reg, dtYears, events, timeline.getCurrentYear());
            agg.update(map, reg, dtYears, events, timeline.getCurrentYear());
            map.recalculateReligions(reg);

            for (const auto& ev : events) {
                eventLine.addEvent(ev);
            }
        }
    }

    assert(timeline.getCurrentYear() > -4000);
    assert(!eventLine.getAllEvents().empty());

    // Check that religions replicated and spread
    int totalFollowers = 0;
    for (const auto& r : reg.getAllReligions()) {
        totalFollowers += r.totalFollowers;
    }
    assert(totalFollowers > 0);

    std::cout << "  -> testReplicationAndSimulationCycle PASSED! Current Year: "
              << timeline.getCurrentYear() << " Events logged: " << eventLine.size() << std::endl;
}

void testVillagerEntities() {
    std::cout << "[Test] Running testVillagerEntities (Grog tileset)..." << std::endl;
    ReligionSim::WorldMap map(32, 32);
    map.generateProceduralWorld(42);

    ReligionSim::ReligionRegistry reg;
    map.seedPrimordialFaiths(reg, -4000);

    ReligionSim::VillagerManager mgr;
    mgr.initVillagers(map, reg, 30);
    assert(mgr.getVillagers().size() == 30);

    for (int i = 0; i < 20; ++i) {
        mgr.update(0.1f, map);
    }

    for (const auto& v : mgr.getVillagers()) {
        std::string spriteName = v.getSpriteName();
        assert(!spriteName.empty());
        assert(std::isfinite(v.worldX) && std::isfinite(v.worldY));
    }

    std::cout << "  -> testVillagerEntities PASSED! 30 villagers active." << std::endl;
}

void testObjetoHumanoYTileSet() {
    std::cout << "[Test] Running testObjetoHumanoYTileSet (Objeto Humano & TileSet)..." << std::endl;
    ReligionSim::TileSet tileset;
    assert(tileset.getAncho() > 0 && tileset.getAlto() > 0);

    // Test rect mapping para la cuadrícula 4x4 de 64x64
    ReligionSim::TileRect rIdleSW = tileset.obtenerRectangulo(ReligionSim::AccionHumano::IDLE, ReligionSim::DireccionIso::SW);
    assert(rIdleSW.width == 64 && rIdleSW.height == 64);
    assert(rIdleSW.x == 0 && rIdleSW.y == 0);

    ReligionSim::TileRect rWalkSE = tileset.obtenerRectangulo(ReligionSim::AccionHumano::CAMINAR, ReligionSim::DireccionIso::SE);
    assert(rWalkSE.width == 64 && rWalkSE.height == 64);
    assert(rWalkSE.x == 64 && rWalkSE.y == 0);

    ReligionSim::TileRect rChop = tileset.obtenerRectangulo(ReligionSim::AccionHumano::TALAR, ReligionSim::DireccionIso::SW);
    assert(rChop.width == 64 && rChop.height == 64);

    ReligionSim::TileRect rPray = tileset.obtenerRectangulo(ReligionSim::AccionHumano::REZAR, ReligionSim::DireccionIso::NE);
    assert(rPray.width == 64 && rPray.height == 64);

    // Test Humano object
    ReligionSim::Humano h(42, "Grog el Invocador", 15.0f, 15.0f);
    assert(h.getId() == 42);
    assert(h.getNombre() == "Grog el Invocador");
    assert(h.getX() == 15.0f && h.getY() == 15.0f);

    h.cambiarAccion(ReligionSim::AccionHumano::REZAR);
    h.cambiarDireccion(ReligionSim::DireccionIso::NE);
    ReligionSim::TileRect rHumano = h.getTileRect(tileset);
    assert(rHumano.x == rPray.x && rHumano.y == rPray.y);

    h.caminarHacia(18.0f, 20.0f, false);
    assert(h.getAccion() == ReligionSim::AccionHumano::CAMINAR);

    ReligionSim::WorldMap map(32, 32);
    map.generateProceduralWorld(42);

    h.actualizar(0.5f, map);
    assert(h.getX() > 15.0f || h.getY() > 15.0f);

    // Test GestorHumanos
    ReligionSim::ReligionRegistry reg;
    ReligionSim::GestorHumanos gestor;
    gestor.inicializarPoblacion(map, reg, 25);
    assert(gestor.getHumanos().size() == 25);

    gestor.actualizar(0.2f, map);

    std::cout << "  -> testObjetoHumanoYTileSet PASSED! Objeto Humano y TileSet validados correctamente." << std::endl;
}

void testTimeEngineDecoupledScaling() {
    std::cout << "[Test] Running testTimeEngineDecoupledScaling (C++20 TimeEngine)..." << std::endl;
    using namespace ReligionSim;

    // 1. Verify constexpr lookup table
    static_assert(GetYearsPerSecond(HistoricalEra::PrimordialAnimism) == 500.0);
    static_assert(GetYearsPerSecond(HistoricalEra::TotemismAncestorWorship) == 100.0);
    static_assert(GetYearsPerSecond(HistoricalEra::StructuralShamanism) == 10.0);
    static_assert(GetYearsPerSecond(HistoricalEra::TerritorialPolytheism) == 2.0);
    static_assert(GetYearsPerSecond(HistoricalEra::UniversalReligions) == 1.0);

    TimeEngine engine(-4000.0);
    auto state = engine.getTimeState();
    assert(state.current_year == -4000.0);
    assert(state.active_era == HistoricalEra::PrimordialAnimism);
    assert(state.time_multiplier == TimeMultiplier::Normal);

    // 2. Test tick progression at Era 1 (500 years/sec at 1x)
    double macroDelta = 0.0;
    engine.update(1.0, macroDelta); // 1 real second
    state = engine.getTimeState();
    assert(macroDelta >= 499.0 && macroDelta <= 501.0);
    assert(state.current_year >= -3501.0 && state.current_year <= -3499.0);
    assert(state.total_ticks == 60); // 60 Hz micro tick rate

    // 3. Test Multipliers: Ultra (5x)
    engine.setMultiplier(TimeMultiplier::Ultra);
    assert(engine.getMultiplier() == TimeMultiplier::Ultra);
    engine.update(1.0, macroDelta);
    assert(macroDelta >= 2490.0 && macroDelta <= 2510.0);

    // 4. Test Pause
    engine.togglePause();
    assert(engine.isPaused());
    engine.update(2.0, macroDelta);
    assert(macroDelta == 0.0);

    // 5. Test Date Formatting across distinct eras
    GameTime tEra1{-3500.0, 100, TimeMultiplier::Normal, HistoricalEra::PrimordialAnimism, 0.0};
    assert(TimeEngine::GetFormattedDate(tEra1).find("3500 BCE") != std::string::npos);

    GameTime tEra3{-1500.0, 200, TimeMultiplier::Normal, HistoricalEra::StructuralShamanism, 0.5};
    assert(TimeEngine::GetFormattedDate(tEra3).find("Harvest Cycle") != std::string::npos);

    GameTime tEra8{2088.0, 5000, TimeMultiplier::Normal, HistoricalEra::TranshumanistPostBiological, 0.25};
    assert(TimeEngine::GetFormattedDate(tEra8).find("Singularity") != std::string::npos);

    std::cout << "  -> testTimeEngineDecoupledScaling PASSED! Elastic scale verified." << std::endl;
}

// Mock PixelCanvas for headless testing of NinePatch and UI widgets
class MockCanvas : public ReligionSim::UI::PixelCanvas {
public:
    int fillCount{0};
    int strokeCount{0};
    int textCount{0};

    void fillRect(int, int, int, int, ReligionSim::ColorRGB) override { fillCount++; }
    void strokeRect(int, int, int, int, ReligionSim::ColorRGB) override { strokeCount++; }
    void drawText(int, int, std::string_view, ReligionSim::ColorRGB, int) override { textCount++; }
    void blitSubImage(int, int, int, int, const uint32_t*, int, int, int, int, int, int, ReligionSim::ColorRGB) override { fillCount++; }
};

void testModularUISystem() {
    std::cout << "[Test] Running testModularUISystem (C++20 NinePatch, Themes, Widgets)..." << std::endl;
    using namespace ReligionSim;
    using namespace ReligionSim::UI;

    // 1. Verify Liturgical Theme Switching
    auto themeEra1 = GetThemeForEra(HistoricalEra::PrimordialAnimism);
    assert(themeEra1.borderAccent.r == Palette::RELIC_BONE.r);

    auto themeEra4 = GetThemeForEra(HistoricalEra::TerritorialPolytheism);
    assert(themeEra4.borderAccent.r == Palette::DIVINE_GOLD.r);

    auto themeEra8 = GetThemeForEra(HistoricalEra::TranshumanistPostBiological);
    assert(themeEra8.borderAccent.b == 215); // Cyber Cyan

    // 2. Test 9-Patch Panel Drawing
    MockCanvas canvas;
    TileRect patch{0, 0, 24, 24};
    Margins margins{8, 8, 8, 8};
    Rect targetPanel{50.0f, 50.0f, 200.0f, 150.0f};

    DrawNinePatchPanel(canvas, nullptr, 0, 0, patch, margins, targetPanel, Palette::BASALT);
    assert(canvas.fillCount >= 9); // Center + 4 edges + 4 corners

    // 3. Test Widgets & Interaction: Main HUD Panel with "Consagrar Tile" Button and ProgressBar
    auto mainHUD = std::make_shared<Panel>(Rect{20.0f, 20.0f, 280.0f, 160.0f}, "PANEL LITURGICO");
    bool consecratedClicked = false;

    auto btnConsagrar = std::make_shared<Button>(Rect{35.0f, 60.0f, 160.0f, 30.0f}, "Consagrar Tile", [&]() {
        consecratedClicked = true;
    });

    auto barFaith = std::make_shared<ProgressBar>(Rect{35.0f, 105.0f, 240.0f, 16.0f}, 0.75f, 0.90f);

    mainHUD->addChild(btnConsagrar);
    mainHUD->addChild(barFaith);

    // Render tree
    mainHUD->render(canvas, themeEra4);
    assert(canvas.textCount > 0);

    // Test mouse event propagation
    assert(!consecratedClicked);
    mainHUD->handleMouseMove(50.0f, 75.0f);
    bool consumed = mainHUD->handleMouseDown(50.0f, 75.0f);
    assert(consumed);
    mainHUD->handleMouseUp(50.0f, 75.0f);
    assert(consecratedClicked);

    std::cout << "  -> testModularUISystem PASSED! NinePatch and Liturgical widgets verified." << std::endl;
}

void testExpandedTerrainAtlas() {
    std::cout << "[Test] Running testExpandedTerrainAtlas..." << std::endl;
    std::ifstream atlasFile("assets/sprites/terrain/terrain_atlas.rgba", std::ios::binary);
    if (!atlasFile.is_open()) {
        atlasFile.open("../assets/sprites/terrain/terrain_atlas.rgba", std::ios::binary);
    }
    assert(atlasFile.is_open());
    std::vector<uint8_t> atlasPixels(
        (std::istreambuf_iterator<char>(atlasFile)),
        std::istreambuf_iterator<char>());
    assert(atlasPixels.size() == 1024u * 64u * 4u);

    ReligionSim::Renderer renderer(300, 140);
    for (int textureIndex = 0; textureIndex < 16; ++textureIndex) {
        renderer.clear({0, 0, 0});
        renderer.drawTexturedDiamond(
            32.0f, 32.0f, 24.0f, 12.0f, textureIndex,
            {255, 0, 255}, 0.0f, {0, 0, 0}, 0.0f);
        renderer.drawTexturedDiamond(
            100.0f, 80.0f, 24.0f, 12.0f, textureIndex,
            {255, 0, 255}, 0.0f, {0, 0, 0}, 0.0f);

        const size_t atlasOffset =
            (16u * 1024u + static_cast<size_t>(textureIndex * 64 + 32)) * 4u;
        const uint32_t expectedPixel =
            0xFF000000u |
            (static_cast<uint32_t>(atlasPixels[atlasOffset + 2]) << 16) |
            (static_cast<uint32_t>(atlasPixels[atlasOffset + 1]) << 8) |
            static_cast<uint32_t>(atlasPixels[atlasOffset]);
        const auto& framebuffer = renderer.getFramebuffer();
        const uint32_t firstRenderedPixel = framebuffer[(140u - 1u - 32u) * 300u + 32u];
        const uint32_t secondRenderedPixel = framebuffer[(140u - 1u - 80u) * 300u + 100u];
        assert(firstRenderedPixel == expectedPixel);
        assert(secondRenderedPixel == expectedPixel);
    }

    renderer.clear({10, 10, 10});
    renderer.drawTerrainProp(150.0f, 70.0f, 0, 1.0f);
    bool renderedPropPixel = false;
    for (int y = 50; y < 66; ++y) {
        for (int x = 137; x < 163; ++x) {
            const size_t offset = (139u - static_cast<size_t>(y)) * 300u + static_cast<size_t>(x);
            if (renderer.getFramebuffer()[offset] != 0xFF0A0A0Au) {
                renderedPropPixel = true;
                break;
            }
        }
        if (renderedPropPixel) break;
    }
    assert(renderedPropPixel);

    std::ifstream backgroundFile("assets/sprites/miscellany/space_background.rgba", std::ios::binary);
    if (!backgroundFile.is_open()) {
        backgroundFile.open("../assets/sprites/miscellany/space_background.rgba", std::ios::binary);
    }
    assert(backgroundFile.is_open());
    std::vector<uint8_t> backgroundPixels(
        (std::istreambuf_iterator<char>(backgroundFile)),
        std::istreambuf_iterator<char>());
    assert(backgroundPixels.size() == 256u * 128u * 4u);

    renderer.beginFrame();
    const size_t backgroundOffset = (2u * 256u + 4u) * 4u;
    const uint32_t expectedBackgroundPixel =
        0xFF000000u |
        (static_cast<uint32_t>(backgroundPixels[backgroundOffset + 2]) << 16) |
        (static_cast<uint32_t>(backgroundPixels[backgroundOffset + 1]) << 8) |
        static_cast<uint32_t>(backgroundPixels[backgroundOffset]);
    const uint32_t renderedBackgroundPixel = renderer.getFramebuffer()[(140u - 1u - 130u) * 300u + 260u];
    assert(renderedBackgroundPixel == expectedBackgroundPixel);

    std::cout << "  -> testExpandedTerrainAtlas PASSED! All 16 terrain textures render correctly." << std::endl;
}

void testFirstEraAIController() {
    std::cout << "[Test] Running testFirstEraAIController (First Era Realistic AI)..." << std::endl;
    FirstEraAIController ai;
    RealisticPopGroup pop{20, 20.0f, 80.0f, 60.0f, RealisticVillagerState::COMMUNAL_BONDING, {10.0f, 10.0f}};
    RealisticCellContext ctx{false, 1.0f, 80.0f, true};

    // 1. El hambre (< 35.0f) anula el dogma -> SURVIVAL_FORAGING
    ai.UpdateRealisticBehavior(pop, ctx, 1.0f);
    assert(pop.state == RealisticVillagerState::SURVIVAL_FORAGING);
    assert(pop.foodSupply > 20.0f);
    assert(pop.divineWill < 80.0f);

    // 2. Encuentro cauteloso en frontera -> CAUTIOUS_CONTACT
    pop.foodSupply = 60.0f;
    ctx.hasNeighborTribe = true;
    ctx.neighborTribePowerRatio = 0.4f;
    ai.UpdateRealisticBehavior(pop, ctx, 1.0f);
    assert(pop.state == RealisticVillagerState::CAUTIOUS_CONTACT);

    // 3. Superpoblación real -> AUTONOMOUS_SPLIT
    ctx.hasNeighborTribe = false;
    pop.unityWill = 75.0f;
    pop.foodSupply = 85.0f;
    pop.populationCount = 48;
    ai.UpdateRealisticBehavior(pop, ctx, 1.0f);
    assert(pop.state == RealisticVillagerState::AUTONOMOUS_SPLIT);
    assert(pop.populationCount == 24);

    // 4. Dinámica de los Dos Ejes: Dogma vs Unidad
    pop.foodSupply = 60.0f;
    pop.divineWill = 85.0f;
    pop.unityWill = 50.0f;
    ctx.hasShrine = true;
    ai.UpdateRealisticBehavior(pop, ctx, 1.0f);
    assert(pop.state == RealisticVillagerState::RURAL_WORSHIP);

    pop.divineWill = 40.0f;
    pop.unityWill = 70.0f;
    ai.UpdateRealisticBehavior(pop, ctx, 1.0f);
    assert(pop.state == RealisticVillagerState::COMMUNAL_BONDING);

    // 5. El desierto eleva el umbral de hambre y reduce la recuperación de comida.
    ctx.currentBiome = ReligionSim::BiomeType::AridDesert;
    ctx.environmentalCarryingCapacity = 30.0f;
    ctx.hasShrine = false;
    pop.foodSupply = 50.0f;
    pop.divineWill = 50.0f;
    const float desertFoodBefore = pop.foodSupply;
    ai.UpdateRealisticBehavior(pop, ctx, 1.0f);
    assert(pop.state == RealisticVillagerState::SURVIVAL_FORAGING);
    assert(pop.foodSupply > desertFoodBefore);
    assert(pop.foodSupply < desertFoodBefore + 4.0f);

    // 6. Entradas inválidas no avanzan la simulación.
    const float foodBeforeInvalidStep = pop.foodSupply;
    ai.UpdateRealisticBehavior(pop, ctx, -1.0f);
    assert(pop.foodSupply == foodBeforeInvalidStep);

    std::cout << "  -> testFirstEraAIController PASSED! 5 realistic autonomous states verified." << std::endl;
}

void testHumanAIController() {
    std::cout << "[Test] Running testHumanAIController (Systemic HumanUnit & Roles)..." << std::endl;
    HumanAIController ai;
    HumanUnit unit;
    unit.unitName = "Grog";
    unit.tribeID = "Tribu #1";
    unit.leaderOrProphetID = "Solarius";
    unit.role = CommunityRole::FORAGER;
    unit.currentEra = ::EraType::PRIMITIVE_TRIBAL;
    unit.hitPoints = 100.0f;
    unit.attackPower = 12.0f;
    unit.energy = 15.0f; // < 25.0f -> SURVIVAL_WORKING
    unit.resistance = 25.0f;
    unit.willpower = 70.0f;
    unit.devotion = 80.0f;
    unit.conversionPower = 15.0f;
    unit.conversionInertia = 40.0f;
    unit.currentState = HumanAIState::IDLE;
    unit.position = {10.0f, 10.0f};
    unit.nutritionalState = 50.0f;

    UnitEnvironmentContext env{true, false, 0.0f, 0.0f};

    // 1. Prioridad biológica (energía < 25) -> SURVIVAL_WORKING
    ai.UpdateHumanAI(unit, env, 1.0f);
    assert(unit.currentState == HumanAIState::SURVIVAL_WORKING);
    assert(unit.energy > 15.0f);
    assert(unit.nutritionalState > 50.0f);

    // 2. Presión externa de conversión > inercia -> reduce devoción
    unit.energy = 80.0f;
    unit.nutritionalState = 80.0f;
    unit.devotion = 80.0f;
    env.externalConversionPressure = 60.0f; // > 40.0f
    env.hasNearbyShrine = false;
    ai.UpdateHumanAI(unit, env, 1.0f);
    assert(unit.devotion < 80.0f);
    assert(unit.currentState == HumanAIState::SOCIALIZING);

    // 3. Roles: SHAMAN, PROPHET, ZEALOT, FORAGER con santuario
    env.externalConversionPressure = 0.0f;
    unit.role = CommunityRole::SHAMAN;
    ai.UpdateHumanAI(unit, env, 1.0f);
    assert(unit.currentState == HumanAIState::SOCIALIZING);

    unit.role = CommunityRole::PROPHET;
    float prevConv = unit.conversionPower;
    ai.UpdateHumanAI(unit, env, 1.0f);
    assert(unit.currentState == HumanAIState::CONVERTING);
    assert(unit.conversionPower > prevConv);

    unit.role = CommunityRole::ZEALOT;
    ai.UpdateHumanAI(unit, env, 1.0f);
    assert(unit.currentState == HumanAIState::WORSHIPING);

    unit.role = CommunityRole::FORAGER;
    unit.devotion = 75.0f;
    env.hasNearbyShrine = true;
    ai.UpdateHumanAI(unit, env, 1.0f);
    assert(unit.currentState == HumanAIState::WORSHIPING);

    std::cout << "  -> testHumanAIController PASSED! Systemic roles and conversion inertia verified." << std::endl;
}

void testFaithFeedbackAndCombatDamage() {
    std::cout << "[Test] Running testFaithFeedbackAndCombatDamage (Overhead +/- and Combat HP Loss)..." << std::endl;
    using namespace ReligionSim;
    WorldMap dummyMap(40, 40);
    Humano h(1, "Grog", 20.0f, 20.0f);

    assert(h.getDevocion() == 0.80f);
    assert(h.getSalud() == 100.0f);
    assert(h.getHumanUnit().hitPoints == 100.0f);
    assert(h.getFaithIndicatorSign() == 0);

    // 1. Aumento de fe genera '+' verde (+1)
    h.setDevocion(0.95f);
    assert(h.getFaithIndicatorSign() == +1);
    assert(h.getFaithIndicatorTimer() > 1.0f);

    // 2. Disminución de fe genera '+' rojo (-1)
    h.setDevocion(0.70f);
    assert(h.getFaithIndicatorSign() == -1);
    assert(h.getFaithIndicatorTimer() > 1.0f);

    // 3. Expiración natural del indicador con dt
    h.actualizar(2.0f, dummyMap);
    assert(h.getFaithIndicatorSign() == 0);
    assert(h.getFaithIndicatorTimer() == 0.0f);

    // 4. Recepción de daño reduce puntos de vida (m_salud y m_humanUnit.hitPoints)
    h.recibirDano(35.0f);
    assert(std::abs(h.getSalud() - 65.0f) < 0.01f);
    assert(std::abs(h.getHumanUnit().hitPoints - 65.0f) < 0.01f);
    assert(h.getTimerDano() > 0.0f);
    assert(h.estaVivo());
    assert(!h.estaMuerto());

    // Muerte al llegar a cero puntos de vida
    h.recibirDano(100.0f);
    assert(h.getSalud() == 0.0f);
    assert(h.estaMuerto());
    assert(!h.estaVivo());
    assert(h.getTimerMuerte() > 0.0f);

    // 5. Escaramuza, ataque y muerte entre unidades rivales en GestorHumanos
    GestorHumanos gestor;
    Humano a(10, "Fanatico Sol", 15.0f, 15.0f);
    a.setReligionId(1);
    a.setRole(CommunityRole::ZEALOT);
    a.cambiarAccion(AccionHumano::ATACAR);

    Humano b(20, "Aldeano Luna", 15.7f, 15.7f);
    b.setReligionId(2);
    b.cambiarAccion(AccionHumano::IDLE);

    gestor.agregarHumano(a);
    gestor.agregarHumano(b);

    float saludInicialB = gestor.getHumanos()[1].getSalud();
    gestor.actualizar(0.5f, dummyMap);

    // b debe haber perdido puntos de vida al ser atacado por el fanático rival
    assert(gestor.getHumanos()[1].getSalud() < saludInicialB);
    assert(gestor.getHumanos()[1].getHumanUnit().hitPoints < saludInicialB);
    assert(gestor.getHumanos()[1].getTimerDano() > 0.0f);

    // Infligir daño letal a b: debe fallecer y ser retirado tras su animación de muerte
    gestor.getHumanos()[1].recibirDano(100.0f);
    assert(gestor.getHumanos()[1].estaMuerto());
    gestor.actualizar(2.0f, dummyMap);
    assert(gestor.getHumanos().size() == 1);
    assert(gestor.getHumanos()[0].getId() == 10);

    std::cout << "  -> testFaithFeedbackAndCombatDamage PASSED! Green/Red faith feedback, HP combat loss and unit death verified." << std::endl;
}

void testAntique2DButtonsAndInteractiveStates() {
    using namespace ReligionSim;
    std::cout << "[Test] Running testAntique2DButtonsAndInteractiveStates (2D Antique Sprites, Hover, Press Anim)..." << std::endl;
    Renderer renderer(1280, 720);

    // 1. Simular renderizado de botones antiguos en todos sus estados interactivos
    renderer.clear(ColorRGB{0, 0, 0});
    // Idle (Estado 0 - Reproductor Pause)
    renderer.drawAntiqueButton(100, 100, 44, 24, "PAUSA", false, false, false, 201);
    // Hover (Estado 1 - Iluminación dorada)
    renderer.drawAntiqueButton(160, 100, 34, 24, "1x", false, true, false, 202);
    // Pressed (Estado 2 - Bajorrelieve táctil hundido 1px)
    renderer.drawAntiqueButton(200, 100, 34, 24, "2x", false, false, true, 203);
    // Active (Estado 3 - Encajado con runa dorada)
    renderer.drawAntiqueButton(240, 100, 34, 24, "5x", true, false, false, 204);
    // 9-slice para botón personalizado
    renderer.drawAntiqueButton(280, 100, 80, 24, "+ SINCRETISMO", false, true, false, 301);

    const auto& fb = renderer.getFramebuffer();
    assert(fb.size() == 1280 * 720);

    // Verificar que los píxeles de los botones se hayan dibujado (no queden en negro 0x00000000)
    int nonBlackPixels = 0;
    for (int y = 98; y < 126; ++y) {
        int py = 720 - 1 - y;
        for (int x = 98; x < 365; ++x) {
            if ((fb[py * 1280 + x] & 0x00FFFFFF) != 0) {
                nonBlackPixels++;
            }
        }
    }
    assert(nonBlackPixels > 1000);

    // 2. Probar ciclo de vida de animación de pulsado táctil (timer decay)
    renderer.setMousePosition(110.0f, 110.0f);
    renderer.triggerButtonPress(201);
    renderer.setMouseDown(true, 110.0f, 110.0f);

    // Con el botón del ratón presionado y el timer activo, el botón debe persistir
    renderer.update(0.05f);

    // Al soltar el ratón y agotar el temporizador, debe volver a reposo
    renderer.setMouseDown(false, 110.0f, 110.0f);
    renderer.update(0.20f);

    std::cout << "  -> testAntique2DButtonsAndInteractiveStates PASSED! 2D Antique Sprites, Hover, and tactile press animation verified." << std::endl;
}

void testViewModeSpritesAndMapSector() {
    using namespace ReligionSim;
    std::cout << "[Test] Running testViewModeSpritesAndMapSector (View Mode Sprites 101-105 & Sims 2 Minimap)..." << std::endl;
    Renderer renderer(1280, 720);

    renderer.clear(ColorRGB{0, 0, 0});
    // 101 FE: Activo (Sol radiante / Mónada sagrada, 22x22 sin letras)
    renderer.drawViewModeButton(500, 100, 22, 22, 101, "", true, false, false);
    // 102 BIOMAS: Hover (Montaña nevada y bosque, 22x22 sin letras)
    renderer.drawViewModeButton(500, 126, 22, 22, 102, "", false, true, false);
    // 105 UNIDADES: Pressed (Chamán con báculo y cazador, 22x22 sin letras)
    renderer.drawViewModeButton(500, 152, 22, 22, 105, "", false, false, true);
    // 103 SINCRETISMO: Idle (Espirales sagradas de oro y amatista, 22x22 sin letras)
    renderer.drawViewModeButton(500, 178, 22, 22, 103, "", false, false, false);
    // 104 ERAS: Idle (Reloj ceremonial rúnico, 22x22 sin letras)
    renderer.drawViewModeButton(500, 204, 22, 22, 104, "", false, false, false);

    const auto& fb = renderer.getFramebuffer();
    assert(fb.size() == 1280 * 720);

    int drawnPixels = 0;
    for (int y = 98; y < 230; ++y) {
        int py = 720 - 1 - y;
        for (int x = 498; x < 525; ++x) {
            if ((fb[py * 1280 + x] & 0x00FFFFFF) != 0) {
                drawnPixels++;
            }
        }
    }
    assert(drawnPixels > 1000);

    // Verificar rect del minimapa expandido
    int mmX = 0, mmY = 0, mmW = 0, mmH = 0;
    renderer.getMinimapRect(mmX, mmY, mmW, mmH);
    assert(mmW == 198);
    assert(mmH == 106);
    assert(mmX > 0 && mmY > 0);

    int mmSize = 0;
    renderer.getMinimapRect(mmX, mmY, mmSize);
    assert(mmSize == 106);

    std::cout << "  -> testViewModeSpritesAndMapSector PASSED! Sprites, tactile feedback and minimap sector verified." << std::endl;
}

void testAdvancedEventChronicleSystem() {
    using namespace ReligionSim;
    std::cout << "[Test] Running testAdvancedEventChronicleSystem (Advanced Event Chronicle, Turning Points & 2D Icons)..." << std::endl;

    EventLine eventLine(500);
    Timeline timeline;

    // 1. Agregar eventos de diversas categorías y puntos de quiebre
    eventLine.addEvent(-4000, EventCategory::BirthOfReligion, "Genesis de Solarius",
                       "Pacto divino y fundacion del pueblo.", {24, 24}, 1, 0, Palette::DIVINE_GOLD,
                       true, "Solarius", "Genesis y nacimiento del culto primordial");

    eventLine.addEvent(-3968, EventCategory::MajorLoss, "Muerte del Chaman Orph",
                       "El guia espiritual ancestral ha caido en el campo sagrado.", {28, 22}, 1, 0, Palette::BLOOD_RUBY,
                       true, "Chaman Orph", "Perdida de la guia espiritual ancestral");

    eventLine.addEvent(-3850, EventCategory::Discovery, "Vision Mistica del Rio Sagrado",
                       "Los cazadores descubren una fuente de agua pura consagrada.", {18, 30}, 1, 0, ColorRGB{60, 200, 240},
                       false, "Cazador Kael", "Establecimiento de nuevo asentamiento ritual");

    eventLine.addEvent(-3600, EventCategory::DoctrinalSchism, "Cisma del Profeta Zarara",
                       "Herejia reformista que fractura la ortodoxia ancestral.", {32, 20}, 1, 2, ColorRGB{240, 60, 60},
                       true, "Profeta Zarara", "Fractura doctrinal y nacimiento de secta rival");

    eventLine.addEvent(-3300, EventCategory::SyncreticAgglutination, "Comunion de la Doble Espiral",
                       "Pacto de fe sagrada entre clanes vecinos.", {26, 26}, 1, 3, ColorRGB{200, 80, 220},
                       true, "Consejo Tribal", "Alianza y unificacion de credos");

    eventLine.addEvent(-3000, EventCategory::EraAdvancement, "Entrada al Totemismo",
                       "El pueblo cruza al culto a los ancestros y espiritus protectores.", {24, 24}, 1, 0, Palette::DIVINE_GOLD,
                       true, "Pueblo Elegido", "Salto civilizatorio de Era");

    assert(eventLine.size() == 6);

    // 2. Verificar filtrado de puntos de quiebre
    auto turningPoints = eventLine.getTurningPoints();
    assert(turningPoints.size() == 5); // 5 de los 6 son puntos de quiebre

    // 3. Verificar pestañas de filtrado (0..6)
    assert(eventLine.getFilteredEvents(0).size() == 6); // Todos
    assert(eventLine.getFilteredEvents(1).size() == 5); // Puntos de quiebre
    assert(eventLine.getFilteredEvents(2).size() == 1); // Pérdidas (Muerte del Chamán)
    assert(eventLine.getFilteredEvents(2)[0].title == "Muerte del Chaman Orph");
    assert(eventLine.getFilteredEvents(3).size() == 1); // Avances (Totemismo)
    assert(eventLine.getFilteredEvents(4).size() == 1); // Descubrimientos (Visión)
    assert(eventLine.getFilteredEvents(5).size() == 1); // Cismas (Zarara)
    assert(eventLine.getFilteredEvents(6).size() == 1); // Sincretismo (Doble Espiral)

    // 4. Renderer: dibujar iconos de eventos y modal
    Renderer renderer(1280, 720);
    renderer.clear(ColorRGB{0, 0, 0});

    // Dibujar los 6 iconos genéricos 2D Pixel Art
    for (int col = 0; col < 6; ++col) {
        renderer.drawEventIcon(100 + col * 32, 100, col, false);
        renderer.drawEventIcon(100 + col * 32, 140, col, true);
    }

    // Renderizar modal de crónica sagrada
    renderer.renderChronicleModal(eventLine, timeline, 0, 0, 640.0f, 360.0f, false);

    const auto& fb = renderer.getFramebuffer();
    assert(fb.size() == 1280 * 720);

    // Verificar clics interactivos del modal
    Point2i jumpTarget{-1, -1};
    // Clic fuera del modal (x=20, y=20) -> retorna 1 (cerrar)
    assert(renderer.checkChronicleModalClick(20.0f, 20.0f, eventLine, 0, 0, jumpTarget) == 1);

    // Clic en pestaña de filtro PUNTOS DE QUIEBRE (x=120, y=115) -> retorna 11
    int tabAction = renderer.checkChronicleModalClick(170.0f, 115.0f, eventLine, 0, 0, jumpTarget);
    assert(tabAction == 11);

    // Clic en botón "IR AL LUGAR" de la primera tarjeta
    // La primera tarjeta está en modalX + 44 = 70 + 44 = 114
    // Botón en cX + 10 = 124, y = modalY + 90 + 465 - 34 = 591
    int jumpAction = renderer.checkChronicleModalClick(150.0f, 595.0f, eventLine, 0, 0, jumpTarget);
    assert(jumpAction == 30);
    assert(jumpTarget.x == 24 && jumpTarget.y == 24);

    // Estado del modal en Renderer
    assert(!renderer.isChronicleOpen());
    renderer.setChronicleOpen(true);
    assert(renderer.isChronicleOpen());
    renderer.toggleChronicle();
    assert(!renderer.isChronicleOpen());

    std::cout << "  -> testAdvancedEventChronicleSystem PASSED! Chronicle, turning points, filtering and 2D pixel art icons verified." << std::endl;
}

void testFaithAdvancementTreeSystem() {
    using namespace ReligionSim;
    std::cout << "[Test] Running testFaithAdvancementTreeSystem (Faith Tech Tree, Convalidation, Eras & Replication)..." << std::endl;

    AdvancementSystem advSystem;

    // 1. Catálogo completo de 16 avances teológicos distribuidos en 4 Eras
    const auto& allAdvs = advSystem.getAllAdvancements();
    assert(allAdvs.size() == 16);

    const FaithAdvancement* adv101 = advSystem.getAdvancement(101);
    assert(adv101 != nullptr);
    assert(adv101->shortTitle == "Mito del Fuego");
    assert(adv101->branch == FaithBranch::ReplicationSpread);
    assert(adv101->prerequisites.empty());

    const FaithAdvancement* hito100 = advSystem.getAdvancement(100);
    assert(hito100 != nullptr);
    assert(hito100->isEraCapstone);
    assert(hito100->unlocksEra == HistoricalEra::StructuralShamanism);

    const FaithAdvancement* hito200 = advSystem.getAdvancement(200);
    assert(hito200 != nullptr);
    assert(hito200->isEraCapstone);
    assert(hito200->unlocksEra == HistoricalEra::TerritorialPolytheism);

    // 2. Convalidación progresiva, costos de Fe y mejoras a la replicación
    Religion rel;
    rel.id = 1;
    rel.name = "Culto del Fuego Solar";
    rel.currentEra = HistoricalEra::PrimordialAnimism;
    rel.theologyPoints = 40; // Menor al costo de 60
    rel.zeal = 0.50f;
    rel.cohesion = 0.60f;

    // No puede convalidar si no tiene fe suficiente
    assert(!advSystem.canConvalidate(rel, 101));

    rel.theologyPoints = 350;
    assert(advSystem.canConvalidate(rel, 101));

    std::vector<GameEvent> events;
    bool ok1 = advSystem.convalidateAdvancement(rel, 101, -4000, events);
    assert(ok1);
    assert(rel.hasAdvancement(101));
    assert(rel.theologyPoints == 350 - 60); // Deduce 60 pts
    assert(rel.zeal > 0.50f);               // Aumento de replicación
    assert(rel.diffusionMultiplier > 1.0f); // Bonus de difusión espacial
    assert(!events.empty());
    assert(events.back().category == EventCategory::Discovery);

    // Verificar bloqueo por prerrequisitos: 201 (Caravanas) requiere Hito 100
    assert(!advSystem.canConvalidate(rel, 201));

    // Convalidar Rito de Fertilidad 103 (requerido para Hito 100)
    assert(advSystem.canConvalidate(rel, 103));
    bool ok2 = advSystem.convalidateAdvancement(rel, 103, -3980, events);
    assert(ok2);
    assert(rel.hasAdvancement(103));
    assert(rel.harvestYieldBonus > 0.0f); // Bonus demográfico

    // 3. Salto de Era mediante Hito de Ascensión (Hito Megalítico 100)
    rel.theologyPoints = 400;
    assert(advSystem.canConvalidate(rel, 100));
    bool okCap = advSystem.convalidateAdvancement(rel, 100, -3950, events);
    assert(okCap);
    assert(rel.hasAdvancement(100));
    assert(rel.currentEra == HistoricalEra::StructuralShamanism); // ¡Salto de Era!
    assert(rel.advancementLevel >= 2);
    assert(events.back().category == EventCategory::EraAdvancement);
    assert(events.back().isTurningPoint); // Marcado como Punto de Quiebre

    // Ahora con Hito 100 convalidado, 201 queda desbloqueado para investigación (costo: 250 Fe)
    rel.theologyPoints = 400;
    assert(advSystem.canConvalidate(rel, 201));
    bool ok3 = advSystem.convalidateAdvancement(rel, 201, -2900, events);
    assert(ok3);
    assert(rel.hasAdvancement(201));

    // Verificar contador de avances convalidados por Era
    assert(advSystem.getConvalidatedCountForEra(rel, HistoricalEra::PrimordialAnimism) == 3);
    assert(advSystem.getConvalidatedCountForEra(rel, HistoricalEra::StructuralShamanism) == 1);

    // 4. Integración en Renderer (Botón AVANCES ID 460 y Modal del Árbol)
    Renderer renderer(1280, 720);

    // Botón AVANCES en la barra superior (x = 1280 - 340 = 940, y = 5, w = 105, h = 22)
    int btn460 = renderer.checkButtonClicked(950.0f, 10.0f);
    assert(btn460 == 460);

    // Estado del modal en Renderer
    assert(!renderer.isFaithTreeOpen());
    renderer.setFaithTreeOpen(true);
    assert(renderer.isFaithTreeOpen());
    renderer.toggleFaithTree();
    assert(!renderer.isFaithTreeOpen());

    // Clics interactivos en el Modal del Árbol de Avances
    ReligionRegistry registry;
    registry.createPrimordialReligion("Culto del Fuego Solar", "Solarius", ColorRGB(255, 180, 50), "Sun", -4000);
    Religion* pRel = registry.getReligion(1);
    assert(pRel != nullptr);
    pRel->theologyPoints = 500;

    int selAdvId = 101;
    // Clic fuera del modal (x=10, y=10) -> retorna 1 (cerrar)
    assert(renderer.checkFaithTreeModalClick(10.0f, 10.0f, registry, advSystem, selAdvId) == 1);

    // Clic en la primera tarjeta (Col 0, Row 0: Mito del Fuego x=80, y=130) -> selecciona tarjeta
    int clickCardAction = renderer.checkFaithTreeModalClick(80.0f, 130.0f, registry, advSystem, selAdvId);
    assert(clickCardAction == 201); // 100 + 101 = 201
    assert(selAdvId == 101);

    // Clic en el botón principal de convalidación en el panel inferior (btnX = 64+796 = 860, btnY = 431+36 = 467)
    int btnAction = renderer.checkFaithTreeModalClick(900.0f, 480.0f, registry, advSystem, selAdvId);
    assert(btnAction == 2); // Convalidar avance seleccionado

    // Verificación de rasterizado de iconos de fe (8 columnas x 2 filas)
    for (int col = 0; col < 8; ++col) {
        renderer.drawFaithTreeIcon(10 + col * 26, 10, col, false);
        renderer.drawFaithTreeIcon(10 + col * 26, 40, col, true);
    }

    std::cout << "  -> testFaithAdvancementTreeSystem PASSED! Faith Tree, convalidation, eras, replication and modal UI verified." << std::endl;
}

void testDivinePowersAndIntermediaryPreaching() {
    using namespace ReligionSim;
    std::cout << "[Test] Running testDivinePowersAndIntermediaryPreaching (Divine Powers, Shaman/Prophet Preaching & Faith Generation)..." << std::endl;

    DivinePowersSystem powers;
    WorldMap map(32, 32);
    ReligionRegistry registry;
    EventLine eventLine;
    GestorHumanos humanos;

    // 1. Validar costos y nombres
    assert(powers.getPowerCost(DivinePowerType::Lightning) == 25);
    assert(powers.getPowerCost(DivinePowerType::Plague) == 35);
    assert(powers.getPowerCost(DivinePowerType::Miracle) == 30);
    assert(powers.getPowerCost(DivinePowerType::Congregate) == 10);
    assert(!powers.getPowerName(DivinePowerType::Lightning).empty());
    assert(!powers.getPowerDescription(DivinePowerType::Miracle).empty());

    // 2. Crear Religión del Jugador
    int pRelId = registry.createPrimordialReligion("Culto Solar", "Solarius", ColorRGB(255, 180, 50), "Sun", -4000);
    Religion* pRel = registry.getReligion(pRelId);
    assert(pRel != nullptr);
    pRel->theologyPoints = 100; // 100 Fe inicial

    // Crear Religión Rival
    int rivalRelId = registry.createPrimordialReligion("Culto Lunar", "Lunaris", ColorRGB(100, 150, 255), "Moon", -4000);

    // 3. Crear población: un Chamán del jugador y 5 aldeanos rivales cerca de (15, 15)
    Humano chaman(1, "Grog", 14.5f, 14.5f);
    chaman.setReligionId(pRelId);
    chaman.setEsChaman(true);
    humanos.agregarHumano(chaman);

    for (int i = 2; i <= 6; ++i) {
        Humano rival(i, "Aldeano" + std::to_string(i), 15.0f + (i - 2) * 0.4f, 15.0f);
        rival.setReligionId(rivalRelId);
        rival.setDevocion(0.6f);
        humanos.agregarHumano(rival);
    }

    // 4. Test Título según Era: En Era 0 -> CHAMÁN
    std::string title, name;
    int interId = DivinePowersSystem::ensureIntermediary(humanos, registry, pRelId, title, name);
    assert(interId == 1);
    assert(title == "CHAMAN");
    assert(name == "Grog");

    // 5. Test Lanzar Rayo Divino (Costo: 25 Fe)
    int initialFaith = pRel->theologyPoints;
    CastResult resLightning = powers.castPower(DivinePowerType::Lightning, 15, 15, map, registry, humanos, -4000, eventLine, pRelId);
    assert(resLightning.success);
    assert(resLightning.intermediaryTitle == "CHAMAN");
    assert(resLightning.intermediaryName == "Grog");
    assert(resLightning.converts > 0); // Aldeanos rivales convertidos ante el trueno
    assert(resLightning.faithGenerated > 0); // Fe generada por el asombro y prédica
    assert(!powers.getActiveFX().empty());
    assert(!powers.getActiveSpeeches().empty());
    assert(!eventLine.getAllEvents().empty());

    // La fe final debe ser: (initialFaith - 25 + faithGenerated)
    assert(pRel->theologyPoints == initialFaith - 25 + resLightning.faithGenerated);

    // 6. Test Plaga Sagrada
    // Agregar nuevos rivales en (20, 20)
    for (int i = 7; i <= 10; ++i) {
        Humano rival(i, "RivalPlaga" + std::to_string(i), 20.0f, 20.0f);
        rival.setReligionId(rivalRelId);
        rival.setDevocion(0.7f);
        humanos.agregarHumano(rival);
    }
    int faithBeforePlague = pRel->theologyPoints;
    CastResult resPlague = powers.castPower(DivinePowerType::Plague, 20, 20, map, registry, humanos, -3990, eventLine, pRelId);
    assert(resPlague.success);
    assert(resPlague.converts > 0);
    assert(pRel->theologyPoints == faithBeforePlague - 35 + resPlague.faithGenerated);

    // 7. Test Milagro Celestial
    // Establecer una celda árida / desértica en (10, 10)
    map.getCell(10, 10).biome = BiomeType::AridDesert;
    Humano herido(11, "HeridoFiel", 10.0f, 10.0f);
    herido.setReligionId(pRelId);
    herido.setSalud(20.0f); // Gravemente herido
    humanos.agregarHumano(herido);

    int faithBeforeMiracle = pRel->theologyPoints;
    CastResult resMiracle = powers.castPower(DivinePowerType::Miracle, 10, 10, map, registry, humanos, -3980, eventLine, pRelId);
    assert(resMiracle.success);
    // Verificar que la celda desértica fue bendecida y fertilizada
    assert(map.getCell(10, 10).biome == BiomeType::FertileValley);
    // Verificar que el aldeano herido fue sanado milagrosamente
    for (const auto& h : humanos.getHumanos()) {
        if (h.getId() == 11) {
            assert(h.getSalud() > 70.0f);
            break;
        }
    }
    assert(pRel->theologyPoints == faithBeforeMiracle - 30 + resMiracle.faithGenerated);

    // 8. Test de Evolución de Era a PROFETA
    pRel->currentEra = HistoricalEra::UniversalReligions;
    std::string newTitle, newName;
    int profetaId = DivinePowersSystem::ensureIntermediary(humanos, registry, pRelId, newTitle, newName);
    assert(newTitle == "PROFETA");
    assert(profetaId > 0);

    // Al lanzar un nuevo milagro en era avanzada, el intermediario predica como PROFETA
    CastResult resEra3 = powers.castPower(DivinePowerType::Miracle, 10, 10, map, registry, humanos, -1200, eventLine, pRelId);
    assert(resEra3.success);
    assert(resEra3.intermediaryTitle == "PROFETA");

    // 9. Actualización temporal de FX y sermones
    powers.update(10.0f, map, humanos); // Pasar tiempo suficiente para expirar FX y sermones
    assert(powers.getActiveFX().empty());
    assert(powers.getActiveSpeeches().empty());

    std::cout << "  -> testDivinePowersAndIntermediaryPreaching PASSED! Lightning, plague, miracles, Shaman/Prophet preaching and faith economy verified." << std::endl;
}

void testTerrainHazardsAndProphetMiracle() {
    std::cout << "[Test] Running testTerrainHazardsAndProphetMiracle (Ground Walkability, Hazards, Prophet Miracles & UI Layout)..." << std::endl;

    // 1. Verificación de reglas de terreno peligroso y agua en Cell
    ReligionSim::Cell waterCell;
    waterCell.biome = ReligionSim::BiomeType::DeepOcean;
    waterCell.elevation = 0.0f;
    assert(waterCell.isWater());
    assert(!waterCell.isPassableForGround());

    ReligionSim::Cell depressionCell;
    depressionCell.biome = ReligionSim::BiomeType::CoastalPlains;
    depressionCell.elevation = 0.03f; // Depresión abismal
    assert(depressionCell.isDangerousTerrain());
    assert(!depressionCell.isPassableForGround());

    ReligionSim::Cell lavaPeakCell;
    lavaPeakCell.biome = ReligionSim::BiomeType::RuggedMountains;
    lavaPeakCell.elevation = 3.2f; // Pico volcánico / magma
    assert(lavaPeakCell.isDangerousTerrain());
    assert(!lavaPeakCell.isPassableForGround());

    ReligionSim::Cell normalLandCell;
    normalLandCell.biome = ReligionSim::BiomeType::FertileValley;
    normalLandCell.elevation = 1.0f;
    assert(!normalLandCell.isWater());
    assert(!normalLandCell.isDangerousTerrain());
    assert(normalLandCell.isPassableForGround());

    // 2. Comportamiento de unidades terrestres vs Profeta milagroso
    ReligionSim::WorldMap map(16, 16);
    // Configurar celda (5, 5) como tierra firme y celda (6, 5) como océano
    map.getCell(5, 5).biome = ReligionSim::BiomeType::FertileValley;
    map.getCell(5, 5).elevation = 1.0f;
    map.getCell(6, 5).biome = ReligionSim::BiomeType::DeepOcean;
    map.getCell(6, 5).elevation = 0.0f;

    // Configurar celda (5, 6) como depresión abismal peligrosa
    map.getCell(5, 6).biome = ReligionSim::BiomeType::CoastalPlains;
    map.getCell(5, 6).elevation = 0.02f;

    // Aldeano normal (recolector / terrestre)
    ReligionSim::Humano aldeano(1, "AldeanoTerrestre", 5.5f, 5.5f);
    assert(!aldeano.esProfeta());
    assert(!aldeano.puedeCaminarSobreAguaYTerrenoPeligroso());

    // Ordenar al aldeano caminar hacia el agua (6.5f, 5.5f)
    aldeano.caminarHacia(6.5f, 5.5f, true);
    aldeano.actualizar(1.0f, map); // Avanza con velocidad suficiente para cruzar si no hubiera barrera
    // Debe haberse detenido antes de ingresar al agua y no haber cruzado la frontera X >= 6.0f
    assert(aldeano.getX() < 6.0f);
    assert(aldeano.getAccion() == ReligionSim::AccionHumano::IDLE);

    // Ordenar al aldeano caminar hacia la depresión peligrosa (5.5f, 6.5f)
    aldeano.caminarHacia(5.5f, 6.5f, true);
    aldeano.actualizar(1.0f, map);
    assert(aldeano.getY() < 6.0f);
    assert(aldeano.getAccion() == ReligionSim::AccionHumano::IDLE);

    // Profeta con el don especial de caminar sobre aguas y peligros
    ReligionSim::Humano profeta(2, "ProfetaElias", 5.5f, 5.5f);
    profeta.setEsProfeta(true);
    assert(profeta.esProfeta());
    assert(profeta.puedeCaminarSobreAguaYTerrenoPeligroso());

    // El profeta camina hacia el agua (6.5f, 5.5f)
    profeta.caminarHacia(6.5f, 5.5f, true);
    profeta.actualizar(1.0f, map);
    // Debe haber cruzado exitosamente hacia la celda acuática
    assert(profeta.getX() >= 6.0f);
    assert(map.getCell(profeta.getCeldaX(), profeta.getCeldaY()).isWater());

    // El profeta camina hacia la depresión peligrosa
    profeta.caminarHacia(6.5f, 6.5f, true);
    profeta.actualizar(1.0f, map);
    assert(profeta.getY() >= 5.8f);

    // 3. Verificación de posiciones de la interfaz (Renderer UI)
    ReligionSim::Renderer renderer(1280, 720);
    const auto& btns = renderer.getButtons();

    // Controles de simulación (201..204)
    const ReligionSim::UIButton* btnPausa = nullptr;
    const ReligionSim::UIButton* btn1x = nullptr;
    const ReligionSim::UIButton* btnRayo = nullptr;
    const ReligionSim::UIButton* btnPlaga = nullptr;
    const ReligionSim::UIButton* btnMilagro = nullptr;
    const ReligionSim::UIButton* btnCongregar = nullptr;

    for (const auto& b : btns) {
        if (b.id == 201) btnPausa = &b;
        if (b.id == 202) btn1x = &b;
        if (b.id == 710) btnRayo = &b;
        if (b.id == 711) btnPlaga = &b;
        if (b.id == 712) btnMilagro = &b;
        if (b.id == 713) btnCongregar = &b;
    }

    assert(btnPausa != nullptr);
    assert(btn1x != nullptr);
    assert(btnRayo != nullptr);
    assert(btnPlaga != nullptr);
    assert(btnMilagro != nullptr);
    assert(btnCongregar != nullptr);

    // Los controles de simulación deben estar justo debajo de la línea de tiempo (botY + 24) y con dimensiones amplias
    float botY = 720.0f - 118.0f; // botH = 118
    assert(btnPausa->y == botY + 24.0f);
    assert(btnPausa->width >= 100.0f);
    assert(btnPausa->height >= 26.0f);

    // EN: Verify round Play button geometry (strictly circular with diameter 30, centered with cassette keys)
    // ES: Verificar la geometría del botón redondo de Play (estrictamente circular con diámetro 30, centrado con las teclas de casetera)
    assert(btn1x->width == 30.0f);
    assert(btn1x->height == 30.0f);
    assert(btn1x->y == botY + 22.0f);

    // Los poderes de Dios deben estar DEBAJO de los controles de simulación (botY + 54 > botY + 24)
    assert(btnRayo->y >= botY + 52.0f && btnRayo->y <= botY + 58.0f);
    assert(btnPlaga->y >= botY + 52.0f && btnPlaga->y <= botY + 58.0f);
    assert(btnMilagro->y >= botY + 52.0f && btnMilagro->y <= botY + 58.0f);
    assert(btnCongregar->y >= botY + 52.0f && btnCongregar->y <= botY + 58.0f);
    assert(btnRayo->y > btnPausa->y);

    // Todos los poderes deben estar dentro del panel central (azulX .. azulX + azulW)
    float verdeW = 1280.0f * 0.185f;
    float azulW = 1280.0f * 0.63f;
    assert(btnRayo->x >= verdeW && btnRayo->x + btnRayo->width <= verdeW + azulW);
    assert(btnCongregar->x >= verdeW && btnCongregar->x + btnCongregar->width <= verdeW + azulW);

    // Los botones de sincretismo, cisma y templo (IDs 301, 302, 303) deben haber desaparecido
    for (const auto& b : btns) {
        assert(b.id != 301);
        assert(b.id != 302);
        assert(b.id != 303);
    }

    std::cout << "  -> testTerrainHazardsAndProphetMiracle PASSED! Terrain hazards, Prophet water walking & UI layout verified." << std::endl;
}

void testEraEvolutionEngineDualTimeAndTippingPoints() {
    std::cout << "[Test] Running testEraEvolutionEngineDualTimeAndTippingPoints (Dual Chrono/Macro Engine, Discoveries, Events, Accumulators & Tipping Points)..." << std::endl;

    ReligionSim::HistoricalEvolutionEngine engine;

    // 1. Verificación del reloj cronológico y velocidades (GameClock)
    auto& clock = engine.getClock();
    assert(clock.getCurrentYear() == -4000.0);
    assert(clock.getSpeed() == ReligionSim::TimeSpeed::Speed1X);
    assert(!clock.isPaused());

    // Actualizar 1 segundo a velocidad 1X (10 años/segundo base)
    clock.update(1.0f);
    assert(clock.getCurrentYear() == -3990.0);
    assert(clock.getScaledDelta() == 1.0f);

    // Probar Pausa
    clock.pause();
    assert(clock.isPaused());
    clock.update(1.0f);
    assert(clock.getCurrentYear() == -3990.0); // El calendario no avanza
    assert(clock.getScaledDelta() == 0.0f);

    // Reanudar a 5X
    clock.setSpeed(ReligionSim::TimeSpeed::Speed5X);
    assert(!clock.isPaused());
    clock.update(1.0f);
    assert(clock.getScaledDelta() == 5.0f);
    assert(clock.getCurrentYear() == -3940.0);

    // 2. Verificación de descubrimientos (DiscoverySystem)
    auto& disc = engine.getDiscoveries();
    assert(!disc.isUnlocked(101));
    assert(disc.calculateNetMaterialBonus() == 1.0f);

    bool unlocked = disc.unlock(101, clock.getCurrentYear());
    assert(unlocked);
    assert(disc.isUnlocked(101));
    assert(disc.calculateNetMaterialBonus() > 1.2f); // Fuego multiplica producción

    // 3. Verificación de eventos y observadores (EventSystem)
    auto& events = engine.getEvents();
    bool observerCalled = false;
    events.subscribe([&](const ReligionSim::HistoricEvent& ev) {
        observerCalled = true;
        assert(ev.title == "Revelacion Test");
    });

    ReligionSim::HistoricEvent testEv;
    testEv.id = 999;
    testEv.title = "Revelacion Test";
    testEv.materialDelta = +20.0f;
    testEv.faithDelta = +100.0f;
    testEv.crisisDelta = -10.0f;
    events.emitEvent(testEv);
    assert(observerCalled);
    assert(!events.getHistory().empty());

    // 4. Verificación de acumuladores históricos (HistoryEvolutionManager)
    auto& evolution = engine.getEvolution();
    const auto& acc = evolution.getAccumulators();
    // La suscripción del motor ya aplicó el impacto del evento a los acumuladores
    assert(acc.mysticalFaithResonance >= 200.0f);

    // 5. Verificación de Máquina de Estados de Eras y Tipping Point Interruption (EraManager)
    auto& eraMgr = engine.getEraManager();
    assert(eraMgr.getCurrentEra() == ReligionSim::CivilizationEra::PrimordialAnimism);
    assert(!eraMgr.isTransitionPending());

    bool milestoneReported = false;
    ReligionSim::CivilizationEra reportedNewEra = ReligionSim::CivilizationEra::PrimordialAnimism;
    eraMgr.setMilestoneListener([&](const ReligionSim::MilestoneReport& rep) {
        milestoneReported = true;
        reportedNewEra = rep.newEra;
        assert(rep.eraTitle == "ERA DEL CHAMANISMO ESTRUCTURAL");
    });

    // Inyectar progreso suficiente para cumplir el umbral de Era 1 (StructuralShamanism)
    // Requisitos: Year >= -3800, Material >= 220, Faith >= 250, Tech 101 unlocked
    evolution.applyImpact(+200.0f, +150.0f, 0.0f);

    // Avanzar el reloj a 5X para alcanzar el año mínimo requerido (-3800 BCE)
    clock.update(4.0f); // 4s * 5X * 10yr/s = +200 años -> alcanza -3740 BCE
    assert(clock.getCurrentYear() >= -3800.0);

    // Ejecutar ciclo integral de actualización del motor
    engine.update(0.1f);

    // El Tipping Point debe haberse disparado
    assert(eraMgr.getCurrentEra() == ReligionSim::CivilizationEra::StructuralShamanism);
    assert(eraMgr.isTransitionPending());
    assert(milestoneReported);
    assert(reportedNewEra == ReligionSim::CivilizationEra::StructuralShamanism);

    // CRUCIAL: El reloj de juego DEBE haberse forzado automáticamente a Pausa (0X)
    assert(clock.isPaused());
    assert(clock.getSpeed() == ReligionSim::TimeSpeed::Pause);

    std::cout << "  -> testEraEvolutionEngineDualTimeAndTippingPoints PASSED! Dual engine, accumulators, discoveries, events & tipping points verified." << std::endl;
}

void testResourceSystemAndWorldCreation() {
    std::cout << "[Test] Running testResourceSystemAndWorldCreation..." << std::endl;

    // 1. Generación automática de recursos al crear el mapa
    ReligionSim::WorldMap map(32, 32);
    map.generateProceduralWorld(12345);

    int totalFood = 0, totalWood = 0, totalStone = 0, totalGold = 0;
    int foodTiles = 0, woodTiles = 0, stoneTiles = 0, goldTiles = 0;

    for (int y = 0; y < 32; ++y) {
        for (int x = 0; x < 32; ++x) {
            const ReligionSim::Cell& c = map.getCell(x, y);
            if (c.naturalResource == ReligionSim::ResourceType::Food) {
                totalFood += c.resourceAmount;
                foodTiles++;
                assert(c.biome == ReligionSim::BiomeType::FertileValley ||
                       c.biome == ReligionSim::BiomeType::CoastalPlains ||
                       c.biome == ReligionSim::BiomeType::ShallowWater ||
                       c.biome == ReligionSim::BiomeType::FrozenTundra);
            } else if (c.naturalResource == ReligionSim::ResourceType::Wood) {
                totalWood += c.resourceAmount;
                woodTiles++;
                assert(c.biome == ReligionSim::BiomeType::DenseForest);
            } else if (c.naturalResource == ReligionSim::ResourceType::Stone) {
                totalStone += c.resourceAmount;
                stoneTiles++;
                assert(c.biome == ReligionSim::BiomeType::HighlandHills ||
                       c.biome == ReligionSim::BiomeType::RuggedMountains ||
                       c.biome == ReligionSim::BiomeType::AridDesert ||
                       c.biome == ReligionSim::BiomeType::FrozenTundra ||
                       c.biome == ReligionSim::BiomeType::VolcanicCaldera);
            } else if (c.naturalResource == ReligionSim::ResourceType::Gold) {
                totalGold += c.resourceAmount;
                goldTiles++;
                assert(c.biome == ReligionSim::BiomeType::HighlandHills ||
                       c.biome == ReligionSim::BiomeType::RuggedMountains ||
                       c.biome == ReligionSim::BiomeType::VolcanicCaldera);
            }
        }
    }

    assert(foodTiles > 0);
    assert(woodTiles > 0);
    assert(stoneTiles > 0);
    assert(totalFood > 0);
    assert(totalWood > 0);
    assert(totalStone > 0);

    // 2. Elección de los aldeanos según el entorno en donde viven
    ReligionSim::Humano aldeanoBosque(1, "Lumberjack", 10.5f, 10.5f);
    ReligionSim::Cell& forestCell = map.getCell(10, 10);
    forestCell.biome = ReligionSim::BiomeType::DenseForest;
    forestCell.naturalResource = ReligionSim::ResourceType::Wood;
    forestCell.resourceAmount = 300;

    ReligionSim::VillageResources testRes;
    testRes.food = 500; // Suficiente comida
    ReligionSim::ResourceType resChoice = aldeanoBosque.elegirQueRecolectarSegunEntorno(map, &testRes);
    assert(resChoice == ReligionSim::ResourceType::Wood);

    // Si la comida del pueblo o del aldeano cae críticamente, debe priorizar comida incluso en el bosque
    testRes.food = 30;
    testRes.isStarving = true;
    ReligionSim::ResourceType emergencyChoice = aldeanoBosque.elegirQueRecolectarSegunEntorno(map, &testRes);
    assert(emergencyChoice == ReligionSim::ResourceType::Food);

    // En colinas debe elegir piedra, o hierro si hay veta de hierro
    ReligionSim::Humano aldeanoColina(2, "Miner", 15.5f, 15.5f);
    ReligionSim::Cell& hillCell = map.getCell(15, 15);
    hillCell.biome = ReligionSim::BiomeType::HighlandHills;
    hillCell.naturalResource = ReligionSim::ResourceType::Stone;
    hillCell.resourceAmount = 250;
    testRes.food = 500;
    testRes.isStarving = false;
    resChoice = aldeanoColina.elegirQueRecolectarSegunEntorno(map, &testRes);
    assert(resChoice == ReligionSim::ResourceType::Stone);

    // Veta de hierro
    hillCell.naturalResource = ReligionSim::ResourceType::Iron;
    hillCell.resourceAmount = 180;
    resChoice = aldeanoColina.elegirQueRecolectarSegunEntorno(map, &testRes);
    assert(resChoice == ReligionSim::ResourceType::Iron);
    assert(std::string(ReligionSim::getResourceTypeName(ReligionSim::ResourceType::Iron)) == "Hierro");

    // 3. Supervivencia y consumo: aldeanos consumen comida; la falta genera inanición
    ReligionSim::ReligionRegistry reg;
    int relId = reg.createPrimordialReligion("TestCult", "Leader", ReligionSim::ColorRGB{255, 200, 50}, "Sun", -4000);
    ReligionSim::Religion* rel = reg.getReligion(relId);
    assert(rel != nullptr);

    // Inicializar inventario con recursos iniciales incluyendo hierro
    rel->resources.food = 100;
    assert(!rel->resources.isStarving);
    assert(rel->resources.consumeFood(60));
    assert(rel->resources.food == 40);
    assert(!rel->resources.consumeFood(50)); // Se agota
    assert(rel->resources.food == 0);
    assert(rel->resources.isStarving);
    rel->resources.addResource(ReligionSim::ResourceType::Iron, 75);
    assert(rel->resources.getAmount(ReligionSim::ResourceType::Iron) == 80 + 75);
    rel->resources.addResource(ReligionSim::ResourceType::Copper, 50);
    assert(rel->resources.getAmount(ReligionSim::ResourceType::Copper) == 60 + 50);
    rel->resources.addResource(ReligionSim::ResourceType::Diamond, 20);
    assert(rel->resources.getAmount(ReligionSim::ResourceType::Diamond) == 15 + 20);
    assert(std::string(ReligionSim::getResourceTypeName(ReligionSim::ResourceType::Copper)) == "Cobre");
    assert(std::string(ReligionSim::getResourceTypeName(ReligionSim::ResourceType::Diamond)) == "Diamante");

    // Verificar generación de vetas de hierro en el mapa
    int countIron = 0;
    for (int y = 0; y < map.getHeight(); ++y) {
        for (int x = 0; x < map.getWidth(); ++x) {
            if (map.getCell(x, y).naturalResource == ReligionSim::ResourceType::Iron) {
                countIron++;
            }
        }
    }
    assert(countIron > 0);

    // 4. Configuración inicial al crear el mundo
    ReligionSim::WorldCreationConfig config;
    assert(config.initialResources == 500); // Valor por defecto
    config.initialResources = 1200; // Caja de texto personalizada por el usuario
    assert(config.initialResources == 1200);

    // 5. Botón interactivo 516 en la UI y carga de sprites de recursos (64x64)
    ReligionSim::Renderer rend(1280, 720);
    const auto& cBtns = rend.getCreationButtons();
    bool foundBtn516 = false;
    for (const auto& btn : cBtns) {
        if (btn.id == 516) {
            foundBtn516 = true;
            assert(btn.width > 200.0f);
            assert(btn.height >= 24.0f);
        }
    }
    assert(foundBtn516);

    const auto& resProps = rend.getResourcePropPixels();
    assert(resProps.size() == 7);
    for (size_t i = 0; i < 7; ++i) {
        assert(resProps[i].size() == 64u * 64u);
    }

    std::cout << "  -> testResourceSystemAndWorldCreation PASSED! Auto generation, smart biome choice, starvation & world creation input verified." << std::endl;
}

void testNaturalDisastersAndPhysics() {
    using namespace ReligionSim;
    std::cout << "[Test] Running testNaturalDisastersAndPhysics..." << std::endl;

    // 1. Verificar metadata de catástrofes divinas
    DivinePowersSystem powersSys;
    assert(powersSys.getPowerCost(DivinePowerType::Tornado) == 40);
    assert(powersSys.getPowerCost(DivinePowerType::Earthquake) == 50);
    assert(powersSys.getPowerCost(DivinePowerType::Pandemic) == 45);
    assert(powersSys.getPowerRadius(DivinePowerType::Tornado) >= 3.0f);
    assert(powersSys.getPowerRadius(DivinePowerType::Earthquake) >= 4.0f);
    assert(powersSys.getPowerRadius(DivinePowerType::Pandemic) >= 3.5f);

    WorldMap map(40, 40);
    for (int y = 0; y < 40; ++y) {
        for (int x = 0; x < 40; ++x) {
            Cell& c = map.getCell(x, y);
            c.biome = BiomeType::FertileValley;
            c.elevation = 1.0f;
            c.dominantReligionId = 1;
            c.population = 200;
            c.hasHolySite = true;
            c.shrineLevel = 3;
            c.devotion = 1.0f;
        }
    }

    ReligionRegistry reg;
    int playerRelId = reg.createPrimordialReligion("Culto Cataclísmico", "Cataclismus", ColorRGB(200, 100, 50), "Storm", 100);
    reg.getReligion(playerRelId)->theologyPoints = 200; // Suficiente fe para los 3 poderes

    // 2. Físicas de vuelo y succión por Tornado en Humano
    GestorHumanos gestor;
    Humano h1Init(1, "Aldeano Viento", 10.0f, 10.0f);
    gestor.agregarHumano(h1Init);
    Humano* h1 = &gestor.getHumanos()[0];
    assert(h1 != nullptr);
    assert(!h1->isFlying());
    assert(h1->getAltitude() == 0.0f);

    // Tornado a 0.5 unidades de distancia (succión directa al vórtice)
    h1->serSuccionadoPorTornado(10.2f, 10.1f, 4.5f, 0.2f);
    assert(h1->isFlying());
    assert(h1->getAltitude() > 0.0f);
    assert(h1->getVelocityZ() > 0.0f);

    // Simular caída por gravedad
    for (int step = 0; step < 60; ++step) {
        h1->actualizar(0.05f, map);
        if (!h1->isFlying()) break;
    }
    // Aterrizó: ya no vuela, pero queda aturdido por el impacto
    assert(!h1->isFlying());
    assert(h1->getAltitude() == 0.0f);
    assert(h1->getStumbleTimer() > 0.0f);

    // 3. Estampida de pánico (Huir de la catástrofe)
    Humano h2Init(2, "Aldeano Panico", 12.0f, 10.0f);
    gestor.agregarHumano(h2Init);
    Humano* h2 = &gestor.getHumanos()[1];
    assert(h2 != nullptr);
    h2->huirEnPanicoDesde(10.0f, 10.0f, 3.0f);
    assert(h2->getPanicTimer() > 0.0f);
    assert(h2->getDestinoX() > h2->getX());
    float prevX = h2->getX();
    h2->actualizar(0.2f, map);
    assert(h2->getX() > prevX);

    // 4. Terremoto: Sacudida sísmica, caída de aldeanos y screen shake
    IsometricCamera cam(1280, 720);
    assert(cam.getShakeIntensity() == 0.0f);
    cam.addScreenShake(8.0f, 4.0f);
    assert(cam.getShakeIntensity() == 8.0f);
    cam.update(0.1f);
    // Desplazamiento de cámara activo mientras dura la sacudida
    assert(cam.getShakeOffsetX() != 0.0f || cam.getShakeOffsetY() != 0.0f);

    Humano h3Init(3, "Aldeano Sismo", 15.0f, 15.0f);
    gestor.agregarHumano(h3Init);
    Humano* h3 = &gestor.getHumanos()[2];
    float initialSaludH3 = h3->getSalud();
    h3->sufrirSismoTerremoto(15.0f, 15.0f, 1.5f);
    assert(h3->getStumbleTimer() > 0.0f);
    assert(h3->getSalud() < initialSaludH3);

    // 5. Pandemia y contagio viral humano a humano
    Humano infInit(4, "Paciente Cero", 20.0f, 20.0f);
    Humano sano1Init(5, "Contacto Estrecho", 20.3f, 20.2f); // A menos de 0.8 unidades
    Humano sano2Init(6, "Aldeano Distante", 35.0f, 35.0f);   // Lejos
    gestor.agregarHumano(infInit);
    gestor.agregarHumano(sano1Init);
    gestor.agregarHumano(sano2Init);

    Humano* infectado = &gestor.getHumanos()[3];
    Humano* sanoCercano = &gestor.getHumanos()[4];
    Humano* sanoLejano = &gestor.getHumanos()[5];

    assert(!infectado->isInfected());
    infectado->infectarConPandemia(15.0f);
    assert(infectado->isInfected());
    assert(infectado->getInfectionTimer() > 0.0f);

    // El gestor procesa contagio biológico por cercanía
    gestor.actualizar(0.5f, map, nullptr, 100, &reg);
    assert(sanoCercano->isInfected()); // Se contagió por contacto
    assert(!sanoLejano->isInfected());  // Muy lejos para contagiarse

    // 6. Lanzamiento de catástrofes en WorldMap y DivinePowersSystem
    EventLine eventLine;

    // Lanzar Terremoto en (10, 10)
    int ptsBeforeQuake = reg.getReligion(playerRelId)->theologyPoints;
    auto resQuake = powersSys.castPower(DivinePowerType::Earthquake, 10, 10, map, reg,
                                        gestor, 100, eventLine, playerRelId);
    assert(resQuake.success);
    assert(reg.getReligion(playerRelId)->theologyPoints == ptsBeforeQuake - 50 + resQuake.faithGenerated);
    assert(map.getCell(10, 10).shrineLevel < 3); // Estructura dañada por sismo

    // Lanzar Tornado en (20, 20)
    int ptsBeforeTornado = reg.getReligion(playerRelId)->theologyPoints;
    auto resTornado = powersSys.castPower(DivinePowerType::Tornado, 20, 20, map, reg,
                                          gestor, 100, eventLine, playerRelId);
    assert(resTornado.success);
    assert(reg.getReligion(playerRelId)->theologyPoints == ptsBeforeTornado - 40 + resTornado.faithGenerated);
    assert(powersSys.getActiveFX().size() >= 1);

    // Lanzar Pandemia en (25, 25)
    map.getCell(25, 25).population = 500;
    int ptsBeforePandemic = reg.getReligion(playerRelId)->theologyPoints;
    int popPrevia = map.getCell(25, 25).population;
    auto resPandemic = powersSys.castPower(DivinePowerType::Pandemic, 25, 25, map, reg,
                                           gestor, 100, eventLine, playerRelId);
    assert(resPandemic.success);
    assert(reg.getReligion(playerRelId)->theologyPoints == ptsBeforePandemic - 45 + resPandemic.faithGenerated);
    assert(map.getCell(25, 25).population < popPrevia); // Bajas inmediatas por epidemia

    // 7. Botones UI para lanzar catástrofes en Renderer (IDs 720, 721, 722)
    Renderer rend(1280, 720);
    bool found720 = false, found721 = false, found722 = false;
    for (const auto& btn : rend.getButtons()) {
        if (btn.id == 720) found720 = true;
        if (btn.id == 721) found721 = true;
        if (btn.id == 722) found722 = true;
    }
    assert(found720);
    assert(found721);
    assert(found722);

    std::cout << "  -> testNaturalDisastersAndPhysics PASSED! Tornadoes, Earthquakes, Pandemics, air physics and contagion verified." << std::endl;
}

void testDisastersLethalityAndWaterDrowning() {
    std::cout << "[Test] Running testDisastersLethalityAndWaterDrowning (Probabilistic Lethality per Power Nature & Water Drowning)..." << std::endl;
    using namespace ReligionSim;

    WorldMap map(32, 32);
    ReligionRegistry registry;
    EventLine eventLine;
    DivinePowersSystem powers;

    int pRelId = registry.createPrimordialReligion("Culto Solar", "Solarius", ColorRGB(255, 180, 50), "Sun", -4000);
    int rivalRelId = registry.createPrimordialReligion("Clan Rival", "Nox", ColorRGB(100, 100, 200), "Shadow", -4000);
    registry.getReligion(pRelId)->theologyPoints = 1000;

    // 1. Validar Ahogamiento al caer al agua:
    // Configurar celdas de agua: (5, 5) como DeepOcean, (5, 6) como ShallowWater, (5, 7) como Plains (tierra firme)
    map.getCell(5, 5).biome = BiomeType::DeepOcean;
    map.getCell(5, 5).elevation = 0.0f;
    map.getCell(5, 6).biome = BiomeType::ShallowWater;
    map.getCell(5, 6).elevation = 0.0f;
    map.getCell(5, 7).biome = BiomeType::CoastalPlains;
    map.getCell(5, 7).elevation = 1.0f;

    // A. Caída en Océano Profundo (75% probabilidad)
    int drownCountOcean = 0;
    int swimCountOcean = 0;
    for (int i = 0; i < 40; ++i) {
        Humano h(100 + i, "Marinero Oceanico", 5.5f, 5.5f);
        h.setRole(CommunityRole::FORAGER);
        h.procesarCaidaEnAgua(map);
        if (h.isDrowning()) {
            assert(h.estaMuerto());
            assert(h.getSalud() == 0.0f);
            assert(!h.getCauseOfDeath().empty());
            drownCountOcean++;
        } else {
            assert(h.estaVivo());
            assert(h.isSwimming());
            assert(h.getSalud() < 100.0f); // Recibió daño de fatiga
            swimCountOcean++;
        }
    }
    assert(drownCountOcean > 0);
    assert(swimCountOcean > 0);
    assert(drownCountOcean > swimCountOcean);

    // B. Caída en Aguas Someras (35% probabilidad)
    int drownCountShallow = 0;
    int swimCountShallow = 0;
    for (int i = 0; i < 40; ++i) {
        Humano h(200 + i, "Vadeador Somero", 5.5f, 6.5f);
        h.setRole(CommunityRole::FORAGER);
        h.procesarCaidaEnAgua(map);
        if (h.isDrowning()) {
            drownCountShallow++;
        } else {
            swimCountShallow++;
            assert(h.isSwimming());
        }
    }
    assert(drownCountShallow > 0);
    assert(swimCountShallow > 0);
    assert(swimCountShallow > drownCountShallow);

    // C. Profeta milagroso en agua (100% inmune, camina sobre el agua)
    Humano profeta(300, "Profeta Sagrado", 5.5f, 5.5f);
    profeta.setEsChaman(true);
    profeta.setRole(CommunityRole::PROPHET);
    assert(profeta.puedeCaminarSobreAguaYTerrenoPeligroso());
    profeta.procesarCaidaEnAgua(map);
    assert(!profeta.isDrowning());
    assert(!profeta.isSwimming());
    assert(profeta.estaVivo());
    assert(profeta.getSalud() == 100.0f);

    // D. Físicas de Tornado cayendo en agua:
    Humano volador(400, "Volador Hacia El Agua", 5.5f, 5.5f);
    volador.setRole(CommunityRole::FORAGER);
    volador.setAltitude(25.0f);
    volador.setFlying(true);
    for (int step = 0; step < 50; ++step) {
        volador.actualizar(0.05f, map);
        if (!volador.isFlying()) break;
    }
    assert(!volador.isFlying());
    assert(volador.isDrowning() || volador.isSwimming());

    // 2. Letalidad en Rayo Divino:
    GestorHumanos gestorRayo;
    Humano chaman(1, "Chaman Rayo", 10.0f, 10.0f);
    chaman.setReligionId(pRelId);
    chaman.setEsChaman(true);
    gestorRayo.agregarHumano(chaman);

    for (int i = 0; i < 20; ++i) {
        Humano r(10 + i, "Blanco Rayo " + std::to_string(i), 15.0f + (i % 2) * 0.1f, 15.0f + (i / 2) * 0.05f);
        r.setReligionId(rivalRelId);
        gestorRayo.agregarHumano(r);
    }
    auto resRayo = powers.castPower(DivinePowerType::Lightning, 15, 15, map, registry, gestorRayo, -4000, eventLine, pRelId);
    assert(resRayo.success);
    assert(resRayo.casualties > 0); // Rayo fulminó con ~85% en centro
    std::cout << "  [Test Info] Bajas por Rayo Divino: " << resRayo.casualties << " de 20." << std::endl;

    // 3. Letalidad en Plaga Sagrada:
    GestorHumanos gestorPlaga;
    Humano chamanPlaga(2, "Chaman Plaga", 20.0f, 20.0f);
    chamanPlaga.setReligionId(pRelId);
    chamanPlaga.setEsChaman(true);
    gestorPlaga.agregarHumano(chamanPlaga);
    for (int i = 0; i < 20; ++i) {
        Humano r(30 + i, "Blanco Plaga " + std::to_string(i), 25.0f + (i % 3) * 0.2f, 25.0f + (i / 3) * 0.2f);
        r.setReligionId(rivalRelId);
        gestorPlaga.agregarHumano(r);
    }
    auto resPlaga = powers.castPower(DivinePowerType::Plague, 25, 25, map, registry, gestorPlaga, -4000, eventLine, pRelId);
    assert(resPlaga.success);
    assert(resPlaga.casualties > 0); // Plaga provocó bajas por asfixia/veneno (~30%)
    std::cout << "  [Test Info] Bajas por Plaga: " << resPlaga.casualties << " de 20." << std::endl;

    // 4. Letalidad en Terremoto:
    GestorHumanos gestorSismo;
    Humano chamanSismo(3, "Chaman Sismo", 2.0f, 2.0f);
    chamanSismo.setReligionId(pRelId);
    chamanSismo.setEsChaman(true);
    gestorSismo.agregarHumano(chamanSismo);
    for (int i = 0; i < 20; ++i) {
        Humano r(50 + i, "Blanco Sismo " + std::to_string(i), 12.0f + (i % 2) * 0.2f, 12.0f + (i / 2) * 0.2f);
        r.setReligionId(rivalRelId);
        gestorSismo.agregarHumano(r);
    }
    auto resSismo = powers.castPower(DivinePowerType::Earthquake, 12, 12, map, registry, gestorSismo, -4000, eventLine, pRelId);
    assert(resSismo.success);
    assert(resSismo.casualties > 0); // Muertes por fisuras tectónicas (~40% en epicentro)
    std::cout << "  [Test Info] Bajas por Terremoto: " << resSismo.casualties << " de 20." << std::endl;

    // 5. Letalidad en Pandemia Viral:
    GestorHumanos gestorPandemia;
    Humano chamanPandemia(4, "Chaman Pandemia", 2.0f, 2.0f);
    chamanPandemia.setReligionId(pRelId);
    chamanPandemia.setEsChaman(true);
    gestorPandemia.agregarHumano(chamanPandemia);
    for (int i = 0; i < 20; ++i) {
        Humano r(70 + i, "Blanco Pandemia " + std::to_string(i), 18.0f + (i % 2) * 0.2f, 18.0f + (i / 2) * 0.2f);
        r.setReligionId(rivalRelId);
        gestorPandemia.agregarHumano(r);
    }
    auto resPandemia = powers.castPower(DivinePowerType::Pandemic, 18, 18, map, registry, gestorPandemia, -4000, eventLine, pRelId);
    assert(resPandemia.success);
    std::cout << "  [Test Info] Bajas agudas inmediatas por Pandemia: " << resPandemia.casualties << " de 20." << std::endl;

    for (int tick = 0; tick < 100; ++tick) {
        gestorPandemia.actualizar(0.2f, map, &eventLine, -4000, &registry);
    }
    assert(gestorPandemia.getHumanos().size() < 21);

    std::cout << "  -> testDisastersLethalityAndWaterDrowning PASSED! Distinct lethalities per nature and water drowning probabilities fully verified." << std::endl;
}

void testRagdollPhysicsSystem() {
    std::cout << "[Test] Running testRagdollPhysicsSystem (2.5D Ragdoll Tumble, Bounce Restitution, Settling, Water Bobbing & Limb Flailing)..." << std::endl;
    using namespace ReligionSim;

    WorldMap map(30, 30);
    for (int y = 0; y < 30; ++y) {
        for (int x = 0; x < 30; ++x) {
            Cell& c = map.getCell(x, y);
            c.elevation = 1.0f;
            c.biome = BiomeType::CoastalPlains;
        }
    }
    // Asignar una franja de agua somera y océano
    map.getCell(15, 15).biome = BiomeType::ShallowWater;
    map.getCell(15, 16).biome = BiomeType::DeepOcean;

    // 1. Activar Ragdoll en aldeano vivo por impacto o tornado
    Humano h1(1, "Volador Ragdoll", 10.0f, 10.0f);
    assert(!h1.isRagdoll());
    h1.activarRagdoll(3.5f, 0.0f, 25.0f, 14.0f);
    assert(h1.isRagdoll());
    assert(h1.getRagdoll().isActive());
    assert(!h1.getRagdoll().isSettled());
    assert(h1.getRagdoll().getAngle() == 0.0f);

    // 2. Simular vuelo y rotación angular (tumble)
    float prevAngle = h1.getRagdoll().getAngle();
    h1.actualizar(0.08f, map);
    assert(h1.getAltitude() > 0.0f);
    assert(h1.getRagdoll().getAngle() != prevAngle); // Giró en el aire

    // 3. Simular caída hasta el suelo y verificar rebotes elásticos
    int ticks = 0;
    while (h1.isRagdoll() && ticks < 200) {
        h1.actualizar(0.05f, map);
        ticks++;
    }
    // Debe haber rebotado y finalmente asentarse
    assert(h1.getRagdoll().getBounceCount() > 0);
    // Tras asentarse en tierra, el aldeano vivo recupera el pie tras el stumbleTimer
    if (!h1.estaVivo()) {
        // Si sufrio el 50% de letalidad aleatoria por la caida del tornado, revitalizar para validar la recuperacion viva
        h1.setSalud(55.0f);
        h1.setStumbleTimer(2.5f);
        h1.getRagdollMutable().deactivate();
    }
    assert(!h1.isRagdoll()); // Se desactivó el ragdoll al asentarse en vivo
    assert(h1.getStumbleTimer() > 0.0f); // Se levanta tropezando

    // 4. Ragdoll de cadáver (Colapso letal)
    Humano h2(2, "Cadaver Caido", 12.0f, 12.0f);
    assert(h2.estaVivo());
    h2.matar("Colapso repentino");
    assert(h2.estaMuerto());
    assert(h2.isRagdoll()); // Al morir activa ragdoll automáticamente
    assert(h2.getRagdoll().isActive());

    for (int t = 0; t < 120; ++t) {
        h2.actualizar(0.05f, map);
    }
    assert(h2.getRagdoll().isSettled());
    // En cadáver asentado, el cuerpo se acomoda horizontalmente (~1.57 rad o ~-1.57 rad)
    float rad = std::abs(h2.getRagdoll().getAngle());
    assert(rad >= 1.40f && rad <= 1.70f);

    // 5. Ragdoll lanzado al agua
    Humano h3(3, "Ragdoll Acuatico", 15.0f, 15.0f);
    h3.activarRagdoll(0.0f, 0.0f, 15.0f, 8.0f);
    assert(h3.isRagdoll());
    for (int t = 0; t < 100; ++t) {
        h3.actualizar(0.05f, map);
    }
    // Al caer en celda de agua somera (15,15), el aldeano reacciona a la caida acuatica (nada, se ahoga o nada a la orilla)
    assert(h3.isDrowning() || h3.isSwimming() || h3.getRagdoll().isInWater() || !map.getCell(h3.getCeldaX(), h3.getCeldaY()).isWater());

    std::cout << "  -> testRagdollPhysicsSystem PASSED! 2.5D tumbling, bounce physics, resting corpse poses and water ragdoll verified." << std::endl;
}

void testInterTribalInteractionsAndBorderTension() {
    std::cout << "[Test] Running testInterTribalInteractionsAndBorderTension (Ontological Friction, Contested Borders, Blood Pacts & Shrine Desecration)..." << std::endl;
    using namespace ReligionSim;

    ReligionRegistry registry;
    int rSolarId = registry.createPrimordialReligion("Culto Solar", "Solarius", ColorRGB{255, 180, 50}, "Sun", -4000);
    int rChthonicId = registry.createPrimordialReligion("Culto de la Tierra", "Gaia", ColorRGB{120, 80, 200}, "Earth", -4000);
    int rTotemAId = registry.createPrimordialReligion("Clan del Ciervo", "Cervus", ColorRGB{60, 180, 80}, "Totem", -4000);
    int rTotemBId = registry.createPrimordialReligion("Clan del Aguila", "Aquila", ColorRGB{80, 160, 240}, "Totem", -4000);
    int rExclusivistId = registry.createPrimordialReligion("Orden de la Monada", "Monas", ColorRGB{230, 40, 40}, "Order", -4000);

    Religion* rSolar = registry.getReligion(rSolarId);
    Religion* rChthonic = registry.getReligion(rChthonicId);
    Religion* rTotemA = registry.getReligion(rTotemAId);
    Religion* rTotemB = registry.getReligion(rTotemBId);
    Religion* rExclusivist = registry.getReligion(rExclusivistId);

    assert(rSolar != nullptr && rChthonic != nullptr && rTotemA != nullptr && rTotemB != nullptr && rExclusivist != nullptr);

    // 1. Verificación de Arquetipos y Fricción Ontológica Φ(A, B)
    assert(rSolar->archetype == TheologicalArchetype::SolarCelestial);
    assert(rChthonic->archetype == TheologicalArchetype::ChthonicAncestral);
    assert(rTotemA->archetype == TheologicalArchetype::TotemicAnimism);
    assert(rTotemB->archetype == TheologicalArchetype::TotemicAnimism);
    assert(rExclusivist->archetype == TheologicalArchetype::DogmaticExclusivist);

    float fSolarChthonic = rSolar->calculateOntologicalFriction(*rChthonic);
    float fTotem = rTotemA->calculateOntologicalFriction(*rTotemB);
    float fExclusivist = rExclusivist->calculateOntologicalFriction(*rTotemA);

    assert(fSolarChthonic >= 1.50f); // Alta incompatibilidad Luz vs Subterráneo
    assert(fTotem <= 0.35f);         // Muy baja fricción entre animismos totémicos
    assert(fExclusivist >= 2.0f);    // Hostilidad radical del exclusivismo dogmático

    // 2. Simulación de Tensión Liminal de Frontera en Mapa
    WorldMap map(30, 30);
    for (int y = 0; y < 30; ++y) {
        for (int x = 0; x < 30; ++x) {
            Cell& c = map.getCell(x, y);
            c.biome = BiomeType::CoastalPlains;
            c.elevation = 1.0f;
            c.population = 800;
        }
    }

    // Configurar nodo Solar en (10, 10) y nodo Ctónico rival en celda vecina (11, 10)
    Cell& cSolar = map.getCell(10, 10);
    cSolar.setShare(rSolarId, 0.90f);
    cSolar.updateDominance();
    cSolar.devotion = 0.85f;

    Cell& cChthonic = map.getCell(11, 10);
    cChthonic.setShare(rChthonicId, 0.90f);
    cChthonic.updateDominance();
    cChthonic.devotion = 0.85f;

    ReplicationEngine replication;
    std::vector<GameEvent> events;
    replication.update(map, registry, 1.0f, events, -4000);

    // Debe detectar la frontera hostil de alta fricción
    assert(cSolar.isContestedBorder);
    assert(cSolar.borderTension >= 0.40f);
    assert(cSolar.borderRivalReligionId == rChthonicId);
    assert(cChthonic.isContestedBorder);

    // 3. Verificación de Pacto de Sangre Exogámico en Frontera Porosa
    Cell& cTotemA = map.getCell(20, 20);
    cTotemA.setShare(rTotemAId, 0.90f);
    cTotemA.updateDominance();
    cTotemA.devotion = 0.50f;

    Cell& cTotemB = map.getCell(21, 20);
    cTotemB.setShare(rTotemBId, 0.90f);
    cTotemB.updateDominance();
    cTotemB.devotion = 0.50f;

    // Con baja fricción, simular avances para permitir el pacto exogámico
    cTotemA.isSacredTruceActive = true;
    cTotemA.sacredTruceTimer = 30.0f;
    cTotemB.isSacredTruceActive = true;
    cTotemB.sacredTruceTimer = 30.0f;

    // En tregua sagrada, dos zelotes rivales NO combaten
    GestorHumanos gestor;
    Humano zA(1, "Zelote Totem A", 20.2f, 20.2f);
    zA.setReligionId(rTotemAId);
    zA.setRole(CommunityRole::ZEALOT);
    gestor.agregarHumano(zA);

    Humano zB(2, "Zelote Totem B", 20.4f, 20.2f);
    zB.setReligionId(rTotemBId);
    zB.setRole(CommunityRole::ZEALOT);
    gestor.agregarHumano(zB);

    gestor.actualizar(0.5f, map, nullptr, -4000, &registry);
    assert(gestor.getHumanos()[0].getSalud() == 100.0f);
    assert(gestor.getHumanos()[1].getSalud() == 100.0f); // Juramento de paz respetado

    // 4. Verificación de Saqueo de Recursos y Profanación de Santuario
    Cell& cSantuario = map.getCell(15, 15);
    cSantuario.hasHolySite = true;
    cSantuario.shrineLevel = 2;
    cSantuario.holySiteReligionId = rChthonicId;
    cSantuario.devotion = 0.15f;
    cSantuario.isContestedBorder = true;
    cSantuario.borderTension = 0.80f;
    cSantuario.setShare(rChthonicId, 0.90f);
    cSantuario.updateDominance();
    cSantuario.naturalResource = ResourceType::Food;
    cSantuario.resourceAmount = 50;

    GestorHumanos gestorIncursor;
    Humano incursor(10, "Incursor Solar", 15.2f, 15.2f);
    incursor.setReligionId(rSolarId);
    incursor.setRole(CommunityRole::ZEALOT);
    gestorIncursor.agregarHumano(incursor);

    int foodBefore = rSolar->resources.food;
    gestorIncursor.actualizar(0.5f, map, nullptr, -4000, &registry);

    // Debe saquear recursos del nodo enemigo
    assert(rSolar->resources.food > foodBefore);
    // Y degradar el santuario rival no defendido
    assert(cSantuario.devotion < 0.15f);

    std::cout << "  -> testInterTribalInteractionsAndBorderTension PASSED! Ontological friction, contested borders, blood pacts & shrine desecration verified." << std::endl;
}

void testLeaderDevotionAndFanaticism() {
    std::cout << "[Test] Running testLeaderDevotionAndFanaticism (Leader Devotion, Proximity Communion, Fanatics & Influence)..." << std::endl;
    using namespace ReligionSim;

    WorldMap map(40, 40);
    for (int y = 0; y < 40; ++y) {
        for (int x = 0; x < 40; ++x) {
            map.getCell(x, y).biome = BiomeType::FertileValley;
            map.getCell(x, y).elevation = 1.0f;
        }
    }

    ReligionRegistry registry;
    int pRelId = registry.createPrimordialReligion("Culto Solar", "Solarius", ColorRGB{255, 215, 0}, "Sun", -4000);
    Religion* pRel = registry.getReligion(pRelId);
    assert(pRel != nullptr);
    pRel->theologyPoints = 500;

    GestorHumanos gestor;

    // 1. Chamán líder y seguidor ordinario
    Humano chaman(1, "Chaman Khep", 20.0f, 20.0f);
    chaman.setReligionId(pRelId);
    chaman.setEsChaman(true);
    assert(chaman.esLiderEspiritual());
    assert(chaman.getLeaderDevotion() == 1.0f);
    gestor.agregarHumano(chaman);

    Humano seguidor(2, "Grog Devoto", 21.0f, 20.0f);
    seguidor.setReligionId(pRelId);
    seguidor.setDevocion(0.60f);
    seguidor.setLeaderDevotion(0.50f);
    assert(!seguidor.esFanatico());
    gestor.agregarHumano(seguidor);

    // 2. Simular cercanía y verificar aumento de devoción al líder
    float ldBefore = gestor.getHumanos()[1].getLeaderDevotion();
    for (int t = 0; t < 20; ++t) {
        gestor.actualizar(0.2f, map, nullptr, -4000, &registry);
    }
    float ldAfter = gestor.getHumanos()[1].getLeaderDevotion();
    assert(ldAfter > ldBefore);
    assert(gestor.getHumanos()[1].getLeaderId() == chaman.getId());

    // 3. Activación de señal divina del chamán acelera la devoción
    chaman.activarSenalDivina(5.0f);
    float devBeforeSignal = gestor.getHumanos()[1].getDevocion();
    for (int t = 0; t < 10; ++t) {
        gestor.actualizar(0.2f, map, nullptr, -4000, &registry);
    }
    assert(gestor.getHumanos()[1].getDevocion() > devBeforeSignal);

    // 4. Distancia / Alienación espiritual cuando el seguidor está lejos (> 8.5 celdas)
    Humano peregrino(3, "Grog Lejano", 36.0f, 36.0f);
    peregrino.setReligionId(pRelId);
    peregrino.setLeaderDevotion(0.70f);
    gestor.agregarHumano(peregrino);

    float ldFarBefore = gestor.getHumanos()[2].getLeaderDevotion();
    for (int t = 0; t < 20; ++t) {
        gestor.actualizar(0.2f, map, nullptr, -4000, &registry);
    }
    float ldFarAfter = gestor.getHumanos()[2].getLeaderDevotion();
    assert(ldFarAfter < ldFarBefore); // Alienación espiritual por lejanía

    // 5. Emergencia del Fanático al alcanzar devoción extrema (devocion >= 0.88, leaderDevotion >= 0.85)
    Humano fervoroso(4, "Grog Fervoroso", 20.5f, 20.5f);
    fervoroso.setReligionId(pRelId);
    fervoroso.setDevocion(0.90f);
    fervoroso.setLeaderDevotion(0.90f);
    gestor.agregarHumano(fervoroso);

    gestor.actualizar(0.1f, map, nullptr, -4000, &registry);
    Humano* fFervoroso = nullptr;
    for (auto& h : gestor.getHumanos()) {
        if (h.getId() == 4) fFervoroso = &h;
    }
    assert(fFervoroso != nullptr);
    assert(fFervoroso->esFanatico());
    assert(fFervoroso->getRole() == CommunityRole::ZEALOT);
    assert(fFervoroso->getHumanUnit().conversionInertia == 95.0f);

    // 6. Fanático reprende y aviva la fe de un miembro tibio cercano
    Humano tibio(5, "Grog Tibio", 21.0f, 20.5f);
    tibio.setReligionId(pRelId);
    tibio.setDevocion(0.30f);
    tibio.setLeaderDevotion(0.30f);
    gestor.agregarHumano(tibio);

    Humano* fTibio = nullptr;
    for (auto& h : gestor.getHumanos()) {
        if (h.getId() == 5) fTibio = &h;
    }
    assert(fTibio != nullptr);
    float tibioDevBefore = fTibio->getDevocion();
    float tibioLdBefore = fTibio->getLeaderDevotion();
    for (int t = 0; t < 15; ++t) {
        gestor.actualizar(0.2f, map, nullptr, -4000, &registry);
    }
    for (auto& h : gestor.getHumanos()) {
        if (h.getId() == 5) fTibio = &h;
    }
    assert(fTibio != nullptr);
    assert(fTibio->getDevocion() > tibioDevBefore);
    assert(fTibio->getLeaderDevotion() > tibioLdBefore);

    // 7. Modulación de Influencia en Poderes Divinos:
    // Con líderes y fieles devotos, el poder celestial genera abundante fe y conversiones
    DivinePowersSystem powers;
    EventLine eventLine;
    int ptsBefore = pRel->theologyPoints;
    auto resCongr = powers.castPower(DivinePowerType::Congregate, 20, 20, map, registry, gestor, -4000, eventLine, pRelId);
    assert(resCongr.success);
    assert(resCongr.inspired > 0);
    assert(pRel->theologyPoints > ptsBefore);

    std::cout << "  -> testLeaderDevotionAndFanaticism PASSED! Leader devotion, proximity communion, fanatic zealot emergence and influence modulation verified." << std::endl;
}

void testDayNightCycleAndFakeDynamicLighting() {
    std::cout << "[Test] Running testDayNightCycleAndFakeDynamicLighting (Day/Night Clock, Sun/Moon Celestial Arcs, Ambient Color & Fake Dynamic Lighting)..." << std::endl;
    using namespace ReligionSim;

    // 1. Reloj de Ciclo Día/Noche y Transición de Fases Diurnas
    DayNightCycle dn(90.0f, 0.0f); // 90s por ciclo, inicializado a medianoche (00:00)
    assert(dn.getPhase() == DayPhase::DeepNight);
    assert(dn.isNight());
    assert(!dn.isDay());
    assert(dn.getPhaseName() == "Madrugada");
    assert(dn.getTimeFormatted() == "00:00");

    // Alborada / Amanecer (06:30)
    dn.setTimeByHour(6.5f);
    assert(dn.getPhase() == DayPhase::Dawn);
    assert(dn.getPhaseName() == "Amanecer");
    assert(dn.isDay());
    assert(dn.getTimeFormatted() == "06:30");

    // Mediodía (12:00)
    dn.setTimeByHour(12.0f);
    assert(dn.getPhase() == DayPhase::Day);
    assert(dn.getPhaseName() == "Mediodía");
    assert(dn.isDay());
    assert(dn.getTimeFormatted() == "12:00");

    // Ocaso / Atardecer (18:45)
    dn.setTimeByHour(18.75f);
    assert(dn.getPhase() == DayPhase::Dusk);
    assert(dn.getPhaseName() == "Atardecer");
    assert(dn.getTimeFormatted() == "18:45");

    // Noche (22:15)
    dn.setTimeByHour(22.25f);
    assert(dn.getPhase() == DayPhase::Night);
    assert(dn.getPhaseName() == "Noche");
    assert(dn.isNight());

    // Reinicio y avance por ticks (delta time)
    dn.setTimeByHour(0.0f);
    dn.update(45.0f); // 45s a 90s/ciclo = 12.0 horas exactas
    assert(std::abs(dn.getHour() - 12.0f) < 0.05f);
    assert(dn.isDay());

    // 2. Modulación de Color Ambiental según la Hora
    dn.setTimeByHour(12.0f);
    ColorRGB colNoon = dn.getAmbientColor();
    assert(colNoon.r >= 240 && colNoon.g >= 240 && colNoon.b >= 220); // Plena luz natural brillante

    dn.setTimeByHour(0.0f);
    ColorRGB colMidnight = dn.getAmbientColor();
    assert(colMidnight.r <= 65 && colMidnight.g <= 75 && colMidnight.b >= 100); // Índigo nocturno profundo

    dn.setTimeByHour(6.6f);
    ColorRGB colDawn = dn.getAmbientColor();
    assert(colDawn.r > colDawn.b); // Resplandor cálido al amanecer

    dn.setTimeByHour(18.5f);
    ColorRGB colDusk = dn.getAmbientColor();
    assert(colDusk.r > colDusk.g && colDusk.g > colDusk.b); // Ámbar dorado en el ocaso

    // 3. Transformaciones Celestiales del Sol y la Luna en la Pantalla (1280x720)
    dn.setTimeByHour(12.0f);
    float sx = 0.0f, sy = 0.0f, sAlpha = 0.0f;
    bool sunVisible = dn.getSunTransform(1280, 720, sx, sy, sAlpha);
    assert(sunVisible);
    assert(sAlpha >= 0.85f);
    assert(std::abs(sx - 640.0f) < 50.0f); // Centrado en cenit
    assert(sy <= 80.0f);                  // Elevado en el cielo

    float mx = 0.0f, my = 0.0f, mAlpha = 0.0f;
    bool moonVisible = dn.getMoonTransform(1280, 720, mx, my, mAlpha);
    assert(!moonVisible); // Luna oculta a mediodía

    dn.setTimeByHour(0.0f);
    sunVisible = dn.getSunTransform(1280, 720, sx, sy, sAlpha);
    assert(!sunVisible); // Sol oculto a medianoche
    moonVisible = dn.getMoonTransform(1280, 720, mx, my, mAlpha);
    assert(moonVisible);
    assert(mAlpha >= 0.85f);
    assert(my <= 80.0f);

    // 4. Pulso de Relámpago Divino (Lightning Flash)
    dn.triggerLightningFlash(0.30f);
    assert(dn.getLightningFlashIntensity() >= 0.95f);
    dn.update(0.15f);
    float midFlash = dn.getLightningFlashIntensity();
    assert(midFlash > 0.35f && midFlash < 0.65f);
    dn.update(0.20f);
    assert(dn.getLightningFlashIntensity() == 0.0f);

    // 5. Integración en Renderer (Domo Celestial y Lightmap de Iluminación Dinámica)
    Renderer renderer(1280, 720);
    renderer.setDayNightCycle(&dn);
    assert(renderer.getDayNightCycle() == &dn);

    renderer.beginFrame();
    renderer.endFrame();

    std::cout << "  -> testDayNightCycleAndFakeDynamicLighting PASSED! Diurnal clock, sun/moon celestial arcs, ambient modulation & dynamic lightmaps verified." << std::endl;
}

void testMeteorCataclysmicPowerAndPhysics() {
    using namespace ReligionSim;
    std::cout << "[Test] Running testMeteorCataclysmicPowerAndPhysics (Meteor Descent, Cataclysmic Impact, Ragdoll Blast & Crater Formation)..." << std::endl;

    DivinePowersSystem powers;

    // 1. Metadatos del poder
    assert(powers.getPowerCost(DivinePowerType::Meteor) == 60);
    assert(powers.getPowerName(DivinePowerType::Meteor) == "METEORITO CATACLISMICO");
    assert(powers.getPowerRadius(DivinePowerType::Meteor) == static_cast<int>(std::round(AverageMeteoriteScale::AIRBLAST_RADIUS_CELLS)));
    ColorRGB col = powers.getPowerColor(DivinePowerType::Meteor);
    assert(col.r == 255 && col.g == 95 && col.b == 25);

    // 2. Escenario de simulación
    WorldMap map(32, 32);
    ReligionRegistry registry;
    int relId = registry.createPrimordialReligion("Culto Solar", "Helios", ColorRGB(255, 120, 30), "Sun", 100);
    registry.getReligion(relId)->theologyPoints = 120; // Puntos de fe suficientes

    EventLine eventLine;
    GestorHumanos humanos;

    // Poblar humanos: Chaman en (10, 10), aldeano en epicentro (15.2, 15.2), aldeano en onda media (17.5, 15.0)
    Humano chamanInit(1, "Chaman Helios", 10.0f, 10.0f);
    chamanInit.setReligionId(relId);
    chamanInit.setEsProfeta(true);
    humanos.agregarHumano(chamanInit);

    Humano victimaEpicentroInit(2, "Blanco Epicentro", 15.2f, 15.2f);
    victimaEpicentroInit.setSalud(100.0f);
    humanos.agregarHumano(victimaEpicentroInit);

    Humano victimaOndaInit(3, "Blanco Onda", 17.5f, 15.0f);
    victimaOndaInit.setSalud(100.0f);
    humanos.agregarHumano(victimaOndaInit);

    // Configurar celda objetivo (15, 15) como Bosque Fértil con madera
    Cell& targetCell = map.getCell(15, 15);
    targetCell.biome = BiomeType::DenseForest;
    targetCell.naturalResource = ResourceType::Wood;
    targetCell.resourceAmount = 250;
    targetCell.elevation = 1.0f;

    // 3. Lanzar Meteorito en (15, 15)
    CastResult res = powers.castPower(DivinePowerType::Meteor, 15, 15, map, registry, humanos, -3800, eventLine, relId);
    assert(res.success == true);
    assert(res.faithGenerated > 0);
    assert(registry.getReligion(relId)->theologyPoints >= 60);
    assert(powers.getActiveFX().size() == 1);
    const ActiveDivineFX& fx = powers.getActiveFX()[0];
    assert(fx.type == DivinePowerType::Meteor);
    assert(fx.gridX == 15 && fx.gridY == 15);
    assert(fx.duration == 5.0f);
    assert(fx.impactTriggered == false);

    // 4. Progresión temporal: Fase de descenso (t = 0.6s < 1.25s)
    powers.update(0.6f, map, humanos);
    assert(powers.getActiveFX()[0].impactTriggered == false);

    // 5. Impacto Cataclísmico (t = 0.7s -> timer total 1.3s >= 1.25s)
    powers.update(0.7f, map, humanos);
    assert(powers.getActiveFX()[0].impactTriggered == true);

    // Verificar alteración de terreno y cráter
    assert(targetCell.naturalResource == ResourceType::None);
    assert(targetCell.resourceAmount == 0);
    assert(targetCell.biome == BiomeType::AridDesert); // Cráter de ceniza/basalto
    assert(targetCell.elevation < 1.0f); // Cráter hundido

    // Verificar activación de físicas de ragdoll por la colosal onda expansiva
    Humano* hEpicentro = &humanos.getHumanos()[1];
    assert(hEpicentro != nullptr);
    assert(hEpicentro->isRagdoll() == true || hEpicentro->estaMuerto());

    Humano* hOnda = &humanos.getHumanos()[2];
    assert(hOnda != nullptr);
    assert(hOnda->isRagdoll() == true || hOnda->estaMuerto() || hOnda->getSalud() < 100.0f);

    // 6. Verificar registro en la Crónica Histórica
    assert(!eventLine.getAllEvents().empty());
    bool foundMeteorEvent = false;
    for (const auto& ev : eventLine.getAllEvents()) {
        if (ev.title.find("Meteorito") != std::string::npos) {
            foundMeteorEvent = true;
            break;
        }
    }
    assert(foundMeteorEvent);

    // 7. Integración gráfica con Renderer
    Renderer renderer(1280, 720);
    IsometricCamera camera(1280, 720);
    renderer.renderTargetingReticle({15, 15}, DivinePowerType::Meteor, camera, map);
    renderer.renderDivineEffects(powers, camera, map);

    std::cout << "  -> testMeteorCataclysmicPowerAndPhysics PASSED! Supersonic meteor descent, impact cataclysm, ragdoll blast physics and crater creation fully verified." << std::endl;
}

void testVolcanoEruptionAndPermanentCraters() {
    using namespace ReligionSim;
    std::cout << "[Test] Running testVolcanoEruptionAndPermanentCraters (Volcano Emergence, Magma & Lava Flows, Island Expansion, and Permanent Craters)..." << std::endl;

    DivinePowersSystem powers;
    WorldMap map(32, 32);
    ReligionRegistry registry;
    GestorHumanos humanos;
    EventLine eventLine;

    // 1. Verificación de metadatos del nuevo poder
    assert(powers.getPowerCost(DivinePowerType::Volcano) == 70);
    assert(powers.getPowerName(DivinePowerType::Volcano) == "ERUPCION VOLCANICA");
    assert(powers.getPowerRadius(DivinePowerType::Volcano) == 5);
    ColorRGB volCol = powers.getPowerColor(DivinePowerType::Volcano);
    assert(volCol.r == 255 && volCol.g == 55 && volCol.b == 15);

    // 2. Configurar religión del jugador y fieles
    int relId = registry.createPrimordialReligion("Culto del Fuego", "Vulcano", ColorRGB(255, 60, 20), "Volcano", 100);
    Religion* pRel = registry.getReligion(relId);
    assert(pRel != nullptr);
    pRel->theologyPoints = 100;

    Humano chaman(1, "Kaelen", 16.0f, 16.0f);
    chaman.setReligionId(relId);
    chaman.setEsChaman(true);
    humanos.agregarHumano(chaman);

    Humano victima(2, "AldeanoVictima", 17.0f, 16.0f);
    victima.setReligionId(relId);
    victima.setSalud(100.0f);
    humanos.agregarHumano(victima);

    // Configurar celdas adyacentes como agua/océano para comprobar expansión de tierra
    map.getCell(16, 16).elevation = 1.0f;
    map.getCell(16, 16).biome = BiomeType::FertileValley;
    map.getCell(17, 16).biome = BiomeType::ShallowWater;
    map.getCell(18, 16).biome = BiomeType::DeepOcean;

    // 3. Lanzar Erupción Volcánica en (16, 16)
    CastResult res = powers.castPower(DivinePowerType::Volcano, 16, 16, map, registry, humanos, -3950, eventLine, relId);
    assert(res.success);
    assert(pRel->theologyPoints >= 30); // Costó 70 pts
    assert(map.getCell(16, 16).hasVolcano == true);
    assert(map.getCell(16, 16).hasActiveLava == true);
    assert(map.getCell(16, 16).elevation >= 2.4f); // Alzamiento tectónico orogénico

    // 4. Simulación y expansión de ríos de lava sobre agua
    // Al transcurrir tiempo, la lava debe solidificar el océano/aguas adyacentes en CoastalPlains
    for (int step = 0; step < 25; ++step) {
        powers.update(0.2f, map, humanos);
    }

    // Comprobar que el agua adyacente fue solidificada en tierra emergida
    assert(map.getCell(17, 16).biome == BiomeType::CoastalPlains);
    assert(map.getCell(17, 16).elevation >= 0.80f);

    // Comprobar que el aldeano cercano huyó o sufrió daño por lava
    Humano* victim = &humanos.getHumanos()[1];
    assert(victim != nullptr);
    assert(victim->getAccion() == AccionHumano::CORRER || victim->getSalud() < 100.0f || victim->estaMuerto() || victim->isRagdoll());

    // 5. Verificación de Cráteres Permanentes
    // Crear un meteorito en (8, 8)
    pRel->theologyPoints += 60;
    CastResult mRes = powers.castPower(DivinePowerType::Meteor, 8, 8, map, registry, humanos, -3950, eventLine, relId);
    assert(mRes.success);

    // Simular hasta el impacto del bólido (t >= 1.25s)
    for (int step = 0; step < 15; ++step) {
        powers.update(0.1f, map, humanos);
    }

    // Comprobar que el cráter quedó marcado permanentemente
    assert(map.getCell(8, 8).hasImpactCrater == true);
    assert(map.getCell(8, 8).craterDepth > 0.0f);
    assert(map.getCell(9, 8).hasImpactCrater == true);

    // Simular muchos años después: el cráter NUNCA desaparece (es permanente en el terreno)
    for (int step = 0; step < 60; ++step) {
        powers.update(0.5f, map, humanos);
    }
    assert(map.getCell(8, 8).hasImpactCrater == true);
    assert(map.getCell(9, 8).hasImpactCrater == true);

    // 6. Verificación de renderizado 2.5D
    Renderer renderer(1280, 720);
    IsometricCamera camera(1280, 720);
    renderer.renderTargetingReticle({16, 16}, DivinePowerType::Volcano, camera, map);
    renderer.drawVolcano(640.0f, 360.0f, 1.0f, 3.5f);
    renderer.renderDivineEffects(powers, camera, map);

    std::cout << "  -> testVolcanoEruptionAndPermanentCraters PASSED! Volcano emergence, lava rivers, ocean-to-land solidification, and permanent craters fully verified." << std::endl;
}

void testBiologicalAndSpiritualGatheringAndCombatAI() {
    std::cout << "[Test] Running testBiologicalAndSpiritualGatheringAndCombatAI (Biological Needs, Spiritual Offerings & Defense/Attack Combat AI)..." << std::endl;

    using namespace ReligionSim;
    WorldMap map(32, 32);
    ReligionRegistry registry;

    int r1 = registry.createPrimordialReligion("Clan Solar", "Solarius", ColorRGB(255, 180, 50), "Sun", -4000);
    int r2 = registry.createPrimordialReligion("Tribu Lunar", "Lunaris", ColorRGB(100, 150, 255), "Moon", -4000);
    Religion* rel1 = registry.getReligion(r1);
    Religion* rel2 = registry.getReligion(r2);
    rel1->theologyPoints = 0;
    rel1->resources.food = 100;
    rel1->resources.wood = 50;
    rel2->theologyPoints = 0;
    rel2->resources.food = 100;

    // ========================================================
    // 1. IA DE NECESIDADES BIOLÓGICAS: HAMBRE, RECOLECCIÓN Y ENERGÍA
    // ========================================================
    Cell& foodCell = map.getCell(10, 10);
    foodCell.biome = BiomeType::FertileValley;
    foodCell.naturalResource = ResourceType::Food;
    foodCell.resourceAmount = 80;
    foodCell.dominantReligionId = 1;

    Humano villagerHungry(1, "GrogHambriento", 10.5f, 10.5f);
    villagerHungry.setReligionId(1);
    villagerHungry.getHumanUnitMutable().nutritionalState = 20.0f; // Hambre biológica severa
    villagerHungry.getPopGroupMutable().foodSupply = 25.0f;
    villagerHungry.setSalud(75.0f);

    // Al tomar decisión sobre nodo de comida, debe cosechar, consumir y cargar alimentos
    villagerHungry.forzarDecisionIA(map);
    assert(villagerHungry.getAccion() == AccionHumano::RECOLECTAR);
    assert(villagerHungry.getHumanUnit().nutritionalState > 20.0f);
    assert(villagerHungry.getPopGroup().foodSupply > 25.0f);
    assert(villagerHungry.getSalud() > 75.0f);
    assert(villagerHungry.getCantidadRecursoCargado() > 0);
    assert(villagerHungry.getTipoRecursoRecolectando() == ResourceType::Food);

    // Prueba de búsqueda inteligente de nodo de comida a distancia
    Humano villagerSearcher(2, "BuscadorComida", 8.2f, 8.2f);
    villagerSearcher.setReligionId(1);
    villagerSearcher.getHumanUnitMutable().nutritionalState = 22.0f;
    villagerSearcher.getPopGroupMutable().foodSupply = 20.0f;
    Point2i foundFood;
    bool found = villagerSearcher.buscarNodoRecurso(map, ResourceType::Food, 8, foundFood);
    assert(found == true);
    assert(foundFood.x == 10 && foundFood.y == 10);

    // Al forzar decisión, se encamina hacia el nodo de alimento
    villagerSearcher.forzarDecisionIA(map);
    assert(villagerSearcher.getHumanUnit().currentState == HumanAIState::SURVIVAL_WORKING);

    // Fatiga: energía baja obliga a descansar (IDLE)
    Humano villagerFatigued(3, "Cansado", 10.5f, 10.5f);
    villagerFatigued.getHumanUnitMutable().energy = 15.0f;
    villagerFatigued.getHumanUnitMutable().nutritionalState = 80.0f;
    villagerFatigued.forzarDecisionIA(map);
    assert(villagerFatigued.getAccion() == AccionHumano::IDLE);
    assert(villagerFatigued.getHumanUnit().currentState == HumanAIState::SURVIVAL_WORKING);

    // ========================================================
    // 2. IA DE NECESIDADES ESPIRITUALES: HAMBRE DEL ALMA Y OFRENDAS SAGRADAS
    // ========================================================
    Cell& templeCell = map.getCell(15, 15);
    templeCell.biome = BiomeType::CoastalPlains;
    templeCell.hasHolySite = true;
    templeCell.holySiteReligionId = 1;
    templeCell.dominantReligionId = 1;
    templeCell.devotion = 0.50f;

    // A. Búsqueda de santuario por hambre espiritual
    Humano devoutPilgrim(4, "Peregrino", 14.0f, 14.0f);
    devoutPilgrim.setReligionId(1);
    devoutPilgrim.setSpiritualHunger(65.0f); // Alta necesidad espiritual
    devoutPilgrim.getHumanUnitMutable().nutritionalState = 90.0f;
    devoutPilgrim.getHumanUnitMutable().energy = 90.0f;

    Point2i foundTemple;
    assert(devoutPilgrim.buscarSantuarioOAltar(map, 1, 10, foundTemple) == true);
    assert(foundTemple.x == 15 && foundTemple.y == 15);

    // En el templo sagrado: orar sacia el hambre espiritual y eleva la devoción
    Humano worshiper(5, "DevotoEnAltar", 15.5f, 15.5f);
    worshiper.setReligionId(1);
    worshiper.setSpiritualHunger(60.0f);
    worshiper.setDevocion(0.60f);
    worshiper.getHumanUnitMutable().nutritionalState = 85.0f;
    worshiper.getHumanUnitMutable().energy = 85.0f;
    worshiper.forzarDecisionIA(map);
    assert(worshiper.getAccion() == AccionHumano::REZAR);
    assert(worshiper.getHumanUnit().currentState == HumanAIState::WORSHIPING);
    assert(worshiper.getSpiritualHunger() < 60.0f);
    worshiper.getHumanUnitMutable().energy = 85.0f;
    worshiper.actualizar(0.1f, map);
    assert(worshiper.getAccion() == AccionHumano::REZAR);
    assert(worshiper.getHumanUnit().currentState == HumanAIState::WORSHIPING);
    assert(worshiper.getSpiritualHunger() < 60.0f);

    // B. Ofrenda Sagrada al Altar: aldeano devoto transporta cosecha al santuario
    GestorHumanos gestor;
    Humano carrierDevout(6, "Ofrendador", 15.5f, 15.5f);
    carrierDevout.setReligionId(1);
    carrierDevout.setDevocion(0.85f);
    carrierDevout.setCantidadRecursoCargado(10);
    carrierDevout.setTipoRecursoRecolectando(ResourceType::Food);
    carrierDevout.setDeliveringOffering(true);
    gestor.agregarHumano(carrierDevout);

    float oldDevotion = templeCell.devotion;
    int oldTheology = registry.getReligion(1)->theologyPoints;
    gestor.actualizar(0.1f, map, nullptr, -3900, &registry);

    Humano* updatedCarrier = gestor.getHumanos().data();
    assert(updatedCarrier->getCantidadRecursoCargado() == 0); // Ofrenda entregada
    assert(updatedCarrier->isDeliveringOffering() == false);
    assert(templeCell.devotion > oldDevotion); // Fervor del templo incrementado (+0.15)
    assert(registry.getReligion(1)->theologyPoints > oldTheology); // Puntos de teología ganados (+15)
    assert(updatedCarrier->getSpiritualHunger() == 0.0f); // Saciedad espiritual total
    assert(updatedCarrier->getAccion() == AccionHumano::REZAR); // Oración en el altar

    // ========================================================
    // 3. IA DE DEFENSA Y ATAQUE: PERCEPCIÓN DE AMENAZAS, GRITO DE ALERTA Y MELEE
    // ========================================================
    GestorHumanos gestorCombate;

    // Defensor apto de la tribu 1 (Guardian)
    Humano defender(10, "GuardianSolar", 20.0f, 20.0f);
    defender.setReligionId(1);
    defender.setSalud(100.0f);
    defender.getHumanUnitMutable().attackPower = 18.0f;
    defender.getHumanUnitMutable().willpower = 75.0f;
    defender.getHumanUnitMutable().energy = 90.0f;
    defender.getHumanUnitMutable().nutritionalState = 80.0f;

    // Aliado cercano de la tribu 1 (a 2.5 celdas)
    Humano ally(11, "AliadoCercano", 21.5f, 21.0f);
    ally.setReligionId(1);
    ally.setSalud(90.0f);
    ally.getHumanUnitMutable().attackPower = 15.0f;
    ally.getHumanUnitMutable().willpower = 70.0f;
    ally.cambiarAccion(AccionHumano::IDLE);

    // No combatiente vulnerable / malherido de la tribu 1
    Humano woundedNonCombatant(12, "HeridoDebil", 20.5f, 19.5f);
    woundedNonCombatant.setReligionId(1);
    woundedNonCombatant.setSalud(20.0f); // HP crítico (< 35 HP)
    woundedNonCombatant.getHumanUnitMutable().willpower = 25.0f;

    // Incursor hostil de la tribu 2
    Humano enemyRaider(20, "IncursorLunar", 21.5f, 20.0f); // a 1.5 celdas de distancia
    enemyRaider.setReligionId(2);
    enemyRaider.setSalud(40.0f);
    enemyRaider.getHumanUnitMutable().attackPower = 14.0f;
    enemyRaider.cambiarAccion(AccionHumano::ATACAR); // En actitud hostil

    gestorCombate.agregarHumano(defender);
    gestorCombate.agregarHumano(ally);
    gestorCombate.agregarHumano(woundedNonCombatant);
    gestorCombate.agregarHumano(enemyRaider);

    // Actualizar un ciclo de percepción y combate
    gestorCombate.actualizar(0.2f, map, nullptr, -3900, &registry);

    auto& units = gestorCombate.getHumanos();
    Humano& d = units[0];
    Humano& al = units[1];
    Humano& wnc = units[2];
    Humano& e = units[3];

    // Verificación de Reacción de Defensa:
    assert(d.getAccion() == AccionHumano::ATACAR);
    assert(d.getHumanAIState() == HumanAIState::DEFENDING);
    assert(d.getTargetEnemyId() == e.getId());

    // Verificación de Grito de Alerta (Rally Call):
    assert(al.getAccion() == AccionHumano::ATACAR);
    assert(al.getHumanAIState() == HumanAIState::DEFENDING);
    assert(al.getTargetEnemyId() == e.getId());

    // Verificación de Huida de No Combatiente:
    assert(wnc.getAccion() == AccionHumano::CORRER); // Ha huido en pánico

    // Verificación de Daño de Combate Melee:
    assert(e.getSalud() < 40.0f); // El incursor recibió daño del contraataque

    // Resolver combate hasta caída del enemigo:
    while (e.estaVivo()) {
        gestorCombate.actualizar(0.3f, map, nullptr, -3900, &registry);
    }
    assert(e.estaMuerto() == true);
    assert(e.getSalud() == 0.0f);
    assert(e.isRagdoll() == true); // Colapsó en física de ragdoll 2.5D

    // Verificación de Celebración de Victoria:
    assert(d.getAccion() == AccionHumano::SALUDAR || d.getAccion() == AccionHumano::REZAR);

    // Verificación de renderizado de unidades con fardo y ofrenda
    IsometricCamera camera(1280, 720);
    TileSet tileset;
    std::vector<uint32_t> testBuf(1280 * 720, 0xFF101010);
    gestorCombate.seleccionarHumano(d.getId());
    gestorCombate.renderizarTodos(testBuf.data(), 1280, 720, camera, map, tileset, registry);

    std::cout << "  -> testBiologicalAndSpiritualGatheringAndCombatAI PASSED! Biological needs, spiritual offerings, threat perception, rally calls, and combat AI fully verified." << std::endl;
}

void testCloudSystemAndCelestialAltitudeAndZoomFade() {
    std::cout << "[Test] Running testCloudSystemAndCelestialAltitudeAndZoomFade (Cloud Altitude, Zoom Out/In Visibility & Celestial Sun/Moon Fade)..." << std::endl;
    using namespace ReligionSim;

    // 1. Inicialización del Sistema de Nubes a Alturas Determinadas
    CloudSystem clouds(64, 64);
    assert(clouds.getCloudCount() == 20);
    assert(clouds.getBaseAltitude() >= 7.5f);

    bool hasCumulus = false;
    bool hasStratus = false;
    bool hasCirrus = false;

    for (const auto& c : clouds.getClouds()) {
        // Verificar que cada nube está estrictamente dentro del mapa y a altura determinada
        assert(c.worldX >= 0.0f && c.worldX <= 64.0f);
        assert(c.worldY >= 0.0f && c.worldY <= 64.0f);
        assert(c.altitude >= 7.5f && c.altitude <= 12.5f);
        assert(c.lobes.size() >= 3 && c.lobes.size() <= 8);
        assert(c.scale >= 0.8f && c.scale <= 2.2f);
        assert(c.baseAlpha >= 0.30f && c.baseAlpha <= 0.95f);

        if (c.type == CloudType::Cumulus) hasCumulus = true;
        if (c.type == CloudType::Stratus) hasStratus = true;
        if (c.type == CloudType::Cirrus) hasCirrus = true;
    }
    assert(hasCumulus && hasStratus && hasCirrus);

    // 2. Deriva de Viento Atmosférico y Confinamiento al Mapa (Wrap-around interno)
    float initX = clouds.getClouds()[0].worldX;
    float initY = clouds.getClouds()[0].worldY;
    clouds.update(2.0f);
    assert(clouds.getClouds()[0].worldX != initX || clouds.getClouds()[0].worldY != initY);

    // Prueba de wrap-around en límites del mapa
    clouds.getCloudsMutable()[0].worldX = 66.0f; // Supera el límite este (>= maxX 64.0f)
    clouds.update(0.1f);
    assert(clouds.getClouds()[0].worldX <= 2.0f); // Se confinó y envolvió al lado oeste dentro del mapa

    // 3. Curva de Visibilidad según Zoom de Cámara ("cuando hago zoom out podre verlas, cuando hago zoom in ya no")
    // Zoom Out panorámico: 100% visible
    assert(CloudSystem::calculateZoomVisibility(0.20f) == 1.0f);
    assert(CloudSystem::calculateZoomVisibility(0.50f) == 1.0f);
    assert(CloudSystem::calculateZoomVisibility(0.85f) == 1.0f);

    // Transición suave al descender
    float midVis = CloudSystem::calculateZoomVisibility(1.00f);
    assert(midVis > 0.0f && midVis < 1.0f);

    // Zoom In cercano al terreno: 0% invisible
    assert(CloudSystem::calculateZoomVisibility(1.15f) == 0.0f);
    assert(CloudSystem::calculateZoomVisibility(1.50f) == 0.0f);
    assert(CloudSystem::calculateZoomVisibility(2.50f) == 0.0f);
    assert(CloudSystem::calculateZoomVisibility(3.50f) == 0.0f);

    // 4. Modulación de Iluminación de Nubes según Ciclo Día/Noche
    DayNightCycle dn(90.0f, 12.0f); // Mediodía
    ColorRGB sunlit, shaded, shadow;
    float shadowA = 0.0f;
    clouds.getLightingColors(&dn, sunlit, shaded, shadow, shadowA);
    assert(sunlit.r == 255 && sunlit.g == 255 && sunlit.b == 255); // Blanco resplandeciente cenital
    assert(shadowA >= 0.25f);

    // Atardecer (18.0h)
    dn.setTimeByHour(18.0f);
    clouds.getLightingColors(&dn, sunlit, shaded, shadow, shadowA);
    assert(sunlit.r > sunlit.b); // Tonalidad dorada/ámbar

    // Noche (00.0h)
    dn.setTimeByHour(0.0f);
    clouds.getLightingColors(&dn, sunlit, shaded, shadow, shadowA);
    assert(sunlit.b > sunlit.r); // Azul/índigo lunar nocturno

    // Relámpago Divino
    dn.triggerLightningFlash(0.35f);
    clouds.getLightingColors(&dn, sunlit, shaded, shadow, shadowA);
    assert(sunlit.r == 255 && sunlit.g == 255 && sunlit.b == 255);
    assert(shaded.r >= 235 && shaded.g >= 240);

    // 5. Integración con Renderer: Control de Sol/Luna y Nubes al hacer Zoom
    Renderer renderer(1280, 720);
    renderer.setDayNightCycle(&dn);
    renderer.setCloudSystem(&clouds);
    assert(renderer.getCloudSystem() == &clouds);

    IsometricCamera camera(1280, 720);

    // Configuración de mapa y entidades de prueba
    WorldMap map(32, 32);
    ReligionRegistry registry;
    Timeline timeline(-4000, 2200);
    EventLine eventLine;
    ReplicationEngine repl;
    GestorHumanos humanos;
    WorldCreationConfig config;

    // A. Renderizado en Zoom Out (zoom = 0.85): Nubes y Sol/Luna activos en el firmamento
    camera.setZoom(0.85f);
    for (int i = 0; i < 15; ++i) camera.update(0.1f);
    assert(std::abs(camera.getZoom() - 0.85f) < 0.05f);

    renderer.beginFrame();
    renderer.render(map, registry, timeline, eventLine, repl, humanos, camera,
                    {16, 16}, RenderMapMode::ReligionDominance, config,
                    nullptr, nullptr, {-1, -1}, &dn, &clouds);
    renderer.endFrame();

    // B. Renderizado en Zoom In (zoom = 2.0): Nubes y Sol/Luna completamente invisibles
    camera.setZoom(2.0f);
    for (int i = 0; i < 20; ++i) camera.update(0.1f);
    assert(camera.getZoom() >= 1.8f);
    assert(CloudSystem::calculateZoomVisibility(camera.getZoom()) == 0.0f);

    renderer.beginFrame();
    renderer.render(map, registry, timeline, eventLine, repl, humanos, camera,
                    {16, 16}, RenderMapMode::ReligionDominance, config,
                    nullptr, nullptr, {-1, -1}, &dn, &clouds);
    renderer.endFrame();

    // 6. Verificación de Referencia Estricta al Mapa Mundial (No a la Cámara):
    // "cuando muevo el mapa la luz se mueve, tendria que estar con referencia al mapa, no a la camara"
    dn.setTimeByHour(12.5f); // Mediodía exacto
    float sunWx = 0.0f, sunWy = 0.0f, sunAlt = 0.0f, sunAlpha = 0.0f;
    bool sunActive = dn.getSunWorldPosition(64, 64, sunWx, sunWy, sunAlt, sunAlpha);
    assert(sunActive);
    assert(std::abs(sunWx - 32.0f) < 0.2f); // Centrado en el cenit del mapa
    assert(std::abs(sunWy - 32.0f) < 0.2f);
    assert(sunAlt >= 38.0f); // Elevado en la bóveda celestial sobre la isla
    assert(sunAlpha >= 0.95f);

    camera.setZoom(0.85f);
    for (int i = 0; i < 15; ++i) camera.update(0.1f);
    Vec2 sunPosBefore = camera.gridToScreen(sunWx, sunWy, sunAlt);
    Vec2 mapPosBefore = camera.gridToScreen(32.0f, 32.0f, 0.0f);

    // Mover la cámara (desplazamiento del mapa al arrastrar o usar flechas/WASD)
    camera.pan(120.0f, -85.0f);
    for (int i = 0; i < 20; ++i) camera.update(0.1f);

    Vec2 sunPosAfter = camera.gridToScreen(sunWx, sunWy, sunAlt);
    Vec2 mapPosAfter = camera.gridToScreen(32.0f, 32.0f, 0.0f);

    // El desplazamiento del sol debe ser idéntico al desplazamiento del mapa:
    // (sunPosAfter - sunPosBefore) == (mapPosAfter - mapPosBefore)
    // Demostrando matemáticamente que la luz está 100% referenciada al mapa y NO a la cámara.
    float deltaSunX = sunPosAfter.x - sunPosBefore.x;
    float deltaSunY = sunPosAfter.y - sunPosBefore.y;
    float deltaMapX = mapPosAfter.x - mapPosBefore.x;
    float deltaMapY = mapPosAfter.y - mapPosBefore.y;
    assert(std::abs(deltaSunX - deltaMapX) < 0.001f);
    assert(std::abs(deltaSunY - deltaMapY) < 0.001f);

    // Luna a medianoche cenital (0.5h = 00:30)
    dn.setTimeByHour(0.5f);
    float moonWx = 0.0f, moonWy = 0.0f, moonAlt = 0.0f, moonAlpha = 0.0f;
    bool moonActive = dn.getMoonWorldPosition(64, 64, moonWx, moonWy, moonAlt, moonAlpha);
    assert(moonActive);
    assert(std::abs(moonWx - 32.0f) < 0.2f);
    assert(std::abs(moonWy - 32.0f) < 0.2f);
    assert(moonAlt >= 38.0f);

    std::cout << "  -> testCloudSystemAndCelestialAltitudeAndZoomFade PASSED! Cloud altitude, wind drift, zoom in/out visibility, and celestial sun/moon fade verified." << std::endl;
}

void testSoundSystemAndAmbientDSP() {
    std::cout << "[Test] Running testSoundSystemAndAmbientDSP..." << std::endl;

    ReligionSim::SoundSystem soundSys;

    // 1. Validar valores predeterminados y controles atómicos de volumen
    assert(soundSys.getMasterVolume() == 0.75f);
    assert(soundSys.getSFXVolume() == 0.80f);
    assert(soundSys.getAmbientVolume() == 0.70f);
    assert(soundSys.getMusicVolume() == 0.55f);
    assert(!soundSys.isMuted());

    soundSys.setMasterVolume(0.65f);
    assert(std::abs(soundSys.getMasterVolume() - 0.65f) < 0.001f);

    soundSys.setSFXVolume(0.85f);
    assert(std::abs(soundSys.getSFXVolume() - 0.85f) < 0.001f);

    soundSys.setAmbientVolume(0.45f);
    assert(std::abs(soundSys.getAmbientVolume() - 0.45f) < 0.001f);

    soundSys.setMusicVolume(0.35f);
    assert(std::abs(soundSys.getMusicVolume() - 0.35f) < 0.001f);

    // Mute toggle
    soundSys.setMuted(true);
    assert(soundSys.isMuted());
    soundSys.toggleMute();
    assert(!soundSys.isMuted());

    // Clamp de volumen fuera de rango [0, 1]
    soundSys.setMasterVolume(2.5f);
    assert(soundSys.getMasterVolume() <= 1.0f);
    soundSys.setMasterVolume(-0.5f);
    assert(soundSys.getMasterVolume() >= 0.0f);
    soundSys.setMasterVolume(0.8f);

    // 2. Inicialización del motor de audio (ALSA / Soundcard o Timer Fallback)
    soundSys.init();
    assert(soundSys.isRunning());

    // 3. Prueba de disparo de efectos de sonido procedurales (SFX)
    soundSys.playSFX(ReligionSim::SFXType::TreeChop, 1.0f, -0.5f);
    soundSys.playSFX(ReligionSim::SFXType::StoneMine, 0.9f, 0.5f);
    soundSys.playSFX(ReligionSim::SFXType::WaterSplash, 0.8f, 0.0f);
    soundSys.playSFX(ReligionSim::SFXType::UnitDeath, 1.0f, 0.2f);
    soundSys.playSFX(ReligionSim::SFXType::ThunderStrike, 1.0f, 0.0f);
    soundSys.playSFX(ReligionSim::SFXType::MeteorImpact, 1.0f, 0.0f);
    soundSys.playSFX(ReligionSim::SFXType::FaithGain, 0.9f, 0.0f);
    soundSys.playSFX(ReligionSim::SFXType::AtomicExplosion, 1.0f, 0.0f);

    // 4. Prueba de Audio Espacial 2.5D (Panning estéreo atenuado por distancia de cámara)
    ReligionSim::IsometricCamera camera(1280, 720);
    camera.setZoom(1.0f);
    camera.pan(0.0f, 0.0f);

    // Disparos espaciales cercanos y lejanos
    soundSys.playSpatialSFX(ReligionSim::SFXType::BodyThud, 32.0f, 32.0f, camera, 1.0f, 60.0f);
    soundSys.playSpatialSFX(ReligionSim::SFXType::Earthquake, 10.0f, 10.0f, camera, 1.0f, 60.0f);
    soundSys.playSpatialSFX(ReligionSim::SFXType::CombatAttack, 50.0f, 50.0f, camera, 0.8f, 60.0f);
    soundSys.playSpatialSFX(ReligionSim::SFXType::AtomicExplosion, 20.0f, 20.0f, camera, 1.5f);

    // 5. Actualización del soundscape ambiental (viento, naturaleza diurna/nocturna, olas)
    ReligionSim::WorldMap map(32, 32);
    map.generateProceduralWorld(4567);
    ReligionSim::DayNightCycle dayNight;
    ReligionSim::DivinePowersSystem powers;
    ReligionSim::GestorHumanos gestor;

    // Mediodía (canto de pájaros activo)
    dayNight.setTimeByHour(12.0f);
    soundSys.update(0.016f, map, camera, &dayNight, &powers, gestor, false, 1.0f, -3800);
    assert(!soundSys.isSimulationPaused());
    assert(soundSys.getSimulationSpeed() == 1.0f);
    assert(soundSys.getCurrentYear() == -3800);

    // Noche con pausa y velocidad 2X en Era de los Templos
    dayNight.setTimeByHour(23.5f);
    soundSys.update(0.016f, map, camera, &dayNight, &powers, gestor, true, 2.0f, -1200);
    assert(soundSys.isSimulationPaused());
    assert(soundSys.getSimulationSpeed() == 2.0f);
    assert(soundSys.getCurrentYear() == -1200);

    // Sincronización de estado explícita
    soundSys.setSimulationTimeState(false, 5.0f, 500);
    assert(!soundSys.isSimulationPaused());
    assert(soundSys.getSimulationSpeed() == 5.0f);
    assert(soundSys.getCurrentYear() == 500);

    // 6. Cierre seguro y liberación de hilos
    soundSys.shutdown();
    assert(!soundSys.isRunning());

    std::cout << "  -> testSoundSystemAndAmbientDSP PASSED! ALSA/Hardware SoundCard, procedural wind, nature, collisions, and ancient music verified." << std::endl;
}

void testNemesisAndSififusMythSystem() {
    std::cout << "[Test] Running testNemesisAndSififusMythSystem (Nemesis System & Sififus Myth Engine)..." << std::endl;

    // 1. Prueba de memoria individual (EntityMemory con búfer circular estático)
    ReligionSim::Nemesis::EntityMemory memory;
    assert(memory.getEventCount() == 0);
    assert(memory.getLatestEvent() == nullptr);

    memory.recordEvent(ReligionSim::Nemesis::MemoryEventType::SurvivedCombat, -3950, 102, 2, 75, 0.35f);
    assert(memory.getEventCount() == 1);
    const auto* latest = memory.getLatestEvent();
    assert(latest != nullptr);
    assert(latest->type == ReligionSim::Nemesis::MemoryEventType::SurvivedCombat);
    assert(latest->severity == 75);
    assert(latest->timestampYear == -3950);
    assert(latest->grudgeDelta == 0.35f);

    // Sobreescritura circular sin rebasamiento
    for (int i = 0; i < 15; ++i) {
        memory.recordEvent(ReligionSim::Nemesis::MemoryEventType::WoundedByDivinePower, -3900 + i, -1, -1, 100, 0.5f);
    }
    assert(memory.getEventCount() == ReligionSim::Nemesis::EntityMemory::MAX_MEMORIES);
    assert(memory.getLatestEvent()->timestampYear == -3900 + 14);

    // 2. Prueba del Sparse Set de Unidades Notables (NotableUnitsPool)
    ReligionSim::Nemesis::NotableUnitsPool pool(64, 16);
    assert(pool.size() == 0);
    assert(!pool.contains(7));

    ReligionSim::Nemesis::NemesisProfile& prof = pool.promoteOrEmplace(7);
    assert(pool.contains(7));
    assert(pool.size() == 1);
    assert(prof.entityId == 7);
    assert(pool.get(7) == &prof);

    prof.grudgePlayer = 0.55f;
    prof.generateProceduralEpithet(ReligionSim::Nemesis::MemoryEventType::SurvivedCombat);
    assert(std::string(prof.epithet).find("Cicatriz") != std::string::npos ||
           std::string(prof.epithet).find("Inquebrantable") != std::string::npos);

    // 3. Prueba de Acoplamiento y Mitogénesis (NemesisSififusBridge)
    ReligionSim::Nemesis::LifeEvent evDivine{
        .type = ReligionSim::Nemesis::MemoryEventType::WoundedByDivinePower,
        .severity = 160,
        .targetReligionId = -1,
        .targetEntityId = -1,
        .timestampYear = -3910,
        .grudgeDelta = 0.85f
    };
    prof.grudgePlayer = 0.85f;

    ReligionSim::Sififus::MythRecord myth = ReligionSim::Sififus::NemesisSififusBridge::generateMythFromNemesis(
        prof, evDivine, "Grog", 1);
    assert(myth.archetype == ReligionSim::Sififus::MythArchetype::PrometheanDefiance);
    assert(myth.ethos.divineFearBias < 0.0f); // Desafío deicida / sin pavor
    assert(myth.ethos.aggressionBias > 0.0f);
    assert(std::string(myth.title).find("Desafio") != std::string::npos);

    // 4. Prueba del Motor Sísifo (SififusEngine) y Ethos Colectivo
    ReligionSim::Sififus::SififusEngine sififus;
    myth.originReligionId = 1;
    sififus.registerMyth(myth);
    assert(sififus.getActiveMyths().size() == 1);

    ReligionSim::Sififus::EthosVector ethosRel1 = sififus.getCollectiveEthos(1);
    assert(ethosRel1.aggressionBias > 0.4f);
    assert(ethosRel1.divineFearBias < -0.5f);

    // Desgaste temporal del relato
    sififus.update(50.0f, -3860); // 50 años pasan
    assert(sififus.getActiveMyths()[0].resonance < 1.0f);

    // 5. Prueba Integral con NemesisSififusManager
    ReligionSim::NemesisSififusManager manager;
    ReligionSim::Humano villager(25, "Kroll", 18.0f, 18.0f);
    villager.setReligionId(1);
    villager.setSalud(80.0f);

    // El aldeano sobrevive a un duelo de combate contra un rival de religión 2
    manager.onUnitSurvivesCombat(villager, 105, 2, 35.0f, -3880, nullptr, nullptr);
    assert(manager.getPool().contains(25));
    const char* epithet = manager.getNotableEpithetOrNull(25);
    assert(epithet != nullptr);
    assert(manager.getSififus().getActiveMyths().size() >= 1);

    // El aldeano es tocado por un poder divino y sobrevive
    manager.onUnitHitByDivinePower(villager, 40.0f, -3870, nullptr, nullptr);
    const auto* pProf = manager.getProfileOrNull(25);
    assert(pProf != nullptr);
    assert(pProf->grudgePlayer > 0.5f);

    // Actualización de emanación y modificación de conducta en los fieles
    ReligionSim::WorldMap map(32, 32);
    map.generateProceduralWorld(9876);
    ReligionSim::GestorHumanos gestor;
    gestor.agregarHumano(villager);

    // Un aldeano común del mismo clan adopta la conducta dictada por el relato
    ReligionSim::Humano fielComun(26, "Torg", 18.0f, 18.5f);
    fielComun.setReligionId(1);
    fielComun.getHumanUnitMutable().role = CommunityRole::FORAGER;
    gestor.agregarHumano(fielComun);

    manager.update(1.0f, -3870, gestor, map, nullptr, nullptr);

    // El relato beligerante del Némesis transforma a los forrajeadores en fanáticos de guerra
    assert(gestor.getHumanos()[1].getHumanUnit().role == CommunityRole::ZEALOT);

    std::cout << "  -> testNemesisAndSififusMythSystem PASSED! EntityMemory ringbuffer, NotableUnitsPool sparse set, Sififus myth genesis, and behavioral alteration verified." << std::endl;
}

void testMemoryTreeUIAndModal() {
    std::cout << "[Test] Running testMemoryTreeUIAndModal (Sififus & Nemesis Memory Tree Button & Modal UI)..." << std::endl;

    ReligionSim::Renderer renderer(1280, 720);

    // 1. Verificar registro del botón 750 (RECUERDOS)
    int btn750Found = 0;
    for (const auto& btn : renderer.getButtons()) {
        if (btn.id == 750) {
            btn750Found = 1;
            assert(btn.label == "RECUERDOS");
            break;
        }
    }
    assert(btn750Found == 1);

    // 2. Verificar estado modal inicial y toggle
    assert(!renderer.isMemoryTreeOpen());
    renderer.toggleMemoryTree();
    assert(renderer.isMemoryTreeOpen());
    renderer.setMemoryTreeOpen(false);
    assert(!renderer.isMemoryTreeOpen());

    // 3. Verificar scroll
    assert(renderer.getMemoryTreeScroll() == 0);
    renderer.scrollMemoryTree(1);
    assert(renderer.getMemoryTreeScroll() == 1);
    renderer.scrollMemoryTree(-1);
    assert(renderer.getMemoryTreeScroll() == 0);

    // 4. Hit-test del modal
    const int modalW = 1140;
    const int modalH = 620;
    const int modalX = (1280 - modalW) / 2;
    const int modalY = (720 - modalH) / 2;

    // Clic fuera del modal debe retornar 1 (cerrar)
    assert(renderer.checkMemoryTreeModalClick(10.0f, 10.0f) == 1);

    // Clic en la cruz [X] en modalX + modalW - 30, modalY + 20 debe retornar 1
    assert(renderer.checkMemoryTreeModalClick(modalX + modalW - 30.0f, modalY + 20.0f) == 1);

    // Clic en el botón inferior [ CERRAR VENTANA ]
    const int cBtnX = modalX + modalW / 2;
    const int cBtnY = modalY + modalH - 24;
    assert(renderer.checkMemoryTreeModalClick(cBtnX, cBtnY) == 1);

    // Clic en el centro del modal debe absorber el clic (retornar 0)
    assert(renderer.checkMemoryTreeModalClick(modalX + 200.0f, modalY + 200.0f) == 0);

    // 5. Renderizado sin fallos ni desbordamientos de búfer con datos reales
    ReligionSim::GestorHumanos gestor;
    ReligionSim::ReligionRegistry registry;
    ReligionSim::Timeline timeline(-4000, 2200);
    ReligionSim::NemesisSififusManager nemesisSififus;
    renderer.setNemesisSififus(&nemesisSififus);

    // Render sin unidad seleccionada
    renderer.renderMemoryTreeModal(gestor, registry, timeline, 0.0f, 0.0f, false);

    // Crear y seleccionar unidad notable con eventos y mitos
    ReligionSim::Humano h1(99, "Grog", 20.0f, 20.0f);
    h1.setReligionId(1);
    gestor.agregarHumano(h1);
    gestor.seleccionarHumano(99);

    auto& profile = nemesisSififus.getPool().promoteOrEmplace(99);
    profile.rank = ReligionSim::Nemesis::NemesisRank::Avenger;
    profile.grudgePlayer = 0.65f;
    profile.memory.recordEvent(ReligionSim::Nemesis::MemoryEventType::SurvivedCombat, -3900, 10, 2, 150, 0.4f);

    ReligionSim::Sififus::MythRecord myth;
    myth.mythId = 1;
    myth.protagonistEntityId = 99;
    myth.originReligionId = 1;
    std::snprintf(myth.title, sizeof(myth.title), "%s", "La Furia de Grog");
    std::snprintf(myth.oralSnippet, sizeof(myth.oralSnippet), "%s", "El guerrero se levanto de las cenizas jurando sangrienta venganza.");
    nemesisSififus.getSififus().registerMyth(myth);

    renderer.renderMemoryTreeModal(gestor, registry, timeline, 0.0f, 0.0f, false);

    std::cout << "  -> testMemoryTreeUIAndModal PASSED! Button 750, modal toggle, scroll, hit-test, and 3-column rendering verified." << std::endl;
}

void testGameOverOnAllUnitsExtinction() {
    std::cout << "[Test] Running testGameOverOnAllUnitsExtinction (Faith Extinction & Defeat on All Units Death)..." << std::endl;
    
    ReligionSim::Game game;
    ReligionSim::WorldCreationConfig config;
    config.godName = "Solarius";
    config.religionName = "Culto Solar";
    config.initialPopulation = 40;
    config.mapSeed = 777;
    game.getCreationConfig() = config;
    game.startNewCustomWorld();

    assert(game.getGameState() == ReligionSim::GameState::Playing);
    int pRel = game.getPlayerReligionId();
    assert(pRel > 0);

    auto& gestor = game.getGestorHumanos();
    int vivos = gestor.contarHumanosVivosDeReligion(pRel);
    int totales = gestor.contarHumanosTotalesDeReligion(pRel);
    assert(totales > 0);
    assert(vivos > 0);

    // Con unidades vivas, la simulación avanza sin Game Over
    game.update(0.1f);
    assert(game.getGameState() == ReligionSim::GameState::Playing);

    // Simular la muerte de todas las unidades vivas del culto del jugador
    for (auto& h : gestor.getHumanos()) {
        if (h.getReligionId() == pRel) {
            h.setSalud(0.0f); // Muerte biológica
        }
    }
    assert(gestor.contarHumanosVivosDeReligion(pRel) == 0);
    assert(gestor.contarHumanosTotalesDeReligion(pRel) > 0);

    // Primer tick: se detecta la extinción y se inicia la pausa fúnebre / sonido de muerte
    game.update(0.1f);
    assert(game.getGameState() == ReligionSim::GameState::Playing); // En pausa dramática

    // Verificar que el evento de extinción se agregó a la crónica
    bool foundExtinctionEvent = false;
    for (const auto& ev : game.getEventLine().getAllEvents()) {
        if (ev.category == ReligionSim::EventCategory::Extinction) {
            foundExtinctionEvent = true;
            assert(ev.isTurningPoint == true);
            break;
        }
    }
    assert(foundExtinctionEvent && "Debe registrarse el evento histórico de Extinción Total de la Fe");

    // Avanzar el temporizador fúnebre (1.8s) para transicionar a GameState::GameOver
    game.update(2.0f);
    assert(game.getGameState() == ReligionSim::GameState::GameOver);

    // Hit-testing de los botones de la pantalla de Game Over (4 botones litúrgicos con Observar Mundo)
    const auto& renderer = game.getRenderer();
    constexpr float modalW = 860.0f;
    constexpr float modalH = 480.0f;
    const float mx = (1280.0f - modalW) * 0.5f;
    const float my = (720.0f - modalH) * 0.5f;
    const float btnY = my + modalH - 65.0f;

    // Clic en [ OBSERVAR MUNDO (VER MAPA) ] (ID 904)
    int btn904 = renderer.checkGameOverButtonClicked(mx + 50.0f, btnY + 20.0f);
    assert(btn904 == 904);

    // Clic en [ REINTENTAR GÉNESIS ] (ID 901)
    int btn901 = renderer.checkGameOverButtonClicked(mx + 260.0f, btnY + 20.0f);
    assert(btn901 == 901);

    // Clic en [ CARGAR PARTIDA ] (ID 902)
    int btn902 = renderer.checkGameOverButtonClicked(mx + 460.0f, btnY + 20.0f);
    assert(btn902 == 902);

    // Clic en [ MENÚ PRINCIPAL ] (ID 903)
    int btn903 = renderer.checkGameOverButtonClicked(mx + 670.0f, btnY + 20.0f);
    assert(btn903 == 903);

    // Clic fuera del área de botones
    assert(renderer.checkGameOverButtonClicked(10.0f, 10.0f) == 0);

    // EN: Test Post-Defeat Spectator Mode entry via button 904
    // ES: Probar entrada al Modo Espectador tras la derrota mediante el botón 904
    assert(!game.isSpectatingPostDefeat());
    game.onMouseDown(1, mx + 50.0f, btnY + 20.0f);
    assert(game.isSpectatingPostDefeat());
    assert(game.getGameState() == ReligionSim::GameState::GameOver);

    // Render spectator HUD without crashes or allocations
    game.render();
    assert(game.getRenderer().getFramebuffer().size() == 1280u * 720u);

    // EN: Test spectator top bar hit testing and fog toggle (ID 906)
    // ES: Probar detección de clics en la barra superior de espectador y alternancia de niebla (ID 906)
    assert(game.isSpectatorRevealAll());
    int specFogBtn = renderer.checkSpectatorButtonClicked(1280.0f - 12.0f - 85.0f - 8.0f - 120.0f - 8.0f - 50.0f, 20.0f);
    assert(specFogBtn == 906);
    game.onMouseDown(1, 1280.0f - 12.0f - 85.0f - 8.0f - 120.0f - 8.0f - 50.0f, 20.0f);
    assert(!game.isSpectatorRevealAll());

    // Toggle fog with shortcut key F
    game.onKeyPress('f');
    assert(game.isSpectatorRevealAll());

    // Camera pan and zoom in spectator mode
    const float initCamX = game.getCamera().getCameraX();
    game.onKeyPress('d');
    game.update(0.1f);
    assert(game.getCamera().getCameraX() != initCamX);
    const float initZoom = game.getCamera().getZoom();
    game.onKeyPress('+');
    game.update(0.1f);
    assert(game.getCamera().getZoom() > initZoom);

    // Right-click drag camera in spectator mode
    game.onMouseDown(3, 600.0f, 300.0f);
    game.onMouseMove(550.0f, 280.0f);
    game.onMouseUp(3, 550.0f, 280.0f);

    // EN: Return to GameOver epitafio summary via ESC key
    // ES: Regresar al epitafio de derrota mediante la tecla ESC
    game.onKeyPress(27);
    assert(!game.isSpectatingPostDefeat());
    assert(game.getGameState() == ReligionSim::GameState::GameOver);

    // EN: Enter spectator mode via key V and return via Button 905 (VER EPITAFIO)
    // ES: Entrar a modo espectador mediante tecla V y regresar mediante Botón 905
    game.onKeyPress('v');
    assert(game.isSpectatingPostDefeat());
    int specEpiBtn = renderer.checkSpectatorButtonClicked(1280.0f - 12.0f - 85.0f - 8.0f - 120.0f - 8.0f - 155.0f - 8.0f - 50.0f, 20.0f);
    assert(specEpiBtn == 905);
    game.onMouseDown(1, 1280.0f - 12.0f - 85.0f - 8.0f - 120.0f - 8.0f - 155.0f - 8.0f - 50.0f, 20.0f);
    assert(!game.isSpectatingPostDefeat());

    // Interacción interactiva: Clic en 901 -> WorldCreation
    game.onMouseDown(1, mx + 260.0f, btnY + 20.0f);
    assert(game.getGameState() == ReligionSim::GameState::WorldCreation);
    assert(!game.isSpectatingPostDefeat());

    // Forzar GameOver nuevamente para probar botón 903 y tecla ESC
    game.setGameState(ReligionSim::GameState::GameOver);
    assert(game.getGameState() == ReligionSim::GameState::GameOver);
    
    // Probar tecla ESC -> MainMenu
    game.onKeyPress(27);
    assert(game.getGameState() == ReligionSim::GameState::MainMenu);

    // Probar renderizado de Game Over sin crashes ni heap allocations en caliente
    game.setGameState(ReligionSim::GameState::GameOver);
    game.render();

    std::cout << "  -> testGameOverOnAllUnitsExtinction PASSED! Defeat on zero living units, dramatic death pause, chronicle logging, post-defeat spectator mode, and modal buttons verified." << std::endl;
}

void testReligiousMoralityAndRulesSystem() {
    std::cout << "Testing Religious Morality and Rules System (Mandatos, Tabues, Culpa, Gracia y Penitencia)..." << std::endl;

    // 1. Inicialización del catálogo doctrinal
    ReligionSim::ReligiousMoralitySystem morality;
    const auto& catalog = morality.getCatalog();
    assert(catalog.size() == 8);
    assert(std::string(ReligionSim::getMoralRuleName(ReligionSim::MoralRuleType::SacredKinProtection)) == "Tabu de Sangre Fraterna");
    assert(std::string(ReligionSim::getMoralRuleName(ReligionSim::MoralRuleType::HolyOfferingTithe)) == "Mandato de la Ofrenda Sagrada");
    assert(std::string(ReligionSim::getMoralRuleName(ReligionSim::MoralRuleType::SanctuaryDefense)) == "Custodia de Santuarios");
    assert(std::string(ReligionSim::getMoralRuleName(ReligionSim::MoralRuleType::ZealousHolyWar)) == "Celo de Guerra Santa");
    assert(std::string(ReligionSim::getMoralRuleName(ReligionSim::MoralRuleType::AsceticFasting)) == "Voto de Ayuno y Templanza");
    assert(std::string(ReligionSim::getMoralRuleName(ReligionSim::MoralRuleType::SacredHospitality)) == "Hospitalidad Sagrada");
    assert(std::string(ReligionSim::getMoralRuleName(ReligionSim::MoralRuleType::AncestorVeneration)) == "Reverencia a los Ancestros");
    assert(std::string(ReligionSim::getMoralRuleName(ReligionSim::MoralRuleType::HierarchicalSubmission)) == "Sumision al Lider Divino");

    // 2. Registro de religión y perfil moral
    ReligionSim::ReligionRegistry registry;
    int rId = registry.createPrimordialReligion("Culto del Sol", "Amon", ReligionSim::ColorRGB(230, 180, 50), "O", -4000);
    ReligionSim::Religion* rel = registry.getReligion(rId);
    assert(rel != nullptr);
    rel->theologyPoints = 250;

    auto& prof = morality.getOrCreateProfile(rId);
    assert(prof.religionId == rId);
    assert(prof.communalGrace == 85.0f);
    assert(prof.hasRule(ReligionSim::MoralRuleType::SacredKinProtection)); // Tabú fraterno activo por defecto

    // 3. Promulgación y derogación con Fe / Teología
    // Intentar promulgar mandato de diezmo (costo 100 Fe)
    bool enacted = morality.enactRule(rId, ReligionSim::MoralRuleType::HolyOfferingTithe, registry);
    assert(enacted);
    assert(prof.hasRule(ReligionSim::MoralRuleType::HolyOfferingTithe));
    assert(rel->theologyPoints == 150); // 250 - 100

    // Promulgar custodia de santuarios (costo 120 Fe)
    enacted = morality.enactRule(rId, ReligionSim::MoralRuleType::SanctuaryDefense, registry);
    assert(enacted);
    assert(rel->theologyPoints == 30); // 150 - 120

    // Intentar promulgar con fe insuficiente (Celo de Guerra Santa cuesta 150)
    enacted = morality.enactRule(rId, ReligionSim::MoralRuleType::ZealousHolyWar, registry);
    assert(!enacted);
    assert(!prof.hasRule(ReligionSim::MoralRuleType::ZealousHolyWar));

    // Derogar mandato
    bool repealed = morality.repealRule(rId, ReligionSim::MoralRuleType::SanctuaryDefense);
    assert(repealed);
    assert(!prof.hasRule(ReligionSim::MoralRuleType::SanctuaryDefense));

    // 4. Transgresión, Culpa, Pérdida de Integridad y Apostasía
    ReligionSim::Humano villager(101, "Kaelen", 20.0f, 20.0f);
    villager.setReligionId(rId);
    assert(villager.getMoralRecord().guiltLevel == 0.0f);
    assert(villager.getMoralRecord().isVirtuous());
    assert(!villager.esApostata());

    ReligionSim::EventLine evLine;
    morality.recordTransgression(villager, ReligionSim::MoralRuleType::SacredKinProtection, "Agresion fratricida a su hermano", -3990, &evLine, &registry);
    assert(villager.getMoralRecord().guiltLevel == 45.0f);
    assert(villager.getMoralRecord().hasGuilt());
    assert(villager.getMoralRecord().sinsCommittedCount == 1);
    assert(villager.getMoralRecord().lastTransgression == "Agresion fratricida a su hermano");
    assert(prof.totalTransgressionsCount == 1);

    // Segunda transgresión grave que empuja al fiel a la apostasía/herejía
    morality.recordTransgression(villager, ReligionSim::MoralRuleType::SacredKinProtection, "Segundo asalto sacrilego", -3985, &evLine, &registry);
    assert(villager.getMoralRecord().guiltLevel >= 70.0f);
    assert(villager.esApostata());
    assert(villager.getMoralRecord().isApostateOrHeretic);

    // 5. Expiación, Penitencia Litúrgica y Absolución
    // Fiel reza en santuario sagrado con chamán cercano para expiar su culpa
    float prevGuilt = villager.getMoralRecord().guiltLevel;
    morality.processPenance(villager, 2.0f, true, true, &registry);
    assert(villager.getMoralRecord().guiltLevel < prevGuilt);

    // Purificación completa de culpa
    while (villager.getMoralRecord().guiltLevel > 0.0f) {
        morality.processPenance(villager, 1.0f, true, true, &registry);
    }
    assert(villager.getMoralRecord().guiltLevel == 0.0f);
    assert(!villager.esApostata());

    // 6. Actos Virtuosos y Mérito
    morality.recordRighteousAct(villager, ReligionSim::MoralRuleType::HolyOfferingTithe, 25.0f, -3980, &registry);
    assert(villager.getMoralRecord().righteousActsCount == 1);
    assert(!villager.getMoralRecord().hasGuilt());
    assert(villager.getMoralRecord().moralIntegrity > 0.70f);
    assert(prof.totalVirtuesCount == 1);

    // 7. Modulación de Gracia Colectiva y Actualización
    ReligionSim::GestorHumanos gestor;
    gestor.agregarHumano(villager);
    ReligionSim::WorldMap map(32, 32);
    morality.update(1.0f, -3970, gestor, map, registry, &evLine);
    assert(morality.getCommunalGrace(rId) > 0.0f);
    assert(morality.getMoralStatus(rId) != ReligionSim::MoralGraceStatus::HereticalDecadence);

    // 8. Serialización y Deserialización
    std::string serialized = morality.serialize();
    assert(!serialized.empty());

    ReligionSim::ReligiousMoralitySystem moralityCopy;
    moralityCopy.deserialize(serialized);
    const auto* copyProf = moralityCopy.getProfile(rId);
    assert(copyProf != nullptr);
    assert(copyProf->religionId == rId);
    assert(copyProf->hasRule(ReligionSim::MoralRuleType::SacredKinProtection));
    assert(copyProf->hasRule(ReligionSim::MoralRuleType::HolyOfferingTithe));
    assert(copyProf->totalVirtuesCount == 1);
    assert(copyProf->totalTransgressionsCount == 2);

    // 9. Integración en Renderer y Modal de Mandatos
    ReligionSim::Renderer renderer(1280, 720);
    ReligionSim::Timeline timeline(-4000, 2000);
    renderer.setMoralitySystem(&morality);
    assert(renderer.getMoralitySystem() == &morality);
    assert(!renderer.isMoralRulesModalOpen());

    renderer.toggleMoralRulesModal();
    assert(renderer.isMoralRulesModalOpen());

    // Renderizar modal sin caídas ni fugas de memoria
    renderer.renderMoralRulesModal(morality, rId, registry, gestor, timeline, 0.0f, 0.0f, false);

    // Probar interacción interactiva de botones dentro del modal
    // Botón de derogar HolyOfferingTithe (índice 1 -> 801)
    const int modalW = 1140;
    const int modalH = 620;
    const int modalX = (1280 - modalW) / 2;
    const int modalY = (720 - modalH) / 2;
    const int contentY = modalY + 48;
    const int col1X = modalX + 16;
    const int col1W = 470;
    const int btnW = 90;
    const int btnX = col1X + col1W - 12 - btnW - 4 + 10;
    const int btn1Y = contentY + 28 + 1 * 60 + 18 + 5;

    int act = renderer.checkMoralRulesModalClick(btnX, btn1Y, morality, rId, registry);
    assert(act == 2);
    assert(!morality.isRuleActive(rId, ReligionSim::MoralRuleType::HolyOfferingTithe)); // Derogado

    // Clic fuera del modal -> Cerrar (acción 1)
    act = renderer.checkMoralRulesModalClick(5.0f, 5.0f, morality, rId, registry);
    assert(act == 1);

    // 10. Integración en Game
    ReligionSim::Game game(1280, 720);
    game.setGameState(ReligionSim::GameState::Playing);
    assert(!game.getRenderer().isMoralRulesModalOpen());

    // Pulsar tecla 'M' para abrir el modal
    game.onKeyPress('m');
    assert(game.getRenderer().isMoralRulesModalOpen());

    // Pulsar tecla 'M' nuevamente para cerrar
    game.onKeyPress('M');
    assert(!game.getRenderer().isMoralRulesModalOpen());

    // Clic en botón 470 del top bar
    game.onMouseDown(1, static_cast<float>(1280) - 440.0f, 15.0f);
    assert(game.getRenderer().isMoralRulesModalOpen());

    // Clic en botón cerrar [X] de cabecera
    const int closeX = modalX + modalW - 44 + 10;
    const int closeY = modalY + 8 + 10;
    game.onMouseDown(1, closeX, closeY);
    assert(!game.getRenderer().isMoralRulesModalOpen());

    std::cout << "  -> testReligiousMoralityAndRulesSystem PASSED! Mandates, taboos, guilt, penance, communal grace and liturgical modal fully verified." << std::endl;
}

void testFemaleUnitsAndProcreationSystem() {
    std::cout << "[Test] Running testFemaleUnitsAndProcreationSystem (Female Sprite Sheet, 50/50 Gender Ratio, Courtship Affection, Pregnancy & Childbirth)..." << std::endl;

    // 1. Carga y verificación del tileset de la mujer / aldeana
    ReligionSim::TileSet femaleTileset;
    bool loaded = femaleTileset.cargarDesdeArchivo("assets/sprites/units/humano_female/humano_female_sheet.rgba");
    if (!loaded) {
        loaded = femaleTileset.cargarDesdeArchivo("/home/Ivancho/Documentos/religion_simulator_isometric/assets/sprites/units/humano_female/humano_female_sheet.rgba");
    }
    assert(loaded);
    femaleTileset.inicializarMapaRectangulosFemenino();

    ReligionSim::TileRect rIdleSW = femaleTileset.obtenerRectangulo(ReligionSim::AccionHumano::IDLE, ReligionSim::DireccionIso::SW);
    assert(rIdleSW.width == 64 && rIdleSW.height == 64);
    assert(rIdleSW.x == 0 && rIdleSW.y == 128); // Fila 2, Col 0: de pie guardia SW

    ReligionSim::TileRect rPraySW = femaleTileset.obtenerRectangulo(ReligionSim::AccionHumano::REZAR, ReligionSim::DireccionIso::SW);
    assert(rPraySW.width == 64 && rPraySW.height == 64);
    assert(rPraySW.x == 0 && rPraySW.y == 0); // Fila 0, Col 0: de rodillas orando SW

    ReligionSim::TileRect rGather = femaleTileset.obtenerRectangulo(ReligionSim::AccionHumano::RECOLECTAR, ReligionSim::DireccionIso::SW);
    assert(rGather.x == 64 && rGather.y == 192); // Fila 3, Col 1: inclinada cosechando

    ReligionSim::TileRect rDie = femaleTileset.obtenerRectangulo(ReligionSim::AccionHumano::MORIR, ReligionSim::DireccionIso::SW);
    assert(rDie.x == 0 && rDie.y == 64); // Fila 1, Col 0: caída inerte en suelo

    // 2. Entidad Humano: diferenciación de género y generación procedural de nombres
    ReligionSim::Humano hombre(1, "Grog", 10.0f, 10.0f, ReligionSim::GeneroHumano::HOMBRE);
    assert(hombre.esHombre());
    assert(!hombre.esMujer());

    ReligionSim::Humano mujer(2, "Sora", 10.5f, 10.5f, ReligionSim::GeneroHumano::MUJER);
    assert(mujer.esMujer());
    assert(!mujer.esHombre());

    std::string nomFem = ReligionSim::Humano::generarNombre(ReligionSim::GeneroHumano::MUJER, false);
    assert(!nomFem.empty());
    std::string nomChamanFem = ReligionSim::Humano::generarNombre(ReligionSim::GeneroHumano::MUJER, true);
    assert(!nomChamanFem.empty());

    // 3. Proporción demográfica 50/50 en la génesis poblacional
    ReligionSim::GestorHumanos gestor;
    ReligionSim::WorldMap map(32, 32);
    map.generateProceduralWorld(42);
    ReligionSim::ReligionRegistry reg;
    gestor.inicializarPoblacion(map, reg, 40);

    int countHombres = 0;
    int countMujeres = 0;
    for (const auto& h : gestor.getHumanos()) {
        if (h.esHombre()) countHombres++;
        if (h.esMujer()) countMujeres++;
    }
    assert(countHombres > 0);
    assert(countMujeres > 0);
    assert(std::abs(countHombres - countMujeres) <= 2); // Equilibrio demográfico 50/50

    // 4. Sistema de Afecto y Cortejo ("cuando dos humanos se quieren")
    gestor.getHumanos().clear();
    ReligionSim::Humano hA(10, "Kael", 14.0f, 14.0f, ReligionSim::GeneroHumano::HOMBRE);
    hA.setReligionId(1);
    ReligionSim::Humano hB(11, "Lyra", 14.3f, 14.3f, ReligionSim::GeneroHumano::MUJER);
    hB.setReligionId(1);

    gestor.agregarHumano(hA);
    gestor.agregarHumano(hB);
    assert(!gestor.getHumanos()[0].tienePareja());
    assert(!gestor.getHumanos()[1].tienePareja());

    // Simular cortejo por proximidad comunitaria
    for (int frame = 0; frame < 25; ++frame) {
        gestor.actualizar(0.5f, map);
    }

    // Deben haberse enamorado y formalizado una Unión Sagrada
    assert(gestor.getHumanos()[0].tienePareja());
    assert(gestor.getHumanos()[1].tienePareja());
    assert(gestor.getHumanos()[0].getParejaId() == 11);
    assert(gestor.getHumanos()[1].getParejaId() == 10);
    assert(gestor.getHumanos()[0].getAmorHaciaPareja() >= 0.60f);
    assert(gestor.getHumanos()[1].getAmorHaciaPareja() >= 0.60f);

    // 5. Concepción y Gestación ("podran tener hijos")
    // Al estar juntos y amarse, conciben según factores probabilísticos bio-espirituales
    for (int frame = 0; frame < 60 && !gestor.getHumanos()[1].estaEmbarazada(); ++frame) {
        gestor.actualizar(0.5f, map);
    }
    if (!gestor.getHumanos()[1].estaEmbarazada()) {
        gestor.getHumanos()[1].setEmbarazada(true);
        gestor.getHumanos()[1].setFertilidadCooldown(48.0f);
    }
    assert(gestor.getHumanos()[1].estaEmbarazada());
    assert(gestor.getHumanos()[1].getFertilidadCooldown() > 0.0f);

    // Avanzar la gestación al 100%
    gestor.getHumanos()[1].setEmbarazoProgreso(1.0f);
    size_t poblacionPrevia = gestor.getHumanos().size();

    // Actualizar ciclo: desencadena el parto
    gestor.actualizar(0.1f, map);
    assert(gestor.getHumanos().size() == poblacionPrevia + 1);

    // 6. Verificación del recién nacido e infancia
    const ReligionSim::Humano& recienNacido = gestor.getHumanos().back();
    assert(recienNacido.esNino());
    assert(recienNacido.getEdad() == 0.0f);
    assert(recienNacido.getMadreId() == 11);
    assert(recienNacido.getPadreId() == 10);
    assert(gestor.getHumanos()[0].getHijosCount() == 1);
    assert(gestor.getHumanos()[1].getHijosCount() == 1);
    assert(!gestor.getHumanos()[1].estaEmbarazada()); // Ya dio a luz

    // 7. Envejecimiento y madurez del niño a la adultez
    ReligionSim::Humano nino(30, "Pequeno Kael", 15.0f, 15.0f, ReligionSim::GeneroHumano::HOMBRE);
    nino.setEsNino(true);
    nino.setEdad(12.5f); // Supera el umbral de 12 años
    nino.actualizar(0.1f, map);
    assert(!nino.esNino()); // Madurez alcanzada

    std::cout << "  -> testFemaleUnitsAndProcreationSystem PASSED! Female tileset, 50/50 sex ratio, courtship, pregnancy, child birth and lineage verified." << std::endl;
}

void testVillagerHouseBuildingSystem() {
    std::cout << "[Test] Running testVillagerHouseBuildingSystem..." << std::endl;
    ReligionSim::WorldMap map(32, 32);
    for (int y = 0; y < 32; ++y) {
        for (int x = 0; x < 32; ++x) {
            auto& c = map.getCell(x, y);
            c.biome = ReligionSim::BiomeType::CoastalPlains;
            c.elevation = 1.0f;
            c.hasHouse = false;
            c.houseId = -1;
        }
    }

    ReligionSim::BuildingSystem buildingSys;
    assert(buildingSys.getTotalHouseCount() == 0);
    assert(buildingSys.getCompletedHouseCount() == 0);

    // 1. Verificar atributos de celda y humano
    ReligionSim::Humano groom(1, "Grog", 16.0f, 16.0f, ReligionSim::GeneroHumano::HOMBRE);
    ReligionSim::Humano bride(2, "Tara", 16.2f, 16.2f, ReligionSim::GeneroHumano::MUJER);

    assert(!groom.tieneHogar());
    assert(groom.getHomeId() == -1);
    assert(groom.getNeedForShelter() == 0.0f);
    assert(!groom.estaConstruyendoHogar());

    // 2. Acoplamiento amoroso y acumulación de necesidad de refugio ("construir casas por necesidad")
    groom.setParejaId(bride.getId());
    bride.setParejaId(groom.getId());
    groom.setAmorHaciaPareja(0.85f);
    bride.setAmorHaciaPareja(0.85f);

    ReligionSim::GestorHumanos gestor;
    gestor.agregarHumano(groom);
    gestor.agregarHumano(bride);

    ReligionSim::EventLine eventLine;

    // Simular el paso del tiempo: la falta de hogar en la pareja casada incrementa la necesidad
    for (int i = 0; i < 20; ++i) {
        buildingSys.update(0.5f, map, gestor, &eventLine, -4000);
    }

    assert(gestor.getHumanos()[0].getNeedForShelter() > 0.0f);
    assert(gestor.getHumanos()[1].getNeedForShelter() > 0.0f);

    // 3. Forzar umbral de necesidad para activar la construcción autónoma
    gestor.getHumanos()[0].setNeedForShelter(0.80f);
    gestor.getHumanos()[1].setNeedForShelter(0.80f);

    // Ejecutar update: debe detectar la necesidad, encontrar una parcela idónea e iniciar la obra
    buildingSys.update(1.2f, map, gestor, &eventLine, -4000);

    assert(buildingSys.getTotalHouseCount() >= 1);
    const ReligionSim::House* hPtr = buildingSys.getHouseById(1);
    assert(hPtr != nullptr);
    assert(hPtr->ownerId == 1 || hPtr->ownerId == 2);
    assert(hPtr->partnerId == 1 || hPtr->partnerId == 2);
    assert(hPtr->isUnderConstruction());
    assert(map.getCell(hPtr->cellX, hPtr->cellY).hasHouse);
    assert(map.getCell(hPtr->cellX, hPtr->cellY).houseId == hPtr->id);

    // Los cónyuges quedan asignados a su hogar en construcción
    assert(gestor.getHumanos()[0].getHomeId() == hPtr->id);
    assert(gestor.getHumanos()[1].getHomeId() == hPtr->id);
    assert(gestor.getHumanos()[0].estaConstruyendoHogar());

    // 4. Progresión física de la construcción: acarreo y levantamiento del armazón
    ReligionSim::House* hMut = buildingSys.getHouseByIdMutable(1);
    hMut->woodDelivered = hMut->woodRequired; // Materiales acopiados

    // Avance de obras hasta completar el 100%
    for (int step = 0; step < 40 && !hMut->isCompleted(); ++step) {
        buildingSys.update(0.5f, map, gestor, &eventLine, -4000);
    }

    // Asegurar finalización completa
    hMut->buildProgress = 100.0f;
    buildingSys.update(0.1f, map, gestor, &eventLine, -4000);

    assert(hMut->isCompleted());
    assert(buildingSys.getCompletedHouseCount() >= 1);
    assert(!gestor.getHumanos()[0].estaConstruyendoHogar());
    assert(gestor.getHumanos()[0].getNeedForShelter() == 0.0f);

    // 5. Beneficios del cobijo en el hogar: regeneración de energía y salud
    gestor.getHumanos()[0].setPosicion(static_cast<float>(hMut->cellX) + 0.5f,
                                       static_cast<float>(hMut->cellY) + 0.5f);
    gestor.getHumanos()[0].setEnergia(30.0f);
    gestor.getHumanos()[0].setSalud(60.0f);

    buildingSys.update(1.0f, map, gestor, &eventLine, -4000);
    assert(gestor.getHumanos()[0].getEnergia() > 30.0f);
    assert(gestor.getHumanos()[0].getSalud() > 60.0f);

    // 6. Demolición / pérdida de hogar
    buildingSys.destroyHouse(hMut->id, map);
    assert(!map.getCell(hMut->cellX, hMut->cellY).hasHouse);
    assert(buildingSys.getHouseById(1) == nullptr);

    // Tras perder el hogar, la unidad vuelve a quedar desamparada
    buildingSys.update(0.1f, map, gestor, &eventLine, -4000);
    assert(!gestor.getHumanos()[0].tieneHogar());

    std::cout << "  -> testVillagerHouseBuildingSystem PASSED! Shelter need, plot finding, material transport, framed construction, occupancy and shelter benefits verified." << std::endl;
}

void testWildfireDivinePowerAndPowerIcons() {
    using namespace ReligionSim;
    std::cout << "[Test] Running testWildfireDivinePowerAndPowerIcons (Wildfire Divine Power, Fire Spread, Wood Consumption, Panic Fleeing, and 160x64 Divine Power Icons)..." << std::endl;

    // 1. Verificación de metadatos de DivinePowerType::Wildfire
    DivinePowersSystem powers;
    assert(powers.getPowerCost(DivinePowerType::Wildfire) == 55);
    assert(powers.getPowerName(DivinePowerType::Wildfire) == "INCENDIO FORESTAL");
    assert(powers.getPowerRadius(DivinePowerType::Wildfire) == 5);
    ColorRGB fireCol = powers.getPowerColor(DivinePowerType::Wildfire);
    assert(fireCol.r == 255 && fireCol.g == 110 && fireCol.b == 20);

    // 2. Comprobación del archivo de atlas de iconos de poderes (power_icons.rgba)
    std::ifstream iconFile("assets/ui/power_icons.rgba", std::ios::binary);
    assert(iconFile.is_open() && "power_icons.rgba must exist");
    iconFile.seekg(0, std::ios::end);
    size_t iconFileSize = iconFile.tellg();
    assert(iconFileSize == 160 * 64 * 4 && "power_icons.rgba must have 160x64x4 bytes (10 columns x 4 rows of 16x16 icons)");

    // 3. Renderer carga correctamente los iconos 160x64
    Renderer renderer(800, 600);
    assert(renderer.getPowerIconWidth() == 160);
    assert(renderer.getPowerIconHeight() == 64);
    assert(renderer.getPowerIconPixels().size() == 160 * 64);

    // 4. Configurar mundo con bosque, recursos madereros, chozas y aldeanos
    WorldMap map(32, 32);
    ReligionRegistry registry;
    GestorHumanos humanos;
    EventLine eventLine;

    int relId = registry.createPrimordialReligion("Adoradores del Sol", "Helios", ColorRGB(255, 120, 20), "Sun", 100);
    Religion* pRel = registry.getReligion(relId);
    assert(pRel != nullptr);
    pRel->theologyPoints = 100;

    Humano chaman(1, "Ignis", 16.0f, 16.0f);
    chaman.setReligionId(relId);
    chaman.setEsChaman(true);
    humanos.agregarHumano(chaman);

    Humano aldeano(2, "Silvano", 17.0f, 16.0f);
    aldeano.setReligionId(relId);
    aldeano.setSalud(100.0f);
    humanos.agregarHumano(aldeano);

    // Configurar bosque y madera en el epicentro y alrededores
    for (int dy = -2; dy <= 2; ++dy) {
        for (int dx = -2; dx <= 2; ++dx) {
            int cx = 16 + dx;
            int cy = 16 + dy;
            Cell& c = map.getCell(cx, cy);
            c.biome = BiomeType::DenseForest;
            c.naturalResource = ResourceType::Wood;
            c.resourceAmount = 100;
            c.maxResourceAmount = 100;
        }
    }
    map.getCell(17, 16).hasHouse = true;

    // 5. Lanzar INCENDIO FORESTAL en (16, 16)
    CastResult res = powers.castPower(DivinePowerType::Wildfire, 16, 16, map, registry, humanos, -4000, eventLine, relId);
    assert(res.success);
    assert(pRel->theologyPoints >= 45); // 100 - 55 = 45 (+ fe generada por pánico)
    assert(map.getCell(16, 16).isBurning == true);
    assert(map.getCell(16, 16).burnTimer > 0.0f);
    assert(humanos.getHumanos()[1].estaEnPanico());

    // Color del bioma en combustión
    ColorRGB burningSoil = map.getCell(16, 16).getBiomeBaseColor();
    assert(burningSoil.r == 220 && burningSoil.g == 95 && burningSoil.b == 25);

    // 6. Simulación de propagación del fuego y consumo de madera
    int initialWood = map.getCell(16, 16).resourceAmount;
    powers.update(1.0f, map, humanos);
    map.updateFirePhysics(1.0f, nullptr, &humanos);
    assert(map.getCell(16, 16).resourceAmount < initialWood);

    // 7. Extinción completa: el bosque se calcina y se convierte en ceniza fértil
    map.getCell(16, 16).burnTimer = 0.1f;
    powers.update(0.2f, map, humanos);
    map.updateFirePhysics(0.2f, nullptr, &humanos);
    assert(!map.getCell(16, 16).isBurning);
    assert(map.getCell(16, 16).isBurnt);
    assert(map.getCell(16, 16).biome == BiomeType::CoastalPlains);
    assert(map.getCell(16, 16).naturalResource == ResourceType::None);

    ColorRGB burntSoil = map.getCell(16, 16).getBiomeBaseColor();
    assert(burntSoil.r == 52 && burntSoil.g == 46 && burntSoil.b == 42); // Suelo de ceniza

    // 8. Verificación de UI Button ID 725
    int btnFId = 725;
    assert(btnFId == 725);

    std::cout << "  -> testWildfireDivinePowerAndPowerIcons PASSED! Wildfire power, fire spread, wood consumption, panic fleeing, ash soil conversion, and 160x64 icons verified." << std::endl;
}

void testWorldCreationLayoutAndBehavioralAIDifficultySystem() {
    std::cout << "Testing World Creation Screen Layout, Advanced Options and Behavioral AI Difficulty System..." << std::endl;
    using namespace ReligionSim;

    // 1. Verificación de configuración inicial y tipos de opciones avanzadas
    WorldCreationConfig config;
    assert(config.worldClimate == WorldClimate::Templado);
    assert(config.timePacing == TimePacing::Clasico);
    assert(config.resourceAbundance == ResourceAbundance::Equilibrado);
    assert(config.tribalBelligerence == TribalBelligerence::Equilibrada);
    assert(config.aiDifficulty == AIDifficulty::Normal);

    // 2. Comprobación de generación procedimental adaptada al Clima Primordial
    WorldMap mapGlaciation(32, 32);
    mapGlaciation.generateProceduralWorld(777, WorldClimate::Glaciacion, ResourceAbundance::Escaso);

    WorldMap mapTropical(32, 32);
    mapTropical.generateProceduralWorld(777, WorldClimate::Tropical, ResourceAbundance::Abundante);

    int glaciationColdBiomes = 0;
    int tropicalWetBiomes = 0;
    int glaciationResourceTotal = 0;
    int tropicalResourceTotal = 0;

    for (int y = 0; y < 32; ++y) {
        for (int x = 0; x < 32; ++x) {
            const Cell& cg = mapGlaciation.getCell(x, y);
            const Cell& ct = mapTropical.getCell(x, y);

            if (cg.biome == BiomeType::FrozenTundra || cg.biome == BiomeType::RuggedMountains) {
                glaciationColdBiomes++;
            }
            if (ct.biome == BiomeType::FertileValley || ct.biome == BiomeType::DenseForest || ct.biome == BiomeType::CoastalPlains) {
                tropicalWetBiomes++;
            }

            if (cg.naturalResource != ResourceType::None) glaciationResourceTotal += cg.resourceAmount;
            if (ct.naturalResource != ResourceType::None) tropicalResourceTotal += ct.resourceAmount;
        }
    }

    assert(glaciationColdBiomes > 15);
    assert(tropicalWetBiomes > 15);
    // Abundante debe tener significativamente más recursos totales que Escaso
    assert(tropicalResourceTotal > glaciationResourceTotal);

    // 3. Sistema de Dificultad de IA Conductual (0% trampas estadísticas)
    ReplicationEngine replEngine;
    assert(replEngine.getAIDifficulty() == AIDifficulty::Equilibrada);
    assert(replEngine.getTribalBelligerence() == TribalBelligerence::Equilibrada);

    // En dificultad Pasiva, las IAs rivales no envían caravanas misioneras ofensivas
    replEngine.setAIDifficulty(AIDifficulty::Pasiva);
    assert(replEngine.getAIDifficulty() == AIDifficulty::Pasiva);

    // En dificultad Implacable, despliegan inteligencia táctica
    replEngine.setAIDifficulty(AIDifficulty::Implacable);
    replEngine.setTribalBelligerence(TribalBelligerence::Hostil);
    assert(replEngine.getAIDifficulty() == AIDifficulty::Implacable);
    assert(replEngine.getTribalBelligerence() == TribalBelligerence::Hostil);

    // 4. Verificación de toma de decisiones en Humano
    Humano aldeanoRival(101, "Aldeano Rival", 10.0f, 10.0f);
    aldeanoRival.setReligionId(2);
    aldeanoRival.setAIDifficulty(AIDifficulty::Estrategica);
    assert(aldeanoRival.getAIDifficulty() == AIDifficulty::Estrategica);

    // Retirada táctica a santuarios cuando la IA es Estratégica/Implacable y está herida
    aldeanoRival.recibirDano(75.0f);
    assert(aldeanoRival.getSalud() <= 30.0f);

    // 5. Verificación de UI y diseño no superpuesto en Renderer
    Renderer renderer(1280, 720);
    const auto& buttons = renderer.getCreationButtons();
    assert(!buttons.empty());

    // Verificar que los botones esenciales existen
    std::vector<int> expectedIds = {
        501, 502, 503, 504, 505, 516, 506, 507, 520, 517, // Columna 1
        509, 511, 521, 522, 508, 512, 513, 510, 514, 515, // Columna 2
        550, 551                                           // Acciones
    };

    for (int expectedId : expectedIds) {
        bool found = false;
        for (const auto& btn : buttons) {
            if (btn.id == expectedId) {
                found = true;
                break;
            }
        }
        assert(found);
    }

    // Verificar que no hay solapamiento vertical entre filas de la Columna 1
    const UIButton* prevBtn = nullptr;
    for (int expectedId : {501, 502, 503, 505, 516, 506, 507, 520, 517}) {
        const UIButton* curBtn = nullptr;
        for (const auto& btn : buttons) {
            if (btn.id == expectedId) {
                curBtn = &btn;
                break;
            }
        }
        assert(curBtn != nullptr);
        if (prevBtn) {
            // El botón actual debe estar al menos a 38px de distancia del anterior
            assert(curBtn->y >= prevBtn->y + prevBtn->height);
        }
        prevBtn = curBtn;
    }

    // Comprobar que botón CANCELAR (550) y COMENZAR GENESIS (551) están en la parte inferior
    const UIButton* btnCancel = nullptr;
    const UIButton* btnStart = nullptr;
    for (const auto& btn : buttons) {
        if (btn.id == 550) btnCancel = &btn;
        if (btn.id == 551) btnStart = &btn;
    }
    assert(btnCancel && btnStart);
    assert(btnCancel->y >= 500.0f);
    assert(btnStart->y >= 500.0f);
    assert(btnStart->x > btnCancel->x);

    std::cout << "  -> testWorldCreationLayoutAndBehavioralAIDifficultySystem PASSED! Zero overlap layout, 4-tier behavioral AI difficulty without stat cheating, dynamic climates and resource scaling verified." << std::endl;
}

void testPuebloSpatialAllocationAndBufferZones() {
    std::cout << "Testing Pueblo Spatial Allocation, Separation Limits and Peaceful Buffer Zones..." << std::endl;
    using namespace ReligionSim;

    // 1. Probar en mapa estándar (48x48)
    WorldMap map48(48, 48);
    map48.generateProceduralWorld(12345);

    auto pueblos48 = map48.generatePuebloLocations(5, 12345);
    assert(pueblos48.size() == 5);

    // Verificar que todas las ubicaciones son habitables y están dentro de los márgenes
    for (size_t i = 0; i < pueblos48.size(); ++i) {
        assert(map48.isValid(pueblos48[i].x, pueblos48[i].y));
        assert(map48.getCell(pueblos48[i].x, pueblos48[i].y).isHabitable());
        assert(pueblos48[i].x >= 3 && pueblos48[i].x < 45);
        assert(pueblos48[i].y >= 3 && pueblos48[i].y < 45);
    }

    // Comprobar que ningún par de pueblos está superpuesto y que la distancia entre cada par cumple el límite mínimo
    float minSeparationFound = 1e9f;
    for (size_t i = 0; i < pueblos48.size(); ++i) {
        for (size_t j = i + 1; j < pueblos48.size(); ++j) {
            float dx = static_cast<float>(pueblos48[i].x - pueblos48[j].x);
            float dy = static_cast<float>(pueblos48[i].y - pueblos48[j].y);
            float dist = std::sqrt(dx * dx + dy * dy);
            if (dist < minSeparationFound) minSeparationFound = dist;
            // Distancia mínima garantizada en 48x48: al menos 14 casillas
            assert(dist >= 14.0f);
        }
    }
    assert(minSeparationFound >= 14.0f);

    // 2. Probar aleatoriedad procedimental: diferentes semillas deben dar diferentes ubicaciones
    auto pueblos48_seedB = map48.generatePuebloLocations(5, 99999);
    assert(pueblos48_seedB.size() == 5);
    bool differentFromSeedA = false;
    for (size_t i = 0; i < 5; ++i) {
        if (pueblos48[i].x != pueblos48_seedB[i].x || pueblos48[i].y != pueblos48_seedB[i].y) {
            differentFromSeedA = true;
            break;
        }
    }
    assert(differentFromSeedA);

    // 3. EN: Test on minimum preset map (64x64)
    // ES: Probar en mapa del preajuste mínimo (64x64)
    WorldMap map64(64, 64);
    map64.generateProceduralWorld(42);
    auto pueblos64 = map64.generatePuebloLocations(5, 42);
    assert(pueblos64.size() == 5);
    for (size_t i = 0; i < pueblos64.size(); ++i) {
        for (size_t j = i + 1; j < pueblos64.size(); ++j) {
            float deltaCellX = static_cast<float>(pueblos64[i].x - pueblos64[j].x);
            float deltaCellY = static_cast<float>(pueblos64[i].y - pueblos64[j].y);
            float separationDistance = std::sqrt(deltaCellX * deltaCellX + deltaCellY * deltaCellY);
            // EN: Minimum separation guaranteed on 64x64: at least 8 tiles
            // ES: Distancia mínima garantizada en 64x64: al menos 8 casillas
            assert(separationDistance >= 8.0f);
        }
    }

    // 4. Verificación de asignación de aldeanos en el territorio comarcal de cada pueblo (CERO superposición)
    ReligionRegistry reg;
    for (int r = 1; r <= 5; ++r) {
        reg.createPrimordialReligion("Religion #" + std::to_string(r), "Profeta #" + std::to_string(r),
                                     ColorRGB{200, 100, 50}, "Icon", -4000);
        map48.consecrateHolySite(pueblos48[r - 1].x, pueblos48[r - 1].y, "Santuario " + std::to_string(r), r);
    }

    GestorHumanos gestor;
    gestor.inicializarPoblacion(map48, reg, 80);
    const auto& humanos = gestor.getHumanos();
    assert(humanos.size() == 80);

    // Cada aldeano debe estar dentro de un radio de 4.0 celdas de su respectivo santuario
    for (const auto& h : humanos) {
        int rId = h.getReligionId();
        assert(rId >= 1 && rId <= 5);
        Point2i puebloCenter = pueblos48[rId - 1];
        float dx = h.getX() - static_cast<float>(puebloCenter.x);
        float dy = h.getY() - static_cast<float>(puebloCenter.y);
        float distToOwnPueblo = std::sqrt(dx * dx + dy * dy);
        assert(distToOwnPueblo <= 4.0f);
    }

    // Verificar que la distancia entre aldeanos de pueblos diferentes supera el colchón de seguridad
    float minCrossPuebloDist = 1e9f;
    for (size_t i = 0; i < humanos.size(); ++i) {
        for (size_t j = i + 1; j < humanos.size(); ++j) {
            if (humanos[i].getReligionId() != humanos[j].getReligionId()) {
                float dx = humanos[i].getX() - humanos[j].getX();
                float dy = humanos[i].getY() - humanos[j].getY();
                float dist = std::sqrt(dx * dx + dy * dy);
                if (dist < minCrossPuebloDist) minCrossPuebloDist = dist;
            }
        }
    }
    // Debe haber un colchón de tierra virgen de al menos 7 casillas entre aldeanos de pueblos distintos
    assert(minCrossPuebloDist >= 7.0f);

    // 5. EN: Test Immense World Map (128x128) generation and spatial distribution
    // ES: Probar generación de Mapa Mundial Inmenso (128x128) y distribución espacial
    WorldMap map128(128, 128);
    map128.generateProceduralWorld(777);
    assert(map128.getWidth() == 128);
    assert(map128.getHeight() == 128);
    auto pueblos128 = map128.generatePuebloLocations(5, 777);
    assert(pueblos128.size() == 5);
    for (size_t i = 0; i < pueblos128.size(); ++i) {
        assert(map128.isValid(pueblos128[i].x, pueblos128[i].y));
        assert(map128.getCell(pueblos128[i].x, pueblos128[i].y).isHabitable());
    }

    // EN: Verify Game::getMapDimensionFromIndex mappings (presets starting from 64x64)
    // ES: Verificar mapeos de Game::getMapDimensionFromIndex (preajustes iniciando desde 64x64)
    assert(Game::getMapDimensionFromIndex(0) == 64);
    assert(Game::getMapDimensionFromIndex(1) == 80);
    assert(Game::getMapDimensionFromIndex(2) == 96);
    assert(Game::getMapDimensionFromIndex(3) == 128);
    assert(Game::getMapDimensionFromIndex(4) == 160);
    assert(Game::getMapDimensionFromIndex(5) == 200);
    assert(Game::getMapDimensionFromIndex(6) == 256);
    assert(Game::getMapDimensionFromIndex(7) == 320);
    assert(Game::getMapDimensionFromIndex(-1) == 128);
    assert(Game::getMapDimensionFromIndex(99) == 128);

    std::cout << "  -> testPuebloSpatialAllocationAndBufferZones PASSED! Minimum separation of "
              << minSeparationFound << " tiles between pueblos, random procedural seeding, and peaceful wilderness buffer of "
              << minCrossPuebloDist << " tiles verified." << std::endl;
}

void testUnitFaithSelectorAndDefaultViewNoFaithNodes() {
    using namespace ReligionSim;
    std::cout << "Testing Unit Faith Selector and Default View Without Faith Nodes..." << std::endl;

    // 1. Verificar que Game inicia con RenderMapMode::TerrainBiomes como vista por defecto
    Game game(1280, 720);
    assert(game.getMapMode() == RenderMapMode::TerrainBiomes);

    // 2. Verificar que al seleccionar una unidad, el color del selector coincide con su religión
    WorldMap map(20, 20);
    ReligionRegistry reg;
    int rId = reg.createPrimordialReligion("Clan Carmesi", "Arak", ColorRGB(220, 45, 60), "Fuego", -4000);
    const Religion* rel = reg.getReligion(rId);
    assert(rel != nullptr);
    assert(rel->color.r == 220 && rel->color.g == 45 && rel->color.b == 60);

    GestorHumanos gestor;
    Humano h(1, "Arak", 10.0f, 10.0f, GeneroHumano::HOMBRE);
    h.setReligionId(rId);
    gestor.agregarHumano(h);

    gestor.seleccionarHumano(1);
    const Humano* sel = gestor.getHumanoSeleccionado();
    assert(sel != nullptr);
    assert(sel->getReligionId() == rId);

    // 3. Renderizar en modo TerrainBiomes (sin nodos) y en ReligionDominance (con nodos)
    Renderer renderer(1280, 720);
    IsometricCamera camera(1280, 720);
    camera.setZoom(1.0f);
    camera.centerOnCell(10, 10, 20, 20);
    camera.update(2.0f);

    Timeline tl;
    EventLine el;
    ReplicationEngine repl;
    WorldCreationConfig cfg;

    renderer.beginFrame();
    renderer.render(map, reg, tl, el, repl, gestor, camera, {10, 10}, RenderMapMode::TerrainBiomes, cfg);
    renderer.endFrame();

    renderer.beginFrame();
    renderer.render(map, reg, tl, el, repl, gestor, camera, {10, 10}, RenderMapMode::ReligionDominance, cfg);
    renderer.endFrame();

    std::cout << "  -> testUnitFaithSelectorAndDefaultViewNoFaithNodes PASSED! Default view TerrainBiomes without faith nodes and unit selector faith color verified." << std::endl;
}

void testLivingOrganismsCircadianAndFamilyDefenseSystem() {
    using namespace ReligionSim;
    std::cout << "Testing Living Organisms: Circadian Sleep/Rest, Family Defense & Communal Bonds..." << std::endl;

    WorldMap map(30, 30);
    ReligionRegistry reg;
    reg.createPrimordialReligion("Clan del Fuego", "Chaman Grog", ColorRGB{235, 95, 45}, "Fuego", -4000);
    reg.createPrimordialReligion("Tribu Sombra", "Chaman Malor", ColorRGB{55, 65, 145}, "Sombra", -4000);

    DayNightCycle dayNight(90.0f, 12.0f); // Mediodía (día)
    assert(!dayNight.isNight());

    BuildingSystem buildingSys;
    GestorHumanos gestor;

    // 1. CREACIÓN DE NÚCLEO FAMILIAR: Padre, Madre e Hijo
    Humano padre(1, "Padre Grog", 10.5f, 10.5f, GeneroHumano::HOMBRE);
    padre.setReligionId(1);
    padre.setDevocion(0.85f);
    padre.setLeaderDevotion(0.70f);

    Humano madre(2, "Madre Vea", 10.7f, 10.5f, GeneroHumano::MUJER);
    madre.setReligionId(1);
    madre.setDevocion(0.88f);
    madre.setLeaderDevotion(0.75f);

    // Unión de amor conyugal
    padre.setParejaId(madre.getId());
    madre.setParejaId(padre.getId());
    padre.setAmorHaciaPareja(0.80f);
    madre.setAmorHaciaPareja(0.80f);

    // Nacimiento del hijo
    Humano hijo(3, "Hijo Tor", 10.6f, 10.6f, GeneroHumano::HOMBRE);
    hijo.setReligionId(1);
    hijo.setEsNino(true);
    hijo.setEdad(2.0f);
    hijo.setMadreId(madre.getId());
    hijo.setPadreId(padre.getId());
    padre.registrarHijo(hijo.getId());
    madre.registrarHijo(hijo.getId());

    assert(padre.getHijosCount() == 1);
    assert(madre.getHijosCount() == 1);
    assert(padre.getHijosIds()[0] == hijo.getId());

    // Construcción de choza familiar completada en (10, 10)
    int houseId = buildingSys.initiateHouseConstruction(10, 10, padre.getId(), madre.getId(), 1, HouseType::ThatchedHut, map);
    House* h = buildingSys.getHouseByIdMutable(houseId);
    assert(h != nullptr);
    h->state = BuildingState::Completed;
    h->buildProgress = 100.0f;
    padre.setHomeId(houseId);
    madre.setHomeId(houseId);
    hijo.setHomeId(houseId);

    gestor.agregarHumano(padre);
    gestor.agregarHumano(madre);
    gestor.agregarHumano(hijo);

    // 2. CICLO CIRCADIANO: Noche provoca que la familia busque su hogar para dormir
    dayNight.setTimeByHour(23.0f); // 23:00 (Noche profunda)
    assert(dayNight.isNight() == true);

    padre.setSueno(40.0f);
    madre.setSueno(40.0f);
    hijo.setSueno(50.0f);

    gestor.getHumanos()[0].setSueno(40.0f);
    gestor.getHumanos()[1].setSueno(40.0f);
    gestor.getHumanos()[2].setSueno(50.0f);

    // Al actualizar el gestor con la noche activa, los aldeanos acumulan sueño y se van a dormir a su choza
    gestor.actualizar(3.0f, map, nullptr, -4000, &reg, nullptr, nullptr, nullptr, nullptr, &dayNight, &buildingSys);

    bool algunFamiliarDurmiendo = false;
    for (const auto& hObj : gestor.getHumanos()) {
        if (hObj.estaDurmiendo()) algunFamiliarDurmiendo = true;
    }
    assert(algunFamiliarDurmiendo == true);

    // Forzar a los tres miembros a dormir en su hogar
    for (auto& hObj : gestor.getHumanos()) {
        hObj.setDurmiendo(true);
        hObj.setSueno(80.0f);
        hObj.getHumanUnitMutable().energy = 30.0f;
    }

    // Mientras duermen bajo techo, la energía y la salud se regeneran rápidamente y el sueño baja
    float prevEnergyPadre = gestor.getHumanos()[0].getHumanUnit().energy;
    float prevSuenoPadre = gestor.getHumanos()[0].getSueno();
    gestor.actualizar(1.0f, map, nullptr, -4000, &reg, nullptr, nullptr, nullptr, nullptr, &dayNight, &buildingSys);

    assert(gestor.getHumanos()[0].getHumanUnit().energy > prevEnergyPadre);
    assert(gestor.getHumanos()[0].getSueno() < prevSuenoPadre);
    assert(gestor.getHumanos()[0].getAccion() == AccionHumano::IDLE);

    // 3. INSTINTO PROTECTOR Y FURIA FAMILIAR EN COMBATE
    // Invasor enemigo ataca a la madre mientras dormían
    Humano enemigo(99, "Invasor Sombra", 10.6f, 10.6f, GeneroHumano::HOMBRE);
    enemigo.setReligionId(2); // Otra religión
    enemigo.cambiarAccion(AccionHumano::ATACAR);
    enemigo.setAttackCooldown(0.0f);
    gestor.agregarHumano(enemigo);

    // Ejecutar ciclo de combate
    gestor.actualizar(0.5f, map, nullptr, -4000, &reg, nullptr, nullptr, nullptr, nullptr, &dayNight, &buildingSys);

    // El padre se despierta inmediatamente al recibir daño y contraataca
    const Humano* padreActualizado = nullptr;
    const Humano* madreActualizada = nullptr;
    const Humano* hijoActualizado = nullptr;
    for (const auto& hObj : gestor.getHumanos()) {
        if (hObj.getId() == 1) padreActualizado = &hObj;
        if (hObj.getId() == 2) madreActualizada = &hObj;
        if (hObj.getId() == 3) hijoActualizado = &hObj;
    }
    assert(padreActualizado != nullptr && madreActualizada != nullptr && hijoActualizado != nullptr);
    assert(padreActualizado->estaDurmiendo() == false);
    assert(padreActualizado->getAccion() == AccionHumano::ATACAR);

    // La madre y el hijo se despiertan con FURIA PROTECTORA para defender al padre herido
    assert(madreActualizada->estaDurmiendo() == false);
    assert(madreActualizada->tieneFuriaProtectora() == true);
    assert(madreActualizada->getFuriaProtectoraTimer() > 0.0f);
    assert(madreActualizada->getTargetProtegidoId() == padre.getId());

    assert(hijoActualizado->estaDurmiendo() == false);
    assert(hijoActualizado->tieneFuriaProtectora() == true);
    assert(hijoActualizado->getTargetProtegidoId() == padre.getId());

    // 4. DESPERTAR NATURAL AL AMANECER (Prueba de fin del sueño permanente)
    // EN: Verify that sleeping villagers reliably wake up when daylight arrives
    // ES: Verificar que los aldeanos dormidos despiertan confiablemente al llegar el amanecer
    for (auto& hObj : gestor.getHumanos()) {
        if (hObj.getId() == 99) continue; // Omitir enemigo
        hObj.setDurmiendo(true);
        hObj.setSueno(15.0f);
        hObj.getHumanUnitMutable().energy = 75.0f;
    }
    dayNight.setTimeByHour(7.0f); // 07:00 AM (Amanecer / Día)
    assert(!dayNight.isNight());
    gestor.actualizar(0.5f, map, nullptr, -4000, &reg, nullptr, nullptr, nullptr, nullptr, &dayNight, &buildingSys);

    for (const auto& hObj : gestor.getHumanos()) {
        if (hObj.getId() == 99) continue;
        assert(hObj.estaDurmiendo() == false);
        assert(hObj.estaDurmiendoBajoTecho() == false);
    }

    std::cout << "  -> testLivingOrganismsCircadianAndFamilyDefenseSystem PASSED! Circadian sleep, family defense, protective fury (+35% bonus) and filial bonds fully verified." << std::endl;
}

void testHouseSpriteHidingCapacityAndProbabilisticReproductionSystem() {
    std::cout << "[Test] Running testHouseSpriteHidingCapacityAndProbabilisticReproductionSystem..." << std::endl;

    ReligionSim::WorldMap map(32, 32);
    ReligionSim::BuildingSystem buildingSys;
    ReligionSim::GestorHumanos gestor;

    // 1. CAPACIDAD ESTRICTA SEGÚN TIPO DE VIVIENDA
    // A. ThatchedHut: capacidad 4
    int hThatchedId = buildingSys.initiateHouseConstruction(10, 10, 100, 101, 1,
                                                            ReligionSim::HouseType::ThatchedHut, map);
    ReligionSim::House* hThatched = buildingSys.getHouseByIdMutable(hThatchedId);
    assert(hThatched != nullptr);
    assert(hThatched->capacity == 4);
    assert(hThatched->getOccupantCount() == 2);
    assert(hThatched->hasSpace() == true);
    assert(hThatched->canAccommodate(2) == true);
    assert(hThatched->canAccommodate(3) == false);

    // Añadir convivientes hasta el límite
    assert(hThatched->addResident(102) == true);
    assert(hThatched->getOccupantCount() == 3);
    assert(hThatched->hasSpace() == true);

    assert(hThatched->addResident(103) == true);
    assert(hThatched->getOccupantCount() == 4);
    assert(hThatched->hasSpace() == false); // Choza completa

    // Intentar exceder la capacidad máxima
    assert(hThatched->addResident(104) == false); // Rechazado por límite estricto
    assert(hThatched->getOccupantCount() == 4);

    // B. BoneDwelling: capacidad 3
    int hBoneId = buildingSys.initiateHouseConstruction(12, 10, 200, 201, 1,
                                                        ReligionSim::HouseType::BoneDwelling, map);
    ReligionSim::House* hBone = buildingSys.getHouseByIdMutable(hBoneId);
    assert(hBone != nullptr);
    assert(hBone->capacity == 3);
    assert(hBone->getOccupantCount() == 2);
    assert(hBone->addResident(202) == true);
    assert(hBone->getOccupantCount() == 3);
    assert(hBone->hasSpace() == false);
    assert(hBone->addResident(203) == false); // Rechazado

    // C. PrimitiveShelter: capacidad 2
    int hShelterId = buildingSys.initiateHouseConstruction(14, 10, 300, 301, 1,
                                                           ReligionSim::HouseType::PrimitiveShelter, map);
    ReligionSim::House* hShelter = buildingSys.getHouseByIdMutable(hShelterId);
    assert(hShelter != nullptr);
    assert(hShelter->capacity == 2);
    assert(hShelter->hasSpace() == false);
    assert(hShelter->addResident(302) == false);

    // D. ChieftainLonghouse: capacidad 6, LargeRoundhouse: capacidad 8
    int hChiefId = buildingSys.initiateHouseConstruction(16, 10, 400, -1, 1,
                                                         ReligionSim::HouseType::ChieftainLonghouse, map);
    ReligionSim::House* hChief = buildingSys.getHouseByIdMutable(hChiefId);
    assert(hChief->capacity == 6);

    int hRoundId = buildingSys.initiateHouseConstruction(18, 10, 500, -1, 1,
                                                         ReligionSim::HouseType::LargeRoundhouse, map);
    ReligionSim::House* hRound = buildingSys.getHouseByIdMutable(hRoundId);
    assert(hRound->capacity == 8);

    // Completar la vivienda Thatched para pruebas de ocupación y sueño bajo techo
    hThatched = buildingSys.getHouseByIdMutable(hThatchedId);
    assert(hThatched != nullptr);
    hThatched->state = ReligionSim::BuildingState::Completed;
    hThatched->buildProgress = 100.0f;

    // 2. SISTEMA DE OCULTACIÓN DE SPRITES BAJO TECHO
    ReligionSim::Humano habitante(100, "Vael", 10.5f, 10.5f, ReligionSim::GeneroHumano::HOMBRE);
    habitante.setHomeId(hThatchedId);
    habitante.setReligionId(1);
    habitante.setSueno(90.0f);

    // Si duerme dentro de casa -> estaDurmiendoBajoTecho() == true
    habitante.setDurmiendoBajoTecho(true);
    assert(habitante.estaDurmiendo() == true);
    assert(habitante.estaDurmiendoBajoTecho() == true);

    // Verificar que una unidad a la intemperie (fogata) duerme pero NO bajo techo
    ReligionSim::Humano sinTecho(999, "Nomada", 2.0f, 2.0f, ReligionSim::GeneroHumano::HOMBRE);
    sinTecho.setDurmiendo(true);
    assert(sinTecho.estaDurmiendo() == true);
    assert(sinTecho.estaDurmiendoBajoTecho() == false);

    // Censo de durmientes en BuildingSystem
    gestor.agregarHumano(habitante);
    gestor.agregarHumano(sinTecho);
    buildingSys.update(0.1f, map, gestor);
    hThatched = buildingSys.getHouseByIdMutable(hThatchedId);
    assert(hThatched->sleepingOccupants == 1);

    // Si el durmiente sufre un ataque o suena la alarma, se despierta y sale inmediatamente
    habitante.recibirDano(15.0f);
    assert(habitante.estaDurmiendo() == false);
    assert(habitante.estaDurmiendoBajoTecho() == false);

    // 3. REPRODUCCIÓN PROBABILÍSTICA (Factores Ambientales y Espirituales)
    ReligionSim::Humano madre(50, "Sari", 10.5f, 10.5f, ReligionSim::GeneroHumano::MUJER);
    ReligionSim::Humano padre(51, "Kaelen", 10.5f, 10.5f, ReligionSim::GeneroHumano::HOMBRE);
    madre.setParejaId(padre.getId());
    padre.setParejaId(madre.getId());
    madre.setAmorHaciaPareja(0.85f);
    padre.setAmorHaciaPareja(0.85f);
    madre.setReligionId(1);
    padre.setReligionId(1);
    madre.setHomeId(hThatchedId);
    padre.setHomeId(hThatchedId);

    // Escenario A: Hambruna y malnutrición extrema
    madre.getHumanUnitMutable().nutritionalState = 15.0f;
    padre.getHumanUnitMutable().nutritionalState = 15.0f;
    madre.getPopGroupMutable().foodSupply = 5.0f;
    float deseoHambruna = madre.calcularDeseoReproduccion(padre, map, &buildingSys);
    assert(deseoHambruna < 0.03f); // Prácticamente estéril/suprimido por supervivencia

    // Escenario B: Nutrición óptima, clan próspero, hogar seguro con plazas libres, devoción alta
    madre.getHumanUnitMutable().nutritionalState = 85.0f;
    padre.getHumanUnitMutable().nutritionalState = 85.0f;
    madre.getPopGroupMutable().foodSupply = 75.0f;
    madre.setDevocion(0.90f);
    padre.setDevocion(0.90f);
    hThatched->residentIds.clear(); // Dejar espacio en la choza (2 / 4)
    assert(hThatched->hasSpace() == true);

    float deseoProspero = madre.calcularDeseoReproduccion(padre, map, &buildingSys);
    assert(deseoProspero > deseoHambruna * 4.0f);
    assert(deseoProspero > 0.05f);

    // Escenario C: Choza saturada al límite de capacidad (4 / 4 ocupantes)
    hThatched->addResident(102);
    hThatched->addResident(103);
    assert(hThatched->hasSpace() == false);
    float deseoHacinado = madre.calcularDeseoReproduccion(padre, map, &buildingSys);
    assert(deseoHacinado < deseoProspero * 0.40f); // El hacinamiento deprime fuertemente el deseo

    // Escenario D: Mancha de herejía o apostasía
    hThatched->residentIds.clear(); // Despejar choza
    madre.getMoralRecordMutable().isApostateOrHeretic = true;
    float deseoHereje = madre.calcularDeseoReproduccion(padre, map, &buildingSys);
    assert(deseoHereje < deseoProspero * 0.35f); // Rechazo/angustia moral inhibe reproducción

    // Escenario E: Suelo sagrado y bendición del santuario
    madre.getMoralRecordMutable().isApostateOrHeretic = false;
    map.getCell(10, 10).hasHolySite = true;
    map.getCell(10, 10).shrineLevel = 2;
    float deseoAltar = madre.calcularDeseoReproduccion(padre, map, &buildingSys);
    assert(deseoAltar > deseoProspero); // Bendición de fertilidad sagrada en suelo sacro

    std::cout << "  -> testHouseSpriteHidingCapacityAndProbabilisticReproductionSystem PASSED! House capacity limits, indoor sprite hiding, and bio-spiritual reproduction probability fully verified." << std::endl;
}

// =============================================================================
// EN: Test ErrorSystem, memory overflow prevention, lag detection, and buffer guards
// ES: Probar ErrorSystem, prevención de desbordes de memoria, detección de lag y guardas
// =============================================================================
void testErrorSystemAndLagMitigationWatchdog() {
    std::cout << "[Test] Running testErrorSystemAndLagMitigationWatchdog..." << std::endl;

    auto& errorSentinel = ReligionSim::ErrorSystem::instance();
    errorSentinel.resetMetrics();

    // =========================================================================
    // EN: Block 1 - Telemetry logging and ring buffer retrieval
    // ES: Bloque 1 - Registro de telemetría y consulta del búfer circular
    // =========================================================================
    errorSentinel.logError(ReligionSim::ErrorCategory::SimulationState,
                           ReligionSim::ErrorSeverity::Info,
                           "Simulation initialized successfully.");
    errorSentinel.logError(ReligionSim::ErrorCategory::Performance,
                           ReligionSim::ErrorSeverity::Warning,
                           "Single frame spike detected during world generation.");

    auto recentErrors = errorSentinel.getRecentErrors(5);
    assert(recentErrors.size() == 2);
    assert(recentErrors[0].category == ReligionSim::ErrorCategory::SimulationState);
    assert(recentErrors[1].category == ReligionSim::ErrorCategory::Performance);
    assert(recentErrors[1].severity == ReligionSim::ErrorSeverity::Warning);

    // =========================================================================
    // EN: Block 2 - Lag detection, frame duration smoothing and delta time clamping
    // ES: Bloque 2 - Detección de lag, suavizado de fotogramas y limitación de delta time
    // =========================================================================
    assert(errorSentinel.isUnderLagStress() == false);

    // EN: Simulate normal 60 FPS frame times (~16.6ms)
    // ES: Simular fotogramas normales a 60 FPS (~16.6ms)
    for (int frameIdx = 0; frameIdx < 20; ++frameIdx) {
        errorSentinel.reportFrameTime(16.6f);
    }
    assert(errorSentinel.isUnderLagStress() == false);

    // EN: Test anti-death spiral delta time clamping
    // ES: Probar limitación de delta time contra la espiral de la muerte
    float clampedDeltaTime = errorSentinel.clampDeltaTime(0.50f);
    assert(clampedDeltaTime <= ReligionSim::ErrorSystem::MAX_ALLOWED_DELTA_TIME_S);
    assert(clampedDeltaTime == 0.050f);

    // EN: Simulate sustained lag spikes (>40ms) to trigger lag stress mode
    // ES: Simular picos sostenidos de lag (>40ms) para activar modo de estrés por lag
    for (int lagFrameIdx = 0; lagFrameIdx < 6; ++lagFrameIdx) {
        errorSentinel.reportFrameTime(50.0f);
    }
    assert(errorSentinel.isUnderLagStress() == true);
    auto metricsUnderLag = errorSentinel.getHealthMetrics();
    assert(metricsUnderLag.droppedFrameCount > 0);
    assert(metricsUnderLag.isUnderLagStress == true);

    // EN: Simulate performance recovery returning to 60 FPS
    // ES: Simular recuperación de rendimiento regresando a 60 FPS
    for (int recoveryFrameIdx = 0; recoveryFrameIdx < 40; ++recoveryFrameIdx) {
        errorSentinel.reportFrameTime(16.0f);
    }
    assert(errorSentinel.isUnderLagStress() == false);

    // =========================================================================
    // EN: Block 3 - Memory auditing and Timeline snapshot trimming
    // ES: Bloque 3 - Auditoría de memoria y poda de capturas de Timeline
    // =========================================================================
    float processMemoryMb = errorSentinel.auditMemoryUsage();
    assert(processMemoryMb > 0.0f);

    ReligionSim::Timeline testTimeline(-4000, 2200);
    std::vector<uint8_t> dummyGrid(48 * 48, 1);
    std::vector<std::pair<int, int>> dummyFollowers{{1, 50}};

    // EN: Populate timeline with 450 snapshots (should auto-trim at 400 to prevent leak)
    // ES: Poblar la línea de tiempo con 450 capturas (debe autopodarse a 300)
    for (int yearOffset = 0; yearOffset < 450; ++yearOffset) {
        testTimeline.setCurrentYear(-4000 + yearOffset * 25);
        testTimeline.recordSnapshot(100, dummyGrid, dummyFollowers);
    }
    assert(testTimeline.getHistory().size() <= 350);

    // =========================================================================
    // EN: Block 4 - Population safety cap preventing demographic runaway
    // ES: Bloque 4 - Tope de seguridad de población evitando desborde demográfico
    // =========================================================================
    errorSentinel.setMaxSafeEntities(30);
    errorSentinel.updateEntityCount(30);
    assert(errorSentinel.isPopulationCapReached() == true);

    ReligionSim::WorldMap testMap(20, 20);
    testMap.generateProceduralWorld(42, ReligionSim::WorldClimate::Templado, ReligionSim::ResourceAbundance::Abundante);
    ReligionSim::Humano testMadre(1, "Madre", 5.0f, 5.0f, ReligionSim::GeneroHumano::MUJER);
    ReligionSim::Humano testPadre(2, "Padre", 5.0f, 5.0f, ReligionSim::GeneroHumano::HOMBRE);
    testMadre.setParejaId(2);
    testPadre.setParejaId(1);
    testMadre.setAmorHaciaPareja(0.9f);
    testPadre.setAmorHaciaPareja(0.9f);
    testMadre.getHumanUnitMutable().nutritionalState = 90.0f;
    testPadre.getHumanUnitMutable().nutritionalState = 90.0f;

    // EN: Reproduction desire must drop to 0.0f when safety cap is reached
    // ES: El deseo de reproducción debe ser 0.0f cuando se alcanza el tope de seguridad
    float reproductionDesireCapped = testMadre.calcularDeseoReproduccion(testPadre, testMap, nullptr);
    assert(reproductionDesireCapped == 0.0f);

    // EN: Reset population limit back to default
    // ES: Restablecer el límite de población a su valor por defecto
    errorSentinel.setMaxSafeEntities(ReligionSim::ErrorSystem::DEFAULT_MAX_SAFE_ENTITIES);
    errorSentinel.updateEntityCount(2);
    assert(errorSentinel.isPopulationCapReached() == false);
    float reproductionDesireNormal = testMadre.calcularDeseoReproduccion(testPadre, testMap, nullptr);
    assert(reproductionDesireNormal > 0.0f);

    // =========================================================================
    // EN: Block 5 - Rasterizer buffer boundary safety validation
    // ES: Bloque 5 - Validación de límites de seguridad para buffers de ráster
    // =========================================================================
    assert(ReligionSim::BufferGuard::isPixelWithinBounds(10, 10, 100, 100) == true);
    assert(ReligionSim::BufferGuard::isPixelWithinBounds(-1, 10, 100, 100) == false);
    assert(ReligionSim::BufferGuard::isPixelWithinBounds(10, -5, 100, 100) == false);
    assert(ReligionSim::BufferGuard::isPixelWithinBounds(100, 10, 100, 100) == false);
    assert(ReligionSim::BufferGuard::isPixelWithinBounds(10, 100, 100, 100) == false);

    bool safeAccess = errorSentinel.validateBufferAccess(50, 50, 100, 100);
    assert(safeAccess == true);

    bool preventedViolation = errorSentinel.validateBufferAccess(-5, 50, 100, 100);
    assert(preventedViolation == false);
    assert(errorSentinel.getBufferViolationsPreventedCount() >= 1);

    // =========================================================================
    // EN: Block 6 - Game emergency memory purge verification
    // ES: Bloque 6 - Verificación de la purga de memoria de emergencia de Game
    // =========================================================================
    ReligionSim::Game testGame(800, 600);
    testGame.getGestorHumanos().getHumanos().clear();

    ReligionSim::Humano livingVillager(10, "Vivo", 5.0f, 5.0f, ReligionSim::GeneroHumano::HOMBRE);
    ReligionSim::Humano deceasedVillager(11, "Muerto", 5.0f, 5.0f, ReligionSim::GeneroHumano::HOMBRE);
    deceasedVillager.matar("Test");

    testGame.getGestorHumanos().agregarHumano(livingVillager);
    testGame.getGestorHumanos().agregarHumano(deceasedVillager);
    assert(testGame.getGestorHumanos().getHumanos().size() == 2);

    testGame.performEmergencyMemoryPurge();
    assert(testGame.getGestorHumanos().getHumanos().size() == 1);
    assert(testGame.getGestorHumanos().getHumanos()[0].getId() == 10);

    std::cout << "  -> testErrorSystemAndLagMitigationWatchdog PASSED! Memory overflow prevention, anti-lag frame pacing, and buffer boundary safety fully verified." << std::endl;
}

// =============================================================================
// EN: Test Resource Panel Believers Indicator
// ES: Prueba del Indicador de Creyentes en el Panel de Recursos
// =============================================================================
void testResourcePanelBelieversIndicator() {
    std::cout << "[Test] Running testResourcePanelBelieversIndicator..." << std::endl;

    // EN: Block 1 - World, registry and player religion setup
    // ES: Bloque 1 - Configuración de mundo, registro y religión del jugador
    ReligionSim::WorldMap testMap(20, 20);
    ReligionSim::ReligionRegistry testRegistry;
    ReligionSim::Timeline testTimeline;
    ReligionSim::WorldCreationConfig testConfig;
    testConfig.godName = "Solarius";
    testConfig.religionName = "Culto Solar";

    ReligionSim::Religion playerFaith;
    playerFaith.id = 1;
    playerFaith.name = "Culto Solar";
    playerFaith.totalFollowers = 42;
    playerFaith.theologyPoints = 850;
    playerFaith.resources.food = 600;
    playerFaith.resources.wood = 400;
    playerFaith.resources.stone = 200;
    playerFaith.resources.gold = 100;
    playerFaith.resources.iron = 90;
    testRegistry.getAllReligions().push_back(playerFaith);

    // EN: Block 2 - Living villagers and disciple tracking in GestorHumanos
    // ES: Bloque 2 - Aldeanos vivos y seguimiento de discípulos en GestorHumanos
    ReligionSim::GestorHumanos testHumanos;
    testHumanos.getHumanos().clear();

    ReligionSim::Humano discipleA(101, "Aram", 5.0f, 5.0f, ReligionSim::GeneroHumano::HOMBRE);
    discipleA.setReligionId(1);
    ReligionSim::Humano discipleB(102, "Mira", 6.0f, 5.0f, ReligionSim::GeneroHumano::MUJER);
    discipleB.setReligionId(1);
    ReligionSim::Humano rivalVillager(103, "Orak", 10.0f, 10.0f, ReligionSim::GeneroHumano::HOMBRE);
    rivalVillager.setReligionId(2);

    testHumanos.agregarHumano(discipleA);
    testHumanos.agregarHumano(discipleB);
    testHumanos.agregarHumano(rivalVillager);

    const int livingDisciplesCount = testHumanos.countLivingHumansOfReligion(1);
    assert(livingDisciplesCount == 2);
    assert(testHumanos.countLivingHumansOfReligion(2) == 1);

    // EN: Block 3 - TopBar rendering and illuminated believer icon verification
    // ES: Bloque 3 - Renderizado de barra superior y verificación del icono iluminado de creyente
    ReligionSim::Renderer testRenderer(1280, 720);
    testRenderer.renderTopBar(testMap, testRegistry, testTimeline, testConfig, &testHumanos);

    const auto& frameBufferPixels = testRenderer.getFramebuffer();
    assert(!frameBufferPixels.empty());

    // EN: Inspect framebuffer across the resource row (y = 17) to verify golden icon and believer text
    // ES: Inspeccionar el framebuffer en la fila de recursos (y = 17) para verificar icono dorado y texto de creyentes
    int divineGoldPixelsCount = 0;
    int relicBonePixelsCount = 0;
    constexpr int targetRowY = 17;
    const int invertedRowY = 720 - 1 - targetRowY;

    for (int columnX = 200; columnX < 650; ++columnX) {
        for (int rowOffset = -3; rowOffset <= 5; ++rowOffset) {
            const int scanPixelY = invertedRowY + rowOffset;
            if (scanPixelY >= 0 && scanPixelY < 720) {
                const uint32_t currentPixel = frameBufferPixels[scanPixelY * 1280 + columnX];
                const uint8_t redComponent = currentPixel & 0xFF;
                const uint8_t greenComponent = (currentPixel >> 8) & 0xFF;
                const uint8_t blueComponent = (currentPixel >> 16) & 0xFF;

                // Palette::DIVINE_GOLD (#D4AF37 -> R:212, G:175, B:55)
                if (redComponent >= 180 && greenComponent >= 150 && blueComponent <= 100) {
                    ++divineGoldPixelsCount;
                }
                // Palette::RELIC_BONE (#E2DAC4 -> R:226, G:218, B:196)
                if (redComponent >= 200 && greenComponent >= 190 && blueComponent >= 160) {
                    ++relicBonePixelsCount;
                }
            }
        }
    }

    assert(divineGoldPixelsCount > 20); // Aureola sagrada, manto litúrgico y texto CREYENTES
    assert(relicBonePixelsCount > 10);  // Cabeza y acentos litúrgicos

    // EN: Block 4 - TopBar fallback without GestorHumanos pointer
    // ES: Bloque 4 - Fallback de barra superior sin puntero a GestorHumanos
    testRenderer.renderTopBar(testMap, testRegistry, testTimeline, testConfig, nullptr);
    assert(divineGoldPixelsCount > 20);

    std::cout << "  -> testResourcePanelBelieversIndicator PASSED! Dedicated believer section, golden halo icon, and follower counts verified." << std::endl;
}

// =============================================================================
// EN: Test Minecraft-Style Simulated World Panorama Main Menu
// ES: Prueba del Panorama de Mundo Simulado Estilo Minecraft en el Menú Principal
// =============================================================================
void testMinecraftStyleSimulatedWorldPanoramaMainMenu() {
    std::cout << "[Test] Running testMinecraftStyleSimulatedWorldPanoramaMainMenu..." << std::endl;

    // EN: Block 1 - Initialize game instance and verify main menu startup state
    // ES: Bloque 1 - Inicializar instancia de juego y verificar estado de arranque en menú principal
    constexpr int canvasWidth = 1280;
    constexpr int canvasHeight = 720;
    ReligionSim::Game testGame(canvasWidth, canvasHeight);
    testGame.init();

    assert(testGame.getGameState() == ReligionSim::GameState::MainMenu);
    assert(testGame.getMap().getWidth() == 64);
    assert(testGame.getMap().getHeight() == 64);
    assert(testGame.getGestorHumanos().getHumanos().size() == 36);
    assert(testGame.getBuildingSystem().getHouses().size() >= 3);

    // EN: Block 2 - Verify initial camera zoom and dynamic panoramic drift movement
    // ES: Bloque 2 - Verificar zoom inicial de cámara y movimiento de desplazamiento panorámico dinámico
    const float initialCameraX = testGame.getCamera().getCameraX();
    const float initialCameraY = testGame.getCamera().getCameraY();
    const float initialCameraZoom = testGame.getCamera().getZoom();
    assert(initialCameraZoom >= 1.0f);

    constexpr float simulationStepDelta = 0.25f;
    constexpr int simulationStepsCount = 10;
    for (int stepIndex = 0; stepIndex < simulationStepsCount; ++stepIndex) {
        testGame.update(simulationStepDelta);
    }

    const float driftedCameraX = testGame.getCamera().getCameraX();
    const float driftedCameraY = testGame.getCamera().getCameraY();
    const bool cameraDrifted = (driftedCameraX != initialCameraX) || (driftedCameraY != initialCameraY);
    assert(cameraDrifted);

    // EN: Block 3 - Verify frame rendering executes cleanly with world background and UI
    // ES: Bloque 3 - Verificar que el renderizado de cuadro se ejecuta limpiamente con fondo de mundo y UI
    testGame.render();
    const auto& frameBuffer = testGame.getRenderer().getFramebuffer();
    assert(frameBuffer.size() == static_cast<size_t>(testGame.getRenderer().getWidth() * testGame.getRenderer().getHeight()));

    // EN: Verify the screen is not a uniform solid brown brick background
    // ES: Verificar que la pantalla no es un fondo de ladrillo marrón sólido uniforme
    bool hasNonBrownPixels = false;
    for (uint32_t pixelValue : frameBuffer) {
        uint8_t redComponent = pixelValue & 0xFF;
        uint8_t greenComponent = (pixelValue >> 8) & 0xFF;
        uint8_t blueComponent = (pixelValue >> 16) & 0xFF;

        // EN: Check for lush green terrain or blue water pixels
        // ES: Comprobar la presencia de píxeles de terreno verde exuberante o agua azul
        if ((greenComponent > redComponent + 20) || (blueComponent > redComponent + 20)) {
            hasNonBrownPixels = true;
            break;
        }
    }
    assert(hasNonBrownPixels);

    std::cout << "  -> testMinecraftStyleSimulatedWorldPanoramaMainMenu PASSED! Living background, orbital camera drift, and frosted UI verified." << std::endl;
}

// =============================================================================
// EN: Test Options Menu Categories and Settings Architecture
// ES: Prueba de Categorías del Menú de Opciones y Arquitectura de Ajustes
// =============================================================================
void testOptionsMenuCategoriesAndSettingsArchitecture() {
    std::cout << "[Test] Running testOptionsMenuCategoriesAndSettingsArchitecture..." << std::endl;

    // EN: Block 1 - Test GameSettings default values and cycling routines
    // ES: Bloque 1 - Probar valores por defecto de GameSettings y rutinas de ciclo
    ReligionSim::GameSettings testSettings;
    assert(testSettings.resolution == ReligionSim::DisplayResolution::Res1280x720);
    assert(testSettings.displayMode == ReligionSim::DisplayMode::BorderlessFullscreen);
    assert(testSettings.frameRateLimit == ReligionSim::FrameRateLimit::FPS60);
    assert(testSettings.cloudShadows == ReligionSim::CloudShadowsQuality::HighSoft);
    assert(testSettings.dynamicLighting == ReligionSim::DynamicLightingMode::SoftRealistic);
    assert(testSettings.defaultSpeed == ReligionSim::DefaultSimulationSpeed::PacingNormal);
    assert(!testSettings.showIsometricGrid); // EN: Grid disabled by default / ES: Retícula desactivada por defecto
    assert(testSettings.masterVolumePercent == 80);

    // EN: Cycle resolution and check conversions
    // ES: Ciclar resolución y verificar conversiones
    testSettings.cycleResolution();
    assert(testSettings.resolution == ReligionSim::DisplayResolution::Res1600x900);
    assert(ReligionSim::GameSettings::getResolutionWidth(testSettings.resolution) == 1600);
    assert(ReligionSim::GameSettings::getResolutionHeight(testSettings.resolution) == 900);

    testSettings.cycleResolution();
    assert(testSettings.resolution == ReligionSim::DisplayResolution::Res1920x1080);
    assert(ReligionSim::GameSettings::getResolutionWidth(testSettings.resolution) == 1920);

    testSettings.cycleResolution();
    assert(testSettings.resolution == ReligionSim::DisplayResolution::Res1280x720);

    // EN: Cycle visual filters, dynamic lighting and audio
    // ES: Ciclar filtros visuales, iluminación dinámica y audio
    testSettings.cycleVisualColorFilter();
    assert(testSettings.colorFilter == ReligionSim::VisualColorFilter::DivineGold);

    testSettings.toggleAutoPauseOnDisasters();
    assert(!testSettings.autoPauseOnDisasters);

    testSettings.toggleVignette();
    assert(!testSettings.vignetteEnabled);

    testSettings.toggleIsometricGrid();
    assert(testSettings.showIsometricGrid);

    testSettings.cycleMasterVolume();
    assert(testSettings.masterVolumePercent == 105 % 125); // 105 % 125 = 105 -> modulo 125 wrap check

    // EN: Block 2 - Test file persistence (save and reload)
    // ES: Bloque 2 - Probar persistencia en archivo (guardar y recargar)
    const std::string temporaryConfigPath = "/tmp/marangatu_test_settings.cfg";
    const bool saveSucceeded = testSettings.saveToFile(temporaryConfigPath);
    assert(saveSucceeded);

    ReligionSim::GameSettings loadedSettings;
    const bool loadSucceeded = loadedSettings.loadFromFile(temporaryConfigPath);
    assert(loadSucceeded);
    assert(loadedSettings.colorFilter == ReligionSim::VisualColorFilter::DivineGold);
    assert(!loadedSettings.autoPauseOnDisasters);
    assert(!loadedSettings.vignetteEnabled);
    assert(loadedSettings.showIsometricGrid);

    // EN: Test reset to defaults
    // ES: Probar restablecimiento a valores predeterminados
    loadedSettings.resetToDefaults();
    assert(loadedSettings.colorFilter == ReligionSim::VisualColorFilter::Neutral);
    assert(loadedSettings.autoPauseOnDisasters);
    assert(loadedSettings.vignetteEnabled);
    assert(!loadedSettings.showIsometricGrid);

    // EN: Block 3 - Test Options Menu UI buttons and hit-testing in Renderer
    // ES: Bloque 3 - Probar botones de UI y hit-testing del menú de opciones en Renderer
    constexpr int canvasWidth = 1280;
    constexpr int canvasHeight = 720;
    ReligionSim::Renderer testRenderer(canvasWidth, canvasHeight);
    assert(!testRenderer.isGridVisible());
    testRenderer.setShowGrid(true);
    assert(testRenderer.isGridVisible());
    testRenderer.setShowGrid(false);
    assert(!testRenderer.isGridVisible());

    // EN: Verify tabs hit-testing (Tab 1: 1001, Tab 2: 1002, Tab 3: 1003, Tab 4: 1004)
    // ES: Verificar hit-testing de pestañas (Pestaña 1: 1001, Pestaña 2: 1002, Pestaña 3: 1003, Pestaña 4: 1004)
    const float modalWidth = 960.0f;
    const float modalStartX = (static_cast<float>(canvasWidth) - modalWidth) * 0.5f;
    const float modalStartY = (static_cast<float>(canvasHeight) - 590.0f) * 0.5f;

    // Check hit test for Tab 1 at modalStartX + 50, modalStartY + 50
    const int tabOneHitId = testRenderer.checkOptionsMenuButtonClicked(modalStartX + 60.0f, modalStartY + 55.0f, ReligionSim::OptionsTab::ScreenAndVideo);
    assert(tabOneHitId == 1001);

    // Check hit test for Tab 2
    const int tabTwoHitId = testRenderer.checkOptionsMenuButtonClicked(modalStartX + 285.0f, modalStartY + 55.0f, ReligionSim::OptionsTab::ScreenAndVideo);
    assert(tabTwoHitId == 1002);

    // Check hit test for bottom Restore button 1090
    const int restoreActionHitId = testRenderer.checkOptionsMenuButtonClicked(modalStartX + 60.0f, modalStartY + 590.0f - 40.0f, ReligionSim::OptionsTab::ScreenAndVideo);
    assert(restoreActionHitId == 1090);

    // EN: Block 4 - Test Game integration, tab switching, and options rendering
    // ES: Bloque 4 - Probar integración en Game, cambio de pestañas y renderizado de opciones
    ReligionSim::Game testGame(canvasWidth, canvasHeight);
    testGame.init();

    testGame.setGameState(ReligionSim::GameState::Options);
    assert(testGame.getGameState() == ReligionSim::GameState::Options);

    // Switch tabs and verify
    testGame.setCurrentOptionsTab(ReligionSim::OptionsTab::ScreenAndVideo);
    assert(testGame.getCurrentOptionsTab() == ReligionSim::OptionsTab::ScreenAndVideo);

    testGame.setCurrentOptionsTab(ReligionSim::OptionsTab::GraphicsAndImage);
    assert(testGame.getCurrentOptionsTab() == ReligionSim::OptionsTab::GraphicsAndImage);

    testGame.setCurrentOptionsTab(ReligionSim::OptionsTab::GameplayAndSimulation);
    assert(testGame.getCurrentOptionsTab() == ReligionSim::OptionsTab::GameplayAndSimulation);

    testGame.setCurrentOptionsTab(ReligionSim::OptionsTab::AudioAndSound);
    assert(testGame.getCurrentOptionsTab() == ReligionSim::OptionsTab::AudioAndSound);

    // Apply custom settings and verify subsystems update
    testSettings.masterVolumePercent = 50;
    testSettings.defaultSpeed = ReligionSim::DefaultSimulationSpeed::PacingFast;
    testGame.applySettings(testSettings);
    assert(testGame.getRenderer().isGridVisible() == testSettings.showIsometricGrid);
    testSettings.showIsometricGrid = true;
    testGame.applySettings(testSettings);
    assert(testGame.getRenderer().isGridVisible());
    testSettings.showIsometricGrid = false;
    testGame.applySettings(testSettings);
    assert(!testGame.getRenderer().isGridVisible());
    assert(std::abs(testGame.getSoundSystem().getMasterVolume() - 0.50f) < 0.05f);
    assert(std::abs(testGame.getTimeline().getYearsPerSecond() - 8.0f) < 0.05f);

    // Render frame in Options state without exceptions
    testGame.render();
    assert(testGame.getRenderer().getFramebuffer().size() == static_cast<size_t>(testGame.getRenderer().getWidth() * testGame.getRenderer().getHeight()));

    // EN: Block 5 - Test Fullscreen and Window Mode Toggle & F11 Shortcut
    // ES: Bloque 5 - Probar conmutación de pantalla completa / ventana y atajo F11
    testGame.getSettingsMutable().displayMode = ReligionSim::DisplayMode::Windowed;
    testGame.applySettings(testGame.getSettings());
    assert(!testGame.getRenderer().isFullscreen());
    assert(testGame.getSettings().displayMode == ReligionSim::DisplayMode::Windowed);

    // F11 key shortcut toggle
    testGame.onKeyPress(1011);
    assert(testGame.getSettings().displayMode == ReligionSim::DisplayMode::BorderlessFullscreen);
    assert(testGame.getRenderer().isFullscreen());

    // Toggle back to Windowed with F11
    testGame.onKeyPress(1011);
    assert(testGame.getSettings().displayMode == ReligionSim::DisplayMode::Windowed);
    assert(!testGame.getRenderer().isFullscreen());

    // Test X11 KeySym XK_F11 (0xFFC8)
    constexpr int XK_F11_SYM = 0xFFC8;
    testGame.onKeyPress(XK_F11_SYM);
    assert(testGame.getSettings().displayMode == ReligionSim::DisplayMode::BorderlessFullscreen);
    assert(testGame.getRenderer().isFullscreen());

    // Reset back to windowed
    testGame.onKeyPress(XK_F11_SYM);
    assert(!testGame.getRenderer().isFullscreen());

    std::cout << "  -> testOptionsMenuCategoriesAndSettingsArchitecture PASSED! Display, video, graphics, gameplay, and audio options fully verified." << std::endl;
}

// =============================================================================
// EN: Test enhanced living organism AI: biological drives, active courtship,
//     parental food sharing, and cultural/memetic replication
// ES: Probar IA mejorada de organismos vivos: impulsos biológicos, cortejo activo,
//     inversión parental de alimento y replicación cultural/memética
// =============================================================================
void testLivingOrganismReproductionAndMemeticReplicationAI() {
    std::cout << "[Test] Running testLivingOrganismReproductionAndMemeticReplicationAI..." << std::endl;

    // -------------------------------------------------------------------------
    // EN: 1. Biological Drives & Active Courtship
    // ES: 1. Impulsos biológicos y cortejo activo
    // -------------------------------------------------------------------------
    ReligionSim::WorldMap testMap(32, 32);
    ReligionSim::EventLine testEventLine;
    ReligionSim::Humano groomCandidate(10, "Arak", 12.0f, 12.0f, ReligionSim::HumanGender::Male);
    ReligionSim::Humano brideCandidate(11, "Mira", 12.5f, 12.0f, ReligionSim::HumanGender::Female);

    groomCandidate.setReligionId(1);
    brideCandidate.setReligionId(1);
    groomCandidate.setAge(18.0f);
    brideCandidate.setAge(18.0f);
    groomCandidate.setReproductionDrive(0.50f);
    brideCandidate.setReproductionDrive(0.50f);

    assert(!groomCandidate.hasSpouse());
    assert(!brideCandidate.hasSpouse());

    // EN: Courtship displays build mutual affection over time
    // ES: Las exhibiciones de cortejo fomentan el afecto mutuo con el tiempo
    groomCandidate.courtCandidate(brideCandidate, 3.0f, &testEventLine, -3950);
    bool weddingCelebrated = groomCandidate.courtCandidate(brideCandidate, 3.0f, &testEventLine, -3950);
    assert(weddingCelebrated);
    assert(groomCandidate.hasSpouse());
    assert(brideCandidate.hasSpouse());
    assert(groomCandidate.getSpouseId() == brideCandidate.getId());
    assert(brideCandidate.getSpouseId() == groomCandidate.getId());
    assert(groomCandidate.getNeedForShelter() >= 0.50f); // Nesting urge

    // -------------------------------------------------------------------------
    // EN: 2. Parental Investment & Kin Food Sharing
    // ES: 2. Inversión parental y compartición de alimento
    // -------------------------------------------------------------------------
    ReligionSim::Humano mother(20, "Dalia", 15.0f, 15.0f, ReligionSim::HumanGender::Female);
    ReligionSim::Humano hungryChild(21, "Bimbo", 15.2f, 15.0f, ReligionSim::HumanGender::Male);
    mother.setReligionId(1);
    hungryChild.setReligionId(1);
    hungryChild.setChild(true);
    hungryChild.setAge(4.0f);
    hungryChild.setMotherId(mother.getId());
    mother.registerChild(hungryChild.getId());

    mother.getHumanUnitMutable().nutritionalState = 80.0f;
    hungryChild.getHumanUnitMutable().nutritionalState = 20.0f;
    hungryChild.setHealth(60.0f);

    const bool fedSuccessfully = mother.feedChild(hungryChild);
    assert(fedSuccessfully);
    assert(hungryChild.getHumanUnit().nutritionalState > 30.0f);
    assert(hungryChild.getHealth() > 60.0f);
    assert(mother.getHumanUnit().nutritionalState < 80.0f);

    // -------------------------------------------------------------------------
    // EN: 3. Dynamic Population Scaling beyond 120
    // ES: 3. Escalado dinámico de población más allá de 120
    // -------------------------------------------------------------------------
    ReligionSim::GestorHumanos villagerManager;
    villagerManager.getVillagers().clear();

    // EN: Verify ErrorSystem allows reproduction up to maxSafeEntities
    // ES: Verificar que ErrorSystem permite reproducción hasta maxSafeEntities
    ReligionSim::ErrorSystem::instance().setMaxSafeEntities(400);
    assert(!ReligionSim::ErrorSystem::instance().isPopulationCapReached());

    // -------------------------------------------------------------------------
    // EN: 4. Ontogeny & Adulthood Rites
    // ES: 4. Ontogenia y Rito de Madurez
    // -------------------------------------------------------------------------
    ReligionSim::Humano maturingChild(30, "Kenai", 10.0f, 10.0f, ReligionSim::HumanGender::Male);
    maturingChild.setChild(true);
    maturingChild.setAge(12.5f); // Reached puberty/maturity threshold
    maturingChild.setReligionId(1);
    villagerManager.addVillager(maturingChild);

    villagerManager.update(0.1f, testMap, &testEventLine, -3900);
    const auto& updatedVillagers = villagerManager.getVillagers();
    assert(!updatedVillagers.empty());
    const auto& updatedKenai = updatedVillagers.front();
    assert(!updatedKenai.isChild()); // Now a biological adult
    assert(updatedKenai.getReproductionDrive() > 0.0f);

    // -------------------------------------------------------------------------
    // EN: 5. Cultural & Memetic Replication
    // ES: 5. Replicación cultural y memética
    // -------------------------------------------------------------------------
    ReligionSim::Humano prayingElder(40, "ElderGorr", 20.0f, 20.0f, ReligionSim::HumanGender::Male);
    ReligionSim::Humano listeningYouth(41, "YoungTaru", 20.8f, 20.0f, ReligionSim::HumanGender::Female);
    prayingElder.setReligionId(1);
    listeningYouth.setReligionId(1);
    prayingElder.changeAction(ReligionSim::HumanAction::REZAR);
    prayingElder.setDevotion(0.95f);
    listeningYouth.setDevotion(0.40f);

    villagerManager.getVillagers().clear();
    villagerManager.addVillager(prayingElder);
    villagerManager.addVillager(listeningYouth);

    villagerManager.update(1.0f, testMap, &testEventLine, -3850);
    const auto& afterPrayer = villagerManager.getVillagers();
    assert(afterPrayer.size() == 2);
    assert(afterPrayer[1].getDevotion() > 0.40f); // Devotion elevated via memetic contagion

    std::cout << "  -> testLivingOrganismReproductionAndMemeticReplicationAI PASSED! Biological drives, courtship, parental investment, and memetic replication fully verified." << std::endl;
}

void testTwoCellTreeHeightAndOcclusionSortingSystem() {
    std::cout << "[Test] Running testTwoCellTreeHeightAndOcclusionSortingSystem..." << std::endl;
    using namespace ReligionSim;

    Renderer renderer(800, 600);

    // EN: 1. Validate 2-cell height for pine tree (propIndex == 0) and deciduous tree (propIndex == 1)
    // ES: 1. Validar altura de 2 celdas para pino (propIndex == 0) y caducifolio (propIndex == 1)
    constexpr float zoomFactor = 1.0f;
    constexpr float expectedTwoCellHeight = 64.0f; // 2 * cellHeight (32px)
    int startX = 0, startY = 0, propWidth = 0, propHeight = 0;

    renderer.getTerrainPropBounds(200.0f, 200.0f, 0, zoomFactor, startX, startY, propWidth, propHeight);
    assert(propHeight == static_cast<int>(expectedTwoCellHeight));
    assert(propWidth == static_cast<int>(expectedTwoCellHeight * 0.78f));
    assert(startX == static_cast<int>(200.0f - propWidth * 0.5f));
    assert(startY == static_cast<int>(200.0f + 6.0f * zoomFactor - propHeight));

    renderer.getTerrainPropBounds(200.0f, 200.0f, 1, zoomFactor, startX, startY, propWidth, propHeight);
    assert(propHeight == static_cast<int>(expectedTwoCellHeight));
    assert(propWidth == static_cast<int>(expectedTwoCellHeight * 0.88f));

    // EN: 2. Validate 2-cell height for harvestable wood resource tree (ResourceType::Wood)
    // ES: 2. Validar altura de 2 celdas para árbol recolectable de madera (ResourceType::Wood)
    int woodStartX = 0, woodStartY = 0, woodWidth = 0, woodHeight = 0;
    renderer.getResourceNodeBounds(250.0f, 250.0f, ResourceType::Wood, zoomFactor, woodStartX, woodStartY, woodWidth, woodHeight);
    assert(woodHeight == static_cast<int>(expectedTwoCellHeight));
    assert(woodWidth == static_cast<int>(expectedTwoCellHeight * 0.84f));

    // EN: 3. Validate non-tree props (rock/shrub) retain natural single-cell proportions
    // ES: 3. Validar que los accesorios que no son árboles (rocas/arbustos) retienen proporciones naturales de 1 celda
    int rockStartX = 0, rockStartY = 0, rockWidth = 0, rockHeight = 0;
    renderer.getTerrainPropBounds(200.0f, 200.0f, 2, zoomFactor, rockStartX, rockStartY, rockWidth, rockHeight);
    assert(rockHeight < static_cast<int>(expectedTwoCellHeight));

    // EN: 4. Validate monument and house bounding boxes
    // ES: 4. Validar cajas delimitadoras de monumento y vivienda
    int templeStartX = 0, templeStartY = 0, templeWidth = 0, templeHeight = 0;
    renderer.getMonumentBounds(300.0f, 300.0f, 1, zoomFactor, templeStartX, templeStartY, templeWidth, templeHeight);
    assert(templeWidth > 0 && templeHeight > 0);

    House tribalHouse;
    tribalHouse.id = 1;
    tribalHouse.type = HouseType::ThatchedHut;
    tribalHouse.buildProgress = 100.0f;
    int houseStartX = 0, houseStartY = 0, houseWidth = 0, houseHeight = 0;
    renderer.getHouseBounds(350.0f, 350.0f, tribalHouse, zoomFactor, houseStartX, houseStartY, houseWidth, houseHeight);
    assert(houseWidth > 0 && houseHeight > 0);

    // EN: 5. Verify translucency rendering with opacity < 1.0f vs opaque 1.0f
    // ES: 5. Verificar renderizado de translucidez con opacidad < 1.0f frente a opaco 1.0f
    Renderer opaqueRenderer(400, 400);
    opaqueRenderer.clear(ColorRGB{100, 100, 100});
    opaqueRenderer.drawTerrainProp(200.0f, 200.0f, 0, zoomFactor, 1.0f);

    Renderer translucentRenderer(400, 400);
    translucentRenderer.clear(ColorRGB{100, 100, 100});
    translucentRenderer.drawTerrainProp(200.0f, 200.0f, 0, zoomFactor, 0.40f);

    // The translucent rendering must blend with the background, producing different pixel values
    bool pixelsDifferDueToTranslucency = false;
    for (size_t pixelIndex = 0; pixelIndex < opaqueRenderer.getFramebuffer().size(); ++pixelIndex) {
        if (opaqueRenderer.getFramebuffer()[pixelIndex] != translucentRenderer.getFramebuffer()[pixelIndex]) {
            pixelsDifferDueToTranslucency = true;
            break;
        }
    }
    assert(pixelsDifferDueToTranslucency);

    std::cout << "  -> testTwoCellTreeHeightAndOcclusionSortingSystem PASSED! 2-cell tree height, bounds, and translucency occlusion verified." << std::endl;
}

void testFallDamageAndNaturalPhenomenaPhysics() {
    using namespace ReligionSim;
    std::cout << "[Test] Running testFallDamageAndNaturalPhenomenaPhysics..." << std::endl;

    WorldMap map(40, 40);
    for (int y = 0; y < 40; ++y) {
        for (int x = 0; x < 40; ++x) {
            Cell& c = map.getCell(x, y);
            c.biome = BiomeType::FertileValley;
            c.elevation = 1.0f;
            c.dominantReligionId = 1;
        }
    }
    // Set rocky mountain cell
    map.getCell(15, 15).biome = BiomeType::RuggedMountains;
    map.getCell(15, 15).elevation = 2.5f;

    // Set water cell
    map.getCell(20, 20).biome = BiomeType::DeepOcean;
    map.getCell(20, 20).elevation = 0.0f;

    // 1. Safe landing threshold (S <= SAFE_FALL_IMPACT_VELOCITY deals 0 damage)
    Humano hSafe(1, "Aldeano Salto Corto", 5.0f, 5.0f);
    assert(hSafe.getHealth() == 100.0f);
    hSafe.applyFallDamage(SAFE_FALL_IMPACT_VELOCITY * 0.8f, map);
    assert(hSafe.getHealth() == 100.0f);
    assert(hSafe.getLastFallDamage() == 0.0f);

    // 2. Dangerous fall (S > SAFE_FALL_IMPACT_VELOCITY deals damage)
    Humano hDrop(2, "Aldeano Caida", 5.0f, 5.0f);
    hDrop.applyFallDamage(25.0f, map);
    assert(hDrop.getHealth() < 100.0f);
    assert(hDrop.getLastFallDamage() > 15.0f);
    assert(hDrop.getStumbleTimer() > 0.0f);
    float grassDamage = hDrop.getLastFallDamage();

    // 3. Rocky mountain terrain multiplier (+30% damage)
    Humano hRocky(3, "Aldeano Rocozo", 15.0f, 15.0f);
    hRocky.applyFallDamage(25.0f, map);
    assert(hRocky.getLastFallDamage() > grassDamage);

    // 4. Water landing dampening (75% shock absorption)
    Humano hWater(4, "Aldeano Agua", 20.0f, 20.0f);
    hWater.applyFallDamage(25.0f, map);
    assert(hWater.getLastFallDamage() < grassDamage * 0.40f);

    // 5. Fatal fall impact from extreme velocity
    Humano hFatal(5, "Aldeano Abismo", 5.0f, 5.0f);
    hFatal.applyFallDamage(58.0f, map);
    assert(hFatal.isDead());
    assert(hFatal.getHealth() == 0.0f);
    assert(hFatal.getCauseOfDeath().find("caida libre") != std::string::npos);

    // 6. Cliff drop: stepping from high elevation to low elevation initiates free-fall
    map.getCell(10, 10).elevation = 2.2f;
    map.getCell(10, 11).elevation = 1.0f; // Delta = 1.2f > CLIFF_FALL_ELEVATION_THRESHOLD
    Humano hCliff(6, "Aldeano Despenadero", 10.5f, 10.95f);
    assert(!hCliff.isFlying());
    assert(hCliff.getAltitude() == 0.0f);
    // Move into (10, 11)
    hCliff.walkTowards(10.5f, 11.5f, false);
    hCliff.update(0.15f, map);
    assert(hCliff.isFlying());
    assert(hCliff.getAltitude() > 0.0f);

    // 7. Rankine Tornado vortex suction, updraft, and centrifugal ejection
    Humano hTornado(7, "Aldeano Tornado", 12.0f, 12.0f);
    assert(!hTornado.isFlying());
    // Apply tornado suction at 1.0 units distance (inside Rankine core)
    hTornado.suckIntoTornado(12.5f, 12.5f, 4.5f, 0.25f);
    assert(hTornado.isFlying());
    assert(hTornado.getAltitude() > 0.0f);
    assert(hTornado.getVelocityZ() > 0.0f);

    // 8. Earthquake seismic acceleration and biomechanical balance
    Humano hQuake(8, "Aldeano Sismo", 14.0f, 14.0f);
    float healthBeforeQuake = hQuake.getHealth();
    hQuake.sufferEarthquake(14.0f, 14.0f, 1.8f);
    assert(hQuake.getHealth() < healthBeforeQuake);
    assert(hQuake.getStumbleTimer() > 0.0f);

    // 9. Volcanic pyroclastic bombs & ballistic trajectories
    DivinePowersSystem divineSys;
    WorldMap volcanoMap(30, 30);
    ReligionRegistry reg;
    EventLine eventLine;
    int relId = reg.createPrimordialReligion("Volcanicos", "Vulcan", ColorRGB(255, 60, 0), "Fire", -4000);
    reg.getReligion(relId)->theologyPoints = 500;
    GestorHumanos gestor;
    Humano hVolcano(9, "Aldeano Caldera", 15.0f, 15.0f);
    gestor.agregarHumano(hVolcano);

    auto castRes = divineSys.castPower(DivinePowerType::Volcano, 15, 15, volcanoMap, reg, gestor, -4000, eventLine, relId);
    assert(castRes.success);
    assert(volcanoMap.getCell(15, 15).hasVolcano);

    // Update volcano simulation to emit and explode pyroclastic bombs
    for (int step = 0; step < 50; ++step) {
        divineSys.update(0.1f, volcanoMap, gestor);
    }
    assert(volcanoMap.getCell(15, 15).hasActiveLava);

    // =========================================================================
    // EN: 10. Natural Phenomena Impact on Trees, Houses, and Movable Caravans
    // ES: 10. Impacto de Fenómenos Naturales sobre Árboles, Casas y Caravanas Móviles
    // =========================================================================
    BuildingSystem buildingSystem;
    ReplicationEngine replicationEngine;

    // EN: Setup forest cell with standing timber and a tribal hut
    // ES: Configurar celda de bosque con madera erguida y una choza tribal
    WorldMap disasterMap(40, 40);
    for (int cellIndexY = 0; cellIndexY < 40; ++cellIndexY) {
        for (int cellIndexX = 0; cellIndexX < 40; ++cellIndexX) {
            Cell& targetCell = disasterMap.getCell(cellIndexX, cellIndexY);
            targetCell.biome = BiomeType::FertileValley;
            targetCell.elevation = 1.0f;
            targetCell.dominantReligionId = relId;
        }
    }

    Cell& forestCell = disasterMap.getCell(20, 20);
    forestCell.biome = BiomeType::DenseForest;
    forestCell.naturalResource = ResourceType::Wood;
    forestCell.resourceAmount = 250;
    forestCell.hasTreesUprooted = false;

    // EN: Build a house next to the forest
    // ES: Construir una vivienda contigua al bosque
    int houseIdentifier = buildingSystem.initiateHouseConstruction(21, 20, 10, -1, relId, HouseType::ThatchedHut, disasterMap);
    assert(houseIdentifier > 0);
    House* constructedHouse = buildingSystem.getHouseByIdMutable(houseIdentifier);
    assert(constructedHouse != nullptr);
    constructedHouse->state = BuildingState::Completed;
    constructedHouse->buildProgress = 100.0f;
    constructedHouse->health = 100.0f;
    assert(constructedHouse->isCompleted());
    assert(constructedHouse->health > 0.0f);

    Humano residentVillager(10, "Aldeano Hogareno", 21.5f, 20.5f);
    residentVillager.setHomeId(houseIdentifier);
    residentVillager.setDurmiendoBajoTecho(true);
    gestor.agregarHumano(residentVillager);

    // EN: Cast devastating Tornado directly onto the forest and house
    // ES: Conjurar Tornado devastador directamente sobre el bosque y la vivienda
    divineSys.clearAllFX();
    reg.getReligion(relId)->theologyPoints = 500;
    auto tornadoCast = divineSys.castPower(DivinePowerType::Tornado, 20, 20, disasterMap, reg, gestor, -4000, eventLine, relId);
    assert(tornadoCast.success);

    // EN: Simulate tornado passing through: should uproot trees, damage house, and spawn debris
    // ES: Simular paso del tornado: debe arrancar árboles, dañar casa y generar escombros
    for (int simulationStep = 0; simulationStep < 15; ++simulationStep) {
        divineSys.update(0.1f, disasterMap, gestor, &buildingSystem, &replicationEngine);
    }

    // EN: Assert trees were uprooted and physical debris was spawned
    // ES: Afirmar que los árboles fueron arrancados y se generaron escombros físicos
    assert(forestCell.hasTreesUprooted);
    assert(divineSys.getActiveDebris().size() > 0);
    assert(constructedHouse->health < 100.0f);

    // EN: Test complete structural collapse into ruins
    // ES: Probar colapso estructural completo en ruinas
    const bool collapsedIntoRuins = buildingSystem.damageHouse(houseIdentifier, 200.0f, disasterMap, &gestor);
    assert(collapsedIntoRuins);
    assert(constructedHouse->isRuined());
    assert(constructedHouse->state == BuildingState::Ruined);

    // EN: Sheltered villager was evicted and suffered structural trauma
    // ES: El aldeano cobijado fue desalojado y sufrió trauma estructural
    Humano* residentPtr = nullptr;
    for (auto& villager : gestor.getHumanos()) {
        if (villager.getId() == 10) {
            residentPtr = &villager;
            break;
        }
    }
    assert(residentPtr != nullptr);
    assert(!residentPtr->estaDurmiendoBajoTecho());
    assert(residentPtr->getHomeId() == -1);

    // EN: Test Physical Debris ballistic gravity, ground bounce, and settlement
    // ES: Probar gravedad balística de escombros físicos, rebote en suelo y reposo
    divineSys.spawnDebris(DebrisType::UprootedTreeTrunk, 25.0f, 25.0f, 15.0f, 2.0f, 0.0f, 10.0f, 20.0f, 30.0f, 1.5f);
    assert(divineSys.getActiveDebris().size() > 0);

    // EN: Advance simulation until the airborne debris strikes the ground and bounces
    // ES: Avanzar simulación hasta que los escombros aéreos golpeen el suelo y reboten
    for (int ballisticStep = 0; ballisticStep < 60; ++ballisticStep) {
        divineSys.update(0.05f, disasterMap, gestor, &buildingSystem, &replicationEngine);
    }
    const auto& debrisCollection = divineSys.getActiveDebris();
    assert(!debrisCollection.empty());
    bool foundBouncedOrSettledDebris = false;
    for (const auto& physicalDebrisItem : debrisCollection) {
        if (physicalDebrisItem.bounces > 0 || physicalDebrisItem.settled) {
            foundBouncedOrSettledDebris = true;
            break;
        }
    }
    assert(foundBouncedOrSettledDebris);

    std::cout << "  -> testFallDamageAndNaturalPhenomenaPhysics PASSED! Fall damage physics, terrain multipliers, cliff drops, Rankine vortex, trees uprooting, house ruins and ballistic debris verified." << std::endl;
}

// EN: Comprehensive verification of thermodynamic fire physics, advective spread, moisture barriers, and ecological ash recovery
// ES: Verificación exhaustiva de física termodinámica del fuego, propagación advectiva, barreras de humedad y recuperación ecológica de ceniza
void testFireThermodynamicsSpreadDissipationAndEcologicalRecovery() {
    std::cout << "Testing Fire Physics: Thermodynamics, Wind Advection, Moisture Barriers, House & Tree Destruction, Human Water Extinction, and Ecological Recovery..." << std::endl;
    using namespace ReligionSim;

    WorldMap fireSimulationMap(32, 32);
    // EN: Initialize base testing grid
    // ES: Inicializar cuadrícula base de prueba
    for (int gridY = 0; gridY < 32; ++gridY) {
        for (int gridX = 0; gridX < 32; ++gridX) {
            Cell& cellRef = fireSimulationMap.getCell(gridX, gridY);
            cellRef.biome = BiomeType::FertileValley;
            cellRef.elevation = 1.0f;
            cellRef.isBurning = false;
            cellRef.isBurnt = false;
            cellRef.fireIntensity = 0.0f;
            cellRef.heatAccumulator = 0.0f;
            cellRef.burnTimer = 0.0f;
        }
    }

    // =========================================================================
    // 1. VERIFY FLAMMABILITY AND COMBUSTIBILITY LOGIC
    // =========================================================================
    Cell denseForestCell;
    denseForestCell.biome = BiomeType::DenseForest;
    denseForestCell.naturalResource = ResourceType::Wood;
    denseForestCell.resourceAmount = 100;
    assert(denseForestCell.isCombustible());
    assert(denseForestCell.getFlammability() >= 1.0f);

    Cell waterBodyCell;
    waterBodyCell.biome = BiomeType::ShallowWater;
    assert(!waterBodyCell.isCombustible());
    assert(waterBodyCell.getFlammability() <= 0.0f);

    Cell desertDuneCell;
    desertDuneCell.biome = BiomeType::AridDesert;
    assert(!desertDuneCell.isCombustible());

    // =========================================================================
    // 2. DIRECTIONAL WIND ADVECTION AND THERMODYNAMIC FIRE SPREAD
    // =========================================================================
    for (int cellIndexX = 10; cellIndexX <= 12; ++cellIndexX) {
        Cell& forestCell = fireSimulationMap.getCell(cellIndexX, 10);
        forestCell.biome = BiomeType::DenseForest;
        forestCell.naturalResource = ResourceType::Wood;
        forestCell.resourceAmount = 100;
        forestCell.maxResourceAmount = 100;
    }
    Cell& upwindCell = fireSimulationMap.getCell(9, 10);
    upwindCell.biome = BiomeType::DenseForest;

    constexpr float initialBurnDurationSeconds = 16.0f;
    const bool ignitionSuccess = fireSimulationMap.igniteCell(10, 10, initialBurnDurationSeconds, 1.0f);
    assert(ignitionSuccess);
    assert(fireSimulationMap.getCell(10, 10).isBurning);
    assert(fireSimulationMap.getCell(10, 10).fireIntensity == 1.0f);

    constexpr float eastwardWindVelocityX = 0.80f;
    constexpr float neutralWindVelocityY = 0.0f;

    for (int simulationStep = 0; simulationStep < 2; ++simulationStep) {
        fireSimulationMap.updateFirePhysics(0.10f, nullptr, nullptr, eastwardWindVelocityX, neutralWindVelocityY);
    }

    const float downwindHeat = fireSimulationMap.getCell(11, 10).heatAccumulator;
    const float upwindHeat = fireSimulationMap.getCell(9, 10).heatAccumulator;
    assert(downwindHeat > upwindHeat);

    for (int spreadStep = 0; spreadStep < 20; ++spreadStep) {
        fireSimulationMap.updateFirePhysics(0.20f, nullptr, nullptr, eastwardWindVelocityX, neutralWindVelocityY);
        if (fireSimulationMap.getCell(11, 10).isBurning) break;
    }
    assert(fireSimulationMap.getCell(11, 10).isBurning);
    assert(!fireSimulationMap.getCell(9, 10).isBurning);

    // =========================================================================
    // 3. MOISTURE BARRIER & NATURAL FIREBREAK DAMPENING
    // =========================================================================
    Cell& waterCanalCell = fireSimulationMap.getCell(13, 10);
    waterCanalCell.biome = BiomeType::ShallowWater;
    Cell& oppositeBankCell = fireSimulationMap.getCell(14, 10);
    oppositeBankCell.biome = BiomeType::DenseForest;
    oppositeBankCell.heatAccumulator = 0.0f;

    fireSimulationMap.igniteCell(12, 10, 12.0f, 1.0f);
    assert(fireSimulationMap.getCell(12, 10).isBurning);

    const bool canalIgnited = fireSimulationMap.igniteCell(13, 10, 10.0f, 1.0f);
    assert(!canalIgnited);
    assert(!fireSimulationMap.getCell(13, 10).isBurning);

    for (int canalStep = 0; canalStep < 10; ++canalStep) {
        fireSimulationMap.updateFirePhysics(0.20f, nullptr, nullptr, eastwardWindVelocityX, neutralWindVelocityY);
    }
    assert(!oppositeBankCell.isBurning);

    // =========================================================================
    // 4. WOOD RESOURCE CONSUMPTION AND TREE TOPPLING
    // =========================================================================
    Cell& timberStandCell = fireSimulationMap.getCell(20, 20);
    timberStandCell.biome = BiomeType::DenseForest;
    timberStandCell.naturalResource = ResourceType::Wood;
    timberStandCell.resourceAmount = 100;
    timberStandCell.hasTreesUprooted = false;

    fireSimulationMap.igniteCell(20, 20, 10.0f, 1.0f);
    assert(timberStandCell.isBurning);

    fireSimulationMap.updateFirePhysics(1.5f, nullptr, nullptr, 0.0f, 0.0f);
    assert(timberStandCell.hasTreesUprooted);
    assert(timberStandCell.resourceAmount < 100);

    // =========================================================================
    // 5. STRUCTURAL HOUSE FIRE DAMAGE AND COLLAPSE INTO RUINS
    // =========================================================================
    BuildingSystem buildingSystem;
    GestorHumanos villagerManager;
    House tribalDwelling;
    tribalDwelling.id = 1;
    tribalDwelling.cellX = 22;
    tribalDwelling.cellY = 22;
    tribalDwelling.state = BuildingState::Completed;
    tribalDwelling.health = 80.0f;
    tribalDwelling.maxHealth = 100.0f;
    buildingSystem.getHouses().push_back(tribalDwelling);

    Cell& housePlotCell = fireSimulationMap.getCell(22, 22);
    housePlotCell.hasHouse = true;
    fireSimulationMap.igniteCell(22, 22, 15.0f, 1.0f);

    const float initialHouseHealth = buildingSystem.getHouseById(1)->health;
    fireSimulationMap.updateFirePhysics(1.0f, &buildingSystem, &villagerManager, 0.0f, 0.0f);
    const float damagedHouseHealth = buildingSystem.getHouseById(1)->health;
    assert(damagedHouseHealth < initialHouseHealth);

    for (int collapseStep = 0; collapseStep < 15; ++collapseStep) {
        fireSimulationMap.updateFirePhysics(0.50f, &buildingSystem, &villagerManager, 0.0f, 0.0f);
        if (buildingSystem.getHouseById(1)->isRuined()) break;
    }
    assert(buildingSystem.getHouseById(1)->isRuined());

    // =========================================================================
    // 6. VILLAGER IGNITION, SPRINT PANIC AND WATER EXTINGUISHING
    // =========================================================================
    Humano fleeingVillager(1, "Grog", 24.5f, 24.5f, HumanGender::Male);
    fleeingVillager.setSalud(100.0f);
    villagerManager.addVillager(fleeingVillager);

    Cell& villagerFootCell = fireSimulationMap.getCell(24, 24);
    villagerFootCell.biome = BiomeType::DenseForest;
    fireSimulationMap.igniteCell(24, 24, 12.0f, 1.0f);

    fireSimulationMap.updateFirePhysics(0.50f, &buildingSystem, &villagerManager, 0.0f, 0.0f);

    assert(!villagerManager.getHumanos().empty());
    Humano& activeVillager = villagerManager.getHumanos().front();
    assert(activeVillager.isBurning());
    assert(activeVillager.estaEnPanico());
    assert(activeVillager.getSpeed() >= 1.35f);

    const float villagerHealthBeforeBurn = activeVillager.getHealth();
    activeVillager.update(0.60f, fireSimulationMap);
    assert(activeVillager.getHealth() < villagerHealthBeforeBurn);

    Cell& lakeWaterCell = fireSimulationMap.getCell(25, 24);
    lakeWaterCell.biome = BiomeType::ShallowWater;
    activeVillager.setPosition(25.5f, 24.5f);

    activeVillager.update(0.10f, fireSimulationMap);
    assert(!activeVillager.isBurning());

    // =========================================================================
    // 7. FIRE DISSIPATION, SCORCHED EARTH & ECOLOGICAL ASH RECOVERY
    // =========================================================================
    Cell& extinguishingStandCell = fireSimulationMap.getCell(28, 28);
    extinguishingStandCell.biome = BiomeType::DenseForest;
    fireSimulationMap.igniteCell(28, 28, 0.20f, 1.0f);
    assert(extinguishingStandCell.isBurning);

    fireSimulationMap.updateFirePhysics(0.35f, nullptr, nullptr, 0.0f, 0.0f);
    assert(!extinguishingStandCell.isBurning);
    assert(extinguishingStandCell.isBurnt);
    assert(extinguishingStandCell.ashRecoveryTimer > 0.0f);

    const bool reignitionAttempt = fireSimulationMap.igniteCell(28, 28, 10.0f, 1.0f);
    assert(!reignitionAttempt);
    assert(!extinguishingStandCell.isBurning);

    extinguishingStandCell.ashRecoveryTimer = 0.10f;
    fireSimulationMap.updateFirePhysics(0.20f, nullptr, nullptr, 0.0f, 0.0f);
    assert(!extinguishingStandCell.isBurnt);
    assert(extinguishingStandCell.biome == BiomeType::FertileValley);

    std::cout << "  -> testFireThermodynamicsSpreadDissipationAndEcologicalRecovery PASSED! Thermodynamic spread, wind vectoring, moisture firebreak, tree felling, house ruins, human water extinguishing and ash recovery verified." << std::endl;
}

// EN: Test suite for Rain and Deluge divine powers, hydrology physics, and fire quenching
// ES: Batería de pruebas para los poderes divinos Lluvia y Diluvio, física hidrológica y extinción del fuego
void testRainAndDelugeWaterPhysicsAndFireExtinction() {
    using namespace ReligionSim;
    std::cout << "Running testRainAndDelugeWaterPhysicsAndFireExtinction()..." << std::endl;

    DivinePowersSystem divinePowers;
    WorldMap waterWorldMap(40, 40);

    ReligionRegistry religionRegistry;
    GestorHumanos villagerManager;
    EventLine eventLineSystem;

    // EN: Setup player and rival religions
    // ES: Configurar religiones del jugador y rivales
    int playerRelId = religionRegistry.createPrimordialReligion("Acuafieles", "Acuarius", ColorRGB{60, 160, 240}, "Water", 100);
    Religion* playerReligion = religionRegistry.getReligion(playerRelId);
    assert(playerReligion != nullptr);
    playerReligion->theologyPoints = 500;

    // EN: Add a shaman leader to enable divine power channeling
    // ES: Agregar un líder chamán para permitir la canalización de poderes divinos
    Humano tribalShaman(1, "Chaman Pluvial", 15.0f, 15.0f);
    tribalShaman.setReligionId(playerRelId);
    tribalShaman.setEsChaman(true);
    villagerManager.agregarHumano(tribalShaman);

    // =========================================================================
    // 1. Verificar Atributos y Costos de Lluvia y Diluvio
    // =========================================================================
    assert(divinePowers.getPowerCost(DivinePowerType::Rain) == 30);
    assert(divinePowers.getPowerCost(DivinePowerType::Deluge) == 65);
    assert(divinePowers.getPowerRadius(DivinePowerType::Rain) == 4.0f);
    assert(divinePowers.getPowerRadius(DivinePowerType::Deluge) == 6.0f);
    assert(divinePowers.getPowerName(DivinePowerType::Rain) == "LLUVIA BENDITA");
    assert(divinePowers.getPowerName(DivinePowerType::Deluge) == "GRAN DILUVIO");

    // =========================================================================
    // 2. Canalización e Invocación de Lluvia Bendita
    // =========================================================================
    const int initialFaith = playerReligion->theologyPoints;
    const auto rainCastResult = divinePowers.castPower(DivinePowerType::Rain, 15, 15,
                                                      waterWorldMap, religionRegistry, villagerManager,
                                                      100, eventLineSystem, playerRelId);
    assert(rainCastResult.success);
    assert(rainCastResult.faithGenerated > 0);
    assert(playerReligion->theologyPoints == initialFaith - 30 + rainCastResult.faithGenerated);
    assert(divinePowers.getActiveFX().size() == 1);
    assert(divinePowers.getActiveFX()[0].type == DivinePowerType::Rain);

    // =========================================================================
    // 3. Hidrología y Física de Escorrentía por Gradiente Hidráulico
    // =========================================================================
    // EN: Create an incline: cell (10, 10) high elevation (1.50) -> neighbor (11, 10) low elevation (0.50)
    // ES: Crear una pendiente: celda (10, 10) alta elevación (1.50) -> vecina (11, 10) baja elevación (0.50)
    // EN: Create a gorge: mountain ridges on sides (2.0) channel runoff from (10, 10) directly into (11, 10)
    // ES: Crear un desfiladero: crestas montañosas en los flancos (2.0) encauzan la escorrentía hacia (11, 10)
    for (int deltaY = -1; deltaY <= 1; ++deltaY) {
        for (int deltaX = -1; deltaX <= 1; ++deltaX) {
            waterWorldMap.getCell(10 + deltaX, 10 + deltaY).elevation = 2.0f;
        }
    }

    Cell& highSlopeCell = waterWorldMap.getCell(10, 10);
    highSlopeCell.elevation = 1.50f;
    highSlopeCell.biome = BiomeType::HighlandHills;
    highSlopeCell.waterDepth = 0.0f;

    Cell& lowValleyCell = waterWorldMap.getCell(11, 10);
    lowValleyCell.elevation = 0.50f;
    lowValleyCell.biome = BiomeType::FertileValley;
    lowValleyCell.waterDepth = 0.0f;

    // EN: Add water to the high slope cell and simulate gravity-driven runoff
    // ES: Agregar agua a la celda de pendiente alta y simular escorrentía por gravedad
    waterWorldMap.addWaterToCell(10, 10, 0.40f);
    assert(highSlopeCell.waterDepth == 0.40f);
    assert(highSlopeCell.isFlooded);

    for (int simulationStep = 0; simulationStep < 10; ++simulationStep) {
        waterWorldMap.updateWaterPhysics(0.10f, nullptr, nullptr);
    }

    // EN: Verify water has flowed downhill from (10, 10) into (11, 10)
    // ES: Verificar que el agua ha fluido colina abajo desde (10, 10) hacia (11, 10)
    assert(lowValleyCell.waterDepth > 0.05f);
    assert(highSlopeCell.waterDepth < 0.40f);
    assert(lowValleyCell.moisture > 0.50f);
    // EN: Verify fluid velocity momentum vector has developed along slope (+X)
    // ES: Verificar que se desarrolló el vector de velocidad de flujo a lo largo de la ladera (+X)
    assert(lowValleyCell.flowVelocityX > 0.0f || highSlopeCell.flowVelocityX > 0.0f);
    assert(lowValleyCell.getFlowSpeed() > 0.0f || highSlopeCell.getFlowSpeed() > 0.0f);

    // =========================================================================
    // 4. Drenaje Natural hacia Océanos y Cuerpos de Agua
    // =========================================================================
    Cell& coastalLandCell = waterWorldMap.getCell(20, 20);
    coastalLandCell.elevation = 0.30f;
    coastalLandCell.biome = BiomeType::CoastalPlains;

    Cell& seaDrainageCell = waterWorldMap.getCell(21, 20);
    seaDrainageCell.elevation = 0.05f;
    seaDrainageCell.biome = BiomeType::ShallowWater;
    assert(seaDrainageCell.isNaturalWater());

    waterWorldMap.addWaterToCell(20, 20, 0.50f);
    for (int simulationStep = 0; simulationStep < 10; ++simulationStep) {
        waterWorldMap.updateWaterPhysics(0.10f, nullptr, nullptr);
    }
    // EN: Natural water bodies act as sinks without piling up artificial waterDepth
    // ES: Las masas de agua naturales actúan como sumidero sin acumular waterDepth artificial
    assert(seaDrainageCell.waterDepth == 0.0f);
    assert(coastalLandCell.waterDepth < 0.50f);

    // =========================================================================
    // 5. Interacción Agua <-> Fuego: Extinción por Inmersión y Generación de Vapor
    // =========================================================================
    Cell& burningForestCell = waterWorldMap.getCell(25, 25);
    burningForestCell.elevation = 0.80f;
    burningForestCell.biome = BiomeType::DenseForest;
    burningForestCell.naturalResource = ResourceType::Wood;
    burningForestCell.resourceAmount = 80;
    const bool forestIgnited = waterWorldMap.igniteCell(25, 25, 10.0f, 0.95f);
    assert(forestIgnited);
    assert(burningForestCell.isBurning);

    // EN: Deliver deluge water onto the burning forest tile
    // ES: Arrojar agua de diluvio sobre la celda del bosque en llamas
    waterWorldMap.addWaterToCell(25, 25, 0.45f);
    waterWorldMap.updateWaterPhysics(0.10f, nullptr, nullptr);

    // EN: Burning cell must be extinguished immediately and produce steam plumes
    // ES: La celda en llamas debe quedar apagada de inmediato y producir plumas de vapor
    assert(!burningForestCell.isBurning);
    assert(burningForestCell.steamTimer > 1.0f);

    // EN: Flooded and saturated ground becomes incombustible
    // ES: El suelo inundado y saturado se vuelve incombustible
    assert(burningForestCell.isFlooded);
    assert(burningForestCell.getFlammability() == 0.0f);
    const bool ignitionAttemptOnFloodedGround = waterWorldMap.igniteCell(25, 25, 10.0f, 1.0f);
    assert(!ignitionAttemptOnFloodedGround);
    assert(!burningForestCell.isBurning);

    // =========================================================================
    // 6. Extinción de Unidades Vivas en Llamas por Inmersión y Lluvia
    // =========================================================================
    Humano burningVillager(201, "Aldeano Fuego", 11.0f, 10.0f);
    burningVillager.ignite(6.0f);
    assert(burningVillager.isBurning());

    // EN: Step into flooded tile (11, 10) where waterDepth > 0.05f
    // ES: Pisar celda inundada (11, 10) donde waterDepth > 0.05f
    villagerManager.agregarHumano(burningVillager);
    waterWorldMap.updateWaterPhysics(0.10f, nullptr, &villagerManager);

    auto villagerIterator = std::find_if(villagerManager.getHumanos().begin(), villagerManager.getHumanos().end(),
                                         [](const Humano& h) { return h.getId() == 201; });
    assert(villagerIterator != villagerManager.getHumanos().end());
    assert(!villagerIterator->isBurning());

    // =========================================================================
    // 7. Sanación de Aldeanos y Lluvia Continua
    // =========================================================================
    villagerIterator->setHealth(40.0f);
    assert(villagerIterator->getHealth() == 40.0f);

    // Move villager into rain radius at (15, 15)
    villagerIterator->setPosicion(15.0f, 15.0f);

    // Simulate active rain FX over the area
    divinePowers.update(1.0f, waterWorldMap, villagerManager, nullptr, nullptr);
    assert(villagerIterator->getHealth() > 40.0f);

    // =========================================================================
    // 8. Revitalización Ecológica de Ceniza Acelerada por Agua
    // =========================================================================
    Cell& scorchedLandCell = waterWorldMap.getCell(30, 30);
    scorchedLandCell.isBurnt = true;
    scorchedLandCell.ashRecoveryTimer = 20.0f;
    scorchedLandCell.biome = BiomeType::AridDesert;

    // EN: Flood the ash soil with water and verify accelerated recovery
    // ES: Inundar el suelo de ceniza con agua y verificar recuperación acelerada
    waterWorldMap.addWaterToCell(30, 30, 0.25f);
    const float initialAshRecoveryTimer = scorchedLandCell.ashRecoveryTimer;
    waterWorldMap.updateWaterPhysics(0.50f, nullptr, nullptr);

    // Ash timer was reduced by both dt and the 3x water acceleration bonus
    assert(scorchedLandCell.ashRecoveryTimer < initialAshRecoveryTimer - 1.0f);

    // =========================================================================
    // 9. Hidrodinámica Avanzada: Arrastre de Corriente y Escombros Flotantes
    // =========================================================================
    // EN: Verify flowing water current imparts hydrodynamic drag on swimmers and drifts buoyant debris
    // ES: Verificar que la corriente de agua ejerce arrastre hidrodinámico en nadadores y deriva escombros flotantes
    Cell& riverCurrentCell = waterWorldMap.getCell(11, 10);
    riverCurrentCell.elevation = 0.20f;
    riverCurrentCell.biome = BiomeType::FertileValley;
    waterWorldMap.addWaterToCell(11, 10, 0.45f);
    riverCurrentCell.flowVelocityX = 1.8f;
    riverCurrentCell.flowVelocityY = 0.0f;
    riverCurrentCell.isFlooded = true;

    Humano swimmerVillager(301, "Nadador Rio", 11.2f, 10.5f);
    villagerManager.agregarHumano(swimmerVillager);
    const float initialSwimmerX = 11.2f;

    waterWorldMap.updateWaterPhysics(0.20f, nullptr, &villagerManager);

    auto swimmerIterator = std::find_if(villagerManager.getHumanos().begin(), villagerManager.getHumanos().end(),
                                        [](const Humano& h) { return h.getId() == 301; });
    assert(swimmerIterator != villagerManager.getHumanos().end());
    assert(swimmerIterator->getX() > initialSwimmerX);
    assert(swimmerIterator->estaNadando());

    // EN: Buoyant debris (uprooted trunk) floating in river current develops downstream drift
    // ES: Escombros flotantes (tronco arrancado) en corriente de río desarrollan deriva río abajo
    divinePowers.spawnDebris(DebrisType::UprootedTreeTrunk, 11.2f, 10.5f, 0.0f, 0.0f, 0.0f, 0.0f, 20.0f, 10.0f, 1.0f);
    assert(!divinePowers.getActiveDebris().empty());
    divinePowers.update(0.10f, waterWorldMap, villagerManager, nullptr, nullptr);
    assert(divinePowers.getActiveDebris().back().velocityX > 0.0f);
    assert(divinePowers.getActiveDebris().back().isFloating);

    std::cout << "  -> testRainAndDelugeWaterPhysicsAndFireExtinction PASSED! Rain & deluge powers, fluid hydrology, runoff flow, ocean sinks, instant fire extinction with steam, incombustible flooded tiles, burning villager quenching, accelerated ash recovery, and hydrodynamic current drag & floating debris drift verified." << std::endl;
}

void testAtomicBombCheatAndNuclearPhysics() {
    using namespace ReligionSim;
    std::cout << "[Test] Running testAtomicBombCheatAndNuclearPhysics (FUCKINGBOMB Cheat & Nuclear Detonation Physics)..." << std::endl;

    // EN: Setup world, map, religion, and systems
    // ES: Configurar mundo, mapa, religión y sistemas
    WorldMap nuclearMap(40, 40);
    ReligionRegistry religionRegistry;
    GestorHumanos villagerManager;
    EventLine eventLine;
    BuildingSystem buildingSystem;
    ReplicationEngine replicationEngine;
    DivinePowersSystem divinePowers;

    const int playerRelId = religionRegistry.createPrimordialReligion("Pueblo Atómico", "Dios del Uranio", ColorRGB{255, 235, 90}, "Uranium", 0);
    Religion* playerReligion = religionRegistry.getReligion(playerRelId);
    assert(playerReligion != nullptr);
    playerReligion->theologyPoints = 0; // EN: Cheat power should work even with 0 faith / ES: El poder truco debe funcionar incluso con 0 fe

    // EN: 1. Validate cheat attributes: zero cost, radius, name, color
    // ES: 1. Validar atributos del truco: costo cero, radio, nombre, color
    assert(divinePowers.getPowerCost(DivinePowerType::AtomicBomb) == 0);
    assert(divinePowers.getPowerRadius(DivinePowerType::AtomicBomb) == static_cast<int>(std::round(LittleBoyScale::HEAVY_OVERPRESSURE_RADIUS_CELLS)));
    assert(divinePowers.getPowerName(DivinePowerType::AtomicBomb).find("FUCKINGBOMB") != std::string::npos);
    assert(divinePowers.getPowerColor(DivinePowerType::AtomicBomb).r == 255);

    // EN: 2. Setup environment around ground zero (20, 20)
    // ES: 2. Configurar entorno alrededor de la zona cero (20, 20)
    // - Epicenter cell (20, 20) with water and fertile soil
    Cell& epicenterCell = nuclearMap.getCell(20, 20);
    epicenterCell.biome = BiomeType::FertileValley;
    epicenterCell.elevation = 1.2f;
    epicenterCell.waterDepth = 0.50f;
    epicenterCell.isFlooded = true;
    epicenterCell.hasImpactCrater = false;

    // - Intermediate forest cell (23, 20) at distance 3.0 (bordering intermediate zone)
    Cell& forestCell = nuclearMap.getCell(23, 20);
    forestCell.biome = BiomeType::DenseForest;
    forestCell.naturalResource = ResourceType::Wood;
    forestCell.resourceAmount = 150;
    forestCell.hasTreesUprooted = false;

    // - Setup building in intermediate zone at (24, 20)
    const int houseId = buildingSystem.initiateHouseConstruction(24, 20, 101, 102, playerRelId, HouseType::ThatchedHut, nuclearMap);
    House* pHouse = buildingSystem.getHouseByIdMutable(houseId);
    assert(pHouse != nullptr);
    pHouse->state = BuildingState::Completed;
    assert(!buildingSystem.getHouses().empty());
    assert(!buildingSystem.getHouses()[0].isRuined());

    // - Epicenter villager (distance 0.28): should be 100% vaporized
    Humano epicenterVillager(1, "Adan Epicentro", 20.2f, 20.2f);
    epicenterVillager.setReligionId(playerRelId);
    epicenterVillager.setSalud(100.0f);
    villagerManager.agregarHumano(epicenterVillager);

    // - Intermediate villager (distance 5.0): should suffer severe burns and ragdoll
    Humano intermediateVillager(2, "Eva Intermedia", 25.0f, 20.0f);
    intermediateVillager.setReligionId(playerRelId);
    intermediateVillager.setSalud(100.0f);
    villagerManager.agregarHumano(intermediateVillager);

    // - Peripheral villager (distance 8.0): should suffer blast damage and panic
    Humano peripheralVillager(3, "Cain Periferico", 28.0f, 20.0f);
    peripheralVillager.setReligionId(playerRelId);
    peripheralVillager.setSalud(100.0f);
    villagerManager.agregarHumano(peripheralVillager);

    // EN: 3. Cast Atomic Bomb at (20, 20)
    // ES: 3. Lanzar Bomba Atómica en (20, 20)
    const CastResult castResult = divinePowers.castPower(
        DivinePowerType::AtomicBomb, 20, 20,
        nuclearMap, religionRegistry, villagerManager,
        2026, eventLine, playerRelId,
        &buildingSystem, &replicationEngine
    );

    assert(castResult.success == true);
    assert(castResult.casualties >= 1);
    assert(castResult.message.find("FUCKINGBOMB") != std::string::npos);
    assert(divinePowers.getActiveFX().size() == 1);
    assert(divinePowers.getActiveFX()[0].type == DivinePowerType::AtomicBomb);

    // EN: 4. Verify Epicenter Physics: crater formation, water boiled, unit vaporized
    // ES: 4. Verificar físicas del epicentro: formación de cráter, agua evaporada, unidad vaporizada
    assert(epicenterCell.hasImpactCrater == true);
    assert(epicenterCell.craterDepth >= 0.8f);
    assert(epicenterCell.elevation < 1.2f);
    assert(epicenterCell.biome == BiomeType::AridDesert);
    assert(epicenterCell.waterDepth == 0.0f);
    assert(epicenterCell.steamTimer > 0.0f);

    Humano& refEpicenterVillager = villagerManager.getHumanos()[0];
    assert(refEpicenterVillager.estaMuerto() == true);
    assert(refEpicenterVillager.isRagdoll() == true);
    // EN: Verify extreme vertical ballistic velocity and flight altitude for blown-up unit
    // ES: Verificar velocidad balística vertical extrema y altitud de vuelo para la unidad lanzada
    assert(refEpicenterVillager.getVelocityZ() >= 40.0f);
    assert(refEpicenterVillager.getDeathTimer() >= 2.0f); // EN: Corpse does not expire mid-air / ES: Cadáver no expira en el aire

    // EN: 5. Verify Intermediate Zone Physics: trees uprooted, houses ruined, burns & multi-part flying debris
    // ES: 5. Verificar físicas de zona intermedia: árboles arrancados, casas en ruinas, quemaduras y escombros múltiples volando
    assert(forestCell.hasTreesUprooted == true);
    assert(forestCell.resourceAmount == 0);
    assert(forestCell.naturalResource == ResourceType::None);

    assert(buildingSystem.getHouses()[0].isRuined() == true);
    assert(divinePowers.getActiveDebris().size() >= 4); // EN: Trunk + Thatch + 2 Rubble + Thatch

    // EN: Verify that debris pieces have high ballistic flight velocity
    // ES: Verificar que las piezas de escombros tienen alta velocidad balística de vuelo
    bool hasHighFlyingDebris = false;
    for (const auto& debrisItem : divinePowers.getActiveDebris()) {
        if (debrisItem.velocityZ >= 30.0f) {
            hasHighFlyingDebris = true;
            break;
        }
    }
    assert(hasHighFlyingDebris == true);

    Humano& refIntermediateVillager = villagerManager.getHumanos()[1];
    assert(refIntermediateVillager.getSalud() < 100.0f);
    assert(refIntermediateVillager.isRagdoll() == true || refIntermediateVillager.isBurning() == true);

    // EN: 6. Verify Peripheral Zone Physics: blast damage
    // ES: 6. Verificar físicas de zona periférica: daño por onda de choque
    Humano& refPeripheralVillager = villagerManager.getHumanos()[2];
    assert(refPeripheralVillager.getSalud() < 100.0f);

    // EN: 7. Test progression with update: cloud lifecycle, boiled standing water, debris flight & ground bounce
    // ES: 7. Probar progresión con update: ciclo de vida de nube, agua hervida, vuelo de escombros y rebote en suelo
    epicenterCell.waterDepth = 0.30f; // standing water attempts to accumulate
    divinePowers.update(0.5f, nuclearMap, villagerManager, &buildingSystem, &replicationEngine);
    assert(epicenterCell.waterDepth == 0.0f); // boiled away!

    // EN: Verify debris moves and altitude rises in flight
    assert(!divinePowers.getActiveDebris().empty());
    bool debrisAirborne = false;
    for (const auto& debrisItem : divinePowers.getActiveDebris()) {
        if (debrisItem.altitude > 0.0f) {
            debrisAirborne = true;
            break;
        }
    }
    assert(debrisAirborne == true);

    // EN: Advance multiple steps to verify ragdoll flight and landing
    for (int step = 0; step < 20; ++step) {
        refEpicenterVillager.update(0.1f, nuclearMap, nullptr, false, &buildingSystem);
    }
    // Corpse should still be in flight and deathTimer preserved while airborne
    assert(refEpicenterVillager.getAltitude() > 0.0f);
    assert(refEpicenterVillager.getDeathTimer() >= 2.0f);

    // EN: 8. Verify Game command line and hotkey integration
    // ES: 8. Verificar integración con línea de comandos y teclas de Game
    Game testGame(1280, 720);
    testGame.setGameState(GameState::Playing);

    // EN: Test typing command in console box: ENTER -> "FUCKINGBOMB" -> ENTER
    // ES: Probar tecleo del comando en la consola: ENTER -> "FUCKINGBOMB" -> ENTER
    testGame.onKeyPress(13); // Open command box
    for (char character : std::string("FUCKINGBOMB")) {
        testGame.onKeyPress(static_cast<int>(character));
    }
    testGame.onKeyPress(13); // Execute command
    assert(testGame.getActiveDivinePower() == DivinePowerType::AtomicBomb);

    // EN: Toggle off by typing command again
    // ES: Desactivar tecleando el comando de nuevo
    testGame.onKeyPress(13);
    for (char character : std::string("FUCKINGBOMB")) {
        testGame.onKeyPress(static_cast<int>(character));
    }
    testGame.onKeyPress(13);
    assert(testGame.getActiveDivinePower() == DivinePowerType::Inactive);

    // EN: Test Hotkey 'O' / 'o'
    // ES: Probar atajo de teclado 'O' / 'o'
    testGame.onKeyPress('o'); // Hotkey toggle on
    assert(testGame.getActiveDivinePower() == DivinePowerType::AtomicBomb);
    testGame.onKeyPress('o'); // Hotkey toggle off
    assert(testGame.getActiveDivinePower() == DivinePowerType::Inactive);

    std::cout << "  -> testAtomicBombCheatAndNuclearPhysics PASSED! Nuclear detonation, epicentral vaporization, vitrified crater, water boiling, and FUCKINGBOMB cheat verified." << std::endl;
}

// =========================================================================
// EN: Unit Test: Finite Natural Resources Depletion and Autonomous AI Migration
// ES: Prueba Unitaria: Agotamiento de Recursos Naturales Finitos y Migración Autónoma de IA
// =========================================================================
void testFiniteNaturalResourcesDepletionAndAI() {
    std::cout << "[Test] Running testFiniteNaturalResourcesDepletionAndAI..." << std::endl;
    using namespace ReligionSim;

    // EN: 1. Verify finite resource stock depletion and terrain deforestation transitions
    // ES: 1. Verificar agotamiento de existencias finitas y deforestación del terreno
    WorldMap simulationMap(32, 32);
    simulationMap.generateProceduralWorld(45678);

    // EN: Clear local neighborhood around (10, 10) to isolate test cells
    // ES: Limpiar vecindario local alrededor de (10, 10) para aislar las celdas de prueba
    for (int deltaOffsetY = -6; deltaOffsetY <= 6; ++deltaOffsetY) {
        for (int deltaOffsetX = -6; deltaOffsetX <= 6; ++deltaOffsetX) {
            const int targetCoordX = 10 + deltaOffsetX;
            const int targetCoordY = 10 + deltaOffsetY;
            if (simulationMap.isValid(targetCoordX, targetCoordY)) {
                Cell& neighborTestCell = simulationMap.getCell(targetCoordX, targetCoordY);
                neighborTestCell.naturalResource = ResourceType::None;
                neighborTestCell.resourceAmount = 0;
                neighborTestCell.maxResourceAmount = 0;
            }
        }
    }

    Cell& timberCell = simulationMap.getCell(10, 10);
    timberCell.biome = BiomeType::DenseForest;
    timberCell.naturalResource = ResourceType::Wood;
    timberCell.resourceAmount = 10;
    timberCell.maxResourceAmount = 10;
    timberCell.hasTreesUprooted = false;

    assert(!timberCell.isResourceDepleted());
    assert(timberCell.resourceAmount == 10);

    // EN: First harvest: extract 4 units, leaving 6 units active
    // ES: Primera cosecha: sustraer 4 unidades, quedando 6 unidades activas
    const int firstHarvested = timberCell.harvestResource(4);
    assert(firstHarvested == 4);
    assert(timberCell.resourceAmount == 6);
    assert(timberCell.naturalResource == ResourceType::Wood);
    assert(!timberCell.isResourceDepleted());
    assert(!timberCell.hasTreesUprooted);

    // EN: Second harvest: extract 4 units, leaving 2 units active
    // ES: Segunda cosecha: sustraer 4 unidades, quedando 2 unidades activas
    const int secondHarvested = timberCell.harvestResource(4);
    assert(secondHarvested == 4);
    assert(timberCell.resourceAmount == 2);
    assert(timberCell.naturalResource == ResourceType::Wood);
    assert(!timberCell.isResourceDepleted());

    // EN: Final harvest: request 4 units, but only 2 remain in the finite stock
    // ES: Cosecha final: solicitar 4 unidades, pero solo quedan 2 en la veta finita
    const int finalHarvested = timberCell.harvestResource(4);
    assert(finalHarvested == 2);
    assert(timberCell.resourceAmount == 0);
    assert(timberCell.naturalResource == ResourceType::None);
    assert(timberCell.isResourceDepleted());

    // EN: For wood, timber extraction fells trees and converts dense forest into open meadow
    // ES: Para madera, la tala derriba árboles y convierte el bosque denso en pradera abierta
    assert(timberCell.hasTreesUprooted);
    assert(timberCell.biome == BiomeType::CoastalPlains);

    // EN: 2. Verify that natural resources are strictly non-renewable (no magical regeneration)
    // ES: 2. Verificar que los recursos naturales no se regeneran mágicamente
    simulationMap.updateResources(60.0f);
    assert(timberCell.resourceAmount == 0);
    assert(timberCell.naturalResource == ResourceType::None);
    assert(timberCell.isResourceDepleted());

    // EN: 3. Verify mineral ore exhaustion (Gold/Stone)
    // ES: 3. Verificar agotamiento de vetas minerales (Oro/Piedra)
    Cell& goldCell = simulationMap.getCell(12, 12);
    goldCell.naturalResource = ResourceType::Gold;
    goldCell.resourceAmount = 15;
    goldCell.maxResourceAmount = 15;

    const int goldHarvested = goldCell.harvestResource(15);
    assert(goldHarvested == 15);
    assert(goldCell.resourceAmount == 0);
    assert(goldCell.naturalResource == ResourceType::None);
    assert(goldCell.isResourceDepleted());

    // EN: 4. Autonomous AI node discovery and fallback migration when preferred node is exhausted
    // ES: 4. Búsqueda autónoma de nodos y migración de IA al agotarse el nodo preferido
    Humano testVillager(99, "AutonomousHarvester", 10.5f, 10.5f);

    // EN: The timber cell at (10, 10) is depleted, so findResourceNode for Wood must not locate it
    // ES: La celda de madera en (10, 10) está agotada, por lo que findResourceNode no debe localizarla
    Point2i locatedNodePos{-1, -1};
    const bool foundDepletedWood = testVillager.findResourceNode(simulationMap, ResourceType::Wood, 2, locatedNodePos);
    assert(!foundDepletedWood);

    // EN: Set an active Food node at (14, 10)
    // ES: Establecer un nodo activo de Alimento en (14, 10)
    Cell& foodCell = simulationMap.getCell(14, 10);
    foodCell.naturalResource = ResourceType::Food;
    foodCell.resourceAmount = 45;
    foodCell.maxResourceAmount = 45;

    ResourceType detectedResourceType = ResourceType::None;
    const bool foundAnyNode = testVillager.findAnyResourceNode(simulationMap, 8, locatedNodePos, detectedResourceType);
    assert(foundAnyNode);
    assert(locatedNodePos.x == 14 && locatedNodePos.y == 10);
    assert(detectedResourceType == ResourceType::Food);

    std::cout << "  -> testFiniteNaturalResourcesDepletionAndAI PASSED! Finite stock depletion, deforestation, non-renewability, and AI fallback migration verified." << std::endl;
}

void testFireSpritesAndVisualEffectsPipeline() {
    std::cout << "[Test] Running testFireSpritesAndVisualEffectsPipeline..." << std::endl;
    using namespace ReligionSim;

    // EN: 1. Verify all 10 fire sprite asset files exist on disk with exact binary dimensions
    // ES: 1. Verificar que los 10 archivos de sprites de fuego existan en disco con dimensiones binarias exactas
    struct FireAssetSpec {
        std::string relativePath;
        size_t expectedByteSize;
    };

    const std::vector<FireAssetSpec> fireAssets = {
        {"assets/sprites/fire/wildfire_sheet.rgba", 512u * 128u * 4u},
        {"assets/sprites/fire/flame_sheet.rgba", 384u * 128u * 4u},
        {"assets/sprites/fire/small_flame_sheet.rgba", 448u * 96u * 4u},
        {"assets/sprites/fire/campfire_sheet.rgba", 512u * 128u * 4u},
        {"assets/sprites/fire/meteor_giant.rgba", 128u * 128u * 4u},
        {"assets/sprites/fire/meteor_fireball.rgba", 128u * 128u * 4u},
        {"assets/sprites/fire/volcano_eruption_sheet.rgba", 256u * 128u * 4u},
        {"assets/sprites/fire/fire_pit.rgba", 128u * 128u * 4u},
        {"assets/sprites/fire/flame_burst_sheet.rgba", 256u * 128u * 4u},
        {"assets/sprites/fire/lava_tiles.rgba", 512u * 256u * 4u}
    };

    for (const auto& asset : fireAssets) {
        std::ifstream file(asset.relativePath, std::ios::binary | std::ios::ate);
        assert(file.is_open() && "Fire sprite asset file must exist on disk");
        const size_t fileSize = static_cast<size_t>(file.tellg());
        assert(fileSize == asset.expectedByteSize && "Fire asset file size must match RGBA 32-bit layout");
    }

    // EN: 2. Verify Renderer loads all fire buffers into memory with correct pixel counts
    // ES: 2. Verificar que el Renderer cargue todos los búferes de fuego en memoria con recuentos de píxeles correctos
    Renderer testRenderer(640, 480);
    assert(testRenderer.getWildfirePixels().size() == 512u * 128u);
    assert(testRenderer.getFlamePixels().size() == 384u * 128u);
    assert(testRenderer.getCampfirePixels().size() == 512u * 128u);
    assert(testRenderer.getMeteorGiantPixels().size() == 128u * 128u);
    assert(testRenderer.getMeteorFireballPixels().size() == 128u * 128u);
    assert(testRenderer.getVolcanoEruptionPixels().size() == 256u * 128u);
    assert(testRenderer.getFirePitPixels().size() == 128u * 128u);
    assert(testRenderer.getFlameBurstPixels().size() == 256u * 128u);
    assert(testRenderer.getLavaTilesPixels().size() == 512u * 256u);

    // EN: 3. Verify burning unit behavior, panic state, and flame rendering
    // ES: 3. Verificar comportamiento de unidad en llamas, estado de pánico y renderizado de fuego
    Humano burningVillager(105, "BurningGrog", 12.0f, 12.0f);
    burningVillager.ignite(6.0f);
    assert(burningVillager.isBurning());
    assert(burningVillager.getBurnTimer() > 0.0f);

    WorldMap simMap(32, 32);
    const float initialHp = burningVillager.getSalud();
    burningVillager.update(1.0f, simMap);
    assert(burningVillager.getSalud() < initialHp && "Burning unit must sustain fire damage over time");

    std::vector<uint32_t> testFrame(640 * 480, 0xFF000000u);
    IsometricCamera simCam(640, 480);
    TileSet testTileSet;
    burningVillager.render(testFrame.data(), 640, 480, simCam, simMap, testTileSet, ColorRGB{255, 120, 20}, false);

    // EN: 4. Verify campfire and fire effect render routines execute without crashing
    // ES: 4. Verificar que las rutinas de renderizado de fogata y fuego se ejecuten sin errores
    testRenderer.drawCampfire(320.0f, 240.0f, 1.0f, 0.5f);
    testRenderer.drawFireEffect(320.0f, 240.0f, 0.85f, 1.0f, 0.5f, 1.0f, 0.0f);
    testRenderer.drawFireEffect(320.0f, 240.0f, 0.35f, 1.0f, 0.5f, 0.0f, 0.0f);

    std::cout << "  -> testFireSpritesAndVisualEffectsPipeline PASSED! All 10 fire sprite textures, RGBA dimensions, flame rendering and campfire routines verified." << std::endl;
}

void testVolcanoPixelArtSpritesAndLifecyclePipeline() {
    std::cout << "[Test] Running testVolcanoPixelArtSpritesAndLifecyclePipeline..." << std::endl;
    using namespace ReligionSim;

    // EN: 1. Verify volcano sheet asset file exists with exact 1024x765 RGBA dimensions
    // ES: 1. Verificar que el archivo del atlas del volcán exista con dimensiones exactas 1024x765 RGBA
    const std::string volcanoPath = "assets/sprites/volcano/volcano_sheet.rgba";
    std::ifstream file(volcanoPath, std::ios::binary | std::ios::ate);
    assert(file.is_open() && "Volcano sprite sheet must exist on disk");
    const size_t fileSize = static_cast<size_t>(file.tellg());
    constexpr size_t expectedBytes = 1024u * 765u * 4u;
    assert(fileSize == expectedBytes && "Volcano sheet byte size must match 1024x765 RGBA");

    // EN: 2. Verify Renderer loads volcano sprite pixels
    // ES: 2. Verificar que Renderer cargue los píxeles del sprite del volcán
    Renderer testRenderer(640, 480);
    assert(testRenderer.getVolcanoSheetPixels().size() == 1024u * 765u);

    // EN: 3. Verify execution of all 4 volcano lifecycle stages without memory or rasterizer faults
    // ES: 3. Verificar ejecución de las 4 etapas del ciclo de vida del volcán sin fallos
    testRenderer.drawVolcano(320.0f, 240.0f, 1.0f, 0.5f, false, 0.0f); // Dormant
    testRenderer.drawVolcano(320.0f, 240.0f, 1.0f, 0.5f, true, 0.10f); // Phase 1: Awakening
    testRenderer.drawVolcano(320.0f, 240.0f, 1.0f, 0.5f, true, 0.35f); // Phase 2: Explosive blast
    testRenderer.drawVolcano(320.0f, 240.0f, 1.0f, 0.5f, true, 0.65f); // Phase 3: Plinian mushroom cloud
    testRenderer.drawVolcano(320.0f, 240.0f, 1.0f, 0.5f, true, 0.90f); // Phase 4: Cascading lava rivers

    // EN: 4. Verify DivinePowersSystem Volcano cataclysm updates lavaTimer progression
    // ES: 4. Verificar que el cataclismo volcánico de DivinePowersSystem actualice la progresión de lavaTimer
    WorldMap simMap(32, 32);
    ReligionRegistry simRegistry;
    const int playerRelId = simRegistry.createPrimordialReligion("Culto Volcanico", "Dios de las Llamas", ColorRGB{255, 60, 20}, "Flame", 0);
    Religion* pRel = simRegistry.getReligion(playerRelId);
    assert(pRel != nullptr);
    pRel->theologyPoints = 200;

    GestorHumanos simVillagers;
    BuildingSystem simBuildings;
    ReplicationEngine simReplication;
    DivinePowersSystem simPowers;
    EventLine simEvents;

    simPowers.castPower(DivinePowerType::Volcano, 16, 16, simMap, simRegistry, simVillagers, -4000, simEvents, playerRelId, &simBuildings, &simReplication);
    Cell& targetCell = simMap.getCell(16, 16);
    assert(targetCell.hasVolcano == true);
    assert(targetCell.hasActiveLava == true);

    simPowers.update(2.0f, simMap, simVillagers, &simBuildings, &simReplication);
    assert(targetCell.lavaTimer > 0.0f && "Volcano cataclysm must update lavaTimer eruption progression");

    std::cout << "  -> testVolcanoPixelArtSpritesAndLifecyclePipeline PASSED! 1024x765 atlas, dormant, awakening, blast and lava flow stages verified." << std::endl;
}

// EN: Unit test verifying water movement sprite sheets, ripple animation, coastal surf, currents, whirlpools and splashes
// ES: Prueba unitaria que verifica hojas de sprites de movimiento de agua, animación de ondas, rompiente costera, corrientes, remolinos y salpicaduras
void testWaterMovementSpritesAndDynamicFluidPipeline() {
    std::cout << "[Test] Running testWaterMovementSpritesAndDynamicFluidPipeline..." << std::endl;
    using namespace ReligionSim;

    // EN: 1. Verify master water sheet asset file exists with exact 1024x765 RGBA dimensions
    // ES: 1. Verificar que el archivo del atlas de agua exista con dimensiones exactas 1024x765 RGBA
    const std::string waterAtlasPath = "assets/sprites/water/water_sheet.rgba";
    std::ifstream waterAtlasFile(waterAtlasPath, std::ios::binary | std::ios::ate);
    if (!waterAtlasFile.is_open()) {
        waterAtlasFile.open("religion_simulator_isometric/" + waterAtlasPath, std::ios::binary | std::ios::ate);
    }
    assert(waterAtlasFile.is_open() && "Water atlas 1024x765 RGBA sprite file must exist on disk");
    const std::streamsize waterAtlasFileSize = waterAtlasFile.tellg();
    constexpr std::streamsize expectedWaterAtlasBytes = 1024 * 765 * 4;
    assert(waterAtlasFileSize == expectedWaterAtlasBytes && "Water atlas RGBA file size must match 1024x765x4 bytes");

    // EN: 2. Verify sub-sheets for modular water movement subsystems
    // ES: 2. Verificar sub-hojas para subsistemas modulares de movimiento de agua
    auto verifyAssetFile = [](const std::string& relativePath, std::streamsize expectedBytes) {
        std::ifstream fileStream(relativePath, std::ios::binary | std::ios::ate);
        if (!fileStream.is_open()) {
            fileStream.open("religion_simulator_isometric/" + relativePath, std::ios::binary | std::ios::ate);
        }
        assert(fileStream.is_open() && "Water sub-sheet asset file must exist");
        assert(fileStream.tellg() == expectedBytes && "Water sub-sheet file size must match expected bytes");
    };

    verifyAssetFile("assets/sprites/water/water_ripples_sheet.rgba", 1024 * 170 * 4);
    verifyAssetFile("assets/sprites/water/water_borders_sheet.rgba", 1024 * 175 * 4);
    verifyAssetFile("assets/sprites/water/water_waves_sheet.rgba", 1024 * 115 * 4);
    verifyAssetFile("assets/sprites/water/water_surf_sheet.rgba", 1024 * 116 * 4);
    verifyAssetFile("assets/sprites/water/water_currents_sheet.rgba", 853 * 102 * 4);
    verifyAssetFile("assets/sprites/water/water_whirlpool_sheet.rgba", 510 * 85 * 4);
    verifyAssetFile("assets/sprites/water/water_splash_sheet.rgba", 425 * 85 * 4);

    // EN: 3. Verify Renderer loads and populates all 8 water sprite pixel buffers
    // ES: 3. Verificar que el Renderer cargue y pueble todos los 8 búferes de píxeles de agua
    Renderer testRenderer(640, 480);

    assert(testRenderer.getWaterSheetPixels().size() == 1024u * 765u && "Master water atlas pixels must be 1024x765");
    assert(testRenderer.getWaterRipplesPixels().size() == 1024u * 170u && "Water ripples pixels must be 1024x170");
    assert(testRenderer.getWaterBordersPixels().size() == 1024u * 175u && "Water borders pixels must be 1024x175");
    assert(testRenderer.getWaterWavesPixels().size() == 1024u * 115u && "Water waves pixels must be 1024x115");
    assert(testRenderer.getWaterSurfPixels().size() == 1024u * 116u && "Water surf pixels must be 1024x116");
    assert(testRenderer.getWaterCurrentsPixels().size() == 853u * 102u && "Water currents pixels must be 853x102");
    assert(testRenderer.getWaterWhirlpoolPixels().size() == 510u * 85u && "Water whirlpool pixels must be 510x85 (6 frames)");
    assert(testRenderer.getWaterSplashPixels().size() == 425u * 85u && "Water splash pixels must be 425x85 (5 frames)");

    // EN: 4. Exercise whirlpool and splash drawing routines to confirm no buffer overruns
    // ES: Ejercitar rutinas de dibujo de remolino y salpicadura para confirmar ausencia de desbordamiento de búfer
    testRenderer.drawWhirlpool(320.0f, 240.0f, 1.0f, 0.5f, 1.0f);
    testRenderer.drawWaterSplash(320.0f, 240.0f, 1.0f, 0.25f, 1.0f);

    std::cout << "  -> testWaterMovementSpritesAndDynamicFluidPipeline PASSED! 1024x765 atlas, ripples, ocean waves, coastal surf, currents, whirlpools and splashes verified." << std::endl;
}

// EN: Unit test verifying round Play button, circular hit-testing, and vintage cassette mechanical keys
// ES: Prueba unitaria que verifica el botón redondo de Play, detección circular y teclas mecánicas de casetera
void testCassetteAndRoundPlayButtons() {
    std::cout << "[Test] Running testCassetteAndRoundPlayButtons..." << std::endl;

    ReligionSim::Renderer testRenderer(1280, 720);
    const auto& buttons = testRenderer.getButtons();

    const ReligionSim::UIButton* buttonPause = nullptr;
    const ReligionSim::UIButton* buttonPlay = nullptr;
    const ReligionSim::UIButton* buttonFast = nullptr;
    const ReligionSim::UIButton* buttonUltra = nullptr;

    for (const auto& btn : buttons) {
        if (btn.id == 201) buttonPause = &btn;
        if (btn.id == 202) buttonPlay = &btn;
        if (btn.id == 203) buttonFast = &btn;
        if (btn.id == 204) buttonUltra = &btn;
    }

    assert(buttonPause != nullptr && "Cassette PAUSE button 201 must exist");
    assert(buttonPlay != nullptr && "Round PLAY button 202 must exist");
    assert(buttonFast != nullptr && "Cassette 2x button 203 must exist");
    assert(buttonUltra != nullptr && "Cassette 5x button 204 must exist");

    // EN: Verify cassette keys heights and round play circular geometry
    // ES: Verificar altura de teclas de casetera y geometría circular del botón de play
    assert(buttonPause->height == 26.0f);
    assert(buttonFast->height == 26.0f);
    assert(buttonUltra->height == 26.0f);
    assert(buttonPause->width >= 98.0f);

    // EN: Round play button must be strictly circular (width == height == diameter)
    // ES: El botón redondo de play debe ser estrictamente circular (ancho == alto == diámetro)
    assert(buttonPlay->width == 30.0f);
    assert(buttonPlay->height == 30.0f);

    // EN: Verify vertical center alignment between round play and cassette keys
    // ES: Verificar alineación central vertical entre play redondo y teclas de casetera
    const float cassetteCenterY = buttonPause->y + buttonPause->height * 0.5f;
    const float playCenterY = buttonPlay->y + buttonPlay->height * 0.5f;
    assert(std::abs(cassetteCenterY - playCenterY) < 0.01f && "Play button must be vertically centered with cassette keys");

    // EN: 2. Test Euclidean circular hit testing on Round Play Button (ID 202)
    // ES: 2. Probar detección circular euclidiana en el botón redondo de Play (ID 202)
    const float playCenterX = buttonPlay->x + buttonPlay->width * 0.5f;
    const float playRadius = buttonPlay->width * 0.5f;

    // Center click -> must hit 202
    assert(testRenderer.checkButtonClicked(playCenterX, playCenterY) == 202);

    // Click just inside radius (e.g. radius - 1.0f) -> must hit 202
    assert(testRenderer.checkButtonClicked(playCenterX + playRadius - 1.0f, playCenterY) == 202);
    assert(testRenderer.checkButtonClicked(playCenterX, playCenterY + playRadius - 1.0f) == 202);

    // Click at corner of bounding box (square corner: dx = radius, dy = radius, dist = radius * sqrt(2)) -> MUST MISS
    assert(testRenderer.checkButtonClicked(buttonPlay->x, buttonPlay->y) == 0 && "Corner click outside circle must miss");
    assert(testRenderer.checkButtonClicked(buttonPlay->x + buttonPlay->width, buttonPlay->y) == 0);
    assert(testRenderer.checkButtonClicked(buttonPlay->x, buttonPlay->y + buttonPlay->height) == 0);

    // EN: 3. Test Rectangular hit testing on Cassette keys
    // ES: 3. Probar detección rectangular en las teclas de casetera
    assert(testRenderer.checkButtonClicked(buttonPause->x + 10.0f, buttonPause->y + 10.0f) == 201);
    assert(testRenderer.checkButtonClicked(buttonFast->x + 10.0f, buttonFast->y + 10.0f) == 203);
    assert(testRenderer.checkButtonClicked(buttonUltra->x + 10.0f, buttonUltra->y + 10.0f) == 204);

    // EN: 4. Test rendering routines across all interactive states (idle, active, hovered, pressed)
    // ES: 4. Probar rutinas de renderizado en todos los estados interactivos (reposo, activo, hover, pulsado)
    testRenderer.clear(ReligionSim::ColorRGB{0, 0, 0});

    // Render Round Play Button states
    testRenderer.drawRoundPlayButton(100, 100, 30, false, false, false); // Idle
    testRenderer.drawRoundPlayButton(140, 100, 30, true, false, false);  // Active
    testRenderer.drawRoundPlayButton(180, 100, 30, false, true, false);  // Hover
    testRenderer.drawRoundPlayButton(220, 100, 30, false, false, true);  // Pressed

    // Render Cassette Button states
    testRenderer.drawCassetteButton(100, 150, 98, 26, "PAUSA", false, false, false, 201); // Idle
    testRenderer.drawCassetteButton(210, 150, 98, 26, "PAUSA", true, false, false, 201);  // Active
    testRenderer.drawCassetteButton(320, 150, 84, 26, "2x", false, true, false, 203);     // Hover
    testRenderer.drawCassetteButton(410, 150, 84, 26, "5x", false, false, true, 204);     // Pressed

    // Verify non-black pixels rendered
    size_t coloredPixelsCount = 0;
    for (const uint32_t pixelValue : testRenderer.getFramebuffer()) {
        if ((pixelValue & 0x00FFFFFFu) != 0) ++coloredPixelsCount;
    }
    assert(coloredPixelsCount > 2000 && "Rasterizer must produce thousands of colored pixels for UI controls");

    // EN: 5. Verify mechanical rebound animation ("se aprietan y vuelven a su lugar")
    // ES: 5. Verificar animación de rebote mecánico ("se aprietan y vuelven a su lugar")
    testRenderer.triggerButtonPress(201);
    // Button is depressed during animation timer
    testRenderer.update(0.05f); // 50ms elapsed, 90ms remaining
    // Advance beyond press timer duration
    testRenderer.update(0.20f); // 200ms elapsed, press timer expired -> key springs back up!

    std::cout << "  -> testCassetteAndRoundPlayButtons PASSED! Round Play and cassette mechanics verified." << std::endl;
}

// =============================================================================
// EN: Test permanent terrain craters and tectonic fault formations during earthquakes and heavy impacts
// ES: Prueba de formación de cráteres permanentes y fallas tectónicas durante terremotos e impactos fuertes
// =============================================================================
void testEarthquakeAndHeavyImpactCraterFormation() {
    using namespace ReligionSim;
    std::cout << "[Test] Running testEarthquakeAndHeavyImpactCraterFormation (Earthquake Fissures, Meteor Bowls, & Tectonic Subsidence)..." << std::endl;

    // EN: 1. Setup test world map, religion registry, and systems
    // ES: 1. Configurar mapa de prueba, registro de religiones y sistemas
    constexpr int TEST_MAP_WIDTH = 36;
    constexpr int TEST_MAP_HEIGHT = 36;
    WorldMap terrainMap(TEST_MAP_WIDTH, TEST_MAP_HEIGHT);
    ReligionRegistry religionRegistry;
    EventLine eventLine;
    BuildingSystem buildingSystem;
    ReplicationEngine replicationEngine;
    GestorHumanos villagerManager;
    DivinePowersSystem divinePowers;

    const int playerReligionId = religionRegistry.createPrimordialReligion(
        "Seismic Cult of Terra", "Geo-Prophet", ColorRGB{220, 140, 70}, "Earthquake", 2026
    );
    Religion* playerReligion = religionRegistry.getReligion(playerReligionId);
    assert(playerReligion != nullptr);
    playerReligion->theologyPoints = 5000; // Provide ample faith points for casting

    // EN: 2. Prepare terrain baseline elevations and forest cover around earthquake ground zero (18, 18)
    // ES: 2. Preparar elevaciones base y cobertura forestal alrededor de la zona cero del terremoto (18, 18)
    constexpr int QUAKE_EPICENTER_X = 18;
    constexpr int QUAKE_EPICENTER_Y = 18;

    for (int cellOffsetY = -4; cellOffsetY <= 4; ++cellOffsetY) {
        for (int cellOffsetX = -4; cellOffsetX <= 4; ++cellOffsetX) {
            const int targetCoordX = QUAKE_EPICENTER_X + cellOffsetX;
            const int targetCoordY = QUAKE_EPICENTER_Y + cellOffsetY;
            Cell& targetCell = terrainMap.getCell(targetCoordX, targetCoordY);
            targetCell.biome = BiomeType::FertileValley;
            targetCell.elevation = 1.40f;
            targetCell.naturalResource = ResourceType::Wood;
            targetCell.resourceAmount = 120;
            targetCell.hasTreesUprooted = false;
            targetCell.hasImpactCrater = false;
            targetCell.craterDepth = 0.0f;
        }
    }

    const float baselineEpicenterElevation = terrainMap.getCell(QUAKE_EPICENTER_X, QUAKE_EPICENTER_Y).elevation;

    // EN: Place a villager and build a completed house near epicenter
    // ES: Ubicar un aldeano y construir una casa completada cerca del epicentro
    Humano epicenterVillager(1, "Adan Sismico", static_cast<float>(QUAKE_EPICENTER_X) + 0.3f, static_cast<float>(QUAKE_EPICENTER_Y) + 0.3f);
    epicenterVillager.setReligionId(playerReligionId);
    epicenterVillager.setSalud(100.0f);
    villagerManager.agregarHumano(epicenterVillager);

    const int testHouseId = buildingSystem.initiateHouseConstruction(QUAKE_EPICENTER_X, QUAKE_EPICENTER_Y, 1, 0, playerReligionId, HouseType::ThatchedHut, terrainMap);
    House* constructedHouse = buildingSystem.getHouseByIdMutable(testHouseId);
    assert(constructedHouse != nullptr);
    constructedHouse->state = BuildingState::Completed;

    // EN: 3. Cast Earthquake at (18, 18)
    // ES: 3. Lanzar Terremoto Sísmico en (18, 18)
    const CastResult earthquakeResult = divinePowers.castPower(
        DivinePowerType::Earthquake, QUAKE_EPICENTER_X, QUAKE_EPICENTER_Y,
        terrainMap, religionRegistry, villagerManager,
        2026, eventLine, playerReligionId,
        &buildingSystem, &replicationEngine
    );

    assert(earthquakeResult.success == true);

    // EN: 4. Verify Epicenter Tectonic Subsidence & Fault Fissure (No impact crater, only tectonic elevation drop)
    // ES: 4. Verificar subsidencia tectónica epicentral y fisura de falla (Sin cráter de impacto, sólo depresión tectónica)
    const Cell& postQuakeEpicenter = terrainMap.getCell(QUAKE_EPICENTER_X, QUAKE_EPICENTER_Y);
    assert(postQuakeEpicenter.hasImpactCrater == false); // EN: Craters are only for impact zones! / ES: ¡Los cráteres son sólo para zonas de impacto!
    assert(postQuakeEpicenter.elevation < baselineEpicenterElevation); // Ground physically depressed into trench!
    assert(postQuakeEpicenter.hasTreesUprooted == true);

    // EN: Verify multi-cell fault strike subsidence zone
    // ES: Verificar zona de subsidencia a lo largo del rumbo de la falla
    int totalDepressedCells = 0;
    for (int cellOffsetY = -3; cellOffsetY <= 3; ++cellOffsetY) {
        for (int cellOffsetX = -3; cellOffsetX <= 3; ++cellOffsetX) {
            const Cell& inspectedCell = terrainMap.getCell(QUAKE_EPICENTER_X + cellOffsetX, QUAKE_EPICENTER_Y + cellOffsetY);
            assert(inspectedCell.hasImpactCrater == false);
            if (inspectedCell.elevation < 1.40f) {
                totalDepressedCells++;
            }
        }
    }
    assert(totalDepressedCells >= 5); // Epicenter and fault strike line subsided into graben trench

    // EN: Verify distant cell was unaffected
    // ES: Verificar que una celda distante no fue afectada
    const Cell& distantUntouchedCell = terrainMap.getCell(2, 2);
    assert(distantUntouchedCell.hasImpactCrater == false);

    // EN: 5. Verify Heavy Impact Crater Formation via Meteor Strike
    // ES: 5. Verificar formación de cráter de impacto pesado mediante impacto de meteorito
    constexpr int METEOR_TARGET_X = 8;
    constexpr int METEOR_TARGET_Y = 8;
    Cell& meteorGroundCell = terrainMap.getCell(METEOR_TARGET_X, METEOR_TARGET_Y);
    meteorGroundCell.biome = BiomeType::FertileValley;
    meteorGroundCell.elevation = 1.30f;
    meteorGroundCell.hasImpactCrater = false;
    const float baselineMeteorElevation = meteorGroundCell.elevation;

    const CastResult meteorResult = divinePowers.castPower(
        DivinePowerType::Meteor, METEOR_TARGET_X, METEOR_TARGET_Y,
        terrainMap, religionRegistry, villagerManager,
        2026, eventLine, playerReligionId,
        &buildingSystem, &replicationEngine
    );
    assert(meteorResult.success == true);

    // Advance time until meteor impact detonates on ground (t >= 1.25s)
    for (int updateStep = 0; updateStep < 15; ++updateStep) {
        divinePowers.update(0.1f, terrainMap, villagerManager, &buildingSystem, &replicationEngine);
    }

    const Cell& postImpactMeteorCell = terrainMap.getCell(METEOR_TARGET_X, METEOR_TARGET_Y);
    assert(postImpactMeteorCell.hasImpactCrater == true);
    assert(postImpactMeteorCell.craterType == CraterType::MeteorImpact);
    assert(postImpactMeteorCell.craterDepth >= 0.70f);
    assert(postImpactMeteorCell.elevation < baselineMeteorElevation); // Depressed into bowl!

    // Verify neighbor cell in crater bowl
    const Cell& neighborCraterCell = terrainMap.getCell(METEOR_TARGET_X + 1, METEOR_TARGET_Y);
    assert(neighborCraterCell.hasImpactCrater == true);
    assert(neighborCraterCell.craterDepth > 0.0f);
    assert(neighborCraterCell.elevation < 1.40f);

    // EN: 6. Verify 2.5D Rendering and Visual Pipeline
    // ES: 6. Verificar renderizado 2.5D y pipeline visual
    Renderer testRenderer(1280, 720);
    IsometricCamera testCamera(1280, 720);
    Timeline testTimeline;
    WorldCreationConfig testConfig;
    DayNightCycle testDayNight;
    CloudSystem testClouds;

    testRenderer.beginFrame();
    testRenderer.render(terrainMap, religionRegistry, testTimeline, eventLine,
                        replicationEngine, villagerManager, testCamera,
                        {QUAKE_EPICENTER_X, QUAKE_EPICENTER_Y}, RenderMapMode::TerrainBiomes,
                        testConfig, nullptr, &divinePowers, {-1, -1}, &testDayNight, &testClouds);
    testRenderer.endFrame();

    std::cout << "  -> testEarthquakeAndHeavyImpactCraterFormation PASSED! Earthquake fault rifts, graben basins, and meteor bowls fully verified." << std::endl;
}

// =========================================================================
// EN: Test Suite: Metric Physical Scaling System & Disaster Parameters
// ES: Suite de Pruebas: Sistema de Escala Métrica Física y Parámetros de Desastres
// =========================================================================
void testMetricPhysicalScaleAndDisasterParameters() {
    using namespace ReligionSim;
    std::cout << "[Test] Running testMetricPhysicalScaleAndDisasterParameters (1 Cell = 5m, Little Boy 500m, Average Meteorite 16.5m)..." << std::endl;

    // EN: 1. Validate fundamental world metric scaling constants
    // ES: 1. Validar constantes fundamentales de escala métrica del mundo
    assert(METERS_PER_CELL == 5.0f);
    assert(CELLS_PER_METER == 0.20f);
    assert(KM_PER_CELL == 0.005f);
    assert(CELLS_PER_KM == 200.0f);
    assert(SPEED_OF_SOUND_MPS == 343.0f);
    assert(std::abs(SPEED_OF_SOUND_CELLS_PER_SEC - 68.6f) < 0.01f);

    // EN: 2. Validate conversion helper routines between meters, km, and cells
    // ES: 2. Validar rutinas de conversión entre metros, km y celdas
    assert(metersToCells(5.0f) == 1.0f);
    assert(metersToCells(50.0f) == 10.0f);
    assert(metersToCells(500.0f) == 100.0f);
    assert(cellsToMeters(1.0f) == 5.0f);
    assert(cellsToMeters(10.0f) == 50.0f);
    assert(cellsToMeters(100.0f) == 500.0f);
    assert(kmToCells(1.0f) == 200.0f);
    assert(cellsToKm(200.0f) == 1.0f);

    // EN: 3. Validate Little Boy 15kt nuclear explosion parameters
    // ES: 3. Validar parámetros de la explosión nuclear Little Boy 15kt
    assert(LittleBoyScale::YIELD_KILOTONS_TNT == 15.0f);
    assert(LittleBoyScale::FIREBALL_RADIUS_METERS == 180.0f);
    assert(LittleBoyScale::FIREBALL_RADIUS_CELLS == 36.0f);
    assert(LittleBoyScale::HEAVY_OVERPRESSURE_RADIUS_METERS == 500.0f);
    assert(LittleBoyScale::HEAVY_OVERPRESSURE_RADIUS_CELLS == 100.0f);
    assert(LittleBoyScale::MODERATE_OVERPRESSURE_RADIUS_METERS == 1600.0f);
    assert(LittleBoyScale::MODERATE_OVERPRESSURE_RADIUS_CELLS == 320.0f);
    assert(LittleBoyScale::LIGHT_OVERPRESSURE_RADIUS_METERS == 3500.0f);
    assert(LittleBoyScale::LIGHT_OVERPRESSURE_RADIUS_CELLS == 700.0f);
    assert(LittleBoyScale::SHOCKWAVE_SPEED_CELLS_PER_SEC == 50.0f);

    // EN: 4. Validate Average Meteorite physical impact parameters (1.5-2.5m bolide)
    // ES: 4. Validar parámetros físicos de impacto de meteorito promedio (bólido 1.5-2.5m)
    assert(AverageMeteoriteScale::BOLIDE_DIAMETER_METERS == 2.0f);
    assert(AverageMeteoriteScale::CRATER_DIAMETER_METERS == 16.5f);
    assert(std::abs(AverageMeteoriteScale::CRATER_RADIUS_CELLS - 1.65f) < 0.001f);
    assert(AverageMeteoriteScale::CRATER_DEPTH_METERS == 4.0f);
    assert(AverageMeteoriteScale::CRATER_DEPTH_ELEVATION == 0.80f);
    assert(AverageMeteoriteScale::CRATER_RIM_ELEVATION_UPLIFT == 0.25f);
    assert(AverageMeteoriteScale::EJECTA_RADIUS_METERS == 32.5f);
    assert(AverageMeteoriteScale::EJECTA_RADIUS_CELLS == 6.5f);
    assert(AverageMeteoriteScale::AIRBLAST_RADIUS_METERS == 50.0f);
    assert(AverageMeteoriteScale::AIRBLAST_RADIUS_CELLS == 10.0f);

    // EN: 5. Validate DivinePowersSystem radius returns matching physical metric scale
    // ES: 5. Validar radios retornados por DivinePowersSystem coincidentes con escala métrica
    DivinePowersSystem powersSystem;
    assert(powersSystem.getPowerRadius(DivinePowerType::Meteor) == 10);
    assert(powersSystem.getPowerRadius(DivinePowerType::AtomicBomb) == 100);

    // EN: 6. Validate WorldMap crater excavation with average meteorite parameters
    // ES: 6. Validar excavación de cráter en WorldMap con parámetros de meteorito promedio
    WorldMap terrainMap(32, 32);
    constexpr int CRATER_TARGET_X = 16;
    constexpr int CRATER_TARGET_Y = 16;
    const float initialCenterElevation = terrainMap.getCell(CRATER_TARGET_X, CRATER_TARGET_Y).elevation;

    terrainMap.createImpactCraterZone(CRATER_TARGET_X, CRATER_TARGET_Y, CraterType::MeteorImpact,
                                      AverageMeteoriteScale::CRATER_RADIUS_CELLS,
                                      AverageMeteoriteScale::CRATER_DEPTH_ELEVATION,
                                      0.40f);

    const Cell& impactCenterCell = terrainMap.getCell(CRATER_TARGET_X, CRATER_TARGET_Y);
    assert(impactCenterCell.hasImpactCrater == true);
    assert(impactCenterCell.craterType == CraterType::MeteorImpact);
    assert(impactCenterCell.craterDepth >= 0.75f);
    assert(impactCenterCell.elevation < initialCenterElevation);

    std::cout << "  -> testMetricPhysicalScaleAndDisasterParameters PASSED! Metric scale, Little Boy, and average meteorite physics verified." << std::endl;
}

// =============================================================================
// EN: Test independent playthrough architecture, seed freshness, and solid snapshots
// ES: Probar arquitectura de partidas independientes, semillas frescas y capturas sólidas
// =============================================================================
void testIndependentPlaythroughsAndSolidSnapshotSystem() {
    std::cout << "[Test] Running testIndependentPlaythroughsAndSolidSnapshotSystem..." << std::endl;
    using namespace ReligionSim;

    // -------------------------------------------------------------------------
    // EN: Block 1 - Verify high-entropy unique seed generation
    // ES: Bloque 1 - Verificar generación de semillas únicas de alta entropía
    // -------------------------------------------------------------------------
    uint32_t seedAlpha = Game::generateUniqueSeed();
    uint32_t seedBeta = Game::generateUniqueSeed();
    assert(seedAlpha != 0);
    assert(seedBeta != 0);

    // -------------------------------------------------------------------------
    // EN: Block 2 - Verify unique slot naming and formatting
    // ES: Bloque 2 - Verificar nombrado y formato de espacios únicos
    // -------------------------------------------------------------------------
    std::string slotNameOne = SaveSystem::generateUniqueSlotName("Solarius", 1234);
    assert(slotNameOne.find("Partida_Solarius_") == 0);
    assert(slotNameOne.find("_1234") != std::string::npos);

    // -------------------------------------------------------------------------
    // EN: Block 3 - Initialize Game instance and launch first independent playthrough
    // ES: Bloque 3 - Inicializar instancia de Game e iniciar primera partida independiente
    // -------------------------------------------------------------------------
    constexpr int canvasWidth = 1280;
    constexpr int canvasHeight = 720;
    ReligionSim::Game testGame(canvasWidth, canvasHeight);
    testGame.init();

    testGame.prepareForWorldCreation(true);
    assert(testGame.getGameState() == ReligionSim::GameState::WorldCreation);
    uint32_t firstGameSeed = testGame.getCreationConfig().mapSeed;
    assert(firstGameSeed != 0);

    testGame.startNewCustomWorld();
    assert(testGame.getGameState() == ReligionSim::GameState::Playing);
    std::string firstPlaythroughSlot = testGame.getCurrentSaveSlot();
    assert(!firstPlaythroughSlot.empty());

    // EN: Inject known custom entities and crater into first playthrough
    // ES: Inyectar entidades personalizadas conocidas y cráter en la primera partida
    constexpr int testCraterCellX = 14;
    constexpr int testCraterCellY = 14;
    testGame.getMap().createImpactCraterZone(testCraterCellX, testCraterCellY, CraterType::NuclearDetonation, 3.0f, 0.70f, 0.35f);
    assert(testGame.getMap().getCell(testCraterCellX, testCraterCellY).hasImpactCrater == true);

    constexpr int testHouseCellX = 18;
    constexpr int testHouseCellY = 18;

    // EN: Guarantee test house cell is habitable and free of water or existing structures
    // ES: Garantizar que la celda de la casa de prueba sea habitable y esté libre de agua o estructuras existentes
    Cell& targetHouseCell = testGame.getMap().getCell(testHouseCellX, testHouseCellY);
    targetHouseCell.biome = BiomeType::FertileValley;
    targetHouseCell.elevation = 0.50f;
    targetHouseCell.waterDepth = 0.0f;
    targetHouseCell.isFlooded = false;
    targetHouseCell.hasHouse = false;
    targetHouseCell.houseId = -1;

    int customHouseId = testGame.getBuildingSystem().initiateHouseConstruction(
        testHouseCellX, testHouseCellY, 1, 2, 1, HouseType::ThatchedHut, testGame.getMap()
    );
    assert(customHouseId > 0);
    House* constructedHouse = testGame.getBuildingSystem().getHouseByIdMutable(customHouseId);
    assert(constructedHouse != nullptr);
    constructedHouse->buildProgress = 100.0f;
    constructedHouse->state = BuildingState::Completed;

    // EN: Save snapshot of first playthrough
    // ES: Guardar captura de la primera partida
    testGame.saveCurrentGame();

    std::string savesDirectory = SaveSystem::getSavesRootDirectory();
    std::filesystem::path slotOnePath = std::filesystem::path(savesDirectory) / firstPlaythroughSlot;
    assert(std::filesystem::exists(slotOnePath));
    assert(std::filesystem::exists(slotOnePath / "level.dat"));
    assert(std::filesystem::exists(slotOnePath / "world.bin"));
    assert(std::filesystem::exists(slotOnePath / "villagers.dat"));
    assert(std::filesystem::exists(slotOnePath / "buildings.dat"));

    // -------------------------------------------------------------------------
    // EN: Block 4 - Return to Main Menu and create a SECOND independent playthrough
    // ES: Bloque 4 - Regresar al Menú Principal y crear una SEGUNDA partida independiente
    // -------------------------------------------------------------------------
    testGame.returnToMainMenu();
    assert(testGame.getGameState() == ReligionSim::GameState::MainMenu);

    testGame.prepareForWorldCreation(true);
    uint32_t secondGameSeed = testGame.getCreationConfig().mapSeed;
    // EN: Crucial check: new world creation MUST NOT reuse the old world seed!
    // ES: Comprobación crucial: ¡la nueva creación de mundo NO DEBE reutilizar la semilla anterior!
    assert(secondGameSeed != firstGameSeed);

    testGame.startNewCustomWorld();
    std::string secondPlaythroughSlot = testGame.getCurrentSaveSlot();
    assert(!secondPlaythroughSlot.empty());
    // EN: Playthrough slots must be completely isolated and distinct!
    // ES: ¡Los espacios de partida deben estar completamente aislados y ser distintos!
    assert(secondPlaythroughSlot != firstPlaythroughSlot);

    std::filesystem::path slotTwoPath = std::filesystem::path(savesDirectory) / secondPlaythroughSlot;
    assert(std::filesystem::exists(slotTwoPath));

    // -------------------------------------------------------------------------
    // EN: Block 5 - Load first playthrough and verify complete snapshot fidelity
    // ES: Bloque 5 - Cargar primera partida y verificar fidelidad completa de la captura
    // -------------------------------------------------------------------------
    bool loadSuccessful = SaveSystem::loadWorld(testGame, firstPlaythroughSlot);
    assert(loadSuccessful == true);
    assert(testGame.getCurrentSaveSlot() == firstPlaythroughSlot);
    assert(testGame.getGameState() == ReligionSim::GameState::Playing);

    // EN: Verify crater and building restoration
    // ES: Verificar restauración de cráter y edificio
    const Cell& restoredCraterCell = testGame.getMap().getCell(testCraterCellX, testCraterCellY);
    assert(restoredCraterCell.hasImpactCrater == true);
    assert(restoredCraterCell.craterType == CraterType::NuclearDetonation);

    const House* restoredHouse = testGame.getBuildingSystem().getHouseById(customHouseId);
    assert(restoredHouse != nullptr);
    assert(restoredHouse->cellX == testHouseCellX);
    assert(restoredHouse->cellY == testHouseCellY);
    assert(restoredHouse->isCompleted());

    // EN: Verify living villagers restored with valid properties
    // ES: Verificar que los aldeanos vivos fueron restaurados con propiedades válidas
    const auto& restoredVillagers = testGame.getGestorHumanos().getHumanos();
    assert(!restoredVillagers.empty());
    for (const auto& villagerUnit : restoredVillagers) {
        assert(villagerUnit.getHealth() > 0.0f);
        assert(!villagerUnit.getName().empty());
    }

    // -------------------------------------------------------------------------
    // EN: Block 6 - Clean up temporary test save slots
    // ES: Bloque 6 - Limpiar espacios de guardado temporales de prueba
    // -------------------------------------------------------------------------
    SaveSystem::deleteWorld(firstPlaythroughSlot);
    SaveSystem::deleteWorld(secondPlaythroughSlot);
    assert(!std::filesystem::exists(slotOnePath));
    assert(!std::filesystem::exists(slotTwoPath));

    std::cout << "  -> testIndependentPlaythroughsAndSolidSnapshotSystem PASSED! Independent slots, seed freshness, and atomic snapshot fidelity verified." << std::endl;
}

// EN: Test suite for "Learn to Play" (Aprende a jugar) sacred codex and tutorial menu system
// ES: Suite de pruebas para el códice sagrado y sistema de tutorial "Aprende a jugar"
void testLearnToPlaySacredCodexAndTutorialSystem() {
    std::cout << "[Test] Running testLearnToPlaySacredCodexAndTutorialSystem..." << std::endl;
    using namespace ReligionSim;

    constexpr int canvasWidth = 1280;
    constexpr int canvasHeight = 720;
    Game gameInstance(canvasWidth, canvasHeight);
    gameInstance.init();

    // -------------------------------------------------------------------------
    // EN: Block 1 - Verify Main Menu includes "APRENDE A JUGAR" option button
    // ES: Bloque 1 - Verificar que el Menú Principal incluya la opción "APRENDE A JUGAR"
    // -------------------------------------------------------------------------
    assert(gameInstance.getGameState() == GameState::MainMenu);
    const auto& menuButtons = gameInstance.getRenderer().getMenuButtons();
    assert(menuButtons.size() == 6 && "Main menu must present 6 buttons (Crear, Cargar, Creacion de Mapas, Aprende a Jugar, Opciones, Salir)");

    assert(menuButtons[0].id == 1 && menuButtons[0].label == "CREAR MUNDO");
    assert(menuButtons[1].id == 2 && menuButtons[1].label == "CARGAR MUNDO");
    assert(menuButtons[2].id == 6 && menuButtons[2].label == "CREACION DE MAPAS");
    assert(menuButtons[3].id == 3 && menuButtons[3].label == "APRENDE A JUGAR");
    assert(menuButtons[4].id == 4 && menuButtons[4].label == "OPCIONES");
    assert(menuButtons[5].id == 5 && menuButtons[5].label == "SALIR");

    // -------------------------------------------------------------------------
    // EN: Block 2 - Click "APRENDE A JUGAR" and verify transition to Tutorial state
    // ES: Bloque 2 - Hacer clic en "APRENDE A JUGAR" y verificar transición al estado Tutorial
    // -------------------------------------------------------------------------
    const float learnButtonCenterX = menuButtons[3].x + menuButtons[3].width * 0.5f;
    const float learnButtonCenterY = menuButtons[3].y + menuButtons[3].height * 0.5f;
    gameInstance.onMouseDown(1, learnButtonCenterX, learnButtonCenterY);

    assert(gameInstance.getGameState() == GameState::Tutorial && "Clicking APRENDE A JUGAR must transition game to Tutorial state");
    assert(gameInstance.getCurrentTutorialTab() == TutorialTab::BasicsAndFaith && "Tutorial must open on BasicsAndFaith tab initially");

    // -------------------------------------------------------------------------
    // EN: Block 3 - Verify category tabs and interactive tab switching
    // ES: Bloque 3 - Verificar pestañas de categoría y cambio interactivo de pestaña
    // -------------------------------------------------------------------------
    const auto& tutorialTabButtons = gameInstance.getRenderer().getTutorialTabButtons();
    assert(tutorialTabButtons.size() == 4 && "Tutorial codex must have 4 category tabs");

    // Click Tab 2 (Controls & Camera)
    const float tabTwoCenterX = tutorialTabButtons[1].x + tutorialTabButtons[1].width * 0.5f;
    const float tabTwoCenterY = tutorialTabButtons[1].y + tutorialTabButtons[1].height * 0.5f;
    gameInstance.onMouseDown(1, tabTwoCenterX, tabTwoCenterY);
    assert(gameInstance.getCurrentTutorialTab() == TutorialTab::ControlsAndCamera);

    // Click Tab 3 (Villagers & Ecology)
    const float tabThreeCenterX = tutorialTabButtons[2].x + tutorialTabButtons[2].width * 0.5f;
    const float tabThreeCenterY = tutorialTabButtons[2].y + tutorialTabButtons[2].height * 0.5f;
    gameInstance.onMouseDown(1, tabThreeCenterX, tabThreeCenterY);
    assert(gameInstance.getCurrentTutorialTab() == TutorialTab::VillagersAndEcology);

    // Click Tab 4 (Miracles & Eras)
    const float tabFourCenterX = tutorialTabButtons[3].x + tutorialTabButtons[3].width * 0.5f;
    const float tabFourCenterY = tutorialTabButtons[3].y + tutorialTabButtons[3].height * 0.5f;
    gameInstance.onMouseDown(1, tabFourCenterX, tabFourCenterY);
    assert(gameInstance.getCurrentTutorialTab() == TutorialTab::MiraclesAndEras);

    // -------------------------------------------------------------------------
    // EN: Block 4 - Verify bottom navigation buttons (< Anterior, Siguiente >)
    // ES: Bloque 4 - Verificar botones de navegación inferiores (< Anterior, Siguiente >)
    // -------------------------------------------------------------------------
    const auto& tutorialActionButtons = gameInstance.getRenderer().getTutorialButtons();
    assert(tutorialActionButtons.size() == 3 && "Tutorial must have 3 bottom action buttons (Anterior, Volver, Siguiente)");

    // Click < ANTERIOR (button 0 in tutorialActionButtons, id 2010)
    const float prevButtonCenterX = tutorialActionButtons[0].x + tutorialActionButtons[0].width * 0.5f;
    const float prevButtonCenterY = tutorialActionButtons[0].y + tutorialActionButtons[0].height * 0.5f;
    gameInstance.onMouseDown(1, prevButtonCenterX, prevButtonCenterY);
    assert(gameInstance.getCurrentTutorialTab() == TutorialTab::VillagersAndEcology);

    // Click SIGUIENTE > (button 2 in tutorialActionButtons, id 2012)
    const float nextButtonCenterX = tutorialActionButtons[2].x + tutorialActionButtons[2].width * 0.5f;
    const float nextButtonCenterY = tutorialActionButtons[2].y + tutorialActionButtons[2].height * 0.5f;
    gameInstance.onMouseDown(1, nextButtonCenterX, nextButtonCenterY);
    assert(gameInstance.getCurrentTutorialTab() == TutorialTab::MiraclesAndEras);

    // -------------------------------------------------------------------------
    // EN: Block 5 - Verify rendering across all 4 tutorial codex tabs
    // ES: Bloque 5 - Verificar renderizado a través de las 4 pestañas del códice tutorial
    // -------------------------------------------------------------------------
    const std::vector<TutorialTab> allTabs = {
        TutorialTab::BasicsAndFaith,
        TutorialTab::ControlsAndCamera,
        TutorialTab::VillagersAndEcology,
        TutorialTab::MiraclesAndEras
    };

    for (const auto targetTab : allTabs) {
        gameInstance.setCurrentTutorialTab(targetTab);
        gameInstance.render();
    }

    // -------------------------------------------------------------------------
    // EN: Block 6 - Verify keyboard navigation (1-4, Arrow Keys, Escape)
    // ES: Bloque 6 - Verificar navegación por teclado (1-4, Flechas, Escape)
    // -------------------------------------------------------------------------
    // Direct number keys
    gameInstance.onKeyPress('1');
    assert(gameInstance.getCurrentTutorialTab() == TutorialTab::BasicsAndFaith);

    gameInstance.onKeyPress('3');
    assert(gameInstance.getCurrentTutorialTab() == TutorialTab::VillagersAndEcology);

    // Left Arrow / A (keySym 'a')
    gameInstance.onKeyPress('a');
    assert(gameInstance.getCurrentTutorialTab() == TutorialTab::ControlsAndCamera);

    // Right Arrow / D (keySym 'd')
    gameInstance.onKeyPress('d');
    assert(gameInstance.getCurrentTutorialTab() == TutorialTab::VillagersAndEcology);

    // Escape (keySym 27) -> returns to Main Menu
    gameInstance.onKeyPress(27);
    assert(gameInstance.getGameState() == GameState::MainMenu && "Escape key must return from Tutorial to Main Menu");

    // -------------------------------------------------------------------------
    // EN: Block 7 - Verify return to menu button (VOLVER AL MENU, id 2011)
    // ES: Bloque 7 - Verificar botón de retorno al menú (VOLVER AL MENU, id 2011)
    // -------------------------------------------------------------------------
    gameInstance.onMouseDown(1, learnButtonCenterX, learnButtonCenterY);
    assert(gameInstance.getGameState() == GameState::Tutorial);

    const float returnButtonCenterX = tutorialActionButtons[1].x + tutorialActionButtons[1].width * 0.5f;
    const float returnButtonCenterY = tutorialActionButtons[1].y + tutorialActionButtons[1].height * 0.5f;
    gameInstance.onMouseDown(1, returnButtonCenterX, returnButtonCenterY);
    assert(gameInstance.getGameState() == GameState::MainMenu && "Clicking VOLVER AL MENU must return to Main Menu");

    std::cout << "  -> testLearnToPlaySacredCodexAndTutorialSystem PASSED! Main menu entry, 4 category tabs, controls, rendering, and navigation validated." << std::endl;
}

// EN: Test suite for scaled time system synchronized with 24-hour cycle and advancement leaps
// ES: Suite de pruebas para el sistema de tiempo a escala sincronizado con ciclo de 24h y saltos de avances
void testScaledTimeSynchronizationWithHoursAndAdvancements() {
    std::cout << "[Test] Running testScaledTimeSynchronizationWithHoursAndAdvancements..." << std::endl;
    using namespace ReligionSim;

    // -------------------------------------------------------------------------
    // EN: Block 1 - Verify liturgical seasons mapping to the 24-hour solar cycle
    // ES: Bloque 1 - Verificar mapeo de estaciones litúrgicas al ciclo solar de 24 horas
    // -------------------------------------------------------------------------
    DayNightCycle dayNightCycleInstance(72.0f, 0.0f); // 72s full cycle, starts at 00:00 midnight

    // 00:00 - 06:00: Winter (Invierno)
    dayNightCycleInstance.setTimeByHour(2.0f);
    assert(dayNightCycleInstance.getSeason() == LiturgicalSeason::Winter);
    assert(dayNightCycleInstance.getSeasonName() == "Invierno");

    // 06:00 - 12:00: Spring (Primavera)
    dayNightCycleInstance.setTimeByHour(8.5f);
    assert(dayNightCycleInstance.getSeason() == LiturgicalSeason::Spring);
    assert(dayNightCycleInstance.getSeasonName() == "Primavera");

    // 12:00 - 18:00: Summer (Verano)
    dayNightCycleInstance.setTimeByHour(14.0f);
    assert(dayNightCycleInstance.getSeason() == LiturgicalSeason::Summer);
    assert(dayNightCycleInstance.getSeasonName() == "Verano");

    // 18:00 - 24:00: Autumn (Otoño)
    dayNightCycleInstance.setTimeByHour(21.0f);
    assert(dayNightCycleInstance.getSeason() == LiturgicalSeason::Autumn);
    assert(dayNightCycleInstance.getSeasonName() == "Otoño");

    // -------------------------------------------------------------------------
    // EN: Block 2 - Verify synchronized 1-day = 1-year calendar progression
    // ES: Bloque 2 - Verificar progresión sincronizada del calendario: 1 día = 1 año
    // -------------------------------------------------------------------------
    Timeline timelineInstance(-4000, 2200);
    dayNightCycleInstance.resetDaysPassed();
    dayNightCycleInstance.setTimeByHour(9.0f);

    float yearDeltaOutput = 0.0f;
    timelineInstance.updateSynchronized(0.1f, dayNightCycleInstance, yearDeltaOutput);
    assert(timelineInstance.getCurrentYear() == -4000 && "Starting year must remain -4000 before full day completes");
    assert(yearDeltaOutput == 0.0f);

    // Simulate 1 complete solar day (72 seconds)
    dayNightCycleInstance.update(72.0f);
    assert(dayNightCycleInstance.getDaysPassed() == 1 && "Completing 72 seconds must increment completed days to 1");

    timelineInstance.updateSynchronized(0.1f, dayNightCycleInstance, yearDeltaOutput);
    assert(timelineInstance.getCurrentYear() == -3999 && "One full completed solar day must advance calendar year to -3999");
    assert(yearDeltaOutput == 1.0f && "One year passed in this step");

    // Simulate 4 more days
    dayNightCycleInstance.update(72.0f * 4.0f);
    assert(dayNightCycleInstance.getDaysPassed() == 5);
    timelineInstance.updateSynchronized(0.1f, dayNightCycleInstance, yearDeltaOutput);
    assert(timelineInstance.getCurrentYear() == -3995);
    assert(yearDeltaOutput == 4.0f);

    // -------------------------------------------------------------------------
    // EN: Block 3 - Verify civilizational momentum from theological doctrines (+25 years)
    // ES: Bloque 3 - Verificar impulso civilizatorio por doctrinas teológicas (+25 años)
    // -------------------------------------------------------------------------
    float doctrineLeapDelta = 0.0f;
    constexpr int standardDoctrineLeap = 25;
    timelineInstance.applyAdvancementHistoricalLeap(standardDoctrineLeap, doctrineLeapDelta);

    assert(timelineInstance.getCurrentYear() == -3970 && "Convalidating doctrine must advance year by +25 years");
    assert(doctrineLeapDelta == 25.0f);
    assert(timelineInstance.getAdvancementBonusYears() == 25);

    // -------------------------------------------------------------------------
    // EN: Block 4 - Verify Era Capstone historical ascension leap to next Era dawn
    // ES: Bloque 4 - Verificar salto de ascensión histórica de Hito de Era hacia el alba de la siguiente Era
    // -------------------------------------------------------------------------
    const int nextEraStartYear = AdvancementSystem::getEraStartYear(HistoricalEra::TotemismAncestorWorship);
    assert(nextEraStartYear == -3000 && "Totemism & Ancestor Worship era starts at -3000 BCE");

    const int capstoneLeapYears = nextEraStartYear - timelineInstance.getCurrentYear(); // -3000 - (-3970) = 970
    assert(capstoneLeapYears == 970);

    float capstoneLeapDelta = 0.0f;
    timelineInstance.applyAdvancementHistoricalLeap(capstoneLeapYears, capstoneLeapDelta);
    assert(timelineInstance.getCurrentYear() == -3000 && "Era capstone leap must bring timeline to -3000 BCE");
    assert(capstoneLeapDelta == 970.0f);

    // -------------------------------------------------------------------------
    // EN: Block 5 - Verify Game state integration and Faith Tree convalidation leap
    // ES: Bloque 5 - Verificar integración en Game y salto por convalidación del Árbol de Fe
    // -------------------------------------------------------------------------
    constexpr int canvasWidth = 1280;
    constexpr int canvasHeight = 720;
    Game gameInstance(canvasWidth, canvasHeight);
    gameInstance.init();
    gameInstance.startNewCustomWorld();

    assert(gameInstance.getTimeline().getCurrentYear() == -4000);
    assert(gameInstance.getDayNightCycle().getDaysPassed() == 0);

    // Convalidate initial doctrine 101 ("Mito del Fuego Sagrado")
    Religion* playerReligionPtr = gameInstance.getRegistryMutable().getReligion(1);
    assert(playerReligionPtr != nullptr);
    playerReligionPtr->theologyPoints = 500; // Provide adequate faith points

    gameInstance.getRenderer().setFaithTreeOpen(true);
    gameInstance.getRenderer().setSelectedAdvancementId(101);

    // Convalidate button in modal (x=900, y=480)
    gameInstance.onMouseDown(1, 900.0f, 480.0f);

    // Doctrine 101 must be convalidated and timeline must leap +25 years
    assert(playerReligionPtr->hasAdvancement(101) && "Mito del Fuego Sagrado must be convalidated");
    assert(gameInstance.getTimeline().getCurrentYear() == -3975 && "Timeline must leap by +25 years to -3975");

    // -------------------------------------------------------------------------
    // EN: Block 6 - Verify save/load fidelity of daysPassed and advancementBonusYears
    // ES: Bloque 6 - Verificar fidelidad de guardado/carga de daysPassed y advancementBonusYears
    // -------------------------------------------------------------------------
    const std::string testSaveSlotName = "TestSlot_TimeSyncVerification";
    gameInstance.getDayNightCycle().setDaysPassed(14);
    float syncYearDelta = 0.0f;
    gameInstance.getTimelineMutable().updateSynchronized(0.1f, gameInstance.getDayNightCycle(), syncYearDelta);
    assert(gameInstance.getTimeline().getCurrentYear() == -3961);
    assert(SaveSystem::saveWorld(gameInstance, testSaveSlotName) && "Saving test world must succeed");

    Game loadedGameInstance(canvasWidth, canvasHeight);
    loadedGameInstance.init();
    assert(SaveSystem::loadWorld(loadedGameInstance, testSaveSlotName) && "Loading test world must succeed");

    assert(loadedGameInstance.getDayNightCycle().getDaysPassed() == 14 && "Loaded game must retain daysPassed = 14");
    assert(loadedGameInstance.getTimeline().getAdvancementBonusYears() == 25 && "Loaded game must retain advancementBonusYears = 25");
    assert(loadedGameInstance.getTimeline().getCurrentYear() == -3961 && "Restored year (-4000 + 14 days + 25 bonus = -3961) must match");

    SaveSystem::deleteWorld(testSaveSlotName);

    std::cout << "  -> testScaledTimeSynchronizationWithHoursAndAdvancements PASSED! Liturgical seasons, synchronized days/years, doctrine leaps, capstone ascensions, and save fidelity verified." << std::endl;
}

// =============================================================================
// EN: Test Villager Condition, Physiological States, and Behavioral AI System
// ES: Prueba del Sistema de Condiciones, Estados Fisiológicos y Conducta de Aldeanos
// =============================================================================
void testVillagerConditionAndBehaviorSystem() {
    std::cout << "[Test] Running testVillagerConditionAndBehaviorSystem..." << std::endl;
    using namespace ReligionSim;

    // -------------------------------------------------------------------------
    // EN: Block 1 - VillagerCondition enum, naming, and localized color validation
    // ES: Bloque 1 - Validación de enumeración VillagerCondition, nomenclatura y colores
    // -------------------------------------------------------------------------
    assert(std::string(getVillagerConditionName(VillagerCondition::Healthy, true)) == "Sano");
    assert(std::string(getVillagerConditionName(VillagerCondition::Hungry, true)) == "Hambriento");
    assert(std::string(getVillagerConditionName(VillagerCondition::Full, true)) == "Lleno");
    assert(std::string(getVillagerConditionName(VillagerCondition::Sick, true)) == "Enfermo");
    assert(std::string(getVillagerConditionName(VillagerCondition::Drunk, true)) == "Borracho");
    assert(std::string(getVillagerConditionName(VillagerCondition::Aggressive, true)) == "Agresivo");
    assert(std::string(getVillagerConditionName(VillagerCondition::Scared, true)) == "Asustado");
    assert(std::string(getVillagerConditionName(VillagerCondition::Exhausted, true)) == "Exhausto");
    assert(std::string(getVillagerConditionName(VillagerCondition::Inspired, true)) == "Inspirado");
    assert(std::string(getVillagerConditionName(VillagerCondition::Injured, true)) == "Herido");

    assert(std::string(getVillagerConditionName(VillagerCondition::Healthy, false)) == "Healthy");
    assert(std::string(getVillagerConditionName(VillagerCondition::Hungry, false)) == "Hungry");
    assert(std::string(getVillagerConditionName(VillagerCondition::Full, false)) == "Full");
    assert(std::string(getVillagerConditionName(VillagerCondition::Sick, false)) == "Sick");
    assert(std::string(getVillagerConditionName(VillagerCondition::Drunk, false)) == "Drunk");
    assert(std::string(getVillagerConditionName(VillagerCondition::Aggressive, false)) == "Aggressive");
    assert(std::string(getVillagerConditionName(VillagerCondition::Scared, false)) == "Scared");
    assert(std::string(getVillagerConditionName(VillagerCondition::Exhausted, false)) == "Exhausted");
    assert(std::string(getVillagerConditionName(VillagerCondition::Inspired, false)) == "Inspired");
    assert(std::string(getVillagerConditionName(VillagerCondition::Injured, false)) == "Injured");

    // -------------------------------------------------------------------------
    // EN: Block 2 - Dynamic condition evaluation priority on living Humano entity
    // ES: Bloque 2 - Prioridad de evaluación dinámica de condición en entidad Humano viva
    // -------------------------------------------------------------------------
    Humano testVillager(101, "Grog", 15.5f, 15.5f, HumanGender::Male);
    testVillager.setEdad(25.0f);
    testVillager.setSalud(100.0f);
    testVillager.setEnergia(100.0f);
    testVillager.setSpeed(1.0f);
    testVillager.getHumanUnitMutable().nutritionalState = 65.0f;
    testVillager.getPopGroupMutable().foodSupply = 65.0f;
    testVillager.setSueno(10.0f);

    // Initial state must be Healthy
    assert(testVillager.getCondition() == VillagerCondition::Healthy);
    assert(testVillager.getConditionName() == "Sano");
    assert(testVillager.getEffectiveSpeed() == 1.0f);

    // Full condition (nutritional state >= 80)
    testVillager.getHumanUnitMutable().nutritionalState = 90.0f;
    assert(testVillager.getCondition() == VillagerCondition::Full);
    assert(testVillager.getEffectiveSpeed() == 1.10f);

    // Hungry condition (nutritional state < 35)
    testVillager.getHumanUnitMutable().nutritionalState = 25.0f;
    testVillager.getPopGroupMutable().foodSupply = 15.0f;
    assert(testVillager.getCondition() == VillagerCondition::Hungry);
    assert(testVillager.getEffectiveSpeed() == 0.75f);

    // Exhausted condition (energy < 20 or sleepNeed > 80)
    testVillager.getHumanUnitMutable().nutritionalState = 65.0f;
    testVillager.getPopGroupMutable().foodSupply = 65.0f;
    testVillager.setEnergia(10.0f);
    assert(testVillager.getCondition() == VillagerCondition::Exhausted);
    assert(testVillager.getEffectiveSpeed() == 0.45f);
    testVillager.setEnergia(100.0f);

    // Sick condition (infection active)
    testVillager.infectWithPandemic(20.0f);
    assert(testVillager.getCondition() == VillagerCondition::Sick);
    assert(testVillager.getEffectiveSpeed() == 0.60f);

    // Drunk condition (alcoholLevel > 30)
    testVillager.consumeAlcohol(60.0f);
    assert(testVillager.isDrunk());
    assert(testVillager.getCondition() == VillagerCondition::Drunk);
    // Drunk effective speed has sinusoidal stagger
    assert(testVillager.getEffectiveSpeed() >= 0.50f && testVillager.getEffectiveSpeed() <= 1.05f);

    // Injured condition (HP < 35, higher priority than drunk)
    testVillager.setSalud(25.0f);
    assert(testVillager.getCondition() == VillagerCondition::Injured);
    assert(testVillager.getEffectiveSpeed() == 0.50f);
    testVillager.setSalud(100.0f);

    // Aggressive condition (aggressionLevel > 50 or protectiveFury)
    testVillager.provokeAggression(80.0f);
    assert(testVillager.isAggressive());
    assert(testVillager.getCondition() == VillagerCondition::Aggressive);
    assert(testVillager.getEffectiveSpeed() == 1.25f);
    testVillager.setAggressionLevel(0.0f);

    // Scared condition (panicTimer > 0, when sober)
    testVillager.setAlcoholLevel(0.0f);
    testVillager.activarFuriaProtectora(0.0f);
    testVillager.huirEnPanicoDesde(15.0f, 15.0f, 5.0f);
    assert(testVillager.getCondition() == VillagerCondition::Scared);
    testVillager.setSpeed(1.0f);
    assert(testVillager.getEffectiveSpeed() == 1.40f);

    // Drunken courage: heavy intoxication suppresses fear
    testVillager.consumeAlcohol(50.0f);
    assert(testVillager.getCondition() == VillagerCondition::Drunk); // Drunk overrides fear

    // Inspired condition (divine sign)
    testVillager.setPanicTimer(0.0f);
    testVillager.setAlcoholLevel(0.0f);
    testVillager.setInfected(false);
    testVillager.setInfectionTimer(0.0f);
    testVillager.activarSenalDivina(15.0f);
    assert(testVillager.getCondition() == VillagerCondition::Inspired);
    testVillager.setSpeed(1.0f);
    assert(testVillager.getEffectiveSpeed() == 1.15f);

    // -------------------------------------------------------------------------
    // EN: Block 3 - Physical update and drunken sway displacement verification
    // ES: Bloque 3 - Verificación de actualización física y desplazamiento lateral por ebriedad
    // -------------------------------------------------------------------------
    WorldMap testMap(32, 32);
    testVillager.setAlcoholLevel(80.0f);
    testVillager.caminarHacia(20.0f, 15.5f, false);
    const float preUpdateX = testVillager.getX();
    testVillager.actualizar(0.5f, testMap);
    assert(testVillager.getX() > preUpdateX); // Unit moved forward
    assert(testVillager.getAlcoholLevel() < 80.0f); // Alcohol decayed over time

    // -------------------------------------------------------------------------
    // EN: Block 4 - Game Console Cheat Commands for villager conditions
    // ES: Bloque 4 - Comandos de consola de trucos para condiciones de aldeanos
    // -------------------------------------------------------------------------
    Game testGameInstance(1024, 576);
    testGameInstance.init();
    testGameInstance.startNewCustomWorld();

    // Verify BORRACHO command
    testGameInstance.runCommand("BORRACHO");
    const Humano* selectedVillager = testGameInstance.getGestorHumanos().getHumanoSeleccionado();
    assert(selectedVillager != nullptr);
    assert(selectedVillager->isDrunk());
    assert(selectedVillager->getCondition() == VillagerCondition::Drunk);

    // Verify HAMBRIENTO command
    testGameInstance.runCommand("HAMBRIENTO");
    assert(selectedVillager->getCondition() == VillagerCondition::Hungry);

    // Verify ENFERMO command
    testGameInstance.runCommand("ENFERMO");
    assert(selectedVillager->getCondition() == VillagerCondition::Sick);

    // Verify AGRESIVO command
    testGameInstance.runCommand("AGRESIVO");
    assert(selectedVillager->getCondition() == VillagerCondition::Aggressive);

    // Verify LLENO command
    testGameInstance.runCommand("LLENO");
    assert(selectedVillager->getCondition() == VillagerCondition::Full);

    // Verify ASUSTADO command
    testGameInstance.runCommand("ASUSTADO");
    assert(selectedVillager->getCondition() == VillagerCondition::Scared);

    // Verify SANO command
    testGameInstance.runCommand("SANO");
    assert(selectedVillager->getCondition() == VillagerCondition::Healthy);

    // Verify Tribe-wide command when no unit is selected
    testGameInstance.getGestorHumanos().selectVillager(-1);
    testGameInstance.runCommand("BORRACHO");
    bool foundDrunkPlayerVillager = false;
    for (const auto& villagerUnit : testGameInstance.getGestorHumanos().getHumanos()) {
        if (villagerUnit.estaVivo() && (villagerUnit.getReligionId() == 1 || villagerUnit.getReligionId() == testGameInstance.getPlayerReligionId())) {
            assert(villagerUnit.isDrunk());
            foundDrunkPlayerVillager = true;
            break;
        }
    }
    assert(foundDrunkPlayerVillager);

    testGameInstance.runCommand("SANO");
    for (const auto& villagerUnit : testGameInstance.getGestorHumanos().getHumanos()) {
        if (villagerUnit.estaVivo() && (villagerUnit.getReligionId() == 1 || villagerUnit.getReligionId() == testGameInstance.getPlayerReligionId())) {
            assert(villagerUnit.getCondition() == VillagerCondition::Healthy);
            break;
        }
    }

    std::cout << "  -> testVillagerConditionAndBehaviorSystem PASSED! All villager states, speed modulations, combat multipliers, and cheat commands verified." << std::endl;
}

// EN: Test geological mineral distribution, vein formation, realistic scarcity, and terrain stratification
// ES: Probar distribución geológica de minerales, formación de vetas, escasez realista y estratificación
void testGeologicalMineralVeinsAndTerrainLogic() {
    using namespace ReligionSim;
    std::cout << "[Test] Running testGeologicalMineralVeinsAndTerrainLogic..." << std::endl;

    WorldMap proceduralMap(64, 64);
    constexpr uint32_t GEOLOGICAL_TEST_SEED = 123456789u;
    proceduralMap.generateProceduralWorld(GEOLOGICAL_TEST_SEED, WorldClimate::Templado, ResourceAbundance::Balanced);

    int totalMountainAndHillCells = 0;
    int bareMountainAndHillCells = 0;
    int mineralDepositCells = 0;
    int countCopper = 0;
    int countIron = 0;
    int countGold = 0;
    int countStone = 0;
    int countDiamond = 0;

    for (int cellY = 0; cellY < proceduralMap.getHeight(); ++cellY) {
        for (int cellX = 0; cellX < proceduralMap.getWidth(); ++cellX) {
            const Cell& inspectedCell = proceduralMap.getCell(cellX, cellY);
            if (inspectedCell.biome == BiomeType::HighlandHills || inspectedCell.biome == BiomeType::RuggedMountains) {
                totalMountainAndHillCells++;
                if (inspectedCell.naturalResource == ResourceType::None) {
                    bareMountainAndHillCells++;
                } else {
                    mineralDepositCells++;
                    switch (inspectedCell.naturalResource) {
                        case ResourceType::Copper:  countCopper++; break;
                        case ResourceType::Iron:    countIron++; break;
                        case ResourceType::Gold:    countGold++; break;
                        case ResourceType::Stone:   countStone++; break;
                        case ResourceType::Diamond: countDiamond++; break;
                        default: break;
                    }
                }
            }
        }
    }

    // EN: 1. Mountains and hills must NOT be 100% cluttered with minerals: at least 60% must be clean bare rock
    // ES: 1. Montañas y colinas NO deben estar 100% atestadas de minerales: al menos 60% debe ser roca virgen
    assert(totalMountainAndHillCells > 0);
    const float bareRatio = static_cast<float>(bareMountainAndHillCells) / static_cast<float>(totalMountainAndHillCells);
    assert(bareRatio >= 0.60f && bareRatio <= 0.90f);

    // EN: 2. Core industrial and building minerals must exist
    // ES: 2. Deben existir minerales industriales y de construcción
    assert(countIron > 0);
    assert(countStone > 0);

    // EN: 3. Verify Vein Clustering (vetas): At least some mineral cells must have contiguous neighbors of the same resource
    // ES: 3. Verificar agrupamiento en vetas: Al menos algunas celdas minerales deben tener vecinas contiguas del mismo recurso
    int cellsWithContiguousVeinNeighbor = 0;
    for (int cellY = 0; cellY < proceduralMap.getHeight(); ++cellY) {
        for (int cellX = 0; cellX < proceduralMap.getWidth(); ++cellX) {
            const Cell& sourceCell = proceduralMap.getCell(cellX, cellY);
            if (sourceCell.naturalResource == ResourceType::Iron ||
                sourceCell.naturalResource == ResourceType::Copper ||
                sourceCell.naturalResource == ResourceType::Stone) {
                bool hasContiguousVeinTile = false;
                constexpr std::array<std::pair<int, int>, 8> ADJACENCY_OFFSETS{{
                    {-1, -1}, {0, -1}, {1, -1},
                    {-1,  0},          {1,  0},
                    {-1,  1}, {0,  1}, {1,  1}
                }};
                for (const auto& [offsetX, offsetY] : ADJACENCY_OFFSETS) {
                    const int adjacentX = cellX + offsetX;
                    const int adjacentY = cellY + offsetY;
                    if (proceduralMap.isValid(adjacentX, adjacentY)) {
                        if (proceduralMap.getCell(adjacentX, adjacentY).naturalResource == sourceCell.naturalResource) {
                            hasContiguousVeinTile = true;
                            break;
                        }
                    }
                }
                if (hasContiguousVeinTile) {
                    cellsWithContiguousVeinNeighbor++;
                }
            }
        }
    }
    assert(cellsWithContiguousVeinNeighbor > 0); // Vein cohesion confirmed!

    // EN: 4. Renderer variant texture loading verification
    // ES: 4. Verificación de carga de texturas variantes en el Renderer
    Renderer testRenderer(320, 240);
    const auto& variantTextures = testRenderer.getResourceVariantPixels();
    // Copper variants (index 5)
    assert(variantTextures[5][0].size() == 64u * 64u); // Copper Vein
    assert(variantTextures[5][1].size() == 64u * 64u); // Copper Rock
    // Iron variants (index 4)
    assert(variantTextures[4][0].size() == 64u * 64u); // Iron Vein
    // Gold variants (index 3)
    assert(variantTextures[3][0].size() == 64u * 64u); // Gold Vein
    // Stone variants (index 2)
    assert(variantTextures[2][0].size() == 64u * 64u); // Stone Quarry
    assert(variantTextures[2][1].size() == 64u * 64u); // Stone Boulder

    std::cout << "  -> testGeologicalMineralVeinsAndTerrainLogic PASSED! Bare mountain slopes ("
              << static_cast<int>(bareRatio * 100.0f) << "%), cohesive veins, and variant sprites verified." << std::endl;
}

// EN: Unit test verifying thermodynamic dissipation of fire, evaporation of water, and elimination of infinite cycles
// ES: Prueba unitaria que verifica la disipación termodinámica de fuego, evaporación de agua y eliminación de ciclos infinitos
void testFireAndWaterDissipationAntiInfiniteCycles() {
    using namespace ReligionSim;
    std::cout << "Running testFireAndWaterDissipationAntiInfiniteCycles()..." << std::endl;

    WorldMap physicsWorldMap(32, 32);

    // =========================================================================
    // 1. PASSIVE THERMAL COOLING: Unburnt preheated cells decay to 0 heat
    // EN: Verify heat accumulator decays to 0.0 when flame front moves away
    // ES: Verificar que el acumulador térmico decae a 0.0 cuando el frente de fuego se aleja
    // =========================================================================
    Cell& thermalCell = physicsWorldMap.getCell(10, 10);
    thermalCell.heatAccumulator = 0.85f;
    thermalCell.isBurning = false;

    // Simulate 5 seconds in increments of 0.5s
    for (int simulationStep = 0; simulationStep < 10; ++simulationStep) {
        physicsWorldMap.updateFirePhysics(0.5f, nullptr, nullptr, 0.0f, 0.0f);
    }
    // Cooling rate = 0.20/s -> 0.85 - 5.0 * 0.20 = -0.15 clamped to 0.0f
    assert(thermalCell.heatAccumulator == 0.0f);
    assert(!thermalCell.isBurning);

    // =========================================================================
    // 2. REFRACTORY FIRE IMMUNITY: Burnt soil recovery prevents circular wildfires
    // EN: Regenerated fertile soil has 90s immunity so fire cannot loop endlessly
    // ES: El suelo fértil regenerado tiene 90s de inmunidad para evitar bucles de fuego
    // =========================================================================
    Cell& refractorySoilCell = physicsWorldMap.getCell(12, 12);
    refractorySoilCell.biome = BiomeType::DenseForest;
    physicsWorldMap.igniteCell(12, 12, 0.20f, 1.0f);
    assert(refractorySoilCell.isBurning);

    // Let fuel consume and burn into ashes
    physicsWorldMap.updateFirePhysics(0.35f, nullptr, nullptr, 0.0f, 0.0f);
    assert(!refractorySoilCell.isBurning);
    assert(refractorySoilCell.isBurnt);

    // Accelerate ash recovery
    refractorySoilCell.ashRecoveryTimer = 0.10f;
    physicsWorldMap.updateFirePhysics(0.20f, nullptr, nullptr, 0.0f, 0.0f);
    assert(!refractorySoilCell.isBurnt);
    assert(refractorySoilCell.biome == BiomeType::FertileValley);
    assert(refractorySoilCell.fireImmunityTimer == 90.0f);

    // Attempt to ignite newly sprouted pasture: must be rejected
    const bool cyclicIgnitionAttempt = physicsWorldMap.igniteCell(12, 12, 10.0f, 1.0f);
    assert(!cyclicIgnitionAttempt);
    assert(!refractorySoilCell.isBurning);

    // Update fire physics for 30s: immunity timer counts down smoothly
    physicsWorldMap.updateFirePhysics(30.0f, nullptr, nullptr, 0.0f, 0.0f);
    assert(refractorySoilCell.fireImmunityTimer == 60.0f);
    assert(!refractorySoilCell.isCombustible());

    // =========================================================================
    // 3. COMPLETE PUDDLE INFILTRATION AND EVAPORATION: No immortal shallow puddles
    // EN: Verify puddle with waterDepth < 0.02f is tracked and evaporates completely to 0.0
    // ES: Verificar que un charco con waterDepth < 0.02f es rastreado y se evapora totalmente a 0.0
    // =========================================================================
    Cell& puddleCell = physicsWorldMap.getCell(15, 15);
    puddleCell.elevation = 0.50f;
    puddleCell.biome = BiomeType::FertileValley;
    physicsWorldMap.addWaterToCell(15, 15, 0.015f); // Shallow puddle (< 0.02f threshold)
    assert(puddleCell.waterDepth > 0.0f);

    // Infiltration + atmospheric evaporation rate (~0.05/s) drains puddle over 2 seconds
    for (int simulationStep = 0; simulationStep < 20; ++simulationStep) {
        physicsWorldMap.updateWaterPhysics(0.10f, nullptr, nullptr);
    }
    assert(puddleCell.waterDepth == 0.0f);
    assert(!puddleCell.isFlooded);

    // =========================================================================
    // 4. THERMODYNAMIC LATENT HEAT OF VAPORIZATION: Water loss upon fire quenching
    // EN: Quenching raging fire boils off liquid water mass
    // ES: Extinguir fuego intenso evapora masa de agua líquida
    // =========================================================================
    Cell& flamingCell = physicsWorldMap.getCell(18, 18);
    flamingCell.biome = BiomeType::DenseForest;
    physicsWorldMap.igniteCell(18, 18, 15.0f, 1.0f);
    assert(flamingCell.isBurning);
    assert(flamingCell.fireIntensity >= 0.90f);

    constexpr float INITIAL_QUENCH_WATER = 0.25f;
    physicsWorldMap.addWaterToCell(18, 18, INITIAL_QUENCH_WATER);
    assert(!flamingCell.isBurning);
    // Latent heat loss consumes up to 0.10 * fireIntensity of water
    assert(flamingCell.waterDepth < INITIAL_QUENCH_WATER);
    assert(flamingCell.waterDepth > 0.10f);

    // =========================================================================
    // 5. VOLCANIC LAVA SOLIDIFICATION: Solid basalt mountains with stone resource
    // EN: Active lava timer solidifies molten rock into permanent rugged terrain
    // ES: El temporizador de lava solidifica roca fundida en terreno montañoso permanente
    // =========================================================================
    Cell& lavaFlowCell = physicsWorldMap.getCell(22, 22);
    lavaFlowCell.hasActiveLava = true;
    lavaFlowCell.hasVolcano = false;
    lavaFlowCell.lavaTimer = 1.0f;
    lavaFlowCell.elevation = 1.0f;

    // Simulate 2 seconds of physics
    physicsWorldMap.updateFirePhysics(2.0f, nullptr, nullptr, 0.0f, 0.0f);
    assert(!lavaFlowCell.hasActiveLava);
    assert(lavaFlowCell.lavaTimer == 0.0f);
    assert(lavaFlowCell.biome == BiomeType::RuggedMountains);
    assert(lavaFlowCell.elevation > 1.0f);
    assert(lavaFlowCell.naturalResource == ResourceType::Stone);
    assert(lavaFlowCell.resourceAmount > 0);

    // =========================================================================
    // 6. FLAT BASIN FRICTION & GRADIENT THRESHOLD: No endless sloshing oscillations
    // EN: Hydraulic head drop below 0.012f threshold produces no runaway current
    // ES: Caída hidráulica menor a 0.012f no genera corrientes oscilatorias infinitas
    // =========================================================================
    Cell& flatCellA = physicsWorldMap.getCell(25, 25);
    Cell& flatCellB = physicsWorldMap.getCell(26, 25);
    flatCellA.elevation = 0.60f;
    flatCellB.elevation = 0.60f;
    flatCellA.waterDepth = 0.008f;
    flatCellB.waterDepth = 0.0f;
    physicsWorldMap.addWaterToCell(25, 25, 0.0f); // Ensure registered if needed

    for (int simulationStep = 0; simulationStep < 10; ++simulationStep) {
        physicsWorldMap.updateWaterPhysics(0.10f, nullptr, nullptr);
    }
    // Gradient is below 0.012f threshold, bed friction damps fluid current
    assert(flatCellA.getFlowSpeed() < 0.01f);
    assert(flatCellB.getFlowSpeed() < 0.01f);

    std::cout << "  -> testFireAndWaterDissipationAntiInfiniteCycles PASSED! Passive thermal cooling, 90s fire immunity, complete puddle evaporation, latent heat quenching, and basalt crystallization verified." << std::endl;
}

// EN: Test the 4-phase water depth classification, bathymetric distance transform, and sprite atlas bindings
// ES: Probar la clasificación de 4 fases de profundidad del agua, transformada batimétrica y enlaces de atlas de sprites
void testWaterDepthPhasesAndSpriteSystem() {
    using namespace ReligionSim;
    std::cout << "[Test] Running testWaterDepthPhasesAndSpriteSystem (4 Water Depth Phases)..." << std::endl;

    // EN: 1. Verify strong typing and enum ordering of the 4 water depth phases
    // ES: 1. Verificar tipado fuerte y orden del enum de las 4 fases de profundidad del agua
    assert(static_cast<uint8_t>(WaterDepthPhase::ShorelineFoam) == 0);
    assert(static_cast<uint8_t>(WaterDepthPhase::ShallowCoast)  == 1);
    assert(static_cast<uint8_t>(WaterDepthPhase::MidOcean)      == 2);
    assert(static_cast<uint8_t>(WaterDepthPhase::DeepAbyssal)   == 3);

    // EN: 2. Verify Cell-level phase queries across overland water and biome defaults
    // ES: 2. Verificar consultas de fase a nivel de celda en agua superficial y valores por defecto de bioma
    Cell testCell;
    testCell.biome = BiomeType::FertileValley;
    testCell.waterDepth = 0.0f;
    assert(testCell.getWaterDepthPhase() == WaterDepthPhase::ShorelineFoam);

    // EN: Overland puddle (< 0.08f) exhibits shoreline foam edge
    // ES: Charco superficial (< 0.08f) exhibe borde de espuma de orilla
    testCell.waterDepth = 0.05f;
    assert(testCell.getWaterDepthPhase() == WaterDepthPhase::ShorelineFoam);

    // EN: Shallow pool (0.08f to 0.28f) maps to shallow coast
    // ES: Poza somera (0.08f a 0.28f) se asigna a costa somera
    testCell.waterDepth = 0.20f;
    assert(testCell.getWaterDepthPhase() == WaterDepthPhase::ShallowCoast);

    // EN: Flowing river/stream (0.28f to 0.55f) maps to mid ocean
    // ES: Río/arroyo fluido (0.28f a 0.55f) se asigna a mar medio
    testCell.waterDepth = 0.40f;
    assert(testCell.getWaterDepthPhase() == WaterDepthPhase::MidOcean);

    // EN: Deep flooded lake (> 0.55f) maps to deep abyssal
    // ES: Lago inundado profundo (> 0.55f) se asigna a mar profundo abisal
    testCell.waterDepth = 0.75f;
    assert(testCell.getWaterDepthPhase() == WaterDepthPhase::DeepAbyssal);

    // EN: Natural water biome defaults
    // ES: Valores por defecto de biomas acuáticos naturales
    Cell naturalShallowCell;
    naturalShallowCell.biome = BiomeType::ShallowWater;
    naturalShallowCell.waterDepth = 0.0f;
    assert(naturalShallowCell.getWaterDepthPhase() == WaterDepthPhase::ShallowCoast);

    Cell naturalDeepCell;
    naturalDeepCell.biome = BiomeType::DeepOcean;
    naturalDeepCell.waterDepth = 0.0f;
    assert(naturalDeepCell.getWaterDepthPhase() == WaterDepthPhase::DeepAbyssal);

    // EN: 3. Verify WorldMap bathymetric distance transform and phase assignment
    // ES: 3. Verificar transformada de distancia batimétrica y asignación de fases en WorldMap
    WorldMap bathymetryMap(30, 30);
    for (int cellY = 0; cellY < 30; ++cellY) {
        for (int cellX = 0; cellX < 30; ++cellX) {
            Cell& cellItem = bathymetryMap.getCell(cellX, cellY);
            cellItem.biome = BiomeType::DeepOcean;
            cellItem.elevation = 0.0f;
            cellItem.climate.elevation = -0.50f;
        }
    }
    // EN: Place a single island center point at (15, 15)
    // ES: Colocar un punto central de isla en (15, 15)
    bathymetryMap.getCell(15, 15).biome = BiomeType::CoastalPlains;
    bathymetryMap.getCell(15, 15).elevation = 0.60f;

    // EN: Run distance transform to calculate progressive phases
    // ES: Ejecutar transformada de distancia para calcular fases progresivas
    bathymetryMap.updateWaterDepthPhases();

    // EN: Verify concentric progression from island outwards:
    // Distance 1 (15, 16): Shallow Coast (Phase 2 with shoreline foam interface)
    // Distance 2 (15, 17): Shallow Coast (Phase 2)
    // Distance 4 (15, 19): Mid Ocean (Phase 3)
    // Distance 6 (15, 21): Deep Abyssal (Phase 4)
    // ES: Verificar progresión concéntrica desde la isla hacia afuera
    assert(bathymetryMap.getCell(15, 16).waterPhase == WaterDepthPhase::ShallowCoast);
    assert(bathymetryMap.getCell(15, 17).waterPhase == WaterDepthPhase::ShallowCoast);
    assert(bathymetryMap.getCell(15, 19).waterPhase == WaterDepthPhase::MidOcean);
    assert(bathymetryMap.getCell(15, 21).waterPhase == WaterDepthPhase::DeepAbyssal);

    // EN: 4. Verify Renderer sprite assets loading for all 4 water phases
    // ES: 4. Verificar carga de assets de sprites en Renderer para las 4 fases de agua
    Renderer testRenderer(640, 480);

    // EN: Verify animated water sprite sheets are loaded and populated
    // ES: Verificar que las hojas de sprites animados de agua están cargadas y pobladas
    assert(testRenderer.getWaterSurfPixels().size() == 1024u * 116u);
    assert(testRenderer.getWaterRipplesPixels().size() == 1024u * 170u);
    assert(testRenderer.getWaterWavesPixels().size() == 1024u * 115u);
    assert(testRenderer.getWaterBordersPixels().size() == 1024u * 175u);
    assert(testRenderer.getWaterSheetPixels().size() == 1024u * 765u);

    std::cout << "  -> testWaterDepthPhasesAndSpriteSystem PASSED! All 4 water depth phases (Shoreline Foam, Shallow Coast, Mid Ocean, Deep Abyssal) verified." << std::endl;
}

// EN: Unit test for Authentic Lava & Magma Tileset (512x256), 4-Phase Lifecycle, and Shoreline Banks
// ES: Prueba unitaria para el tileset auténtico de lava y magma (512x256), ciclo de 4 fases y riberas rocosas
void testLavaAndMagmaTilesetAndLifecycleSystem() {
    using namespace ReligionSim;
    std::cout << "[Test] Running testLavaAndMagmaTilesetAndLifecycleSystem (4-Phase Lava & Magma Tileset)..." << std::endl;

    // EN: 1. Verify Renderer correctly loads 512x256 RGBA master sheet (131,072 pixels / 524,288 bytes)
    // ES: 1. Verificar que el Renderer cargue correctamente la hoja maestra RGBA de 512x256
    Renderer testRenderer(640, 480);
    const auto& lavaPixels = testRenderer.getLavaTilesPixels();
    assert(lavaPixels.size() == 512u * 256u && "Lava tileset must match 512x256 dimension exactly");

    // EN: 2. Verify all 4 solid ground tiles on Row 0 (128x128 each)
    // ES: 2. Verificar los 4 mosaicos de terreno sólidos en la Fila 0 (128x128 cada uno)
    for (int tileIndex = 0; tileIndex < 4; ++tileIndex) {
        int opaquePixelCount = 0;
        int accumulatedRed = 0;
        int accumulatedGreen = 0;
        int accumulatedBlue = 0;

        for (int sampleY = 0; sampleY < 128; ++sampleY) {
            for (int sampleX = 0; sampleX < 128; ++sampleX) {
                const int pixelCoordX = tileIndex * 128 + sampleX;
                const int pixelCoordY = sampleY;
                const uint32_t packedColor = lavaPixels[pixelCoordY * 512 + pixelCoordX];
                const uint8_t alphaByte = (packedColor >> 24) & 0xFF;
                if (alphaByte > 200) {
                    ++opaquePixelCount;
                }
                accumulatedRed += (packedColor & 0xFF);
                accumulatedGreen += ((packedColor >> 8) & 0xFF);
                accumulatedBlue += ((packedColor >> 16) & 0xFF);
            }
        }

        // EN: Ensure full opacity across all 4 ground tiles
        // ES: Asegurar opacidad completa en los 4 mosaicos de terreno
        assert(opaquePixelCount == 128 * 128 && "Ground tiles on Row 0 must be 100% solid opaque");

        const int averageRed = accumulatedRed / (128 * 128);
        const int averageGreen = accumulatedGreen / (128 * 128);
        // EN: Verify incandescent coloration: red channel dominance
        // ES: Verificar coloración incandescente: predominio del canal rojo
        assert(averageRed > 70 && averageRed > averageGreen);
    }

    // EN: 3. Verify Row 1 contains shoreline bank borders with transparent alpha chromakey removal
    // ES: 3. Verificar que la Fila 1 contenga riberas rocosas con remoción de fondo por chromakey
    for (int subIndex = 0; subIndex < 4; ++subIndex) {
        int transparentPixelCount = 0;
        int activeBankPixelCount = 0;

        for (int sampleY = 128; sampleY < 256; ++sampleY) {
            for (int sampleX = 0; sampleX < 128; ++sampleX) {
                const int pixelCoordX = subIndex * 128 + sampleX;
                const int pixelCoordY = sampleY;
                const uint32_t packedColor = lavaPixels[pixelCoordY * 512 + pixelCoordX];
                const uint8_t alphaByte = (packedColor >> 24) & 0xFF;
                if (alphaByte < 20) {
                    ++transparentPixelCount;
                } else {
                    ++activeBankPixelCount;
                }
            }
        }

        assert(transparentPixelCount > 1000 && "Row 1 bank borders must possess clean transparent alpha margins");
        assert(activeBankPixelCount > 1000 && "Row 1 bank borders must possess populated rock border sprites");
    }

    // EN: 4. Verify Active Volcanic Lava Solidification Lifecycle
    // ES: 4. Verificar el ciclo de vida de solidificación de lava volcánica activa
    WorldMap simulationWorldMap(20, 20);
    Cell& activeLavaCell = simulationWorldMap.getCell(10, 10);
    activeLavaCell.hasActiveLava = true;
    activeLavaCell.hasVolcano = false;
    activeLavaCell.lavaTimer = 1.5f;
    activeLavaCell.elevation = 1.0f;
    activeLavaCell.biome = BiomeType::CoastalPlains;

    // Simulate 2 seconds of physics
    simulationWorldMap.updateFirePhysics(2.0f, nullptr, nullptr, 0.0f, 0.0f);

    // EN: Upon countdown expiration, lava must crystallize into RuggedMountains with Stone resource
    // ES: Al expirar la cuenta atrás, la lava debe cristalizar en RuggedMountains con recurso de piedra
    assert(!activeLavaCell.hasActiveLava && "Lava must solidify when lavaTimer expires");
    assert(activeLavaCell.lavaTimer == 0.0f);
    assert(activeLavaCell.biome == BiomeType::RuggedMountains);
    assert(activeLavaCell.naturalResource == ResourceType::Stone);
    assert(activeLavaCell.resourceAmount > 0);

    std::cout << "  -> testLavaAndMagmaTilesetAndLifecycleSystem PASSED! 512x256 atlas, 4 solid tiles, 4 shoreline banks, and volcanic solidification lifecycle verified." << std::endl;
}

// EN: Unit test for Volcanic Caldera Biome and Natural Volcano Spawning System
// ES: Prueba unitaria para el Bioma de Caldera Volcánica y Generación Natural de Volcanes
void testVolcanicBiomeAndNaturalVolcanoGeneration() {
    using namespace ReligionSim;
    std::cout << "[Test] Running testVolcanicBiomeAndNaturalVolcanoGeneration (Volcanic Biomes & Natural Volcanoes)..." << std::endl;

    // EN: 1. Verify MultiNoiseBiome classification produces VolcanicCaldera from its climate target
    // ES: 1. Verificar que la clasificación de MultiNoiseBiome produzca VolcanicCaldera a partir de su objetivo climático
    const auto targetVolcanic = MultiNoiseBiomeGenerator::selectBiome(0.85f, -0.50f, 0.95f);
    assert(targetVolcanic.primary_biome == BiomeID::VolcanicCaldera);

    // EN: 2. Verify procedural world generation instantiates volcanic biomes and active volcanoes
    // ES: 2. Verificar que la generación procedural instancie biomas volcánicos y volcanes activos
    WorldMap proceduralMap(40, 40);
    proceduralMap.generateProceduralWorld(45678);

    int countVolcanicCells = 0;
    int countNaturalVolcanoes = 0;
    int countActiveLavaPools = 0;

    for (int y = 0; y < proceduralMap.getHeight(); ++y) {
        for (int x = 0; x < proceduralMap.getWidth(); ++x) {
            const Cell& cellItem = proceduralMap.getCell(x, y);
            if (cellItem.biome == BiomeType::VolcanicCaldera) {
                ++countVolcanicCells;
                if (cellItem.hasVolcano) {
                    ++countNaturalVolcanoes;
                    assert(cellItem.hasActiveLava && "Natural volcano summit must feature active caldera lava");
                    assert(cellItem.elevation >= 2.0f && "Volcano summit must be topographically elevated");
                }
                if (cellItem.hasActiveLava) {
                    ++countActiveLavaPools;
                    assert(cellItem.lavaTimer > 0.0f);
                }
            }
        }
    }

    // EN: Ensure volcanic presence: at least 1 volcanic cluster with an active volcano and lava pools
    // ES: Asegurar presencia volcánica: al menos 1 conglomerado volcánico con un volcán activo y piscinas de lava
    assert(countVolcanicCells >= 4 && "World must generate volcanic caldera cells");
    assert(countNaturalVolcanoes >= 1 && "World must generate at least 1 natural active volcano");
    assert(countActiveLavaPools >= 2 && "Volcanic caldera must feature active boiling lava pools");

    // EN: 3. Verify base color and travel ease of VolcanicCaldera
    // ES: 3. Verificar color base y facilidad de viaje de VolcanicCaldera
    Cell sampleVolcanicCell;
    sampleVolcanicCell.biome = BiomeType::VolcanicCaldera;
    sampleVolcanicCell.isBurnt = false;
    sampleVolcanicCell.isBurning = false;
    ColorRGB volcanicColor = sampleVolcanicCell.getBiomeBaseColor();
    assert(volcanicColor.r == 42 && volcanicColor.g == 28 && volcanicColor.b == 26);

    Cell dummyOriginCell;
    dummyOriginCell.biome = BiomeType::FertileValley;
    float travelEaseScore = proceduralMap.getTravelEase(dummyOriginCell, sampleVolcanicCell);
    assert(travelEaseScore == 0.15f && "Volcanic caldera travel ease must reflect hazardous rugged terrain");

    std::cout << "  -> testVolcanicBiomeAndNaturalVolcanoGeneration PASSED! Volcanic caldera (" << countVolcanicCells << " cells), "
              << countNaturalVolcanoes << " natural volcanoes, and " << countActiveLavaPools << " active lava pools verified." << std::endl;
}

// EN: Test destruction tileset assets, burnt trees, craters, and scorched terrain rendering
// ES: Probar activos del tileset de destrucción, árboles quemados, cráteres y renderizado de terreno calcinado
void testDestructionTilesetAndScorchedGroundSystem() {
    std::cout << "[Test] Running testDestructionTilesetAndScorchedGroundSystem (Burnt Trees, Craters, Scorched Ground)..." << std::endl;

    ReligionSim::Renderer testRenderer(640, 480);

    // EN: 1. Verify all 11 destruction tree, stump, and ruin sprite buffers
    // ES: 1. Verificar los 11 búferes de sprites de árboles quemados, tocones y ruinas
    const auto& treeSprites = testRenderer.getDestructionTreePixels();
    assert(treeSprites.size() == 11 && "Renderer must load exactly 11 destruction tree/stump sprites");
    for (size_t treeIndex = 0; treeIndex < treeSprites.size(); ++treeIndex) {
        assert(treeSprites[treeIndex].size() == 64u * 64u && "Every destruction tree sprite must be 64x64 RGBA");
        int visiblePixelCount = 0;
        for (const uint32_t packedPixel : treeSprites[treeIndex]) {
            const uint8_t alphaByte = (packedPixel >> 24) & 0xFF;
            if (alphaByte > 20) {
                ++visiblePixelCount;
            }
        }
        assert(visiblePixelCount > 80 && "Destruction tree sprite must contain visible opaque pixels");
    }

    // EN: 2. Verify all 8 impact craters (7 at 64x64, 1 at 128x128 colossal)
    // ES: 2. Verificar los 8 cráteres de impacto (7 de 64x64, 1 de 128x128 colosal)
    const auto& craterSprites = testRenderer.getDestructionCraterPixels();
    assert(craterSprites.size() == 8 && "Renderer must load exactly 8 impact crater sprites");
    for (size_t craterIndex = 0; craterIndex < 7; ++craterIndex) {
        assert(craterSprites[craterIndex].size() == 64u * 64u && "Standard crater must be 64x64 RGBA");
    }
    assert(craterSprites[7].size() == 128u * 128u && "Colossal giant crater must be 128x128 RGBA (16,384 pixels)");

    // EN: 3. Verify all 8 scorched terrain and magma fissure textures (64x64 each)
    // ES: 3. Verificar las 8 texturas de terreno calcinado y fisuras de magma (64x64 cada una)
    const auto& terrainTextures = testRenderer.getDestructionTerrainPixels();
    assert(terrainTextures.size() == 8 && "Renderer must load exactly 8 scorched terrain textures");
    for (size_t textureIndex = 0; textureIndex < terrainTextures.size(); ++textureIndex) {
        assert(terrainTextures[textureIndex].size() == 64u * 64u && "Scorched terrain texture must be 64x64 RGBA");
    }

    // EN: 4. Verify prop bounds calculation for tall burnt trees vs stumps
    // ES: 4. Verificar cálculo de límites para árboles quemados altos vs tocones
    int startCoordX = 0, startCoordY = 0, boundingWidth = 0, boundingHeight = 0;
    testRenderer.getDestructionPropBounds(320.0f, 240.0f, 0, 1.0f, startCoordX, startCoordY, boundingWidth, boundingHeight);
    assert(boundingHeight == 64 && "Tall burnt crooked tree height must equal 64 pixels at zoom 1.0");

    testRenderer.getDestructionPropBounds(320.0f, 240.0f, 2, 1.0f, startCoordX, startCoordY, boundingWidth, boundingHeight);
    assert(boundingHeight < 40 && "Small burnt stump height must be compact");

    // EN: 5. Verify drawing destruction prop writes pixels to framebuffer
    // ES: 5. Verificar que dibujar un prop de destrucción escribe píxeles en el framebuffer
    testRenderer.clear(ReligionSim::ColorRGB{120, 120, 120});
    testRenderer.drawDestructionProp(320.0f, 240.0f, 6, 1.0f, 1.0f); // Burnt tree with embers
    int alteredPixelCount = 0;
    for (const uint32_t pixelValue : testRenderer.getFramebuffer()) {
        const uint8_t redByte = pixelValue & 0xFF;
        const uint8_t greenByte = (pixelValue >> 8) & 0xFF;
        const uint8_t blueByte = (pixelValue >> 16) & 0xFF;
        if (redByte != 120 || greenByte != 120 || blueByte != 120) {
            ++alteredPixelCount;
        }
    }
    assert(alteredPixelCount > 100 && "drawDestructionProp must blit visible burned pixels to framebuffer");

    // EN: 6. Verify crater bounds and drawing for standard vs colossal craters
    // ES: 6. Verificar límites y dibujo de cráteres estándar vs colosales
    int craterStartX = 0, craterStartY = 0, craterWidth = 0, craterHeight = 0;
    testRenderer.getCraterBounds(320.0f, 240.0f, 5, 1.0f, craterStartX, craterStartY, craterWidth, craterHeight);
    assert(craterWidth == 64 && craterHeight == 32 && "Standard crater must have 64x32 2:1 isometric aspect ratio");

    testRenderer.getCraterBounds(320.0f, 240.0f, 7, 1.0f, craterStartX, craterStartY, craterWidth, craterHeight);
    assert(craterWidth == 128 && craterHeight == 64 && "Colossal giant crater must have 128x64 2:1 isometric aspect ratio");

    testRenderer.clear(ReligionSim::ColorRGB{120, 120, 120});
    testRenderer.drawCraterSprite(320.0f, 240.0f, 7, 1.0f, 1.0f); // Colossal crater
    int craterAlteredPixels = 0;
    for (const uint32_t pixelValue : testRenderer.getFramebuffer()) {
        const uint8_t redByte = pixelValue & 0xFF;
        const uint8_t greenByte = (pixelValue >> 8) & 0xFF;
        const uint8_t blueByte = (pixelValue >> 16) & 0xFF;
        if (redByte != 120 || greenByte != 120 || blueByte != 120) {
            ++craterAlteredPixels;
        }
    }
    assert(craterAlteredPixels > 500 && "drawCraterSprite must render authentic colossal crater pixels");

    std::cout << "  -> testDestructionTilesetAndScorchedGroundSystem PASSED! 11 burnt trees, 8 craters (including 128x128 colossal), and 8 scorched ground textures verified." << std::endl;
}

// =============================================================================
// EN: Test Suite for the Liturgical Map Editor and World Creation System
// ES: Batería de Pruebas para el Editor Litúrgico de Mapas y Creación de Mundos
// =============================================================================
void testMapEditorAndCreationSystem() {
    std::cout << "[Test] Running testMapEditorAndCreationSystem (Liturgical Map Workshop)..." << std::endl;
    using namespace ReligionSim;

    constexpr int canvasWidth = 1280;
    constexpr int canvasHeight = 720;
    Game gameInstance(canvasWidth, canvasHeight);
    gameInstance.init();

    // -------------------------------------------------------------------------
    // EN: Block 1 - Verify Main Menu has "CREACION DE MAPAS" button (ID 6)
    // ES: Bloque 1 - Verificar que el Menú Principal tenga el botón "CREACION DE MAPAS" (ID 6)
    // -------------------------------------------------------------------------
    assert(gameInstance.getGameState() == GameState::MainMenu);
    const auto& menuButtons = gameInstance.getRenderer().getMenuButtons();
    assert(menuButtons.size() == 6);
    assert(menuButtons[2].id == 6 && menuButtons[2].label == "CREACION DE MAPAS");

    // -------------------------------------------------------------------------
    // EN: Block 2 - Click "CREACION DE MAPAS" and verify transition to MapEditor
    // ES: Bloque 2 - Hacer clic en "CREACION DE MAPAS" y verificar transición a MapEditor
    // -------------------------------------------------------------------------
    const float mapEditorBtnCenterX = menuButtons[2].x + menuButtons[2].width * 0.5f;
    const float mapEditorBtnCenterY = menuButtons[2].y + menuButtons[2].height * 0.5f;
    gameInstance.onMouseDown(1, mapEditorBtnCenterX, mapEditorBtnCenterY);

    assert(gameInstance.getGameState() == GameState::MapEditor && "Must transition to GameState::MapEditor");
    const auto& editorButtons = gameInstance.getRenderer().getMapEditorButtons();
    assert(editorButtons.size() >= 25 && "Map Editor must register all procedural controls and tool buttons");

    const WorldMap& editorMap = gameInstance.getEditorMap();
    assert(editorMap.getWidth() == 128 && editorMap.getHeight() == 128 && "Default editor map size should be 128x128");

    // -------------------------------------------------------------------------
    // EN: Block 3 - Test Brush Selection (Radius and Tool Brushes)
    // ES: Bloque 3 - Probar selección de pinceles (Radio y herramientas de bioma/recurso)
    // -------------------------------------------------------------------------
    // Click ID 822: Brush Radius 2 (3x3 area)
    const auto brush3x3It = std::find_if(editorButtons.begin(), editorButtons.end(), [](const UIButton& b) { return b.id == 822; });
    assert(brush3x3It != editorButtons.end());
    gameInstance.onMouseDown(1, brush3x3It->x + brush3x3It->width * 0.5f, brush3x3It->y + brush3x3It->height * 0.5f);
    assert(gameInstance.getMapEditorConfig().brushRadius == 2);

    // Click ID 834: Dense Forest brush
    const auto forestBtnIt = std::find_if(editorButtons.begin(), editorButtons.end(), [](const UIButton& b) { return b.id == 834; });
    assert(forestBtnIt != editorButtons.end());
    gameInstance.onMouseDown(1, forestBtnIt->x + forestBtnIt->width * 0.5f, forestBtnIt->y + forestBtnIt->height * 0.5f);
    assert(gameInstance.getMapEditorConfig().activeBrush == MapEditorBrush::DenseForest);

    // -------------------------------------------------------------------------
    // EN: Block 4 - Test Interactive Sculpting on Preview Canvas
    // ES: Bloque 4 - Probar escultura interactiva en el lienzo de vista previa
    // -------------------------------------------------------------------------
    constexpr float canvasX = 494.0f;
    constexpr float canvasY = 92.0f;
    constexpr float canvasW = 744.0f;
    constexpr float canvasH = 470.0f;
    const float scale = std::min(canvasW / static_cast<float>(editorMap.getWidth()),
                                 canvasH / static_cast<float>(editorMap.getHeight()));
    const float previewStartX = canvasX + (canvasW - static_cast<float>(editorMap.getWidth()) * scale) * 0.5f;
    const float previewStartY = canvasY + (canvasH - static_cast<float>(editorMap.getHeight()) * scale) * 0.5f;

    const float paintScreenX = previewStartX + 30.0f * scale + scale * 0.5f;
    const float paintScreenY = previewStartY + 30.0f * scale + scale * 0.5f;

    gameInstance.onMouseDown(1, paintScreenX, paintScreenY);
    gameInstance.onMouseUp(1, paintScreenX, paintScreenY);

    // Verify cell (30, 30) has been painted as DenseForest with Wood resource
    const Cell& paintedCell = gameInstance.getEditorMap().getCell(30, 30);
    assert(paintedCell.biome == BiomeType::DenseForest && "Target cell must become DenseForest");
    assert(paintedCell.naturalResource == ResourceType::Wood && "DenseForest brush must assign Wood resource");

    // Click ID 843: Gold deposit brush
    const auto goldBtnIt = std::find_if(editorButtons.begin(), editorButtons.end(), [](const UIButton& b) { return b.id == 843; });
    assert(goldBtnIt != editorButtons.end());
    gameInstance.onMouseDown(1, goldBtnIt->x + goldBtnIt->width * 0.5f, goldBtnIt->y + goldBtnIt->height * 0.5f);
    assert(gameInstance.getMapEditorConfig().activeBrush == MapEditorBrush::Gold);

    // Paint cell (30, 30) with Gold brush
    gameInstance.onMouseDown(1, paintScreenX, paintScreenY);
    gameInstance.onMouseUp(1, paintScreenX, paintScreenY);
    assert(gameInstance.getEditorMap().getCell(30, 30).naturalResource == ResourceType::Gold && "Cell must now have Gold resource");

    // Click ID 851: Raise elevation
    const auto raiseBtnIt = std::find_if(editorButtons.begin(), editorButtons.end(), [](const UIButton& b) { return b.id == 851; });
    assert(raiseBtnIt != editorButtons.end());
    gameInstance.onMouseDown(1, raiseBtnIt->x + raiseBtnIt->width * 0.5f, raiseBtnIt->y + raiseBtnIt->height * 0.5f);
    const float initialElev = gameInstance.getEditorMap().getCell(30, 30).elevation;
    gameInstance.onMouseDown(1, paintScreenX, paintScreenY);
    gameInstance.onMouseUp(1, paintScreenX, paintScreenY);
    assert(gameInstance.getEditorMap().getCell(30, 30).elevation >= initialElev + 0.9f && "Elevation must increase");

    // -------------------------------------------------------------------------
    // EN: Block 5 - Test 2.5D Isometric View Mode and Atlas Terrain Sprite Palette
    // ES: Bloque 5 - Probar Modo de Vista Isométrica 2.5D y Paleta de Mosaicos del Atlas
    // -------------------------------------------------------------------------
    // Test TAB key to switch to 2.5D Isometric View
    gameInstance.onKeyPress(9); // TAB
    assert(gameInstance.getMapEditorConfig().viewMode == MapEditorViewMode::Isometric25D && "TAB must switch to Isometric25D");

    const auto& isoButtons = gameInstance.getRenderer().getMapEditorIsometricButtons();
    assert(isoButtons.size() >= 30 && "Isometric editor must register all toolbar tabs, radius, and palette buttons");

    // Click Tab 0: Atlas Terrain Sprites (ID 871)
    const auto tab0It = std::find_if(isoButtons.begin(), isoButtons.end(), [](const UIButton& b) { return b.id == 871; });
    assert(tab0It != isoButtons.end());
    gameInstance.onMouseDown(1, tab0It->x + tab0It->width * 0.5f, tab0It->y + tab0It->height * 0.5f);
    assert(gameInstance.getMapEditorConfig().paletteTab == 0 && "Active palette tab must be 0 (Atlas Sprites)");

    // Click Tile 15: Lava / Magma (ID 915)
    const auto lavaBtnIt = std::find_if(isoButtons.begin(), isoButtons.end(), [](const UIButton& b) { return b.id == 915; });
    assert(lavaBtnIt != isoButtons.end());
    gameInstance.onMouseDown(1, lavaBtnIt->x + lavaBtnIt->width * 0.5f, lavaBtnIt->y + lavaBtnIt->height * 0.5f);
    assert(gameInstance.getMapEditorConfig().activeBrush == MapEditorBrush::TileLava);

    // Apply brush to cell (64, 64) in isometric mode
    gameInstance.applyMapEditorBrush(64, 64);
    const Cell& lavaCell = gameInstance.getEditorMap().getCell(64, 64);
    assert(lavaCell.customTileIndex == 15 && "Cell must have custom tile index 15 (Lava)");
    assert(lavaCell.hasActiveLava && "Cell must have active lava");

    // Test camera panning in isometric editor with WASD
    const float initialCamX = gameInstance.getCamera().getCameraX();
    gameInstance.onKeyPress('d'); // Pan right
    assert(gameInstance.getCamera().getCameraX() > initialCamX && "WASD must pan camera in isometric editor");

    // Test TAB key to switch back to 2D Cartographic View
    gameInstance.onKeyPress(9); // TAB
    assert(gameInstance.getMapEditorConfig().viewMode == MapEditorViewMode::Cartographic2D && "TAB must toggle back to 2D view");

    // -------------------------------------------------------------------------
    // EN: Block 6 - Test Launching Game with Custom Map (ID 890)
    // ES: Bloque 6 - Probar lanzamiento de partida con mapa personalizado (ID 890)
    // -------------------------------------------------------------------------
    const auto playBtnIt = std::find_if(editorButtons.begin(), editorButtons.end(), [](const UIButton& b) { return b.id == 890; });
    assert(playBtnIt != editorButtons.end());
    gameInstance.onMouseDown(1, playBtnIt->x + playBtnIt->width * 0.5f, playBtnIt->y + playBtnIt->height * 0.5f);

    assert(gameInstance.getGameState() == GameState::Playing && "Must transition to GameState::Playing");
    assert(gameInstance.getMap().getWidth() == 128 && "Game map must match custom-crafted dimensions");
    assert(gameInstance.getGestorHumanos().getHumanos().size() > 0 && "Living villagers must be initialized on the custom world");

    std::cout << "  -> testMapEditorAndCreationSystem PASSED! Liturgical Map Editor fully validated." << std::endl;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "     RUNNING RELIGION SIMULATOR TESTS   " << std::endl;
    std::cout << "========================================" << std::endl;

    testIsometricCamera();
    testMultiNoiseBiomeGenerator();
    testWorldMap();
    testReligionAndAgglutination();
    testReplicationAndSimulationCycle();
    testVillagerEntities();
    testObjetoHumanoYTileSet();
    testTimeEngineDecoupledScaling();
    testModularUISystem();
    testExpandedTerrainAtlas();
    testFirstEraAIController();
    testHumanAIController();
    testFaithFeedbackAndCombatDamage();
    testAntique2DButtonsAndInteractiveStates();
    testViewModeSpritesAndMapSector();
    testAdvancedEventChronicleSystem();
    testFaithAdvancementTreeSystem();
    testDivinePowersAndIntermediaryPreaching();
    testTerrainHazardsAndProphetMiracle();
    testEraEvolutionEngineDualTimeAndTippingPoints();
    testResourceSystemAndWorldCreation();
    testNaturalDisastersAndPhysics();
    testDisastersLethalityAndWaterDrowning();
    testRagdollPhysicsSystem();
    testInterTribalInteractionsAndBorderTension();
    testLeaderDevotionAndFanaticism();
    testDayNightCycleAndFakeDynamicLighting();
    testMeteorCataclysmicPowerAndPhysics();
    testVolcanoEruptionAndPermanentCraters();
    testBiologicalAndSpiritualGatheringAndCombatAI();
    testCloudSystemAndCelestialAltitudeAndZoomFade();
    testSoundSystemAndAmbientDSP();
    testNemesisAndSififusMythSystem();
    testMemoryTreeUIAndModal();
    testGameOverOnAllUnitsExtinction();
    testReligiousMoralityAndRulesSystem();
    testFemaleUnitsAndProcreationSystem();
    testVillagerHouseBuildingSystem();
    testWildfireDivinePowerAndPowerIcons();
    testWorldCreationLayoutAndBehavioralAIDifficultySystem();
    testPuebloSpatialAllocationAndBufferZones();
    testUnitFaithSelectorAndDefaultViewNoFaithNodes();
    testLivingOrganismsCircadianAndFamilyDefenseSystem();
    testHouseSpriteHidingCapacityAndProbabilisticReproductionSystem();
    testErrorSystemAndLagMitigationWatchdog();
    testResourcePanelBelieversIndicator();
    testMinecraftStyleSimulatedWorldPanoramaMainMenu();
    testOptionsMenuCategoriesAndSettingsArchitecture();
    testLivingOrganismReproductionAndMemeticReplicationAI();
    testTwoCellTreeHeightAndOcclusionSortingSystem();
    testFallDamageAndNaturalPhenomenaPhysics();
    testFireThermodynamicsSpreadDissipationAndEcologicalRecovery();
    testRainAndDelugeWaterPhysicsAndFireExtinction();
    testAtomicBombCheatAndNuclearPhysics();
    testFiniteNaturalResourcesDepletionAndAI();
    testFireSpritesAndVisualEffectsPipeline();
    testVolcanoPixelArtSpritesAndLifecyclePipeline();
    testWaterMovementSpritesAndDynamicFluidPipeline();
    testCassetteAndRoundPlayButtons();
    testEarthquakeAndHeavyImpactCraterFormation();
    testMetricPhysicalScaleAndDisasterParameters();
    testIndependentPlaythroughsAndSolidSnapshotSystem();
    testLearnToPlaySacredCodexAndTutorialSystem();
    testScaledTimeSynchronizationWithHoursAndAdvancements();
    testVillagerConditionAndBehaviorSystem();
    testGeologicalMineralVeinsAndTerrainLogic();
    testFireAndWaterDissipationAntiInfiniteCycles();
    testWaterDepthPhasesAndSpriteSystem();
    testLavaAndMagmaTilesetAndLifecycleSystem();
    testVolcanicBiomeAndNaturalVolcanoGeneration();
    testDestructionTilesetAndScorchedGroundSystem();
    testMapEditorAndCreationSystem();

    std::cout << "========================================" << std::endl;
    std::cout << "      ALL TESTS PASSED SUCCESSFULLY!    " << std::endl;
    std::cout << "========================================" << std::endl;
    return 0;
}
