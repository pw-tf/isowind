#include "occlusionfader.hpp"

#include <algorithm>
#include <array>

#include <osg/BlendFunc>
#include <osg/Group>
#include <osg/UserDataContainer>
#include <osgUtil/IntersectionVisitor>
#include <osgUtil/LineSegmentIntersector>

#include <components/esm3/loaddoor.hpp>
#include <components/sceneutil/depth.hpp>
#include <components/sceneutil/positionattitudetransform.hpp>

#include "../mwworld/class.hpp"

#include "objects.hpp"
#include "vismask.hpp"

namespace MWRender
{
    namespace
    {
        // How see-through a faded object gets is up to the shader; this is how fast it gets there and back.
        constexpr float sFadeInRate = 6.f; // per second
        constexpr float sFadeOutRate = 2.5f;
        // How long something must have stopped being in the way before it fades back, so walking past a row of
        // posts or along a wall's edge doesn't make it flicker.
        constexpr float sFadeOutDelay = 0.35f;

        MWWorld::Ptr findPtr(const osg::NodePath& nodePath)
        {
            // The innermost object holding a Ptr (the reference's base node)
            for (auto it = nodePath.rbegin(); it != nodePath.rend(); ++it)
            {
                const osg::UserDataContainer* container = (*it)->getUserDataContainer();
                if (!container)
                    continue;
                for (unsigned int i = 0; i < container->getNumUserObjects(); ++i)
                {
                    if (const PtrHolder* holder = dynamic_cast<const PtrHolder*>(container->getUserObject(i)))
                        return holder->mPtr;
                }
            }
            return MWWorld::Ptr();
        }
    }

    OcclusionFader::OcclusionFader(osg::Group* sceneRoot)
        : mSceneRoot(sceneRoot)
        , mVisitor(new osgUtil::IntersectionVisitor)
    {
        // Only what can be faded: no terrain (the silhouette shows the player behind hills), characters, effects,
        // water or the sky.
        unsigned int mask = ~0u;
        mask &= ~(Mask_RenderToTexture | Mask_Sky | Mask_Debug | Mask_Effect | Mask_Water | Mask_SimpleWater
            | Mask_Groundcover | Mask_Actor | Mask_Player | Mask_Terrain | Mask_ParticleSystem
            | Mask_WeatherParticles | Mask_FirstPerson);
        mVisitor->setTraversalMask(mask);
    }

    OcclusionFader::~OcclusionFader()
    {
        clear();
    }

    bool OcclusionFader::canFade(const MWWorld::Ptr& ptr)
    {
        if (ptr.isEmpty())
            return false;
        // Characters and creatures are what the player looks for, and doors are how they get around
        return !ptr.getClass().isActor() && ptr.getType() != ESM::Door::sRecordId;
    }

    void OcclusionFader::findOccluders(
        const osg::Vec3f& eye, const MWWorld::Ptr& player, std::vector<MWWorld::Ptr>& out)
    {
        const osg::Vec3f feet = player.getRefData().getPosition().asVec3();

        // Points over the player's body and around it (about as far as the shader's keyhole, the circle in which
        // cutaway.glsl opens faded objects), so whatever covers part of it is found, not only what covers their middle.
        osg::Vec3f across = (feet - eye) ^ osg::Vec3f(0.f, 0.f, 1.f);
        if (across.normalize() < 1e-3f)
            across = osg::Vec3f(1.f, 0.f, 0.f);
        const std::array<osg::Vec3f, 11> points = {
            feet + osg::Vec3f(0.f, 0.f, 30.f),
            feet + osg::Vec3f(0.f, 0.f, 75.f),
            feet + osg::Vec3f(0.f, 0.f, 120.f),
            feet + osg::Vec3f(0.f, 0.f, 160.f),
            feet + osg::Vec3f(0.f, 0.f, 70.f) + across * 160.f,
            feet + osg::Vec3f(0.f, 0.f, 70.f) - across * 160.f,
            feet + osg::Vec3f(0.f, 0.f, 150.f) + across * 110.f,
            feet + osg::Vec3f(0.f, 0.f, 150.f) - across * 110.f,
            feet + osg::Vec3f(0.f, 0.f, 20.f) + across * 110.f,
            feet + osg::Vec3f(0.f, 0.f, 20.f) - across * 110.f,
            feet + osg::Vec3f(0.f, 0.f, 70.f),
        };

        osg::ref_ptr<osgUtil::IntersectorGroup> group = new osgUtil::IntersectorGroup;
        for (const osg::Vec3f& point : points)
        {
            osg::ref_ptr<osgUtil::LineSegmentIntersector> intersector
                = new osgUtil::LineSegmentIntersector(osgUtil::Intersector::MODEL, eye, point);
            // Every object along the way, not only the nearest
            intersector->setIntersectionLimit(osgUtil::Intersector::NO_LIMIT);
            group->addIntersector(intersector);
        }

        mVisitor->reset();
        mVisitor->setIntersector(group);
        mSceneRoot->accept(*mVisitor);

        for (const osg::ref_ptr<osgUtil::Intersector>& intersector : group->getIntersectors())
        {
            const auto& intersections
                = static_cast<osgUtil::LineSegmentIntersector*>(intersector.get())->getIntersections();
            for (const auto& intersection : intersections)
            {
                MWWorld::Ptr ptr = findPtr(intersection.nodePath);
                // (Paged objects, merged with their neighbours, have no Ptr of their own and can't be faded alone)
                if (canFade(ptr) && std::find(out.begin(), out.end(), ptr) == out.end())
                    out.push_back(ptr);
            }
        }
    }

    void OcclusionFader::update(float dt, const osg::Vec3f& eye, const MWWorld::Ptr& player, bool active)
    {
        // Objects whose node left the scene (deleted, or their cell unloaded) are gone
        for (auto it = mFaded.begin(); it != mFaded.end();)
        {
            if (it->second.mNode->getNumParents() == 0)
                it = mFaded.erase(it);
            else
                ++it;
        }

        std::vector<MWWorld::Ptr> occluders;
        if (active && !player.isEmpty() && player.isInCell())
            findOccluders(eye, player, occluders);

        for (const MWWorld::Ptr& ptr : occluders)
        {
            osg::Node* node = ptr.getRefData().getBaseNode();
            if (!node)
                continue;
            auto it = mFaded.find(node);
            if (it == mFaded.end())
            {
                Faded faded;
                faded.mPtr = ptr;
                faded.mNode = node;
                startFading(faded);
                it = mFaded.emplace(node, faded).first;
            }
            it->second.mOccludingFor = 0.f;
            it->second.mFade = std::min(1.f, it->second.mFade + dt * sFadeInRate);
        }

        for (auto it = mFaded.begin(); it != mFaded.end();)
        {
            Faded& faded = it->second;
            const bool occluding
                = std::find(occluders.begin(), occluders.end(), faded.mPtr) != occluders.end();
            if (!occluding)
            {
                faded.mOccludingFor += dt;
                if (faded.mOccludingFor > sFadeOutDelay)
                    faded.mFade = std::max(0.f, faded.mFade - dt * sFadeOutRate);
            }
            faded.mUniform->set(faded.mFade);

            if (faded.mFade <= 0.f && !occluding)
            {
                stopFading(faded);
                it = mFaded.erase(it);
            }
            else
                ++it;
        }
    }

    void OcclusionFader::clear()
    {
        for (auto& [node, faded] : mFaded)
            stopFading(faded);
        mFaded.clear();
    }

    void OcclusionFader::startFading(Faded& faded)
    {
        faded.mUniform = new osg::Uniform("occluderFade", 0.f);
        faded.mUniform->setDataVariance(osg::Object::DYNAMIC);
        osg::StateSet* stateset = faded.mNode->getOrCreateStateSet();
        stateset->setDataVariance(osg::Object::DYNAMIC);
        stateset->addUniform(faded.mUniform);

        // Blended with what is behind, and not in the depth buffer, so post-processing that reads depth (ambient
        // occlusion, fog) works on the scene behind it
        faded.mRenderingHint = stateset->getRenderingHint();
        faded.mRenderBinMode = stateset->getRenderBinMode();
        faded.mBinNumber = stateset->getBinNumber();
        faded.mBinName = stateset->getBinName();
        stateset->setAttributeAndModes(new osg::BlendFunc(osg::BlendFunc::SRC_ALPHA, osg::BlendFunc::ONE_MINUS_SRC_ALPHA),
            osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
        stateset->setAttributeAndModes(new SceneUtil::AutoDepth(osg::Depth::LEQUAL, 0.0, 1.0, false),
            osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
        stateset->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
        stateset->setRenderBinMode(osg::StateSet::OVERRIDE_RENDERBIN_DETAILS);
    }

    void OcclusionFader::stopFading(Faded& faded)
    {
        osg::StateSet* stateset = faded.mNode->getStateSet();
        if (!stateset)
            return;
        stateset->removeUniform(faded.mUniform);
        stateset->removeAttribute(osg::StateAttribute::BLENDFUNC);
        stateset->removeMode(GL_BLEND);
        stateset->removeAttribute(osg::StateAttribute::DEPTH);
        stateset->setRenderingHint(faded.mRenderingHint);
        stateset->setRenderBinDetails(faded.mBinNumber, faded.mBinName,
            static_cast<osg::StateSet::RenderBinMode>(faded.mRenderBinMode));
    }

    std::vector<MWWorld::Ptr> OcclusionFader::getFadedObjects() const
    {
        std::vector<MWWorld::Ptr> result;
        for (const auto& [node, faded] : mFaded)
        {
            if (faded.mFade > 0.05f && faded.mNode->getNumParents() > 0)
                result.push_back(faded.mPtr);
        }
        return result;
    }
}
