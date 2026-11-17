#pragma once

#include "Types.hpp"
#include <string>
#include <cmath>
#include <algorithm>
#include <vector>

namespace ReligionSim {

enum class DayPhase : uint8_t {
    DeepNight, // Madrugada (00:00 - 05:00)
    Dawn,      // Amanecer / Aurora (05:00 - 07:30)
    Day,       // Mañana / Mediodía / Tarde (07:30 - 17:00)
    Dusk,      // Atardecer / Ocaso (17:00 - 20:00)
    Night      // Noche temprana (20:00 - 24:00)
};

// EN: Liturgical Season of the Year corresponding to the 24-hour solar cycle
// ES: Estación Litúrgica del Año correspondiente al ciclo solar de 24 horas
enum class LiturgicalSeason : uint8_t {
    Winter = 0,   // Invierno (00:00 - 06:00 / Noche profunda, frío y descanso)
    Spring = 1,   // Primavera (06:00 - 12:00 / Aurora, despertar y siembra)
    Summer = 2,   // Verano (12:00 - 18:00 / Cenit solar, cosecha y vigor)
    Autumn = 3    // Otoño (18:00 - 24:00 / Ocaso, agradecimiento y vigilia)
};

class DayNightCycle {
public:
    // Duración de 1 ciclo diurno completo (24h de simulación): 72.0s reales a 1X para ritmo contemplativo
    explicit DayNightCycle(float dayDurationSeconds = 72.0f, float initialHour = 9.0f);

    void update(float dt);

    void setTimeOfDay(float t);
    void setTimeByHour(float hour);

    float getTimeOfDay() const { return m_timeOfDay; }
    float getHour() const { return m_timeOfDay * 24.0f; }

    DayPhase getPhase() const;
    bool isDay() const;
    bool isNight() const;

    LiturgicalSeason getSeason() const;
    std::string getSeasonName() const;

    int getDaysPassed() const { return m_daysPassed; }
    void setDaysPassed(int days) { m_daysPassed = std::max(0, days); }
    void resetDaysPassed() { m_daysPassed = 0; }
    double getTotalHoursPassed() const { return static_cast<double>(m_daysPassed) * 24.0 + static_cast<double>(getHour()); }

    std::string getPhaseName() const;
    std::string getTimeFormatted() const;

    // Color ambiental modulador de la iluminación dinámica
    ColorRGB getAmbientColor() const;

    // Colores del domo celestial superior
    ColorRGB getSkyZenithColor() const;
    ColorRGB getSkyHorizonColor() const;

    // Proyección celestial del Sol y la Luna a lo largo del arco cenital
    bool getSunTransform(int screenW, int screenH, float& outX, float& outY, float& outAlpha) const;
    bool getMoonTransform(int screenW, int screenH, float& outX, float& outY, float& outAlpha) const;

    // Proyección celestial referenciada estrictamente al espacio del mapa mundial
    bool getSunWorldPosition(int mapWidth, int mapHeight, float& outWorldX, float& outWorldY, float& outAltitude, float& outAlpha) const;
    bool getMoonWorldPosition(int mapWidth, int mapHeight, float& outWorldX, float& outWorldY, float& outAltitude, float& outAlpha) const;

    void setPaused(bool p) { m_paused = p; }
    bool isPaused() const { return m_paused; }

    void setDayDuration(float sec) { if (sec > 1.0f) m_dayDuration = sec; }
    float getDayDuration() const { return m_dayDuration; }

    float getCycleTimer() const { return m_timer; }

    // Pulso de relámpago celestial (iluminación global momentánea por Rayo Divino)
    void triggerLightningFlash(float duration = 0.25f);
    float getLightningFlashIntensity() const;

private:
    float m_dayDuration{72.0f}; // Segundos reales de un ciclo completo de 24 horas (ritmo contemplativo)
    float m_timeOfDay{0.375f};  // 0.375 * 24 = 9:00 AM (comienza en la mañana)
    float m_timer{0.0f};
    int m_daysPassed{0};
    bool m_paused{false};

    float m_lightningFlashTimer{0.0f};
    float m_lightningFlashDuration{0.25f};
};

} // namespace ReligionSim
