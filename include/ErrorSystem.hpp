#pragma once

#include <string>
#include <string_view>
#include <array>
#include <vector>
#include <cstdint>
#include <chrono>
#include <algorithm>
#include <iostream>

namespace ReligionSim {

// =============================================================================
// EN: Error and telemetry classification categories
// ES: Categorías de clasificación para errores y telemetría
// =============================================================================
enum class ErrorCategory : uint8_t {
    Memory           = 0, // EN: RAM usage, leak or heap overflow / ES: Consumo de RAM, fugas o desborde de heap
    Performance      = 1, // EN: Frame rate drops, spikes and lag / ES: Caídas de FPS, picos de tiempo y lag
    BufferSafety     = 2, // EN: Rasterizer or sprite array boundary violation / ES: Violación de límites en ráster o sprites
    EntityOverload   = 3, // EN: Population exceeding safe simulation limit / ES: Población superando límite seguro
    SimulationState  = 4  // EN: State corruption, NaNs, or infinite loops / ES: Corrupción de estado, NaNs o bucles infinitos
};

// =============================================================================
// EN: Severity level of logged incidents
// ES: Nivel de severidad de los incidentes registrados
// =============================================================================
enum class ErrorSeverity : uint8_t {
    Info     = 0, // EN: Routine informational telemetry / ES: Telemetría informativa ordinaria
    Warning  = 1, // EN: Non-critical degradation or threshold breach / ES: Degradación no crítica o umbral rebasado
    Error    = 2, // EN: Prevented fault or dropped frame burst / ES: Fallo prevenido o ráfaga de frames perdidos
    Critical = 3  // EN: Critical condition triggering emergency countermeasures / ES: Condición crítica que activa contramedidas
};

// =============================================================================
// EN: Bounded individual error log record with fixed-size message (no heap allocs)
// ES: Registro acotado de error individual con mensaje de tamaño fijo (sin allocs en heap)
// =============================================================================
struct ErrorRecord {
    ErrorCategory category{ErrorCategory::SimulationState};
    ErrorSeverity severity{ErrorSeverity::Info};
    uint64_t timestampMilliseconds{0};
    float memoryUsageMb{0.0f};
    float frameDurationMs{0.0f};
    char messageText[128]{0};
};

// =============================================================================
// EN: Real-time system health metrics snapshot
// ES: Instantánea en tiempo real de las métricas de salud del sistema
// =============================================================================
struct SystemHealthMetrics {
    float currentMemoryMb{0.0f};
    float peakMemoryMb{0.0f};
    float currentFrameTimeMs{0.0f};
    float averageFrameTimeMs{0.0f};
    float currentFps{60.0f};
    uint32_t activeEntityCount{0};
    uint32_t maxSafeEntityLimit{350};
    uint32_t droppedFrameCount{0};
    uint32_t totalErrorsLogged{0};
    uint32_t bufferViolationsPrevented{0};
    bool isUnderLagStress{false};
    bool isMemoryWarningActive{false};
    bool isPopulationCapped{false};
};

// =============================================================================
// EN: ErrorSystem - Centralized watchdog against memory overflows and lag spikes
// ES: ErrorSystem - Guardián centralizado contra desbordes de memoria y picos de lag
// =============================================================================
class ErrorSystem {
public:
    // EN: Configuration thresholds for memory and performance guardrails
    // ES: Umbrales de configuración para las barreras de memoria y rendimiento
    static constexpr size_t MAX_LOGGED_ERRORS = 128;
    static constexpr float WARNING_MEMORY_MB = 450.0f;
    static constexpr float CRITICAL_MEMORY_MB = 750.0f;
    static constexpr float TARGET_FRAME_TIME_MS = 16.666f;     // 60 FPS
    static constexpr float WARNING_FRAME_TIME_MS = 33.333f;    // < 30 FPS
    static constexpr float CRITICAL_FRAME_TIME_MS = 66.666f;   // < 15 FPS
    static constexpr float MAX_ALLOWED_DELTA_TIME_S = 0.050f;   // Anti-death spiral clamp
    static constexpr uint32_t DEFAULT_MAX_SAFE_ENTITIES = 350;

    // EN: Singleton accessor
    // ES: Acceso a la instancia única singleton
    static ErrorSystem& instance();

    ErrorSystem();
    ~ErrorSystem() = default;

    // Non-copyable, non-movable singleton semantics
    ErrorSystem(const ErrorSystem&) = delete;
    ErrorSystem& operator=(const ErrorSystem&) = delete;

    // =========================================================================
    // EN: Telemetry logging interface (safe for high-frequency calls, no heap alloc)
    // ES: Interfaz de registro de telemetría (segura para alta frecuencia, sin allocs)
    // =========================================================================
    void logError(ErrorCategory category, ErrorSeverity severity, const char* formattedMessage);
    void logError(ErrorCategory category, ErrorSeverity severity, const std::string& messageString) {
        logError(category, severity, messageString.c_str());
    }

    // =========================================================================
    // EN: Real-time frame timing and lag mitigation
    // ES: Medición de tiempos de fotograma y mitigación de lag en tiempo real
    // =========================================================================
    void reportFrameTime(float frameDurationMs);
    [[nodiscard]] float clampDeltaTime(float rawDeltaTimeSeconds) const noexcept;
    [[nodiscard]] bool isUnderLagStress() const noexcept { return m_isUnderLagStress; }

    // =========================================================================
    // EN: Memory usage auditing and leak/overflow detection
    // ES: Auditoría de consumo de memoria y detección de fugas/desbordes
    // =========================================================================
    float auditMemoryUsage();
    [[nodiscard]] float getCurrentMemoryMb() const noexcept { return m_currentMemoryMb; }
    [[nodiscard]] float getPeakMemoryMb() const noexcept { return m_peakMemoryMb; }
    [[nodiscard]] bool isMemoryWarningActive() const noexcept { return m_isMemoryWarningActive; }
    [[nodiscard]] bool shouldTriggerEmergencyPurge() const noexcept { return m_shouldTriggerEmergencyPurge; }
    void acknowledgeEmergencyPurge() noexcept { m_shouldTriggerEmergencyPurge = false; }

    // =========================================================================
    // EN: Population limits and demographic safety guardrails
    // ES: Límites de población y barreras de seguridad demográfica
    // =========================================================================
    void updateEntityCount(uint32_t currentCount);
    [[nodiscard]] bool isPopulationCapReached() const noexcept { return m_isPopulationCapReached; }
    [[nodiscard]] uint32_t getMaxSafeEntities() const noexcept { return m_maxSafeEntityLimit; }
    void setMaxSafeEntities(uint32_t newLimit) noexcept { m_maxSafeEntityLimit = newLimit; }

    // =========================================================================
    // EN: Rasterizer and buffer boundary safety guards
    // ES: Guardas de seguridad para ráster y límites de buffers
    // =========================================================================
    [[nodiscard]] bool validateBufferAccess(int pixelX, int pixelY, int bufferWidth, int bufferHeight,
                                           const char* contextName = "Rasterizer");
    [[nodiscard]] uint32_t getBufferViolationsPreventedCount() const noexcept {
        return m_bufferViolationsPrevented;
    }

    // =========================================================================
    // EN: Diagnostics and health query methods
    // ES: Métodos de diagnóstico y consulta de salud del sistema
    // =========================================================================
    [[nodiscard]] SystemHealthMetrics getHealthMetrics() const noexcept;
    [[nodiscard]] std::vector<ErrorRecord> getRecentErrors(size_t maximumCount = 10) const;
    void resetMetrics() noexcept;

private:
    // EN: Query OS process memory (resident set size in MB)
    // ES: Consultar la memoria del proceso al SO (resident set size en MB)
    static float queryProcessResidentMemoryMb();

    // EN: Circular ring buffer for zero-allocation error logging
    // ES: Búfer anular circular para registro de errores sin asignación dinámica
    std::array<ErrorRecord, MAX_LOGGED_ERRORS> m_errorHistory{};
    size_t m_historyHeadIndex{0};
    size_t m_totalLoggedErrorCount{0};

    // EN: Performance tracking state
    // ES: Estado de seguimiento de rendimiento
    float m_lastFrameDurationMs{16.0f};
    float m_smoothedFrameDurationMs{16.0f};
    float m_recentLagDurationAccumulatorMs{0.0f};
    uint32_t m_droppedFrameCount{0};
    bool m_isUnderLagStress{false};

    // EN: Memory tracking state
    // ES: Estado de seguimiento de memoria
    float m_currentMemoryMb{0.0f};
    float m_peakMemoryMb{0.0f};
    float m_memoryAuditTimerSeconds{0.0f};
    bool m_isMemoryWarningActive{false};
    bool m_shouldTriggerEmergencyPurge{false};

    // EN: Entity population bounds
    // ES: Límites de población de entidades
    uint32_t m_activeEntityCount{0};
    uint32_t m_maxSafeEntityLimit{DEFAULT_MAX_SAFE_ENTITIES};
    bool m_isPopulationCapReached{false};

    // EN: Buffer boundary safety metrics
    // ES: Métricas de seguridad de límites de buffer
    uint32_t m_bufferViolationsPrevented{0};
    uint32_t m_bufferViolationLogThrottle{0};

    // EN: High-resolution start timestamp for relative ms
    // ES: Marca temporal de inicio de alta resolución para ms relativos
    std::chrono::steady_clock::time_point m_startTimePoint;
};

// =============================================================================
// EN: BufferGuard helper namespace for inline bounds verification
// ES: Espacio de nombres auxiliar BufferGuard para verificación de límites en línea
// =============================================================================
namespace BufferGuard {

// EN: Fast inline check returning true if pixel coordinates reside safely inside the target buffer
// ES: Comprobación rápida en línea que retorna true si las coordenadas residen dentro del buffer
[[nodiscard]] inline constexpr bool isPixelWithinBounds(int pixelCoordinateX, int pixelCoordinateY,
                                                        int bufferWidth, int bufferHeight) noexcept {
    return (pixelCoordinateX >= 0 && pixelCoordinateX < bufferWidth &&
            pixelCoordinateY >= 0 && pixelCoordinateY < bufferHeight);
}

// EN: Safe pixel writing primitive clamping out-of-bounds coordinates
// ES: Primitiva segura de escritura de píxeles que descarta coordenadas fuera de rango
inline void writePixelSafe(uint32_t* destinationBuffer, int pixelCoordinateX, int pixelCoordinateY,
                           int bufferWidth, int bufferHeight, uint32_t pixelColorRgba) noexcept {
    if (isPixelWithinBounds(pixelCoordinateX, pixelCoordinateY, bufferWidth, bufferHeight)) {
        destinationBuffer[pixelCoordinateY * bufferWidth + pixelCoordinateX] = pixelColorRgba;
    }
}

} // namespace BufferGuard

} // namespace ReligionSim
