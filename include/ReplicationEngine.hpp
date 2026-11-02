#pragma once

#include "Types.hpp"
#include "WorldMap.hpp"
#include "Religion.hpp"
#include <vector>

namespace ReligionSim {

struct MissionaryCaravan {
    Point2i from;
    Point2i to;
    int religionId;
    float progress; // 0.0 to 1.0
    float power;
};

class ReplicationEngine {
public:
    ReplicationEngine();

    void update(WorldMap& map, ReligionRegistry& registry, float dtYears,
                std::vector<GameEvent>& outEvents, int currentYear);

    const std::vector<MissionaryCaravan>& getActiveCaravans() const { return m_caravans; }

    AIDifficulty getAIDifficulty() const { return m_aiDifficulty; }
    void setAIDifficulty(AIDifficulty diff) { m_aiDifficulty = diff; }
    TribalBelligerence getTribalBelligerence() const { return m_tribalBelligerence; }
    void setTribalBelligerence(TribalBelligerence bel) { m_tribalBelligerence = bel; }

private:
    std::vector<MissionaryCaravan> m_caravans;
    AIDifficulty m_aiDifficulty{AIDifficulty::Equilibrada};
    TribalBelligerence m_tribalBelligerence{TribalBelligerence::Equilibrada};

    void processDemographics(WorldMap& map, const ReligionRegistry& registry, float dtYears);
    void processSpatialDiffusion(WorldMap& map, ReligionRegistry& registry, float dtYears);
    void processMissionaryCaravans(WorldMap& map, ReligionRegistry& registry, float dtYears,
                                  std::vector<GameEvent>& outEvents, int currentYear);
    void processBorderTensionAndLiminalInteractions(WorldMap& map, ReligionRegistry& registry, float dtYears,
                                                   std::vector<GameEvent>& outEvents, int currentYear);
};

} // namespace ReligionSim
