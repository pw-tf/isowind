#ifndef OPENMW_MWRENDER_GHOSTLAYER_H
#define OPENMW_MWRENDER_GHOSTLAYER_H

#include <osg/Geometry>
#include <osg/Group>
#include <osg/ref_ptr>

#include <components/sceneutil/rtt.hpp>

namespace osg
{
    class StateSet;
}

namespace MWRender
{
    /// Isowind's top-down camera, outdoors: what stands between the camera and the player turns see-through around
    /// them (shaders/lib/util/cutaway.glsl). The main view leaves those surfaces out; this pass draws only them, from the
    /// same view, into a texture, with its own depth buffer, so of a building only its nearest surface is kept (blending
    /// a whole mesh would show its own back walls and floors through it). GhostComposite then lays that over the main
    /// view, as see-through as the shader made each pixel, behind whatever stands in front of it.
    class GhostLayer : public SceneUtil::RTTNode
    {
    public:
        /// passState: what the pass needs set (the cut's switch, its textures, cutawayGhostPass).
        GhostLayer(osg::Node* scene, osg::StateSet* passState, uint32_t width, uint32_t height);

        void setDefaults(osg::Camera* camera) override;

    private:
        osg::ref_ptr<osg::Node> mScene;
        osg::ref_ptr<osg::StateSet> mPassState;
    };

    /// Lays a GhostLayer over the main view: a full screen triangle, drawn first among the transparent things (after
    /// the water), blended, and depth-tested at the ghost's own depth, so whoever stands in front of the building stays
    /// in front of it. Add it to the main camera's scene only.
    class GhostComposite : public osg::Group
    {
    public:
        GhostComposite();

        void setLayer(GhostLayer* layer) { mLayer = layer; }
        GhostLayer* getLayer() const { return mLayer.get(); }

        /// Keeps it sorted behind every transparent thing: its (unculled) bounds are put far along the view.
        void placeBehind(const osg::Vec3f& eye, const osg::Vec3f& forward);

    private:
        osg::ref_ptr<GhostLayer> mLayer;
        osg::ref_ptr<osg::Geometry> mTriangle;
    };
}

#endif
