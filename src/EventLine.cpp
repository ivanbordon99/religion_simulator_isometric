#include "EventLine.hpp"
#include <algorithm>

namespace ReligionSim {

EventLine::EventLine(size_t maxEvents)
    : m_maxEvents(maxEvents) {
    m_events.reserve(maxEvents);
}

void EventLine::addEvent(const GameEvent& ev) {
    GameEvent item = ev;
    item.id = m_nextId++;
    if (m_events.size() >= m_maxEvents) {
        m_events.erase(m_events.begin());
    }
    m_events.push_back(item);
}

void EventLine::addEvent(int year, EventCategory cat, const std::string& title,
                        const std::string& desc, const Point2i& loc,
                        int relA, int relB, ColorRGB color,
                        bool isTurningPoint, const std::string& actor,
                        const std::string& consequence) {
    GameEvent ev;
    ev.year = year;
    ev.category = cat;
    ev.title = title;
    ev.description = desc;
    ev.location = loc;
    ev.religionAId = relA;
    ev.religionBId = relB;
    ev.badgeColor = color;
    ev.isTurningPoint = isTurningPoint;
    ev.actorName = actor;
    ev.consequence = consequence;
    addEvent(ev);
}

const GameEvent* EventLine::getEventById(int id) const {
    for (const auto& ev : m_events) {
        if (ev.id == id) return &ev;
    }
    return nullptr;
}

const GameEvent* EventLine::getSelectedEvent() const {
    if (m_selectedEventId < 0) return nullptr;
    return getEventById(m_selectedEventId);
}

void EventLine::selectEvent(int id) {
    m_selectedEventId = id;
}

void EventLine::clearSelection() {
    m_selectedEventId = -1;
}

std::vector<GameEvent> EventLine::getRecentEvents(size_t count) const {
    std::vector<GameEvent> result;
    if (m_events.empty()) return result;

    size_t n = std::min(count, m_events.size());
    result.insert(result.end(), m_events.end() - n, m_events.end());
    std::reverse(result.begin(), result.end());
    return result;
}

std::vector<GameEvent> EventLine::getEventsForYearRange(int startYear, int endYear) const {
    std::vector<GameEvent> result;
    for (const auto& ev : m_events) {
        if (ev.year >= startYear && ev.year <= endYear) {
            result.push_back(ev);
        }
    }
    return result;
}

std::vector<GameEvent> EventLine::getTurningPoints() const {
    std::vector<GameEvent> result;
    for (const auto& ev : m_events) {
        if (ev.isTurningPoint) {
            result.push_back(ev);
        }
    }
    return result;
}

std::vector<GameEvent> EventLine::getFilteredEvents(int filterIndex) const {
    if (filterIndex == 0) {
        return m_events; // Todos
    }
    if (filterIndex == 1) {
        return getTurningPoints(); // Puntos de quiebre
    }

    std::vector<GameEvent> result;
    for (const auto& ev : m_events) {
        bool match = false;
        switch (filterIndex) {
            case 2: // Pérdidas y Conflictos
                match = (ev.category == EventCategory::MajorLoss || ev.category == EventCategory::Extinction ||
                         ev.category == EventCategory::BorderConflict);
                break;
            case 3: // Avances
                match = (ev.category == EventCategory::EraAdvancement || ev.category == EventCategory::HolySiteErected);
                break;
            case 4: // Descubrimientos
                match = (ev.category == EventCategory::Discovery || ev.category == EventCategory::PilgrimageWave);
                break;
            case 5: // Cismas
                match = (ev.category == EventCategory::DoctrinalSchism || ev.category == EventCategory::ReligiousReformation);
                break;
            case 6: // Sincretismo y Alianzas
                match = (ev.category == EventCategory::SyncreticAgglutination || ev.category == EventCategory::ExogamicPact);
                break;
            default:
                match = true;
                break;
        }
        if (match) {
            result.push_back(ev);
        }
    }
    return result;
}

} // namespace ReligionSim
