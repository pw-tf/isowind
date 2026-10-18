#ifndef OPENMW_SERVERCONFIG_HPP
#define OPENMW_SERVERCONFIG_HPP

#include <filesystem>
#include <map>
#include <string>

namespace mwmp
{
    // The server's own settings: tes3mp-server-default.cfg, overridden by tes3mp-server.cfg when present. Same
    // "[Section]" / "key = value" / "# comment" format and keys as TES3MP. (OpenMW 0.51's Settings::Manager only
    // knows the engine's own settings, so the server keeps its files separate.)
    class ServerConfig
    {
    public:
        // Throws if the file can't be read.
        void load(const std::filesystem::path &path);

        std::string getString(const std::string &key, const std::string &section) const;
        int getInt(const std::string &key, const std::string &section) const;
        bool getBool(const std::string &key, const std::string &section) const;

    private:
        std::map<std::pair<std::string, std::string>, std::string> mValues; // (section, key) -> value
    };
}

#endif //OPENMW_SERVERCONFIG_HPP
