#include "playermask.hpp"

#include <osg/Camera>
#include <osg/Program>

#include "vismask.hpp"

namespace MWRender
{
    namespace
    {
        // Small enough to be cheap; it is sampled with linear filtering, which softens the silhouette's edge.
        constexpr uint32_t sMaskSize = 512; // cutaway.glsl's CUTAWAY_MASK_SIZE

        // projectionMatrix is OpenMW's own projection uniform (the scene's shaders use it instead of
        // gl_ProjectionMatrix), so the mask lines up with the main view exactly. Actors are skinned on the CPU, so
        // plain gl_Vertex is already posed.
        constexpr const char* sVertexShader = R"GLSL(
#version 120
uniform mat4 projectionMatrix;
varying float viewDepth;
void main()
{
    vec4 viewPos = gl_ModelViewMatrix * gl_Vertex;
    gl_Position = projectionMatrix * viewPos;
    viewDepth = -viewPos.z;
}
)GLSL";

        constexpr const char* sFragmentShader = R"GLSL(
#version 120
varying float viewDepth;
void main()
{
    gl_FragData[0] = vec4(viewDepth, 0.0, 0.0, 1.0);
}
)GLSL";
    }

    PlayerMask::PlayerMask(osg::Node* scene)
        : RTTNode(sMaskSize, sMaskSize, 0, false, 0, StereoAwareness::Unaware, false)
        , mScene(scene)
    {
        // Depths are a few thousand units; half floats keep plenty of precision for the cutaway's comparisons.
        setColorBufferInternalFormat(GL_RGBA16F_ARB);
        setNodeMask(Mask_RenderToTexture);
    }

    void PlayerMask::setDefaults(osg::Camera* camera)
    {
        // Inherit the main camera's view and projection.
        camera->setReferenceFrame(osg::Camera::RELATIVE_RF);
        camera->setName("PlayerMaskCamera");
        camera->setNodeMask(Mask_RenderToTexture);
        // Only the player: the scene root and the player node pass, everything else in the scene has its own mask.
        camera->setCullMask(Mask_Scene | Mask_Player);
        camera->setClearColor(osg::Vec4(0.f, 0.f, 0.f, 0.f));

        osg::ref_ptr<osg::Program> program = new osg::Program;
        program->addShader(new osg::Shader(osg::Shader::VERTEX, sVertexShader));
        program->addShader(new osg::Shader(osg::Shader::FRAGMENT, sFragmentShader));
        osg::StateSet* stateset = camera->getOrCreateStateSet();
        const auto force = osg::StateAttribute::OVERRIDE | osg::StateAttribute::PROTECTED;
        stateset->setAttributeAndModes(program, osg::StateAttribute::ON | force);
        stateset->setMode(GL_BLEND, osg::StateAttribute::OFF | force);
        stateset->setMode(GL_CULL_FACE, osg::StateAttribute::OFF | force);

        camera->addChild(mScene);
    }
}
