-- Isowind MMO parties: players in a party share their quests as in co-op - journal, topics, quest variables, kill
-- credit and what quest scripts do to the world (isowindMmo.lua asks GetMemberNames) - while loot stays each one's own.
-- Members are also made allies (TES3MP's alliances: their followers don't fight each other).
--
--   /party                       who is in your party
--   /party invite <name or id>   invite a player (the number before their name in the player list works too)
--   /party accept / decline      answer an invitation
--   /party leave                 leave your party
--   /p <message>                 talk to your party
--
-- Kept in the world's data (isowindParties): members, and the party's journal, topics and quest variables, so
-- members who were offline catch up when they log in.

if not IsowindHost.IsMmo() then
    return
end

IsowindParty = {}

local MAX_MEMBERS = 8
local INVITE_SECONDS = 120
local invites = {} -- invited player's name -> { from = inviter's name, time = os.time() }

local function lower(name)
    return string.lower(name)
end

local function nameOf(pid)
    return lower(Players[pid].accountName)
end

local function parties()
    WorldInstance.data.isowindParties = WorldInstance.data.isowindParties or { nextId = 1, list = {} }
    return WorldInstance.data.isowindParties
end

local function partyOf(name)
    for id, party in pairs(parties().list) do
        if tableHelper.containsValue(party.members, name) then
            return id, party
        end
    end
end

local function onlinePid(name)
    for pid, player in pairs(Players) do
        if player:IsLoggedIn() and lower(player.accountName) == name then
            return pid
        end
    end
end

local function message(pid, text)
    tes3mp.SendMessage(pid, color.Orange .. "[Party] " .. color.White .. text .. "\n", false)
end

local function tellParty(party, text, exceptName)
    for _, member in ipairs(party.members) do
        local pid = onlinePid(member)
        if pid ~= nil and member ~= exceptName then
            message(pid, text)
        end
    end
end

-- Everyone a player's quest progress goes to: their party, or just them
function IsowindParty.GetMemberNames(name)
    local _, party = partyOf(name)
    if party ~= nil then
        return party.members
    end
    return { name }
end

-- --- Sharing ----------------------------------------------------------------------------------------------------

local function journalKey(item)
    return tostring(item.type) .. "|" .. lower(item.quest) .. "|" .. tostring(item.index)
end

local function isQuestVariable(id)
    return tableHelper.containsCaseInsensitiveString(clientVariableScopes.globals.quest, id)
end

-- Journal entries a member doesn't have yet: saved for them, and sent if they're on
local function giveJournal(player, items)
    player.data.journal = player.data.journal or {}
    local have = {}
    for _, item in ipairs(player.data.journal) do
        have[journalKey(item)] = true
    end
    local missing = {}
    for _, item in ipairs(items) do
        if not have[journalKey(item)] then
            have[journalKey(item)] = true
            table.insert(missing, item)
        end
    end
    if #missing == 0 then
        return
    end
    stateHelper:SaveJournal(player, { journal = missing })
    if player:IsLoggedIn() then
        tes3mp.ClearJournalChanges(player.pid)
        for _, item in ipairs(missing) do
            if item.type == enumerations.journal.ENTRY then
                local timestamp = item.timestamp or WorldInstance.data.time
                tes3mp.AddJournalEntryWithTimestamp(player.pid, item.quest, item.index, item.actorRefId or "player",
                    timestamp.daysPassed, timestamp.month, timestamp.day)
            else
                tes3mp.AddJournalIndex(player.pid, item.quest, item.index)
            end
        end
        tes3mp.SendJournalChanges(player.pid)
    end
end

local function giveTopics(player, topics)
    player.data.topics = player.data.topics or {}
    local missing = {}
    for _, topic in ipairs(topics) do
        if not tableHelper.containsValue(player.data.topics, topic) then
            table.insert(player.data.topics, topic)
            table.insert(missing, topic)
        end
    end
    if #missing == 0 then
        return
    end
    player:QuicksaveToDrive()
    if player:IsLoggedIn() then
        tes3mp.ClearTopicChanges(player.pid)
        for _, topic in ipairs(missing) do
            tes3mp.AddTopic(player.pid, topic)
        end
        tes3mp.SendTopicChanges(player.pid)
    end
end

local function giveVariables(player, variables)
    if next(variables) == nil then
        return
    end
    stateHelper:SaveClientScriptGlobal(player, variables)
    if player:IsLoggedIn() then
        tes3mp.ClearClientGlobals()
        for id, variable in pairs(variables) do
            if variable.variableType == enumerations.variableType.FLOAT then
                tes3mp.AddClientGlobalFloat(id, variable.floatValue)
            else
                tes3mp.AddClientGlobalInteger(id, variable.intValue, variable.variableType)
            end
        end
        tes3mp.SendClientScriptGlobal(player.pid)
    end
end

local function addToPartyJournal(party, items)
    local have = {}
    for _, item in ipairs(party.journal) do
        have[journalKey(item)] = true
    end
    for _, item in ipairs(items) do
        if not have[journalKey(item)] then
            have[journalKey(item)] = true
            table.insert(party.journal, item)
        end
    end
end

-- The other members who are on
local function forOtherMembers(party, name, callback)
    for _, member in ipairs(party.members) do
        local pid = onlinePid(member)
        if member ~= name and pid ~= nil then
            callback(Players[pid])
        end
    end
end

local function onJournal(eventStatus, pid, playerPacket)
    local _, party = partyOf(nameOf(pid))
    if party == nil or playerPacket == nil or playerPacket.journal == nil then
        return
    end
    addToPartyJournal(party, playerPacket.journal)
    forOtherMembers(party, nameOf(pid), function(member) giveJournal(member, playerPacket.journal) end)
    WorldInstance:QuicksaveToDrive()
end

local function onTopic(eventStatus, pid)
    local _, party = partyOf(nameOf(pid))
    if party == nil then
        return
    end
    local topics = {}
    for i = 0, tes3mp.GetTopicChangesSize(pid) - 1 do
        local topic = tes3mp.GetTopicId(pid, i)
        table.insert(topics, topic)
        if not tableHelper.containsValue(party.topics, topic) then
            table.insert(party.topics, topic)
        end
    end
    forOtherMembers(party, nameOf(pid), function(member) giveTopics(member, topics) end)
    WorldInstance:QuicksaveToDrive()
end

local function onVariables(eventStatus, pid, variables)
    local _, party = partyOf(nameOf(pid))
    if party == nil or variables == nil then
        return
    end
    local shared = {}
    for id, variable in pairs(variables) do
        if isQuestVariable(id) then
            shared[id] = variable
            party.variables[id] = variable
        end
    end
    if next(shared) == nil then
        return
    end
    forOtherMembers(party, nameOf(pid), function(member) giveVariables(member, shared) end)
    WorldInstance:QuicksaveToDrive()
end

-- A member logging in catches up with what the party did meanwhile
local function onLogin(eventStatus, pid)
    if Players[pid] == nil or not Players[pid]:IsLoggedIn() then
        return
    end
    local _, party = partyOf(nameOf(pid))
    if party == nil then
        return
    end
    local player = Players[pid]
    giveJournal(player, party.journal)
    giveTopics(player, party.topics)
    giveVariables(player, party.variables)
    tellParty(party, Players[pid].accountName .. " is here.", nameOf(pid))
end

-- --- Allies -----------------------------------------------------------------------------------------------------

local function setAllied(player, otherName, allied)
    player.data.alliedPlayers = player.data.alliedPlayers or {}
    local index = nil
    for i, name in ipairs(player.data.alliedPlayers) do
        if lower(name) == otherName then
            index = i
        end
    end
    if allied and index == nil then
        table.insert(player.data.alliedPlayers, otherName)
    elseif not allied and index ~= nil then
        table.remove(player.data.alliedPlayers, index)
    end
    player:QuicksaveToDrive()
    if player:IsLoggedIn() then
        player:LoadAllies()
    end
end

-- --- Joining and leaving ----------------------------------------------------------------------------------------

-- The quest variables a player has that parties share
local function questVariablesOf(player)
    local variables = {}
    local globals = player.data.clientVariables and player.data.clientVariables.globals or {}
    for id, variable in pairs(globals) do
        if type(variable) == "table" and isQuestVariable(id) then
            variables[id] = variable
        end
    end
    return variables
end

local function join(pid, inviterName)
    local name = nameOf(pid)
    local player = Players[pid]
    local inviterPid = onlinePid(inviterName)
    local id, party = partyOf(inviterName)

    if party == nil then
        if inviterPid == nil then
            message(pid, "They've left; ask them to invite you again.")
            return
        end
        local inviter = Players[inviterPid]
        local data = parties()
        id = tostring(data.nextId)
        data.nextId = data.nextId + 1
        party = { members = { inviterName }, journal = {}, topics = {}, variables = {} }
        addToPartyJournal(party, inviter.data.journal or {})
        for _, topic in ipairs(inviter.data.topics or {}) do
            table.insert(party.topics, topic)
        end
        party.variables = questVariablesOf(inviter)
        data.list[id] = party
    end

    if #party.members >= MAX_MEMBERS then
        message(pid, "That party is full (" .. MAX_MEMBERS .. ").")
        return
    end

    -- Both sides get what the other has done
    local newJournal = player.data.journal or {}
    local newTopics = player.data.topics or {}
    local newVariables = questVariablesOf(player)
    for variableId, variable in pairs(newVariables) do
        if party.variables[variableId] ~= nil then
            newVariables[variableId] = nil
        else
            party.variables[variableId] = variable
        end
    end

    forOtherMembers(party, name, function(member)
        giveJournal(member, newJournal)
        giveTopics(member, newTopics)
        giveVariables(member, newVariables)
        setAllied(member, player.accountName, true)
        setAllied(player, member.accountName, true)
    end)
    addToPartyJournal(party, newJournal)
    for _, topic in ipairs(newTopics) do
        if not tableHelper.containsValue(party.topics, topic) then
            table.insert(party.topics, topic)
        end
    end
    table.insert(party.members, name)

    giveJournal(player, party.journal)
    giveTopics(player, party.topics)
    giveVariables(player, party.variables)

    WorldInstance:QuicksaveToDrive()
    tellParty(party, player.accountName .. " joined the party.", name)
    message(pid, "You joined the party. Its quests are now yours too. /p talks to it, /party leave leaves it.")
end

local function leave(pid)
    local name = nameOf(pid)
    local id, party = partyOf(name)
    if party == nil then
        message(pid, "You're not in a party.")
        return
    end
    tableHelper.removeValue(party.members, name)
    tableHelper.cleanNils(party.members)
    forOtherMembers(party, name, function(member)
        setAllied(member, name, false)
        setAllied(Players[pid], lower(member.accountName), false)
    end)
    tellParty(party, Players[pid].accountName .. " left the party.", name)
    if #party.members <= 1 then
        tellParty(party, "The party is over.", name)
        parties().list[id] = nil
    end
    WorldInstance:QuicksaveToDrive()
    message(pid, "You left the party. You keep the quests you have.")
end

-- --- Commands ---------------------------------------------------------------------------------------------------

local function findPlayer(text)
    local number = tonumber(text)
    if number ~= nil and Players[number] ~= nil and Players[number]:IsLoggedIn() then
        return number
    end
    return onlinePid(lower(text or ""))
end

local function partyCommand(pid, cmd)
    local action = cmd[2] and lower(cmd[2]) or ""
    local name = nameOf(pid)

    if action == "invite" then
        local targetText = tableHelper.concatenateFromIndex(cmd, 3, " ")
        local targetPid = findPlayer(targetText)
        if targetPid == nil then
            message(pid, "No one called " .. tostring(targetText) .. " is on. Use their name or their number.")
        elseif targetPid == pid then
            message(pid, "That's you.")
        elseif partyOf(nameOf(targetPid)) ~= nil then
            message(pid, Players[targetPid].accountName .. " is already in a party.")
        else
            local _, party = partyOf(name)
            if party ~= nil and #party.members >= MAX_MEMBERS then
                message(pid, "Your party is full (" .. MAX_MEMBERS .. ").")
                return
            end
            invites[nameOf(targetPid)] = { from = name, time = os.time() }
            message(pid, "You invited " .. Players[targetPid].accountName .. ".")
            message(targetPid, Players[pid].accountName .. " invites you to their party, to do quests together: " ..
                color.Yellow .. "/party accept" .. color.White .. " or " .. color.Yellow .. "/party decline")
        end

    elseif action == "accept" or action == "decline" then
        local invite = invites[name]
        invites[name] = nil
        if invite == nil or os.time() - invite.time > INVITE_SECONDS then
            message(pid, "You have no invitation waiting.")
        elseif action == "decline" then
            message(pid, "You declined.")
            local inviterPid = onlinePid(invite.from)
            if inviterPid ~= nil then
                message(inviterPid, Players[pid].accountName .. " declined.")
            end
        elseif partyOf(name) ~= nil then
            message(pid, "Leave your party first (/party leave).")
        else
            join(pid, invite.from)
        end

    elseif action == "leave" then
        leave(pid)

    else
        local _, party = partyOf(name)
        if party == nil then
            message(pid, "You're not in a party. " .. color.Yellow .. "/party invite <name>" .. color.White ..
                " invites someone; a party shares quests, and each of you keeps your own loot.")
        else
            local list = {}
            for _, member in ipairs(party.members) do
                table.insert(list, member .. (onlinePid(member) and "" or " (away)"))
            end
            message(pid, "Your party: " .. table.concat(list, ", ") .. ". " .. color.Yellow .. "/p <message>" ..
                color.White .. " talks to it, " .. color.Yellow .. "/party leave" .. color.White .. " leaves it.")
        end
    end
end

local function partyChat(pid, cmd)
    local _, party = partyOf(nameOf(pid))
    local text = tableHelper.concatenateFromIndex(cmd, 2, " ")
    if party == nil then
        message(pid, "You're not in a party.")
    elseif text ~= nil and text ~= "" then
        for _, member in ipairs(party.members) do
            local memberPid = onlinePid(member)
            if memberPid ~= nil then
                tes3mp.SendMessage(memberPid, color.Orange .. "[Party] " .. Players[pid].accountName .. ": " ..
                    color.White .. text .. "\n", false)
            end
        end
    end
end

customCommandHooks.registerCommand("party", partyCommand)
customCommandHooks.registerCommand("p", partyChat)

customEventHooks.registerHandler("OnPlayerJournal", onJournal)
customEventHooks.registerHandler("OnPlayerTopic", onTopic)
customEventHooks.registerHandler("OnClientScriptGlobal", onVariables)
customEventHooks.registerHandler("OnPlayerFinishLogin", onLogin)

return IsowindParty
