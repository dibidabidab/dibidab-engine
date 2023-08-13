
#ifndef GAME_ENTITYTEMPLATE_H
#define GAME_ENTITYTEMPLATE_H


#include "../../../external/entt/src/entt/entity/registry.hpp"

#include <string>


class EntityEngine;
class Networked;

/**
 * Abstract class.
 *
 * Entity templates are used to construct one (or more) entities with a collection of components.
 *
 * .create() is used to create the entity as if it only exists client-side.
 *
 * .createNetworked() is used to create the entity as if it exists on all clients.
 */
class EntityTemplate
{
  private:
    friend class EntityEngine;
    int templateHash = -1;

  protected:
    EntityEngine *engine = NULL;

  public:

    virtual const std::string &getDescription();

    entt::entity create(bool persistent=false);

    entt::entity createNetworked(int networkID=rand(), bool serverSide=true, bool persistent=false);

    virtual void createComponents(entt::entity, bool persistent=false) = 0;

  protected:

    virtual void makeNetworkedServerSide(Networked &) {}

    virtual void makeNetworkedClientSide(Networked &) {}

    virtual ~EntityTemplate() = default;

};


#endif
