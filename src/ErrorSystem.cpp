#include "ErrorSystem.hpp"
#include <cstdio>
#include <cstring>
#include <cmath>

#if defined(__linux__) || defined(__unix__) || defined(__APPLE__)
#include <unistd.h>
#include <sys/resource.h>
#endif

namespace ReligionSim {

// =============================================================================
// EN: Global singleton instance accessor
// ES: Acceso a la instancia global singleton
// =============================================================================
ErrorSystem& ErrorSystem::instance() {
    static ErrorSystem singletonInstance;
    return singletonInstance;
}

// =============================================================================
// EN: Constructor initializing timing benchmarks and base metrics
// ES: Constructor que inicializa marcas temporales y métricas base
// =============================================================================
ErrorSystem::ErrorSystem()
    : m_startTimePoint(std::chrono::steady_clock::now()) {
    auditMemoryUsage();
}

// =============================================================================
// EN: Log an error or telemetry incident into the bounded circular buffer
// ES: Registrar un error o incidencia de telemetría en el búfer circular acotado
// =============================================================================
void ErrorSystem::logError(ErrorCategory category, ErrorSeverity severity, const char* formattedMessage) {
    if (formattedMessage == nullptr) {
        return;
    }

    // EN: Compute relative elapsed milliseconds since initialization
    // ES: Calcular los milisegundos transcurridos desde la inicialización
    const auto currentTimePoint = std::chrono::steady_clock::now();
    const auto elapsedDuration = std::chrono::duration_cast<std::chrono::milliseconds>(currentTimePoint - m_startTimePoint);
    const uint64_t timestampMs = static_cast<uint64_t>(std::max<int64_t>(0, elapsedDuration.count()));

    // EN: Populate the circular history slot without dynamic heap allocations
    // ES: Poblar la ranura del historial circular sin asignaciones dinámicas en heap
    ErrorRecord& targetRecord = m_errorHistory[m_historyHeadIndex];
    targetRecord.category = category;
    targetRecord.severity = severity;
    targetRecord.timestampMilliseconds = timestampMs;
    targetRecord.memoryUsageMb = m_currentMemoryMb;
    targetRecord.frameDurationMs = m_lastFrameDurationMs;

    std::snprintf(targetRecord.messageText, sizeof(targetRecord.messageText), "%s", formattedMessage);

    // EN: Advance circular buffer pointer and total count
    // ES: Avanzar el puntero del búfer circular y el conteo total
    m_historyHeadIndex = (m_historyHeadIndex + 1) % MAX_LOGGED_ERRORS;
    m_totalLoggedErrorCount++;

    // EN: Emit console warnings and errors for immediate developer feedback
    // ES: Emitir advertencias y errores en consola para retroalimentación inmediata
    if (severity == ErrorSeverity::Warning) {
        std::cout << "[ErrorSystem:WARNING] (" << m_lastFrameDurationMs << "ms, "
                  << m_currentMemoryMb << "MB RAM) " << formattedMessage << std::endl;
    } else if (severity == ErrorSeverity::Error || severity == ErrorSeverity::Critical) {
        std::cerr << "[ErrorSystem:ALERT] (" << m_lastFrameDurationMs << "ms, "
                  << m_currentMemoryMb << "MB RAM) " << formattedMessage << std::endl;
    }
}

// =============================================================================
// EN: Report and analyze frame duration to detect lag and prevent stutter
// ES: Informar y analizar la duración del fotograma para detectar lag y tirones
// =============================================================================
void ErrorSystem::reportFrameTime(float frameDurationMs) {
    m_lastFrameDurationMs = frameDurationMs;

    // EN: Exponential moving average filter for smooth FPS estimation
    // ES: Filtro de media móvil exponencial para estimación suave de FPS
    constexpr float smoothingWeight = 0.08f;
    m_smoothedFrameDurationMs = (m_smoothedFrameDurationMs * (1.0f - smoothingWeight)) +
                                (frameDurationMs * smoothingWeight);

    // EN: Detect lag spike exceeding acceptable 30 FPS boundary (33.3ms)
    // ES: Detectar pico de lag que sobrepasa el límite aceptable de 30 FPS (33.3ms)
    if (frameDurationMs > WARNING_FRAME_TIME_MS) {
        m_droppedFrameCount++;
        m_recentLagDurationAccumulatorMs += frameDurationMs;

        // EN: Trigger lag stress mode if prolonged or heavy frame drops occur
        // ES: Activar modo de estrés por lag si ocurren caídas pesadas o prolongadas
        if (m_recentLagDurationAccumulatorMs > 90.0f || frameDurationMs > CRITICAL_FRAME_TIME_MS) {
            if (!m_isUnderLagStress) {
                m_isUnderLagStress = true;
                logError(ErrorCategory::Performance, ErrorSeverity::Warning,
                         "Lag stress activated: Frame time exceeded threshold. Throttling non-essential tasks.");
            }
        }
    } else {
        // EN: Cool down lag accumulator when performance is healthy (frame rate >= 30 FPS)
        // ES: Enfriar acumulador de lag cuando el rendimiento es saludable (tasa de fotogramas >= 30 FPS)
        m_recentLagDurationAccumulatorMs = std::max(0.0f, m_recentLagDurationAccumulatorMs - 8.0f);
        if (m_isUnderLagStress && m_smoothedFrameDurationMs < 34.0f && m_recentLagDurationAccumulatorMs <= 0.0f) {
            m_isUnderLagStress = false;
            logError(ErrorCategory::Performance, ErrorSeverity::Info,
                     "Lag stress cleared: Framerate restored to healthy pacing.");
        }
    }
}

// =============================================================================
// EN: Delta time clamping to strictly prevent the simulation 'spiral of death'
// ES: Limitación de delta time para prevenir la 'espiral de la muerte' en la simulación
// =============================================================================
float ErrorSystem::clampDeltaTime(float rawDeltaTimeSeconds) const noexcept {
    // EN: Prevent negative, NaN or unbounded delta times
    // ES: Evitar delta times negativos, NaN o desmedidos
    if (std::isnan(rawDeltaTimeSeconds) || rawDeltaTimeSeconds < 0.0f) {
        return 0.016f;
    }
    return std::min(rawDeltaTimeSeconds, MAX_ALLOWED_DELTA_TIME_S);
}

// =============================================================================
// EN: Audit system process memory and evaluate leak/overflow conditions
// ES: Auditar la memoria del proceso y evaluar condiciones de fuga o desborde
// =============================================================================
float ErrorSystem::auditMemoryUsage() {
    m_currentMemoryMb = queryProcessResidentMemoryMb();
    if (m_currentMemoryMb > m_peakMemoryMb) {
        m_peakMemoryMb = m_currentMemoryMb;
    }

    // EN: Verify warning and critical RAM thresholds
    // ES: Verificar umbrales de aviso y críticos de memoria RAM
    if (m_currentMemoryMb >= CRITICAL_MEMORY_MB) {
        m_shouldTriggerEmergencyPurge = true;
        m_isMemoryWarningActive = true;
        logError(ErrorCategory::Memory, ErrorSeverity::Critical,
                 "CRITICAL: Memory consumption exceeded emergency threshold! Triggering emergency purge.");
    } else if (m_currentMemoryMb >= WARNING_MEMORY_MB) {
        if (!m_isMemoryWarningActive) {
            m_isMemoryWarningActive = true;
            logError(ErrorCategory::Memory, ErrorSeverity::Warning,
                     "WARNING: Elevated memory consumption detected. Compacting old caches.");
        }
    } else {
        m_isMemoryWarningActive = false;
    }

    return m_currentMemoryMb;
}

// =============================================================================
// EN: Update current entity count and apply population safety limits
// ES: Actualizar recuento de entidades y aplicar límites seguros de población
// =============================================================================
void ErrorSystem::updateEntityCount(uint32_t currentCount) {
    m_activeEntityCount = currentCount;
    const bool wasCapped = m_isPopulationCapReached;
    m_isPopulationCapReached = (currentCount >= m_maxSafeEntityLimit);

    if (m_isPopulationCapReached && !wasCapped) {
        logError(ErrorCategory::EntityOverload, ErrorSeverity::Warning,
                 "Population safety cap reached. Temporarily pausing new births to avoid lag.");
    }
}

// =============================================================================
// EN: Validate rasterizer coordinates to prevent memory corruption/segfaults
// ES: Validar coordenadas del ráster para prevenir corrupción de memoria o segfaults
// =============================================================================
bool ErrorSystem::validateBufferAccess(int pixelX, int pixelY, int bufferWidth, int bufferHeight,
                                      const char* contextName) {
    const bool isInside = BufferGuard::isPixelWithinBounds(pixelX, pixelY, bufferWidth, bufferHeight);
    if (!isInside) {
        m_bufferViolationsPrevented++;
        // EN: Throttle logging so flood doesn't cause its own lag
        // ES: Regular la frecuencia de log para que una ráfaga no cause lag propio
        if ((m_bufferViolationLogThrottle++ % 120) == 0) {
            char logBuffer[128];
            std::snprintf(logBuffer, sizeof(logBuffer),
                          "Out-of-bounds pixel write prevented at (%d, %d) in buffer [%dx%d] (%s)",
                          pixelX, pixelY, bufferWidth, bufferHeight, contextName);
            logError(ErrorCategory::BufferSafety, ErrorSeverity::Error, logBuffer);
        }
    }
    return isInside;
}

// =============================================================================
// EN: Retrieve aggregate snapshot of system health metrics
// ES: Obtener instantánea global con las métricas de salud del sistema
// =============================================================================
SystemHealthMetrics ErrorSystem::getHealthMetrics() const noexcept {
    SystemHealthMetrics healthData;
    healthData.currentMemoryMb = m_currentMemoryMb;
    healthData.peakMemoryMb = m_peakMemoryMb;
    healthData.currentFrameTimeMs = m_lastFrameDurationMs;
    healthData.averageFrameTimeMs = m_smoothedFrameDurationMs;
    healthData.currentFps = (m_smoothedFrameDurationMs > 0.001f)
                                ? std::clamp(1000.0f / m_smoothedFrameDurationMs, 0.0f, 240.0f)
                                : 60.0f;
    healthData.activeEntityCount = m_activeEntityCount;
    healthData.maxSafeEntityLimit = m_maxSafeEntityLimit;
    healthData.droppedFrameCount = m_droppedFrameCount;
    healthData.totalErrorsLogged = static_cast<uint32_t>(m_totalLoggedErrorCount);
    healthData.bufferViolationsPrevented = m_bufferViolationsPrevented;
    healthData.isUnderLagStress = m_isUnderLagStress;
    healthData.isMemoryWarningActive = m_isMemoryWarningActive;
    healthData.isPopulationCapped = m_isPopulationCapReached;
    return healthData;
}

// =============================================================================
// EN: Retrieve recent error records in chronological order
// ES: Recuperar registros de error recientes en orden cronológico
// =============================================================================
std::vector<ErrorRecord> ErrorSystem::getRecentErrors(size_t maximumCount) const {
    std::vector<ErrorRecord> resultList;
    const size_t availableCount = std::min(m_totalLoggedErrorCount, MAX_LOGGED_ERRORS);
    const size_t returnCount = std::min(availableCount, maximumCount);

    resultList.reserve(returnCount);

    // EN: Read from circular buffer starting from the oldest requested record
    // ES: Leer del búfer circular comenzando desde el registro más antiguo solicitado
    for (size_t i = 0; i < returnCount; ++i) {
        const size_t offsetFromHead = returnCount - i;
        const size_t readIndex = (m_historyHeadIndex + MAX_LOGGED_ERRORS - offsetFromHead) % MAX_LOGGED_ERRORS;
        resultList.push_back(m_errorHistory[readIndex]);
    }

    return resultList;
}

// =============================================================================
// EN: Reset state metrics (useful for testing and level reloads)
// ES: Reiniciar métricas de estado (útil para pruebas y recarga de niveles)
// =============================================================================
void ErrorSystem::resetMetrics() noexcept {
    m_historyHeadIndex = 0;
    m_totalLoggedErrorCount = 0;
    m_droppedFrameCount = 0;
    m_isUnderLagStress = false;
    m_recentLagDurationAccumulatorMs = 0.0f;
    m_bufferViolationsPrevented = 0;
    m_bufferViolationLogThrottle = 0;
    m_shouldTriggerEmergencyPurge = false;
    m_isMemoryWarningActive = false;
    m_isPopulationCapReached = false;
    m_errorHistory.fill(ErrorRecord{});
}

// =============================================================================
// EN: Low-level query for process resident set size (RSS) via OS primitives
// ES: Consulta de bajo nivel del resident set size (RSS) del proceso mediante el SO
// =============================================================================
float ErrorSystem::queryProcessResidentMemoryMb() {
#if defined(__linux__)
    // EN: Read exact resident pages from /proc/self/statm
    // ES: Leer páginas residentes exactas de /proc/self/statm
    std::FILE* statmStream = std::fopen("/proc/self/statm", "r");
    if (statmStream != nullptr) {
        long totalVirtualPages = 0;
        long residentPages = 0;
        if (std::fscanf(statmStream, "%ld %ld", &totalVirtualPages, &residentPages) == 2) {
            std::fclose(statmStream);
            const long pageSizeBytes = sysconf(_SC_PAGESIZE);
            return static_cast<float>(residentPages * pageSizeBytes) / (1024.0f * 1024.0f);
        }
        std::fclose(statmStream);
    }
#elif defined(__unix__) || defined(__APPLE__)
    struct rusage processResourceUsage;
    if (getrusage(RUSAGE_SELF, &processResourceUsage) == 0) {
        return static_cast<float>(processResourceUsage.ru_maxrss) / 1024.0f;
    }
#endif
    return 32.0f; // EN: Fallback baseline / ES: Línea base de respaldo
}

} // namespace ReligionSim
