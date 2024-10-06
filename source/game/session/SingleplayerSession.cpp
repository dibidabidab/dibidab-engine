
#include "SingleplayerSession.h"

void SingleplayerSession::join(std::string username)
{
    std::string declineReason;
    validateUsername(username, declineReason);

    if (!declineReason.empty())
    {
        onJoinRequestDeclined(declineReason);
        return;
    }
    assert(players.empty()); // splitscreen not yet supported

    localPlayer = std::make_shared<Player>();
    localPlayer->id = 1999;
    localPlayer->name = username;
    players.push_back(localPlayer);
}

void SingleplayerSession::update(double deltaTime)
{
    if (level)
        level->update(deltaTime);
}

void SingleplayerSession::setLevel(Level *newLevel)
{
    if (level && level->isUpdating())
        throw gu_err("cant set a level while updating");
    delete level;
    level = newLevel;
    if (level)
    {
        level->initialize();
        onNewLevel(level);
        spawnPlayerEntities();
    }
    else onNewLevel(nullptr);
}
