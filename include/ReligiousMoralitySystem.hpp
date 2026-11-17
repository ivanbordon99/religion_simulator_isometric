#pragma once

#include "Types.hpp"
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <cmath>

namespace ReligionSim {

class Humano;
class GestorHumanos;
class WorldMap;
class ReligionRegistry;
class EventLine;
class SoundSystem;
class IsometricCamera;

// Tipos de Mandatos y Tabúes Sagrados
enum class MoralRuleType : uint8_t {
    SacredKinProtection = 0,    // Tabú de Sangre Fraterna: prohibido dañar a miembros del mismo clan o fe
    HolyOfferingTithe = 1,      // Mandato del Diezmo Sagrado: obligación de entregar ofrendas en templos
    SanctuaryDefense = 2,       // Mandato de Custodia de Santuarios: deber de defender altares sagrados
    ZealousHolyWar = 3,         // Mandato de Guerra Santa: gloria en el combate contra cultos sacrílegos
    AsceticFasting = 4,         // Voto de Ayuno y Templanza: pureza ritual mediante la renuncia y oración
    SacredHospitality = 5,      // Voto de Hospitalidad Sagrada: bienvenida al forastero pacífico y comercio
    AncestorVeneration = 6,     // Culto y Reverencia a los Ancestros: honras fúnebres sagradas y memoria
    HierarchicalSubmission = 7  // Mandato de Sumisión al Chamán/Profeta: obediencia estricta a la voz viva
};

inline const char* getMoralRuleName(MoralRuleType t) {
    switch (t) {
        case MoralRuleType::SacredKinProtection:    return "Tabu de Sangre Fraterna";
        case MoralRuleType::HolyOfferingTithe:      return "Mandato de la Ofrenda Sagrada";
        case MoralRuleType::SanctuaryDefense:       return "Custodia de Santuarios";
        case MoralRuleType::ZealousHolyWar:         return "Celo de Guerra Santa";
        case MoralRuleType::AsceticFasting:         return "Voto de Ayuno y Templanza";
        case MoralRuleType::SacredHospitality:      return "Hospitalidad Sagrada";
        case MoralRuleType::AncestorVeneration:     return "Reverencia a los Ancestros";
        case MoralRuleType::HierarchicalSubmission: return "Sumision al Lider Divino";
        default: return "Mandato Desconocido";
    }
}

// Estados de Gracia Moral Colectiva de la Religión
enum class MoralGraceStatus : uint8_t {
    DivineBlessing = 0,     // Gracia >= 80%: Bendición Divina activa (+25% Teología, alta devoción, armonía)
    Righteous = 1,          // Gracia 55-79%: Estado de Rectitud Virtuosa ordinaria
    TenuousGrace = 2,       // Gracia 35-54%: Gracia Tenue (fricción por pecados sin expiar)
    MoralCrisis = 3,        // Gracia 15-34%: Crisis Moral y Penitencia comunitaria
    HereticalDecadence = 4  // Gracia < 15%: Decadencia Herética y riesgo extremo de cisma
};

inline const char* getMoralGraceStatusName(MoralGraceStatus s) {
    switch (s) {
        case MoralGraceStatus::DivineBlessing:    return "Bendicion Divina";
        case MoralGraceStatus::Righteous:         return "Rectitud Virtuosa";
        case MoralGraceStatus::TenuousGrace:      return "Gracia Tenue";
        case MoralGraceStatus::MoralCrisis:       return "Crisis Moral";
        case MoralGraceStatus::HereticalDecadence: return "Decadencia Heretica";
        default: return "Indeterminado";
    }
}

// Definición doctrinal de un mandato o tabú
struct MoralRuleDefinition {
    MoralRuleType type{MoralRuleType::SacredKinProtection};
    std::string name;
    std::string commandment;       // Precepto / Texto canónico sagrado
    std::string virtueDescription; // Efecto positivo al cumplirlo
    std::string sinDescription;    // Qué transgresión constituye pecado y culpa
    int theologyCost{100};          // Costo de fe para promulgar
    HistoricalEra requiredEra{HistoricalEra::PrimordialAnimism};
    float guiltSeverity{30.0f};    // Culpa infligida al violar la regla
};

// Registro Moral Individual de cada Fiel (Humano)
struct UnitMoralRecord {
    float moralIntegrity{0.88f};        // 0.0f (Depravado / Roto) a 1.0f (Santo / Puro)
    float guiltLevel{0.0f};              // 0.0f a 100.0f (Mancha espiritual)
    float penanceProgress{0.0f};         // 0.0f a 100.0f (Progreso de expiación)
    bool isApostateOrHeretic{false};     // Si se convierte en disidente por exceso de culpa
    int sinsCommittedCount{0};
    int righteousActsCount{0};
    std::string lastTransgression{"Ninguna"};
    int lastTransgressionYear{-4000};

    [[nodiscard]] bool hasGuilt() const { return guiltLevel > 0.5f; }
    [[nodiscard]] bool isVirtuous() const { return moralIntegrity >= 0.82f && guiltLevel < 5.0f; }

    void addGuilt(float amount, const std::string& reason, int year) {
        guiltLevel = std::clamp(guiltLevel + amount, 0.0f, 100.0f);
        moralIntegrity = std::clamp(moralIntegrity - amount * 0.005f, 0.05f, 1.0f);
        sinsCommittedCount++;
        lastTransgression = reason;
        lastTransgressionYear = year;
        if (guiltLevel >= 70.0f || moralIntegrity <= 0.35f) {
            isApostateOrHeretic = true;
        }
    }

    void absolvePenance(float amount) {
        guiltLevel = std::max(0.0f, guiltLevel - amount);
        moralIntegrity = std::min(1.0f, moralIntegrity + amount * 0.003f);
        if (guiltLevel <= 15.0f) {
            isApostateOrHeretic = false;
        }
    }

    void addRighteousAct(float integrityBoost = 0.04f) {
        righteousActsCount++;
        moralIntegrity = std::min(1.0f, moralIntegrity + integrityBoost);
        guiltLevel = std::max(0.0f, guiltLevel - 2.5f);
    }
};

// Perfil Moral Comunitario de una Religión
struct ReligionMoralProfile {
    int religionId{0};
    std::vector<MoralRuleType> activeRules; // Hasta 4 mandatos activos simultáneos
    float communalGrace{85.0f};             // 0.0f a 100.0f
    int totalVirtuesCount{0};
    int totalTransgressionsCount{0};
    float moralPurityRatio{0.90f};          // Ratio de fieles virtuosos vs transgresores
    int totalApostatesCount{0};

    [[nodiscard]] bool hasRule(MoralRuleType r) const {
        return std::find(activeRules.begin(), activeRules.end(), r) != activeRules.end();
    }

    bool activateRule(MoralRuleType r) {
        if (hasRule(r) || activeRules.size() >= 4) return false;
        activeRules.push_back(r);
        return true;
    }

    bool deactivateRule(MoralRuleType r) {
        auto it = std::find(activeRules.begin(), activeRules.end(), r);
        if (it != activeRules.end()) {
            activeRules.erase(it);
            return true;
        }
        return false;
    }

    [[nodiscard]] MoralGraceStatus getStatus() const {
        if (communalGrace >= 80.0f) return MoralGraceStatus::DivineBlessing;
        if (communalGrace >= 55.0f) return MoralGraceStatus::Righteous;
        if (communalGrace >= 35.0f) return MoralGraceStatus::TenuousGrace;
        if (communalGrace >= 15.0f) return MoralGraceStatus::MoralCrisis;
        return MoralGraceStatus::HereticalDecadence;
    }
};

// Sistema Central de Moral Religiosa y Reglas
class ReligiousMoralitySystem {
public:
    ReligiousMoralitySystem();

    // Catálogo doctrinal
    [[nodiscard]] const std::vector<MoralRuleDefinition>& getCatalog() const { return m_catalog; }
    [[nodiscard]] const MoralRuleDefinition* getRuleDef(MoralRuleType type) const;

    // Gestión de perfiles morales por religión
    ReligionMoralProfile& getOrCreateProfile(int religionId);
    [[nodiscard]] const ReligionMoralProfile* getProfile(int religionId) const;

    // Promulgación y derogación de mandatos mediante Puntos de Teología
    bool enactRule(int religionId, MoralRuleType rule, ReligionRegistry& registry);
    bool repealRule(int religionId, MoralRuleType rule);
    [[nodiscard]] bool isRuleActive(int religionId, MoralRuleType rule) const;

    // Registro de actos morales individuales y colectivos
    void recordTransgression(Humano& unit, MoralRuleType violatedRule, const std::string& detail,
                             int currentYear, EventLine* eventLine = nullptr, ReligionRegistry* registry = nullptr);

    void recordRighteousAct(Humano& unit, MoralRuleType followedRule, float merit,
                            int currentYear, ReligionRegistry* registry = nullptr);

    // Proceso de penitencia, expiación y absolución sacerdotal en el templo o altar
    void processPenance(Humano& unit, float dt, bool atHolySite, bool shamanNearby,
                        ReligionRegistry* registry = nullptr, SoundSystem* soundSystem = nullptr,
                        const IsometricCamera* camera = nullptr);

    // Actualización cíclica de la moral comunitaria
    void update(float dtYears, int currentYear, GestorHumanos& gestor,
                WorldMap& map, ReligionRegistry& registry, EventLine* eventLine);

    // Métricas y acceso rápido
    [[nodiscard]] float getCommunalGrace(int religionId) const;
    [[nodiscard]] MoralGraceStatus getMoralStatus(int religionId) const;

    // Serialización / Persistencia
    [[nodiscard]] std::string serialize() const;
    void deserialize(const std::string& data);

    // EN: Purge all religious profiles and reset moral records
    // ES: Purgar todos los perfiles religiosos y reiniciar registros morales
    void clear() {
        m_profiles.clear();
    }

private:
    std::vector<MoralRuleDefinition> m_catalog;
    std::map<int, ReligionMoralProfile> m_profiles;

    void initCatalog();
};

} // namespace ReligionSim
