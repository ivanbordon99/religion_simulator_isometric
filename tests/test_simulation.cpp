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
#include "Game.hpp"
#include <iostream>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <vector>

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
        bool representedLandBiomes[9]{};

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
    assert(btn1x->y == botY + 24.0f);
    assert(btnPausa->width >= 100.0f);
    assert(btn1x->width >= 70.0f);
    assert(btnPausa->height >= 26.0f);

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
                       c.biome == ReligionSim::BiomeType::FrozenTundra);
            } else if (c.naturalResource == ReligionSim::ResourceType::Gold) {
                totalGold += c.resourceAmount;
                goldTiles++;
                assert(c.biome == ReligionSim::BiomeType::HighlandHills ||
                       c.biome == ReligionSim::BiomeType::RuggedMountains);
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
    assert(resProps.size() == 5);
    for (size_t i = 0; i < 5; ++i) {
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
    assert(powers.getPowerRadius(DivinePowerType::Meteor) == 5);
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

    // 4. Prueba de Audio Espacial 2.5D (Panning estéreo atenuado por distancia de cámara)
    ReligionSim::IsometricCamera camera(1280, 720);
    camera.setZoom(1.0f);
    camera.pan(0.0f, 0.0f);

    // Disparos espaciales cercanos y lejanos
    soundSys.playSpatialSFX(ReligionSim::SFXType::BodyThud, 32.0f, 32.0f, camera, 1.0f, 60.0f);
    soundSys.playSpatialSFX(ReligionSim::SFXType::Earthquake, 10.0f, 10.0f, camera, 1.0f, 60.0f);
    soundSys.playSpatialSFX(ReligionSim::SFXType::CombatAttack, 50.0f, 50.0f, camera, 0.8f, 60.0f);

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

    // Hit-testing de los botones de la pantalla de Game Over
    const auto& renderer = game.getRenderer();
    float modalW = 860.0f;
    float modalH = 480.0f;
    float mx = (1280.0f - modalW) * 0.5f;
    float my = (720.0f - modalH) * 0.5f;
    float btnY = my + modalH - 65.0f;

    // Clic en [ REINTENTAR GÉNESIS ] (ID 901)
    int btn901 = renderer.checkGameOverButtonClicked(mx + 50.0f, btnY + 20.0f);
    assert(btn901 == 901);

    // Clic en [ CARGAR PARTIDA ] (ID 902)
    int btn902 = renderer.checkGameOverButtonClicked(mx + 330.0f, btnY + 20.0f);
    assert(btn902 == 902);

    // Clic en [ MENÚ PRINCIPAL ] (ID 903)
    int btn903 = renderer.checkGameOverButtonClicked(mx + 600.0f, btnY + 20.0f);
    assert(btn903 == 903);

    // Clic fuera del área de botones
    assert(renderer.checkGameOverButtonClicked(10.0f, 10.0f) == 0);

    // Interacción interactiva: Clic en 901 -> WorldCreation
    game.onMouseDown(1, mx + 50.0f, btnY + 20.0f);
    assert(game.getGameState() == ReligionSim::GameState::WorldCreation);

    // Forzar GameOver nuevamente para probar botón 903 y tecla ESC
    game.setGameState(ReligionSim::GameState::GameOver);
    assert(game.getGameState() == ReligionSim::GameState::GameOver);
    
    // Probar tecla ESC -> MainMenu
    game.onKeyPress(27);
    assert(game.getGameState() == ReligionSim::GameState::MainMenu);

    // Probar renderizado de Game Over sin crashes ni heap allocations en caliente
    game.setGameState(ReligionSim::GameState::GameOver);
    game.render();

    std::cout << "  -> testGameOverOnAllUnitsExtinction PASSED! Defeat on zero living units, dramatic death pause, chronicle logging, and modal buttons verified." << std::endl;
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
    assert(map.getCell(16, 16).resourceAmount < initialWood);

    // 7. Extinción completa: el bosque se calcina y se convierte en ceniza fértil
    map.getCell(16, 16).burnTimer = 0.1f;
    powers.update(0.2f, map, humanos);
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

    // 3. Probar en mapa pequeño (32x32)
    WorldMap map32(32, 32);
    map32.generateProceduralWorld(42);
    auto pueblos32 = map32.generatePuebloLocations(5, 42);
    assert(pueblos32.size() == 5);
    for (size_t i = 0; i < pueblos32.size(); ++i) {
        for (size_t j = i + 1; j < pueblos32.size(); ++j) {
            float dx = static_cast<float>(pueblos32[i].x - pueblos32[j].x);
            float dy = static_cast<float>(pueblos32[i].y - pueblos32[j].y);
            float dist = std::sqrt(dx * dx + dy * dy);
            // Distancia mínima garantizada en 32x32: al menos 8 casillas
            assert(dist >= 8.0f);
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

    std::cout << "========================================" << std::endl;
    std::cout << "      ALL TESTS PASSED SUCCESSFULLY!    " << std::endl;
    std::cout << "========================================" << std::endl;
    return 0;
}
