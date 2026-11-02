#include "ReplicationEngine.hpp"
#include <algorithm>
#include <cmath>
#include <random>

namespace ReligionSim {

namespace {
    std::mt19937 g_repRng(777);
}

ReplicationEngine::ReplicationEngine() {
    m_caravans.reserve(64);
}

void ReplicationEngine::update(WorldMap& map, ReligionRegistry& registry, float dtYears,
                               std::vector<GameEvent>& outEvents, int currentYear) {
    processDemographics(map, registry, dtYears);
    processSpatialDiffusion(map, registry, dtYears);
    processMissionaryCaravans(map, registry, dtYears, outEvents, currentYear);
    processBorderTensionAndLiminalInteractions(map, registry, dtYears, outEvents, currentYear);
}

void ReplicationEngine::processDemographics(WorldMap& map, const ReligionRegistry& registry, float dtYears) {
    int w = map.getWidth();
    int h = map.getHeight();
    (void)dtYears;

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            Cell& c = map.getCell(x, y);
            if (!c.isHabitable()) continue;

            // La población de cada celda está determinada 1:1 por los aldeanos reales.
            // Si la religión dominante tiene doctrinas de cosecha y fertilidad, el límite de población crece
            int maxPop = std::max(1, c.maxPopulation);
            if (c.dominantReligionId > 0) {
                const Religion* domRel = registry.getReligion(c.dominantReligionId);
                if (domRel && domRel->harvestYieldBonus > 0.0f) {
                    maxPop = static_cast<int>(maxPop * (1.0f + domRel->harvestYieldBonus));
                }
            }
            c.population = std::clamp(c.population, 0, maxPop * 2);

            // Development scales with sustained real population
            c.development = std::clamp(static_cast<float>(c.population) / static_cast<float>(maxPop), 0.05f, 1.0f);
        }
    }
}

void ReplicationEngine::processSpatialDiffusion(WorldMap& map, ReligionRegistry& registry, float dtYears) {
    int w = map.getWidth();
    int h = map.getHeight();

    // Delta buffer to apply changes simultaneously
    struct CellDiff {
        int religionId;
        float delta;
    };
    std::vector<std::vector<CellDiff>> diffs(w * h);

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            const Cell& c = map.getCell(x, y);
            // Las zonas de influencia solo existen y difunden donde hay aldeanos (nodos de fe)
            if (!c.isHabitable() || c.population <= 0 || c.faithShare.empty()) continue;

            auto neighbors = map.getNeighbors(x, y, true);

            for (const auto& sharePair : c.faithShare) {
                int rId = sharePair.first;
                float shareSrc = sharePair.second;
                if (shareSrc < 0.02f) continue;

                const Religion* rel = registry.getReligion(rId);
                if (!rel || rel->isExtinct) continue;

                float zeal = rel->zeal;
                float shrineBonus = c.hasHolySite ? (1.0f + 0.35f * c.shrineLevel * rel->holySitePowerMultiplier) : 1.0f;
                float baseDiffusion = 0.025f * zeal * shrineBonus * rel->diffusionMultiplier * dtYears;

                for (const auto& nb : neighbors) {
                    const Cell& target = map.getCell(nb.x, nb.y);
                    // Solo difundir hacia celdas que también tengan aldeanos presentes
                    if (!target.isHabitable() || target.population <= 0) continue;

                    float travel = map.getTravelEase(c, target);
                    float targetShare = target.getShare(rId);

                    // Gradient diffusion: spread when source has higher concentration or target is open
                    if (shareSrc > targetShare + 0.05f) {
                        float gradient = shareSrc - targetShare;

                        // Target resistance from rival dominant faith
                        float resistance = 1.0f;
                        if (target.dominantReligionId > 0 && target.dominantReligionId != rId) {
                            const Religion* rival = registry.getReligion(target.dominantReligionId);
                            if (rival) {
                                resistance = 1.0f + rival->cohesion * target.devotion * 2.0f;
                            }
                        }

                        float transfer = (gradient * baseDiffusion * travel) / resistance;
                        transfer = std::clamp(transfer, 0.0f, 0.15f);

                        if (transfer > 0.0005f) {
                            diffs[nb.y * w + nb.x].push_back({rId, transfer});
                        }
                    }
                }
            }
        }
    }

    // Apply accumulated diffusion (only on cells with villagers)
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            Cell& c = map.getCell(x, y);
            if (!c.isHabitable() || c.population <= 0) {
                c.faithShare.clear();
                c.dominantReligionId = 0;
                c.dominantFraction = 0.0f;
                continue;
            }

            const auto& cellDiffs = diffs[y * w + x];
            for (const auto& cd : cellDiffs) {
                c.addShare(cd.religionId, cd.delta);
            }

            c.normalizeShares();
            c.updateDominance();
        }
    }
}

void ReplicationEngine::processMissionaryCaravans(WorldMap& map, ReligionRegistry& registry, float dtYears,
                                                 std::vector<GameEvent>& outEvents, int currentYear) {
    int w = map.getWidth();
    int h = map.getHeight();

    // Advance existing caravans
    for (auto it = m_caravans.begin(); it != m_caravans.end();) {
        const Religion* rel = registry.getReligion(it->religionId);
        float speedMult = (rel && rel->hasAdvancement(201)) ? 1.35f : 1.0f;
        it->progress += 0.25f * dtYears * speedMult; // Travel duration ~ 3-4 years
        if (it->progress >= 1.0f) {
            // Caravan arrived at target cell!
            if (map.isValid(it->to.x, it->to.y)) {
                Cell& target = map.getCell(it->to.x, it->to.y);
                if (target.isHabitable() && target.population > 0) {
                    target.addShare(it->religionId, it->power);
                    target.normalizeShares();
                    target.updateDominance();

                    const Religion* r = registry.getReligion(it->religionId);
                    if (r && target.dominantReligionId == it->religionId && target.dominantFraction > 0.6f) {
                        GameEvent ev;
                        ev.year = currentYear;
                        ev.category = EventCategory::PilgrimageWave;
                        ev.location = it->to;
                        ev.religionAId = it->religionId;
                        ev.badgeColor = r->color;
                        ev.title = "Missionary Vanguard Triumphs";
                        ev.description = "Missionaries of " + r->name + " have established a major congregation in new lands.";
                        outEvents.push_back(ev);
                    }
                }
            }
            it = m_caravans.erase(it);
        } else {
            ++it;
        }
    }

    // Spawn new caravans from high-piety Holy Sites
    size_t maxCaravans = 12;
    for (const auto& r : registry.getAllReligions()) {
        if (!r.isExtinct && r.hasAdvancement(201)) maxCaravans = 18;
    }

    if (m_caravans.size() < maxCaravans) {
        std::uniform_int_distribution<int> rx(0, w - 1);
        std::uniform_int_distribution<int> ry(0, h - 1);
        std::uniform_real_distribution<float> probDist(0.0f, 1.0f);

        for (int i = 0; i < 5; ++i) {
            int sx = rx(g_repRng);
            int sy = ry(g_repRng);
            const Cell& c = map.getCell(sx, sy);
            if (!c.hasHolySite || c.population <= 0 || c.dominantReligionId <= 0) continue;

            // En dificultad Pasiva, las religiones rivales no envían caravanas ofensivas hacia otras tierras
            if (c.dominantReligionId != 1 && m_aiDifficulty == AIDifficulty::Pasiva) continue;

            if (probDist(g_repRng) < 0.26f) {
                int tx = -1, ty = -1;
                // En dificultad Estratégica o Implacable, la IA rival busca inteligentemente celdas fronterizas disputadas con alta población
                if (c.dominantReligionId != 1 && (m_aiDifficulty == AIDifficulty::Estrategica || m_aiDifficulty == AIDifficulty::Implacable)) {
                    int bestScore = -1;
                    for (int dy = -7; dy <= 7; ++dy) {
                        for (int dx = -7; dx <= 7; ++dx) {
                            int nx = sx + dx;
                            int ny = sy + dy;
                            if (!map.isValid(nx, ny)) continue;
                            const Cell& tc = map.getCell(nx, ny);
                            if (tc.isHabitable() && tc.population > 0 && tc.dominantReligionId != c.dominantReligionId) {
                                int score = tc.population;
                                if (tc.isContestedBorder) score += 40;
                                if (tc.dominantReligionId == 1) score += 35; // Foco prioritario hacia la frontera con el jugador
                                if (score > bestScore) {
                                    bestScore = score;
                                    tx = nx;
                                    ty = ny;
                                }
                            }
                        }
                    }
                }

                // Fallback aleatorio estándar
                if (tx == -1 || ty == -1) {
                    tx = rx(g_repRng);
                    ty = ry(g_repRng);
                }

                const Cell& tc = map.getCell(tx, ty);
                if (tc.isHabitable() && tc.population > 0 && tc.dominantReligionId != c.dominantReligionId) {
                    const Religion* r = registry.getReligion(c.dominantReligionId);
                    float hsm = r ? r->holySitePowerMultiplier : 1.0f;
                    MissionaryCaravan car;
                    car.from = {sx, sy};
                    car.to = {tx, ty};
                    car.religionId = c.dominantReligionId;
                    car.progress = 0.0f;
                    car.power = (0.25f + 0.05f * c.shrineLevel) * hsm;
                    m_caravans.push_back(car);
                }
            }
        }
    }
}

void ReplicationEngine::processBorderTensionAndLiminalInteractions(WorldMap& map, ReligionRegistry& registry,
                                                                   float dtYears, std::vector<GameEvent>& outEvents,
                                                                   int currentYear) {
    int w = map.getWidth();
    int h = map.getHeight();
    std::uniform_real_distribution<float> prob(0.0f, 1.0f);

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            Cell& c = map.getCell(x, y);
            if (!c.isHabitable() || c.population <= 0 || c.dominantReligionId <= 0) {
                c.borderTension = 0.0f;
                c.isContestedBorder = false;
                c.borderRivalReligionId = 0;
                continue;
            }

            int rA = c.dominantReligionId;
            Religion* relA = registry.getReligion(rA);
            if (!relA) continue;

            // Decaimiento de treguas sagradas / pactos de sangre
            if (c.isSacredTruceActive) {
                c.sacredTruceTimer -= dtYears;
                if (c.sacredTruceTimer <= 0.0f) {
                    c.isSacredTruceActive = false;
                    c.sacredTruceTimer = 0.0f;
                }
            }

            float maxTension = 0.0f;
            int maxRivalId = 0;
            auto neighbors = map.getNeighbors(x, y, true);

            for (const auto& nb : neighbors) {
                Cell& target = map.getCell(nb.x, nb.y);
                if (!target.isHabitable() || target.population <= 0 || target.dominantReligionId <= 0) continue;
                int rB = target.dominantReligionId;
                if (rA == rB) continue;

                Religion* relB = registry.getReligion(rB);
                if (!relB) continue;

                // Fricción ontológica entre arquetipos cosmogónicos Φ(A, B)
                float friction = relA->calculateOntologicalFriction(*relB);
                float meanTol = (relA->syncretismTolerance + relB->syncretismTolerance) * 0.5f;
                float meanZealDev = (relA->zeal * c.devotion + relB->zeal * target.devotion) * 0.5f;

                // Tensión liminal de frontera modulada por Beligerancia Tribal
                float belMod = (m_tribalBelligerence == TribalBelligerence::Pacifica) ? 0.60f :
                               ((m_tribalBelligerence == TribalBelligerence::Hostil) ? 1.45f : 1.0f);
                float omega = std::clamp(meanZealDev * friction * (1.2f - meanTol) * belMod, 0.0f, 1.0f);
                if (c.isSacredTruceActive || target.isSacredTruceActive) {
                    omega *= 0.20f; // La tregua apacigua radicalmente la hostilidad
                }

                if (omega > maxTension) {
                    maxTension = omega;
                    maxRivalId = rB;
                }

                // Ocasión de Pacto de Sangre y Alianza Exogámica en fronteras porosas de baja tensión
                float pactChance = (m_tribalBelligerence == TribalBelligerence::Pacifica) ? 0.08f :
                                   ((m_tribalBelligerence == TribalBelligerence::Hostil) ? 0.015f : 0.04f);
                if (omega < 0.28f && !c.isSacredTruceActive && !target.isSacredTruceActive &&
                    prob(g_repRng) < (pactChance * dtYears)) {
                    c.isSacredTruceActive = true;
                    c.sacredTruceTimer = 35.0f;
                    target.isSacredTruceActive = true;
                    target.sacredTruceTimer = 35.0f;

                    // Reciprocidad de dones (Gift economy antropológica)
                    relA->resources.food += 25;
                    relB->resources.food += 25;
                    relA->resources.wood += 15;
                    relB->resources.wood += 15;

                    GameEvent ev;
                    ev.year = currentYear;
                    ev.category = EventCategory::ExogamicPact;
                    ev.location = {x, y};
                    ev.religionAId = rA;
                    ev.religionBId = rB;
                    ev.badgeColor = ColorRGB{240, 220, 110};
                    ev.title = "Pacto de Sangre Exogamico";
                    ev.description = "Los clanes fronterizos de " + relA->name + " y " + relB->name +
                                     " han concertado una alianza matrimonial sagrada e intercambio de dones, sellando la paz en el linde.";
                    ev.isTurningPoint = false;
                    ev.actorName = "Ancianos de " + relA->name + " y " + relB->name;
                    ev.consequence = "Tregua sagrada de frontera y prosperidad compartida (+25 Comida, +15 Materiales)";
                    outEvents.push_back(ev);
                }

                // Fugas e intercambio benéfico en fronteras pacíficas
                if ((omega < 0.25f || c.isSacredTruceActive) && relA->harvestYieldBonus > 0.0f) {
                    target.development = std::min(1.0f, target.development + 0.005f * dtYears);
                }
            }

            c.borderTension = maxTension;
            c.isContestedBorder = (maxTension >= 0.40f);
            c.borderRivalReligionId = maxRivalId;
        }
    }
}

} // namespace ReligionSim
