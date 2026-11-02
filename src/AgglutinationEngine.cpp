#include "AgglutinationEngine.hpp"
#include <random>
#include <algorithm>
#include <map>

namespace ReligionSim {

namespace {
    std::mt19937 g_aggRng(999);
}

AgglutinationEngine::AgglutinationEngine() {}

void AgglutinationEngine::update(WorldMap& map, ReligionRegistry& registry, float dtYears,
                                std::vector<GameEvent>& outEvents, int currentYear) {
    m_agglutinationAccumulator += dtYears;

    // Check syncretism & schisms every ~10 simulation years
    if (m_agglutinationAccumulator >= 8.0f) {
        checkSyncreticAgglutination(map, registry, m_agglutinationAccumulator, outEvents, currentYear);
        checkDoctrinalSchisms(map, registry, m_agglutinationAccumulator, outEvents, currentYear);
        m_agglutinationAccumulator = 0.0f;
    }
}

void AgglutinationEngine::checkSyncreticAgglutination(WorldMap& map, ReligionRegistry& registry,
                                                    float dtYears, std::vector<GameEvent>& outEvents,
                                                    int currentYear) {
    (void)dtYears;
    int w = map.getWidth();
    int h = map.getHeight();

    std::uniform_real_distribution<float> prob(0.0f, 1.0f);

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            Cell& c = map.getCell(x, y);
            if (!c.isHabitable() || c.population <= 0 || c.faithShare.size() < 2) {
                c.coexistingCenturies = 0;
                continue;
            }

            // Find two top faiths
            int r1 = 0, r2 = 0;
            float s1 = 0.0f, s2 = 0.0f;
            for (const auto& p : c.faithShare) {
                if (p.second > s1) {
                    s2 = s1; r2 = r1;
                    s1 = p.second; r1 = p.first;
                } else if (p.second > s2) {
                    s2 = p.second; r2 = p.first;
                }
            }

            // Both faiths must have significant local presence (> 20%)
            if (s1 > 0.22f && s2 > 0.22f && r1 != r2) {
                c.coexistingCenturies++;
                c.syncreticTension = std::min(1.0f, c.syncreticTension + 0.05f);

                const Religion* rel1 = registry.getReligion(r1);
                const Religion* rel2 = registry.getReligion(r2);
                if (!rel1 || !rel2) continue;

                float synChance = (rel1->syncretismTolerance + rel2->syncretismTolerance) * 0.5f;
                if (rel1->hasDoctrine(DoctrineType::SyncreticOpenness) ||
                    rel2->hasDoctrine(DoctrineType::SyncreticOpenness)) {
                    synChance += 0.35f;
                }

                // If coexisted for sufficient time and dice rolls favorable
                if (c.coexistingCenturies >= 3 && prob(g_aggRng) < (synChance * 0.12f)) {
                    int newRelId = registry.createSyncreticReligion(r1, r2, currentYear, {x, y});
                    const Religion* newRel = registry.getReligion(newRelId);
                    if (newRel) {
                        // Agglutinate believers in epicenter and immediate neighbors
                        c.setShare(newRelId, 0.65f);
                        c.coexistingCenturies = 0;

                        auto neighbors = map.getNeighbors(x, y, true);
                        for (const auto& nb : neighbors) {
                            Cell& nbc = map.getCell(nb.x, nb.y);
                            if (nbc.isHabitable() && nbc.population > 0) {
                                nbc.addShare(newRelId, 0.35f);
                                nbc.normalizeShares();
                                nbc.updateDominance();
                            }
                        }

                        // Emit event
                        GameEvent ev;
                        ev.year = currentYear;
                        ev.category = EventCategory::SyncreticAgglutination;
                        ev.location = {x, y};
                        ev.religionAId = r1;
                        ev.religionBId = r2;
                        ev.badgeColor = newRel->color;
                        ev.title = "Fusion Sincretista: " + newRel->name;
                        ev.description = "El contacto sagrado prolongado entre " + rel1->name + " y " + rel2->name +
                                         " ha engendrado una comunion espiritual hibrida y unificada: " + newRel->name + "!";
                        ev.isTurningPoint = true;
                        ev.actorName = "Fieles de " + rel1->name + " y " + rel2->name;
                        ev.consequence = "Nacimiento de dogma sincretico y unificacion de linajes";
                        outEvents.push_back(ev);
                        return; // One major agglutination per check
                    }
                }
            } else {
                c.coexistingCenturies = std::max(0, c.coexistingCenturies - 1);
            }
        }
    }
}

void AgglutinationEngine::checkDoctrinalSchisms(WorldMap& map, ReligionRegistry& registry,
                                              float dtYears, std::vector<GameEvent>& outEvents,
                                              int currentYear) {
    (void)dtYears;
    int w = map.getWidth();
    int h = map.getHeight();

    std::uniform_real_distribution<float> prob(0.0f, 1.0f);

    for (auto& r : registry.getAllReligions()) {
        if (r.isExtinct) continue;

        // A religion risks a schism if it has grown large and spans many cells
        if (r.totalCellsDominant >= 16 && r.schismCount < 2) {
            float schismRisk = (1.0f - r.cohesion) * 0.10f + (r.totalCellsDominant * 0.005f);

            // Eras of Universal Religions or Rationalization heighten reformation & schism potential
            if (r.currentEra == HistoricalEra::RationalizationSecularization) schismRisk *= 1.8f;
            else if (r.currentEra == HistoricalEra::UniversalReligions) schismRisk *= 1.4f;
            else if (r.currentEra == HistoricalEra::PostSecular) schismRisk *= 1.6f;

            if (prob(g_aggRng) < schismRisk) {
                // Find a frontier cell far from existing holy sites (minimum distance >= 6 cells)
                std::vector<Point2i> candidates;
                for (int y = 0; y < h; ++y) {
                    for (int x = 0; x < w; ++x) {
                        const Cell& c = map.getCell(x, y);
                        if (c.population > 0 && c.dominantReligionId == r.id && !c.hasHolySite) {
                            bool tooCloseToTemple = false;
                            for (int hy = std::max(0, y - 6); hy <= std::min(h - 1, y + 6) && !tooCloseToTemple; ++hy) {
                                for (int hx = std::max(0, x - 6); hx <= std::min(w - 1, x + 6); ++hx) {
                                    if (map.getCell(hx, hy).hasHolySite && (std::abs(hx - x) + std::abs(hy - y) < 6)) {
                                        tooCloseToTemple = true;
                                        break;
                                    }
                                }
                            }
                            if (!tooCloseToTemple) {
                                candidates.push_back({x, y});
                            }
                        }
                    }
                }

                if (!candidates.empty()) {
                    std::uniform_int_distribution<size_t> cDist(0, candidates.size() - 1);
                    Point2i origin = candidates[cDist(g_aggRng)];

                    int schismId = registry.createSchismReligion(r.id, currentYear, origin);
                    const Religion* schismRel = registry.getReligion(schismId);
                    if (schismRel) {
                        Cell& epicenter = map.getCell(origin.x, origin.y);
                        epicenter.setShare(schismId, 0.85f);
                        epicenter.devotion = 0.90f;
                        map.consecrateHolySite(origin.x, origin.y, "Cradle of " + schismRel->name);

                        // Convert frontier cluster only where villagers are present
                        auto neighbors = map.getNeighbors(origin.x, origin.y, true);
                        for (const auto& nb : neighbors) {
                            Cell& nbc = map.getCell(nb.x, nb.y);
                            if (nbc.isHabitable() && nbc.population > 0) {
                                nbc.setShare(schismId, 0.55f);
                                nbc.normalizeShares();
                                nbc.updateDominance();
                            }
                        }

                        GameEvent ev;
                        ev.year = currentYear;
                        ev.category = EventCategory::DoctrinalSchism;
                        ev.location = origin;
                        ev.religionAId = r.id;
                        ev.religionBId = schismId;
                        ev.badgeColor = schismRel->color;
                        ev.title = "Cisma Doctrinal en " + r.name;
                        ev.description = "El profeta " + schismRel->founderName + " ha proclamado la herejia reformista, fundando " +
                                         schismRel->name + " en desafio a la ortodoxia ancestral.";
                        ev.isTurningPoint = true;
                        ev.actorName = schismRel->founderName;
                        ev.consequence = "Fractura teologica y dispersion de fieles";
                        outEvents.push_back(ev);
                        return; // One schism per check cycle
                    }
                }
            }
        }
    }
}

int AgglutinationEngine::triggerForcedSyncretism(WorldMap& map, ReligionRegistry& registry,
                                                int x, int y, std::vector<GameEvent>& outEvents,
                                                int currentYear) {
    if (!map.isValid(x, y)) return 0;
    Cell& c = map.getCell(x, y);
    if (c.faithShare.size() < 2) return 0;

    int r1 = c.faithShare[0].first;
    int r2 = c.faithShare[1].first;

    int newId = registry.createSyncreticReligion(r1, r2, currentYear, {x, y});
    const Religion* newRel = registry.getReligion(newId);
    if (newRel) {
        c.setShare(newId, 0.80f);
        map.upgradeHolySite(x, y);

        GameEvent ev;
        ev.year = currentYear;
        ev.category = EventCategory::SyncreticAgglutination;
        ev.location = {x, y};
        ev.religionAId = r1;
        ev.religionBId = r2;
        ev.badgeColor = newRel->color;
        ev.title = "Divine Synthesis: " + newRel->name;
        ev.description = "By spiritual decree, " + registry.getReligion(r1)->name + " and " +
                         registry.getReligion(r2)->name + " have fused into " + newRel->name + "!";
        outEvents.push_back(ev);
    }
    return newId;
}

int AgglutinationEngine::triggerForcedSchism(WorldMap& map, ReligionRegistry& registry,
                                            int x, int y, std::vector<GameEvent>& outEvents,
                                            int currentYear) {
    if (!map.isValid(x, y)) return 0;
    Cell& c = map.getCell(x, y);
    if (c.dominantReligionId == 0) return 0;

    int parentId = c.dominantReligionId;
    int schismId = registry.createSchismReligion(parentId, currentYear, {x, y});
    const Religion* schismRel = registry.getReligion(schismId);
    if (schismRel) {
        c.setShare(schismId, 0.85f);
        map.consecrateHolySite(x, y, "Reformer Haven of " + schismRel->name);

        GameEvent ev;
        ev.year = currentYear;
        ev.category = EventCategory::DoctrinalSchism;
        ev.location = {x, y};
        ev.religionAId = parentId;
        ev.religionBId = schismId;
        ev.badgeColor = schismRel->color;
        ev.title = "Spark of Reformation: " + schismRel->name;
        ev.description = "A fiery reformation sweeps through the provinces, establishing " + schismRel->name + "!";
        outEvents.push_back(ev);
    }
    return schismId;
}

} // namespace ReligionSim
