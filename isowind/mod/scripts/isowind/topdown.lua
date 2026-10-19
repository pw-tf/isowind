-- Isowind top-down mode: an isometric-style camera that follows the player, WASD movement relative to the
-- screen, a mouse cursor the character turns to face, and click-to-interact (Diablo / Project Zomboid style).
--
--   WASD            move (W is always "up the screen")
--   mouse           move the cursor; with a weapon or spell ready the character aims at it while their legs run
--                   the way they move (twin-stick), otherwise they face the way they walk
--   left click      attack / cast with a weapon or spell ready, otherwise interact with what is under the cursor
--   right click     power attack with a weapon ready (the weapon's strongest attack type)
--   E (Activate)    interact with what is under the cursor, or else the nearest thing in reach in front
--   mouse wheel     zoom: all the way out is the top-down view, in a little is Morrowind's third person view,
--                   all the way in first person (the D-pad on a controller)
--   F7              hop between the top-down view and the normal one
--
-- With a controller (it takes over as soon as a stick or button is used, the mouse as soon as it moves):
--   left stick      move                     right stick     aim / pick what X uses
--   A               jump                     B               ready weapon
--   X               use / talk               Y               ready magic
--   right trigger   attack / cast            left trigger    power attack
--   bumpers         previous / next quick key
--   D-pad up / down zoom (hold to keep zooming), through the three views
--   D-pad left      quick keys menu (on a server: the chat, typed with the keyboard on the screen)
--   Start           menu                     Back            inventory (hold: journal)
--   left stick in   sneak                    right stick in  rest
-- There is no cursor then: what the right stick points at (or else what is nearest in front) gets the ring and the
-- label, and with a weapon or spell ready the aim locks onto the nearest hostile creature in front.
-- The menus are the engine's controller menus ('controller menus' in the profile's settings.cfg): D-pad and A / B,
-- the triggers switch pages, Y puts an item or spell on a quick key, and a keyboard on the screen types.
--
-- The zoom picks the view: zoomed out to TOPDOWN_FROM or farther it is this top-down view; in closer, Morrowind's
-- third person view at that distance; closer than FIRST_PERSON_BELOW, first person. Zooming and the first / third
-- person switch are taken from the built-in camera script for good (claimCamera).
-- The view is a fixed isometric angle: turned 45 degrees, looking down at the classic isometric 35.26 degrees.
-- The camera slides in toward the player rather than go underground on slopes, and creatures (and hurt or hostile
-- people) get health bars (healthbars.lua).

local async = require('openmw.async')
local camera = require('openmw.camera')
local core = require('openmw.core')
local input = require('openmw.input')
local nearby = require('openmw.nearby')
local postprocessing = require('openmw.postprocessing')
local self = require('openmw.self')
local storage = require('openmw.storage')
local types = require('openmw.types')
local ui = require('openmw.ui')
local util = require('openmw.util')
local I = require('openmw.interfaces')

local healthbars = require('scripts.isowind.healthbars')

local Actor = types.Actor
local Player = types.Player
local MODE = camera.MODE
local v2 = util.vector2
local v3 = util.vector3

-- Our cursor, labels and health bars get their own layer above the HUD: the HUD layer is scaled by
-- 'hud scaling factor', and these should stay at their normal size.
local OVERLAY = 'IsowindOverlay'
if not ui.layers.indexOf(OVERLAY) then
    ui.layers.insertAfter('HUD', OVERLAY, { interactive = false })
end

local controlSettings = storage.playerSection('SettingsOMWControls')
local ownSettings = storage.playerSection('IsowindTopDown')

-- Tuning ---------------------------------------------------------------------------------------------------------
local VIEW_YAW = math.rad(45)       -- the view is turned 45 degrees, so the world's grid runs diagonally
local PITCH = math.atan(1 / math.sqrt(2)) -- true isometric: looking down 35.26 degrees
local FOV = math.rad(30)            -- a narrow view from far away looks close to isometric
local MIN_DISTANCE, MAX_DISTANCE = 25, 3500 -- zoom range, from first person out to the farthest top-down view
local TOPDOWN_FROM = 500            -- zoomed out to here or farther the view is top-down...
local FIRST_PERSON_BELOW = 50       -- ...zoomed in closer than this it is first person; third person in between
local THIRD_PERSON_DISTANCE = 200   -- where F7 puts the zoom when leaving the top-down view
local ZOOM_SPEED = 8                -- how quickly the camera eases to the zoom level (per second)
local GROUND_CLEARANCE = 60         -- the camera stays at least this far above the ground
local CLOSEST_DISTANCE = 150        -- how close it may slide in to stay above the ground
local FOCUS_HEIGHT = 80             -- the camera looks at the player's waist, not their feet
local TURN_SPEED = math.rad(720)    -- how fast the character turns (per second)
local AIM_DEAD_ZONE = 80            -- ignore the cursor for aiming when it is this close to the character
local CURSOR_SPEED = 1.0            -- cursor speed relative to the mouse
local ZOOM_STEP = 0.15              -- about this fraction of the distance per wheel notch
local ACTIVATE_SLACK = 64           -- extra reach on top of the game's activation distance (top-down is imprecise)
local SNAP_RADIUS = 0.06            -- the cursor picks a creature this close to its figure (fraction of screen height)
local SNAP_RANGE = 3000             -- only creatures this close to the player are picked that way
local TAG = 'IsowindTopDown'
local STICK_DEAD_ZONE = 0.3         -- right stick deflection below this doesn't aim
local AIM_ASSIST_RANGE = 2500       -- how far the controller's aim looks for a hostile creature
local AIM_ASSIST_COS = math.cos(math.rad(55)) -- ...within this angle of where the character aims or faces
local STICK_PICK_COS = math.cos(math.rad(30)) -- what the right stick points at, for X
local JOURNAL_HOLD = 0.5            -- seconds Back is held to open the journal instead of the inventory
local DPAD_ZOOM_DELAY = 0.3         -- D-pad up / down held this long zooms continuously...
local DPAD_ZOOM_RATE = 1.6          -- ...this many times faster (or slower) per second
--------------------------------------------------------------------------------------------------------------------

local active = false                -- whether the top-down view's overrides are currently applied
local viewMode = nil                -- 'topdown', 'third' or 'first', from the zoom (wantedView)
local cameraClaimed = false         -- zoom and the first / third person switch are this script's, once claimed
local lastTopDownDistance = 1000    -- where F7 brings the zoom back to
local viewYaw = VIEW_YAW
local targetDistance = 1000         -- the zoom level asked for with the wheel
local distance = targetDistance     -- the camera's actual distance (eased, and kept above the ground)
local cursor = v2(0.5, 0.5)         -- normalised screen position, (0, 0) is the top left
local hovered = nil                 -- activatable object under the cursor
local hoveredInRange = false
local aimPoint = nil
local jumpRequested = false
local gamepad = false               -- the last input came from a controller
local stickAim = nil                -- the world direction the right stick points, or nil
local backPressedAt, journalOpened, dpadHeld = nil, false, 0

-- Defined further down
local nearestUsable, setRingVisible

local cursorElement, labelElement

local function makeUi()
    if cursorElement then return end
    -- The game's own menu pointer (openmw_pointer.xml: 32x32, tip at 7, 0).
    cursorElement = ui.create {
        layer = 'IsowindOverlay',
        type = ui.TYPE.Image,
        props = {
            relativePosition = cursor,
            anchor = v2(7 / 32, 0),
            size = v2(32, 32),
            resource = ui.texture { path = 'textures/tx_cursor.dds' },
            visible = false,
        },
    }
    labelElement = ui.create {
        layer = 'IsowindOverlay',
        type = ui.TYPE.Text,
        props = {
            relativePosition = cursor,
            position = v2(22, 2),
            anchor = v2(0, 1),
            text = '',
            multiline = true,
            textSize = 18,
            textColor = util.color.rgb(0.87, 0.79, 0.62),
            textShadow = true,
            textShadowColor = util.color.rgb(0, 0, 0),
            visible = false,
        },
    }
end

local function setUiVisible(visible)
    makeUi()
    cursorElement.layout.props.visible = visible and not gamepad
    labelElement.layout.props.visible = visible and labelElement.layout.props.text ~= ''
    cursorElement:update()
    labelElement:update()
end

local function controlsAllowed()
    return not core.isWorldPaused()
        and Player.getControlSwitch(self, Player.CONTROL_SWITCH.Controls)
        and not I.UI.getMode()
end

local function apply()
    if active then return end
    active = true
    makeUi()
    -- Running is the default (Shift walks, Caps Lock still toggles). Set only once, so a later choice sticks.
    -- With 'player running costs fatigue = false' running is free.
    if not ownSettings:get('runByDefault') then
        controlSettings:set('alwaysRun', true)
        ownSettings:set('runByDefault', true)
    end
    I.Controls.overrideMovementControls(true)
    I.Camera.disableStandingPreview(TAG)
    I.Camera.disableHeadBobbing(TAG)
    Player.setControlSwitch(self, Player.CONTROL_SWITCH.Looking, false)
    camera.showCrosshair(false)
end

local function release()
    if not active then return end
    active = false
    I.Controls.overrideMovementControls(false)
    I.Camera.enableStandingPreview(TAG)
    I.Camera.enableHeadBobbing(TAG)
    Player.setControlSwitch(self, Player.CONTROL_SWITCH.Looking, true)
    camera.setFieldOfView(camera.getBaseFieldOfView())
    self.controls.movement = 0
    self.controls.sideMovement = 0
    self.controls.yawChange = 0
    self.controls.pitchChange = 0
    setUiVisible(false)
    setRingVisible(false)
    healthbars.clear()
end

local function wrapAngle(a)
    return (a + math.pi) % (2 * math.pi) - math.pi
end

local function objectName(object)
    local ok, record = pcall(function() return object.type.record(object) end)
    if ok and record and record.name and record.name ~= '' then
        return record.name
    end
    return nil
end

-- The camera as set this frame; the cursor ray is built from it (camera.viewportToWorldVector ignores a field of
-- view set by scripts, and would describe last frame's camera anyway).
local view = { position = v3(0, 0, 0), forward = v3(0, 1, 0), right = v3(1, 0, 0), up = v3(0, 0, 1) }

local function focusPoint()
    return self.position + v3(0, 0, FOCUS_HEIGHT)
end

-- Whether the camera at distance d is above the ground (terrain only: houses and trees are cut away instead).
local function aboveGround(d, margin)
    local pos = focusPoint() - view.forward * d
    local hit = nearby.castRay(pos + v3(0, 0, 5000), pos - v3(0, 0, 5000),
        { collisionType = nearby.COLLISION_TYPE.HeightMap })
    return not hit.hit or hit.hitPos.z + margin < pos.z
end

-- The farthest distance up to d at which the camera stays above the ground, so it never ends up inside a hill.
local function groundSafeDistance(d, margin)
    if aboveGround(d, margin) then return d end
    local near, far = CLOSEST_DISTANCE, d
    for _ = 1, 6 do
        local middle = 0.5 * (near + far)
        if aboveGround(middle, margin) then near = middle else far = middle end
    end
    return near
end

local function updateCamera(dt)
    if camera.getMode() ~= MODE.Static then
        camera.setMode(MODE.Static)
    end
    local cp, sp = math.cos(PITCH), math.sin(PITCH)
    local cy, sy = math.cos(viewYaw), math.sin(viewYaw)
    view.forward = v3(sy * cp, cy * cp, -sp)
    view.right = v3(cy, -sy, 0)
    view.up = v3(sy * sp, cy * sp, cp)

    -- Ease toward the zoom level, sliding in early (and quicker) where the ground rises behind the player, and
    -- never letting the camera sink below it.
    local wanted = groundSafeDistance(targetDistance, GROUND_CLEARANCE * 3)
    local rate = wanted < distance and ZOOM_SPEED * 2 or ZOOM_SPEED
    distance = distance + (wanted - distance) * (1 - math.exp(-rate * dt))
    distance = math.min(distance, groundSafeDistance(distance, GROUND_CLEARANCE))

    view.position = focusPoint() - view.forward * distance
    camera.setStaticPosition(view.position)
    camera.setYaw(viewYaw)
    camera.setPitch(PITCH)
    camera.setFieldOfView(FOV)
end

-- World direction of the ray from the camera through the cursor.
local function cursorDirection()
    local screen = ui.screenSize()
    local tanHalf = math.tan(FOV / 2)
    local x = (cursor.x * 2 - 1) * tanHalf * screen.x / screen.y
    local y = (1 - cursor.y * 2) * tanHalf
    return (view.forward + view.right * x + view.up * y):normalize()
end

-- Normalised screen position of a world position, or nil when it is off screen or behind the camera.
local function worldToScreen(pos)
    local rel = pos - view.position
    local depth = rel:dot(view.forward)
    if depth < 1 then return nil end
    local screen = ui.screenSize()
    local tanHalf = math.tan(FOV / 2)
    local x = rel:dot(view.right) / (depth * tanHalf * screen.x / screen.y)
    local y = rel:dot(view.up) / (depth * tanHalf)
    if math.abs(x) > 1 or math.abs(y) > 1 then return nil end
    return v2((x + 1) / 2, (1 - y) / 2)
end

local function updateCursor()
    local screen = ui.screenSize()
    cursor = v2(
        util.clamp(cursor.x + input.getMouseMoveX() * CURSOR_SPEED / screen.x, 0, 1),
        util.clamp(cursor.y + input.getMouseMoveY() * CURSOR_SPEED / screen.y, 0, 1))
end

local function updateZoom()
    -- Zoom3rdPerson is 10 per wheel notch (scripts/omw/input/actionbindings.lua).
    local notches = input.getNumberActionValue('Zoom3rdPerson') / 10
    if notches ~= 0 then
        targetDistance = util.clamp(targetDistance * math.exp(-notches * ZOOM_STEP), MIN_DISTANCE, MAX_DISTANCE)
    end
end

local function segmentDistance(p, a, b)
    local ab = b - a
    local t = util.clamp((p - a):dot(ab) / math.max(ab:dot(ab), 1e-4), 0, 1)
    return (p - (a + ab * t)):length()
end

-- What the camera fades out (shaders/lib/util/cutaway.glsl): the engine fades whole objects that stand in front of
-- the player (camera.getFadedObjects), and indoors removes ceilings and upper floors over them. Floors and the
-- ground below their knees always stay.
local fadedObjects = {}

local function updateFadedObjects()
    fadedObjects = {}
    for _, object in ipairs(camera.getFadedObjects()) do
        fadedObjects[object.id] = true
    end
end

local function isCutAway(pos, object, normal)
    if pos.z < self.position.z + 25 then return false end
    if object and fadedObjects[object.id] then return true end
    -- Indoors: surfaces facing up or down above the player's head
    if self.cell and not self.cell.isExterior and pos.z > self.position.z + 200 then
        return normal ~= nil and math.abs(normal.z) > 0.5
    end
    return false
end

-- Cast through the cursor, passing through what isn't drawn: back faces (many ceilings and outer walls are only
-- visible from inside) and whatever the cutaway removes. Otherwise the cursor would land on a hidden ceiling.
local function castThroughCutaway(from, dir)
    local to = from + dir * 30000
    local start = from
    for _ = 1, 10 do
        local result = nearby.castRenderingRay(start, to, { ignore = self })
        if not result.hit then return result end
        -- Characters and doors are drawn from both sides, so their back faces are solid.
        local object = result.hitObject
        local backFace = result.hitNormal and result.hitNormal:dot(dir) > 0
            and not (object and (types.Actor.objectIsInstance(object) or types.Door.objectIsInstance(object)))
        if not backFace and not isCutAway(result.hitPos, result.hitObject, result.hitNormal) then
            return result
        end
        start = result.hitPos + dir * 2
    end
    return { hit = false }
end

-- What is under the cursor, and the point the character should face.
local function updatePick()
    local from = view.position
    local dir = cursorDirection()
    local result = castThroughCutaway(from, dir)

    hovered = nil
    hoveredInRange = false
    if result.hit then
        aimPoint = result.hitPos
        local object = result.hitObject
        if object and object ~= self.object and objectName(object) and object.type ~= types.Static then
            hovered = object
            local reach = core.getGMST('iMaxActivateDist') + ACTIVATE_SLACK
            hoveredInRange = (result.hitPos - self.position):length() <= reach
        end
    elseif dir.z < -0.001 then
        -- Nothing hit (e.g. empty sky over the edge): use the plane at the player's feet.
        local t = (self.position.z - from.z) / dir.z
        aimPoint = from + dir * t
    end

    -- Forgiving selection: unless the cursor is right on a creature or person, take the one whose figure on screen
    -- (the line from their feet to their head) is nearest to the cursor, within SNAP_RADIUS. Aim at their chest.
    if hovered and Actor.objectIsInstance(hovered) then return end
    local screen = ui.screenSize()
    local cursorPx = v2(cursor.x * screen.x, cursor.y * screen.y)
    local snapPx = SNAP_RADIUS * screen.y
    local best, bestDist, bestBox = nil, snapPx, nil
    for _, actor in ipairs(nearby.actors) do
        if actor ~= self.object and (actor.position - self.position):length() < SNAP_RANGE then
            local box = actor:getBoundingBox()
            local feet = worldToScreen(v3(box.center.x, box.center.y, box.center.z - box.halfSize.z))
            local head = worldToScreen(v3(box.center.x, box.center.y, box.center.z + box.halfSize.z))
            if feet and head then
                local dist = segmentDistance(cursorPx, v2(feet.x * screen.x, feet.y * screen.y),
                    v2(head.x * screen.x, head.y * screen.y))
                if dist < bestDist then best, bestDist, bestBox = actor, dist, box end
            end
        end
    end
    if best then
        hovered = best
        aimPoint = bestBox.center
        local d = best.position - self.position
        local gap = math.sqrt(d.x * d.x + d.y * d.y) - math.max(bestBox.halfSize.x, bestBox.halfSize.y)
        hoveredInRange = gap <= core.getGMST('iMaxActivateDist') + ACTIVATE_SLACK
    end
end

-- Selection ring under the hovered creature (shaders/isowind_selection.omwfx): blue for passive ones, light blue once in
-- reach to talk to; red for hostile ones, orange once within weapon reach.
local RING_PASSIVE, RING_PASSIVE_NEAR = v3(0.25, 0.45, 1.0), v3(0.55, 0.85, 1.0)
local RING_HOSTILE, RING_HOSTILE_NEAR = v3(0.9, 0.12, 0.08), v3(1.0, 0.55, 0.1)
local HOSTILE_FIGHT = 80
local RANGED_REACH = 4000
local RING_SCALE = 1.7       -- ring radius relative to the creature's width
local RING_MIN_RADIUS = 50

local ringShader = nil
local ringLoaded = false

local function getRingShader()
    if not ringLoaded then
        ringLoaded = true
        local ok, shader = pcall(postprocessing.load, 'isowind_selection')
        if ok then ringShader = shader end
    end
    return ringShader
end

setRingVisible = function(visible)
    local shader = getRingShader()
    if not shader then return end
    if visible and not shader:isEnabled() then
        shader:enable(0) -- first, so the retro look (palette, CRT) applies to the ring too
    elseif not visible and shader:isEnabled() then
        shader:disable()
    end
end

local function hidesRing()
    local shader = getRingShader()
    if shader and shader:isEnabled() then
        shader:setVector4('uSelection', util.vector4(0, 0, 0, 0))
    end
end

local function weaponReach()
    local weapon = Actor.getEquipment(self, Actor.EQUIPMENT_SLOT.CarriedRight)
    if weapon and types.Weapon.objectIsInstance(weapon) then
        local record = types.Weapon.record(weapon)
        local T = types.Weapon.TYPE
        if record.type == T.MarksmanBow or record.type == T.MarksmanCrossbow or record.type == T.MarksmanThrown then
            return RANGED_REACH
        end
        return core.getGMST('fCombatDistance') * record.reach
    end
    return core.getGMST('fCombatDistance') -- hand-to-hand
end

local function updateRing()
    local shader = getRingShader()
    if not shader then return end
    if not (hovered and Actor.objectIsInstance(hovered)) then
        hidesRing()
        return
    end

    local box = hovered:getBoundingBox()
    local bodyRadius = math.max(box.halfSize.x, box.halfSize.y)
    local radius = math.max(RING_MIN_RADIUS, bodyRadius * RING_SCALE)
    local hostile = not Actor.isDead(hovered) and Actor.stats.ai.fight(hovered).modified >= HOSTILE_FIGHT
    local color
    if hostile then
        local d = hovered.position - self.position
        local gap = math.sqrt(d.x * d.x + d.y * d.y) - bodyRadius
        color = gap <= weaponReach() and RING_HOSTILE_NEAR or RING_HOSTILE
    else
        color = hoveredInRange and RING_PASSIVE_NEAR or RING_PASSIVE
    end
    local feet = hovered.position
    shader:setVector4('uSelection', util.vector4(feet.x, feet.y, feet.z, radius))
    shader:setVector4('uSelectionColor', util.vector4(color.x, color.y, color.z, 0.85))
end

-- What the game's tooltip says besides the name: where a door leads, and whether it (or a container) is locked or
-- trapped, in the game's own words.
local function gmst(id, fallback)
    local ok, value = pcall(core.getGMST, id)
    return ok and value and value ~= '' and value or fallback
end

local function objectDetails(object)
    local lines = {}
    if types.Door.objectIsInstance(object) and types.Door.isTeleport(object) then
        local cell = types.Door.destCell(object)
        local destination = cell and cell.name or ''
        if destination == '' and cell and cell.region then
            local region = core.regions.records[cell.region]
            destination = region and region.name or ''
        end
        if destination ~= '' then
            lines[#lines + 1] = gmst('sTo', 'to') .. ' ' .. destination
        end
    end
    if types.Lockable.objectIsInstance(object) then
        if types.Lockable.isLocked(object) then
            lines[#lines + 1] = gmst('sLockLevel', 'Lock Level') .. ': ' .. types.Lockable.getLockLevel(object)
        end
        if types.Lockable.getTrapSpell(object) then
            lines[#lines + 1] = gmst('sTrapped', 'Trapped')
        end
    end
    return lines
end

local function updateLabel()
    local text = hovered and objectName(hovered) or ''
    if text ~= '' then
        local ok, details = pcall(objectDetails, hovered)
        if ok then
            for _, line in ipairs(details) do
                text = text .. string.char(10) .. line
            end
        end
    end
    labelElement.layout.props.text = text
    labelElement.layout.props.textColor = hoveredInRange and util.color.rgb(0.87, 0.79, 0.62)
        or util.color.rgb(0.55, 0.5, 0.42)
    local labelAt = cursor
    if gamepad and hovered then
        local box = hovered:getBoundingBox()
        labelAt = worldToScreen(box.center + v3(0, 0, box.halfSize.z)) or cursor
    end
    labelElement.layout.props.relativePosition = labelAt
    cursorElement.layout.props.relativePosition = cursor
end

local function turnTowards(targetYaw, dt)
    local yawDiff = wrapAngle(targetYaw - self.rotation:getYaw())
    local maxTurn = TURN_SPEED * dt
    self.controls.yawChange = util.clamp(yawDiff, -maxTurn, maxTurn)
end

-- WASD is relative to the screen: the world direction it asks for (length 0..1).
local function wantedDirection()
    local up = input.getRangeActionValue('MoveForward') - input.getRangeActionValue('MoveBackward')
    local right = input.getRangeActionValue('MoveRight') - input.getRangeActionValue('MoveLeft')

    local dx = math.sin(viewYaw) * up + math.cos(viewYaw) * right
    local dy = math.cos(viewYaw) * up - math.sin(viewYaw) * right
    local length = math.sqrt(dx * dx + dy * dy)
    if length > 1 then dx, dy = dx / length, dy / length end
    return dx, dy
end

-- With a weapon or spell ready the character always aims at the cursor; the engine (twin-stick movement in
-- character.cpp) turns their legs to run the way they move while the upper body keeps aiming. Otherwise they face
-- the way they walk.
local function updateFacing(dt, dx, dy)
    local aiming = Actor.getStance(self) ~= Actor.STANCE.Nothing and aimPoint
    self.controls.yawChange = 0

    if aiming then
        -- Dead zone: with the cursor right on the character, tiny cursor moves would swing the aim wildly.
        local d = aimPoint - self.position
        if d.x * d.x + d.y * d.y > AIM_DEAD_ZONE * AIM_DEAD_ZONE then
            turnTowards(math.atan2(d.x, d.y), dt)
        end
    elseif dx * dx + dy * dy > 0.01 then
        turnTowards(math.atan2(dx, dy), dt)
    end

    -- When aiming, pitch toward the target so arrows and spells go where the cursor points.
    local pitch = 0
    if aiming then
        local fromChest = aimPoint - (self.position + v3(0, 0, 100))
        local horizontal = math.sqrt(fromChest.x * fromChest.x + fromChest.y * fromChest.y)
        pitch = util.clamp(-math.atan2(fromChest.z, horizontal), math.rad(-60), math.rad(60))
    end
    self.controls.pitchChange = pitch - self.rotation:getPitch()
end

-- Right click with a weapon ready: a power attack, always the weapon's strongest type (chop, slash or thrust by
-- maximum damage). Morrowind has no manual block (shields block on their own). Ranged weapons and hand-to-hand
-- just attack; the engine picks a valid type for them.
local function powerAttackType()
    local weapon = Actor.getEquipment(self, Actor.EQUIPMENT_SLOT.CarriedRight)
    if not weapon or not types.Weapon.objectIsInstance(weapon) then return self.ATTACK_TYPE.Any end
    local record = types.Weapon.record(weapon)
    local best, bestDamage = self.ATTACK_TYPE.Chop, record.chopMaxDamage
    if record.slashMaxDamage > bestDamage then best, bestDamage = self.ATTACK_TYPE.Slash, record.slashMaxDamage end
    if record.thrustMaxDamage > bestDamage then best = self.ATTACK_TYPE.Thrust end
    return best
end

local function updatePowerAttack()
    -- Runs after the built-in controls (scripts/omw/input/playercontrols.lua), which set use from the left button
    -- and release the attack again once neither button is held.
    local leftTrigger = input.getAxisValue(input.CONTROLLER_AXIS.TriggerLeft) >= 0.6
    if Actor.getStance(self) == Actor.STANCE.Weapon and (input.isMouseButtonPressed(3) or leftTrigger) then
        self.controls.use = powerAttackType()
    end
end

-- Convert the wanted world direction into the character's own forward / sideways movement.
local function updateMovement(dx, dy)
    local yaw = self.rotation:getYaw()
    self.controls.movement = dx * math.sin(yaw) + dy * math.cos(yaw)
    self.controls.sideMovement = dx * math.cos(yaw) - dy * math.sin(yaw)
    self.controls.run = input.getBooleanActionValue('Run') ~= controlSettings:get('alwaysRun')
    self.controls.jump = jumpRequested
    if not controlSettings:get('toggleSneak') then
        self.controls.sneak = input.getBooleanActionValue('Sneak')
    end
end

input.registerTriggerHandler('Jump', async:callback(function()
    if active and controlsAllowed() then
        jumpRequested = Player.getControlSwitch(self, Player.CONTROL_SWITCH.Jumping)
    end
end))

input.registerTriggerHandler('ToggleSneak', async:callback(function()
    if active and controlsAllowed() and controlSettings:get('toggleSneak') then
        self.controls.sneak = not self.controls.sneak
    end
end))

-- Screen-relative stick input (up the screen = viewYaw) as a world direction, like WASD (wantedDirection).
local function screenToWorld(up, right)
    return v3(math.sin(viewYaw) * up + math.cos(viewYaw) * right, math.cos(viewYaw) * up - math.sin(viewYaw) * right, 0)
end

-- Which device is in use: any stick or trigger moved, or a button pressed, means the controller; the mouse moving
-- means the mouse.
local function updateDevice()
    if input.getMouseMoveX() ~= 0 or input.getMouseMoveY() ~= 0 then
        gamepad = false
    end
    local A = input.CONTROLLER_AXIS
    for _, axis in ipairs({ A.LeftX, A.LeftY, A.RightX, A.RightY, A.TriggerLeft, A.TriggerRight }) do
        if math.abs(input.getAxisValue(axis)) > STICK_DEAD_ZONE then
            gamepad = true
        end
    end
    stickAim = nil
    if gamepad then
        local right, down = input.getAxisValue(A.RightX), input.getAxisValue(A.RightY)
        if right * right + down * down > STICK_DEAD_ZONE * STICK_DEAD_ZONE then
            stickAim = screenToWorld(-down, right):normalize()
        end
    end
end

local function isHostile(actor)
    return not Actor.isDead(actor) and Actor.stats.ai.fight(actor).modified >= HOSTILE_FIGHT
end

-- The hostile creature nearest the given direction (within maxCos of it) and range, preferring near ones.
local function targetToward(direction, minCos)
    local best, bestScore = nil, math.huge
    for _, actor in ipairs(nearby.actors) do
        if actor ~= self.object and isHostile(actor) then
            local d = actor.position - self.position
            local dist = math.sqrt(d.x * d.x + d.y * d.y)
            if dist < AIM_ASSIST_RANGE then
                local alignment = dist > 1 and v3(d.x, d.y, 0):normalize():dot(direction) or 1
                if alignment >= minCos then
                    local score = dist * (2 - alignment)
                    if score < bestScore then best, bestScore = actor, score end
                end
            end
        end
    end
    return best
end

-- With a controller: what the ring and label are on, what X uses and where a ready weapon or spell aims.
local function updateGamepadPick()
    hovered = nil
    hoveredInRange = false
    aimPoint = nil
    local yaw = self.rotation:getYaw()
    local facing = v3(math.sin(yaw), math.cos(yaw), 0)
    if Actor.getStance(self) ~= Actor.STANCE.Nothing then
        -- Aim lock: the hostile creature the right stick points at, or else the nearest one in front
        local target = targetToward(stickAim or facing, stickAim and STICK_PICK_COS or AIM_ASSIST_COS)
        if target then
            hovered = target
            aimPoint = target:getBoundingBox().center
            local d = target.position - self.position
            hoveredInRange = math.sqrt(d.x * d.x + d.y * d.y) <= core.getGMST('iMaxActivateDist') + ACTIVATE_SLACK
        elseif stickAim then
            aimPoint = self.position + v3(0, 0, 100) + stickAim * 600
        end
    else
        -- What X would use: in the right stick's direction if it points somewhere, else ahead
        hovered = nearestUsable(stickAim)
        hoveredInRange = hovered ~= nil
    end
end

-- Back held opens the journal (a tap opens the inventory, as bound); D-pad up / down held keeps zooming.
local function updateControllerHolds(dt)
    local B = input.CONTROLLER_BUTTON
    -- Real time: menus pause the game, and dt with it
    if input.isControllerButtonPressed(B.Back) then
        backPressedAt = backPressedAt or core.getRealTime()
        if not journalOpened and core.getRealTime() - backPressedAt >= JOURNAL_HOLD then
            journalOpened = true
            I.UI.setMode('Journal')
        end
    else
        backPressedAt, journalOpened = nil, false
    end
    local zoom = (input.isControllerButtonPressed(B.DPadDown) and 1 or 0)
        - (input.isControllerButtonPressed(B.DPadUp) and 1 or 0)
    if zoom ~= 0 then
        dpadHeld = dpadHeld + dt
        if dpadHeld > DPAD_ZOOM_DELAY then
            targetDistance = util.clamp(targetDistance * DPAD_ZOOM_RATE ^ (zoom * dt), MIN_DISTANCE, MAX_DISTANCE)
        end
    else
        dpadHeld = 0
    end
end

-- Which view the zoom asks for
local function wantedView()
    if targetDistance >= TOPDOWN_FROM then return 'topdown' end
    if targetDistance < FIRST_PERSON_BELOW then return 'first' end
    return 'third'
end

-- Zooming and the choice of first or third person are this script's for good, so the built-in camera script
-- doesn't zoom or switch views on its own
local function claimCamera()
    if cameraClaimed then return end
    cameraClaimed = true
    I.Camera.disableZoom(TAG)
    I.Camera.disableModeControl(TAG)
end

local function switchView(wanted)
    viewMode = wanted
    if wanted == 'topdown' then
        distance = targetDistance
        apply()
    else
        release()
        camera.setMode(wanted == 'first' and MODE.FirstPerson or MODE.ThirdPerson)
    end
end

local function onFrame(dt)
    claimCamera()
    updateDevice()
    updateControllerHolds(dt)
    if controlsAllowed() then updateZoom() end

    local wanted = wantedView()
    if wanted ~= viewMode then switchView(wanted) end
    if wanted ~= 'topdown' then
        -- Morrowind's own views: the camera script runs them, at the distance zoomed to
        if wanted == 'third' then camera.setPreferredThirdPersonDistance(targetDistance) end
        return
    end
    apply()

    if not controlsAllowed() then
        -- Menus, dialogue, paused: keep the camera, stop driving the character.
        self.controls.movement = 0
        self.controls.sideMovement = 0
        self.controls.yawChange = 0
        self.controls.pitchChange = 0
        if not core.isWorldPaused() or I.UI.getMode() then updateCamera(dt) end
        setUiVisible(false)
        hidesRing()
        healthbars.clear()
        return
    end

    updateCursor()
    updateCamera(dt)
    updateFadedObjects()
    if gamepad then updateGamepadPick() else updatePick() end
    updateLabel()
    setUiVisible(true)
    setRingVisible(true)
    updateRing()
    healthbars.update(worldToScreen)
    local dx, dy = wantedDirection()
    updateFacing(dt, dx, dy)
    updateMovement(dx, dy)
    updatePowerAttack()
    jumpRequested = false
end

local function activate(object)
    core.sendGlobalEvent('Isowind_Activate', { object = object, actor = self.object })
end

local function activateHovered()
    if not hovered then return false end
    if hoveredInRange then
        activate(hovered)
    else
        ui.showMessage('Too far away.')
    end
    return true
end

-- The nearest usable thing within reach, favouring what is in front of the player (or in the given direction). With a
-- controller and no direction given, anything in reach will do (it can't be pointed at).
nearestUsable = function(direction)
    local reach = core.getGMST('iMaxActivateDist') + ACTIVATE_SLACK
    local yaw = self.rotation:getYaw()
    local facing = direction or v3(math.sin(yaw), math.cos(yaw), 0)
    local best, bestScore = nil, math.huge
    for _, list in ipairs({ nearby.doors, nearby.containers, nearby.items, nearby.actors, nearby.activators }) do
        for _, object in ipairs(list) do
            if object ~= self.object and objectName(object) then
                local d = object.position - self.position
                local dist = d:length()
                if dist <= reach then
                    local ahead = dist > 1 and v3(d.x, d.y, 0):normalize():dot(facing) or 1
                    local allowed
                    if direction then
                        allowed = ahead > STICK_PICK_COS or dist < 80
                    elseif gamepad then
                        allowed = true
                    else
                        allowed = ahead > 0.2 or dist < 80
                    end
                    if allowed then
                        local score = dist * (2 - ahead)
                        if score < bestScore then best, bestScore = object, score end
                    end
                end
            end
        end
    end
    return best
end

local function onMouseButtonPress(button)
    if not (active and button == 1 and controlsAllowed()) then return end
    if Actor.getStance(self) ~= Actor.STANCE.Nothing then return end
    activateHovered()
end

-- Activate (E): what is under the cursor, or else the nearest thing in reach. The engine's own activation looks at
-- the middle of the screen, which in top-down is never within reach, so it does nothing.
local function onInputAction(action)
    if not (active and action == input.ACTION.Activate and controlsAllowed()) then return end
    if gamepad then
        -- X: what is nearest in the right stick's direction (or ahead), whatever the aim is locked on
        local object = nearestUsable(stickAim)
        if object then activate(object) end
        return
    end
    if activateHovered() then return end
    local object = nearestUsable()
    if object then activate(object) end
end

local function onKeyPress(key)
    if key.code == input.KEY.F7 then
        if targetDistance >= TOPDOWN_FROM then
            lastTopDownDistance = targetDistance
            targetDistance = THIRD_PERSON_DISTANCE
            ui.showMessage('Normal view')
        else
            targetDistance = lastTopDownDistance
            ui.showMessage('Top-down view')
        end
    end
end

-- A frame that fails part way must not leave the selection ring or the health bars stuck where they were: hide them
-- and say (once per different error) what went wrong.
local lastFrameError = nil
local function onFrameSafe(dt)
    local ok, err = pcall(onFrame, dt)
    if not ok then
        pcall(hidesRing)
        pcall(healthbars.clear)
        if err ~= lastFrameError then
            lastFrameError = err
            print('top-down frame failed: ' .. tostring(err))
        end
    end
end

return {
    engineHandlers = {
        onFrame = onFrameSafe,
        onMouseButtonPress = onMouseButtonPress,
        onInputAction = onInputAction,
        onKeyPress = onKeyPress,
        onControllerButtonPress = function() gamepad = true end,
        onTeleported = function() camera.instantTransition() end,
        onSave = function()
            return { distance = targetDistance, topDownDistance = lastTopDownDistance }
        end,
        onLoad = function(data)
            if not data then return end
            targetDistance = data.distance or targetDistance
            lastTopDownDistance = data.topDownDistance or lastTopDownDistance
            -- A save from before the zoom chose the view, made in the normal view
            if data.enabled == false and targetDistance >= TOPDOWN_FROM then
                targetDistance = THIRD_PERSON_DISTANCE
            end
            distance = targetDistance
            viewMode = nil
        end,
    },
}
