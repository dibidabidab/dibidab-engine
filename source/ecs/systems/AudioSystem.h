
#ifndef GAME_AUDIOSYSTEM_H
#define GAME_AUDIOSYSTEM_H

#include "EntitySystem.h"
#include "../../level/room/Room.h"
#include "../../game/dibidab.h"
#include "../../generated/SoundSpeaker.hpp"

class AudioSystem : public EntitySystem
{
    using EntitySystem::EntitySystem;

  protected:


    void update(double deltaTime, EntityEngine *room) override
    {
        room->entities.view<SoundSpeaker>().each([&](auto e, SoundSpeaker &speaker) {

            auto hash = speaker.getHash();
            if (hash != speaker.prevHash)
            {
                // speaker settings have changed, update SoundSource:

                if (!speaker.source && speaker.sound.isSet())
                {
                    speaker.source = std::make_shared<au::SoundSource>(speaker.sound.get());
                    speaker.source->play();
                }

                if (speaker.source)
                {
                    if (speaker.paused != speaker.source->isPaused())
                    {
                        if (speaker.paused)
                            speaker.source->pause();
                        else
                            speaker.source->play();
                    }

                    speaker.source->setVolume(speaker.volume * dibidab::settings.audio.masterVolume);
                    speaker.source->setLooping(speaker.looping);
                    speaker.source->setPitch(speaker.pitch);
                }
                speaker.prevHash = hash;
            }
            if (!speaker.looping && speaker.source && speaker.source->hasStopped())
                room->entities.remove<SoundSpeaker>(e);
        });
    }

};


#endif
