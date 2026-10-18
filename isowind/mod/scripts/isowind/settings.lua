-- Isowind's page in the game's Options -> Scripts (a MENU script, where settings pages are registered). The player
-- scripts read these from storage.playerSection('Settings/Isowind/Multiplayer').

local I = require('openmw.interfaces')

I.Settings.registerPage {
    key = 'Isowind',
    l10n = 'Isowind',
    name = 'settings_page',
    description = 'settings_page_desc',
}

I.Settings.registerGroup {
    key = 'Settings/Isowind/Multiplayer',
    page = 'Isowind',
    l10n = 'Isowind',
    name = 'settings_multiplayer',
    permanentStorage = true,
    settings = {
        {
            key = 'ShowPlayerNames',
            renderer = 'checkbox',
            name = 'settings_player_names',
            description = 'settings_player_names_desc',
            default = true,
        },
    },
}
