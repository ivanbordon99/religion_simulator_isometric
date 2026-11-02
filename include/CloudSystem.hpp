#pragma once

#include "Types.hpp"
#include <vector>
#include <cmath>
#include <algorithm>

namespace ReligionSim {

class DayNightCycle;

struct CloudLobe {
    float offsetX{0.0f};      // Desplazamiento X en celdas respecto al núcleo de la nube
    float offsetY{0.0f};      // Desplazamiento Y en celdas respecto al núcleo de la nube
    float radiusCells{2.4f};  // Radio del lóbulo en celdas isométricas
    float density{0.82f};     // Densidad y opacidad interna del vapor
    float aspectX{1.2f};      // Elongación en eje X por arrastre de viento
    float aspectY{0.85f};     // Compresión vertical isométrica
};

enum class CloudType : uint8_t {
    Cumulus,    // Nube algodonosa densa en altitud media-baja (estilo cúmulo)
    Stratus,    // Capa de nubes estratificadas horizontales
    Cirrus      // Velo alto y filamentoso de cristales de hielo
};

struct CloudCluster {
    int id{0};
    CloudType type{CloudType::Cumulus};
    float worldX{0.0f};       // Coordenada isométrica X (en celdas del mapa)
    float worldY{0.0f};       // Coordenada isométrica Y (en celdas del mapa)
    float altitude{8.2f};     // Altura determinada sobre el terreno (unidades de elevación, e.g. 7.5 - 11.5)
    float speedX{0.18f};      // Velocidad propia de deriva atmosférica en X
    float speedY{0.06f};      // Velocidad propia de deriva atmosférica en Y
    float scale{1.0f};        // Escala volumétrica global del cúmulo
    float baseAlpha{0.80f};   // Opacidad máxima base de la nube
    float pulseTimer{0.0f};   // Temporizador de respiración morfológica
    std::vector<CloudLobe> lobes;
};

class CloudSystem {
public:
    CloudSystem(int mapWidth = 64, int mapHeight = 64);

    void reset(int mapWidth = 64, int mapHeight = 64);
    void update(float deltaTime);

    // Parámetros de viento atmosférico
    void setWind(float vx, float vy) { m_windX = vx; m_windY = vy; }
    float getWindX() const { return m_windX; }
    float getWindY() const { return m_windY; }

    // Altura determinada de la capa de nubes (unidades de elevación sobre el suelo)
    float getBaseAltitude() const { return m_baseAltitude; }
    void setBaseAltitude(float alt) { m_baseAltitude = alt; }

    // Dimensiones del mapa
    int getMapWidth() const { return m_mapWidth; }
    int getMapHeight() const { return m_mapHeight; }

    // Acceso a los cúmulos de nubes
    const std::vector<CloudCluster>& getClouds() const { return m_clouds; }
    std::vector<CloudCluster>& getCloudsMutable() { return m_clouds; }
    size_t getCloudCount() const { return m_clouds.size(); }

    // Cálculo del factor de visibilidad por Zoom:
    // - Zoom Out (<= 0.85): 1.0 (100% visible desde las alturas celestiales)
    // - Transición suave (0.85 < zoom < 1.15): disipación progresiva al descender
    // - Zoom In (>= 1.15): 0.0 (completamente invisible al mirar de cerca el mapa/aldeanos)
    static float calculateZoomVisibility(float zoom);

    // Paleta de iluminación según el Ciclo Día/Noche
    void getLightingColors(const DayNightCycle* dayNight,
                           ColorRGB& outSunlitColor,
                           ColorRGB& outShadedColor,
                           ColorRGB& outShadowColor,
                           float& outShadowAlpha) const;

private:
    int m_mapWidth{64};
    int m_mapHeight{64};
    float m_windX{0.22f};
    float m_windY{0.08f};
    float m_baseAltitude{8.2f}; // Altura estándar: 8.2 pasos de elevación (~164 px)
    float m_globalTime{0.0f};
    std::vector<CloudCluster> m_clouds;

    void initializeClouds();
};

} // namespace ReligionSim
