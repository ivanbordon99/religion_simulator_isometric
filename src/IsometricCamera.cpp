#include "IsometricCamera.hpp"
#include <algorithm>
#include <cmath>

namespace ReligionSim {

IsometricCamera::IsometricCamera(int screenWidth, int screenHeight)
    : m_screenWidth(screenWidth), m_screenHeight(screenHeight) {
    m_cameraX = static_cast<float>(screenWidth) / 2.0f;
    m_cameraY = static_cast<float>(screenHeight) / 4.0f;
    m_targetCameraX = m_cameraX;
    m_targetCameraY = m_cameraY;
}

void IsometricCamera::setScreenSize(int width, int height) {
    m_screenWidth = width;
    m_screenHeight = height;
}

void IsometricCamera::update(float deltaTime) {
    // Smooth interpolation for camera pan & zoom
    float lerpSpeed = 12.0f * deltaTime;
    lerpSpeed = std::clamp(lerpSpeed, 0.0f, 1.0f);

    m_cameraX += (m_targetCameraX - m_cameraX) * lerpSpeed;
    m_cameraY += (m_targetCameraY - m_cameraY) * lerpSpeed;
    m_zoom += (m_targetZoom - m_zoom) * lerpSpeed;

    // Actualización de sacudida sísmica (Screen Shake)
    if (m_shakeTimer > 0.0f) {
        m_shakeTimer -= deltaTime;
        float progress = m_shakeTimer / std::max(0.01f, m_shakeDuration);
        float currentMag = m_shakeIntensity * progress;
        m_shakeSeed += deltaTime * 48.0f;
        m_shakeOffsetX = std::sin(m_shakeSeed * 1.8f) * currentMag;
        m_shakeOffsetY = std::cos(m_shakeSeed * 2.4f) * (currentMag * 0.7f);
    } else {
        m_shakeOffsetX = 0.0f;
        m_shakeOffsetY = 0.0f;
        m_shakeIntensity = 0.0f;
        m_shakeTimer = 0.0f;
    }
}

void IsometricCamera::addScreenShake(float intensity, float duration) {
    m_shakeIntensity = std::max(m_shakeIntensity, intensity);
    m_shakeDuration = std::max(m_shakeDuration, duration);
    m_shakeTimer = std::max(m_shakeTimer, duration);
}

void IsometricCamera::zoomBy(float factor, float focalScreenX, float focalScreenY) {
    float newZoom = std::clamp(m_targetZoom * factor, m_minZoom, m_maxZoom);
    if (std::abs(newZoom - m_targetZoom) < 0.001f) return;

    // Anchor zoom around focal screen point
    float ratio = newZoom / m_targetZoom;
    m_targetCameraX = focalScreenX - (focalScreenX - m_targetCameraX) * ratio;
    m_targetCameraY = focalScreenY - (focalScreenY - m_targetCameraY) * ratio;
    m_targetZoom = newZoom;
}

void IsometricCamera::setZoom(float zoom) {
    m_targetZoom = std::clamp(zoom, m_minZoom, m_maxZoom);
}

void IsometricCamera::pan(float dx, float dy) {
    m_targetCameraX += dx;
    m_targetCameraY += dy;
    m_cameraX += dx;
    m_cameraY += dy;
}

void IsometricCamera::startDrag(float mouseX, float mouseY) {
    m_isDragging = true;
    m_dragStartX = mouseX;
    m_dragStartY = mouseY;
    m_camStartX = m_targetCameraX;
    m_camStartY = m_targetCameraY;
}

void IsometricCamera::updateDrag(float mouseX, float mouseY) {
    if (!m_isDragging) return;
    float dx = mouseX - m_dragStartX;
    float dy = mouseY - m_dragStartY;
    m_targetCameraX = m_camStartX + dx;
    m_targetCameraY = m_camStartY + dy;
    m_cameraX = m_targetCameraX;
    m_cameraY = m_targetCameraY;
}

void IsometricCamera::endDrag() {
    m_isDragging = false;
}

void IsometricCamera::centerOnCell(int gridX, int gridY, int mapWidth, int mapHeight) {
    (void)mapWidth;
    (void)mapHeight;
    float halfW = BASE_TILE_WIDTH * 0.5f;
    float halfH = BASE_TILE_HEIGHT * 0.5f;

    float isoX = (static_cast<float>(gridX) - static_cast<float>(gridY)) * halfW;
    float isoY = (static_cast<float>(gridX) + static_cast<float>(gridY)) * halfH;

    m_targetCameraX = static_cast<float>(m_screenWidth) * 0.5f - isoX * m_zoom;
    m_targetCameraY = static_cast<float>(m_screenHeight) * 0.5f - isoY * m_zoom;
}

void IsometricCamera::reset(int mapWidth, int mapHeight) {
    centerOnCell(mapWidth / 2, mapHeight / 2, mapWidth, mapHeight);
    m_targetZoom = 0.85f;
    m_zoom = 0.85f;
}

Vec2 IsometricCamera::gridToScreen(float gx, float gy, float elevation) const {
    float halfW = BASE_TILE_WIDTH * 0.5f;
    float halfH = BASE_TILE_HEIGHT * 0.5f;

    float isoX = (gx - gy) * halfW;
    float isoY = (gx + gy) * halfH - elevation * ELEVATION_STEP;

    return {
        isoX * m_zoom + m_cameraX + m_shakeOffsetX,
        isoY * m_zoom + m_cameraY + m_shakeOffsetY
    };
}

Vec2 IsometricCamera::gridTileCenterScreen(int gx, int gy, float elevation) const {
    return gridToScreen(static_cast<float>(gx) + 0.5f, static_cast<float>(gy) + 0.5f, elevation);
}

void IsometricCamera::screenToGrid(float sx, float sy, int& outGx, int& outGy) const {
    float isoX = (sx - m_cameraX - m_shakeOffsetX) / m_zoom;
    float isoY = (sy - m_cameraY - m_shakeOffsetY) / m_zoom;

    float halfW = BASE_TILE_WIDTH * 0.5f;
    float halfH = BASE_TILE_HEIGHT * 0.5f;

    float u = isoX / halfW;
    float v = isoY / halfH;

    float gx = (u + v) * 0.5f;
    float gy = (v - u) * 0.5f;

    outGx = static_cast<int>(std::floor(gx));
    outGy = static_cast<int>(std::floor(gy));
}

} // namespace ReligionSim
