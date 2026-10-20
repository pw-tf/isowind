-- How to run: copy this into a copy of the server folder as CoreScripts/scripts/custom/, add
-- require("custom/eventtest") at the end of customScripts.lua, put mode = mmo under [Isowind] in tes3mp-server.cfg and
-- start tes3mp-server for a few seconds; the log shows PASS/FAIL lines and "done, N failed".
-- Tests of isowindMmo.lua's one-off events (Tarhiel falling near Seyda Neen) inside a real server: fake players and
-- packets, real cells and world data.
local log = function(text) tes3mp.LogMessage(enumerations.log.INFO, "[eventtest] " .. text) end
local failures = 0
local function check(name, condition)
    if condition then log("PASS " .. name) else log("FAIL " .. name); failures = failures + 1 end
end

-- Every Set/Add/Send/Clear call is recorded instead of done
local sent = {}
local current = {}
local function newList()
    return { objects = {}, object = { locals = {} } }
end
local function installMocks()
    for name, fn in pairs(tes3mp) do
        if type(name) == "string" and (name:find("^Set") or name:find("^Add") or name:find("^Send") or
            name:find("^Clear") or name == "CopyReceivedObjectListToStore") then
            tes3mp[name] = function(...)
                local args = { ... }
                if name == "ClearObjectList" then current = newList()
                elseif name == "SetObjectListPid" then current.pid = args[1]
                elseif name == "SetObjectListConsoleCommand" then current.command = args[1]
                elseif name == "SetObjectRefNum" then current.object.refNum = tostring(args[1])
                elseif name == "SetObjectMpNum" then current.object.mpNum = tostring(args[1])
                elseif name == "SetObjectRefId" then current.object.refId = args[1]
                elseif name == "SetObjectState" then current.object.state = args[1]
                elseif name == "AddClientLocalInteger" then
                    table.insert(current.object.locals, { index = args[1], value = args[2], type = args[3] })
                elseif name == "AddClientLocalFloat" then
                    table.insert(current.object.locals, { index = args[1], value = args[2], type = 2 })
                elseif name == "AddObject" then
                    table.insert(current.objects, current.object)
                    current.object = { locals = {} }
                elseif name:find("^Send") then
                    table.insert(sent, { name = name, args = args, pid = current.pid, command = current.command,
                        objects = current.objects })
                end
            end
        end
    end
    current = newList()
end

local incoming = { origin = enumerations.packetOrigin.CLIENT_SCRIPT_LOCAL, script = "" }
tes3mp.ReadReceivedObjectList = function() end
tes3mp.GetObjectListOrigin = function() return incoming.origin end
tes3mp.GetObjectListClientScript = function() return incoming.script end
local function receive(origin, script)
    incoming = { origin = origin, script = script or "" }
end

-- Where the fake players are
local places = {}
tes3mp.GetCell = function(pid) return places[pid].cell end
tes3mp.IsInExterior = function(pid) return places[pid].exterior end
tes3mp.GetPosX = function(pid) return places[pid].x end
tes3mp.GetPosY = function(pid) return places[pid].y end

local function sentTo(name, pid)
    local found = {}
    for _, packet in ipairs(sent) do
        if packet.name == name and packet.pid == pid then table.insert(found, packet) end
    end
    return found
end
local function firstIndexOf(name, pid)
    for i, packet in ipairs(sent) do
        if packet.name == name and packet.pid == pid then return i end
    end
end
local function hasObject(packet, refNum, state)
    for _, object in ipairs(packet.objects) do
        if object.refNum == refNum and (state == nil or object.state == state) then return true end
    end
    return false
end
local function localValue(packet, refNum, index)
    for _, object in ipairs(packet.objects) do
        if object.refNum == refNum then
            for _, entry in ipairs(object.locals) do
                if entry.index == index and entry.type ~= 2 then return entry.value end
            end
        end
    end
end

local function validate(event, args)
    return customEventHooks.triggerValidators(event, args)
end

local LOCAL = enumerations.packetOrigin.CLIENT_SCRIPT_LOCAL
local SHORT, FLOAT = enumerations.variableType.SHORT, enumerations.variableType.FLOAT
local CELL = "-3, -8"
local TARHIEL, PLATFORM, BOOK = "474443-0", "476745-0", "476746-0"

local function makePlayer(pid, name, place)
    Players[pid] = { pid = pid, accountName = name, name = name, consoleCommandsQueued = {}, summons = {},
        data = { location = { cell = place.cell }, alliedPlayers = {} }, IsLoggedIn = function() return true end,
        QuicksaveToDrive = function() end, Message = function() end }
    places[pid] = place
end

local function enableTarhiel(pid)
    receive(LOCAL, "fallingScript")
    return validate("OnObjectState", { pid, CELL, { [TARHIEL] = { refId = "agronian guy", uniqueIndex = TARHIEL,
        state = true } }, {} })
end
local function setFellonce(pid, value, timer)
    receive(LOCAL, "fallingScript")
    local variables = { [SHORT] = { [0] = value } }
    if timer ~= nil then variables[FLOAT] = { [0] = timer } end
    return validate("OnClientScriptLocal", { pid, CELL, { [BOOK] = { refId = "bk_falljournal_unique",
        uniqueIndex = BOOK, variables = variables } }, {} })
end

local function runTests()
    installMocks()
    WorldInstance.data.isowindEventsHad = nil
    logicHandler.LoadCell(CELL)
    local cell = LoadedCells[CELL]
    for _, key in ipairs({ "isowindEvents", "isowindEventLocals", "isowindEventDeaths", "isowindStates" }) do
        cell.data[key] = nil
    end

    -- Alpha and Beta stand near Tarhiel's journal, Gamma is far off; Delta is in a house in Seyda Neen
    makePlayer(0, "Alpha", { cell = CELL, exterior = true, x = -16750, y = -58560 })
    makePlayer(1, "Beta", { cell = CELL, exterior = true, x = -15000, y = -56500 })
    makePlayer(2, "Gamma", { cell = "-1, -8", exterior = true, x = -4000, y = -60000 })
    makePlayer(3, "Delta", { cell = "Seyda Neen, Arrille's Tradehouse", exterior = false, x = -16700, y = -58500 })
    cell.visitors = { 0, 1 }

    check("the event's script reports its variables",
        tableHelper.containsValue(config.synchronizedClientScriptIds, "fallingScript"))

    -- Alpha's game brings Tarhiel in: Alpha's run, with Beta who is near
    sent = {}
    local status = enableTarhiel(0)
    check("event enable skips the shared save", status.validDefaultHandler == false)
    check("Tarhiel is an event person now", cell.data.isowindEvents[TARHIEL] ~= nil)
    local states = cell.data.isowindStates[TARHIEL].players
    check("enabled for Alpha and Beta", states.alpha == true and states.beta == true)
    check("not for Gamma (far) or Delta (indoors)", states.gamma == nil and states.delta == nil)
    check("the event counts as had for Alpha and Beta", WorldInstance.data.isowindEventsHad.fallingscript.alpha and
        WorldInstance.data.isowindEventsHad.fallingscript.beta)
    check("but not for Gamma or Delta", not WorldInstance.data.isowindEventsHad.fallingscript.gamma and
        not WorldInstance.data.isowindEventsHad.fallingscript.delta)
    local localAlpha = sentTo("SendConsoleCommand", 0)[1]
    local localBeta = sentTo("SendConsoleCommand", 1)[1]
    check("Alpha's and Beta's games run Tarhiel themselves", localAlpha ~= nil and localAlpha.command ==
        "isowind local" and hasObject(localAlpha, "474443") and localBeta ~= nil and localBeta.command ==
        "isowind local")
    check("they're told that before he's enabled", firstIndexOf("SendConsoleCommand", 0) <
        firstIndexOf("SendObjectState", 0) and firstIndexOf("SendConsoleCommand", 1) < firstIndexOf("SendObjectState", 1))
    check("Alpha's game gets the enable back", sentTo("SendObjectState", 0)[1] ~= nil and
        hasObject(sentTo("SendObjectState", 0)[1], "474443", true))
    check("Beta's game gets it too", sentTo("SendObjectState", 1)[1] ~= nil and
        hasObject(sentTo("SendObjectState", 1)[1], "474443", true))
    check("Gamma's and Delta's don't", #sentTo("SendObjectState", 2) == 0 and #sentTo("SendObjectState", 3) == 0)

    -- How far Alpha's script has got: kept for both, Beta's game told, the timer dropped
    sent = {}
    status = setFellonce(0, 1, 1.5)
    check("event variables skip the shared save", status.validDefaultHandler == false)
    local record = cell.data.isowindEventLocals[BOOK]
    check("fellonce 1 kept for Alpha and Beta", record.players.alpha["0:0"] == 1 and record.players.beta["0:0"] == 1)
    check("the timer isn't kept", record.players.alpha["2:0"] == nil)
    check("not shared", cell.data.objectData[BOOK] == nil or cell.data.objectData[BOOK].variables == nil)
    local toBeta = sentTo("SendClientScriptLocal", 1)[1]
    check("Beta's game told fellonce 1", toBeta ~= nil and localValue(toBeta, "476746", 0) == 1)
    check("Alpha's game not told (it set it)", #sentTo("SendClientScriptLocal", 0) == 0)

    sent = {}
    receive(LOCAL, "fallingScript")
    status = validate("OnClientScriptLocal", { 0, CELL, { [BOOK] = { refId = "bk_falljournal_unique",
        uniqueIndex = BOOK, variables = { [FLOAT] = { [0] = 2.5 } } } }, {} })
    check("a timer alone is dropped", status.validDefaultHandler == false and #sent == 0)

    -- Beta's own game runs the script too: Beta's alone
    sent = {}
    setFellonce(1, 2)
    check("Beta's own progress is Beta's", record.players.beta["0:0"] == 2 and record.players.alpha["0:0"] == 1)
    check("and goes to no one", #sent == 0)
    receive(LOCAL, "fallingScript")
    validate("OnObjectState", { 1, CELL, { [PLATFORM] = { refId = "fallguy_collision_unique", uniqueIndex = PLATFORM,
        state = false } }, {} })
    check("Beta's platform disable is Beta's", cell.data.isowindStates[PLATFORM].players.beta == false and
        cell.data.isowindStates[PLATFORM].players.alpha == nil)
    check("and comes back to Beta only", #sentTo("SendObjectState", 1) == 1 and #sentTo("SendObjectState", 0) == 0)

    -- Alpha's script carries on: both
    sent = {}
    receive(LOCAL, "fallingScript")
    validate("OnObjectState", { 0, CELL, { [PLATFORM] = { refId = "fallguy_collision_unique", uniqueIndex = PLATFORM,
        state = false } }, {} })
    check("Alpha's platform disable is both's", cell.data.isowindStates[PLATFORM].players.alpha == false and
        cell.data.isowindStates[PLATFORM].players.beta == false)
    check("and both games get it", #sentTo("SendObjectState", 0) == 1 and #sentTo("SendObjectState", 1) == 1)

    -- Delta comes out of the house while it goes on: Delta joins Alpha's run, with what it has done so far
    places[3] = { cell = CELL, exterior = true, x = -17500, y = -59000 }
    cell.visitors = { 0, 1, 3 }
    sent = {}
    setFellonce(0, -1)
    check("Delta has the event now", WorldInstance.data.isowindEventsHad.fallingscript.delta == true)
    check("Delta has Tarhiel enabled and the platform gone", cell.data.isowindStates[TARHIEL].players.delta == true
        and cell.data.isowindStates[PLATFORM].players.delta == false)
    local soFar = sentTo("SendObjectState", 3)[1]
    check("Delta's game gets both", soFar ~= nil and hasObject(soFar, "474443", true) and
        hasObject(soFar, "476745", false))
    local deltaLocals = sentTo("SendClientScriptLocal", 3)
    check("Delta's game gets fellonce 1 so far, then -1", #deltaLocals == 2 and
        localValue(deltaLocals[1], "476746", 0) == 1 and localValue(deltaLocals[2], "476746", 0) == -1)
    check("fellonce -1 for all three", record.players.alpha["0:0"] == -1 and record.players.beta["0:0"] == -1 and
        record.players.delta["0:0"] == -1)
    check("Alpha's and Beta's games hear only the -1 (Beta's)", #sentTo("SendClientScriptLocal", 0) == 0 and
        #sentTo("SendClientScriptLocal", 1) == 1)

    -- Gamma comes by later, from far off: Gamma's own run
    places[2] = { cell = CELL, exterior = true, x = -16740, y = -58550 }
    places[0] = { cell = "-1, -8", exterior = true, x = -4000, y = -60000 }
    places[1] = { cell = "-1, -8", exterior = true, x = -4100, y = -60000 }
    sent = {}
    enableTarhiel(2)
    check("Gamma has the event", WorldInstance.data.isowindEventsHad.fallingscript.gamma == true and
        cell.data.isowindStates[TARHIEL].players.gamma == true)
    check("Gamma's run gives the others nothing", #sentTo("SendObjectState", 0) == 0 and
        #sentTo("SendObjectState", 1) == 0 and #sentTo("SendObjectState", 2) == 1)

    -- Tarhiel dies in Alpha's game, which says so (as gameplay, with where the body lies)
    local function reportDeath(pid, origin, uniqueIndex, refId, x, y, z)
        receive(origin)
        return validate("OnClientScriptLocal", { pid, CELL, { [uniqueIndex] = { refId = refId,
            uniqueIndex = uniqueIndex, variables = { [FLOAT] = { [0] = x, [1] = y, [2] = z } } } }, {} })
    end
    sent = {}
    status = reportDeath(0, enumerations.packetOrigin.CLIENT_GAMEPLAY, TARHIEL, "agronian guy", -16747.4, -58568, 668.5)
    check("the death report skips the default handlers", status.validDefaultHandler == false and
        status.validCustomHandlers == false)
    local death = cell.data.isowindEventDeaths ~= nil and cell.data.isowindEventDeaths[TARHIEL] ~= nil and
        cell.data.isowindEventDeaths[TARHIEL].alpha or nil
    check("Alpha's Tarhiel is dead where he fell", death ~= nil and death.posZ == 668.5)
    check("nothing's shared", cell.data.objectData[TARHIEL] == nil or cell.data.objectData[TARHIEL].variables == nil)
    reportDeath(1, enumerations.packetOrigin.CLIENT_CONSOLE, TARHIEL, "agronian guy", 0, 0, 0)
    check("one from the console isn't a death", cell.data.isowindEventDeaths[TARHIEL].beta == nil)
    reportDeath(2, enumerations.packetOrigin.CLIENT_GAMEPLAY, TARHIEL, "agronian guy", 0, 0, 0)
    check("one from a game that wasn't told to run him is ignored", cell.data.isowindEventDeaths[TARHIEL].gamma == nil)
    reportDeath(0, enumerations.packetOrigin.CLIENT_GAMEPLAY, BOOK, "bk_falljournal_unique", 0, 0, 0)
    check("one about something else is ignored", cell.data.isowindEventDeaths[BOOK] == nil and
        (cell.data.objectData[BOOK] == nil or cell.data.objectData[BOOK].variables == nil))

    -- Someone the authority's game says died isn't brought back
    customEventHooks.triggerHandlers("OnActorDeath", customEventHooks.makeEventStatus(true, true),
        { 0, CELL, { [TARHIEL] = { refId = "agronian guy", killer = {} } } })
    check("an event person doesn't respawn", WorldInstance.data.isowindRespawns == nil or
        WorldInstance.data.isowindRespawns[CELL] == nil or WorldInstance.data.isowindRespawns[CELL][TARHIEL] == nil)

    -- Alpha logs in again later and loads the cell: dead Tarhiel, the platform gone, fellonce -1
    local realTime = os.time
    os.time = function() return realTime() + 200 end
    places[0] = { cell = CELL, exterior = true, x = -16750, y = -58560 }
    cell.visitors = { 0 }
    sent = {}
    customEventHooks.triggerHandlers("OnCellLoad", customEventHooks.makeEventStatus(true, true), { 0, CELL })
    local commands = sentTo("SendConsoleCommand", 0)
    local dead, alive = nil, nil
    for _, packet in ipairs(commands) do
        if packet.command:find("^isowind local dead") then dead = packet end
        if packet.command == "isowind local" then alive = packet end
    end
    check("cell load: Alpha's game runs its dead Tarhiel", dead ~= nil and
        dead.command == "isowind local dead -16747.40 -58568.00 668.50" and hasObject(dead, "474443"))
    check("cell load: the platform isn't a person but is told too (the game ignores it)", alive ~= nil and
        hasObject(alive, "476745"))
    local stateReplay = sentTo("SendObjectState", 0)[1]
    check("cell load: Tarhiel enabled and the platform gone", stateReplay ~= nil and
        hasObject(stateReplay, "474443", true) and hasObject(stateReplay, "476745", false))
    local localsReplay = sentTo("SendClientScriptLocal", 0)[1]
    check("cell load: fellonce -1", localsReplay ~= nil and localValue(localsReplay, "476746", 0) == -1)
    check("cell load: the event marks come before the states", firstIndexOf("SendConsoleCommand", 0) <
        firstIndexOf("SendObjectState", 0))

    -- Alpha's game's script starts over before the replay arrives: answered with what Alpha has, no new run
    sent = {}
    status = setFellonce(0, 1)
    local answer = sentTo("SendClientScriptLocal", 0)[1]
    check("a re-fired variable is answered with Alpha's", status.validDefaultHandler == false and answer ~= nil and
        localValue(answer, "476746", 0) == -1)
    check("and isn't kept", record.players.alpha["0:0"] == -1)
    receive(LOCAL, "fallingScript")
    validate("OnObjectState", { 0, CELL, { [PLATFORM] = { refId = "fallguy_collision_unique", uniqueIndex = PLATFORM,
        state = true } }, {} })
    local stateAnswer = sentTo("SendObjectState", 0)[1]
    check("a re-fired enable is answered with Alpha's state", stateAnswer ~= nil and
        hasObject(stateAnswer, "476745", false) and cell.data.isowindStates[PLATFORM].players.alpha == false)
    check("no one else hears of it", #sent == 2)

    -- Someone who has never had it loads the cell: their game runs Tarhiel itself, as the game's files have him
    makePlayer(4, "Epsilon", { cell = CELL, exterior = true, x = -16750, y = -58560 })
    cell.visitors = { 0, 4 }
    sent = {}
    customEventHooks.triggerHandlers("OnCellLoad", customEventHooks.makeEventStatus(true, true), { 4, CELL })
    local firstTime = sentTo("SendConsoleCommand", 4)
    check("first load: one isowind local, Tarhiel and the platform", #firstTime == 1 and
        firstTime[1].command == "isowind local" and hasObject(firstTime[1], "474443") and
        hasObject(firstTime[1], "476745"))
    check("first load: nothing of the others' events", #sentTo("SendObjectState", 4) == 0 and
        #sentTo("SendClientScriptLocal", 4) == 0)
    Players[4] = nil

    -- Long after the cell loaded, a change of Alpha's script is Alpha's
    os.time = function() return realTime() + 300 end
    sent = {}
    setFellonce(0, 5)
    check("later progress is kept, for Alpha only", record.players.alpha["0:0"] == 5 and
        record.players.beta["0:0"] == -1 and #sent == 0)
    os.time = realTime

    -- Other scripts' variables are as before
    receive(LOCAL, "GG_OpenGate1")
    status = validate("OnClientScriptLocal", { 0, CELL, { ["1-0"] = { refId = "x", uniqueIndex = "1-0",
        variables = { [SHORT] = { [0] = 1 } } } }, {} })
    check("a mechanism's variables stay shared", status.validDefaultHandler ~= false)

    for _, key in ipairs({ "isowindEvents", "isowindEventLocals", "isowindEventDeaths", "isowindStates" }) do
        cell.data[key] = nil
    end
    log("done, " .. failures .. " failed")
end

customEventHooks.registerHandler("OnServerPostInit", function()
    local ok, err = pcall(runTests)
    if not ok then log("ERROR " .. tostring(err)) end
end)
