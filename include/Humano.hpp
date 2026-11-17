#pragma once

#include "Types.hpp"
#include "TileSet.hpp"
#include "FirstEraAIController.hpp"
#include "HumanAIController.hpp"
#include "RagdollSystem.hpp"
#include "ReligiousMoralitySystem.hpp"
#include <string>
#include <vector>
#include <cmath>
#include <algorithm>

namespace ReligionSim {

class WorldMap;
class ReligionRegistry;
class IsometricCamera;
class EventLine;
class SoundSystem;
class NemesisSififusManager;
class ReligiousMoralitySystem;
class DayNightCycle;
class BuildingSystem;

// EN: Biological human gender enumeration
// ES: Enumeración biológica de género humano
enum class HumanGender : uint8_t {
    Male = 0,
    Female = 1,
    HOMBRE = Male,
    MUJER = Female
};
using GeneroHumano = HumanGender;

// EN: Physiological, psychological, and behavioral conditions of villagers
// ES: Condiciones fisiológicas, psicológicas y conductuales de los aldeanos
enum class VillagerCondition : uint16_t {
    Healthy = 0,    // EN: Healthy / ES: Sano (Equilibrio vital óptimo, buen desempeño)
    Hungry = 1,     // EN: Hungry / ES: Hambriento (Desnutrición, debilidad, busca comida urgente)
    Full = 2,       // EN: Full / ES: Lleno (Saciado, bonus de energía, altruismo y reproducción)
    Sick = 3,       // EN: Sick / ES: Enfermo (Infección/malestar, tose, busca alivio o cura)
    Drunk = 4,      // EN: Drunk / ES: Borracho (Embriagado por fermentos/vino, tambaleo y euforia festiva)
    Aggressive = 5, // EN: Aggressive / ES: Agresivo (Beligerancia, furia combativa, ataca con furia)
    Scared = 6,     // EN: Scared / ES: Asustado (Pánico ante cataclismos o amenazas mortales, huida)
    Exhausted = 7,  // EN: Exhausted / ES: Exhausto (Fatiga extrema, lentitud severa, busca dormir)
    Inspired = 8,   // EN: Inspired / ES: Inspirado (Éxtasis sagrado, bendición divina, alta productividad)
    Injured = 9     // EN: Injured / ES: Herido (Salud crítica, renguera, busca auxilio)
};
using EstadoAldeano = VillagerCondition;

// EN: Convert villager condition enum to readable localized text
// ES: Convertir enumeración de condición de aldeano a texto legible localizado
inline const char* getVillagerConditionName(VillagerCondition condition, bool inSpanish = true) {
    if (inSpanish) {
        switch (condition) {
            case VillagerCondition::Healthy:    return "Sano";
            case VillagerCondition::Hungry:     return "Hambriento";
            case VillagerCondition::Full:       return "Lleno";
            case VillagerCondition::Sick:       return "Enfermo";
            case VillagerCondition::Drunk:      return "Borracho";
            case VillagerCondition::Aggressive: return "Agresivo";
            case VillagerCondition::Scared:     return "Asustado";
            case VillagerCondition::Exhausted:  return "Exhausto";
            case VillagerCondition::Inspired:   return "Inspirado";
            case VillagerCondition::Injured:    return "Herido";
            default:                            return "Sano";
        }
    } else {
        switch (condition) {
            case VillagerCondition::Healthy:    return "Healthy";
            case VillagerCondition::Hungry:     return "Hungry";
            case VillagerCondition::Full:       return "Full";
            case VillagerCondition::Sick:       return "Sick";
            case VillagerCondition::Drunk:      return "Drunk";
            case VillagerCondition::Aggressive: return "Aggressive";
            case VillagerCondition::Scared:     return "Scared";
            case VillagerCondition::Exhausted:  return "Exhausted";
            case VillagerCondition::Inspired:   return "Inspired";
            case VillagerCondition::Injured:    return "Injured";
            default:                            return "Healthy";
        }
    }
}

// EN: Color representation associated with each villager condition
// ES: Color representativo asociado a cada condición del aldeano
inline ColorRGB getVillagerConditionColor(VillagerCondition condition) {
    switch (condition) {
        case VillagerCondition::Healthy:    return ColorRGB{46, 215, 80};   // Verde saludable
        case VillagerCondition::Hungry:     return ColorRGB{245, 145, 35};  // Naranja hambre
        case VillagerCondition::Full:       return ColorRGB{235, 195, 55};  // Oro saciado
        case VillagerCondition::Sick:       return ColorRGB{125, 215, 55};  // Verde enfermizo / febril
        case VillagerCondition::Drunk:      return ColorRGB{195, 85, 225};  // Violeta embriaguez
        case VillagerCondition::Aggressive: return ColorRGB{235, 45, 45};   // Rojo furia
        case VillagerCondition::Scared:     return ColorRGB{255, 220, 50};  // Amarillo pánico
        case VillagerCondition::Exhausted:  return ColorRGB{140, 175, 215}; // Azul grisáceo fatiga
        case VillagerCondition::Inspired:   return ColorRGB{255, 235, 110}; // Oro celestial éxtasis
        case VillagerCondition::Injured:    return ColorRGB{215, 50, 50};   // Carmesí herida
        default:                            return ColorRGB{226, 218, 196}; // Relic bone
    }
}

class Humano {
public:
    Humano(int id = 1, const std::string& name = "Grog", float startX = 20.5f, float startY = 20.5f,
           HumanGender gender = HumanGender::Male);

    // EN: Procedural name generator based on gender and shamanic role
    // ES: Generador procedural de nombres según género y rol
    static std::string generateName(HumanGender gender, bool isShaman = false);
    inline static std::string generarNombre(HumanGender gender, bool isShaman = false) {
        return generateName(gender, isShaman);
    }

    // EN: Biological gender and identity
    // ES: Identidad biológica y de género
    HumanGender getGender() const { return m_gender; }
    void setGender(HumanGender gender) { m_gender = gender; }
    bool isFemale() const { return m_gender == HumanGender::Female; }
    bool isMale() const { return m_gender == HumanGender::Male; }

    inline HumanGender getGenero() const { return getGender(); }
    inline void setGenero(HumanGender gender) { setGender(gender); }
    inline bool esMujer() const { return isFemale(); }
    inline bool esHombre() const { return isMale(); }

    // EN: Affection, courtship, and spouse marriage system
    // ES: Sistema de Afecto, Cortejo y Matrimonio
    bool hasSpouse() const { return m_spouseId > 0; }
    int getSpouseId() const { return m_spouseId; }
    void setSpouseId(int spouseId) { m_spouseId = spouseId; }
    float getSpouseAffection() const { return m_spouseAffection; }
    void setSpouseAffection(float affection) { m_spouseAffection = std::clamp(affection, 0.0f, 1.0f); }
    void modifyAffection(float deltaAffection) { m_spouseAffection = std::clamp(m_spouseAffection + deltaAffection, 0.0f, 1.0f); }

    inline bool tienePareja() const { return hasSpouse(); }
    inline int getParejaId() const { return getSpouseId(); }
    inline void setParejaId(int spouseId) { setSpouseId(spouseId); }
    inline float getAmorHaciaPareja() const { return getSpouseAffection(); }
    inline void setAmorHaciaPareja(float affection) { setSpouseAffection(affection); }
    inline void modificarAmor(float deltaAffection) { modifyAffection(deltaAffection); }

    // EN: Conception, gestation, and biological procreation system
    // ES: Sistema de Concepción, Gestación y Procreación
    bool isPregnant() const { return m_isPregnant; }
    void setPregnant(bool isPregnant) { m_isPregnant = isPregnant; if (!isPregnant) m_pregnancyProgress = 0.0f; }
    float getPregnancyProgress() const { return m_pregnancyProgress; }
    void setPregnancyProgress(float progress) { m_pregnancyProgress = std::clamp(progress, 0.0f, 1.0f); }
    void advancePregnancy(float deltaProgress) { m_pregnancyProgress = std::clamp(m_pregnancyProgress + deltaProgress, 0.0f, 1.0f); }
    float getFertilityCooldown() const { return m_fertilityCooldown; }
    void setFertilityCooldown(float cooldown) { m_fertilityCooldown = std::max(0.0f, cooldown); }
    void reduceFertilityCooldown(float deltaTime) { m_fertilityCooldown = std::max(0.0f, m_fertilityCooldown - deltaTime); }

    inline bool estaEmbarazada() const { return isPregnant(); }
    inline void setEmbarazada(bool isPregnant) { setPregnant(isPregnant); }
    inline float getEmbarazoProgreso() const { return getPregnancyProgress(); }
    inline void setEmbarazoProgreso(float progress) { setPregnancyProgress(progress); }
    inline void avanzarEmbarazo(float deltaProgress) { advancePregnancy(deltaProgress); }
    inline float getFertilidadCooldown() const { return getFertilityCooldown(); }
    inline void setFertilidadCooldown(float cooldown) { setFertilityCooldown(cooldown); }
    inline void reducirFertilidadCooldown(float deltaTime) { reduceFertilityCooldown(deltaTime); }

    // EN: Lineage, descent, and life cycle (infancy to adulthood)
    // ES: Linaje, Descendencia y Ciclo Vital (Infancia a Adultez)
    float getAge() const { return m_age; }
    void setAge(float age) { m_age = std::max(0.0f, age); }
    void advanceAge(float deltaAge) { m_age += deltaAge; }
    bool isChild() const { return m_isChild; }
    void setChild(bool isChild) { m_isChild = isChild; }
    int getFatherId() const { return m_fatherId; }
    void setFatherId(int fatherId) { m_fatherId = fatherId; }
    int getMotherId() const { return m_motherId; }
    void setMotherId(int motherId) { m_motherId = motherId; }
    int getChildrenCount() const { return m_childrenCount; }
    void setChildrenCount(int count) { m_childrenCount = std::max(0, count); }
    void incrementChildren() { ++m_childrenCount; }
    const std::vector<int>& getChildrenIds() const { return m_childrenIds; }
    void registerChild(int childId) {
        if (std::find(m_childrenIds.begin(), m_childrenIds.end(), childId) == m_childrenIds.end()) {
            m_childrenIds.push_back(childId);
        }
        m_childrenCount = static_cast<int>(m_childrenIds.size());
    }

    inline float getEdad() const { return getAge(); }
    inline void setEdad(float age) { setAge(age); }
    inline void avanzarEdad(float deltaAge) { advanceAge(deltaAge); }
    inline bool esNino() const { return isChild(); }
    inline void setEsNino(bool isChild) { setChild(isChild); }
    inline int getPadreId() const { return getFatherId(); }
    inline void setPadreId(int fatherId) { setFatherId(fatherId); }
    inline int getMadreId() const { return getMotherId(); }
    inline void setMadreId(int motherId) { setMotherId(motherId); }
    inline int getHijosCount() const { return getChildrenCount(); }
    inline void setHijosCount(int count) { setChildrenCount(count); }
    inline void incrementarHijos() { incrementChildren(); }
    inline const std::vector<int>& getHijosIds() const { return getChildrenIds(); }
    inline void registrarHijo(int childId) { registerChild(childId); }

    // EN: Circadian rhythm, biological sleep, and rest
    // ES: Sistema Circadiano, Sueño y Descanso Biológico
    bool isSleeping() const { return m_isSleeping; }
    bool isSleepingIndoors() const { return m_isSleeping && m_isSleepingIndoors; }
    void setSleeping(bool isSleeping) {
        m_isSleeping = isSleeping;
        if (isSleeping) {
            m_action = HumanAction::IDLE;
            m_speed = 0.0f;
        } else {
            m_isSleepingIndoors = false;
            m_speed = 0.65f;
        }
    }
    void setSleepingIndoors(bool isIndoors) {
        m_isSleepingIndoors = isIndoors;
        if (isIndoors) {
            m_isSleeping = true;
            m_action = HumanAction::IDLE;
        }
    }
    float getSleepNeed() const { return m_sleepNeed; }
    void setSleepNeed(float need) { m_sleepNeed = std::clamp(need, 0.0f, 100.0f); }
    void modifySleepNeed(float deltaNeed) { m_sleepNeed = std::clamp(m_sleepNeed + deltaNeed, 0.0f, 100.0f); }

    inline bool estaDurmiendo() const { return isSleeping(); }
    inline bool estaDurmiendoBajoTecho() const { return isSleepingIndoors(); }
    inline void setDurmiendo(bool isSleeping) { setSleeping(isSleeping); }
    inline void setDurmiendoBajoTecho(bool isIndoors) { setSleepingIndoors(isIndoors); }
    inline float getSueno() const { return getSleepNeed(); }
    inline void setSueno(float need) { setSleepNeed(need); }
    inline void modificarSueno(float deltaNeed) { modifySleepNeed(deltaNeed); }

    // EN: Familial protective fury and protective bonds
    // ES: Instinto Protector Familiar y Vínculos de Sangre
    bool hasProtectiveFury() const { return m_protectiveFuryTimer > 0.0f; }
    void activateProtectiveFury(float duration = 7.0f) {
        m_protectiveFuryTimer = duration;
        m_isSleeping = false;
        m_isSleepingIndoors = false;
    }
    float getProtectiveFuryTimer() const { return m_protectiveFuryTimer; }
    int getTargetProtectedId() const { return m_targetProtectedId; }
    void setTargetProtectedId(int protectedId) { m_targetProtectedId = protectedId; }

    inline bool tieneFuriaProtectora() const { return hasProtectiveFury(); }
    inline void activarFuriaProtectora(float duration = 7.0f) { activateProtectiveFury(duration); }
    inline float getFuriaProtectoraTimer() const { return getProtectiveFuryTimer(); }
    inline int getTargetProtegidoId() const { return getTargetProtectedId(); }
    inline void setTargetProtegidoId(int protectedId) { setTargetProtectedId(protectedId); }

    // EN: Home, dwelling, and shelter necessity system
    // ES: Sistema de Hogar, Vivienda y Construcción por Necesidad
    bool hasHome() const { return m_homeId > 0; }
    int getHomeId() const { return m_homeId; }
    void setHomeId(int homeId) { m_homeId = homeId; }
    float getNeedForShelter() const { return m_needForShelter; }
    void setNeedForShelter(float need) { m_needForShelter = std::clamp(need, 0.0f, 1.0f); }
    void modifyNeedForShelter(float deltaNeed) { m_needForShelter = std::clamp(m_needForShelter + deltaNeed, 0.0f, 1.0f); }
    bool isBuildingHome() const { return m_isBuildingHome; }
    void setIsBuildingHome(bool isBuilding) { m_isBuildingHome = isBuilding; }

    inline bool tieneHogar() const { return hasHome(); }
    inline void modificarNeedForShelter(float deltaNeed) { modifyNeedForShelter(deltaNeed); }
    inline bool estaConstruyendoHogar() const { return isBuildingHome(); }
    inline void setEstaConstruyendoHogar(bool isBuilding) { setIsBuildingHome(isBuilding); }

    // EN: Identity and spatial coordinates
    // ES: Identidad y coordenadas espaciales
    int getId() const { return m_id; }
    const std::string& getName() const { return m_name; }
    void setName(const std::string& unitName) {
        m_name = unitName;
        m_humanUnit.unitName = unitName;
    }
    inline const std::string& getNombre() const { return getName(); }
    inline void setNombre(const std::string& unitName) { setName(unitName); }

    float getX() const { return m_x; }
    float getY() const { return m_y; }
    int getCellX() const { return m_cellX; }
    int getCellY() const { return m_cellY; }
    void setPosition(float posX, float posY) {
        m_x = posX;
        m_y = posY;
        m_cellX = static_cast<int>(std::floor(m_x));
        m_cellY = static_cast<int>(std::floor(m_y));
        m_humanUnit.position = {m_x, m_y};
    }
    inline int getCeldaX() const { return getCellX(); }
    inline int getCeldaY() const { return getCellY(); }
    inline void setPosicion(float posX, float posY) { setPosition(posX, posY); }

    HumanAction getAction() const { return m_action; }
    IsoDirection getDirection() const { return m_direction; }
    float getSpeed() const { return m_speed; }
    void setSpeed(float speed) { m_speed = speed; }

    inline HumanAction getAccion() const { return getAction(); }
    inline IsoDirection getDireccion() const { return getDirection(); }
    inline float getVelocidad() const { return getSpeed(); }
    inline void setVelocidad(float speed) { setSpeed(speed); }

    int getReligionId() const { return m_religionId; }
    void setReligionId(int religionId) { m_religionId = religionId; }

    float getDevotion() const { return m_devotion; }
    void setDevotion(float devotion) {
        float previousDevotion = m_devotion;
        m_devotion = std::clamp(devotion, 0.0f, 1.0f);
        m_humanUnit.devotion = m_devotion * 100.0f;
        float devotionDifference = m_devotion - previousDevotion;
        if (devotionDifference >= 0.012f) {
            triggerFaithIndicator(+1);
        } else if (devotionDifference <= -0.012f) {
            triggerFaithIndicator(-1);
        } else {
            m_faithDeltaAccum += devotionDifference;
            if (m_faithDeltaAccum >= 0.018f) {
                triggerFaithIndicator(+1);
            } else if (m_faithDeltaAccum <= -0.018f) {
                triggerFaithIndicator(-1);
            }
        }
    }
    inline float getDevocion() const { return getDevotion(); }
    inline void setDevocion(float devotion) { setDevotion(devotion); }

    int getFaithIndicatorSign() const { return m_faithIndicatorSign; }
    float getFaithIndicatorTimer() const { return m_faithIndicatorTimer; }
    void triggerFaithIndicator(int sign, float duration = 1.35f) {
        if (sign == 0) return;
        m_faithIndicatorSign = sign;
        m_faithIndicatorTimer = duration;
        m_faithDeltaAccum = 0.0f;
    }

    bool isShaman() const { return m_isShaman || m_humanUnit.role == CommunityRole::SHAMAN; }
    void setShaman(bool isShaman) {
        m_isShaman = isShaman;
        if (isShaman) {
            m_humanUnit.role = CommunityRole::SHAMAN;
            m_humanUnit.conversionPower = 38.0f;
            m_humanUnit.conversionInertia = 85.0f;
            m_humanUnit.willpower = 92.0f;
            m_leaderDevotion = 1.0f;
        } else if (m_humanUnit.role == CommunityRole::SHAMAN) {
            m_humanUnit.role = CommunityRole::FORAGER;
        }
    }
    inline bool esChaman() const { return isShaman(); }
    inline void setEsChaman(bool isShaman) { setShaman(isShaman); }

    bool isProphet() const { return m_humanUnit.role == CommunityRole::PROPHET; }
    void setProphet(bool isProphet) {
        if (isProphet) {
            m_isShaman = false;
            m_humanUnit.role = CommunityRole::PROPHET;
            m_humanUnit.conversionPower = 48.0f;
            m_humanUnit.conversionInertia = 90.0f;
            m_humanUnit.willpower = 95.0f;
            m_leaderDevotion = 1.0f;
        } else if (m_humanUnit.role == CommunityRole::PROPHET) {
            m_humanUnit.role = CommunityRole::FORAGER;
        }
    }
    inline bool esProfeta() const { return isProphet(); }
    inline void setEsProfeta(bool isProphet) { setProphet(isProphet); }

    bool isSpiritualLeader() const {
        return isShaman() || isProphet();
    }
    inline bool esLiderEspiritual() const { return isSpiritualLeader(); }

    float getLeaderDevotion() const { return m_leaderDevotion; }
    void setLeaderDevotion(float leaderDevotion) { m_leaderDevotion = std::clamp(leaderDevotion, 0.0f, 1.0f); }
    int getLeaderId() const { return m_leaderId; }
    void setLeaderId(int leaderId) { m_leaderId = leaderId; }

    bool isZealot() const {
        return m_humanUnit.role == CommunityRole::ZEALOT;
    }
    inline bool esFanatico() const { return isZealot(); }
    void promoteToZealot();
    void demoteZealot();
    inline void promoverAFanatico() { promoteToZealot(); }
    inline void degradarFanatico() { demoteZealot(); }

    bool canWalkOnWaterAndDangerousTerrain() const {
        return isProphet();
    }
    inline bool puedeCaminarSobreAguaYTerrenoPeligroso() const { return canWalkOnWaterAndDangerousTerrain(); }

    ResourceType getGatheringResourceType() const { return m_gatheringResourceType; }
    void setGatheringResourceType(ResourceType resourceType) { m_gatheringResourceType = resourceType; }
    ResourceType chooseGatheringResourceType(const WorldMap& mapRef, const VillageResources* villageResourcesRef = nullptr);
    inline ResourceType getTipoRecursoRecolectando() const { return getGatheringResourceType(); }
    inline void setTipoRecursoRecolectando(ResourceType resourceType) { setGatheringResourceType(resourceType); }
    inline ResourceType elegirQueRecolectarSegunEntorno(const WorldMap& mapRef, const VillageResources* villageResourcesRef = nullptr) {
        return chooseGatheringResourceType(mapRef, villageResourcesRef);
    }

    int getCarriedResourceAmount() const { return m_carriedResourceAmount; }
    void setCarriedResourceAmount(int amount) { m_carriedResourceAmount = std::max(0, amount); }
    int getCarryingCapacity() const { return m_carryingCapacity; }
    void setCarryingCapacity(int capacity) { m_carryingCapacity = std::max(1, capacity); }
    bool isCarryingCapacityFull() const { return m_carriedResourceAmount >= m_carryingCapacity; }

    inline int getCantidadRecursoCargado() const { return getCarriedResourceAmount(); }
    inline void setCantidadRecursoCargado(int amount) { setCarriedResourceAmount(amount); }
    inline int getCapacidadCarga() const { return getCarryingCapacity(); }
    inline void setCapacidadCarga(int capacity) { setCarryingCapacity(capacity); }
    inline bool estaLlenoDeCarga() const { return isCarryingCapacityFull(); }

    float getSpiritualHunger() const { return m_spiritualHunger; }
    void setSpiritualHunger(float hunger) { m_spiritualHunger = std::clamp(hunger, 0.0f, 100.0f); }
    bool isDeliveringOffering() const { return m_isDeliveringOffering; }
    void setDeliveringOffering(bool isDelivering) { m_isDeliveringOffering = isDelivering; }

    int getTargetEnemyId() const { return m_targetEnemyId; }
    void setTargetEnemyId(int enemyId) { m_targetEnemyId = enemyId; }
    float getAttackCooldown() const { return m_attackCooldown; }
    void setAttackCooldown(float cooldown) { m_attackCooldown = std::max(0.0f, cooldown); }

    // EN: Spatial environmental map queries
    // ES: Búsquedas sistémicas en el mapa
    bool findResourceNode(const WorldMap& mapRef, ResourceType resourceType, int maxRadius, Point2i& outCellPos) const;
    bool findAnyResourceNode(const WorldMap& mapRef, int maxRadius, Point2i& outCellPos, ResourceType& outResourceType) const;
    bool findShrineOrAltar(const WorldMap& mapRef, int religionId, int maxRadius, Point2i& outCellPos) const;
    bool findCommunityCenter(const WorldMap& mapRef, int religionId, int maxRadius, Point2i& outCellPos) const;

    inline bool buscarNodoRecurso(const WorldMap& mapRef, ResourceType resourceType, int maxRadius, Point2i& outCellPos) const {
        return findResourceNode(mapRef, resourceType, maxRadius, outCellPos);
    }
    inline bool buscarCualquierNodoRecurso(const WorldMap& mapRef, int maxRadius, Point2i& outCellPos, ResourceType& outResourceType) const {
        return findAnyResourceNode(mapRef, maxRadius, outCellPos, outResourceType);
    }
    inline bool buscarSantuarioOAltar(const WorldMap& mapRef, int religionId, int maxRadius, Point2i& outCellPos) const {
        return findShrineOrAltar(mapRef, religionId, maxRadius, outCellPos);
    }
    inline bool buscarCentroComunidad(const WorldMap& mapRef, int religionId, int maxRadius, Point2i& outCellPos) const {
        return findCommunityCenter(mapRef, religionId, maxRadius, outCellPos);
    }

    float getDivineSignTimer() const { return m_divineSignTimer; }
    void activateDivineSign(float duration = 5.5f) {
        m_divineSignTimer = duration;
        m_stateTimer = duration;
        m_action = HumanAction::CELEBRATE;
        m_devotion = 1.0f;
        m_humanUnit.devotion = 100.0f;
        m_humanUnit.energy = 100.0f;
        m_humanUnit.currentState = HumanAIState::WORSHIPING;
        triggerFaithIndicator(+1, 1.6f);
    }
    inline float getTimerSenalDivina() const { return getDivineSignTimer(); }
    inline void activarSenalDivina(float duration = 5.5f) { activateDivineSign(duration); }

    float getHealth() const { return m_health; }
    bool isAlive() const { return m_health > 0.0f; }
    bool isDead() const { return m_health <= 0.0f; }
    float getDeathTimer() const { return m_deathTimer; }

    inline float getSalud() const { return getHealth(); }
    inline bool estaVivo() const { return isAlive(); }
    inline bool estaMuerto() const { return isDead(); }
    inline float getTimerMuerte() const { return getDeathTimer(); }

    void setHealth(float health) {
        m_health = std::clamp(health, 0.0f, 100.0f);
        m_humanUnit.hitPoints = m_health;
        if (m_health <= 0.0f && m_deathTimer <= 0.0f) {
            kill("Pérdida total de salud");
        }
    }
    void takeDamage(float damageAmount) {
        if (damageAmount <= 0.0f || m_health <= 0.0f) return;
        m_health = std::max(0.0f, m_health - damageAmount);
        m_humanUnit.hitPoints = m_health;
        m_damageTimer = 0.65f;
        m_isSleeping = false;
        m_isSleepingIndoors = false;
        if (m_health <= 0.0f && m_deathTimer <= 0.0f) {
            kill("Caído en combate");
        }
    }
    float getDamageTimer() const { return m_damageTimer; }
    float getEnergy() const { return m_energy; }
    void setEnergy(float energy) {
        m_energy = std::clamp(energy, 0.0f, 100.0f);
        m_humanUnit.energy = m_energy;
    }
    void modifyEnergy(float deltaEnergy) { setEnergy(m_energy + deltaEnergy); }

    inline void setSalud(float health) { setHealth(health); }
    inline void heal(float healAmount) {
        if (healAmount <= 0.0f || m_health <= 0.0f) return;
        setHealth(m_health + healAmount);
    }
    inline void curar(float healAmount) { heal(healAmount); }
    inline void recibirDano(float damageAmount) { takeDamage(damageAmount); }
    inline float getTimerDano() const { return getDamageTimer(); }
    inline float getEnergia() const { return getEnergy(); }
    inline void setEnergia(float energy) { setEnergy(energy); }
    inline void modificarEnergia(float deltaEnergy) { modifyEnergy(deltaEnergy); }

    // EN: Physiological, psychological, and behavioral condition evaluation
    // ES: Evaluación de condición fisiológica, psicológica y conductual del aldeano
    [[nodiscard]] VillagerCondition getCondition() const;
    [[nodiscard]] inline VillagerCondition getCondicion() const { return getCondition(); }
    [[nodiscard]] std::string getConditionName(bool inSpanish = true) const {
        return getVillagerConditionName(getCondition(), inSpanish);
    }
    [[nodiscard]] inline std::string getNombreCondicion(bool inSpanish = true) const {
        return getConditionName(inSpanish);
    }
    [[nodiscard]] ColorRGB getConditionColor() const {
        return getVillagerConditionColor(getCondition());
    }

    // EN: Alcohol intoxication, drunkenness level, and festivity
    // ES: Intoxicación etílica, nivel de embriaguez y festividad
    [[nodiscard]] float getAlcoholLevel() const { return m_alcoholLevel; }
    void setAlcoholLevel(float level) { m_alcoholLevel = std::clamp(level, 0.0f, 100.0f); }
    void consumeAlcohol(float amount) { setAlcoholLevel(m_alcoholLevel + amount); }
    [[nodiscard]] inline float getAlcohol() const { return getAlcoholLevel(); }
    inline void setAlcohol(float level) { setAlcoholLevel(level); }
    inline void consumirAlcohol(float amount) { consumeAlcohol(amount); }
    [[nodiscard]] bool isDrunk() const { return m_alcoholLevel > 30.0f; }
    [[nodiscard]] inline bool estaBorracho() const { return isDrunk(); }

    // EN: Combat rage, belligerence, and aggressive state
    // ES: Furia combativa, beligerancia y estado agresivo
    [[nodiscard]] float getAggressionLevel() const { return m_aggressionLevel; }
    void setAggressionLevel(float level) { m_aggressionLevel = std::clamp(level, 0.0f, 100.0f); }
    void provokeAggression(float amount = 50.0f) { setAggressionLevel(m_aggressionLevel + amount); }
    [[nodiscard]] inline float getAgresividad() const { return getAggressionLevel(); }
    inline void setAgresividad(float level) { setAggressionLevel(level); }
    inline void provocarAgresividad(float amount = 50.0f) { provokeAggression(amount); }
    [[nodiscard]] bool isAggressive() const { return m_protectiveFuryTimer > 0.0f || m_aggressionLevel > 50.0f; }
    [[nodiscard]] inline bool estaAgresivo() const { return isAggressive(); }

    // EN: Reset temporary conditions (alcohol, rage, panic, infection, ecstasy)
    // ES: Restablecer condiciones temporales (alcohol, furia, pánico, infección, éxtasis)
    void resetConditions();
    inline void resetCondiciones() { resetConditions(); }

    // EN: Effective movement speed modulated by current biological/psychological condition
    // ES: Velocidad efectiva de movimiento modulada por la condición biológica/psicológica actual
    [[nodiscard]] float getEffectiveSpeed() const;
    [[nodiscard]] inline float getVelocidadEfectiva() const { return getEffectiveSpeed(); }

    const RealisticPopGroup& getPopGroup() const { return m_popGroup; }
    RealisticPopGroup& getPopGroupMutable() { return m_popGroup; }
    RealisticVillagerState getRealisticState() const { return m_popGroup.state; }

    const HumanUnit& getHumanUnit() const { return m_humanUnit; }
    HumanUnit& getHumanUnitMutable() { return m_humanUnit; }
    CommunityRole getRole() const { return m_humanUnit.role; }
    void setRole(CommunityRole role) {
        m_humanUnit.role = role;
        if (role == CommunityRole::ZEALOT) {
            m_humanUnit.conversionInertia = 95.0f;
            m_humanUnit.willpower = 95.0f;
            m_humanUnit.conversionPower = 28.0f;
            m_devotion = std::max(m_devotion, 0.92f);
            m_leaderDevotion = std::max(m_leaderDevotion, 0.90f);
            m_humanUnit.devotion = m_devotion * 100.0f;
        }
    }
    HumanAIState getHumanAIState() const { return m_humanUnit.currentState; }

    const UnitMoralRecord& getMoralRecord() const { return m_moralRecord; }
    UnitMoralRecord& getMoralRecordMutable() { return m_moralRecord; }
    bool isApostate() const { return m_moralRecord.isApostateOrHeretic; }
    inline bool esApostata() const { return isApostate(); }

    AIDifficulty getAIDifficulty() const { return m_aiDifficulty; }
    void setAIDifficulty(AIDifficulty difficulty) { m_aiDifficulty = difficulty; }
    TribalBelligerence getTribalBelligerence() const { return m_tribalBelligerence; }
    void setTribalBelligerence(TribalBelligerence belligerence) { m_tribalBelligerence = belligerence; }

    float getAltitude() const { return m_altitude; }
    void setAltitude(float altitude) { m_altitude = altitude; }
    bool isFlying() const { return m_flying; }
    void setFlying(bool flying) { m_flying = flying; }
    float getVelocityZ() const { return m_velocityZ; }
    float getVelocityX() const { return m_velocityX; }
    float getVelocityY() const { return m_velocityY; }

    float getPanicTimer() const { return m_panicTimer; }
    void setPanicTimer(float timer) { m_panicTimer = timer; }
    bool isPanicking() const { return m_panicTimer > 0.0f; }
    inline bool estaEnPanico() const { return isPanicking(); }
    float getAnimTimer() const { return m_animTimer; }

    float getStumbleTimer() const { return m_stumbleTimer; }
    void setStumbleTimer(float timer) { m_stumbleTimer = timer; }

    bool isInfected() const { return m_isInfected; }
    void setInfected(bool infected) { m_isInfected = infected; }
    float getInfectionTimer() const { return m_infectionTimer; }
    void setInfectionTimer(float timer) { m_infectionTimer = timer; }
    inline void setTimerInfeccion(float timer) { setInfectionTimer(timer); }
    float getCoughTimer() const { return m_coughTimer; }

    bool isDrowning() const { return m_isDrowning; }
    void setDrowning(bool drowning) { m_isDrowning = drowning; }
    bool isSwimming() const { return m_isSwimming; }
    void setSwimming(bool swimming) { m_isSwimming = swimming; }
    inline bool estaNadando() const { return isSwimming(); }
    inline void setEstaNadando(bool swimming) { setSwimming(swimming); }
    const std::string& getCauseOfDeath() const { return m_causeOfDeath; }
    void setCauseOfDeath(const std::string& cause) { m_causeOfDeath = cause; }
    void kill(const std::string& cause = "Fallecimiento", float corpseDurationSec = 2.0f);
    inline void matar(const std::string& cause = "Fallecimiento", float corpseDurationSec = 2.0f) { kill(cause, corpseDurationSec); }

    void activateRagdoll(float impulseX, float impulseY, float impulseZ, float angularSpeedRadS = 12.0f);
    inline void activarRagdoll(float impulseX, float impulseY, float impulseZ, float angularSpeedRadS = 12.0f) {
        activateRagdoll(impulseX, impulseY, impulseZ, angularSpeedRadS);
    }
    bool isRagdoll() const { return m_ragdoll.isActive(); }
    const Ragdoll& getRagdoll() const { return m_ragdoll; }
    Ragdoll& getRagdollMutable() { return m_ragdoll; }

    void fleeInPanicFrom(float originX, float originY, float duration = 5.0f);
    inline void huirEnPanicoDesde(float originX, float originY, float duration = 5.0f) {
        fleeInPanicFrom(originX, originY, duration);
    }
    void suckIntoTornado(float tornadoX, float tornadoY, float effectRadius, float deltaTime);
    inline void serSuccionadoPorTornado(float tornadoX, float tornadoY, float effectRadius, float deltaTime) {
        suckIntoTornado(tornadoX, tornadoY, effectRadius, deltaTime);
    }
    void sufferEarthquake(float quakeX, float quakeY, float intensity);
    inline void sufrirSismoTerremoto(float quakeX, float quakeY, float intensity) {
        sufferEarthquake(quakeX, quakeY, intensity);
    }
    void displaceByEarthquake(float offsetX, float offsetY, const WorldMap& mapRef);
    inline void desplazarPorSismo(float offsetX, float offsetY, const WorldMap& mapRef) {
        displaceByEarthquake(offsetX, offsetY, mapRef);
    }
    void infectWithPandemic(float duration = 18.0f);
    inline void infectarConPandemia(float duration = 18.0f) {
        infectWithPandemic(duration);
    }

    // EN: Fire ignition, burning state, panic flight, and water extinguishing
    // ES: Ignición por fuego, estado en llamas, huida en pánico y extinción en agua
    [[nodiscard]] bool isBurning() const { return m_isBurning; }
    [[nodiscard]] inline bool estaEnLlamas() const { return isBurning(); }
    [[nodiscard]] float getBurnTimer() const { return m_burnTimer; }
    void ignite(float durationSeconds = 6.0f);
    inline void prenderFuego(float durationSeconds = 6.0f) { ignite(durationSeconds); }
    void extinguishFire();
    inline void apagarFuego() { extinguishFire(); }

    void processWaterFall(const WorldMap& mapRef);
    inline void procesarCaidaEnAgua(const WorldMap& mapRef) {
        processWaterFall(mapRef);
    }
    void processGroundLanding(const WorldMap& mapRef);
    inline void procesarAterrizajeTierra(const WorldMap& mapRef) {
        processGroundLanding(mapRef);
    }
    // EN: Calculate and apply physics-based fall damage to living units
    // ES: Calcular y aplicar daño por caída basado en físicas a unidades vivas
    void applyFallDamage(float impactSpeed, const WorldMap& mapRef);
    inline void aplicarDanoPorCaida(float impactSpeed, const WorldMap& mapRef) {
        applyFallDamage(impactSpeed, mapRef);
    }
    float getLastFallDamage() const { return m_lastFallDamage; }

    // EN: Check cliff elevation drop and initiate free-fall if stepping off an edge
    // ES: Verificar desnivel de riscos y precipicios e iniciar caída libre si se sale por un borde
    void checkCliffFall(int previousCellX, int previousCellY, const WorldMap& mapRef);
    inline void verificarCaidaPorRisco(int previousCellX, int previousCellY, const WorldMap& mapRef) {
        checkCliffFall(previousCellX, previousCellY, mapRef);
    }

    bool findNearbySolidGround(const WorldMap& mapRef, float& outWorldX, float& outWorldY) const;
    inline bool buscarTierraFirmeCercana(const WorldMap& mapRef, float& outWorldX, float& outWorldY) const {
        return findNearbySolidGround(mapRef, outWorldX, outWorldY);
    }
    void swimTowardsSolidGround(const WorldMap& mapRef);
    inline void nadarHaciaTierraFirme(const WorldMap& mapRef) {
        swimTowardsSolidGround(mapRef);
    }

    void update(float deltaTime, const WorldMap& mapRef,
                const std::vector<Humano>* totalPopulation = nullptr,
                bool isNightTime = false,
                const BuildingSystem* buildingSystemRef = nullptr);
    inline void actualizar(float deltaTime, const WorldMap& mapRef,
                           const std::vector<Humano>* totalPopulation = nullptr,
                           bool isNightTime = false,
                           const BuildingSystem* buildingSystemRef = nullptr) {
        update(deltaTime, mapRef, totalPopulation, isNightTime, buildingSystemRef);
    }

    void forceAIDecision(const WorldMap& mapRef,
                         const std::vector<Humano>* totalPopulation = nullptr,
                         bool isNightTime = false,
                         const BuildingSystem* buildingSystemRef = nullptr) {
        makeAIDecision(mapRef, totalPopulation, isNightTime, buildingSystemRef);
    }
    inline void forzarDecisionIA(const WorldMap& mapRef,
                                 const std::vector<Humano>* totalPopulation = nullptr,
                                 bool isNightTime = false,
                                 const BuildingSystem* buildingSystemRef = nullptr) {
        forceAIDecision(mapRef, totalPopulation, isNightTime, buildingSystemRef);
    }
    void makeAIDecision(const WorldMap& mapRef, const std::vector<Humano>* totalPopulation = nullptr,
                        bool isNightTime = false, const BuildingSystem* buildingSystemRef = nullptr);
    inline void tomarDecisionIA(const WorldMap& mapRef, const std::vector<Humano>* totalPopulation = nullptr,
                                bool isNightTime = false, const BuildingSystem* buildingSystemRef = nullptr) {
        makeAIDecision(mapRef, totalPopulation, isNightTime, buildingSystemRef);
    }

    void changeAction(HumanAction newAction);
    inline void cambiarAccion(HumanAction newAction) { changeAction(newAction); }
    void changeDirection(IsoDirection newDirection);
    inline void cambiarDireccion(IsoDirection newDirection) { changeDirection(newDirection); }
    void orientTowards(float targetPosX, float targetPosY);
    inline void orientarHacia(float targetPosX, float targetPosY) { orientTowards(targetPosX, targetPosY); }
    void walkTowards(float targetPosX, float targetPosY, bool isRunning = false);
    inline void caminarHacia(float targetPosX, float targetPosY, bool isRunning = false) {
        walkTowards(targetPosX, targetPosY, isRunning);
    }
    void orderManualMovement(float targetPosX, float targetPosY);
    inline void ordenarMovimientoManual(float targetPosX, float targetPosY) { orderManualMovement(targetPosX, targetPosY); }
    bool hasManualOrder() const { return m_hasManualOrder; }
    inline bool tieneOrdenManual() const { return hasManualOrder(); }
    float getTargetDestX() const { return m_targetDestX; }
    float getTargetDestY() const { return m_targetDestY; }
    inline float getDestinoX() const { return getTargetDestX(); }
    inline float getDestinoY() const { return getTargetDestY(); }

    TileRect getTileRect(const TileSet& tileSetRef) const;

    void render(uint32_t* dstBuffer, int bufferWidth, int bufferHeight,
                const IsometricCamera& cameraRef, const WorldMap& mapRef,
                const TileSet& tileSetRef, ColorRGB auraColor, bool isSelected = false) const;
    inline void renderizar(uint32_t* dstBuffer, int bufferWidth, int bufferHeight,
                           const IsometricCamera& cameraRef, const WorldMap& mapRef,
                           const TileSet& tileSetRef, ColorRGB auraColor, bool isSelected = false) const {
        render(dstBuffer, bufferWidth, bufferHeight, cameraRef, mapRef, tileSetRef, auraColor, isSelected);
    }

    [[nodiscard]] float calculateReproductionDesire(const Humano& spouseRef, const WorldMap& mapRef,
                                                    const BuildingSystem* buildingSystemRef) const;
    inline float calcularDeseoReproduccion(const Humano& spouseRef, const WorldMap& mapRef,
                                           const BuildingSystem* buildingSystemRef) const {
        return calculateReproductionDesire(spouseRef, mapRef, buildingSystemRef);
    }

    // EN: Living organism biological drives (reproduction, parental investment, and colonization)
    // ES: Impulsos biológicos del organismo vivo (reproducción, inversión parental y colonización)
    float getReproductionDrive() const { return m_reproductionDrive; }
    void setReproductionDrive(float drive) { m_reproductionDrive = std::clamp(drive, 0.0f, 1.0f); }
    void modifyReproductionDrive(float deltaDrive) { m_reproductionDrive = std::clamp(m_reproductionDrive + deltaDrive, 0.0f, 1.0f); }

    float getCourtshipTimer() const { return m_courtshipTimer; }
    void setCourtshipTimer(float timer) { m_courtshipTimer = std::max(0.0f, timer); }

    float getParentalCareTimer() const { return m_parentalCareTimer; }
    void setParentalCareTimer(float timer) { m_parentalCareTimer = std::max(0.0f, timer); }

    float getPioneeringUrge() const { return m_pioneeringUrge; }
    void setPioneeringUrge(float urge) { m_pioneeringUrge = std::clamp(urge, 0.0f, 1.0f); }
    void modifyPioneeringUrge(float deltaUrge) { m_pioneeringUrge = std::clamp(m_pioneeringUrge + deltaUrge, 0.0f, 1.0f); }

    inline float getDeseoReproduccionBiologico() const { return getReproductionDrive(); }
    inline void setDeseoReproduccionBiologico(float drive) { setReproductionDrive(drive); }
    inline void modificarDeseoReproduccionBiologico(float deltaDrive) { modifyReproductionDrive(deltaDrive); }
    inline float getTimerCortejo() const { return getCourtshipTimer(); }
    inline void setTimerCortejo(float timer) { setCourtshipTimer(timer); }
    inline float getImpulsoPionero() const { return getPioneeringUrge(); }
    inline void setImpulsoPionero(float urge) { setPioneeringUrge(urge); }
    inline void modificarImpulsoPionero(float deltaUrge) { modifyPioneeringUrge(deltaUrge); }

    // EN: Parental investment: share gathered or stored food with a hungry child
    // ES: Inversión parental: compartir alimento recolectado o corporal con un hijo hambriento
    bool feedChild(Humano& childRef);
    inline bool alimentarHijo(Humano& childRef) { return feedChild(childRef); }

    // EN: Active courtship of a compatible prospective partner
    // ES: Cortejo activo hacia una pareja potencial compatible
    bool courtCandidate(Humano& candidateRef, float deltaTime, EventLine* eventLine = nullptr, int currentYear = -4000);
    inline bool cortejarPretendiente(Humano& candidateRef, float deltaTime, EventLine* eventLine = nullptr, int currentYear = -4000) {
        return courtCandidate(candidateRef, deltaTime, eventLine, currentYear);
    }

private:
    int m_id{1};
    std::string m_name{"Grog"};
    HumanGender m_gender{HumanGender::Male};

    // Vínculo afectivo, conyugal y reproducción
    int m_spouseId{-1};
    float m_spouseAffection{0.0f};
    bool m_isPregnant{false};
    float m_pregnancyProgress{0.0f};
    float m_fertilityCooldown{0.0f};
    float m_reproductionDrive{0.20f};
    float m_courtshipTimer{0.0f};
    float m_parentalCareTimer{0.0f};
    float m_pioneeringUrge{0.0f};

    // Linaje, edad y crecimiento
    float m_age{20.0f};
    bool m_isChild{false};
    int m_fatherId{-1};
    int m_motherId{-1};
    int m_childrenCount{0};

    // Vivienda y necesidad de refugio
    int m_homeId{-1};
    float m_needForShelter{0.0f};
    bool m_isBuildingHome{false};

    // Estado biológico de descanso e instinto protector
    bool m_isSleeping{false};
    bool m_isSleepingIndoors{false};
    float m_sleepNeed{10.0f};
    float m_protectiveFuryTimer{0.0f};
    int m_targetProtectedId{-1};
    std::vector<int> m_childrenIds;

    float m_x{20.5f};
    float m_y{20.5f};
    int m_cellX{20};
    int m_cellY{20};

    float m_targetDestX{20.5f};
    float m_targetDestY{20.5f};

    HumanAction m_action{HumanAction::IDLE};
    IsoDirection m_direction{IsoDirection::SW};

    float m_speed{0.65f};
    int m_religionId{1};
    float m_devotion{0.80f};
    float m_leaderDevotion{0.50f};
    int m_leaderId{-1};
    bool m_isShaman{false};
    bool m_hasManualOrder{false};
    float m_divineSignTimer{0.0f};
    float m_health{100.0f};
    float m_energy{100.0f};
    float m_damageTimer{0.0f};
    float m_deathTimer{0.0f};

    float m_faithDeltaAccum{0.0f};
    int m_faithIndicatorSign{0}; // +1: '+' verde, -1: '+' rojo
    float m_faithIndicatorTimer{0.0f};

    float m_stateTimer{2.5f};
    float m_animTimer{0.0f};
    RealisticPopGroup m_popGroup{};
    HumanUnit m_humanUnit{};
    UnitMoralRecord m_moralRecord{};
    ResourceType m_gatheringResourceType{ResourceType::None};
    int m_carriedResourceAmount{0};
    int m_carryingCapacity{10};
    float m_spiritualHunger{20.0f};
    bool m_isDeliveringOffering{false};
    int m_targetEnemyId{-1};
    float m_attackCooldown{0.0f};

    // Físicas de vuelo y estados de desastres naturales
    float m_altitude{0.0f};
    float m_velocityZ{0.0f};
    float m_velocityX{0.0f};
    float m_velocityY{0.0f};
    bool m_flying{false};
    float m_angularAngle{0.0f};
    float m_lastFallDamage{0.0f};

    float m_panicTimer{0.0f};
    float m_stumbleTimer{0.0f};
    bool m_isInfected{false};
    float m_infectionTimer{0.0f};
    float m_coughTimer{0.0f};

    AIDifficulty m_aiDifficulty{AIDifficulty::Equilibrada};
    TribalBelligerence m_tribalBelligerence{TribalBelligerence::Equilibrada};

    bool m_isDrowning{false};
    bool m_isSwimming{false};
    bool m_isBurning{false};
    float m_burnTimer{0.0f};
    float m_burnDamageTimer{0.0f};
    std::string m_causeOfDeath{""};

    // EN: Alcohol intoxication, drunkenness sway, and aggression states
    // ES: Intoxicación etílica, tambaleo de embriaguez y estados de agresividad
    float m_alcoholLevel{0.0f};
    float m_drunkenSwayTimer{0.0f};
    float m_aggressionLevel{0.0f};

    Ragdoll m_ragdoll;
};

using Human = Humano;

class GestorHumanos {
public:
    GestorHumanos();

    // EN: Initialize world human population
    // ES: Inicializar población humana de aldeanos sobre el mapa
    void initializePopulation(WorldMap& mapRef, const ReligionRegistry& registryRef, int populationCount = 50);
    inline void inicializarPoblacion(WorldMap& mapRef, const ReligionRegistry& registryRef, int populationCount = 50) {
        initializePopulation(mapRef, registryRef, populationCount);
    }

    void update(float deltaTime, WorldMap& mapRef, EventLine* eventLine = nullptr, int currentYear = -4000,
                ReligionRegistry* registry = nullptr, SoundSystem* soundSystem = nullptr,
                const IsometricCamera* camera = nullptr,
                NemesisSififusManager* nemesisSififus = nullptr,
                ReligiousMoralitySystem* moralitySystem = nullptr,
                const DayNightCycle* dayNight = nullptr,
                const BuildingSystem* buildingSystem = nullptr);
    inline void actualizar(float deltaTime, WorldMap& mapRef, EventLine* eventLine = nullptr, int currentYear = -4000,
                           ReligionRegistry* registry = nullptr, SoundSystem* soundSystem = nullptr,
                           const IsometricCamera* camera = nullptr,
                           NemesisSififusManager* nemesisSififus = nullptr,
                           ReligiousMoralitySystem* moralitySystem = nullptr,
                           const DayNightCycle* dayNight = nullptr,
                           const BuildingSystem* buildingSystem = nullptr) {
        update(deltaTime, mapRef, eventLine, currentYear, registry, soundSystem, camera, nemesisSififus, moralitySystem, dayNight, buildingSystem);
    }

    void renderAll(uint32_t* dstBuffer, int bufferWidth, int bufferHeight,
                   const IsometricCamera& cameraRef, const WorldMap& mapRef,
                   const TileSet& tileSetRef, const ReligionRegistry& registryRef) const;
    inline void renderizarTodos(uint32_t* dstBuffer, int bufferWidth, int bufferHeight,
                                const IsometricCamera& cameraRef, const WorldMap& mapRef,
                                const TileSet& tileSetRef, const ReligionRegistry& registryRef) const {
        renderAll(dstBuffer, bufferWidth, bufferHeight, cameraRef, mapRef, tileSetRef, registryRef);
    }

    const std::vector<Humano>& getVillagers() const { return m_villagers; }
    std::vector<Humano>& getVillagers() { return m_villagers; }
    inline const std::vector<Humano>& getHumanos() const { return getVillagers(); }
    inline std::vector<Humano>& getHumanos() { return getVillagers(); }

    [[nodiscard]] int countLivingHumansOfReligion(int religionId) const {
        int livingCount = 0;
        for (const auto& villager : m_villagers) {
            if (villager.getReligionId() == religionId && villager.isAlive()) {
                ++livingCount;
            }
        }
        return livingCount;
    }
    inline int contarHumanosVivosDeReligion(int religionId) const {
        return countLivingHumansOfReligion(religionId);
    }

    [[nodiscard]] int countTotalHumansOfReligion(int religionId) const {
        int totalCount = 0;
        for (const auto& villager : m_villagers) {
            if (villager.getReligionId() == religionId) {
                ++totalCount;
            }
        }
        return totalCount;
    }
    inline int contarHumanosTotalesDeReligion(int religionId) const {
        return countTotalHumansOfReligion(religionId);
    }

    void addVillager(const Humano& villagerRef);
    inline void agregarHumano(const Humano& villagerRef) { addVillager(villagerRef); }

    const Humano* getSelectedVillager() const;
    Humano* getSelectedVillagerMutable();
    inline const Humano* getHumanoSeleccionado() const { return getSelectedVillager(); }
    inline Humano* getHumanoSeleccionadoMutable() { return getSelectedVillagerMutable(); }

    void selectVillager(int villagerId);
    inline void seleccionarHumano(int villagerId) { selectVillager(villagerId); }

    int findShamanOfReligion(int religionId) const;
    inline int buscarChamanDeReligion(int religionId) const { return findShamanOfReligion(religionId); }

    int findSpiritualLeaderOfReligion(int religionId) const;
    inline int buscarLiderEspiritualDeReligion(int religionId) const { return findSpiritualLeaderOfReligion(religionId); }

    void executeDivineSign(int signType, WorldMap& mapRef, ReligionRegistry& registryRef,
                           EventLine& eventLineRef, int currentYear);
    inline void ejecutarSenalDivina(int signType, WorldMap& mapRef, ReligionRegistry& registryRef,
                                    EventLine& eventLineRef, int currentYear) {
        executeDivineSign(signType, mapRef, registryRef, eventLineRef, currentYear);
    }

    int findNearbyVillager(float worldPosX, float worldPosY, float maxRadius = 1.0f, int onlyReligionId = -1) const;
    inline int buscarHumanoCercano(float worldPosX, float worldPosY, float maxRadius = 1.0f, int onlyReligionId = -1) const {
        return findNearbyVillager(worldPosX, worldPosY, maxRadius, onlyReligionId);
    }

    int findVillagerOnScreen(float screenPosX, float screenPosY, const IsometricCamera& cameraRef,
                             const WorldMap& mapRef, int onlyReligionId = -1) const;
    inline int buscarHumanoEnPantalla(float screenPosX, float screenPosY, const IsometricCamera& cameraRef,
                                      const WorldMap& mapRef, int onlyReligionId = -1) const {
        return findVillagerOnScreen(screenPosX, screenPosY, cameraRef, mapRef, onlyReligionId);
    }

    AIDifficulty getAIDifficulty() const { return m_aiDifficulty; }
    void setAIDifficulty(AIDifficulty diff) {
        m_aiDifficulty = diff;
        for (auto& villager : m_villagers) villager.setAIDifficulty(diff);
    }
    TribalBelligerence getTribalBelligerence() const { return m_tribalBelligerence; }
    void setTribalBelligerence(TribalBelligerence bel) {
        m_tribalBelligerence = bel;
        for (auto& villager : m_villagers) villager.setTribalBelligerence(bel);
    }

private:
    std::vector<Humano> m_villagers;
    int m_selectedVillagerId{-1};
    int m_nextVillagerId{1};
    FirstEraAIController m_firstEraAI{};
    HumanAIController m_humanAI{};
    float m_birthAccumulator{0.0f};
    float m_survivalTimer{0.0f};
    AIDifficulty m_aiDifficulty{AIDifficulty::Equilibrada};
    TribalBelligerence m_tribalBelligerence{TribalBelligerence::Equilibrada};
};

using HumanManager = GestorHumanos;

} // namespace ReligionSim
