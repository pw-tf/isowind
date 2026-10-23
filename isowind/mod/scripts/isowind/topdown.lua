-- Isowind top-down mode: an isometric-style camera that follows the player, WASD movement relative to the
-- screen, a mouse cursor the character turns to face, and click-to-interact (Diablo / Project Zomboid style).
--
--   WASD            move (W is always "up the screen")
--   mouse           move the cursor; with a weapon or spell ready the character aims at it while their legs run
--                   the way they move (twin-stick), otherwise they face the way they walk
--   left click      attack / cast with a weapon or spell ready, otherwise interact with what is under the cursor
--   right click     power attack with a weapon ready (the weapon's strongest attack type)
--   E (Activate)    interact with what is under the cursor, or else the nearest thing in reach in front
--                   (a locked door or chest with no key: a go at picking it, with a lockpick carried)
--   Space / Ctrl    jump / sneak; in water, held: swim up / dive (as the Jump and Sneak bindings are)
--   mouse wheel     zoom, within the view (the D-pad on a controller)
--   F7              switch view: top-down, then Morrowind's third person view, then first person (rebindable, and
--                   bindable to a controller button, in Options -> Scripts -> Isowind); with an attack held (an
--                   arrow drawn) the view still switches, and the attack starts again in the new view
--
-- With a controller (it takes over as soon as a stick or button is used, the mouse as soon as it moves):
--   left stick      move                     right stick     aim / pick what X uses
--   A               jump                     B               ready weapon
--   X               use / talk               Y               ready magic
--   right trigger   attack / cast            left trigger    power attack
--   bumpers         previous / next quick key
--   D-pad up / down zoom (hold to keep zooming), within the view; in menus they move through the options
--   D-pad left      not assigned
--   Start           menu                     Back            inventory (hold: journal)
--   left stick in   sneak                    right stick in  rest
--   in water, A held swims up and the left stick held in dives (the Jump and Sneak bindings)
-- There is no cursor then: what the right stick points at (or else what is nearest in front) gets the ring and the
-- label, and X uses it (as E does: a locked door or chest is picked). With a weapon or spell ready the aim locks
-- onto the nearest hostile creature in front (X still uses what is nearest, not that creature); with no hostile one
-- there the ring and label stay on what X uses (friendly people, doors, containers and their locks), but the aim
-- never locks onto those. A lockpick or probe never locks on, and looks for doors and containers first.
-- The menus are the engine's controller menus ('controller menus' in the profile's settings.cfg): D-pad and A / B,
-- the bumpers switch the inventory screen's pages and the triggers the tabs inside a window, Y puts an item or spell
-- on a quick key, and X on a text field (moved onto with the D-pad) brings up a keyboard on the screen to type with;
-- it never comes up by itself. The inventory screen's map, inventory, magic and stats are pages, one at a time under a
-- tab bar, with the mouse and keyboard too: Q / E turn its pages (and Options' tabs), Z / C the tabs inside a window.
--
-- The zoom distance says the view: TOPDOWN_FROM or farther is this top-down view; in closer, Morrowind's third person
-- view at that distance; closer than FIRST_PERSON_BELOW, first person. Zooming stays within the view's range and the
-- switch view trigger (F7) moves between them, each keeping its own zoom. Zooming and the first / third person switch
-- are taken from the built-in camera script for good (claimCamera).
-- The view is a fixed isometric angle: turned 45 degrees, looking down at the classic isometric 35.26 degrees.
-- The camera slides in toward the player rather than go underground on slopes, and creatures (and hurt or hostile
-- people) get health bars (healthbars.lua). In multiplayer, party members (MMO) get a green ring, and other players
-- off the screen an arrow at its edge pointing their way: party members in MMO mode, everyone in co-op.
-- Signs can't be read from up here, so within SIGN_RANGE of the player their names show over them: signposts (each
-- arm), shop and canton signs, and the town banners.

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
local THIRD_PERSON_DISTANCE = 200   -- the third person view's zoom until it is zoomed
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
local SWIM_PITCH = math.rad(85)     -- the steepest dive or climb in water (straight down would lose the way faced)
local SIGN_RANGE = 500              -- signs this close to the player show their names
local SIGN_REFRESH = 0.25           -- how often to look for signs nearby (seconds)
local SIGN_LABELS = 8               -- at most this many sign names at once (the nearest)
local SIGN_TEXT_SIZE = 16
--------------------------------------------------------------------------------------------------------------------

local active = false                -- whether the top-down view's overrides are currently applied
local viewMode = nil                -- 'topdown', 'third' or 'first', from the zoom (wantedView)
local cameraClaimed = false         -- zoom and the first / third person switch are this script's, once claimed
local lastTopDownDistance = 1000    -- the zoom of the views not in, for switching back to them
local lastThirdDistance = THIRD_PERSON_DISTANCE
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
local aimLocked = false             -- hovered is the hostile creature the controller's aim is locked onto
local backPressedAt, journalOpened = nil, false
local dpadHeld = 0                  -- how long D-pad up / down has been held; nil while a hold begun in a menu lasts
local pendingMode = nil             -- the first / third person camera mode asked for, until the camera is in it

-- Defined further down
local nearestUsable, setRingVisible, hideSignLabels

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

-- Whether an attack is being held (an arrow drawn, a swing wound up). Switching to or from first person rebuilds the
-- character's animations, so the camera holds the switch back while the body is busy (camera.cpp: an attack, a hit,
-- being knocked down); with an attack held it is made at once instead, and the attack starts again in the new view
-- as the button is still down.
local function attackHeld()
    return Actor.getStance(self) == Actor.STANCE.Weapon and input.getBooleanActionValue('Use')
        and Actor.canMove(self)
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
    hideSignLabels()
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
        camera.setMode(MODE.Static, attackHeld())
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

-- Where a world position is in the view: x and y run from -1 to 1 across the screen (y up) and beyond off it, and its
-- depth in front of the camera (negative behind it).
local function viewCoordinates(pos)
    local rel = pos - view.position
    local depth = rel:dot(view.forward)
    local screen = ui.screenSize()
    local tanHalf = math.tan(FOV / 2)
    local d = math.max(math.abs(depth), 1)
    return rel:dot(view.right) / (d * tanHalf * screen.x / screen.y), rel:dot(view.up) / (d * tanHalf), depth
end

-- Normalised screen position of a world position, or nil when it is off screen or behind the camera.
local function worldToScreen(pos)
    local x, y, depth = viewCoordinates(pos)
    if depth < 1 or math.abs(x) > 1 or math.abs(y) > 1 then return nil end
    return v2((x + 1) / 2, (1 - y) / 2)
end

local function updateCursor()
    local screen = ui.screenSize()
    cursor = v2(
        util.clamp(cursor.x + input.getMouseMoveX() * CURSOR_SPEED / screen.x, 0, 1),
        util.clamp(cursor.y + input.getMouseMoveY() * CURSOR_SPEED / screen.y, 0, 1))
end

-- Zoom by a factor, within the view: zooming never changes it (first person doesn't zoom)
local function zoomBy(factor)
    if targetDistance >= TOPDOWN_FROM then
        targetDistance = util.clamp(targetDistance * factor, TOPDOWN_FROM, MAX_DISTANCE)
    elseif targetDistance >= FIRST_PERSON_BELOW then
        targetDistance = util.clamp(targetDistance * factor, FIRST_PERSON_BELOW, TOPDOWN_FROM - 1)
    end
end

local function updateZoom()
    -- Zoom3rdPerson is 10 per wheel notch (scripts/omw/input/actionbindings.lua).
    local notches = input.getNumberActionValue('Zoom3rdPerson') / 10
    if notches ~= 0 then zoomBy(math.exp(-notches * ZOOM_STEP)) end
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
    aimLocked = false
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
local MARKS = 8              -- party rings and arrows the shader has room for (its uParty and uArrows)
local ARROW_MARGIN = 40      -- the arrows sit this far in from the screen's edge (pixels at 1080 lines)
local noMarks = {}
for i = 1, MARKS do noMarks[i] = util.vector4(0, 0, 0, 0) end

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
        shader:setVector4Array('uParty', noMarks)
        shader:setVector4Array('uArrows', noMarks)
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

-- Party members (MMO mode) get a green ring like the selection ring, and other players off the screen an arrow at its
-- edge pointing their way: party members in MMO mode, everyone in co-op. Only players in the cells this game has
-- loaded: it knows where no one else is.
local function updateParty()
    local shader = getRingShader()
    if not shader then return end
    local rings, arrows = {}, {}
    if nearby.isAlly then
        local mmo = nearby.isMmo()
        local screen = ui.screenSize()
        local margin = ARROW_MARGIN * screen.y / 1080
        local maxX, maxY = 1 - 2 * margin / screen.x, 1 - 2 * margin / screen.y
        for _, actor in ipairs(nearby.actors) do
            if nearby.isOtherPlayer(actor) then
                local ally = nearby.isAlly(actor)
                local box = actor:getBoundingBox()
                if mmo and ally and #rings < MARKS then
                    local feet = actor.position
                    local radius = math.max(RING_MIN_RADIUS, math.max(box.halfSize.x, box.halfSize.y) * RING_SCALE)
                    rings[#rings + 1] = util.vector4(feet.x, feet.y, feet.z, radius)
                end
                local x, y, depth = viewCoordinates(box.center)
                if (ally or not mmo) and #arrows < MARKS and (depth < 1 or math.abs(x) > 1 or math.abs(y) > 1) then
                    -- On the edge, the way they are from the middle of the screen (the player). Behind the camera too:
                    -- viewCoordinates divides by the depth's size, so x and y keep the way they are from the camera.
                    local scale = math.min(maxX / math.max(math.abs(x), 1e-4), maxY / math.max(math.abs(y), 1e-4))
                    local direction = v2(x * screen.x, y * screen.y):normalize()
                    arrows[#arrows + 1] = util.vector4((x * scale + 1) / 2, (y * scale + 1) / 2, direction.x,
                        direction.y)
                end
            end
        end
    end
    for i = #rings + 1, MARKS do rings[i] = noMarks[i] end
    for i = #arrows + 1, MARKS do arrows[i] = noMarks[i] end
    shader:setVector4Array('uParty', rings)
    shader:setVector4Array('uArrows', arrows)
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

-- Signs: the activators with a sign for a model (in Morrowind.esm: the signposts' arms, Active_signpost_*, shop signs,
-- Active_sign_* and Furn_sign_*, and Vivec's canton signs, Ex_V_sign_*), and the town banners (Act_banner_*). Their
-- record names are what they say.
local SIGN_MODEL_WORD = 'sign'
local BANNER_PREFIX = 'act_banner_'
local signLabels = {}               -- a pool of text elements, made as they are needed
local nearSigns = {}                -- the signs within SIGN_RANGE, nearest first
local signRefreshIn = 0
local isSignRecord = {}             -- record id -> whether it is a sign

local function isSign(object)
    local id = object.recordId
    local known = isSignRecord[id]
    if known == nil then
        known = false
        local ok, record = pcall(types.Activator.record, object)
        if ok and record and record.name and record.name ~= '' then
            local model = string.lower(record.model or '')
            known = model:find(SIGN_MODEL_WORD, 1, true) ~= nil or id:sub(1, #BANNER_PREFIX) == BANNER_PREFIX
        end
        isSignRecord[id] = known
    end
    return known
end

local function refreshNearSigns()
    nearSigns = {}
    local distances = {}
    for _, object in ipairs(nearby.activators) do
        local dist = (object.position - self.position):length()
        if dist <= SIGN_RANGE and object.enabled and isSign(object) then
            nearSigns[#nearSigns + 1] = object
            distances[object.id] = dist
        end
    end
    table.sort(nearSigns, function(a, b) return distances[a.id] < distances[b.id] end)
end

local function signLabel(i)
    if not signLabels[i] then
        signLabels[i] = ui.create {
            layer = OVERLAY,
            type = ui.TYPE.Text,
            props = {
                anchor = v2(0.5, 1),
                text = '',
                textSize = SIGN_TEXT_SIZE,
                textColor = util.color.rgb(0.792, 0.647, 0.376), -- Morrowind's normal text colour
                textShadow = true,
                textShadowColor = util.color.rgb(0, 0, 0),
                visible = false,
            },
        }
    end
    return signLabels[i]
end

hideSignLabels = function()
    signRefreshIn = 0 -- look again when they next show, rather than show the signs found before
    for _, label in ipairs(signLabels) do
        if label.layout.props.visible then
            label.layout.props.visible = false
            label:update()
        end
    end
end

-- Each name just over the top of its sign. A signpost's arms all stand at the post, but each arm's box lies along
-- the way it points; names that would still land on one another are moved up out of the way.
local function updateSignLabels(dt)
    signRefreshIn = signRefreshIn - dt
    if signRefreshIn <= 0 then
        signRefreshIn = SIGN_REFRESH
        refreshNearSigns()
    end
    -- (the layer is only there from the frame after this script made it: ui.layers.insertAfter waits a frame)
    local layer = ui.layers.indexOf(OVERLAY)
    if not layer then return end
    local size = ui.layers[layer].size
    local lineHeight = SIGN_TEXT_SIZE + 2
    local shown = {}
    for _, sign in ipairs(nearSigns) do
        if #shown >= SIGN_LABELS then break end
        -- (the one the cursor or the right stick is on already has the label)
        if sign:isValid() and sign.enabled and sign ~= hovered then
            local box = sign:getBoundingBox()
            local at = worldToScreen(box.center + v3(0, 0, box.halfSize.z))
            local text = objectName(sign)
            if at and text then
                shown[#shown + 1] = { text = text, x = at.x * size.x, y = at.y * size.y,
                    halfWidth = #text * SIGN_TEXT_SIZE * 0.28 }
            end
        end
    end
    -- Lowest on the screen first, each later one moved up above any it would overlap
    table.sort(shown, function(a, b) return a.y > b.y end)
    for i, s in ipairs(shown) do
        local moved = true
        while moved do
            moved = false
            for j = 1, i - 1 do
                local p = shown[j]
                if math.abs(s.x - p.x) < s.halfWidth + p.halfWidth and math.abs(s.y - p.y) < lineHeight then
                    s.y = p.y - lineHeight
                    moved = true
                end
            end
        end
        local label = signLabel(i)
        label.layout.props.text = s.text
        label.layout.props.relativePosition = v2(s.x / size.x, s.y / size.y)
        label.layout.props.visible = true
        label:update()
    end
    for i = #shown + 1, #signLabels do
        local label = signLabels[i]
        if label.layout.props.visible then
            label.layout.props.visible = false
            label:update()
        end
    end
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

-- In water, Jump held (Space, A) swims up and Sneak held (Ctrl, the left stick pressed in) dives: 1, -1 or 0.
local function swimRise()
    if not Actor.isSwimming(self) then return 0 end
    local up = input.isActionPressed(input.ACTION.Jump)
    local down = input.isActionPressed(input.ACTION.Sneak)
    if up == down then return 0 end
    return up and 1 or -1
end

-- Convert the wanted world direction into the character's own forward / sideways movement. The engine swims the way
-- the character faces, pitch and all, so swimming up or down tilts them and swims them forward (or back, when backing
-- away), the steeper the less they move across; the facing (updateFacing) otherwise keeps them level.
local function updateMovement(dx, dy)
    local yaw = self.rotation:getYaw()
    local forward = dx * math.sin(yaw) + dy * math.cos(yaw)
    local side = dx * math.cos(yaw) - dy * math.sin(yaw)
    local rise = swimRise()
    if rise ~= 0 then
        local across = math.abs(forward)
        local pitch = util.clamp(math.atan2(forward >= 0 and -rise or rise, across), -SWIM_PITCH, SWIM_PITCH)
        forward = (forward >= 0 and 1 or -1) * math.sqrt(across * across + rise * rise)
        local length = math.sqrt(forward * forward + side * side)
        if length > 1 then forward, side = forward / length, side / length end
        self.controls.pitchChange = pitch - self.rotation:getPitch()
    end
    self.controls.movement = forward
    self.controls.sideMovement = side
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
    -- (in water Sneak is held to dive: swimRise)
    if active and controlsAllowed() and controlSettings:get('toggleSneak') and not Actor.isSwimming(self) then
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

-- A lockpick or probe ready (they are 'weapons' too): it is for locks and traps, not for aiming at anyone.
local function lockToolReady()
    if Actor.getStance(self) ~= Actor.STANCE.Weapon then return false end
    local tool = Actor.getEquipment(self, Actor.EQUIPMENT_SLOT.CarriedRight)
    return tool ~= nil and (types.Lockpick.objectIsInstance(tool) or types.Probe.objectIsInstance(tool))
end

-- With a controller: what the ring and label are on, what X uses and where a ready weapon or spell aims.
local function updateGamepadPick()
    hovered = nil
    hoveredInRange = false
    aimPoint = nil
    aimLocked = false
    local yaw = self.rotation:getYaw()
    local facing = v3(math.sin(yaw), math.cos(yaw), 0)
    local lockTool = lockToolReady()
    if Actor.getStance(self) ~= Actor.STANCE.Nothing then
        -- Aim lock: the hostile creature the right stick points at, or else the nearest one in front
        local target = not lockTool
            and targetToward(stickAim or facing, stickAim and STICK_PICK_COS or AIM_ASSIST_COS)
        if target then
            hovered = target
            aimLocked = true
            aimPoint = target:getBoundingBox().center
            local d = target.position - self.position
            hoveredInRange = math.sqrt(d.x * d.x + d.y * d.y) <= core.getGMST('iMaxActivateDist') + ACTIVATE_SLACK
            return
        end
        if stickAim then
            aimPoint = self.position + v3(0, 0, 100) + stickAim * 600
        end
    end
    -- What X would use: in the right stick's direction if it points somewhere, else ahead. With a weapon or spell
    -- ready and no hostile creature to aim at, the same (the ring and label show, the aim doesn't lock onto it).
    hovered = lockTool and nearestUsable(stickAim, true) or nearestUsable(stickAim)
    hoveredInRange = hovered ~= nil
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
    -- The D-pad zooms only in the game itself: in menus and dialogue it moves through the options, and a hold begun
    -- there doesn't zoom once the menu closes, not until it is let go.
    local up, down = input.isControllerButtonPressed(B.DPadUp), input.isControllerButtonPressed(B.DPadDown)
    local zoom = (down and 1 or 0) - (up and 1 or 0)
    if not (up or down) then
        dpadHeld = 0
    elseif not controlsAllowed() then
        dpadHeld = nil
    elseif dpadHeld and zoom ~= 0 then
        dpadHeld = dpadHeld + dt
        if dpadHeld > DPAD_ZOOM_DELAY then
            zoomBy(DPAD_ZOOM_RATE ^ (zoom * dt))
        end
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

-- First or third person. Asked for every time, even when the camera is already in that mode: that also drops a
-- switch the other way still held back (attackHeld) from an earlier zoom.
local function requestMode(mode)
    pendingMode = mode
    camera.setMode(mode, attackHeld())
end

-- A switch held back is asked for again each frame until it has happened, so it still lands if something else (the
-- built-in standing preview) switches the camera in between; asking again changes nothing else. Third person counts
-- as reached in any mode but first person and top-down, so the standing preview (its own mode) is left alone.
local function updatePendingMode()
    if not pendingMode then return end
    local mode = camera.getMode()
    local reached
    if pendingMode == MODE.FirstPerson then
        reached = mode == MODE.FirstPerson
    else
        reached = mode ~= MODE.FirstPerson and mode ~= MODE.Static
    end
    if reached and camera.getQueuedMode() == nil then
        pendingMode = nil
    else
        camera.setMode(pendingMode, attackHeld())
    end
end

local function switchView(wanted)
    viewMode = wanted
    if wanted == 'topdown' then
        pendingMode = nil
        distance = targetDistance
        apply()
    else
        release()
        requestMode(wanted == 'first' and MODE.FirstPerson or MODE.ThirdPerson)
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
        -- Morrowind's own views: the camera script runs them, at the distance zoomed to. While a switch to first
        -- person is held back the camera stays in close (the built-in script would put it back out).
        updatePendingMode()
        camera.setPreferredThirdPersonDistance(math.max(targetDistance, FIRST_PERSON_BELOW))
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
        hideSignLabels()
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
    updateParty()
    healthbars.update(worldToScreen)
    local dx, dy = wantedDirection()
    updateFacing(dt, dx, dy)
    updateMovement(dx, dy)
    updatePowerAttack()
    jumpRequested = false
    updateSignLabels(dt) -- last: should it fail, the character still moves
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
-- controller and no direction given, anything in reach will do (it can't be pointed at). locksOnly: only doors and
-- containers.
nearestUsable = function(direction, locksOnly)
    local reach = core.getGMST('iMaxActivateDist') + ACTIVATE_SLACK
    local yaw = self.rotation:getYaw()
    local facing = direction or v3(math.sin(yaw), math.cos(yaw), 0)
    local best, bestScore = nil, math.huge
    local lists = locksOnly and { nearby.doors, nearby.containers }
        or { nearby.doors, nearby.containers, nearby.items, nearby.actors, nearby.activators }
    for _, list in ipairs(lists) do
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
        -- X: what the label is on (with a lockpick ready, the lock it shows), or else what is nearest in the right
        -- stick's direction (or ahead); not the hostile creature the aim is locked on
        local object = hovered
        if not (object and hoveredInRange and not aimLocked and object:isValid()) then
            object = nearestUsable(stickAim)
        end
        if object then activate(object) end
        return
    end
    if activateHovered() then return end
    local object = nearestUsable()
    if object then activate(object) end
end

-- Switch view (the trigger is registered in settings.lua): top-down, then third person, then first person, each
-- view back at the zoom it was left at
input.registerTriggerHandler('IsowindSwitchView', async:callback(function()
    if not controlsAllowed() then return end
    local current = wantedView()
    if current == 'topdown' then
        lastTopDownDistance = targetDistance
        targetDistance = lastThirdDistance
        ui.showMessage('Third person view')
    elseif current == 'third' then
        lastThirdDistance = targetDistance
        targetDistance = MIN_DISTANCE
        ui.showMessage('First person view')
    else
        targetDistance = lastTopDownDistance
        ui.showMessage('Top-down view')
    end
end))

-- F7 switches the view until the player binds it to something else (Options -> Scripts -> Isowind)
do
    local bindings = storage.playerSection('OMWInputBindings')
    if not ownSettings:get('SwitchViewBound') then
        ownSettings:set('SwitchViewBound', true)
        if bindings:get('IsowindSwitchView') == nil then
            bindings:set('IsowindSwitchView',
                { device = 'keyboard', button = input.KEY.F7, type = 'trigger', key = 'IsowindSwitchView' })
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
        pcall(hideSignLabels)
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
        onControllerButtonPress = function() gamepad = true end,
        onTeleported = function()
            camera.instantTransition()
            signRefreshIn = 0 -- the signs found before are in another cell now
        end,
        onSave = function()
            return {
                distance = targetDistance,
                topDownDistance = lastTopDownDistance,
                thirdDistance = lastThirdDistance,
            }
        end,
        onLoad = function(data)
            if not data then return end
            targetDistance = data.distance or targetDistance
            lastTopDownDistance = data.topDownDistance or lastTopDownDistance
            lastThirdDistance = data.thirdDistance or lastThirdDistance
            -- A save from before the zoom chose the view, made in the normal view
            if data.enabled == false and targetDistance >= TOPDOWN_FROM then
                targetDistance = THIRD_PERSON_DISTANCE
            end
            distance = targetDistance
            viewMode = nil
        end,
    },
}
