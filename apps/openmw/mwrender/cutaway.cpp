#include "cutaway.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <random>

#include <osg/Camera>
#include <osg/ComputeBoundsVisitor>
#include <osg/Group>
#include <osg/Texture2D>
#include <osg/UserDataContainer>
#include <osgUtil/CullVisitor>
#include <osgUtil/IntersectionVisitor>
#include <osgUtil/LineSegmentIntersector>

#include <components/sceneutil/nodecallback.hpp>
#include <components/sceneutil/positionattitudetransform.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwworld/class.hpp"

#include "objects.hpp"
#include "playermask.hpp"
#include "vismask.hpp"

namespace MWRender
{
    namespace
    {
        // The porthole: an upper body this high over the feet, revealed within this radius of it on screen (plus a
        // soft band); never what is within this of it or below the waist
        constexpr float sBodyFrom = 70.f;
        constexpr float sBodyTo = 120.f;
        constexpr float sCoreRadius = 170.f;
        constexpr float sSoftBand = 110.f;
        constexpr float sBodyNear = 15.f;
        constexpr float sBodySoft = 35.f;
        constexpr float sWaist = 90.f;
        constexpr float sWaistSoft = 25.f;
        // ...nor what is behind the player, past this far behind them along the ground (fading over this much): seen
        // from above, the top of an arch or a pole behind them is nearer the camera than they are
        constexpr float sBehindFrom = 20.f;
        constexpr float sBehindSoft = 40.f;
        // Around the opening what is in the way is see-through: this opaque next to it, whole again this far from the
        // body
        constexpr float sGhostOpacity = 0.45f;
        constexpr float sGhostReach = 520.f;

        // When the porthole opens: rays to points over the body a few times a second; open when more than half of
        // them are hidden, closed again under this much, after a moment; and how long opening and closing take
        constexpr float sRayInterval = 0.1f;
        constexpr float sOpenAbove = 0.5f;
        constexpr float sCloseBelow = 0.3f;
        constexpr float sOpenDelay = 0.05f;
        constexpr float sCloseDelay = 0.3f;
        constexpr float sOpenTime = 0.2f;
        constexpr float sCloseTime = 0.6f;

        // Indoors, another floor: what stands wholly this far above the player's floor (over the doors and the lamps
        // on the walls; a character: standing this high), or wholly this far below it, with this much slack once it
        // is; it dissolves away over this long. A character is this tall.
        constexpr float sAbove = 220.f;
        constexpr float sActorAbove = 150.f;
        constexpr float sBelow = 40.f;
        constexpr float sFloorSlack = 16.f;
        constexpr float sFloorFade = 0.3f;
        constexpr float sActorHeight = 120.f;

        // How fast the floor follows the player's feet (per second), and how far is a teleport
        constexpr float sFloorRate = 12.f;
        constexpr float sTeleportDistance = 500.f;

        constexpr int sNoiseSize = 64; // cutaway.glsl's CUTAWAY_NOISE_SIZE

        float smoothstep(float edge0, float edge1, float x)
        {
            const float t = std::clamp((x - edge0) / (edge1 - edge0), 0.f, 1.f);
            return t * t * (3.f - 2.f * t);
        }

        // The object a scene node is the base node of
        MWWorld::Ptr ptrOf(const osg::Node& node)
        {
            if (const osg::UserDataContainer* container = node.getUserDataContainer())
            {
                for (unsigned int i = 0; i < container->getNumUserObjects(); ++i)
                {
                    if (const PtrHolder* holder = dynamic_cast<const PtrHolder*>(container->getUserObject(i)))
                        return holder->mPtr;
                }
            }
            return MWWorld::Ptr();
        }

        // What the scene's geometry is, for the rays: no terrain, characters, effects or the sky
        unsigned int geometryMask()
        {
            return ~(Mask_RenderToTexture | Mask_Sky | Mask_Debug | Mask_Effect | Mask_Water | Mask_SimpleWater
                | Mask_Groundcover | Mask_Actor | Mask_Player | Mask_Terrain | Mask_ParticleSystem
                | Mask_WeatherParticles | Mask_FirstPerson | Mask_GUI);
        }

        // A blue noise tile (void and cluster): thresholds that dissolve a surface evenly, with no visible pattern.
        // Values 1..254, so neither 0 nor 1 opacity is ever dithered.
        osg::ref_ptr<osg::Image> makeBlueNoise()
        {
            constexpr int n = sNoiseSize;
            constexpr int count = n * n;
            constexpr int radius = 6;
            constexpr float sigma = 1.9f;
            std::array<float, (2 * radius + 1) * (2 * radius + 1)> kernel;
            for (int dy = -radius; dy <= radius; ++dy)
                for (int dx = -radius; dx <= radius; ++dx)
                    kernel[(dy + radius) * (2 * radius + 1) + dx + radius]
                        = std::exp(-(dx * dx + dy * dy) / (2.f * sigma * sigma));

            std::vector<float> energy(count, 0.f);
            std::vector<char> bits(count, 0);
            auto splat = [&](std::vector<float>& field, int index, float sign) {
                const int x0 = index % n;
                const int y0 = index / n;
                for (int dy = -radius; dy <= radius; ++dy)
                    for (int dx = -radius; dx <= radius; ++dx)
                        field[((y0 + dy + n) % n) * n + (x0 + dx + n) % n]
                            += sign * kernel[(dy + radius) * (2 * radius + 1) + dx + radius];
            };
            auto tightest = [&](const std::vector<float>& field, const std::vector<char>& set) {
                int best = -1;
                for (int i = 0; i < count; ++i)
                    if (set[i] && (best < 0 || field[i] > field[best]))
                        best = i;
                return best;
            };
            auto emptiest = [&](const std::vector<float>& field, const std::vector<char>& set) {
                int best = -1;
                for (int i = 0; i < count; ++i)
                    if (!set[i] && (best < 0 || field[i] < field[best]))
                        best = i;
                return best;
            };

            std::mt19937 random(1);
            const int initial = count / 10;
            for (int placed = 0; placed < initial;)
            {
                const int i = static_cast<int>(random() % count);
                if (bits[i])
                    continue;
                bits[i] = 1;
                splat(energy, i, 1.f);
                ++placed;
            }
            // Spread them evenly: move the most crowded point to the emptiest place until that is where it was
            for (int iteration = 0; iteration < count; ++iteration)
            {
                const int crowded = tightest(energy, bits);
                bits[crowded] = 0;
                splat(energy, crowded, -1.f);
                const int empty = emptiest(energy, bits);
                bits[empty] = 1;
                splat(energy, empty, 1.f);
                if (empty == crowded)
                    break;
            }

            std::vector<int> rank(count, 0);
            {
                std::vector<float> field = energy;
                std::vector<char> set = bits;
                for (int r = initial - 1; r >= 0; --r)
                {
                    const int crowded = tightest(field, set);
                    set[crowded] = 0;
                    splat(field, crowded, -1.f);
                    rank[crowded] = r;
                }
            }
            for (int r = initial; r < count; ++r)
            {
                const int empty = emptiest(energy, bits);
                bits[empty] = 1;
                splat(energy, empty, 1.f);
                rank[empty] = r;
            }

            osg::ref_ptr<osg::Image> image = new osg::Image;
            image->allocateImage(n, n, 1, GL_RGBA, GL_UNSIGNED_BYTE);
            unsigned char* data = image->data();
            for (int i = 0; i < count; ++i)
            {
                const auto value = static_cast<unsigned char>(1 + (rank[i] * 253) / (count - 1));
                std::fill(data + i * 4, data + i * 4 + 4, value);
            }
            return image;
        }

        // The cut (with its textures) for the main camera's view only: its state is pushed while the main camera is
        // the one culling the scene, so the shadow cameras' and every render to texture's view of it (water, the map)
        // stay whole.
        class MainViewCallback : public SceneUtil::NodeCallback<MainViewCallback, osg::Node*, osgUtil::CullVisitor*>
        {
        public:
            MainViewCallback(osg::Camera* mainCamera, osg::StateSet* stateset, PlayerMask* mask, int unit)
                : mMainCamera(mainCamera)
                , mStateSet(stateset)
                , mMask(mask)
                , mUnit(unit)
            {
            }

            void operator()(osg::Node* node, osgUtil::CullVisitor* cv)
            {
                if (cv->getCurrentCamera() != mMainCamera)
                {
                    traverse(node, cv);
                    return;
                }
                if (mMask && !mBound)
                {
                    mStateSet->setTextureAttribute(mUnit, mMask->getColorTexture(cv), osg::StateAttribute::ON);
                    mBound = true;
                }
                cv->pushStateSet(mStateSet);
                traverse(node, cv);
                cv->popStateSet();
            }

        private:
            osg::Camera* mMainCamera;
            osg::ref_ptr<osg::StateSet> mStateSet;
            osg::ref_ptr<PlayerMask> mMask;
            int mUnit;
            bool mBound = false;
        };
    }

    Cutaway::Cutaway(osg::Group* rootNode, osg::Group* sceneRoot, osg::Camera* mainCamera, PlayerMask* playerMask,
        int playerMaskUnit, int noiseUnit)
        : mRootNode(rootNode)
        , mSceneRoot(sceneRoot)
        , mMainCamera(mainCamera)
        , mRayVisitor(new osgUtil::IntersectionVisitor)
    {
        auto dynamic = [](osg::Uniform* uniform) {
            uniform->setDataVariance(osg::Object::DYNAMIC);
            return uniform;
        };
        // Everything but the switch and the textures is on the root node, so every view has them; only the main view
        // turns the cut on.
        osg::StateSet* stateset = rootNode->getOrCreateStateSet();
        stateset->addUniform(new osg::Uniform("cutawayEnabled", 0.f));
        stateset->addUniform(new osg::Uniform("noCutaway", 0.f));
        stateset->addUniform(new osg::Uniform("cutawayGhostPass", 0.f));
        stateset->addUniform(new osg::Uniform("cutawayFade", 0.f));
        mGhostUniform = dynamic(new osg::Uniform("cutawayGhost", osg::Vec4f()));
        mEyeUniform = dynamic(new osg::Uniform("cutawayEye", osg::Vec3f()));
        mCapsuleA = dynamic(new osg::Uniform("cutawayCapA", osg::Vec3f()));
        mCapsuleB = dynamic(new osg::Uniform("cutawayCapB", osg::Vec3f()));
        mSight = dynamic(new osg::Uniform("cutawaySight", osg::Vec4f()));
        mLevelUniform = dynamic(new osg::Uniform("cutawayLevel", osg::Vec4f()));
        mAwayUniform = dynamic(new osg::Uniform("cutawayAway", osg::Vec4f()));
        for (osg::Uniform* uniform : { mGhostUniform.get(), mEyeUniform.get(), mCapsuleA.get(), mCapsuleB.get(),
                 mSight.get(), mLevelUniform.get(), mAwayUniform.get() })
            stateset->addUniform(uniform);
        stateset->addUniform(new osg::Uniform("playerMaskTex", playerMaskUnit));
        stateset->addUniform(new osg::Uniform("cutawayNoise", noiseUnit));

        mEnabled = dynamic(new osg::Uniform("cutawayEnabled", 0.f));
        mMainViewState = new osg::StateSet;
        mMainViewState->setDataVariance(osg::Object::DYNAMIC);
        mMainViewState->addUniform(mEnabled);

        osg::ref_ptr<osg::Texture2D> noise = new osg::Texture2D(makeBlueNoise());
        noise->setFilter(osg::Texture::MIN_FILTER, osg::Texture::NEAREST);
        noise->setFilter(osg::Texture::MAG_FILTER, osg::Texture::NEAREST);
        noise->setWrap(osg::Texture::WRAP_S, osg::Texture::REPEAT);
        noise->setWrap(osg::Texture::WRAP_T, osg::Texture::REPEAT);
        mMainViewState->setTextureAttribute(noiseUnit, noise, osg::StateAttribute::ON);

        sceneRoot->addCullCallback(new MainViewCallback(mainCamera, mMainViewState, playerMask, playerMaskUnit));

        // The ghost's pass: the cut on, as in the main view, with its texture, drawing only the ghost
        mGhostState = new osg::StateSet;
        mGhostState->setDataVariance(osg::Object::DYNAMIC);
        mGhostState->addUniform(mEnabled);
        mGhostState->addUniform(new osg::Uniform("cutawayGhostPass", 1.f));
        mGhostState->setTextureAttribute(noiseUnit, noise, osg::StateAttribute::ON);
        mGhostComposite = new GhostComposite;
        mGhostComposite->setNodeMask(0);
        rootNode->addChild(mGhostComposite);
        mRayVisitor->setTraversalMask(geometryMask());
    }

    Cutaway::~Cutaway()
    {
        restoreFloors();
    }

    void Cutaway::update(float dt, const osg::Vec3f& eye, const MWWorld::Ptr& player, bool active, bool indoors,
        osg::Group* cellNode)
    {
        mActive = active && !player.isEmpty() && player.isInCell() && mDebug >= 0;
        mEnabled->set(mActive ? 1.f : 0.f);
        if (!mActive)
        {
            mReset = true;
            restoreFloors();
            updateGhost(false);
            return;
        }

        const osg::Vec3f feet = player.getRefData().getPosition().asVec3();
        // Moved farther than anyone walks in a frame: a door or a teleport, so nothing eases over from before
        if ((feet - mLastFeet).length() > sTeleportDistance)
            mReset = true;
        mLastFeet = feet;
        mFeet = feet;

        // The floor: where the player stands, and held while they are in the air, except that it follows them
        // straight down (off a ledge), so jumping never changes which floor they are on
        MWBase::World* world = MWBase::Environment::get().getWorld();
        const bool grounded = world->isOnGround(player) || world->isFlying(player) || world->isSwimming(player);
        if (mReset)
            mFloor = feet.z();
        else if (grounded)
            mFloor += (feet.z() - mFloor) * (1.f - std::exp(-sFloorRate * dt));
        mFloor = std::min(mFloor, feet.z());

        mIndoors = indoors;
        mEye = eye;
        osg::Vec2f away(feet.x() - eye.x(), feet.y() - eye.y());
        if (away.normalize() > 1e-3f)
            mAway = away;

        if (mIndoors)
            updateFloors(dt, cellNode, player);
        else
            restoreFloors();
        updateCoverage(dt);
        mReset = false;
        // (Not in the debug view, which tints what the porthole takes instead, but for 4: the ghost in magenta)
        updateGhost((mDebug == 0 || mDebug == 4) && mSightStrength > 0.f);

        mEyeUniform->set(mEye);
        mCapsuleA->set(feet + osg::Vec3f(0.f, 0.f, sBodyFrom));
        mCapsuleB->set(feet + osg::Vec3f(0.f, 0.f, sBodyTo));
        mSight->set(osg::Vec4f(sCoreRadius, sSoftBand, mSightStrength, 0.f));
        mLevelUniform->set(osg::Vec4f(mFloor, sWaist, 0.f, 0.f));
        mAwayUniform->set(osg::Vec4f(mAway.x(), mAway.y(), 0.f, static_cast<float>(mDebug)));
    }

    void Cutaway::updateFloors(float dt, osg::Group* cellNode, const MWWorld::Ptr& player)
    {
        if (cellNode != mCellNode)
        {
            restoreFloors();
            mCellNode = cellNode;
        }
        if (!mCellNode)
            return;

        // The cell's objects, as they come and go (a dropped item, someone arriving); the player isn't one
        const osg::Node* playerNode = player.getRefData().getBaseNode();
        std::map<osg::Node*, Floored> seen;
        for (unsigned int i = 0; i < mCellNode->getNumChildren(); ++i)
        {
            osg::Node* node = mCellNode->getChild(i);
            if (node == playerNode)
                continue;
            auto found = mFloored.find(node);
            if (found != mFloored.end())
            {
                seen.emplace(node, std::move(found->second));
                mFloored.erase(found);
                continue;
            }
            const MWWorld::Ptr ptr = ptrOf(*node);
            if (ptr.isEmpty() || ptr == player)
                continue;
            Floored object;
            object.mNode = node;
            object.mActor = ptr.getClass().isActor();
            if (!object.mActor)
            {
                osg::ComputeBoundsVisitor bounds;
                bounds.setTraversalMask(~0u);
                node->accept(bounds);
                object.mBounds = bounds.getBoundingBox();
                if (!object.mBounds.valid())
                    continue;
            }
            seen.emplace(node, std::move(object));
        }
        // Whatever left the cell (or went to another) is whole again
        for (auto& [node, object] : mFloored)
            setFade(object, 0.f);
        mFloored.swap(seen);

        // Wholly above the player's floor or wholly below it: on another floor. A little slack once it is, so walking
        // up stairs doesn't flick things in and out.
        for (auto& [node, object] : mFloored)
        {
            float bottom = object.mBounds.zMin();
            float top = object.mBounds.zMax();
            if (object.mActor)
            {
                const MWWorld::Ptr ptr = ptrOf(*node);
                if (ptr.isEmpty())
                    continue;
                bottom = ptr.getRefData().getPosition().pos[2];
                top = bottom + sActorHeight;
            }
            const float slack = object.mOther ? sFloorSlack : 0.f;
            const float above = object.mActor ? sActorAbove : sAbove;
            object.mOther = bottom > mFloor + above - slack || top < mFloor - sBelow + slack;
            const float target = object.mOther ? 1.f : 0.f;
            if (mReset)
                setFade(object, target);
            else if (object.mAmount != target)
                setFade(object, object.mOther ? std::min(1.f, object.mAmount + dt / sFloorFade)
                                              : std::max(0.f, object.mAmount - dt / sFloorFade));
        }
    }

    void Cutaway::setFade(Floored& object, float amount)
    {
        object.mAmount = amount;
        if (amount > 0.f && !object.mFade)
        {
            object.mFade = new osg::Uniform("cutawayFade", amount);
            object.mFade->setDataVariance(osg::Object::DYNAMIC);
            osg::StateSet* stateset = object.mNode->getOrCreateStateSet();
            stateset->setDataVariance(osg::Object::DYNAMIC);
            stateset->addUniform(object.mFade);
        }
        if (!object.mFade)
            return;
        if (amount > 0.f)
            object.mFade->set(amount);
        else
        {
            if (osg::StateSet* stateset = object.mNode->getStateSet())
                stateset->removeUniform(object.mFade);
            object.mFade = nullptr;
        }
    }

    void Cutaway::restoreFloors()
    {
        for (auto& [node, object] : mFloored)
            setFade(object, 0.f);
        mFloored.clear();
        mCellNode = nullptr;
    }

    bool Cutaway::isHidden(const MWWorld::ConstPtr& ptr) const
    {
        if (!mActive || ptr.isEmpty())
            return false;
        const osg::Node* node = ptr.getRefData().getBaseNode();
        const auto found = mFloored.find(const_cast<osg::Node*>(node));
        return found != mFloored.end() && found->second.mOther && found->second.mAmount > 0.5f;
    }

    void Cutaway::updateGhost(bool on)
    {
        // The pass is as big as the scene is drawn (made again when that changes), so the composite reads it pixel
        // for pixel; with no size yet, no ghost
        const uint32_t width = mRenderWidth;
        const uint32_t height = mRenderHeight;
        if (width == 0 || height == 0)
            on = false;
        if (on && (!mGhostLayer || mGhostLayer->width() != width || mGhostLayer->height() != height))
        {
            if (mGhostLayer)
                mRootNode->removeChild(mGhostLayer);
            mGhostLayer = new GhostLayer(mSceneRoot, mGhostState, width, height);
            mRootNode->addChild(mGhostLayer);
            mGhostComposite->setLayer(mGhostLayer);
        }
        if (on)
        {
            // Sorted behind every transparent thing
            osg::Vec3f forward = mFeet + osg::Vec3f(0.f, 0.f, sBodyFrom) - mEye;
            forward.normalize();
            mGhostComposite->placeBehind(mEye, forward);
        }

        // The main view leaves the ghost out only while the pass draws it
        mGhostOn = on;
        mGhostUniform->set(osg::Vec4f(sGhostReach, sGhostOpacity, on ? 1.f : 0.f, 0.f));
        if (mGhostLayer)
            mGhostLayer->setNodeMask(on ? Mask_RenderToTexture : 0);
        mGhostComposite->setNodeMask(on ? ~0u : 0);
    }

    void Cutaway::updateCoverage(float dt)
    {
        mRayTimer -= dt;
        if (mRayTimer <= 0.f || mReset)
        {
            mRayTimer = sRayInterval;
            // Points over the body, from the feet to the head and to either side, each as much of it as the others
            const osg::Vec3f across = osg::Vec3f(-mAway.y(), mAway.x(), 0.f) * 22.f;
            const std::array<osg::Vec3f, 8> points = {
                osg::Vec3f(0.f, 0.f, 15.f),
                osg::Vec3f(0.f, 0.f, 40.f),
                osg::Vec3f(0.f, 0.f, 65.f),
                osg::Vec3f(0.f, 0.f, 90.f),
                osg::Vec3f(0.f, 0.f, 110.f),
                osg::Vec3f(0.f, 0.f, 125.f),
                osg::Vec3f(0.f, 0.f, 80.f) + across,
                osg::Vec3f(0.f, 0.f, 80.f) - across,
            };
            osg::ref_ptr<osgUtil::IntersectorGroup> group = new osgUtil::IntersectorGroup;
            std::array<osg::ref_ptr<osgUtil::LineSegmentIntersector>, points.size()> rays;
            for (size_t i = 0; i < points.size(); ++i)
            {
                rays[i] = new osgUtil::LineSegmentIntersector(
                    osgUtil::Intersector::MODEL, mEye, mFeet + points[i]);
                rays[i]->setIntersectionLimit(osgUtil::Intersector::NO_LIMIT);
                group->addIntersector(rays[i]);
            }
            mRayVisitor->reset();
            mRayVisitor->setIntersector(group);
            mSceneRoot->accept(*mRayVisitor);

            // A point is hidden by anything drawn on its way (not what is on another floor: that is gone)
            int hidden = 0;
            for (const auto& ray : rays)
            {
                for (const auto& hit : ray->getIntersections())
                {
                    if (onOtherFloor(hit.nodePath))
                        continue;
                    ++hidden;
                    break;
                }
            }
            mCoverage = static_cast<float>(hidden) / static_cast<float>(points.size());
        }

        // Open past one threshold, closed under a lower one, each after a moment, so walking along an edge doesn't
        // flicker; then eased, quickly open and slowly closed
        const bool wantOpen = mOpen ? mCoverage > sCloseBelow : mCoverage > sOpenAbove;
        if (wantOpen != mOpen)
        {
            mOpenFor += dt;
            if (mReset || mOpenFor >= (mOpen ? sCloseDelay : sOpenDelay))
            {
                mOpen = wantOpen;
                mOpenFor = 0.f;
            }
        }
        else
            mOpenFor = 0.f;
        const float target = mOpen ? 1.f : 0.f;
        if (mReset)
            mSightStrength = target;
        else
            mSightStrength += (target - mSightStrength) * (1.f - std::exp(-dt * 3.f / (mOpen ? sOpenTime : sCloseTime)));
    }

    bool Cutaway::onOtherFloor(const osg::NodePath& path) const
    {
        for (osg::Node* node : path)
        {
            const auto found = mFloored.find(node);
            if (found != mFloored.end())
                return found->second.mOther && found->second.mAmount > 0.5f;
        }
        return false;
    }

    // The porthole and the ghost, as shaders/lib/util/cutaway.glsl has them (cutawaySightAmounts), without the ragged
    // edge
    osg::Vec2f Cutaway::sightAt(const osg::Vec3f& point) const
    {
        if (mSightStrength <= 0.f)
            return osg::Vec2f();
        const osg::Vec3f a = mFeet + osg::Vec3f(0.f, 0.f, sBodyFrom);
        const osg::Vec3f b = mFeet + osg::Vec3f(0.f, 0.f, sBodyTo);
        osg::Vec3f ray = point - mEye;
        const float length = ray.normalize();
        const osg::Vec3f axis = b - a;
        const osg::Vec3f fromBody = mEye - a;
        const float along = ray * axis;
        const float denom = std::max(axis * axis - along * along, 1e-4f);
        const float s = std::clamp((axis * fromBody - along * (ray * fromBody)) / denom, 0.f, 1.f);
        const osg::Vec3f nearest = a + axis * s;
        const float atBody = (nearest - mEye) * ray;
        const float distance = (mEye + ray * atBody - nearest).length();
        const float footprint = 1.f - smoothstep(sCoreRadius, sCoreRadius + sSoftBand, distance);
        const float ghost = 1.f - smoothstep(sCoreRadius, sGhostReach, distance);
        const float inFront = 1.f - smoothstep(atBody - sBodyNear - sBodySoft, atBody - sBodyNear, length);
        const float high = smoothstep(sWaist - sWaistSoft, sWaist + sWaistSoft, point.z() - mFloor);
        const float ahead = -((point.x() - mFeet.x()) * mAway.x() + (point.y() - mFeet.y()) * mAway.y());
        const float notBehind = smoothstep(-sBehindFrom - sBehindSoft, -sBehindFrom, ahead);
        return osg::Vec2f(footprint, ghost) * (inFront * high * notBehind * mSightStrength);
    }

    bool Cutaway::isCut(const osg::Vec3f& point, bool actor) const
    {
        if (!mActive || actor)
            return false;
        // Where less than half the surface is left (the porthole, the faintest of the ghost), the cursor goes through
        const osg::Vec2f sight = sightAt(point);
        const float left = mGhostOn ? (1.f - sight.x()) * (1.f + (sGhostOpacity - 1.f) * sight.y()) : 1.f - sight.x();
        return left < 0.5f;
    }

    void Cutaway::setRenderSize(int width, int height)
    {
        mRenderWidth = static_cast<uint32_t>(std::max(width, 0));
        mRenderHeight = static_cast<uint32_t>(std::max(height, 0));
    }

    void Cutaway::setDebug(int level)
    {
        mDebug = level;
    }
}
