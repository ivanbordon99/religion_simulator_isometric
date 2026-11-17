#include "SaveSystem.hpp"
#include "Game.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <algorithm>

namespace fs = std::filesystem;

namespace ReligionSim {

namespace {

// =============================================================================
// EN: Write a text file with atomic replacement to prevent corruption
// ES: Escribir un archivo de texto con reemplazo atómico para prevenir corrupción
// =============================================================================
bool writeTextFileAtomic(const fs::path& targetFinalPath, const std::string& fileContent) {
    fs::path temporaryPath = targetFinalPath;
    temporaryPath += ".tmp";
    {
        std::ofstream targetFile(temporaryPath);
        if (!targetFile.is_open()) return false;
        targetFile << fileContent;
        targetFile.flush();
    }
    std::error_code errorCode;
    fs::rename(temporaryPath, targetFinalPath, errorCode);
    if (errorCode) {
        fs::remove(temporaryPath, errorCode);
        return false;
    }
    return true;
}

} // namespace

std::string SaveSystem::getSavesRootDirectory() {
    std::vector<std::string> searchPaths = {
        "saves",
        "religion_simulator_isometric/saves",
        "/home/Ivancho/Documentos/religion_simulator_isometric/saves"
    };
    for (const auto& pathCandidate : searchPaths) {
        if (fs::exists(pathCandidate) && fs::is_directory(pathCandidate)) {
            return pathCandidate;
        }
    }
    try {
        fs::create_directories("saves");
        return "saves";
    } catch (...) {
        return "saves";
    }
}

// =============================================================================
// EN: Generate unique independent save slot name with timestamp and seed suffix
// ES: Generar nombre de espacio de guardado independiente único con marca de tiempo y sufijo de semilla
// =============================================================================
std::string SaveSystem::generateUniqueSlotName(const std::string& godName, uint32_t seed) {
    auto epochSeconds = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
    std::string sanitizedGodName = godName.empty() ? "Deidad" : godName;
    for (char& characterElement : sanitizedGodName) {
        if (!std::isalnum(static_cast<unsigned char>(characterElement))) characterElement = '_';
    }
    return "Partida_" + sanitizedGodName + "_" + std::to_string(epochSeconds) + "_" + std::to_string(seed % 10000);
}

std::string SaveSystem::formatTimestamp(int64_t epochMs) {
    if (epochMs <= 0) return "Desconocida";
    std::time_t timeValue = static_cast<std::time_t>(epochMs / 1000);
    std::tm timeStructure{};
#ifdef _WIN32
    localtime_s(&timeStructure, &timeValue);
#else
    localtime_r(&timeValue, &timeStructure);
#endif
    std::ostringstream stringStream;
    stringStream << std::put_time(&timeStructure, "%d/%m/%Y %H:%M");
    return stringStream.str();
}

std::string SaveSystem::formatSummary(const WorldSaveMeta& meta) {
    std::ostringstream stringStream;
    stringStream << meta.mapWidth << "x" << meta.mapHeight << " (" << (meta.mapWidth * 5) << "m)"
                 << " | Era: " << AdvancementSystem::getEraName(meta.era)
                 << " | " << Timeline::formatYear(meta.currentYear)
                 << " | " << meta.totalPopulation << " aldeanos";
    return stringStream.str();
}

std::vector<WorldSaveMeta> SaveSystem::scanSaveSlots() {
    std::vector<WorldSaveMeta> saveSlotsList;
    std::string rootDirectory = getSavesRootDirectory();

    if (!fs::exists(rootDirectory) || !fs::is_directory(rootDirectory)) {
        return saveSlotsList;
    }

    for (const auto& directoryEntry : fs::directory_iterator(rootDirectory)) {
        if (!directoryEntry.is_directory()) continue;

        std::string folderName = directoryEntry.path().filename().string();
        std::string metaPath = (directoryEntry.path() / "level.dat").string();
        std::string iconPath = (directoryEntry.path() / "icon.ppm").string();

        if (fs::exists(metaPath)) {
            WorldSaveMeta saveMeta{};
            saveMeta.folderName = folderName;
            saveMeta.iconPPMPath = iconPath;
            saveMeta.hasIcon = fs::exists(iconPath);

            std::ifstream fileStream(metaPath);
            if (fileStream.is_open()) {
                std::string lineContent;
                while (std::getline(fileStream, lineContent)) {
                    auto delimiterPosition = lineContent.find('=');
                    if (delimiterPosition == std::string::npos) continue;
                    std::string key = lineContent.substr(0, delimiterPosition);
                    std::string value = lineContent.substr(delimiterPosition + 1);

                    if (key == "worldName") saveMeta.worldName = value;
                    else if (key == "godName") saveMeta.godName = value;
                    else if (key == "religionName") saveMeta.religionName = value;
                    else if (key == "seed") saveMeta.seed = static_cast<uint32_t>(std::stoul(value));
                    else if (key == "currentYear") saveMeta.currentYear = std::stoi(value);
                    else if (key == "era") saveMeta.era = static_cast<HistoricalEra>(std::stoi(value));
                    else if (key == "population") saveMeta.totalPopulation = std::stoi(value);
                    else if (key == "activeReligions") saveMeta.activeReligions = std::stoi(value);
                    else if (key == "lastPlayed") saveMeta.lastPlayedTimestamp = std::stoll(value);
                    else if (key == "mapWidth") saveMeta.mapWidth = std::stoi(value);
                    else if (key == "mapHeight") saveMeta.mapHeight = std::stoi(value);
                    else if (key == "timeOfDay") saveMeta.timeOfDay = std::stof(value);
                    else if (key == "slotId") saveMeta.slotId = value;
                }
            }

            if (saveMeta.worldName.empty()) {
                saveMeta.worldName = folderName;
            }
            saveSlotsList.push_back(saveMeta);
        }
    }

    // EN: Sort saves from most recent to oldest (Minecraft-style)
    // ES: Ordenar partidas de más reciente a más antigua (estilo Minecraft)
    std::sort(saveSlotsList.begin(), saveSlotsList.end(), [](const WorldSaveMeta& a, const WorldSaveMeta& b) {
        return a.lastPlayedTimestamp > b.lastPlayedTimestamp;
    });

    return saveSlotsList;
}

// =============================================================================
// EN: Save complete independent world snapshot with atomic write resilience
// ES: Guardar captura completa de mundo independiente con resiliencia de escritura atómica
// =============================================================================
bool SaveSystem::saveWorld(Game& game, const std::string& worldFolderName) {
    std::string rootDirectory = getSavesRootDirectory();
    fs::path saveDirectory = fs::path(rootDirectory) / worldFolderName;

    try {
        fs::create_directories(saveDirectory);
    } catch (const std::exception& exception) {
        std::cerr << "[SaveSystem] Error creando carpeta de guardado: " << exception.what() << std::endl;
        return false;
    }

    auto nowEpochMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();

    const auto& map = game.getMap();
    const auto& registry = game.getRegistry();
    const auto& timeline = game.getTimeline();
    const auto& config = game.getCreationConfig();

    HistoricalEra playerEra = HistoricalEra::PrimordialAnimism;
    const Religion* playerReligion = registry.getReligion(1);
    if (playerReligion) {
        playerEra = playerReligion->currentEra;
    }

    // -------------------------------------------------------------------------
    // EN: 1. Save metadata in level.dat atomically
    // ES: 1. Guardar metadatos en level.dat de forma atómica
    // -------------------------------------------------------------------------
    {
        std::ostringstream metaStream;
        metaStream << "worldName=" << (config.godName.empty() ? worldFolderName : "Mundo de " + config.godName) << "\n";
        metaStream << "godName=" << config.godName << "\n";
        metaStream << "religionName=" << config.religionName << "\n";
        metaStream << "seed=" << config.mapSeed << "\n";
        metaStream << "currentYear=" << timeline.getCurrentYear() << "\n";
        metaStream << "startYear=" << timeline.getStartYear() << "\n";
        metaStream << "era=" << static_cast<int>(playerEra) << "\n";
        metaStream << "population=" << game.getGestorHumanos().getHumanos().size() << "\n";
        metaStream << "activeReligions=" << registry.getAllReligions().size() << "\n";
        metaStream << "lastPlayed=" << nowEpochMs << "\n";
        metaStream << "mapWidth=" << map.getWidth() << "\n";
        metaStream << "mapHeight=" << map.getHeight() << "\n";
        metaStream << "timeOfDay=" << game.getDayNightCycle().getTimeOfDay() << "\n";
        metaStream << "daysPassed=" << game.getDayNightCycle().getDaysPassed() << "\n";
        metaStream << "advancementBonusYears=" << game.getTimeline().getAdvancementBonusYears() << "\n";
        metaStream << "slotId=" << worldFolderName << "\n";

        fs::path levelPath = saveDirectory / "level.dat";
        if (!writeTextFileAtomic(levelPath, metaStream.str())) {
            std::cerr << "[SaveSystem] Error al escribir level.dat" << std::endl;
            return false;
        }
    }

    // -------------------------------------------------------------------------
    // EN: 2. Save world map state, craters, and terrain resources in world.bin v3
    // ES: 2. Guardar estado del mapa, cráteres y recursos del terreno en world.bin v3
    // -------------------------------------------------------------------------
    {
        fs::path finalMapPath = saveDirectory / "world.bin";
        fs::path temporaryMapPath = saveDirectory / "world.bin.tmp";

        std::ofstream binaryFile(temporaryMapPath, std::ios::binary);
        if (!binaryFile.is_open()) return false;

        constexpr uint32_t magicHeader = 0x53414954; // 'FAIS'
        // EN: File version 4 supports multi-variant mineral and geological textures
        // ES: Versión 4 de archivo soporta variantes múltiples de minerales y texturas geológicas
        constexpr uint32_t fileVersion = 4;
        binaryFile.write(reinterpret_cast<const char*>(&magicHeader), 4);
        binaryFile.write(reinterpret_cast<const char*>(&fileVersion), 4);

        int mapWidth = map.getWidth();
        int mapHeight = map.getHeight();
        binaryFile.write(reinterpret_cast<const char*>(&mapWidth), sizeof(int));
        binaryFile.write(reinterpret_cast<const char*>(&mapHeight), sizeof(int));

        for (int cellY = 0; cellY < mapHeight; ++cellY) {
            for (int cellX = 0; cellX < mapWidth; ++cellX) {
                const Cell& cell = map.getCell(cellX, cellY);
                binaryFile.write(reinterpret_cast<const char*>(&cell.elevation), sizeof(float));
                uint8_t biomeByte = static_cast<uint8_t>(cell.biome);
                binaryFile.write(reinterpret_cast<const char*>(&biomeByte), 1);
                uint8_t primaryClimateBiome = static_cast<uint8_t>(cell.climate.primary_biome);
                uint8_t secondaryClimateBiome = static_cast<uint8_t>(cell.climate.secondary_biome);
                binaryFile.write(reinterpret_cast<const char*>(&primaryClimateBiome), 1);
                binaryFile.write(reinterpret_cast<const char*>(&secondaryClimateBiome), 1);
                binaryFile.write(reinterpret_cast<const char*>(&cell.climate.blend_factor), sizeof(float));
                binaryFile.write(reinterpret_cast<const char*>(&cell.climate.elevation), sizeof(float));
                binaryFile.write(reinterpret_cast<const char*>(&cell.climate.temperature), sizeof(float));
                binaryFile.write(reinterpret_cast<const char*>(&cell.climate.moisture), sizeof(float));
                binaryFile.write(reinterpret_cast<const char*>(&cell.population), sizeof(int));
                binaryFile.write(reinterpret_cast<const char*>(&cell.maxPopulation), sizeof(int));
                binaryFile.write(reinterpret_cast<const char*>(&cell.development), sizeof(float));
                binaryFile.write(reinterpret_cast<const char*>(&cell.devotion), sizeof(float));
                binaryFile.write(reinterpret_cast<const char*>(&cell.dominantReligionId), sizeof(int));
                binaryFile.write(reinterpret_cast<const char*>(&cell.dominantFraction), sizeof(float));
                binaryFile.write(reinterpret_cast<const char*>(&cell.shrineLevel), sizeof(int));

                uint32_t shareCount = static_cast<uint32_t>(cell.faithShare.size());
                binaryFile.write(reinterpret_cast<const char*>(&shareCount), sizeof(uint32_t));
                for (const auto& shareItem : cell.faithShare) {
                    binaryFile.write(reinterpret_cast<const char*>(&shareItem.first), sizeof(int));
                    binaryFile.write(reinterpret_cast<const char*>(&shareItem.second), sizeof(float));
                }

                // EN: Version 3: Terrain impact craters, fissures, uprooted trees, and natural resources
                // ES: Versión 3: Cráteres de impacto de terreno, fisuras, árboles arrancados y recursos naturales
                uint8_t craterByte = cell.hasImpactCrater ? 1 : 0;
                binaryFile.write(reinterpret_cast<const char*>(&craterByte), 1);
                binaryFile.write(reinterpret_cast<const char*>(&cell.craterDepth), sizeof(float));
                uint8_t craterTypeByte = static_cast<uint8_t>(cell.craterType);
                binaryFile.write(reinterpret_cast<const char*>(&craterTypeByte), 1);
                binaryFile.write(reinterpret_cast<const char*>(&cell.fissureAngle), sizeof(float));
                uint8_t treesUprootedByte = cell.hasTreesUprooted ? 1 : 0;
                binaryFile.write(reinterpret_cast<const char*>(&treesUprootedByte), 1);

                uint8_t resourceTypeByte = static_cast<uint8_t>(cell.naturalResource);
                binaryFile.write(reinterpret_cast<const char*>(&resourceTypeByte), 1);
                binaryFile.write(reinterpret_cast<const char*>(&cell.resourceAmount), sizeof(int));
                binaryFile.write(reinterpret_cast<const char*>(&cell.maxResourceAmount), sizeof(int));
                uint8_t resourceVariantByte = cell.resourceVariant;
                binaryFile.write(reinterpret_cast<const char*>(&resourceVariantByte), sizeof(uint8_t));
            }
        }
        binaryFile.flush();
        binaryFile.close();

        std::error_code renameError;
        fs::rename(temporaryMapPath, finalMapPath, renameError);
        if (renameError) return false;
    }

    // -------------------------------------------------------------------------
    // EN: 3. Save individual living villagers in villagers.dat atomically
    // ES: 3. Guardar aldeanos vivos individuales en villagers.dat de forma atómica
    // -------------------------------------------------------------------------
    {
        std::ostringstream villagerStream;
        const auto& villagers = game.getGestorHumanos().getHumanos();
        for (const auto& villagerUnit : villagers) {
            if (villagerUnit.isDead()) continue;
            villagerStream << "V:" << villagerUnit.getId() << "|"
                           << villagerUnit.getName() << "|"
                           << villagerUnit.getX() << "|"
                           << villagerUnit.getY() << "|"
                           << villagerUnit.getHealth() << "|"
                           << villagerUnit.getAge() << "|"
                           << static_cast<int>(villagerUnit.getGender()) << "|"
                           << (villagerUnit.isShaman() ? 1 : 0) << "|"
                           << (villagerUnit.isProphet() ? 1 : 0) << "|"
                           << villagerUnit.getReligionId() << "|"
                           << villagerUnit.getDevotion() << "|"
                           << (villagerUnit.isChild() ? 1 : 0) << "|"
                           << villagerUnit.getSpouseId() << "|"
                           << villagerUnit.getSpouseAffection() << "|"
                           << villagerUnit.getFatherId() << "|"
                           << villagerUnit.getMotherId() << "\n";
        }
        fs::path villagersPath = saveDirectory / "villagers.dat";
        writeTextFileAtomic(villagersPath, villagerStream.str());
    }

    // -------------------------------------------------------------------------
    // EN: 4. Save tribal houses and structures in buildings.dat atomically
    // ES: 4. Guardar viviendas y estructuras tribales en buildings.dat de forma atómica
    // -------------------------------------------------------------------------
    {
        std::ostringstream buildingStream;
        const auto& houses = game.getBuildingSystem().getHouses();
        for (const auto& houseStructure : houses) {
            buildingStream << "H:" << houseStructure.id << "|"
                           << houseStructure.cellX << "|"
                           << houseStructure.cellY << "|"
                           << houseStructure.religionId << "|"
                           << static_cast<int>(houseStructure.type) << "|"
                           << static_cast<int>(houseStructure.state) << "|"
                           << houseStructure.buildProgress << "|"
                           << houseStructure.health << "|"
                           << houseStructure.maxHealth << "|"
                           << houseStructure.woodDelivered << "|"
                           << houseStructure.woodRequired << "|"
                           << houseStructure.ownerId << "|"
                           << houseStructure.partnerId << "|"
                           << houseStructure.capacity << "|";
            for (size_t residentIndex = 0; residentIndex < houseStructure.residentIds.size(); ++residentIndex) {
                buildingStream << houseStructure.residentIds[residentIndex];
                if (residentIndex + 1 < houseStructure.residentIds.size()) buildingStream << ",";
            }
            buildingStream << "\n";
        }
        fs::path buildingsPath = saveDirectory / "buildings.dat";
        writeTextFileAtomic(buildingsPath, buildingStream.str());
    }

    // -------------------------------------------------------------------------
    // EN: 5. Save religions, doctrines, and theology points in religions.dat atomically
    // ES: 5. Guardar religiones, doctrinas y puntos teológicos en religions.dat de forma atómica
    // -------------------------------------------------------------------------
    {
        std::ostringstream religionStream;
        for (const auto& currentReligion : registry.getAllReligions()) {
            religionStream << "ID:" << currentReligion.id << "|"
                           << currentReligion.name << "|"
                           << currentReligion.founderName << "|"
                           << currentReligion.symbol << "|"
                           << currentReligion.foundedYear << "|"
                           << static_cast<int>(currentReligion.color.r) << ","
                           << static_cast<int>(currentReligion.color.g) << ","
                           << static_cast<int>(currentReligion.color.b) << "|"
                           << currentReligion.zeal << "|"
                           << currentReligion.cohesion << "|"
                           << currentReligion.theologyPoints << "|"
                           << static_cast<int>(currentReligion.currentEra) << "|"
                           << (currentReligion.isExtinct ? 1 : 0) << "|";
            for (size_t advancementIndex = 0; advancementIndex < currentReligion.unlockedAdvancements.size(); ++advancementIndex) {
                religionStream << currentReligion.unlockedAdvancements[advancementIndex];
                if (advancementIndex + 1 < currentReligion.unlockedAdvancements.size()) religionStream << ",";
            }
            religionStream << "|" << currentReligion.resources.food << "," << currentReligion.resources.wood << ","
                           << currentReligion.resources.stone << "," << currentReligion.resources.gold;
            religionStream << "\n";
        }
        fs::path religionsPath = saveDirectory / "religions.dat";
        writeTextFileAtomic(religionsPath, religionStream.str());
    }

    // -------------------------------------------------------------------------
    // EN: 6. Save moral mandates and ethical grace in morality.dat atomically
    // ES: 6. Guardar mandatos morales y gracia ética en morality.dat de forma atómica
    // -------------------------------------------------------------------------
    {
        fs::path moralityPath = saveDirectory / "morality.dat";
        writeTextFileAtomic(moralityPath, game.getMoralitySystem().serialize());
    }

    // -------------------------------------------------------------------------
    // EN: 7. Save history chronicle events in events.dat atomically
    // ES: 7. Guardar eventos de la crónica histórica en events.dat de forma atómica
    // -------------------------------------------------------------------------
    {
        std::ostringstream eventStream;
        const auto& chronicleEvents = game.getEventLine().getAllEvents();
        for (const auto& gameEvent : chronicleEvents) {
            eventStream << "E:" << gameEvent.id << "|"
                        << gameEvent.year << "|"
                        << static_cast<int>(gameEvent.category) << "|"
                        << gameEvent.title << "|"
                        << gameEvent.description << "|"
                        << gameEvent.location.x << "," << gameEvent.location.y << "|"
                        << gameEvent.religionAId << "|"
                        << gameEvent.religionBId << "|"
                        << static_cast<int>(gameEvent.badgeColor.r) << ","
                        << static_cast<int>(gameEvent.badgeColor.g) << ","
                        << static_cast<int>(gameEvent.badgeColor.b) << "|"
                        << (gameEvent.isTurningPoint ? 1 : 0) << "|"
                        << gameEvent.actorName << "|"
                        << gameEvent.consequence << "\n";
        }
        fs::path eventsPath = saveDirectory / "events.dat";
        writeTextFileAtomic(eventsPath, eventStream.str());
    }

    // -------------------------------------------------------------------------
    // EN: 8. Capture visual thumbnail snapshot icon.ppm
    // ES: 8. Capturar icono visual de snapshot en icon.ppm
    // -------------------------------------------------------------------------
    {
        std::string iconPath = (saveDirectory / "icon.ppm").string();
        game.getRenderer().exportFrameToPPM(iconPath);
    }

    std::cout << "[SaveSystem] Partida guardada exitosamente en: " << saveDirectory << std::endl;
    return true;
}

// =============================================================================
// EN: Load complete independent world snapshot restoring all subsystems
// ES: Cargar captura completa de mundo independiente restaurando todos los subsistemas
// =============================================================================
bool SaveSystem::loadWorld(Game& game, const std::string& worldFolderName) {
    std::string rootDirectory = getSavesRootDirectory();
    fs::path saveDirectory = fs::path(rootDirectory) / worldFolderName;

    if (!fs::exists(saveDirectory)) {
        std::cerr << "[SaveSystem] No existe la carpeta del mundo: " << saveDirectory << std::endl;
        return false;
    }

    // -------------------------------------------------------------------------
    // EN: 1. Read metadata from level.dat
    // ES: 1. Leer metadatos desde level.dat
    // -------------------------------------------------------------------------
    std::string levelPath = (saveDirectory / "level.dat").string();
    std::ifstream metaFile(levelPath);
    if (!metaFile.is_open()) return false;

    int currentYear = -4000;
    int startYear = -4000;
    bool hasExplicitStartYear = false;
    int mapWidthFromFile = 64;
    int mapHeightFromFile = 64;
    float timeOfDay = 0.375f; // 9.0h morning
    int daysPassed = 0;
    int advancementBonusYears = 0;
    std::string godName;
    std::string religionName;
    uint32_t seed = 42;

    std::string lineContent;
    while (std::getline(metaFile, lineContent)) {
        auto delimiterPosition = lineContent.find('=');
        if (delimiterPosition == std::string::npos) continue;
        std::string key = lineContent.substr(0, delimiterPosition);
        std::string value = lineContent.substr(delimiterPosition + 1);
        if (key == "currentYear") currentYear = std::stoi(value);
        else if (key == "startYear") { startYear = std::stoi(value); hasExplicitStartYear = true; }
        else if (key == "mapWidth") mapWidthFromFile = std::stoi(value);
        else if (key == "mapHeight") mapHeightFromFile = std::stoi(value);
        else if (key == "godName") godName = value;
        else if (key == "religionName") religionName = value;
        else if (key == "seed") seed = static_cast<uint32_t>(std::stoul(value));
        else if (key == "timeOfDay") timeOfDay = std::stof(value);
        else if (key == "daysPassed") daysPassed = std::stoi(value);
        else if (key == "advancementBonusYears") advancementBonusYears = std::stoi(value);
    }

    if (!hasExplicitStartYear) {
        startYear = currentYear - daysPassed - advancementBonusYears;
    }

    // -------------------------------------------------------------------------
    // EN: 2. Restore map terrain, cells, and craters from world.bin
    // ES: 2. Restaurar terreno, celdas y cráteres desde world.bin
    // -------------------------------------------------------------------------
    std::string mapPath = (saveDirectory / "world.bin").string();
    std::ifstream binaryMapFile(mapPath, std::ios::binary);
    if (!binaryMapFile.is_open()) return false;

    uint32_t magicHeader = 0;
    uint32_t fileVersion = 0;
    binaryMapFile.read(reinterpret_cast<char*>(&magicHeader), 4);
    binaryMapFile.read(reinterpret_cast<char*>(&fileVersion), 4);
    if (magicHeader != 0x53414954 || (fileVersion != 1 && fileVersion != 2 && fileVersion != 3 && fileVersion != 4)) {
        std::cerr << "[SaveSystem] Formato de archivo world.bin no válido." << std::endl;
        return false;
    }

    int readWidth = 0;
    int readHeight = 0;
    binaryMapFile.read(reinterpret_cast<char*>(&readWidth), sizeof(int));
    binaryMapFile.read(reinterpret_cast<char*>(&readHeight), sizeof(int));
    if (readWidth <= 0 || readHeight <= 0) {
        readWidth = mapWidthFromFile;
        readHeight = mapHeightFromFile;
    }

    WorldMap newMap(readWidth, readHeight);
    if (fileVersion == 1) {
        newMap.generateProceduralWorld(seed);
    }

    for (int cellY = 0; cellY < readHeight; ++cellY) {
        for (int cellX = 0; cellX < readWidth; ++cellX) {
            Cell& targetCell = newMap.getCell(cellX, cellY);
            targetCell.x = cellX;
            targetCell.y = cellY;
            binaryMapFile.read(reinterpret_cast<char*>(&targetCell.elevation), sizeof(float));
            uint8_t biomeByte = 0;
            binaryMapFile.read(reinterpret_cast<char*>(&biomeByte), 1);
            targetCell.biome = static_cast<BiomeType>(biomeByte);

            if (fileVersion >= 2) {
                uint8_t primaryClimateBiome = 0;
                uint8_t secondaryClimateBiome = 0;
                binaryMapFile.read(reinterpret_cast<char*>(&primaryClimateBiome), 1);
                binaryMapFile.read(reinterpret_cast<char*>(&secondaryClimateBiome), 1);
                targetCell.climate.primary_biome = static_cast<BiomeID>(primaryClimateBiome);
                targetCell.climate.secondary_biome = static_cast<BiomeID>(secondaryClimateBiome);
                binaryMapFile.read(reinterpret_cast<char*>(&targetCell.climate.blend_factor), sizeof(float));
                binaryMapFile.read(reinterpret_cast<char*>(&targetCell.climate.elevation), sizeof(float));
                binaryMapFile.read(reinterpret_cast<char*>(&targetCell.climate.temperature), sizeof(float));
                binaryMapFile.read(reinterpret_cast<char*>(&targetCell.climate.moisture), sizeof(float));
            }

            binaryMapFile.read(reinterpret_cast<char*>(&targetCell.population), sizeof(int));
            binaryMapFile.read(reinterpret_cast<char*>(&targetCell.maxPopulation), sizeof(int));
            binaryMapFile.read(reinterpret_cast<char*>(&targetCell.development), sizeof(float));
            binaryMapFile.read(reinterpret_cast<char*>(&targetCell.devotion), sizeof(float));
            binaryMapFile.read(reinterpret_cast<char*>(&targetCell.dominantReligionId), sizeof(int));
            binaryMapFile.read(reinterpret_cast<char*>(&targetCell.dominantFraction), sizeof(float));
            binaryMapFile.read(reinterpret_cast<char*>(&targetCell.shrineLevel), sizeof(int));
            targetCell.hasHolySite = (targetCell.shrineLevel > 0);
            if (targetCell.hasHolySite) {
                targetCell.holySiteReligionId = (targetCell.dominantReligionId > 0) ? targetCell.dominantReligionId : 1;
            }

            uint32_t shareCount = 0;
            binaryMapFile.read(reinterpret_cast<char*>(&shareCount), sizeof(uint32_t));
            targetCell.faithShare.clear();
            for (uint32_t shareIndex = 0; shareIndex < shareCount; ++shareIndex) {
                int religionId = 0;
                float shareFraction = 0.0f;
                binaryMapFile.read(reinterpret_cast<char*>(&religionId), sizeof(int));
                binaryMapFile.read(reinterpret_cast<char*>(&shareFraction), sizeof(float));
                targetCell.faithShare.push_back({religionId, shareFraction});
            }

            // EN: Restore version 3 terrain physics: craters, fissures, and natural resources
            // ES: Restaurar físicas de terreno versión 3: cráteres, fisuras y recursos naturales
            if (fileVersion >= 3) {
                uint8_t craterByte = 0;
                binaryMapFile.read(reinterpret_cast<char*>(&craterByte), 1);
                targetCell.hasImpactCrater = (craterByte == 1);
                binaryMapFile.read(reinterpret_cast<char*>(&targetCell.craterDepth), sizeof(float));
                uint8_t craterTypeByte = 0;
                binaryMapFile.read(reinterpret_cast<char*>(&craterTypeByte), 1);
                targetCell.craterType = static_cast<CraterType>(craterTypeByte);
                binaryMapFile.read(reinterpret_cast<char*>(&targetCell.fissureAngle), sizeof(float));
                uint8_t treesUprootedByte = 0;
                binaryMapFile.read(reinterpret_cast<char*>(&treesUprootedByte), 1);
                targetCell.hasTreesUprooted = (treesUprootedByte == 1);

                uint8_t resourceTypeByte = 0;
                binaryMapFile.read(reinterpret_cast<char*>(&resourceTypeByte), 1);
                targetCell.naturalResource = static_cast<ResourceType>(resourceTypeByte);
                binaryMapFile.read(reinterpret_cast<char*>(&targetCell.resourceAmount), sizeof(int));
                binaryMapFile.read(reinterpret_cast<char*>(&targetCell.maxResourceAmount), sizeof(int));
                if (fileVersion >= 4) {
                    uint8_t resourceVariantByte = 0;
                    binaryMapFile.read(reinterpret_cast<char*>(&resourceVariantByte), sizeof(uint8_t));
                    targetCell.resourceVariant = resourceVariantByte;
                } else {
                    targetCell.resourceVariant = 0;
                }
            }
        }
    }

    if (fileVersion < 3) {
        newMap.assignNaturalResources(seed);
    }

    // -------------------------------------------------------------------------
    // EN: 3. Restore religion registry and unlocked advancements
    // ES: 3. Restaurar registro de religiones y avances desbloqueados
    // -------------------------------------------------------------------------
    ReligionRegistry newRegistry;
    std::string religionPath = (saveDirectory / "religions.dat").string();
    std::ifstream religionFile(religionPath);
    if (religionFile.is_open()) {
        std::string religionLine;
        while (std::getline(religionFile, religionLine)) {
            if (religionLine.rfind("ID:", 0) != 0) continue;
            std::stringstream lineStream(religionLine.substr(3));
            std::string idString, nameString, founderString, symbolString, yearString, colorString;
            std::string zealString, cohesionString, theologyString, eraString, extinctString, advancementString;

            std::getline(lineStream, idString, '|');
            std::getline(lineStream, nameString, '|');
            std::getline(lineStream, founderString, '|');
            std::getline(lineStream, symbolString, '|');
            std::getline(lineStream, yearString, '|');
            std::getline(lineStream, colorString, '|');
            std::getline(lineStream, zealString, '|');
            std::getline(lineStream, cohesionString, '|');
            std::getline(lineStream, theologyString, '|');
            std::getline(lineStream, eraString, '|');
            std::getline(lineStream, extinctString, '|');
            std::getline(lineStream, advancementString, '|');

            int colorRed = 255, colorGreen = 200, colorBlue = 50;
            std::stringstream colorStream(colorString);
            std::string colorComponent;
            if (std::getline(colorStream, colorComponent, ',')) colorRed = std::stoi(colorComponent);
            if (std::getline(colorStream, colorComponent, ',')) colorGreen = std::stoi(colorComponent);
            if (std::getline(colorStream, colorComponent, ',')) colorBlue = std::stoi(colorComponent);

            int restoredRelId = newRegistry.createPrimordialReligion(
                nameString, founderString, ColorRGB(colorRed, colorGreen, colorBlue), symbolString, std::stoi(yearString)
            );
            Religion* religionPointer = newRegistry.getReligion(restoredRelId);
            if (religionPointer) {
                if (!zealString.empty()) religionPointer->zeal = std::stof(zealString);
                if (!cohesionString.empty()) religionPointer->cohesion = std::stof(cohesionString);
                if (!theologyString.empty()) religionPointer->theologyPoints = std::stoi(theologyString);
                if (!eraString.empty()) religionPointer->currentEra = static_cast<HistoricalEra>(std::stoi(eraString));
                if (!extinctString.empty()) religionPointer->isExtinct = (std::stoi(extinctString) == 1);
                if (!advancementString.empty()) {
                    std::stringstream advancementStream(advancementString);
                    std::string advancementIdString;
                    while (std::getline(advancementStream, advancementIdString, ',')) {
                        if (!advancementIdString.empty()) religionPointer->addAdvancement(std::stoi(advancementIdString));
                    }
                }
                std::string resourceString;
                if (std::getline(lineStream, resourceString, '|')) {
                    std::stringstream resourceStream(resourceString);
                    std::string foodStr, woodStr, stoneStr, goldStr;
                    if (std::getline(resourceStream, foodStr, ',')) religionPointer->resources.food = std::stoi(foodStr);
                    if (std::getline(resourceStream, woodStr, ',')) religionPointer->resources.wood = std::stoi(woodStr);
                    if (std::getline(resourceStream, stoneStr, ',')) religionPointer->resources.stone = std::stoi(stoneStr);
                    if (std::getline(resourceStream, goldStr, ',')) religionPointer->resources.gold = std::stoi(goldStr);
                }
            }
        }
    }

    // -------------------------------------------------------------------------
    // EN: 4. Restore tribal houses and settlement buildings
    // ES: 4. Restaurar viviendas y edificios tribales de los asentamientos
    // -------------------------------------------------------------------------
    game.getBuildingSystem().clear();
    std::string buildingPath = (saveDirectory / "buildings.dat").string();
    if (fs::exists(buildingPath)) {
        std::ifstream buildingFile(buildingPath);
        if (buildingFile.is_open()) {
            std::string buildingLine;
            while (std::getline(buildingFile, buildingLine)) {
                if (buildingLine.rfind("H:", 0) != 0) continue;
                std::stringstream stream(buildingLine.substr(2));
                std::string idStr, xStr, yStr, relStr, typeStr, stateStr, progStr, hpStr, maxHpStr, wdStr, wrStr, ownStr, partStr, capStr, resStr;
                std::getline(stream, idStr, '|');
                std::getline(stream, xStr, '|');
                std::getline(stream, yStr, '|');
                std::getline(stream, relStr, '|');
                std::getline(stream, typeStr, '|');
                std::getline(stream, stateStr, '|');
                std::getline(stream, progStr, '|');
                std::getline(stream, hpStr, '|');
                std::getline(stream, maxHpStr, '|');
                std::getline(stream, wdStr, '|');
                std::getline(stream, wrStr, '|');
                std::getline(stream, ownStr, '|');
                std::getline(stream, partStr, '|');
                std::getline(stream, capStr, '|');
                std::getline(stream, resStr, '|');

                House restoredHouse{};
                restoredHouse.id = std::stoi(idStr);
                restoredHouse.cellX = std::stoi(xStr);
                restoredHouse.cellY = std::stoi(yStr);
                restoredHouse.religionId = std::stoi(relStr);
                restoredHouse.type = static_cast<HouseType>(std::stoi(typeStr));
                restoredHouse.state = static_cast<BuildingState>(std::stoi(stateStr));
                restoredHouse.buildProgress = std::stof(progStr);
                restoredHouse.health = std::stof(hpStr);
                restoredHouse.maxHealth = std::stof(maxHpStr);
                restoredHouse.woodDelivered = std::stoi(wdStr);
                restoredHouse.woodRequired = std::stoi(wrStr);
                restoredHouse.ownerId = std::stoi(ownStr);
                restoredHouse.partnerId = std::stoi(partStr);
                restoredHouse.capacity = std::stoi(capStr);

                if (!resStr.empty()) {
                    std::stringstream residentsStream(resStr);
                    std::string residentIdString;
                    while (std::getline(residentsStream, residentIdString, ',')) {
                        if (!residentIdString.empty()) restoredHouse.residentIds.push_back(std::stoi(residentIdString));
                    }
                }

                game.getBuildingSystem().addHouseDirect(restoredHouse);
                if (newMap.isValid(restoredHouse.cellX, restoredHouse.cellY)) {
                    Cell& houseCell = newMap.getCell(restoredHouse.cellX, restoredHouse.cellY);
                    houseCell.hasHouse = true;
                    houseCell.houseId = restoredHouse.id;
                }
            }
        }
    }

    // -------------------------------------------------------------------------
    // EN: 5. Restore individual real villagers with 1:1 fidelity
    // ES: 5. Restaurar aldeanos reales individuales con fidelidad 1:1
    // -------------------------------------------------------------------------
    bool villagersLoadedFromSnapshot = false;
    std::string villagerPath = (saveDirectory / "villagers.dat").string();
    if (fs::exists(villagerPath)) {
        std::ifstream villagerFile(villagerPath);
        if (villagerFile.is_open()) {
            game.getGestorHumanos().getHumanos().clear();
            std::string villagerLine;
            while (std::getline(villagerFile, villagerLine)) {
                if (villagerLine.rfind("V:", 0) != 0) continue;
                std::stringstream stream(villagerLine.substr(2));
                std::string idStr, name, xStr, yStr, hpStr, ageStr, genStr, shaStr, proStr, relStr, devStr, childStr, spIdStr, spAffStr, fIdStr, mIdStr;
                std::getline(stream, idStr, '|');
                std::getline(stream, name, '|');
                std::getline(stream, xStr, '|');
                std::getline(stream, yStr, '|');
                std::getline(stream, hpStr, '|');
                std::getline(stream, ageStr, '|');
                std::getline(stream, genStr, '|');
                std::getline(stream, shaStr, '|');
                std::getline(stream, proStr, '|');
                std::getline(stream, relStr, '|');
                std::getline(stream, devStr, '|');
                std::getline(stream, childStr, '|');
                std::getline(stream, spIdStr, '|');
                std::getline(stream, spAffStr, '|');
                std::getline(stream, fIdStr, '|');
                std::getline(stream, mIdStr, '|');

                int villagerId = std::stoi(idStr);
                float posX = std::stof(xStr);
                float posY = std::stof(yStr);
                HumanGender gender = static_cast<HumanGender>(std::stoi(genStr));

                Humano restoredVillager(villagerId, name, posX, posY, gender);
                restoredVillager.setHealth(std::stof(hpStr));
                restoredVillager.setAge(std::stof(ageStr));
                restoredVillager.setEsChaman(std::stoi(shaStr) == 1);
                if (std::stoi(proStr) == 1) restoredVillager.setEsProfeta(true);
                restoredVillager.setReligionId(std::stoi(relStr));
                restoredVillager.setDevocion(std::stof(devStr));
                restoredVillager.setChild(std::stoi(childStr) == 1);
                restoredVillager.setSpouseId(std::stoi(spIdStr));
                restoredVillager.setSpouseAffection(std::stof(spAffStr));
                restoredVillager.setFatherId(std::stoi(fIdStr));
                restoredVillager.setMotherId(std::stoi(mIdStr));

                game.getGestorHumanos().addVillager(restoredVillager);
            }
            if (!game.getGestorHumanos().getHumanos().empty()) {
                villagersLoadedFromSnapshot = true;
            }
        }
    }

    // -------------------------------------------------------------------------
    // EN: 6. Restore history chronicle events
    // ES: 6. Restaurar eventos históricos de la crónica
    // -------------------------------------------------------------------------
    std::string eventPath = (saveDirectory / "events.dat").string();
    if (fs::exists(eventPath)) {
        std::ifstream eventFile(eventPath);
        if (eventFile.is_open()) {
            game.getEventLine().clear();
            std::string eventLine;
            while (std::getline(eventFile, eventLine)) {
                if (eventLine.rfind("E:", 0) != 0) continue;
                std::stringstream stream(eventLine.substr(2));
                std::string idStr, yearStr, catStr, title, desc, locStr, relAStr, relBStr, colStr, tpStr, actor, consequence;
                std::getline(stream, idStr, '|');
                std::getline(stream, yearStr, '|');
                std::getline(stream, catStr, '|');
                std::getline(stream, title, '|');
                std::getline(stream, desc, '|');
                std::getline(stream, locStr, '|');
                std::getline(stream, relAStr, '|');
                std::getline(stream, relBStr, '|');
                std::getline(stream, colStr, '|');
                std::getline(stream, tpStr, '|');
                std::getline(stream, actor, '|');
                std::getline(stream, consequence, '|');

                GameEvent restoredEvent{};
                restoredEvent.id = std::stoi(idStr);
                restoredEvent.year = std::stoi(yearStr);
                restoredEvent.category = static_cast<EventCategory>(std::stoi(catStr));
                restoredEvent.title = title;
                restoredEvent.description = desc;
                auto commaPos = locStr.find(',');
                if (commaPos != std::string::npos) {
                    restoredEvent.location.x = std::stoi(locStr.substr(0, commaPos));
                    restoredEvent.location.y = std::stoi(locStr.substr(commaPos + 1));
                }
                restoredEvent.religionAId = std::stoi(relAStr);
                restoredEvent.religionBId = std::stoi(relBStr);
                std::stringstream colorStream(colStr);
                std::string cr, cg, cb;
                if (std::getline(colorStream, cr, ',') && std::getline(colorStream, cg, ',') && std::getline(colorStream, cb, ',')) {
                    restoredEvent.badgeColor = ColorRGB(std::stoi(cr), std::stoi(cg), std::stoi(cb));
                }
                restoredEvent.isTurningPoint = (std::stoi(tpStr) == 1);
                restoredEvent.actorName = actor;
                restoredEvent.consequence = consequence;

                game.getEventLine().addEventDirect(restoredEvent);
            }
        }
    }

    // -------------------------------------------------------------------------
    // EN: 7. Restore religious morality profiles and commandments
    // ES: 7. Restaurar perfiles morales religiosos y mandatos
    // -------------------------------------------------------------------------
    std::string moralityPath = (saveDirectory / "morality.dat").string();
    if (fs::exists(moralityPath)) {
        std::ifstream moralityFile(moralityPath);
        if (moralityFile.is_open()) {
            std::stringstream bufferStream;
            bufferStream << moralityFile.rdbuf();
            game.getMoralitySystem().deserialize(bufferStream.str());
        }
    }

    // -------------------------------------------------------------------------
    // EN: 8. Inject restored state into Game instance and configure slot
    // ES: 8. Inyectar estado restaurado en la instancia de Game y configurar slot
    // -------------------------------------------------------------------------
    game.loadStateFromSave(newMap, newRegistry, currentYear, godName, religionName, seed,
                           villagersLoadedFromSnapshot, timeOfDay, worldFolderName,
                           daysPassed, advancementBonusYears, startYear);

    std::cout << "[SaveSystem] Partida '" << worldFolderName << "' cargada exitosamente." << std::endl;
    return true;
}

bool SaveSystem::deleteWorld(const std::string& worldFolderName) {
    std::string rootDirectory = getSavesRootDirectory();
    fs::path saveDirectory = fs::path(rootDirectory) / worldFolderName;
    if (fs::exists(saveDirectory)) {
        std::error_code errorCode;
        fs::remove_all(saveDirectory, errorCode);
        return !errorCode;
    }
    return false;
}

} // namespace ReligionSim