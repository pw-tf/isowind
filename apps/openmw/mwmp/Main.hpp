#ifndef OPENMW_MWMP_MAIN
#define OPENMW_MWMP_MAIN

#include <string>
#include <vector>

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>

#include <components/files/collections.hpp>

namespace mwmp
{
    class GUIController;
    class CellController;
    class LocalSystem;
    class LocalPlayer;
    class Networking;

    // Isowind multiplayer (TES3MP's client). Only started when the game is run with --connect: otherwise
    // isInitialized() stays false, and the game is plain single-player.
    class Main
    {
    public:
        Main();
        ~Main();

        static void optionsDesc(boost::program_options::options_description *desc);
        static void configure(const boost::program_options::variables_map &variables);

        // Whether --connect was given.
        static bool isRequested();

        // Connects and checks the content files against the server's. False if that failed (the player has been told
        // why), in which case the game should quit.
        static bool init(const std::vector<std::string> &content, const Files::Collections &collections);
        static void postInit();
        static bool isInitialized();
        static void destroy();
        static const Main &get();
        static void frame(float dt);

        static bool isValidPacketScript(const std::string &scriptId);
        static bool isValidPacketGlobal(const std::string &globalId);

        Networking *getNetworking() const;
        LocalSystem *getLocalSystem() const;
        LocalPlayer *getLocalPlayer() const;
        GUIController *getGUIController() const;
        CellController *getCellController() const;

        void updateWorld(float dt) const;

    private:
        static std::string address;
        static std::string serverPassword;
        Main (const Main&) = delete;
        Main& operator= (const Main&) = delete;
        static Main *pMain;
        Networking *mNetworking;
        LocalSystem *mLocalSystem;
        LocalPlayer *mLocalPlayer;

        GUIController *mGUIController;
        CellController *mCellController;

        std::string server;
        unsigned short port;
    };
}

#endif //OPENMW_MWMP_MAIN
