
#ifndef GAME_RAINBOWSYSTEM_H
#define GAME_RAINBOWSYSTEM_H

#include "EntitySystem.h"
#include "../../room/Room.h"
#include "../components/Rainbow.h"
#include "../components/physics/Physics.h"
#include "../components/Polyline.h"
#include "../components/physics/PolyPlatform.h"
#include "../components/PlayerControlled.h"

class RainbowSystem : public EntitySystem
{
    using EntitySystem::EntitySystem;

    Room *room = NULL;

  protected:
    void update(double deltaTime, Room *room) override
    {
        this->room = room;

        limitRainbowArrowAngle();

        updateRainbowSpawners(deltaTime);

        updateDisappearingRainbows(deltaTime);
    }

    void limitRainbowArrowAngle();

    void updateRainbowSpawners(float deltaTime);

    void updateDisappearingRainbows(float deltaTime);

};


#endif