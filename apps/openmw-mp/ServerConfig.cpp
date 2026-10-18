#include "ServerConfig.hpp"

#include <fstream>
#include <stdexcept>

#include <components/misc/strings/algorithm.hpp>

using namespace mwmp;

namespace
{
    std::string trim(const std::string &text)
    {
        const char *space = " \t\r\n";
        std::string::size_type begin = text.find_first_not_of(space);
        if (begin == std::string::npos)
            return {};
        return text.substr(begin, text.find_last_not_of(space) - begin + 1);
    }
}

void ServerConfig::load(const std::filesystem::path &path)
{
    std::ifstream file(path);
    if (!file)
        throw std::runtime_error("Cannot read settings file " + path.string());

    std::string section;
    std::string line;
    while (std::getline(file, line))
    {
        line = trim(line);
        if (line.empty() || line[0] == '#')
            continue;
        if (line.front() == '[' && line.back() == ']')
        {
            section = trim(line.substr(1, line.size() - 2));
            continue;
        }
        std::string::size_type equals = line.find('=');
        if (equals == std::string::npos)
            continue;
        mValues[{ section, trim(line.substr(0, equals)) }] = trim(line.substr(equals + 1));
    }
}

std::string ServerConfig::getString(const std::string &key, const std::string &section) const
{
    auto found = mValues.find({ section, key });
    if (found == mValues.end())
        throw std::runtime_error("Missing setting " + key + " in [" + section + "]");
    return found->second;
}

int ServerConfig::getInt(const std::string &key, const std::string &section) const
{
    return std::stoi(getString(key, section));
}

bool ServerConfig::getBool(const std::string &key, const std::string &section) const
{
    const std::string value = getString(key, section);
    return Misc::StringUtils::ciEqual(value, "true") || value == "1";
}
