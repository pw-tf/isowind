-- Isowind MMO mode: the locks and traps of the game's own doors and containers are each player's own, shared with
-- their party (isowindParty.lua) as quests are. A chest one player picked, unlocked with its key, opened with a spell or
-- had a quest script unlock stays locked for everyone else, and a trap one disarmed or set off is still there for them.
--
-- * Kept in the cell's data, per player: isowindLocks (the lock level they have, 0 for unlocked) and isowindTraps (the
--   traps gone for them). A change is written for the party's members at the time of it; someone who joins the party
--   later doesn't get it, as with what quests change in the world (isowindMmo.lua's isowindStates).
-- * The player's game makes the change when the server sends it back (it waits for that), and the party's other
--   members who are on get it at once. Each player's are sent again whenever their game loads the cell, after the
--   shared state.
-- * Things players placed stay shared, and so do the console's changes. A door's being open or closed stays shared too:
--   one a player unlocked and opened is open for everyone, though still locked for the others.
-- Co-op servers don't load this: their world is one, locks and traps included.

if not IsowindHost.IsMmo() then
    return
end

local isowindLocks = {}

-- The same helpers as isowindMmo.lua's
local function playerKey(pid)
    return string.lower(Players[pid].accountName)
end

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

-- Who shares a player's change: their party, or just them (isowindMmo.lua's questSharers)
local function sharers(pid)
    local name = playerKey(pid)
    if IsowindParty ~= nil then
        return IsowindParty.GetMemberNames(name)
    end
    return { name }
end

-- The player's hands, spells and keys, and quest scripts and dialogue; not the console
local function isPlayersOwn(packetOrigin)
    return packetOrigin == enumerations.packetOrigin.CLIENT_GAMEPLAY or
        packetOrigin == enumerations.packetOrigin.CLIENT_SCRIPT_LOCAL or
        packetOrigin == enumerations.packetOrigin.CLIENT_SCRIPT_GLOBAL or
        packetOrigin == enumerations.packetOrigin.CLIENT_DIALOGUE
end

-- The two kinds of change, by packet. A trap is either there or gone: the game doesn't tell the server whether it was
-- disarmed or set off, and what the server sends always says disarmed (set off would set it off again in every game)
local kinds = {
    {
        packetType = "ObjectLock",
        field = "isowindLocks",
        value = function(object) return object.lockLevel end,
        add = function(uniqueIndex, record, value)
            packetBuilder.AddObjectLock(uniqueIndex, { refId = record.refId, lockLevel = value })
        end,
        send = function() tes3mp.SendObjectLock(false, false) end
    },
    {
        packetType = "ObjectTrap",
        field = "isowindTraps",
        value = function() return true end,
        add = function(uniqueIndex, record) packetBuilder.AddObjectTrap(uniqueIndex, { refId = record.refId }) end,
        send = function() tes3mp.SendObjectTrap(false, false) end
    }
}

-- A lock or trap change from a player: the game's own objects in it become theirs and their party's
local function onChange(kind, eventStatus, pid, cellDescription, objects)
    if eventStatus.validDefaultHandler == false or not isPlayersOwn(tes3mp.GetObjectListOrigin()) then
        return
    end
    local cell = LoadedCells[cellDescription]
    if cell == nil or Players[pid] == nil then
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

    local names = sharers(pid)
    cell.data[kind.field] = cell.data[kind.field] or {}
    local records = cell.data[kind.field]
    for uniqueIndex, object in pairs(mine) do
        local record = records[uniqueIndex] or { refId = object.refId, players = {} }
        for _, name in ipairs(names) do
            record.players[name] = kind.value(object)
        end
        records[uniqueIndex] = record
    end

    local recipients = otherOnlinePids(pid, names)
    table.insert(recipients, pid)
    for _, recipient in ipairs(recipients) do
        tes3mp.ClearObjectList()
        tes3mp.SetObjectListPid(recipient)
        tes3mp.SetObjectListCell(cellDescription)
        for uniqueIndex, object in pairs(mine) do
            kind.add(uniqueIndex, records[uniqueIndex], kind.value(object))
        end
        kind.send()
    end

    -- The rest as the default handler does
    if next(shared) ~= nil then
        cell:SaveObjectsByPacketType(kind.packetType, shared)
        cell:LoadObjectsByPacketType(kind.packetType, pid, shared, tableHelper.getArrayFromIndexes(shared), true)
    end
    cell:QuicksaveToDrive()
    return customEventHooks.makeEventStatus(false, true)
end

-- A player loading a cell gets their own locks and traps there, on top of the shared state just sent (their game makes
-- the same change twice harmlessly, mwmp::ObjectList::lockObjects and triggerTrapObjects)
local function onCellLoad(eventStatus, pid, cellDescription)
    local cell = LoadedCells[cellDescription]
    if cell == nil or Players[pid] == nil then
        return
    end
    local name = playerKey(pid)

    for _, kind in ipairs(kinds) do
        local records = cell.data[kind.field]
        if records ~= nil then
            tes3mp.ClearObjectList()
            tes3mp.SetObjectListPid(pid)
            tes3mp.SetObjectListCell(cellDescription)
            local count = 0
            for uniqueIndex, record in pairs(records) do
                local value = record.players[name]
                if value ~= nil then
                    kind.add(uniqueIndex, record, value)
                    count = count + 1
                end
            end
            if count > 0 then
                kind.send()
            end
        end
    end
end

-- After the server's own validators (added when it starts), so these see what they decided
customEventHooks.registerHandler("OnServerPostInit", function(eventStatus)
    for _, kind in ipairs(kinds) do
        customEventHooks.registerValidator("On" .. kind.packetType, function(eventStatus, pid, cellDescription, objects)
            return onChange(kind, eventStatus, pid, cellDescription, objects)
        end)
    end
    customEventHooks.registerHandler("OnCellLoad", onCellLoad)
end)

return isowindLocks
