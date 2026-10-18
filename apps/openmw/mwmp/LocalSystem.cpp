#include "LocalSystem.hpp"
#include "Main.hpp"
#include "Networking.hpp"

using namespace mwmp;

LocalSystem::LocalSystem()
{

}

LocalSystem::~LocalSystem()
{

}

Networking *LocalSystem::getNetworking()
{
    return mwmp::Main::get().getNetworking();
}
