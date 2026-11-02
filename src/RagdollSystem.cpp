#include "RagdollSystem.hpp"
#include "WorldMap.hpp"
#include "IsometricCamera.hpp"
#include "TileSet.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>

namespace ReligionSim {

Ragdoll::Ragdoll() {
    m_armL.length = 6.0f;
    m_armL.color = ColorRGB{190, 150, 110};
    m_armR.length = 6.0f;
    m_armR.color = ColorRGB{190, 150, 110};

    m_legL.length = 7.0f;
    m_legL.color = ColorRGB{90, 65, 45};
    m_legR.length = 7.0f;
    m_legR.color = ColorRGB{90, 65, 45};
}

// EN: Launch ragdoll physics simulation with initial velocities
// ES: Activar física de ragdoll con velocidades iniciales
void Ragdoll::launch(float impulseX, float impulseY, float impulseZ, float angularSpeedRadS) {
    (void)impulseX;
    (void)impulseY;
    (void)impulseZ;
    m_active = true;
    m_settled = false;
    m_inWater = false;
    m_angularVelocity = angularSpeedRadS;
    m_bounceCount = 0;
    m_flailTimer = 0.0f;
    m_waterBobTimer = 0.0f;
    m_settleDuration = 0.0f;
    m_impactFlashTimer = 0.0f;
}

void Ragdoll::deactivate() {
    m_active = false;
    m_settled = false;
    m_inWater = false;
    m_angle = 0.0f;
    m_angularVelocity = 0.0f;
}

// EN: Update physics simulation for ragdoll entity (trajectory, drag, ground bounce, floating)
// ES: Actualizar simulación física de la entidad ragdoll (trayectoria, arrastre, rebote, flotación)
void Ragdoll::update(float deltaTime, float& posX, float& posY, float& altitude,
                     float& velocityX, float& velocityY, float& velocityZ,
                     const WorldMap& mapRef, bool isDead) {
    if (!m_active || deltaTime <= 0.0f) return;

    if (m_impactFlashTimer > 0.0f) {
        m_impactFlashTimer = std::max(0.0f, m_impactFlashTimer - deltaTime);
    }

    int cellX = std::clamp(static_cast<int>(std::floor(posX)), 0, mapRef.getWidth() - 1);
    int cellY = std::clamp(static_cast<int>(std::floor(posY)), 0, mapRef.getHeight() - 1);
    bool cellIsWater = mapRef.isValid(cellX, cellY) && mapRef.getCell(cellX, cellY).isWater();

    // EN: Behavior for deceased units (corpses settling flat on ground or water)
    // ES: Comportamiento para unidades fallecidas (cadáveres asentados en tierra o agua)
    if (isDead) {
        m_inWater = cellIsWater;

        // EN: Fall under gravity if corpse was airborne
        // ES: Caída por gravedad si el cadáver estaba en el aire
        if (altitude > 0.0f) {
            altitude += velocityZ * deltaTime;
            posX += velocityX * deltaTime;
            posY += velocityY * deltaTime;
            velocityZ -= 90.0f * deltaTime;
            velocityX *= std::max(0.0f, 1.0f - deltaTime * 4.0f);
            velocityY *= std::max(0.0f, 1.0f - deltaTime * 4.0f);

            // EN: Orient quickly towards horizontal resting posture
            // ES: Orientación rápida hacia posición acostada horizontal
            float targetAngle = (m_angle >= 0.0f) ? 1.57f : -1.57f;
            if (std::abs(m_angle) < 0.01f) targetAngle = 1.57f;
            m_angle += (targetAngle - m_angle) * std::min(1.0f, deltaTime * 8.0f);
        }

        // EN: Ground or water surface impact: immediate collapse and flat repose
        // ES: Contacto con suelo o superficie de agua: reposo plano sin rebotes
        if (altitude <= 0.0f) {
            altitude = 0.0f;
            velocityX = 0.0f;
            velocityY = 0.0f;
            velocityZ = 0.0f;
            m_angularVelocity = 0.0f;
            m_settled = true;
            m_settleDuration += deltaTime;

            // EN: Resting horizontally on ground (~1.57 rad)
            // ES: Reposo horizontal en el suelo (~1.57 rad)
            float targetAngle = (m_angle >= 0.0f) ? 1.57f : -1.57f;
            if (std::abs(m_angle) < 0.01f) targetAngle = 1.57f;
            m_angle += (targetAngle - m_angle) * std::min(1.0f, deltaTime * 14.0f);
            if (std::abs(targetAngle - m_angle) < 0.04f) {
                m_angle = targetAngle;
            }
        }

        // EN: Clamp inside map boundaries
        // ES: Limitar dentro de los límites del mapa
        posX = std::clamp(posX, 0.5f, static_cast<float>(mapRef.getWidth() - 1) + 0.5f);
        posY = std::clamp(posY, 0.5f, static_cast<float>(mapRef.getHeight() - 1) + 0.5f);
        return;
    }

    // EN: Behavior for living units launched by cataclysms or impact
    // ES: Comportamiento para unidades vivas lanzadas por cataclismos
    if (!m_settled || altitude > 0.0f) {
        altitude += velocityZ * deltaTime;
        posX += velocityX * deltaTime;
        posY += velocityY * deltaTime;

        // EN: Clamp inside world boundaries
        // ES: Limitar dentro de las fronteras del mapa
        posX = std::clamp(posX, 0.5f, static_cast<float>(mapRef.getWidth() - 1) + 0.5f);
        posY = std::clamp(posY, 0.5f, static_cast<float>(mapRef.getHeight() - 1) + 0.5f);

        // EN: Gravity and aerodynamic drag
        // ES: Gravedad y resistencia aerodinámica
        if (!m_inWater) {
            velocityZ -= 65.0f * deltaTime;
            velocityX *= std::max(0.0f, 1.0f - deltaTime * 0.75f);
            velocityY *= std::max(0.0f, 1.0f - deltaTime * 0.75f);
        } else {
            velocityZ -= 18.0f * deltaTime; // EN: Buoyancy in fluid / ES: Flotabilidad en fluido
            velocityX *= std::max(0.0f, 1.0f - deltaTime * 2.8f);
            velocityY *= std::max(0.0f, 1.0f - deltaTime * 2.8f);
        }

        // EN: Body tumble and rotation
        // ES: Rotación y voltereta del cuerpo
        m_angle += m_angularVelocity * deltaTime;
        m_angularVelocity *= std::max(0.0f, 1.0f - deltaTime * 0.85f);

        // EN: Procedural inertial flailing of limbs
        // ES: Sacudida inercial de extremidades articuladas
        m_flailTimer += deltaTime * (std::abs(m_angularVelocity) * 1.5f + std::abs(velocityZ) * 0.12f + 4.0f);
        m_armL.angle = std::sin(m_flailTimer * 5.5f) * 0.85f + (velocityX * 0.18f);
        m_armR.angle = -std::sin(m_flailTimer * 5.5f + 0.6f) * 0.85f - (velocityX * 0.18f);
        m_legL.angle = std::cos(m_flailTimer * 4.8f) * 0.70f + (velocityY * 0.15f);
        m_legR.angle = -std::cos(m_flailTimer * 4.8f + 0.8f) * 0.70f - (velocityY * 0.15f);
    }

    // EN: Surface collision (Ground or Water)
    // ES: Colisión con la superficie (Tierra o Agua)
    if (altitude <= 0.0f) {
        if (cellIsWater) {
            m_inWater = true;
            m_waterBobTimer += deltaTime;

            // EN: Gentle hydrodynamic bobbing
            // ES: Flotación hidrodinámica suave
            altitude = std::sin(m_waterBobTimer * 2.8f) * 2.2f - 3.5f;
            velocityX *= std::max(0.0f, 1.0f - deltaTime * 3.5f);
            velocityY *= std::max(0.0f, 1.0f - deltaTime * 3.5f);
            velocityZ = 0.0f;
            m_angularVelocity *= std::max(0.0f, 1.0f - deltaTime * 3.0f);

            if (std::abs(velocityX) < 0.08f && std::abs(velocityY) < 0.08f) {
                m_settled = true;
                float targetAngle = (m_angle >= 0.0f) ? 1.57f : -1.57f;
                m_angle += (targetAngle - m_angle) * std::min(1.0f, deltaTime * 5.0f);
            }
        } else {
            m_inWater = false;

            // EN: Elastic restitution bounce against rocky ground
            // ES: Rebote elástico contra suelo rocoso o tierra
            if (std::abs(velocityZ) > 13.0f) {
                altitude = 0.0f;
                velocityZ = -velocityZ * 0.38f;
                velocityX *= 0.60f;
                velocityY *= 0.60f;
                m_angularVelocity = -m_angularVelocity * 0.45f + (velocityX * 4.0f);
                m_bounceCount++;
                m_impactFlashTimer = 0.15f;
            } else {
                // EN: Final friction damping on ground
                // ES: Freno final por fricción con el suelo
                altitude = 0.0f;
                velocityZ = 0.0f;
                velocityX *= std::max(0.0f, 1.0f - deltaTime * 6.5f);
                velocityY *= std::max(0.0f, 1.0f - deltaTime * 6.5f);

                if (std::abs(velocityX) < 0.12f && std::abs(velocityY) < 0.12f) {
                    m_settled = true;
                    velocityX = 0.0f;
                    velocityY = 0.0f;
                    m_angularVelocity = 0.0f;

                    float targetAngle = (m_angle >= 0.0f) ? 1.57f : -1.57f;
                    m_angle += (targetAngle - m_angle) * std::min(1.0f, deltaTime * 8.0f);
                    m_settleDuration += deltaTime;
                }
            }
        }
    }
}

void Ragdoll::render(uint32_t* dstBuffer, int dstBufferWidth, int dstBufferHeight,
                     const IsometricCamera& camera, const WorldMap& mapRef,
                     const TileSet& tileSetRef, const TileRect& rect,
                     float posX, float posY, float altitude,
                     float scale, ColorRGB auraColor, float alpha,
                     bool isProphet) const {
    if (!m_active || alpha <= 0.01f) return;

    const int dstW = dstBufferWidth;
    const int dstH = dstBufferHeight;

    int cellX = std::clamp(static_cast<int>(std::floor(posX)), 0, mapRef.getWidth() - 1);
    int cellY = std::clamp(static_cast<int>(std::floor(posY)), 0, mapRef.getHeight() - 1);
    float elev = 1.0f;
    if (mapRef.isValid(cellX, cellY)) {
        elev = mapRef.getCell(cellX, cellY).elevation;
    }

    Vec2 pieSuelo = camera.gridToScreen(posX, posY, elev);
    Vec2 centroCuerpo = pieSuelo;

    float destW = rect.width * scale;
    (void)destW;
    float destH = rect.height * scale;
    float halfH = destH * 0.5f;

    // Altitud sobre el terreno y acomodación al suelo según la inclinación
    // Cuando el cuerpo está rotado horizontalmente (acostado, m_angle ~ 1.57 rad),
    // su centro visual descansa a ras del suelo (6.0f * scale sobre pieSuelo.y),
    // evitando que el cadáver quede suspendido flotando en el aire.
    float tilt = std::clamp(std::abs(std::sin(m_angle)), 0.0f, 1.0f);
    float restingOffset = halfH * (1.0f - tilt) + (6.0f * scale) * tilt;
    centroCuerpo.y -= (altitude * camera.getZoom() + restingOffset);

    // 1. Sombra proyectada en el suelo mientras vuela
    if (altitude > 1.0f) {
        int shadowRx = std::max(2, static_cast<int>(12.0f * scale * std::max(0.20f, 1.0f - (altitude / 90.0f))));
        int shadowRy = std::max(1, static_cast<int>(6.0f * scale * std::max(0.20f, 1.0f - (altitude / 90.0f))));
        int scx = static_cast<int>(pieSuelo.x);
        int scy = static_cast<int>(pieSuelo.y);
        for (int dy = -shadowRy; dy <= shadowRy; ++dy) {
            int py = scy + dy;
            int fy = dstH - 1 - py;
            if (fy < 0 || fy >= dstH) continue;
            for (int dx = -shadowRx; dx <= shadowRx; ++dx) {
                int px = scx + dx;
                if (px < 0 || px >= dstW) continue;
                float dNorm = (static_cast<float>(dx * dx) / (shadowRx * shadowRx)) +
                              (static_cast<float>(dy * dy) / (shadowRy * shadowRy));
                if (dNorm <= 1.0f) {
                    uint32_t bg = dstBuffer[fy * dstW + px];
                    uint8_t br = static_cast<uint8_t>((bg & 0xFF) * 0.65f);
                    uint8_t bgc = static_cast<uint8_t>(((bg >> 8) & 0xFF) * 0.65f);
                    uint8_t bb = static_cast<uint8_t>(((bg >> 16) & 0xFF) * 0.65f);
                    dstBuffer[fy * dstW + px] = (0xFFu << 24) | (bb << 16) | (bgc << 8) | br;
                }
            }
        }
    } else if (m_settled && !m_inWater) {
        // Sombra suave de contacto para el cuerpo acostado en el suelo
        int shadowRx = std::max(3, static_cast<int>(16.0f * scale));
        int shadowRy = std::max(2, static_cast<int>(5.0f * scale));
        int scx = static_cast<int>(pieSuelo.x);
        int scy = static_cast<int>(pieSuelo.y - 2.0f * scale);
        for (int dy = -shadowRy; dy <= shadowRy; ++dy) {
            int py = scy + dy;
            int fy = dstH - 1 - py;
            if (fy < 0 || fy >= dstH) continue;
            for (int dx = -shadowRx; dx <= shadowRx; ++dx) {
                int px = scx + dx;
                if (px < 0 || px >= dstW) continue;
                float dNorm = (static_cast<float>(dx * dx) / (shadowRx * shadowRx)) +
                              (static_cast<float>(dy * dy) / (shadowRy * shadowRy));
                if (dNorm <= 1.0f) {
                    uint32_t bg = dstBuffer[fy * dstW + px];
                    float shadowFactor = 1.0f - (0.35f * alpha);
                    uint8_t br = static_cast<uint8_t>((bg & 0xFF) * shadowFactor);
                    uint8_t bgc = static_cast<uint8_t>(((bg >> 8) & 0xFF) * shadowFactor);
                    uint8_t bb = static_cast<uint8_t>(((bg >> 16) & 0xFF) * shadowFactor);
                    dstBuffer[fy * dstW + px] = (0xFFu << 24) | (bb << 16) | (bgc << 8) | br;
                }
            }
        }
    }

    // 2. Ondas y espuma si está flotando o cayendo en agua
    if (m_inWater) {
        int wx = static_cast<int>(pieSuelo.x);
        int wy = static_cast<int>(pieSuelo.y);
        int waveR = static_cast<int>(12.0f * scale);
        ColorRGB waveCol{80, 180, 245};
        float waveAlpha = 0.55f * alpha;
        for (int dy = -waveR / 2; dy <= waveR / 2; ++dy) {
            int py = wy + dy;
            int fy = dstH - 1 - py;
            if (fy < 0 || fy >= dstH) continue;
            for (int dx = -waveR; dx <= waveR; ++dx) {
                int px = wx + dx;
                if (px < 0 || px >= dstW) continue;
                float dNorm = (static_cast<float>(dx * dx) / (waveR * waveR)) +
                              (static_cast<float>(dy * dy * 4) / (waveR * waveR));
                if (dNorm >= 0.70f && dNorm <= 1.0f) {
                    uint32_t bg = dstBuffer[fy * dstW + px];
                    uint8_t br = bg & 0xFF;
                    uint8_t bgc = (bg >> 8) & 0xFF;
                    uint8_t bb = (bg >> 16) & 0xFF;
                    uint8_t outR = static_cast<uint8_t>(br * (1.0f - waveAlpha) + waveCol.r * waveAlpha);
                    uint8_t outG = static_cast<uint8_t>(bgc * (1.0f - waveAlpha) + waveCol.g * waveAlpha);
                    uint8_t outB = static_cast<uint8_t>(bb * (1.0f - waveAlpha) + waveCol.b * waveAlpha);
                    dstBuffer[fy * dstW + px] = (0xFFu << 24) | (outB << 16) | (outG << 8) | outR;
                }
            }
        }
    }

    // 3. Renderizar extremidades articuladas flácidas (brazos y piernas flailing)
    if (!m_settled) {
        auto dibujarMiembro = [&](float offsetTorsoX, float offsetTorsoY, float anguloLimb, float largo, ColorRGB color) {
            float cosT = std::cos(m_angle);
            float sinT = std::sin(m_angle);

            // Posición de la articulación (hombro/cadera) en pantalla
            float jx = centroCuerpo.x + (offsetTorsoX * cosT - offsetTorsoY * sinT) * scale;
            float jy = centroCuerpo.y + (offsetTorsoX * sinT + offsetTorsoY * cosT) * scale;

            // Extremo de la extremidad con su propio ángulo relativo
            float totalAng = m_angle + anguloLimb;
            float ex = jx + std::sin(totalAng) * largo * scale;
            float ey = jy + std::cos(totalAng) * largo * scale;

            int pasos = static_cast<int>(largo * scale * 1.5f);
            if (pasos < 2) pasos = 2;
            for (int p = 0; p <= pasos; ++p) {
                float t = static_cast<float>(p) / pasos;
                int px = static_cast<int>(jx + (ex - jx) * t);
                int py = static_cast<int>(jy + (ey - jy) * t);
                int fy = dstH - 1 - py;
                if (px >= 0 && px < dstW && fy >= 0 && fy < dstH) {
                    dstBuffer[fy * dstW + px] = (0xFFu << 24) | (color.b << 16) | (color.g << 8) | color.r;
                }
            }
        };

        // Brazos articulados
        dibujarMiembro(-6.0f, -4.0f, m_armL.angle, m_armL.length, m_armL.color);
        dibujarMiembro(6.0f, -4.0f, m_armR.angle, m_armR.length, m_armR.color);

        // Piernas articuladas
        dibujarMiembro(-4.0f, 6.0f, m_legL.angle, m_legL.length, m_legL.color);
        dibujarMiembro(4.0f, 6.0f, m_legR.angle, m_legR.length, m_legR.color);
    }

    // 4. Dibujar cuerpo principal con rotación continua por TileSet
    tileSetRef.blitTileRotated(dstBuffer, dstW, dstH, rect,
                               centroCuerpo.x, centroCuerpo.y,
                               m_angle, scale, auraColor, alpha, m_inWater);

    // 5. Destello al rebotar contra el suelo
    if (m_impactFlashTimer > 0.0f) {
        int fx = static_cast<int>(centroCuerpo.x);
        int fy = static_cast<int>(centroCuerpo.y);
        int puffR = static_cast<int>(8.0f * scale);
        ColorRGB dustCol{210, 180, 140};
        for (int dy = -puffR; dy <= puffR; ++dy) {
            int py = fy + dy;
            int fry = dstH - 1 - py;
            if (fry < 0 || fry >= dstH) continue;
            for (int dx = -puffR; dx <= puffR; ++dx) {
                int px = fx + dx;
                if (px < 0 || px >= dstW) continue;
                if (dx * dx + dy * dy <= puffR * puffR) {
                    uint32_t bg = dstBuffer[fry * dstW + px];
                    uint8_t br = bg & 0xFF;
                    uint8_t bgc = (bg >> 8) & 0xFF;
                    uint8_t bb = (bg >> 16) & 0xFF;
                    uint8_t outR = static_cast<uint8_t>(br * 0.5f + dustCol.r * 0.5f);
                    uint8_t outG = static_cast<uint8_t>(bgc * 0.5f + dustCol.g * 0.5f);
                    uint8_t outB = static_cast<uint8_t>(bb * 0.5f + dustCol.b * 0.5f);
                    dstBuffer[fry * dstW + px] = (0xFFu << 24) | (outB << 16) | (outG << 8) | outR;
                }
            }
        }
    }

    (void)isProphet;
}

} // namespace ReligionSim
