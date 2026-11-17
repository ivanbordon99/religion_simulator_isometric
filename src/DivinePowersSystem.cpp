#include "DivinePowersSystem.hpp"
#include "WorldMap.hpp"
#include "Religion.hpp"
#include "Humano.hpp"
#include "BuildingSystem.hpp"
#include "ReplicationEngine.hpp"
#include <cmath>
#include <algorithm>
#include <random>
#include <iostream>

namespace ReligionSim {

namespace {
    std::mt19937 g_divineRng(1024);
}

DivinePowersSystem::DivinePowersSystem() {
}

int DivinePowersSystem::getPowerCost(DivinePowerType type) const {
    switch (type) {
        case DivinePowerType::Lightning:  return 25;
        case DivinePowerType::Plague:     return 35;
        case DivinePowerType::Miracle:    return 30;
        case DivinePowerType::Congregate: return 10;
        case DivinePowerType::Tornado:    return 40;
        case DivinePowerType::Earthquake: return 50;
        case DivinePowerType::Pandemic:   return 45;
        case DivinePowerType::Meteor:     return 60;
        case DivinePowerType::Volcano:    return 70;
        case DivinePowerType::Wildfire:   return 55;
        case DivinePowerType::Rain:       return 30;
        case DivinePowerType::Deluge:     return 65;
        case DivinePowerType::AtomicBomb: return 0; // EN: Cheat power with zero faith cost / ES: Poder truco con costo cero de fe
        default: return 0;
    }
}

std::string DivinePowersSystem::getPowerName(DivinePowerType type) const {
    switch (type) {
        case DivinePowerType::Lightning:  return "RAYO DIVINO";
        case DivinePowerType::Plague:     return "PLAGA SAGRADA";
        case DivinePowerType::Miracle:    return "MILAGRO CELESTIAL";
        case DivinePowerType::Congregate: return "CONGREGAR FIELES";
        case DivinePowerType::Tornado:    return "TORNADO DEVASTADOR";
        case DivinePowerType::Earthquake: return "TERREMOTO SISMICO";
        case DivinePowerType::Pandemic:   return "PANDEMIA VIRAL";
        case DivinePowerType::Meteor:     return "METEORITO CATACLISMICO";
        case DivinePowerType::Volcano:    return "ERUPCION VOLCANICA";
        case DivinePowerType::Wildfire:   return "INCENDIO FORESTAL";
        case DivinePowerType::Rain:       return "LLUVIA BENDITA";
        case DivinePowerType::Deluge:     return "GRAN DILUVIO";
        case DivinePowerType::AtomicBomb: return "EXPLOSION ATOMICA [FUCKINGBOMB]";
        default: return "";
    }
}

std::string DivinePowersSystem::getPowerDescription(DivinePowerType type) const {
    switch (type) {
        case DivinePowerType::Lightning:
            return "Descarga un rayo celestial sobre la celda objetivo. Inflige dano severo a infieles e inspira a tu intermediario.";
        case DivinePowerType::Plague:
            return "Desata un enjambre de langostas y peste sobre un area de 3x3 celdas. Arruina cosechas enemigas y debilita la fe ajena.";
        case DivinePowerType::Miracle:
            return "Derrama luz celestial y vida sobre un area de 3x3 celdas. Sana a los fieles heridos y fertiliza la tierra.";
        case DivinePowerType::Congregate:
            return "Tu intermediario convoca a todos los creyentes a reunirse en oracion comunal en torno al altar.";
        case DivinePowerType::Tornado:
            return "Engendra un ciclon movil con fisicas de vortice. Succiona aldeanos haciendolos volar por los aires, mientras en la periferia huyen despavoridos y los recursos son arrasados.";
        case DivinePowerType::Earthquake:
            return "Desata un sismo tectonico que sacude la pantalla y fractura el terreno. Hace tropezar y derriba aldeanos, causa panico masivo y dana la estructura de los templos.";
        case DivinePowerType::Pandemic:
            return "Desata un brote viral altamente contagioso. Se transmite directamente por proximidad de humano a humano con tos, cuarentena y gran mortandad.";
        case DivinePowerType::Meteor:
            return "Invoca un colosal bolido celeste en llamas. Atraviesa la atmosfera a velocidad hipersonica, vaporiza el epicentro, desata una onda de choque devastadora y deja un crater humeante.";
        case DivinePowerType::Volcano:
            return "Despierta un imponente volcan activo que surge de las entranas del mundo. Expulsa fuentes de magma incandescente, bombas volcanicas y rios de lava que calcinan la tierra y expanden la costa sobre el mar.";
        case DivinePowerType::Wildfire:
            return "Desata un abrasador fuego sagrado sobre bosques y arboledas. Las llamas se propagan de celda en celda, calcinan la madera y chozas, y hacen huir a los aldeanos aterrados del fuego.";
        case DivinePowerType::Rain:
            return "Derrama una bienhechora lluvia celestial. Apaga incendios, humedece el suelo, fertiliza la tierra calcinada y refresca a los fieles.";
        case DivinePowerType::Deluge:
            return "Desata un cataclismo biblico de tormenta torrencial y crecida de aguas. El agua inunda valles y depresiones, sofoca cualquier incendio instantaneamente con nubes de vapor, y arrastra todo a su paso.";
        case DivinePowerType::AtomicBomb:
            return "Cataclismo nuclear sin precedentes [TRUCO FUCKINGBOMB]. Desata una detonacion atomica con destello cegador, evaporacion instantanea del epicentro, onda supersonica que derriba bosques y aldeas, hongo nuclear colosal y crater vitrificado radiactivo.";
        default:
            return "";
    }
}

int DivinePowersSystem::getPowerRadius(DivinePowerType type) const {
    switch (type) {
        case DivinePowerType::Lightning:  return 1;
        case DivinePowerType::Plague:     return 2;
        case DivinePowerType::Miracle:    return 2;
        case DivinePowerType::Congregate: return 4;
        case DivinePowerType::Tornado:    return 4;
        case DivinePowerType::Earthquake: return 6;
        case DivinePowerType::Pandemic:   return 5;
        case DivinePowerType::Meteor:     return static_cast<int>(std::round(AverageMeteoriteScale::AIRBLAST_RADIUS_CELLS)); // EN: 10 cells (50m airblast) / ES: 10 celdas (50m onda aérea)
        case DivinePowerType::Volcano:    return 5;
        case DivinePowerType::Wildfire:   return 5;
        case DivinePowerType::Rain:       return 4;
        case DivinePowerType::Deluge:     return 6;
        case DivinePowerType::AtomicBomb: return static_cast<int>(std::round(LittleBoyScale::HEAVY_OVERPRESSURE_RADIUS_CELLS)); // EN: 100 cells (500m heavy blast) / ES: 100 celdas (500m onda destructiva)
        default: return 1;
    }
}

ColorRGB DivinePowersSystem::getPowerColor(DivinePowerType type) const {
    switch (type) {
        case DivinePowerType::Lightning:  return ColorRGB{120, 220, 255}; // Azul celeste eléctrico
        case DivinePowerType::Plague:     return ColorRGB{140, 225, 70};  // Verde peste
        case DivinePowerType::Miracle:    return ColorRGB{255, 230, 90};  // Oro radiante
        case DivinePowerType::Congregate: return ColorRGB{230, 161, 59};  // Oro litúrgico
        case DivinePowerType::Tornado:    return ColorRGB{180, 200, 220}; // Gris tormenta ciclónico
        case DivinePowerType::Earthquake: return ColorRGB{220, 140, 70};  // Terracota sísmico
        case DivinePowerType::Pandemic:   return ColorRGB{120, 230, 80};  // Verde tóxico miasma
        case DivinePowerType::Meteor:     return ColorRGB{255, 95, 25};   // Naranja magma ígneo
        case DivinePowerType::Volcano:    return ColorRGB{255, 55, 15};   // Rojo carmesí volcánico
        case DivinePowerType::Wildfire:   return ColorRGB{255, 110, 20};  // Naranja fuego ardiente
        case DivinePowerType::Rain:       return ColorRGB{90, 195, 255};  // Azul celeste lluvia
        case DivinePowerType::Deluge:     return ColorRGB{25, 110, 235};  // Azul profundo diluvio
        case DivinePowerType::AtomicBomb: return ColorRGB{255, 240, 100}; // Amarillo nuclear incandescente
        default:                          return ColorRGB{255, 255, 255};
    }
}

std::string DivinePowersSystem::getTitleForEra(HistoricalEra era) {
    if (era >= HistoricalEra::UniversalReligions) {
        return "PROFETA";
    }
    return "CHAMAN";
}

int DivinePowersSystem::ensureIntermediary(GestorHumanos& humanos, ReligionRegistry& registry,
                                           int playerRelId, std::string& outTitle, std::string& outName) {
    const Religion* pRel = registry.getReligion(playerRelId);
    HistoricalEra era = pRel ? pRel->currentEra : HistoricalEra::PrimordialAnimism;
    outTitle = getTitleForEra(era);

    // 1. Buscar intermediario vivo de la religión del jugador
    for (auto& h : humanos.getHumanos()) {
        if (h.estaVivo() && h.getReligionId() == playerRelId && h.esLiderEspiritual()) {
            if (outTitle == "PROFETA" && !h.esProfeta()) {
                h.setEsProfeta(true);
            } else if (outTitle == "CHAMAN" && !h.esChaman()) {
                h.setEsChaman(true);
            }
            outName = h.getNombre();
            return h.getId();
        }
    }

    // 2. Si no hay líder vivo, consagrar al fiel con mayor devoción
    Humano* mejorCandidato = nullptr;
    float mejorDevocion = -1.0f;
    for (auto& h : humanos.getHumanos()) {
        if (h.estaVivo() && h.getReligionId() == playerRelId) {
            if (h.getDevocion() > mejorDevocion) {
                mejorDevocion = h.getDevocion();
                mejorCandidato = &h;
            }
        }
    }

    if (mejorCandidato) {
        if (outTitle == "PROFETA") {
            mejorCandidato->setEsProfeta(true);
        } else {
            mejorCandidato->setEsChaman(true);
        }
        mejorCandidato->setDevocion(1.0f);
        outName = mejorCandidato->getNombre();
        return mejorCandidato->getId();
    }

    // 3. Si no hay ningún fiel de la fe del jugador, consagrar a cualquier humano vivo disponible
    if (!humanos.getHumanos().empty()) {
        for (auto& h : humanos.getHumanos()) {
            if (h.estaVivo()) {
                h.setReligionId(playerRelId);
                h.setDevocion(1.0f);
                if (outTitle == "PROFETA") {
                    h.setEsProfeta(true);
                } else {
                    h.setEsChaman(true);
                }
                outName = h.getNombre();
                return h.getId();
            }
        }
    }

    outName = "Guia Sagrado";
    return -1;
}

CastResult DivinePowersSystem::castPower(DivinePowerType type, int targetGx, int targetGy,
                                         WorldMap& map, ReligionRegistry& registry,
                                         GestorHumanos& humanos, int currentYear,
                                         EventLine& eventLine, int playerRelId,
                                         BuildingSystem* buildingSystem,
                                         ReplicationEngine* replicationEngine) {
    CastResult res;
    res.success = false;

    if (type == DivinePowerType::Inactive) {
        res.message = "Ningun poder seleccionado.";
        return res;
    }

    Religion* pRel = registry.getReligion(playerRelId);
    if (!pRel) {
        res.message = "Error: Religion del jugador no encontrada.";
        return res;
    }

    int cost = getPowerCost(type);
    if (pRel->theologyPoints < cost) {
        res.message = "Fe insuficiente (" + std::to_string(pRel->theologyPoints) + "/" +
                      std::to_string(cost) + " requeridos).";
        return res;
    }

    // Asegurar y determinar el intermediario de la fe
    std::string interTitle;
    std::string interName;
    int interId = ensureIntermediary(humanos, registry, playerRelId, interTitle, interName);
    res.intermediaryId = interId;
    res.intermediaryTitle = interTitle;
    res.intermediaryName = interName;

    Humano* intermediario = nullptr;
    if (interId > 0) {
        for (auto& h : humanos.getHumanos()) {
            if (h.getId() == interId) {
                intermediario = &h;
                break;
            }
        }
    }

    // Deducir el costo de fe
    pRel->theologyPoints -= cost;

    // Análisis de la relación de devoción comunitaria hacia el líder/intermediario
    float totalLeaderDev = 0.0f;
    int ownFollowersCount = 0;
    int fanaticCount = 0;
    for (const auto& h : humanos.getHumanos()) {
        if (h.estaVivo() && h.getReligionId() == playerRelId) {
            totalLeaderDev += h.getLeaderDevotion();
            ownFollowersCount++;
            if (h.esFanatico()) fanaticCount++;
        }
    }
    float avgLeaderDev = (ownFollowersCount > 0) ? (totalLeaderDev / static_cast<float>(ownFollowersCount)) : 0.50f;
    // Multiplicador de influencia: 0.40x (pésima devoción al líder) hasta 1.60x (máxima devoción y fanatismo)
    float leaderInfluenceMult = 0.40f + 1.20f * avgLeaderDev;

    std::string godName = pRel->founderName.empty() ? "Solarius" : pRel->founderName;
    std::string relName = pRel->name.empty() ? "Fe Ancestral" : pRel->name;
    ColorRGB rCol = pRel->color;

    int converts = 0;
    int inspired = 0;
    std::string sermon;
    std::uniform_real_distribution<float> dist01(0.0f, 1.0f);

    if (type == DivinePowerType::Lightning) {
        // =====================================================================
        // PODER 1: RAYO DIVINO (Lanzar Rayos)
        // =====================================================================
        float tx = static_cast<float>(targetGx) + 0.5f;
        float ty = static_cast<float>(targetGy) + 0.5f;

        // Daño severo, letalidad probabilística e impacto a unidades cercanas a la celda del impacto
        int casualties = 0;

        for (auto& h : humanos.getHumanos()) {
            if (!h.estaVivo()) continue;
            float dx = h.getX() - tx;
            float dy = h.getY() - ty;
            float distSq = dx * dx + dy * dy;
            if (distSq <= 3.5f) { // Radio hasta ~1.87 celdas
                float dist = std::sqrt(distSq);
                bool isFoe = (h.getReligionId() != playerRelId);
                float roll = dist01(g_divineRng);
                float blastAngle = (dist > 0.05f) ? std::atan2(dy, dx) : (static_cast<float>(h.getId()) * 1.57f);
                float blastForce = (dist <= 0.85f) ? 4.5f : 2.5f;
                float blastZ = (dist <= 0.85f) ? 36.0f : 20.0f;
                float giro = (dist <= 0.85f) ? 18.0f : 10.0f;

                if (dist <= 0.85f) {
                    // Centro del impacto directo: 85% de letalidad en infieles, 35% en fieles
                    float probMuerte = isFoe ? 0.85f : 0.35f;
                    if (roll < probMuerte) {
                        h.matar("Fulminado directamente por Rayo Divino (85% mort.)");
                        h.activarRagdoll(std::cos(blastAngle) * blastForce, std::sin(blastAngle) * blastForce, blastZ, giro);
                        casualties++;
                        std::cout << "[Rayo Divino] ¡" << h.getNombre() << " fue calcinado instantaneamente por el trueno sagrado!" << std::endl;
                    } else {
                        h.recibirDano(75.0f);
                        h.activarRagdoll(std::cos(blastAngle) * blastForce, std::sin(blastAngle) * blastForce, blastZ, giro);
                        if (isFoe) {
                            h.setDevocion(std::max(0.0f, h.getDevocion() - 0.50f));
                            h.triggerFaithIndicator(-1, 1.8f);
                        } else {
                            h.setDevocion(1.0f);
                            h.triggerFaithIndicator(+1, 1.5f);
                        }
                    }
                } else {
                    // Arco voltaico lateral / onda de choque: 45% de letalidad en infieles, 15% en fieles
                    float probMuerte = isFoe ? 0.45f : 0.15f;
                    if (roll < probMuerte) {
                        h.matar("Electrocutado por arco voltaico lateral de Rayo Divino (45% mort.)");
                        h.activarRagdoll(std::cos(blastAngle) * blastForce, std::sin(blastAngle) * blastForce, blastZ, giro);
                        casualties++;
                        std::cout << "[Rayo Divino] ¡" << h.getNombre() << " sucumbio a la descarga electrica lateral!" << std::endl;
                    } else {
                        h.recibirDano(45.0f);
                        h.activarRagdoll(std::cos(blastAngle) * blastForce, std::sin(blastAngle) * blastForce, blastZ, giro);
                        if (isFoe) {
                            h.setDevocion(std::max(0.0f, h.getDevocion() - 0.35f));
                            h.triggerFaithIndicator(-1, 1.5f);
                        } else {
                            h.setDevocion(1.0f);
                            h.triggerFaithIndicator(+1, 1.5f);
                        }
                    }
                }
            }
        }

        // EN: Thermal strike ignition: lightning sets fire to dry timber and thatched roofs
        // ES: Ignición térmica del punto de impacto: el rayo enciende madera seca y techos de paja
        if (map.isValid(targetGx, targetGy)) {
            Cell& strikeCell = map.getCell(targetGx, targetGy);
            if (!strikeCell.isWater() && strikeCell.isCombustible()) {
                map.igniteCell(targetGx, targetGy, 16.0f, 1.0f);
            }
        }

        // El intermediario canaliza el prodigio y predica a la multitud
        if (intermediario) {
            intermediario->activarSenalDivina(6.5f);
            if (interTitle == "CHAMAN") {
                sermon = "El trueno sagrado de " + godName + " ha quebrado la tierra. Doblegad vuestro orgullo y venerad Su poder.";
            } else {
                sermon = "He aqui la espada de juicio de " + godName + ". Ningun falso idolo permanecera ante el fuego celestial.";
            }

            // Radio de predica del intermediario + radio de impacto del rayo (radio 8.5)
            for (auto& h : humanos.getHumanos()) {
                if (!h.estaVivo() || &h == intermediario) continue;
                float d1 = std::sqrt((h.getX() - tx) * (h.getX() - tx) + (h.getY() - ty) * (h.getY() - ty));
                float d2 = std::sqrt((h.getX() - intermediario->getX()) * (h.getX() - intermediario->getX()) +
                                     (h.getY() - intermediario->getY()) * (h.getY() - intermediario->getY()));
                if (d1 <= 8.5f || d2 <= 8.5f) {
                    if (h.getReligionId() != playerRelId) {
                        // Terror sagrado y conversión ante la prédica (mayor con alta devoción al líder)
                        float convThreshold = 0.35f + 0.65f * avgLeaderDev;
                        if (dist01(g_divineRng) <= convThreshold) {
                            h.setReligionId(playerRelId);
                            h.setDevocion(0.85f);
                            h.setLeaderDevotion(0.65f);
                            h.cambiarAccion(AccionHumano::REZAR);
                            h.triggerFaithIndicator(+1, 2.2f);
                            converts++;
                        }
                    } else {
                        float unitLDev = h.getLeaderDevotion();
                        if (unitLDev >= 0.70f) {
                            h.setDevocion(1.0f);
                            h.triggerFaithIndicator(+1, 1.8f);
                            if (unitLDev >= 0.85f) h.promoverAFanatico();
                            inspired++;
                        } else if (unitLDev < 0.40f) {
                            // Duda y sospecha ante el trueno: debilidad del lazo con el líder
                            h.setLeaderDevotion(std::max(0.05f, h.getLeaderDevotion() - 0.12f));
                            h.huirEnPanicoDesde(tx, ty, 3.5f);
                        } else {
                            h.setDevocion(std::min(1.0f, h.getDevocion() + 0.15f));
                            h.triggerFaithIndicator(+1, 1.2f);
                            inspired++;
                        }
                    }
                }
            }
        }

        // Generación de Fe acumulada por el prodigio (modulada por devoción al líder)
        int faithGenerated = static_cast<int>((20 + converts * 40 + inspired * 12) * leaderInfluenceMult);
        pRel->theologyPoints += faithGenerated;

        res.faithGenerated = faithGenerated;
        res.converts = converts;
        res.inspired = inspired;
        res.casualties = casualties;
        res.sermon = sermon;
        res.success = true;
        res.message = "[RAYO DIVINO] " + interTitle + " " + interName + " desato la ira celestial: " +
                      std::to_string(casualties) + " fulminados, " +
                      std::to_string(converts) + " conversos (+" + std::to_string(faithGenerated) + " Fe).";

        // Registrar efecto visual
        {
            ActiveDivineFX fx;
            fx.type = DivinePowerType::Lightning;
            fx.gridX = targetGx;
            fx.gridY = targetGy;
            fx.worldX = static_cast<float>(targetGx) + 0.5f;
            fx.worldY = static_cast<float>(targetGy) + 0.5f;
            fx.duration = 0.85f;
            fx.timer = 0.0f;
            fx.converts = converts;
            fx.inspired = inspired;
            fx.faithGenerated = faithGenerated;
            fx.seed = static_cast<float>(targetGx * 17 + targetGy * 31);
            m_activeFX.push_back(fx);
        }

        // Registrar sermón del predicador
        if (intermediario) {
            m_activeSpeeches.push_back({
                intermediario->getId(),
                interName, interTitle,
                sermon,
                intermediario->getX(), intermediario->getY(),
                0.0f, 5.0f
            });
        }

        eventLine.addEvent(
            currentYear, EventCategory::MajorLoss,
            "Rayo Celestial de " + godName,
            "El Creador descargo un rayo en (" + std::to_string(targetGx) + "," + std::to_string(targetGy) +
            "). " + interTitle + " " + interName + " utilizo la senal: \"" + sermon + "\" (" +
            std::to_string(converts) + " conversos, +" + std::to_string(faithGenerated) + " Fe).",
            {targetGx, targetGy}, playerRelId, 0, rCol,
            true, interName, "Intervencion divina destructiva y predicacion multitudinaria"
        );

    } else if (type == DivinePowerType::Plague) {
        // =====================================================================
        // PODER 2: PLAGA SAGRADA (Mandar Plagas)
        // =====================================================================
        // Afecta área de 3x3 celdas centrada en targetGx, targetGy
        for (int dy = -1; dy <= 1; ++dy) {
            for (int dx = -1; dx <= 1; ++dx) {
                int cx = targetGx + dx;
                int cy = targetGy + dy;
                if (!map.isValid(cx, cy)) continue;
                Cell& c = map.getCell(cx, cy);
                // Si la celda no es del jugador, deteriora su producción
                if (c.dominantReligionId != playerRelId) {
                    c.maxPopulation = std::max(500, c.maxPopulation - 400);
                    c.devotion = std::max(0.1f, c.devotion - 0.30f);
                }
            }
        }

        // Debilitar y provocar bajas en unidades infieles en el área (30% de letalidad por asfixia y toxinas)
        int casualties = 0;
        for (auto& h : humanos.getHumanos()) {
            if (!h.estaVivo()) continue;
            int hx = h.getCeldaX();
            int hy = h.getCeldaY();
            if (std::abs(hx - targetGx) <= 1 && std::abs(hy - targetGy) <= 1) {
                if (h.getReligionId() != playerRelId) {
                    float roll = dist01(g_divineRng);
                    if (roll < 0.30f) {
                        h.matar("Asfixiado y devorado por la Plaga de Langostas (30% mort.)");
                        casualties++;
                        std::cout << "[Plaga Sagrada] ¡" << h.getNombre() << " sucumbio ante el enjambre venenoso!" << std::endl;
                    } else {
                        h.recibirDano(40.0f);
                        h.setDevocion(std::max(0.0f, h.getDevocion() - 0.45f));
                        h.triggerFaithIndicator(-1, 2.0f);
                    }
                }
            }
        }

        // El intermediario predica ante la plaga
        if (intermediario) {
            intermediario->activarSenalDivina(6.5f);
            if (interTitle == "CHAMAN") {
                sermon = "Las plagas azotan a quienes rechazan los mandatos de " + godName + ". Buscad Su proteccion sagrada.";
            } else {
                sermon = "Las plagas purgan la falsedad del mundo. Confesad la unica verdad de " + godName + " para ser librados.";
            }

            // Radio de conversión
            for (auto& h : humanos.getHumanos()) {
                if (!h.estaVivo() || &h == intermediario) continue;
                float d1 = std::sqrt((h.getCeldaX() - targetGx) * (h.getCeldaX() - targetGx) +
                                     (h.getCeldaY() - targetGy) * (h.getCeldaY() - targetGy));
                float d2 = std::sqrt((h.getX() - intermediario->getX()) * (h.getX() - intermediario->getX()) +
                                     (h.getY() - intermediario->getY()) * (h.getY() - intermediario->getY()));
                if (d1 <= 8.0f || d2 <= 8.0f) {
                    if (h.getReligionId() != playerRelId) {
                        float convThreshold = 0.30f + 0.70f * avgLeaderDev;
                        if (dist01(g_divineRng) <= convThreshold) {
                            h.setReligionId(playerRelId);
                            h.setDevocion(0.80f);
                            h.setLeaderDevotion(0.60f);
                            h.cambiarAccion(AccionHumano::REZAR);
                            h.triggerFaithIndicator(+1, 2.0f);
                            converts++;
                        }
                    } else {
                        float unitLDev = h.getLeaderDevotion();
                        if (unitLDev >= 0.65f) {
                            h.setDevocion(1.0f);
                            h.triggerFaithIndicator(+1, 1.5f);
                            inspired++;
                        } else {
                            h.setLeaderDevotion(std::max(0.05f, h.getLeaderDevotion() - 0.10f));
                        }
                    }
                }
            }
        }

        int faithGenerated = static_cast<int>((25 + converts * 45 + inspired * 10) * leaderInfluenceMult);
        pRel->theologyPoints += faithGenerated;

        res.faithGenerated = faithGenerated;
        res.converts = converts;
        res.inspired = inspired;
        res.casualties = casualties;
        res.sermon = sermon;
        res.success = true;
        res.message = "[PLAGA SAGRADA] El enjambre azoto la region: " + std::to_string(casualties) +
                      " muertos por asfixia/veneno, " + std::to_string(converts) +
                      " conversos (+" + std::to_string(faithGenerated) + " Fe).";

        {
            ActiveDivineFX fx;
            fx.type = DivinePowerType::Plague;
            fx.gridX = targetGx;
            fx.gridY = targetGy;
            fx.worldX = static_cast<float>(targetGx) + 0.5f;
            fx.worldY = static_cast<float>(targetGy) + 0.5f;
            fx.duration = 3.0f;
            fx.timer = 0.0f;
            fx.radius = 3.5f;
            fx.converts = converts;
            fx.inspired = inspired;
            fx.faithGenerated = faithGenerated;
            fx.seed = static_cast<float>(targetGx * 43 + targetGy * 19);
            m_activeFX.push_back(fx);
        }

        if (intermediario) {
            m_activeSpeeches.push_back({
                intermediario->getId(),
                interName, interTitle,
                sermon,
                intermediario->getX(), intermediario->getY(),
                0.0f, 5.0f
            });
        }

        eventLine.addEvent(
            currentYear, EventCategory::MajorLoss,
            "Plaga de Langostas y Peste",
            "Una plaga divina azoto el sector (" + std::to_string(targetGx) + "," + std::to_string(targetGy) +
            "). " + interTitle + " " + interName + " proclamo el juicio de " + godName + " (" +
            std::to_string(converts) + " conversos, +" + std::to_string(faithGenerated) + " Fe).",
            {targetGx, targetGy}, playerRelId, 0, rCol,
            true, interName, "Plaga de juicio y redencion por temor sagrado"
        );

    } else if (type == DivinePowerType::Miracle) {
        // =====================================================================
        // PODER 3: MILAGRO CELESTIAL (Hacer Milagros)
        // =====================================================================
        // 1. Purificar y fertilizar el suelo en 3x3 celdas
        for (int dy = -1; dy <= 1; ++dy) {
            for (int dx = -1; dx <= 1; ++dx) {
                int cx = targetGx + dx;
                int cy = targetGy + dy;
                if (!map.isValid(cx, cy)) continue;
                Cell& c = map.getCell(cx, cy);
                if (c.biome == BiomeType::AridDesert || c.biome == BiomeType::FrozenTundra ||
                    c.biome == BiomeType::RuggedMountains) {
                    c.biome = BiomeType::FertileValley;
                }
                c.maxPopulation = std::min(6000, c.maxPopulation + 600);
                c.devotion = std::min(1.0f, c.devotion + 0.35f);
            }
        }

        // 2. Sanación milagrosa de aldeanos en el área
        for (auto& h : humanos.getHumanos()) {
            if (!h.estaVivo()) continue;
            int hx = h.getCeldaX();
            int hy = h.getCeldaY();
            if (std::abs(hx - targetGx) <= 1 && std::abs(hy - targetGy) <= 1) {
                h.setSalud(std::min(100.0f, h.getSalud() + 75.0f));
                h.setDevocion(1.0f);
                h.triggerFaithIndicator(+1, 2.5f);
            }
        }

        // 3. El intermediario predica ante el milagro
        if (intermediario) {
            intermediario->activarSenalDivina(6.5f);
            if (interTitle == "CHAMAN") {
                sermon = "El manantial celestial brota y la carne sana por " + godName + ". Benditos los que confian en Su gracia.";
            } else {
                sermon = "Contemplad el milagro de " + godName + ". Los ciegos ven y la tierra florece ante Su infinita misericordia.";
            }

            // Conversión por compasión y asombro celestial (radio 9)
            for (auto& h : humanos.getHumanos()) {
                if (!h.estaVivo() || &h == intermediario) continue;
                float d1 = std::sqrt((h.getCeldaX() - targetGx) * (h.getCeldaX() - targetGx) +
                                     (h.getCeldaY() - targetGy) * (h.getCeldaY() - targetGy));
                float d2 = std::sqrt((h.getX() - intermediario->getX()) * (h.getX() - intermediario->getX()) +
                                     (h.getY() - intermediario->getY()) * (h.getY() - intermediario->getY()));
                if (d1 <= 9.0f || d2 <= 9.0f) {
                    if (h.getReligionId() != playerRelId) {
                        float convThreshold = 0.45f + 0.55f * avgLeaderDev;
                        if (dist01(g_divineRng) <= convThreshold) {
                            h.setReligionId(playerRelId);
                            h.setDevocion(0.95f);
                            h.setLeaderDevotion(0.80f);
                            h.cambiarAccion(AccionHumano::REZAR);
                            h.triggerFaithIndicator(+1, 2.5f);
                            converts++;
                        }
                    } else {
                        float unitLDev = h.getLeaderDevotion();
                        if (unitLDev >= 0.70f) {
                            h.setDevocion(1.0f);
                            h.setLeaderDevotion(1.0f);
                            h.triggerFaithIndicator(+1, 2.0f);
                            if (unitLDev >= 0.85f) h.promoverAFanatico();
                            inspired++;
                        } else {
                            // El milagro supera la incredulidad y eleva la devoción al líder
                            h.setLeaderDevotion(std::min(1.0f, h.getLeaderDevotion() + 0.35f));
                            h.setDevocion(1.0f);
                            h.triggerFaithIndicator(+1, 1.8f);
                            inspired++;
                        }
                    }
                }
            }
        }

        int faithGenerated = static_cast<int>((30 + converts * 50 + inspired * 15) * leaderInfluenceMult);
        pRel->theologyPoints += faithGenerated;

        res.faithGenerated = faithGenerated;
        res.converts = converts;
        res.inspired = inspired;
        res.sermon = sermon;
        res.success = true;
        res.message = "[MILAGRO CELESTIAL] Sanación y tierra bendecida por " + interTitle + " " + interName +
                      " (+" + std::to_string(faithGenerated) + " Fe, " + std::to_string(converts) + " conversos).";

        {
            ActiveDivineFX fx;
            fx.type = DivinePowerType::Miracle;
            fx.gridX = targetGx;
            fx.gridY = targetGy;
            fx.worldX = static_cast<float>(targetGx) + 0.5f;
            fx.worldY = static_cast<float>(targetGy) + 0.5f;
            fx.duration = 3.0f;
            fx.timer = 0.0f;
            fx.radius = 3.5f;
            fx.converts = converts;
            fx.inspired = inspired;
            fx.faithGenerated = faithGenerated;
            fx.seed = static_cast<float>(targetGx * 29 + targetGy * 53);
            m_activeFX.push_back(fx);
        }

        if (intermediario) {
            m_activeSpeeches.push_back({
                intermediario->getId(),
                interName, interTitle,
                sermon,
                intermediario->getX(), intermediario->getY(),
                0.0f, 5.0f
            });
        }

        eventLine.addEvent(
            currentYear, EventCategory::Discovery,
            "Milagro Celestial de " + godName,
            "Manifestacion milagrosa en (" + std::to_string(targetGx) + "," + std::to_string(targetGy) +
            "). La tierra florece y " + interTitle + " " + interName + " predica el prodigio (" +
            std::to_string(converts) + " nuevos conversos, +" + std::to_string(faithGenerated) + " Fe).",
            {targetGx, targetGy}, playerRelId, 0, rCol,
            true, interName, "Milagro visible de fertilidad y curacion"
        );

    } else if (type == DivinePowerType::Congregate) {
        // =====================================================================
        // PODER 4: CONGREGAR FIELES (Comunión Sagrada)
        // =====================================================================
        if (intermediario) {
            intermediario->activarSenalDivina(6.0f);
            sermon = "Venid, hijos de " + godName + ". Unios en un solo circulo sagrado y elevad vuestras plegarias.";

            int congregados = 0;
            int idx = 0;
            for (auto& h : humanos.getHumanos()) {
                if (!h.estaVivo() || &h == intermediario) continue;
                if (h.getReligionId() == playerRelId) {
                    float unitLDev = h.getLeaderDevotion();
                    if (unitLDev >= 0.38f) {
                        float ang = static_cast<float>(idx++) * 1.15f;
                        float rad = 1.35f + static_cast<float>(idx % 3) * 0.65f;
                        int tx = std::clamp(static_cast<int>(std::round(intermediario->getX() + std::cos(ang) * rad)), 2, map.getWidth() - 3);
                        int ty = std::clamp(static_cast<int>(std::round(intermediario->getY() + std::sin(ang) * rad)), 2, map.getHeight() - 3);
                        if (map.getCell(tx, ty).isHabitable()) {
                            h.caminarHacia(static_cast<float>(tx) + 0.5f, static_cast<float>(ty) + 0.5f, true);
                        }
                        float devGain = 0.15f + 0.15f * unitLDev;
                        h.setDevocion(std::min(1.0f, h.getDevocion() + devGain));
                        h.setLeaderDevotion(std::min(1.0f, h.getLeaderDevotion() + 0.15f));
                        h.triggerFaithIndicator(+1, 1.5f);
                        if (h.getLeaderDevotion() >= 0.85f && h.getDevocion() >= 0.88f) {
                            h.promoverAFanatico();
                        }
                        congregados++;
                    } else {
                        // Devoción deficiente al líder: desobediencia o lentitud ante el llamado
                        if (h.getId() % 2 == 0) {
                            h.caminarHacia(intermediario->getX(), intermediario->getY(), false);
                            h.setLeaderDevotion(h.getLeaderDevotion() + 0.05f);
                            congregados++;
                        }
                    }
                }
            }

            int faithGenerated = static_cast<int>((15 + congregados * 8) * leaderInfluenceMult);
            pRel->theologyPoints += faithGenerated;

            res.faithGenerated = faithGenerated;
            res.inspired = congregados;
            res.sermon = sermon;
            res.success = true;
            res.message = "[CONGREGAR] " + interTitle + " " + interName + " congrego a " +
                          std::to_string(congregados) + " fieles en torno al altar (+" +
                          std::to_string(faithGenerated) + " Fe).";

            m_activeSpeeches.push_back({
                intermediario->getId(),
                interName, interTitle,
                sermon,
                intermediario->getX(), intermediario->getY(),
                0.0f, 4.5f
            });

            eventLine.addEvent(
                currentYear, EventCategory::PilgrimageWave,
                "Llamado Sagrado de " + interName,
                interTitle + " " + interName + " congrego a la comunidad de " + relName +
                " en solemne oracion colectiva (+" + std::to_string(faithGenerated) + " Fe).",
                {intermediario->getCeldaX(), intermediario->getCeldaY()}, playerRelId, 0, rCol,
                false, interName, "Comunion de fieles e incremento de devocion"
            );
        } else {
            res.message = "No hay intermediario para convocar al pueblo.";
        }
    } else if (type == DivinePowerType::Tornado) {
        // =====================================================================
        // CATÁSTROFE 1: TORNADO DEVASTADOR (Vórtice, Succión y Vuelo de Aldeanos)
        // =====================================================================
        float tx = static_cast<float>(targetGx) + 0.5f;
        float ty = static_cast<float>(targetGy) + 0.5f;

        float angle = static_cast<float>((targetGx * 73 + targetGy * 37) % 360) * (3.14159f / 180.0f);
        float driftVx = std::cos(angle) * 0.75f;
        float driftVy = std::sin(angle) * 0.75f;

        m_activeFX.push_back({
            DivinePowerType::Tornado,
            targetGx, targetGy,
            tx, ty,
            driftVx, driftVy,
            12.0f, // 12 segundos de duración
            0.0f,
            1.0f,
            4.5f,  // Radio de influencia
            0, 0, 0,
            static_cast<float>(targetGx * 13 + targetGy * 7)
        });

        int volandoCount = 0;
        int panicoCount = 0;
        int casualties = 0;
        for (auto& h : humanos.getHumanos()) {
            if (!h.estaVivo()) continue;
            float dx = h.getX() - tx;
            float dy = h.getY() - ty;
            float d = std::sqrt(dx * dx + dy * dy);
            if (d < 2.2f) {
                h.serSuccionadoPorTornado(tx, ty, 4.5f, 0.15f);
                if (h.estaMuerto()) {
                    casualties++;
                } else if (h.isFlying()) {
                    volandoCount++;
                }
            } else if (d <= 4.5f) {
                h.huirEnPanicoDesde(tx, ty, 5.5f);
                panicoCount++;
            }
        }

        if (intermediario) {
            intermediario->activarSenalDivina(6.0f);
            sermon = "¡El gran torbellino de " + godName + " se alza sobre la tierra! ¡Los impios vuelan por los aires ante Su aliento!";
            m_activeSpeeches.push_back({
                intermediario->getId(), interName, interTitle,
                sermon, intermediario->getX(), intermediario->getY(), 0.0f, 5.0f
            });
        }

        int faithGenerated = static_cast<int>((30 + volandoCount * 15 + panicoCount * 5) * leaderInfluenceMult);
        pRel->theologyPoints += faithGenerated;

        res.faithGenerated = faithGenerated;
        res.casualties = casualties;
        res.success = true;
        res.message = "[TORNADO DEVASTADOR] ¡Un ciclon azota (" + std::to_string(targetGx) + "," +
                      std::to_string(targetGy) + ")! " + std::to_string(volandoCount) +
                      " aldeanos volando, " + std::to_string(casualties) + " victimas mortales y " +
                      std::to_string(panicoCount) + " huyendo en panico.";

        eventLine.addEvent(
            currentYear, EventCategory::MajorLoss,
            "Tornado Devastador de " + godName,
            "Un violento ciclon se desato en (" + std::to_string(targetGx) + "," + std::to_string(targetGy) +
            "). Los vientos huracanados levantaron aldeanos por los cielos y arrasaron los recursos circundantes.",
            {targetGx, targetGy}, playerRelId, 0, ColorRGB{180, 200, 220},
            true, interName, "Catastrofe natural de vortice y destruccion ambiental"
        );
    } else if (type == DivinePowerType::Earthquake) {
        // =====================================================================
        // CATÁSTROFE 2: TERREMOTO SÍSMICO (Sacudida, Fisuras y Daño Estructural)
        // =====================================================================
        float tx = static_cast<float>(targetGx) + 0.5f;
        float ty = static_cast<float>(targetGy) + 0.5f;

        // EN: Form permanent seismic fault craters, graben depressions, and tectonic rifts
        // ES: Formar cráteres sísmicos permanentes, depresiones de graben y fallas tectónicas
        const float faultAngle = static_cast<float>((targetGx * 37 + targetGy * 23 + 19) % 360) * (3.14159265f / 180.0f);
        map.createEarthquakeFissureZone(targetGx, targetGy, 1.25f, 5.5f, faultAngle, buildingSystem, &humanos);

        // EN: Spawn stone and timber rubble debris along the epicentral rupture
        // ES: Generar escombros de piedra y madera a lo largo de la ruptura epicentral
        constexpr int SEISMIC_DEBRIS_COUNT = 6;
        for (int debrisIndex = 0; debrisIndex < SEISMIC_DEBRIS_COUNT; ++debrisIndex) {
            const float debrisAngle = faultAngle + static_cast<float>(debrisIndex) * 1.047f;
            const float debrisOffset = 1.0f + static_cast<float>(debrisIndex % 3) * 0.9f;
            const float debrisSpawnX = tx + std::cos(debrisAngle) * debrisOffset;
            const float debrisSpawnY = ty + std::sin(debrisAngle) * debrisOffset;
            spawnDebris(DebrisType::HouseRubble, debrisSpawnX, debrisSpawnY, 2.5f,
                        std::cos(debrisAngle) * 3.5f, std::sin(debrisAngle) * 3.5f,
                        12.0f, 25.0f, 20.0f, 1.2f);
        }

        m_activeFX.push_back({
            DivinePowerType::Earthquake,
            targetGx, targetGy,
            tx, ty,
            0.0f, 0.0f,
            7.0f, // 7 segundos de sismo
            0.0f,
            1.2f, // Intensidad
            6.0f, // Radio
            0, 0, 0,
            static_cast<float>(targetGx * 29 + targetGy * 17)
        });

        // Daño a estructuras sagradas en el área
        for (int dy = -3; dy <= 3; ++dy) {
            for (int dx = -3; dx <= 3; ++dx) {
                int cx = targetGx + dx;
                int cy = targetGy + dy;
                if (!map.isValid(cx, cy)) continue;
                Cell& c = map.getCell(cx, cy);
                if (c.hasHolySite) {
                    c.devotion = std::max(0.1f, c.devotion - 0.40f);
                    if (c.shrineLevel > 1 && (dx * dx + dy * dy <= 4)) {
                        c.shrineLevel--;
                    }
                }
            }
        }

        // Sacudida sísmica, derribo y bajas de aldeanos en el radio
        int afectados = 0;
        int casualties = 0;
        for (auto& h : humanos.getHumanos()) {
            if (!h.estaVivo()) continue;
            float dx = h.getX() - tx;
            float dy = h.getY() - ty;
            float d = std::sqrt(dx * dx + dy * dy);
            if (d <= 6.5f) {
                float roll = dist01(g_divineRng);
                if (d <= 2.5f) {
                    // Epicentro sísmico: 40% de letalidad por apertura de fisuras o aplastamiento
                    float pushAngle = (d > 0.05f) ? std::atan2(dy, dx) : (static_cast<float>(h.getId()) * 1.57f);
                    if (roll < 0.40f) {
                        h.matar("Tragado por falla tectonica o aplastado por sismo (40% mort.)");
                        h.activarRagdoll(std::cos(pushAngle) * 2.2f, std::sin(pushAngle) * 2.2f, 12.0f, 10.0f);
                        casualties++;
                        std::cout << "[Terremoto] ¡" << h.getNombre() << " perecio tragado por una falla tectonica!" << std::endl;
                    } else {
                        h.sufrirSismoTerremoto(tx, ty, 1.4f);
                        h.activarRagdoll(std::cos(pushAngle) * 1.8f, std::sin(pushAngle) * 1.8f, 10.0f, 8.0f);
                        afectados++;
                    }
                } else {
                    // Periferia sísmica: 15% de letalidad por desprendimientos rocosos
                    float pushAngle = (d > 0.05f) ? std::atan2(dy, dx) : (static_cast<float>(h.getId()) * 1.57f);
                    if (roll < 0.15f) {
                        h.matar("Aplastado por desprendimiento rocoso durante el sismo (15% mort.)");
                        h.activarRagdoll(std::cos(pushAngle) * 1.4f, std::sin(pushAngle) * 1.4f, 8.0f, 6.0f);
                        casualties++;
                        std::cout << "[Terremoto] ¡" << h.getNombre() << " murio por derrumbes provocados por el temblor!" << std::endl;
                    } else {
                        h.sufrirSismoTerremoto(tx, ty, 0.85f);
                        if (dist01(g_divineRng) < 0.45f) {
                            h.activarRagdoll(std::cos(pushAngle) * 1.0f, std::sin(pushAngle) * 1.0f, 5.0f, 5.0f);
                        }
                        afectados++;
                    }
                }

                // Desplazamiento tectónico lateral que puede arrojar al agua
                float pushAngle = std::atan2(dy, dx);
                float pushDist = (d <= 2.5f) ? 1.2f : 0.6f;
                h.desplazarPorSismo(std::cos(pushAngle) * pushDist, std::sin(pushAngle) * pushDist, map);
            }
        }

        if (intermediario) {
            intermediario->activarSenalDivina(5.5f);
            sermon = "¡La tierra se estremece bajo el peso de " + godName + "! ¡Que caigan los falsos templos ante el verdadero Creador!";
            m_activeSpeeches.push_back({
                intermediario->getId(), interName, interTitle,
                sermon, intermediario->getX(), intermediario->getY(), 0.0f, 5.0f
            });
        }

        int faithGenerated = static_cast<int>((35 + afectados * 8) * leaderInfluenceMult);
        pRel->theologyPoints += faithGenerated;

        res.faithGenerated = faithGenerated;
        res.casualties = casualties;
        res.success = true;
        res.message = "[TERREMOTO SÍSMICO] Fuerte sismo sacude (" + std::to_string(targetGx) + "," +
                      std::to_string(targetGy) + "). " + std::to_string(casualties) +
                      " muertos por fisuras/derrumbes, " + std::to_string(afectados) +
                      " heridos derribados y panico masivo.";

        eventLine.addEvent(
            currentYear, EventCategory::MajorLoss,
            "Terremoto Catastrofico en (" + std::to_string(targetGx) + "," + std::to_string(targetGy) + ")",
            "La tierra se quebro en fallas tectonicas con el poder de " + godName +
            ". Temblores sacudieron los cimientos de la provincia desatando el pavor.",
            {targetGx, targetGy}, playerRelId, 0, ColorRGB{220, 140, 70},
            true, interName, "Cataclismo sismico y sacudida de la tierra"
        );
    } else if (type == DivinePowerType::Pandemic) {
        // =====================================================================
        // CATÁSTROFE 3: PANDEMIA VIRAL (Brote Infeccioso y Contagio Dinámico)
        // =====================================================================
        float tx = static_cast<float>(targetGx) + 0.5f;
        float ty = static_cast<float>(targetGy) + 0.5f;

        m_activeFX.push_back({
            DivinePowerType::Pandemic,
            targetGx, targetGy,
            tx, ty,
            0.0f, 0.0f,
            16.0f,
            0.0f,
            1.0f,
            5.0f,
            0, 0, 0,
            static_cast<float>(targetGx * 43 + targetGy * 19)
        });

        int infectados = 0;
        int casualties = 0;
        for (auto& h : humanos.getHumanos()) {
            if (!h.estaVivo()) continue;
            float dx = h.getX() - tx;
            float dy = h.getY() - ty;
            float d = std::sqrt(dx * dx + dy * dy);
            if (d <= 5.0f) {
                // En el foco agudo (< 2.0 celdas), un 15% de individuos vulnerables sucumbe de inmediato al miasma denso
                if (d <= 2.0f && dist01(g_divineRng) < 0.15f) {
                    h.matar("Asfixia aguda e intoxicacion letal por Miasma Virico (15% mort.)");
                    casualties++;
                    std::cout << "[Pandemia Viral] ¡" << h.getNombre() << " sucumbio inmediatamente a la nube toxica de la peste!" << std::endl;
                } else {
                    h.infectarConPandemia(20.0f);
                    infectados++;
                }
            }
        }

        if (map.isValid(targetGx, targetGy)) {
            Cell& c = map.getCell(targetGx, targetGy);
            c.population = std::max(20, c.population - 150);
        }

        if (intermediario) {
            intermediario->activarSenalDivina(6.0f);
            sermon = "¡La peste purga a los pecadores! Solo aquellos que abracen el credo de " + godName + " encontraran salvacion.";
            m_activeSpeeches.push_back({
                intermediario->getId(), interName, interTitle,
                sermon, intermediario->getX(), intermediario->getY(), 0.0f, 5.0f
            });
        }

        int faithGenerated = static_cast<int>((25 + infectados * 10) * leaderInfluenceMult);
        pRel->theologyPoints += faithGenerated;

        res.faithGenerated = faithGenerated;
        res.casualties = casualties;
        res.success = true;
        res.message = "[PANDEMIA VIRAL] Brote estalla en (" + std::to_string(targetGx) + "," +
                      std::to_string(targetGy) + "). " + std::to_string(casualties) +
                      " bajas inmediatas, " + std::to_string(infectados) +
                      " infectados tosiendo y propagando el virus.";

        eventLine.addEvent(
            currentYear, EventCategory::MajorLoss,
            "Brote de Pandemia Viral",
            "Una peste altamente contagiosa fue desatada sobre (" + std::to_string(targetGx) + "," + std::to_string(targetGy) +
            "). La plaga se contagia de persona a persona entre accesos de tos y desesperacion.",
            {targetGx, targetGy}, playerRelId, 0, ColorRGB{120, 230, 80},
            true, interName, "Epidemia biologica y crisis sanitaria comunitaria"
        );
    } else if (type == DivinePowerType::Meteor) {
        // ======================================================================================
        // EN: CATACLYSM: AVERAGE METEORITE IMPACT (Typical 1.5-2.5m Bolide, 16.5m Crater, 50m Airblast)
        // ES: CATACLISMO: IMPACTO DE METEORITO PROMEDIO (Bólido 1.5-2.5m, Cráter 16.5m, Onda Aérea 50m)
        // ======================================================================================
        const float targetCenterX = static_cast<float>(targetGx) + 0.5f;
        const float targetCenterY = static_cast<float>(targetGy) + 0.5f;
        constexpr float METEOR_AIRBLAST_RADIUS = AverageMeteoriteScale::AIRBLAST_RADIUS_CELLS; // 10.0f cells = 50.0 meters

        m_activeFX.push_back({
            DivinePowerType::Meteor,
            targetGx, targetGy,
            targetCenterX, targetCenterY,
            0.0f, 0.0f,
            5.0f,  // EN: 5.0 seconds total lifetime (1.25s atmospheric descent + 3.75s crater & smoke) / ES: 5.0s vida total
            0.0f,
            1.0f,
            METEOR_AIRBLAST_RADIUS,  // EN: Physical airblast hazard radius (10.0 cells = 50.0 meters)
            0, 0, 0,
            static_cast<float>(targetGx * 23 + targetGy * 41 + 7),
            false  // impactTriggered
        });

        // EN: Advance panic: villagers seeing the fiery descent streak across the sky flee in panic
        // ES: Pánico anticipado: los aldeanos al ver descender la estela ardiente huyen en pánico
        int panicCount = 0;
        for (auto& villager : humanos.getHumanos()) {
            if (!villager.estaVivo()) continue;
            const float deltaVillagerX = villager.getX() - targetCenterX;
            const float deltaVillagerY = villager.getY() - targetCenterY;
            const float distanceToTarget = std::sqrt(deltaVillagerX * deltaVillagerX + deltaVillagerY * deltaVillagerY);
            if (distanceToTarget <= METEOR_AIRBLAST_RADIUS + 1.5f) {
                villager.huirEnPanicoDesde(targetCenterX, targetCenterY, 8.5f);
                panicCount++;
            }
        }

        if (intermediario) {
            intermediario->activarSenalDivina(8.0f);
            sermon = "¡He aqui el fuego supremo de los astros! ¡" + godName + " arroja las estrellas para juzgar la tierra!";
            m_activeSpeeches.push_back({
                intermediario->getId(), interName, interTitle,
                sermon, intermediario->getX(), intermediario->getY(), 0.0f, 6.0f
            });
        }

        int faithGenerated = static_cast<int>((60 + panicCount * 8) * leaderInfluenceMult);
        pRel->theologyPoints += faithGenerated;

        res.faithGenerated = faithGenerated;
        res.casualties = 0; // Las bajas se consuman en el momento del impacto a t=1.25s
        res.success = true;
        res.message = "[METEORITO CATACLISMICO] ¡Bolido celeste desciende en picado hacia (" +
                      std::to_string(targetGx) + "," + std::to_string(targetGy) + ")! " +
                      std::to_string(panicCount) + " aldeanos huyen aterrados de la zona de impacto.";

        eventLine.addEvent(
            currentYear, EventCategory::MajorLoss,
            "Impacto de Meteorito Celeste",
            "Un inmenso asteroide en llamas cayo desde el cenit sobre (" + std::to_string(targetGx) + "," + std::to_string(targetGy) +
            "). El impacto causo un cataclismo tectonico y un crater sagrado imborrable.",
            {targetGx, targetGy}, playerRelId, 0, ColorRGB{255, 95, 25},
            true, interName, "Cataclismo cosmico y juicio celestial"
        );
    } else if (type == DivinePowerType::Volcano) {
        // CATÁSTROFE 5: ERUPCIÓN VOLCÁNICA (Nacimiento de Gran Volcán, Magma y Ríos de Lava)
        // =================================================================================
        float tx = static_cast<float>(targetGx) + 0.5f;
        float ty = static_cast<float>(targetGy) + 0.5f;

        // 1. Alzar la tierra y fundar el Gran Volcán
        Cell& cCenter = map.getCell(targetGx, targetGy);
        cCenter.hasVolcano = true;
        cCenter.hasActiveLava = true;
        // EN: Lava stays active during eruption, then solidifies into basalt
        // ES: La lava se mantiene activa durante la erupción, luego se solidifica en basalto
        cCenter.lavaTimer = 45.0f;
        cCenter.biome = BiomeType::AridDesert; // Basalto volcánico
        cCenter.elevation = std::max(2.4f, cCenter.elevation + 1.4f);
        cCenter.resourceAmount = 0;
        cCenter.naturalResource = ResourceType::None;

        // Ondulaciones orogénicas alrededor del cono (radio 1 y 2)
        for (int dy = -2; dy <= 2; ++dy) {
            for (int dx = -2; dx <= 2; ++dx) {
                if (dx == 0 && dy == 0) continue;
                int nx = targetGx + dx;
                int ny = targetGy + dy;
                if (!map.isValid(nx, ny)) continue;
                Cell& nc = map.getCell(nx, ny);
                float dist = std::sqrt(static_cast<float>(dx * dx + dy * dy));
                if (dist <= 1.5f) {
                    nc.elevation = std::min(3.5f, nc.elevation + 0.6f);
                }
            }
        }

        m_activeFX.push_back({
            DivinePowerType::Volcano,
            targetGx, targetGy,
            tx, ty,
            0.0f, 0.0f,
            20.0f, // 20 segundos de erupción continua
            0.0f,
            1.0f,
            5.0f,  // Radio de influencia de la lava y bombas volcánicas
            0, 0, 0,
            static_cast<float>(targetGx * 31 + targetGy * 19 + 13),
            false
        });

        // Pánico inmediato de aldeanos huyendo de la erupción
        int panicoCount = 0;
        for (auto& h : humanos.getHumanos()) {
            if (!h.estaVivo()) continue;
            float dx = h.getX() - tx;
            float dy = h.getY() - ty;
            float d = std::sqrt(dx * dx + dy * dy);
            if (d <= 6.0f) {
                h.huirEnPanicoDesde(tx, ty, 8.0f);
                panicoCount++;
            }
        }

        if (intermediario) {
            intermediario->activarSenalDivina(10.0f);
            sermon = "¡Las entranas del mundo rugen con fuego primordial! ¡Postraos ante el volcan sagrado de " + godName + "!";
            m_activeSpeeches.push_back({
                intermediario->getId(), interName, interTitle,
                sermon, intermediario->getX(), intermediario->getY(), 0.0f, 6.0f
            });
        }

        int faithGenerated = static_cast<int>((75 + panicoCount * 10) * leaderInfluenceMult);
        pRel->theologyPoints += faithGenerated;

        res.faithGenerated = faithGenerated;
        res.casualties = 0;
        res.success = true;
        res.message = "[ERUPCION VOLCANICA] ¡Un colosal volcan despierta en (" +
                      std::to_string(targetGx) + "," + std::to_string(targetGy) + ")! Magma y lava brotan de la caldera. " +
                      std::to_string(panicoCount) + " aldeanos huyen despavoridos.";

        eventLine.addEvent(
            currentYear, EventCategory::MajorLoss,
            "Despertar Volcanico de " + godName,
            "Un volcan de basalto surgio desde las profundidades en (" + std::to_string(targetGx) + "," + std::to_string(targetGy) +
            "), expulsando rios de lava que transforman la geografia y expanden la tierra.",
            {targetGx, targetGy}, playerRelId, 0, ColorRGB{255, 60, 15},
            true, interName, "Erupcion geologica y fuego primordial"
        );
    } else if (type == DivinePowerType::Wildfire) {
        // CATÁSTROFE 6: INCENDIO FORESTAL (Fuego Sagrado y Propagación Ígnea Celular)
        // =========================================================================
        float tx = static_cast<float>(targetGx) + 0.5f;
        float ty = static_cast<float>(targetGy) + 0.5f;

        // 1. Ignición del epicentro y celdas inflamables adyacentes
        int ignitedCells = 0;
        for (int dy = -2; dy <= 2; ++dy) {
            for (int dx = -2; dx <= 2; ++dx) {
                int cx = targetGx + dx;
                int cy = targetGy + dy;
                if (!map.isValid(cx, cy)) continue;
                if (map.igniteCell(cx, cy, 18.0f, 1.0f)) {
                    ignitedCells++;
                }
            }
        }

        m_activeFX.push_back({
            DivinePowerType::Wildfire,
            targetGx, targetGy,
            tx, ty,
            0.0f, 0.0f,
            20.0f, // 20 segundos de duración del incendio forestal
            0.0f,
            1.0f,
            5.0f,  // Radio de acción
            0, 0, 0,
            static_cast<float>(targetGx * 41 + targetGy * 29 + 17),
            false
        });

        // 2. Pánico inmediato y huida de aldeanos ante las llamas
        int panicoCount = 0;
        int directBurns = 0;
        for (auto& h : humanos.getHumanos()) {
            if (!h.estaVivo()) continue;
            float dx = h.getX() - tx;
            float dy = h.getY() - ty;
            float d = std::sqrt(dx * dx + dy * dy);
            if (d <= 6.5f) {
                h.huirEnPanicoDesde(tx, ty, 8.5f);
                panicoCount++;

                // Daño por quemaduras directas en proximidad inmediata
                if (d <= 2.2f) {
                    h.recibirDano(45.0f);
                    directBurns++;
                    if (!h.estaVivo()) {
                        h.matar("Calcinado por el frente de fuego del incendio forestal");
                    }
                }
            }
        }

        if (intermediario) {
            intermediario->activarSenalDivina(8.0f);
            sermon = "¡El fuego sagrado de " + godName + " desciende sobre la espesura! ¡Que la llama purifique la tierra rebelde!";
            m_activeSpeeches.push_back({
                intermediario->getId(), interName, interTitle,
                sermon, intermediario->getX(), intermediario->getY(), 0.0f, 6.0f
            });
        }

        int faithGenerated = static_cast<int>((60 + panicoCount * 8 + directBurns * 12) * leaderInfluenceMult);
        pRel->theologyPoints += faithGenerated;

        res.faithGenerated = faithGenerated;
        res.casualties = directBurns;
        res.success = true;
        res.message = "[INCENDIO FORESTAL] ¡Fuego sagrado desatado en (" +
                      std::to_string(targetGx) + "," + std::to_string(targetGy) + ")! " +
                      std::to_string(ignitedCells) + " celdas de bosque arden en llamas. " +
                      std::to_string(panicoCount) + " aldeanos huyen despavoridos.";

        eventLine.addEvent(
            currentYear, EventCategory::MajorLoss,
            "Incendio Forestal de " + godName,
            "Un pavoroso incendio forestal se desato en (" + std::to_string(targetGx) + "," + std::to_string(targetGy) +
            "), calcinando espesuras, madera y chozas bajo el fuego sagrado de " + godName + ".",
            {targetGx, targetGy}, playerRelId, 0, ColorRGB{255, 110, 20},
            true, interName, "Fuego sagrado purificador y calcinacion de bosques"
        );
    } else if (type == DivinePowerType::Rain) {
        // EN: BLESSED RAIN - Heavenly precipitation, fertilizing soil and quenching fires
        // ES: LLUVIA BENDITA - Precipitación celestial, fertilizando suelos y apagando incendios
        const float targetCenterX = static_cast<float>(targetGx) + 0.5f;
        const float targetCenterY = static_cast<float>(targetGy) + 0.5f;

        m_activeFX.push_back({
            DivinePowerType::Rain,
            targetGx, targetGy,
            targetCenterX, targetCenterY,
            0.0f, 0.0f,
            16.0f,
            0.0f,
            1.0f,
            4.0f,
            0, 0, 0,
            static_cast<float>(targetGx * 31 + targetGy * 17 + 11),
            false
        });

        int firesExtinguished = 0;
        for (int deltaCellY = -4; deltaCellY <= 4; ++deltaCellY) {
            for (int deltaCellX = -4; deltaCellX <= 4; ++deltaCellX) {
                if (deltaCellX * deltaCellX + deltaCellY * deltaCellY > 16) continue;
                const int currentX = targetGx + deltaCellX;
                const int currentY = targetGy + deltaCellY;
                if (!map.isValid(currentX, currentY)) continue;
                Cell& rainCell = map.getCell(currentX, currentY);
                if (rainCell.isBurning) {
                    rainCell.extinguish();
                    firesExtinguished++;
                }
                map.addWaterToCell(currentX, currentY, 0.18f);
            }
        }

        int healedVillagers = 0;
        for (auto& villager : humanos.getHumanos()) {
            if (!villager.estaVivo()) continue;
            const float deltaDistX = villager.getX() - targetCenterX;
            const float deltaDistY = villager.getY() - targetCenterY;
            if (deltaDistX * deltaDistX + deltaDistY * deltaDistY <= 20.0f) {
                if (villager.isBurning()) {
                    villager.extinguishFire();
                }
                villager.setSalud(std::min(100.0f, villager.getHealth() + 25.0f));
                healedVillagers++;
            }
        }

        if (intermediario) {
            intermediario->activarSenalDivina(8.0f);
            sermon = "¡Las nubes celestiales de " + godName + " derraman vida y frescor! ¡Que las aguas apaguen el fuego y nutran la tierra!";
            m_activeSpeeches.push_back({
                intermediario->getId(), interName, interTitle,
                sermon, intermediario->getX(), intermediario->getY(), 0.0f, 6.0f
            });
        }

        const int faithGenerated = static_cast<int>((50 + healedVillagers * 8 + firesExtinguished * 15) * leaderInfluenceMult);
        pRel->theologyPoints += faithGenerated;

        res.faithGenerated = faithGenerated;
        res.success = true;
        res.message = "[LLUVIA BENDITA] ¡Lluvia celestial derramada en (" +
                      std::to_string(targetGx) + "," + std::to_string(targetGy) + ")! " +
                      std::to_string(firesExtinguished) + " incendios sofocados, " +
                      std::to_string(healedVillagers) + " aldeanos refrescados y bendecidos.";

        eventLine.addEvent(
            currentYear, EventCategory::Discovery,
            "Lluvia Celestial de " + godName,
            "Cielos abiertos derramaron lluvia benefactora en (" + std::to_string(targetGx) + "," + std::to_string(targetGy) +
            "), apagando incendios y fertilizando los campos sagrados bajo el amparo de " + godName + ".",
            {targetGx, targetGy}, playerRelId, 0, ColorRGB{90, 195, 255},
            true, interName, "Precipitación bienhechora y sofoco de llamas"
        );
    } else if (type == DivinePowerType::Deluge) {
        // EN: GREAT DELUGE - Biblical cataclysm of torrential storm and massive flooding
        // ES: GRAN DILUVIO - Cataclismo bíblico de tormenta torrencial e inundación masiva
        const float targetCenterX = static_cast<float>(targetGx) + 0.5f;
        const float targetCenterY = static_cast<float>(targetGy) + 0.5f;

        m_activeFX.push_back({
            DivinePowerType::Deluge,
            targetGx, targetGy,
            targetCenterX, targetCenterY,
            0.0f, 0.0f,
            24.0f,
            0.0f,
            1.0f,
            6.0f,
            0, 0, 0,
            static_cast<float>(targetGx * 47 + targetGy * 23 + 19),
            false
        });

        int firesExtinguished = 0;
        int floodedCount = 0;
        for (int deltaCellY = -6; deltaCellY <= 6; ++deltaCellY) {
            for (int deltaCellX = -6; deltaCellX <= 6; ++deltaCellX) {
                if (deltaCellX * deltaCellX + deltaCellY * deltaCellY > 36) continue;
                const int currentX = targetGx + deltaCellX;
                const int currentY = targetGy + deltaCellY;
                if (!map.isValid(currentX, currentY)) continue;
                Cell& delugeCell = map.getCell(currentX, currentY);
                if (delugeCell.isBurning) {
                    delugeCell.extinguish();
                    firesExtinguished++;
                }
                map.addWaterToCell(currentX, currentY, 0.45f);
                if (delugeCell.isFlooded) floodedCount++;
            }
        }

        int panicCount = 0;
        for (auto& villager : humanos.getHumanos()) {
            if (!villager.estaVivo()) continue;
            const float deltaDistX = villager.getX() - targetCenterX;
            const float deltaDistY = villager.getY() - targetCenterY;
            const float dist = std::sqrt(deltaDistX * deltaDistX + deltaDistY * deltaDistY);
            if (dist <= 7.5f) {
                if (villager.isBurning()) {
                    villager.extinguishFire();
                }
                villager.huirEnPanicoDesde(targetCenterX, targetCenterY, 8.0f);
                panicCount++;
            }
        }

        if (intermediario) {
            intermediario->activarSenalDivina(10.0f);
            sermon = "¡Las compuertas del abismo celestial se abren por orden de " + godName + "! ¡Que las aguas del Diluvio laven y purifiquen la faz del mundo!";
            m_activeSpeeches.push_back({
                intermediario->getId(), interName, interTitle,
                sermon, intermediario->getX(), intermediario->getY(), 0.0f, 6.0f
            });
        }

        const int faithGenerated = static_cast<int>((80 + panicCount * 6 + firesExtinguished * 20) * leaderInfluenceMult);
        pRel->theologyPoints += faithGenerated;

        res.faithGenerated = faithGenerated;
        res.success = true;
        res.message = "[GRAN DILUVIO] ¡Tormenta cataclismica en (" +
                      std::to_string(targetGx) + "," + std::to_string(targetGy) + ")! " +
                      std::to_string(floodedCount) + " celdas anegadas, " +
                      std::to_string(firesExtinguished) + " fuegos sofocados y " +
                      std::to_string(panicCount) + " aldeanos nadando o huyendo a tierras altas.";

        eventLine.addEvent(
            currentYear, EventCategory::MajorLoss,
            "El Gran Diluvio de " + godName,
            "Un diluvio descomunal desatado por " + godName + " sumergio valles en (" + std::to_string(targetGx) + "," + std::to_string(targetGy) +
            "), anegando el terreno y extinguiendo todo rastro de fuego bajo inmensas corrientes.",
            {targetGx, targetGy}, playerRelId, 0, ColorRGB{25, 110, 235},
            true, interName, "Inundación cataclísmica y crecida de aguas"
        );
    } else if (type == DivinePowerType::AtomicBomb) {
        // ======================================================================================
        // EN: CHEAT POWER: ATOMIC BOMB [FUCKINGBOMB] - Nuclear detonation, thermal flash, crater
        // ES: PODER TRUCO: EXPLOSIÓN ATÓMICA [FUCKINGBOMB] - Detonación nuclear Little Boy, destello, cráter
        // ======================================================================================
        const float targetCenterX = static_cast<float>(targetGx) + 0.5f;
        const float targetCenterY = static_cast<float>(targetGy) + 0.5f;
        constexpr float TOTAL_NUCLEAR_BLAST_RADIUS = LittleBoyScale::HEAVY_OVERPRESSURE_RADIUS_CELLS; // 100.0f cells = 500 meters
        constexpr float NUCLEAR_BLAST_RADIUS = 9.0f; // Immediate initial flash devastation zone (t=0)
        constexpr float CRATER_BOWL_RADIUS = 1.6f;
        constexpr float EPICENTER_RADIUS = 3.5f;
        constexpr float INTERMEDIATE_RADIUS = 6.5f;

        m_activeFX.push_back({
            DivinePowerType::AtomicBomb,
            targetGx, targetGy,
            targetCenterX, targetCenterY,
            0.0f, 0.0f,
            22.0f, // EN: 22.0 seconds lifetime for mushroom cloud and expanding radioactive dust / ES: 22.0s para hongo y polvo radiactivo en expansión
            0.0f,
            1.0f,
            TOTAL_NUCLEAR_BLAST_RADIUS, // EN: Full Little Boy 500m heavy blast radius (100.0 cells)
            0, 0, 0,
            static_cast<float>(targetGx * 67 + targetGy * 89 + 31),
            true,  // EN: Detonation triggered immediately on cast / ES: Detonación disparada de inmediato al lanzar
            0.0f   // EN: Secondary timer tracks dynamic shockwave radius / ES: Temporizador auxiliar rastrea radio de onda
        });

        int vaporizedCount = 0;
        int burnedCount = 0;
        int panicCount = 0;
        int felledTreesCount = 0;
        int ruinedHousesCount = 0;

        // EN: 1. Terrain alteration: permanent crater, vitrification, boiling of water, tree felling
        // ES: 1. Alteración del terreno: cráter permanente, vitrificación, ebullición del agua, derribo de árboles
        const int blastRadiusInt = static_cast<int>(std::ceil(NUCLEAR_BLAST_RADIUS));
        for (int deltaCellY = -blastRadiusInt; deltaCellY <= blastRadiusInt; ++deltaCellY) {
            for (int deltaCellX = -blastRadiusInt; deltaCellX <= blastRadiusInt; ++deltaCellX) {
                const float distanceSq = static_cast<float>(deltaCellX * deltaCellX + deltaCellY * deltaCellY);
                if (distanceSq > NUCLEAR_BLAST_RADIUS * NUCLEAR_BLAST_RADIUS) continue;

                const int currentCellX = targetGx + deltaCellX;
                const int currentCellY = targetGy + deltaCellY;
                if (!map.isValid(currentCellX, currentCellY)) continue;

                Cell& cell = map.getCell(currentCellX, currentCellY);
                const float cellDistance = std::sqrt(distanceSq);

                if (cellDistance <= CRATER_BOWL_RADIUS) {
                    // EN: EPICENTER: Form a single colossal impact crater at center; disintegrate all vegetation ("nada más")
                    // ES: EPICENTRO: Formar un único cráter colosal de impacto en el centro; desintegrar toda la vegetación ("nada más")
                    if (deltaCellX == 0 && deltaCellY == 0) {
                        cell.formCrater(CraterType::NuclearDetonation, 1.0f, 0.45f);
                    } else {
                        cell.hasImpactCrater = false;
                    }
                    cell.hasTreesUprooted = false;
                    cell.resourceAmount = 0;
                    cell.naturalResource = ResourceType::None;
                    cell.resourceVariant = 0;
                    if (cell.biome != BiomeType::DeepOcean && cell.biome != BiomeType::ShallowWater) {
                        cell.biome = BiomeType::AridDesert; // EN: Vitrified ash/trinitite / ES: Ceniza vitrificada/trinitita
                    }
                    if (cell.hasHolySite) {
                        cell.hasHolySite = false;
                        cell.shrineLevel = 0;
                    }
                    if (cell.waterDepth > 0.0f || cell.isFlooded) {
                        cell.waterDepth = 0.0f;
                        cell.isFlooded = false;
                        cell.steamTimer = 6.0f; // EN: Massive superheated steam plume / ES: Pluma masiva de vapor sobrecalentado
                    }
                    if (cell.isBurning) {
                        cell.extinguish();
                    }
                    cell.isBurnt = true;
                    cell.elevation = std::max(0.05f, cell.elevation - 0.20f);
                } else if (cellDistance <= INTERMEDIATE_RADIUS) {
                    // EN: INTERMEDIATE ZONE: Flattened trees, thermal ignition of combustible terrain
                    // ES: ZONA INTERMEDIA: Árboles derribados, ignición térmica de terreno combustible
                    if (cell.naturalResource == ResourceType::Wood || cell.biome == BiomeType::DenseForest) {
                        if (!cell.hasTreesUprooted) {
                            cell.hasTreesUprooted = true;
                            cell.resourceAmount = 0;
                            cell.naturalResource = ResourceType::None;
                            felledTreesCount++;

                            const float blastDist = std::max(0.2f, cellDistance);
                            const float blastVectorX = static_cast<float>(deltaCellX) / blastDist;
                            const float blastVectorY = static_cast<float>(deltaCellY) / blastDist;

                            // EN: Launch heavy tree trunk flying across the sky with high ballistic arc
                            // ES: Lanzar tronco pesado volando por el cielo en arco balístico elevado
                            spawnDebris(DebrisType::UprootedTreeTrunk,
                                        static_cast<float>(currentCellX) + 0.5f,
                                        static_cast<float>(currentCellY) + 0.5f,
                                        5.0f, blastVectorX * 20.0f, blastVectorY * 20.0f,
                                        45.0f, 45.0f, 40.0f, 2.5f);

                            // EN: Fling splintered foliage and canopy thatch soaring high into the air
                            // ES: Lanzar follaje astillado y paja de copa volando alto en el aire
                            spawnDebris(DebrisType::RoofThatch,
                                        static_cast<float>(currentCellX) + 0.5f,
                                        static_cast<float>(currentCellY) + 0.5f,
                                        7.0f, blastVectorX * 16.0f + 3.0f, blastVectorY * 16.0f - 2.5f,
                                        58.0f, 15.0f, 18.0f, 1.8f);
                        }
                    }
                    if (cell.isCombustible() && !cell.isBurning && !cell.isWater() &&
                        (((currentCellX + currentCellY) % 3 == 0) || cell.naturalResource == ResourceType::Wood)) {
                        map.igniteCell(currentCellX, currentCellY, 20.0f, 1.0f);
                    }
                } else {
                    // EN: PERIPHERAL ZONE: Thermal pulse scorches dry timber
                    // ES: ZONA PERIFÉRICA: El pulso térmico chamusca madera seca
                    if (cell.isCombustible() && !cell.isBurning && !cell.isWater() && cell.moisture < 0.35f && ((currentCellX + currentCellY) % 4 == 0)) {
                        map.igniteCell(currentCellX, currentCellY, 14.0f, 0.75f);
                    }
                }
            }
        }

        // ======================================================================================
        // EN: 2. Massive crater ejecta salvo blasted skyward by instantaneous nuclear excavation
        // ES: 2. Salva masiva de escombros eyectados al cielo por excavación nuclear instantánea
        // ======================================================================================
        constexpr int CRATER_ROCK_COUNT = 16;
        for (int rockIndex = 0; rockIndex < CRATER_ROCK_COUNT; ++rockIndex) {
            const float rockAngle = (static_cast<float>(rockIndex) / CRATER_ROCK_COUNT) * 6.28318f + (static_cast<float>(targetGx % 7) * 0.4f);
            const float rockSpeed = 16.0f + 14.0f * std::sin(static_cast<float>(rockIndex) * 1.7f + 2.1f);
            const float rockLaunchZ = 45.0f + 35.0f * std::cos(static_cast<float>(rockIndex) * 2.3f + 1.2f);
            const float spawnOffsetX = std::cos(rockAngle) * 0.8f;
            const float spawnOffsetY = std::sin(rockAngle) * 0.8f;
            spawnDebris(DebrisType::RockDebris,
                        targetCenterX + spawnOffsetX, targetCenterY + spawnOffsetY,
                        4.0f, std::cos(rockAngle) * rockSpeed, std::sin(rockAngle) * rockSpeed,
                        rockLaunchZ, 30.0f, 35.0f, 2.2f);
        }

        constexpr int CRATER_SOIL_COUNT = 14;
        for (int soilIndex = 0; soilIndex < CRATER_SOIL_COUNT; ++soilIndex) {
            const float soilAngle = (static_cast<float>(soilIndex) / CRATER_SOIL_COUNT) * 6.28318f + 0.35f;
            const float soilSpeed = 14.0f + 12.0f * std::cos(static_cast<float>(soilIndex) * 1.9f + 0.5f);
            const float soilLaunchZ = 40.0f + 30.0f * std::sin(static_cast<float>(soilIndex) * 2.1f + 0.8f);
            spawnDebris(DebrisType::SoilChunk,
                        targetCenterX, targetCenterY,
                        3.5f, std::cos(soilAngle) * soilSpeed, std::sin(soilAngle) * soilSpeed,
                        soilLaunchZ, 15.0f, 20.0f, 1.5f);
        }

        constexpr int CRATER_SLAG_COUNT = 12;
        for (int slagIndex = 0; slagIndex < CRATER_SLAG_COUNT; ++slagIndex) {
            const float slagAngle = (static_cast<float>(slagIndex) / CRATER_SLAG_COUNT) * 6.28318f + 0.75f;
            const float slagSpeed = 18.0f + 15.0f * std::sin(static_cast<float>(slagIndex) * 1.5f + 1.1f);
            const float slagLaunchZ = 55.0f + 35.0f * std::cos(static_cast<float>(slagIndex) * 1.9f + 2.4f);
            spawnDebris(DebrisType::IncandescentSlag,
                        targetCenterX, targetCenterY,
                        4.5f, std::cos(slagAngle) * slagSpeed, std::sin(slagAngle) * slagSpeed,
                        slagLaunchZ, 20.0f, 28.0f, 1.8f);
        }

        // EN: Atomized water geyser columns if nuclear bomb detonates near water
        // ES: Columnas de géiser de agua atomizada si la bomba nuclear detona cerca de agua
        bool hasNearbyWater = false;
        for (int deltaY = -3; deltaY <= 3; ++deltaY) {
            for (int deltaX = -3; deltaX <= 3; ++deltaX) {
                if (map.isValid(targetGx + deltaX, targetGy + deltaY)) {
                    const Cell& nearCell = map.getCell(targetGx + deltaX, targetGy + deltaY);
                    if (nearCell.waterDepth > 0.05f || nearCell.isWater()) {
                        hasNearbyWater = true;
                        break;
                    }
                }
            }
            if (hasNearbyWater) break;
        }

        if (hasNearbyWater) {
            constexpr int WATER_GEYSER_COUNT = 12;
            for (int waterIndex = 0; waterIndex < WATER_GEYSER_COUNT; ++waterIndex) {
                const float waterAngle = (static_cast<float>(waterIndex) / WATER_GEYSER_COUNT) * 6.28318f;
                const float waterSpeed = 15.0f + 10.0f * std::sin(static_cast<float>(waterIndex) * 1.8f);
                const float waterLaunchZ = 45.0f + 30.0f * std::cos(static_cast<float>(waterIndex) * 2.2f);
                spawnDebris(DebrisType::WaterSprayGeyser,
                            targetCenterX, targetCenterY,
                            3.0f, std::cos(waterAngle) * waterSpeed, std::sin(waterAngle) * waterSpeed,
                            waterLaunchZ, 6.0f, 0.0f, 1.0f);
            }
        }

        // EN: 3. Structural demolition: houses pulverized and debris launched into ballistic flight
        // ES: 3. Demolición estructural: casas pulverizadas y escombros lanzados en vuelo balístico
        if (buildingSystem) {
            for (auto& houseRef : buildingSystem->getHouses()) {
                const float deltaHouseX = static_cast<float>(houseRef.cellX) + 0.5f - targetCenterX;
                const float deltaHouseY = static_cast<float>(houseRef.cellY) + 0.5f - targetCenterY;
                const float houseDistanceSq = deltaHouseX * deltaHouseX + deltaHouseY * deltaHouseY;
                if (houseDistanceSq <= NUCLEAR_BLAST_RADIUS * NUCLEAR_BLAST_RADIUS) {
                    const float houseDistance = std::sqrt(houseDistanceSq);
                    const float shockFactor = 1.0f - (houseDistance / NUCLEAR_BLAST_RADIUS);
                    const float nuclearStructuralDamage = 350.0f * shockFactor;
                    const bool wasRuined = buildingSystem->damageHouse(houseRef.id, nuclearStructuralDamage, map, &humanos);
                    if (wasRuined) {
                        ruinedHousesCount++;
                        const float blastDist = std::max(0.3f, houseDistance);
                        const float dirX = deltaHouseX / blastDist;
                        const float dirY = deltaHouseY / blastDist;

                        // EN: Launch 2 roof thatch bundles flying into the sky
                        // ES: Lanzar 2 bloques de paja del techo volando por los cielos
                        spawnDebris(DebrisType::RoofThatch,
                                    static_cast<float>(houseRef.cellX) + 0.3f,
                                    static_cast<float>(houseRef.cellY) + 0.3f,
                                    8.0f, dirX * 18.0f + 2.0f, dirY * 18.0f - 2.0f,
                                    62.0f, 16.0f, 18.0f, 1.8f);
                        spawnDebris(DebrisType::RoofThatch,
                                    static_cast<float>(houseRef.cellX) + 0.7f,
                                    static_cast<float>(houseRef.cellY) + 0.7f,
                                    9.0f, dirX * 15.0f - 2.5f, dirY * 15.0f + 3.0f,
                                    55.0f, 18.0f, 20.0f, 1.8f);

                        // EN: Launch 2 stone masonry rubble blocks hurtling across the ground
                        // ES: Lanzar 2 bloques de mampostería de piedra volando a través del terreno
                        spawnDebris(DebrisType::HouseRubble,
                                    static_cast<float>(houseRef.cellX) + 0.5f,
                                    static_cast<float>(houseRef.cellY) + 0.5f,
                                    7.0f, dirX * 15.0f, dirY * 15.0f,
                                    42.0f, 38.0f, 35.0f, 2.4f);
                        spawnDebris(DebrisType::HouseRubble,
                                    static_cast<float>(houseRef.cellX) + 0.5f,
                                    static_cast<float>(houseRef.cellY) + 0.5f,
                                    6.0f, dirX * 12.0f - dirY * 4.0f, dirY * 12.0f + dirX * 4.0f,
                                    35.0f, 32.0f, 30.0f, 2.2f);
                    }
                }
            }
        }

        // EN: 3. Caravans obliterated: shockwave destroys traders and launches crates
        // ES: 3. Caravanas destruidas: la onda expansiva destruye mercaderes y lanza fardos
        if (replicationEngine) {
            for (auto& caravanRef : replicationEngine->getActiveCaravansMutable()) {
                const float carPositionX = static_cast<float>(caravanRef.from.x) + (caravanRef.to.x - caravanRef.from.x) * caravanRef.progress;
                const float carPositionY = static_cast<float>(caravanRef.from.y) + (caravanRef.to.y - caravanRef.from.y) * caravanRef.progress;
                const float carDistanceSq = (carPositionX - targetCenterX) * (carPositionX - targetCenterX) +
                                            (carPositionY - targetCenterY) * (carPositionY - targetCenterY);
                if (carDistanceSq <= NUCLEAR_BLAST_RADIUS * NUCLEAR_BLAST_RADIUS) {
                    caravanRef.power = 0.0f;
                    caravanRef.progress = 0.0f;
                    const float deltaCarX = carPositionX - targetCenterX;
                    const float deltaCarY = carPositionY - targetCenterY;
                    const float carDist = std::max(0.3f, std::sqrt(deltaCarX * deltaCarX + deltaCarY * deltaCarY));
                    const float dirX = deltaCarX / carDist;
                    const float dirY = deltaCarY / carDist;

                    // EN: Launch 3 resource crates flying outwards with violent velocity
                    // ES: Lanzar 3 cajas de recursos volando hacia afuera con velocidad violenta
                    spawnDebris(DebrisType::ResourceCrate, carPositionX, carPositionY, 6.0f,
                                dirX * 18.0f, dirY * 18.0f, 45.0f, 25.0f, 22.0f, 1.8f);
                    spawnDebris(DebrisType::ResourceCrate, carPositionX + 0.3f, carPositionY - 0.3f, 7.0f,
                                dirX * 15.0f - 3.0f, dirY * 15.0f + 2.5f, 52.0f, 22.0f, 20.0f, 1.6f);
                    spawnDebris(DebrisType::ResourceCrate, carPositionX - 0.3f, carPositionY + 0.3f, 8.0f,
                                dirX * 13.0f + 3.0f, dirY * 13.0f - 3.0f, 38.0f, 20.0f, 18.0f, 1.5f);
                }
            }
        }

        // EN: 4. Human unit physics: vaporization at epicenter, severe thermal burns & ragdoll
        // ES: 4. Física de unidades humanas: vaporización en epicentro, quemaduras térmicas y ragdoll
        for (auto& villager : humanos.getHumanos()) {
            if (!villager.estaVivo()) continue;
            const float deltaWorldX = villager.getX() - targetCenterX;
            const float deltaWorldY = villager.getY() - targetCenterY;
            const float distanceSq = deltaWorldX * deltaWorldX + deltaWorldY * deltaWorldY;
            if (distanceSq > NUCLEAR_BLAST_RADIUS * NUCLEAR_BLAST_RADIUS) continue;

            const float villagerDistance = std::sqrt(distanceSq);
            const float blastAngle = (villagerDistance > 0.05f) ? std::atan2(deltaWorldY, deltaWorldX)
                                                                 : (static_cast<float>(villager.getId()) * 1.57f);

            if (villagerDistance <= EPICENTER_RADIUS) {
                // EN: 100% instant vaporization, hurled skyward with extreme ballistic momentum
                // ES: 100% vaporización instantánea, arrojado a los cielos con impulso balístico extremo
                villager.matar("Desintegrado y vaporizado instantaneamente en el epicentro nuclear [FUCKINGBOMB]");
                villager.activarRagdoll(std::cos(blastAngle) * 26.0f, std::sin(blastAngle) * 26.0f, 65.0f, 36.0f);
                vaporizedCount++;
            } else if (villagerDistance <= INTERMEDIATE_RADIUS) {
                // EN: Extreme thermal burns, ignited on fire, launched soaring into high ballistic flight
                // ES: Quemaduras térmicas extremas, ignición en llamas, lanzado volando en alta parábola balística
                villager.recibirDano(85.0f);
                villager.ignite(8.0f);
                villager.activarRagdoll(std::cos(blastAngle) * 18.0f, std::sin(blastAngle) * 18.0f, 48.0f, 26.0f);
                villager.huirEnPanicoDesde(targetCenterX, targetCenterY, 10.0f);
                burnedCount++;
            } else {
                // EN: Peripheral supersonic shockwave knockdown and high-velocity fling
                // ES: Derribo por onda expansiva supersónica periférica y lanzamiento a gran velocidad
                villager.recibirDano(35.0f);
                villager.activarRagdoll(std::cos(blastAngle) * 10.0f, std::sin(blastAngle) * 10.0f, 28.0f, 16.0f);
                villager.huirEnPanicoDesde(targetCenterX, targetCenterY, 8.0f);
                panicCount++;
            }
        }

        if (intermediario) {
            intermediario->activarSenalDivina(12.0f);
            sermon = "¡EL PODER SUPREMO DEL ATOMO HA SIDO DESATADO! ¡" + godName + " RECLAMA EL CENIT Y PURIFICA LA TIERRA CON FUEGO ETERNO!";
            m_activeSpeeches.push_back({
                intermediario->getId(), interName, interTitle,
                sermon, intermediario->getX(), intermediario->getY(), 0.0f, 8.0f
            });
        }

        // EN: Faith generated by apocalyptic nuclear awe
        // ES: Fe generada por el sobrecogimiento apocalíptico nuclear
        const int faithGenerated = static_cast<int>((120 + vaporizedCount * 25 + burnedCount * 10 + panicCount * 5) * leaderInfluenceMult);
        pRel->theologyPoints += faithGenerated;

        res.faithGenerated = faithGenerated;
        res.casualties = vaporizedCount;
        res.success = true;
        res.message = "[FUCKINGBOMB] ¡DETONACION ATOMICA EN (" +
                      std::to_string(targetGx) + "," + std::to_string(targetGy) + ")! " +
                      std::to_string(vaporizedCount) + " vaporizados instantaneamente, " +
                      std::to_string(burnedCount) + " quemados por el pulso termico, " +
                      std::to_string(felledTreesCount) + " arboles arrancados, " +
                      std::to_string(ruinedHousesCount) + " casas en ruinas.";

        eventLine.addEvent(
            currentYear, EventCategory::MajorLoss,
            "¡DETONACION ATOMICA [FUCKINGBOMB]!",
            "Un cataclismo nuclear definitivo fue desatado en (" + std::to_string(targetGx) + "," + std::to_string(targetGy) +
            ") por " + godName + ". Un hongo atomico ascendio a los cielos, dejando un crater vitrificado y la desintegracion total del epicentro.",
            {targetGx, targetGy}, playerRelId, 0, ColorRGB{255, 235, 90},
            true, interName, "Cataclismo nuclear total y juicio atómico supremo"
        );
    }

    map.recalculateReligions(registry);
    return res;
}

void DivinePowersSystem::spawnDebris(DebrisType type, float x, float y, float z,
                                     float vx, float vy, float vz, float mass,
                                     float impactDmg, float radius) {
    constexpr size_t MAX_ACTIVE_DEBRIS = 512;
    if (m_activeDebris.size() >= MAX_ACTIVE_DEBRIS) return; // Cap to keep simulation efficient
    PhysicalDebris deb;
    deb.type = type;
    deb.worldX = x;
    deb.worldY = y;
    deb.altitude = z;
    deb.velocityX = vx;
    deb.velocityY = vy;
    deb.velocityZ = vz;
    deb.mass = mass;
    deb.impactDamage = impactDmg;
    deb.damageRadius = radius;
    deb.rotationAngle = 0.0f;
    deb.angularVelocity = ((vx + vy) * 2.5f) + 4.0f;
    deb.settled = false;
    deb.bounces = 0;
    deb.lifeTimer = 0.0f;
    deb.maxLifetime = 14.0f;
    m_activeDebris.push_back(deb);
}

void DivinePowersSystem::update(float dt, WorldMap& map, GestorHumanos& humanos,
                                BuildingSystem* buildingSystem,
                                ReplicationEngine* replicationEngine) {
    if (dt <= 0.0f) return;

    // Actualizar efectos activos y físicas sobre el mapa
    for (auto& fx : m_activeFX) {
        fx.timer += dt;

        if (fx.type == DivinePowerType::Tornado) {
            // Desplazamiento orgánico del tornado a través del mapa
            fx.worldX += fx.velocityX * dt;
            fx.worldY += fx.velocityY * dt;

            // Rebotar suavemente en bordes del mapa
            if (fx.worldX < 2.0f) { fx.worldX = 2.0f; fx.velocityX = std::abs(fx.velocityX); }
            if (fx.worldX > map.getWidth() - 3.0f) { fx.worldX = map.getWidth() - 3.0f; fx.velocityX = -std::abs(fx.velocityX); }
            if (fx.worldY < 2.0f) { fx.worldY = 2.0f; fx.velocityY = std::abs(fx.velocityY); }
            if (fx.worldY > map.getHeight() - 3.0f) { fx.worldY = map.getHeight() - 3.0f; fx.velocityY = -std::abs(fx.velocityY); }

            fx.gridX = static_cast<int>(std::floor(fx.worldX));
            fx.gridY = static_cast<int>(std::floor(fx.worldY));

            // Aplicar físicas de succión y vuelo vorticial a los aldeanos
            for (auto& h : humanos.getHumanos()) {
                if (h.estaVivo()) {
                    h.serSuccionadoPorTornado(fx.worldX, fx.worldY, fx.radius, dt);
                }
            }

            // EN: Affect Trees: uproot standing timber and fling physical tree trunks
            // ES: Afectar Árboles: arrancar madera erguida y lanzar troncos físicos
            const int tornadoIntRadius = static_cast<int>(std::ceil(fx.radius));
            for (int dy = -tornadoIntRadius; dy <= tornadoIntRadius; ++dy) {
                for (int dx = -tornadoIntRadius; dx <= tornadoIntRadius; ++dx) {
                    const int cx = fx.gridX + dx;
                    const int cy = fx.gridY + dy;
                    if (!map.isValid(cx, cy)) continue;
                    const float dsq = static_cast<float>(dx * dx + dy * dy);
                    if (dsq > fx.radius * fx.radius) continue;

                    Cell& cellRef = map.getCell(cx, cy);
                    if (cellRef.naturalResource == ResourceType::Wood || cellRef.biome == BiomeType::DenseForest) {
                        if (!cellRef.hasTreesUprooted) {
                            cellRef.hasTreesUprooted = true;
                            cellRef.resourceAmount = std::max(0, cellRef.resourceAmount - static_cast<int>(35.0f * dt));
                            const float dist = std::max(0.1f, std::sqrt(dsq));
                            const float tangX = -static_cast<float>(dy) / dist;
                            const float tangY = static_cast<float>(dx) / dist;
                            spawnDebris(DebrisType::UprootedTreeTrunk,
                                        static_cast<float>(cx) + 0.5f, static_cast<float>(cy) + 0.5f,
                                        3.0f, tangX * 6.5f, tangY * 6.5f, 22.0f, 40.0f, 30.0f, 1.8f);
                        } else if (cellRef.resourceAmount > 0) {
                            cellRef.resourceAmount = std::max(0, cellRef.resourceAmount - static_cast<int>(35.0f * dt));
                        }
                    }
                }
            }

            // EN: Affect Houses: rip thatched roofs and collapse walls
            // ES: Afectar Casas: arrancar techumbres de paja y colapsar muros
            if (buildingSystem) {
                for (auto& house : buildingSystem->getHouses()) {
                    const float deltaHouseX = static_cast<float>(house.cellX) + 0.5f - fx.worldX;
                    const float deltaHouseY = static_cast<float>(house.cellY) + 0.5f - fx.worldY;
                    const float houseDistSq = deltaHouseX * deltaHouseX + deltaHouseY * deltaHouseY;
                    if (houseDistSq <= fx.radius * fx.radius) {
                        const float houseDist = std::sqrt(houseDistSq);
                        const float houseWindDamage = 35.0f * (1.0f - (houseDist / fx.radius)) * dt;

                        std::uniform_real_distribution<float> dist01(0.0f, 1.0f);
                        if (dist01(g_divineRng) < 0.20f) {
                            spawnDebris(DebrisType::RoofThatch,
                                        static_cast<float>(house.cellX) + 0.5f, static_cast<float>(house.cellY) + 0.5f,
                                        6.0f, fx.velocityX * 0.7f, fx.velocityY * 0.7f, 18.0f, 8.0f, 10.0f, 1.2f);
                        }

                        const bool wasRuined = buildingSystem->damageHouse(house.id, houseWindDamage, map, &humanos);
                        if (wasRuined) {
                            spawnDebris(DebrisType::HouseRubble,
                                        static_cast<float>(house.cellX) + 0.5f, static_cast<float>(house.cellY) + 0.5f,
                                        5.0f, deltaHouseX * 1.5f, deltaHouseY * 1.5f, 14.0f, 25.0f, 25.0f, 1.8f);
                        }
                    }
                }
            }

            // EN: Affect Movable Caravans: derail progress and scatter goods
            // ES: Afectar Caravanas Móviles: descarrilar avance y dispersar mercancías
            if (replicationEngine) {
                for (auto& caravan : replicationEngine->getActiveCaravansMutable()) {
                    const float carX = static_cast<float>(caravan.from.x) + (caravan.to.x - caravan.from.x) * caravan.progress;
                    const float carY = static_cast<float>(caravan.from.y) + (caravan.to.y - caravan.from.y) * caravan.progress;
                    const float carDistSq = (carX - fx.worldX) * (carX - fx.worldX) + (carY - fx.worldY) * (carY - fx.worldY);
                    if (carDistSq <= fx.radius * fx.radius) {
                        caravan.progress = std::max(0.0f, caravan.progress - 0.25f * dt);
                        caravan.power = std::max(0.0f, caravan.power - 15.0f * dt);
                        spawnDebris(DebrisType::ResourceCrate, carX, carY, 4.0f, (carX - fx.worldX) * 2.5f, (carY - fx.worldY) * 2.5f, 16.0f, 18.0f, 14.0f, 1.2f);
                    }
                }
            }
        } else if (fx.type == DivinePowerType::Earthquake) {
            // Pulso continuo del sismo derribando aldeanos y empujando terreno en su radio
            for (auto& h : humanos.getHumanos()) {
                if (h.estaVivo()) {
                    float dx = h.getX() - fx.worldX;
                    float dy = h.getY() - fx.worldY;
                    float dsq = dx * dx + dy * dy;
                    if (dsq <= fx.radius * fx.radius) {
                        h.sufrirSismoTerremoto(fx.worldX, fx.worldY, fx.intensity * 0.4f);
                        if (h.estaVivo()) {
                            float pushAngle = std::atan2(dy, dx);
                            h.desplazarPorSismo(std::cos(pushAngle) * 0.4f * dt, std::sin(pushAngle) * 0.4f * dt, map);
                        }
                    }
                }
            }

            // EN: Affect Trees & Slopes: topple hillside timber and create falling logs
            // ES: Afectar Árboles y Laderas: derribar madera en pendientes y generar troncos rodantes
            const int quakeRadius = static_cast<int>(std::ceil(fx.radius));
            for (int dy = -quakeRadius; dy <= quakeRadius; ++dy) {
                for (int dx = -quakeRadius; dx <= quakeRadius; ++dx) {
                    const int cx = fx.gridX + dx;
                    const int cy = fx.gridY + dy;
                    if (!map.isValid(cx, cy)) continue;
                    if (dx * dx + dy * dy > fx.radius * fx.radius) continue;

                    Cell& cellRef = map.getCell(cx, cy);
                    if ((cellRef.naturalResource == ResourceType::Wood || cellRef.biome == BiomeType::DenseForest) &&
                        (cellRef.elevation >= 1.4f || fx.intensity >= 1.2f)) {
                        if (!cellRef.hasTreesUprooted) {
                            cellRef.hasTreesUprooted = true;
                            spawnDebris(DebrisType::UprootedTreeTrunk,
                                        static_cast<float>(cx) + 0.5f, static_cast<float>(cy) + 0.5f,
                                        2.0f, static_cast<float>(dx) * 0.8f, static_cast<float>(dy) * 0.8f, 6.0f, 35.0f, 22.0f, 1.5f);
                        }
                    }
                }
            }

            // EN: Affect Houses: seismic ground shear damages foundation and collapses structures
            // ES: Afectar Casas: cizalla sísmica del suelo daña cimientos y colapsa estructuras
            if (buildingSystem) {
                for (auto& house : buildingSystem->getHouses()) {
                    const float deltaHouseX = static_cast<float>(house.cellX) + 0.5f - fx.worldX;
                    const float deltaHouseY = static_cast<float>(house.cellY) + 0.5f - fx.worldY;
                    const float houseDistSq = deltaHouseX * deltaHouseX + deltaHouseY * deltaHouseY;
                    if (houseDistSq <= fx.radius * fx.radius) {
                        const float houseDist = std::sqrt(houseDistSq);
                        const float falloff = 1.0f - (houseDist / fx.radius);
                        const float quakeDamage = (24.0f * falloff * fx.intensity) * dt;

                        const bool wasRuined = buildingSystem->damageHouse(house.id, quakeDamage, map, &humanos);
                        if (wasRuined) {
                            spawnDebris(DebrisType::HouseRubble,
                                        static_cast<float>(house.cellX) + 0.5f, static_cast<float>(house.cellY) + 0.5f,
                                        3.0f, deltaHouseX * 0.6f, deltaHouseY * 0.6f, 8.0f, 25.0f, 20.0f, 1.6f);
                        }
                    }
                }
            }

            // EN: Affect Caravans: seismic shaking stalls trade/missionary passage
            // ES: Afectar Caravanas: sacudida sísmica estanca el paso comercial y misionero
            if (replicationEngine) {
                for (auto& caravan : replicationEngine->getActiveCaravansMutable()) {
                    const float carX = static_cast<float>(caravan.from.x) + (caravan.to.x - caravan.from.x) * caravan.progress;
                    const float carY = static_cast<float>(caravan.from.y) + (caravan.to.y - caravan.from.y) * caravan.progress;
                    if ((carX - fx.worldX) * (carX - fx.worldX) + (carY - fx.worldY) * (carY - fx.worldY) <= fx.radius * fx.radius) {
                        caravan.power = std::max(0.0f, caravan.power - 10.0f * dt);
                    }
                }
            }
        } else if (fx.type == DivinePowerType::Pandemic) {
            // Contagiar aldeanos que entren en la nube del brote
            for (auto& h : humanos.getHumanos()) {
                if (h.estaVivo() && !h.isInfected()) {
                    float dx = h.getX() - fx.worldX;
                    float dy = h.getY() - fx.worldY;
                    if (dx * dx + dy * dy <= fx.radius * fx.radius) {
                        h.infectarConPandemia(16.0f);
                    }
                }
            }
            // Reducción poblacional en el foco
            int cx = std::clamp(fx.gridX, 0, map.getWidth() - 1);
            int cy = std::clamp(fx.gridY, 0, map.getHeight() - 1);
            Cell& c = map.getCell(cx, cy);
            if (c.population > 20) {
                c.population = std::max(20, c.population - static_cast<int>(18.0f * dt));
            }
        } else if (fx.type == DivinePowerType::Meteor) {
            const float IMPACT_TIME = 1.25f;
            if (fx.timer < IMPACT_TIME) {
                // Durante el descenso atmosférico, los aldeanos dentro del radio huyen en pánico
                for (auto& h : humanos.getHumanos()) {
                    if (h.estaVivo()) {
                        float dx = h.getX() - fx.worldX;
                        float dy = h.getY() - fx.worldY;
                        if (dx * dx + dy * dy <= (fx.radius + 1.5f) * (fx.radius + 1.5f)) {
                            h.huirEnPanicoDesde(fx.worldX, fx.worldY, 7.0f);
                        }
                    }
                }
            } else if (!fx.impactTriggered) {
                // ¡MOMENTO EXACTO DEL IMPACTO CATACLÍSMICO!
                fx.impactTriggered = true;
                std::uniform_real_distribution<float> dist01(0.0f, 1.0f);

                // 1. Devastación del terreno y formación del cráter permanente
                int cx = std::clamp(fx.gridX, 0, map.getWidth() - 1);
                int cy = std::clamp(fx.gridY, 0, map.getHeight() - 1);

                // EN: Excavate simple bowl impact crater matching average meteorite scale (16.5m diameter = 1.65 cells radius)
                // ES: Excavar cráter de cuenca simple según escala de meteorito promedio (16.5m diámetro = 1.65 celdas radio)
                map.createImpactCraterZone(cx, cy, CraterType::MeteorImpact,
                                           AverageMeteoriteScale::CRATER_RADIUS_CELLS,
                                           AverageMeteoriteScale::CRATER_DEPTH_ELEVATION,
                                           0.40f);

                Cell& centerCell = map.getCell(cx, cy);
                centerCell.resourceAmount = 0;
                centerCell.naturalResource = ResourceType::None;
                if (centerCell.hasHolySite) {
                    centerCell.shrineLevel = std::max(0, centerCell.shrineLevel - 2);
                    if (centerCell.shrineLevel == 0) centerCell.hasHolySite = false;
                }

                // EN: 2. Overturned rock flaps and continuous ejecta blanket (radius 6.5 cells = 32.5 m)
                // Spawn ballistic RockDebris boulders and SoilChunk clods radiating outwards
                // ES: 2. Solapas de roca invertidas y manto continuo de eyección (radio 6.5 celdas = 32.5 m)
                // Generar peñascos de roca balística y terrones de tierra irradiando hacia afuera
                constexpr int METEOR_EJECTA_ROCKS = 12;
                for (int rockIndex = 0; rockIndex < METEOR_EJECTA_ROCKS; ++rockIndex) {
                    const float rockAngle = (static_cast<float>(rockIndex) / METEOR_EJECTA_ROCKS) * 6.2831853f + fx.seed;
                    const float rockSpeed = 12.0f + 6.0f * std::sin(static_cast<float>(rockIndex) * 1.7f);
                    const float rockLaunchZ = 26.0f + 14.0f * std::cos(static_cast<float>(rockIndex) * 2.1f);
                    spawnDebris(DebrisType::RockDebris,
                                fx.worldX + std::cos(rockAngle) * 0.8f,
                                fx.worldY + std::sin(rockAngle) * 0.8f,
                                3.0f, std::cos(rockAngle) * rockSpeed, std::sin(rockAngle) * rockSpeed,
                                rockLaunchZ, 30.0f, 25.0f, 1.8f);
                }

                constexpr int METEOR_EJECTA_SOIL = 10;
                for (int soilIndex = 0; soilIndex < METEOR_EJECTA_SOIL; ++soilIndex) {
                    const float soilAngle = (static_cast<float>(soilIndex) / METEOR_EJECTA_SOIL) * 6.2831853f + 0.45f;
                    const float soilSpeed = 9.0f + 5.0f * std::cos(static_cast<float>(soilIndex) * 1.9f);
                    const float soilLaunchZ = 22.0f + 10.0f * std::sin(static_cast<float>(soilIndex) * 2.3f);
                    spawnDebris(DebrisType::SoilChunk,
                                fx.worldX, fx.worldY,
                                2.5f, std::cos(soilAngle) * soilSpeed, std::sin(soilAngle) * soilSpeed,
                                soilLaunchZ, 15.0f, 16.0f, 1.4f);
                }

                // EN: Devastate forest timber, topple trees, scatter trunks, AND ignite surroundings
                // ES: Devastar madera forestal, derribar árboles, dispersar troncos E incendiar los alrededores
                const int meteorRadiusInt = static_cast<int>(std::ceil(fx.radius));
                for (int offsetCellY = -meteorRadiusInt; offsetCellY <= meteorRadiusInt; ++offsetCellY) {
                    for (int offsetCellX = -meteorRadiusInt; offsetCellX <= meteorRadiusInt; ++offsetCellX) {
                        const int neighborCellX = cx + offsetCellX;
                        const int neighborCellY = cy + offsetCellY;
                        if (!map.isValid(neighborCellX, neighborCellY)) continue;
                        const float cellDistSq = static_cast<float>(offsetCellX * offsetCellX + offsetCellY * offsetCellY);
                        if (cellDistSq > fx.radius * fx.radius) continue;

                        Cell& neighborCell = map.getCell(neighborCellX, neighborCellY);

                        if (neighborCell.naturalResource == ResourceType::Wood || neighborCell.biome == BiomeType::DenseForest) {
                            if (!neighborCell.hasTreesUprooted) {
                                neighborCell.hasTreesUprooted = true;
                                neighborCell.resourceAmount = 0;
                                neighborCell.naturalResource = ResourceType::None;

                                const float blastDist = std::max(0.2f, std::sqrt(cellDistSq));
                                const float blastVectorX = static_cast<float>(offsetCellX) / blastDist;
                                const float blastVectorY = static_cast<float>(offsetCellY) / blastDist;
                                spawnDebris(DebrisType::UprootedTreeTrunk,
                                            static_cast<float>(neighborCellX) + 0.5f,
                                            static_cast<float>(neighborCellY) + 0.5f,
                                            4.0f, blastVectorX * 9.5f, blastVectorY * 9.5f,
                                            26.0f, 40.0f, 35.0f, 2.0f);
                            }
                        }

                        // EN: The incandescent thermal flash ignites combustible terrain around crater rim
                        // ES: El destello térmico incandescente enciende terreno combustible en el borde del cráter
                        if (neighborCell.isCombustible() && !neighborCell.isBurning && !neighborCell.isWater()) {
                            map.igniteCell(neighborCellX, neighborCellY, 16.0f, 1.0f);
                        }
                    }
                }

                // EN: Affect Houses: supersonic shockwave smashes homes into ruins and expels rubble
                // ES: Afectar Casas: onda expansiva supersónica destruye hogares en ruinas y expulsa escombros
                if (buildingSystem) {
                    for (auto& houseRef : buildingSystem->getHouses()) {
                        const float deltaHouseX = static_cast<float>(houseRef.cellX) + 0.5f - fx.worldX;
                        const float deltaHouseY = static_cast<float>(houseRef.cellY) + 0.5f - fx.worldY;
                        const float houseDistanceSq = deltaHouseX * deltaHouseX + deltaHouseY * deltaHouseY;
                        if (houseDistanceSq <= fx.radius * fx.radius) {
                            const float houseDistance = std::sqrt(houseDistanceSq);
                            const float shockFactor = 1.0f - (houseDistance / fx.radius);
                            const float meteorStructuralDamage = 175.0f * shockFactor;
                            const bool wasRuined = buildingSystem->damageHouse(houseRef.id, meteorStructuralDamage, map, &humanos);
                            if (wasRuined) {
                                spawnDebris(DebrisType::HouseRubble,
                                            static_cast<float>(houseRef.cellX) + 0.5f,
                                            static_cast<float>(houseRef.cellY) + 0.5f,
                                            6.0f, deltaHouseX * 2.2f, deltaHouseY * 2.2f,
                                            22.0f, 30.0f, 30.0f, 2.0f);
                            }
                        }
                    }
                }

                // EN: Affect Movable Caravans: shockwave decimates caravans and scatters cargo
                // ES: Afectar Caravanas Móviles: onda expansiva diezma caravanas y dispersa mercancías
                if (replicationEngine) {
                    for (auto& caravanRef : replicationEngine->getActiveCaravansMutable()) {
                        const float carPositionX = static_cast<float>(caravanRef.from.x) + (caravanRef.to.x - caravanRef.from.x) * caravanRef.progress;
                        const float carPositionY = static_cast<float>(caravanRef.from.y) + (caravanRef.to.y - caravanRef.from.y) * caravanRef.progress;
                        const float carDistanceSq = (carPositionX - fx.worldX) * (carPositionX - fx.worldX) +
                                                    (carPositionY - fx.worldY) * (carPositionY - fx.worldY);
                        if (carDistanceSq <= fx.radius * fx.radius) {
                            caravanRef.power = 0.0f;
                            caravanRef.progress = std::max(0.0f, caravanRef.progress - 0.50f);
                            spawnDebris(DebrisType::ResourceCrate, carPositionX, carPositionY, 5.0f,
                                        (carPositionX - fx.worldX) * 3.0f, (carPositionY - fx.worldY) * 3.0f,
                                        20.0f, 20.0f, 15.0f, 1.5f);
                        }
                    }
                }

                // EN: 6. Blast physics, ragdoll ejection, and lethality zones
                // ES: 6. Físicas de explosión, eyección en ragdoll y zonas de letalidad
                for (auto& villager : humanos.getHumanos()) {
                    if (!villager.estaVivo()) continue;
                    const float deltaWorldX = villager.getX() - fx.worldX;
                    const float deltaWorldY = villager.getY() - fx.worldY;
                    const float distanceSq = deltaWorldX * deltaWorldX + deltaWorldY * deltaWorldY;
                    if (distanceSq > fx.radius * fx.radius) continue;

                    const float distanceToImpact = std::sqrt(distanceSq);
                    const float blastAngle = (distanceToImpact > 0.05f) ? std::atan2(deltaWorldY, deltaWorldX)
                                                                        : (static_cast<float>(villager.getId()) * 1.57f);
                    const float casualtyRoll = dist01(g_divineRng);

                    if (distanceToImpact <= AverageMeteoriteScale::CRATER_RADIUS_CELLS * 1.15f) {
                        // EN: EPICENTER (16.5m crater): 95% lethality, violent ragdoll ejection
                        // ES: EPICENTRO (cráter 16.5m): 95% letalidad, eyección violenta en ragdoll
                        if (casualtyRoll < 0.95f) {
                            villager.matar("Vaporizado en el epicentro cosmico del Meteorito (95% mort.)");
                            villager.activarRagdoll(std::cos(blastAngle) * 6.5f, std::sin(blastAngle) * 6.5f, 44.0f, 24.0f);
                            std::cout << "[Impacto Meteorito] ¡" << villager.getNombre() << " fue calcinado instantaneamente en el epicentro!" << std::endl;
                        } else {
                            // EN: Miraculous survivor converted to fanatic
                            // ES: Superviviente milagroso convertido en fanático
                            villager.recibirDano(80.0f);
                            villager.activarRagdoll(std::cos(blastAngle) * 4.5f, std::sin(blastAngle) * 4.5f, 32.0f, 18.0f);
                            villager.setDevocion(1.0f);
                            villager.promoverAFanatico();
                            villager.triggerFaithIndicator(+1, 2.5f);
                            std::cout << "[Supervivencia Milagrosa] ¡" << villager.getNombre() << " sobrevivio al impacto del meteorito y se convirtio en Fanatico!" << std::endl;
                        }
                    } else if (distanceToImpact <= AverageMeteoriteScale::EJECTA_RADIUS_CELLS) {
                        // EN: EJECTA APRON ZONE (32.5m = 6.5 cells): 50% lethality, high-momentum ragdoll
                        // ES: ZONA DE MANTO DE EYECCIÓN (32.5m = 6.5 celdas): 50% letalidad, ragdoll de alto impulso
                        if (casualtyRoll < 0.50f) {
                            villager.matar("Aplastado por la onda de choque supersonica del Meteorito (50% mort.)");
                            villager.activarRagdoll(std::cos(blastAngle) * 4.2f, std::sin(blastAngle) * 4.2f, 26.0f, 16.0f);
                        } else {
                            villager.recibirDano(55.0f);
                            villager.activarRagdoll(std::cos(blastAngle) * 3.2f, std::sin(blastAngle) * 3.2f, 19.0f, 13.0f);
                            villager.huirEnPanicoDesde(fx.worldX, fx.worldY, 7.5f);
                        }
                    } else {
                        // EN: PERIPHERAL AIRBLAST ZONE (50m = 10.0 cells): 15% lethality, knockdown and stun
                        // ES: ZONA PERIFÉRICA DE ONDA AÉREA (50m = 10.0 celdas): 15% letalidad, derribo y aturdimiento
                        if (casualtyRoll < 0.15f) {
                            villager.matar("Derribado mortalmente por esquirlas del impacto de Meteorito (15% mort.)");
                            villager.activarRagdoll(std::cos(blastAngle) * 2.5f, std::sin(blastAngle) * 2.5f, 15.0f, 9.0f);
                        } else {
                            villager.recibirDano(25.0f);
                            villager.activarRagdoll(std::cos(blastAngle) * 1.8f, std::sin(blastAngle) * 1.8f, 11.0f, 7.0f);
                            villager.huirEnPanicoDesde(fx.worldX, fx.worldY, 6.5f);
                        }
                    }
                }
            }
        } else if (fx.type == DivinePowerType::Volcano) {
            // El volcán activo pulsa magma, ríos de lava y bombas volcánicas
            int cx = std::clamp(fx.gridX, 0, map.getWidth() - 1);
            int cy = std::clamp(fx.gridY, 0, map.getHeight() - 1);
            Cell& centerCell = map.getCell(cx, cy);
            centerCell.hasVolcano = true;
            centerCell.hasActiveLava = true;
            // EN: Volcanic cone crater remains active with lava reservoir
            // ES: El cráter del cono volcánico permanece activo con reservorio de lava
            centerCell.lavaTimer = 45.0f;
            centerCell.hasTreesUprooted = true;

            // Expansión progresiva de la lava hacia las celdas adyacentes
            int lavaRadius = std::clamp(static_cast<int>(fx.timer / 4.0f) + 1, 1, 3);
            for (int dy = -lavaRadius; dy <= lavaRadius; ++dy) {
                for (int dx = -lavaRadius; dx <= lavaRadius; ++dx) {
                    int nx = cx + dx;
                    int ny = cy + dy;
                    if (!map.isValid(nx, ny)) continue;
                    float d = std::sqrt(static_cast<float>(dx * dx + dy * dy));
                    if (d > static_cast<float>(lavaRadius)) continue;

                    Cell& nc = map.getCell(nx, ny);

                    // Si la lava toca el océano o aguas someras, ¡forma tierra sólida de basalto (expande la isla)!
                    if (nc.biome == BiomeType::DeepOcean || nc.biome == BiomeType::ShallowWater) {
                        nc.biome = BiomeType::CoastalPlains; // Basalto enfriado en tierra emergida
                        nc.elevation = 0.85f;
                        nc.hasActiveLava = false;
                    } else if (d <= 2.2f) {
                        nc.hasActiveLava = true;
                        // EN: 35s active lava countdown before solidifying into basalt rock
                        // ES: 35s de cuenta regresiva de lava activa antes de solidificarse en roca basáltica
                        nc.lavaTimer = 35.0f;
                        if (nc.naturalResource == ResourceType::Wood || nc.biome == BiomeType::DenseForest) {
                            nc.hasTreesUprooted = true;
                        }
                        if (nc.naturalResource != ResourceType::None) {
                            nc.resourceAmount = std::max(0, nc.resourceAmount - static_cast<int>(35.0f * dt));
                            if (nc.resourceAmount == 0) nc.naturalResource = ResourceType::None;
                        }
                        // EN: Damage houses overrun by molten volcanic lava
                        // ES: Dañar casas invadidas por lava volcánica ardiente
                        if (buildingSystem) {
                            buildingSystem->damageHouseAt(nx, ny, 45.0f * dt, map, &humanos);
                        }
                        // EN: Molten lava ignites surrounding combustible terrain
                        // ES: La lava fundida enciende terreno combustible circundante
                        if (nc.isCombustible() && !nc.isBurning) {
                            map.igniteCell(nx, ny, 16.0f, 1.0f);
                        }
                    }
                }
            }

            // Quemar aldeanos en la zona de lava y generar pánico
            for (auto& h : humanos.getHumanos()) {
                if (!h.estaVivo()) continue;
                float dx = h.getX() - fx.worldX;
                float dy = h.getY() - fx.worldY;
                float distSq = dx * dx + dy * dy;
                if (distSq <= fx.radius * fx.radius) {
                    float dist = std::sqrt(distSq);
                    h.huirEnPanicoDesde(fx.worldX, fx.worldY, 7.5f);
                    if (dist <= 2.5f) {
                        h.recibirDano(28.0f * dt);
                        if (h.estaMuerto()) {
                            float blastAngle = (dist > 0.05f) ? std::atan2(dy, dx) : 0.0f;
                            h.activarRagdoll(std::cos(blastAngle) * 2.5f, std::sin(blastAngle) * 2.5f, 15.0f, 10.0f);
                        }
                    }
                }
            }

            // EN: Eject ballistic pyroclastic bombs into the air
            // ES: Expulsar bombas piroclásticas balísticas por el aire
            fx.secondaryTimer += dt;
            if (fx.secondaryTimer >= 2.0f && m_activeBombs.size() < 8) {
                fx.secondaryTimer = 0.0f;
                std::uniform_real_distribution<float> dist01(0.0f, 1.0f);
                float bombAngle = dist01(g_divineRng) * 6.283f;
                float bombLaunchSpeed = 1.8f + dist01(g_divineRng) * 2.2f;
                PyroclasticBomb bomb;
                bomb.worldX = fx.worldX;
                bomb.worldY = fx.worldY;
                bomb.altitude = 12.0f;
                bomb.velocityX = std::cos(bombAngle) * bombLaunchSpeed;
                bomb.velocityY = std::sin(bombAngle) * bombLaunchSpeed;
                bomb.velocityZ = 26.0f + dist01(g_divineRng) * 12.0f;
                bomb.blastRadius = 2.2f;
                m_activeBombs.push_back(bomb);
            }
        } else if (fx.type == DivinePowerType::Wildfire) {
            // CATÁSTROFE 6: INCENDIO FORESTAL (Foco Sagrado, Calor y Pánico)
            // EN: Stoke thermal energy in epicenter; global spread is managed by WorldMap::updateFirePhysics
            // ES: Avivar energía térmica en el epicentro; la expansión global la maneja WorldMap::updateFirePhysics
            int minX = std::max(0, fx.gridX - 5);
            int maxX = std::min(map.getWidth() - 1, fx.gridX + 5);
            int minY = std::max(0, fx.gridY - 5);
            int maxY = std::min(map.getHeight() - 1, fx.gridY + 5);

            for (int cy = minY; cy <= maxY; ++cy) {
                for (int cx = minX; cx <= maxX; ++cx) {
                    Cell& c = map.getCell(cx, cy);
                    if (c.isBurning) {
                        c.heatAccumulator = std::min(2.0f, c.heatAccumulator + 0.30f * dt);
                    }
                }
            }

            // Pánico y daño por fuego a los aldeanos en el radio divino
            for (auto& h : humanos.getHumanos()) {
                if (!h.estaVivo()) continue;
                float dx = h.getX() - fx.worldX;
                float dy = h.getY() - fx.worldY;
                float distSq = dx * dx + dy * dy;

                if (distSq <= (fx.radius + 2.0f) * (fx.radius + 2.0f)) {
                    h.huirEnPanicoDesde(fx.worldX, fx.worldY, 8.0f);
                    int hx = std::clamp(static_cast<int>(h.getX()), 0, map.getWidth() - 1);
                    int hy = std::clamp(static_cast<int>(h.getY()), 0, map.getHeight() - 1);
                    if (map.getCell(hx, hy).isBurning || distSq <= 2.25f) {
                        h.recibirDano(35.0f * dt);
                        if (!h.isBurning()) {
                            h.ignite(6.0f);
                        }
                        if (h.estaMuerto()) {
                            float blastAngle = (distSq > 0.05f) ? std::atan2(dy, dx) : 0.0f;
                            h.activarRagdoll(std::cos(blastAngle) * 2.2f, std::sin(blastAngle) * 2.2f, 16.0f, 10.0f);
                            h.matar("Calcinado por el frente de fuego del incendio forestal");
                        }
                    }
                }
            }

            // EN: Affect Caravans: forest fire flames impede missionary caravan progress
            // ES: Afectar Caravanas: las llamas del incendio forestal frenan el avance de la caravana misionera
            if (replicationEngine) {
                for (auto& caravanRef : replicationEngine->getActiveCaravansMutable()) {
                    const float carPositionX = static_cast<float>(caravanRef.from.x) + (caravanRef.to.x - caravanRef.from.x) * caravanRef.progress;
                    const float carPositionY = static_cast<float>(caravanRef.from.y) + (caravanRef.to.y - caravanRef.from.y) * caravanRef.progress;
                    const int carCellX = std::clamp(static_cast<int>(carPositionX), 0, map.getWidth() - 1);
                    const int carCellY = std::clamp(static_cast<int>(carPositionY), 0, map.getHeight() - 1);
                    if (map.getCell(carCellX, carCellY).isBurning) {
                        caravanRef.power = std::max(0.0f, caravanRef.power - 25.0f * dt);
                        caravanRef.progress = std::max(0.0f, caravanRef.progress - 0.15f * dt);
                    }
                }
            }
        } else if (fx.type == DivinePowerType::Rain) {
            // EN: Blessed Rain: continuous gentle rainfall moistens soil, quenches fires with steam, and heals living villagers
            // ES: Lluvia Bendita: lluvia suave continua humedece el suelo, apaga fuegos con vapor y sana a los aldeanos vivos
            const int centerCellX = std::clamp(fx.gridX, 0, map.getWidth() - 1);
            const int centerCellY = std::clamp(fx.gridY, 0, map.getHeight() - 1);
            const int effectRadius = static_cast<int>(std::ceil(fx.radius));

            constexpr float RAIN_WATER_PRECIPITATION_RATE = 0.08f;
            constexpr float RAIN_MOISTURE_ACCUMULATION_RATE = 0.40f;
            constexpr float RAIN_FIRE_EXTINGUISH_RATE = 0.90f;
            constexpr float RAIN_VILLAGER_HEAL_RATE = 4.0f;

            for (int deltaCellY = -effectRadius; deltaCellY <= effectRadius; ++deltaCellY) {
                for (int deltaCellX = -effectRadius; deltaCellX <= effectRadius; ++deltaCellX) {
                    const int targetCellX = centerCellX + deltaCellX;
                    const int targetCellY = centerCellY + deltaCellY;
                    if (!map.isValid(targetCellX, targetCellY)) continue;

                    const float distanceToCenter = std::sqrt(static_cast<float>(deltaCellX * deltaCellX + deltaCellY * deltaCellY));
                    if (distanceToCenter > fx.radius) continue;

                    const float radialFalloff = 1.0f - (distanceToCenter / (fx.radius + 0.1f));
                    Cell& targetCell = map.getCell(targetCellX, targetCellY);

                    // EN: Accumulate water depth and raise environmental moisture
                    // ES: Acumular profundidad de agua y elevar la humedad ambiental
                    map.addWaterToCell(targetCellX, targetCellY, RAIN_WATER_PRECIPITATION_RATE * radialFalloff * dt);
                    targetCell.moisture = std::min(1.0f, targetCell.moisture + RAIN_MOISTURE_ACCUMULATION_RATE * dt);

                    // EN: Gradually quench fire and produce steam puffs when extinguished
                    // ES: Apagar el fuego gradualmente y producir bocanadas de vapor al extinguirse
                    if (targetCell.isBurning) {
                        targetCell.fireIntensity = std::max(0.0f, targetCell.fireIntensity - RAIN_FIRE_EXTINGUISH_RATE * dt);
                        targetCell.heatAccumulator = std::max(0.0f, targetCell.heatAccumulator - 0.70f * dt);
                        if (targetCell.fireIntensity <= 0.01f) {
                            targetCell.extinguish();
                            targetCell.steamTimer = 2.0f;
                        }
                    } else if (targetCell.heatAccumulator > 0.0f) {
                        targetCell.heatAccumulator = std::max(0.0f, targetCell.heatAccumulator - 0.50f * dt);
                    }
                }
            }

            // EN: Blessed rain refreshes living villagers, heals health and quenches burning bodies
            // ES: La lluvia bendita refresca a los aldeanos vivos, restaura salud y apaga cuerpos en llamas
            for (auto& villager : humanos.getHumanos()) {
                if (!villager.estaVivo()) continue;
                const float distanceDeltaX = villager.getX() - fx.worldX;
                const float distanceDeltaY = villager.getY() - fx.worldY;
                if (distanceDeltaX * distanceDeltaX + distanceDeltaY * distanceDeltaY <= fx.radius * fx.radius) {
                    if (villager.isBurning()) {
                        villager.extinguishFire();
                    }
                    if (villager.getHealth() < 100.0f) {
                        villager.heal(RAIN_VILLAGER_HEAL_RATE * dt);
                    }
                }
            }
        } else if (fx.type == DivinePowerType::Deluge) {
            // EN: Great Deluge: massive torrential downpour, rapid inundation, instant fire suppression with dense steam
            // ES: Gran Diluvio: aguacero torrencial masivo, rápida inundación, supresión instantánea del fuego con vapor denso
            const int centerCellX = std::clamp(fx.gridX, 0, map.getWidth() - 1);
            const int centerCellY = std::clamp(fx.gridY, 0, map.getHeight() - 1);
            const int effectRadius = static_cast<int>(std::ceil(fx.radius));

            constexpr float DELUGE_WATER_PRECIPITATION_RATE = 0.35f;

            for (int deltaCellY = -effectRadius; deltaCellY <= effectRadius; ++deltaCellY) {
                for (int deltaCellX = -effectRadius; deltaCellX <= effectRadius; ++deltaCellX) {
                    const int targetCellX = centerCellX + deltaCellX;
                    const int targetCellY = centerCellY + deltaCellY;
                    if (!map.isValid(targetCellX, targetCellY)) continue;

                    const float distanceToCenter = std::sqrt(static_cast<float>(deltaCellX * deltaCellX + deltaCellY * deltaCellY));
                    if (distanceToCenter > fx.radius) continue;

                    const float radialFactor = 1.0f - (distanceToCenter / (fx.radius + 0.5f));
                    Cell& targetCell = map.getCell(targetCellX, targetCellY);

                    // EN: Inundate cell with floodwaters and saturate moisture to 100%
                    // ES: Inundar celda con aguas torrenciales y saturar humedad al 100%
                    map.addWaterToCell(targetCellX, targetCellY, DELUGE_WATER_PRECIPITATION_RATE * radialFactor * dt);
                    targetCell.moisture = 1.0f;

                    // EN: Instantly quench any active fire, releasing thick rising steam clouds
                    // ES: Apagar instantáneamente cualquier fuego activo, liberando nubes densas de vapor
                    if (targetCell.isBurning) {
                        targetCell.extinguish();
                        targetCell.steamTimer = 3.0f;
                    }
                    targetCell.heatAccumulator = 0.0f;
                }
            }

            // EN: Extinguish burning villagers and scatter them away from violent torrent epicenters
            // ES: Apagar aldeanos en llamas y dispersarlos lejos del epicentro del torrente violento
            for (auto& villager : humanos.getHumanos()) {
                if (!villager.estaVivo()) continue;
                const float distanceDeltaX = villager.getX() - fx.worldX;
                const float distanceDeltaY = villager.getY() - fx.worldY;
                const float distanceSquared = distanceDeltaX * distanceDeltaX + distanceDeltaY * distanceDeltaY;
                if (distanceSquared <= fx.radius * fx.radius) {
                    if (villager.isBurning()) {
                        villager.extinguishFire();
                    }
                    if (distanceSquared > 0.01f && distanceSquared < 5.0f) {
                        villager.huirEnPanicoDesde(fx.worldX, fx.worldY, 6.0f);
                    }
                }
            }
        } else if (fx.type == DivinePowerType::AtomicBomb) {
            // EN: ATOMIC BOMB UPDATE - Supersonic shockwave propagation, dynamic object flight, and fallout
            // ES: ACTUALIZACIÓN DE BOMBA ATÓMICA - Propagación de onda supersónica, objetos por los aires y radiación
            const int centerCellX = std::clamp(fx.gridX, 0, map.getWidth() - 1);
            const int centerCellY = std::clamp(fx.gridY, 0, map.getHeight() - 1);

            // EN: Superheated ground zero boils away moisture/water during the first 4.0 seconds
            // ES: La zona cero supercalentada evapora cualquier humedad o agua durante los primeros 4.0 segundos
            if (fx.timer <= 4.0f) {
                constexpr float EPICENTER_BOIL_RADIUS = 3.5f;
                for (int deltaCellY = -4; deltaCellY <= 4; ++deltaCellY) {
                    for (int deltaCellX = -4; deltaCellX <= 4; ++deltaCellX) {
                        const float distanceSq = static_cast<float>(deltaCellX * deltaCellX + deltaCellY * deltaCellY);
                        if (distanceSq > EPICENTER_BOIL_RADIUS * EPICENTER_BOIL_RADIUS) continue;

                        const int targetCellX = centerCellX + deltaCellX;
                        const int targetCellY = centerCellY + deltaCellY;
                        if (!map.isValid(targetCellX, targetCellY)) continue;

                        Cell& targetCell = map.getCell(targetCellX, targetCellY);
                        if (targetCell.waterDepth > 0.0f) {
                            targetCell.waterDepth = 0.0f;
                            targetCell.isFlooded = false;
                            targetCell.steamTimer = 4.0f;
                        }
                    }
                }
            }

            // EN: Advance supersonic shockwave front
            // ES: Avanzar frente de onda expansiva supersónica
            constexpr float SHOCKWAVE_SPEED = LittleBoyScale::SHOCKWAVE_SPEED_CELLS_PER_SEC; // 50.0f cells per second (~250 m/s Mach wave)
            const float previousWaveRadius = fx.secondaryTimer;
            const float currentWaveRadius = std::min(fx.radius, fx.timer * SHOCKWAVE_SPEED);
            fx.secondaryTimer = currentWaveRadius;

            if (currentWaveRadius > previousWaveRadius && fx.timer < 3.5f) {
                const int minCellX = std::clamp(static_cast<int>(std::floor(fx.worldX - currentWaveRadius)), 0, map.getWidth() - 1);
                const int maxCellX = std::clamp(static_cast<int>(std::ceil(fx.worldX + currentWaveRadius)), 0, map.getWidth() - 1);
                const int minCellY = std::clamp(static_cast<int>(std::floor(fx.worldY - currentWaveRadius)), 0, map.getHeight() - 1);
                const int maxCellY = std::clamp(static_cast<int>(std::ceil(fx.worldY + currentWaveRadius)), 0, map.getHeight() - 1);

                for (int currentCellY = minCellY; currentCellY <= maxCellY; ++currentCellY) {
                    for (int currentCellX = minCellX; currentCellX <= maxCellX; ++currentCellX) {
                        const float deltaTileX = static_cast<float>(currentCellX) + 0.5f - fx.worldX;
                        const float deltaTileY = static_cast<float>(currentCellY) + 0.5f - fx.worldY;
                        const float tileDist = std::hypot(deltaTileX, deltaTileY);

                        if (tileDist >= previousWaveRadius && tileDist < currentWaveRadius) {
                            Cell& targetTile = map.getCell(currentCellX, currentCellY);
                            const float normDist = std::max(0.15f, tileDist);
                            const float dirX = deltaTileX / normDist;
                            const float dirY = deltaTileY / normDist;
                            const float dynamicPressure = std::clamp(1.0f - (tileDist / fx.radius), 0.15f, 1.0f);

                            // EN: Blast trees out of the ground and fling timber soaring skyward
                            // ES: Arrancar árboles del suelo y lanzar madera volando hacia los cielos
                            if (!targetTile.hasTreesUprooted && (targetTile.naturalResource == ResourceType::Wood || targetTile.biome == BiomeType::DenseForest)) {
                                targetTile.hasTreesUprooted = true;
                                targetTile.resourceAmount = 0;
                                targetTile.naturalResource = ResourceType::None;
                                spawnDebris(DebrisType::UprootedTreeTrunk,
                                            static_cast<float>(currentCellX) + 0.5f, static_cast<float>(currentCellY) + 0.5f,
                                            6.0f, dirX * (18.0f + 10.0f * dynamicPressure), dirY * (18.0f + 10.0f * dynamicPressure),
                                            40.0f + 25.0f * dynamicPressure, 45.0f, 35.0f, 2.4f);
                                spawnDebris(DebrisType::RoofThatch,
                                            static_cast<float>(currentCellX) + 0.5f, static_cast<float>(currentCellY) + 0.5f,
                                            7.0f, dirX * (16.0f + 8.0f * dynamicPressure) + 2.0f, dirY * (16.0f + 8.0f * dynamicPressure) - 2.0f,
                                            50.0f + 25.0f * dynamicPressure, 15.0f, 18.0f, 1.8f);
                            }

                            // EN: Shatter rock formations and launch bedrock boulders into the sky
                            // ES: Destrozar formaciones rocosas y lanzar peñascos de roca madre al cielo
                            if (targetTile.naturalResource == ResourceType::Stone) {
                                targetTile.resourceAmount = 0;
                                targetTile.naturalResource = ResourceType::None;
                                spawnDebris(DebrisType::RockDebris,
                                            static_cast<float>(currentCellX) + 0.5f, static_cast<float>(currentCellY) + 0.5f,
                                            5.0f, dirX * (16.0f + 12.0f * dynamicPressure), dirY * (16.0f + 12.0f * dynamicPressure),
                                            35.0f + 30.0f * dynamicPressure, 28.0f, 32.0f, 2.0f);
                                spawnDebris(DebrisType::RockDebris,
                                            static_cast<float>(currentCellX) + 0.4f, static_cast<float>(currentCellY) + 0.6f,
                                            6.0f, dirX * (14.0f + 10.0f * dynamicPressure) - dirY * 3.0f, dirY * (14.0f + 10.0f * dynamicPressure) + dirX * 3.0f,
                                            42.0f + 25.0f * dynamicPressure, 24.0f, 28.0f, 1.8f);
                            }

                            // EN: Water spray / geyser column ejected when shockwave strikes surface water
                            // ES: Columna de géiser / agua atomizada eyectada cuando la onda golpea agua superficial
                            if (targetTile.waterDepth > 0.05f || targetTile.isWater()) {
                                spawnDebris(DebrisType::WaterSprayGeyser,
                                            static_cast<float>(currentCellX) + 0.5f, static_cast<float>(currentCellY) + 0.5f,
                                            3.0f, dirX * (14.0f + 8.0f * dynamicPressure), dirY * (14.0f + 8.0f * dynamicPressure),
                                            38.0f + 28.0f * dynamicPressure, 6.0f, 0.0f, 1.0f);
                            }

                            // EN: The ferocious supersonic wave tears scorched soil chunks and ground stones into the air
                            // ES: La feroz onda supersónica arranca terrones de tierra y piedras del suelo por los aires
                            const uint32_t cellHash = static_cast<uint32_t>(currentCellX * 73 + currentCellY * 37 + static_cast<int>(fx.seed));
                            if (cellHash % 3 == 0) {
                                spawnDebris(DebrisType::SoilChunk,
                                            static_cast<float>(currentCellX) + 0.5f, static_cast<float>(currentCellY) + 0.5f,
                                            4.0f, dirX * (15.0f + 10.0f * dynamicPressure), dirY * (15.0f + 10.0f * dynamicPressure),
                                            32.0f + 26.0f * dynamicPressure, 14.0f, 18.0f, 1.4f);
                            }
                            if (cellHash % 5 == 0) {
                                spawnDebris(DebrisType::RockDebris,
                                            static_cast<float>(currentCellX) + 0.5f, static_cast<float>(currentCellY) + 0.5f,
                                            5.0f, dirX * (17.0f + 11.0f * dynamicPressure), dirY * (17.0f + 11.0f * dynamicPressure),
                                            36.0f + 28.0f * dynamicPressure, 22.0f, 25.0f, 1.6f);
                            }

                            // EN: Thermal ignition of flammable terrain
                            // ES: Ignición térmica de terreno inflamable
                            if (targetTile.isCombustible() && !targetTile.isBurning && !targetTile.isWater()) {
                                map.igniteCell(currentCellX, currentCellY, 18.0f, 0.85f);
                            }
                        }
                    }
                }

                // EN: Demolish buildings reached by the advancing shockwave front
                // ES: Demoler edificios alcanzados por el frente de onda expansiva
                if (buildingSystem) {
                    for (auto& houseRef : buildingSystem->getHouses()) {
                        if (houseRef.isRuined()) continue;
                        const float deltaHouseX = static_cast<float>(houseRef.cellX) + 0.5f - fx.worldX;
                        const float deltaHouseY = static_cast<float>(houseRef.cellY) + 0.5f - fx.worldY;
                        const float houseDistance = std::hypot(deltaHouseX, deltaHouseY);
                        if (houseDistance >= previousWaveRadius && houseDistance < currentWaveRadius) {
                            const float normDist = std::max(0.2f, houseDistance);
                            const float dirX = deltaHouseX / normDist;
                            const float dirY = deltaHouseY / normDist;
                            const float dynamicPressure = std::clamp(1.0f - (houseDistance / fx.radius), 0.2f, 1.0f);
                            const bool wasRuined = buildingSystem->damageHouse(houseRef.id, 320.0f * dynamicPressure, map, &humanos);
                            if (wasRuined) {
                                spawnDebris(DebrisType::RoofThatch,
                                            static_cast<float>(houseRef.cellX) + 0.4f, static_cast<float>(houseRef.cellY) + 0.4f,
                                            7.0f, dirX * 18.0f + 2.0f, dirY * 18.0f - 2.0f,
                                            52.0f + 20.0f * dynamicPressure, 16.0f, 18.0f, 1.8f);
                                spawnDebris(DebrisType::HouseRubble,
                                            static_cast<float>(houseRef.cellX) + 0.6f, static_cast<float>(houseRef.cellY) + 0.6f,
                                            6.0f, dirX * 16.0f, dirY * 16.0f,
                                            42.0f + 18.0f * dynamicPressure, 35.0f, 30.0f, 2.2f);
                            }
                        }
                    }
                }

                // EN: Obliterate caravans reached by the shockwave front
                // ES: Destruir caravanas alcanzadas por el frente de la onda
                if (replicationEngine) {
                    for (auto& caravanRef : replicationEngine->getActiveCaravansMutable()) {
                        const float carPositionX = static_cast<float>(caravanRef.from.x) + (caravanRef.to.x - caravanRef.from.x) * caravanRef.progress;
                        const float carPositionY = static_cast<float>(caravanRef.from.y) + (caravanRef.to.y - caravanRef.from.y) * caravanRef.progress;
                        const float carDistance = std::hypot(carPositionX - fx.worldX, carPositionY - fx.worldY);
                        if (carDistance >= previousWaveRadius && carDistance < currentWaveRadius) {
                            caravanRef.power = 0.0f;
                            caravanRef.progress = 0.0f;
                            const float normDist = std::max(0.2f, carDistance);
                            const float dirX = (carPositionX - fx.worldX) / normDist;
                            const float dirY = (carPositionY - fx.worldY) / normDist;
                            spawnDebris(DebrisType::ResourceCrate, carPositionX, carPositionY, 6.0f,
                                        dirX * 18.0f, dirY * 18.0f, 48.0f, 22.0f, 20.0f, 1.8f);
                        }
                    }
                }

                // EN: Supersonic expanding shockwave front violently hurls villagers through the air
                // ES: Frente de onda expansiva supersónica lanza violentamente a los aldeanos por los aires
                for (auto& villager : humanos.getHumanos()) {
                    if (!villager.estaVivo()) continue;
                    const float deltaVillagerX = villager.getX() - fx.worldX;
                    const float deltaVillagerY = villager.getY() - fx.worldY;
                    const float villagerDistance = std::hypot(deltaVillagerX, deltaVillagerY);

                    if (villagerDistance >= previousWaveRadius && villagerDistance < currentWaveRadius) {
                        const float blastAngle = (villagerDistance > 0.05f) ? std::atan2(deltaVillagerY, deltaVillagerX)
                                                                             : (static_cast<float>(villager.getId()) * 1.57f);
                        const float dynamicPressure = std::clamp(1.0f - (villagerDistance / fx.radius), 0.2f, 1.0f);
                        const float horizontalLaunchSpeed = 16.0f + 14.0f * dynamicPressure;
                        const float verticalLaunchSpeed = 38.0f + 32.0f * dynamicPressure;

                        if (villagerDistance <= LittleBoyScale::FIREBALL_RADIUS_CELLS) {
                            villager.matar("Vaporizado por el frente termico nuclear de Little Boy");
                            villager.activarRagdoll(std::cos(blastAngle) * (horizontalLaunchSpeed * 1.4f),
                                                    std::sin(blastAngle) * (horizontalLaunchSpeed * 1.4f),
                                                    verticalLaunchSpeed * 1.3f, 36.0f);
                        } else {
                            villager.recibirDano(60.0f * dynamicPressure);
                            villager.activarRagdoll(std::cos(blastAngle) * horizontalLaunchSpeed,
                                                    std::sin(blastAngle) * horizontalLaunchSpeed,
                                                    verticalLaunchSpeed, 28.0f);
                            villager.huirEnPanicoDesde(fx.worldX, fx.worldY, 12.0f);
                        }
                    }
                }

                // EN: High-velocity dynamic wind sweeps settled debris back into the air
                // ES: Viento dinámico de alta velocidad eleva de nuevo los escombros posados por los aires
                for (auto& debrisItem : m_activeDebris) {
                    if (!debrisItem.settled) continue;
                    const float deltaDebrisX = debrisItem.worldX - fx.worldX;
                    const float deltaDebrisY = debrisItem.worldY - fx.worldY;
                    const float debrisDistance = std::hypot(deltaDebrisX, deltaDebrisY);
                    if (debrisDistance >= previousWaveRadius && debrisDistance < currentWaveRadius) {
                        const float normDist = std::max(0.2f, debrisDistance);
                        const float dirX = deltaDebrisX / normDist;
                        const float dirY = deltaDebrisY / normDist;
                        const float dynamicPressure = std::clamp(1.0f - (debrisDistance / fx.radius), 0.2f, 1.0f);
                        debrisItem.settled = false;
                        debrisItem.velocityZ = 25.0f + 25.0f * dynamicPressure;
                        debrisItem.velocityX += dirX * (18.0f * dynamicPressure);
                        debrisItem.velocityY += dirY * (18.0f * dynamicPressure);
                        debrisItem.angularVelocity += 20.0f;
                    }
                }
            }

            // EN: Expanding toxic radioactive dust cloud: engulfs villagers, inflicts radiation sickness and panic
            // ES: Nube de polvo radiactivo en expansión: envuelve aldeanos, causa enfermedad por radiación y pánico
            if (fx.timer > 0.35f) {
                const float cloudTime = fx.timer - 0.35f;
                // EN: Progressive radial expansion: reaches up to ~30 cells radius
                // ES: Expansión radial progresiva: alcanza hasta ~30 celdas de radio
                const float radioactiveCloudRadius = std::min(30.0f, 3.5f + cloudTime * 3.4f + std::sqrt(cloudTime) * 4.6f);
                const float cloudRadiusSq = radioactiveCloudRadius * radioactiveCloudRadius;

                for (auto& villager : humanos.getHumanos()) {
                    if (!villager.estaVivo()) continue;
                    const float deltaVillagerX = villager.getX() - fx.worldX;
                    const float deltaVillagerY = villager.getY() - fx.worldY;
                    const float distSq = deltaVillagerX * deltaVillagerX + deltaVillagerY * deltaVillagerY;

                    if (distSq <= cloudRadiusSq) {
                        // EN: Inflict ionizing radiation damage and trigger acute radiation sickness
                        // ES: Infligir daño por radiación ionizante y desatar síndrome agudo de radiación
                        villager.recibirDano(14.0f * dt);
                        villager.infectWithPandemic(25.0f);
                        if (!villager.isPanicking() && distSq > 4.0f) {
                            villager.huirEnPanicoDesde(fx.worldX, fx.worldY, 12.0f);
                        }
                    }
                }
            }
        }
    }
    m_activeFX.erase(std::remove_if(m_activeFX.begin(), m_activeFX.end(),
                                    [](const ActiveDivineFX& fx) {
                                        return fx.timer >= fx.duration;
                                    }),
                     m_activeFX.end());

    // EN: Update physical ballistic trajectory of volcanic pyroclastic bombs
    // ES: Actualizar trayectoria balística física de las bombas piroclásticas volcánicas
    for (auto& bomb : m_activeBombs) {
        if (bomb.exploded) continue;
        bomb.worldX += bomb.velocityX * dt;
        bomb.worldY += bomb.velocityY * dt;
        bomb.altitude += bomb.velocityZ * dt;
        bomb.velocityZ -= GRAVITY_ACCELERATION_UNITS_S2 * dt;

        if (bomb.altitude <= 0.0f) {
            bomb.altitude = 0.0f;
            bomb.exploded = true;

            const int bombImpactCellX = std::clamp(static_cast<int>(bomb.worldX), 0, map.getWidth() - 1);
            const int bombImpactCellY = std::clamp(static_cast<int>(bomb.worldY), 0, map.getHeight() - 1);
            Cell& bombCell = map.getCell(bombImpactCellX, bombImpactCellY);

            // EN: High-velocity volcanic projectile slams into ground, carving an impact crater
            // ES: Proyectil volcánico de alta velocidad impacta contra el suelo, excavando un cráter de impacto
            bombCell.formCrater(CraterType::KineticHeavyImpact, 0.55f, 0.18f);

            if (bombCell.naturalResource == ResourceType::Wood || bombCell.biome == BiomeType::DenseForest) {
                bombCell.hasTreesUprooted = true;
            }
            if (buildingSystem) {
                buildingSystem->damageHouseAt(bombImpactCellX, bombImpactCellY, 52.0f, map, &humanos);
            }
            if (bombCell.isCombustible() && !bombCell.isBurning) {
                map.igniteCell(bombImpactCellX, bombImpactCellY, 14.0f, 1.0f);
            }
            spawnDebris(DebrisType::PyroclasticBomb, bomb.worldX, bomb.worldY, 2.0f,
                        bomb.velocityX * 0.4f, bomb.velocityY * 0.4f, 15.0f, 25.0f, 20.0f, 1.4f);

            // EN: Ground detonation: radial shockwave and projectile blast damage
            // ES: Detonación en suelo: onda expansiva radial y daño por explosión de proyectil
            for (auto& h : humanos.getHumanos()) {
                if (!h.estaVivo()) continue;
                float dx = h.getX() - bomb.worldX;
                float dy = h.getY() - bomb.worldY;
                float distSq = dx * dx + dy * dy;
                if (distSq <= bomb.blastRadius * bomb.blastRadius) {
                    float dist = std::sqrt(distSq);
                    float blastAngle = (dist > 0.05f) ? std::atan2(dy, dx) : 0.0f;
                    float blastFactor = 1.0f - (dist / bomb.blastRadius);
                    h.recibirDano(32.0f * blastFactor);
                    h.activarRagdoll(std::cos(blastAngle) * 3.2f * blastFactor,
                                     std::sin(blastAngle) * 3.2f * blastFactor,
                                     18.0f * blastFactor, 10.0f);
                    h.huirEnPanicoDesde(bomb.worldX, bomb.worldY, 6.0f);
                }
            }
        }
    }
    m_activeBombs.erase(std::remove_if(m_activeBombs.begin(), m_activeBombs.end(),
                                       [](const PyroclasticBomb& b) { return b.exploded; }),
                        m_activeBombs.end());

    // EN: Update physics, gravity, ground bounces, and kinetic impact for physical debris
    // ES: Actualizar físicas, gravedad, rebotes en suelo e impacto cinético para escombros físicos
    for (auto& debrisItem : m_activeDebris) {
        debrisItem.lifeTimer += dt;
        if (debrisItem.settled) continue;

        const int currentCellX = std::clamp(static_cast<int>(debrisItem.worldX), 0, map.getWidth() - 1);
        const int currentCellY = std::clamp(static_cast<int>(debrisItem.worldY), 0, map.getHeight() - 1);
        const Cell& currentCell = map.getCell(currentCellX, currentCellY);
        const bool hasSurfaceWater = (currentCell.waterDepth > 0.04f || currentCell.isWater());
        const bool isBuoyantMaterial = (debrisItem.type == DebrisType::RoofThatch ||
                                       debrisItem.type == DebrisType::ResourceCrate ||
                                       debrisItem.type == DebrisType::UprootedTreeTrunk);

        if (debrisItem.isFloating) {
            if (isBuoyantMaterial && hasSurfaceWater) {
                // EN: Continue bobbing on water surface and drift along current
                // ES: Continuar oscilando en la superficie del agua y derivar con la corriente
                debrisItem.worldX += debrisItem.velocityX * dt;
                debrisItem.worldY += debrisItem.velocityY * dt;
                debrisItem.altitude = 0.08f + std::sin(debrisItem.lifeTimer * 2.8f) * 0.04f;
                debrisItem.velocityZ = 0.0f;
                debrisItem.velocityX = debrisItem.velocityX * std::max(0.0f, 1.0f - dt * 2.0f) + currentCell.flowVelocityX * 0.65f;
                debrisItem.velocityY = debrisItem.velocityY * std::max(0.0f, 1.0f - dt * 2.0f) + currentCell.flowVelocityY * 0.65f;
                debrisItem.angularVelocity *= std::max(0.0f, 1.0f - dt * 1.5f);
                debrisItem.rotationAngle += debrisItem.angularVelocity * dt;
                debrisItem.settled = (currentCell.getFlowSpeed() <= 0.04f && std::abs(debrisItem.velocityX) < 0.05f && std::abs(debrisItem.velocityY) < 0.05f);
                continue;
            } else {
                debrisItem.isFloating = false;
            }
        }

        debrisItem.worldX += debrisItem.velocityX * dt;
        debrisItem.worldY += debrisItem.velocityY * dt;
        debrisItem.altitude += debrisItem.velocityZ * dt;
        debrisItem.velocityZ -= GRAVITY_ACCELERATION_UNITS_S2 * dt;
        debrisItem.rotationAngle += debrisItem.angularVelocity * dt;

        // EN: Aerodynamic horizontal air resistance
        // ES: Resistencia aerodinámica horizontal del aire
        debrisItem.velocityX *= std::max(0.0f, 1.0f - dt * 0.22f);
        debrisItem.velocityY *= std::max(0.0f, 1.0f - dt * 0.22f);

        if (debrisItem.altitude <= 0.0f) {
            const int debrisCellX = std::clamp(static_cast<int>(debrisItem.worldX), 0, map.getWidth() - 1);
            const int debrisCellY = std::clamp(static_cast<int>(debrisItem.worldY), 0, map.getHeight() - 1);
            const Cell& landingCell = map.getCell(debrisCellX, debrisCellY);

            // EN: Water spray geysers splash into surface moisture and disperse immediately
            // ES: Los géiseres de agua atomizada salpican en humedad superficial y se dispersan
            if (debrisItem.type == DebrisType::WaterSprayGeyser) {
                Cell& landingCellMutable = map.getCell(debrisCellX, debrisCellY);
                landingCellMutable.addWater(0.12f);
                debrisItem.altitude = 0.0f;
                debrisItem.settled = true;
                debrisItem.lifeTimer = debrisItem.maxLifetime;
                continue;
            }

            // EN: Incandescent nuclear slag ignites combustible ground on landing
            // ES: Escoria nuclear incandescente enciende terreno combustible al aterrizar
            if (debrisItem.type == DebrisType::IncandescentSlag) {
                Cell& landingCellMutable = map.getCell(debrisCellX, debrisCellY);
                if (landingCellMutable.isCombustible() && !landingCellMutable.isWater()) {
                    map.igniteCell(debrisCellX, debrisCellY, 16.0f, 0.85f);
                }
            }

            // EN: Fragile soil chunks crumble upon impact, reducing bounces
            // ES: Los terrones frágiles de tierra se desmoronan al impactar, reduciendo rebotes
            if (debrisItem.type == DebrisType::SoilChunk) {
                debrisItem.bounces += 2;
            }

            if (isBuoyantMaterial && hasSurfaceWater) {
                // EN: Hydrodynamic buoyancy: floating debris bobs on water surface and drifts along current
                // ES: Flotabilidad hidrodinámica: escombros flotantes oscilan sobre el agua y derivan con la corriente
                debrisItem.isFloating = true;
                debrisItem.altitude = 0.08f + std::sin(debrisItem.lifeTimer * 2.8f) * 0.04f;
                debrisItem.velocityZ = 0.0f;
                debrisItem.velocityX = debrisItem.velocityX * std::max(0.0f, 1.0f - dt * 2.0f) + landingCell.flowVelocityX * 0.65f;
                debrisItem.velocityY = debrisItem.velocityY * std::max(0.0f, 1.0f - dt * 2.0f) + landingCell.flowVelocityY * 0.65f;
                debrisItem.angularVelocity *= std::max(0.0f, 1.0f - dt * 1.5f);
                debrisItem.settled = false;
            } else {
                debrisItem.altitude = 0.0f;
                const float impactVelocity = std::abs(debrisItem.velocityZ);

                // EN: Kinetic impact damage to humans and houses when exceeding safe speed
                // ES: Daño por impacto cinético a humanos y casas al superar la velocidad segura
                if (impactVelocity > SAFE_FALL_IMPACT_VELOCITY) {
                    // EN: Severe kinetic impact of falling rock or heavy rubble crushes ground and forms crater
                    // ES: Impacto cinético severo de roca en caída o escombros pesados aplasta el suelo y forma cráter
                    if (impactVelocity >= 25.0f &&
                        (debrisItem.type == DebrisType::PyroclasticBomb ||
                         debrisItem.type == DebrisType::HouseRubble ||
                         debrisItem.type == DebrisType::RockDebris ||
                         debrisItem.type == DebrisType::IncandescentSlag)) {
                        Cell& landingCellMutable = map.getCell(debrisCellX, debrisCellY);
                        landingCellMutable.formCrater(CraterType::KineticHeavyImpact, 0.40f, 0.14f);
                    }

                    const float kineticDamage = debrisItem.impactDamage * (impactVelocity / 20.0f);
                    for (auto& villager : humanos.getHumanos()) {
                        if (!villager.estaVivo()) continue;
                        const float deltaX = villager.getX() - debrisItem.worldX;
                        const float deltaY = villager.getY() - debrisItem.worldY;
                        if (deltaX * deltaX + deltaY * deltaY <= debrisItem.damageRadius * debrisItem.damageRadius) {
                            villager.recibirDano(kineticDamage);
                            const float blastAngle = (deltaX * deltaX + deltaY * deltaY > 0.001f) ? std::atan2(deltaY, deltaX) : 0.0f;
                            villager.activarRagdoll(std::cos(blastAngle) * 3.5f, std::sin(blastAngle) * 3.5f, 16.0f, 10.0f);
                        }
                    }
                    if (buildingSystem) {
                        buildingSystem->damageHouseAt(static_cast<int>(debrisItem.worldX),
                                                      static_cast<int>(debrisItem.worldY),
                                                      kineticDamage, map, &humanos);
                    }
                }

                debrisItem.bounces++;
                if (debrisItem.bounces < 4 && impactVelocity > 4.5f) {
                    debrisItem.velocityZ = -debrisItem.velocityZ * 0.42f; // EN: Elastic restitution bounce / ES: Rebote por restitución elástica
                    debrisItem.velocityX *= 0.68f;
                    debrisItem.velocityY *= 0.68f;
                    debrisItem.angularVelocity *= 0.60f;
                } else {
                    // EN: Ground skidding friction until reaching a complete stop
                    // ES: Fricción de deslizamiento en suelo hasta detenerse por completo
                    debrisItem.velocityZ = 0.0f;
                    debrisItem.velocityX *= std::max(0.0f, 1.0f - dt * 5.0f);
                    debrisItem.velocityY *= std::max(0.0f, 1.0f - dt * 5.0f);
                    debrisItem.angularVelocity *= std::max(0.0f, 1.0f - dt * 6.0f);
                    if (std::abs(debrisItem.velocityX) < 0.15f && std::abs(debrisItem.velocityY) < 0.15f) {
                        debrisItem.velocityX = 0.0f;
                        debrisItem.velocityY = 0.0f;
                        debrisItem.angularVelocity = 0.0f;
                        debrisItem.settled = true;
                    }
                }
            }
        }
    }
    m_activeDebris.erase(std::remove_if(m_activeDebris.begin(), m_activeDebris.end(),
                                        [](const PhysicalDebris& debrisRef) {
                                            return debrisRef.lifeTimer >= debrisRef.maxLifetime;
                                        }),
                         m_activeDebris.end());

    // Actualizar sermones activos y seguir la posición del orador si se mueve
    for (auto& sp : m_activeSpeeches) {
        sp.timer += dt;
        for (const auto& h : humanos.getHumanos()) {
            if (h.getId() == sp.humanId) {
                sp.worldX = h.getX();
                sp.worldY = h.getY();
                break;
            }
        }
    }
    m_activeSpeeches.erase(std::remove_if(m_activeSpeeches.begin(), m_activeSpeeches.end(),
                                          [](const PreacherSpeech& sp) {
                                              return sp.timer >= sp.maxDuration;
                                          }),
                           m_activeSpeeches.end());
}

void DivinePowersSystem::clearAllFX() {
    m_activeFX.clear();
    m_activeSpeeches.clear();
    m_activeBombs.clear();
    m_activeDebris.clear();
}

} // namespace ReligionSim
