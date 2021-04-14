
#ifndef GAME_ENTITYENGINE_H
#define GAME_ENTITYENGINE_H

#include <map>
#include <list>
#include <utils/type_name.h>
#include <utils/math_utils.h>
#include "../../external/entt/src/entt/entity/registry.hpp"
#include "entity_templates/EntityTemplate.h"
#include "systems/EntitySystem.h"
#include "../luau.h"
#include "../macro_magic/component.h"
#include "EventEmitter.h"

class EntityEngine
{
    bool initialized = false, updating = false;
  protected:

    std::list<EntitySystem *> systems;
    std::map<int, EntityTemplate *> entityTemplates;
    std::vector<std::string> entityTemplateNames;
    std::string templateFolder = "scripts/entities/";

    virtual void initializeLuaEnvironment();

  public:
    sol::environment luaEnvironment;
    entt::registry entities;
    EventEmitter events;

    ivec2 cursorPosition = ivec2(0);

    void initialize();

    void addSystem(EntitySystem *sys, bool pushFront=false);

    std::list<EntitySystem *> getSystems() { return systems; }

    template <class EntityTemplate_>
    EntityTemplate &getTemplate()
    {
        return getTemplate(getTypeName<EntityTemplate_>());
    }

    EntityTemplate &getTemplate(std::string name);

    EntityTemplate &getTemplate(int templateHash);

    const std::vector<std::string> &getTemplateNames() const;

    entt::entity getChildByName(entt::entity parent, const char *childName);

    template<typename Component>
    Component &getChildComponentByName(entt::entity parent, const char *childName)
    {
        entt::entity child = getChildByName(parent, childName);
        return entities.get<Component>(child);
    }

    template<typename Component>
    Component *tryGetChildComponentByName(entt::entity parent, const char *childName)
    {
        entt::entity child = getChildByName(parent, childName);
        return entities.try_get<Component>(child);
    }

    void luaTableToComponent(entt::entity, const std::string &componentName, const sol::table &);

    static const ComponentUtils &componentUtils(const std::string &componentName);

    entt::entity createChild(entt::entity parent, const char *childName="");

    void setParent(entt::entity child, entt::entity parent, const char *childName="");

    // returns false if name is already in use
    bool setName(entt::entity, const char *name=NULL);

    entt::entity getByName(const char *name) const;

    const char *getName(entt::entity) const;

    const std::unordered_map<std::string, entt::entity> &getNamedEntities() const { return namedEntities; };

    template<typename type>
    void emitEntityEvent(entt::entity e, const type &event, const char *customEventName=NULL)
    {
        if (auto *emitter = entities.try_get<EventEmitter>(e))
            emitter->emit(event, customEventName);
    }

    virtual ~EntityEngine();

    virtual void update(double deltaTime);

    bool isUpdating() const { return updating; };


    virtual vec3 getPosition(entt::entity) const;

    virtual void setPosition(entt::entity, const vec3 &);

  protected:

    template <class EntityTemplate>
    void registerEntityTemplate()
    {
        auto name = getTypeName<EntityTemplate>();
        addEntityTemplate(name, new EntityTemplate());
    }

    void registerLuaEntityTemplate(const char *assetPath);

    void addEntityTemplate(const std::string &name, EntityTemplate *);

  private:
    void onChildCreation(entt::registry &, entt::entity);

    void onChildDeletion(entt::registry &, entt::entity);

    void onParentDeletion(entt::registry &, entt::entity);

    std::unordered_map<std::string, entt::entity> namedEntities;

    void onEnitiyDenaming(entt::registry &, entt::entity);

};


#endif
