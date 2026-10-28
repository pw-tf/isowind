#ifndef OPENMW_MWMP_PORTABLECHARACTER_HPP
#define OPENMW_MWMP_PORTABLECHARACTER_HPP

#include <filesystem>
#include <string>

namespace mwmp
{
    class LocalPlayer;

    // Isowind: a single-player character taken onto a server that allows it (the launcher's Character list, passed
    // as --character with the character's newest save). It keeps its name; after connecting the client says which
    // character it is, and the server either carries on as usual (it doesn't take them, or its own copy of the
    // character is newer) or asks for it, and the character is sent as the server keeps characters (JSON, read from
    // the save; custom/isowindPortable.lua on the server takes it from there).
    //
    // A character is told apart from others of the same name by an id kept beside its saves (isowind-character.txt
    // in its saves folder, made the first time it joins a server).
    //
    // The whole character goes, and comes back: items and spells the player made go with their records (the server
    // makes them again), and once the server has said the character is theirs there, the client writes the
    // character as it is there beside its saves every half minute and when the game closes
    // (isowind-server-progress.dat): stats, skills, spells, the whole inventory and the records the server made for
    // it, factions, bounty, reputation, the journal and the topics known. The next time the character is loaded in
    // single-player that goes on top of the save, and the game saves; they are where they were in single-player.
    namespace PortableCharacter
    {
        // The character's name, from its save; empty if the save can't be read
        std::string readName(const std::string &save);

        // Tells the server which character this is: its id, and when it was last played (its save's time)
        void sendInfo(LocalPlayer &player);

        // Sends the character, the server having asked for it
        void send(LocalPlayer &player);

        // The server has logged the player in as a character that goes to single-player (with its id): what they gain
        // is written back from now on, to the folder of the character they joined with, or (a character made on the
        // server) to its own among the single-player characters, found by its id or made
        void setLinked(LocalPlayer &player);

        // Writes the character's progress back every half minute
        void update(LocalPlayer &player, float dt);

        // And once more as the game closes
        void finish(LocalPlayer &player);

        // Single-player: puts the progress written back from a server on the character just loaded, if there is some
        // newer than the save. True if it did; the game should then save, and call clearServerProgress.
        bool applyServerProgress(const std::filesystem::path &characterFolder, const std::filesystem::path &save);

        // The progress is in a save now: it goes
        void clearServerProgress(const std::filesystem::path &characterFolder);

        // Single-player, a character made on a server and played offline for the first time (its folder has the
        // progress and no saves), on a new game: first who they are (name, race, looks, class, birthsign), then,
        // once the game has made the player from those, the rest of the progress and where they were. False if it
        // can't be done.
        bool setUpServerCharacter(const std::filesystem::path &characterFolder);
        bool finishServerCharacter(const std::filesystem::path &characterFolder);
    }
}

#endif //OPENMW_MWMP_PORTABLECHARACTER_HPP
