#include <core/app/Application.h>
#include <core/system/SystemGUI.h>
#include <core/scene/Scene.h>
#include <core/asset/AssetDatabase.h> // TODO Remove?
#include <core/util/typecheck.h>
#include <core/util/Profiler.h>

#include <modules/render/RenderState.h>
#include <modules/render/entities/VRHeadset.h>
#include <modules/render/entities/RenderStep.h>

#include "RenderManager.h"
#include "Drawable.h"
#include "Camera.h"

namespace sn
{

SN_OBJECT_IMPL(RenderManager)

const char * RenderManager::TAG = "RenderManager";

//------------------------------------------------------------------------------
RenderManager::RenderManager() : Entity(), m_effectQuad(nullptr)
{
    // This entity is always present across scenes
    setFlag(SN_EF_STICKY, true);
}

//------------------------------------------------------------------------------
RenderManager::~RenderManager()
{
    // Release screens
    for (auto it = m_screens.begin(); it != m_screens.end(); ++it)
    {
        delete it->second;
    }
    m_screens.clear();

    if (m_effectQuad)
    {
        m_effectQuad->release();
        m_effectQuad = nullptr;
    }
}

//------------------------------------------------------------------------------
void RenderManager::onReady()
{
    Entity::onReady();

    // TODO If two renderManager are created, just use another window ID and it might work
    SN_ASSERT(getScene()->getTaggedEntity(TAG) == nullptr, "Two RenderManagers is not supported at the moment");

    addTag(TAG);

    // Register to update manager.
    setUpdatable(true, getObjectType().getName());
    listenToSystemEvents();

    // TODO This must depend on RenderManager service parameters
    // Create default screen
    //addScreen(0);
}

//------------------------------------------------------------------------------
RenderScreen * RenderManager::addScreen(u32 windowID, const WindowParams * winParams)
{
    // Note: windowID can be -1 to mean "give me any window"

    auto it = m_screens.find(windowID);
    if (it == m_screens.end())
    {
        // Get or create window in which we'll render the final image
        Window * window = SystemGUI::get().getWindowByID(windowID);
        if (window == nullptr)
        {
            if (winParams)
                window = SystemGUI::get().createWindow(*winParams);
            else
                window = SystemGUI::get().createWindow();
            windowID = SystemGUI::get().getWindowID(window);
        }
        else if (winParams)
        {
            window->applyParams(*winParams);
        }

        RenderScreen * screen = new RenderScreen(*window);
        m_screens[windowID] = screen;
        return screen;
    }
    else
    {
        SN_ERROR("Render screen already exist for window " << windowID);
        return nullptr;
    }
}

//------------------------------------------------------------------------------
void RenderManager::removeScreen(u32 windowID, bool showError)
{
    auto it = m_screens.find(windowID);
    if (it != m_screens.end())
    {
        delete it->second;
        m_screens.erase(it);
    }
    else if (showError)
    {
        SN_ERROR("Render screen not found for window " << windowID);
    }
}

//------------------------------------------------------------------------------
RenderScreen * RenderManager::getScreenByWindowId(u32 windowID)
{
    auto it = m_screens.find(windowID);
    if (it != m_screens.end())
        return it->second;
    else
        return nullptr;
}

//------------------------------------------------------------------------------
void RenderManager::onUpdate()
{
    Entity::onUpdate();

    VideoDriver * driverPtr = Application::get().getDriverManager().getDriver<VideoDriver>();
    if (driverPtr)
    {
        // Render!
        render(*driverPtr);
    }
}

//------------------------------------------------------------------------------
bool RenderManager::onSystemEvent(const sn::Event & event)
{
	switch (event.type)
	{
	case SN_EVENT_WINDOW_RESIZED:
        onWindowResized(event);
		break;

    case SN_EVENT_WINDOW_ASKED_CLOSE:
        onWindowClosed(event.windowID);
        break;

	default:
		break;
	}

	// The events are not consumed by default
	return false;
}

//------------------------------------------------------------------------------
void RenderManager::onWindowResized(const Event & ev)
{
    RenderScreen * screen = getScreenByWindowId(ev.windowID);
    if (screen)
    {
        onScreenResized(ev.windowID, ev.window.width, ev.window.height);
    }
}

//------------------------------------------------------------------------------
void RenderManager::onScreenResized(u32 windowID, u32 width, u32 height)
{
    auto cameras = getScene()->getTaggedEntities(Camera::TAG);
    for (auto it = cameras.begin(); it != cameras.end(); ++it)
    {
        Entity * e = *it;
        if (e->isInstanceOf<Camera>())
        {
            Camera & cam = *(Camera*)e;
            if (cam.getTargetWindowID() == windowID)
            {
                cam.onTargetResized();
            }
        }
        else
        {
            SN_WARNING("Entity tagged Camera is not a Camera: " << e->toString());
        }
    }
}

//------------------------------------------------------------------------------
void RenderManager::onWindowClosed(u32 windowID)
{
    removeScreen(windowID, false);
}

//------------------------------------------------------------------------------
// Helper
template <typename T>
T * checkTaggedType(const std::string & tag, Entity * e)
{
    const ObjectType & ot = getObjectType<T>();
    if (e->getObjectType().is(ot))
    {
        return static_cast<T*>(e);
    }
    else
    {
        SN_ERROR("Entity " << e->toString() << " has tag " << tag << " but is not a " << ot.toString());
        return nullptr;
    }
}

//------------------------------------------------------------------------------
void RenderManager::render(VideoDriver & driver)
{
    SN_BEGIN_PROFILE_SAMPLE_NAMED("Render");

    /*
    renderAllTaggedCameras(driver, Camera::TAG);
    
    // Display rendered surfaces
    for (auto it = m_screens.begin(); it != m_screens.end(); ++it)
    {
        RenderScreen & screen = *it->second;
        screen.swapBuffers();
    }
    */

    // Get steps
    std::vector<RenderStep*> renderSteps;
    getChildrenOfType<RenderStep>(renderSteps);

    if (!renderSteps.empty())
    {
        // Execute steps
        for (auto it = renderSteps.begin(); it != renderSteps.end(); ++it)
        {
            RenderStep * step = *it;
            step->onDraw(driver);
        }

        // Display rendered surfaces
        for (auto it = m_screens.begin(); it != m_screens.end(); ++it)
        {
            RenderScreen & screen = *it->second;
            screen.swapBuffers();
        }
    }

    SN_END_PROFILE_SAMPLE();
}

//------------------------------------------------------------------------------
void RenderManager::renderAllTaggedCameras(VideoDriver & driver, const std::string & tag)
{
    Scene * scene = getScene();

    // Get cameras
    std::vector<Entity*> cameras = scene->getTaggedEntities(Camera::TAG);
    std::vector<Camera*> sortedCameras;
    for (auto it = cameras.begin(); it != cameras.end(); ++it)
    {
        Camera * cam = checkTaggedType<Camera>(Camera::TAG, *it);
        if (cam && cam->isEnabled())
        {
            sortedCameras.push_back(cam);
        }
    }

    // Sort cameras
    std::sort(sortedCameras.begin(), sortedCameras.end(), 
        [](Camera * a, Camera * b) {
            return a->getDrawOrder() < b->getDrawOrder();
        }
    );

    // Draw scene parts viewed by cameras
    for (auto it = sortedCameras.begin(); it != sortedCameras.end(); ++it)
    {
        renderCamera(driver, **it);
    }
}

//------------------------------------------------------------------------------
void RenderManager::renderCamera(VideoDriver & driver, Camera & camera)
{
    SN_BEGIN_PROFILE_SAMPLE_NAMED("Render camera");

    // Note: at the moment, only render with a single main context

    // Get context of the target window, or the main shared context if we render offscreen
    RenderScreen * screen = camera.getRenderTarget() == nullptr ? getScreenByWindowId(camera.getTargetWindowID()) : nullptr;
    if (screen)
        driver.makeCurrent(*screen);
    else
        driver.makeCurrent();

    // Get VR
    // TODO VR shouldn't be searched on each render
    VRHeadset * vr = nullptr;
    VRHeadset::EyeIndex vrEyeIndex = VRHeadset::EYE_LEFT;
    if (camera.getParent())
    {
        vr = camera.getParent()->getChild<VRHeadset>();
        if (vr)
        {
            if (camera.hasTag(vr->getAbstractEyeDescription(VRHeadset::EYE_LEFT).tag))
            {
                vrEyeIndex = VRHeadset::EYE_LEFT;
            }
            else if (camera.hasTag(vr->getAbstractEyeDescription(VRHeadset::EYE_RIGHT).tag))
            {
                vrEyeIndex = VRHeadset::EYE_RIGHT;
            }
            else
            {
                vr = nullptr;
            }
        }
    }

#ifdef SN_BUILD_DEBUG
    // Press a key to disable camera effects
    bool bypassEffects = false;
    if (camera.getRenderTarget() == nullptr)
    {
        bypassEffects = sn::isKeyPressed(SN_KEY_F6);
    }
#else
    const bool bypassEffects = false;
#endif

    IntRect viewport = camera.getPixelViewport();

    // If the camera has effects
    if (camera.getEffectCount() > 0 && !bypassEffects)
    {
        // Update the resolution of framebuffers
        // TODO Don't leave this here, it should be automated (render target resizing)
        if (vr)
        {
            Vector2u fbSize = vr->getPreferredFramebufferSize(vrEyeIndex);
            camera.updateEffectBuffers(&fbSize);
        }
        else
        {
            camera.updateEffectBuffers();
        }

        RenderTexture * rt = camera.getEffectBuffer(0);
        // Bind its first effect framebuffer as target of scene rendering
        RenderTexture::bind(rt);
        // Use filled viewport
        Vector2u size = rt->getSize();
        driver.setViewport(0, 0, size.x(), size.y());
    }
    else
    {
        // Or bind the target directly
        RenderTexture::bind(camera.getRenderTarget());
        // Set final viewport
        driver.setViewport(viewport);
    }

    // Clear?
    if (camera.getClearBits())
    {
        ClearMask mask = camera.getClearBits();
        if (mask & SNR_CLEAR_COLOR)
            driver.clearColor(camera.getClearColor());
        driver.clearTarget(mask);
    }

    // Get drawables
    std::vector<Entity*> drawables = getScene()->getTaggedEntities(Drawable::TAG);
    std::vector<Drawable*> sortedDrawables;
    for (auto it = drawables.begin(); it != drawables.end(); ++it)
    {
        Drawable * d = checkTaggedType<Drawable>(Drawable::TAG, *it);
        if (d && d->isEnabled() && d->hasTag(camera.getVisibilityTag()))
        {
            sortedDrawables.push_back(d);
        }
    }

    // TODO Fix broken draw order of drawables (put it in sn::Drawable?)
    // Sort drawables
    //std::sort(sortedDrawables.begin(), sortedDrawables.end(),
    //    [](Drawable * a, Drawable * b) {
    //        return a->getDrawOrder() < b->getDrawOrder();
    //    }
    //);

    Matrix4 projectionMatrix = camera.getProjectionMatrix();
    Matrix4 viewMatrix = camera.getViewMatrix();

    // TODO Refactor temporary code
    // This is a temporary workaround to setup a FOV specified by 4 angles,
    // because current Cameras and Matrix4 are not VR-ready
    if (vr)
    {
        const VRHeadset::EyeDescription & eyeDesc = vr->getAbstractEyeDescription(vrEyeIndex);
        projectionMatrix.loadPerspectiveProjection(eyeDesc.fov, camera.getNear(), camera.getFar());
    }

    RenderState state(driver);
    state.viewMatrix = viewMatrix;
    state.projectionMatrix = projectionMatrix;

    // Draw them
    for (auto it = sortedDrawables.begin(); it != sortedDrawables.end(); ++it) 
    {
        Drawable & d = **it;
        d.onDraw(state);

        driver.useProgram(nullptr);
    }

    // If the camera has effects
    if (camera.getEffectCount() > 0 && !bypassEffects)
    {
        // Create the default quad on which we'll render images
        if (m_effectQuad == 0)
        {
            m_effectQuad = new Mesh();

            VertexDescription desc;
            desc.addAttribute("Position", VertexAttribute::USE_POSITION, VertexAttribute::TYPE_FLOAT32, 3);
            desc.addAttribute("Texcoord", VertexAttribute::USE_TEXCOORD, VertexAttribute::TYPE_FLOAT32, 2);
            m_effectQuad->create(desc);
            m_effectQuad->setPrimitiveType(SN_MESH_QUADS);

            const std::array<f32, 4*3> positions = {
                -1, -1, 0,
                 1, -1, 0,
                 1,  1, 0,
                -1,  1, 0
            };
            const std::array<f32, 4*2> uvs = {
                0, 0,
                1, 0,
                1, 1,
                0, 1
            };

            m_effectQuad->updateArray<f32>(VertexAttribute::USE_POSITION, positions);
            m_effectQuad->updateArray<f32>(VertexAttribute::USE_TEXCOORD, uvs);

            m_effectQuad->recalculateIndices();
        }

        // Disable depth-test, we'll draw an overlay
        driver.setDepthTest(false);

        RenderTexture * targetBuffer = camera.getEffectBuffer(0);
        RenderTexture * sourceBuffer = camera.getEffectBuffer(1);

        // Apply effects
        for (u32 i = 0; i < camera.getEffectCount(); ++i)
        {
            // Swap source and target:
            // Source becomes what we rendered before, target is where we'll draw with the added effect
            std::swap(sourceBuffer, targetBuffer);

            // If it's the last effect
            if (i + 1 == camera.getEffectCount())
            {
                // Bind the final target (which is the screen)
                RenderTexture::bind(camera.getRenderTarget());
                // With the final viewport
                driver.setViewport(viewport);
            }
            else
            {
                // Bind the next effect buffer
                RenderTexture::bind(targetBuffer);
            }

            // Apply effect material
            const Camera::Effect & effect = camera.getEffect(i);
            Material & material = *effect.material.get();
            if (material.getShader())
            {
                ShaderProgram * shader = material.getShader();
                driver.useProgram(shader);

                material.setRenderTexture(Material::MAIN_TEXTURE, sourceBuffer);

                // TODO This code is temporary. A better approach is needed.
                // VR Support callback
                if (vr)
                {
                    vr->onRenderEye(&camera, vrEyeIndex, &material, sourceBuffer->getSize(), viewport);
                }

                // No projection, no modelview. Everything is [-1, 1].

                material.applyParameters();
            }

            Mesh * effectMesh = effect.mesh.get();
            if (effectMesh == nullptr)
                effectMesh = m_effectQuad;

            // Draw
            driver.drawMesh(*effectMesh);

            driver.useProgram(nullptr);
        }
    }

    // Go back to screen
    RenderTexture::bind(0);

    SN_END_PROFILE_SAMPLE();
}

//------------------------------------------------------------------------------
void RenderManager::serializeState(sn::Variant & o, const SerializationContext & ctx)
{
    Entity::serializeState(o, ctx);
    // TODO serialize RenderManager settings
}

//------------------------------------------------------------------------------
void RenderManager::unserializeState(const Variant & o, const SerializationContext & ctx)
{
    Entity::unserializeState(o, ctx);
    if (o.isDictionary())
    {
        const Variant & screensValue = o["screens"];
        if (screensValue.isArray())
        {
            const Variant::Array & a = screensValue.getArray();
            for (u32 i = 0; i < a.size(); ++i)
            {
                const Variant & screenValue = a[i];
                WindowParams params;
                sn::unserialize(screenValue, params);
                addScreen(i, &params);
            }
        }
    }
}

} // namespace sn

