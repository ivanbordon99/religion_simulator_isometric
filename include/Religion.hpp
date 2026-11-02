#pragma once

#include "Types.hpp"
#include <string>
#include <vector>
#include <unordered_set>

namespace ReligionSim {

// Arquetipos cosmogónicos antropológicos que definen la compatibilidad ontológica entre fes
enum class TheologicalArchetype : uint8_t {
    TotemicAnimism = 0,    // Espíritus de la naturaleza y tótems; permeable, alta adaptación, bajo exclusivismo
    SolarCelestial,        // Soberanía celestial / dios supremo; proselitista, orden moral, belicosidad expansiva
    ChthonicAncestral,     // Culto telúrico / huesos de antepasados; apego territorial radical, terror al forastero
    PolytheisticPantheon,  // Panteón asimilador; absorbe deidades rivales como cortesanos divinos menores
    DogmaticExclusivist    // Monoteísmo absoluto / purismo canónico; intolerancia al ídolo, cruzada o persecución
};

struct Religion {
    int id{0};
    std::string name;
    std::string founderName;
    std::string symbol{"Sun"};
    int foundedYear{-4000};
    ColorRGB color{255, 180, 50};
    TheologicalArchetype archetype{TheologicalArchetype::TotemicAnimism};

    // Core traits (0.0 to 1.0)
    float zeal{0.5f};                  // Conversion pressure and replication velocity
    float syncretismTolerance{0.5f};   // Willingness to blend, absorb, and agglutinate with other faiths
    float cohesion{0.7f};              // Dogmatic stability and resistance to schisms / heresies
    float spiritualPiety{0.6f};        // Devotion and generation of holy wonders / relics
    float scholasticism{0.2f};         // Written doctrine, theology development, speed of era advance

    // Current status
    HistoricalEra currentEra{HistoricalEra::PrimordialAnimism};
    int totalFollowers{0};
    int peakFollowers{0};
    int holySitesCount{0};
    int totalCellsDominant{0};
    bool isExtinct{false};

    // Advancement / Tree
    std::vector<DoctrineType> doctrines;
    std::vector<int> unlockedAdvancements; // IDs de avances teológicos convalidados en el árbol
    int theologyPoints{0};
    int advancementLevel{1};

    // Modificadores acumulativos de avances de la fe
    float diffusionMultiplier{1.0f};      // Multiplicador de difusión espacial
    float harvestYieldBonus{0.0f};        // Bonus a capacidad de población / cosechas en celdas dominadas
    float holySitePowerMultiplier{1.0f};  // Multiplicador de potencia en templos y caravanas
    int passiveFaithBonusPerYear{0};      // Puntos de fe / piedad pasivos generados anualmente

    // Recursos del pueblo / comunidad sagrada para su supervivencia
    VillageResources resources;

    // Lineage & Agglutination history
    int parentReligionId{0};          // 0 if primordial, or parent if schismatic branch
    int syncreticParentA{0};          // Non-zero if created via syncretic agglutination
    int syncreticParentB{0};
    int schismCount{0};

    bool hasDoctrine(DoctrineType d) const {
        for (auto doc : doctrines) {
            if (doc == d) return true;
        }
        return false;
    }

    void addDoctrine(DoctrineType d) {
        if (!hasDoctrine(d)) {
            doctrines.push_back(d);
        }
    }

    bool hasAdvancement(int advId) const {
        for (int id : unlockedAdvancements) {
            if (id == advId) return true;
        }
        return false;
    }

    void addAdvancement(int advId) {
        if (!hasAdvancement(advId)) {
            unlockedAdvancements.push_back(advId);
        }
    }

    // Calcula el coeficiente de fricción ontológica Φ(A, B) entre dos fes
    float calculateOntologicalFriction(const Religion& other) const {
        if (id == other.id) return 0.0f;
        if (archetype == TheologicalArchetype::DogmaticExclusivist ||
            other.archetype == TheologicalArchetype::DogmaticExclusivist) {
            return 2.10f; // Intolerancia radical al dogma ajeno
        }
        if ((archetype == TheologicalArchetype::SolarCelestial && other.archetype == TheologicalArchetype::ChthonicAncestral) ||
            (archetype == TheologicalArchetype::ChthonicAncestral && other.archetype == TheologicalArchetype::SolarCelestial)) {
            return 1.65f; // Luz y soberanía celestial vs culto ctónico/ancestral
        }
        if (archetype == TheologicalArchetype::PolytheisticPantheon ||
            other.archetype == TheologicalArchetype::PolytheisticPantheon) {
            return 0.45f; // Panteón abierto a asimilar nuevos númenes
        }
        if (archetype == TheologicalArchetype::TotemicAnimism &&
            other.archetype == TheologicalArchetype::TotemicAnimism) {
            return 0.28f; // Respeto mutuo y trueque entre clanes totémicos
        }
        return 0.90f; // Fricción estándar
    }
};

class ReligionRegistry {
public:
    ReligionRegistry();

    int createPrimordialReligion(const std::string& name, const std::string& founder,
                                 ColorRGB color, const std::string& symbol, int year);

    int createSyncreticReligion(int parentAId, int parentBId, int year, const Point2i& origin);

    int createSchismReligion(int parentId, int year, const Point2i& origin);

    Religion* getReligion(int id);
    const Religion* getReligion(int id) const;

    const std::vector<Religion>& getAllReligions() const { return m_religions; }
    std::vector<Religion>& getAllReligions() { return m_religions; }

    void updateStats(int totalWorldPop);

    static std::string generateName(const std::string& prefix, const std::string& suffix);
    static std::string generateProphetName();
    static ColorRGB generateDistinctColor(size_t index);

private:
    std::vector<Religion> m_religions;
    int m_nextId{1};
};

} // namespace ReligionSim
