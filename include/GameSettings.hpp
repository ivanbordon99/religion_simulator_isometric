#pragma once

// EN: Comprehensive Game Settings and Configuration Architecture
// ES: Arquitectura Integral de Configuración y Opciones del Juego

#include "Types.hpp"
#include <string>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cstdint>

namespace ReligionSim {

// =============================================================================
// EN: Display and Video Configuration Enums
// ES: Enumeraciones de Configuración de Pantalla y Video
// =============================================================================
enum class DisplayResolution {
    Res1280x720,  // HD 720p (Base canonical resolution)
    Res1600x900,  // HD+ 900p (Widescreen balanced)
    Res1920x1080  // Full HD 1080p (High-density presentation)
};

enum class DisplayMode {
    Windowed,            // Modo Ventana estándar
    BorderlessFullscreen // Pantalla Completa sin bordes
};

enum class FrameRateLimit {
    FPS30,     // 30 FPS (Ahorro de energía)
    FPS60,     // 60 FPS (Estándar fluido)
    FPS120,    // 120 FPS (Alta tasa de refresco)
    Unlimited  // Sin límite de cuadros
};

enum class UiScale {
    Scale100, // 100% Escala normal
    Scale125, // 125% Escala ampliada
    Scale150  // 150% Escala grande
};

enum class BrightnessLevel {
    Bright80,  // 80% Tenue / Nocturno
    Bright100, // 100% Calibrado estándar
    Bright120, // 120% Brillante
    Bright140  // 140% Radiante / Solar
};

// =============================================================================
// EN: Graphics and Visual / Image Configuration Enums
// ES: Enumeraciones de Configuración de Gráficos e Imagen
// =============================================================================
enum class CloudShadowsQuality {
    Disabled, // Desactivadas
    Classic,  // Clásicas nítidas
    HighSoft  // Alta definición con difuminado
};

enum class DynamicLightingMode {
    Disabled,          // Iluminación fija plana
    SoftRealistic,     // Transiciones suaves realistas
    VibrantLiturgical  // Sombras y tonalidades litúrgicas marcadas
};

enum class WaterAnimationDetail {
    Static,             // Agua estática
    Standard,           // Oleaje estándar animado
    DetailedReflections // Oleaje con reflejos y crestas translúcidas
};

enum class ScreenShakeIntensity {
    Off,     // 0% Sin sacudida
    Reduced, // 50% Sacudida atenuada
    Full     // 100% Impacto sísmico total
};

enum class VisualColorFilter {
    Neutral,        // Colores naturales sin teñido
    DivineGold,     // Tonalidad cálida dorada sacra
    SepiaParchment, // Pergamino litúrgico antiguo
    BasaltCold      // Basalto místico frío
};

// =============================================================================
// EN: Gameplay and Simulation Enums
// ES: Enumeraciones de Configuración de Jugabilidad y Simulación
// =============================================================================
enum class DefaultSimulationSpeed {
    PacingHalf,   // 0.5x (2 años por segundo)
    PacingNormal, // 1.0x (4 años por segundo)
    PacingFast,   // 2.0x (8 años por segundo)
    PacingEpic    // 4.0x (16 años por segundo)
};

enum class CameraPanSpeed {
    PanSlow,   // Desplazamiento lento y cinemático
    PanNormal, // Desplazamiento equilibrado RTS
    PanFast    // Desplazamiento rápido dinámico
};

enum class LifeEventNotifications {
    AllEvents, // Todos los nacimientos y muertes
    MajorOnly, // Solo líderes, chamanes y profetas
    Minimal    // Solo hitos de cambio de era
};

// =============================================================================
// EN: Options Menu Navigation Tabs
// ES: Pestañas de Navegación del Menú de Opciones
// =============================================================================
enum class OptionsTab {
    ScreenAndVideo,        // Pantalla y Video
    GraphicsAndImage,      // Gráficos e Imagen
    GameplayAndSimulation, // Jugabilidad y Simulación
    AudioAndSound          // Audio y Sonido
};

// =============================================================================
// EN: Grouped Settings Struct
// ES: Estructura Agrupada de Ajustes y Opciones
// =============================================================================
struct GameSettings {
    // 1. Pantalla y Video
    DisplayResolution resolution{DisplayResolution::Res1280x720};
    DisplayMode displayMode{DisplayMode::BorderlessFullscreen}; // EN: Fullscreen by default / ES: Pantalla completa por defecto
    FrameRateLimit frameRateLimit{FrameRateLimit::FPS60};
    UiScale uiScale{UiScale::Scale100};
    BrightnessLevel brightness{BrightnessLevel::Bright100};
    bool verticalSync{true};

    // 2. Gráficos e Imagen
    CloudShadowsQuality cloudShadows{CloudShadowsQuality::HighSoft};
    DynamicLightingMode dynamicLighting{DynamicLightingMode::SoftRealistic};
    WaterAnimationDetail waterAnimation{WaterAnimationDetail::DetailedReflections};
    bool vignetteEnabled{true};
    ScreenShakeIntensity screenShake{ScreenShakeIntensity::Full};
    VisualColorFilter colorFilter{VisualColorFilter::Neutral};
    bool showIsometricGrid{false}; // EN: Terrain isometric grid disabled by default / ES: Retícula isométrica de terreno desactivada por defecto
    bool developerMode{false};    // EN: Developer mode telemetry and axes overlay / ES: Capa de telemetría y ejes del modo desarrollador

    // 3. Jugabilidad y Simulación
    DefaultSimulationSpeed defaultSpeed{DefaultSimulationSpeed::PacingNormal};
    bool autoPauseOnDisasters{true};
    CameraPanSpeed cameraPanSpeed{CameraPanSpeed::PanNormal};
    bool invertCameraDrag{false};
    bool showFloatingNumbers{true};
    LifeEventNotifications eventNotifications{LifeEventNotifications::AllEvents};

    // 4. Audio y Sonido
    int masterVolumePercent{80};  // 0 .. 100%
    int musicVolumePercent{70};   // 0 .. 100%
    int sfxVolumePercent{85};     // 0 .. 100%
    int ambientVolumePercent{75}; // 0 .. 100%
    bool soundMuted{false};

    // =========================================================================
    // EN: Helper Resolution Conversion Routines
    // ES: Rutinas Auxiliares de Conversión de Resolución
    // =========================================================================
    static int getResolutionWidth(DisplayResolution targetResolution) {
        switch (targetResolution) {
            case DisplayResolution::Res1280x720:  return 1280;
            case DisplayResolution::Res1600x900:  return 1600;
            case DisplayResolution::Res1920x1080: return 1920;
        }
        return 1280;
    }

    static int getResolutionHeight(DisplayResolution targetResolution) {
        switch (targetResolution) {
            case DisplayResolution::Res1280x720:  return 720;
            case DisplayResolution::Res1600x900:  return 900;
            case DisplayResolution::Res1920x1080: return 1080;
        }
        return 720;
    }

    static std::string getResolutionName(DisplayResolution targetResolution) {
        switch (targetResolution) {
            case DisplayResolution::Res1280x720:  return "1280 x 720 (HD)";
            case DisplayResolution::Res1600x900:  return "1600 x 900 (HD+)";
            case DisplayResolution::Res1920x1080: return "1920 x 1080 (FHD)";
        }
        return "1280 x 720";
    }

    static std::string getDisplayModeName(DisplayMode targetMode) {
        switch (targetMode) {
            case DisplayMode::Windowed:            return "VENTANA";
            case DisplayMode::BorderlessFullscreen: return "PANTALLA COMPLETA";
        }
        return "VENTANA";
    }

    static std::string getFrameRateLimitName(FrameRateLimit targetLimit) {
        switch (targetLimit) {
            case FrameRateLimit::FPS30:     return "30 FPS";
            case FrameRateLimit::FPS60:     return "60 FPS";
            case FrameRateLimit::FPS120:    return "120 FPS";
            case FrameRateLimit::Unlimited: return "ILIMITADO";
        }
        return "60 FPS";
    }

    static std::string getUiScaleName(UiScale targetScale) {
        switch (targetScale) {
            case UiScale::Scale100: return "100% (NORMAL)";
            case UiScale::Scale125: return "125% (AMPLIADO)";
            case UiScale::Scale150: return "150% (GRANDE)";
        }
        return "100%";
    }

    static std::string getBrightnessName(BrightnessLevel targetBrightness) {
        switch (targetBrightness) {
            case BrightnessLevel::Bright80:  return "80% (TENUE)";
            case BrightnessLevel::Bright100: return "100% (NORMAL)";
            case BrightnessLevel::Bright120: return "120% (BRILLANTE)";
            case BrightnessLevel::Bright140: return "140% (RADIANTE)";
        }
        return "100%";
    }

    static float getBrightnessFactor(BrightnessLevel targetBrightness) {
        switch (targetBrightness) {
            case BrightnessLevel::Bright80:  return 0.80f;
            case BrightnessLevel::Bright100: return 1.00f;
            case BrightnessLevel::Bright120: return 1.20f;
            case BrightnessLevel::Bright140: return 1.40f;
        }
        return 1.00f;
    }

    static std::string getCloudShadowsName(CloudShadowsQuality targetQuality) {
        switch (targetQuality) {
            case CloudShadowsQuality::Disabled: return "DESACTIVADAS";
            case CloudShadowsQuality::Classic:  return "CLASICAS";
            case CloudShadowsQuality::HighSoft:  return "ALTAS (SUAVE)";
        }
        return "ALTAS";
    }

    static std::string getDynamicLightingName(DynamicLightingMode targetLighting) {
        switch (targetLighting) {
            case DynamicLightingMode::Disabled:          return "DESACTIVADA";
            case DynamicLightingMode::SoftRealistic:     return "SUAVE REALISTA";
            case DynamicLightingMode::VibrantLiturgical:  return "LITURGICA MARCADA";
        }
        return "SUAVE REALISTA";
    }

    static std::string getWaterAnimationName(WaterAnimationDetail targetDetail) {
        switch (targetDetail) {
            case WaterAnimationDetail::Static:              return "ESTATICA";
            case WaterAnimationDetail::Standard:            return "ESTANDAR";
            case WaterAnimationDetail::DetailedReflections: return "CON REFLEJOS";
        }
        return "CON REFLEJOS";
    }

    static std::string getScreenShakeName(ScreenShakeIntensity targetIntensity) {
        switch (targetIntensity) {
            case ScreenShakeIntensity::Off:     return "0% (DESACTIVADA)";
            case ScreenShakeIntensity::Reduced: return "50% (ATENUADA)";
            case ScreenShakeIntensity::Full:    return "100% (TOTAL)";
        }
        return "100%";
    }

    static float getScreenShakeFactor(ScreenShakeIntensity targetIntensity) {
        switch (targetIntensity) {
            case ScreenShakeIntensity::Off:     return 0.0f;
            case ScreenShakeIntensity::Reduced: return 0.5f;
            case ScreenShakeIntensity::Full:    return 1.0f;
        }
        return 1.0f;
    }

    static std::string getVisualColorFilterName(VisualColorFilter targetFilter) {
        switch (targetFilter) {
            case VisualColorFilter::Neutral:        return "NEUTRO NATURAL";
            case VisualColorFilter::DivineGold:     return "DORADO SACRO";
            case VisualColorFilter::SepiaParchment: return "PERGAMINO ANTIGUO";
            case VisualColorFilter::BasaltCold:     return "BASALTO MISTICO";
        }
        return "NEUTRO NATURAL";
    }

    static std::string getSimulationSpeedName(DefaultSimulationSpeed targetSpeed) {
        switch (targetSpeed) {
            case DefaultSimulationSpeed::PacingHalf:   return "0.5x (2 a/s)";
            case DefaultSimulationSpeed::PacingNormal: return "1.0x (4 a/s)";
            case DefaultSimulationSpeed::PacingFast:   return "2.0x (8 a/s)";
            case DefaultSimulationSpeed::PacingEpic:   return "4.0x (16 a/s)";
        }
        return "1.0x (4 a/s)";
    }

    static float getSimulationSpeedValue(DefaultSimulationSpeed targetSpeed) {
        switch (targetSpeed) {
            case DefaultSimulationSpeed::PacingHalf:   return 2.0f;
            case DefaultSimulationSpeed::PacingNormal: return 4.0f;
            case DefaultSimulationSpeed::PacingFast:   return 8.0f;
            case DefaultSimulationSpeed::PacingEpic:   return 16.0f;
        }
        return 4.0f;
    }

    static std::string getCameraPanSpeedName(CameraPanSpeed targetSpeed) {
        switch (targetSpeed) {
            case CameraPanSpeed::PanSlow:   return "SUAVE CINEMATICA";
            case CameraPanSpeed::PanNormal: return "NORMAL RTS";
            case CameraPanSpeed::PanFast:   return "RAPIDA DINAMICA";
        }
        return "NORMAL RTS";
    }

    static float getCameraPanSpeedFactor(CameraPanSpeed targetSpeed) {
        switch (targetSpeed) {
            case CameraPanSpeed::PanSlow:   return 0.70f;
            case CameraPanSpeed::PanNormal: return 1.00f;
            case CameraPanSpeed::PanFast:   return 1.50f;
        }
        return 1.00f;
    }

    static std::string getLifeEventNotificationsName(LifeEventNotifications targetNotif) {
        switch (targetNotif) {
            case LifeEventNotifications::AllEvents: return "TODOS";
            case LifeEventNotifications::MajorOnly: return "SOLO LIDERES";
            case LifeEventNotifications::Minimal:   return "MINIMAS";
        }
        return "TODOS";
    }

    // =========================================================================
    // EN: Value Cycling Actions for Options Menu Interactions
    // ES: Acciones de Ciclo de Valores para Interacciones del Menú de Opciones
    // =========================================================================
    void cycleResolution() {
        if (resolution == DisplayResolution::Res1280x720) resolution = DisplayResolution::Res1600x900;
        else if (resolution == DisplayResolution::Res1600x900) resolution = DisplayResolution::Res1920x1080;
        else resolution = DisplayResolution::Res1280x720;
    }

    void cycleDisplayMode() {
        displayMode = (displayMode == DisplayMode::Windowed)
                      ? DisplayMode::BorderlessFullscreen
                      : DisplayMode::Windowed;
    }

    void cycleFrameRateLimit() {
        if (frameRateLimit == FrameRateLimit::FPS30) frameRateLimit = FrameRateLimit::FPS60;
        else if (frameRateLimit == FrameRateLimit::FPS60) frameRateLimit = FrameRateLimit::FPS120;
        else if (frameRateLimit == FrameRateLimit::FPS120) frameRateLimit = FrameRateLimit::Unlimited;
        else frameRateLimit = FrameRateLimit::FPS30;
    }

    void cycleUiScale() {
        if (uiScale == UiScale::Scale100) uiScale = UiScale::Scale125;
        else if (uiScale == UiScale::Scale125) uiScale = UiScale::Scale150;
        else uiScale = UiScale::Scale100;
    }

    void cycleBrightness() {
        if (brightness == BrightnessLevel::Bright80) brightness = BrightnessLevel::Bright100;
        else if (brightness == BrightnessLevel::Bright100) brightness = BrightnessLevel::Bright120;
        else if (brightness == BrightnessLevel::Bright120) brightness = BrightnessLevel::Bright140;
        else brightness = BrightnessLevel::Bright80;
    }

    void toggleVerticalSync() {
        verticalSync = !verticalSync;
    }

    void cycleCloudShadows() {
        if (cloudShadows == CloudShadowsQuality::Disabled) cloudShadows = CloudShadowsQuality::Classic;
        else if (cloudShadows == CloudShadowsQuality::Classic) cloudShadows = CloudShadowsQuality::HighSoft;
        else cloudShadows = CloudShadowsQuality::Disabled;
    }

    void cycleDynamicLighting() {
        if (dynamicLighting == DynamicLightingMode::Disabled) dynamicLighting = DynamicLightingMode::SoftRealistic;
        else if (dynamicLighting == DynamicLightingMode::SoftRealistic) dynamicLighting = DynamicLightingMode::VibrantLiturgical;
        else dynamicLighting = DynamicLightingMode::Disabled;
    }

    void cycleWaterAnimation() {
        if (waterAnimation == WaterAnimationDetail::Static) waterAnimation = WaterAnimationDetail::Standard;
        else if (waterAnimation == WaterAnimationDetail::Standard) waterAnimation = WaterAnimationDetail::DetailedReflections;
        else waterAnimation = WaterAnimationDetail::Static;
    }

    void toggleVignette() {
        vignetteEnabled = !vignetteEnabled;
    }

    void cycleScreenShake() {
        if (screenShake == ScreenShakeIntensity::Off) screenShake = ScreenShakeIntensity::Reduced;
        else if (screenShake == ScreenShakeIntensity::Reduced) screenShake = ScreenShakeIntensity::Full;
        else screenShake = ScreenShakeIntensity::Off;
    }

    void cycleVisualColorFilter() {
        if (colorFilter == VisualColorFilter::Neutral) colorFilter = VisualColorFilter::DivineGold;
        else if (colorFilter == VisualColorFilter::DivineGold) colorFilter = VisualColorFilter::SepiaParchment;
        else if (colorFilter == VisualColorFilter::SepiaParchment) colorFilter = VisualColorFilter::BasaltCold;
        else colorFilter = VisualColorFilter::Neutral;
    }

    // EN: Toggle terrain isometric grid visualization on and off
    // ES: Alternar visualización de retícula isométrica de terreno encendida y apagada
    void toggleIsometricGrid() {
        showIsometricGrid = !showIsometricGrid;
    }

    // EN: Toggle developer mode telemetry and axes on and off
    // ES: Alternar telemetría y ejes del modo desarrollador encendidos y apagados
    void toggleDeveloperMode() {
        developerMode = !developerMode;
    }

    void cycleSimulationSpeed() {
        if (defaultSpeed == DefaultSimulationSpeed::PacingHalf) defaultSpeed = DefaultSimulationSpeed::PacingNormal;
        else if (defaultSpeed == DefaultSimulationSpeed::PacingNormal) defaultSpeed = DefaultSimulationSpeed::PacingFast;
        else if (defaultSpeed == DefaultSimulationSpeed::PacingFast) defaultSpeed = DefaultSimulationSpeed::PacingEpic;
        else defaultSpeed = DefaultSimulationSpeed::PacingHalf;
    }

    void toggleAutoPauseOnDisasters() {
        autoPauseOnDisasters = !autoPauseOnDisasters;
    }

    void cycleCameraPanSpeed() {
        if (cameraPanSpeed == CameraPanSpeed::PanSlow) cameraPanSpeed = CameraPanSpeed::PanNormal;
        else if (cameraPanSpeed == CameraPanSpeed::PanNormal) cameraPanSpeed = CameraPanSpeed::PanFast;
        else cameraPanSpeed = CameraPanSpeed::PanSlow;
    }

    void toggleInvertCameraDrag() {
        invertCameraDrag = !invertCameraDrag;
    }

    void toggleFloatingNumbers() {
        showFloatingNumbers = !showFloatingNumbers;
    }

    void cycleLifeEventNotifications() {
        if (eventNotifications == LifeEventNotifications::AllEvents) eventNotifications = LifeEventNotifications::MajorOnly;
        else if (eventNotifications == LifeEventNotifications::MajorOnly) eventNotifications = LifeEventNotifications::Minimal;
        else eventNotifications = LifeEventNotifications::AllEvents;
    }

    void cycleMasterVolume() {
        masterVolumePercent = (masterVolumePercent + 25) % 125;
    }

    void cycleMusicVolume() {
        musicVolumePercent = (musicVolumePercent + 25) % 125;
    }

    void cycleSfxVolume() {
        sfxVolumePercent = (sfxVolumePercent + 25) % 125;
    }

    void cycleAmbientVolume() {
        ambientVolumePercent = (ambientVolumePercent + 25) % 125;
    }

    void toggleSoundMute() {
        soundMuted = !soundMuted;
    }

    void resetToDefaults() {
        *this = GameSettings{};
    }

    // =========================================================================
    // EN: Settings File Serialization and Deserialization
    // ES: Serialización y Deserialización de Archivos de Ajustes
    // =========================================================================
    bool saveToFile(const std::string& targetFilePath) const {
        std::ofstream targetFile(targetFilePath);
        if (!targetFile.is_open()) return false;

        targetFile << "# MARANGATU SETTINGS CONFIGURATION\n";
        targetFile << "resolution=" << static_cast<int>(resolution) << "\n";
        targetFile << "displayMode=" << static_cast<int>(displayMode) << "\n";
        targetFile << "frameRateLimit=" << static_cast<int>(frameRateLimit) << "\n";
        targetFile << "uiScale=" << static_cast<int>(uiScale) << "\n";
        targetFile << "brightness=" << static_cast<int>(brightness) << "\n";
        targetFile << "verticalSync=" << (verticalSync ? 1 : 0) << "\n";

        targetFile << "cloudShadows=" << static_cast<int>(cloudShadows) << "\n";
        targetFile << "dynamicLighting=" << static_cast<int>(dynamicLighting) << "\n";
        targetFile << "waterAnimation=" << static_cast<int>(waterAnimation) << "\n";
        targetFile << "vignetteEnabled=" << (vignetteEnabled ? 1 : 0) << "\n";
        targetFile << "screenShake=" << static_cast<int>(screenShake) << "\n";
        targetFile << "colorFilter=" << static_cast<int>(colorFilter) << "\n";
        targetFile << "showIsometricGrid=" << (showIsometricGrid ? 1 : 0) << "\n";
        targetFile << "developerMode=" << (developerMode ? 1 : 0) << "\n";

        targetFile << "defaultSpeed=" << static_cast<int>(defaultSpeed) << "\n";
        targetFile << "autoPauseOnDisasters=" << (autoPauseOnDisasters ? 1 : 0) << "\n";
        targetFile << "cameraPanSpeed=" << static_cast<int>(cameraPanSpeed) << "\n";
        targetFile << "invertCameraDrag=" << (invertCameraDrag ? 1 : 0) << "\n";
        targetFile << "showFloatingNumbers=" << (showFloatingNumbers ? 1 : 0) << "\n";
        targetFile << "eventNotifications=" << static_cast<int>(eventNotifications) << "\n";

        targetFile << "masterVolumePercent=" << masterVolumePercent << "\n";
        targetFile << "musicVolumePercent=" << musicVolumePercent << "\n";
        targetFile << "sfxVolumePercent=" << sfxVolumePercent << "\n";
        targetFile << "ambientVolumePercent=" << ambientVolumePercent << "\n";
        targetFile << "soundMuted=" << (soundMuted ? 1 : 0) << "\n";

        return true;
    }

    bool loadFromFile(const std::string& sourceFilePath) {
        std::ifstream sourceFile(sourceFilePath);
        if (!sourceFile.is_open()) return false;

        std::string currentLine;
        while (std::getline(sourceFile, currentLine)) {
            if (currentLine.empty() || currentLine[0] == '#') continue;

            const size_t delimiterIndex = currentLine.find('=');
            if (delimiterIndex == std::string::npos) continue;

            const std::string key = currentLine.substr(0, delimiterIndex);
            const std::string value = currentLine.substr(delimiterIndex + 1);
            const int numericValue = std::stoi(value);

            if (key == "resolution") resolution = static_cast<DisplayResolution>(numericValue);
            else if (key == "displayMode") displayMode = static_cast<DisplayMode>(numericValue);
            else if (key == "frameRateLimit") frameRateLimit = static_cast<FrameRateLimit>(numericValue);
            else if (key == "uiScale") uiScale = static_cast<UiScale>(numericValue);
            else if (key == "brightness") brightness = static_cast<BrightnessLevel>(numericValue);
            else if (key == "verticalSync") verticalSync = (numericValue != 0);

            else if (key == "cloudShadows") cloudShadows = static_cast<CloudShadowsQuality>(numericValue);
            else if (key == "dynamicLighting") dynamicLighting = static_cast<DynamicLightingMode>(numericValue);
            else if (key == "waterAnimation") waterAnimation = static_cast<WaterAnimationDetail>(numericValue);
            else if (key == "vignetteEnabled") vignetteEnabled = (numericValue != 0);
            else if (key == "screenShake") screenShake = static_cast<ScreenShakeIntensity>(numericValue);
            else if (key == "colorFilter") colorFilter = static_cast<VisualColorFilter>(numericValue);
            else if (key == "showIsometricGrid") showIsometricGrid = (numericValue != 0);
            else if (key == "developerMode") developerMode = (numericValue != 0);

            else if (key == "defaultSpeed") defaultSpeed = static_cast<DefaultSimulationSpeed>(numericValue);
            else if (key == "autoPauseOnDisasters") autoPauseOnDisasters = (numericValue != 0);
            else if (key == "cameraPanSpeed") cameraPanSpeed = static_cast<CameraPanSpeed>(numericValue);
            else if (key == "invertCameraDrag") invertCameraDrag = (numericValue != 0);
            else if (key == "showFloatingNumbers") showFloatingNumbers = (numericValue != 0);
            else if (key == "eventNotifications") eventNotifications = static_cast<LifeEventNotifications>(numericValue);

            else if (key == "masterVolumePercent") masterVolumePercent = std::clamp(numericValue, 0, 100);
            else if (key == "musicVolumePercent") musicVolumePercent = std::clamp(numericValue, 0, 100);
            else if (key == "sfxVolumePercent") sfxVolumePercent = std::clamp(numericValue, 0, 100);
            else if (key == "ambientVolumePercent") ambientVolumePercent = std::clamp(numericValue, 0, 100);
            else if (key == "soundMuted") soundMuted = (numericValue != 0);
        }

        return true;
    }
};

} // namespace ReligionSim
