
#ifndef GAME_SAVEGAME_H
#define GAME_SAVEGAME_H

#include <json.hpp>
#include "../luau.h"

#ifndef DIBIDAB_NO_SAVE_GAME

struct SaveGame
{
    SaveGame(const char *path);

    sol::table luaTable;

    void save(const char *path=NULL); // if path == NULL then same path from constructor is used.

    static sol::table getSaveDataForEntity(const std::string &entitySaveGameID, bool temporary);

  private:
    std::string loadedFromPath;
};
#endif

#endif
