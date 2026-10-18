#ifndef OPENMW_MWRENDER_PLAYERMASK_H
#define OPENMW_MWRENDER_PLAYERMASK_H

#include <components/sceneutil/rtt.hpp>

namespace MWRender
{
    /// Renders the player alone, with the main camera's view and projection, into a small texture holding their
    /// view-space depth (red) and coverage (alpha). The top-down cutaway (shaders/lib/util/cutaway.glsl) reads it to
    /// remove exactly what is in front of the player on screen, rather than guessing from distances.
    class PlayerMask : public SceneUtil::RTTNode
    {
    public:
        explicit PlayerMask(osg::Node* scene);

        void setDefaults(osg::Camera* camera) override;

    private:
        osg::ref_ptr<osg::Node> mScene;
    };
}

#endif
