#pragma once

#include <cstdint>
#include <array>
#include <vector>
#include <string>
#include <string_view>
#include <unordered_map>
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace ReligionSim {

class Humano;
class GestorHumanos;
class WorldMap;
class ReligionRegistry;
class EventLine;

// =========================================================================
// 1. SISTEMA NÉMESIS: REGISTRO DE MEMORIA Y EXPERIENCIAS VIVENCIALES
// =========================================================================
namespace Nemesis {

enum class MemoryEventType : uint8_t {
    SurvivedCombat          = 0, // Sobrevivió a duelo o escaramuza bélica
    WoundedByDivinePower    = 1, // Impactado por rayo, meteorito o sismo divino
    SparedByPlayer          = 2, // Se le perdonó la vida / sanado milagrosamente
    WitnessedSacrilege      = 3, // Presenció la destrucción o profanación de su templo
    BetrayedByLeader        = 4, // Abandonado sin víveres o desterrado por su cacique
    RivalKilledKinsman      = 5, // Vio morir a un allegado a manos de un rival
    StoleHolyRelic          = 6, // Saqueó u obtuvo una reliquia sagrada
    Excommunicated          = 7  // Condenado como hereje o apóstata
};

// Evento vivencial unitario compacto (16 bytes para alineación en caché L1/L2)
struct alignas(16) LifeEvent {
    MemoryEventType type{MemoryEventType::SurvivedCombat};
    uint8_t severity{0};           // Magnitud del trauma o júbilo (0-255)
    int16_t targetReligionId{-1};   // Facción instigadora o enemiga
    int32_t targetEntityId{-1};     // ID del agresor o salvador (-1 = Divino)
    int32_t timestampYear{-4000};   // Año de la simulación en que ocurrió
    float grudgeDelta{0.0f};        // Variación emocional: [-1.0f (devoción) a +1.0f (rencor)]
};

// Búfer circular estático de memoria individual: O(1) inserción, cero asignaciones en el heap
class EntityMemory {
public:
    static constexpr size_t MAX_MEMORIES = 8;

    void recordEvent(MemoryEventType type, int32_t year, int32_t targetEntity,
                     int16_t targetRel, uint8_t severity, float grudge) {
        m_events[m_head] = LifeEvent{
            .type = type,
            .severity = severity,
            .targetReligionId = targetRel,
            .targetEntityId = targetEntity,
            .timestampYear = year,
            .grudgeDelta = grudge
        };
        m_head = static_cast<uint8_t>((m_head + 1) % MAX_MEMORIES);
        if (m_count < MAX_MEMORIES) ++m_count;
    }

    [[nodiscard]] const LifeEvent* getLatestEvent() const noexcept {
        if (m_count == 0) return nullptr;
        size_t idx = (m_head + MAX_MEMORIES - 1) % MAX_MEMORIES;
        return &m_events[idx];
    }

    [[nodiscard]] size_t getEventCount() const noexcept { return m_count; }
    [[nodiscard]] const LifeEvent& getEvent(size_t index) const noexcept { return m_events[index]; }

private:
    std::array<LifeEvent, MAX_MEMORIES> m_events{};
    uint8_t m_head{0};
    uint8_t m_count{0};
};

enum class NemesisRank : uint8_t {
    Peasant      = 0, // Aldeano común sin registro destacado
    Survivor     = 1, // Sobreviviente curtido (ha superado 1 evento mortal)
    Avenger      = 2, // Vengador (acumula rencor concentrado contra un clan)
    Warlord      = 3, // Caudillo / Cabecilla rebelde (comanda lealtades disidentes)
    Heresiarch   = 4, // Hereje Supremo (capaz de fundar cismas doctrinales)
    LivingLegend = 5  // Avatar histórico inamovible (impacto civilizatorio total)
};

enum class NemesisPersonality : uint8_t {
    ZealotVengeful,    // Fanático vengativo: busca aniquilar herejes y altares rivales
    ApostateReformer,  // Apóstata reformador: rechaza a los dioses y funda cultos laicos
    BloodswornBrute,   // Juramentado de sangre: busca duelos cuerpo a cuerpo
    FanaticDevotee     // Devoto incondicional: transformado por la gracia divina
};

struct NemesisProfile {
    int32_t entityId{-1};
    NemesisRank rank{NemesisRank::Peasant};
    NemesisPersonality personality{NemesisPersonality::ZealotVengeful};

    // Matriz de Rencor y Notoriedad
    float grudgePlayer{0.0f};     // [-1.0f (Lealtad Santa) a +1.0f (Rencor Deicida)]
    float grudgeFaction{0.0f};    // Rencor contra la religión dominante del clan de origen
    int32_t nemesisTargetId{-1};  // ID específico de su némesis personal (-1 = ninguno)

    // Atributos de Ascenso
    float combatProwess{1.0f};    // Multiplicador de combate y resistencia física
    float eloquence{1.0f};        // Capacidad de seducir y convertir adeptos en cismas
    float zealotry{1.0f};         // Resistencia a la conversión mental del jugador

    // Título y Traumas
    char epithet[48]{"El Superviviente"};
    EntityMemory memory;

    void generateProceduralEpithet(MemoryEventType trigger);
};

// Sparse Set de Unidades Notables: O(1) búsqueda, adición y remoción contigua en memoria
class NotableUnitsPool {
public:
    explicit NotableUnitsPool(size_t maxEntities = 4096, size_t maxNotables = 128);

    [[nodiscard]] bool contains(int32_t entityId) const noexcept;
    NemesisProfile* get(int32_t entityId) noexcept;
    const NemesisProfile* get(int32_t entityId) const noexcept;
    NemesisProfile& promoteOrEmplace(int32_t entityId);
    void remove(int32_t entityId) noexcept;

    [[nodiscard]] size_t size() const noexcept { return m_dense.size(); }
    [[nodiscard]] std::vector<NemesisProfile>& getDenseProfiles() noexcept { return m_profiles; }
    [[nodiscard]] const std::vector<NemesisProfile>& getDenseProfiles() const noexcept { return m_profiles; }

private:
    void evictLeastSignificantNotable();

private:
    static constexpr uint32_t INVALID_INDEX = 0xFFFFFFFF;
    std::vector<uint32_t> m_sparse;
    std::vector<int32_t> m_dense;
    std::vector<NemesisProfile> m_profiles;
    size_t m_maxNotables;
};

} // namespace Nemesis

// =========================================================================
// 2. SISTEMA SÍSIFO (SIFIFUS ENGINE): MITOGÉNESIS Y RELATOS CULTURALES
// =========================================================================
namespace Sififus {

enum class MythArchetype : uint8_t {
    BloodVendetta       = 0, // Juramento de Venganza: odio focalizado contra un clan o dios
    MartyrSacrifice     = 1, // El Mártir Santo: devoción fanática, desprecio a la propia vida
    DivineRetribution   = 2, // Castigo del Cielo: sumisión temerosa ante desastres divinos
    PrometheanDefiance  = 3, // Desafío Titánico: herejía, rencor deicida, profanación
    SacredCovenant      = 4, // Pacto de Misericordia: agradecimiento divino, proselitismo
    HolyTaboo           = 5  // Tabú Dogmático: prohibición de consumir o talar un recurso
};

// Vector de Modificadores de Conducta (EthosVector) aplicado a los fieles
struct alignas(16) EthosVector {
    float aggressionBias{0.0f};      // [-1.0f (Pacifismo Absoluto) a +1.0f (Sed de Sangre)]
    float divineFearBias{0.0f};      // [-1.0f (Desafío Hereje) a +1.0f (Pánico Religioso)]
    float devotionPriority{0.0f};    // Ponderación de tiempo dedicado a rezar vs trabajar
    float xenophobiaBias{0.0f};      // Rechazo a matrimonios interclánicos y conversión
    float martyrdomDrive{0.0f};      // Disposición a inmolarse o luchar sin retirarse
    float resourceTabooId{-1.0f};    // ID de recurso vetado (-1 = ninguno)
};

// Estructura de un Relato Vivo
struct MythRecord {
    int32_t mythId{-1};
    int32_t protagonistEntityId{-1}; // El Némesis o aldeano central de la leyenda
    int16_t originReligionId{-1};
    int16_t targetReligionId{-1};    // Facción contra la que se orienta el relato (-1 = ninguna)
    int32_t creationYear{-4000};

    MythArchetype archetype{MythArchetype::BloodVendetta};
    EthosVector ethos;

    // Dinámica de propagación cultural
    float resonance{1.0f};           // Intensidad emocional activa [0.0f a 1.0f]
    float decayRate{0.015f};         // Desgaste del relato con el paso de los años
    uint32_t believerCount{0};       // Cantidad de fieles que han interiorizado el relato

    // Identidad narrativa procedural
    char title[64]{"Canto Primordial"};
    char oralSnippet[128]{"Y en aquel año oscuro, el fuego descendió sin piedad..."};
};

class NemesisSififusBridge {
public:
    static MythRecord generateMythFromNemesis(const Nemesis::NemesisProfile& profile,
                                              const Nemesis::LifeEvent& event,
                                              const char* protagonistName,
                                              int32_t mythId);
};

class SififusEngine {
public:
    static constexpr size_t MAX_ACTIVE_MYTHS = 64;

    explicit SififusEngine(EventLine* eventLine = nullptr);

    void registerMyth(const MythRecord& myth);
    void update(float dtYears, int currentYear);

    [[nodiscard]] EthosVector getCollectiveEthos(int16_t religionId) const noexcept;
    [[nodiscard]] const std::vector<MythRecord>& getActiveMyths() const noexcept { return m_activeMyths; }
    void setEventLine(EventLine* el) { m_eventLine = el; }

private:
    void evictFadedMyth();
    void recomputeCollectiveEthos();

private:
    EventLine* m_eventLine{nullptr};
    std::vector<MythRecord> m_activeMyths;
    std::unordered_map<int16_t, EthosVector> m_collectiveEthos;
    int32_t m_nextMythId{1};
};

} // namespace Sififus

// =========================================================================
// 3. GESTOR UNIFICADO: NÉMESIS & SÍSIFO MANAGER
// =========================================================================
class NemesisSififusManager {
public:
    NemesisSififusManager();

    // Notificaciones de eventos desde el combate o poderes divinos
    void onUnitSurvivesCombat(Humano& victim, int32_t attackerId, int16_t attackerRelId,
                             float damageTaken, int currentYear, EventLine* eventLine,
                             ReligionRegistry* registry);

    void onUnitHitByDivinePower(Humano& victim, float damageTaken, int currentYear,
                                EventLine* eventLine, ReligionRegistry* registry);

    void onUnitSparedOrBlessed(Humano& unit, int currentYear, EventLine* eventLine,
                               ReligionRegistry* registry);

    // Actualización temporal de mitos, emanaciones y modificación de la conducta de los fieles
    void update(float dtYears, int currentYear, GestorHumanos& gestor, WorldMap& map,
                ReligionRegistry* registry, EventLine* eventLine);

    // Acceso a subsistemas
    Nemesis::NotableUnitsPool& getPool() { return m_pool; }
    const Nemesis::NotableUnitsPool& getPool() const { return m_pool; }
    Sififus::SififusEngine& getSififus() { return m_sififus; }
    const Sififus::SififusEngine& getSififus() const { return m_sififus; }

    // Consulta de epíteto o estado notable
    const char* getNotableEpithetOrNull(int32_t entityId) const;
    const Nemesis::NemesisProfile* getProfileOrNull(int32_t entityId) const;

private:
    Nemesis::NotableUnitsPool m_pool;
    Sififus::SififusEngine m_sififus;
    int32_t m_nextMythId{1};
};

} // namespace ReligionSim
