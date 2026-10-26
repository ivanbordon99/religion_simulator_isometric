#pragma once
#include "Types.hpp"
#if __has_include(<raylib.h>)
#include <raylib.h>
#else
struct Vector2 {
    float x{0.0f};
    float y{0.0f};
};
#endif
#include <algorithm>
#include <cmath>

// Estados autónomos realistas para los aldeanos (Pops) de la primera era
enum class RealisticVillagerState {
    SURVIVAL_FORAGING, // Supervivencia estricta (recolección/caza prioritaria)
    COMMUNAL_BONDING,  // Fortalecimiento de lazos sociales internos (Unidad)
    RURAL_WORSHIP,     // Prácticas chamánicas locales (Dogma)
    CAUTIOUS_CONTACT,  // Vigilancia y comercio silencioso en frontera
    AUTONOMOUS_SPLIT   // Escisión del clan por presión demográfica
};

// Estructura demográfica y motivacional del grupo
struct RealisticPopGroup {
    int populationCount{15};
    float foodSupply{65.0f};       // Recursos físicos (0.0f a 100.0f)
    float divineWill{70.0f};       // Eje vertical: Dogma y Fervor (0.0f a 100.0f)
    float unityWill{60.0f};        // Eje horizontal: Cohesión y Solidaridad (0.0f a 100.0f)
    
    RealisticVillagerState state{RealisticVillagerState::COMMUNAL_BONDING};
    Vector2 coordinates{0.0f, 0.0f};
};

// Contexto ambiental de la celda y entorno físico
struct RealisticCellContext {
    bool hasNeighborTribe{false};
    float neighborTribePowerRatio{1.0f};       // Poder relativo del grupo vecino (0.1 a 2.0)
    float environmentalCarryingCapacity{80.0f}; // Límite de recursos ecológicos de la celda
    bool hasShrine{false};
    ReligionSim::BiomeType currentBiome{ReligionSim::BiomeType::CoastalPlains};
};

class FirstEraAIController {
public:
    // Método principal de actualización autónoma (Ejecutado por cada grupo en el loop del juego)
    void UpdateRealisticBehavior(RealisticPopGroup& pop, const RealisticCellContext& context, float deltaTime) {
        if (!std::isfinite(deltaTime) || deltaTime <= 0.0f) {
            return;
        }

        pop.populationCount = std::max(0, pop.populationCount);
        pop.foodSupply = std::clamp(pop.foodSupply, 0.0f, 100.0f);
        pop.divineWill = std::clamp(pop.divineWill, 0.0f, 100.0f);
        pop.unityWill = std::clamp(pop.unityWill, 0.0f, 100.0f);

        const float biomePenalty =
            context.currentBiome == ReligionSim::BiomeType::AridDesert ? 1.8f : 1.0f;
        const float foodThreshold = 35.0f * biomePenalty;

        // 1. Restricción Ecológica y Antropológica Absoluta: el hambre anula el dogma.
        if (pop.foodSupply < foodThreshold) {
            pop.state = RealisticVillagerState::SURVIVAL_FORAGING;
            ExecuteSurvivalLogic(pop, context, deltaTime, biomePenalty);
            return;
        }

        // 2. Realismo de Frontera: Encuentro cauteloso con otras tribus vecinas
        if (context.hasNeighborTribe) {
            pop.state = RealisticVillagerState::CAUTIOUS_CONTACT;
            HandleRealisticBorderEncounter(pop, context.neighborTribePowerRatio, deltaTime);
            return;
        }

        // 3. Dinámica Demográfica: Superpoblación real provoca división autónoma del clan
        if (pop.unityWill > 65.0f && pop.foodSupply > 75.0f && pop.populationCount > 40) {
            pop.state = RealisticVillagerState::AUTONOMOUS_SPLIT;
            HandleAutonomousMigration(pop);
            return;
        }

        // 4. Dinámica de los Dos Ejes (Voluntad Divina vs Voluntad de Unidad)
        if (pop.divineWill > pop.unityWill && context.hasShrine) {
            pop.state = RealisticVillagerState::RURAL_WORSHIP;
            pop.divineWill = std::min(100.0f, pop.divineWill + 0.2f * deltaTime);
        } 
        else {
            pop.state = RealisticVillagerState::COMMUNAL_BONDING;
            pop.unityWill = std::min(100.0f, pop.unityWill + 0.3f * deltaTime);
        }
    }

private:
    void ExecuteSurvivalLogic(RealisticPopGroup& pop, const RealisticCellContext& context,
                              float deltaTime, float biomePenalty) {
        // La población prioriza la búsqueda física; el fervor decae rápidamente ante la escasez
        const float resourceFactor = std::clamp(context.environmentalCarryingCapacity, 0.0f, 100.0f) / 80.0f;
        const float foodRecovery = (4.0f * resourceFactor / biomePenalty) * deltaTime;
        pop.foodSupply = std::min(100.0f, pop.foodSupply + foodRecovery);
        pop.divineWill = std::max(0.0f, pop.divineWill - 0.8f * deltaTime);
    }

    void HandleRealisticBorderEncounter(RealisticPopGroup& pop, float powerRatio, float deltaTime) {
        if (powerRatio < 0.6f) {
            // Grupo más débil: Aumenta la cautela y buscan cohesión y protección espiritual
            pop.unityWill = std::min(100.0f, pop.unityWill + 0.4f * deltaTime);
            pop.divineWill = std::min(100.0f, pop.divineWill + 0.2f * deltaTime); 
        } else {
            // Fuerzas parejas: Intercambio cultural lento, fricción controlada
            pop.unityWill = std::max(0.0f, pop.unityWill - 0.1f * deltaTime);
        }
    }

    void HandleAutonomousMigration(RealisticPopGroup& pop) {
        // El asentamiento se divide de forma orgánica para no sobrepasar la capacidad de carga
        pop.populationCount /= 2;
        pop.foodSupply /= 2.0f;
    }
};
