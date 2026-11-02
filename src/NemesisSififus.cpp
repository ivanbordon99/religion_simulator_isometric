#include "NemesisSififus.hpp"
#include "Humano.hpp"
#include "WorldMap.hpp"
#include "Religion.hpp"
#include "EventLine.hpp"
#include <iostream>
#include <algorithm>

namespace ReligionSim {

// =========================================================================
// 1. SISTEMA NÉMESIS IMPLEMENTACIÓN
// =========================================================================
namespace Nemesis {

void NemesisProfile::generateProceduralEpithet(MemoryEventType trigger) {
    switch (trigger) {
        case MemoryEventType::SurvivedCombat:
            if (grudgePlayer > 0.4f || grudgeFaction > 0.4f) {
                std::snprintf(epithet, sizeof(epithet), "El Cicatriz de Guerra");
            } else {
                std::snprintf(epithet, sizeof(epithet), "El Inquebrantable");
            }
            break;
        case MemoryEventType::WoundedByDivinePower:
            if (grudgePlayer > 0.3f) {
                std::snprintf(epithet, sizeof(epithet), "El Marcado por el Rayo");
            } else {
                std::snprintf(epithet, sizeof(epithet), "El Tocado por el Fuego");
            }
            break;
        case MemoryEventType::SparedByPlayer:
            std::snprintf(epithet, sizeof(epithet), "El Resucitado de Gracia");
            break;
        case MemoryEventType::WitnessedSacrilege:
            std::snprintf(epithet, sizeof(epithet), "El Azote del Hereje");
            break;
        case MemoryEventType::BetrayedByLeader:
            std::snprintf(epithet, sizeof(epithet), "El Paria Renegado");
            break;
        case MemoryEventType::RivalKilledKinsman:
            std::snprintf(epithet, sizeof(epithet), "Ojo de Sangre");
            break;
        case MemoryEventType::StoleHolyRelic:
            std::snprintf(epithet, sizeof(epithet), "El Ladron Sagrado");
            break;
        case MemoryEventType::Excommunicated:
            std::snprintf(epithet, sizeof(epithet), "El Hereje Indomito");
            break;
        default:
            std::snprintf(epithet, sizeof(epithet), "El Superviviente");
            break;
    }
}

NotableUnitsPool::NotableUnitsPool(size_t maxEntities, size_t maxNotables)
    : m_sparse(maxEntities, INVALID_INDEX), m_maxNotables(maxNotables) {
    m_dense.reserve(maxNotables);
    m_profiles.reserve(maxNotables);
}

bool NotableUnitsPool::contains(int32_t entityId) const noexcept {
    if (entityId < 0 || static_cast<size_t>(entityId) >= m_sparse.size()) return false;
    uint32_t idx = m_sparse[entityId];
    return idx < m_dense.size() && m_dense[idx] == entityId;
}

NemesisProfile* NotableUnitsPool::get(int32_t entityId) noexcept {
    if (!contains(entityId)) return nullptr;
    return &m_profiles[m_sparse[entityId]];
}

const NemesisProfile* NotableUnitsPool::get(int32_t entityId) const noexcept {
    if (!contains(entityId)) return nullptr;
    return &m_profiles[m_sparse[entityId]];
}

NemesisProfile& NotableUnitsPool::promoteOrEmplace(int32_t entityId) {
    if (contains(entityId)) {
        return m_profiles[m_sparse[entityId]];
    }

    if (static_cast<size_t>(entityId) >= m_sparse.size()) {
        m_sparse.resize(entityId + 512, INVALID_INDEX);
    }

    if (m_dense.size() >= m_maxNotables) {
        evictLeastSignificantNotable();
    }

    uint32_t denseIdx = static_cast<uint32_t>(m_dense.size());
    m_sparse[entityId] = denseIdx;
    m_dense.push_back(entityId);

    NemesisProfile prof;
    prof.entityId = entityId;
    m_profiles.push_back(prof);

    return m_profiles.back();
}

void NotableUnitsPool::remove(int32_t entityId) noexcept {
    if (!contains(entityId)) return;

    uint32_t denseIdx = m_sparse[entityId];
    uint32_t lastDenseIdx = static_cast<uint32_t>(m_dense.size() - 1);
    int32_t lastEntity = m_dense[lastDenseIdx];

    // Swap and pop
    m_dense[denseIdx] = lastEntity;
    m_profiles[denseIdx] = m_profiles[lastDenseIdx];
    m_sparse[lastEntity] = denseIdx;

    m_dense.pop_back();
    m_profiles.pop_back();
    m_sparse[entityId] = INVALID_INDEX;
}

void NotableUnitsPool::evictLeastSignificantNotable() {
    if (m_profiles.empty()) return;
    size_t lowestIdx = 0;
    float minScore = 99999.0f;
    for (size_t i = 0; i < m_profiles.size(); ++i) {
        float score = static_cast<float>(m_profiles[i].rank) * 10.0f +
                      std::abs(m_profiles[i].grudgePlayer) +
                      std::abs(m_profiles[i].grudgeFaction);
        if (score < minScore) {
            minScore = score;
            lowestIdx = i;
        }
    }
    remove(m_dense[lowestIdx]);
}

} // namespace Nemesis

// =========================================================================
// 2. SISTEMA SÍSIFO IMPLEMENTACIÓN
// =========================================================================
namespace Sififus {

MythRecord NemesisSififusBridge::generateMythFromNemesis(const Nemesis::NemesisProfile& profile,
                                                         const Nemesis::LifeEvent& event,
                                                         const char* protagonistName,
                                                         int32_t mythId) {
    MythRecord myth;
    myth.mythId = mythId;
    myth.protagonistEntityId = profile.entityId;
    myth.creationYear = event.timestampYear;
    myth.targetReligionId = event.targetReligionId;
    myth.resonance = 1.0f;
    myth.decayRate = 0.012f;

    std::string safeName = (protagonistName && protagonistName[0] != '\0') ? protagonistName : "El Elegido";

    switch (event.type) {
        case Nemesis::MemoryEventType::WoundedByDivinePower:
            if (profile.grudgePlayer > 0.35f) {
                myth.archetype = MythArchetype::PrometheanDefiance;
                myth.ethos.divineFearBias = -0.80f;
                myth.ethos.aggressionBias = 0.55f;
                myth.ethos.devotionPriority = -0.65f;
                std::snprintf(myth.title, sizeof(myth.title), "El Desafio de %s", safeName.c_str());
                std::snprintf(myth.oralSnippet, sizeof(myth.oralSnippet),
                              "¡El fuego cayo del cielo y devoro la tierra, mas %s se alzo desafiante!", safeName.c_str());
            } else {
                myth.archetype = MythArchetype::DivineRetribution;
                myth.ethos.divineFearBias = 0.90f;
                myth.ethos.devotionPriority = 0.75f;
                std::snprintf(myth.title, sizeof(myth.title), "El Escarmiento Celestial de %s", safeName.c_str());
                std::snprintf(myth.oralSnippet, sizeof(myth.oralSnippet),
                              "Los dioses tronaron sobre %s; la sumision es el unico camino a la salvacion.", safeName.c_str());
            }
            break;

        case Nemesis::MemoryEventType::SurvivedCombat:
        case Nemesis::MemoryEventType::RivalKilledKinsman:
            myth.archetype = MythArchetype::BloodVendetta;
            myth.ethos.aggressionBias = 0.85f;
            myth.ethos.xenophobiaBias = 0.75f;
            myth.ethos.martyrdomDrive = 0.50f;
            std::snprintf(myth.title, sizeof(myth.title), "El Juramento de Sangre de %s", safeName.c_str());
            std::snprintf(myth.oralSnippet, sizeof(myth.oralSnippet),
                          "Por las heridas de %s, ningún rival cruzara en son de paz.", safeName.c_str());
            break;

        case Nemesis::MemoryEventType::SparedByPlayer:
            myth.archetype = MythArchetype::SacredCovenant;
            myth.ethos.divineFearBias = 0.20f;
            myth.ethos.devotionPriority = 0.95f;
            myth.ethos.martyrdomDrive = 0.60f;
            std::snprintf(myth.title, sizeof(myth.title), "La Gracia Sagrada de %s", safeName.c_str());
            std::snprintf(myth.oralSnippet, sizeof(myth.oralSnippet),
                          "La mano divina descendio en misericordia y toco el alma de %s.", safeName.c_str());
            break;

        case Nemesis::MemoryEventType::WitnessedSacrilege:
            myth.archetype = MythArchetype::HolyTaboo;
            myth.ethos.martyrdomDrive = 0.80f;
            myth.ethos.xenophobiaBias = 0.85f;
            std::snprintf(myth.title, sizeof(myth.title), "La Profanacion del Altar de %s", safeName.c_str());
            std::snprintf(myth.oralSnippet, sizeof(myth.oralSnippet),
                          "¡El fuego sagrado fue profanado ante los ojos de %s!", safeName.c_str());
            break;

        default:
            myth.archetype = MythArchetype::MartyrSacrifice;
            myth.ethos.martyrdomDrive = 0.45f;
            std::snprintf(myth.title, sizeof(myth.title), "La Balada de %s", safeName.c_str());
            std::snprintf(myth.oralSnippet, sizeof(myth.oralSnippet),
                          "Las leyendas de %s resonaran a traves de las eras.", safeName.c_str());
            break;
    }

    return myth;
}

SififusEngine::SififusEngine(EventLine* eventLine) : m_eventLine(eventLine) {
    m_activeMyths.reserve(MAX_ACTIVE_MYTHS);
}

void SififusEngine::registerMyth(const MythRecord& myth) {
    if (m_activeMyths.size() >= MAX_ACTIVE_MYTHS) {
        evictFadedMyth();
    }
    m_activeMyths.push_back(myth);

    if (m_eventLine) {
        Point2i loc{0, 0};
        m_eventLine->addEvent(myth.creationYear, EventCategory::Discovery,
                              std::string(myth.title),
                              std::string(myth.oralSnippet), loc,
                              myth.originReligionId, myth.targetReligionId,
                              ColorRGB{220, 180, 70}, true,
                              "Sistema Sísifo (Mitogénesis)",
                              "Relato cultural activo que altera la conducta de los fieles.");
    }

    recomputeCollectiveEthos();
}

void SififusEngine::update(float dtYears, int currentYear) {
    if (dtYears <= 0.0f) return;

    bool changed = false;
    for (auto it = m_activeMyths.begin(); it != m_activeMyths.end();) {
        it->resonance = std::max(0.0f, it->resonance - it->decayRate * dtYears);
        if (it->resonance <= 0.06f) {
            if (m_eventLine) {
                Point2i loc{0, 0};
                m_eventLine->addEvent(currentYear, EventCategory::MajorLoss,
                                      "Olvido Cultural: " + std::string(it->title),
                                      "El eco de este relato se ha desvanecido en la memoria oral de las tribus.",
                                      loc, it->originReligionId, 0, ColorRGB{160, 150, 140});
            }
            it = m_activeMyths.erase(it);
            changed = true;
        } else {
            ++it;
        }
    }

    if (changed || dtYears > 0.05f) {
        recomputeCollectiveEthos();
    }
}

EthosVector SififusEngine::getCollectiveEthos(int16_t religionId) const noexcept {
    auto it = m_collectiveEthos.find(religionId);
    if (it != m_collectiveEthos.end()) {
        return it->second;
    }
    return EthosVector{};
}

void SififusEngine::evictFadedMyth() {
    if (m_activeMyths.empty()) return;
    size_t minIdx = 0;
    float minRes = 999.0f;
    for (size_t i = 0; i < m_activeMyths.size(); ++i) {
        if (m_activeMyths[i].resonance < minRes) {
            minRes = m_activeMyths[i].resonance;
            minIdx = i;
        }
    }
    m_activeMyths.erase(m_activeMyths.begin() + minIdx);
}

void SififusEngine::recomputeCollectiveEthos() {
    m_collectiveEthos.clear();

    for (const auto& myth : m_activeMyths) {
        EthosVector& eth = m_collectiveEthos[myth.originReligionId];
        float w = myth.resonance;

        eth.aggressionBias   += myth.ethos.aggressionBias * w;
        eth.divineFearBias   += myth.ethos.divineFearBias * w;
        eth.devotionPriority += myth.ethos.devotionPriority * w;
        eth.xenophobiaBias   += myth.ethos.xenophobiaBias * w;
        eth.martyrdomDrive   += myth.ethos.martyrdomDrive * w;
    }

    for (auto& [relId, eth] : m_collectiveEthos) {
        (void)relId;
        eth.aggressionBias   = std::clamp(eth.aggressionBias, -1.0f, 1.0f);
        eth.divineFearBias   = std::clamp(eth.divineFearBias, -1.0f, 1.0f);
        eth.devotionPriority = std::clamp(eth.devotionPriority, -1.0f, 1.0f);
        eth.xenophobiaBias   = std::clamp(eth.xenophobiaBias, -1.0f, 1.0f);
        eth.martyrdomDrive   = std::clamp(eth.martyrdomDrive, 0.0f, 1.0f);
    }
}

} // namespace Sififus

// =========================================================================
// 3. GESTOR UNIFICADO: NÉMESIS & SÍSIFO MANAGER IMPLEMENTACIÓN
// =========================================================================
NemesisSififusManager::NemesisSififusManager()
    : m_pool(4096, 128), m_sififus(nullptr) {}

void NemesisSififusManager::onUnitSurvivesCombat(Humano& victim, int32_t attackerId, int16_t attackerRelId,
                                                float damageTaken, int currentYear, EventLine* eventLine,
                                                ReligionRegistry* registry) {
    (void)registry;
    if (!victim.estaVivo() || damageTaken < 15.0f) return;

    Nemesis::NemesisProfile& prof = m_pool.promoteOrEmplace(victim.getId());
    prof.combatProwess *= 1.15f;
    prof.grudgeFaction = std::clamp(prof.grudgeFaction + 0.35f, -1.0f, 1.0f);
    prof.nemesisTargetId = attackerId;

    uint8_t severity = static_cast<uint8_t>(std::clamp(damageTaken * 2.2f, 10.0f, 255.0f));
    prof.memory.recordEvent(Nemesis::MemoryEventType::SurvivedCombat, currentYear,
                            attackerId, attackerRelId, severity, 0.35f);

    Nemesis::NemesisRank prevRank = prof.rank;
    if (prof.rank == Nemesis::NemesisRank::Peasant) {
        prof.rank = Nemesis::NemesisRank::Survivor;
        prof.generateProceduralEpithet(Nemesis::MemoryEventType::SurvivedCombat);
    } else if (prof.rank == Nemesis::NemesisRank::Survivor && prof.memory.getEventCount() >= 2) {
        prof.rank = Nemesis::NemesisRank::Avenger;
        prof.generateProceduralEpithet(Nemesis::MemoryEventType::SurvivedCombat);
    } else if (prof.rank == Nemesis::NemesisRank::Avenger && prof.memory.getEventCount() >= 4) {
        prof.rank = Nemesis::NemesisRank::Warlord;
        prof.generateProceduralEpithet(Nemesis::MemoryEventType::SurvivedCombat);
        victim.promoverAFanatico();
    }

    if (prof.rank != prevRank && eventLine) {
        Point2i loc{victim.getCeldaX(), victim.getCeldaY()};
        std::string desc = victim.getNombre() + " asciende a " + std::string(prof.epithet) +
                           " tras sobrevivir a un sangriento duelo.";
        eventLine->addEvent(currentYear, EventCategory::BorderConflict,
                            "Ascenso Némesis: " + std::string(prof.epithet),
                            desc, loc, victim.getReligionId(), attackerRelId,
                            ColorRGB{220, 80, 50}, true, victim.getNombre(), "Fervor bélico inflamado");
    }

    // Mitogénesis en Sísifo
    if (prof.rank >= Nemesis::NemesisRank::Survivor) {
        auto latestEv = prof.memory.getLatestEvent();
        if (latestEv) {
            Sififus::MythRecord myth = Sififus::NemesisSififusBridge::generateMythFromNemesis(
                prof, *latestEv, victim.getNombre().c_str(), m_nextMythId++);
            myth.originReligionId = static_cast<int16_t>(victim.getReligionId());
            m_sififus.registerMyth(myth);
        }
    }
}

void NemesisSififusManager::onUnitHitByDivinePower(Humano& victim, float damageTaken, int currentYear,
                                                   EventLine* eventLine, ReligionRegistry* registry) {
    if (!victim.estaVivo() || damageTaken < 20.0f) return;

    Nemesis::NemesisProfile& prof = m_pool.promoteOrEmplace(victim.getId());
    prof.zealotry *= 1.30f;
    prof.grudgePlayer = std::clamp(prof.grudgePlayer + 0.65f, -1.0f, 1.0f);

    uint8_t severity = static_cast<uint8_t>(std::clamp(damageTaken * 2.5f, 20.0f, 255.0f));
    prof.memory.recordEvent(Nemesis::MemoryEventType::WoundedByDivinePower, currentYear,
                            -1, -1, severity, 0.65f);

    Nemesis::NemesisRank prevRank = prof.rank;
    if (prof.rank < Nemesis::NemesisRank::Avenger) {
        prof.rank = Nemesis::NemesisRank::Avenger;
        prof.generateProceduralEpithet(Nemesis::MemoryEventType::WoundedByDivinePower);
    } else if (prof.rank == Nemesis::NemesisRank::Avenger && prof.grudgePlayer > 0.6f) {
        prof.rank = Nemesis::NemesisRank::Heresiarch;
        prof.personality = Nemesis::NemesisPersonality::ApostateReformer;
        prof.generateProceduralEpithet(Nemesis::MemoryEventType::Excommunicated);

        // Disparar Cisma Doctrinal emergente si la religión existe
        if (registry) {
            Point2i loc{victim.getCeldaX(), victim.getCeldaY()};
            int newRelId = registry->createSchismReligion(victim.getReligionId(), currentYear, loc);
            if (newRelId > 0) {
                victim.setReligionId(newRelId);
                victim.setDevocion(1.0f);
            }
        }
    }

    if (prof.rank != prevRank && eventLine) {
        Point2i loc{victim.getCeldaX(), victim.getCeldaY()};
        std::string desc = victim.getNombre() + " sobrevive al castigo del cielo y es ahora conocido como " +
                           std::string(prof.epithet);
        eventLine->addEvent(currentYear, EventCategory::Discovery,
                            "Némesis Divino: " + std::string(prof.epithet),
                            desc, loc, victim.getReligionId(), 0,
                            ColorRGB{255, 120, 40}, true, victim.getNombre(), "Mito de desafío celestial");
    }

    // Mitogénesis en Sísifo
    auto latestEv = prof.memory.getLatestEvent();
    if (latestEv) {
        Sififus::MythRecord myth = Sififus::NemesisSififusBridge::generateMythFromNemesis(
            prof, *latestEv, victim.getNombre().c_str(), m_nextMythId++);
        myth.originReligionId = static_cast<int16_t>(victim.getReligionId());
        m_sififus.registerMyth(myth);
    }
}

void NemesisSififusManager::onUnitSparedOrBlessed(Humano& unit, int currentYear, EventLine* eventLine,
                                                 ReligionRegistry* registry) {
    (void)registry;
    if (!unit.estaVivo()) return;

    Nemesis::NemesisProfile& prof = m_pool.promoteOrEmplace(unit.getId());
    prof.grudgePlayer = -0.90f; // Máxima lealtad y devoción
    prof.personality = Nemesis::NemesisPersonality::FanaticDevotee;
    prof.eloquence *= 1.45f;
    prof.rank = Nemesis::NemesisRank::LivingLegend;
    prof.generateProceduralEpithet(Nemesis::MemoryEventType::SparedByPlayer);

    prof.memory.recordEvent(Nemesis::MemoryEventType::SparedByPlayer, currentYear,
                            -1, -1, 180, -0.90f);

    unit.setDevocion(1.0f);
    unit.triggerFaithIndicator(+1, 3.0f);

    if (eventLine) {
        Point2i loc{unit.getCeldaX(), unit.getCeldaY()};
        std::string desc = unit.getNombre() + " ha presenciado la gracia celestial: " + std::string(prof.epithet);
        eventLine->addEvent(currentYear, EventCategory::ReligiousReformation,
                            "Testigo de Gracia: " + std::string(prof.epithet),
                            desc, loc, unit.getReligionId(), 0,
                            ColorRGB{240, 210, 80}, true, unit.getNombre(), "Fervor y conversión multitudinaria");
    }

    auto latestEv = prof.memory.getLatestEvent();
    if (latestEv) {
        Sififus::MythRecord myth = Sififus::NemesisSififusBridge::generateMythFromNemesis(
            prof, *latestEv, unit.getNombre().c_str(), m_nextMythId++);
        myth.originReligionId = static_cast<int16_t>(unit.getReligionId());
        m_sififus.registerMyth(myth);
    }
}

void NemesisSififusManager::update(float dtYears, int currentYear, GestorHumanos& gestor, WorldMap& map,
                                  ReligionRegistry* registry, EventLine* eventLine) {
    (void)registry;
    m_sififus.setEventLine(eventLine);
    m_sififus.update(dtYears, currentYear);

    // 1. Emanación activa de Unidades Notables sobre el mapa y devoción
    auto& profiles = m_pool.getDenseProfiles();
    for (size_t i = 0; i < profiles.size(); ++i) {
        auto& prof = profiles[i];
        Humano* unit = nullptr;
        for (auto& h : gestor.getHumanos()) {
            if (h.getId() == prof.entityId) {
                unit = &h;
                break;
            }
        }

        if (!unit || !unit->estaVivo()) {
            continue;
        }

        int cx = unit->getCeldaX();
        int cy = unit->getCeldaY();
        if (map.isValid(cx, cy)) {
            Cell& c = map.getCell(cx, cy);

            // Herejes o Vengadores desgastan la fe ortodoxa e impulsan el sincretismo y la tensión de frontera
            if (prof.grudgePlayer > 0.30f) {
                float corrup = prof.eloquence * 0.05f * dtYears;
                c.devotion = std::max(0.0f, c.devotion - corrup);
                if (prof.rank >= Nemesis::NemesisRank::Heresiarch) {
                    c.syncreticTension = std::min(1.0f, c.syncreticTension + 0.10f * dtYears);
                    c.borderTension = std::min(1.0f, c.borderTension + 0.08f * dtYears);
                }
            }
            // Mártires santos / Devotos de gracia irradian fe
            else if (prof.grudgePlayer < -0.30f) {
                float insp = prof.eloquence * 0.12f * dtYears;
                c.devotion = std::min(1.0f, c.devotion + insp);
            }
        }

        // Modificador de atributos del notable
        unit->setVelocidad(std::clamp(0.65f * prof.combatProwess, 0.40f, 1.40f));
    }

    // 2. Aplicar el Ethos de Sísifo a los actos de todos los fieles vivos
    for (auto& h : gestor.getHumanos()) {
        if (!h.estaVivo() || h.isRagdoll()) continue;

        Sififus::EthosVector ethos = m_sififus.getCollectiveEthos(static_cast<int16_t>(h.getReligionId()));

        // Actos: Devoción y Oración vs Materialismo
        if (ethos.devotionPriority > 0.30f) {
            h.setSpiritualHunger(std::min(100.0f, h.getSpiritualHunger() + ethos.devotionPriority * 6.0f * dtYears));
            if (h.getAccion() == AccionHumano::IDLE) {
                h.cambiarAccion(AccionHumano::REZAR);
            }
        } else if (ethos.devotionPriority < -0.30f) {
            h.setSpiritualHunger(0.0f);
        }

        // Actos: Incursiones y Beligerancia
        if (ethos.aggressionBias > 0.50f && h.getHumanUnit().role == CommunityRole::FORAGER) {
            h.promoverAFanatico();
        }

        // Actos: Martirio (valentía fanática)
        if (ethos.martyrdomDrive > 0.40f) {
            h.setPanicTimer(std::max(0.0f, h.getPanicTimer() - dtYears * 5.0f));
        }

        // Actos: Temor Divino
        if (ethos.divineFearBias > 0.60f && h.getPanicTimer() > 0.0f) {
            h.setPanicTimer(h.getPanicTimer() + dtYears * 2.0f);
        }
    }
}

const char* NemesisSififusManager::getNotableEpithetOrNull(int32_t entityId) const {
    const Nemesis::NemesisProfile* p = m_pool.get(entityId);
    return p ? p->epithet : nullptr;
}

const Nemesis::NemesisProfile* NemesisSififusManager::getProfileOrNull(int32_t entityId) const {
    return m_pool.get(entityId);
}

} // namespace ReligionSim
