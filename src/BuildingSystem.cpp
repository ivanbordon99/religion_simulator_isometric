#include "BuildingSystem.hpp"
#include "WorldMap.hpp"
#include <iostream>
#include <random>
#include <algorithm>
#include <cmath>

namespace ReligionSim {

static std::mt19937 g_bldRng(1337);

BuildingSystem::BuildingSystem()
    : m_houses(), m_nextHouseId(1), m_plotEvaluationTimer(0.0f) {}

// =============================================================================
// EN: Find suitable plot for building a new home based on terrain and ecology
// ES: Buscar una parcela idónea para construir un nuevo hogar según el terreno
// =============================================================================
bool BuildingSystem::findSuitableHousePlot(int originCellX, int originCellY, const WorldMap& worldMap,
                                           int& outCellX, int& outCellY, int searchRadius) const {
    struct PlotCandidate {
        int cellX{0};
        int cellY{0};
        float suitabilityScore{0.0f};
    };
    std::vector<PlotCandidate> candidatePlots;

    const int minCoordinateX = std::max(1, originCellX - searchRadius);
    const int maxCoordinateX = std::min(worldMap.getWidth() - 2, originCellX + searchRadius);
    const int minCoordinateY = std::max(1, originCellY - searchRadius);
    const int maxCoordinateY = std::min(worldMap.getHeight() - 2, originCellY + searchRadius);

    // =========================================================================
    // EN: Scan neighboring cells within search radius
    // ES: Escanear celdas vecinas dentro del radio de búsqueda
    // =========================================================================
    for (int candidateY = minCoordinateY; candidateY <= maxCoordinateY; ++candidateY) {
        for (int candidateX = minCoordinateX; candidateX <= maxCoordinateX; ++candidateX) {
            const Cell& candidateCell = worldMap.getCell(candidateX, candidateY);

            // EN: Suitability criteria: dry, passable terrain, no previous buildings or hazards
            // ES: Criterios de idoneidad: terreno seco, transitable, sin construcciones previas ni peligros
            if (candidateCell.isWater() || !candidateCell.isPassableForGround()) continue;
            if (candidateCell.hasHouse || candidateCell.hasHolySite || candidateCell.hasVolcano || candidateCell.hasImpactCrater) continue;

            // EN: Ensure no existing house occupies this coordinate
            // ES: Verificar que no haya otra casa existente en esta coordenada
            bool isCellOccupied = false;
            for (const auto& existingHouse : m_houses) {
                if (existingHouse.cellX == candidateX && existingHouse.cellY == candidateY) {
                    isCellOccupied = true;
                    break;
                }
            }
            if (isCellOccupied) continue;

            const float deltaCoordinateX = static_cast<float>(candidateX - originCellX);
            const float deltaCoordinateY = static_cast<float>(candidateY - originCellY);
            const float distanceSquared = deltaCoordinateX * deltaCoordinateX + deltaCoordinateY * deltaCoordinateY;

            // EN: Compute suitability score based on proximity, biome fertility, and timber resources
            // ES: Calcular puntuación de idoneidad basada en proximidad, fertilidad y recursos de madera
            float suitabilityScore = 100.0f - distanceSquared * 2.5f;
            if (candidateCell.biome == BiomeType::FertileValley) suitabilityScore += 15.0f;
            if (candidateCell.biome == BiomeType::DenseForest) suitabilityScore += 10.0f;
            if (candidateCell.naturalResource == ResourceType::Wood) suitabilityScore += 8.0f;

            // EN: Small stochastic jitter for organic tribal layout
            // ES: Ligera variación estocástica para distribución orgánica de la aldea
            std::uniform_real_distribution<float> scoreJitter(-3.0f, 3.0f);
            suitabilityScore += scoreJitter(g_bldRng);

            candidatePlots.push_back({candidateX, candidateY, suitabilityScore});
        }
    }

    if (candidatePlots.empty()) return false;

    // EN: Sort candidate plots in descending order of suitability
    // ES: Ordenar parcelas candidatas en orden descendente de idoneidad
    std::sort(candidatePlots.begin(), candidatePlots.end(),
              [](const PlotCandidate& firstPlot, const PlotCandidate& secondPlot) {
                  return firstPlot.suitabilityScore > secondPlot.suitabilityScore;
              });

    outCellX = candidatePlots[0].cellX;
    outCellY = candidatePlots[0].cellY;
    return true;
}

// =============================================================================
// EN: Initiate construction of a new tribal dwelling with specified typology
// ES: Iniciar la construcción de una nueva vivienda tribal con tipología específica
// =============================================================================
int BuildingSystem::initiateHouseConstruction(int cellX, int cellY, int ownerId, int partnerId,
                                              int religionId, HouseType type, WorldMap& worldMap) {
    if (!worldMap.isValid(cellX, cellY)) return -1;

    Cell& targetCell = worldMap.getCell(cellX, cellY);
    if (targetCell.hasHouse || targetCell.isWater()) return -1;

    House newHouse;
    newHouse.id = m_nextHouseId++;
    newHouse.cellX = cellX;
    newHouse.cellY = cellY;
    newHouse.religionId = religionId;
    newHouse.type = type;
    newHouse.state = BuildingState::SitePlanned;
    newHouse.buildProgress = 0.0f;
    newHouse.woodDelivered = 0;
    newHouse.ownerId = ownerId;
    newHouse.partnerId = partnerId;
    newHouse.health = 100.0f;
    newHouse.maxHealth = 100.0f;
    newHouse.comfort = 1.0f;

    // EN: Adjust structural parameters and capacity based on house type
    // ES: Ajustar parámetros estructurales y capacidad según el tipo de vivienda
    switch (type) {
        case HouseType::LargeRoundhouse:
            newHouse.woodRequired = 18;
            newHouse.capacity = 8;
            newHouse.maxHealth = 150.0f;
            newHouse.health = 150.0f;
            break;
        case HouseType::BoneDwelling:
            newHouse.woodRequired = 12;
            newHouse.capacity = 3;
            newHouse.maxHealth = 120.0f;
            newHouse.health = 120.0f;
            break;
        case HouseType::ChieftainLonghouse:
            newHouse.woodRequired = 20;
            newHouse.capacity = 6;
            newHouse.maxHealth = 160.0f;
            newHouse.health = 160.0f;
            break;
        case HouseType::PrimitiveShelter:
            newHouse.woodRequired = 6;
            newHouse.capacity = 2;
            newHouse.maxHealth = 60.0f;
            newHouse.health = 60.0f;
            break;
        case HouseType::ThatchedHut:
        default:
            newHouse.woodRequired = 10;
            newHouse.capacity = 4;
            newHouse.maxHealth = 100.0f;
            newHouse.health = 100.0f;
            break;
    }

    targetCell.hasHouse = true;
    targetCell.houseId = newHouse.id;

    evictResident(ownerId);
    if (partnerId > 0) evictResident(partnerId);

    m_houses.push_back(newHouse);
    return newHouse.id;
}

const House* BuildingSystem::getHouseAt(int cellX, int cellY) const {
    for (const auto& existingHouse : m_houses) {
        if (existingHouse.cellX == cellX && existingHouse.cellY == cellY) return &existingHouse;
    }
    return nullptr;
}

House* BuildingSystem::getHouseAtMutable(int cellX, int cellY) {
    for (auto& existingHouse : m_houses) {
        if (existingHouse.cellX == cellX && existingHouse.cellY == cellY) return &existingHouse;
    }
    return nullptr;
}

const House* BuildingSystem::getHouseById(int houseId) const {
    for (const auto& existingHouse : m_houses) {
        if (existingHouse.id == houseId) return &existingHouse;
    }
    return nullptr;
}

House* BuildingSystem::getHouseByIdMutable(int houseId) {
    for (auto& existingHouse : m_houses) {
        if (existingHouse.id == houseId) return &existingHouse;
    }
    return nullptr;
}

void BuildingSystem::evictResident(int humanId) {
    if (humanId <= 0) return;
    for (auto& targetHouse : m_houses) {
        if (targetHouse.ownerId == humanId) targetHouse.ownerId = -1;
        if (targetHouse.partnerId == humanId) targetHouse.partnerId = -1;
        targetHouse.removeResident(humanId);
    }
}

void BuildingSystem::destroyHouse(int houseId, WorldMap& worldMap, GestorHumanos* villagerManager) {
    for (auto iterator = m_houses.begin(); iterator != m_houses.end(); ++iterator) {
        if (iterator->id == houseId) {
            if (worldMap.isValid(iterator->cellX, iterator->cellY)) {
                Cell& targetCell = worldMap.getCell(iterator->cellX, iterator->cellY);
                if (targetCell.houseId == houseId) {
                    targetCell.hasHouse = false;
                    targetCell.houseId = -1;
                }
            }
            if (villagerManager) {
                for (auto& villager : villagerManager->getHumanos()) {
                    if (villager.getHomeId() == houseId) {
                        villager.setHomeId(-1);
                        villager.setDurmiendoBajoTecho(false);
                        villager.setNeedForShelter(0.65f);
                    }
                }
            }
            m_houses.erase(iterator);
            return;
        }
    }
}

void BuildingSystem::clear() {
    m_houses.clear();
    m_nextHouseId = 1;
}

int BuildingSystem::getCompletedHouseCount() const {
    int completedCount = 0;
    for (const auto& existingHouse : m_houses) {
        if (existingHouse.isCompleted()) completedCount++;
    }
    return completedCount;
}

int BuildingSystem::getUnderConstructionCount() const {
    int underConstructionCount = 0;
    for (const auto& existingHouse : m_houses) {
        if (existingHouse.isUnderConstruction()) underConstructionCount++;
    }
    return underConstructionCount;
}

// =============================================================================
// EN: Periodic cleanup of deceased occupants and succession of house ownership
// ES: Limpieza periódica de ocupantes fallecidos y sucesión de titularidad del hogar
// =============================================================================
void BuildingSystem::cleanOrphanedResidents(GestorHumanos& villagerManager) {
    auto& activePopulation = villagerManager.getHumanos();
    for (auto& currentHouse : m_houses) {
        // EN: Check if the original homeowner is still alive
        // ES: Verificar si el dueño original del hogar sigue vivo
        bool isOwnerAlive = false;
        if (currentHouse.ownerId > 0) {
            for (const auto& villager : activePopulation) {
                if (villager.getId() == currentHouse.ownerId && villager.estaVivo()) {
                    isOwnerAlive = true;
                    break;
                }
            }
        }
        if (!isOwnerAlive && currentHouse.partnerId > 0) {
            // EN: Surviving partner inherits ownership of the household
            // ES: El cónyuge superviviente hereda la titularidad del hogar
            currentHouse.ownerId = currentHouse.partnerId;
            currentHouse.partnerId = -1;
            isOwnerAlive = true;
        }

        // EN: Remove deceased occupants from resident roster
        // ES: Eliminar residentes fallecidos del registro del hogar
        currentHouse.residentIds.erase(
            std::remove_if(currentHouse.residentIds.begin(), currentHouse.residentIds.end(),
                [&activePopulation](int residentId) {
                    for (const auto& villager : activePopulation) {
                        if (villager.getId() == residentId && villager.estaVivo()) return false;
                    }
                    return true;
                }),
            currentHouse.residentIds.end()
        );
    }
}

// =============================================================================
// EN: Update building system, managing construction, sleeping census, and decay
// ES: Actualizar sistema de edificios, gestionando obras, censo y deterioro
// =============================================================================
void BuildingSystem::update(float deltaTime, WorldMap& worldMap, GestorHumanos& villagerManager,
                            EventLine* eventLine, int currentYear,
                            SoundSystem* soundSystem,
                            const IsometricCamera* camera,
                            ReligiousMoralitySystem* moralitySystem) {
    m_plotEvaluationTimer += deltaTime;
    auto& activePopulation = villagerManager.getHumanos();

    // =========================================================================
    // EN: 0. Update indoor sleeping census for all completed dwellings
    // ES: 0. Actualizar censo de durmientes bajo techo en cada vivienda
    // =========================================================================
    for (auto& currentHouse : m_houses) {
        int sleepingOccupantCount = 0;
        if (currentHouse.isCompleted()) {
            for (const auto& villager : activePopulation) {
                if (villager.estaVivo() && villager.getHomeId() == currentHouse.id && villager.estaDurmiendoBajoTecho()) {
                    sleepingOccupantCount++;
                }
            }
        }
        currentHouse.sleepingOccupants = sleepingOccupantCount;
        if (sleepingOccupantCount > 0) {
            currentHouse.hearthGlowTimer += deltaTime;
        }
    }

    // =========================================================================
    // EN: 1. Periodic cleanup of orphaned residents and ownership succession
    // ES: 1. Limpieza periódica de huérfanos y sucesión de titularidad
    // =========================================================================
    if (m_plotEvaluationTimer >= 3.0f) {
        cleanOrphanedResidents(villagerManager);
    }

    // =========================================================================
    // EN: 2. Evaluate individual and familial shelter necessity
    // ES: 2. Evaluar necesidad individual y conyugal de cobijo
    // =========================================================================
    for (auto& villager : activePopulation) {
        if (!villager.estaVivo()) continue;

        if (villager.tieneHogar()) {
            const House* assignedHouse = getHouseById(villager.getHomeId());
            if (!assignedHouse || assignedHouse->isRuined()) {
                // EN: Home was destroyed; reset shelter state
                // ES: La vivienda fue destruida; reiniciar estado de cobijo
                villager.setHomeId(-1);
                villager.setEstaConstruyendoHogar(false);
                villager.setNeedForShelter(0.50f);
            } else if (assignedHouse->isCompleted()) {
                // EN: Satisfied with shelter: necessity naturally decays
                // ES: Satisfecho con la vivienda: la necesidad decrece
                villager.modificarNeedForShelter(-0.12f * deltaTime);

                // EN: Health and vigor benefits within home proximity (distance <= 1.8 cells)
                // ES: Beneficios de salud y energía en proximidad al hogar (distancia <= 1.8 celdas)
                const float deltaCoordinateX = villager.getX() - (static_cast<float>(assignedHouse->cellX) + 0.5f);
                const float deltaCoordinateY = villager.getY() - (static_cast<float>(assignedHouse->cellY) + 0.5f);
                const float distanceSquared = deltaCoordinateX * deltaCoordinateX + deltaCoordinateY * deltaCoordinateY;

                if (distanceSquared <= 3.2f) {
                    villager.setEnergia(std::min(100.0f, villager.getEnergia() + 10.0f * deltaTime));
                    villager.setSalud(std::min(100.0f, villager.getSalud() + 2.5f * deltaTime));

                    if (villager.tienePareja()) {
                        villager.modificarAmor(0.025f * deltaTime);
                    }
                }
            }
        } else {
            // EN: Unsheltered: environmental stress and biological need accumulate
            // ES: A la intemperie: estrés ambiental y necesidad biológica acumulada
            float accumulatedNeed = 0.025f * deltaTime;

            if (villager.tienePareja()) {
                accumulatedNeed += 0.055f * deltaTime;
            }
            if (villager.esMujer() && villager.estaEmbarazada()) {
                accumulatedNeed += 0.095f * deltaTime;
            }
            if (villager.getHijosCount() > 0) {
                accumulatedNeed += 0.065f * deltaTime;
            }

            villager.modificarNeedForShelter(accumulatedNeed);
        }
    }

    // =========================================================================
    // EN: 3. Autonomous site planning triggered by accumulated shelter necessity
    // ES: 3. Planificación autónoma de obras según la necesidad acumulada
    // =========================================================================
    if (m_plotEvaluationTimer >= 1.0f) {
        m_plotEvaluationTimer = 0.0f;

        for (auto& villager : activePopulation) {
            if (!villager.estaVivo() || villager.esNino()) continue;
            if (villager.tieneHogar() || villager.estaConstruyendoHogar()) continue;

            // EN: Threshold of necessity for initiating house planning
            // ES: Umbral de necesidad para iniciar la planificación de vivienda
            if (villager.getNeedForShelter() >= 0.55f) {
                Humano* partnerVillagerPtr = nullptr;
                if (villager.tienePareja()) {
                    for (auto& candidateVillager : activePopulation) {
                        if (candidateVillager.getId() == villager.getParejaId() && candidateVillager.estaVivo()) {
                            partnerVillagerPtr = &candidateVillager;
                            break;
                        }
                    }
                }

                // EN: Search suitable plot near the requesting villager
                // ES: Buscar terreno idóneo cerca del aldeano solicitante
                int plotCellX = -1;
                int plotCellY = -1;
                if (findSuitableHousePlot(villager.getCeldaX(), villager.getCeldaY(), worldMap, plotCellX, plotCellY, 5)) {
                    const Cell& targetCell = worldMap.getCell(plotCellX, plotCellY);
                    HouseType selectedHouseType = HouseType::ThatchedHut;

                    if (targetCell.biome == BiomeType::FrozenTundra || targetCell.biome == BiomeType::HighlandHills) {
                        selectedHouseType = HouseType::BoneDwelling;
                    } else if (villager.esLiderEspiritual()) {
                        selectedHouseType = HouseType::ChieftainLonghouse;
                    } else if (villager.getHijosCount() >= 2) {
                        selectedHouseType = HouseType::LargeRoundhouse;
                    }

                    const int partnerId = partnerVillagerPtr ? partnerVillagerPtr->getId() : -1;
                    const int createdHouseId = initiateHouseConstruction(plotCellX, plotCellY, villager.getId(), partnerId,
                                                                         villager.getReligionId(), selectedHouseType, worldMap);

                    if (createdHouseId > 0) {
                        villager.setHomeId(createdHouseId);
                        villager.setEstaConstruyendoHogar(true);

                        if (partnerVillagerPtr) {
                            partnerVillagerPtr->setHomeId(createdHouseId);
                            partnerVillagerPtr->setEstaConstruyendoHogar(true);
                        }

                        // EN: Assign dependent unsheltered children respecting capacity
                        // ES: Asignar hijos pequeños sin hogar respetando capacidad
                        for (auto& childVillager : activePopulation) {
                            if (childVillager.estaVivo() && childVillager.esNino() && !childVillager.tieneHogar()) {
                                if (childVillager.getMadreId() == villager.getId() || childVillager.getPadreId() == villager.getId() ||
                                    (partnerVillagerPtr && (childVillager.getMadreId() == partnerVillagerPtr->getId() ||
                                                           childVillager.getPadreId() == partnerVillagerPtr->getId()))) {
                                    House* createdHousePtr = getHouseByIdMutable(createdHouseId);
                                    if (createdHousePtr && createdHousePtr->hasSpace()) {
                                        evictResident(childVillager.getId());
                                        createdHousePtr->addResident(childVillager.getId());
                                        childVillager.setHomeId(createdHouseId);
                                    }
                                }
                            }
                        }

                        // EN: Dispatch narrative chronicle event
                        // ES: Registrar evento narrativo en la crónica
                        if (eventLine) {
                            GameEvent narrativeEvent;
                            narrativeEvent.year = currentYear;
                            narrativeEvent.category = EventCategory::ExogamicPact;
                            narrativeEvent.title = "Nuevo Hogar Planeado";
                            std::string builderNames = villager.getNombre();
                            if (partnerVillagerPtr) builderNames += " y " + partnerVillagerPtr->getNombre();
                            narrativeEvent.description = builderNames + " han comenzado la construccion de una vivienda por necesidad de abrigo para su familia.";
                            narrativeEvent.actorName = villager.getNombre();
                            eventLine->addEvent(narrativeEvent);
                        }

                        break; // EN: Process one new site per cycle for organic pacing / ES: Procesar una obra por ciclo
                    }
                }
            }
        }
    }

    // =========================================================================
    // EN: 4. Physical construction progression for active building sites
    // ES: 4. Progresión física de la construcción de viviendas
    // =========================================================================
    for (auto& currentHouse : m_houses) {
        if (!currentHouse.isUnderConstruction()) continue;

        Humano* primaryBuilder = nullptr;
        Humano* secondaryHelper = nullptr;

        for (auto& villager : activePopulation) {
            if (!villager.estaVivo()) continue;
            if (villager.getId() == currentHouse.ownerId) primaryBuilder = &villager;
            else if (villager.getId() == currentHouse.partnerId) secondaryHelper = &villager;
        }

        if (!primaryBuilder && !secondaryHelper) {
            // EN: Both builders deceased; site is abandoned
            // ES: Ambos constructores han fallecido; la obra queda abandonada
            continue;
        }

        Humano& activeWorker = primaryBuilder ? *primaryBuilder : *secondaryHelper;

        // EN: Phase 1: Material gathering and hauling
        // ES: Fase 1: Recolección y acarreo de materiales
        if (currentHouse.woodDelivered < currentHouse.woodRequired) {
            currentHouse.state = BuildingState::GatheringMaterials;

            const float targetPositionX = static_cast<float>(currentHouse.cellX) + 0.5f;
            const float targetPositionY = static_cast<float>(currentHouse.cellY) + 0.5f;
            const float deltaPositionX = targetPositionX - activeWorker.getX();
            const float deltaPositionY = targetPositionY - activeWorker.getY();
            const float workerDistance = std::sqrt(deltaPositionX * deltaPositionX + deltaPositionY * deltaPositionY);

            if (workerDistance > 1.2f) {
                activeWorker.setPosicion(activeWorker.getX() + (deltaPositionX / workerDistance) * 0.9f * deltaTime,
                                         activeWorker.getY() + (deltaPositionY / workerDistance) * 0.9f * deltaTime);
                activeWorker.cambiarAccion(AccionHumano::CARGAR);
            } else {
                currentHouse.woodDelivered += 3;
                activeWorker.cambiarAccion(AccionHumano::TALAR);
            }
        }
        // EN: Phase 2 & 3: Framing and thatching
        // ES: Fase 2 y 3: Levantamiento del armazón y techado
        else {
            const float targetPositionX = static_cast<float>(currentHouse.cellX) + 0.5f;
            const float targetPositionY = static_cast<float>(currentHouse.cellY) + 0.5f;
            const float deltaPositionX = targetPositionX - activeWorker.getX();
            const float deltaPositionY = targetPositionY - activeWorker.getY();
            const float workerDistance = std::sqrt(deltaPositionX * deltaPositionX + deltaPositionY * deltaPositionY);

            if (workerDistance > 0.8f) {
                activeWorker.setPosicion(activeWorker.getX() + (deltaPositionX / workerDistance) * 0.8f * deltaTime,
                                         activeWorker.getY() + (deltaPositionY / workerDistance) * 0.8f * deltaTime);
                activeWorker.cambiarAccion(AccionHumano::CAMINAR);
            } else {
                activeWorker.cambiarAccion(AccionHumano::TALAR);

                float constructionSpeed = 10.5f;
                if (secondaryHelper && secondaryHelper->estaVivo()) constructionSpeed += 6.0f;

                currentHouse.buildProgress += deltaTime * constructionSpeed;

                if (currentHouse.buildProgress < 50.0f) {
                    currentHouse.state = BuildingState::Framing;
                } else if (currentHouse.buildProgress < 100.0f) {
                    currentHouse.state = BuildingState::Thatching;
                } else {
                    // EN: Dwelling successfully completed!
                    // ES: ¡Vivienda completada con éxito!
                    currentHouse.state = BuildingState::Completed;
                    currentHouse.buildProgress = 100.0f;

                    activeWorker.cambiarAccion(AccionHumano::SALUDAR);
                    activeWorker.triggerFaithIndicator(+1, 2.5f);
                    activeWorker.setEstaConstruyendoHogar(false);
                    activeWorker.setNeedForShelter(0.0f);

                    if (secondaryHelper) {
                        secondaryHelper->cambiarAccion(AccionHumano::SALUDAR);
                        secondaryHelper->triggerFaithIndicator(+1, 2.5f);
                        secondaryHelper->setEstaConstruyendoHogar(false);
                        secondaryHelper->setNeedForShelter(0.0f);
                    }

                    if (moralitySystem) {
                        activeWorker.getMoralRecordMutable().addRighteousAct(0.08f);
                        if (secondaryHelper) secondaryHelper->getMoralRecordMutable().addRighteousAct(0.08f);
                    }

                    if (soundSystem && camera) {
                        soundSystem->playSpatialSFX(SFXType::FaithGain,
                                                   targetPositionX, targetPositionY, *camera, 1.0f);
                    }

                    if (eventLine) {
                        GameEvent completionEvent;
                        completionEvent.year = currentYear;
                        completionEvent.category = EventCategory::HolySiteErected;
                        completionEvent.title = "Vivienda Finalizada";
                        std::string builderNames = activeWorker.getNombre();
                        if (secondaryHelper) builderNames += " y " + secondaryHelper->getNombre();
                        completionEvent.description = builderNames + " han erigido su " + currentHouse.getTypeName() + ". La familia ahora prospera protegida bajo techo propio.";
                        completionEvent.actorName = activeWorker.getNombre();
                        eventLine->addEvent(completionEvent);
                    }
                }
            }
        }
    }
}

} // namespace ReligionSim
