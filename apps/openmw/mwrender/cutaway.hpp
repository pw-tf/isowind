#ifndef OPENMW_MWRENDER_CUTAWAY_H
#define OPENMW_MWRENDER_CUTAWAY_H

#include <map>

#include <osg/BoundingBox>
#include <osg/Node>
#include <osg/Uniform>
#include <osg/Vec2f>
#include <osg/Vec3f>
#include <osg/ref_ptr>

#include "../mwworld/ptr.hpp"

#include "ghostlayer.hpp"

namespace osg
{
    class Camera;
    class Group;
    class Node;
    class StateSet;
}

namespace osgUtil
{
    class IntersectionVisitor;
}

namespace MWRender
{
    class PlayerMask;

    /// Isowind's top-down camera: what is removed so the player can be seen.
    ///  - The porthole, in and out of doors: where more than half of the player is hidden (raycasts to points of their
    ///    body), what lies on the view rays in front of their upper body goes, as a clear opening in a see-through
    ///    ghost of it (GhostLayer); never anything behind them or below their waist, so a bar or a table in front of
    ///    them leaves them standing behind it. Evaluated per pixel by shaders/lib/util/cutaway.glsl and the same way
    ///    by isCut(), so the cursor never lands on what isn't drawn.
    ///  - Indoors, the other floors: every object standing wholly above the player's floor or wholly below it (walls,
    ///    upper floors, furniture, doors, people) dissolves away; whatever reaches the player's floor (their room,
    ///    the stairs) stays. Morrowind's rooms only show their ceilings from inside, so from above the room is seen
    ///    without them. (Gone only from the view: their lamps still light, and they still cast shadows.)
    /// Only the main view is cut: shadows, reflections and the map see the whole scene.
    class Cutaway
    {
    public:
        Cutaway(osg::Group* rootNode, osg::Group* sceneRoot, osg::Camera* mainCamera, PlayerMask* playerMask,
            int playerMaskUnit, int noiseUnit);
        ~Cutaway();

        /// Moves the cut to this frame's camera and player. active: the top-down camera is in use; cellNode: the
        /// scene node of the objects of the cell the player is in, when indoors.
        void update(float dt, const osg::Vec3f& eye, const MWWorld::Ptr& player, bool active, bool indoors,
            osg::Group* cellNode);

        /// Whether the point is cut away by the porthole, as the shaders cut it; actor: the point is on a character,
        /// a creature or a door. (What is on another floor is left out of the scene, so rays never hit it.)
        bool isCut(const osg::Vec3f& point, bool actor) const;

        /// Whether the object is on another floor and gone from the view (indoors), so the cursor goes through it.
        bool isHidden(const MWWorld::ConstPtr& ptr) const;

        /// The size the scene is drawn at (the post-processor's, smaller than the window with the pixel art look):
        /// the ghost's pass is drawn at it too, pixel for pixel.
        void setRenderSize(int width, int height);

        /// Debug view: what the cut removes is drawn tinted instead (blue: the porthole, orange: another floor going);
        /// 4 shows the ghost's own pass (magenta: drawn see-through, cyan: the clear opening, dark blue: left to the
        /// main view); -1 cuts nothing, to compare.
        void setDebug(int level);

    private:
        // An object of the cell indoors, and how far it has dissolved away
        struct Floored
        {
            osg::ref_ptr<osg::Node> mNode;
            osg::BoundingBox mBounds; // (world; for an actor, worked out where it stands each time)
            bool mActor = false;
            osg::ref_ptr<osg::Uniform> mFade;
            float mAmount = 0.f;
            bool mOther = false; // on another floor
        };

        void updateFloors(float dt, osg::Group* cellNode, const MWWorld::Ptr& player);
        void setFade(Floored& object, float amount);
        void restoreFloors();
        void updateCoverage(float dt);
        void updateGhost(bool on);
        bool onOtherFloor(const osg::NodePath& path) const;
        osg::Vec2f sightAt(const osg::Vec3f& point) const;

        osg::ref_ptr<osg::Group> mRootNode;
        osg::ref_ptr<osg::Group> mSceneRoot;
        osg::Camera* mMainCamera;
        osg::ref_ptr<osgUtil::IntersectionVisitor> mRayVisitor;

        osg::ref_ptr<osg::StateSet> mMainViewState; // pushed for the main camera only
        osg::ref_ptr<osg::Uniform> mEnabled;
        osg::ref_ptr<osg::Uniform> mEyeUniform;
        osg::ref_ptr<osg::Uniform> mCapsuleA;
        osg::ref_ptr<osg::Uniform> mCapsuleB;
        osg::ref_ptr<osg::Uniform> mSight;
        osg::ref_ptr<osg::Uniform> mLevelUniform;
        osg::ref_ptr<osg::Uniform> mAwayUniform;
        osg::ref_ptr<osg::Uniform> mGhostUniform;
        int mDebug = 0;

        // The values the shaders get, kept for isCut
        bool mActive = false;
        bool mIndoors = false;
        osg::Vec3f mEye;
        osg::Vec3f mFeet;
        osg::Vec3f mLastFeet;
        osg::Vec2f mAway{ 0.f, 1.f }; // away from the camera along the ground
        float mFloor = 0.f;
        bool mReset = true;

        // The porthole: how much of the player is hidden (raycasts a few times a second), whether that's enough to
        // open it, and how open it is (eased)
        float mRayTimer = 0.f;
        float mCoverage = 0.f;
        bool mOpen = false;
        float mOpenFor = 0.f; // seconds the coverage has been on the other side of the threshold
        float mSightStrength = 0.f;

        // The ghost: the pass that draws it (made for the scene's size), and what lays it over the view
        osg::ref_ptr<osg::StateSet> mGhostState;
        osg::ref_ptr<GhostLayer> mGhostLayer;
        osg::ref_ptr<GhostComposite> mGhostComposite;
        bool mGhostOn = false;
        uint32_t mRenderWidth = 0;
        uint32_t mRenderHeight = 0;

        // The other floors: the cell's objects, by node
        osg::ref_ptr<osg::Group> mCellNode;
        std::map<osg::Node*, Floored> mFloored;
    };
}

#endif
