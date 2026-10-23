-- Isowind's page in the game's Options -> Scripts (a MENU script, where settings pages are registered). The player
-- scripts read these from storage.playerSection('Settings/Isowind/Multiplayer').
-- The switch view trigger is registered here too, so it lasts (triggers from a MENU script do) and its bindings show
-- on the page; topdown.lua handles it and gives the keyboard binding its default, F7.

local I = require('openmw.interfaces')
local input = require('openmw.input')

input.registerTrigger {
    key = 'IsowindSwitchView',
    l10n = 'Isowind',
    name = 'switch_view',
    description = 'switch_view_desc',
}

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

-- Two bindings, so a key and a controller button can both switch the view (one button each)
I.Settings.registerGroup {
    key = 'Settings/Isowind/Controls',
    page = 'Isowind',
    l10n = 'Isowind',
    name = 'settings_controls',
    permanentStorage = true,
    settings = {
        {
            key = 'SwitchView',
            renderer = 'inputBinding',
            name = 'switch_view',
            description = 'switch_view_key_desc',
            default = 'IsowindSwitchView',
            argument = { type = 'trigger', key = 'IsowindSwitchView' },
        },
        {
            key = 'SwitchViewController',
            renderer = 'inputBinding',
            name = 'switch_view',
            description = 'switch_view_button_desc',
            default = 'IsowindSwitchViewController',
            argument = { type = 'trigger', key = 'IsowindSwitchView' },
        },
    },
}
