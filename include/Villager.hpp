#pragma once

#include "Types.hpp"
#include <string>
#include <vector>

namespace ReligionSim {

class WorldMap;
class ReligionRegistry;

enum class VillagerActivity {
    Idle,
    Walking,
    PrayingAtShrine,
    ChoppingGathering,
    CarryingRelic,
    Greeting
};

struct Villager {
    int id{0};
    std::string name{"Grog"};
    int cellX{20};
    int cellY{20};

    // Sub-tile continuous coordinates
    float worldX{20.5f};
    float worldY{20.5f};
    float targetX{20.5f};
    float targetY{20.5f};

    float moveSpeed{0.6f}; // Tiles per second
    VillagerActivity activity{VillagerActivity::Idle};
    IsoDirection direction{IsoDirection::SW};

    int religionId{1};
    float piety{0.7f};
    float stateTimer{2.0f};
    float animTimer{0.0f};

    std::string getSpriteName() const;
    void update(float dt, const WorldMap& map);
    void pickNewBehavior(const WorldMap& map);
};

class VillagerManager {
public:
    VillagerManager();

    void initVillagers(const WorldMap& map, const ReligionRegistry& registry, int count = 40);
    void update(float dt, const WorldMap& map);

    const std::vector<Villager>& getVillagers() const { return m_villagers; }
    std::vector<Villager>& getVillagers() { return m_villagers; }

    const Villager* getSelectedVillager() const;
    void selectVillager(int id);

private:
    std::vector<Villager> m_villagers;
    int m_selectedId{-1};
    int m_nextId{1};
};

} // namespace ReligionSim
