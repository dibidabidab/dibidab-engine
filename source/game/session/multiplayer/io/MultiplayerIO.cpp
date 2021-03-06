
#include <iostream>
#include "MultiplayerIO.h"

MultiplayerIO::MultiplayerIO(SharedSocket socket, PacketHandlingMode handlingMode) : socket(socket), handlingMode(handlingMode)
{
    socket->onMessage = [&](auto data, auto size)
    {
        handleInput(data, size);
    };
}

void MultiplayerIO::handleInput(const char *data, int size)
{
    static int typeHashSize = sizeof(PacketTypeHash);

    if (size < typeHashSize)
    {
        std::cerr << "Received packet of less than " << typeHashSize << "bytes. Could not determine type." << std::endl;
        return;
    }

    PacketTypeHash packetType = ((PacketTypeHash *) data)[0];

    auto receiver = packetReceivers[packetType];

    if (!receiver)
    {
        if (packetTypeNames.count(packetType))
            std::cerr << "Received packet of type " << packetTypeNames[packetType] << ", but no receiver was registered!" << std::endl;
        else
            std::cerr << "Received packet of unregistered type " << packetType << std::endl;
        return;
    }
    void *packet;
    try
    {
        packet = receiver->function(receiver, data + typeHashSize, size - typeHashSize);

    } catch(const std::exception& e) {
        std::cerr << "Caught exception while receiving " << packetTypeNames[packetType] << "-packet:\n" << e.what() << std::endl;
        return;
    }
    packetsReceived++;

    if (handlingMode == PacketHandlingMode::IMMEDIATELY_ON_SOCKET_THREAD)
    {
        handlePacket(packetType, packet);
    }
    else
    {
        unhandledPacketsMutex.lock();
        unhandledPackets[packetType].push_back(packet);
        unhandledPacketsMutex.unlock();
    }

}

MultiplayerIO::~MultiplayerIO()
{
    for (auto &r : packetReceivers) delete r.second;    // TODO: realFunction is not deleted.
    for (auto &h : packetHandlers) delete h.second;
    for (auto &s : packetSenders) delete s.second;

    for (auto &p : unhandledPackets)
        if (!p.second.empty())
            std::cerr << "MultiplayerIO deleted before handling remaining packets!" << std::endl;

    std::cout << "io ended\n";
}

void MultiplayerIO::printTypes()
{
    for (auto t : packetTypeNames)
        std::cout << t.second << ": " << t.first << std::endl;
}

void MultiplayerIO::handlePackets()
{
    // todo, preserve order of received packets
    unhandledPacketsMutex.lock();
    for (auto &uP : unhandledPackets)
    {
        PacketTypeHash hash = uP.first;

        for (auto packet : uP.second) handlePacket(hash, packet);
        uP.second.clear();
    }
    unhandledPacketsMutex.unlock();
}

void MultiplayerIO::handlePacket(PacketTypeHash typeHash, void *packet)
{
    auto handler = packetHandlers[typeHash];
    if (!handler)
    {
        std::cerr << "No handler found for packet of type " << packetTypeNames[typeHash] << std::endl;
        return;
    }
    try
    {
        handler->function(handler, packet);
    } catch(const std::exception& e) {
        std::cerr << "Caught exception while handling " << packetTypeNames[typeHash] << "-packet:\n" << e.what() << std::endl;
        std::cerr << "CLOSING THE SOCKET...\n";
        socket->close();
        return;
    }
}
