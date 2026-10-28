-- Isowind: characters from single-player (the launcher's Character list). With "portableCharacters = true" under
-- [Isowind] in the server's settings, a player who joins with one plays it here: the client says which character it
-- is (an id kept beside its saves) and when it was last played, before logging in, and after the password
--
--   a name new here      registers it, and the character comes from the save instead of character creation
--   the same character   the newer copy wins: a newer save replaces the one here, an older one is left be
--   another character    (a different id, or one made here) with the name: the player can't take it, and goes
--
-- What comes is what the server keeps of a character: race, class, birthsign, stats, attributes, skills, the whole
-- inventory and equipment, quick keys, spells, skill books read and bounty, and where they were; and what this server
-- keeps for each player rather than for everyone (in MMO the journal, topics, factions, reputation, kills and the
-- quests' global variables; in co-op those are the world's). Potions they brewed, items they enchanted and spells they
-- made come with their records, which are made here again under this server's ids (as if made here; ones made for
-- them before are reused) and linked to the player.
--
-- Logged in as their character (whichever copy won), the client is told so, and writes what the character gains
-- here (quests too) back to single-player. So is a character made here: it's given an id the first time, and the
-- client keeps it among the player's single-player characters, to play offline too.
--
-- The client's half is apps/openmw/mwmp/PortableCharacter.cpp; the id lives in the player's file as
-- customVariables.isowindPortable.

local json = require("dkjson")

-- pid: { id, played, chunks, action ("register" or "replace") } for players joining with a character
local joining = {}

local function message(pid, text)
    tes3mp.SendMessage(pid, color.Yellow .. text .. color.Default .. "\n", false)
end

-- Tells the player why in a box, then lets them go (time to read it)
function IsowindPortableKick(pid)
    if Players[pid] ~= nil and not Players[pid]:IsLoggedIn() then
        Players[pid]:Kick()
    end
end

local function refuse(pid, text)
    joining[pid] = nil
    tes3mp.MessageBox(pid, -1, text)
    -- (and in the chat: the game shows the last thing said there when it's let go)
    message(pid, text)
    tes3mp.StartTimer(tes3mp.CreateTimerEx("IsowindPortableKick", time.seconds(8), "i", pid))
end

-- The records the player's game made (by store type, under that game's ids), made here as generated records linked to
-- the player: that game's ids to this server's
local function bringRecords(player, records)
    local ids = {}
    if records == nil then
        return ids
    end
    -- Enchantments first: the enchanted items need their ids
    for _, storeType in ipairs({ "enchantment", "potion", "spell", "armor", "book", "clothing", "weapon" }) do
        local recordStore = RecordStores[storeType]
        local made = {}
        for oldId, record in pairs(records[storeType] or {}) do
            if record.enchantmentId ~= nil and ids[record.enchantmentId] ~= nil then
                record.enchantmentId = ids[record.enchantmentId]
            end
            -- One the same, made for the player before (coming from single-player once more), is that one
            local id = recordStore:GetMatchingRecordId(record, recordStore.data.generatedRecords,
                player.data.recordLinks ~= nil and player.data.recordLinks[storeType] or nil, { "quantity" })
            if id == nil then
                id = recordStore:GenerateRecordId()
                made[id] = record
            end
            ids[oldId] = id
        end
        if next(made) ~= nil then
            recordStore:SaveGeneratedRecords(made)
            for id, record in pairs(made) do
                player:AddLinkToRecord(storeType, id)
                if storeType ~= "enchantment" and record.enchantmentId ~= nil and
                    RecordStores["enchantment"].data.generatedRecords[record.enchantmentId] ~= nil then
                    RecordStores["enchantment"]:AddLinkToRecord(record.enchantmentId, id, storeType)
                end
            end
            recordStore:QuicksaveToDrive()
        end
    end
    RecordStores["enchantment"]:QuicksaveToDrive()
    return ids
end

-- Whether the server keeps a global variable for everyone rather than for each player (as eventHandler's
-- OnClientScriptGlobal decides it), or not at all
local function isWorldGlobal(id)
    local scopes = clientVariableScopes.globals or {}
    local function listed(list)
        return list ~= nil and tableHelper.containsCaseInsensitiveString(list, id)
    end
    return listed(scopes.ignored) or listed(scopes.kills) or (config.shareJournal and listed(scopes.quest)) or
        (config.shareFactionRanks and listed(scopes.factionRanks)) or
        (config.shareFactionExpulsion and listed(scopes.factionExpulsion)) or listed(scopes.worldwide)
end

-- The character as the server keeps it, from what the client sent (PortableCharacter.cpp's toJson)
local function apply(player, character)
    local data = player.data
    local ids = bringRecords(player, character.records)
    local function idHere(id)
        return ids[id] or id
    end

    data.character.race = character.race
    data.character.head = character.head
    data.character.hair = character.hair
    data.character.gender = character.male and 1 or 0
    data.character.birthsign = character.birthsign
    data.character.modelOverride = ""
    if character.customClass ~= nil then
        local custom = character.customClass
        local function names(numbers, nameOf)
            local list = {}
            for _, number in ipairs(numbers) do
                table.insert(list, nameOf(number))
            end
            return table.concat(list, ", ")
        end
        data.character.class = "custom"
        data.customClass = {
            name = custom.name,
            description = custom.description,
            specialization = custom.specialization,
            majorAttributes = names(custom.majorAttributes, tes3mp.GetAttributeName),
            majorSkills = names(custom.majorSkills, tes3mp.GetSkillName),
            minorSkills = names(custom.minorSkills, tes3mp.GetSkillName)
        }
    else
        data.character.class = character.class
        data.customClass = {}
    end

    data.stats = {
        level = character.level,
        levelProgress = character.levelProgress,
        healthBase = character.health[1], healthCurrent = character.health[2],
        magickaBase = character.magicka[1], magickaCurrent = character.magicka[2],
        fatigueBase = character.fatigue[1], fatigueCurrent = character.fatigue[2]
    }
    for index, attribute in ipairs(character.attributes) do
        data.attributes[tes3mp.GetAttributeName(index - 1)] = {
            base = attribute[1], damage = attribute[2], skillIncrease = attribute[3]
        }
    end
    for index, skill in ipairs(character.skills) do
        data.skills[tes3mp.GetSkillName(index - 1)] = { base = skill[1], damage = skill[2], progress = skill[3] }
    end

    data.inventory = {}
    for _, item in ipairs(character.inventory) do
        table.insert(data.inventory, {
            refId = idHere(item[1]), count = item[2], charge = item[3], enchantmentCharge = item[4], soul = item[5]
        })
    end
    data.equipment = {}
    for slot, item in pairs(character.equipment) do
        data.equipment[tonumber(slot)] = {
            refId = idHere(item[1]), count = item[2], charge = item[3], enchantmentCharge = item[4]
        }
    end
    data.spellbook = {}
    for _, spell in ipairs(character.spells) do
        table.insert(data.spellbook, idHere(spell))
    end
    data.books = character.books
    -- The quick keys, on the items and spells as they're called here; what was cooling down or in effect belongs to
    -- the old copy
    data.quickKeys = {}
    for _, key in ipairs(character.quickKeys or {}) do
        data.quickKeys[key[1]] = { keyType = key[2], itemId = idHere(key[3]) }
    end
    data.cooldowns = {}
    data.spellsActive = {}

    if data.fame == nil then data.fame = {} end
    if not config.shareBounty then data.fame.bounty = character.bounty end

    data.location = {
        cell = character.cell, regionName = "",
        posX = character.position[1], posY = character.position[2], posZ = character.position[3],
        rotX = character.rotation[1], rotZ = character.rotation[2]
    }

    if not config.shareReputation then data.fame.reputation = character.reputation end

    if not config.shareFactionRanks then
        data.factionRanks = {}
        for faction, stats in pairs(character.factions) do
            if stats[1] >= 0 then data.factionRanks[faction] = stats[1] end
        end
    end
    if not config.shareFactionExpulsion then
        data.factionExpulsion = {}
        for faction, stats in pairs(character.factions) do
            if stats[2] then data.factionExpulsion[faction] = true end
        end
    end
    if not config.shareFactionReputation then
        data.factionReputation = {}
        for faction, stats in pairs(character.factions) do
            if stats[3] ~= 0 then data.factionReputation[faction] = stats[3] end
        end
    end
    if not config.shareJournal then
        data.journal = {}
        for _, entry in ipairs(character.journal) do
            table.insert(data.journal, {
                type = enumerations.journal.ENTRY, quest = entry[1], index = entry[2], actorRefId = "player",
                timestamp = { daysPassed = entry[3], month = entry[4], day = entry[5] }
            })
        end
        -- Then each quest where it is now
        for _, quest in ipairs(character.quests) do
            table.insert(data.journal, { type = enumerations.journal.INDEX, quest = quest[1], index = quest[2] })
        end
    end
    if not config.shareKills then
        data.kills = character.kills or {}
    end
    -- The global variables this server keeps for each player (quests' own, here), not those it keeps for everyone
    if data.clientVariables == nil then data.clientVariables = {} end
    if data.clientVariables.globals == nil then data.clientVariables.globals = {} end
    for id, global in pairs(character.globals or {}) do
        if not isWorldGlobal(id) then
            if global[1] == enumerations.variableType.FLOAT then
                data.clientVariables.globals[id] = { variableType = global[1], floatValue = global[2] }
            else
                data.clientVariables.globals[id] = { variableType = global[1], intValue = global[2] }
            end
        end
    end
    if not config.shareTopics then
        data.topics = character.topics
    end
end

-- The whole character has come: it's applied, and the player logs in with it as with one kept here
local function arrived(pid, text)
    local player = Players[pid]
    local entry = joining[pid]
    joining[pid] = nil

    local character = json.decode(text)
    if type(character) ~= "table" or character.error ~= nil or character.attributes == nil then
        tes3mp.LogMessage(enumerations.log.ERROR, "[Isowind] " .. logicHandler.GetChatName(pid) ..
            "'s single-player character couldn't be read: " .. tostring(character and character.error))
        refuse(pid, "Your single-player character couldn't be read from its save. Join without it (pick " ..
            "\"New character\" in Isowind) or try another save.")
        return
    end
    if character.unfinished then
        refuse(pid, "This character hasn't finished character creation in single-player yet. Finish it there " ..
            "first, or join with \"New character\".")
        return
    end

    apply(player, character)
    if player.data.customVariables == nil then player.data.customVariables = {} end
    player.data.customVariables.isowindPortable = { id = entry.id, played = entry.played }

    if entry.action == "register" then
        player:SaveLogin()
        player:CreateAccount()
        if not player:HasAccount() then
            return
        end
    else
        player:SaveToDrive()
    end
    tes3mp.LogMessage(enumerations.log.INFO, "[Isowind] " .. logicHandler.GetChatName(pid) ..
        " brought their single-player character (" .. entry.action .. ")")

    player:FinishLogin()
    if entry.action == "register" then
        player:Message("You have successfully registered.\n" .. config.chatWindowInstructions)
    else
        player:Message("You have successfully logged in.\n" .. config.chatWindowInstructions)
    end
    message(pid, "Your character came from single-player" ..
        (entry.action == "replace" and " (newer there than here)" or "") .. ".")
    if (character.skippedItems or 0) > 0 or (character.skippedSpells or 0) > 0 then
        message(pid, "Some things couldn't come (their records weren't in your save): " ..
            (character.skippedItems or 0) .. " items, " .. (character.skippedSpells or 0) .. " spells.")
    end
end

-- Before logging in: which character it is, then (once asked for) the character in chunks
local function onPortableCharacter(eventStatus, pid, changeType)
    if Players[pid] == nil or Players[pid]:IsLoggedIn() then
        return
    end
    if changeType == enumerations.miscellaneous.PORTABLE_CHARACTER then
        if not IsowindHost.portableCharacters then
            message(pid, "This server doesn't take characters from single-player: " ..
                (Players[pid]:HasAccount() and "log in to the character of that name here." or
                "you'll make a new one here."))
            return
        end
        joining[pid] = {
            id = tes3mp.GetPortableCharacterId(pid),
            played = tes3mp.GetPortableCharacterPlayed(pid),
            chunks = {}
        }
        return
    end

    local entry = joining[pid]
    if entry == nil or entry.action == nil then
        return
    end
    local count = tes3mp.GetPortableDataChunkCount(pid)
    entry.chunks[tes3mp.GetPortableDataChunk(pid) + 1] = tes3mp.GetPortableData(pid)
    for index = 1, count do
        if entry.chunks[index] == nil then
            return
        end
    end
    arrived(pid, table.concat(entry.chunks, "", 1, count))
end

-- The password typed: with a character from single-player, it's sent for instead of making one or loading this one
local function onGUIAction(eventStatus, pid, idGui, data)
    local entry = joining[pid]
    local player = Players[pid]
    if entry == nil or player == nil or player:IsLoggedIn() or data == nil then
        return
    end

    if idGui == guiHelper.ID.REGISTER and not player:HasAccount() then
        player.isNewlyRegistered = true
        player:GenerateSaltedHash(data)
        player.data.settings.consoleAllowed = "default"
        entry.action = "register"
        tes3mp.SendPortableRequest(pid)
        return customEventHooks.makeEventStatus(false, false)
    end

    if idGui == guiHelper.ID.LOGIN and player:HasAccount() then
        -- A wrong password or a banned name: as usual
        player:LoadFromDrive()
        if player.data.login.passwordHash ~= tes3mp.GetSHA256Hash(data .. player.data.login.passwordSalt) or
            tableHelper.containsValue(banList.playerNames, string.lower(player.accountName)) then
            return
        end

        local kept = player.data.customVariables ~= nil and player.data.customVariables.isowindPortable or nil
        if kept == nil or kept.id ~= entry.id then
            refuse(pid, "Someone else's character, or one made on this server, is called " .. player.accountName ..
                " here, so yours can't come in under that name. Rename yours in single-player (a new " ..
                "character), or join with \"New character\".")
            return customEventHooks.makeEventStatus(false, false)
        end
        if entry.played > (player.data.timestamps.lastDisconnect or 0) then
            entry.action = "replace"
            tes3mp.SendPortableRequest(pid)
            return customEventHooks.makeEventStatus(false, false)
        end
        -- Played here since: this copy is the newer one (and what it gains goes back to single-player too)
        joining[pid] = nil
        message(pid, "Your character here is newer than your single-player save, so you play this one.")
    end
end

-- Logged in (or just made here): the character goes to single-player, under its id (one is made for a character made
-- here the first time)
local function link(eventStatus, pid)
    local player = Players[pid]
    if not IsowindHost.portableCharacters or player == nil or not player:IsLoggedIn() then
        return
    end
    if player.data.customVariables == nil then player.data.customVariables = {} end
    local kept = player.data.customVariables.isowindPortable
    if kept == nil then
        kept = { id = string.lower(tes3mp.GenerateRandomString(32)), played = 0 }
        player.data.customVariables.isowindPortable = kept
        player:QuicksaveToDrive()
    end
    tes3mp.SendPortableLinked(pid, kept.id)
end

customEventHooks.registerHandler("OnServerPostInit", function(eventStatus)
    customEventHooks.registerHandler("OnPlayerPortableCharacter", onPortableCharacter)
    customEventHooks.registerValidator("OnGUIAction", onGUIAction)
    customEventHooks.registerHandler("OnPlayerAuthentified", link)
    customEventHooks.registerHandler("OnPlayerEndCharGen", link)
    customEventHooks.registerHandler("OnPlayerDisconnect", function(eventStatus, pid)
        joining[pid] = nil
    end)
end)
