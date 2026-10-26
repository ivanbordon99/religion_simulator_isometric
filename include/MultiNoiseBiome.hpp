#pragma once

#include <array>
#include <cstdint>

namespace ReligionSim {

enum class BiomeID : std::uint8_t {
    Ocean,
    Coast,
    Desert,
    Savanna,
    Plains,
    TemperateForest,
    Taiga,
    TropicalRainforest,
    SnowyMountain,
    RockyPeak,
    Count
};

struct BiomeTarget {
    BiomeID id;
    float target_temperature;
    float target_moisture;
    float target_elevation;
    float weight_t;
    float weight_m;
    float weight_e;
};

struct TileBiomeData {
    BiomeID primary_biome{BiomeID::Plains};
    BiomeID secondary_biome{BiomeID::Plains};
    float blend_factor{0.0f};
    float elevation{0.0f};
    float temperature{0.0f};
    float moisture{0.0f};
};

class MultiNoiseBiomeGenerator {
public:
    static constexpr float COOLING_FACTOR = 0.42f;
    static constexpr std::array<BiomeTarget, 10> BIOME_TARGETS{{
        {BiomeID::Ocean,             0.05f,  0.00f, -0.78f, 0.45f, 0.45f, 3.00f},
        {BiomeID::Coast,             0.15f, -0.10f, -0.34f, 0.45f, 0.65f, 2.40f},
        {BiomeID::Desert,            0.72f, -0.78f, -0.04f, 1.35f, 1.45f, 0.55f},
        {BiomeID::Savanna,           0.50f, -0.38f,  0.02f, 0.95f, 0.95f, 0.55f},
        {BiomeID::Plains,            0.15f,  0.02f,  0.06f, 0.85f, 0.85f, 0.50f},
        {BiomeID::TemperateForest,   0.12f,  0.56f,  0.12f, 0.85f, 0.95f, 0.55f},
        {BiomeID::Taiga,            -0.55f,  0.22f,  0.20f, 1.10f, 0.75f, 0.65f},
        {BiomeID::TropicalRainforest,0.78f,  0.82f,  0.00f, 1.15f, 1.10f, 0.50f},
        {BiomeID::SnowyMountain,    -0.82f,  0.20f,  0.82f, 1.10f, 0.65f, 1.55f},
        {BiomeID::RockyPeak,        -0.18f, -0.08f,  0.90f, 0.50f, 0.45f, 1.65f}
    }};

    static TileBiomeData sample(int x, int y, int width, int height, std::uint32_t seed);
    static TileBiomeData selectBiome(float temperature, float moisture, float elevation);
    static float applyOrographicCooling(float temperature_base, float elevation);
};

float sample_noise(float x, float y, float frequency, int seed);

} // namespace ReligionSim
