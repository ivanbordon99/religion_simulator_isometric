#include "CloudSystem.hpp"
#include "DayNightCycle.hpp"
#include <random>

namespace ReligionSim {

CloudSystem::CloudSystem(int mapWidth, int mapHeight)
    : m_mapWidth(mapWidth), m_mapHeight(mapHeight) {
    initializeClouds();
}

void CloudSystem::reset(int mapWidth, int mapHeight) {
    m_mapWidth = mapWidth;
    m_mapHeight = mapHeight;
    m_clouds.clear();
    initializeClouds();
}

void CloudSystem::initializeClouds() {
    m_clouds.clear();
    m_clouds.reserve(20);

    // Semilla determinística para consistencia en la generación inicial
    uint32_t seed = 42135791u;
    auto lcg = [&seed]() {
        seed = seed * 1664525u + 1013904223u;
        return seed;
    };
    auto randF = [&lcg](float minV, float maxV) {
        float f = static_cast<float>(lcg() % 10000) / 10000.0f;
        return minV + f * (maxV - minV);
    };

    int cloudId = 1;

    // 1. Cúmulos algodonosos principales (10 cúmulos confinados dentro del mapa)
    for (int i = 0; i < 10; ++i) {
        CloudCluster c;
        c.id = cloudId++;
        c.type = CloudType::Cumulus;
        c.worldX = randF(2.0f, static_cast<float>(m_mapWidth) - 2.0f);
        c.worldY = randF(2.0f, static_cast<float>(m_mapHeight) - 2.0f);
        c.altitude = randF(7.6f, 8.6f);
        c.speedX = randF(-0.04f, 0.05f);
        c.speedY = randF(-0.02f, 0.03f);
        c.scale = randF(0.95f, 1.45f);
        c.baseAlpha = randF(0.72f, 0.88f);
        c.pulseTimer = randF(0.0f, 6.28f);

        // Generar lóbulos orgánicos interconectados (5 a 7 lóbulos)
        int numLobes = 5 + (lcg() % 3);
        c.lobes.reserve(numLobes);

        // Lóbulo central dominante
        c.lobes.push_back({0.0f, 0.0f, randF(2.6f, 3.4f), 0.90f, 1.25f, 0.85f});

        for (int l = 1; l < numLobes; ++l) {
            float angle = randF(0.0f, 6.283185f);
            float dist = randF(1.2f, 2.5f);
            float rad = randF(1.6f, 2.7f);
            float dens = randF(0.68f, 0.85f);
            float ax = randF(1.10f, 1.35f);
            float ay = randF(0.75f, 0.95f);
            c.lobes.push_back({std::cos(angle) * dist, std::sin(angle) * dist, rad, dens, ax, ay});
        }

        m_clouds.push_back(std::move(c));
    }

    // 2. Capas estratificadas horizontales (6 estratos dentro del mapa)
    for (int i = 0; i < 6; ++i) {
        CloudCluster c;
        c.id = cloudId++;
        c.type = CloudType::Stratus;
        c.worldX = randF(3.0f, static_cast<float>(m_mapWidth) - 3.0f);
        c.worldY = randF(3.0f, static_cast<float>(m_mapHeight) - 3.0f);
        c.altitude = randF(8.8f, 9.8f);
        c.speedX = randF(0.02f, 0.08f);
        c.speedY = randF(-0.01f, 0.03f);
        c.scale = randF(1.10f, 1.65f);
        c.baseAlpha = randF(0.55f, 0.70f);
        c.pulseTimer = randF(0.0f, 6.28f);

        int numLobes = 5 + (lcg() % 2);
        c.lobes.reserve(numLobes);
        c.lobes.push_back({0.0f, 0.0f, randF(3.0f, 4.2f), 0.78f, 1.65f, 0.70f});

        for (int l = 1; l < numLobes; ++l) {
            float spanX = randF(-3.2f, 3.2f);
            float spanY = randF(-1.0f, 1.0f);
            float rad = randF(2.0f, 3.0f);
            c.lobes.push_back({spanX, spanY, rad, randF(0.50f, 0.65f), 1.80f, 0.65f});
        }

        m_clouds.push_back(std::move(c));
    }

    // 3. Velos altos de cirros (4 cirros dentro del mapa)
    for (int i = 0; i < 4; ++i) {
        CloudCluster c;
        c.id = cloudId++;
        c.type = CloudType::Cirrus;
        c.worldX = randF(4.0f, static_cast<float>(m_mapWidth) - 4.0f);
        c.worldY = randF(4.0f, static_cast<float>(m_mapHeight) - 4.0f);
        c.altitude = randF(10.5f, 12.0f);
        c.speedX = randF(0.06f, 0.14f); // Vientos más rápidos en la alta troposfera
        c.speedY = randF(0.02f, 0.06f);
        c.scale = randF(1.3f, 2.0f);
        c.baseAlpha = randF(0.35f, 0.50f);
        c.pulseTimer = randF(0.0f, 6.28f);

        c.lobes.push_back({0.0f, 0.0f, randF(3.5f, 5.0f), 0.45f, 2.2f, 0.55f});
        c.lobes.push_back({randF(-3.0f, 3.0f), randF(-1.2f, 1.2f), randF(2.5f, 3.8f), 0.35f, 2.4f, 0.50f});
        c.lobes.push_back({randF(-4.5f, 4.5f), randF(-1.8f, 1.8f), randF(2.2f, 3.2f), 0.30f, 2.6f, 0.45f});

        m_clouds.push_back(std::move(c));
    }
}

void CloudSystem::update(float deltaTime) {
    if (deltaTime <= 0.0f) return;
    m_globalTime += deltaTime;

    const float minX = 0.0f;
    const float maxX = static_cast<float>(m_mapWidth);
    const float minY = 0.0f;
    const float maxY = static_cast<float>(m_mapHeight);

    for (auto& cloud : m_clouds) {
        float effectiveVx = m_windX + cloud.speedX;
        float effectiveVy = m_windY + cloud.speedY;

        cloud.worldX += effectiveVx * deltaTime;
        cloud.worldY += effectiveVy * deltaTime;
        cloud.pulseTimer += deltaTime * 0.35f;

        // Mantener las nubes estrictamente confinadas dentro de los límites del mapa
        if (cloud.worldX >= maxX) {
            cloud.worldX = minX;
            cloud.worldY = std::clamp(cloud.worldY + (cloud.speedY * 10.0f), 2.0f, maxY - 2.0f);
        } else if (cloud.worldX < minX) {
            cloud.worldX = maxX;
            cloud.worldY = std::clamp(cloud.worldY - (cloud.speedY * 10.0f), 2.0f, maxY - 2.0f);
        }

        if (cloud.worldY >= maxY) {
            cloud.worldY = minY;
            cloud.worldX = std::clamp(cloud.worldX + (cloud.speedX * 10.0f), 2.0f, maxX - 2.0f);
        } else if (cloud.worldY < minY) {
            cloud.worldY = maxY;
            cloud.worldX = std::clamp(cloud.worldX - (cloud.speedX * 10.0f), 2.0f, maxX - 2.0f);
        }
    }
}

float CloudSystem::calculateZoomVisibility(float zoom) {
    // Zoom Out (<= 0.85f): 100% visible desde la alta perspectiva panorámica
    if (zoom <= 0.85f) {
        return 1.0f;
    }
    // Zoom In (>= 1.15f): 0% invisible al descender y observar de cerca el terreno y los aldeanos
    if (zoom >= 1.15f) {
        return 0.0f;
    }

    // Transición suave cúbica (Hermite smoothstep) entre 0.85 y 1.15
    float t = (1.15f - zoom) / (1.15f - 0.85f);
    return t * t * (3.0f - 2.0f * t);
}

void CloudSystem::getLightingColors(const DayNightCycle* dayNight,
                                   ColorRGB& outSunlitColor,
                                   ColorRGB& outShadedColor,
                                   ColorRGB& outShadowColor,
                                   float& outShadowAlpha) const {
    if (!dayNight) {
        // Iluminación natural estándar por defecto
        outSunlitColor = ColorRGB{255, 255, 255};
        outShadedColor = ColorRGB{200, 212, 228};
        outShadowColor = ColorRGB{40, 50, 70};
        outShadowAlpha = 0.32f;
        return;
    }

    float hour = dayNight->getHour();

    // 1. Mediodía / Luz de día plena (09:00 - 16:30)
    if (hour >= 9.0f && hour <= 16.5f) {
        outSunlitColor = ColorRGB{255, 255, 255};
        outShadedColor = ColorRGB{205, 216, 232};
        outShadowColor = ColorRGB{35, 45, 65};
        outShadowAlpha = 0.34f;
    }
    // 2. Amanecer / Alborada (05:00 - 09:00)
    else if (hour >= 5.0f && hour < 9.0f) {
        float t = (hour - 5.0f) / 4.0f;
        // Mezcla de cálido dorado matutino a blanco diurno
        uint8_t r = static_cast<uint8_t>(255);
        uint8_t g = static_cast<uint8_t>(std::lerp(215.0f, 255.0f, t));
        uint8_t b = static_cast<uint8_t>(std::lerp(165.0f, 255.0f, t));
        outSunlitColor = ColorRGB{r, g, b};

        uint8_t sr = static_cast<uint8_t>(std::lerp(185.0f, 205.0f, t));
        uint8_t sg = static_cast<uint8_t>(std::lerp(148.0f, 216.0f, t));
        uint8_t sb = static_cast<uint8_t>(std::lerp(180.0f, 232.0f, t));
        outShadedColor = ColorRGB{sr, sg, sb};

        outShadowColor = ColorRGB{52, 44, 62};
        outShadowAlpha = std::lerp(0.24f, 0.34f, t);
    }
    // 3. Ocaso / Atardecer (16.5h - 20.0h)
    else if (hour > 16.5f && hour <= 20.0f) {
        float t = (hour - 16.5f) / 3.5f;
        // Mezcla de blanco diurno a ámbar y carmesí crepuscular
        uint8_t r = static_cast<uint8_t>(255);
        uint8_t g = static_cast<uint8_t>(std::lerp(255.0f, 175.0f, t));
        uint8_t b = static_cast<uint8_t>(std::lerp(255.0f, 115.0f, t));
        outSunlitColor = ColorRGB{r, g, b};

        uint8_t sr = static_cast<uint8_t>(std::lerp(205.0f, 142.0f, t));
        uint8_t sg = static_cast<uint8_t>(std::lerp(216.0f, 108.0f, t));
        uint8_t sb = static_cast<uint8_t>(std::lerp(232.0f, 155.0f, t));
        outShadedColor = ColorRGB{sr, sg, sb};

        outShadowColor = ColorRGB{58, 38, 56};
        outShadowAlpha = std::lerp(0.34f, 0.22f, t);
    }
    // 4. Noche y Madrugada (20:00 - 05:00)
    else {
        // Iluminación nocturna lunar: nubes índigo plateadas
        outSunlitColor = ColorRGB{115, 142, 185};
        outShadedColor = ColorRGB{38, 48, 75};
        outShadowColor = ColorRGB{14, 18, 30};
        outShadowAlpha = 0.15f;
    }

    // Efecto de Relámpago Divino (Lightning Flash)
    float flash = dayNight->getLightningFlashIntensity();
    if (flash > 0.01f) {
        outSunlitColor = ColorRGB{255, 255, 255};
        outShadedColor = ColorRGB{240, 248, 255};
        outShadowAlpha = std::max(0.0f, outShadowAlpha - flash * 0.30f);
    }
}

} // namespace ReligionSim
