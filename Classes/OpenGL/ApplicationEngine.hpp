//
//  ApplicationEngine.h
//  WireframeSkeleton
//
//  Created by Aurelien Cobb on 22/02/2014.
//
//

#ifndef WireframeSkeleton_ApplicationEngine_h
#define WireframeSkeleton_ApplicationEngine_h

#include "Interfaces.hpp"
#include "ParametricEquations.hpp"
#include <algorithm>

using namespace std;

static const int SurfaceCount = 6;
static const int ButtonCount = SurfaceCount - 1;
static const float AnimationDuration = 0.3;

struct Animation {
    bool Active;
    float Elapsed;
    float Duration;
    Visual StartingVisuals[SurfaceCount];
    Visual EndingVisuals[SurfaceCount];
};

class ApplicationEngine : public IApplicationEngine {
public:
    ApplicationEngine(IRenderingEngine * renderingEngine);
    ~ApplicationEngine();
    void Initialize(int width, int height);
    void Render() const;
    void UpdateAnimation(float timeStep);
    void OnFingerUp(ivec2 location);
    void OnFingerDown(ivec2 location);
    void OnFingerMove(ivec2 oldLocation, ivec2 newLocation);
private:
    void PopulateVisuals(Visual * visuals) const;
    int MapToButton(ivec2 touchPoint) const;
    vec3 MapToSphere(ivec2 touchPoint) const;
    float m_trackballRadius;
    ivec2 m_screenSize;
    ivec2 m_centerPoint;
    ivec2 m_fingerStart;
    bool m_spinning;
    Quaternion m_orientation;
    Quaternion m_previousOrientation;
    IRenderingEngine * m_renderingEngine;
    int m_currentSurface;
    ivec2 m_buttonSize;
    int m_pressedButton;
    int m_buttonSurfaces[ButtonCount];
    Animation m_animation;
};

#endif
