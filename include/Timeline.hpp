#pragma once

#include "Types.hpp"
#include <vector>
#include <string>
#include <cstdint>

namespace ReligionSim {

class DayNightCycle;

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

    // EN: Synchronize timeline with in-game DayNightCycle 24h hours and applied advancements
    // ES: Sincronizar línea temporal con las horas de DayNightCycle y los avances aplicados
    void updateSynchronized(float deltaTime, const DayNightCycle& dayNight, float& outDtYears);

    // EN: Apply historical momentum from convalidating theological doctrines or capstones
    // ES: Aplicar impulso histórico por convalidar doctrinas teológicas o hitos de era
    void applyAdvancementHistoricalLeap(int leapYears, float& outDtYears);

    // EN: Get sub-year fraction (0.0 to 1.0) strictly corresponding to the 24-hour cycle
    // ES: Obtener fracción sub-anual (0.0 a 1.0) correspondiente estrictamente al ciclo de 24 horas
    float getYearFraction() const { return m_yearFraction; }

    int getAdvancementBonusYears() const { return m_advancementBonusYears; }
    void setAdvancementBonusYears(int bonusYears) { m_advancementBonusYears = bonusYears; }

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
        m_yearFraction = 0.0f;
        m_advancementBonusYears = 0;
        m_lastDaySeen = 0;
        m_lastHourSeen = 0.0f;
        m_history.clear();
        m_isPaused = false;
        m_speedMultiplier = 1;
        m_tickAccumulator = 0.0f;
    }

    static std::string formatYear(int year);

private:
    int m_startYear{-4000};
    int m_endYear{2200};
    int m_currentYear{-4000};

    float m_yearFraction{0.0f};
    int m_advancementBonusYears{0};
    int m_lastDaySeen{0};
    float m_lastHourSeen{0.0f};

    bool m_isPaused{false};
    int m_speedMultiplier{1}; // 1x, 2x, 5x, 10x
    float m_yearsPerSecond{4.0f};
    float m_tickAccumulator{0.0f};
    int m_lastSnapshotYear{-9999};

    std::vector<HistorySnapshot> m_history;
};

} // namespace ReligionSim
