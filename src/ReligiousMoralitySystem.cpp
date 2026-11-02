#include "ReligiousMoralitySystem.hpp"
#include "Humano.hpp"
#include "Religion.hpp"
#include "EventLine.hpp"
#include "SoundSystem.hpp"
#include "IsometricCamera.hpp"
#include "Timeline.hpp"
#include <iostream>
#include <sstream>

namespace ReligionSim {

ReligiousMoralitySystem::ReligiousMoralitySystem() {
    initCatalog();
}

void ReligiousMoralitySystem::initCatalog() {
    m_catalog.clear();

    // 1. Tabú de Sangre Fraterna
    MoralRuleDefinition kin;
    kin.type = MoralRuleType::SacredKinProtection;
    kin.name = "Tabu de Sangre Fraterna";
    kin.commandment = "No derramaras la sangre de tus hermanos de fe; quien mate a un hermano manchara su linaje ante los dioses.";
    kin.virtueDescription = "+20% Cohesion comunitaria y serenidad social. Previene cismas fratricidas.";
    kin.sinDescription = "Agredir o asesinar a un hermano de clan (Incurre en +45 Culpa y remordimiento).";
    kin.theologyCost = 80;
    kin.requiredEra = HistoricalEra::PrimordialAnimism;
    kin.guiltSeverity = 45.0f;
    m_catalog.push_back(kin);

    // 2. Mandato de la Ofrenda Sagrada
    MoralRuleDefinition tithe;
    tithe.type = MoralRuleType::HolyOfferingTithe;
    tithe.name = "Mandato de la Ofrenda Sagrada";
    tithe.commandment = "Consagraras los primeros frutos de la cosecha, la caza y la piedra sobre el altar sagrado.";
    tithe.virtueDescription = "+50% Puntos de Teologia al entregar ofrendas; incrementa la Gracia Divina comunitaria.";
    tithe.sinDescription = "Atesorar recursos y descuidar las ofrendas en el templo (Pecado de Avaricia).";
    tithe.theologyCost = 100;
    tithe.requiredEra = HistoricalEra::TotemismAncestorWorship;
    tithe.guiltSeverity = 18.0f;
    m_catalog.push_back(tithe);

    // 3. Custodia de Santuarios
    MoralRuleDefinition sanc;
    sanc.type = MoralRuleType::SanctuaryDefense;
    sanc.name = "Custodia de Santuarios";
    sanc.commandment = "Los altares y templos son la morada terrenal de tu dios; defenderlos con la vida es deber sagrado.";
    sanc.virtueDescription = "+30% Ataque y resistencia en un radio de 5 celdas de santuarios y templos aliados.";
    sanc.sinDescription = "Permitir la profanacion o destruccion de un altar sagrado ante enemigos de la fe.";
    sanc.theologyCost = 120;
    sanc.requiredEra = HistoricalEra::TotemismAncestorWorship;
    sanc.guiltSeverity = 35.0f;
    m_catalog.push_back(sanc);

    // 4. Celo de Guerra Santa
    MoralRuleDefinition war;
    war.type = MoralRuleType::ZealousHolyWar;
    war.name = "Celo de Guerra Santa";
    war.commandment = "Combatir y purgar a los cultos sacrilegos e idolatras es senda de gloria y bendicion eterna.";
    war.virtueDescription = "+35% Dano en combate contra cultos rivales y +20% fervor en la conversion de herejes.";
    war.sinDescription = "Huir en cobardia ante enemigos de la fe o pactar treguas sin consagracion sacerdotal.";
    war.theologyCost = 150;
    war.requiredEra = HistoricalEra::StructuralShamanism;
    war.guiltSeverity = 30.0f;
    m_catalog.push_back(war);

    // 5. Voto de Ayuno y Templanza
    MoralRuleDefinition fast;
    fast.type = MoralRuleType::AsceticFasting;
    fast.name = "Voto de Ayuno y Templanza";
    fast.commandment = "Purificaras tu cuerpo con el ayuno sagrado; la carne sacia el vientre pero la templanza eleva el espiritu.";
    fast.virtueDescription = "La oracion mitiga el doble de Hambre Espiritual; otorga +40% resistencia ante pandemias.";
    fast.sinDescription = "Glotoneria y consumo desenfrenado de provisiones comunitarias en tiempos de afliccion.";
    fast.theologyCost = 140;
    fast.requiredEra = HistoricalEra::StructuralShamanism;
    fast.guiltSeverity = 20.0f;
    m_catalog.push_back(fast);

    // 6. Hospitalidad Sagrada
    MoralRuleDefinition hosp;
    hosp.type = MoralRuleType::SacredHospitality;
    hosp.name = "Hospitalidad Sagrada";
    hosp.commandment = "Acogeras al peregrino pacifico y comerciaras con rectitud; ningun huesped sera agraviado.";
    hosp.virtueDescription = "+40% Tolerancia sincretica; acelera la atraccion de caravanas y nuevos conversos pacíficos.";
    hosp.sinDescription = "Atacar a caravanas o extranjeros neutrales o violar salvoconductos sagrados.";
    hosp.theologyCost = 130;
    hosp.requiredEra = HistoricalEra::TerritorialPolytheism;
    hosp.guiltSeverity = 35.0f;
    m_catalog.push_back(hosp);

    // 7. Reverencia a los Ancestros
    MoralRuleDefinition anc;
    anc.type = MoralRuleType::AncestorVeneration;
    anc.name = "Reverencia a los Ancestros";
    anc.commandment = "Honraras los restos y la memoria de los caidos; ningun hermano quedara sin plegaria de transito.";
    anc.virtueDescription = "Los fieles rezan junto a caidos aliados fortaleciendo su devocion (+10%) y superando el panico.";
    anc.sinDescription = "Abandonar a los difuntos en el barro sin oracion ni honras fúnebres sagradas.";
    anc.theologyCost = 110;
    anc.requiredEra = HistoricalEra::PrimordialAnimism;
    anc.guiltSeverity = 25.0f;
    m_catalog.push_back(anc);

    // 8. Sumisión al Líder Divino
    MoralRuleDefinition sub;
    sub.type = MoralRuleType::HierarchicalSubmission;
    sub.name = "Sumision al Lider Divino";
    sub.commandment = "La palabra del Chaman o Profeta es la voz viva de tu dios; desobedecer es rebelarse contra el cielo.";
    sub.virtueDescription = "La devocion al lider se multiplica x2; los fanaticos son inmunes a doctrinas y cismas foraneos.";
    sub.sinDescription = "Desobedecer las ordenes sagradas o murmurar contra el liderazgo espiritual de la tribu.";
    sub.theologyCost = 160;
    sub.requiredEra = HistoricalEra::StructuralShamanism;
    sub.guiltSeverity = 40.0f;
    m_catalog.push_back(sub);
}

const MoralRuleDefinition* ReligiousMoralitySystem::getRuleDef(MoralRuleType type) const {
    for (const auto& r : m_catalog) {
        if (r.type == type) return &r;
    }
    return nullptr;
}

ReligionMoralProfile& ReligiousMoralitySystem::getOrCreateProfile(int religionId) {
    auto it = m_profiles.find(religionId);
    if (it != m_profiles.end()) {
        return it->second;
    }
    ReligionMoralProfile prof;
    prof.religionId = religionId;
    prof.communalGrace = 85.0f;
    // Por defecto, las religiones primordiales inician con el Tabú de Sangre Fraterna activo
    prof.activateRule(MoralRuleType::SacredKinProtection);
    auto res = m_profiles.emplace(religionId, prof);
    return res.first->second;
}

const ReligionMoralProfile* ReligiousMoralitySystem::getProfile(int religionId) const {
    auto it = m_profiles.find(religionId);
    if (it != m_profiles.end()) {
        return &(it->second);
    }
    return nullptr;
}

bool ReligiousMoralitySystem::enactRule(int religionId, MoralRuleType rule, ReligionRegistry& registry) {
    const auto* def = getRuleDef(rule);
    if (!def) return false;

    Religion* rel = registry.getReligion(religionId);
    if (!rel) return false;

    if (rel->theologyPoints < def->theologyCost) {
        return false; // Fe insuficiente
    }

    auto& prof = getOrCreateProfile(religionId);
    if (prof.hasRule(rule) || prof.activeRules.size() >= 4) {
        return false; // Ya activa o límite de 4 alcanzado
    }

    rel->theologyPoints -= def->theologyCost;
    prof.activateRule(rule);
    prof.communalGrace = std::min(100.0f, prof.communalGrace + 8.0f);

    std::cout << "[Moralidad] Religion #" << religionId << " (" << rel->name
              << ") ha promulgado el mandato: " << def->name << std::endl;
    return true;
}

bool ReligiousMoralitySystem::repealRule(int religionId, MoralRuleType rule) {
    auto& prof = getOrCreateProfile(religionId);
    if (!prof.hasRule(rule)) return false;

    prof.deactivateRule(rule);
    prof.communalGrace = std::max(10.0f, prof.communalGrace - 5.0f);

    std::cout << "[Moralidad] Religion #" << religionId
              << " ha derogado el mandato: " << getMoralRuleName(rule) << std::endl;
    return true;
}

bool ReligiousMoralitySystem::isRuleActive(int religionId, MoralRuleType rule) const {
    const auto* prof = getProfile(religionId);
    return prof && prof->hasRule(rule);
}

void ReligiousMoralitySystem::recordTransgression(Humano& unit, MoralRuleType violatedRule,
                                                 const std::string& detail, int currentYear,
                                                 EventLine* eventLine, ReligionRegistry* registry) {
    const auto* def = getRuleDef(violatedRule);
    float sev = def ? def->guiltSeverity : 25.0f;

    unit.getMoralRecordMutable().addGuilt(sev, detail, currentYear);
    unit.triggerFaithIndicator(-1, 2.0f);

    int rId = unit.getReligionId();
    auto& prof = getOrCreateProfile(rId);
    prof.totalTransgressionsCount++;
    prof.communalGrace = std::max(5.0f, prof.communalGrace - sev * 0.15f);

    std::cout << "[Transgresión Moral] " << unit.getNombre() << " (Rel #" << rId
              << ") violo " << getMoralRuleName(violatedRule) << ": " << detail
              << " (+ " << sev << " Culpa)" << std::endl;

    if (sev >= 35.0f && eventLine) {
        std::string relName = (registry && registry->getReligion(rId)) ? registry->getReligion(rId)->name : "la fe";
        GameEvent ev;
        ev.year = currentYear;
        ev.title = "SACRILEGIO MORAL EN " + relName;
        ev.description = unit.getNombre() + " cometio una grave transgresion (" + detail + ") quebrantando el " + getMoralRuleName(violatedRule) + ".";
        ev.category = EventCategory::ReligiousReformation;
        ev.location = {unit.getCeldaX(), unit.getCeldaY()};
        ev.religionAId = rId;
        ev.badgeColor = {210, 45, 55};
        ev.actorName = unit.getNombre();
        ev.consequence = "Mancha moral y afliccion en la Gracia Comunitaria";
        eventLine->addEvent(ev);
    }
}

void ReligiousMoralitySystem::recordRighteousAct(Humano& unit, MoralRuleType followedRule,
                                                float merit, int currentYear, ReligionRegistry* registry) {
    (void)followedRule;
    (void)currentYear;
    unit.getMoralRecordMutable().addRighteousAct(std::min(0.08f, merit * 0.005f));
    unit.triggerFaithIndicator(+1, 1.2f);

    int rId = unit.getReligionId();
    auto& prof = getOrCreateProfile(rId);
    prof.totalVirtuesCount++;
    prof.communalGrace = std::min(100.0f, prof.communalGrace + std::min(1.8f, merit * 0.04f));

    if (registry) {
        Religion* rel = registry->getReligion(rId);
        if (rel) {
            rel->theologyPoints += static_cast<int>(merit * 0.25f);
        }
    }
}

void ReligiousMoralitySystem::processPenance(Humano& unit, float dt, bool atHolySite, bool shamanNearby,
                                            ReligionRegistry* registry, SoundSystem* soundSystem,
                                            const IsometricCamera* camera) {
    if (!unit.estaVivo() || !unit.getMoralRecord().hasGuilt()) return;

    float absRate = (atHolySite ? 12.0f : 4.5f) * (shamanNearby ? 2.5f : 1.0f) * dt;
    unit.getMoralRecordMutable().absolvePenance(absRate);

    // Feedback visual y auditivo de purificación
    if (unit.getMoralRecord().guiltLevel <= 1.0f && soundSystem && camera) {
        soundSystem->playSpatialSFX(SFXType::FaithGain, unit.getX(), unit.getY(), *camera, 0.65f);
        unit.triggerFaithIndicator(+1, 2.0f);
    }

    if (registry && atHolySite) {
        Religion* rel = registry->getReligion(unit.getReligionId());
        if (rel) {
            rel->theologyPoints += static_cast<int>(1.5f * dt);
        }
    }
}

void ReligiousMoralitySystem::update(float dtYears, int currentYear, GestorHumanos& gestor,
                                    WorldMap& map, ReligionRegistry& registry, EventLine* eventLine) {
    (void)currentYear;
    (void)map;

    // 1. Evaluación periódica de fieles individuales por religión
    for (auto& [rId, prof] : m_profiles) {
        int livingCount = 0;
        int virtuousCount = 0;
        int guiltyCount = 0;
        int apostateCount = 0;

        for (auto& h : gestor.getHumanos()) {
            if (!h.estaVivo() || h.getReligionId() != rId) continue;
            livingCount++;

            const auto& mor = h.getMoralRecord();
            if (mor.isVirtuous()) virtuousCount++;
            if (mor.hasGuilt()) guiltyCount++;
            if (mor.isApostateOrHeretic) apostateCount++;

            // Si tiene voto de ayuno activo y no come en penuria, su oración purifica más
            if (prof.hasRule(MoralRuleType::AsceticFasting) && h.getAccion() == AccionHumano::REZAR) {
                h.setSpiritualHunger(std::max(0.0f, h.getSpiritualHunger() - 15.0f * (dtYears * 10.0f)));
            }
        }

        prof.totalApostatesCount = apostateCount;
        if (livingCount > 0) {
            prof.moralPurityRatio = static_cast<float>(virtuousCount) / static_cast<float>(livingCount);
        }

        // 2. Modulación de Gracia Comunitaria
        if (virtuousCount > guiltyCount && apostateCount == 0) {
            prof.communalGrace = std::min(100.0f, prof.communalGrace + 1.2f * (dtYears * 5.0f));
        } else if (apostateCount > 0 || guiltyCount > virtuousCount) {
            float decay = (static_cast<float>(apostateCount) * 1.5f + static_cast<float>(guiltyCount) * 0.4f);
            prof.communalGrace = std::max(5.0f, prof.communalGrace - decay * (dtYears * 5.0f));
        }

        // 3. Efectos de la Gracia Colectiva en la Teología de la Religión
        Religion* rel = registry.getReligion(rId);
        if (rel) {
            if (prof.getStatus() == MoralGraceStatus::DivineBlessing) {
                // Bendición: bonus pasivo de puntos de teología
                rel->theologyPoints += static_cast<int>(18.0f * dtYears);
                rel->cohesion = std::min(1.0f, rel->cohesion + 0.02f * dtYears);
            } else if (prof.getStatus() == MoralGraceStatus::MoralCrisis ||
                       prof.getStatus() == MoralGraceStatus::HereticalDecadence) {
                // Crisis: merma en cohesión y riesgo de cisma
                rel->cohesion = std::max(0.20f, rel->cohesion - 0.05f * dtYears);
            }
        }

        // 4. Alerta de crisis moral en la crónica
        if (prof.communalGrace < 20.0f && eventLine && (std::rand() % 40 == 0)) {
            GameEvent ev;
            ev.year = currentYear;
            ev.title = "CRISIS DE MORALIDAD Y APOSTASIA";
            ev.description = "El pueblo de " + (rel ? rel->name : "la fe") + " esta abrumado por transgresiones no expiadas. La Gracia Divina se ha oscurecido.";
            ev.category = EventCategory::ReligiousReformation;
            ev.location = {24, 24};
            ev.religionAId = rId;
            ev.badgeColor = {190, 30, 40};
            ev.isTurningPoint = true;
            eventLine->addEvent(ev);
        }
    }
}

float ReligiousMoralitySystem::getCommunalGrace(int religionId) const {
    const auto* prof = getProfile(religionId);
    return prof ? prof->communalGrace : 75.0f;
}

MoralGraceStatus ReligiousMoralitySystem::getMoralStatus(int religionId) const {
    const auto* prof = getProfile(religionId);
    return prof ? prof->getStatus() : MoralGraceStatus::Righteous;
}

std::string ReligiousMoralitySystem::serialize() const {
    std::ostringstream ss;
    ss << m_profiles.size() << "\n";
    for (const auto& [rId, prof] : m_profiles) {
        ss << rId << " " << prof.communalGrace << " "
           << prof.totalVirtuesCount << " " << prof.totalTransgressionsCount << " "
           << prof.activeRules.size();
        for (auto rule : prof.activeRules) {
            ss << " " << static_cast<int>(rule);
        }
        ss << "\n";
    }
    return ss.str();
}

void ReligiousMoralitySystem::deserialize(const std::string& data) {
    if (data.empty()) return;
    std::istringstream ss(data);
    size_t count = 0;
    if (!(ss >> count)) return;

    for (size_t i = 0; i < count; ++i) {
        int rId = 0;
        float grace = 80.0f;
        int virtues = 0, trans = 0;
        size_t rulesCount = 0;
        if (!(ss >> rId >> grace >> virtues >> trans >> rulesCount)) break;

        ReligionMoralProfile prof;
        prof.religionId = rId;
        prof.communalGrace = grace;
        prof.totalVirtuesCount = virtues;
        prof.totalTransgressionsCount = trans;

        for (size_t j = 0; j < rulesCount; ++j) {
            int rInt = 0;
            if (ss >> rInt) {
                prof.activateRule(static_cast<MoralRuleType>(rInt));
            }
        }
        m_profiles[rId] = prof;
    }
}

} // namespace ReligionSim
