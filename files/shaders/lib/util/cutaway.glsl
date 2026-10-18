#ifndef LIB_UTIL_CUTAWAY
#define LIB_UTIL_CUTAWAY

// Isowind's top-down camera: what stands between the camera and the player stays recognisable instead of being cut
// into:
//  - objects in the way (a house, a wall, a bridge, a tree) fade, each on its own (occluderFade, set per object by
//    MWRender::OcclusionFader), and come back when they no longer are in the way: right around the player they open
//    completely, so the player shows clearly through them, around that they turn see-through, more so the nearer
//    the player, and farther out they stay whole. The fade is real transparency (cutawayAlpha, which the object
//    shader applies; OcclusionFader blends faded objects and keeps them out of the depth buffer), so post-processing
//    that reads depth (ambient occlusion, fog) sees the scene behind them rather than a dither;
//  - indoors (cutawayOverheadRadius, set by RenderingManager::update; nothing fades there) the room is seen like a
//    dollhouse, cut with clean edges: what stands between the player and the camera ends at waist height, and
//    everything above door height (roof, rafters, ceilings, upper floors, people upstairs) is removed;
//  - wherever something still covers the player (a hill, a faded ghost), it shows their silhouette, outlined, so
//    they are never lost.
// Nothing at or below the player's feet is faded, so floors and the ground they stand on stay solid even when they
// belong to an object that fades. Characters, creatures and doors are never faded (noCutaway; only the storey above
// goes), nor is water. Only
// active with a narrow field of view and a camera far from the player, which is how the top-down camera is set up,
// so normal views are unaffected.

uniform vec3 playerPos;
uniform mat4 osg_ViewMatrix;
uniform mat4 osg_ViewMatrixInverse;
uniform mat4 projectionMatrix;
// 0: cut away as described; 0.75 (characters, creatures, doors): never faded, only cut above the storey indoors;
// 1 (water and its reflections): never cut at all
uniform float noCutaway;
uniform float occluderFade;
uniform float cutawayOverheadRadius;

// The player mask (PlayerMask in apps/openmw/mwrender/playermask.cpp): the player rendered alone, red = their view
// depth, alpha = coverage. screenRes is declared by the including shader.
uniform sampler2D playerMaskTex;

// How opaque this fragment of a faded object stays (set by applyCutaway, applied by the object shader).
float cutawayAlpha = 1.0;

// Nothing lower than this above the player's feet is faded.
#define CUTAWAY_FLOOR_CLEARANCE 25.0
// How see-through a faded object gets just outside the keyhole, and how many keyhole radii out that fades back to
// whole; behind the player it is always whole, so a big tree or house only clears around the player.
#define CUTAWAY_GHOST 0.65
// How see-through the keyhole itself is: not quite clear, so what is under a roof blends into the fade around it
#define CUTAWAY_KEYHOLE_CLEAR 0.9
#define CUTAWAY_WINDOW_OUTER 3.0
// How far toward the camera (on the ground, world units) from the middle of the player something must be to count as
// in front of them.
#define CUTAWAY_FRONT_MARGIN 10.0
// Indoors, how high above the player's feet everything ends: above the doors, below the rafters and the roof.
#define CUTAWAY_STOREY_HEIGHT 230.0
// Indoors, how high above the player's feet what stands between them and the camera ends.
#define CUTAWAY_WALL_HEIGHT 110.0
// The circle around the player in which a faded object opens completely: centred on the middle of their body, as big
// as a margin of this much (world units at the player's distance; they are about 125 units tall) around the line from
// their feet to their head would be, times CUTAWAY_KEYHOLE_SCALE.
#define CUTAWAY_KEYHOLE_RADIUS 70.0
#define CUTAWAY_KEYHOLE_SCALE 1.2
// The silhouette's colour, and how strongly its inside and its outline show.
#define CUTAWAY_SILHOUETTE_COLOUR vec3(0.95, 0.78, 0.38)
#define CUTAWAY_SILHOUETTE_FILL 0.3
#define CUTAWAY_SILHOUETTE_EDGE 0.85
// The player mask's resolution (PlayerMask's sMaskSize)
#define CUTAWAY_MASK_SIZE 512.0

// 4x4 ordered dither, 0..15/16.
float cutawayBayer2(vec2 a)
{
    a = floor(a);
    return fract(dot(a, vec2(0.5, a.y * 0.75)));
}

float cutawayBayer4(vec2 a)
{
    return cutawayBayer2(0.5 * a) * 0.25 + cutawayBayer2(a);
}

// Whether the player is on screen at uv, behind something at fragDepth.
float cutawayCoversPlayer(vec2 uv, float fragDepth)
{
    vec4 mask = texture2D(playerMaskTex, uv);
    // Linear filtering blends the player with the empty (0, 0) background: divide out the coverage.
    return (mask.a > 0.5 && fragDepth < mask.r / mask.a - 20.0) ? 1.0 : 0.0;
}

// Whether this view is the top-down camera; feet and head are the player's, in view space.
bool cutawayTopDown(out vec3 feet, out vec3 head)
{
    feet = (osg_ViewMatrix * vec4(playerPos + vec3(0.0, 0.0, 10.0), 1.0)).xyz;
    head = (osg_ViewMatrix * vec4(playerPos + vec3(0.0, 0.0, 125.0), 1.0)).xyz;
    // projectionMatrix[1][1] is 1 / tan(fovY / 2); above 2.7 means a field of view narrower than about 40 degrees.
    // The top-down camera can slide in close (to stay above the ground), so the distance only rules out first person.
    return noCutaway < 0.9 && projectionMatrix[1][1] >= 2.7 && -0.5 * (feet.z + head.z) >= 150.0;
}

// Fades this fragment as described above (discarding it), and returns how strongly the player's silhouette shows
// on it: x = its inside, y = its outline (pass to applyCutawaySilhouette once the colour is known).
// terrain: the ground can't fade, it only shows the silhouette.
vec2 applyCutaway(vec3 viewPos, vec3 viewNormal, bool terrain)
{
    vec3 feet;
    vec3 head;
    if (!cutawayTopDown(feet, head))
        return vec2(0.0);

    float fragDepth = -viewPos.z;
    vec3 world = (osg_ViewMatrixInverse * vec4(viewPos, 1.0)).xyz;

    // Characters, creatures and doors: only gone with the storey above the player indoors (people upstairs)
    if (noCutaway > 0.5)
    {
        if (cutawayOverheadRadius > 1.0)
        {
            float above = step(CUTAWAY_STOREY_HEIGHT, world.z - playerPos.z);
            float around = 1.0 - smoothstep(cutawayOverheadRadius * 0.8, cutawayOverheadRadius,
                length(world.xy - playerPos.xy));
            if (above * around > 0.0 && cutawayBayer4(gl_FragCoord.xy) < above * around)
                discard;
        }
        return vec2(0.0);
    }

    if (!terrain && world.z >= playerPos.z + CUTAWAY_FLOOR_CLEARANCE)
    {
        // In front of the player means between them and the camera on the ground, not only nearer the camera: seen
        // from above, the top of a wall at the player's back is nearer than they are, and must stay
        vec3 cameraForward = -(osg_ViewMatrixInverse * vec4(0.0, 0.0, 1.0, 0.0)).xyz;
        vec2 towardCamera = -normalize(cameraForward.xy + vec2(1e-5));
        bool inFront = dot(world.xy - playerPos.xy, towardCamera) > CUTAWAY_FRONT_MARGIN;

        // Level surfaces below the player's waist (swamp scum on a pool they wade in, rugs, tabletops, lower steps)
        // never hide them from above, so a faded object stays whole there
        vec3 worldNormal = (osg_ViewMatrixInverse * vec4(viewNormal, 0.0)).xyz;
        bool lowFloor = abs(worldNormal.z) > 0.7 && world.z < playerPos.z + CUTAWAY_WALL_HEIGHT;

        float cut = 0.0;
        if (occluderFade > 0.0 && !lowFloor)
        {
            // Open in a circle around the player on screen (measured at their distance) and see-through around it,
            // where this fragment is in front of them
            float transparency = 0.0;
            float playerDepth = -0.5 * (feet.z + head.z);
            if (fragDepth < playerDepth && inFront)
            {
                vec2 onScreen = viewPos.xy * (playerDepth / max(fragDepth, 1.0));
                vec2 feetOnScreen = feet.xy * (playerDepth / -feet.z);
                vec2 headOnScreen = head.xy * (playerDepth / -head.z);
                // The area of the margin around their outline, as a circle
                float halfLength = 0.5 * length(headOnScreen - feetOnScreen);
                float radius = CUTAWAY_KEYHOLE_SCALE
                    * sqrt(CUTAWAY_KEYHOLE_RADIUS * (CUTAWAY_KEYHOLE_RADIUS + halfLength * 4.0 / 3.14159));
                float dist = length(onScreen - 0.5 * (feetOnScreen + headOnScreen));
                float keyhole = CUTAWAY_KEYHOLE_CLEAR * (1.0 - smoothstep(radius * 0.5, radius, dist));
                float ghost = 1.0 - smoothstep(radius, radius * CUTAWAY_WINDOW_OUTER, dist);
                transparency = occluderFade * max(keyhole, CUTAWAY_GHOST * ghost);
            }
            cutawayAlpha = 1.0 - transparency;
            if (cutawayAlpha < 0.02)
                discard;
        }

        // Indoors: walls (and anything else tall) between the player and the camera end at waist height across the
        // room, like a dollhouse, so the room and whoever is in it can be seen; outdoors only the window above opens
        if (cutawayOverheadRadius > 1.0 && inFront)
        {
            float around = 1.0 - smoothstep(cutawayOverheadRadius * 0.8, cutawayOverheadRadius,
                length(world.xy - playerPos.xy));
            cut = max(cut, around * step(CUTAWAY_WALL_HEIGHT, world.z - playerPos.z));
        }

        // Indoors: ceilings and upper floors above the player's head (surfaces facing up or down), and everything
        // higher than door height above their feet (the roof, rafters and beams), so walls end there and nothing
        // overhead clutters the room
        if (cutawayOverheadRadius > 1.0 && world.z > playerPos.z + 200.0)
        {
            float around = 1.0 - smoothstep(cutawayOverheadRadius * 0.8, cutawayOverheadRadius,
                length(world.xy - playerPos.xy));
            if (abs(worldNormal.z) > 0.5)
                cut = max(cut, around);
            cut = max(cut, around * step(CUTAWAY_STOREY_HEIGHT, world.z - playerPos.z));
        }

        if (cut > 0.0 && cutawayBayer4(gl_FragCoord.xy) < cut)
            discard;
    }

    // The silhouette, where this fragment is in front of the player on screen
    vec2 uv = gl_FragCoord.xy / screenRes;
    float inside = cutawayCoversPlayer(uv, fragDepth);
    if (inside == 0.0)
        return vec2(0.0);
    vec2 texel = vec2(1.5 / CUTAWAY_MASK_SIZE);
    float around = 0.25 * (cutawayCoversPlayer(uv + vec2(texel.x, 0.0), fragDepth)
        + cutawayCoversPlayer(uv - vec2(texel.x, 0.0), fragDepth)
        + cutawayCoversPlayer(uv + vec2(0.0, texel.y), fragDepth)
        + cutawayCoversPlayer(uv - vec2(0.0, texel.y), fragDepth));
    return vec2(inside, inside * (1.0 - around));
}

vec3 applyCutawaySilhouette(vec3 colour, vec2 silhouette)
{
    float amount = max(silhouette.x * CUTAWAY_SILHOUETTE_FILL, silhouette.y * CUTAWAY_SILHOUETTE_EDGE);
    return mix(colour, CUTAWAY_SILHOUETTE_COLOUR, amount);
}

#endif
