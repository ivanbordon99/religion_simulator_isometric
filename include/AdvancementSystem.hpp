#pragma once

#include "Types.hpp"
#include "Religion.hpp"
#include <vector>
#include <string>

namespace ReligionSim {

struct AdvancementDoc {
    DoctrineType type;
    std::string name;
    std::string description;
    EraType era;
    int theologyCost;
    float spreadModifier;
    float syncretismModifier;
    float cohesionModifier;
};

enum class FaithBranch : uint8_t {
    ReplicationSpread = 0,   // Replicación y difusión misionera
    HarvestDemographics = 1, // Crecimiento demográfico, cosechas y templos
    DogmaCohesion = 2,       // Cohesión dogmática y estabilidad anti-cisma
    EraCapstone = 3          // Hito de ascensión de Era histórica
};

struct FaithAdvancement {
    int id{0};
    std::string name;
    std::string shortTitle;
    std::string description;
    std::string mechanicalBonus;
    HistoricalEra era{HistoricalEra::PrimordialAnimism};
    FaithBranch branch{FaithBranch::ReplicationSpread};
    int tier{0}; // 0 = Raíz, 1 = Intermedio, 2 = Mayor, 3 = Capstone
    int faithCost{100};
    std::vector<int> prerequisites;
    bool isEraCapstone{false};
    HistoricalEra unlocksEra{HistoricalEra::PrimordialAnimism};
    int iconCol{0}; // Índice de columna en assets/ui/faith_tree_icons.rgba (0..7)

    // Bonificaciones mecánicas
    float zealBonus{0.0f};
    float diffusionBonus{0.0f};
    float cohesionBonus{0.0f};
    float popGrowthBonus{0.0f};
    float holySiteBonus{0.0f};
    int passiveFaithPerYear{0};
};

class AdvancementSystem {
public:
    AdvancementSystem();

    void updateAdvancements(ReligionRegistry& registry, int currentYear,
                            std::vector<GameEvent>& outEvents);

    static std::string getEraName(HistoricalEra era);
    static std::string getEraDescription(HistoricalEra era);
    static HistoricalEra getEraForYear(int year);
    static int getEraStartYear(HistoricalEra era);
    static const EraConfig& getEraConfig(HistoricalEra era);

    const AdvancementDoc* getDocInfo(DoctrineType doc) const;

    // Árbol de Avances de la Fe
    const std::vector<FaithAdvancement>& getAllAdvancements() const { return m_advancementTree; }
    const FaithAdvancement* getAdvancement(int id) const;
    bool canConvalidate(const Religion& rel, int advId) const;
    bool convalidateAdvancement(Religion& rel, int advId, int currentYear,
                                std::vector<GameEvent>& outEvents);
    int getConvalidatedCountForEra(const Religion& rel, HistoricalEra era) const;
    int getTotalAdvancementsInEra(HistoricalEra era) const;

private:
    std::vector<AdvancementDoc> m_doctrineCatalog;
    std::vector<FaithAdvancement> m_advancementTree;
};

} // namespace ReligionSim
