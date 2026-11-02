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

class DayNightCycle {
public:
    // Duración de 1 ciclo diurno completo (24h de simulación): 90.0s reales a 1X
    explicit DayNightCycle(float dayDurationSeconds = 90.0f, float initialHour = 9.0f);

    void update(float dt);

    void setTimeOfDay(float t);
    void setTimeByHour(float hour);

    float getTimeOfDay() const { return m_timeOfDay; }
    float getHour() const { return m_timeOfDay * 24.0f; }

    DayPhase getPhase() const;
    bool isDay() const;
    bool isNight() const;

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
    float m_dayDuration{90.0f}; // Segundos reales de un ciclo completo de 24 horas
    float m_timeOfDay{0.375f};  // 0.375 * 24 = 9:00 AM (comienza en la mañana)
    float m_timer{0.0f};
    bool m_paused{false};

    float m_lightningFlashTimer{0.0f};
    float m_lightningFlashDuration{0.25f};
};

} // namespace ReligionSim
