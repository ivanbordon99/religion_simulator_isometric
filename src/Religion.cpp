#include "Religion.hpp"
#include <random>
#include <array>
#include <sstream>
#include <algorithm>

namespace ReligionSim {

namespace {
    std::mt19937 g_rng(1337);

    const std::vector<std::string> PROPHET_PREFIXES = {
        "Zara", "Mani", "Akhen", "Siddha", "El", "Mel", "Amun", "Ishtar",
        "Orph", "Mith", "Aethel", "Kuan", "Theoph", "Hiero", "Brahma"
    };

    const std::vector<std::string> PROPHET_SUFFIXES = {
        "thustra", "khep", "aten", "artha", "shaddai", "chizedek", "ra", "danna",
        "eus", "ras", "gard", "yin", "ilus", "phant", "nanda"
    };

    const std::vector<std::string> THEO_PREFIXES = {
        "Sol", "Aura", "Terra", "Cosmo", "Theo", "Pneuma", "Aether",
        "Lumen", "Veritas", "Dharma", "Tao", "Logos", "Sanctum", "Aegis"
    };

    const std::vector<std::string> THEO_ROOTS = {
        "ism", "ianity", "ology", "cracy", "covenant", "order", "creed",
        "gnosis", "pantheon", "path", "circle", "communion"
    };

    const std::vector<std::string> SYMBOLS = {
        "Sun", "Moon", "Cross", "Spiral", "Wheel", "Lotus", "Flame",
        "Eye", "Tree", "Crescent", "Star", "Chalice", "Triquetra"
    };
}

ReligionRegistry::ReligionRegistry() {
    m_religions.reserve(64);
}

int ReligionRegistry::createPrimordialReligion(const std::string& name, const std::string& founder,
                                              ColorRGB color, const std::string& symbol, int year) {
    Religion r;
    r.id = m_nextId++;
    r.name = name;
    r.founderName = founder;
    r.symbol = symbol;
    r.foundedYear = year;
    r.color = color;
    r.zeal = 0.55f;
    r.syncretismTolerance = 0.45f;
    r.cohesion = 0.70f;
    r.spiritualPiety = 0.60f;
    r.scholasticism = 0.15f;
    r.currentEra = HistoricalEra::PrimordialAnimism;
    r.advancementLevel = 1;
    r.addDoctrine(DoctrineType::AnimistSpirits);

    std::string lowerName = name;
    for (char& c : lowerName) c = static_cast<char>(std::tolower(c));
    std::string lowerSym = symbol;
    for (char& c : lowerSym) c = static_cast<char>(std::tolower(c));

    if (lowerSym.find("sun") != std::string::npos || lowerName.find("sol") != std::string::npos ||
        lowerName.find("cielo") != std::string::npos || lowerName.find("fuego") != std::string::npos) {
        r.archetype = TheologicalArchetype::SolarCelestial;
        r.zeal = 0.65f;
    } else if (lowerSym.find("earth") != std::string::npos || lowerName.find("tierra") != std::string::npos ||
               lowerName.find("gaia") != std::string::npos || lowerName.find("sombra") != std::string::npos ||
               lowerName.find("ancestro") != std::string::npos) {
        r.archetype = TheologicalArchetype::ChthonicAncestral;
        r.cohesion = 0.80f;
    } else if (lowerName.find("monad") != std::string::npos || lowerName.find("orden") != std::string::npos ||
               lowerName.find("eterno") != std::string::npos) {
        r.archetype = TheologicalArchetype::DogmaticExclusivist;
        r.cohesion = 0.85f;
        r.syncretismTolerance = 0.20f;
    } else if (lowerName.find("star") != std::string::npos || lowerName.find("dharma") != std::string::npos ||
               lowerName.find("panteon") != std::string::npos) {
        r.archetype = TheologicalArchetype::PolytheisticPantheon;
        r.syncretismTolerance = 0.70f;
    } else {
        r.archetype = TheologicalArchetype::TotemicAnimism;
    }

    m_religions.push_back(r);
    return r.id;
}

int ReligionRegistry::createSyncreticReligion(int parentAId, int parentBId, int year, const Point2i& origin) {
    (void)origin;
    Religion* a = getReligion(parentAId);
    Religion* b = getReligion(parentBId);
    if (!a || !b) return 0;

    Religion r;
    r.id = m_nextId++;
    r.syncreticParentA = parentAId;
    r.syncreticParentB = parentBId;
    r.foundedYear = year;

    // Blended Name
    std::string prefix = a->name.substr(0, std::min(a->name.size(), size_t(5)));
    std::string suffix = b->name.substr(b->name.size() > 4 ? b->name.size() - 4 : 0);
    r.name = prefix + "-" + b->name.substr(0, std::min(b->name.size(), size_t(6))) + " Syncretism";
    r.founderName = "Council of " + a->founderName + " & " + b->founderName;
    r.symbol = a->symbol + "+" + b->symbol;

    // Blended Color with high saturation
    r.color = ColorRGB::blend(a->color, b->color, 0.5f);
    // slightly brighten syncretic faith
    r.color.r = static_cast<uint8_t>(std::min(255, r.color.r + 25));
    r.color.g = static_cast<uint8_t>(std::min(255, r.color.g + 25));
    r.color.b = static_cast<uint8_t>(std::min(255, r.color.b + 25));

    // Blended attributes
    r.zeal = (a->zeal + b->zeal) * 0.5f + 0.1f; // renewed vitality
    r.syncretismTolerance = std::max(a->syncretismTolerance, b->syncretismTolerance) * 0.9f + 0.1f;
    r.cohesion = 0.65f;
    r.spiritualPiety = (a->spiritualPiety + b->spiritualPiety) * 0.5f;
    r.scholasticism = std::max(a->scholasticism, b->scholasticism);
    r.advancementLevel = std::max(a->advancementLevel, b->advancementLevel);
    r.currentEra = (a->advancementLevel >= b->advancementLevel) ? a->currentEra : b->currentEra;

    // Inherit doctrines & syncretic openness
    for (auto d : a->doctrines) r.addDoctrine(d);
    for (auto d : b->doctrines) r.addDoctrine(d);
    r.addDoctrine(DoctrineType::SyncreticOpenness);
    r.archetype = TheologicalArchetype::PolytheisticPantheon;

    m_religions.push_back(r);
    return r.id;
}

int ReligionRegistry::createSchismReligion(int parentId, int year, const Point2i& origin) {
    (void)origin;
    Religion* p = getReligion(parentId);
    if (!p) return 0;

    p->schismCount++;

    Religion r;
    r.id = m_nextId++;
    r.parentReligionId = parentId;
    r.foundedYear = year;
    r.founderName = generateProphetName();
    r.symbol = p->symbol;

    static const std::vector<std::string> SCHISM_DESCRIPTORS = {
        "Reformed ", "Orthodox ", "Pure ", "Covenant of ", "Mystic ", "True "
    };
    std::uniform_int_distribution<size_t> descDist(0, SCHISM_DESCRIPTORS.size() - 1);
    r.name = SCHISM_DESCRIPTORS[descDist(g_rng)] + p->name;

    // Hue shift for schism color
    r.color = p->color;
    std::uniform_int_distribution<int> colorShift(-45, 45);
    r.color.r = static_cast<uint8_t>(std::clamp(static_cast<int>(r.color.r) + colorShift(g_rng), 30, 245));
    r.color.g = static_cast<uint8_t>(std::clamp(static_cast<int>(r.color.g) + colorShift(g_rng), 30, 245));
    r.color.b = static_cast<uint8_t>(std::clamp(static_cast<int>(r.color.b) + colorShift(g_rng), 30, 245));

    // Schisms often have high zeal and strong dogmatic fervor
    r.zeal = std::min(0.95f, p->zeal + 0.25f);
    r.syncretismTolerance = std::max(0.1f, p->syncretismTolerance - 0.2f);
    r.cohesion = 0.80f;
    r.spiritualPiety = std::min(0.95f, p->spiritualPiety + 0.2f);
    r.scholasticism = p->scholasticism;
    r.advancementLevel = p->advancementLevel;
    r.currentEra = p->currentEra;
    r.archetype = TheologicalArchetype::DogmaticExclusivist;

    for (auto d : p->doctrines) r.addDoctrine(d);
    r.addDoctrine(DoctrineType::ProselytizingZeal);

    m_religions.push_back(r);
    return r.id;
}

Religion* ReligionRegistry::getReligion(int id) {
    for (auto& r : m_religions) {
        if (r.id == id) return &r;
    }
    return nullptr;
}

const Religion* ReligionRegistry::getReligion(int id) const {
    for (const auto& r : m_religions) {
        if (r.id == id) return &r;
    }
    return nullptr;
}

void ReligionRegistry::updateStats(int totalWorldPop) {
    for (auto& r : m_religions) {
        if (r.totalFollowers > r.peakFollowers) {
            r.peakFollowers = r.totalFollowers;
        }
        if (r.totalFollowers == 0 && r.peakFollowers > 100) {
            r.isExtinct = true;
        } else if (r.totalFollowers > 0) {
            r.isExtinct = false;
        }
    }
    (void)totalWorldPop;
}

std::string ReligionRegistry::generateName(const std::string& prefix, const std::string& suffix) {
    return prefix + suffix;
}

std::string ReligionRegistry::generateProphetName() {
    std::uniform_int_distribution<size_t> pDist(0, PROPHET_PREFIXES.size() - 1);
    std::uniform_int_distribution<size_t> sDist(0, PROPHET_SUFFIXES.size() - 1);
    return PROPHET_PREFIXES[pDist(g_rng)] + PROPHET_SUFFIXES[sDist(g_rng)];
}

ColorRGB ReligionRegistry::generateDistinctColor(size_t index) {
    static const ColorRGB PALETTE[] = {
        {230, 57, 70},   // Crimson / Red
        {244, 162, 97},  // Solar Gold
        {42, 157, 143},  // Persian Teal
        {233, 196, 106}, // Amber Saffron
        {38, 70, 83},    // Deep Celestial Indigo
        {155, 89, 182},  // Royal Amethyst
        {46, 204, 113},  // Jade Emerald
        {52, 152, 219},  // Azure Sky
        {231, 76, 60},   // Vermilion
        {241, 196, 15},  // Golden Lotus
        {26, 188, 156},  // Turquoise
        {142, 68, 173},  // Deep Violet
        {211, 84, 0},    // Ochre Orange
        {189, 195, 199}, // Silver Monolith
        {52, 73, 94}     // Iron Slate
    };
    constexpr size_t count = sizeof(PALETTE) / sizeof(PALETTE[0]);
    return PALETTE[index % count];
}

} // namespace ReligionSim
