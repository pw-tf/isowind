-- How to run: copy this into a copy of the server folder as CoreScripts/scripts/custom/, add
-- require("custom/partytest") at the end of customScripts.lua, put mode = mmo under [Isowind] in tes3mp-server.cfg and
-- start tes3mp-server for a few seconds; the log shows PASS/FAIL lines and "done, N failed".
-- Tests of isowindParty.lua inside a real server: two fake players, real world data.
local log = function(text) tes3mp.LogMessage(enumerations.log.INFO, "[partytest] " .. text) end
local failures = 0
local function check(name, condition)
    if condition then log("PASS " .. name) else log("FAIL " .. name); failures = failures + 1 end
end

local sent = {}
local current = {}
local function installMocks()
    for name, fn in pairs(tes3mp) do
        if type(name) == "string" and (name:find("^Set") or name:find("^Add") or name:find("^Send") or
            name:find("^Clear") or name == "CopyReceivedObjectListToStore") then
            tes3mp[name] = function(...)
                local args = { ... }
                if name == "ClearObjectList" then current = { objects = {} }
                elseif name == "SetObjectRefNum" then current.refNum = args[1]
                elseif name == "SetObjectState" then current.state = args[1]
                elseif name == "AddObject" then table.insert(current.objects, { refNum = current.refNum, state = current.state })
                elseif name == "SetObjectListPid" then current.pid = args[1]
                elseif name:find("^Send") then
                    table.insert(sent, { name = name, args = args, pid = current.pid, objects = current.objects })
                end
            end
        end
    end
    current = { objects = {} }
end

local topicChanges = {}
local function sentTo(name, pid)
    for _, packet in ipairs(sent) do
        if packet.name == name and (packet.args[1] == pid or packet.pid == pid) then return packet end
    end
end
local function messagesTo(pid)
    local texts = {}
    for _, packet in ipairs(sent) do
        if packet.name == "SendMessage" and packet.args[1] == pid then table.insert(texts, packet.args[2]) end
    end
    return table.concat(texts, " | ")
end
local function hasJournal(player, quest, index)
    for _, item in ipairs(player.data.journal or {}) do
        if item.quest == quest and item.index == index then return true end
    end
    return false
end
local function makePlayer(pid, name, journal)
    local player = { pid = pid, accountName = name, name = name, consoleCommandsQueued = {}, summons = {},
        alliesLoaded = 0,
        data = { location = { cell = "-3, 6" }, alliedPlayers = {}, journal = journal, topics = {},
            clientVariables = { globals = {} }, customVariables = {} } }
    player.IsLoggedIn = function() return true end
    player.QuicksaveToDrive = function() end
    player.Save = function() end
    player.Message = function() end
    player.LoadAllies = function(self) self.alliesLoaded = self.alliesLoaded + 1 end
    Players[pid] = player
    return player
end
local function command(pid, text)
    local cmd = text:split(" ")
    customCommandHooks.getCallback(cmd[1])(pid, cmd)
end

local function runTests()
    installMocks()
    tes3mp.GetTopicChangesSize = function() return #topicChanges end
    tes3mp.GetTopicId = function(pid, i) return topicChanges[i + 1] end
    tes3mp.GetObjectListOrigin = function() return enumerations.packetOrigin.CLIENT_SCRIPT_LOCAL end
    WorldInstance.data.isowindParties = nil
    local ENTRY = enumerations.journal.ENTRY

    local alpha = makePlayer(0, "Alpha", { { type = ENTRY, quest = "a1_1_findspymaster", index = 1, actorRefId = "x" } })
    local beta = makePlayer(1, "Beta", { { type = ENTRY, quest = "mg_guildmanual", index = 10, actorRefId = "y" } })
    beta.data.clientVariables.globals.hortatorvotes = { variableType = enumerations.variableType.SHORT, intValue = 2 }

    -- Invite and accept
    command(0, "party invite Beta")
    check("Beta is told of the invitation", messagesTo(1):find("invites you") ~= nil)
    command(1, "party accept")
    local _, party = nil, nil
    for id, p in pairs(WorldInstance.data.isowindParties.list) do party = p end
    check("party has both", party ~= nil and #party.members == 2)
    check("they're allies", #alpha.data.alliedPlayers == 1 and #beta.data.alliedPlayers == 1 and
        alpha.alliesLoaded > 0 and beta.alliesLoaded > 0)
    check("Beta got Alpha's journal", hasJournal(beta, "a1_1_findspymaster", 1))
    check("Alpha got Beta's journal", hasJournal(alpha, "mg_guildmanual", 10))
    check("Alpha got Beta's quest variable", alpha.data.clientVariables.globals.hortatorvotes ~= nil and
        alpha.data.clientVariables.globals.hortatorvotes.intValue == 2)
    check("journals sent to both games", sentTo("SendJournalChanges", 0) ~= nil and sentTo("SendJournalChanges", 1) ~= nil)

    -- A new journal entry reaches the other member, once
    sent = {}
    local item = { type = ENTRY, quest = "a1_2_antabolis", index = 5, actorRefId = "z" }
    customEventHooks.triggerHandlers("OnPlayerJournal", customEventHooks.makeEventStatus(true, true),
        { 0, { journal = { item } } })
    check("new entry given to Beta", hasJournal(beta, "a1_2_antabolis", 5) and sentTo("SendJournalChanges", 1) ~= nil)
    check("not sent back to Alpha", sentTo("SendJournalChanges", 0) == nil)
    local count = 0
    customEventHooks.triggerHandlers("OnPlayerJournal", customEventHooks.makeEventStatus(true, true),
        { 0, { journal = { item } } })
    for _, j in ipairs(beta.data.journal) do if j.quest == "a1_2_antabolis" then count = count + 1 end end
    check("same entry not given twice", count == 1)

    -- Topics
    sent = {}
    topicChanges = { "latest rumors" }
    customEventHooks.triggerHandlers("OnPlayerTopic", customEventHooks.makeEventStatus(true, true), { 1 })
    check("topic given to Alpha", tableHelper.containsValue(alpha.data.topics, "latest rumors") and
        sentTo("SendTopicChanges", 0) ~= nil)

    -- Quest variables, but not others
    sent = {}
    customEventHooks.triggerHandlers("OnClientScriptGlobal", customEventHooks.makeEventStatus(true, true),
        { 0, { heartdestroyed = { variableType = enumerations.variableType.SHORT, intValue = 1 },
            npcvoicedistance = { variableType = enumerations.variableType.FLOAT, floatValue = 750 } } })
    check("quest variable given to Beta", beta.data.clientVariables.globals.heartdestroyed ~= nil)
    check("other variables not", beta.data.clientVariables.globals.npcvoicedistance == nil)

    -- A quest script disabling someone: the whole party
    logicHandler.LoadCell("-3, 6")
    local cell = LoadedCells["-3, 6"]
    sent = {}
    customEventHooks.triggerValidators("OnObjectState", { 0, "-3, 6", { ["7777-0"] = { refId = "quest_npc",
        uniqueIndex = "7777-0", state = false } }, {} })
    local record = cell.data.isowindStates["7777-0"]
    check("quest disable kept for both members", record.players.alpha == false and record.players.beta == false)
    local toBeta = sentTo("SendObjectState", 1)
    check("Beta's game told now", toBeta ~= nil and toBeta.objects[1] ~= nil and toBeta.objects[1].state == false)
    check("Alpha's game not told (it did it)", sentTo("SendObjectState", 0) == nil)

    -- Kill credit for the ally
    beta.data.kills = nil
    cell.data.objectData["555-0"] = { refId = "mudcrab", stats = { healthBase = 35 } }
    customEventHooks.triggerHandlers("OnActorDeath", customEventHooks.makeEventStatus(true, true),
        { 0, "-3, 6", { ["555-0"] = { refId = "mudcrab", killer = { pid = 0 } } } })
    check("party member gets the kill", beta.data.kills ~= nil and beta.data.kills.mudcrab == 1)

    -- Catching up at login
    sent = {}
    table.insert(party.journal, { type = ENTRY, quest = "tg_loyalty", index = 30, actorRefId = "w" })
    customEventHooks.triggerHandlers("OnPlayerFinishLogin", customEventHooks.makeEventStatus(true, true), { 1 })
    check("Beta catches up at login", hasJournal(beta, "tg_loyalty", 30))

    -- Leaving
    sent = {}
    command(1, "party leave")
    check("two-player party ends when one leaves", next(WorldInstance.data.isowindParties.list) == nil)
    check("no longer allies", #alpha.data.alliedPlayers == 0 and #beta.data.alliedPlayers == 0)
    check("Beta keeps the quests", hasJournal(beta, "a1_1_findspymaster", 1))

    -- Not in a party: Beta's game (running the cell) reports a rat Alpha killed; Alpha gets it, not Beta
    alpha.data.kills = {}
    beta.data.kills = {}
    cell.data.objectData["556-0"] = { refId = "rat_cave_fgrh", stats = { healthBase = 8 } }
    customEventHooks.triggerHandlers("OnActorDeath", customEventHooks.makeEventStatus(true, true),
        { 1, "-3, 6", { ["556-0"] = { refId = "rat_cave_fgrh", killer = { pid = 0 } } } })
    check("the killer gets the kill", alpha.data.kills.rat_cave_fgrh == 1)
    check("whoever's game reported it doesn't", beta.data.kills.rat_cave_fgrh == nil)

    -- A rat killed by a guard: no one's
    cell.data.objectData["557-0"] = { refId = "rat_cave_fgrh", stats = { healthBase = 8 } }
    customEventHooks.triggerHandlers("OnActorDeath", customEventHooks.makeEventStatus(true, true),
        { 1, "-3, 6", { ["557-0"] = { refId = "rat_cave_fgrh", killer = { refId = "imperial guard", name = "Guard" } } } })
    check("a kill by someone else counts for no player", alpha.data.kills.rat_cave_fgrh == 1 and
        beta.data.kills.rat_cave_fgrh == nil)

    -- Alone again: a quest script is just theirs
    customEventHooks.triggerValidators("OnObjectState", { 0, "-3, 6", { ["7778-0"] = { refId = "quest_npc2",
        uniqueIndex = "7778-0", state = false } }, {} })
    check("after leaving, quest changes are only one's own", cell.data.isowindStates["7778-0"].players.alpha == false
        and cell.data.isowindStates["7778-0"].players.beta == nil)

    log("done, " .. failures .. " failed")
end

customEventHooks.registerHandler("OnServerPostInit", function()
    local ok, err = pcall(runTests)
    if not ok then log("ERROR " .. tostring(err)) end
end)
