-- The first time the game is played: how to move, WASD or click to move (clicktomove.lua), asked in a small window
-- (changed later in Options -> Scripts -> Isowind). The game waits under it (the Interface mode, its windows hidden).
-- 1 or 2 choose too, and a controller's A takes WASD (click to move is for the mouse); closing it some other way (Esc)
-- leaves WASD, the default. The choice is the installation's, not the character's (permanent storage). Playing with a
-- controller it isn't asked (topdown.lua: the menus made for a controller, Isowind.exe's Controller tick box, or the
-- controller last used): its own controls, never click to move; it is asked once the mouse and keyboard are used.
-- Used by topdown.lua, which calls update every frame in the top-down view.

local async = require('openmw.async')
local core = require('openmw.core')
local input = require('openmw.input')
local storage = require('openmw.storage')
local ui = require('openmw.ui')
local util = require('openmw.util')
local I = require('openmw.interfaces')

local v2 = util.vector2
local l10n = core.l10n('Isowind')
local controls = storage.playerSection('Settings/Isowind/Controls')
local ownSettings = storage.playerSection('IsowindTopDown')

local prompt = nil
local keysUp = false                -- 1, 2 and A have been up since it opened (one held as it opened doesn't choose)
local modeCame = false              -- the Interface mode asked for has come (I.UI.getMode says so from a frame later)

local M = {}

local function close(mode)
    controls:set('MoveMode', mode)
    ownSettings:set('MoveModeChosen', true)
    if prompt then
        prompt:destroy()
        prompt = nil
    end
    -- Interface again (bringing back the windows it hid), then no mode: as Questman's ui.lua closes its own
    if I.UI.getMode() == 'Interface' then
        I.UI.setMode('Interface')
        I.UI.setMode()
    end
end

local CHOICE_WIDTH = 280           -- each choice's text wraps at this width, so the two are the same size side by side
local CHOICE_TEXT_HEIGHT = 72       -- room for its description's lines
local GAP = v2(16, 16)              -- between the two choices, and around the window's lines

local function choice(name, description, mode)
    local templates = I.MWUI.templates
    return {
        template = templates.boxSolid,
        events = { mouseClick = async:callback(function() close(mode) end) },
        content = ui.content {
            {
                template = templates.padding,
                content = ui.content {
                    {
                        type = ui.TYPE.Flex,
                        props = { arrange = ui.ALIGNMENT.Center },
                        content = ui.content {
                            { template = templates.textHeader, props = { text = l10n(name) } },
                            { template = templates.interval },
                            {
                                template = templates.textNormal,
                                props = {
                                    text = l10n(description),
                                    autoSize = false,
                                    size = v2(CHOICE_WIDTH, CHOICE_TEXT_HEIGHT),
                                    multiline = true,
                                    wordWrap = true,
                                    textAlignH = ui.ALIGNMENT.Center,
                                },
                            },
                        },
                    },
                },
            },
        },
    }
end

local function open()
    local templates = I.MWUI.templates
    I.UI.setMode('Interface', { windows = {} })
    keysUp, modeCame = false, false
    prompt = ui.create {
        layer = 'Windows',
        template = templates.boxTransparentThick,
        props = { relativePosition = v2(0.5, 0.5), anchor = v2(0.5, 0.5) },
        content = ui.content {
            {
                template = templates.padding,
                content = ui.content {
                    {
                        type = ui.TYPE.Flex,
                        props = { arrange = ui.ALIGNMENT.Center },
                        content = ui.content {
                            { template = templates.textHeader, props = { text = l10n('move_prompt_title') } },
                            { props = { size = GAP } },
                            {
                                type = ui.TYPE.Flex,
                                props = { horizontal = true },
                                content = ui.content {
                                    choice('move_prompt_wasd', 'move_prompt_wasd_desc', 'move_wasd'),
                                    { props = { size = GAP } },
                                    choice('move_prompt_click', 'move_prompt_click_desc', 'move_click'),
                                },
                            },
                            { props = { size = GAP } },
                            { template = templates.textNormal, props = { text = l10n('move_prompt_later') } },
                        },
                    },
                },
            },
        },
    }
end

-- canAsk: the player has control of the character (it is asked then, once)
function M.update(canAsk)
    -- (the topdown view's onFrame keeps calling while it is up: the game paused under it doesn't stop that)
    if not prompt then
        if canAsk and not ownSettings:get('MoveModeChosen') then open() end
        return
    end
    if I.UI.getMode() == 'Interface' then
        modeCame = true
    else
        if modeCame then close('move_wasd') end
        return
    end
    local one, two = input.isKeyPressed(input.KEY._1), input.isKeyPressed(input.KEY._2)
    local a = input.isControllerButtonPressed(input.CONTROLLER_BUTTON.A)
    if not keysUp then
        keysUp = not (one or two or a)
    elseif one or a then
        close('move_wasd')
    elseif two then
        close('move_click')
    end
end

return M
