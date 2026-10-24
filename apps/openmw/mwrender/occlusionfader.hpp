#ifndef OPENMW_MWRENDER_OCCLUSIONFADER_H
#define OPENMW_MWRENDER_OCCLUSIONFADER_H

#include <map>
#include <vector>

#include <osg/Uniform>
#include <osg/Vec3f>
#include <osg/ref_ptr>

#include "../mwworld/ptr.hpp"

namespace osg
{
    class Group;
    class Node;
}

namespace osgUtil
{
    class IntersectionVisitor;
}

namespace MWRender
{
    /// Isowind's top-down camera: fades whole objects that stand between the camera and the player (a house, a wall,
    /// a bridge, a tree) to a see-through ghost, rather than cutting holes into the scene, and fades them back once
    /// they no longer do. Each faded object gets its own occluderFade uniform, which shaders/lib/util/cutaway.glsl
    /// turns into a dithered fade. Characters, creatures and doors are never faded.
    class OcclusionFader
    {
    public:
        explicit OcclusionFader(osg::Group* sceneRoot);
        ~OcclusionFader();

        /// Finds what is between eye and the player now (only while active, otherwise everything fades back) and
        /// moves each object's fade towards where it should be.
        void update(float dt, const osg::Vec3f& eye, const MWWorld::Ptr& player, bool active);

        /// Forgets every object at once (the scene is changing).
        void clear();

        /// Objects currently fading out or faded; the top-down cursor picks through these.
        std::vector<MWWorld::Ptr> getFadedObjects() const;

        /// Whether something level (a floor, a bridge, a canton's tier) hangs over the player or right around them,
        /// as of the last update while active; the room-like cut then applies outdoors too (RenderingManager::update).
        bool isPlayerCovered() const { return mPlayerCovered; }

    private:
        struct Faded
        {
            MWWorld::Ptr mPtr;
            osg::ref_ptr<osg::Node> mNode;
            osg::ref_ptr<osg::Uniform> mUniform;
            // While it fades it is blended and kept out of the depth buffer (see cutaway.glsl): its render bin before
            int mRenderingHint = 0;
            int mRenderBinMode = 0;
            int mBinNumber = 0;
            std::string mBinName;
            float mFade = 0.f;
            float mOccludingFor = 0.f; // seconds since it last stood in the way, while it no longer does
        };

        void startFading(Faded& faded);
        void stopFading(Faded& faded);
        void findOccluders(const osg::Vec3f& eye, const MWWorld::Ptr& player, std::vector<MWWorld::Ptr>& out);
        static bool canFade(const MWWorld::Ptr& ptr);

        osg::ref_ptr<osg::Group> mSceneRoot;
        osg::ref_ptr<osgUtil::IntersectionVisitor> mVisitor;
        std::map<osg::Node*, Faded> mFaded;
        bool mPlayerCovered = false;
    };
}

#endif
