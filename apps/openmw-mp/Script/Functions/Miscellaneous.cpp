#include "Miscellaneous.hpp"

#include <components/openmw-mp/TimedLog.hpp>

#include <apps/openmw-mp/Networking.hpp>

#include <extern/PicoSHA2/picosha2.h>

#include <iostream>
#include <random>

static std::string tempRandomString;
static std::string tempHashString;

const char* MiscellaneousFunctions::GenerateRandomString(unsigned int length) noexcept
{
    const std::string characters = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

    std::random_device randomDevice;
    std::mt19937 generator(randomDevice());
    std::uniform_int_distribution<std::mt19937::result_type> distribution(0, characters.size() - 1);
    std::string randomString = "";

    for (std::size_t i = 0; i < length; ++i)
    {
        randomString += characters[distribution(generator)];
    }

    tempRandomString = randomString;
    return tempRandomString.c_str();
}

const char* MiscellaneousFunctions::GetSHA256Hash(const char* inputString) noexcept
{
    tempHashString = picosha2::hash256_hex_string(std::string{inputString});
    return tempHashString.c_str();
}

unsigned int MiscellaneousFunctions::GetLastPlayerId() noexcept
{
    return Players::getLastPlayerId();
}

int MiscellaneousFunctions::GetCurrentMpNum() noexcept
{
    return mwmp::Networking::getPtr()->getCurrentMpNum();
}

void MiscellaneousFunctions::SetCurrentMpNum(int mpNum) noexcept
{
    mwmp::Networking::getPtr()->setCurrentMpNum(mpNum);
}
