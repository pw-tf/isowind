-- How to run: copy this into a copy of the server folder as CoreScripts/scripts/custom/, add
-- require("custom/locktest") at the end of customScripts.lua, put mode = mmo under [Isowind] in tes3mp-server.cfg and
-- start tes3mp-server for a few seconds; the log shows PASS/FAIL lines and "done, N failed".
-- Tests of isowindLocks.lua inside a real server: three fake players and packets, a real party and cell.
local log = function(text) tes3mp.LogMessage(enumerations.log.INFO, "[locktest] " .. text) end
local failures = 0
local function check(name, condition)
    if condition then log("PASS " .. name) else log("FAIL " .. name); failures = failures + 1 end
end

-- Every Set/Add/Send/Clear/Copy call is recorded instead of done, with whom each packet was for and what it carried
local sent = {}
local current = {}
local incoming = { origin = enumerations.packetOrigin.CLIENT_GAMEPLAY }
local function installMocks()
    for name, fn in pairs(tes3mp) do
        if type(name) == "string" and (name:find("^Set") or name:find("^Add") or name:find("^Send") or
            name:find("^Clear") or name == "CopyReceivedObjectListToStore") then
            tes3mp[name] = function(...)
                local args = { ... }
                if name == "ClearObjectList" then current = { objects = {}, object = {} }
                elseif name == "SetObjectListPid" then current.pid = args[1]
                elseif name == "SetObjectRefNum" then current.object.refNum = tostring(args[1])
                elseif name == "SetObjectMpNum" then current.object.mpNum = tostring(args[1])
                elseif name == "SetObjectRefId" then current.object.refId = args[1]
                elseif name == "SetObjectLockLevel" then current.object.lockLevel = args[1]
                elseif name == "SetObjectDisarmState" then current.object.disarmed = args[1]
                elseif name == "AddObject" then table.insert(current.objects, current.object); current.object = {}
                elseif name:find("^Send") then
                    table.insert(sent, { name = name, args = args, pid = current.pid, objects = current.objects })
                end
            end
        end
    end
    current = { objects = {}, object = {} }
    tes3mp.ReadReceivedObjectList = function() end
    tes3mp.GetObjectListOrigin = function() return incoming.origin end
    tes3mp.GetTopicChangesSize = function() return 0 end
end

local function packetsNamed(name)
    local list = {}
    for _, packet in ipairs(sent) do
        if packet.name == name then table.insert(list, packet) end
    end
    return list
end
local function sentTo(name, pid)
    for _, packet in ipairs(sent) do
        if packet.name == name and packet.pid == pid then return packet end
    end
end
local function carries(packet, refNum)
    if packet == nil then return nil end
    for _, object in ipairs(packet.objects) do
        if object.refNum == refNum then return object end
    end
end

local function makePlayer(pid, name)
    local player = { pid = pid, accountName = name, name = name, consoleCommandsQueued = {}, summons = {},
        data = { location = { cell = "-3, 6" }, alliedPlayers = {}, journal = {}, topics = {},
            clientVariables = { globals = {} }, customVariables = {} } }
    player.IsLoggedIn = function() return true end
    player.QuicksaveToDrive = function() end
    player.Save = function() end
    player.Message = function() end
    player.LoadAllies = function() end
    Players[pid] = player
    return player
end
local function command(pid, text)
    local cmd = text:split(" ")
    customCommandHooks.getCallback(cmd[1])(pid, cmd)
end

local CELL = "-3, 6"
local function lockPacket(pid, origin, objects)
    incoming.origin = origin or enumerations.packetOrigin.CLIENT_GAMEPLAY
    return customEventHooks.triggerValidators("OnObjectLock", { pid, CELL, objects, {} })
end
local function trapPacket(pid, objects)
    incoming.origin = enumerations.packetOrigin.CLIENT_GAMEPLAY
    return customEventHooks.triggerValidators("OnObjectTrap", { pid, CELL, objects, {} })
end
local function lockObject(refNum, mpNum, refId, lockLevel)
    local uniqueIndex = refNum .. "-" .. mpNum
    return { [uniqueIndex] = { refId = refId, uniqueIndex = uniqueIndex, lockLevel = lockLevel } }
end
local function loadCell(pid)
    sent = {}
    customEventHooks.triggerHandlers("OnCellLoad", customEventHooks.makeEventStatus(true, true), { pid, CELL })
end

local function runTests()
    installMocks()
    WorldInstance.data.isowindParties = nil
    makePlayer(0, "Alpha")
    makePlayer(1, "Beta")
    makePlayer(2, "Gamma")
    logicHandler.LoadCell(CELL)
    local cell = LoadedCells[CELL]

    -- Alone: a pick is the picker's own
    sent = {}
    local status = lockPacket(0, nil, lockObject(8101, 0, "contain_chest_locked", 0))
    check("pick skips the shared save", status.validDefaultHandler == false)
    local locks = cell.data.isowindLocks
    check("pick kept for the picker", locks ~= nil and locks["8101-0"].players.alpha == 0)
    check("pick not for the others", locks["8101-0"].players.beta == nil and locks["8101-0"].players.gamma == nil)
    check("pick not in the shared state", not tableHelper.containsValue(cell.data.packets.lock or {}, "8101-0") and
        (cell.data.objectData["8101-0"] == nil or cell.data.objectData["8101-0"].lockLevel == nil))
    local packets = packetsNamed("SendObjectLock")
    local back = packets[1]
    check("pick sent back to the picker only, once", #packets == 1 and back.pid == 0 and back.args[1] == false and
        back.args[2] == false)
    check("send-back unlocks the chest", carries(back, "8101") ~= nil and carries(back, "8101").lockLevel == 0 and
        carries(back, "8101").refId == "contain_chest_locked")

    -- Loading the cell: the picker's game gets it again, the others' don't
    loadCell(0)
    check("cell load replays the pick to the picker", carries(sentTo("SendObjectLock", 0), "8101") ~= nil and
        #packetsNamed("SendObjectLock") == 1)
    loadCell(1)
    check("cell load gives the others nothing", #packetsNamed("SendObjectLock") == 0 and
        #packetsNamed("SendObjectTrap") == 0)

    -- Kept on the drive: still there after the cell is unloaded and loaded again
    logicHandler.UnloadCell(CELL)
    logicHandler.LoadCell(CELL)
    cell = LoadedCells[CELL]
    loadCell(0)
    check("pick kept through an unload", carries(sentTo("SendObjectLock", 0), "8101") ~= nil)

    -- Something a player placed stays shared
    sent = {}
    status = lockPacket(0, nil, lockObject(8102, 5, "contain_placed", 0))
    check("placed object goes to the default handler", status.validDefaultHandler ~= false and
        cell.data.isowindLocks["8102-5"] == nil)

    -- Both in one packet: the game's own one is the player's, the placed one shared (sent to everyone)
    sent = {}
    local both = lockObject(8103, 0, "door_locked", 0)
    both["8104-7"] = { refId = "contain_placed", uniqueIndex = "8104-7", lockLevel = 0 }
    status = lockPacket(0, nil, both)
    check("mixed packet skips the default handler", status.validDefaultHandler == false)
    check("mixed packet: own one kept per player", cell.data.isowindLocks["8103-0"].players.alpha == 0 and
        cell.data.isowindLocks["8104-7"] == nil)
    check("mixed packet: placed one saved as shared", tableHelper.containsValue(cell.data.packets.lock, "8104-7") and
        not tableHelper.containsValue(cell.data.packets.lock, "8103-0"))
    local toEveryone = nil
    for _, packet in ipairs(packetsNamed("SendObjectLock")) do
        if packet.args[1] == true then toEveryone = packet end
    end
    check("mixed packet: placed one sent to everyone, without the own one", carries(toEveryone, "8104") ~= nil and
        carries(toEveryone, "8103") == nil)

    -- The console stays shared
    status = lockPacket(0, enumerations.packetOrigin.CLIENT_CONSOLE, lockObject(8106, 0, "door_console", 0))
    check("console change goes to the default handler", status.validDefaultHandler ~= false and
        cell.data.isowindLocks["8106-0"] == nil)

    -- One the server's config refuses is left alone
    local disallowed = config.disallowedLockRefIds
    config.disallowedLockRefIds = { "door_refused" }
    status = lockPacket(0, nil, lockObject(8110, 0, "door_refused", 0))
    config.disallowedLockRefIds = disallowed
    check("refused lock change isn't recorded", status.validDefaultHandler == false and
        cell.data.isowindLocks["8110-0"] == nil)

    -- Alone: a disarmed trap is the disarmer's own
    sent = {}
    status = trapPacket(1, { ["8107-0"] = { refId = "contain_trapped", uniqueIndex = "8107-0" } })
    check("disarm skips the shared save", status.validDefaultHandler == false and
        not tableHelper.containsValue(cell.data.packets.trap or {}, "8107-0"))
    check("disarm kept for the disarmer only", cell.data.isowindTraps["8107-0"].players.beta == true and
        cell.data.isowindTraps["8107-0"].players.alpha == nil)
    local trapBack = packetsNamed("SendObjectTrap")
    check("disarm sent back to the disarmer only, as disarmed", #trapBack == 1 and trapBack[1].pid == 1 and
        carries(trapBack[1], "8107") ~= nil and carries(trapBack[1], "8107").disarmed == true)

    -- A party: Alpha and Beta
    command(0, "party invite Beta")
    command(1, "party accept")
    check("party formed", #IsowindParty.GetMemberNames("alpha") == 2)

    -- Beta picks: the whole party has it, and the online members are told
    sent = {}
    lockPacket(1, nil, lockObject(8105, 0, "contain_chest_party", 0))
    local record = cell.data.isowindLocks["8105-0"]
    check("party pick kept for both members", record.players.alpha == 0 and record.players.beta == 0)
    check("party pick not for someone outside it", record.players.gamma == nil)
    check("party pick sent back to the picker", carries(sentTo("SendObjectLock", 1), "8105") ~= nil)
    check("party pick sent to the other member", carries(sentTo("SendObjectLock", 0), "8105") ~= nil)
    check("party pick not sent outside the party", sentTo("SendObjectLock", 2) == nil)

    -- A Lock spell on it afterwards locks it again for the party
    lockPacket(0, nil, lockObject(8105, 0, "contain_chest_party", 50))
    check("Lock spell kept for the party", record.players.alpha == 50 and record.players.beta == 50)

    -- Alpha sets off a trap: gone for the party, and what is sent says disarmed (not set off again)
    sent = {}
    trapPacket(0, { ["8108-0"] = { refId = "door_trapped", uniqueIndex = "8108-0" } })
    local sprung = cell.data.isowindTraps["8108-0"]
    check("sprung trap gone for both members", sprung.players.alpha == true and sprung.players.beta == true and
        sprung.players.gamma == nil)
    check("sprung trap sent to both as disarmed", carries(sentTo("SendObjectTrap", 0), "8108") ~= nil and
        carries(sentTo("SendObjectTrap", 0), "8108").disarmed == true and
        carries(sentTo("SendObjectTrap", 1), "8108") ~= nil and sentTo("SendObjectTrap", 2) == nil)

    -- Loading the cell: Beta gets the party's and their own, Gamma nothing
    loadCell(1)
    local betaLocks = sentTo("SendObjectLock", 1)
    check("Beta's replay: the party's lock", carries(betaLocks, "8105") ~= nil and
        carries(betaLocks, "8105").lockLevel == 50)
    check("Beta's replay: not Alpha's pick from before the party", carries(betaLocks, "8101") == nil)
    local betaTraps = sentTo("SendObjectTrap", 1)
    check("Beta's replay: both traps", carries(betaTraps, "8107") ~= nil and carries(betaTraps, "8108") ~= nil)
    loadCell(2)
    check("Gamma's replay: nothing", #sent == 0)

    -- Gamma joins later: not the party's earlier changes, but the ones after
    command(0, "party invite Gamma")
    command(2, "party accept")
    check("joining later doesn't give earlier changes", record.players.gamma == nil and sprung.players.gamma == nil)
    sent = {}
    lockPacket(0, enumerations.packetOrigin.CLIENT_SCRIPT_GLOBAL, lockObject(8109, 0, "door_quest", 0))
    local quest = cell.data.isowindLocks["8109-0"]
    check("quest script unlock kept for the whole party", quest.players.alpha == 0 and quest.players.beta == 0 and
        quest.players.gamma == 0)
    check("quest script unlock sent to all three", sentTo("SendObjectLock", 0) ~= nil and
        sentTo("SendObjectLock", 1) ~= nil and sentTo("SendObjectLock", 2) ~= nil)

    log("done, " .. failures .. " failed")
end

customEventHooks.registerHandler("OnServerPostInit", function()
    local ok, err = pcall(runTests)
    if not ok then log("ERROR " .. tostring(err)) end
end)
