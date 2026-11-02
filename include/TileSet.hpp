#pragma once

#include "Types.hpp"
#include <string>
#include <vector>
#include <unordered_map>

namespace ReligionSim {

// EN: Human sprite actions for character animation
// ES: Acciones de sprites humanos para animación de personajes
enum class HumanAction : uint8_t {
    IDLE = 0,        // EN: Standing idle / ES: De pie descansando
    WALK = 1,        // EN: Walking forward / ES: Caminando hacia adelante
    RUN = 2,         // EN: Running fast / ES: Corriendo veloz
    CHOP = 3,        // EN: Chopping wood with axe / ES: Golpe descendente con hacha de piedra
    GATHER = 4,      // EN: Gathering forage / ES: Recolectando / inclinándose con herramienta
    CARRY = 5,       // EN: Carrying burden / ES: Transportando leños o reliquias sagradas
    PRAY = 6,        // EN: Praying before altar / ES: En cuclillas / orando ante el tótem o fogata
    ATTACK = 7,      // EN: Combat attack strike / ES: En guardia / golpe frontal con mazo
    CELEBRATE = 8,   // EN: Victory celebration / ES: Celebración / brazos en alto / victoria
    DIE = 9,         // EN: Dead on ground / ES: Tendido en el suelo inerte
    HURT = 10,       // EN: Staggering hurt / ES: Tambaleándose / noqueado

    // EN: Compatibility aliases with previous identifiers
    // ES: Alias de compatibilidad con identificadores anteriores
    CAMINAR = WALK,
    CORRER = RUN,
    TALAR = CHOP,
    RECOLECTAR = GATHER,
    CARGAR = CARRY,
    REZAR = PRAY,
    ATACAR = ATTACK,
    SALUDAR = CELEBRATE,
    MORIR = DIE,
    HERIDO = HURT
};
using AccionHumano = HumanAction;

struct TileRect {
    int x{0};
    int y{0};
    int width{0};
    int height{0};
};

class TileSet {
public:
    TileSet();

    // EN: Load tileset texture data from binary RGBA or image file
    // ES: Carga la imagen del tileset desde archivo (RGBA binario o genera datos)
    bool loadFromFile(const std::string& filePath);
    inline bool cargarDesdeArchivo(const std::string& filePath) { return loadFromFile(filePath); }

    int getWidth() const { return m_width; }
    inline int getAncho() const { return getWidth(); }

    int getHeight() const { return m_height; }
    inline int getAlto() const { return getHeight(); }

    const std::vector<uint32_t>& getPixels() const { return m_pixels; }
    inline const std::vector<uint32_t>& getPixeles() const { return getPixels(); }

    uint32_t getPixel(int pixelX, int pixelY) const {
        if (pixelX < 0 || pixelX >= m_width || pixelY < 0 || pixelY >= m_height) return 0;
        return m_pixels[pixelY * m_width + pixelX];
    }

    // EN: Query bounding rectangle for specified action and isometric direction
    // ES: Obtiene el rectángulo exacto dentro del tileset para una acción y dirección
    TileRect getRectangle(HumanAction action, IsoDirection direction) const;
    inline TileRect obtenerRectangulo(HumanAction action, IsoDirection direction) const {
        return getRectangle(action, direction);
    }

    // EN: Configure sprite bounds mapping for female villager sheet
    // ES: Configura el mapeo de sprites para la hoja femenina
    void initializeFemaleRectangleMap();
    inline void inicializarMapaRectangulosFemenino() { initializeFemaleRectangleMap(); }

    // EN: Configure standard sprite bounds mapping
    // ES: Configura el mapeo de sprites estándar
    void initializeRectangleMap();
    inline void inicializarMapaRectangulos() { initializeRectangleMap(); }

    // EN: Blit tile with alpha blending, scaling, and selection outline
    // ES: Dibuja el tile sobre un framebuffer destino con mezcla alfa y soporte de escala
    void blitTile(uint32_t* dstBuffer, int dstW, int dstH,
                  const TileRect& rect, float footScreenX, float footScreenY,
                  float renderScale, ColorRGB auraColor, bool isSelected = false) const;

    // EN: Blit rotated tile (for ragdoll tumble and bodies at rest)
    // ES: Dibuja el tile rotado (para física de ragdoll y reposo)
    void blitTileRotated(uint32_t* dstBuffer, int dstW, int dstH,
                         const TileRect& rect, float centerScreenX, float centerScreenY,
                         float rotationAngleRad, float renderScale, ColorRGB auraColor,
                         float alpha = 1.0f, bool submergedInWater = false) const;

    // EN: Hash action and direction into lookup key
    // ES: Clave hash para acción y dirección
    int hashActionDirection(HumanAction action, IsoDirection direction) const {
        return (static_cast<int>(action) << 8) | static_cast<int>(direction);
    }
    inline int hashAccionDir(HumanAction action, IsoDirection direction) const {
        return hashActionDirection(action, direction);
    }

private:
    int m_width{1024};
    int m_height{558};
    std::vector<uint32_t> m_pixels;
    std::unordered_map<int, TileRect> m_rectMap;
};

} // namespace ReligionSim
