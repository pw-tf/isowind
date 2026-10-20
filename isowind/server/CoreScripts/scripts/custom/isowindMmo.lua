-- Isowind MMO mode (mode = mmo in the server's [Isowind] settings, see isowindHost.lua).
--
-- * Everyone loots for themselves: what a player takes from or puts in a container, a corpse, a plant or ore is
--   theirs alone (kept per player in the cell's data, isowindLoot, and replayed to them when they load the cell).
--   Containers players placed stay shared.
-- * Loose items from the game's files (not ones players dropped) are picked up per player too (isowindTaken).
-- * Quests, topics, factions, bounties, reputation and kill counts are each player's own, and so is what quest scripts
--   do to the world's own people and things: enabling or disabling them (isowindStates) or deleting them (isowindTaken),
--   so a quest that removes someone for one player leaves them there for the others.
-- * Enemies and NPCs come back after they die, players there or not, after a time from how
--   tough they were (WoW-like tiers, from their health): trash 1.5 minutes, normal 5, elite 15, bosses 20, divided by
--   the host's respawn speed. Ones from the game's files are reset on each client ("isowind respawn", a console
--   command packet the game handles itself); leveled-list ones are spawned again where they first appeared. Someone a
--   quest script brought into the world (Tarhiel falling from the sky) is a one-off and stays dead.
-- * The game's one-off events (the scripts in config.isowindEventScriptIds: Tarhiel falling from the sky near Seyda
--   Neen) happen once for each player. What an event's script does to the world (enabling, disabling or deleting) and
--   how far it has got (its short and long local variables, isowindEventLocals) are each player's own. Players near the
--   one whose event it is (EVENT_BUBBLE_UNITS outdoors, the same cell indoors) who haven't had it see it with them, and
--   it counts as had for them too; the others get their own when they come by. The people an event script brings in or
--   takes away are run by each player's game itself ("isowind local"), not by whoever's game runs the cell, so each
--   player's game has its own of them: their deaths (isowindEventDeaths, which the player's game reports) are each
--   player's own, as their loot already is. The party doesn't share events: they're for whoever is there.

if not IsowindHost.IsMmo() then
    return
end

-- The server keeps how far each player's one-off events have got: their games send their scripts' local variables
for _, scriptId in ipairs(config.isowindEventScriptIds or {}) do
    tableHelper.insertValueIfMissing(config.synchronizedClientScriptIds, scriptId)
end

config.shareJournal = false
config.shareTopics = false
config.shareFactionRanks = false
config.shareFactionExpulsion = false
config.shareFactionReputation = false
config.shareBounty = false
config.shareReputation = false
config.shareKills = false
config.shareVideos = false

local isowindMmo = {}

-- Minutes until an actor comes back, by its maximum health
local tiers = {
    { name = "trash", maxHealth = 60, minutes = 1.5 },
    { name = "normal", maxHealth = 200, minutes = 5 },
    { name = "elite", maxHealth = 500, minutes = 15 },
    { name = "boss", minutes = 20 }
}
-- Bosses whatever their health
local bossPatterns = { "^dagoth", "^vivec_god", "^almalexia", "^karstaag", "^lich_", "^heart_akulakhan" }
-- Weak creatures, for when their health never reached the server (killed in a hit or two)
local trashPatterns = { "^rat", "mudcrab", "scrib", "kwama forager", "cliff racer", "slaughterfish", "^nix",
    "^skeleton", "ancestor_ghost", "^guar" }

local CHECK_SECONDS = 10

-- Plants, ore and kwama eggs a player harvested grow back for them after this many minutes (divided by the respawn
-- speed). Chests don't refill: players keep things in them.
local REGROW_MINUTES = 10
local regrowPatterns = { "^flora_", "^rock_", "^egg_kwama" }

-- This session's players (by name): the cells their game has loaded since they logged in, so has their own loot in
-- it, and what grew back there for them, to give their game when they load it again
local seenCells = {}
local regrown = {}


local function playerKey(pid)
    return string.lower(Players[pid].accountName)
end

-- Whose world a player's quest scripts change: theirs, or their whole party's (isowindParty.lua)
local function questSharers(pid)
    local name = playerKey(pid)
    if IsowindParty ~= nil then
        return IsowindParty.GetMemberNames(name)
    end
    return { name }
end

-- The pids of the others among names who are on
local function otherOnlinePids(pid, names)
    local pids = {}
    for otherPid, player in pairs(Players) do
        if otherPid ~= pid and player:IsLoggedIn() and
            tableHelper.containsValue(names, string.lower(player.accountName)) then
            table.insert(pids, otherPid)
        end
    end
    return pids
end

local function splitIndex(uniqueIndex)
    local parts = uniqueIndex:split("-")
    return tonumber(parts[1]), tonumber(parts[2])
end

local function withCell(cellDescription, callback)
    local temporary = LoadedCells[cellDescription] == nil
    if temporary then
        logicHandler.LoadCell(cellDescription)
    end
    local result = callback(LoadedCells[cellDescription])
    if temporary then
        logicHandler.UnloadCell(cellDescription)
    end
    return result
end

local function isPlacedByPlayer(cell, uniqueIndex)
    return tableHelper.containsValue(cell.data.packets.place or {}, uniqueIndex)
end

local function isActor(cell, uniqueIndex)
    local objectData = cell.data.objectData[uniqueIndex]
    return objectData ~= nil and (objectData.deathState ~= nil or objectData.stats ~= nil or objectData.location ~= nil)
end

-- --- Loot per player ---------------------------------------------------------------------------------------------

local function regrows(refId)
    refId = string.lower(refId or "")
    for _, pattern in ipairs(regrowPatterns) do
        if refId:find(pattern) then
            return true
        end
    end
    return false
end

local function getRegrowths()
    WorldInstance.data.isowindRegrow = WorldInstance.data.isowindRegrow or {}
    return WorldInstance.data.isowindRegrow
end

-- When a player's harvest of a plant or ore is due to grow back: once they've only taken from it
local function scheduleRegrowth(cellDescription, uniqueIndex, refId, name, mine)
    local regrowths = getRegrowths()
    local onlyTaken = next(mine) ~= nil
    for _, item in pairs(mine) do
        if item.count > 0 then
            onlyTaken = false
        end
    end
    local cellDue = regrowths[cellDescription] or {}
    local due = cellDue[uniqueIndex] or {}
    if onlyTaken and regrows(refId) then
        due[name] = due[name] or os.time() + math.floor(REGROW_MINUTES * 60 / IsowindHost.respawnSpeed)
    else
        due[name] = nil
    end
    cellDue[uniqueIndex] = next(due) ~= nil and due or nil
    regrowths[cellDescription] = next(cellDue) ~= nil and cellDue or nil
end

-- A Container packet from a player (already read): kept as that player's change, and sent back to them only
local function saveOwnContainerChange(pid, cell)
    local action = tes3mp.GetObjectListAction()
    local name = playerKey(pid)
    cell.data.isowindLoot = cell.data.isowindLoot or {}

    for objectIndex = 0, tes3mp.GetObjectListSize() - 1 do
        local uniqueIndex = tes3mp.GetObjectRefNum(objectIndex) .. "-" .. tes3mp.GetObjectMpNum(objectIndex)
        local record = cell.data.isowindLoot[uniqueIndex]
        if record == nil then
            record = { refId = tes3mp.GetObjectRefId(objectIndex), players = {} }
            cell.data.isowindLoot[uniqueIndex] = record
        end
        local mine = record.players[name] or {}

        for itemIndex = 0, tes3mp.GetContainerChangesSize(objectIndex) - 1 do
            local item = {
                refId = tes3mp.GetContainerItemRefId(objectIndex, itemIndex),
                charge = tes3mp.GetContainerItemCharge(objectIndex, itemIndex),
                enchantmentCharge = tes3mp.GetContainerItemEnchantmentCharge(objectIndex, itemIndex),
                soul = tes3mp.GetContainerItemSoul(objectIndex, itemIndex)
            }
            local change
            if action == enumerations.container.ADD then
                change = tes3mp.GetContainerItemCount(objectIndex, itemIndex)
            else
                change = -tes3mp.GetContainerItemActionCount(objectIndex, itemIndex)
            end
            local key = string.lower(item.refId) .. "|" .. tostring(item.charge) .. "|" ..
                tostring(item.enchantmentCharge) .. "|" .. tostring(item.soul)
            local entry = mine[key] or item
            entry.count = (entry.count or 0) + change
            if entry.count == 0 then
                mine[key] = nil
            else
                mine[key] = entry
            end
        end

        if next(mine) == nil then
            record.players[name] = nil
        else
            record.players[name] = mine
        end
        scheduleRegrowth(cell.description, uniqueIndex, record.refId, name, mine)
    end

    seenCells[name] = seenCells[name] or {}
    seenCells[name][cell.description] = true
    cell:QuicksaveToDrive()
    WorldInstance:QuicksaveToDrive()
end

local function onContainer(eventStatus, pid, cellDescription, objects)
    if eventStatus.validDefaultHandler == false then
        return
    end
    local action = tes3mp.GetObjectListAction()
    local subAction = tes3mp.GetObjectListContainerSubAction()
    -- Replies to the server's request for what containers hold are the shared, untouched contents
    if subAction == enumerations.containerSub.REPLY_TO_REQUEST or
        (action ~= enumerations.container.ADD and action ~= enumerations.container.REMOVE) then
        return
    end

    return withCell(cellDescription, function(cell)
        for objectIndex = 0, tes3mp.GetObjectListSize() - 1 do
            local uniqueIndex = tes3mp.GetObjectRefNum(objectIndex) .. "-" .. tes3mp.GetObjectMpNum(objectIndex)
            if isPlacedByPlayer(cell, uniqueIndex) then
                return
            end
        end

        saveOwnContainerChange(pid, cell)

        -- The player's game makes the change when it comes back, unless a script or dialogue already made it
        local packetOrigin = tes3mp.GetObjectListOrigin()
        if packetOrigin ~= enumerations.packetOrigin.CLIENT_SCRIPT_LOCAL and
            packetOrigin ~= enumerations.packetOrigin.CLIENT_SCRIPT_GLOBAL and
            packetOrigin ~= enumerations.packetOrigin.CLIENT_DIALOGUE then
            tes3mp.CopyReceivedObjectListToStore()
            tes3mp.SendContainer(false, false)
        end
        return customEventHooks.makeEventStatus(false, true)
    end)
end

-- --- Loose items per player --------------------------------------------------------------------------------------

-- A script or dialogue (a quest's), rather than the player's hands or the console
local function isFromScript(packetOrigin)
    return packetOrigin == enumerations.packetOrigin.CLIENT_SCRIPT_LOCAL or
        packetOrigin == enumerations.packetOrigin.CLIENT_SCRIPT_GLOBAL or
        packetOrigin == enumerations.packetOrigin.CLIENT_DIALOGUE
end

-- --- One-off events ----------------------------------------------------------------------------------------------

-- How far apart players outdoors can be to see an event together (a cell is 8192 units across, and the cells around
-- count too); indoors they have to be in the same cell
local EVENT_BUBBLE_UNITS = 4096
-- A run of an event ends this long after the last change made by the script of the player whose run it is
local EVENT_RUN_SECONDS = 120
-- What a player's game sends this soon after loading a cell comes from its scripts starting over there, before the
-- server's replay of how far they got: for an event the player has had, it's answered with what the player has
local EVENT_REFIRE_SECONDS = 10

-- This session's runs of each event, by its script (lower case) and the player whose run it is: { event, starter,
-- members = { name = true }, last = time, changes = { cellDescription = { states, deletes, locals } } }
local eventRuns = {}
-- When each player's game last loaded each cell, this session
local cellLoadedAt = {}
-- The event people each player's game was told to run itself, this session: { name = { cellDescription = { uniqueIndex
-- = true } } }
local runsOwnCopy = {}

-- The event a packet from a script is about (its script, in lower case), or nil
local function eventScriptOf(packetOrigin)
    if packetOrigin ~= enumerations.packetOrigin.CLIENT_SCRIPT_LOCAL and
        packetOrigin ~= enumerations.packetOrigin.CLIENT_SCRIPT_GLOBAL then
        return nil
    end
    local script = string.lower(tes3mp.GetObjectListClientScript() or "")
    for _, scriptId in ipairs(config.isowindEventScriptIds or {}) do
        if string.lower(scriptId) == script then
            return script
        end
    end
    return nil
end

local function getEventsHad()
    WorldInstance.data.isowindEventsHad = WorldInstance.data.isowindEventsHad or {}
    return WorldInstance.data.isowindEventsHad
end

local function hasHadEvent(event, name)
    local had = getEventsHad()[event]
    return had ~= nil and had[name] == true
end

local function markEventHad(event, name)
    local eventsHad = getEventsHad()
    eventsHad[event] = eventsHad[event] or {}
    eventsHad[event][name] = true
    WorldInstance:QuicksaveToDrive()
end

local function isEventObject(cell, uniqueIndex)
    return cell.data.isowindEvents ~= nil and cell.data.isowindEvents[uniqueIndex] ~= nil
end

-- Where a player is: their cell, and their place in the world (which outdoors goes across cells)
local function placeOf(pid)
    return { cell = tes3mp.GetCell(pid), exterior = tes3mp.IsInExterior(pid), x = tes3mp.GetPosX(pid),
        y = tes3mp.GetPosY(pid) }
end

local function inBubble(place, otherPlace)
    if place.exterior and otherPlace.exterior then
        local dx, dy = place.x - otherPlace.x, place.y - otherPlace.y
        return dx * dx + dy * dy <= EVENT_BUBBLE_UNITS * EVENT_BUBBLE_UNITS
    end
    return not place.exterior and not otherPlace.exterior and place.cell == otherPlace.cell
end

local function onlinePidOf(name)
    for pid, player in pairs(Players) do
        if player:IsLoggedIn() and playerKey(pid) == name then
            return pid
        end
    end
    return nil
end

local function isRefire(name, cellDescription)
    local loadedAt = cellLoadedAt[name] ~= nil and cellLoadedAt[name][cellDescription] or nil
    return loadedAt ~= nil and os.time() - loadedAt <= EVENT_REFIRE_SECONDS
end

-- An event script's short and long local variables from a ClientScriptLocal packet, as { "type:index" = value } (keys
-- that stay as they are in the cell's file); its floats are timers, changing all the time, and are never kept
local function intLocals(variables)
    local result = {}
    for variableType, variableTable in pairs(variables or {}) do
        local typeNumber = tonumber(variableType)
        if typeNumber == enumerations.variableType.SHORT or typeNumber == enumerations.variableType.LONG then
            for internalIndex, value in pairs(variableTable) do
                result[typeNumber .. ":" .. tonumber(internalIndex)] = value
            end
        end
    end
    return result
end

-- Back to the packet's form: { variableType = { internalIndex = value } }
local function packetLocals(vars)
    local variables = {}
    for key, value in pairs(vars) do
        local variableType, internalIndex = key:match("^(%d+):(%d+)$")
        if variableType ~= nil then
            variableType = tonumber(variableType)
            variables[variableType] = variables[variableType] or {}
            variables[variableType][tonumber(internalIndex)] = value
        end
    end
    return variables
end

local function setOwnState(cell, uniqueIndex, refId, name, state)
    cell.data.isowindStates = cell.data.isowindStates or {}
    local record = cell.data.isowindStates[uniqueIndex] or { refId = refId, players = {} }
    record.players[name] = state
    cell.data.isowindStates[uniqueIndex] = record
end

local function getOwnState(cell, uniqueIndex, name)
    local record = cell.data.isowindStates ~= nil and cell.data.isowindStates[uniqueIndex] or nil
    if record == nil then
        return nil
    end
    return record.players[name]
end

local function setOwnTaken(cell, uniqueIndex, name)
    cell.data.isowindTaken = cell.data.isowindTaken or {}
    cell.data.isowindTaken[uniqueIndex] = cell.data.isowindTaken[uniqueIndex] or {}
    cell.data.isowindTaken[uniqueIndex][name] = true
end

local function setOwnLocals(cell, uniqueIndex, refId, name, vars)
    cell.data.isowindEventLocals = cell.data.isowindEventLocals or {}
    local record = cell.data.isowindEventLocals[uniqueIndex] or { refId = refId, players = {} }
    local mine = record.players[name] or {}
    for key, value in pairs(vars) do
        mine[key] = value
    end
    record.players[name] = mine
    cell.data.isowindEventLocals[uniqueIndex] = record
end

local function getOwnLocals(cell, uniqueIndex, name)
    local record = cell.data.isowindEventLocals ~= nil and cell.data.isowindEventLocals[uniqueIndex] or nil
    if record == nil then
        return nil
    end
    return record.players[name]
end

-- To one player's game: { uniqueIndex = { refId, state } }
local function sendStates(pid, cellDescription, states)
    tes3mp.ClearObjectList()
    tes3mp.SetObjectListPid(pid)
    tes3mp.SetObjectListCell(cellDescription)
    for uniqueIndex, object in pairs(states) do
        packetBuilder.AddObjectState(uniqueIndex, object)
    end
    tes3mp.SendObjectState(false)
end

-- { uniqueIndex = { refId } }
local function sendDeletes(pid, cellDescription, deletes)
    tes3mp.ClearObjectList()
    tes3mp.SetObjectListPid(pid)
    tes3mp.SetObjectListCell(cellDescription)
    for uniqueIndex, object in pairs(deletes) do
        packetBuilder.AddObjectDelete(uniqueIndex, object)
    end
    tes3mp.SendObjectDelete(false)
end

-- { uniqueIndex = { refId, vars } }
local function sendLocals(pid, cellDescription, locals)
    tes3mp.ClearObjectList()
    tes3mp.SetObjectListPid(pid)
    tes3mp.SetObjectListCell(cellDescription)
    for uniqueIndex, entry in pairs(locals) do
        packetBuilder.AddClientScriptLocal(uniqueIndex, { refId = entry.refId, variables = packetLocals(entry.vars) })
    end
    tes3mp.SendClientScriptLocal(false)
end

-- A command the game handles itself (mwmp::ObjectList::runConsoleCommands), to one player: { uniqueIndex = refId }
local function sendCommand(pid, cellDescription, command, objects)
    tes3mp.ClearObjectList()
    tes3mp.SetObjectListPid(pid)
    tes3mp.SetObjectListCell(cellDescription)
    tes3mp.SetObjectListConsoleCommand(command)
    for uniqueIndex, refId in pairs(objects) do
        local refNum, mpNum = splitIndex(uniqueIndex)
        tes3mp.SetObjectRefNum(refNum)
        tes3mp.SetObjectMpNum(mpNum)
        tes3mp.SetObjectRefId(refId)
        tes3mp.AddObject()
    end
    tes3mp.SendConsoleCommand(false, false)
end

-- Tells a player's game to run some of a cell's event people itself, each as that player has them: dead where they
-- died for them ("isowind local dead x y z"), or else as the game's files have them ("isowind local"), until the event
-- changes them. Their game does nothing with the things among them.
local function sendEventPeople(pid, cell, uniqueIndexes)
    local name = playerKey(pid)
    local deaths = cell.data.isowindEventDeaths or {}
    runsOwnCopy[name] = runsOwnCopy[name] or {}
    runsOwnCopy[name][cell.description] = runsOwnCopy[name][cell.description] or {}
    local alive = {}
    for _, uniqueIndex in ipairs(uniqueIndexes) do
        local entry = cell.data.isowindEvents[uniqueIndex]
        local death = deaths[uniqueIndex] ~= nil and deaths[uniqueIndex][name] or nil
        if death ~= nil then
            sendCommand(pid, cell.description, string.format("isowind local dead %.2f %.2f %.2f", death.posX,
                death.posY, death.posZ), { [uniqueIndex] = entry.refId })
        else
            alive[uniqueIndex] = entry.refId
        end
        runsOwnCopy[name][cell.description][uniqueIndex] = true
    end
    if next(alive) ~= nil then
        sendCommand(pid, cell.description, "isowind local", alive)
    end
end

-- What an event script enables or disables becomes one of its cell's event objects; from then on the people among
-- them are run by each player's game itself, so every game that has the cell loaded is told now (the others when they
-- load it)
local function registerEventObjects(cell, objects, event)
    cell.data.isowindEvents = cell.data.isowindEvents or {}
    local added = {}
    for uniqueIndex, object in pairs(objects) do
        if cell.data.isowindEvents[uniqueIndex] == nil then
            cell.data.isowindEvents[uniqueIndex] = { refId = object.refId, script = event }
            table.insert(added, uniqueIndex)
        end
    end
    if #added == 0 then
        return
    end
    for _, visitor in pairs(cell.visitors) do
        if Players[visitor] ~= nil and Players[visitor]:IsLoggedIn() then
            sendEventPeople(visitor, cell, added)
        end
    end
end

local function runChanges(run, cellDescription)
    run.changes[cellDescription] = run.changes[cellDescription] or { states = {}, deletes = {}, locals = {} }
    return run.changes[cellDescription]
end

local function memberNames(run)
    local names = {}
    for member in pairs(run.members) do
        table.insert(names, member)
    end
    return names
end

-- A player joining a run gets what it has done so far, kept as theirs
local function giveRunSoFar(run, pid)
    local name = playerKey(pid)
    for cellDescription, changes in pairs(run.changes) do
        local hasCellLoaded = false
        withCell(cellDescription, function(cell)
            for uniqueIndex, object in pairs(changes.states) do
                setOwnState(cell, uniqueIndex, object.refId, name, object.state)
            end
            for uniqueIndex in pairs(changes.deletes) do
                setOwnTaken(cell, uniqueIndex, name)
            end
            for uniqueIndex, entry in pairs(changes.locals) do
                setOwnLocals(cell, uniqueIndex, entry.refId, name, entry.vars)
            end
            hasCellLoaded = tableHelper.containsValue(cell.visitors, pid)
            cell:QuicksaveToDrive()
        end)
        if next(changes.states) ~= nil then
            sendStates(pid, cellDescription, changes.states)
        end
        if next(changes.deletes) ~= nil then
            sendDeletes(pid, cellDescription, changes.deletes)
        end
        -- (the variables only to a game that has the cell loaded; the replay when it loads the cell gives them)
        if next(changes.locals) ~= nil and hasCellLoaded then
            sendLocals(pid, cellDescription, changes.locals)
        end
    end
end

-- The players near the one whose run it is, who haven't had the event, join it: it's theirs too from now on
local function joinNearby(run, starterPid)
    local here = placeOf(starterPid)
    for otherPid, player in pairs(Players) do
        if otherPid ~= starterPid and player:IsLoggedIn() then
            local other = playerKey(otherPid)
            if not run.members[other] and not hasHadEvent(run.event, other) and inBubble(here, placeOf(otherPid)) then
                run.members[other] = true
                markEventHad(run.event, other)
                tes3mp.LogMessage(enumerations.log.INFO, "[Isowind] " .. other .. " has the " .. run.event ..
                    " event with " .. run.starter)
                giveRunSoFar(run, otherPid)
            end
        end
    end
end

-- The run a change from a player's event script belongs to, and whether the run is theirs: the one they're in, or a new
-- one of theirs if they haven't had the event. Nil for a player who has had it and is in none.
local function eventRunOf(pid, event)
    local name = playerKey(pid)
    local now = os.time()
    eventRuns[event] = eventRuns[event] or {}
    local runs = eventRuns[event]
    for starter, run in pairs(runs) do
        if now - run.last > EVENT_RUN_SECONDS then
            runs[starter] = nil
        end
    end
    for _, run in pairs(runs) do
        if run.members[name] then
            if run.starter == name then
                run.last = now
                joinNearby(run, pid)
            end
            return run, run.starter == name
        end
    end
    if hasHadEvent(event, name) then
        return nil, false
    end
    local run = { event = event, starter = name, members = { [name] = true }, last = now, changes = {} }
    runs[name] = run
    markEventHad(event, name)
    tes3mp.LogMessage(enumerations.log.INFO, "[Isowind] " .. name .. " has the " .. event .. " event")
    joinNearby(run, pid)
    return run, true
end

-- An event script enabling or disabling the game's own people or things: for the player, and the others in their run
-- if it's theirs
local function onEventStates(pid, cell, mine, event)
    local name = playerKey(pid)
    registerEventObjects(cell, mine, event)
    local run, isStarter = eventRunOf(pid, event)

    if run == nil and isRefire(name, cell.description) then
        -- Their game's scripts starting over on loading the cell: it gets back what the player already has
        local answer = {}
        for uniqueIndex, object in pairs(mine) do
            local state = getOwnState(cell, uniqueIndex, name)
            if state == nil then
                state = object.state
                setOwnState(cell, uniqueIndex, object.refId, name, state)
            end
            answer[uniqueIndex] = { refId = object.refId, state = state }
        end
        sendStates(pid, cell.description, answer)
        return
    end

    local names = { name }
    if isStarter then
        names = memberNames(run)
        local changes = runChanges(run, cell.description)
        for uniqueIndex, object in pairs(mine) do
            changes.states[uniqueIndex] = { refId = object.refId, state = object.state }
        end
    end
    -- Each of them sees it now (the player's own game waits for the server to make the change)
    for _, member in ipairs(names) do
        for uniqueIndex, object in pairs(mine) do
            setOwnState(cell, uniqueIndex, object.refId, member, object.state)
        end
        local memberPid = member == name and pid or onlinePidOf(member)
        if memberPid ~= nil then
            sendStates(memberPid, cell.description, mine)
        end
    end
end

-- Whose an event script's deletion is: the run's if it's the player's run, else the player's own
local function eventDeleters(pid, cell, mine, event)
    local run, isStarter = eventRunOf(pid, event)
    if not isStarter then
        return { playerKey(pid) }
    end
    local changes = runChanges(run, cell.description)
    for uniqueIndex, object in pairs(mine) do
        changes.deletes[uniqueIndex] = { refId = object.refId }
    end
    return memberNames(run)
end

-- A player's game telling of the death of its own copy of an event person, and where the body lies: a ClientScriptLocal
-- packet as gameplay (no script's), with the place as floats 0 to 2 (mwmp::MechanicsHelper::sendClientLocalDeath; the
-- server only takes ActorDeath from the game that runs the cell, and a console command would be one of the player's).
-- Kept as that player's, only for this cell's event people that the server told that player's game to run itself and
-- that are there for them; anything else in such a packet changes nothing.
local function onEventDeath(pid, cellDescription, objects)
    local name = playerKey(pid)
    local own = runsOwnCopy[name] ~= nil and runsOwnCopy[name][cellDescription] or {}
    withCell(cellDescription, function(cell)
        for uniqueIndex, object in pairs(objects) do
            local place = object.variables ~= nil and object.variables[enumerations.variableType.FLOAT] or {}
            local x, y, z = tonumber(place[0]), tonumber(place[1]), tonumber(place[2])
            if x ~= nil and y ~= nil and z ~= nil and isEventObject(cell, uniqueIndex) and own[uniqueIndex] and
                getOwnState(cell, uniqueIndex, name) == true then
                cell.data.isowindEventDeaths = cell.data.isowindEventDeaths or {}
                cell.data.isowindEventDeaths[uniqueIndex] = cell.data.isowindEventDeaths[uniqueIndex] or {}
                cell.data.isowindEventDeaths[uniqueIndex][name] = { posX = x, posY = y, posZ = z }
                tes3mp.LogMessage(enumerations.log.INFO, "[Isowind] " .. name .. "'s " .. tostring(object.refId) ..
                    " " .. uniqueIndex .. " died in " .. cellDescription)
            else
                tes3mp.LogMessage(enumerations.log.WARN, "[Isowind] Ignored a death of " .. tostring(object.refId) ..
                    " " .. uniqueIndex .. " from " .. name .. ": not their game's own event person")
            end
        end
        cell:QuicksaveToDrive()
    end)
    return customEventHooks.makeEventStatus(false, false)
end

-- How far an event's script has got for a player (its short and long variables; ClientScriptLocal packets, which the
-- game sends for the scripts in config.synchronizedClientScriptIds): theirs, and their run's others' if the run is
-- theirs. Their own game already has it (it sets the variable before it tells the server), so it isn't sent back.
local function onClientScriptLocal(eventStatus, pid, cellDescription, objects)
    if eventStatus.validDefaultHandler == false then
        return
    end
    local packetOrigin = tes3mp.GetObjectListOrigin()
    if packetOrigin == enumerations.packetOrigin.CLIENT_GAMEPLAY then
        return onEventDeath(pid, cellDescription, objects)
    end
    local event = eventScriptOf(packetOrigin)
    if event == nil then
        return
    end
    local cell = LoadedCells[cellDescription]
    if cell == nil then
        return customEventHooks.makeEventStatus(false, true)
    end

    local changes = {}
    for uniqueIndex, object in pairs(objects) do
        local vars = intLocals(object.variables)
        if next(vars) ~= nil then
            changes[uniqueIndex] = { refId = object.refId, vars = vars }
        end
    end
    -- Only timers: nothing to keep
    if next(changes) == nil then
        return customEventHooks.makeEventStatus(false, true)
    end

    local name = playerKey(pid)
    local run, isStarter = eventRunOf(pid, event)
    if run == nil and isRefire(name, cellDescription) then
        -- Their game's script starting over on loading the cell: it gets back how far the player had got
        local answer = {}
        for uniqueIndex, change in pairs(changes) do
            local own = getOwnLocals(cell, uniqueIndex, name)
            if own ~= nil then
                answer[uniqueIndex] = { refId = change.refId, vars = own }
            else
                setOwnLocals(cell, uniqueIndex, change.refId, name, change.vars)
            end
        end
        if next(answer) ~= nil then
            sendLocals(pid, cellDescription, answer)
        end
        cell:QuicksaveToDrive()
        return customEventHooks.makeEventStatus(false, true)
    end

    local names = { name }
    if isStarter then
        names = memberNames(run)
        local runLocals = runChanges(run, cellDescription).locals
        for uniqueIndex, change in pairs(changes) do
            local entry = runLocals[uniqueIndex] or { refId = change.refId, vars = {} }
            for key, value in pairs(change.vars) do
                entry.vars[key] = value
            end
            runLocals[uniqueIndex] = entry
        end
    end
    for _, member in ipairs(names) do
        for uniqueIndex, change in pairs(changes) do
            setOwnLocals(cell, uniqueIndex, change.refId, member, change.vars)
        end
        -- (to a game that has the cell loaded: in one that hasn't, the script's variables aren't there to set; the
        -- replay when it loads the cell gives them)
        local memberPid = member ~= name and onlinePidOf(member) or nil
        if memberPid ~= nil and tableHelper.containsValue(cell.visitors, memberPid) then
            sendLocals(memberPid, cellDescription, changes)
        end
    end
    cell:QuicksaveToDrive()
    return customEventHooks.makeEventStatus(false, true)
end

local function onObjectDelete(eventStatus, pid, cellDescription, objects)
    local packetOrigin = tes3mp.GetObjectListOrigin()
    local fromScript = isFromScript(packetOrigin)
    if eventStatus.validDefaultHandler == false or
        (packetOrigin ~= enumerations.packetOrigin.CLIENT_GAMEPLAY and not fromScript) then
        return
    end
    local cell = LoadedCells[cellDescription]
    if cell == nil then
        return
    end

    -- The game's own things a player picks up, or a quest of theirs deletes (people too); bodies a player disposes of
    -- stay shared
    local mine, shared = {}, {}
    for uniqueIndex, object in pairs(objects) do
        local _, mpNum = splitIndex(uniqueIndex)
        if mpNum == 0 and (fromScript or not isActor(cell, uniqueIndex)) then
            mine[uniqueIndex] = object
        else
            shared[uniqueIndex] = object
        end
    end
    if next(mine) == nil then
        return
    end

    -- A quest's deletion is the whole party's, an event's its run's; a pickup only the player's
    local event = fromScript and eventScriptOf(packetOrigin) or nil
    local names
    if event ~= nil then
        names = eventDeleters(pid, cell, mine, event)
    else
        names = fromScript and questSharers(pid) or { playerKey(pid) }
    end
    cell.data.isowindTaken = cell.data.isowindTaken or {}
    for uniqueIndex, object in pairs(mine) do
        cell.data.isowindTaken[uniqueIndex] = cell.data.isowindTaken[uniqueIndex] or {}
        for _, name in ipairs(names) do
            cell.data.isowindTaken[uniqueIndex][name] = true
        end
    end
    local recipients = otherOnlinePids(pid, names)
    table.insert(recipients, pid)
    for _, recipient in ipairs(recipients) do
        tes3mp.ClearObjectList()
        tes3mp.SetObjectListPid(recipient)
        tes3mp.SetObjectListCell(cellDescription)
        for uniqueIndex, object in pairs(mine) do
            packetBuilder.AddObjectDelete(uniqueIndex, object)
        end
        tes3mp.SendObjectDelete(false)
    end

    -- The rest as the default handler does
    if next(shared) ~= nil then
        cell:SaveObjectsByPacketType("ObjectDelete", shared)
        cell:LoadObjectsByPacketType("ObjectDelete", pid, shared, tableHelper.getArrayFromIndexes(shared), true)
    end
    cell:QuicksaveToDrive()
    return customEventHooks.makeEventStatus(false, true)
end

-- A quest script enabling or disabling one of the game's own people or things: for that player (and their party) only;
-- an event script's, for that player (and the others in their run)
local function onObjectState(eventStatus, pid, cellDescription, objects)
    local packetOrigin = tes3mp.GetObjectListOrigin()
    if eventStatus.validDefaultHandler == false or not isFromScript(packetOrigin) then
        return
    end
    local cell = LoadedCells[cellDescription]
    if cell == nil then
        return
    end

    local mine, shared = {}, {}
    for uniqueIndex, object in pairs(objects) do
        local _, mpNum = splitIndex(uniqueIndex)
        if mpNum == 0 then
            mine[uniqueIndex] = object
        else
            shared[uniqueIndex] = object
        end
    end
    if next(mine) == nil then
        return
    end

    local event = eventScriptOf(packetOrigin)
    if event ~= nil then
        onEventStates(pid, cell, mine, event)
    else
        local names = questSharers(pid)
        cell.data.isowindStates = cell.data.isowindStates or {}
        for uniqueIndex, object in pairs(mine) do
            local record = cell.data.isowindStates[uniqueIndex] or { refId = object.refId, players = {} }
            for _, name in ipairs(names) do
                record.players[name] = object.state
            end
            cell.data.isowindStates[uniqueIndex] = record
        end
        -- The player and the party's others who are on see it now (the player's own game waits for the server to
        -- make the change, mwmp::hooks::requestScriptObjectState)
        local recipients = otherOnlinePids(pid, names)
        table.insert(recipients, pid)
        for _, recipient in ipairs(recipients) do
            tes3mp.ClearObjectList()
            tes3mp.SetObjectListPid(recipient)
            tes3mp.SetObjectListCell(cellDescription)
            for uniqueIndex, object in pairs(mine) do
                packetBuilder.AddObjectState(uniqueIndex, object)
            end
            tes3mp.SendObjectState(false)
        end
    end

    -- The rest as the default handler does
    if next(shared) ~= nil then
        cell:SaveObjectsByPacketType("ObjectState", shared)
        cell:LoadObjectsByPacketType("ObjectState", pid, shared, tableHelper.getArrayFromIndexes(shared), true)
    end
    cell:QuicksaveToDrive()
    return customEventHooks.makeEventStatus(false, true)
end

-- Plants and ore that grew back, to a player's game that still has them harvested: { uniqueIndex = { refId, items } }
local function sendRegrown(pid, cellDescription, grown)
    tes3mp.ClearObjectList()
    tes3mp.SetObjectListPid(pid)
    tes3mp.SetObjectListCell(cellDescription)
    for uniqueIndex, entry in pairs(grown) do
        local refNum, mpNum = splitIndex(uniqueIndex)
        tes3mp.SetObjectRefNum(refNum)
        tes3mp.SetObjectMpNum(mpNum)
        tes3mp.SetObjectRefId(entry.refId)
        for _, item in pairs(entry.items) do
            tes3mp.SetContainerItemRefId(item.refId)
            tes3mp.SetContainerItemCount(-item.count)
            tes3mp.SetContainerItemCharge(item.charge)
            tes3mp.SetContainerItemEnchantmentCharge(item.enchantmentCharge)
            tes3mp.SetContainerItemSoul(item.soul)
            tes3mp.AddContainerItem()
        end
        tes3mp.AddObject()
    end
    tes3mp.SetObjectListAction(enumerations.container.ADD)
    tes3mp.SendContainer(false, false)
end

-- A player loading a cell gets their own loot and pickups there on top of the shared state just sent
local function onCellLoad(eventStatus, pid, cellDescription)
    local cell = LoadedCells[cellDescription]
    if cell == nil or Players[pid] == nil then
        return
    end
    local name = playerKey(pid)

    -- Their game's scripts start over there now, before the replay of how far its events got (isRefire)
    cellLoadedAt[name] = cellLoadedAt[name] or {}
    cellLoadedAt[name][cellDescription] = os.time()

    -- The cell's event people, run by their game itself, before what the player's events did to them
    if cell.data.isowindEvents ~= nil then
        sendEventPeople(pid, cell, tableHelper.getArrayFromIndexes(cell.data.isowindEvents))
    end

    if cell.data.isowindTaken ~= nil then
        tes3mp.ClearObjectList()
        tes3mp.SetObjectListPid(pid)
        tes3mp.SetObjectListCell(cellDescription)
        local count = 0
        for uniqueIndex, takers in pairs(cell.data.isowindTaken) do
            if takers[name] then
                packetBuilder.AddObjectDelete(uniqueIndex, {})
                count = count + 1
            end
        end
        if count > 0 then
            tes3mp.SendObjectDelete(false)
        end
    end

    if cell.data.isowindStates ~= nil then
        tes3mp.ClearObjectList()
        tes3mp.SetObjectListPid(pid)
        tes3mp.SetObjectListCell(cellDescription)
        local count = 0
        for uniqueIndex, record in pairs(cell.data.isowindStates) do
            local state = record.players[name]
            if state ~= nil then
                packetBuilder.AddObjectState(uniqueIndex, { refId = record.refId, state = state })
                count = count + 1
            end
        end
        if count > 0 then
            tes3mp.SendObjectState(false)
        end
    end

    if cell.data.isowindLoot ~= nil then
        for _, action in ipairs({ enumerations.container.ADD, enumerations.container.REMOVE }) do
            tes3mp.ClearObjectList()
            tes3mp.SetObjectListPid(pid)
            tes3mp.SetObjectListCell(cellDescription)
            local objectCount = 0
            for uniqueIndex, record in pairs(cell.data.isowindLoot) do
                local mine = record.players[name]
                if mine ~= nil then
                    local refNum, mpNum = splitIndex(uniqueIndex)
                    tes3mp.SetObjectRefNum(refNum)
                    tes3mp.SetObjectMpNum(mpNum)
                    tes3mp.SetObjectRefId(record.refId)
                    local itemCount = 0
                    for _, item in pairs(mine) do
                        if (item.count > 0) == (action == enumerations.container.ADD) then
                            tes3mp.SetContainerItemRefId(item.refId)
                            tes3mp.SetContainerItemCount(math.abs(item.count))
                            tes3mp.SetContainerItemActionCount(math.abs(item.count))
                            tes3mp.SetContainerItemCharge(item.charge)
                            tes3mp.SetContainerItemEnchantmentCharge(item.enchantmentCharge)
                            tes3mp.SetContainerItemSoul(item.soul)
                            tes3mp.AddContainerItem()
                            itemCount = itemCount + 1
                        end
                    end
                    if itemCount > 0 then
                        tes3mp.AddObject()
                        objectCount = objectCount + 1
                    end
                end
            end
            if objectCount > 0 then
                tes3mp.SetObjectListAction(action)
                tes3mp.SendContainer(false, false)
            end
        end
    end

    -- What grew back since their game last had this cell loaded (their game still has it harvested)
    local grown = regrown[name] ~= nil and regrown[name][cellDescription] or nil
    if grown ~= nil then
        sendRegrown(pid, cellDescription, grown)
        regrown[name][cellDescription] = nil
    end

    -- How far the player's events there have got (their game's scripts start from nothing at each login)
    if cell.data.isowindEventLocals ~= nil then
        local locals = {}
        for uniqueIndex, record in pairs(cell.data.isowindEventLocals) do
            if record.players[name] ~= nil then
                locals[uniqueIndex] = { refId = record.refId, vars = record.players[name] }
            end
        end
        if next(locals) ~= nil then
            sendLocals(pid, cellDescription, locals)
        end
    end

    seenCells[name] = seenCells[name] or {}
    seenCells[name][cellDescription] = true
end

-- Plants and ore due to grow back: the player's harvest is forgotten, and their game gets the plant back now if they're
-- there, else the next time it loads the cell (if it has it loaded this session); the first line goes on as before:
-- the cell loaded this session gets the plant back the next time they load it
local function checkRegrowth()
    local regrowths = getRegrowths()
    local now = os.time()
    local changed = false
    for cellDescription, cellDue in pairs(regrowths) do
        local ready = {}
        for uniqueIndex, due in pairs(cellDue) do
            for name, time in pairs(due) do
                if time <= now then
                    table.insert(ready, { uniqueIndex = uniqueIndex, name = name })
                end
            end
        end
        if #ready > 0 then
            withCell(cellDescription, function(cell)
                local loot = cell.data.isowindLoot or {}
                for _, entry in ipairs(ready) do
                    local record = loot[entry.uniqueIndex]
                    local mine = record ~= nil and record.players[entry.name] or nil
                    local here = nil
                    for _, visitor in pairs(cell.visitors) do
                        if Players[visitor] ~= nil and playerKey(visitor) == entry.name then
                            here = visitor
                        end
                    end
                    if mine ~= nil then
                        record.players[entry.name] = nil
                        if here ~= nil then
                            sendRegrown(here, cellDescription, { [entry.uniqueIndex] = { refId = record.refId,
                                items = mine } })
                        elseif seenCells[entry.name] ~= nil and seenCells[entry.name][cellDescription] then
                            regrown[entry.name] = regrown[entry.name] or {}
                            regrown[entry.name][cellDescription] = regrown[entry.name][cellDescription] or {}
                            regrown[entry.name][cellDescription][entry.uniqueIndex] =
                                { refId = record.refId, items = mine }
                        end
                    end
                    cellDue[entry.uniqueIndex][entry.name] = nil
                    if next(cellDue[entry.uniqueIndex]) == nil then
                        cellDue[entry.uniqueIndex] = nil
                    end
                end
                cell:QuicksaveToDrive()
            end)
            if next(cellDue) == nil then
                regrowths[cellDescription] = nil
            end
            changed = true
        end
    end
    if changed then
        WorldInstance:QuicksaveToDrive()
    end
end

-- --- Respawns ----------------------------------------------------------------------------------------------------

local function getRespawns()
    WorldInstance.data.isowindRespawns = WorldInstance.data.isowindRespawns or {}
    return WorldInstance.data.isowindRespawns
end

local function respawnMinutes(refId, objectData)
    refId = string.lower(refId or "")
    for _, pattern in ipairs(bossPatterns) do
        if refId:find(pattern) then
            return tiers[#tiers].minutes
        end
    end
    local health = objectData ~= nil and objectData.stats ~= nil and objectData.stats.healthBase or nil
    if health == nil then
        for _, pattern in ipairs(trashPatterns) do
            if refId:find(pattern) then
                return tiers[1].minutes
            end
        end
        return tiers[2].minutes
    end
    for _, tier in ipairs(tiers) do
        if tier.maxHealth == nil or health < tier.maxHealth then
            return tier.minutes
        end
    end
end

-- Leveled-list creatures: where they first appeared, to come back there
local function onObjectSpawn(eventStatus, pid, cellDescription, objects)
    local cell = LoadedCells[cellDescription]
    if cell == nil then
        return
    end
    cell.data.isowindSpawnPoints = cell.data.isowindSpawnPoints or {}
    for uniqueIndex, object in pairs(objects) do
        if object.summon == nil and object.location ~= nil then
            cell.data.isowindSpawnPoints[uniqueIndex] = { refId = object.refId, location = object.location }
        end
    end
end

-- Someone a quest script brought into the world (enabled) for a player, like Tarhiel falling from the sky near Seyda
-- Neen: a one-off, so their death stays and they don't come back to fall again
local function isQuestAppearance(cell, uniqueIndex)
    local record = cell.data.isowindStates ~= nil and cell.data.isowindStates[uniqueIndex] or nil
    if record == nil then
        return false
    end
    for _, state in pairs(record.players) do
        if state == true then
            return true
        end
    end
    return false
end

local function onActorDeath(eventStatus, pid, cellDescription, actors)
    local cell = LoadedCells[cellDescription]
    if cell == nil then
        return
    end
    local respawns = getRespawns()
    local now = os.time()
    for uniqueIndex, actor in pairs(actors) do
        local objectData = cell.data.objectData[uniqueIndex]
        local _, mpNum = splitIndex(uniqueIndex)
        local isSummon = objectData ~= nil and objectData.summon ~= nil
        local spawnPoints = cell.data.isowindSpawnPoints or {}
        if not isSummon and not isQuestAppearance(cell, uniqueIndex) and not isEventObject(cell, uniqueIndex) and
            (mpNum == 0 or spawnPoints[uniqueIndex] ~= nil) then
            local minutes = respawnMinutes(actor.refId or (objectData and objectData.refId), objectData)
            respawns[cellDescription] = respawns[cellDescription] or {}
            respawns[cellDescription][uniqueIndex] = now + math.floor(minutes * 60 / IsowindHost.respawnSpeed)
        end
    end
    WorldInstance:QuicksaveToDrive()
end

local function respawnInCell(cellDescription, uniqueIndexes)
    local fromGameFiles = {}
    local leveled = {}

    withCell(cellDescription, function(cell)
        local spawnPoints = cell.data.isowindSpawnPoints or {}
        for _, uniqueIndex in ipairs(uniqueIndexes) do
            local objectData = cell.data.objectData[uniqueIndex]
            local moved = objectData ~= nil and (objectData.cellChangeTo ~= nil or objectData.cellChangeFrom ~= nil)
            -- One disabled for everyone (by the console, or before the server was MMO) stays gone; what a player's quest
            -- disabled stays that way for them (it's in isowindStates, which a respawn keeps)
            local disabled = objectData ~= nil and objectData.state == false
            local _, mpNum = splitIndex(uniqueIndex)
            -- A one-off event's people are each player's game's own, dead or alive
            if not moved and not disabled and not isEventObject(cell, uniqueIndex) then
                if cell.data.isowindLoot ~= nil then
                    cell.data.isowindLoot[uniqueIndex] = nil
                end
                if mpNum == 0 then
                    -- Forgetting its death, wounds, place and inventory brings it back as the game's files have it
                    cell:DeleteObjectData(uniqueIndex)
                    table.insert(fromGameFiles, uniqueIndex)
                elseif spawnPoints[uniqueIndex] ~= nil then
                    table.insert(leveled, { uniqueIndex = uniqueIndex, spawn = spawnPoints[uniqueIndex] })
                    spawnPoints[uniqueIndex] = nil
                end
            end
        end
        cell:QuicksaveToDrive()
    end)

    local anyPid = nil
    for pid, player in pairs(Players) do
        if player:IsLoggedIn() then
            anyPid = pid
            break
        end
    end

    if #fromGameFiles > 0 and anyPid ~= nil then
        logicHandler.RunConsoleCommandOnObjects(anyPid, "isowind respawn", cellDescription, fromGameFiles, true)
    end

    for _, entry in ipairs(leveled) do
        if anyPid ~= nil then
            logicHandler.DeleteObjectForEveryone(cellDescription, entry.uniqueIndex)
        else
            withCell(cellDescription, function(cell) cell:DeleteObjectData(entry.uniqueIndex) end)
        end
        local newIndex = logicHandler.CreateObjectAtLocation(cellDescription, entry.spawn.location,
            { refId = entry.spawn.refId, count = 1, charge = -1, enchantmentCharge = -1, soul = "" }, "spawn")
        if newIndex ~= nil then
            withCell(cellDescription, function(cell)
                cell.data.isowindSpawnPoints = cell.data.isowindSpawnPoints or {}
                cell.data.isowindSpawnPoints[newIndex] = entry.spawn
                cell:QuicksaveToDrive()
            end)
        end
    end

    tes3mp.LogMessage(enumerations.log.INFO, "[Isowind] Respawned " .. (#fromGameFiles + #leveled) ..
        " in " .. cellDescription)
end

local function checkRespawns()
    local respawns = getRespawns()
    local now = os.time()
    local changed = false
    for cellDescription, due in pairs(respawns) do
        local ready = {}
        for uniqueIndex, time in pairs(due) do
            if time <= now then
                table.insert(ready, uniqueIndex)
            end
        end
        if #ready > 0 then
            respawnInCell(cellDescription, ready)
            for _, uniqueIndex in ipairs(ready) do
                due[uniqueIndex] = nil
            end
            if next(due) == nil then
                respawns[cellDescription] = nil
            end
            changed = true
        end
    end
    if changed then
        WorldInstance:QuicksaveToDrive()
    end
end

local respawnTimer = nil

function IsowindMmoRespawnTimer()
    for _, check in ipairs({ checkRespawns, checkRegrowth }) do
        local ok, err = pcall(check)
        if not ok then
            tes3mp.LogMessage(enumerations.log.ERROR, "[Isowind] Respawn check failed: " .. tostring(err))
        end
    end
    tes3mp.RestartTimer(respawnTimer, time.seconds(CHECK_SECONDS))
end

-- After the server's own validators (added when it starts), so these see what they decided
customEventHooks.registerHandler("OnServerPostInit", function(eventStatus)
    customEventHooks.registerValidator("OnContainer", onContainer)
    customEventHooks.registerValidator("OnObjectDelete", onObjectDelete)
    customEventHooks.registerValidator("OnObjectState", onObjectState)
    customEventHooks.registerValidator("OnClientScriptLocal", onClientScriptLocal)
    customEventHooks.registerHandler("OnCellLoad", onCellLoad)
    customEventHooks.registerHandler("OnObjectSpawn", onObjectSpawn)
    customEventHooks.registerHandler("OnActorDeath", onActorDeath)
    -- A player who logs in again starts with a game that hasn't loaded any cell
    customEventHooks.registerValidator("OnPlayerDisconnect", function(eventStatus, pid)
        if Players[pid] ~= nil then
            seenCells[playerKey(pid)] = nil
            regrown[playerKey(pid)] = nil
            cellLoadedAt[playerKey(pid)] = nil
            runsOwnCopy[playerKey(pid)] = nil
        end
    end)
    respawnTimer = tes3mp.CreateTimer("IsowindMmoRespawnTimer", time.seconds(CHECK_SECONDS))
    tes3mp.StartTimer(respawnTimer)
end)

return isowindMmo
