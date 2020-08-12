
#ifndef GAME_TILEMAP_H
#define GAME_TILEMAP_H

#include <vector>
#include <list>
#include <functional>
#include <utils/math_utils.h>
#include "TileMapOutliner.h"
#include "../ecs/systems/EntitySystem.h"
#include "../../macro_magic/serializable.h"
#include "../../rendering/room/tile_map/TileSet.h"
#include "WindMap.h"

SERIALIZABLE(
    tile_update,
    FIELD(uint8, x),
    FIELD(uint8, y),
    FIELD(uint8, newTile),
    FIELD(uint8, newTileMaterial)
)
END_SERIALIZABLE(tile_update)

enum class Tile : unsigned char
{
    /*                                      ___               ___        _______
     *     ___      |\           /|         \  |             |  /
     *    |   |     | \         / |          \ |             | /
     *    |___|     |__\       /__|           \|             |/
     */
    empty, full, slope_down, slope_up, sloped_ceil_down, sloped_ceil_up, platform,

    // same as slope_down, but spread over 2 tiles:
    slope_down_half0, slope_down_half1,

    // same as slope_up, but spread over 2 tiles:
    slope_up_half0, slope_up_half1,
};
typedef uint8 TileMaterial;

SERIALIZABLE(
    TileMaterialProperties,
    FIELD(std::string, name),
    FIELD(asset<TileSet>, tileSet),
    FIELD_DEF_VAL(float, friction, 1),
    FIELD_DEF_VAL(float, bounciness, 0),
    FIELD(vec3, color)
)
END_SERIALIZABLE_FULL_JSON(TileMaterialProperties)

typedef std::vector<std::pair<vec2, vec2>> TileMapOutlines;

class TileMap
{

    Tile *tiles;
    TileMaterial *tileMaterials;

    TileMapOutlines outlines;

    std::list<tile_update> tileUpdatesSinceLastUpdate, tileUpdatesPrevUpdate;
    friend class Room;

    const std::vector<TileMaterialProperties> materialProperties;

  public:

    static const int PIXELS_PER_TILE = 16;

    constexpr const static auto TILE_TYPES = {
            Tile::empty, Tile::full,
            Tile::slope_down, Tile::slope_up, Tile::sloped_ceil_down, Tile::sloped_ceil_up,
            Tile::platform,
            Tile::slope_down_half0, Tile::slope_down_half1,
            Tile::slope_up_half0, Tile::slope_up_half1
    };

    const uint8 width, height, nrOfMaterialTypes;

    WindMap wind;

    /**
     * Creates an empty map
     */
    TileMap(ivec2 size);

    ~TileMap();

    Tile getTile(uint8 x, uint8 y) const;

    TileMaterial getMaterial(uint8 x, uint8 y) const;

    const TileMaterialProperties &getMaterialProperties(TileMaterial) const;

    void setTile(uint8 x, uint8 y, Tile);

    void setTile(uint8 x, uint8 y, Tile, TileMaterial);

    bool contains(uint8 x, uint8 y) const;

    const TileMapOutlines &getOutlines() const { return outlines; }

    const std::list<tile_update> &updatesSinceLastUpdate() const { return tileUpdatesSinceLastUpdate; }

    const std::list<tile_update> &updatesPrevUpdate() const { return tileUpdatesPrevUpdate; }

    /**
     * Loops through the room's tiles in a 2 dimensional loop
     * @param pixelCoordsMin    position of the first tile in pixels
     * @param pixelCoordsMax    position of the last tile in pixels
     * @param callback          function to be called for each tile
     */
    void loopThroughTiles(
            const ivec2 &pixelCoordsMin, const ivec2 &pixelCoordsMax,

            const std::function<void(ivec2 tileCoords, Tile tile)> &callback
    ) const;

    /**
     * Exports this map to binary data, which will be APPENDED to `out`
     */
    void toBinary(std::vector<char> &out);

    /**
     * Loads a map from binary data.
     */
    void fromBinary(const char *data, int size);


  private:
    void refreshOutlines();

};


#endif
