
#ifndef DIBIDAB_DIBIDAB_H
#define DIBIDAB_DIBIDAB_H

#include "session/Session.h"
#include "../generated/DibidabEngineSettings.hpp"

namespace gu
{
    struct Config;
}

namespace dibidab
{

    extern EngineSettings settings;

    extern std::map<std::string, std::string> startupArgs;

    extern delegate<void()> onSessionChange;

    Session &getCurrentSession();
    Session *tryGetCurrentSession();
    void setCurrentSession(Session *);

    void addDefaultAssetLoaders();

    void init(int argc, char *argv[], gu::Config &config);

    void run();
};


#endif
