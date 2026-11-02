-- Click to move, with the mouse in the top-down view (Options -> Scripts -> Isowind, or the question asked the first
-- time the game is played: moveprompt.lua).
--   Right click     on the ground runs there (held: the spot follows the cursor); on a person or a thing walks right
--                   up to it and uses it (talks, picks up, opens); it never attacks: on a hostile creature, to the
--                   ground there
--   Left click      as without click to move (topdown.lua): an attack or a spell where the character stands, or with
--                   nothing ready, use what is under the cursor
--   Shift held      the character stands still (topdown.lua): no click sends them anywhere, nor WASD, and they keep
--                   aiming at the cursor (it isn't walking, as it is without click to move)
-- With the left button chosen to move instead (MoveButton), it does all of that, and on a hostile creature with a weapon
-- or spell ready walks into range (bows, thrown weapons and target spells stop short, with a clear shot) and attacks
-- once, held attacks again and again; Shift and left click attacks where the character stands, and the right button
-- power attacks. WASD or a stick takes over at once; with a controller it is as ever.
-- Used by topdown.lua, which lends what it knows (init) and turns the way to go into the character's movement.

local camera = require('openmw.camera')
local core = require('openmw.core')
local input = require('openmw.input')
local nearby = require('openmw.nearby')
local self = require('openmw.self')
local storage = require('openmw.storage')
local types = require('openmw.types')
local util = require('openmw.util')

local Actor = types.Actor
local v3 = util.vector3
local controls = storage.playerSection('Settings/Isowind/Controls') -- MoveButton (settings.lua)

local WAYPOINT_REACHED = 40         -- a point on the way this close counts as passed...
local GROUND_ARRIVED = 30           -- ...and the spot clicked this close as arrived at
local USE_DISTANCE = 45             -- people and things are used from this close (to the side of their box)...
local REACH_MARGIN = 0.5            -- ...and melee and touch attacks walk this far into their reach
local RANGED_ENGAGE = 1200          -- bows, thrown weapons and spells cast at a target attack from this close
local STEER_INTERVAL = 0.15         -- how often the spot (and the way) is found again while the button is held
local TARGET_MOVED = 100            -- the way is found again once the target is this far from where it was found to
local STUCK_TIME = 0.5              -- getting no closer for this long finds the way again (or, near enough to what
                                    -- was clicked, uses or attacks it from there: a counter between)...
local STUCK_TRIES = 3               -- ...this many times, then the character gives up
local ATTACK_WINDUP = 0.45          -- a click's attack is held this long (at weapon speed 1) for a full swing or draw
local ATTACK_REST = 0.1             -- with the button held on the enemy, let go this long between attacks
local GROUND_NORMAL_Z = 0.7         -- a surface this level or more can be walked to (up 1, a wall 0)
local SNAP_BOX = v3(30, 30, 60)     -- a surface clicked counts as on the navigation mesh this near it (half extents)
local STAIRS_REACH = 400            -- stairs clicked anywhere (a side, the back): their steps up to this far above it
local STEP_SNAP_HEIGHT = 40         -- the navigation mesh found for a step must be this near it in height (not the floor)
local DIRECT_END = 50               -- a way found ending farther than this from stairs clicked: straight there instead
local NAV_STEP = 12                 -- past a surface that can't be walked on, the cursor's ray is followed in steps this
local NAV_DEPTH = 300               -- long, this far, looking for walkable ground hidden behind it...
local NAV_PROBES = 80               -- ...at most this many looks for the whole ray
local NAV_PROBE_BOX = v3(10, 10, 30) -- how near the ray the ground must come (half extents)
local STAIRS_WORDS = { 'stair', 'step', 'ramp' } -- in a static's id or model: stairs (Morrowind has no kind for them)

-- What topdown.lua lends (init): weaponReach(), RANGED_REACH, isHostile(actor), lockToolReady(),
-- withinReachHeight(object), activate(object), ACTIVATE_SLACK
local lent

-- Where the character is going: { kind = 'ground' | 'use' | 'attack', point, object, button (the one clicked), direct
-- (stairs: straight there where the navigation mesh doesn't reach) }
local order = nil
local path, pathIndex = nil, 1      -- the way there (nearby.findPath), and the point on it gone for next
local pathTo = nil                  -- where the way was found to
local steerIn = 0                   -- seconds until the spot held to is found again
local stuckFor, stuckTries, closest = 0, 0, math.huge
local steering = false              -- the button is held from a click on the ground: the spot follows the cursor
local attackFor = 0                 -- the click's own attack: seconds left holding it (use), then...
local restFor = 0                   -- ...seconds left letting go
local castPending = false           -- ...or the click's spell, cast this frame
local shiftAttack = false           -- Shift and click: the engine's own attack where the character stands, as held
local isStairsRecord = {}           -- record id -> whether it is stairs

local M = {}

function M.init(fromTopDown)
    lent = fromTopDown
end

-- The mouse button that moves: 3 the right one (the default), 1 the left
function M.button()
    return controls:get('MoveButton') == 'move_button_left' and 1 or 3
end

function M.cancel()
    order, path, pathTo, steering = nil, nil, nil, false
    attackFor, restFor, castPending = 0, 0, false
end

-- Whether the character is going somewhere
function M.going()
    return order ~= nil
end

-- The spot clicked on the ground being gone to (for the red X), or nil
function M.groundTarget()
    return order and order.kind == 'ground' and order.point or nil
end

local function startOrder(kind, point, object, button)
    order = { kind = kind, point = point, object = object, button = button }
    path, pathTo = nil, nil
    steerIn, stuckFor, stuckTries, closest = STEER_INTERVAL, 0, 0, math.huge
end

local function activateReach()
    return core.getGMST('iMaxActivateDist') + lent.ACTIVATE_SLACK
end

-- How far the player is across the ground from something: to the nearest side of its box
local function gapTo(object)
    local box = object:getBoundingBox()
    local p = self.position
    local x = util.clamp(p.x, box.center.x - box.halfSize.x, box.center.x + box.halfSize.x)
    local y = util.clamp(p.y, box.center.y - box.halfSize.y, box.center.y + box.halfSize.y)
    return math.sqrt((p.x - x) ^ 2 + (p.y - y) ^ 2)
end

local function isStairs(object)
    if not (object and types.Static.objectIsInstance(object)) then return false end
    local id = object.recordId
    local known = isStairsRecord[id]
    if known == nil then
        local ok, record = pcall(types.Static.record, object)
        local text = id .. ' ' .. string.lower(ok and record and record.model or '')
        known = false
        for _, word in ipairs(STAIRS_WORDS) do
            if text:find(word, 1, true) then known = true end
        end
        isStairsRecord[id] = known
    end
    return known
end

-- The range (core.magic.RANGE) of the spell or enchanted item ready to cast, by its first effect
local function castRange()
    local effects
    local spell = Actor.getSelectedSpell(self)
    if spell then
        effects = spell.effects
    else
        local item = Actor.getSelectedEnchantedItem(self)
        local ok, record = pcall(function() return item.type.record(item) end)
        local enchantment = ok and record and record.enchant and core.magic.enchantments.records[record.enchant]
        effects = enchantment and enchantment.effects
    end
    local first = effects and effects[1]
    return first and first.range or core.magic.RANGE.Touch
end

-- How close to go to attack with what is ready: the distance, its full reach (attacked from as far as that when a
-- table or the like is in the way), and whether that is from afar (then it wants a clear shot)
local function attackDistance()
    if Actor.getStance(self) == Actor.STANCE.Spell then
        local range = castRange()
        if range == core.magic.RANGE.Self then return math.huge, math.huge, false end
        if range == core.magic.RANGE.Target then return RANGED_ENGAGE, RANGED_ENGAGE, true end
        local touch = core.getGMST('fCombatDistance')
        return touch * REACH_MARGIN, touch, false
    end
    local reach = lent.weaponReach()
    if reach >= lent.RANGED_REACH then return RANGED_ENGAGE, RANGED_ENGAGE, true end
    return reach * REACH_MARGIN, reach, false
end

local function clearShot(target)
    local hit = nearby.castRay(self.position + v3(0, 0, 100), target:getBoundingBox().center, { ignore = self })
    return not hit.hit or hit.hitObject == target
end

-- A click's attack: a weapon held long enough for a full swing or draw (quicker weapons need less), a spell cast once
local function startAttack()
    restFor = ATTACK_REST
    if Actor.getStance(self) == Actor.STANCE.Spell then
        castPending = true
        return
    end
    local weapon = Actor.getEquipment(self, Actor.EQUIPMENT_SLOT.CarriedRight)
    local speed = 1
    if weapon and types.Weapon.objectIsInstance(weapon) then
        speed = math.max(types.Weapon.record(weapon).speed, 0.25)
    end
    attackFor = ATTACK_WINDUP / speed
end

local function attacking()
    return castPending or attackFor > 0 or restFor > 0
end

-- Whether the click's own attack is going on (topdown.lua's aim line shows meanwhile)
function M.attacking()
    return attacking()
end

-- Whether a spell (or enchanted item) cast at a target is ready
function M.targetSpellReady()
    return Actor.getStance(self) == Actor.STANCE.Spell and castRange() == core.magic.RANGE.Target
end

-- The way there over the navigation mesh (straight there where there is none, or none yet). Stairs clicked (direct)
-- whose way doesn't end on them (tall stairs the navigation mesh stops part way up, though the character can climb
-- them): straight there, not round to the floor nearest them, behind or under them.
local function findWay(destination)
    local ok, status, points = pcall(nearby.findPath, self.position, destination,
        { agentBounds = Actor.getPathfindingAgentBounds(self) })
    local S = nearby.FIND_PATH_STATUS
    if ok and (status == S.Success or status == S.PartialPath) and #points > 0 then
        path = points
        if order and order.direct and (status ~= S.Success or (points[#points] - destination):length() > DIRECT_END) then
            path = { destination }
        end
        -- Where the navigation mesh misses the floor the player stands on, the way starts from the one nearest below
        -- (or above) it: a spot on this same floor is gone to straight, not down there and back up
        local level = STEP_SNAP_HEIGHT * 1.5
        if math.abs(points[1].z - self.position.z) > level and math.abs(destination.z - self.position.z) <= level then
            path = { destination }
        end
    else
        path = { destination }
    end
    pathIndex, pathTo = 1, destination
end

-- What a click on the ground picked, in openmw.log, with LOG_GROUND on (for finding where clicks on upper floors go)
local LOG_GROUND = false
local function note(kind, point, hit)
    if not LOG_GROUND then return end
    local function at(p) return p and string.format('(%.0f, %.0f, %.0f)', p.x, p.y, p.z) or 'none' end
    print(string.format('[Isowind click to move] %s: %s, ray hit %s, player %s', kind, at(point), at(hit),
        at(self.position)))
end

-- The top of the stairs (a step) straight above or below where they were hit: where a click on their side, back or
-- underside means. Past anything else above (a floor over them).
local function stairsTop(object, point)
    local from, to = point + v3(0, 0, STAIRS_REACH), point - v3(0, 0, 1)
    for _ = 1, 4 do
        local hit = nearby.castRay(from, to, { ignore = self })
        if not hit.hit then return nil end
        if hit.hitObject == object then return hit.hitPos end
        from = hit.hitPos - v3(0, 0, 1)
    end
    return nil
end

-- Where the cursor's ray meets the ground, for a click there: never on what stands between (a tree, a rooftop, a wall),
-- nor behind or under the stairs. Morrowind has no kind of object for stairs: they are statics with 'stair' (or
-- 'step', 'ramp') in their id or model. Along the ray, the first of:
--   stairs, hit anywhere (not what the cutaway removes): the step above where the ray hit them, on the navigation mesh
--     if it is there and the player can walk to it; else that step itself, gone to straight where the navigation mesh
--     stops (direct: tall stairs it covers only part way, which the character climbs all the same)
--   a floor (facing up: GROUND_NORMAL_Z; not people, not what the cutaway removes) the player can walk to, or where
--     the navigation mesh misses it, that spot itself (direct); never past it to a floor below
--   walkable ground hidden just behind anything else in the way (not a floor)
-- So not a rooftop in front of the street, nor the floor behind or under the stairs. With nothing reachable (no
-- navigation mesh yet), the surface nearest the player's own level; over an edge, the plane at the player's feet.
-- from, dir: the camera and the cursor's direction from it. Returns the spot (nil: none), and whether it is direct.
function M.ground(from, dir)
    local bounds = Actor.getPathfindingAgentBounds(self)
    local function reachable(point)
        local ok, status = pcall(nearby.findPath, self.position, point, { agentBounds = bounds })
        return ok and status == nearby.FIND_PATH_STATUS.Success
    end
    local function onNavMesh(point, box, maxHeight)
        local ok, found = pcall(nearby.findNearestNavMeshPosition, point,
            { agentBounds = bounds, searchAreaHalfExtents = box })
        if ok and found and (not maxHeight or math.abs(found.z - point.z) <= maxHeight) and reachable(found) then
            return found
        end
        return nil
    end
    local probes = NAV_PROBES
    local unreachable = nil         -- the last ground found and not reachable (a roof): not tried again near there
    local function groundBehind(start)
        for s = 0, NAV_DEPTH, NAV_STEP do
            if probes <= 0 then return nil end
            probes = probes - 1
            local ok, point = pcall(nearby.findNearestNavMeshPosition, start + dir * s,
                { agentBounds = bounds, searchAreaHalfExtents = NAV_PROBE_BOX })
            if ok and point and not (unreachable and (point - unreachable):length() < 50) then
                if reachable(point) then return point end
                unreachable = point
            end
        end
        return nil
    end

    local to = from + dir * 30000
    local start = from
    local best, bestHeight = nil, math.huge
    for _ = 1, 12 do
        local result = nearby.castRenderingRay(start, to, { ignore = self })
        if not result.hit then break end
        local object = result.hitObject
        local isActor = object and Actor.objectIsInstance(object)
        local whole = object and (isActor or types.Door.objectIsInstance(object))
        local normal = result.hitNormal
        local facing = normal and normal:dot(dir) < 0
        if not isActor and not camera.isCutAway(result.hitPos, whole, object) then
            if isStairs(object) then
                -- (never on past them: that is the floor behind or under them)
                local top = stairsTop(object, result.hitPos)
                note('stairs ' .. object.recordId, top, result.hitPos)
                if not top then return nil, false end
                return onNavMesh(top, SNAP_BOX, STEP_SNAP_HEIGHT) or top, true
            elseif facing and normal.z >= GROUND_NORMAL_Z then
                -- A floor: the navigation mesh on it (this floor's, never one a little below or above it:
                -- STEP_SNAP_HEIGHT), if the player can walk there; mapped but out of reach (a rooftop), on past it; and
                -- where the navigation mesh misses it (though the character walks on it), that very spot, gone to
                -- straight (direct). Never past a floor to one below it.
                local ok, found = pcall(nearby.findNearestNavMeshPosition, result.hitPos,
                    { agentBounds = bounds, searchAreaHalfExtents = SNAP_BOX })
                if ok and found and math.abs(found.z - result.hitPos.z) <= STEP_SNAP_HEIGHT then
                    if reachable(found) then
                        note('floor', found, result.hitPos)
                        return found, false
                    end
                else
                    note('floor (not on the navigation mesh)', result.hitPos, found)
                    return result.hitPos, true
                end
                local height = math.abs(result.hitPos.z - self.position.z)
                if height < bestHeight then best, bestHeight = result.hitPos, height end
            elseif facing then
                local behind = groundBehind(result.hitPos)
                if behind then
                    note('behind ' .. tostring(object and object.recordId or 'terrain'), behind, result.hitPos)
                    return behind, false
                end
            end
        end
        start = result.hitPos + dir * 2
    end
    if not best and dir.z < -0.001 then
        best = from + dir * ((self.position.z - from.z) / dir.z)
    end
    note('nothing reachable', best)
    return best, false
end

-- Which way along the path from here, across the ground (a direction as topdown.lua's wantedDirection gives it)
local function pathDirection(destination)
    local near = WAYPOINT_REACHED * WAYPOINT_REACHED
    while pathIndex < #path do
        local d = path[pathIndex] - self.position
        if d.x * d.x + d.y * d.y > near then break end
        pathIndex = pathIndex + 1
    end
    local d = path[pathIndex] - self.position
    -- At the path's end, which can stop short of the spot (the navigation mesh's edge): straight on to it
    if pathIndex == #path and d.x * d.x + d.y * d.y <= near then
        d = destination - self.position
    end
    local length = math.sqrt(d.x * d.x + d.y * d.y)
    if length < 1 then return 0, 0 end
    return d.x / length, d.y / length
end

-- The button held from a click on the ground: the spot follows the cursor until it is let go (found every
-- STEER_INTERVAL, the way with it: it takes a dozen rays), whether or not the character has caught up with it
local function updateSteering(dt, groundFn)
    if not steering then return end
    if input.isMouseButtonPressed(order.button) then
        steerIn = steerIn - dt
        if steerIn <= 0 then
            steerIn = STEER_INTERVAL
            local spot, direct = groundFn()
            if spot then
                order.point, order.direct = spot, direct
                path = nil
            end
        end
        return
    end
    steering = false
    stuckFor, closest = 0, math.huge
    findWay(order.point)
end

-- Getting no closer (STUCK_TIME): near enough to what was clicked, it is used or attacked from where the character
-- stands (a counter between); else the way is found again, a few times, then the character gives up. True when it
-- has given up.
local function onStuck(destination)
    local object = order.object
    if order.kind == 'use' and gapTo(object) <= activateReach() and lent.withinReachHeight(object) then
        order.close = true
        return false
    elseif order.kind == 'attack' then
        local _, fullReach = attackDistance()
        if gapTo(object) <= fullReach then
            order.close = true
            return false
        end
    end
    stuckTries = stuckTries + 1
    if stuckTries > STUCK_TRIES then return true end
    findWay(destination)
    return false
end

-- Carries out the order: the way to go (as wantedDirection gives it) and where to aim (nil: as ever, so with a bow
-- drawn the character still aims at the cursor while running the other way; a point: the target attacked), or nothing
-- with no order. groundFn: where the cursor points on the ground (topdown.lua's groundUnderCursor)
function M.update(dt, groundFn)
    if not order then return nil end
    updateSteering(dt, groundFn)
    local object = order.object
    if object and not (object:isValid() and object.enabled) then
        M.cancel()
        return nil
    end
    local destination = object and object.position or order.point

    if order.kind == 'ground' then
        local d = destination - self.position
        if d.x * d.x + d.y * d.y <= GROUND_ARRIVED * GROUND_ARRIVED then
            if not steering then M.cancel() end
            return 0, 0, nil
        end
    elseif order.kind == 'use' then
        if (order.close or gapTo(object) <= USE_DISTANCE) and lent.withinReachHeight(object) then
            M.cancel()
            lent.activate(object)
            return 0, 0, nil
        end
    else
        -- Attack: once in range, stand and attack, again and again while the button stays held
        if Actor.isDead(object) or Actor.getStance(self) == Actor.STANCE.Nothing then
            M.cancel()
            return nil
        end
        if order.attacked and not attacking() and not input.isMouseButtonPressed(order.button) then
            M.cancel()
            return nil
        end
        local center = object:getBoundingBox().center
        local reach, fullReach, ranged = attackDistance()
        local gap = gapTo(object)
        if (gap <= reach or (order.close and gap <= fullReach)) and (not ranged or clearShot(object)) then
            if not attacking() then
                startAttack()
                order.attacked = true
            end
            return 0, 0, center
        end
        -- An attack begun is finished where it was begun; then, held, after it again should it have stepped away
        if attacking() then return 0, 0, center end
    end

    -- On the way: found again every so often while the spot follows the cursor, once the target has moved off where it
    -- was found to, and when getting no closer
    if not path or (pathTo - destination):length() > TARGET_MOVED then
        findWay(destination)
    end
    if not steering then
        local remaining = (destination - self.position):length()
        if remaining < closest - 5 then
            closest, stuckFor = remaining, 0
        else
            stuckFor = stuckFor + dt
            if stuckFor > STUCK_TIME then
                stuckFor, closest = 0, remaining
                if onStuck(destination) then
                    M.cancel()
                    return 0, 0, nil
                end
            end
        end
    end
    local dx, dy = pathDirection(destination)
    return dx, dy, nil
end

-- A click of the move button (M.button: 1 left, 3 right): what the cursor is on (hovered, or nil), and groundFn, where
-- it points on the ground (M.ground). Returns the spot when the character is to go there (for the red X).
function M.click(hovered, groundFn, button)
    local stance = Actor.getStance(self)
    local target = hovered and hovered:isValid() and hovered or nil
    -- Shift held: the character stands where they are (topdown.lua), so a click doesn't send them anywhere; with the
    -- left button moving, it attacks where they stand
    if input.isShiftPressed() then
        M.cancel()
        shiftAttack = button == 1 and stance ~= Actor.STANCE.Nothing
        return nil
    end
    local hostile = target and Actor.objectIsInstance(target) and lent.isHostile(target)
    if hostile and button == 3 then
        target = nil -- the right button never attacks: to the ground there, as anywhere
    end
    if target and hostile and stance ~= Actor.STANCE.Nothing and not lent.lockToolReady() then
        startOrder('attack', nil, target, button)
    elseif target then
        startOrder('use', nil, target, button)
    else
        local spot, direct = groundFn()
        if spot then
            startOrder('ground', spot, nil, button)
            order.direct = direct
            steering = true
            return spot
        end
    end
    return nil
end

-- The click's own attack; with the left button moving, the left button doesn't attack by itself (it is bound to Use,
-- and a click on the ground moves): then only the click's attack, Shift and click, and the power attack (the right
-- button) do. With the right button moving the left one attacks where the character stands as ever. Runs after the
-- built-in controls (scripts/omw/input/playercontrols.lua) have set use. enabled: click to move is on, with the mouse.
function M.updateAttack(dt, enabled)
    if shiftAttack and not input.isMouseButtonPressed(1) then shiftAttack = false end
    if not enabled or shiftAttack then return end
    local leftMoves = M.button() == 1
    if leftMoves and Actor.getStance(self) == Actor.STANCE.Weapon and input.isMouseButtonPressed(3) then return end
    if not leftMoves and not attacking() then return end
    local use = self.ATTACK_TYPE.NoAttack
    if castPending then
        castPending = false
        use = self.ATTACK_TYPE.Any
    elseif attackFor > 0 then
        attackFor = attackFor - dt
        use = self.ATTACK_TYPE.Any
    elseif restFor > 0 then
        restFor = restFor - dt
    end
    self.controls.use = use
end

return M
