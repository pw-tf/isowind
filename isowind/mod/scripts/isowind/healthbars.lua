-- Health bars over the heads of creatures, and of people once they are hurt or hostile (Diablo style); in
-- multiplayer, the other players' names over them too (Options -> Scripts -> Isowind).
-- Used by topdown.lua, which supplies the projection from the world onto the screen.

local self = require('openmw.self')
local nearby = require('openmw.nearby')
local storage = require('openmw.storage')
local types = require('openmw.types')
local ui = require('openmw.ui')
local util = require('openmw.util')

local Actor = types.Actor
local v2 = util.vector2
local v3 = util.vector3

local RANGE = 3000                -- only actors this close to the player get a bar
local WIDTH, HEIGHT = 48, 6       -- in UI pixels
local HOSTILE_FIGHT = 80          -- NPCs with at least this Fight are shown even unhurt

local texture = ui.texture { path = 'textures/menu_bar_red.dds' }
local backColor = util.color.rgb(0.08, 0.04, 0.04)
local fillColor = util.color.rgb(0.85, 0.15, 0.1)

local NAME_SIZE = 16
local nameColor = util.color.rgb(0.792, 0.647, 0.376) -- Morrowind's normal text colour
local multiplayerSettings = storage.playerSection('Settings/Isowind/Multiplayer')

local bars = {} -- actor id -> UI element
local names = {} -- actor id -> UI element (other players' names)

local M = {}

local function healthFraction(actor)
    local health = Actor.stats.dynamic.health(actor)
    local max = health.base + health.modifier
    if max <= 0 then return nil end
    return util.clamp(health.current / max, 0, 1)
end

local function wanted(actor)
    if actor == self.object or Actor.isDead(actor) then return nil end
    if (actor.position - self.position):length() > RANGE then return nil end
    local fraction = healthFraction(actor)
    if not fraction then return nil end
    if types.Creature.objectIsInstance(actor) or fraction < 1
        or Actor.stats.ai.fight(actor).modified >= HOSTILE_FIGHT then
        return fraction
    end
    return nil
end

local function makeBar()
    return ui.create {
        layer = 'IsowindOverlay',
        type = ui.TYPE.Widget,
        props = { anchor = v2(0.5, 1), size = v2(WIDTH, HEIGHT) },
        content = ui.content {
            {
                type = ui.TYPE.Image,
                props = { relativeSize = v2(1, 1), resource = texture, color = backColor },
            },
            {
                name = 'fill',
                type = ui.TYPE.Image,
                props = { position = v2(1, 1), size = v2(WIDTH - 2, HEIGHT - 2), resource = texture, color = fillColor },
            },
        },
    }
end

local function remove(id)
    bars[id]:destroy()
    bars[id] = nil
end

local function removeName(id)
    names[id]:destroy()
    names[id] = nil
end

local function makeName(text)
    return ui.create {
        layer = 'IsowindOverlay',
        type = ui.TYPE.Text,
        props = {
            anchor = v2(0.5, 1),
            text = text,
            textSize = NAME_SIZE,
            textColor = nameColor,
            textShadow = true,
            textShadowColor = util.color.rgb(0, 0, 0),
        },
    }
end

-- Another player whose name should show (multiplayer, and the option is on)
local function showsName(actor)
    return nearby.isOtherPlayer ~= nil and nearby.isOtherPlayer(actor)
        and multiplayerSettings:get('ShowPlayerNames') ~= false
        and (actor.position - self.position):length() <= RANGE
end

-- project(worldPosition) returns a normalised screen position, or nil when it is off screen.
function M.update(project)
    local seen = {}
    local seenNames = {}
    for _, actor in ipairs(nearby.actors) do
        if showsName(actor) then
            local box = actor:getBoundingBox()
            local screen = project(v3(box.center.x, box.center.y, box.center.z + box.halfSize.z + 12))
            if screen then
                seenNames[actor.id] = true
                local label = names[actor.id]
                if not label then
                    label = makeName(types.NPC.record(actor).name)
                    names[actor.id] = label
                end
                label.layout.props.relativePosition = screen
                -- above where the health bar goes
                label.layout.props.position = v2(0, -(HEIGHT + 2))
                label:update()
            end
        end
        local fraction = wanted(actor)
        if fraction then
            local box = actor:getBoundingBox()
            local top = v3(box.center.x, box.center.y, box.center.z + box.halfSize.z + 12)
            local screen = project(top)
            if screen then
                seen[actor.id] = true
                local bar = bars[actor.id]
                if not bar then
                    bar = makeBar()
                    bars[actor.id] = bar
                end
                bar.layout.props.relativePosition = screen
                bar.layout.content.fill.props.size = v2(math.floor((WIDTH - 2) * fraction + 0.5), HEIGHT - 2)
                bar:update()
            end
        end
    end
    for id in pairs(bars) do
        if not seen[id] then remove(id) end
    end
    for id in pairs(names) do
        if not seenNames[id] then removeName(id) end
    end
end

function M.clear()
    for id in pairs(bars) do remove(id) end
    for id in pairs(names) do removeName(id) end
end

return M
