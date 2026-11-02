#include "SoundSystem.hpp"
#include "WorldMap.hpp"
#include "IsometricCamera.hpp"
#include "DayNightCycle.hpp"
#include "DivinePowersSystem.hpp"
#include "Humano.hpp"
#include <iostream>
#include <algorithm>
#include <cstring>
#include <chrono>
#include <dlfcn.h>

namespace ReligionSim {

namespace {
    // Declaraciones de tipos para API ALSA
    typedef void snd_pcm_t;
    typedef int snd_pcm_stream_t;
    typedef int snd_pcm_format_t;
    typedef int snd_pcm_access_t;
    typedef long snd_pcm_sframes_t;
    typedef unsigned long snd_pcm_uframes_t;

    constexpr int SND_PCM_STREAM_PLAYBACK = 0;
    constexpr int SND_PCM_FORMAT_S16_LE = 2;
    constexpr int SND_PCM_ACCESS_RW_INTERLEAVED = 3;

    // Escala pentatónica ancestral (D menor / dórico)
    constexpr float PENTATONIC_SCALE[] = {
        220.00f, // A3
        261.63f, // C4
        293.66f, // D4
        349.23f, // F4
        392.00f, // G4
        440.00f, // A4
        523.25f, // C5
        587.33f  // D5
    };
    constexpr int NUM_SCALE_NOTES = sizeof(PENTATONIC_SCALE) / sizeof(PENTATONIC_SCALE[0]);

    inline float fastNoise(uint32_t& seed) {
        seed = seed * 1664525u + 1013904223u;
        return static_cast<float>(static_cast<int32_t>(seed)) / 2147483648.0f;
    }
}

SoundSystem::SoundSystem() {
}

SoundSystem::~SoundSystem() {
    shutdown();
}

bool SoundSystem::init() {
    if (m_running.load()) return true;

    // 1. Intentar cargar dinámicamente libasound.so.2 (ALSA)
    m_alsaLib = dlopen("libasound.so.2", RTLD_NOW);
    if (!m_alsaLib) {
        m_alsaLib = dlopen("/usr/lib/x86_64-linux-gnu/libasound.so.2", RTLD_NOW);
    }

    if (m_alsaLib) {
        auto snd_pcm_open = (int (*)(snd_pcm_t**, const char*, snd_pcm_stream_t, int))dlsym(m_alsaLib, "snd_pcm_open");
        auto snd_pcm_set_params = (int (*)(snd_pcm_t*, snd_pcm_format_t, snd_pcm_access_t, unsigned int, unsigned int, int, unsigned int))dlsym(m_alsaLib, "snd_pcm_set_params");

        if (snd_pcm_open && snd_pcm_set_params) {
            // Dispositivos candidatos en orden de preferencia
            const char* candidates[] = {
                "default",
                "pulse",
                "pipewire",
                "sysdefault",
                "plughw:0,0",
                "hw:0,0"
            };

            for (const char* dev : candidates) {
                snd_pcm_t* pcm = nullptr;
                int err = snd_pcm_open(&pcm, dev, SND_PCM_STREAM_PLAYBACK, 0);
                if (err == 0 && pcm != nullptr) {
                    // Configurar: 16-bit signed, estéreo, 44100 Hz, latencia de 45 ms
                    err = snd_pcm_set_params(pcm, SND_PCM_FORMAT_S16_LE, SND_PCM_ACCESS_RW_INTERLEAVED, 2, 44100, 1, 45000);
                    if (err == 0) {
                        m_alsaPcm = pcm;
                        m_hardwareActive = true;
                        m_deviceName = std::string("ALSA (") + dev + ")";
                        std::cout << "[SoundSystem] Placa de sonido inicializada con exito: " << m_deviceName << " @ 44.1 kHz Estereo" << std::endl;
                        break;
                    } else {
                        auto snd_pcm_close = (int (*)(snd_pcm_t*))dlsym(m_alsaLib, "snd_pcm_close");
                        if (snd_pcm_close) snd_pcm_close(pcm);
                    }
                }
            }
        }
    }

    if (!m_hardwareActive) {
        std::cout << "[SoundSystem] Dispositivo de sonido fisico no disponible en este entorno. Modo sintetizador emulado / silencioso activo." << std::endl;
        m_deviceName = "Emulated Silent Mixer";
    }

    // 2. Iniciar el hilo de síntesis y reproducción de audio en tiempo real
    m_running.store(true);
    m_audioThread = std::thread(&SoundSystem::audioThreadLoop, this);

    return true;
}

void SoundSystem::shutdown() {
    if (!m_running.load()) return;

    m_running.store(false);
    if (m_audioThread.joinable()) {
        m_audioThread.join();
    }

    if (m_alsaPcm && m_alsaLib) {
        auto snd_pcm_close = (int (*)(snd_pcm_t*))dlsym(m_alsaLib, "snd_pcm_close");
        if (snd_pcm_close) {
            snd_pcm_close(static_cast<snd_pcm_t*>(m_alsaPcm));
        }
        m_alsaPcm = nullptr;
    }

    if (m_alsaLib) {
        dlclose(m_alsaLib);
        m_alsaLib = nullptr;
    }

    m_hardwareActive = false;
    std::cout << "[SoundSystem] Subsistema de audio finalizado limpiamente." << std::endl;
}

void SoundSystem::setMasterVolume(float vol) {
    m_masterVolume.store(std::clamp(vol, 0.0f, 1.0f));
}

void SoundSystem::setMusicVolume(float vol) {
    m_musicVolume.store(std::clamp(vol, 0.0f, 1.0f));
}

void SoundSystem::setAmbientVolume(float vol) {
    m_ambientVolume.store(std::clamp(vol, 0.0f, 1.0f));
}

void SoundSystem::setSFXVolume(float vol) {
    m_sfxVolume.store(std::clamp(vol, 0.0f, 1.0f));
}

void SoundSystem::toggleMute() {
    m_muted.store(!m_muted.load());
    std::cout << "[SoundSystem] " << (m_muted.load() ? "Sonido SILENCIADO (Mute)" : "Sonido ACTIVADO") << std::endl;
}

void SoundSystem::playSFX(SFXType type, float volume, float pan, float pitch) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_activeSFX.size() >= 32) return; // Evitar saturación

    ActiveSFXInstance inst;
    inst.type = type;
    inst.elapsed = 0.0f;
    inst.volume = std::clamp(volume, 0.0f, 2.0f);
    inst.pan = std::clamp(pan, -1.0f, 1.0f);
    inst.pitch = std::clamp(pitch, 0.25f, 4.0f);

    switch (type) {
        case SFXType::UIClick:       inst.duration = 0.05f; break;
        case SFXType::BodyThud:      inst.duration = 0.25f; break;
        case SFXType::WaterSplash:   inst.duration = 0.45f; break;
        case SFXType::TreeChop:      inst.duration = 0.22f; break;
        case SFXType::StoneMine:     inst.duration = 0.26f; break;
        case SFXType::GatherFood:    inst.duration = 0.18f; break;
        case SFXType::ThunderStrike: inst.duration = 1.80f; break;
        case SFXType::MeteorDescent: inst.duration = 1.20f; break;
        case SFXType::MeteorImpact:  inst.duration = 2.60f; break;
        case SFXType::Earthquake:    inst.duration = 2.80f; break;
        case SFXType::TornadoWhoosh: inst.duration = 1.50f; break;
        case SFXType::PlagueSwarm:   inst.duration = 1.60f; break;
        case SFXType::PrayerChant:   inst.duration = 2.40f; break;
        case SFXType::FaithGain:     inst.duration = 0.85f; break;
        case SFXType::EraAscension:  inst.duration = 3.20f; break;
        case SFXType::CombatAttack:  inst.duration = 0.22f; break;
        case SFXType::UnitDeath:     inst.duration = 0.65f; break;
    }

    m_activeSFX.push_back(inst);
}

void SoundSystem::playSpatialSFX(SFXType type, float worldX, float worldY,
                                 const IsometricCamera& camera, float volume, float pitch) {
    int screenW = camera.getScreenWidth();
    int screenH = camera.getScreenHeight();
    int camGx = 0, camGy = 0;
    camera.screenToGrid(screenW * 0.5f, screenH * 0.5f, camGx, camGy);

    float dx = worldX - static_cast<float>(camGx);
    float dy = worldY - static_cast<float>(camGy);
    float dist = std::hypot(dx, dy);

    if (dist > 45.0f) return; // Fuera del radio audible de la cámara

    // Paneo estéreo según proyección en pantalla
    Vec2 scr = camera.gridToScreen(worldX, worldY);
    float pan = (scr.x - screenW * 0.5f) / (screenW * 0.5f);
    pan = std::clamp(pan, -0.9f, 0.9f);

    // Atenuación cuadrática suave con la distancia
    float distAtten = 1.0f / (1.0f + (dist / 14.0f) * (dist / 14.0f));
    float finalVol = volume * distAtten;

    if (finalVol > 0.015f) {
        playSFX(type, finalVol, pan, pitch);
    }
}

void SoundSystem::update(float dt,
                         const WorldMap& map,
                         const IsometricCamera& camera,
                         const DayNightCycle* dayNight,
                         const DivinePowersSystem* powers,
                         const GestorHumanos& humanos,
                         bool isPaused,
                         float speedMultiplier,
                         int currentYear) {
    (void)humanos;
    m_simulationPaused.store(isPaused);
    m_simulationSpeed.store(speedMultiplier);
    m_currentYear.store(currentYear);

    int screenW = camera.getScreenWidth();
    int screenH = camera.getScreenHeight();
    int camGx = 0, camGy = 0;
    camera.screenToGrid(screenW * 0.5f, screenH * 0.5f, camGx, camGy);
    int centerCellX = std::clamp(camGx, 0, map.getWidth() - 1);
    int centerCellY = std::clamp(camGy, 0, map.getHeight() - 1);

    // 1. Detección ambiental en el entorno de la cámara
    int waterCount = 0;
    int forestCount = 0;
    int holySiteCount = 0;
    float elevationSum = 0.0f;
    int sampleRadius = 4;
    int totalCells = 0;

    for (int dy = -sampleRadius; dy <= sampleRadius; ++dy) {
        for (int dx = -sampleRadius; dx <= sampleRadius; ++dx) {
            int nx = centerCellX + dx;
            int ny = centerCellY + dy;
            if (!map.isValid(nx, ny)) continue;
            totalCells++;
            const Cell& c = map.getCell(nx, ny);
            if (c.isWater()) waterCount++;
            if (c.biome == BiomeType::DenseForest || c.naturalResource == ResourceType::Wood) forestCount++;
            if (c.hasHolySite) holySiteCount++;
            elevationSum += c.elevation;
        }
    }

    float avgElevation = totalCells > 0 ? (elevationSum / totalCells) : 1.0f;
    float targetWater = totalCells > 0 ? (static_cast<float>(waterCount) / totalCells) : 0.0f;
    float targetForest = totalCells > 0 ? (static_cast<float>(forestCount) / totalCells) : 0.0f;
    float targetHoly = holySiteCount > 0 ? 1.0f : 0.0f;

    // Transición suave de parámetros ambientales
    m_waterProximity += (targetWater - m_waterProximity) * std::min(1.0f, dt * 2.0f);
    m_forestProximity += (targetForest - m_forestProximity) * std::min(1.0f, dt * 2.0f);
    m_holySiteProximity += (targetHoly - m_holySiteProximity) * std::min(1.0f, dt * 2.0f);

    // 2. Factor de viento según altitud, zoom y tormentas
    float zoom = camera.getZoom();
    m_cameraZoomFactor = zoom;
    float baseWind = 0.20f + (avgElevation > 1.8f ? 0.35f : 0.0f);
    if (zoom < 0.75f) baseWind += 0.25f; // Viento de gran altitud en zoom alejado

    // Verificar si hay desastres activos de viento/catástrofe
    float disasterBoost = 0.0f;
    if (powers) {
        for (const auto& fx : powers->getActiveFX()) {
            if (fx.type == DivinePowerType::Meteor ||
                fx.type == DivinePowerType::Tornado ||
                fx.type == DivinePowerType::Earthquake ||
                fx.type == DivinePowerType::Lightning) {
                disasterBoost += 0.40f;
            }
        }
    }
    m_disasterTension += (disasterBoost - m_disasterTension) * std::min(1.0f, dt * 4.0f);
    m_windIntensity = std::clamp(baseWind + m_disasterTension * 0.5f, 0.1f, 1.0f);

    // 3. Ciclo día / noche para grillos vs pájaros sincronizado exactamente con la hora de DayNightCycle
    if (dayNight) {
        float hour = dayNight->getHour(); // Rango de 0.0f a 24.0f
        // Madrugada / Noche profunda: 20:30 a 04:30 -> nightFactor = 1.0 (grillos y sosiego)
        // Amanecer: 04:30 a 07:30 -> transición gradual 1.0 -> 0.0 (despertar de las aves)
        // Día pleno / Mediodía: 07:30 a 17:30 -> nightFactor = 0.0 (canto de pájaros pleno)
        // Atardecer / Ocaso: 17:30 a 20:30 -> transición gradual 0.0 -> 1.0 (crepúsculo)
        if (hour >= 20.5f || hour < 4.5f) {
            m_nightFactor = 1.0f;
        } else if (hour >= 7.5f && hour <= 17.5f) {
            m_nightFactor = 0.0f;
        } else if (hour >= 4.5f && hour < 7.5f) {
            m_nightFactor = 1.0f - (hour - 4.5f) / 3.0f;
        } else {
            m_nightFactor = (hour - 17.5f) / 3.0f;
        }
    }
}

// =========================================================================
// BUCLE DE AUDIO Y SÍNTESIS DSP EN TIEMPO REAL
// =========================================================================
void SoundSystem::audioThreadLoop() {
    constexpr size_t FRAMES_PER_BUFFER = 1024;
    std::vector<int16_t> pcmBuffer(FRAMES_PER_BUFFER * 2);

    auto snd_pcm_writei = m_alsaLib ? (snd_pcm_sframes_t (*)(snd_pcm_t*, const void*, snd_pcm_uframes_t))dlsym(m_alsaLib, "snd_pcm_writei") : nullptr;
    auto snd_pcm_recover = m_alsaLib ? (int (*)(snd_pcm_t*, int, int))dlsym(m_alsaLib, "snd_pcm_recover") : nullptr;

    auto nextBufferTime = std::chrono::steady_clock::now();
    const auto bufferDuration = std::chrono::microseconds(static_cast<long long>(FRAMES_PER_BUFFER * 1000000.0 / 44100.0));

    while (m_running.load()) {
        renderAudioFrames(pcmBuffer.data(), FRAMES_PER_BUFFER);

        if (m_hardwareActive && m_alsaPcm && snd_pcm_writei) {
            snd_pcm_sframes_t framesWritten = snd_pcm_writei(static_cast<snd_pcm_t*>(m_alsaPcm), pcmBuffer.data(), FRAMES_PER_BUFFER);
            if (framesWritten < 0) {
                if (snd_pcm_recover) {
                    snd_pcm_recover(static_cast<snd_pcm_t*>(m_alsaPcm), static_cast<int>(framesWritten), 0);
                }
            }
        } else {
            // Emulación de sincronización de reloj de audio
            nextBufferTime += bufferDuration;
            std::this_thread::sleep_until(nextBufferTime);
        }
    }
}

void SoundSystem::renderAudioFrames(int16_t* outputBuffer, size_t numFrames) {
    if (m_muted.load()) {
        std::memset(outputBuffer, 0, numFrames * 2 * sizeof(int16_t));
        return;
    }

    constexpr float dt = 1.0f / 44100.0f;
    float master = m_masterVolume.load();
    float musicVol = m_musicVolume.load();
    float ambVol = m_ambientVolume.load();
    float sfxVol = m_sfxVolume.load();

    std::lock_guard<std::mutex> lock(m_mutex);

    for (size_t f = 0; f < numFrames; ++f) {
        // 1. Síntesis de paisaje ambiental (Viento, Olas, Pájaros, Grillos, Fuego)
        float wind = synthesizeWind(dt) * ambVol * 0.40f;
        float waves = synthesizeWaterWaves(dt) * ambVol * m_waterProximity * 0.45f;
        float birds = synthesizeBirdsong(dt) * ambVol * m_forestProximity * (1.0f - m_nightFactor) * 0.35f;
        float crickets = synthesizeCrickets(dt) * ambVol * m_nightFactor * 0.30f;
        float embers = synthesizeFireEmbers(dt) * ambVol * m_holySiteProximity * 0.35f;

        float ambientL = wind * 0.9f + waves * 0.85f + birds * 0.70f + crickets * 0.8f + embers * 0.6f;
        float ambientR = wind * 1.1f + waves * 1.15f + birds * 1.30f + crickets * 1.2f + embers * 1.4f;

        // 2. Síntesis de banda sonora musical ancestral adaptativa
        updateMusicSequencer(dt);
        float musicL = 0.0f, musicR = 0.0f;
        synthesizeMusic(dt, musicL, musicR);
        musicL *= musicVol;
        musicR *= musicVol;

        // 3. Mezcla de efectos de sonido instantáneos
        float sfxTotalL = 0.0f, sfxTotalR = 0.0f;
        for (auto& sfx : m_activeSFX) {
            float sL = 0.0f, sR = 0.0f;
            renderSFXInstance(sfx, dt, sL, sR);
            sfxTotalL += sL * sfxVol;
            sfxTotalR += sR * sfxVol;
        }

        // Mezcla total estéreo
        float mixL = (ambientL + musicL + sfxTotalL) * master;
        float mixR = (ambientR + musicR + sfxTotalR) * master;

        // Soft clipper analógico (curva tanh / cúbica suave para evitar distorsión)
        mixL = std::clamp(mixL * 0.85f, -1.0f, 1.0f);
        mixR = std::clamp(mixR * 0.85f, -1.0f, 1.0f);
        mixL = mixL - (mixL * mixL * mixL) * 0.18f;
        mixR = mixR - (mixR * mixR * mixR) * 0.18f;

        outputBuffer[f * 2]     = static_cast<int16_t>(mixL * 32760.0f);
        outputBuffer[f * 2 + 1] = static_cast<int16_t>(mixR * 32760.0f);
    }

    // Retirar efectos de sonido que ya finalizaron su duración
    m_activeSFX.erase(
        std::remove_if(m_activeSFX.begin(), m_activeSFX.end(),
                       [](const ActiveSFXInstance& s) { return s.elapsed >= s.duration; }),
        m_activeSFX.end());
}

// =========================================================================
// SÍNTESIS DSP AMBIENTAL
// =========================================================================
float SoundSystem::synthesizeWind(float dt) {
    m_windGustTime += dt * 0.8f;
    float gust = 0.6f + 0.30f * std::sin(m_windGustTime * 0.65f) + 0.15f * std::sin(m_windGustTime * 1.8f);
    float noise = fastNoise(m_noiseSeed);

    // Filtro pasabajos resonante dinámico
    float cutoff = 90.0f + 650.0f * (m_windIntensity * gust);
    float alpha = std::clamp(2.0f * 3.14159f * cutoff * dt, 0.001f, 0.8f);
    m_windFilterState += alpha * (noise - m_windFilterState);

    return m_windFilterState * gust * 1.6f;
}

float SoundSystem::synthesizeWaterWaves(float dt) {
    m_waterWaveTime += dt;
    float swell = std::max(0.0f, std::sin(m_waterWaveTime * 0.75f));
    float foamNoise = fastNoise(m_noiseSeed) * 0.25f;

    // Sonido sordo del oleaje + espuma
    float wave = (std::sin(m_waterWaveTime * 0.75f * 2.0f * 3.14159f) * 0.4f + foamNoise) * (swell * swell);
    return wave * 0.85f;
}

float SoundSystem::synthesizeBirdsong(float dt) {
    m_birdTimer -= dt;
    if (m_birdTimer <= 0.0f) {
        // Intervalo aleatorio entre gorjeos
        m_birdTimer = 2.5f + (fastNoise(m_noiseSeed) + 1.0f) * 2.0f;
        m_birdFreq = 2600.0f + (fastNoise(m_noiseSeed) + 1.0f) * 700.0f;
    }

    // Gorjeo activo durante los primeros 0.22s del ciclo
    if (m_birdTimer > 2.0f && m_birdTimer < 2.22f) {
        float chirpT = 2.22f - m_birdTimer;
        float mod = std::sin(chirpT * 60.0f) * 450.0f;
        m_birdPhase += (m_birdFreq + mod) * dt * 2.0f * 3.14159f;
        float env = std::sin((chirpT / 0.22f) * 3.14159f);
        return std::sin(m_birdPhase) * env * 0.45f;
    }
    return 0.0f;
}

float SoundSystem::synthesizeCrickets(float dt) {
    m_cricketPhase += dt * 4800.0f * 2.0f * 3.14159f;
    float modPulse = std::sin(m_cricketPhase * (18.0f / 4800.0f));
    if (modPulse > 0.4f) {
        float tone = std::sin(m_cricketPhase) * 0.35f + std::sin(m_cricketPhase * 1.25f) * 0.15f;
        return tone * (modPulse - 0.4f);
    }
    return 0.0f;
}

float SoundSystem::synthesizeFireEmbers(float dt) {
    m_fireCrackleTimer -= dt;
    if (m_fireCrackleTimer <= 0.0f) {
        m_fireCrackleTimer = 0.03f + (fastNoise(m_noiseSeed) + 1.0f) * 0.06f;
        float pop = fastNoise(m_noiseSeed);
        return pop * 0.65f;
    }
    return 0.0f;
}

// =========================================================================
// MÚSICA ANCESTRAL PROCEDURAL ADAPTATIVA (SINCRONIZADA CON EL TIEMPO)
// =========================================================================
void SoundSystem::updateMusicSequencer(float dt) {
    if (m_simulationPaused.load()) {
        // En pausa de simulación, se congelan los compases y la percusión,
        // decayendo suavemente las notas para sostener un pad sagrado en suspensión
        m_musicDrumEnv = std::max(0.0f, m_musicDrumEnv - dt * 3.5f);
        m_musicRattleEnv = std::max(0.0f, m_musicRattleEnv - dt * 12.0f);
        m_musicFluteEnv = std::max(0.0f, m_musicFluteEnv - dt * 1.5f);
        m_musicBellEnv = std::max(0.0f, m_musicBellEnv - dt * 0.8f);
        return;
    }

    // Escala rítmica armónica según la velocidad del tiempo (1X, 2X, 5X)
    float speedMult = m_simulationSpeed.load();
    float tempoScale = 1.0f;
    if (speedMult >= 4.5f) {
        tempoScale = 1.55f; // Trance sagrado acelerado sin estridencia
    } else if (speedMult >= 1.8f) {
        tempoScale = 1.25f; // Dinamismo ceremonial ágil
    }

    m_musicBeatTime += dt * tempoScale;
    constexpr float STEP_DURATION = 0.225f; // ~66 BPM a velocidad normal

    if (m_musicBeatTime >= STEP_DURATION) {
        m_musicBeatTime -= STEP_DURATION;
        m_musicStep = (m_musicStep + 1) % 16;
        if (m_musicStep == 0) {
            m_musicBar = (m_musicBar + 1) % 8;
        }

        // A. Bombo Tribal Primitivo / Tambor de Marco
        // Golpes en tiempos 0, 6, 8, 12 del compás de 16 pasos
        if (m_musicStep == 0 || m_musicStep == 6 || m_musicStep == 8 || m_musicStep == 12) {
            m_musicDrumEnv = (m_musicStep == 0) ? 1.0f : 0.65f;
            m_musicDrumFreq = (m_musicStep == 0) ? 75.0f : 95.0f;
        }

        // B. Sonajero de Semillas / Cascabeles
        // Acentos en contratiempos (pasos 2, 4, 10, 14)
        if (m_musicStep == 2 || m_musicStep == 4 || m_musicStep == 10 || m_musicStep == 14) {
            m_musicRattleEnv = 0.45f;
        }

        // C. Flauta Ancestral / Ocarina de Arcilla
        // Melodía pentatónica en pasos seleccionados
        if (m_musicStep == 0 || m_musicStep == 3 || m_musicStep == 6 || m_musicStep == 10 || m_musicStep == 13) {
            int noteIdx = (m_musicBar * 2 + m_musicStep / 2) % NUM_SCALE_NOTES;
            m_musicFluteTargetFreq = PENTATONIC_SCALE[noteIdx];
            m_musicFluteEnv = 0.60f;
        }

        // D. Gong / Campana Sagrada de Templo (Sincronizado con Era 3+ / Año >= -2500)
        int year = m_currentYear.load();
        if (year >= -2500 && m_musicStep == 0 && (m_musicBar % 2 == 0)) {
            m_musicBellEnv = 0.85f;
            m_musicBellFreq = (m_musicBar == 0) ? 440.0f : 587.33f; // A4 o D5
        }
    }

    // Decaimiento de envolventes de percusión
    m_musicDrumEnv = std::max(0.0f, m_musicDrumEnv - dt * 4.5f);
    m_musicDrumFreq = std::max(42.0f, m_musicDrumFreq - dt * 220.0f);
    m_musicRattleEnv = std::max(0.0f, m_musicRattleEnv - dt * 18.0f);
    m_musicFluteEnv = std::max(0.0f, m_musicFluteEnv - dt * 1.8f);
    m_musicBellEnv = std::max(0.0f, m_musicBellEnv - dt * 0.9f);

    // Portamento suave de la flauta
    m_musicFluteFreq += (m_musicFluteTargetFreq - m_musicFluteFreq) * std::min(1.0f, dt * 12.0f);
}

void SoundSystem::synthesizeMusic(float dt, float& outL, float& outR) {
    // 1. Tambor de marco (Bombo con caída continua de frecuencia sin saltos de fase)
    float drum = 0.0f;
    if (m_musicDrumEnv > 0.001f) {
        m_musicDrumPhase += m_musicDrumFreq * dt * 2.0f * 3.14159f;
        drum = std::sin(m_musicDrumPhase) * m_musicDrumEnv;
    }

    // 2. Cascabeles / Sonajero (Ruido blanco modulado y filtrado)
    float rattle = 0.0f;
    if (m_musicRattleEnv > 0.001f) {
        rattle = fastNoise(m_noiseSeed) * m_musicRattleEnv * 0.4f;
    }

    // 3. Flauta de caña con trémolo y soplo de aire
    float flute = 0.0f;
    if (m_musicFluteEnv > 0.001f) {
        float vibrato = 1.0f + std::sin(m_musicBeatTime * 35.0f) * 0.015f;
        m_musicFlutePhase += (m_musicFluteFreq * vibrato) * dt * 2.0f * 3.14159f;
        float breath = fastNoise(m_noiseSeed) * 0.08f;
        flute = (std::sin(m_musicFlutePhase) + breath) * m_musicFluteEnv * 0.45f;
    }

    // 4. Campana / Gong Ceremonial sagrado de Templo
    float bell = 0.0f;
    if (m_musicBellEnv > 0.001f) {
        m_musicBellPhase += m_musicBellFreq * dt * 2.0f * 3.14159f;
        bell = (std::sin(m_musicBellPhase) + 0.35f * std::sin(m_musicBellPhase * 2.76f)) * m_musicBellEnv * 0.35f;
    }

    // 5. Drone armónico continuo sagrado (Bajo continuo en D2 73.4 Hz y A2 110 Hz)
    m_musicDronePhase1 += 73.42f * dt * 2.0f * 3.14159f;
    m_musicDronePhase2 += 110.00f * dt * 2.0f * 3.14159f;
    float drone = (std::sin(m_musicDronePhase1) * 0.6f + std::sin(m_musicDronePhase2) * 0.4f) * 0.16f;

    // 6. Armonía litúrgica coral para eras históricas avanzadas (año >= 0)
    float liturgical = 0.0f;
    int year = m_currentYear.load();
    if (year >= 0) {
        m_musicLiturgicalPhase1 += 220.0f * dt * 2.0f * 3.14159f; // A3
        m_musicLiturgicalPhase2 += 329.63f * dt * 2.0f * 3.14159f; // E4
        float litFactor = std::min(1.0f, static_cast<float>(year + 500) / 1500.0f);
        liturgical = (std::sin(m_musicLiturgicalPhase1) * 0.5f + std::sin(m_musicLiturgicalPhase2) * 0.5f) * 0.08f * litFactor;
    }

    // Mezcla estéreo musical con apertura panorámica
    outL = drum * 0.5f + rattle * 0.3f + flute * 0.45f + bell * 0.4f + (drone + liturgical) * 0.5f;
    outR = drum * 0.5f + rattle * 0.7f + flute * 0.55f + bell * 0.6f + (drone + liturgical) * 0.5f;
}

// =========================================================================
// SÍNTESIS DE EFECTOS DE SONIDO (SFX) Y CHOQUES
// =========================================================================
void SoundSystem::renderSFXInstance(ActiveSFXInstance& sfx, float dt, float& outL, float& outR) {
    sfx.elapsed += dt;
    float t = sfx.elapsed;
    float dur = sfx.duration;
    float progress = std::clamp(t / dur, 0.0f, 1.0f);
    float sig = 0.0f;

    switch (sfx.type) {
        case SFXType::UIClick: {
            // Clic de madera / piedra seca
            float env = std::max(0.0f, 1.0f - progress * progress);
            sig = std::sin(t * 1400.0f * sfx.pitch * 2.0f * 3.14159f) * env * 0.5f;
            break;
        }
        case SFXType::BodyThud: {
            // Choque sordo al caer cuerpo o rodar por el suelo
            float env = std::max(0.0f, 1.0f - progress);
            float freq = 85.0f * sfx.pitch * std::exp(-progress * 5.0f);
            sig = (std::sin(t * freq * 2.0f * 3.14159f) + fastNoise(m_noiseSeed) * 0.25f) * env * 0.9f;
            break;
        }
        case SFXType::WaterSplash: {
            // Zambullida y burbujas al caer al agua o nadar
            float env = std::max(0.0f, 1.0f - progress);
            float bubble = std::sin(t * (240.0f + progress * 320.0f) * sfx.pitch * 2.0f * 3.14159f);
            float splashNoise = fastNoise(m_noiseSeed) * 0.4f;
            sig = (bubble * 0.6f + splashNoise) * env * 0.8f;
            break;
        }
        case SFXType::TreeChop: {
            // Hachazo contra tronco de madera
            float env = std::max(0.0f, 1.0f - progress * 1.8f);
            float woodThump = std::sin(t * 220.0f * sfx.pitch * 2.0f * 3.14159f);
            float chipClick = fastNoise(m_noiseSeed) * 0.35f;
            sig = (woodThump + chipClick) * env * 0.95f;
            break;
        }
        case SFXType::StoneMine: {
            // Golpe de pico en roca / piedra
            float env = std::max(0.0f, 1.0f - progress);
            float ping = std::sin(t * 1650.0f * sfx.pitch * 2.0f * 3.14159f) * 0.6f +
                         std::sin(t * 3100.0f * sfx.pitch * 2.0f * 3.14159f) * 0.4f;
            sig = (ping + fastNoise(m_noiseSeed) * 0.25f) * env * 0.85f;
            break;
        }
        case SFXType::GatherFood: {
            // Crujido suave de hojas y ramas al forrajear
            float env = std::sin(progress * 3.14159f);
            sig = fastNoise(m_noiseSeed) * env * 0.45f;
            break;
        }
        case SFXType::ThunderStrike: {
            // Trueno sagrado y rayo divino
            float snap = (progress < 0.08f) ? (fastNoise(m_noiseSeed) * 1.5f) : 0.0f;
            float rumble = std::sin(t * 45.0f * 2.0f * 3.14159f) * fastNoise(m_noiseSeed);
            float env = std::max(0.0f, 1.0f - progress * 0.6f);
            sig = (snap + rumble * 0.85f) * env * 1.2f;
            break;
        }
        case SFXType::MeteorDescent: {
            // Rugido ultrasónico descendente
            float env = std::min(1.0f, progress * 3.0f) * std::max(0.0f, 1.0f - progress);
            float roar = fastNoise(m_noiseSeed) * 0.7f + std::sin(t * 110.0f * 2.0f * 3.14159f) * 0.3f;
            sig = roar * env * 1.1f;
            break;
        }
        case SFXType::MeteorImpact: {
            // Explosión cataclísmica de bólido con temblor de tierra
            float blast = (progress < 0.12f) ? (fastNoise(m_noiseSeed) * 1.8f) : 0.0f;
            float subRumble = std::sin(t * (35.0f + 20.0f * (1.0f - progress)) * 2.0f * 3.14159f);
            float env = std::max(0.0f, 1.0f - progress * 0.45f);
            sig = (blast + subRumble * 1.2f + fastNoise(m_noiseSeed) * 0.5f) * env * 1.3f;
            break;
        }
        case SFXType::Earthquake: {
            // Retumbar telúrico subterráneo
            float env = std::sin(progress * 3.14159f);
            float tectonics = std::sin(t * 28.0f * 2.0f * 3.14159f) * 0.8f + fastNoise(m_noiseSeed) * 0.4f;
            sig = tectonics * env * 0.95f;
            break;
        }
        case SFXType::TornadoWhoosh: {
            // Vórtice de ciclón
            float env = std::sin(progress * 3.14159f);
            float swirl = fastNoise(m_noiseSeed) * (0.6f + 0.4f * std::sin(t * 8.0f));
            sig = swirl * env * 0.9f;
            break;
        }
        case SFXType::PlagueSwarm: {
            // Zumbido ponzoñoso de langostas / insectos venenosos
            float env = std::sin(progress * 3.14159f);
            float buzz = std::sin(t * 380.0f * 2.0f * 3.14159f) * fastNoise(m_noiseSeed);
            sig = buzz * env * 0.75f;
            break;
        }
        case SFXType::PrayerChant: {
            // Oración mística y zumbido armónico
            float env = std::sin(progress * 3.14159f);
            float chant = std::sin(t * 110.0f * 2.0f * 3.14159f) * 0.5f +
                          std::sin(t * 220.0f * 2.0f * 3.14159f) * 0.3f +
                          std::sin(t * 330.0f * 2.0f * 3.14159f) * 0.2f;
            sig = chant * env * 0.7f;
            break;
        }
        case SFXType::FaithGain: {
            // Campanillas divinas ascendentes
            float env = std::max(0.0f, 1.0f - progress);
            int step = std::clamp(static_cast<int>(progress * 4.0f), 0, 3);
            float freqs[4] = {659.25f, 783.99f, 987.77f, 1318.51f}; // E5, G5, B5, E6
            sig = std::sin(t * freqs[step] * 2.0f * 3.14159f) * env * 0.45f;
            break;
        }
        case SFXType::EraAscension: {
            // Cuerno neolítico y gong de bronce solemne
            float env = std::max(0.0f, 1.0f - progress * 0.35f);
            float horn = std::sin(t * 146.83f * 2.0f * 3.14159f) * 0.6f +
                         std::sin(t * 220.00f * 2.0f * 3.14159f) * 0.4f;
            sig = horn * env * 0.85f;
            break;
        }
        case SFXType::CombatAttack: {
            // Mandoble / garrotazo de combate
            float env = std::max(0.0f, 1.0f - progress * 3.5f);
            float swing = std::sin(t * 180.0f * sfx.pitch * 2.0f * 3.14159f) + fastNoise(m_noiseSeed) * 0.5f;
            sig = swing * env * 0.8f;
            break;
        }
        case SFXType::UnitDeath: {
            // Suspiro o caída de unidad mortal
            float env = std::max(0.0f, 1.0f - progress);
            float sigh = std::sin(t * (180.0f - progress * 80.0f) * 2.0f * 3.14159f);
            sig = sigh * env * 0.6f;
            break;
        }
    }

    // Paneo estéreo de potencia constante
    float panNorm = (sfx.pan + 1.0f) * 0.5f; // 0.0 (izq) a 1.0 (der)
    float panL = std::cos(panNorm * 1.57079f);
    float panR = std::sin(panNorm * 1.57079f);

    outL = sig * sfx.volume * panL;
    outR = sig * sfx.volume * panR;
}

} // namespace ReligionSim
