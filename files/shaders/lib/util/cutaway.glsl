#ifndef LIB_UTIL_CUTAWAY
#define LIB_UTIL_CUTAWAY

// Isowind's top-down camera: what is removed so the player can be seen. The rules are MWRender::Cutaway's
// (apps/openmw/mwrender/cutaway.cpp, whose isCut() answers them the same way for the cursor), and all the numbers come
// from there:
//  - the porthole (cutawaySightAmounts): where more than half the player is hidden, what lies on the view rays in
//    front of their upper body goes, with a ragged edge; nothing behind the player or below their waist is touched.
//    It is a clear opening in a see-through ghost of what is around it (MWRender::GhostLayer: this shader draws those
//    surfaces in a pass of their own, cutawayGhostPass, and the main view leaves them out);
//  - indoors, the other floors: whatever stands wholly above or below the player's floor (walls, upper floors,
//    furniture, doors, people) dissolves away as a whole (cutawayFade, set on each such object by Cutaway, which then
//    leaves it out of the view altogether).
// Characters, creatures and doors are never in the porthole (noCutaway 0.75), water is never cut (noCutaway 1), and
// the ground is never cut: where it, or anything else, still covers the player, their outline shows on it.
// cutawayEnabled is only set in the main view, so shadows, reflections and the map are whole.

uniform mat4 osg_ViewMatrixInverse;
uniform float cutawayEnabled;
uniform vec3 cutawayEye;
// The player's upper body, as a segment
uniform vec3 cutawayCapA;
uniform vec3 cutawayCapB;
// x: the porthole's radius, y: its soft band, z: how open it is (0..1)
uniform vec4 cutawaySight;
// x: the player's floor (z), y: the waist's height over it
uniform vec4 cutawayLevel;
// xy: away from the camera along the ground, w: debug view
uniform vec4 cutawayAway;
// x: how far out the ghost reaches (as the porthole's radius), y: how opaque it is next to the opening, z: it is on
uniform vec4 cutawayGhost;
// 1 in the ghost's own pass (GhostLayer), which draws only what the main view leaves out for it
uniform float cutawayGhostPass;
// How far an object on another floor has dissolved away (0..1)
uniform float cutawayFade;
// 0: cut as described; 0.75 (characters, creatures, doors, particles): never in the porthole; 1 (water and its
// reflections): never
uniform float noCutaway;

// The player mask (PlayerMask in apps/openmw/mwrender/playermask.cpp): the player rendered alone, red = their view
// depth, alpha = coverage. screenRes is declared by the including shader.
uniform sampler2D playerMaskTex;
// Blue noise thresholds (1..254 / 255), tiled over the screen
uniform sampler2D cutawayNoise;

// The silhouette's colour, and how strongly its inside and its outline show.
#define CUTAWAY_SILHOUETTE_COLOUR vec3(0.95, 0.78, 0.38)
#define CUTAWAY_SILHOUETTE_FILL 0.3
#define CUTAWAY_SILHOUETTE_EDGE 0.85
// The player mask's resolution (PlayerMask's sMaskSize)
#define CUTAWAY_MASK_SIZE 512.0
// The noise tile's size (Cutaway's sNoiseSize)
#define CUTAWAY_NOISE_SIZE 64.0
// The porthole's ragged edge: this far in and out (world units), in bumps this many pixels apart
#define CUTAWAY_EDGE_NOISE 8.0
#define CUTAWAY_EDGE_SCALE 48.0
// How close to the body and how softly the porthole stops in front of it, and how softly at the waist
// (Cutaway's sBodyNear, sBodySoft, sWaistSoft)
#define CUTAWAY_BODY_NEAR 15.0
#define CUTAWAY_BODY_SOFT 35.0
#define CUTAWAY_WAIST_SOFT 25.0
// ...nor what is behind the player along the ground, past this far (fading over this much): the top of an arch or a
// pole behind them is nearer the camera than they are (Cutaway's sBehindFrom, sBehindSoft)
#define CUTAWAY_BEHIND_FROM 20.0
#define CUTAWAY_BEHIND_SOFT 40.0
// The ghost: its pass draws what is at least this far into it, and the main view leaves out only what is this far,
// so at its outer edge both draw the same surface for a little way, and no gap opens between them
#define CUTAWAY_GHOST_MIN 0.004
#define CUTAWAY_GHOST_MAIN 0.03

float cutawayHash(vec2 p)
{
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

// Smooth noise in -1..1 over the screen: the same for every surface on a pixel
float cutawayEdgeNoise()
{
    vec2 p = gl_FragCoord.xy / CUTAWAY_EDGE_SCALE;
    vec2 i = floor(p);
    vec2 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    float n = mix(mix(cutawayHash(i), cutawayHash(i + vec2(1.0, 0.0)), f.x),
        mix(cutawayHash(i + vec2(0.0, 1.0)), cutawayHash(i + vec2(1.0, 1.0)), f.x), f.y);
    return n * 2.0 - 1.0;
}

// How opaque the ghost pass makes what it draws (set by applyCutaway, applied by applyCutawayAlpha)
float cutawayLayerAlpha = 1.0;

// How much the porthole removes of a point (x, 0..1), and how far it is in the ghost (y, 0..1). Cutaway::sightAt.
vec2 cutawaySightAmounts(vec3 world)
{
    vec3 ray = world - cutawayEye;
    float len = length(ray);
    ray /= len;
    vec3 axis = cutawayCapB - cutawayCapA;
    vec3 fromBody = cutawayEye - cutawayCapA;
    float along = dot(ray, axis);
    float denom = max(dot(axis, axis) - along * along, 1e-4);
    float s = clamp((dot(axis, fromBody) - along * dot(ray, fromBody)) / denom, 0.0, 1.0);
    vec3 nearest = cutawayCapA + axis * s;
    float atBody = dot(nearest - cutawayEye, ray);
    float dist = length(cutawayEye + ray * atBody - nearest) + cutawayEdgeNoise() * CUTAWAY_EDGE_NOISE;
    float footprint = 1.0 - smoothstep(cutawaySight.x, cutawaySight.x + cutawaySight.y, dist);
    float ghost = 1.0 - smoothstep(cutawaySight.x, cutawayGhost.x, dist);
    float inFront = 1.0 - smoothstep(atBody - CUTAWAY_BODY_NEAR - CUTAWAY_BODY_SOFT, atBody - CUTAWAY_BODY_NEAR, len);
    float high = smoothstep(cutawayLevel.y - CUTAWAY_WAIST_SOFT, cutawayLevel.y + CUTAWAY_WAIST_SOFT,
        world.z - cutawayLevel.x);
    float ahead = -dot(world.xy - cutawayCapA.xy, cutawayAway.xy);
    float notBehind = smoothstep(-CUTAWAY_BEHIND_FROM - CUTAWAY_BEHIND_SOFT, -CUTAWAY_BEHIND_FROM, ahead);
    float amount = inFront * high * notBehind * cutawaySight.z;
    return vec2(footprint, ghost) * amount;
}

// Whether the player is on screen at uv, behind something at fragDepth.
float cutawayCoversPlayer(vec2 uv, float fragDepth)
{
    vec4 mask = texture2D(playerMaskTex, uv);
    // Linear filtering blends the player with the empty (0, 0) background: divide out the coverage.
    return (mask.a > 0.5 && fragDepth < mask.r / mask.a - 20.0) ? 1.0 : 0.0;
}

// Cuts this fragment away (discarding it) if the rules say so. Returns what applyCutawayColour needs once the colour
// is known: x = the player's silhouette inside, y = its outline, z = unused, w = the debug tint (2: the porthole,
// 3: another floor dissolving away). terrain: the ground is never cut. Call it before anything else in main().
vec4 applyCutaway(vec3 viewPos, bool terrain)
{
    if (cutawayEnabled < 0.5 || noCutaway > 0.9)
        return vec4(0.0);

    float threshold = texture2D(cutawayNoise, gl_FragCoord.xy / CUTAWAY_NOISE_SIZE).r;
    float debugTint = 0.0;

    // Another floor, dissolving away as a whole
    if (cutawayFade > 0.0 && threshold < cutawayFade)
    {
        if (cutawayAway.w < 0.5 || cutawayGhostPass > 0.5)
            discard;
        debugTint = 3.0;
    }

    vec3 world = (osg_ViewMatrixInverse * vec4(viewPos, 1.0)).xyz;
    bool actor = noCutaway > 0.5;
    bool cut = false;
    bool ghost = false;
    if (!terrain && !actor && cutawaySight.z > 0.0)
    {
        vec2 sight = cutawaySightAmounts(world);
        if (cutawayGhost.z > 0.5)
        {
            // What is in the ghost's reach is drawn by the ghost's pass alone, see-through, and clear in the opening
            ghost = sight.y > CUTAWAY_GHOST_MIN;
            if (sight.y > CUTAWAY_GHOST_MAIN && cutawayGhostPass < 0.5)
                cut = true;
            cutawayLayerAlpha = (1.0 - sight.x) * mix(1.0, cutawayGhost.y, sight.y);
        }
        else if (threshold >= 1.0 - sight.x)
            cut = true; // (the debug view: what the porthole takes, tinted instead)
    }
    if (cutawayGhostPass > 0.5)
    {
        // The ghost's pass: nothing else, and nothing too faint to see (it would hide what is behind it)
        // (Debug view 4: what this pass draws, in magenta, what is clear in the opening, cyan, and the rest, dark blue)
        if (cutawayAway.w > 3.5 && ghost && cutawayLayerAlpha < CUTAWAY_GHOST_MIN)
        {
            cutawayLayerAlpha = 1.0;
            return vec4(0.0, 0.0, 0.0, 6.0);
        }
        if (cutawayAway.w > 3.5 && !ghost)
        {
            cutawayLayerAlpha = 1.0;
            return vec4(0.0, 0.0, 0.0, 7.0);
        }
        if (!ghost || cutawayLayerAlpha < CUTAWAY_GHOST_MIN)
            discard;
        return vec4(0.0, 0.0, 0.0, cutawayAway.w > 3.5 ? 5.0 : 0.0);
    }

    if (cut)
    {
        if (cutawayAway.w < 0.5 || cutawayAway.w > 3.5)
            discard;
        debugTint = 2.0;
    }

    // The silhouette, where this fragment is in front of the player on screen
    float fragDepth = -viewPos.z;
    vec2 uv = gl_FragCoord.xy / screenRes;
    float covers = cutawayCoversPlayer(uv, fragDepth);
    if (covers == 0.0)
        return vec4(0.0, 0.0, 0.0, debugTint);
    vec2 texel = vec2(1.5 / CUTAWAY_MASK_SIZE);
    float around = 0.25 * (cutawayCoversPlayer(uv + vec2(texel.x, 0.0), fragDepth)
        + cutawayCoversPlayer(uv - vec2(texel.x, 0.0), fragDepth)
        + cutawayCoversPlayer(uv + vec2(0.0, texel.y), fragDepth)
        + cutawayCoversPlayer(uv - vec2(0.0, texel.y), fragDepth));
    return vec4(covers, covers * (1.0 - around), 0.0, debugTint);
}

// The ghost's pass draws what it keeps this opaque; anywhere else alpha is what it was. A blended surface keeps its
// own alpha in the ghost too (blended: the clear parts of leaves, black in Morrowind's own textures, would turn solid)
float applyCutawayAlpha(float alpha, bool blended)
{
    if (cutawayGhostPass < 0.5)
        return alpha;
    return blended ? cutawayLayerAlpha * clamp(alpha, 0.0, 1.0) : cutawayLayerAlpha;
}

vec3 applyCutawayColour(vec3 colour, vec4 cut)
{
    float amount = max(cut.x * CUTAWAY_SILHOUETTE_FILL, cut.y * CUTAWAY_SILHOUETTE_EDGE);
    colour = mix(colour, CUTAWAY_SILHOUETTE_COLOUR, amount);
    if (cut.w > 6.5)
        colour = vec3(0.0, 0.0, 0.5);
    else if (cut.w > 5.5)
        colour = vec3(0.0, 1.0, 1.0);
    else if (cut.w > 4.5)
        colour = vec3(1.0, 0.0, 1.0);
    else if (cut.w > 2.5)
        colour = mix(colour, vec3(1.0, 0.6, 0.1), 0.6);
    else if (cut.w > 1.5)
        colour = mix(colour, vec3(0.1, 0.4, 1.0), 0.6);
    return colour;
}

#endif
