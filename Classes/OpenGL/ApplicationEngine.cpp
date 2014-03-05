//
//  ApplicationEngine.cpp
//  WireframeSkeleton
//
//  Created by Aurelien Cobb on 22/02/2014.
//
//

#include "ApplicationEngine.hpp"

IApplicationEngine * CreateApplicationEngine(IRenderingEngine * renderingEngine, IResourceManager * resourceManager) {
    return new ApplicationEngine(renderingEngine, resourceManager);
}

ApplicationEngine::ApplicationEngine(IRenderingEngine * renderingEngine, IResourceManager * resourceManager) :
    m_spinning(false), m_renderingEngine(renderingEngine), m_pressedButton(-1), m_resourceManager(resourceManager) {
        m_buttonSurfaces[0] = 0;
        m_buttonSurfaces[1] = 1;
        m_buttonSurfaces[2] = 2;
        m_buttonSurfaces[3] = 4;
        m_buttonSurfaces[4] = 5;
        m_currentSurface = 3;
        m_animation.Active = false;
}

ApplicationEngine::~ApplicationEngine() {
    delete m_renderingEngine;
}

void ApplicationEngine::Initialize(int width, int height) {
    m_trackballRadius = width / 3;
    m_buttonSize.x = width / ButtonCount;
    m_buttonSize.y = m_buttonSize.x;
    m_screenSize = ivec2(width, height);
    m_centerPoint = m_screenSize / 2;
    
    vector<ISurface*>surfaces(SurfaceCount);
    string path = m_resourceManager->GetResourcepath();
    surfaces[0] = new ObjSurface(path + "/Ninja.obj");
    surfaces[1] = new Sphere(1.4);
    surfaces[2] = new Torus(1.4, 0.3);
    surfaces[3] = new TrefoilKnot(1.8);
    surfaces[4] = new ObjSurface(path + "/micronapalmv2.obj");
    surfaces[5] = new MobiusStrip(1);
    m_renderingEngine->Initialize(surfaces);
    for (int i = 0; i < SurfaceCount; i++) {
        delete surfaces[i];
    }
}

void ApplicationEngine::PopulateVisuals(Visual * visuals) const {
    for (int buttonIndex = 0; buttonIndex < ButtonCount; ++buttonIndex) {
        int visualIndex = m_buttonSurfaces[buttonIndex];
        visuals[visualIndex].Color = vec3(0.75, 0.75, 0.75);
        if (m_pressedButton == buttonIndex) {
            visuals[visualIndex].Color = vec3(1.0, 1.0, 1.0);
        }
        visuals[visualIndex].ViewportSize = m_buttonSize;
        visuals[visualIndex].LowerLeft = ivec2(buttonIndex * m_buttonSize.x, 0);
        visuals[visualIndex].Orientation = Quaternion();
    }
    visuals[m_currentSurface].Color = m_spinning ? vec3(1, 1, 1) : vec3(0, 1, 1);
    
    visuals[m_currentSurface].LowerLeft = ivec2(0, m_buttonSize.y);
    visuals[m_currentSurface].ViewportSize = ivec2(m_screenSize.x, m_screenSize.y - m_buttonSize.y);
    visuals[m_currentSurface].Orientation = m_orientation;
}

void ApplicationEngine::Render() const {
    vector<Visual> visuals(SurfaceCount);
    if (!m_animation.Active) {
        PopulateVisuals(&visuals[0]);
    } else {
        float t = 0;
        if (m_animation.Duration != 0) {
            t = m_animation.Elapsed / m_animation.Duration;
        }
        for (int i = 0; i < SurfaceCount; i++) {
            const Visual & start = m_animation.StartingVisuals[i];
            const Visual & end = m_animation.EndingVisuals[i];
            Visual & tweened = visuals[i];
            
            tweened.Color = start.Color.Lerp(t, end.Color);
            tweened.LowerLeft = start.LowerLeft.Lerp(t, end.LowerLeft);
            tweened.ViewportSize = start.ViewportSize.Lerp(t, end.ViewportSize);
            tweened.Orientation = start.Orientation.Slerp(t, end.Orientation);
        }
    }
    m_renderingEngine->Render(visuals);
}

void ApplicationEngine::UpdateAnimation(float timeStep) {
    if (m_animation.Active) {
        m_animation.Elapsed += timeStep;
        if (m_animation.Elapsed > m_animation.Duration) {
            m_animation.Active = false;
        }
    }
}

void ApplicationEngine::OnFingerUp(ivec2 location) {
    m_spinning = false;
    if (m_pressedButton != -1 && m_pressedButton == MapToButton(location) && !m_animation.Active) {
        m_animation.Active = true;
        m_animation.Elapsed = 0;
        m_animation.Duration = AnimationDuration;
        PopulateVisuals(&m_animation.StartingVisuals[0]);
        swap(m_buttonSurfaces[m_pressedButton], m_currentSurface);
        PopulateVisuals(&m_animation.EndingVisuals[0]);
    }
    m_pressedButton = -1;
}

void ApplicationEngine::OnFingerDown(ivec2 location) {
    m_fingerStart = location;
    m_previousOrientation = m_orientation;
    m_pressedButton = MapToButton(location);
    if (m_pressedButton == -1) {
        m_spinning = true;
    }
}

void ApplicationEngine::OnFingerMove(ivec2 oldLocation, ivec2 newLocation) {
    if (m_spinning) {
        vec3 start = MapToSphere(m_fingerStart);
        vec3 end = MapToSphere(newLocation);
        Quaternion delta = Quaternion::CreateFromVectors(start, end);
        m_orientation = delta.Rotated(m_previousOrientation);
    }
    if (m_pressedButton != -1 && m_pressedButton != MapToButton(newLocation)) {
        m_pressedButton = -1;
    }
}

int ApplicationEngine::MapToButton(ivec2 touchPoint) const {
    if (touchPoint.y < m_screenSize.y - m_buttonSize.y) {
        return -1;
    }
    int buttonIndex = touchPoint.x / m_buttonSize.x;
    if (buttonIndex >= ButtonCount) {
        return -1;
    }
    return buttonIndex;
}

vec3 ApplicationEngine::MapToSphere(ivec2 touchPoint) const {
    vec2 p = touchPoint - m_centerPoint;
    p.y = -p.y;
    const float radius = m_trackballRadius;
    const float safeRadius = radius - 1;
    if (p.Length() > safeRadius) {
        float theta = atan2(p.y, p.x);
        p.x = safeRadius * cos(theta);
        p.y = safeRadius * sin(theta);
    }
    float z = sqrt(radius * radius - p.LengthSquared());
    vec3 mapped = vec3(p.x, p.y, z);
    return mapped / radius;
}