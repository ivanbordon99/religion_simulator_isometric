#pragma once

#include "Types.hpp"
#include "MultiNoiseBiome.hpp"
#include <vector>
#include <string>
#include <utility>
#include <algorithm>
#include <cmath>

namespace ReligionSim {

// EN: Crater and tectonic rift morphology types
// ES: Tipos de morfología de cráteres y fallas tectónicas
enum class CraterType : uint8_t {
    MeteorImpact = 0,    // EN: High-heat meteor impact bowl with glowing magma veins / ES: Cráter de meteorito con magma incandescente
    TectonicSeismic,     // EN: Earthquake fault rift chasm and shattered bedrock / ES: Falla tectónica de terremoto y roca fragmentada
    NuclearDetonation,   // EN: Vitrified atomic crater basin / ES: Cuenca de cráter nuclear vitrificada
    KineticHeavyImpact   // EN: Crushed bedrock kinetic impact / ES: Impacto cinético de roca triturada
};

struct Cell {
    int x{0};
    int y{0};
    float elevation{1.0f}; // 0 = water/ocean, 1 = plains, 2 = hills, 3+ = mountains
    BiomeType biome{BiomeType::CoastalPlains};
    TileBiomeData climate;
    // EN: Optional explicit terrain atlas tile override (-1: use procedural biome mapping)
    // ES: Sobrescritura explícita opcional de mosaico del atlas (-1: usar mapeo procedimental por bioma)
    int8_t customTileIndex{-1};

    int population{1000};
    int maxPopulation{5000};
    float development{0.1f}; // 0.0 to 1.0 (village -> town -> metropolis)

    // Religion distribution: list of (religionId, fraction of population)
    // where sum of fractions <= 1.0 (remainder is secular/animist folk)
    std::vector<std::pair<int, float>> faithShare;
    int dominantReligionId{0};
    float dominantFraction{0.0f};

    // Holy site & Devotion
    bool hasHolySite{false};
    int shrineLevel{0}; // 0: None, 1: Shrine, 2: Temple, 3: Grand Sanctuary, 4: Wonder
    int holySiteReligionId{0};
    std::string holySiteName;
    float devotion{0.5f}; // Local fervor

    // Tension & Agglutination potential
    float syncreticTension{0.0f};
    int coexistingCenturies{0};

    // Tensión liminal y frontera inter-religiosa
    float borderTension{0.0f};           // 0.0 a 1.0 (intensidad de fricción en el linde)
    bool isContestedBorder{false};       // true si es celda frontera entre dos fes con fricción
    int borderRivalReligionId{0};        // ID de la religión rival al otro lado de la frontera
    bool isSacredTruceActive{false};     // Tregua sagrada o pacto de sangre intertribal
    float sacredTruceTimer{0.0f};        // Duración de la tregua/exogamia en años

    // Recursos naturales generados automáticamente en el mapa
    ResourceType naturalResource{ResourceType::None};
    int resourceAmount{0};
    int maxResourceAmount{0};
    float resourceRegenTimer{0.0f};
    bool hasTreesUprooted{false}; // true si los árboles fueron arrancados por cataclismo o sismo
    // EN: Visual geological variant of the natural deposit (vein, rock outcrop, nuggets, hoard/quarry)
    // ES: Variante geológica visual del depósito natural (veta, afloramiento rocoso, pepitas, tesoro/cantera)
    uint8_t resourceVariant{0};

    // Viviendas y asentamientos tribales
    bool hasHouse{false};
    int houseId{-1};

    // EN: Permanent terrain craters, impact bowls, and tectonic seismic rifts
    // ES: Cráteres permanentes de terreno, cuencas de impacto y fallas sísmicas tectónicas
    bool hasImpactCrater{false};
    float craterDepth{0.45f};
    CraterType craterType{CraterType::MeteorImpact};
    float fissureAngle{0.0f}; // EN: Angle of seismic fracture line in radians / ES: Ángulo de la línea de falla sísmica en radianes

    // EN: Form a crater or tectonic depression on the cell
    // ES: Formar un cráter o depresión tectónica en la celda
    void formCrater(CraterType type, float depth, float elevationDrop = 0.20f) noexcept {
        hasImpactCrater = true;
        craterDepth = std::clamp(std::max(craterDepth, depth), 0.10f, 1.0f);
        craterType = type;
        if (elevationDrop > 0.0f) {
            elevation = std::max(0.05f, elevation - elevationDrop);
        }
        // EN: Complete disintegration of vegetation inside impact bowl: no stumps or timber left behind ("nada más")
        // ES: Desintegración total de vegetación en la cuenca de impacto: sin tocones ni maderas ("nada más")
        hasTreesUprooted = false;
        if (naturalResource == ResourceType::Wood) {
            naturalResource = ResourceType::None;
            resourceAmount = 0;
            resourceVariant = 0;
        }
    }

    // Estructura y actividad volcánica
    bool hasVolcano{false};
    float volcanoHeight{2.2f};
    bool hasActiveLava{false};
    float lavaTimer{0.0f};

    // EN: Wildfires and thermodynamic combustion physics
    // ES: Incendios forestales y física de combustión termodinámica
    bool isBurning{false};
    float burnTimer{0.0f};
    float maxBurnTimer{14.0f};
    float fireIntensity{0.0f};          // EN: 0.0 to 1.0 (flame height & heat output) / ES: 0.0 a 1.0 (altura de llama y calor)
    float heatAccumulator{0.0f};        // EN: Preheating thermal energy transferred to neighbors / ES: Energía térmica acumulada para ignición
    bool isBurnt{false};
    float ashRecoveryTimer{0.0f};       // EN: Seconds until scorched ashes fertilize the soil / ES: Segundos hasta que las cenizas regeneren el suelo
    float fireImmunityTimer{0.0f};      // EN: Refractory cooldown timer: fresh sprout soil cannot reignite / ES: Tiempo refractario: suelo con brotes tiernos no puede arder

    // EN: Hydrology, water depth, soil moisture, and fluid physics
    // ES: Hidrología, profundidad de agua, humedad del suelo y física de fluidos
    float waterDepth{0.0f};             // EN: Water layer depth on cell in units / ES: Lámina de agua acumulada sobre la celda
    float flowVelocityX{0.0f};          // EN: Horizontal fluid current velocity vector X in m/s / ES: Vector de velocidad de corriente en X
    float flowVelocityY{0.0f};          // EN: Horizontal fluid current velocity vector Y in m/s / ES: Vector de velocidad de corriente en Y
    float moisture{0.20f};              // EN: Soil moisture saturation from 0.0 to 1.0 / ES: Saturación de humedad del suelo de 0.0 a 1.0
    bool isFlooded{false};              // EN: True if temporary floodwaters submerge terrain / ES: Verdadero si aguas torrenciales inundan el terreno
    float steamTimer{0.0f};             // EN: Rising steam plume timer when water quenches fire / ES: Temporizador de vapor cuando el agua apaga fuego
    // EN: 4-Phase water depth classification (Shoreline Foam, Shallow Coast, Mid Ocean, Deep Abyssal)
    // ES: Clasificación de 4 fases de profundidad del agua (Espuma de orilla, costa somera, mar medio, abisal profundo)
    WaterDepthPhase waterPhase{WaterDepthPhase::ShallowCoast};

    // EN: Progressive water depth phase query based on bathymetric depth or fluid accumulation
    // ES: Consulta de fase de profundidad del agua progresiva según batimetría o acumulación de fluido
    [[nodiscard]] WaterDepthPhase getWaterDepthPhase() const noexcept {
        if (!isWater() && waterDepth <= 0.015f) {
            return WaterDepthPhase::ShorelineFoam;
        }
        if (waterPhase != WaterDepthPhase::ShallowCoast) {
            return waterPhase;
        }
        if (biome == BiomeType::DeepOcean) {
            return WaterDepthPhase::DeepAbyssal;
        }
        if (waterDepth > 0.55f) {
            return WaterDepthPhase::DeepAbyssal;
        } else if (waterDepth > 0.28f) {
            return WaterDepthPhase::MidOcean;
        } else if (waterDepth > 0.08f) {
            return WaterDepthPhase::ShallowCoast;
        } else if (waterDepth > 0.0f) {
            return WaterDepthPhase::ShorelineFoam;
        }
        return WaterDepthPhase::ShallowCoast;
    }

    [[nodiscard]] float getTotalHydraulicHead() const noexcept {
        return elevation + waterDepth;
    }

    [[nodiscard]] float getFlowSpeed() const noexcept {
        return std::sqrt(flowVelocityX * flowVelocityX + flowVelocityY * flowVelocityY);
    }

    [[nodiscard]] float getInfiltrationRate() const noexcept {
        switch (biome) {
            case BiomeType::FertileValley:   return 0.055f; // Loam sponge
            case BiomeType::DenseForest:     return 0.045f; // Forest canopy and roots
            case BiomeType::CoastalPlains:   return 0.035f; // Sandy loam
            case BiomeType::AridDesert:      return 0.018f; // Baked crust
            case BiomeType::HighlandHills:   return 0.010f; // Thin topsoil
            case BiomeType::RuggedMountains: return 0.002f; // Impermeable rock
            default:                         return 0.025f;
        }
    }

    // EN: Calculate fuel flammability factor based on biome, timber resources, and structures
    // ES: Calcular factor de inflamabilidad del combustible según bioma, madera y estructuras
    [[nodiscard]] float getFlammability() const {
        if (isWater() || isBurnt || isFlooded || waterDepth > 0.05f) return 0.0f;
        float flammabilityScore = 0.0f;
        switch (biome) {
            case BiomeType::DenseForest: flammabilityScore += 1.00f; break;
            case BiomeType::FertileValley: flammabilityScore += 0.65f; break;
            case BiomeType::CoastalPlains: flammabilityScore += 0.45f; break;
            case BiomeType::HighlandHills: flammabilityScore += 0.25f; break;
            case BiomeType::FrozenTundra: flammabilityScore += 0.10f; break;
            case BiomeType::AridDesert:
            case BiomeType::RuggedMountains: flammabilityScore += 0.05f; break;
            default: break;
        }
        if (naturalResource == ResourceType::Wood && resourceAmount > 0) {
            flammabilityScore += 0.55f;
        }
        if (hasHouse) {
            flammabilityScore += 0.60f;
        }
        // EN: Dampened significantly by soil moisture
        // ES: Amortiguado sustancialmente por la humedad del suelo
        flammabilityScore *= std::max(0.0f, 1.0f - moisture * 0.90f);
        return flammabilityScore;
    }

    [[nodiscard]] bool isCombustible() const {
        if (fireImmunityTimer > 0.0f) return false;
        return getFlammability() > 0.10f;
    }

    void extinguish() {
        if (isBurning) {
            steamTimer = 2.5f;
        }
        isBurning = false;
        burnTimer = 0.0f;
        fireIntensity = 0.0f;
        heatAccumulator = 0.0f;
    }

    // EN: Add liquid water to the cell, updating saturation, flood state, and extinguishing fires
    // ES: Agregar agua líquida a la celda, actualizando saturación, estado de inundación y extinguiendo fuego
    void addWater(float amount) {
        if (amount <= 0.0f) return;
        waterDepth += amount;
        moisture = std::min(1.0f, moisture + amount * 2.5f);
        if (waterDepth >= 0.35f) {
            isFlooded = true;
        }
        if (isBurning) {
            // EN: Boiling off water when quenching intense flames (latent heat of vaporization)
            // ES: Hervir agua al extinguir llamas intensas (calor latente de vaporización)
            const float boilingWaterLoss = std::min(waterDepth, 0.10f * std::max(0.20f, fireIntensity));
            waterDepth = std::max(0.0f, waterDepth - boilingWaterLoss);
            if (waterDepth >= 0.03f || boilingWaterLoss >= 0.04f) {
                extinguish();
            } else {
                fireIntensity = std::max(0.15f, fireIntensity - 0.40f);
            }
        }
    }

    // EN: Harvest natural resources from this cell until depletion
    // ES: Extraer recursos naturales de esta celda hasta su agotamiento
    int harvestResource(int amount) {
        if (resourceAmount <= 0) {
            resourceAmount = 0;
            resourceVariant = 0;
            naturalResource = ResourceType::None;
            return 0;
        }
        const int harvested = std::min(amount, resourceAmount);
        resourceAmount -= harvested;
        if (resourceAmount <= 0) {
            resourceAmount = 0;
            resourceVariant = 0;
            const ResourceType depletedType = naturalResource;
            naturalResource = ResourceType::None;
            // EN: If timber is depleted, standing trees are cut down and dense forest clears to open plains
            // ES: Si la madera se agota, los árboles quedan talados y el bosque denso se despeja a pastizal
            if (depletedType == ResourceType::Wood) {
                hasTreesUprooted = true;
                if (biome == BiomeType::DenseForest) {
                    biome = BiomeType::CoastalPlains;
                }
            }
        }
        return harvested;
    }

    // EN: Query if this cell had a natural resource that has been completely exhausted
    // ES: Consultar si esta celda albergaba un recurso natural que fue completamente agotado
    bool isResourceDepleted() const {
        return (naturalResource == ResourceType::None && maxResourceAmount > 0);
    }

    // Helper functions
    float getShare(int religionId) const;
    void setShare(int religionId, float share);
    void addShare(int religionId, float delta);
    void normalizeShares();
    void updateDominance();

    bool isWater() const {
        return biome == BiomeType::DeepOcean || biome == BiomeType::ShallowWater || isFlooded;
    }

    bool isNaturalWater() const {
        return biome == BiomeType::DeepOcean || biome == BiomeType::ShallowWater;
    }

    bool isSubmerged() const {
        return isWater() || waterDepth >= 0.25f;
    }

    bool isDangerousTerrain() const {
        // Terreno peligroso: depresiones profundas/abismales (elevation <= 0.05f)
        // o cumbres de magma/lava escarpada (RuggedMountains o RockyPeak con elevación >= 2.8f)
        return (elevation <= 0.05f) ||
               (biome == BiomeType::RuggedMountains && elevation >= 2.8f) ||
               (climate.primary_biome == BiomeID::RockyPeak && elevation >= 2.8f);
    }

    bool isHabitable() const {
        return !isWater() && biome != BiomeType::RuggedMountains;
    }

    bool isPassableForGround() const {
        return !isWater() && isHabitable() && !isDangerousTerrain();
    }

    ColorRGB getBiomeBaseColor() const;
};

} // namespace ReligionSim
