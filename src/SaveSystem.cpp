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

std::string SaveSystem::getSavesRootDirectory() {
    std::vector<std::string> candidates = {
        "saves",
        "religion_simulator_isometric/saves",
        "/home/Ivancho/Documentos/religion_simulator_isometric/saves"
    };
    for (const auto& path : candidates) {
        if (fs::exists(path) && fs::is_directory(path)) {
            return path;
        }
    }
    // Si no existe, crear la local por defecto
    try {
        fs::create_directories("saves");
        return "saves";
    } catch (...) {
        return "saves";
    }
}

std::string SaveSystem::formatTimestamp(int64_t epochMs) {
    if (epochMs <= 0) return "Desconocida";
    std::time_t t = static_cast<std::time_t>(epochMs / 1000);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    std::ostringstream ss;
    ss << std::put_time(&tm, "%d/%m/%Y %H:%M");
    return ss.str();
}

std::string SaveSystem::formatSummary(const WorldSaveMeta& meta) {
    std::ostringstream ss;
    ss << "Era: " << AdvancementSystem::getEraName(meta.era)
       << " | " << Timeline::formatYear(meta.currentYear)
       << " | " << meta.totalPopulation << " aldeanos";
    return ss.str();
}

std::vector<WorldSaveMeta> SaveSystem::scanSaveSlots() {
    std::vector<WorldSaveMeta> result;
    std::string root = getSavesRootDirectory();

    if (!fs::exists(root) || !fs::is_directory(root)) {
        return result;
    }

    for (const auto& entry : fs::directory_iterator(root)) {
        if (!entry.is_directory()) continue;

        std::string folderName = entry.path().filename().string();
        std::string metaPath = (entry.path() / "level.dat").string();
        std::string iconPath = (entry.path() / "icon.ppm").string();

        if (fs::exists(metaPath)) {
            WorldSaveMeta meta{};
            meta.folderName = folderName;
            meta.iconPPMPath = iconPath;
            meta.hasIcon = fs::exists(iconPath);

            std::ifstream file(metaPath);
            if (file.is_open()) {
                std::string line;
                while (std::getline(file, line)) {
                    auto pos = line.find('=');
                    if (pos == std::string::npos) continue;
                    std::string key = line.substr(0, pos);
                    std::string val = line.substr(pos + 1);

                    if (key == "worldName") meta.worldName = val;
                    else if (key == "godName") meta.godName = val;
                    else if (key == "religionName") meta.religionName = val;
                    else if (key == "seed") meta.seed = static_cast<uint32_t>(std::stoul(val));
                    else if (key == "currentYear") meta.currentYear = std::stoi(val);
                    else if (key == "era") meta.era = static_cast<HistoricalEra>(std::stoi(val));
                    else if (key == "population") {
                        meta.totalPopulation = std::stoi(val);
                        if (meta.totalPopulation > 250) meta.totalPopulation = 80;
                    }
                    else if (key == "activeReligions") meta.activeReligions = std::stoi(val);
                    else if (key == "lastPlayed") meta.lastPlayedTimestamp = std::stoll(val);
                }
            }

            if (meta.worldName.empty()) {
                meta.worldName = folderName;
            }
            result.push_back(meta);
        }
    }

    // Ordenar de más reciente a más antiguo (estilo Minecraft)
    std::sort(result.begin(), result.end(), [](const WorldSaveMeta& a, const WorldSaveMeta& b) {
        return a.lastPlayedTimestamp > b.lastPlayedTimestamp;
    });

    return result;
}

bool SaveSystem::saveWorld(Game& game, const std::string& worldFolderName) {
    std::string root = getSavesRootDirectory();
    fs::path saveDir = fs::path(root) / worldFolderName;

    try {
        fs::create_directories(saveDir);
    } catch (const std::exception& e) {
        std::cerr << "[SaveSystem] Error creando carpeta: " << e.what() << std::endl;
        return false;
    }

    auto nowEpoch = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();

    const auto& map = game.getMap();
    const auto& registry = game.getRegistry();
    const auto& timeline = game.getTimeline();
    const auto& config = game.getCreationConfig();

    HistoricalEra playerEra = HistoricalEra::PrimordialAnimism;
    const Religion* pRel = registry.getReligion(1);
    if (pRel) {
        playerEra = pRel->currentEra;
    }

    // 1. Guardar metadatos en level.dat
    std::string levelPath = (saveDir / "level.dat").string();
    {
        std::ofstream metaFile(levelPath);
        if (!metaFile.is_open()) return false;
        metaFile << "worldName=" << (config.godName.empty() ? worldFolderName : "Mundo de " + config.godName) << "\n";
        metaFile << "godName=" << config.godName << "\n";
        metaFile << "religionName=" << config.religionName << "\n";
        metaFile << "seed=" << config.mapSeed << "\n";
        metaFile << "currentYear=" << timeline.getCurrentYear() << "\n";
        metaFile << "era=" << static_cast<int>(playerEra) << "\n";
        metaFile << "population=" << map.getTotalWorldPopulation() << "\n";
        metaFile << "activeReligions=" << registry.getAllReligions().size() << "\n";
        metaFile << "lastPlayed=" << nowEpoch << "\n";
        metaFile << "mapWidth=" << map.getWidth() << "\n";
        metaFile << "mapHeight=" << map.getHeight() << "\n";
    }

    // 2. Guardar estado del mapa y celdas en world.bin
    std::string mapPath = (saveDir / "world.bin").string();
    {
        std::ofstream binFile(mapPath, std::ios::binary);
        if (!binFile.is_open()) return false;

        uint32_t magic = 0x53414954; // 'FAIS'
        uint32_t version = 2;
        binFile.write(reinterpret_cast<const char*>(&magic), 4);
        binFile.write(reinterpret_cast<const char*>(&version), 4);

        int w = map.getWidth();
        int h = map.getHeight();
        binFile.write(reinterpret_cast<const char*>(&w), sizeof(int));
        binFile.write(reinterpret_cast<const char*>(&h), sizeof(int));

        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                const Cell& c = map.getCell(x, y);
                binFile.write(reinterpret_cast<const char*>(&c.elevation), sizeof(float));
                uint8_t biomeByte = static_cast<uint8_t>(c.biome);
                binFile.write(reinterpret_cast<const char*>(&biomeByte), 1);
                uint8_t primaryClimateBiome = static_cast<uint8_t>(c.climate.primary_biome);
                uint8_t secondaryClimateBiome = static_cast<uint8_t>(c.climate.secondary_biome);
                binFile.write(reinterpret_cast<const char*>(&primaryClimateBiome), 1);
                binFile.write(reinterpret_cast<const char*>(&secondaryClimateBiome), 1);
                binFile.write(reinterpret_cast<const char*>(&c.climate.blend_factor), sizeof(float));
                binFile.write(reinterpret_cast<const char*>(&c.climate.elevation), sizeof(float));
                binFile.write(reinterpret_cast<const char*>(&c.climate.temperature), sizeof(float));
                binFile.write(reinterpret_cast<const char*>(&c.climate.moisture), sizeof(float));
                binFile.write(reinterpret_cast<const char*>(&c.population), sizeof(int));
                binFile.write(reinterpret_cast<const char*>(&c.maxPopulation), sizeof(int));
                binFile.write(reinterpret_cast<const char*>(&c.development), sizeof(float));
                binFile.write(reinterpret_cast<const char*>(&c.devotion), sizeof(float));
                binFile.write(reinterpret_cast<const char*>(&c.dominantReligionId), sizeof(int));
                binFile.write(reinterpret_cast<const char*>(&c.dominantFraction), sizeof(float));
                binFile.write(reinterpret_cast<const char*>(&c.shrineLevel), sizeof(int));

                uint32_t shareCount = static_cast<uint32_t>(c.faithShare.size());
                binFile.write(reinterpret_cast<const char*>(&shareCount), sizeof(uint32_t));
                for (const auto& share : c.faithShare) {
                    binFile.write(reinterpret_cast<const char*>(&share.first), sizeof(int));
                    binFile.write(reinterpret_cast<const char*>(&share.second), sizeof(float));
                }
            }
        }
    }

    // 3. Guardar religiones y doctrinas en religions.dat
    std::string relPath = (saveDir / "religions.dat").string();
    {
        std::ofstream relFile(relPath);
        if (relFile.is_open()) {
            for (const auto& r : registry.getAllReligions()) {
                relFile << "ID:" << r.id << "|"
                        << r.name << "|"
                        << r.founderName << "|"
                        << r.symbol << "|"
                        << r.foundedYear << "|"
                        << static_cast<int>(r.color.r) << ","
                        << static_cast<int>(r.color.g) << ","
                        << static_cast<int>(r.color.b) << "|"
                        << r.zeal << "|"
                        << r.cohesion << "|"
                        << r.theologyPoints << "|"
                        << static_cast<int>(r.currentEra) << "|"
                        << (r.isExtinct ? 1 : 0) << "\n";
            }
        }
    }

    // 4. Capturar Snapshot estilo Minecraft: icon.ppm (cuadrado o thumbnail del mundo)
    std::string iconPath = (saveDir / "icon.ppm").string();
    game.getRenderer().exportFrameToPPM(iconPath);

    std::cout << "[SaveSystem] Partida guardada exitosamente en: " << saveDir << std::endl;
    return true;
}

bool SaveSystem::loadWorld(Game& game, const std::string& worldFolderName) {
    std::string root = getSavesRootDirectory();
    fs::path saveDir = fs::path(root) / worldFolderName;

    if (!fs::exists(saveDir)) {
        std::cerr << "[SaveSystem] No existe la carpeta del mundo: " << saveDir << std::endl;
        return false;
    }

    // 1. Leer level.dat
    std::string levelPath = (saveDir / "level.dat").string();
    std::ifstream metaFile(levelPath);
    if (!metaFile.is_open()) return false;

    int currentYear = -4000;
    int mapW = 48, mapH = 48;
    (void)mapW; (void)mapH;
    std::string godName, religionName;
    uint32_t seed = 42;

    std::string line;
    while (std::getline(metaFile, line)) {
        auto pos = line.find('=');
        if (pos == std::string::npos) continue;
        std::string k = line.substr(0, pos);
        std::string v = line.substr(pos + 1);
        if (k == "currentYear") currentYear = std::stoi(v);
        else if (k == "mapWidth") mapW = std::stoi(v);
        else if (k == "mapHeight") mapH = std::stoi(v);
        else if (k == "godName") godName = v;
        else if (k == "religionName") religionName = v;
        else if (k == "seed") seed = static_cast<uint32_t>(std::stoul(v));
    }

    // 2. Restaurar mapa
    std::string mapPath = (saveDir / "world.bin").string();
    std::ifstream binFile(mapPath, std::ios::binary);
    if (!binFile.is_open()) return false;

    uint32_t magic = 0, version = 0;
    binFile.read(reinterpret_cast<char*>(&magic), 4);
    binFile.read(reinterpret_cast<char*>(&version), 4);
    if (magic != 0x53414954 || (version != 1 && version != 2)) {
        std::cerr << "[SaveSystem] Formato de archivo world.bin no válido." << std::endl;
        return false;
    }

    int readW = 0, readH = 0;
    binFile.read(reinterpret_cast<char*>(&readW), sizeof(int));
    binFile.read(reinterpret_cast<char*>(&readH), sizeof(int));

    WorldMap newMap(readW, readH);
    if (version == 1) {
        newMap.generateProceduralWorld(seed);
    }
    for (int y = 0; y < readH; ++y) {
        for (int x = 0; x < readW; ++x) {
            Cell& c = newMap.getCell(x, y);
            c.x = x;
            c.y = y;
            binFile.read(reinterpret_cast<char*>(&c.elevation), sizeof(float));
            uint8_t biomeByte = 0;
            binFile.read(reinterpret_cast<char*>(&biomeByte), 1);
            c.biome = static_cast<BiomeType>(biomeByte);
            if (version >= 2) {
                uint8_t primaryClimateBiome = 0;
                uint8_t secondaryClimateBiome = 0;
                binFile.read(reinterpret_cast<char*>(&primaryClimateBiome), 1);
                binFile.read(reinterpret_cast<char*>(&secondaryClimateBiome), 1);
                c.climate.primary_biome = static_cast<BiomeID>(primaryClimateBiome);
                c.climate.secondary_biome = static_cast<BiomeID>(secondaryClimateBiome);
                binFile.read(reinterpret_cast<char*>(&c.climate.blend_factor), sizeof(float));
                binFile.read(reinterpret_cast<char*>(&c.climate.elevation), sizeof(float));
                binFile.read(reinterpret_cast<char*>(&c.climate.temperature), sizeof(float));
                binFile.read(reinterpret_cast<char*>(&c.climate.moisture), sizeof(float));
            }
            binFile.read(reinterpret_cast<char*>(&c.population), sizeof(int));
            binFile.read(reinterpret_cast<char*>(&c.maxPopulation), sizeof(int));
            binFile.read(reinterpret_cast<char*>(&c.development), sizeof(float));
            binFile.read(reinterpret_cast<char*>(&c.devotion), sizeof(float));
            binFile.read(reinterpret_cast<char*>(&c.dominantReligionId), sizeof(int));
            binFile.read(reinterpret_cast<char*>(&c.dominantFraction), sizeof(float));
            binFile.read(reinterpret_cast<char*>(&c.shrineLevel), sizeof(int));
            c.hasHolySite = (c.shrineLevel > 0);
            if (c.hasHolySite) {
                c.holySiteReligionId = (c.dominantReligionId > 0) ? c.dominantReligionId : 1;
            }

            uint32_t shareCount = 0;
            binFile.read(reinterpret_cast<char*>(&shareCount), sizeof(uint32_t));
            c.faithShare.clear();
            for (uint32_t s = 0; s < shareCount; ++s) {
                int rId = 0;
                float shareF = 0.0f;
                binFile.read(reinterpret_cast<char*>(&rId), sizeof(int));
                binFile.read(reinterpret_cast<char*>(&shareF), sizeof(float));
                c.faithShare.push_back({rId, shareF});
            }
        }
    }

    // 3. Restaurar religiones
    ReligionRegistry newRegistry;
    std::string relPath = (saveDir / "religions.dat").string();
    std::ifstream relFile(relPath);
    if (relFile.is_open()) {
        std::string rline;
        while (std::getline(relFile, rline)) {
            if (rline.rfind("ID:", 0) != 0) continue;
            std::stringstream ss(rline.substr(3));
            std::string idStr, name, founder, symbol, yearStr, colStr, zealStr, cohStr, theolStr, eraStr, extStr;

            std::getline(ss, idStr, '|');
            std::getline(ss, name, '|');
            std::getline(ss, founder, '|');
            std::getline(ss, symbol, '|');
            std::getline(ss, yearStr, '|');
            std::getline(ss, colStr, '|');
            std::getline(ss, zealStr, '|');
            std::getline(ss, cohStr, '|');
            std::getline(ss, theolStr, '|');
            std::getline(ss, eraStr, '|');
            std::getline(ss, extStr, '|');

            int r = 255, g = 200, b = 50;
            std::stringstream css(colStr);
            std::string cs;
            if (std::getline(css, cs, ',')) r = std::stoi(cs);
            if (std::getline(css, cs, ',')) g = std::stoi(cs);
            if (std::getline(css, cs, ',')) b = std::stoi(cs);

            int relId = newRegistry.createPrimordialReligion(name, founder, ColorRGB(r, g, b), symbol, std::stoi(yearStr));
            Religion* rel = newRegistry.getReligion(relId);
            if (rel) {
                if (!zealStr.empty()) rel->zeal = std::stof(zealStr);
                if (!cohStr.empty()) rel->cohesion = std::stof(cohStr);
                if (!theolStr.empty()) rel->theologyPoints = std::stoi(theolStr);
                if (!eraStr.empty()) rel->currentEra = static_cast<HistoricalEra>(std::stoi(eraStr));
                if (!extStr.empty()) rel->isExtinct = (std::stoi(extStr) == 1);
            }
        }
    }

    // 4. Inyectar en Game
    game.loadStateFromSave(newMap, newRegistry, currentYear, godName, religionName, seed);

    std::cout << "[SaveSystem] Partida '" << worldFolderName << "' cargada exitosamente." << std::endl;
    return true;
}

bool SaveSystem::deleteWorld(const std::string& worldFolderName) {
    std::string root = getSavesRootDirectory();
    fs::path saveDir = fs::path(root) / worldFolderName;
    if (fs::exists(saveDir)) {
        std::error_code ec;
        fs::remove_all(saveDir, ec);
        return !ec;
    }
    return false;
}

} // namespace ReligionSim
