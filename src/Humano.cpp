#include "Humano.hpp"
#include "WorldMap.hpp"
#include "Religion.hpp"
#include "IsometricCamera.hpp"
#include "SoundSystem.hpp"
#include "NemesisSififus.hpp"
#include "BuildingSystem.hpp"
#include "DayNightCycle.hpp"
#include "EventLine.hpp"
#include "ErrorSystem.hpp"
#include <cmath>
#include <random>
#include <algorithm>
#include <iostream>
#include <fstream>

namespace ReligionSim {

namespace {
    std::mt19937 g_humanoRng(2026);

    const std::vector<std::string> NOMBRES_GROG = {
        "Grog el Cazador", "Grog el Devoto", "Grog el Leñador", "Grog del Fuego",
        "Grog el Recolector", "Grog el Peregrino", "Grog el Chamán", "Grog el Nómada",
        "Grog el Constructor", "Grog el Fiel", "Grog el Anciano", "Grog el Sabio"
    };

    const std::vector<std::string> NOMBRES_CHAMAN = {
        "Chaman Arandu", "Chaman Khep", "Chaman Orph", "Chaman Mithra", "Chaman Dharma"
    };

    const std::vector<std::string> NOMBRES_FEMENINOS = {
        "Sora la Tejedora", "Maya de las Aguas", "Kira la Recolectora", "Nayla la Fiel",
        "Elena del Roble", "Lyra la Sabia", "Sari de la Alborada", "Tara la Devota",
        "Vea la Curandera", "Mira la Cantora", "Zira de la Tierra", "Aria la Peregrina"
    };
}

// EN: Procedural name generator based on gender and shamanic role
// ES: Generador procedural de nombres según género y rol
std::string Humano::generateName(HumanGender gender, bool isShaman) {
    if (isShaman) {
        std::uniform_int_distribution<size_t> shamanDist(0, NOMBRES_CHAMAN.size() - 1);
        return NOMBRES_CHAMAN[shamanDist(g_humanoRng)];
    }
    if (gender == HumanGender::Female) {
        std::uniform_int_distribution<size_t> femaleDist(0, NOMBRES_FEMENINOS.size() - 1);
        return NOMBRES_FEMENINOS[femaleDist(g_humanoRng)];
    }
    std::uniform_int_distribution<size_t> maleDist(0, NOMBRES_GROG.size() - 1);
    return NOMBRES_GROG[maleDist(g_humanoRng)];
}

Humano::Humano(int id, const std::string& name, float startX, float startY, HumanGender gender)
    : m_id(id), m_name(name), m_gender(gender), m_x(startX), m_y(startY),
      m_targetDestX(startX), m_targetDestY(startY) {
    m_cellX = static_cast<int>(std::floor(startX));
    m_cellY = static_cast<int>(std::floor(startY));

    // EN: Initialize demographic and motivational state for First Era (FirstEraAIController)
    // ES: Inicializar estado demográfico y motivacional de Primera Era (FirstEraAIController)
    m_popGroup.populationCount = 18 + (id * 7) % 30;
    m_popGroup.foodSupply = 42.0f + static_cast<float>((id * 19) % 48);
    m_popGroup.divineWill = m_devotion * 100.0f;
    m_popGroup.unityWill = 58.0f + static_cast<float>((id * 13) % 36);
    m_popGroup.state = RealisticVillagerState::COMMUNAL_BONDING;
    m_popGroup.coordinates = {startX, startY};

    // EN: Initialize systemic individual attributes for human unit (HumanAIController)
    // ES: Inicializar atributos sistémicos individuales de la unidad humana (HumanAIController)
    m_humanUnit.unitName = name;
    m_humanUnit.tribeID = "Tribu #" + std::to_string(m_religionId);
    m_humanUnit.leaderOrProphetID = "Chaman Ancestral";
    m_humanUnit.currentEra = ::EraType::PRIMITIVE_TRIBAL;
    if (id % 13 == 0) {
        m_humanUnit.role = CommunityRole::PROPHET;
        m_humanUnit.conversionPower = 32.0f;
        m_humanUnit.conversionInertia = 78.0f;
        m_humanUnit.willpower = 88.0f;
    } else if (id % 9 == 0) {
        m_humanUnit.role = CommunityRole::ZEALOT;
        m_humanUnit.conversionPower = 24.0f;
        m_humanUnit.conversionInertia = 95.0f;
        m_humanUnit.willpower = 95.0f;
    } else {
        m_humanUnit.role = CommunityRole::FORAGER;
        m_humanUnit.conversionPower = 12.0f;
        m_humanUnit.conversionInertia = 45.0f;
        m_humanUnit.willpower = 60.0f;
    }
    m_humanUnit.currentState = HumanAIState::SOCIALIZING;
    m_humanUnit.position = {startX, startY};
    m_humanUnit.nutritionalState = m_popGroup.foodSupply;

    if (m_humanUnit.role == CommunityRole::PROPHET) {
        m_leaderDevotion = 1.0f;
    } else if (m_humanUnit.role == CommunityRole::ZEALOT) {
        m_devotion = 0.92f;
        m_leaderDevotion = 0.95f;
    }

    m_humanUnit.devotion = m_devotion * 100.0f;
    m_humanUnit.hitPoints = m_health;
    m_humanUnit.energy = m_energy;
}

void Humano::promoteToZealot() {
    if (m_humanUnit.role != CommunityRole::ZEALOT) {
        m_humanUnit.role = CommunityRole::ZEALOT;
        m_humanUnit.conversionInertia = 95.0f;
        m_humanUnit.willpower = 95.0f;
        m_humanUnit.conversionPower = 28.0f;
        m_humanUnit.attackPower = std::max(m_humanUnit.attackPower, 16.0f);
        m_devotion = std::max(m_devotion, 0.92f);
        m_leaderDevotion = std::max(m_leaderDevotion, 0.90f);
        m_humanUnit.devotion = m_devotion * 100.0f;
        m_isShaman = false;
        triggerFaithIndicator(+1, 2.0f);
    }
}

void Humano::demoteZealot() {
    if (m_humanUnit.role == CommunityRole::ZEALOT) {
        m_humanUnit.role = CommunityRole::FORAGER;
        m_humanUnit.conversionInertia = 45.0f;
        m_humanUnit.willpower = 65.0f;
        m_humanUnit.conversionPower = 14.0f;
        triggerFaithIndicator(-1, 1.5f);
    }
}

void Humano::changeAction(HumanAction newAction) {
    m_action = newAction;
    m_animTimer = 0.0f;
}

void Humano::changeDirection(IsoDirection newDirection) {
    m_direction = newDirection;
}

void Humano::orientTowards(float targetPosX, float targetPosY) {
    float deltaPosX = targetPosX - m_x;
    float deltaPosY = targetPosY - m_y;
    if (std::abs(deltaPosX) > std::abs(deltaPosY)) {
        m_direction = (deltaPosX > 0) ? IsoDirection::SE : IsoDirection::NW;
    } else {
        m_direction = (deltaPosY > 0) ? IsoDirection::SW : IsoDirection::NE;
    }
}

void Humano::walkTowards(float targetPosX, float targetPosY, bool isRunning) {
    m_targetDestX = targetPosX;
    m_targetDestY = targetPosY;
    m_speed = isRunning ? 1.40f : 0.65f;
    m_action = isRunning ? HumanAction::CORRER : HumanAction::CAMINAR;
    orientTowards(targetPosX, targetPosY);
}

void Humano::orderManualMovement(float targetPosX, float targetPosY) {
    m_hasManualOrder = true;
    walkTowards(targetPosX, targetPosY, false);
}

TileRect Humano::getTileRect(const TileSet& tileSetRef) const {
    return tileSetRef.getRectangle(m_action, m_direction);
}

// EN: Evaluate active physiological, psychological, and behavioral condition of the villager
// ES: Evaluar la condición fisiológica, psicológica y conductual activa del aldeano
VillagerCondition Humano::getCondition() const {
    // EN: Living check: deceased units are governed by death/corpse mechanics
    // ES: Comprobación de vida: las unidades fallecidas se rigen por la mecánica de cadáver
    if (m_health <= 0.0f) {
        return VillagerCondition::Healthy;
    }

    // EN: 1. Panic and terror state (burning or disaster fear, unless alcohol provides false courage)
    // ES: 1. Estado de pánico y terror (en llamas o pánico por cataclismos, salvo que el alcohol dé falso valor)
    if (m_panicTimer > 0.0f || m_isBurning) {
        if (m_alcoholLevel > 35.0f) {
            return VillagerCondition::Drunk; // EN: Drunken stupor ignores fear / ES: Estupor de ebriedad ignora el miedo
        }
        return VillagerCondition::Scared;
    }

    // EN: 2. Aggressive combat fury, protective blood rage, or provoked belligerence
    // ES: 2. Furia combativa agresiva, cólera protectora o beligerancia provocada
    if (m_protectiveFuryTimer > 0.0f || m_aggressionLevel > 50.0f) {
        return VillagerCondition::Aggressive;
    }

    // EN: 3. Severe physical trauma and bleeding wounds (HP below critical threshold)
    // ES: 3. Trauma físico severo y heridas sangrantes (HP por debajo del umbral crítico)
    if (m_health < 35.0f) {
        return VillagerCondition::Injured;
    }

    // EN: 4. Alcohol intoxication from fermented wild fruits, mead, or festival revelry
    // ES: 4. Intoxicación etílica por frutos silvestres fermentados, hidromiel o festejos
    if (m_alcoholLevel > 30.0f) {
        return VillagerCondition::Drunk;
    }

    // EN: 5. Contagious epidemic sickness or pathological infection
    // ES: 5. Enfermedad epidémica contagiosa o infección patológica
    if (m_isInfected || m_infectionTimer > 0.0f) {
        return VillagerCondition::Sick;
    }

    // EN: 6. Biological hunger and nutritional deficit
    // ES: 6. Hambre biológica y déficit nutricional
    if (m_humanUnit.nutritionalState < 35.0f || m_popGroup.foodSupply < 20.0f) {
        return VillagerCondition::Hungry;
    }

    // EN: 7. Circadian exhaustion, stamina depletion, and heavy sleep need
    // ES: 7. Agotamiento circadiano, estamina vacía y pesada necesidad de sueño
    if (m_humanUnit.energy < 20.0f || m_sleepNeed > 80.0f) {
        return VillagerCondition::Exhausted;
    }

    // EN: 8. Divine sacred inspiration, holy signs, and worship ecstasy
    // ES: 8. Inspiración sagrada divina, señales celestiales y éxtasis de culto
    if (m_divineSignTimer > 0.0f || (m_devotion > 0.90f && m_humanUnit.currentState == HumanAIState::WORSHIPING)) {
        return VillagerCondition::Inspired;
    }

    // EN: 9. Satiated, well-nourished full condition
    // ES: 9. Condición saciada y de nutrición plena
    if (m_humanUnit.nutritionalState >= 80.0f) {
        return VillagerCondition::Full;
    }

    // EN: 10. Default baseline healthy state with optimal equilibrium
    // ES: 10. Estado saludable basal por defecto con equilibrio óptimo
    return VillagerCondition::Healthy;
}

// EN: Calculate effective movement speed modulated by current biological/psychological condition
// ES: Calcular la velocidad efectiva de movimiento modulada por la condición biológica/psicológica actual
float Humano::getEffectiveSpeed() const {
    const VillagerCondition currentCondition = getCondition();
    float speedMultiplier = 1.0f;
    switch (currentCondition) {
        case VillagerCondition::Hungry:     speedMultiplier = 0.75f; break; // EN: Weakened by hunger / ES: Ralentizado por hambre
        case VillagerCondition::Full:       speedMultiplier = 1.10f; break; // EN: Satiated vigor / ES: Vigor saciado
        case VillagerCondition::Sick:       speedMultiplier = 0.60f; break; // EN: Feverish drag / ES: Arrastre febril
        case VillagerCondition::Drunk:      speedMultiplier = 0.75f + 0.20f * std::sin(m_animTimer * 4.0f); break; // EN: Stumbling pace / ES: Paso tambaleante
        case VillagerCondition::Aggressive: speedMultiplier = 1.25f; break; // EN: Adrenaline sprint / ES: Carga de adrenalina
        case VillagerCondition::Scared:     speedMultiplier = 1.40f; break; // EN: Panic sprint / ES: Huida despavorida
        case VillagerCondition::Exhausted:  speedMultiplier = 0.45f; break; // EN: Heavy exhaustion / ES: Fatiga pesada
        case VillagerCondition::Inspired:   speedMultiplier = 1.15f; break; // EN: Sacred buoyancy / ES: Paso sagrado ligero
        case VillagerCondition::Injured:    speedMultiplier = 0.50f; break; // EN: Hobbled limp / ES: Renguera por heridas
        case VillagerCondition::Healthy:    speedMultiplier = 1.00f; break; // EN: Standard baseline / ES: Línea base estándar
    }
    return std::clamp(m_speed * speedMultiplier, 0.15f, 2.50f);
}

// EN: Reset temporary conditions (alcohol, rage, panic, infection, ecstasy)
// ES: Restablecer condiciones temporales (alcohol, furia, pánico, infección, éxtasis)
void Humano::resetConditions() {
    m_alcoholLevel = 0.0f;
    m_aggressionLevel = 0.0f;
    m_protectiveFuryTimer = 0.0f;
    m_panicTimer = 0.0f;
    m_isInfected = false;
    m_infectionTimer = 0.0f;
    m_divineSignTimer = 0.0f;
    m_isBurning = false;
}

void Humano::activateRagdoll(float impulseX, float impulseY, float impulseZ, float angularSpeedRadS) {
    if (impulseZ > 0.0f || impulseX != 0.0f || impulseY != 0.0f) {
        m_altitude = std::max(m_altitude, 0.5f);
    }
    m_velocityX = impulseX;
    m_velocityY = impulseY;
    m_velocityZ = impulseZ;
    m_flying = (m_altitude > 0.0f || impulseZ > 8.0f);
    m_ragdoll.launch(impulseX, impulseY, impulseZ, angularSpeedRadS);
}

void Humano::kill(const std::string& cause, float corpseDurationSec) {
    m_health = 0.0f;
    m_humanUnit.hitPoints = 0.0f;
    m_deathTimer = corpseDurationSec;
    m_causeOfDeath = cause;
    m_stumbleTimer = 2.0f;
    m_action = HumanAction::IDLE;
    m_speed = 0.0f;
    m_isSleeping = false;
    m_isSleepingIndoors = false;

    // EN: If ragdoll physics not active, collapse into resting ragdoll
    // ES: Si el ragdoll no está activo todavía, colapsar en físicas de ragdoll en reposo
    if (!m_ragdoll.isActive()) {
        activateRagdoll(0.0f, 0.0f, 0.0f, 0.0f);
    }
}

void Humano::fleeInPanicFrom(float originX, float originY, float duration) {
    m_panicTimer = duration;
    m_isSleeping = false;
    m_isSleepingIndoors = false;
    float deltaPosX = m_x - originX;
    float deltaPosY = m_y - originY;
    float distance = std::sqrt(deltaPosX * deltaPosX + deltaPosY * deltaPosY);
    if (distance < 0.01f) {
        deltaPosX = 1.0f; deltaPosY = 0.5f; distance = 1.118f;
    }
    float targetEscapeX = m_x + (deltaPosX / distance) * 7.0f;
    float targetEscapeY = m_y + (deltaPosY / distance) * 7.0f;
    walkTowards(targetEscapeX, targetEscapeY, true);
}

void Humano::suckIntoTornado(float tornadoX, float tornadoY, float effectRadius, float deltaTime) {
    if (m_health <= 0.0f) return;
    m_isSleeping = false;
    m_isSleepingIndoors = false;
    const float deltaPosX = m_x - tornadoX;
    const float deltaPosY = m_y - tornadoY;
    float distance = std::sqrt(deltaPosX * deltaPosX + deltaPosY * deltaPosY);
    if (distance < 0.05f) distance = 0.05f;

    // EN: Rankine cyclonic vortex core radius (eyewall transition threshold)
    // ES: Radio del núcleo ciclónico de Rankine (umbral de transición de la pared del ojo)
    constexpr float VORTEX_CORE_RADIUS = 2.3f;

    // EN: Zone 1: Central vortex (< VORTEX_CORE_RADIUS) -> SUCTION, ROTATION AND UPWARD CONVECTION
    // ES: Zona 1: Vórtice central (< VORTEX_CORE_RADIUS) -> SUCCIÓN, ROTACIÓN Y CONVECCIÓN ASCENDENTE
    if (distance <= VORTEX_CORE_RADIUS) {
        m_flying = true;

        // EN: Aerodynamic centripetal pull towards eye center
        // ES: Atracción centrípeta aerodinámica hacia el centro del ojo
        const float suctionStrength = 3.6f / (distance + 0.20f);
        m_x -= (deltaPosX / distance) * suctionStrength * deltaTime;
        m_y -= (deltaPosY / distance) * suctionStrength * deltaTime;

        // EN: Rankine vortex tangential cyclonic velocity
        // ES: Velocidad ciclónica tangencial del vórtice de Rankine
        const float tangentialSpeed = 5.8f * (1.0f - (distance / (VORTEX_CORE_RADIUS * 1.4f)));
        const float tangX = -deltaPosY / distance;
        const float tangY = deltaPosX / distance;
        m_x += tangX * tangentialSpeed * deltaTime;
        m_y += tangY * tangentialSpeed * deltaTime;

        m_angularAngle += 11.5f * deltaTime;

        // EN: Convective updraft accelerating altitude inside the vortex funnel
        // ES: Corriente convectiva ascendente que acelera la altitud en el embudo del vórtice
        const float updraftStrength = 46.0f * (1.0f - (distance / (VORTEX_CORE_RADIUS * 1.25f)));
        m_altitude = std::min(65.0f, m_altitude + updraftStrength * deltaTime);
        m_velocityZ = 26.0f;

        // EN: Centrifugal ejection: rotational inertia flings unit out of the upper vortex
        // ES: Eyección centrífuga: la inercia rotacional expulsa a la unidad del vórtice superior
        if (m_altitude >= 26.0f && distance >= 1.5f) {
            const float ejectionSpeed = 5.2f;
            const float flingVx = (deltaPosX / distance) * ejectionSpeed + tangX * 3.8f;
            const float flingVy = (deltaPosY / distance) * ejectionSpeed + tangY * 3.8f;
            activateRagdoll(flingVx, flingVy, 18.0f, 15.0f);
            return;
        }

        const int dirIdx = (static_cast<int>(m_angularAngle / 0.785f)) % 8;
        m_direction = static_cast<IsoDirection>((dirIdx < 0 ? dirIdx + 8 : dirIdx) % 4);
        m_action = HumanAction::CORRER;
    }
    // EN: Zone 2: Periphery (VORTEX_CORE_RADIUS to effectRadius) -> PANIC, FLEEING AND TANGENTIAL GALE
    // ES: Zona 2: Periferia (VORTEX_CORE_RADIUS a effectRadius) -> PÁNICO, HUIDA Y RÁFAGAS TANGENCIALES
    else if (distance <= effectRadius) {
        if (!m_flying) {
            fleeInPanicFrom(tornadoX, tornadoY, 5.2f);
        } else {
            const float tangX = -deltaPosY / distance;
            const float tangY = deltaPosX / distance;
            m_velocityX = (deltaPosX / distance) * 3.8f + tangX * 4.2f;
            m_velocityY = (deltaPosY / distance) * 3.8f + tangY * 4.2f;
            m_velocityZ = 16.0f;
        }
    }
}

void Humano::sufferEarthquake(float quakeX, float quakeY, float intensity) {
    if (m_health <= 0.0f) return;
    m_isSleeping = false;
    m_isSleepingIndoors = false;
    const float deltaPosX = m_x - quakeX;
    const float deltaPosY = m_y - quakeY;
    const float distance = std::sqrt(deltaPosX * deltaPosX + deltaPosY * deltaPosY);

    constexpr float MAX_QUAKE_RADIUS = 6.8f;
    if (distance <= MAX_QUAKE_RADIUS) {
        const float falloffFactor = 1.0f - (distance / MAX_QUAKE_RADIUS);

        // EN: Seismic ground acceleration equation: a(r) = A0 * falloff * intensity
        // ES: Ecuación de aceleración sísmica del suelo: a(r) = A0 * atenuación * intensidad
        const float groundAcceleration = 9.8f * falloffFactor * intensity;
        const float criticalTippingAcceleration = HUMAN_GROUND_STATIC_FRICTION * 9.8f;

        // EN: If ground acceleration exceeds human static friction, balance is lost
        // ES: Si la aceleración del suelo supera la fricción estática humana, se pierde el equilibrio
        if (groundAcceleration > criticalTippingAcceleration) {
            m_stumbleTimer = std::max(m_stumbleTimer, (1.8f + falloffFactor * 2.6f) * intensity);
            if (falloffFactor >= 0.65f && !m_ragdoll.isActive()) {
                // EN: Violent epicenter shaking knocks unit completely off their feet into ragdoll
                // ES: La sacudida violenta del epicentro derriba al aldeano al suelo en ragdoll
                const float blastAngle = (distance > 0.05f) ? std::atan2(deltaPosY, deltaPosX) : 0.0f;
                activateRagdoll(std::cos(blastAngle) * 2.2f, std::sin(blastAngle) * 2.2f, 6.0f, 8.0f);
            }
        } else {
            m_stumbleTimer = std::max(m_stumbleTimer, 1.2f * intensity);
        }

        takeDamage((10.0f + falloffFactor * 28.0f) * intensity);
        if (m_health <= 0.0f) {
            kill("Fallecimiento por sacudida sismica y fractura del suelo");
            return;
        }
        m_action = HumanAction::IDLE;
        fleeInPanicFrom(quakeX, quakeY, 6.0f);
    }
}

void Humano::displaceByEarthquake(float offsetX, float offsetY, const WorldMap& mapRef) {
    if (m_health <= 0.0f) return;
    const int previousCellX = m_cellX;
    const int previousCellY = m_cellY;

    const float nextPosX = std::clamp(m_x + offsetX, 0.5f, static_cast<float>(mapRef.getWidth() - 1) + 0.5f);
    const float nextPosY = std::clamp(m_y + offsetY, 0.5f, static_cast<float>(mapRef.getHeight() - 1) + 0.5f);
    m_x = nextPosX;
    m_y = nextPosY;
    m_cellX = std::clamp(static_cast<int>(std::floor(m_x)), 0, mapRef.getWidth() - 1);
    m_cellY = std::clamp(static_cast<int>(std::floor(m_y)), 0, mapRef.getHeight() - 1);

    // EN: Check if displaced over a fault line or cliff edge into a chasm
    // ES: Verificar si fue desplazado sobre una falla o borde de risco hacia un abismo
    checkCliffFall(previousCellX, previousCellY, mapRef);

    if (mapRef.isValid(m_cellX, m_cellY)) {
        const Cell& currentCell = mapRef.getCell(m_cellX, m_cellY);
        if (currentCell.isWater() && !canWalkOnWaterAndDangerousTerrain()) {
            processWaterFall(mapRef);
        }
    }
}

void Humano::infectWithPandemic(float duration) {
    if (m_health <= 0.0f) return;
    if (!m_isInfected) {
        m_isInfected = true;
        m_infectionTimer = duration;
        m_coughTimer = 1.5f;
        triggerFaithIndicator(-1, 2.0f);
    }
}

// EN: Ignite unit with active flame effect, causing panic, running speed increase, and periodic burn damage
// ES: Encender unidad con efecto de llamas activo, causando pánico, mayor velocidad de huida y daño periódico por quemadura
void Humano::ignite(float durationSeconds) {
    if (m_health <= 0.0f) return;
    m_isBurning = true;
    m_burnTimer = std::max(m_burnTimer, durationSeconds);
    m_burnDamageTimer = 0.0f;
    m_panicTimer = std::max(m_panicTimer, durationSeconds * 0.8f);
    constexpr float burningPanicSpeed = 1.40f;
    m_speed = burningPanicSpeed;
    triggerFaithIndicator(-1, 2.0f);
}

void Humano::extinguishFire() {
    m_isBurning = false;
    m_burnTimer = 0.0f;
    m_burnDamageTimer = 0.0f;
}

// EN: Process falling into water bodies with probabilistic drowning and fatigue damage
// ES: Procesar caída en cuerpos de agua con ahogamiento probabilístico y daño de fatiga
void Humano::processWaterFall(const WorldMap& mapRef) {
    int cellX = std::clamp(static_cast<int>(std::floor(m_x)), 0, mapRef.getWidth() - 1);
    int cellY = std::clamp(static_cast<int>(std::floor(m_y)), 0, mapRef.getHeight() - 1);
    if (!mapRef.isValid(cellX, cellY)) return;

    const Cell& cell = mapRef.getCell(cellX, cellY);
    if (!cell.isWater()) return;

    // Los profetas poseen el don milagroso de caminar sobre las aguas sin hundirse
    if (canWalkOnWaterAndDangerousTerrain()) {
        m_isDrowning = false;
        m_isSwimming = false;
        m_altitude = 0.0f;
        m_flying = false;
        return;
    }

    // Probabilidad de ahogamiento según la naturaleza y profundidad del agua:
    // Océano Profundo: 75% probabilidad de morir ahogado
    // Aguas Someras / Ríos / Costas: 35% probabilidad de morir ahogado
    float drownProbability = (cell.biome == BiomeType::DeepOcean) ? 0.75f : 0.35f;

    std::uniform_real_distribution<float> dist01(0.0f, 1.0f);
    float roll = dist01(g_humanoRng);

    if (roll < drownProbability) {
        m_isDrowning = true;
        m_isSwimming = false;
        kill(cell.biome == BiomeType::DeepOcean ? "Ahogado en oceano profundo (75% mort.)" : "Ahogado en aguas someras (35% mort.)");
    } else {
        m_isDrowning = false;
        m_isSwimming = true;
        m_ragdoll.deactivate();
        float damage = (cell.biome == BiomeType::DeepOcean) ? 25.0f : 15.0f;
        takeDamage(damage);
        m_stumbleTimer = 1.0f;
        swimTowardsSolidGround(mapRef);
    }
}

void Humano::processGroundLanding(const WorldMap& mapRef) {
    m_flying = false;
    m_altitude = 0.0f;
    m_velocityZ = 0.0f;
    m_velocityX = 0.0f;
    m_velocityY = 0.0f;
    m_cellX = std::clamp(static_cast<int>(std::floor(m_x)), 0, mapRef.getWidth() - 1);
    m_cellY = std::clamp(static_cast<int>(std::floor(m_y)), 0, mapRef.getHeight() - 1);

    if (mapRef.isValid(m_cellX, m_cellY)) {
        const Cell& currentCell = mapRef.getCell(m_cellX, m_cellY);
        if (currentCell.isWater() && !canWalkOnWaterAndDangerousTerrain()) {
            processWaterFall(mapRef);
        } else {
            m_stumbleTimer = 1.5f;
            m_action = HumanAction::IDLE;
            m_ragdoll.deactivate();
        }
    }
}

// EN: Calculate and apply physics-based fall damage to living units based on impact velocity and terrain
// ES: Calcular y aplicar daño por caída basado en físicas a unidades vivas según velocidad de impacto y terreno
void Humano::applyFallDamage(float impactSpeed, const WorldMap& mapRef) {
    if (m_health <= 0.0f || impactSpeed <= SAFE_FALL_IMPACT_VELOCITY) {
        m_lastFallDamage = 0.0f;
        return;
    }

    // EN: Excess impact velocity above the safe human landing threshold
    // ES: Exceso de velocidad de impacto por encima del umbral de aterrizaje seguro humano
    const float excessSpeed = impactSpeed - SAFE_FALL_IMPACT_VELOCITY;

    // EN: Quadratic kinetic energy scaling for fall trauma
    // ES: Escala cinética cuadrática para el cálculo del trauma por caída
    float calculatedDamage = excessSpeed * FALL_DAMAGE_VELOCITY_COEFFICIENT *
                             (1.0f + (excessSpeed / FALL_DAMAGE_SEVERITY_SCALE));

    const int currentCellX = std::clamp(static_cast<int>(std::floor(m_x)), 0, mapRef.getWidth() - 1);
    const int currentCellY = std::clamp(static_cast<int>(std::floor(m_y)), 0, mapRef.getHeight() - 1);

    bool landedInWater = false;
    if (mapRef.isValid(currentCellX, currentCellY)) {
        const Cell& landingCell = mapRef.getCell(currentCellX, currentCellY);
        if (landingCell.isWater()) {
            landedInWater = true;
            // EN: Water provides 75% hydrodynamic dampening of fall impact
            // ES: El agua proporciona un 75% de amortiguación hidrodinámica del impacto de caída
            calculatedDamage *= WATER_FALL_DAMPING_FACTOR;
        } else if (landingCell.biome == BiomeType::RuggedMountains ||
                   landingCell.climate.primary_biome == BiomeID::RockyPeak ||
                   landingCell.hasImpactCrater) {
            // EN: Jagged basalt rocks and crater stones increase trauma by 30%
            // ES: Rocas escarpadas de basalto y piedras de cráter aumentan el trauma un 30%
            calculatedDamage *= ROCKY_TERRAIN_DAMAGE_MULTIPLIER;
        } else if (landingCell.biome == BiomeType::FertileValley ||
                   landingCell.biome == BiomeType::CoastalPlains) {
            // EN: Soft loam and grassy plains provide 15% cushioning
            // ES: Tierra fértil blanda y llanuras de hierba amortiguan un 15%
            calculatedDamage *= SOFT_TERRAIN_DAMAGE_MULTIPLIER;
        }
    }

    m_lastFallDamage = calculatedDamage;

    if (calculatedDamage > 0.5f) {
        takeDamage(calculatedDamage);
        m_stumbleTimer = std::min(4.5f, 0.8f + (calculatedDamage / 18.0f));

        if (m_health <= 0.0f) {
            const std::string deathReason = landedInWater
                ? "Fallecimiento por impacto contra superficie de agua a alta velocidad"
                : "Fallecimiento por impacto contra el suelo tras caida libre";
            kill(deathReason);
            if (!m_ragdoll.isActive()) {
                activateRagdoll(m_velocityX * 0.4f, m_velocityY * 0.4f, 4.0f, 3.5f);
            }
        } else if (calculatedDamage >= 22.0f && !m_ragdoll.isActive()) {
            // EN: Heavy non-lethal impact knocks unit to the ground briefly
            // ES: Impacto severo no letal derriba a la unidad momentáneamente al suelo
            activateRagdoll(m_velocityX * 0.25f, m_velocityY * 0.25f, 4.5f, 4.0f);
        }
    }
}

// EN: Check cliff elevation drop and initiate free-fall if stepping off an edge
// ES: Verificar desnivel de riscos y precipicios e iniciar caída libre si se sale por un borde
void Humano::checkCliffFall(int previousCellX, int previousCellY, const WorldMap& mapRef) {
    if (m_health <= 0.0f || m_altitude > 0.0f || m_flying) return;
    // EN: Prophets possess miraculous grace and do not stumble or fall off cliffs
    // ES: Los profetas poseen gracia milagrosa y no tropiezan ni se despeñan por riscos
    if (canWalkOnWaterAndDangerousTerrain()) return;

    if (!mapRef.isValid(previousCellX, previousCellY) || !mapRef.isValid(m_cellX, m_cellY)) return;
    if (previousCellX == m_cellX && previousCellY == m_cellY) return;

    const Cell& prevCell = mapRef.getCell(previousCellX, previousCellY);
    const Cell& currCell = mapRef.getCell(m_cellX, m_cellY);

    // EN: Entering water is handled by processWaterFall
    // ES: El ingreso al agua lo procesa processWaterFall
    if (currCell.isWater()) return;

    const float previousElevation = prevCell.elevation;
    const float currentElevation = currCell.elevation;
    const float elevationDelta = previousElevation - currentElevation;

    // EN: If stepping off a significant vertical drop/cliff
    // ES: Si pisa al vacío por un desnivel vertical significativo
    if (elevationDelta > CLIFF_FALL_ELEVATION_THRESHOLD) {
        // EN: Convert elevation difference into vertical altitude above ground
        // ES: Convertir el desnivel en altitud vertical sobre el nuevo suelo
        m_altitude = elevationDelta * 12.0f;
        m_velocityZ = -4.0f;
        m_flying = true;
        m_stumbleTimer = 0.5f;
    }
}

// EN: Search for nearest dry/passable ground cell around the villager
// ES: Buscar celda de tierra firme transitable más cercana al aldeano
bool Humano::findNearbySolidGround(const WorldMap& mapRef, float& outWorldX, float& outWorldY) const {
    int currentCellX = std::clamp(static_cast<int>(std::floor(m_x)), 0, mapRef.getWidth() - 1);
    int currentCellY = std::clamp(static_cast<int>(std::floor(m_y)), 0, mapRef.getHeight() - 1);

    float shortestDistanceSq = 9999.0f;
    int bestCellX = -1;
    int bestCellY = -1;

    for (int searchRadius = 1; searchRadius <= 10; ++searchRadius) {
        for (int deltaCellY = -searchRadius; deltaCellY <= searchRadius; ++deltaCellY) {
            for (int deltaCellX = -searchRadius; deltaCellX <= searchRadius; ++deltaCellX) {
                if (std::abs(deltaCellX) != searchRadius && std::abs(deltaCellY) != searchRadius) continue;
                int neighborCellX = currentCellX + deltaCellX;
                int neighborCellY = currentCellY + deltaCellY;
                if (!mapRef.isValid(neighborCellX, neighborCellY)) continue;
                const Cell& neighborCell = mapRef.getCell(neighborCellX, neighborCellY);
                if (neighborCell.isPassableForGround()) {
                    float distanceSq = static_cast<float>(deltaCellX * deltaCellX + deltaCellY * deltaCellY);
                    if (distanceSq < shortestDistanceSq) {
                        shortestDistanceSq = distanceSq;
                        bestCellX = neighborCellX;
                        bestCellY = neighborCellY;
                    }
                }
            }
        }
        if (bestCellX != -1) break;
    }

    if (bestCellX != -1) {
        outWorldX = static_cast<float>(bestCellX) + 0.5f;
        outWorldY = static_cast<float>(bestCellY) + 0.5f;
        return true;
    }
    return false;
}

// EN: Swim towards nearest solid land upon falling into deep water
// ES: Nadar hacia tierra firme tras caer al agua
void Humano::swimTowardsSolidGround(const WorldMap& mapRef) {
    float destinationPosX = 0.0f;
    float destinationPosY = 0.0f;
    if (findNearbySolidGround(mapRef, destinationPosX, destinationPosY)) {
        walkTowards(destinationPosX, destinationPosY, true);
        m_action = HumanAction::CORRER;
    }
}

// EN: Choose gathering resource type based on local environment, biome and tribal needs
// ES: Elegir qué recolectar según el entorno, bioma y necesidades de la tribu
ResourceType Humano::chooseGatheringResourceType(const WorldMap& mapRef, const VillageResources* villageResourcesRef) {
    const int currentCellX = static_cast<int>(std::floor(m_x));
    const int currentCellY = static_cast<int>(std::floor(m_y));
    if (!mapRef.isValid(currentCellX, currentCellY)) return ResourceType::Food;

    const Cell& currentCell = mapRef.getCell(currentCellX, currentCellY);

    // 1. Prioridad de supervivencia: si la comida es escasa en la comunidad o el aldeano tiene hambre
    const bool isFoodScarce = (villageResourcesRef && (villageResourcesRef->food < 150 || villageResourcesRef->isStarving)) ||
                              (m_popGroup.foodSupply < 45.0f) ||
                              (m_humanUnit.nutritionalState < 45.0f);

    if (isFoodScarce) {
        if (currentCell.naturalResource == ResourceType::Food && currentCell.resourceAmount > 0) {
            return ResourceType::Food;
        }
        const auto neighbors = mapRef.getNeighbors(currentCellX, currentCellY, false);
        for (const auto& neighborPos : neighbors) {
            if (!mapRef.isValid(neighborPos.x, neighborPos.y)) continue;
            const Cell& neighborCell = mapRef.getCell(neighborPos.x, neighborPos.y);
            if (neighborCell.naturalResource == ResourceType::Food && neighborCell.resourceAmount > 0) {
                return ResourceType::Food;
            }
        }
        return ResourceType::Food;
    }

    // 2. Si la supervivencia está asegurada, el aldeano elige según el entorno en el que vive:
    // A) En o cerca de bosques densos: talar madera para combustible, herramientas y chozas
    if ((currentCell.biome == BiomeType::DenseForest || currentCell.naturalResource == ResourceType::Wood) && currentCell.resourceAmount > 0) {
        return ResourceType::Wood;
    }
    // B) En colinas o montañas: extraer piedra o vetas de diamante / oro / cobre / hierro
    if (currentCell.biome == BiomeType::HighlandHills || currentCell.biome == BiomeType::RuggedMountains) {
        if (currentCell.naturalResource == ResourceType::Diamond && currentCell.resourceAmount > 0) {
            return ResourceType::Diamond;
        }
        if (currentCell.naturalResource == ResourceType::Gold && currentCell.resourceAmount > 0) {
            return ResourceType::Gold;
        }
        if (currentCell.naturalResource == ResourceType::Copper && currentCell.resourceAmount > 0) {
            return ResourceType::Copper;
        }
        if (currentCell.naturalResource == ResourceType::Iron && currentCell.resourceAmount > 0) {
            return ResourceType::Iron;
        }
        if (currentCell.naturalResource == ResourceType::Stone && currentCell.resourceAmount > 0) {
            return ResourceType::Stone;
        }
    }
    // C) En desierto: cantería / arcilla / arenisca
    if (currentCell.biome == BiomeType::AridDesert && currentCell.resourceAmount > 0) {
        return ResourceType::Stone;
    }
    // D) En valles fértiles, costas o llanuras: agricultura, pesca y reservas de alimento
    if (currentCell.naturalResource == ResourceType::Food && currentCell.resourceAmount > 0) {
        return ResourceType::Food;
    }

    // Aprovechar el recurso natural que tenga stock en la celda
    if (currentCell.naturalResource != ResourceType::None && currentCell.resourceAmount > 0) {
        return currentCell.naturalResource;
    }

    // Si la celda está agotada, busca en las vecinas
    const auto neighbors = mapRef.getNeighbors(currentCellX, currentCellY, false);
    for (const auto& neighborPos : neighbors) {
        if (!mapRef.isValid(neighborPos.x, neighborPos.y)) continue;
        const Cell& neighborCell = mapRef.getCell(neighborPos.x, neighborPos.y);
        if (neighborCell.naturalResource != ResourceType::None && neighborCell.resourceAmount > 0) {
            return neighborCell.naturalResource;
        }
    }

    return ResourceType::Food;
}

bool Humano::findResourceNode(const WorldMap& mapRef, ResourceType resourceType, int maxRadius, Point2i& outCellPos) const {
    float bestDistSq = 999999.0f;
    bool found = false;
    for (int deltaCellY = -maxRadius; deltaCellY <= maxRadius; ++deltaCellY) {
        for (int deltaCellX = -maxRadius; deltaCellX <= maxRadius; ++deltaCellX) {
            int testCellX = m_cellX + deltaCellX;
            int testCellY = m_cellY + deltaCellY;
            if (mapRef.isValid(testCellX, testCellY)) {
                const Cell& testCell = mapRef.getCell(testCellX, testCellY);
                if (testCell.naturalResource == resourceType && testCell.resourceAmount > 0) {
                    float distSq = static_cast<float>(deltaCellX * deltaCellX + deltaCellY * deltaCellY);
                    if (distSq < bestDistSq) {
                        bestDistSq = distSq;
                        outCellPos = {testCellX, testCellY};
                        found = true;
                    }
                }
            }
        }
    }
    return found;
}

// EN: Search for the closest available natural resource node of any type with positive stock
// ES: Buscar el nodo de recursos naturales disponible más cercano de cualquier tipo con existencias
bool Humano::findAnyResourceNode(const WorldMap& mapRef, int maxRadius, Point2i& outCellPos, ResourceType& outResourceType) const {
    float bestDistSq = 999999.0f;
    bool found = false;
    for (int deltaCellY = -maxRadius; deltaCellY <= maxRadius; ++deltaCellY) {
        for (int deltaCellX = -maxRadius; deltaCellX <= maxRadius; ++deltaCellX) {
            const int testCellX = m_cellX + deltaCellX;
            const int testCellY = m_cellY + deltaCellY;
            if (mapRef.isValid(testCellX, testCellY)) {
                const Cell& testCell = mapRef.getCell(testCellX, testCellY);
                if (testCell.naturalResource != ResourceType::None && testCell.resourceAmount > 0) {
                    const float distSq = static_cast<float>(deltaCellX * deltaCellX + deltaCellY * deltaCellY);
                    if (distSq < bestDistSq) {
                        bestDistSq = distSq;
                        outCellPos = {testCellX, testCellY};
                        outResourceType = testCell.naturalResource;
                        found = true;
                    }
                }
            }
        }
    }
    return found;
}

bool Humano::findShrineOrAltar(const WorldMap& mapRef, int religionId, int maxRadius, Point2i& outCellPos) const {
    float bestDistSq = 999999.0f;
    bool found = false;
    for (int deltaCellY = -maxRadius; deltaCellY <= maxRadius; ++deltaCellY) {
        for (int deltaCellX = -maxRadius; deltaCellX <= maxRadius; ++deltaCellX) {
            int testCellX = m_cellX + deltaCellX;
            int testCellY = m_cellY + deltaCellY;
            if (mapRef.isValid(testCellX, testCellY)) {
                const Cell& testCell = mapRef.getCell(testCellX, testCellY);
                if (testCell.hasHolySite && (religionId <= 0 || testCell.holySiteReligionId == religionId || testCell.dominantReligionId == religionId)) {
                    float distSq = static_cast<float>(deltaCellX * deltaCellX + deltaCellY * deltaCellY);
                    if (distSq < bestDistSq) {
                        bestDistSq = distSq;
                        outCellPos = {testCellX, testCellY};
                        found = true;
                    }
                }
            }
        }
    }
    return found;
}

bool Humano::findCommunityCenter(const WorldMap& mapRef, int religionId, int maxRadius, Point2i& outCellPos) const {
    return findShrineOrAltar(mapRef, religionId, maxRadius, outCellPos);
}


void Humano::update(float deltaTime, const WorldMap& mapRef,
                    const std::vector<Humano>* totalPopulation,
                    bool isNightTime,
                    const BuildingSystem* buildingSystemRef) {
    if (m_health <= 0.0f) {
        if (m_ragdoll.isActive()) {
            m_ragdoll.update(deltaTime, m_x, m_y, m_altitude, m_velocityX, m_velocityY, m_velocityZ, mapRef, true);
        }
        // EN: Only decrement death timer once the body has landed and settled on ground
        // ES: Solo decrementar el temporizador de defuncion una vez que el cuerpo reposa en el suelo
        if (!m_ragdoll.isActive() || m_ragdoll.isSettled() || m_altitude <= 0.0f) {
            if (m_deathTimer > 0.0f) {
                m_deathTimer = std::max(0.0f, m_deathTimer - deltaTime);
            }
        }
        m_cellX = std::clamp(static_cast<int>(std::floor(m_x)), 0, mapRef.getWidth() - 1);
        m_cellY = std::clamp(static_cast<int>(std::floor(m_y)), 0, mapRef.getHeight() - 1);
        return;
    }

    if (m_ragdoll.isActive()) {
        m_ragdoll.update(deltaTime, m_x, m_y, m_altitude, m_velocityX, m_velocityY, m_velocityZ, mapRef, false);
        m_cellX = std::clamp(static_cast<int>(std::floor(m_x)), 0, mapRef.getWidth() - 1);
        m_cellY = std::clamp(static_cast<int>(std::floor(m_y)), 0, mapRef.getHeight() - 1);

        // EN: Evaluate impact velocity recorded during ragdoll bounce or ground landing
        // ES: Evaluar velocidad de impacto registrada durante rebote o aterrizaje en ragdoll
        const float impactSpeed = m_ragdoll.consumeLastImpactVelocity();
        if (impactSpeed > 0.0f) {
            applyFallDamage(impactSpeed, mapRef);
        }

        if (m_ragdoll.isSettled() && m_altitude <= 0.0f) {
            m_flying = false;
            m_altitude = 0.0f;
            if (mapRef.isValid(m_cellX, m_cellY) && mapRef.getCell(m_cellX, m_cellY).isWater()) {
                processWaterFall(mapRef);
            } else {
                processGroundLanding(mapRef);
            }
        }
        return;
    }

    // EN: Flight and free-fall physics (tornado vortex, cliff drop, or physical launch)
    // ES: Físicas de vuelo y caída libre (vórtice de tornado, precipicio o lanzamiento físico)
    if (!m_ragdoll.isActive() && (m_altitude > 0.0f || m_flying)) {
        m_altitude += m_velocityZ * deltaTime;
        m_x += m_velocityX * deltaTime;
        m_y += m_velocityY * deltaTime;

        m_velocityX *= std::max(0.0f, 1.0f - deltaTime * 0.9f);
        m_velocityY *= std::max(0.0f, 1.0f - deltaTime * 0.9f);
        m_velocityZ -= GRAVITY_ACCELERATION_UNITS_S2 * deltaTime;

        if (m_altitude <= 0.0f) {
            const float impactVelocity = std::abs(m_velocityZ);
            m_altitude = 0.0f;
            m_velocityZ = 0.0f;
            m_velocityX = 0.0f;
            m_velocityY = 0.0f;
            m_flying = false;
            applyFallDamage(impactVelocity, mapRef);
            processGroundLanding(mapRef);
        }

        m_cellX = std::clamp(static_cast<int>(std::floor(m_x)), 0, mapRef.getWidth() - 1);
        m_cellY = std::clamp(static_cast<int>(std::floor(m_y)), 0, mapRef.getHeight() - 1);
        return;
    }

    if (m_panicTimer > 0.0f) {
        m_panicTimer = std::max(0.0f, m_panicTimer - deltaTime);
    }
    if (m_stumbleTimer > 0.0f) {
        m_stumbleTimer = std::max(0.0f, m_stumbleTimer - deltaTime);
    }
    if (m_damageTimer > 0.0f) {
        m_damageTimer = std::max(0.0f, m_damageTimer - deltaTime);
    }
    if (m_faithIndicatorTimer > 0.0f) {
        m_faithIndicatorTimer = std::max(0.0f, m_faithIndicatorTimer - deltaTime);
        if (m_faithIndicatorTimer <= 0.0f) {
            m_faithIndicatorSign = 0;
        }
    }
    if (m_attackCooldown > 0.0f) {
        m_attackCooldown = std::max(0.0f, m_attackCooldown - deltaTime);
    }
    if (m_fertilityCooldown > 0.0f) {
        m_fertilityCooldown = std::max(0.0f, m_fertilityCooldown - deltaTime);
    }

    // EN: Life cycle update: aging and reaching biological adulthood
    // ES: Actualización del ciclo vital: envejecimiento y alcance de la madurez biológica
    m_age += deltaTime * 0.25f;
    if (m_isChild && m_age >= 12.0f) {
        m_isChild = false; // Madurez biológica: pasa a ser adulto de la comunidad
    }

    // EN: Living organism biological reproductive and parental drives
    // ES: Impulsos reproductivos y parentales del organismo vivo
    if (!m_isChild && m_age >= 12.0f && !m_isPregnant) {
        // EN: Adults desire reproduction when healthy and fed
        // ES: Los adultos desean reproducirse cuando están sanos y nutridos
        const float metabolicVitality = (m_humanUnit.nutritionalState / 100.0f) * (m_health / 100.0f);
        if (metabolicVitality >= 0.35f && m_panicTimer <= 0.0f) {
            const float reproductionDriveRate = tienePareja() ? 0.016f : 0.028f;
            modifyReproductionDrive(reproductionDriveRate * metabolicVitality * deltaTime);
        } else {
            modifyReproductionDrive(-0.020f * deltaTime);
        }
    } else {
        m_reproductionDrive = 0.0f;
    }

    if (m_courtshipTimer > 0.0f) {
        m_courtshipTimer = std::max(0.0f, m_courtshipTimer - deltaTime);
    }
    if (m_parentalCareTimer > 0.0f) {
        m_parentalCareTimer = std::max(0.0f, m_parentalCareTimer - deltaTime);
    }

    // EN: Gestation advancement for pregnant mothers
    // ES: Avance de la gestación para madres embarazadas
    if (m_isPregnant) {
        m_pregnancyProgress = std::min(1.0f, m_pregnancyProgress + deltaTime * 0.045f);
    }

    // EN: Protective fury instinct
    // ES: Instinto de furia protectora
    if (m_protectiveFuryTimer > 0.0f) {
        m_protectiveFuryTimer = std::max(0.0f, m_protectiveFuryTimer - deltaTime);
        if (m_protectiveFuryTimer <= 0.0f) {
            m_targetProtectedId = -1;
        }
    }

    // EN: Circadian sleep behavior
    // ES: Si la unidad está durmiendo, permanece inmóvil y reposa
    if (m_isSleeping) {
        m_action = HumanAction::IDLE;
        m_speed = 0.0f;
        m_targetDestX = m_x;
        m_targetDestY = m_y;

        float shelterBonusFactor = m_isSleepingIndoors ? 1.75f : 1.0f;
        modifySleepNeed(-18.0f * shelterBonusFactor * deltaTime);
        m_humanUnit.energy = std::min(100.0f, m_humanUnit.energy + 20.0f * shelterBonusFactor * deltaTime);
        m_health = std::min(100.0f, m_health + 2.0f * shelterBonusFactor * deltaTime);
        m_humanUnit.hitPoints = m_health;
    }

    // EN: Pandemic infection progress
    // ES: Infección de pandemia
    if (m_isInfected) {
        m_infectionTimer = std::max(0.0f, m_infectionTimer - deltaTime);
        m_coughTimer = std::max(0.0f, m_coughTimer - deltaTime);
        if (m_coughTimer <= 0.0f) {
            m_coughTimer = 2.0f;
            takeDamage(2.5f);
        }
        if (m_infectionTimer <= 0.0f) {
            m_isInfected = false;
        }
    }

    // EN: Process burning status effect: panic sprint, burn damage, extinguishing in water, and contact ignition
    // ES: Procesar efecto de estado en llamas: carrera en pánico, daño por fuego, extinción en agua e ignición por contacto
    if (m_isBurning && m_health > 0.0f) {
        m_burnTimer -= deltaTime;
        m_burnDamageTimer += deltaTime;

        // EN: Periodic fire damage
        // ES: Daño periódico por quemadura
        if (m_burnDamageTimer >= 0.5f) {
            m_burnDamageTimer = 0.0f;
            constexpr float fireTickDamage = 7.0f;
            recibirDano(fireTickDamage);
            if (m_health <= 0.0f) {
                matar("Calcinado por el fuego envolvente");
                m_isBurning = false;
            }
        }

        // EN: Extinguish immediately if the unit steps into water
        // ES: Extinguir de inmediato si la unidad pisa agua
        const int currentCellCoordX = getCeldaX();
        const int currentCellCoordY = getCeldaY();
        if (mapRef.isValid(currentCellCoordX, currentCellCoordY) && mapRef.getCell(currentCellCoordX, currentCellCoordY).isWater()) {
            extinguishFire();
        } else {
            // EN: Sprinted panic flight while on fire
            // ES: Carrera en pánico acelerada mientras arde
            constexpr float panicSprintSpeed = 1.40f;
            m_speed = panicSprintSpeed;
            m_panicTimer = std::max(m_panicTimer, 1.5f);

            // EN: Contact ignition: chance to ignite flammable dry terrain underfoot
            // ES: Ignición por contacto: posibilidad de encender terreno seco bajo sus pies
            if (mapRef.isValid(currentCellCoordX, currentCellCoordY)) {
                Cell& footCell = const_cast<WorldMap&>(mapRef).getCell(currentCellCoordX, currentCellCoordY);
                if (!footCell.isBurning && !footCell.isBurnt && footCell.isCombustible()) {
                    const_cast<WorldMap&>(mapRef).igniteCell(currentCellCoordX, currentCellCoordY, 12.0f, 0.70f);
                }
            }
        }

        if (m_burnTimer <= 0.0f) {
            extinguishFire();
        }
    }

    // EN: Alcohol intoxication metabolism and decay
    // ES: Metabolismo del alcohol y reducción progresiva de la embriaguez
    if (m_alcoholLevel > 0.0f) {
        m_alcoholLevel = std::max(0.0f, m_alcoholLevel - deltaTime * 1.5f);
        // EN: Heavy drunkenness suppresses fear and panic completely
        // ES: La embriaguez intensa suprime el miedo y el pánico por completo
        if (m_alcoholLevel > 35.0f && m_panicTimer > 0.0f) {
            m_panicTimer = 0.0f;
        }
    }

    // EN: Aggression level decay over time
    // ES: Disipación progresiva del nivel de agresividad
    if (m_aggressionLevel > 0.0f) {
        m_aggressionLevel = std::max(0.0f, m_aggressionLevel - deltaTime * 4.0f);
    }

    // EN: Continuous physiological condition modifiers (Full, Hungry, Sick, Inspired)
    // ES: Modificadores fisiológicos continuos de condición (Lleno, Hambriento, Enfermo, Inspirado)
    const VillagerCondition currentCondition = getCondition();
    if (currentCondition == VillagerCondition::Full) {
        // EN: Full condition provides continuous stamina boost and gentle cell regeneration
        // ES: Estar lleno otorga regeneración continua de estamina y salud suave
        m_humanUnit.energy = std::min(100.0f, m_humanUnit.energy + 3.0f * deltaTime);
        m_health = std::min(100.0f, m_health + 0.6f * deltaTime);
        m_humanUnit.hitPoints = m_health;
    } else if (currentCondition == VillagerCondition::Hungry) {
        // EN: Hungry condition drains energy faster; if starvation reaches 0, unit takes damage
        // ES: El hambre drena energía más rápido; si la inanición llega a 0, sufre daño biológico
        m_humanUnit.energy = std::max(0.0f, m_humanUnit.energy - 1.2f * deltaTime);
        if (m_humanUnit.nutritionalState <= 0.0f) {
            takeDamage(1.5f * deltaTime);
        }
    } else if (currentCondition == VillagerCondition::Sick) {
        // EN: Sickness causes fatigue accumulation and stamina drain
        // ES: La enfermedad causa fatiga acumulativa y desgaste de estamina
        m_humanUnit.energy = std::max(0.0f, m_humanUnit.energy - 2.0f * deltaTime);
    } else if (currentCondition == VillagerCondition::Inspired) {
        // EN: Sacred inspiration rejuvenates energy and spiritual willpower
        // ES: La inspiración sagrada rejuvenece energía y fuerza de voluntad espiritual
        m_humanUnit.energy = std::min(100.0f, m_humanUnit.energy + 2.5f * deltaTime);
        m_humanUnit.willpower = std::min(100.0f, m_humanUnit.willpower + 1.5f * deltaTime);
    }

    m_animTimer += deltaTime;
    if (m_stateTimer > 0.0f) {
        m_stateTimer = std::max(0.0f, m_stateTimer - deltaTime);
    }

    // EN: Walking/Running movement execution with active condition speed modulation and drunken sway
    // ES: Ejecución de desplazamiento con modulación de velocidad según condición activa y tambaleo de ebriedad
    if (m_action == HumanAction::CAMINAR || m_action == HumanAction::CORRER) {
        float deltaPosX = m_targetDestX - m_x;
        float deltaPosY = m_targetDestY - m_y;
        float distance = std::sqrt(deltaPosX * deltaPosX + deltaPosY * deltaPosY);

        if (distance > 0.05f) {
            const float effectiveMoveSpeed = getEffectiveSpeed();
            float moveStep = effectiveMoveSpeed * deltaTime;
            if (moveStep > distance) moveStep = distance;

            float nextPosX = m_x + (deltaPosX / distance) * moveStep;
            float nextPosY = m_y + (deltaPosY / distance) * moveStep;

            // EN: If villager is drunk, apply sinusoidal lateral sway perpendicular to movement vector
            // ES: Si el aldeano está borracho, aplicar bamboleo sinusoidal lateral perpendicular al vector
            if (isDrunk()) {
                const float perpAxisX = -deltaPosY / distance;
                const float perpAxisY = deltaPosX / distance;
                const float drunkennessFactor = m_alcoholLevel / 100.0f;
                const float swayDisplacement = std::sin(m_animTimer * 5.0f + static_cast<float>(m_id * 3)) * (0.016f * drunkennessFactor);
                nextPosX += perpAxisX * swayDisplacement;
                nextPosY += perpAxisY * swayDisplacement;
            }

            int nextCellX = static_cast<int>(std::floor(nextPosX));
            int nextCellY = static_cast<int>(std::floor(nextPosY));

            // EN: Ground units cannot walk on water or dangerous terrain (chasms/lava).
            // ES: Las unidades terrestres no pueden caminar por agua ni terreno peligroso.
            if (!canWalkOnWaterAndDangerousTerrain()) {
                bool nextIsDangerous = !mapRef.isValid(nextCellX, nextCellY) ||
                                       !mapRef.getCell(nextCellX, nextCellY).isPassableForGround();
                if (nextIsDangerous) {
                    if (mapRef.isValid(m_cellX, m_cellY) && mapRef.getCell(m_cellX, m_cellY).isPassableForGround()) {
                        m_action = HumanAction::IDLE;
                        m_speed = 0.65f;
                        m_hasManualOrder = false;
                        m_stateTimer = 2.5f;
                        return;
                    }
                }
            }

            const int previousMoveCellX = m_cellX;
            const int previousMoveCellY = m_cellY;

            m_x = nextPosX;
            m_y = nextPosY;

            if (std::abs(deltaPosX) > std::abs(deltaPosY)) {
                m_direction = (deltaPosX > 0) ? IsoDirection::SE : IsoDirection::NW;
            } else {
                m_direction = (deltaPosY > 0) ? IsoDirection::SW : IsoDirection::NE;
            }

            m_cellX = static_cast<int>(std::floor(m_x));
            m_cellY = static_cast<int>(std::floor(m_y));

            checkCliffFall(previousMoveCellX, previousMoveCellY, mapRef);

            // EN: If holy site, circle naturally
            // ES: Si la celda tiene santuario, rodear con respeto
            if (mapRef.isValid(m_cellX, m_cellY) && mapRef.getCell(m_cellX, m_cellY).hasHolySite) {
                float centerCellX = static_cast<float>(m_cellX) + 0.50f;
                float centerCellY = static_cast<float>(m_cellY) + 0.50f;
                float relRadiusX = m_x - centerCellX;
                float relRadiusY = m_y - centerCellY;
                float relDistance = std::sqrt(relRadiusX * relRadiusX + relRadiusY * relRadiusY);
                if (relDistance < 0.42f) {
                    if (relDistance < 0.001f) { relRadiusX = 0.30f; relRadiusY = 0.30f; relDistance = 0.424f; }
                    m_x = centerCellX + (relRadiusX / relDistance) * 0.43f;
                    m_y = centerCellY + (relRadiusY / relDistance) * 0.43f;
                }
            }
        } else {
            m_x = m_targetDestX;
            m_y = m_targetDestY;
            m_cellX = static_cast<int>(std::floor(m_x));
            m_cellY = static_cast<int>(std::floor(m_y));
            if (m_hasManualOrder) {
                m_hasManualOrder = false;
                m_action = HumanAction::IDLE;
                m_speed = 0.65f;
                m_stateTimer = 6.0f;
            } else {
                makeAIDecision(mapRef, totalPopulation, isNightTime, buildingSystemRef);
            }
        }
    } else {
        if (m_stateTimer <= 0.0f) {
            makeAIDecision(mapRef, totalPopulation, isNightTime, buildingSystemRef);
        }
    }
}


void Humano::makeAIDecision(const WorldMap& mapa, const std::vector<Humano>* poblacionTotal,
                             bool esDeNoche, const BuildingSystem* buildingSys) {
    if (estaMuerto() || isRagdoll() || m_altitude > 0.0f || m_flying || m_stumbleTimer > 0.0f) {
        return;
    }

    std::uniform_real_distribution<float> fDist(0.0f, 1.0f);
    std::uniform_real_distribution<float> durDist(2.5f, 4.5f);
    m_stateTimer = durDist(g_humanoRng);

    int curX = std::clamp(static_cast<int>(std::floor(m_x)), 0, mapa.getWidth() - 1);
    int curY = std::clamp(static_cast<int>(std::floor(m_y)), 0, mapa.getHeight() - 1);

    if (!mapa.isValid(curX, curY)) {
        m_x = 20.5f; m_y = 20.5f;
        m_targetDestX = 20.5f; m_targetDestY = 20.5f;
        m_action = HumanAction::IDLE;
        return;
    }

    const Cell& celda = mapa.getCell(curX, curY);
    if (m_religionId <= 0 && celda.dominantReligionId > 0) {
        m_religionId = celda.dominantReligionId;
    }

    // 0A. INSTINTO DE FURIA PROTECTORA Y AUXILIO FAMILIAR
    if (tieneFuriaProtectora()) {
        m_isSleeping = false;
        m_isSleepingIndoors = false;
        if (m_targetEnemyId > 0 && poblacionTotal) {
            const Humano* enemigo = nullptr;
            for (const auto& other : *poblacionTotal) {
                if (other.getId() == m_targetEnemyId && other.estaVivo()) {
                    enemigo = &other;
                    break;
                }
            }
            if (enemigo) {
                float edx = enemigo->getX() - m_x;
                float edy = enemigo->getY() - m_y;
                float edist = std::sqrt(edx * edx + edy * edy);
                m_humanUnit.currentState = HumanAIState::DEFENDING;
                if (edist <= 1.8f) {
                    cambiarAccion(HumanAction::ATACAR);
                    orientarHacia(enemigo->getX(), enemigo->getY());
                    m_stateTimer = 1.0f;
                    return;
                } else {
                    caminarHacia(enemigo->getX(), enemigo->getY(), true);
                    m_speed = 1.25f;
                    m_stateTimer = 1.5f;
                    return;
                }
            } else {
                m_protectiveFuryTimer = 0.0f;
                m_targetProtectedId = -1;
                m_targetEnemyId = -1;
            }
        }
    }

    // 0B. COMPORTAMIENTO DE COMBATE ACTIVO O HUIDA DE UNIDADES VULNERABLES
    if (m_targetEnemyId > 0 && poblacionTotal) {
        bool enemyAlive = false;
        for (const auto& other : *poblacionTotal) {
            if (other.getId() == m_targetEnemyId && other.estaVivo()) {
                enemyAlive = true;
                break;
            }
        }
        if (!enemyAlive) {
            m_targetEnemyId = -1;
            if (m_action == HumanAction::ATACAR) {
                m_action = HumanAction::SALUDAR;
                m_humanUnit.currentState = HumanAIState::SOCIALIZING;
                triggerFaithIndicator(+1, 1.5f);
                return;
            }
        }
    }

    if (m_targetEnemyId > 0 || m_action == HumanAction::ATACAR) {
        bool esVulnerable = m_isChild || m_isPregnant ||
                            (m_health < 35.0f && !esFanatico() && m_humanUnit.role != CommunityRole::ZEALOT);
        if (esVulnerable) {
            // Retirada táctica a su hogar o santuario
            bool refugiado = false;
            if (tieneHogar() && buildingSys) {
                const House* h = buildingSys->getHouseById(m_homeId);
                if (h && h->isCompleted()) {
                    caminarHacia(static_cast<float>(h->cellX) + 0.5f, static_cast<float>(h->cellY) + 0.5f, true);
                    refugiado = true;
                }
            }
            if (!refugiado) {
                Point2i safePos;
                if (buscarSantuarioOAltar(mapa, m_religionId, 12, safePos) ||
                    buscarCentroComunidad(mapa, m_religionId, 12, safePos)) {
                    caminarHacia(static_cast<float>(safePos.x) + 0.5f, static_cast<float>(safePos.y) + 0.5f, true);
                } else {
                    huirEnPanicoDesde(m_x + 0.5f, m_y + 0.5f, 3.5f);
                }
            }
            m_targetEnemyId = -1;
            m_humanUnit.currentState = HumanAIState::SURVIVAL_WORKING;
            return;
        }
        // Combatiente apto: mantener postura de defensa o ataque
        m_humanUnit.currentState = HumanAIState::DEFENDING;
        return;
    }

    // =========================================================================
    // EN: 0C. Circadian rhythm, biological sleep, and morning wake-up
    // ES: 0C. Sistema Circadiano, Sueño y Despertar Matutino
    // =========================================================================
    if (m_isSleeping) {
        // EN: Evaluate whether to wake up (daylight has arrived or fully rested or protective fury)
        // ES: Evaluar si despertar (llegó la luz del día, descanso suficiente o furia protectora)
        bool isRested = (m_sleepNeed <= 40.0f || m_humanUnit.energy >= 55.0f);
        bool shouldWakeUp = (!esDeNoche && (isRested || m_humanUnit.energy >= 30.0f)) || tieneFuriaProtectora();

        if (shouldWakeUp) {
            setDurmiendo(false);
            setDurmiendoBajoTecho(false);
            m_action = HumanAction::IDLE;
            m_speed = 0.65f;
            m_stateTimer = 0.5f;
            return;
        } else {
            // EN: Continue sleeping peacefully during nighttime or severe fatigue
            // ES: Continúa durmiendo en reposo de noche o ante fatiga severa
            m_humanUnit.currentState = HumanAIState::SURVIVAL_WORKING;
            m_action = HumanAction::IDLE;
            m_speed = 0.0f;
            m_stateTimer = 3.0f;
            return;
        }
    } else {
        // EN: Low energy rest priority (stamina recovery pause during daytime)
        // ES: Prioridad de descanso por baja energía (pausa para recuperar estamina durante el día)
        if (!esDeNoche && (m_humanUnit.energy < 20.0f || m_sleepNeed >= 90.0f) && !m_hasManualOrder && m_panicTimer <= 0.0f) {
            m_humanUnit.currentState = HumanAIState::SURVIVAL_WORKING;
            m_action = HumanAction::IDLE;
            m_speed = 0.0f;
            m_stateTimer = 2.5f;
            return;
        }

        // EN: Evaluate biological sleep necessity at night
        // ES: Evaluar necesidad de sueño biológico durante la noche
        bool needsNightSleep = esDeNoche && (m_sleepNeed >= 25.0f || m_humanUnit.energy < 75.0f);

        if (needsNightSleep && !m_hasManualOrder && m_panicTimer <= 0.0f) {
            m_humanUnit.currentState = HumanAIState::SURVIVAL_WORKING;
            // Opción 1: Dormir en su vivienda construida
            if (tieneHogar() && buildingSys) {
                const House* h = buildingSys->getHouseById(m_homeId);
                if (h && h->isCompleted()) {
                    float hx = static_cast<float>(h->cellX) + 0.5f;
                    float hy = static_cast<float>(h->cellY) + 0.5f;
                    float dHome = std::hypot(m_x - hx, m_y - hy);
                    if (dHome <= 0.85f) {
                        setDurmiendo(true);
                        setDurmiendoBajoTecho(true); // Se resguarda dentro de la choza
                        m_action = HumanAction::IDLE;
                        m_speed = 0.0f;
                        m_stateTimer = 4.0f;
                        return;
                    } else {
                        caminarHacia(hx, hy, false);
                        m_stateTimer = 3.5f;
                        return;
                    }
                }
            }

            // Opción 2: Sin hogar o casa en obras -> Dormir junto al fuego sagrado / altar comunal
            Point2i hearthPos;
            if (buscarSantuarioOAltar(mapa, m_religionId, 16, hearthPos) ||
                buscarCentroComunidad(mapa, m_religionId, 16, hearthPos)) {
                float fx = static_cast<float>(hearthPos.x) + 0.5f;
                float fy = static_cast<float>(hearthPos.y) + 0.5f;
                float dHearth = std::hypot(m_x - fx, m_y - fy);
                if (dHearth <= 1.3f) {
                    setDurmiendo(true);
                    setDurmiendoBajoTecho(false); // Duerme a la intemperie junto al fuego
                    m_action = HumanAction::IDLE;
                    m_speed = 0.0f;
                    m_stateTimer = 4.0f;
                    return;
                } else {
                    caminarHacia(fx, fy, false);
                    m_stateTimer = 3.5f;
                    return;
                }
            }

            // Fallback: descanso directo al aire libre nocturno
            setDurmiendo(true);
            setDurmiendoBajoTecho(false);
            m_action = HumanAction::IDLE;
            m_speed = 0.0f;
            m_stateTimer = 4.0f;
            return;
        }
    }

    // 0D. COMPORTAMIENTO DE INFANTES (Cuidado filial y apego a los padres)
    if (m_isChild && poblacionTotal) {
        const Humano* parent = nullptr;
        for (const auto& other : *poblacionTotal) {
            if (other.getId() == m_motherId && other.estaVivo()) {
                parent = &other;
                break;
            }
        }
        if (!parent && m_fatherId > 0) {
            for (const auto& other : *poblacionTotal) {
                if (other.getId() == m_fatherId && other.estaVivo()) {
                    parent = &other;
                    break;
                }
            }
        }
        if (parent) {
            float pDist = std::hypot(m_x - parent->getX(), m_y - parent->getY());
            // EN: If it is night and parent is sleeping nearby, child sleeps beside them
            // ES: Si es de noche y el progenitor duerme cerca, el infante se acurruca a dormir
            if (esDeNoche && parent->estaDurmiendo() && pDist <= 1.2f) {
                setDurmiendo(true);
                setDurmiendoBajoTecho(parent->estaDurmiendoBajoTecho());
                m_action = HumanAction::IDLE;
                m_speed = 0.0f;
                m_stateTimer = 3.5f;
                return;
            }
            if (pDist > 2.8f) {
                caminarHacia(parent->getX() + (fDist(g_humanoRng) - 0.5f) * 0.7f,
                             parent->getY() + (fDist(g_humanoRng) - 0.5f) * 0.7f, false);
                m_stateTimer = 3.0f;
                return;
            } else {
                if (parent->getAccion() == HumanAction::REZAR) {
                    // EN: Child imitates praying parent, absorbing sacred rituals
                    // ES: El infante imita al progenitor en oración, absorbiendo ritos sagrados
                    m_action = HumanAction::REZAR;
                    m_humanUnit.currentState = HumanAIState::WORSHIPING;
                    m_devotion = std::min(1.0f, m_devotion + 0.05f);
                    m_stateTimer = 3.5f;
                    return;
                } else if (m_humanUnit.nutritionalState < 45.0f) {
                    // EN: Hungry child greets parent seeking food
                    // ES: Infante hambriento saluda al progenitor solicitando sustento
                    m_action = HumanAction::SALUDAR;
                    orientTowards(parent->getX(), parent->getY());
                    m_stateTimer = 2.0f;
                    return;
                } else {
                    m_action = (fDist(g_humanoRng) < 0.65f) ? HumanAction::SALUDAR : HumanAction::IDLE;
                    m_stateTimer = 2.5f;
                    return;
                }
            }
        }
    }

    // =========================================================================
    // EN: 0E. Urgent biological hunger and sustenance foraging
    // ES: 0E. Hambre biológica urgente y forrajeo de sustento
    // =========================================================================
    if ((m_humanUnit.nutritionalState < 38.0f || m_popGroup.foodSupply < 22.0f) && !m_isChild) {
        if (celda.naturalResource == ResourceType::Food && celda.resourceAmount > 0) {
            int harvestedFood = const_cast<WorldMap&>(mapa).getCell(curX, curY).harvestResource(2);
            if (harvestedFood > 0) {
                m_carriedResourceAmount = std::min(m_carryingCapacity, m_carriedResourceAmount + harvestedFood);
                m_gatheringResourceType = ResourceType::Food;
                m_humanUnit.nutritionalState = std::min(100.0f, m_humanUnit.nutritionalState + 25.0f);
                m_popGroup.foodSupply = std::min(100.0f, m_popGroup.foodSupply + 20.0f);
                m_health = std::min(100.0f, m_health + 10.0f);

                // EN: Occasional fermentation in wild berries induces merriment and mild drunkenness
                // ES: La fermentación ocasional en bayas silvestres induce alegría y embriaguez leve
                if (fDist(g_humanoRng) < 0.14f) {
                    consumeAlcohol(35.0f);
                }

                m_action = HumanAction::RECOLECTAR;
                m_humanUnit.currentState = HumanAIState::SURVIVAL_WORKING;
                m_stateTimer = 3.0f;
                return;
            }
        }
        Point2i foodNodeLocation{-1, -1};
        if (buscarNodoRecurso(mapa, ResourceType::Food, 16, foodNodeLocation)) {
            m_gatheringResourceType = ResourceType::Food;
            m_humanUnit.currentState = HumanAIState::SURVIVAL_WORKING;
            caminarHacia(static_cast<float>(foodNodeLocation.x) + 0.5f,
                         static_cast<float>(foodNodeLocation.y) + 0.5f, false);
            m_stateTimer = 3.5f;
            return;
        }
    }

    // =========================================================================
    // EN: 0E2. Active Condition Behavioral AI: Drunk, Sick, Aggressive, Scared, Exhausted
    // ES: 0E2. IA Conductual por Condición Activa: Borracho, Enfermo, Agresivo, Asustado, Exhausto
    // =========================================================================
    const VillagerCondition activeCondition = getCondition();

    // EN: Drunk villager: euphoric revelry, slurred singing and cheerful socializing with peers
    // ES: Aldeano borracho: jolgorio eufórico, cantos y saludos festivos con miembros de la tribu
    if (activeCondition == VillagerCondition::Drunk && !m_isChild) {
        if (poblacionTotal && fDist(g_humanoRng) < 0.55f) {
            for (const auto& peerUnit : *poblacionTotal) {
                if (&peerUnit != this && peerUnit.estaVivo()) {
                    const float peerDistance = std::hypot(m_x - peerUnit.getX(), m_y - peerUnit.getY());
                    if (peerDistance <= 3.5f) {
                        orientTowards(peerUnit.getX(), peerUnit.getY());
                        m_action = HumanAction::SALUDAR;
                        m_humanUnit.currentState = HumanAIState::SOCIALIZING;
                        m_stateTimer = 2.5f;
                        return;
                    }
                }
            }
        }
    }

    // EN: Sick villager: seeks shelter/bed to recuperate or altar/shrine to pray for healing
    // ES: Aldeano enfermo: busca cobijo en choza o altar para orar por sanación divina
    if (activeCondition == VillagerCondition::Sick && !m_isChild) {
        if (tieneHogar() && buildingSys) {
            const House* housePtr = buildingSys->getHouseById(m_homeId);
            if (housePtr && housePtr->isCompleted()) {
                const float houseDistance = std::hypot(m_x - (static_cast<float>(housePtr->cellX) + 0.5f),
                                                       m_y - (static_cast<float>(housePtr->cellY) + 0.5f));
                if (houseDistance <= 0.85f) {
                    setDurmiendo(true);
                    setDurmiendoBajoTecho(true);
                    m_action = HumanAction::IDLE;
                    m_stateTimer = 4.0f;
                    return;
                } else {
                    caminarHacia(static_cast<float>(housePtr->cellX) + 0.5f,
                                 static_cast<float>(housePtr->cellY) + 0.5f, false);
                    m_stateTimer = 3.5f;
                    return;
                }
            }
        }
        Point2i shrineLocation{-1, -1};
        if (buscarSantuarioOAltar(mapa, m_religionId, 16, shrineLocation)) {
            const float shrineDistance = std::hypot(m_x - (static_cast<float>(shrineLocation.x) + 0.5f),
                                                    m_y - (static_cast<float>(shrineLocation.y) + 0.5f));
            if (shrineDistance <= 1.5f) {
                m_action = HumanAction::REZAR;
                m_humanUnit.currentState = HumanAIState::WORSHIPING;
                m_stateTimer = 4.0f;
                return;
            } else {
                caminarHacia(static_cast<float>(shrineLocation.x) + 0.5f,
                             static_cast<float>(shrineLocation.y) + 0.5f, false);
                m_stateTimer = 3.5f;
                return;
            }
        }
    }

    // EN: Aggressive villager: actively stalks and charges rival clan members or hostile threats
    // ES: Aldeano agresivo: acecha y carga activamente contra rivales de otros clanes o amenazas
    if (activeCondition == VillagerCondition::Aggressive && !m_isChild && poblacionTotal) {
        const Humano* targetRival = nullptr;
        float closestRivalDistSq = 256.0f; // 16 celdas de radio
        for (const auto& otherVillager : *poblacionTotal) {
            if (&otherVillager == this || !otherVillager.estaVivo()) continue;
            if (otherVillager.getReligionId() != m_religionId && otherVillager.getReligionId() > 0) {
                const float deltaCoordX = otherVillager.getX() - m_x;
                const float deltaCoordY = otherVillager.getY() - m_y;
                const float distSquared = deltaCoordX * deltaCoordX + deltaCoordY * deltaCoordY;
                if (distSquared < closestRivalDistSq) {
                    closestRivalDistSq = distSquared;
                    targetRival = &otherVillager;
                }
            }
        }
        if (targetRival) {
            m_targetEnemyId = targetRival->getId();
            m_humanUnit.currentState = HumanAIState::ATTACKING;
            const float rivalDistance = std::sqrt(closestRivalDistSq);
            if (rivalDistance <= 1.4f) {
                cambiarAccion(HumanAction::ATACAR);
                orientTowards(targetRival->getX(), targetRival->getY());
                m_stateTimer = 1.0f;
            } else {
                caminarHacia(targetRival->getX(), targetRival->getY(), true);
                m_speed = 1.30f;
                m_stateTimer = 2.0f;
            }
            return;
        }
    }

    // EN: Scared villager: runs away in panic from threats, fire, or disasters
    // ES: Aldeano asustado: huye en pánico de amenazas, fuego o cataclismos
    if (activeCondition == VillagerCondition::Scared) {
        if (m_action != HumanAction::CORRER) {
            huirEnPanicoDesde(m_x + (fDist(g_humanoRng) - 0.5f), m_y + (fDist(g_humanoRng) - 0.5f), 4.0f);
            m_stateTimer = 2.5f;
            return;
        }
    }

    // EN: Exhausted villager: stamina depleted, collapses into sleep
    // ES: Aldeano exhausto: estamina agotada, se rinde al sueño de inmediato
    if (activeCondition == VillagerCondition::Exhausted && !m_isChild) {
        setDurmiendo(true);
        setDurmiendoBajoTecho(false);
        m_action = HumanAction::IDLE;
        m_speed = 0.0f;
        m_stateTimer = 3.5f;
        return;
    }

    // EN: Full villager: boost procreation desire and altruism
    // ES: Aldeano lleno: estimula deseo reproductivo y altruismo
    if (activeCondition == VillagerCondition::Full) {
        modifyReproductionDrive(0.08f);
    }

    // =========================================================================
    // EN: 0F. Parental investment: nourish hungry children
    // ES: 0F. Inversión parental: nutrir a hijos hambrientos
    // =========================================================================
    if (!m_isChild && m_childrenCount > 0 && poblacionTotal && m_humanUnit.nutritionalState > 45.0f && m_parentalCareTimer <= 0.0f) {
        for (const auto& candidateChild : *poblacionTotal) {
            if (candidateChild.estaVivo() && candidateChild.esNino() &&
                (candidateChild.getMadreId() == m_id || candidateChild.getPadreId() == m_id)) {
                if (candidateChild.getHumanUnit().nutritionalState < 48.0f) {
                    const float childDistance = std::hypot(m_x - candidateChild.getX(), m_y - candidateChild.getY());
                    if (childDistance <= 1.8f) {
                        feedChild(const_cast<Humano&>(candidateChild));
                        m_stateTimer = 2.5f;
                        return;
                    } else if (childDistance <= 16.0f) {
                        caminarHacia(candidateChild.getX(), candidateChild.getY(), false);
                        m_stateTimer = 3.0f;
                        return;
                    }
                }
            }
        }
    }

    // =========================================================================
    // EN: 0G. Living organism reproductive drive: active mate search & courtship
    // ES: 0G. Impulso reproductivo de organismo vivo: búsqueda activa de pareja y cortejo
    // =========================================================================
    if (!m_isChild && m_age >= 12.0f && !tienePareja() && poblacionTotal && !esDeNoche && m_sleepNeed < 75.0f) {
        if (m_reproductionDrive >= 0.25f || fDist(g_humanoRng) < 0.50f) {
            const Humano* prospectiveMate = nullptr;
            float closestDistanceSquared = 576.0f; // Hasta 24 celdas de radio

            for (const auto& potentialPartner : *poblacionTotal) {
                if (&potentialPartner == this || !potentialPartner.estaVivo() || potentialPartner.esNino() || potentialPartner.tienePareja()) continue;
                if (potentialPartner.getReligionId() != m_religionId || potentialPartner.esHombre() == esHombre()) continue;

                const float deltaCoordinateX = potentialPartner.getX() - m_x;
                const float deltaCoordinateY = potentialPartner.getY() - m_y;
                const float candidateDistanceSquared = deltaCoordinateX * deltaCoordinateX + deltaCoordinateY * deltaCoordinateY;

                if (candidateDistanceSquared < closestDistanceSquared) {
                    closestDistanceSquared = candidateDistanceSquared;
                    prospectiveMate = &potentialPartner;
                }
            }

            if (prospectiveMate) {
                const float prospectiveDistance = std::sqrt(closestDistanceSquared);
                if (prospectiveDistance <= 1.8f) {
                    courtCandidate(*const_cast<Humano*>(prospectiveMate), 1.0f);
                    m_stateTimer = 2.5f;
                    return;
                } else {
                    caminarHacia(prospectiveMate->getX() + (fDist(g_humanoRng) - 0.5f) * 0.5f,
                                 prospectiveMate->getY() + (fDist(g_humanoRng) - 0.5f) * 0.5f, false);
                    m_humanUnit.currentState = HumanAIState::SOCIALIZING;
                    m_stateTimer = 3.5f;
                    return;
                }
            }
        }
    }

    // =========================================================================
    // EN: 0H. Mated pair bonding, nesting shelter drive, and procreation seeking
    // ES: 0H. Vínculo de pareja, anidación y búsqueda de intimidad conyugal
    // =========================================================================
    if (!m_isChild && tienePareja() && poblacionTotal) {
        // EN: Unsheltered couples experience urgent nesting urge to build a home
        // ES: Parejas sin hogar experimentan urgente impulso de anidación para construir
        if (!tieneHogar()) {
            modifyNeedForShelter(0.12f);
        }

        if (m_reproductionDrive >= 0.28f || fDist(g_humanoRng) < 0.40f) {
            const Humano* spousePtr = nullptr;
            for (const auto& candidateSpouse : *poblacionTotal) {
                if (candidateSpouse.getId() == m_spouseId && candidateSpouse.estaVivo()) {
                    spousePtr = &candidateSpouse;
                    break;
                }
            }

            if (spousePtr) {
                const float spouseDistance = std::hypot(m_x - spousePtr->getX(), m_y - spousePtr->getY());
                if (spouseDistance <= 1.8f) {
                    // EN: Close intimate communion between spouses
                    // ES: Comunión íntima cercana entre cónyuges
                    m_action = HumanAction::SALUDAR;
                    modifyAffection(0.06f);
                    const_cast<Humano*>(spousePtr)->modifyAffection(0.06f);
                    modifyReproductionDrive(-0.20f);
                    const_cast<Humano*>(spousePtr)->modifyReproductionDrive(-0.20f);
                    m_humanUnit.currentState = HumanAIState::SOCIALIZING;
                    m_stateTimer = 3.0f;
                    return;
                } else if (spouseDistance <= 22.0f) {
                    caminarHacia(spousePtr->getX() + (fDist(g_humanoRng) - 0.5f) * 0.6f,
                                 spousePtr->getY() + (fDist(g_humanoRng) - 0.5f) * 0.6f, false);
                    m_humanUnit.currentState = HumanAIState::SOCIALIZING;
                    m_stateTimer = 3.5f;
                    return;
                }
            }
        }
    }
    // 4. COMPORTAMIENTO DEDICADO DEL LÍDER ESPIRITUAL (CHAMÁN O PROFETA)
    if (esLiderEspiritual()) {
        float roll = fDist(g_humanoRng);
        bool isRivalAI = (m_religionId != 1);

        // Nivel 1 (Pasiva): El líder rival permanece en su santuario orando o en comunión local sin incursiones
        if (isRivalAI && m_aiDifficulty == AIDifficulty::Pasiva) {
            m_action = HumanAction::REZAR;
            m_humanUnit.currentState = HumanAIState::WORSHIPING;
            m_spiritualHunger = 0.0f;
            m_stateTimer = 5.0f;
            return;
        }

        if (celda.hasHolySite) {
            // En Nivel 3 (Estratégica) y Nivel 4 (Implacable): la IA rival busca activamente celdas vulnerables a convertir
            if (isRivalAI && (m_aiDifficulty == AIDifficulty::Estrategica || m_aiDifficulty == AIDifficulty::Implacable)) {
                Point2i targetCell{-1, -1};
                float bestVulnerability = -1.0f;

                for (int dy = -8; dy <= 8; ++dy) {
                    for (int dx = -8; dx <= 8; ++dx) {
                        int nx = curX + dx;
                        int ny = curY + dy;
                        if (!mapa.isValid(nx, ny)) continue;
                        const Cell& c = mapa.getCell(nx, ny);
                        if (!c.isPassableForGround() || c.population <= 0) continue;

                        float vuln = 0.0f;
                        // Si es celda del jugador o frontera en disputa:
                        if (c.dominantReligionId == 1) {
                            vuln = (1.0f - c.dominantFraction) * 50.0f + static_cast<float>(c.population) * 0.5f;
                        } else if (c.dominantReligionId == m_religionId && c.devotion < 0.50f) {
                            // Celo interno: reconsolidar fieles tibios (Anti-cisma táctico)
                            vuln = (1.0f - c.devotion) * 60.0f;
                        } else if (c.isContestedBorder) {
                            vuln = c.borderTension * 40.0f;
                        }

                        if (vuln > bestVulnerability) {
                            bestVulnerability = vuln;
                            targetCell = {nx, ny};
                        }
                    }
                }

                if (targetCell.x != -1 && bestVulnerability > 15.0f && roll < 0.65f) {
                    caminarHacia(static_cast<float>(targetCell.x) + 0.5f, static_cast<float>(targetCell.y) + 0.5f, false);
                    m_humanUnit.currentState = HumanAIState::CONVERTING;
                    m_action = HumanAction::CAMINAR;
                    m_stateTimer = 4.0f;
                    return;
                }
            }

            // Si está en el santuario sagrado:
            if (m_spiritualHunger >= 25.0f || roll < 0.38f) {
                // Comunión y oración en el altar
                m_action = HumanAction::REZAR;
                m_humanUnit.currentState = HumanAIState::WORSHIPING;
                m_spiritualHunger = 0.0f;
                m_stateTimer = 4.0f;
                return;
            } else {
                // Salir del templo para bendecir campos, visitar aldeanos y predicar
                std::vector<Point2i> destinosSagrados;
                for (int dy = -5; dy <= 5; ++dy) {
                    for (int dx = -5; dx <= 5; ++dx) {
                        int nx = curX + dx;
                        int ny = curY + dy;
                        if (!mapa.isValid(nx, ny)) continue;
                        const Cell& c = mapa.getCell(nx, ny);
                        if (c.isPassableForGround() && (c.population > 0 || c.naturalResource == ResourceType::Food || c.dominantReligionId == m_religionId)) {
                            destinosSagrados.push_back({nx, ny});
                        }
                    }
                }
                if (!destinosSagrados.empty()) {
                    std::uniform_int_distribution<size_t> dDist(0, destinosSagrados.size() - 1);
                    Point2i dest = destinosSagrados[dDist(g_humanoRng)];
                    caminarHacia(static_cast<float>(dest.x) + 0.5f, static_cast<float>(dest.y) + 0.5f, false);
                    m_humanUnit.currentState = HumanAIState::CONVERTING;
                    return;
                }
            }
        } else {
            // Si está fuera del santuario:
            if (m_spiritualHunger >= 45.0f || roll < 0.30f) {
                // Peregrinar de regreso hacia el templo
                Point2i altarPos;
                if (buscarSantuarioOAltar(mapa, m_religionId, 16, altarPos)) {
                    float angle = static_cast<float>(m_id % 8) * 0.785398f + 0.392699f;
                    float targetX = static_cast<float>(altarPos.x) + 0.5f + std::cos(angle) * 0.50f;
                    float targetY = static_cast<float>(altarPos.y) + 0.5f + std::sin(angle) * 0.50f;
                    caminarHacia(targetX, targetY, false);
                    m_humanUnit.currentState = HumanAIState::WORSHIPING;
                    return;
                }
            } else if (roll < 0.65f) {
                // Bendecir o predicar en la comunidad
                m_action = HumanAction::SALUDAR;
                m_humanUnit.currentState = HumanAIState::CONVERTING;
                m_stateTimer = 3.0f;
                return;
            } else {
                // Moverse hacia otra celda de la aldea
                auto vecinos = mapa.getNeighbors(curX, curY, false);
                std::vector<Point2i> nbsHab;
                for (auto& nb : vecinos) {
                    if (mapa.isValid(nb.x, nb.y) && mapa.getCell(nb.x, nb.y).isPassableForGround()) {
                        nbsHab.push_back(nb);
                    }
                }
                if (!nbsHab.empty()) {
                    std::uniform_int_distribution<size_t> nDist(0, nbsHab.size() - 1);
                    Point2i dest = nbsHab[nDist(g_humanoRng)];
                    caminarHacia(static_cast<float>(dest.x) + 0.5f, static_cast<float>(dest.y) + 0.5f, false);
                    return;
                }
            }
        }
    }

    // 5. NECESIDADES ESPIRITUALES: HAMBRE DEL ALMA Y CULTO (ALDEANOS)
    bool tieneHambreEspiritual = (m_spiritualHunger >= 40.0f) || (m_devotion < 0.45f && fDist(g_humanoRng) < 0.65f);
    if (tieneHambreEspiritual) {
        m_humanUnit.currentState = HumanAIState::WORSHIPING;
        // Si está en el santuario, orar y saciar el hambre espiritual
        if (celda.hasHolySite) {
            m_action = HumanAction::REZAR;
            m_spiritualHunger = std::max(0.0f, m_spiritualHunger - 28.0f);
            setDevocion(std::min(1.0f, m_devotion + 0.10f));
            m_stateTimer = 4.0f;
            return;
        }

        // Si no está en el santuario, peregrinar hacia él
        Point2i altarPos;
        if (buscarSantuarioOAltar(mapa, m_religionId, 14, altarPos)) {
            float angle = static_cast<float>(m_id % 8) * 0.785398f + 0.392699f;
            float targetX = static_cast<float>(altarPos.x) + 0.5f + std::cos(angle) * 0.50f;
            float targetY = static_cast<float>(altarPos.y) + 0.5f + std::sin(angle) * 0.50f;
            caminarHacia(targetX, targetY, false);
            return;
        } else {
            // Oración rústica espontánea
            m_action = HumanAction::REZAR;
            m_spiritualHunger = std::max(0.0f, m_spiritualHunger - 20.0f);
            m_stateTimer = 3.0f;
            return;
        }
    }

    // 6. RECOLECCIÓN COMUNITARIA DE MATERIALES (Madera, Piedra, Minerales)
    // EN: If inventory reached capacity, return to friendly sanctuary or settlement to deposit cargo
    // ES: Si el inventario alcanzó su capacidad, retornar al santuario o asentamiento aliado a depositar la carga
    if (m_carriedResourceAmount >= m_carryingCapacity) {
        if (celda.hasHolySite || celda.hasHouse || (celda.dominantReligionId == m_religionId && celda.population > 0)) {
            // EN: Successfully deposited carried goods in clan hearth
            // ES: Bienes transportados depositados con éxito en el hogar del clan
            m_carriedResourceAmount = 0;
            m_isDeliveringOffering = false;
            m_popGroup.foodSupply = std::min(100.0f, m_popGroup.foodSupply + 25.0f);
            m_action = HumanAction::SALUDAR;
            m_stateTimer = 2.0f;
            return;
        }

        Point2i deliveryPos{-1, -1};
        if (buscarSantuarioOAltar(mapa, m_religionId, 18, deliveryPos)) {
            caminarHacia(static_cast<float>(deliveryPos.x) + 0.5f, static_cast<float>(deliveryPos.y) + 0.5f, false);
            m_stateTimer = 3.5f;
            return;
        }
    }

    ResourceType targetRes = elegirQueRecolectarSegunEntorno(mapa);
    if (targetRes != ResourceType::None) {
        if (celda.naturalResource == targetRes && celda.resourceAmount > 0) {
            int harvested = const_cast<WorldMap&>(mapa).getCell(curX, curY).harvestResource(2);
            m_carriedResourceAmount = std::min(m_carryingCapacity, m_carriedResourceAmount + harvested);
            m_gatheringResourceType = targetRes;
            m_action = (targetRes == ResourceType::Wood) ? HumanAction::TALAR : HumanAction::RECOLECTAR;
            m_humanUnit.currentState = HumanAIState::SURVIVAL_WORKING;
            m_stateTimer = 3.5f;

            // EN: If harvesting food, consume a portion to replenish biological nutrition, clan food, and vitality
            // ES: Si recolecta alimento, consume una porción para reponer nutrición biológica, reserva del clan y vitalidad
            if (targetRes == ResourceType::Food && harvested > 0) {
                m_humanUnit.nutritionalState = std::min(100.0f, m_humanUnit.nutritionalState + 18.0f);
                m_popGroup.foodSupply = std::min(100.0f, m_popGroup.foodSupply + 18.0f);
                m_health = std::min(100.0f, m_health + 8.0f);
                m_humanUnit.hitPoints = m_health;
            }

            if (m_carriedResourceAmount >= m_carryingCapacity) {
                if (m_devotion >= 0.65f || esFanatico()) {
                    m_isDeliveringOffering = true;
                }
            }
            return;
        }

        // EN: Search for designated resource node; if exhausted locally, seamlessly fallback to any available active deposit
        // ES: Buscar nodo del recurso preferido; si se agotó localmente, migrar hacia cualquier otro depósito activo
        Point2i nodePos{-1, -1};
        if (buscarNodoRecurso(mapa, targetRes, 16, nodePos) ||
            buscarCualquierNodoRecurso(mapa, 18, nodePos, targetRes)) {
            m_gatheringResourceType = targetRes;
            m_humanUnit.currentState = HumanAIState::SURVIVAL_WORKING;
            caminarHacia(static_cast<float>(nodePos.x) + 0.5f, static_cast<float>(nodePos.y) + 0.5f, false);
            m_stateTimer = 3.5f;
            return;
        }
    }

    // 7. VIDA SOCIAL Y COMUNAL (Rutina de la Primera Era)
    float roll = fDist(g_humanoRng);
    auto vecinos = mapa.getNeighbors(curX, curY, false);
    std::vector<Point2i> vecinosHabitables;
    bool cruzaPeligro = puedeCaminarSobreAguaYTerrenoPeligroso();
    for (const auto& nb : vecinos) {
        if (!mapa.isValid(nb.x, nb.y)) continue;
        if (cruzaPeligro || mapa.getCell(nb.x, nb.y).isPassableForGround()) {
            vecinosHabitables.push_back(nb);
        }
    }

    if (m_humanUnit.currentState == HumanAIState::CONVERTING) {
        if (!vecinosHabitables.empty() && roll < 0.65f) {
            std::uniform_int_distribution<size_t> nDist(0, vecinosHabitables.size() - 1);
            Point2i dest = vecinosHabitables[nDist(g_humanoRng)];
            caminarHacia(static_cast<float>(dest.x) + 0.3f + fDist(g_humanoRng) * 0.4f,
                         static_cast<float>(dest.y) + 0.3f + fDist(g_humanoRng) * 0.4f, false);
        } else {
            m_action = (roll < 0.80f) ? HumanAction::SALUDAR : HumanAction::REZAR;
        }
        return;
    }

    // 7B. Vínculo de Pareja y Cortejo Activo
    if (!m_isChild && poblacionTotal) {
        // Si tiene pareja viva: en momentos de ocio comunal buscar acercarse a su cónyuge
        if (tienePareja() && roll < 0.35f) {
            const Humano* spouse = nullptr;
            for (const auto& other : *poblacionTotal) {
                if (other.getId() == m_spouseId && other.estaVivo()) {
                    spouse = &other;
                    break;
                }
            }
            if (spouse) {
                float sDist = std::hypot(m_x - spouse->getX(), m_y - spouse->getY());
                if (sDist > 2.0f && sDist <= 12.0f) {
                    caminarHacia(spouse->getX() + (fDist(g_humanoRng) - 0.5f) * 0.8f,
                                 spouse->getY() + (fDist(g_humanoRng) - 0.5f) * 0.8f, false);
                    m_humanUnit.currentState = HumanAIState::SOCIALIZING;
                    m_stateTimer = 3.5f;
                    return;
                }
            }
        }
        // Si es soltero adulto y de día: buscar activamente cortejar a soltero del sexo opuesto
        else if (!tienePareja() && !esDeNoche && m_sleepNeed < 60.0f && roll < 0.30f) {
            const Humano* pretendiente = nullptr;
            float minDistSq = 100.0f;
            for (const auto& other : *poblacionTotal) {
                if (&other == this || !other.estaVivo() || other.esNino() || other.tienePareja()) continue;
                if (other.getReligionId() != m_religionId || other.esHombre() == esHombre()) continue;
                float ddx = other.getX() - m_x;
                float ddy = other.getY() - m_y;
                float dsq = ddx * ddx + ddy * ddy;
                if (dsq < minDistSq && dsq > 1.0f) {
                    minDistSq = dsq;
                    pretendiente = &other;
                }
            }
            if (pretendiente) {
                caminarHacia(pretendiente->getX() + (fDist(g_humanoRng) - 0.5f) * 0.6f,
                             pretendiente->getY() + (fDist(g_humanoRng) - 0.5f) * 0.6f, false);
                m_humanUnit.currentState = HumanAIState::SOCIALIZING;
                m_stateTimer = 3.5f;
                return;
            }
        }
    }

    if (roll < 0.25f) {
        m_action = HumanAction::SALUDAR;
    } else if (roll < 0.85f && !vecinosHabitables.empty()) {
        std::uniform_int_distribution<size_t> nDist(0, vecinosHabitables.size() - 1);
        Point2i dest = vecinosHabitables[nDist(g_humanoRng)];
        caminarHacia(static_cast<float>(dest.x) + 0.3f + fDist(g_humanoRng) * 0.4f,
                     static_cast<float>(dest.y) + 0.3f + fDist(g_humanoRng) * 0.4f, false);
    } else {
        m_action = HumanAction::IDLE;
    }
}



void Humano::render(uint32_t* dstBuffer, int bufferWidth, int bufferHeight,
                    const IsometricCamera& camara, const WorldMap& mapa,
                    const TileSet& tileset, ColorRGB auraColor, bool seleccionado) const {
    const int dstW = bufferWidth;
    const int dstH = bufferHeight;
    // Si el aldeano está durmiendo protegido dentro de su hogar, su sprite desaparece (está cobijado bajo techo)
    if (estaDurmiendoBajoTecho()) {
        return;
    }

    float elev = 1.0f;
    if (mapa.isValid(m_cellX, m_cellY)) {
        elev = mapa.getCell(m_cellX, m_cellY).elevation;
    }

    Vec2 pieSuelo = camara.gridToScreen(m_x, m_y, elev);
    Vec2 piePantalla = pieSuelo;

    // Escala del humano/chamán/mujer/niño acorde al zoom de la cámara isométrica
    float factorBase = m_isChild ? 0.32f : (m_isShaman ? 0.54f : (m_gender == HumanGender::MUJER ? 0.46f : 0.48f));
    float escala = std::clamp(camara.getZoom() * factorBase, 0.15f, 1.35f);

    // Sombra proyectada en el suelo cuando el aldeano está volando por tornado/física aérea
    if (m_altitude > 0.0f) {
        piePantalla.y -= m_altitude * camara.getZoom();

        int shadowRx = std::max(2, static_cast<int>(12.0f * escala * std::max(0.20f, 1.0f - (m_altitude / 90.0f))));
        int shadowRy = std::max(1, static_cast<int>(6.0f * escala * std::max(0.20f, 1.0f - (m_altitude / 90.0f))));
        int scx = static_cast<int>(pieSuelo.x);
        int scy = static_cast<int>(pieSuelo.y);
        for (int dy = -shadowRy; dy <= shadowRy; ++dy) {
            int py = scy + dy;
            int fy = dstH - 1 - py;
            if (fy < 0 || fy >= dstH) continue;
            for (int dx = -shadowRx; dx <= shadowRx; ++dx) {
                int px = scx + dx;
                if (px < 0 || px >= dstW) continue;
                float dNorm = (static_cast<float>(dx * dx) / (shadowRx * shadowRx)) + (static_cast<float>(dy * dy) / (shadowRy * shadowRy));
                if (dNorm <= 1.0f) {
                    uint32_t bg = dstBuffer[fy * dstW + px];
                    uint8_t br = static_cast<uint8_t>((bg & 0xFF) * 0.68f);
                    uint8_t bgc = static_cast<uint8_t>(((bg >> 8) & 0xFF) * 0.68f);
                    uint8_t bb = static_cast<uint8_t>(((bg >> 16) & 0xFF) * 0.68f);
                    dstBuffer[fy * dstW + px] = (0xFFu << 24) | (bb << 16) | (bgc << 8) | br;
                }
            }
        }
    }

    // Comprobación de frustum
    if (piePantalla.x < -60 || piePantalla.x > dstW + 60 ||
        piePantalla.y < -60 || piePantalla.y > dstH + 60) {
        return;
    }

    TileRect rect = getTileRect(tileset);

    // Halo milagroso bajo los pies de profetas al caminar sobre aguas o terreno peligroso
    if (esProfeta() && mapa.isValid(m_cellX, m_cellY) && !mapa.getCell(m_cellX, m_cellY).isPassableForGround()) {
        int cx = static_cast<int>(piePantalla.x);
        int cy = static_cast<int>(piePantalla.y);
        int rx = static_cast<int>(14.0f * escala);
        int ry = static_cast<int>(7.0f * escala);
        if (rx >= 2 && ry >= 1) {
            for (int dy = -ry; dy <= ry; ++dy) {
                int py = cy + dy;
                int fy = dstH - 1 - py;
                if (fy < 0 || fy >= dstH) continue;
                for (int dx = -rx; dx <= rx; ++dx) {
                    int px = cx + dx;
                    if (px < 0 || px >= dstW) continue;
                    float dNorm = (static_cast<float>(dx * dx) / (rx * rx)) + (static_cast<float>(dy * dy) / (ry * ry));
                    if (dNorm >= 0.65f && dNorm <= 1.05f) {
                        uint32_t bg = dstBuffer[fy * dstW + px];
                        uint8_t br = bg & 0xFF;
                        uint8_t bgc = (bg >> 8) & 0xFF;
                        uint8_t bb = (bg >> 16) & 0xFF;
                        ColorRGB haloCol{255, 230, 110}; // Oro celestial de milagro
                        uint8_t outR = static_cast<uint8_t>(br * 0.35f + haloCol.r * 0.65f);
                        uint8_t outG = static_cast<uint8_t>(bgc * 0.35f + haloCol.g * 0.65f);
                        uint8_t outB = static_cast<uint8_t>(bb * 0.35f + haloCol.b * 0.65f);
                        dstBuffer[fy * dstW + px] = (0xFFu << 24) | (outB << 16) | (outG << 8) | outR;
                    }
                }
            }
        }
    }

    // Ondas acuáticas y estela si el aldeano está nadando para salvar su vida
    if (m_isSwimming) {
        int wx = static_cast<int>(piePantalla.x);
        int wy = static_cast<int>(piePantalla.y);
        int waveR = static_cast<int>(11.0f * escala);
        ColorRGB swimCol{90, 190, 245};
        for (int dy = -waveR / 2; dy <= waveR / 2; ++dy) {
            int py = wy + dy;
            int fy = dstH - 1 - py;
            if (fy < 0 || fy >= dstH) continue;
            for (int dx = -waveR; dx <= waveR; ++dx) {
                int px = wx + dx;
                if (px < 0 || px >= dstW) continue;
                float dNorm = (static_cast<float>(dx * dx) / (waveR * waveR)) +
                              (static_cast<float>(dy * dy * 4) / (waveR * waveR));
                if (dNorm >= 0.70f && dNorm <= 1.0f) {
                    uint32_t bg = dstBuffer[fy * dstW + px];
                    uint8_t br = bg & 0xFF;
                    uint8_t bgc = (bg >> 8) & 0xFF;
                    uint8_t bb = (bg >> 16) & 0xFF;
                    uint8_t outR = static_cast<uint8_t>(br * 0.45f + swimCol.r * 0.55f);
                    uint8_t outG = static_cast<uint8_t>(bgc * 0.45f + swimCol.g * 0.55f);
                    uint8_t outB = static_cast<uint8_t>(bb * 0.45f + swimCol.b * 0.55f);
                    dstBuffer[fy * dstW + px] = (0xFFu << 24) | (outB << 16) | (outG << 8) | outR;
                }
            }
        }
    }

    // Si la unidad ha muerto (HP <= 0) o está en estado de físicas Ragdoll
    if (m_health <= 0.0f || m_ragdoll.isActive()) {
        float alpha = estaMuerto() ? std::clamp(m_deathTimer / 0.60f, 0.0f, 1.0f) : 1.0f;
        // Asegurar que todo cadáver renderice en posición acostada en el suelo
        if (estaMuerto() && !m_ragdoll.isActive()) {
            const_cast<Humano*>(this)->activarRagdoll(0.0f, 0.0f, 0.0f, 0.0f);
        }
        m_ragdoll.render(dstBuffer, dstW, dstH, camara, mapa, tileset, rect,
                         m_x, m_y, m_altitude, escala, auraColor, alpha, esProfeta());

        // Si la unidad ha muerto (HP <= 0), mostrar icono flotante de calavera ancestral sobre el cuerpo y finalizar
        if (m_health <= 0.0f) {
            constexpr float DEATH_ANIM_DURATION = 2.0f;
            float t = 1.0f - std::clamp(m_deathTimer / DEATH_ANIM_DURATION, 0.0f, 1.0f);
            float floatOffset = t * 16.0f * std::max(0.70f, camara.getZoom());
            int cx = static_cast<int>(piePantalla.x);
            int cy = static_cast<int>(piePantalla.y - m_altitude * camara.getZoom() - 10.0f * escala - floatOffset);
            float skullAlpha = std::clamp(m_deathTimer / 0.60f, 0.0f, 1.0f);

            // Burbujas si murió ahogado
            if (m_isDrowning) {
                int wx = static_cast<int>(piePantalla.x);
                int wy = static_cast<int>(piePantalla.y);
                ColorRGB bubbleCol{180, 230, 255};
                for (int b = 0; b < 3; ++b) {
                    int bx = wx + (b - 1) * 5;
                    int by = wy - static_cast<int>(t * (16.0f + b * 7.0f) * escala);
                    int fy = dstH - 1 - by;
                    if (fy >= 0 && fy < dstH && bx >= 0 && bx < dstW) {
                        dstBuffer[fy * dstW + bx] = (0xFFu << 24) | (bubbleCol.b << 16) | (bubbleCol.g << 8) | bubbleCol.r;
                    }
                }
            }

            // Calavera en color hueso con cuencas oculares (azules si es ahogado, rojas de sangre por tierra)
            ColorRGB boneCol{226, 218, 196};
            ColorRGB bloodCol = m_isDrowning ? ColorRGB{60, 180, 240} : ColorRGB{140, 20, 30};
            ColorRGB outlCol{14, 9, 8};

            for (int dy = -3; dy <= 3; ++dy) {
                int py = cy + dy;
                int fy = dstH - 1 - py;
                if (fy < 0 || fy >= dstH) continue;
                for (int dx = -3; dx <= 3; ++dx) {
                    int px = cx + dx;
                    if (px < 0 || px >= dstW) continue;

                    bool isEye = (dy == -1 && (dx == -1 || dx == 1));
                    bool isSkull = (dy == -3 && std::abs(dx) <= 1) ||
                                   (dy == -2 && std::abs(dx) <= 2) ||
                                   (dy == -1 && (std::abs(dx) <= 2 && !isEye)) ||
                                   (dy == 0 && std::abs(dx) <= 1) ||
                                   (dy == 1 && (dx == -1 || dx == 1));
                    bool isOutline = !isSkull && !isEye &&
                                    ((std::abs(dx) <= 2 && std::abs(dy) <= 3) ||
                                     (std::abs(dx) <= 3 && std::abs(dy) <= 2));

                    if (isEye || isSkull || isOutline) {
                        ColorRGB col = isEye ? bloodCol : (isSkull ? boneCol : outlCol);
                        uint32_t bg = dstBuffer[fy * dstW + px];
                        uint8_t br = bg & 0xFF;
                        uint8_t bgc = (bg >> 8) & 0xFF;
                        uint8_t bb = (bg >> 16) & 0xFF;

                        uint8_t outR = static_cast<uint8_t>(br * (1.0f - skullAlpha) + col.r * skullAlpha);
                        uint8_t outG = static_cast<uint8_t>(bgc * (1.0f - skullAlpha) + col.g * skullAlpha);
                        uint8_t outB = static_cast<uint8_t>(bb * (1.0f - skullAlpha) + col.b * skullAlpha);
                        dstBuffer[fy * dstW + px] = (0xFFu << 24) | (outB << 16) | (outG << 8) | outR;
                    }
                }
            }
            return;
        }

        // Si la unidad está viva pero en ragdoll (volando o cayendo), mostrar barra de salud si está herida o seleccionada
        if (m_health < 99.5f || seleccionado) {
            float destH = rect.height * escala;
            int barW = std::clamp(static_cast<int>(20.0f * std::max(0.70f, camara.getZoom())), 14, 26);
            int barH = 3;
            int barX = static_cast<int>(piePantalla.x - barW * 0.5f);
            int barY = static_cast<int>(piePantalla.y - m_altitude * camara.getZoom() - destH - 4.0f * escala);

            for (int by = barY - 1; by <= barY + barH; ++by) {
                int fy = dstH - 1 - by;
                if (fy < 0 || fy >= dstH) continue;
                for (int bx = barX - 1; bx <= barX + barW; ++bx) {
                    if (bx < 0 || bx >= dstW) continue;
                    bool esBorde = (by == barY - 1 || by == barY + barH || bx == barX - 1 || bx == barX + barW);
                    if (esBorde) {
                        dstBuffer[fy * dstW + bx] = (0xFFu << 24) | (8u << 16) | (8u << 8) | 12u;
                    } else {
                        dstBuffer[fy * dstW + bx] = (0xFFu << 24) | (14u << 16) | (14u << 8) | 30u;
                    }
                }
            }

            float frac = std::clamp(m_health / 100.0f, 0.0f, 1.0f);
            int fillW = static_cast<int>(barW * frac);
            ColorRGB hpCol = frac > 0.5f ? ColorRGB{60, 220, 80} : (frac > 0.25f ? ColorRGB{240, 180, 40} : ColorRGB{230, 40, 40});

            for (int by = barY; by < barY + barH; ++by) {
                int fy = dstH - 1 - by;
                if (fy < 0 || fy >= dstH) continue;
                for (int bx = barX; bx < barX + fillW; ++bx) {
                    if (bx < 0 || bx >= dstW) continue;
                    dstBuffer[fy * dstW + bx] = (0xFFu << 24) | (hpCol.b << 16) | (hpCol.g << 8) | hpCol.r;
                }
            }
        }
        return;
    }

    tileset.blitTile(dstBuffer, dstW, dstH, rect, piePantalla.x, piePantalla.y, escala, auraColor, seleccionado);

    float destH = rect.height * escala;
    float headX = piePantalla.x;
    float headY = piePantalla.y - destH;

    // 0. Si la unidad ha muerto (HP <= 0), mostrar icono flotante de calavera ancestral y finalizar
    if (m_health <= 0.0f) {
        constexpr float DEATH_ANIM_DURATION = 2.0f;
        float t = 1.0f - std::clamp(m_deathTimer / DEATH_ANIM_DURATION, 0.0f, 1.0f);
        float floatOffset = t * 16.0f * std::max(0.70f, camara.getZoom());
        int cx = static_cast<int>(headX);
        int cy = static_cast<int>(headY - 6.0f * escala - floatOffset);
        float alpha = std::clamp(m_deathTimer / 0.40f, 0.0f, 1.0f);

        // Ondas acuáticas y burbujas si la causa de defunción fue ahogamiento
        if (m_isDrowning) {
            int wx = static_cast<int>(piePantalla.x);
            int wy = static_cast<int>(piePantalla.y);
            int waveR = static_cast<int>((9.0f + t * 15.0f) * escala);
            ColorRGB waterWaveCol{60, 160, 240};
            ColorRGB bubbleCol{180, 230, 255};

            for (int dy = -waveR / 2; dy <= waveR / 2; ++dy) {
                int py = wy + dy;
                int fy = dstH - 1 - py;
                if (fy < 0 || fy >= dstH) continue;
                for (int dx = -waveR; dx <= waveR; ++dx) {
                    int px = wx + dx;
                    if (px < 0 || px >= dstW) continue;
                    float dNorm = (static_cast<float>(dx * dx) / (waveR * waveR)) +
                                  (static_cast<float>(dy * dy * 4) / (waveR * waveR));
                    if (dNorm >= 0.70f && dNorm <= 1.05f) {
                        uint32_t bg = dstBuffer[fy * dstW + px];
                        uint8_t br = bg & 0xFF;
                        uint8_t bgc = (bg >> 8) & 0xFF;
                        uint8_t bb = (bg >> 16) & 0xFF;
                        uint8_t outR = static_cast<uint8_t>(br * 0.4f + waterWaveCol.r * 0.6f);
                        uint8_t outG = static_cast<uint8_t>(bgc * 0.4f + waterWaveCol.g * 0.6f);
                        uint8_t outB = static_cast<uint8_t>(bb * 0.4f + waterWaveCol.b * 0.6f);
                        dstBuffer[fy * dstW + px] = (0xFFu << 24) | (outB << 16) | (outG << 8) | outR;
                    }
                }
            }

            // Burbujas ascendentes de asfixia
            for (int b = 0; b < 3; ++b) {
                int bx = wx + (b - 1) * 5;
                int by = wy - static_cast<int>(t * (16.0f + b * 7.0f) * escala);
                int fy = dstH - 1 - by;
                if (fy >= 0 && fy < dstH && bx >= 0 && bx < dstW) {
                    dstBuffer[fy * dstW + bx] = (0xFFu << 24) | (bubbleCol.b << 16) | (bubbleCol.g << 8) | bubbleCol.r;
                }
            }
        }

        // Calavera en color hueso con cuencas oculares (azules si es ahogado, rojas de sangre por tierra)
        ColorRGB boneCol{226, 218, 196};
        ColorRGB bloodCol = m_isDrowning ? ColorRGB{60, 180, 240} : ColorRGB{140, 20, 30};
        ColorRGB outlCol{14, 9, 8};

        for (int dy = -3; dy <= 3; ++dy) {
            int py = cy + dy;
            int fy = dstH - 1 - py;
            if (fy < 0 || fy >= dstH) continue;
            for (int dx = -3; dx <= 3; ++dx) {
                int px = cx + dx;
                if (px < 0 || px >= dstW) continue;

                bool isEye = (dy == -1 && (dx == -1 || dx == 1));
                bool isSkull = (dy == -3 && std::abs(dx) <= 1) ||
                               (dy == -2 && std::abs(dx) <= 2) ||
                               (dy == -1 && (std::abs(dx) <= 2 && !isEye)) ||
                               (dy == 0 && std::abs(dx) <= 1) ||
                               (dy == 1 && (dx == -1 || dx == 1));
                bool isOutline = !isSkull && !isEye &&
                                ((std::abs(dx) <= 2 && std::abs(dy) <= 3) ||
                                 (std::abs(dx) <= 3 && std::abs(dy) <= 2));

                if (isEye || isSkull || isOutline) {
                    ColorRGB col = isEye ? bloodCol : (isSkull ? boneCol : outlCol);
                    uint32_t bg = dstBuffer[fy * dstW + px];
                    uint8_t br = bg & 0xFF;
                    uint8_t bgc = (bg >> 8) & 0xFF;
                    uint8_t bb = (bg >> 16) & 0xFF;

                    uint8_t outR = static_cast<uint8_t>(br * (1.0f - alpha) + col.r * alpha);
                    uint8_t outG = static_cast<uint8_t>(bgc * (1.0f - alpha) + col.g * alpha);
                    uint8_t outB = static_cast<uint8_t>(bb * (1.0f - alpha) + col.b * alpha);
                    dstBuffer[fy * dstW + px] = (0xFFu << 24) | (outB << 16) | (outG << 8) | outR;
                }
            }
        }
        return; // La unidad caída no dibuja barra de HP ni indicador de fe
    }

    // 1. Barra de Vida (HP) y destello de daño cuando la unidad está herida, atacada o seleccionada
    bool mostrarBarraHP = (m_health < 99.5f || m_damageTimer > 0.0f || seleccionado);
    if (mostrarBarraHP) {
        int barW = std::clamp(static_cast<int>(20.0f * std::max(0.70f, camara.getZoom())), 14, 26);
        int barH = 3;
        int barX = static_cast<int>(headX - barW * 0.5f);
        int barY = static_cast<int>(headY - 4.0f * escala);

        // Borde oscuro exterior
        for (int by = barY - 1; by <= barY + barH; ++by) {
            int fy = dstH - 1 - by;
            if (fy < 0 || fy >= dstH) continue;
            for (int bx = barX - 1; bx <= barX + barW; ++bx) {
                if (bx < 0 || bx >= dstW) continue;
                bool esBorde = (by == barY - 1 || by == barY + barH || bx == barX - 1 || bx == barX + barW);
                if (esBorde) {
                    dstBuffer[fy * dstW + bx] = (0xFFu << 24) | (8u << 16) | (8u << 8) | 12u; // ColorRGB{12, 8, 8}
                } else {
                    dstBuffer[fy * dstW + bx] = (0xFFu << 24) | (14u << 16) | (14u << 8) | 30u; // Fondo oscuro #1e0e0e
                }
            }
        }

        // Relleno proporcional de vida
        float hpFrac = std::clamp(m_health / 100.0f, 0.0f, 1.0f);
        int fillW = static_cast<int>(std::round(barW * hpFrac));
        ColorRGB colHP{46, 215, 80}; // Verde saludable
        if (m_damageTimer > 0.0f && (static_cast<int>(m_damageTimer * 18.0f) & 1)) {
            colHP = ColorRGB{255, 150, 150}; // Destello de impacto
        } else if (hpFrac <= 0.28f) {
            colHP = ColorRGB{230, 45, 45};  // Rojo crítico
        } else if (hpFrac <= 0.55f) {
            colHP = ColorRGB{235, 185, 45}; // Amarillo advertencia
        }
        uint32_t fillRGBA = colHP.toRGBA();

        for (int by = barY; by < barY + barH; ++by) {
            int fy = dstH - 1 - by;
            if (fy < 0 || fy >= dstH) continue;
            for (int bx = barX; bx < barX + fillW; ++bx) {
                if (bx >= 0 && bx < dstW) {
                    dstBuffer[fy * dstW + bx] = fillRGBA;
                }
            }
        }
    }

    // EN: Enveloping authentic pixel-art flame sprite and smoke trail when the unit is actively burning
    // ES: Sprite de llama pixel-art auténtico envolvente y estela de humo cuando la unidad está ardiendo activamente
    if (m_isBurning && m_health > 0.0f) {
        const float flameAnim = m_animTimer * 12.0f;
        static std::vector<uint32_t> s_smallFlamePixels;
        static bool s_smallFlameLoaded = false;
        if (!s_smallFlameLoaded) {
            s_smallFlameLoaded = true;
            const std::vector<std::string> flamePaths = {
                "assets/sprites/fire/small_flame_sheet.rgba",
                "../assets/sprites/fire/small_flame_sheet.rgba",
                "religion_simulator_isometric/assets/sprites/fire/small_flame_sheet.rgba",
                "/home/Ivancho/Documentos/religion_simulator_isometric/assets/sprites/fire/small_flame_sheet.rgba"
            };
            for (const auto& path : flamePaths) {
                std::ifstream file(path, std::ios::binary);
                if (file.is_open()) {
                    file.seekg(0, std::ios::end);
                    const size_t fileSize = static_cast<size_t>(file.tellg());
                    file.seekg(0, std::ios::beg);
                    if (fileSize == 448u * 96u * 4u) {
                        std::vector<uint8_t> rawData(fileSize);
                        file.read(reinterpret_cast<char*>(rawData.data()), fileSize);
                        s_smallFlamePixels.resize(448u * 96u);
                        for (size_t i = 0; i < 448u * 96u; ++i) {
                            const uint8_t r = rawData[i * 4 + 0];
                            const uint8_t g = rawData[i * 4 + 1];
                            const uint8_t b = rawData[i * 4 + 2];
                            const uint8_t a = rawData[i * 4 + 3];
                            s_smallFlamePixels[i] = (static_cast<uint32_t>(a) << 24) |
                                                    (static_cast<uint32_t>(b) << 16) |
                                                    (static_cast<uint32_t>(g) << 8)  |
                                                     static_cast<uint32_t>(r);
                        }
                        break;
                    }
                }
            }
        }

        if (!s_smallFlamePixels.empty()) {
            constexpr int frameW = 64;
            constexpr int frameH = 96;
            constexpr int sheetW = 448;
            constexpr int frameCount = 7;
            const int frameIndex = (static_cast<int>(m_animTimer * 14.0f + m_id * 3) % frameCount);
            const int frameStartX = frameIndex * frameW;

            const int destWidth = std::max(4, static_cast<int>(20.0f * escala));
            const int destHeight = std::max(6, static_cast<int>(30.0f * escala));
            const int startScreenX = static_cast<int>(piePantalla.x) - destWidth / 2;
            const int startScreenY = static_cast<int>(piePantalla.y) - static_cast<int>(20.0f * escala);

            for (int dy = 0; dy < destHeight; ++dy) {
                const int targetY = startScreenY + dy;
                const int frameY = dstH - 1 - targetY;
                if (frameY < 0 || frameY >= dstH) continue;

                const int srcY = std::clamp((dy * frameH) / destHeight, 0, frameH - 1);

                for (int dx = 0; dx < destWidth; ++dx) {
                    const int targetX = startScreenX + dx;
                    if (targetX < 0 || targetX >= dstW) continue;

                    const int srcX = std::clamp(frameStartX + (dx * frameW) / destWidth, 0, sheetW - 1);
                    const uint32_t srcPixel = s_smallFlamePixels[srcY * sheetW + srcX];
                    const uint8_t srcA = (srcPixel >> 24) & 0xFF;
                    if (srcA < 15) continue;

                    const uint8_t srcR = srcPixel & 0xFF;
                    const uint8_t srcG = (srcPixel >> 8) & 0xFF;
                    const uint8_t srcB = (srcPixel >> 16) & 0xFF;

                    const float alphaFactor = (srcA / 255.0f);
                    const uint32_t bgPixel = dstBuffer[frameY * dstW + targetX];
                    const uint8_t bgR = bgPixel & 0xFF;
                    const uint8_t bgG = (bgPixel >> 8) & 0xFF;
                    const uint8_t bgB = (bgPixel >> 16) & 0xFF;

                    const uint8_t outR = static_cast<uint8_t>(srcR * alphaFactor + bgR * (1.0f - alphaFactor));
                    const uint8_t outG = static_cast<uint8_t>(srcG * alphaFactor + bgG * (1.0f - alphaFactor));
                    const uint8_t outB = static_cast<uint8_t>(srcB * alphaFactor + bgB * (1.0f - alphaFactor));

                    dstBuffer[frameY * dstW + targetX] = (0xFFu << 24) | (outB << 16) | (outG << 8) | outR;
                }
            }
        } else {
            const int fireBaseX = static_cast<int>(piePantalla.x);
            const int fireBaseY = static_cast<int>(piePantalla.y - 12.0f * escala);

            for (int flameIndex = 0; flameIndex < 6; ++flameIndex) {
                const float flamePhase = flameIndex * 1.047f;
                const int offsetX = static_cast<int>(std::sin(flameAnim + flamePhase) * (6.0f * escala));
                const int offsetY = static_cast<int>(((flameIndex * 4) % 18) * escala);
                const int sparkX = fireBaseX + offsetX;
                const int sparkY = fireBaseY + offsetY;
                const int frameY = dstH - 1 - sparkY;

                if (sparkX >= 0 && sparkX < dstW && frameY >= 0 && frameY < dstH) {
                    const ColorRGB flameColor = (flameIndex % 3 == 0) ? ColorRGB{255, 235, 90}
                                              : ((flameIndex % 3 == 1) ? ColorRGB{255, 120, 20}
                                                                       : ColorRGB{220, 40, 20});
                    dstBuffer[frameY * dstW + sparkX] = flameColor.toRGBA();
                    if (sparkX + 1 < dstW) {
                        dstBuffer[frameY * dstW + (sparkX + 1)] = flameColor.toRGBA();
                    }
                }
            }
        }

        // EN: Draw rising smoke puffs billowing above head
        // ES: Dibujar bocanadas de humo ascendente ondulando sobre la cabeza
        const int smokeBaseX = static_cast<int>(headX);
        const int smokeBaseY = static_cast<int>(headY - 6.0f * escala);
        for (int smokeStep = 0; smokeStep < 3; ++smokeStep) {
            const float smokeShift = std::sin(flameAnim * 0.5f + smokeStep * 1.8f) * (3.0f * escala);
            const int smokeX = static_cast<int>(smokeBaseX + smokeShift);
            const int smokeY = static_cast<int>(smokeBaseY + (smokeStep * 6.0f) * escala);
            const int frameY = dstH - 1 - smokeY;
            if (smokeX >= 0 && smokeX < dstW && frameY >= 0 && frameY < dstH) {
                const uint32_t currentBg = dstBuffer[frameY * dstW + smokeX];
                const uint8_t bgR = currentBg & 0xFF;
                const uint8_t bgG = (currentBg >> 8) & 0xFF;
                const uint8_t bgB = (currentBg >> 16) & 0xFF;
                constexpr ColorRGB smokeColor{55, 50, 48};
                constexpr float smokeOpacity = 0.45f;
                const uint8_t blendR = static_cast<uint8_t>(bgR * (1.0f - smokeOpacity) + smokeColor.r * smokeOpacity);
                const uint8_t blendG = static_cast<uint8_t>(bgG * (1.0f - smokeOpacity) + smokeColor.g * smokeOpacity);
                const uint8_t blendB = static_cast<uint8_t>(bgB * (1.0f - smokeOpacity) + smokeColor.b * smokeOpacity);
                dstBuffer[frameY * dstW + smokeX] = (0xFFu << 24) | (blendB << 16) | (blendG << 8) | blendR;
            }
        }
    }

    // 2. Chispas de impacto cuando la unidad es atacada
    if (m_damageTimer > 0.0f) {
        int sparkBaseX = static_cast<int>(headX);
        int sparkBaseY = static_cast<int>(piePantalla.y - 28.0f * escala);
        int sparkOffsets[4][2] = {{-3, -2}, {2, 3}, {-4, 1}, {3, -3}};
        for (int i = 0; i < 4; ++i) {
            int sx = sparkBaseX + sparkOffsets[i][0];
            int sy = sparkBaseY + sparkOffsets[i][1];
            int fy = dstH - 1 - sy;
            if (sx >= 0 && sx < dstW && fy >= 0 && fy < dstH) {
                ColorRGB sCol = (i & 1) ? ColorRGB{255, 230, 120} : ColorRGB{245, 60, 40};
                dstBuffer[fy * dstW + sx] = sCol.toRGBA();
            }
        }
    }

    // 3. Indicador flotante "+" de cambio de Fe (Verde cuando aumenta, Rojo cuando disminuye)
    if (m_faithIndicatorTimer > 0.0f && m_faithIndicatorSign != 0) {
        float durTotal = 1.35f;
        float t = 1.0f - std::clamp(m_faithIndicatorTimer / durTotal, 0.0f, 1.0f);
        float floatOffset = t * 16.0f * std::max(0.70f, camara.getZoom());
        int cx = static_cast<int>(headX);
        int cy = static_cast<int>(headY - (mostrarBarraHP ? 10.0f : 5.0f) * escala - floatOffset);
        float alpha = std::clamp(m_faithIndicatorTimer / 0.35f, 0.0f, 1.0f);

        // '+' verde intenso cuando aumenta la fe, '+' rojo intenso cuando se pierde
        ColorRGB coreCol = (m_faithIndicatorSign > 0) ? ColorRGB{46, 235, 96} : ColorRGB{235, 48, 48};
        ColorRGB outlCol{14, 9, 8};

        // Dibujar cruz de 5px con contorno oscuro de 1px
        for (int dy = -3; dy <= 3; ++dy) {
            int py = cy + dy;
            int fy = dstH - 1 - py;
            if (fy < 0 || fy >= dstH) continue;

            for (int dx = -3; dx <= 3; ++dx) {
                int px = cx + dx;
                if (px < 0 || px >= dstW) continue;

                // Núcleo del símbolo '+': cruz de 5px vertical y 5px horizontal
                bool isCore = (dx == 0 && std::abs(dy) <= 2) || (dy == 0 && std::abs(dx) <= 2);
                // Borde/sombra exterior de 1px que rodea a la cruz
                bool isOutline = !isCore && ((std::abs(dx) <= 1 && std::abs(dy) <= 3) ||
                                            (std::abs(dy) <= 1 && std::abs(dx) <= 3));

                if (isCore || isOutline) {
                    ColorRGB col = isCore ? coreCol : outlCol;
                    uint32_t bg = dstBuffer[fy * dstW + px];
                    uint8_t br = bg & 0xFF;
                    uint8_t bgc = (bg >> 8) & 0xFF;
                    uint8_t bb = (bg >> 16) & 0xFF;

                    uint8_t outR = static_cast<uint8_t>(br * (1.0f - alpha) + col.r * alpha);
                    uint8_t outG = static_cast<uint8_t>(bgc * (1.0f - alpha) + col.g * alpha);
                    uint8_t outB = static_cast<uint8_t>(bb * (1.0f - alpha) + col.b * alpha);
                    dstBuffer[fy * dstW + px] = (0xFFu << 24) | (outB << 16) | (outG << 8) | outR;
                }
            }
        }
    }

    // 3B. Indicador visual de Sueño / Descanso ("zZ" flotante y ondulante)
    if (m_isSleeping && m_health > 0.0f) {
        float tOsc = std::sin(m_animTimer * 2.5f);
        int zx1 = static_cast<int>(headX + 4.0f * escala + tOsc * 2.0f);
        int zy1 = static_cast<int>(headY - 8.0f * escala - tOsc * 2.5f);
        int zx2 = zx1 + static_cast<int>(6.0f * escala);
        int zy2 = zy1 - static_cast<int>(5.0f * escala);

        ColorRGB zCol{185, 220, 255}; // Celeste etéreo de descanso
        auto drawP = [&](int px, int py, ColorRGB col) {
            int fy = dstH - 1 - py;
            if (px >= 0 && px < dstW && fy >= 0 && fy < dstH) {
                dstBuffer[fy * dstW + px] = col.toRGBA();
            }
        };

        // 'z' pequeña (3x3)
        drawP(zx1, zy1, zCol);     drawP(zx1+1, zy1, zCol);   drawP(zx1+2, zy1, zCol);
                                   drawP(zx1+1, zy1+1, zCol);
        drawP(zx1, zy1+2, zCol);   drawP(zx1+1, zy1+2, zCol); drawP(zx1+2, zy1+2, zCol);

        // 'Z' mediana (4x4)
        drawP(zx2, zy2, zCol);     drawP(zx2+1, zy2, zCol);   drawP(zx2+2, zy2, zCol); drawP(zx2+3, zy2, zCol);
                                                              drawP(zx2+2, zy2+1, zCol);
                                   drawP(zx2+1, zy2+2, zCol);
        drawP(zx2, zy2+3, zCol);   drawP(zx2+1, zy2+3, zCol); drawP(zx2+2, zy2+3, zCol); drawP(zx2+3, zy2+3, zCol);
    }

    // 3C. Indicador visual de Furia Protectora (Llamas de devoción y amor familiar)
    if (tieneFuriaProtectora() && m_health > 0.0f) {
        int fx = static_cast<int>(headX);
        int fy_pos = static_cast<int>(headY - (mostrarBarraHP ? 8.0f : 4.0f) * escala);
        int sparkOffsets[3][2] = {{-2, -1}, {0, -3}, {2, -1}};
        for (int s = 0; s < 3; ++s) {
            int sx = fx + sparkOffsets[s][0];
            int sy = fy_pos + sparkOffsets[s][1] + (static_cast<int>(m_animTimer * 12.0f + s) % 3);
            int fy = dstH - 1 - sy;
            if (sx >= 0 && sx < dstW && fy >= 0 && fy < dstH) {
                ColorRGB fCol = (s == 1) ? ColorRGB{255, 230, 80} : ColorRGB{245, 60, 40};
                dstBuffer[fy * dstW + sx] = fCol.toRGBA();
            }
        }
    }

    // 4. Indicador de Pandemia Viral / Infección (esporas de miasma verde sobre la cabeza)
    if (m_isInfected && m_health > 0.0f) {
        int hx = static_cast<int>(headX);
        int hy = static_cast<int>(headY - (mostrarBarraHP ? 8.0f : 4.0f) * escala);
        ColorRGB pCol{120, 230, 60};
        ColorRGB darkPCol{40, 80, 20};
        for (int dy = -2; dy <= 2; ++dy) {
            int py = hy + dy;
            int fy = dstH - 1 - py;
            if (fy < 0 || fy >= dstH) continue;
            for (int dx = -2; dx <= 2; ++dx) {
                int px = hx + dx;
                if (px < 0 || px >= dstW) continue;
                if ((std::abs(dx) + std::abs(dy)) <= 2) {
                    dstBuffer[fy * dstW + px] = ((dx == 0 && dy == 0) ? pCol : darkPCol).toRGBA();
                }
            }
        }
    }

    // 5. Indicador de Aturdimiento / Tropiezo por Sismo (estrellas giratorias)
    if (m_stumbleTimer > 0.0f && m_health > 0.0f) {
        int hx = static_cast<int>(headX);
        int hy = static_cast<int>(headY - 6.0f * escala);
        float starAngle = m_animTimer * 6.0f;
        int sx1 = hx + static_cast<int>(std::cos(starAngle) * 6.0f);
        int sy1 = hy + static_cast<int>(std::sin(starAngle) * 3.0f);
        int sx2 = hx - static_cast<int>(std::cos(starAngle) * 6.0f);
        int sy2 = hy - static_cast<int>(std::sin(starAngle) * 3.0f);
        ColorRGB sCol{255, 235, 90};
        int f1 = dstH - 1 - sy1;
        int f2 = dstH - 1 - sy2;
        if (sx1 >= 0 && sx1 < dstW && f1 >= 0 && f1 < dstH) dstBuffer[f1 * dstW + sx1] = sCol.toRGBA();
        if (sx2 >= 0 && sx2 < dstW && f2 >= 0 && f2 < dstH) dstBuffer[f2 * dstW + sx2] = sCol.toRGBA();
    }

    // 6. Corona de Fervor / Llama de Fanático (punto álgido de devoción y reverencia al líder)
    if (esFanatico() && m_health > 0.0f) {
        int hx = static_cast<int>(headX);
        int hy = static_cast<int>(headY - (mostrarBarraHP ? 8.0f : 4.0f) * escala);
        float flamePhase = m_animTimer * 12.0f;
        ColorRGB flameGold{255, 215, 40};
        ColorRGB flameRed{255, 75, 20};
        ColorRGB flameCore{255, 255, 180};

        // Corona flamígera de 5 puntas pulsantes sobre la cabeza del fanático
        for (int p = -2; p <= 2; ++p) {
            float pxOffset = static_cast<float>(p) * 2.8f * escala;
            float flameH = (3.5f + std::sin(flamePhase + static_cast<float>(p) * 1.5f) * 1.8f) * escala;
            int fx = static_cast<int>(hx + pxOffset);
            int fyBase = static_cast<int>(hy - 1.0f * escala);
            int fyTop = static_cast<int>(hy - 1.0f * escala - flameH);

            for (int cy = fyTop; cy <= fyBase; ++cy) {
                int frameY = dstH - 1 - cy;
                if (frameY < 0 || frameY >= dstH) continue;
                if (fx < 0 || fx >= dstW) continue;
                float t = static_cast<float>(cy - fyTop) / std::max(1.0f, static_cast<float>(fyBase - fyTop));
                ColorRGB col = (t < 0.35f) ? flameCore : ((t < 0.70f) ? flameGold : flameRed);
                dstBuffer[frameY * dstW + fx] = (0xFFu << 24) | (col.b << 16) | (col.g << 8) | col.r;
            }
        }
    }

    // 7. Visualización de Fardo de Recurso o Cesta de Ofrenda Sagrada a hombros
    if (m_carriedResourceAmount > 0 && m_health > 0.0f) {
        int bx = static_cast<int>(headX + (m_direction == IsoDirection::SW || m_direction == IsoDirection::NW ? -7.0f : 7.0f) * escala);
        int by = static_cast<int>(piePantalla.y - 18.0f * escala);
        int bundleW = std::max(2, static_cast<int>(5.0f * escala));
        int bundleH = std::max(2, static_cast<int>(4.0f * escala));

        ColorRGB resCol{230, 180, 50}; // Comida / trigo por defecto
        if (m_isDeliveringOffering) {
            resCol = Palette::DIVINE_GOLD; // Ofrenda sagrada dorada
        } else {
            switch (m_gatheringResourceType) {
                case ResourceType::Food:    resCol = ColorRGB{235, 175, 45}; break; // Trigo / frutos
                case ResourceType::Wood:    resCol = ColorRGB{125, 75, 40}; break;  // Leña marrón
                case ResourceType::Stone:   resCol = ColorRGB{155, 160, 165}; break;// Piedra gris
                case ResourceType::Gold:    resCol = ColorRGB{255, 215, 0}; break;  // Oro brillante
                case ResourceType::Iron:    resCol = ColorRGB{90, 105, 120}; break; // Hierro metálico
                case ResourceType::Copper:  resCol = ColorRGB{210, 115, 60}; break; // Cobre rojizo/anaranjado
                case ResourceType::Diamond: resCol = ColorRGB{120, 220, 255}; break;// Diamante celeste cristalino
                default:                    resCol = ColorRGB{200, 180, 150}; break;
            }
        }
        ColorRGB outCol{25, 15, 10};

        for (int dy = -bundleH; dy <= bundleH; ++dy) {
            int py = by + dy;
            int fy = dstH - 1 - py;
            if (fy < 0 || fy >= dstH) continue;
            for (int dx = -bundleW; dx <= bundleW; ++dx) {
                int px = bx + dx;
                if (px < 0 || px >= dstW) continue;
                bool isBrd = (std::abs(dx) == bundleW || std::abs(dy) == bundleH);
                ColorRGB col = isBrd ? outCol : resCol;
                if (m_isDeliveringOffering && !isBrd && ((dx + dy + static_cast<int>(m_animTimer * 10.0f)) % 3 == 0)) {
                    col = ColorRGB{255, 255, 220}; // Destello sagrado
                }
                dstBuffer[fy * dstW + px] = col.toRGBA();
            }
        }
    }

    // 8. EN: Visual overhead condition indicator emblems (Drunk, Sick, Hungry, Aggressive, Scared, Inspired, Injured)
    // ES: Emblemas visuales de condición sobre la cabeza (Borracho, Enfermo, Hambriento, Agresivo, Asustado, Inspirado, Herido)
    if (m_health > 0.0f) {
        const VillagerCondition currentCond = getCondition();
        const int emblemBaseX = static_cast<int>(headX);
        const int emblemBaseY = static_cast<int>(headY - (mostrarBarraHP ? 12.0f : 7.0f) * escala);

        if (currentCond == VillagerCondition::Drunk) {
            // EN: Violet/magenta bubbles of drunken revelry floating upwards
            // ES: Burbujas flotantes violetas de embriaguez y festividad
            for (int bubbleIndex = 0; bubbleIndex < 3; ++bubbleIndex) {
                const float bubblePhase = m_animTimer * 3.0f + static_cast<float>(bubbleIndex) * 2.1f;
                const int bubbleOffsetX = static_cast<int>(std::sin(bubblePhase) * 5.0f * escala);
                const int bubbleOffsetY = static_cast<int>(std::fmod(m_animTimer * 8.0f + static_cast<float>(bubbleIndex) * 6.0f, 14.0f) * escala);
                const int bubbleX = emblemBaseX + bubbleOffsetX;
                const int bubbleY = emblemBaseY - bubbleOffsetY;
                const int frameY = dstH - 1 - bubbleY;
                if (bubbleX >= 0 && bubbleX < dstW && frameY >= 0 && frameY < dstH) {
                    dstBuffer[frameY * dstW + bubbleX] = ColorRGB{215, 95, 240}.toRGBA();
                }
            }
        } else if (currentCond == VillagerCondition::Hungry) {
            // EN: Small pulsating orange droplet / stomach pang marker
            // ES: Pequeña gota pulsante naranja de punzada estomacal por hambre
            const float pulse = (std::sin(m_animTimer * 6.0f) + 1.0f) * 0.5f;
            const int pangY = emblemBaseY - static_cast<int>(pulse * 2.0f * escala);
            const int frameY = dstH - 1 - pangY;
            if (emblemBaseX >= 1 && emblemBaseX < dstW - 1 && frameY >= 1 && frameY < dstH - 1) {
                const ColorRGB hungryColor{245, 145, 35};
                dstBuffer[frameY * dstW + emblemBaseX] = hungryColor.toRGBA();
                dstBuffer[(frameY - 1) * dstW + emblemBaseX] = hungryColor.toRGBA();
                dstBuffer[frameY * dstW + (emblemBaseX - 1)] = hungryColor.toRGBA();
                dstBuffer[frameY * dstW + (emblemBaseX + 1)] = hungryColor.toRGBA();
            }
        } else if (currentCond == VillagerCondition::Aggressive) {
            // EN: Crimson rage sparks flickering above head
            // ES: Chispas carmesí de furia combativa parpadeando sobre la cabeza
            for (int sparkIndex = 0; sparkIndex < 3; ++sparkIndex) {
                const int sparkOffsetX = ((sparkIndex == 0) ? -3 : ((sparkIndex == 1) ? 0 : 3));
                const int sparkOffsetY = ((static_cast<int>(m_animTimer * 12.0f + sparkIndex * 2) % 4) + 1);
                const int sparkX = emblemBaseX + static_cast<int>(sparkOffsetX * escala);
                const int sparkY = emblemBaseY - static_cast<int>(sparkOffsetY * escala);
                const int frameY = dstH - 1 - sparkY;
                if (sparkX >= 0 && sparkX < dstW && frameY >= 0 && frameY < dstH) {
                    dstBuffer[frameY * dstW + sparkX] = ColorRGB{245, 45, 45}.toRGBA();
                }
            }
        } else if (currentCond == VillagerCondition::Scared) {
            // EN: Yellow exclamation mark '!' vibrating with fear
            // ES: Signo de exclamación amarillo '!' vibrando por temor
            const int jitterX = static_cast<int>(std::sin(m_animTimer * 20.0f) * 1.5f);
            const int markX = emblemBaseX + jitterX;
            for (int dotY = 0; dotY <= 3; ++dotY) {
                const int frameY = dstH - 1 - (emblemBaseY - static_cast<int>(dotY * escala));
                if (markX >= 0 && markX < dstW && frameY >= 0 && frameY < dstH) {
                    dstBuffer[frameY * dstW + markX] = ColorRGB{255, 230, 45}.toRGBA();
                }
            }
            const int dotFrameY = dstH - 1 - (emblemBaseY + static_cast<int>(2.0f * escala));
            if (markX >= 0 && markX < dstW && dotFrameY >= 0 && dotFrameY < dstH) {
                dstBuffer[dotFrameY * dstW + markX] = ColorRGB{255, 230, 45}.toRGBA();
            }
        } else if (currentCond == VillagerCondition::Inspired) {
            // EN: Radiant golden divine sparks ascending
            // ES: Chispas divinas doradas ascendentes de inspiración
            const float sparkAngle = m_animTimer * 4.0f;
            const int sparkX = emblemBaseX + static_cast<int>(std::cos(sparkAngle) * 4.0f * escala);
            const int sparkY = emblemBaseY - static_cast<int>(std::sin(sparkAngle) * 3.0f * escala);
            const int frameY = dstH - 1 - sparkY;
            if (sparkX >= 0 && sparkX < dstW && frameY >= 0 && frameY < dstH) {
                dstBuffer[frameY * dstW + sparkX] = ColorRGB{255, 235, 110}.toRGBA();
            }
        } else if (currentCond == VillagerCondition::Injured) {
            // EN: Dripping crimson blood droplets
            // ES: Gotas de sangre carmesí goteando por heridas
            const int bloodOffsetY = static_cast<int>(std::fmod(m_animTimer * 7.0f, 8.0f) * escala);
            const int bloodX = emblemBaseX - static_cast<int>(2.0f * escala);
            const int bloodY = emblemBaseY + bloodOffsetY;
            const int frameY = dstH - 1 - bloodY;
            if (bloodX >= 0 && bloodX < dstW && frameY >= 0 && frameY < dstH) {
                dstBuffer[frameY * dstW + bloodX] = ColorRGB{215, 30, 30}.toRGBA();
            }
        }
    }
}



// =============================================================================
// EN: Parental investment: share gathered food or body reserves with a hungry child
// ES: Inversión parental: compartir alimento recolectado o reservas corporales con un hijo hambriento
// =============================================================================
bool Humano::feedChild(Humano& childRef) {
    if (!estaVivo() || !childRef.estaVivo() || !childRef.esNino()) {
        return false;
    }
    // EN: Verify biological kinship or guardianship
    // ES: Verificar parentesco biológico o tutela
    if (childRef.getMadreId() != m_id && childRef.getPadreId() != m_id) {
        return false;
    }

    // EN: Parent shares food from carried inventory or direct nutritional reserves
    // ES: El progenitor comparte alimento de su inventario recolectado o reserva nutricional
    constexpr float minParentNutritionThreshold = 35.0f;
    constexpr float foodTransferAmount = 24.0f;

    bool sharedFood = false;
    if (m_gatheringResourceType == ResourceType::Food && m_carriedResourceAmount > 0) {
        m_carriedResourceAmount = std::max(0, m_carriedResourceAmount - 1);
        childRef.getHumanUnitMutable().nutritionalState = std::clamp(
            childRef.getHumanUnit().nutritionalState + foodTransferAmount, 0.0f, 100.0f);
        childRef.setHealth(childRef.getHealth() + 6.0f);
        sharedFood = true;
    } else if (m_humanUnit.nutritionalState > minParentNutritionThreshold) {
        m_humanUnit.nutritionalState -= 8.0f;
        childRef.getHumanUnitMutable().nutritionalState = std::clamp(
            childRef.getHumanUnit().nutritionalState + foodTransferAmount * 0.75f, 0.0f, 100.0f);
        childRef.setHealth(childRef.getHealth() + 4.0f);
        sharedFood = true;
    }

    if (sharedFood) {
        m_parentalCareTimer = 4.0f;
        m_action = HumanAction::SALUDAR;
        childRef.triggerFaithIndicator(+1, 1.8f);
        m_humanUnit.currentState = HumanAIState::SOCIALIZING;
        return true;
    }
    return false;
}

// =============================================================================
// EN: Active courtship: build romantic affection with a compatible candidate
// ES: Cortejo activo: fomentar el afecto romántico con un pretendiente compatible
// =============================================================================
bool Humano::courtCandidate(Humano& candidateRef, float deltaTime, EventLine* eventLine, int currentYear) {
    if (!estaVivo() || !candidateRef.estaVivo() || m_isChild || candidateRef.esNino()) {
        return false;
    }
    if (tienePareja() || candidateRef.tienePareja()) {
        return false;
    }
    if (m_gender == candidateRef.getGender()) {
        return false;
    }
    if (m_religionId != candidateRef.getReligionId()) {
        return false;
    }

    constexpr float affectionGainRate = 0.12f;
    constexpr float nuptialAffectionThreshold = 0.55f;

    // EN: Mutual courtship display increases affection and reproductive drive
    // ES: La exhibición mutua de cortejo incrementa el afecto y el impulso reproductivo
    modifyAffection(affectionGainRate * deltaTime);
    candidateRef.modifyAffection(affectionGainRate * deltaTime);
    modifyReproductionDrive(0.05f * deltaTime);
    candidateRef.modifyReproductionDrive(0.05f * deltaTime);

    m_action = HumanAction::SALUDAR;
    candidateRef.changeAction(HumanAction::SALUDAR);
    orientTowards(candidateRef.getX(), candidateRef.getY());
    candidateRef.orientTowards(m_x, m_y);

    // EN: Formalize sacred union if mutual affection threshold is achieved
    // ES: Formalizar unión sagrada si se alcanza el umbral de afecto mutuo
    if (m_spouseAffection >= nuptialAffectionThreshold && candidateRef.getSpouseAffection() >= nuptialAffectionThreshold) {
        setSpouseId(candidateRef.getId());
        candidateRef.setSpouseId(m_id);
        setSpouseAffection(0.80f);
        candidateRef.setSpouseAffection(0.80f);

        // EN: Newlyweds experience high nesting drive for a home
        // ES: Los recién casados experimentan alto impulso de anidación para un hogar
        modifyNeedForShelter(0.50f);
        candidateRef.modifyNeedForShelter(0.50f);

        triggerFaithIndicator(+1, 2.0f);
        candidateRef.triggerFaithIndicator(+1, 2.0f);

        if (eventLine) {
            const std::string groomName = esHombre() ? m_name : candidateRef.getName();
            const std::string brideName = esMujer() ? m_name : candidateRef.getName();
            GameEvent weddingEvent;
            weddingEvent.year = currentYear;
            weddingEvent.category = EventCategory::ExogamicPact;
            weddingEvent.title = "Union Conyugal";
            weddingEvent.description = groomName + " y " + brideName + " se han desposado bajo el pacto de la fe tribal, listos para perpetuar su linaje.";
            weddingEvent.actorName = groomName;
            eventLine->addEvent(weddingEvent);
        }
        return true;
    }

    return false;
}

float Humano::calculateReproductionDesire(const Humano& spouseRef, const WorldMap& mapRef,
                                       const BuildingSystem* buildingSys) const {
    const auto& pareja = spouseRef;
    const auto& mapa = mapRef;
    // Si alguno no está vivo o no son biológicamente compatibles
    if (!estaVivo() || !pareja.estaVivo()) return 0.0f;
    if (m_isChild || pareja.esNino()) return 0.0f;
    if (m_gender == pareja.getGenero()) return 0.0f;

    // EN: Guard against demographic runaway and memory bloat if population cap is reached
    // ES: Evitar desbordes demográficos y saturación de memoria si se alcanza el tope de población
    if (ErrorSystem::instance().isPopulationCapReached()) {
        return 0.0f;
    }

    const Humano& madre = esMujer() ? *this : pareja;
    const Humano& padre = esHombre() ? *this : pareja;

    if (madre.estaEmbarazada() || madre.getFertilidadCooldown() > 0.0f || padre.getFertilidadCooldown() > 0.0f) {
        return 0.0f;
    }

    // Afecto conyugal mutuo mínimo para desear procrear
    float amorProm = (m_spouseAffection + pareja.getAmorHaciaPareja()) * 0.5f;
    if (amorProm < 0.50f) return 0.0f;

    // 1. FACTORES AMBIENTALES Y FÍSICOS
    // A. Peligro inminente, combate o pánico suprime por completo el deseo reproductivo
    if (m_damageTimer > 0.0f || pareja.getTimerDano() > 0.0f ||
        m_panicTimer > 0.0f || pareja.getPanicTimer() > 0.0f) {
        return 0.0f;
    }

    float fPeligro = 1.0f;
    if (mapa.isValid(m_cellX, m_cellY)) {
        const Cell& c = mapa.getCell(m_cellX, m_cellY);
        if (c.borderTension > 0.35f || c.hasVolcano || c.hasImpactCrater) {
            fPeligro = 0.20f;
        }
    }

    // B. Nutrición individual de ambos progenitores
    float nutriProm = (m_humanUnit.nutritionalState + pareja.getHumanUnit().nutritionalState) * 0.5f;
    float fNutricion = 1.0f;
    if (nutriProm < 35.0f) {
        fNutricion = 0.08f; // Hambruna severa: infertilidad biológica por supervivencia
    } else if (nutriProm < 55.0f) {
        fNutricion = 0.45f; // Dieta precaria
    } else if (nutriProm >= 70.0f) {
        fNutricion = 1.25f; // Nutrición óptima y vigor biológico
    }

    // C. Reservas comunales de alimentos del clan
    float fComidaComunal = 1.0f;
    if (m_popGroup.foodSupply < 20.0f) {
        fComidaComunal = 0.25f; // Despensa tribal desabastecida
    } else if (m_popGroup.foodSupply >= 55.0f) {
        fComidaComunal = 1.20f; // Abundancia comunitaria
    }

    // D. Cobijo y Capacidad de la Vivienda (Factor crítico de hacinamiento)
    float fCobijo = 0.60f; // Sin hogar propio (a la intemperie)
    int hId = m_homeId > 0 ? m_homeId : pareja.getHomeId();
    if (buildingSys && hId > 0) {
        const House* h = buildingSys->getHouseById(hId);
        if (h && h->isCompleted()) {
            if (h->hasSpace()) {
                // Hogar seguro y con plazas disponibles para acoger al recién nacido
                fCobijo = 1.65f;
            } else {
                // Choza al límite estricto de capacidad: disuasión por hacinamiento
                fCobijo = 0.15f;
            }
        }
    }

    // E. Salud y Energía física
    float saludProm = (m_health + pareja.getSalud()) * 0.5f;
    float energiaProm = (m_humanUnit.energy + pareja.getHumanUnit().energy) * 0.5f;
    float fSalud = (saludProm >= 70.0f && energiaProm >= 45.0f) ? 1.15f : 0.50f;

    float factorAmbiental = fPeligro * fNutricion * fComidaComunal * fCobijo * fSalud;

    // 2. FACTORES ESPIRITUALES
    // A. Devoción y Fervor Religioso ("Multiplicaos y preservad la sagrada fe")
    float devProm = (m_devotion + pareja.getDevocion()) * 0.5f;
    float fDevocion = 0.85f;
    if (devProm >= 0.70f) fDevocion = 1.35f;
    else if (devProm < 0.45f) fDevocion = 0.70f;

    // B. Proximidad a Santuarios, Altares o Suelo Sagrado
    float fSantuario = 1.0f;
    if (mapa.isValid(m_cellX, m_cellY)) {
        const Cell& c = mapa.getCell(m_cellX, m_cellY);
        if (c.hasHolySite || c.shrineLevel > 0) {
            fSantuario = 1.45f; // Bendición divina de fertilidad en suelo sagrado
        }
    }

    // C. Pureza Moral y ausencia de herejía o culpa
    float fMoral = 1.0f;
    const auto& morA = m_moralRecord;
    const auto& morB = pareja.getMoralRecord();
    if (morA.isApostateOrHeretic || morB.isApostateOrHeretic) {
        fMoral = 0.20f; // Mancha de herejía o apostasía
    } else if (morA.guiltLevel > 45.0f || morB.guiltLevel > 45.0f) {
        fMoral = 0.55f; // Culpa acumulada y congoja
    } else if (morA.moralIntegrity >= 0.75f && morB.moralIntegrity >= 0.75f) {
        fMoral = 1.20f; // Almas virtuosas en gracia
    }

    // D. Impulso biológico innato de los organismos vivos (Reproducción y replicación)
    const float biologicalDriveAverage = (m_reproductionDrive + pareja.getReproductionDrive()) * 0.5f;
    const float biologicalDriveMultiplier = std::clamp(0.60f + 0.50f * biologicalDriveAverage, 0.50f, 1.40f);

    float factorEspiritual = fDevocion * fSantuario * fMoral;

    // 3. PROBABILIDAD FINAL DE DESEO DE CONCEPCIÓN
    // Base de 4.0% por ciclo de intimidad conyugal, modulada por factores bio-espirituales
    float probFinal = 0.040f * factorAmbiental * factorEspiritual * (amorProm / 0.60f) * biologicalDriveMultiplier;
    return std::clamp(probFinal, 0.0f, 0.85f);
}

// --- Gestor de Humanos ---
GestorHumanos::GestorHumanos() {
    m_villagers.reserve(128);
}

void GestorHumanos::initializePopulation(WorldMap& mapRef, const ReligionRegistry& registryRef, int populationCount) {
    m_villagers.clear();
    WorldMap& mapa = mapRef;
    const ReligionRegistry& registry = registryRef;
    int cantidad = populationCount;
    int w = mapa.getWidth();
    int h = mapa.getHeight();

    // Capturar santuarios iniciales antes de limpiar las zonas de influencia vacías
    struct HearthNode {
        int x;
        int y;
        int religionId;
    };
    std::vector<HearthNode> hearths;
    for (int cy = 0; cy < h; ++cy) {
        for (int cx = 0; cx < w; ++cx) {
            const Cell& c = mapa.getCell(cx, cy);
            if (c.hasHolySite) {
                int hRel = (c.dominantReligionId > 0) ? c.dominantReligionId : c.holySiteReligionId;
                if (hRel > 0) {
                    hearths.push_back({cx, cy, hRel});
                }
            }
        }
    }

    // Limpiar población y zonas de influencia en TODAS las celdas
    for (int cy = 0; cy < h; ++cy) {
        for (int cx = 0; cx < w; ++cx) {
            Cell& c = mapa.getCell(cx, cy);
            c.population = 0;
            c.faithShare.clear();
            c.dominantReligionId = 0;
            c.dominantFraction = 0.0f;
            c.syncreticTension = 0.0f;
        }
    }

    std::vector<int> activeRelIds;
    for (const auto& r : registry.getAllReligions()) {
        if (!r.isExtinct) {
            activeRelIds.push_back(r.id);
        }
    }
    if (activeRelIds.empty()) {
        activeRelIds.push_back(1);
    }

    std::uniform_int_distribution<int> rx(2, w - 3);
    std::uniform_int_distribution<int> ry(2, h - 3);
    std::uniform_int_distribution<size_t> tDist(0, NOMBRES_GROG.size() - 1);
    std::uniform_real_distribution<float> subOffset(0.2f, 0.8f);
    std::uniform_real_distribution<float> pDist(0.60f, 0.95f);

    int creados = 0;

    // 1. Sembrar a todos los aldeanos dentro de los límites de SU propio pueblo / santuario
    if (!hearths.empty()) {
        int numHearths = static_cast<int>(hearths.size());
        int basePerHearth = cantidad / numHearths;
        int remainder = cantidad % numHearths;

        for (int hIdx = 0; hIdx < numHearths; ++hIdx) {
            const auto& hearth = hearths[hIdx];
            int quota = basePerHearth + (hIdx < remainder ? 1 : 0);

            // Recolectar celdas habitables locales dentro del radio del pueblo (radio 0 a 2, máx 3)
            std::vector<Point2i> villageCells;
            villageCells.push_back({hearth.x, hearth.y});

            for (int r = 1; r <= 3; ++r) {
                for (int dy = -r; dy <= r; ++dy) {
                    for (int dx = -r; dx <= r; ++dx) {
                        if (std::abs(dx) != r && std::abs(dy) != r) continue;
                        int tx = hearth.x + dx;
                        int ty = hearth.y + dy;
                        if (mapa.isValid(tx, ty) && mapa.getCell(tx, ty).isHabitable()) {
                            villageCells.push_back({tx, ty});
                        }
                    }
                }
            }

            if (villageCells.empty()) {
                villageCells.push_back({hearth.x, hearth.y});
            }

            for (int k = 0; k < quota && creados < cantidad; ++k) {
                bool esChamanInicial = (k == 0);
                Point2i targetCell = esChamanInicial ? Point2i{hearth.x, hearth.y} : villageCells[k % villageCells.size()];
                Cell& validCell = mapa.getCell(targetCell.x, targetCell.y);

                float posX = static_cast<float>(targetCell.x) + 0.22f + 0.56f * static_cast<float>((k * 37) % 100) / 100.0f;
                float posY = static_cast<float>(targetCell.y) + 0.22f + 0.56f * static_cast<float>((k * 59) % 100) / 100.0f;

                if (esChamanInicial) {
                    posX = static_cast<float>(hearth.x) + 0.82f;
                    posY = static_cast<float>(hearth.y) + 0.82f;
                }

                // Balance demográfico 50/50 exacto
                HumanGender genero = (creados % 2 == 1) ? HumanGender::Female : HumanGender::Male;
                std::string unitName = Humano::generateName(genero, esChamanInicial);

                Humano hObj(m_nextVillagerId++, unitName, posX, posY, genero);
                hObj.setEsChaman(esChamanInicial);
                hObj.setReligionId(hearth.religionId);
                hObj.setDevocion(esChamanInicial ? 0.98f : pDist(g_humanoRng));
                hObj.changeAction(HumanAction::IDLE);
                hObj.changeDirection(IsoDirection::SW);

                m_villagers.push_back(hObj);
                if (esChamanInicial && hearth.religionId == 1 && m_selectedVillagerId < 0) {
                    m_selectedVillagerId = hObj.getId();
                }

                validCell.population++;
                validCell.addShare(hearth.religionId, hObj.getDevocion());
                validCell.normalizeShares();
                validCell.updateDominance();
                creados++;
            }
        }
    } else {
        // Fallback cuando no hay santuarios consagrados en el mapa
        int intentos = 0;
        int maxIntentos = cantidad * 50;
        while (creados < cantidad && intentos < maxIntentos) {
            intentos++;
            int rxPos = rx(g_humanoRng);
            int ryPos = ry(g_humanoRng);
            Cell& c = mapa.getCell(rxPos, ryPos);

            if (c.isHabitable()) {
                float posX = static_cast<float>(rxPos) + subOffset(g_humanoRng);
                float posY = static_cast<float>(ryPos) + subOffset(g_humanoRng);
                HumanGender genero = (creados % 2 == 1) ? HumanGender::Female : HumanGender::Male;
                std::string uName = Humano::generateName(genero, false);
                Humano hObj(m_nextVillagerId++, uName, posX, posY, genero);

                int rId = activeRelIds[creados % activeRelIds.size()];
                hObj.setReligionId(rId);
                hObj.setDevocion(pDist(g_humanoRng));
                hObj.changeAction(HumanAction::IDLE);
                hObj.changeDirection(IsoDirection::SW);

                m_villagers.push_back(hObj);
                c.population++;
                c.addShare(rId, hObj.getDevocion());
                c.normalizeShares();
                c.updateDominance();
                creados++;
            }
        }
    }
}

void GestorHumanos::update(float deltaTime, WorldMap& mapRef, EventLine* eventLine, int currentYear,
                               ReligionRegistry* registry, SoundSystem* soundSystem,
                               const IsometricCamera* camera,
                               NemesisSififusManager* nemesisSififus,
                               ReligiousMoralitySystem* moralitySystem,
                               const DayNightCycle* dayNight,
                               const BuildingSystem* buildingSystem) {
    const float dt = deltaTime;
    WorldMap& mapa = mapRef;
    int w = mapa.getWidth();
    int h = mapa.getHeight();
    bool esDeNoche = dayNight ? dayNight->isNight() : false;

    // EN: Update active entity count in ErrorSystem for population guardrails
    // ES: Actualizar el conteo de entidades activas en ErrorSystem para límites de población
    ErrorSystem::instance().updateEntityCount(static_cast<uint32_t>(m_villagers.size()));
    const bool isUnderLagStress = ErrorSystem::instance().isUnderLagStress();

    // 1. Movimiento, IA de Primera Era (FirstEraAIController) e interacción nodo-a-nodo
    if (dt > 0.0f) {
        for (auto& hObj : m_villagers) {
            int cx = std::clamp(hObj.getCeldaX(), 0, w - 1);
            int cy = std::clamp(hObj.getCeldaY(), 0, h - 1);
            Cell& curCell = mapa.getCell(cx, cy);
            HumanUnit& hu = hObj.getHumanUnitMutable();
            hu.position = {hObj.getX(), hObj.getY()};
            bool shrineNearby = curCell.hasHolySite;

            // EN: If sleeping peacefully and unthreatened, bypass heavy O(N) social and inter-tribal loops
            // ES: Si está durmiendo plácidamente y sin amenaza, omitir bucles O(N) sociales e inter-tribales
            const bool isSleepingPeacefully = hObj.estaDurmiendo() && !curCell.isContestedBorder && hObj.getTimerDano() <= 0.0f;
            (void)isUnderLagStress;
            if (!isSleepingPeacefully) {
                int ownClanCount = 0;
                int rivalTribeCount = 0;
                float externalConversionPressure = 0.0f;

            for (const auto& other : m_villagers) {
                float dx = other.getX() - hObj.getX();
                float dy = other.getY() - hObj.getY();
                float dsq = dx * dx + dy * dy;
                if (dsq <= 12.25f) {
                    if (other.getReligionId() == hObj.getReligionId()) {
                        ownClanCount++;
                        if (other.esChaman()) shrineNearby = true;
                    } else if (other.getReligionId() > 0) {
                        rivalTribeCount++;
                        float distFactor = 1.0f / (1.0f + std::sqrt(dsq) * 0.45f);
                        externalConversionPressure += other.getHumanUnit().conversionPower * distFactor;
                    }
                }
            }

            if (!shrineNearby) {
                for (int oy = -2; oy <= 2 && !shrineNearby; ++oy) {
                    for (int ox = -2; ox <= 2 && !shrineNearby; ++ox) {
                        if (mapa.isValid(cx + ox, cy + oy) && mapa.getCell(cx + ox, cy + oy).hasHolySite) {
                            shrineNearby = true;
                        }
                    }
                }
            }

            RealisticCellContext context;
            context.hasNeighborTribe = (rivalTribeCount > 0);
            context.neighborTribePowerRatio = (ownClanCount > 0)
                ? std::clamp(static_cast<float>(rivalTribeCount) / static_cast<float>(ownClanCount), 0.1f, 2.0f)
                : 1.0f;
            context.currentBiome = curCell.biome;
            switch (curCell.biome) {
                case BiomeType::FertileValley:
                    context.environmentalCarryingCapacity = 100.0f;
                    break;
                case BiomeType::CoastalPlains:
                    context.environmentalCarryingCapacity = 85.0f;
                    break;
                case BiomeType::DenseForest:
                    context.environmentalCarryingCapacity = 75.0f;
                    break;
                case BiomeType::HighlandHills:
                    context.environmentalCarryingCapacity = 60.0f;
                    break;
                case BiomeType::AridDesert:
                    context.environmentalCarryingCapacity = 30.0f;
                    break;
                case BiomeType::RuggedMountains:
                    context.environmentalCarryingCapacity = 20.0f;
                    break;
                case BiomeType::FrozenTundra:
                    context.environmentalCarryingCapacity = 40.0f;
                    break;
                case BiomeType::VolcanicCaldera:
                    // EN: Hazardous volcanic caldera carrying capacity
                    // ES: Capacidad de carga ambiental peligrosa de caldera volcánica
                    context.environmentalCarryingCapacity = 10.0f;
                    break;
                case BiomeType::DeepOcean:
                case BiomeType::ShallowWater:
                    context.environmentalCarryingCapacity = 0.0f;
                    break;
            }
            context.hasShrine = shrineNearby;

            RealisticPopGroup& pop = hObj.getPopGroupMutable();
            pop.coordinates = {hObj.getX(), hObj.getY()};
            pop.populationCount = std::max(pop.populationCount, ownClanCount * 4);
            if (pop.state != RealisticVillagerState::SURVIVAL_FORAGING) {
                pop.foodSupply = std::max(0.0f, pop.foodSupply - 0.75f * dt);
            }

            m_firstEraAI.UpdateRealisticBehavior(pop, context, dt);

            // Actualizar IA individual de la unidad humana (HumanAIController)
            UnitEnvironmentContext unitEnv;
            unitEnv.hasNearbyShrine = shrineNearby;
            unitEnv.isBorderOrConflictZone = (rivalTribeCount > 0) || curCell.isContestedBorder;
            unitEnv.localDangerLevel = std::clamp(static_cast<float>(rivalTribeCount) * 12.0f +
                                                 (curCell.isContestedBorder ? curCell.borderTension * 35.0f : 0.0f), 0.0f, 100.0f);
            unitEnv.externalConversionPressure = externalConversionPressure;

            hu.tribeID = "Tribu #" + std::to_string(hObj.getReligionId());
            if (hu.currentState != HumanAIState::SURVIVAL_WORKING) {
                hu.nutritionalState = std::max(0.0f, hu.nutritionalState - 0.65f * dt);
            }
            m_humanAI.UpdateHumanAI(hu, unitEnv, dt);
            hu.nutritionalState = std::clamp(hu.nutritionalState, 0.0f, 100.0f);
            pop.foodSupply = std::clamp(0.5f * (pop.foodSupply + hu.nutritionalState), 0.0f, 100.0f);
            pop.divineWill = std::clamp(0.5f * (pop.divineWill + hu.devotion), 0.0f, 100.0f);

            if (!hObj.esChaman()) {
                hObj.setDevocion(std::clamp(hu.devotion / 100.0f, 0.15f, 1.0f));
            } else {
                hObj.setDevocion(std::clamp(hu.devotion / 100.0f, 0.20f, 1.0f));
            }

            // 1.1 Dinámica de Devoción hacia Líderes Espirituales / Profetas y Fanatismo
            const Humano* nearestLeader = nullptr;
            float minLeaderDistSq = 999999.0f;
            if (!hObj.esLiderEspiritual()) {
                for (const auto& other : m_villagers) {
                    if (other.estaVivo() && other.getReligionId() == hObj.getReligionId() && other.esLiderEspiritual()) {
                        float dxx = other.getX() - hObj.getX();
                        float dyy = other.getY() - hObj.getY();
                        float dsq = dxx * dxx + dyy * dyy;
                        if (dsq < minLeaderDistSq) {
                            minLeaderDistSq = dsq;
                            nearestLeader = &other;
                        }
                    }
                }
            }

            if (hObj.esLiderEspiritual()) {
                hObj.setLeaderDevotion(1.0f);
                hObj.setLeaderId(hObj.getId());
            } else if (nearestLeader) {
                hObj.setLeaderId(nearestLeader->getId());
                float leaderDist = std::sqrt(minLeaderDistSq);
                if (leaderDist <= 6.0f) {
                    // Cercanía al líder espiritual: la comunión y guía aumentan la devoción al líder
                    float gainRate = 0.08f;
                    if (nearestLeader->getTimerSenalDivina() > 0.0f) {
                        gainRate += 0.28f; // Éxtasis místico ante la señal divina
                    }
                    if (nearestLeader->getAccion() == HumanAction::REZAR) {
                        gainRate += 0.14f; // Sincronía en oración sagrada
                    }
                    hObj.setLeaderDevotion(hObj.getLeaderDevotion() + gainRate * dt);
                    // El lazo con el líder refuerza también la devoción general
                    hObj.setDevocion(hObj.getDevocion() + 0.05f * dt);

                    // Orientación reverente hacia el líder cuando está ocioso
                    if (hObj.getAccion() == HumanAction::IDLE && leaderDist <= 3.5f) {
                        hObj.orientarHacia(nearestLeader->getX(), nearestLeader->getY());
                    }
                } else if (leaderDist > 8.5f) {
                    // Desconexión espiritual: lejanía física del líder erosiona la devoción personal
                    hObj.setLeaderDevotion(hObj.getLeaderDevotion() - 0.025f * dt);
                    if (hObj.getLeaderDevotion() < 0.30f) {
                        // Tibieza espiritual comunitaria
                        hObj.setDevocion(hObj.getDevocion() - 0.018f * dt);
                    }
                }
            } else {
                // Sin líder vivo en la religión: orfandad espiritual comunitaria
                hObj.setLeaderId(-1);
                hObj.setLeaderDevotion(hObj.getLeaderDevotion() - 0.04f * dt);
            }

            // Transición y comprobación de Fanatismo (punto álgido de devoción)
            if (!hObj.esLiderEspiritual()) {
                if (hObj.getDevocion() >= 0.88f && hObj.getLeaderDevotion() >= 0.85f) {
                    if (hObj.getRole() != CommunityRole::ZEALOT) {
                        int zealotCount = 0;
                        for (const auto& other : m_villagers) {
                            if (&other != &hObj && other.estaVivo() && other.getReligionId() == hObj.getReligionId() &&
                                other.getRole() == CommunityRole::ZEALOT) {
                                zealotCount++;
                            }
                        }
                        int maxZealots = std::max(3, static_cast<int>(ownClanCount * 0.20f));
                        if (zealotCount < maxZealots) {
                            hObj.promoverAFanatico();
                        }
                    }
                } else if (hObj.getRole() == CommunityRole::ZEALOT &&
                          (hObj.getDevocion() < 0.65f && hObj.getLeaderDevotion() < 0.55f)) {
                    hObj.degradarFanatico();
                }
            }

            // Comportamiento sistémico de los Fanáticos
            if (hObj.esFanatico() && hObj.estaVivo()) {
                // A. Escolta de guardia al líder espiritual (no interrumpe tareas activas de recolección, carga u ofrendas)
                bool ocupado = (hObj.getAccion() == HumanAction::RECOLECTAR ||
                                hObj.getAccion() == HumanAction::TALAR ||
                                hObj.getAccion() == HumanAction::CARGAR ||
                                hObj.isDeliveringOffering() ||
                                hObj.getPopGroup().foodSupply < 40.0f);
                if (nearestLeader && nearestLeader->estaVivo() && !hObj.tieneOrdenManual() &&
                    hObj.getAccion() != HumanAction::ATACAR && !ocupado) {
                    float ldDist = std::sqrt(minLeaderDistSq);
                    if (ldDist > 3.6f) {
                        float offsetAng = static_cast<float>(hObj.getId() % 6) * 1.047f;
                        float targetX = nearestLeader->getX() + std::cos(offsetAng) * 1.5f;
                        float targetY = nearestLeader->getY() + std::sin(offsetAng) * 1.5f;
                        int tcx = static_cast<int>(std::floor(targetX));
                        int tcy = static_cast<int>(std::floor(targetY));
                        if (mapa.isValid(tcx, tcy) && mapa.getCell(tcx, tcy).isPassableForGround()) {
                            hObj.caminarHacia(targetX, targetY, ldDist > 6.0f);
                        }
                    }
                }

                // B. Admonición Zelote: reprender y reavivar a miembros tibios de la misma fe
                for (auto& other : m_villagers) {
                    if (&other == &hObj || !other.estaVivo() || other.getReligionId() != hObj.getReligionId()) continue;
                    if (other.esLiderEspiritual() || other.esFanatico()) continue;

                    float dxx = other.getX() - hObj.getX();
                    float dyy = other.getY() - hObj.getY();
                    if (dxx * dxx + dyy * dyy <= 9.0f) { // ~3 celdas
                        if (other.getDevocion() < 0.45f || other.getLeaderDevotion() < 0.40f) {
                            other.setDevocion(other.getDevocion() + 0.18f * dt);
                            other.setLeaderDevotion(other.getLeaderDevotion() + 0.20f * dt);
                            other.triggerFaithIndicator(+1, 1.5f);
                        }
                    }
                }
            }

            // Incursión de Saqueo de Recursos en frontera hostil
            if (curCell.isContestedBorder && !curCell.isSacredTruceActive && curCell.borderTension >= 0.40f) {
                if (curCell.naturalResource != ResourceType::None && curCell.resourceAmount > 0 &&
                    curCell.dominantReligionId > 0 && curCell.dominantReligionId != hObj.getReligionId()) {
                    if (hObj.getPopGroup().foodSupply < 45.0f || hObj.getRole() == CommunityRole::ZEALOT) {
                        int pillaged = curCell.harvestResource(1);
                        if (pillaged > 0 && registry) {
                            Religion* rOwn = registry->getReligion(hObj.getReligionId());
                            if (rOwn) {
                                if (curCell.naturalResource == ResourceType::Food) rOwn->resources.food += pillaged;
                                else if (curCell.naturalResource == ResourceType::Wood) rOwn->resources.wood += pillaged;
                                else if (curCell.naturalResource == ResourceType::Stone) rOwn->resources.stone += pillaged;
                                else if (curCell.naturalResource == ResourceType::Gold) rOwn->resources.gold += pillaged;
                                else if (curCell.naturalResource == ResourceType::Iron) rOwn->resources.iron += pillaged;
                                else if (curCell.naturalResource == ResourceType::Copper) rOwn->resources.copper += pillaged;
                                else if (curCell.naturalResource == ResourceType::Diamond) {
                                    rOwn->resources.diamond += pillaged;
                                    rOwn->theologyPoints += pillaged * 2;
                                }
                                hObj.getPopGroupMutable().foodSupply = std::min(100.0f, hObj.getPopGroup().foodSupply + 6.0f);
                            }
                        }
                    }
                }
            }

            // Asedio o Profanación de Santuario rival abandonado en territorio hostil
            if (curCell.hasHolySite && curCell.holySiteReligionId > 0 &&
                curCell.holySiteReligionId != hObj.getReligionId() && !curCell.isSacredTruceActive) {
                bool hasDefendersNearby = false;
                for (const auto& other : m_villagers) {
                    if (other.estaVivo() && other.getReligionId() == curCell.holySiteReligionId) {
                        float distSq = (other.getX() - curCell.x - 0.5f) * (other.getX() - curCell.x - 0.5f) +
                                       (other.getY() - curCell.y - 0.5f) * (other.getY() - curCell.y - 0.5f);
                        if (distSq <= 6.25f) { // ~2.5 celdas
                            hasDefendersNearby = true;
                            break;
                        }
                    }
                }

                if (!hasDefendersNearby && (hObj.getRole() == CommunityRole::ZEALOT || curCell.borderTension >= 0.50f)) {
                    curCell.devotion = std::max(0.05f, curCell.devotion - 0.20f * dt);
                    if (registry) {
                        Religion* rAtk = registry->getReligion(hObj.getReligionId());
                        Religion* rDef = registry->getReligion(curCell.holySiteReligionId);
                        if (rAtk && rDef) {
                            rAtk->theologyPoints += static_cast<int>(2.0f * dt);
                            rDef->theologyPoints = std::max(0, rDef->theologyPoints - static_cast<int>(3.0f * dt));
                        }
                        if (curCell.devotion <= 0.12f && curCell.shrineLevel > 1) {
                            curCell.shrineLevel--;
                            if (eventLine && rAtk && rDef) {
                                GameEvent ev;
                                ev.year = currentYear;
                                ev.category = EventCategory::BorderConflict;
                                ev.location = {cx, cy};
                                ev.religionAId = rAtk->id;
                                ev.religionBId = rDef->id;
                                ev.badgeColor = rAtk->color;
                                ev.title = "Profanacion de Santuario";
                                ev.description = "Incursores zelotes de " + rAtk->name + " han profanado y saqueado el altar sagrado de " + rDef->name + "!";
                                ev.isTurningPoint = false;
                                ev.actorName = hObj.getNombre();
                                ev.consequence = "Perdida de fervor y degradacion del templo rival";
                                eventLine->addEvent(ev);
                            }
                        }
                    }
                }
            }

            // EN: End of active awake villager simulation / ES: Fin de simulación de aldeano despierto activo
            }

            // =========================================================================
            // EN: Circadian rhythm, biological sleep and morning wake-up
            // ES: Actualización del Sistema Circadiano, Sueño y Despertar Matutino
            // =========================================================================
            if (hObj.estaDurmiendo()) {
                hObj.cambiarAccion(HumanAction::IDLE);
                bool tieneCasaTerminada = false;
                if (hObj.tieneHogar() && buildingSystem) {
                    const House* h = buildingSystem->getHouseById(hObj.getHomeId());
                    if (h && h->isCompleted()) tieneCasaTerminada = true;
                }

                if (tieneCasaTerminada) {
                    // Descanso óptimo bajo techo (+28 energía/s, regenera salud y sueño acelerado)
                    hObj.modificarSueno(-22.0f * dt);
                    hu.energy = std::min(100.0f, hu.energy + 28.0f * dt);
                    hObj.setSalud(std::min(100.0f, hObj.getSalud() + 3.5f * dt));
                } else {
                    // Descanso al ras del suelo o junto a fogata comunal
                    hObj.modificarSueno(-14.0f * dt);
                    hu.energy = std::min(100.0f, hu.energy + 16.0f * dt);
                    hObj.setSalud(std::min(100.0f, hObj.getSalud() + 1.5f * dt));
                }
                hObj.getHumanUnitMutable().hitPoints = hObj.getSalud();

                // EN: Wake up at dawn or daytime if reasonably rested, or immediately if daytime has arrived and unit is not critically exhausted
                // ES: Despertar al amanecer o de día si está razonablemente descansado, o si amaneció y no está exhausto crítico
                bool isRested = (hObj.getSueno() <= 35.0f || hu.energy >= 65.0f);
                bool forceDawnWakeup = (!esDeNoche && (isRested || hu.energy >= 35.0f));
                if (forceDawnWakeup || hObj.tieneFuriaProtectora()) {
                    hObj.setDurmiendo(false);
                    hObj.setDurmiendoBajoTecho(false);
                    hObj.cambiarAccion(HumanAction::IDLE);
                }
            } else {
                // Acumulación natural de cansancio y fatiga biológica
                float tasaSueno = esDeNoche ? 2.6f : 0.85f;
                if (hObj.getAccion() == HumanAction::ATACAR || hObj.getAccion() == HumanAction::CORRER) {
                    tasaSueno += 1.4f;
                } else if (hObj.getAccion() == HumanAction::TALAR || hObj.getAccion() == HumanAction::RECOLECTAR) {
                    tasaSueno += 0.8f;
                }
                hObj.modificarSueno(tasaSueno * dt);

                // Gasto ordinario de energía según actividad física
                if (hObj.getAccion() == HumanAction::IDLE || hObj.getAccion() == HumanAction::REZAR) {
                    hu.energy = std::min(100.0f, hu.energy + 8.0f * dt);
                } else if (hObj.getAccion() == HumanAction::CORRER) {
                    hu.energy = std::max(0.0f, hu.energy - 2.8f * dt);
                } else if (hObj.getAccion() == HumanAction::ATACAR) {
                    hu.energy = std::max(0.0f, hu.energy - 3.5f * dt);
                } else if (hObj.getAccion() == HumanAction::TALAR || hObj.getAccion() == HumanAction::RECOLECTAR) {
                    hu.energy = std::max(0.0f, hu.energy - 2.0f * dt);
                } else if (hObj.getAccion() == HumanAction::CAMINAR || hObj.getAccion() == HumanAction::CARGAR) {
                    hu.energy = std::max(0.0f, hu.energy - 1.2f * dt);
                }
            }

            // Actualización de necesidades espirituales: la oración mitiga el hambre del alma
            if (hObj.getAccion() == HumanAction::REZAR) {
                hObj.setSpiritualHunger(std::max(0.0f, hObj.getSpiritualHunger() - 22.0f * dt));
                hObj.setDevocion(std::min(1.0f, hObj.getDevocion() + 0.06f * dt));
                if (registry && (curCell.hasHolySite || shrineNearby)) {
                    Religion* r = registry->getReligion(hObj.getReligionId());
                    if (r) r->theologyPoints += static_cast<int>(1.2f * dt);
                }
                if (moralitySystem && hObj.getMoralRecord().hasGuilt()) {
                    moralitySystem->processPenance(hObj, dt, curCell.hasHolySite || shrineNearby, shrineNearby, registry, soundSystem, camera);
                }
            } else {
                hObj.setSpiritualHunger(std::min(100.0f, hObj.getSpiritualHunger() + 0.65f * dt));
            }

            if (hObj.getAttackCooldown() > 0.0f) {
                hObj.setAttackCooldown(hObj.getAttackCooldown() - dt);
            }

            // Procesar consumación de entrega de ofrendas o almacenamiento al arribar a destino
            if (hObj.getCantidadRecursoCargado() > 0) {
                if (hObj.isDeliveringOffering()) {
                    if (curCell.hasHolySite && (curCell.holySiteReligionId == hObj.getReligionId() || curCell.dominantReligionId == hObj.getReligionId())) {
                        curCell.devotion = std::min(1.0f, curCell.devotion + 0.15f);
                        hObj.setDevocion(std::min(1.0f, hObj.getDevocion() + 0.12f));
                        hObj.setSpiritualHunger(0.0f);
                        hObj.triggerFaithIndicator(+1, 2.0f);
                        if (registry) {
                            Religion* r = registry->getReligion(hObj.getReligionId());
                            if (r) r->theologyPoints += 15;
                        }
                        if (moralitySystem && moralitySystem->isRuleActive(hObj.getReligionId(), MoralRuleType::HolyOfferingTithe)) {
                            moralitySystem->recordRighteousAct(hObj, MoralRuleType::HolyOfferingTithe, 25.0f, currentYear, registry);
                        }
                        hObj.setCantidadRecursoCargado(0);
                        hObj.setDeliveringOffering(false);
                        hObj.cambiarAccion(HumanAction::REZAR);
                    }
                } else {
                    if (curCell.dominantReligionId == hObj.getReligionId() || curCell.hasHolySite) {
                        if (registry) {
                            Religion* r = registry->getReligion(hObj.getReligionId());
                            if (r) {
                                r->resources.addResource(hObj.getTipoRecursoRecolectando(), hObj.getCantidadRecursoCargado());
                                if (hObj.getTipoRecursoRecolectando() == ResourceType::Diamond) {
                                    r->theologyPoints += hObj.getCantidadRecursoCargado() * 2;
                                }
                            }
                        }
                        hObj.setCantidadRecursoCargado(0);
                        hObj.cambiarAccion(HumanAction::SALUDAR);
                    }
                }
            }

            hObj.actualizar(dt, mapa, &m_villagers, esDeNoche, buildingSystem);

            // Efectos de sonido ambientales de las actividades humanas (sincronizados con el ritmo de trabajo)
            if (soundSystem && camera && hObj.estaVivo() && dt > 0.0f) {
                float idPhase = static_cast<float>(hObj.getId() % 7) * 0.16f;
                float animT = hObj.getAnimTimer() + idPhase;
                float prevAnimT = animT - dt;

                if (hObj.getAccion() == HumanAction::TALAR) {
                    // Golpe de hacha rítmico cada 1.05s sincronizado con el balanceo del leñador
                    if (static_cast<int>(animT / 1.05f) != static_cast<int>(prevAnimT / 1.05f)) {
                        soundSystem->playSpatialSFX(SFXType::TreeChop, hObj.getX(), hObj.getY(), *camera, 0.70f);
                    }
                } else if (hObj.getAccion() == HumanAction::RECOLECTAR) {
                    if (curCell.naturalResource == ResourceType::Stone ||
                        curCell.naturalResource == ResourceType::Iron ||
                        curCell.naturalResource == ResourceType::Gold ||
                        curCell.naturalResource == ResourceType::Copper ||
                        curCell.naturalResource == ResourceType::Diamond) {
                        // Golpe de piqueta cada 1.15s al picar minerales o canteras
                        if (static_cast<int>(animT / 1.15f) != static_cast<int>(prevAnimT / 1.15f)) {
                            soundSystem->playSpatialSFX(SFXType::StoneMine, hObj.getX(), hObj.getY(), *camera, 0.65f);
                        }
                    } else if (curCell.naturalResource == ResourceType::Food) {
                        // Crujido de arbusto cada 1.35s al recolectar bayas o hierbas
                        if (static_cast<int>(animT / 1.35f) != static_cast<int>(prevAnimT / 1.35f)) {
                            soundSystem->playSpatialSFX(SFXType::GatherFood, hObj.getX(), hObj.getY(), *camera, 0.55f);
                        }
                    }
                } else if (hObj.isSwimming()) {
                    // Brazada de nado cada 0.90s en el agua
                    if (static_cast<int>(animT / 0.90f) != static_cast<int>(prevAnimT / 0.90f)) {
                        soundSystem->playSpatialSFX(SFXType::WaterSplash, hObj.getX(), hObj.getY(), *camera, 0.60f);
                    }
                }
            }
        }

        // 1.5. Separación suave entre aldeanos para evitar aglomeraciones en un solo punto
        const size_t numH = m_villagers.size();
        for (size_t i = 0; i < numH; ++i) {
            Humano& a = m_villagers[i];
            if (!a.estaVivo() || a.isRagdoll() || a.getAltitude() > 0.0f || a.estaDurmiendo()) continue;

            for (size_t j = i + 1; j < numH; ++j) {
                Humano& b = m_villagers[j];
                if (!b.estaVivo() || b.isRagdoll() || b.getAltitude() > 0.0f || b.estaDurmiendo()) continue;

                float dx = b.getX() - a.getX();
                float dy = b.getY() - a.getY();
                float dsq = dx * dx + dy * dy;
                if (dsq < 0.0625f && dsq > 0.000001f) { // ~0.25 celdas
                    float dist = std::sqrt(dsq);
                    float overlap = 0.25f - dist;
                    float push = overlap * 0.5f * std::min(1.0f, dt * 3.0f);
                    float nx = (dx / dist) * push;
                    float ny = (dy / dist) * push;

                    float nax = a.getX() - nx;
                    float nay = a.getY() - ny;
                    int caX = static_cast<int>(std::floor(nax));
                    int caY = static_cast<int>(std::floor(nay));
                    if (mapa.isValid(caX, caY) && mapa.getCell(caX, caY).isPassableForGround()) {
                        a.setPosicion(nax, nay);
                    }

                    float nbx = b.getX() + nx;
                    float nby = b.getY() + ny;
                    int cbX = static_cast<int>(std::floor(nbx));
                    int cbY = static_cast<int>(std::floor(nby));
                    if (mapa.isValid(cbX, cbY) && mapa.getCell(cbX, cbY).isPassableForGround()) {
                        b.setPosicion(nbx, nby);
                    }
                }
            }
        }

        // 2. Combate cuerpo a cuerpo, defensa de la fe y conversión espiritual directa
        const size_t n = m_villagers.size();
        for (size_t i = 0; i < n; ++i) {
            for (size_t j = i + 1; j < n; ++j) {
                Humano& a = m_villagers[i];
                Humano& b = m_villagers[j];
                if (!a.estaVivo() || !b.estaVivo()) continue;
                if (a.getReligionId() == b.getReligionId()) continue;

                float dx = a.getX() - b.getX();
                float dy = a.getY() - b.getY();
                float dsq = dx * dx + dy * dy;

                // Comprobar si hay tregua sagrada o pacto de no agresión en las celdas
                bool enTregua = false;
                int caX = std::clamp(a.getCeldaX(), 0, w - 1);
                int caY = std::clamp(a.getCeldaY(), 0, h - 1);
                int cbX = std::clamp(b.getCeldaX(), 0, w - 1);
                int cbY = std::clamp(b.getCeldaY(), 0, h - 1);
                if ((mapa.isValid(caX, caY) && mapa.getCell(caX, caY).isSacredTruceActive) ||
                    (mapa.isValid(cbX, cbY) && mapa.getCell(cbX, cbY).isSacredTruceActive)) {
                    enTregua = true;
                }

                // A. PERCEPCIÓN DE AMENAZAS Y COORDINACIÓN DEFENSIVA/OFENSIVA (Radio <= 4.5 celdas, dsq <= 20.25)
                if (!enTregua && dsq <= 20.25f) {
                    bool amenazaParaA = (b.getAccion() == HumanAction::ATACAR) ||
                                       (b.esFanatico() || a.esFanatico()) ||
                                       (mapa.isValid(caX, caY) && mapa.getCell(caX, caY).borderTension >= 0.35f);
                    bool amenazaParaB = (a.getAccion() == HumanAction::ATACAR) ||
                                       (a.esFanatico() || b.esFanatico()) ||
                                       (mapa.isValid(cbX, cbY) && mapa.getCell(cbX, cbY).borderTension >= 0.35f);

                    // Comprobar si rival amenaza al líder espiritual o templo sagrado de A
                    if (a.esLiderEspiritual() || (mapa.isValid(caX, caY) && mapa.getCell(caX, caY).hasHolySite)) {
                        amenazaParaA = true;
                    }
                    if (b.esLiderEspiritual() || (mapa.isValid(cbX, cbY) && mapa.getCell(cbX, cbY).hasHolySite)) {
                        amenazaParaB = true;
                    }

                    // Reacción de Unidad A ante la amenaza de B
                    if (amenazaParaA && a.getAccion() != HumanAction::ATACAR && a.getAccion() != HumanAction::CORRER) {
                        bool esAptoParaLucha = (a.getSalud() >= 35.0f) &&
                                              (a.getHumanUnit().willpower >= 35.0f || a.esFanatico() || a.getRole() == CommunityRole::ZEALOT);
                        if (esAptoParaLucha) {
                            a.getHumanUnitMutable().currentState = HumanAIState::DEFENDING;
                            a.setTargetEnemyId(b.getId());
                            if (dsq <= 3.24f) {
                                a.cambiarAccion(HumanAction::ATACAR);
                                a.orientarHacia(b.getX(), b.getY());
                            } else {
                                a.caminarHacia(b.getX(), b.getY(), true);
                            }

                            // Grito de Alerta Comunitario (Rally Call): llamar a defensores aliados cercanos (radio 6 celdas)
                            for (auto& defensor : m_villagers) {
                                if (&defensor == &a || !defensor.estaVivo() || defensor.getReligionId() != a.getReligionId()) continue;
                                if (defensor.getSalud() >= 35.0f && defensor.getAccion() != HumanAction::ATACAR) {
                                    float distDefSq = (defensor.getX() - b.getX()) * (defensor.getX() - b.getX()) +
                                                      (defensor.getY() - b.getY()) * (defensor.getY() - b.getY());
                                    if (distDefSq <= 36.0f) { // ~6 celdas
                                        defensor.getHumanUnitMutable().currentState = HumanAIState::DEFENDING;
                                        defensor.setTargetEnemyId(b.getId());
                                        if (distDefSq <= 3.24f) {
                                            defensor.cambiarAccion(HumanAction::ATACAR);
                                            defensor.orientarHacia(b.getX(), b.getY());
                                        } else {
                                            defensor.caminarHacia(b.getX(), b.getY(), true);
                                        }
                                    }
                                }
                            }
                        } else {
                            // No combatiente vulnerable: huir en pánico
                            a.getHumanUnitMutable().currentState = HumanAIState::SURVIVAL_WORKING;
                            a.huirEnPanicoDesde(b.getX(), b.getY(), 3.5f);
                        }
                    }

                    // Reacción de Unidad B ante la amenaza de A
                    if (amenazaParaB && b.getAccion() != HumanAction::ATACAR && b.getAccion() != HumanAction::CORRER) {
                        bool esAptoParaLucha = (b.getSalud() >= 35.0f) &&
                                              (b.getHumanUnit().willpower >= 35.0f || b.esFanatico() || b.getRole() == CommunityRole::ZEALOT);
                        if (esAptoParaLucha) {
                            b.getHumanUnitMutable().currentState = HumanAIState::DEFENDING;
                            b.setTargetEnemyId(a.getId());
                            if (dsq <= 3.24f) {
                                b.cambiarAccion(HumanAction::ATACAR);
                                b.orientarHacia(a.getX(), a.getY());
                            } else {
                                b.caminarHacia(a.getX(), a.getY(), true);
                            }

                            // Grito de Alerta Comunitario para el bando B
                            for (auto& defensor : m_villagers) {
                                if (&defensor == &b || !defensor.estaVivo() || defensor.getReligionId() != b.getReligionId()) continue;
                                if (defensor.getSalud() >= 35.0f && defensor.getAccion() != HumanAction::ATACAR) {
                                    float distDefSq = (defensor.getX() - a.getX()) * (defensor.getX() - a.getX()) +
                                                      (defensor.getY() - a.getY()) * (defensor.getY() - a.getY());
                                    if (distDefSq <= 36.0f) {
                                        defensor.getHumanUnitMutable().currentState = HumanAIState::DEFENDING;
                                        defensor.setTargetEnemyId(a.getId());
                                        if (distDefSq <= 3.24f) {
                                            defensor.cambiarAccion(HumanAction::ATACAR);
                                            defensor.orientarHacia(a.getX(), a.getY());
                                        } else {
                                            defensor.caminarHacia(a.getX(), a.getY(), true);
                                        }
                                    }
                                }
                            }
                        } else {
                            // No combatiente vulnerable: huir en pánico
                            b.getHumanUnitMutable().currentState = HumanAIState::SURVIVAL_WORKING;
                            b.huirEnPanicoDesde(a.getX(), a.getY(), 3.5f);
                        }
                    }
                }

                // B. COMBATE CUERPO A CUERPO (Alcance melee <= 1.8 celdas, dsq <= 3.24)
                if (!enTregua && dsq <= 3.24f) {
                    if (a.getAccion() != HumanAction::ATACAR && a.getSalud() >= 35.0f) {
                        a.cambiarAccion(HumanAction::ATACAR);
                    }
                    if (b.getAccion() != HumanAction::ATACAR && b.getSalud() >= 35.0f) {
                        b.cambiarAccion(HumanAction::ATACAR);
                    }
                    // Golpe de A hacia B
                    if (a.getAccion() == HumanAction::ATACAR && a.getAttackCooldown() <= 0.0f && b.estaVivo()) {
                        a.orientarHacia(b.getX(), b.getY());
                        a.setAttackCooldown(a.esFanatico() ? 0.60f : 0.85f);
                        float atkPowerA = a.getHumanUnit().attackPower * (a.esFanatico() ? 1.40f : 1.0f);
                        if (a.tieneFuriaProtectora()) atkPowerA *= 1.35f; // +35% bono por defender a sus seres queridos

                        // EN: Modulate attack power by active physiological/psychological condition
                        // ES: Modular potencia de ataque según condición fisiológica/psicológica activa
                        const VillagerCondition conditionA = a.getCondition();
                        switch (conditionA) {
                            case VillagerCondition::Aggressive: atkPowerA *= 1.35f; break; // +35% furia combativa
                            case VillagerCondition::Inspired:   atkPowerA *= 1.20f; break; // +20% éxtasis sagrado
                            case VillagerCondition::Injured:    atkPowerA *= 0.75f; break; // -25% debilitado por heridas
                            case VillagerCondition::Drunk:      atkPowerA *= 0.70f; break; // -30% torpeza por ebriedad
                            case VillagerCondition::Sick:       atkPowerA *= 0.65f; break; // -35% debilidad febril
                            case VillagerCondition::Hungry:     atkPowerA *= 0.80f; break; // -20% desfallecimiento por hambre
                            case VillagerCondition::Exhausted:  atkPowerA *= 0.60f; break; // -40% fatiga extrema
                            default:                            break;
                        }

                        float resB = b.getHumanUnit().resistance;
                        float baseDmgA = std::max(6.0f, atkPowerA * 0.85f - resB * 0.15f);
                        b.recibirDano(baseDmgA);

                        // Alarma de Furia Protectora para la familia de B (despertar y correr al rescate)
                        for (auto& kin : m_villagers) {
                            if (&kin == &b || !kin.estaVivo() || kin.getReligionId() != b.getReligionId()) continue;
                            bool esFamiliar = (kin.getParejaId() == b.getId());
                            if (kin.getId() == b.getMadreId() || kin.getId() == b.getPadreId()) esFamiliar = true;
                            if (b.getMadreId() == kin.getId() || b.getPadreId() == kin.getId()) esFamiliar = true;
                            for (int hid : kin.getHijosIds()) { if (hid == b.getId()) { esFamiliar = true; break; } }
                            for (int hid : b.getHijosIds()) { if (hid == kin.getId()) { esFamiliar = true; break; } }

                            if (esFamiliar) {
                                float kdx = kin.getX() - b.getX();
                                float kdy = kin.getY() - b.getY();
                                if (kdx * kdx + kdy * kdy <= 64.0f) { // Radio de 8 celdas
                                    kin.activarFuriaProtectora(7.5f);
                                    kin.setTargetProtegidoId(b.getId());
                                    kin.setTargetEnemyId(a.getId());
                                    kin.getHumanUnitMutable().currentState = HumanAIState::DEFENDING;
                                    kin.caminarHacia(a.getX(), a.getY(), true);
                                }
                            }
                        }

                        if (soundSystem && camera) {
                            soundSystem->playSpatialSFX(SFXType::CombatAttack, b.getX(), b.getY(), *camera, 0.80f);
                        }
                        if (nemesisSififus && b.estaVivo()) {
                            nemesisSififus->onUnitSurvivesCombat(b, a.getId(), static_cast<int16_t>(a.getReligionId()),
                                                                baseDmgA, currentYear, eventLine, registry);
                        }
                        if (moralitySystem) {
                            if (a.getReligionId() == b.getReligionId()) {
                                if (moralitySystem->isRuleActive(a.getReligionId(), MoralRuleType::SacredKinProtection)) {
                                    moralitySystem->recordTransgression(a, MoralRuleType::SacredKinProtection,
                                                                        "Derramamiento de sangre contra hermano de fe " + b.getNombre(),
                                                                        currentYear, eventLine, registry);
                                }
                            } else {
                                if (moralitySystem->isRuleActive(a.getReligionId(), MoralRuleType::ZealousHolyWar)) {
                                    moralitySystem->recordRighteousAct(a, MoralRuleType::ZealousHolyWar, 12.0f, currentYear, registry);
                                }
                                int cax = std::clamp(a.getCeldaX(), 0, w - 1);
                                int cay = std::clamp(a.getCeldaY(), 0, h - 1);
                                const Cell& cellA = mapa.getCell(cax, cay);
                                if (cellA.hasHolySite && (cellA.holySiteReligionId == a.getReligionId()) &&
                                    moralitySystem->isRuleActive(a.getReligionId(), MoralRuleType::SanctuaryDefense)) {
                                    moralitySystem->recordRighteousAct(a, MoralRuleType::SanctuaryDefense, 18.0f, currentYear, registry);
                                }
                            }
                        }

                        // Si B perece en combate
                        if (b.getSalud() <= 0.0f) {
                            if (soundSystem && camera) {
                                soundSystem->playSpatialSFX(SFXType::UnitDeath, b.getX(), b.getY(), *camera, 0.85f);
                            }
                            b.matar("Caído en batalla por la defensa de su fe ante " + a.getNombre());
                            for (auto& villager : m_villagers) {
                                if (villager.getTargetEnemyId() == b.getId() || villager.getId() == a.getId()) {
                                    villager.setTargetEnemyId(-1);
                                    villager.cambiarAccion(HumanAction::SALUDAR);
                                    villager.triggerFaithIndicator(+1, 2.0f);
                                    villager.getHumanUnitMutable().currentState = HumanAIState::SOCIALIZING;
                                }
                            }
                            if (b.esChaman()) {
                                int rId = b.getReligionId();
                                for (auto& cand : m_villagers) {
                                    if (cand.getReligionId() == rId && cand.getSalud() > 0.0f && !cand.esChaman()) {
                                        cand.setEsChaman(true);
                                        cand.setNombre("Chaman " + cand.getNombre());
                                        cand.triggerFaithIndicator(+1, 2.0f);
                                        break;
                                    }
                                }
                            }
                        }
                    }

                    // Golpe de B hacia A
                    if (b.getAccion() == HumanAction::ATACAR && b.getAttackCooldown() <= 0.0f && a.estaVivo()) {
                        b.orientarHacia(a.getX(), a.getY());
                        b.setAttackCooldown(b.esFanatico() ? 0.60f : 0.85f);
                        float atkPowerB = b.getHumanUnit().attackPower * (b.esFanatico() ? 1.40f : 1.0f);
                        if (b.tieneFuriaProtectora()) atkPowerB *= 1.35f; // +35% bono por defender a sus seres queridos

                        // EN: Modulate attack power by active physiological/psychological condition
                        // ES: Modular potencia de ataque según condición fisiológica/psicológica activa
                        const VillagerCondition conditionB = b.getCondition();
                        switch (conditionB) {
                            case VillagerCondition::Aggressive: atkPowerB *= 1.35f; break; // +35% furia combativa
                            case VillagerCondition::Inspired:   atkPowerB *= 1.20f; break; // +20% éxtasis sagrado
                            case VillagerCondition::Injured:    atkPowerB *= 0.75f; break; // -25% debilitado por heridas
                            case VillagerCondition::Drunk:      atkPowerB *= 0.70f; break; // -30% torpeza por ebriedad
                            case VillagerCondition::Sick:       atkPowerB *= 0.65f; break; // -35% debilidad febril
                            case VillagerCondition::Hungry:     atkPowerB *= 0.80f; break; // -20% desfallecimiento por hambre
                            case VillagerCondition::Exhausted:  atkPowerB *= 0.60f; break; // -40% fatiga extrema
                            default:                            break;
                        }

                        float resA = a.getHumanUnit().resistance;
                        float baseDmgB = std::max(6.0f, atkPowerB * 0.85f - resA * 0.15f);
                        a.recibirDano(baseDmgB);

                        // Alarma de Furia Protectora para la familia de A (despertar y correr al rescate)
                        for (auto& kin : m_villagers) {
                            if (&kin == &a || !kin.estaVivo() || kin.getReligionId() != a.getReligionId()) continue;
                            bool esFamiliar = (kin.getParejaId() == a.getId());
                            if (kin.getId() == a.getMadreId() || kin.getId() == a.getPadreId()) esFamiliar = true;
                            if (a.getMadreId() == kin.getId() || a.getPadreId() == kin.getId()) esFamiliar = true;
                            for (int hid : kin.getHijosIds()) { if (hid == a.getId()) { esFamiliar = true; break; } }
                            for (int hid : a.getHijosIds()) { if (hid == kin.getId()) { esFamiliar = true; break; } }

                            if (esFamiliar) {
                                float kdx = kin.getX() - a.getX();
                                float kdy = kin.getY() - a.getY();
                                if (kdx * kdx + kdy * kdy <= 64.0f) { // Radio de 8 celdas
                                    kin.activarFuriaProtectora(7.5f);
                                    kin.setTargetProtegidoId(a.getId());
                                    kin.setTargetEnemyId(b.getId());
                                    kin.getHumanUnitMutable().currentState = HumanAIState::DEFENDING;
                                    kin.caminarHacia(b.getX(), b.getY(), true);
                                }
                            }
                        }

                        if (soundSystem && camera) {
                            soundSystem->playSpatialSFX(SFXType::CombatAttack, a.getX(), a.getY(), *camera, 0.80f);
                        }
                        if (nemesisSififus && a.estaVivo()) {
                            nemesisSififus->onUnitSurvivesCombat(a, b.getId(), static_cast<int16_t>(b.getReligionId()),
                                                                baseDmgB, currentYear, eventLine, registry);
                        }
                        if (moralitySystem) {
                            if (b.getReligionId() == a.getReligionId()) {
                                if (moralitySystem->isRuleActive(b.getReligionId(), MoralRuleType::SacredKinProtection)) {
                                    moralitySystem->recordTransgression(b, MoralRuleType::SacredKinProtection,
                                                                        "Derramamiento de sangre contra hermano de fe " + a.getNombre(),
                                                                        currentYear, eventLine, registry);
                                }
                            } else {
                                if (moralitySystem->isRuleActive(b.getReligionId(), MoralRuleType::ZealousHolyWar)) {
                                    moralitySystem->recordRighteousAct(b, MoralRuleType::ZealousHolyWar, 12.0f, currentYear, registry);
                                }
                                int cbx = std::clamp(b.getCeldaX(), 0, w - 1);
                                int cby = std::clamp(b.getCeldaY(), 0, h - 1);
                                const Cell& cellB = mapa.getCell(cbx, cby);
                                if (cellB.hasHolySite && (cellB.holySiteReligionId == b.getReligionId()) &&
                                    moralitySystem->isRuleActive(b.getReligionId(), MoralRuleType::SanctuaryDefense)) {
                                    moralitySystem->recordRighteousAct(b, MoralRuleType::SanctuaryDefense, 18.0f, currentYear, registry);
                                }
                            }
                        }

                        // Si A perece en combate
                        if (a.getSalud() <= 0.0f) {
                            if (soundSystem && camera) {
                                soundSystem->playSpatialSFX(SFXType::UnitDeath, a.getX(), a.getY(), *camera, 0.85f);
                            }
                            a.matar("Caído en batalla por la defensa de su fe ante " + b.getNombre());
                            for (auto& villager : m_villagers) {
                                if (villager.getTargetEnemyId() == a.getId() || villager.getId() == b.getId()) {
                                    villager.setTargetEnemyId(-1);
                                    villager.cambiarAccion(HumanAction::SALUDAR);
                                    villager.triggerFaithIndicator(+1, 2.0f);
                                    villager.getHumanUnitMutable().currentState = HumanAIState::SOCIALIZING;
                                }
                            }
                            if (a.esChaman()) {
                                int rId = a.getReligionId();
                                for (auto& cand : m_villagers) {
                                    if (cand.getReligionId() == rId && cand.getSalud() > 0.0f && !cand.esChaman()) {
                                        cand.setEsChaman(true);
                                        cand.setNombre("Chaman " + cand.getNombre());
                                        cand.triggerFaithIndicator(+1, 2.0f);
                                        break;
                                    }
                                }
                            }
                        }
                    }
                }

                // B. Conversión e influencia espiritual pacífica directa (sólo entre vivos)
                if (a.estaVivo() && b.estaVivo()) {
                    float radioMax = (a.esChaman() || b.esChaman()) ? 2.6f : 1.8f;
                    if (dsq <= radioMax * radioMax) {
                        float multA = a.esChaman() ? (a.getTimerSenalDivina() > 0.0f ? 2.6f : 1.75f)
                                                   : (a.getAccion() == HumanAction::REZAR ? 1.45f : 1.0f);
                        float multB = b.esChaman() ? (b.getTimerSenalDivina() > 0.0f ? 2.6f : 1.75f)
                                                   : (b.getAccion() == HumanAction::REZAR ? 1.45f : 1.0f);
                        float convFactorA = 0.7f + (a.getHumanUnit().conversionPower / 40.0f);
                        float convFactorB = 0.7f + (b.getHumanUnit().conversionPower / 40.0f);
                        float pesoA = a.getDevocion() * multA * convFactorA;
                        float pesoB = b.getDevocion() * multB * convFactorB;

                        if (pesoA >= pesoB && !b.esChaman() && !b.esProfeta() && !b.esFanatico()) {
                            float resistB = std::clamp(b.getHumanUnit().conversionInertia / 120.0f, 0.15f, 0.75f);
                            float nuevaDevB = b.getDevocion() - 0.16f * dt * pesoA * (1.0f - resistB);
                            if (nuevaDevB < 0.35f) {
                                b.setReligionId(a.getReligionId());
                                b.setDevocion(0.68f);
                                b.setLeaderDevotion(0.50f);
                                b.triggerFaithIndicator(+1, 1.5f);
                            } else {
                                b.setDevocion(nuevaDevB);
                            }
                        } else if (pesoB > pesoA && !a.esChaman() && !a.esProfeta() && !a.esFanatico()) {
                            float resistA = std::clamp(a.getHumanUnit().conversionInertia / 120.0f, 0.15f, 0.75f);
                            float nuevaDevA = a.getDevocion() - 0.16f * dt * pesoB * (1.0f - resistA);
                            if (nuevaDevA < 0.35f) {
                                a.setReligionId(b.getReligionId());
                                a.setDevocion(0.68f);
                                a.setLeaderDevotion(0.50f);
                                a.triggerFaithIndicator(+1, 1.5f);
                            } else {
                                a.setDevocion(nuevaDevA);
                            }
                        }
                    }
                }
            }
        }

        // C. Transmisión epidemiológica de Pandemia Viral (Vector de contagio biológico humano a humano)
        for (size_t i = 0; i < n; ++i) {
            Humano& a = m_villagers[i];
            if (!a.isInfected() || !a.estaVivo()) continue;

            for (size_t j = 0; j < n; ++j) {
                if (i == j) continue;
                Humano& b = m_villagers[j];
                if (!b.estaVivo() || b.isInfected()) continue;

                float dx = a.getX() - b.getX();
                float dy = a.getY() - b.getY();
                float distSq = dx * dx + dy * dy;

                if (distSq <= 2.25f) { // Radio ~1.5 celdas
                    // Contagio biológico por cercanía / aerosol viral
                    b.infectarConPandemia(16.0f);
                    // Aldeanos sanos huyen del foco de infección si detectan la peste
                    if (b.getPanicTimer() <= 0.0f) {
                        b.huirEnPanicoDesde(a.getX(), a.getY(), 3.5f);
                    }
                }
            }
        }

        // 3. Retirar del mundo a las unidades fallecidas cuya animación de muerte haya concluido
        m_villagers.erase(
            std::remove_if(m_villagers.begin(), m_villagers.end(), [this](const Humano& h) {
                if (h.getSalud() <= 0.0f && h.getTimerMuerte() <= 0.0f) {
                    if (h.getId() == m_selectedVillagerId) {
                        m_selectedVillagerId = -1;
                    }
                    return true;
                }
                return false;
            }),
            m_villagers.end()
        );

        // 4. Sistema de Afecto, Cortejo y Procreación ("cuando dos humanos se quieren podran tener hijos")
        // A. Cortejo y fortalecimiento del vínculo amoroso entre aldeanos adultos de sexos opuestos
        for (size_t i = 0; i < n; ++i) {
            Humano& a = m_villagers[i];
            if (!a.estaVivo() || a.esNino()) continue;

            // Comprobación de viudez: si la pareja falleció, el dolor se disipa y con el tiempo puede amar de nuevo
            if (a.tienePareja()) {
                bool parejaViva = false;
                for (size_t k = 0; k < n; ++k) {
                    if (m_villagers[k].getId() == a.getParejaId() && m_villagers[k].estaVivo()) {
                        parejaViva = true;
                        break;
                    }
                }
                if (!parejaViva) {
                    a.modificarAmor(-0.02f * dt);
                    if (a.getAmorHaciaPareja() <= 0.10f) {
                        a.setParejaId(-1);
                        a.setAmorHaciaPareja(0.0f);
                    }
                }
            }

            for (size_t j = i + 1; j < n; ++j) {
                Humano& b = m_villagers[j];
                if (!b.estaVivo() || b.esNino()) continue;

                // Solo hombre y mujer se cortejan y conciben
                if (a.esHombre() == b.esHombre()) continue;

                float dx = a.getX() - b.getX();
                float dy = a.getY() - b.getY();
                float distSq = dx * dx + dy * dy;

                if (distSq <= 6.5f) { // Proximidad comunitaria (~2.5 celdas)
                    // Caso 1: Ambos solteros de la misma fe o en paz comunitaria -> Cortejo
                    if (!a.tienePareja() && !b.tienePareja() && a.getReligionId() == b.getReligionId()) {
                        a.modificarAmor(0.06f * dt);
                        b.modificarAmor(0.06f * dt);

                        // Si el afecto mutuo supera el 60%, formalizan su unión amorosa
                        if (a.getAmorHaciaPareja() >= 0.60f && b.getAmorHaciaPareja() >= 0.60f) {
                            a.setParejaId(b.getId());
                            b.setParejaId(a.getId());
                            a.setAmorHaciaPareja(0.75f);
                            b.setAmorHaciaPareja(0.75f);

                            if (eventLine) {
                                std::string hombreNom = a.esHombre() ? a.getNombre() : b.getNombre();
                                std::string mujerNom = a.esMujer() ? a.getNombre() : b.getNombre();
                                GameEvent ev;
                                ev.year = currentYear;
                                ev.category = EventCategory::ExogamicPact;
                                ev.title = "Union Sagrada";
                                ev.description = hombreNom + " y " + mujerNom + " se han unido en amor conyugal y devocion mutua.";
                                ev.actorName = hombreNom;
                                eventLine->addEvent(ev);
                            }
                        }
                    }
                    // Caso 2: Son cónyuges -> Nutren su amor y pueden concebir
                    else if (a.getParejaId() == b.getId() && b.getParejaId() == a.getId()) {
                        a.modificarAmor(0.02f * dt);
                        b.modificarAmor(0.02f * dt);

                        // Probabilidad de concepción sujeta a factores ambientales, habitacionales y espirituales
                        const uint32_t maxSafePopulation = ErrorSystem::instance().getMaxSafeEntities();
                        if (!ErrorSystem::instance().isPopulationCapReached() && m_villagers.size() < maxSafePopulation) {
                            Humano& madre = a.esMujer() ? a : b;
                            Humano& padre = a.esHombre() ? a : b;

                            if (!madre.estaEmbarazada() && madre.getFertilidadCooldown() <= 0.0f && padre.getFertilidadCooldown() <= 0.0f) {
                                float probConcepcion = madre.calcularDeseoReproduccion(padre, mapa, buildingSystem);
                                std::uniform_real_distribution<float> rollProb(0.0f, 1.0f);

                                if (rollProb(g_humanoRng) < probConcepcion) {
                                    madre.setEmbarazada(true);
                                    madre.setEmbarazoProgreso(0.0f);
                                    madre.setFertilidadCooldown(48.0f);
                                    padre.setFertilidadCooldown(30.0f);

                                    if (eventLine) {
                                        GameEvent ev;
                                        ev.year = currentYear;
                                        ev.category = EventCategory::ExogamicPact;
                                        ev.title = "Esperanza de Vida";
                                        std::string contexto = "";
                                        int hId = madre.getHomeId() > 0 ? madre.getHomeId() : padre.getHomeId();
                                        if (buildingSystem && hId > 0) {
                                            const House* h = buildingSystem->getHouseById(hId);
                                            if (h && h->isCompleted() && h->hasSpace()) {
                                                contexto = " bajo el abrigo de su " + std::string(h->getTypeName());
                                            }
                                        }
                                        if (mapa.isValid(madre.getCeldaX(), madre.getCeldaY()) && mapa.getCell(madre.getCeldaX(), madre.getCeldaY()).hasHolySite) {
                                            contexto += (contexto.empty() ? " en" : " y") + std::string(" suelo sagrado");
                                        }
                                        ev.description = madre.getNombre() + " y " + padre.getNombre() + " esperan un nuevo hijo de la fe" + contexto + ".";
                                        ev.actorName = madre.getNombre();
                                        eventLine->addEvent(ev);
                                    }
                                    if (soundSystem && camera) {
                                        soundSystem->playSpatialSFX(SFXType::PrayerChant, madre.getX(), madre.getY(),
                                                                   *camera, 0.65f);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        // B. Nacimiento de recién nacidos tras completar el periodo de gestación
        // EN: Postpone births if safety population cap is active to prevent memory overflow and lag
        // ES: Posponer nacimientos si el tope de población está activo para evitar desbordes y lag
        if (ErrorSystem::instance().isPopulationCapReached()) {
            return;
        }

        std::vector<Humano> recienNacidos;
        for (auto& hObj : m_villagers) {
            if (hObj.estaVivo() && hObj.esMujer() && hObj.estaEmbarazada() && hObj.getEmbarazoProgreso() >= 1.0f) {
                hObj.setEmbarazada(false);
                hObj.setEmbarazoProgreso(0.0f);
                int nid = m_nextVillagerId++;
                hObj.incrementarHijos();
                hObj.registrarHijo(nid);

                Humano* padrePtr = nullptr;
                if (hObj.getParejaId() > 0) {
                    for (auto& candid : m_villagers) {
                        if (candid.getId() == hObj.getParejaId() && candid.estaVivo()) {
                            padrePtr = &candid;
                            padrePtr->incrementarHijos();
                            padrePtr->registrarHijo(nid);
                            break;
                        }
                    }
                }

                std::uniform_real_distribution<float> fDist(0.0f, 1.0f);
                GeneroHumano hijoGenero = (fDist(g_humanoRng) >= 0.5f) ? HumanGender::MUJER : HumanGender::HOMBRE;
                std::string nomBebe = Humano::generarNombre(hijoGenero, false);
                float nx = hObj.getX() + (fDist(g_humanoRng) - 0.5f) * 0.9f;
                float ny = hObj.getY() + (fDist(g_humanoRng) - 0.5f) * 0.9f;

                Humano bebe(nid, nomBebe, nx, ny, hijoGenero);
                bebe.setReligionId(hObj.getReligionId());
                bebe.setDevocion(std::clamp(hObj.getDevocion(), 0.70f, 0.98f));
                bebe.setLeaderDevotion(hObj.getLeaderDevotion());
                bebe.setEsNino(true);
                bebe.setEdad(0.0f);
                bebe.setMadreId(hObj.getId());
                bebe.setPadreId(padrePtr ? padrePtr->getId() : -1);
                bebe.setSueno(15.0f);
                bebe.setReproductionDrive(0.0f);

                // EN: Parents satisfy reproductive urge upon successful birth
                // ES: Los progenitores satisfacen su impulso reproductivo tras el nacimiento
                hObj.setReproductionDrive(0.05f);
                if (padrePtr) {
                    padrePtr->setReproductionDrive(0.05f);
                }

                // Asignar al hogar familiar si los padres tienen choza y HAY ESPACIO
                if (buildingSystem) {
                    int hId = hObj.getHomeId();
                    if (hId <= 0 && padrePtr) hId = padrePtr->getHomeId();
                    if (hId > 0) {
                        House* h = const_cast<BuildingSystem*>(buildingSystem)->getHouseByIdMutable(hId);
                        if (h && h->hasSpace()) {
                            h->addResident(nid);
                            bebe.setHomeId(h->id);
                        } else {
                            // Choza al límite estricto de capacidad: el infante no cabe y la familia busca nuevo hogar
                            bebe.setHomeId(-1);
                            hObj.modificarNeedForShelter(0.40f);
                            if (padrePtr) padrePtr->modificarNeedForShelter(0.40f);
                        }
                    }
                }

                bebe.triggerFaithIndicator(+1, 2.0f);

                // Acto virtuoso para los padres en el sistema moral
                hObj.getMoralRecordMutable().addRighteousAct(0.06f);
                if (padrePtr) {
                    padrePtr->getMoralRecordMutable().addRighteousAct(0.06f);
                }

                if (eventLine) {
                    std::string padreNom = padrePtr ? padrePtr->getNombre() : "el clan";
                    GameEvent ev;
                    ev.year = currentYear;
                    ev.category = EventCategory::BirthOfReligion;
                    ev.title = "Nacimiento Sagrado";
                    ev.description = "Ha nacido " + nomBebe + "! Retono de " + padreNom + " y " + hObj.getNombre() + " bajo la fe.";
                    ev.actorName = nomBebe;
                    eventLine->addEvent(ev);
                }
                if (soundSystem && camera) {
                    soundSystem->playSpatialSFX(SFXType::FaithGain, hObj.getX(), hObj.getY(),
                                               *camera, 1.15f);
                }

                recienNacidos.push_back(bebe);
            }
        }

        if (!recienNacidos.empty()) {
            m_villagers.insert(m_villagers.end(), recienNacidos.begin(), recienNacidos.end());
        }

        // C. Ontogenia, Maduración e Iniciación de Nuevos Adultos
        // EN: Check maturation of children into biological adults (Ontogeny & Puberty Rites)
        // ES: Comprobar maduración de infantes a adultos biológicos (Ontogenia y Rito de Pubertad)
        for (auto& matureCandidate : m_villagers) {
            if (matureCandidate.estaVivo() && matureCandidate.esNino() && matureCandidate.getAge() >= 12.0f) {
                matureCandidate.setChild(false);
                matureCandidate.setReproductionDrive(0.35f); // Emergencia del impulso reproductivo adulto
                matureCandidate.getHumanUnitMutable().energy = 100.0f;
                matureCandidate.triggerFaithIndicator(+1, 2.0f);

                if (eventLine) {
                    GameEvent maturityEvent;
                    maturityEvent.year = currentYear;
                    maturityEvent.category = EventCategory::Discovery;
                    maturityEvent.title = "Rito de Madurez";
                    maturityEvent.description = matureCandidate.getName() + " ha alcanzado la madurez biologica y se incorpora como adulto fecundo al clan.";
                    maturityEvent.actorName = matureCandidate.getName();
                    eventLine->addEvent(maturityEvent);
                }
            }
        }

        // D. Inversión Parental: Alimentación y Cuidado de Crías
        // EN: Parental investment: living parents share food with hungry offspring
        // ES: Inversión parental: progenitores vivos comparten sustento con crías hambrientas
        for (auto& parentUnit : m_villagers) {
            if (!parentUnit.estaVivo() || parentUnit.esNino() || parentUnit.getChildrenCount() == 0) continue;
            if (parentUnit.getParentalCareTimer() > 0.0f) continue;

            for (int childUnitId : parentUnit.getChildrenIds()) {
                for (auto& targetChild : m_villagers) {
                    if (targetChild.getId() == childUnitId && targetChild.estaVivo() && targetChild.esNino()) {
                        if (targetChild.getHumanUnit().nutritionalState < 45.0f) {
                            const float deltaCoordinateX = parentUnit.getX() - targetChild.getX();
                            const float deltaCoordinateY = parentUnit.getY() - targetChild.getY();
                            const float distanceSquared = deltaCoordinateX * deltaCoordinateX + deltaCoordinateY * deltaCoordinateY;
                            if (distanceSquared <= 6.25f) { // ~2.5 celdas
                                parentUnit.feedChild(targetChild);
                                break;
                            }
                        }
                    }
                }
            }
        }

        // E. Replicación Cultural y Memética de la Fe
        // EN: Cultural and Memetic Replication: liturgical prayer transmits dogma and fervor to nearby clan members
        // ES: Replicación cultural y memética: la oración litúrgica transmite dogma y fervor a miembros cercanos del clan
        for (auto& liturgicalUnit : m_villagers) {
            if (!liturgicalUnit.estaVivo() || liturgicalUnit.getAccion() != HumanAction::REZAR) continue;

            const float prestigeMultiplier = liturgicalUnit.esLiderEspiritual() ? 1.8f : (liturgicalUnit.esFanatico() ? 1.4f : 1.0f);
            const float resonanceRate = 0.08f * prestigeMultiplier * dt;

            for (auto& congregationUnit : m_villagers) {
                if (&congregationUnit == &liturgicalUnit || !congregationUnit.estaVivo()) continue;
                if (congregationUnit.getReligionId() != liturgicalUnit.getReligionId()) continue;

                const float deltaDistanceX = liturgicalUnit.getX() - congregationUnit.getX();
                const float deltaDistanceY = liturgicalUnit.getY() - congregationUnit.getY();
                const float prayerDistanceSquared = deltaDistanceX * deltaDistanceX + deltaDistanceY * deltaDistanceY;

                if (prayerDistanceSquared <= 12.25f) { // ~3.5 celdas
                    // EN: High-devotion units elevate lower-devotion units (Memetic resonance)
                    // ES: Unidades de alta devoción elevan a las de menor devoción (Resonancia memética)
                    if (liturgicalUnit.getDevocion() > congregationUnit.getDevocion()) {
                        congregationUnit.setDevocion(congregationUnit.getDevocion() + resonanceRate);
                        congregationUnit.getHumanUnitMutable().devotion = congregationUnit.getDevocion() * 100.0f;
                    }
                    // EN: Children in liturgical presence learn and absorb the faith
                    // ES: Los niños en presencia litúrgica aprenden y absorben la fe
                    if (congregationUnit.esNino()) {
                        congregationUnit.setDevocion(std::min(1.0f, congregationUnit.getDevocion() + resonanceRate * 1.5f));
                        if (congregationUnit.getAccion() == HumanAction::IDLE && prayerDistanceSquared <= 4.0f) {
                            congregationUnit.cambiarAccion(HumanAction::REZAR);
                            congregationUnit.orientarHacia(liturgicalUnit.getX(), liturgicalUnit.getY());
                        }
                    }
                }
            }
        }
    }

    // EN: 2. Clear all map cells: influence zones only exist where living villagers reside
    // ES: 2. Limpiar todas las celdas: las zonas de influencia sólo existen donde hay aldeanos
    const int mapWidth = w;
    const int mapHeight = h;
    for (int cellY = 0; cellY < mapHeight; ++cellY) {
        for (int cellX = 0; cellX < mapWidth; ++cellX) {
            Cell& targetCell = mapa.getCell(cellX, cellY);
            targetCell.population = 0;
            targetCell.faithShare.clear();
            targetCell.dominantReligionId = 0;
            targetCell.dominantFraction = 0.0f;
            targetCell.syncreticTension = 0.0f;
        }
    }

    // EN: 3. Project faith exclusively from living villagers on the cell they occupy
    // ES: 3. Proyectar la fe exclusivamente de los aldeanos vivos en la celda que ocupan
    for (const auto& livingVillager : m_villagers) {
        if (!livingVillager.estaVivo()) continue;
        const int cellX = livingVillager.getCeldaX();
        const int cellY = livingVillager.getCeldaY();
        if (mapa.isValid(cellX, cellY)) {
            Cell& occupiedCell = mapa.getCell(cellX, cellY);
            occupiedCell.population++;
            if (occupiedCell.isHabitable() && livingVillager.getReligionId() > 0) {
                occupiedCell.addShare(livingVillager.getReligionId(), std::max(0.25f, livingVillager.getDevocion()));
            }
        }
    }

    // EN: 4. Normalize shares and dominance only in populated cells
    // ES: 4. Normalizar participación y dominancia sólo en celdas con aldeanos
    for (int cellY = 0; cellY < mapHeight; ++cellY) {
        for (int cellX = 0; cellX < mapWidth; ++cellX) {
            Cell& targetCell = mapa.getCell(cellX, cellY);
            if (targetCell.population > 0) {
                targetCell.normalizeShares();
                targetCell.updateDominance();
                if (targetCell.faithShare.size() >= 2) {
                    targetCell.syncreticTension = 0.65f;
                }
            }
        }
    }
}

void GestorHumanos::renderAll(uint32_t* dstBuffer, int bufferWidth, int bufferHeight,
                                   const IsometricCamera& camara, const WorldMap& mapa,
                                   const TileSet& tileset, const ReligionRegistry& registry) const {
    const int dstW = bufferWidth;
    const int dstH = bufferHeight;
    // Ordenar humanos por profundidad isométrica (X + Y)
    std::vector<const Humano*> ordenados;
    ordenados.reserve(m_villagers.size());
    for (const auto& h : m_villagers) {
        ordenados.push_back(&h);
    }

    std::sort(ordenados.begin(), ordenados.end(), [](const Humano* a, const Humano* b) {
        return (a->getX() + a->getY()) < (b->getX() + b->getY());
    });

    for (const auto* h : ordenados) {
        ColorRGB aura(255, 215, 0);
        const Religion* rel = registry.getReligion(h->getReligionId());
        if (rel) aura = rel->color;

        bool esSeleccionado = (h->getId() == m_selectedVillagerId);
        h->renderizar(dstBuffer, dstW, dstH, camara, mapa, tileset, aura, esSeleccionado);
    }
}

void GestorHumanos::addVillager(const Humano& villagerRef) {
    m_villagers.push_back(villagerRef);
}

const Humano* GestorHumanos::getSelectedVillager() const {
    if (m_selectedVillagerId < 0) return nullptr;
    for (const auto& h : m_villagers) {
        if (h.getId() == m_selectedVillagerId) return &h;
    }
    return nullptr;
}

Humano* GestorHumanos::getSelectedVillagerMutable() {
    if (m_selectedVillagerId < 0) return nullptr;
    for (auto& h : m_villagers) {
        if (h.getId() == m_selectedVillagerId) return &h;
    }
    return nullptr;
}

void GestorHumanos::selectVillager(int villagerId) {
    m_selectedVillagerId = villagerId;
}

int GestorHumanos::findShamanOfReligion(int religionId) const {
    return buscarLiderEspiritualDeReligion(religionId);
}

int GestorHumanos::findSpiritualLeaderOfReligion(int religionId) const {
    for (const auto& h : m_villagers) {
        if (h.estaVivo() && h.getReligionId() == religionId && h.esLiderEspiritual()) {
            return h.getId();
        }
    }
    return -1;
}

void GestorHumanos::executeDivineSign(int signType, WorldMap& mapRef, ReligionRegistry& registryRef,
                                        EventLine& eventLine, int currentYear) {
    const int tipoSenal = signType;
    WorldMap& mapa = mapRef;
    ReligionRegistry& registry = registryRef;
    Humano* chaman = getHumanoSeleccionadoMutable();
    if (!chaman || !chaman->esChaman()) return;

    int relId = chaman->getReligionId();
    Religion* rel = registry.getReligion(relId);
    ColorRGB rCol = rel ? rel->color : Palette::DIVINE_GOLD;
    std::string relName = rel ? rel->name : "Fe Ancestral";

    chaman->activarSenalDivina(6.0f);

    if (tipoSenal == 1) {
        // 1. REVELAR VISIÓN: Comunión profética con el Chamán, exalta devoción de su pueblo
        for (auto& h : m_villagers) {
            if (h.getReligionId() == relId) {
                h.setDevocion(std::min(1.0f, h.getDevocion() + 0.25f));
                if (&h != chaman) {
                    h.cambiarAccion(HumanAction::REZAR);
                }
            }
        }
        if (rel) rel->theologyPoints += 80;
        eventLine.addEvent(
            currentYear, EventCategory::PilgrimageWave,
            "Vision a " + chaman->getNombre(),
            "Te has comunicado con " + chaman->getNombre() + ". El pueblo de " + relName + " entra en fervor mistico.",
            {chaman->getCeldaX(), chaman->getCeldaY()}, relId, 0, rCol
        );
    } else if (tipoSenal == 2) {
        // 2. LLAMAR PUEBLO: Congrega a los fieles alrededor del Chamán para cerrar la triangulación de fe
        int idx = 0;
        for (auto& h : m_villagers) {
            if (&h == chaman) continue;
            float dx = h.getX() - chaman->getX();
            float dy = h.getY() - chaman->getY();
            float dsq = dx * dx + dy * dy;
            if (h.getReligionId() == relId || dsq <= 36.0f) {
                float ang = static_cast<float>(idx++) * 1.15f;
                float rad = 1.35f + static_cast<float>(idx % 3) * 0.65f;
                int tx = std::clamp(static_cast<int>(std::round(chaman->getX() + std::cos(ang) * rad)), 2, mapa.getWidth() - 3);
                int ty = std::clamp(static_cast<int>(std::round(chaman->getY() + std::sin(ang) * rad)), 2, mapa.getHeight() - 3);
                if (mapa.getCell(tx, ty).isHabitable()) {
                    h.caminarHacia(static_cast<float>(tx) + 0.5f, static_cast<float>(ty) + 0.5f, true);
                }
            }
        }
        eventLine.addEvent(
            currentYear, EventCategory::PilgrimageWave,
            "Llamado de " + chaman->getNombre(),
            chaman->getNombre() + " transmite tu senal divina convocando a los nodos de " + relName + ".",
            {chaman->getCeldaX(), chaman->getCeldaY()}, relId, 0, rCol
        );
    } else if (tipoSenal == 3) {
        // 3. MILAGRO DE FE: Pulso divino que convierte aldeanos cercanos al credo del Chamán
        chaman->cambiarAccion(HumanAction::REZAR);
        int convertidos = 0;
        for (auto& h : m_villagers) {
            if (&h == chaman || h.esChaman()) continue;
            float dx = h.getX() - chaman->getX();
            float dy = h.getY() - chaman->getY();
            if (dx * dx + dy * dy <= 32.0f) { // Radio ~ 5.6 celdas
                if (h.getReligionId() != relId) {
                    h.setReligionId(relId);
                    convertidos++;
                }
                h.setDevocion(0.95f);
                h.cambiarAccion(HumanAction::SALUDAR);
            }
        }
        if (rel) rel->theologyPoints += 120;
        eventLine.addEvent(
            currentYear, EventCategory::BirthOfReligion,
            "Milagro de " + chaman->getNombre(),
            "Una senal divina a traves de " + chaman->getNombre() + " ilumino la tierra (" +
            std::to_string(convertidos) + " nodos convertidos).",
            {chaman->getCeldaX(), chaman->getCeldaY()}, relId, 0, rCol
        );
    }

    actualizar(0.0f, mapa);
    mapa.recalculateReligions(registry);
}


int GestorHumanos::findNearbyVillager(float worldPosX, float worldPosY, float maxRadius, int onlyReligionId) const {
    int bestVillagerId = -1;
    float bestDistanceSquared = maxRadius * maxRadius;

    for (const auto& candidateVillager : m_villagers) {
        if (onlyReligionId > 0 && candidateVillager.getReligionId() != onlyReligionId) continue;
        const float deltaPosX = candidateVillager.getX() - worldPosX;
        const float deltaPosY = candidateVillager.getY() - worldPosY;
        const float distanceSquared = deltaPosX * deltaPosX + deltaPosY * deltaPosY;
        if (distanceSquared < bestDistanceSquared) {
            bestDistanceSquared = distanceSquared;
            bestVillagerId = candidateVillager.getId();
        }
    }
    return bestVillagerId;
}

int GestorHumanos::findVillagerOnScreen(float screenPosX, float screenPosY,
                                         const IsometricCamera& cameraRef, const WorldMap& mapRef,
                                         int onlyReligionId) const {
    int bestVillagerId = -1;
    float bestDistanceSquared = 1e9f;

    for (const auto& candidateVillager : m_villagers) {
        if (onlyReligionId > 0 && candidateVillager.getReligionId() != onlyReligionId) continue;
        const float baseScaleFactor = candidateVillager.esChaman() ? 0.54f : 0.48f;
        const float renderScale = std::clamp(cameraRef.getZoom() * baseScaleFactor, 0.20f, 1.35f);
        const float destWidth = 64.0f * renderScale;
        const float destHeight = 64.0f * renderScale;

        float cellElevation = 1.0f;
        if (mapRef.isValid(candidateVillager.getCeldaX(), candidateVillager.getCeldaY())) {
            cellElevation = mapRef.getCell(candidateVillager.getCeldaX(), candidateVillager.getCeldaY()).elevation;
        }
        const Vec2 footScreenPos = cameraRef.gridToScreen(candidateVillager.getX(), candidateVillager.getY(), cellElevation);

        const float minScreenX = footScreenPos.x - destWidth * 0.5f - 8.0f;
        const float maxScreenX = footScreenPos.x + destWidth * 0.5f + 8.0f;
        const float minScreenY = footScreenPos.y - destHeight - 8.0f;
        const float maxScreenY = footScreenPos.y + 8.0f;

        if (screenPosX >= minScreenX && screenPosX <= maxScreenX && screenPosY >= minScreenY && screenPosY <= maxScreenY) {
            const float centerScreenX = footScreenPos.x;
            const float centerScreenY = footScreenPos.y - destHeight * 0.5f;
            const float deltaPosX = screenPosX - centerScreenX;
            const float deltaPosY = screenPosY - centerScreenY;
            const float distanceSquared = deltaPosX * deltaPosX + deltaPosY * deltaPosY;
            if (distanceSquared < bestDistanceSquared) {
                bestDistanceSquared = distanceSquared;
                bestVillagerId = candidateVillager.getId();
            }
        }
    }
    return bestVillagerId;
}

} // namespace ReligionSim
