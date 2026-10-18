-- How to run: copy this into a copy of the server folder as CoreScripts/scripts/custom/mmotest.lua, add
-- require("custom/mmotest") at the end of customScripts.lua, put mode = mmo under [Isowind] in tes3mp-server.cfg and
-- start tes3mp-server for a few seconds; the log shows "[mmotest] PASS/FAIL ..." and "done, N failed".
-- Tests of isowindMmo.lua inside a real server: fake players and packets, real cells and world data.
local log = function(text) tes3mp.LogMessage(enumerations.log.INFO, "[mmotest] " .. text) end
local failures = 0
local function check(name, condition)
    if condition then log("PASS " .. name) else log("FAIL " .. name); failures = failures + 1 end
end

-- Every Set/Add/Send/Clear/Copy call is recorded instead of done; Get* answer from the packet being "received"
local sent = {}
local current = {}
local incoming = nil
local function record(name)
    return function(...)
        local args = { ... }
        if name == "ClearObjectList" then current = { objects = {}, object = { items = {} }, item = {} }
        elseif name == "SetObjectRefNum" then current.object.refNum = args[1]
        elseif name == "SetObjectMpNum" then current.object.mpNum = args[1]
        elseif name == "SetObjectRefId" then current.object.refId = args[1]
        elseif name == "SetContainerItemRefId" then current.item.refId = args[1]
        elseif name == "SetContainerItemCount" then current.item.count = args[1]
        elseif name == "SetContainerItemActionCount" then current.item.actionCount = args[1]
        elseif name == "AddContainerItem" then table.insert(current.object.items, current.item); current.item = {}
        elseif name == "AddObject" then table.insert(current.objects, current.object); current.object = { items = {} }
        elseif name == "SetObjectListAction" then current.action = args[1]
        elseif name == "SetObjectListConsoleCommand" then current.command = args[1]
        elseif name == "SetContainerItemActionCountByIndex" then
            table.insert(sent, { name = name, args = args })
        elseif name:find("^Send") then
            table.insert(sent, { name = name, args = args, action = current.action, command = current.command,
                objects = current.objects })
        end
    end
end
local function installMocks()
for name, fn in pairs(tes3mp) do
    if type(name) == "string" and (name:find("^Set") or name:find("^Add") or name:find("^Send") or
        name:find("^Clear") or name == "CopyReceivedObjectListToStore") then
        tes3mp[name] = record(name)
    end
end
current = { objects = {}, object = { items = {} }, item = {} }
end

local function receive(packet)
    incoming = packet
end
tes3mp.ReadReceivedObjectList = function() end
tes3mp.GetObjectListAction = function() return incoming.action end
tes3mp.GetObjectListContainerSubAction = function() return incoming.subAction or 0 end
tes3mp.GetObjectListOrigin = function() return incoming.origin or enumerations.packetOrigin.CLIENT_GAMEPLAY end
tes3mp.GetObjectListSize = function() return #incoming.objects end
tes3mp.GetObjectRefNum = function(i) return incoming.objects[i + 1].refNum end
tes3mp.GetObjectMpNum = function(i) return incoming.objects[i + 1].mpNum end
tes3mp.GetObjectRefId = function(i) return incoming.objects[i + 1].refId end
tes3mp.GetContainerChangesSize = function(i) return #incoming.objects[i + 1].items end
local function item(i, j) return incoming.objects[i + 1].items[j + 1] end
tes3mp.GetContainerItemRefId = function(i, j) return item(i, j).refId end
tes3mp.GetContainerItemCount = function(i, j) return item(i, j).count end
tes3mp.GetContainerItemActionCount = function(i, j) return item(i, j).actionCount or 0 end
tes3mp.GetContainerItemCharge = function() return -1 end
tes3mp.GetContainerItemEnchantmentCharge = function() return -1 end
tes3mp.GetContainerItemSoul = function() return "" end

local function lastSent(name)
    for i = #sent, 1, -1 do if sent[i].name == name then return sent[i] end end
end

local function validate(event, args)
    return customEventHooks.triggerValidators(event, args)
end

local function runTests()
    installMocks()
    local CELL = "-3, 6"
    local pid = 0
    Players[pid] = { pid = pid, accountName = "Alpha", name = "Alpha", consoleCommandsQueued = {}, summons = {},
        data = { location = { cell = CELL }, alliedPlayers = {} }, IsLoggedIn = function() return true end,
        QuicksaveToDrive = function() end, Message = function() end }
    logicHandler.LoadCell(CELL)
    local cell = LoadedCells[CELL]
    cell.visitors = { pid }
    local ADD, REMOVE = enumerations.container.ADD, enumerations.container.REMOVE

    -- Taking from a chest is this player's own, and only they get it back
    receive({ action = REMOVE, objects = { { refNum = 1234, mpNum = 0, refId = "contain_chest",
        items = { { refId = "gold_001", count = 50, actionCount = 20 } } } } })
    local status = validate("OnContainer", { pid, CELL, {} })
    check("container change skips the shared save", status.validDefaultHandler == false)
    local echo = lastSent("SendContainer")
    check("container change sent back to the taker only", echo ~= nil and echo.args[1] == false and echo.args[2] == false)
    local mine = cell.data.isowindLoot["1234-0"].players.alpha
    local key = next(mine)
    check("loot recorded as -20 gold", key ~= nil and mine[key].count == -20)
    check("chest not marked to refill", WorldInstance.data.isowindRegrow == nil or
        WorldInstance.data.isowindRegrow[CELL] == nil)

    -- Loading the cell gives the player their own state on top of the shared one
    sent = {}
    customEventHooks.triggerHandlers("OnCellLoad", customEventHooks.makeEventStatus(true, true), { pid, CELL })
    local replay = lastSent("SendContainer")
    check("cell load replays the loot as a REMOVE of 20",
        replay ~= nil and replay.action == REMOVE and replay.objects[1].items[1].actionCount == 20 and
        replay.objects[1].refId == "contain_chest")

    -- Putting it back cancels it
    receive({ action = ADD, objects = { { refNum = 1234, mpNum = 0, refId = "contain_chest",
        items = { { refId = "gold_001", count = 20 } } } } })
    validate("OnContainer", { pid, CELL, {} })
    check("putting it back clears the record", cell.data.isowindLoot["1234-0"].players.alpha == nil)

    -- Replies to the server's requests stay shared
    receive({ action = enumerations.container.SET, subAction = enumerations.containerSub.REPLY_TO_REQUEST,
        objects = { { refNum = 1234, mpNum = 0, refId = "contain_chest", items = {} } } })
    status = validate("OnContainer", { pid, CELL, {} })
    check("request reply goes to the default handler", status.validDefaultHandler ~= false)

    -- Picking up a loose item from the game's files
    sent = {}
    receive({ objects = {} })
    status = validate("OnObjectDelete", { pid, CELL, { ["5678-0"] = { refId = "misc_com_bottle_01",
        uniqueIndex = "5678-0" } }, {} })
    check("pickup skips the shared save", status.validDefaultHandler == false)
    check("pickup recorded for the player", cell.data.isowindTaken["5678-0"].alpha == true)
    local delete = lastSent("SendObjectDelete")
    check("pickup sent to the player only", delete ~= nil and delete.args[1] == false)
    check("pickup not saved as shared", not tableHelper.containsValue(cell.data.packets.delete or {}, "5678-0"))

    -- A dead mudcrab comes back after 1.5 minutes, once no one is near
    cell.data.objectData["999-0"] = { refId = "mudcrab", stats = { healthBase = 35 } }
    customEventHooks.triggerHandlers("OnActorDeath", customEventHooks.makeEventStatus(true, true),
        { pid, CELL, { ["999-0"] = { refId = "mudcrab", killer = {} } } })
    local due = WorldInstance.data.isowindRespawns[CELL]["999-0"]
    check("mudcrab due in about 90 seconds", due ~= nil and math.abs(due - os.time() - 90) <= 2)

    cell.data.objectData["998-0"] = { refId = "dagoth gares", stats = { healthBase = 300 } }
    cell.data.objectData["997-0"] = { refId = "ordinator", stats = { healthBase = 250 } }
    customEventHooks.triggerHandlers("OnActorDeath", customEventHooks.makeEventStatus(true, true),
        { pid, CELL, { ["998-0"] = { refId = "dagoth gares", killer = {} },
            ["997-0"] = { refId = "ordinator", killer = {} } } })
    check("Dagoth is a boss (60 minutes)",
        math.abs(WorldInstance.data.isowindRespawns[CELL]["998-0"] - os.time() - 3600) <= 2)
    check("250 health is elite (15 minutes)",
        math.abs(WorldInstance.data.isowindRespawns[CELL]["997-0"] - os.time() - 900) <= 2)

    -- MMO kill counts are each player's own
    check("killer's own kill count is 1", Players[pid].data.kills ~= nil and Players[pid].data.kills.mudcrab == 1)
    local killCount = lastSent("SendWorldKillCount")
    check("kill count sent to the killer only", killCount ~= nil and killCount.args[1] == pid and
        killCount.args[2] == false)

    -- A body whose contents never come unlocks after 5 seconds
    check("body locked right after death", tableHelper.containsValue(cell.unusableContainerUniqueIndexes, "999-0"))
    local realTime = os.time
    os.time = function() return realTime() + 10 end
    IsowindUnlockCorpses()
    os.time = realTime
    check("body unlocked after 5 seconds", not tableHelper.containsValue(cell.unusableContainerUniqueIndexes, "999-0"))

    -- Taking from a container whose contents the server doesn't know (as co-op does it) goes through
    sent = {}
    receive({ action = REMOVE, objects = { { refNum = 777, mpNum = 0, refId = "contain_barrel",
        items = { { refId = "ingred_bread_01", count = 3, actionCount = 2 } } } } })
    cell:SaveContainers(pid)
    check("take from unknown contents not refused", lastSent("SetContainerItemActionCountByIndex") == nil)
    check("unknown contents stay unknown", cell.data.objectData["777-0"].inventory == nil)
    cell.data.objectData["777-0"].inventory = {}
    receive({ action = REMOVE, objects = { { refNum = 777, mpNum = 0, refId = "contain_barrel",
        items = { { refId = "ingred_bread_01", count = 3, actionCount = 2 } } } } })
    cell:SaveContainers(pid)
    local refused = lastSent("SetContainerItemActionCountByIndex")
    check("take of something known not to be there still refused", refused ~= nil and refused.args[3] == 0)

    -- One a quest script disabled after it died
    cell.data.objectData["996-0"] = { refId = "quest_npc", state = false, stats = { healthBase = 40 } }
    customEventHooks.triggerHandlers("OnActorDeath", customEventHooks.makeEventStatus(true, true),
        { pid, CELL, { ["996-0"] = { refId = "quest_npc", killer = {} } } })
    WorldInstance.data.isowindRespawns[CELL]["996-0"] = os.time() - 1

    -- Not while the player is there
    WorldInstance.data.isowindRespawns[CELL]["999-0"] = os.time() - 1
    sent = {}
    IsowindMmoRespawnTimer()
    check("no respawn with the player in the cell", lastSent("SendConsoleCommand") == nil)

    -- The player walks off, next to the cell: still not
    Players[pid].data.location.cell = "-2, 6"
    cell.visitors = {}
    IsowindMmoRespawnTimer()
    check("no respawn with the player next door", lastSent("SendConsoleCommand") == nil)

    -- Far away: it respawns
    Players[pid].data.location.cell = "10, 10"
    logicHandler.UnloadCell(CELL)
    IsowindMmoRespawnTimer()
    local respawn = lastSent("SendConsoleCommand")
    check("respawn sent to everyone", respawn ~= nil and respawn.command == "isowind respawn" and
        respawn.args[1] == true)
    check("respawn names the mudcrab", respawn ~= nil and respawn.objects[1] ~= nil and
        respawn.objects[1].refNum == "999" or (respawn ~= nil and respawn.objects[1] ~= nil and
        tostring(respawn.objects[1].refNum) == "999"))
    logicHandler.LoadCell(CELL)
    cell = LoadedCells[CELL]
    check("mudcrab's death forgotten", cell.data.objectData["999-0"] == nil)
    check("disabled one not brought back", cell.data.objectData["996-0"] ~= nil and
        cell.data.objectData["996-0"].state == false)
    check("boss still waiting", WorldInstance.data.isowindRespawns[CELL]["998-0"] ~= nil)

    -- Plants grow back for the harvester
    Players[pid].data.location.cell = CELL
    receive({ action = REMOVE, objects = { { refNum = 4321, mpNum = 0, refId = "flora_kreshweed_01",
        items = { { refId = "ingred_kresh_fiber_01", count = 1, actionCount = 1 } } } } })
    validate("OnContainer", { pid, CELL, {} })
    local regrow = WorldInstance.data.isowindRegrow[CELL]["4321-0"].alpha
    check("plant regrowth due in 10 minutes", regrow ~= nil and math.abs(regrow - os.time() - 600) <= 2)
    WorldInstance.data.isowindRegrow[CELL]["4321-0"].alpha = os.time() - 1
    Players[pid].data.location.cell = "10, 10"
    cell.visitors = {}
    logicHandler.UnloadCell(CELL)
    IsowindMmoRespawnTimer()
    logicHandler.LoadCell(CELL)
    cell = LoadedCells[CELL]
    check("harvest forgotten after regrowth", cell.data.isowindLoot["4321-0"].players.alpha == nil)
    sent = {}
    Players[pid].data.location.cell = CELL
    customEventHooks.triggerHandlers("OnCellLoad", customEventHooks.makeEventStatus(true, true), { pid, CELL })
    local grown = nil
    for _, packet in ipairs(sent) do
        if packet.name == "SendContainer" and packet.action == ADD then grown = packet end
    end
    check("plant given back to the player's game as an ADD of 1", grown ~= nil and
        grown.objects[1].items[1].count == 1 and grown.objects[1].items[1].refId == "ingred_kresh_fiber_01")

    log("done, " .. failures .. " failed")
end

customEventHooks.registerHandler("OnServerPostInit", function()
    local ok, err = pcall(runTests)
    if not ok then log("ERROR " .. tostring(err)) end
end)
