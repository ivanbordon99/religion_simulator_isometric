#pragma once

#include "Types.hpp"
#include <string>
#include <vector>
#include <cstdint>
#include <chrono>

namespace ReligionSim {

class Game;

// Metadatos de un Mundo Guardado (inspirado en level.dat y lista de mundos de Minecraft)
struct WorldSaveMeta {
    std::string folderName;       // p. ej. "Partida_Solarius_1731362400"
    std::string worldName;        // Nombre visible: "Imperio de Solarius"
    std::string godName;          // Dios rector
    std::string religionName;     // Culto principal
    uint32_t seed{42};            // Semilla del generador
    int currentYear{-4000};       // Año de la partida
    HistoricalEra era{HistoricalEra::PrimordialAnimism};
    int totalPopulation{80};
    int activeReligions{5};
    int64_t lastPlayedTimestamp{0}; // Epoch milisegundos
    std::string iconPPMPath;      // Ruta a icon.ppm / icon.png
    bool hasIcon{false};
    int mapWidth{64};
    int mapHeight{64};
    float timeOfDay{9.0f};
    int completedHouses{0};
    int totalHouses{0};
    std::string slotId;
};

class SaveSystem {
public:
    static std::string getSavesRootDirectory();

    // EN: Generate unique independent save slot name
    // ES: Generar nombre de espacio de guardado independiente único
    static std::string generateUniqueSlotName(const std::string& godName, uint32_t seed);

    // Descubrimiento y lectura de mundos existentes
    static std::vector<WorldSaveMeta> scanSaveSlots();

    // Guardar partida completa con captura de snapshot
    static bool saveWorld(Game& game, const std::string& worldFolderName);

    // Cargar partida completa
    static bool loadWorld(Game& game, const std::string& worldFolderName);

    // Eliminar mundo guardado
    static bool deleteWorld(const std::string& worldFolderName);

    // Formatear fecha y hora estilo Minecraft (ej. "24/10/2026 01:15")
    static std::string formatTimestamp(int64_t epochMs);

    // Formatear tamaño o resumen (ej. "48x48 | 280k hab | 4000 BCE")
    static std::string formatSummary(const WorldSaveMeta& meta);
};

} // namespace ReligionSim
