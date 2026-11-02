#pragma once

#include "Types.hpp"
#include <vector>
#include <cmath>

namespace ReligionSim {

class WorldMap;
class IsometricCamera;
class TileSet;
struct TileRect;

// Estructura de articulación para extremidades en física de ragdoll
struct RagdollLimbJoint {
    float length{7.0f};       // Longitud del miembro en píxeles locales
    float angle{0.0f};        // Ángulo de inclinación actual relativo al torso
    float angularVel{0.0f};   // Velocidad angular de sacudida
    ColorRGB color{180, 140, 100}; // Color de la extremidad (ropa/piel)
};

// Sistema de físicas Ragdoll 2.5D (inercia, volteretas, rebote y reposo)
class Ragdoll {
public:
    Ragdoll();

    // EN: Activate ragdoll with initial linear and angular velocity (tornado, blast, shockwave, collapse)
    // ES: Activar ragdoll con impulso lineal y angular (lanzamiento por tornado, rayo o colapso)
    void launch(float impulseX, float impulseY, float impulseZ, float angularSpeedRadS = 12.0f);
    void deactivate();

    // EN: Update physics simulation per time step
    // ES: Actualización de físicas por paso de tiempo
    void update(float deltaTime, float& posX, float& posY, float& altitude,
                float& velocityX, float& velocityY, float& velocityZ,
                const WorldMap& mapRef, bool isDead);

    // EN: Render rotated body sprite, procedural articulated limbs and impact effects
    // ES: Renderizado del cuerpo rotado, extremidades articuladas y efectos de impacto
    void render(uint32_t* dstBuffer, int dstBufferWidth, int dstBufferHeight,
                const IsometricCamera& camera, const WorldMap& mapRef,
                const TileSet& tileSetRef, const TileRect& rect,
                float posX, float posY, float altitude,
                float renderScale, ColorRGB auraColor, float alpha = 1.0f,
                bool isProphet = false) const;

    bool isActive() const { return m_active; }
    bool isSettled() const { return m_settled; }
    bool isInWater() const { return m_inWater; }
    float getAngle() const { return m_angle; }
    int getBounceCount() const { return m_bounceCount; }

private:
    bool m_active{false};
    bool m_settled{false};
    bool m_inWater{false};

    float m_angle{0.0f};            // Ángulo de rotación del cuerpo principal (radianes)
    float m_angularVelocity{0.0f};  // Velocidad de giro / voltereta
    int m_bounceCount{0};           // Contador de rebotes en el suelo

    // Físicas de articulaciones (extremidades colgantes y sacudidas)
    RagdollLimbJoint m_armL;
    RagdollLimbJoint m_armR;
    RagdollLimbJoint m_legL;
    RagdollLimbJoint m_legR;

    float m_flailTimer{0.0f};       // Temporizador de aleteo en el aire
    float m_waterBobTimer{0.0f};    // Oscilación de flotación en agua
    float m_settleDuration{0.0f};   // Tiempo que lleva en reposo
    float m_impactFlashTimer{0.0f}; // Destello de impacto al rebotar
};

} // namespace ReligionSim
