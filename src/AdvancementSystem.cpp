#include "AdvancementSystem.hpp"
#include <algorithm>

namespace ReligionSim {

AdvancementSystem::AdvancementSystem() {
    m_doctrineCatalog = {
        {DoctrineType::AnimistSpirits, "Animist Communion", "Spirits animate the rivers, winds and stones.",
         HistoricalEra::PrimordialAnimism, 0, 1.0f, 1.2f, 1.0f},

        {DoctrineType::SacredSacrifice, "Totemic Ancestor Rituals", "Clan totems and relics appease ancestral spirits.",
         HistoricalEra::TotemismAncestorWorship, 50, 1.15f, 0.9f, 1.1f},

        {DoctrineType::AsceticMonasticism, "Shamanic Megaliths & Harvest", "Trance states, soil fertility rituals, and stone megaliths.",
         HistoricalEra::StructuralShamanism, 100, 1.25f, 1.1f, 1.15f},

        {DoctrineType::PantheonPolytheism, "Territorial Pantheon", "Ziggurats, city-state pantheons, and temple tithing.",
         HistoricalEra::TerritorialPolytheism, 200, 1.35f, 1.2f, 1.1f},

        {DoctrineType::ImperialDivineRight, "Sacred Sacerdotal Hierarchy", "King-priests and codified ritual law govern the state.",
         HistoricalEra::TerritorialPolytheism, 350, 1.45f, 0.8f, 1.3f},

        {DoctrineType::WrittenCanon, "Universal Holy Scripture", "Single codified truth in scripture for personal salvation.",
         HistoricalEra::UniversalReligions, 550, 1.6f, 0.7f, 1.4f},

        {DoctrineType::ProselytizingZeal, "Universal Mission & Pilgrimage", "Monastic orders and pilgrims evangelize continental realms.",
         HistoricalEra::UniversalReligions, 800, 1.85f, 0.5f, 1.3f},

        {DoctrineType::SyncreticOpenness, "Ecumenical Fusion", "Dialogue and assimilation of syncretic pantheons.",
         HistoricalEra::UniversalReligions, 1050, 1.4f, 1.8f, 1.0f},

        {DoctrineType::PrintingEvangelism, "Print-Era Dissemination", "Movable type multiplies tracts, igniting reformations.",
         HistoricalEra::RationalizationSecularization, 1400, 2.2f, 0.7f, 0.9f},

        {DoctrineType::SecularHumanism, "Rationality & Secular Morality", "Scientific inquiry and ethical conscience in secular nation-states.",
         HistoricalEra::RationalizationSecularization, 1800, 1.7f, 1.6f, 1.2f},

        {DoctrineType::ScholasticTheology, "Post-Secular Micro-Cults", "Customized cyber-spirituality, wellness cults, and viral networks.",
         HistoricalEra::PostSecular, 2400, 2.0f, 1.7f, 0.9f},

        {DoctrineType::CosmicTranscendence, "Post-Biological Singularity", "Quantum computing consciousness, mind uploads, and cyber-faith.",
         HistoricalEra::TranshumanistPostBiological, 3200, 2.6f, 1.5f, 1.5f}
    };

    // Árbol Teológico de Avances de la Fe: 16 avances distribuidos en 4 Eras
    m_advancementTree = {
        // =========================================================================
        // ERA 0: ANIMISMO PRIMORDIAL (-4000 a -3000 BCE)
        // =========================================================================
        {
            101,
            "Mito del Fuego Sagrado",
            "Mito del Fuego",
            "Relato ancestral del primer pacto divino alrededor de las brasas.",
            "+15% Velocidad de replicacion (Zeal) y +10% difusion espacial.",
            HistoricalEra::PrimordialAnimism,
            FaithBranch::ReplicationSpread,
            0, // Tier 0 (Raíz)
            60, // Fe
            {}, // Sin prerrequisitos
            false, HistoricalEra::PrimordialAnimism,
            4, // Icono: Fuego Sagrado
            0.15f, 0.10f, 0.0f, 0.0f, 0.0f, 1
        },
        {
            103,
            "Rito de Fertilidad de la Tierra",
            "Rito Fertilidad",
            "Danza de las estaciones y ofrendas para bendecir los suelos y cosechas.",
            "+20% Limite poblacional y mayor natalidad de aldeanos creyentes.",
            HistoricalEra::PrimordialAnimism,
            FaithBranch::HarvestDemographics,
            0, // Tier 0 (Raíz)
            70, // Fe
            {}, // Sin prerrequisitos
            false, HistoricalEra::PrimordialAnimism,
            1, // Icono: Espiga de trigo
            0.05f, 0.0f, 0.0f, 0.20f, 0.0f, 1
        },
        {
            105,
            "Culto a los Ancestros y Reliquias",
            "Culto Ancestros",
            "Preservacion de huesos y talismanes de los primeros sabios tribales.",
            "+25% Cohesion dogmatica. Reduce la perdida de fe ante la muerte de aldeanos.",
            HistoricalEra::PrimordialAnimism,
            FaithBranch::DogmaCohesion,
            0, // Tier 0 (Raíz)
            80, // Fe
            {}, // Sin prerrequisitos
            false, HistoricalEra::PrimordialAnimism,
            5, // Icono: Escudo relicario
            0.0f, 0.0f, 0.25f, 0.0f, 0.0f, 1
        },
        {
            100,
            "Santuario Megalitico y Comunion",
            "Hito: Megalitos",
            "Ereccion de menhires megaliticos y consagracion del primer circulo sagrado.",
            "¡ASCENSION HISTORICA! Avanza la civilizacion a la Era 1: Chamanismo Estructural.",
            HistoricalEra::PrimordialAnimism,
            FaithBranch::EraCapstone,
            1, // Tier 1 (Hito de Era)
            180, // Fe
            {101, 103}, // Requiere al menos Mito del Fuego o Rito de Fertilidad
            true, HistoricalEra::StructuralShamanism,
            6, // Icono: Corona / Hito
            0.10f, 0.15f, 0.15f, 0.10f, 0.30f, 3
        },

        // =========================================================================
        // ERA 1: CHAMANISMO ESTRUCTURAL (-3000 a -1000 BCE)
        // =========================================================================
        {
            201,
            "Caravanas Nomadas de la Fe",
            "Caravanas de Fe",
            "Expediciones de fieles que llevan iconos sagrados a traves de estepas y bosques.",
            "Desbloquea Caravanas Misioneras con +35% de velocidad y mayor rango.",
            HistoricalEra::StructuralShamanism,
            FaithBranch::ReplicationSpread,
            1,
            250,
            {100, 101},
            false, HistoricalEra::StructuralShamanism,
            0, // Icono: Paloma misionera
            0.15f, 0.25f, 0.0f, 0.0f, 0.25f, 3
        },
        {
            203,
            "Graneros Consagrados y Festines",
            "Graneros Sagrados",
            "Almacenamiento comunitario bendecido que garantiza alimento en epocas duras.",
            "+25% Desarrollo en celdas dominantes; aldeanos trabajan con mayor vigor.",
            HistoricalEra::StructuralShamanism,
            FaithBranch::HarvestDemographics,
            1,
            260,
            {100, 103},
            false, HistoricalEra::StructuralShamanism,
            1, // Icono: Espiga
            0.0f, 0.0f, 0.10f, 0.25f, 0.15f, 3
        },
        {
            205,
            "Circulo Chamanico y Tabues",
            "Circulo Chamanico",
            "Castas de sabios que fijan prohibiciones rituales y protegen la fe comunal.",
            "-50% Probabilidad de cismas hereticos; chamanes obtienen +30 HP en combate espiritual.",
            HistoricalEra::StructuralShamanism,
            FaithBranch::DogmaCohesion,
            1,
            280,
            {100, 105},
            false, HistoricalEra::StructuralShamanism,
            5, // Icono: Escudo
            0.0f, 0.05f, 0.30f, 0.0f, 0.10f, 3
        },
        {
            200,
            "Cuspide del Zigurat y Panteon",
            "Hito: Zigurat",
            "Monumento escalonado que une el cielo y la tierra bajo una jerarquia sagrada.",
            "¡ASCENSION HISTORICA! Avanza la civilizacion a la Era 2: Politeismo Territorial.",
            HistoricalEra::StructuralShamanism,
            FaithBranch::EraCapstone,
            2,
            480,
            {201, 203},
            true, HistoricalEra::TerritorialPolytheism,
            6, // Icono: Corona
            0.15f, 0.20f, 0.20f, 0.20f, 0.50f, 6
        },

        // =========================================================================
        // ERA 2: POLITEISMO TERRITORIAL (-1000 BCE a 200 CE)
        // =========================================================================
        {
            301,
            "Rutas Continentales de Peregrinaje",
            "Peregrinaje Continental",
            "Caminos empedrados custodiados por fieles para atraer creyentes extranjeros.",
            "+50% Frecuencia de Caravanas Misioneras y eliminacion del desgaste por distancia.",
            HistoricalEra::TerritorialPolytheism,
            FaithBranch::ReplicationSpread,
            2,
            650,
            {200, 201},
            false, HistoricalEra::TerritorialPolytheism,
            0, // Icono: Paloma
            0.25f, 0.35f, 0.10f, 0.0f, 0.30f, 5
        },
        {
            302,
            "Acueductos Benditos y Ciudades-Templo",
            "Ciudades-Templo",
            "Grandes urbes construidas alrededor del Templo Mayor con canales sagrados.",
            "+40% Capacidad maxima poblacional y generacion de +5 Fe anual pasiva.",
            HistoricalEra::TerritorialPolytheism,
            FaithBranch::HarvestDemographics,
            2,
            700,
            {200, 203},
            false, HistoricalEra::TerritorialPolytheism,
            3, // Icono: Templo
            0.05f, 0.15f, 0.10f, 0.40f, 0.50f, 6
        },
        {
            303,
            "Codice Escrito y Ley Canonica",
            "Codice Canonico",
            "Leyes sagradas grabadas en papiro y tablas que rigen a toda la nacion teocratica.",
            "Inmunidad casi total a cismas (+40% Cohesion); los dogmas se transmiten intactos.",
            HistoricalEra::TerritorialPolytheism,
            FaithBranch::DogmaCohesion,
            2,
            750,
            {200, 205},
            false, HistoricalEra::TerritorialPolytheism,
            2, // Icono: Codice
            0.10f, 0.10f, 0.40f, 0.0f, 0.20f, 6
        },
        {
            300,
            "Revelacion del Dogma Universal",
            "Hito: Dogma Universal",
            "Unificacion teologica definitiva proclamada para redimir a la humanidad entera.",
            "¡ASCENSION HISTORICA! Avanza la civilizacion a la Era 3: Religiones Universales.",
            HistoricalEra::TerritorialPolytheism,
            FaithBranch::EraCapstone,
            3,
            1100,
            {301, 303},
            true, HistoricalEra::UniversalReligions,
            7, // Icono: Trascendencia / Ojo
            0.30f, 0.40f, 0.35f, 0.25f, 0.70f, 10
        },

        // =========================================================================
        // ERA 3: RELIGIONES UNIVERSALES (200 a 1500 CE)
        // =========================================================================
        {
            401,
            "Evangelizacion Ecumenica Global",
            "Evangelizacion Global",
            "Misioneros infatigables cruzan oceanos y montanas llevando la palabra sagrada.",
            "+80% Replicacion global de la fe y asimilacion pacifica de cultos menores.",
            HistoricalEra::UniversalReligions,
            FaithBranch::ReplicationSpread,
            3,
            1400,
            {300, 301},
            false, HistoricalEra::UniversalReligions,
            0, // Icono: Paloma
            0.40f, 0.50f, 0.15f, 0.0f, 0.40f, 12
        },
        {
            402,
            "Caridad Institucional y Sanacion",
            "Caridad y Sanacion",
            "Hospicios y monasterios donde la devocion sana a los enfermos y nutre al necesitado.",
            "Aldeanos creyentes recuperan salud constantemente; reduce la mortalidad al minimo.",
            HistoricalEra::UniversalReligions,
            FaithBranch::HarvestDemographics,
            3,
            1500,
            {300, 302},
            false, HistoricalEra::UniversalReligions,
            1, // Icono: Espiga
            0.10f, 0.20f, 0.20f, 0.40f, 0.50f, 12
        },
        {
            403,
            "Teologia Escolastica y Dogma Eterno",
            "Teologia Escolastica",
            "Sintesis suprema de razon y fe que disipa cualquier duda o herejia.",
            "Inmunidad absoluta contra cismas doctrinales y +20 Fe anual pasiva.",
            HistoricalEra::UniversalReligions,
            FaithBranch::DogmaCohesion,
            3,
            1600,
            {300, 303},
            false, HistoricalEra::UniversalReligions,
            2, // Icono: Codice
            0.15f, 0.20f, 0.50f, 0.0f, 0.30f, 15
        },
        {
            400,
            "Trascendencia Mistica y Apogeo Sagrado",
            "Hito: Trascendencia",
            "Comunion total entre la divinidad y la humanidad, alcanzando la inmortalidad del alma.",
            "¡APOGEO TEOLOGICO! Consagra la hegemonia espiritual eterna de tu pueblo.",
            HistoricalEra::UniversalReligions,
            FaithBranch::EraCapstone,
            4,
            2500,
            {401, 402, 403},
            true, HistoricalEra::RationalizationSecularization,
            7, // Icono: Estrella cosmica
            0.50f, 0.60f, 0.50f, 0.50f, 1.00f, 25
        }
    };
}

const EraConfig& AdvancementSystem::getEraConfig(HistoricalEra era) {
    auto idx = static_cast<std::size_t>(era);
    if (idx >= static_cast<std::size_t>(HistoricalEra::Count)) {
        idx = 0;
    }
    return HISTORICAL_ERAS_TABLE[idx];
}

std::string AdvancementSystem::getEraName(HistoricalEra era) {
    return std::string(getEraConfig(era).name);
}

std::string AdvancementSystem::getEraDescription(HistoricalEra era) {
    const auto& cfg = getEraConfig(era);
    return std::string(cfg.faith_concept) + " | Amenaza: " + std::string(cfg.primary_threat);
}

HistoricalEra AdvancementSystem::getEraForYear(int year) {
    if (year < -3000) return HistoricalEra::PrimordialAnimism;             // Era 1: -4000 to -3000
    if (year < -2000) return HistoricalEra::TotemismAncestorWorship;        // Era 2: -3000 to -2000
    if (year < -1000) return HistoricalEra::StructuralShamanism;           // Era 3: -2000 to -1000
    if (year < 200)   return HistoricalEra::TerritorialPolytheism;         // Era 4: -1000 to 200
    if (year < 1500)  return HistoricalEra::UniversalReligions;            // Era 5: 200 to 1500
    if (year < 1980)  return HistoricalEra::RationalizationSecularization; // Era 6: 1500 to 1980
    if (year < 2060)  return HistoricalEra::PostSecular;                   // Era 7: 1980 to 2060
    return HistoricalEra::TranshumanistPostBiological;                      // Era 8: 2060+
}

// EN: Return the canonical starting year of each historical era
// ES: Devolver el año inicial canónico de cada era histórica
int AdvancementSystem::getEraStartYear(HistoricalEra era) {
    switch (era) {
        case HistoricalEra::PrimordialAnimism:             return -4000;
        case HistoricalEra::TotemismAncestorWorship:        return -3000;
        case HistoricalEra::StructuralShamanism:           return -2000;
        case HistoricalEra::TerritorialPolytheism:         return -1000;
        case HistoricalEra::UniversalReligions:            return 200;
        case HistoricalEra::RationalizationSecularization: return 1500;
        case HistoricalEra::PostSecular:                   return 1980;
        case HistoricalEra::TranshumanistPostBiological:   return 2060;
        default: return -4000;
    }
}

// EN: Return total count of advancements defined in the faith tree for a specific era
// ES: Devolver la cantidad total de avances definidos en el árbol de fe para una era específica
int AdvancementSystem::getTotalAdvancementsInEra(HistoricalEra era) const {
    int totalAdvancementsCount = 0;
    for (const auto& advancementItem : m_advancementTree) {
        if (advancementItem.era == era) {
            totalAdvancementsCount++;
        }
    }
    return totalAdvancementsCount > 0 ? totalAdvancementsCount : 4;
}

const AdvancementDoc* AdvancementSystem::getDocInfo(DoctrineType doc) const {
    for (const auto& item : m_doctrineCatalog) {
        if (item.type == doc) return &item;
    }
    return nullptr;
}

const FaithAdvancement* AdvancementSystem::getAdvancement(int id) const {
    for (const auto& adv : m_advancementTree) {
        if (adv.id == id) return &adv;
    }
    return nullptr;
}

bool AdvancementSystem::canConvalidate(const Religion& rel, int advId) const {
    if (rel.hasAdvancement(advId)) return false; // Ya convalidado
    const FaithAdvancement* adv = getAdvancement(advId);
    if (!adv) return false;

    // Verificar Fe / Piedad suficiente
    if (rel.theologyPoints < adv->faithCost) return false;

    // Verificar que cumpla todos los prerrequisitos
    for (int reqId : adv->prerequisites) {
        if (!rel.hasAdvancement(reqId)) {
            return false;
        }
    }

    // Para hitos de era, no ascender si ya se encuentra en o por encima de esa era
    if (adv->isEraCapstone) {
        if (static_cast<int>(adv->unlocksEra) <= static_cast<int>(rel.currentEra)) {
            return false;
        }
    }

    return true;
}

bool AdvancementSystem::convalidateAdvancement(Religion& rel, int advId, int currentYear,
                                              std::vector<GameEvent>& outEvents) {
    if (!canConvalidate(rel, advId)) return false;
    const FaithAdvancement* adv = getAdvancement(advId);
    if (!adv) return false;

    // Descontar costo en puntos de fe / piedad
    rel.theologyPoints -= adv->faithCost;
    rel.addAdvancement(advId);

    // Aplicar mejoras acumulativas a la replicación y estabilidad de la fe
    rel.zeal = std::clamp(rel.zeal + adv->zealBonus, 0.1f, 1.0f);
    rel.cohesion = std::clamp(rel.cohesion + adv->cohesionBonus, 0.1f, 1.0f);
    rel.diffusionMultiplier += adv->diffusionBonus;
    rel.harvestYieldBonus += adv->popGrowthBonus;
    rel.holySitePowerMultiplier += adv->holySiteBonus;
    rel.passiveFaithBonusPerYear += adv->passiveFaithPerYear;

    // Generar evento sagrado
    GameEvent ev;
    ev.year = currentYear;
    ev.location = {0, 0};
    ev.religionAId = rel.id;
    ev.badgeColor = rel.color;

    if (adv->isEraCapstone) {
        // Salto de Era formal
        rel.currentEra = adv->unlocksEra;
        rel.advancementLevel = static_cast<int>(adv->unlocksEra) + 1;

        ev.category = EventCategory::EraAdvancement;
        ev.isTurningPoint = true;
        ev.title = rel.name + " Asciende a " + getEraName(adv->unlocksEra);
        ev.description = "Mediante el hito '" + adv->name + "', los fieles de " + rel.name +
                         " han transformado su fe y cruzado a una nueva Era: " + getEraDescription(adv->unlocksEra);
        ev.actorName = "Pueblo Sagrado de " + rel.name;
        ev.consequence = "Salto de Era civilizatoria y desbloqueo de doctrinas teologicas superiores";
    } else {
        ev.category = EventCategory::Discovery;
        ev.isTurningPoint = (adv->tier >= 2);
        ev.title = "Doctrina Convalidada: " + adv->name;
        ev.description = "La fe de " + rel.name + " ha integrado la doctrina sagrada '" + adv->name + "'. " + adv->description;
        ev.actorName = "Consejo Liturgico de " + rel.name;
        ev.consequence = adv->mechanicalBonus;
    }

    outEvents.push_back(ev);
    return true;
}

int AdvancementSystem::getConvalidatedCountForEra(const Religion& rel, HistoricalEra era) const {
    int count = 0;
    for (const auto& adv : m_advancementTree) {
        if (adv.era == era && rel.hasAdvancement(adv.id)) {
            count++;
        }
    }
    return count;
}

void AdvancementSystem::updateAdvancements(ReligionRegistry& registry, int currentYear,
                                          std::vector<GameEvent>& outEvents) {
    EraType worldEra = getEraForYear(currentYear);

    for (auto& r : registry.getAllReligions()) {
        if (r.isExtinct) continue;

        // Puntos de teología/fe generados por seguidores, lugares sagrados y escolasticismo
        float pointGain = (r.totalFollowers * 0.0005f) +
                          (r.holySitesCount * 4.0f) +
                          (r.scholasticism * 10.0f) + 1.0f;
        r.theologyPoints += static_cast<int>(pointGain);

        // Bonificación pasiva de fe proveniente de avances convalidados
        r.theologyPoints += r.passiveFaithBonusPerYear;

        // Transición temporal de Era mundial si el tiempo histórico supera la era actual
        if (static_cast<int>(worldEra) > static_cast<int>(r.currentEra)) {
            r.currentEra = worldEra;
            r.advancementLevel = static_cast<int>(worldEra) + 1;

            GameEvent ev;
            ev.year = currentYear;
            ev.category = EventCategory::EraAdvancement;
            ev.religionAId = r.id;
            ev.badgeColor = r.color;
            ev.title = r.name + " Entra en " + getEraName(worldEra);
            ev.description = "Los fieles de " + r.name + " han cruzado a una nueva epoca: " + getEraDescription(worldEra);
            ev.isTurningPoint = true;
            ev.actorName = "Pueblo de " + r.name;
            ev.consequence = "Salto de Era civilizatorio y transformacion cultural";
            outEvents.push_back(ev);
        }

        // Si la religión es controlada por la IA (ID != 1), evaluar convalidar avances autónomamente
        if (r.id != 1) {
            for (const auto& adv : m_advancementTree) {
                if (canConvalidate(r, adv.id) && r.theologyPoints >= static_cast<int>(adv.faithCost * 1.25f)) {
                    convalidateAdvancement(r, adv.id, currentYear, outEvents);
                    break; // Un avance por ciclo para progresión gradual
                }
            }
        }

        // Compatibilidad con catálogo de doctrinas heredadas
        for (const auto& doc : m_doctrineCatalog) {
            if (static_cast<int>(doc.era) <= static_cast<int>(r.currentEra)) {
                if (!r.hasDoctrine(doc.type) && r.theologyPoints >= doc.theologyCost) {
                    r.addDoctrine(doc.type);
                    r.zeal = std::clamp(r.zeal * doc.spreadModifier, 0.1f, 1.0f);
                    r.syncretismTolerance = std::clamp(r.syncretismTolerance * doc.syncretismModifier, 0.05f, 1.0f);
                    r.cohesion = std::clamp(r.cohesion * doc.cohesionModifier, 0.1f, 1.0f);
                    r.scholasticism = std::min(1.0f, r.scholasticism + 0.1f);
                }
            }
        }
    }
}

} // namespace ReligionSim
