-- Isowind MMO mode: followers of one's own.
--
-- Someone from the world (an NPC or creature from the game's files, not a summon) told to follow or escort a player
-- becomes that player's own: their game runs it for them alone from then on (the server's "isowind own", a console
-- command packet the game handles itself, mwmp::MechanicsHelper::runPrivateFollowerCommand), and the follow order goes
-- no further, so the world's stays at its post for everyone else, who can each have their own of it the same way.
-- Quest scripts and dialogue find it as ever: it's the same one in the player's game.
--
-- What the player's game tells of it (a ClientScriptLocal packet as gameplay: floats 0 to 2 the place, short 0 the
-- state, mwmp::MechanicsHelper::updatePrivateFollowers) is kept with the player (isowindFollowers):
-- * Following: after logging in, it's at their side again ("isowind follow").
-- * Staying (it stopped following): it stays where it stopped, for them ("isowind stay <x> <y> <z> <cell>").
-- * Dead: for them only, and it doesn't come back; its body lies where it fell ("isowind stay dead ...").
-- The player can send it away: /companions lists theirs, /dismiss <number or all> sends one back to its post, the
-- world's again for them ("isowind dismiss"), and /follow <number> asks one to follow them again where it is (at its
-- post if sent back, or where it stayed).
--
-- While it follows, the other players see it with them: their games make a look-alike of it ("isowind copy <x> <y>
-- <z>", "isowind uncopy" once it no longer follows) that can't be talked to or hurt, moved by what the owner's game
-- sends about it as about a copy numbered 0-<n> (n = FOLLOWER_COPY_BASE + pid * FOLLOWER_COPIES_PER_PLAYER + which of
-- the player's followers it is this session; the server takes those from that player's game alone, whoever has
-- authority over the cell). The owner's game learns the number with "isowind own <n>" / "isowind follow <n>".
--
-- A party has one of each (isowindParty.lua keeps which member holds it), for every member to use: one who uses its
-- copy (their game asks: state 4 in the same kind of packet), or who tells the same NPC to follow them, takes it over
-- ("isowind take <n> <flags> <x> <y> <z> <cell>" to their game, "isowind release" to the one that had it). If the
-- member it follows logs out or leaves the party, it goes to another member who is on, at their side.
--
-- Before followers were each player's own, a follow order moved the world's own NPC for everyone (Din, taken to the
-- Gnisis temple, was no longer at his post for anyone). Actors from the game's files that were moved out of their cell
-- before this script was there are put back at their post when either cell loads: what the server kept of them goes,
-- from both cells. Actors moved since are left as they are (isowindMovedSince, kept from the first start with this
-- script). Players with journal entries from one of them (who took it there) keep their own of it where it was
-- (isowindDisplacedReset, looked at once for each player).

if not IsowindHost.IsMmo() then
    return
end

local isowindFollowers = {}
IsowindFollowers = isowindFollowers

local state = { FOLLOWING = 1, STAYING = 2, DEAD = 3, WANTED = 4 }

-- This session's players (by name) whose game has been told where their followers are
local placed = {}

-- The numbers of this session's copies, by player name: { pid, slots = { uniqueIndex = slot }, count }
local copyNumbers = {}

local FOLLOWER_COPY_BASE = 1073741824 -- 0x40000000, components/openmw-mp/Base/BaseActor.hpp
local FOLLOWER_COPIES_PER_PLAYER = 256

local function playerKey(pid)
    return string.lower(Players[pid].accountName)
end

-- A player's followers: { uniqueIndex = { refId, postCell, state, cell, posX, posY, posZ } }, postCell being where the
-- game's files have it (where their game finds it after logging in)
local function followersOf(pid)
    local data = Players[pid].data
    data.customVariables = data.customVariables or {}
    data.customVariables.isowindFollowers = data.customVariables.isowindFollowers or {}
    return data.customVariables.isowindFollowers
end

-- A command the game handles itself (mwmp::ObjectList::runConsoleCommands), to one player, about one actor
local function sendCommand(pid, cellDescription, command, uniqueIndex, refId)
    local parts = uniqueIndex:split("-")
    tes3mp.ClearObjectList()
    tes3mp.SetObjectListPid(pid)
    tes3mp.SetObjectListCell(cellDescription)
    tes3mp.SetObjectListConsoleCommand(command)
    tes3mp.SetObjectRefNum(tonumber(parts[1]))
    tes3mp.SetObjectMpNum(tonumber(parts[2]))
    tes3mp.SetObjectRefId(refId)
    tes3mp.AddObject()
    tes3mp.SendConsoleCommand(false, false)
end

-- The number of a player's follower's copy, for this session
local function copyMpNum(pid, uniqueIndex)
    local name = playerKey(pid)
    local numbers = copyNumbers[name]
    if numbers == nil or numbers.pid ~= pid then
        numbers = { pid = pid, slots = {}, count = 0 }
        copyNumbers[name] = numbers
    end
    if numbers.slots[uniqueIndex] == nil then
        if numbers.count >= FOLLOWER_COPIES_PER_PLAYER then
            return nil
        end
        numbers.slots[uniqueIndex] = numbers.count
        numbers.count = numbers.count + 1
    end
    return FOLLOWER_COPY_BASE + pid * FOLLOWER_COPIES_PER_PLAYER + numbers.slots[uniqueIndex]
end

local function withCopyNumber(command, pid, uniqueIndex)
    local mpNum = copyMpNum(pid, uniqueIndex)
    return mpNum ~= nil and (command .. " " .. mpNum) or command
end

-- The others' games are told of a player's follower's copy: where it is ("isowind copy"), or that it's gone
local function sendCopy(ownerPid, uniqueIndex, follower, toPids, gone)
    local mpNum = copyMpNum(ownerPid, uniqueIndex)
    if mpNum == nil or follower.cell == nil or follower.refId == nil or follower.refId == "" then
        return
    end
    local command = gone and "isowind uncopy" or string.format("isowind copy %.2f %.2f %.2f", follower.posX or 0,
        follower.posY or 0, follower.posZ or 0)
    for _, pid in ipairs(toPids) do
        sendCommand(pid, follower.cell, command, "0-" .. mpNum, follower.refId)
    end
end

local function otherPids(pid)
    local pids = {}
    for otherPid, player in pairs(Players) do
        if otherPid ~= pid and player:IsLoggedIn() then
            table.insert(pids, otherPid)
        end
    end
    return pids
end

local function holderOf(name, uniqueIndex)
    return IsowindParty ~= nil and IsowindParty.GetFollowerHolder(name, uniqueIndex) or nil
end

local function setHolder(name, uniqueIndex, holder)
    if IsowindParty ~= nil then
        IsowindParty.SetFollowerHolder(name, uniqueIndex, holder)
    end
end

-- A follower goes from one member of a party to another: out of the first one's game, into the other's, at their side
-- following them (place nil), or where it is (place: x, y, z, cell; talk: as they used its copy)
local function handOver(fromPid, uniqueIndex, toPid, place)
    local fromFollowers = followersOf(fromPid)
    local follower = fromFollowers[uniqueIndex]
    if follower == nil then
        return
    end
    if follower.state == state.FOLLOWING then
        sendCopy(fromPid, uniqueIndex, follower, otherPids(fromPid), true)
    end
    sendCommand(fromPid, follower.cell or follower.postCell, "isowind release", uniqueIndex, follower.refId)
    fromFollowers[uniqueIndex] = nil
    followersOf(toPid)[uniqueIndex] = follower

    local mpNum = copyMpNum(toPid, uniqueIndex) or 0
    if place == nil then
        follower.state = state.FOLLOWING
        sendCommand(toPid, follower.postCell, "isowind follow " .. mpNum, uniqueIndex, follower.refId)
    else
        local flags = (follower.state == state.FOLLOWING and 1 or 0) + (place.talk and 2 or 0)
        follower.cell = place.cell
        follower.posX, follower.posY, follower.posZ = place.x, place.y, place.z
        sendCommand(toPid, follower.postCell, string.format("isowind take %d %d %.2f %.2f %.2f %s", mpNum, flags,
            place.x, place.y, place.z, place.cell), uniqueIndex, follower.refId)
    end
    setHolder(playerKey(toPid), uniqueIndex, playerKey(toPid))
    Players[fromPid]:QuicksaveToDrive()
    Players[toPid]:QuicksaveToDrive()
    tes3mp.LogMessage(enumerations.log.INFO, "[Isowind] " .. follower.refId .. " " .. uniqueIndex .. " went from " ..
        playerKey(fromPid) .. " to " .. playerKey(toPid))
end

-- The followers following a player who goes (logs out, leaves the party) go to another member of their party who is
-- on; the rest stay theirs
local function handOverFollowing(pid)
    local others = IsowindParty ~= nil and IsowindParty.GetOnlineMemberPids(playerKey(pid)) or {}
    for uniqueIndex, follower in pairs(followersOf(pid)) do
        if follower.state == state.FOLLOWING and others[1] ~= nil and
            holderOf(playerKey(pid), uniqueIndex) == playerKey(pid) then
            handOver(pid, uniqueIndex, others[1], nil)
        end
    end
end

function isowindFollowers.OnLeavingParty(pid)
    handOverFollowing(pid)
end

-- A player used a party member's follower's copy: it comes to them, there, and they talk to it
local function onWanted(pid, cellDescription, uniqueIndex, x, y, z)
    local _, mpNum = uniqueIndex:match("^(%d+)-(%d+)$")
    mpNum = tonumber(mpNum)
    if mpNum == nil or mpNum < FOLLOWER_COPY_BASE or x == nil then
        return
    end
    local ownerPid = math.floor((mpNum - FOLLOWER_COPY_BASE) / FOLLOWER_COPIES_PER_PLAYER)
    local slot = (mpNum - FOLLOWER_COPY_BASE) % FOLLOWER_COPIES_PER_PLAYER
    local owner = Players[ownerPid]
    if owner == nil or not owner:IsLoggedIn() or ownerPid == pid then
        return
    end
    local numbers = copyNumbers[playerKey(ownerPid)]
    local followerIndex = nil
    if numbers ~= nil and numbers.pid == ownerPid then
        for index, number in pairs(numbers.slots) do
            if number == slot then
                followerIndex = index
            end
        end
    end
    local follower = followerIndex ~= nil and followersOf(ownerPid)[followerIndex] or nil
    if follower == nil then
        return
    end
    local members = IsowindParty ~= nil and IsowindParty.GetMemberNames(playerKey(ownerPid)) or {}
    if not tableHelper.containsValue(members, playerKey(pid)) then
        tes3mp.SendMessage(pid, color.White .. "That's " .. owner.accountName .. "'s companion.\n", false)
        return
    end
    handOver(ownerPid, followerIndex, pid, { x = x, y = y, z = z, cell = cellDescription, talk = true })
end

-- Where the game's files have an actor now in a cell: there, unless it moved there from another (the cell keeps where
-- from, cell/base.lua's SaveActorCellChanges)
local function postCellOf(cellDescription, uniqueIndex)
    local cell = LoadedCells[cellDescription]
    if cell ~= nil and tableHelper.containsValue(cell.data.packets.cellChangeFrom or {}, uniqueIndex) then
        local objectData = cell.data.objectData[uniqueIndex]
        if objectData ~= nil and objectData.cellChangeFrom ~= nil then
            return objectData.cellChangeFrom
        end
    end
    return cellDescription
end

-- A follow or escort order toward the player who gave it (their game sends the order: a dialogue result or script)
-- makes the actor theirs, and doesn't reach the game running its cell
local function onActorAI(eventStatus, pid, cellDescription)
    if eventStatus.validDefaultHandler == false then
        return
    end
    tes3mp.ReadReceivedActorList()
    local followers = followersOf(pid)
    local taken = false
    for index = 0, tes3mp.GetActorListSize() - 1 do
        local action = tes3mp.GetActorAIAction(index)
        local mpNum = tes3mp.GetActorMpNum(index)
        if (action == enumerations.ai.FOLLOW or action == enumerations.ai.ESCORT) and mpNum == 0 and
            tes3mp.GetActorAITargetPid(index) == pid then
            local uniqueIndex = tes3mp.GetActorRefNum(index) .. "-" .. mpNum
            local refId = tes3mp.GetActorRefId(index)
            if refId == "" and LoadedCells[cellDescription] ~= nil then
                local objectData = LoadedCells[cellDescription].data.objectData[uniqueIndex]
                refId = objectData ~= nil and objectData.refId or ""
            end
            -- One of each for a party: another member's comes to this one (the one they told to follow, in their game)
            local holder = holderOf(playerKey(pid), uniqueIndex)
            if holder ~= nil and holder ~= playerKey(pid) and followers[uniqueIndex] == nil then
                for holderPid, player in pairs(Players) do
                    if player:IsLoggedIn() and string.lower(player.accountName) == holder then
                        local held = followersOf(holderPid)[uniqueIndex]
                        if held ~= nil and held.state == state.FOLLOWING then
                            sendCopy(holderPid, uniqueIndex, held, otherPids(holderPid), true)
                        end
                        if held ~= nil then
                            sendCommand(holderPid, held.cell or held.postCell, "isowind release", uniqueIndex, refId)
                            followersOf(holderPid)[uniqueIndex] = nil
                            player:QuicksaveToDrive()
                        end
                    end
                end
                tes3mp.LogMessage(enumerations.log.INFO, "[Isowind] " .. refId .. " " .. uniqueIndex .. " went from " ..
                    holder .. " to " .. playerKey(pid) .. ", who told it to follow them")
            end
            setHolder(playerKey(pid), uniqueIndex, playerKey(pid))
            if followers[uniqueIndex] == nil then
                followers[uniqueIndex] = { refId = refId, postCell = postCellOf(cellDescription, uniqueIndex),
                    state = state.FOLLOWING, cell = cellDescription }
                tes3mp.LogMessage(enumerations.log.INFO, "[Isowind] " .. refId .. " " .. uniqueIndex .. " follows " ..
                    playerKey(pid) .. ", who has their own of it from now on (its post: " ..
                    followers[uniqueIndex].postCell .. ")")
            end
            sendCommand(pid, cellDescription, withCopyNumber("isowind own", pid, uniqueIndex), uniqueIndex, refId)
            taken = true
        end
    end
    if taken then
        Players[pid]:QuicksaveToDrive()
        return customEventHooks.makeEventStatus(false, false)
    end
end

-- What a player's game tells of its own followers (and only those: anything else in such a packet is the MMO's,
-- isowindMmo.lua's event deaths, which have no state)
local function onClientScriptLocal(eventStatus, pid, cellDescription, objects)
    if eventStatus.validDefaultHandler == false or
        tes3mp.GetObjectListOrigin() ~= enumerations.packetOrigin.CLIENT_GAMEPLAY then
        return
    end
    local followers = followersOf(pid)
    local told = false
    for uniqueIndex, object in pairs(objects) do
        local variables = object.variables or {}
        local shorts = variables[enumerations.variableType.SHORT] or {}
        local floats = variables[enumerations.variableType.FLOAT] or {}
        local newState = tonumber(shorts[0])
        if newState == state.WANTED then
            told = true
            onWanted(pid, cellDescription, uniqueIndex, tonumber(floats[0]), tonumber(floats[1]), tonumber(floats[2]))
        elseif newState ~= nil then
            told = true
            local follower = followers[uniqueIndex]
            local x, y, z = tonumber(floats[0]), tonumber(floats[1]), tonumber(floats[2])
            if follower == nil or x == nil or y == nil or z == nil then
                tes3mp.LogMessage(enumerations.log.WARN, "[Isowind] Ignored news of " .. tostring(object.refId) ..
                    " " .. uniqueIndex .. " from " .. playerKey(pid) .. ": not their follower")
            else
                local wasFollowing = follower.state == state.FOLLOWING
                -- (The follow order didn't say who it is; the game says it here)
                if (follower.refId == nil or follower.refId == "") and object.refId ~= nil and object.refId ~= "" then
                    follower.refId = object.refId
                end
                -- Dead stays dead (its body can still be moved)
                if follower.state ~= state.DEAD then
                    if newState ~= follower.state then
                        tes3mp.LogMessage(enumerations.log.INFO, "[Isowind] " .. playerKey(pid) .. "'s " ..
                            follower.refId .. " " .. uniqueIndex .. " is " ..
                            (tableHelper.getIndexByValue(state, newState) or tostring(newState)) .. " in " ..
                            cellDescription)
                    end
                    follower.state = newState
                    -- Dead for the party too: a member can have the world's own again
                    if newState == state.DEAD then
                        setHolder(playerKey(pid), uniqueIndex, nil)
                    end
                end
                follower.cell = cellDescription
                follower.posX, follower.posY, follower.posZ = x, y, z
                -- The others see it while it follows
                if follower.state == state.FOLLOWING then
                    sendCopy(pid, uniqueIndex, follower, otherPids(pid), false)
                elseif wasFollowing then
                    sendCopy(pid, uniqueIndex, follower, otherPids(pid), true)
                end
            end
        end
    end
    if told then
        Players[pid]:QuicksaveToDrive()
        return customEventHooks.makeEventStatus(false, false)
    end
end

-- --- The world's actors that follow orders took away before ----------------------------------------------------------

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

local function forgetInCell(cell, uniqueIndex)
    cell.data.objectData[uniqueIndex] = nil
    for _, list in pairs(cell.data.packets) do
        if type(list) == "table" and tableHelper.containsValue(list, uniqueIndex) then
            tableHelper.removeValue(list, uniqueIndex)
            tableHelper.cleanNils(list)
        end
    end
    cell:QuicksaveToDrive()
end

-- Back at its post: the cell it was moved to and its own cell forget it was moved
local function putBack(uniqueIndex, postCell, movedTo)
    local record = withCell(movedTo, function(cell)
        local objectData = cell.data.objectData[uniqueIndex] or {}
        local location = objectData.location or {}
        forgetInCell(cell, uniqueIndex)
        return { refId = objectData.refId, postCell = postCell, cell = movedTo, posX = location.posX,
            posY = location.posY, posZ = location.posZ, dead = objectData.deathState ~= nil, checked = {} }
    end)
    withCell(postCell, function(cell)
        if record.refId == nil and cell.data.objectData[uniqueIndex] ~= nil then
            record.refId = cell.data.objectData[uniqueIndex].refId
        end
        forgetInCell(cell, uniqueIndex)
    end)
    WorldInstance.data.isowindDisplacedReset = WorldInstance.data.isowindDisplacedReset or {}
    WorldInstance.data.isowindDisplacedReset[uniqueIndex] = record
    WorldInstance:QuicksaveToDrive()
    tes3mp.LogMessage(enumerations.log.INFO, "[Isowind] Put " .. tostring(record.refId) .. " " .. uniqueIndex ..
        " back in " .. postCell .. " (it had been taken to " .. movedTo .. ")")
end

-- Before a cell's state goes to the player's game: the world's actors moved to or from it before
local function putBackDisplaced(eventStatus, pid, cellDescription)
    local movedSince = WorldInstance.data.isowindMovedSince or {}
    local moves = withCell(cellDescription, function(cell)
        local found = {}
        for _, uniqueIndex in pairs(cell.data.packets.cellChangeFrom or {}) do
            local objectData = cell.data.objectData[uniqueIndex]
            if uniqueIndex:match("^%d+%-0$") and not movedSince[uniqueIndex] and objectData ~= nil and
                objectData.cellChangeFrom ~= nil then
                table.insert(found, { uniqueIndex, objectData.cellChangeFrom, cellDescription })
            end
        end
        for _, uniqueIndex in pairs(cell.data.packets.cellChangeTo or {}) do
            local objectData = cell.data.objectData[uniqueIndex]
            if uniqueIndex:match("^%d+%-0$") and not movedSince[uniqueIndex] and objectData ~= nil and
                objectData.cellChangeTo ~= nil then
                table.insert(found, { uniqueIndex, cellDescription, objectData.cellChangeTo })
            end
        end
        return found
    end)
    for _, move in ipairs(moves) do
        putBack(move[1], move[2], move[3])
    end
    if #moves > 0 then
        -- The players on who took them there keep theirs there
        for otherPid, player in pairs(Players) do
            if player:IsLoggedIn() and placed[playerKey(otherPid)] then
                isowindFollowers.KeepDisplaced(otherPid, true)
            end
        end
    end
end

-- The world's actors moved since this script was there are left where they're moved to
local function onActorCellChange(eventStatus, pid, cellDescription)
    if eventStatus.validDefaultHandler == false then
        return
    end
    tes3mp.ReadReceivedActorList()
    local movedSince = WorldInstance.data.isowindMovedSince
    local added = false
    for index = 0, tes3mp.GetActorListSize() - 1 do
        local uniqueIndex = tes3mp.GetActorRefNum(index) .. "-" .. tes3mp.GetActorMpNum(index)
        if tes3mp.GetActorMpNum(index) == 0 and not movedSince[uniqueIndex] then
            movedSince[uniqueIndex] = true
            added = true
        end
    end
    if added then
        WorldInstance:QuicksaveToDrive()
    end
end

-- A player gets their own of each actor put back that has spoken their journal entries (looked at once for each
-- player, so the ones who talk to it later at its post don't), where it was; placed now if their game is already told
-- where their followers are
local placeFollower

function isowindFollowers.KeepDisplaced(pid, placeNow)
    local name = playerKey(pid)
    local followers = followersOf(pid)
    local spokenBy = {}
    for _, item in ipairs(Players[pid].data.journal or {}) do
        if item.actorRefId ~= nil then
            spokenBy[string.lower(item.actorRefId)] = true
        end
    end
    local changed = false
    for uniqueIndex, record in pairs(WorldInstance.data.isowindDisplacedReset or {}) do
        record.checked = record.checked or {}
        if not record.checked[name] then
            record.checked[name] = true
            changed = true
            if record.refId ~= nil and record.posX ~= nil and spokenBy[string.lower(record.refId)] and
                followers[uniqueIndex] == nil then
                followers[uniqueIndex] = { refId = record.refId, postCell = record.postCell,
                    state = record.dead and state.DEAD or state.STAYING, cell = record.cell, posX = record.posX,
                    posY = record.posY, posZ = record.posZ }
                tes3mp.LogMessage(enumerations.log.INFO, "[Isowind] " .. name .. " keeps their own " .. record.refId ..
                    " " .. uniqueIndex .. " in " .. record.cell)
                if placeNow then
                    placeFollower(pid, uniqueIndex, followers[uniqueIndex])
                end
            end
        end
    end
    if changed then
        WorldInstance:QuicksaveToDrive()
        Players[pid]:QuicksaveToDrive()
    end
end

-- A player's game is told where one of their followers is: at their side, where it stayed, or dead where it fell
function placeFollower(pid, uniqueIndex, follower)
    local command = withCopyNumber("isowind follow", pid, uniqueIndex)
    if follower.state ~= state.FOLLOWING and follower.posX ~= nil then
        command = string.format("isowind stay %s%.2f %.2f %.2f %s", follower.state == state.DEAD and "dead " or "",
            follower.posX, follower.posY, follower.posZ, follower.cell)
    end
    sendCommand(pid, follower.postCell, command, uniqueIndex, follower.refId)
end

-- A player's game loading a cell gets the copies of the others' followers there. Once it has loaded the world after
-- logging in, it's told where the player's own followers are: at their side, where they stayed, or dead where they fell.
local function onCellLoad(eventStatus, pid, cellDescription)
    if Players[pid] == nil or not Players[pid]:IsLoggedIn() then
        return
    end
    for otherPid, player in pairs(Players) do
        if otherPid ~= pid and player:IsLoggedIn() then
            for uniqueIndex, follower in pairs(followersOf(otherPid)) do
                if follower.state == state.FOLLOWING and follower.cell == cellDescription then
                    sendCopy(otherPid, uniqueIndex, follower, { pid }, false)
                end
            end
        end
    end

    local name = playerKey(pid)
    if placed[name] then
        return
    end
    placed[name] = true
    isowindFollowers.KeepDisplaced(pid, false)
    local followers = followersOf(pid)
    for uniqueIndex, follower in pairs(followers) do
        -- Another member of their party has it now
        local holder = holderOf(name, uniqueIndex)
        if holder ~= nil and holder ~= name then
            followers[uniqueIndex] = nil
            Players[pid]:QuicksaveToDrive()
        else
            placeFollower(pid, uniqueIndex, follower)
        end
    end
end

-- --- /companions and /dismiss --------------------------------------------------------------------------------------

-- The ones a player sent back to their post (/dismiss): { uniqueIndex = { refId, postCell } }, theirs to ask again
local function dismissedOf(pid)
    local data = Players[pid].data
    data.customVariables = data.customVariables or {}
    data.customVariables.isowindDismissed = data.customVariables.isowindDismissed or {}
    return data.customVariables.isowindDismissed
end

-- A player's companions, numbered: their followers, then the ones they sent back
local function companionList(pid)
    local followers, dismissed = {}, {}
    for uniqueIndex, follower in pairs(followersOf(pid)) do
        table.insert(followers, { uniqueIndex = uniqueIndex, follower = follower })
    end
    for uniqueIndex, record in pairs(dismissedOf(pid)) do
        table.insert(dismissed, { uniqueIndex = uniqueIndex, follower = record, dismissed = true })
    end
    local byIndex = function(a, b) return a.uniqueIndex < b.uniqueIndex end
    table.sort(followers, byIndex)
    table.sort(dismissed, byIndex)
    for _, entry in ipairs(dismissed) do
        table.insert(followers, entry)
    end
    return followers
end

local function nameOfCompanion(entry)
    local refId = entry.follower.refId
    return refId ~= nil and refId ~= "" and refId or entry.uniqueIndex
end

-- Whether a player is where a companion is: the same cell, or next to it outdoors
local function isNear(pid, cellDescription)
    local here = tes3mp.GetCell(pid)
    if here == cellDescription then
        return true
    end
    local x, y = here:match("^(%-?%d+), (%-?%d+)$")
    local otherX, otherY = cellDescription:match("^(%-?%d+), (%-?%d+)$")
    return x ~= nil and otherX ~= nil and math.abs(tonumber(x) - tonumber(otherX)) <= 1 and
        math.abs(tonumber(y) - tonumber(otherY)) <= 1
end

local stateNames = { [state.FOLLOWING] = "following you", [state.STAYING] = "staying", [state.DEAD] = "dead" }

local function companionsCommand(pid, cmd)
    local list = companionList(pid)
    if #list == 0 then
        tes3mp.SendMessage(pid, color.White .. "You have no companions.\n", false)
        return
    end
    local lines = {}
    for number, entry in ipairs(list) do
        local follower = entry.follower
        local where
        if entry.dismissed then
            where = "sent back, in " .. tostring(follower.postCell)
        else
            where = (stateNames[follower.state] or "?") ..
                (follower.state ~= state.FOLLOWING and follower.cell and (", in " .. follower.cell) or "")
        end
        table.insert(lines, number .. ". " .. nameOfCompanion(entry) .. " - " .. where)
    end
    tes3mp.SendMessage(pid, color.White .. "Your companions:\n" .. table.concat(lines, "\n") .. "\n" .. color.Yellow ..
        "/follow <number>" .. color.White .. " asks one to follow you (go where they are), " .. color.Yellow ..
        "/dismiss <number>" .. color.White .. " (or " .. color.Yellow .. "/dismiss all" .. color.White ..
        ") sends one back where it came from.\n", false)
end

local function dismissCommand(pid, cmd)
    local list = {}
    for _, entry in ipairs(companionList(pid)) do
        if not entry.dismissed then
            table.insert(list, entry)
        end
    end
    local which = cmd[2] and string.lower(cmd[2]) or ""
    local chosen = {}
    if which == "all" then
        chosen = list
    elseif tonumber(which) ~= nil and companionList(pid)[tonumber(which)] ~= nil and
        not companionList(pid)[tonumber(which)].dismissed then
        chosen = { companionList(pid)[tonumber(which)] }
    else
        companionsCommand(pid, cmd)
        return
    end
    local followers = followersOf(pid)
    local dismissed = dismissedOf(pid)
    for _, entry in ipairs(chosen) do
        local uniqueIndex, follower = entry.uniqueIndex, entry.follower
        if follower.state == state.FOLLOWING then
            sendCopy(pid, uniqueIndex, follower, otherPids(pid), true)
        end
        sendCommand(pid, follower.cell or follower.postCell, "isowind dismiss", uniqueIndex, follower.refId or "")
        followers[uniqueIndex] = nil
        -- (a dead one is gone for good)
        if follower.state ~= state.DEAD then
            dismissed[uniqueIndex] = { refId = follower.refId, postCell = follower.postCell }
        end
        if holderOf(playerKey(pid), uniqueIndex) == playerKey(pid) then
            setHolder(playerKey(pid), uniqueIndex, nil)
        end
        tes3mp.LogMessage(enumerations.log.INFO, "[Isowind] " .. playerKey(pid) .. " dismissed " ..
            tostring(follower.refId) .. " " .. uniqueIndex)
        tes3mp.SendMessage(pid, color.White .. nameOfCompanion(entry) .. " goes back to " .. follower.postCell ..
            ". " .. color.Yellow .. "/follow" .. color.White .. " there asks them to come along again.\n", false)
    end
    Players[pid]:QuicksaveToDrive()
end

-- A companion of theirs follows the player again, if they're where it is: one sent back (at its post) or one that
-- stayed somewhere
local function followCommand(pid, cmd)
    local list = companionList(pid)
    local entry = tonumber(cmd[2] or "") ~= nil and list[tonumber(cmd[2])] or nil
    if entry == nil then
        companionsCommand(pid, cmd)
        return
    end
    local uniqueIndex, follower = entry.uniqueIndex, entry.follower
    local name = nameOfCompanion(entry)
    if not entry.dismissed and follower.state == state.FOLLOWING then
        tes3mp.SendMessage(pid, color.White .. name .. " already follows you.\n", false)
        return
    elseif not entry.dismissed and follower.state == state.DEAD then
        tes3mp.SendMessage(pid, color.White .. name .. " is dead.\n", false)
        return
    end
    local holder = holderOf(playerKey(pid), uniqueIndex)
    if holder ~= nil and holder ~= playerKey(pid) then
        tes3mp.SendMessage(pid, color.White .. name .. " is with " .. holder .. ".\n", false)
        return
    end
    local where = entry.dismissed and follower.postCell or follower.cell or follower.postCell
    if not isNear(pid, where) then
        tes3mp.SendMessage(pid, color.White .. name .. " is in " .. where .. ": go there to ask them.\n", false)
        return
    end

    local followers = followersOf(pid)
    if entry.dismissed then
        dismissedOf(pid)[uniqueIndex] = nil
        follower = { refId = follower.refId, postCell = follower.postCell }
        followers[uniqueIndex] = follower
    end
    follower.state = state.FOLLOWING
    follower.cell = tes3mp.GetCell(pid)
    setHolder(playerKey(pid), uniqueIndex, playerKey(pid))
    sendCommand(pid, follower.postCell, withCopyNumber("isowind follow", pid, uniqueIndex), uniqueIndex,
        follower.refId or "")
    Players[pid]:QuicksaveToDrive()
    tes3mp.LogMessage(enumerations.log.INFO, "[Isowind] " .. tostring(follower.refId) .. " " .. uniqueIndex ..
        " follows " .. playerKey(pid) .. " again")
    tes3mp.SendMessage(pid, color.White .. name .. " follows you.\n", false)
end

customCommandHooks.registerCommand("companions", companionsCommand)
customCommandHooks.registerCommand("dismiss", dismissCommand)
customCommandHooks.registerCommand("follow", followCommand)

-- Before isowindMmo.lua's (required after this), whose OnClientScriptLocal takes the rest of the gameplay ones
customEventHooks.registerHandler("OnServerPostInit", function(eventStatus)
    WorldInstance.data.isowindMovedSince = WorldInstance.data.isowindMovedSince or {}
    customEventHooks.registerValidator("OnCellLoad", putBackDisplaced)
    customEventHooks.registerHandler("OnActorCellChange", onActorCellChange)
    customEventHooks.registerValidator("OnActorAI", onActorAI)
    customEventHooks.registerValidator("OnClientScriptLocal", onClientScriptLocal)
    customEventHooks.registerHandler("OnCellLoad", onCellLoad)
    -- Their followers' copies go from the others' games
    customEventHooks.registerValidator("OnPlayerDisconnect", function(eventStatus, pid)
        if Players[pid] ~= nil then
            for uniqueIndex, follower in pairs(followersOf(pid)) do
                if follower.state == state.FOLLOWING then
                    sendCopy(pid, uniqueIndex, follower, otherPids(pid), true)
                end
            end
            handOverFollowing(pid)
            placed[playerKey(pid)] = nil
            copyNumbers[playerKey(pid)] = nil
        end
    end)
end)

return isowindFollowers
