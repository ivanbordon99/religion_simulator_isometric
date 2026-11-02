#pragma once

#include "Types.hpp"
#include <vector>
#include <string>
#include <atomic>
#include <thread>
#include <mutex>
#include <cmath>
#include <cstdint>

namespace ReligionSim {

class WorldMap;
class IsometricCamera;
class DayNightCycle;
class DivinePowersSystem;
class GestorHumanos;

// Tipos de efectos de sonido sintetizados proceduralmente
enum class SFXType {
    UIClick,         // Clic táctil de madera antigua para botones y menús
    BodyThud,        // Choque sordo al caer o rebotar un cuerpo (ragdoll)
    WaterSplash,     // Chapoteo acuático con burbujas (caer al agua o nadar)
    TreeChop,        // Golpe resonante de hacha al talar árboles
    StoneMine,       // Chasquido pétreo y metálico al picar canteras
    GatherFood,      // Crujido suave de arbustos al forrajear bayas
    ThunderStrike,   // Latigazo eléctrico y retumbar del trueno divino
    MeteorDescent,   // Rugido flameante ultrasónico de bólido en descenso
    MeteorImpact,    // Impacto cataclísmico con onda expansiva subgrave
    Earthquake,      // Crujido tectónico y sacudida de baja frecuencia
    TornadoWhoosh,   // Vórtice de viento ciclónico arremolinado
    PlagueSwarm,     // Zumbido tóxico de enjambre venenoso
    PrayerChant,     // Canto sagrado / zumbido armónico de meditación
    FaithGain,       // Campanillas ascendentes de devoción celestial
    EraAscension,    // Cuerno tribal y gong ceremonial al cambiar de era
    CombatAttack,    // Golpe seco de combate cuerpo a cuerpo
    UnitDeath        // Gemido tenue o exhalación de muerte
};

// Estructura de instancia de sonido en reproducción
struct ActiveSFXInstance {
    SFXType type{SFXType::UIClick};
    float elapsed{0.0f};
    float duration{0.5f};
    float volume{1.0f};
    float pan{0.0f};    // -1.0f (izquierda) a +1.0f (derecha)
    float pitch{1.0f};  // Multiplicador de afinación / velocidad
    float p0{0.0f};     // Parámetros internos de síntesis DSP
    float p1{0.0f};
    float p2{0.0f};
};

// =========================================================================
// SISTEMA INTEGRAL DE AUDIO, MÚSICA Y PAISAJE SONORO AMBIENTAL
// =========================================================================
class SoundSystem {
public:
    SoundSystem();
    ~SoundSystem();

    // Inicializar dispositivo de audio de la placa de sonido (hilo de audio en tiempo real)
    bool init();
    void shutdown();

    // Actualización de estado ambiental según la cámara, mapa, ciclo día/noche y desastres
    void update(float dt,
                const WorldMap& map,
                const IsometricCamera& camera,
                const DayNightCycle* dayNight,
                const DivinePowersSystem* powers,
                const GestorHumanos& humanos,
                bool isPaused = false,
                float speedMultiplier = 1.0f,
                int currentYear = -4000);

    // Sincronización con el tiempo de simulación y era histórica
    void setSimulationTimeState(bool isPaused, float speedMultiplier, int currentYear) {
        m_simulationPaused.store(isPaused);
        m_simulationSpeed.store(speedMultiplier);
        m_currentYear.store(currentYear);
    }
    bool isSimulationPaused() const { return m_simulationPaused.load(); }
    float getSimulationSpeed() const { return m_simulationSpeed.load(); }
    int getCurrentYear() const { return m_currentYear.load(); }

    // Reproducción de efectos de sonido estéreo o espaciales 2.5D
    void playSFX(SFXType type, float volume = 1.0f, float pan = 0.0f, float pitch = 1.0f);
    void playSpatialSFX(SFXType type, float worldX, float worldY,
                        const IsometricCamera& camera, float volume = 1.0f, float pitch = 1.0f);

    // Controles de volumen maestro y mezclas
    void setMasterVolume(float vol);
    float getMasterVolume() const { return m_masterVolume; }

    void setMusicVolume(float vol);
    float getMusicVolume() const { return m_musicVolume; }

    void setAmbientVolume(float vol);
    float getAmbientVolume() const { return m_ambientVolume; }

    void setSFXVolume(float vol);
    float getSFXVolume() const { return m_sfxVolume; }

    void toggleMute();
    void setMuted(bool mute) { m_muted = mute; }
    bool isMuted() const { return m_muted; }

    // Estado del hardware de audio
    bool isRunning() const { return m_running; }
    bool isHardwareActive() const { return m_hardwareActive; }
    const std::string& getDeviceName() const { return m_deviceName; }

private:
    // Hilo de renderizado de audio
    void audioThreadLoop();
    void renderAudioFrames(int16_t* outputBuffer, size_t numFrames);

    // Síntesis de paisaje sonoro ambiental
    float synthesizeWind(float dt);
    float synthesizeWaterWaves(float dt);
    float synthesizeBirdsong(float dt);
    float synthesizeCrickets(float dt);
    float synthesizeFireEmbers(float dt);

    // Síntesis de banda sonora musical ancestral adaptativa
    void updateMusicSequencer(float dt);
    void synthesizeMusic(float dt, float& outL, float& outR);

    // Síntesis de efectos de sonido instantáneos
    void renderSFXInstance(ActiveSFXInstance& sfx, float dt, float& outL, float& outR);

    // Filtros DSP rápidos
    float lowpassFilter(float input, float cutoff, float& state);
    float bandpassFilter(float input, float centerFreq, float q, float& s1, float& s2);

private:
    std::thread m_audioThread;
    std::atomic<bool> m_running{false};
    std::mutex m_mutex;

    bool m_hardwareActive{false};
    std::string m_deviceName{"None"};

    // Manejador dinámico de biblioteca ALSA (libasound.so.2)
    void* m_alsaLib{nullptr};
    void* m_alsaPcm{nullptr};

    // Volúmenes y silenciamiento
    std::atomic<float> m_masterVolume{0.75f};
    std::atomic<float> m_musicVolume{0.55f};
    std::atomic<float> m_ambientVolume{0.70f};
    std::atomic<float> m_sfxVolume{0.80f};
    std::atomic<bool> m_muted{false};

    // Sincronización con el tiempo de la simulación
    std::atomic<bool> m_simulationPaused{false};
    std::atomic<float> m_simulationSpeed{1.0f};
    std::atomic<int> m_currentYear{-4000};

    // Parámetros ambientales dinámicos
    float m_windIntensity{0.25f};
    float m_waterProximity{0.0f};
    float m_forestProximity{0.5f};
    float m_nightFactor{0.0f};
    float m_holySiteProximity{0.0f};
    float m_disasterTension{0.0f};
    float m_cameraZoomFactor{1.0f};

    // Estados internos de síntesis ambiental
    uint32_t m_noiseSeed{1337};
    float m_windGustTime{0.0f};
    float m_windFilterState{0.0f};
    float m_waterWaveTime{0.0f};
    float m_birdTimer{1.5f};
    float m_birdFreq{3200.0f};
    float m_birdPhase{0.0f};
    float m_cricketPhase{0.0f};
    float m_fireCrackleTimer{0.0f};

    // Estados del secuenciador de música ancestral
    float m_musicBeatTime{0.0f};
    int m_musicStep{0};
    int m_musicBar{0};
    float m_musicFlutePhase{0.0f};
    float m_musicFluteFreq{293.66f}; // D4
    float m_musicFluteTargetFreq{293.66f};
    float m_musicFluteEnv{0.0f};
    float m_musicDrumEnv{0.0f};
    float m_musicDrumFreq{55.0f};
    float m_musicDrumPhase{0.0f};
    float m_musicRattleEnv{0.0f};
    float m_musicDronePhase1{0.0f};
    float m_musicDronePhase2{0.0f};
    float m_musicBellPhase{0.0f};
    float m_musicBellEnv{0.0f};
    float m_musicBellFreq{587.33f}; // D5 (Gong de templo ancestral)
    float m_musicLiturgicalPhase1{0.0f};
    float m_musicLiturgicalPhase2{0.0f};

    // Cola de efectos de sonido activos
    std::vector<ActiveSFXInstance> m_activeSFX;
};

} // namespace ReligionSim
