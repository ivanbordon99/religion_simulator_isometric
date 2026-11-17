#include "Timeline.hpp"
#include "DayNightCycle.hpp"
#include <algorithm>
#include <cmath>
#include <sstream>

namespace ReligionSim {

Timeline::Timeline(int startYear, int endYear)
    : m_startYear(startYear), m_endYear(endYear), m_currentYear(startYear) {
    m_history.reserve(256);
}

void Timeline::update(float deltaTime, float& outDtYears) {
    outDtYears = 0.0f;
    if (m_isPaused || m_currentYear >= m_endYear) return;

    // Simulation pace: 1 real second = baseYearsPerSec at 1x
    float baseYearsPerSec = m_yearsPerSecond;
    float deltaYears = deltaTime * baseYearsPerSec * static_cast<float>(m_speedMultiplier);

    m_tickAccumulator += deltaYears;
    int step = static_cast<int>(m_tickAccumulator);
    if (step >= 1) {
        m_currentYear = std::min(m_endYear, m_currentYear + step);
        m_tickAccumulator -= static_cast<float>(step);
        outDtYears = static_cast<float>(step);
    }
}

// EN: Synchronize timeline progression with DayNightCycle hours and advancement maturity
// ES: Sincronizar el progreso de la línea temporal con las horas de DayNightCycle y la madurez de avances
void Timeline::updateSynchronized(float deltaTime, const DayNightCycle& dayNight, float& outDtYears) {
    (void)deltaTime;
    outDtYears = 0.0f;
    if (m_isPaused || m_currentYear >= m_endYear) return;

    // EN: The 24-hour solar cycle maps strictly to the current annual cycle (00:00 to 24:00 = 1 year)
    // ES: El ciclo solar de 24 horas se mapea estrictamente al ciclo anual actual (00:00 a 24:00 = 1 año)
    m_yearFraction = dayNight.getTimeOfDay();

    // EN: Calculate target year derived directly from completed days plus advancement bonuses
    // ES: Calcular el año objetivo derivado directamente de los días completados más las bonificaciones de avances
    const int totalDaysCompleted = dayNight.getDaysPassed();
    const int calculatedYear = m_startYear + totalDaysCompleted + m_advancementBonusYears;

    if (calculatedYear > m_currentYear) {
        const int yearStepDelta = calculatedYear - m_currentYear;
        m_currentYear = std::min(m_endYear, calculatedYear);
        outDtYears = static_cast<float>(yearStepDelta);
    }
}

// EN: Apply historical civilizational leap from convalidated theological advancements or capstones
// ES: Aplicar salto histórico civilizatorio por avances teológicos o hitos de era convalidados
void Timeline::applyAdvancementHistoricalLeap(int leapYears, float& outDtYears) {
    if (leapYears <= 0) return;
    m_advancementBonusYears += leapYears;
    const int previousRecordedYear = m_currentYear;
    m_currentYear = std::min(m_endYear, m_currentYear + leapYears);
    const int actualAdvanceDelta = m_currentYear - previousRecordedYear;
    if (actualAdvanceDelta > 0) {
        outDtYears += static_cast<float>(actualAdvanceDelta);
    }
}

float Timeline::getTimelineProgress() const {
    float span = static_cast<float>(m_endYear - m_startYear);
    if (span <= 0.0f) return 0.0f;
    return std::clamp(static_cast<float>(m_currentYear - m_startYear) / span, 0.0f, 1.0f);
}

void Timeline::scrubToProgress(float progress) {
    progress = std::clamp(progress, 0.0f, 1.0f);
    float span = static_cast<float>(m_endYear - m_startYear);
    m_currentYear = m_startYear + static_cast<int>(span * progress);
}

void Timeline::setSpeedMultiplier(int speed) {
    m_speedMultiplier = std::clamp(speed, 1, 10);
}

void Timeline::cycleSpeed() {
    if (m_speedMultiplier == 1) m_speedMultiplier = 2;
    else if (m_speedMultiplier == 2) m_speedMultiplier = 5;
    else if (m_speedMultiplier == 5) m_speedMultiplier = 10;
    else m_speedMultiplier = 1;
}

void Timeline::recordSnapshot(int totalPop, const std::vector<uint8_t>& domMap,
                             const std::vector<std::pair<int, int>>& relFollowers) {
    if (m_currentYear - m_lastSnapshotYear < 20) return;

    HistorySnapshot snap;
    snap.year = m_currentYear;
    snap.totalWorldPop = totalPop;
    snap.dominantReligions = domMap;
    snap.religionFollowers = relFollowers;

    m_history.push_back(snap);
    m_lastSnapshotYear = m_currentYear;

    // EN: Guard against unbounded memory growth by keeping snapshots capped
    // ES: Evitar crecimiento ilimitado de memoria manteniendo un tope en las capturas
    if (m_history.size() > 400) {
        trimHistory(300);
    }
}

// =============================================================================
// EN: Prune oldest historical snapshots while preserving modern timeline records
// ES: Podar las capturas históricas más antiguas preservando los registros recientes
// =============================================================================
void Timeline::trimHistory(size_t maximumSnapshotCount) {
    if (m_history.size() <= maximumSnapshotCount || maximumSnapshotCount == 0) {
        return;
    }
    const size_t excessCount = m_history.size() - maximumSnapshotCount;
    m_history.erase(m_history.begin(), m_history.begin() + excessCount);
}

const HistorySnapshot* Timeline::getSnapshotNearYear(int year) const {
    if (m_history.empty()) return nullptr;

    const HistorySnapshot* best = &m_history[0];
    int bestDiff = std::abs(best->year - year);

    for (const auto& snap : m_history) {
        int diff = std::abs(snap.year - year);
        if (diff < bestDiff) {
            bestDiff = diff;
            best = &snap;
        }
    }
    return best;
}

std::string Timeline::formatYear(int year) {
    std::ostringstream ss;
    if (year < 0) {
        ss << (-year) << " BCE";
    } else if (year == 0) {
        ss << "1 CE";
    } else {
        ss << year << " CE";
    }
    return ss.str();
}

} // namespace ReligionSim
