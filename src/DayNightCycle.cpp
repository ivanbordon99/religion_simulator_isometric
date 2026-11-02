#include "DayNightCycle.hpp"
#include <cstdio>
#include <array>

namespace ReligionSim {

namespace {

struct ColorKeyframe {
    float hour;
    ColorRGB color;
};

ColorRGB interpolateKeyframes(const std::vector<ColorKeyframe>& keys, float hour) {
    if (keys.empty()) return ColorRGB{255, 255, 255};
    if (hour <= keys.front().hour) return keys.front().color;
    if (hour >= keys.back().hour) return keys.back().color;

    for (size_t i = 0; i + 1 < keys.size(); ++i) {
        if (hour >= keys[i].hour && hour <= keys[i + 1].hour) {
            float span = keys[i + 1].hour - keys[i].hour;
            float t = (span > 0.0001f) ? (hour - keys[i].hour) / span : 0.0f;
            float s = t * t * (3.0f - 2.0f * t); // Smoothstep

            const auto& c0 = keys[i].color;
            const auto& c1 = keys[i + 1].color;
            return {
                static_cast<uint8_t>(std::clamp(c0.r + (c1.r - c0.r) * s, 0.0f, 255.0f)),
                static_cast<uint8_t>(std::clamp(c0.g + (c1.g - c0.g) * s, 0.0f, 255.0f)),
                static_cast<uint8_t>(std::clamp(c0.b + (c1.b - c0.b) * s, 0.0f, 255.0f)),
                255
            };
        }
    }
    return keys.back().color;
}

} // anonymous namespace

DayNightCycle::DayNightCycle(float dayDurationSeconds, float initialHour)
    : m_dayDuration(dayDurationSeconds > 1.0f ? dayDurationSeconds : 90.0f) {
    setTimeByHour(initialHour);
}

void DayNightCycle::update(float dt) {
    if (m_lightningFlashTimer > 0.0f) {
        m_lightningFlashTimer = std::max(0.0f, m_lightningFlashTimer - dt);
    }
    if (m_paused || dt <= 0.0f) return;

    m_timer += dt;
    m_timeOfDay += dt / m_dayDuration;
    while (m_timeOfDay >= 1.0f) m_timeOfDay -= 1.0f;
    while (m_timeOfDay < 0.0f) m_timeOfDay += 1.0f;
}

void DayNightCycle::setTimeOfDay(float t) {
    m_timeOfDay = std::clamp(t, 0.0f, 1.0f);
    if (m_timeOfDay >= 1.0f) m_timeOfDay = 0.0f;
}

void DayNightCycle::setTimeByHour(float hour) {
    float h = std::fmod(hour, 24.0f);
    if (h < 0.0f) h += 24.0f;
    setTimeOfDay(h / 24.0f);
}

DayPhase DayNightCycle::getPhase() const {
    float h = getHour();
    if (h < 5.0f) return DayPhase::DeepNight;
    if (h < 7.5f) return DayPhase::Dawn;
    if (h < 17.0f) return DayPhase::Day;
    if (h < 20.0f) return DayPhase::Dusk;
    return DayPhase::Night;
}

bool DayNightCycle::isDay() const {
    float h = getHour();
    return (h >= 6.0f && h < 18.5f);
}

bool DayNightCycle::isNight() const {
    return !isDay();
}

std::string DayNightCycle::getPhaseName() const {
    switch (getPhase()) {
        case DayPhase::DeepNight: return "Madrugada";
        case DayPhase::Dawn:      return "Amanecer";
        case DayPhase::Day:       return "Mediodía";
        case DayPhase::Dusk:      return "Atardecer";
        case DayPhase::Night:     return "Noche";
    }
    return "Día";
}

std::string DayNightCycle::getTimeFormatted() const {
    float h = getHour();
    int hourInt = static_cast<int>(std::floor(h));
    int minInt = static_cast<int>(std::floor((h - hourInt) * 60.0f));
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%02d:%02d", hourInt, minInt);
    return std::string(buf);
}

ColorRGB DayNightCycle::getAmbientColor() const {
    static const std::vector<ColorKeyframe> ambientKeys = {
        { 0.0f,  ColorRGB{45, 55, 115} },   // Medianoche: Índigo lunar profundo
        { 4.5f,  ColorRGB{48, 58, 118} },   // Noche tardía
        { 5.8f,  ColorRGB{145, 85, 105} },  // Pre-aurora rosada violácea
        { 6.6f,  ColorRGB{220, 150, 115} }, // Amanecer: Cálido melocotón dorado
        { 7.8f,  ColorRGB{245, 215, 175} }, // Mañana radiante
        { 12.0f, ColorRGB{255, 252, 240} }, // Mediodía: Pleno sol cenital limpio
        { 16.5f, ColorRGB{250, 215, 165} }, // Tarde dorada
        { 18.2f, ColorRGB{245, 155, 75} },  // Ocaso / Hora dorada: Ámbar brillante
        { 19.4f, ColorRGB{220, 105, 60} },  // Puesta de sol: Rojo sangre y oro
        { 20.4f, ColorRGB{115, 70, 105} },  // Crepúsculo: Violeta carmesí
        { 21.6f, ColorRGB{55, 65, 125} },   // Anochecer azul marino
        { 24.0f, ColorRGB{45, 55, 115} }    // Medianoche cíclica
    };

    return interpolateKeyframes(ambientKeys, getHour());
}

ColorRGB DayNightCycle::getSkyZenithColor() const {
    static const std::vector<ColorKeyframe> zenithKeys = {
        { 0.0f,  ColorRGB{8, 10, 24} },     // Cosmos oscuro
        { 5.5f,  ColorRGB{10, 12, 28} },
        { 6.5f,  ColorRGB{38, 25, 65} },    // Violeta amanecer
        { 8.0f,  ColorRGB{30, 75, 160} },
        { 12.0f, ColorRGB{32, 95, 195} },   // Azul cerúleo puro cenital
        { 16.5f, ColorRGB{30, 85, 175} },
        { 18.5f, ColorRGB{35, 20, 60} },    // Violeta atardecer
        { 20.5f, ColorRGB{12, 14, 32} },
        { 24.0f, ColorRGB{8, 10, 24} }
    };

    return interpolateKeyframes(zenithKeys, getHour());
}

ColorRGB DayNightCycle::getSkyHorizonColor() const {
    static const std::vector<ColorKeyframe> horizonKeys = {
        { 0.0f,  ColorRGB{14, 18, 38} },    // Bruma nocturna
        { 5.5f,  ColorRGB{20, 24, 46} },
        { 6.5f,  ColorRGB{255, 175, 75} },  // Resplandor coral dorado de la alborada
        { 8.0f,  ColorRGB{180, 215, 250} },
        { 12.0f, ColorRGB{165, 215, 250} }, // Atmósfera dorada diurna
        { 16.5f, ColorRGB{220, 195, 170} },
        { 18.5f, ColorRGB{245, 115, 35} },  // Ocaso carmesí ámbar llameante
        { 20.5f, ColorRGB{24, 28, 55} },
        { 24.0f, ColorRGB{14, 18, 38} }
    };

    return interpolateKeyframes(horizonKeys, getHour());
}

bool DayNightCycle::getSunTransform(int screenW, int screenH, float& outX, float& outY, float& outAlpha) const {
    float h = getHour();
    // El Sol se eleva en el horizonte Este a las 05:15 y se oculta en el Oeste a las 19:45
    constexpr float sunriseH = 5.2f;
    constexpr float sunsetH = 19.8f;
    constexpr float daySpan = sunsetH - sunriseH;

    if (h < sunriseH || h > sunsetH) {
        outAlpha = 0.0f;
        return false;
    }

    float sunProg = (h - sunriseH) / daySpan; // 0.0 a 1.0
    float startX = screenW * 0.12f;
    float endX = screenW * 0.88f;
    outX = startX + sunProg * (endX - startX);

    // Arco parabólico cenital
    float zenithY = 52.0f;
    float horizonY = std::min(210.0f, screenH * 0.32f);
    float arch = std::sin(3.14159265f * sunProg);
    outY = horizonY - arch * (horizonY - zenithY);

    outAlpha = std::clamp(arch * 2.5f, 0.0f, 1.0f);
    return true;
}

bool DayNightCycle::getMoonTransform(int screenW, int screenH, float& outX, float& outY, float& outAlpha) const {
    float h = getHour();
    // La Luna emerge en el horizonte a las 18:15 y se pone a las 06:45
    constexpr float moonriseH = 18.2f;
    constexpr float moonsetH = 6.8f;
    constexpr float nightSpan = 12.6f;

    bool isMoonTime = (h >= moonriseH || h <= moonsetH);
    if (!isMoonTime) {
        outAlpha = 0.0f;
        return false;
    }

    float nightElapsed = (h >= moonriseH) ? (h - moonriseH) : (h + (24.0f - moonriseH));
    float moonProg = nightElapsed / nightSpan; // 0.0 a 1.0

    float startX = screenW * 0.12f;
    float endX = screenW * 0.88f;
    outX = startX + moonProg * (endX - startX);

    float zenithY = 56.0f;
    float horizonY = std::min(210.0f, screenH * 0.32f);
    float arch = std::sin(3.14159265f * moonProg);
    outY = horizonY - arch * (horizonY - zenithY);

    outAlpha = std::clamp(arch * 2.5f, 0.0f, 1.0f);
    return true;
}

bool DayNightCycle::getSunWorldPosition(int mapWidth, int mapHeight, float& outWorldX, float& outWorldY, float& outAltitude, float& outAlpha) const {
    float h = getHour();
    constexpr float sunriseH = 5.2f;
    constexpr float sunsetH = 19.8f;
    constexpr float daySpan = sunsetH - sunriseH;

    if (h < sunriseH || h > sunsetH) {
        outAlpha = 0.0f;
        return false;
    }

    float sunProg = (h - sunriseH) / daySpan; // 0.0 a 1.0
    float arch = std::sin(3.14159265f * sunProg);

    float centerX = static_cast<float>(mapWidth) * 0.5f;
    float centerY = static_cast<float>(mapHeight) * 0.5f;
    float offset = (sunProg - 0.5f); // -0.5 en el amanecer (Este), +0.5 en el atardecer (Oeste)
    float spanGrid = std::clamp(static_cast<float>(mapWidth) * 0.30f, 12.0f, 24.0f);

    outWorldX = centerX - offset * spanGrid;
    outWorldY = centerY + offset * spanGrid;

    // Altura celestial determinada sobre el mapa (35.0 a 48.0 pasos de elevación, muy por encima de las nubes)
    outAltitude = 35.0f + 13.0f * arch;
    outAlpha = std::clamp(arch * 2.5f, 0.0f, 1.0f);
    return true;
}

bool DayNightCycle::getMoonWorldPosition(int mapWidth, int mapHeight, float& outWorldX, float& outWorldY, float& outAltitude, float& outAlpha) const {
    float h = getHour();
    constexpr float moonriseH = 18.2f;
    constexpr float moonsetH = 6.8f;
    constexpr float nightSpan = 12.6f;

    bool isMoonTime = (h >= moonriseH || h <= moonsetH);
    if (!isMoonTime) {
        outAlpha = 0.0f;
        return false;
    }

    float nightElapsed = (h >= moonriseH) ? (h - moonriseH) : (h + (24.0f - moonriseH));
    float moonProg = nightElapsed / nightSpan; // 0.0 a 1.0
    float arch = std::sin(3.14159265f * moonProg);

    float centerX = static_cast<float>(mapWidth) * 0.5f;
    float centerY = static_cast<float>(mapHeight) * 0.5f;
    float offset = (moonProg - 0.5f);
    float spanGrid = std::clamp(static_cast<float>(mapWidth) * 0.30f, 12.0f, 24.0f);

    outWorldX = centerX - offset * spanGrid;
    outWorldY = centerY + offset * spanGrid;

    // Altura celestial determinada sobre el mapa (35.0 a 48.0 pasos de elevación, muy por encima de las nubes)
    outAltitude = 35.0f + 13.0f * arch;
    outAlpha = std::clamp(arch * 2.5f, 0.0f, 1.0f);
    return true;
}

void DayNightCycle::triggerLightningFlash(float duration) {
    m_lightningFlashDuration = (duration > 0.05f) ? duration : 0.25f;
    m_lightningFlashTimer = m_lightningFlashDuration;
}

float DayNightCycle::getLightningFlashIntensity() const {
    if (m_lightningFlashTimer <= 0.0f) return 0.0f;
    return std::clamp(m_lightningFlashTimer / m_lightningFlashDuration, 0.0f, 1.0f);
}

} // namespace ReligionSim
