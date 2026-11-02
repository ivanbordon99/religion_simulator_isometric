#include "EraEvolutionEngine.hpp"
#include <iomanip>

namespace ReligionSim {

// Demostración de bucle principal decoupled (Fixed Timestep Accumulator Pattern)
// adaptable a SDL2 (SDL_GetTicks / SDL_PollEvent) y OpenGL (SwapBuffers)
void RunSimulatedGameLoopExample() {
    HistoricalEvolutionEngine engine;

    // Escuchar el evento crítico de transición de Era para notificar a la interfaz de usuario
    engine.getEraManager().setMilestoneListener([](const MilestoneReport& report) {
        std::cout << "\n====================================================================\n";
        std::cout << "[HITO HISTORICO - TIPPING POINT CRITICO ACTIVADO]\n";
        std::cout << "Ano: " << report.transitionYear << " BCE\n";
        std::cout << "Nueva Era: " << report.eraTitle << "\n";
        std::cout << "Cronica: " << report.chronicleSummary << "\n";
        std::cout << "Parametros: Material=" << report.finalStats.materialProgress
                  << " | Fe=" << report.finalStats.mysticalFaithResonance
                  << " | Crisis=" << report.finalStats.crisisIndex << "\n";
        std::cout << "ESTADO: Velocidad forzada a Pausa (0X). Simulacion biologica congelada.\n";
        std::cout << "====================================================================\n\n";
    });

    std::cout << "[GameLoop] Iniciando motor de simulacion con tiempo cronologico y causa-efecto...\n";

    // Simular paso de ticks reales (60 FPS, dt = 16.6ms)
    constexpr float FIXED_DT = 1.0f / 60.0f;

    // Acelerar tiempo a 5X para el avance cronológico
    engine.getClock().setSpeed(TimeSpeed::Speed5X);
    engine.getClock().setYearsPerRealSecond(20.0); // 20 anos solares por segundo real

    // Inyectar fe por ritos chamánicos y desbloquear fuego controlado
    engine.getEvolution().applyImpact(+150.0f, +180.0f, +5.0f);
    engine.getDiscoveries().unlock(101, engine.getClock().getCurrentYear());

    bool running = true;
    int frameCount = 0;

    while (running && frameCount < 300) {
        frameCount++;

        // 1. Procesamiento de entradas del usuario (ej. SDL_PollEvent / Hotkeys)
        // [Z] Milagro, [X] Plaga, [Space] Pausa

        // 2. Actualización del doble motor de simulación
        engine.update(FIXED_DT);

        // 3. Comprobación de estado tras evaluación de Tipping Point
        if (engine.getClock().isPaused() && engine.getEraManager().isTransitionPending()) {
            std::cout << "[GameLoop Frame " << frameCount << "] Tipping point detectado en ano "
                      << engine.getClock().getCurrentYear() << ". Simulación pasiva en pausa." << std::endl;

            // La GUI muestra el modal de ascensión de Era al usuario; el usuario confirma y reanuda
            engine.getEraManager().acknowledgeTransition();
            engine.getClock().setSpeed(TimeSpeed::Speed2X); // Reanudar a 2X
        }

        // 4. Renderizado (OpenGL / SDL2 RenderPass con buffer 2D)
    }

    std::cout << "[GameLoop] Ciclo de prueba finalizado con exito.\n";
}

} // namespace ReligionSim
