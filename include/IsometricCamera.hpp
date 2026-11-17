#pragma once

#include "Types.hpp"

namespace ReligionSim {

class IsometricCamera {
public:
    IsometricCamera(int screenWidth = 1280, int screenHeight = 720);

    void setScreenSize(int width, int height);
    void update(float deltaTime);

    // Zoom controls
    void zoomBy(float factor, float focalScreenX, float focalScreenY);
    void setZoom(float zoom);
    float getZoom() const { return m_zoom; }

    // Pan controls
    void pan(float dx, float dy);
    void startDrag(float mouseX, float mouseY);
    void updateDrag(float mouseX, float mouseY);
    void endDrag();
    bool isDragging() const { return m_isDragging; }

    void centerOnCell(int gridX, int gridY, int mapWidth, int mapHeight);
    void reset(int mapWidth, int mapHeight);

    // EN: Instantly snap camera position and zoom to target values without lerp lag
    // ES: Ajustar instantáneamente la posición y zoom de la cámara a los valores objetivo sin retardo lerp
    void snapToTarget() {
        m_cameraX = m_targetCameraX;
        m_cameraY = m_targetCameraY;
        m_zoom = m_targetZoom;
    }

    void addScreenShake(float intensity, float duration);
    float getShakeIntensity() const { return m_shakeIntensity; }
    float getShakeOffsetX() const { return m_shakeOffsetX; }
    float getShakeOffsetY() const { return m_shakeOffsetY; }

    // Coordinate conversion
    Vec2 gridToScreen(float gx, float gy, float elevation = 0.0f) const;
    Vec2 gridTileCenterScreen(int gx, int gy, float elevation = 0.0f) const;
    void screenToGrid(float sx, float sy, int& outGx, int& outGy) const;

    float getCameraX() const { return m_cameraX; }
    float getCameraY() const { return m_cameraY; }
    int getScreenWidth() const { return m_screenWidth; }
    int getScreenHeight() const { return m_screenHeight; }

private:
    int m_screenWidth{1280};
    int m_screenHeight{720};

    float m_cameraX{0.0f};
    float m_cameraY{0.0f};
    float m_targetCameraX{0.0f};
    float m_targetCameraY{0.0f};

    float m_zoom{1.0f};
    float m_targetZoom{1.0f};
    float m_minZoom{0.10f};
    float m_maxZoom{3.5f};

    bool m_isDragging{false};
    float m_dragStartX{0.0f};
    float m_dragStartY{0.0f};
    float m_camStartX{0.0f};
    float m_camStartY{0.0f};

    // Sacudida sísmica de pantalla (Screen Shake por terremotos/catástrofes)
    float m_shakeIntensity{0.0f};
    float m_shakeDuration{0.0f};
    float m_shakeTimer{0.0f};
    float m_shakeSeed{0.0f};
    float m_shakeOffsetX{0.0f};
    float m_shakeOffsetY{0.0f};
};

} // namespace ReligionSim
