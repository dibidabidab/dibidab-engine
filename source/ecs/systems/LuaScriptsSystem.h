
#ifndef GAME_LUASCRIPTSSYSTEM_H
#define GAME_LUASCRIPTSSYSTEM_H

#include "EntitySystem.h"
#include "../../level/room/Room.h"
#include "../../generated/LuaScripted.hpp"

class LuaScriptsSystem : public EntitySystem
{
    using EntitySystem::EntitySystem;

    EntityEngine *engine;

  protected:
    void init(EntityEngine *) override;

    void update(double deltaTime, EntityEngine *room) override;

    void onDestroyed(entt::registry &, entt::entity);

    ~LuaScriptsSystem() override;

};


#endif
