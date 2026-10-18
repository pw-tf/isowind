-- Isowind MMO mode (mode = mmo in the server's [Isowind] settings, see isowindHost.lua).
--
-- * Everyone loots for themselves: what a player takes from or puts in a container, a corpse, a plant or ore is
--   theirs alone (kept per player in the cell's data, isowindLoot, and replayed to them when they load the cell).
--   Containers players placed stay shared.
-- * Loose items from the game's files (not ones players dropped) are picked up per player too (isowindTaken).
-- * Quests, topics, factions, bounties, reputation and kill counts are each player's own, and so is what quest scripts
--   do to the world's own people and things: enabling or disabling them (isowindStates) or deleting them (isowindTaken),
--   so a quest that removes someone for one player leaves them there for the others.
-- * Enemies and NPCs come back after they die, once no player is in or next to their cell, after a time from how
--   tough they were (WoW-like tiers, from their health): trash 1.5 minutes, normal 5, elite 15, bosses 60, divided by
--   the host's respawn speed. Ones from the game's files are reset on each client ("isowind respawn", a console
--   command packet the game handles itself); leveled-list ones are spawned again where they first appeared.

if not IsowindHost.IsMmo() then
    return
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
    { name = "boss", minutes = 60 }
}
-- Bosses whatever their health
local bossPatterns = { "^dagoth", "^vivec_god", "^almalexia", "^karstaag", "^lich_", "^heart_akulakhan" }

local CHECK_SECONDS = 10

-- Plants, ore and kwama eggs a player harvested grow back for them after this many minutes (divided by the respawn
-- speed). Chests don't refill: players keep things in them.
local REGROW_MINUTES = 10
local regrowPatterns = { "^flora_", "^rock_", "^egg_kwama" }

-- This session's players (by name): the cells their game has loaded since they logged in, so has their own loot in
-- it, and what grew back there for them, to give their game when they load it again
local seenCells = {}
local regrown = {}

local isAreaEmpty

local function playerKey(pid)
    return string.lower(Players[pid].accountName)
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

    local name = playerKey(pid)
    cell.data.isowindTaken = cell.data.isowindTaken or {}
    tes3mp.ClearObjectList()
    tes3mp.SetObjectListPid(pid)
    tes3mp.SetObjectListCell(cellDescription)
    for uniqueIndex, object in pairs(mine) do
        cell.data.isowindTaken[uniqueIndex] = cell.data.isowindTaken[uniqueIndex] or {}
        cell.data.isowindTaken[uniqueIndex][name] = true
        packetBuilder.AddObjectDelete(uniqueIndex, object)
    end
    tes3mp.SendObjectDelete(false)

    -- The rest as the default handler does
    if next(shared) ~= nil then
        cell:SaveObjectsByPacketType("ObjectDelete", shared)
        cell:LoadObjectsByPacketType("ObjectDelete", pid, shared, tableHelper.getArrayFromIndexes(shared), true)
    end
    cell:QuicksaveToDrive()
    return customEventHooks.makeEventStatus(false, true)
end

-- A quest script enabling or disabling one of the game's own people or things: for that player only
local function onObjectState(eventStatus, pid, cellDescription, objects)
    if eventStatus.validDefaultHandler == false or not isFromScript(tes3mp.GetObjectListOrigin()) then
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

    local name = playerKey(pid)
    cell.data.isowindStates = cell.data.isowindStates or {}
    for uniqueIndex, object in pairs(mine) do
        local record = cell.data.isowindStates[uniqueIndex] or { refId = object.refId, players = {} }
        record.players[name] = object.state
        cell.data.isowindStates[uniqueIndex] = record
    end

    -- The rest as the default handler does
    if next(shared) ~= nil then
        cell:SaveObjectsByPacketType("ObjectState", shared)
        cell:LoadObjectsByPacketType("ObjectState", pid, shared, tableHelper.getArrayFromIndexes(shared), true)
    end
    cell:QuicksaveToDrive()
    return customEventHooks.makeEventStatus(false, true)
end

-- A player loading a cell gets their own loot and pickups there on top of the shared state just sent
local function onCellLoad(eventStatus, pid, cellDescription)
    local cell = LoadedCells[cellDescription]
    if cell == nil or Players[pid] == nil then
        return
    end
    local name = playerKey(pid)

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
        regrown[name][cellDescription] = nil
    end

    seenCells[name] = seenCells[name] or {}
    seenCells[name][cellDescription] = true
end

-- Plants and ore due to grow back, for players not near them: their harvest is forgotten, and a player whose game has
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
        if #ready > 0 and isAreaEmpty(cellDescription) then
            withCell(cellDescription, function(cell)
                local loot = cell.data.isowindLoot or {}
                for _, entry in ipairs(ready) do
                    local record = loot[entry.uniqueIndex]
                    local mine = record ~= nil and record.players[entry.name] or nil
                    if mine ~= nil then
                        record.players[entry.name] = nil
                        if seenCells[entry.name] ~= nil and seenCells[entry.name][cellDescription] then
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
        if not isSummon and (mpNum == 0 or spawnPoints[uniqueIndex] ~= nil) then
            local minutes = respawnMinutes(actor.refId or (objectData and objectData.refId), objectData)
            respawns[cellDescription] = respawns[cellDescription] or {}
            respawns[cellDescription][uniqueIndex] = now + math.floor(minutes * 60 / IsowindHost.respawnSpeed)
        end
    end
    WorldInstance:QuicksaveToDrive()
end

-- No player in the cell, or (outdoors) in the cells around it that their game has loaded
function isAreaEmpty(cellDescription)
    local loaded = LoadedCells[cellDescription]
    if loaded ~= nil and tableHelper.getCount(loaded.visitors) > 0 then
        return false
    end
    local x, y = cellDescription:match("^(%-?%d+), (%-?%d+)$")
    for _, player in pairs(Players) do
        if player:IsLoggedIn() and player.data.location ~= nil then
            local playerCell = player.data.location.cell
            if playerCell == cellDescription then
                return false
            end
            if x ~= nil then
                local px, py = playerCell:match("^(%-?%d+), (%-?%d+)$")
                if px ~= nil and math.abs(tonumber(px) - tonumber(x)) <= 1 and
                    math.abs(tonumber(py) - tonumber(y)) <= 1 then
                    return false
                end
            end
        end
    end
    return true
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
            if not moved and not disabled then
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
        if #ready > 0 and isAreaEmpty(cellDescription) then
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
    customEventHooks.registerHandler("OnCellLoad", onCellLoad)
    customEventHooks.registerHandler("OnObjectSpawn", onObjectSpawn)
    customEventHooks.registerHandler("OnActorDeath", onActorDeath)
    -- A player who logs in again starts with a game that hasn't loaded any cell
    customEventHooks.registerValidator("OnPlayerDisconnect", function(eventStatus, pid)
        if Players[pid] ~= nil then
            seenCells[playerKey(pid)] = nil
            regrown[playerKey(pid)] = nil
        end
    end)
    respawnTimer = tes3mp.CreateTimer("IsowindMmoRespawnTimer", time.seconds(CHECK_SECONDS))
    tes3mp.StartTimer(respawnTimer)
end)

return isowindMmo
