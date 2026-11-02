#pragma once

#include "Types.hpp"
#include <vector>
#include <string>
#include <cstdint>

namespace ReligionSim {

struct HistorySnapshot {
    int year{-4000};
    int totalWorldPop{0};
    std::vector<uint8_t> dominantReligions; // flattened grid dominant rel ID
    std::vector<std::pair<int, int>> religionFollowers; // relId -> count
};

class Timeline {
public:
    Timeline(int startYear = -4000, int endYear = 2200);

    void update(float deltaTime, float& outDtYears);

    int getCurrentYear() const { return m_currentYear; }
    void setCurrentYear(int year) { m_currentYear = year; }

    int getStartYear() const { return m_startYear; }
    int getEndYear() const { return m_endYear; }

    float getTimelineProgress() const;
    void scrubToProgress(float progress);

    bool isPaused() const { return m_isPaused; }
    void togglePause() { m_isPaused = !m_isPaused; }
    void setPaused(bool paused) { m_isPaused = paused; }

    int getSpeedMultiplier() const { return m_speedMultiplier; }
    void setSpeedMultiplier(int speed);
    void cycleSpeed();

    float getYearsPerSecond() const { return m_yearsPerSecond; }
    void setYearsPerSecond(float yps) { m_yearsPerSecond = yps; }

    void recordSnapshot(int totalPop, const std::vector<uint8_t>& domMap,
                        const std::vector<std::pair<int, int>>& relFollowers);
    const std::vector<HistorySnapshot>& getHistory() const { return m_history; }
    const HistorySnapshot* getSnapshotNearYear(int year) const;

    // EN: Trim historical snapshots to cap memory usage and prevent vector bloat
    // ES: Podar instantáneas históricas para limitar el uso de memoria y evitar desbordes
    void trimHistory(size_t maximumSnapshotCount = 300);

    void reset(int startYear = -4000, int endYear = 2200) {
        m_startYear = startYear;
        m_endYear = endYear;
        m_currentYear = startYear;
        m_history.clear();
        m_isPaused = false;
        m_speedMultiplier = 1;
    }

    static std::string formatYear(int year);

private:
    int m_startYear{-4000};
    int m_endYear{2200};
    int m_currentYear{-4000};

    bool m_isPaused{false};
    int m_speedMultiplier{1}; // 1x, 2x, 5x, 10x
    float m_yearsPerSecond{4.0f};
    float m_tickAccumulator{0.0f};
    int m_lastSnapshotYear{-9999};

    std::vector<HistorySnapshot> m_history;
};

} // namespace ReligionSim
