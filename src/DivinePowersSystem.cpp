#include "DivinePowersSystem.hpp"
#include "WorldMap.hpp"
#include "Religion.hpp"
#include "Humano.hpp"
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
        case DivinePowerType::Meteor:     return 5;
        case DivinePowerType::Volcano:    return 5;
        case DivinePowerType::Wildfire:   return 5;
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
                                         EventLine& eventLine, int playerRelId) {
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
        // CATÁSTROFE 4: METEORITO CATACLÍSMICO (Bólido Celeste, Onda Expansiva y Cráter Sagrado)
        // ======================================================================================
        float tx = static_cast<float>(targetGx) + 0.5f;
        float ty = static_cast<float>(targetGy) + 0.5f;

        m_activeFX.push_back({
            DivinePowerType::Meteor,
            targetGx, targetGy,
            tx, ty,
            0.0f, 0.0f,
            5.0f,  // 5.0 segundos de duración total (1.25s caída atmosférica + 3.75s cráter y humo)
            0.0f,
            1.0f,
            5.0f,  // Radio de influencia (5 celdas)
            0, 0, 0,
            static_cast<float>(targetGx * 23 + targetGy * 41 + 7),
            false  // impactTriggered
        });

        // Pánico inicial inmediato: los aldeanos ven encenderse el cielo y huyen aterrados
        int panicoCount = 0;
        for (auto& h : humanos.getHumanos()) {
            if (!h.estaVivo()) continue;
            float dx = h.getX() - tx;
            float dy = h.getY() - ty;
            float d = std::sqrt(dx * dx + dy * dy);
            if (d <= 5.5f) {
                h.huirEnPanicoDesde(tx, ty, 6.5f);
                panicoCount++;
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

        int faithGenerated = static_cast<int>((60 + panicoCount * 8) * leaderInfluenceMult);
        pRel->theologyPoints += faithGenerated;

        res.faithGenerated = faithGenerated;
        res.casualties = 0; // Las bajas se consuman en el momento del impacto a t=1.25s
        res.success = true;
        res.message = "[METEORITO CATACLISMICO] ¡Bolido celeste desciende en picado hacia (" +
                      std::to_string(targetGx) + "," + std::to_string(targetGy) + ")! " +
                      std::to_string(panicoCount) + " aldeanos huyen aterrados de la zona de impacto.";

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
                Cell& c = map.getCell(cx, cy);
                if (c.isWater()) continue;

                // Propagación inicial: bosques, valles fértiles, matorrales, madera o chozas
                bool combustible = (c.biome == BiomeType::DenseForest || c.biome == BiomeType::FertileValley ||
                                    c.biome == BiomeType::CoastalPlains || c.naturalResource == ResourceType::Wood ||
                                    c.hasHouse);
                if (combustible && !c.isBurning) {
                    c.isBurning = true;
                    c.burnTimer = 14.0f; // 14 segundos de combustión activa
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
    }

    map.recalculateReligions(registry);
    return res;
}

void DivinePowersSystem::update(float dt, WorldMap& map, GestorHumanos& humanos) {
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

            // Arrasar recursos naturales en las celdas que atraviesa
            int cx = std::clamp(fx.gridX, 0, map.getWidth() - 1);
            int cy = std::clamp(fx.gridY, 0, map.getHeight() - 1);
            Cell& c = map.getCell(cx, cy);
            if (c.naturalResource != ResourceType::None && c.resourceAmount > 0) {
                c.resourceAmount = std::max(0, c.resourceAmount - static_cast<int>(25.0f * dt));
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
                Cell& centerCell = map.getCell(cx, cy);
                centerCell.resourceAmount = 0;
                centerCell.naturalResource = ResourceType::None;
                centerCell.hasImpactCrater = true; // ¡CRÁTER PERMANENTE EN EL TERRENO!
                centerCell.craterDepth = 0.5f;
                if (centerCell.biome != BiomeType::DeepOcean &&
                    centerCell.biome != BiomeType::ShallowWater) {
                    centerCell.biome = BiomeType::AridDesert; // Cráter de ceniza y basalto fundido
                    centerCell.elevation = std::max(0.1f, centerCell.elevation - 0.25f);
                }
                if (centerCell.hasHolySite) {
                    centerCell.shrineLevel = std::max(0, centerCell.shrineLevel - 2);
                    if (centerCell.shrineLevel == 0) centerCell.hasHolySite = false;
                }

                // Celdas adyacentes en anillo (radio 1)
                for (int dx = -1; dx <= 1; ++dx) {
                    for (int dy = -1; dy <= 1; ++dy) {
                        int nx = cx + dx;
                        int ny = cy + dy;
                        if (map.isValid(nx, ny) && (dx != 0 || dy != 0)) {
                            Cell& nc = map.getCell(nx, ny);
                            nc.hasImpactCrater = true; // Cráter periférico permanente
                            if (nc.naturalResource == ResourceType::Wood) {
                                nc.resourceAmount = std::max(0, nc.resourceAmount - 150);
                                if (nc.resourceAmount == 0) nc.naturalResource = ResourceType::None;
                            }
                        }
                    }
                }

                // 2. Físicas de explosión, ragdoll y letalidad
                for (auto& h : humanos.getHumanos()) {
                    if (!h.estaVivo()) continue;
                    float dx = h.getX() - fx.worldX;
                    float dy = h.getY() - fx.worldY;
                    float distSq = dx * dx + dy * dy;
                    if (distSq > fx.radius * fx.radius) continue;

                    float dist = std::sqrt(distSq);
                    float blastAngle = (dist > 0.05f) ? std::atan2(dy, dx) : (static_cast<float>(h.getId()) * 1.57f);
                    float roll = dist01(g_divineRng);

                    if (dist <= 1.8f) {
                        // EPICENTRO: 92% letalidad, eyección violenta en ragdoll
                        if (roll < 0.92f) {
                            h.matar("Vaporizado en el epicentro cosmico del Meteorito (92% mort.)");
                            h.activarRagdoll(std::cos(blastAngle) * 5.8f, std::sin(blastAngle) * 5.8f, 42.0f, 22.0f);
                            std::cout << "[Impacto Meteorito] ¡" << h.getNombre() << " fue calcinado instantaneamente en el epicentro!" << std::endl;
                        } else {
                            // Superviviente milagroso
                            h.recibirDano(80.0f);
                            h.activarRagdoll(std::cos(blastAngle) * 4.2f, std::sin(blastAngle) * 4.2f, 32.0f, 18.0f);
                            h.setDevocion(1.0f);
                            h.promoverAFanatico();
                            h.triggerFaithIndicator(+1, 2.5f);
                            std::cout << "[Supervivencia Milagrosa] ¡" << h.getNombre() << " sobrevivio al impacto del meteorito y se convirtio en Fanatico!" << std::endl;
                        }
                    } else if (dist <= 3.5f) {
                        // ONDA DE CHOQUE INTERMEDIA: 45% letalidad, ragdoll de alto impulso
                        if (roll < 0.45f) {
                            h.matar("Aplastado por la onda de choque supersonica del Meteorito (45% mort.)");
                            h.activarRagdoll(std::cos(blastAngle) * 3.8f, std::sin(blastAngle) * 3.8f, 24.0f, 15.0f);
                        } else {
                            h.recibirDano(50.0f);
                            h.activarRagdoll(std::cos(blastAngle) * 3.0f, std::sin(blastAngle) * 3.0f, 18.0f, 12.0f);
                            h.huirEnPanicoDesde(fx.worldX, fx.worldY, 7.0f);
                        }
                    } else {
                        // ONDA DE CHOQUE PERIFÉRICA: 15% letalidad, derribo y aturdimiento
                        if (roll < 0.15f) {
                            h.matar("Derribado mortalmente por esquirlas del impacto de Meteorito (15% mort.)");
                            h.activarRagdoll(std::cos(blastAngle) * 2.2f, std::sin(blastAngle) * 2.2f, 14.0f, 8.0f);
                        } else {
                            h.recibirDano(25.0f);
                            h.activarRagdoll(std::cos(blastAngle) * 1.6f, std::sin(blastAngle) * 1.6f, 10.0f, 6.0f);
                            h.huirEnPanicoDesde(fx.worldX, fx.worldY, 6.0f);
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
                        if (nc.naturalResource != ResourceType::None) {
                            nc.resourceAmount = std::max(0, nc.resourceAmount - static_cast<int>(35.0f * dt));
                            if (nc.resourceAmount == 0) nc.naturalResource = ResourceType::None;
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
        } else if (fx.type == DivinePowerType::Wildfire) {
            // CATÁSTROFE 6: INCENDIO FORESTAL (Propagación Ígnea Celular, Consumo de Madera y Pánico)
            int minX = std::max(0, fx.gridX - 7);
            int maxX = std::min(map.getWidth() - 1, fx.gridX + 7);
            int minY = std::max(0, fx.gridY - 7);
            int maxY = std::min(map.getHeight() - 1, fx.gridY + 7);
            std::uniform_real_distribution<float> dist01(0.0f, 1.0f);

            // 1. Simulación celular del fuego y combustión
            for (int cy = minY; cy <= maxY; ++cy) {
                for (int cx = minX; cx <= maxX; ++cx) {
                    Cell& c = map.getCell(cx, cy);
                    if (!c.isBurning) continue;

                    c.burnTimer -= dt;

                    // Consumo de recursos madereros
                    if (c.naturalResource == ResourceType::Wood) {
                        c.resourceAmount = std::max(0, c.resourceAmount - static_cast<int>(30.0f * dt));
                        if (c.resourceAmount == 0) c.naturalResource = ResourceType::None;
                    }

                    // Calcinación de cabañas/chozas de madera
                    if (c.hasHouse && c.burnTimer < 8.0f) {
                        c.hasHouse = false;
                    }

                    // Propagación celular ígnea a celdas adyacentes (Vecindad de Moore)
                    for (int dy = -1; dy <= 1; ++dy) {
                        for (int dx = -1; dx <= 1; ++dx) {
                            if (dx == 0 && dy == 0) continue;
                            int nx = cx + dx;
                            int ny = cy + dy;
                            if (!map.isValid(nx, ny)) continue;

                            Cell& nc = map.getCell(nx, ny);
                            if (nc.isWater() || nc.isBurning || nc.isBurnt) continue;

                            bool combustible = (nc.biome == BiomeType::DenseForest || nc.biome == BiomeType::FertileValley ||
                                                nc.biome == BiomeType::CoastalPlains || nc.naturalResource == ResourceType::Wood ||
                                                nc.hasHouse);
                            if (combustible) {
                                float spreadChance = 0.22f * dt;
                                if (dist01(g_divineRng) < spreadChance) {
                                    nc.isBurning = true;
                                    nc.burnTimer = 10.0f + dist01(g_divineRng) * 5.0f;
                                }
                            }
                        }
                    }

                    // Extinción y transición a suelo de cenizas fértiles
                    if (c.burnTimer <= 0.0f) {
                        c.isBurning = false;
                        c.isBurnt = true;
                        if (c.biome == BiomeType::DenseForest) {
                            c.biome = BiomeType::CoastalPlains; // El bosque calcinado se abre a llanuras
                            c.naturalResource = ResourceType::None;
                            c.resourceAmount = 0;
                        }
                    }
                }
            }

            // 2. Pánico y daño por fuego a los aldeanos
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
                        if (h.estaMuerto()) {
                            float blastAngle = (distSq > 0.05f) ? std::atan2(dy, dx) : 0.0f;
                            h.activarRagdoll(std::cos(blastAngle) * 2.2f, std::sin(blastAngle) * 2.2f, 16.0f, 10.0f);
                            h.matar("Calcinado por el frente de fuego del incendio forestal");
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
}

} // namespace ReligionSim
