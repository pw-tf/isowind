#include "ghostlayer.hpp"

#include <osg/BlendFunc>
#include <osg/Camera>
#include <osg/ColorMaski>
#include <osg/Program>
#include <osgUtil/CullVisitor>

#include <components/sceneutil/depth.hpp>
#include <components/sceneutil/nodecallback.hpp>

#include "vismask.hpp"

namespace MWRender
{
    namespace
    {
        // Keeps the main view's eye point (for LODs) as the camera's own: the pass must draw what the main view
        // would, down to the level of detail, or its edge would show (as Water's InheritViewPointCallback)
        class InheritViewPoint : public SceneUtil::NodeCallback<InheritViewPoint, osg::Node*, osgUtil::CullVisitor*>
        {
        public:
            void operator()(osg::Node* node, osgUtil::CullVisitor* cv)
            {
                osg::ref_ptr<osg::RefMatrix> modelViewMatrix = new osg::RefMatrix(*cv->getModelViewMatrix());
                cv->popModelViewMatrix();
                cv->pushModelViewMatrix(modelViewMatrix, osg::Transform::ABSOLUTE_RF_INHERIT_VIEWPOINT);
                traverse(node, cv);
            }
        };

        // Binds the layer's textures for the composite, as the main view sees them
        class BindLayer : public SceneUtil::NodeCallback<BindLayer, GhostComposite*, osgUtil::CullVisitor*>
        {
        public:
            void operator()(GhostComposite* node, osgUtil::CullVisitor* cv)
            {
                GhostLayer* layer = node->getLayer();
                if (!layer)
                    return;
                osg::StateSet* stateset = node->getOrCreateStateSet();
                // Read texel for pixel: filtering would blend a surface's depth with the empty background at its
                // edges, and the edges would fail the depth test (bright slivers where the sky is behind)
                osg::Texture* textures[] = { layer->getColorTexture(cv), layer->getDepthTexture(cv) };
                for (int unit = 0; unit < 2; ++unit)
                {
                    osg::Texture* texture = textures[unit];
                    if (texture->getFilter(osg::Texture::MIN_FILTER) != osg::Texture::NEAREST)
                    {
                        texture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::NEAREST);
                        texture->setFilter(osg::Texture::MAG_FILTER, osg::Texture::NEAREST);
                    }
                    stateset->setTextureAttribute(unit, texture, osg::StateAttribute::ON);
                }
                traverse(node, cv);
            }
        };

        // The composite's bounds: wherever placeBehind put them, whatever its vertices (they are screen positions)
        struct FarBounds : public osg::Drawable::ComputeBoundingBoxCallback
        {
            osg::BoundingBox mBox;
            osg::BoundingBox computeBound(const osg::Drawable&) const override { return mBox; }
        };

        constexpr const char* sVertexShader = R"GLSL(
#version 120
void main()
{
    gl_Position = vec4(gl_Vertex.xy, 0.0, 1.0);
}
)GLSL";

        // The layer's colour, as see-through as its alpha, at its own depth
        constexpr const char* sFragmentShader = R"GLSL(
#version 120
uniform sampler2D ghostColour;
uniform sampler2D ghostDepth;
uniform vec2 screenRes;
void main()
{
    vec2 uv = gl_FragCoord.xy / screenRes;
    vec4 colour = texture2D(ghostColour, uv);
    // (Next to nothing, as in the opening: it isn't there, for depth either)
    if (colour.a < 0.02)
        discard;
    gl_FragDepth = texture2D(ghostDepth, uv).r;
    gl_FragData[0] = colour;
}
)GLSL";
    }

    GhostLayer::GhostLayer(osg::Node* scene, osg::StateSet* passState, uint32_t width, uint32_t height)
        : RTTNode(width, height, 0, false, 1, StereoAwareness::Unaware, false)
        , mScene(scene)
        , mPassState(passState)
    {
        setColorBufferInternalFormat(GL_RGBA8);
        setNodeMask(Mask_RenderToTexture);
    }

    void GhostLayer::setDefaults(osg::Camera* camera)
    {
        // The main camera's view and projection (as Water's refraction), so it matches the main view exactly
        camera->setReferenceFrame(osg::Camera::RELATIVE_RF);
        camera->addCullCallback(new InheritViewPoint);
        camera->setComputeNearFarMode(osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR);
        camera->setName("CutawayGhostCamera");
        camera->setNodeMask(Mask_RenderToTexture);
        camera->setClearColor(osg::Vec4f(0.f, 0.f, 0.f, 0.f));
        // What can turn see-through (statics and objects; the terrain for the objects merged into it far off, though
        // the ground itself never does), and the lights
        camera->setCullMask(Mask_Scene | Mask_Static | Mask_Object | Mask_Terrain | Mask_Lighting | Mask_Sun);
        camera->getOrCreateStateSet()->merge(*mPassState);
        camera->addChild(mScene);
    }

    GhostComposite::GhostComposite()
        : mTriangle(new osg::Geometry)
    {
        // One triangle over the whole screen
        osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
        vertices->push_back(osg::Vec3f(-1.f, -1.f, 0.f));
        vertices->push_back(osg::Vec3f(3.f, -1.f, 0.f));
        vertices->push_back(osg::Vec3f(-1.f, 3.f, 0.f));
        mTriangle->setVertexArray(vertices);
        mTriangle->addPrimitiveSet(new osg::DrawArrays(GL_TRIANGLES, 0, 3));
        mTriangle->setUseDisplayList(false);
        mTriangle->setCullingActive(false);
        mTriangle->setComputeBoundingBoxCallback(new FarBounds);
        addChild(mTriangle);
        setCullingActive(false);
        addCullCallback(new BindLayer);

        osg::StateSet* stateset = getOrCreateStateSet();
        osg::ref_ptr<osg::Program> program = new osg::Program;
        program->addShader(new osg::Shader(osg::Shader::VERTEX, sVertexShader));
        program->addShader(new osg::Shader(osg::Shader::FRAGMENT, sFragmentShader));
        const auto force = osg::StateAttribute::OVERRIDE | osg::StateAttribute::PROTECTED;
        stateset->setAttributeAndModes(program, osg::StateAttribute::ON | force);
        stateset->addUniform(new osg::Uniform("ghostColour", 0));
        stateset->addUniform(new osg::Uniform("ghostDepth", 1));
        stateset->setAttributeAndModes(
            new osg::BlendFunc(osg::BlendFunc::SRC_ALPHA, osg::BlendFunc::ONE_MINUS_SRC_ALPHA),
            osg::StateAttribute::ON | force);
        // Tested against what the main view drew, and written: post-processing that reads depth (fog, ambient
        // occlusion) must see the see-through surface where it is, not what is behind it (the sky would fog it white)
        stateset->setAttributeAndModes(
            new SceneUtil::AutoDepth(osg::Depth::LEQUAL, 0.0, 1.0, true), osg::StateAttribute::ON | force);
        stateset->setMode(GL_CULL_FACE, osg::StateAttribute::OFF | force);
        // Only the colour: the normals the post-processing reads (a second target, when there is one) stay as drawn
        stateset->setAttribute(new osg::ColorMaski(1, false, false, false, false), osg::StateAttribute::ON | force);
        stateset->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
    }

    void GhostComposite::placeBehind(const osg::Vec3f& eye, const osg::Vec3f& forward)
    {
        // The transparent bin is drawn back to front by the distance of each thing's bounds
        const osg::Vec3f far = eye + forward * 1e6f;
        static_cast<FarBounds*>(mTriangle->getComputeBoundingBoxCallback())->mBox
            = osg::BoundingBox(far - osg::Vec3f(1.f, 1.f, 1.f), far + osg::Vec3f(1.f, 1.f, 1.f));
        mTriangle->dirtyBound();
    }
}
