
#ifndef GAME_NETWORKINGSYSTEM_H
#define GAME_NETWORKINGSYSTEM_H


#include "LevelSystem.h"
#include "../../Level.h"
#include "../components/Networked.h"

class NetworkingSystem : public LevelSystem
{
  protected:
    void init(Level *lvl) override
    {
        lvl->entities.on_construct<Networked>().connect<&NetworkingSystem::onCreated>(this);
        lvl->entities.on_destroy<Networked>().connect<&NetworkingSystem::onDestroyed>(this);
    }

    void onCreated(entt::registry &reg, entt::entity entity)
    {
        Networked &networked = reg.get<Networked>(entity);
        std::cout << "Entity " << int(entity) << ":" << networked.networkID << " created\n";
    }

    void onDestroyed(entt::registry &reg, entt::entity entity)
    {
        Networked &networked = reg.get<Networked>(entity);
        std::cout << "Entity " << int(entity) << ":" << networked.networkID << " destroyed\n";
    }

    void update(double deltaTime, Level *lvl) override
    {
        lvl->entities.view<Networked>().each([&](auto e, Networked &networked) {

            for (auto *c : networked.toSend->list)
            {
                json jsonToSend;
                bool hasChanged, isPresent;
                c->componentToJsonIfChanged(hasChanged, isPresent, jsonToSend, e, lvl->entities);

                bool wasPresent = networked.componentPresence[c->getComponentTypeHash()];

                if (wasPresent && !isPresent)
                {
                    std::cout << c->getComponentTypeName() << " was deleted from " << int(e) << ":" << networked.networkID <<"\n";
                }

                if (hasChanged)
                {
                    std::cout << c->getComponentTypeName() << " changed for " << int(e) << ":" << networked.networkID << ":\n" << jsonToSend << "\n";
                }
            }
        });
    }
};


#endif
