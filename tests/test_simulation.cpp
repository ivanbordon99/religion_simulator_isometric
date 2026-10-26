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
#include "Renderer.hpp"
#include "TimeEngine.hpp"
#include "UISystem.hpp"
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
    for (int textureIndex = 8; textureIndex < 16; ++textureIndex) {
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

    std::cout << "  -> testExpandedTerrainAtlas PASSED! All eight new terrain textures render correctly." << std::endl;
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

    std::cout << "========================================" << std::endl;
    std::cout << "      ALL TESTS PASSED SUCCESSFULLY!    " << std::endl;
    std::cout << "========================================" << std::endl;
    return 0;
}
