-- Isowind: the server window's side of things (the server window in Isowind.exe, ServerForm). It talks to the server
-- through two files in the server's folder (where the server runs), one line each, the fields separated by tabs:
--
--   isowind-admin-status.txt     written every second: the players, the bans, the world's time, the commands' results
--       time      <os time>                                   (the window takes the server as alive while it changes)
--       uptime    <seconds>
--       world     <hour> <day> <month> <year>
--       player    <pid> <name> <staff rank> <ping> <IP> <cell> <region> <level>
--       banname   <account name>
--       banip     <IP address>
--       result    <command id> <1 done, 0 not> <what happened>
--   isowind-admin-commands.txt   appended to by the window, read from where it was read up to:
--       <id> rank <pid> <0 player, 1 moderator, 2 admin, 3 server owner>
--       <id> kick <pid>                  <id> banplayer <pid> (the account and its IP addresses)
--       <id> banname <name>              <id> banip <IP>
--       <id> unbanname <name>            <id> unbanip <IP>
--       <id> say <text>                  <id> tell <pid> <text>
--       <id> save                        <id> stop (saves first)
--       <id> hour <0 to 24>              <id> weather <pid> <0 to 9> (the weather in that player's region)
--       <id> teleport <pid> <to pid>     <id> resetcell <cell>
--       <id> startup (runs the world's startup scripts again, through a player in the world)

IsowindAdmin = {}

local statusFile = "isowind-admin-status.txt"
local commandsFile = "isowind-admin-commands.txt"
local commandOffset = 0
local results = {}
local tickTimer = nil

local weatherNames = { [0] = "clear", "cloudy", "foggy", "overcast", "rain", "thunder", "ash", "blight", "snow",
    "blizzard" }
local rankNames = { [0] = "player", "moderator", "admin", "server owner" }

local function field(value)
    return (tostring(value == nil and "" or value):gsub("[\t\r\n]", " "))
end

local function log(text)
    tes3mp.LogMessage(enumerations.log.INFO, "[Isowind admin] " .. text)
end

-- What a command did: in the server's log, and back to the window (the last 20)
local function result(id, ok, text)
    log(text)
    table.insert(results, { id = id, ok = ok, text = text })
    while #results > 20 do
        table.remove(results, 1)
    end
end

local function loggedIn(pid)
    pid = tonumber(pid)
    if pid ~= nil and Players[pid] ~= nil and Players[pid]:IsLoggedIn() then
        return pid
    end
    return nil
end

-- Any player in the world: some things only reach the world through a player's game
local function anyPlayer()
    for pid, player in pairs(Players) do
        if player:IsLoggedIn() then
            return pid
        end
    end
    return nil
end

function IsowindAdmin.SaveAll()
    for _, player in pairs(Players) do
        if player:IsLoggedIn() then
            player:QuicksaveToDrive()
        end
    end
    for _, cell in pairs(LoadedCells) do
        cell:QuicksaveToDrive()
    end
    for _, store in pairs(RecordStores) do
        store:QuicksaveToDrive()
    end
    WorldInstance:QuicksaveToDrive()
end

-- The world's startup scripts (config.worldStartupScripts), through a player in the world; with nobody there, through
-- the next one to log in (custom/isowindHost.lua)
function IsowindAdmin.RunStartupScripts()
    local pid = anyPlayer()
    if pid == nil then
        WorldInstance.coreVariables.hasRunStartupScripts = false
        WorldInstance:QuicksaveToDrive()
        return "the world's startup scripts will run when the next player logs in"
    end
    for _, scriptName in pairs(config.worldStartupScripts) do
        logicHandler.RunConsoleCommandOnPlayer(pid, "startscript " .. scriptName, false)
    end
    WorldInstance.coreVariables.hasRunStartupScripts = true
    WorldInstance:QuicksaveToDrive()
    return "ran the world's startup scripts through " .. Players[pid].name
end

local function banAccount(id, name)
    local accountName = string.lower(fileHelper.fixFilename(name))
    local player = logicHandler.GetPlayerByName(name)
    if player == nil then
        return result(id, false, name .. " has no account on this server")
    end
    if tableHelper.containsValue(banList.playerNames, accountName) then
        return result(id, false, name .. " was already banned")
    end
    table.insert(banList.playerNames, accountName)
    SaveBanList()
    for _, ipAddress in pairs(player.data.ipAddresses) do
        tes3mp.BanAddress(ipAddress)
    end
    -- Banning an address doesn't drop who is already in
    for pid, online in pairs(Players) do
        if online:IsLoggedIn() and string.lower(online.accountName) == accountName then
            online:Kick()
        end
    end
    result(id, true, "banned " .. name .. " and the addresses they played from")
end

local commands = {}

commands.rank = function(id, args)
    local pid, rank = loggedIn(args[1]), tonumber(args[2])
    if pid == nil or rank == nil or rank < 0 or rank > 3 then
        return result(id, false, "no such player, or no such rank")
    end
    local player = Players[pid]
    player.data.settings.staffRank = math.floor(rank)
    player:QuicksaveToDrive()
    tes3mp.SendMessage(pid, "You are now " .. (rank == 0 and "a player" or "a " .. rankNames[rank]) ..
        " on this server.\n", false)
    result(id, true, player.name .. " is now " .. rankNames[rank])
end

commands.kick = function(id, args)
    local pid = loggedIn(args[1])
    if pid == nil then
        return result(id, false, "no such player")
    end
    local name = Players[pid].name
    tes3mp.SendMessage(pid, name .. " was kicked from the server.\n", true)
    Players[pid]:Kick()
    result(id, true, "kicked " .. name)
end

commands.banplayer = function(id, args)
    local pid = loggedIn(args[1])
    if pid == nil then
        return result(id, false, "no such player")
    end
    banAccount(id, Players[pid].accountName)
end

commands.banname = function(id, args)
    if args[1] == nil or args[1] == "" then
        return result(id, false, "no name")
    end
    banAccount(id, args[1])
end

commands.banip = function(id, args)
    local ipAddress = args[1]
    if ipAddress == nil or ipAddress == "" then
        return result(id, false, "no address")
    end
    if tableHelper.containsValue(banList.ipAddresses, ipAddress) then
        return result(id, false, ipAddress .. " was already banned")
    end
    table.insert(banList.ipAddresses, ipAddress)
    SaveBanList()
    tes3mp.BanAddress(ipAddress)
    for pid, player in pairs(Players) do
        if player:IsLoggedIn() and tes3mp.GetIP(pid) == ipAddress then
            player:Kick()
        end
    end
    result(id, true, "banned " .. ipAddress)
end

commands.unbanname = function(id, args)
    local accountName = string.lower(args[1] or "")
    if not tableHelper.containsValue(banList.playerNames, accountName) then
        return result(id, false, (args[1] or "") .. " is not banned")
    end
    tableHelper.removeValue(banList.playerNames, accountName)
    SaveBanList()
    local player = logicHandler.GetPlayerByName(accountName)
    if player ~= nil then
        for _, ipAddress in pairs(player.data.ipAddresses) do
            tes3mp.UnbanAddress(ipAddress)
        end
    end
    result(id, true, "unbanned " .. accountName)
end

commands.unbanip = function(id, args)
    local ipAddress = args[1] or ""
    if not tableHelper.containsValue(banList.ipAddresses, ipAddress) then
        return result(id, false, ipAddress .. " is not banned")
    end
    tableHelper.removeValue(banList.ipAddresses, ipAddress)
    SaveBanList()
    tes3mp.UnbanAddress(ipAddress)
    result(id, true, "unbanned " .. ipAddress)
end

commands.say = function(id, args)
    local pid = anyPlayer()
    local text = table.concat(args, " ")
    if pid == nil then
        return result(id, false, "nobody is in the world to hear it")
    end
    tes3mp.SendMessage(pid, color.Yellow .. "[Server] " .. color.White .. text .. "\n", true)
    result(id, true, "said to everyone: " .. text)
end

commands.tell = function(id, args)
    local pid = loggedIn(args[1])
    if pid == nil then
        return result(id, false, "no such player")
    end
    local text = table.concat(args, " ", 2)
    tes3mp.SendMessage(pid, color.Yellow .. "[Server to you] " .. color.White .. text .. "\n", false)
    result(id, true, "said to " .. Players[pid].name .. ": " .. text)
end

commands.save = function(id, args)
    IsowindAdmin.SaveAll()
    result(id, true, "saved the world, the players and the loaded cells")
end

commands.stop = function(id, args)
    IsowindAdmin.SaveAll()
    result(id, true, "saved; stopping the server")
    IsowindAdmin.WriteStatus()
    tes3mp.StopServer(0)
end

commands.hour = function(id, args)
    local hour = tonumber(args[1])
    if hour == nil or hour < 0 or hour > 24 then
        return result(id, false, "not an hour of the day")
    end
    if hour == 24 then
        hour = 0
    end
    WorldInstance.data.time.hour = hour
    WorldInstance:QuicksaveToDrive()
    local pid = anyPlayer()
    if pid ~= nil then
        WorldInstance:LoadTime(pid, true)
    end
    hourCounter = hour
    result(id, true, string.format("the time is now %02d:%02d", math.floor(hour), math.floor(hour % 1 * 60)))
end

commands.weather = function(id, args)
    local pid, weather = loggedIn(args[1]), tonumber(args[2])
    if pid == nil or weather == nil or weatherNames[weather] == nil then
        return result(id, false, "no such player, or no such weather")
    end
    local regionName = Players[pid].data.location.regionName
    local region = regionName ~= nil and WorldInstance.storedRegions[regionName] or nil
    if region == nil then
        return result(id, false, Players[pid].name .. " is not in a region with weather (indoors?)")
    end
    region.currentWeather = weather
    region.nextWeather = -1
    region.queuedWeather = -1
    region.transitionFactor = 0
    WorldInstance:LoadRegionWeather(regionName, pid, true, true)
    result(id, true, "the weather in " .. regionName .. " is now " .. weatherNames[weather])
end

commands.teleport = function(id, args)
    local pid, targetPid = loggedIn(args[1]), loggedIn(args[2])
    if pid == nil or targetPid == nil or pid == targetPid then
        return result(id, false, "no such players")
    end
    logicHandler.TeleportToPlayer(pid, pid, targetPid)
    result(id, true, "teleported " .. Players[pid].name .. " to " .. Players[targetPid].name)
end

commands.resetcell = function(id, args)
    local cellDescription = table.concat(args, " ")
    if cellDescription == "" then
        return result(id, false, "no cell")
    end
    local pid = anyPlayer()
    if pid ~= nil then
        logicHandler.ResetCell(pid, cellDescription)
    elseif LoadedCells[cellDescription] == nil then
        -- Nobody in the world: the cell's saved state is dropped, and it loads fresh next time
        logicHandler.LoadCell(cellDescription)
        LoadedCells[cellDescription].data.objectData = {}
        LoadedCells[cellDescription].data.packets = {}
        LoadedCells[cellDescription]:EnsurePacketTables()
        LoadedCells[cellDescription].data.loadState.hasFullActorList = false
        LoadedCells[cellDescription].data.loadState.hasFullContainerData = false
        LoadedCells[cellDescription]:ClearRecordLinks()
        logicHandler.UnloadCell(cellDescription)
    end
    result(id, true, "reset " .. cellDescription .. "; " .. IsowindAdmin.RunStartupScripts())
end

commands.startup = function(id, args)
    result(id, true, IsowindAdmin.RunStartupScripts())
end

local function run(line)
    local parts = {}
    for part in (line .. "\t"):gmatch("([^\t]*)\t") do
        table.insert(parts, part)
    end
    local id, verb = parts[1], parts[2]
    local handler = verb ~= nil and commands[verb] or nil
    if handler == nil then
        return result(id, false, "unknown command: " .. line)
    end
    local args = {}
    for i = 3, #parts do
        args[#args + 1] = parts[i]
    end
    local ok, problem = pcall(handler, id, args)
    if not ok then
        result(id, false, verb .. " failed: " .. tostring(problem))
    end
end

-- New complete lines since the last read (the window writes each command with its line end in one go)
local function readCommands()
    local file = io.open(commandsFile, "rb")
    if file == nil then
        commandOffset = 0
        return
    end
    local size = file:seek("end")
    if size < commandOffset then
        commandOffset = 0
    end
    file:seek("set", commandOffset)
    local data = file:read("*a") or ""
    file:close()
    local lastLineEnd = data:match(".*()\n")
    if lastLineEnd == nil then
        return
    end
    commandOffset = commandOffset + lastLineEnd
    for line in data:sub(1, lastLineEnd - 1):gmatch("[^\r\n]+") do
        run(line)
    end
end

function IsowindAdmin.WriteStatus()
    local lines = {
        "time\t" .. os.time(),
        "uptime\t" .. math.floor(tes3mp.GetMillisecondsSinceServerStart() / 1000),
    }
    if WorldInstance ~= nil then
        local t = WorldInstance.data.time
        table.insert(lines, "world\t" .. field(t.hour) .. "\t" .. field(t.day) .. "\t" .. field(t.month) .. "\t" ..
            field(t.year))
    end
    for pid, player in pairs(Players) do
        if player:IsLoggedIn() then
            local level = player.data.stats ~= nil and player.data.stats.level or ""
            table.insert(lines, table.concat({ "player", pid, field(player.name),
                field(player.data.settings.staffRank), field(tes3mp.GetAvgPing(pid)), field(tes3mp.GetIP(pid)),
                field(tes3mp.GetCell(pid)), field(player.data.location.regionName), field(level) }, "\t"))
        end
    end
    for _, name in ipairs(banList.playerNames or {}) do
        table.insert(lines, "banname\t" .. field(name))
    end
    for _, ipAddress in ipairs(banList.ipAddresses or {}) do
        table.insert(lines, "banip\t" .. field(ipAddress))
    end
    for _, r in ipairs(results) do
        table.insert(lines, "result\t" .. field(r.id) .. "\t" .. (r.ok and "1" or "0") .. "\t" .. field(r.text))
    end

    -- Written whole and then put in place, so the window never reads half of it
    local temporary = statusFile .. ".tmp"
    local file = io.open(temporary, "w")
    if file == nil then
        return
    end
    file:write(table.concat(lines, "\n"), "\n")
    file:close()
    os.remove(statusFile)
    os.rename(temporary, statusFile)
end

function IsowindAdminTick()
    local ok, problem = pcall(readCommands)
    if not ok then
        log("reading the commands failed: " .. tostring(problem))
    end
    ok, problem = pcall(IsowindAdmin.WriteStatus)
    if not ok then
        log("writing the status failed: " .. tostring(problem))
    end
    tes3mp.RestartTimer(tickTimer, time.seconds(1))
end

customEventHooks.registerHandler("OnServerPostInit", function(eventStatus)
    -- Commands left from an earlier run are not run again
    local file = io.open(commandsFile, "rb")
    if file ~= nil then
        commandOffset = file:seek("end")
        file:close()
    end
    tickTimer = tes3mp.CreateTimer("IsowindAdminTick", time.seconds(1))
    tes3mp.StartTimer(tickTimer)
end)
