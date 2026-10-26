#include "MultiNoiseBiome.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace ReligionSim {

namespace {

float latticeValue(int x, int y, int seed) {
    std::uint32_t hash = static_cast<std::uint32_t>(seed);
    hash ^= static_cast<std::uint32_t>(x) * 0x9E3779B9u;
    hash ^= static_cast<std::uint32_t>(y) * 0x85EBCA6Bu;
    hash ^= hash >> 16;
    hash *= 0x7FEB352Du;
    hash ^= hash >> 15;
    hash *= 0x846CA68Bu;
    hash ^= hash >> 16;
    return static_cast<float>(hash & 0x00FFFFFFu) / 0x007FFFFFu - 1.0f;
}

float smoothLerp(float a, float b, float t) {
    const float smooth = t * t * (3.0f - 2.0f * t);
    return a + (b - a) * smooth;
}

float fractalNoise(float x, float y, std::uint32_t seed) {
    return std::clamp(
        sample_noise(x, y, 2.4f, static_cast<int>(seed)) * 0.62f +
        sample_noise(x, y, 5.2f, static_cast<int>(seed + 1u)) * 0.28f +
        sample_noise(x, y, 11.0f, static_cast<int>(seed + 2u)) * 0.10f,
        -1.0f, 1.0f);
}

} // namespace

float sample_noise(float x, float y, float frequency, int seed) {
    const float sampleX = x * frequency;
    const float sampleY = y * frequency;
    const int x0 = static_cast<int>(std::floor(sampleX));
    const int y0 = static_cast<int>(std::floor(sampleY));
    const float fx = sampleX - static_cast<float>(x0);
    const float fy = sampleY - static_cast<float>(y0);

    const float top = smoothLerp(
        latticeValue(x0, y0, seed), latticeValue(x0 + 1, y0, seed), fx);
    const float bottom = smoothLerp(
        latticeValue(x0, y0 + 1, seed), latticeValue(x0 + 1, y0 + 1, seed), fx);
    return std::clamp(smoothLerp(top, bottom, fy), -1.0f, 1.0f);
}

TileBiomeData MultiNoiseBiomeGenerator::sample(int x, int y, int width, int height,
                                               std::uint32_t seed) {
    const float nx = static_cast<float>(x) / static_cast<float>(std::max(1, width - 1)) * 2.0f - 1.0f;
    const float ny = static_cast<float>(y) / static_cast<float>(std::max(1, height - 1)) * 2.0f - 1.0f;
    const float distanceFromCenter = std::sqrt(nx * nx + ny * ny);

    const float elevationSignal = fractalNoise(nx, ny, seed);
    const float legacyHeight = elevationSignal * 0.72f + 0.62f - distanceFromCenter * 0.48f;
    const float elevation = std::clamp(legacyHeight * 2.0f - 1.0f, -1.0f, 1.0f);

    const float moistureBase =
        (sample_noise(nx, ny, 2.8f, static_cast<int>(seed + 3u)) * 0.65f +
         sample_noise(nx, ny, 6.0f, static_cast<int>(seed + 4u)) * 0.35f);
    const float moisture = std::clamp(moistureBase, -1.0f, 1.0f);
    const float temperatureBase = std::clamp(
        1.0f - 2.0f * std::abs(ny) +
        sample_noise(nx, ny, 3.4f, static_cast<int>(seed + 5u)) * 0.24f,
        -1.0f, 1.0f);
    const float temperature = applyOrographicCooling(temperatureBase, elevation);

    TileBiomeData result = selectBiome(temperature, moisture, elevation);
    result.elevation = elevation;
    result.temperature = temperature;
    result.moisture = moisture;
    return result;
}

float MultiNoiseBiomeGenerator::applyOrographicCooling(float temperature_base, float elevation) {
    return std::clamp(temperature_base - COOLING_FACTOR * elevation, -1.0f, 1.0f);
}

TileBiomeData MultiNoiseBiomeGenerator::selectBiome(float temperature, float moisture,
                                                     float elevation) {
    temperature = std::clamp(temperature, -1.0f, 1.0f);
    moisture = std::clamp(moisture, -1.0f, 1.0f);
    elevation = std::clamp(elevation, -1.0f, 1.0f);

    float bestDistance = std::numeric_limits<float>::infinity();
    float secondDistance = std::numeric_limits<float>::infinity();
    BiomeID bestBiome = BiomeID::Plains;
    BiomeID secondBiome = BiomeID::Plains;

    for (const BiomeTarget& target : BIOME_TARGETS) {
        const float dt = temperature - target.target_temperature;
        const float dm = moisture - target.target_moisture;
        const float de = elevation - target.target_elevation;
        const float distance = target.weight_t * dt * dt +
                               target.weight_m * dm * dm +
                               target.weight_e * de * de;
        if (distance < bestDistance) {
            secondDistance = bestDistance;
            secondBiome = bestBiome;
            bestDistance = distance;
            bestBiome = target.id;
        } else if (distance < secondDistance) {
            secondDistance = distance;
            secondBiome = target.id;
        }
    }

    const float primaryDistance = std::sqrt(bestDistance);
    const float secondaryDistance = std::sqrt(secondDistance);
    const float blend = (secondaryDistance > 0.0001f)
        ? std::clamp(1.0f - primaryDistance / secondaryDistance, 0.0f, 1.0f)
        : 0.0f;
    return {bestBiome, secondBiome, blend, elevation, temperature, moisture};
}

} // namespace ReligionSim
