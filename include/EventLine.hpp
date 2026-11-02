#pragma once

#include "Types.hpp"
#include <vector>
#include <deque>
#include <string>

namespace ReligionSim {

class EventLine {
public:
    EventLine(size_t maxEvents = 500);

    void addEvent(const GameEvent& ev);
    void addEvent(int year, EventCategory cat, const std::string& title,
                  const std::string& desc, const Point2i& loc,
                  int relA = 0, int relB = 0, ColorRGB color = {255, 200, 50},
                  bool isTurningPoint = false, const std::string& actor = "",
                  const std::string& consequence = "");

    const std::vector<GameEvent>& getAllEvents() const { return m_events; }
    const GameEvent* getEventById(int id) const;
    const GameEvent* getSelectedEvent() const;

    void selectEvent(int id);
    void clearSelection();

    // Event line feed (latest recent events)
    std::vector<GameEvent> getRecentEvents(size_t count = 6) const;
    std::vector<GameEvent> getEventsForYearRange(int startYear, int endYear) const;
    std::vector<GameEvent> getTurningPoints() const;
    std::vector<GameEvent> getFilteredEvents(int filterIndex) const;

    size_t size() const { return m_events.size(); }

private:
    size_t m_maxEvents{500};
    int m_nextId{1};
    int m_selectedEventId{-1};
    std::vector<GameEvent> m_events;
};

} // namespace ReligionSim
