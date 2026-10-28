-- Isowind: the host's choices, from [Isowind] in tes3mp-server-default.cfg and then tes3mp-server.cfg (which
-- Isowind.exe's "Host a server" and "isowind.sh host" write). The server runs in the folder they're in.
--
--   mode          coop (one shared world) or mmo (everyone loots and quests for themselves, enemies respawn)
--   pvp           whether players can hurt each other
--   difficulty    -100 to 100, 0 is Morrowind's normal
--   respawnSpeed  MMO: how much faster than normal enemies come back (0.5 to 4)
--   timeSpeed     how much faster than Morrowind's the time of day goes (0.5 to 4)
--   portableCharacters  whether players can bring their single-player characters (custom/isowindPortable.lua)
--
-- Players are told the mode and PvP with the game settings sent at login ("isowind mode", "isowind pvp").

IsowindHost = { mode = "coop", pvp = false, difficulty = 0, respawnSpeed = 1, timeSpeed = 1, portableCharacters = false }

local function readSettings(path, values)
    local file = io.open(path, "r")
    if file == nil then
        return
    end
    local section = ""
    for line in file:lines() do
        line = line:match("^%s*(.-)%s*$")
        if line ~= "" and line:sub(1, 1) ~= "#" then
            local name = line:match("^%[(.-)%]$")
            if name ~= nil then
                section = name
            elseif section == "Isowind" then
                local key, value = line:match("^(.-)%s*=%s*(.*)$")
                if key ~= nil and value ~= "" then
                    values[key] = value
                end
            end
        end
    end
    file:close()
end

local function clamp(value, low, high, default)
    value = tonumber(value)
    if value == nil then
        return default
    end
    return math.max(low, math.min(high, value))
end

local values = {}
readSettings("tes3mp-server-default.cfg", values)
readSettings("tes3mp-server.cfg", values)

IsowindHost.mode = string.lower(values.mode or "coop") == "mmo" and "mmo" or "coop"
IsowindHost.pvp = string.lower(values.pvp or "false") == "true"
IsowindHost.difficulty = math.floor(clamp(values.difficulty, -500, 500, 0))
IsowindHost.respawnSpeed = clamp(values.respawnSpeed, 0.1, 10, 1)
IsowindHost.timeSpeed = clamp(values.timeSpeed, 0.1, 10, 1)
IsowindHost.portableCharacters = string.lower(values.portableCharacters or "false") == "true"

function IsowindHost.IsMmo()
    return IsowindHost.mode == "mmo"
end

config.gameMode = IsowindHost.mode == "mmo" and "Isowind MMO" or "Isowind co-op"
config.difficulty = IsowindHost.difficulty
table.insert(config.gameSettings, { name = "isowind mode", value = IsowindHost.mode })
table.insert(config.gameSettings, { name = "isowind pvp", value = IsowindHost.pvp })

tes3mp.LogMessage(enumerations.log.INFO, "[Isowind] " .. config.gameMode .. ", PvP " ..
    (IsowindHost.pvp and "on" or "off") .. ", difficulty " .. IsowindHost.difficulty .. ", respawns x" ..
    IsowindHost.respawnSpeed .. ", time x" .. IsowindHost.timeSpeed .. ", single-player characters " ..
    (IsowindHost.portableCharacters and "allowed" or "not allowed"))

-- Morrowind's time of day goes 30 times (40 at night) faster than real time; the host's speed scales both
customEventHooks.registerHandler("OnServerInit", function(eventStatus)
    if WorldInstance == nil then
        return
    end
    WorldInstance.data.time.dayTimeScale = config.defaultTimeTable.dayTimeScale * IsowindHost.timeSpeed
    WorldInstance.data.time.nightTimeScale = config.defaultTimeTable.nightTimeScale * IsowindHost.timeSpeed
    WorldInstance:UpdateFrametimeMultiplier()
    WorldInstance:QuicksaveToDrive()
end)

-- A dead actor can't be looted until the game in charge of its cell has said what it carries (the server asks when it
-- dies). If that answer never comes, its body would stay locked ("currently unusable"), so it unlocks after 5 seconds
-- anyway, and the server asks a player there again.
local lockedCorpses = {}
local unlockTimer = nil

customEventHooks.registerHandler("OnActorDeath", function(eventStatus, pid, cellDescription, actors)
    for uniqueIndex in pairs(actors) do
        table.insert(lockedCorpses, { cell = cellDescription, uniqueIndex = uniqueIndex, time = os.time() + 5 })
    end
end)

function IsowindUnlockCorpses()
    local now = os.time()
    local waiting = {}
    for _, corpse in ipairs(lockedCorpses) do
        if corpse.time > now then
            table.insert(waiting, corpse)
        else
            local cell = LoadedCells[corpse.cell]
            if cell ~= nil and tableHelper.containsValue(cell.unusableContainerUniqueIndexes, corpse.uniqueIndex) then
                tableHelper.removeValue(cell.unusableContainerUniqueIndexes, corpse.uniqueIndex)
                tes3mp.LogMessage(enumerations.log.WARN, "[Isowind] No contents came for the body of " ..
                    corpse.uniqueIndex .. " in " .. corpse.cell .. "; unlocked it and asked again")
                local visitor = cell.visitors[1]
                if visitor ~= nil and Players[visitor] ~= nil and Players[visitor]:IsLoggedIn() then
                    cell:RequestContainers(visitor, { corpse.uniqueIndex })
                end
            end
        end
    end
    lockedCorpses = waiting
    tes3mp.RestartTimer(unlockTimer, time.seconds(2))
end

customEventHooks.registerHandler("OnServerPostInit", function(eventStatus)
    unlockTimer = tes3mp.CreateTimer("IsowindUnlockCorpses", time.seconds(2))
    tes3mp.StartTimer(unlockTimer)
end)

-- The world's startup scripts (config.worldStartupScripts, what /runstartup runs: some actors and objects start out
-- wrong without them) run by themselves, through the first player in the world: after they log in, or after a new
-- character's creation. Again after the cells are reset (IsowindHost.ResetStartupScripts).
customEventHooks.registerHandler("OnPlayerAuthentified", function(eventStatus, pid)
    if WorldInstance == nil or WorldInstance:HasRunStartupScripts() then
        return
    end
    for _, scriptName in pairs(config.worldStartupScripts) do
        logicHandler.RunConsoleCommandOnPlayer(pid, "startscript " .. scriptName, false)
    end
    WorldInstance.coreVariables.hasRunStartupScripts = true
    WorldInstance:QuicksaveToDrive()
    tes3mp.LogMessage(enumerations.log.INFO, "[Isowind] Ran the world's startup scripts through " ..
        logicHandler.GetChatName(pid))
end)
