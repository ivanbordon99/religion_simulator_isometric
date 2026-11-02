#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>
#include <algorithm>
#include <iostream>

namespace ReligionSim {

// ============================================================================
// 1. RELOJ DE JUEGO Y GESTIÓN TEMPORAL CRONOLÓGICA (GameClock)
// ============================================================================

enum class TimeSpeed : uint8_t {
    Pause   = 0, // 0X: Simulación congelada (evaluaciones críticas, inspección)
    Speed1X = 1, // 1X: Ritmo estándar de tiempo biológico y cronológico
    Speed2X = 2, // 2X: Aceleración moderada
    Speed5X = 5  // 5X: Salto generacional macro-histórico
};

class GameClock {
public:
    explicit GameClock(double startYear = -4000.0)
        : m_currentYear(startYear), m_baseYear(startYear) {}

    void update(float realDeltaSeconds) {
        if (m_speed == TimeSpeed::Pause) {
            m_scaledDeltaSeconds = 0.0f;
            m_unscaledDeltaSeconds = realDeltaSeconds;
            return;
        }

        m_unscaledDeltaSeconds = realDeltaSeconds;
        const float multiplier = static_cast<float>(m_speed);
        m_scaledDeltaSeconds = realDeltaSeconds * multiplier;

        // Avance del calendario macro (años solares y meses biológicos)
        m_yearAccumulator += static_cast<double>(m_scaledDeltaSeconds) * m_yearsPerRealSecond;
        while (m_yearAccumulator >= 1.0) {
            m_yearAccumulator -= 1.0;
            m_currentYear += 1.0;
            m_totalYearsElapsed++;
        }

        // Ticks cronológicos para procesos biológicos (consumo, envejecimiento, cosechas)
        m_biologicalSubTick += m_scaledDeltaSeconds;
        while (m_biologicalSubTick >= BIOLOGICAL_TICK_STEP) {
            m_biologicalSubTick -= BIOLOGICAL_TICK_STEP;
            m_totalBiologicalTicks++;
        }
    }

    void setSpeed(TimeSpeed speed) noexcept { m_speed = speed; }
    void pause() noexcept { m_speed = TimeSpeed::Pause; }
    void resume() noexcept {
        if (m_speed == TimeSpeed::Pause) m_speed = TimeSpeed::Speed1X;
    }

    [[nodiscard]] bool isPaused() const noexcept { return m_speed == TimeSpeed::Pause; }
    [[nodiscard]] TimeSpeed getSpeed() const noexcept { return m_speed; }
    [[nodiscard]] float getScaledDelta() const noexcept { return m_scaledDeltaSeconds; }
    [[nodiscard]] float getUnscaledDelta() const noexcept { return m_unscaledDeltaSeconds; }
    [[nodiscard]] double getCurrentYear() const noexcept { return m_currentYear; }
    [[nodiscard]] uint64_t getTotalYearsElapsed() const noexcept { return m_totalYearsElapsed; }
    [[nodiscard]] uint64_t getBiologicalTicks() const noexcept { return m_totalBiologicalTicks; }
    void setYearsPerRealSecond(double yps) noexcept { m_yearsPerRealSecond = yps; }

private:
    static constexpr float BIOLOGICAL_TICK_STEP = 0.1f; // 10 Hz para procesos metabólicos

    TimeSpeed m_speed{TimeSpeed::Speed1X};
    float m_unscaledDeltaSeconds{0.0f};
    float m_scaledDeltaSeconds{0.0f};

    double m_currentYear{-4000.0};
    double m_baseYear{-4000.0};
    double m_yearAccumulator{0.0};
    double m_yearsPerRealSecond{10.0}; // Escala base elástica

    float m_biologicalSubTick{0.0f};
    uint64_t m_totalBiologicalTicks{0};
    uint64_t m_totalYearsElapsed{0};
};

// ============================================================================
// 2. SISTEMA DE DESCUBRIMIENTOS MATERIALES Y TÉCNICOS (DiscoverySystem)
// ============================================================================

enum class DiscoveryCategory : uint8_t {
    MaterialExtraction, // Sílex pulido, minería, arcilla
    Thermodynamics,     // Fuego controlado, fundición de bronce, forja de hierro
    AgrarianTechnology, // Riego artificial, arado de tracción, graneros comunales
    SocialEngineering   // Escritura cuneiforme, códigos legales, moneda acuñada
};

struct Discovery {
    uint32_t id{0};
    std::string code;
    std::string name;
    std::string description;
    DiscoveryCategory category{DiscoveryCategory::MaterialExtraction};

    bool unlocked{false};
    double yearUnlocked{0.0};

    // Modificadores físicos y de producción en el mundo
    float productionMultiplier{1.0f}; // Impacto en la extracción y manufactura
    float carryingCapacityBonus{0.0f}; // Crecimiento demográfico sostenible
    float environmentalResilience{0.0f}; // Amortiguación ante sequías/plagas
    float materialProgressWeight{100.0f}; // Aporte directo al acumulador histórico
};

class DiscoverySystem {
public:
    void registerDiscovery(const Discovery& disc) {
        m_catalog[disc.id] = disc;
    }

    bool unlock(uint32_t id, double currentYear) {
        auto it = m_catalog.find(id);
        if (it == m_catalog.end() || it->second.unlocked) return false;

        it->second.unlocked = true;
        it->second.yearUnlocked = currentYear;
        m_recentlyUnlocked.push_back(id);
        return true;
    }

    [[nodiscard]] bool isUnlocked(uint32_t id) const {
        auto it = m_catalog.find(id);
        return it != m_catalog.end() && it->second.unlocked;
    }

    [[nodiscard]] const Discovery* getDiscovery(uint32_t id) const {
        auto it = m_catalog.find(id);
        return (it != m_catalog.end()) ? &it->second : nullptr;
    }

    // Extrae y limpia la cola de descubrimientos recién completados
    std::vector<uint32_t> consumeRecentUnlocks() {
        std::vector<uint32_t> out = std::move(m_recentlyUnlocked);
        m_recentlyUnlocked.clear();
        return out;
    }

    // Suma ponderada de capacidades materiales y demográficas actuales
    [[nodiscard]] float calculateNetMaterialBonus() const {
        float mult = 1.0f;
        for (const auto& [id, d] : m_catalog) {
            if (d.unlocked) mult *= d.productionMultiplier;
        }
        return mult;
    }

    [[nodiscard]] float calculateTotalCarryingCapacityBonus() const {
        float bonus = 0.0f;
        for (const auto& [id, d] : m_catalog) {
            if (d.unlocked) bonus += d.carryingCapacityBonus;
        }
        return bonus;
    }

    [[nodiscard]] size_t getUnlockedCount() const {
        size_t count = 0;
        for (const auto& [id, d] : m_catalog) {
            if (d.unlocked) count++;
        }
        return count;
    }

private:
    std::unordered_map<uint32_t, Discovery> m_catalog;
    std::vector<uint32_t> m_recentlyUnlocked;
};

// ============================================================================
// 3. GENERADOR DE ACONTECIMIENTOS Y CRISIS EMERGENTES (EventSystem)
// ============================================================================

enum class EventType : uint8_t {
    EnvironmentalCatastrophe, // Sequías extremas, erupciones volcánicas, inundaciones
    MysticalRevelation,       // Visiones chamánicas, epifanías proféticas, teofanías
    IdeologicalCrisis,        // Cismas teológicos, herejías iconoclastas, rebelión civil
    DemographicExodus         // Colapso de cosechas y migraciones forzadas
};

struct HistoricEvent {
    uint32_t id{0};
    std::string title;
    std::string narrative;
    EventType type{EventType::EnvironmentalCatastrophe};
    double yearOccurred{0.0};

    // Vector de impacto sobre los acumuladores históricos de la civilización
    float materialDelta{0.0f};  // Pérdida o auge de infraestructura y grano
    float faithDelta{0.0f};     // Aumento de fervor místico o pánico religioso
    float crisisDelta{0.0f};    // Presión sobre la estabilidad social
};

class EventSystem {
public:
    using EventObserver = std::function<void(const HistoricEvent&)>;

    void subscribe(EventObserver observer) {
        m_observers.push_back(observer);
    }

    void emitEvent(const HistoricEvent& ev) {
        m_eventHistory.push_back(ev);
        for (const auto& obs : m_observers) {
            if (obs) obs(ev);
        }
    }

    [[nodiscard]] const std::vector<HistoricEvent>& getHistory() const noexcept {
        return m_eventHistory;
    }

    // Generador de eventos de estrés ambiental o revelación espiritual
    void evaluateEmergentEvents(double currentYear, float crisisIndex, float faithResonance) {
        m_eventCooldown -= 1.0f;
        if (m_eventCooldown > 0.0f) return;

        // Probabilidad de cisma si el índice de crisis se dispara bajo alta fe
        if (crisisIndex > 65.0f && faithResonance > 400.0f) {
            HistoricEvent cisma;
            cisma.id = static_cast<uint32_t>(m_eventHistory.size() + 1);
            cisma.title = "Cisma del Fuego Purificador";
            cisma.narrative = "El hambre prolongada divide al sacerdocio: una facción fanática acusa a los ancianos de tibieza ritual.";
            cisma.type = EventType::IdeologicalCrisis;
            cisma.yearOccurred = currentYear;
            cisma.faithDelta = +120.0f; // El conflicto dogmático inflama el fervor
            cisma.crisisDelta = +25.0f;
            cisma.materialDelta = -40.0f;
            emitEvent(cisma);
            m_eventCooldown = 15.0f;
            return;
        }

        // Epifanía profética en momentos de alta resonancia espiritual
        if (faithResonance > 600.0f && crisisIndex < 30.0f) {
            HistoricEvent vision;
            vision.id = static_cast<uint32_t>(m_eventHistory.size() + 1);
            vision.title = "Aparición de la Rueda Celeste";
            vision.narrative = "Una aurora sagrada desciende sobre el templo; los profetas codifican las primeras escrituras divinas.";
            vision.type = EventType::MysticalRevelation;
            vision.yearOccurred = currentYear;
            vision.faithDelta = +180.0f;
            vision.crisisDelta = -15.0f; // La revelación unifica y calma a la población
            vision.materialDelta = +30.0f;
            emitEvent(vision);
            m_eventCooldown = 25.0f;
        }
    }

private:
    std::vector<HistoricEvent> m_eventHistory;
    std::vector<EventObserver> m_observers;
    float m_eventCooldown{5.0f};
};

// ============================================================================
// 4. GESTOR DE EVOLUCIÓN HISTÓRICA Y ACUMULADORES (HistoryEvolutionManager)
// ============================================================================

struct EvolutionAccumulators {
    float materialProgress{0.0f};     // Eje X: Fuerzas productivas, demografía y excedente
    float mysticalFaithResonance{0.0f}; // Eje Y: Fervor, dogmas, templos y teología
    float crisisIndex{0.0f};          // Eje Z: Tensión ambiental, hambrunas y herejías (0..100)
};

class HistoryEvolutionManager {
public:
    HistoryEvolutionManager() = default;

    void update(float dt, const GameClock& clock, DiscoverySystem& discoveries, EventSystem& events) {
        if (clock.isPaused()) return;

        // 1. Integración de descubrimientos materiales recién desbloqueados
        auto recentUnlocks = discoveries.consumeRecentUnlocks();
        for (uint32_t dId : recentUnlocks) {
            const auto* d = discoveries.getDiscovery(dId);
            if (d) {
                m_accumulators.materialProgress += d->materialProgressWeight;
                // Un gran salto técnico alivia el índice de crisis ambiental
                m_accumulators.crisisIndex = std::max(0.0f, m_accumulators.crisisIndex - d->environmentalResilience * 10.0f);
            }
        }

        // 2. Dinámica de retroalimentación biológica y cronológica pasiva
        // El crecimiento demográfico aporta un goteo constante de progreso material
        float netProduction = discoveries.calculateNetMaterialBonus();
        m_accumulators.materialProgress += 1.8f * netProduction * dt;

        // La resonancia de fe decae suavemente con el tiempo secular a menos que se mantenga viva
        m_accumulators.mysticalFaithResonance = std::max(0.0f, m_accumulators.mysticalFaithResonance - 0.25f * dt);

        // 3. El índice de crisis se autorregula hacia la media ambiental
        if (m_accumulators.crisisIndex > 10.0f) {
            m_accumulators.crisisIndex -= 0.6f * dt; // Sanación gradual tras catástrofes
        }

        // 4. Evaluar emergencias dinámicas que alteran los acumuladores
        events.evaluateEmergentEvents(clock.getCurrentYear(), m_accumulators.crisisIndex, m_accumulators.mysticalFaithResonance);
    }

    void applyImpact(float materialDelta, float faithDelta, float crisisDelta) {
        m_accumulators.materialProgress = std::max(0.0f, m_accumulators.materialProgress + materialDelta);
        m_accumulators.mysticalFaithResonance = std::max(0.0f, m_accumulators.mysticalFaithResonance + faithDelta);
        m_accumulators.crisisIndex = std::clamp(m_accumulators.crisisIndex + crisisDelta, 0.0f, 100.0f);
    }

    [[nodiscard]] const EvolutionAccumulators& getAccumulators() const noexcept {
        return m_accumulators;
    }
    [[nodiscard]] EvolutionAccumulators& getAccumulatorsMutable() noexcept {
        return m_accumulators;
    }

private:
    EvolutionAccumulators m_accumulators{50.0f, 100.0f, 10.0f}; // Condiciones iniciales del clan
};

// ============================================================================
// 5. MÁQUINA DE ESTADOS DE ERAS Y PUNTOS DE INFLEXIÓN (EraManager)
// ============================================================================

enum class CivilizationEra : uint8_t {
    PrimordialAnimism = 0,       // Bandas nómadas, espíritus de la naturaleza
    StructuralShamanism = 1,     // Santuarios megalíticos, ritos de fertilidad
    TerritorialPolytheism = 2,   // Ciudades-Estado, panteones sagrados, metalurgia
    UniversalReligions = 3,      // Dogma escrito, monoteísmo, misiones continentales
    RationalizationSecular = 4,  // Ilustración, empirismo, secularización
    TranshumanistSingularity = 5 // Posbiología, mente unificada, trascendencia
};

struct MilestoneReport {
    CivilizationEra previousEra;
    CivilizationEra newEra;
    std::string eraTitle;
    std::string chronicleSummary;
    double transitionYear;
    EvolutionAccumulators finalStats;
};

struct TippingPointCondition {
    CivilizationEra targetEra;
    std::string eraTitle;
    std::string narrative;

    double minYearThreshold;
    float requiredMaterialProgress;
    float requiredFaithResonance;
    float minCrisisCatalyst; // La ruptura de paradigma requiere superación de una crisis o umbral de estrés
    std::vector<uint32_t> mandatoryDiscoveryIds;
};

class EraManager {
public:
    using MilestoneCallback = std::function<void(const MilestoneReport&)>;

    EraManager() {
        configureHistoricalTippingPoints();
    }

    void setMilestoneListener(MilestoneCallback cb) {
        m_onMilestone = cb;
    }

    [[nodiscard]] CivilizationEra getCurrentEra() const noexcept {
        return m_currentEra;
    }

    // ========================================================================
    // EVALUACIÓN DE PUNTOS DE INFLEXIÓN (Tipping Points)
    // ========================================================================
    bool evaluateTippingPoints(GameClock& clock,
                              HistoryEvolutionManager& evolution,
                              const DiscoverySystem& discoveries) {
        if (m_transitionPending) return false;

        const auto& accum = evolution.getAccumulators();
        double currentYear = clock.getCurrentYear();

        for (const auto& tp : m_tippingPoints) {
            if (tp.targetEra <= m_currentEra) continue;

            // 1. Filtro cronológico mínimo (madurez biológica y climática del mundo)
            if (currentYear < tp.minYearThreshold) continue;

            // 2. Filtro material y productivo
            if (accum.materialProgress < tp.requiredMaterialProgress) continue;

            // 3. Filtro de resonancia espiritual / fe
            if (accum.mysticalFaithResonance < tp.requiredFaithResonance) continue;

            // 4. Verificación de tecnologías o conceptos obligatorios
            bool missingTech = false;
            for (uint32_t techId : tp.mandatoryDiscoveryIds) {
                if (!discoveries.isUnlocked(techId)) {
                    missingTech = true;
                    break;
                }
            }
            if (missingTech) continue;

            // ================================================================
            // CRUCE DE UMBRAL CRÍTICO: DISPARO DE INTERRUPCIÓN Y PAUSA
            // ================================================================
            triggerEraTransition(tp, clock, accum);
            return true;
        }

        return false;
    }

    void acknowledgeTransition() {
        m_transitionPending = false;
    }

    [[nodiscard]] bool isTransitionPending() const noexcept {
        return m_transitionPending;
    }

    [[nodiscard]] const std::optional<MilestoneReport>& getLastMilestone() const noexcept {
        return m_lastMilestone;
    }

private:
    void triggerEraTransition(const TippingPointCondition& tp,
                              GameClock& clock,
                              const EvolutionAccumulators& accum) {
        CivilizationEra oldEra = m_currentEra;
        m_currentEra = tp.targetEra;
        m_transitionPending = true;

        // Interrupción obligatoria del flujo temporal: Pausa 0X para contemplación histórica
        clock.pause();

        MilestoneReport report{
            oldEra,
            m_currentEra,
            tp.eraTitle,
            tp.narrative,
            clock.getCurrentYear(),
            accum
        };

        m_lastMilestone = report;

        // Notificar inmediatamente al subsistema de renderizado / GUI
        if (m_onMilestone) {
            m_onMilestone(report);
        }
    }

    void configureHistoricalTippingPoints() {
        m_tippingPoints.push_back({
            CivilizationEra::StructuralShamanism,
            "ERA DEL CHAMANISMO ESTRUCTURAL",
            "El dominio del fuego y la talla de herramientas consolidan el primer culto formal. "
            "La tribu erige monolitos y altares sacrificiales en los valles sagrados.",
            -3800.0, // Año cronológico mínimo
            220.0f,  // Progreso Material
            250.0f,  // Resonancia de Fe
            0.0f,
            {101}    // Requiere descubrimiento ID 101: Fuego Controlado
        });

        m_tippingPoints.push_back({
            CivilizationEra::TerritorialPolytheism,
            "ERA DEL POLITEISMO TERRITORIAL",
            "La agricultura excedentaria y la fundición de bronce propician el surgimiento de ciudades-estado. "
            "Panteones de dioses guerreros legitiman la autoridad de reyes-sacerdotes.",
            -2500.0,
            650.0f,
            600.0f,
            0.0f,
            {101, 102} // Requiere ID 101 (Fuego) e ID 102 (Agricultura y Riego)
        });

        m_tippingPoints.push_back({
            CivilizationEra::UniversalReligions,
            "ERA DE LAS RELIGIONES UNIVERSALES",
            "Las crisis doctrinales y las rutas continentales dan a luz a escrituras canónicas. "
            "El dogma universal promete salvación individual más allá de las fronteras tribales.",
            -800.0,
            1400.0f,
            1200.0f,
            20.0f,
            {101, 102, 103} // Requiere Escritura y Código Canónico
        });
    }

    CivilizationEra m_currentEra{CivilizationEra::PrimordialAnimism};
    bool m_transitionPending{false};
    std::optional<MilestoneReport> m_lastMilestone;
    MilestoneCallback m_onMilestone;
    std::vector<TippingPointCondition> m_tippingPoints;
};

// ============================================================================
// 6. MOTOR INTEGRADO DE EVOLUCIÓN HISTÓRICA Y CAUSA-EFECTO
// ============================================================================

class HistoricalEvolutionEngine {
public:
    HistoricalEvolutionEngine() {
        // Enlazar eventos de crisis y revelación con los acumuladores históricos
        m_events.subscribe([this](const HistoricEvent& ev) {
            m_evolution.applyImpact(ev.materialDelta, ev.faithDelta, ev.crisisDelta);
        });

        // Registrar descubrimientos base
        m_discoveries.registerDiscovery({101, "FIRE", "Fuego Controlado", "Termodinámica inicial y cocción de alimentos", DiscoveryCategory::Thermodynamics, false, 0.0, 1.25f, 50.0f, 0.3f, 150.0f});
        m_discoveries.registerDiscovery({102, "AGRI", "Agricultura y Riego", "Sedentarismo, excedente de grano y almacenamiento", DiscoveryCategory::AgrarianTechnology, false, 0.0, 1.60f, 180.0f, 0.4f, 350.0f});
        m_discoveries.registerDiscovery({103, "SCRIPT", "Escritura Canónica", "Codificación de mitos, leyes y registros tributarios", DiscoveryCategory::SocialEngineering, false, 0.0, 1.35f, 90.0f, 0.6f, 500.0f});
    }

    GameClock& getClock() noexcept { return m_clock; }
    const GameClock& getClock() const noexcept { return m_clock; }

    DiscoverySystem& getDiscoveries() noexcept { return m_discoveries; }
    const DiscoverySystem& getDiscoveries() const noexcept { return m_discoveries; }

    EventSystem& getEvents() noexcept { return m_events; }
    const EventSystem& getEvents() const noexcept { return m_events; }

    HistoryEvolutionManager& getEvolution() noexcept { return m_evolution; }
    const HistoryEvolutionManager& getEvolution() const noexcept { return m_evolution; }

    EraManager& getEraManager() noexcept { return m_eraManager; }
    const EraManager& getEraManager() const noexcept { return m_eraManager; }

    // Ciclo integral de actualización del doble motor temporal
    void update(float realDeltaSeconds) {
        // 1. El tiempo cronológico avanza los ticks ambientales y el calendario
        m_clock.update(realDeltaSeconds);

        float dt = m_clock.getScaledDelta();

        // 2. Si el tiempo no está en Pausa (0X), procesar causa y efecto histórico
        if (!m_clock.isPaused() && dt > 0.0f) {
            m_evolution.update(dt, m_clock, m_discoveries, m_events);
        }

        // 3. Evaluar Tipping Points: si la combinación de tiempo, técnica y fe cruza
        // el umbral crítico, se fuerza Pausa inmediata y se congela la simulación pasiva
        m_eraManager.evaluateTippingPoints(m_clock, m_evolution, m_discoveries);
    }

private:
    GameClock m_clock{-4000.0};
    DiscoverySystem m_discoveries;
    EventSystem m_events;
    HistoryEvolutionManager m_evolution;
    EraManager m_eraManager;
};

} // namespace ReligionSim
