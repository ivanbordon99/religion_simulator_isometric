#pragma once
#include "FirstEraAIController.hpp"
#include <string>
#include <algorithm>

// Enumeraciones para clasificaciones sistémicas
enum class EraType {
    PRIMITIVE_TRIBAL,
    BRONZE_AGE,
    CLASSICAL,
    MEDIEVAL
};

enum class CommunityRole {
    FORAGER,      // Recolector / Cazador base
    SHAMAN,       // Mediador local / Cohesión (Primera Era)
    PROPHET,      // Disruptor / Cambio de dogma
    ELDER,        // Sabio / Alta cohesión comunitaria
    ZEALOT        // Fanático / Impulsor de Voluntad Divina
};

enum class HumanAIState {
    IDLE,
    SURVIVAL_WORKING, // Cubriendo necesidades biológicas y energía
    WORSHIPING,       // Canalizando Fervor / Devoción hacia el altar
    SOCIALIZING,      // Fortaleciendo lazos de tribu (Inercia de conversión)
    CONVERTING,       // Intentando influenciar a otras unidades
    MIGRATING,        // Desplazamiento autónomo
    DEFENDING,        // Defensa activa ante invasores o peligros
    ATTACKING         // Ofensiva o combate directo contra enemigos
};

// Estructura principal que define a una unidad humana viva con todos sus atributos sistémicos
struct HumanUnit {
    // Identidad y Pertenencia
    std::string unitName{"Grog"};
    std::string tribeID{"Clan Solar"};          // Cambia según la era o evolución cultural
    std::string leaderOrProphetID{"Solarius"};  // Dios, líder o profeta al que siguen o sirven
    CommunityRole role{CommunityRole::FORAGER};
    ::EraType currentEra{::EraType::PRIMITIVE_TRIBAL};

    // Atributos Vitales y Físicos
    float hitPoints{100.0f};          // Puntos de vida (HP max, ej. 100.0f)
    float attackPower{12.0f};         // Fuerza de ataque física
    float energy{85.0f};              // Energía o estamina para realizar acciones (0.0f - 100.0f)
    float resistance{25.0f};          // Resistencia a fatiga, clima o daño ambiental

    // Atributos Espirituales y Psicológicos (Los dos ejes y conversión)
    float willpower{70.0f};           // Fuerza de voluntad general
    float devotion{80.0f};            // Devoción hacia la divinidad o dogma actual (0.0f - 100.0f)
    float conversionPower{15.0f};     // Capacidad para influenciar o convertir a otros
    float conversionInertia{40.0f};   // Resistencia a cambiar de religión/tribu (Inercia de conversión)

    // Estado y Posicionamiento
    HumanAIState currentState{HumanAIState::SOCIALIZING};
    Vector2 position{0.0f, 0.0f};     // Coordenadas en el mapa del motor
    float nutritionalState{75.0f};    // Estado biológico de hambre/comida interna
};

// Contexto ambiental y social circundante para la toma de decisiones de la unidad
struct UnitEnvironmentContext {
    bool hasNearbyShrine{false};
    bool isBorderOrConflictZone{false};
    float localDangerLevel{0.0f};
    float externalConversionPressure{0.0f}; // Presión ideológica de otra tribu cercana
};

class HumanAIController {
public:
    // Método principal de actualización para la IA individual de cada humano
    void UpdateHumanAI(HumanUnit& human, const UnitEnvironmentContext& env, float deltaTime) {
        
        // 0. Si la unidad está en combate defensivo u ofensivo activo, mantener la postura
        if (human.currentState == HumanAIState::DEFENDING || human.currentState == HumanAIState::ATTACKING) {
            human.energy = std::max(0.0f, human.energy - 2.5f * deltaTime);
            if (human.hitPoints < 20.0f && human.role != CommunityRole::ZEALOT) {
                human.currentState = HumanAIState::SURVIVAL_WORKING;
            }
            return;
        }

        // 1. Prioridad Absoluta: Supervivencia Biológica y Recuperación de Energía
        if (human.energy < 25.0f || human.nutritionalState < 30.0f) {
            human.currentState = HumanAIState::SURVIVAL_WORKING;
            ExecuteSurvivalRoutine(human, deltaTime);
            return;
        }

        // 2. Respuesta a Presiones Externas (Conversión o Conflicto)
        if (env.externalConversionPressure > human.conversionInertia) {
            // La inercia de conversión frena el cambio inmediato, pero genera fricción
            HandleConversionResistance(human, env.externalConversionPressure, deltaTime);
        }

        // 3. Comportamiento basado en el Rol Asignado en la Tribu
        switch (human.role) {
            case CommunityRole::SHAMAN:
                UpdateShamanBehavior(human, env, deltaTime);
                break;
            case CommunityRole::PROPHET:
                UpdateProphetBehavior(human, env, deltaTime);
                break;
            case CommunityRole::ZEALOT:
                UpdateZealotBehavior(human, env, deltaTime);
                break;
            default:
                UpdateStandardVillagerBehavior(human, env, deltaTime);
                break;
        }
    }

private:
    void ExecuteSurvivalRoutine(HumanUnit& human, float deltaTime) {
        // La unidad descansa o recolecta lo indispensable, reduciendo la devoción temporalmente
        human.energy = std::min(100.0f, human.energy + 15.0f * deltaTime);
        human.nutritionalState += 10.0f * deltaTime;
        human.devotion = std::max(0.0f, human.devotion - 0.2f * deltaTime);
        human.currentState = HumanAIState::SURVIVAL_WORKING;
    }

    void HandleConversionResistance(HumanUnit& human, float pressure, float deltaTime) {
        // Si la presión externa supera la inercia, la devoción tambalea
        human.devotion = std::max(0.0f, human.devotion - (pressure - human.conversionInertia) * deltaTime);
    }

    void UpdateShamanBehavior(HumanUnit& human, const UnitEnvironmentContext& env, float deltaTime) {
        (void)env;
        // El chamán prioriza mantener la cohesión local y generar estabilidad
        human.currentState = HumanAIState::SOCIALIZING;
        human.devotion = std::min(100.0f, human.devotion + 0.5f * deltaTime);
        human.energy = std::max(0.0f, human.energy - 2.0f * deltaTime);
    }

    void UpdateProphetBehavior(HumanUnit& human, const UnitEnvironmentContext& env, float deltaTime) {
        (void)env;
        // El profeta rompe esquemas y proyecta un alto poder de conversión
        human.currentState = HumanAIState::CONVERTING;
        human.conversionPower += 0.1f * deltaTime;
    }

    void UpdateZealotBehavior(HumanUnit& human, const UnitEnvironmentContext& env, float deltaTime) {
        (void)env;
        // El fanático prioriza el culto extremo y la Voluntad Divina
        human.currentState = HumanAIState::WORSHIPING;
        human.devotion = std::min(100.0f, human.devotion + 1.0f * deltaTime);
    }

    void UpdateStandardVillagerBehavior(HumanUnit& human, const UnitEnvironmentContext& env, float deltaTime) {
        if (env.hasNearbyShrine && human.devotion > 50.0f) {
            human.currentState = HumanAIState::WORSHIPING;
            human.devotion = std::min(100.0f, human.devotion + 0.3f * deltaTime);
        } else {
            human.currentState = HumanAIState::SOCIALIZING;
        }
        human.energy = std::max(0.0f, human.energy - 1.0f * deltaTime);
    }
};
